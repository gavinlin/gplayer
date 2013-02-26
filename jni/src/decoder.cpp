/*
 * =====================================================================================
 *
 *       Filename:  decoder.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2013年02月12日 16时49分22秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author: gavin 
 *   Organization:  
 *
 * =====================================================================================
 */
#include "decoder.h"
#include "trace.h"

IDecoder::IDecoder(AVStream* stream){
	mQueue = new PacketQueue();
	mStream = stream;
}

IDecoder::~IDecoder(){
	if(mRunning){
		stop();
	}
	free(mQueue);
	avcodec_close(mStream->codec);
}

void IDecoder::enqueue(AVPacket* packet){
	mQueue->put(packet);
}

int IDecoder::packets(){
	return mQueue->size();
}

void IDecoder::stop(){
	mQueue->abort();
	int ret = -1;
	if((ret = wait()) !=0 ){
		ERROR("couldn't cancel IDecoder %d",ret);
		return;
	}
}

void IDecoder::handleRun(void* ptr){
	ERROR("handleRun");
	if(!prepare()){
		ERROR("couldn't prepare decoder");
		return;
	}
		TRACE("decoder mRunning is %d",mRunning);
	decode(ptr);
}

bool IDecoder::prepare(){
	return false;
}

bool IDecoder::process(AVPacket* packet){
	return false;
}

bool IDecoder::decode(void* ptr){
	return false;
}

