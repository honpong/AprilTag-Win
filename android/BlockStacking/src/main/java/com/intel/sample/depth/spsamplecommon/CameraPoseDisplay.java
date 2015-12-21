/********************************************************************************

 INTEL CORPORATION PROPRIETARY INFORMATION This software is supplied under the
 terms of a license agreement or nondisclosure agreement with Intel Corporation
 and may not be copied or disclosed except in accordance with the terms of that
 agreement.
 Copyright(c) 2011-2015 Intel Corporation. All Rights Reserved.

 *********************************************************************************/
package com.intel.sample.depth.spsamplecommon;


import android.opengl.GLES30;
import android.opengl.Matrix;

import com.intel.camera.toolkit.depth.Point3DF;
import com.intel.camera.toolkit.depth.sceneperception.SPTypes;
import com.intel.sample.depth.spsamplecommon.SPUtils.GLShader;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

/**
 * displays a camera pose with a rectangle window and x, y, z axes to show
 * the orientation of the camera pose.
 *
 */
public class CameraPoseDisplay extends SPTypes.CameraPose {
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

    private GLShader mShader = new GLShader();
    private static int mProgramHandle;

    public CameraPoseDisplay(SPTypes.CameraPose camPose) {
        Matrix.transposeM(mValue, 0, camPose.get(), 0);
        // determine corners of the rectangle display of camera pose
        for (int i = 0; i < 4; i++)
        {
            float ptx = (((i & 1) ^ (i >> 1)) != 0) ? -0.08f : 0.08f;
            float pty = ((i & 2) != 0) ? -0.06f : 0.06f;
            CAMERA_FRAME_CORNERS[i*3] = ptx;
            CAMERA_FRAME_CORNERS[i*3 + 1] = pty;
            CAMERA_FRAME_CORNERS[i*3 + 2] = 0.0f;
        }

        mShader.initialize(VERTEX_SHADER_CODE, FRAGMENT_SHADER_CODE);
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
    public void set(SPTypes.CameraPose newPose){
        Matrix.transposeM(mValue, 0, newPose.get(),0);
        mProjectedVertices.put(0, mValue[12]);
        mProjectedVertices.put(1, mValue[13]);
        mProjectedVertices.put(2, mValue[14]);
        mProjectedVertices.put(3, 1.0f);

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
    public void draw(float[] mvMatrix, float[] mvpMatrix) {
        mShader.useShader();
        mProgramHandle = mShader.getProgramHandle();
        mProjectedVertices.position(0);
        mCamPoseIndices.position(0);

        mShader.setMV(mvMatrix);
        mShader.setMVP(mvpMatrix);

        int positionHandle = GLES30.glGetAttribLocation(mProgramHandle, "aPosition");
        GLES30.glVertexAttribPointer(positionHandle, 3, GLES30.GL_FLOAT, false,
                0, mProjectedVertices);
        GLES30.glEnableVertexAttribArray(positionHandle);

        int colorHandle = GLES30.glGetAttribLocation(mProgramHandle, "aColor");
        GLES30.glVertexAttrib4f(colorHandle, 0.0f, 0.0f, 1.0f, 1.0f);
        GLES30.glDisableVertexAttribArray(colorHandle);

        GLES30.glDrawElements(GLES30.GL_POINTS, 1, GLES30.GL_UNSIGNED_BYTE, mCamPoseIndices);

        GLES30.glLineWidth(4.0f);
        //draw line to surface center
        GLES30.glVertexAttrib4f(colorHandle, 0.0f, 0.0f, 1.0f, 1.0f);
        GLES30.glDrawElements(GLES30.GL_LINES, 2, GLES30.GL_UNSIGNED_BYTE, mCamPoseIndices.position(0));

        GLES30.glLineWidth(6.0f);

        //draw x axis
        GLES30.glVertexAttrib4f(colorHandle, 1.0f, 0.0f, 0.0f, 1.0f);
        GLES30.glDrawElements(GLES30.GL_LINES, 2, GLES30.GL_UNSIGNED_BYTE, mCamPoseIndices.position(2));
        //draw y axis
        GLES30.glVertexAttrib4f(colorHandle, 0.0f, 1.0f, 0.0f, 1.0f);
        GLES30.glDrawElements(GLES30.GL_LINES, 2, GLES30.GL_UNSIGNED_BYTE, mCamPoseIndices.position(4));
        //draw z axis
        GLES30.glVertexAttrib4f(colorHandle, 0.0f, 0.0f, 1.0f, 1.0f);
        GLES30.glDrawElements(GLES30.GL_LINES, 2, GLES30.GL_UNSIGNED_BYTE, mCamPoseIndices.position(6));

        GLES30.glLineWidth(4.0f);
        //draw camera frames
        GLES30.glVertexAttrib4f(colorHandle, 0.3f, 0.3f, 0.3f, 1.0f);
        GLES30.glDrawElements(GLES30.GL_LINES, 8, GLES30.GL_UNSIGNED_BYTE, mCamPoseIndices.position(8));
    }

    private final String VERTEX_SHADER_CODE =

        // Input vertex attributes
        "attribute vec3 aPosition;\n"+
        "attribute vec4 aColor;\n"+

        // Matrices for the transformation of the vertices
        "uniform mat4 uMVPMatrix;\n"+
        "uniform mat4 uMVMatrix;\n"+

        // Output to fragment shader
        "varying vec3 vPosition;\n"+
        "varying vec4 vColor;\n"+

        // main shader program
        "void main()\n"+
        "{\n"+
        // modify the vertex position using the model-view matrix
        "   vPosition = vec3(uMVMatrix * vec4(aPosition, 1.0f));\n"+
        // color remains unchanged
        "   vColor = aColor;\n"+
        "   gl_Position = uMVPMatrix * vec4(aPosition, 1.0f);\n"+
        "}\n";

    private final String FRAGMENT_SHADER_CODE =
        "precision mediump float;\n"+
        // input position and color for the fragment from the vertex shader
        "varying vec3 vPosition;\n"+
        "varying vec4 vColor;\n"+

        // main shader program
        "void main()\n"+
        "{\n"+
        // no modifications to the color
        "   gl_FragColor = vColor;\n"+
        "}\n";
}

