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
 * \file      ExynosCameraHWInterface2.cpp
 * \brief     source file for Android Camera API 2.0 HAL
 * \author    Sungjoong Kang(sj3.kang@samsung.com)
 * \date      2012/07/10
 *
 * <b>Revision History: </b>
 * - 2012/05/31 : Sungjoong Kang(sj3.kang@samsung.com) \n
 *   Initial Release
 *
 * - 2012/07/10 : Sungjoong Kang(sj3.kang@samsung.com) \n
 *   2nd Release
 *
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "ExynosCameraHAL2"
#include <utils/Log.h>

#include "ExynosCameraHWInterface2.h"
#include "exynos_format.h"



namespace android {


void m_savePostView(const char *fname, uint8_t *buf, uint32_t size)
{
    int nw;
    int cnt = 0;
    uint32_t written = 0;

    ALOGV("opening file [%s], address[%x], size(%d)", fname, (unsigned int)buf, size);
    int fd = open(fname, O_RDWR | O_CREAT, 0644);
    if (fd < 0) {
        ALOGE("failed to create file [%s]: %s", fname, strerror(errno));
        return;
    }

    ALOGV("writing %d bytes to file [%s]", size, fname);
    while (written < size) {
        nw = ::write(fd, buf + written, size - written);
        if (nw < 0) {
            ALOGE("failed to write to file %d [%s]: %s",written,fname, strerror(errno));
            break;
        }
        written += nw;
        cnt++;
    }
    ALOGV("done writing %d bytes to file [%s] in %d passes",size, fname, cnt);
    ::close(fd);
}

int get_pixel_depth(uint32_t fmt)
{
    int depth = 0;

    switch (fmt) {
    case V4L2_PIX_FMT_JPEG:
        depth = 8;
        break;

    case V4L2_PIX_FMT_NV12:
    case V4L2_PIX_FMT_NV21:
    case V4L2_PIX_FMT_YUV420:
    case V4L2_PIX_FMT_YVU420M:
    case V4L2_PIX_FMT_NV12M:
    case V4L2_PIX_FMT_NV12MT:
        depth = 12;
        break;

    case V4L2_PIX_FMT_RGB565:
    case V4L2_PIX_FMT_YUYV:
    case V4L2_PIX_FMT_YVYU:
    case V4L2_PIX_FMT_UYVY:
    case V4L2_PIX_FMT_VYUY:
    case V4L2_PIX_FMT_NV16:
    case V4L2_PIX_FMT_NV61:
    case V4L2_PIX_FMT_YUV422P:
    case V4L2_PIX_FMT_SBGGR10:
    case V4L2_PIX_FMT_SBGGR12:
    case V4L2_PIX_FMT_SBGGR16:
        depth = 16;
        break;

    case V4L2_PIX_FMT_RGB32:
        depth = 32;
        break;
    default:
        ALOGE("Get depth failed(format : %d)", fmt);
        break;
    }

    return depth;
}

int cam_int_s_fmt(node_info_t *node)
{
    struct v4l2_format v4l2_fmt;
    unsigned int framesize;
    int ret;

    memset(&v4l2_fmt, 0, sizeof(struct v4l2_format));

    v4l2_fmt.type = node->type;
    framesize = (node->width * node->height * get_pixel_depth(node->format)) / 8;

    if (node->planes >= 1) {
        v4l2_fmt.fmt.pix_mp.width       = node->width;
        v4l2_fmt.fmt.pix_mp.height      = node->height;
        v4l2_fmt.fmt.pix_mp.pixelformat = node->format;
        v4l2_fmt.fmt.pix_mp.field       = V4L2_FIELD_ANY;
    } else {
        ALOGE("%s:S_FMT, Out of bound : Number of element plane",__FUNCTION__);
    }

    /* Set up for capture */
    ret = exynos_v4l2_s_fmt(node->fd, &v4l2_fmt);

    if (ret < 0)
        ALOGE("%s: exynos_v4l2_s_fmt fail (%d)",__FUNCTION__, ret);


    return ret;
}

int cam_int_reqbufs(node_info_t *node)
{
    struct v4l2_requestbuffers req;
    int ret;

    req.count = node->buffers;
    req.type = node->type;
    req.memory = node->memory;

    ret = exynos_v4l2_reqbufs(node->fd, &req);

    if (ret < 0)
        ALOGE("%s: VIDIOC_REQBUFS (fd:%d) failed (%d)",__FUNCTION__,node->fd, ret);

    return req.count;
}

int cam_int_qbuf(node_info_t *node, int index)
{
    struct v4l2_buffer v4l2_buf;
    struct v4l2_plane planes[VIDEO_MAX_PLANES];
    int i;
    int ret = 0;

    v4l2_buf.m.planes   = planes;
    v4l2_buf.type       = node->type;
    v4l2_buf.memory     = node->memory;
    v4l2_buf.index      = index;
    v4l2_buf.length     = node->planes;

    for(i = 0; i < node->planes; i++){
        v4l2_buf.m.planes[i].m.fd = (int)(node->buffer[index].fd.extFd[i]);
        v4l2_buf.m.planes[i].length  = (unsigned long)(node->buffer[index].size.extS[i]);
    }

    ret = exynos_v4l2_qbuf(node->fd, &v4l2_buf);

    if (ret < 0)
        ALOGE("%s: cam_int_qbuf failed (index:%d)(ret:%d)",__FUNCTION__, index, ret);

    return ret;
}

int cam_int_streamon(node_info_t *node)
{
    enum v4l2_buf_type type = node->type;
    int ret;


    ret = exynos_v4l2_streamon(node->fd, type);

    if (ret < 0)
        ALOGE("%s: VIDIOC_STREAMON failed [%d] (%d)",__FUNCTION__, node->fd,ret);

    ALOGV("On streaming I/O... ... fd(%d)", node->fd);

    return ret;
}

int cam_int_streamoff(node_info_t *node)
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    int ret;


    ALOGV("Off streaming I/O... fd(%d)", node->fd);
    ret = exynos_v4l2_streamoff(node->fd, type);

    if (ret < 0)
        ALOGE("%s: VIDIOC_STREAMOFF failed (%d)",__FUNCTION__, ret);

    return ret;
}

int isp_int_streamoff(node_info_t *node)
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    int ret;

    ALOGV("Off streaming I/O... fd(%d)", node->fd);
    ret = exynos_v4l2_streamoff(node->fd, type);

    if (ret < 0)
        ALOGE("%s: VIDIOC_STREAMOFF failed (%d)",__FUNCTION__, ret);

    return ret;
}

int cam_int_dqbuf(node_info_t *node)
{
    struct v4l2_buffer v4l2_buf;
    struct v4l2_plane planes[VIDEO_MAX_PLANES];
    int ret;

    v4l2_buf.type       = node->type;
    v4l2_buf.memory     = node->memory;
    v4l2_buf.m.planes   = planes;
    v4l2_buf.length     = node->planes;

    ret = exynos_v4l2_dqbuf(node->fd, &v4l2_buf);
    if (ret < 0)
        ALOGE("%s: VIDIOC_DQBUF failed (%d)",__FUNCTION__, ret);

    return v4l2_buf.index;
}

int cam_int_s_input(node_info_t *node, int index)
{
    int ret;

    ret = exynos_v4l2_s_input(node->fd, index);
    if (ret < 0)
        ALOGE("%s: VIDIOC_S_INPUT failed (%d)",__FUNCTION__, ret);

    return ret;
}


gralloc_module_t const* ExynosCameraHWInterface2::m_grallocHal;

RequestManager::RequestManager(SignalDrivenThread* main_thread):
    m_numOfEntries(0),
    m_entryInsertionIndex(-1),
    m_entryProcessingIndex(-1),
    m_entryFrameOutputIndex(-1),
    m_lastAeMode(0),
    m_lastAaMode(0),
    m_lastAwbMode(0),
    m_lastAeComp(0),
    m_frameIndex(-1)
{
    m_metadataConverter = new MetadataConverter;
    m_mainThread = main_thread;
    for (int i=0 ; i<NUM_MAX_REQUEST_MGR_ENTRY; i++) {
        memset(&(entries[i]), 0x00, sizeof(request_manager_entry_t));
        entries[i].internal_shot.shot.ctl.request.frameCount = -1;
    }
    m_sensorPipelineSkipCnt = 0;
    return;
}

RequestManager::~RequestManager()
{
    ALOGV("%s", __FUNCTION__);
    if (m_metadataConverter != NULL) {
        delete m_metadataConverter;
        m_metadataConverter = NULL;
    }

    return;
}

int RequestManager::GetNumEntries()
{
    return m_numOfEntries;
}

void RequestManager::SetDefaultParameters(int cropX)
{
    m_cropX = cropX;
}

bool RequestManager::IsRequestQueueFull()
{
    Mutex::Autolock lock(m_requestMutex);
    if (m_numOfEntries>=NUM_MAX_REQUEST_MGR_ENTRY)
        return true;
    else
        return false;
}

void RequestManager::RegisterRequest(camera_metadata_t * new_request)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);

    Mutex::Autolock lock(m_requestMutex);

    request_manager_entry * newEntry = NULL;
    int newInsertionIndex = GetNextIndex(m_entryInsertionIndex);
    ALOGV("DEBUG(%s): got lock, new insertIndex(%d), cnt before reg(%d)", __FUNCTION__,newInsertionIndex,m_numOfEntries );


    newEntry = &(entries[newInsertionIndex]);

    if (newEntry->status!=EMPTY) {
        ALOGV("DEBUG(%s): Circular buffer abnormal ", __FUNCTION__);
        return;
    }
    newEntry->status = REGISTERED;
    newEntry->original_request = new_request;
    memset(&(newEntry->internal_shot), 0, sizeof(struct camera2_shot_ext));
    m_metadataConverter->ToInternalShot(new_request, &(newEntry->internal_shot));
    newEntry->output_stream_count = newEntry->internal_shot.shot.ctl.request.outputStreams[15];

    m_numOfEntries++;
    m_entryInsertionIndex = newInsertionIndex;


    ALOGV("## RegisterReq DONE num(%d), insert(%d), processing(%d), frame(%d), (frameCnt(%d))",
    m_numOfEntries,m_entryInsertionIndex,m_entryProcessingIndex, m_entryFrameOutputIndex, newEntry->internal_shot.shot.ctl.request.frameCount);
}

void RequestManager::DeregisterRequest(camera_metadata_t ** deregistered_request)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    int frame_index;
    request_manager_entry * currentEntry;

    Mutex::Autolock lock(m_requestMutex);

    frame_index = GetFrameIndex();
    currentEntry =  &(entries[frame_index]);
    if (currentEntry->status != CAPTURED) {
        ALOGV("DBG(%s): Circular buffer abnormal. processing(%d), frame(%d), status(%d) ", __FUNCTION__
        , m_entryProcessingIndex, m_entryFrameOutputIndex,(int)(currentEntry->status));
        return;
    }
    if (deregistered_request)  *deregistered_request = currentEntry->original_request;

    currentEntry->status = EMPTY;
    currentEntry->original_request = NULL;
    memset(&(currentEntry->internal_shot), 0, sizeof(struct camera2_shot_ext));
    currentEntry->internal_shot.shot.ctl.request.frameCount = -1;
    currentEntry->output_stream_count = 0;
    currentEntry->dynamic_meta_vaild = false;
    m_numOfEntries--;
    ALOGV("## DeRegistReq DONE num(%d), insert(%d), processing(%d), frame(%d)",
     m_numOfEntries,m_entryInsertionIndex,m_entryProcessingIndex, m_entryFrameOutputIndex);

    return;
}

bool RequestManager::PrepareFrame(size_t* num_entries, size_t* frame_size,
                camera_metadata_t ** prepared_frame, int afState)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    Mutex::Autolock lock(m_requestMutex);
    status_t res = NO_ERROR;
    int tempFrameOutputIndex = GetFrameIndex();
    request_manager_entry * currentEntry =  &(entries[tempFrameOutputIndex]);
    ALOGV("DEBUG(%s): processing(%d), frameOut(%d), insert(%d) recentlycompleted(%d)", __FUNCTION__,
        m_entryProcessingIndex, m_entryFrameOutputIndex, m_entryInsertionIndex, m_completedIndex);

    if (currentEntry->status != CAPTURED) {
        ALOGV("DBG(%s): Circular buffer abnormal status(%d)", __FUNCTION__, (int)(currentEntry->status));

        return false;
    }
    m_entryFrameOutputIndex = tempFrameOutputIndex;
    m_tempFrameMetadata = place_camera_metadata(m_tempFrameMetadataBuf, 2000, 15, 500); //estimated
    add_camera_metadata_entry(m_tempFrameMetadata, ANDROID_CONTROL_AF_STATE, &afState, 1);
    res = m_metadataConverter->ToDynamicMetadata(&(currentEntry->internal_shot),
                m_tempFrameMetadata);
    if (res!=NO_ERROR) {
        ALOGE("ERROR(%s): ToDynamicMetadata (%d) ", __FUNCTION__, res);
        return false;
    }
    *num_entries = get_camera_metadata_entry_count(m_tempFrameMetadata);
    *frame_size = get_camera_metadata_size(m_tempFrameMetadata);
    *prepared_frame = m_tempFrameMetadata;
    ALOGV("## PrepareFrame DONE: frameOut(%d) frameCnt-req(%d)", m_entryFrameOutputIndex,
        currentEntry->internal_shot.shot.ctl.request.frameCount);
    // Dump();
    return true;
}

int RequestManager::MarkProcessingRequest(ExynosBuffer* buf, int *afMode)
{

    Mutex::Autolock lock(m_requestMutex);
    struct camera2_shot_ext * shot_ext;
    struct camera2_shot_ext * request_shot;
    int targetStreamIndex = 0;
    request_manager_entry * newEntry = NULL;
    static int count = 0;

    if (m_numOfEntries == 0)  {
        ALOGD("DEBUG(%s): Request Manager Empty ", __FUNCTION__);
        return -1;
    }

    if ((m_entryProcessingIndex == m_entryInsertionIndex)
        && (entries[m_entryProcessingIndex].status == REQUESTED || entries[m_entryProcessingIndex].status == CAPTURED)) {
        ALOGD("## MarkProcReq skipping(request underrun) -  num(%d), insert(%d), processing(%d), frame(%d)",
         m_numOfEntries,m_entryInsertionIndex,m_entryProcessingIndex, m_entryFrameOutputIndex);
        return -1;
    }

    int newProcessingIndex = GetNextIndex(m_entryProcessingIndex);
    ALOGV("DEBUG(%s): index(%d)", __FUNCTION__, newProcessingIndex);

    newEntry = &(entries[newProcessingIndex]);
    request_shot = &(newEntry->internal_shot);
    *afMode = (int)(newEntry->internal_shot.shot.ctl.aa.afMode);
    if (newEntry->status != REGISTERED) {
        ALOGD("DEBUG(%s)(%d): Circular buffer abnormal ", __FUNCTION__, newProcessingIndex);
        return -1;
    }

    newEntry->status = REQUESTED;

    shot_ext = (struct camera2_shot_ext *)buf->virt.extP[1];

    memset(shot_ext, 0x00, sizeof(struct camera2_shot_ext));
    shot_ext->shot.ctl.request.frameCount = request_shot->shot.ctl.request.frameCount;
    shot_ext->request_sensor = 1;
    shot_ext->dis_bypass = 1;
    shot_ext->dnr_bypass = 1;
    shot_ext->fd_bypass = 1;
    shot_ext->setfile = 0;

    for (int i = 0; i < newEntry->output_stream_count; i++) {
        targetStreamIndex = newEntry->internal_shot.shot.ctl.request.outputStreams[i];

        if (targetStreamIndex==0) {
            ALOGV("DEBUG(%s): outputstreams(%d) is for scalerP", __FUNCTION__, i);
            shot_ext->request_scp = 1;
        }
        else if (targetStreamIndex == 1) {
            ALOGV("DEBUG(%s): outputstreams(%d) is for scalerC", __FUNCTION__, i);
            shot_ext->request_scc = 1;
        }
        else if (targetStreamIndex == 2) {
            ALOGV("DEBUG(%s): outputstreams(%d) is for scalerP (record)", __FUNCTION__, i);
            shot_ext->request_scp = 1;
            shot_ext->shot.ctl.request.outputStreams[2] = 1;
        }
        else {
            ALOGV("DEBUG(%s): outputstreams(%d) has abnormal value(%d)", __FUNCTION__, i, targetStreamIndex);
        }
    }

    if (count == 0){
        shot_ext->shot.ctl.aa.mode = AA_CONTROL_AUTO;
    } else
        shot_ext->shot.ctl.aa.mode = AA_CONTROL_NONE;

    count++;
    shot_ext->shot.ctl.request.metadataMode = METADATA_MODE_FULL;
    shot_ext->shot.ctl.stats.faceDetectMode = FACEDETECT_MODE_FULL;
    shot_ext->shot.magicNumber = 0x23456789;
    shot_ext->shot.ctl.sensor.exposureTime = 0;
    shot_ext->shot.ctl.sensor.frameDuration = 33*1000*1000;
    shot_ext->shot.ctl.sensor.sensitivity = 0;

    shot_ext->shot.ctl.scaler.cropRegion[0] = 0;
    shot_ext->shot.ctl.scaler.cropRegion[1] = 0;
    shot_ext->shot.ctl.scaler.cropRegion[2] = m_cropX;

    m_entryProcessingIndex = newProcessingIndex;
    return newProcessingIndex;
}

void RequestManager::NotifyStreamOutput(int frameCnt, int stream_id)
{
    int index;

    ALOGV("DEBUG(%s): frameCnt(%d), stream_id(%d)", __FUNCTION__, frameCnt, stream_id);

    index = FindEntryIndexByFrameCnt(frameCnt);
    if (index == -1) {
        ALOGE("ERR(%s): Cannot find entry for frameCnt(%d)", __FUNCTION__, frameCnt);
        return;
    }
    ALOGV("DEBUG(%s): frameCnt(%d), stream_id(%d) last cnt (%d)", __FUNCTION__, frameCnt, stream_id,  entries[index].output_stream_count);

    entries[index].output_stream_count--;  //TODO : match stream id also
    CheckCompleted(index);
    return;
}

void RequestManager::CheckCompleted(int index)
{
    ALOGV("DEBUG(%s): reqIndex(%d) current Count(%d)", __FUNCTION__, index, entries[index].output_stream_count);
    SetFrameIndex(index);
    m_mainThread->SetSignal(SIGNAL_MAIN_STREAM_OUTPUT_DONE);
    return;
}

void RequestManager::SetFrameIndex(int index)
{
    Mutex::Autolock lock(m_requestMutex);
    m_frameIndex = index;
}

int RequestManager::GetFrameIndex()
{
    return m_frameIndex;
}

void RequestManager::ApplyDynamicMetadata(struct camera2_shot_ext *shot_ext)
{
    int index;
    struct camera2_shot_ext * request_shot;
    nsecs_t timeStamp;
    int i;

    ALOGV("DEBUG(%s): frameCnt(%d)", __FUNCTION__, shot_ext->shot.ctl.request.frameCount);

    for (i = 0 ; i < NUM_MAX_REQUEST_MGR_ENTRY ; i++) {
        if((entries[i].internal_shot.shot.ctl.request.frameCount == shot_ext->shot.ctl.request.frameCount)
            && (entries[i].status == CAPTURED))
            break;
    }

    if (i == NUM_MAX_REQUEST_MGR_ENTRY){
        ALOGE("[%s] no entry found(framecount:%d)", __FUNCTION__, shot_ext->shot.ctl.request.frameCount);
        return;
    }

    request_manager_entry * newEntry = &(entries[i]);
    request_shot = &(newEntry->internal_shot);

    newEntry->dynamic_meta_vaild = true;
    timeStamp = request_shot->shot.dm.sensor.timeStamp;
    memcpy(&(request_shot->shot.dm), &(shot_ext->shot.dm), sizeof(struct camera2_dm));
    request_shot->shot.dm.sensor.timeStamp = timeStamp;
    CheckCompleted(i);
}

void RequestManager::DumpInfoWithIndex(int index)
{
    struct camera2_shot_ext * currMetadata = &(entries[index].internal_shot);

    ALOGV("####   frameCount(%d) exposureTime(%lld) ISO(%d)",
        currMetadata->shot.ctl.request.frameCount,
        currMetadata->shot.ctl.sensor.exposureTime,
        currMetadata->shot.ctl.sensor.sensitivity);
    if (currMetadata->shot.ctl.request.outputStreams[15] == 0)
        ALOGV("####   No output stream selected");
    else if (currMetadata->shot.ctl.request.outputStreams[15] == 1)
        ALOGV("####   OutputStreamId : %d", currMetadata->shot.ctl.request.outputStreams[0]);
    else if (currMetadata->shot.ctl.request.outputStreams[15] == 2)
        ALOGV("####   OutputStreamId : %d, %d", currMetadata->shot.ctl.request.outputStreams[0],
            currMetadata->shot.ctl.request.outputStreams[1]);
    else
        ALOGV("####   OutputStream num (%d) abnormal ", currMetadata->shot.ctl.request.outputStreams[15]);
}

void    RequestManager::UpdateIspParameters(struct camera2_shot_ext *shot_ext, int frameCnt, bool afTrigger)
{
    int index, targetStreamIndex;
    struct camera2_shot_ext * request_shot;

    ALOGV("DEBUG(%s): updating info with frameCnt(%d)", __FUNCTION__, frameCnt);
    if (frameCnt < 0)
        return;

    index = FindEntryIndexByFrameCnt(frameCnt);
    if (index == -1) {
        ALOGE("ERR(%s): Cannot find entry for frameCnt(%d)", __FUNCTION__, frameCnt);
        return;
    }

    request_manager_entry * newEntry = &(entries[index]);
    request_shot = &(newEntry->internal_shot);
    memcpy(&(shot_ext->shot.ctl), &(request_shot->shot.ctl), sizeof(struct camera2_ctl));
    shot_ext->request_sensor = 1;
    shot_ext->dis_bypass = 1;
    shot_ext->dnr_bypass = 1;
    shot_ext->fd_bypass = 1;
    shot_ext->setfile = 0;

    shot_ext->request_scc = 0;
    shot_ext->request_scp = 0;

    shot_ext->shot.ctl.request.outputStreams[0] = 0;
    shot_ext->shot.ctl.request.outputStreams[1] = 0;
    shot_ext->shot.ctl.request.outputStreams[2] = 0;

    if (m_lastAaMode == request_shot->shot.ctl.aa.mode) {
        shot_ext->shot.ctl.aa.mode = (enum aa_mode)(0);
    }
    else {
        shot_ext->shot.ctl.aa.mode = request_shot->shot.ctl.aa.mode;
        m_lastAaMode = (int)(shot_ext->shot.ctl.aa.mode);
    }
    if (m_lastAeMode == request_shot->shot.ctl.aa.aeMode) {
        shot_ext->shot.ctl.aa.aeMode = (enum aa_aemode)(0);
    }
    else {
        shot_ext->shot.ctl.aa.aeMode = request_shot->shot.ctl.aa.aeMode;
        m_lastAeMode = (int)(shot_ext->shot.ctl.aa.aeMode);
    }
    if (m_lastAwbMode == request_shot->shot.ctl.aa.awbMode) {
        shot_ext->shot.ctl.aa.awbMode = (enum aa_awbmode)(0);
    }
    else {
        shot_ext->shot.ctl.aa.awbMode = request_shot->shot.ctl.aa.awbMode;
        m_lastAwbMode = (int)(shot_ext->shot.ctl.aa.awbMode);
    }
    if (m_lastAeComp == request_shot->shot.ctl.aa.aeExpCompensation) {
        shot_ext->shot.ctl.aa.aeExpCompensation = 0;
    }
    else {
        shot_ext->shot.ctl.aa.aeExpCompensation = request_shot->shot.ctl.aa.aeExpCompensation;
        m_lastAeComp = (int)(shot_ext->shot.ctl.aa.aeExpCompensation);
    }
    if (afTrigger) {
        ALOGE("### AF Trigger ");
        shot_ext->shot.ctl.aa.afTrigger = 1;
        shot_ext->shot.ctl.aa.afRegions[0] = 0;
        shot_ext->shot.ctl.aa.afRegions[1] = 0;
        shot_ext->shot.ctl.aa.afRegions[2] = 0;
        shot_ext->shot.ctl.aa.afRegions[3] = 0;
        shot_ext->shot.ctl.aa.afRegions[4] = 0;
    }
    else
        shot_ext->shot.ctl.aa.afTrigger = 0;
    for (int i = 0; i < newEntry->output_stream_count; i++) {
       targetStreamIndex = newEntry->internal_shot.shot.ctl.request.outputStreams[i];

        if (targetStreamIndex==0) {
            ALOGV("DEBUG(%s): outputstreams(%d) is for scalerP", __FUNCTION__, i);
            shot_ext->request_scp = 1;
        }
        else if (targetStreamIndex == 1) {
            ALOGV("DEBUG(%s): outputstreams(%d) is for scalerC", __FUNCTION__, i);
            shot_ext->request_scc = 1;
        }
        else if (targetStreamIndex == 2) {
            ALOGV("DEBUG(%s): outputstreams(%d) is for scalerP (record)", __FUNCTION__, i);
            shot_ext->request_scp = 1;
            shot_ext->shot.ctl.request.outputStreams[2] = 1;
            shot_ext->shot.ctl.aa.aeTargetFpsRange[0] = 30;
            shot_ext->shot.ctl.aa.aeTargetFpsRange[1] = 30;
        }
        else {
            ALOGV("DEBUG(%s): outputstreams(%d) has abnormal value(%d)", __FUNCTION__, i, targetStreamIndex);
        }
    }
        ALOGV("(%s): applied aa(%d) aemode(%d) expComp(%d), awb(%d) afmode(%d), ", __FUNCTION__,
        (int)(shot_ext->shot.ctl.aa.mode), (int)(shot_ext->shot.ctl.aa.aeMode),
        (int)(shot_ext->shot.ctl.aa.aeExpCompensation), (int)(shot_ext->shot.ctl.aa.awbMode),
        (int)(shot_ext->shot.ctl.aa.afMode));
}

