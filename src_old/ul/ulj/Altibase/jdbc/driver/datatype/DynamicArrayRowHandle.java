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

import java.sql.BatchUpdateException;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;

import Altibase.jdbc.driver.util.DynamicArray;
import Altibase.jdbc.driver.util.DynamicArrayCursor;

public class DynamicArrayRowHandle implements RowHandle
{
    protected DynamicArrayCursor mStoreCursor;
    protected DynamicArrayCursor mLoadCursor;
    private List<DynamicArray>   mArrays;
    private List<Column>         mColumns;
    private HashSet<Integer>     mHoleSet;
    private boolean              mIsPrepared;     // BUG-47460 prepared fetch인지 여부

    public DynamicArrayRowHandle()
    {
        mStoreCursor = new DynamicArrayCursor();
        mLoadCursor = new DynamicArrayCursor();
        mArrays = null;
        mIsPrepared = false;
    }

    public void setColumns(List<Column> aColumns)
    {
        mColumns = aColumns;
        if (mArrays == null || mArrays.size() != mColumns.size())
        {
            mArrays = new ArrayList<DynamicArray>(aColumns.size());
            for (Column sBindColumn : aColumns)
            {
                // BUG-47460 컬럼객체를 초기화하면서 DynaminArray를 미리 생성한다.
                DynamicArray sArray = sBindColumn.createTypedDynamicArray();
                sArray.setCursor(mStoreCursor, mLoadCursor);
                mArrays.add(sArray);
            }
        }
    }

    /*
     * store 관련 메소드
     *  - initToStore(): 캐시를 초기화하고 처음부터 store하기 위해 호출한다.
     *  - cacheSize(): 현재 캐시되어 있는 row의 개수를 구한다.
     *  - store(): row를 하나 캐시한다.
     */
    public void initToStore()
    {
        // BUG-47460 성능을 위해 direct execute일때만 Dynamic Array를 체크한다.
        // BUG-48280 dequeue 같은 경우 아직 fetch 결과가 없기 때문에 mColumns값이 null이 들어올 수 있다.
        if (!mIsPrepared && mColumns != null)
        {
            for (int i = 0; i < mColumns.size(); i++)
            {
                Column sBindColumn = mColumns.get(i);
                DynamicArray sArray = mArrays.get(i);
                if (!sBindColumn.isArrayCompatible(sArray))
                {
                    // 최초 column이 세팅된 후 array가 만들어지지 않았을 경우나
                    // array를 만들었는데 그 이후 컬럼 타입이 바뀐 경우
                    sArray = sBindColumn.createTypedDynamicArray();
                    sArray.setCursor(mStoreCursor, mLoadCursor);
                    mArrays.set(i, sArray);
                }
            }
        }
        mStoreCursor.setPosition(0);
        if (mHoleSet != null)
        {
            mHoleSet.clear();
        }
    }

    public int size()
    {
        return mStoreCursor.getPosition();
    }

    public void store()
    {
        // 첫번째 index는 버린다.
        // 0는 beforeFirst를 위한 index로 사용하기 위함이다.
        mStoreCursor.next();
        for (int i = 0; i < mColumns.size(); i++)
        {
            mColumns.get(i).storeTo(mArrays.get(i));
        }
    }

    public void increaseStoreCursor()
    {
        mStoreCursor.next();
    }

    public void storeLobResult4Batch() throws BatchUpdateException
    {
        mStoreCursor.next();
        mLoadCursor.next();
        for (int i = 0; i < mColumns.size(); i++)
        {
            Column sBindColumn = mColumns.get(i);
            DynamicArray sArray = mArrays.get(i);
            if (sBindColumn instanceof BlobLocatorColumn)
            {
                ((BlobLocatorColumn)sBindColumn).storeToLobArray(sArray);
            }
            else if (sBindColumn instanceof ClobLocatorColumn)
            {
                ((ClobLocatorColumn)sBindColumn).storeToLobArray(sArray);
            }
        }
    }

