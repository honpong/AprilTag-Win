package com.realitycap.android.rcutility;

import java.io.IOException;

import android.content.Context;
import android.hardware.Camera;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class CameraSurfaceView extends SurfaceView implements SurfaceHolder.Callback
{
	private SurfaceHolder mHolder;
	private Camera mCamera;
	
	public CameraSurfaceView(Context context, Camera camera)
	{
		super(context);
		mCamera = camera;
		
		// Install a SurfaceHolder.Callback so we get notified when the underlying surface is created and destroyed.
		mHolder = getHolder();
		mHolder.addCallback(this);
	}
	
	public void surfaceCreated(SurfaceHolder holder)
	{
		// The Surface has been created, now tell the camera where to draw the preview.
		try
		{
			mCamera.setPreviewDisplay(holder);
		}
		catch (IOException e)
		{
			Log.d(MyApplication.TAG, "Error setting camera preview: " + e.getMessage());
		}
	}
	
	public void surfaceDestroyed(SurfaceHolder holder)
	{
		// empty. Take care of releasing the Camera preview in your activity.
	}
	
	public void surfaceChanged(SurfaceHolder holder, int format, int w, int h)
	{
		// If your preview can change or rotate, take care of those events here.
		// Make sure to stop the preview before resizing or reformatting it.
	}

	public void close()
	{
		Log.v(MyApplication.TAG, "close()");
		mHolder.removeCallback(this);
		mHolder = null;
	}
}
