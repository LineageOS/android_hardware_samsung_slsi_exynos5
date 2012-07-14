/*
 * Samsung Exynos5 SoC series Camera API 2.0 HAL
 *
 * Internal Metadata (controls/dynamic metadata and static metadata)
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd
 * Contact: Sungjoong Kang, <sj3.kang@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/* 2012.04.18   Version 0.1  Initial Release */
/* 2012.04.23   Version 0.2  Added static metadata (draft) */
/* 2012.05.14   Version 0.3  Bug fixes and data type modification */
/* 2012.05.15   Version 0.4  Modified for Google's new camera_metadata.h  */


#ifndef CAMERA2_INTERNAL_METADATA_H_
#define CAMERA2_INTERNAL_METADATA_H_

//#include "camera_common.h"
#include <stdint.h>
#include <sys/cdefs.h>
#include <sys/types.h>


typedef struct rational_NEW {
    uint32_t num;
    uint32_t den;
} rational_NEW_t;


/*
 *   controls/dynamic metadata 
 */
 

/* android.request */

typedef enum metadata_mode_NEW
{
    METADATA_MODE_NONE_NEW,
    METADATA_MODE_FULL_NEW
} metadata_mode_NEW_t;

typedef struct camera2_request_ctl_NEW {
    uint32_t            id;
    metadata_mode_NEW_t     metadataMode;
    uint8_t             outputStreams[16];
    uint8_t             numOutputStream;
    uint32_t            frameCount;
} camera2_request_ctl_NEW_t;

typedef struct camera2_request_dm_NEW {
    uint32_t            id;
    metadata_mode_NEW_t     metadataMode;
    uint32_t            frameCount; 
} camera2_request_dm_NEW_t;



/* android.lens */

typedef enum optical_stabilization_mode_NEW
{
    OPTICAL_STABILIZATION_MODE_OFF_NEW,
    OPTICAL_STABILIZATION_MODE_ON_NEW
} optical_stabilization_mode_NEW_t;
    
typedef struct camera2_lens_ctl_NEW {
    float                           focusDistance;
    float                           aperture;
    float                           focalLength;
    float                           filterDensity;
    optical_stabilization_mode_NEW_t    opticalStabilizationMode;
} camera2_lens_ctl_NEW_t;

typedef struct camera2_lens_dm_NEW {
    float                           focusDistance;
    float                           aperture;
    float                           focalLength;
    float                           filterDensity;
    optical_stabilization_mode_NEW_t    opticalStabilizationMode;
    float                           focusRange[2];
} camera2_lens_dm_NEW_t;



/* android.sensor */

typedef struct camera2_sensor_ctl_NEW {
    uint64_t    exposureTime;
    uint64_t    frameDuration;
    uint32_t    sensitivity;
} camera2_sensor_ctl_NEW_t;

typedef struct camera2_sensor_dm_NEW {
    uint64_t    exposureTime;
    uint64_t    frameDuration;
    uint32_t    sensitivity;
    uint64_t    timeStamp;
    uint32_t    frameCount;
} camera2_sensor_dm_NEW_t;



/* android.flash */

typedef enum flash_mode_NEW
{
    FLASH_MODE_OFF_NEW,
    FLASH_MODE_SINGLE_NEW,
    FLASH_MODE_AUTO_SINGLE_NEW,
    FLASH_MODE_TORCH_NEW
} flash_mode_NEW_t;
    
typedef struct camera2_flash_ctl_NEW {
    flash_mode_NEW_t    flashMode;
    uint8_t         firingPower;
    uint64_t        firingTime;
} camera2_flash_ctl_NEW_t;

typedef struct camera2_flash_dm_NEW {
    flash_mode_NEW_t    flashMode;
    uint8_t         firingPower;
    uint64_t        firingTime;
} camera2_flash_dm_NEW_t;



/* android.flash */

typedef enum hotpixel_mode_NEW
{
    HOTPIXEL_MODE_OFF_NEW,
    HOTPIXEL_MODE_FAST_NEW,
    HOTPIXEL_MODE_HIGH_QUALITY_NEW
} hotpixel_mode_NEW_t;


