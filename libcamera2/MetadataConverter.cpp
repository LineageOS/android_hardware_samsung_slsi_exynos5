/*
**
** Copyright 2008, The Android Open Source Project
** Copyright 2012, Samsung Electronics Co. LTD
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

/*!
 * \file      MetadataConverter.cpp
 * \brief     source file for Metadata converter ( for camera hal2 implementation )
 * \author    Sungjoong Kang(sj3.kang@samsung.com)
 * \date      2012/05/31
 *
 * <b>Revision History: </b>
 * - 2012/05/31 : Sungjoong Kang(sj3.kang@samsung.com) \n
 *   Initial Release
 */

//#define LOG_NDEBUG 1
#define LOG_TAG "MetadataConverter"
#include <utils/Log.h>

#include "MetadataConverter.h"

namespace android {


MetadataConverter::MetadataConverter()
{
    return;
}


MetadataConverter::~MetadataConverter()
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return;
}

status_t MetadataConverter::CheckEntryTypeMismatch(camera_metadata_entry_t * entry,
    uint8_t type)
{
    if (!(entry->type==type))
    {
        ALOGV("DEBUG(%s):Metadata Missmatch tag(%s) type (%d) count(%d)",
            __FUNCTION__, get_camera_metadata_tag_name(entry->tag), entry->type, entry->count);
        return BAD_VALUE;
    }
    return NO_ERROR;
}

status_t MetadataConverter::CheckEntryTypeMismatch(camera_metadata_entry_t * entry,
    uint8_t type, size_t count)
{
    if (!((entry->type==type)&&(entry->count==count)))
    {
        ALOGV("DEBUG(%s):Metadata Missmatch tag(%s) type (%d) count(%d)",
            __FUNCTION__, get_camera_metadata_tag_name(entry->tag), entry->type, entry->count);
        return BAD_VALUE;
    }
    return NO_ERROR;
}

