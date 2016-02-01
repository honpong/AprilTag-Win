/*******************************************************************************

 INTEL CORPORATION PROPRIETARY INFORMATION
 This software is supplied under the terms of a license agreement or nondisclosure
 agreement with Intel Corporation and may not be copied or disclosed except in
 accordance with the terms of that agreement
 Copyright(c) 2015 Intel Corporation. All Rights Reserved.

 *******************************************************************************/

package com.intel.sample.depth.blockstacking;

import android.app.Activity;
import android.app.Fragment;
import android.graphics.Color;
import android.opengl.Matrix;
import android.os.Bundle;
import android.util.Log;
import android.util.Size;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.webkit.WebChromeClient;
import android.webkit.WebSettings;
import android.webkit.WebView;

import com.intel.camera.toolkit.depth.Camera.Calibration;
import com.intel.camera.toolkit.depth.sceneperception.SPCore;
import com.intel.camera.toolkit.depth.sceneperception.SPCore.CameraTrackListener;
import com.intel.camera.toolkit.depth.sceneperception.SPTypes.CameraPose;
import com.intel.camera.toolkit.depth.sceneperception.SPTypes.CameraStreamIntrinsics;
import com.intel.camera.toolkit.depth.sceneperception.SPTypes.SPInputStream;
import com.intel.camera.toolkit.depth.sceneperception.SPTypes.Status;
import com.intel.camera.toolkit.depth.sceneperception.SPTypes.TrackingAccuracy;
import com.intel.sample.depth.spsamplecommon.CameraHandler;
import com.intel.sample.depth.spsamplecommon.CameraSyncedFramesListener;
import com.intel.sample.depth.spsamplecommon.DepthProcessModule;
import com.intel.sample.depth.spsamplecommon.SPUtils;

import java.util.concurrent.atomic.AtomicBoolean;

public class SPBasicFragment extends Fragment implements DepthProcessModule
{
	private CameraHandler mDepthCameraHandler;
	private static final String TAG = "BlockStacking";

	/**
	 * set handler of depth camera cycle
	 */
	public void setCameraModule(CameraHandler camModule) {
		mDepthCameraHandler = camModule;
	}
	public SPBasicFragment() { }

	//////////////////////////////////////////////////////////////////////////
	//                S c e n e  P e r c e p t i o n ( SP)  		        //
	//////////////////////////////////////////////////////////////////////////
	private SPCore mSPCore = null; 	// instance of SP
	private CameraPose mCameraPose = new CameraPose(); // current camera pose
	// first camera pose to (re)start from
	private CameraPose mInitialCameraPose;
	//active state indicator SP process
	private AtomicBoolean mIsScenePerceptionActive = new AtomicBoolean(false);
	//indicator of having a request to reset camera tracking
	private volatile boolean mSPResetTracking = false;
	// threshold for percentage of coverage of valid depth values per depth
	// input frame used for determining acceptable initial input frame.
	private final float ACCEPTABLE_INPUT_COVERAGE_PERC = 0.3f;
	// status of running state of SP
	private Status mSPStatus = Status.SP_STATUS_NOT_CONFIGURED;
	private CameraStreamIntrinsics internalIntrinsics = new CameraStreamIntrinsics();
	private float[] perspMatrix;
	private String perspMatrixJStr;


	//               	 		S t a r t    U p					          //
	////////////////////////////////////////////////////////////////////////////
	// starting sequence of SP module
	private void startScenePerception(CameraPose initialPose){
		if(null != mSPCore) {
			//register for tracking event
			mSPCore.addCameraTrackListener(new DepthCameraTrackListener());
			mSPCore.getInternalCameraIntrinsics(internalIntrinsics);
			perspMatrix = calcPerspMatrix(internalIntrinsics);
			perspMatrixJStr = String.format("{ m00: %f, m01: %f, m02: %f, m03: %f, m10: %f, m11: %f, m12: %f, m13: %f, m20: %f, m21: %f, m22: %f, m23: %f, m30: %f, m31: %f, m32: %f, m33: %f }",
					perspMatrix[0], perspMatrix[1], perspMatrix[2], perspMatrix[3], perspMatrix[4], perspMatrix[5], perspMatrix[6], perspMatrix[7], perspMatrix[8], perspMatrix[9], perspMatrix[10], perspMatrix[11], perspMatrix[12], perspMatrix[13], perspMatrix[14], perspMatrix[15]
			);
			mSPStatus = Status.SP_STATUS_SUCCESS; //initial running state
		}
		else {
			mSPStatus = Status.SP_STATUS_ERROR;
		}
	}

	private float[] calcPerspMatrix(CameraStreamIntrinsics intrinsics)
	{
		float far = 100f;
		float near = .01f;

		float[] persp = new float[16];
		persp[0] = intrinsics.getFocalLengthHorizontal() * 2f / intrinsics.getImageWidth() / 1f;
		persp[5] = -intrinsics.getFocalLengthVertical() * 2f / intrinsics.getImageHeight() / 1f;
		persp[8] = (intrinsics.getPrincipalPointCoordU() + .5f - intrinsics.getImageWidth() / 2f) * 2f / intrinsics.getImageWidth() / 1f;
		persp[9] = -(intrinsics.getPrincipalPointCoordV() + .5f - intrinsics.getImageHeight() / 2f) * 2f / intrinsics.getImageHeight() / 1f;
		persp[10] = (far+near) / (far-near);
		persp[14] = 1.0f;
		persp[11] = -2f * far * near /  (far-near);

		return persp;
	}

