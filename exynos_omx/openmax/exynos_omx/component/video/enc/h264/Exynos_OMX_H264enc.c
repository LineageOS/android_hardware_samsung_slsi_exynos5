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
 * @file        Exynos_OMX_H264enc.c
 * @brief
 * @author      SeungBeom Kim (sbcrux.kim@samsung.com)
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
#include "Exynos_OSAL_ETC.h"
#include "Exynos_OSAL_Semaphore.h"
#include "Exynos_OSAL_Thread.h"
#include "Exynos_OSAL_Android.h"
#include "library_register.h"
#include "Exynos_OMX_H264enc.h"
#include "ExynosVideoApi.h"
#include "csc.h"

#undef  EXYNOS_LOG_TAG
#define EXYNOS_LOG_TAG    "EXYNOS_H264_ENC"
#define EXYNOS_LOG_OFF
#include "Exynos_OSAL_Log.h"

/* H.264 Encoder Supported Levels & profiles */
EXYNOS_OMX_VIDEO_PROFILELEVEL supportedAVCProfileLevels[] ={
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel1},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel1b},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel11},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel12},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel13},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel2},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel21},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel22},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel3},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel31},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel32},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel4},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel41},
    {OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel42},

    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel1},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel1b},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel11},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel12},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel13},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel2},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel21},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel22},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel3},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel31},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel32},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel4},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel41},
    {OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel42},

    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel1},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel1b},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel11},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel12},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel13},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel2},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel21},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel22},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel3},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel31},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel32},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel4},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel41},
    {OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel42}};


static OMX_U32 OMXAVCProfileToProfileIDC(OMX_VIDEO_AVCPROFILETYPE profile)
{
    OMX_U32 ret = 0;

    if (profile == OMX_VIDEO_AVCProfileBaseline)
        ret = 0;
    else if (profile == OMX_VIDEO_AVCProfileMain)
        ret = 2;
    else if (profile == OMX_VIDEO_AVCProfileHigh)
        ret = 4;

    return ret;
}

static OMX_U32 OMXAVCLevelToLevelIDC(OMX_VIDEO_AVCLEVELTYPE level)
{
    OMX_U32 ret = 11; //default OMX_VIDEO_AVCLevel4

    if (level == OMX_VIDEO_AVCLevel1)
        ret = 0;
    else if (level == OMX_VIDEO_AVCLevel1b)
        ret = 1;
    else if (level == OMX_VIDEO_AVCLevel11)
        ret = 2;
    else if (level == OMX_VIDEO_AVCLevel12)
        ret = 3;
    else if (level == OMX_VIDEO_AVCLevel13)
        ret = 4;
    else if (level == OMX_VIDEO_AVCLevel2)
        ret = 5;
    else if (level == OMX_VIDEO_AVCLevel21)
        ret = 6;
    else if (level == OMX_VIDEO_AVCLevel22)
        ret = 7;
    else if (level == OMX_VIDEO_AVCLevel3)
        ret = 8;
    else if (level == OMX_VIDEO_AVCLevel31)
        ret = 9;
    else if (level == OMX_VIDEO_AVCLevel32)
        ret = 10;
    else if (level == OMX_VIDEO_AVCLevel4)
        ret = 11;
    else if (level == OMX_VIDEO_AVCLevel41)
        ret = 12;
    else if (level == OMX_VIDEO_AVCLevel42)
        ret = 13;

    return ret;
}

static OMX_U8 *FindDelimiter(OMX_U8 *pBuffer, OMX_U32 size)
{
    OMX_U32 i;

    for (i = 0; i < size - 3; i++) {
        if ((pBuffer[i] == 0x00)   &&
            (pBuffer[i + 1] == 0x00) &&
            (pBuffer[i + 2] == 0x00) &&
            (pBuffer[i + 3] == 0x01))
            return (pBuffer + i);
    }

    return NULL;
}

static void Print_H264Enc_Param(ExynosVideoEncParam *pEncParam)
{
    ExynosVideoEncCommonParam *pCommonParam = &pEncParam->commonParam;
    ExynosVideoEncH264Param   *pH264Param   = &pEncParam->codecParam.h264;

    /* common parameters */
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "SourceWidth             : %d", pCommonParam->SourceWidth);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "SourceHeight            : %d", pCommonParam->SourceHeight);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "IDRPeriod               : %d", pCommonParam->IDRPeriod);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "SliceMode               : %d", pCommonParam->SliceMode);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "RandomIntraMBRefresh    : %d", pCommonParam->RandomIntraMBRefresh);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Bitrate                 : %d", pCommonParam->Bitrate);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "FrameQp                 : %d", pCommonParam->FrameQp);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "FrameQp_P               : %d", pCommonParam->FrameQp_P);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "QSCodeMax              : %d", pCommonParam->QSCodeMax);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "QSCodeMin               : %d", pCommonParam->QSCodeMin);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "PadControlOn            : %d", pCommonParam->PadControlOn);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "LumaPadVal              : %d", pCommonParam->LumaPadVal);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "CbPadVal                : %d", pCommonParam->CbPadVal);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "CrPadVal                : %d", pCommonParam->CrPadVal);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "FrameMap                : %d", pCommonParam->FrameMap);

    /* H.264 specific parameters */
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "ProfileIDC              : %d", pH264Param->ProfileIDC);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "LevelIDC                : %d", pH264Param->LevelIDC);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "FrameQp_B               : %d", pH264Param->FrameQp_B);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "FrameRate               : %d", pH264Param->FrameRate);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "SliceArgument           : %d", pH264Param->SliceArgument);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "NumberBFrames           : %d", pH264Param->NumberBFrames);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "NumberReferenceFrames   : %d", pH264Param->NumberReferenceFrames);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "NumberRefForPframes     : %d", pH264Param->NumberRefForPframes);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "LoopFilterDisable       : %d", pH264Param->LoopFilterDisable);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "LoopFilterAlphaC0Offset : %d", pH264Param->LoopFilterAlphaC0Offset);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "LoopFilterBetaOffset    : %d", pH264Param->LoopFilterBetaOffset);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "SymbolMode              : %d", pH264Param->SymbolMode);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "PictureInterlace        : %d", pH264Param->PictureInterlace);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Transform8x8Mode        : %d", pH264Param->Transform8x8Mode);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "DarkDisable             : %d", pH264Param->DarkDisable);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "SmoothDisable           : %d", pH264Param->SmoothDisable);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "StaticDisable           : %d", pH264Param->StaticDisable);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "ActivityDisable         : %d", pH264Param->ActivityDisable);

    /* rate control related parameters */
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "EnableFRMRateControl    : %d", pCommonParam->EnableFRMRateControl);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "EnableMBRateControl     : %d", pCommonParam->EnableMBRateControl);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "CBRPeriodRf             : %d", pCommonParam->CBRPeriodRf);
}

