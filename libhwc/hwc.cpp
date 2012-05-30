/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <s3c-fb.h>

#include <EGL/egl.h>

#include <cutils/log.h>
#include <hardware/gralloc.h>
#include <hardware/hardware.h>
#include <hardware/hwcomposer.h>
#include <hardware_legacy/uevent.h>
#include <utils/Vector.h>

#include "ump.h"
#include "ion.h"
#include "gralloc_priv.h"

struct hwc_callback_entry
{
	void (*callback)(void *, private_handle_t *);
	void *data;
};
typedef android::Vector<struct hwc_callback_entry> hwc_callback_queue_t;

const size_t NUM_HW_WINDOWS = 5;
const size_t NO_FB_NEEDED = NUM_HW_WINDOWS + 1;

struct exynos5_hwc_composer_device_t;

struct exynos5_hwc_post_data_t {
	exynos5_hwc_composer_device_t	*pdev;
	hwc_layer_t		*overlays[NUM_HW_WINDOWS];
	size_t			fb_window;
};

struct exynos5_hwc_composer_device_t {
	hwc_composer_device_t	base;

	int			fd;
	exynos5_hwc_post_data_t	bufs;

	const private_module_t	*gralloc_module;
	hwc_procs_t		*procs;
	pthread_t		vsync_thread;
};

static void dump_layer(hwc_layer_t const *l)
{
	ALOGV("\ttype=%d, flags=%08x, handle=%p, tr=%02x, blend=%04x, "
			"{%d,%d,%d,%d}, {%d,%d,%d,%d}",
			l->compositionType, l->flags, l->handle, l->transform,
			l->blending,
			l->sourceCrop.left,
			l->sourceCrop.top,
			l->sourceCrop.right,
			l->sourceCrop.bottom,
			l->displayFrame.left,
			l->displayFrame.top,
			l->displayFrame.right,
			l->displayFrame.bottom);
}

static void dump_handle(private_handle_t *h)
{
	ALOGV("\t\tformat = %d, width = %u, height = %u, bpp = %u, stride = %u",
			h->format, h->width, h->height, h->bpp, h->stride);
}

static void dump_config(s3c_fb_win_config &c)
{
	ALOGV("\tstate = %u", c.state);
	if (c.state == c.S3C_FB_WIN_STATE_BUFFER) {
		ALOGV("\t\tfd = %d, offset = %u, stride = %u, "
				"x = %d, y = %d, w = %u, h = %u, "
				"format = %u",
				c.fd, c.offset, c.stride,
				c.x, c.y, c.w, c.h,
				c.format);
	}
	else if (c.state == c.S3C_FB_WIN_STATE_COLOR) {
		ALOGV("\t\tcolor = %u", c.color);
	}
}

inline int WIDTH(const hwc_rect &rect) { return rect.right - rect.left; }
inline int HEIGHT(const hwc_rect &rect) { return rect.bottom - rect.top; }

static bool is_scaled(const hwc_layer_t &layer)
{
	return WIDTH(layer.displayFrame) != WIDTH(layer.sourceCrop) ||
			HEIGHT(layer.displayFrame) != HEIGHT(layer.sourceCrop);
}

static enum s3c_fb_pixel_format exynos5_format_to_s3c_format(int format)
{
	switch (format) {
	case HAL_PIXEL_FORMAT_RGBA_8888:
		return S3C_FB_PIXEL_FORMAT_RGBA_8888;
	case HAL_PIXEL_FORMAT_RGB_888:
		return S3C_FB_PIXEL_FORMAT_RGB_888;
	case HAL_PIXEL_FORMAT_RGB_565:
		return S3C_FB_PIXEL_FORMAT_RGB_565;
	case HAL_PIXEL_FORMAT_BGRA_8888:
		return S3C_FB_PIXEL_FORMAT_BGRA_8888;
	case HAL_PIXEL_FORMAT_RGBA_5551:
		return S3C_FB_PIXEL_FORMAT_RGBA_5551;
	case HAL_PIXEL_FORMAT_RGBA_4444:
		return S3C_FB_PIXEL_FORMAT_RGBA_4444;

	default:
		return S3C_FB_PIXEL_FORMAT_MAX;
	}
}

static bool exynos5_format_is_supported(int format)
{
	return exynos5_format_to_s3c_format(format) < S3C_FB_PIXEL_FORMAT_MAX;
}

static bool exynos5_format_is_supported_by_gscaler(int format)
{
	switch(format) {
	case HAL_PIXEL_FORMAT_RGB_888:
	case HAL_PIXEL_FORMAT_RGB_565:
	case HAL_PIXEL_FORMAT_YV12:
		return true;

	default:
		return false;
	}
}

