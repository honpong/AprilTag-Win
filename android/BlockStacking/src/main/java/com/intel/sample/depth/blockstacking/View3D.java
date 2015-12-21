/********************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION This software is supplied under the
terms of a license agreement or nondisclosure agreement with Intel Corporation
and may not be copied or disclosed except in accordance with the terms of that
agreement.
Copyright(c) 2011-2015 Intel Corporation. All Rights Reserved.

*********************************************************************************/

package com.intel.sample.depth.blockstacking;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import com.intel.camera.toolkit.depth.Point3DF;
import com.intel.camera.toolkit.depth.sceneperception.SPTypes.CameraPose;
import com.intel.camera.toolkit.depth.sceneperception.SPTypes.CameraStreamIntrinsics;
import com.intel.sample.depth.spsamplecommon.CameraPoseDisplay;
import com.intel.sample.depth.spsamplecommon.SPUtils;
import com.intel.sample.depth.spsamplecommon.SPUtils.ViewChange;
import com.intel.sample.depth.spsamplecommon.SPUtils.GLShader;

import android.content.Context;
import android.graphics.Bitmap;
import android.opengl.GLES30;
import android.opengl.GLSurfaceView;
import android.opengl.GLUtils;
import android.opengl.Matrix;
import android.os.Handler;
import android.util.AttributeSet;
import android.util.Size;
import android.view.SurfaceHolder;

class View3D extends  GLSurfaceView implements
		SurfaceHolder.Callback {
	// indicate if the last display update task is completed
	private volatile boolean mIsLastDisplayComplete = true;
	private Handler mUiHandler; //handler to UI thread
	
	public View3D(Context context) {
		super(context);
		this.setEGLContextClientVersion(3);
	}
	
	public View3D(Context context, AttributeSet attrs) {
		super(context, attrs);
		this.setEGLContextClientVersion(3);
	}
	
	/**
	 * set to UI thread handler to enable posting UI events
	 * @param uiHandler
	 */
	public void setUIHandler(Handler uiHandler){
		mUiHandler = uiHandler;
	}
	
	/**
	 * onInputUpdate is called when there is new data in the display 
	 * content. 
	 * Calling this function is required when the rendering mode 
	 * RENDERMODE_WHEN_DIRTY is used.
	 */
	public void onInputUpdate() {
		if (mIsLastDisplayComplete) {
			mIsLastDisplayComplete = false;		
			mUiHandler.post(new Runnable(){
				@Override
				public void run(){
					(View3D.this).requestRender();
					mIsLastDisplayComplete = true;
				}				
			});		
		}
	}
}

/**
 * ReconstructionRenderer renders display of the most recently tracked camera pose 
 * and either images of reconstruction rendering or live mesh pieces of the reconstruction.
 *
 */
class ReconstructionRenderer implements GLSurfaceView.Renderer {
	
	//current camera pose to be rendered
	private CameraPoseDisplay mCurPose;
	// initial zooming factor of the viewing angle
	private static float mInitZoomFactor = -3.0f;
	/**
	 * set the initial zooming factor of the view angle from which, the reconstruction 
	 * will be rendered.
	 * @param zoomFactor
	 */
	public static void setInitialZoomFactor(float zoomFactor) {
		mInitZoomFactor = zoomFactor;
	}	
	
	// indicate if data required for setting up camera projection matrix is ready
	private volatile boolean mIsCamProjectionDataReady = false;
	// projection matrix for rendering camera pose or mesh content
	private float[] mSPProjectionMat; 
	private int mImgWidth, mImgHeight; // image dimension of render volume image output	
		
	// Shader for displaying the rendering of the volume view
	private GLShader mShader;

	// Handle for passing the texture data to the shader
	private int mTextureDataHandle;
	
	// Matrices for controlling the display
	private float[] mModelViewMatrix = new float[16];
	private float[] mProjectionMatrix = new float[16];
	private float[] mMVPMatrix = new float[16];


	/**
	 * set camera calibration parameters
	 * @param renderCamIntrinsics that includes:
	 * <br> horizontal focal length
	 * <br> vertical focal length
	 * <br> u-coordinate of principle point
	 * <br> v-coordinate of principle point
	 * width dimension of image of rendering
	 * height dimension of image of rendering
	 */
	public void setCameraParams(CameraStreamIntrinsics renderCamIntrinsics){	
		mImgWidth = renderCamIntrinsics.getImageWidth();
		mImgHeight = renderCamIntrinsics.getImageHeight();		
		mSPProjectionMat = configureAugmentedCamera(renderCamIntrinsics.getFocalLengthHorizontal(),
				renderCamIntrinsics.getFocalLengthVertical(), 0.0f, renderCamIntrinsics.getPrincipalPointCoordU(), 
				mImgHeight - renderCamIntrinsics.getPrincipalPointCoordV(), mImgWidth,
	    		mImgHeight, 0.01f, 1000.0f);	
		mIsCamProjectionDataReady = true;
	}