static void Set_H264Enc_Param(EXYNOS_OMX_BASECOMPONENT *pExynosComponent)
{
    EXYNOS_OMX_BASEPORT           *pExynosInputPort  = NULL;
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc         = NULL;
    EXYNOS_H264ENC_HANDLE         *pH264Enc          = NULL;

    ExynosVideoEncParam       *pEncParam    = NULL;
    ExynosVideoEncCommonParam *pCommonParam = NULL;
    ExynosVideoEncH264Param   *pH264Param   = NULL;

    pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    pH264Enc = (EXYNOS_H264ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

    pEncParam = &pVideoEnc->encParam;
    pCommonParam = &pEncParam->commonParam;
    pH264Param = &pEncParam->codecParam.h264;
    pEncParam->eCompressionFormat = VIDEO_CODING_AVC;
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "eCompressionFormat: %d", pEncParam->eCompressionFormat);

    /* common parameters */
    pCommonParam->SourceWidth  = pExynosOutputPort->portDefinition.format.video.nFrameWidth;
    pCommonParam->SourceHeight = pExynosOutputPort->portDefinition.format.video.nFrameHeight;
    pCommonParam->IDRPeriod    = pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].nPFrames + 1;
    pCommonParam->SliceMode    = 0;
    pCommonParam->RandomIntraMBRefresh = 0;
    pCommonParam->Bitrate      = pExynosOutputPort->portDefinition.format.video.nBitrate;
    pCommonParam->FrameQp      = pVideoEnc->quantization.nQpI;
    pCommonParam->FrameQp_P    = pVideoEnc->quantization.nQpP;
    pCommonParam->QSCodeMax    = 51;
    pCommonParam->QSCodeMin    = 10;
    pCommonParam->PadControlOn = 0;    /* 0: disable, 1: enable */
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

    /* H.264 specific parameters */
    pH264Param->ProfileIDC   = OMXAVCProfileToProfileIDC(pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].eProfile);    /*0: OMX_VIDEO_AVCProfileMain */
    pH264Param->LevelIDC     = OMXAVCLevelToLevelIDC(pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].eLevel);    /*40: OMX_VIDEO_AVCLevel4 */
    pH264Param->FrameQp_B    = pVideoEnc->quantization.nQpB;
    pH264Param->FrameRate    = (pExynosInputPort->portDefinition.format.video.xFramerate) >> 16;
    pH264Param->SliceArgument = 0;    /* Slice mb/byte size number */
    pH264Param->NumberBFrames = 0;    /* 0 ~ 2 */
    pH264Param->NumberReferenceFrames = 1;
    pH264Param->NumberRefForPframes   = 1;
    pH264Param->LoopFilterDisable     = 1;    /* 1: Loop Filter Disable, 0: Filter Enable */
    pH264Param->LoopFilterAlphaC0Offset = 0;
    pH264Param->LoopFilterBetaOffset    = 0;
    pH264Param->SymbolMode       = 0;    /* 0: CAVLC, 1: CABAC */
    pH264Param->PictureInterlace = 0;
    pH264Param->Transform8x8Mode = 0;    /* 0: 4x4, 1: allow 8x8 */
    pH264Param->DarkDisable     = 1;
    pH264Param->SmoothDisable   = 1;
    pH264Param->StaticDisable   = 1;
    pH264Param->ActivityDisable = 1;

    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pVideoEnc->eControlRate[OUTPUT_PORT_INDEX]: 0x%x", pVideoEnc->eControlRate[OUTPUT_PORT_INDEX]);
    /* rate control related parameters */
    switch (pVideoEnc->eControlRate[OUTPUT_PORT_INDEX]) {
    case OMX_Video_ControlRateVariable:
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Video Encode VBR");
        pCommonParam->EnableFRMRateControl = 0;    /* 0: Disable, 1: Frame level RC */
        pCommonParam->EnableMBRateControl  = 0;    /* 0: Disable, 1:MB level RC */
        pCommonParam->CBRPeriodRf  = 100;
        break;
    case OMX_Video_ControlRateConstant:
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Video Encode CBR");
        pCommonParam->EnableFRMRateControl = 1;    /* 0: Disable, 1: Frame level RC */
        pCommonParam->EnableMBRateControl  = 1;    /* 0: Disable, 1:MB level RC */
        pCommonParam->CBRPeriodRf  = 10;
        break;
    case OMX_Video_ControlRateDisable:
    default: /*Android default */
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Video Encode VBR");
        pCommonParam->EnableFRMRateControl = 0;
        pCommonParam->EnableMBRateControl  = 0;
        pCommonParam->CBRPeriodRf  = 100;
        break;
    }

    Print_H264Enc_Param(pEncParam);

}

