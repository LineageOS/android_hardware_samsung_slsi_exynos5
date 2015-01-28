/*
 * Copyright (C) 2008 The Android Open Source Project
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

#include <limits.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <cutils/log.h>
#include <cutils/atomic.h>

#include <hardware/hardware.h>
#include <hardware/gralloc.h>

#include "gralloc_priv.h"
#include "exynos_format.h"

#include <ion/ion.h>
#include <linux/ion.h>

/*****************************************************************************/

static int gralloc_map(gralloc_module_t const* module, buffer_handle_t handle)
{
    private_handle_t* hnd = (private_handle_t*)handle;

    void* mappedAddress = mmap(0, hnd->size, PROT_READ|PROT_WRITE, MAP_SHARED, 
                               hnd->fd, 0);
    if (mappedAddress == MAP_FAILED) {
        ALOGE("%s: could not mmap %s", __func__, strerror(errno));
        return -errno;
    }
    ALOGV("%s: base %p %d %d %d %d\n", __func__, mappedAddress, hnd->size,
          hnd->width, hnd->height, hnd->stride);
    hnd->base = mappedAddress;
    return 0;
}

static int gralloc_unmap(gralloc_module_t const* module, buffer_handle_t handle)
{
    private_handle_t* hnd = (private_handle_t*)handle;

    if (hnd->base != NULL && munmap(hnd->base, hnd->size) < 0) {
        ALOGE("%s :could not unmap %s %p %d", __func__, strerror(errno),
              hnd->base, hnd->size);
    }
    hnd->base = NULL;
    if (hnd->base1 != NULL && hnd->format == HAL_PIXEL_FORMAT_YCbCr_420_SP_TILED) {
        // unmap plane-2 of mapped NV12 TILED format
        size_t chroma_vstride = ALIGN(hnd->height / 2, 32);
        size_t chroma_size = chroma_vstride * hnd->stride;
        if (munmap(hnd->base1, chroma_size) < 0) {
            ALOGE("%s :could not unmap %s %p %zd", __func__, strerror(errno),
                  hnd->base1, chroma_size);
        }
        hnd->base1 = NULL;
    }
    ALOGV("%s: base %p %d %d %d %d\n", __func__, hnd->base, hnd->size,
          hnd->width, hnd->height, hnd->stride);
    return 0;
}


static int gralloc_map_yuv(gralloc_module_t const* module, buffer_handle_t handle)
{
    private_handle_t* hnd = (private_handle_t*)handle;

    // only support NV12 TILED format
    if (hnd->format != HAL_PIXEL_FORMAT_YCbCr_420_SP_TILED) {
        return -EINVAL;
    }

    size_t chroma_vstride = ALIGN(hnd->height / 2, 32);
    size_t chroma_size = chroma_vstride * hnd->stride;
    ALOGI("map_yuv: size=%d/%zu", hnd->size, chroma_size);

    if (hnd->base == NULL) {
        int err = gralloc_map(module, handle);
        if (err != 0) {
            return err;
        }
    }

    // map plane-2 for NV12 TILED format
    void* mappedAddress = mmap(0, chroma_size, PROT_READ|PROT_WRITE, MAP_SHARED,
                               hnd->fd1, 0);
    if (mappedAddress == MAP_FAILED) {
        ALOGE("%s: could not mmap %s", __func__, strerror(errno));
        gralloc_unmap(module, handle);
        return -errno;
    }

    ALOGV("%s: chroma %p %d %d %d %d\n", __func__, mappedAddress, hnd->size,
          hnd->width, hnd->height, hnd->stride);
    hnd->base1 = mappedAddress;
    return 0;
}

/*****************************************************************************/

int getIonFd(gralloc_module_t const *module)
{
    private_module_t* m = const_cast<private_module_t*>(reinterpret_cast<const private_module_t*>(module));
    if (m->ionfd == -1)
        m->ionfd = ion_open();
    return m->ionfd;
}

static pthread_mutex_t sMapLock = PTHREAD_MUTEX_INITIALIZER; 

/*****************************************************************************/

