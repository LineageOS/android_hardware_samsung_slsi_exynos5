/*
 *
 * Copyright 2010 Samsung Electronics S.LSI Co. LTD
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

/*
 * @file        Exynos_OMX_Mpeg4enc.c
 * @brief
 * @author      Yunji Kim (yunji.kim@samsung.com)
 * @version     1.1.0
 * @history
 *   2010.7.15 : Create
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Exynos_OMX_Macros.h"
#include "Exynos_OMX_Basecomponent.h"
#include "Exynos_OMX_Baseport.h"
#include "Exynos_OMX_Venc.h"
#include "Exynos_OSAL_Semaphore.h"
#include "Exynos_OSAL_Thread.h"
#include "Exynos_OSAL_ETC.h"
#include "Exynos_OSAL_Android.h"
#include "library_register.h"
#include "Exynos_OMX_Mpeg4enc.h"
#include "ExynosVideoApi.h"
#include "csc.h"

#undef  EXYNOS_LOG_TAG
#define EXYNOS_LOG_TAG    "EXYNOS_MPEG4_ENC"
#define EXYNOS_LOG_OFF
#include "Exynos_OSAL_Log.h"


/* MPEG4 Encoder Supported Levels & profiles */
EXYNOS_OMX_VIDEO_PROFILELEVEL supportedMPEG4ProfileLevels[] ={
    {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level0},
    {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level0b},
    {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level1},
    {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level2},
    {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level3},
    {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level4},
    {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level4a},
    {OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level5},
    {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level0},
    {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level0b},
    {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level1},
    {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level2},
    {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level3},
    {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level4},
    {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level4a},
    {OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level5}};

/* H.263 Encoder Supported Levels & profiles */
EXYNOS_OMX_VIDEO_PROFILELEVEL supportedH263ProfileLevels[] = {
    {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level10},
    {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level20},
    {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level30},
    {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level40},
    {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level45},
    {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level50},
    {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level60},
    {OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level70}};

static OMX_U32 OMXMpeg4ProfileToMFCProfile(OMX_VIDEO_MPEG4PROFILETYPE profile)
{
    OMX_U32 ret;

    switch (profile) {
    case OMX_VIDEO_MPEG4ProfileSimple:
        ret = 0;
        break;
    case OMX_VIDEO_MPEG4ProfileAdvancedSimple:
        ret = 1;
        break;
    default:
        ret = 0;
    };

    return ret;
}

static OMX_U32 OMXMpeg4LevelToMFCLevel(OMX_VIDEO_MPEG4LEVELTYPE level)
{
    OMX_U32 ret;

    switch (level) {
    case OMX_VIDEO_MPEG4Level0:
        ret = 0;
        break;
    case OMX_VIDEO_MPEG4Level0b:
        ret = 1;
        break;
    case OMX_VIDEO_MPEG4Level1:
        ret = 2;
        break;
    case OMX_VIDEO_MPEG4Level2:
        ret = 3;
        break;
    case OMX_VIDEO_MPEG4Level3:
        ret = 4;
        break;
    case OMX_VIDEO_MPEG4Level4:
    case OMX_VIDEO_MPEG4Level4a:
        ret = 6;
        break;
    case OMX_VIDEO_MPEG4Level5:
        ret = 7;
        break;
    default:
        ret = 0;
    };

    return ret;
}

static void Print_Mpeg4Enc_Param(ExynosVideoEncParam *pEncParam)
{
    ExynosVideoEncCommonParam *pCommonParam = &pEncParam->commonParam;
    ExynosVideoEncMpeg4Param  *pMpeg4Param  = &pEncParam->codecParam.mpeg4;

    /* common parameters */
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "SourceWidth             : %d", pCommonParam->SourceWidth);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "SourceHeight            : %d", pCommonParam->SourceHeight);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "IDRPeriod               : %d", pCommonParam->IDRPeriod);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "SliceMode               : %d", pCommonParam->SliceMode);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "RandomIntraMBRefresh    : %d", pCommonParam->RandomIntraMBRefresh);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Bitrate                 : %d", pCommonParam->Bitrate);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "FrameQp                 : %d", pCommonParam->FrameQp);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "FrameQp_P               : %d", pCommonParam->FrameQp_P);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "QSCodeMax               : %d", pCommonParam->QSCodeMax);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "QSCodeMin               : %d", pCommonParam->QSCodeMin);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "PadControlOn            : %d", pCommonParam->PadControlOn);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "LumaPadVal              : %d", pCommonParam->LumaPadVal);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "CbPadVal                : %d", pCommonParam->CbPadVal);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "CrPadVal                : %d", pCommonParam->CrPadVal);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "FrameMap                : %d", pCommonParam->FrameMap);

    /* Mpeg4 specific parameters */
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "ProfileIDC              : %d", pMpeg4Param->ProfileIDC);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "LevelIDC                : %d", pMpeg4Param->LevelIDC);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "FrameQp_B               : %d", pMpeg4Param->FrameQp_B);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "TimeIncreamentRes       : %d", pMpeg4Param->TimeIncreamentRes);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "VopTimeIncreament       : %d", pMpeg4Param->VopTimeIncreament);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "SliceArgument           : %d", pMpeg4Param->SliceArgument);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "NumberBFrames           : %d", pMpeg4Param->NumberBFrames);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "DisableQpelME           : %d", pMpeg4Param->DisableQpelME);

    /* rate control related parameters */
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "EnableFRMRateControl    : %d", pCommonParam->EnableFRMRateControl);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "EnableMBRateControl     : %d", pCommonParam->EnableMBRateControl);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "CBRPeriodRf             : %d", pCommonParam->CBRPeriodRf);
}

static void Print_H263Enc_Param(ExynosVideoEncParam *pEncParam)
{
    ExynosVideoEncCommonParam *pCommonParam = &pEncParam->commonParam;
    ExynosVideoEncH263Param   *pH263Param   = &pEncParam->codecParam.h263;

    /* common parameters */
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "SourceWidth             : %d", pCommonParam->SourceWidth);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "SourceHeight            : %d", pCommonParam->SourceHeight);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "IDRPeriod               : %d", pCommonParam->IDRPeriod);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "SliceMode               : %d", pCommonParam->SliceMode);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "RandomIntraMBRefresh    : %d", pCommonParam->RandomIntraMBRefresh);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Bitrate                 : %d", pCommonParam->Bitrate);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "FrameQp                 : %d", pCommonParam->FrameQp);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "FrameQp_P               : %d", pCommonParam->FrameQp_P);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "QSCodeMax               : %d", pCommonParam->QSCodeMax);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "QSCodeMin               : %d", pCommonParam->QSCodeMin);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "PadControlOn            : %d", pCommonParam->PadControlOn);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "LumaPadVal              : %d", pCommonParam->LumaPadVal);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "CbPadVal                : %d", pCommonParam->CbPadVal);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "CrPadVal                : %d", pCommonParam->CrPadVal);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "FrameMap                : %d", pCommonParam->FrameMap);

    /* H263 specific parameters */
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "FrameRate               : %d", pH263Param->FrameRate);

    /* rate control related parameters */
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "EnableFRMRateControl    : %d", pCommonParam->EnableFRMRateControl);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "EnableMBRateControl     : %d", pCommonParam->EnableMBRateControl);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "CBRPeriodRf             : %d", pCommonParam->CBRPeriodRf);
}

