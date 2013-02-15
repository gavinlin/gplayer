/*
 * =====================================================================================
 *
 *       Filename:  packetqueue.h
 *
 *    Description:  from ffmpeg tutorial
 *
 *        Version:  1.0
 *        Created:  2013年02月14日 19时13分16秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Gavinlin 
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef _GPLAYER_PACKETQUEUE_H
#define _GPLAYER_PACKETQUEUE_H

#include <pthread.h>

extern "C"{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}

class PacketQueue{
	public:
		PacketQueue();
		~PacketQueue();

		void flush();
		int put(AVPacket* pkt);
		int get(AVPacket* pkt, bool block);
		int size();
		void abort();

	private:
		AVPacketList* mFirst;
		AVPacketList* mLast;
		int mNbPackets;
		int mSize;
		bool mAbortRequest;
		pthread_mutex_t mLock;
		pthread_cond_t mCondition;
};

#endif