typedef struct camera2_hotpixel_ctl_NEW {
    hotpixel_mode_NEW_t mode;
} camera2_hotpixel_ctl_NEW_t;

typedef struct camera2_hotpixel_dm_NEW {
    hotpixel_mode_NEW_t mode;
} camera2_hotpixel_dm_NEW_t;



/* android.demosaic */

typedef enum demosaic_mode_NEW
{
    DEMOSAIC_MODE_FAST_NEW = 1,
    DEMOSAIC_MODE_HIGH_QUALITY_NEW
} demosaic_mode_NEW_t;

typedef struct camera2_demosaic_ctl_NEW {
    demosaic_mode_NEW_t mode;
} camera2_demosaic_ctl_NEW_t;

typedef struct camera2_demosaic_dm_NEW {
    demosaic_mode_NEW_t mode;
} camera2_demosaic_dm_NEW_t;



/* android.noiseReduction */

typedef enum noise_mode_NEW
{
    NOISEREDUCTION_MODE_OFF_NEW,
    NOISEREDUCTION_MODE_FAST_NEW,
    NOISEREDUCTION_MODE_HIGH_QUALITY_NEW
} noise_mode_NEW_t;

typedef struct camera2_noisereduction_ctl_NEW {
    noise_mode_NEW_t    mode;
    uint8_t         strength;
} camera2_noisereduction_ctl_NEW_t;

typedef struct camera2_noisereduction_dm_NEW {
    noise_mode_NEW_t    mode;
    uint8_t         strength;
} camera2_noisereduction_dm_NEW_t;



/* android.shading */

typedef enum shading_mode_NEW
{
    SHADING_MODE_OFF_NEW,
    SHADING_MODE_FAST_NEW,
    SHADING_MODE_HIGH_QUALITY_NEW
} shading_mode_NEW_t;

typedef struct camera2_shading_ctl_NEW {
    shading_mode_NEW_t  mode;
} camera2_shading_ctl_NEW_t;

typedef struct camera2_shading_dm_NEW {
    shading_mode_NEW_t  mode;
} camera2_shading_dm_NEW_t;



/* android.geometric */

typedef enum geometric_mode_NEW
{
    GEOMETRIC_MODE_OFF_NEW,
    GEOMETRIC_MODE_FAST_NEW,
    GEOMETRIC_MODE_HIGH_QUALITY_NEW
} geometric_mode_NEW_t;

typedef struct camera2_geometric_ctl_NEW {
    geometric_mode_NEW_t    mode;
} camera2_geometric_ctl_NEW_t;

typedef struct camera2_geometric_dm_NEW {
    geometric_mode_NEW_t    mode;
} camera2_geometric_dm_NEW_t;



/* android.colorCorrection */

typedef enum colorcorrection_mode_NEW
{
    COLORCORRECTION_MODE_FAST_NEW = 1,
    COLORCORRECTION_MODE_HIGH_QUALITY_NEW, 
    COLORCORRECTION_MODE_TRANSFORM_MATRIX_NEW    
} colorcorrection_mode_NEW_t;


typedef struct camera2_colorcorrection_ctl_NEW {
    colorcorrection_mode_NEW_t  mode;
    float                   transform[9];
} camera2_colorcorrection_ctl_NEW_t;

typedef struct camera2_colorcorrection_dm_NEW {
    colorcorrection_mode_NEW_t  mode;
    float                   transform[9];
} camera2_colorcorrection_dm_NEW_t;



/* android.tonemap */

typedef enum tonemap_mode_NEW
{
    TONEMAP_MODE_FAST_NEW = 1,
    TONEMAP_MODE_HIGH_QUALITY_NEW,
    TONEMAP_MODE_CONTRAST_CURVE_NEW,    
} tonemap_mode_NEW_t;

typedef struct camera2_tonemap_ctl_NEW {
    tonemap_mode_NEW_t  mode;
    float           curveRed[32]; // assuming maxCurvePoints = 32
    float           curveGreen[32];
    float           curveBlue[32];
} camera2_tonemap_ctl_NEW_t;

typedef struct camera2_tonemap_dm_NEW {
    tonemap_mode_NEW_t  mode;
    float           curveRed[32]; // assuming maxCurvePoints = 32
    float           curveGreen[32];
    float           curveBlue[32];
} camera2_tonemap_dm_NEW_t;



