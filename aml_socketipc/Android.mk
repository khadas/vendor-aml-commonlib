LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_CFLAGS := -fPIC
LOCAL_ALLOW_UNDEFINED_SYMBOLS := true
LOCAL_MODULE    := libaml_socketipc
LOCAL_SRC_FILES := aml_socketipc.c
LOCAL_COPY_HEADERS := aml_socketipc.h
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)
