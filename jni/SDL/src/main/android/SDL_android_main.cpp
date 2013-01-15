
#include "SDL_config.h"

#ifdef __ANDROID__

/* Include the SDL main definition header */
#include "SDL_main.h"

/*******************************************************************************
                 Functions called by JNI
*******************************************************************************/
#include <jni.h>
#include "trace.h"

// Called before SDL_main() to initialize JNI bindings in SDL library
extern "C" void SDL_Android_Init(JNIEnv* env, jclass cls);

// Library init
extern "C" jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    return JNI_VERSION_1_4;
}

// Start up the SDL app
extern "C" void Java_org_libsdl_app_SDLActivity_nativeInit(JNIEnv* env, jclass cls,jstring filename,jint width,jint height)
{
    /* This interface could expand with ABI negotiation, calbacks, etc. */
    SDL_Android_Init(env, cls);

    /* Run the application code! */
    int status;
    char *argv[5];
    char  width_char[128] ;
    char  height_char[128];
    char * mFilename;
    jboolean isCopy;
    sprintf(width_char,"%d",width);
    sprintf(height_char,"%d",height);
    mFilename = (char *)(env)->GetStringUTFChars(filename,&isCopy);
    ERROR("nativeInit in width is %d ,height is %d , width_char is %s, height_char is %s",width,height,width_char,height_char);
    argv[0] = strdup("SDL_app");
    argv[1] = mFilename;
    argv[2] = width_char;
    argv[3] = height_char;
    argv[4] = NULL;
    status = SDL_main(4, argv);

    (env)->ReleaseStringUTFChars(filename,mFilename);
    /* Do not issue an exit or the whole application will terminate instead of just the SDL thread */
    //exit(status);
}

#endif /* __ANDROID__ */

/* vi: set ts=4 sw=4 expandtab: */