int     RequestManager::FindEntryIndexByFrameCnt(int frameCnt)
{
    for (int i = 0 ; i < NUM_MAX_REQUEST_MGR_ENTRY ; i++) {
        if (entries[i].internal_shot.shot.ctl.request.frameCount == frameCnt)
            return i;
    }
    return -1;
}

void    RequestManager::RegisterTimestamp(int frameCnt, nsecs_t * frameTime)
{
    int index = FindEntryIndexByFrameCnt(frameCnt);
    if (index == -1) {
        ALOGE("ERR(%s): Cannot find entry for frameCnt(%d)", __FUNCTION__, frameCnt);
        return;
    }

    request_manager_entry * currentEntry = &(entries[index]);
    currentEntry->internal_shot.shot.dm.sensor.timeStamp = *((uint64_t*)frameTime);
    ALOGV("DEBUG(%s): applied timestamp for reqIndex(%d) frameCnt(%d) (%lld)", __FUNCTION__,
        index, frameCnt, currentEntry->internal_shot.shot.dm.sensor.timeStamp);
}

uint64_t  RequestManager::GetTimestamp(int index)
{
    if (index < 0 || index >= NUM_MAX_REQUEST_MGR_ENTRY) {
        ALOGE("ERR(%s): Request entry outside of bounds (%d)", __FUNCTION__, index);
        return 0;
    }

    request_manager_entry * currentEntry = &(entries[index]);
    uint64_t frameTime = currentEntry->internal_shot.shot.dm.sensor.timeStamp;
    ALOGV("DEBUG(%s): Returning timestamp for reqIndex(%d) (%lld)", __FUNCTION__, index, frameTime);
    return frameTime;
}

int     RequestManager::FindFrameCnt(struct camera2_shot_ext * shot_ext)
{
    int i;

    if (m_numOfEntries == 0) {
        ALOGV("(%s): No Entry found", __FUNCTION__);
        return -1;
    }

    for (i = 0 ; i < NUM_MAX_REQUEST_MGR_ENTRY ; i++) {
        if(entries[i].internal_shot.shot.ctl.request.frameCount != shot_ext->shot.ctl.request.frameCount)
            continue;

        if (entries[i].status == REQUESTED) {
            entries[i].status = CAPTURED;
            return entries[i].internal_shot.shot.ctl.request.frameCount;
        }

    }

    ALOGD("(%s): No Entry found", __FUNCTION__);

    return -1;
}

void     RequestManager::SetInitialSkip(int count)
{
    ALOGV("(%s): Pipeline Restarting. setting cnt(%d) - current(%d)", __FUNCTION__, count, m_sensorPipelineSkipCnt);
    if (count > m_sensorPipelineSkipCnt)
        m_sensorPipelineSkipCnt = count;
}

int     RequestManager::GetSkipCnt()
{
    ALOGV("(%s): skip cnt(%d)", __FUNCTION__, m_sensorPipelineSkipCnt);
    if (m_sensorPipelineSkipCnt == 0)
        return m_sensorPipelineSkipCnt;
    else
        return --m_sensorPipelineSkipCnt;
}

void RequestManager::Dump(void)
{
    int i = 0;
    request_manager_entry * currentEntry;
    ALOGD("## Dump  totalentry(%d), insert(%d), processing(%d), frame(%d)",
    m_numOfEntries,m_entryInsertionIndex,m_entryProcessingIndex, m_entryFrameOutputIndex);

    for (i = 0 ; i < NUM_MAX_REQUEST_MGR_ENTRY ; i++) {
        currentEntry =  &(entries[i]);
        ALOGD("[%2d] status[%d] frameCnt[%3d] numOutput[%d] outstream[0]-%d outstream[1]-%d", i,
        currentEntry->status, currentEntry->internal_shot.shot.ctl.request.frameCount,
            currentEntry->output_stream_count,
            currentEntry->internal_shot.shot.ctl.request.outputStreams[0],
            currentEntry->internal_shot.shot.ctl.request.outputStreams[1]);
    }
}

int     RequestManager::GetNextIndex(int index)
{
    index++;
    if (index >= NUM_MAX_REQUEST_MGR_ENTRY)
        index = 0;

    return index;
}

ExynosCameraHWInterface2::ExynosCameraHWInterface2(int cameraId, camera2_device_t *dev, ExynosCamera2 * camera):
            m_requestQueueOps(NULL),
            m_frameQueueOps(NULL),
            m_callbackCookie(NULL),
            m_numOfRemainingReqInSvc(0),
            m_isRequestQueuePending(false),
            m_isRequestQueueNull(true),
            m_isSensorThreadOn(false),
            m_isSensorStarted(false),
            m_isIspStarted(false),
            m_ionCameraClient(0),
            m_initFlag1(false),
            m_initFlag2(false),
            m_scp_flushing(false),
            m_closing(false),
            m_recordingEnabled(false),
            m_needsRecordBufferInit(false),
            lastFrameCnt(-1),
            m_scp_closing(false),
            m_scp_closed(false),
            m_afState(HAL_AFSTATE_INACTIVE),
            m_afMode(NO_CHANGE),
            m_afMode2(NO_CHANGE),
            m_IsAfModeUpdateRequired(false),
            m_IsAfTriggerRequired(false),
            m_IsAfLockRequired(false),
            m_wideAspect(false),
            m_afTriggerId(0),
            m_halDevice(dev),
            m_need_streamoff(0),
            m_cameraId(cameraId)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    int ret = 0;

    m_exynosPictureCSC = NULL;
    m_exynosVideoCSC = NULL;

    if (!m_grallocHal) {
        ret = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, (const hw_module_t **)&m_grallocHal);
        if (ret)
            ALOGE("ERR(%s):Fail on loading gralloc HAL", __FUNCTION__);
    }

    m_camera2 = camera;
    m_ionCameraClient = createIonClient(m_ionCameraClient);
    if(m_ionCameraClient == 0)
        ALOGE("ERR(%s):Fail on ion_client_create", __FUNCTION__);


    m_BayerManager = new BayerBufManager();
    m_mainThread    = new MainThread(this);
    InitializeISPChain();
    m_sensorThread  = new SensorThread(this);
    m_mainThread->Start("MainThread", PRIORITY_DEFAULT, 0);
    ALOGV("DEBUG(%s): created sensorthread ################", __FUNCTION__);

    m_requestManager = new RequestManager((SignalDrivenThread*)(m_mainThread.get()));
    CSC_METHOD cscMethod = CSC_METHOD_HW;
    m_exynosPictureCSC = csc_init(cscMethod);
    if (m_exynosPictureCSC == NULL)
        ALOGE("ERR(%s): csc_init() fail", __FUNCTION__);
    csc_set_hw_property(m_exynosPictureCSC, CSC_HW_PROPERTY_FIXED_NODE, PICTURE_GSC_NODE_NUM);

    m_exynosVideoCSC = csc_init(cscMethod);
    if (m_exynosVideoCSC == NULL)
        ALOGE("ERR(%s): csc_init() fail", __FUNCTION__);
    csc_set_hw_property(m_exynosVideoCSC, CSC_HW_PROPERTY_FIXED_NODE, VIDEO_GSC_NODE_NUM);


    ALOGV("DEBUG(%s): END", __FUNCTION__);
    m_setExifFixedAttribute();
}

ExynosCameraHWInterface2::~ExynosCameraHWInterface2()
{
    ALOGV("%s: ENTER", __FUNCTION__);
    this->release();
    ALOGV("%s: EXIT", __FUNCTION__);
}

void ExynosCameraHWInterface2::release()
{
    int i, res;
    ALOGD("%s: ENTER", __func__);
    m_closing = true;

    if (m_streamThreads[1] != NULL) {
        m_streamThreads[1]->release();
        m_streamThreads[1]->SetSignal(SIGNAL_THREAD_TERMINATE);
    }

    if (m_streamThreads[0] != NULL) {
        m_streamThreads[0]->release();
        m_streamThreads[0]->SetSignal(SIGNAL_THREAD_TERMINATE);
    }

    if (m_ispThread != NULL) {
        m_ispThread->release();
    }

    if (m_sensorThread != NULL) {
        m_sensorThread->release();
    }

    if (m_mainThread != NULL) {
        m_mainThread->release();
    }

    if (m_exynosPictureCSC)
        csc_deinit(m_exynosPictureCSC);
    m_exynosPictureCSC = NULL;

    if (m_exynosVideoCSC)
        csc_deinit(m_exynosVideoCSC);
    m_exynosVideoCSC = NULL;


    if (m_streamThreads[1] != NULL) {
        while (!m_streamThreads[1]->IsTerminated())
        {
            ALOGD("Waiting for ISP thread is tetminated");
            usleep(100000);
        }
        m_streamThreads[1] = NULL;
    }

    if (m_streamThreads[0] != NULL) {
        while (!m_streamThreads[0]->IsTerminated())
        {
            ALOGD("Waiting for sensor thread is tetminated");
            usleep(100000);
        }
        m_streamThreads[0] = NULL;
    }

    if (m_ispThread != NULL) {
        while (!m_ispThread->IsTerminated())
        {
            ALOGD("Waiting for isp thread is tetminated");
            usleep(100000);
        }
        m_ispThread = NULL;
    }

    if (m_sensorThread != NULL) {
        while (!m_sensorThread->IsTerminated())
        {
            ALOGD("Waiting for sensor thread is tetminated");
            usleep(100000);
        }
        m_sensorThread = NULL;
    }

    if (m_mainThread != NULL) {
        while (!m_mainThread->IsTerminated())
        {
            ALOGD("Waiting for main thread is tetminated");
            usleep(100000);
        }
        m_mainThread = NULL;
    }

    if (m_requestManager != NULL) {
        delete m_requestManager;
        m_requestManager = NULL;
    }

    if (m_BayerManager != NULL) {
        delete m_BayerManager;
        m_BayerManager = NULL;
    }
//    for(i = 0; i < m_camera_info.sensor.buffers; i++)
    for (i = 0; i < NUM_BAYER_BUFFERS; i++)
        freeCameraMemory(&m_camera_info.sensor.buffer[i], m_camera_info.sensor.planes);

    for(i = 0; i < m_camera_info.capture.buffers; i++)
        freeCameraMemory(&m_camera_info.capture.buffer[i], m_camera_info.capture.planes);

    ALOGV("DEBUG(%s): calling exynos_v4l2_close - sensor", __FUNCTION__);
    res = exynos_v4l2_close(m_camera_info.sensor.fd);
    if (res != NO_ERROR ) {
        ALOGE("ERR(%s): exynos_v4l2_close failed(%d)",__FUNCTION__ , res);
    }

    ALOGV("DEBUG(%s): calling exynos_v4l2_close - isp", __FUNCTION__);
    res = exynos_v4l2_close(m_camera_info.isp.fd);
    if (res != NO_ERROR ) {
        ALOGE("ERR(%s): exynos_v4l2_close failed(%d)",__FUNCTION__ , res);
    }

    ALOGV("DEBUG(%s): calling exynos_v4l2_close - capture", __FUNCTION__);
    res = exynos_v4l2_close(m_camera_info.capture.fd);
    if (res != NO_ERROR ) {
        ALOGE("ERR(%s): exynos_v4l2_close failed(%d)",__FUNCTION__ , res);
    }

    ALOGV("DEBUG(%s): calling exynos_v4l2_close - scp", __FUNCTION__);
    res = exynos_v4l2_close(m_fd_scp);
    if (res != NO_ERROR ) {
        ALOGE("ERR(%s): exynos_v4l2_close failed(%d)",__FUNCTION__ , res);
    }
    ALOGV("DEBUG(%s): calling deleteIonClient", __FUNCTION__);
    deleteIonClient(m_ionCameraClient);

    ALOGV("%s: EXIT", __func__);
}

void ExynosCameraHWInterface2::InitializeISPChain()
{
    char node_name[30];
    int fd = 0;
    int i;

    /* Open Sensor */
    memset(&node_name, 0x00, sizeof(char[30]));
    sprintf(node_name, "%s%d", NODE_PREFIX, 40);
    fd = exynos_v4l2_open(node_name, O_RDWR, 0);

    if (fd < 0) {
        ALOGE("ERR(%s): failed to open sensor video node (%s) fd (%d)", __FUNCTION__,node_name, fd);
    }
    else {
        ALOGV("DEBUG(%s): sensor video node opened(%s) fd (%d)", __FUNCTION__,node_name, fd);
    }
    m_camera_info.sensor.fd = fd;

    /* Open ISP */
    memset(&node_name, 0x00, sizeof(char[30]));
    sprintf(node_name, "%s%d", NODE_PREFIX, 41);
    fd = exynos_v4l2_open(node_name, O_RDWR, 0);

    if (fd < 0) {
        ALOGE("ERR(%s): failed to open isp video node (%s) fd (%d)", __FUNCTION__,node_name, fd);
    }
    else {
        ALOGV("DEBUG(%s): isp video node opened(%s) fd (%d)", __FUNCTION__,node_name, fd);
    }
    m_camera_info.isp.fd = fd;

    /* Open ScalerC */
    memset(&node_name, 0x00, sizeof(char[30]));
    sprintf(node_name, "%s%d", NODE_PREFIX, 42);
    fd = exynos_v4l2_open(node_name, O_RDWR, 0);

    if (fd < 0) {
        ALOGE("ERR(%s): failed to open capture video node (%s) fd (%d)", __FUNCTION__,node_name, fd);
    }
    else {
        ALOGV("DEBUG(%s): capture video node opened(%s) fd (%d)", __FUNCTION__,node_name, fd);
    }
    m_camera_info.capture.fd = fd;

    /* Open ScalerP */
    memset(&node_name, 0x00, sizeof(char[30]));
    sprintf(node_name, "%s%d", NODE_PREFIX, 44);
    fd = exynos_v4l2_open(node_name, O_RDWR, 0);
    if (fd < 0) {
        ALOGE("DEBUG(%s): failed to open preview video node (%s) fd (%d)", __FUNCTION__,node_name, fd);
    }
    else {
        ALOGV("DEBUG(%s): preview video node opened(%s) fd (%d)", __FUNCTION__,node_name, fd);
    }
    m_fd_scp = fd;

    if(m_cameraId == 0)
        m_camera_info.sensor_id = SENSOR_NAME_S5K4E5;
    else
        m_camera_info.sensor_id = SENSOR_NAME_S5K6A3;

    memset(&m_camera_info.dummy_shot, 0x00, sizeof(struct camera2_shot_ext));
    m_camera_info.dummy_shot.shot.ctl.request.metadataMode = METADATA_MODE_FULL;
    m_camera_info.dummy_shot.shot.magicNumber = 0x23456789;

    m_camera_info.dummy_shot.dis_bypass = 1;
    m_camera_info.dummy_shot.dnr_bypass = 1;
    m_camera_info.dummy_shot.fd_bypass = 1;

    /*sensor setting*/
    m_camera_info.dummy_shot.shot.ctl.sensor.exposureTime = 0;
    m_camera_info.dummy_shot.shot.ctl.sensor.frameDuration = 0;
    m_camera_info.dummy_shot.shot.ctl.sensor.sensitivity = 0;

    m_camera_info.dummy_shot.shot.ctl.scaler.cropRegion[0] = 0;
    m_camera_info.dummy_shot.shot.ctl.scaler.cropRegion[1] = 0;

    /*request setting*/
    m_camera_info.dummy_shot.request_sensor = 1;
    m_camera_info.dummy_shot.request_scc = 0;
    m_camera_info.dummy_shot.request_scp = 0;
    m_camera_info.dummy_shot.shot.ctl.request.outputStreams[0] = 0;
    m_camera_info.dummy_shot.shot.ctl.request.outputStreams[1] = 0;
    m_camera_info.dummy_shot.shot.ctl.request.outputStreams[2] = 0;

    m_camera_info.sensor.width = m_camera2->getSensorRawW();
    m_camera_info.sensor.height = m_camera2->getSensorRawH();

    m_camera_info.sensor.format = V4L2_PIX_FMT_SBGGR16;
    m_camera_info.sensor.planes = 2;
    m_camera_info.sensor.buffers = NUM_BAYER_BUFFERS;
    m_camera_info.sensor.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    m_camera_info.sensor.memory = V4L2_MEMORY_DMABUF;
    m_camera_info.sensor.ionClient = m_ionCameraClient;

    for(i = 0; i < m_camera_info.sensor.buffers; i++){
        initCameraMemory(&m_camera_info.sensor.buffer[i], m_camera_info.sensor.planes);
        m_camera_info.sensor.buffer[i].size.extS[0] = m_camera_info.sensor.width*m_camera_info.sensor.height*2;
        m_camera_info.sensor.buffer[i].size.extS[1] = 8*1024; // HACK, driver use 8*1024, should be use predefined value
        allocCameraMemory(m_camera_info.sensor.ionClient, &m_camera_info.sensor.buffer[i], m_camera_info.sensor.planes);
    }

    m_camera_info.isp.width = m_camera_info.sensor.width;
    m_camera_info.isp.height = m_camera_info.sensor.height;
    m_camera_info.isp.format = m_camera_info.sensor.format;
    m_camera_info.isp.planes = m_camera_info.sensor.planes;
    m_camera_info.isp.buffers = m_camera_info.sensor.buffers;
    m_camera_info.isp.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    m_camera_info.isp.memory = V4L2_MEMORY_DMABUF;
    m_camera_info.isp.ionClient = m_ionCameraClient;

    for(i = 0; i < m_camera_info.isp.buffers; i++){
        initCameraMemory(&m_camera_info.isp.buffer[i], m_camera_info.isp.planes);
        m_camera_info.isp.buffer[i].size.extS[0]    = m_camera_info.sensor.buffer[i].size.extS[0];
        m_camera_info.isp.buffer[i].size.extS[1]    = m_camera_info.sensor.buffer[i].size.extS[1];
        m_camera_info.isp.buffer[i].fd.extFd[0]     = m_camera_info.sensor.buffer[i].fd.extFd[0];
        m_camera_info.isp.buffer[i].fd.extFd[1]     = m_camera_info.sensor.buffer[i].fd.extFd[1];
        m_camera_info.isp.buffer[i].virt.extP[0]    = m_camera_info.sensor.buffer[i].virt.extP[0];
        m_camera_info.isp.buffer[i].virt.extP[1]    = m_camera_info.sensor.buffer[i].virt.extP[1];
    };

    /* init ISP */
    cam_int_s_input(&(m_camera_info.isp), m_camera_info.sensor_id);
    cam_int_s_fmt(&(m_camera_info.isp));
    ALOGV("DEBUG(%s): isp calling reqbuf", __FUNCTION__);
    cam_int_reqbufs(&(m_camera_info.isp));
    ALOGV("DEBUG(%s): isp calling querybuf", __FUNCTION__);
    ALOGV("DEBUG(%s): isp mem alloc done",  __FUNCTION__);

    /* init Sensor */
    cam_int_s_input(&(m_camera_info.sensor), m_camera_info.sensor_id);
    ALOGV("DEBUG(%s): sensor s_input done",  __FUNCTION__);
    if (cam_int_s_fmt(&(m_camera_info.sensor))< 0) {
        ALOGE("ERR(%s): sensor s_fmt fail",  __FUNCTION__);
    }
    ALOGV("DEBUG(%s): sensor s_fmt done",  __FUNCTION__);
    cam_int_reqbufs(&(m_camera_info.sensor));
    ALOGV("DEBUG(%s): sensor reqbuf done",  __FUNCTION__);
    for (i = 0; i < m_camera_info.sensor.buffers; i++) {
        ALOGV("DEBUG(%s): sensor initial QBUF [%d]",  __FUNCTION__, i);
        memcpy( m_camera_info.sensor.buffer[i].virt.extP[1], &(m_camera_info.dummy_shot),
                sizeof(struct camera2_shot_ext));
        m_camera_info.dummy_shot.shot.ctl.sensor.frameDuration = 33*1000*1000; // apply from frame #1
        m_camera_info.dummy_shot.shot.ctl.request.frameCount = -1;
        cam_int_qbuf(&(m_camera_info.sensor), i);
    }
    ALOGV("== stream_on :: .sensor");
    cam_int_streamon(&(m_camera_info.sensor));

    /* init Capture */
    m_camera_info.capture.width = m_camera2->getSensorW();
    m_camera_info.capture.height = m_camera2->getSensorH();
    m_camera_info.capture.format = V4L2_PIX_FMT_YUYV;
    m_camera_info.capture.planes = 1;
    m_camera_info.capture.buffers = 8;
    m_camera_info.capture.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    m_camera_info.capture.memory = V4L2_MEMORY_DMABUF;
    m_camera_info.capture.ionClient = m_ionCameraClient;

    for(i = 0; i < m_camera_info.capture.buffers; i++){
        initCameraMemory(&m_camera_info.capture.buffer[i], m_camera_info.capture.planes);
        m_camera_info.capture.buffer[i].size.extS[0] = m_camera_info.capture.width*m_camera_info.capture.height*2;
        allocCameraMemory(m_camera_info.capture.ionClient, &m_camera_info.capture.buffer[i], m_camera_info.capture.planes);
    }

    cam_int_s_input(&(m_camera_info.capture), m_camera_info.sensor_id);
    cam_int_s_fmt(&(m_camera_info.capture));
    ALOGV("DEBUG(%s): capture calling reqbuf", __FUNCTION__);
    cam_int_reqbufs(&(m_camera_info.capture));
    ALOGV("DEBUG(%s): capture calling querybuf", __FUNCTION__);

    for (i = 0; i < m_camera_info.capture.buffers; i++) {
        ALOGV("DEBUG(%s): capture initial QBUF [%d]",  __FUNCTION__, i);
        cam_int_qbuf(&(m_camera_info.capture), i);
    }

    ALOGV("== stream_on :: capture");
    if (cam_int_streamon(&(m_camera_info.capture)) < 0) {
        ALOGE("ERR(%s): capture stream on fail", __FUNCTION__);
    } else {
        m_camera_info.capture.status = true;
    }
}

