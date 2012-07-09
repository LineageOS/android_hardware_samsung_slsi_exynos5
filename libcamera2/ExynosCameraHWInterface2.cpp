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
 * \date      2012/05/31
 *
 * <b>Revision History: </b>
 * - 2012/05/31 : Sungjoong Kang(sj3.kang@samsung.com) \n
 *   Initial Release
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "ExynosCameraHWInterface2"
#include <utils/Log.h>

#include "ExynosCameraHWInterface2.h"
#include "exynos_format.h"



namespace android {



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
        ALOGE("%s:S_FMT, Out of bound : Number of element plane",__func__);
    }

    /* Set up for capture */
    ret = exynos_v4l2_s_fmt(node->fd, &v4l2_fmt);

    if (ret < 0)
        ALOGE("%s: exynos_v4l2_s_fmt fail (%d)",__func__, ret); 

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
        ALOGE("%s: VIDIOC_REQBUFS (fd:%d) failed (%d)",__func__,node->fd, ret);

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
        v4l2_buf.m.planes[i].m.fd = (int)(node->buffer[index].ionBuffer[i]);
        v4l2_buf.m.planes[i].length  = (unsigned long)(node->buffer[index].size[i]);
    }

    ret = exynos_v4l2_qbuf(node->fd, &v4l2_buf);

    if (ret < 0)
        ALOGE("%s: cam_int_qbuf failed (index:%d)(ret:%d)",__func__, index, ret);

    return ret;
}

int cam_int_streamon(node_info_t *node)
{
    enum v4l2_buf_type type = node->type;
    int ret;

    ret = exynos_v4l2_streamon(node->fd, type);

    if (ret < 0)
        ALOGE("%s: VIDIOC_STREAMON failed (%d)",__func__, ret);

    ALOGV("On streaming I/O... ... fd(%d)", node->fd);

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
        ALOGE("%s: VIDIOC_DQBUF failed (%d)",__func__, ret);

    return v4l2_buf.index;
}

int cam_int_s_input(node_info_t *node, int index)
{
    int ret;
    
    ret = exynos_v4l2_s_input(node->fd, index);
    if (ret < 0)
        ALOGE("%s: VIDIOC_S_INPUT failed (%d)",__func__, ret);

    return ret;
}


gralloc_module_t const* ExynosCameraHWInterface2::m_grallocHal;

RequestManager::RequestManager(SignalDrivenThread* main_thread):
    m_numOfEntries(0),
    m_entryInsertionIndex(0),
    m_entryProcessingIndex(0),
    m_entryFrameOutputIndex(0)
{
    m_metadataConverter = new MetadataConverter;
    m_mainThread = main_thread;
    for (int i=0 ; i<NUM_MAX_REQUEST_MGR_ENTRY; i++)
        entries[i].status = EMPTY;
    return;
}

RequestManager::~RequestManager()
{
    return;
}

int RequestManager::GetNumEntries()
{
    return m_numOfEntries;
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
    ALOGV("DEBUG(%s):", __func__);
    
    Mutex::Autolock lock(m_requestMutex);
    
    request_manager_entry * newEntry = NULL;
    int newInsertionIndex = ++m_entryInsertionIndex;
    if (newInsertionIndex >= NUM_MAX_REQUEST_MGR_ENTRY)
        newInsertionIndex = 0;
    ALOGV("DEBUG(%s): got lock, new insertIndex(%d), cnt before reg(%d)", __func__,newInsertionIndex,m_numOfEntries );         

    
    newEntry = &(entries[newInsertionIndex]);

    if (newEntry->status!=EMPTY) {
        ALOGE("ERROR(%s): Circular buffer abnormal ", __func__);
        return;    
    }
    newEntry->status = REGISTERED;
    newEntry->original_request = new_request;
    // TODO : allocate internal_request dynamically
    m_metadataConverter->ToInternalCtl(new_request, &(newEntry->internal_request));
    newEntry->output_stream_count = newEntry->internal_request.ctl.request.numOutputStream;

    m_numOfEntries++;
    m_entryInsertionIndex = newInsertionIndex;

    
    ALOGV("## RegisterReq DONE num(%d), insert(%d), processing(%d), frame(%d), (frameCnt(%d))",
     m_numOfEntries,m_entryInsertionIndex,m_entryProcessingIndex, m_entryFrameOutputIndex, newEntry->internal_request.ctl.request.frameCount);
    
}

void RequestManager::DeregisterRequest(camera_metadata_t ** deregistered_request)
{
    ALOGV("DEBUG(%s):", __func__);
    Mutex::Autolock lock(m_requestMutex);

    request_manager_entry * currentEntry =  &(entries[m_entryFrameOutputIndex]);
    
    if (currentEntry->status!=PROCESSING) {
        ALOGE("ERROR(%s): Circular buffer abnormal. processing(%d), frame(%d), status(%d) ", __func__
        , m_entryProcessingIndex, m_entryFrameOutputIndex,(int)(currentEntry->status));
        return;    
    }
    *deregistered_request = currentEntry->original_request;
    
    currentEntry->status = EMPTY;
    currentEntry->original_request = NULL;
    memset(&(currentEntry->internal_request), 0, sizeof(camera2_ctl_metadata_NEW_t));
    currentEntry->output_stream_count = 0;
    m_numOfEntries--;
    ALOGV("## DeRegistReq DONE num(%d), insert(%d), processing(%d), frame(%d)",
     m_numOfEntries,m_entryInsertionIndex,m_entryProcessingIndex, m_entryFrameOutputIndex);
    
    return;
  
}

