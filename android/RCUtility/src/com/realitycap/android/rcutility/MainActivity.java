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
import android.widget.ToggleButton;

public class MainActivity extends Activity
{
	public static final String TAG = "RCUtility";
	
	private ToggleButton calibrationButton;
	private ToggleButton captureButton;
	private Button liveButton;
	private Button replayButton;
	
	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		
		calibrationButton = (ToggleButton) this.findViewById(R.id.calibrationButton);
		calibrationButton.setOnCheckedChangeListener(new OnCheckedChangeListener() {
			@Override
			public void onCheckedChanged(CompoundButton buttonView, boolean isChecked)
			{
				if (isChecked)
					startCalibration();
				else
					stopCalibration();
			}
		});
		
		captureButton = (ToggleButton) this.findViewById(R.id.captureButton);
		captureButton.setOnCheckedChangeListener(new OnCheckedChangeListener() {
			@Override
			public void onCheckedChanged(CompoundButton buttonView, boolean isChecked)
			{
				if (isChecked)
					startCapture();
				else
					stopCapture();
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
	}

	protected void startCalibration()
	{
		log("startCalibration");
	}

	protected void stopCalibration()
	{
		log("stopCalibration");
	}
	
	protected void startCapture()
	{
		log("startCapture");
	}

	protected void stopCapture()
	{
		log("stopCapture");
	}

	protected void startLiveActivity()
	{
		log("startLiveActivity");
	}

	protected void openFilePicker()
	{
		log("openFilePicker");
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
	
	private void logError(Exception e)
	{
		if (e != null) logError(e.getLocalizedMessage());
	}
	
	private void logError(String line)
	{
		if (line != null) Log.e(TAG, line);
	}
}
