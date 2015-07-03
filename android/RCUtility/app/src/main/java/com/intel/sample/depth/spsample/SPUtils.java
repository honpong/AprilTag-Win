/********************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION This software is supplied under the
terms of a license agreement or nondisclosure agreement with Intel Corporation
and may not be copied or disclosed except in accordance with the terms of that
agreement.
Copyright(c) 2011-2015 Intel Corporation. All Rights Reserved.

*********************************************************************************/

package com.intel.sample.depth.spsample;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.nio.ShortBuffer;
import java.util.LinkedList;
import java.util.Queue;

import javax.microedition.khronos.opengles.GL10;

import android.app.Activity;
import android.content.Context;
import android.opengl.GLES11;
import android.opengl.Matrix;
import android.util.FloatMath;
import android.util.Size;
import android.view.Gravity;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.intel.camera.toolkit.depth.Camera.Calibration;
import com.intel.camera.toolkit.depth.sceneperception.SPCore;
import com.intel.camera.toolkit.depth.sceneperception.SPTypes.CameraPose;
import com.intel.camera.toolkit.depth.sceneperception.SPTypes.Configuration;
import com.intel.camera.toolkit.depth.Point3DF;
import com.intel.camera.toolkit.depth.StreamProfile;
import com.intel.camera.toolkit.depth.StreamProfileSet;
import com.intel.camera.toolkit.depth.StreamType;

public class SPUtils {
	
	/**
	 * createConfiguration creates a valid configuration parameter to SP module
	 */
	static public Configuration createConfiguration(Size colorImgSize, Calibration.Intrinsics camParam, int resol) {
		Configuration setupConfig;
		if (camParam == null || colorImgSize == null) {
			setupConfig = new Configuration(320, 240, 304.789948f, 304.789948f, 169.810745f, 125.464630f,
					resol);
		}
		else {
			setupConfig = new Configuration(colorImgSize.getWidth(), colorImgSize.getHeight(),
				camParam.focalLength.x,    camParam.focalLength.y, 
				camParam.principalPoint.x, camParam.principalPoint.y, resol);
		}
		return setupConfig;
	}
	
	private static final float INITIAL_VOLUME_DIMEN = 4.0f; //todo need to get from SP
	
	/**
	 * setPoseWithMiddleInitialPosition sets an input camera pose to the position 
	 * that is in the middle of the front face of the volume that initially holds
	 * reconstruction result 
	 */
	public static void setPoseWithMiddleInitialPosition(CameraPose camPose, int resolution){		
		int divFactor = 2;
		
		switch (resolution) {
		case Configuration.SP_LOW_RESOLUTION: 
			divFactor = 2;
			break;
		case Configuration.SP_MED_RESOLUTION:
			divFactor = 4;
			break;
		case Configuration.SP_HIGH_RESOLUTION:
			divFactor = 8;
			break;
		default:
			divFactor = 2;
		}
		camPose.setPosition(INITIAL_VOLUME_DIMEN / divFactor, INITIAL_VOLUME_DIMEN/ divFactor, 0.0f);
	}
	
	/**
	 * FPSCalculator calculates frames per second counter. It requires calling 
	 * function updateTimeOnFrame upon inclusion of new frames.
	 */
	public static class FPSCalculator{
    	private float mAvgFPS = 0.0f;
    	private int mAvgWindowSize = 10;
    	private Queue<Float> mVals = new LinkedList<Float>();
    	private long mPrevTime = 0;
		private long mCurTime = 0;
    	
    	public FPSCalculator(){}
    	
    	/**
    	 * constructor with an averaging window size that corresponds to the 
    	 * number of fps calculations to be averaged.
    	 * @param avgWindowSize
    	 */
    	public FPSCalculator(int avgWindowSize){
    		mAvgWindowSize = avgWindowSize;
    	}
    	
