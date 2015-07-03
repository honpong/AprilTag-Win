/********************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION This software is supplied under the
terms of a license agreement or nondisclosure agreement with Intel Corporation
and may not be copied or disclosed except in accordance with the terms of that
agreement.
Copyright(c) 2011-2015 Intel Corporation. All Rights Reserved.

*********************************************************************************/

package com.intel.sample.depth.spsample;

import android.content.Context;
import android.app.Activity;
import android.graphics.PointF;
import android.hardware.Sensor;
import android.os.Bundle;
import android.util.Log;
import android.util.Size;

import com.intel.camera.toolkit.depth.Camera;
import com.intel.camera.toolkit.depth.ImageSet;
import com.intel.camera.toolkit.depth.OnSenseManagerHandler;
import com.intel.camera.toolkit.depth.Point3DF;
import com.intel.camera.toolkit.depth.RSException;
import com.intel.camera.toolkit.depth.RSPixelFormat;
import com.intel.camera.toolkit.depth.StreamProfile;
import com.intel.camera.toolkit.depth.StreamProfileSet;
import com.intel.camera.toolkit.depth.StreamType;
import com.intel.camera.toolkit.depth.Camera.Calibration;
import com.intel.camera.toolkit.depth.sceneperception.SPCore;
import com.intel.camera.toolkit.depth.sceneperception.SPTypes.Configuration;
import com.intel.camera.toolkit.depth.sceneperception.SPTypes.Status;
import com.intel.camera.toolkit.depth.sensemanager.IMUCaptureManager;
import com.intel.camera.toolkit.depth.sensemanager.SenseManager;
import com.intel.camera.toolkit.depth.sensemanager.SensorSample;
import com.intel.camera.toolkit.depth.Image;

/**
 * SDKCameraManager handles camera bring-up, streaming configuration
 * and delivery of frames as well as camera events.
 */
public class SDKCameraManager extends CameraHandler {
	private static final String TAG = "SDKCameraManager";	
    private static final String PLAYBACK_KEY = "play";

	private SenseManager mSenseManager = null; //SDK SenseManager
    private IMUCaptureManager mIMUManager = null; //for capturing of gravity info    
	private volatile boolean mIsCamRunning = false;
	private StreamProfileSet mStreamProfileSet = null; //active camera streaming profile
	// Listener to camera frames. Depth and color frames must be time-synced. 
	private CameraSyncedFramesListener mSyncedFramesListener = null;
	private Context mMainContext;
    private String mPlaybackFile = null;
    private long mStartTime = 0L;
    private boolean firstFrame = true;
	
    public SDKCameraManager(Context mainContext,  
    		DepthProcessModule depthProcessor) {
    	super(mainContext, depthProcessor);
    	
    	try {
    		mMainContext = mainContext;
    		mSenseManager = new SenseManager(mMainContext);
    	}
    	catch(Exception e) {
    		mSenseManager = null;
    		Log.e(TAG, "ERROR - failed to acquire camera");
    	}
    	
        Bundle extras = ((Activity)mMainContext).getIntent().getExtras ( );

        if ( extras != null ) {
          if ( extras.containsKey ( PLAYBACK_KEY ) ) {
              mPlaybackFile = extras.getString ( PLAYBACK_KEY );
              Log.i(TAG, "Playback file: " + mPlaybackFile);
          }           
        }
    }
    
	@Override
	public void onStart() {
		configureStreaming();
		setEnabledStreaming(true);
	}
	
	@Override 
	public void onPause() {
		frameProcessCompleteCallback(); //release any currently acquired frames
		setEnabledStreaming(false);
	}

	@Override
	public void onResume() {
		setEnabledStreaming(true);
	}
	
	@Override
	public void onDestroy() {
		setEnabledStreaming(false);
	}
	