static void Change_H264Enc_Param(EXYNOS_OMX_BASECOMPONENT *pExynosComponent)
{
    EXYNOS_OMX_BASEPORT           *pExynosInputPort  = NULL;
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc         = NULL;
    EXYNOS_H264ENC_HANDLE         *pH264Enc          = NULL;

    ExynosVideoEncOps         *pEncOps      = NULL;
    ExynosVideoEncParam       *pEncParam    = NULL;
    ExynosVideoEncCommonParam *pCommonParam = NULL;
    ExynosVideoEncH264Param   *pH264Param   = NULL;

    int setParam = 0;

    pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    pH264Enc = (EXYNOS_H264ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    pEncOps = pVideoEnc->pEncOps;

    pEncParam = &pVideoEnc->encParam;
    pCommonParam = &pEncParam->commonParam;
    pH264Param = &pEncParam->codecParam.h264;

    if (pVideoEnc->IntraRefreshVOP == OMX_TRUE) {
        setParam = VIDEO_FRAME_I;
        pEncOps->Set_FrameType(pH264Enc->hMFCH264Handle.hMFCHandle, setParam);
        pVideoEnc->IntraRefreshVOP = OMX_FALSE;
    }
    if (pCommonParam->IDRPeriod != (int)pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].nPFrames + 1) {
        setParam = pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].nPFrames + 1;
        pEncOps->Set_IDRPeriod(pH264Enc->hMFCH264Handle.hMFCHandle, setParam);
    }
    if (pCommonParam->Bitrate != (int)pExynosOutputPort->portDefinition.format.video.nBitrate) {
        setParam = pExynosOutputPort->portDefinition.format.video.nBitrate;
        pEncOps->Set_BitRate(pH264Enc->hMFCH264Handle.hMFCHandle, setParam);
    }
    if (pH264Param->FrameRate != (int)((pExynosOutputPort->portDefinition.format.video.xFramerate) >> 16)) {
        setParam = (pExynosInputPort->portDefinition.format.video.xFramerate) >> 16;
        pEncOps->Set_FrameRate(pH264Enc->hMFCH264Handle.hMFCHandle, setParam);
    }

    Set_H264Enc_Param(pExynosComponent);
}

