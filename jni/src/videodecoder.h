/*
 * =====================================================================================
 *
 *       Filename:  videodecoder.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2013年02月12日 16时50分09秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author: gavin 
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef _GPLAYER_DECODER_VIDEO_H
#define _GPLAYER_DECODER_VIDEO_H

#include "decoder.h"
//#include "refreshthread.h"
extern "C"{
#include "SDL.h"
#include "SDL_thread.h"
}
#define VIDEO_PICTURE_QUEUE_SIZE 2
#define AV_SYNC_THRESHOLD 0.01
#define AV_NOSYNC_THRESHOLE 10.0

typedef void (*VideoDecodingHandler)(AVFrame*, double);
typedef int (*GetAudioClockHandler)(void);

typedef struct VideoPicture{
	AVFrame *bmp;
	int width, height;
	int allocated;
	double pts;
}VideoPicture;

class DecoderVideo : public IDecoder{
	public:
		DecoderVideo(AVStream* stream);
		~DecoderVideo();
//		static void sigroutine(int signo);
//		static int refreshStart;

		VideoDecodingHandler onDecode;
		GetAudioClockHandler audioClock;

		VideoPicture pictq[VIDEO_PICTURE_QUEUE_SIZE];
		int pictq_size,pictq_rindex,pictq_windex;
		// SDL_mutex* pictq_mutex;
		// SDL_cond* pictq_cond;
		pthread_mutex_t* pictq_mutex;
		pthread_cond_t* pictq_cond;
		double mVideoClock;

	private:
		bool prepare();
		double synchronize(AVFrame *src_frame, double pts);
		bool decode(void *ptr);
		bool process(AVPacket *packet);
		int queue_picture(AVFrame* pFrame, double pts);
		static int getBuffer(struct AVCodecContext *c, AVFrame *pic);
		static void releaseBuffer(struct AVCodecContext *c, AVFrame *pic);
		void video_refresh_timer(void *userdata);
		void alloc_picture(void* ptr);
		void videoDisplay(VideoPicture* vp);

		double video_current_pts;
		int64_t video_current_pts_time;
		double frame_last_pts;
		double frame_last_delay;
		double frame_timer;
		AVFrame* mFrame;
		struct SwsContext *img_convert_ctx;

		SDL_Thread* mEventThread;
		static int startEventThread(void* ptr);
		static uint32_t sdl_refresh_timer_cb(uint32_t interval,void *opaque);
		static void schedule_refresh(int time);

		void test(VideoPicture* vp, int iFrame);
};

#endif //_GPLAYER_DECODER_VIDEO_H