	/**
	 * setCameraPose updates current internal camera pose to be rendered. 
	 * @param camPose current camera pose to be set to.
	 */
	public void setCameraPose(CameraPose camPose) {
		mCurPose.set(camPose);
	}
	
	/**
	 * setBaseRenderMatrix updates the base render matrix. The base render matrix is 
	 * used to derive render matrix by incorporating the view changes. It also updates 
	 * render matrix if the view point is set from the camera pose. 
	 * @param camPose current camera pose to be set to.
	 * @param isStaticViewPoint indicates if the viewpoint is fixed (value true) or 
	 * is changed (value false) as the current pose parameter changes. 
	 */
	public void setBaseRenderMatrix(CameraPose camPose, boolean isStaticViewPoint) {
		//update base value of render projection
		Matrix.transposeM(mBaseReconstRenderMatrix, 0, camPose.get(), 0);		
		if(!isStaticViewPoint) {			
			// render matrix is computed from initial value with movement factor
			Matrix.multiplyMM(mRenderMatrix, 0, mBaseReconstRenderMatrix, 0, mViewChangeMatrix, 0);			
			computeOpenGLRenderMatrix();
		}
	}	
	/**
	 * set the middle point of the surface so as to render a line from 
	 * camera pose to indicate where the camera is looking at in the 
	 * surface
	 * @param centerPoint contains x, y , z coordinates.
	 */
	public void setCenterSurfacePoint(Point3DF centerPoint) {
		mCurPose.setCenterSurfacePoint(centerPoint);
	}
	
	// conversion matrix for changing Scene Perception coordinate system to 
	// OpenGLES coordinate system. 
	private static final float[] SWITCH_COORD_SYS_MAT = new float[]{
			1.0f, 0.0f, 0.0f, 0.0f, 
			0.0f, -1.0f, 0.0f, 0.0f, 
			0.0f, 0.0f, -1.0f, 0.0f, 
			0.0f, 0.0f, 0.0f, 1.0f};	
	/**
	 * computeOpenGLRenderMatrix derives GL render matrix from Scene Perception's render 
	 * matrix and takes into account coordinate system change. 
	 */
	private void computeOpenGLRenderMatrix() {			
		float[] invRenderMat = new float[16];
		Matrix.invertM(invRenderMat, 0, mRenderMatrix, 0);
		Matrix.multiplyMM(mOpenGLRenderMatrix, 0, SWITCH_COORD_SYS_MAT, 0, invRenderMat, 0);
	}
	
	private volatile boolean mIsResetView = false; //indicate if there is a request to reset view	
	/**
	 * reset projection matrix to view camera pose and mesh to initial value 
	 * and resets the image of reconstruction to a blank image.  
	 */
	public void resetView() {
		setViewChange(ViewChange.RESET, 1.0f);
		if (mVolumeViewBmp != null && mIsCamProjectionDataReady) {
			synchronized (mVolumeViewBmp) {
				mVolumeViewBmp = Bitmap.createBitmap(mImgWidth,
						mImgHeight, Bitmap.Config.ARGB_8888);
			}
		}
		mIsResetView = true;
	}
	// camera pose from which to obtain a view of reconstruction volume	
	private float[] mRenderMatrix = new float[16];
	private float[] mTransposeRenderMatrix = new float[16];
	/**
	 * get the camera pose from which to obtain a view of the reconstruction 
	 * volume.
	 * @return an array of 16 floats the represent the viewing camera pose 
	 * in row major order.
	 */
	public float[] getRenderMatrix() {
		Matrix.transposeM(mTransposeRenderMatrix, 0, mRenderMatrix, 0);
		return mTransposeRenderMatrix.clone();
	}
	// projection matrix for OpenGL to render camera pose and mesh pieces
	private float[] mOpenGLRenderMatrix = new float[16];
	// base projection matrix to (re)start from
	private float[] mBaseReconstRenderMatrix = new float[16];
	// representation of a displacement of viewing point
	private float[] mViewChangeMatrix = new float[16];	
	/**
	 * set the initial projection matrix to render camera pose and mesh 
	 * pieces.
	 * @param initPose the viewing pose from which the content of the 
	 * reconstruction is obtained. 
	 */
	public void setInitialRenderPose(CameraPose initPose){
		Matrix.transposeM(mBaseReconstRenderMatrix, 0, initPose.get(), 0);
		//initialize camera movement factor
		System.arraycopy(CameraPose.IDENTITY_POSE.get(), 0, mViewChangeMatrix, 0, 16);		
		mViewChangeMatrix[14] = mInitZoomFactor;
		// render matrix is computed from initial value with view change factor
		Matrix.multiplyMM(mRenderMatrix, 0, mBaseReconstRenderMatrix, 0, mViewChangeMatrix, 0);
		computeOpenGLRenderMatrix();		
	}
	
