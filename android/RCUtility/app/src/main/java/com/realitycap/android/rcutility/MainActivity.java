package com.realitycap.android.rcutility;

import android.app.Activity;
import android.content.SharedPreferences;
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

import com.intel.camera.toolkit.depth.Camera;

public class MainActivity extends Activity implements ITrackerReceiver
{
    private static final String PREF_KEY_CALIBRATION = "calibration";
    public static final String CALIBRATION_FILENAME = "calibration.json";
    public static final String DEFAULT_CALIBRATION_FILENAME = "ft210.json";

    private TextView statusText;
    private ToggleButton calibrationButton;
    private ToggleButton captureButton;
    private Button liveButton;
    private Button replayButton;

    IMUManager imuMan;
    TrackerProxy trackerProxy;
    RealSenseManager rsMan;

    enum AppState
    {
        Idle, Calibrating, Capturing, LiveVis, ReplayVis
    }

    AppState appState = AppState.Idle;

    SharedPreferences prefs;
    TextFileIO textFileIO = new TextFileIO();

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
                enterLiveState();
            }
        });

        replayButton = (Button) this.findViewById(R.id.replayButton);
        replayButton.setOnClickListener(new OnClickListener()
        {
            @Override
            public void onClick(View v)
            {
                openFilePicker();
            }
        });

        imuMan = new IMUManager();
        trackerProxy = new TrackerProxy();
        trackerProxy.receiver = this;

        imuMan.setSensorEventListener(trackerProxy);

        rsMan = new RealSenseManager(this, trackerProxy);

        prefs = getSharedPreferences("RCUtilityPrefs", 0);

        setStatusText("Ready");
    }

    @Override protected void onDestroy()
    {
        trackerProxy.destroyTracker();
        super.onDestroy();
    }

    protected void setStatusText(String text)
    {
        statusText.setText(text);
        log(text);
    }

    protected boolean startSensors()
    {
        if (!imuMan.startSensors()) return false;

        if (!rsMan.startCameras())
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
        rsMan.stopCameras();
    }

    protected boolean enterCalibrationState()
    {
        if (appState != AppState.Idle) return false;
        setStatusText("Starting calibration...");

        trackerProxy.createTracker();

        if (!startSensors())
        {
            return abortTracking("Failed to start sensors.");
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

        stopSensors();
        trackerProxy.stopTracker();
        trackerProxy.destroyTracker();

        setStatusText("Calibration stopped.");

        calibrationButton.setChecked(false);
        appState = AppState.Idle;
    }

    protected void exitCalibrationState()
    {
        if (appState != AppState.Calibrating) return;

        stopSensors();

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

        trackerProxy.createTracker();

        setStatusText("Starting capture...");
        if (!startSensors())
        {
            return abortTracking("Failed to start sensors.");
        }

        trackerProxy.setOutputLog(Environment.getExternalStorageDirectory() + "/capture");

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

    protected boolean enterLiveState()
    {
        if (appState != AppState.Idle) return false;
        setStatusText("Starting live view...");

        trackerProxy.createTracker();

        if (!startSensors())
        {
            return abortTracking("Failed to start sensors.");
        }

        if (!configureCamera())
        {
            return abortTracking("Failed to get camera intrinsics.");
        }

        if (!setCalibrationFromFile(CALIBRATION_FILENAME))
        {
            return abortTracking("Failed to load calibration file.");
        }

        if (!trackerProxy.startTracker())
        {
            return abortTracking("Failed to start tracking.");
        }

        setStatusText("Live view running...");
        appState = AppState.LiveVis;
        return true;
    }

    protected void exitLiveState()
    {
        if (appState != AppState.Idle) return;
        setStatusText("Stopping live view...");
        stopSensors();
        trackerProxy.destroyTracker();
        appState = AppState.Idle;
        setStatusText("Stopped.");
    }

    private boolean abortTracking(String message)
    {
        stopSensors();
        trackerProxy.destroyTracker();
        setStatusText(message);
        return false;
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
        Camera.Calibration.Intrinsics intr = rsMan.getCameraIntrinsics();
        if (intr == null)
        {
            stopSensors();
            trackerProxy.destroyTracker();
            return false;
        }
        trackerProxy.configureCamera(TrackerProxy.CAMERA_EGRAY8, 640, 480, intr.principalPoint.x, intr.principalPoint.y, intr.focalLength.x, intr.focalLength.y, 0, false, 0);
        return true;
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

    // work in progress
    @Override public void onDataUpdated(SensorFusionData data)
    {
        Handler mainHandler = new Handler(getMainLooper());
        Runnable myRunnable = new Runnable()
        {
            @Override
            public void run()
            {
                // do something on main thread
            }
        };
        mainHandler.post(myRunnable); // runs the Runnable on the main thread
    }

    // work in progress
    protected boolean openFilePicker()
    {
        if (appState != AppState.Idle) return false;
        setStatusText("openFilePicker");
        return true;
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
        if (line != null) Log.d(MyApplication.TAG, line);
    }

    @SuppressWarnings("unused")
    private void logError(Exception e)
    {
        if (e != null) logError(e.getLocalizedMessage());
    }

    private void logError(String line)
    {
        if (line != null) Log.e(MyApplication.TAG, line);
    }
}


