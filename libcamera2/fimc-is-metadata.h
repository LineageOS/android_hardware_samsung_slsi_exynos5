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

/* 2012.04.18 	Version 0.1  Initial Release */
/* 2012.04.23 	Version 0.2  Added static metadata (draft) */


#ifndef FIMC_IS_METADATA_H_
#define FIMC_IS_METADATA_H_

//#include "camera_common.h"
#include <stdint.h>
#include <sys/cdefs.h>
#include <sys/types.h>


typedef struct rational {
	uint32_t num;
	uint32_t den;
} rational_t;


/*
 *   controls/dynamic metadata 
 */
 

/* android.request */

typedef enum metadata_mode
{
	METADATA_MODE_NONE,
	METADATA_MODE_FULL
} metadata_mode_t;

typedef struct camera2_request_ctl {
	uint32_t			id;
	metadata_mode_t		metadataMode;
	uint8_t				outputStreams[16];
} camera2_request_ctl_t;

typedef struct camera2_request_dm {
	uint32_t			id;
	metadata_mode_t		metadataMode;
	uint32_t			frameCount; 
} camera2_request_dm_t;



/* android.lens */

typedef enum optical_stabilization_mode
{
	OPTICAL_STABILIZATION_MODE_OFF,
	OPTICAL_STABILIZATION_MODE_ON
} optical_stabilization_mode_t;
	
typedef struct camera2_lens_ctl {
	float							focusDistance;
	float							aperture;
	float							focalLength;
	float							filterDensity;
	optical_stabilization_mode_t	opticalStabilizationMode;
} camera2_lens_ctl_t;

typedef struct camera2_lens_dm {
	float							focusDistance;
	float							aperture;
	float							focalLength;
	float							filterDensity;
	optical_stabilization_mode_t	opticalStabilizationMode;
	float							focusRange[2];
} camera2_lens_dm_t;



/* android.sensor */

typedef struct camera2_sensor_ctl {
	uint64_t	exposureTime;
	uint64_t	frameDuration;
	uint32_t	sensitivity;
} camera2_sensor_ctl_t;

typedef struct camera2_sensor_dm {
	uint64_t	exposureTime;
	uint64_t	frameDuration;
	uint32_t	sensitivity;
	uint64_t	timeStamp;
	uint32_t	frameCount;
} camera2_sensor_dm_t;



/* android.flash */

typedef enum flash_mode
{
	CAM2_FLASH_MODE_OFF,
	CAM2_FLASH_MODE_SINGLE,
	CAM2_FLASH_MODE_TORCH
} flash_mode_t;

typedef struct camera2_flash_ctl {
	flash_mode_t	flashMode;
	uint8_t			firingPower;
	uint64_t		firingTime;
} camera2_flash_ctl_t;

typedef struct camera2_flash_dm {
	flash_mode_t	flashMode;
	uint8_t			firingPower;
	uint64_t		firingTime;
} camera2_flash_dm_t;



/* android.flash */

typedef enum hotpixel_mode
{
	HOTPIXEL_MODE_OFF,
	HOTPIXEL_MODE_FAST,
	HOTPIXEL_MODE_HIGH_QUALITY
} hotpixel_mode_t;


typedef struct camera2_hotpixel_ctl {
	hotpixel_mode_t	mode;
} camera2_hotpixel_ctl_t;

typedef struct camera2_hotpixel_dm {
	hotpixel_mode_t	mode;
} camera2_hotpixel_dm_t;



/* android.demosaic */

typedef enum demosaic_mode
{
	DEMOSAIC_MODE_OFF,
	DEMOSAIC_MODE_FAST,
	DEMOSAIC_MODE_HIGH_QUALITY
} demosaic_mode_t;

typedef struct camera2_demosaic_ctl {
	demosaic_mode_t	mode;
} camera2_demosaic_ctl_t;

typedef struct camera2_demosaic_dm {
	demosaic_mode_t	mode;
} camera2_demosaic_dm_t;



/* android.noiseReduction */

