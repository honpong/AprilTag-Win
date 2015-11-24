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
import java.nio.IntBuffer;
import java.nio.ShortBuffer;
import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;

import android.opengl.GLES11;
import android.util.Log;

import com.intel.camera.toolkit.depth.sceneperception.SPCore;
import com.intel.camera.toolkit.depth.sceneperception.SPTypes.BlockMeshingUpdateInfo;
import com.intel.camera.toolkit.depth.sceneperception.SPTypes.MeshResolution;
import com.intel.camera.toolkit.depth.sceneperception.SPTypes.Status;
import com.intel.sample.depth.spsample.SPUtils.float3;

/**
 * Mesher handles updating mesh content and display of mesh of reconstruction result 
 * obtained from Scene Perception module.
 */
public class Mesher {
	private static final String TAG = "Mesher";
	private SPCore mSPCore = null;
	private MeshResolution mResolution = MeshResolution.SP_LOW_MESHRESOLUTION;
	private volatile boolean mIsShadingEnabled = false;
	public Mesher(SPCore spInstance) {
		mSPCore = spInstance;
		// start meshing with initial lowest thresholds to have all mesh data
		mSPCore.setMeshingUpdateThresholds(0.0f, 0.0f);
	}
	
	/**
	 * sets resolution of the mesh output. 
	 * @param meshResolution a resolution to set to.
	 */
	public void setMeshResolution(MeshResolution meshResolution) {
		mResolution = meshResolution;
	}
	
	/**
	 * enables shading by use of normal vectors.
	 * @param isEnabled if shading is to be enabled. 
	 */
	public void setEnabledShading(boolean isEnabled){
		mIsShadingEnabled = isEnabled;
	}
	
    private BlockMeshingUpdateInfo mBmui;    
    /** Each voxel in a 4^3 grid can have a maximum of 5 faces (marching cubes). */
    public final static int SP_BLOCK_FACES  = (4 * 4 * 4 * 5); 
    /** Maximum number of vertices is the number of edges in a 4^3 grid. */
    public final static int SP_BLOCK_VERTICES = (3 * 5 * 5 * 4); 
    /** theoretical max 128 x 128 x 128 = 2,097,152 we take just 128 x 128 (a full slice) / 8. */
    public final static int SP_MAXBLOCKMESH = 32768/8; 
    /** theoretical max value is the number of blocks x max faces per block (a quarter is taken). */
    public final static int SP_MAXFACES = (SP_MAXBLOCKMESH * SP_BLOCK_FACES / 4);
    /** theoretical max value is number of blocks x max vertices per block (a quarter is taken). */
    public final static int SP_MAXVERTICES = (SP_MAXBLOCKMESH * SP_BLOCK_VERTICES / 4);	    
    
    /** holds result of mesh update task. */
    private Future<Status> mMeshUpdateTask = null;
    /** execution service that runs mesh updating tasks. */
    private ExecutorService mMeshUpdateExService = Executors.newSingleThreadExecutor();
    private boolean mIsFirstMeshingCall = true;
    
    /**
     * holds information for a block mesh such as number of vertices, faces and 
     * vertex starting index, face starting index with respect to the overall mesh.
     */
    private class DrawPiece {    	
    	public DrawPiece(){}
    	public DrawPiece(DrawPiece other){
    		mID = other.mID;
    		faceStartIdx = other.faceStartIdx;
    		vertStartIdx = other.vertStartIdx;
    		numFaces = other.numFaces;
    		numVertices = other.numVertices;
    	}
    	public int numVertices;
    	public int numFaces;
    	public int vertStartIdx;
    	public int faceStartIdx;
    	public int mID;
    }
    
    private final int MESH_SIZE_MUL_FAC = 4;
	private DrawPiece[] mBlocksBuffer;
	private FloatBuffer mVerticesBuffer;
	private ByteBuffer mColorsBuffer;
	private FloatBuffer mNormalsBuffer;
	private ShortBuffer mFacesBuffer;
	
