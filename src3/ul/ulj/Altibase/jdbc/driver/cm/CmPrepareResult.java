/*
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

package Altibase.jdbc.driver.cm;

public class CmPrepareResult extends CmStatementIdResult
{
    // BUG-42424 AltibasePreparedStatement사용하기때문에 scope변경 
    public static final byte MY_OP = CmOperation.DB_OP_PREPARE_V3_RESULT;
    
    public static final int STATEMENT_TYPE_DDL               = 0x00000000;
    public static final int STATEMENT_TYPE_DML               = 0x00010000;
    public static final int STATEMENT_TYPE_DCL               = 0x00020000;
    public static final int STATEMENT_TYPE_EAP               = 0x00030000;
    public static final int STATEMENT_TYPE_SP                = 0x00040000;
    public static final int STATEMENT_TYPE_DB                = 0x00050000;

    public static final int STATEMENT_TYPE_SELECT            = STATEMENT_TYPE_DML + 0;
    public static final int STATEMENT_TYPE_LOCK_TABLE        = STATEMENT_TYPE_DML + 1;
    public static final int STATEMENT_TYPE_INSERT            = STATEMENT_TYPE_DML + 2;
    public static final int STATEMENT_TYPE_UPDATE            = STATEMENT_TYPE_DML + 3;
    public static final int STATEMENT_TYPE_DELETE            = STATEMENT_TYPE_DML + 4;
    public static final int STATEMENT_TYPE_CHECK_SEQUENCE    = STATEMENT_TYPE_DML + 5;
    public static final int STATEMENT_TYPE_MOVE              = STATEMENT_TYPE_DML + 6;
    public static final int STATEMENT_TYPE_ENQUEUE           = STATEMENT_TYPE_DML + 7;
    public static final int STATEMENT_TYPE_DEQUEUE           = STATEMENT_TYPE_DML + 8;
    public static final int STATEMENT_TYPE_SELECT_FOR_UPDATE = STATEMENT_TYPE_DML + 9;
    public static final int STATEMENT_TYPE_SELECT_FOR_FIXED_TABLE = STATEMENT_TYPE_DCL + 49;

    public static final int STATEMENT_TYPE_FUNCTION          = STATEMENT_TYPE_SP + 0;
    public static final int STATEMENT_TYPE_PROCEDURE         = STATEMENT_TYPE_SP + 1;

    private int     mStatementType;
    private int     mParameterCount;
    private int     mResultSetCount;
    private boolean mIsPrepared;    // BUG-42424 데이터가 서버로부터 전달되었는지 여부를 나타낸다.
    
    public CmPrepareResult()
    {
    }
    
    public int getStatementType()
    {
        return mStatementType;
    }
    
    public int getParameterCount()
    {
        return mParameterCount;
    }
    
    public int getResultSetCount()
    {
        return mResultSetCount;
    }

    public boolean isSelectStatement()
    {
        switch (mStatementType)
        {
            case STATEMENT_TYPE_SELECT:
            case STATEMENT_TYPE_SELECT_FOR_UPDATE:
            case STATEMENT_TYPE_SELECT_FOR_FIXED_TABLE:
                return true;
            default:
                return false;
        }
    }

    public boolean isStoredProcedureStatement()
    {
        switch (mStatementType)
        {
            case STATEMENT_TYPE_FUNCTION:
            case STATEMENT_TYPE_PROCEDURE:
                return true;
            default:
                return false;
        }
    }

    public boolean isDequeueStatement()
    {
        return mStatementType == STATEMENT_TYPE_DEQUEUE;
    }

    public boolean isInsertStatement()
    {
        return (mStatementType == STATEMENT_TYPE_INSERT);
    }

    protected byte getResultOp()
    {
        return MY_OP;
    }

    public void setStatementType(int aStatementType)
    {
        mStatementType = aStatementType;
    }
    
    public void setParameterCount(int aParameterCount)
    {
        mParameterCount = aParameterCount;
    }
    
    public void setResultSetCount(int aResultSetCount)
    {
        mResultSetCount = aResultSetCount;
    }

    public boolean isPrepared()
    {
        return mIsPrepared;
    }

    public void setPrepared(boolean aIsPrepared)
    {
        mIsPrepared = aIsPrepared;
    }

    @Override
    public String toString()
    {
        final StringBuilder sSb = new StringBuilder("CmPrepareResult{");
        sSb.append("mStatementType=").append(mStatementType);
        sSb.append(", mParameterCount=").append(mParameterCount);
        sSb.append(", mResultSetCount=").append(mResultSetCount);
        sSb.append(", mIsPrepared=").append(mIsPrepared);
        sSb.append('}');
        return sSb.toString();
    }
}
