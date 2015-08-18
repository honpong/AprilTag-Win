package com.realitycap.android.rcutility;

import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Toast;

/**
 * Created by benhirashima on 8/17/15.
 */
public class VisActivity extends TrackerActivity implements SurfaceHolder.Callback
{
    private static final String TAG = VisActivity.class.getSimpleName();
    public static final String ACTION_LIVE_VIS = "com.realitycap.rcutility.LIVE_VIS";
    public static final String ACTION_REPLAY_VIS = "com.realitycap.rcutility.REPLAY_VIS";

    enum AppState
    {
        Idle, LiveVis, ReplayVis
    }

    AppState appState = AppState.Idle;

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        trackerProxy.createTracker();
        trackerProxy.receiver = this;

        setContentView(R.layout.activity_vis);
        SurfaceView surfaceView = (SurfaceView) findViewById(R.id.surfaceview);
        surfaceView.getHolder().addCallback(this);
        surfaceView.setOnClickListener(new View.OnClickListener()
        {
            public void onClick(View view)
            {
                if (appState == AppState.Idle)
                    enterLiveState();
                else
                    enterIdleState();
            }
        });
    }

    @Override protected void onPause()
    {
        enterIdleState();
        super.onPause();
    }

    @Override protected void onDestroy()
    {
        enterIdleState();
        trackerProxy.destroyTracker();
        super.onDestroy();
    }

    protected void enterIdleState()
    {
        switch (appState)
        {
            case LiveVis:
                exitLiveState();
                break;
            case ReplayVis:
                exitReplayState();
                break;
        }
    }

    protected boolean enterLiveState()
    {
        if (appState != AppState.Idle) return false;

        showMessage("Starting live visualization...");

        if (!setCalibrationFromFile(CALIBRATION_FILENAME))
        {
            return abortTracking("Failed to load calibration file. You may need to run calibration first.");
        }

        if (!configureCamera())
        {
            return abortTracking("Failed to get camera intrinsics.");
        }

        if (!startSensors())
        {
            return abortTracking("Failed to start sensors.");
        }

        if (!trackerProxy.startTracker())
        {
            return abortTracking("Failed to start tracking.");
        }

        appState = AppState.LiveVis;
        return true;
    }

    protected void exitLiveState()
    {
        if (appState != AppState.LiveVis) return;
        showMessage("Stopping ...");
        stopSensors();
        appState = AppState.Idle;
        showMessage("Stopped.");
    }

    protected boolean enterReplayState(String absFilePath)
    {
        if (appState != AppState.Idle) return false;
        showMessage("Starting replay of file: " + absFilePath);

        if (!setCalibrationFromFile(CALIBRATION_FILENAME))
        {
            return abortTracking("Failed to load calibration file. You may need to run calibration first.");
        }

        if (!configureCamera())
        {
            return abortTracking("Failed to get camera intrinsics.");
        }

        if (!trackerProxy.startTracker())
        {
            return abortTracking("Failed to start tracking.");
        }

//        if (!startPlayback(absFilePath))
//        {
//            return abortTracking("Failed to start playback.");
//        }

        showMessage("Replay running...");
        appState = AppState.ReplayVis;
        return true;
    }

    protected void exitReplayState()
    {
        if (appState != AppState.ReplayVis) return;
        showMessage("Stopping ...");
//        stopPlayback();
        appState = AppState.Idle;
        showMessage("Stopped.");
    }

    private boolean abortTracking(String message)
    {
        Log.d(TAG, "abortTracking()");
        showMessage(message);
        stopSensors();
        return false;
    }

    protected void showMessage(String text)
    {
        Toast.makeText(this, text, Toast.LENGTH_SHORT).show();
        Log.i(TAG, text);
    }

    public void surfaceChanged(SurfaceHolder holder, int format, int w, int h)
    {
        if (appState == AppState.Idle) return;
        trackerProxy.setGLSurface(holder.getSurface());
    }

    public void surfaceCreated(SurfaceHolder holder)
    {
    }

    public void surfaceDestroyed(SurfaceHolder holder)
    {
    }

    @Override public void onStatusUpdated(int runState, int errorCode, int confidence, float progress)
    {
        // do nothing
    }

    @Override public void onDataUpdated(SensorFusionData data)
    {
        // do nothing
    }
}