typedef enum noise_mode
{
	NOISEREDUCTION_MODE_OFF,
	NOISEREDUCTION_MODE_FAST,
	NOISEREDUCTION_MODE_HIGH_QUALITY
} noise_mode_t;

typedef struct camera2_noisereduction_ctl {
	noise_mode_t	mode;
	uint8_t			strength;
} camera2_noisereduction_ctl_t;

typedef struct camera2_noisereduction_dm {
	noise_mode_t	mode;
	uint8_t			strength;
} camera2_noisereduction_dm_t;



/* android.shading */

typedef enum shading_mode
{
	SHADING_MODE_OFF,
	SHADING_MODE_FAST,
	SHADING_MODE_HIGH_QUALITY
} shading_mode_t;

typedef struct camera2_shading_ctl {
	shading_mode_t	mode;
} camera2_shading_ctl_t;

typedef struct camera2_shading_dm {
	shading_mode_t	mode;
} camera2_shading_dm_t;



/* android.geometric */

typedef enum geometric_mode
{
	GEOMETRIC_MODE_OFF,
	GEOMETRIC_MODE_FAST,
	GEOMETRIC_MODE_HIGH_QUALITY
} geometric_mode_t;

typedef struct camera2_geometric_ctl {
	geometric_mode_t	mode;
} camera2_geometric_ctl_t;

typedef struct camera2_geometric_dm {
	geometric_mode_t	mode;
} camera2_geometric_dm_t;



/* android.colorCorrection */

typedef enum colorcorrection_mode
{
	COLORCORRECTION_MODE_TRANSFORM_MATRIX,
	COLORCORRECTION_MODE_FAST,
	COLORCORRECTION_MODE_HIGH_QUALITY, 
	COLORCORRECTION_MODE_EFFECT_MONO, 
	COLORCORRECTION_MODE_EFFECT_NEGATIVE,
	COLORCORRECTION_MODE_EFFECT_SOLARIZE,
	COLORCORRECTION_MODE_EFFECT_SEPIA,
	COLORCORRECTION_MODE_EFFECT_POSTERIZE,
	COLORCORRECTION_MODE_EFFECT_WHITEBOARD,
	COLORCORRECTION_MODE_EFFECT_BLACKBOARD,
	COLORCORRECTION_MODE_EFFECT_AQUA
} colorcorrection_mode_t;


typedef struct camera2_colorcorrection_ctl {
	colorcorrection_mode_t	mode;
	float					transform[9];
} camera2_colorcorrection_ctl_t;

typedef struct camera2_colorcorrection_dm {
	colorcorrection_mode_t	mode;
	float					transform[9];
} camera2_colorcorrection_dm_t;



/* android.tonemap */

typedef enum tonemap_mode
{
	TONEMAP_MODE_CONTRAST_CURVE,
	TONEMAP_MODE_FAST,
	TONEMAP_MODE_HIGH_QUALITY
} tonemap_mode_t;

typedef struct camera2_tonemap_ctl {
	tonemap_mode_t	mode;
	float			curveRed[32]; // assuming maxCurvePoints = 32
	float			curveGreen[32];
	float			curveBlue[32];
} camera2_tonemap_ctl_t;

typedef struct camera2_tonemap_dm {
	tonemap_mode_t	mode;
	float			curveRed[32]; // assuming maxCurvePoints = 32
	float			curveGreen[32];
	float			curveBlue[32];
} camera2_tonemap_dm_t;



/* android.edge */

typedef enum edge_mode
{
	EDGE_MODE_OFF,
	EDGE_MODE_FAST,
	EDGE_MODE_HIGH_QUALITY
} edge_mode_t;

typedef struct camera2_edge_ctl {
	edge_mode_t 	mode;
	uint8_t			strength;
} camera2_edge_ctl_t;

typedef struct camera2_edge_dm {
	edge_mode_t 	mode;
	uint8_t			strength;
} camera2_edge_dm_t;



/* android.scaler */

typedef struct camera2_scaler_ctl {
	uint32_t	cropRegion[3];
	uint32_t	rotation;
} camera2_scaler_ctl_t;