void ExynosCameraHWInterface2::StartISP()
{
    int i;

    for (i = 0; i < m_camera_info.isp.buffers; i++) {
        ALOGV("DEBUG(%s): isp initial QBUF [%d]",  __FUNCTION__, i);
        cam_int_qbuf(&(m_camera_info.isp), i);
    }

    ALOGV("== stream_on :: isp");
    cam_int_streamon(&(m_camera_info.isp));

    for (i = 0; i < m_camera_info.isp.buffers; i++) {
        ALOGV("DEBUG(%s): isp initial DQBUF [%d]",  __FUNCTION__, i);
        cam_int_dqbuf(&(m_camera_info.isp));
    }
    exynos_v4l2_s_ctrl(m_camera_info.sensor.fd, V4L2_CID_IS_S_STREAM, IS_ENABLE_STREAM);
}

int ExynosCameraHWInterface2::getCameraId() const
{
    return m_cameraId;
}

int ExynosCameraHWInterface2::setRequestQueueSrcOps(const camera2_request_queue_src_ops_t *request_src_ops)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    if ((NULL != request_src_ops) && (NULL != request_src_ops->dequeue_request)
            && (NULL != request_src_ops->free_request) && (NULL != request_src_ops->request_count)) {
        m_requestQueueOps = (camera2_request_queue_src_ops_t*)request_src_ops;
        return 0;
    }
    else {
        ALOGE("DEBUG(%s):setRequestQueueSrcOps : NULL arguments", __FUNCTION__);
        return 1;
    }
}

int ExynosCameraHWInterface2::notifyRequestQueueNotEmpty()
{
    ALOGV("DEBUG(%s):setting [SIGNAL_MAIN_REQ_Q_NOT_EMPTY] current(%d)", __FUNCTION__, m_requestManager->GetNumEntries());
    if ((NULL==m_frameQueueOps)|| (NULL==m_requestQueueOps)) {
        ALOGE("DEBUG(%s):queue ops NULL. ignoring request", __FUNCTION__);
        return 0;
    }
    m_isRequestQueueNull = false;
    if (m_requestManager->GetNumEntries() == 0)
        m_requestManager->SetInitialSkip(5);
    m_mainThread->SetSignal(SIGNAL_MAIN_REQ_Q_NOT_EMPTY);
    return 0;
}

int ExynosCameraHWInterface2::setFrameQueueDstOps(const camera2_frame_queue_dst_ops_t *frame_dst_ops)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    if ((NULL != frame_dst_ops) && (NULL != frame_dst_ops->dequeue_frame)
            && (NULL != frame_dst_ops->cancel_frame) && (NULL !=frame_dst_ops->enqueue_frame)) {
        m_frameQueueOps = (camera2_frame_queue_dst_ops_t *)frame_dst_ops;
        return 0;
    }
    else {
        ALOGE("DEBUG(%s):setFrameQueueDstOps : NULL arguments", __FUNCTION__);
        return 1;
    }
}

int ExynosCameraHWInterface2::getInProgressCount()
{
    int inProgressCount = m_requestManager->GetNumEntries();
    ALOGV("DEBUG(%s): # of dequeued req (%d)", __FUNCTION__, inProgressCount);
    return inProgressCount;
}

int ExynosCameraHWInterface2::flushCapturesInProgress()
{
    return 0;
}

int ExynosCameraHWInterface2::constructDefaultRequest(int request_template, camera_metadata_t **request)
{
    ALOGV("DEBUG(%s): making template (%d) ", __FUNCTION__, request_template);

    if (request == NULL) return BAD_VALUE;
    if (request_template < 0 || request_template >= CAMERA2_TEMPLATE_COUNT) {
        return BAD_VALUE;
    }
    status_t res;
    // Pass 1, calculate size and allocate
    res = m_camera2->constructDefaultRequest(request_template,
            request,
            true);
    if (res != OK) {
        return res;
    }
    // Pass 2, build request
    res = m_camera2->constructDefaultRequest(request_template,
            request,
            false);
    if (res != OK) {
        ALOGE("Unable to populate new request for template %d",
                request_template);
    }

    return res;
}

int ExynosCameraHWInterface2::allocateStream(uint32_t width, uint32_t height, int format, const camera2_stream_ops_t *stream_ops,
                                    uint32_t *stream_id, uint32_t *format_actual, uint32_t *usage, uint32_t *max_buffers)
{
    ALOGV("DEBUG(%s): allocate stream width(%d) height(%d) format(%x)", __FUNCTION__,  width, height, format);
    char node_name[30];
    int fd = 0, allocCase = 0;
    StreamThread *AllocatedStream;
    stream_parameters_t newParameters;

    if (format == CAMERA2_HAL_PIXEL_FORMAT_OPAQUE &&
        m_camera2->isSupportedResolution(width, height)) {
        if (!(m_streamThreads[0].get())) {
            ALOGV("DEBUG(%s): stream 0 not exist", __FUNCTION__);
            allocCase = 0;
        }
        else {
            if ((m_streamThreads[0].get())->m_activated == true) {
                ALOGV("DEBUG(%s): stream 0 exists and activated.", __FUNCTION__);
                allocCase = 1;
            }
            else {
                ALOGV("DEBUG(%s): stream 0 exists and deactivated.", __FUNCTION__);
                allocCase = 2;
            }
        }
        if ((width == 1920 && height == 1080) || (width == 1280 && height == 720)) {
            m_wideAspect = true;
        }
        else {
            m_wideAspect = false;
        }
        ALOGV("DEBUG(%s): m_wideAspect (%d)", __FUNCTION__, m_wideAspect);

        if (allocCase == 0 || allocCase == 2) {
            *stream_id = 0;

            if (allocCase == 0) {
                m_streamThreads[0]  = new StreamThread(this, *stream_id);
             }
            AllocatedStream = (StreamThread*)(m_streamThreads[0].get());
            m_scp_flushing = false;
            m_scp_closing = false;
            m_scp_closed = false;
            usleep(100000); // TODO : guarantee the codes below will be run after readyToRunInternal()

            *format_actual = HAL_PIXEL_FORMAT_EXYNOS_YV12;
            *usage = GRALLOC_USAGE_SW_WRITE_OFTEN;
            *max_buffers = 8;

            newParameters.streamType    = STREAM_TYPE_DIRECT;
            newParameters.outputWidth   = width;
            newParameters.outputHeight  = height;
            newParameters.nodeWidth     = width;
            newParameters.nodeHeight    = height;
            newParameters.outputFormat  = *format_actual;
            newParameters.nodeFormat    = HAL_PIXEL_FORMAT_2_V4L2_PIX(*format_actual);
            newParameters.streamOps     = stream_ops;
            newParameters.usage         = *usage;
            newParameters.numHwBuffers  = 8;
            newParameters.numOwnSvcBuffers = *max_buffers;
            newParameters.fd            = m_fd_scp;
            newParameters.nodePlanes    = 3;
            newParameters.svcPlanes     = 3;
            newParameters.halBuftype    = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
            newParameters.memory        = V4L2_MEMORY_DMABUF;
            newParameters.ionClient     = m_ionCameraClient;
            newParameters.numSvcBufsInHal  = 0;
            AllocatedStream->m_index = *stream_id;
            AllocatedStream->setParameter(&newParameters);
            AllocatedStream->m_activated = true;

            m_scp_flushing = false;
            m_scp_closing = false;
            m_scp_closed = false;
            m_requestManager->SetDefaultParameters(m_camera2->getSensorW());
            m_camera_info.dummy_shot.shot.ctl.scaler.cropRegion[2] = m_camera2->getSensorW();
            return 0;
        }
        else if (allocCase == 1) {
            record_parameters_t recordParameters;
            StreamThread *parentStream;
            parentStream = (StreamThread*)(m_streamThreads[0].get());
            if (!parentStream) {
                return 1;
                // TODO
            }
            *stream_id = 2;
            usleep(100000); // TODO : guarantee the codes below will be run after readyToRunInternal()

            *format_actual = HAL_PIXEL_FORMAT_YCbCr_420_SP; // NV12M
            *usage = GRALLOC_USAGE_SW_WRITE_OFTEN;
            *max_buffers = 6;

            recordParameters.outputWidth   = width;
            recordParameters.outputHeight  = height;
            recordParameters.outputFormat     = *format_actual;
            recordParameters.svcPlanes        = NUM_PLANES(*format_actual);
            recordParameters.streamOps     = stream_ops;
            recordParameters.usage         = *usage;
            recordParameters.numOwnSvcBuffers = *max_buffers;
            recordParameters.numSvcBufsInHal  = 0;

            parentStream->setRecordingParameter(&recordParameters);
            m_scp_flushing = false;
            m_scp_closing = false;
            m_scp_closed = false;
            m_recordingEnabled = true;
            return 0;
        }
    }
    else if (format == HAL_PIXEL_FORMAT_BLOB
            && m_camera2->isSupportedJpegResolution(width, height)) {

        *stream_id = 1;

        if (!(m_streamThreads[*stream_id].get())) {
            ALOGV("DEBUG(%s): stream 0 not exist", __FUNCTION__);
            m_streamThreads[1]  = new StreamThread(this, *stream_id);
            allocCase = 0;
        }
        else {
            if ((m_streamThreads[*stream_id].get())->m_activated == true) {
                ALOGV("DEBUG(%s): stream 0 exists and activated.", __FUNCTION__);
                allocCase = 1;
            }
            else {
                ALOGV("DEBUG(%s): stream 0 exists and deactivated.", __FUNCTION__);
                allocCase = 2;
            }
        }

        AllocatedStream = (StreamThread*)(m_streamThreads[*stream_id].get());

        fd = m_camera_info.capture.fd;
        usleep(100000); // TODO : guarantee the codes below will be run after readyToRunInternal()

        *format_actual = HAL_PIXEL_FORMAT_BLOB;

        *usage = GRALLOC_USAGE_SW_WRITE_OFTEN;
        *max_buffers = 4;

        newParameters.streamType    = STREAM_TYPE_INDIRECT;
        newParameters.outputWidth   = width;
        newParameters.outputHeight  = height;

        newParameters.nodeWidth     = m_camera2->getSensorW();
        newParameters.nodeHeight    = m_camera2->getSensorH();

        newParameters.outputFormat  = *format_actual;
        newParameters.nodeFormat    = V4L2_PIX_FMT_YUYV;
        newParameters.streamOps     = stream_ops;
        newParameters.usage         = *usage;
        newParameters.numHwBuffers  = 8;
        newParameters.numOwnSvcBuffers = *max_buffers;
        newParameters.fd            = fd;
        newParameters.nodePlanes    = 1;
        newParameters.svcPlanes     = 1;
        newParameters.halBuftype    = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        newParameters.memory        = V4L2_MEMORY_DMABUF;
        newParameters.ionClient     = m_ionCameraClient;
        newParameters.numSvcBufsInHal  = 0;
        AllocatedStream->m_index = *stream_id;
        AllocatedStream->setParameter(&newParameters);
        return 0;
    }
    ALOGE("DEBUG(%s): Unsupported Pixel Format", __FUNCTION__);
    return 1; // TODO : check proper error code
}

int ExynosCameraHWInterface2::registerStreamBuffers(uint32_t stream_id,
        int num_buffers, buffer_handle_t *registeringBuffers)
{
    int                     i,j;
    void                    *virtAddr[3];
    uint32_t                plane_index = 0;
    stream_parameters_t     *targetStreamParms;
    record_parameters_t     *targetRecordParms;
    node_info_t             *currentNode;

    struct v4l2_buffer v4l2_buf;
    struct v4l2_plane  planes[VIDEO_MAX_PLANES];

    ALOGV("DEBUG(%s): streamID (%d), num_buff(%d), handle(%x) ", __FUNCTION__,
        stream_id, num_buffers, (uint32_t)registeringBuffers);

    if (stream_id == 0) {
        targetStreamParms = &(m_streamThreads[0]->m_parameters);
    }
    else if (stream_id == 1) {
        targetStreamParms = &(m_streamThreads[1]->m_parameters);
        // TODO : make clear stream off case
        m_need_streamoff = 0;

        if (m_camera_info.capture.status == false) {
            /* capture */
            m_camera_info.capture.buffers = 8;
            cam_int_s_fmt(&(m_camera_info.capture));
            cam_int_reqbufs(&(m_camera_info.capture));
            for (i = 0; i < m_camera_info.capture.buffers; i++) {
                ALOGV("DEBUG(%s): capture initial QBUF [%d]",  __FUNCTION__, i);
                cam_int_qbuf(&(m_camera_info.capture), i);
            }

            if (cam_int_streamon(&(m_camera_info.capture)) < 0) {
                ALOGE("ERR(%s): capture stream on fail", __FUNCTION__);
            } else {
                m_camera_info.capture.status = true;
            }
        }
    }
    else if (stream_id == 2) {
        m_need_streamoff = 0;
        targetRecordParms = &(m_streamThreads[0]->m_recordParameters);

        targetRecordParms->numSvcBuffers = num_buffers;

        for (i = 0 ; i<targetRecordParms->numSvcBuffers ; i++) {
            ALOGV("DEBUG(%s): registering Stream Buffers[%d] (%x) ", __FUNCTION__,
                i, (uint32_t)(registeringBuffers[i]));
            if (m_grallocHal) {
                if (m_grallocHal->lock(m_grallocHal, registeringBuffers[i],
                       targetRecordParms->usage, 0, 0,
                       targetRecordParms->outputWidth, targetRecordParms->outputHeight, virtAddr) != 0) {
                    ALOGE("ERR(%s): could not obtain gralloc buffer", __FUNCTION__);
                }
                else {
                    ExynosBuffer currentBuf;
                    const private_handle_t *priv_handle = reinterpret_cast<const private_handle_t *>(registeringBuffers[i]);
                    currentBuf.fd.extFd[0] = priv_handle->fd;
                    currentBuf.fd.extFd[1] = priv_handle->fd1;
                    currentBuf.fd.extFd[2] = priv_handle->fd2;
                    for (plane_index=0 ; plane_index < targetRecordParms->svcPlanes ; plane_index++) {
                        currentBuf.virt.extP[plane_index] = (char *)virtAddr[plane_index];
                        ALOGV("DEBUG(%s): plane(%d): fd(%d) addr(%x)",
                             __FUNCTION__, plane_index, currentBuf.fd.extFd[plane_index],
                             (unsigned int)currentBuf.virt.extP[plane_index]);
                    }
                    targetRecordParms->svcBufStatus[i]  = ON_SERVICE;
                    targetRecordParms->svcBuffers[i]    = currentBuf;
                    targetRecordParms->svcBufHandle[i]  = registeringBuffers[i];
                }
            }
        }
        m_needsRecordBufferInit = true;
        return 0;
    }
    else {
        ALOGE("ERR(%s) unregisterd stream id (%d)", __FUNCTION__, stream_id);
        return 1;
    }

    if (targetStreamParms->streamType == STREAM_TYPE_DIRECT) {
        if (num_buffers < targetStreamParms->numHwBuffers) {
            ALOGE("ERR(%s) registering insufficient num of buffers (%d) < (%d)",
                __FUNCTION__, num_buffers, targetStreamParms->numHwBuffers);
            return 1;
        }
    }
    ALOGV("DEBUG(%s): format(%x) width(%d), height(%d) svcPlanes(%d)",
            __FUNCTION__, targetStreamParms->outputFormat, targetStreamParms->outputWidth,
            targetStreamParms->outputHeight, targetStreamParms->svcPlanes);

    targetStreamParms->numSvcBuffers = num_buffers;
    currentNode = &(targetStreamParms->node); // TO Remove

    currentNode->fd         = targetStreamParms->fd;
    currentNode->width      = targetStreamParms->nodeWidth;
    currentNode->height     = targetStreamParms->nodeHeight;
    currentNode->format     = targetStreamParms->nodeFormat;
    currentNode->planes     = targetStreamParms->nodePlanes;
    currentNode->buffers    = targetStreamParms->numHwBuffers;
    currentNode->type       = targetStreamParms->halBuftype;
    currentNode->memory     = targetStreamParms->memory;
    currentNode->ionClient  = targetStreamParms->ionClient;

    if (targetStreamParms->streamType == STREAM_TYPE_DIRECT) {
        if(m_need_streamoff == 1) {
            if (m_sensorThread != NULL) {
                m_sensorThread->release();
                /* TODO */
                usleep(500000);
            } else {
                ALOGE("+++++++ sensor thread is NULL %d", __LINE__);
            }

            ALOGV("(%s): calling capture streamoff", __FUNCTION__);
            if (cam_int_streamoff(&(m_camera_info.capture)) < 0) {
                ALOGE("ERR(%s): capture stream off fail", __FUNCTION__);
            } else {
                m_camera_info.capture.status = false;
            }

            ALOGV("(%s): calling capture streamoff done", __FUNCTION__);

            m_camera_info.capture.buffers = 0;
            ALOGV("DEBUG(%s): capture calling reqbuf 0 ", __FUNCTION__);
            cam_int_reqbufs(&(m_camera_info.capture));
            ALOGV("DEBUG(%s): capture calling reqbuf 0 done", __FUNCTION__);

            m_isIspStarted = false;
        }

        if (m_need_streamoff == 1) {
            m_camera_info.sensor.buffers = NUM_BAYER_BUFFERS;
            m_camera_info.isp.buffers = m_camera_info.sensor.buffers;
            m_camera_info.capture.buffers = 8;
            /* isp */
            cam_int_s_fmt(&(m_camera_info.isp));
            cam_int_reqbufs(&(m_camera_info.isp));
            /* sensor */
            cam_int_s_fmt(&(m_camera_info.sensor));
            cam_int_reqbufs(&(m_camera_info.sensor));

            for (i = 0; i < 8; i++) {
                ALOGV("DEBUG(%s): sensor initial QBUF [%d]",  __FUNCTION__, i);
                memcpy( m_camera_info.sensor.buffer[i].virt.extP[1], &(m_camera_info.dummy_shot),
                        sizeof(struct camera2_shot_ext));
                m_camera_info.dummy_shot.shot.ctl.sensor.frameDuration = 33*1000*1000; // apply from frame #1
                m_camera_info.dummy_shot.shot.ctl.request.frameCount = -1;
                cam_int_qbuf(&(m_camera_info.sensor), i);
            }

            /* capture */
            cam_int_s_fmt(&(m_camera_info.capture));
            cam_int_reqbufs(&(m_camera_info.capture));
            for (i = 0; i < m_camera_info.capture.buffers; i++) {
                ALOGV("DEBUG(%s): capture initial QBUF [%d]",  __FUNCTION__, i);
                cam_int_qbuf(&(m_camera_info.capture), i);
            }

       }

        cam_int_s_input(currentNode, m_camera_info.sensor_id);
        cam_int_s_fmt(currentNode);
        cam_int_reqbufs(currentNode);

    }
    else if (targetStreamParms->streamType == STREAM_TYPE_INDIRECT) {
        for(i = 0; i < currentNode->buffers; i++){
            memcpy(&(currentNode->buffer[i]), &(m_camera_info.capture.buffer[i]), sizeof(ExynosBuffer));
        }
    }

    for (i = 0 ; i<targetStreamParms->numSvcBuffers ; i++) {
        ALOGV("DEBUG(%s): registering Stream Buffers[%d] (%x) ", __FUNCTION__,
            i, (uint32_t)(registeringBuffers[i]));
        if (m_grallocHal) {
            if (m_grallocHal->lock(m_grallocHal, registeringBuffers[i],
                   targetStreamParms->usage, 0, 0,
                   currentNode->width, currentNode->height, virtAddr) != 0) {
                ALOGE("ERR(%s): could not obtain gralloc buffer", __FUNCTION__);
            }
            else {
                v4l2_buf.m.planes   = planes;
                v4l2_buf.type       = currentNode->type;
                v4l2_buf.memory     = currentNode->memory;
                v4l2_buf.index      = i;
                v4l2_buf.length     = currentNode->planes;

                ExynosBuffer currentBuf;
                const private_handle_t *priv_handle = reinterpret_cast<const private_handle_t *>(registeringBuffers[i]);

                m_getAlignedYUVSize(currentNode->format,
                    currentNode->width, currentNode->height, &currentBuf);

                v4l2_buf.m.planes[0].m.fd = priv_handle->fd;
                v4l2_buf.m.planes[2].m.fd = priv_handle->fd1;
                v4l2_buf.m.planes[1].m.fd = priv_handle->fd2;
                currentBuf.fd.extFd[0] = priv_handle->fd;
                currentBuf.fd.extFd[2] = priv_handle->fd1;
                currentBuf.fd.extFd[1] = priv_handle->fd2;
                ALOGV("DEBUG(%s):  ion_size(%d), stride(%d), ", __FUNCTION__, priv_handle->size, priv_handle->stride);
                if (currentNode->planes == 1) {
                    currentBuf.size.extS[0] = priv_handle->size;
                    currentBuf.size.extS[1] = 0;
                    currentBuf.size.extS[2] = 0;
                }
                for (plane_index = 0 ; plane_index < v4l2_buf.length ; plane_index++) {
                    currentBuf.virt.extP[plane_index] = (char *)virtAddr[plane_index];
                    v4l2_buf.m.planes[plane_index].length  = currentBuf.size.extS[plane_index];
                    ALOGV("DEBUG(%s): plane(%d): fd(%d) addr(%x), length(%d)",
                         __FUNCTION__, plane_index, v4l2_buf.m.planes[plane_index].m.fd,
                         (unsigned int)currentBuf.virt.extP[plane_index],
                         v4l2_buf.m.planes[plane_index].length);
                }

                if (targetStreamParms->streamType == STREAM_TYPE_DIRECT) {
                    if (i < currentNode->buffers) {
                        if (exynos_v4l2_qbuf(currentNode->fd, &v4l2_buf) < 0) {
                            ALOGE("ERR(%s): stream id(%d) exynos_v4l2_qbuf() fail fd(%d)",
                                __FUNCTION__, stream_id, currentNode->fd);
                            //return false;
                        }
                        ALOGV("DEBUG(%s): stream id(%d) exynos_v4l2_qbuf() success fd(%d)",
                                __FUNCTION__, stream_id, currentNode->fd);
                        targetStreamParms->svcBufStatus[i]  = REQUIRES_DQ_FROM_SVC;
                    }
                    else {
                        targetStreamParms->svcBufStatus[i]  = ON_SERVICE;
                    }
                }
                else if (targetStreamParms->streamType == STREAM_TYPE_INDIRECT) {
                    targetStreamParms->svcBufStatus[i]  = ON_SERVICE;
                }
                targetStreamParms->svcBuffers[i]       = currentBuf;
                targetStreamParms->svcBufHandle[i]     = registeringBuffers[i];
            }
        }
    }

    ALOGV("DEBUG(%s): calling  streamon", __FUNCTION__);
    if (targetStreamParms->streamType == STREAM_TYPE_DIRECT) {
        ALOGD("%s(%d), stream id = %d", __FUNCTION__, __LINE__, stream_id);
        cam_int_streamon(&(targetStreamParms->node));
    }

    if (m_need_streamoff == 1) {
        if (cam_int_streamon(&(m_camera_info.capture)) < 0) {
            ALOGE("ERR(%s): capture stream on fail", __FUNCTION__);
        } else {
            m_camera_info.capture.status = true;
        }

        cam_int_streamon(&(m_camera_info.sensor));
    }

    ALOGV("DEBUG(%s): calling  streamon END", __FUNCTION__);
    ALOGV("DEBUG(%s): END registerStreamBuffers", __FUNCTION__);

    if(!m_isIspStarted) {
        m_isIspStarted = true;
        StartISP();
    }

    if (m_need_streamoff == 1) {
        m_requestManager->SetInitialSkip(8);
        m_sensorThread->Start("SensorThread", PRIORITY_DEFAULT, 0);
        m_mainThread->SetSignal(SIGNAL_MAIN_REQ_Q_NOT_EMPTY);
    }
    m_need_streamoff = 1;

    return 0;
}

