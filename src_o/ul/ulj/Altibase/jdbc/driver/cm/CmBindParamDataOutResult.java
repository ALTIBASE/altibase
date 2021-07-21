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

import java.sql.BatchUpdateException;
import java.util.List;

import Altibase.jdbc.driver.datatype.Column;
import Altibase.jdbc.driver.datatype.DynamicArrayRowHandle;

public class CmBindParamDataOutResult extends CmStatementIdResult
{
    static final byte MY_OP = CmOperation.DB_OP_PARAM_DATA_OUT_LIST;

    private List<Column>          mBindParams;
    private DynamicArrayRowHandle mRowHandle4Blob;

    public CmBindParamDataOutResult()
    {
    }

    void setBindParams(List<Column> aColumns)
    {
        mBindParams = aColumns;
    }

    public void setRowHandle(DynamicArrayRowHandle aRowHandle)
    {
        mRowHandle4Blob = aRowHandle;
    }
    
    void storeLobDataAtBatch() throws BatchUpdateException
    {
        if(mRowHandle4Blob != null)
        {
            mRowHandle4Blob.storeLobResult4Batch();
        }
    }
    
    protected byte getResultOp()
    {
        return MY_OP;
    }

    List<Column> getBindParams()
    {
        return mBindParams;
    }
}
