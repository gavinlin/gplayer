/*
 * =====================================================================================
 *
 *       Filename:  audiodecoder.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2013年02月12日 16时50分26秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author: gavin 
 *   Organization:  
 *
 * =====================================================================================
 */
#include"audiodecoder.h"
#include "trace.h"
#include <media/AudioTrack.h>

using namespace android;

DecoderAudio::DecoderAudio(AVStream* stream) : IDecoder(stream){
	audio_clock = 0;
}

DecoderAudio::~DecoderAudio(){
	mRunning = false;
	if(audio_buf)
		free(audio_buf);
	if(audio_buf2)
		free(audio_buf2);
		av_free_packet(&audio_pkt);
		av_free_packet(&audio_pkt_temp);
}

bool DecoderAudio::prepare(){
	memset(&audio_pkt_temp, 0, sizeof(audio_pkt_temp));
	audio_src_fmt = audio_tgt_fmt = AV_SAMPLE_FMT_S16;
	audio_src_freq = audio_tgt_freq = ANDROID_DEFAULT_FREQ;
	audio_src_channel_layout = audio_tgt_channel_layout = av_get_default_channel_layout(ANDROID_DEFAULT_NBCHANNELS);
	audio_src_channels = audio_tgt_channels = ANDROID_DEFAULT_NBCHANNELS;

	return true;
}

bool DecoderAudio::process(AVPacket *packet){
	AVPacket *pkt_temp = &audio_pkt_temp;
	AVPacket *pkt = &audio_pkt;
	AVCodecContext *dec = mStream->codec;
	int len1, len2, data_size, n, resampled_data_size;
	int64_t dec_channel_layout;
	int got_frame;
	double pts;
	int new_packet = 1;
	int flush_complete = 0;

	*pkt_temp = *packet;
	if(packet->pts != AV_NOPTS_VALUE){
		audio_clock = av_q2d(mStream->time_base)*pkt->pts;
	}
	while(pkt_temp->size > 0 || (!pkt_temp->data && new_packet)){
		if(!mFrame){
			if(!(mFrame = avcodec_alloc_frame())){
				return false;
			}
		}else
			avcodec_get_frame_defaults(mFrame);
		
		if(flush_complete)
			break;
		new_packet = 0;
		len1 = avcodec_decode_audio4(dec,mFrame,&got_frame,
				pkt_temp);
		if(len1 < 0){
			pkt_temp->size = 0;
			break;
		}
		pkt_temp->data += len1;
		pkt_temp->size -= len1;

		if(!got_frame){
			if(!pkt_temp->data && dec->codec->capabilities & CODEC_CAP_DELAY)
				flush_complete = 1;
			continue;
		}
		data_size = av_samples_get_buffer_size(NULL, dec->channels,
												mFrame->nb_samples,
												dec->sample_fmt, 1);
		dec_channel_layout = (dec->channel_layout && dec->channels == av_get_channel_layout_nb_channels(dec->channel_layout)) ? dec->channel_layout : av_get_default_channel_layout(dec->channels);

		if(dec->sample_fmt != audio_src_fmt ||
			dec_channel_layout != audio_src_channel_layout ||
			dec->sample_rate != audio_src_freq){
			if(swr_ctx)
				swr_free(&swr_ctx);
			swr_ctx = swr_alloc_set_opts(NULL,
					audio_tgt_channel_layout, audio_tgt_fmt,
					audio_tgt_freq, dec_channel_layout,dec->sample_fmt,
					dec->sample_rate,0,NULL);
			if(!swr_ctx || swr_init(swr_ctx) < 0){
				break;
			}
			audio_src_channel_layout = dec_channel_layout;
			audio_src_channels = dec->channels;
			audio_src_freq = dec->sample_rate;
			audio_src_fmt = dec->sample_fmt;
		}
		resampled_data_size = data_size;
		if(swr_ctx){
			const uint8_t *in[] = {mFrame->data[0]};
			uint8_t *out[] = {audio_buf2};
			len2 = swr_convert(swr_ctx,out,sizeof(audio_buf2) / audio_tgt_channels / av_get_bytes_per_sample(audio_tgt_fmt), in, mFrame->nb_samples);
			if(len2 < 0)
				break;
			if(len2 == sizeof(audio_buf2) / audio_tgt_channels / av_get_bytes_per_sample(audio_tgt_fmt)){
				swr_init(swr_ctx);
			}
			audio_buf = audio_buf2;
			resampled_data_size = len2 * audio_tgt_channels * av_get_bytes_per_sample(audio_tgt_fmt);
		} else {
			audio_buf = mFrame->data[0];
		}

		pts = audio_clock;
		audio_clock += (double)data_size / (dec->channels * dec->sample_rate * av_get_bytes_per_sample(dec->sample_fmt));
		onDecode(audio_buf, resampled_data_size);
		return true;
	}
	return true;
}

bool DecoderAudio::decode(void* ptr){
	
	AVPacket pPacket;
	int new_packet = 0;
//	while(mRunning){
//		if(audio_pkt.data){
//			av_free_packet(&audio_pkt);
//		}
//		if(audio_pkt_temp.data){
//			memset(&audio_pkt_temp, 0, sizeof(audio_pkt_temp));
//		}
//		if( (new_packet = mQueue->get(&pPacket, true)) < 0){
//			mRunning = false;
//			return false;
//		}			
//		if(new_packet){
//
//			if(!process(&pPacket)){
//				mRunning = false;
//				return false;
//			}
//		}
//	}	
}