    public void update()
    {
        int sOldPos = mStoreCursor.getPosition();
        mStoreCursor.setPosition(mLoadCursor.getPosition());
        for (int i = 0; i < mColumns.size(); i++)
        {
            mColumns.get(i).storeTo(mArrays.get(i));
        }
        mStoreCursor.setPosition(sOldPos);
    }

    /*
     * row cursor position getter
     *  - getPosition(): 현재 row 위치를 구한다.
     *  - isBeforeFirst()
     *  - isAfterLast()
     *  - isFirst()
     *  - isLast()
     */
    public int getPosition()
    {
        return mLoadCursor.getPosition();
    }

    public boolean isBeforeFirst()
    {
        return mLoadCursor.isBeforeFirst();
    }

    public boolean isFirst()
    {
        return mLoadCursor.isFirst();
    }

    public boolean isLast()
    {
        return mLoadCursor.equals(mStoreCursor);
    }

    public boolean isAfterLast()
    {
        return !mStoreCursor.geThan(mLoadCursor);
    }

    /*
     * row cursor positioning 메소드
     *  - setPosition()
     *  - beforeFirst()
     *  - afterLast()
     *  - next()
     *  - previous()
     */
    public boolean setPosition(int aPos)
    {
        if (aPos <= 0)
        {
            mLoadCursor.setPosition(0);
            return false;
        }
        else if (aPos > mStoreCursor.getPosition())
        {
            mLoadCursor.setPosition(mStoreCursor.getPosition() + 1);
            return false;
        }
        else
        {
            mLoadCursor.setPosition(aPos);
            load();
            return true;
        }
    }

    public void beforeFirst()
    {
        mLoadCursor.setPosition(0);
    }

    public void afterLast()
    {
        mLoadCursor.setPosition(mStoreCursor.getPosition() + 1);
    }

    public boolean next()
    {
        if (isAfterLast())
        {
            return false;
        }

        do
        {
            mLoadCursor.next();
        } while (mHoleSet != null && mHoleSet.contains(mLoadCursor.getPosition()));

        return checkAndLoad();
    }

    public boolean previous()
    {
        if (isBeforeFirst())
        {
            return false;
        }

        do
        {
            mLoadCursor.previous();
        } while (mHoleSet != null && mHoleSet.contains(mLoadCursor.getPosition()));

        return checkAndLoad();
    }

    public void reload()
    {
        checkAndLoad();
    }

    private boolean checkAndLoad()
    {
        if (!mLoadCursor.isBeforeFirst() && mStoreCursor.geThan(mLoadCursor))
        {
            load();
            return true;
        }
        return false;
    }

    private void load()
    {
        for (int i = 0; i < mColumns.size(); i++)
        {
            mColumns.get(i).loadFrom(mArrays.get(i));
        }
    }

    /**
     * 현재 커서 위치의 Row를 지운다.
     */
    public void delete()
    {
        if (mHoleSet == null)
        {
            mHoleSet = new HashSet<Integer>();
        }
        mHoleSet.add(mLoadCursor.getPosition());
    }

    public void changeBindColumnType(int aIndex, Column aColumn, ColumnInfo aColumnInfo,
                                     byte aInOutType)
    {
        mColumns.set(aIndex, aColumn);
        mArrays.set(aIndex, aColumn.createTypedDynamicArray());
        mArrays.get(aIndex).setCursor(mStoreCursor, mLoadCursor);
        ColumnInfo sColumnInfo = (ColumnInfo)aColumnInfo.clone();
        sColumnInfo.modifyInOutType(aInOutType);
        mColumns.get(aIndex).getDefaultColumnInfo(sColumnInfo);
        sColumnInfo.addInType();
        mColumns.get(aIndex).setColumnInfo(sColumnInfo);
    }

    /**
     * 컬럼인덱스에 해당하는 DynamicArray를 리턴한다.
     * @param aIndex 컬럼인덱스
     * @return 컬럼인덱스에 해당하는 DynamicArray
     */
    public DynamicArray getDynamicArray(int aIndex)
    {
        return mArrays.get(aIndex);
    }

    public void setPrepared(boolean aIsPrepared)
    {
        mIsPrepared = aIsPrepared;
    }
}
