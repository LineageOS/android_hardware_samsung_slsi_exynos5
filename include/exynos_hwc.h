/*
 * Copyright (C) 2016 The CyanogenMod Project
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

#ifndef EXYNOS_HWC_H_
#define EXYNOS_HWC_H_

const size_t NUM_HW_WINDOWS = 5;
const size_t NO_FB_NEEDED = NUM_HW_WINDOWS + 1;
const size_t MAX_PIXELS = 2560 * 1600 * 2;
const size_t GSC_W_ALIGNMENT = 16;
const size_t GSC_H_ALIGNMENT = 16;
const size_t GSC_DST_CROP_W_ALIGNMENT_RGB888 = 32;
const size_t GSC_DST_W_ALIGNMENT_RGB888 = 32;
const size_t GSC_DST_H_ALIGNMENT_RGB888 = 1;
const size_t FIMD_GSC_IDX = 0;
const size_t HDMI_GSC_IDX = 1;
const int AVAILABLE_GSC_UNITS[] = { 0, 3 };
const size_t NUM_GSC_UNITS = sizeof(AVAILABLE_GSC_UNITS) /
        sizeof(AVAILABLE_GSC_UNITS[0]);
const size_t BURSTLEN_BYTES = 16 * 8;
const size_t NUM_HDMI_BUFFERS = 3;

const char *VSYNC_FD = "/sys/devices/platform/exynos5-fb.1/vsync";

const char *HDMI_MIXER_0 = "/dev/v4l-subdev7";
const char *HDMI_LAYERS[] = { "/dev/video16", "/dev/video17" };

static inline void setup_alpha_plane(s3c_fb_win_config *cfg, const int &value)
{
	// dummy
	//cfg->plane_alpha = value;
}

#endif //__EXYNOS_HWC_H__
