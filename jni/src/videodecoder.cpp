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

static uint64_t global_video_pkt_pts = AV_NOPTS_VALUE;

DecoderVideo::DecoderVideo(AVStream *stream) : IDecoder(stream){
	mStream->codec->get_buffer = getBuffer;
	mStream->codec->release_buffer = releaseBuffer;
}

DecoderVideo::~DecoderVideo(){

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

		onDecode(mFrame, pts);

		return true;
	}
	return false;
}

bool DecoderVideo::decode(void* ptr)
{
	AVPacket        pPacket;
	
	TRACE( "decoding video");
	
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
            // mRunning = false;
            // return false;
        }
        // Free the packet that was allocated by av_read_frame
        av_free_packet(&pPacket);
    }
	
    TRACE("decoding video ended");
	
    // Free the RGB image
    av_free(mFrame);

    return true;
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

