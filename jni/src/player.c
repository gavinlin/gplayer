#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "SDL.h"
#include "SDL_thread.h"
#include "trace.h"
#include <stdio.h>
#include <math.h>
#include <time.h>

#define DEBUG_GAVIN

#define SDL_AUDIO_BUFFER_SIZE 1024

#define MAX_AUDIOQ_SIZE (5 * 16 * 1024)
#define MAX_VIDEOQ_SIZE (5 * 256 * 1024)

#define AV_SYNC_THRESHOLD 0.01
#define AV_NOSYNC_THRESHOLD 10.0

#define SAMPLE_CORRECTION_PERCENT_MAX 10
#define AUDIO_DIFF_AVG_NB 20

#define FF_ALLOC_EVENT   (SDL_USEREVENT)
#define FF_REFRESH_EVENT (SDL_USEREVENT + 1)
#define FF_QUIT_EVENT (SDL_USEREVENT + 2)

#define VIDEO_PICTURE_QUEUE_SIZE 1

#define DEFAULT_AV_SYNC_TYPE AV_SYNC_VIDEO_MASTER
typedef struct PacketQueue 
{
	AVPacketList *first_pkt,*last_pkt;
	int nb_packets;
	int size;
	SDL_mutex *mutex;
	SDL_cond *cond;
} PacketQueue;

typedef struct VideoPicture{
	SDL_Overlay *bmp;
	int width, height;
	int allocated;
	double pts;
}VideoPicture;

typedef struct VideoState{
	AVFormatContext		*pFormatCtx;
	int					videoStream,audioStream;
	int             av_sync_type;
	double          external_clock; /* external clock base */
	int64_t         external_clock_time;

	double          audio_clock;
	AVStream			*audio_st;
	PacketQueue			audioq;
	uint8_t				audio_buf[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2];
	unsigned int		audio_buf_size;
	unsigned int		audio_buf_index;
	AVPacket			audio_pkt;
	uint8_t				*audio_pkt_data;
	int					audio_pkt_size;
	int             audio_hw_buf_size;  
	double          audio_diff_cum; /* used for AV difference average computation */
	double          audio_diff_avg_coef;
	double          audio_diff_threshold;
	int             audio_diff_avg_count;
	double          frame_timer;
	double          frame_last_pts;
	double          frame_last_delay;
	double          video_clock; ///<pts of last decoded frame / predicted pts of next decoded frame
	double          video_current_pts; ///<current displayed pts (different from video_clock if frame fifos are used)
	int64_t         video_current_pts_time;  ///<time (av_gettime) at which we updated video_current_pts - used to have running video pts
	AVStream        *video_st;
	PacketQueue     videoq;

	VideoPicture		pictq[VIDEO_PICTURE_QUEUE_SIZE];
	int					pictq_size,pictq_rindex,pictq_windex;
	SDL_mutex			*pictq_mutex;
	SDL_cond			*pictq_cond;

	SDL_Thread			*parse_tid;
	SDL_Thread			*video_tid;

	struct SwsContext *img_convert_ctx;

	char				filename[1024];
	int					quit;
	int					width;
	int					height;
} VideoState; 

enum {
	AV_SYNC_AUDIO_MASTER,
	AV_SYNC_VIDEO_MASTER,
	AV_SYNC_EXTERNAL_MASTER,
};

SDL_Surface     *screen;

/* Since we only have one decoding thread, the Big Struct
   can be global in case we need it. */
VideoState *global_video_state;

int quit = 0;

//int rint(int x)
//{
//	return (int)(x + (x < 0 ? -0.5 : 0.5));
//};

void packet_queue_init(PacketQueue *q)
{
	memset(q,0,sizeof(PacketQueue));
	q->mutex = SDL_CreateMutex();
	q->cond = SDL_CreateCond();
}