	/**
	 * enable or disable camera streaming
	 * @param isEnabled
	 */
	private void setEnabledStreaming(boolean isEnabled) {
		if (mSenseManager != null) {
	        try {	        	
	        	if (isEnabled) {
		        	if (!mIsCamRunning) {
		        		
		        		// Choose camera mode
		        	    if (mPlaybackFile == null){
		        	    	mSenseManager.enableStreams(mStreamHandler, mStreamProfileSet);
		        	    	
//		        	    	// Custom stream configuration request
//		        	    	StreamProfileSet profSet = new StreamProfileSet();
//			        	    //TODO: Remove hard-coded profiles
//			        	    profSet.set(new StreamProfile(640, 480, RSPixelFormat.RGBA_8888, 35, StreamType.COLOR));
//			        	    profSet.set(new StreamProfile(320, 240, RSPixelFormat.Z16,       35, StreamType.DEPTH));		        		
//			        	    mSenseManager.enableStreams(mStreamHandler, profSet);
		        	    	
		        	    } else {
		        	    	Log.i(TAG, "enableStreaming uses " + mPlaybackFile);
		        	        mSenseManager.enablePlaybackStreams(mStreamHandler, mPlaybackFile, /*loopback*/ false, /*realtime*/ false);
		        	    }
		        				        		
		                startImuManager();
				        mIsCamRunning = true;
			    		mDepthProcessor.setProgramStatus("Waiting to start camera streams"); 
		        	}
				}
	        	else if (mIsCamRunning) {
	                stopImuManager();
	        		mSenseManager.close();
					mIsCamRunning = false;
	        	}
	    	}
	        catch(Exception e) {
	        	Log.e(TAG, "Exception:" + e.getMessage());
	        	e.printStackTrace();
		        mDepthProcessor.setProgramStatus("ERROR - fail to acquire camera frames");
	        }
		}
		else {
			mDepthProcessor.setProgramStatus("ERROR - fail to acquire camera frames");
		}
	}
	
	/**
	 * Close delivery of IMU samples
	 */
    private void stopImuManager(){
	    try {
	    	if (mIMUManager != null) mIMUManager.close();
	    	mIMUManager = null;
		}
		catch (Exception e) {
			mIMUManager = null;
	    	Log.e(TAG, "Exception:" + e.getMessage());
		}    	
    }
    
    /**
     * start delivery of IMU samples
     */
    private void startImuManager(){    	
    	try {
	    	if (mIMUManager == null) {
	        	mIMUManager = IMUCaptureManager.instance(mMainContext);
	    	}
	    	if (mIMUManager != null) {
	    		if ( !mIMUManager.enableSensor(Sensor.TYPE_GRAVITY)) {
	    			mDepthProcessor.setProgramStatus("ERROR - fail to acquire gravity sensor");
	    			Log.e(TAG, "Failed to acquire gravity sensor");
	    		}
				if ( !mIMUManager.enableSensor(Sensor.TYPE_ACCELEROMETER)) {
					mDepthProcessor.setProgramStatus("ERROR - fail to acquire ameter sensor");
					Log.e(TAG, "Failed to acquire ameter sensor");
				}
				if ( !mIMUManager.enableSensor(Sensor.TYPE_GYROSCOPE_UNCALIBRATED)) {
					mDepthProcessor.setProgramStatus("ERROR - fail to acquire gyro sensor");
					Log.e(TAG, "Failed to acquire gyro sensor");
				}
	    	}
    	}
    	catch (Exception e) {
    		mIMUManager = null;
        	Log.e(TAG, "Exception:" + e.getMessage());
        	mDepthProcessor.setProgramStatus("ERROR - fail to acquire gravity sensor");
    	}
    }
    
    /**
     * configure camera streaming profile based on specification by Scene Perception module
     */
	private void configureStreaming() {	
		if (mSenseManager != null) {
			//start scene perception
			try {
				//switch to use different voxel resolution of Scene Perception
				mSenseManager.enableScenePerception(Configuration.SP_LOW_RESOLUTION);
				mSPCore = mSenseManager.queryScenePerception();
			}
			catch (Exception e) {
	        	Log.e(TAG, "Exception:" + e.getMessage());
	        	e.printStackTrace();
		        mDepthProcessor.setProgramStatus("ERROR - fail to acquire scene perception module");			
			}				
			Log.d(TAG, "Scene Perception enabled");
			mStreamProfileSet = SPUtils.createScenePerceptionStreamProfileSet(mSPCore);
			mInputSize = new Size(mStreamProfileSet.get(StreamType.DEPTH).Width,
					mStreamProfileSet.get(StreamType.DEPTH).Height);
			mSyncedFramesListener = mDepthProcessor.getSyncedFramesListener(this, mInputSize);
		}
		else {
			mDepthProcessor.setProgramStatus("ERROR - camera is not present or fail to acquire camera frames"); 
		}
	}

