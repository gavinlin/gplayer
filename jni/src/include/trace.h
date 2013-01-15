
#ifndef TRACE_H
#define TRACE_H

#include <android/log.h>

#define DEBUG

#ifdef DEBUG

#define TRACE(args...) \
	__android_log_print(ANDROID_LOG_INFO,"iPlayer", args)

#define DEBUG(args...) \
	__android_log_print(ANDROID_LOG_DEBUG, "iPlayer", args)

#define ERROR(args...) \
	__android_log_print(ANDROID_LOG_ERROR, "iPlayer", args)

#define INFO(args...) \
	__android_log_print(ANDROID_LOG_INFO, "iPlayer", args)

#else

#define TRACE(args...)
#define DEBUG(args...)
#define ERROR(args...)
#define INFO(args...)

#endif //DEBUG

#endif //TRACE_H
