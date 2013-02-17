package com.lingavin.gplayer;

import java.lang.ref.WeakReference;

import android.view.Surface;
import android.view.SurfaceHolder;

public class MediaPlayer{
	
	private int mNativeContext;
	private SurfaceHolder mSurfaceHolder;
	private Surface mSurface;
	
	static{
		/**注意调用关系*/
		System.loadLibrary("ffmpeg");
		System.loadLibrary("mediaplayer");
		System.loadLibrary("mediaplayer_jni");
		nativeInit();
	}

	public MediaPlayer(){
		nativeSetup();
	}
	
	public native void setDataSource(String path);
	private native void setVideoSurface(Surface surface);
	private native void nativePrepare();
	private static native final void nativeInit();
	private native void nativeSetup();
	private native void nativeStart();
	
	public void setPath(String mFilePath) {
		setDataSource(mFilePath);
	}

	public void start() {
		
		nativeStart();
	}

	public void setDisplay(SurfaceHolder holder) {
		mSurfaceHolder = holder;
		if(mSurfaceHolder != null){
			mSurface = mSurfaceHolder.getSurface();
		}else{
			mSurface = null;
		}
		setVideoSurface(mSurface);
	}

	public void prepare() {
		nativePrepare();
	}

	public void suspend() {
		// TODO Auto-generated method stub
	}

	public void resume() {
		// TODO Auto-generated method stub
		
	}

	public void pause() {
		// TODO Auto-generated method stub
		
	}

	public boolean isPlaying() {
		// TODO Auto-generated method stub
		return false;
	}

	public int getDuration() {
		// TODO Auto-generated method stub
		return 0;
	}

	public int getCurrentPosition() {
		// TODO Auto-generated method stub
		return 0;
	}

}
