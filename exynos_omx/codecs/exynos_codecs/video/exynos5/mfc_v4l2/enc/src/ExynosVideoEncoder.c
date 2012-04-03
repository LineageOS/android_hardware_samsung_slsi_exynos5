/*
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, Licensed under the Apache License, Version 2.0 (the "License");
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

/*
 * @file        ExynosVideoEncoder.c
 * @brief
 * @author      Hyeyeon Chung (hyeon.chung@samsung.com)
 * @author      Jinsung Yang (jsgood.yang@samsung.com)
 * @author      Yunji Kim (yunji.kim@samsung.com)
 * @version     1.0.0
 * @history
 *   2012.02.09: Initial Version
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <sys/poll.h>

#include "ExynosVideoApi.h"
#include "ExynosVideoEnc.h"
#include "exynos_v4l2.h"

/* #define LOG_NDEBUG 0 */
#define LOG_TAG "ExynosVideoEncoder"
#include <utils/Log.h>

#define MAX_CTRL_NUM    92
#define H264_CTRL_NUM   92
#define MPEG4_CTRL_NUM  26
#define H263_CTRL_NUM   18

/*
 * [Common] __CodingType_To_V4L2PixelFormat
 */
static unsigned int __CodingType_To_V4L2PixelFormat(ExynosVideoCodingType codingType)
{
    unsigned int pixelformat = V4L2_PIX_FMT_H264;

    switch (codingType) {
    case VIDEO_CODING_AVC:
        pixelformat = V4L2_PIX_FMT_H264;
        break;
    case VIDEO_CODING_MPEG4:
        pixelformat = V4L2_PIX_FMT_MPEG4;
        break;
    case VIDEO_CODING_VP8:
        pixelformat = V4L2_PIX_FMT_VP8;
        break;
    case VIDEO_CODING_H263:
        pixelformat = V4L2_PIX_FMT_H263;
        break;
    case VIDEO_CODING_VC1:
        pixelformat = V4L2_PIX_FMT_VC1_ANNEX_G;
        break;
    case VIDEO_CODING_VC1_RCV:
        pixelformat = V4L2_PIX_FMT_VC1_ANNEX_L;
        break;
    case VIDEO_CODING_MPEG2:
        pixelformat = V4L2_PIX_FMT_MPEG2;
        break;
    default:
        pixelformat = V4L2_PIX_FMT_H264;
        break;
    }

    return pixelformat;
}

/*
 * [Common] __ColorFormatType_To_V4L2PixelFormat
 */
static unsigned int __ColorFormatType_To_V4L2PixelFormat(ExynosVideoColorFormatType colorFormatType)
{
    unsigned int pixelformat = V4L2_PIX_FMT_NV12M;

    switch (colorFormatType) {
    case VIDEO_COLORFORMAT_NV12_TILED:
        pixelformat = V4L2_PIX_FMT_NV12MT_16X16;
        break;
    case VIDEO_COLORFORMAT_NV21:
        pixelformat = V4L2_PIX_FMT_NV21M;
        break;
    case VIDEO_COLORFORMAT_NV12:
    default:
        pixelformat = V4L2_PIX_FMT_NV12M;
        break;
    }

    return pixelformat;
}

/*
 * [Encoder OPS] Init
 */
static void *MFC_Encoder_Init(void)
{
    ExynosVideoEncContext *pCtx = NULL;
    int needCaps = (V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_VIDEO_OUTPUT | V4L2_CAP_STREAMING);

    pCtx = (ExynosVideoEncContext *)malloc(sizeof(*pCtx));
    if (pCtx == NULL) {
        ALOGE("%s: Failed to allocate encoder context buffer", __func__);
        goto EXIT_ALLOC_FAIL;
    }

    memset(pCtx, 0, sizeof(*pCtx));

    pCtx->hEnc = exynos_v4l2_open_devname(VIDEO_ENCODER_NAME, O_RDWR | O_NONBLOCK, 0);
    if (pCtx->hEnc < 0) {
        ALOGE("%s: Failed to open encoder device", __func__);
        goto EXIT_OPEN_FAIL;
    }

    if (!exynos_v4l2_querycap(pCtx->hEnc, needCaps)) {
        ALOGE("%s: Failed to querycap", __func__);
        goto EXIT_QUERYCAP_FAIL;
    }

    return (void *)pCtx;

EXIT_QUERYCAP_FAIL:
    close(pCtx->hEnc);

EXIT_OPEN_FAIL:
    free(pCtx);
    pCtx = NULL;

EXIT_ALLOC_FAIL:
    return NULL;
}

/*
 * [Encoder OPS] Finalize
 */
static ExynosVideoErrorType MFC_Encoder_Finalize(void *pHandle)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;
    ExynosVideoPlane      *pVideoPlane;

    int i, j;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (pCtx->bShareInbuf == VIDEO_FALSE) {
        for (i = 0; i < pCtx->nInbufs; i++) {
            for (j = 0; j < VIDEO_ENCODER_INBUF_PLANES; j++) {
                pVideoPlane = &pCtx->pInbuf[i].planes[j];
                if (pVideoPlane->addr != NULL) {
                    munmap(pVideoPlane->addr, pVideoPlane->allocSize);
                    pVideoPlane->addr = NULL;
                    pVideoPlane->allocSize = 0;
                    pVideoPlane->dataSize = 0;
                }

                pCtx->pInbuf[i].pGeometry = NULL;
                pCtx->pInbuf[i].bQueued = VIDEO_FALSE;
            }
        }
    }

    if (pCtx->bShareOutbuf == VIDEO_FALSE) {
        for (i = 0; i < pCtx->nOutbufs; i++) {
            for (j = 0; j < VIDEO_ENCODER_OUTBUF_PLANES; j++) {
                pVideoPlane = &pCtx->pOutbuf[i].planes[j];
                if (pVideoPlane->addr != NULL) {
                    munmap(pVideoPlane->addr, pVideoPlane->allocSize);
                    pVideoPlane->addr = NULL;
                    pVideoPlane->allocSize = 0;
                    pVideoPlane->dataSize = 0;
                }

                pCtx->pOutbuf[i].pGeometry = NULL;
                pCtx->pOutbuf[i].bQueued = VIDEO_FALSE;
            }
        }
    }

    if (pCtx->pInbuf != NULL)
        free(pCtx->pInbuf);

    if (pCtx->pOutbuf != NULL)
        free(pCtx->pOutbuf);

    if (pCtx->hEnc > 0)
        close(pCtx->hEnc);

    free(pCtx);

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Enable Frame Skip
 */