void RequestManager::PrepareFrame(size_t* num_entries, size_t* frame_size, 
                camera_metadata_t ** prepared_frame)
{
    ALOGV("DEBUG(%s):", __func__);
    Mutex::Autolock lock(m_requestMutex);
    status_t res = NO_ERROR;
    m_entryFrameOutputIndex++;
    if (m_entryFrameOutputIndex >= NUM_MAX_REQUEST_MGR_ENTRY)
        m_entryFrameOutputIndex = 0;
    request_manager_entry * currentEntry =  &(entries[m_entryFrameOutputIndex]);
    ALOGV("DEBUG(%s): processing(%d), frame(%d), insert(%d)", __func__, 
        m_entryProcessingIndex, m_entryFrameOutputIndex, m_entryInsertionIndex);
    if (currentEntry->status!=PROCESSING) {
        ALOGE("ERROR(%s): Circular buffer abnormal status(%d)", __func__, (int)(currentEntry->status));
        return;    
    }

    m_tempFrameMetadata = place_camera_metadata(m_tempFrameMetadataBuf, 2000, 10, 500); //estimated
    res = m_metadataConverter->ToDynamicMetadata(&(currentEntry->internal_request),
                m_tempFrameMetadata);
    if (res!=NO_ERROR) {
        ALOGE("ERROR(%s): ToDynamicMetadata (%d) ", __func__, res);
        return;    
    }
    *num_entries = get_camera_metadata_entry_count(m_tempFrameMetadata);
    *frame_size = get_camera_metadata_size(m_tempFrameMetadata);
    *prepared_frame = m_tempFrameMetadata;
    ALOGV("## PrepareFrame DONE: frame(%d) frameCnt(%d)", m_entryFrameOutputIndex,
        currentEntry->internal_request.ctl.request.frameCount);
    
    return;
}

void RequestManager::MarkProcessingRequest(exynos_camera_memory_t* buf)
{
    ALOGV("DEBUG(%s):", __func__);
    Mutex::Autolock lock(m_requestMutex);
    camera2_shot_t * current_shot;

    request_manager_entry * newEntry = NULL;
    int newProcessingIndex = m_entryProcessingIndex + 1;
    if (newProcessingIndex >= NUM_MAX_REQUEST_MGR_ENTRY)
        newProcessingIndex = 0;

    
    newEntry = &(entries[newProcessingIndex]);

    if (newEntry->status!=REGISTERED) {
        ALOGE("ERROR(%s): Circular buffer abnormal ", __func__);
        return;    
    }
    newEntry->status = PROCESSING;

    
    m_entryProcessingIndex = newProcessingIndex; 
    ALOGV("## MarkProcReq DONE num(%d), insert(%d), processing(%d), frame(%d)",
     m_numOfEntries,m_entryInsertionIndex,m_entryProcessingIndex, m_entryFrameOutputIndex);
    
        
}

void RequestManager::NotifyStreamOutput(uint32_t stream_id)
{
    ALOGV("DEBUG(%s):", __func__);
    m_mainThread->SetSignal(SIGNAL_MAIN_STREAM_OUTPUT_DONE);
    return;
}

ExynosCameraHWInterface2::ExynosCameraHWInterface2(int cameraId, camera2_device_t *dev):
            m_requestQueueOps(NULL),
            m_frameQueueOps(NULL),
            m_callbackCookie(NULL),
            m_numOfRemainingReqInSvc(0),
            m_isRequestQueuePending(false),
            m_isSensorThreadOn(false),
            m_isStreamStarted(false),
            m_isBufferInit(false),
            m_halDevice(dev),
            m_ionCameraClient(0)
{
    ALOGV("DEBUG(%s):", __func__);
    int ret = 0;

    if (!m_grallocHal) {
        ret = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, (const hw_module_t **)&m_grallocHal);
        if (ret)
            ALOGE("ERR(%s):Fail on loading gralloc HAL", __func__);
    }  

    m_ionCameraClient = createIonClient(m_ionCameraClient);
    if(m_ionCameraClient == 0)
        ALOGE("ERR(%s):Fail onion_client_create", __func__);

    m_mainThread    = new MainThread(this);
    m_sensorThread  = new SensorThread(this);
    usleep(200000);
    m_requestManager = new RequestManager((SignalDrivenThread*)(m_mainThread.get()));
    m_streamThread  = new StreamThread(this, 0);

}

ExynosCameraHWInterface2::~ExynosCameraHWInterface2()
{
    ALOGV("DEBUG(%s):", __func__);
    this->release();
}

void ExynosCameraHWInterface2::release()
{
    int i;
    ALOGV("DEBUG(%s):", __func__);

    if (m_mainThread != NULL) {
        
    }
    
    if (m_sensorThread != NULL){

    }

    for(i = 0; i < m_camera_info.sensor.buffers; i++)
        freeCameraMemory(&m_camera_info.sensor.buffer[i], m_camera_info.sensor.planes);

    for(i = 0; i < m_camera_info.isp.buffers; i++)
        freeCameraMemory(&m_camera_info.isp.buffer[i], m_camera_info.isp.planes);

    for(i = 0; i < m_camera_info.capture.buffers; i++)
        freeCameraMemory(&m_camera_info.capture.buffer[i], m_camera_info.capture.planes);

    deleteIonClient(m_ionCameraClient);
}    
    
int ExynosCameraHWInterface2::getCameraId() const
{
    return 0;
}
    

int ExynosCameraHWInterface2::setRequestQueueSrcOps(const camera2_request_queue_src_ops_t *request_src_ops)
{
    ALOGV("DEBUG(%s):", __func__);
    if ((NULL != request_src_ops) && (NULL != request_src_ops->dequeue_request)
            && (NULL != request_src_ops->free_request) && (NULL != request_src_ops->request_count)) {
        m_requestQueueOps = (camera2_request_queue_src_ops_t*)request_src_ops;
        return 0;
    }
    else {
        ALOGE("DEBUG(%s):setRequestQueueSrcOps : NULL arguments", __func__);
        return 1;
    }
}

int ExynosCameraHWInterface2::notifyRequestQueueNotEmpty()
{
    ALOGV("DEBUG(%s):setting [SIGNAL_MAIN_REQ_Q_NOT_EMPTY]", __func__);
    if ((NULL==m_frameQueueOps)|| (NULL==m_requestQueueOps)) {
        ALOGE("DEBUG(%s):queue ops NULL. ignoring request", __func__);
        return 0;
    }
    m_mainThread->SetSignal(SIGNAL_MAIN_REQ_Q_NOT_EMPTY);
    return 0;
}

int ExynosCameraHWInterface2::setFrameQueueDstOps(const camera2_frame_queue_dst_ops_t *frame_dst_ops)
{
    ALOGV("DEBUG(%s):", __func__);
    if ((NULL != frame_dst_ops) && (NULL != frame_dst_ops->dequeue_frame)
            && (NULL != frame_dst_ops->cancel_frame) && (NULL !=frame_dst_ops->enqueue_frame)) {
        m_frameQueueOps = (camera2_frame_queue_dst_ops_t *)frame_dst_ops;
        return 0;
    }
    else {
        ALOGE("DEBUG(%s):setFrameQueueDstOps : NULL arguments", __func__);
        return 1;
    }
}

