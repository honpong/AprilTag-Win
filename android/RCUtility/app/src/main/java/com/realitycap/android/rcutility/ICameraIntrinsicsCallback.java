package com.realitycap.android.rcutility;

import com.intel.camera.toolkit.depth.Camera;

/**
 * Created by bhirashi on 10/20/15.
 */
public interface ICameraIntrinsicsCallback
{
    void cameraIntrinsicsObtained(Camera.Calibration.Intrinsics intr);
}
