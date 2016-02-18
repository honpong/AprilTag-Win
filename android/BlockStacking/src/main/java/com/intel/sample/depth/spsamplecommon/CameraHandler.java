/********************************************************************************

 INTEL CORPORATION PROPRIETARY INFORMATION This software is supplied under the
 terms of a license agreement or nondisclosure agreement with Intel Corporation
 and may not be copied or disclosed except in accordance with the terms of that
 agreement.
 Copyright(c) 2011-2015 Intel Corporation. All Rights Reserved.

 *********************************************************************************/

package com.intel.sample.depth.spsamplecommon;

import android.content.Context;
import android.util.Size;

import com.intel.camera.toolkit.depth.sceneperception.SPCore;

/**
 * CameraHandler handles camera and delivers camera configuration and frames.
 */
public abstract class CameraHandler {
    protected DepthProcessModule mDepthProcessor; //depth processor of camera frames
    protected Size mColorInputSize; //current input size for color stream
    protected Size mDepthInputSize; //current input size for depth stream
    protected SPCore mSPCore = null; //handle of SP module

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