int ExynosCameraHWInterface2::getInProgressCount()
{
    int inProgressCount = m_requestManager->GetNumEntries();
    ALOGV("DEBUG(%s): # of dequeued req (%d)", __func__, inProgressCount);
    return inProgressCount;
}

int ExynosCameraHWInterface2::flushCapturesInProgress()
{
    return 0;
}

// temporarily copied from EmulatedFakeCamera2
// TODO : implement our own codes
status_t constructDefaultRequestInternal(
        int request_template,
        camera_metadata_t **request,
        bool sizeRequest);
        
int ExynosCameraHWInterface2::constructDefaultRequest(int request_template, camera_metadata_t **request)
{
    ALOGV("DEBUG(%s): making template (%d) ", __func__, request_template);

    if (request == NULL) return BAD_VALUE;
    if (request_template < 0 || request_template >= CAMERA2_TEMPLATE_COUNT) {
        return BAD_VALUE;
    }
    status_t res;
    // Pass 1, calculate size and allocate
    res = constructDefaultRequestInternal(request_template,
            request,
            true);
    if (res != OK) {
        return res;
    }
    // Pass 2, build request
    res = constructDefaultRequestInternal(request_template,
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
    ALOGV("DEBUG(%s): allocate with width(%d) height(%d) format(%x)", __func__,  width, height, format);
    char node_name[30];
    int fd = 0, i, j;
    *stream_id = 0;
    *format_actual = HAL_PIXEL_FORMAT_YV12;
    *usage = GRALLOC_USAGE_SW_WRITE_OFTEN | GRALLOC_USAGE_YUV_ADDR;
    *max_buffers = 8;

 
     m_streamThread->SetParameter(*stream_id, width, height, *format_actual, stream_ops, *usage, fd, &(m_camera_info.preview));
    return 0;
}

int ExynosCameraHWInterface2::registerStreamBuffers(uint32_t stream_id, int num_buffers, buffer_handle_t *buffers)
{
    int i,j;
    void *virtAddr[3];
    int fd = 0, plane_index = 0;;
    char node_name[30];
    struct v4l2_buffer v4l2_buf;
    struct v4l2_plane  planes[VIDEO_MAX_PLANES];
    
    ALOGV("DEBUG(%s): streamID (%d), num_buff(%d), handle(%x) ", __func__, stream_id, num_buffers, (uint32_t)buffers);
    if (stream_id == 0) {

        memset(&node_name, 0x00, sizeof(char[30]));
        sprintf(node_name, "%s%d", NODE_PREFIX, 44);
        fd = exynos_v4l2_open(node_name, O_RDWR, 0);
        if (fd < 0) {
            ALOGV("DEBUG(%s): failed to open preview video node (%s) fd (%d)", __func__,node_name, fd);
        }
        m_camera_info.preview.fd    = fd;
        m_camera_info.preview.width = 1920;  // to modify
        m_camera_info.preview.height = 1080; // to modify
        m_camera_info.preview.format = V4L2_PIX_FMT_YVU420M;
        m_camera_info.preview.planes = 3;
        m_camera_info.preview.buffers = 8;  // to modify
        m_camera_info.preview.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        m_camera_info.preview.memory = V4L2_MEMORY_DMABUF;


        cam_int_s_input(&(m_camera_info.preview), m_camera_info.sensor_id); 
        cam_int_s_fmt(&(m_camera_info.preview));
        ALOGV("DEBUG(%s): preview calling reqbuf", __func__);
        cam_int_reqbufs(&(m_camera_info.preview));
        
        for (i=0 ; i<m_camera_info.preview.buffers ; i++) {
            ALOGV("Registering Stream Buffers[%d] (%x) width(%d), height(%d)", i, 
                    (uint32_t)(buffers[i]), m_streamThread->m_parameters.width, m_streamThread->m_parameters.height);
                    
            if (m_grallocHal) {
                if (m_grallocHal->lock(m_grallocHal, buffers[i],
                       m_streamThread->m_parameters.usage,
                       0, 0, m_streamThread->m_parameters.width, m_streamThread->m_parameters.height, virtAddr) != 0) {
                       
                    ALOGE("ERR(%s):could not obtain gralloc buffer", __func__);
                }
                else {
                    v4l2_buf.m.planes = planes;
                    v4l2_buf.type       = m_camera_info.preview.type;
                    v4l2_buf.memory     = m_camera_info.preview.memory;
                    v4l2_buf.index      = i;
                    v4l2_buf.length     = 3;

                    const private_handle_t *priv_handle = reinterpret_cast<const private_handle_t *>(buffers[i]);

                    v4l2_buf.m.planes[0].m.fd = priv_handle->fd;
                    v4l2_buf.m.planes[1].m.fd = priv_handle->u_fd;
                    v4l2_buf.m.planes[2].m.fd = priv_handle->v_fd;

                    // HACK
                    m_streamThread->m_parameters.grallocVirtAddr[i] = virtAddr[0];
                    v4l2_buf.m.planes[0].length  = 1920*1088;
                    v4l2_buf.m.planes[1].length  = 1920*1088/4;
                    v4l2_buf.m.planes[2].length  = 1920*1088/4;

                    if (exynos_v4l2_qbuf(m_camera_info.preview.fd, &v4l2_buf) < 0) {
                        ALOGE("ERR(%s):preview exynos_v4l2_qbuf() fail", __func__);
                        return false;
                    }

                }
            }              
            
            
        }
        ALOGV("DEBUG(%s): preview initial QBUF done", __func__);



    }
      ALOGV("DEBUG(%s): END registerStreamBuffers", __func__);  
    return 0;
}

int ExynosCameraHWInterface2::releaseStream(uint32_t stream_id)
{
    ALOGV("DEBUG(%s):", __func__);
    return 0;
}

int ExynosCameraHWInterface2::allocateReprocessStream(
    uint32_t width, uint32_t height, uint32_t format, const camera2_stream_in_ops_t *reprocess_stream_ops,
    uint32_t *stream_id, uint32_t *consumer_usage, uint32_t *max_buffers)
{
    ALOGV("DEBUG(%s):", __func__);
    return 0;
}

int ExynosCameraHWInterface2::releaseReprocessStream(uint32_t stream_id)
{
    ALOGV("DEBUG(%s):", __func__);
    return 0;
}

int ExynosCameraHWInterface2::triggerAction(uint32_t trigger_id, int ext1, int ext2)
{
    ALOGV("DEBUG(%s):", __func__);
    return 0;
}

int ExynosCameraHWInterface2::setNotifyCallback(camera2_notify_callback notify_cb, void *user)
{
    ALOGV("DEBUG(%s):", __func__);
    m_notifyCb = notify_cb;
    m_callbackCookie = user;
    return 0;
}

int ExynosCameraHWInterface2::getMetadataVendorTagOps(vendor_tag_query_ops_t **ops)
{
    ALOGV("DEBUG(%s):", __func__);
    return 0;
}

int ExynosCameraHWInterface2::dump(int fd)
{
    ALOGV("DEBUG(%s):", __func__);
    return 0;
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
    int res = 0;
    
    ALOGV("DEBUG(%s): m_mainThreadFunc (%x)", __func__, currentSignal);
    
    if (currentSignal & SIGNAL_MAIN_REQ_Q_NOT_EMPTY) {
        ALOGV("DEBUG(%s): MainThread processing SIGNAL_MAIN_REQ_Q_NOT_EMPTY", __func__);
        if (m_requestManager->IsRequestQueueFull()==false
                && m_requestManager->GetNumEntries()<NUM_MAX_DEQUEUED_REQUEST) {
            m_requestQueueOps->dequeue_request(m_requestQueueOps, &currentRequest);
            if (NULL == currentRequest) {
                ALOGV("DEBUG(%s): dequeue_request returned NULL ", __func__);    
            }
            else {
                m_requestManager->RegisterRequest(currentRequest);
                
                m_numOfRemainingReqInSvc = m_requestQueueOps->request_count(m_requestQueueOps);
                ALOGV("DEBUG(%s): remaining req cnt (%d)", __func__, m_numOfRemainingReqInSvc);
                if (m_requestManager->IsRequestQueueFull()==false
                    && m_requestManager->GetNumEntries()<NUM_MAX_DEQUEUED_REQUEST)
                    self->SetSignal(SIGNAL_MAIN_REQ_Q_NOT_EMPTY); // dequeue repeatedly
                m_sensorThread->SetSignal(SIGNAL_SENSOR_START_REQ_PROCESSING);
            }
        }
        else {
            m_isRequestQueuePending = true;
        }
    }

    if (currentSignal & SIGNAL_MAIN_STREAM_OUTPUT_DONE) {
        ALOGV("DEBUG(%s): MainThread processing SIGNAL_MAIN_STREAM_OUTPUT_DONE", __func__);
        m_requestManager->PrepareFrame(&numEntries, &frameSize, &preparedFrame);
        m_requestManager->DeregisterRequest(&deregisteredRequest);
        m_requestQueueOps->free_request(m_requestQueueOps, deregisteredRequest);
        m_frameQueueOps->dequeue_frame(m_frameQueueOps, numEntries, frameSize, &currentFrame);
        if (currentFrame==NULL) {
            ALOGE("%s: frame dequeue returned NULL",__func__ );
        }
        else {
            ALOGV("%s: frame dequeue done. numEntries(%d) frameSize(%d)",__func__ , numEntries,frameSize);
        }
            
        res = append_camera_metadata(currentFrame, preparedFrame);
        if (res==0) {
            ALOGV("%s: frame metadata append success",__func__);
            m_frameQueueOps->enqueue_frame(m_frameQueueOps, currentFrame);            
        }
        else {
            ALOGE("%s: frame metadata append fail (%d)",__func__, res);
        }
        self->SetSignal(SIGNAL_MAIN_REQ_Q_NOT_EMPTY);
    }
    return;
}
void ExynosCameraHWInterface2::m_sensorThreadInitialize(SignalDrivenThread * self)
{
    ALOGV("DEBUG(%s): ", __func__ );
    SensorThread * selfSensor = ((SensorThread*)self);
    char node_name[30];
    int fd = 0;
    int i =0, j=0;


    m_camera_info.sensor_id = SENSOR_NAME_S5K4E5;
    
    memset(&m_camera_info.current_shot, 0x00, sizeof(camera2_shot_t));
    m_camera_info.current_shot.ctl.request.metadataMode = METADATA_MODE_FULL;
    m_camera_info.current_shot.magicNumber = 0x23456789;

    m_camera_info.current_shot.ctl.scaler.cropRegion[0] = 0;
    m_camera_info.current_shot.ctl.scaler.cropRegion[1] = 0;
    m_camera_info.current_shot.ctl.scaler.cropRegion[2] = 1920;

    /*sensor init*/ 
    memset(&node_name, 0x00, sizeof(char[30]));
    sprintf(node_name, "%s%d", NODE_PREFIX, 40);
    fd = exynos_v4l2_open(node_name, O_RDWR, 0);
    
    if (fd < 0) {
        ALOGV("DEBUG(%s): failed to open sensor video node (%s) fd (%d)", __func__,node_name, fd);
    }
    else {
        ALOGV("DEBUG(%s): sensor video node opened(%s) fd (%d)", __func__,node_name, fd);
    }
    m_camera_info.sensor.fd = fd;
    m_camera_info.sensor.width = 2560 + 16;
    m_camera_info.sensor.height = 1920 + 10;
    m_camera_info.sensor.format = V4L2_PIX_FMT_SBGGR16;
    m_camera_info.sensor.planes = 2;
    m_camera_info.sensor.buffers = 8;
    m_camera_info.sensor.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    m_camera_info.sensor.memory = V4L2_MEMORY_DMABUF;
    m_camera_info.sensor.ionClient = m_ionCameraClient;

    for(i = 0; i < m_camera_info.sensor.buffers; i++){
        initCameraMemory(&m_camera_info.sensor.buffer[i], m_camera_info.sensor.planes);
        m_camera_info.sensor.buffer[i].size[0] = m_camera_info.sensor.width*m_camera_info.sensor.height*2;
        m_camera_info.sensor.buffer[i].size[1] = 5*1024; // HACK, driver use 5*1024, should be use predefined value
        allocCameraMemory(m_camera_info.sensor.ionClient, &m_camera_info.sensor.buffer[i], m_camera_info.sensor.planes);
    }

    /*isp init*/
    memset(&node_name, 0x00, sizeof(char[30]));
    sprintf(node_name, "%s%d", NODE_PREFIX, 41);
    fd = exynos_v4l2_open(node_name, O_RDWR, 0);
    
    if (fd < 0) {
        ALOGV("DEBUG(%s): failed to open isp video node (%s) fd (%d)", __func__,node_name, fd);
    }
    else {
        ALOGV("DEBUG(%s): isp video node opened(%s) fd (%d)", __func__,node_name, fd);
    }

    m_camera_info.isp.fd = fd;
    m_camera_info.isp.width = 2560;
    m_camera_info.isp.height = 1920;
    m_camera_info.isp.format = V4L2_PIX_FMT_SBGGR10;
    m_camera_info.isp.planes = 1;
    m_camera_info.isp.buffers = 1;
    m_camera_info.isp.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    m_camera_info.isp.memory = V4L2_MEMORY_DMABUF;
    m_camera_info.isp.ionClient = m_ionCameraClient;

    for(i = 0; i < m_camera_info.isp.buffers; i++){
        initCameraMemory(&m_camera_info.isp.buffer[i], m_camera_info.isp.planes);
        m_camera_info.isp.buffer[i].size[0] = m_camera_info.isp.width*m_camera_info.isp.height*2;
        allocCameraMemory(m_camera_info.isp.ionClient, &m_camera_info.isp.buffer[i], m_camera_info.isp.planes);
    };


    /*capture init*/
    memset(&node_name, 0x00, sizeof(char[30]));
    sprintf(node_name, "%s%d", NODE_PREFIX, 42);
    fd = exynos_v4l2_open(node_name, O_RDWR, 0);
    if (fd < 0) {
        ALOGV("DEBUG(%s): failed to open capture video node (%s) fd (%d)", __func__,node_name, fd);
    }
    else {
        ALOGV("DEBUG(%s): capture video node opened(%s) fd (%d)", __func__,node_name, fd);
    }

    
    m_camera_info.capture.fd = fd;
    m_camera_info.capture.width = 2560;
    m_camera_info.capture.height = 1920;
    m_camera_info.capture.format = V4L2_PIX_FMT_YUYV;
    m_camera_info.capture.planes = 1;
    m_camera_info.capture.buffers = 8;
    m_camera_info.capture.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    m_camera_info.capture.memory = V4L2_MEMORY_DMABUF;
    m_camera_info.capture.ionClient = m_ionCameraClient;

    for(i = 0; i < m_camera_info.capture.buffers; i++){
        initCameraMemory(&m_camera_info.capture.buffer[i], m_camera_info.capture.planes);
        m_camera_info.capture.buffer[i].size[0] = m_camera_info.capture.width*m_camera_info.capture.height*2;
        allocCameraMemory(m_camera_info.capture.ionClient, &m_camera_info.capture.buffer[i], m_camera_info.capture.planes);
    }

    cam_int_s_input(&(m_camera_info.sensor), m_camera_info.sensor_id);
    
    if (cam_int_s_fmt(&(m_camera_info.sensor))< 0) {
        ALOGE("DEBUG(%s): sensor s_fmt fail",  __func__);
    }
    cam_int_reqbufs(&(m_camera_info.sensor));

    for (i = 0; i < m_camera_info.sensor.buffers; i++) {
        ALOGV("DEBUG(%s): sensor initial QBUF [%d]",  __func__, i);
        memcpy( m_camera_info.sensor.buffer[i].virBuffer[1], &(m_camera_info.current_shot),
                sizeof(camera2_shot_t));
        
        cam_int_qbuf(&(m_camera_info.sensor), i);
    }
    cam_int_streamon(&(m_camera_info.sensor));
    
    m_camera_info.sensor.currentBufferIndex = 0;

    cam_int_s_input(&(m_camera_info.isp), m_camera_info.sensor_id); 
    cam_int_s_fmt(&(m_camera_info.isp));
    ALOGV("DEBUG(%s): isp calling reqbuf", __func__);
    cam_int_reqbufs(&(m_camera_info.isp));
    ALOGV("DEBUG(%s): isp calling querybuf", __func__);

    for (i = 0; i < m_camera_info.isp.buffers; i++) {
        ALOGV("DEBUG(%s): isp initial QBUF [%d]",  __func__, i);    
        cam_int_qbuf(&(m_camera_info.isp), i);
    }

    cam_int_streamon(&(m_camera_info.isp));


 
    ALOGV("DEBUG(%s): END of SensorThreadInitialize ", __func__);
    return;
}

void ExynosCameraHWInterface2::m_sensorThreadFunc(SignalDrivenThread * self)
{
    uint32_t currentSignal = self->GetProcessingSignal();
    int index;
    ALOGV("DEBUG(%s): m_sensorThreadFunc (%x)", __func__, currentSignal);
    
    if (currentSignal & SIGNAL_SENSOR_START_REQ_PROCESSING)
    {
        ALOGV("DEBUG(%s): SensorThread processing SIGNAL_SENSOR_START_REQ_PROCESSING", __func__);

        m_requestManager->MarkProcessingRequest(&(m_camera_info.sensor.buffer[m_camera_info.sensor.currentBufferIndex]));
        if (!m_isStreamStarted)
        {
            m_isStreamStarted = true;
            ALOGV("DEBUG(%s):  calling preview streamon", __func__);
            cam_int_streamon(&(m_camera_info.preview));   
            ALOGV("DEBUG(%s):  calling preview streamon done", __func__);
            exynos_v4l2_s_ctrl(m_camera_info.isp.fd, V4L2_CID_IS_S_STREAM, IS_ENABLE_STREAM);
            ALOGV("DEBUG(%s):  calling isp sctrl done", __func__);
            exynos_v4l2_s_ctrl(m_camera_info.sensor.fd, V4L2_CID_IS_S_STREAM, IS_ENABLE_STREAM);
            ALOGV("DEBUG(%s):  calling sensor sctrl done", __func__);
            //sleep(3);
        }
        else
        {
            ALOGV("DEBUG(%s):  streaming started already", __func__);
        }
        ALOGV("### Sensor DQBUF start");
        index = cam_int_dqbuf(&(m_camera_info.sensor));
        ALOGV("### Sensor DQBUF done index(%d), calling QBUF", index);
        cam_int_qbuf(&(m_camera_info.sensor), index);
        ALOGV("### Sensor QBUF done index(%d)", index);
        m_streamThread->SetSignal(SIGNAL_STREAM_DATA_COMING);
      
        
    }
    return;
}

void ExynosCameraHWInterface2::m_streamThreadInitialize(SignalDrivenThread * self)
{
    ALOGV("DEBUG(%s): ", __func__ );
    memset(&(((StreamThread*)self)->m_parameters), 0, sizeof(stream_parameters_t));
    memset(&(((StreamThread*)self)->m_tempParameters), 0, sizeof(stream_parameters_t));
    return;
}

int ExynosCameraHWInterface2::matchBuffer(void * bufAddr)
{
    int j;
    for (j=0 ; j < 8 ; j++) {
        if (m_streamThread->m_parameters.grallocVirtAddr[j]== bufAddr)
            return j;
    }
    
    return -1;
}
void ExynosCameraHWInterface2::m_streamThreadFunc(SignalDrivenThread * self)
{
    uint32_t currentSignal = self->GetProcessingSignal();
    ALOGV("DEBUG(%s): m_streamThreadFunc[%d] (%x)", __func__, ((StreamThread*)self)->m_index, currentSignal);

    if (currentSignal & SIGNAL_STREAM_CHANGE_PARAMETER)
    {
        ALOGV("DEBUG(%s): processing SIGNAL_STREAM_CHANGE_PARAMETER", __func__);
        ALOGV("DEBUG(%s): [1] node width(%d), height(%d), fd(%d), buffers(%d)", __func__, 
            m_camera_info.preview.width, m_camera_info.preview.height, m_camera_info.preview.fd, m_camera_info.preview.buffers); 


        ALOGV("DEBUG(%s): [2] node width(%d), height(%d), fd(%d), buffers(%d)", __func__, 
            m_camera_info.preview.width, m_camera_info.preview.height, m_camera_info.preview.fd, m_camera_info.preview.buffers);            
        ((StreamThread*)self)->ApplyChange();
    }
    if (currentSignal & SIGNAL_STREAM_DATA_COMING)
    {
        buffer_handle_t * buf = NULL;
        status_t res;
        void *virtAddr[3];
        int i, j;
        int index;
        int ret;
        StreamThread * selfStream = ((StreamThread*)self);
        ALOGV("DEBUG(%s): processing SIGNAL_STREAM_DATA_COMING", __func__);
        if (!m_isBufferInit) {
            for ( i=0 ; i < 8 ; i++)
            {
                res = selfStream->m_parameters.streamOps->dequeue_buffer(selfStream->m_parameters.streamOps, &buf);
                if (res != NO_ERROR || buf == NULL) {
                    ALOGE("%s: Unable to dequeue buffer : %d",__func__ , res);
                    return;
                }
                
                ALOGV("DEBUG(%s):got buf(%x) version(%d), numFds(%d), numInts(%d)", __func__, (uint32_t)(*buf),
                   ((native_handle_t*)(*buf))->version, ((native_handle_t*)(*buf))->numFds, ((native_handle_t*)(*buf))->numInts);
                   
                if (m_grallocHal->lock(m_grallocHal, *buf,
                           selfStream->m_parameters.usage,
                           0, 0, selfStream->m_parameters.width, selfStream->m_parameters.height, virtAddr) != 0) {
                           
                    ALOGE("ERR(%s):could not obtain gralloc buffer", __func__);
                }
                ALOGV("DEBUG(%s) locked img buf plane0(%x) plane1(%x) plane2(%x)", __func__, (unsigned int)virtAddr[0], (unsigned int)virtAddr[1], (unsigned int)virtAddr[2]); 
                ret = matchBuffer(virtAddr[0]);
                if (ret==-1) {
                    ALOGE("##### could not find matched buffer");
                }
                else {
                    ALOGV("##### found matched buffer[%d]", ret);
                    m_streamThread->m_parameters.bufHandle[i] = buf;
                }
                
            }
            m_isBufferInit = true;
        }
        ALOGV("##### buffer init done[1]");

        ALOGV("### preview DQBUF start");
        index = cam_int_dqbuf(&(m_camera_info.preview));
        ALOGV("### preview DQBUF done index(%d)", index);

        res = selfStream->m_parameters.streamOps->enqueue_buffer(selfStream->m_parameters.streamOps, systemTime(), m_streamThread->m_parameters.bufHandle[index]);
        ALOGV("### preview enqueue_buffer to svc done res(%d)", res);
        
         res = selfStream->m_parameters.streamOps->dequeue_buffer(selfStream->m_parameters.streamOps, &buf);
        if (res != NO_ERROR || buf == NULL) {
            ALOGE("%s: Unable to dequeue buffer : %d",__func__ , res);
            return;
        }
        
        ALOGV("DEBUG(%s):got buf(%x) version(%d), numFds(%d), numInts(%d)", __func__, (uint32_t)(*buf),
           ((native_handle_t*)(*buf))->version, ((native_handle_t*)(*buf))->numFds, ((native_handle_t*)(*buf))->numInts);
           
        if (m_grallocHal->lock(m_grallocHal, *buf,
                   selfStream->m_parameters.usage,
                   0, 0, selfStream->m_parameters.width, selfStream->m_parameters.height, virtAddr) != 0) {
                   
            ALOGE("ERR(%s):could not obtain gralloc buffer", __func__);
        }
        ALOGV("DEBUG(%s) locked img buf plane0(%x) plane1(%x) plane2(%x)", __func__, (unsigned int)virtAddr[0], (unsigned int)virtAddr[1], (unsigned int)virtAddr[2]); 
        ret = matchBuffer(virtAddr[0]);
        if (ret==-1) {
            ALOGE("##### could not find matched buffer");
        }
        else {
            ALOGV("##### found matched buffer[%d]", ret);
            int plane_index;
            struct v4l2_buffer v4l2_buf;
            struct v4l2_plane  planes[VIDEO_MAX_PLANES];


                   
            v4l2_buf.m.planes = planes;
            v4l2_buf.type       = m_camera_info.preview.type;
            v4l2_buf.memory     = m_camera_info.preview.memory;
            v4l2_buf.index      = ret;
            v4l2_buf.length     = 3;

            const private_handle_t *priv_handle = reinterpret_cast<const private_handle_t *>(*buf);

            v4l2_buf.m.planes[0].m.fd = priv_handle->fd;
            v4l2_buf.m.planes[1].m.fd = priv_handle->u_fd;
            v4l2_buf.m.planes[2].m.fd = priv_handle->v_fd;

            // HACK
            v4l2_buf.m.planes[0].length  = 1920*1088;
            v4l2_buf.m.planes[1].length  = 1920*1088/4;
            v4l2_buf.m.planes[2].length  = 1920*1088/4;
            

            if (exynos_v4l2_qbuf(m_camera_info.preview.fd, &v4l2_buf) < 0) {
                ALOGE("ERR(%s):preview exynos_v4l2_qbuf() fail", __func__);
                return;
            }

            
            ALOGV("### preview QBUF done index(%d)", index);
        }

        m_requestManager->NotifyStreamOutput(selfStream->m_parameters.id);
        
    }
 
    return;
}

void ExynosCameraHWInterface2::StreamThread::SetParameter(uint32_t id, uint32_t width, uint32_t height,
        int format, const camera2_stream_ops_t* stream_ops, uint32_t usage, int fd, node_info_t * node)
{
    ALOGV("DEBUG(%s): id(%d) width(%d) height(%d) format(%x) fd(%d)", __func__, id, width, height, format, fd); 
    m_tempParameters.id = id;
    m_tempParameters.width = width;
    m_tempParameters.height = height;
    m_tempParameters.format = format;
    m_tempParameters.streamOps = stream_ops;
    m_tempParameters.usage = usage;
    m_tempParameters.fd = fd;
    m_tempParameters.node = node;
    ALOGV("DEBUG(%s): node width(%d), height(%d), fd(%d), buffers(%d)", __func__, 
     (m_tempParameters.node)->width, (m_tempParameters.node)->height, (m_tempParameters.node)->fd, (m_tempParameters.node)->buffers); 
   
    SetSignal(SIGNAL_STREAM_CHANGE_PARAMETER);
}

void ExynosCameraHWInterface2::StreamThread::ApplyChange()
{
    ALOGV("DEBUG(%s):", __func__);
    memcpy(&m_parameters, &m_tempParameters, sizeof(stream_parameters_t));
}

int ExynosCameraHWInterface2::createIonClient(ion_client ionClient)
{
    if (ionClient == 0) {
        ionClient = ion_client_create();
        if (ionClient < 0) {
            ALOGE("[%s]src ion client create failed, value = %d\n", __func__, ionClient);
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

int ExynosCameraHWInterface2::allocCameraMemory(ion_client ionClient, exynos_camera_memory_t *buf, int iMemoryNum)
{
    int ret = 0;
    int i = 0;

    if (ionClient == 0) {
        ALOGE("[%s] ionClient is zero (%d)\n", __func__, ionClient);
        return -1;
    }

    for (i=0;i<iMemoryNum;i++) {
        if (buf->size[i] == 0) {
            break;
        }

        buf->ionBuffer[i] = ion_alloc(ionClient, \
                                      buf->size[i], 0, ION_HEAP_EXYNOS_MASK,0);
        if ((buf->ionBuffer[i] == -1) ||(buf->ionBuffer[i] == 0)) {
            ALOGE("[%s]ion_alloc(%d) failed\n", __func__, buf->size[i]);
            buf->ionBuffer[i] = -1;
            freeCameraMemory(buf, iMemoryNum);
            return -1;
        }

        buf->virBuffer[i] = (char *)ion_map(buf->ionBuffer[i], \
                                        buf->size[i], 0);
        if ((buf->virBuffer[i] == (char *)MAP_FAILED) || (buf->virBuffer[i] == NULL)) {
            ALOGE("[%s]src ion map failed(%d)\n", __func__, buf->size[i]);
            buf->virBuffer[i] = (char *)MAP_FAILED;
            freeCameraMemory(buf, iMemoryNum);
            return -1;
        }
        ALOGV("allocCameraMem : [%d][0x%08x]", i, buf->virBuffer[i]);
    }

    return ret;
}

void ExynosCameraHWInterface2::freeCameraMemory(exynos_camera_memory_t *buf, int iMemoryNum)
{
    int i =0 ;

    for (i=0;i<iMemoryNum;i++) {
        if (buf->ionBuffer[i] != -1) {
            if (buf->virBuffer[i] != (char *)MAP_FAILED) {
                ion_unmap(buf->virBuffer[i], buf->size[i]);
            }
            ion_free(buf->ionBuffer[i]);
        }
        buf->ionBuffer[i] = -1;
        buf->virBuffer[i] = (char *)MAP_FAILED;
        buf->size[i] = 0;
    }
}

void ExynosCameraHWInterface2::initCameraMemory(exynos_camera_memory_t *buf, int iMemoryNum)
{
    int i =0 ;
    for (i=0;i<iMemoryNum;i++) {
        buf->virBuffer[i] = (char *)MAP_FAILED;
        buf->ionBuffer[i] = -1;
        buf->size[i] = 0;
    }
}


static camera2_device_t *g_cam2_device;

static int HAL2_camera_device_close(struct hw_device_t* device)
{
    ALOGV("DEBUG(%s):", __func__);
    if (device) {
        camera2_device_t *cam_device = (camera2_device_t *)device;
        delete static_cast<ExynosCameraHWInterface2 *>(cam_device->priv);
        free(cam_device);
        g_cam2_device = 0;
    }
    return 0;
}

static inline ExynosCameraHWInterface2 *obj(const struct camera2_device *dev)
{
    return reinterpret_cast<ExynosCameraHWInterface2 *>(dev->priv);
}

static int HAL2_device_set_request_queue_src_ops(const struct camera2_device *dev,
            const camera2_request_queue_src_ops_t *request_src_ops)
{
    ALOGV("DEBUG(%s):", __func__);
    return obj(dev)->setRequestQueueSrcOps(request_src_ops);
}

static int HAL2_device_notify_request_queue_not_empty(const struct camera2_device *dev)
{
    ALOGV("DEBUG(%s):", __func__);
    return obj(dev)->notifyRequestQueueNotEmpty();
}

static int HAL2_device_set_frame_queue_dst_ops(const struct camera2_device *dev,
            const camera2_frame_queue_dst_ops_t *frame_dst_ops)
{
    ALOGV("DEBUG(%s):", __func__);
    return obj(dev)->setFrameQueueDstOps(frame_dst_ops);
}

static int HAL2_device_get_in_progress_count(const struct camera2_device *dev)
{
    ALOGV("DEBUG(%s):", __func__);
    return obj(dev)->getInProgressCount();
}

static int HAL2_device_flush_captures_in_progress(const struct camera2_device *dev)
{
    ALOGV("DEBUG(%s):", __func__);
    return obj(dev)->flushCapturesInProgress();
}

static int HAL2_device_construct_default_request(const struct camera2_device *dev,
            int request_template, camera_metadata_t **request)
{
    ALOGV("DEBUG(%s):", __func__);
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
    ALOGV("DEBUG(%s):", __func__);
    return obj(dev)->allocateStream(width, height, format, stream_ops,
                                    stream_id, format_actual, usage, max_buffers);
}


static int HAL2_device_register_stream_buffers(const struct camera2_device *dev,
            uint32_t stream_id,
            int num_buffers,
            buffer_handle_t *buffers)
{
    ALOGV("DEBUG(%s):", __func__);
    return obj(dev)->registerStreamBuffers(stream_id, num_buffers, buffers);
}

static int HAL2_device_release_stream(
        const struct camera2_device *dev,
            uint32_t stream_id)
{
    ALOGV("DEBUG(%s):", __func__);
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
    ALOGV("DEBUG(%s):", __func__);
    return obj(dev)->allocateReprocessStream(width, height, format, reprocess_stream_ops,
                                    stream_id, consumer_usage, max_buffers);
}

static int HAL2_device_release_reprocess_stream(
        const struct camera2_device *dev,
            uint32_t stream_id)
{
    ALOGV("DEBUG(%s):", __func__);
    return obj(dev)->releaseReprocessStream(stream_id);
}

static int HAL2_device_trigger_action(const struct camera2_device *dev,
           uint32_t trigger_id,
            int ext1,
            int ext2)
{
    ALOGV("DEBUG(%s):", __func__);
    return obj(dev)->triggerAction(trigger_id, ext1, ext2);
}

static int HAL2_device_set_notify_callback(const struct camera2_device *dev,
            camera2_notify_callback notify_cb,
            void *user)
{
    ALOGV("DEBUG(%s):", __func__);
    return obj(dev)->setNotifyCallback(notify_cb, user);
}

static int HAL2_device_get_metadata_vendor_tag_ops(const struct camera2_device*dev,
            vendor_tag_query_ops_t **ops)
{
    ALOGV("DEBUG(%s):", __func__);
    return obj(dev)->getMetadataVendorTagOps(ops);
}

static int HAL2_device_dump(const struct camera2_device *dev, int fd)
{
    ALOGV("DEBUG(%s):", __func__);
    return obj(dev)->dump(fd);
}





static int HAL2_getNumberOfCameras()
{
    ALOGV("DEBUG(%s):", __func__);
    return 1;
}


// temporarily copied from EmulatedFakeCamera2
// TODO : implement our own codes
status_t constructStaticInfo(
        camera_metadata_t **info,
        bool sizeRequest);

static int HAL2_getCameraInfo(int cameraId, struct camera_info *info)
{
    ALOGV("DEBUG(%s):", __func__);
    static camera_metadata_t * mCameraInfo = NULL;
    status_t res;
    
    info->facing = CAMERA_FACING_BACK;
    info->orientation = 0;
    info->device_version = HARDWARE_DEVICE_API_VERSION(2, 0);
    if (mCameraInfo==NULL) {
        res = constructStaticInfo(&mCameraInfo, true);
        if (res != OK) {
            ALOGE("%s: Unable to allocate static info: %s (%d)",
                    __func__, strerror(-res), res);
            return res;
        }
        res = constructStaticInfo(&mCameraInfo, false);
        if (res != OK) {
            ALOGE("%s: Unable to fill in static info: %s (%d)",
                    __func__, strerror(-res), res);
            return res;
        }
    }
    info->static_camera_characteristics = mCameraInfo;
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
    ALOGD(">>> I'm Samsung's CameraHAL_2 <<<");

    int cameraId = atoi(id);
    if (cameraId < 0 || cameraId >= HAL2_getNumberOfCameras()) {
        ALOGE("ERR(%s):Invalid camera ID %s", __func__, id);
        return -EINVAL;
    }

    if (g_cam2_device) {
        if (obj(g_cam2_device)->getCameraId() == cameraId) {
            ALOGV("DEBUG(%s):returning existing camera ID %s", __func__, id);
            goto done;
        } else {
            ALOGE("ERR(%s):Cannot open camera %d. camera %d is already running!",
                    __func__, cameraId, obj(g_cam2_device)->getCameraId());
            return -ENOSYS;
        }
    }

    g_cam2_device = (camera2_device_t *)malloc(sizeof(camera2_device_t));
    if (!g_cam2_device)
        return -ENOMEM;

    g_cam2_device->common.tag     = HARDWARE_DEVICE_TAG;
    g_cam2_device->common.version = CAMERA_DEVICE_API_VERSION_2_0;
    g_cam2_device->common.module  = const_cast<hw_module_t *>(module);
    g_cam2_device->common.close   = HAL2_camera_device_close;

    g_cam2_device->ops = &camera2_device_ops;

    ALOGV("DEBUG(%s):open camera2 %s", __func__, id);

    g_cam2_device->priv = new ExynosCameraHWInterface2(cameraId, g_cam2_device);

done:
    *device = (hw_device_t *)g_cam2_device;
    ALOGV("DEBUG(%s):opened camera2 %s (%p)", __func__, id, *device);

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
