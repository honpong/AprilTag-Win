/********************************************************************************

 INTEL CORPORATION PROPRIETARY INFORMATION This software is supplied under the
 terms of a license agreement or nondisclosure agreement with Intel Corporation
 and may not be copied or disclosed except in accordance with the terms of that
 agreement.
 Copyright(c) 2011-2015 Intel Corporation. All Rights Reserved.

 *********************************************************************************/

package com.intel.sample.depth.spsamplecommon;

import java.util.List;
import java.util.concurrent.atomic.AtomicBoolean;
import android.content.Context;
import android.app.Activity;
import android.os.Bundle;
import android.os.SystemClock;
import android.util.Log;
import android.util.Size;
import android.view.Surface;

import com.intel.camera.toolkit.depth.Camera;
import com.intel.camera.toolkit.depth.ImageSet;
import com.intel.camera.toolkit.depth.Module;
import com.intel.camera.toolkit.depth.SenseManagerCallback;
import com.intel.camera.toolkit.depth.RSException;
import com.intel.camera.toolkit.depth.SensorSample;
import com.intel.camera.toolkit.depth.SensorType;
import com.intel.camera.toolkit.depth.StreamProfile;
import com.intel.camera.toolkit.depth.StreamProfileSet;
import com.intel.camera.toolkit.depth.StreamSet;
import com.intel.camera.toolkit.depth.StreamType;
import com.intel.camera.toolkit.depth.Camera.Calibration;
import com.intel.camera.toolkit.depth.sceneperception.SPCore;
import com.intel.camera.toolkit.depth.sceneperception.SPTypes;
import com.intel.camera.toolkit.depth.sceneperception.SPTypes.CameraStreamIntrinsics;
import com.intel.camera.toolkit.depth.sceneperception.SPTypes.SPInputStream;
import com.intel.camera.toolkit.depth.sceneperception.SPTypes.Status;
import com.intel.camera.toolkit.depth.sensemanager.SenseManager;
import com.intel.camera.toolkit.depth.IMUCaptureManager;
import com.intel.camera.toolkit.depth.Image;
import com.intel.sample.depth.spsamplecommon.CameraHandler;

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
	public void onStart() {}

	@Override
	public void onPause() {
		frameProcessCompleteCallback(); //release any currently acquired frames
		setEnabledStreaming(false);
		unconfigureCamera();
	}

	@Override
	public void onResume() {
		configureStreaming();
		setEnabledStreaming(true);
	}

	@Override
	public void onDestroy() {}

	/**
	 * enable or disable camera streaming.
	 * @param isEnabled whether to enable camera streaming.
	 */
	private void setEnabledStreaming(boolean isEnabled) {
		if (mSenseManager != null) {
			try {
				if (isEnabled) {
					if (!mIsCamRunning) {
						mSenseManager.start();
						mIsCamRunning = true;
						mDepthProcessor.setProgramStatus("Waiting to start camera streams");
					}
				}
				else if (mIsCamRunning) {
					mSenseManager.stop();
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
		if (mIMUManager != null) {
			mIMUManager.disableSensor(SensorType.GRAVITY);
			mIMUManager.disableSensor(SensorType.GYROSCOPE);
			mIMUManager.disableSensor(SensorType.ACCELEROMETER);
		}
	}

	/**
	 * starts delivery of IMU samples.
	 */
//    private void startImuManager(){    	
//    	try {
//	    	if (mIMUManager == null) {
//	        	mIMUManager = mSenseManager.queryCaptureManager().queryImuCaptureManager();
//	    	}
//	    	if (mIMUManager != null) {
//	    		if ( !mIMUManager.enableSensor(SensorType.GRAVITY)) {
//	    			mDepthProcessor.setProgramStatus("ERROR - fail to acquire gravity sensor");
//	    			Log.e(TAG, "Failed to acquire gravity sensor");
//	    		} else if ( !mIMUManager.enableSensor(SensorType.GYROSCOPE)) {
//	    			mDepthProcessor.setProgramStatus("ERROR - fail to acquire gyroscope");
//	    			Log.e(TAG, "Failed to acquire gravity sensor");
//	    		} else if ( !mIMUManager.enableSensor(SensorType.ACCELEROMETER)) {
//	    			mDepthProcessor.setProgramStatus("ERROR - fail to acquire gyroscope");
//	    			Log.e(TAG, "Failed to acquire gravity sensor");
//	    		}
//	    	}
//    	}
//    	catch (Exception e) {
//    		mIMUManager = null;
//        	Log.e(TAG, "Exception:" + e.getMessage());
//        	mDepthProcessor.setProgramStatus("ERROR - fail to acquire gravity sensor");
//    	}
//    }

	private void startImuManager(){
		try {
			if (mIMUManager == null) {
				mIMUManager = mSenseManager.queryIMUCaptureManager();
			}
			if (mIMUManager != null) {
				if ( !mIMUManager.enableSensor(SensorType.GRAVITY)) {
					mDepthProcessor.setProgramStatus("ERROR - fail to acquire gravity sensor");
					stopImuManager();
					Log.e(TAG, "Failed to acquire gravity sensor");
				} else if ( !mIMUManager.enableSensor(SensorType.GYROSCOPE)) {
					mDepthProcessor.setProgramStatus("ERROR - fail to acquire gyroscope");
					stopImuManager();
					Log.e(TAG, "Failed to acquire gravity sensor");
				} else if ( !mIMUManager.enableSensor(SensorType.ACCELEROMETER)) {
					mDepthProcessor.setProgramStatus("ERROR - fail to acquire gyroscope");
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
				mSenseManager.enableModule(SenseManager.SCENE_PERCEPTION);
				mSPCore = (SPCore)mSenseManager.queryModule(SenseManager.SCENE_PERCEPTION);

				Log.d(TAG, "Scene Perception enabled");
				mStreamProfileSet = ((Module)mSPCore).getPreferredProfiles().get(0); //choose QVGA depth and rgb
				mDepthInputSize = new Size(mStreamProfileSet.get(StreamType.DEPTH).Width,
						mStreamProfileSet.get(StreamType.DEPTH).Height);
				mColorInputSize = new Size(mStreamProfileSet.get(StreamType.COLOR).Width,
						mStreamProfileSet.get(StreamType.COLOR).Height);
				mSyncedFramesListener = mDepthProcessor.getSyncedFramesListener(this, mDepthInputSize);

				configureCamera();
			} catch (Exception e) {
				Log.e(TAG, "Exception:" + e.getMessage());
				e.printStackTrace();
				mDepthProcessor.setProgramStatus("ERROR - fail to acquire scene perception module");
			}
		} else {
			mDepthProcessor.setProgramStatus("ERROR - camera is not present or fail to acquire camera frames");
		}
	}


    private boolean mIsCameraConfigured = false;
    private void configureCamera() throws RSException {
        if (mIsCameraConfigured) return;
		// Choose camera mode
		try {
			if (mPlaybackFile == null){
				mSenseManager.enableCameraDesc(new Camera.Desc(Camera.Type.HARDWARE, Camera.Facing.ANY, mStreamProfileSet.getStreamTypeSet()));
			} else {
				Log.i(TAG, "enableStreaming uses " + mPlaybackFile);
				mSenseManager.enableCameraDesc(new Camera.Desc(Camera.Type.PLAYBACK, Camera.Facing.ANY, mStreamProfileSet.getStreamTypeSet()));
				mSenseManager.enabledPlaybackFile(mPlaybackFile);
				mSenseManager.enablePlaybackOptions(/*loopback*/ false, /*realtime*/ false);
			}
		} catch (Exception e) {
			Log.e(TAG, "configureCamera() failed, file path: " + mPlaybackFile);
			Log.e(TAG, e.getMessage());
        }


        try {
			mSenseManager.enableStream(mStreamProfileSet);
			mSenseManager.init(mStreamHandler, null);
            mIsCameraConfigured = true;
		} catch (Exception e) {
			Log.e(TAG, "mSenseManager.enableStream() failed");
			Log.e(TAG, e.getMessage());
		}
    }

	private void unconfigureCamera(){
		mSenseManager.close();
        mIsCameraConfigured = false;
	}


	/**
	 * callback function to be used by client of streaming to signal complete 
	 * processing of the frames so that these frames could be released.
	 */
	public void frameProcessCompleteCallback(){
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
		SPCore retVal = null;

		try {
			retVal = (SPCore) mSenseManager.queryModule(SenseManager.SCENE_PERCEPTION);
		} catch (Exception e) {
			Log.e(TAG, "queryScenePerception() failed");
			Log.e(TAG, e.getMessage());
		}

		return retVal;
	}

	private Image mColorImg;
	private Image mDepthImg;
	private ImageSet mCurImgSet;
	private AtomicBoolean mIsDepthProcessorConfigured = new AtomicBoolean(false);
	SenseManagerCallback mStreamHandler = new SenseManagerCallback()
	{
		private int frameCount = 0;
		private SPInputStream curInput = new SPInputStream();
		@Override
		public void onNewSample(ImageSet images, Camera.CaptureInfo info) {
			if (mIsDepthProcessorConfigured.get()) {
				frameCount++;

				Image color, depth;
				color = images.acquireImage(StreamType.COLOR);
				depth = images.acquireImage(StreamType.DEPTH);

				long timeOffset = SystemClock.elapsedRealtimeNanos() - SystemClock.uptimeMillis() * 1000000; //in nanoseconds
				if (mPlaybackFile != null) timeOffset = 0;


				if (color != null && depth != null) {
					mColorImg = color;
					mDepthImg = depth;

					if (mCurImgSet != null) mCurImgSet.releaseAllImages();
					mCurImgSet = images;
					mCurImgSet.retainAllImages();

					long frameTime =  mColorImg.getTimeStamp() + timeOffset;
					if (mIMUManager == null) {
						curInput.setValues(mDepthImg.getImageBuffer(), frameTime,
								mColorImg.getImageBuffer(), frameTime);
					} else {

						SensorSample[] gravity = null, acceler = null, gyroscp = null;
						try {
							gravity = mIMUManager.querySensorSamples(SensorType.GRAVITY);
						} catch (IllegalAccessException e) {
							Log.e(TAG, "Error accessing gravity sensor", e);
						}

						try {
							acceler = mIMUManager.querySensorSamples(SensorType.ACCELEROMETER);
						} catch (IllegalAccessException e) {
							Log.e(TAG, "Error accessing accelerometer sensor", e);
						}

						try {
							gyroscp = mIMUManager.querySensorSamples(SensorType.GYROSCOPE);
						} catch (IllegalAccessException e) {
							Log.e(TAG, "Error accessing gyroscope sensor", e);
						}

						curInput.setValues(mDepthImg.getImageBuffer(), frameTime,
								mColorImg.getImageBuffer(), frameTime, gravity, acceler, gyroscp);

						mSyncedFramesListener.onSyncedFramesAvailable(curInput);
					}
				}else {
					if (mCurImgSet != null) mCurImgSet.releaseAllImages();
					mCurImgSet = null;
					mColorImg = null;
					mDepthImg = null;
					Log.e(TAG, "ERROR - fail to acquire camera frame at " + frameCount);
				}
			}
		}

		@Override
		public void onNewSnapShot(ImageSet images, Camera.CaptureInfo info) {

		}

		@Override
		public boolean onRequestMatchFound(StreamProfileSet streams, StreamProfileSet snapshot, StreamSet<Surface> preview) {
			return false;
		}

		@Override
		public void onSetProfile(Camera.CaptureInfo info) {
			StreamProfile cs = info.getStreamCaptureProfiles().get(StreamType.COLOR);
			StreamProfile ds = info.getStreamCaptureProfiles().get(StreamType.DEPTH);
			if(null == cs || null == ds) {
				mDepthProcessor.setProgramStatus("ERROR - camera cannot be configured");
				return;
			}

			startImuManager();

			// if offline playback mode, check if the profile is supported by SP
			if(mPlaybackFile != null) {
				StreamProfileSet curStream = new StreamProfileSet();
				curStream.set(StreamType.COLOR, cs);
				curStream.set(StreamType.DEPTH, ds);
				boolean foundMatchingProfile = false;
				List<StreamProfileSet> profileSets = ((Module) mSPCore).getPreferredProfiles();
				for(int i = 0; i < profileSets.size(); i++) {
					StreamProfileSet profileSet = profileSets.get(i);
					StreamProfile colorProfile = profileSet.get(StreamType.COLOR);
					StreamProfile depthProfile = profileSet.get(StreamType.DEPTH);

					if(cs.ImgSize.equals(colorProfile.ImgSize) && cs.Format.equals(colorProfile.Format)
							&& ds.ImgSize.equals(depthProfile.ImgSize) && ds.Format.equals(depthProfile.Format)) {
						foundMatchingProfile = true;
						break;
					}

				}
				if(!foundMatchingProfile) {
					Log.e(TAG, "ERROR - camera stream profile not supported");
					mDepthProcessor.setProgramStatus("ERROR - camera stream profile not supported");
					return;
				}
				// if the profile is supported, use this new input size for setting SP configuration
				mDepthInputSize = ds.ImgSize;
				mColorInputSize = cs.ImgSize;
			}

			//configure scene perception
			if (!mIsDepthProcessorConfigured.get()) {
				Calibration.Intrinsics colorParams = null; //intrinsics param of color camera
				Calibration.Intrinsics depthParams = null; //intrinsics param of depth camera
				float[] depthToColorExtrinsicTranslation = null;
				//obtain camera intrinsics data
				Calibration calib = info.getStreamCalibrationData();
				if (calib != null){
					colorParams = calib.colorIntrinsics;
					depthParams = calib.depthIntrinsics;
					depthToColorExtrinsicTranslation = new float[]{
							calib.depthToColorExtrinsics.translation.x,
							calib.depthToColorExtrinsics.translation.y,
							calib.depthToColorExtrinsics.translation.z};
				}
				else {
					Log.e(TAG, "ERROR - camera calibration data is not accessible");
					mDepthProcessor.setProgramStatus("ERROR - camera calibration data is not accessible");
					return;
				}

                mSPCore.setInertialSupportEnabled(true);

				Status spStatus = mSPCore.setConfiguration( new CameraStreamIntrinsics(depthParams, mDepthInputSize),
						new CameraStreamIntrinsics(colorParams, mColorInputSize), depthToColorExtrinsicTranslation, 
						SPTypes.SP_LOW_RESOLUTION, null /* use default calibration file */);
				if (spStatus != Status.SP_STATUS_SUCCESS) {
					mDepthProcessor.setProgramStatus("ERROR - Scene Perception cannot be (re-)configured - " + spStatus);
				} else {
                    Log.d(TAG, "OnSetProfile mIsDepthProcessorConfigured");

                    mIsDepthProcessorConfigured.set(true);
					mDepthProcessor.setProgramStatus("Scene Perception configured");
					mDepthProcessor.configureDepthProcess(SPTypes.SP_LOW_RESOLUTION, mDepthInputSize, mColorInputSize,
							colorParams, depthParams, depthToColorExtrinsicTranslation);
					mDepthProcessor.setProgramStatus("Waiting for camera frames");
				}
			}
		}

		@Override
		public void onError(RSException error) {
			if (error.errorCode == RSException.PLAYBACK_EOF) {
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
