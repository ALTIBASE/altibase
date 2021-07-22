/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
package com.altibase.altilinker.jcm;

import java.nio.ByteBuffer;

/**
 * Packet header (16 bytes) of CM block
 */
public class PacketHeader
{
    public static final byte HeaderSign = 0x07;
    public static final int  HeaderSize = 16;        // 16 bytes

    private byte  mHeaderSign;
    private byte  mReserved1;
    private int   mPayloadLength; // unsigned short
    private long  mSeqNo;         // unsigned int
    private short mReserved2;
    private int   mSessionId;     // unsigned short
    private int   mReserved3;

    /**
     * Validate packet header
     * 
     * @return      true, if validate. false, if not.
     */
    public boolean validate()
    {
        if (mHeaderSign != PacketHeader.HeaderSign)
        {
            return false;
        }
        
        return true;
    }
    
    /**
     * Get payload length of packet (CM block)
     * 
     * @param       Payload length of packet (CM block)
     */
    public int getPayloadLength()
    {
        return mPayloadLength;
    }

    /**
     * Get packet length (CM block length)
     * 
     * @return Packet length
     */
    public int getPacketLength()
    {
        return PacketHeader.HeaderSize + mPayloadLength;            
    }
    
    /**
     * Set payload length
     * 
     * @param aPayloadLength        Payload length
     */
    public void setPayloadLength(int aPayloadLength)
    {
        mPayloadLength = aPayloadLength;
    }
    
    /**
     * Get sequence number of packet
     * 
     * @return      Sequence number of packet
     */
    public long getSeqNo()
    {
        return mSeqNo;
    }
    
    /**
     * Get session ID of packet
     * 
     * @return      Session ID of packet
     */
    public int getSessionId()
    {
        return mSessionId;
    }
    
    /**
     * Set session ID of packet
     * 
     * @param aSessionId    Session ID of packet
     */
    public void setSessionId(int aSessionId)
    {
        mSessionId = aSessionId;
    }
    
    /**
     * Read a packet header from buffer
     * 
     * @param aReadBuffer   Buffer to read a packet header
     * @return              true, if validate. false, if not.
     */
    protected boolean read(ByteBuffer aReadBuffer)
    {
        // read header sign: 1byte
        mHeaderSign = aReadBuffer.get();
        
        if (mHeaderSign != PacketHeader.HeaderSign)
        {
            return false;
        }

        // read reserved1: 1byte
        aReadBuffer.get();

        // read payload length: 2bytes
        mPayloadLength = 0xffff & (int)aReadBuffer.getShort();

        // read sequence number: 4bytes
        mSeqNo = 0xffffffff & (long)aReadBuffer.getInt();

        // read reserved2: 2bytes
        aReadBuffer.getShort();
        
        // read session ID: 2bytes
        mSessionId = 0xffff & (int)aReadBuffer.getShort();

        // read reserved3: 4bytes
        aReadBuffer.getInt();
        
        return true;
    }
    
    /**
     * Write a packet header to buffer
     * 
     * @param aWriteBuffer  Buffer to write a packet header
     */
    protected void write(ByteBuffer aWriteBuffer)
    {
        mHeaderSign = HeaderSign; 
        mReserved1  = 0;
        mReserved2  = 0;
        mReserved3  = 0;
        
        aWriteBuffer.put     (mHeaderSign);
        aWriteBuffer.put     (mReserved1);
        aWriteBuffer.putShort((short)mPayloadLength);
        aWriteBuffer.putInt  ((int)mSeqNo);
        aWriteBuffer.putShort((short)mReserved2);
        aWriteBuffer.putShort((short)mSessionId);
        aWriteBuffer.putInt  ((short)mReserved3);
        
        ++mSeqNo;
        
        if (mSeqNo > 0x7fffffff)
        {
            mSeqNo = 0;
        }
    }
}
