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
LOCAL_MODULE := BINDER 
LOCAL_SRC_FILES := $(PREBUILT_PATH)/libbinder.so 
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := UTILS
LOCAL_SRC_FILES := $(PREBUILT_PATH)/libutils.so 
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := MEDIA
LOCAL_SRC_FILES := $(PREBUILT_PATH)/libmedia.so 
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := atrack
LOCAL_CFLAGS += -O2 -Wall -DBUILD_STANDALONE -DCPU_ARM -DAVSREMOTE -finline-functions -fPIC -D__ARM_EABI__=1 -DOLD_LOGDH
LOCAL_SRC_FILES := audiotrack.cpp
LOCAL_ARM_MODE :=arm
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../include/android_ics \
	$(LOCAL_PATH)/../include/common
LOCAL_LDLIBS := -llog \
	/home/gavin/workspace/gplayer/jni/prebuilt/android_ics/libmedia.so \
	/home/gavin/workspace/gplayer/jni/prebuilt/android_ics/libutils.so 
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := mediaplayer

FFMPEG_PATH := ../ffmpeg
SDL_PATH := ../SDL/include

LOCAL_CFLAGS := -D__STDC_CONSTANT_MACROS

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH) \
	$(LOCAL_PATH)/$(FFMPEG_PATH) \
	$(LOCAL_PATH)/$(SDL_PATH) \
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
	surface.cpp \
	packetqueue.cpp 

LOCAL_SHARED_LIBRARIES := ffmpeg SDL MEDIA SURFACEFLINGER SKIA GUI BINDER UTILS

LOCAL_LDLIBS := -llog 

LOCAL_ARM_MODE :=arm

include $(BUILD_SHARED_LIBRARY)