int ExynosCameraHWInterface2::releaseStream(uint32_t stream_id)
{
    StreamThread *targetStream;
    ALOGV("DEBUG(%s):", __FUNCTION__);

    if (stream_id == 0) {
        targetStream = (StreamThread*)(m_streamThreads[0].get());
        m_scp_flushing = true;
    }
    else if (stream_id == 1) {
        targetStream = (StreamThread*)(m_streamThreads[1].get());
    }
    else if (stream_id == 2 && m_recordingEnabled) {
        m_recordingEnabled = false;
        m_needsRecordBufferInit = true;
        return 0;
    }
    else {
        ALOGE("ERR:(%s): wrong stream id (%d)", __FUNCTION__, stream_id);
        return 1;
    }

    targetStream->m_releasing = true;
    do {
        ALOGD("stream thread release %d", __LINE__);
        targetStream->release();
        usleep(33000);
    } while (targetStream->m_releasing);
    targetStream->m_activated = false;
    ALOGV("DEBUG(%s): DONE", __FUNCTION__);
    return 0;
}

int ExynosCameraHWInterface2::allocateReprocessStream(
    uint32_t width, uint32_t height, uint32_t format,
    const camera2_stream_in_ops_t *reprocess_stream_ops,
    uint32_t *stream_id, uint32_t *consumer_usage, uint32_t *max_buffers)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return 0;
}

int ExynosCameraHWInterface2::releaseReprocessStream(uint32_t stream_id)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return 0;
}

int ExynosCameraHWInterface2::triggerAction(uint32_t trigger_id, int ext1, int ext2)
{
    ALOGV("DEBUG(%s): id(%x), %d, %d", __FUNCTION__, trigger_id, ext1, ext2);

    switch (trigger_id) {
    case CAMERA2_TRIGGER_AUTOFOCUS:
        ALOGV("DEBUG(%s):TRIGGER_AUTOFOCUS id(%d)", __FUNCTION__, ext1);
        OnAfTrigger(ext1);
        break;

    case CAMERA2_TRIGGER_CANCEL_AUTOFOCUS:
        ALOGV("DEBUG(%s):CANCEL_AUTOFOCUS id(%d)", __FUNCTION__, ext1);
        OnAfCancel(ext1);
        break;
    default:
        break;
    }
    return 0;
}

int ExynosCameraHWInterface2::setNotifyCallback(camera2_notify_callback notify_cb, void *user)
{
    ALOGV("DEBUG(%s): cb_addr(%x)", __FUNCTION__, (unsigned int)notify_cb);
    m_notifyCb = notify_cb;
    m_callbackCookie = user;
    return 0;
}

int ExynosCameraHWInterface2::getMetadataVendorTagOps(vendor_tag_query_ops_t **ops)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return 0;
}

int ExynosCameraHWInterface2::dump(int fd)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return 0;
}

void ExynosCameraHWInterface2::m_getAlignedYUVSize(int colorFormat, int w, int h, ExynosBuffer *buf)
{
    switch (colorFormat) {
    // 1p
    case V4L2_PIX_FMT_RGB565 :
    case V4L2_PIX_FMT_YUYV :
    case V4L2_PIX_FMT_UYVY :
    case V4L2_PIX_FMT_VYUY :
    case V4L2_PIX_FMT_YVYU :
        buf->size.extS[0] = FRAME_SIZE(V4L2_PIX_2_HAL_PIXEL_FORMAT(colorFormat), w, h);
        buf->size.extS[1] = 0;
        buf->size.extS[2] = 0;
        break;
    // 2p
    case V4L2_PIX_FMT_NV12 :
    case V4L2_PIX_FMT_NV12T :
    case V4L2_PIX_FMT_NV21 :
        buf->size.extS[0] = ALIGN(w,   16) * ALIGN(h,   16);
        buf->size.extS[1] = ALIGN(w/2, 16) * ALIGN(h/2, 16);
        buf->size.extS[2] = 0;
        break;
    case V4L2_PIX_FMT_NV12M :
    case V4L2_PIX_FMT_NV12MT_16X16 :
    case V4L2_PIX_FMT_NV21M:
        buf->size.extS[0] = ALIGN(w, 16) * ALIGN(h,     16);
        buf->size.extS[1] = ALIGN(buf->size.extS[0] / 2, 256);
        buf->size.extS[2] = 0;
        break;
    case V4L2_PIX_FMT_NV16 :
    case V4L2_PIX_FMT_NV61 :
        buf->size.extS[0] = ALIGN(w, 16) * ALIGN(h, 16);
        buf->size.extS[1] = ALIGN(w, 16) * ALIGN(h,  16);
        buf->size.extS[2] = 0;
        break;
     // 3p
    case V4L2_PIX_FMT_YUV420 :
    case V4L2_PIX_FMT_YVU420 :
        buf->size.extS[0] = (w * h);
        buf->size.extS[1] = (w * h) >> 2;
        buf->size.extS[2] = (w * h) >> 2;
        break;
    case V4L2_PIX_FMT_YUV420M:
    case V4L2_PIX_FMT_YVU420M :
    case V4L2_PIX_FMT_YUV422P :
        buf->size.extS[0] = ALIGN(w,  32) * ALIGN(h,  16);
        buf->size.extS[1] = ALIGN(w/2, 16) * ALIGN(h/2, 8);
        buf->size.extS[2] = ALIGN(w/2, 16) * ALIGN(h/2, 8);
        break;
    default:
        ALOGE("ERR(%s):unmatched colorFormat(%d)", __FUNCTION__, colorFormat);
        return;
        break;
    }
}

bool ExynosCameraHWInterface2::m_getRatioSize(int  src_w,  int   src_h,
                                             int  dst_w,  int   dst_h,
                                             int *crop_x, int *crop_y,
                                             int *crop_w, int *crop_h,
                                             int zoom)
{
    *crop_w = src_w;
    *crop_h = src_h;

    if (   src_w != dst_w
        || src_h != dst_h) {
        float src_ratio = 1.0f;
        float dst_ratio = 1.0f;

        // ex : 1024 / 768
        src_ratio = (float)src_w / (float)src_h;

        // ex : 352  / 288
        dst_ratio = (float)dst_w / (float)dst_h;

        if (dst_w * dst_h < src_w * src_h) {
            if (dst_ratio <= src_ratio) {
                // shrink w
                *crop_w = src_h * dst_ratio;
                *crop_h = src_h;
            } else {
                // shrink h
                *crop_w = src_w;
                *crop_h = src_w / dst_ratio;
            }
        } else {
            if (dst_ratio <= src_ratio) {
                // shrink w
                *crop_w = src_h * dst_ratio;
                *crop_h = src_h;
            } else {
                // shrink h
                *crop_w = src_w;
                *crop_h = src_w / dst_ratio;
            }
        }
    }

    if (zoom != 0) {
        float zoomLevel = ((float)zoom + 10.0) / 10.0;
        *crop_w = (int)((float)*crop_w / zoomLevel);
        *crop_h = (int)((float)*crop_h / zoomLevel);
    }

    #define CAMERA_CROP_WIDTH_RESTRAIN_NUM  (0x2)
    unsigned int w_align = (*crop_w & (CAMERA_CROP_WIDTH_RESTRAIN_NUM - 1));
    if (w_align != 0) {
        if (  (CAMERA_CROP_WIDTH_RESTRAIN_NUM >> 1) <= w_align
            && *crop_w + (CAMERA_CROP_WIDTH_RESTRAIN_NUM - w_align) <= dst_w) {
            *crop_w += (CAMERA_CROP_WIDTH_RESTRAIN_NUM - w_align);
        }
        else
            *crop_w -= w_align;
    }

    #define CAMERA_CROP_HEIGHT_RESTRAIN_NUM  (0x2)
    unsigned int h_align = (*crop_h & (CAMERA_CROP_HEIGHT_RESTRAIN_NUM - 1));
    if (h_align != 0) {
        if (  (CAMERA_CROP_HEIGHT_RESTRAIN_NUM >> 1) <= h_align
            && *crop_h + (CAMERA_CROP_HEIGHT_RESTRAIN_NUM - h_align) <= dst_h) {
            *crop_h += (CAMERA_CROP_HEIGHT_RESTRAIN_NUM - h_align);
        }
        else
            *crop_h -= h_align;
    }

    *crop_x = (src_w - *crop_w) >> 1;
    *crop_y = (src_h - *crop_h) >> 1;

    if (*crop_x & (CAMERA_CROP_WIDTH_RESTRAIN_NUM >> 1))
        *crop_x -= 1;

    if (*crop_y & (CAMERA_CROP_HEIGHT_RESTRAIN_NUM >> 1))
        *crop_y -= 1;

    return true;
}

BayerBufManager::BayerBufManager()
{
    ALOGV("DEBUG(%s): ", __FUNCTION__);
    for (int i = 0; i < NUM_BAYER_BUFFERS ; i++) {
        entries[i].status = BAYER_ON_HAL_EMPTY;
        entries[i].reqFrameCnt = 0;
    }
    sensorEnqueueHead = 0;
    sensorDequeueHead = 0;
    ispEnqueueHead = 0;
    ispDequeueHead = 0;
    numOnSensor = 0;
    numOnIsp = 0;
    numOnHalFilled = 0;
    numOnHalEmpty = NUM_BAYER_BUFFERS;
}

BayerBufManager::~BayerBufManager()
{
    ALOGV("%s", __FUNCTION__);
}

int     BayerBufManager::GetIndexForSensorEnqueue()
{
    int ret = 0;
    if (numOnHalEmpty == 0)
        ret = -1;
    else
        ret = sensorEnqueueHead;
    ALOGV("DEBUG(%s): returning (%d)", __FUNCTION__, ret);
    return ret;
}

int    BayerBufManager::MarkSensorEnqueue(int index)
{
    ALOGV("DEBUG(%s)    : BayerIndex[%d] ", __FUNCTION__, index);

    // sanity check
    if (index != sensorEnqueueHead) {
        ALOGV("DEBUG(%s)    : Abnormal BayerIndex[%d] - expected[%d]", __FUNCTION__, index, sensorEnqueueHead);
        return -1;
    }
    if (entries[index].status != BAYER_ON_HAL_EMPTY) {
        ALOGV("DEBUG(%s)    : Abnormal status in BayerIndex[%d] = (%d) expected (%d)", __FUNCTION__,
            index, entries[index].status, BAYER_ON_HAL_EMPTY);
        return -1;
    }

    entries[index].status = BAYER_ON_SENSOR;
    entries[index].reqFrameCnt = 0;
    numOnHalEmpty--;
    numOnSensor++;
    sensorEnqueueHead = GetNextIndex(index);
    ALOGV("DEBUG(%s) END: HAL-e(%d) HAL-f(%d) Sensor(%d) ISP(%d) ",
        __FUNCTION__, numOnHalEmpty, numOnHalFilled, numOnSensor, numOnIsp);
    return 0;
}

int    BayerBufManager::MarkSensorDequeue(int index, int reqFrameCnt, nsecs_t *timeStamp)
{
    ALOGV("DEBUG(%s)    : BayerIndex[%d] reqFrameCnt(%d)", __FUNCTION__, index, reqFrameCnt);

    if (entries[index].status != BAYER_ON_SENSOR) {
        ALOGE("DEBUG(%s)    : Abnormal status in BayerIndex[%d] = (%d) expected (%d)", __FUNCTION__,
            index, entries[index].status, BAYER_ON_SENSOR);
        return -1;
    }

    entries[index].status = BAYER_ON_HAL_FILLED;
    numOnHalFilled++;
    numOnSensor--;

    return 0;
}

int     BayerBufManager::GetIndexForIspEnqueue(int *reqFrameCnt)
{
    int ret = 0;
    if (numOnHalFilled == 0)
        ret = -1;
    else {
        *reqFrameCnt = entries[ispEnqueueHead].reqFrameCnt;
        ret = ispEnqueueHead;
    }
    ALOGV("DEBUG(%s): returning BayerIndex[%d]", __FUNCTION__, ret);
    return ret;
}

int     BayerBufManager::GetIndexForIspDequeue(int *reqFrameCnt)
{
    int ret = 0;
    if (numOnIsp == 0)
        ret = -1;
    else {
        *reqFrameCnt = entries[ispDequeueHead].reqFrameCnt;
        ret = ispDequeueHead;
    }
    ALOGV("DEBUG(%s): returning BayerIndex[%d]", __FUNCTION__, ret);
    return ret;
}

int    BayerBufManager::MarkIspEnqueue(int index)
{
    ALOGV("DEBUG(%s)    : BayerIndex[%d] ", __FUNCTION__, index);

    // sanity check
    if (index != ispEnqueueHead) {
        ALOGV("DEBUG(%s)    : Abnormal BayerIndex[%d] - expected[%d]", __FUNCTION__, index, ispEnqueueHead);
        return -1;
    }
    if (entries[index].status != BAYER_ON_HAL_FILLED) {
        ALOGV("DEBUG(%s)    : Abnormal status in BayerIndex[%d] = (%d) expected (%d)", __FUNCTION__,
            index, entries[index].status, BAYER_ON_HAL_FILLED);
        return -1;
    }

    entries[index].status = BAYER_ON_ISP;
    numOnHalFilled--;
    numOnIsp++;
    ispEnqueueHead = GetNextIndex(index);
    ALOGV("DEBUG(%s) END: HAL-e(%d) HAL-f(%d) Sensor(%d) ISP(%d) ",
        __FUNCTION__, numOnHalEmpty, numOnHalFilled, numOnSensor, numOnIsp);
    return 0;
}

int    BayerBufManager::MarkIspDequeue(int index)
{
    ALOGV("DEBUG(%s)    : BayerIndex[%d]", __FUNCTION__, index);

    // sanity check
    if (index != ispDequeueHead) {
        ALOGV("DEBUG(%s)    : Abnormal BayerIndex[%d] - expected[%d]", __FUNCTION__, index, ispDequeueHead);
        return -1;
    }
    if (entries[index].status != BAYER_ON_ISP) {
        ALOGV("DEBUG(%s)    : Abnormal status in BayerIndex[%d] = (%d) expected (%d)", __FUNCTION__,
            index, entries[index].status, BAYER_ON_ISP);
        return -1;
    }

    entries[index].status = BAYER_ON_HAL_EMPTY;
    entries[index].reqFrameCnt = 0;
    numOnHalEmpty++;
    numOnIsp--;
    ispDequeueHead = GetNextIndex(index);
    ALOGV("DEBUG(%s) END: HAL-e(%d) HAL-f(%d) Sensor(%d) ISP(%d) ",
        __FUNCTION__, numOnHalEmpty, numOnHalFilled, numOnSensor, numOnIsp);
    return 0;
}

int BayerBufManager::GetNumOnSensor()
{
    return numOnSensor;
}

int BayerBufManager::GetNumOnHalFilled()
{
    return numOnHalFilled;
}

int BayerBufManager::GetNumOnIsp()
{
    return numOnIsp;
}

int     BayerBufManager::GetNextIndex(int index)
{
    index++;
    if (index >= NUM_BAYER_BUFFERS)
        index = 0;

    return index;
}

void ExynosCameraHWInterface2::m_mainThreadFunc(SignalDrivenThread * self)
{
    camera_metadata_t *currentRequest = NULL;
    camera_metadata_t *currentFrame = NULL;
    size_t numEntries = 0;
    size_t frameSize = 0;
    camera_metadata_t * preparedFrame = NULL;
    camera_metadata_t *deregisteredRequest = NULL;
    uint32_t currentSignal = self->GetProcessingSignal();
    MainThread *  selfThread      = ((MainThread*)self);
    int res = 0;

    int ret;

    ALOGV("DEBUG(%s): m_mainThreadFunc (%x)", __FUNCTION__, currentSignal);

    if (currentSignal & SIGNAL_THREAD_RELEASE) {
        ALOGV("DEBUG(%s): processing SIGNAL_THREAD_RELEASE", __FUNCTION__);

        ALOGV("DEBUG(%s): processing SIGNAL_THREAD_RELEASE DONE", __FUNCTION__);
        selfThread->SetSignal(SIGNAL_THREAD_TERMINATE);
        return;
    }

    if (currentSignal & SIGNAL_MAIN_REQ_Q_NOT_EMPTY) {
        ALOGV("DEBUG(%s): MainThread processing SIGNAL_MAIN_REQ_Q_NOT_EMPTY", __FUNCTION__);
        if (m_requestManager->IsRequestQueueFull()==false) {
            m_requestQueueOps->dequeue_request(m_requestQueueOps, &currentRequest);
            if (NULL == currentRequest) {
                ALOGE("DEBUG(%s)(0x%x): dequeue_request returned NULL ", __FUNCTION__, currentSignal);
                m_isRequestQueueNull = true;
            }
            else {
                m_requestManager->RegisterRequest(currentRequest);

                m_numOfRemainingReqInSvc = m_requestQueueOps->request_count(m_requestQueueOps);
                ALOGV("DEBUG(%s): remaining req cnt (%d)", __FUNCTION__, m_numOfRemainingReqInSvc);
                if (m_requestManager->IsRequestQueueFull()==false)
                    selfThread->SetSignal(SIGNAL_MAIN_REQ_Q_NOT_EMPTY); // dequeue repeatedly

                m_sensorThread->SetSignal(SIGNAL_SENSOR_START_REQ_PROCESSING);
            }
        }
        else {
            m_isRequestQueuePending = true;
        }
    }

    if (currentSignal & SIGNAL_MAIN_STREAM_OUTPUT_DONE) {
        ALOGV("DEBUG(%s): MainThread processing SIGNAL_MAIN_STREAM_OUTPUT_DONE", __FUNCTION__);
        /*while (1)*/ {
            ret = m_requestManager->PrepareFrame(&numEntries, &frameSize, &preparedFrame, GetAfStateForService());
            if (ret == false)
                ALOGD("++++++ PrepareFrame ret = %d", ret);

            m_requestManager->DeregisterRequest(&deregisteredRequest);

            ret = m_requestQueueOps->free_request(m_requestQueueOps, deregisteredRequest);
            if (ret < 0)
                ALOGD("++++++ free_request ret = %d", ret);

            ret = m_frameQueueOps->dequeue_frame(m_frameQueueOps, numEntries, frameSize, &currentFrame);
            if (ret < 0)
                ALOGD("++++++ dequeue_frame ret = %d", ret);

            if (currentFrame==NULL) {
                ALOGV("DBG(%s): frame dequeue returned NULL",__FUNCTION__ );
            }
            else {
                ALOGV("DEBUG(%s): frame dequeue done. numEntries(%d) frameSize(%d)",__FUNCTION__ , numEntries, frameSize);
            }
            res = append_camera_metadata(currentFrame, preparedFrame);
            if (res==0) {
                ALOGV("DEBUG(%s): frame metadata append success",__FUNCTION__);
                m_frameQueueOps->enqueue_frame(m_frameQueueOps, currentFrame);
            }
            else {
                ALOGE("ERR(%s): frame metadata append fail (%d)",__FUNCTION__, res);
            }
        }
        if (!m_isRequestQueueNull) {
            selfThread->SetSignal(SIGNAL_MAIN_REQ_Q_NOT_EMPTY);
        }

        if (getInProgressCount()>0) {
            ALOGV("DEBUG(%s): STREAM_OUTPUT_DONE and signalling REQ_PROCESSING",__FUNCTION__);
            m_sensorThread->SetSignal(SIGNAL_SENSOR_START_REQ_PROCESSING);
        }
    }
    ALOGV("DEBUG(%s): MainThread Exit", __FUNCTION__);
    return;
}

void ExynosCameraHWInterface2::m_sensorThreadInitialize(SignalDrivenThread * self)
{
    ALOGV("DEBUG(%s): ", __FUNCTION__ );
    /* will add */
    return;
}


void ExynosCameraHWInterface2::DumpInfoWithShot(struct camera2_shot_ext * shot_ext)
{
    ALOGD("####  common Section");
    ALOGD("####                 magic(%x) ",
        shot_ext->shot.magicNumber);
    ALOGD("####  ctl Section");
    ALOGD("####     meta(%d) aper(%f) exp(%lld) duration(%lld) ISO(%d) AWB(%d)",
        shot_ext->shot.ctl.request.metadataMode,
        shot_ext->shot.ctl.lens.aperture,
        shot_ext->shot.ctl.sensor.exposureTime,
        shot_ext->shot.ctl.sensor.frameDuration,
        shot_ext->shot.ctl.sensor.sensitivity,
        shot_ext->shot.ctl.aa.awbMode);

    ALOGD("####                 OutputStream Sensor(%d) SCP(%d) SCC(%d) pv(%d) rec(%d)",
        shot_ext->request_sensor, shot_ext->request_scp, shot_ext->request_scc,
        shot_ext->shot.ctl.request.outputStreams[0],
        shot_ext->shot.ctl.request.outputStreams[2]);

    ALOGD("####  DM Section");
    ALOGD("####     meta(%d) aper(%f) exp(%lld) duration(%lld) ISO(%d) timestamp(%lld) AWB(%d) cnt(%d)",
        shot_ext->shot.dm.request.metadataMode,
        shot_ext->shot.dm.lens.aperture,
        shot_ext->shot.dm.sensor.exposureTime,
        shot_ext->shot.dm.sensor.frameDuration,
        shot_ext->shot.dm.sensor.sensitivity,
        shot_ext->shot.dm.sensor.timeStamp,
        shot_ext->shot.dm.aa.awbMode,
        shot_ext->shot.dm.request.frameCount );
}