static ExynosVideoErrorType MFC_Decoder_Enable_FrameSkip(
    void *pHandle,
    ExynosVideoEncParam*encParam,
    ExynosVideoFrameSkipMode frameSkip)
{
    ExynosVideoEncContext   *pCtx       = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType     ret        = VIDEO_ERROR_NONE;
    ExynosVideoEncInitParam *pInitParam = &encParam->initParam;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    pInitParam->FrameSkip = frameSkip;

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Set Extended Control
 */
static ExynosVideoErrorType MFC_Encoder_Set_EncParam (
    void *pHandle,
    ExynosVideoEncParam*pEncParam)
{
    ExynosVideoEncContext     *pCtx         = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType       ret          = VIDEO_ERROR_NONE;
    ExynosVideoEncInitParam   *pInitParam   = &pEncParam->initParam;
    ExynosVideoEncCommonParam *pCommonParam = &(pEncParam->commonParam);
    ExynosVideoEncH264Param*   pH264Param   = NULL;
    ExynosVideoEncMpeg4Param*  pMpeg4Param  = NULL;
    ExynosVideoEncH263Param*   pH263Param   = NULL;

    int i;
    struct v4l2_ext_control  ext_ctrl[MAX_CTRL_NUM];
    struct v4l2_ext_controls ext_ctrls;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    /* common parameters */
    ext_ctrl[0].id = V4L2_CID_MPEG_VIDEO_GOP_SIZE;
    ext_ctrl[0].value = pCommonParam->IDRPeriod;
    ext_ctrl[1].id = V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MODE;
    ext_ctrl[1].value = pCommonParam->SliceMode;  /* 0: one, 1: fixed #mb, 3: fixed #bytes */
    ext_ctrl[2].id = V4L2_CID_MPEG_VIDEO_CYCLIC_INTRA_REFRESH_MB;
    ext_ctrl[2].value = pCommonParam->RandomIntraMBRefresh;
    ext_ctrl[3].id = V4L2_CID_MPEG_MFC51_VIDEO_PADDING;
    ext_ctrl[3].value = pCommonParam->PadControlOn;
    ext_ctrl[4].id = V4L2_CID_MPEG_MFC51_VIDEO_PADDING_YUV;
    ext_ctrl[4].value = pCommonParam->CrPadVal;
    ext_ctrl[4].value |= (pCommonParam->CbPadVal << 8);
    ext_ctrl[4].value |= (pCommonParam->LumaPadVal << 16);
    ext_ctrl[5].id = V4L2_CID_MPEG_VIDEO_FRAME_RC_ENABLE;
    ext_ctrl[5].value = pCommonParam->EnableFRMRateControl;
    ext_ctrl[6].id = V4L2_CID_MPEG_VIDEO_BITRATE;

    /* FIXME temporary fix */
    if (pCommonParam->Bitrate)
        ext_ctrl[6].value = pCommonParam->Bitrate;
    else
        ext_ctrl[6].value = 1; /* just for testing Movie studio */

    /* codec specific parameters */
    switch (pEncParam->eCompressionFormat) {
    case VIDEO_CODING_AVC:
        pH264Param = &pEncParam->codecParam.h264;

        /* common parameters but id is depends on codec */
        ext_ctrl[7].id = V4L2_CID_MPEG_VIDEO_H264_I_FRAME_QP;
        ext_ctrl[7].value = pCommonParam->FrameQp;
        ext_ctrl[8].id =  V4L2_CID_MPEG_VIDEO_H264_P_FRAME_QP;
        ext_ctrl[8].value = pCommonParam->FrameQp_P;
        ext_ctrl[9].id =  V4L2_CID_MPEG_VIDEO_H264_MAX_QP;
        ext_ctrl[9].value = pCommonParam->QSCodeMax;
        ext_ctrl[10].id = V4L2_CID_MPEG_VIDEO_H264_MIN_QP;
        ext_ctrl[10].value = pCommonParam->QSCodeMin;
        ext_ctrl[11].id = V4L2_CID_MPEG_MFC51_VIDEO_RC_REACTION_COEFF;
        ext_ctrl[11].value = pCommonParam->CBRPeriodRf;

        /* H.264 specific parameters */
        if (pCommonParam->SliceMode == 0) {
            ext_ctrl[12].id = V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MAX_MB;
            ext_ctrl[12].value = 1;  /* default */
            ext_ctrl[13].id = V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MAX_BYTES;
            ext_ctrl[13].value = 2800; /* based on MFC6.x */
        } else if (pCommonParam->SliceMode == 1) {
            ext_ctrl[12].id = V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MAX_MB;
            ext_ctrl[12].value = pH264Param->SliceArgument;
            ext_ctrl[13].id = V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MAX_BYTES;
            ext_ctrl[13].value = 2800; /* based on MFC6.x */
        } else if (pCommonParam->SliceMode == 3) {
            ext_ctrl[12].id = V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MAX_MB;
            ext_ctrl[12].value = 1; /* default */
            ext_ctrl[13].id = V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MAX_BYTES;
            ext_ctrl[13].value = pH264Param->SliceArgument;
        }

        ext_ctrl[14].id = V4L2_CID_MPEG_VIDEO_H264_PROFILE;
        ext_ctrl[14].value = pH264Param->ProfileIDC;
        ext_ctrl[15].id = V4L2_CID_MPEG_VIDEO_H264_LEVEL;
        ext_ctrl[15].value = pH264Param->LevelIDC;
        ext_ctrl[16].id = V4L2_CID_MPEG_VIDEO_MAX_REF_PIC;
        ext_ctrl[16].value = pH264Param->NumberReferenceFrames;
        ext_ctrl[17].id = V4L2_CID_MPEG_MFC51_VIDEO_H264_NUM_REF_PIC_FOR_P;
        ext_ctrl[17].value = pH264Param->NumberRefForPframes;
        /*
         * It should be set using h264Param->NumberBFrames after being handled by appl.
         */
        ext_ctrl[18].id =  V4L2_CID_MPEG_VIDEO_B_FRAMES;
        ext_ctrl[18].value = pH264Param->NumberBFrames;
        ext_ctrl[19].id = V4L2_CID_MPEG_VIDEO_H264_LOOP_FILTER_MODE;
        ext_ctrl[19].value = pH264Param->LoopFilterDisable;
        ext_ctrl[20].id = V4L2_CID_MPEG_VIDEO_H264_LOOP_FILTER_ALPHA;
        ext_ctrl[20].value = pH264Param->LoopFilterAlphaC0Offset;
        ext_ctrl[21].id = V4L2_CID_MPEG_VIDEO_H264_LOOP_FILTER_BETA;
        ext_ctrl[21].value = pH264Param->LoopFilterBetaOffset;
        ext_ctrl[22].id = V4L2_CID_MPEG_VIDEO_H264_ENTROPY_MODE;
        ext_ctrl[22].value = pH264Param->SymbolMode;
        ext_ctrl[23].id = V4L2_CID_MPEG_MFC51_VIDEO_H264_INTERLACE;
        ext_ctrl[23].value = pH264Param->PictureInterlace;
        ext_ctrl[24].id = V4L2_CID_MPEG_VIDEO_H264_8X8_TRANSFORM;
        ext_ctrl[24].value = pH264Param->Transform8x8Mode;
        ext_ctrl[25].id = V4L2_CID_MPEG_VIDEO_MB_RC_ENABLE;
        ext_ctrl[25].value = pH264Param->EnableMBRateControl;
        ext_ctrl[26].id = V4L2_CID_MPEG_MFC51_VIDEO_H264_RC_FRAME_RATE;
        ext_ctrl[26].value = pH264Param->FrameRate;
        ext_ctrl[27].id =  V4L2_CID_MPEG_VIDEO_H264_B_FRAME_QP;
        ext_ctrl[27].value = pH264Param->FrameQp_B;
        ext_ctrl[28].id =  V4L2_CID_MPEG_MFC51_VIDEO_H264_ADAPTIVE_RC_DARK;
        ext_ctrl[28].value = pH264Param->DarkDisable;
        ext_ctrl[29].id =  V4L2_CID_MPEG_MFC51_VIDEO_H264_ADAPTIVE_RC_SMOOTH;
        ext_ctrl[29].value = pH264Param->SmoothDisable;
        ext_ctrl[30].id =  V4L2_CID_MPEG_MFC51_VIDEO_H264_ADAPTIVE_RC_STATIC;
        ext_ctrl[30].value = pH264Param->StaticDisable;
        ext_ctrl[31].id =  V4L2_CID_MPEG_MFC51_VIDEO_H264_ADAPTIVE_RC_ACTIVITY;
        ext_ctrl[31].value = pH264Param->ActivityDisable;

        /* doesn't have to be set */
        ext_ctrl[32].id = V4L2_CID_MPEG_VIDEO_GOP_CLOSURE;
        ext_ctrl[32].value = 0;
        ext_ctrl[33].id = V4L2_CID_MPEG_VIDEO_H264_I_PERIOD;
        ext_ctrl[33].value = 10;
        ext_ctrl[34].id = V4L2_CID_MPEG_VIDEO_VBV_SIZE;
        ext_ctrl[34].value = 0;
        ext_ctrl[35].id = V4L2_CID_MPEG_VIDEO_HEADER_MODE;
        ext_ctrl[35].value = 0; /* 0: seperated header, 1: header + first frame */
        ext_ctrl[36].id =  V4L2_CID_MPEG_VIDEO_H264_VUI_SAR_ENABLE;
        ext_ctrl[36].value = 0;
        ext_ctrl[37].id = V4L2_CID_MPEG_VIDEO_H264_VUI_SAR_IDC;
        ext_ctrl[37].value = V4L2_MPEG_VIDEO_H264_VUI_SAR_IDC_UNSPECIFIED;
        ext_ctrl[38].id = V4L2_CID_MPEG_VIDEO_H264_VUI_EXT_SAR_WIDTH;
        ext_ctrl[38].value = 0;
        ext_ctrl[39].id =  V4L2_CID_MPEG_VIDEO_H264_VUI_EXT_SAR_HEIGHT;
        ext_ctrl[39].value = 0;

        /* Initial parameters : Frame Skip */
        if (pInitParam->FrameSkip == V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_LEVEL_LIMIT) {
            ext_ctrl[40].id = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE;
            ext_ctrl[40].value = V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_LEVEL_LIMIT;
        } else if (pInitParam->FrameSkip == V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_BUF_LIMIT) {
            ext_ctrl[40].id = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE;
            ext_ctrl[40].value = V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_BUF_LIMIT;
        } else {
            /* ENC_FRAME_SKIP_MODE_DISABLE (default) */
            ext_ctrl[40].id = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE;
            ext_ctrl[40].value = V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_DISABLED;
        }

        /* SVC is not supported yet */
        ext_ctrl[41].id =  V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING;
        ext_ctrl[41].value = 0;
        ext_ctrl[42].id =  V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_TYPE;
        ext_ctrl[42].value = V4L2_MPEG_VIDEO_H264_HIERARCHICAL_CODING_B;
        ext_ctrl[43].id =  V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER;
        ext_ctrl[43].value = 3;
        ext_ctrl[44].id =  V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER_QP;
        ext_ctrl[44].value = (0 << 16 | 0);
        ext_ctrl[45].id =  V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER_QP;
        ext_ctrl[45].value = (1 << 16 | 0);
        ext_ctrl[46].id =  V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER_QP;
        ext_ctrl[46].value = (2 << 16 | 0);

        ext_ctrl[47].id =  V4L2_CID_MPEG_VIDEO_H264_SEI_FRAME_PACKING;
        ext_ctrl[47].value = 0;
        ext_ctrl[48].id =  V4L2_CID_MPEG_VIDEO_H264_SEI_FP_CURRENT_FRAME_0;
        ext_ctrl[48].value = 0;
        ext_ctrl[49].id =  V4L2_CID_MPEG_VIDEO_H264_SEI_FP_ARRANGEMENT_TYPE;
        ext_ctrl[49].value = V4L2_MPEG_VIDEO_H264_SEI_FP_TYPE_SIDE_BY_SIDE;

        /* FMO is not supported yet */
        ext_ctrl[50].id =  V4L2_CID_MPEG_VIDEO_H264_FMO;
        ext_ctrl[50].value = 0;
        ext_ctrl[51].id =  V4L2_CID_MPEG_VIDEO_H264_FMO_MAP_TYPE;
        ext_ctrl[51].value = V4L2_MPEG_VIDEO_H264_FMO_MAP_TYPE_INTERLEAVED_SLICES;
        ext_ctrl[52].id = V4L2_CID_MPEG_VIDEO_H264_FMO_SLICE_GROUP;
        ext_ctrl[52].value = 4;
        ext_ctrl[53].id =  V4L2_CID_MPEG_VIDEO_H264_FMO_RUN_LENGTH;
        ext_ctrl[53].value = (0 << 30 | 0);
        ext_ctrl[54].id =  V4L2_CID_MPEG_VIDEO_H264_FMO_RUN_LENGTH;
        ext_ctrl[54].value = (1 << 30 | 0);
        ext_ctrl[55].id =  V4L2_CID_MPEG_VIDEO_H264_FMO_RUN_LENGTH;
        ext_ctrl[55].value = (2 << 30 | 0);
        ext_ctrl[56].id =  V4L2_CID_MPEG_VIDEO_H264_FMO_RUN_LENGTH;
        ext_ctrl[56].value = (3 << 30 | 0);
        ext_ctrl[57].id =  V4L2_CID_MPEG_VIDEO_H264_FMO_CHANGE_DIRECTION;
        ext_ctrl[57].value = V4L2_MPEG_VIDEO_H264_FMO_CHANGE_DIR_RIGHT;
        ext_ctrl[58].id = V4L2_CID_MPEG_VIDEO_H264_FMO_CHANGE_RATE;
        ext_ctrl[58].value = 0;

        /* ASO is not supported yet */
        ext_ctrl[59].id =  V4L2_CID_MPEG_VIDEO_H264_ASO;
        ext_ctrl[59].value = 0;
        for (i = 0; i < 32; i++) {
            ext_ctrl[60 + i].id =  V4L2_CID_MPEG_VIDEO_H264_ASO_SLICE_ORDER;
            ext_ctrl[60 + i].value = (i << 16 | 0);
        }
        break;

    case VIDEO_CODING_MPEG4:
        pMpeg4Param = &pEncParam->codecParam.mpeg4;

        /* common parameters but id is depends on codec */
        ext_ctrl[7].id = V4L2_CID_MPEG_VIDEO_MPEG4_I_FRAME_QP;
        ext_ctrl[7].value = pCommonParam->FrameQp;
        ext_ctrl[8].id =  V4L2_CID_MPEG_VIDEO_MPEG4_P_FRAME_QP;
        ext_ctrl[8].value = pCommonParam->FrameQp_P;
        ext_ctrl[9].id =  V4L2_CID_MPEG_VIDEO_MPEG4_MAX_QP;
        ext_ctrl[9].value = pCommonParam->QSCodeMax;
        ext_ctrl[10].id = V4L2_CID_MPEG_VIDEO_MPEG4_MIN_QP;
        ext_ctrl[10].value = pCommonParam->QSCodeMin;
        ext_ctrl[11].id = V4L2_CID_MPEG_MFC51_VIDEO_RC_REACTION_COEFF;
        ext_ctrl[11].value = pCommonParam->CBRPeriodRf;

        /* MPEG4 specific parameters */
        if (pCommonParam->SliceMode == 0) {
            ext_ctrl[12].id = V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MAX_MB;
            ext_ctrl[12].value = 1;  /* default */
            ext_ctrl[13].id = V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MAX_BYTES;
            ext_ctrl[13].value = 2800; /* based on MFC6.x */
        } else if (pCommonParam->SliceMode == 1) {
            ext_ctrl[12].id = V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MAX_MB;
            ext_ctrl[12].value = pMpeg4Param->SliceArgument;
            ext_ctrl[13].id = V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MAX_BYTES;
            ext_ctrl[13].value = 2800; /* based on MFC6.x */
        } else if (pCommonParam->SliceMode == 3) {
            ext_ctrl[12].id = V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MAX_MB;
            ext_ctrl[12].value = 1; /* default */
            ext_ctrl[13].id = V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MAX_BYTES;
            ext_ctrl[13].value = pMpeg4Param->SliceArgument;
        }

        ext_ctrl[14].id = V4L2_CID_MPEG_VIDEO_MPEG4_PROFILE;
        ext_ctrl[14].value = pMpeg4Param->ProfileIDC;
        ext_ctrl[15].id = V4L2_CID_MPEG_VIDEO_MPEG4_LEVEL;
        ext_ctrl[15].value = pMpeg4Param->LevelIDC;
        ext_ctrl[16].id = V4L2_CID_MPEG_VIDEO_MPEG4_QPEL;
        ext_ctrl[16].value = pMpeg4Param->DisableQpelME;

        /*
         * It should be set using mpeg4Param->NumberBFrames after being handled by appl.
         */
        ext_ctrl[17].id =  V4L2_CID_MPEG_VIDEO_B_FRAMES;
        ext_ctrl[17].value = pMpeg4Param->NumberBFrames;

        ext_ctrl[18].id = V4L2_CID_MPEG_MFC51_VIDEO_MPEG4_VOP_TIME_RES;
        ext_ctrl[18].value = pMpeg4Param->TimeIncreamentRes;
        ext_ctrl[19].id = V4L2_CID_MPEG_MFC51_VIDEO_MPEG4_VOP_FRM_DELTA;
        ext_ctrl[19].value = pMpeg4Param->VopTimeIncreament;
        ext_ctrl[20].id =  V4L2_CID_MPEG_VIDEO_MPEG4_B_FRAME_QP;
        ext_ctrl[20].value = pMpeg4Param->FrameQp_B;
        ext_ctrl[21].id = V4L2_CID_MPEG_VIDEO_VBV_SIZE;
        ext_ctrl[21].value = 0;
        ext_ctrl[22].id = V4L2_CID_MPEG_VIDEO_HEADER_MODE;
        ext_ctrl[22].value = 0;
        ext_ctrl[23].id = V4L2_CID_MPEG_MFC51_VIDEO_RC_FIXED_TARGET_BIT;
        ext_ctrl[23].value = 1;

        /* MFC 6.x Only */
        ext_ctrl[24].id = V4L2_CID_MPEG_VIDEO_MB_RC_ENABLE;
        ext_ctrl[24].value = pMpeg4Param->EnableMBRateControl;

        /* Initial parameters : Frame Skip */
        if (pInitParam->FrameSkip == V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_LEVEL_LIMIT) {
            ext_ctrl[25].id = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE;
            ext_ctrl[25].value = V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_LEVEL_LIMIT;
        } else if (pInitParam->FrameSkip == V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_BUF_LIMIT) {
            ext_ctrl[25].id = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE;
            ext_ctrl[25].value = V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_BUF_LIMIT;
        } else {
            /* ENC_FRAME_SKIP_MODE_DISABLE (default) */
            ext_ctrl[25].id = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE;
            ext_ctrl[25].value = V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_DISABLED;
        }

        break;

    case VIDEO_CODING_H263:
        pH263Param = &pEncParam->codecParam.h263;

        /* common parameters but id is depends on codec */
        ext_ctrl[7].id = V4L2_CID_MPEG_VIDEO_H263_I_FRAME_QP;
        ext_ctrl[7].value = pCommonParam->FrameQp;
        ext_ctrl[8].id =  V4L2_CID_MPEG_VIDEO_H263_P_FRAME_QP;
        ext_ctrl[8].value = pCommonParam->FrameQp_P;
        ext_ctrl[9].id =  V4L2_CID_MPEG_VIDEO_H263_MAX_QP;
        ext_ctrl[9].value = pCommonParam->QSCodeMax;
        ext_ctrl[10].id = V4L2_CID_MPEG_VIDEO_H263_MIN_QP;
        ext_ctrl[10].value = pCommonParam->QSCodeMin;
        ext_ctrl[11].id = V4L2_CID_MPEG_MFC51_VIDEO_RC_REACTION_COEFF;
        ext_ctrl[11].value = pCommonParam->CBRPeriodRf;

        /* H263 specific parameters */
        ext_ctrl[12].id = V4L2_CID_MPEG_MFC51_VIDEO_H263_RC_FRAME_RATE;
        ext_ctrl[12].value = pH263Param->FrameRate;
        ext_ctrl[13].id = V4L2_CID_MPEG_VIDEO_VBV_SIZE;
        ext_ctrl[13].value = 0;
        ext_ctrl[14].id = V4L2_CID_MPEG_VIDEO_HEADER_MODE;
        ext_ctrl[14].value = V4L2_MPEG_VIDEO_HEADER_MODE_SEPARATE;
        ext_ctrl[15].id = V4L2_CID_MPEG_MFC51_VIDEO_RC_FIXED_TARGET_BIT;
        ext_ctrl[15].value = 1;

         /* MFC 6.x Only */
        ext_ctrl[16].id = V4L2_CID_MPEG_VIDEO_MB_RC_ENABLE;
        ext_ctrl[16].value = pH263Param->EnableMBRateControl;

        /* Initial parameters : Frame Skip */
        if (pInitParam->FrameSkip == V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_LEVEL_LIMIT) {
            ext_ctrl[17].id = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE;
            ext_ctrl[17].value = V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_LEVEL_LIMIT;
        } else if (pInitParam->FrameSkip == V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_BUF_LIMIT) {
            ext_ctrl[17].id = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE;
            ext_ctrl[17].value = V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_BUF_LIMIT;
        } else {
            /* ENC_FRAME_SKIP_MODE_DISABLE (default) */
            ext_ctrl[17].id = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE;
            ext_ctrl[17].value = V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_DISABLED;
        }

        break;

    default:
        ALOGE("[%s] Undefined codec type",__func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    ext_ctrls.ctrl_class = V4L2_CTRL_CLASS_MPEG;
    ext_ctrls.controls = ext_ctrl;
    if (pEncParam->eCompressionFormat== VIDEO_CODING_AVC)
        ext_ctrls.count = H264_CTRL_NUM;
    else if (pEncParam->eCompressionFormat == VIDEO_CODING_MPEG4)
        ext_ctrls.count = MPEG4_CTRL_NUM;
    else if (pEncParam->eCompressionFormat == VIDEO_CODING_H263)
        ext_ctrls.count = H263_CTRL_NUM;

    if (exynos_v4l2_s_ext_ctrl(pCtx->hEnc, &ext_ctrls)) {
        ALOGE("%s: Failed to s_ext_ctrl", __func__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Set Frame Tag
 */
static ExynosVideoErrorType MFC_Encoder_Set_FrameTag(
    void *pHandle,
    int frameTag)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hEnc, V4L2_CID_MPEG_MFC51_VIDEO_FRAME_TAG, frameTag)) {
        ALOGE("%s: Failed to s_ctrl", __func__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Get Frame Tag
 */
static int MFC_Encoder_Get_FrameTag(void *pHandle)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    int frameTag = -1;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        goto EXIT;
    }

    if (exynos_v4l2_g_ctrl(pCtx->hEnc, V4L2_CID_MPEG_MFC51_VIDEO_FRAME_TAG, &frameTag)) {
        ALOGE("%s: Failed to g_ctrl", __func__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return frameTag;
}

/*
 * [Encoder OPS] Set Frame Type
 */
static ExynosVideoErrorType MFC_Encoder_Set_FrameType(
    void *pHandle,
    ExynosVideoFrameType frameType)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hEnc, V4L2_CID_MPEG_MFC51_VIDEO_FORCE_FRAME_TYPE, frameType)) {
        ALOGE("%s: Failed to s_ctrl", __func__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Set Frame Rate
 */
static ExynosVideoErrorType MFC_Encoder_Set_FrameRate(
    void *pHandle,
    int frameRate)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hEnc, V4L2_CID_MPEG_MFC51_VIDEO_FRAME_RATE_CH, frameRate)) {
        ALOGE("%s: Failed to s_ctrl", __func__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Set Bit Rate
 */
static ExynosVideoErrorType MFC_Encoder_Set_BitRate(
    void *pHandle,
    int bitRate)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hEnc, V4L2_CID_MPEG_MFC51_VIDEO_BIT_RATE_CH, bitRate)) {
        ALOGE("%s: Failed to s_ctrl", __func__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Set Frame Skip
 */
static ExynosVideoErrorType MFC_Encoder_Set_FrameSkip(
    void *pHandle,
    int frameSkip)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hEnc, V4L2_CID_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE, frameSkip)) {
        ALOGE("%s: Failed to s_ctrl", __func__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Set IDR Period
 */
static ExynosVideoErrorType MFC_Encoder_Set_IDRPeriod(
    void *pHandle,
    int IDRPeriod)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hEnc, V4L2_CID_MPEG_VIDEO_H264_I_PERIOD, IDRPeriod)) {
        ALOGE("%s: Failed to s_ctrl", __func__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}


/*
 * [Encoder Buffer OPS] Set Shareable Buffer (Input)
 */
static ExynosVideoErrorType MFC_Encoder_Set_Shareable_Inbuf(void *pHandle)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    pCtx->bShareInbuf = VIDEO_TRUE;

EXIT:
    return ret;
}

/*
 * [Encoder Buffer OPS] Set Shareable Buffer (Output)
 */
static ExynosVideoErrorType MFC_Encoder_Set_Shareable_Outbuf(void *pHandle)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    pCtx->bShareOutbuf = VIDEO_TRUE;

EXIT:
    return ret;
}

/*
 * [Encoder Buffer OPS] Get Buffer Info (Input)
 */
static ExynosVideoErrorType MFC_Encoder_Get_BufferInfo_Inbuf(
    void *pHandle,
    int nIndex,
    ExynosVideoBuffer *pBuffer)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_NOBUFFERS;
        goto EXIT;
    }

    memcpy(pBuffer, &pCtx->pInbuf[nIndex], sizeof(*pBuffer));

EXIT:
    return ret;
}

/*
 * [Encoder Buffer OPS] Get Buffer Info (Output)
 */
static ExynosVideoErrorType MFC_Encoder_Get_BufferInfo_Outbuf(
    void *pHandle,
    int nIndex,
    ExynosVideoBuffer *pBuffer)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_NOBUFFERS;
        goto EXIT;
    }

    memcpy(pBuffer, &pCtx->pOutbuf[nIndex], sizeof(*pBuffer));

