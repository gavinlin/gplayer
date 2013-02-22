/*
 * =====================================================================================
 *
 *       Filename:  videodecoder.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2013年02月12日 16时49分52秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include "videodecoder.h"
#include "trace.h"
#include "output.h"
extern "C"{
#include "libswscale/swscale.h"
}

static uint64_t global_video_pkt_pts = AV_NOPTS_VALUE;
static int refreshStart = 1;

#define QUIT_EVENT (SDL_USEREVENT)
#define ALLOC_EVENT (SDL_USEREVENT+1)
#define REFRESH_EVENT (SDL_USEREVENT+2)

DecoderVideo::DecoderVideo(AVStream *stream) : IDecoder(stream){
	mStream->codec->get_buffer = getBuffer;
	mStream->codec->release_buffer = releaseBuffer;
	pictq_mutex = SDL_CreateMutex();
	pictq_cond = SDL_CreateCond();
	// mRefreshThread = new RefreshThread(this);
	pictq_size = 0;
	pictq_windex = 0;
	pictq_rindex = 0;
	frame_last_delay = 0;
	frame_last_pts = 0;
	// signal(SIGALRM, DecoderVideo::sigroutine);

	img_convert_ctx = sws_getContext(
			mStream->codec->width,mStream->codec->height,
			mStream->codec->pix_fmt,
			mStream->codec->width,mStream->codec->height,
			PIX_FMT_RGB565,SWS_POINT,NULL,NULL,NULL);
	for(int i= 0;i < VIDEO_PICTURE_QUEUE_SIZE;i++){
		memset(&pictq[i],0,sizeof(VideoPicture));
	}
}

DecoderVideo::~DecoderVideo(){
	SDL_DestroyMutex(pictq_mutex);
	SDL_DestroyCond(pictq_cond);
}

bool DecoderVideo::prepare(){
	mFrame = avcodec_alloc_frame();
	if(mFrame == NULL){
		return false;
	}
	return true;
}
double DecoderVideo::synchronize(AVFrame *src_frame, double pts) {

	double frame_delay;

	if (pts != 0) {
		/* if we have pts, set video clock to it */
		mVideoClock = pts;
	} else {
		/* if we aren't given a pts, set it to the clock */
		pts = mVideoClock;
	}
	/* update the video clock */
	frame_delay = av_q2d(mStream->codec->time_base);
	/* if we are repeating a frame, adjust clock accordingly */
	frame_delay += src_frame->repeat_pict * (frame_delay * 0.5);
	mVideoClock += frame_delay;
	return pts;
}

bool DecoderVideo::process(AVPacket *packet)
{
    int	completed;
    int pts = 0;
	int len1;

	// Decode video frame
	len1 = avcodec_decode_video2(mStream->codec,
						 mFrame,
						 &completed,
						 packet);
	
	if (packet->dts == AV_NOPTS_VALUE && mFrame->opaque
			&& *(uint64_t*) mFrame->opaque != AV_NOPTS_VALUE) {
		pts = *(uint64_t *) mFrame->opaque;
	} else if (packet->dts != AV_NOPTS_VALUE) {
		pts = packet->dts;
	} else {
		pts = 0;
	}
	pts *= av_q2d(mStream->time_base);
	if (completed) {
		pts = synchronize(mFrame, pts);

		//onDecode(mFrame, pts);
		if(queue_picture(mFrame, pts) < 0)
			return false;
	}
	return true;
}

bool DecoderVideo::decode(void* ptr)
{
	AVPacket        pPacket;
	
	TRACE( "decoding video");

	//create thread for refresh
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)){
		TRACE("could not initialize sdl");
	}