void ExynosCameraHWInterface2::m_sensorThreadFunc(SignalDrivenThread * self)
{
    uint32_t        currentSignal = self->GetProcessingSignal();
    SensorThread *  selfThread      = ((SensorThread*)self);
    int index;
    int index_isp;
    status_t res;
    nsecs_t frameTime;
    int bayersOnSensor = 0, bayersOnIsp = 0;
    int j = 0;
    bool isCapture = false;
    ALOGV("DEBUG(%s): m_sensorThreadFunc (%x)", __FUNCTION__, currentSignal);

    if (currentSignal & SIGNAL_THREAD_RELEASE) {
        ALOGV("(%s): ENTER processing SIGNAL_THREAD_RELEASE", __FUNCTION__);

        ALOGV("(%s): calling sensor streamoff", __FUNCTION__);
        cam_int_streamoff(&(m_camera_info.sensor));
        ALOGV("(%s): calling sensor streamoff done", __FUNCTION__);

        m_camera_info.sensor.buffers = 0;
        ALOGV("DEBUG(%s): sensor calling reqbuf 0 ", __FUNCTION__);
        cam_int_reqbufs(&(m_camera_info.sensor));
        ALOGV("DEBUG(%s): sensor calling reqbuf 0 done", __FUNCTION__);

        ALOGV("(%s): calling ISP streamoff", __FUNCTION__);
        isp_int_streamoff(&(m_camera_info.isp));
        ALOGV("(%s): calling ISP streamoff done", __FUNCTION__);

        m_camera_info.isp.buffers = 0;
        ALOGV("DEBUG(%s): isp calling reqbuf 0 ", __FUNCTION__);
        cam_int_reqbufs(&(m_camera_info.isp));
        ALOGV("DEBUG(%s): isp calling reqbuf 0 done", __FUNCTION__);

        exynos_v4l2_s_ctrl(m_camera_info.sensor.fd, V4L2_CID_IS_S_STREAM, IS_DISABLE_STREAM);

        ALOGV("(%s): EXIT processing SIGNAL_THREAD_RELEASE", __FUNCTION__);
        selfThread->SetSignal(SIGNAL_THREAD_TERMINATE);
        return;
    }

    if (currentSignal & SIGNAL_SENSOR_START_REQ_PROCESSING)
    {
        ALOGV("DEBUG(%s): SensorThread processing SIGNAL_SENSOR_START_REQ_PROCESSING", __FUNCTION__);
        int targetStreamIndex = 0, i=0;
        int matchedFrameCnt = -1, processingReqIndex;
        struct camera2_shot_ext *shot_ext;
        struct camera2_shot_ext *shot_ext_capture;
        bool triggered = false;
        int afMode;

        /* dqbuf from sensor */
        ALOGV("Sensor DQbuf start");
        index = cam_int_dqbuf(&(m_camera_info.sensor));
        shot_ext = (struct camera2_shot_ext *)(m_camera_info.sensor.buffer[index].virt.extP[1]);

        m_recordOutput = shot_ext->shot.ctl.request.outputStreams[2];

        matchedFrameCnt = m_requestManager->FindFrameCnt(shot_ext);

        if (matchedFrameCnt != -1) {
                frameTime = systemTime();
        m_requestManager->RegisterTimestamp(matchedFrameCnt, &frameTime);
            m_requestManager->UpdateIspParameters(shot_ext, matchedFrameCnt, false);
            if (m_IsAfModeUpdateRequired) {
                ALOGE("### AF Mode change(Mode %d) ", m_afMode);
                shot_ext->shot.ctl.aa.afMode = m_afMode;
                if (m_afMode == AA_AFMODE_CONTINUOUS_VIDEO || m_afMode == AA_AFMODE_CONTINUOUS_PICTURE) {
                    ALOGE("### With Automatic triger for continuous modes");
                    m_afState = HAL_AFSTATE_STARTED;
                    shot_ext->shot.ctl.aa.afTrigger = 1;
                    triggered = true;
                }
                m_IsAfModeUpdateRequired = false;
                if (m_afMode2 != NO_CHANGE) {
                    enum aa_afmode tempAfMode = m_afMode2;
                    m_afMode2 = NO_CHANGE;
                    SetAfMode(tempAfMode);
                }
            }
            else {
                shot_ext->shot.ctl.aa.afMode = NO_CHANGE;
            }
            if (m_IsAfTriggerRequired) {
                ALOGE("### AF Triggering with mode (%d)", m_afMode);
                if (m_afState == HAL_AFSTATE_SCANNING) {
                     ALOGE("(%s): restarting trigger ", __FUNCTION__);
                }
                else {
                    if (m_afState != HAL_AFSTATE_NEEDS_COMMAND)
                        ALOGE("(%s): wrong trigger state %d", __FUNCTION__, m_afState);
                    else
                        m_afState = HAL_AFSTATE_STARTED;
                }
                shot_ext->shot.ctl.aa.afMode = m_afMode;
                m_IsAfTriggerRequired = false;
            }
            else {
            }
            if (m_wideAspect) {
//                shot_ext->setfile = ISS_SUB_SCENARIO_VIDEO;
                shot_ext->shot.ctl.aa.aeTargetFpsRange[0] = 30;
                shot_ext->shot.ctl.aa.aeTargetFpsRange[1] = 30;
            }
            else {
//                shot_ext->setfile = ISS_SUB_SCENARIO_STILL;
            }
            if (triggered)
                shot_ext->shot.ctl.aa.afTrigger = 1;

            // TODO : check collision with AFMode Update
            if (m_IsAfLockRequired) {
                shot_ext->shot.ctl.aa.afMode = AA_AFMODE_OFF;
                m_IsAfLockRequired = false;
            }
            ALOGV("### Isp Qbuf start(%d) count (%d), SCP(%d) SCC(%d) DIS(%d) shot_size(%d)",
                index,
                shot_ext->shot.ctl.request.frameCount,
                shot_ext->request_scp,
                shot_ext->request_scc,
                shot_ext->dis_bypass, sizeof(camera2_shot));

            if(shot_ext->request_scc == 1) {
                isCapture = true;
            }

            if(isCapture)
            {
                for(j = 0; j < m_camera_info.isp.buffers; j++)
                {
                    shot_ext_capture = (struct camera2_shot_ext *)(m_camera_info.isp.buffer[j].virt.extP[1]);
                    shot_ext_capture->request_scc = 1;
                }
            }

            cam_int_qbuf(&(m_camera_info.isp), index);
            //m_ispThread->SetSignal(SIGNAL_ISP_START_BAYER_DEQUEUE);

            usleep(10000);
            if(isCapture)
            {
                for(j = 0; j < m_camera_info.isp.buffers; j++)
                {
                    shot_ext_capture = (struct camera2_shot_ext *)(m_camera_info.isp.buffer[j].virt.extP[1]);
                    ALOGD("shot_ext_capture[%d] scp = %d, scc = %d", j, shot_ext_capture->request_scp, shot_ext_capture->request_scc);
//                    DumpInfoWithShot(shot_ext_capture);
                }
            }


            ALOGV("### isp DQBUF start");
            index_isp = cam_int_dqbuf(&(m_camera_info.isp));
            //m_previewOutput = 0;

            if(isCapture)
            {
                for(j = 0; j < m_camera_info.isp.buffers; j++)
                {
                    shot_ext_capture = (struct camera2_shot_ext *)(m_camera_info.isp.buffer[j].virt.extP[1]);
                    ALOGD("shot_ext_capture[%d] scp = %d, scc = %d", j, shot_ext_capture->request_scp, shot_ext_capture->request_scc);
//                    DumpInfoWithShot(shot_ext_capture);
                }
            }
            shot_ext = (struct camera2_shot_ext *)(m_camera_info.isp.buffer[index_isp].virt.extP[1]);

            ALOGV("### Isp DQbuf done(%d) count (%d), SCP(%d) SCC(%d) shot_size(%d)",
                index,
                shot_ext->shot.ctl.request.frameCount,
                shot_ext->request_scp,
                shot_ext->request_scc,
                shot_ext->dis_bypass, sizeof(camera2_shot));

            if(isCapture) {
                    ALOGD("======= request_scc is 1");
                    memcpy(&m_jpegMetadata, &shot_ext->shot, sizeof(struct camera2_shot));
                    ALOGV("### Saving informationfor jpeg");
                    m_streamThreads[1]->SetSignal(SIGNAL_STREAM_DATA_COMING);

                for(j = 0; j < m_camera_info.isp.buffers; j++)
                {
                    shot_ext_capture = (struct camera2_shot_ext *)(m_camera_info.isp.buffer[j].virt.extP[1]);
                    shot_ext_capture->request_scc = 0;
                }

                isCapture = false;
            }

            if (shot_ext->request_scp) {
                m_previewOutput = 1;
                m_streamThreads[0]->SetSignal(SIGNAL_STREAM_DATA_COMING);
            }

            if (shot_ext->request_scc) {
                m_streamThreads[1]->SetSignal(SIGNAL_STREAM_DATA_COMING);
            }

            ALOGV("(%s): SCP_CLOSING check sensor(%d) scc(%d) scp(%d) ", __FUNCTION__,
               shot_ext->request_sensor, shot_ext->request_scc, shot_ext->request_scp);
            if (shot_ext->request_scc + shot_ext->request_scp + shot_ext->request_sensor == 0) {
                ALOGV("(%s): SCP_CLOSING check OK ", __FUNCTION__);
                m_scp_closed = true;
            }
            else
                m_scp_closed = false;

            m_requestManager->ApplyDynamicMetadata(shot_ext);
            OnAfNotification(shot_ext->shot.dm.aa.afState);
        }

        processingReqIndex = m_requestManager->MarkProcessingRequest(&(m_camera_info.sensor.buffer[index]), &afMode);
        if (processingReqIndex == -1)
        {
            ALOGE("DEBUG(%s) req underrun => inserting bubble to BayerIndex(%d)", __FUNCTION__, index);
        }
        else {
            SetAfMode((enum aa_afmode)afMode);
        }

        shot_ext = (struct camera2_shot_ext *)(m_camera_info.sensor.buffer[index].virt.extP[1]);
        if (m_scp_closing || m_scp_closed) {
            ALOGD("(%s): SCP_CLOSING(%d) SCP_CLOSED(%d)", __FUNCTION__, m_scp_closing, m_scp_closed);
            shot_ext->request_scc = 0;
            shot_ext->request_scp = 0;
            shot_ext->request_sensor = 0;
        }

//        ALOGD("### Sensor Qbuf start(%d) SCP(%d) SCC(%d) DIS(%d)", index, shot_ext->request_scp, shot_ext->request_scc, shot_ext->dis_bypass);

        cam_int_qbuf(&(m_camera_info.sensor), index);
        ALOGV("### Sensor QBUF done");

        if (!m_closing){
            selfThread->SetSignal(SIGNAL_SENSOR_START_REQ_PROCESSING);
        }
        return;
    }
    return;
}

void ExynosCameraHWInterface2::m_ispThreadInitialize(SignalDrivenThread * self)
{
    ALOGV("DEBUG(%s): ", __FUNCTION__ );
    /* will add */
    return;
}


void ExynosCameraHWInterface2::m_ispThreadFunc(SignalDrivenThread * self)
{
     ALOGV("DEBUG(%s): ", __FUNCTION__ );
    /* will add */
    return;
}

void ExynosCameraHWInterface2::m_streamThreadInitialize(SignalDrivenThread * self)
{
    StreamThread *          selfThread      = ((StreamThread*)self);
    ALOGV("DEBUG(%s): ", __FUNCTION__ );
    memset(&(selfThread->m_parameters), 0, sizeof(stream_parameters_t));
    selfThread->m_isBufferInit = false;

    return;
}