int packet_queue_put(PacketQueue *q, AVPacket *pkt) {

	AVPacketList *pkt1;
	if(av_dup_packet(pkt) < 0) {
		return -1;
	}
	pkt1 = av_malloc(sizeof(AVPacketList));
	if (!pkt1)
		return -1;
	pkt1->pkt = *pkt;
	pkt1->next = NULL;

	SDL_LockMutex(q->mutex);

	if (!q->last_pkt)
		q->first_pkt = pkt1;
	else
		q->last_pkt->next = pkt1;
	q->last_pkt = pkt1;
	q->nb_packets++;
	q->size += pkt1->pkt.size;
	SDL_CondSignal(q->cond);

	SDL_UnlockMutex(q->mutex);
	return 0;
}


static int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block)
{
	AVPacketList *pkt1;
	int ret;

	SDL_LockMutex(q->mutex);

	for(;;) {

		if(global_video_state->quit) {
			ret = -1;
			break;
		}

		pkt1 = q->first_pkt;
		if (pkt1) {
			q->first_pkt = pkt1->next;
			if (!q->first_pkt)
				q->last_pkt = NULL;
			q->nb_packets--;
			q->size -= pkt1->pkt.size;
			*pkt = pkt1->pkt;
			av_free(pkt1);
			ret = 1;
			break;
		} else if (!block) {
			ret = 0;
			break;
		} else {
			SDL_CondWait(q->cond, q->mutex);
		}
	}
	SDL_UnlockMutex(q->mutex);
	return ret;
}
double get_audio_clock(VideoState *is) {
	double pts;
	int hw_buf_size, bytes_per_sec, n;

	pts = is->audio_clock; /* maintained in the audio thread */
	hw_buf_size = is->audio_buf_size - is->audio_buf_index;
	bytes_per_sec = 0;
	n = is->audio_st->codec->channels * 2;
	if(is->audio_st) {
		bytes_per_sec = is->audio_st->codec->sample_rate * n;
	}
	if(bytes_per_sec) {
		pts -= (double)hw_buf_size / bytes_per_sec;
	}
	return pts;
}
double get_video_clock(VideoState *is) {
	double delta;

	delta = (av_gettime() - is->video_current_pts_time) / 1000000.0;
	return is->video_current_pts + delta;
}
double get_external_clock(VideoState *is) {
	return av_gettime() / 1000000.0;
}

double get_master_clock(VideoState *is) {
	if(is->av_sync_type == AV_SYNC_VIDEO_MASTER) {
		return get_video_clock(is);
	} else if(is->av_sync_type == AV_SYNC_AUDIO_MASTER) {
		return get_audio_clock(is);
	} else {
		return get_external_clock(is);
	}
}
/* Add or subtract samples to get a better sync, return new
   audio buffer size */
int synchronize_audio(VideoState *is, short *samples,
		int samples_size, double pts) {
	int n;
	double ref_clock;

	n = 2 * is->audio_st->codec->channels;

	if(is->av_sync_type != AV_SYNC_AUDIO_MASTER) {
		double diff, avg_diff;
		int wanted_size, min_size, max_size, nb_samples;

		ref_clock = get_master_clock(is);
		diff = get_audio_clock(is) - ref_clock;

		if(diff < AV_NOSYNC_THRESHOLD) {
			// accumulate the diffs
			is->audio_diff_cum = diff + is->audio_diff_avg_coef
				* is->audio_diff_cum;
			if(is->audio_diff_avg_count < AUDIO_DIFF_AVG_NB) {
				is->audio_diff_avg_count++;
			} else {
				avg_diff = is->audio_diff_cum * (1.0 - is->audio_diff_avg_coef);
				if(fabs(avg_diff) >= is->audio_diff_threshold) {
					wanted_size = samples_size + ((int)(diff * is->audio_st->codec->sample_rate) * n);
					min_size = samples_size * ((100 - SAMPLE_CORRECTION_PERCENT_MAX) / 100);
					max_size = samples_size * ((100 + SAMPLE_CORRECTION_PERCENT_MAX) / 100);
					if(wanted_size < min_size) {
						wanted_size = min_size;
					} else if (wanted_size > max_size) {
						wanted_size = max_size;
					}
					if(wanted_size < samples_size) {
						/* remove samples */
						samples_size = wanted_size;
					} else if(wanted_size > samples_size) {
						uint8_t *samples_end, *q;
						int nb;

						/* add samples by copying final sample*/
						nb = (samples_size - wanted_size);
						samples_end = (uint8_t *)samples + samples_size - n;
						q = samples_end + n;
						while(nb > 0) {
							memcpy(q, samples_end, n);
							q += n;
							nb -= n;
						}
						samples_size = wanted_size;
					}
				}
			}
		} else {
			/* difference is TOO big; reset diff stuff */
			is->audio_diff_avg_count = 0;
			is->audio_diff_cum = 0;
		}
	}
	return samples_size;
}