static uint8_t exynos5_format_to_bpp(int format)
{
	switch (format) {
	case HAL_PIXEL_FORMAT_RGBA_8888:
	case HAL_PIXEL_FORMAT_BGRA_8888:
		return 32;

	case HAL_PIXEL_FORMAT_RGB_888:
		return 24;

	case HAL_PIXEL_FORMAT_RGB_565:
	case HAL_PIXEL_FORMAT_RGBA_5551:
	case HAL_PIXEL_FORMAT_RGBA_4444:
		return 16;

	default:
		ALOGW("unrecognized pixel format %u", format);
		return 0;
	}
}

bool exynos5_supports_overlay(hwc_layer_t &layer, size_t i) {
/*	private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);

	if (!handle) {
		ALOGW("\tlayer %u: handle is NULL", i);
		return false;
	}
	if (!exynos5_format_is_supported(handle->format)) {
		ALOGW("\tlayer %u: pixel format %u not supported", i,
				handle->format);
		return false;
	}
	if (is_scaled(layer)) {
		// TODO: this can be made into an overlay if a gscaler
		// unit is available.  Also some size and pixel format
		// limitations (see 46-3 of datasheet)
		ALOGW("\tlayer %u: scaling and transforming not supported", i);
		return false;
	}
	if (layer.blending != HWC_BLENDING_NONE) {
		// TODO: support this
		ALOGW("\tlayer %u: blending not supported", i);
		return false;
	}

	return true;*/

	return false;
	// for debugging purposes, everything goes into a framebuffer
}

static int exynos5_prepare(hwc_composer_device_t *dev, hwc_layer_list_t* list)
{
	if (!list)
		return 0;

	ALOGV("preparing %u layers", list->numHwLayers);

	exynos5_hwc_composer_device_t *pdev =
			(exynos5_hwc_composer_device_t *)dev;
	memset(pdev->bufs.overlays, 0, sizeof(pdev->bufs.overlays));

	bool fb_needed = false;
	size_t first_fb = 0, last_fb;

	// find unsupported overlays
	for (size_t i = 0; i < list->numHwLayers; i++) {
		hwc_layer_t &layer = list->hwLayers[i];

		if (layer.compositionType == HWC_BACKGROUND) {
			ALOGV("\tlayer %u: background supported", i);
			continue;
		}

		if (exynos5_supports_overlay(list->hwLayers[i], i)) {
			ALOGV("\tlayer %u: overlay supported", i);
			layer.compositionType = HWC_OVERLAY;
			continue;
		}

		if (!fb_needed) {
			first_fb = i;
			fb_needed = true;
		}
		last_fb = i;
		layer.compositionType = HWC_FRAMEBUFFER;
	}

	// can't composite overlays sandwiched between framebuffers
	if (fb_needed)
		for (size_t i = first_fb; i < last_fb; i++)
			list->hwLayers[i].compositionType = HWC_FRAMEBUFFER;

	// if we need a framebuffer but can't fit it in, reserve the last
	// window as a framebuffer
	if (fb_needed && first_fb >= NUM_HW_WINDOWS) {
		first_fb = NUM_HW_WINDOWS - 1;
		for (size_t i = first_fb + 1; i < list->numHwLayers; i++)
			list->hwLayers[i].compositionType = HWC_FRAMEBUFFER;
	}

	unsigned int nextWindow = 0;

	// assign as many overlays as possible to windows
	for (size_t i = 0; i < list->numHwLayers; i++) {
		hwc_layer_t &layer = list->hwLayers[i];

		if (nextWindow == NUM_HW_WINDOWS) {
			ALOGV("not enough windows to assign layer %u", i);
			layer.compositionType = HWC_FRAMEBUFFER;
			continue;
		}

		if (fb_needed && i == first_fb) {
			ALOGV("assigning framebuffer to window %u\n",
					nextWindow);
			nextWindow++;
			continue;
		}

		if (layer.compositionType != HWC_FRAMEBUFFER) {
			ALOGV("assigning layer %u to window %u", i, nextWindow);
			pdev->bufs.overlays[nextWindow] = &layer;
			nextWindow++;
		}
	}

	if (fb_needed)
		pdev->bufs.fb_window = first_fb;
	else
		pdev->bufs.fb_window = NO_FB_NEEDED;

	for (size_t i = 0; i < list->numHwLayers; i++) {
		dump_layer(&list->hwLayers[i]);
		if(list->hwLayers[i].handle)
			dump_handle(private_handle_t::dynamicCast(
					list->hwLayers[i].handle));
	}

	return 0;
}

