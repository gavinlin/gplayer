/*
 * =====================================================================================
 *
 *       Filename:  com_lingavin_gplayer_mediaplayer.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2013年02月12日 16时31分28秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  gavinlin 
 *   Organization:  
 *
 * =====================================================================================
 */
#include "trace.h"
#include <jni.h>
#include <stdlib.h>
#include <mediaplayer.h>

struct fields_t{
	jfieldID context;
};
static fields_t fields;

static const char* const kClassPathName = "com/lingavin/gplayer/MediaPlayer";

static MediaPlayer* getMediaPlayer(JNIEnv* env, jobject thiz){
	return (MediaPlayer*)env->GetIntField(thiz, fields.context);
}

static void setMediaPlayer(JNIEnv* env, jobject thiz, MediaPlayer* player){
	MediaPlayer* old = (MediaPlayer*)env->GetIntField(thiz, fields.context);
	if(old != NULL){
		free(old);
	}
	env->SetIntField(thiz, fields.context, (int)player);
}

static void com_lingavin_gplayer_MediaPlayer_nativeSetup(JNIEnv* env,jobject thiz, jint sdkVersion){
	MediaPlayer* mp = new MediaPlayer(sdkVersion);
	if(mp == NULL){
		ERROR("nativeSetup error : mp is null");
		return ;
	}

	setMediaPlayer(env, thiz, mp);
}

static jboolean com_lingavin_gplayer_MediaPlayer_nativeIsPlaying(JNIEnv *env, jobject thiz){
	MediaPlayer *mp = getMediaPlayer(env, thiz);
	return mp->isPlaying();
}


static void com_lingavin_gplayer_MediaPlayer_nativeSuspend(JNIEnv *env, jobject thiz){
	MediaPlayer *mp = getMediaPlayer(env, thiz);
	mp->suspend();
}

static void com_lingavin_gplayer_MediaPlayer_nativePrepare(JNIEnv* env, jobject thiz){
	MediaPlayer* mp = getMediaPlayer(env, thiz);
	mp->prepare();
}

static void com_lingavin_gplayer_MediaPlayer_nativeStart(JNIEnv* env,jobject thiz){
	MediaPlayer* mp = getMediaPlayer(env,thiz);
	mp->start();
}

static void com_lingavin_gplayer_MediaPlayer_setVideoSurface(JNIEnv* env, jobject thiz, jobject jsurface){
	MediaPlayer* mp = getMediaPlayer(env,thiz);

	if(mp == NULL){
		ERROR("setVideoSurface error : mp is null");
		return;
	}

	if(jsurface == NULL){
		ERROR("setVideoSurface error : surface is null");
		return;
	}

	mp->setVideoSurface(env, jsurface);
}

static jint com_lingavin_gplayer_MediaPlayer_nativeDuration(JNIEnv* env, jobject thiz){
	MediaPlayer* mp = getMediaPlayer(env,thiz);
	if(mp == NULL){
		ERROR("setVideoSurface error : mp is null");
		return 0;
	}
	return mp->getDuration();
}

static jint com_lingavin_gplayer_MediaPlayer_nativeCurrentPosition(JNIEnv* env, jobject thiz){
	MediaPlayer* mp = getMediaPlayer(env,thiz);
	if(mp == NULL){
		ERROR("setVideoSurface error : mp is null");
		return 0;
	}
	return mp->getCurrentPosition();
}

/********************************************************/

static void com_lingavin_gplayer_MediaPlayer_nativeInit(JNIEnv* env){
	jclass clazz = env->FindClass(kClassPathName);
	if(clazz == NULL){
		ERROR("native init error : clazz is null");
		return;
	}

	fields.context = env->GetFieldID(clazz, "mNativeContext", "I");
	if(fields.context == NULL){
		ERROR("native init error : fields context is null");
		return;
	}
}

static void com_lingavin_gplayer_MediaPlayer_setDataSource(JNIEnv* env,jobject thiz, jstring path){
	MediaPlayer* mp = getMediaPlayer(env,thiz);
	if(path == NULL){
		ERROR("path is null");
	}

	const char* pPath = env->GetStringUTFChars(path, NULL);
	TRACE("native get path is %s",pPath);
	mp->setDataSource(pPath);
	env->ReleaseStringUTFChars(path,pPath);
}

int jniRegisterNativeMethods(JNIEnv* env,
							const char* className,
							const JNINativeMethod* gMethods,
							int numMethods){
	jclass clazz;
	clazz = env->FindClass(className);
	if(clazz == NULL){
		return -1;
	}

	if(env->RegisterNatives(clazz, gMethods, numMethods) < 0){
		return -1;
	}

	return 0;
}

static JNINativeMethod gMethods[] = {
	{"setDataSource", "(Ljava/lang/String;)V", (void *)com_lingavin_gplayer_MediaPlayer_setDataSource },
	{"nativeInit", "()V", (void *)com_lingavin_gplayer_MediaPlayer_nativeInit },
	{"setVideoSurface", "(Landroid/view/Surface;)V", (void *)com_lingavin_gplayer_MediaPlayer_setVideoSurface},
	{"nativeSetup", "(I)V", (void *)com_lingavin_gplayer_MediaPlayer_nativeSetup},
	{"nativePrepare", "()V", (void *)com_lingavin_gplayer_MediaPlayer_nativePrepare},
	{"nativeStart", "()V", (void *)com_lingavin_gplayer_MediaPlayer_nativeStart},
	{"nativeSuspend", "()V", (void *)com_lingavin_gplayer_MediaPlayer_nativeSuspend},
	{"nativeIsPlaying", "()Z", (void *)com_lingavin_gplayer_MediaPlayer_nativeIsPlaying},
	{"nativeDuration", "()I", (void *)com_lingavin_gplayer_MediaPlayer_nativeDuration},
	{"nativeCurrentPosition", "()I", (void *)com_lingavin_gplayer_MediaPlayer_nativeCurrentPosition},
};

int register_com_lingavin_gplayer_mediaplayer(JNIEnv* env){
	return jniRegisterNativeMethods(env, kClassPathName, gMethods, sizeof(gMethods) / sizeof(gMethods[0]));
}

jint JNI_OnLoad(JavaVM* vm, void* reserved){
	JNIEnv* env = NULL;
	jint result = JNI_ERR;

	if(vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK){		
		ERROR("jni init fialed!!");
		return result;
	}

	if(register_com_lingavin_gplayer_mediaplayer(env) != JNI_OK){
		ERROR("register mediaplayer failed");
		goto end;
	}

	result = JNI_VERSION_1_4;

end:
	return result;
}
