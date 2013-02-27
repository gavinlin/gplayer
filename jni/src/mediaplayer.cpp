/*
 * =====================================================================================
 *
 *       Filename:  mediaplayer.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2013年02月12日 16时47分46秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author: gavin 
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>

extern "C"{

#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"

}

#include "mediaplayer.h"
#include "audiotrack.h"
#include "surface.h"

static MediaPlayer* sPlayer;
static void* libhandle = 0;
static void* libsurfacehandle = 0;

MediaPlayer::MediaPlayer(){
	sPlayer = this;
	libhandle = dlopen("/data/data/com.lingavin.gplayer/lib/libatrack.so", RTLD_NOW);
	if(libhandle){
		AndroidAudioTrack_register = (typeof(AndroidAudioTrack_register)) dlsym(libhandle,"AndroidAudioTrack_register");
		AndroidAudioTrack_set= (typeof(AndroidAudioTrack_set)) dlsym(libhandle,"AndroidAudioTrack_set");
		AndroidAudioTrack_start = (typeof(AndroidAudioTrack_start)) dlsym(libhandle,"AndroidAudioTrack_start");
		AndroidAudioTrack_flush = (typeof(AndroidAudioTrack_flush)) dlsym(libhandle,"AndroidAudioTrack_flush");
		AndroidAudioTrack_stop = (typeof(AndroidAudioTrack_stop)) dlsym(libhandle,"AndroidAudioTrack_stop");
		AndroidAudioTrack_reload = (typeof(AndroidAudioTrack_reload)) dlsym(libhandle,"AndroidAudioTrack_reload");
		AndroidAudioTrack_unregister = (typeof(AndroidAudioTrack_unregister)) dlsym(libhandle,"AndroidAudioTrack_unregister");
	}

	libsurfacehandle= dlopen("/data/data/com.lingavin.gplayer/lib/libsurface.so", RTLD_NOW);
	if(libsurfacehandle){
		AndroidSurface_register = (typeof(AndroidSurface_register)) dlsym(libsurfacehandle,"AndroidSurface_register");
		AndroidSurface_getPixels = (typeof(AndroidSurface_getPixels)) dlsym(libsurfacehandle,"AndroidSurface_getPixels");
		AndroidSurface_updateSurface = (typeof(AndroidSurface_updateSurface)) dlsym(libsurfacehandle,"AndroidSurface_updateSurface");
		AndroidSurface_unregister = (typeof(AndroidSurface_unregister)) dlsym(libsurfacehandle,"AndroidSurface_unregister");
	}
}

MediaPlayer::~MediaPlayer(){
	dlclose(libhandle);
	dlclose(libsurfacehandle);
}

int MediaPlayer::getAudioClock(){
	return sPlayer->mDecoderAudio->audio_clock;
}

void MediaPlayer::decode(uint8_t* buffer, int buffer_size){
	// TRACE("decode audio data %d",buffer_size);
	int ret;
	//if(ret = (Output::AudioDriver_write(buffer, buffer_size)) <= 0){
	//	ERROR("couldn't write buffers to audio track ret is %d", ret);
	//}
}

void MediaPlayer::decode(AVFrame *frame, double pts){
	
	sws_scale(sPlayer->img_convert_ctx,
			frame->data,
			frame->linesize,
			0,
			sPlayer->mVideoHeight,
			sPlayer->mFrame->data,
			sPlayer->mFrame->linesize);
	AndroidSurface_updateSurface();
}

void MediaPlayer::decodeMovie(void* ptr){
	AVPacket pkt1, *pPacket = &pkt1;
	//start audio thread
	mDecoderAudio = new DecoderAudio(audio_st);
	mDecoderAudio->onDecode = decode;
	//prepare os output
	if(AndroidAudioTrack_set(MUSIC,
								44100,
								PCM_16_BIT,
								(audio_st->codec->channels == 2) ? CHANNEL_OUT_STEREO
									: CHANNEL_OUT_MONO,
								SDL_AUDIO_BUFFER_SIZE,
								0,
								DecoderAudio::cbf,mDecoderAudio) != 0){
		return ;
	}
	ERROR("set audio track successed");
	if(AndroidAudioTrack_start() != 0){
		return ;
	}


	mDecoderAudio->startAsync();

	//start video thread
	mDecoderVideo = new DecoderVideo(video_st);
	mDecoderVideo->onDecode = decode;
	mDecoderVideo->audioClock = getAudioClock;
	mDecoderVideo->startAsync();

	//start video refresh thread
//	mRefreshThread = new RefreshThread(mDecoderVideo);
//	mRefreshThread->startAsync();

	//put packet to queue
	mCurrentState = MEDIA_PLAYER_STARTED;
	while(mCurrentState != MEDIA_PLAYER_DECODED &&
			mCurrentState != MEDIA_PLAYER_STOPPED &&
			mCurrentState != MEDIA_PLAYER_STATE_ERROR){
		if(mDecoderAudio->packets() > MAX_PLAYER_QUEUE_SIZE &&
				mDecoderVideo->packets() > MAX_PLAYER_QUEUE_SIZE){
			usleep(200);
			continue;
		}		
		if(av_read_frame(pFormatCtx, pPacket) < 0){
			mCurrentState = MEDIA_PLAYER_DECODED;
			continue;
		}

		if(pPacket->stream_index == mVideoStreamIndex){
			mDecoderVideo->enqueue(pPacket);
		}else if(pPacket->stream_index == mAudioStreamIndex){
			mDecoderAudio->enqueue(pPacket);
		}else{
			av_free_packet(pPacket);
		}
	}

	int ret = -1;
	if((ret = mDecoderAudio->wait()) != 0){
		ERROR("can't cancel audio thread");
	}

	if(mCurrentState == MEDIA_PLAYER_STATE_ERROR){
		ERROR("playing error");
	}

	mCurrentState = MEDIA_PLAYER_PLAYBACK_COMPLETE;
}

void* MediaPlayer::startPlayer(void* ptr){
	TRACE("start decode movie");
	sPlayer->decodeMovie(ptr);
}

status_t MediaPlayer::start(){
	if(mCurrentState != MEDIA_PLAYER_PREPARED){
		return INVALID_OPERATION;
	}
	pthread_create(&mPlayerThread, NULL, startPlayer, NULL);
	return NO_ERROR;
}

void MediaPlayer::dumpInfo(){
	int hours, mins, secs, us;
	secs = pFormatCtx->duration / AV_TIME_BASE;
	us = pFormatCtx->duration % AV_TIME_BASE;
	mins = secs / 60;
	secs %= 60;
	hours = mins / 60;
	mins %= 60;
	TRACE("duration is %02d:%02d:%02d.%02d",hours,mins,secs,us);
}

status_t MediaPlayer::setDataSource(const char* path){
	status_t ret = NO_ERROR;
	AVFormatContext *ic = NULL; // must be null here ,or it will be crush
	av_register_all();
	memset(mFilePath,0,sizeof(mFilePath));
	strcpy(mFilePath,path);
	TRACE("mediaplayer setData successed mFilePath is %s",mFilePath);
	TRACE("register all");
	if(avformat_open_input(&ic, mFilePath, NULL, NULL) != 0){
		ret = INVALID_OPERATION;
	}
	pFormatCtx = ic;
	TRACE("avformat open input successed");
	if(av_find_stream_info(pFormatCtx) < 0){
		ret = INVALID_OPERATION;
	}

	TRACE("find stream info successed");
	dumpInfo();
	return ret;
}

status_t MediaPlayer::setVideoSurface(JNIEnv* env, jobject jsurface){
	if(AndroidSurface_register(env,jsurface) != 0){
		return INVALID_OPERATION;
	}

	if(AndroidAudioTrack_register() != 0){
		return INVALID_OPERATION;
	}
	TRACE("set vudeo surface successed");
	return NO_ERROR;
}

status_t MediaPlayer::prepareVideo(){
	mVideoStreamIndex = -1;
	AVCodecContext* codecCtx;
	AVCodec* codec;
	for(int i = 0; i < pFormatCtx->nb_streams; i++){
		if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO){
			mVideoStreamIndex = i;
			break;
		}
	}

	if(mVideoStreamIndex == -1){
		return INVALID_OPERATION;
	}

	codecCtx = pFormatCtx->streams[mVideoStreamIndex]->codec;
	codec = avcodec_find_decoder(codecCtx->codec_id);
	if(!codec || (avcodec_open(codecCtx, codec) < 0)){
		return INVALID_OPERATION;
	}

	mVideoWidth = codecCtx->width;
	mVideoHeight = codecCtx->height;
	mDuration = pFormatCtx->duration;

	video_st = pFormatCtx->streams[mVideoStreamIndex];

	img_convert_ctx = sws_getContext(
			video_st->codec->width,video_st->codec->height,
			video_st->codec->pix_fmt,
			video_st->codec->width,video_st->codec->height,
			PIX_FMT_RGB565,SWS_POINT,NULL,NULL,NULL);
	void* pixels;
	if(AndroidSurface_getPixels(video_st->codec->width,
										video_st->codec->height,
										&pixels) != 0){
		return INVALID_OPERATION;
	}

	mFrame = avcodec_alloc_frame();
	if(mFrame == NULL){
		return INVALID_OPERATION;
	}
	TRACE("prepare video successed");
		avpicture_fill((AVPicture *) mFrame,
				   (uint8_t *) pixels,
				   PIX_FMT_RGB565,
				   video_st->codec->width,
				   video_st->codec->height);

	return NO_ERROR;
}

status_t MediaPlayer::prepareAudio(){
	mAudioStreamIndex = -1;
	for(int i=0; i < pFormatCtx->nb_streams; i++){
		if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO){
			mAudioStreamIndex = i;
			break;
		}
	}

	if(mAudioStreamIndex == -1){
		return INVALID_OPERATION;
	}

	audio_st = pFormatCtx->streams[mAudioStreamIndex];
	AVCodecContext* codecCtx = audio_st->codec;
	AVCodec* codec = avcodec_find_decoder(codecCtx->codec_id);
	if(codec == NULL){
		return INVALID_OPERATION;
	}

	if(avcodec_open(codecCtx, codec) < 0){
		return INVALID_OPERATION;
	}

	return NO_ERROR;
}

status_t MediaPlayer::prepare(int sdkVersion){
	status_t ret;

	mCurrentState = MEDIA_PLAYER_PREPARING; 
	if((ret = prepareVideo()) != NO_ERROR){
		mCurrentState = MEDIA_PLAYER_STATE_ERROR;
		return ret;
	}
	if((ret = prepareAudio()) != NO_ERROR){
		mCurrentState = MEDIA_PLAYER_STATE_ERROR;
		return ret;
	}
	mCurrentState = MEDIA_PLAYER_PREPARED;
	return NO_ERROR;
}

status_t MediaPlayer::suspend(){
	TRACE("suspend");
	mCurrentState = MEDIA_PLAYER_STOPPED;
	if(mDecoderAudio != NULL){
		mDecoderAudio->stop();
	}
	if(mDecoderVideo != NULL){
		mDecoderVideo->stop();
	}
	if(pthread_join(mPlayerThread, NULL) != 0){
		ERROR("stop playerthread error");
	}

	delete mDecoderAudio;
	delete mDecoderVideo;

	av_close_input_file(pFormatCtx);

	AndroidAudioTrack_unregister();
	AndroidSurface_unregister();
	TRACE("suspend successed");
	return NO_ERROR;
}


bool MediaPlayer::isPlaying(){
	return mCurrentState == MEDIA_PLAYER_STARTED ? true : false;
}