    private boolean mIsMemAllocated = false;
    private synchronized boolean allocateMem(){
    	boolean canAllocateMem = false;
		try {
			mVerticesBuffer = ByteBuffer.allocateDirect(3 * MESH_SIZE_MUL_FAC * SP_MAXVERTICES * 4).order(ByteOrder.nativeOrder()).asFloatBuffer();
			mColorsBuffer =  ByteBuffer.allocateDirect(3 * MESH_SIZE_MUL_FAC * SP_MAXVERTICES).order(ByteOrder.nativeOrder());
			mNormalsBuffer = ByteBuffer.allocateDirect(3 * MESH_SIZE_MUL_FAC * SP_MAXVERTICES * 4).order(ByteOrder.nativeOrder()).asFloatBuffer();
			mFacesBuffer = ByteBuffer.allocateDirect(3 * MESH_SIZE_MUL_FAC * SP_MAXFACES * 2).order(ByteOrder.nativeOrder()).asShortBuffer();
			mCurFacesNormals = new float[3 * SP_BLOCK_FACES];
			FACE_NORMAL_ZERO = new float[3 * SP_BLOCK_FACES];
			mCurVerticesFaceCount = new byte[SP_BLOCK_VERTICES];
			VERTICE_FACE_COUNT = new byte[SP_BLOCK_VERTICES];
			mCurNormals = new float[3 * SP_MAXVERTICES];
			mBlocksBuffer = new DrawPiece[MESH_SIZE_MUL_FAC * SP_MAXBLOCKMESH];
			if (mCurFacesNormals != null && FACE_NORMAL_ZERO != null &&  mCurVerticesFaceCount != null &&
	    			VERTICE_FACE_COUNT != null && mCurNormals != null && mBlocksBuffer != null 
	    			&& mVerticesBuffer != null && mColorsBuffer != null && mNormalsBuffer != null && 
	    			mFacesBuffer != null
	    			) {	    		
					canAllocateMem = true;
    		}
		}
		catch (Exception e) {
			Log.e(TAG, "Error - fail to allocate memory for doing meshing");
		}
		catch (Error e) {
			Log.e(TAG, "Error - fail to allocate memory for doing meshing");
		}
    	return canAllocateMem;
    }

    /**
     * resets meshing to restart the update with an initial full mesh result.
     */
    public synchronized void reset(){
    	if (mMeshUpdateTask != null) {
    		try {
				mMeshUpdateTask.get(); //wait for the current meshing task to complete
			}
			catch (Exception e) {Log.e(TAG, "ERROR - can not update mesh");}
    	}   
    	mIsFirstMeshingCall = true;
    	mSPCore.setMeshingUpdateThresholds(0.0f, 0.0f);
    	mBlocksSize = 0;
    	mVerticesSize = 0;
    	mFacesSize = 0;
    }
    
    /**
     * schedules the next update on mesh result, i.e. on top of all meshes
     * obtained so far. The update is confined to scene portion within current view of 
     * the camera.    
     * @param bmui contains information on constraints of maximum numbers such as 
     * number of block meshes, number of vertices and number of faces or on other 
     * bounding constraints such as the box that meshing will be bound to.
     * Such constraints apply to the current mesh update.
     * @param useColor indicates whether to enable color meshing.
     */
    public void updateMesh(final BlockMeshingUpdateInfo bmui, final boolean useColor){
    	mBmui = bmui;
    	if (mMeshUpdateTask == null ||
    		(mMeshUpdateTask != null && mMeshUpdateTask.isDone())) {
    		if (mSPCore.isReconstructionUpdated()) {
    			mMeshUpdateTask = mMeshUpdateExService.submit(new MeshUpdateCallable(mBmui, useColor));
    		}
    	}
    }

