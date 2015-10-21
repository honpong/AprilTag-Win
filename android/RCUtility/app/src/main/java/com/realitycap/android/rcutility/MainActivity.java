package com.realitycap.android.rcutility;

import android.app.Activity;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.TextView;
import android.widget.ToggleButton;

import com.intel.camera.toolkit.depth.Camera;

public class MainActivity extends TrackerActivity
{
    private static final String TAG = MainActivity.class.getSimpleName();

    private TextView statusText;
    private ToggleButton calibrationButton;
    private ToggleButton captureButton;

    enum AppState
    {
        Idle, Calibrating, Capturing
    }

    AppState appState = AppState.Idle;

    SharedPreferences prefs;

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        statusText = (TextView) this.findViewById(R.id.statusText);

        calibrationButton = (ToggleButton) this.findViewById(R.id.calibrationButton);
        calibrationButton.setOnCheckedChangeListener(new OnCheckedChangeListener()
        {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked)
            {
                if (isChecked)
                {
                    if (!enterCalibrationState()) calibrationButton.setChecked(false);
//                    mWorkerThread.postTask(startCalibrationTask);
                }
                else
                {
                    abortCalibration();
                }
            }
        });

        captureButton = (ToggleButton) this.findViewById(R.id.captureButton);
        captureButton.setOnCheckedChangeListener(new OnCheckedChangeListener()
        {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked)
            {
                if (isChecked)
                {
                    if (!enterCaptureState()) captureButton.setChecked(false);
                }
                else
                {
                    exitCaptureState();
                }
            }
        });

        Button liveButton = (Button) this.findViewById(R.id.liveButton);
        liveButton.setOnClickListener(new OnClickListener()
        {
            @Override
            public void onClick(View v)
            {
                if (appState == AppState.Idle) startLiveActivity();
            }
        });

        Button replayButton = (Button) this.findViewById(R.id.replayButton);
        replayButton.setOnClickListener(new OnClickListener()
        {
            @Override
            public void onClick(View v)
            {
                if (appState == AppState.Idle) openFilePicker();
            }
        });

        prefs = getSharedPreferences("RCUtilityPrefs", 0);

        setStatusText("Ready");
    }

    @Override protected void onResume()
    {
        super.onResume();

        if (!hasSavedIntrinsics()) getCameraIntrinsics();
    }

    protected void setStatusText(final String text)
    {
        this.runOnUiThread(new Runnable()
        {
            @Override public void run()
            {
                statusText.setText(text);
                Log.d(TAG, text);
            }
        });
    }

    protected boolean enterCalibrationState()
    {
        if (appState != AppState.Idle) return false;
        setStatusText("Starting calibration...");

        trackerProxy.createTracker();

        if (!imuMan.startSensors())
        {
            return abortTracking("Failed to start IMU sensors.");
        }

        if (!configureCamera())
        {
            return abortTracking("Failed to configure camera.");
        }

        if (!setDefaultCalibrationFromFile(DEFAULT_CALIBRATION_FILENAME))
        {
            return abortTracking("Failed to load default calibration file.");
        }

        if (!trackerProxy.startCalibration())
        {
            return abortTracking("Failed to start calibration.");
        }

        appState = AppState.Calibrating;
        setStatusText("Calibrating");
        return true;
    }

    protected void abortCalibration()
    {
        if (appState != AppState.Calibrating) return;

        imuMan.stopSensors();
        trackerProxy.stopTracker();
        trackerProxy.destroyTracker();

        setStatusText("Calibration stopped.");

        calibrationButton.setChecked(false);
        appState = AppState.Idle;
    }

    protected void exitCalibrationState()
    {
        if (appState != AppState.Calibrating) return;

        imuMan.stopSensors();

        boolean success;
        String cal = trackerProxy.getCalibration();
        if (cal != null)
        {
            prefs.edit().putString(PREF_KEY_CALIBRATION, cal).apply();
            success = textFileIO.writeTextToFileOnSDCard(cal, "calibration.json");
        }
        else success = false;

        trackerProxy.stopTracker();
        trackerProxy.destroyTracker();

        if (success) setStatusText("Calibration successful.");
        else setStatusText("Calibration not saved.");

        calibrationButton.setChecked(false);
        appState = AppState.Idle;
    }

    protected boolean enterCaptureState()
    {
        if (appState != AppState.Idle) return false;

        setStatusText("Starting capture...");

        trackerProxy.createTracker();
        trackerProxy.setOutputLog(Environment.getExternalStorageDirectory() + "/capture");

        if (!startSensors())
        {
            return abortTracking("Failed to start sensors.");
        }

        setStatusText("Capturing...");
        appState = AppState.Capturing;
        return true;
    }

    protected void exitCaptureState()
    {
        if (appState != AppState.Capturing) return;
        setStatusText("Stopping capture...");
        stopSensors();
        trackerProxy.destroyTracker();
        setStatusText("Capture stopped.");
        appState = AppState.Idle;
    }

    protected void startLiveActivity()
    {
        if (appState != AppState.Idle) return;
        Intent intent = new Intent(this, VisActivity.class);
        intent.setAction(VisActivity.ACTION_LIVE_VIS);
        startActivity(intent);
    }

    protected void startReplayActivity(Uri captureFile)
    {
        Intent intent = new Intent(this, VisActivity.class);
        intent.setAction(VisActivity.ACTION_REPLAY_VIS);
        intent.setData(captureFile);
        startActivity(intent);
    }

    private boolean abortTracking(String message)
    {
        Log.d(TAG, "abortTracking");
        stopSensors();
        trackerProxy.destroyTracker();
        setStatusText(message);
        return false;
    }

    protected void getCameraIntrinsics()
    {
        setStatusText("Fetching camera intrinsics...");
        rsMan.startCameras(callback);
    }

    protected MyCameraIntrinsicsCallback callback = new MyCameraIntrinsicsCallback();

    class MyCameraIntrinsicsCallback implements ICameraIntrinsicsCallback
    {
        @Override public void cameraIntrinsicsObtained(Camera.Calibration.Intrinsics intr)
        {
            rsMan.stopCameras();

            SharedPreferences prefs = getSharedPreferences(MyApplication.SHARED_PREFS, MODE_PRIVATE);
            prefs.edit().
                    putFloat(PREF_PRINCIPAL_POINT_X, intr.principalPoint.x).
                    putFloat(PREF_PRINCIPAL_POINT_Y, intr.principalPoint.y).
                    putFloat(PREF_FOCAL_LENGTH_X, intr.focalLength.x).
                    putFloat(PREF_FOCAL_LENGTH_Y, intr.focalLength.y).
                    commit();

            setStatusText("Ready");
        }
    }

    @Override public void onStatusUpdated(final int runState, final int errorCode, final int confidence, final float progress)
    {
        Handler mainHandler = new Handler(getMainLooper());
        Runnable myRunnable = new Runnable()
        {
            @Override
            public void run()
            {
                if (appState == AppState.Calibrating)
                {
                    if (errorCode > 1)
                    {
                        exitCalibrationState();
                        setStatusText("Tracker error code: " + errorCode);
                        return;
                    }

                    int percentage = Math.round(progress * 100);

                    switch (runState)
                    {
                        case 0: // idle
                            exitCalibrationState();
                            setStatusText("Calibration successful.");
                            break;
                        case 1: // static calibration
                            if (progress <= 1.) setStatusText("Place device on a flat surface. Progress: " + percentage + "%");
                            break;
                        case 5: // portrait calibration
                            if (progress <= 1.) setStatusText("Hold steady in portrait orientation. Progress: " + percentage + "%");
                            break;
                        case 6: // landscape calibration
                            if (progress <= 1.) setStatusText("Hold steady in landscape orientation. Progress: " + percentage + "%");
                            break;
                        default:
                            break;
                    }
                }
                else if (appState == AppState.Capturing)
                {
                    if (errorCode > 1)
                    {
                        exitCaptureState();
                        setStatusText("Tracker error code: " + errorCode);
                    }
                }
            }
        };
        mainHandler.post(myRunnable); // runs the Runnable on the main thread
    }

    @Override public void onDataUpdated(final SensorFusionData data)
    {

    }

    // work in progress
    protected boolean openFilePicker()
    {
        if (appState != AppState.Idle) return false;

        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
        startActivityForResult(intent, ACTION_PICK_REPLAY_FILE);

        return true;
    }

    @Override protected void onActivityResult(int requestCode, int resultCode, Intent data)
    {
        if (resultCode == Activity.RESULT_OK && requestCode == ACTION_PICK_REPLAY_FILE)
        {
            startReplayActivity(data.getData());
        }
        else super.onActivityResult(requestCode, resultCode, data);
    }
}


