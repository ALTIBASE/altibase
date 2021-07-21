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

import javax.transaction.xa.XAException;

public class CmXAResult extends CmResult
{
    static final byte MY_OP = CmOperation.DB_OP_XA_RESULT;
    
    private int mResultValue;
    
    public CmXAResult()
    {
    }
    
    public int getResultValue()
    {
        return mResultValue;
    }

    public String getResultValueString()
    {
        switch (mResultValue)
        {
            case XAException.XAER_ASYNC   : return "XAER_ASYNC";
            case XAException.XAER_RMERR   : return "XAER_RMERR";
            case XAException.XAER_NOTA    : return "XAER_NOTA";
            case XAException.XAER_INVAL   : return "XAER_INVAL";
            case XAException.XAER_PROTO   : return "XAER_PROTO";
            case XAException.XAER_RMFAIL  : return "XAER_RMFAIL";
            case XAException.XAER_DUPID   : return "XAER_DUPID";
            case XAException.XAER_OUTSIDE : return "XAER_OUTSIDE";
            default                       : return "XAER_UNKNOWN";
        }
    }

    protected byte getResultOp()
    {
        return MY_OP;
    }

    void setResultValue(int aValue)
    {
        mResultValue = aValue;
    }
}
