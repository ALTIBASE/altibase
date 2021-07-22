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
import java.util.LinkedList;

import com.altibase.altilinker.adlp.CommonHeader;
import com.altibase.altilinker.adlp.type.ResultCode;
import com.altibase.altilinker.util.TraceLogger;

/**
 * Result operation abstract class to be sent to DK module of ALTIBASE server
 */
public abstract class ResultOperation extends Operation
{
    /**
     * Constructor
     * 
     * @param aOperationId      Operation ID
     * @param aRemoteOperation  Whether remote operation or control operation
     * @param aFragmentable     Whether potential fragmentable operation or not
     */
    protected ResultOperation(byte    aOperationId,
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
    protected ResultOperation(byte aOperationId, boolean aRemoteOperation)
    {
        super(aOperationId, aRemoteOperation);
    }
    
    /**
     * Whether request operation or result operation
     * 
     * @return  true, if request operation. false, if result operation.
     */
    public boolean isRequestOperation()
    {
        return false;
    }
    
    /**
     * Get operation payload size
     * 
     * @return  Operation payload size
     */
    protected int getOpPayloadSize()
    {
        return 0;
    }

    /**
     * Write operation to common header and operation payload
     * 
     * @param aCommonHeader     Common header to be written
     * @param aOpPayload        Operation payload to be written
     * @return                  true, if this operation write successfully. false, if fail.
     */
    protected abstract boolean writeOperation(CommonHeader aCommonHeader,
                                              ByteBuffer   aOpPayload);
    
    /**
     * Get fragment position hint list
     * 
     * @return  Fragment position hint list
     */
    protected LinkedList getFragmentPosHint()
    {
        return null;
    }
    
    /**
     * Whether operation payload exist or does not exist
     * 
     * @return  true, if operation payload exist. false, if not exist.  
     */
    protected boolean canWriteData()
    {
        boolean sWriteData = true;
        
        switch (getResultCode())
        {
        case ResultCode.UnknownOperation:
        case ResultCode.UnknownSession:
        case ResultCode.WrongPacket:
            sWriteData = false;
            break;
            
        default:
            sWriteData = true;
            break;
        }
        
        return sWriteData;
    }
    
    /**
     * Fill common header from this operation
     * 
     * @param aCommonHeader     Common header to fill
     */
    protected void fillCommonHeader(CommonHeader aCommonHeader)
    {
        aCommonHeader.setOperationId(getOperationId());
        aCommonHeader.setOpSpecificParameter(getOpSpecificParameter());
        aCommonHeader.setXA( mXA );
        aCommonHeader.setTxBegin( mTxBegin );
        aCommonHeader.setSessionId(getSessionId());
        aCommonHeader.setDataLength(0);
    }
    
    /**
     * Write 1 byte value to buffer
     * 
     * @param aWriteBuffer      Buffer to be written
     * @param aValue            1 byte value
     */
    protected void writeByte(ByteBuffer aWriteBuffer, byte aValue)
    {
        aWriteBuffer.put(aValue);
    }

    /**
     * Write 2 byte value to buffer
     * 
     * @param aWriteBuffer      Buffer to be written
     * @param aValue            2 byte value
     */
    protected void writeShort(ByteBuffer aWriteBuffer, short aValue)
    {
        aWriteBuffer.putShort(aValue);
    }

    /**
     * Write 4 byte value to buffer
     * 
     * @param aWriteBuffer      Buffer to be written
     * @param aValue            4 byte value
     */
    protected void writeInt(ByteBuffer aWriteBuffer, int aValue)
    {
        aWriteBuffer.putInt(aValue);
    }

    /**
     * Write 8 byte value to buffer
     * 
     * @param aWriteBuffer      Buffer to be written
     * @param aValue            8 byte value
     */
    protected void writeLong(ByteBuffer aWriteBuffer, long aValue)
    {
        aWriteBuffer.putLong(aValue);
    }
    
    /**
     * Write variable length string to buffer
     * 
     * @param aWriteBuffer      Buffer to be written
     * @param aValue            String
     */
    protected void writeVariableString(ByteBuffer aWriteBuffer, String aValue)
    {
        if (aValue == null)
        {
            aWriteBuffer.putInt(0);
        }
        else
        {
            byte[] sStringBinary = getBytesFromString(aValue, false);
    
            aWriteBuffer.putInt(sStringBinary.length);
            
            if (sStringBinary.length > 0)
            {
                aWriteBuffer.put(sStringBinary);
            }
        }
    }

    /**
     * Write variable length string to buffer by database character set of ALTIBASE server
     * 
     * @param aWriteBuffer      Buffer to be written
     * @param aValue            String
     * @param aByDBCharSet      Database character set of ALTIBASE server
     */
    protected void writeVariableString(ByteBuffer aWriteBuffer,
                                       String     aValue,
                                       boolean    aByDBCharSet)
    {
        if (aValue == null)
        {
            aWriteBuffer.putInt(0);
        }
        else
        {
            byte[] sStringBinary = getBytesFromString(aValue, aByDBCharSet);

            aWriteBuffer.putInt(sStringBinary.length);
            
            if (sStringBinary.length > 0)
            {
                aWriteBuffer.put(sStringBinary);
            }
        }
    }
    
    /**
     * Write fixed length string
     * 
     * @param aWriteBuffer      Buffer to be written
     * @param aValue            String
     * @param aFixedLength      Fixed length
     * @param aWriteLength      Write length of string
     * @param aLengthLessChar   Character to fill lack of fixed length string
     */
    protected void writeFixedString(ByteBuffer aWriteBuffer,
                                    String     aValue,
                                    int        aFixedLength,
                                    boolean    aWriteLength,
                                    char       aLengthLessChar)
    {
        if (aFixedLength <= 0)
        {
            return;
        }

        String sValue = aValue;
        
        if (aWriteLength == true)
        {
            aWriteBuffer.putInt(aFixedLength);
        }
        
        if (sValue == null)
        {
            sValue = new String();
            
            int i;
            for (i = 0; i < aFixedLength; ++i)
            {
                sValue += aLengthLessChar;
            }
        }
        
        byte[] sStringBinary = getBytesFromString(sValue, false);

        if (sStringBinary.length < aFixedLength)
        {
            int i;
            int sAddLength = aFixedLength - sStringBinary.length;
            
            for (i = 0; i < sAddLength; ++i)
            {
                sValue = aLengthLessChar + sValue;
            }

            sStringBinary = getBytesFromString(sValue, false);
        }
        
        aWriteBuffer.put(sStringBinary, 0, aFixedLength);
    }

    /**
     * Write fixed length string by database character set of ALTIBASE server
     * 
     * @param aWriteBuffer      Buffer to be written
     * @param aValue            String
     * @param aFixedLength      Fixed length
     * @param aWriteLength      Write length of string
     * @param aLengthLessChar   Character to fill lack of fixed length string
     * @param aByDBCharSet      Database chracter set of ALTIBASE server
     */
    protected void writeFixedString(ByteBuffer aWriteBuffer,
                                    String     aValue,
                                    int        aFixedLength,
                                    boolean    aWriteLength,
                                    char       aLengthLessChar,
                                    boolean    aByDBCharSet)
    {
        if (aFixedLength <= 0)
        {
            return;
        }

        String sValue = aValue;

        if (aWriteLength == true)
        {
            aWriteBuffer.putInt(aFixedLength);
        }

        if (sValue == null)
        {
            sValue = new String();

            int i;
            for (i = 0; i < aFixedLength; ++i)
            {
                sValue += aLengthLessChar;
            }
        }

        byte[] sStringBinary = getBytesFromString(sValue, aByDBCharSet);

        if (sStringBinary.length < aFixedLength)
        {
            int i;
            int sAddLength = aFixedLength - sStringBinary.length;

            for (i = 0; i < sAddLength; ++i)
            {
                sValue = aLengthLessChar + sValue;
            }

            sStringBinary = getBytesFromString(sValue, aByDBCharSet);
        }

        aWriteBuffer.put(sStringBinary, 0, aFixedLength);
    }
    
    /**
     * Get bytes from string by database character set of ALTIBASE server
     * 
     * @param aString           Input string
     * @param aByDBCharSet      Database character set of ALTIBASE server
     * @return                  Byte array
     */
    protected byte[] getBytesFromString(String aString, boolean aByDBCharSet)
    {        
        byte[] sStringBinary = null;
        
        if (aByDBCharSet == true)
        {
            String sDBCharSetName = getDBCharSetName();
            if (sDBCharSetName != null)
            {
                try
                {
                    sStringBinary = aString.getBytes(sDBCharSetName);
                }
                catch (Exception e)
                {
                    TraceLogger.getInstance().log(e);
                }
            }
        }

        if (sStringBinary == null)
        {
            sStringBinary = aString.getBytes();
        }
        
        return sStringBinary;
    }
    
    /**
     * Write byte buffer
     * 
     * @param aWriteBuffer      Byte buffer to be written
     * @param aValue            Buffer to write
     */
    protected void writeBuffer(ByteBuffer aWriteBuffer, ByteBuffer aValue)
    {
        if (aValue == null)
            return;

        aValue.position(0);
        
        aWriteBuffer.put(aValue);
    }
}