/* android.edge */

typedef enum edge_mode_NEW
{
    EDGE_MODE_OFF_NEW,
    EDGE_MODE_FAST_NEW,
    EDGE_MODE_HIGH_QUALITY_NEW
} edge_mode_NEW_t;

typedef struct camera2_edge_ctl_NEW {
    edge_mode_NEW_t     mode;
    uint8_t         strength;
} camera2_edge_ctl_NEW_t;

typedef struct camera2_edge_dm_NEW {
    edge_mode_NEW_t     mode;
    uint8_t         strength;
} camera2_edge_dm_NEW_t;



/* android.scaler */

typedef struct camera2_scaler_ctl_NEW {
    uint32_t    cropRegion[3];
	//uint32_t    rotation;
} camera2_scaler_ctl_NEW_t;

typedef struct camera2_scaler_dm_NEW {
    //uint32_t    size[2];
    //uint8_t     format;
    uint32_t    cropRegion[3];
    //uint32_t    rotation;
} camera2_scaler_dm_NEW_t;



/* android.jpeg */

typedef struct camera2_jpeg_ctl_NEW {
    uint32_t     quality;
    uint32_t    thumbnailSize[2];
    uint32_t     thumbnailQuality;
    double      gpsCoordinates[2]; // needs check
    uint8_t     gpsProcessingMethod[32];
    uint64_t    gpsTimestamp;
    uint32_t    orientation;
} camera2_jpeg_ctl_NEW_t;

typedef struct camera2_jpeg_dm_NEW {
    uint8_t     quality;
    uint32_t    thumbnailSize[2];
    uint8_t     thumbnailQuality;
    double      gpsCoordinates[3];
    uint8_t     gpsProcessingMethod;
    uint64_t    gpsTimestamp;
    uint32_t    orientation;
} camera2_jpeg_dm_NEW_t;



/* android.statistics */

typedef enum facedetect_mode_NEW
{
    FACEDETECT_MODE_OFF_NEW,
    FACEDETECT_MODE_SIMPLE_NEW,
    FACEDETECT_MODE_FULL_NEW
} facedetect_mode_NEW_t;

typedef enum histogram_mode_NEW
{
    HISTOGRAM_MODE_OFF_NEW,
    HISTOGRAM_MODE_ON_NEW
} histogram_mode_NEW_t;

typedef enum sharpnessmap_mode_NEW
{
    SHARPNESSMAP_MODE_OFF_NEW,
    SHARPNESSMAP_MODE_ON_NEW
} sharpnessmap_mode_NEW_t;

typedef struct camera2_stats_ctl_NEW {
    facedetect_mode_NEW_t   faceDetectMode;
    histogram_mode_NEW_t    histogramMode;
    sharpnessmap_mode_NEW_t sharpnessMapMode;
} camera2_stats_ctl_NEW_t;

/* REMARKS : FD results are not included */
typedef struct camera2_stats_dm_NEW {
    facedetect_mode_NEW_t   faceDetectMode;
    // faceRetangles
    // faceScores
    // faceLandmarks
    // faceIds
    histogram_mode_NEW_t    histogramMode;
    // histogram
    sharpnessmap_mode_NEW_t sharpnessMapMode;
    // sharpnessMap
} camera2_stats_dm_NEW_t;



/* android.control */

typedef enum aa_captureintent_NEW
{
	AA_CAPTURE_INTENT_CUSTOM_NEW, 
    AA_CAPTURE_INTENT_PREVIEW_NEW,
    AA_CAPTURE_INTENT_STILL_CAPTURE_NEW,
    AA_CAPTURE_INTENT_VIDEO_RECORD_NEW,
    AA_CAPTURE_INTENT_VIDEO_SNAPSHOT_NEW,
    AA_CAPTURE_INTENT_ZERO_SHUTTER_LAG_NEW
} aa_captureintent_NEW_t;

typedef enum aa_mode_NEW
{
    AA_MODE_OFF_NEW,
    AA_MODE_AUTO_NEW,
    AA_MODE_USE_SCENE_MODE_NEW
} aa_mode_NEW_t;

