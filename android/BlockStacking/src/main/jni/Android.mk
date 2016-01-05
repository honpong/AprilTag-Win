MY_LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_PATH := $(MY_LOCAL_PATH)
LOCAL_SDK_VERSION := 21
LOCAL_MODULE    := sceneperception_sample
LOCAL_SRC_FILES := com_intel_examples_depth_spsample_spbasicfragment.cpp
LOCAL_LDFLAGS 	:= -llog -landroid

# Security Requirement: Add the defense flags
LOCAL_LDFLAGS   += -z noexecstack
LOCAL_LDFLAGS   += -z relro
LOCAL_LDFLAGS   += -z now 

# Security Requirement: Add the defense flags
LOCAL_CFLAGS    += -fstack-protector
LOCAL_CFLAGS   += -fPIE -fPIC
LOCAL_CFLAGS   += -pie
LOCAL_CFLAGS   += -O2 -D_FORTIFY_SOURCE=2
LOCAL_CFLAGS   += -Wformat -Wformat-security

LOCAL_CPPFLAGS += -std=gnu++11 -Werror -Wformat -Wformat-security -fexceptions -frtti -s 

# Security Requirement: Add the defense flags
LOCAL_CPPFLAGS    += -fstack-protector
LOCAL_CPPFLAGS   += -fPIE -fPIC
LOCAL_CPPFLAGS   += -pie
LOCAL_CPPFLAGS   += -O2 -D_FORTIFY_SOURCE=2

include $(BUILD_SHARED_LIBRARY)
