LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	csc.c

OMX_NAME := exynos

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../exynos_omx/openmax/$(OMX_NAME)_omx/include/khronos \
	$(LOCAL_PATH)/../exynos_omx/openmax/$(OMX_NAME)_omx/include/$(OMX_NAME) \
	$(LOCAL_PATH)/../libexynosutils

LOCAL_CFLAGS :=

LOCAL_MODULE := libcsc

LOCAL_PRELINK_MODULE := false

LOCAL_ARM_MODE := arm

LOCAL_STATIC_LIBRARIES := libswconverter
LOCAL_SHARED_LIBRARIES := liblog libexynosutils

LOCAL_CFLAGS += -DUSE_SAMSUNG_COLORFORMAT

ifeq ($(TARGET_BOARD_PLATFORM), exynos5)
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../include
LOCAL_CFLAGS += -DUSE_GSCALER
LOCAL_SHARED_LIBRARIES += libexynosgscaler
endif

LOCAL_CFLAGS += -DUSE_ION
LOCAL_SHARED_LIBRARIES += libion_exynos

LOCAL_CFLAGS += -DEXYNOS_OMX

include $(BUILD_SHARED_LIBRARY)
