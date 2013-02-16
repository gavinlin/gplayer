/*
 * =====================================================================================
 *
 *       Filename:  mediaplayer.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2013年02月12日 16时48分08秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef _GPLAYER_MEDIAPLAYER_H
#define _GPLAYER_MEDIAPLAYER_H

#include <jni.h>
#include <pthread.h>
#include <utils/Errors.h>
#include "trace.h"
#include "videodecoder.h"
#include "audiodecoder.h"

using namespace android;

enum media_player_status{
	MEDIA_PLAYER_STATE_ERROR       = 0,
	MEDIA_PLAYER_IDLE		       = 1 << 0,
	MEDIA_PLAYER_INITIALIZED       = 1 << 1,
	MEDIA_PLAYER_PREPARING         = 1 << 2,
	MEDIA_PLAYER_PREPARED          = 1 << 3,
	MEDIA_PLAYER_DECODED           = 1 << 4,
	MEDIA_PLAYER_STARTED           = 1 << 5,
	MEDIA_PLAYER_PAUSED            = 1 << 6,
	MEDIA_PLAYER_STOPPED           = 1 << 7,
	MEDIA_PLAYER_PLAYBACK_COMPLETE = 1 << 8
};

#define MAX_PLAYER_QUEUE_SIZE 10

class MediaPlayer{
	public:
		MediaPlayer();
		~MediaPlayer();
		status_t setDataSource(const char* path);
		status_t setVideoSurface(JNIEnv *env, jobject jsurface);
		status_t prepare();
		status_t start();

	private:
		media_player_status mCurrentState;
		int mAudioStreamIndex;
		int mVideoStreamIndex;
		AVFormatContext * pFormatCtx;
		AVStream *video_st;
		AVStream *audio_st;
		AVFrame* mFrame;
		struct SwsContext* img_convert_ctx;
		char mFilePath[4096];
		int mVideoWidth;
		int mVideoHeight;
		int mDuration;
		pthread_t mPlayerThread;
		pthread_mutex_t mLock;
		DecoderAudio *mDecoderAudio;
		DecoderVideo *mDecoderVideo;

		static void* startPlayer(void* ptr);
		void decodeMovie(void* ptr);
		void dumpInfo();
		status_t prepareVideo();
		status_t prepareAudio();
		static void decode(uint8_t* buffer, int buffer_size);
		static void decode(AVFrame *frame, double pts);
};

#endif //_GPLAYER_MEDIAPLAYER_H