int audio_decode_frame(VideoState *is, uint8_t *audio_buf, int buf_size, double *pts_ptr) {

	int len1, data_size, n;
	AVPacket *pkt = &is->audio_pkt;
	double pts;

	for(;;) {
		while(is->audio_pkt_size > 0) {
			data_size = buf_size;
			len1 = avcodec_decode_audio3(is->audio_st->codec, 
					(int16_t *)audio_buf, &data_size, 
					pkt);
			if(len1 < 0) {
				/* if error, skip frame */
				is->audio_pkt_size = 0;
				break;
			}
			is->audio_pkt_data += len1;
			is->audio_pkt_size -= len1;
			if(data_size <= 0) {
				/* No data yet, get more frames */
				continue;
			}
			pts = is->audio_clock;
			*pts_ptr = pts;
			n = 2 * is->audio_st->codec->channels;
			is->audio_clock += (double)data_size /
				(double)(n * is->audio_st->codec->sample_rate);
			/* We have data, return it and come back for more later */
			return data_size;
		}
		if(pkt->data)
			av_free_packet(pkt);

		if(is->quit) {
			return -1;
		}
		/* next packet */
		if(packet_queue_get(&is->audioq, pkt, 1) < 0) {
			return -1;
		}
		is->audio_pkt_data = pkt->data;
		is->audio_pkt_size = pkt->size;
		/* if update, update the audio clock w/pts */
		if(pkt->pts != AV_NOPTS_VALUE) {
			is->audio_clock = av_q2d(is->audio_st->time_base)*pkt->pts;
		}
	}
}

