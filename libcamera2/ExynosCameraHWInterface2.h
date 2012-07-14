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
 * \file      ExynosCameraHWInterface2.h
 * \brief     header file for Android Camera API 2.0 HAL
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

#ifndef EXYNOS_CAMERA_HW_INTERFACE_2_H
#define EXYNOS_CAMERA_HW_INTERFACE_2_H

#include <hardware/camera2.h>
#include <camera/Camera.h>
#include <camera/CameraParameters.h>
#include "SignalDrivenThread.h"
#include "MetadataConverter.h"
#include "exynos_v4l2.h"
#include "ExynosRect.h"
#include "ExynosBuffer.h"
#include "videodev2_exynos_camera.h"
#include "gralloc_priv.h"
#include "ExynosJpegEncoderForCamera.h"
#include <fcntl.h>
#include "fimc-is-metadata.h"
#include "ion.h"
#include "ExynosExif.h"
#include "csc.h"

namespace android {


#define NODE_PREFIX     "/dev/video"

#define NUM_MAX_STREAM_THREAD       (5)
#define NUM_MAX_DEQUEUED_REQUEST    (8)
/* #define NUM_MAX_REQUEST_MGR_ENTRY   NUM_MAX_DEQUEUED_REQUEST */
#define NUM_MAX_REQUEST_MGR_ENTRY   (10)
/* #define NUM_OF_STREAM_BUF           (15) */
#define MAX_CAMERA_MEMORY_PLANE_NUM	(4)
#define NUM_MAX_CAMERA_BUFFERS      (16)
#define NUM_BAYER_BUFFERS           (8)
#define SHOT_FRAME_DELAY            (3)

#define PICTURE_GSC_NODE_NUM (2)

#define SIGNAL_MAIN_REQ_Q_NOT_EMPTY             (SIGNAL_THREAD_COMMON_LAST<<1)
#define SIGNAL_MAIN_REPROCESS_Q_NOT_EMPTY       (SIGNAL_THREAD_COMMON_LAST<<2)
#define SIGNAL_MAIN_STREAM_OUTPUT_DONE          (SIGNAL_THREAD_COMMON_LAST<<3)
#define SIGNAL_SENSOR_START_REQ_PROCESSING      (SIGNAL_THREAD_COMMON_LAST<<4)
#define SIGNAL_STREAM_GET_BUFFER                (SIGNAL_THREAD_COMMON_LAST<<5)
#define SIGNAL_STREAM_PUT_BUFFER                (SIGNAL_THREAD_COMMON_LAST<<6)
#define SIGNAL_STREAM_CHANGE_PARAMETER          (SIGNAL_THREAD_COMMON_LAST<<7)
#define SIGNAL_THREAD_RELEASE                   (SIGNAL_THREAD_COMMON_LAST<<8)
#define SIGNAL_ISP_START_BAYER_INPUT            (SIGNAL_THREAD_COMMON_LAST<<9)

#define SIGNAL_STREAM_DATA_COMING               (SIGNAL_THREAD_COMMON_LAST<<15)



enum sensor_name {
    SENSOR_NAME_S5K3H2  = 1,
    SENSOR_NAME_S5K6A3  = 2,
    SENSOR_NAME_S5K4E5  = 3,
    SENSOR_NAME_S5K3H7  = 4,
    SENSOR_NAME_CUSTOM  = 5,
    SENSOR_NAME_END
};

/*
typedef struct exynos_camera_memory {
	ion_buffer ionBuffer[MAX_CAMERA_MEMORY_PLANE_NUM];
	char *virBuffer[MAX_CAMERA_MEMORY_PLANE_NUM];
	int size[MAX_CAMERA_MEMORY_PLANE_NUM];
} exynos_camera_memory_t;
*/

typedef struct node_info {
    int fd;
    int width;
    int height;
    int format;
    int planes;
    int buffers;
    //int currentBufferIndex;
    enum v4l2_memory memory;
    enum v4l2_buf_type type;
	ion_client ionClient;
	ExynosBuffer	buffer[NUM_MAX_CAMERA_BUFFERS];
} node_info_t;


typedef struct camera_hw_info {
    int sensor_id;
    //int sensor_frame_count; // includes bubble

    node_info_t sensor;
    node_info_t isp;
    node_info_t capture;

    /*shot*/  // temp
    struct camera2_shot_ext dummy_shot;

} camera_hw_info_t;

typedef enum request_entry_status {
    EMPTY,
    REGISTERED,
    PROCESSING
} request_entry_status_t;

typedef struct request_manager_entry {
    request_entry_status_t      status;
    //int                         id;
    camera_metadata_t           *original_request;
    // TODO : allocate memory dynamically
    // camera2_ctl_metadata_t  *internal_request;
    camera2_ctl_metadata_NEW_t  internal_shot;
    int                         output_stream_count;
    bool                         dynamic_meta_vaild;
    //int                         request_serial_number;
} request_manager_entry_t;

class RequestManager {
public:
    RequestManager(SignalDrivenThread* main_thread);
    ~RequestManager();
    int     GetNumEntries();
    bool    IsRequestQueueFull();