typedef struct camera2_scaler_dm {
	uint32_t	size[2];
	uint8_t		format;
	uint32_t	cropRegion[3];
	uint32_t	rotation;
} camera2_scaler_dm_t;



/* android.jpeg */

typedef struct camera2_jpeg_ctl {
	uint8_t		quality;
	uint32_t	thumbnailSize[2];
	uint8_t		thumbnailQuality;
	double		gpsCoordinates[3];
	uint8_t		gpsProcessingMethod;
	uint64_t	gpsTimestamp;
	uint32_t	orientation;
} camera2_jpeg_ctl_t;

typedef struct camera2_jpeg_dm {
	uint8_t		quality;
	uint32_t	thumbnailSize[2];
	uint8_t		thumbnailQuality;
	double		gpsCoordinates[3];
	uint8_t		gpsProcessingMethod;
	uint64_t	gpsTimestamp;
	uint32_t	orientation;
} camera2_jpeg_dm_t;



/* android.statistics */

typedef enum facedetect_mode
{
	FACEDETECT_MODE_OFF,
	FACEDETECT_MODE_SIMPLE,
	FACEDETECT_MODE_FULL
} facedetect_mode_t;

typedef enum histogram_mode
{
	HISTOGRAM_MODE_OFF,
	HISTOGRAM_MODE_ON
} histogram_mode_t;

typedef enum sharpnessmap_mode
{
	SHARPNESSMAP_MODE_OFF,
	SHARPNESSMAP_MODE_ON
} sharpnessmap_mode_t;

typedef struct camera2_stats_ctl {
	facedetect_mode_t	faceDetectMode;
	histogram_mode_t	histogramMode;
	sharpnessmap_mode_t	sharpnessMapMode;
} camera2_stats_ctl_t;

/* REMARKS : FD results are not included */
typedef struct camera2_stats_dm {
	facedetect_mode_t	faceDetectMode;
	// faceRetangles
	// faceScores
	// faceLandmarks
	// faceIds
	histogram_mode_t	histogramMode;
	// histogram
	sharpnessmap_mode_t	sharpnessMapMode;
	// sharpnessMap
} camera2_stats_dm_t;



/* android.control */

typedef enum aa_mode
{
	AA_MODE_OFF,
	AA_MODE_AUTO,
	AA_MODE_SCENE_MODE_FACE_PRIORITY,
	AA_MODE_SCENE_MODE_ACTION,
	AA_MODE_SCENE_MODE_PORTRAIT,
	AA_MODE_SCENE_MODE_LANDSCAPE,
	AA_MODE_SCENE_MODE_NIGHT,
	AA_MODE_SCENE_MODE_NIGHT_PORTRAIT,
	AA_MODE_SCENE_MODE_THEATRE,
	AA_MODE_SCENE_MODE_BEACH,
	AA_MODE_SCENE_MODE_SNOW,
	AA_MODE_SCENE_MODE_SUNSET,
	AA_MODE_SCENE_MODE_STEADYPHOTO,
	AA_MODE_SCENE_MODE_FIREWORKS,
	AA_MODE_SCENE_MODE_SPORTS,
	AA_MODE_SCENE_MODE_PARTY,
	AA_MODE_SCENE_MODE_CANDLELIGHT,
	AA_MODE_SCENE_MODE_BARCODE
} aa_mode_t;

typedef enum aa_aemode
{
	AA_AEMODE_OFF,
	AA_AEMODE_ON,
	AA_AEMODE_ON_AUTO_FLASH,
	AA_AEMODE_ON_ALWAYS_FLASH,
	AA_AEMODE_ON_AUTO_FLASH_REDEYE
} aa_aemode_t;

typedef enum aa_ae_antibanding_mode
{
	AA_AE_ANTIBANDING_OFF,
	AA_AE_ANTIBANDING_50HZ,
	AA_AE_ANTIBANDING_60HZ,
	AA_AE_ANTIBANDING_AUTO
} aa_ae_antibanding_mode_t;