static void Set_Mpeg4Enc_Param(EXYNOS_OMX_BASECOMPONENT *pExynosComponent)
{
    EXYNOS_OMX_BASEPORT           *pExynosInputPort  = NULL;
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc         = NULL;
    EXYNOS_MPEG4ENC_HANDLE        *pMpeg4Enc         = NULL;

    ExynosVideoEncParam       *pEncParam    = NULL;
    ExynosVideoEncCommonParam *pCommonParam = NULL;
    ExynosVideoEncMpeg4Param  *pMpeg4Param  = NULL;

    pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    pMpeg4Enc = pVideoEnc->hCodecHandle;
    pExynosInputPort  = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

    pEncParam    = &pVideoEnc->encParam;
    pCommonParam = &pEncParam->commonParam;
    pMpeg4Param  = &pEncParam->codecParam.mpeg4;
    pEncParam->eCompressionFormat = VIDEO_CODING_MPEG4;
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "eCompressionFormat: %d", pEncParam->eCompressionFormat);

    /* common parameters */
    pCommonParam->SourceWidth  = pExynosOutputPort->portDefinition.format.video.nFrameWidth;
    pCommonParam->SourceHeight = pExynosOutputPort->portDefinition.format.video.nFrameHeight;
    pCommonParam->IDRPeriod    = pMpeg4Enc->mpeg4Component[OUTPUT_PORT_INDEX].nPFrames + 1;
    pCommonParam->SliceMode    = 0;
    pCommonParam->RandomIntraMBRefresh = 0;
    pCommonParam->Bitrate      = pExynosOutputPort->portDefinition.format.video.nBitrate;
    pCommonParam->FrameQp      = pVideoEnc->quantization.nQpI;
    pCommonParam->FrameQp_P    = pVideoEnc->quantization.nQpP;
    pCommonParam->QSCodeMax    = 30;
    pCommonParam->QSCodeMin    = 10;
    pCommonParam->PadControlOn = 0; /* 0: Use boundary pixel, 1: Use the below setting value */
    pCommonParam->LumaPadVal   = 0;
    pCommonParam->CbPadVal     = 0;
    pCommonParam->CrPadVal     = 0;

    switch ((EXYNOS_OMX_COLOR_FORMATTYPE)pExynosInputPort->portDefinition.format.video.eColorFormat) {
    case OMX_SEC_COLOR_FormatNV12LPhysicalAddress:
    case OMX_SEC_COLOR_FormatNV12LVirtualAddress:
    case OMX_COLOR_FormatYUV420SemiPlanar:
    case OMX_COLOR_FormatYUV420Planar:
#ifdef USE_METADATABUFFERTYPE
    case OMX_COLOR_FormatAndroidOpaque:
#endif
        pCommonParam->FrameMap = VIDEO_COLORFORMAT_NV12;
        break;
    case OMX_SEC_COLOR_FormatNV12TPhysicalAddress:
    case OMX_SEC_COLOR_FormatNV12Tiled:
        pCommonParam->FrameMap = VIDEO_COLORFORMAT_NV12_TILED;
        break;
    case OMX_SEC_COLOR_FormatNV21LPhysicalAddress:
    case OMX_SEC_COLOR_FormatNV21Linear:
        pCommonParam->FrameMap = VIDEO_COLORFORMAT_NV21;
        break;
    default:
        pCommonParam->FrameMap = VIDEO_COLORFORMAT_NV12_TILED;
        break;
    }

    /* Mpeg4 specific parameters */
    pMpeg4Param->ProfileIDC = OMXMpeg4ProfileToMFCProfile(pMpeg4Enc->mpeg4Component[OUTPUT_PORT_INDEX].eProfile);
    pMpeg4Param->LevelIDC   = OMXMpeg4LevelToMFCLevel(pMpeg4Enc->mpeg4Component[OUTPUT_PORT_INDEX].eLevel);
    pMpeg4Param->FrameQp_B  = pVideoEnc->quantization.nQpB;
    pMpeg4Param->TimeIncreamentRes = (pExynosInputPort->portDefinition.format.video.xFramerate) >> 16;
    pMpeg4Param->VopTimeIncreament = 1;
    pMpeg4Param->SliceArgument = 0; /* MB number or byte number */
    pMpeg4Param->NumberBFrames = 0; /* 0(not used) ~ 2 */
    pMpeg4Param->DisableQpelME = 1;

    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pVideoEnc->eControlRate[OUTPUT_PORT_INDEX]: 0x%x", pVideoEnc->eControlRate[OUTPUT_PORT_INDEX]);
    /* rate control related parameters */
    switch (pVideoEnc->eControlRate[OUTPUT_PORT_INDEX]) {
    case OMX_Video_ControlRateVariable:
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Video Encode VBR");
        pCommonParam->EnableFRMRateControl = 0; /* 0: Disable, 1: Frame level RC */
        pCommonParam->EnableMBRateControl  = 0; /* 0: Disable, 1:MB level RC */
        pCommonParam->CBRPeriodRf          = 100;
        break;
    case OMX_Video_ControlRateConstant:
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Video Encode CBR");
        pCommonParam->EnableFRMRateControl = 1; /* 0: Disable, 1: Frame level RC */
        pCommonParam->EnableMBRateControl  = 1; /* 0: Disable, 1:MB level RC */
        pCommonParam->CBRPeriodRf          = 10;
        break;
    case OMX_Video_ControlRateDisable:
    default: /*Android default */
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Video Encode VBR");
        pCommonParam->EnableFRMRateControl = 0;
        pCommonParam->EnableMBRateControl  = 0;
        pCommonParam->CBRPeriodRf          = 100;
        break;
    }

    Print_Mpeg4Enc_Param(pEncParam);
}

static void Set_H263Enc_Param(EXYNOS_OMX_BASECOMPONENT *pExynosComponent)
{
    EXYNOS_OMX_BASEPORT           *pExynosInputPort  = NULL;
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc         = NULL;
    EXYNOS_MPEG4ENC_HANDLE        *pMpeg4Enc         = NULL;

    ExynosVideoEncParam       *pEncParam    = NULL;
    ExynosVideoEncCommonParam *pCommonParam = NULL;
    ExynosVideoEncH263Param   *pH263Param   = NULL;

    pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    pMpeg4Enc = pVideoEnc->hCodecHandle;
    pExynosInputPort  = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

    pEncParam    = &pVideoEnc->encParam;
    pCommonParam = &pEncParam->commonParam;
    pH263Param   = &pEncParam->codecParam.h263;
    pEncParam->eCompressionFormat = VIDEO_CODING_H263;
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "eCompressionFormat: %d", pEncParam->eCompressionFormat);

    /* common parameters */
    pCommonParam->SourceWidth  = pExynosOutputPort->portDefinition.format.video.nFrameWidth;
    pCommonParam->SourceHeight = pExynosOutputPort->portDefinition.format.video.nFrameHeight;
    pCommonParam->IDRPeriod    = pMpeg4Enc->h263Component[OUTPUT_PORT_INDEX].nPFrames + 1;
    pCommonParam->SliceMode    = 0;
    pCommonParam->RandomIntraMBRefresh = 0;
    pCommonParam->Bitrate      = pExynosOutputPort->portDefinition.format.video.nBitrate;
    pCommonParam->FrameQp      = pVideoEnc->quantization.nQpI;
    pCommonParam->FrameQp_P    = pVideoEnc->quantization.nQpP;
    pCommonParam->QSCodeMax    = 30;
    pCommonParam->QSCodeMin    = 10;
    pCommonParam->PadControlOn = 0; /* 0: Use boundary pixel, 1: Use the below setting value */
    pCommonParam->LumaPadVal   = 0;
    pCommonParam->CbPadVal     = 0;
    pCommonParam->CrPadVal     = 0;

    switch ((EXYNOS_OMX_COLOR_FORMATTYPE)pExynosInputPort->portDefinition.format.video.eColorFormat) {
    case OMX_SEC_COLOR_FormatNV12LPhysicalAddress:
    case OMX_SEC_COLOR_FormatNV12LVirtualAddress:
    case OMX_COLOR_FormatYUV420SemiPlanar:
    case OMX_COLOR_FormatYUV420Planar:
#ifdef USE_METADATABUFFERTYPE
    case OMX_COLOR_FormatAndroidOpaque:
#endif
        pCommonParam->FrameMap = VIDEO_COLORFORMAT_NV12;
        break;
    case OMX_SEC_COLOR_FormatNV12TPhysicalAddress:
    case OMX_SEC_COLOR_FormatNV12Tiled:
        pCommonParam->FrameMap = VIDEO_COLORFORMAT_NV12_TILED;
        break;
    case OMX_SEC_COLOR_FormatNV21LPhysicalAddress:
    case OMX_SEC_COLOR_FormatNV21Linear:
        pCommonParam->FrameMap = VIDEO_COLORFORMAT_NV21;
        break;
    default:
        pCommonParam->FrameMap = VIDEO_COLORFORMAT_NV12_TILED;
        break;
    }

    /* H263 specific parameters */
    pH263Param->FrameRate            = (pExynosInputPort->portDefinition.format.video.xFramerate) >> 16;

    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pVideoEnc->eControlRate[OUTPUT_PORT_INDEX]: 0x%x", pVideoEnc->eControlRate[OUTPUT_PORT_INDEX]);
    /* rate control related parameters */
    switch (pVideoEnc->eControlRate[OUTPUT_PORT_INDEX]) {
    case OMX_Video_ControlRateVariable:
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Video Encode VBR");
        pCommonParam->EnableFRMRateControl = 0; /* 0: Disable, 1: Frame level RC */
        pCommonParam->EnableMBRateControl  = 0; /* 0: Disable, 1:MB level RC */
        pCommonParam->CBRPeriodRf          = 100;
        break;
    case OMX_Video_ControlRateConstant:
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Video Encode CBR");
        pCommonParam->EnableFRMRateControl = 1; /* 0: Disable, 1: Frame level RC */
        pCommonParam->EnableMBRateControl  = 1; /* 0: Disable, 1:MB level RC */
        pCommonParam->CBRPeriodRf          = 10;
        break;
    case OMX_Video_ControlRateDisable:
    default: /*Android default */
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Video Encode VBR");
        pCommonParam->EnableFRMRateControl = 0;
        pCommonParam->EnableMBRateControl  = 0;
        pCommonParam->CBRPeriodRf          = 100;
        break;
    }

    Print_H263Enc_Param(pEncParam);
}

static void Change_Mpeg4Enc_Param(EXYNOS_OMX_BASECOMPONENT *pExynosComponent)
{
    EXYNOS_OMX_BASEPORT           *pExynosInputPort  = NULL;
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc         = NULL;
    EXYNOS_MPEG4ENC_HANDLE        *pMpeg4Enc         = NULL;

    ExynosVideoEncOps         *pEncOps      = NULL;
    ExynosVideoEncParam       *pEncParam    = NULL;
    ExynosVideoEncCommonParam *pCommonParam = NULL;
    ExynosVideoEncMpeg4Param  *pMpeg4Param  = NULL;

    int setParam = 0;

    pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    pMpeg4Enc = pVideoEnc->hCodecHandle;
    pExynosInputPort  = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    pEncOps = pVideoEnc->pEncOps;

    pEncParam    = &pVideoEnc->encParam;
    pCommonParam = &pEncParam->commonParam;
    pMpeg4Param  = &pEncParam->codecParam.mpeg4;

    if (pVideoEnc->IntraRefreshVOP == OMX_TRUE) {
        setParam = VIDEO_FRAME_I;
        pEncOps->Set_FrameType(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle, setParam);
        pVideoEnc->IntraRefreshVOP = OMX_FALSE;
    }
    if (pCommonParam->IDRPeriod != (int)pMpeg4Enc->mpeg4Component[OUTPUT_PORT_INDEX].nPFrames + 1) {
        setParam = pMpeg4Enc->mpeg4Component[OUTPUT_PORT_INDEX].nPFrames + 1;
        pEncOps->Set_IDRPeriod(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle, setParam);
    }
    if (pCommonParam->Bitrate != (int)pExynosOutputPort->portDefinition.format.video.nBitrate) {
        setParam = pExynosOutputPort->portDefinition.format.video.nBitrate;
        pEncOps->Set_BitRate(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle, setParam);
    }
    if (pMpeg4Param->TimeIncreamentRes != (int)((pExynosOutputPort->portDefinition.format.video.xFramerate) >> 16)) {
        setParam = (pExynosInputPort->portDefinition.format.video.xFramerate) >> 16;
        pEncOps->Set_FrameRate(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle, setParam);
    }

    Set_Mpeg4Enc_Param(pExynosComponent);
}