    	/**
    	 * update with the time of the new frame coming.
    	 * @param newTimeinMillis
    	 */
    	public synchronized void updateTimeOnFrame(long newTimeinMillis) {
			mCurTime = newTimeinMillis;
			//skip measuring the first frame
			if (mPrevTime == 0) {
				mPrevTime = mCurTime;
				return;
			}
			if (mPrevTime > 0 && (mCurTime > mPrevTime + 1)) {
				float newVal = 1000.0f /(mCurTime - mPrevTime); 
				mVals.offer(newVal);
				if (mVals.size() == mAvgWindowSize) {//calculate first time
					float total = 0;
					for (float element : mVals) {
						total += element;
					}
					mAvgFPS = total / mAvgWindowSize;						
				}
				else if (mVals.size() > mAvgWindowSize) {
					float newTotal = mAvgFPS * mAvgWindowSize;
					newTotal -= mVals.poll();
					newTotal += newVal;
					mAvgFPS = newTotal/mAvgWindowSize;
				}
				mPrevTime = mCurTime;
			}
    	}
    	
    	/**
    	 * get the current fps
    	 * @return
    	 */
    	public synchronized int getFPS() {
    		return (int)mAvgFPS;
    	}
    	
    	/**
    	 * get text content of fps result
    	 * @return
    	 */
    	public synchronized String getFPSText() {
    		int fps = (int)mAvgFPS; 
    		if (fps > 0) {
    			return "Fps = " + Integer.toString(fps);
    		}
    		else {
    			return "";
    		}
    		
    	}
    	
    	/**
    	 * reset fps counter
    	 */
    	public synchronized void reset(){
			mVals.clear();
			mPrevTime = 0;
			mCurTime = 0;
			mAvgFPS = 0.0f;
    	}
	}
	
	/**
	 * display a toast view message on screen
	 * @param curActivity the activity to be displayed upon
	 * @param msgContent content of message
	 */
    public static void showOnScreenMessage(final Activity curActivity, final String msgContent) {
    	if (curActivity != null && !curActivity.isDestroyed()) {
	    	curActivity.runOnUiThread(new Runnable() {
	    		@Override
	    		public void run() {
			    	Toast scrMsg = Toast.makeText(curActivity, msgContent, Toast.LENGTH_LONG);
			    	scrMsg.setGravity(Gravity.CENTER_VERTICAL, 0, 0);
			    	LinearLayout msgLayout = (LinearLayout) scrMsg.getView();
			    	TextView msgView = (TextView) msgLayout.getChildAt(0);
			    	msgView.setTextSize(30);
			    	scrMsg.show();
	    		}
	    	});
    	}
	}
    
    /**
     * float3 is a vector class of 03 components that supports arithmetic operations
     */
    public static class float3
    {
    	public float x, y, z;

    	public float3(float _x, float _y, float _z){
    		x = _x;
    		y = _y;
    		z = _z;
    	}

		float3 add(float3 rhs)
		{
			return new float3(x + rhs.x, y + rhs.y, z + rhs.z);
		}

		float3 sub(float3 rhs)
		{
			return new float3(x - rhs.x, y - rhs.y, z - rhs.z);
		}

		float3 mul(float a)
		{
			return new float3(x * a, y * a, z * a);
		}

		float3 div(float a)
		{
			a = 1.0f / a;
			return new float3(x * a, y * a, z * a);
		}	

		boolean equals(float3 rhs)
		{
			return (Math.abs(x - rhs.x) < EPS) && (Math.abs(y - rhs.y) < EPS) && (Math.abs(z - rhs.z) < EPS);
		}

		float dot(float3 rhs)
		{
			return x * rhs.x + y * rhs.y + z * rhs.z;
		}

		float3 cross(float3 right)
		{
			return new float3(y * right.z - z * right.y,
				z * right.x - x * right.z,
				x * right.y - y * right.x);
		}

		float length()
		{
			return (float) Math.sqrt(x * x + y * y + z * z);
		}

		float3 normalize()
		{
			float len = length();
			if (len > 1E-16f)
			{
				len = 1.0f / len;
				x *= len;
				y *= len;
				z *= len;
			}
			return new float3(x,y,z);
		}
		private final float EPS = 0.0000001f;
	};
	
