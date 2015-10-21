package com.realitycap.android.rcutility;

import android.app.Application;
import android.content.Context;

public class MyApplication extends Application
{
	public static final String TAG = "RCUtility";
    public static final String SHARED_PREFS = "RCUtilityPrefs";
	private static Context context;
	
	public static Context getContext()
	{
		return context;
	}
	
	@Override
	public void onCreate()
	{
		super.onCreate();
		context = this.getApplicationContext();
	}
}
