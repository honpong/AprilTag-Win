package com.realitycap.android.rcutility;

import android.app.ActionBar;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.SurfaceHolder;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

/**
 * Created by benhirashima on 8/17/15.
 */
public class VisActivity extends TrackerActivity
{
    private static final String TAG = VisActivity.class.getSimpleName();
    public static final String ACTION_LIVE_VIS = "com.realitycap.rcutility.LIVE_VIS";
    public static final String ACTION_REPLAY_VIS = "com.realitycap.rcutility.REPLAY_VIS";

    enum AppState
    {
        Idle, LiveVis, ReplayVis
    }

    AppState appState = AppState.Idle;
    AppState desiredAppState = AppState.Idle;
    MyGLSurfaceView surfaceView;
    Uri replayFileUri = null;

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        showMessage("Tap anywhere to start");

        final Intent intent = getIntent();
        if (intent.getAction() == ACTION_LIVE_VIS)
            desiredAppState = AppState.LiveVis;
        else if (intent.getAction() == ACTION_REPLAY_VIS)
        {
            desiredAppState = AppState.ReplayVis;
            replayFileUri = intent.getData();
        }

        trackerProxy.createTracker();

        setContentView(R.layout.activity_vis);
        surfaceView = (MyGLSurfaceView) findViewById(R.id.surfaceview);
        surfaceView.setOnClickListener(new View.OnClickListener()
        {
            public void onClick(View view)
            {
                handleStartStopTap();
            }
        });

        ActionBar actionBar = getActionBar();
        actionBar.setDisplayHomeAsUpEnabled(true);
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

    @Override public boolean onCreateOptionsMenu(Menu menu)
    {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.vis_activity_menu, menu);

        return super.onCreateOptionsMenu(menu);
    }

    @Override public boolean onOptionsItemSelected(MenuItem item)
    {
        if (item.getItemId() == R.id.action_start)
        {
            handleStartStopTap();
        }
        return super.onOptionsItemSelected(item);
    }

    protected void handleStartStopTap()
    {
        if (appState == AppState.Idle)
        {
            if (desiredAppState == AppState.LiveVis)
                enterLiveState();
            else if (desiredAppState == AppState.ReplayVis)
                enterReplayState(replayFileUri);
        }
        else enterIdleState();
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

        setButtonText("Stop");
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

        surfaceView.startRendering();

        appState = AppState.LiveVis;
        return true;
    }

    protected void exitLiveState()
    {
        if (appState != AppState.LiveVis) return;
        showMessage("Stopping ...");
        surfaceView.stopRendering();
        stopSensors();
        appState = AppState.Idle;
        showMessage("Stopped.");
        setButtonText("Start");
    }

    protected boolean enterReplayState(Uri uri)
    {
        if (appState != AppState.Idle || uri == null) return false;

        setButtonText("Stop");

        String absFilePath = UriUtils.getPath(this, uri);
        if (absFilePath == null || absFilePath.length() == 0)
        {
            showMessage("Bad replay file URI.");
            return false;
        }
        else
        {
            showMessage("Starting replay of file: " + absFilePath);
        }

        if (!trackerProxy.startReplay(absFilePath))
        {
            return abortTracking("Failed to start replay.");
        }

        showMessage("Replay running...");
        appState = AppState.ReplayVis;
        return true;
    }

    protected void exitReplayState()
    {
        if (appState != AppState.ReplayVis) return;
        showMessage("Stopping ...");
        trackerProxy.stopReplay();
        appState = AppState.Idle;
        showMessage("Stopped.");
        setButtonText("Start");
    }

    protected void setButtonText(String text)
    {
        TextView button = (TextView)findViewById(R.id.action_start);
        button.setText(text);
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

    @Override public void onStatusUpdated(int runState, int errorCode, int confidence, float progress)
    {
        // won't be called.
    }

    @Override public void onDataUpdated(SensorFusionData data)
    {
        // won't be called.
    }
}