	/** 
	 * cross product of two vectors of 03 components.
	 * @param lhs left side argument to the cross product<br>
	 * @param rhs right side argument to the cross product<br>
	 * @return cross product as a vector of 03 components.
	 */
	float[] crossProduct(float[] lhs, float[] rhs)
	{
		float[] result = new float[3];
		result[0] = lhs[1] * rhs[2] - lhs[2] * rhs[1];
		result[1] = lhs[2] * rhs[0] - lhs[0] * rhs[2];
		result[2] = lhs[0] * rhs[1] - lhs[1] * rhs[0];
		return result;
	}
	
	/**
	 * normalize a vector of 03 components so the length becomes 1.0f
	 * @param vectorX
	 * @param vectorY
	 * @param vectorZ
	 * @return
	 */
	static float[] normalizeVector(float vectorX, float vectorY, float vectorZ) {
		double magnitude = Math.sqrt(vectorX * vectorX + vectorY * vectorY +
				vectorZ * vectorZ);
		float[] normalizedVec = new float[3];
		
		normalizedVec[0] = (float)(vectorX / magnitude);
		normalizedVec[1] = (float)(vectorY / magnitude);
		normalizedVec[2] = (float)(vectorZ / magnitude);
		
		return normalizedVec;
	}
	
	/**
	 * create a rotation matrix (3x3) for a given input camera pose (no translation / 
	 * position change) by taking a gravity vector to be one row in the rotation 
	 * matrix.
	 * This is to ensure alignment of pose with the gravity so that the downward  
	 * orientation (given by gravity vector) is also the downward orientation 
	 * in the reconstruction volume.
	 * @param camPose the pose whose rotation matrix is to be updated.
	 * @param gravity the normalized gravity vector given in the same coordinate 
	 * system of Scene Perception.
	 * @param rowPosition the row number (1-3) that will be aligned with gravity.
	 * @return true if the operation successful and false otherwise.
	 */
	static boolean alignPoseWithGravity(CameraPose camPose, float[] gravity, int rowPosition) {
		if (gravity == null || camPose == null) {
			return false;
		}
		//mapping the coordinate systems of IMU and camera
		float[] poseMatrix = camPose.get();
		poseMatrix[(rowPosition - 1) * 4] = gravity[0]; //exchange x - y axes
		poseMatrix[(rowPosition - 1) * 4 + 1] = gravity[1];
		poseMatrix[(rowPosition - 1) * 4 + 2] = gravity[2];
		
		float dotProdXY = poseMatrix[(rowPosition - 1) * 4] * poseMatrix[(rowPosition - 1) * 4] +
				poseMatrix[(rowPosition - 1) * 4 + 1] * poseMatrix[(rowPosition - 1) * 4 + 1];
		float dotProdXYZ = dotProdXY + poseMatrix[(rowPosition - 1) * 4 + 2] * poseMatrix[(rowPosition - 1) * 4 + 2];

		float DIVISION_EPSILON = 0.000000005f;
		float PRECISION = 0.000005f;
		if (Math.abs(dotProdXYZ - 1.0f) > PRECISION)
		{
			return false;
		}

		double meanXY = Math.sqrt((dotProdXY));

		//fill in first row
		if (rowPosition == 1)
		{
			//second row
			if (meanXY > DIVISION_EPSILON)
			{
				poseMatrix[4] = (float)(poseMatrix[1] / meanXY);
				poseMatrix[5] = -(float)(poseMatrix[0] / meanXY);
				poseMatrix[6] = 0.0f;
			}
			else //passed in gravity is: (0,0,+/-1)
			{
				poseMatrix[4] = 0.0f;
				poseMatrix[5] = 1.0f;
				poseMatrix[6] = 0.0f;
			}
			//third row is a cross product
			poseMatrix[8] = poseMatrix[1] * poseMatrix[6] - poseMatrix[2] * poseMatrix[5];
			poseMatrix[9] = poseMatrix[2] * poseMatrix[4] - poseMatrix[0] * poseMatrix[6];
			poseMatrix[10] = poseMatrix[0] * poseMatrix[5] - poseMatrix[1] * poseMatrix[4];
		}

		//fill in the second row
		if (rowPosition == 2)
		{
			//first row
			if (meanXY > DIVISION_EPSILON)
			{
				//first row
				poseMatrix[0] = (float)(poseMatrix[5] / meanXY);
				poseMatrix[1] = -(float)(poseMatrix[4] / meanXY);
				poseMatrix[2] = 0.0f;
			}
			else //passed in row is: (0,0,+/-1)
			{
				//first row
				poseMatrix[0] = 1.0f;
				poseMatrix[1] = 0.0f;
				poseMatrix[2] = 0.0f;
			}
			//third row is a cross product
			poseMatrix[8] = poseMatrix[1] * poseMatrix[6] - poseMatrix[2] * poseMatrix[5];
			poseMatrix[9] = poseMatrix[2] * poseMatrix[4] - poseMatrix[0] * poseMatrix[6];
			poseMatrix[10] = poseMatrix[0] * poseMatrix[5] - poseMatrix[1] * poseMatrix[4];
		}

		//fill in the third row
		if (rowPosition == 3)
		{
			//first row
			if (meanXY > DIVISION_EPSILON)
			{
				poseMatrix[0] = (float)(poseMatrix[9] / meanXY);
				poseMatrix[1] = -(float)(poseMatrix[8] / meanXY);
				poseMatrix[2] = 0.0f;
			}
			else //passed in row is: (0,0,+/-1)
			{
				poseMatrix[0] = 1.0f;
				poseMatrix[1] = 0.0f;
				poseMatrix[2] = 0.0f;
			}
			//second row is a cross product: third row x first row
			poseMatrix[4] = poseMatrix[9] * poseMatrix[2] - poseMatrix[10] * poseMatrix[1];
			poseMatrix[5] = poseMatrix[10] * poseMatrix[0] - poseMatrix[8] * poseMatrix[2];
			poseMatrix[6] = poseMatrix[8] * poseMatrix[1] - poseMatrix[9] * poseMatrix[0];
		}		
		return true;
	}
	
