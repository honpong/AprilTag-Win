/********************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION This software is supplied under the
terms of a license agreement or nondisclosure agreement with Intel Corporation
and may not be copied or disclosed except in accordance with the terms of that
agreement.
Copyright(c) 2011-2015 Intel Corporation. All Rights Reserved.

*********************************************************************************/

package com.intel.sample.depth.spsamplecommon;


import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.LinkedList;
import java.util.Queue;
import android.app.Activity;
import android.opengl.GLES30;
import android.opengl.Matrix;
import android.util.Size;
import android.view.Gravity;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;
import com.intel.camera.toolkit.depth.Camera.Calibration;
import com.intel.camera.toolkit.depth.sceneperception.SPTypes.CameraPose;
import com.intel.camera.toolkit.depth.sceneperception.SPTypes.SPInputStream;
import com.intel.camera.toolkit.depth.Point3DF;

import javax.microedition.khronos.egl.EGL10;
import android.util.Log;

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
    	private float x;
    	/**
    	 * y coordinate of the vector. 
    	 */
		private float y;
    	/**
    	 * z coordinate of the vector. 
    	 */
		private float z;

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
		 * @return x component of the vector
		 */
		public float getX() {
			return this.x;
		}
		/**
		 * @return y component of the vector
		 */

		public float getY() {
			return this.y;
		}
		/**
		 * @return z component of the vector
		 */
		public float getZ() {
			return this.z;
		}

    	/**
    	 * adds values from another vector.
    	 * @param rhs another vector.
    	 * @return a vector with updated value. 
    	 */
		public float3 add(float3 rhs)
		{
			return new float3(x + rhs.x, y + rhs.y, z + rhs.z);
		}

    	/**
    	 * subtracts values from another vector.
    	 * @param rhs another vector.
    	 * @return a vector with updated value. 
    	 */		
		public float3 sub(float3 rhs)
		{
			return new float3(x - rhs.x, y - rhs.y, z - rhs.z);
		}

    	/**
    	 * multiplies with another vector.
    	 * @param rhs another vector.
    	 * @return a vector with updated value. 
    	 */			
		public float3 mul(float rhs)
		{
			return new float3(x * rhs, y * rhs, z * rhs);
		}

    	/**
    	 * divided by another vector.
    	 * @param rhs another vector.
    	 * @return a vector with updated value.
    	 */		
		public float3 div(float rhs)
		{
			rhs = 1.0f / rhs;
			return new float3(x * rhs, y * rhs, z * rhs);
		}	

		/**
		 * compares with another vector.
		 * @param rhs another vector.
		 * @return true if two vectors are equal and false otherwise.
		 */
		public boolean equals(float3 rhs)
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
		 * @param right another vector.
		 * @return the cross product vector.
		 */		
		public float3 cross(float3 right)
		{
			return new float3(y * right.z - z * right.y,
				z * right.x - x * right.z,
				x * right.y - y * right.x);
		}

		/**
		 * computes length of the vector.
		 * @return length of the vector.
		 */
		public float length()
		{
			return (float) Math.sqrt(x * x + y * y + z * z);
		}

		/**
		 * normalizes the vector so its magnitude becomes 1.
		 * @return a normalized vector.
		 */
		public float3 normalize()
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
	 * @param camPose the pose whose rotation matrix's values are used to start the alignment with
	 * gravity.
	 * @param input an input stream that includes samples of gravity whose values are in 
	 * the coordinate system of Scene Perception. The first gravity sample will be used.
	 * @return if the alignment is successful, returns a camera pose that is aligned with gravity. 
	 * Otherwise, return a same camera pose as the input camera pose. 


	 */
	public static CameraPose getGravityAlignedPoseIfApplicable(CameraPose camPose, SPInputStream input) {
		boolean isAligned = false;
		CameraPose alignedPose = new CameraPose(camPose);
		if (input != null && camPose != null) {
			float[] gravity = new float[3];			
			input.getNearestGravitySample(gravity);
			if (gravity.length >= 3) {
				gravity = normalizeVector(gravity);
				if (CameraPose.alignPoseWithGravity(alignedPose, gravity, 2)) {
					isAligned = true;

				}
			}
		}
		
		if (!isAligned) {
			alignedPose.set(camPose);
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


	private static final String TAG = "GLESUtils";

	public static String glErrorToString(int error)
	{
		switch(error)
		{
			case GLES30.GL_NO_ERROR:
				return "GL_NO_ERROR";
			case GLES30.GL_INVALID_ENUM:
				return "GL_INVALID_ENUM";
			case GLES30.GL_INVALID_VALUE:
				return "GL_INVALID_VALUE";
			case GLES30.GL_INVALID_OPERATION:
				return "GL_INVALID_OPERATION";
			case GLES30.GL_INVALID_FRAMEBUFFER_OPERATION:
				return "GL_INVALID_FRAMEBUFFER_OPERATION";
			case GLES30.GL_OUT_OF_MEMORY:
				return "GL_OUT_OF_MEMORY";
			default:
				return "unknown gl error";
		}


	}

	public static String eglErrorToString(int error)
	{
		switch(error)
		{
			case EGL10.EGL_SUCCESS:
				return "EGL_SUCCESS";
			case EGL10.EGL_BAD_MATCH :
				return "EGL_BAD_MATCH";
			case EGL10.EGL_BAD_DISPLAY :
				return "EGL_BAD_DISPLAY";
			case EGL10.EGL_NOT_INITIALIZED :
				return "EGL_NOT_INITIALIZED ";
			case EGL10.EGL_BAD_CONFIG :
				return "EGL_BAD_CONFIG ";
			case EGL10.EGL_BAD_CONTEXT:
				return "EGL_BAD_CONTEXT";
			case EGL10.EGL_BAD_ATTRIBUTE :
				return "EGL_BAD_ATTRIBUTE";
			case EGL10.EGL_BAD_ALLOC :
				return "EGL_BAD_ALLOC";

			default:
				return "unknown egl error";
		}


	}

	public static void throwExceptionIfGLError() {
		int error = GLES30.glGetError();

		if(error != GLES30.GL_NO_ERROR) {
			Log.e(TAG, "error: " + glErrorToString(error));
			throw new RuntimeException(glErrorToString(error));
		}
	}

	public static class GLShader {
		private int mProgramHandle;	// Handle for the Shader program
		private boolean isCompiled = false;

		private final String TAG = "SP_GLShader";

		public void initialize(String vertexShaderCode, String fragmentShaderCode) {
			int vs = compileShader(GLES30.GL_VERTEX_SHADER, vertexShaderCode);
			int fs = compileShader(GLES30.GL_FRAGMENT_SHADER, fragmentShaderCode);
			Log.d(TAG, "Vertex Shader Compile: " + GLES30.glGetShaderInfoLog(vs));
			Log.d(TAG, "Frag Shader Compile: " + GLES30.glGetShaderInfoLog(fs));
			mProgramHandle = GLES30.glCreateProgram();
			GLES30.glAttachShader(mProgramHandle, vs);
			GLES30.glAttachShader(mProgramHandle, fs);
			GLES30.glLinkProgram(mProgramHandle);
			isCompiled = true;
		}

		public int getProgramHandle() {
			if(!isCompiled)
				return -1;
			else
				return mProgramHandle;
		}

		public void setMVP(float [] mvp) {
			int mvpMatrixHandle = GLES30.glGetUniformLocation(mProgramHandle, "uMVPMatrix");
			GLES30.glUniformMatrix4fv(mvpMatrixHandle, 1, false, mvp, 0);
		}

		public void setMV(float [] mv)	{
			int mvMatrixHandle = GLES30.glGetUniformLocation(mProgramHandle, "uMVMatrix");
			GLES30.glUniformMatrix4fv(mvMatrixHandle, 1, false, mv, 0);
		}

		public void useShader()	{
			GLES30.glUseProgram(mProgramHandle);
		}

		public boolean isCompiled()	{
			return isCompiled;
		}

		public int compileShader(final int shaderType, final String shaderSource)   {
			int handle = GLES30.glCreateShader(shaderType);
			if(handle != 0) {
				GLES30.glShaderSource(handle, shaderSource);
				GLES30.glCompileShader(handle);
				Log.d(TAG, "Shader Compile: " + GLES30.glGetShaderInfoLog(handle));
				int[] status = new int[1];
				GLES30.glGetShaderiv(handle, GLES30.GL_COMPILE_STATUS, status, 0);
				if(status[0]==0) {
					Log.d(TAG,"Error compiling shader");
					GLES30.glDeleteShader(handle);
					handle = 0;
				}
			}
			return handle;
		}
	}
}