    /** 
     * schedules the next update on mesh result, i.e. on top of all  
     * meshes obtained so far. The update is confined to scene portion within  
     * current view of the camera.    
     * Constraints on maximum numbers for the mesh update such as number of block 
     * meshes, number of vertices and number of faces, or bounding box constraints 
     * will be reset to the maximum values defined by the class (e.g. SP_BLOCK_FACES, 
     * SP_BLOCK_VERTICES).
     * @param useColor indicates whether to enable color meshing.
    */    
    public void updateMesh(final boolean useColor){
    	if (!mIsMemAllocated) {
    		if (allocateMem()) {
    			mIsMemAllocated = true;
		    	if (mBmui == null) {
		    		try {
		    			mBmui = new BlockMeshingUpdateInfo(SP_MAXBLOCKMESH, SP_MAXVERTICES, 
						SP_MAXFACES, true, 0, mResolution, 0, 
						new float[] {-0.1f, -0.1f, 0.75f, 0.1f, 0.1f, 1.5f});
		    		}
		    		catch (Exception e) {
		    			Log.e(TAG, "Error - fail to allocate memory for doing meshing");
		    		}
		    		catch (Error e) {
		    			Log.e(TAG, "Error - fail to allocate memory for doing meshing");
		    		}
		    	}
    		}
    	}
    	if (mIsMemAllocated) {	
    		updateMesh(mBmui, useColor);
    	}
    	else {
    		Log.e(TAG, "Error - fail to allocate memory for doing meshing");
    	}
    }
    
    /**
     * Mesh update task that includes getting an updated mesh.
     */
	private class MeshUpdateCallable implements Callable<Status> {
		private BlockMeshingUpdateInfo mBmui;
		private boolean mUseColor;
		
		public MeshUpdateCallable(BlockMeshingUpdateInfo bmui, boolean useColor){
			mBmui = bmui;
			mUseColor = useColor;
		}

		@Override
		public Status call() {
			// call meshing. the first time the thresholds are 0
			Status meshingStatus = Status.SP_STATUS_WARNING;
	    	while (meshingStatus == Status.SP_STATUS_WARNING) {
			//reset to the initial maximal values
	    	mBmui.resetValues();

	    	mBmui.setEnabledColor(mUseColor);
	    	
	    		meshingStatus = mSPCore.updateMesh(mBmui);

	    		if (meshingStatus == Status.SP_STATUS_ERROR) {
				Log.e(TAG, "Error - meshing incurs an error - " + meshingStatus);
					break;
	    	}			
			for (int i = 0; i < mBmui.mNumBlockMeshes; i++)
			{
				if ((mBmui.getBMNumVertices(i) > 0) && (mBmui.getBMNumFaces(i) > 0))
				{
					// re-compute face indices relative to vertex buffer
					for (int idx = 0; idx < mBmui.getBMNumFaces(i) * 3; idx++)
					{
						mBmui.mFaces.put(mBmui.getBMFaceStartIndex(i) + idx, 
								mBmui.mFaces.get(mBmui.getBMFaceStartIndex(i) + idx) - mBmui.getBMVertexStartIndex(i) / 4);
					}

					// update normal buffer
    				if (mIsShadingEnabled) {
						computeVerticeNormal(mCurNormals, 3 * mBmui.getBMVertexStartIndex(i) / 4, 
								mBmui.mVertices, mBmui.getBMVertexStartIndex(i),mBmui.getBMNumVertices(i), 
								mBmui.mFaces, mBmui.getBMFaceStartIndex(i), mBmui.getBMNumFaces(i));
    				}
				}
			}			
			createMesh();
			}
	    	//setting thresholds for mesh updating once, after first full update 
	    	if (mIsFirstMeshingCall) {
	    		mSPCore.setMeshingUpdateThresholds(0.03f, 0.005f);
	    		mIsFirstMeshingCall = false;
	    	}			
			return meshingStatus;					
		}
	};
      
    private volatile boolean mStateMeshColor = true;
    
	static final float[] ambient = new float[]{ 0.65f, 0.65f, 0.65f, 1.0f };
	static final float[] DIFFUSE = new float[]{ 0.6f, 0.6f, 0.6f, 0.0f };
	static final float[] SPECULAR = new float[]{ 1.0f, 1.0f, 1.0f, 1.0f };
	static final float[] SPEC_REF = new float[]{ 1.0f, 1.0f, 1.0f, 1.0f };
	static final float[] SHINE_REF = new float[]{ 0.5f, 0.5f, 0.5f, 1.0f };
	