//    SDL_EventState(SDL_ACTIVEEVENT, SDL_IGNORE);
//    SDL_EventState(SDL_SYSWMEVENT, SDL_IGNORE);
//    SDL_EventState(SDL_USEREVENT, SDL_IGNORE);

	mEventThread = SDL_CreateThread(startEventThread,"event_thread",this);

	schedule_refresh(40);
    while(mRunning)
    {
        if(mQueue->get(&pPacket, true) < 0)
        {
            mRunning = false;
            return false;
        }
        if(!process(&pPacket))
        {
			ERROR("process is false");
             mRunning = false;
             return false;
        }
        // Free the packet that was allocated by av_read_frame
        av_free_packet(&pPacket);
    }
	
    TRACE("decoding video ended");
	
    // Free the RGB image
    av_free(mFrame);

    return true;
}

int DecoderVideo::queue_picture(AVFrame* pFrame, double pts){
	VideoPicture *vp;
	AVPicture pict;

	SDL_LockMutex(pictq_mutex);
	while(pictq_size >= VIDEO_PICTURE_QUEUE_SIZE && mRunning){
		SDL_CondWait(pictq_cond,pictq_mutex);
	}
	ERROR("pictq_size is %d %d",pictq_size,__LINE__);
	SDL_UnlockMutex(pictq_mutex);

	if(!mRunning){
		return -1;
	}

	vp = &pictq[pictq_windex];

	if(!vp->bmp || 
			vp->width != mStream->codec->width ||
			vp->height != mStream->codec->height){
	ERROR("pictq_size is %d %d",pictq_size,__LINE__);
		SDL_Event event;
		vp->allocated = 0;
		event.type = ALLOC_EVENT;
		event.user.data1 = NULL;
		SDL_PushEvent(&event);

		SDL_LockMutex(pictq_mutex);
	ERROR("pictq_size is %d %d",pictq_size,__LINE__);
		while(!vp->allocated && mRunning){
			SDL_CondWait(pictq_cond, pictq_mutex);
		}
		SDL_UnlockMutex(pictq_mutex);
	ERROR("pictq_size is %d %d",pictq_size,__LINE__);



	}
	ERROR("pictq_size is %d %d",pictq_size,__LINE__);
	if(vp->bmp){
	ERROR("pictq_size is %d %d",pictq_size,__LINE__);

	SDL_LockMutex(pictq_mutex);

	pict.data[0] = vp->bmp->data[0];
	pict.data[1] = vp->bmp->data[1];
	pict.data[2] = vp->bmp->data[2];
	pict.linesize[0] = vp->bmp->linesize[0];
	pict.linesize[1] = vp->bmp->linesize[1];
	pict.linesize[2] = vp->bmp->linesize[2];

//		img_convert_ctx = sws_getCachedContext(
//				img_convert_ctx,
//				vp->width,vp->height,mStream->codec->pix_fmt,
//				vp->width,vp->height,PIX_FMT_RGB565,
//				SWS_POINT,NULL,
//				NULL,NULL
//				);


		sws_scale(img_convert_ctx,
				(const uint8_t * const*)pFrame->data,
				pFrame->linesize,
				0,
				mStream->codec->height,
				pict.data,
				pict.linesize);	
		SDL_UnlockMutex(pictq_mutex);
		vp->pts = pts;
		if(++pictq_windex == VIDEO_PICTURE_QUEUE_SIZE){
			pictq_windex = 0;
		}
		SDL_LockMutex(pictq_mutex);
		pictq_size++;
		SDL_UnlockMutex(pictq_mutex);
	}
	ERROR("pictq_size is %d %d",pictq_size,__LINE__);
	return 0;
}


int DecoderVideo::getBuffer(struct AVCodecContext *c, AVFrame *pic){
	int ret = avcodec_default_get_buffer(c, pic);
	uint64_t * pts = (uint64_t *)av_mallocz(sizeof(uint64_t));
	*pts = global_video_pkt_pts;
	pic->opaque = pts;
	return ret;
}

void DecoderVideo::releaseBuffer(struct AVCodecContext *c, AVFrame *pic){
	if(pic)
		av_freep(&pic->opaque);
	avcodec_default_release_buffer(c, pic);
}

