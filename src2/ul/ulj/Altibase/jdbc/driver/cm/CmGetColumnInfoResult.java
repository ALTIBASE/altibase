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

import Altibase.jdbc.driver.datatype.Column;

import java.util.ArrayList;
import java.util.List;

public class CmGetColumnInfoResult extends CmStatementIdResult
{
    static final byte MY_OP = CmOperation.DB_OP_GET_COLUMN_INFO_RESULT;
    
    private List<Column> mColumnList;

    public CmGetColumnInfoResult()
    {
    }
    
    public List<Column> getColumns()
    {
        return mColumnList;
    }
    
    protected byte getResultOp()
    {
        return MY_OP;
    }
    
    void setColumns(List<Column> aColumnList)
    {
        mColumnList = aColumnList;
    }
    
    void addColumn(Column aColumn)
    {
        if (mColumnList == null)
        {
            mColumnList = new ArrayList<Column>();
        }
        mColumnList.add(aColumn);
    }
}