	/**
	 * draws mesh content and uses current camera location for lighting source.
	 * @param posX x-coordinate of the camera pose.
	 * @param posY y-coordinate of the camera pose.
	 * @param posZ z-coordinate of the camera pose.
	 */
	public synchronized void draw(float posX, float posY, float posZ) {
		GLES11.glDisable(GLES11.GL_CULL_FACE);
		float	lightPos[] = { posX, posY, posZ, 1.0f };
    	if (!mStateMeshColor)
    	{
    		ambient[0] = ambient[1] = ambient[2] = 0.2f;
    		DIFFUSE[0] = DIFFUSE[1] = DIFFUSE[2] = 1.0f;
    	}
    	GLES11.glEnable(GLES11.GL_LIGHTING);    	
		GLES11.glLightModelf(GLES11.GL_LIGHT_MODEL_TWO_SIDE, 1.0f);
    	GLES11.glEnable(GLES11.GL_LIGHT0);
    	GLES11.glLightfv(GLES11.GL_LIGHT0, GLES11.GL_AMBIENT, ambient, 0);
    	GLES11.glLightfv(GLES11.GL_LIGHT0, GLES11.GL_DIFFUSE, DIFFUSE, 0);
    	GLES11.glLightfv(GLES11.GL_LIGHT0, GLES11.GL_SPECULAR, SPECULAR, 0);
    	GLES11.glLightfv(GLES11.GL_LIGHT0, GLES11.GL_POSITION, lightPos, 0);

    	GLES11.glColor4f(0.7f, 0.7f, 0.7f, 1.0f);

    	if (mStateMeshColor)
    	{
    		GLES11.glEnable(GLES11.GL_COLOR_MATERIAL);
    	}
    	else
    	{
    		GLES11.glDisable(GLES11.GL_COLOR_MATERIAL);
    	}
    	
    	GLES11.glMaterialfv(GLES11.GL_FRONT, GLES11.GL_SPECULAR, SPEC_REF,0);
    	GLES11.glMaterialfv(GLES11.GL_FRONT, GLES11.GL_SHININESS, SHINE_REF, 0);   	
    	GLES11.glShadeModel(GLES11.GL_SMOOTH);
		
    	// vertex buffer
    	int[] vBO = new int[1];
    	GLES11.glGenBuffers(1, vBO, 0);
    	GLES11.glBindBuffer(GLES11.GL_ARRAY_BUFFER, vBO[0]); 	
    	GLES11.glBufferData(GLES11.GL_ARRAY_BUFFER, 4 * mVerticesSize * 4, mVerticesBuffer, GLES11.GL_STATIC_DRAW);  	
    	GLES11.glVertexPointer(3, GLES11.GL_FLOAT, 16, 0);    	

    	// normal buffer
    	int[] nBO = new int[1];
    	if (mIsShadingEnabled) {  
	    	GLES11.glEnableClientState(GLES11.GL_NORMAL_ARRAY);
	    	GLES11.glGenBuffers(1, nBO, 0);
	    	GLES11.glBindBuffer(GLES11.GL_ARRAY_BUFFER, nBO[0]);
	    	GLES11.glBufferData(GLES11.GL_ARRAY_BUFFER, 3 * mVerticesSize * 4, mNormalsBuffer, GLES11.GL_STATIC_DRAW);
	    	GLES11.glNormalPointer(GLES11.GL_FLOAT, 0, 0);
    	}
    	
    	// color buffer
    	int[] cBO = new int[1];
    	if (mStateMeshColor)
    	{
        	GLES11.glEnableClientState(GLES11.GL_COLOR_ARRAY);        	
    		GLES11.glGenBuffers(1, cBO, 0);
    		GLES11.glBindBuffer(GLES11.GL_ARRAY_BUFFER, cBO[0]);
    		GLES11.glBufferData(GLES11.GL_ARRAY_BUFFER, mVerticesSize * 3, mColorsBuffer, GLES11.GL_STATIC_DRAW);	
    		GLES11.glColorPointer(3, GLES11.GL_UNSIGNED_BYTE, 0, 0);
    	}

    	// face buffer 
    	int[] fBO = new int[1];    	
    	GLES11.glGenBuffers(1, fBO, 0);
    	GLES11.glBindBuffer(GLES11.GL_ELEMENT_ARRAY_BUFFER, fBO[0]);
    	GLES11.glBufferData(GLES11.GL_ELEMENT_ARRAY_BUFFER, 2 * mFacesSize * 3, mFacesBuffer, GLES11.GL_STATIC_DRAW);   	
    	GLES11.glDrawElements(GLES11.GL_TRIANGLES, mFacesSize * 3, GLES11.GL_UNSIGNED_SHORT, 0);  	
    	GLES11.glBindBuffer(GLES11.GL_ELEMENT_ARRAY_BUFFER, 0);
    	GLES11.glBindBuffer(GLES11.GL_ARRAY_BUFFER, 0);
    	
    	// disable features used
    	if (mIsShadingEnabled) {
	    	GLES11.glDisableClientState(GLES11.GL_NORMAL_ARRAY);
	    	GLES11.glDeleteBuffers(1, nBO, 0);
    	}
    	GLES11.glDisableClientState(GLES11.GL_COLOR_ARRAY);
    	GLES11.glDisable(GLES11.GL_LIGHTING);
    	
    	// delete buffers
    	GLES11.glDeleteBuffers(1, vBO, 0);
    	GLES11.glDeleteBuffers(1, cBO, 0);
    	GLES11.glDeleteBuffers(1, fBO, 0);
    }    
	
