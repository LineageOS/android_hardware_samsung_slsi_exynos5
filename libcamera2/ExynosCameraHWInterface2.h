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
 * \date      2012/05/31
 *
 * <b>Revision History: </b>
 * - 2012/05/31 : Sungjoong Kang(sj3.kang@samsung.com) \n
 *   Initial Release
 */
 
#ifndef EXYNOS_CAMERA_HW_INTERFACE_2_H
#define EXYNOS_CAMERA_HW_INTERFACE_2_H

#include <hardware/camera2.h>
#include <camera/Camera.h>
#include <camera/CameraParameters.h>
#include "SignalDrivenThread.h"
#include "MetadataConverter.h"
#include "exynos_v4l2.h"
#include "videodev2_exynos_camera.h"
#include "gralloc_priv.h"

#include <fcntl.h>
#include "fimc-is-metadata.h"
#include "ion.h"

namespace android {


#define NODE_PREFIX     "/dev/video"

#define NUM_MAX_STREAM_THREAD       (5)
#define NUM_MAX_DEQUEUED_REQUEST    (4)
#define NUM_MAX_REQUEST_MGR_ENTRY   NUM_MAX_DEQUEUED_REQUEST
#define NUM_OF_STREAM_BUF           (15)
#define MAX_CAMERA_MEMORY_PLANE_NUM	(4)

#define SIGNAL_MAIN_REQ_Q_NOT_EMPTY             (SIGNAL_THREAD_COMMON_LAST<<1) 
#define SIGNAL_MAIN_REPROCESS_Q_NOT_EMPTY       (SIGNAL_THREAD_COMMON_LAST<<2) 
#define SIGNAL_MAIN_STREAM_OUTPUT_DONE          (SIGNAL_THREAD_COMMON_LAST<<3) 
#define SIGNAL_SENSOR_START_REQ_PROCESSING      (SIGNAL_THREAD_COMMON_LAST<<4) 
#define SIGNAL_STREAM_GET_BUFFER                (SIGNAL_THREAD_COMMON_LAST<<5)
#define SIGNAL_STREAM_PUT_BUFFER                (SIGNAL_THREAD_COMMON_LAST<<6)
#define SIGNAL_STREAM_CHANGE_PARAMETER          (SIGNAL_THREAD_COMMON_LAST<<7)


#define SIGNAL_STREAM_DATA_COMING               (SIGNAL_THREAD_COMMON_LAST<<15)


#define MAX_NUM_CAMERA_BUFFERS (16)
enum sensor_name {
    SENSOR_NAME_S5K3H2  = 1,
    SENSOR_NAME_S5K6A3  = 2,
    SENSOR_NAME_S5K4E5  = 3,
    SENSOR_NAME_S5K3H7  = 4,
    SENSOR_NAME_CUSTOM  = 5,
    SENSOR_NAME_END
};

typedef struct exynos_camera_memory {
	ion_buffer ionBuffer[MAX_CAMERA_MEMORY_PLANE_NUM];
	char *virBuffer[MAX_CAMERA_MEMORY_PLANE_NUM];
	int size[MAX_CAMERA_MEMORY_PLANE_NUM];
} exynos_camera_memory_t;

typedef struct node_info {
    int fd;
    int width;
    int height;
    int format;
    int planes;
    int buffers;
    int currentBufferIndex;
    enum v4l2_memory memory;
    enum v4l2_buf_type type;
	ion_client ionClient;
	exynos_camera_memory_t	buffer[MAX_NUM_CAMERA_BUFFERS];
} node_info_t;


typedef struct camera_hw_info {
    int sensor_id;

    node_info_t sensor;
    node_info_t isp;   
    node_info_t capture;   
    node_info_t preview; 
    

    /*shot*/
    camera2_shot_t current_shot;
} camera_hw_info_t;

typedef enum request_entry_status
{
    EMPTY,
    REGISTERED,
    PROCESSING
} request_entry_status_t;

typedef struct request_manager_entry {
    request_entry_status_t  status;
    int                     id;
    camera_metadata_t       *original_request;
    // TODO : allocate memory dynamically
    // camera2_ctl_metadata_t  *internal_request;
    camera2_ctl_metadata_NEW_t  internal_request;
    int                     output_stream_count;
} request_manager_entry_t;

class RequestManager {
public:
    RequestManager(SignalDrivenThread* main_thread);
    ~RequestManager();
    int     GetNumEntries();
    bool    IsRequestQueueFull();
    
    void    RegisterRequest(camera_metadata_t * new_request);
    void    DeregisterRequest(camera_metadata_t ** deregistered_request);
    void    PrepareFrame(size_t* num_entries, size_t* frame_size, 
                camera_metadata_t ** prepared_frame);
    void    MarkProcessingRequest(exynos_camera_memory_t* buf);
    void    NotifyStreamOutput(uint32_t stream_id);
    
private:

    MetadataConverter               *m_metadataConverter;
    SignalDrivenThread              *m_mainThread;
    int                             m_numOfEntries;
    int                             m_entryInsertionIndex;
    int                             m_entryProcessingIndex;
    int                             m_entryFrameOutputIndex;
    request_manager_entry_t         entries[NUM_MAX_REQUEST_MGR_ENTRY];

    Mutex                           m_requestMutex;

    //TODO : alloc dynamically
    char                            m_tempFrameMetadataBuf[2000];
    camera_metadata_t               *m_tempFrameMetadata;
    int32_t             frame_seq_number;
};

typedef struct stream_parameters
{
            uint32_t                id;
            uint32_t                width;
            uint32_t                height;
            int                     format;            
    const   camera2_stream_ops_t*   streamOps;
            uint32_t                usage;
            uint32_t                max_buffers;
            int                     fd;
            void                    *grallocVirtAddr[NUM_OF_STREAM_BUF];
            bool                    availableBufHandle[NUM_OF_STREAM_BUF];
            buffer_handle_t         *bufHandle[NUM_OF_STREAM_BUF];
            node_info_t             *node;
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
            SignalDrivenThread("MainThread", PRIORITY_DEFAULT, 0),
            mHardware(hw) { }
        virtual void onFirstRef() {
        }
        status_t readyToRunInternal() {
            return NO_ERROR;
        }
        void threadLoopInternal() {
            mHardware->m_mainThreadFunc(this);
            return;
        }
    };

    class SensorThread : public SignalDrivenThread {
        ExynosCameraHWInterface2 *mHardware;
    public:
        SensorThread(ExynosCameraHWInterface2 *hw):
            SignalDrivenThread("SensorThread", PRIORITY_DEFAULT, 0),
            mHardware(hw),
            m_isBayerOutputEnabled(false) { }
        virtual void onFirstRef() {
            mHardware->m_sensorThreadInitialize(this);
        }
        status_t readyToRunInternal() {
            mHardware->m_sensorThreadInitialize(this);            
            return NO_ERROR;
        }
        void threadLoopInternal() {
            mHardware->m_sensorThreadFunc(this);
            return;
        }
    //private:
        bool            m_isBayerOutputEnabled;
        int             m_sensorFd;
        int             m_ispFd;
    };

    class StreamThread : public SignalDrivenThread {
        ExynosCameraHWInterface2 *mHardware;
    public:
        StreamThread(ExynosCameraHWInterface2 *hw, uint8_t new_index):
            SignalDrivenThread("StreamThread", PRIORITY_DEFAULT, 0),
            mHardware(hw),
            m_index(new_index) { }
        virtual void onFirstRef() {
            mHardware->m_streamThreadInitialize(this);            
        }
        status_t readyToRunInternal() {
            mHardware->m_streamThreadInitialize(this);             
            return NO_ERROR;
        }
        void threadLoopInternal() {
            mHardware->m_streamThreadFunc(this);
            return;
        }
        void SetParameter(uint32_t id, uint32_t width, uint32_t height, int format,
                    const camera2_stream_ops_t* stream_ops, uint32_t usage, int fd, node_info_t * node);
        void ApplyChange(void);

                uint8_t                 m_index;
    //private:                
        stream_parameters_t             m_parameters;
        stream_parameters_t             m_tempParameters; 

    };    

    sp<MainThread>      m_mainThread;
    sp<SensorThread>    m_sensorThread;
    sp<StreamThread>    m_streamThread;



    RequestManager      *m_requestManager;

    void                m_mainThreadFunc(SignalDrivenThread * self);
    void                m_sensorThreadFunc(SignalDrivenThread * self);
    void                m_sensorThreadInitialize(SignalDrivenThread * self);    
    void                m_streamThreadFunc(SignalDrivenThread * self);
    void                m_streamThreadInitialize(SignalDrivenThread * self);      

	int                 createIonClient(ion_client ionClient);
	int                 deleteIonClient(ion_client ionClient);
    int                 allocCameraMemory(ion_client ionClient, exynos_camera_memory_t *buf, int iMemoryNum);
	void                freeCameraMemory(exynos_camera_memory_t *buf, int iMemoryNum);
	void                initCameraMemory(exynos_camera_memory_t *buf, int iMemoryNum);

    camera2_request_queue_src_ops_t     *m_requestQueueOps;
    camera2_frame_queue_dst_ops_t       *m_frameQueueOps;
    camera2_notify_callback             m_notifyCb;
    void                                *m_callbackCookie;

    int                                 m_numOfRemainingReqInSvc;
    bool                                m_isRequestQueuePending;

    camera2_device_t *m_halDevice;
    static gralloc_module_t const* m_grallocHal;


    camera_hw_info_t m_camera_info;

	ion_client m_ionCameraClient;

    bool        m_isSensorThreadOn;
    bool        m_isStreamStarted;
    int         matchBuffer(void * bufAddr);
    bool        m_isBufferInit;
};

}; // namespace android

#endif