static void exynos5_config_handle(private_handle_t *handle,
		hwc_rect_t &sourceCrop, hwc_rect_t &displayFrame,
		s3c_fb_win_config &cfg) {
	cfg.state = cfg.S3C_FB_WIN_STATE_BUFFER;
	cfg.fd = handle->fd;
	cfg.x = displayFrame.left;
	cfg.y = displayFrame.top;
	cfg.w = WIDTH(displayFrame);
	cfg.h = HEIGHT(displayFrame);
	cfg.format = exynos5_format_to_s3c_format(handle->format);
	uint8_t bpp = exynos5_format_to_bpp(handle->format);
	cfg.offset = (sourceCrop.top * handle->stride + sourceCrop.left) *
			bpp / 8;
	cfg.stride = handle->stride * bpp / 8;
}


static void exynos5_config_overlay(hwc_layer_t *layer, s3c_fb_win_config &cfg)
{
	if (!layer)
		return;

	if (layer->compositionType == HWC_BACKGROUND) {
		hwc_color_t color = layer->backgroundColor;
		cfg.state = cfg.S3C_FB_WIN_STATE_COLOR;
		cfg.color = (color.r << 16) | (color.g << 8) | color.b;
		return;
	}

	private_handle_t *handle = private_handle_t::dynamicCast(layer->handle);
	exynos5_config_handle(handle, layer->sourceCrop, layer->displayFrame,
			cfg);
}

static void exynos5_post_callback(void *data, private_handle_t *fb)
{
	exynos5_hwc_post_data_t *pdata =
			(exynos5_hwc_post_data_t *)data;

	struct s3c_fb_win_config config[NUM_HW_WINDOWS];
	memset(config, 0, sizeof(config));
	for (size_t i = 0; i < NUM_HW_WINDOWS; i++) {
		if (i == pdata->fb_window) {
			hwc_rect_t rect = { 0, 0, fb->width, fb->height };
			exynos5_config_handle(fb, rect, rect, config[i]);
		} else
			exynos5_config_overlay(pdata->overlays[i], config[i]);
		dump_config(config[i]);
	}

	int ret = ioctl(pdata->pdev->fd, S3CFB_WIN_CONFIG, config);
	if (ret < 0)
		ALOGE("ioctl S3CFB_WIN_CONFIG failed: %d", ret);

	free(pdata);
}

static int exynos5_set(struct hwc_composer_device *dev, hwc_display_t dpy,
		hwc_surface_t sur, hwc_layer_list_t* list)
{
	exynos5_hwc_composer_device_t *pdev =
			(exynos5_hwc_composer_device_t *)dev;

	if (!dpy || !sur)
		return 0;

	hwc_callback_queue_t *queue = NULL;
	pthread_mutex_t *lock = NULL;
	exynos5_hwc_post_data_t *data = NULL;

	if (list) {
		data = (exynos5_hwc_post_data_t *)
				malloc(sizeof(exynos5_hwc_post_data_t));
		memcpy(data, &pdev->bufs, sizeof(pdev->bufs));

		if (pdev->bufs.fb_window == NO_FB_NEEDED) {
			exynos5_post_callback(data, NULL);
			return 0;
		}

		struct hwc_callback_entry entry;
		entry.callback = exynos5_post_callback;
		entry.data = data;

		queue = reinterpret_cast<hwc_callback_queue_t *>(
			pdev->gralloc_module->queue);
		lock = const_cast<pthread_mutex_t *>(
			&pdev->gralloc_module->queue_lock);

		pthread_mutex_lock(lock);
		queue->push_front(entry);
		pthread_mutex_unlock(lock);
	}

	EGLBoolean success = eglSwapBuffers((EGLDisplay)dpy, (EGLSurface)sur);
	if (!success) {
		ALOGE("HWC_EGL_ERROR");
		if (list) {
			pthread_mutex_lock(lock);
			queue->removeAt(0);
			pthread_mutex_unlock(lock);
			free(data);
		}
		return HWC_EGL_ERROR;
	}

	return 0;
}

static void exynos5_registerProcs(struct hwc_composer_device* dev,
		hwc_procs_t const* procs)
{
	struct exynos5_hwc_composer_device_t* pdev =
			(struct exynos5_hwc_composer_device_t*)dev;
	pdev->procs = const_cast<hwc_procs_t *>(procs);
}

static int exynos5_query(struct hwc_composer_device* dev, int what, int *value)
{
	struct exynos5_hwc_composer_device_t *pdev =
			(struct exynos5_hwc_composer_device_t *)dev;

	switch (what) {
	case HWC_BACKGROUND_LAYER_SUPPORTED:
		// we support the background layer
		value[0] = 1;
		break;
	case HWC_VSYNC_PERIOD:
		// vsync period in nanosecond
		value[0] = 1000000000.0 / pdev->gralloc_module->fps;
		break;
	default:
		// unsupported query
		return -EINVAL;
	}
	return 0;
}

