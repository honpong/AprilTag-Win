package com.realitycap.android.rcutility;

import android.app.Activity;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.TextView;
import android.widget.ToggleButton;

public class MainActivity extends TrackerActivity
{
    private static final String TAG = MainActivity.class.getSimpleName();

    private TextView statusText;
    private ToggleButton calibrationButton;
    private ToggleButton captureButton;
    private Button liveButton;
    private Button replayButton;

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

        liveButton = (Button) this.findViewById(R.id.liveButton);
        liveButton.setOnClickListener(new OnClickListener()
        {
            @Override
            public void onClick(View v)
            {
                if (appState == AppState.Idle) startLiveActivity();
            }
        });

        replayButton = (Button) this.findViewById(R.id.replayButton);
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

    protected void setStatusText(String text)
    {
        statusText.setText(text);
        log(text);
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
            return abortTracking("Failed to get camera intrinsics.");
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

        boolean success = true;
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
                        return;
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

    @Override
    public boolean onCreateOptionsMenu(Menu menu)
    {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item)
    {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();
        if (id == R.id.action_settings)
        {
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    private void log(String line)
    {
        if (line != null) Log.d(TAG, line);
    }

    @SuppressWarnings("unused")
    private void logError(Exception e)
    {
        if (e != null) logError(e.getLocalizedMessage());
    }

    private void logError(String line)
    {
        if (line != null) Log.e(TAG, line);
    }
}


