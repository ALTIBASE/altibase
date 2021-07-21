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

package Altibase.jdbc.driver;

import java.sql.SQLException;
import java.util.List;

import Altibase.jdbc.driver.datatype.Column;
import Altibase.jdbc.driver.datatype.RowHandle;
import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;

/** 
 * ping 쿼리에 의해서 생성되는 ResultSet</br>
 * SELECT 1 의 결과와 동일한 row를 가진다. 
 * @author yjpark
 *
 */
public class AltibaseLightWeightResultSet extends AltibaseReadableResultSet
{
    private final List<Column> mColumns;
    private final int          mType;
    private boolean            mResultExist;
    
    public AltibaseLightWeightResultSet(AltibaseStatement aStatement, List<Column> aColumns,
                                        int aResultSetType)
    {
        mStatement = aStatement;
        mColumns = aColumns;
        mType = aResultSetType;
        mResultExist = true;
    }
    
    public boolean next() throws SQLException
    {
        throwErrorForClosed();
        boolean sResultExist = this.mResultExist;
        this.mResultExist = false;
        return sResultExist;
    }

    public boolean isBeforeFirst() throws SQLException
    {
        throwErrorForClosed();
        return false;
    }

    public boolean isAfterLast() throws SQLException
    {
        throwErrorForClosed();
        return false;
    }

    public boolean isFirst() throws SQLException
    {
        throwErrorForClosed();
        return false;
    }

    public boolean isLast() throws SQLException
    {
        throwErrorForClosed();
        return false;
    }

    public void beforeFirst() throws SQLException
    {
        throwErrorForForwardOnly();
    }

    public void afterLast() throws SQLException
    {
        throwErrorForForwardOnly();
    }

    public int getRow() throws SQLException
    {
        throwErrorForClosed();
        return 0;
    }

    public boolean absolute(int row) throws SQLException
    {
        throwErrorForForwardOnly();
        return false;
    }

    public boolean previous() throws SQLException
    {
        throwErrorForForwardOnly();
        return false;
    }

    public int getType() throws SQLException
    {
        throwErrorForClosed();
        return mType;
    }

    public void refreshRow() throws SQLException
    {
        throwErrorForForwardOnly();
    }

    protected RowHandle rowHandle()
    {
        Error.throwInternalError(ErrorDef.ILLEGAL_ACCESS);
        return null;
    }

    int size()
    {
        return 1;
    }

    protected List<Column> getTargetColumns()
    {
        return mColumns;
    }
}
