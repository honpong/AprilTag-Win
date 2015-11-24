/*******************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2015 Intel Corporation. All Rights Reserved.

*******************************************************************************/

package com.intel.sample.depth.spsample;

import java.nio.ByteBuffer;
import java.util.concurrent.atomic.AtomicBoolean;

import android.app.Activity;
import android.app.Fragment;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.util.Size;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.webkit.WebChromeClient;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.widget.Toast;

import com.intel.camera.toolkit.depth.Camera.Calibration;
import com.intel.camera.toolkit.depth.sceneperception.SPCore;
import com.intel.camera.toolkit.depth.sceneperception.SPCore.CameraTrackListener;
import com.intel.camera.toolkit.depth.sceneperception.SPTypes.CameraPose;
import com.intel.camera.toolkit.depth.sceneperception.SPTypes.SPInputStream;
import com.intel.camera.toolkit.depth.sceneperception.SPTypes.Status;
import com.intel.camera.toolkit.depth.sceneperception.SPTypes.CameraStreamIntrinsics;
import com.intel.camera.toolkit.depth.sceneperception.SPTypes.TrackingAccuracy;
import com.intel.camera.toolkit.depth.Point3DF;
import com.intel.sample.depth.spsample.SPUtils.FPSCalculator;

/**
 * SPBasicFragment handles UI control and display of inputs and results of camera tracking 
 * and reconstruction.
 * It receives input streams (depth, color images and IMU samples) from SDKCameraManager
 * and makes use of Scene Perception to do camera tracking and reconstruction of the scene.
 */
public class SPBasicFragment extends Fragment implements DepthProcessModule {
	private static final String TAG = "ScenePerceptionSample";

	/**
	 * Option to display images of reconstruction rendering. 
	 */
	public static final boolean DISPLAY_RECONSTRUCTION_RENDERING = false;
	/**
	 * Option to display mesh pieces of the reconstruction. 
	 */	
	public static final boolean DISPLAY_LIVE_MESH = true;
	/**
	 * Constructor
	 * @param useLiveMeshing if true, live meshing is displayed. Otherwise, an image 
	 * of reconstruction rendering is displayed.
	 */
	public SPBasicFragment(boolean useLiveMeshing) {
		mIsMeshingTurnedOn = useLiveMeshing;
	}
	// same size for depth frame inputs
	private Size mDepthInputSize;
	
	// same size for color frame inputs
	private Size mColorInputSize;

	
	// UI Handler for running UI related update
	private final Handler mHandler = new Handler();
	
	/**
	 * get UI Handler for running UI related update
	 */
	public Handler getHandler() {
		return mHandler;
	}
	
	// handler for depth camera module
	private CameraHandler mDepthCameraHandler;
	
	/**
	 * set handler of depth camera cycle
	 */
	public void setCameraModule(CameraHandler camModule) {
		mDepthCameraHandler = camModule;
	}
	
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
	private CameraStreamIntrinsics mSPConfiguration; //chosen SP configuration profile
	// status of running state of SP
	private Status mSPStatus = Status.SP_STATUS_NOT_CONFIGURED;
	//toggle on/off for live meshing
	private volatile boolean mIsMeshingTurnedOn = false; 
	private Mesher mMesher; 
	private Point3DF mSurfaceCenterPoint = new Point3DF();
    
