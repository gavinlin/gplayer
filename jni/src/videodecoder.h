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
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef _GPLAYER_DECODER_VIDEO_H
#define _GPLAYER_DECODER_VIDEO_H

#include "decoder.h"
//#include "refreshthread.h"


#define VIDEO_PICTURE_QUEUE_SIZE 2
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

		VideoPicture pictq[VIDEO_PICTURE_QUEUE_SIZE];
		int pictq_size,pictq_rindex,pictq_windex;
		pthread_mutex_t pictq_mutex;
		pthread_cond_t pictq_cond;
		double mVideoClock;

	private:
		AVFrame* mFrame;
		struct SwsContext *img_convert_ctx;
		// RefreshThread* mRefreshThread;

		bool prepare();
		double synchronize(AVFrame *src_frame, double pts);
		bool decode(void *ptr);
		bool process(AVPacket *packet);
		int queue_picture(AVFrame* pFrame, double pts);
		static int getBuffer(struct AVCodecContext *c, AVFrame *pic);
		static void releaseBuffer(struct AVCodecContext *c, AVFrame *pic);
};

#endif //_GPLAYER_DECODER_VIDEO_H