status_t MetadataConverter::ToInternalShot(camera_metadata_t * request, struct camera2_shot_ext * dst_ext)
{
    uint32_t    num_entry = 0;
    uint32_t    index = 0;
    uint32_t    i = 0;
    camera_metadata_entry_t curr_entry;
    struct camera2_shot * dst = NULL;

    ALOGV("DEBUG(%s):", __FUNCTION__);
    if (request == NULL || dst_ext == NULL)
        return BAD_VALUE;

    dst = &(dst_ext->shot);

    num_entry = (uint32_t)get_camera_metadata_data_count(request);
    for (index = 0 ; index < num_entry ; index++) {

        if (get_camera_metadata_entry(request, index, &curr_entry)==0) {
            //ALOGV("### MetadataConverter.ToInternalCtl. tag(%x)", curr_entry.tag);
            switch (curr_entry.tag) {

            case ANDROID_LENS_FOCUS_DISTANCE:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_FLOAT, 1))
                    break;
                dst->ctl.lens.focusDistance = curr_entry.data.f[0];
                break;

            case ANDROID_LENS_APERTURE:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_FLOAT, 1))
                    break;
                dst->ctl.lens.aperture = curr_entry.data.f[0];
                break;

            case ANDROID_LENS_FOCAL_LENGTH:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_FLOAT, 1))
                    break;
                dst->ctl.lens.focalLength = curr_entry.data.f[0];
                break;

            case ANDROID_LENS_FILTER_DENSITY:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_FLOAT, 1))
                    break;
                dst->ctl.lens.filterDensity = curr_entry.data.f[0];
                break;

            case ANDROID_LENS_OPTICAL_STABILIZATION_MODE:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_BYTE, 1))
                    break;
                dst->ctl.lens.opticalStabilizationMode =
                    (enum optical_stabilization_mode)curr_entry.data.u8[0];
                break;



            case ANDROID_SENSOR_EXPOSURE_TIME:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_INT64, 1))
                    break;
                dst->ctl.sensor.exposureTime =  curr_entry.data.i64[0];
                break;

            case ANDROID_SENSOR_FRAME_DURATION:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_INT64, 1))
                    break;
                dst->ctl.sensor.frameDuration = curr_entry.data.i64[0];
                break;

            case ANDROID_SENSOR_SENSITIVITY:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_INT32, 1))
                    break;
                dst->ctl.sensor.sensitivity = curr_entry.data.i32[0];
                break;



            case ANDROID_FLASH_MODE:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_BYTE, 1))
                    break;
                dst->ctl.flash.flashMode = (enum flash_mode)curr_entry.data.u8[0];
                break;

            case ANDROID_FLASH_FIRING_POWER:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_BYTE, 1))
                    break;
                dst->ctl.flash.firingPower = curr_entry.data.u8[0];
                break;

            case ANDROID_FLASH_FIRING_TIME:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_INT64, 1))
                    break;
                dst->ctl.flash.firingTime = curr_entry.data.i64[0];
                break;



            case ANDROID_HOT_PIXEL_MODE:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_BYTE, 1))
                    break;
                dst->ctl.hotpixel.mode = (enum processing_mode)curr_entry.data.u8[0];
                break;



            case ANDROID_DEMOSAIC_MODE:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_BYTE, 1))
                    break;
                dst->ctl.demosaic.mode = (enum processing_mode)curr_entry.data.u8[0];
                break;



            case ANDROID_NOISE_MODE:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_BYTE, 1))
                    break;
                dst->ctl.noise.mode = (enum processing_mode)curr_entry.data.u8[0];
                break;

            case ANDROID_NOISE_STRENGTH:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_BYTE, 1))
                    break;
                dst->ctl.noise.strength= curr_entry.data.u8[0];
                break;



            case ANDROID_SHADING_MODE:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_BYTE, 1))
                    break;
                dst->ctl.shading.mode = (enum processing_mode)curr_entry.data.u8[0];
                break;



            case ANDROID_GEOMETRIC_MODE:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_BYTE, 1))
                    break;
                dst->ctl.geometric.mode = (enum processing_mode)curr_entry.data.u8[0];
                break;



            case ANDROID_COLOR_MODE:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_BYTE, 1))
                    break;
                dst->ctl.color.mode = (enum colorcorrection_mode)curr_entry.data.u8[0];
                break;

            case ANDROID_COLOR_TRANSFORM:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_FLOAT, 9))
                    break;
                for (i=0 ; i<curr_entry.count ; i++)
                    dst->ctl.color.transform[i] = curr_entry.data.f[i];
                break;



            case ANDROID_TONEMAP_MODE:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_BYTE, 1))
                    break;
                dst->ctl.tonemap.mode = (enum tonemap_mode)curr_entry.data.u8[0];
                break;

            case ANDROID_TONEMAP_CURVE_RED:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_FLOAT, 32))
                    break;
                for (i=0 ; i<curr_entry.count ; i++)
                    dst->ctl.tonemap.curveRed[i] = curr_entry.data.f[i];
                break;

            case ANDROID_TONEMAP_CURVE_GREEN:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_FLOAT, 32))
                    break;
                for (i=0 ; i<curr_entry.count ; i++)
                    dst->ctl.tonemap.curveGreen[i] = curr_entry.data.f[i];
                break;

            case ANDROID_TONEMAP_CURVE_BLUE:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_FLOAT, 32))
                    break;
                for (i=0 ; i<curr_entry.count ; i++)
                    dst->ctl.tonemap.curveBlue[i] = curr_entry.data.f[i];
                break;



            case ANDROID_EDGE_MODE:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_BYTE, 1))
                    break;
                dst->ctl.edge.mode = (enum processing_mode)curr_entry.data.u8[0];
                break;

            case ANDROID_EDGE_STRENGTH:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_BYTE, 1))
                    break;
                dst->ctl.edge.strength = curr_entry.data.u8[0];
                break;



            case ANDROID_SCALER_CROP_REGION:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_INT32, 3))
                    break;
                for (i=0 ; i<curr_entry.count ; i++)
                    dst->ctl.scaler.cropRegion[i] = curr_entry.data.i32[i];
                break;



            case ANDROID_JPEG_QUALITY:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_INT32, 1))
                    break;
                dst->ctl.jpeg.quality= curr_entry.data.i32[0];
                break;

            case ANDROID_JPEG_THUMBNAIL_SIZE:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_INT32, 2))
                    break;
                for (i=0 ; i<curr_entry.count ; i++)
                    dst->ctl.jpeg.thumbnailSize[i] = curr_entry.data.i32[i];
                break;

            case ANDROID_JPEG_THUMBNAIL_QUALITY:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_INT32, 1))
                    break;
                dst->ctl.jpeg.thumbnailQuality= curr_entry.data.i32[0];
                break;

            case ANDROID_JPEG_GPS_COORDINATES:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_DOUBLE, 2)) // needs check
                    break;
                for (i=0 ; i<curr_entry.count ; i++)
                    dst->ctl.jpeg.gpsCoordinates[i] = curr_entry.data.d[i];
                break;

            case ANDROID_JPEG_GPS_PROCESSING_METHOD:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_BYTE, 32))
                    break;
                dst->ctl.jpeg.gpsProcessingMethod = curr_entry.data.u8[0];
                break;

            case ANDROID_JPEG_GPS_TIMESTAMP:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_INT64, 1))
                    break;
                dst->ctl.jpeg.gpsTimestamp = curr_entry.data.i64[0];
                break;

            case ANDROID_JPEG_ORIENTATION:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_INT32, 1))
                    break;
                dst->ctl.jpeg.orientation = curr_entry.data.i32[0];
                break;



            case ANDROID_STATS_FACE_DETECT_MODE:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_BYTE, 1))
                    break;
                dst->ctl.stats.faceDetectMode = (enum facedetect_mode)curr_entry.data.u8[0];
                break;

            case ANDROID_STATS_HISTOGRAM_MODE:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_BYTE, 1))
                    break;
                dst->ctl.stats.histogramMode = (enum stats_mode)curr_entry.data.u8[0];
                break;

            case ANDROID_STATS_SHARPNESS_MAP_MODE:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_BYTE, 1))
                    break;
                dst->ctl.stats.sharpnessMapMode = (enum stats_mode)curr_entry.data.u8[0];
                break;



            case ANDROID_CONTROL_CAPTURE_INTENT:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_BYTE, 1))
                    break;
                dst->ctl.aa.captureIntent = (enum aa_capture_intent)curr_entry.data.u8[0];
                break;

            case ANDROID_CONTROL_MODE:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_BYTE, 1))
                    break;
                dst->ctl.aa.mode = (enum aa_mode)curr_entry.data.u8[0];
                break;

            case ANDROID_CONTROL_EFFECT_MODE:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_BYTE, 1))
                    break;
                dst->ctl.aa.effectMode = (enum aa_effect_mode)curr_entry.data.u8[0];
                break;

            case ANDROID_CONTROL_SCENE_MODE:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_BYTE, 1))
                    break;
                dst->ctl.aa.sceneMode = (enum aa_scene_mode)curr_entry.data.u8[0];
                break;

            case ANDROID_CONTROL_VIDEO_STABILIZATION_MODE:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_BYTE, 1))
                    break;
                dst->ctl.aa.videoStabilizationMode = curr_entry.data.u8[0];
                break;

            case ANDROID_CONTROL_AE_MODE:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_BYTE, 1))
                    break;
                dst->ctl.aa.aeMode= (enum aa_aemode)curr_entry.data.u8[0];
                break;

            case ANDROID_CONTROL_AE_REGIONS:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_INT32, 5))
                    break;
                for (i=0 ; i<curr_entry.count ; i++)
                    dst->ctl.aa.aeRegions[i] = curr_entry.data.i32[i];
                break;

            case ANDROID_CONTROL_AE_EXP_COMPENSATION:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_INT32, 1))
                    break;
                dst->ctl.aa.aeExpCompensation = curr_entry.data.i32[0];
                break;

            case ANDROID_CONTROL_AE_TARGET_FPS_RANGE:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_INT32, 2))
                    break;
                for (i=0 ; i<curr_entry.count ; i++)
                    dst->ctl.aa.aeTargetFpsRange[i] = curr_entry.data.i32[i];
                break;

            case ANDROID_CONTROL_AE_ANTIBANDING_MODE:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_BYTE, 1))
                    break;
                dst->ctl.aa.aeAntibandingMode = (enum aa_ae_antibanding_mode)curr_entry.data.u8[0];
                break;

            case ANDROID_CONTROL_AWB_MODE:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_BYTE, 1))
                    break;
                dst->ctl.aa.awbMode = (enum aa_awbmode)(curr_entry.data.u8[0] + 1);
                break;

            case ANDROID_CONTROL_AWB_REGIONS:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_INT32, 5))
                    break;
                for (i=0 ; i<curr_entry.count ; i++)
                    dst->ctl.aa.awbRegions[i] = curr_entry.data.i32[i];
                break;

            case ANDROID_CONTROL_AF_MODE:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_BYTE, 1))
                    break;
                dst->ctl.aa.afMode = (enum aa_afmode)curr_entry.data.u8[0];
                break;

            case ANDROID_CONTROL_AF_REGIONS:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_INT32, 5))
                    break;
                for (i=0 ; i<curr_entry.count ; i++)
                    dst->ctl.aa.afRegions[i] = curr_entry.data.i32[i];
                break;



            case ANDROID_REQUEST_ID:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_INT32, 1))
                    break;
                dst->ctl.request.id = curr_entry.data.i32[0];
                ALOGV("DEBUG(%s): ANDROID_REQUEST_ID (%d)",  __FUNCTION__,  dst->ctl.request.id);
                break;

            case ANDROID_REQUEST_METADATA_MODE:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_BYTE, 1))
                    break;
                dst->ctl.request.metadataMode = (enum metadata_mode)curr_entry.data.u8[0];
                ALOGV("DEBUG(%s): ANDROID_REQUEST_METADATA_MODE (%d)",  __FUNCTION__, (int)( dst->ctl.request.metadataMode));
                break;

            case ANDROID_REQUEST_OUTPUT_STREAMS:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_BYTE))
                    break;
                for (i=0 ; i<curr_entry.count ; i++) {
                    dst->ctl.request.outputStreams[i] = curr_entry.data.u8[i];
                    ALOGV("DEBUG(%s): OUTPUT_STREAM[%d] = %d ",  __FUNCTION__, i, (int)(dst->ctl.request.outputStreams[i]));
                }
                dst->ctl.request.id = curr_entry.count; // temporary
                break;

            case ANDROID_REQUEST_FRAME_COUNT:
                if (NO_ERROR != CheckEntryTypeMismatch(&curr_entry, TYPE_INT32, 1))
                    break;
                dst->ctl.request.frameCount = curr_entry.data.i32[0];
                ALOGV("DEBUG(%s): ANDROID_REQUEST_FRAME_COUNT (%d)",  __FUNCTION__, dst->ctl.request.frameCount);
                break;

            default:
                ALOGD("DEBUG(%s):Bad Metadata tag (%d)",  __FUNCTION__, curr_entry.tag);
                break;
            }
        }
    }

    return NO_ERROR;
}