	/**
	 * callback function to be used by client of acquired frames to signal complete 
	 * processing of the frames so that these frames could be released.
	 */
	public void frameProcessCompleteCallback(){
		if (mColorImg != null && mCurImgSet != null) {
			mColorImg.releaseAccess();
			mCurImgSet.releaseImage(mColorImg);
			mColorImg = null;
		}
		if (mDepthImg != null && mCurImgSet != null) {
			mDepthImg.releaseAccess();
			mCurImgSet.releaseImage(mDepthImg);
			mDepthImg = null;
		}
		mCurImgSet = null;
	}
	
	@Override
	public SPCore queryScenePerception() {
		return mSenseManager.queryScenePerception();
	}
	
	private Image mColorImg;
	private Image mDepthImg;
	private Size QVGA = new Size(320, 240);
	private MyImage mColorImgQVGA = new MyImage(QVGA, RSPixelFormat.RGBA_8888);
	private MyImage mDepthImgQVGA = new MyImage(QVGA, RSPixelFormat.Z16);
	private ImageSet mCurImgSet;
	private volatile boolean mIsDepthProcessorConfigured = false;
	OnSenseManagerHandler mStreamHandler = new OnSenseManagerHandler()
	{
		private int frameCount = 0;
		@Override
		public void onNewSample(ImageSet images) {
			if (mIsDepthProcessorConfigured) {
				frameCount++;
				mCurImgSet = images;
				Image color, depth;
				color = images.acquireImage(StreamType.COLOR);
				depth = images.acquireImage(StreamType.DEPTH);		
				if (color != null && depth != null) {
				
					if (color.getInfo().ImgSize.equals(mColorImgQVGA.getInfo().ImgSize) == false){
						Log.i(TAG, "Color resolution is not set to QVGA. Image is being rescaled on the fly, which degrades performance");
//						SPBasicFragment.imageResize(color, mColorImgQVGA);
//						mColorImg = mColorImgQVGA;
					} else {
						mColorImg = color;
					}
					
					if (depth.getInfo().ImgSize.equals(mDepthImgQVGA.getInfo().ImgSize) == false){
						Log.i(TAG, "Depth resolution is not set to QVGA. Image is being rescaled on the fly, which degrades performance");
//						SPBasicFragment.imageResize(depth, mDepthImgQVGA);
//						mDepthImg = mDepthImgQVGA;
					} else {
						mDepthImg = depth;
					}

                    if (firstFrame) {
                        firstFrame = false;
                        mStartTime = color.getTimeStamp();
                    }
					mSyncedFramesListener.onSyncedFramesAvailable(mDepthImg.acquireAccess(), 
																  mColorImg.acquireAccess(), 
																  getLatestGravityVect());
				}
				else {
					mCurImgSet.releaseAllImages();
					mCurImgSet = null;
					mColorImg = null;
					mDepthImg = null;
					Log.e(TAG, "ERROR - fail to acquire camera frame at " + frameCount);
				}
			}
		}

		@Override
		public void onSetProfile(Camera.CaptureInfo info) {
			StreamProfile cs = info.getStreamProfiles().get(StreamType.COLOR);
			StreamProfile ds = info.getStreamProfiles().get(StreamType.DEPTH);	
			if(null == cs || null == ds) {
				mDepthProcessor.setProgramStatus("ERROR - camera cannot be configured");
				return;
			}
			
			//configure scene perception
			if (!mIsDepthProcessorConfigured) {
				
				//obtain camera intrinsics data
				try {
					Calibration calib = info.getCalibrationData();
					if (calib != null){
						mColorParams 		= calib.colorIntrinsics;
						mDepthParams 		= calib.depthIntrinsics;
						mDepthToColorParams = calib.depthToColorExtrinsics;
					}
					
					//TODO: Remove hard-coded calibration info.
					mColorParams = new Camera.Calibration.Intrinsics(); 
					mColorParams.focalLength = new PointF(304.789948f, 304.789948f);
					mColorParams.principalPoint = new PointF(169.810745f, 125.464630f);
					
					mDepthParams = new Camera.Calibration.Intrinsics();
					mDepthParams.focalLength = new PointF(310.735077f, 310.735077f);
					mDepthParams.principalPoint = new PointF(159.917892f, 119.500000f);
					
					mDepthToColorParams = new Camera.Calibration.Extrinsics();
					mDepthToColorParams.translation = new Point3DF(-58.962776184082031f, -0.015729120001196861f, 0.22349929809570313f);
				}
				catch(Exception e){
					Log.e(TAG, "ERROR - camera calibration data is not accessible");
				}

				float[] depthToColorExtrinsicTranslation = new float[]{-58.962776184082031f, -0.015729120001196861f, 0.22349929809570313f};
				if (mDepthToColorParams != null) {
						Point3DF t = mDepthToColorParams.translation;
						depthToColorExtrinsicTranslation = new float[]{t.x, t.y, t.z};
				}
				Configuration spConfiguration = SPUtils.createConfiguration(mInputSize, mColorParams, Configuration.SP_LOW_RESOLUTION);
				Status spStatus = mSPCore.setConfiguration(spConfiguration);
				if (spStatus != Status.SP_STATUS_SUCCESS) {
					mDepthProcessor.setProgramStatus("ERROR - Scene Perception can not be (re)configured - " + spStatus);
				}
				else {
					mDepthProcessor.setProgramStatus("Scene Perception configured");
					mDepthProcessor.configureDepthProcess(Configuration.SP_LOW_RESOLUTION, mInputSize, mColorParams, mDepthParams, depthToColorExtrinsicTranslation);
					mDepthProcessor.setProgramStatus("Waiting for camera frames"); 	 
					mIsDepthProcessorConfigured = true;
				}
			}
		}

		@Override
		public void onError(StreamProfileSet arg0, int arg1) {
			if (arg1 == RSException.PLAYBACK_EOF) ((Activity)mMainContext).finish();
			else {				
				setEnabledStreaming(false);
				Log.e( TAG, "ERROR - camera is not present or fail to acquire camera frames");
				mDepthProcessor.setProgramStatus("ERROR - fail to acquire camera frames");
			}
		}
	};
	