typedef enum aa_awbmode
{
	AA_AWBMODE_OFF,
	AA_AWBMODE_WB_AUTO,
	AA_AWBMODE_WB_INCANDESCENT,
	AA_AWBMODE_WB_FLUORESCENT,
	AA_AWBMODE_WB_WARM_FLUORESCENT,
	AA_AWBMODE_WB_DAYLIGHT,
	AA_AWBMODE_WB_CLOUDY_DAYLIGHT,
	AA_AWBMODE_WB_TWILIGHT,
	AA_AWBMODE_WB_SHADE
} aa_awbmode_t;

typedef enum aa_afmode
{
	AA_AFMODE_OFF,
	AA_AFMODE_FOCUS_MODE_AUTO,
	AA_AFMODE_FOCUS_MODE_MACRO,
	AA_AFMODE_FOCUS_MODE_CONTINUOUS_VIDEO,
	AA_AFMODE_FOCUS_MODE_CONTINUOUS_PICTURE
} aa_afmode_t;

typedef enum aa_afstate
{
	AA_AFSTATE_INACTIVE,
	AA_AFSTATE_PASSIVE_SCAN,
	AA_AFSTATE_ACTIVE_SCAN,
	AA_AFSTATE_AF_ACQUIRED_FOCUS,
	AA_AFSTATE_AF_FAILED_FOCUS
} aa_afstate_t;

typedef struct camera2_aa_ctl {
	aa_mode_t					mode;
	aa_aemode_t					aeMode;
	uint32_t					aeRegions[5]; // 5 per region(x1,y1,x2,y2,weight). currently assuming 1 region.
	int32_t						aeExpCompensation;
	uint32_t					aeTargetFpsRange[2];
	aa_ae_antibanding_mode_t	aeAntibandingMode;
	aa_awbmode_t				awbMode;
	uint32_t					awbRegions[5]; // 5 per region(x1,y1,x2,y2,weight). currently assuming 1 region.
	aa_afmode_t 				afMode;
	uint32_t					afRegions[5]; // 5 per region(x1,y1,x2,y2,weight). currently assuming 1 region.
	uint8_t						afTrigger;
	uint8_t						videoStabilizationMode;
} camera2_aa_ctl_t;

typedef struct camera2_aa_dm {
	aa_mode_t					mode;
	aa_aemode_t					aeMode; // needs check
	uint32_t					aeRegions[5]; // 5 per region(x1,y1,x2,y2,weight). currently assuming 1 region.
	int32_t						aeExpCompensation; // needs check
	aa_awbmode_t				awbMode;
	uint32_t					awbRegions[5]; // 5 per region(x1,y1,x2,y2,weight). currently assuming 1 region.
	aa_afmode_t 				afMode;
	uint32_t					afRegions[5]; // 5 per region(x1,y1,x2,y2,weight). currently assuming 1 region.
	uint8_t						afTrigger;
	aa_afstate_t				afState;
	uint8_t						videoStabilizationMode;
} camera2_aa_dm_t;



	
// sizeof(camera2_ctl) = ?
typedef struct camera2_ctl {
	camera2_request_ctl_t 			request;
	camera2_lens_ctl_t				lens;
	camera2_sensor_ctl_t			sensor;
	camera2_flash_ctl_t				flash;
	camera2_hotpixel_ctl_t			hotpixel;
	camera2_demosaic_ctl_t			demosaic;
	camera2_noisereduction_ctl_t	noise;
	camera2_shading_ctl_t			shading;
	camera2_geometric_ctl_t			geometric;
	camera2_colorcorrection_ctl_t	color;
	camera2_tonemap_ctl_t			tonemap;
	camera2_edge_ctl_t				edge;
	camera2_scaler_ctl_t			scaler;
	camera2_jpeg_ctl_t				jpeg;
	camera2_stats_ctl_t				stats;
	camera2_aa_ctl_t				aa;
} camera2_ctl_t;

