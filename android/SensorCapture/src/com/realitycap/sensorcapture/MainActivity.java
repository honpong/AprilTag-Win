package com.realitycap.sensorcapture;

import java.io.DataOutputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.text.SimpleDateFormat;
import java.util.GregorianCalendar;
import java.util.Locale;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;

import android.app.Activity;
import android.content.Context;
import android.graphics.ImageFormat;
import android.hardware.Camera;
import android.hardware.Camera.PreviewCallback;
import android.hardware.Camera.Size;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.TextView;

public class MainActivity extends Activity implements SensorEventListener
{
	public static final String TAG = "SensorCapture";
	private static final String LOG_FILE = Environment.getExternalStorageDirectory() + "/" + TAG;
	private static final int MOTION_SENSOR_RATE = 10000; // 100hz
	private SensorManager mSensorManager;
	private Button button;
	private TextView accelX, accelY, accelZ, accelRate, gyroX, gyroY, gyroZ, gyroRate, frameRate, imageFormat, supportedFormats;
	private boolean isRunning = false;
	private CameraPreview cameraPreview;
	private Camera camera;
	int accelCount, gyroCount, frameCount;
	long accelTime, gyroTime, frameTime;
	byte[] buffer;
	DataOutputStream outputStream = null;
	FileOutputStream fileOutput = null;
	final BlockingQueue<Packet> serialQueue = new ArrayBlockingQueue<Packet>(100);
	Thread fileWritingThread;
	
	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		
		button = (Button) this.findViewById(R.id.button1);
		button.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v)
			{
				toggleStart();
			}
		});
		
		accelX = (TextView) this.findViewById(R.id.accelX);
		accelY = (TextView) this.findViewById(R.id.accelY);
		accelZ = (TextView) this.findViewById(R.id.accelZ);
		accelRate = (TextView) this.findViewById(R.id.accelRate);
		gyroX = (TextView) this.findViewById(R.id.gyroX);
		gyroY = (TextView) this.findViewById(R.id.gyroY);
		gyroZ = (TextView) this.findViewById(R.id.gyroZ);
		gyroRate = (TextView) this.findViewById(R.id.gyroRate);
		frameRate = (TextView) this.findViewById(R.id.frameRate);
		imageFormat = (TextView) this.findViewById(R.id.imageFormat);
		supportedFormats = (TextView) this.findViewById(R.id.supportedFormats);
		
		accelCount = gyroCount = frameCount = 0;
		accelTime = gyroTime = frameTime = 0;
		
		mSensorManager = (SensorManager) getSystemService(Context.SENSOR_SERVICE);
	}
	
	@Override
	protected void onStart()
	{
		super.onStart();
		log("onStart()");
		startVideo();
		startSensors();
	}

	@Override
	protected void onStop()
	{
		log("onStop()");
		stopCapture();
		stopVideo();
		stopSensors();
		super.onStop();
	}
	
	private void toggleStart()
	{
		if (isRunning)
		{
			stopCapture();
			button.setText("Start");
		}
		else
		{
			startCapture();
			button.setText("Stop");
		}
	}
	
	private void startVideo()
	{
		camera = getCameraInstance();
		if (camera == null)
		{
			log("Failed to access camera");
			return;
		}
		
		camera.setDisplayOrientation(90);
		
		final Camera.Parameters parameters = camera.getParameters();
		parameters.setPreviewFormat(ImageFormat.NV21);
		parameters.setFocusMode(Camera.Parameters.FOCUS_MODE_INFINITY);
//		parameters.setPreviewSize(320, 240);
		parameters.setPreviewSize(640, 480);
		camera.setParameters(parameters);
		
		imageFormat.setText(getImageFormatString(parameters.getPreviewFormat()));
		supportedFormats.setText("Supported\n" + getSupportedImageFormatsString(parameters));
		
		cameraPreview = new CameraPreview(this, camera);
		final FrameLayout preview = (FrameLayout) findViewById(R.id.camera_preview);
		preview.removeAllViews();
		preview.addView(cameraPreview);
		
		final Size previewSize = parameters.getPreviewSize();
		final int dataBufferSize = (int) (previewSize.height * previewSize.width * (ImageFormat.getBitsPerPixel(parameters.getPreviewFormat()) / 8.0));
		final int bufferCount = 8;
		for (int i = 0; i < bufferCount; i++)
		{
			camera.addCallbackBuffer(new byte[dataBufferSize]);
		}
		
		camera.setPreviewCallbackWithBuffer(new PreviewCallback() {
			@Override
			public void onPreviewFrame(final byte[] data, Camera camera)
			{
				if(isRunning) enqueueData(new PacketCamera((short) 1, 640 * 480, (short) 0, System.nanoTime() / 1000, data));
				else camera.addCallbackBuffer(data);
				clockFrameRate();
			}
		});
		
		camera.startPreview();
	}
	
	private void stopVideo()
	{
		camera.setPreviewCallback(null);
		camera.stopPreview();
		camera.release();
		camera = null;
		cameraPreview.close();
		cameraPreview = null;
	}
	
	private void startSensors()
	{
		final Sensor accelerometer = mSensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
		if (accelerometer != null) mSensorManager.registerListener(this, accelerometer, MOTION_SENSOR_RATE);
		
		final Sensor gyro = mSensorManager.getDefaultSensor(Sensor.TYPE_GYROSCOPE);
		if (gyro != null) mSensorManager.registerListener(this, gyro, MOTION_SENSOR_RATE);
	}
	
	private void stopSensors()
	{
		mSensorManager.unregisterListener(this);
	}
	
	private void startCapture()
	{
		if (isRunning) return;
		log("startCapture()");
		
		final GregorianCalendar now = new GregorianCalendar();
		final SimpleDateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd-HH-mm-ss", Locale.US);
		final String datedFileName = LOG_FILE + "-" + dateFormat.format(now.getTime());
		openFile(datedFileName);
						
		fileWritingThread = new Thread(new Runnable() {
			@Override
			public void run()
			{
				try
				{
					Packet data;
					while (true)
					{
						if ((data = serialQueue.take()) != null)
						{
							data.writeToFile(outputStream);
							if(data instanceof PacketCamera) {
								camera.addCallbackBuffer(data.byteArray);
							}
						}
					}
				}
				catch (InterruptedException e)
				{
					logError(e);
				}
			}
		});
		fileWritingThread.start();

		isRunning = true;
		
//		Debug.startMethodTracing(TAG);
	}
	
	private void stopCapture()
	{
		if (!isRunning) return;
		log("stopCapture()");

		isRunning = false;
		
		while(!serialQueue.isEmpty()) {
			try {
				Thread.sleep(50);
			} catch (InterruptedException e) {
			}
		}
		fileWritingThread.interrupt();
		closeFile();		
		
//		Debug.stopMethodTracing();
	}
	
	private void enqueueData(Packet data)
	{
		boolean success = serialQueue.offer(data);
		if (!success) log("Queue is full");
	}
	
	private boolean openFile(String fileName)
	{
		log("openFile(" + fileName + ")");
		try
		{
			fileOutput = new FileOutputStream(fileName);
			outputStream = new DataOutputStream(fileOutput);
			return outputStream != null;
		}
		catch (FileNotFoundException ex)
		{
			logError("File not found.");
			return false;
		}
	}
	
	private void closeFile()
	{
		log("closeFile()");
		try
		{
			outputStream.flush();
			outputStream.close();
		}
		catch (IOException e)
		{
			logError(e);
		}
		finally
		{
			outputStream = null;
		}
	}
	
	@Override
	public void onAccuracyChanged(Sensor sensor, int accuracy)
	{
		log(String.format("onAccuracyChanged(%s, %d)", sensor.getName(), accuracy));
	}
	
	@Override
	public void onSensorChanged(SensorEvent sensorEvent)
	{
		if (sensorEvent.sensor.getType() == Sensor.TYPE_ACCELEROMETER)
		{
			if(isRunning) {
				final ByteBuffer bb = ByteBuffer.allocate(16);
				bb.order(null);
				bb.putFloat(sensorEvent.values[0]);
				bb.putFloat(sensorEvent.values[1]);
				bb.putFloat(sensorEvent.values[2]);
				bb.putFloat(0);
			
				enqueueData(new PacketImu((short) 20, bb.capacity(), (short) 0, System.nanoTime() / 1000, bb.array()));
			}
			clockAccelRate();
			accelX.setText(String.format("X %.3f", sensorEvent.values[0]));
			accelY.setText(String.format("Y %.3f", sensorEvent.values[1]));
			accelZ.setText(String.format("Z %.3f", sensorEvent.values[2]));
		}
		else if (sensorEvent.sensor.getType() == Sensor.TYPE_GYROSCOPE || sensorEvent.sensor.getType() == Sensor.TYPE_GYROSCOPE_UNCALIBRATED)
		{
			if(isRunning) {
				final ByteBuffer bb = ByteBuffer.allocate(16);
				bb.order(null);
				bb.putFloat(sensorEvent.values[0]);
				bb.putFloat(sensorEvent.values[1]);
				bb.putFloat(sensorEvent.values[2]);
				bb.putFloat(0);
			
				enqueueData(new PacketImu((short) 21, bb.capacity(), (short) 0, System.nanoTime() / 1000, bb.array()));
			}
			clockGyroRate();
			gyroX.setText(String.format("X %.3f", sensorEvent.values[0]));
			gyroY.setText(String.format("Y %.3f", sensorEvent.values[1]));
			gyroZ.setText(String.format("Z %.3f", sensorEvent.values[2]));
		}
	}
	
	private Camera getCameraInstance()
	{
		Camera c = null;
		try
		{
			c = Camera.open(0); // attempt to get a Camera instance
		}
		catch (Exception e)
		{
			// Camera is not available (in use or does not exist)
			logError(e);
		}
		return c; // returns null if camera is unavailable
	}
	
	private void clockFrameRate()
	{
		final long timeNow = System.currentTimeMillis();
		if (timeNow - frameTime > 1000)
		{
			frameRate.setText(String.format("%d FPS", frameCount));
			frameCount = 1;
			frameTime = timeNow;
			if (serialQueue != null) imageFormat.setText(String.valueOf(serialQueue.size()) + " in queue"); 
		}
		else
		{
			frameCount++;
		}
	}
	
	private boolean clockAccelRate()
	{
		final long timeNow = System.currentTimeMillis();
		if (timeNow - accelTime > 1000)
		{
			accelRate.setText(String.format("%d Hz", accelCount));
			accelCount = 1;
			accelTime = timeNow;
			return true;
		}
		else
		{
			accelCount++;
		}
		return false;
	}
	
	private boolean clockGyroRate()
	{
		final long timeNow = System.currentTimeMillis();
		if (timeNow - gyroTime > 1000)
		{
			gyroRate.setText(String.format("%d Hz", gyroCount));
			gyroCount = 1;
			gyroTime = timeNow;
			return true;
		}
		else
		{
			gyroCount++;
		}
		return false;
	}
	
	private String getSupportedImageFormatsString(Camera.Parameters parameters)
	{
		final StringBuilder sb = new StringBuilder(parameters.getSupportedPreviewFormats().size());
		for (Integer i : parameters.getSupportedPreviewFormats())
		{
			sb.append(getImageFormatString(i));
			sb.append("\n");
		}
		return sb.toString();
	}
	
	private String getImageFormatString(int i)
	{
		switch (i)
		{
			case ImageFormat.JPEG:
				return "JPEG";
			case ImageFormat.NV16:
				return "NV16";
			case ImageFormat.NV21:
				return "NV21";
			case ImageFormat.RGB_565:
				return "RGB_565";
			case ImageFormat.UNKNOWN:
				return "UNKNOWN";
			case ImageFormat.YUV_420_888:
				return "YUV_420_888";
			case ImageFormat.YUY2:
				return "YUY2";
			case ImageFormat.YV12:
				return "YV12";
			default:
				return "";
		}
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