static void Change_H263Enc_Param(EXYNOS_OMX_BASECOMPONENT *pExynosComponent)
{
    EXYNOS_OMX_BASEPORT           *pExynosInputPort  = NULL;
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc         = NULL;
    EXYNOS_MPEG4ENC_HANDLE        *pMpeg4Enc         = NULL;

    ExynosVideoEncOps         *pEncOps      = NULL;
    ExynosVideoEncParam       *pEncParam    = NULL;
    ExynosVideoEncCommonParam *pCommonParam = NULL;
    ExynosVideoEncH263Param   *pH263Param   = NULL;

    int setParam = 0;

    pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    pMpeg4Enc = pVideoEnc->hCodecHandle;
    pExynosInputPort  = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    pEncOps = pVideoEnc->pEncOps;

    pEncParam    = &pVideoEnc->encParam;
    pCommonParam = &pEncParam->commonParam;
    pH263Param   = &pEncParam->codecParam.h263;

    if (pVideoEnc->IntraRefreshVOP == OMX_TRUE) {
        setParam = VIDEO_FRAME_I;
        pEncOps->Set_FrameType(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle, setParam);
        pVideoEnc->IntraRefreshVOP = OMX_FALSE;
    }
    if (pCommonParam->IDRPeriod != (int)pMpeg4Enc->h263Component[OUTPUT_PORT_INDEX].nPFrames + 1) {
        setParam = pMpeg4Enc->h263Component[OUTPUT_PORT_INDEX].nPFrames + 1;
        pEncOps->Set_IDRPeriod(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle, setParam);
    }
    if (pCommonParam->Bitrate != (int)pExynosOutputPort->portDefinition.format.video.nBitrate) {
        setParam = pExynosOutputPort->portDefinition.format.video.nBitrate;
        pEncOps->Set_BitRate(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle, setParam);
    }
    if (pH263Param->FrameRate != (int)((pExynosOutputPort->portDefinition.format.video.xFramerate) >> 16)) {
        setParam = (pExynosInputPort->portDefinition.format.video.xFramerate) >> 16;
        pEncOps->Set_FrameRate(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle, setParam);
    }

    Set_H263Enc_Param(pExynosComponent);
}