//void DecoderVideo::sigroutine(int signo){
//			TRACE("you should refresh");
//			refreshStart = 1;
////	switch(signo){
////		case SIGALRM:
////			break;
////	}
////	return;
//}

uint32_t DecoderVideo::sdl_refresh_timer_cb(uint32_t interval, void *opaque){
	SDL_Event event;
	event.type = REFRESH_EVENT;
	event.user.data1 = opaque;
	SDL_PushEvent(&event);
	return 0;
}

void DecoderVideo::schedule_refresh(int time){

	TRACE("set timer %d",time);
//	struct itimerval value;
//	value.it_value.tv_sec = 0;
//	value.it_value.tv_usec = time * 1000;
//	value.it_interval.tv_sec = 0;
//	value.it_interval.tv_usec = time * 1000;
//	setitimer(ITIMER_REAL, &value, NULL);

	SDL_AddTimer(time, sdl_refresh_timer_cb, NULL);
}

void DecoderVideo::videoDisplay(VideoPicture* vp){
	if(vp->bmp){
		TRACE("video display");
		onDecode(vp->bmp,0);
		//		onDecode(mFrame,0);
	}
}

void DecoderVideo::video_refresh_timer(void *userdata){
	VideoPicture *vp;
	double actual_delay, delay, sync_threshold, ref_clock, diff;
//	TRACE("refresh timer");
	if(mStream){
		if(pictq_size == 0){
			//schedule_refresh
//			TRACE("refresh timer no pictq");
			schedule_refresh(40);
		}else{
			vp = &pictq[pictq_rindex];	
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

			if(++pictq_rindex == VIDEO_PICTURE_QUEUE_SIZE){
				pictq_rindex = 0;
			}
			SDL_LockMutex(pictq_mutex);
			pictq_size--;
			SDL_CondSignal(pictq_cond);
			SDL_UnlockMutex(pictq_mutex);
		}
	} else {
//		TRACE("refresh timer no stream");
		schedule_refresh(100);
	}
}

void DecoderVideo::alloc_picture(void *userdata){
	VideoPicture* vp;
	vp = &pictq[pictq_windex];
	TRACE("Alloc_picture %d",__LINE__);
	if(vp->bmp != 0){
		ERROR("vp->bmp is %p",vp->bmp);
		av_free(vp->bmp);
	}

	TRACE("Alloc_picture %d",__LINE__);
	vp->width = mStream->codec->width;
	vp->height = mStream->codec->height;
	vp->bmp = avcodec_alloc_frame();
	TRACE("Alloc_picture %d",__LINE__);
	int numBytes=avpicture_get_size(PIX_FMT_RGB565,
			mStream->codec->width,
			mStream->codec->height);
	TRACE("numBytes is %d",numBytes);
	uint8_t * buffer = (uint8_t *)av_malloc(numBytes*sizeof(uint8_t));
	TRACE("Alloc_picture %d",__LINE__);
	avpicture_fill((AVPicture *)vp->bmp,buffer,
			PIX_FMT_RGB565,mStream->codec->width,mStream->codec->height);
	SDL_LockMutex(pictq_mutex);
	vp->allocated = 1;
	SDL_CondSignal(pictq_cond);
	SDL_UnlockMutex(pictq_mutex);
	TRACE("Alloc_picture %d",__LINE__);

}

int DecoderVideo::startEventThread(void* ptr){
	DecoderVideo* mDecoderVideo = (DecoderVideo*)ptr;
	SDL_Event event;
	while(mDecoderVideo->mRunning){
		SDL_WaitEvent(&event);
		switch(event.type){
			case QUIT_EVENT:

				break;
			case ALLOC_EVENT:
				ERROR("ALLOC_EVENT");
				mDecoderVideo->alloc_picture(event.user.data1);
				break;
			case REFRESH_EVENT:
//				ERROR("REFRESH EVENt");
				mDecoderVideo->video_refresh_timer(event.user.data1);
				break;
			default:
				break;
		}
	}
}
