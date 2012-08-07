# =============================================================================
#
# Makefile pointing to all makefiles within the project.
#
# =============================================================================
MOBICORE_PROJECT_PATH := $(call my-dir)
# Setup common variables
COMP_PATH_Logwrapper := $(MOBICORE_PROJECT_PATH)/common/LogWrapper
COMP_PATH_MobiCore := $(MOBICORE_PROJECT_PATH)/common/MobiCore
COMP_PATH_MobiCoreDriverMod := $(MOBICORE_PROJECT_PATH)/include


# Include the Daemon
include $(MOBICORE_PROJECT_PATH)/daemon/Android.mk

MC_INCLUDE_DIR := $(COMP_PATH_MobiCore)/inc \
    $(COMP_PATH_MobiCore)/inc/TlCm \
    $(MOBICORE_PROJECT_PATH)/daemon/ClientLib/public \
    $(MOBICORE_PROJECT_PATH)/daemon/Registry/Public

MC_DEBUG := _DEBUG

# Include the provisioning lib
include $(MOBICORE_PROJECT_PATH)/provlib/Android.mk

