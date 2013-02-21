/*
 * =====================================================================================
 *
 *       Filename:  refreshthread.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2013年02月20日 01时23分10秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef _GPLAYER_REFRESHTHREAD
#define _GPLAYER_REFRESHTHREAD
#include "thread.h"
#include "videodecoder.h"

#define AV_SYNC_THRESHOLD 0.01
#define AV_NOSYNC_THRESHOLE 10.0

typedef void (*VideoDecodingHandler)(AVFrame*, double);
typedef int (*GetAudioClockHandler)(void);

class RefreshThread : public Thread{
	public:
		RefreshThread(DecoderVideo *decoderVideo);
		~RefreshThread();
		void stop();
		void handleRun(void* ptr);
		static void sigroutine(int signo);

		VideoDecodingHandler onDecode;
		GetAudioClockHandler audioClock;
		static int refreshStart;
	private:
		void video_refresh_timer();
		double video_current_pts;
		int64_t video_current_pts_time;
		double frame_last_pts;
		double frame_last_delay;
		double frame_timer;
		void schedule_refresh(int time);
		void videoDisplay(VideoPicture* vp);

		DecoderVideo* mDecoderVideo;
};

#endif
