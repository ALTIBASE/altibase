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
import java.util.HashMap;
import java.util.Map;

import Altibase.jdbc.driver.cm.CmFetchResult;
import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;

final class AltibaseKeySetDrivenResultSet extends AltibaseScrollInsensitiveResultSet
{
    private int                mRowIdColumnIndex;
    private Map<Long, Integer> mKeyMap;
    private int                mNextFillIndex;

    AltibaseKeySetDrivenResultSet(AltibaseStatement aStatement, CmFetchResult aFetchResult, int aFetchCount)
    {
        init(aStatement, aFetchResult, aFetchCount);
    }

    /**
     * Keyset-driven으로 생성된 Result Set을 감싼다.
     * <p>
     * base result set은 반드시 Keyset-driven(scroll-sensitive 또는 updatable)로 연 것이어야 하며,
     * key column은 SQL BIGINT 값으로 target절 가장 마지막에 있어야 한다.
     *
     * @param aBaseResultSet Keyset-driven으로 생성된 Result Set
     */
    AltibaseKeySetDrivenResultSet(AltibaseScrollInsensitiveResultSet aBaseResultSet)
    {
        init(aBaseResultSet.mStatement, aBaseResultSet.mFetchResult, aBaseResultSet.mFetchSize);
    }

    /**
     * 객체만 생성할 수 있게 해주는 생성자.
     * 이 생성자를 통해 만든 객체는 반드시 사용 전에 다음 함수를 통해 초기화를 해야한다:
     * {@link #init(AltibaseStatement, CmFetchResult, int)}
     * <p>
     * 나중에 초기화하는 방법을 사용할 수 있게 하는것은 생성이 잦은 경우 그 비용을 줄이려는 것이다.
     */
    AltibaseKeySetDrivenResultSet()
    {
    }

    void init(AltibaseStatement aStatement, CmFetchResult aFetchResult, int aFetchCount)
    {
        mStatement = aStatement;
        mFetchResult = aFetchResult;
        mFetchSize = aFetchCount;

        mRowIdColumnIndex = getTargetColumnCount();
        if (mKeyMap == null)
        {
            mKeyMap = new HashMap<Long, Integer>();
        }
        else
        {
            mKeyMap.clear();
        }
        mNextFillIndex = 1;
    }

    public boolean absoluteByProwID(long sProwID) throws SQLException
    {
        throwErrorForClosed();

        ensureFillKeySetMap();

        Integer sRowPos = mKeyMap.get(sProwID);
        if (sRowPos == null)
        {
            return false;
        }
        return absolute(sRowPos);
    }

    private void ensureFillKeySetMap() throws SQLException
    {
        if (mNextFillIndex > rowHandle().size())
        {
            return;
        }

        int sOldPos = rowHandle().getPosition();
        rowHandle().setPosition(mNextFillIndex - 1);
        for (; mNextFillIndex <= rowHandle().size(); mNextFillIndex++)
        {
            boolean sNextResult = rowHandle().next();
            if (!sNextResult)
            {
                Error.throwInternalError(ErrorDef.INTERNAL_DATA_INTEGRITY_BROKEN);
            }
            long sProwID = getTargetColumn(mRowIdColumnIndex).getLong();
            mKeyMap.put(sProwID, mNextFillIndex);
        }
        rowHandle().setPosition(sOldPos);
    }
}