status_t MetadataConverter::ToDynamicMetadata(struct camera2_shot_ext * metadata_ext, camera_metadata_t * dst)
{
    status_t    res;
    struct camera2_shot * metadata = &(metadata_ext->shot);
    uint8_t  byteData;
    uint32_t intData;

    ALOGV("DEBUG(%s): TEMP version using original request METADATA", __FUNCTION__);
    if (0 != add_camera_metadata_entry(dst, ANDROID_REQUEST_ID,
                &(metadata->ctl.request.id), 1))
        return NO_MEMORY;

    if (0 != add_camera_metadata_entry(dst, ANDROID_REQUEST_METADATA_MODE,
                &(metadata->ctl.request.metadataMode), 1))
        return NO_MEMORY;

    // needs check!
    if (0 != add_camera_metadata_entry(dst, ANDROID_REQUEST_FRAME_COUNT,
                &(metadata->ctl.request.frameCount), 1))
        return NO_MEMORY;


    if (metadata->ctl.request.metadataMode == METADATA_MODE_NONE) {
        ALOGV("DEBUG(%s): METADATA_MODE_NONE", __FUNCTION__);
        return NO_ERROR;
    }

    ALOGV("DEBUG(%s): METADATA_MODE_FULL", __FUNCTION__);

    if (0 != add_camera_metadata_entry(dst, ANDROID_SENSOR_TIMESTAMP,
                &(metadata->dm.sensor.timeStamp), 1))
        return NO_MEMORY;
    ALOGV("DEBUG(%s): Timestamp: %lld", __FUNCTION__, metadata->dm.sensor.timeStamp);
    return NO_ERROR;


}

