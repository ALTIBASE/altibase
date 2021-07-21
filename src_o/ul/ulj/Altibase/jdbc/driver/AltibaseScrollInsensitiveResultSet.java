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

import Altibase.jdbc.driver.cm.CmFetchResult;
import Altibase.jdbc.driver.cm.CmProtocol;
import Altibase.jdbc.driver.cm.CmProtocolContextDirExec;
import Altibase.jdbc.driver.datatype.RowHandle;
import Altibase.jdbc.driver.ex.Error;

public class AltibaseScrollInsensitiveResultSet extends AltibaseScrollableResultSet
{
    protected CmFetchResult            mFetchResult;

    AltibaseScrollInsensitiveResultSet(AltibaseStatement aStatement, CmProtocolContextDirExec aContext, int aFetchCount)
    {
        mContext = aContext;
        mStatement = aStatement;
        mFetchResult = aContext.getFetchResult();
        mFetchSize = aFetchCount;
    }

    /**
     * 하위 클래스 생성시 중복 작업을 피하기 위한 생성자.
     * 이 생성자를 쓸 때는 protected 멤버를 하위클래스에서 직접 초기화 해줘야 한다.
     */
    protected AltibaseScrollInsensitiveResultSet()
    {
    }

    public void close() throws SQLException
    {
        if (isClosed())
        {
            return;
        }

        int sResultSetCount = mContext.getPrepareResult().getResultSetCount();
        
        if(mStatement instanceof AltibasePreparedStatement)
        {
            if(sResultSetCount > 1)
            {
                if(mStatement.mCurrentResultIndex == (sResultSetCount - 1))
                {
                    ((AltibasePreparedStatement)mStatement).closeAllResultSet();
                    closeResultSetCursor();
                }
                else
                {
                    ((AltibasePreparedStatement)mStatement).closeCurrentResultSet();
                }
            }
            else
            {
                ((AltibasePreparedStatement)mStatement).closeCurrentResultSet();
                closeResultSetCursor();
            }
        }
        else
        {
            closeResultSetCursor();
        }
        
        super.close();
    }

    protected RowHandle rowHandle()
    {
        return mFetchResult.rowHandle();
    }

    protected List getTargetColumns()
    {
        return mFetchResult.getColumns();
    }

    public boolean isAfterLast() throws SQLException
    {
        throwErrorForClosed();
        return !mFetchResult.fetchRemains() && mFetchResult.rowHandle().isAfterLast();
    }

    public int getRow() throws SQLException
    {
        if (isBeforeFirst() || isAfterLast())
        {
            return 0;
        }
        return mFetchResult.rowHandle().getPosition();
    }

    public boolean isBeforeFirst() throws SQLException
    {
        throwErrorForClosed();
        return mFetchResult.rowHandle().isBeforeFirst();
    }

    public boolean isFirst() throws SQLException
    {
        throwErrorForClosed();
        return mFetchResult.rowHandle().isFirst();
    }

    public boolean isLast() throws SQLException
    {
        throwErrorForClosed();
        if (mFetchResult.fetchRemains())
        {
            // fetch할게 남아 있으면 최소한 한 row 이상은 있다고 가정한 것이다.
            // 만약 fetch할게 남아 있다고 했지만 실제 fetchNext를 요청했을 때
            // row가 하나도 안오고 fetch end가 올 수 있다면 이 코드는 버그이다.
            return false;
        }
        else
        {
            return mFetchResult.rowHandle().isLast();
        }
    }

    public boolean absolute(int aRow) throws SQLException
    {
        throwErrorForClosed();
        cursorMoved();
        boolean sResult;
        if (aRow < 0)
        {
            fetchAll();
            // aRow가 -1이면 last 위치를 가리킨다.
            aRow = mFetchResult.rowHandle().size() + 1 + aRow;
            sResult = mFetchResult.rowHandle().setPosition(aRow);
        }
        else
        {
            if (checkCache(aRow))
            {
                sResult = mFetchResult.rowHandle().setPosition(aRow);
            }
            else
            {
                mFetchResult.rowHandle().afterLast();
                sResult = false;
            }
        }
        return sResult;
    }

    public void afterLast() throws SQLException
    {
        throwErrorForClosed();
        cursorMoved();
        fetchAll();
        mFetchResult.rowHandle().afterLast();
    }

    public void beforeFirst() throws SQLException
    {
        throwErrorForClosed();
        cursorMoved();
        mFetchResult.rowHandle().beforeFirst();
    }

    public boolean previous() throws SQLException
    {
        throwErrorForClosed();
        cursorMoved();
        return mFetchResult.rowHandle().previous();
    }

    public boolean next() throws SQLException
    {
        throwErrorForClosed();
        cursorMoved();
        boolean sResult;
        while (true)
        {
            sResult = mFetchResult.rowHandle().next();
            if (sResult)
            {
                break;
            }
            else
            {
                if (mFetchResult.fetchRemains())
                {
                    // forward only와는 다르게 cache를 지우지 않는다.
                    try
                    {
                        CmProtocol.fetchNext(mStatement.getProtocolContext(), mFetchSize);
                    }
                    catch (SQLException ex)
                    {
                        AltibaseFailover.trySTF(mStatement.getAltibaseConnection().failoverContext(), ex);
                    }
                    if (mStatement.getProtocolContext().getError() != null)
                    {
                        mWarning = Error.processServerError(mWarning, mStatement.getProtocolContext().getError());
                    }
                    // continue를 하면 next()로 한번 이동할 것이기 때문에 미리 한칸 당긴다.
                    mFetchResult.rowHandle().previous();
                    continue;
                }
                else
                {
                    break;
                }
            }
        }
        return sResult;
    }

    public int getType() throws SQLException
    {
        throwErrorForClosed();
        return TYPE_SCROLL_INSENSITIVE;
    }

    public void refreshRow() throws SQLException
    {
        // do nothing
    }

    private void fetchAll() throws SQLException
    {
        while (mFetchResult.fetchRemains())
        {
            try
            {
                CmProtocol.fetchNext(mStatement.getProtocolContext(), mFetchSize);
            }
            catch (SQLException ex)
            {
                AltibaseFailover.trySTF(mStatement.getAltibaseConnection().failoverContext(), ex);
            }
            if (mStatement.getProtocolContext().getError() != null)
            {
                mWarning = Error.processServerError(mWarning, mStatement.getProtocolContext().getError());
            }
        }
    }

    private boolean checkCache(int aRow) throws SQLException
    {
        while (mFetchResult.fetchRemains() && mFetchResult.rowHandle().size() < aRow)
        {
            try
            {
                CmProtocol.fetchNext(mStatement.getProtocolContext(), mFetchSize);
            }
            catch (SQLException ex)
            {
                AltibaseFailover.trySTF(mStatement.getAltibaseConnection().failoverContext(), ex);
            }
            if (mStatement.getProtocolContext().getError() != null)
            {
                mWarning = Error.processServerError(mWarning, mStatement.getProtocolContext().getError());
            }
        }
        return mFetchResult.rowHandle().size() >= aRow;
    }

    int size()
    {
        if (mFetchResult.fetchRemains())
        {
            return Integer.MAX_VALUE;
        }
        else
        {
            return mFetchResult.rowHandle().size();
        }
    }

    void deleteRowInCache() throws SQLException
    {
        rowHandle().delete();
        previous();
    }
}
