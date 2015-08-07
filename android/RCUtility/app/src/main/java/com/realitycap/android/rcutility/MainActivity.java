package com.realitycap.android.rcutility;

import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Environment;
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
import android.os.Handler;

import com.intel.camera.toolkit.depth.Camera;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;

public class MainActivity extends Activity implements ITrackerReceiver
{
    private static final String PREF_KEY_CALIBRATION = "calibration";

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
                    if (!startCalibration()) calibrationButton.setChecked(false);
                }
                else
                {
                    stopCalibration();
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
                    if (!startCapture()) captureButton.setChecked(false);
                }
                else
                {
                    stopCapture();
                }
            }
        });

        liveButton = (Button) this.findViewById(R.id.liveButton);
        liveButton.setOnClickListener(new OnClickListener()
        {
            @Override
            public void onClick(View v)
            {
                startLiveActivity();
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
        if (!rsMan.startCameras()) return false;
//        if(!rsMan.startImu()) return false;
        return true;
    }

    protected void stopSensors()
    {
        imuMan.stopSensors();
//        rsMan.stopImu();
        rsMan.stopCameras();
    }

    protected boolean startCalibration()
    {
        if (appState != AppState.Idle) return false;
        setStatusText("Starting calibration...");

        trackerProxy.createTracker();

        if (!startSensors())
        {
            cancelCalibration();
            setStatusText("Failed to start sensors.");
            return false;
        }

        Camera.Calibration.Intrinsics intr = rsMan.getCameraIntrinsics();
        if (intr == null)
        {
            cancelCalibration();
            setStatusText("Failed to get camera intrinsics.");
            return false;
        }
        trackerProxy.configureCamera(TrackerProxy.CAMERA_EGRAY8, 640, 480, intr.principalPoint.x, intr.principalPoint.y, intr.focalLength.x, intr.focalLength.y, 0, false, 0);

        final String defaultCal = readTextFromFile("ft210.json");
        if (defaultCal == null || defaultCal.length() <= 0)
        {
            cancelCalibration();
            setStatusText("Failed to load default calibration file, or file was empty.");
            return false;
        }

        if (!trackerProxy.setCalibration(defaultCal))
        {
            cancelCalibration();
            setStatusText("Failed to set default calibration.");
            return false;
        }

        if (!trackerProxy.startCalibration())
        {
            cancelCalibration();
            return false;
        }

        appState = AppState.Calibrating;
        setStatusText("Calibrating");
        return true;
    }

    private void cancelCalibration()
    {
        stopSensors();
        trackerProxy.destroyTracker();
        setStatusText("Failed to start calibration");
    }

    protected void stopCalibration()
    {
        if (appState != AppState.Calibrating) return;
        stopSensors();

        boolean success = true;
        String cal = trackerProxy.getCalibration();
        if (cal != null)
        {
            prefs.edit().putString(PREF_KEY_CALIBRATION, cal).apply();
            success = writeTextToFile(cal, "calibration.json");
        }
        else success = false;

        trackerProxy.stopTracker();
        trackerProxy.destroyTracker();

        if (success) setStatusText("Calibration stopped.");
        else setStatusText("Calibration not saved");

        appState = AppState.Idle;
    }

    // work in progress
    protected boolean startCapture()
    {
        trackerProxy.createTracker();

        if (appState != AppState.Idle) return false;
        setStatusText("Starting capture...");
        if (!startSensors())
        {
            stopSensors(); // in case one set of sensors started and the other didn't
            trackerProxy.destroyTracker();
            setStatusText("Failed to start sensors.");
            return false;
        }
		trackerProxy.setOutputLog(Environment.getExternalStorageDirectory() + "/capture");
        setStatusText("Capturing...");
        appState = AppState.Capturing;
        return true;
    }

    // work in progress
    protected void stopCapture()
    {
        if (appState != AppState.Capturing) return;
        stopSensors();
        trackerProxy.destroyTracker();
        setStatusText("Capture stopped.");
        appState = AppState.Idle;
    }

    // work in progress
    protected boolean startLiveActivity()
    {
        if (appState != AppState.Idle) return false;
        setStatusText("startLiveActivity");
        return true;
    }

    protected String readTextFromFile(String filename)
    {
        BufferedReader in = null;
        try
        {
            StringBuilder buf = new StringBuilder();
            InputStream is = getAssets().open(filename);
            in = new BufferedReader(new InputStreamReader(is));

            String str;
            while ((str = in.readLine()) != null)
            {
                buf.append(str);
            }
            return buf.toString();
        }
        catch (IOException e)
        {
            Log.e(MyApplication.TAG, e.getLocalizedMessage());
        }
        finally
        {
            if (in != null)
            {
                try
                {
                    in.close();
                }
                catch (IOException e)
                {
                    Log.e(MyApplication.TAG, e.getLocalizedMessage());
                }
            }
        }

        return null;
    }

    protected boolean writeTextToFile(String text, String filename)
    {
        FileOutputStream outputStream;
        File file = new File(Environment.getExternalStorageDirectory(), filename);

        try
        {
            outputStream = new FileOutputStream(file);
            outputStream.write(text.getBytes());
            outputStream.close();
        }
        catch (Exception e)
        {
            Log.e(MyApplication.TAG, e.getLocalizedMessage());
            return false;
        }

        return true;
    }

    // work in progress
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
                        stopCalibration();
                        setStatusText("Tracker error code: " + errorCode);
                        return;
                    }

                    int percentage = Math.round(progress * 100);

                    switch (runState)
                    {
                        case 0: // idle
                            stopCalibration();
                            break;
                        case 1: // static calibration
                            if (progress <= 1.) setStatusText("Place device on a flat surface. Progress: " + percentage + "%");
                            break;
                        case 5: // portrait calibration
                            if (progress <= 1.) setStatusText("Hold steady in portrait orientation. Progress: " + percentage + "%");
                            break;
                        case 6: // landscape calibraiton
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
                        stopCapture();
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


