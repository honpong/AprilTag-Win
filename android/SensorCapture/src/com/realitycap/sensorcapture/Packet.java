package com.realitycap.sensorcapture;

import java.io.DataOutputStream;



public abstract class Packet
{
	public PacketHeader header;
	public byte[] byteArray;
	
	public Packet(short type, int bytes, short user, long time, byte[] byteArray)
	{
		this.header = new PacketHeader(bytes, type, user, time);
		this.byteArray = byteArray;
	}
	
	public abstract void writeToFile(DataOutputStream outputStream);
}
