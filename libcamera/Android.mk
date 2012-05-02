LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

# HAL module implemenation stored in
# hw/<COPYPIX_HARDWARE_MODULE_ID>.<ro.product.board>.so
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR_SHARED_LIBRARIES)/hw

LOCAL_C_INCLUDES += \
	hardware/samsung_slsi/exynos5/include \
	frameworks/native/include

LOCAL_SRC_FILES:= \
	ExynosCamera.cpp \
	ExynosJpegEncoderForCamera.cpp \
	ExynosCameraHWInterface.cpp

LOCAL_SHARED_LIBRARIES:= libutils libcutils libbinder liblog libcamera_client libhardware

LOCAL_CFLAGS += -DGAIA_FW_BETA

LOCAL_SHARED_LIBRARIES += libexynosutils libhwjpeg libexynosv4l2 libcsc libion_exynos

LOCAL_MODULE := camera.exynos5

LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