    void    RegisterRequest(camera_metadata_t *new_request);
    void    DeregisterRequest(camera_metadata_t **deregistered_request);
    bool    PrepareFrame(size_t *num_entries, size_t *frame_size,
                camera_metadata_t **prepared_frame);
    //void    MarkProcessingRequest(exynos_camera_memory_t* buf);
    //void    MarkProcessingRequest(ExynosBuffer* buf);
    int   MarkProcessingRequest(ExynosBuffer *buf);
    //void    NotifyStreamOutput(uint32_t stream_id, int isp_processing_index);
    //void      NotifyStreamOutput(ExynosBuffer* buf, uint32_t stream_id);
    void      NotifyStreamOutput(int index, int stream_id);
    //int     FindEntryIndexByRequestSerialNumber(int serial_num);
    void    DumpInfoWithIndex(int index);
    void    ApplyDynamicMetadata(int index);
    void    CheckCompleted(int index);
    void    UpdateOutputStreamInfo(struct camera2_shot_ext *shot_ext, int index);
    void    RegisterTimestamp(int index, nsecs_t *frameTime);
    uint64_t  GetTimestamp(int index);
    void  Dump(void);
private:

    MetadataConverter               *m_metadataConverter;
    SignalDrivenThread              *m_mainThread;
    int                             m_numOfEntries;
    int                             m_entryInsertionIndex;
    int                             m_entryProcessingIndex;
    int                             m_entryFrameOutputIndex;
    request_manager_entry_t         entries[NUM_MAX_REQUEST_MGR_ENTRY];
    int                             m_completedIndex;

    Mutex                           m_requestMutex;

    //TODO : alloc dynamically
    char                            m_tempFrameMetadataBuf[2000];
    camera_metadata_t               *m_tempFrameMetadata;
    //int32_t                         m_request_serial_number;
};

#define NOT_AVAILABLE           (0)
#define REQUIRES_DQ_FROM_SVC    (1)
#define ON_DRIVER               (2)
#define ON_HAL                  (3)
#define ON_SERVICE              (4)

#define BAYER_NOT_AVAILABLE     (0)
#define BAYER_ON_SENSOR         (1)
#define BAYER_ON_HAL_FILLED     (2)
#define BAYER_ON_ISP            (3)
#define BAYER_ON_SERVICE        (4)
#define BAYER_ON_HAL_EMPTY      (5)

typedef struct stream_parameters {
            int                     streamType;
            uint32_t                outputWidth;
            uint32_t                outputHeight;
            uint32_t                nodeWidth;
            uint32_t                nodeHeight;
            int                     outputFormat;
            int                     nodeFormat;
    const   camera2_stream_ops_t*   streamOps;
            uint32_t                usage;
            int                     numHwBuffers;
            int                     numSvcBuffers;
            int                     fd;
            int                     svcPlanes;
            int                     nodePlanes;
    enum v4l2_memory                memory;
    enum v4l2_buf_type              halBuftype;

            buffer_handle_t         svcBufHandle[NUM_MAX_CAMERA_BUFFERS];
            ExynosBuffer            svcBuffers[NUM_MAX_CAMERA_BUFFERS];
            int                     svcBufStatus[NUM_MAX_CAMERA_BUFFERS];

            //buffer_handle_t         halBufHandle[NUM_MAX_CAMERA_BUFFERS];
            //ExynosBuffer            halBuffers[NUM_MAX_CAMERA_BUFFERS];
            //int                     halBufStatus[NUM_MAX_CAMERA_BUFFERS];
	        ion_client              ionClient;
            node_info_t             node;
} stream_parameters_t;


class ExynosCameraHWInterface2 : public virtual RefBase {
public:
    ExynosCameraHWInterface2(int cameraId, camera2_device_t *dev);
    virtual             ~ExynosCameraHWInterface2();

    virtual void        release();

    inline  int         getCameraId() const;

