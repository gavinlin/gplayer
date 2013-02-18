package com.lingavin.gplayer.views;

import com.lingavin.gplayer.MediaPlayer;

import android.content.Context;
import android.util.AttributeSet;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.MediaController;
import android.widget.MediaController.MediaPlayerControl;

public class MovieView extends SurfaceView {

	private MediaPlayer mp;
	private MediaController mMediaController;
	private final String TAG = "MovieView";
	
	public MovieView(Context context, AttributeSet attrs, int defStyle) {
		super(context, attrs, defStyle);
		initVideoView();
	}

	public MovieView(Context context, AttributeSet attrs) {
		super(context, attrs);
		initVideoView();
	}

	public MovieView(Context context) {
		super(context);
		initVideoView();
	}

	private void initVideoView() {
		mp = new MediaPlayer();
		
		SurfaceHolder holder = getHolder();
		holder.addCallback(mMediaCallback);
	}
	
	public void setPath(String mFilePath) {
		mp.setPath(mFilePath);
	}

	private void openVideo(SurfaceHolder holder) {
		mp.setDisplay(holder);
		mp.prepare();
	}
	
	protected void startVideo() {
		attachMediaController();
		mp.start();
	}
	
	private void release(){
		mp.suspend();
	}

	private void attachMediaController() {
		mMediaController = new MediaController(getContext());
		View anchorView = this.getParent() instanceof View ? (View)this.getParent() : this;
		mMediaController.setMediaPlayer(mMediaPlayerControl);
		mMediaController.setAnchorView(anchorView);
		mMediaController.setEnabled(true);
	}
	
    public boolean onTouchEvent(android.view.MotionEvent event) {
    	if(!mMediaController.isShowing()) {
			mMediaController.show(3000);
			mMediaController.setEnabled(true);
		}
		return true;
    }
	
	MediaPlayerControl mMediaPlayerControl = new MediaPlayerControl() {
		
		@Override
		public void start() {
			mp.resume();
			Log.i(TAG, "start");
		}
		
		@Override
		public void seekTo(int pos) {
			// TODO Auto-generated method stub
			
		}
		
		@Override
		public void pause() {
			Log.i("gavin", "pause!!!!!!!!!!");
			mp.pause();
		}
		
		@Override
		public boolean isPlaying() {
			return mp.isPlaying();
		}
		
		@Override
		public int getDuration() {
			// TODO Auto-generated method stub
			return mp.getDuration();
		}
		
		@Override
		public int getCurrentPosition() {
			return mp.getCurrentPosition();
		}
		
		@Override
		public int getBufferPercentage() {
			// TODO Auto-generated method stub
			return 0;
		}
		
		@Override
		public boolean canSeekForward() {
			// TODO Auto-generated method stub
			return false;
		}
		
		@Override
		public boolean canSeekBackward() {
			// TODO Auto-generated method stub
			return false;
		}
		
		@Override
		public boolean canPause() {
			// TODO Auto-generated method stub
			return true;
		}
	};
	
	SurfaceHolder.Callback mMediaCallback = new SurfaceHolder.Callback() {
		
		@Override
		public void surfaceDestroyed(SurfaceHolder holder) {
			release();
			if(mMediaController.isShowing()){
				mMediaController.hide();
			}
		}
		
		@Override
		public void surfaceCreated(SurfaceHolder holder) {
			openVideo(holder);
		}
		
		@Override
		public void surfaceChanged(SurfaceHolder holder, int format, int width,
				int height) {
			startVideo();
		}
	};
}