OMX_ERRORTYPE Exynos_H264Enc_GetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nParamIndex,
    OMX_INOUT OMX_PTR     pComponentParameterStructure)
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
    case OMX_IndexParamVideoAvc:
    {
        OMX_VIDEO_PARAM_AVCTYPE *pDstAVCComponent = (OMX_VIDEO_PARAM_AVCTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_AVCTYPE *pSrcAVCComponent = NULL;
        EXYNOS_H264ENC_HANDLE   *pH264Enc = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pDstAVCComponent, sizeof(OMX_VIDEO_PARAM_AVCTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstAVCComponent->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pH264Enc = (EXYNOS_H264ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        pSrcAVCComponent = &pH264Enc->AVCComponent[pDstAVCComponent->nPortIndex];

        Exynos_OSAL_Memcpy(pDstAVCComponent, pSrcAVCComponent, sizeof(OMX_VIDEO_PARAM_AVCTYPE));
    }
        break;
    case OMX_IndexParamStandardComponentRole:
    {
        OMX_PARAM_COMPONENTROLETYPE *pComponentRole = (OMX_PARAM_COMPONENTROLETYPE *)pComponentParameterStructure;
        ret = Exynos_OMX_Check_SizeVersion(pComponentRole, sizeof(OMX_PARAM_COMPONENTROLETYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        Exynos_OSAL_Strcpy((char *)pComponentRole->cRole, EXYNOS_OMX_COMPONENT_H264_ENC_ROLE);
    }
        break;
    case OMX_IndexParamVideoProfileLevelQuerySupported:
    {
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *pDstProfileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE*)pComponentParameterStructure;
        EXYNOS_OMX_VIDEO_PROFILELEVEL *pProfileLevel = NULL;
        OMX_U32 maxProfileLevelNum = 0;

        ret = Exynos_OMX_Check_SizeVersion(pDstProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstProfileLevel->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pProfileLevel = supportedAVCProfileLevels;
        maxProfileLevelNum = sizeof(supportedAVCProfileLevels) / sizeof(EXYNOS_OMX_VIDEO_PROFILELEVEL);

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
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *pDstProfileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE*)pComponentParameterStructure;
        OMX_VIDEO_PARAM_AVCTYPE *pSrcAVCComponent = NULL;
        EXYNOS_H264ENC_HANDLE *pH264Enc = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pDstProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstProfileLevel->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pH264Enc = (EXYNOS_H264ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        pSrcAVCComponent = &pH264Enc->AVCComponent[pDstProfileLevel->nPortIndex];

        pDstProfileLevel->eProfile = pSrcAVCComponent->eProfile;
        pDstProfileLevel->eLevel = pSrcAVCComponent->eLevel;
    }
        break;
    case OMX_IndexParamVideoErrorCorrection:
    {
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pDstErrorCorrectionType = (OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pSrcErrorCorrectionType = NULL;
        EXYNOS_H264ENC_HANDLE *pH264Enc = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pDstErrorCorrectionType, sizeof(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstErrorCorrectionType->nPortIndex != OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pH264Enc = (EXYNOS_H264ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        pSrcErrorCorrectionType = &pH264Enc->errorCorrectionType[OUTPUT_PORT_INDEX];

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

OMX_ERRORTYPE Exynos_H264Enc_SetParameter(
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
    case OMX_IndexParamVideoAvc:
    {
        OMX_VIDEO_PARAM_AVCTYPE *pDstAVCComponent = NULL;
        OMX_VIDEO_PARAM_AVCTYPE *pSrcAVCComponent = (OMX_VIDEO_PARAM_AVCTYPE *)pComponentParameterStructure;
        EXYNOS_H264ENC_HANDLE   *pH264Enc = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pSrcAVCComponent, sizeof(OMX_VIDEO_PARAM_AVCTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcAVCComponent->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pH264Enc = (EXYNOS_H264ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        pDstAVCComponent = &pH264Enc->AVCComponent[pSrcAVCComponent->nPortIndex];

        Exynos_OSAL_Memcpy(pDstAVCComponent, pSrcAVCComponent, sizeof(OMX_VIDEO_PARAM_AVCTYPE));
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

        if (!Exynos_OSAL_Strcmp((char*)pComponentRole->cRole, EXYNOS_OMX_COMPONENT_H264_ENC_ROLE)) {
            pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX].portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
        } else {
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }
    }
        break;
    case OMX_IndexParamVideoProfileLevelCurrent:
    {
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *pSrcProfileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_AVCTYPE *pDstAVCComponent = NULL;
        EXYNOS_H264ENC_HANDLE *pH264Enc = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pSrcProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcProfileLevel->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pH264Enc = (EXYNOS_H264ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;

        pDstAVCComponent = &pH264Enc->AVCComponent[pSrcProfileLevel->nPortIndex];
        pDstAVCComponent->eProfile = pSrcProfileLevel->eProfile;
        pDstAVCComponent->eLevel = pSrcProfileLevel->eLevel;
    }
        break;
    case OMX_IndexParamVideoErrorCorrection:
    {
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pSrcErrorCorrectionType = (OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pDstErrorCorrectionType = NULL;
        EXYNOS_H264ENC_HANDLE *pH264Enc = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pSrcErrorCorrectionType, sizeof(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcErrorCorrectionType->nPortIndex != OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pH264Enc = (EXYNOS_H264ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        pDstErrorCorrectionType = &pH264Enc->errorCorrectionType[OUTPUT_PORT_INDEX];

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

OMX_ERRORTYPE Exynos_H264Enc_GetConfig(
    OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE nIndex,
    OMX_PTR pComponentConfigStructure)
{
    OMX_ERRORTYPE             ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent    = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;
    EXYNOS_H264ENC_HANDLE    *pH264Enc         = NULL;

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
    pH264Enc = (EXYNOS_H264ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;

    switch (nIndex) {
    case OMX_IndexConfigVideoAVCIntraPeriod:
    {
        OMX_VIDEO_CONFIG_AVCINTRAPERIOD *pAVCIntraPeriod = (OMX_VIDEO_CONFIG_AVCINTRAPERIOD *)pComponentConfigStructure;
        OMX_U32           portIndex = pAVCIntraPeriod->nPortIndex;

        if ((portIndex != OUTPUT_PORT_INDEX)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        } else {
            pAVCIntraPeriod->nIDRPeriod = pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].nPFrames + 1;
            pAVCIntraPeriod->nPFrames = pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].nPFrames;
        }
    }
        break;
    default:
        ret = Exynos_OMX_VideoEncodeGetConfig(hComponent, nIndex, pComponentConfigStructure);
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_H264Enc_SetConfig(
    OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE nIndex,
    OMX_PTR pComponentConfigStructure)
{
    OMX_ERRORTYPE                  ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE             *pOMXComponent    = NULL;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc        = NULL;
    EXYNOS_H264ENC_HANDLE         *pH264Enc         = NULL;

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

    pVideoEnc = ((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle);
    pH264Enc = (EXYNOS_H264ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;

    switch (nIndex) {
    case OMX_IndexConfigVideoIntraPeriod:
    {
        EXYNOS_OMX_VIDEOENC_COMPONENT *pVEncBase = ((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle);
        OMX_U32 nPFrames = (*((OMX_U32 *)pComponentConfigStructure)) - 1;

        pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].nPFrames = nPFrames;

        ret = OMX_ErrorNone;
    }
        break;
    case OMX_IndexConfigVideoAVCIntraPeriod:
    {
        OMX_VIDEO_CONFIG_AVCINTRAPERIOD *pAVCIntraPeriod = (OMX_VIDEO_CONFIG_AVCINTRAPERIOD *)pComponentConfigStructure;
        OMX_U32           portIndex = pAVCIntraPeriod->nPortIndex;

        if ((portIndex != OUTPUT_PORT_INDEX)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        } else {
            if (pAVCIntraPeriod->nIDRPeriod == (pAVCIntraPeriod->nPFrames + 1))
                pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].nPFrames = pAVCIntraPeriod->nPFrames;
            else {
                ret = OMX_ErrorBadParameter;
                goto EXIT;
            }
        }
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

OMX_ERRORTYPE Exynos_H264Enc_GetExtensionIndex(
    OMX_IN OMX_HANDLETYPE  hComponent,
    OMX_IN OMX_STRING      cParameterName,
    OMX_OUT OMX_INDEXTYPE *pIndexType)
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

OMX_ERRORTYPE Exynos_H264Enc_ComponentRoleEnum(OMX_HANDLETYPE hComponent, OMX_U8 *cRole, OMX_U32 nIndex)
{
    OMX_ERRORTYPE               ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE          *pOMXComponent    = NULL;
    EXYNOS_OMX_BASECOMPONENT   *pExynosComponent = NULL;

    FunctionIn();

    if ((hComponent == NULL) || (cRole == NULL)) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (nIndex == (MAX_COMPONENT_ROLE_NUM-1)) {
        Exynos_OSAL_Strcpy((char *)cRole, EXYNOS_OMX_COMPONENT_H264_ENC_ROLE);
        ret = OMX_ErrorNone;
    } else {
        ret = OMX_ErrorNoMore;
    }

EXIT:
    FunctionOut();

    return ret;
}

static OMX_ERRORTYPE Exynos_H264Enc_EncodeThread(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE                  ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE             *pOMXComponent    = (OMX_COMPONENTTYPE *)hComponent;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc        = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_H264ENC_HANDLE         *pH264Enc         = (EXYNOS_H264ENC_HANDLE *)pVideoEnc->hCodecHandle;
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
                    if (pInbufOps->Run(pH264Enc->hMFCH264Handle.hMFCHandle) != VIDEO_ERROR_NONE) {
                        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to run input buffer");
                        ret = OMX_ErrorInsufficientResources;
                        goto EXIT;
                    }

                    pVideoEnc->bInputRun = OMX_TRUE;
                }

                pVideoEnc->pOutbuf = pOutbufOps->Dequeue(pH264Enc->hMFCH264Handle.hMFCHandle);
                pInbufOps->Dequeue(pH264Enc->hMFCH264Handle.hMFCHandle);
            }

            pH264Enc->hMFCH264Handle.returnCodec = VIDEO_TRUE;
            Exynos_OSAL_SemaphorePost(pVideoEnc->NBEncThread.hEncFrameEnd);
        }
    }

EXIT:
    FunctionOut();
    Exynos_OSAL_ThreadExit(NULL);

    return ret;
}

/* MFC Init */
OMX_ERRORTYPE Exynos_H264Enc_Init(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE                  ret              = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc        = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_OMX_BASEPORT           *pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    ExynosVideoEncOps             *pEncOps          = NULL;
    ExynosVideoEncBufferOps       *pInbufOps        = NULL;
    ExynosVideoEncBufferOps       *pOutbufOps       = NULL;
    EXYNOS_H264ENC_HANDLE         *pH264Enc         = NULL;

    OMX_PTR hMFCHandle = NULL;
    OMX_S32 returnCodec = 0;
    int i;

    ExynosVideoEncParam encParam;
    ExynosVideoBuffer bufferInfo;
    ExynosVideoGeometry bufferConf;

    CSC_METHOD csc_method = CSC_METHOD_SW;

    FunctionIn();

    pH264Enc = (EXYNOS_H264ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    pH264Enc->hMFCH264Handle.bConfiguredMFC = OMX_FALSE;
    pExynosComponent->bUseFlagEOF = OMX_FALSE;
    pExynosComponent->bSaveFlagEOS = OMX_FALSE;

    /* alloc ops structure */
    pEncOps = (ExynosVideoEncOps *)Exynos_OSAL_Malloc(sizeof(*pEncOps));
    pInbufOps = (ExynosVideoEncBufferOps *)Exynos_OSAL_Malloc(sizeof(*pInbufOps));
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
    pH264Enc->hMFCH264Handle.hMFCHandle = pVideoEnc->pEncOps->Init();
    if (pH264Enc->hMFCH264Handle.hMFCHandle == NULL) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to allocate context buffer");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    Set_H264Enc_Param(pExynosComponent);
    encParam = pVideoEnc->encParam;
    if (pEncOps->Set_EncParam) {
        if(pEncOps->Set_EncParam(pH264Enc->hMFCH264Handle.hMFCHandle, &encParam) != VIDEO_ERROR_NONE) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to set geometry for input buffer");
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }
    }

    /* set input buffer shareable */
    if(pExynosInputPort->portDefinition.format.video.eColorFormat == OMX_SEC_COLOR_FormatNV12LVirtualAddress) {
        if (pInbufOps->Set_Shareable) {
            if (pInbufOps->Set_Shareable(pH264Enc->hMFCH264Handle.hMFCHandle) != VIDEO_ERROR_NONE) {
                Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to set input buffer shareable");
                ret = OMX_ErrorInsufficientResources;
                goto EXIT;
            }
        }
    }

    Exynos_OSAL_Memset(&bufferConf, 0, sizeof(bufferConf));

    /* input buffer info: only 3 config values needed */
    bufferConf.nFrameWidth = pExynosInputPort->portDefinition.format.video.nFrameWidth;
    bufferConf.nFrameHeight = pExynosInputPort->portDefinition.format.video.nFrameHeight;
    bufferConf.eColorFormat = pVideoEnc->encParam.commonParam.FrameMap;

    /* set input buffer geometry */
    if (pInbufOps->Set_Geometry) {
        if (pInbufOps->Set_Geometry(pH264Enc->hMFCH264Handle.hMFCHandle, &bufferConf) != VIDEO_ERROR_NONE) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to set geometry for input buffer");
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }
    }

    /* cacheable for input */
    if (pInbufOps->Enable_Cacheable) {
        if (pInbufOps->Enable_Cacheable(pH264Enc->hMFCH264Handle.hMFCHandle) != VIDEO_ERROR_NONE)
            Exynos_OSAL_Log(EXYNOS_LOG_WARNING, "Failed to enable cacheable property for input buffer");
    }

    /* setup input buffer */
    if (pInbufOps->Setup(pH264Enc->hMFCH264Handle.hMFCHandle, MFC_INPUT_BUFFER_NUM_MAX) != VIDEO_ERROR_NONE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to setup input buffer");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    for (i = 0; i < MFC_INPUT_BUFFER_NUM_MAX; i++) {
         pInbufOps->Get_BufferInfo(pH264Enc->hMFCH264Handle.hMFCHandle, i, &bufferInfo);

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
                                                   Exynos_H264Enc_EncodeThread,
                                                   pOMXComponent)) {
        pH264Enc->hMFCH264Handle.returnCodec = VIDEO_TRUE;
    }

    Exynos_OSAL_Memset(pExynosComponent->timeStamp, -19771003, sizeof(OMX_TICKS) * MAX_TIMESTAMP);
    Exynos_OSAL_Memset(pExynosComponent->nFlags, 0, sizeof(OMX_U32) * MAX_FLAGS);
    pH264Enc->hMFCH264Handle.indexTimestamp = 0;

    pVideoEnc->csc_handle = csc_init(csc_method);

EXIT:
    FunctionOut();

    return ret;
}

/* MFC Terminate */
OMX_ERRORTYPE Exynos_H264Enc_Terminate(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE                  ret              = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc        = ((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle);
    ExynosVideoEncOps             *pEncOps          = pVideoEnc->pEncOps;
    ExynosVideoEncBufferOps       *pInbufOps        = pVideoEnc->pInbufOps;
    ExynosVideoEncBufferOps       *pOutbufOps       = pVideoEnc->pOutbufOps;
    EXYNOS_H264ENC_HANDLE         *pH264Enc         = NULL;
    OMX_PTR                        hMFCHandle       = NULL;

    FunctionIn();

    pH264Enc = (EXYNOS_H264ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    hMFCHandle = pH264Enc->hMFCH264Handle.hMFCHandle;

    pExynosComponent->processData[INPUT_PORT_INDEX].dataBuffer = NULL;
    pExynosComponent->processData[INPUT_PORT_INDEX].allocSize = 0;

    if (pVideoEnc->NBEncThread.hNBEncodeThread != NULL) {
        pVideoEnc->NBEncThread.bExitEncodeThread = OMX_TRUE;
        Exynos_OSAL_SemaphorePost(pVideoEnc->NBEncThread.hEncFrameStart);
        Exynos_OSAL_ThreadTerminate(pVideoEnc->NBEncThread.hNBEncodeThread);
        pVideoEnc->NBEncThread.hNBEncodeThread = NULL;
    }

    if (pVideoEnc->NBEncThread.hEncFrameEnd != NULL) {
        Exynos_OSAL_SemaphoreTerminate(pVideoEnc->NBEncThread.hEncFrameEnd);
        pVideoEnc->NBEncThread.hEncFrameEnd = NULL;
    }

    if (pVideoEnc->NBEncThread.hEncFrameStart != NULL) {
        Exynos_OSAL_SemaphoreTerminate(pVideoEnc->NBEncThread.hEncFrameStart);
        pVideoEnc->NBEncThread.hEncFrameStart = NULL;
    }

    if (hMFCHandle != NULL) {
        pInbufOps->Stop(hMFCHandle);
        pOutbufOps->Stop(hMFCHandle);
        pEncOps->Finalize(hMFCHandle);

        Exynos_OSAL_Free(pInbufOps);
        Exynos_OSAL_Free(pOutbufOps);
        Exynos_OSAL_Free(pEncOps);

        pH264Enc->hMFCH264Handle.hMFCHandle = NULL;
    }

    if (pVideoEnc->csc_handle != NULL) {
        csc_deinit(pVideoEnc->csc_handle);
        pVideoEnc->csc_handle = NULL;
    }

EXIT:
    FunctionOut();

    return ret;
}

static OMX_ERRORTYPE Exynos_H264Enc_Configure(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pInputData, EXYNOS_OMX_DATA *pOutputData)
{
    OMX_ERRORTYPE                  ret               = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent  = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc         = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_H264ENC_HANDLE         *pH264Enc          = (EXYNOS_H264ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
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
        bufferConf.eCompressionFormat = VIDEO_CODING_AVC;
        if (pOutbufOps->Set_Geometry(pH264Enc->hMFCH264Handle.hMFCHandle, &bufferConf) != VIDEO_ERROR_NONE) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to set geometry for output buffer");
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }
    }

    /* cacheable for output */
    if (pOutbufOps->Enable_Cacheable) {
        if (pOutbufOps->Enable_Cacheable(pH264Enc->hMFCH264Handle.hMFCHandle) != VIDEO_ERROR_NONE)
            Exynos_OSAL_Log(EXYNOS_LOG_WARNING, "Failed to enable cacheable property for output buffer");
    }

    if (pOutbufOps->Setup(pH264Enc->hMFCH264Handle.hMFCHandle, MFC_OUTPUT_BUFFER_NUM_MAX) != VIDEO_ERROR_NONE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to setup output buffer");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    if (pOutbufOps->Enqueue_All) {
        if (pOutbufOps->Enqueue_All(pH264Enc->hMFCH264Handle.hMFCHandle) != VIDEO_ERROR_NONE) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to flush all output buffer");
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }
    }

    /* start header encoding */
    if (pOutbufOps->Run(pH264Enc->hMFCH264Handle.hMFCHandle) != VIDEO_ERROR_NONE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to run output buffer for header parsing");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    pVideoEnc->pOutbuf = pOutbufOps->Dequeue(pH264Enc->hMFCH264Handle.hMFCHandle);
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

    pH264Enc->hMFCH264Handle.bConfiguredMFC = OMX_TRUE;
    pH264Enc->hMFCH264Handle.returnCodec = VIDEO_TRUE;

EXIT:
    return ret;
}

/* nonblock */
static OMX_ERRORTYPE Exynos_H264Enc_Encode(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pInputData, EXYNOS_OMX_DATA *pOutputData)
{
    OMX_ERRORTYPE                  ret               = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent  = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc         = ((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle);
    EXYNOS_H264ENC_HANDLE         *pH264Enc          = (EXYNOS_H264ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    EXYNOS_OMX_BASEPORT           *pExynosInputPort  = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT           *pExynosPort       = NULL;
    ExynosVideoEncOps             *pEncOps           = pVideoEnc->pEncOps;
    ExynosVideoEncBufferOps       *pInbufOps         = pVideoEnc->pInbufOps;
    ExynosVideoEncBufferOps       *pOutbufOps        = pVideoEnc->pOutbufOps;

    pVideoEnc->NBEncThread.oneFrameSize = pInputData->dataLen;
    OMX_PTR ppBuf[3];

    FunctionIn();

    if (pH264Enc->hMFCH264Handle.bConfiguredMFC == OMX_FALSE) {
        ret = Exynos_H264Enc_Configure(pOMXComponent, pInputData, pOutputData);
        if (ret != OMX_ErrorNone) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: failed to configure encoder", __func__);
            ret = OMX_ErrorUndefined;
            goto EXIT;
        } else {
            OMX_U8 *p = NULL;
            int iSpsSize = 0;
            int iPpsSize = 0;

            /* Calculate sps/pps size if needed */
            p = FindDelimiter((OMX_U8 *)(pOutputData->dataBuffer + 4), pOutputData->dataLen- 4);

            iSpsSize = (unsigned int)p - (unsigned int)pOutputData->dataBuffer;
            pH264Enc->hMFCH264Handle.headerData.pHeaderSPS = (OMX_PTR)pOutputData->dataBuffer;
            pH264Enc->hMFCH264Handle.headerData.SPSLen = iSpsSize;

            iPpsSize = pOutputData->dataLen - iSpsSize;
            pH264Enc->hMFCH264Handle.headerData.pHeaderPPS = (OMX_U8 *)pOutputData->dataBuffer + iSpsSize;
            pH264Enc->hMFCH264Handle.headerData.PPSLen = iPpsSize;

            pOutputData->timeStamp = 0;
            pOutputData->nFlags |= OMX_BUFFERFLAG_CODECCONFIG;
            pOutputData->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;

            pH264Enc->hMFCH264Handle.bConfiguredMFC = OMX_TRUE;

            ret = OMX_ErrorInputDataEncodeYet;
            goto EXIT;
        }
    }

    if ((pInputData->nFlags & OMX_BUFFERFLAG_ENDOFFRAME) && (pExynosComponent->bUseFlagEOF == OMX_FALSE))
        pExynosComponent->bUseFlagEOF = OMX_TRUE;

    if (pVideoEnc->NBEncThread.oneFrameSize <= 0) {
        pOutputData->timeStamp = pInputData->timeStamp;
        pOutputData->nFlags = pInputData->nFlags;
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    pExynosPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];

    if (((pInputData->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) ||
         (pExynosComponent->getAllDelayBuffer == OMX_TRUE)) {
        pH264Enc->hMFCH264Handle.pMFCYUVVirBuffer[0] = pVideoEnc->MFCEncInputBuffer[pVideoEnc->indexInputBuffer].YVirAddr;
        pH264Enc->hMFCH264Handle.pMFCYUVVirBuffer[1] = pVideoEnc->MFCEncInputBuffer[pVideoEnc->indexInputBuffer].CVirAddr;
    } else {
        switch (pExynosPort->portDefinition.format.video.eColorFormat) {
        case OMX_SEC_COLOR_FormatNV12TPhysicalAddress:
        case OMX_SEC_COLOR_FormatNV12LPhysicalAddress:
        case OMX_SEC_COLOR_FormatNV21LPhysicalAddress:
#ifndef USE_METADATABUFFERTYPE
            /* USE_FIMC_FRAME_BUFFER */
            Exynos_OSAL_Memcpy(&(pH264Enc->hMFCH264Handle.pMFCYUVVirBuffer[0]), pInputData->dataBuffer, sizeof(OMX_PTR));
            Exynos_OSAL_Memcpy(&(pH264Enc->hMFCH264Handle.pMFCYUVVirBuffer[1]), pInputData->dataBuffer + sizeof(OMX_PTR), sizeof(OMX_PTR));
#else
            Exynos_OSAL_GetInfoFromMetaData(pInputData, ppBuf);

            Exynos_OSAL_Memcpy(&(pH264Enc->hMFCH264Handle.pMFCYUVVirBuffer[0]), ppBuf[0], sizeof(OMX_PTR));
            Exynos_OSAL_Memcpy(&(pH264Enc->hMFCH264Handle.pMFCYUVVirBuffer[1]), ppBuf[1], sizeof(OMX_PTR));
#endif
            break;
        case OMX_SEC_COLOR_FormatNV12LVirtualAddress:
            pH264Enc->hMFCH264Handle.pMFCYUVVirBuffer[0] = *((void **)pInputData->dataBuffer);
            pH264Enc->hMFCH264Handle.pMFCYUVVirBuffer[1] = pH264Enc->hMFCH264Handle.pMFCYUVVirBuffer[0] +
                                                          (ALIGN_TO_16B(pExynosPort->portDefinition.format.video.nFrameWidth) *
                                                           ALIGN_TO_16B(pExynosPort->portDefinition.format.video.nFrameHeight));
            break;
        default:
            pH264Enc->hMFCH264Handle.pMFCYUVPhyBuffer[0] = pVideoEnc->MFCEncInputBuffer[pVideoEnc->indexInputBuffer].YPhyAddr;
            pH264Enc->hMFCH264Handle.pMFCYUVVirBuffer[0] = pVideoEnc->MFCEncInputBuffer[pVideoEnc->indexInputBuffer].YVirAddr;
            pH264Enc->hMFCH264Handle.pMFCYUVPhyBuffer[1] = pVideoEnc->MFCEncInputBuffer[pVideoEnc->indexInputBuffer].CPhyAddr;
            pH264Enc->hMFCH264Handle.pMFCYUVVirBuffer[1] = pVideoEnc->MFCEncInputBuffer[pVideoEnc->indexInputBuffer].CVirAddr;
            break;
        }
    }

    pH264Enc->hMFCH264Handle.pMFCYUVDataSize[0] = ALIGN_TO_16B(pExynosPort->portDefinition.format.video.nFrameWidth) *
                                                  ALIGN_TO_16B(pExynosPort->portDefinition.format.video.nFrameHeight);
    pH264Enc->hMFCH264Handle.pMFCYUVDataSize[1] = pH264Enc->hMFCH264Handle.pMFCYUVDataSize[0] / 2;

    pExynosComponent->timeStamp[pH264Enc->hMFCH264Handle.indexTimestamp] = pInputData->timeStamp;
    pExynosComponent->nFlags[pH264Enc->hMFCH264Handle.indexTimestamp] = pInputData->nFlags;

    if ((pH264Enc->hMFCH264Handle.returnCodec == VIDEO_TRUE) &&
        (pVideoEnc->bFirstFrame == OMX_FALSE)) {
        OMX_S32 indexTimestamp = 0;

        /* wait for mfc encode done */
        if (pVideoEnc->NBEncThread.bEncoderRun != OMX_FALSE) {
            Exynos_OSAL_SemaphoreWait(pVideoEnc->NBEncThread.hEncFrameEnd);
            pVideoEnc->NBEncThread.bEncoderRun = OMX_FALSE;
        }

        Exynos_OSAL_SleepMillisec(0);

        indexTimestamp = pEncOps->Get_FrameTag(pH264Enc->hMFCH264Handle.hMFCHandle);
        if ((indexTimestamp < 0) || (indexTimestamp >= MAX_TIMESTAMP)) {
            pOutputData->timeStamp = pInputData->timeStamp;
            pOutputData->nFlags = pInputData->nFlags;
        } else {
            pOutputData->timeStamp = pExynosComponent->timeStamp[indexTimestamp];
            pOutputData->nFlags = pExynosComponent->nFlags[indexTimestamp];
        }

        if (pVideoEnc->pOutbuf != NULL) {
            /** Fill Output Buffer **/
            pOutputData->dataBuffer = pVideoEnc->pOutbuf->planes[0].addr;
            pOutputData->allocSize = pVideoEnc->pOutbuf->planes[0].allocSize;
            pOutputData->dataLen = pVideoEnc->pOutbuf->planes[0].dataSize;
            pOutputData->usedDataLen = 0;

            pOutputData->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;
            if (pVideoEnc->pOutbuf->frameType == VIDEO_FRAME_I)
                pOutputData->nFlags |= OMX_BUFFERFLAG_SYNCFRAME;

            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "MFC Encode OK!");

            ret = OMX_ErrorNone;
        } else {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: failed to get output buffer, ret:%d", __FUNCTION__, pH264Enc->hMFCH264Handle.returnCodec);
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
        Change_H264Enc_Param(pExynosComponent);
        pVideoEnc->configChange = OMX_FALSE;
    }

    if (pEncOps->Set_FrameTag(pH264Enc->hMFCH264Handle.hMFCHandle, pH264Enc->hMFCH264Handle.indexTimestamp) != VIDEO_ERROR_NONE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to set frame tag");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    if (pOutbufOps->Enqueue(pH264Enc->hMFCH264Handle.hMFCHandle,
                           (unsigned char **)&pVideoEnc->pOutbuf->planes[0].addr,
                           (unsigned int *)&pVideoEnc->pOutbuf->planes[0].dataSize,
                            1, NULL) != VIDEO_ERROR_NONE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to enqueue output buffer");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    if (pInbufOps->Enqueue(pH264Enc->hMFCH264Handle.hMFCHandle,
                          (unsigned char **)pH264Enc->hMFCH264Handle.pMFCYUVVirBuffer,
                          (unsigned int *)pH264Enc->hMFCH264Handle.pMFCYUVDataSize,
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
    pH264Enc->hMFCH264Handle.indexTimestamp++;
    pH264Enc->hMFCH264Handle.indexTimestamp %= MAX_TIMESTAMP;
    pVideoEnc->bFirstFrame = OMX_FALSE;
    Exynos_OSAL_SleepMillisec(0);

EXIT:
    FunctionOut();

    return ret;
}

/* MFC Encode */
OMX_ERRORTYPE Exynos_H264Enc_BufferProcess(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pInputData, EXYNOS_OMX_DATA *pOutputData)
{
    OMX_ERRORTYPE               ret               = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT   *pExynosComponent  = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_H264ENC_HANDLE      *pH264Enc          = (EXYNOS_H264ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
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

    ret = Exynos_H264Enc_Encode(pOMXComponent, pInputData, pOutputData);
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

        /* pOutputData->usedDataLen = 0; */
        pOutputData->remainDataLen = pOutputData->dataLen - pOutputData->usedDataLen;
    }

EXIT:
    FunctionOut();

    return ret;
}

OSCL_EXPORT_REF OMX_ERRORTYPE Exynos_OMX_ComponentInit(OMX_HANDLETYPE hComponent, OMX_STRING componentName)
{
    OMX_ERRORTYPE                  ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE             *pOMXComponent    = NULL;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = NULL;
    EXYNOS_OMX_BASEPORT           *pExynosPort      = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc        = NULL;
    EXYNOS_H264ENC_HANDLE         *pH264Enc         = NULL;
    int i = 0;

    FunctionIn();

    if ((hComponent == NULL) || (componentName == NULL)) {
        ret = OMX_ErrorBadParameter;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorBadParameter, Line:%d", __LINE__);
        goto EXIT;
    }
    if (Exynos_OSAL_Strcmp(EXYNOS_OMX_COMPONENT_H264_ENC, componentName) != 0) {
        ret = OMX_ErrorBadParameter;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorBadParameter, componentName:%s, Line:%d", componentName, __LINE__);
        goto EXIT;
    }

    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_VideoEncodeComponentInit(pOMXComponent);
    if (ret != OMX_ErrorNone) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_Error, Line:%d", __LINE__);
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    pExynosComponent->codecType = HW_VIDEO_ENC_CODEC;

    pExynosComponent->componentName = (OMX_STRING)Exynos_OSAL_Malloc(MAX_OMX_COMPONENT_NAME_SIZE);
    if (pExynosComponent->componentName == NULL) {
        Exynos_OMX_VideoEncodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    Exynos_OSAL_Memset(pExynosComponent->componentName, 0, MAX_OMX_COMPONENT_NAME_SIZE);

    pH264Enc = Exynos_OSAL_Malloc(sizeof(EXYNOS_H264ENC_HANDLE));
    if (pH264Enc == NULL) {
        Exynos_OMX_VideoEncodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    Exynos_OSAL_Memset(pH264Enc, 0, sizeof(EXYNOS_H264ENC_HANDLE));
    pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    pVideoEnc->hCodecHandle = (OMX_HANDLETYPE)pH264Enc;

    Exynos_OSAL_Strcpy(pExynosComponent->componentName, EXYNOS_OMX_COMPONENT_H264_ENC);
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
    pExynosPort->portDefinition.format.video.nStride = 0; /*DEFAULT_FRAME_WIDTH;*/
    pExynosPort->portDefinition.nBufferSize = DEFAULT_VIDEO_INPUT_BUFFER_SIZE;
    pExynosPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
    Exynos_OSAL_Memset(pExynosPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
    Exynos_OSAL_Strcpy(pExynosPort->portDefinition.format.video.cMIMEType, "raw/video");
    pExynosPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
    pExynosPort->portDefinition.bEnabled = OMX_TRUE;

    /* Output port */
    pExynosPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    pExynosPort->portDefinition.format.video.nFrameWidth = DEFAULT_FRAME_WIDTH;
    pExynosPort->portDefinition.format.video.nFrameHeight= DEFAULT_FRAME_HEIGHT;
    pExynosPort->portDefinition.format.video.nStride = 0; /*DEFAULT_FRAME_WIDTH;*/
    pExynosPort->portDefinition.nBufferSize = DEFAULT_VIDEO_OUTPUT_BUFFER_SIZE;
    pExynosPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
    Exynos_OSAL_Memset(pExynosPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
    Exynos_OSAL_Strcpy(pExynosPort->portDefinition.format.video.cMIMEType, "video/avc");
    pExynosPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    pExynosPort->portDefinition.bEnabled = OMX_TRUE;

    for(i = 0; i < ALL_PORT_NUM; i++) {
        INIT_SET_SIZE_VERSION(&pH264Enc->AVCComponent[i], OMX_VIDEO_PARAM_AVCTYPE);
        pH264Enc->AVCComponent[i].nPortIndex = i;
        pH264Enc->AVCComponent[i].eProfile   = OMX_VIDEO_AVCProfileBaseline;
        pH264Enc->AVCComponent[i].eLevel     = OMX_VIDEO_AVCLevel31;

        pH264Enc->AVCComponent[i].nPFrames = 20;
    }

    pOMXComponent->GetParameter      = &Exynos_H264Enc_GetParameter;
    pOMXComponent->SetParameter      = &Exynos_H264Enc_SetParameter;
    pOMXComponent->GetConfig         = &Exynos_H264Enc_GetConfig;
    pOMXComponent->SetConfig         = &Exynos_H264Enc_SetConfig;
    pOMXComponent->GetExtensionIndex = &Exynos_H264Enc_GetExtensionIndex;
    pOMXComponent->ComponentRoleEnum = &Exynos_H264Enc_ComponentRoleEnum;
    pOMXComponent->ComponentDeInit   = &Exynos_OMX_ComponentDeinit;

    pExynosComponent->exynos_mfc_componentInit      = &Exynos_H264Enc_Init;
    pExynosComponent->exynos_mfc_componentTerminate = &Exynos_H264Enc_Terminate;
    pExynosComponent->exynos_mfc_bufferProcess      = &Exynos_H264Enc_BufferProcess;
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
    EXYNOS_H264ENC_HANDLE      *pH264Enc         = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    Exynos_OSAL_Free(pExynosComponent->componentName);
    pExynosComponent->componentName = NULL;

    pH264Enc = (EXYNOS_H264ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    if (pH264Enc != NULL) {
        Exynos_OSAL_Free(pH264Enc);
        pH264Enc = ((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle = NULL;
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
