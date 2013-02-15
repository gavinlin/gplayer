/*
 * =====================================================================================
 *
 *       Filename:  thread.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2013年02月12日 16时48分42秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef _GPLAYER_THREAD_H
#define _GPLAYER_THREAD_H

#include <pthread.h>

class Thread{
	public:
		Thread();
		~Thread();
		void start();
		void startAsync();
		int wait();

		void waitOnNotify();
		void notify();
		virtual void stop();

	protected:
		bool mRunning;
		virtual void handleRun(void* ptr);

	private:
		pthread_t mThread;
		pthread_mutex_t mLock;
		pthread_cond_t mCondition;

		static void* startThread(void* ptr);
};

#endif

