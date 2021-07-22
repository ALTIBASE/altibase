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

import com.altibase.altilinker.adlp.op.OpId;

/**
 * Operation abstract class for ADLP protocol 
 */
public abstract class Operation
{
    private byte    mOperationId         = OpId.None;
    private byte    mOpSpecificParameter = 0;
    private int     mSessionId           = 0;

    private boolean mRemoteOperation     = false;
    private boolean mFragmentable        = false;
    
    protected static String mDBCharSetName = ""; 
    
    private long    mSeqNo = 0;
    
    protected boolean mXA                = false;
    protected boolean mTxBegin           = false;
    
    
    /**
     * Constructor
     * 
     * @param aOperationId     Operation ID
     * @param aRemoteOperation Whether remote operation or control operation
     * @param aFragmentable    Whether potential fragmentable operation or not
     */
    protected Operation(byte    aOperationId,
                        boolean aRemoteOperation,
                        boolean aFragmentable)
    {
        mOperationId     = aOperationId;
        mRemoteOperation = aRemoteOperation;
        mFragmentable    = aFragmentable;
        
    }
    
    protected Operation(byte    aOperationId,
                        boolean aRemoteOperation,
                        boolean aFragmentable,
                        boolean aXA,
                        boolean aTxBegin )
    {
        mOperationId     = aOperationId;
        mRemoteOperation = aRemoteOperation;
        mFragmentable    = aFragmentable;
        mXA              = aXA;
        mTxBegin         = aTxBegin;
    }

    /**
     * Constructor
     * 
     * @param aOperationId     Operation ID
     * @param aRemoteOperation Whether remote operation or control operation
     */
    protected Operation(byte aOperationId, boolean aRemoteOperation)
    {
        this(aOperationId, aRemoteOperation, false);
    }

    /**
     * Close for this instance
     */
    public void close()
    {
        // nothing to do
    }

    /**
     * Get operation ID of common header of ADLP protocol
     * 
     * @return  Operation ID
     */
    public byte getOperationId()
    {
        return mOperationId;
    }

    /**
     * Get result code in operation specific parameter field of common header of result operation
     * 
     * @return  Result code
     */
    public byte getResultCode()
    {
        return mOpSpecificParameter;
    }
    
    /**
     * Get operation specific parameter of common header of ADLP protocol
     * 
     * @return  Operation specific parameter
     */
    public byte getOpSpecificParameter()
    {
        return mOpSpecificParameter;
    }

    /**
     * Get session ID of common header of ADLP protocol
     * 
     * @return  Session ID
     */
    public int getSessionId()
    {
        return mSessionId;
    }
    
    /**
     * Set session ID of common header of ADLP protocol
     */
    public void setSessionId(int aSessionId)
    {
        mSessionId = aSessionId;
    }

    /**
     * Whether control operation or remote operation
     * 
     * @return  true, if this operation is control operation. false, if else.
     */
    public boolean isControlOperation()
    {
        return (mOperationId >= 0) ? true : false;
    }
    
    /**
     * Whether data operation or control operation
     * 
     * @return  true, if this operation is data operation. false, if else.
     */
    public boolean isDataOperation()
    {
        return (mOperationId < 0) ? true : false;
    }
    
    /**
     * Whether remote operation or control operation
     * 
     * @return  true, if this operation is remote operation. false, if else.
     */
    public boolean isRemoteOperation()
    {
        return (mRemoteOperation == true) ? true : false;
    }
    
    /**
     * Whether potential fragmentable operation or not
     * 
     * @return  true, if this operation is potential fragmentable operation. false, if else.
     */
    public boolean isFragmentable()
    {
        return mFragmentable;
    }
    
    /**
     * Set result code in operation specific parameter field of common header of ADLP protocol
     */
    public void setResultCode(byte aResultCode)
    {
        mOpSpecificParameter = aResultCode;
    }
    
    /**
     * Set operation specific parameter of common header of ADLP protocol
     */
    public void setOpSpecificParameter(byte aOpSpecificParameter)
    {
        mOpSpecificParameter = aOpSpecificParameter;
    }

    /**
     * Get database character set name of ALTIBASE server
     * 
     * @return  Database character set name of ALTIBASE server
     */
    public static String getDBCharSetName()
    {
        return mDBCharSetName;
    }

    /**
     * Set database character set name of ALTIBASE server
     */
    public static void setDBCharSetName(String aDBCharSetName)
    {
        mDBCharSetName = aDBCharSetName;
    }

    /**
     * Whether request operation or result operation
     * 
     * @return  true, if this operation is request operation. false, if else. 
     */
    public abstract boolean isRequestOperation();
    
    /**
     *  BUG-39201
     *  Set SeqNo
     *  
     *  @param aSeqNo   SeqNo
     */
    public void setSeqNo( long aSeqNo )
    {
        mSeqNo = aSeqNo;
    }
    
    /**
     *  BUG-39201
     *  Get SeqNo
     *  
     *  @return SeqNo
     */
    public long getSeqNo()
    {
        return mSeqNo;
    }      
    
    public byte getFlags()
    {
        byte aRetValue = 0;
        
        return aRetValue;
    }
    
    public boolean isTxBegin()
    {
        return mTxBegin;
    }

    public void setXA( boolean aIsXA )
    {
        mXA = aIsXA;
    }
    
    public boolean isXA()
    {
        return mXA;
    }
}