    virtual int         setRequestQueueSrcOps(const camera2_request_queue_src_ops_t *request_src_ops);
    virtual int         notifyRequestQueueNotEmpty();
    virtual int         setFrameQueueDstOps(const camera2_frame_queue_dst_ops_t *frame_dst_ops);
    virtual int         getInProgressCount();
    virtual int         flushCapturesInProgress();
    virtual int         constructDefaultRequest(int request_template, camera_metadata_t **request);
    virtual int         allocateStream(uint32_t width, uint32_t height,
                                    int format, const camera2_stream_ops_t *stream_ops,
                                    uint32_t *stream_id, uint32_t *format_actual, uint32_t *usage, uint32_t *max_buffers);
    virtual int         registerStreamBuffers(uint32_t stream_id, int num_buffers, buffer_handle_t *buffers);
    virtual int         releaseStream(uint32_t stream_id);
    virtual int         allocateReprocessStream(uint32_t width, uint32_t height,
                                    uint32_t format, const camera2_stream_in_ops_t *reprocess_stream_ops,
                                    uint32_t *stream_id, uint32_t *consumer_usage, uint32_t *max_buffers);
    virtual int         releaseReprocessStream(uint32_t stream_id);
    virtual int         triggerAction(uint32_t trigger_id, int ext1, int ext2);
    virtual int         setNotifyCallback(camera2_notify_callback notify_cb, void *user);
    virtual int         getMetadataVendorTagOps(vendor_tag_query_ops_t **ops);
    virtual int         dump(int fd);
private:
class MainThread : public SignalDrivenThread {
        ExynosCameraHWInterface2 *mHardware;
    public:
        MainThread(ExynosCameraHWInterface2 *hw):
            SignalDrivenThread(),
            mHardware(hw) {
//            Start("MainThread", PRIORITY_DEFAULT, 0);
        }
        ~MainThread();
        status_t readyToRunInternal()
	{
            return NO_ERROR;
        }
        void threadFunctionInternal()
	{
            mHardware->m_mainThreadFunc(this);
            return;
        }
        void        release(void);
    };
/*
    class MainThread : public SignalDrivenThread {
        ExynosCameraHWInterface2 *mHardware;
    public:
        MainThread(ExynosCameraHWInterface2 *hw):
            SignalDrivenThread("MainThread", PRIORITY_DEFAULT, 0),
            mHardware(hw) { }
        ~MainThread();
        status_t readyToRunInternal() {
            return NO_ERROR;
        }
        void threadFunctionInternal() {
            mHardware->m_mainThreadFunc(this);
            return;
        }
        void        release(void);
    };
*/
    class SensorThread : public SignalDrivenThread {
        ExynosCameraHWInterface2 *mHardware;
    public:
        SensorThread(ExynosCameraHWInterface2 *hw):
            SignalDrivenThread("SensorThread", PRIORITY_DEFAULT, 0),
            mHardware(hw),
            m_isBayerOutputEnabled(false) { }
        ~SensorThread();
        status_t readyToRunInternal() {
            mHardware->m_sensorThreadInitialize(this);
            return NO_ERROR;
        }
        void threadFunctionInternal() {
            mHardware->m_sensorThreadFunc(this);
            return;
        }
        void            release(void);	
    //private:
        bool            m_isBayerOutputEnabled;
        int             m_sensorFd;
    };

    class IspThread : public SignalDrivenThread {
        ExynosCameraHWInterface2 *mHardware;
    public:
        IspThread(ExynosCameraHWInterface2 *hw):
            SignalDrivenThread("IspThread", PRIORITY_DEFAULT, 0),
            mHardware(hw) { }
        ~IspThread();
        status_t readyToRunInternal() {
            mHardware->m_ispThreadInitialize(this);
            return NO_ERROR;
        }
        void threadFunctionInternal() {
            mHardware->m_ispThreadFunc(this);
            return;
        }
        void            release(void);
    //private:
        int             m_ispFd;
    };

    class StreamThread : public SignalDrivenThread {
        ExynosCameraHWInterface2 *mHardware;
    public:
        StreamThread(ExynosCameraHWInterface2 *hw, uint8_t new_index):
            SignalDrivenThread("StreamThread", PRIORITY_DEFAULT, 0),
            mHardware(hw),
            m_index(new_index) { }
        ~StreamThread();
        status_t readyToRunInternal() {
            mHardware->m_streamThreadInitialize(this);
            return NO_ERROR;
        }
        void threadFunctionInternal() {
            mHardware->m_streamThreadFunc(this);
            return;
        }
        void        setParameter(stream_parameters_t * new_parameters);
        void        applyChange(void);
        void        release(void);
        int         findBufferIndex(void * bufAddr);


