/*
 * =====================================================================================
 *
 *       Filename:  thread.cpp
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2013年02月12日 16时48分30秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#include "thread.h"
#include "trace.h"

Thread::Thread(){
	pthread_mutex_init(&mLock, NULL);
	pthread_cond_init(&mCondition, NULL);
}

Thread::~Thread(){
	pthread_mutex_destroy(&mLock);
	pthread_cond_destroy(&mCondition);
}

void Thread::start(){
	handleRun(NULL);
}

void Thread::startAsync(){
	pthread_create(&mThread, NULL, startThread, this);
}

int Thread::wait(){
	if(!mRunning){
		return 0;
	}
	return pthread_join(mThread, NULL);
}

void Thread::stop(){

}

void* Thread::startThread(void* ptr){
	TRACE("startThread");
	Thread* thread = (Thread*)ptr;
	thread->mRunning = true;
	thread->handleRun(ptr);
	thread->mRunning = false;
}

void Thread::waitOnNotify(){
	pthread_mutex_lock(&mLock);
	pthread_cond_wait(&mCondition,&mLock);
	pthread_mutex_unlock(&mLock);
}

void Thread::notify(){
	pthread_mutex_lock(&mLock);
	pthread_cond_signal(&mCondition);
	pthread_mutex_unlock(&mLock);
}

void Thread::handleRun(void* ptr){

}