	/**
	 * update the viewing angle and distance based on the view change action 
	 * @param viewFactor action to change viewing angle
	 * @param distance the amount/intensity of change
	 * @param poseChangeFactor is a view change matrix used for updating the view matrix. The  
	 * matrix is represented as an array of 16 floats and in column major order. 
	 */
	public void setViewChange(ViewChange viewFactor, float distance, float[] poseChangeFactor) {
		
		float[] prevMoveMatrix = new float[16];
		System.arraycopy(mViewChangeMatrix, 0, prevMoveMatrix, 0, 16);
		
		//update camera view change factor incrementally		
		mViewChangeMatrix = SPUtils.getUpdatedViewMatrix(viewFactor, distance, prevMoveMatrix, poseChangeFactor);
		if (viewFactor == ViewChange.RESET) {
			mViewChangeMatrix[14] = mInitZoomFactor; //reset zoom factor
		}
		
		// render matrix is computed from initial base value with view change factor
		Matrix.multiplyMM(mRenderMatrix, 0, mBaseReconstRenderMatrix, 0, mViewChangeMatrix, 0);	
		computeOpenGLRenderMatrix();					
	}
	
	/**
	 * update the viewing angle and distance based on the view change action 
	 * @param viewFactor action to change viewing angle
	 * @param distance the amount/intensity of change 
	 */
	public void setViewChange(ViewChange viewFactor, float distance) {
		setViewChange(viewFactor, distance, null);
	}	
	
	// vertices of the rectangle frame to hold images of reconstruction rendering
	private FloatBuffer mRecVertexBuffer = ByteBuffer.allocateDirect(12 * 4).order(ByteOrder.nativeOrder()).asFloatBuffer();
	

    // Mapping coordinates for the vertices
    private static float[] TEXTURE_COORDINATE = {	0.0f, 1.0f, 
    												0.0f, 0.0f, 
    												1.0f, 1.0f, 
    												1.0f, 0.0f	};
    private FloatBuffer mTextCoordBuffer = ByteBuffer.allocateDirect(
    		TEXTURE_COORDINATE.length * 4).order(ByteOrder.nativeOrder()).asFloatBuffer();

	private int[] mVolTextBO = new int[1];
    
        
	private final float[] mBackgroundColor = new float[]{0.0f, 0.0f, 1.0f, 1.0f};
	@Override
	public void onSurfaceCreated(GL10 gl, EGLConfig config) {
		// Initialize the camera pose display
		mCurPose = new CameraPoseDisplay(CameraPose.IDENTITY_POSE);
		GLES30.glClearColor(mBackgroundColor[0], mBackgroundColor[1], mBackgroundColor[2], mBackgroundColor[3]);			
		
		// load the shader
		mShader = new GLShader();
		mShader.initialize(VERTEX_SHADER_CODE, FRAGMENT_SHADER_CODE);
		mTextCoordBuffer.put(TEXTURE_COORDINATE);
		mTextCoordBuffer.position(0);

		GLES30.glGenTextures(1, mVolTextBO, 0);
	}

	private float mDisplayDimenRatio = 0;
	private static final float FIELD_OF_VIEW = 70.0f; // in degrees
	private Size mDisplaySize = new Size(0,0);
	
