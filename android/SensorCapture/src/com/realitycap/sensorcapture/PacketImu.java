package com.realitycap.sensorcapture;

import java.io.DataOutputStream;
import java.io.IOException;

public class PacketImu extends Packet
{
	
	public PacketImu(short type, int bytes, short user, long time, byte[] byteArray)
	{
		super(type, bytes, user, time, byteArray);
		// TODO Auto-generated constructor stub
	}

	@Override
	public void writeToFile(DataOutputStream outputStream)
	{
		if (outputStream == null) return;
		try
		{
			outputStream.write(header.getBytes());
			outputStream.write(byteArray);
		}
		catch (IOException ex)
		{
//			logError(ex);
		}
	}
	
}
