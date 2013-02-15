/*
 * =====================================================================================
 *
 *       Filename:  audiodecoder.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2013年02月12日 17时06分15秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef _GPLAYER_DECODER_AUDIO_H
#define _GPLAYER_DECODER_AUDIO_H

#include "decoder.h"
extern "C"{
#include "libswresample/swresample.h"
}

typedef void (*AudioDecodingHandler)(uint8_t*, int);

#define ANDROID_DEFAULT_FREQ 44100
#define ANDROID_DEFAULT_NBCHANNELS 2

class DecoderAudio : public IDecoder{
	public:
		int audio_clock;
		DecoderAudio(AVStream* stream);
		~DecoderAudio();

		AudioDecodingHandler onDecode;

	private:
		int16_t *mSamples;
		int mSamplesSize;
		AVPacket audio_pkt_temp;
		AVPacket audio_pkt;
		uint8_t *audio_buf;
		DECLARE_ALIGNED(16,uint8_t,audio_buf2)[AVCODEC_MAX_AUDIO_FRAME_SIZE * 4];
		enum AVSampleFormat audio_src_fmt;
		enum AVSampleFormat audio_tgt_fmt;
		int audio_src_channels;
		int audio_tgt_channels;
		int64_t audio_src_channel_layout;
		int64_t audio_tgt_channel_layout;
		int audio_src_freq;
		int audio_tgt_freq;
		struct SwrContext *swr_ctx;
		AVFrame *mFrame;

		bool prepare();
		bool decode(void* ptr);
		bool process(AVPacket* packet);
};


#endif //_GPLAYER_DECODER_AUDIO_H