static int exynos5_eventControl(struct hwc_composer_device *dev, int event,
		int enabled)
{
	struct exynos5_hwc_composer_device_t *pdev =
			(struct exynos5_hwc_composer_device_t *)dev;

	switch (event) {
	case HWC_EVENT_VSYNC:
		__u32 val = !!enabled;
		int err = ioctl(pdev->fd, S3CFB_SET_VSYNC_INT, &val);
		if (err < 0) {
			ALOGE("vsync ioctl failed");
			return -errno;
		}

		return 0;
	}

	return -EINVAL;
}

static void handle_vsync_uevent(struct exynos5_hwc_composer_device_t *pdev,
		const char *buff, int len)
{
	uint64_t timestamp = 0;
	const char *s = buff;

	if (!pdev->procs || !pdev->procs->vsync)
		return;

	s += strlen(s) + 1;

	while (*s) {
		if (!strncmp(s, "VSYNC=", strlen("VSYNC=")))
			timestamp = strtoull(s + strlen("VSYNC="), NULL, 0);

		s += strlen(s) + 1;
		if (s - buff >= len)
			break;
	}

	pdev->procs->vsync(pdev->procs, 0, timestamp);
}

static void *hwc_vsync_thread(void *data)
{
	struct exynos5_hwc_composer_device_t *pdev =
			(struct exynos5_hwc_composer_device_t *)data;
	char uevent_desc[4096];
	memset(uevent_desc, 0, sizeof(uevent_desc));

	setpriority(PRIO_PROCESS, 0, HAL_PRIORITY_URGENT_DISPLAY);

	uevent_init();
	while (true) {
		int len = uevent_next_event(uevent_desc,
				sizeof(uevent_desc) - 2);

		bool vsync = !strcmp(uevent_desc,
				"change@/devices/platform/exynos5-fb.1");
		if (vsync)
			handle_vsync_uevent(pdev, uevent_desc, len);
	}

	return NULL;
}

struct hwc_methods exynos5_methods = {
	eventControl: exynos5_eventControl,
};

static int exynos5_close(hw_device_t* device);

static int exynos5_open(const struct hw_module_t *module, const char *name,
		struct hw_device_t **device)
{
	int ret;

	if (strcmp(name, HWC_HARDWARE_COMPOSER)) {
		return -EINVAL;
	}

	struct exynos5_hwc_composer_device_t *dev;
	dev = (struct exynos5_hwc_composer_device_t *)malloc(sizeof(*dev));
	memset(dev, 0, sizeof(*dev));

	if (hw_get_module(GRALLOC_HARDWARE_MODULE_ID,
			(const struct hw_module_t **)&dev->gralloc_module)) {
		ALOGE("failed to get gralloc hw module");
		ret = -EINVAL;
		goto err_get_module;
	}

	dev->fd = open("/dev/graphics/fb0", O_RDWR);
	if (dev->fd < 0) {
		ALOGE("failed to open framebuffer");
		ret = dev->fd;
		goto err_get_module;
	}

	dev->base.common.tag = HARDWARE_DEVICE_TAG;
	dev->base.common.version = HWC_DEVICE_API_VERSION_0_3;
	dev->base.common.module = const_cast<hw_module_t *>(module);
	dev->base.common.close = exynos5_close;

	dev->base.prepare = exynos5_prepare;
	dev->base.set = exynos5_set;
	dev->base.registerProcs = exynos5_registerProcs;
	dev->base.query = exynos5_query;
	dev->base.methods = &exynos5_methods;

	dev->bufs.pdev = dev;

	*device = &dev->base.common;

	ret = pthread_create(&dev->vsync_thread, NULL, hwc_vsync_thread, dev);
	if (ret) {
		ALOGE("failed to start vsync thread: %s", strerror(ret));
		ret = -ret;
		goto err_ioctl;
	}

	return 0;

err_ioctl:
	close(dev->fd);
err_get_module:
	free(dev);
	return ret;
}

static int exynos5_close(hw_device_t *device)
{
	struct exynos5_hwc_composer_device_t *dev =
			(struct exynos5_hwc_composer_device_t *)device;
	close(dev->fd);
	return 0;
}

static struct hw_module_methods_t exynos5_hwc_module_methods = {
	open: exynos5_open,
};

hwc_module_t HAL_MODULE_INFO_SYM = {
	common: {
		tag: HARDWARE_MODULE_TAG,
		module_api_version: HWC_MODULE_API_VERSION_0_1,
		hal_api_version: HARDWARE_HAL_API_VERSION,
		id: HWC_HARDWARE_MODULE_ID,
		name: "Samsung exynos5 hwcomposer module",
		author: "Google",
		methods: &exynos5_hwc_module_methods,
	}
};
