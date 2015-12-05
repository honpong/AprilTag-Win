/********************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION This software is supplied under the
terms of a license agreement or nondisclosure agreement with Intel Corporation
and may not be copied or disclosed except in accordance with the terms of that
agreement.
Copyright(c) 2011-2015 Intel Corporation. All Rights Reserved.

*********************************************************************************/

package com.intel.blockstacking;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.util.LinkedList;
import java.util.Queue;
import javax.microedition.khronos.opengles.GL10;
import android.app.Activity;
import android.content.Context;
import android.opengl.GLES11;
import android.opengl.Matrix;
import android.util.Size;
import android.view.Gravity;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;
import com.intel.camera.toolkit.depth.Camera.Calibration;
import com.intel.camera.toolkit.depth.sceneperception.SPCore;
import com.intel.camera.toolkit.depth.sceneperception.SPTypes.CameraPose;
import com.intel.camera.toolkit.depth.sceneperception.SPTypes.SPInputStream;
import com.intel.camera.toolkit.depth.Point3DF;

/**
 * SPUtils includes a collection of functions and classes that helps to enable several functionalities in the sample.
 *
 */
public class SPUtils {
	/**
	 * FPSCalculator calculates frames per second. It requires calling 
	 * function {@link FPSCalculator#updateTimeOnFrame updateTimeOnFrame} upon addition of new frames.
	 */
	public static class FPSCalculator{
    	private float mAvgFPS = 0.0f;
    	private int mAvgWindowSize = 10;
    	private Queue<Float> mVals = new LinkedList<Float>();
    	private long mPrevTime = 0;
		private long mCurTime = 0;
    	
		/**
		 * constructor.
		 */
    	public FPSCalculator(){}
    	
    	/**
    	 * constructor with an averaging window size that corresponds to the 
    	 * number of fps calculations to be averaged.
    	 * @param avgWindowSize size of the averaging window.
    	 */
    	public FPSCalculator(int avgWindowSize){
    		mAvgWindowSize = avgWindowSize;
    	}
    	
    	/**
    	 * updates with elapse time of the new frame coming.
    	 * @param newTimeinMillis elapse time of the new frame.
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
    	 * gets the current fps.
    	 * @return current fps.
    	 */
    	public synchronized int getFPS() {
    		return (int)mAvgFPS;
    	}
    	
    	/**
    	 * gets a String that describes fps result.
    	 * @return a string of fps result.
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
    	 * resets fps counter.
    	 */
    	public synchronized void reset(){
			mVals.clear();
			mPrevTime = 0;
			mCurTime = 0;
			mAvgFPS = 0.0f;
    	}
	}
	
	/**
	 * displays a toast view message on screen.
	 * @param curActivity the activity to be displayed upon.
	 * @param msgContent content of message.
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
    	/**
    	 * x coordinate of the vector. 
    	 */
    	public float x;
    	/**
    	 * y coordinate of the vector. 
    	 */    	
    	public float y;
    	/**
    	 * z coordinate of the vector. 
    	 */    	
    	public float z;

    	/**
    	 * constructor.
    	 * @param _x value of in the x coordinate. 
    	 * @param _y value of in the y coordinate.
    	 * @param _z value of in the z coordinate.
    	 */
    	public float3(float _x, float _y, float _z){
    		x = _x;
    		y = _y;
    		z = _z;
    	}

    	/**
    	 * adds values from another vector.
    	 * @param rhs another vector.
    	 * @return a vector with updated value. 
    	 */
		float3 add(float3 rhs)
		{
			return new float3(x + rhs.x, y + rhs.y, z + rhs.z);
		}

    	/**
    	 * subtracts values from another vector.
    	 * @param rhs another vector.
    	 * @return a vector with updated value. 
    	 */		
		float3 sub(float3 rhs)
		{
			return new float3(x - rhs.x, y - rhs.y, z - rhs.z);
		}

    	/**
    	 * multiplies with another vector.
    	 * @param rhs another vector.
    	 * @return a vector with updated value. 
    	 */			
		float3 mul(float a)
		{
			return new float3(x * a, y * a, z * a);
		}

    	/**
    	 * divided by another vector.
    	 * @param rhs another vector.
    	 * @return a vector with updated value. 
    	 */		
		float3 div(float a)
		{
			a = 1.0f / a;
			return new float3(x * a, y * a, z * a);
		}	

		/**
		 * compares with another vector.
		 * @param rhs another vector.
		 * @return true if two vectors are equal and false otherwise.
		 */
		boolean equals(float3 rhs)
		{
			return (Math.abs(x - rhs.x) < EPS) && (Math.abs(y - rhs.y) < EPS) && (Math.abs(z - rhs.z) < EPS);
		}

		/**
		 * computes a dot product with another vector.
		 * @param rhs another vector.
		 * @return value of the dot product.
		 */		
		float dot(float3 rhs)
		{
			return x * rhs.x + y * rhs.y + z * rhs.z;
		}

		/**
		 * computes a cross product with another vector.
		 * @param rhs another vector.
		 * @return the cross product vector.
		 */		
		float3 cross(float3 right)
		{
			return new float3(y * right.z - z * right.y,
				z * right.x - x * right.z,
				x * right.y - y * right.x);
		}

