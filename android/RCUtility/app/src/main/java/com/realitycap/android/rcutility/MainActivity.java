package com.realitycap.android.rcutility;

import android.app.Activity;
import android.os.Bundle;
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

public class MainActivity extends Activity
{
	private TextView statusText;
	private ToggleButton calibrationButton;
	private ToggleButton captureButton;
	private Button liveButton;
	private Button replayButton;
	
	IMUManager imuMan;
	VideoManager videoMan;
	TrackerProxy trackerProxy;
    RealSenseManager rsMan;

	enum AppState
	{
		Idle, Calibrating, Capturing, LiveVis, ReplayVis
	}
	
	AppState appState = AppState.Idle;
	
	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		
		statusText = (TextView) this.findViewById(R.id.statusText);
		
		calibrationButton = (ToggleButton) this.findViewById(R.id.calibrationButton);
		calibrationButton.setOnCheckedChangeListener(new OnCheckedChangeListener() {
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
		captureButton.setOnCheckedChangeListener(new OnCheckedChangeListener() {
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
		liveButton.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v)
			{
				startLiveActivity();
			}
		});
		
		replayButton = (Button) this.findViewById(R.id.replayButton);
		replayButton.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v)
			{
				openFilePicker();
			}
		});
		
		imuMan = new IMUManager();
		videoMan = new VideoManager();
		trackerProxy = new TrackerProxy();
		
		imuMan.setSensorEventListener(trackerProxy);

		rsMan = new RealSenseManager(this, trackerProxy);

        trackerProxy.createTracker();
		
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
        if(!imuMan.startSensors()) return false;
        if(!rsMan.startCameras()) return false;
        return true;
	}
	
	protected void stopSensors()
	{
		imuMan.stopSensors();
        rsMan.stopCameras();
	}
	
	protected boolean startCalibration()
	{
		if (appState != AppState.Idle) return false;
        setStatusText("Starting calibration...");

        if (!startSensors()) cancelCalibration();

        Camera.Calibration.Intrinsics intr = rsMan.getCameraIntrinsics();
        if (intr == null) cancelCalibration();
        trackerProxy.configureCamera(0, 640, 480, intr.principalPoint.x, intr.principalPoint.y, intr.focalLength.x, intr.focalLength.y, 0, false, 0);

        boolean result = trackerProxy.startCalibration();
		if (result)
        {
            appState = AppState.Calibrating;
            setStatusText("Calibrating");
        }
        else cancelCalibration();

		return result;
	}

    private void cancelCalibration()
    {
        stopSensors();
        appState = AppState.Idle;
        setStatusText("Failed to start calibration");
    }
	
	protected void stopCalibration()
	{
		if (appState != AppState.Calibrating) return;
		trackerProxy.stopTracker();
		imuMan.stopSensors();
        rsMan.stopCameras();
		setStatusText("Calibration stopped.");
		appState = AppState.Idle;
	}
	
	protected boolean startCapture()
	{
		if (appState != AppState.Idle) return false;
        setStatusText("Starting capture...");
		if (!startSensors())
        {
            stopSensors(); // in case one set of sensors started and the other didn't
            setStatusText("Failed to start sensors.");
            return false;
        }
//		trackerProxy.setOutputLog("capture.rssdk");
		setStatusText("Capturing...");
		appState = AppState.Capturing;
		return true;
	}
	
	protected void stopCapture()
	{
		if (appState != AppState.Capturing) return;
		stopSensors();
		setStatusText("Capture stopped.");
		appState = AppState.Idle;
	}
	
	protected boolean startLiveActivity()
	{
		if (appState != AppState.Idle) return false;
		setStatusText("startLiveActivity");
		return true;
	}
	
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