int gralloc_register_buffer(gralloc_module_t const* module,
                            buffer_handle_t handle)
{
    if (private_handle_t::validate(handle) < 0)
        return -EINVAL;

    private_handle_t* hnd = (private_handle_t*)handle;
    ALOGV("%s: base %p %d %d %d %d\n", __func__, hnd->base, hnd->size,
          hnd->width, hnd->height, hnd->stride);

    int ret;
    ret = ion_import(getIonFd(module), hnd->fd, &hnd->handle);
    if (ret)
        ALOGE("error importing handle %d %x\n", hnd->fd, hnd->format);
    if (hnd->fd1 >= 0) {
        ret = ion_import(getIonFd(module), hnd->fd1, &hnd->handle1);
        if (ret)
            ALOGE("error importing handle1 %d %x\n", hnd->fd1, hnd->format);
    }
    if (hnd->fd2 >= 0) {
        ret = ion_import(getIonFd(module), hnd->fd2, &hnd->handle2);
        if (ret)
            ALOGE("error importing handle2 %d %x\n", hnd->fd2, hnd->format);
    }

    return ret;
}

int gralloc_unregister_buffer(gralloc_module_t const* module,
                              buffer_handle_t handle)
{
    if (private_handle_t::validate(handle) < 0)
        return -EINVAL;

    private_handle_t* hnd = (private_handle_t*)handle;
    ALOGV("%s: base %p %d %d %d %d\n", __func__, hnd->base, hnd->size,
          hnd->width, hnd->height, hnd->stride);

    gralloc_unmap(module, handle);

    if (hnd->handle)
        ion_free(getIonFd(module), hnd->handle);
    if (hnd->handle1)
        ion_free(getIonFd(module), hnd->handle1);
    if (hnd->handle2)
        ion_free(getIonFd(module), hnd->handle2);

    return 0;
}

int gralloc_lock(gralloc_module_t const* module,
                 buffer_handle_t handle, int usage,
                 int l, int t, int w, int h,
                 void** vaddr)
{
    // this is called when a buffer is being locked for software
    // access. in thin implementation we have nothing to do since
    // not synchronization with the h/w is needed.
    // typically this is used to wait for the h/w to finish with
    // this buffer if relevant. the data cache may need to be
    // flushed or invalidated depending on the usage bits and the
    // hardware.

    if (private_handle_t::validate(handle) < 0)
        return -EINVAL;

    private_handle_t* hnd = (private_handle_t*)handle;
    if (!hnd->base)
        gralloc_map(module, hnd);
    *vaddr = (void*)hnd->base;
    return 0;
}

int gralloc_lock_ycbcr(gralloc_module_t const* module,
                 buffer_handle_t handle, int usage,
                 int l, int t, int w, int h,
                 struct android_ycbcr *ycbcr)
{
    // This is called when a YUV buffer is being locked for software
    // access. In this implementation we have nothing to do since
    // no synchronization with the HW is needed.
    // Typically this is used to wait for the h/w to finish with
    // this buffer if relevant. The data cache may need to be
    // flushed or invalidated depending on the usage bits and the
    // hardware.

    if (private_handle_t::validate(handle) < 0)
        return -EINVAL;

    private_handle_t* hnd = (private_handle_t*)handle;
    ALOGV("lock_ycbcr for fmt=%d %dx%d %dx%d %d", hnd->format, hnd->width, hnd->height,
          hnd->stride, hnd->vstride, hnd->size);
    switch (hnd->format) {
        case HAL_PIXEL_FORMAT_YCbCr_420_SP_TILED:
        {
            int err = 0;
            if (hnd->base1 == NULL || hnd->base == NULL) {
                err = gralloc_map_yuv(module, hnd);
            }
            if (err == 0) {
                ycbcr->y  = (void*)hnd->base;
                ycbcr->cb = (void*)hnd->base1;
                ycbcr->cr = (void*)((uint8_t *)hnd->base + 1);
                ycbcr->ystride = hnd->stride;
                ycbcr->cstride = hnd->stride;
                ycbcr->chroma_step = 2;
                memset(ycbcr->reserved, 0, sizeof(ycbcr->reserved));
            }
            return err;
        }
        default:
            return -EINVAL;
    }
}

int gralloc_perform(struct gralloc_module_t const* module, int operation, ... )
{
    // dummy implementation required to implement lock_ycbcr
    return -EINVAL;
}

int gralloc_unlock(gralloc_module_t const* module, 
                   buffer_handle_t handle)
{
    // we're done with a software buffer. nothing to do in this
    // implementation. typically this is used to flush the data cache.
    private_handle_t* hnd = (private_handle_t*)handle;
    ion_sync_fd(getIonFd(module), hnd->fd);
    if (hnd->fd1 >= 0)
        ion_sync_fd(getIonFd(module), hnd->fd1);
    if (hnd->fd2 >= 0)
        ion_sync_fd(getIonFd(module), hnd->fd2);

    if (private_handle_t::validate(handle) < 0)
        return -EINVAL;
    return 0;
}
