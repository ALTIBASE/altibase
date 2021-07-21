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

package Altibase.jdbc.driver.datatype;

import Altibase.jdbc.driver.util.DynamicArray;

import java.sql.SQLException;
import java.util.List;

/**
 *  Forward Only이고 ResultSet이 한개일때 Row 데이터를 저장하기 위한 클래스.<br>
 *  RowHandle처럼 DynamicArray를 사용하지 않고 java.util.ArrayList를 이용해 컬럼 데이터를 저장한다.
 */
public class ArrayListRowHandle implements RowHandle
{
    private List<Column>       mColumns;
    private int                mStoreIndex;
    private int                mLoadIndex;

    public int size()
    {
        return mStoreIndex;
    }

    public void store() throws SQLException
    {
        mStoreIndex++;
        for (Column mColumn : mColumns)
        {
            mColumn.storeTo();
        }
    }

    public void initToStore()
    {
        if (mColumns != null)
        {
            for (Column mColumn : mColumns)
            {
                mColumn.clearValues();
            }
        }
        mStoreIndex = 0;
    }

    public void setColumns(List<Column> aColumns)
    {
        this.mColumns = aColumns;
    }

    public void changeBindColumnType(int aIndex, Column aColumn, ColumnInfo aColumnInfo, byte aInOutType)
    {
        mColumns.set(aIndex, aColumn);
        ColumnInfo sColumnInfo = (ColumnInfo)aColumnInfo.clone();
        sColumnInfo.modifyInOutType(aInOutType);
        mColumns.get(aIndex).getDefaultColumnInfo(sColumnInfo);
        sColumnInfo.addInType();
        mColumns.get(aIndex).setColumnInfo(sColumnInfo);
    }

    public void increaseStoreCursor()
    {
        mStoreIndex++;
    }

    public void beforeFirst()
    {
        mLoadIndex = 0;
    }

    public void setPrepared(boolean aIsPrepared)
    {
        // do not implement
    }

    public DynamicArray getDynamicArray(int aIndex)
    {
        return null;
    }

    public boolean next()
    {
        if (mLoadIndex >= mStoreIndex)
        {
            mLoadIndex++;
            return false;
        }

        for (Column mColumn : mColumns)
        {
            mColumn.loadFrom(mLoadIndex);
        }

        mLoadIndex++;

        return true;
    }

    public boolean isAfterLast()
    {
        return (mLoadIndex > mStoreIndex);
    }

    public int getPosition()
    {
        return mLoadIndex;
    }

    public boolean isBeforeFirst()
    {
        return mLoadIndex == 0;
    }

    public boolean isFirst()
    {
        return mLoadIndex == 1;
    }

    public boolean isLast()
    {
        return mLoadIndex == mStoreIndex;
    }

    public boolean setPosition(int aIndex)
    {
        return false;
    }

    public void afterLast()
    {
        // do not implement
    }

    public boolean previous()
    {
        return false;
    }

    public void delete()
    {
        // do not implement
    }

    public void reload()
    {
        // do not implement
    }

    public void update()
    {
        // do not implement
    }
}