void ExynosCameraHWInterface2::m_streamThreadFunc(SignalDrivenThread * self)
{
    uint32_t                currentSignal   = self->GetProcessingSignal();
    StreamThread *          selfThread      = ((StreamThread*)self);
    stream_parameters_t     *selfStreamParms =  &(selfThread->m_parameters);
    record_parameters_t     *selfRecordParms =  &(selfThread->m_recordParameters);
    node_info_t             *currentNode    = &(selfStreamParms->node);

    ALOGV("DEBUG(%s): m_streamThreadFunc[%d] (%x)", __FUNCTION__, selfThread->m_index, currentSignal);

    if (currentSignal & SIGNAL_STREAM_CHANGE_PARAMETER) {
        ALOGV("DEBUG(%s): processing SIGNAL_STREAM_CHANGE_PARAMETER", __FUNCTION__);
        selfThread->applyChange();
        if (selfStreamParms->streamType == STREAM_TYPE_INDIRECT) {
            m_resizeBuf.size.extS[0] = ALIGN(selfStreamParms->outputWidth, 16) * ALIGN(selfStreamParms->outputHeight, 16) * 2;
            m_resizeBuf.size.extS[1] = 0;
            m_resizeBuf.size.extS[2] = 0;

            if (allocCameraMemory(selfStreamParms->ionClient, &m_resizeBuf, 1) == -1) {
                ALOGE("ERR(%s): Failed to allocate resize buf", __FUNCTION__);
            }
        }
        ALOGV("DEBUG(%s): processing SIGNAL_STREAM_CHANGE_PARAMETER DONE", __FUNCTION__);
    }

    if (currentSignal & SIGNAL_THREAD_RELEASE) {
        int i, index = -1, cnt_to_dq = 0;
        status_t res;
        ALOGV("DEBUG(%s): processing SIGNAL_THREAD_RELEASE", __FUNCTION__);
        ALOGD("(%s):(%d) SIGNAL_THREAD_RELEASE", __FUNCTION__, selfStreamParms->streamType);

        if (selfThread->m_isBufferInit) {
            for ( i=0 ; i < selfStreamParms->numSvcBuffers; i++) {
                ALOGV("DEBUG(%s): checking buffer index[%d] - status(%d)",
                    __FUNCTION__, i, selfStreamParms->svcBufStatus[i]);
                if (selfStreamParms->svcBufStatus[i] ==ON_DRIVER) cnt_to_dq++;
            }

            ALOGV("DEBUG(%s): calling stream(%d) streamoff (fd:%d)", __FUNCTION__,
            selfThread->m_index, selfStreamParms->fd);
            if (cam_int_streamoff(&(selfStreamParms->node)) < 0 ){
                ALOGE("ERR(%s): stream off fail", __FUNCTION__);
            } else {
                if (selfStreamParms->streamType == STREAM_TYPE_DIRECT) {
                    m_scp_closing = true;
                } else {
                    m_camera_info.capture.status = false;
                }
            }
            ALOGV("DEBUG(%s): calling stream(%d) streamoff done", __FUNCTION__, selfThread->m_index);
            ALOGV("DEBUG(%s): calling stream(%d) reqbuf 0 (fd:%d)", __FUNCTION__,
                    selfThread->m_index, selfStreamParms->fd);
            currentNode->buffers = 0;
            cam_int_reqbufs(currentNode);
            ALOGV("DEBUG(%s): calling stream(%d) reqbuf 0 DONE(fd:%d)", __FUNCTION__,
                    selfThread->m_index, selfStreamParms->fd);
        }
        if (selfThread->m_index == 1 && m_resizeBuf.size.s != 0) {
            freeCameraMemory(&m_resizeBuf, 1);
        }
        selfThread->m_isBufferInit = false;
        selfThread->m_index = 255;

        selfThread->m_releasing = false;

        ALOGV("DEBUG(%s): processing SIGNAL_THREAD_RELEASE DONE", __FUNCTION__);

        return;
    }

    if (currentSignal & SIGNAL_STREAM_DATA_COMING) {
        buffer_handle_t * buf = NULL;
        status_t res;
        void *virtAddr[3];
        int i, j;
        int index;
        nsecs_t timestamp;

        ALOGV("DEBUG(%s): stream(%d) processing SIGNAL_STREAM_DATA_COMING",
            __FUNCTION__,selfThread->m_index);

        if (selfStreamParms->streamType == STREAM_TYPE_INDIRECT)
        {
            ALOGD("stream(%s) processing SIGNAL_STREAM_DATA_COMING",
                __FUNCTION__,selfThread->m_index);
        }

        if (!(selfThread->m_isBufferInit)) {
            for ( i=0 ; i < selfStreamParms->numSvcBuffers; i++) {
                res = selfStreamParms->streamOps->dequeue_buffer(selfStreamParms->streamOps, &buf);
                if (res != NO_ERROR || buf == NULL) {
                    ALOGE("ERR(%s): Init: unable to dequeue buffer : %d",__FUNCTION__ , res);
                    return;
                }
                ALOGV("DEBUG(%s): got buf(%x) version(%d), numFds(%d), numInts(%d)", __FUNCTION__, (uint32_t)(*buf),
                   ((native_handle_t*)(*buf))->version, ((native_handle_t*)(*buf))->numFds, ((native_handle_t*)(*buf))->numInts);

                if (m_grallocHal->lock(m_grallocHal, *buf,
                           selfStreamParms->usage,
                           0, 0, selfStreamParms->outputWidth, selfStreamParms->outputHeight, virtAddr) != 0) {
                    ALOGE("ERR(%s): could not obtain gralloc buffer", __FUNCTION__);
                    return;
                }
                ALOGV("DEBUG(%s): locked img buf plane0(%x) plane1(%x) plane2(%x)",
                __FUNCTION__, (unsigned int)virtAddr[0], (unsigned int)virtAddr[1], (unsigned int)virtAddr[2]);

                index = selfThread->findBufferIndex(virtAddr[0]);
                if (index == -1) {
                    ALOGE("ERR(%s): could not find buffer index", __FUNCTION__);
                }
                else {
                    ALOGV("DEBUG(%s): found buffer index[%d] - status(%d)",
                        __FUNCTION__, index, selfStreamParms->svcBufStatus[index]);
                    if (selfStreamParms->svcBufStatus[index]== REQUIRES_DQ_FROM_SVC)
                        selfStreamParms->svcBufStatus[index] = ON_DRIVER;
                    else if (selfStreamParms->svcBufStatus[index]== ON_SERVICE)
                        selfStreamParms->svcBufStatus[index] = ON_HAL;
                    else {
                        ALOGV("DBG(%s): buffer status abnormal (%d) "
                            , __FUNCTION__, selfStreamParms->svcBufStatus[index]);
                    }
                    selfStreamParms->numSvcBufsInHal++;
                    if (*buf != selfStreamParms->svcBufHandle[index])
                        ALOGV("DBG(%s): different buf_handle index ", __FUNCTION__);
                    else
                        ALOGV("DEBUG(%s): same buf_handle index", __FUNCTION__);
                }
                selfStreamParms->svcBufIndex = 0;
            }
            selfThread->m_isBufferInit = true;
        }

        if (m_recordingEnabled && m_needsRecordBufferInit) {
            ALOGV("DEBUG(%s): Recording Buffer Initialization numsvcbuf(%d)",
                __FUNCTION__, selfRecordParms->numSvcBuffers);
            int checkingIndex = 0;
            bool found = false;
            for ( i=0 ; i < selfRecordParms->numSvcBuffers; i++) {
                res = selfRecordParms->streamOps->dequeue_buffer(selfRecordParms->streamOps, &buf);
                if (res != NO_ERROR || buf == NULL) {
                    ALOGE("ERR(%s): Init: unable to dequeue buffer : %d",__FUNCTION__ , res);
                    return;
                }
                selfRecordParms->numSvcBufsInHal++;
                ALOGV("DEBUG(%s): [record] got buf(%x) bufInHal(%d) version(%d), numFds(%d), numInts(%d)", __FUNCTION__, (uint32_t)(*buf),
                   selfRecordParms->numSvcBufsInHal, ((native_handle_t*)(*buf))->version, ((native_handle_t*)(*buf))->numFds, ((native_handle_t*)(*buf))->numInts);

                if (m_grallocHal->lock(m_grallocHal, *buf,
                       selfRecordParms->usage, 0, 0,
                       selfRecordParms->outputWidth, selfRecordParms->outputHeight, virtAddr) != 0) {
                    ALOGE("ERR(%s): could not obtain gralloc buffer", __FUNCTION__);
                }
                else {
                      ALOGV("DEBUG(%s): [record] locked img buf plane0(%x) plane1(%x) plane2(%x)",
                        __FUNCTION__, (unsigned int)virtAddr[0], (unsigned int)virtAddr[1], (unsigned int)virtAddr[2]);
                }
                found = false;
                for (checkingIndex = 0; checkingIndex < selfRecordParms->numSvcBuffers ; checkingIndex++) {
                    if (selfRecordParms->svcBufHandle[checkingIndex] == *buf ) {
                        found = true;
                        break;
                    }
                }
                ALOGV("DEBUG(%s): [record] found(%d) - index[%d]", __FUNCTION__, found, checkingIndex);
                if (!found) break;

                index = checkingIndex;

                if (index == -1) {
                    ALOGV("ERR(%s): could not find buffer index", __FUNCTION__);
                }
                else {
                    ALOGV("DEBUG(%s): found buffer index[%d] - status(%d)",
                        __FUNCTION__, index, selfRecordParms->svcBufStatus[index]);
                    if (selfRecordParms->svcBufStatus[index]== ON_SERVICE)
                        selfRecordParms->svcBufStatus[index] = ON_HAL;
                    else {
                        ALOGV("DBG(%s): buffer status abnormal (%d) "
                            , __FUNCTION__, selfRecordParms->svcBufStatus[index]);
                    }
                    if (*buf != selfRecordParms->svcBufHandle[index])
                        ALOGV("DBG(%s): different buf_handle index ", __FUNCTION__);
                    else
                        ALOGV("DEBUG(%s): same buf_handle index", __FUNCTION__);
                }
                selfRecordParms->svcBufIndex = 0;
            }
            m_needsRecordBufferInit = false;
        }

        do {
            if (selfStreamParms->streamType == STREAM_TYPE_DIRECT) {
                ALOGV("DEBUG(%s): stream(%d) type(%d) DQBUF START ",__FUNCTION__,
                    selfThread->m_index, selfStreamParms->streamType);

                index = cam_int_dqbuf(&(selfStreamParms->node));
                ALOGV("DEBUG(%s): stream(%d) type(%d) DQBUF done index(%d)",__FUNCTION__,
                    selfThread->m_index, selfStreamParms->streamType, index);


                if (selfStreamParms->svcBufStatus[index] !=  ON_DRIVER)
                    ALOGV("DBG(%s): DQed buffer status abnormal (%d) ",
                           __FUNCTION__, selfStreamParms->svcBufStatus[index]);
                selfStreamParms->svcBufStatus[index] = ON_HAL;

                if (m_recordOutput && m_recordingEnabled) {
                    ALOGV("DEBUG(%s): Entering record frame creator, index(%d)",__FUNCTION__, selfRecordParms->svcBufIndex);
                    bool found = false;
                    for (int i = 0 ; selfRecordParms->numSvcBuffers ; i++) {
                        if (selfRecordParms->svcBufStatus[selfRecordParms->svcBufIndex] == ON_HAL) {
                            found = true;
                            break;
                        }
                        selfRecordParms->svcBufIndex++;
                        if (selfRecordParms->svcBufIndex >= selfRecordParms->numSvcBuffers)
                            selfRecordParms->svcBufIndex = 0;
                    }
                    if (!found) {
                        ALOGE("(%s): cannot find free recording buffer", __FUNCTION__);
                        selfRecordParms->svcBufIndex++;
                        break;
                    }

                    if (m_exynosVideoCSC) {
                        int videoW = selfRecordParms->outputWidth, videoH = selfRecordParms->outputHeight;
                        int cropX, cropY, cropW, cropH = 0;
                        int previewW = selfStreamParms->outputWidth, previewH = selfStreamParms->outputHeight;
                        m_getRatioSize(previewW, previewH,
                                       videoW, videoH,
                                       &cropX, &cropY,
                                       &cropW, &cropH,
                                       0);

                        ALOGV("DEBUG(%s):cropX = %d, cropY = %d, cropW = %d, cropH = %d",
                                 __FUNCTION__, cropX, cropY, cropW, cropH);

                        csc_set_src_format(m_exynosVideoCSC,
                                           previewW, previewH,
                                           cropX, cropY, cropW, cropH,
                                           HAL_PIXEL_FORMAT_EXYNOS_YV12,
                                           0);

                        csc_set_dst_format(m_exynosVideoCSC,
                                           videoW, videoH,
                                           0, 0, videoW, videoH,
                                           selfRecordParms->outputFormat,
                                           1);

                        csc_set_src_buffer(m_exynosVideoCSC,
                                       (void **)(&(selfStreamParms->svcBuffers[index].fd.fd)));

                        csc_set_dst_buffer(m_exynosVideoCSC,
                            (void **)(&(selfRecordParms->svcBuffers[selfRecordParms->svcBufIndex].fd.fd)));

                        if (csc_convert(m_exynosVideoCSC) != 0) {
                            ALOGE("ERR(%s):csc_convert() fail", __FUNCTION__);
                        }
                        else {
                            ALOGV("(%s):csc_convert() SUCCESS", __FUNCTION__);
                        }
                    }
                    else {
                        ALOGE("ERR(%s):m_exynosVideoCSC == NULL", __FUNCTION__);
                    }

                    res = selfRecordParms->streamOps->enqueue_buffer(selfRecordParms->streamOps,
                            systemTime(),
                            &(selfRecordParms->svcBufHandle[selfRecordParms->svcBufIndex]));
                    ALOGV("DEBUG(%s): stream(%d) record enqueue_buffer to svc done res(%d)", __FUNCTION__,
                        selfThread->m_index, res);
                    if (res == 0) {
                        selfRecordParms->svcBufStatus[selfRecordParms->svcBufIndex] = ON_SERVICE;
                        selfRecordParms->numSvcBufsInHal--;
                    }
                }
                if (m_previewOutput && m_requestManager->GetSkipCnt() <= 0) {

                    ALOGV("** Display Preview(frameCnt:%d)", m_requestManager->GetFrameIndex());
                    res = selfStreamParms->streamOps->enqueue_buffer(selfStreamParms->streamOps,
                            m_requestManager->GetTimestamp(m_requestManager->GetFrameIndex()),
                            &(selfStreamParms->svcBufHandle[index]));

                    ALOGV("DEBUG(%s): stream(%d) enqueue_buffer to svc done res(%d)", __FUNCTION__, selfThread->m_index, res);
                }
                else {
                    res = selfStreamParms->streamOps->cancel_buffer(selfStreamParms->streamOps,
                            &(selfStreamParms->svcBufHandle[index]));
                    ALOGV("DEBUG(%s): stream(%d) cancel_buffer to svc done res(%d)", __FUNCTION__, selfThread->m_index, res);
                }
                if (res == 0) {
                    selfStreamParms->svcBufStatus[index] = ON_SERVICE;
                    selfStreamParms->numSvcBufsInHal--;
                }
                else {
                    selfStreamParms->svcBufStatus[index] = ON_HAL;
                }
            }
            else if (selfStreamParms->streamType == STREAM_TYPE_INDIRECT) {
                ExynosRect jpegRect;
                bool found = false;
                bool ret = false;
                int pictureW, pictureH, pictureFramesize = 0;
                int pictureFormat;
                int cropX, cropY, cropW, cropH = 0;
                ExynosBuffer resizeBufInfo;
                ExynosRect   m_orgPictureRect;

                ALOGD("DEBUG(%s): stream(%d) type(%d) DQBUF START ",__FUNCTION__,
                    selfThread->m_index, selfStreamParms->streamType);
                index = cam_int_dqbuf(&(selfStreamParms->node));
                ALOGD("DEBUG(%s): stream(%d) type(%d) DQBUF done index(%d)",__FUNCTION__,
                    selfThread->m_index, selfStreamParms->streamType, index);


                for (int i = 0; i < selfStreamParms->numSvcBuffers ; i++) {
                    if (selfStreamParms->svcBufStatus[selfStreamParms->svcBufIndex] == ON_HAL) {
                        found = true;
                        break;
                    }
                    selfStreamParms->svcBufIndex++;
                    if (selfStreamParms->svcBufIndex >= selfStreamParms->numSvcBuffers)
                        selfStreamParms->svcBufIndex = 0;
                }
                if (!found) {
                    ALOGE("ERR(%s): NO free SVC buffer for JPEG", __FUNCTION__);
                    break;
                }

                m_orgPictureRect.w = selfStreamParms->outputWidth;
                m_orgPictureRect.h = selfStreamParms->outputHeight;

                ExynosBuffer* m_pictureBuf = &(m_camera_info.capture.buffer[index]);

                pictureW = selfStreamParms->nodeWidth;
                pictureH = selfStreamParms->nodeHeight;
                pictureFormat = V4L2_PIX_FMT_YUYV;
                pictureFramesize = FRAME_SIZE(V4L2_PIX_2_HAL_PIXEL_FORMAT(pictureFormat), pictureW, pictureH);

                if (m_exynosPictureCSC) {
                    m_getRatioSize(pictureW, pictureH,
                                   m_orgPictureRect.w, m_orgPictureRect.h,
                                   &cropX, &cropY,
                                   &cropW, &cropH,
                                   0);

                    ALOGV("DEBUG(%s):cropX = %d, cropY = %d, cropW = %d, cropH = %d",
                          __FUNCTION__, cropX, cropY, cropW, cropH);

                    csc_set_src_format(m_exynosPictureCSC,
                                       ALIGN(pictureW, 16), ALIGN(pictureH, 16),
                                       cropX, cropY, cropW, cropH,
                                       V4L2_PIX_2_HAL_PIXEL_FORMAT(pictureFormat),
                                       0);

                    csc_set_dst_format(m_exynosPictureCSC,
                                       m_orgPictureRect.w, m_orgPictureRect.h,
                                       0, 0, m_orgPictureRect.w, m_orgPictureRect.h,
                                       V4L2_PIX_2_HAL_PIXEL_FORMAT(V4L2_PIX_FMT_NV16),
                                       0);
                    csc_set_src_buffer(m_exynosPictureCSC,
                                       (void **)&m_pictureBuf->fd.fd);

                    csc_set_dst_buffer(m_exynosPictureCSC,
                                       (void **)&m_resizeBuf.fd.fd);
                    for (int i = 0 ; i < 3 ; i++)
                        ALOGV("DEBUG(%s): m_resizeBuf.virt.extP[%d]=%d m_resizeBuf.size.extS[%d]=%d",
                            __FUNCTION__, i, m_resizeBuf.fd.extFd[i], i, m_resizeBuf.size.extS[i]);

                    if (csc_convert(m_exynosPictureCSC) != 0)
                        ALOGE("ERR(%s): csc_convert() fail", __FUNCTION__);


                }
                else {
                    ALOGE("ERR(%s): m_exynosPictureCSC == NULL", __FUNCTION__);
                }

                resizeBufInfo = m_resizeBuf;

                m_getAlignedYUVSize(V4L2_PIX_FMT_NV16, m_orgPictureRect.w, m_orgPictureRect.h, &m_resizeBuf);

                for (int i = 1; i < 3; i++) {
                    if (m_resizeBuf.size.extS[i] != 0)
                        m_resizeBuf.fd.extFd[i] = m_resizeBuf.fd.extFd[i-1] + m_resizeBuf.size.extS[i-1];

                    ALOGV("(%s): m_resizeBuf.size.extS[%d] = %d", __FUNCTION__, i, m_resizeBuf.size.extS[i]);
                }

                jpegRect.w = m_orgPictureRect.w;
                jpegRect.h = m_orgPictureRect.h;
                jpegRect.colorFormat = V4L2_PIX_FMT_NV16;

                if (yuv2Jpeg(&m_resizeBuf, &selfStreamParms->svcBuffers[selfStreamParms->svcBufIndex], &jpegRect) == false)
                    ALOGE("ERR(%s):yuv2Jpeg() fail", __FUNCTION__);
                cam_int_qbuf(&(selfStreamParms->node), index);
                ALOGV("DEBUG(%s): stream(%d) type(%d) QBUF DONE ",__FUNCTION__,
                    selfThread->m_index, selfStreamParms->streamType);

                m_resizeBuf = resizeBufInfo;

                res = selfStreamParms->streamOps->enqueue_buffer(selfStreamParms->streamOps, systemTime(), &(selfStreamParms->svcBufHandle[selfStreamParms->svcBufIndex]));

                ALOGV("DEBUG(%s): stream(%d) enqueue_buffer index(%d) to svc done res(%d)",
                        __FUNCTION__, selfThread->m_index, selfStreamParms->svcBufIndex, res);
                if (res == 0) {
                    selfStreamParms->svcBufStatus[selfStreamParms->svcBufIndex] = ON_SERVICE;
                    selfStreamParms->numSvcBufsInHal--;
                }
                else {
                    selfStreamParms->svcBufStatus[selfStreamParms->svcBufIndex] = ON_HAL;
                }
            }
        }
        while (0);

        if (selfStreamParms->streamType == STREAM_TYPE_DIRECT  && m_recordOutput && m_recordingEnabled) {
            do {
                ALOGV("DEBUG(%s): record currentBuf#(%d)", __FUNCTION__ , selfRecordParms->numSvcBufsInHal);
                if (selfRecordParms->numSvcBufsInHal >= 1)
                {
                    ALOGV("DEBUG(%s): breaking", __FUNCTION__);
                    break;
                }
                res = selfRecordParms->streamOps->dequeue_buffer(selfRecordParms->streamOps, &buf);
                if (res != NO_ERROR || buf == NULL) {
                    ALOGV("DEBUG(%s): record stream(%d) dequeue_buffer fail res(%d)",__FUNCTION__ , selfThread->m_index,  res);
                    break;
                }
                selfRecordParms->numSvcBufsInHal ++;
                ALOGV("DEBUG(%s): record got buf(%x) numBufInHal(%d) version(%d), numFds(%d), numInts(%d)", __FUNCTION__, (uint32_t)(*buf),
                   selfRecordParms->numSvcBufsInHal, ((native_handle_t*)(*buf))->version, ((native_handle_t*)(*buf))->numFds, ((native_handle_t*)(*buf))->numInts);

                const private_handle_t *priv_handle = reinterpret_cast<const private_handle_t *>(*buf);
                bool found = false;
                int checkingIndex = 0;
                for (checkingIndex = 0; checkingIndex < selfRecordParms->numSvcBuffers ; checkingIndex++) {
                    if (priv_handle->fd == selfRecordParms->svcBuffers[checkingIndex].fd.extFd[0] ) {
                        found = true;
                        break;
                    }
                }
                ALOGV("DEBUG(%s): recording dequeueed_buffer found index(%d)", __FUNCTION__, found);

                if (!found) {
                     break;
                }

                index = checkingIndex;
                if (selfRecordParms->svcBufStatus[index] == ON_SERVICE) {
                    selfRecordParms->svcBufStatus[index] = ON_HAL;
                }
                else {
                    ALOGV("DEBUG(%s): record bufstatus abnormal [%d]  status = %d", __FUNCTION__,
                        index,  selfRecordParms->svcBufStatus[index]);
                }
            } while (0);
        }
        if (selfStreamParms->streamType == STREAM_TYPE_DIRECT) {
            while (selfStreamParms->numSvcBufsInHal < selfStreamParms->numOwnSvcBuffers) {
                res = selfStreamParms->streamOps->dequeue_buffer(selfStreamParms->streamOps, &buf);
                if (res != NO_ERROR || buf == NULL) {
                    ALOGV("DEBUG(%s): stream(%d) dequeue_buffer fail res(%d)",__FUNCTION__ , selfThread->m_index,  res);
                    break;
                }
                selfStreamParms->numSvcBufsInHal++;
                ALOGV("DEBUG(%s): stream(%d) got buf(%x) numInHal(%d) version(%d), numFds(%d), numInts(%d)", __FUNCTION__,
                    selfThread->m_index, (uint32_t)(*buf), selfStreamParms->numSvcBufsInHal,
                   ((native_handle_t*)(*buf))->version, ((native_handle_t*)(*buf))->numFds, ((native_handle_t*)(*buf))->numInts);
                const private_handle_t *priv_handle = reinterpret_cast<const private_handle_t *>(*buf);

                bool found = false;
                int checkingIndex = 0;
                for (checkingIndex = 0; checkingIndex < selfStreamParms->numSvcBuffers ; checkingIndex++) {
                    if (priv_handle->fd == selfStreamParms->svcBuffers[checkingIndex].fd.extFd[0] ) {
                        found = true;
                        break;
                    }
                }
                ALOGV("DEBUG(%s): post_dequeue_buffer found(%d)", __FUNCTION__, found);
                if (!found) break;
                ALOGV("DEBUG(%s): preparing to qbuf [%d]", __FUNCTION__, checkingIndex);
                index = checkingIndex;
                if (index < selfStreamParms->numHwBuffers) {
                    uint32_t    plane_index = 0;
                    ExynosBuffer*  currentBuf = &(selfStreamParms->svcBuffers[index]);
                    struct v4l2_buffer v4l2_buf;
                    struct v4l2_plane  planes[VIDEO_MAX_PLANES];

                    v4l2_buf.m.planes   = planes;
                    v4l2_buf.type       = currentNode->type;
                    v4l2_buf.memory     = currentNode->memory;
                    v4l2_buf.index      = index;
                    v4l2_buf.length     = currentNode->planes;

                    v4l2_buf.m.planes[0].m.fd = priv_handle->fd;
                    v4l2_buf.m.planes[2].m.fd = priv_handle->fd1;
                    v4l2_buf.m.planes[1].m.fd = priv_handle->fd2;
                    for (plane_index=0 ; plane_index < v4l2_buf.length ; plane_index++) {
                        v4l2_buf.m.planes[plane_index].length  = currentBuf->size.extS[plane_index];
                        ALOGV("DEBUG(%s): plane(%d): fd(%d)  length(%d)",
                             __FUNCTION__, plane_index, v4l2_buf.m.planes[plane_index].m.fd,
                             v4l2_buf.m.planes[plane_index].length);
                    }
                    if (exynos_v4l2_qbuf(currentNode->fd, &v4l2_buf) < 0) {
                        ALOGE("ERR(%s): stream id(%d) exynos_v4l2_qbuf() fail",
                            __FUNCTION__, selfThread->m_index);
                        return;
                    }
                    selfStreamParms->svcBufStatus[index] = ON_DRIVER;
                    ALOGV("DEBUG(%s): stream id(%d) type0 QBUF done index(%d)",
                        __FUNCTION__, selfThread->m_index, index);
                }
            }
        }
        else if (selfStreamParms->streamType == STREAM_TYPE_INDIRECT) {
            while (selfStreamParms->numSvcBufsInHal < selfStreamParms->numOwnSvcBuffers) {
                res = selfStreamParms->streamOps->dequeue_buffer(selfStreamParms->streamOps, &buf);
                if (res != NO_ERROR || buf == NULL) {
                    ALOGV("DEBUG(%s): stream(%d) dequeue_buffer fail res(%d)",__FUNCTION__ , selfThread->m_index,  res);
                    break;
                }

                ALOGV("DEBUG(%s): stream(%d) got buf(%x) numInHal(%d) version(%d), numFds(%d), numInts(%d)", __FUNCTION__,
                    selfThread->m_index, (uint32_t)(*buf), selfStreamParms->numSvcBufsInHal,
                   ((native_handle_t*)(*buf))->version, ((native_handle_t*)(*buf))->numFds, ((native_handle_t*)(*buf))->numInts);

                const private_handle_t *priv_handle = reinterpret_cast<const private_handle_t *>(*buf);

                bool found = false;
                int checkingIndex = 0;
                for (checkingIndex = 0; checkingIndex < selfStreamParms->numSvcBuffers ; checkingIndex++) {
                    if (priv_handle->fd == selfStreamParms->svcBuffers[checkingIndex].fd.extFd[0] ) {
                        found = true;
                        break;
                    }
                }
                if (!found) break;
                selfStreamParms->svcBufStatus[checkingIndex] = ON_HAL;
                selfStreamParms->numSvcBufsInHal++;
            }

        }
        ALOGV("DEBUG(%s): stream(%d) processing SIGNAL_STREAM_DATA_COMING DONE",
            __FUNCTION__,selfThread->m_index);
    }
    return;
}

bool ExynosCameraHWInterface2::yuv2Jpeg(ExynosBuffer *yuvBuf,
                            ExynosBuffer *jpegBuf,
                            ExynosRect *rect)
{
    unsigned char *addr;

    int thumbW = 320;
    int thumbH = 240;

    ExynosJpegEncoderForCamera jpegEnc;
    bool ret = false;
    int res = 0;

    unsigned int *yuvSize = yuvBuf->size.extS;

    if (jpegEnc.create()) {
        ALOGE("ERR(%s):jpegEnc.create() fail", __FUNCTION__);
        goto jpeg_encode_done;
    }

    if (jpegEnc.setQuality(100)) {
        ALOGE("ERR(%s):jpegEnc.setQuality() fail", __FUNCTION__);
        goto jpeg_encode_done;
    }

    if (jpegEnc.setSize(rect->w, rect->h)) {
        ALOGE("ERR(%s):jpegEnc.setSize() fail", __FUNCTION__);
        goto jpeg_encode_done;
    }
    ALOGV("%s : width = %d , height = %d\n", __FUNCTION__, rect->w, rect->h);

    if (jpegEnc.setColorFormat(rect->colorFormat)) {
        ALOGE("ERR(%s):jpegEnc.setColorFormat() fail", __FUNCTION__);
        goto jpeg_encode_done;
    }

    if (jpegEnc.setJpegFormat(V4L2_PIX_FMT_JPEG_422)) {
        ALOGE("ERR(%s):jpegEnc.setJpegFormat() fail", __FUNCTION__);
        goto jpeg_encode_done;
    }

    mExifInfo.enableThumb = true;

    if (jpegEnc.setThumbnailSize(thumbW, thumbH)) {
        ALOGE("ERR(%s):jpegEnc.setThumbnailSize(%d, %d) fail", __FUNCTION__, thumbW, thumbH);
        goto jpeg_encode_done;
    }

    ALOGV("(%s):jpegEnc.setThumbnailSize(%d, %d) ", __FUNCTION__, thumbW, thumbH);
    if (jpegEnc.setThumbnailQuality(50)) {
        ALOGE("ERR(%s):jpegEnc.setThumbnailQuality fail", __FUNCTION__);
        goto jpeg_encode_done;
    }

    m_setExifChangedAttribute(&mExifInfo, rect, &m_jpegMetadata);
    ALOGV("DEBUG(%s):calling jpegEnc.setInBuf() yuvSize(%d)", __FUNCTION__, *yuvSize);
    if (jpegEnc.setInBuf((int *)&(yuvBuf->fd.fd), &(yuvBuf->virt.p), (int *)yuvSize)) {
        ALOGE("ERR(%s):jpegEnc.setInBuf() fail", __FUNCTION__);
        goto jpeg_encode_done;
    }
    if (jpegEnc.setOutBuf(jpegBuf->fd.fd, jpegBuf->virt.p, jpegBuf->size.extS[0] + jpegBuf->size.extS[1] + jpegBuf->size.extS[2])) {
        ALOGE("ERR(%s):jpegEnc.setOutBuf() fail", __FUNCTION__);
        goto jpeg_encode_done;
    }

    if (jpegEnc.updateConfig()) {
        ALOGE("ERR(%s):jpegEnc.updateConfig() fail", __FUNCTION__);
        goto jpeg_encode_done;
    }

    if (res = jpegEnc.encode((int *)&jpegBuf->size.s, &mExifInfo)) {
        ALOGE("ERR(%s):jpegEnc.encode() fail ret(%d)", __FUNCTION__, res);
        goto jpeg_encode_done;
    }

    ret = true;

jpeg_encode_done:

    if (jpegEnc.flagCreate() == true)
        jpegEnc.destroy();

    return ret;
}


void ExynosCameraHWInterface2::OnAfTrigger(int id)
{
    switch (m_afMode) {
    case AA_AFMODE_AUTO:
    case AA_AFMODE_MACRO:
        OnAfTriggerAutoMacro(id);
        break;
    case AA_AFMODE_CONTINUOUS_VIDEO:
        OnAfTriggerCAFVideo(id);
        break;
    case AA_AFMODE_CONTINUOUS_PICTURE:
        OnAfTriggerCAFPicture(id);
        break;
    case AA_AFMODE_OFF:
    default:
        break;
    }
}

void ExynosCameraHWInterface2::OnAfTriggerAutoMacro(int id)
{
    int nextState = NO_TRANSITION;
    m_afTriggerId = id;

    switch (m_afState) {
    case HAL_AFSTATE_INACTIVE:
        nextState = HAL_AFSTATE_NEEDS_COMMAND;
        m_IsAfTriggerRequired = true;
        break;
    case HAL_AFSTATE_NEEDS_COMMAND:
        nextState = NO_TRANSITION;
        break;
    case HAL_AFSTATE_STARTED:
        nextState = NO_TRANSITION;
        break;
    case HAL_AFSTATE_SCANNING:
        nextState = NO_TRANSITION;
        break;
    case HAL_AFSTATE_LOCKED:
        nextState = HAL_AFSTATE_NEEDS_COMMAND;
        m_IsAfTriggerRequired = true;
        break;
    case HAL_AFSTATE_FAILED:
        nextState = HAL_AFSTATE_NEEDS_COMMAND;
        m_IsAfTriggerRequired = true;
        break;
    default:
        break;
    }
    ALOGV("(%s): State (%d) -> (%d)", __FUNCTION__, m_afState, nextState);
    if (nextState != NO_TRANSITION)
        m_afState = nextState;
}

void ExynosCameraHWInterface2::OnAfTriggerCAFPicture(int id)
{
    int nextState = NO_TRANSITION;
    m_afTriggerId = id;

    switch (m_afState) {
    case HAL_AFSTATE_INACTIVE:
        nextState = HAL_AFSTATE_FAILED;
        SetAfStateForService(ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED);
        break;
    case HAL_AFSTATE_NEEDS_COMMAND:
        // not used
        break;
    case HAL_AFSTATE_STARTED:
        nextState = HAL_AFSTATE_NEEDS_DETERMINATION;
        m_AfHwStateFailed = false;
        break;
    case HAL_AFSTATE_SCANNING:
        nextState = HAL_AFSTATE_NEEDS_DETERMINATION;
        m_AfHwStateFailed = false;
        break;
    case HAL_AFSTATE_NEEDS_DETERMINATION:
        nextState = NO_TRANSITION;
        break;
    case HAL_AFSTATE_PASSIVE_FOCUSED:
        m_IsAfLockRequired = true;
        if (m_AfHwStateFailed) {
            ALOGV("(%s): LAST : fail", __FUNCTION__);
            SetAfStateForService(ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED);
            nextState = HAL_AFSTATE_FAILED;
        }
        else {
            ALOGV("(%s): LAST : success", __FUNCTION__);
            SetAfStateForService(ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED);
            nextState = HAL_AFSTATE_LOCKED;
        }
        m_AfHwStateFailed = false;
        break;
    case HAL_AFSTATE_LOCKED:
        nextState = NO_TRANSITION;
        break;
    case HAL_AFSTATE_FAILED:
        nextState = NO_TRANSITION;
        break;
    default:
        break;
    }
    ALOGV("(%s): State (%d) -> (%d)", __FUNCTION__, m_afState, nextState);
    if (nextState != NO_TRANSITION)
        m_afState = nextState;
}


void ExynosCameraHWInterface2::OnAfTriggerCAFVideo(int id)
{
    int nextState = NO_TRANSITION;
    m_afTriggerId = id;

    switch (m_afState) {
    case HAL_AFSTATE_INACTIVE:
        nextState = HAL_AFSTATE_FAILED;
        SetAfStateForService(ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED);
        break;
    case HAL_AFSTATE_NEEDS_COMMAND:
        // not used
        break;
    case HAL_AFSTATE_STARTED:
        m_IsAfLockRequired = true;
        nextState = HAL_AFSTATE_FAILED;
        SetAfStateForService(ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED);
        break;
    case HAL_AFSTATE_SCANNING:
        m_IsAfLockRequired = true;
        nextState = HAL_AFSTATE_FAILED;
        SetAfStateForService(ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED);
        break;
    case HAL_AFSTATE_NEEDS_DETERMINATION:
        // not used
        break;
    case HAL_AFSTATE_PASSIVE_FOCUSED:
        m_IsAfLockRequired = true;
        SetAfStateForService(ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED);
        nextState = HAL_AFSTATE_LOCKED;
        break;
    case HAL_AFSTATE_LOCKED:
        nextState = NO_TRANSITION;
        break;
    case HAL_AFSTATE_FAILED:
        nextState = NO_TRANSITION;
        break;
    default:
        break;
    }
    ALOGV("(%s): State (%d) -> (%d)", __FUNCTION__, m_afState, nextState);
    if (nextState != NO_TRANSITION)
        m_afState = nextState;
}