EXIT:
    return ret;
}

/*
 * [Encoder Buffer OPS] Set Geometry (Src)
 */
static ExynosVideoErrorType MFC_Encoder_Set_Geometry_Inbuf(
    void *pHandle,
    ExynosVideoGeometry *bufferConf)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    struct v4l2_format fmt;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (bufferConf == NULL) {
        ALOGE("%s: Buffer geometry must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    memset(&fmt, 0, sizeof(fmt));

    fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    fmt.fmt.pix_mp.pixelformat = __ColorFormatType_To_V4L2PixelFormat(bufferConf->eColorFormat);
    fmt.fmt.pix_mp.width = bufferConf->nFrameWidth;
    fmt.fmt.pix_mp.height = bufferConf->nFrameHeight;
    fmt.fmt.pix_mp.num_planes = VIDEO_ENCODER_INBUF_PLANES;

    if (exynos_v4l2_s_fmt(pCtx->hEnc, &fmt)) {
        ALOGE("%s: Failed to s_fmt", __func__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    memcpy(&pCtx->outbufGeometry, bufferConf, sizeof(pCtx->outbufGeometry));

EXIT:
    return ret;
}

/*
 * [Encoder Buffer OPS] Get Geometry (Src)
 */
static ExynosVideoErrorType MFC_Encoder_Get_Geometry_Inbuf(
    void *pHandle,
    ExynosVideoGeometry *bufferConf)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    struct v4l2_format fmt;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (bufferConf == NULL) {
        ALOGE("%s: Buffer geometry must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    memset(&fmt, 0, sizeof(fmt));

    fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    if (exynos_v4l2_g_fmt(pCtx->hEnc, &fmt)) {
        ALOGE("%s: Failed to g_fmt", __func__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    bufferConf->nFrameHeight = fmt.fmt.pix_mp.width;
    bufferConf->nFrameHeight = fmt.fmt.pix_mp.height;
    bufferConf->nSizeImage = fmt.fmt.pix_mp.plane_fmt[0].sizeimage;

EXIT:
    return ret;
}

/*
 * [Encoder Buffer OPS] Set Geometry (Dst)
 */
static ExynosVideoErrorType MFC_Encoder_Set_Geometry_Outbuf(
    void *pHandle,
    ExynosVideoGeometry *bufferConf)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    struct v4l2_format fmt;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (bufferConf == NULL) {
        ALOGE("%s: Buffer geometry must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    memset(&fmt, 0, sizeof(fmt));

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    fmt.fmt.pix_mp.pixelformat = __CodingType_To_V4L2PixelFormat(bufferConf->eCompressionFormat);
    fmt.fmt.pix_mp.plane_fmt[0].sizeimage = bufferConf->nSizeImage;

    if (exynos_v4l2_s_fmt(pCtx->hEnc, &fmt)) {
        ALOGE("%s: Failed to s_fmt", __func__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    memcpy(&pCtx->inbufGeometry, bufferConf, sizeof(pCtx->inbufGeometry));

EXIT:
    return ret;
}

/*
 * [Encoder Buffer OPS] Get Geometry (Dst)
 */
static ExynosVideoErrorType MFC_Encoder_Get_Geometry_Outbuf(
    void *pHandle,
    ExynosVideoGeometry *bufferConf)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    struct v4l2_format fmt;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (bufferConf == NULL) {
        ALOGE("%s: Buffer geometry must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    memset(&fmt, 0, sizeof(fmt));

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if (exynos_v4l2_g_fmt(pCtx->hEnc, &fmt)) {
        ALOGE("%s: Failed to g_fmt", __func__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    bufferConf->nSizeImage = fmt.fmt.pix_mp.plane_fmt[0].sizeimage;

EXIT:
    return ret;
}

/*
 * [Encoder Buffer OPS] Setup (Src)
 */
static ExynosVideoErrorType MFC_Encoder_Setup_Inbuf(
    void *pHandle,
    unsigned int nBufferCount)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;
    ExynosVideoPlane      *pVideoPlane;

    struct v4l2_requestbuffers req;
    struct v4l2_buffer buf;
    struct v4l2_plane planes[VIDEO_ENCODER_INBUF_PLANES];
    int i, j;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (nBufferCount == 0) {
        ALOGE("%s: Buffer count must be greater than 0", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    memset(&req, 0, sizeof(req));

    req.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    req.count = nBufferCount;

    if (pCtx->bShareInbuf == VIDEO_TRUE)
        req.memory = V4L2_MEMORY_USERPTR;
    else
        req.memory = V4L2_MEMORY_MMAP;

    if (exynos_v4l2_reqbufs(pCtx->hEnc, &req)) {
        ALOGE("Failed to require buffer");
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    pCtx->nInbufs = (int)req.count;

    pCtx->pInbuf = malloc(sizeof(*pCtx->pInbuf) * pCtx->nInbufs);
    if (pCtx->pInbuf == NULL) {
        ALOGE("Failed to allocate input buffer context");
        ret = VIDEO_ERROR_NOMEM;
        goto EXIT;
    }

    memset(&buf, 0, sizeof(buf));

    if (pCtx->bShareInbuf == VIDEO_FALSE) {
        buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.m.planes = planes;
        buf.length = VIDEO_ENCODER_INBUF_PLANES;

        for (i = 0; i < pCtx->nInbufs; i++) {
            buf.index = i;
            if (exynos_v4l2_querybuf(pCtx->hEnc, &buf)) {
                ALOGE("%s: Failed to querybuf", __func__);
                ret = VIDEO_ERROR_APIFAIL;
                goto EXIT;
            }

            for (j = 0; j < VIDEO_ENCODER_INBUF_PLANES; j++) {
                pVideoPlane = &pCtx->pInbuf[i].planes[j];
                pVideoPlane->addr = mmap(NULL,
                        buf.m.planes[j].length, PROT_READ | PROT_WRITE,
                        MAP_SHARED, pCtx->hEnc, buf.m.planes[j].m.mem_offset);

                if (pVideoPlane->addr == MAP_FAILED) {
                    ret = VIDEO_ERROR_MAPFAIL;
                    goto EXIT;
                }

                pVideoPlane->allocSize = buf.m.planes[j].length;
                pVideoPlane->dataSize = 0;
            }

            pCtx->pInbuf[i].pGeometry = &pCtx->inbufGeometry;
            pCtx->pInbuf[i].bQueued = VIDEO_FALSE;

        }
    } else {
        for (i = 0; i < pCtx->nInbufs; i++) {
            pCtx->pInbuf[i].pGeometry = &pCtx->inbufGeometry;
            pCtx->pInbuf[i].bQueued = VIDEO_FALSE;
        }
    }
    return ret;

EXIT:
    if (pCtx->bShareInbuf == VIDEO_FALSE) {
        for (i = 0; i < pCtx->nInbufs; i++) {
            for (j = 0; j < VIDEO_ENCODER_INBUF_PLANES; j++) {
                pVideoPlane = &pCtx->pInbuf[i].planes[j];
                if (pVideoPlane->addr == MAP_FAILED) {
                    pVideoPlane->addr = NULL;
                    break;
                }
            }
            munmap(pVideoPlane->addr, pVideoPlane->allocSize);
            pVideoPlane->allocSize = 0;
            pVideoPlane->dataSize = 0;

            pCtx->pInbuf[i].pGeometry = NULL;
            pCtx->pInbuf[i].bQueued = VIDEO_FALSE;
        }
    } else {
        for (i = 0; i < pCtx->nInbufs; i++) {
            pCtx->pInbuf[i].pGeometry = NULL;
            pCtx->pInbuf[i].bQueued = VIDEO_FALSE;
        }
    }

    return ret;
}

/*
 * [Encoder Buffer OPS] Setup (Dst)
 */
static ExynosVideoErrorType MFC_Encoder_Setup_Outbuf(
    void *pHandle,
    unsigned int nBufferCount)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;
    ExynosVideoPlane      *pVideoPlane;

    struct v4l2_requestbuffers req;
    struct v4l2_buffer buf;
    struct v4l2_plane planes[VIDEO_ENCODER_OUTBUF_PLANES];
    int i, j;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (nBufferCount == 0) {
        ALOGE("%s: Buffer count must be greater than 0", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    memset(&req, 0, sizeof(req));

    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    req.count = nBufferCount;

    if (pCtx->bShareOutbuf == VIDEO_TRUE)
        req.memory = V4L2_MEMORY_USERPTR;
    else
        req.memory = V4L2_MEMORY_MMAP;

    if (exynos_v4l2_reqbufs(pCtx->hEnc, &req)) {
        ALOGE("%s: Failed to reqbuf", __func__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    pCtx->nOutbufs = req.count;

    pCtx->pOutbuf = malloc(sizeof(*pCtx->pOutbuf) * pCtx->nOutbufs);
    if (pCtx->pOutbuf == NULL) {
        ALOGE("Failed to allocate output buffer context");
        ret = VIDEO_ERROR_NOMEM;
        goto EXIT;
    }

    memset(&buf, 0, sizeof(buf));

    if (pCtx->bShareOutbuf == VIDEO_FALSE) {
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.m.planes = planes;
        buf.length = VIDEO_ENCODER_OUTBUF_PLANES;

        for (i = 0; i < pCtx->nOutbufs; i++) {
            buf.index = i;
            if (exynos_v4l2_querybuf(pCtx->hEnc, &buf)) {
                ALOGE("%s: Failed to querybuf", __func__);
                ret = VIDEO_ERROR_APIFAIL;
                goto EXIT;
            }

            for (j = 0; j < VIDEO_ENCODER_OUTBUF_PLANES; j++) {
                pVideoPlane = &pCtx->pOutbuf[i].planes[j];
                pVideoPlane->addr = mmap(NULL,
                        buf.m.planes[j].length, PROT_READ | PROT_WRITE,
                        MAP_SHARED, pCtx->hEnc, buf.m.planes[j].m.mem_offset);

                if (pVideoPlane->addr == MAP_FAILED) {
                    ret = VIDEO_ERROR_MAPFAIL;
                    goto EXIT;
                }

                pVideoPlane->allocSize = buf.m.planes[j].length;
                pVideoPlane->dataSize = 0;
            }

            pCtx->pOutbuf[i].pGeometry = &pCtx->outbufGeometry;
            pCtx->pOutbuf[i].bQueued = VIDEO_FALSE;
        }
    } else {
        pCtx->pOutbuf[i].pGeometry = &pCtx->outbufGeometry;
        pCtx->pOutbuf[i].bQueued = VIDEO_FALSE;
    }

    return ret;

EXIT:
    if (pCtx->bShareOutbuf == VIDEO_FALSE) {
        for (i = 0; i < pCtx->nOutbufs; i++) {
            for (j = 0; j < VIDEO_ENCODER_OUTBUF_PLANES; j++) {
                pVideoPlane = &pCtx->pOutbuf[i].planes[j];
                if (pVideoPlane->addr == MAP_FAILED) {
                    pVideoPlane->addr = NULL;
                    break;
                }

                munmap(pVideoPlane->addr, pVideoPlane->allocSize);
                pVideoPlane->allocSize = 0;
                pVideoPlane->dataSize = 0;
            }

            pCtx->pOutbuf[i].pGeometry = NULL;
            pCtx->pOutbuf[i].bQueued = VIDEO_FALSE;
        }
    } else {
        pCtx->pOutbuf[i].pGeometry = NULL;
        pCtx->pOutbuf[i].bQueued = VIDEO_FALSE;
    }

    return ret;
}

/*
 * [Encoder Buffer OPS] Run (src)
 */
static ExynosVideoErrorType MFC_Encoder_Run_Inbuf(void *pHandle)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_streamon(pCtx->hEnc, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)) {
        ALOGE("%s: Failed to streamon for input buffer", __func__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder Buffer OPS] Run (Dst)
 */
static ExynosVideoErrorType MFC_Encoder_Run_Outbuf(void *pHandle)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_streamon(pCtx->hEnc, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)) {
        ALOGE("%s: Failed to streamon for output buffer", __func__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder Buffer OPS] Stop (Src)
 */
static ExynosVideoErrorType MFC_Encoder_Stop_Inbuf(void *pHandle)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_streamoff(pCtx->hEnc, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE)) {
        ALOGE("%s: Failed to streamoff for input buffer", __func__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder Buffer OPS] Stop (Dst)
 */
static ExynosVideoErrorType MFC_Encoder_Stop_Outbuf(void *pHandle)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_streamoff(pCtx->hEnc, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)) {
        ALOGE("%s: Failed to streamoff for output buffer", __func__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder Buffer OPS] Wait (Src)
 */
static ExynosVideoErrorType MFC_Encoder_Wait_Inbuf(void *pHandle)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    struct pollfd poll_events;
    int poll_state;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    poll_events.fd = pCtx->hEnc;
    poll_events.events = POLLOUT | POLLERR;
    poll_events.revents = 0;

    do {
        poll_state = poll((struct pollfd*)&poll_events, 1, VIDEO_ENCODER_POLL_TIMEOUT);
        if (poll_state > 0) {
            if (poll_events.revents & POLLOUT) {
                break;
            } else {
                ALOGE("%s: Poll return error", __func__);
                ret = VIDEO_ERROR_POLL;
                break;
            }
        } else if (poll_state < 0) {
            ALOGE("%s: Poll state error", __func__);
            ret = VIDEO_ERROR_POLL;
            break;
        }
    } while (poll_state == 0);

EXIT:
    return ret;
}

/*
 * [Encoder Buffer OPS] Wait (Dst)
 */
static ExynosVideoErrorType MFC_Encoder_Wait_Outbuf(void *pHandle)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    struct pollfd poll_events;
    int poll_state;
    int bframe_count = 0; // FIXME

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    poll_events.fd = pCtx->hEnc;
    poll_events.events = POLLIN | POLLERR;
    poll_events.revents = 0;

    do {
        poll_state = poll((struct pollfd*)&poll_events, 1, VIDEO_ENCODER_POLL_TIMEOUT);
        if (poll_state > 0) {
            if (poll_events.revents & POLLIN) {
                break;
            } else {
                ALOGE("%s: Poll return error", __func__);
                ret = VIDEO_ERROR_POLL;
                break;
            }
        } else if (poll_state < 0) {
            ALOGE("%s: Poll state error", __func__);
            ret = VIDEO_ERROR_POLL;
            break;
        } else {
            bframe_count++; // FIXME
        }
    } while (poll_state == 0 && bframe_count < 5); // FIXME

EXIT:
    return ret;
}

/*
 * [Encoder Buffer OPS] Find (Src)
 */
static int MFC_Encoder_Find_Inbuf(
    void *pHandle,
    unsigned char *pBuffer)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    int nIndex = -1;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        nIndex = -1;
        goto EXIT;
    }

    if (pBuffer == NULL) {
        for (nIndex = 0; nIndex < pCtx->nInbufs; nIndex++) {
            if (pCtx->pInbuf[nIndex].bQueued == VIDEO_FALSE)
                break;
        }
    } else {
        for (nIndex = 0; nIndex < pCtx->nInbufs; nIndex++) {
            if (pCtx->pInbuf[nIndex].planes[0].addr == pBuffer)
                break;
        }
    }

    if (nIndex == pCtx->nInbufs) {
        nIndex = -1;
        goto EXIT;
    }

EXIT:
    return nIndex;
}

/*
 * [Encoder Buffer OPS] Find (Dst)
 */
static int MFC_Encoder_Find_Outbuf(
    void *pHandle,
    unsigned char *pBuffer)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    int nIndex = -1;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        nIndex = -1;
        goto EXIT;
    }

    if (pBuffer == NULL) {
        for (nIndex = 0; nIndex < pCtx->nOutbufs; nIndex++) {
            if (pCtx->pOutbuf[nIndex].bQueued == VIDEO_FALSE)
                break;
        }
    } else {
        for (nIndex = 0; nIndex < pCtx->nOutbufs; nIndex++) {
            if (pCtx->pOutbuf[nIndex].planes[0].addr == pBuffer)
                break;
        }
    }

    if (nIndex == pCtx->nOutbufs) {
        nIndex = -1;
        goto EXIT;
    }

EXIT:
    return nIndex;
}

/*
 * [Encoder Buffer OPS] Enqueue (Src)
 */
static ExynosVideoErrorType MFC_Encoder_Enqueue_Inbuf(
    void *pHandle,
    unsigned char *pBuffer[],
    unsigned int   dataSize[],
    int            nPlanes,
    void          *pPrivate)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    struct v4l2_plane planes[VIDEO_ENCODER_INBUF_PLANES];
    struct v4l2_buffer buf;
    int index, i;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    memset(&buf, 0, sizeof(buf));

    buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    buf.m.planes = planes;
    buf.length = VIDEO_ENCODER_INBUF_PLANES;

    if ((pCtx->bShareInbuf == VIDEO_TRUE) || (pBuffer == NULL))
       index = MFC_Encoder_Find_Inbuf(pCtx, NULL);
    else
       index = MFC_Encoder_Find_Inbuf(pCtx, pBuffer[0]);

    if (index == -1) {
        ALOGE("%s: Failed to find input buffer", __func__);
        ret = VIDEO_ERROR_NOBUFFERS;
        goto EXIT;
    }

    buf.index = index;

    if (pCtx->bShareInbuf == VIDEO_TRUE) {
        buf.memory = V4L2_MEMORY_USERPTR;
        for (i = 0; i < nPlanes; i++) {
            buf.m.planes[i].m.userptr = (unsigned long)pBuffer[i];
            buf.m.planes[i].length = dataSize[i];
            buf.m.planes[i].bytesused = dataSize[i];
            pCtx->pInbuf[buf.index].planes[i].addr = pBuffer[i];
        }
    } else {
        buf.memory = V4L2_MEMORY_MMAP;
        for (i = 0; i < nPlanes; i++)
            buf.m.planes[i].bytesused = dataSize[i];
    }

    if (exynos_v4l2_qbuf(pCtx->hEnc, &buf)) {
        ALOGE("%s: Failed to enqueue input buffer", __func__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    pCtx->pInbuf[buf.index].pPrivate = pPrivate;
    pCtx->pInbuf[buf.index].bQueued = VIDEO_TRUE;

EXIT:
    return ret;
}

/*
 * [Encoder Buffer OPS] Enqueue (Dst)
 */
static ExynosVideoErrorType MFC_Encoder_Enqueue_Outbuf(
    void *pHandle,
    unsigned char *pBuffer[],
    unsigned int   dataSize[],
    int            nPlanes,
    void          *pPrivate)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    struct v4l2_plane planes[VIDEO_ENCODER_OUTBUF_PLANES];
    struct v4l2_buffer buf;
    int i, index;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    memset(&buf, 0, sizeof(buf));

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    buf.m.planes = planes;
    buf.length = VIDEO_ENCODER_OUTBUF_PLANES;

    if ((pCtx->bShareOutbuf == VIDEO_TRUE) || (pBuffer == NULL))
        index = MFC_Encoder_Find_Outbuf(pCtx, NULL);
    else
        index = MFC_Encoder_Find_Outbuf(pCtx, pBuffer[0]);


    if (index == -1) {
        ret = VIDEO_ERROR_NOBUFFERS;
        goto EXIT;
    }

    buf.index = index;

    if (pCtx->bShareOutbuf == VIDEO_TRUE) {
        buf.memory = V4L2_MEMORY_USERPTR;
        for (i = 0; i < nPlanes; i++) {
            buf.m.planes[i].m.userptr = (unsigned long)pBuffer[i];
            buf.m.planes[i].length = dataSize[i];
            buf.m.planes[i].bytesused = dataSize[i];
            pCtx->pOutbuf[buf.index].planes[i].addr = pBuffer[i];
        }
    } else {
        buf.memory = V4L2_MEMORY_MMAP;
    }

    if (exynos_v4l2_qbuf(pCtx->hEnc, &buf)) {
        ALOGE("%s: Failed to enqueue output buffer", __func__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    pCtx->pOutbuf[buf.index].pPrivate = pPrivate;
    pCtx->pOutbuf[buf.index].bQueued = VIDEO_TRUE;

EXIT:
    return ret;
}

/*
 * [Encoder Buffer OPS] Dequeue (Src)
 */
static ExynosVideoBuffer *MFC_Encoder_Dequeue_Inbuf(void *pHandle)
{
    ExynosVideoEncContext *pCtx    = (ExynosVideoEncContext *)pHandle;
    ExynosVideoBuffer     *pOutbuf = NULL;

    struct v4l2_buffer buf;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        pOutbuf = NULL;
        goto EXIT;
    }

    memset(&buf, 0, sizeof(buf));

    buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;

    if (pCtx->bShareInbuf == VIDEO_TRUE)
        buf.memory = V4L2_MEMORY_USERPTR;
    else
        buf.memory = V4L2_MEMORY_MMAP;

    if (exynos_v4l2_dqbuf(pCtx->hEnc, &buf) == 0) {
        pCtx->pInbuf[buf.index].bQueued = VIDEO_FALSE;
        pOutbuf = &pCtx->pInbuf[buf.index];
    } else {
        ALOGE("%s: Failed to dequeue input buffer", __func__);
        pOutbuf = NULL;
        goto EXIT;
    }

EXIT:
    return pOutbuf;
}

/*
 * [Encoder Buffer OPS] Dequeue (Dst)
 */
static ExynosVideoBuffer *MFC_Encoder_Dequeue_Outbuf(void *pHandle)
{
    ExynosVideoEncContext *pCtx    = (ExynosVideoEncContext *)pHandle;
    ExynosVideoBuffer     *pOutbuf = NULL;

    struct v4l2_buffer buf;
    struct v4l2_plane planes[2];
    int value;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        pOutbuf = NULL;
        goto EXIT;
    }

    if (MFC_Encoder_Wait_Outbuf(pCtx) != VIDEO_ERROR_NONE) {
        ALOGE("%s: Failed to poll for output buffer", __func__);
        pOutbuf = NULL;
        goto EXIT;
    }

    memset(&buf, 0, sizeof(buf));

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    buf.m.planes = planes;
    buf.length = 1;

    if (pCtx->bShareOutbuf == VIDEO_TRUE)
        buf.memory = V4L2_MEMORY_USERPTR;
    else
        buf.memory = V4L2_MEMORY_MMAP;

    /* no error case for output buffer dequeue in encoder */
    exynos_v4l2_dqbuf(pCtx->hEnc, &buf);

    pOutbuf = &pCtx->pOutbuf[buf.index];
    pOutbuf->planes[0].dataSize = buf.m.planes[0].bytesused;

    switch (buf.flags & (0x7 << 3)) {
    case V4L2_BUF_FLAG_KEYFRAME:
        pOutbuf->frameType = VIDEO_FRAME_I;
        break;
    case V4L2_BUF_FLAG_PFRAME:
        pOutbuf->frameType = VIDEO_FRAME_P;
        break;
    case V4L2_BUF_FLAG_BFRAME:
        pOutbuf->frameType = VIDEO_FRAME_B;
        break;
    default:
        ALOGI("%s: encoded frame type is = %d",__func__, (buf.flags & (0x7 << 3)));
        pOutbuf->frameType = VIDEO_FRAME_OTHERS;
        break;
    };

    pOutbuf->bQueued = VIDEO_FALSE;

EXIT:
    return pOutbuf;
}

/*
 * [Encoder Buffer OPS] Enqueue All (Dst)
 */
static ExynosVideoErrorType MFC_Encoder_Enqueue_All_Outbuf(void *pHandle)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    int i;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    for (i = 0; i < pCtx->nOutbufs; i++)
        MFC_Encoder_Enqueue_Outbuf(pCtx, NULL, NULL, 0, NULL);

EXIT:
    return ret;
}


/*
 * [Encoder OPS] Common
 */
static ExynosVideoEncOps defEncOps = {
    .nSize = 0,
    .Init = MFC_Encoder_Init,
    .Finalize = MFC_Encoder_Finalize,
    .Set_EncParam = MFC_Encoder_Set_EncParam,
    .Set_FrameType = MFC_Encoder_Set_FrameType,
    .Set_FrameRate = MFC_Encoder_Set_FrameRate,
    .Set_BitRate = MFC_Encoder_Set_BitRate,
    .Set_FrameSkip = MFC_Encoder_Set_FrameSkip,
    .Set_IDRPeriod = MFC_Encoder_Set_IDRPeriod,
    .Set_FrameTag = MFC_Encoder_Set_FrameTag,
    .Get_FrameTag = MFC_Encoder_Get_FrameTag,
};

/*
 * [Encoder Buffer OPS] Input
 */
static ExynosVideoEncBufferOps defInbufOps = {
    .nSize = 0,
    .Set_Shareable = MFC_Encoder_Set_Shareable_Inbuf,
    .Get_BufferInfo = MFC_Encoder_Get_BufferInfo_Inbuf,
    .Set_Geometry = MFC_Encoder_Set_Geometry_Inbuf,
    .Get_Geometry = MFC_Encoder_Get_Geometry_Inbuf,
    .Setup = MFC_Encoder_Setup_Inbuf,
    .Run = MFC_Encoder_Run_Inbuf,
    .Stop = MFC_Encoder_Stop_Inbuf,
    .Enqueue = MFC_Encoder_Enqueue_Inbuf,
    .Enqueue_All = NULL,
    .Dequeue = MFC_Encoder_Dequeue_Inbuf,
};

/*
 * [Encoder Buffer OPS] Output
 */
static ExynosVideoEncBufferOps defOutbufOps = {
    .nSize = 0,
    .Set_Shareable = MFC_Encoder_Set_Shareable_Outbuf,
    .Get_BufferInfo = MFC_Encoder_Get_BufferInfo_Outbuf,
    .Set_Geometry = MFC_Encoder_Set_Geometry_Outbuf,
    .Get_Geometry = MFC_Encoder_Get_Geometry_Outbuf,
    .Setup = MFC_Encoder_Setup_Outbuf,
    .Run = MFC_Encoder_Run_Outbuf,
    .Stop = MFC_Encoder_Stop_Outbuf,
    .Enqueue = MFC_Encoder_Enqueue_Outbuf,
    .Enqueue_All = MFC_Encoder_Enqueue_All_Outbuf,
    .Dequeue = MFC_Encoder_Dequeue_Outbuf,
};

int Exynos_Video_Register_Encoder(
    ExynosVideoEncOps *pEncOps,
    ExynosVideoEncBufferOps *pInbufOps,
    ExynosVideoEncBufferOps *pOutbufOps)
{
    ExynosVideoErrorType ret = VIDEO_ERROR_NONE;

    if ((pEncOps == NULL) || (pInbufOps == NULL) || (pOutbufOps == NULL)) {
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    defEncOps.nSize = sizeof(defEncOps);
    defInbufOps.nSize = sizeof(defInbufOps);
    defOutbufOps.nSize = sizeof(defOutbufOps);

    memcpy((char *)pEncOps + sizeof(pEncOps->nSize), (char *)&defEncOps + sizeof(defEncOps.nSize),
            pEncOps->nSize - sizeof(pEncOps->nSize));

    memcpy((char *)pInbufOps + sizeof(pInbufOps->nSize), (char *)&defInbufOps + sizeof(defInbufOps.nSize),
            pInbufOps->nSize - sizeof(pInbufOps->nSize));

    memcpy((char *)pOutbufOps + sizeof(pOutbufOps->nSize), (char *)&defOutbufOps + sizeof(defOutbufOps.nSize),
            pOutbufOps->nSize - sizeof(pOutbufOps->nSize));

EXIT:
    return ret;
}
