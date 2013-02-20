/*
 * =====================================================================================
 *
 *       Filename:  refreshthread.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2013年02月20日 01时14分31秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#include "refreshthread.h"

RefreshThread::RefreshThread(DecoderVideo* decoderVideo){
	mDecoderVideo = decoderVideo;
}

RefreshThread::~RefreshThread(){

}

void RefreshThread::handleRun(void* ptr){
	while(mRunning){
	
	}
}

void RefreshThread::stop(){

}

