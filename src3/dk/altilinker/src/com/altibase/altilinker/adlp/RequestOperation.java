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

import com.altibase.altilinker.adlp.CommonHeader;
import com.altibase.altilinker.util.TraceLogger;
import com.altibase.altilinker.adlp.type.AldpXid;

/**
 * Request operation abstract class to be received from DK module of ALTIBASE server
 */
public abstract class RequestOperation extends Operation
{
    /**
    * Constructor
    * 
    * @param aOperationId       Operation ID
    * @param aRemoteOperation   Whether remote operation or control operation
    * @param aFragmentable      Whether potential fragmentable operation or not
    */
    protected RequestOperation(byte    aOperationId,
                               boolean aRemoteOperation,
                               boolean aFragmentable)
    {
        super(aOperationId, aRemoteOperation, aFragmentable);
    }

    /**
     * Constructor
     * 
     * @param aOperationId      Operation ID
     * @param aRemoteOperation  Whether remote operation or control operation
     */
    protected RequestOperation(byte aOperationId, boolean aRemoteOperation)
    {
        super(aOperationId, aRemoteOperation);
    }
    
    /**
     * Whether request operation or result operation
     * 
     * @return  true, if request operation. false, if not.
     */
    public boolean isRequestOperation()
    {
        return true;
    }

    /**
     * Set common header
     * 
     * @param aCommonHeader     Common header
     */
    protected void setCommonHeader(CommonHeader aCommonHeader)
    {
        setOpSpecificParameter(aCommonHeader.getOpSpecificParameter());
        setSessionId(aCommonHeader.getSessionId());
        
        mXA      = aCommonHeader.isXA();
        mTxBegin = aCommonHeader.isTxBegin();
    }

    /**
     * Whether fragmented operation or not
     * 
     * @param aCommonHeader     Common header
     * @return                  true, if this operation is fragmented. false, if else.
     */
    protected boolean isFragmented(CommonHeader aCommonHeader)
    {
        return false;
    }
    
    /**
     * Create new result operation class instance for this request operation
     * 
     * @return  New result operation class instance
     */
    public abstract ResultOperation newResultOperation();

    /**
     * Read operation by common header and operation payload
     * 
     * @param aCommonHeader     Common header
     * @param aOpPayload        Operation payload
     * @return                  true, if common header and operatio payload is valid to read. false, if invalid. 
     */
    protected abstract boolean readOperation(CommonHeader aCommonHeader,
                                             ByteBuffer   aOpPayload);
    
    /**
     * Validate field values of operation
     * 
     * @return  true, if the values is valid. false, if invalid.
     */
    protected abstract boolean validate();

    /**
     * Read 1 byte value from buffer
     * 
     * @param aReadBuffer       Buffer to read
     * @return                  1 byte value
     */
    protected byte readByte(ByteBuffer aReadBuffer)
    {
        return aReadBuffer.get();
    }
    
    /**
     * Read 1 byte value from buffer
     * 
     * @param aLength           Integer length to read
     * @param aReadBuffer       Buffer to read
     * @param aDefaultValue     Default value
     * @return                  1 byte value
     */
    protected byte readByte(int        aLength,
                            ByteBuffer aReadBuffer,
                            byte       aDefaultValue)
    {
        byte sValue = aDefaultValue;
        
        switch (aLength)
        {
        case 1: sValue = (byte)aReadBuffer.get();      break;
        case 2: sValue = (byte)aReadBuffer.getShort(); break;
        case 4: sValue = (byte)aReadBuffer.getInt();   break;
        case 8: sValue = (byte)aReadBuffer.getLong();  break;
        
        default:
            {
                aReadBuffer.position(aReadBuffer.position() + aLength);
                break;
            }
        }
        
        return sValue;
    }

    /**
     * Read 2 byte value from buffer
     * 
     * @param aReadBuffer       Buffer to read 
     * @return                  2 byte value
     */
    protected short readShort(ByteBuffer aReadBuffer)
    {
        return aReadBuffer.getShort();
    }
    