	@Override
	public void onSurfaceChanged(GL10 gl, int width, int height) {
		gl.glViewport(0, 0, width, height);
		mDisplaySize = new Size(width, height);
		// re-compute rectangle size to hold volume render image texture
		mDisplayDimenRatio = (float)width/height;
		float botTop = (float)Math.tan((FIELD_OF_VIEW /2)/180*Math.PI);
		float leftRight = botTop * mDisplayDimenRatio;
		float distToPlane = 1.0f / botTop; //distance from camera to the image plane
		//vertices of the rectangle of appropriate size to hold images of 
		// reconstruction rendering
		float[] rectVertices = new float[]{
				-leftRight * distToPlane, -botTop * distToPlane, -distToPlane,
				-leftRight * distToPlane, botTop * distToPlane, -distToPlane,
				leftRight  * distToPlane, -botTop * distToPlane, -distToPlane,
				leftRight * distToPlane, botTop * distToPlane, -distToPlane
		};
		mRecVertexBuffer.put(rectVertices);
		mRecVertexBuffer.position(0);
	}		

	/**
	 * get the current size of the display view component
	 * @return
	 */
	public Size getDisplaySize() {
		return mDisplaySize;
	}
	private volatile Bitmap mVolumeViewBmp; //bitmap image of reconstruction rendering
	private volatile boolean mIsVolumeRenderUpdated = true;
	
	public void onInputUpdate(ByteBuffer newContent) {
		if (newContent == null) {
			return;
		}		
		if (newContent.capacity() > 0 && mIsCamProjectionDataReady) {
			if (mVolumeViewBmp == null) {
				mVolumeViewBmp = Bitmap.createBitmap(mImgWidth,
						mImgHeight, Bitmap.Config.ARGB_8888);
			}
			newContent.rewind();
			synchronized (mVolumeViewBmp) {
				mVolumeViewBmp.copyPixelsFromBuffer(newContent);
				mIsVolumeRenderUpdated = true;
			}
			mIsResetView = false;
		}
	}

	private void drawVolumeRenderView(){
		float[] prevModelViewMatrix = new float[16];
		float[] prevProjectionMatrix = new float[16];
		if (mVolumeViewBmp != null) {
			mShader.useShader();
			int programHandle = mShader.getProgramHandle();

			// configure default projection Matrix for rendering texture
			System.arraycopy(mProjectionMatrix, 0, prevProjectionMatrix, 0, 16);
			Matrix.setIdentityM(mProjectionMatrix, 0);				
			setPerspectiveMatrix(mProjectionMatrix, FIELD_OF_VIEW, mDisplayDimenRatio, 0.0001f, 20.0f);
			
			// configure model view matrix
			System.arraycopy(mModelViewMatrix, 0, prevModelViewMatrix, 0, 16);
			Matrix.setIdentityM(mModelViewMatrix, 0);
			Matrix.multiplyMM(mMVPMatrix, 0, mProjectionMatrix, 0, mModelViewMatrix, 0);
			
			// Set the MV matrix	
			mShader.setMV(mModelViewMatrix);
			mShader.setMVP(mMVPMatrix);

			// bind texture
			synchronized (mVolumeViewBmp) {
				if (mIsVolumeRenderUpdated) { //only uploading if the content of bitmap changes
					mTextureDataHandle = loadTexture(mVolumeViewBmp);
					mIsVolumeRenderUpdated = false;
				}
			}
			GLES30.glFrontFace(GLES30.GL_CW);
			int textureUniformHandle = GLES30.glGetUniformLocation(programHandle, "uTexture");
	        int textureCoordinateHandle = GLES30.glGetAttribLocation(programHandle, "aTextureCoord");
	        GLES30.glActiveTexture(GLES30.GL_TEXTURE0);
	        GLES30.glBindTexture(GLES30.GL_TEXTURE_2D, mTextureDataHandle);
	        GLES30.glUniform1i(textureUniformHandle, 0);
			
	        // Set the vertex positions
	        int positionHandle = GLES30.glGetAttribLocation(programHandle, "aPosition");
	        GLES30.glVertexAttribPointer(positionHandle, mRecVertexBuffer.capacity() / 4, GLES30.GL_FLOAT, false,
					0, mRecVertexBuffer);
	        GLES30.glEnableVertexAttribArray(positionHandle);
			
	        // Set the vertex colors
	        int colorHandle = GLES30.glGetAttribLocation(programHandle, "aColor");
	        GLES30.glVertexAttrib4f(colorHandle, 1.0f, 1.0f, 1.0f, 1.0f);
	        GLES30.glDisableVertexAttribArray(colorHandle);
			
	        // Set the textures
	        GLES30.glVertexAttribPointer(textureCoordinateHandle, mTextCoordBuffer.capacity() / 4, GLES30.GL_FLOAT, false,
					0, mTextCoordBuffer);
	        GLES30.glEnableVertexAttribArray(textureCoordinateHandle);
						
	        // Draw the GL elements
	        GLES30.glDrawArrays(GLES30.GL_TRIANGLE_STRIP, 0, 4);
			// restore projection and modelview matrices
			System.arraycopy(prevProjectionMatrix, 0, mProjectionMatrix, 0, 16);
			System.arraycopy(prevModelViewMatrix, 0, mModelViewMatrix, 0, 16);
			Matrix.multiplyMM(mMVPMatrix, 0, mProjectionMatrix, 0, mModelViewMatrix, 0);

			mShader.setMV(mModelViewMatrix);
			mShader.setMVP(mMVPMatrix);
		}
	}