typedef enum aa_scene_mode_NEW
{
    AA_SCENE_MODE_FACE_PRIORITY_NEW,
    AA_SCENE_MODE_ACTION_NEW,
    AA_SCENE_MODE_PORTRAIT_NEW,
    AA_SCENE_MODE_LANDSCAPE_NEW,
    AA_SCENE_MODE_NIGHT_NEW,
    AA_SCENE_MODE_NIGHT_PORTRAIT_NEW,
    AA_SCENE_MODE_THEATRE_NEW,
    AA_SCENE_MODE_BEACH_NEW,
    AA_SCENE_MODE_SNOW_NEW,
    AA_SCENE_MODE_SUNSET_NEW,
    AA_SCENE_MODE_STEADYPHOTO_NEW,
    AA_SCENE_MODE_FIREWORKS_NEW,
    AA_SCENE_MODE_SPORTS_NEW,
    AA_SCENE_MODE_PARTY_NEW,
    AA_SCENE_MODE_CANDLELIGHT_NEW,
    AA_SCENE_MODE_BARCODE_NEW
} aa_scene_mode_NEW_t;

typedef enum aa_video_stab_mode_NEW
{
    AA_VIDEO_STABILIZATION_OFF_NEW,
    AA_VIDEO_STABILIZATION_ON_NEW
} aa_video_stab_mode_NEW_t;

typedef enum aa_effect_mode_NEW
{
    AA_EFFECT_MODE_OFF_NEW,
    AA_EFFECT_MODE_MONO_NEW, 
    AA_EFFECT_MODE_NEGATIVE_NEW,
    AA_EFFECT_MODE_SOLARIZE_NEW,
    AA_EFFECT_MODE_SEPIA_NEW,
    AA_EFFECT_MODE_POSTERIZE_NEW,
    AA_EFFECT_MODE_WHITEBOARD_NEW,
    AA_EFFECT_MODE_BLACKBOARD_NEW,
    AA_EFFECT_MODE_AQUA
} aa_effect_mode_NEW_t;

typedef enum aa_aemode_NEW
{
    AA_AEMODE_OFF_NEW,
    AA_AEMODE_ON_NEW,
    AA_AEMODE_ON_AUTO_FLASH_NEW,
    AA_AEMODE_ON_ALWAYS_FLASH_NEW,
    AA_AEMODE_ON_AUTO_FLASH_REDEYE_NEW
} aa_aemode_NEW_t;

typedef enum aa_ae_antibanding_mode_NEW
{
    AA_AE_ANTIBANDING_OFF_NEW,
    AA_AE_ANTIBANDING_50HZ_NEW,
    AA_AE_ANTIBANDING_60HZ_NEW,
    AA_AE_ANTIBANDING_AUTO_NEW
} aa_ae_antibanding_mode_NEW_t;

typedef enum aa_awbmode_NEW
{
    AA_AWBMODE_OFF_NEW,
    AA_AWBMODE_WB_AUTO_NEW,
    AA_AWBMODE_WB_INCANDESCENT_NEW,
    AA_AWBMODE_WB_FLUORESCENT_NEW,
    AA_AWBMODE_WB_WARM_FLUORESCENT_NEW,
    AA_AWBMODE_WB_DAYLIGHT_NEW,
    AA_AWBMODE_WB_CLOUDY_DAYLIGHT_NEW,
    AA_AWBMODE_WB_TWILIGHT_NEW,
    AA_AWBMODE_WB_SHADE_NEW
} aa_awbmode_NEW_t;

typedef enum aa_afmode_NEW
{
    AA_AFMODE_OFF_NEW,
    AA_AFMODE_FOCUS_MODE_AUTO_NEW,
    AA_AFMODE_FOCUS_MODE_MACRO_NEW,
    AA_AFMODE_FOCUS_MODE_CONTINUOUS_VIDEO_NEW,
    AA_AFMODE_FOCUS_MODE_CONTINUOUS_PICTURE_NEW
} aa_afmode_NEW_t;