		/**
		 * computes length of the vector.
		 * @return length of the vector.
		 */
		float length()
		{
			return (float) Math.sqrt(x * x + y * y + z * z);
		}

		/**
		 * normalizes the vector so its magnitude becomes 1.
		 * @return a normalized vector.
		 */
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
	 * computes cross product of two vectors of 03 components.
	 * @param lhs left side argument to the cross product.
	 * @param rhs right side argument to the cross product.
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
	 * normalizes a vector of 03 components so the length becomes 1.0f.
	 * @param vector an array of 03 components of x, y and z coordinates, to be 
	 * updated with normalized values. 
	 * @return an array that contains normalized values. 
	 */
	static public float[] normalizeVector(float[] vector) {
		double magnitude = Math.sqrt(vector[0] * vector[0] + vector[1] * vector[1] +
				vector[2] * vector[2]);	
		return new float[]{(float)(vector[0] / magnitude), (float)(vector[1] / magnitude),
				(float)(vector[2] / magnitude)};
	}
	
	/**
	 * changes the camera pose's rotation matrix (3x3) (not translation / 
	 * position) by taking a gravity vector to be the second row in the rotation 
	 * matrix.
	 * This is to ensure alignment of pose with the gravity so that the downward  
	 * orientation (given by the gravity vector) is also the downward orientation 
	 * of the reconstruction volume.
	 * @param camPose the pose whose rotation matrix is to be updated.
	 * @param input an input stream that includes samples of gravity whose values are in 
	 * the coordinate system of Scene Perception. The first gravity sample will be used.
	 * @return a new CameraPose whose rotation matrix is converted for alignment successfully or null
	 * otherwise.
	 */
	static CameraPose alignPoseWithGravity(CameraPose camPose, SPInputStream input) {	
		CameraPose alignedPose = new CameraPose(camPose);
		if (input != null && camPose != null) {
			float[] gravity = input.getGravityValues();			
			if (gravity.length >= 3) {
				gravity = normalizeVector(gravity);
				if (!CameraPose.alignPoseWithGravity(alignedPose, gravity, 2)) {
					alignedPose = null;
				}
			}
		}
		return alignedPose;
	}

	/**
	 * provides x, y, z coordinates of the middle point of the surface. The surface is constrained to 
	 * the current depth input. It uses camera calibration data to project a 3D point from a depth 
	 * value obtained at the middle pixel of the depth input image. 
	 * @param depthBuf contains values of a depth input.
	 * @param imgSize size of the depth input image in pixels.
	 * @param camParams camera calibration parameters.
	 * @return a 3D point that includes x, y and z coordinate values. 
	 */
	public static Point3DF getSurfaceCenterPoint(ByteBuffer depthBuf, Size imgSize, Calibration.Intrinsics camParams){
		int[] centerPixel = new int[]{ imgSize.getWidth() / 2, imgSize.getHeight() / 2 };
	    depthBuf.position(0);
	    float centerDepthValue = depthBuf.order(ByteOrder.nativeOrder()).asShortBuffer().get(centerPixel[0] + centerPixel[1] * imgSize.getWidth()) * 0.001f;
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
		/** resets view to initial state. */
		RESET(0), 
		/** zooms into the scene along the current camera's z axis. */ 
		ZOOM_IN(1),
		/** zooms out of the scene along the current camera's z axis. */
		ZOOM_OUT(2),
		/** moves camera view left or right from the current camera pose's orientation. */
		PAN(3),
		/** moves camera view left from the current camera pose's orientation. */
		PAN_LEFT(4),
		/** moves camera view right from the current camera pose's orientation. */
		PAN_RIGHT(5),
		/** moves camera view up or down from the current camera pose's orientation. */
		TILT(6),
		/** moves camera view up from the current camera pose's orientation. */
		TILT_UP(7),
		/** moves camera view down from the current camera pose's orientation. */
		TILT_DOWN(8),
		/** toggles camera view between a static position/orientation and a dynamic position/orientation that 
		 * changes in accordance with camera movement. */		
		TOGGLE_VIEW_POINT(9);
		ViewChange(int val) {
			this.mValue = val;
		}		
    	private final int mValue;
    	
    	/**
    	 * gets value of the enum.
    	 * @return value of the enum. 
    	 */
    	public int getValue() {return this.mValue;}		
	}
	
	/**
	 * computes a matrix 4x4 corresponding to a given view manipulation operation 
	 * and applies this view manipulation operation to a given view matrix.
	 * @param viewFactor one of the view manipulation operations defined in ViewChange.
	 * @param distance the amount of movement, measured in increments of 0.1f.
	 * @param curViewMatrix current view matrix being used to project the 3D content. The  
	 * matrix is represented as an array of 16 floats and in column major order. 
	 * @param poseChangeFactor is a view change matrix used for updating the view matrix. The  
	 * matrix is represented as an array of 16 floats and in column major order and it's applied to 
	 * (the left of) curViewMatrix parameter.  
	 * @return an array of 16 floats that represents the updated viewing matrix in column 
	 * major order. The update is by an application of the given view manipulation operation. 
	 */
	public static float[] getUpdatedViewMatrix(ViewChange viewFactor, float distance, 
			float[] curViewMatrix, float[] poseChangeFactor) {
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
					(float) Math.cos(moveAmount), 0, -(float)Math.sin(moveAmount), 0,
					0, 1, 0, 0, 
					(float)Math.sin(moveAmount), 0, (float)Math.cos(moveAmount), 0,
					0, 0, 0, 1};
			Matrix.multiplyMM(updatedViewMatrix, 0, changeMatrix, 0, curViewMatrix, 0);			
			break;