        uint8_t                         m_index;
    //private:
        stream_parameters_t             m_parameters;
        stream_parameters_t             *m_tempParameters; 
        bool                            m_isBufferInit;
     };

    sp<MainThread>      m_mainThread;
    sp<SensorThread>    m_sensorThread;
    sp<IspThread>       m_ispThread;
    sp<StreamThread>    m_streamThreads[NUM_MAX_STREAM_THREAD];


    int                 m_bayerBufStatus[NUM_BAYER_BUFFERS];
    int                 m_bayerQueueList[NUM_BAYER_BUFFERS+SHOT_FRAME_DELAY];
    int                 m_bayerQueueRequestList[NUM_BAYER_BUFFERS+SHOT_FRAME_DELAY];
    int                 m_bayerDequeueList[NUM_BAYER_BUFFERS];
    int                 m_numBayerQueueList;
    int                 m_numBayerQueueListRemainder;
    int                 m_numBayerDequeueList;

    void                RegisterBayerQueueList(int bufIndex, int requestIndex);
    void                DeregisterBayerQueueList(int bufIndex);
    void                RegisterBayerDequeueList(int bufIndex);
    int                 DeregisterBayerDequeueList(void);
    int                 FindRequestEntryNumber(int bufIndex);
    void                DumpFrameinfoWithBufIndex(int bufIndex);
    
    RequestManager      *m_requestManager;

    void                m_mainThreadFunc(SignalDrivenThread * self);
    void                m_sensorThreadFunc(SignalDrivenThread * self);
    void                m_sensorThreadInitialize(SignalDrivenThread * self);
    void                m_ispThreadFunc(SignalDrivenThread * self);
    void                m_ispThreadInitialize(SignalDrivenThread * self);
    void                m_streamThreadFunc(SignalDrivenThread * self);
    void                m_streamThreadInitialize(SignalDrivenThread * self);

    void                m_getAlignedYUVSize(int colorFormat, int w, int h,
                                                ExynosBuffer *buf);
    bool                m_getRatioSize(int  src_w,  int   src_h,
                                             int  dst_w,  int   dst_h,
                                             int *crop_x, int *crop_y,
                                             int *crop_w, int *crop_h,
                                             int zoom);
	int				createIonClient(ion_client ionClient);
	int					deleteIonClient(ion_client ionClient);
    //int				allocCameraMemory(ion_client ionClient, exynos_camera_memory_t *buf, int iMemoryNum);
	//void				freeCameraMemory(exynos_camera_memory_t *buf, int iMemoryNum);
	//void				initCameraMemory(exynos_camera_memory_t *buf, int iMemoryNum);

    int				allocCameraMemory(ion_client ionClient, ExynosBuffer *buf, int iMemoryNum);
	void				freeCameraMemory(ExynosBuffer *buf, int iMemoryNum);
	void				initCameraMemory(ExynosBuffer *buf, int iMemoryNum);

    void            DumpInfoWithShot(struct camera2_shot_ext * shot_ext);
    bool            yuv2Jpeg(ExynosBuffer *yuvBuf,
                            ExynosBuffer *jpegBuf,
                            ExynosRect *rect);
    exif_attribute_t    mExifInfo;
    void               *m_exynosPictureCSC;

    int                 m_jpegEncodingRequestIndex;

    camera2_request_queue_src_ops_t     *m_requestQueueOps;
    camera2_frame_queue_dst_ops_t       *m_frameQueueOps;
    camera2_notify_callback             m_notifyCb;
    void                                *m_callbackCookie;

    int                                 m_numOfRemainingReqInSvc;
    bool                                m_isRequestQueuePending;
    bool                                m_isRequestQueueNull;
    camera2_device_t                    *m_halDevice;
    static gralloc_module_t const*      m_grallocHal;


    camera_hw_info_t                     m_camera_info;

	ion_client m_ionCameraClient;

    bool                                m_isSensorThreadOn;
    bool                                m_isSensorStarted;



    bool                                m_initFlag1;
    bool                                m_initFlag2;
    int                                 m_ispInputIndex;
    int                                 m_ispProcessingIndex;
    int                                 m_ispThreadProcessingReq;
    int                                 m_processingRequest;

    int                                 m_numExpRemainingOutScp;
    int                                 m_numExpRemainingOutScc;

    int                                 indexToQueue[3+1];
    int                                 m_fd_scp;

    bool                                m_scp_flushing;
    bool                                m_closing;
    ExynosBuffer                        m_resizeBuf;
    int                                 m_svcBufIndex;
    nsecs_t                             m_lastTimeStamp;
};

}; // namespace android

#endif