	//               	 		S e t    I n p u t s				          //
	////////////////////////////////////////////////////////////////////////////
	/**
	 * SDKSyncedFramesHandler handles input (depth and rgba) frames coming from
	 * camera module.
	 * The handler checks for input quality to determine good intial scene for
	 * SP module then passes inputs into SP and to display components.
	 */
	private class SDKSyncedFramesHandler implements CameraSyncedFramesListener {
        private CameraHandler mCamHandler; //handler of camera module
		private boolean mIsFirstFrame = true; //for detecting first frame to set gravity input

		public SDKSyncedFramesHandler(CameraHandler camHandler, Size depthImageSize) {
            mCamHandler = camHandler;
        }

		/**
		 * initially checks input quality to start SP
		 * module. On subsequent inputs, the function sets SP module's input
		 * and input to display components.
		 * @param input contains depth frame and color frame data, optionally includes
		 * IMU samples and their associated time stamps.
		 */
		@Override
		public void onSyncedFramesAvailable(SPInputStream input) {

			float sceneQuality = mSPCore.getSceneQuality(input, false);
            if (!mIsScenePerceptionActive.get())
            {
                mIsScenePerceptionActive.set(sceneQuality >= ACCEPTABLE_INPUT_COVERAGE_PERC);
                sendSceneQualityToWebView(sceneQuality);
            }

			if (mSPStatus != Status.SP_STATUS_SUCCESS) {
				if (mIsScenePerceptionActive.get()) {
					setProgramStatus("ERROR - " + mSPStatus); // TODO: show in web view?
				}
                mCamHandler.frameProcessCompleteCallback();
				return;
			}

			// if user indicates active tracking and reconstruction (pressing Play button)
			if (mIsScenePerceptionActive.get()) {
				//if user restarts from a previous tracking.
				if (mSPResetTracking) {
					CameraPose resetCamPose = SPUtils.getGravityAlignedPoseIfApplicable(mInitialCameraPose, input);
					mSPCore.requestTrackingReset(resetCamPose, input);
					mSPResetTracking = false;
				}
				else { //synchronous setting inputs to SP
					if (mIsFirstFrame) {
						mSPCore.setInitialCameraPose(SPUtils.getGravityAlignedPoseIfApplicable(mInitialCameraPose, input));
						mIsFirstFrame = false;
					}

					mSPCore.setInputs(input);
				}
			}
            //signal complete processing of current input set
            mCamHandler.frameProcessCompleteCallback();
		}
	}

//	private native void depthToRGB(ByteBuffer src, ByteBuffer dest, int width, int height, int stride);

	//               	 	C a m e r a    T r a c k i n g 			          //
	////////////////////////////////////////////////////////////////////////////
	/**
	 * DepthCameraTrackListener monitors camera tracking event and calls to display
	 * frame per second counter of tracked frames as well as to display a render
	 * volume image projected from the successfully tracked pose.
	 */
	private class DepthCameraTrackListener implements CameraTrackListener {
		private SPUtils.FPSCalculator mFPSCal = new SPUtils.FPSCalculator();
		private int DISPLAY_FPS_FREQ = 30;
		private int mTrackedFrameCounter = 0;
		@Override
		public void onTrackingUpdate(TrackingAccuracy trackingResult, CameraPose newCamPose) {
			mTrackedFrameCounter++;
			if (trackingResult != TrackingAccuracy.FAILED) {
				mCameraPose.set(newCamPose);
				mFPSCal.updateTimeOnFrame(System.currentTimeMillis());

				if (mTrackedFrameCounter % DISPLAY_FPS_FREQ == 0)
				{
					sendTrackingStatusToWebView(trackingResult, mFPSCal.getFPS());
				}

				sendPoseToWebView(mCameraPose);
			}
			else
			{
				sendTrackingStatusToWebView(trackingResult, 0);
				setProgramStatus("Tracking: " + trackingResult);
			}
		}

		@Override
		public void onTrackingError(Status trackingStatus) {
			mFPSCal.reset();
			mTrackedFrameCounter = 0;
			setProgramStatus("Tracking: " + trackingStatus);
		}

		@Override
		public void onResetTracking(Status trackingStatus) {
			mTrackedFrameCounter = 0;
			mFPSCal.reset();
			setProgramStatus("Reset Tracking: " + trackingStatus);
		}

	}

	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {

		if (container == null) {
			return null;
		}

		View view = inflater.inflate(R.layout.basic_api_fragment, container, false);
		WebView webView = (WebView)view.findViewById(R.id.web_view);

		WebSettings webSettings = webView.getSettings();
		webSettings.setJavaScriptEnabled(true);
		webSettings.setAllowFileAccessFromFileURLs(true); // necessary to load block texture from file

		webView.setWebChromeClient(new WebChromeClient());

		webView.loadUrl("file:///android_asset/main.html");
		webView.setBackgroundColor(Color.TRANSPARENT); // must be set after loadUrl(). see http://stackoverflow.com/a/12039477/484943

		return view;
	}

