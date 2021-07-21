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
import java.util.LinkedList;

import com.altibase.altilinker.adlp.*;
import com.altibase.altilinker.adlp.type.*;

public class RequestResultSetMetaResultOp extends ResultOperation
{
    public long         mRemoteStatementId = 0;
    public ColumnMeta[] mColumnMeta        = null;
    
    protected LinkedList mFragmentPosHintList = null;
    
    public static class ColumnMeta
    {
        public String mColumnName = null;
        public int    mColumnType = AltibaseSQLType.SQL_NONE;
        public int    mPrecision  = 0;
        public int    mScale      = 0;
        public int    mNullable   = Nullable.Unknown;
    }
    
    public RequestResultSetMetaResultOp()
    {
        super(OpId.RequestResultSetMetaResult, true, true);
    }
    
    public void close()
    {
        mFragmentPosHintList = null;
        
        super.close();
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

                if (mColumnMeta != null)
                {
                    writeInt(aOpPayload, mColumnMeta.length);

                    if (mColumnMeta.length >= 1)
                    {
                        mFragmentPosHintList = new LinkedList();
                    }
                    
                    int i;
                    for (i = 0; i < mColumnMeta.length; ++i)
                    {
                        writeVariableString(aOpPayload, mColumnMeta[i].mColumnName, true);
                        writeInt           (aOpPayload, mColumnMeta[i].mColumnType);
                        writeInt           (aOpPayload, mColumnMeta[i].mPrecision );
                        writeInt           (aOpPayload, mColumnMeta[i].mScale     );
                        writeInt           (aOpPayload, mColumnMeta[i].mNullable  );
                        
                        mFragmentPosHintList.add(new Integer(aOpPayload.position()));
                    }
                }
                else
                {
                    writeInt(aOpPayload, 0);
                }
            }
        }
        catch (BufferUnderflowException e)
        {
            return false;
        }

        return true;
    }

    protected LinkedList getFragmentPosHint()
    {
        return mFragmentPosHintList;
    }
}
