/*
 * =====================================================================================
 *
 *       Filename:  refreshthread.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2013年02月20日 01时23分10秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef _GPLAYER_REFRESHTHREAD
#define _GPLAYER_REFRESHTHREAD
#include "thread.h"
#include "videodecoder.h"

class RefreshThread : public Thread{
	public:
		RefreshThread(DecoderVideo *decoderVideo);
		~RefreshThread();
		void stop();
		void handleRun(void* ptr);

	private:
		DecoderVideo* mDecoderVideo;
};

#endif
