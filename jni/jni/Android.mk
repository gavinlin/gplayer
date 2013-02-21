LOCAL_PATH :=$(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := mediaplayer_jni

LOCAL_CFLAGS := -D__STDC_CONSTANT_MACROS

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../include/android_ics \
	$(LOCAL_PATH)/../include/common \
	$(LOCAL_PATH)/../ffmpeg \
	$(LOCAL_PATH)/../SDL/include \
	$(LOCAL_PATH)/../src

LOCAL_SRC_FILES += \
	com_lingavin_gplayer_mediaplayer.cpp

LOCAL_LDLIBS := -llog

LOCAL_SHARED_LIBRARIES := ffmpeg mediaplayer

include $(BUILD_SHARED_LIBRARY)