	/**
	 * createScenePerceptionStreamProfileSet creates a default camera streaming
	 * profile that is acceptable for handling by SP module.
	 * @param spInstance a valid instance to SP module
	 * @return a stream profile that can be used to configure camera's streaming.
	 */
	public static StreamProfileSet createScenePerceptionStreamProfileSet(SPCore spInstance){
		StreamProfileSet spProfile = new StreamProfileSet();
		Size inputSize = spInstance.getInputSupportedSizes()[0];//pick QVGA size
		StreamProfile colorProfile = new StreamProfile(inputSize.getWidth(), inputSize.getHeight(), 
				spInstance.getInputSupportedColorFormats()[0], 35 /*fps*/, StreamType.COLOR); //RGBA_8888 format
		spProfile.set(StreamType.COLOR, colorProfile);
		StreamProfile depthProfile = new StreamProfile(inputSize.getWidth(), inputSize.getHeight(), 
				spInstance.getInputSupportedDepthFormats()[0], 35 /*fps*/, StreamType.DEPTH); //Z16 format		
		spProfile.set(StreamType.DEPTH, depthProfile);
		return spProfile;
		
	}
	
	/**
	 * provides x, y, z coordinates of the middle point of the surface. The surface is constrained to 
	 * the current depth input. It uses camera calibration data to project a 3D point from a depth 
	 * value obtained at the middle pixel of the depth input image. 
	 * @param depthBuf contains values of a depth input
	 * @param imgSize size of the depth input image in pixels
	 * @param camParams camera calibration parameters
	 * @return
	 */
	public static Point3DF getSurfaceCenterPoint(ByteBuffer depthBuf, Size imgSize, Calibration.Intrinsics camParams){
		ShortBuffer depthImg = depthBuf.asShortBuffer();
	    int[] centerPixel = new int[]{ imgSize.getWidth() / 2, imgSize.getHeight() / 2 };
	    depthImg.position(0);
	    float centerDepthValue = depthImg.get(centerPixel[0] + centerPixel[1] * imgSize.getWidth()) * 0.001f;
	    Point3DF centerSurfacePoint = new Point3DF();
	    centerSurfacePoint.x = centerDepthValue *(centerPixel[0] - camParams.principalPoint.x) / camParams.principalPoint.x;
	    centerSurfacePoint.y = centerDepthValue *(centerPixel[1] - camParams.principalPoint.y) / camParams.principalPoint.y;
	    centerSurfacePoint.z = centerDepthValue;
	    return centerSurfacePoint;
	}
	