	//               	 		S t a r t    U p					          //
	//////////////////////////////////////////////////////////////////////////// 
	// starting sequence of SP module
	private void startScenePerception(CameraPose initialPose){
        if(null != mSPCore) {		
			//register for tracking event
			mSPCore.addCameraTrackListener(new DepthCameraTrackListener());
			mSPStatus = Status.SP_STATUS_SUCCESS; //initial running state	
	    }
	    else {
	        mSPStatus = Status.SP_STATUS_ERROR;
	    }
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
		//converted values to display depth
		private ByteBuffer mDepthViewBuffer = null; 
		private CameraHandler mCamHandler; //handler of camera module
		private boolean mIsFirstFrame = true; //for detecting first frame to set gravity input
		
		public SDKSyncedFramesHandler(CameraHandler camHandler, Size depthImageSize) {
			mCamHandler = camHandler;
			mDepthViewBuffer = ByteBuffer.allocateDirect(depthImageSize.getWidth()*
					depthImageSize.getWidth() * 4); //converting from 16 bits to 32 bits	
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
			if (mSPStatus != Status.SP_STATUS_SUCCESS) {
				if (mIsScenePerceptionActive.get()) {
					setProgramStatus("ERROR - " + mSPStatus);
				}
				mCamHandler.frameProcessCompleteCallback();
				return;
			}		
			
			//checking scene quality for good initialization of SP module
			if (!mIsScenePerceptionActive.get()) {
				float sceneQuality = mSPCore.getSceneQuality(input, false);
                setProgramStatus("Scene quality: " + String.format("%.2f", sceneQuality));
				mIsScenePerceptionActive.set(sceneQuality >= ACCEPTABLE_INPUT_COVERAGE_PERC);
			}

			// if user indicates active tracking and reconstruction (pressing Play button)
			if (mIsScenePerceptionActive.get()) {
//				mTrackingActivationRequested = false;
				//if user restarts from a previous tracking.
				if (mSPResetTracking) {
					if (mMesher != null) { //reset meshing
						mMesher.reset();
					}
					CameraPose resetCamPose = SPUtils.alignPoseWithGravity(mInitialCameraPose, input);
					mSPCore.requestTrackingReset(resetCamPose, input);
					mSPResetTracking = false;
					
					//updating UI display for reset
//					mRecontRenderer.setBaseRenderMatrix(mInitialCameraPose, true); //reset base render matrix
//					mRecontRenderer.resetView();
//					if (null != resetCamPose) {
//						mRecontRenderer.setCameraPose(resetCamPose); //update camera pose
//					}
//					mRenderPose.setFromArray(mRecontRenderer.getRenderMatrix());
//					Matrix.transposeM(mProjectionMatrix, 0, mInitialCameraPose.get(),0);
//					mRequiredUpdateVolumeImage = true; //as reconstruction changes
				}
				else { //synchronous setting inputs to SP
					if (mIsFirstFrame) {
						mSPCore.setInitialCameraPose(SPUtils.alignPoseWithGravity(mInitialCameraPose, input));
					mIsFirstFrame = false;					
				}				
					
					mSPCore.setInputs(input); 
				}
				// update UI - center point in the scene surface for rendering a line to the point
				mSurfaceCenterPoint = SPUtils.getSurfaceCenterPoint(input.getDepthImage(), 
				mDepthInputSize, mDepthIntrinsics);						
			}

			// display of current inputs
//			int inputDisplayType = mSwitchInputView;
//			if (inputDisplayType == DISPLAY_DEPTH_INPUT) {
//				depthToRGB(input.getDepthImage(), mDepthViewBuffer, mDepthInputSize.getWidth(),
//						mDepthInputSize.getHeight(), mDepthInputSize.getWidth() * 2); //Z16 format has 2 bytes
//			}
//			displayFramesAvailable(mDepthViewBuffer, input.getColorImage(), inputDisplayType);
			
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
		private SPUtils.FPSCalculator mFPSCal = new FPSCalculator();
		private int DISPLAY_FPS_FREQ = 30;
		private int mTrackedFrameCounter = 0;
		@Override
		public void onTrackingUpdate(TrackingAccuracy trackingResult, CameraPose newCamPose) {
			mTrackedFrameCounter++;
			if (trackingResult != TrackingAccuracy.FAILED) {
				mCameraPose.set(newCamPose);
				mFPSCal.updateTimeOnFrame(System.currentTimeMillis());

				if (mTrackedFrameCounter % DISPLAY_FPS_FREQ == 0) {
                    String status = "Tracking: " + trackingResult  + " " + mFPSCal.getFPSText();
                    setProgramStatus(status);
				}
				
//				//update view point of render reconstruction if viewpoint is toggled or dynamic
//				updateRenderViewPoint(mCameraPose);
//
//				//Update UI to display new camera pose
//				mRecontRenderer.setCameraPose(mCameraPose);
//
//				// Update UI to render a line to middle point of surface corresponding to most
//				// recent depth input.
//				mRecontRenderer.setCenterSurfacePoint(mSurfaceCenterPoint);
//				mVolumeView.onInputUpdate();
//
//				if (mIsMeshingTurnedOn) { // use live meshing
//					if (mMesher == null) {
//						mMesher = new Mesher(mSPCore);
//						mRecontRenderer.setMeshContent(mMesher);
//					}
//					if (mTrackedFrameCounter % 20 == 0) { //lower update frequency
//						mRecontRenderer.onInputUpdate(); //update mesh
//					}
//				}
//				else { // update render volume image
//					mLastRenderTask = mRenderVolumeEx.submit(mRunRenderVolume);
//				}
				
			}
			else {
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
	
	//               	 	R e n d e r    V o l u m e   			          //
	//////////////////////////////////////////////////////////////////////////// 
//	private ExecutorService mRenderVolumeEx = Executors.newSingleThreadExecutor();
//	private Future<?> mLastRenderTask;
//	private volatile boolean mRequiredUpdateVolumeImage = true;
//
//	/**
//	 * mRunRenderVolume gets a render volume image of the current reconstruction
//	 * result of the scene based on the current successfully tracked camera pose.
//	 * The function calls to display the image based on the currently set zoom
//	 * factor (by zoom in/out buttons).
//	 */
//	private RunRenderVolume mRunRenderVolume = new RunRenderVolume();
//	// camera pose to render the view of reconstruction
//	private CameraPose mRenderPose = new CameraPose();
//
//	private class RunRenderVolume implements Runnable {
//		private int mFrameCount = 0;
//		private volatile boolean mIsRenderVolumePrioritySet = false;
//		@Override
//		public void run() {
//			if (!mIsRenderVolumePrioritySet) {
//				try {
//					mIsRenderVolumePrioritySet = true; //try to set once
//					android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_DISPLAY);
//				}
//				catch (Exception e) {Log.e(TAG, "ERROR - can not set runRenderVolume thread priority");}//do not have permission to set
//			}
//
//			if (mVolumeRenderingRGBAImg == null) { //allocate to the size of output image of SP module
//				mVolumeRenderingRGBAImg = ByteBuffer.allocateDirect(mSPCore.getOutputImagePixelSize().getWidth()
//						*  mSPCore.getOutputImagePixelSize().getHeight() * 4); //each pixel is of RGBA format
//			}
//
//			if (mFrameCount % 6 == 0 && //lower update rate of volume render image
//				mIsReconstructionEnabled) { // do not get volume render image when there is no reconstruction update
//				mRequiredUpdateVolumeImage = true;
//			}
//
//			if (mRequiredUpdateVolumeImage) { //require an update of volume rendering image
//				if (Status.SP_STATUS_SUCCESS == mSPCore.getVolumeRenderImage(mRenderPose, mVolumeRenderingRGBAImg)) {
//					mRecontRenderer.onInputUpdate(mVolumeRenderingRGBAImg); //displays render volume image
//				}
//				mRequiredUpdateVolumeImage = false;
//			}
//			mFrameCount++;
//		}
//	};

	//               	S a v e   M e s h  to  F i l e	   			          //
	//////////////////////////////////////////////////////////////////////////// 
//	private final DateFormat mMeshFileDateFormat = new SimpleDateFormat(
//			"yyyy-MM-dd-HH-mm-ss", Locale.US);
//	/**
//	 * saveSPMesh saves a mesh of the current reconstruction result by SP module
//	 * into a file of pre-defined name format in Download folder.
//	 * The function also displays process status of the saving task.
//	 */
//	private void saveSPMesh(){
//		String state = Environment.getExternalStorageState();
//		String fLocation = "/storage/emulated/legacy/Download";
//
//		if (Environment.MEDIA_MOUNTED.equals(state) || !Environment.MEDIA_MOUNTED_READ_ONLY.equals(state)) {
//			fLocation = Environment.getExternalStorageDirectory().getPath() + "/Download";
//		}
//		File testLocation = new File(fLocation);
//		if (testLocation.exists() && testLocation.canWrite() && mSPCore != null) {
//			Date date = new Date();
//			final String fName = fLocation + "/SP_Mesh_"+ mMeshFileDateFormat.format(date) + ".obj";
//			final AsyncCallStatusResult saveMeshResult = mSPCore.asyncSaveMesh(fName, false, false);
//			setProgramStatus("Trying to save mesh");
//			Thread showSaveMeshResult = new Thread(new Runnable() {
//				@Override
//				public void run(){
//					Status callStatus;
//					try {
//						callStatus = saveMeshResult.awaitResult(); //wait till completion
//					} catch (Exception e) {
//						callStatus = Status.SP_STATUS_ERROR;
//						SPUtils.showOnScreenMessage(getActivity(), "ERROR - failed to save mesh - " + callStatus);
//					}
//					if (callStatus == Status.SP_STATUS_SUCCESS) {
//						SPUtils.showOnScreenMessage(getActivity(), "Mesh is saved in Download folder as " + fName);
//					}
//					else {
//						SPUtils.showOnScreenMessage(getActivity(), "ERROR - failed to save mesh. " + saveMeshResult);
//				}
//				}
//			});
//			showSaveMeshResult.start();
//		}
//		else {
//			setProgramStatus("ERROR - failed to write mesh to location " + fLocation);
//		}
//	}
	
	////////////////////////////////////////////////////////////////////////////    
	//             			O t h e r  U I  E v e n t s						  //
	//////////////////////////////////////////////////////////////////////////// 
//	private ImageView2D mInputView; // left pane to display depth or color inputs
//	private Button mPlayBtn;
//	private volatile boolean mDisplayPlayBtn = true;
//	private ToggleButton mToggleExtensionBtn;
//	private ToggleButton mToggleViewPointBtn;
//	private Button mSaveMeshBtn;
//	private Button mToggleMeshBtn;
//	private Button mZoomInBtn;
//	private Button mZoomOutBtn;
//	private TextView mStatusTView;
//	private ByteBuffer mVolumeRenderingRGBAImg = null;
//	private View3D mVolumeView;
//	private final int DISPLAY_COLOR_INPUT = 0;
//	private final int DISPLAY_DEPTH_INPUT = 1;
//	private volatile int mSwitchInputView = DISPLAY_COLOR_INPUT;
//	private volatile boolean mIsToggleViewPressed = false; //if toggling viewpoint is pressed
//
//	/**
//	 * setEnabledControlUI enables play button or not and displays guidance on when to start.
//	 * @param isEnabled if to enable playing or not
//	 */
//	private void setEnabledPlaying(final boolean isEnabled) {
//		getActivity().runOnUiThread(new Runnable() {
//			@Override
//			public void run() {
//				if (isEnabled) {
//					mStatusTView.setText("Press the play button");
//				}
//				else {
//					mStatusTView.setText("Point to an area within [1m, 2m] with "
//							+ "enough structure and texture to start");
//				}
//				mPlayBtn.setEnabled(isEnabled);
//			}
//		});
//	}
//
//	/**
//	 * displayFramesAvailable displays either the rgba input frame or the
//	 * depth frame.
//	 */
//	private void displayFramesAvailable(ByteBuffer depthViewImage, ByteBuffer colorImage,
//			int inputDisplayType)
//	{
//		final ByteBuffer inputImage = (inputDisplayType == DISPLAY_DEPTH_INPUT)? depthViewImage : colorImage;
//		if(inputDisplayType==DISPLAY_DEPTH_INPUT)
//			mInputView.onImageInputUpdate(inputImage, mDepthInputSize);
//		else
//			mInputView.onImageInputUpdate(inputImage, mColorInputSize);
//	}
//
	private volatile boolean mTrackingActivationRequested = false; //indicate if a request to run tracking is active
//
//	/*
//	 * updateRenderViewPoint updates rendering of reconstruction and camera pose
//	 * if the viewing point is toggled or if it is dynamically changing with camera pose.
//	 * @param curPose the current pose that the rendering needs to be updated to stay
//	 * consistent with.
//	 */
//	private void updateRenderViewPoint(CameraPose curPose) {
//		if (mIsToggleViewPressed) {
//			//update base render matrix at switching to dynamic view
//			if(mIsViewPointStatic){
//				mRecontRenderer.setBaseRenderMatrix(curPose, true);
//			}
//
//			//compute view change matrix
//			float[] curViewChange = new float[16];
//			float[] invChangeMatrix = new float[16];
//			Matrix.transposeM(curViewChange, 0, curPose.get(), 0);
//			Matrix.invertM(invChangeMatrix, 0, curViewChange, 0);
//			Matrix.multiplyMM(curViewChange, 0, invChangeMatrix, 0, mProjectionMatrix, 0);
//			mRecontRenderer.setViewChange(ViewChange.TOGGLE_VIEW_POINT, 1.0f, curViewChange);
//
//			//update render pose from which to get reconstruction view.
//			mRenderPose.setFromArray(mRecontRenderer.getRenderMatrix());
//			mRequiredUpdateVolumeImage = true; //request update of reconstruction from new pose
//
//			mIsViewPointStatic = !mIsViewPointStatic;
//			mIsToggleViewPressed = false;
//			//enable toggle button
//			getActivity().runOnUiThread(new Runnable() {
//				@Override
//				public void run() {
//					mToggleViewPointBtn.setEnabled(true);
//				}
//			});
//		}
//		else {
//			if(!mIsViewPointStatic) {
//				mRecontRenderer.setBaseRenderMatrix(curPose, false);
//				Matrix.transposeM(mProjectionMatrix, 0, mCameraPose.get(), 0);
//				mRenderPose.setFromArray(mRecontRenderer.getRenderMatrix());
//				mRequiredUpdateVolumeImage = true;
//			}
//		}
//	}

	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {

		if (container == null) {
			return null;
		}

		View view = inflater.inflate(R.layout.basic_api_fragment, container, false);
		WebView webView = (WebView)view.findViewById(R.id.web_view);

		WebSettings webSettings = webView.getSettings();
		webSettings.setJavaScriptEnabled(true);

		webView.setWebChromeClient(new WebChromeClient());

		webView.loadUrl("file:///android_asset/main.html");

//		createCustomViews(view);

		return view;
	}
//
//	private final float INITIAL_ZOOM_FACTOR = -1.5f;
//	private ReconstructionRenderer mRecontRenderer;
//
//	/**
//	 * createCustomViews creates display components for input and for render volume
//	 * image output of SP module.
//	 */
//	private void createCustomViews(View parentView) {
//		//instantiate input view for display of inputs
//		ViewStub inputStub = (ViewStub)parentView.findViewById(R.id.input_view_stub);
//		inputStub.setLayoutResource(R.layout.image_view_2d);
//		FrameLayout inputViewLayout= (FrameLayout)inputStub.inflate();
//		View inputImageView = (View) inputViewLayout.findViewById(R.id.view_2d);
//		inputImageView.setContentDescription(getString(R.string.input_view_title));
//		mInputView = (ImageView2D)(inputImageView);
//		((ImageView2D)mInputView).setUiHandler(mHandler);
//
//		//instantiate reconstruction volume view for display of surface reconstruction
//		ViewStub renderVolumeStub = (ViewStub)parentView.findViewById(R.id.volume_view_stub);
//		renderVolumeStub.setLayoutResource(R.layout.view_3d);
//		FrameLayout renderVolumeLayout= (FrameLayout)renderVolumeStub.inflate();
//		View volumeImageView = (View) renderVolumeLayout.findViewById(R.id.view_3d);
//		volumeImageView.setContentDescription(getString(R.string.render_view_title));
//		mVolumeView = (View3D)volumeImageView;
//		mVolumeView.setUIHandler(mHandler);
//
//		// set up renderer component for volume view
//		mRecontRenderer = new ReconstructionRenderer();
//		mRecontRenderer.setEnabledMeshDisplay(mIsMeshingTurnedOn);
//		mRecontRenderer.setInitialZoomFactor(INITIAL_ZOOM_FACTOR);
//		mVolumeView.setRenderer(mRecontRenderer);
//		mVolumeView.setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
//	}
//
//	private volatile boolean mIsReconstructionEnabled = true;
//	private volatile boolean mIsViewPointStatic = true;
//	private float[] mProjectionMatrix = new float[16];
//
//	@SuppressLint("ClickableViewAccessibility")
//	@Override
//	public void onActivityCreated(Bundle savedInstanceState){
//		super.onActivityCreated(savedInstanceState);
//
//		mInputView.setOnClickListener(
//				new View.OnClickListener() {
//					@Override
//					public void onClick(View v) {
//						mSwitchInputView = (mSwitchInputView + 1) % 2;
//					}
//				});
//
//		mToggleExtensionBtn = (ToggleButton) getActivity().findViewById(R.id.button_toggle_extension);
//		mToggleExtensionBtn.setOnClickListener( new View.OnClickListener() {
//			@Override
//			public void onClick(View v) {
//				if (mSPCore != null) {
//					mIsReconstructionEnabled = !mIsReconstructionEnabled;
//					mSPCore.setSceneReconstructionEnabled(mIsReconstructionEnabled);
//				}
//			}
//		});
//
//		mToggleViewPointBtn = (ToggleButton) getActivity().findViewById(R.id.button_toggle_viewpoint);
//		mToggleViewPointBtn.setOnClickListener(new View.OnClickListener() {
//			@Override
//			public void onClick(View v) {
//				if (mSPCore != null) {
//					mIsToggleViewPressed = true;
//					mToggleViewPointBtn.setEnabled(false); //disable until the processing is done
//				}
//			}
//		});
//
//		mPlayBtn = (Button) getActivity().findViewById(R.id.button_toggle_start);
//		mPlayBtn.setOnClickListener(
//			new View.OnClickListener() {
//				@Override
//				public void onClick(View v) {
//					if (mDisplayPlayBtn) {
//						mPlayBtn.setBackgroundResource(R.drawable.stop_button);
//						mDisplayPlayBtn = false;
//						mIsScenePerceptionActive.set(true);
//						mStatusTView.setText("Await scene perception (re)start");
//						setEnabledUIButtons(true);
//						if (mToggleExtensionBtn.isChecked()){
//							mToggleExtensionBtn.performClick();
//						}
//						//(re)start with static viewpoint
//						mToggleViewPointBtn.setChecked(false);
//						mIsViewPointStatic = true;
//					}
//					else { //stop button is pressed
//						mIsScenePerceptionActive.set(false);
//						mDisplayPlayBtn = true;
//						mPlayBtn.setBackgroundResource(R.drawable.play_button);
//						setEnabledUIButtons(false);
//						mTrackingActivationRequested = true; // request scene assessment
//						mSPResetTracking = true; //require reset
//						setProgramStatus("Stop Tracking");
//					}
//				}
//			});
//
//		mTrackingActivationRequested = true;//by default, enable scene quality assessment.
//
//		mSaveMeshBtn = (Button) getActivity().findViewById(R.id.button_saveMesh);
//		mSaveMeshBtn.setOnClickListener( new View.OnClickListener() {
//			@Override
//			public void onClick(View v) {
//					Toast.makeText(getActivity(), "Saving Mesh", Toast.LENGTH_SHORT).show();
//					saveSPMesh();
//				}
//		});
//
//		mZoomInBtn = (Button) getActivity().findViewById(R.id.button_zoom_in);
//		mZoomInBtn.setOnClickListener( new View.OnClickListener() {
//			@Override
//			public void onClick(View v) {
//				mRecontRenderer.setViewChange(ViewChange.ZOOM_IN, 1.0f);
//				mRenderPose.setFromArray(mRecontRenderer.getRenderMatrix());
//				mRequiredUpdateVolumeImage = true;
//			}
//		});
//
//		mZoomOutBtn = (Button) getActivity().findViewById(R.id.button_zoom_out);
//		mZoomOutBtn.setOnClickListener( new View.OnClickListener() {
//			@Override
//			public void onClick(View v) {
//				mRecontRenderer.setViewChange(ViewChange.ZOOM_OUT, 1.0f);
//				mRenderPose.setFromArray(mRecontRenderer.getRenderMatrix());
//				mRequiredUpdateVolumeImage = true;
//			}
//		});
//
//		mVolumeView.setOnTouchListener(new View.OnTouchListener() {
//			private float mPrevPositionX;
//			private float mPrevPositionY;
//			@Override
//			public boolean onTouch(View v, MotionEvent event) {
//			    float newPosX = event.getX();
//			    float newPoxY = event.getY();
//
//				if (event.getAction() == MotionEvent.ACTION_MOVE) {
//		            float dx = 3.0f * (newPosX - mPrevPositionX) / mRecontRenderer.getDisplaySize().getWidth();
//		            float dy = 3.0f * (newPoxY - mPrevPositionY)/ mRecontRenderer.getDisplaySize().getHeight();
//					mRecontRenderer.setViewChange(ViewChange.TILT, dy);
//					mRecontRenderer.setViewChange(ViewChange.PAN, dx);
//					mRenderPose.setFromArray(mRecontRenderer.getRenderMatrix());
//					mRequiredUpdateVolumeImage = true;
//			    }
//			    mPrevPositionX = newPosX;
//			    mPrevPositionY = newPoxY;
//			    return true;
//			}
//		});
//
//		mToggleMeshBtn = (Button) getActivity().findViewById(R.id.button_toggleMesh);
//		mToggleMeshBtn.setOnClickListener( new View.OnClickListener() {
//			@Override
//			public void onClick(View v) {
//				if (mIsMeshingTurnedOn) {
//					if (mMesher != null) { //reset meshing
//						mMesher.reset();
//					}
//				}
//				mIsMeshingTurnedOn = !mIsMeshingTurnedOn;
//				mRecontRenderer.setEnabledMeshDisplay(mIsMeshingTurnedOn);
//			}
//		});
//		mStatusTView = (TextView) getActivity().findViewById(R.id.status_text_view);
//	}
//
//	private void setEnabledUIButtons(boolean state){
//		mZoomInBtn.setEnabled(state);
//		mZoomOutBtn.setEnabled(state);
//		mSaveMeshBtn.setEnabled(state);
//		mToggleExtensionBtn.setEnabled(state);
//		mToggleViewPointBtn.setEnabled(state);
//	}
	
	@Override
	public void onStart(){
		super.onStart();
//		setEnabledUIButtons(mIsScenePerceptionActive.get());
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
//		setEnabledUIButtons(mIsScenePerceptionActive.get());
		mDepthCameraHandler.onResume();
		if (mSPCore != null) {
			mSPCore.resume();
		}
	}

	@Override
	public void onDestroy() {
		try {
			if (mMesher != null) {
				mMesher.release();
				mMesher = null;
			}			
			if (mSPCore != null) {
				mSPCore.release();
				mSPCore = null;
				mIsScenePerceptionActive.set(false);
			}	
			mDepthCameraHandler.onDestroy();
			
//			if (mRenderVolumeEx != null) {
//				mRenderVolumeEx.shutdown();
//				mRenderVolumeEx = null;
//			}
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
		mDepthInputSize = depthInputSize;
		mColorInputSize = colorInputSize;
		mDepthIntrinsics = depthParams.copy();
		mSPCore = mDepthCameraHandler.queryScenePerception();	
		// set camera initial pose
		mInitialCameraPose = new CameraPose(CameraPose.IDENTITY_POSE);
		startScenePerception(mInitialCameraPose);
		
//		// configure UI components
//		mInputView.setInputSize(mColorInputSize);
//
//		// Set the camera parameters and initial pose for the renderer
//		CameraStreamIntrinsics internalIntrinsics = new CameraStreamIntrinsics();
//		mSPCore.getInternalCameraIntrinsics(internalIntrinsics);
//		mRecontRenderer.setCameraParams(internalIntrinsics);
//		mRecontRenderer.setInitialRenderPose(mInitialCameraPose);
//
//
//		// initialize projection matrix for display render volume
//		Matrix.transposeM(mProjectionMatrix, 0, mInitialCameraPose.get(),0);
//		//initialize viewing point matrix for rendering reconstruction
//		mRenderPose.setFromArray(mRecontRenderer.getRenderMatrix());
	}
	
	@Override
	public CameraSyncedFramesListener getSyncedFramesListener(CameraHandler camHandler, Size inputSize){
		return new SDKSyncedFramesHandler(camHandler, inputSize);	     	
	}

	@Override
	public void setProgramStatus(final CharSequence newStatus){
		final Activity curActivity = getActivity();
		if (curActivity != null) {
			curActivity.runOnUiThread(new Runnable() {
				@Override
				public void run() {
//					mStatusTView.setText(newStatus);
                    View rootView = curActivity.getWindow().getCurrentFocus();
                    if (rootView != null)
                    {
                        WebView webView = (WebView) curActivity.getWindow().getCurrentFocus().findViewById(R.id.web_view);
                        if (webView != null) webView.evaluateJavascript("RealSense.trackingDidChangeStatus('" + newStatus + "');", null);
                    }
				}
			});
		}
	}
	
//	static {
//		try{
//			System.loadLibrary("sceneperception_sample");
//		}
//		catch(UnsatisfiedLinkError e){
//			Log.e(TAG, "Cannot load Scene Perception sample library");
//		}
//	}
}
