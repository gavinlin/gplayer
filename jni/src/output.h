/*
 * =====================================================================================
 *
 *       Filename:  output.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2013年02月12日 16时47分26秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef _GPLAYER_OUTPUT_H
#define	_GPLAYER_OUTPUT_H

#include <jni.h>
#include "audiotrack.h"
#include "surface.h"
typedef void (*callback_t)(int event, void* user, void *info);

class Output{
	public:
		static int AudioDriver_register();
		static int AudioDriver_set(int streamType,
									uint32_t smapleRate,
									int format,
									int channels,
									int frameCount,
									uint32_t flags,
									callback_t cbf,
									void* user);
		static int AudioDriver_start();
		static int AudioDriver_flush();
		static int AudioDriver_stop();
		static int AudioDriver_reload();
		static int AudioDriver_write(void* buffer, int buffer_size);
		static int AudioDriver_unregister();

		static int VideoDriver_register(JNIEnv* env, jobject jsurface);
		static int VideoDriver_getPixels(int width, int height, void** pixels);
		static int VideoDriver_updateSurface();
		static int VideoDriver_unregister();
};

#endif // _GPLAYER_OUTPUT_H