	/**
	 * ViewChange identifies view manipulation operations of view content. 
	 */
	static public enum ViewChange {
		RESET(0), 
		ZOOM_IN(1),
		ZOOM_OUT(2),
		PAN(3),
		PAN_LEFT(4),
		PAN_RIGHT(5),
		TILT(6),
		TILT_UP(7),
		TILT_DOWN(8);
		ViewChange(int val) {
			this.mValue = val;
		}		
    	private final int mValue;
    	public int getValue() {return this.mValue;}		
	}
	
	/**
	 * getUpdatedViewMatrix computes a matrix 4x4 corresponds to the view manipulation operation 
	 * given and applies this view manipulation operation to a given view matrix
	 * @param viewFactor either one of the view manipulation operation defined in ViewChange.
	 * @param distance the amount of movement, measure in increments of 0.1f.
	 * @param curViewMatrix current view matrix being used to project the 3D content. The  
	 * matrix is represented as an array of 16 floats and in column major. 
	 * @return an array of 16 floats that represents the updated viewing matrix in column 
	 * major order. The update is by an application of the given view manipulation operation. 
	 */
	public static float[] getUpdatedViewMatrix(ViewChange viewFactor, float distance, 
			float[] curViewMatrix) {
		float[] changeMatrix = null;
		float [] updatedViewMatrix = new float[16];
		float moveAmount = 0.1f * distance;
		switch(viewFactor) {
		case ZOOM_IN:
			changeMatrix = new float[]{
					1, 0, 0, 0, 
					0, 1, 0, 0, 
					0, 0, 1, 0,
					0, 0, moveAmount, 1};
			Matrix.multiplyMM(updatedViewMatrix, 0, curViewMatrix, 0, changeMatrix, 0);			
			break;
			
		case ZOOM_OUT:
			changeMatrix = new float[]{
					1, 0, 0, 0,
					0, 1, 0, 0, 
					0, 0, 1, 0,
					0, 0, -moveAmount, 1};
			Matrix.multiplyMM(updatedViewMatrix, 0, curViewMatrix, 0, changeMatrix, 0);				
			break;
			
		case PAN:
		case PAN_LEFT:
			changeMatrix = new float[]{
					FloatMath.cos(moveAmount), 0, -FloatMath.sin(moveAmount), 0,
					0, 1, 0, 0, 
					FloatMath.sin(moveAmount), 0, FloatMath.cos(moveAmount), 0,
					0, 0, 0, 1};
			Matrix.multiplyMM(updatedViewMatrix, 0, changeMatrix, 0, curViewMatrix, 0);			
			break;

		case PAN_RIGHT:
			changeMatrix = new float[]{
					FloatMath.cos(-moveAmount), 0, -FloatMath.sin(-moveAmount), 0,
					0, 1, 0, 0, 
					FloatMath.sin(-moveAmount), 0, FloatMath.cos(-moveAmount), 0,
					0, 0, 0, 1};
			Matrix.multiplyMM(updatedViewMatrix, 0, changeMatrix, 0, curViewMatrix, 0);				
			break;
			
		case TILT:	
		case TILT_UP:
			changeMatrix = new float[]{
					1, 0, 0, 0,
					0, FloatMath.cos(-moveAmount), FloatMath.sin(-moveAmount), 0, 
					0, -FloatMath.sin(-moveAmount), FloatMath.cos(-moveAmount), 0,
					0, 0, 0, 1};
			Matrix.multiplyMM(updatedViewMatrix, 0, changeMatrix, 0, curViewMatrix, 0);				
			break;
			
		case TILT_DOWN:
			changeMatrix = new float[]{
					1, 0, 0, 0,
					0, FloatMath.cos(moveAmount), FloatMath.sin(moveAmount), 0, 
					0, -FloatMath.sin(moveAmount), FloatMath.cos(moveAmount), 0,
					0, 0, 0, 1};
			Matrix.multiplyMM(updatedViewMatrix, 0, changeMatrix, 0, curViewMatrix, 0);				
			break;					
		case RESET:		
			changeMatrix = new float[16];
			Matrix.invertM(changeMatrix, 0, curViewMatrix, 0);					
			Matrix.multiplyMM(updatedViewMatrix, 0, changeMatrix, 0, curViewMatrix, 0);						
			break;
		default:
			changeMatrix = CameraPose.IDENTITY_POSE.get();
			break;			
			
		}
		return updatedViewMatrix;
	}
}