void ExynosCameraHWInterface2::OnAfNotification(enum aa_afstate noti)
{
    switch (m_afMode) {
    case AA_AFMODE_AUTO:
    case AA_AFMODE_MACRO:
        OnAfNotificationAutoMacro(noti);
        break;
    case AA_AFMODE_CONTINUOUS_VIDEO:
        OnAfNotificationCAFVideo(noti);
        break;
    case AA_AFMODE_CONTINUOUS_PICTURE:
        OnAfNotificationCAFPicture(noti);
        break;
    case AA_AFMODE_OFF:
    default:
        break;
    }
}

void ExynosCameraHWInterface2::OnAfNotificationAutoMacro(enum aa_afstate noti)
{
    int nextState = NO_TRANSITION;
    bool bWrongTransition = false;

    if (m_afState == HAL_AFSTATE_INACTIVE || m_afState == HAL_AFSTATE_NEEDS_COMMAND) {
        switch (noti) {
        case AA_AFSTATE_INACTIVE:
        case AA_AFSTATE_ACTIVE_SCAN:
        case AA_AFSTATE_AF_ACQUIRED_FOCUS:
        case AA_AFSTATE_AF_FAILED_FOCUS:
        default:
            nextState = NO_TRANSITION;
            break;
        }
    }
    else if (m_afState == HAL_AFSTATE_STARTED) {
        switch (noti) {
        case AA_AFSTATE_INACTIVE:
            nextState = NO_TRANSITION;
            break;
        case AA_AFSTATE_ACTIVE_SCAN:
            nextState = HAL_AFSTATE_SCANNING;
            SetAfStateForService(ANDROID_CONTROL_AF_STATE_ACTIVE_SCAN);
            break;
        case AA_AFSTATE_AF_ACQUIRED_FOCUS:
            nextState = NO_TRANSITION;
            break;
        case AA_AFSTATE_AF_FAILED_FOCUS:
            nextState = NO_TRANSITION;
            break;
        default:
            bWrongTransition = true;
            break;
        }
    }
    else if (m_afState == HAL_AFSTATE_SCANNING) {
        switch (noti) {
        case AA_AFSTATE_INACTIVE:
            bWrongTransition = true;
            break;
        case AA_AFSTATE_ACTIVE_SCAN:
            nextState = NO_TRANSITION;
            break;
        case AA_AFSTATE_AF_ACQUIRED_FOCUS:
            nextState = HAL_AFSTATE_LOCKED;
            SetAfStateForService(ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED);
            break;
        case AA_AFSTATE_AF_FAILED_FOCUS:
            nextState = HAL_AFSTATE_FAILED;
            SetAfStateForService(ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED);
            break;
        default:
            bWrongTransition = true;
            break;
        }
    }
    else if (m_afState == HAL_AFSTATE_LOCKED) {
        switch (noti) {
            case AA_AFSTATE_INACTIVE:
            case AA_AFSTATE_ACTIVE_SCAN:
                bWrongTransition = true;
                break;
            case AA_AFSTATE_AF_ACQUIRED_FOCUS:
                nextState = NO_TRANSITION;
                break;
            case AA_AFSTATE_AF_FAILED_FOCUS:
            default:
                bWrongTransition = true;
                break;
        }
    }
    else if (m_afState == HAL_AFSTATE_FAILED) {
        switch (noti) {
            case AA_AFSTATE_INACTIVE:
            case AA_AFSTATE_ACTIVE_SCAN:
            case AA_AFSTATE_AF_ACQUIRED_FOCUS:
                bWrongTransition = true;
                break;
            case AA_AFSTATE_AF_FAILED_FOCUS:
                nextState = NO_TRANSITION;
                break;
            default:
                bWrongTransition = true;
                break;
        }
    }
    if (bWrongTransition) {
        ALOGV("(%s): Wrong Transition state(%d) noti(%d)", __FUNCTION__, m_afState, noti);
        return;
    }
    ALOGV("(%s): State (%d) -> (%d) by (%d)", __FUNCTION__, m_afState, nextState, noti);
    if (nextState != NO_TRANSITION)
        m_afState = nextState;
}

void ExynosCameraHWInterface2::OnAfNotificationCAFPicture(enum aa_afstate noti)
{
    int nextState = NO_TRANSITION;
    bool bWrongTransition = false;

    if (m_afState == HAL_AFSTATE_INACTIVE) {
        switch (noti) {
        case AA_AFSTATE_INACTIVE:
        case AA_AFSTATE_ACTIVE_SCAN:
        case AA_AFSTATE_AF_ACQUIRED_FOCUS:
        case AA_AFSTATE_AF_FAILED_FOCUS:
        default:
            nextState = NO_TRANSITION;
            break;
        }
    }
    else if (m_afState == HAL_AFSTATE_STARTED) {
        switch (noti) {
        case AA_AFSTATE_INACTIVE:
            nextState = NO_TRANSITION;
            break;
        case AA_AFSTATE_ACTIVE_SCAN:
            nextState = HAL_AFSTATE_SCANNING;
            SetAfStateForService(ANDROID_CONTROL_AF_STATE_PASSIVE_SCAN);
            break;
        case AA_AFSTATE_AF_ACQUIRED_FOCUS:
            nextState = HAL_AFSTATE_PASSIVE_FOCUSED;
            SetAfStateForService(ANDROID_CONTROL_AF_STATE_PASSIVE_FOCUSED);
            break;
        case AA_AFSTATE_AF_FAILED_FOCUS:
            nextState = HAL_AFSTATE_FAILED;
            SetAfStateForService(ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED);
            break;
        default:
            bWrongTransition = true;
            break;
        }
    }
    else if (m_afState == HAL_AFSTATE_SCANNING) {
        switch (noti) {
        case AA_AFSTATE_INACTIVE:
            nextState = NO_TRANSITION;
            break;
        case AA_AFSTATE_ACTIVE_SCAN:
            nextState = NO_TRANSITION;
            m_AfHwStateFailed = false;
            break;
        case AA_AFSTATE_AF_ACQUIRED_FOCUS:
            nextState = HAL_AFSTATE_PASSIVE_FOCUSED;
            m_AfHwStateFailed = false;
            SetAfStateForService(ANDROID_CONTROL_AF_STATE_PASSIVE_FOCUSED);
            break;
        case AA_AFSTATE_AF_FAILED_FOCUS:
            nextState = HAL_AFSTATE_PASSIVE_FOCUSED;
            m_AfHwStateFailed = true;
            SetAfStateForService(ANDROID_CONTROL_AF_STATE_PASSIVE_FOCUSED);
            break;
        default:
            bWrongTransition = true;
            break;
        }
    }
    else if (m_afState == HAL_AFSTATE_PASSIVE_FOCUSED) {
        switch (noti) {
        case AA_AFSTATE_INACTIVE:
            nextState = NO_TRANSITION;
            break;
        case AA_AFSTATE_ACTIVE_SCAN:
            nextState = HAL_AFSTATE_SCANNING;
            m_AfHwStateFailed = false;
            SetAfStateForService(ANDROID_CONTROL_AF_STATE_PASSIVE_SCAN);
            break;
        case AA_AFSTATE_AF_ACQUIRED_FOCUS:
            nextState = NO_TRANSITION;
            m_AfHwStateFailed = false;
            break;
        case AA_AFSTATE_AF_FAILED_FOCUS:
            nextState = NO_TRANSITION;
            m_AfHwStateFailed = true;
            break;
        default:
            bWrongTransition = true;
            break;
        }
    }
    else if (m_afState == HAL_AFSTATE_NEEDS_DETERMINATION) {
        switch (noti) {
        case AA_AFSTATE_INACTIVE:
            nextState = NO_TRANSITION;
            break;
        case AA_AFSTATE_ACTIVE_SCAN:
            nextState = NO_TRANSITION;
            break;
        case AA_AFSTATE_AF_ACQUIRED_FOCUS:
            m_IsAfLockRequired = true;
            nextState = HAL_AFSTATE_LOCKED;
            SetAfStateForService(ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED);
            break;
        case AA_AFSTATE_AF_FAILED_FOCUS:
            m_IsAfLockRequired = true;
            nextState = HAL_AFSTATE_FAILED;
            SetAfStateForService(ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED);
            break;
        default:
            bWrongTransition = true;
            break;
        }
    }
    else if (m_afState == HAL_AFSTATE_LOCKED) {
        switch (noti) {
            case AA_AFSTATE_INACTIVE:
                nextState = NO_TRANSITION;
                break;
            case AA_AFSTATE_ACTIVE_SCAN:
                bWrongTransition = true;
                break;
            case AA_AFSTATE_AF_ACQUIRED_FOCUS:
                nextState = NO_TRANSITION;
                break;
            case AA_AFSTATE_AF_FAILED_FOCUS:
            default:
                bWrongTransition = true;
                break;
        }
    }
    else if (m_afState == HAL_AFSTATE_FAILED) {
        switch (noti) {
            case AA_AFSTATE_INACTIVE:
                bWrongTransition = true;
                break;
            case AA_AFSTATE_ACTIVE_SCAN:
                nextState = HAL_AFSTATE_SCANNING;
                break;
            case AA_AFSTATE_AF_ACQUIRED_FOCUS:
                bWrongTransition = true;
                break;
            case AA_AFSTATE_AF_FAILED_FOCUS:
                nextState = NO_TRANSITION;
                break;
            default:
                bWrongTransition = true;
                break;
        }
    }
    if (bWrongTransition) {
        ALOGV("(%s): Wrong Transition state(%d) noti(%d)", __FUNCTION__, m_afState, noti);
        return;
    }
    ALOGV("(%s): State (%d) -> (%d) by (%d)", __FUNCTION__, m_afState, nextState, noti);
    if (nextState != NO_TRANSITION)
        m_afState = nextState;
}

void ExynosCameraHWInterface2::OnAfNotificationCAFVideo(enum aa_afstate noti)
{
    int nextState = NO_TRANSITION;
    bool bWrongTransition = false;

    if (m_afState == HAL_AFSTATE_INACTIVE) {
        switch (noti) {
        case AA_AFSTATE_INACTIVE:
        case AA_AFSTATE_ACTIVE_SCAN:
        case AA_AFSTATE_AF_ACQUIRED_FOCUS:
        case AA_AFSTATE_AF_FAILED_FOCUS:
        default:
            nextState = NO_TRANSITION;
            break;
        }
    }
    else if (m_afState == HAL_AFSTATE_STARTED) {
        switch (noti) {
        case AA_AFSTATE_INACTIVE:
            nextState = NO_TRANSITION;
            break;
        case AA_AFSTATE_ACTIVE_SCAN:
            nextState = HAL_AFSTATE_SCANNING;
            SetAfStateForService(ANDROID_CONTROL_AF_STATE_PASSIVE_SCAN);
            break;
        case AA_AFSTATE_AF_ACQUIRED_FOCUS:
            nextState = HAL_AFSTATE_PASSIVE_FOCUSED;
            SetAfStateForService(ANDROID_CONTROL_AF_STATE_PASSIVE_FOCUSED);
            break;
        case AA_AFSTATE_AF_FAILED_FOCUS:
            nextState = HAL_AFSTATE_FAILED;
            SetAfStateForService(ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED);
            break;
        default:
            bWrongTransition = true;
            break;
        }
    }
    else if (m_afState == HAL_AFSTATE_SCANNING) {
        switch (noti) {
        case AA_AFSTATE_INACTIVE:
            bWrongTransition = true;
            break;
        case AA_AFSTATE_ACTIVE_SCAN:
            nextState = NO_TRANSITION;
            break;
        case AA_AFSTATE_AF_ACQUIRED_FOCUS:
            nextState = HAL_AFSTATE_PASSIVE_FOCUSED;
            SetAfStateForService(ANDROID_CONTROL_AF_STATE_PASSIVE_FOCUSED);
            break;
        case AA_AFSTATE_AF_FAILED_FOCUS:
            nextState = NO_TRANSITION;
            m_IsAfTriggerRequired = true;
            break;
        default:
            bWrongTransition = true;
            break;
        }
    }
    else if (m_afState == HAL_AFSTATE_PASSIVE_FOCUSED) {
        switch (noti) {
        case AA_AFSTATE_INACTIVE:
            bWrongTransition = true;
            break;
        case AA_AFSTATE_ACTIVE_SCAN:
            nextState = HAL_AFSTATE_SCANNING;
            SetAfStateForService(ANDROID_CONTROL_AF_STATE_PASSIVE_SCAN);
            break;
        case AA_AFSTATE_AF_ACQUIRED_FOCUS:
            nextState = NO_TRANSITION;
            break;
        case AA_AFSTATE_AF_FAILED_FOCUS:
            nextState = HAL_AFSTATE_FAILED;
            SetAfStateForService(ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED);
            break;
        default:
            bWrongTransition = true;
            break;
        }
    }
    else if (m_afState == HAL_AFSTATE_NEEDS_DETERMINATION) {
        switch (noti) {
        case AA_AFSTATE_INACTIVE:
            bWrongTransition = true;
            break;
        case AA_AFSTATE_ACTIVE_SCAN:
            nextState = NO_TRANSITION;
            break;
        case AA_AFSTATE_AF_ACQUIRED_FOCUS:
            m_IsAfLockRequired = true;
            nextState = HAL_AFSTATE_LOCKED;
            SetAfStateForService(ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED);
            break;
        case AA_AFSTATE_AF_FAILED_FOCUS:
            nextState = HAL_AFSTATE_FAILED;
            SetAfStateForService(ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED);
            break;
        default:
            bWrongTransition = true;
            break;
        }
    }
    else if (m_afState == HAL_AFSTATE_LOCKED) {
        switch (noti) {
            case AA_AFSTATE_INACTIVE:
                nextState = NO_TRANSITION;
                break;
            case AA_AFSTATE_ACTIVE_SCAN:
                bWrongTransition = true;
                break;
            case AA_AFSTATE_AF_ACQUIRED_FOCUS:
                nextState = NO_TRANSITION;
                break;
            case AA_AFSTATE_AF_FAILED_FOCUS:
            default:
                bWrongTransition = true;
                break;
        }
    }
    else if (m_afState == HAL_AFSTATE_FAILED) {
        switch (noti) {
            case AA_AFSTATE_INACTIVE:
            case AA_AFSTATE_ACTIVE_SCAN:
            case AA_AFSTATE_AF_ACQUIRED_FOCUS:
                bWrongTransition = true;
                break;
            case AA_AFSTATE_AF_FAILED_FOCUS:
                nextState = NO_TRANSITION;
                break;
            default:
                bWrongTransition = true;
                break;
        }
    }
    if (bWrongTransition) {
        ALOGV("(%s): Wrong Transition state(%d) noti(%d)", __FUNCTION__, m_afState, noti);
        return;
    }
    ALOGV("(%s): State (%d) -> (%d) by (%d)", __FUNCTION__, m_afState, nextState, noti);
    if (nextState != NO_TRANSITION)
        m_afState = nextState;
}

void ExynosCameraHWInterface2::OnAfCancel(int id)
{
    switch (m_afMode) {
    case AA_AFMODE_AUTO:
    case AA_AFMODE_MACRO:
        OnAfCancelAutoMacro(id);
        break;
    case AA_AFMODE_CONTINUOUS_VIDEO:
        OnAfCancelCAFVideo(id);
        break;
    case AA_AFMODE_CONTINUOUS_PICTURE:
        OnAfCancelCAFPicture(id);
        break;
    case AA_AFMODE_OFF:
    default:
        break;
    }
}

void ExynosCameraHWInterface2::OnAfCancelAutoMacro(int id)
{
    int nextState = NO_TRANSITION;
    m_afTriggerId = id;

    switch (m_afState) {
    case HAL_AFSTATE_INACTIVE:
        nextState = NO_TRANSITION;
        break;
    case HAL_AFSTATE_NEEDS_COMMAND:
    case HAL_AFSTATE_STARTED:
    case HAL_AFSTATE_SCANNING:
    case HAL_AFSTATE_LOCKED:
    case HAL_AFSTATE_FAILED:
        SetAfMode(AA_AFMODE_OFF);
        SetAfStateForService(ANDROID_CONTROL_AF_STATE_INACTIVE);
        nextState = HAL_AFSTATE_INACTIVE;
        break;
    default:
        break;
    }
    ALOGV("(%s): State (%d) -> (%d)", __FUNCTION__, m_afState, nextState);
    if (nextState != NO_TRANSITION)
        m_afState = nextState;
}

void ExynosCameraHWInterface2::OnAfCancelCAFPicture(int id)
{
    int nextState = NO_TRANSITION;
    m_afTriggerId = id;

    switch (m_afState) {
    case HAL_AFSTATE_INACTIVE:
        nextState = NO_TRANSITION;
        break;
    case HAL_AFSTATE_NEEDS_COMMAND:
    case HAL_AFSTATE_STARTED:
    case HAL_AFSTATE_SCANNING:
    case HAL_AFSTATE_LOCKED:
    case HAL_AFSTATE_FAILED:
    case HAL_AFSTATE_NEEDS_DETERMINATION:
    case HAL_AFSTATE_PASSIVE_FOCUSED:
        SetAfMode(AA_AFMODE_OFF);
        SetAfStateForService(ANDROID_CONTROL_AF_STATE_INACTIVE);
        SetAfMode(AA_AFMODE_CONTINUOUS_PICTURE);
        nextState = HAL_AFSTATE_INACTIVE;
        break;
    default:
        break;
    }
    ALOGV("(%s): State (%d) -> (%d)", __FUNCTION__, m_afState, nextState);
    if (nextState != NO_TRANSITION)
        m_afState = nextState;
}

void ExynosCameraHWInterface2::OnAfCancelCAFVideo(int id)
{
    int nextState = NO_TRANSITION;
    m_afTriggerId = id;

    switch (m_afState) {
    case HAL_AFSTATE_INACTIVE:
        nextState = NO_TRANSITION;
        break;
    case HAL_AFSTATE_NEEDS_COMMAND:
    case HAL_AFSTATE_STARTED:
    case HAL_AFSTATE_SCANNING:
    case HAL_AFSTATE_LOCKED:
    case HAL_AFSTATE_FAILED:
    case HAL_AFSTATE_NEEDS_DETERMINATION:
    case HAL_AFSTATE_PASSIVE_FOCUSED:
        SetAfMode(AA_AFMODE_OFF);
        SetAfStateForService(ANDROID_CONTROL_AF_STATE_INACTIVE);
        SetAfMode(AA_AFMODE_CONTINUOUS_VIDEO);
        nextState = HAL_AFSTATE_INACTIVE;
        break;
    default:
        break;
    }
    ALOGV("(%s): State (%d) -> (%d)", __FUNCTION__, m_afState, nextState);
    if (nextState != NO_TRANSITION)
        m_afState = nextState;
}

void ExynosCameraHWInterface2::SetAfStateForService(int newState)
{
    m_serviceAfState = newState;
    m_notifyCb(CAMERA2_MSG_AUTOFOCUS, newState, m_afTriggerId, 0, m_callbackCookie);
}

int ExynosCameraHWInterface2::GetAfStateForService()
{
   return m_serviceAfState;
}

void ExynosCameraHWInterface2::SetAfMode(enum aa_afmode afMode)
{
    if (m_afMode != afMode) {
        if (m_IsAfModeUpdateRequired) {
            m_afMode2 = afMode;
            ALOGV("(%s): pending(%d) and new(%d)", __FUNCTION__, m_afMode, afMode);
        }
        else {
            ALOGV("(%s): current(%d) new(%d)", __FUNCTION__, m_afMode, afMode);
            m_IsAfModeUpdateRequired = true;
            m_afMode = afMode;
        }
    }
}

void ExynosCameraHWInterface2::m_setExifFixedAttribute(void)
{
    char property[PROPERTY_VALUE_MAX];

    //2 0th IFD TIFF Tags
    //3 Maker
    property_get("ro.product.brand", property, EXIF_DEF_MAKER);
    strncpy((char *)mExifInfo.maker, property,
                sizeof(mExifInfo.maker) - 1);
    mExifInfo.maker[sizeof(mExifInfo.maker) - 1] = '\0';
    //3 Model
    property_get("ro.product.model", property, EXIF_DEF_MODEL);
    strncpy((char *)mExifInfo.model, property,
                sizeof(mExifInfo.model) - 1);
    mExifInfo.model[sizeof(mExifInfo.model) - 1] = '\0';
    //3 Software
    property_get("ro.build.id", property, EXIF_DEF_SOFTWARE);
    strncpy((char *)mExifInfo.software, property,
                sizeof(mExifInfo.software) - 1);
    mExifInfo.software[sizeof(mExifInfo.software) - 1] = '\0';

    //3 YCbCr Positioning
    mExifInfo.ycbcr_positioning = EXIF_DEF_YCBCR_POSITIONING;

    //2 0th IFD Exif Private Tags
    //3 F Number
    mExifInfo.fnumber.num = EXIF_DEF_FNUMBER_NUM;
    mExifInfo.fnumber.den = EXIF_DEF_FNUMBER_DEN;
    //3 Exposure Program
    mExifInfo.exposure_program = EXIF_DEF_EXPOSURE_PROGRAM;
    //3 Exif Version
    memcpy(mExifInfo.exif_version, EXIF_DEF_EXIF_VERSION, sizeof(mExifInfo.exif_version));
    //3 Aperture
    uint32_t av = APEX_FNUM_TO_APERTURE((double)mExifInfo.fnumber.num/mExifInfo.fnumber.den);
    mExifInfo.aperture.num = av*EXIF_DEF_APEX_DEN;
    mExifInfo.aperture.den = EXIF_DEF_APEX_DEN;
    //3 Maximum lens aperture
    mExifInfo.max_aperture.num = mExifInfo.aperture.num;
    mExifInfo.max_aperture.den = mExifInfo.aperture.den;
    //3 Lens Focal Length
    mExifInfo.focal_length.num = EXIF_DEF_FOCAL_LEN_NUM;
    mExifInfo.focal_length.den = EXIF_DEF_FOCAL_LEN_DEN;
    //3 User Comments
    strcpy((char *)mExifInfo.user_comment, EXIF_DEF_USERCOMMENTS);
    //3 Color Space information
    mExifInfo.color_space = EXIF_DEF_COLOR_SPACE;
    //3 Exposure Mode
    mExifInfo.exposure_mode = EXIF_DEF_EXPOSURE_MODE;

    //2 0th IFD GPS Info Tags
    unsigned char gps_version[4] = { 0x02, 0x02, 0x00, 0x00 };
    memcpy(mExifInfo.gps_version_id, gps_version, sizeof(gps_version));

    //2 1th IFD TIFF Tags
    mExifInfo.compression_scheme = EXIF_DEF_COMPRESSION;
    mExifInfo.x_resolution.num = EXIF_DEF_RESOLUTION_NUM;
    mExifInfo.x_resolution.den = EXIF_DEF_RESOLUTION_DEN;
    mExifInfo.y_resolution.num = EXIF_DEF_RESOLUTION_NUM;
    mExifInfo.y_resolution.den = EXIF_DEF_RESOLUTION_DEN;
    mExifInfo.resolution_unit = EXIF_DEF_RESOLUTION_UNIT;
}

