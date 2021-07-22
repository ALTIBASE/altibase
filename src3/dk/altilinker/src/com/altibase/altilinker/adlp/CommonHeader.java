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
 
package com.altibase.altilinker.adlp;

import java.nio.ByteBuffer;

/**
 * Common header class of ADLP protocol
 * 
 * Common header is consisted of 10 bytes, total 4 field,
 * operation ID field, operation specific parameter field,
 * session ID field, data length field.
 */
public class CommonHeader
{
    public static final int HeaderSize = 12;
    
    public static final byte FLAG_XA       = 0x01;
    public static final byte FLAG_TX_BEGIN = 0x02;
    
    protected byte mOperationId;
    protected byte mOpSpecificParameter;
    protected byte mFlags    = 0x00;
    protected byte mReserved = 0x00;     // DKP_ADLP_PACKET_RESERVED_NOT_USED
    protected int  mSessionId;
    protected int  mDataLength;          // Maximum CM Block size is 32KB

    /**
     * Get operation ID
     * 
     * @return      Operation ID
     */
    public byte getOperationId()
    {
        return mOperationId;
    }
    
    /**
     * Set operation ID
     * 
     * @param aOperationId  Operation ID
     */
    public void setOperationId(byte aOperationId)
    {
        mOperationId = aOperationId;
    }
    
    /**
     * get operation specific parameter
     * 
     * @return      Operation specific parameter
     */
    public byte getOpSpecificParameter()
    {
        return mOpSpecificParameter;
    }
    
    /**
     * Set operation specific parameter
     * 
     * @param aOpSpecificParameter  Operation specific parameter
     */
    public void setOpSpecificParameter(byte aOpSpecificParameter)
    {
        mOpSpecificParameter = aOpSpecificParameter;
    }
    
    /**
     * get operation specific parameter
     * 
     * @return      Operation specific parameter
     */
    public byte getFlags()
    {
        return mFlags;
    }
    
    /**
     * Set header flags
     * 
     * @param aFlags
     */
    public void setFlags(byte aFlags)
    {
        mFlags = aFlags;
    }
    
    public void setXA( boolean aIsXA )
    {
        if( aIsXA == true )
        {
            mFlags |= FLAG_XA;
        }
        else
        {
            mFlags &= (~FLAG_XA); // 0xFE
        }
    }
    
    public boolean isXA()
    {
        return ( ( mFlags & FLAG_XA ) == FLAG_XA )? true : false;
    }
    
    public void setTxBegin( boolean aIsBegin )
    {
        if( aIsBegin == true )
        {
            mFlags |= FLAG_TX_BEGIN;
        }
        else
        {
            mFlags &= (~FLAG_TX_BEGIN); // 0xFD
        }
    }
    
    public boolean isTxBegin()
    {
        return ( ( mFlags & FLAG_TX_BEGIN) == FLAG_TX_BEGIN )? true : false;
    }
    
    /**
     * Get session ID
     * 
     * @return      Session ID
     */
    public int getSessionId()
    {
        return mSessionId;
    }
    
    /**
     * Set session ID
     * 
     * @param aSessionId    Session ID
     */
    public void setSessionId(int aSessionId)
    {
        mSessionId = aSessionId;
    }
    
    /**
     * Get operation payload length
     * 
     * @return      Operation payload length
     */
    public int getDataLength()
    {
        return mDataLength;
    }
    
    /**
     * Set operation payload length
     * 
     * @param aDataLength   Operation payload legnth
     */
    public void setDataLength(int aDataLength)
    {
        mDataLength = aDataLength;
    }
    
    /**
     * Read common header from buffer
     * 
     * @param aReadBuffer   Buffer to be read common header
     */
    protected void read(ByteBuffer aReadBuffer)
    {
        // read operation ID: 1byte
        mOperationId = aReadBuffer.get();

        // read operation specific parameter: 1byte
        mOpSpecificParameter = aReadBuffer.get();
        
        mFlags               = aReadBuffer.get();
        
        mReserved            = aReadBuffer.get();
        
        // read session ID: 4bytes
        mSessionId = aReadBuffer.getInt();

        // read data length: 4bytes
        mDataLength = aReadBuffer.getInt();
    }
    
    /**
     * Write common header to buffer
     * 
     * @param aWriteBuffer  Buffer to be written common header
     */
    protected void write(ByteBuffer aWriteBuffer)
    {
        aWriteBuffer.put(mOperationId);
        aWriteBuffer.put(mOpSpecificParameter);
        aWriteBuffer.put(mFlags);
        aWriteBuffer.put(mReserved);
        aWriteBuffer.putInt(mSessionId);
        aWriteBuffer.putInt(mDataLength);
    }
}
