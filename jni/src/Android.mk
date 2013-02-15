LOCAL_PATH := $(call my-dir)


PREBUILT_PATH := ../prebuilt/android_ics
include $(CLEAR_VARS)
LOCAL_MODULE := SURFACEFLINGER
LOCAL_SRC_FILES := $(PREBUILT_PATH)/libsurfaceflinger.so 
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := SKIA
LOCAL_SRC_FILES := $(PREBUILT_PATH)/libskia.so 
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := GUI
LOCAL_SRC_FILES := $(PREBUILT_PATH)/libgui.so 
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := mediaplayer

FFMPEG_PATH := ../ffmpeg

LOCAL_CFLAGS := -D__STDC_CONSTANT_MACROS

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH) \
	$(LOCAL_PATH)/$(FFMPEG_PATH) \
	$(LOCAL_PATH)/../include/android_ics \
	$(LOCAL_PATH)/../include/android_ics/core \
	$(LOCAL_PATH)/../include/common

# Add your application source files here...
LOCAL_SRC_FILES += \
	mediaplayer.cpp \
	thread.cpp \
	decoder.cpp \
	videodecoder.cpp \
	audiodecoder.cpp \
	output.cpp \
	audiotrack.cpp \
	surface.cpp \
	packetqueue.cpp

LOCAL_SHARED_LIBRARIES := ffmpeg MEDIA SURFACEFLINGER SKIA GUI BINDER UTILS

LOCAL_LDLIBS := -llog 

include $(BUILD_SHARED_LIBRARY)