    /**
     * Read 2 byte value from buffer
     * 
     * @param aLength           Integer length to read
     * @param aReadBuffer       Buffer to read
     * @param aDefaultValue     Default value
     * @return                  2 byte value
     */
    protected short readShort(int        aLength,
                              ByteBuffer aReadBuffer,
                              short      aDefaultValue)
    {
        short sValue = aDefaultValue;
        
        switch (aLength)
        {
        case 1: sValue = (short)aReadBuffer.get();      break;
        case 2: sValue = (short)aReadBuffer.getShort(); break;
        case 4: sValue = (short)aReadBuffer.getInt();   break;
        case 8: sValue = (short)aReadBuffer.getLong();  break;
        
        default:
            {
                aReadBuffer.position(aReadBuffer.position() + aLength);
                break;
            }
        }
        
        return sValue;
    }

    /**
     * Read 4 byte value from buffer
     * 
     * @param aReadBuffer       Buffer to read
     * @return                  4 byte value
     */
    protected int readInt(ByteBuffer aReadBuffer)
    {
        return aReadBuffer.getInt();
    }
    
    /**
     * Read 4 byte value from buffer
     * 
     * @param aLength           Integer length to read
     * @param aReadBuffer       Buffer to read
     * @param aDefaultValue     Default value
     * @return                  4 byte value
     */
    protected int readInt(int        aLength,
                          ByteBuffer aReadBuffer,
                          int        aDefaultValue)
    {
        int sValue = aDefaultValue;
        
        switch (aLength)
        {
        case 1: sValue = (int)aReadBuffer.get();      break;
        case 2: sValue = (int)aReadBuffer.getShort(); break;
        case 4: sValue = (int)aReadBuffer.getInt();   break;
        case 8: sValue = (int)aReadBuffer.getLong();  break;
        
        default:
            {
                aReadBuffer.position(aReadBuffer.position() + aLength);
                break;
            }
        }
        
        return sValue;
    }

    /**
     * Read 8 byte value from buffer
     * 
     * @param aReadBuffer       Buffer to read
     * @return                  8 byte value
     */
    protected long readLong(ByteBuffer aReadBuffer)
    {
        return aReadBuffer.getLong();
    }
    
    /**
     * Read 8 byte value from buffer
     * 
     * @param aLength           Integer length to read
     * @param aReadBuffer       Buffer to read
     * @param aDefaultValue     Default value
     * @return                  8 byte value
     */
    protected long readLong(int        aLength,
                            ByteBuffer aReadBuffer,
                            long       aDefaultValue)
    {
        long sValue = aDefaultValue;
        
        switch (aLength)
        {
        case 1: sValue = (long)aReadBuffer.get();      break;
        case 2: sValue = (long)aReadBuffer.getShort(); break;
        case 4: sValue = (long)aReadBuffer.getInt();   break;
        case 8: sValue = (long)aReadBuffer.getLong();  break;
        
        default:
            {
                aReadBuffer.position(aReadBuffer.position() + aLength);
                break;
            }
        }
        
        return sValue;
    }
    
    /**
     * Read variable length string
     * 
     * @param aReadBuffer       Buffer to read
     * @return                  String class instance
     */
    protected String readVariableString(ByteBuffer aReadBuffer)
    {
        int sLength = aReadBuffer.getInt();
        if (sLength == 0)
            return null;
        
        byte[] sStringBinary = new byte[sLength];
        aReadBuffer.get(sStringBinary, 0, sLength);

        String sString = new String(sStringBinary);
        
        return sString;
    }

    /**
     * Read variable length string by database character set of ALTIBASE server
     * 
     * @param aReadBuffer       Read to buffer
     * @param aByDBCharSet      Database character set of ALTIBASE server
     * @return                  String class instance
     */
    protected String readVariableString(ByteBuffer aReadBuffer, boolean aByDBCharSet)
    {
        int sLength = aReadBuffer.getInt();
        if (sLength == 0)
            return null;
        
        byte[] sStringBinary = new byte[sLength];
        aReadBuffer.get(sStringBinary, 0, sLength);

        String sString = null;

        if (aByDBCharSet == true)
        {
            String sDBCharSetName = getDBCharSetName();
            if (sDBCharSetName != null)
            {
                try
                {
                    sString = new String(sStringBinary, sDBCharSetName);
                }
                catch (Exception e)
                {
                    TraceLogger.getInstance().log(e);
                }
            }
        }
        
        if (sString == null)
        {
            sString = new String(sStringBinary);
        }
        
        return sString;
    }  
    
    protected AldpXid readAldpXid( ByteBuffer aReadBuffer )
    {
        return new AldpXid( aReadBuffer );
    }  
    
}
