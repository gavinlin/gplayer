#include <stdlib.h>
#define FROM_ATRACK_CODE 1
#include "audiotrack.h"
#include <utils/Log.h>
#include <media/AudioTrack.h>
#include <media/AudioSystem.h>
#include <utils/Errors.h>

#ifdef ANDROID4_2.h
#include <system/audio.h>
#include <system/audio_policy.h>
#endif

#define TAG "AudioTrackWrapper"

namespace android {
	extern "C" {

		static AudioTrack* track;

		int AndroidAudioTrack_register() {
			__android_log_print(ANDROID_LOG_INFO, TAG, "registering audio track");
			track = new AudioTrack();
			if(track == NULL) {
				return ANDROID_AUDIOTRACK_RESULT_JNI_EXCEPTION;
			}
			__android_log_print(ANDROID_LOG_INFO, TAG, "registered");
			return ANDROID_AUDIOTRACK_RESULT_SUCCESS;
		}

		int AndroidAudioTrack_start() {
			//__android_log_print(ANDROID_LOG_INFO, TAG, "starting audio track");
			if(track == NULL) {
				return ANDROID_AUDIOTRACK_RESULT_ALLOCATION_FAILED;
			}
			track->start();
			return ANDROID_AUDIOTRACK_RESULT_SUCCESS;
		}
		int AndroidAudioTrack_set(int streamType,
				uint32_t sampleRate,
				int format,
				int channels,
				int frameCount,
				uint32_t flags,
				callback_t cbf,
				void* user) {
			if(track == NULL) {
				return ANDROID_AUDIOTRACK_RESULT_ALLOCATION_FAILED;
			}

			__android_log_print(ANDROID_LOG_INFO, TAG, "setting audio track");

#ifdef ANDROID4_2
			status_t ret = track->set((audio_stream_type_t)streamType, 
					sampleRate, 
					(audio_format_t)format, 
					(audio_channel_mask_t)channels, 
					frameCount, 
					(audio_output_flags_t)flags,
					cbf, 
					user);
#else
			status_t ret = track->set(streamType, 
					sampleRate, 
					format, 
					channels, 
					frameCount, 
					flags,
					cbf, 
					user);
#endif
			if (ret != NO_ERROR) {
				return ANDROID_AUDIOTRACK_RESULT_ERRNO;
			}
			__android_log_print(ANDROID_LOG_INFO, TAG, "setting audio track successed");
			return ANDROID_AUDIOTRACK_RESULT_SUCCESS;
		}

		int AndroidAudioTrack_flush() {
			if(track == NULL) {
				return ANDROID_AUDIOTRACK_RESULT_ALLOCATION_FAILED;
			}
			track->flush();
			return ANDROID_AUDIOTRACK_RESULT_SUCCESS;
		}

		int AndroidAudioTrack_stop() {
			if(track == NULL) {
				return ANDROID_AUDIOTRACK_RESULT_ALLOCATION_FAILED;
			}
			track->stop();
			return ANDROID_AUDIOTRACK_RESULT_SUCCESS;
		}

		int AndroidAudioTrack_reload() {
			if(track == NULL) {
				return ANDROID_AUDIOTRACK_RESULT_ALLOCATION_FAILED;
			}
			if(track->reload() != NO_ERROR) {
				return ANDROID_AUDIOTRACK_RESULT_ERRNO;
			}
			return ANDROID_AUDIOTRACK_RESULT_SUCCESS;
		}

		int AndroidAudioTrack_unregister() {
			__android_log_print(ANDROID_LOG_INFO, TAG, "unregistering audio track");
			if(!track->stopped()) {
				track->stop();
			}
			free(track);
			__android_log_print(ANDROID_LOG_INFO, TAG, "unregistered");
			return ANDROID_AUDIOTRACK_RESULT_SUCCESS;
		}

		//int AndroidAudioTrack_write(void *buffer, int buffer_size) {
		//	// give the data to the native AudioTrack object (the data starts at the offset)
		//    ssize_t written = 0;
		//    // regular write() or copy the data to the AudioTrack's shared memory?
		//    if (track->sharedBuffer() == 0) {
		//        written = track->write(buffer, buffer_size);
		//    } else {
		//		// writing to shared memory, check for capacity
		//		if ((size_t)buffer_size > track->sharedBuffer()->size()) {
		//			__android_log_print(ANDROID_LOG_INFO, TAG, "buffer size was too small");
		//			buffer_size = track->sharedBuffer()->size();
		//		}
		//		memcpy(track->sharedBuffer()->pointer(), buffer, buffer_size);
		//		written = buffer_size;
		//	}
		//	return written;
		//}

	}// extern "C"
}; //namespace android

