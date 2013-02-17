package com.lingavin.gplayer;

import com.lingavin.gplayer.views.MovieView;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.view.Window;

public class GPlayerActivity extends Activity {

	private MovieView mMovieView = null;
	private String mFilePath;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		requestWindowFeature(Window.FEATURE_NO_TITLE);
		
		getFilePath();
		
		try{
			initViews();
		}catch (NullPointerException e) {
			finish();
		}
		
		mMovieView.setPath(mFilePath);
		setContentView(mMovieView);
	}

	private void getFilePath() {
		Intent intent = getIntent();
		Uri uri = intent.getData();
		mFilePath = uri.getPath();
		if(mFilePath == null){
			finish();
		}
	}

	private void initViews() throws NullPointerException{
		mMovieView = new MovieView(this);
		if(mMovieView == null)
			throw new NullPointerException();
	}

	@Override
	public void finish() {
		// TODO Auto-generated method stub
		super.finish();
	}

	
}