typedef enum aa_afstate_NEW
{
    AA_AFSTATE_INACTIVE_NEW,
    AA_AFSTATE_PASSIVE_SCAN_NEW,
    AA_AFSTATE_ACTIVE_SCAN_NEW,
    AA_AFSTATE_AF_ACQUIRED_FOCUS_NEW,
    AA_AFSTATE_AF_FAILED_FOCUS_NEW
} aa_afstate_NEW_t;

typedef struct camera2_aa_ctl_NEW {
	aa_captureintent_NEW_t          captureIntent; 
    aa_mode_NEW_t                   mode;
    aa_effect_mode_NEW_t            effect_mode; 
    aa_scene_mode_NEW_t				scene_mode;
	aa_video_stab_mode_NEW_t        videoStabilizationMode;
    aa_aemode_NEW_t                 aeMode;
    uint32_t                    aeRegions[5]; // 5 per region(x1,y1,x2,y2,weight). currently assuming 1 region.
    int32_t                     aeExpCompensation;
    uint32_t                    aeTargetFpsRange[2];
    aa_ae_antibanding_mode_NEW_t    aeAntibandingMode;
    uint8_t                     aeState; // NEEDS_VERIFY after official release
    aa_awbmode_NEW_t                awbMode;
    uint32_t                    awbRegions[5]; // 5 per region(x1,y1,x2,y2,weight). currently assuming 1 region.
    uint8_t                     awbState; // NEEDS_VERIFY after official release
    aa_afmode_NEW_t                 afMode;
    uint32_t                    afRegions[5]; // 5 per region(x1,y1,x2,y2,weight). currently assuming 1 region.
    
} camera2_aa_ctl_NEW_t;

typedef struct camera2_aa_dm_NEW {
	aa_captureintent_NEW_t          captureIntent;
    aa_mode_NEW_t                   mode;
    aa_effect_mode_NEW_t            effect_mode; 
    aa_scene_mode_NEW_t				scene_mode;    
    aa_video_stab_mode_NEW_t        videoStabilizationMode;	
    aa_aemode_NEW_t                 aeMode; // needs check
    uint32_t                    aeRegions[5]; // 5 per region(x1,y1,x2,y2,weight). currently assuming 1 region.
    int32_t                     aeExpCompensation; // needs check
    uint8_t                     aeState; // NEEDS_VERIFY after official release    
    aa_awbmode_NEW_t                awbMode;
    uint32_t                    awbRegions[5]; // 5 per region(x1,y1,x2,y2,weight). currently assuming 1 region.
    uint8_t                     awbState; // NEEDS_VERIFY after official release    
    aa_afmode_NEW_t                 afMode;
    uint32_t                    afRegions[5]; // 5 per region(x1,y1,x2,y2,weight). currently assuming 1 region.
    aa_afstate_NEW_t                  afState;
} camera2_aa_dm_NEW_t;



    
// sizeof(camera2_ctl) = ?
typedef struct camera2_ctl_NEW {
    camera2_request_ctl_NEW_t           request;
    camera2_lens_ctl_NEW_t              lens;
    camera2_sensor_ctl_NEW_t            sensor;
    camera2_flash_ctl_NEW_t             flash;
    camera2_hotpixel_ctl_NEW_t          hotpixel;
    camera2_demosaic_ctl_NEW_t          demosaic;
    camera2_noisereduction_ctl_NEW_t    noise;
    camera2_shading_ctl_NEW_t           shading;
    camera2_geometric_ctl_NEW_t         geometric;
    camera2_colorcorrection_ctl_NEW_t   color;
    camera2_tonemap_ctl_NEW_t           tonemap;
    camera2_edge_ctl_NEW_t              edge;
    camera2_scaler_ctl_NEW_t            scaler;
    camera2_jpeg_ctl_NEW_t              jpeg;
    camera2_stats_ctl_NEW_t             stats;
    camera2_aa_ctl_NEW_t                aa;
} camera2_ctl_NEW_t;

