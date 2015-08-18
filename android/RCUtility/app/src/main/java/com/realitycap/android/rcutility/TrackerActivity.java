package com.realitycap.android.rcutility;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.TextView;
import android.widget.ToggleButton;

import com.intel.camera.toolkit.depth.Camera;

/**
 * Created by benhirashima on 8/17/15.
 */
public abstract class TrackerActivity extends Activity implements ITrackerReceiver
{
    protected static final String PREF_KEY_CALIBRATION = "calibration";
    public static final String CALIBRATION_FILENAME = "calibration.json";
    public static final String DEFAULT_CALIBRATION_FILENAME = "ft210.json";
    protected static final int ACTION_PICK_REPLAY_FILE = 94131;

    protected IMUManager imuMan;
    protected TrackerProxy trackerProxy;
    protected RealSenseManager rsMan;
    protected R200Manager r200Man;
    protected TextFileIO textFileIO = new TextFileIO();

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        imuMan = new IMUManager();
        trackerProxy = new TrackerProxy();
        trackerProxy.receiver = this;

        imuMan.setSensorEventListener(trackerProxy);

        rsMan = new RealSenseManager(this, null);
        r200Man = new R200Manager(trackerProxy);
    }

    @Override protected void onDestroy()
    {
        trackerProxy.destroyTracker();
        super.onDestroy();
    }

    protected boolean startSensors()
    {
        if (!imuMan.startSensors()) return false;

        if (!r200Man.startCamera())
        {
            imuMan.stopSensors();
            return false;
        }

//        if(!rsMan.startImu())
//        {
//            imuMan.stopCameras();
//            return false;
//        }

        return true;
    }

    protected void stopSensors()
    {
        imuMan.stopSensors();
//        rsMan.stopImu();
//        rsMan.stopCameras();
        r200Man.stopCamera();
    }

    protected boolean setDefaultCalibrationFromFile(String fileName)
    {
        final String defaultCal = textFileIO.readTextFromFileInAssets(fileName);
        if (defaultCal == null || defaultCal.length() <= 0) return false;
        return trackerProxy.setCalibration(defaultCal);
    }

    protected boolean setCalibrationFromFile(String fileName)
    {
        final String cal = textFileIO.readTextFromFileOnSDCard(fileName);
        if (cal == null || cal.length() <= 0) return false;
        return trackerProxy.setCalibration(cal);
    }

    protected boolean configureCamera()
    {
        if (r200Man.isRunning()) return false;
        rsMan.startCameras();
        Camera.Calibration.Intrinsics intr = rsMan.getCameraIntrinsics();
        rsMan.stopCameras();
        if (intr == null) return false;
        trackerProxy.configureCamera(TrackerProxy.CAMERA_EGRAY8, 640, 480, intr.principalPoint.x, intr.principalPoint.y, intr.focalLength.x, intr.focalLength.y, 0, false, 0);
        return true;
    }
}