    private static void computeFaceNormal(FloatBuffer pVertices, int pVerticesStartIdx, 
    		IntBuffer pFaces, int pFacesStartIdx, float[] pFacesNormals, int iFaceIndex) {
    	// Uses p2 as a new origin for p1,p3
    	final int i1 = pFaces.get(pFacesStartIdx + 3 * iFaceIndex);
    	final int i2 = pFaces.get(pFacesStartIdx + 3 * iFaceIndex + 1); 
    	final int i3 = pFaces.get(pFacesStartIdx + 3 * iFaceIndex + 2);
    	final float3 p1 = new float3(
    			pVertices.get(pVerticesStartIdx + 4 * i1), 
    			pVertices.get(pVerticesStartIdx + 4 * i1 + 1), 
    			pVertices.get(pVerticesStartIdx + 4 * i1 + 2));
    	final float3 p2 = new float3(
    			pVertices.get(pVerticesStartIdx + 4 * i2), 
    			pVertices.get(pVerticesStartIdx + 4 * i2 + 1), 
    			pVertices.get(pVerticesStartIdx + 4 * i2 + 2));
    	final float3 p3 = new float3(
    			pVertices.get(pVerticesStartIdx +4 * i3), 
    			pVertices.get(pVerticesStartIdx + 4 * i3 + 1), 
    			pVertices.get(pVerticesStartIdx + 4 * i3 + 2));
    	final float3 a = p3.sub(p2);
    	final float3 b = p1.sub(p2);
    	float3 pn = a.cross(b).mul(-1.0f);
    	pn = pn.normalize();
    	pFacesNormals[3 * iFaceIndex] = pn.x;
    	pFacesNormals[3 * iFaceIndex + 1] = pn.y;
    	pFacesNormals[3 * iFaceIndex + 2] = pn.z;
    }


    private float[] mCurFacesNormals;
    private float[] FACE_NORMAL_ZERO;
    private byte[] mCurVerticesFaceCount;
    private byte[] VERTICE_FACE_COUNT;
    private float[] mCurNormals;
    