void ExynosCameraHWInterface2::m_setExifChangedAttribute(exif_attribute_t *exifInfo, ExynosRect *rect,
	camera2_shot *currentEntry)
{
    camera2_dm *dm = &(currentEntry->dm);
    camera2_ctl *ctl = &(currentEntry->ctl);

    ALOGV("(%s): framecnt(%d) exp(%lld) iso(%d)", __FUNCTION__, ctl->request.frameCount, dm->sensor.exposureTime,dm->aa.isoValue );
    if (!ctl->request.frameCount)
       return;
    //2 0th IFD TIFF Tags
    //3 Width
    exifInfo->width = rect->w;
    //3 Height
    exifInfo->height = rect->h;
    //3 Orientation
    switch (ctl->jpeg.orientation) {
    case 90:
        exifInfo->orientation = EXIF_ORIENTATION_90;
        break;
    case 180:
        exifInfo->orientation = EXIF_ORIENTATION_180;
        break;
    case 270:
        exifInfo->orientation = EXIF_ORIENTATION_270;
        break;
    case 0:
    default:
        exifInfo->orientation = EXIF_ORIENTATION_UP;
        break;
    }

    //3 Date time
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime((char *)exifInfo->date_time, 20, "%Y:%m:%d %H:%M:%S", timeinfo);

    //2 0th IFD Exif Private Tags
    //3 Exposure Time
    int shutterSpeed = (dm->sensor.exposureTime/1000);

    if (shutterSpeed < 0) {
        shutterSpeed = 100;
    }

    exifInfo->exposure_time.num = 1;
    // x us -> 1/x s */
    //exifInfo->exposure_time.den = (uint32_t)(1000000 / shutterSpeed);
    exifInfo->exposure_time.den = (uint32_t)((double)1000000 / shutterSpeed);

    //3 ISO Speed Rating
    exifInfo->iso_speed_rating = dm->aa.isoValue;

    uint32_t av, tv, bv, sv, ev;
    av = APEX_FNUM_TO_APERTURE((double)exifInfo->fnumber.num / exifInfo->fnumber.den);
    tv = APEX_EXPOSURE_TO_SHUTTER((double)exifInfo->exposure_time.num / exifInfo->exposure_time.den);
    sv = APEX_ISO_TO_FILMSENSITIVITY(exifInfo->iso_speed_rating);
    bv = av + tv - sv;
    ev = av + tv;
    //ALOGD("Shutter speed=%d us, iso=%d", shutterSpeed, exifInfo->iso_speed_rating);
    ALOGD("AV=%d, TV=%d, SV=%d", av, tv, sv);

    //3 Shutter Speed
    exifInfo->shutter_speed.num = tv * EXIF_DEF_APEX_DEN;
    exifInfo->shutter_speed.den = EXIF_DEF_APEX_DEN;
    //3 Brightness
    exifInfo->brightness.num = bv*EXIF_DEF_APEX_DEN;
    exifInfo->brightness.den = EXIF_DEF_APEX_DEN;
    //3 Exposure Bias
    if (ctl->aa.sceneMode== AA_SCENE_MODE_BEACH||
        ctl->aa.sceneMode== AA_SCENE_MODE_SNOW) {
        exifInfo->exposure_bias.num = EXIF_DEF_APEX_DEN;
        exifInfo->exposure_bias.den = EXIF_DEF_APEX_DEN;
    } else {
        exifInfo->exposure_bias.num = 0;
        exifInfo->exposure_bias.den = 0;
    }
    //3 Metering Mode
    /*switch (m_curCameraInfo->metering) {
    case METERING_MODE_CENTER:
        exifInfo->metering_mode = EXIF_METERING_CENTER;
        break;
    case METERING_MODE_MATRIX:
        exifInfo->metering_mode = EXIF_METERING_MULTISPOT;
        break;
    case METERING_MODE_SPOT:
        exifInfo->metering_mode = EXIF_METERING_SPOT;
        break;
    case METERING_MODE_AVERAGE:
    default:
        exifInfo->metering_mode = EXIF_METERING_AVERAGE;
        break;
    }*/
    exifInfo->metering_mode = EXIF_METERING_CENTER;

    //3 Flash
    int flash = dm->flash.flashMode;
    if (dm->flash.flashMode == FLASH_MODE_OFF || flash < 0)
        exifInfo->flash = EXIF_DEF_FLASH;
    else
        exifInfo->flash = flash;

    //3 White Balance
    if (dm->aa.awbMode == AA_AWBMODE_WB_AUTO)
        exifInfo->white_balance = EXIF_WB_AUTO;
    else
        exifInfo->white_balance = EXIF_WB_MANUAL;

    //3 Scene Capture Type
    switch (ctl->aa.sceneMode) {
    case AA_SCENE_MODE_PORTRAIT:
        exifInfo->scene_capture_type = EXIF_SCENE_PORTRAIT;
        break;
    case AA_SCENE_MODE_LANDSCAPE:
        exifInfo->scene_capture_type = EXIF_SCENE_LANDSCAPE;
        break;
    case AA_SCENE_MODE_NIGHT_PORTRAIT:
        exifInfo->scene_capture_type = EXIF_SCENE_NIGHT;
        break;
    default:
        exifInfo->scene_capture_type = EXIF_SCENE_STANDARD;
        break;
    }

    //2 0th IFD GPS Info Tags
    if (ctl->jpeg.gpsCoordinates[0] != 0 && ctl->jpeg.gpsCoordinates[1] != 0) {

        if (ctl->jpeg.gpsCoordinates[0] > 0)
            strcpy((char *)exifInfo->gps_latitude_ref, "N");
        else
            strcpy((char *)exifInfo->gps_latitude_ref, "S");

        if (ctl->jpeg.gpsCoordinates[1] > 0)
            strcpy((char *)exifInfo->gps_longitude_ref, "E");
        else
            strcpy((char *)exifInfo->gps_longitude_ref, "W");

        if (ctl->jpeg.gpsCoordinates[2] > 0)
            exifInfo->gps_altitude_ref = 0;
        else
            exifInfo->gps_altitude_ref = 1;

        double latitude = fabs(ctl->jpeg.gpsCoordinates[0] / 10000.0);
        double longitude = fabs(ctl->jpeg.gpsCoordinates[1] / 10000.0);
        double altitude = fabs(ctl->jpeg.gpsCoordinates[2] / 100.0);

        exifInfo->gps_latitude[0].num = (uint32_t)latitude;
        exifInfo->gps_latitude[0].den = 1;
        exifInfo->gps_latitude[1].num = (uint32_t)((latitude - exifInfo->gps_latitude[0].num) * 60);
        exifInfo->gps_latitude[1].den = 1;
        exifInfo->gps_latitude[2].num = (uint32_t)((((latitude - exifInfo->gps_latitude[0].num) * 60)
                                        - exifInfo->gps_latitude[1].num) * 60);
        exifInfo->gps_latitude[2].den = 1;

        exifInfo->gps_longitude[0].num = (uint32_t)longitude;
        exifInfo->gps_longitude[0].den = 1;
        exifInfo->gps_longitude[1].num = (uint32_t)((longitude - exifInfo->gps_longitude[0].num) * 60);
        exifInfo->gps_longitude[1].den = 1;
        exifInfo->gps_longitude[2].num = (uint32_t)((((longitude - exifInfo->gps_longitude[0].num) * 60)
                                        - exifInfo->gps_longitude[1].num) * 60);
        exifInfo->gps_longitude[2].den = 1;

        exifInfo->gps_altitude.num = (uint32_t)altitude;
        exifInfo->gps_altitude.den = 1;

        struct tm tm_data;
        long timestamp;
        timestamp = (long)ctl->jpeg.gpsTimestamp;
        gmtime_r(&timestamp, &tm_data);
        exifInfo->gps_timestamp[0].num = tm_data.tm_hour;
        exifInfo->gps_timestamp[0].den = 1;
        exifInfo->gps_timestamp[1].num = tm_data.tm_min;
        exifInfo->gps_timestamp[1].den = 1;
        exifInfo->gps_timestamp[2].num = tm_data.tm_sec;
        exifInfo->gps_timestamp[2].den = 1;
        snprintf((char*)exifInfo->gps_datestamp, sizeof(exifInfo->gps_datestamp),
                "%04d:%02d:%02d", tm_data.tm_year + 1900, tm_data.tm_mon + 1, tm_data.tm_mday);

        exifInfo->enableGps = true;
    } else {
        exifInfo->enableGps = false;
    }

    //2 1th IFD TIFF Tags
    exifInfo->widthThumb = ctl->jpeg.thumbnailSize[0];
    exifInfo->heightThumb = ctl->jpeg.thumbnailSize[1];
}

ExynosCameraHWInterface2::MainThread::~MainThread()
{
    ALOGV("(%s):", __FUNCTION__);
}

void ExynosCameraHWInterface2::MainThread::release()
{
    ALOGV("(%s):", __func__);
    SetSignal(SIGNAL_THREAD_RELEASE);
}

ExynosCameraHWInterface2::SensorThread::~SensorThread()
{
    ALOGV("(%s):", __FUNCTION__);
}

void ExynosCameraHWInterface2::SensorThread::release()
{
    ALOGV("(%s):", __func__);
    SetSignal(SIGNAL_THREAD_RELEASE);
}

ExynosCameraHWInterface2::IspThread::~IspThread()
{
    ALOGV("(%s):", __FUNCTION__);
}

void ExynosCameraHWInterface2::IspThread::release()
{
    ALOGV("(%s):", __func__);
    SetSignal(SIGNAL_THREAD_RELEASE);
}

ExynosCameraHWInterface2::StreamThread::~StreamThread()
{
    ALOGV("(%s):", __FUNCTION__);
}

void ExynosCameraHWInterface2::StreamThread::setParameter(stream_parameters_t * new_parameters)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);

    m_tempParameters = new_parameters;

    SetSignal(SIGNAL_STREAM_CHANGE_PARAMETER);

    // TODO : return synchronously (after setting parameters asynchronously)
    usleep(2000);
}

void ExynosCameraHWInterface2::StreamThread::applyChange()
{
    memcpy(&m_parameters, m_tempParameters, sizeof(stream_parameters_t));

    ALOGV("DEBUG(%s):  Applying Stream paremeters  width(%d), height(%d)",
            __FUNCTION__, m_parameters.outputWidth, m_parameters.outputHeight);
}

void ExynosCameraHWInterface2::StreamThread::release()
{
    ALOGV("(%s):", __func__);
    SetSignal(SIGNAL_THREAD_RELEASE);
}

int ExynosCameraHWInterface2::StreamThread::findBufferIndex(void * bufAddr)
{
    int index;
    for (index = 0 ; index < m_parameters.numSvcBuffers ; index++) {
        if (m_parameters.svcBuffers[index].virt.extP[0] == bufAddr)
            return index;
    }
    return -1;
}

void ExynosCameraHWInterface2::StreamThread::setRecordingParameter(record_parameters_t * recordParm)
{
    memcpy(&m_recordParameters, recordParm, sizeof(record_parameters_t));
}

int ExynosCameraHWInterface2::createIonClient(ion_client ionClient)
{
    if (ionClient == 0) {
        ionClient = ion_client_create();
        if (ionClient < 0) {
            ALOGE("[%s]src ion client create failed, value = %d\n", __FUNCTION__, ionClient);
            return 0;
        }
    }

    return ionClient;
}

int ExynosCameraHWInterface2::deleteIonClient(ion_client ionClient)
{
    if (ionClient != 0) {
        if (ionClient > 0) {
            ion_client_destroy(ionClient);
        }
        ionClient = 0;
    }

    return ionClient;
}

int ExynosCameraHWInterface2::allocCameraMemory(ion_client ionClient, ExynosBuffer *buf, int iMemoryNum)
{
    int ret = 0;
    int i = 0;

    if (ionClient == 0) {
        ALOGE("[%s] ionClient is zero (%d)\n", __FUNCTION__, ionClient);
        return -1;
    }

    for (i=0;i<iMemoryNum;i++) {
        if (buf->size.extS[i] == 0) {
            break;
        }

        buf->fd.extFd[i] = ion_alloc(ionClient, \
                                      buf->size.extS[i], 0, ION_HEAP_EXYNOS_MASK,0);
        if ((buf->fd.extFd[i] == -1) ||(buf->fd.extFd[i] == 0)) {
            ALOGE("[%s]ion_alloc(%d) failed\n", __FUNCTION__, buf->size.extS[i]);
            buf->fd.extFd[i] = -1;
            freeCameraMemory(buf, iMemoryNum);
            return -1;
        }

        buf->virt.extP[i] = (char *)ion_map(buf->fd.extFd[i], \
                                        buf->size.extS[i], 0);
        if ((buf->virt.extP[i] == (char *)MAP_FAILED) || (buf->virt.extP[i] == NULL)) {
            ALOGE("[%s]src ion map failed(%d)\n", __FUNCTION__, buf->size.extS[i]);
            buf->virt.extP[i] = (char *)MAP_FAILED;
            freeCameraMemory(buf, iMemoryNum);
            return -1;
        }
        ALOGV("allocCameraMem : [%d][0x%08x] size(%d)", i, (unsigned int)(buf->virt.extP[i]), buf->size.extS[i]);
    }

    return ret;
}

void ExynosCameraHWInterface2::freeCameraMemory(ExynosBuffer *buf, int iMemoryNum)
{

    int i =0 ;
    int ret = 0;

    for (i=0;i<iMemoryNum;i++) {
        if (buf->fd.extFd[i] != -1) {
            if (buf->virt.extP[i] != (char *)MAP_FAILED) {
                ret = ion_unmap(buf->virt.extP[i], buf->size.extS[i]);
                if (ret < 0)
                    ALOGE("ERR(%s)", __FUNCTION__);
            }
            ion_free(buf->fd.extFd[i]);
        }
        buf->fd.extFd[i] = -1;
        buf->virt.extP[i] = (char *)MAP_FAILED;
        buf->size.extS[i] = 0;
    }
}

void ExynosCameraHWInterface2::initCameraMemory(ExynosBuffer *buf, int iMemoryNum)
{
    int i =0 ;
    for (i=0;i<iMemoryNum;i++) {
        buf->virt.extP[i] = (char *)MAP_FAILED;
        buf->fd.extFd[i] = -1;
        buf->size.extS[i] = 0;
    }
}




static camera2_device_t *g_cam2_device = NULL;
static bool g_camera_vaild = false;
ExynosCamera2 * g_camera2[2] = { NULL, NULL };

static int HAL2_camera_device_close(struct hw_device_t* device)
{
    ALOGV("%s: ENTER", __FUNCTION__);
    if (device) {

        camera2_device_t *cam_device = (camera2_device_t *)device;
        ALOGV("cam_device(0x%08x):", (unsigned int)cam_device);
        ALOGV("g_cam2_device(0x%08x):", (unsigned int)g_cam2_device);
        delete static_cast<ExynosCameraHWInterface2 *>(cam_device->priv);
        g_cam2_device = NULL;
        free(cam_device);
        g_camera_vaild = false;
    }
    if (g_camera2[0] != NULL) {
        delete static_cast<ExynosCamera2 *>(g_camera2[0]);
        g_camera2[0] = NULL;
    }

    if (g_camera2[1] != NULL) {
        delete static_cast<ExynosCamera2 *>(g_camera2[1]);
        g_camera2[1] = NULL;
    }

    ALOGV("%s: EXIT", __FUNCTION__);
    return 0;
}

static inline ExynosCameraHWInterface2 *obj(const struct camera2_device *dev)
{
    return reinterpret_cast<ExynosCameraHWInterface2 *>(dev->priv);
}

static int HAL2_device_set_request_queue_src_ops(const struct camera2_device *dev,
            const camera2_request_queue_src_ops_t *request_src_ops)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->setRequestQueueSrcOps(request_src_ops);
}

static int HAL2_device_notify_request_queue_not_empty(const struct camera2_device *dev)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->notifyRequestQueueNotEmpty();
}

static int HAL2_device_set_frame_queue_dst_ops(const struct camera2_device *dev,
            const camera2_frame_queue_dst_ops_t *frame_dst_ops)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->setFrameQueueDstOps(frame_dst_ops);
}

static int HAL2_device_get_in_progress_count(const struct camera2_device *dev)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->getInProgressCount();
}

static int HAL2_device_flush_captures_in_progress(const struct camera2_device *dev)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->flushCapturesInProgress();
}

static int HAL2_device_construct_default_request(const struct camera2_device *dev,
            int request_template, camera_metadata_t **request)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->constructDefaultRequest(request_template, request);
}

static int HAL2_device_allocate_stream(
            const struct camera2_device *dev,
            // inputs
            uint32_t width,
            uint32_t height,
            int      format,
            const camera2_stream_ops_t *stream_ops,
            // outputs
            uint32_t *stream_id,
            uint32_t *format_actual,
            uint32_t *usage,
            uint32_t *max_buffers)
{
    ALOGV("(%s): ", __FUNCTION__);
    return obj(dev)->allocateStream(width, height, format, stream_ops,
                                    stream_id, format_actual, usage, max_buffers);
}


static int HAL2_device_register_stream_buffers(const struct camera2_device *dev,
            uint32_t stream_id,
            int num_buffers,
            buffer_handle_t *buffers)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->registerStreamBuffers(stream_id, num_buffers, buffers);
}

static int HAL2_device_release_stream(
        const struct camera2_device *dev,
            uint32_t stream_id)
{
    ALOGV("DEBUG(%s)(id: %d):", __FUNCTION__, stream_id);
    if (!g_camera_vaild)
        return 0;
    return obj(dev)->releaseStream(stream_id);
}

static int HAL2_device_allocate_reprocess_stream(
           const struct camera2_device *dev,
            uint32_t width,
            uint32_t height,
            uint32_t format,
            const camera2_stream_in_ops_t *reprocess_stream_ops,
            // outputs
            uint32_t *stream_id,
            uint32_t *consumer_usage,
            uint32_t *max_buffers)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->allocateReprocessStream(width, height, format, reprocess_stream_ops,
                                    stream_id, consumer_usage, max_buffers);
}

static int HAL2_device_release_reprocess_stream(
        const struct camera2_device *dev,
            uint32_t stream_id)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->releaseReprocessStream(stream_id);
}

static int HAL2_device_trigger_action(const struct camera2_device *dev,
           uint32_t trigger_id,
            int ext1,
            int ext2)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->triggerAction(trigger_id, ext1, ext2);
}

static int HAL2_device_set_notify_callback(const struct camera2_device *dev,
            camera2_notify_callback notify_cb,
            void *user)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->setNotifyCallback(notify_cb, user);
}

static int HAL2_device_get_metadata_vendor_tag_ops(const struct camera2_device*dev,
            vendor_tag_query_ops_t **ops)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->getMetadataVendorTagOps(ops);
}

static int HAL2_device_dump(const struct camera2_device *dev, int fd)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->dump(fd);
}





static int HAL2_getNumberOfCameras()
{
    ALOGV("(%s): returning 2", __FUNCTION__);
    return 2;
}


static int HAL2_getCameraInfo(int cameraId, struct camera_info *info)
{
    ALOGV("DEBUG(%s): cameraID: %d", __FUNCTION__, cameraId);
    static camera_metadata_t * mCameraInfo[2] = {NULL, NULL};

    status_t res;

    if (cameraId == 0) {
        info->facing = CAMERA_FACING_BACK;
        if (!g_camera2[0])
            g_camera2[0] = new ExynosCamera2(0);
    }
    else if (cameraId == 1) {
        info->facing = CAMERA_FACING_FRONT;
        if (!g_camera2[1])
            g_camera2[1] = new ExynosCamera2(1);
    }
    else
        return BAD_VALUE;

    info->orientation = 0;
    info->device_version = HARDWARE_DEVICE_API_VERSION(2, 0);
    if (mCameraInfo[cameraId] == NULL) {
        res = g_camera2[cameraId]->constructStaticInfo(&(mCameraInfo[cameraId]), cameraId, true);
        if (res != OK) {
            ALOGE("%s: Unable to allocate static info: %s (%d)",
                    __FUNCTION__, strerror(-res), res);
            return res;
        }
        res = g_camera2[cameraId]->constructStaticInfo(&(mCameraInfo[cameraId]), cameraId, false);
        if (res != OK) {
            ALOGE("%s: Unable to fill in static info: %s (%d)",
                    __FUNCTION__, strerror(-res), res);
            return res;
        }
    }
    info->static_camera_characteristics = mCameraInfo[cameraId];
    return NO_ERROR;
}

#define SET_METHOD(m) m : HAL2_device_##m

static camera2_device_ops_t camera2_device_ops = {
        SET_METHOD(set_request_queue_src_ops),
        SET_METHOD(notify_request_queue_not_empty),
        SET_METHOD(set_frame_queue_dst_ops),
        SET_METHOD(get_in_progress_count),
        SET_METHOD(flush_captures_in_progress),
        SET_METHOD(construct_default_request),
        SET_METHOD(allocate_stream),
        SET_METHOD(register_stream_buffers),
        SET_METHOD(release_stream),
        SET_METHOD(allocate_reprocess_stream),
        SET_METHOD(release_reprocess_stream),
        SET_METHOD(trigger_action),
        SET_METHOD(set_notify_callback),
        SET_METHOD(get_metadata_vendor_tag_ops),
        SET_METHOD(dump),
};

#undef SET_METHOD


static int HAL2_camera_device_open(const struct hw_module_t* module,
                                  const char *id,
                                  struct hw_device_t** device)
{


    int cameraId = atoi(id);

    g_camera_vaild = false;
    ALOGV("\n\n>>> I'm Samsung's CameraHAL_2(ID:%d) <<<\n\n", cameraId);
    if (cameraId < 0 || cameraId >= HAL2_getNumberOfCameras()) {
        ALOGE("ERR(%s):Invalid camera ID %s", __FUNCTION__, id);
        return -EINVAL;
    }

    ALOGV("g_cam2_device : 0x%08x", (unsigned int)g_cam2_device);
    if (g_cam2_device) {
        if (obj(g_cam2_device)->getCameraId() == cameraId) {
            ALOGV("DEBUG(%s):returning existing camera ID %s", __FUNCTION__, id);
            goto done;
        } else {

            while (g_cam2_device)
                usleep(10000);
        }
    }

    g_cam2_device = (camera2_device_t *)malloc(sizeof(camera2_device_t));
    ALOGV("g_cam2_device : 0x%08x", (unsigned int)g_cam2_device);

    if (!g_cam2_device)
        return -ENOMEM;

    g_cam2_device->common.tag     = HARDWARE_DEVICE_TAG;
    g_cam2_device->common.version = CAMERA_DEVICE_API_VERSION_2_0;
    g_cam2_device->common.module  = const_cast<hw_module_t *>(module);
    g_cam2_device->common.close   = HAL2_camera_device_close;

    g_cam2_device->ops = &camera2_device_ops;

    ALOGV("DEBUG(%s):open camera2 %s", __FUNCTION__, id);

    g_cam2_device->priv = new ExynosCameraHWInterface2(cameraId, g_cam2_device, g_camera2[cameraId]);

done:
    *device = (hw_device_t *)g_cam2_device;
    ALOGV("DEBUG(%s):opened camera2 %s (%p)", __FUNCTION__, id, *device);
    g_camera_vaild = true;

    return 0;
}


static hw_module_methods_t camera_module_methods = {
            open : HAL2_camera_device_open
};

extern "C" {
    struct camera_module HAL_MODULE_INFO_SYM = {
      common : {
          tag                : HARDWARE_MODULE_TAG,
          module_api_version : CAMERA_MODULE_API_VERSION_2_0,
          hal_api_version    : HARDWARE_HAL_API_VERSION,
          id                 : CAMERA_HARDWARE_MODULE_ID,
          name               : "Exynos Camera HAL2",
          author             : "Samsung Corporation",
          methods            : &camera_module_methods,
          dso:                NULL,
          reserved:           {0},
      },
      get_number_of_cameras : HAL2_getNumberOfCameras,
      get_camera_info       : HAL2_getCameraInfo
    };
}

}; // namespace android
