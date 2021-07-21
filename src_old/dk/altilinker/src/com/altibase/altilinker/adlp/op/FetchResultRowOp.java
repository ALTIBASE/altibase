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
 
package com.altibase.altilinker.adlp.op;

import java.nio.BufferUnderflowException;
import java.nio.ByteBuffer;

import com.altibase.altilinker.adlp.*;

public class FetchResultRowOp extends ResultOperation
{
    public  long       mRemoteStatementId       = 0;
    public  ByteBuffer mRowDataBuffer           = null;

    public  int        mRecordCount             = 0;
    private int        mLastRecordBeginPosition = 0;
    private int        mColumnCount             = 0;
    
    private static FetchRowDataBufferPool mBufferPool =
                                           FetchRowDataBufferPool.getInstance();

    public FetchResultRowOp()
    {
        super(OpId.FetchResultRow, true, true);
    }
    
    public void close()
    {
        freeRowDataBuffer();
    }
    
    public void allocateRowDataBuffer()
    {
        if (mRowDataBuffer == null)
        {
            mRowDataBuffer = mBufferPool.allocate();
        }
        else
        {
            mRowDataBuffer.clear();
        }
    }

    public void freeRowDataBuffer()
    {
        if (mRowDataBuffer != null)
        {
            mBufferPool.free(mRowDataBuffer);
            mRowDataBuffer = null;
        }
        
        mRecordCount             = 0;
        mLastRecordBeginPosition = 0;
        mColumnCount             = 0;
    }
    
    public int getRecordCount()
    {
        return mRecordCount;
    }
    
    public void increaseRecordCount()
    {
        ++mRecordCount;
    }
    
    public void writeRecordCount()
    {
        int sLastPosition = mRowDataBuffer.position();

        mRowDataBuffer.position(0);
        mRowDataBuffer.putInt(mRecordCount);
        
        if (sLastPosition != 0)
        {
            mRowDataBuffer.position(sLastPosition);
        }
    }
    
    public void setRecordBegin()
    {
        mLastRecordBeginPosition = mRowDataBuffer.position();
    }
    
    public boolean isSetRecordBegin()
    {
        return (mLastRecordBeginPosition != 0) ? true : false;
    }
    
    public void resetColumnCount()
    {
        mColumnCount = 0;
    }
    
    public void increaseColumnCount()
    {
        ++mColumnCount;
    }
    
    public void writeColumnCount()
    {
        int sLastPosition = mRowDataBuffer.position();
        
        mRowDataBuffer.position(mLastRecordBeginPosition);
        mRowDataBuffer.putInt(mColumnCount);
        
        if (sLastPosition != mLastRecordBeginPosition)
        {
            mRowDataBuffer.position(sLastPosition);
        }
    }
    
    public boolean isRecordWriteCompleted()
    {
        if (mRowDataBuffer.position() != mLastRecordBeginPosition)
        {
            return false;
        }
        
        return true;
    }
    
    public void moveLastRecordTo(FetchResultRowOp aNextFetchResultRowOp)
    {
        // get piece size of last record
        int sLastRecordPieceSize = mRowDataBuffer.position() - mLastRecordBeginPosition;

        // move to last record position
        mRowDataBuffer.position(mLastRecordBeginPosition);

        // get piece buffer of last record
        byte[] sLastRecordPiece = new byte[sLastRecordPieceSize];
        mRowDataBuffer.get(sLastRecordPiece);
        
        // set last buffer position
        mRowDataBuffer.position(mLastRecordBeginPosition);


        // write record count = 0 (4bytes)
        aNextFetchResultRowOp.writeRecordCount();

        // memorize record begin position
        aNextFetchResultRowOp.setRecordBegin();

        // write piece buffer of the record
        aNextFetchResultRowOp.mRowDataBuffer.put(sLastRecordPiece);

        // read column count
        int sLastPosition = aNextFetchResultRowOp.mRowDataBuffer.position();
        
        aNextFetchResultRowOp.mRowDataBuffer.position(aNextFetchResultRowOp.mLastRecordBeginPosition);
        aNextFetchResultRowOp.mColumnCount = aNextFetchResultRowOp.mRowDataBuffer.getInt();
        
        aNextFetchResultRowOp.mRowDataBuffer.position(sLastPosition);
    }

    protected boolean writeOperation(CommonHeader aCommonHeader,
                                     ByteBuffer   aOpPayload)
    {
        fillCommonHeader(aCommonHeader);
        
        try
        {
            if (canWriteData() == true)
            {
                // write data
                writeLong(aOpPayload, mRemoteStatementId);

                if (mRowDataBuffer != null)
                {
                    int sRowDataSize = mRowDataBuffer.limit();
                    if (sRowDataSize > 0)
                    {
                        writeBuffer(aOpPayload, mRowDataBuffer);
                    }
                }
            }
        }
        catch (BufferUnderflowException e)
        {
            return false;
        }

        return true;
    }
}
