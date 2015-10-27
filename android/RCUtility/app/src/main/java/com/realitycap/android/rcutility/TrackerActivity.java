package com.realitycap.android.rcutility;

import android.app.Activity;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.TextView;
import android.widget.ToggleButton;

import com.intel.camera.toolkit.depth.Camera;
import com.intel.camera2.extensions.depthcamera.DepthCameraCalibrationDataMap;

/**
 * Created by benhirashima on 8/17/15.
 */
public abstract class TrackerActivity extends Activity implements ITrackerReceiver
{
    private static final String TAG = TrackerActivity.class.getSimpleName();
    protected static final String PREF_KEY_CALIBRATION = "calibration";
    public static final String CALIBRATION_FILENAME = "calibration.json";
    public static final String DEFAULT_CALIBRATION_FILENAME = "ft210.json";
    protected static final int ACTION_PICK_REPLAY_FILE = 94131;
    protected static final String PREF_PRINCIPAL_POINT_X = "principalPointX";
    protected static final String PREF_PRINCIPAL_POINT_Y = "principalPointY";
    protected static final String PREF_FOCAL_LENGTH_X = "focalLengthX";
    protected static final String PREF_FOCAL_LENGTH_Y = "focalLengthY";

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

//        if(!rsMan.startCameras())
//        {
//            imuMan.stopSensors();
//            r200Man.stopCamera();
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

    protected boolean configureCamera()
    {
        DepthCameraCalibrationDataMap.DepthCameraCalibrationData cal;
        try
        {
            cal = r200Man.getCalibrationData();
        }
        catch (Exception e)
        {
            Log.e(TAG, e.getLocalizedMessage());
            return false;
        }

        trackerProxy.configureCamera(
                cal.getColorCameraIntrinsics().getResolution().getWidth(),
                cal.getColorCameraIntrinsics().getResolution().getHeight(),
                cal.getColorCameraIntrinsics().getPrincipalPoint().x,
                cal.getColorCameraIntrinsics().getPrincipalPoint().y,
                cal.getColorCameraIntrinsics().getFocalLength().x,
                cal.getColorCameraIntrinsics().getFocalLength().y,
                cal.getDepthCameraIntrinsics().getResolution().getWidth(),
                cal.getDepthCameraIntrinsics().getResolution().getHeight(),
                cal.getDepthCameraIntrinsics().getPrincipalPoint().x,
                cal.getDepthCameraIntrinsics().getPrincipalPoint().y,
                cal.getDepthCameraIntrinsics().getFocalLength().x,
                cal.getDepthCameraIntrinsics().getFocalLength().y,
                cal.getDepthToColorExtrinsics().getTranslation()[0],
                cal.getDepthToColorExtrinsics().getTranslation()[1],
                cal.getDepthToColorExtrinsics().getTranslation()[2],
                0, false, 0);

        return true;
    }

    protected boolean hasSavedIntrinsics()
    {
        SharedPreferences prefs = getSharedPreferences(MyApplication.SHARED_PREFS, MODE_PRIVATE);
        return  prefs.contains(PREF_FOCAL_LENGTH_X) &&
                prefs.contains(PREF_FOCAL_LENGTH_Y) &&
                prefs.contains(PREF_PRINCIPAL_POINT_X) &&
                prefs.contains(PREF_PRINCIPAL_POINT_Y);
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
}