	private boolean mIsProjectionMatrixDefined = false;	
	@Override
	public void onDrawFrame(GL10 gl) {
		mShader.useShader();
		GLES30.glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		GLES30.glClear(GLES30.GL_COLOR_BUFFER_BIT | GLES30.GL_DEPTH_BUFFER_BIT);

		if (!mIsCamProjectionDataReady) return;
		
		if (mIsResetView) return; //do not draw during reset/stop tracking
		
		//define projection matrix for drawing camera pose or mesh once
		if (!mIsProjectionMatrixDefined) {
		    System.arraycopy(mSPProjectionMat, 0, mProjectionMatrix, 0, 16);
		    mIsProjectionMatrixDefined = true;
		}
	
		//define model view matrix for drawing camera pose or mesh
		System.arraycopy(mOpenGLRenderMatrix, 0, mModelViewMatrix, 0, 16);

		// Compute new MVP matrix
		Matrix.multiplyMM(mMVPMatrix, 0, mProjectionMatrix, 0, mModelViewMatrix, 0);

		// set the modelview and mvp matrix
		mShader.setMV(mModelViewMatrix);
		mShader.setMVP(mMVPMatrix);

		//draw image of reconstruction rendering
		drawVolumeRenderView();

		//draw camera pose
		mCurPose.draw(mModelViewMatrix, mMVPMatrix);
	}
	
	/**
	 * configure a projection matrix based on camera parameters  
	 * @param fx
	 * @param fy
	 * @param skew
	 * @param u0
	 * @param v0
	 * @param imgWidth
	 * @param imgHeight
	 * @param nearClip
	 * @param farClip
	 * @return projection matrix as an array of 16 floats
	 */
    private float[] configureAugmentedCamera(float fx, float fy, 
    		float skew, float u0, float v0, int imgWidth, int imgHeight,
    		float nearClip, float farClip) {
    	//derive orthographic matrix m_orthMatrix
    	float[] orth = new float[16];
    	calOrthographicMat(orth, imgWidth, imgHeight, nearClip, farClip);
    	float[] K = new float[16];
    	setCamParamsMatrix(K, fx, fy, skew, u0, v0, nearClip, farClip);
    	float[] transposeOrth = new float[16];
    	Matrix.transposeM(transposeOrth, 0, orth, 0);
    	float[] transposeK = new float[16];
    	Matrix.transposeM(transposeK, 0, K, 0);
    	float[] projection = new float[16];
    	Matrix.multiplyMM(projection, 0, transposeOrth, 0, transposeK, 0);
    	return projection;
    }
    
    private void calOrthographicMat(float[] orth, int imgWidth, int imgHeight, double nearClip, double farClip) {
    	int L = 0;
    	int R = imgWidth;
    	int B = 0;
    	int T = imgHeight;
    	orth[0 * 4 + 0] = 2.0f / (R - L);
    	orth[0 * 4 + 3] = (float)(-(R + L) / (R - L));
    	orth[1 * 4 + 1] = 2.0f / (T - B);
    	orth[1 * 4 + 3] = (float)(-(T + B) / (T - B));
    	orth[2 * 4 + 2] = (float)(-2.0f / (farClip - nearClip));
    	orth[2 * 4 + 3] = (float)(-(farClip + nearClip) / (farClip - nearClip));
    	orth[3 * 4 + 3] = 1.0f;
    }

