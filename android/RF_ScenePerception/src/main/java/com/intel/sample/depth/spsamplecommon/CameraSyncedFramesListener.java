/********************************************************************************

 INTEL CORPORATION PROPRIETARY INFORMATION This software is supplied under the
 terms of a license agreement or nondisclosure agreement with Intel Corporation
 and may not be copied or disclosed except in accordance with the terms of that
 agreement.
 Copyright(c) 2011-2015 Intel Corporation. All Rights Reserved.

 *********************************************************************************/

package com.intel.sample.depth.spsamplecommon;

import com.intel.camera.toolkit.depth.sceneperception.SPTypes;

/**
 * CameraSyncedFramesListener provides an interface for input processing tasks.
 * Input processing tasks are to receive an input stream when it becomes
 * available from the camera and IMU sensors.
 */
public interface CameraSyncedFramesListener {
    /**
     * processes input stream.
     * @param input contains depth frame and color frame data, optionally includes
     * IMU samples and their associated time stamps.
     */
    public void onSyncedFramesAvailable(SPTypes.SPInputStream input);
}