// sizeof(camera2_dm) = ?
typedef struct camera2_dm_NEW {
    camera2_request_dm_NEW_t            request;
    camera2_lens_dm_NEW_t               lens;
    camera2_sensor_dm_NEW_t             sensor;
    camera2_flash_dm_NEW_t              flash;
    camera2_hotpixel_dm_NEW_t           hotpixel;
    camera2_demosaic_dm_NEW_t           demosaic;
    camera2_noisereduction_dm_NEW_t     noise;
    camera2_shading_dm_NEW_t            shading;
    camera2_geometric_dm_NEW_t          geometric;
    camera2_colorcorrection_dm_NEW_t    color;
    camera2_tonemap_dm_NEW_t            tonemap;
    camera2_edge_dm_NEW_t               edge;
    camera2_scaler_dm_NEW_t             scaler;
    camera2_jpeg_dm_NEW_t               jpeg;
    camera2_stats_dm_NEW_t              stats;
    camera2_aa_dm_NEW_t                 aa;
} camera2_dm_NEW_t;




typedef struct camera2_ctl_metadata_NEW {
    camera2_ctl_NEW_t   ctl;
    camera2_dm_NEW_t    dm;
} camera2_ctl_metadata_NEW_t;
        



/*
 *   static metadata 
 */


/* android.lens */

typedef enum lens_facing_NEW
{
    LENS_FACING_FRONT_NEW,
    LENS_FACING_BACK_NEW
} lens_facing_NEW_t;

typedef struct camera2_lens_sm_NEW {
    float   minimumFocusDistance;
    float   availableFocalLength[2];
    float   availableApertures;  // assuming 1 aperture
    float   availableFilterDensities; // assuming 1 ND filter value
    uint8_t availableOpticalStabilization; // assuming 1
    float   shadingMap[3][40][30];
    float   geometricCorrectionMap[2][3][40][30];
    lens_facing_NEW_t facing;
    float   position[2];
} camera2_lens_sm_NEW_t;



/* android.sensor */

typedef enum sensor_colorfilterarrangement_NEW
{
    SENSOR_COLORFILTERARRANGEMENT_RGGB_NEW,
    SENSOR_COLORFILTERARRANGEMENT_GRBG_NEW,
    SENSOR_COLORFILTERARRANGEMENT_GBRG_NEW,
    SENSOR_COLORFILTERARRANGEMENT_BGGR_NEW,
    SENSOR_COLORFILTERARRANGEMENT_RGB_NEW   
} sensor_colorfilterarrangement_NEW_t;

typedef enum sensor_ref_illuminant_NEW
{
    SENSOR_ILLUMINANT_DAYLIGHT_NEW = 1,
    SENSOR_ILLUMINANT_FLUORESCENT_NEW = 2,
    SENSOR_ILLUMINANT_TUNGSTEN_NEW = 3,
    SENSOR_ILLUMINANT_FLASH_NEW = 4,
    SENSOR_ILLUMINANT_FINE_WEATHER_NEW = 9,
    SENSOR_ILLUMINANT_CLOUDY_WEATHER_NEW = 10,
    SENSOR_ILLUMINANT_SHADE_NEW = 11,
    SENSOR_ILLUMINANT_DAYLIGHT_FLUORESCENT_NEW = 12,
    SENSOR_ILLUMINANT_DAY_WHITE_FLUORESCENT_NEW = 13,
    SENSOR_ILLUMINANT_COOL_WHITE_FLUORESCENT_NEW = 14,
    SENSOR_ILLUMINANT_WHITE_FLUORESCENT_NEW = 15,
    SENSOR_ILLUMINANT_STANDARD_A_NEW = 17,
    SENSOR_ILLUMINANT_STANDARD_B_NEW = 18,
    SENSOR_ILLUMINANT_STANDARD_C_NEW = 19,
    SENSOR_ILLUMINANT_D55_NEW = 20,
    SENSOR_ILLUMINANT_D65_NEW = 21,
    SENSOR_ILLUMINANT_D75_NEW = 22,
    SENSOR_ILLUMINANT_D50_NEW = 23,
    SENSOR_ILLUMINANT_ISO_STUDIO_TUNGSTEN_NEW = 24
} sensor_ref_illuminant_NEW_t;

