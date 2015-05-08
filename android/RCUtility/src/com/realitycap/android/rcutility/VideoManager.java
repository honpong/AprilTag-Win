package com.realitycap.android.rcutility;

import android.graphics.ImageFormat;
import android.hardware.Camera;
import android.hardware.Camera.PreviewCallback;
import android.hardware.Camera.Size;
import android.util.Log;
import android.widget.FrameLayout;

public class VideoManager
{
	protected CameraSurfaceView cameraPreview;
	protected Camera camera;
	protected boolean isRunning = false;
	
	public VideoManager()
	{
		// TODO Auto-generated constructor stub
	}
	
	protected Camera getCameraInstance()
	{
		Camera c = null;
		try
		{
			c = Camera.open(0); // attempt to get a Camera instance
		}
		catch (Exception e)
		{
			// Camera is not available (in use or does not exist)
			Log.e(MyApplication.TAG, e.toString());
		}
		return c; // returns null if camera is unavailable
	}
	
	public void startVideo(FrameLayout previewView)
	{		
		if (isRunning) return;
		Log.d(MyApplication.TAG, "startVideo");
		
		camera = getCameraInstance();
		if (camera == null)
		{
			Log.e(MyApplication.TAG, "Failed to access camera");
			return;
		}
		
		camera.setDisplayOrientation(90);
		
		final Camera.Parameters parameters = camera.getParameters();
		parameters.setPreviewFormat(ImageFormat.NV21);
		parameters.setFocusMode(Camera.Parameters.FOCUS_MODE_INFINITY);
		parameters.setPreviewSize(640, 480);
		camera.setParameters(parameters);
		
//		imageFormat.setText(getImageFormatString(parameters.getPreviewFormat()));
//		supportedFormats.setText("Supported\n" + getSupportedImageFormatsString(parameters));
		
		cameraPreview = new CameraSurfaceView(MyApplication.getContext(), camera);
		previewView.removeAllViews();
		previewView.addView(cameraPreview);
		
		final Size previewSize = parameters.getPreviewSize();
		final int dataBufferSize = (int) (previewSize.height * previewSize.width * (ImageFormat.getBitsPerPixel(parameters.getPreviewFormat()) / 8.0));
		final int bufferCount = 8;
		for (int i = 0; i < bufferCount; i++)
		{
			camera.addCallbackBuffer(new byte[dataBufferSize]);
		}
		
		camera.setPreviewCallbackWithBuffer(new PreviewCallback() {
			@Override
			public void onPreviewFrame(final byte[] data, Camera camera)
			{
				Log.d(MyApplication.TAG, "Camera frame received");
				camera.addCallbackBuffer(data);
			}
		});
		
		camera.startPreview();
		isRunning = true;
	}
	
	public void stopVideo()
	{
		if (!isRunning) return;
		Log.d(MyApplication.TAG, "stopVideo");
		
		camera.setPreviewCallback(null);
		camera.stopPreview();
		camera.release();
		camera = null;
		cameraPreview.close();
		cameraPreview = null;
		isRunning = false;
	}
	
//	protected String getSupportedImageFormatsString(Camera.Parameters parameters)
//	{
//		final StringBuilder sb = new StringBuilder(parameters.getSupportedPreviewFormats().size());
//		for (Integer i : parameters.getSupportedPreviewFormats())
//		{
//			sb.append(getImageFormatString(i));
//			sb.append("\n");
//		}
//		return sb.toString();
//	}
	
	protected String getImageFormatString(int i)
	{
		switch (i)
		{
			case ImageFormat.JPEG:
				return "JPEG";
			case ImageFormat.NV16:
				return "NV16";
			case ImageFormat.NV21:
				return "NV21";
			case ImageFormat.RGB_565:
				return "RGB_565";
			case ImageFormat.UNKNOWN:
				return "UNKNOWN";
//			case ImageFormat.YUV_420_888:
//				return "YUV_420_888";
			case ImageFormat.YUY2:
				return "YUY2";
			case ImageFormat.YV12:
				return "YV12";
			default:
				return "";
		}
	}
}