	@Override
	public void onStart(){
		super.onStart();
		mDepthCameraHandler.onStart();
	}

	@Override
	public void onPause()
	{
		super.onPause();
		mDepthCameraHandler.onPause();
		if (mSPCore != null) {
			mSPCore.pause();
		}
	}

	@Override
	public void onResume(){
		super.onResume();
		mDepthCameraHandler.onResume();
		if (mSPCore != null) {
			mSPCore.resume();
		}
	}

	@Override
	public void onDestroy() {
		try {
			if (mSPCore != null) {
				mSPCore.release();
				mSPCore = null;
				mIsScenePerceptionActive.set(false);
			}
			mDepthCameraHandler.onDestroy();
		}
		catch (Exception e) {
			Log.e(getString(R.string.app_name), "onDestroy():" + e.getMessage());
		}
		super.onDestroy();
	}

	////////////////////////////////////////////////////////////////////////////
	// 				 D e p t h    P r o c e s s   M o d u l e 			     //
	////////////////////////////////////////////////////////////////////////////
	private Calibration.Intrinsics mDepthIntrinsics = new Calibration.Intrinsics();
	@Override
	public void configureDepthProcess(int spResolution, Size depthInputSize, Size colorInputSize,
									  Calibration.Intrinsics colorParams, Calibration.Intrinsics depthParams,
									  float[] depthToColorTranslation) {
		mDepthIntrinsics = depthParams.copy();
		mSPCore = mDepthCameraHandler.queryScenePerception();
		mSPCore.setInertialSupportEnabled(true);
		// set camera initial pose
		mInitialCameraPose = new CameraPose(CameraPose.IDENTITY_POSE);
		startScenePerception(mInitialCameraPose);
	}

	@Override
	public CameraSyncedFramesListener getSyncedFramesListener(CameraHandler camHandler, Size inputSize){
		return new SDKSyncedFramesHandler(camHandler, inputSize);
	}

	@Override
	public void setProgramStatus(CharSequence newStatus)
	{
		Log.d(TAG, newStatus.toString());
	}

	public void sendSceneQualityToWebView(final float quality){
		final Activity curActivity = getActivity();
		if (curActivity != null) {
			curActivity.runOnUiThread(new Runnable() {
				@Override
				public void run() {
					View rootView = curActivity.getWindow().getCurrentFocus();
					if (rootView != null)
					{
						WebView webView = (WebView) curActivity.getWindow().getCurrentFocus().findViewById(R.id.web_view);
						if (webView != null) webView.evaluateJavascript("Tracker.sceneQualityUpdate(" + String.format("%.2f", quality) + ");", null);
					}
				}
			});
		}
	}

	public void sendTrackingStatusToWebView(final TrackingAccuracy trackingAccuracy, final int fps){
		final Activity curActivity = getActivity();
        final StringBuilder sb = new StringBuilder(5);
        sb.append("Tracker.statusUpdate('").append(trackingAccuracy).append("',").append(fps).append(");");

		if (curActivity != null) {
			curActivity.runOnUiThread(new Runnable() {
				@Override
				public void run() {
					View rootView = curActivity.getWindow().getCurrentFocus();
					if (rootView != null)
					{
						WebView webView = (WebView) curActivity.getWindow().getCurrentFocus().findViewById(R.id.web_view);
						if (webView != null)
						{
							webView.evaluateJavascript(sb.toString(), null);
						}
					}
				}
			});
		}
	}

	public void sendPoseToWebView(final CameraPose cameraPose){
		final Activity curActivity = getActivity();
		if (curActivity != null)
		{
			final float[] pose = new float[16];
			Matrix.rotateM(pose, 0, cameraPose.get(), 0, 90f, 1f, 0f, 0f); // workaround for projection matrix difficulties

            final String cameraPoseString = String.format("{ m00: %f, m01: %f, m02: %f, m03: %f, m10: %f, m11: %f, m12: %f, m13: %f, m20: %f, m21: %f, m22: %f, m23: %f, m30: %f, m31: %f, m32: %f, m33: %f }",
                    pose[0], pose[1], pose[2], pose[3], pose[4], pose[5], pose[6], pose[7], pose[8], pose[9], pose[10], pose[11], pose[12], pose[13], pose[14], pose[15]
            );

            final StringBuilder sb = new StringBuilder(5);
            sb.append("Tracker.poseUpdate(").append(perspMatrixJStr).append(",").append(cameraPoseString).append(");");

			curActivity.runOnUiThread(new Runnable() {
				@Override
				public void run()
                {
					View rootView = curActivity.getWindow().getCurrentFocus();
					if (rootView != null)
					{
						WebView webView = (WebView) curActivity.getWindow().getCurrentFocus().findViewById(R.id.web_view);
						if (webView != null)
						{
							webView.evaluateJavascript(sb.toString(), null);
						}
					}
				}
			});
		}
	}
}
