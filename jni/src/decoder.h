/*
 * =====================================================================================
 *
 *       Filename:  deocder.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2013年02月12日 16时49分33秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef _GPLAYER_DECODER_H
#define _GPLAYER_DECODER_H

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}
#include "thread.h"
#include "packetqueue.h"

class IDecoder : public Thread{
	public:
		IDecoder(AVStream* stream);
		~IDecoder();

		void stop();
		void enqueue(AVPacket* packet);
		int packets();
		PacketQueue* mQueue;
		AVStream* mStream;

	protected:

		virtual bool prepare();
		virtual bool decode(void* ptr);
		virtual bool process(AVPacket* packet);
		void handleRun(void* ptr);
};

#endif //_GPLAYER_DECODER_H

