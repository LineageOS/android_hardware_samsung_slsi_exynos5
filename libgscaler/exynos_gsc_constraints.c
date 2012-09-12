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
#include "exynos_gsc_utils.h"

#define ALIGN_TRIMMED(x, a) ((x) - ((x) & (a - 1)))

enum gsc_format
{
    GSC_RGB888,
    GSC_RGB565,
    GSC_YUV422_1P,
    GSC_YUV422_2P,
    GSC_YUV422_3P,
    GSC_YUV420_2P,
    GSC_YUV420_3P,
    GSC_YUV444_3P,
    GSC_TILE,
    GSC_FORMAT_MAX,
};

static enum gsc_format exynos_hal_format_to_gsc_format(int format)
{
    int v4l2_format = HAL_PIXEL_FORMAT_2_V4L2_PIX(format);
    int ret = 0;
    unsigned int bpp, planes;

    switch (v4l2_format) {
    case V4L2_PIX_FMT_RGB32:
    case V4L2_PIX_FMT_BGR32:
    case V4L2_PIX_FMT_RGB24:
        return GSC_RGB888;

    case V4L2_PIX_FMT_RGB565:
        return GSC_RGB565;

    case V4L2_PIX_FMT_NV12MT_16X16:
        return GSC_TILE;

    default:
        break;
    }

    bpp = get_yuv_bpp(v4l2_format);
    planes = get_yuv_planes(v4l2_format);

    switch (bpp) {
    case 12:
        switch (planes) {
        case 2:
            return GSC_YUV420_2P;
        case 3:
            return GSC_YUV420_3P;
        default:
            return GSC_FORMAT_MAX;
        }

    case 16:
        switch (planes) {
        case 1:
            return GSC_YUV422_1P;
        case 2:
            return GSC_YUV422_2P;
        case 3:
            return GSC_YUV422_3P;
        default:
            return GSC_FORMAT_MAX;
        }

    default:
        return GSC_FORMAT_MAX;
    }
}

struct gsc_size_constraint
{
    enum gsc_format format;
    unsigned int src_w_align;
    unsigned int src_h_align;
    unsigned int src_w_min;
    unsigned int src_w_max;
    unsigned int src_h_min;
    unsigned int src_h_max;
    unsigned int cropped_src_w_align;
    unsigned int cropped_src_h_align;
    unsigned int cropped_src_w_min;
    unsigned int cropped_src_w_max;
    unsigned int cropped_src_h_min;
    unsigned int cropped_src_h_max;
    unsigned int src_x_align;
    unsigned int src_y_align;
    bool dst_valid;
    unsigned int scaled_w_align;
    unsigned int scaled_h_align;
    unsigned int scaled_w_min;
    unsigned int scaled_w_max;
    unsigned int scaled_h_min;
    unsigned int scaled_h_max;
    unsigned int dst_w_align;
    unsigned int dst_h_align;
    unsigned int dst_w_min;
    unsigned int dst_w_max;
    unsigned int dst_h_min;
    unsigned int dst_h_max;
    unsigned int dst_x_align;
    unsigned int dst_y_align;
};

const struct gsc_size_constraint normal_constraint[GSC_FORMAT_MAX] = {
        {
                GSC_RGB888,
                16, 16, 32, 4800, 16, 3344, 1, 1, 32, 4800, 16, 3344, 2, 1,
                true,
                32, 1, 16, 4800, 8, 3344, 32, 1, 32, 4800, 8, 4800, 8, 1
        },
        {
                GSC_RGB565,
                16, 16, 32, 4800, 16, 3344, 1, 1, 32, 4800, 16, 3344, 2, 1,
                true,
                64, 1, 16, 4800, 8, 3344, 64, 1, 64, 4800, 8, 4800, 8, 1
        },
        {
                GSC_YUV422_1P,
                16, 16, 64, 4800, 16, 3344, 2, 1, 64, 4800, 16, 3344, 4, 1,
                true,
                64, 2, 32, 4800, 8, 3344, 64, 1, 64, 4800, 8, 4800, 8, 1
        },
        {
                GSC_YUV422_2P,
                16, 16, 64, 4800, 16, 3344, 2, 1, 64, 4800, 16, 3344, 4, 1,
                true,
                2, 2, 32, 4800, 8, 3344, 8, 1, 32, 4800, 8, 4800, 8, 1
        },
        {
                GSC_YUV422_3P,
                16, 16, 64, 4800, 16, 3344, 2, 1, 64, 4800, 16, 3344, 4, 1,
                false,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        },
        {
                GSC_YUV420_2P,
                16, 16, 64, 4800, 32, 3344, 2, 2, 64, 4800, 32, 3344, 4, 2,
                true,
                2, 2, 32, 4800, 16, 3344, 16, 1, 32, 4800, 16, 4800, 8, 2
        },
        {
                GSC_YUV420_3P,
                16, 16, 64, 4800, 32, 3344, 2, 2, 64, 4800, 32, 3344, 4, 2,
                true,
                2, 2, 32, 4800, 16, 3344, 16, 1, 32, 4800, 16, 4800, 16, 2
        },
        {
                GSC_YUV444_3P,
                16, 16, 32, 4800, 16, 3344, 1, 1, 32, 4800, 16, 3344, 2, 1,
                true,
                2, 2, 16, 4800, 8, 3344, 8, 1, 16, 4800, 8, 4800, 8, 1
        },
        {
                GSC_TILE,
                16, 16, 64, 4800, 32, 3344, 2, 2, 64, 2016, 32, 2016, 4, 2,
                true,
                2, 2, 32, 4800, 16, 3344, 8, 1, 32, 4800, 16, 4800, 8, 2
        },
};

