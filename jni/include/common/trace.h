
#ifndef TRACE_H
#define TRACE_H

#include <android/log.h>

#define DEBUG_TRACE

#ifdef DEBUG_TRACE

#define TRACE(args...) \
	__android_log_print(ANDROID_LOG_INFO,"gPlayer", args)

#define DEBUG(args...) \
	__android_log_print(ANDROID_LOG_DEBUG, "gPlayer", args)

#define ERROR(args...) \
	__android_log_print(ANDROID_LOG_ERROR, "gPlayer", args)

#define INFO(args...) \
	__android_log_print(ANDROID_LOG_INFO, "gPlayer", args)

#else

#define TRACE(args...)
#define DEBUG(args...)
#define ERROR(args...)
#define INFO(args...)

#endif //DEBUG_TRACE

#endif //TRACE_H