/**
 * display a camera pose with a rectangle plane and 03 axes (x, y, z axes) to show 
 * the orientation of the camera pose.
 *
 */
class CameraPoseDisplay extends CameraPose {
	private float[] CAMERA_FRAME_CORNERS = new float[12];
	private FloatBuffer mProjectedVertices = ByteBuffer.allocateDirect(9 * 3 * 4).order(ByteOrder.nativeOrder()).asFloatBuffer();
	private byte[] CAM_POSE_INDICES =  new byte[]{
			0, 1, //draw a line: camera position --> middle surface point 
			0, 2, //draw x-axis from camera position
			0, 3, //draw y-axis from camera position
			0, 4, //draw z-axis from camera position
			5, 6, // draw rectangles
			6, 7,
			7, 8,
			8, 5
	};	
	private ByteBuffer mCamPoseIndices = ByteBuffer.allocateDirect(16).put(CAM_POSE_INDICES);
	private float[] mSurfaceCenterPoint = new float[]{0.0f, 0.0f, 0.0f, 1.0f};
	
	public CameraPoseDisplay(CameraPose camPose) {
		Matrix.transposeM(mValue, 0, camPose.get(),0);
		// determine corners of the rectangle display of camera pose
		for (int i = 0; i < 4; i++)
		{
			float ptx = (((i & 1) ^ (i >> 1)) != 0) ? -0.08f : 0.08f;
			float pty = ((i & 2) != 0) ? -0.06f : 0.06f;
			CAMERA_FRAME_CORNERS[i*3] = ptx;
			CAMERA_FRAME_CORNERS[i*3 + 1] = pty;
			CAMERA_FRAME_CORNERS[i*3 + 2] = 0.0f;
		}
	}
	/**
	 * set the surface center point so as to render a line from camera position 
	 * to the surface point.
	 * @param centerPoint
	 */
	public void setCenterSurfacePoint(Point3DF centerPoint) {
		Matrix.multiplyMV(mSurfaceCenterPoint, 0, mValue, 0, 
				new float[]{centerPoint.x, centerPoint.y,
						centerPoint.z, 1.0f}, 0);		
		mProjectedVertices.put(3, mSurfaceCenterPoint[0]);
		mProjectedVertices.put(4, mSurfaceCenterPoint[1]);
		mProjectedVertices.put(5, mSurfaceCenterPoint[2]);
	}
	
	// end-points of axes from camera position as origin
	private float[] mXAxisEndPoint = new float[4]; 
	private float[] mYAxisEndPoint = new float[4]; 
	private float[] mZAxisEndPoint = new float[4]; 
	
