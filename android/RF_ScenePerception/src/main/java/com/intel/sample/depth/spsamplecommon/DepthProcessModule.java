/********************************************************************************

 INTEL CORPORATION PROPRIETARY INFORMATION This software is supplied under the
 terms of a license agreement or nondisclosure agreement with Intel Corporation
 and may not be copied or disclosed except in accordance with the terms of that
 agreement.
 Copyright(c) 2011-2015 Intel Corporation. All Rights Reserved.

 *********************************************************************************/

package com.intel.sample.depth.spsamplecommon;

import android.util.Size;

import com.intel.camera.toolkit.depth.Camera;

/**
 * DepthProcessModule provides an interface for high level process that handles 
 * depth and rgb frames coming from camera and IMU samples.
 * DepthProcessModule interface expects a configuration step for SP and UI module
 * , passing of inputs and setting status (for UI display).
 */
public interface DepthProcessModule {
    /**
     * configures SP module and UI module based on camera configuration parameters
     * such as input size, color and depth camera's intrinsics and extrinsic parameters
     * and resolution of SP.
     */
    public void configureDepthProcess(int spResolution, Size depthInputSize, Size colorInputSize,
                                      Camera.Calibration.Intrinsics colorParams,
                                      Camera.Calibration.Intrinsics depthParams,
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