	/**
	 * Get the most recent gravity vector from sensor. 
	 * Note: Conversion must be made from coordinate system of gravity sensor into 
	 * Scene Perception's coordinate system.
	 * In particular, when camera is perfectly placed horizontally and viewing outward, the
	 * normalized value of gravity should be (x, y, z) ~ (0, 1, 0).
	 * Also, when the camera is rotated clock wise from such horizontal position to 
	 * be placed in vertically and view outward, the normalized value of gravity should
	 * be (x, y, z) ~ (1, 0, 0).
	 * 
	 * @return x, y, z coordinate values of the normalized gravity vector (magnitude of 1)
	 */
	private float[] getLatestGravityVect() {
		float[] mostRecentGrav = null;
		if (mIMUManager != null) {
			SensorSample[] samples = mIMUManager.querySensorSamples(Sensor.TYPE_GRAVITY);
			if (samples != null && samples.length > 0 && samples[0] != null) {
				
				//exchange x and y axes to convert from IMU coordinate to SP's coordinate
				mostRecentGrav = SPUtils.normalizeVector(samples[0].values()[1], samples[0].values()[0], 
						samples[0].values()[2]);
			}

			SensorSample[] ameterSamples = mIMUManager.querySensorSamples(Sensor.TYPE_ACCELEROMETER);
			if (ameterSamples != null && ameterSamples.length > 0 && ameterSamples[0] != null) {

				SensorSample aSample = ameterSamples[0];
				Log.v(TAG, "IMU:Accelerometer " + aSample.values()[0] + ", " + aSample.values()[1] + ", " + aSample.values()[2]);
			}

			SensorSample[] gyroSamples = mIMUManager.querySensorSamples(Sensor.TYPE_ACCELEROMETER);
			if (gyroSamples != null && gyroSamples.length > 0 && gyroSamples[0] != null) {

				SensorSample gSample = gyroSamples[0];
				Log.v(TAG, "IMU:Gyro " + gSample.values()[0] + ", " + gSample.values()[1] + ", " + gSample.values()[2]);
			}
		}
		return mostRecentGrav;
	}
}
