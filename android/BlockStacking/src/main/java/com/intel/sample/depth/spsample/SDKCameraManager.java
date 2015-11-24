/********************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION This software is supplied under the
terms of a license agreement or nondisclosure agreement with Intel Corporation
and may not be copied or disclosed except in accordance with the terms of that
agreement.
Copyright(c) 2011-2015 Intel Corporation. All Rights Reserved.

*********************************************************************************/

package com.intel.sample.depth.spsample;

import java.util.concurrent.atomic.AtomicBoolean;

import android.content.Context;
import android.app.Activity;
import android.hardware.Sensor;
import android.os.Bundle;
import android.util.Log;
import android.util.Size;

import com.intel.camera.toolkit.depth.Camera;
import com.intel.camera.toolkit.depth.ImageSet;
import com.intel.camera.toolkit.depth.Module;
import com.intel.camera.toolkit.depth.OnSenseManagerHandler;
import com.intel.camera.toolkit.depth.Point2DF;
import com.intel.camera.toolkit.depth.RSException;
import com.intel.camera.toolkit.depth.StreamProfile;
import com.intel.camera.toolkit.depth.StreamProfileSet;
import com.intel.camera.toolkit.depth.StreamType;
import com.intel.camera.toolkit.depth.Camera.Calibration;
import com.intel.camera.toolkit.depth.sceneperception.SPCore;
import com.intel.camera.toolkit.depth.sceneperception.SPTypes;
import com.intel.camera.toolkit.depth.sceneperception.SPTypes.CameraStreamIntrinsics;
import com.intel.camera.toolkit.depth.sceneperception.SPTypes.SPInputStream;
import com.intel.camera.toolkit.depth.sceneperception.SPTypes.Status;
import com.intel.camera.toolkit.depth.sensemanager.SenseManager;
import com.intel.camera.toolkit.depth.sensemanager.IMUCaptureManager;
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
	 * enable or disable camera streaming.
	 * @param isEnabled whether to enable camera streaming.
	 */
	private void setEnabledStreaming(boolean isEnabled) {
		if (mSenseManager != null) {
	        try {	        	
	        	if (isEnabled) {
		        	if (!mIsCamRunning) {
		        		
		        		// Choose camera mode
		        	    if (mPlaybackFile == null){
		        	    	mSenseManager.enableStreams(mStreamHandler, mStreamProfileSet);
		        	    	
		        	    	
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
	 * closes delivery of IMU samples.
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
     * starts delivery of IMU samples.
     */
    private void startImuManager(){    	
    	try {
	    	if (mIMUManager == null) {
	        	mIMUManager = IMUCaptureManager.instance(mMainContext);
	    	}
	    	if (mIMUManager != null) {
	    		if ( !mIMUManager.enableSensor(Sensor.TYPE_GRAVITY)) {
	    			mDepthProcessor.setProgramStatus("ERROR - fail to acquire gravity sensor");
    				stopImuManager();
	    			Log.e(TAG, "Failed to acquire gravity sensor");
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
     * configures camera streaming profile based on specification by Scene Perception module.
     */
	private void configureStreaming() {	
		if (mSenseManager != null) {
			//start scene perception
			try {
				//switch to use different voxel resolution of Scene Perception
				mSenseManager.enableScenePerception(SPTypes.SP_LOW_RESOLUTION);
				mSPCore = mSenseManager.queryScenePerception();
			}
			catch (Exception e) {
	        	Log.e(TAG, "Exception:" + e.getMessage());
	        	e.printStackTrace();
		        mDepthProcessor.setProgramStatus("ERROR - fail to acquire scene perception module");			
			}				
			Log.d(TAG, "Scene Perception enabled");
			mStreamProfileSet = ((Module)mSPCore).getPreferredProfiles().get(0); //choose QVGA depth and rgb
			mDepthInputSize = new Size(mStreamProfileSet.get(StreamType.DEPTH).Width,
					mStreamProfileSet.get(StreamType.DEPTH).Height);
			mColorInputSize = new Size(mStreamProfileSet.get(StreamType.COLOR).Width,
					mStreamProfileSet.get(StreamType.COLOR).Height);
			mSyncedFramesListener = mDepthProcessor.getSyncedFramesListener(this, mDepthInputSize);
		}
		else {
			mDepthProcessor.setProgramStatus("ERROR - camera is not present or fail to acquire camera frames"); 
		}
	}

	/**
	 * callback function to be used by client of streaming to signal complete 
	 * processing of the frames so that these frames could be released.
	 */
	public synchronized void frameProcessCompleteCallback(){
		if (mColorImg != null && mCurImgSet != null) {
			mColorImg.release();
			mCurImgSet.releaseImage(mColorImg);
			mColorImg = null;
		}
		if (mDepthImg != null && mCurImgSet != null) {
			mDepthImg.release();
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
	private ImageSet mCurImgSet;
	private AtomicBoolean mIsDepthProcessorConfigured = new AtomicBoolean(false);
	OnSenseManagerHandler mStreamHandler = new OnSenseManagerHandler()
	{
		private int frameCount = 0;
		private SPInputStream curInput = new SPInputStream();
		@Override
		public void onNewSample(ImageSet images) {
			if (mIsDepthProcessorConfigured.get()) {
				frameCount++;
				mCurImgSet = images;
				Image color, depth;
				color = images.acquireImage(StreamType.COLOR);
				depth = images.acquireImage(StreamType.DEPTH);		
				if (color != null && depth != null) {
						mColorImg = color;
						mDepthImg = depth;
						curInput.setValues(mDepthImg.getImageBuffer(), mColorImg.getImageBuffer(), 
						mIMUManager.querySensorSamples(Sensor.TYPE_GRAVITY));
						mSyncedFramesListener.onSyncedFramesAvailable(curInput);						

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
			if (!mIsDepthProcessorConfigured.get()) {
				Calibration.Intrinsics colorParams = null; //intrinsics param of color camera
				Calibration.Intrinsics depthParams = null; //intrinsics param of depth camera
				float[] depthToColorExtrinsicTranslation = null;
				//obtain camera intrinsics data
					Calibration calib = info.getCalibrationData();
					if (calib != null){
					colorParams = calib.colorIntrinsics;
					depthParams = calib.depthIntrinsics;
					depthToColorExtrinsicTranslation = new float[]{
							calib.depthToColorExtrinsics.translation.x, 
							calib.depthToColorExtrinsics.translation.y, 
							calib.depthToColorExtrinsics.translation.z};
					
					//use hard-coded value for QVGA depth and QVGA color
					if (mPlaybackFile != null){
						colorParams = new Camera.Calibration.Intrinsics(); 
						colorParams.focalLength = new Point2DF(304.789948f, 304.789948f);
						colorParams.principalPoint = new Point2DF(169.810745f, 125.464630f);
						
						depthParams = new Camera.Calibration.Intrinsics();
						depthParams.focalLength = new Point2DF(310.735077f, 310.735077f);
						depthParams.principalPoint = new Point2DF(159.917892f, 119.500000f);
						depthToColorExtrinsicTranslation = new float[]{-58.962776184082031f, -0.015729120001196861f, 
								0.22349929809570313f};	
					}
				}
				else {
					mDepthProcessor.setProgramStatus("ERROR - camera calibration data is not accessible");
					return;
				}
				Status spStatus = mSPCore.setConfiguration( new CameraStreamIntrinsics(depthParams, mDepthInputSize), 
						new CameraStreamIntrinsics(colorParams, mColorInputSize), depthToColorExtrinsicTranslation, 
						SPTypes.SP_LOW_RESOLUTION);
				if (spStatus != Status.SP_STATUS_SUCCESS) {
					mDepthProcessor.setProgramStatus("ERROR - Scene Perception cannot be (re-)configured - " + spStatus);
				}
				else {
					mIsDepthProcessorConfigured.set(true);					
					mDepthProcessor.setProgramStatus("Scene Perception configured");
					mDepthProcessor.configureDepthProcess(SPTypes.SP_LOW_RESOLUTION, mDepthInputSize, mColorInputSize, 
						colorParams, depthParams, depthToColorExtrinsicTranslation);
					mDepthProcessor.setProgramStatus("Waiting for camera frames"); 	 
				}
			}
		}

		@Override
		public void onError(StreamProfileSet arg0, int arg1) {
			if (arg1 == RSException.PLAYBACK_EOF) {
				((Activity)mMainContext).finish();
			}
			else {				
				setEnabledStreaming(false);
				Log.e( TAG, "ERROR - camera is not present or fail to acquire camera frames");
				mDepthProcessor.setProgramStatus("ERROR - fail to acquire camera frames");
			}
		}
	};
}