OMX_ERRORTYPE Exynos_Mpeg4Enc_GetParameter(
    OMX_IN    OMX_HANDLETYPE hComponent,
    OMX_IN    OMX_INDEXTYPE  nParamIndex,
    OMX_INOUT OMX_PTR        pComponentParameterStructure)
{
    OMX_ERRORTYPE             ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent    = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;

    FunctionIn();

    if (hComponent == NULL || pComponentParameterStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }
    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pExynosComponent->currentState == OMX_StateInvalid ) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    switch (nParamIndex) {
    case OMX_IndexParamVideoMpeg4:
    {
        OMX_VIDEO_PARAM_MPEG4TYPE *pDstMpeg4Component = (OMX_VIDEO_PARAM_MPEG4TYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_MPEG4TYPE *pSrcMpeg4Component = NULL;
        EXYNOS_MPEG4ENC_HANDLE    *pMpeg4Enc = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pDstMpeg4Component, sizeof(OMX_VIDEO_PARAM_MPEG4TYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstMpeg4Component->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        pSrcMpeg4Component = &pMpeg4Enc->mpeg4Component[pDstMpeg4Component->nPortIndex];

        Exynos_OSAL_Memcpy(pDstMpeg4Component, pSrcMpeg4Component, sizeof(OMX_VIDEO_PARAM_MPEG4TYPE));
    }
        break;
    case OMX_IndexParamVideoH263:
    {
        OMX_VIDEO_PARAM_H263TYPE  *pDstH263Component = (OMX_VIDEO_PARAM_H263TYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_H263TYPE  *pSrcH263Component = NULL;
        EXYNOS_MPEG4ENC_HANDLE    *pMpeg4Enc = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pDstH263Component, sizeof(OMX_VIDEO_PARAM_H263TYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstH263Component->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        pSrcH263Component = &pMpeg4Enc->h263Component[pDstH263Component->nPortIndex];

        Exynos_OSAL_Memcpy(pDstH263Component, pSrcH263Component, sizeof(OMX_VIDEO_PARAM_H263TYPE));
    }
        break;
    case OMX_IndexParamStandardComponentRole:
    {
        OMX_S32 codecType;
        OMX_PARAM_COMPONENTROLETYPE *pComponentRole = (OMX_PARAM_COMPONENTROLETYPE *)pComponentParameterStructure;

        ret = Exynos_OMX_Check_SizeVersion(pComponentRole, sizeof(OMX_PARAM_COMPONENTROLETYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        codecType = ((EXYNOS_MPEG4ENC_HANDLE *)(((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle))->hMFCMpeg4Handle.codecType;
        if (codecType == CODEC_TYPE_MPEG4)
            Exynos_OSAL_Strcpy((char *)pComponentRole->cRole, EXYNOS_OMX_COMPONENT_MPEG4_ENC_ROLE);
        else
            Exynos_OSAL_Strcpy((char *)pComponentRole->cRole, EXYNOS_OMX_COMPONENT_H263_ENC_ROLE);
    }
        break;
    case OMX_IndexParamVideoProfileLevelQuerySupported:
    {
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *pDstProfileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pComponentParameterStructure;
        EXYNOS_OMX_VIDEO_PROFILELEVEL    *pProfileLevel = NULL;
        OMX_U32                           maxProfileLevelNum = 0;
        OMX_S32                           codecType;

        ret = Exynos_OMX_Check_SizeVersion(pDstProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstProfileLevel->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        codecType = ((EXYNOS_MPEG4ENC_HANDLE *)(((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle))->hMFCMpeg4Handle.codecType;
        if (codecType == CODEC_TYPE_MPEG4) {
            pProfileLevel = supportedMPEG4ProfileLevels;
            maxProfileLevelNum = sizeof(supportedMPEG4ProfileLevels) / sizeof(EXYNOS_OMX_VIDEO_PROFILELEVEL);
        } else {
            pProfileLevel = supportedH263ProfileLevels;
            maxProfileLevelNum = sizeof(supportedH263ProfileLevels) / sizeof(EXYNOS_OMX_VIDEO_PROFILELEVEL);
        }

        if (pDstProfileLevel->nProfileIndex >= maxProfileLevelNum) {
            ret = OMX_ErrorNoMore;
            goto EXIT;
        }

        pProfileLevel += pDstProfileLevel->nProfileIndex;
        pDstProfileLevel->eProfile = pProfileLevel->profile;
        pDstProfileLevel->eLevel = pProfileLevel->level;
    }
        break;
    case OMX_IndexParamVideoProfileLevelCurrent:
    {
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *pDstProfileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_MPEG4TYPE        *pSrcMpeg4Component = NULL;
        OMX_VIDEO_PARAM_H263TYPE         *pSrcH263Component = NULL;
        EXYNOS_MPEG4ENC_HANDLE           *pMpeg4Enc = NULL;
        OMX_S32                           codecType;

        ret = Exynos_OMX_Check_SizeVersion(pDstProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstProfileLevel->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        codecType = pMpeg4Enc->hMFCMpeg4Handle.codecType;
        if (codecType == CODEC_TYPE_MPEG4) {
            pSrcMpeg4Component = &pMpeg4Enc->mpeg4Component[pDstProfileLevel->nPortIndex];
            pDstProfileLevel->eProfile = pSrcMpeg4Component->eProfile;
            pDstProfileLevel->eLevel = pSrcMpeg4Component->eLevel;
        } else {
            pSrcH263Component = &pMpeg4Enc->h263Component[pDstProfileLevel->nPortIndex];
            pDstProfileLevel->eProfile = pSrcH263Component->eProfile;
            pDstProfileLevel->eLevel = pSrcH263Component->eLevel;
        }
    }
        break;
    case OMX_IndexParamVideoErrorCorrection:
    {
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pDstErrorCorrectionType = (OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pSrcErrorCorrectionType = NULL;
        EXYNOS_MPEG4ENC_HANDLE              *pMpeg4Enc = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pDstErrorCorrectionType, sizeof(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstErrorCorrectionType->nPortIndex != OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        pSrcErrorCorrectionType = &pMpeg4Enc->errorCorrectionType[OUTPUT_PORT_INDEX];

        pDstErrorCorrectionType->bEnableHEC = pSrcErrorCorrectionType->bEnableHEC;
        pDstErrorCorrectionType->bEnableResync = pSrcErrorCorrectionType->bEnableResync;
        pDstErrorCorrectionType->nResynchMarkerSpacing = pSrcErrorCorrectionType->nResynchMarkerSpacing;
        pDstErrorCorrectionType->bEnableDataPartitioning = pSrcErrorCorrectionType->bEnableDataPartitioning;
        pDstErrorCorrectionType->bEnableRVLC = pSrcErrorCorrectionType->bEnableRVLC;
    }
        break;
    default:
        ret = Exynos_OMX_VideoEncodeGetParameter(hComponent, nParamIndex, pComponentParameterStructure);
        break;
    }
EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_Mpeg4Enc_SetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_IN OMX_PTR        pComponentParameterStructure)
{
    OMX_ERRORTYPE             ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent    = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;

    FunctionIn();

    if (hComponent == NULL || pComponentParameterStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }
    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pExynosComponent->currentState == OMX_StateInvalid ) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    switch (nIndex) {
    case OMX_IndexParamVideoMpeg4:
    {
        OMX_VIDEO_PARAM_MPEG4TYPE *pDstMpeg4Component = NULL;
        OMX_VIDEO_PARAM_MPEG4TYPE *pSrcMpeg4Component = (OMX_VIDEO_PARAM_MPEG4TYPE *)pComponentParameterStructure;
        EXYNOS_MPEG4ENC_HANDLE    *pMpeg4Enc = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pSrcMpeg4Component, sizeof(OMX_VIDEO_PARAM_MPEG4TYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcMpeg4Component->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        pDstMpeg4Component = &pMpeg4Enc->mpeg4Component[pSrcMpeg4Component->nPortIndex];

        Exynos_OSAL_Memcpy(pDstMpeg4Component, pSrcMpeg4Component, sizeof(OMX_VIDEO_PARAM_MPEG4TYPE));
    }
        break;
    case OMX_IndexParamVideoH263:
    {
        OMX_VIDEO_PARAM_H263TYPE *pDstH263Component = NULL;
        OMX_VIDEO_PARAM_H263TYPE *pSrcH263Component = (OMX_VIDEO_PARAM_H263TYPE *)pComponentParameterStructure;
        EXYNOS_MPEG4ENC_HANDLE   *pMpeg4Enc = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pSrcH263Component, sizeof(OMX_VIDEO_PARAM_H263TYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcH263Component->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        pDstH263Component = &pMpeg4Enc->h263Component[pSrcH263Component->nPortIndex];

        Exynos_OSAL_Memcpy(pDstH263Component, pSrcH263Component, sizeof(OMX_VIDEO_PARAM_H263TYPE));
    }
        break;
    case OMX_IndexParamStandardComponentRole:
    {
        OMX_PARAM_COMPONENTROLETYPE *pComponentRole = (OMX_PARAM_COMPONENTROLETYPE*)pComponentParameterStructure;

        ret = Exynos_OMX_Check_SizeVersion(pComponentRole, sizeof(OMX_PARAM_COMPONENTROLETYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if ((pExynosComponent->currentState != OMX_StateLoaded) && (pExynosComponent->currentState != OMX_StateWaitForResources)) {
            ret = OMX_ErrorIncorrectStateOperation;
            goto EXIT;
        }

        if (!Exynos_OSAL_Strcmp((char*)pComponentRole->cRole, EXYNOS_OMX_COMPONENT_MPEG4_ENC_ROLE)) {
            pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX].portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingMPEG4;
            //((EXYNOS_MPEG4ENC_HANDLE *)(((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle))->hMFCMpeg4Handle.codecType = CODEC_TYPE_MPEG4;
        } else if (!Exynos_OSAL_Strcmp((char*)pComponentRole->cRole, EXYNOS_OMX_COMPONENT_H263_ENC_ROLE)) {
            pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX].portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingH263;
            //((EXYNOS_MPEG4ENC_HANDLE *)(((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle))->hMFCMpeg4Handle.codecType = CODEC_TYPE_H263;
        } else {
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }
    }
        break;
    case OMX_IndexParamVideoProfileLevelCurrent:
    {
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *pSrcProfileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_MPEG4TYPE        *pDstMpeg4Component = NULL;
        OMX_VIDEO_PARAM_H263TYPE         *pDstH263Component = NULL;
        EXYNOS_MPEG4ENC_HANDLE           *pMpeg4Enc = NULL;
        OMX_S32                           codecType;

        ret = Exynos_OMX_Check_SizeVersion(pSrcProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcProfileLevel->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        codecType = pMpeg4Enc->hMFCMpeg4Handle.codecType;
        if (codecType == CODEC_TYPE_MPEG4) {
            /*
             * To do: Check validity of profile & level parameters
             */

            pDstMpeg4Component = &pMpeg4Enc->mpeg4Component[pSrcProfileLevel->nPortIndex];
            pDstMpeg4Component->eProfile = pSrcProfileLevel->eProfile;
            pDstMpeg4Component->eLevel = pSrcProfileLevel->eLevel;
        } else {
            /*
             * To do: Check validity of profile & level parameters
             */

            pDstH263Component = &pMpeg4Enc->h263Component[pSrcProfileLevel->nPortIndex];
            pDstH263Component->eProfile = pSrcProfileLevel->eProfile;
            pDstH263Component->eLevel = pSrcProfileLevel->eLevel;
        }
    }
        break;
    case OMX_IndexParamVideoErrorCorrection:
    {
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pSrcErrorCorrectionType = (OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pDstErrorCorrectionType = NULL;
        EXYNOS_MPEG4ENC_HANDLE              *pMpeg4Enc = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pSrcErrorCorrectionType, sizeof(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcErrorCorrectionType->nPortIndex != OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        pDstErrorCorrectionType = &pMpeg4Enc->errorCorrectionType[OUTPUT_PORT_INDEX];

        pDstErrorCorrectionType->bEnableHEC = pSrcErrorCorrectionType->bEnableHEC;
        pDstErrorCorrectionType->bEnableResync = pSrcErrorCorrectionType->bEnableResync;
        pDstErrorCorrectionType->nResynchMarkerSpacing = pSrcErrorCorrectionType->nResynchMarkerSpacing;
        pDstErrorCorrectionType->bEnableDataPartitioning = pSrcErrorCorrectionType->bEnableDataPartitioning;
        pDstErrorCorrectionType->bEnableRVLC = pSrcErrorCorrectionType->bEnableRVLC;
    }
        break;
    default:
        ret = Exynos_OMX_VideoEncodeSetParameter(hComponent, nIndex, pComponentParameterStructure);
        break;
    }
EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_Mpeg4Enc_GetConfig(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_IN OMX_PTR        pComponentConfigStructure)
{
    OMX_ERRORTYPE             ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent    = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;

    FunctionIn();

    if (hComponent == NULL || pComponentConfigStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }
    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pExynosComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    switch (nIndex) {
    default:
        ret = Exynos_OMX_VideoEncodeGetConfig(hComponent, nIndex, pComponentConfigStructure);
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_Mpeg4Enc_SetConfig(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_IN OMX_PTR        pComponentConfigStructure)
{
    OMX_ERRORTYPE                  ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE             *pOMXComponent    = NULL;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc        = NULL;
    EXYNOS_MPEG4ENC_HANDLE        *pMpeg4Enc        = NULL;

    FunctionIn();

    if (hComponent == NULL || pComponentConfigStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }
    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pExynosComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)pVideoEnc->hCodecHandle;

    switch (nIndex) {
    case OMX_IndexConfigVideoIntraPeriod:
    {
        OMX_U32 nPFrames = (*((OMX_U32 *)pComponentConfigStructure)) - 1;

        if (pMpeg4Enc->hMFCMpeg4Handle.codecType == CODEC_TYPE_MPEG4)
            pMpeg4Enc->mpeg4Component[OUTPUT_PORT_INDEX].nPFrames = nPFrames;
        else
            pMpeg4Enc->h263Component[OUTPUT_PORT_INDEX].nPFrames = nPFrames;

        ret = OMX_ErrorNone;
    }
        break;
    default:
        ret = Exynos_OMX_VideoEncodeSetConfig(hComponent, nIndex, pComponentConfigStructure);
        break;
    }

EXIT:
    if (ret == OMX_ErrorNone)
        pVideoEnc->configChange = OMX_TRUE;

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_Mpeg4Enc_GetExtensionIndex(
    OMX_IN  OMX_HANDLETYPE  hComponent,
    OMX_IN  OMX_STRING      cParameterName,
    OMX_OUT OMX_INDEXTYPE  *pIndexType)
{
    OMX_ERRORTYPE             ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent    = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    if ((cParameterName == NULL) || (pIndexType == NULL)) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pExynosComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }
    if (Exynos_OSAL_Strcmp(cParameterName, EXYNOS_INDEX_CONFIG_VIDEO_INTRAPERIOD) == 0) {
        *pIndexType = OMX_IndexConfigVideoIntraPeriod;
        ret = OMX_ErrorNone;
    } else {
        ret = Exynos_OMX_VideoEncodeGetExtensionIndex(hComponent, cParameterName, pIndexType);
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_Mpeg4Enc_ComponentRoleEnum(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_OUT OMX_U8        *cRole,
    OMX_IN  OMX_U32        nIndex)
{
    OMX_ERRORTYPE               ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE          *pOMXComponent    = NULL;
    EXYNOS_OMX_BASECOMPONENT   *pExynosComponent = NULL;
    OMX_S32                     codecType;

    FunctionIn();

    if ((hComponent == NULL) || (cRole == NULL)) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (nIndex != (MAX_COMPONENT_ROLE_NUM - 1)) { /* supports only one role */
        ret = OMX_ErrorNoMore;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }
    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pExynosComponent->currentState == OMX_StateInvalid ) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    codecType = ((EXYNOS_MPEG4ENC_HANDLE *)(((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle))->hMFCMpeg4Handle.codecType;
    if (codecType == CODEC_TYPE_MPEG4)
        Exynos_OSAL_Strcpy((char *)cRole, EXYNOS_OMX_COMPONENT_MPEG4_ENC_ROLE);
    else
        Exynos_OSAL_Strcpy((char *)cRole, EXYNOS_OMX_COMPONENT_H263_ENC_ROLE);

EXIT:
    FunctionOut();

    return ret;
}

static OMX_ERRORTYPE Exynos_Mpeg4Enc_EncodeThread(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE                  ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE             *pOMXComponent    = (OMX_COMPONENTTYPE *)hComponent;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc        = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_MPEG4ENC_HANDLE        *pMpeg4Enc        = (EXYNOS_MPEG4ENC_HANDLE *)pVideoEnc->hCodecHandle;
    ExynosVideoEncOps             *pEncOps          = pVideoEnc->pEncOps;
    ExynosVideoEncBufferOps       *pInbufOps        = pVideoEnc->pInbufOps;
    ExynosVideoEncBufferOps       *pOutbufOps       = pVideoEnc->pOutbufOps;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    while (pVideoEnc->NBEncThread.bExitEncodeThread == OMX_FALSE) {
        Exynos_OSAL_SemaphoreWait(pVideoEnc->NBEncThread.hEncFrameStart);

        if (pVideoEnc->NBEncThread.bExitEncodeThread == OMX_FALSE) {
            if (pVideoEnc->NBEncThread.oneFrameSize > 0) {
                if (pVideoEnc->bInputRun == OMX_FALSE) {
                    if (pInbufOps->Run(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle) != VIDEO_ERROR_NONE) {
                        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to run input buffer");
                        ret = OMX_ErrorInsufficientResources;
                        goto EXIT;
                    }

                    pVideoEnc->bInputRun = OMX_TRUE;
                }

                pVideoEnc->pOutbuf = pOutbufOps->Dequeue(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle);
                pInbufOps->Dequeue(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle);
            }

            pMpeg4Enc->hMFCMpeg4Handle.returnCodec = VIDEO_TRUE;
            Exynos_OSAL_SemaphorePost(pVideoEnc->NBEncThread.hEncFrameEnd);
        }
    }

EXIT:
    FunctionOut();
    Exynos_OSAL_ThreadExit(NULL);

    return ret;
}

/* MFC Init */
OMX_ERRORTYPE Exynos_Mpeg4Enc_Init(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE                  ret              = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc        = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_MPEG4ENC_HANDLE        *pMpeg4Enc        = (EXYNOS_MPEG4ENC_HANDLE *)pVideoEnc->hCodecHandle;
    EXYNOS_OMX_BASEPORT           *pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    ExynosVideoEncOps             *pEncOps          = NULL;
    ExynosVideoEncBufferOps       *pInbufOps        = NULL;
    ExynosVideoEncBufferOps       *pOutbufOps       = NULL;

    OMX_HANDLETYPE                 hMFCHandle       = NULL;
    OMX_S32                        returnCodec      = 0;
    int i;

    ExynosVideoEncParam encParam;
    ExynosVideoBuffer   bufferInfo;
    ExynosVideoGeometry bufferConf;

    CSC_METHOD csc_method = CSC_METHOD_SW;

    FunctionIn();

    pMpeg4Enc->hMFCMpeg4Handle.bConfiguredMFC = OMX_FALSE;
    pExynosComponent->bUseFlagEOF = OMX_FALSE;
    pExynosComponent->bSaveFlagEOS = OMX_FALSE;

    /* alloc ops structure */
    pEncOps    = (ExynosVideoEncOps *)Exynos_OSAL_Malloc(sizeof(*pEncOps));
    pInbufOps  = (ExynosVideoEncBufferOps *)Exynos_OSAL_Malloc(sizeof(*pInbufOps));
    pOutbufOps = (ExynosVideoEncBufferOps *)Exynos_OSAL_Malloc(sizeof(*pOutbufOps));
    if ((pEncOps == NULL) || (pInbufOps == NULL) || (pOutbufOps == NULL)) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to allocate encoder ops buffer");
        ret = OMX_ErrorInsufficientResources;

        if (pEncOps)
            Exynos_OSAL_Free(pEncOps);

        if (pInbufOps)
            Exynos_OSAL_Free(pInbufOps);

        if (pOutbufOps)
            Exynos_OSAL_Free(pOutbufOps);

        goto EXIT;
    }

    pVideoEnc->pEncOps = pEncOps;
    pVideoEnc->pInbufOps = pInbufOps;
    pVideoEnc->pOutbufOps = pOutbufOps;

    /* function pointer mapping */
    pEncOps->nSize = sizeof(*pEncOps);
    pInbufOps->nSize = sizeof(*pInbufOps);
    pOutbufOps->nSize = sizeof(*pOutbufOps);

    Exynos_Video_Register_Encoder(pEncOps, pInbufOps, pOutbufOps);

    /* check mandatory functions for encoder ops */
    if ((pEncOps->Init == NULL) || (pEncOps->Finalize == NULL) ||
        (pEncOps->Set_FrameTag == NULL) || (pEncOps->Get_FrameTag == NULL)) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Mandatory functions must be supplied");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    /* check mandatory functions for buffer ops */
    if ((pInbufOps->Setup == NULL) || (pOutbufOps->Setup == NULL) ||
        (pInbufOps->Run == NULL) || (pOutbufOps->Run == NULL) ||
        (pInbufOps->Stop == NULL) || (pOutbufOps->Stop == NULL) ||
        (pInbufOps->Enqueue == NULL) || (pOutbufOps->Enqueue == NULL) ||
        (pInbufOps->Dequeue == NULL) || (pOutbufOps->Dequeue == NULL)) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Mandatory functions must be supplied");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    /* alloc context, open, querycap */
    pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle = pVideoEnc->pEncOps->Init();
    if (pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle == NULL) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to allocate context buffer");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    if (pMpeg4Enc->hMFCMpeg4Handle.codecType == CODEC_TYPE_MPEG4)
        Set_Mpeg4Enc_Param(pExynosComponent);
    else
        Set_H263Enc_Param(pExynosComponent);

    encParam = pVideoEnc->encParam;
    if (pEncOps->Set_EncParam) {
        if(pEncOps->Set_EncParam(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle, &encParam) != VIDEO_ERROR_NONE) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to set geometry for input buffer");
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }
    }

    /* set input buffer shareable */
    if(pExynosInputPort->portDefinition.format.video.eColorFormat == OMX_SEC_COLOR_FormatNV12LVirtualAddress) {
        if (pInbufOps->Set_Shareable) {
            if (pInbufOps->Set_Shareable(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle) != VIDEO_ERROR_NONE) {
                Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to set input buffer shareable");
                ret = OMX_ErrorInsufficientResources;
                goto EXIT;
            }
        }
    }

    Exynos_OSAL_Memset(&bufferConf, 0, sizeof(bufferConf));

    /* input buffer info: only 3 config values needed */
    bufferConf.nFrameWidth  = pExynosInputPort->portDefinition.format.video.nFrameWidth;
    bufferConf.nFrameHeight = pExynosInputPort->portDefinition.format.video.nFrameHeight;
    bufferConf.eColorFormat = pVideoEnc->encParam.commonParam.FrameMap;

    /* set input buffer geometry */
    if (pInbufOps->Set_Geometry) {
        if (pInbufOps->Set_Geometry(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle, &bufferConf) != VIDEO_ERROR_NONE) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to set geometry for input buffer");
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }
    }

    /* cacheable for input */
    if (pInbufOps->Enable_Cacheable) {
        if (pInbufOps->Enable_Cacheable(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle) != VIDEO_ERROR_NONE)
            Exynos_OSAL_Log(EXYNOS_LOG_WARNING, "Failed to enable cacheable property for input buffer");
    }

    /* setup input buffer */
    if (pInbufOps->Setup(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle, MFC_INPUT_BUFFER_NUM_MAX) != VIDEO_ERROR_NONE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to setup input buffer");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    for (i = 0; i < MFC_INPUT_BUFFER_NUM_MAX; i++) {
         pInbufOps->Get_BufferInfo(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle, i, &bufferInfo);

         pVideoEnc->MFCEncInputBuffer[i].YPhyAddr = NULL;
         pVideoEnc->MFCEncInputBuffer[i].CPhyAddr = NULL;
         pVideoEnc->MFCEncInputBuffer[i].YVirAddr = bufferInfo.planes[0].addr;
         pVideoEnc->MFCEncInputBuffer[i].CVirAddr = bufferInfo.planes[1].addr;
         pVideoEnc->MFCEncInputBuffer[i].YBufferSize = bufferInfo.planes[0].allocSize;
         pVideoEnc->MFCEncInputBuffer[i].CBufferSize = bufferInfo.planes[1].allocSize;
         pVideoEnc->MFCEncInputBuffer[i].YDataSize = bufferInfo.planes[0].dataSize;
         pVideoEnc->MFCEncInputBuffer[i].CDataSize = bufferInfo.planes[1].dataSize;
    }

    pVideoEnc->indexInputBuffer = 0;
    pVideoEnc->bFirstFrame = OMX_TRUE;
    pVideoEnc->bInputRun = OMX_FALSE;

    pVideoEnc->NBEncThread.bExitEncodeThread = OMX_FALSE;
    pVideoEnc->NBEncThread.bEncoderRun = OMX_FALSE;
    Exynos_OSAL_SemaphoreCreate(&(pVideoEnc->NBEncThread.hEncFrameStart));
    Exynos_OSAL_SemaphoreCreate(&(pVideoEnc->NBEncThread.hEncFrameEnd));
    if (OMX_ErrorNone == Exynos_OSAL_ThreadCreate(&pVideoEnc->NBEncThread.hNBEncodeThread,
                                                   Exynos_Mpeg4Enc_EncodeThread,
                                                   pOMXComponent)) {
        pMpeg4Enc->hMFCMpeg4Handle.returnCodec = VIDEO_TRUE;
    }

    Exynos_OSAL_Memset(pExynosComponent->timeStamp, -19771003, sizeof(OMX_TICKS) * MAX_TIMESTAMP);
    Exynos_OSAL_Memset(pExynosComponent->nFlags, 0, sizeof(OMX_U32) * MAX_FLAGS);
    pMpeg4Enc->hMFCMpeg4Handle.indexTimestamp = 0;

    pVideoEnc->csc_handle = csc_init(csc_method);

EXIT:
    FunctionOut();

    return ret;
}

/* MFC Terminate */
OMX_ERRORTYPE Exynos_Mpeg4Enc_Terminate(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE                  ret              = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc        = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_MPEG4ENC_HANDLE        *pMpeg4Enc        = (EXYNOS_MPEG4ENC_HANDLE *)pVideoEnc->hCodecHandle;
    ExynosVideoEncOps             *pEncOps          = pVideoEnc->pEncOps;
    ExynosVideoEncBufferOps       *pInbufOps        = pVideoEnc->pInbufOps;
    ExynosVideoEncBufferOps       *pOutbufOps       = pVideoEnc->pOutbufOps;
    OMX_HANDLETYPE                 hMFCHandle       = NULL;

    FunctionIn();

    if (pVideoEnc->NBEncThread.hNBEncodeThread != NULL) {
        pVideoEnc->NBEncThread.bExitEncodeThread = OMX_TRUE;
        Exynos_OSAL_SemaphorePost(pVideoEnc->NBEncThread.hEncFrameStart);
        Exynos_OSAL_ThreadTerminate(pVideoEnc->NBEncThread.hNBEncodeThread);
        pVideoEnc->NBEncThread.hNBEncodeThread = NULL;
    }

    if(pVideoEnc->NBEncThread.hEncFrameEnd != NULL) {
        Exynos_OSAL_SemaphoreTerminate(pVideoEnc->NBEncThread.hEncFrameEnd);
        pVideoEnc->NBEncThread.hEncFrameEnd = NULL;
    }

    if(pVideoEnc->NBEncThread.hEncFrameStart != NULL) {
        Exynos_OSAL_SemaphoreTerminate(pVideoEnc->NBEncThread.hEncFrameStart);
        pVideoEnc->NBEncThread.hEncFrameStart = NULL;
    }

    hMFCHandle = pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle;
    if (hMFCHandle != NULL) {
        pInbufOps->Stop(hMFCHandle);
        pOutbufOps->Stop(hMFCHandle);
        pEncOps->Finalize(hMFCHandle);

        Exynos_OSAL_Free(pInbufOps);
        Exynos_OSAL_Free(pOutbufOps);
        Exynos_OSAL_Free(pEncOps);

        pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle = NULL;
    }

    if (pVideoEnc->csc_handle != NULL) {
        csc_deinit(pVideoEnc->csc_handle);
        pVideoEnc->csc_handle = NULL;
    }

EXIT:
    FunctionOut();

    return ret;
}

static OMX_ERRORTYPE Exynos_Mpeg4Enc_Configure(
    OMX_COMPONENTTYPE *pOMXComponent,
    EXYNOS_OMX_DATA   *pInputData,
    EXYNOS_OMX_DATA   *pOutputData)
{
    OMX_ERRORTYPE                  ret               = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent  = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc         = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_MPEG4ENC_HANDLE        *pMpeg4Enc         = (EXYNOS_MPEG4ENC_HANDLE *)pVideoEnc->hCodecHandle;
    EXYNOS_OMX_BASEPORT           *pExynosInputPort  = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    ExynosVideoEncOps             *pEncOps           = pVideoEnc->pEncOps;
    ExynosVideoEncBufferOps       *pInbufOps         = pVideoEnc->pInbufOps;
    ExynosVideoEncBufferOps       *pOutbufOps        = pVideoEnc->pOutbufOps;

    OMX_S32 setConfVal = 0;
    ExynosVideoGeometry bufferConf;
    int i;

    Exynos_OSAL_Memset(&bufferConf, 0, sizeof(bufferConf));

    /* set geometry for output (dst) */
    if (pOutbufOps->Set_Geometry) {
        /* input buffer info: only 2 config values needed */
        bufferConf.nSizeImage = DEFAULT_MFC_INPUT_BUFFER_SIZE / 2;
        if (pMpeg4Enc->hMFCMpeg4Handle.codecType == CODEC_TYPE_MPEG4)
            bufferConf.eCompressionFormat = VIDEO_CODING_MPEG4;
        else
            bufferConf.eCompressionFormat = VIDEO_CODING_H263;
        if (pOutbufOps->Set_Geometry(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle, &bufferConf) != VIDEO_ERROR_NONE) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to set geometry for output buffer");
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }
    }

    /* cacheable for output */
    if (pOutbufOps->Enable_Cacheable) {
        if (pOutbufOps->Enable_Cacheable(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle) != VIDEO_ERROR_NONE)
            Exynos_OSAL_Log(EXYNOS_LOG_WARNING, "Failed to enable cacheable property for output buffer");
    }

    if (pOutbufOps->Setup(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle, MFC_OUTPUT_BUFFER_NUM_MAX) != VIDEO_ERROR_NONE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to setup output buffer");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    if (pOutbufOps->Enqueue_All) {
        if (pOutbufOps->Enqueue_All(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle) != VIDEO_ERROR_NONE) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to flush all output buffer");
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }
    }

    /* start header encoding */
    if (pOutbufOps->Run(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle) != VIDEO_ERROR_NONE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to run output buffer for header parsing");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    pVideoEnc->pOutbuf = pOutbufOps->Dequeue(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle);
    if (pVideoEnc->pOutbuf == NULL) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to dequeue output buffer");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    pOutputData->dataBuffer= pVideoEnc->pOutbuf->planes[0].addr;
    pOutputData->allocSize = pVideoEnc->pOutbuf->planes[0].allocSize;
    pOutputData->dataLen = pVideoEnc->pOutbuf->planes[0].dataSize;
    pOutputData->usedDataLen = 0;

    ret = OMX_ErrorNone;

    pMpeg4Enc->hMFCMpeg4Handle.bConfiguredMFC = OMX_TRUE;
    pMpeg4Enc->hMFCMpeg4Handle.returnCodec = VIDEO_TRUE;

EXIT:
    return ret;
}

/* nonblock */
static OMX_ERRORTYPE Exynos_Mpeg4Enc_Encode(
    OMX_COMPONENTTYPE *pOMXComponent,
    EXYNOS_OMX_DATA   *pInputData,
    EXYNOS_OMX_DATA   *pOutputData)
{
    OMX_ERRORTYPE                  ret               = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent  = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc         = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_MPEG4ENC_HANDLE        *pMpeg4Enc         = (EXYNOS_MPEG4ENC_HANDLE *)pVideoEnc->hCodecHandle;
    EXYNOS_OMX_BASEPORT           *pExynosInputPort  = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT           *pExynosPort       = NULL;
    ExynosVideoEncOps             *pEncOps           = pVideoEnc->pEncOps;
    ExynosVideoEncBufferOps       *pInbufOps         = pVideoEnc->pInbufOps;
    ExynosVideoEncBufferOps       *pOutbufOps        = pVideoEnc->pOutbufOps;

    pVideoEnc->NBEncThread.oneFrameSize = pInputData->dataLen;
    OMX_PTR ppBuf[3];

    FunctionIn();

    if (pMpeg4Enc->hMFCMpeg4Handle.bConfiguredMFC == OMX_FALSE) {
        ret = Exynos_Mpeg4Enc_Configure(pOMXComponent, pInputData, pOutputData);
        if (ret != OMX_ErrorNone) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: failed to configure encoder", __FUNCTION__);
            ret = OMX_ErrorUndefined;
            goto EXIT;
        }

        pOutputData->timeStamp = 0;
        pOutputData->nFlags |= OMX_BUFFERFLAG_CODECCONFIG;
        pOutputData->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;

        pMpeg4Enc->hMFCMpeg4Handle.bConfiguredMFC = OMX_TRUE;

        ret = OMX_ErrorInputDataEncodeYet;
        goto EXIT;
    }

    if ((pInputData->nFlags & OMX_BUFFERFLAG_ENDOFFRAME) &&
        (pExynosComponent->bUseFlagEOF == OMX_FALSE))
        pExynosComponent->bUseFlagEOF = OMX_TRUE;

    if (pVideoEnc->NBEncThread.oneFrameSize <= 0) {
        pOutputData->timeStamp = pInputData->timeStamp;
        pOutputData->nFlags = pInputData->nFlags;

        ret = OMX_ErrorNone;
        goto EXIT;
    }

    pExynosPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];

    if (((pInputData->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) ||
        (pExynosComponent->getAllDelayBuffer == OMX_TRUE)){
        /* Dummy input data for get out encoded last frame */
        pMpeg4Enc->hMFCMpeg4Handle.pMFCYUVVirBuffer[0] = pVideoEnc->MFCEncInputBuffer[pVideoEnc->indexInputBuffer].YVirAddr;
        pMpeg4Enc->hMFCMpeg4Handle.pMFCYUVVirBuffer[1] = pVideoEnc->MFCEncInputBuffer[pVideoEnc->indexInputBuffer].CVirAddr;
    } else {
        switch (pExynosPort->portDefinition.format.video.eColorFormat) {
        case OMX_SEC_COLOR_FormatNV12TPhysicalAddress:
        case OMX_SEC_COLOR_FormatNV12LPhysicalAddress:
        case OMX_SEC_COLOR_FormatNV21LPhysicalAddress:
#ifndef USE_METADATABUFFERTYPE
            /* USE_FIMC_FRAME_BUFFER */
            Exynos_OSAL_Memcpy(&(pMpeg4Enc->hMFCMpeg4Handle.pMFCYUVVirBuffer[0]), pInputData->dataBuffer, sizeof(OMX_PTR));
            Exynos_OSAL_Memcpy(&(pMpeg4Enc->hMFCMpeg4Handle.pMFCYUVVirBuffer[1]), pInputData->dataBuffer + sizeof(OMX_PTR), sizeof(OMX_PTR));
#else
            Exynos_OSAL_GetInfoFromMetaData(pInputData, ppBuf);

            Exynos_OSAL_Memcpy(&(pMpeg4Enc->hMFCMpeg4Handle.pMFCYUVVirBuffer[0]), ppBuf[0], sizeof(OMX_PTR));
            Exynos_OSAL_Memcpy(&(pMpeg4Enc->hMFCMpeg4Handle.pMFCYUVVirBuffer[1]), ppBuf[1], sizeof(OMX_PTR));
#endif
            break;
        case OMX_SEC_COLOR_FormatNV12LVirtualAddress:
            pMpeg4Enc->hMFCMpeg4Handle.pMFCYUVVirBuffer[0] = *((void **)pInputData->dataBuffer);
            pMpeg4Enc->hMFCMpeg4Handle.pMFCYUVVirBuffer[1] = (char *)pMpeg4Enc->hMFCMpeg4Handle.pMFCYUVVirBuffer[0] +
                                                          (ALIGN_TO_16B(pExynosPort->portDefinition.format.video.nFrameWidth) *
                                                           ALIGN_TO_16B(pExynosPort->portDefinition.format.video.nFrameHeight));
            break;
        default:
            pMpeg4Enc->hMFCMpeg4Handle.pMFCYUVPhyBuffer[0] = pVideoEnc->MFCEncInputBuffer[pVideoEnc->indexInputBuffer].YPhyAddr;
            pMpeg4Enc->hMFCMpeg4Handle.pMFCYUVVirBuffer[0] = pVideoEnc->MFCEncInputBuffer[pVideoEnc->indexInputBuffer].YVirAddr;
            pMpeg4Enc->hMFCMpeg4Handle.pMFCYUVPhyBuffer[1] = pVideoEnc->MFCEncInputBuffer[pVideoEnc->indexInputBuffer].CPhyAddr;
            pMpeg4Enc->hMFCMpeg4Handle.pMFCYUVVirBuffer[1] = pVideoEnc->MFCEncInputBuffer[pVideoEnc->indexInputBuffer].CVirAddr;
            break;
        }
    }

    pMpeg4Enc->hMFCMpeg4Handle.pMFCYUVDataSize[0] = ALIGN_TO_16B(pExynosPort->portDefinition.format.video.nFrameWidth) *
                                                    ALIGN_TO_16B(pExynosPort->portDefinition.format.video.nFrameHeight);
    pMpeg4Enc->hMFCMpeg4Handle.pMFCYUVDataSize[1] = pMpeg4Enc->hMFCMpeg4Handle.pMFCYUVDataSize[0] / 2;

    pExynosComponent->timeStamp[pMpeg4Enc->hMFCMpeg4Handle.indexTimestamp] = pInputData->timeStamp;
    pExynosComponent->nFlags[pMpeg4Enc->hMFCMpeg4Handle.indexTimestamp] = pInputData->nFlags;

    if ((pMpeg4Enc->hMFCMpeg4Handle.returnCodec == VIDEO_TRUE) &&
        (pVideoEnc->bFirstFrame == OMX_FALSE)) {
        OMX_S32 indexTimestamp = 0;

        /* wait for mfc encode done */
        if (pVideoEnc->NBEncThread.bEncoderRun != OMX_FALSE) {
            Exynos_OSAL_SemaphoreWait(pVideoEnc->NBEncThread.hEncFrameEnd);
            pVideoEnc->NBEncThread.bEncoderRun = OMX_FALSE;
        }

        Exynos_OSAL_SleepMillisec(0);

        indexTimestamp = pEncOps->Get_FrameTag(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle);
        if (indexTimestamp < 0 || indexTimestamp >= MAX_TIMESTAMP) {
            pOutputData->timeStamp = pInputData->timeStamp;
            pOutputData->nFlags = pInputData->nFlags;
        } else {
            pOutputData->timeStamp = pExynosComponent->timeStamp[indexTimestamp];
            pOutputData->nFlags = pExynosComponent->nFlags[indexTimestamp];
        }

        if (pVideoEnc->pOutbuf != NULL) {
            /** Fill Output Buffer **/
            pOutputData->dataBuffer  = pVideoEnc->pOutbuf->planes[0].addr;
            pOutputData->allocSize   = pVideoEnc->pOutbuf->planes[0].allocSize;
            pOutputData->dataLen     = pVideoEnc->pOutbuf->planes[0].dataSize;
            pOutputData->usedDataLen = 0;

            pOutputData->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;
            if (pVideoEnc->pOutbuf->frameType == VIDEO_FRAME_I)
                pOutputData->nFlags |= OMX_BUFFERFLAG_SYNCFRAME;

            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "MFC Encode OK!");

            ret = OMX_ErrorNone;
        } else {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: failed to get output buffer, ret:%d", __FUNCTION__, pMpeg4Enc->hMFCMpeg4Handle.returnCodec);
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }

        if (pExynosComponent->getAllDelayBuffer == OMX_TRUE) {
            ret = OMX_ErrorInputDataEncodeYet;
        }

        if ((pInputData->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) {
            pInputData->nFlags = (pOutputData->nFlags & (~OMX_BUFFERFLAG_EOS));
            pExynosComponent->getAllDelayBuffer = OMX_TRUE;
            ret = OMX_ErrorInputDataEncodeYet;
        }

        if ((pOutputData->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) {
            pExynosComponent->getAllDelayBuffer = OMX_FALSE;
            pOutputData->dataLen = 0;
            pOutputData->usedDataLen = 0;
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "OMX_BUFFERFLAG_EOS!!!");
            ret = OMX_ErrorNone;
        }
    }

    if (pVideoEnc->configChange == OMX_TRUE) {
        if (pMpeg4Enc->hMFCMpeg4Handle.codecType == CODEC_TYPE_MPEG4)
            Change_Mpeg4Enc_Param(pExynosComponent);
        else
            Change_H263Enc_Param(pExynosComponent);
        pVideoEnc->configChange = OMX_FALSE;
    }

    if (pEncOps->Set_FrameTag(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle,
            pMpeg4Enc->hMFCMpeg4Handle.indexTimestamp) != VIDEO_ERROR_NONE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to set frame tag");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    if (pOutbufOps->Enqueue(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle,
                           (unsigned char **)&pVideoEnc->pOutbuf->planes[0].addr,
                           (unsigned int *)&pVideoEnc->pOutbuf->planes[0].dataSize,
                            1, NULL) != VIDEO_ERROR_NONE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to enqueue output buffer");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    if (pInbufOps->Enqueue(pMpeg4Enc->hMFCMpeg4Handle.hMFCHandle,
                          (unsigned char **)pMpeg4Enc->hMFCMpeg4Handle.pMFCYUVVirBuffer,
                          (unsigned int *)pMpeg4Enc->hMFCMpeg4Handle.pMFCYUVDataSize,
                          2, NULL) != VIDEO_ERROR_NONE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to enqueue input buffer");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    } else {
        pVideoEnc->indexInputBuffer++;
        pVideoEnc->indexInputBuffer %= MFC_INPUT_BUFFER_NUM_MAX;
    }

    /* mfc encode start */
    Exynos_OSAL_SemaphorePost(pVideoEnc->NBEncThread.hEncFrameStart);
    pVideoEnc->NBEncThread.bEncoderRun = OMX_TRUE;
    pMpeg4Enc->hMFCMpeg4Handle.indexTimestamp++;
    pMpeg4Enc->hMFCMpeg4Handle.indexTimestamp %= MAX_TIMESTAMP;
    pVideoEnc->bFirstFrame = OMX_FALSE;
    Exynos_OSAL_SleepMillisec(0);

EXIT:
    FunctionOut();

    return ret;
}

/* MFC Encode */
OMX_ERRORTYPE Exynos_Mpeg4Enc_BufferProcess(
    OMX_COMPONENTTYPE *pOMXComponent,
    EXYNOS_OMX_DATA   *pInputData,
    EXYNOS_OMX_DATA   *pOutputData)
{
    OMX_ERRORTYPE               ret               = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT   *pExynosComponent  = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_MPEG4ENC_HANDLE     *pMpeg4Enc         = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    EXYNOS_OMX_BASEPORT        *pExynosInputPort  = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT        *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    OMX_BOOL                    endOfFrame        = OMX_FALSE;
    OMX_BOOL                    flagEOS           = OMX_FALSE;

    FunctionIn();

    if ((!CHECK_PORT_ENABLED(pExynosInputPort)) || (!CHECK_PORT_ENABLED(pExynosOutputPort)) ||
        (!CHECK_PORT_POPULATED(pExynosInputPort)) || (!CHECK_PORT_POPULATED(pExynosOutputPort))) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }
    if (OMX_FALSE == Exynos_Check_BufferProcess_State(pExynosComponent)) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    ret = Exynos_Mpeg4Enc_Encode(pOMXComponent, pInputData, pOutputData);
    if (ret != OMX_ErrorNone) {
        if (ret == OMX_ErrorInputDataEncodeYet) {
            pOutputData->usedDataLen = 0;
            pOutputData->remainDataLen = pOutputData->dataLen;
        } else {
            pExynosComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
                                            pExynosComponent->callbackData,
                                            OMX_EventError, ret, 0, NULL);
        }
    } else {
        pInputData->usedDataLen += pInputData->dataLen;
        pInputData->remainDataLen = pInputData->dataLen - pInputData->usedDataLen;
        pInputData->dataLen -= pInputData->usedDataLen;
        pInputData->usedDataLen = 0;

        pOutputData->remainDataLen = pOutputData->dataLen - pOutputData->usedDataLen;
    }

EXIT:
    FunctionOut();

    return ret;
}

OSCL_EXPORT_REF OMX_ERRORTYPE Exynos_OMX_ComponentInit(
    OMX_HANDLETYPE hComponent,
    OMX_STRING     componentName)
{
    OMX_ERRORTYPE                  ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE             *pOMXComponent    = NULL;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = NULL;
    EXYNOS_OMX_BASEPORT           *pExynosPort      = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc        = NULL;
    EXYNOS_MPEG4ENC_HANDLE        *pMpeg4Enc        = NULL;
    OMX_S32                        codecType        = -1;
    int i = 0;

    FunctionIn();

    if ((hComponent == NULL) || (componentName == NULL)) {
        ret = OMX_ErrorBadParameter;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: parameters are null, ret: %X", __FUNCTION__, ret);
        goto EXIT;
    }
    if (Exynos_OSAL_Strcmp(EXYNOS_OMX_COMPONENT_MPEG4_ENC, componentName) == 0) {
        codecType = CODEC_TYPE_MPEG4;
    } else if (Exynos_OSAL_Strcmp(EXYNOS_OMX_COMPONENT_H263_ENC, componentName) == 0) {
        codecType = CODEC_TYPE_H263;
    } else {
        ret = OMX_ErrorBadParameter;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: componentName(%s) error, ret: %X", __FUNCTION__, componentName, ret);
        goto EXIT;
    }

    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_VideoEncodeComponentInit(pOMXComponent);
    if (ret != OMX_ErrorNone) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: Exynos_OMX_VideoDecodeComponentInit error, ret: %X", __FUNCTION__, ret);
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    pExynosComponent->codecType = HW_VIDEO_ENC_CODEC;

    pExynosComponent->componentName = (OMX_STRING)Exynos_OSAL_Malloc(MAX_OMX_COMPONENT_NAME_SIZE);
    if (pExynosComponent->componentName == NULL) {
        Exynos_OMX_VideoEncodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: componentName alloc error, ret: %X", __FUNCTION__, ret);
        goto EXIT;
    }
    Exynos_OSAL_Memset(pExynosComponent->componentName, 0, MAX_OMX_COMPONENT_NAME_SIZE);

    pMpeg4Enc = Exynos_OSAL_Malloc(sizeof(EXYNOS_MPEG4ENC_HANDLE));
    if (pMpeg4Enc == NULL) {
        Exynos_OMX_VideoEncodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: EXYNOS_MPEG4ENC_HANDLE alloc error, ret: %X", __FUNCTION__, ret);
        goto EXIT;
    }
    Exynos_OSAL_Memset(pMpeg4Enc, 0, sizeof(EXYNOS_MPEG4ENC_HANDLE));
    pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    pVideoEnc->hCodecHandle = (OMX_HANDLETYPE)pMpeg4Enc;
    pMpeg4Enc->hMFCMpeg4Handle.codecType = codecType;

    if (codecType == CODEC_TYPE_MPEG4)
        Exynos_OSAL_Strcpy(pExynosComponent->componentName, EXYNOS_OMX_COMPONENT_MPEG4_ENC);
    else
        Exynos_OSAL_Strcpy(pExynosComponent->componentName, EXYNOS_OMX_COMPONENT_H263_ENC);

    /* Set componentVersion */
    pExynosComponent->componentVersion.s.nVersionMajor = VERSIONMAJOR_NUMBER;
    pExynosComponent->componentVersion.s.nVersionMinor = VERSIONMINOR_NUMBER;
    pExynosComponent->componentVersion.s.nRevision     = REVISION_NUMBER;
    pExynosComponent->componentVersion.s.nStep         = STEP_NUMBER;
    /* Set specVersion */
    pExynosComponent->specVersion.s.nVersionMajor = VERSIONMAJOR_NUMBER;
    pExynosComponent->specVersion.s.nVersionMinor = VERSIONMINOR_NUMBER;
    pExynosComponent->specVersion.s.nRevision     = REVISION_NUMBER;
    pExynosComponent->specVersion.s.nStep         = STEP_NUMBER;

    /* Android CapabilityFlags */
    pExynosComponent->capabilityFlags.iIsOMXComponentMultiThreaded                   = OMX_TRUE;
    pExynosComponent->capabilityFlags.iOMXComponentSupportsExternalInputBufferAlloc  = OMX_TRUE;
    pExynosComponent->capabilityFlags.iOMXComponentSupportsExternalOutputBufferAlloc = OMX_TRUE;
    pExynosComponent->capabilityFlags.iOMXComponentSupportsMovableInputBuffers       = OMX_FALSE;
    pExynosComponent->capabilityFlags.iOMXComponentSupportsPartialFrames             = OMX_FALSE;
    pExynosComponent->capabilityFlags.iOMXComponentUsesNALStartCodes                 = OMX_TRUE;
    pExynosComponent->capabilityFlags.iOMXComponentCanHandleIncompleteFrames         = OMX_TRUE;
    pExynosComponent->capabilityFlags.iOMXComponentUsesFullAVCFrames                 = OMX_TRUE;

    /* Input port */
    pExynosPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    pExynosPort->portDefinition.format.video.nFrameWidth = DEFAULT_FRAME_WIDTH;
    pExynosPort->portDefinition.format.video.nFrameHeight= DEFAULT_FRAME_HEIGHT;
    pExynosPort->portDefinition.format.video.nBitrate = 64000;
    pExynosPort->portDefinition.format.video.xFramerate= (15 << 16);
    pExynosPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
    pExynosPort->portDefinition.format.video.pNativeRender = 0;
    pExynosPort->portDefinition.format.video.bFlagErrorConcealment = OMX_FALSE;
    Exynos_OSAL_Memset(pExynosPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
    Exynos_OSAL_Strcpy(pExynosPort->portDefinition.format.video.cMIMEType, "raw/video");
    pExynosPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
    pExynosPort->portDefinition.nBufferSize = DEFAULT_VIDEO_INPUT_BUFFER_SIZE;
    pExynosPort->portDefinition.bEnabled = OMX_TRUE;

    /* Output port */
    pExynosPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    pExynosPort->portDefinition.format.video.nFrameWidth = DEFAULT_FRAME_WIDTH;
    pExynosPort->portDefinition.format.video.nFrameHeight= DEFAULT_FRAME_HEIGHT;
    pExynosPort->portDefinition.format.video.nBitrate = 64000;
    pExynosPort->portDefinition.format.video.xFramerate= (15 << 16);
    if (codecType == CODEC_TYPE_MPEG4) {
        pExynosPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingMPEG4;
        Exynos_OSAL_Memset(pExynosPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
        Exynos_OSAL_Strcpy(pExynosPort->portDefinition.format.video.cMIMEType, "video/mpeg4");
    } else {
        pExynosPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingH263;
        Exynos_OSAL_Memset(pExynosPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
        Exynos_OSAL_Strcpy(pExynosPort->portDefinition.format.video.cMIMEType, "video/h263");
    }
    pExynosPort->portDefinition.format.video.pNativeRender = 0;
    pExynosPort->portDefinition.format.video.bFlagErrorConcealment = OMX_FALSE;
    pExynosPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    pExynosPort->portDefinition.nBufferSize = DEFAULT_VIDEO_OUTPUT_BUFFER_SIZE;
    pExynosPort->portDefinition.bEnabled = OMX_TRUE;

    if (codecType == CODEC_TYPE_MPEG4) {
        for(i = 0; i < ALL_PORT_NUM; i++) {
            INIT_SET_SIZE_VERSION(&pMpeg4Enc->mpeg4Component[i], OMX_VIDEO_PARAM_MPEG4TYPE);
            pMpeg4Enc->mpeg4Component[i].nPortIndex = i;
            pMpeg4Enc->mpeg4Component[i].eProfile   = OMX_VIDEO_MPEG4ProfileSimple;
            pMpeg4Enc->mpeg4Component[i].eLevel     = OMX_VIDEO_MPEG4Level4;

            pMpeg4Enc->mpeg4Component[i].nPFrames = 10;
            pMpeg4Enc->mpeg4Component[i].nBFrames = 0;          /* No support for B frames */
            pMpeg4Enc->mpeg4Component[i].nMaxPacketSize = 256;  /* Default value */
            pMpeg4Enc->mpeg4Component[i].nAllowedPictureTypes =  OMX_VIDEO_PictureTypeI | OMX_VIDEO_PictureTypeP;
            pMpeg4Enc->mpeg4Component[i].bGov = OMX_FALSE;

        }
    } else {
        for(i = 0; i < ALL_PORT_NUM; i++) {
            INIT_SET_SIZE_VERSION(&pMpeg4Enc->h263Component[i], OMX_VIDEO_PARAM_H263TYPE);
            pMpeg4Enc->h263Component[i].nPortIndex = i;
            pMpeg4Enc->h263Component[i].eProfile   = OMX_VIDEO_H263ProfileBaseline;
            pMpeg4Enc->h263Component[i].eLevel     = OMX_VIDEO_H263Level45;

            pMpeg4Enc->h263Component[i].nPFrames = 20;
            pMpeg4Enc->h263Component[i].nBFrames = 0;          /* No support for B frames */
            pMpeg4Enc->h263Component[i].bPLUSPTYPEAllowed = OMX_FALSE;
            pMpeg4Enc->h263Component[i].nAllowedPictureTypes = OMX_VIDEO_PictureTypeI | OMX_VIDEO_PictureTypeP;
            pMpeg4Enc->h263Component[i].bForceRoundingTypeToZero = OMX_TRUE;
            pMpeg4Enc->h263Component[i].nPictureHeaderRepetition = 0;
            pMpeg4Enc->h263Component[i].nGOBHeaderInterval = 0;
        }
    }

    pOMXComponent->GetParameter      = &Exynos_Mpeg4Enc_GetParameter;
    pOMXComponent->SetParameter      = &Exynos_Mpeg4Enc_SetParameter;
    pOMXComponent->GetConfig         = &Exynos_Mpeg4Enc_GetConfig;
    pOMXComponent->SetConfig         = &Exynos_Mpeg4Enc_SetConfig;
    pOMXComponent->GetExtensionIndex = &Exynos_Mpeg4Enc_GetExtensionIndex;
    pOMXComponent->ComponentRoleEnum = &Exynos_Mpeg4Enc_ComponentRoleEnum;
    pOMXComponent->ComponentDeInit   = &Exynos_OMX_ComponentDeinit;

    pExynosComponent->exynos_mfc_componentInit      = &Exynos_Mpeg4Enc_Init;
    pExynosComponent->exynos_mfc_componentTerminate = &Exynos_Mpeg4Enc_Terminate;
    pExynosComponent->exynos_mfc_bufferProcess      = &Exynos_Mpeg4Enc_BufferProcess;
    pExynosComponent->exynos_checkInputFrame        = NULL;

    pExynosComponent->currentState = OMX_StateLoaded;

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_ComponentDeinit(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE               ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE          *pOMXComponent    = NULL;
    EXYNOS_OMX_BASECOMPONENT   *pExynosComponent = NULL;
    EXYNOS_MPEG4ENC_HANDLE     *pMpeg4Enc        = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    Exynos_OSAL_Free(pExynosComponent->componentName);
    pExynosComponent->componentName = NULL;

    pMpeg4Enc = (EXYNOS_MPEG4ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    if (pMpeg4Enc != NULL) {
        Exynos_OSAL_Free(pMpeg4Enc);
        pMpeg4Enc = ((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle = NULL;
    }

    ret = Exynos_OMX_VideoEncodeComponentDeinit(pOMXComponent);
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}