typedef struct camera2_sensor_sm_NEW {
    uint32_t    exposureTimeRange[2];
    uint32_t    maxFrameDuration;
    uint32_t    sensitivityRange[2];
    sensor_colorfilterarrangement_NEW_t colorFilterArrangement;
    uint32_t    pixelArraySize[2];
    uint32_t    activeArraySize[4];
    uint32_t    whiteLevel;
    uint32_t    blackLevelPattern[4];
    rational_NEW_t  colorTransform1[9];
    rational_NEW_t  colorTransform2[9];
    sensor_ref_illuminant_NEW_t   referenceIlluminant1;
    sensor_ref_illuminant_NEW_t   referenceIlluminant2;
    rational_NEW_t  forwardMatrix1[9];
    rational_NEW_t  forwardMatrix2[9];
    rational_NEW_t  calibrationTransform1[9];   
    rational_NEW_t  calibrationTransform2[9];
    rational_NEW_t  baseGainFactor;
    uint32_t    maxAnalogSensitivity;
    float   noiseModelCoefficients[2];
    uint32_t    orientation;
} camera2_sensor_sm_NEW_t;



/* android.flash */

typedef struct camera2_flash_sm_NEW {
    uint8_t     available;
    uint64_t    chargeDuration;
} camera2_flash_sm_NEW_t;



/* android.colorCorrection */

typedef struct camera2_colorcorrection_sm_NEW {
    colorcorrection_mode_NEW_t    availableModes[10]; // assuming 10 supported modes
} camera2_colorcorrection_sm_NEW_t;



/* android.tonemap */

typedef struct camera2_tonemap_sm_NEW {
    uint32_t    maxCurvePoints;
} camera2_tonemap_sm_NEW_t;



/* android.scaler */

typedef enum scaler_availableformats_NEW {
    SCALER_FORMAT_BAYER_RAW_NEW,
    SCALER_FORMAT_YV12_NEW,
    SCALER_FORMAT_NV21_NEW,
    SCALER_FORMAT_JPEG_NEW,
    SCALER_FORMAT_UNKNOWN_NEW
} scaler_availableformats_NEW_t;

typedef struct camera2_scaler_sm_NEW {
    scaler_availableformats_NEW_t availableFormats[4]; // assuming 
                                                   // # of availableFormats = 4
    uint32_t    availableSizesPerFormat[4];  
    uint32_t    availableSizes[4][8][2]; // assuning availableSizesPerFormat=8
    uint64_t    availableMinFrameDurations[4][8];
    float       maxDigitalZoom;
} camera2_scaler_sm_NEW_t;



/* android.jpeg */

typedef struct camera2_jpeg_sm_NEW {
    uint32_t    availableThumbnailSizes[2][8]; // assuming supported size=8
} camera2_jpeg_sm_NEW_t;



/* android.statistics */

typedef struct camera2_statistics_sm_NEW {
    uint8_t availableFaceDetectModes[3]; // assuming supported modes = 3;
    uint32_t    maxFaceCount;
    uint32_t    histogramBucketCount;
    uint32_t    maxHistogramCount;
    uint32_t    sharpnessMapSize[2];
    uint32_t    maxSharpnessMapValue;
} camera2_statistics_sm_NEW_t;



/* android.control */

typedef struct camera2_aa_sm_NEW {
    uint8_t availableModes[10]; // assuming # of available scene modes = 10
    uint32_t    maxRegions;
    uint8_t aeAvailableModes[8]; // assuming # of available ae modes = 8
    rational_NEW_t  aeCompensationStep;
    int32_t aeCompensationRange[2];
    uint32_t    aeAvailableTargetFpsRanges[2][8];
    uint8_t aeAvailableAntibandingModes[4];
    uint8_t awbAvailableModes[10]; // assuming # of awbAvailableModes = 10
    uint8_t afAvailableModes[4]; // assuming # of afAvailableModes = 4
} camera2_aa_sm_NEW_t;





typedef struct camera2_static_metadata_NEW {
    camera2_lens_sm_NEW_t   lens;
    camera2_sensor_sm_NEW_t sensor;
    camera2_flash_sm_NEW_t  flash;
    camera2_colorcorrection_sm_NEW_t color;
    camera2_tonemap_sm_NEW_t tonemap;
    camera2_scaler_sm_NEW_t scaler;
    camera2_jpeg_sm_NEW_t   jpeg;
    camera2_statistics_sm_NEW_t statistics;
    camera2_aa_sm_NEW_t aa;
} camera2_static_metadata_NEW_t;


#endif