void audio_callback(void *userdata, Uint8 *stream, int len) {

	VideoState *is = (VideoState *)userdata;
	int len1, audio_size;
	double pts;

	while(len > 0) {
		if(is->audio_buf_index >= is->audio_buf_size) {
			/* We have already sent all our data; get more */
			audio_size = audio_decode_frame(is, is->audio_buf, sizeof(is->audio_buf), &pts);
			if(audio_size < 0) {
				/* If error, output silence */
				is->audio_buf_size = 1024;
				memset(is->audio_buf, 0, is->audio_buf_size);
			} else {
				audio_size = synchronize_audio(is, (int16_t *)is->audio_buf,
						audio_size, pts);
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

static Uint32 sdl_refresh_timer_cb(Uint32 interval, void *opaque) {
	SDL_Event event;
	event.type = FF_REFRESH_EVENT;
	event.user.data1 = opaque;
	SDL_PushEvent(&event);
	return 0; /* 0 means stop timer */
}

/* schedule a video refresh in 'delay' ms */
static void schedule_refresh(VideoState *is, int delay) {
	SDL_AddTimer(delay, sdl_refresh_timer_cb, is);
}

void video_display(VideoState *is) {

	SDL_Rect rect;
	VideoPicture *vp;
	AVPicture pict;
	float aspect_ratio;
	int w, h, x, y;
	int i;

	vp = &is->pictq[is->pictq_rindex];
	if(vp->bmp) {
		if(is->video_st->codec->sample_aspect_ratio.num == 0) {
			aspect_ratio = 0;
		} else {
			aspect_ratio = av_q2d(is->video_st->codec->sample_aspect_ratio) *
				is->video_st->codec->width / is->video_st->codec->height;
		}
		if(aspect_ratio <= 0.0) {
			aspect_ratio = (float)is->video_st->codec->width /
				(float)is->video_st->codec->height;
		}
		h = screen->h;
		w = ((int)rint(h * aspect_ratio)) & -3;
		if(w > screen->w) {
			w = screen->w;
			h = ((int)rint(w / aspect_ratio)) & -3;
		}
		x = (screen->w - w) / 2;
		y = (screen->h - h) / 2;

		rect.x = x;
		rect.y = y;
		rect.w = screen->w;
		rect.h = screen->h;
		SDL_DisplayYUVOverlay(vp->bmp, &rect);
	}
}

void video_refresh_timer(void *userdata) {

	VideoState *is = (VideoState *)userdata;
	VideoPicture *vp;
	double actual_delay, delay, sync_threshold, ref_clock, diff;

	if(is->video_st) {
		if(is->pictq_size == 0) {
			schedule_refresh(is, 1);
		} else {
			vp = &is->pictq[is->pictq_rindex];

			is->video_current_pts = vp->pts;
			is->video_current_pts_time = av_gettime();

			delay = vp->pts - is->frame_last_pts; /* the pts from last time */
			if(delay <= 0 || delay >= 1.0) {
				/* if incorrect delay, use previous one */
				delay = is->frame_last_delay;
			}
			/* save for next time */
			is->frame_last_delay = delay;
			is->frame_last_pts = vp->pts;

			/* update delay to sync to audio if not master source */
			if(is->av_sync_type != AV_SYNC_VIDEO_MASTER) {
				ref_clock = get_master_clock(is);
				diff = vp->pts - ref_clock;

				/* Skip or repeat the frame. Take delay into account
				   FFPlay still doesn't "know if this is the best guess." */
				sync_threshold = (delay > AV_SYNC_THRESHOLD) ? delay : AV_SYNC_THRESHOLD;
				if(fabs(diff) < AV_NOSYNC_THRESHOLD) {
					if(diff <= -sync_threshold) {
						delay = 0;
					} else if(diff >= sync_threshold) {
						delay = 2 * delay;
					}
				}
			}

			is->frame_timer += delay;
			/* computer the REAL delay */
			actual_delay = is->frame_timer - (av_gettime() / 1000000.0);
			if(actual_delay < 0.010) {
				/* Really it should skip the picture instead */
				actual_delay = 0.010;
			}
			schedule_refresh(is, (int)(actual_delay * 1000 + 0.5));
			/* show the picture! */
			video_display(is);

			/* update queue for next picture! */
			if(++is->pictq_rindex == VIDEO_PICTURE_QUEUE_SIZE) {
				is->pictq_rindex = 0;
			}
			SDL_LockMutex(is->pictq_mutex);
			is->pictq_size--;
			SDL_CondSignal(is->pictq_cond);
			SDL_UnlockMutex(is->pictq_mutex);
		}
	} else {
		schedule_refresh(is, 100);
	}
}

void alloc_picture(void *userdata)
{
	VideoState *is = (VideoState *)userdata;
	VideoPicture *vp;

	vp = &is->pictq[is->pictq_windex];
	if (vp->bmp)
	{
		SDL_FreeYUVOverlay(vp->bmp);
	}

	vp->bmp = SDL_CreateYUVOverlay(is->video_st->codec->width,
			is->video_st->codec->height,
			SDL_YV12_OVERLAY,
			screen);
	vp->width = is->video_st->codec->width;
	vp->height = is->video_st->codec->height;

	SDL_LockMutex(is->pictq_mutex);
	vp->allocated = 1;
	SDL_CondSignal(is->pictq_cond);
	SDL_UnlockMutex(is->pictq_mutex);
}

int queue_picture(VideoState *is, AVFrame *pFrame, double pts) 
{
	VideoPicture *vp;
	int dst_pix_fmt;
	AVPicture pict;

	SDL_LockMutex(is->pictq_mutex);
	while (is->pictq_size >= VIDEO_PICTURE_QUEUE_SIZE &&
			!is->quit)
	{
		SDL_CondWait(is->pictq_cond,is->pictq_mutex);
	}
	SDL_UnlockMutex(is->pictq_mutex);

	if(is->quit)
	{
		return -1;
	}

	vp = &is->pictq[is->pictq_windex];

	if(!vp->bmp ||
			vp->width != is->video_st->codec->width ||
			vp->height != is->video_st->codec->height)
	{
		SDL_Event event;
		vp->allocated = 0;
		event.type = FF_ALLOC_EVENT;
		event.user.data1 = is;
		SDL_PushEvent(&event);

		SDL_LockMutex(is->pictq_mutex);
		while (!vp->allocated && !is->quit)
		{
			SDL_CondWait(is->pictq_cond, is->pictq_mutex);
		}
		SDL_UnlockMutex(is->pictq_mutex);
		if (is->quit)
		{
			return -1;
		}
	}

	if (vp->bmp)
	{
		//		long t1,t2;
		//		double duration;
		SDL_LockYUVOverlay(vp->bmp);

		dst_pix_fmt = PIX_FMT_YUV420P;

		pict.data[0] = vp->bmp->pixels[0];
		pict.data[1] = vp->bmp->pixels[2];
		pict.data[2] = vp->bmp->pixels[1];

		pict.linesize[0] = vp->bmp->pitches[0];
		pict.linesize[1] = vp->bmp->pitches[2];
		pict.linesize[2] = vp->bmp->pitches[1];

		//		t1 = clock();

		is->img_convert_ctx = sws_getCachedContext(
				is->img_convert_ctx,
				vp->width,vp->height,is->video_st->codec->pix_fmt,
				vp->width,vp->height,PIX_FMT_YUV420P,
				SWS_FAST_BILINEAR,NULL,
				NULL,NULL
				);

		sws_scale(is->img_convert_ctx,
				(const uint8_t * const*)pFrame->data,
				pFrame->linesize,
				0,
				is->video_st->codec->height,
				pict.data,
				pict.linesize);

		//		t2 = clock();
		//		duration = (double)(t2 - t1) / CLOCKS_PER_SEC;
		//		ERROR("img_convert_ctx and sws_scale using time is %f",(duration));

		SDL_UnlockYUVOverlay(vp->bmp);
		vp->pts = pts;

		if (++is->pictq_windex == VIDEO_PICTURE_QUEUE_SIZE)
		{
			is->pictq_windex = 0;
		}
		SDL_LockMutex(is->pictq_mutex);
		is->pictq_size++;
		SDL_UnlockMutex(is->pictq_mutex);
	}
	return 0;
}


double synchronize_video(VideoState *is, AVFrame *src_frame, double pts) {

	double frame_delay;

	if(pts != 0) {
		/* if we have pts, set video clock to it */
		is->video_clock = pts;
	} else {
		/* if we aren't given a pts, set it to the clock */
		pts = is->video_clock;
	}
	/* update the video clock */
	frame_delay = av_q2d(is->video_st->codec->time_base);
	/* if we are repeating a frame, adjust clock accordingly */
	frame_delay += src_frame->repeat_pict * (frame_delay * 0.5);
	is->video_clock += frame_delay;
	return pts;
}
uint64_t global_video_pkt_pts = AV_NOPTS_VALUE;

/* These are called whenever we allocate a frame
 * buffer. We use this to store the global_pts in
 * a frame at the time it is allocated.
 */
int our_get_buffer(struct AVCodecContext *c, AVFrame *pic) {
	int ret = avcodec_default_get_buffer(c, pic);
	uint64_t *pts = av_malloc(sizeof(uint64_t));
	*pts = global_video_pkt_pts;
	pic->opaque = pts;
	return ret;
}
void our_release_buffer(struct AVCodecContext *c, AVFrame *pic) {
	if(pic) av_freep(&pic->opaque);
	avcodec_default_release_buffer(c, pic);
}
int video_thread(void *arg)
{
	VideoState *is = (VideoState *)arg;
	AVPacket pkt1, *packet = &pkt1;
	int len1,frameFinished;
	AVFrame *pFrame;
	double pts; 

	pFrame = avcodec_alloc_frame();

	for (;;)
	{
		if(packet_queue_get(&is->videoq, packet, 1) < 0)
		{
			break;
		}
		pts = 0;
		global_video_pkt_pts = packet->pts;
		len1 = avcodec_decode_video2(is->video_st->codec, pFrame, &frameFinished,
				packet);
		if(packet->dts == AV_NOPTS_VALUE 
				&& pFrame->opaque && *(uint64_t*)pFrame->opaque != AV_NOPTS_VALUE) {
			pts = *(uint64_t *)pFrame->opaque;
		} else if(packet->dts != AV_NOPTS_VALUE) {
			pts = packet->dts;
		} else {
			pts = 0;
		}
		pts *= av_q2d(is->video_st->time_base);

		// Did we get a video frame?
		if(frameFinished) {
			pts = synchronize_video(is, pFrame, pts);
			if(queue_picture(is, pFrame, pts) < 0) {
				break;
			}
		}
		av_free_packet(packet);
	}
	av_free(pFrame);
	return 0;
}

int stream_component_open(VideoState *is, int stream_index)
{
	AVFormatContext *pFormatCtx = is->pFormatCtx;
	AVCodecContext *codecCtx;
	AVCodec *codec;
	SDL_AudioSpec wanted_spec, spec;

	if(stream_index < 0 || stream_index >= pFormatCtx->nb_streams)
	{
		return -1;
	}

	codecCtx = pFormatCtx->streams[stream_index]->codec;

	if (codecCtx->codec_type == AVMEDIA_TYPE_AUDIO)
	{
		wanted_spec.freq = codecCtx->sample_rate;
		wanted_spec.format = AUDIO_S16SYS;
		wanted_spec.channels = codecCtx->channels;
		wanted_spec.silence = 0;
		wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;
		wanted_spec.callback = audio_callback;
		wanted_spec.userdata = is;
		if (SDL_OpenAudio(&wanted_spec, &spec) < 0)
		{
			TRACE("openaudio error");
			return -1;
		}
		is->audio_hw_buf_size = spec.size;
	}

	codec = avcodec_find_decoder(codecCtx->codec_id);
	if(!codec || (avcodec_open(codecCtx,codec) < 0))
	{
		TRACE("unsupport codec");
		return -1;
	}

	switch (codecCtx->codec_type)
	{
		case AVMEDIA_TYPE_AUDIO:
			is->audioStream = stream_index;
			is->audio_st = pFormatCtx->streams[stream_index];
			is->audio_buf_size = 0;
			is->audio_buf_index = 0;
			is->audio_diff_avg_coef = exp(log(0.01 / AUDIO_DIFF_AVG_NB));
			is->audio_diff_avg_count = 0;
			is->audio_diff_threshold = 2.0 * SDL_AUDIO_BUFFER_SIZE / codecCtx->sample_rate;
			memset(&is->audio_pkt, 0, sizeof(is->audio_pkt));
			packet_queue_init(&is->audioq);
			SDL_PauseAudio(0);
			break;
		case AVMEDIA_TYPE_VIDEO:
			is->videoStream = stream_index;
			is->video_st = pFormatCtx->streams[stream_index];
			is->frame_timer = (double)av_gettime() / 1000000.0;
			is->frame_last_delay = 40e-3;
			is->video_current_pts_time = av_gettime();
			packet_queue_init(&is->videoq);
			is->video_tid = SDL_CreateThread(video_thread, "video_thread", is);
			codecCtx->get_buffer = our_get_buffer;
			codecCtx->release_buffer = our_release_buffer;
			break;
		default:
			break;
	}
}

int decode_interrupt_cb(void)
{
	return (global_video_state && global_video_state->quit);
}

int decode_thread(void *arg)
{
	VideoState *is = (VideoState *)arg;
	AVFormatContext *pFormatCtx = NULL;
	AVPacket pkt1, *packet = &pkt1;

	int video_index = -1;
	int audio_index = -1;
	int i;

	is->videoStream = -1;
	is->audioStream = -1;

	global_video_state = is;

	if (avformat_open_input(&pFormatCtx,is->filename,NULL,NULL) != 0)
	{
		return -1;
	}

	is->pFormatCtx = pFormatCtx;

	if(av_find_stream_info(pFormatCtx) < 0)
	{
		return -1;
	}

	av_dump_format(pFormatCtx,0,is->filename,0);

	TRACE("pFormatCtx is here ");

	for (i = 0;i < pFormatCtx->nb_streams; i++)
	{
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO && 
				video_index < 0)
		{
			video_index = i;
		}
		if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO &&
				audio_index < 0)
		{
			audio_index = i;
		}
	}

	if(audio_index >= 0 )
	{
		stream_component_open(is, audio_index);
	}

	if (video_index >= 0)
	{
		stream_component_open(is, video_index);
	}

	if (is->videoStream < 0 || is->audioStream < 0)
	{
		TRACE("stream error");
		goto fail;
	}

	for (;;)
	{
		if (is->quit)
		{
			break;
		}

		if (is->audioq.size > MAX_AUDIOQ_SIZE ||
				is->videoq.size > MAX_VIDEOQ_SIZE)
		{
			SDL_Delay(10);
			continue;
		}

		if(av_read_frame(is->pFormatCtx,packet) < 0 )
		{
			if(pFormatCtx->pb && pFormatCtx->pb->error)
			{
				SDL_Delay(100);
				continue;
			}else{
				break;
			}
		}

		if(packet->stream_index == is->videoStream)
		{
			packet_queue_put(&is->videoq,packet);
		}else if (packet->stream_index == is->audioStream)
		{
			packet_queue_put(&is->audioq,packet);
		}else{
			av_free_packet(packet);
		}
	}

	while (!is->quit)
	{
		SDL_Delay(100);
	}

fail:
	{
		SDL_Event event;
		event.type = FF_QUIT_EVENT;
		event.user.data1 = is;
		SDL_PushEvent(&event);
	}
	return 0;
}


int main(int argc, char *argv[]) {

	SDL_Event event;
	VideoState *is;
	int width ,height;
	//char * filename = "F:/prisonbreak.rmvb";
	//	char * filename = "F:/service.mp4";
	const SDL_VideoInfo *vi;
	int fs_screen_width ;
	int fs_screen_height ;

	char * filename = argv[1];
	is = av_mallocz(sizeof(VideoState));

	width = atoi(argv[2]);
	height = atoi(argv[3]);

	TRACE("main filename is %s %d, argc is %d",filename,__LINE__,argc);
	// Register all formats and codecs
	ERROR("%d",__LINE__);
	av_register_all();

	ERROR("%d",__LINE__);
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
	{
		TRACE("could not initialize sdl");
		return 0;
	}

	screen = SDL_SetVideoMode(width, height,0,SDL_FULLSCREEN);
	if(!screen)
	{
		TRACE("screen error");
		exit(1);
	}

	is->width = screen->w;
	is->height = screen->h;

	vi = SDL_GetVideoInfo();	
	fs_screen_width = vi->current_w;
	fs_screen_height = vi->current_h;
	ERROR("sdl vi fs_screen_width is %d,fs_screen_height is %d",fs_screen_width,fs_screen_height);

	av_strlcpy(is->filename,filename,sizeof(is->filename));

	is->pictq_mutex = SDL_CreateMutex();
	is->pictq_cond = SDL_CreateCond();

	schedule_refresh(is, 40);

	is->parse_tid = SDL_CreateThread(decode_thread, "decode_thread",is);

	if(!is->parse_tid)
	{
		av_free(is);
		return -1;
	}

	for(;;) {

		SDL_WaitEvent(&event);
		switch(event.type) {
			case FF_QUIT_EVENT:
			case SDL_QUIT:
				ERROR("get quit event");
				is->quit = 1;
				SDL_Quit();
				exit(0);
				break;
			case FF_ALLOC_EVENT:
				alloc_picture(event.user.data1);
				break;
			case FF_REFRESH_EVENT:
				video_refresh_timer(event.user.data1);
				break;
			default:
				break;
		}
	}
	return 0;
}