const struct gsc_size_constraint rot90_constraint[GSC_FORMAT_MAX] = {
        {
                GSC_RGB888,
                16, 16, 16, 4800, 32, 3344, 1, 1, 16, 2016, 32, 2016, 2, 2,
                true,
                1, 32, 8, 3344, 32, 4800, 32, 1, 32, 4800, 16, 4800, 32, 1
        },
        {
                GSC_RGB565,
                16, 16, 16, 4800, 32, 3344, 1, 1, 16, 2016, 32, 2016, 2, 2,
                true,
                1, 64, 8, 3344, 64, 4800, 64, 1, 64, 4800, 16, 4800, 64, 1
        },
        {
                GSC_YUV422_1P,
                16, 16, 16, 4800, 64, 3344, 2, 2, 16, 2016, 64, 2016, 2, 2,
                true,
                2, 64, 8, 3344, 64, 4800, 64, 1, 64, 4800, 32, 4800, 64, 1
        },
        {
                GSC_YUV422_2P,
                16, 16, 16, 4800, 64, 3344, 2, 2, 16, 2016, 64, 2016, 2, 2,
                true,
                2, 2, 8, 3344, 32, 4800, 8, 1, 8, 4800, 32, 4800, 8, 1
        },
        {
                GSC_YUV422_3P,
                16, 16, 16, 4800, 64, 3344, 2, 2, 16, 2016, 64, 2016, 2, 2,
                false,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        },
        {
                GSC_YUV420_2P,
                16, 16, 32, 4800, 64, 3344, 2, 2, 32, 2016, 64, 2016, 2, 2,
                true,
                2, 2, 16, 3344, 32, 4800, 16, 1, 16, 4800, 32, 4800, 8, 2
        },
        {
                GSC_YUV420_3P,
                16, 16, 32, 4800, 64, 3344, 2, 2, 32, 2016, 64, 2016, 2, 2,
                true,
                2, 2, 16, 3344, 32, 4800, 16, 1, 16, 4800, 32, 4800, 16, 2
        },
        {
                GSC_YUV444_3P,
                16, 16, 16, 4800, 32, 3344, 1, 1, 16, 2016, 32, 2016, 2, 2,
                true,
                2, 2, 8, 3344, 16, 4800, 8, 1, 8, 4800, 16, 4800, 8, 1
        },
        {
                GSC_TILE,
                16, 16, 32, 4800, 32, 3344, 2, 2, 32, 2016, 64, 2016, 2, 2,
                true,
                2, 2, 16, 3344, 32, 4800, 8, 1, 16, 4800, 32, 4800, 8, 2
        },
};

static const struct gsc_size_constraint *eyxnos_gsc_get_constraint(int hal_format,
        int rotate)
{
    int gsc_format = exynos_hal_format_to_gsc_format(hal_format);

    if (gsc_format == GSC_FORMAT_MAX)
        return NULL;
    if (rotate & HAL_TRANSFORM_ROT_90)
        return &rot90_constraint[gsc_format];
    return &normal_constraint[gsc_format];
}

int exynos_gsc_prescaler_ratio(unsigned int src, unsigned int dst)
{
    if (src <= dst * 4)
        return 1;

    if (src <= dst * 8)
        return 2;

    if (src <= dst * 16)
        return 4;

    return -1;
}

