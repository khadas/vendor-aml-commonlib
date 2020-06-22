LOCAL_PATH      := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := libaml_log
LOCAL_SRC_FILES := aml_log.c
LOCAL_COPY_HEADERS := aml_log.h
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)
