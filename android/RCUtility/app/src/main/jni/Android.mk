
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := sensor-fusion
LOCAL_SRC_FILES := sensor-fusion.cpp
LOCAL_LDLIBS    := -llog -landroid

include $(BUILD_SHARED_LIBRARY)
