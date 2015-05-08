package com.realitycap.sensorcapture;

import java.io.DataOutputStream;
import java.io.IOException;

import android.util.Log;

public class PacketCamera extends Packet
{
	public String pgm;
	
	public PacketCamera(short type, int bytes, short user, long time, byte[] byteArray)
	{
		super(type, bytes + 16, user, time, byteArray);
		pgm = String.format("P5 %4d %3d %d\n", 640, 480, 255);
	}
	
	public void writeToFile(DataOutputStream outputStream)
	{
		if (outputStream == null) return;
		try
		{
			Log.v(MainActivity.TAG, header.getBytes().toString());
			outputStream.write(header.getBytes());
			outputStream.write(pgm.getBytes());
			outputStream.write(byteArray, 0, 640*480);
//			log("file position " + fileOutput.getChannel().position());
		}
		catch (IOException ex)
		{
//			logError(ex);
		}
	}
}