int DecoderAudio::audio_decode_frame(void* decoderAudio, void* buf,int buf_size, double* pts_ptr){
	DecoderAudio * is = (DecoderAudio*)decoderAudio;
	uint8_t * audio_buf = (uint8_t *)buf;
	AVPacket *pkt_temp = &is->audio_pkt_temp;
	AVPacket *pkt = &is->audio_pkt;
	AVCodecContext *dec = is->mStream->codec;
	int len1, len2, data_size, n, resampled_data_size;
	int64_t dec_channel_layout;
	int got_frame;
	double pts;
	int new_packet = 0;
	int flush_complete = 0;
	for(;;) {
		/** attention: the audio packet can contain sereral frames */
		while(pkt_temp->size > 0 || (!pkt_temp->data && new_packet)){
			if(!is->mFrame){
				if(!(is->mFrame = avcodec_alloc_frame()))
				{
					return -1;
				}
			}else
				avcodec_get_frame_defaults(is->mFrame);

			if(flush_complete)
				break;
			new_packet = 0;
			len1 = avcodec_decode_audio4(dec,is->mFrame,&got_frame, pkt_temp);
			if(len1 < 0)
			{
				pkt_temp->size = 0;
				break;
			}
			pkt_temp->data += len1;
			pkt_temp->size -= len1;

			if(!got_frame)
			{
				if(!pkt_temp->data && dec->codec->capabilities & CODEC_CAP_DELAY)
					flush_complete = 1;
				continue;
			}
			data_size = av_samples_get_buffer_size(NULL, dec->channels,
													is->mFrame->nb_samples,
													dec->sample_fmt, 1);
            dec_channel_layout = (dec->channel_layout && dec->channels == av_get_channel_layout_nb_channels(dec->channel_layout)) ? dec->channel_layout : av_get_default_channel_layout(dec->channels);
			
			if(dec->sample_fmt != is->audio_src_fmt ||
				dec_channel_layout != is->audio_src_channel_layout ||
				dec->sample_rate != is->audio_src_freq) {
				if(is->swr_ctx)
					swr_free(&is->swr_ctx);
				is->swr_ctx = swr_alloc_set_opts(NULL,
						is->audio_tgt_channel_layout,is->audio_tgt_fmt,is->audio_tgt_freq,
						dec_channel_layout, dec->sample_fmt, dec->sample_rate,0,NULL);
				if(!is->swr_ctx || swr_init(is->swr_ctx) < 0)
				{
					break;
				}
                is->audio_src_channel_layout = dec_channel_layout;
                is->audio_src_channels = dec->channels;
                is->audio_src_freq = dec->sample_rate;
                is->audio_src_fmt = dec->sample_fmt;
			}

			resampled_data_size = data_size;
			if(is->swr_ctx){
				const uint8_t *in[] = { is->mFrame->data[0]};
				uint8_t *out[] = {is->audio_buf2};

				len2 = swr_convert(is->swr_ctx,out, sizeof(is->audio_buf2) / is->audio_tgt_channels / av_get_bytes_per_sample(is->audio_tgt_fmt), in, is->mFrame->nb_samples);
				if(len2 < 0)
					break;
				if(len2 == sizeof(is->audio_buf2) / is->audio_tgt_channels /av_get_bytes_per_sample(is->audio_tgt_fmt)){
					swr_init(is->swr_ctx);
				}
				is->audio_buf = is->audio_buf2;
                resampled_data_size = len2 * is->audio_tgt_channels * av_get_bytes_per_sample(is->audio_tgt_fmt);
			} else {
				is->audio_buf = is->mFrame->data[0];
			}

			pts = is->audio_clock;
			*pts_ptr = pts;
			is->audio_clock += (double)data_size / 
				(dec->channels * dec->sample_rate * av_get_bytes_per_sample(dec->sample_fmt));
			return resampled_data_size;
		}

		if(pkt->data)
			av_free_packet(pkt);
		memset(pkt_temp, 0, sizeof(*pkt_temp));

		if( (new_packet = is->mQueue->get(pkt, true)) < 0){
			is->mRunning = false;
			return -1;
		}

		*pkt_temp = *pkt;
		if(pkt->pts != AV_NOPTS_VALUE){
			is->audio_clock = av_q2d(is->mStream->time_base)*pkt->pts;
		}
	}	
}

void DecoderAudio::cbf(int event, void* user, void * info){
	AudioTrack::Buffer *buf = (AudioTrack::Buffer*)info;
	int len = buf->size;
	uint8_t *stream = (uint8_t *)buf->raw;
	int len1, audio_size;
	DecoderAudio* is = (DecoderAudio* )user;
	double pts;

	if(event != AudioTrack::EVENT_MORE_DATA){
		return;
	}

	while(len > 0){
		if(is->audio_buf_index >= is->audio_buf_size){
			audio_size = audio_decode_frame(is, is->audio_buf, sizeof(is->audio_buf), &pts);
			if(audio_size < 0){
				//error ,output silent
				is->audio_buf_size = 1024;
				memset(is->audio_buf, 0,is->audio_buf_size);
			}else{
				is->audio_buf_size = audio_size;
			}
			is->audio_buf_index = 0;
		}
		len1 = is->audio_buf_size - is->audio_buf_index;
		if(len1 > len)
			len1 = len;
		memcpy(stream, (uint8_t *)is->audio_buf + is->audio_buf_index, len1);
		len -= len1;
		stream += len1;
		is->audio_buf_index += len1;
	}
}

