/*
 * =====================================================================================
 *
 *       Filename:  refreshthread.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2013年02月20日 01时14分31秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#include "refreshthread.h"
#include <sys/time.h>
#include <signal.h>
extern "C"{

#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"

}
#include "trace.h"
int RefreshThread::refreshStart = 1;

RefreshThread::RefreshThread(DecoderVideo* decoderVideo){
	mDecoderVideo = decoderVideo;
	frame_last_delay = 0;
	frame_last_pts = 0;
	signal(SIGALRM, RefreshThread::sigroutine);
}

RefreshThread::~RefreshThread(){

}

void RefreshThread::sigroutine(int signo){
			TRACE("you should refresh");
			refreshStart = 1;
//	switch(signo){
//		case SIGALRM:
//			break;
//	}
//	return;
}

void RefreshThread::handleRun(void* ptr){
	TRACE("refrehs thread start");
	while(mRunning){
		TRACE("RUINNG refreshStart %d",refreshStart);
		if(refreshStart == 1){
			ERROR("refreshStart %d",refreshStart);
			refreshStart = 0;
			video_refresh_timer();
		}	
	}
}

void RefreshThread::schedule_refresh(int time){

	TRACE("set timer %d",time);
	struct itimerval value;
	value.it_value.tv_usec = time * 1000;
	value.it_value.tv_sec = 0;
	value.it_interval.tv_usec = 0;
	value.it_interval.tv_sec = 0;
	setitimer(ITIMER_REAL, &value, NULL);

}

void RefreshThread::videoDisplay(VideoPicture* vp){
	TRACE("video display");
	onDecode(vp->bmp,0);
}

void RefreshThread::video_refresh_timer(){
	VideoPicture *vp;
	double actual_delay, delay, sync_threshold, ref_clock, diff;
	TRACE("refresh timer");
	if(mDecoderVideo->mStream){
		if(mDecoderVideo->pictq_size == 0){
			//schedule_refresh
			TRACE("refresh timer no pictq");
			schedule_refresh(40);
		}else{
			vp = &mDecoderVideo->pictq[mDecoderVideo->pictq_rindex];	
			video_current_pts = vp->pts;
			video_current_pts_time = av_gettime();

			delay = vp->pts - frame_last_pts;
			if(delay <= 0 || delay >= 1.0){
				delay = frame_last_delay;
			}

			frame_last_delay = delay;
			frame_last_pts = vp->pts;

			ref_clock = (double)audioClock();
			diff = vp->pts - ref_clock;
			
			sync_threshold = (delay > AV_SYNC_THRESHOLD) ? delay : AV_SYNC_THRESHOLD;
			if(fabs(diff) < AV_NOSYNC_THRESHOLE){
				if(diff <= -sync_threshold){
					delay = 0;
				}else if(diff > sync_threshold){
					delay = 2 * delay;
				}
			}

			frame_timer += delay;
			
			actual_delay = frame_timer - (av_gettime() / 1000000.0);
			if(actual_delay < 0.010){
				//should skip the picture instead
				actual_delay = 0.010;
			}
			schedule_refresh((int)(actual_delay * 1000 + 0.5));

			videoDisplay(vp);

			if(++mDecoderVideo->pictq_rindex == VIDEO_PICTURE_QUEUE_SIZE){
				mDecoderVideo->pictq_rindex = 0;
			}
			pthread_mutex_lock(&mDecoderVideo->pictq_mutex);
			mDecoderVideo->pictq_size--;
			pthread_cond_signal(&mDecoderVideo->pictq_cond);
			mDecoderVideo->notify();
			pthread_mutex_unlock(&mDecoderVideo->pictq_mutex);
		}
	} else {
		TRACE("refresh timer no stream");
		schedule_refresh(100);
	}
}


void RefreshThread::stop(){

}

