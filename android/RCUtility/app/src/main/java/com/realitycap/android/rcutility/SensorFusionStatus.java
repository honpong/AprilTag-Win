package com.realitycap.android.rcutility;

public class SensorFusionStatus
{
	int runState;
	float progress;
	int confidence;
	int errorCode;
	
	public int getRunState()
	{
		return runState;
	}
	
	public void setRunState(int runState)
	{
		this.runState = runState;
	}
	
	public float getProgress()
	{
		return progress;
	}
	
	public void setProgress(float progress)
	{
		this.progress = progress;
	}

	public int getConfidence()
	{
		return confidence;
	}

	public void setConfidence(int confidence)
	{
		this.confidence = confidence;
	}

	public int getErrorCode()
	{
		return errorCode;
	}

	public void setErrorCode(int errorCode)
	{
		this.errorCode = errorCode;
	}
}