	/**
	 *  derive camera's intrinsic parameter matrix based on focal lengths (fx, fy) 
	 *  and principle points (u, v). 
	 * @param K
	 * @param alpha
	 * @param beta
	 * @param skew
	 * @param u0
	 * @param v0
	 * @param nearClip
	 * @param farClip
	 */
	private static void setCamParamsMatrix(float[] K, double alpha, double beta, double skew,
			double u0, double v0, double nearClip, double farClip) {
		K[0 * 4 + 0] = (float)(alpha);//negative for mirroring
		K[0 * 4 + 2] = (float)(-u0);
		K[1 * 4 + 1] = (float)(beta);//negative for flipping
		K[1 * 4 + 2] = (float)(-v0);
		K[2 * 4 + 2] = (float)(nearClip + farClip);
		K[2 * 4 + 3] = (float)(nearClip * farClip);
		K[3 * 4 + 2] = -1.0f;
	}
	
	
	private int loadTexture(Bitmap bitmap) {
		if (mVolTextBO[0] != 0)
		{
			// Bind to the texture in OpenGL
			GLES30.glBindTexture(GLES30.GL_TEXTURE_2D, mVolTextBO[0]);

			// Set filtering
			GLES30.glTexParameteri(GLES30.GL_TEXTURE_2D, GLES30.GL_TEXTURE_MIN_FILTER, GLES30.GL_NEAREST);
			GLES30.glTexParameteri(GLES30.GL_TEXTURE_2D, GLES30.GL_TEXTURE_MAG_FILTER, GLES30.GL_LINEAR);
			
			// Load the bitmap into the bound texture.
			GLUtils.texImage2D(GLES30.GL_TEXTURE_2D, 0, bitmap, 0);
		}
		
		if (mVolTextBO[0] == 0) {
			throw new RuntimeException("Error loading texture.");
		}
		return mVolTextBO[0];
	}

	public static void setPerspectiveMatrix(float[] matrix, float fovY, float aspectRatio, float zNear, float zFar) {

		float yMax = zNear * (float)Math.tan(fovY * Math.PI / 360.0);
		float xMax = yMax * aspectRatio;
		float right = xMax;
		float left = -xMax;
		float top = yMax;
		float bottom = -yMax;

		matrix[0*4+0] = 2.0f * zNear / (right - left);
		matrix[0*4+1] = 0.0f;
		matrix[0*4+2] = 0.0f;
		matrix[0*4+3] = 0.0f;
		matrix[1*4+0] = 0.0f;
		matrix[1*4+1] = 2.0f * zNear / (top - bottom);
		matrix[1*4+2] = 0.0f;
		matrix[1*4+3] = 0.0f;
		matrix[2*4+0] = (right + left) / (right - left);
		matrix[2*4+1] = (top + bottom) / (top - bottom);
		matrix[2*4+2] = (-zFar - zNear) / (zFar - zNear);
		matrix[2*4+3] = -1.0f;
		matrix[3*4+0] = 0.0f;
		matrix[3*4+1] = 0.0f;
		matrix[3*4+2] = (-2.0f * zNear * zFar) / (zFar - zNear);
		matrix[3*4+3] = 0.0f;
	}
	

	private final String VERTEX_SHADER_CODE =
		// Input vertex attributes
		"attribute vec3 aPosition;\n"+
		"attribute vec4 aColor;\n"+
		"attribute vec2 aTextureCoord;\n"+
		// Matrices for the transformation of the vertices
		"uniform mat4 uMVPMatrix;\n"+
		"uniform mat4 uMVMatrix;\n"+
		// Output to fragment shader
		"varying vec3 vPosition;\n"+
		"varying vec4 vColor;\n"+
		"varying vec2 vTextureCoord;\n"+
		// main shader program
		"void main()\n"+                                                 	
		"{\n"+
		// modify the vertex position using the model-view matrix
		"	vPosition = vec3(uMVMatrix * vec4(aPosition, 1.0f));\n"+
		"	vColor = aColor;\n"+
		"	vTextureCoord = aTextureCoord;\n"+
		"	gl_Position = uMVPMatrix * vec4(aPosition, 1.0f);\n"+
		"}\n";
		

	private final String FRAGMENT_SHADER_CODE =
		// precision for fragment shader
		"precision mediump float;\n"+
		// sampler for the texture
		"uniform sampler2D uTexture;\n"+
		// input position and color for the fragment from the vertex shader
		"varying vec3 vPosition;\n"+
		"varying vec4 vColor;\n"+
		"varying vec2 vTextureCoord;\n"+
		// main shader program
		"void main()\n"+
		"{\n"+
		"	gl_FragColor = (vColor * texture2D(uTexture, vTextureCoord));\n"+
		"}\n";
}
