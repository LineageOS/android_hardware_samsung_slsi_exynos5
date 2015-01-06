LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CLANG_CFLAGS += \
	-Wno-int-conversion \
	-Wno-incompatible-pointer-types

LOCAL_SRC_FILES := \
	dec/src/ExynosVideoDecoder.c \
	enc/src/ExynosVideoEncoder.c

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	hardware/samsung_slsi/exynos5/include \
	hardware/samsung_slsi/exynos5/exynos_omx/openmax/exynos_omx/include/khronos

LOCAL_MODULE := libExynosVideoApi
LOCAL_MODULE_TAGS := optional
LOCAL_ARM_MODE := arm

include $(BUILD_STATIC_LIBRARY)