// sizeof(camera2_dm) = ?
typedef struct camera2_dm {
	camera2_request_dm_t 			request;
	camera2_lens_dm_t				lens;
	camera2_sensor_dm_t				sensor;
	camera2_flash_dm_t				flash;
	camera2_hotpixel_dm_t			hotpixel;
	camera2_demosaic_dm_t			demosaic;
	camera2_noisereduction_dm_t		noise;
	camera2_shading_dm_t			shading;
	camera2_geometric_dm_t			geometric;
	camera2_colorcorrection_dm_t	color;
	camera2_tonemap_dm_t			tonemap;
	camera2_edge_dm_t				edge;
	camera2_scaler_dm_t				scaler;
	camera2_jpeg_dm_t				jpeg;
	camera2_stats_dm_t				stats;
	camera2_aa_dm_t					aa;
} camera2_dm_t;

typedef struct camera2_vs {
    /** \brief
            Set sensor, lens, flash control for next frame.
        \remarks
            This flag can be combined.
                [0 bit] sensor
                [1 bit] lens
                [2 bit] flash
    */
    uint32_t updateFlag;

    camera2_lens_ctl_t				lens;
    camera2_sensor_ctl_t			sensor;
    camera2_flash_ctl_t				flash;
} camera2_vs_t;

typedef struct camera2_shot {
	camera2_ctl_t	ctl;
	camera2_dm_t	dm;
	/*vendor specific area*/
    camera2_vs_t	vender;
	uint32_t 		magicNumber;
} camera2_shot_t;

/*
 *   static metadata 
 */


/* android.lens */

typedef enum lens_facing
{
	LENS_FACING_FRONT,
	LENS_FACING_BACK
} lens_facing_t;

typedef struct camera2_lens_sm {
	float	minimumFocusDistance;
	float	availableFocalLength[2];
	float	availableApertures;  // assuming 1 aperture
	float	availableFilterDensities; // assuming 1 ND filter value
	uint8_t availableOpticalStabilization; // assuming 1
	float	shadingMap[3][40][30];
	float	geometricCorrectionMap[2][3][40][30];
	lens_facing_t facing;
	float	position[2];
} camera2_lens_sm_t;

/* android.sensor */

typedef enum sensor_colorfilterarrangement
{
	SENSOR_COLORFILTERARRANGEMENT_RGGB,
	SENSOR_COLORFILTERARRANGEMENT_GRBG,
	SENSOR_COLORFILTERARRANGEMENT_GBRG,
	SENSOR_COLORFILTERARRANGEMENT_BGGR,
	SENSOR_COLORFILTERARRANGEMENT_RGB	
} sensor_colorfilterarrangement_t;

typedef enum sensor_ref_illuminant
{
	SENSOR_ILLUMINANT_DAYLIGHT = 1,
	SENSOR_ILLUMINANT_FLUORESCENT = 2,
	SENSOR_ILLUMINANT_TUNGSTEN = 3,
	SENSOR_ILLUMINANT_FLASH = 4,
	SENSOR_ILLUMINANT_FINE_WEATHER = 9,
	SENSOR_ILLUMINANT_CLOUDY_WEATHER = 10,
	SENSOR_ILLUMINANT_SHADE = 11,
	SENSOR_ILLUMINANT_DAYLIGHT_FLUORESCENT = 12,
	SENSOR_ILLUMINANT_DAY_WHITE_FLUORESCENT = 13,
	SENSOR_ILLUMINANT_COOL_WHITE_FLUORESCENT = 14,
	SENSOR_ILLUMINANT_WHITE_FLUORESCENT = 15,
	SENSOR_ILLUMINANT_STANDARD_A = 17,
	SENSOR_ILLUMINANT_STANDARD_B = 18,
	SENSOR_ILLUMINANT_STANDARD_C = 19,
	SENSOR_ILLUMINANT_D55 = 20,
	SENSOR_ILLUMINANT_D65 = 21,
	SENSOR_ILLUMINANT_D75 = 22,
	SENSOR_ILLUMINANT_D50 = 23,
	SENSOR_ILLUMINANT_ISO_STUDIO_TUNGSTEN = 24
} sensor_ref_illuminant_t;