bool exynos_gsc_cfg_valid(const exynos_gsc_img *src_cfg,
        const exynos_gsc_img *dst_cfg, bool local_path, bool check_dst_stride)
{
    const struct gsc_size_constraint *src_constraint =
            eyxnos_gsc_get_constraint(src_cfg->format, dst_cfg->rot);
    const struct gsc_size_constraint *dst_constraint =
            eyxnos_gsc_get_constraint(dst_cfg->format, dst_cfg->rot);

    int max_downscale = local_path ? 4 : 16;
    const int max_upscale = 8;
    uint32_t dst_w, dst_h;

    if (dst_cfg->rot & HAL_TRANSFORM_ROT_90) {
        dst_w = dst_cfg->h;
        dst_h = dst_cfg->w;
    } else {
        dst_w = dst_cfg->w;
        dst_h = dst_cfg->h;
    }

    return src_constraint && dst_constraint && dst_constraint->dst_valid &&
        src_cfg->fw % src_constraint->src_w_align == 0 &&
        src_cfg->fh % src_constraint->src_h_align == 0 &&
        src_cfg->fw >= src_constraint->src_w_min &&
        src_cfg->fw <= src_constraint->src_w_max &&
        src_cfg->fh >= src_constraint->src_h_min &&
        src_cfg->fh <= src_constraint->src_h_max &&
        src_cfg->w >= src_constraint->cropped_src_w_min &&
        src_cfg->w <= src_constraint->cropped_src_w_max &&
        src_cfg->h >= src_constraint->cropped_src_h_min &&
        src_cfg->h <= src_constraint->cropped_src_h_max &&

        (!check_dst_stride ||
            (dst_cfg->fw % dst_constraint->dst_w_align == 0 &&
             dst_cfg->fh % dst_constraint->dst_h_align == 0 &&
             dst_cfg->fw >= dst_constraint->dst_w_min &&
             dst_cfg->fw <= dst_constraint->dst_w_max &&
             dst_cfg->fh >= dst_constraint->dst_h_min &&
             dst_cfg->fh <= dst_constraint->dst_h_max)) &&

        dst_cfg->w >= dst_constraint->scaled_w_min &&
        dst_cfg->w <= dst_constraint->scaled_w_max &&
        dst_cfg->h >= dst_constraint->scaled_h_min &&
        dst_cfg->h <= dst_constraint->scaled_h_max &&

        src_cfg->w <= dst_w * max_downscale &&
        dst_w <= src_cfg->w * max_upscale &&
        src_cfg->h <= dst_h * max_downscale &&
        dst_h <= src_cfg->h * max_upscale;
}

bool exynos_gsc_cfg_aligned(const exynos_gsc_img *src_cfg,
        const exynos_gsc_img *dst_cfg)
{
    const struct gsc_size_constraint *src_constraint =
            eyxnos_gsc_get_constraint(src_cfg->format, dst_cfg->rot);
    const struct gsc_size_constraint *dst_constraint =
            eyxnos_gsc_get_constraint(dst_cfg->format, dst_cfg->rot);
    unsigned int prescale_w =
            exynos_gsc_prescaler_ratio(src_cfg->w, dst_cfg->w);
    unsigned int prescale_h =
            exynos_gsc_prescaler_ratio(src_cfg->h, dst_cfg->h);

    return src_cfg->x == ALIGN_TRIMMED(src_cfg->x, src_constraint->src_x_align) &&
        src_cfg->y == ALIGN_TRIMMED(src_cfg->y, src_constraint->src_y_align) &&
        src_cfg->w == ALIGN_TRIMMED(src_cfg->w,
                src_constraint->cropped_src_w_align * prescale_w) &&
        src_cfg->h == ALIGN_TRIMMED(src_cfg->h,
                src_constraint->cropped_src_h_align * prescale_h) &&
        dst_cfg->x == ALIGN_TRIMMED(dst_cfg->x, dst_constraint->dst_x_align) &&
        dst_cfg->y == ALIGN_TRIMMED(dst_cfg->y, dst_constraint->dst_y_align) &&
        dst_cfg->w == ALIGN_TRIMMED(dst_cfg->w, dst_constraint->scaled_w_align) &&
        dst_cfg->h == ALIGN_TRIMMED(dst_cfg->h, dst_constraint->scaled_h_align);
}

void exynos_gsc_align_cfg(exynos_gsc_img *src_cfg, exynos_gsc_img *dst_cfg)
{
    const struct gsc_size_constraint *src_constraint =
            eyxnos_gsc_get_constraint(src_cfg->format, dst_cfg->rot);
    const struct gsc_size_constraint *dst_constraint =
            eyxnos_gsc_get_constraint(dst_cfg->format, dst_cfg->rot);
    unsigned int prescale_w =
            exynos_gsc_prescaler_ratio(src_cfg->w, dst_cfg->w);
    unsigned int prescale_h =
            exynos_gsc_prescaler_ratio(src_cfg->h, dst_cfg->h);

    src_cfg->x = ALIGN_TRIMMED(src_cfg->x, src_constraint->src_x_align);
    src_cfg->y = ALIGN_TRIMMED(src_cfg->y, src_constraint->src_y_align);
    src_cfg->w = ALIGN_TRIMMED(src_cfg->w,
            src_constraint->cropped_src_w_align * prescale_w);
    src_cfg->h = ALIGN_TRIMMED(src_cfg->h,
            src_constraint->cropped_src_h_align * prescale_h);
    dst_cfg->x = ALIGN_TRIMMED(dst_cfg->x, dst_constraint->dst_x_align);
    dst_cfg->y = ALIGN_TRIMMED(dst_cfg->y, dst_constraint->dst_y_align);
    dst_cfg->w = ALIGN_TRIMMED(dst_cfg->w, dst_constraint->scaled_w_align);
    dst_cfg->h = ALIGN_TRIMMED(dst_cfg->h, dst_constraint->scaled_h_align);
}
