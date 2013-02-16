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

typedef void (*VideoDecodingHandler)(AVFrame*, double);

class DecoderVideo : public IDecoder{
	public:
		DecoderVideo(AVStream* stream);
		~DecoderVideo();

		VideoDecodingHandler onDecode;

	private:
		AVFrame* mFrame;
		double mVideoClock;

		bool prepare();
		double synchronize(AVFrame *src_frame, double pts);
		bool decode(void *ptr);
		bool process(AVPacket *packet);
		static int getBuffer(struct AVCodecContext *c, AVFrame *pic);
		static void releaseBuffer(struct AVCodecContext *c, AVFrame *pic);
};

#endif //_GPLAYER_DECODER_VIDEO_H