typedef struct camera2_sensor_sm {
	uint32_t	exposureTimeRange[2];
	uint32_t	maxFrameDuration;
	uint32_t	sensitivityRange[2];
	sensor_colorfilterarrangement_t colorFilterArrangement;
	uint32_t	pixelArraySize[2];
	uint32_t	activeArraySize[4];
	uint32_t	whiteLevel;
	uint32_t	blackLevelPattern[4];
	rational_t	colorTransform1[9];
	rational_t	colorTransform2[9];
	sensor_ref_illuminant_t	referenceIlluminant1;
	sensor_ref_illuminant_t	referenceIlluminant2;
	rational_t	forwardMatrix1[9];
	rational_t	forwardMatrix2[9];
	rational_t	calibrationTransform1[9];	
	rational_t	calibrationTransform2[9];
	rational_t	baseGainFactor;
	uint32_t	maxAnalogSensitivity;
	float	noiseModelCoefficients[2];
	uint32_t	orientation;
} camera2_sensor_sm_t;



/* android.flash */

typedef struct camera2_flash_sm {
	uint8_t		available;
	uint64_t	chargeDuration;
} camera2_flash_sm_t;



/* android.colorCorrection */

typedef struct camera2_colorcorrection_sm {
	colorcorrection_mode_t	availableModes[10]; // assuming 10 supported modes
} camera2_colorcorrection_sm_t;



/* android.tonemap */

typedef struct camera2_tonemap_sm {
	uint32_t	maxCurvePoints;
} camera2_tonemap_sm_t;



/* android.scaler */

typedef enum scaler_availableformats {
	SCALER_FORMAT_BAYER_RAW,
	SCALER_FORMAT_YV12,
	SCALER_FORMAT_NV21,
	SCALER_FORMAT_JPEG,
	SCALER_FORMAT_UNKNOWN
} scaler_availableformats_t;

typedef struct camera2_scaler_sm {
	scaler_availableformats_t availableFormats[4]; // assuming 
												   // # of availableFormats = 4
	uint32_t	availableSizesPerFormat[4];  
	uint32_t	availableSizes[4][8][2]; // assuning availableSizesPerFormat=8
	uint64_t	availableMinFrameDurations[4][8];
	float		maxDigitalZoom;
} camera2_scaler_sm_t;



/* android.jpeg */

typedef struct camera2_jpeg_sm {
	uint32_t	availableThumbnailSizes[2][8]; // assuming supported size=8
} camera2_jpeg_sm_t;



/* android.statistics */

typedef struct camera2_statistics_sm {
	uint8_t		availableFaceDetectModes[3]; // assuming supported modes = 3;
	uint32_t	maxFaceCount;
	uint32_t	histogramBucketCount;
	uint32_t	maxHistogramCount;
	uint32_t	sharpnessMapSize[2];
	uint32_t	maxSharpnessMapValue;
} camera2_statistics_sm_t;



/* android.control */

typedef struct camera2_aa_sm {
	uint8_t		availableModes[10]; // assuming # of available scene modes = 10
	uint32_t	maxRegions;
	uint8_t		aeAvailableModes[8]; // assuming # of available ae modes = 8
	rational_t	aeCompensationStep;
	int32_t		aeCompensationRange[2];
	uint32_t	aeAvailableTargetFpsRanges[2][8];
	uint8_t		aeAvailableAntibandingModes[4];
	uint8_t		awbAvailableModes[10]; // assuming # of awbAvailableModes = 10
	uint8_t		afAvailableModes[4]; // assuming # of afAvailableModes = 4
} camera2_aa_sm_t;

typedef struct camera2_static_metadata {
	camera2_lens_sm_t				lens;
	camera2_sensor_sm_t 			sensor;
	camera2_flash_sm_t				flash;
	camera2_colorcorrection_sm_t 	color;
	camera2_tonemap_sm_t 			tonemap;
	camera2_scaler_sm_t				scaler;
	camera2_jpeg_sm_t				jpeg;
	camera2_statistics_sm_t			statistics;
	camera2_aa_sm_t					aa;
} camera2_static_metadata_t;

#endif