		case PAN_RIGHT:
			changeMatrix = new float[]{
					(float)Math.cos(-moveAmount), 0, -(float)Math.sin(-moveAmount), 0,
					0, 1, 0, 0, 
					(float)Math.sin(-moveAmount), 0, (float)Math.cos(-moveAmount), 0,
					0, 0, 0, 1};
			Matrix.multiplyMM(updatedViewMatrix, 0, changeMatrix, 0, curViewMatrix, 0);				
			break;
			
		case TILT:	
		case TILT_UP:
			changeMatrix = new float[]{
					1, 0, 0, 0,
					0, (float)Math.cos(-moveAmount), (float)Math.sin(-moveAmount), 0, 
					0, -(float)Math.sin(-moveAmount), (float)Math.cos(-moveAmount), 0,
					0, 0, 0, 1};
			Matrix.multiplyMM(updatedViewMatrix, 0, changeMatrix, 0, curViewMatrix, 0);				
			break;
			
		case TILT_DOWN:
			changeMatrix = new float[]{
					1, 0, 0, 0,
					0, (float)Math.cos(moveAmount), (float)Math.sin(moveAmount), 0, 
					0, -(float)Math.sin(moveAmount), (float)Math.cos(moveAmount), 0,
					0, 0, 0, 1};
			Matrix.multiplyMM(updatedViewMatrix, 0, changeMatrix, 0, curViewMatrix, 0);				
			break;					
			
		case RESET:		
			changeMatrix = new float[16];
			Matrix.invertM(changeMatrix, 0, curViewMatrix, 0);					
			Matrix.multiplyMM(updatedViewMatrix, 0, changeMatrix, 0, curViewMatrix, 0);						
			break;
		
		case TOGGLE_VIEW_POINT:
			Matrix.multiplyMM(updatedViewMatrix, 0, poseChangeFactor, 0, curViewMatrix, 0);	
			break;

		default:
			CameraPose.IDENTITY_POSE.copyToArray(updatedViewMatrix);;
			break;			
			
		}
		return updatedViewMatrix;
	}
}

/**
 * displays a camera pose with a rectangle window and x, y, z axes to show 
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
	
	/**
	 * constructor from another pose.
	 * @param camPose input pose whose values are copied.
	 */
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
	 * sets the surface center point so as to render a line from camera position 
	 * to the surface point.
	 * @param centerPoint the center point in the scene, given with x, y and z coordinate values.
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
	 * updates values of camera pose.
	 * @param newPose input pose whose values are copied.
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
	
	/**
	 * draws a rectangle around and 3-axes from the camera position to represent the camera at current pose.
	 */
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
 * CameraSyncedFramesListener provides an interface for input processing tasks.
 * Input processing tasks are to receive an input stream when it becomes
 * available from the camera and IMU sensors. 
 */
interface CameraSyncedFramesListener {
	/**
	 * processes input stream.
	 * @param input contains depth frame and color frame data, optionally includes
	 * IMU samples and their associated time stamps.
	 */	
	public void onSyncedFramesAvailable(SPInputStream input);
}


/**
 * DepthProcessModule provides an interface for high level process that handles 
 * depth and rgb frames coming from camera and IMU samples.
 * DepthProcessModule interface expects a configuration step for SP and UI module
 * , passing of inputs and setting status (for UI display).
 */
interface DepthProcessModule {
	/**
	 * configures SP module and UI module based on camera configuration parameters 
	 * such as input size, color and depth camera's intrinsics and extrinsic parameters 
	 * and resolution of SP.
	 */
	public void configureDepthProcess(int spResolution, Size depthInputSize, Size colorInputSize, 
			Calibration.Intrinsics colorParams, 
			Calibration.Intrinsics depthParams, 
			float[] depthToColorTranslation);
	/**
	 * provides access to an input processing task of the depth process module. The 
	 * CameraSyncedFramesListener returned is used to pass in inputs for processing.
	 * @param camHandler handler to the camera processing module to allow call
	 * back upon frame process completion.
	 * @param depthInputSize specifies the size of expected depth input (in number of 
	 * pixels) that the listener will receive. 
	 * @return a input handler that receives input for processing.
	 */
	public CameraSyncedFramesListener getSyncedFramesListener(CameraHandler camHandler, 
			Size depthInputSize);	
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
	protected Size mColorInputSize; //current input size for color stream
	protected Size mDepthInputSize; //current input size for depth stream
	protected SPCore mSPCore; //handle of SP module
	
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