#if 0  // blocked for alpha version

status_t MetadataConverter::ToDynamicMetadata(camera2_ctl_metadata_t * metadata, camera_metadata_t * dst)
{
    status_t    res;

    if (0 != add_camera_metadata_entry(dst, ANDROID_REQUEST_ID,
                &(metadata->dm.request.id), 1))
        return NO_MEMORY;

    if (0 != add_camera_metadata_entry(dst, ANDROID_REQUEST_METADATA_MODE,
                &(metadata->dm.request.metadataMode), 1))
        return NO_MEMORY;

    // needs check!
    if (0 != add_camera_metadata_entry(dst, ANDROID_REQUEST_FRAME_COUNT,
                &(metadata->dm.request.frameCount), 1))
        return NO_MEMORY;


    if (metadata->dm.request.metadataMode == METADATA_MODE_NONE) {
        ALOGD("DEBUG(%s): METADATA_MODE_NONE", __func__);
        return NO_ERROR;
    }

     ALOGD("DEBUG(%s): METADATA_MODE_FULL", __func__);

    if (0 != add_camera_metadata_entry(dst, ANDROID_LENS_FOCUS_DISTANCE,
                &(metadata->dm.lens.focusDistance), 1))
        return NO_MEMORY;

    if (0 != add_camera_metadata_entry(dst, ANDROID_LENS_APERTURE,
                &(metadata->dm.lens.aperture), 1))
        return NO_MEMORY;

    if (0 != add_camera_metadata_entry(dst, ANDROID_LENS_FOCAL_LENGTH,
                &(metadata->dm.lens.focalLength), 1))
        return NO_MEMORY;

    if (0 != add_camera_metadata_entry(dst, ANDROID_LENS_FILTER_DENSITY,
                &(metadata->dm.lens.filterDensity), 1))
        return NO_MEMORY;

    if (0 != add_camera_metadata_entry(dst, ANDROID_LENS_OPTICAL_STABILIZATION_MODE,
                &(metadata->dm.lens.opticalStabilizationMode), 1))
        return NO_MEMORY;

    if (0 != add_camera_metadata_entry(dst, ANDROID_LENS_APERTURE,
                &(metadata->dm.lens.aperture), 1))
        return NO_MEMORY;

    if (0 != add_camera_metadata_entry(dst, ANDROID_LENS_FOCUS_RANGE,
                &(metadata->dm.lens.focusRange[0]), 2))
        return NO_MEMORY;



    if (0 != add_camera_metadata_entry(dst, ANDROID_SENSOR_EXPOSURE_TIME,
                &(metadata->dm.sensor.exposureTime), 1))
        return NO_MEMORY;

    if (0 != add_camera_metadata_entry(dst, ANDROID_SENSOR_FRAME_DURATION,
                &(metadata->dm.sensor.frameDuration), 1))
        return NO_MEMORY;

    if (0 != add_camera_metadata_entry(dst, ANDROID_SENSOR_SENSITIVITY,
                &(metadata->dm.sensor.sensitivity), 1))
        return NO_MEMORY;

    if (0 != add_camera_metadata_entry(dst, ANDROID_SENSOR_TIMESTAMP,
                &(metadata->dm.sensor.timeStamp), 1))
        return NO_MEMORY;



    if (0 != add_camera_metadata_entry(dst, ANDROID_FLASH_MODE,
                &(metadata->dm.flash.flashMode), 1))
        return NO_MEMORY;

    if (0 != add_camera_metadata_entry(dst, ANDROID_FLASH_FIRING_POWER,
                &(metadata->dm.flash.firingPower), 1))
        return NO_MEMORY;

    if (0 != add_camera_metadata_entry(dst, ANDROID_FLASH_FIRING_TIME,
                &(metadata->dm.flash.firingPower), 1))
        return NO_MEMORY;



    if (0 != add_camera_metadata_entry(dst, ANDROID_HOT_PIXEL_MODE,
                &(metadata->dm.hotpixel.mode), 1))
        return NO_MEMORY;



    if (0 != add_camera_metadata_entry(dst, ANDROID_DEMOSAIC_MODE,
                &(metadata->dm.demosaic.mode), 1))
        return NO_MEMORY;



    if (0 != add_camera_metadata_entry(dst, ANDROID_NOISE_MODE,
                &(metadata->dm.noise.mode), 1))
        return NO_MEMORY;

    if (0 != add_camera_metadata_entry(dst, ANDROID_NOISE_STRENGTH,
                &(metadata->dm.noise.strength), 1))
        return NO_MEMORY;


    if (0 != add_camera_metadata_entry(dst, ANDROID_SHADING_MODE,
                &(metadata->dm.shading.mode), 1))
        return NO_MEMORY;



    if (0 != add_camera_metadata_entry(dst, ANDROID_GEOMETRIC_MODE,
                &(metadata->dm.geometric.mode), 1))
        return NO_MEMORY;



    if (0 != add_camera_metadata_entry(dst, ANDROID_COLOR_MODE,
                &(metadata->dm.color.mode), 1))
        return NO_MEMORY;

    if (0 != add_camera_metadata_entry(dst, ANDROID_COLOR_TRANSFORM,
                &(metadata->dm.color.transform), 9))
        return NO_MEMORY;



    if (0 != add_camera_metadata_entry(dst, ANDROID_TONEMAP_MODE,
                &(metadata->dm.tonemap.mode), 1))
        return NO_MEMORY;

    if (0 != add_camera_metadata_entry(dst, ANDROID_TONEMAP_CURVE_RED,
                &(metadata->dm.tonemap.curveRed), 32))
        return NO_MEMORY;

    if (0 != add_camera_metadata_entry(dst, ANDROID_TONEMAP_CURVE_GREEN,
                &(metadata->dm.tonemap.curveGreen), 32))
        return NO_MEMORY;

    if (0 != add_camera_metadata_entry(dst, ANDROID_TONEMAP_CURVE_BLUE,
                &(metadata->dm.tonemap.curveBlue), 32))
        return NO_MEMORY;



    if (0 != add_camera_metadata_entry(dst, ANDROID_EDGE_MODE,
                &(metadata->dm.edge.mode), 1))
        return NO_MEMORY;

    if (0 != add_camera_metadata_entry(dst, ANDROID_EDGE_STRENGTH,
                &(metadata->dm.edge.strength), 1))
        return NO_MEMORY;



    if (0 != add_camera_metadata_entry(dst, ANDROID_SCALER_CROP_REGION,
                &(metadata->dm.scaler.cropRegion), 3))
        return NO_MEMORY;



    if (0 != add_camera_metadata_entry(dst, ANDROID_JPEG_QUALITY,
                &(metadata->dm.jpeg.quality), 1))
        return NO_MEMORY;

    if (0 != add_camera_metadata_entry(dst, ANDROID_JPEG_THUMBNAIL_SIZE,
                &(metadata->dm.jpeg.thumbnailSize), 2))
        return NO_MEMORY;

    if (0 != add_camera_metadata_entry(dst, ANDROID_JPEG_THUMBNAIL_QUALITY,
                &(metadata->dm.jpeg.thumbnailQuality), 1))
        return NO_MEMORY;

    if (0 != add_camera_metadata_entry(dst, ANDROID_JPEG_GPS_COORDINATES,
                &(metadata->dm.jpeg.gpsCoordinates), 2)) // needs check
        return NO_MEMORY;

    if (0 != add_camera_metadata_entry(dst, ANDROID_JPEG_GPS_PROCESSING_METHOD,
                &(metadata->dm.jpeg.gpsProcessingMethod), 32))
        return NO_MEMORY;

    if (0 != add_camera_metadata_entry(dst, ANDROID_JPEG_GPS_TIMESTAMP,
                &(metadata->dm.jpeg.gpsTimestamp), 1))
        return NO_MEMORY;

    if (0 != add_camera_metadata_entry(dst, ANDROID_JPEG_ORIENTATION,
                &(metadata->dm.jpeg.orientation), 1))
        return NO_MEMORY;



    if (0 != add_camera_metadata_entry(dst, ANDROID_STATS_FACE_DETECT_MODE,
                &(metadata->dm.stats.faceDetectMode), 1))
        return NO_MEMORY;

    // TODO : more stat entries



    if (0 != add_camera_metadata_entry(dst, ANDROID_CONTROL_MODE,
                &(metadata->dm.aa.mode), 1))
        return NO_MEMORY;

    if (0 != add_camera_metadata_entry(dst, ANDROID_CONTROL_CAPTURE_INTENT,
                &(metadata->dm.aa.captureIntent), 1))
        return NO_MEMORY;

    if (0 != add_camera_metadata_entry(dst, ANDROID_CONTROL_EFFECT_MODE,
                &(metadata->dm.aa.effect_mode), 1))
        return NO_MEMORY;

    if (0 != add_camera_metadata_entry(dst, ANDROID_CONTROL_AE_MODE,
                &(metadata->dm.aa.aeMode), 1))
        return NO_MEMORY;

    if (0 != add_camera_metadata_entry(dst, ANDROID_CONTROL_AE_REGIONS,
                &(metadata->dm.aa.aeRegions), 5))
        return NO_MEMORY;

    if (0 != add_camera_metadata_entry(dst, ANDROID_CONTROL_AE_EXP_COMPENSATION,
                &(metadata->dm.aa.aeExpCompensation), 1))
        return NO_MEMORY;

    if (0 != add_camera_metadata_entry(dst, ANDROID_CONTROL_AE_STATE,
                &(metadata->dm.aa.aeState), 1))
        return NO_MEMORY;

    if (0 != add_camera_metadata_entry(dst, ANDROID_CONTROL_AWB_MODE,
                &(metadata->dm.aa.awbMode), 1))
        return NO_MEMORY;

    if (0 != add_camera_metadata_entry(dst, ANDROID_CONTROL_AWB_REGIONS,
                &(metadata->dm.aa.awbRegions), 5))
        return NO_MEMORY;

    if (0 != add_camera_metadata_entry(dst, ANDROID_CONTROL_AWB_STATE,
                &(metadata->dm.aa.awbState), 1))
        return NO_MEMORY;

    if (0 != add_camera_metadata_entry(dst, ANDROID_CONTROL_AF_MODE,
                &(metadata->dm.aa.afMode), 1))
        return NO_MEMORY;

    if (0 != add_camera_metadata_entry(dst, ANDROID_CONTROL_AF_REGIONS,
                &(metadata->dm.aa.afRegions), 5))
        return NO_MEMORY;

    if (0 != add_camera_metadata_entry(dst, ANDROID_CONTROL_AF_STATE,
                &(metadata->dm.aa.afState), 1))
        return NO_MEMORY;

    if (0 != add_camera_metadata_entry(dst, ANDROID_CONTROL_VIDEO_STABILIZATION_MODE,
                &(metadata->dm.aa.videoStabilizationMode), 1))
        return NO_MEMORY;


    return NO_ERROR;

/*
typedef struct camera2_dm {
    camera2_request_dm_t            request;
    camera2_lens_dm_t               lens;
    camera2_sensor_dm_t             sensor;
    camera2_flash_dm_t              flash;
    camera2_hotpixel_dm_t           hotpixel;
    camera2_demosaic_dm_t           demosaic;
    camera2_noisereduction_dm_t     noise;
    camera2_shading_dm_t            shading;
    camera2_geometric_dm_t          geometric;
    camera2_colorcorrection_dm_t    color;
    camera2_tonemap_dm_t            tonemap;
    camera2_edge_dm_t               edge;
    camera2_scaler_dm_t             scaler;
    camera2_jpeg_dm_t               jpeg;
    camera2_stats_dm_t              stats;
    camera2_aa_dm_t                 aa;
} camera2_dm_t;
*/

}
#endif


}; // namespace android