    private void computeVerticeNormal(float[] pNormals, int pNormalsStartIdx,
    		FloatBuffer pVertices, int pVerticesStartIdx, int iNumVertices, 
    		IntBuffer pFaces, int pFacesStartIdx, int iNumFaces)
    {
    	System.arraycopy(FACE_NORMAL_ZERO, 0, pNormals, pNormalsStartIdx, 3 * iNumVertices);
    	System.arraycopy(VERTICE_FACE_COUNT, 0, mCurVerticesFaceCount, 0, iNumVertices);
    	for (int fi = 0; fi < iNumFaces; fi++)
    	{
    		computeFaceNormal(pVertices, pVerticesStartIdx, pFaces, pFacesStartIdx, mCurFacesNormals, fi);
    	}

    	for (int fi = 0; fi < iNumFaces; fi++)
    	{
    		int i = pFaces.get(pFacesStartIdx + 3 * fi);
    		pNormals[pNormalsStartIdx + 3 * i] 		+= mCurFacesNormals[3 * fi];
    		pNormals[pNormalsStartIdx + 3 * i + 1] 	+= mCurFacesNormals[3 * fi + 1];
    		pNormals[pNormalsStartIdx + 3 * i + 2] 	+= mCurFacesNormals[3 * fi + 2];
    		mCurVerticesFaceCount[i] ++;

    		i = pFaces.get(pFacesStartIdx + 3 * fi + 1);
    		pNormals[pNormalsStartIdx + 3 * i] 		+= mCurFacesNormals[3 * fi];
    		pNormals[pNormalsStartIdx + 3 * i + 1] 	+= mCurFacesNormals[3 * fi + 1];
    		pNormals[pNormalsStartIdx + 3 * i + 2] 	+= mCurFacesNormals[3 * fi + 2];
    		mCurVerticesFaceCount[i] ++;

    		i = pFaces.get(pFacesStartIdx + 3 * fi + 2);
    		pNormals[pNormalsStartIdx + 3 * i] 		+= mCurFacesNormals[3 * fi];
    		pNormals[pNormalsStartIdx + 3 * i + 1] 	+= mCurFacesNormals[3 * fi + 1];
    		pNormals[pNormalsStartIdx + 3 * i + 2] 	+= mCurFacesNormals[3 * fi + 2];
    		mCurVerticesFaceCount[i] ++;
    	}

    	for (int vi = 0; vi < iNumVertices; vi++)
    	{
    		float3 pn = new float3(pNormals[pNormalsStartIdx + 3 * vi], 
    				pNormals[pNormalsStartIdx + 3 * vi + 1], 
    				pNormals[pNormalsStartIdx + 3 * vi + 2]);
    		if (mCurVerticesFaceCount[vi] > 0)
    		{
    			pn = pn.div(mCurVerticesFaceCount[vi]);
    		}
    		pn = pn.normalize();
    		pNormals[pNormalsStartIdx + 3 * vi] = pn.x;
    		pNormals[pNormalsStartIdx + 3 * vi + 1] = pn.y;
    		pNormals[pNormalsStartIdx + 3 * vi + 2] = pn.z;
    	}
    }

    private int mBlocksSize = 0;
	private int mVerticesSize = 0;
	private int mFacesSize = 0;
    
