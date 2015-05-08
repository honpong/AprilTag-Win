package com.realitycap.sensorcapture;

import java.nio.ByteBuffer;

public class PacketHeader
{
	public int bytes;
	public short type;
	public short user;
	public long time;
	
	PacketHeader (int bytes, short type, short user, long time)
	{
		this.bytes = bytes + 16;
		this.type = type;
		this.user = user;
		this.time = time;
	}
	
	public byte[] getBytes()
	{
		ByteBuffer bb = ByteBuffer.allocate(16);
		bb.order(null); // null means little endian
		bb.putInt(bytes);
		bb.putShort(type);
		bb.putShort(user);
		bb.putLong(time);
		return bb.array();
	}
}