	@Override
	/**
	 * update values of camera pose
	 */
	public void set(CameraPose newPose){
		Matrix.transposeM(mValue, 0, newPose.get(),0);
		mProjectedVertices.put(0, mValue[12]);
		mProjectedVertices.put(1, mValue[13]);
		mProjectedVertices.put(2, mValue[14]);
					
		Matrix.multiplyMV(mXAxisEndPoint, 0, mValue, 0, 
				new float[]{0.15f, 0.0f, 0.0f, 1.0f}, 0);	
		mProjectedVertices.put(6, mXAxisEndPoint[0]);
		mProjectedVertices.put(7, mXAxisEndPoint[1]);
		mProjectedVertices.put(8, mXAxisEndPoint[2]);
					
		Matrix.multiplyMV(mYAxisEndPoint, 0, mValue, 0, 
				new float[]{0.0f, 0.15f, 0.0f, 1.0f}, 0);
		mProjectedVertices.put(9, mYAxisEndPoint[0]);
		mProjectedVertices.put(10, mYAxisEndPoint[1]);
		mProjectedVertices.put(11, mYAxisEndPoint[2]);	
					 
		Matrix.multiplyMV(mZAxisEndPoint, 0, mValue, 0, 
				new float[]{0.0f, 0.0f, 0.15f, 1.0f}, 0);
		// display a segment on the same line to surface middle point instead of
		// a point on z-axis to avoid visual discrepancy				
		if (Math.abs(mSurfaceCenterPoint[2] - mValue[14]) > 0.05f) {
			float mulFactor = (mZAxisEndPoint[2] - mValue[14]) / 
					(mSurfaceCenterPoint[2] - mValue[14]);
			mZAxisEndPoint[0] = mSurfaceCenterPoint[0] * mulFactor +  mValue[12] * (1 - mulFactor);
			mZAxisEndPoint[1] = mSurfaceCenterPoint[1] * mulFactor +  mValue[13] * (1 - mulFactor);
			mZAxisEndPoint[2] = mSurfaceCenterPoint[2] * mulFactor +  mValue[14] * (1 - mulFactor);					
		}
		mProjectedVertices.put(12, mZAxisEndPoint[0]);
		mProjectedVertices.put(13, mZAxisEndPoint[1]);
		mProjectedVertices.put(14, mZAxisEndPoint[2]);			
		
		// vertices for camera frame corners
		float[] corner = new float[4];
		for (int i = 0; i < 4; i++)
		{
			Matrix.multiplyMV(corner, 0, mValue, 0, 
					new float[]{CAMERA_FRAME_CORNERS[i * 3], 
				CAMERA_FRAME_CORNERS[i * 3 + 1],
				CAMERA_FRAME_CORNERS[i * 3 + 2], 1.0f}, 0);
			mProjectedVertices.put(15 + i*3, corner[0]);
			mProjectedVertices.put(15 + i*3 + 1, corner[1]);
			mProjectedVertices.put(15 + i*3 + 2, corner[2]);
		}			
	}
	
	// drawCameraPose draws a rectangle and/or 3-axis to represent the camera at current pose.	
	public void draw() {
		mProjectedVertices.position(0);
		mCamPoseIndices.position(0);
		
		GLES11.glEnable(GL10.GL_LINE_SMOOTH);			
		GLES11.glVertexPointer(3, GL10.GL_FLOAT, 0, mProjectedVertices);
		GLES11.glColor4f(0.0f, 0.0f, 1.0f, 1.0f);
		GLES11.glPointSize(8.0f);				
		GLES11.glDrawElements(GL10.GL_POINTS, 1, GL10.GL_UNSIGNED_BYTE, mCamPoseIndices);
		
		GLES11.glLineWidth(4.0f);
		//draw line to surface center
		GLES11.glColor4f(0.0f, 0.0f, 1.0f, 1.0f);
		GLES11.glDrawElements(GL10.GL_LINES, 2, GL10.GL_UNSIGNED_BYTE, mCamPoseIndices.position(0));	
		
		//draw camera frames
		GLES11.glColor4f(0.3f, 0.3f, 0.3f, 1.0f);
		GLES11.glDrawElements(GL10.GL_LINES, 8, GL10.GL_UNSIGNED_BYTE, mCamPoseIndices.position(8));
		
		GLES11.glLineWidth(6.0f);
		//draw x axis
		GLES11.glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
		GLES11.glDrawElements(GL10.GL_LINES, 2, GL10.GL_UNSIGNED_BYTE, mCamPoseIndices.position(2));
		//draw y axis
		GLES11.glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
		GLES11.glDrawElements(GL10.GL_LINES, 2, GL10.GL_UNSIGNED_BYTE, mCamPoseIndices.position(4));	
		//draw z axis
		GLES11.glColor4f(0.0f, 0.0f, 1.0f, 1.0f);
		GLES11.glDrawElements(GL10.GL_LINES, 2, GL10.GL_UNSIGNED_BYTE, mCamPoseIndices.position(6));							
	}
}

/**
 * CameraSyncedFramesListener provides interface for input processing tasks
 * Input processing tasks are to receive depth and rgb inputs when they become
 * available from camera. 
 */
