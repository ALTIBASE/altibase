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

import java.util.LinkedList;

import javax.transaction.xa.Xid;

public class CmXidResult extends CmResult
{
    static final byte MY_OP = CmOperation.DB_OP_XA_XID_RESULT;
    
    private LinkedList mXids = new LinkedList();
    
    public CmXidResult()
    {
    }
    
    public Xid[] getXids()
    {
        return (Xid[])mXids.toArray(new Xid[0]);
    }
    
    protected byte getResultOp()
    {
        return MY_OP;
    }

    void addXid(Xid aXid)
    {
        mXids.add(aXid);
    }
    
    void clearXids()
    {
        mXids.clear();
    }
}