    public synchronized void createMesh()
    {
    	// build set of new meshes ids
    	Set<Integer> newBlockMeshIds = new HashSet<Integer>();
    	for (int i = 0; i < mBmui.mNumBlockMeshes; i++)
    	{
    		newBlockMeshIds.add(mBmui.getBMId(i));
    	}

    	// do packing for the arrays
    	int bs = 0, vs = 0, cs = 0, fs = 0;
    	for (int i = 0; i < mBlocksSize; i++)
    	{
    		// find it in the new set
    		if (newBlockMeshIds.contains(mBlocksBuffer[i].mID))
    		{
    			// if found then to be deleted
    			continue;
    		}

    		if (bs != i)
    		{
    			// copy block record i -> bs
    			mBlocksBuffer[bs] = new DrawPiece(mBlocksBuffer[i]);

    			// copy vertices, colors, and normals
    			for (int v = 0; v < 4 * mBlocksBuffer[bs].numVertices; v++)
    			{
    				mVerticesBuffer.put(vs + v, mVerticesBuffer.get(4 * mBlocksBuffer[bs].vertStartIdx + v));    				
    			}
    			for (int v = 0; v < 3 * mBlocksBuffer[bs].numVertices; v++)
    			{
    				mColorsBuffer.put(cs + v, mColorsBuffer.get(3 * mBlocksBuffer[bs].vertStartIdx + v));
    				if (mIsShadingEnabled) {
    					mNormalsBuffer.put(cs + v, mNormalsBuffer.get(3 * mBlocksBuffer[bs].vertStartIdx + v));
    				}
    			}

    			// copy faces
    			int faceIdx = 0;
    			for (int f = 0; f < (3 * mBlocksBuffer[bs].numFaces); f++)
    			{
    				faceIdx = mFacesBuffer.get(3 * mBlocksBuffer[bs].faceStartIdx + f) - 
    						mBlocksBuffer[bs].vertStartIdx + vs / 4;
    				if (faceIdx > 32767) {//max of Unsigned short type
    					faceIdx = 32767; //clipping
    				}
    				mFacesBuffer.put(fs + f, (short)faceIdx);
    			}

    			// update numbers
    			mBlocksBuffer[bs].vertStartIdx = vs / 4;
    			mBlocksBuffer[bs].faceStartIdx = fs / 3;
    		}
    		vs += 4 * mBlocksBuffer[bs].numVertices;
    		cs += 3 * mBlocksBuffer[bs].numVertices;
    		fs += 3 * mBlocksBuffer[bs].numFaces;
    		bs += 1;
    	}
    	
    	// add new data
    	for (int i = 0; i < mBmui.mNumBlockMeshes; i++)
    	{
    		// create new buffers
    		if ((mBmui.getBMNumVertices(i) > 0) && (mBmui.getBMNumFaces(i) > 0) && (bs < 4 * SP_MAXBLOCKMESH))
    		{
    			// create new GL list
    			int tbufIdx = mBmui.getBMFaceStartIndex(i); //mBmui.mFaces
    			int vbufIdx = mBmui.getBMVertexStartIndex(i); //mBmui.mVertices
    			int nbufIdx = 3 * mBmui.getBMVertexStartIndex(i) / 4; //mNormals
    			int cbufIdx = 3 * mBmui.getBMVertexStartIndex(i) / 4; //mColors
    			for (int v = 0; v < 4 * mBmui.getBMNumVertices(i); v++)
    			{
    				mVerticesBuffer.put(vs + v, mBmui.mVertices.get(vbufIdx + v));
    			}
    			for (int v = 0; v < 3 * mBmui.getBMNumVertices(i); v++)
    			{
    				if (mBmui.mColors != null && mStateMeshColor)
    				{
    					mColorsBuffer.put(cs + v, mBmui.mColors.get(cbufIdx + v));
    				}
    				else
    				{
    					mColorsBuffer.put(cs + v, (byte)100);
    				}
    				if (mIsShadingEnabled) {
    					mNormalsBuffer.put(cs + v, mCurNormals[nbufIdx + v]);
    				}
    			}
    			for (int f = 0; f < 3 * mBmui.getBMNumFaces(i); f++)
    			{
    				int faceIdx = mBmui.mFaces.get(tbufIdx + f) + vs / 4;
    				if (faceIdx > 32767) {//max of Unsigned short type
    					faceIdx = 32767; //clipping
    				}    				
    				mFacesBuffer.put(fs + f, (short)(faceIdx));
    			}

    			// update numbers
    			if (mBlocksBuffer[bs] == null) {
    				mBlocksBuffer[bs] = new DrawPiece();
    			}
    			mBlocksBuffer[bs].mID = mBmui.getBMId(i);
    			mBlocksBuffer[bs].numVertices = mBmui.getBMNumVertices(i);
    			mBlocksBuffer[bs].numFaces = mBmui.getBMNumFaces(i);
    			mBlocksBuffer[bs].vertStartIdx = vs / 4;
    			mBlocksBuffer[bs].faceStartIdx = fs / 3;
    			vs += 4 * mBlocksBuffer[bs].numVertices;
    			cs += 3 * mBlocksBuffer[bs].numVertices;
    			fs += 3 * mBlocksBuffer[bs].numFaces;
    			bs += 1;
    		}
    	}
    	// set values for new sizes
    	mBlocksSize = bs;
    	mVerticesSize = vs / 4;
    	mFacesSize = fs / 3;
    }

	// shut down thread executing mesh update and release resources.
    public void release(){
    	if (mMeshUpdateTask != null) {
    		try {
				mMeshUpdateTask.get(); //wait for the current meshing task to complete
			} catch (Exception e) {Log.e(TAG, "ERROR - can not finish updating mesh upon release");}
    	}
    	
    	if (mMeshUpdateExService != null) {
    		mMeshUpdateExService.shutdown();
    		try {
    			mMeshUpdateExService.awaitTermination(120, TimeUnit.MILLISECONDS);
    		} 
			catch (Exception e) {Log.e(TAG, "ERROR - awaiting termination of mesh update service failed");}
    	}
    	if (mBmui != null) {
    		mBmui = null;
    	}
    }


}