interface CameraSyncedFramesListener {
	/**
	 * onSyncedFramesAvailable initially checks input quality to start SP
	 * module. On subsequent inputs, the function sets SP module's inputs
	 * and inputs to display components
	 * @param depthBuf buffer contains depth input values.
	 * @param colorData buffer contains rgba input values. 
	 * @param gravity values of x, y, z components of gravity vector.
	 */	
	public void onSyncedFramesAvailable(Object depthData, Object colorData, float[] gravity);
	
	/**
	 * onDestroy terminates possible threads and releases resources.
	 */
	public void onDestroy();
}

//interface with camera inputs processing module
/**
 * DepthProcessModule provides interface for high level process that handles 
 * depth and rgb frames coming from camera.
 * DepthProcessModule interface expects a configuration step for SP and UI module
 * , passing of inputs and setting status (for UI display).
 */
interface DepthProcessModule {
	/**
	 * configureDepthProcess configures SP module and UI module based on camera
	 * configuration parameters such as input size, color and depth camera's
	 * intrinsics and extrinsic parameters and resolution of SP.
	 */
	public void configureDepthProcess(int spResolution, Size inputSize, 
			Calibration.Intrinsics colorParams, 
			Calibration.Intrinsics depthParams, 
			float[] depthToColorTranslation);
	/**
	 * getSyncedFramesListener provides access to an input processing task of the
	 * depth process module. The CameraSyncedFramesListener returned is used to
	 * pass inputs for processing.
	 * @param camHandler handler to the camera processing module to allow call
	 * back upon frame process completion.
	 * @param inputSize specifies the size of expected inputs (in number of 
	 * pixels) that the listener will receive. 
	 * @return a input handler that receives input for processing.
	 */
	public CameraSyncedFramesListener getSyncedFramesListener(CameraHandler camHandler, 
			Size inputSize);	
	/**
	 * setProgramStatus updates depth processing with event status (e.g. camera
	 * events, frame delivery errors).
	 * The function will display the status on UI.
	 */
	public void setProgramStatus(final CharSequence newStatus);
}

/**
 * CameraHandler handles camera and delivers camera configuration and frames.
 */
abstract class CameraHandler {
	protected DepthProcessModule mDepthProcessor; //depth processor of camera frames
	protected Size mInputSize; //current input size
	protected SPCore mSPCore; //handle of SP module
	protected int mSPResolution;
	protected Calibration.Intrinsics mColorParams; //intrinsics param of color camera
	protected Calibration.Intrinsics mDepthParams; //intrinsics param of depth camera
	protected Calibration.Extrinsics mDepthToColorParams;
	
	public CameraHandler(Context mainContext, 
			DepthProcessModule depthProcessor) {
		mDepthProcessor = depthProcessor;	
			}
	
	/**
	 * queryScenePerception provides access to SP module. SP module MUST be
	 * enabled before in order to have a valid SP module instance.
	 * @return a valid SP module instance or null if SP module has not been
	 * enabled or SP module can not be configured.
	 * @see CameraHandler#enableScenePerception enableScenePerception
	 */
	public abstract SPCore queryScenePerception();
	
	/**
	 * frameProcessCompleteCallback is called to signal complete processing
	 * of an input set so that clean up / release of resources associated
	 * with the current input set could be done.
	 */
	public abstract void frameProcessCompleteCallback();
	
	/**
	 * onStart is called at during onStart event of Activity/Fragment.
	 * Camera bring up sequence could be initiated during this call.
	 */
	public abstract void onStart();
	
	/**
	 * onResume is called during onResume event of Activity/Fragment
	 * so that camera can resume or restart frame delivery.
	 */
	public abstract void onResume();
	
	/**
	 * onPause is called during onPause event of Activity/Fragment
	 * so that camera and/or frame delivery can be stopped or put
	 * on pause.
	 */
	public abstract void onPause();
	
	/**
	 * onDestroy is called during onDestroy event of Activity/Fragment
	 * so that resources or locking associated with camera can be 
	 * released.
	 */
	public abstract void onDestroy();
}
