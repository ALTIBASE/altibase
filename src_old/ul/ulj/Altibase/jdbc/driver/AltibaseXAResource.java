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

import javax.transaction.xa.XAException;
import javax.transaction.xa.XAResource;
import javax.transaction.xa.Xid;

import Altibase.jdbc.driver.cm.CmProtocol;
import Altibase.jdbc.driver.cm.CmProtocolContextXA;
import Altibase.jdbc.driver.cm.CmXAResult;
import Altibase.jdbc.driver.ex.Error;
import Altibase.jdbc.driver.ex.ErrorDef;

public class AltibaseXAResource implements XAResource
{
    /* Flag definitions for the RM switch */
    /** resource manager dynamically registers. */
    public static final int     TMREGISTER  = 0x00000001;
    /** resource manager does not support association migration */
    public static final int     TMNOMIGRATE = 0x00000002;
    /** resource manager supports asynchronous operations */
    public static final int     TMUSEASYNC  = 0x00000004;

    /* Flag definitions for xa_ and ax_ routines */
    /** perform routine asynchronously */
    public static final int     TMASYNC     = 0x80000000;
    /** return if blocking condition exists */
    public static final int     TMNOWAIT    = 0x10000000;
    /** wait for any asynchronous operation */
    public static final int     TMMULTIPLE  = 0x00400000;
    /** caller intends to perform migration */
    public static final int     TMMIGRATE   = 0x00100000;

    private AltibaseConnection  mConnection;
    private CmProtocolContextXA mContext;
    private CmXAResult          mResult;
    private boolean             mIsOpen;

    AltibaseXAResource(AltibaseConnection aConnection)
    {
        mConnection = aConnection;
        mContext = new CmProtocolContextXA(aConnection.channel(), hashCode());
    }
    
    public void xaOpen() throws SQLException
    {
        mConnection.setRelatedXAResource(this);
        try
        {
            CmProtocol.xaOpen(mContext);
        }
        catch (SQLException ex)
        {
            AltibaseFailover.trySTF(mConnection.failoverContext(), ex);
        }
        if (mContext.getError() != null)
        {
            Error.processServerError(null, mContext.getError());
        }
        mResult = mContext.getXAResult();
        mIsOpen = (mResult.getResultValue() == XA_OK);
        if (!mIsOpen)
        {
            Error.throwSQLException(ErrorDef.XA_OPEN_FAIL, mResult.getResultValueString());
        }
    }

    void xaClose() throws SQLException
    {
        mConnection.setRelatedXAResource(null);
        mIsOpen = false;
        try
        {
            CmProtocol.xaClose(mContext);
        }
        catch (SQLException ex)
        {
            AltibaseFailover.trySTF(mConnection.failoverContext(), ex);
        }
        if (mContext.getError() != null)
        {
            Error.processServerError(null, mContext.getError());
        }
        mResult = mContext.getXAResult();
        if (mResult.getResultValue() != XA_OK)
        {
            Error.throwSQLException(ErrorDef.XA_CLOSE_FAIL, mResult.getResultValueString());
        }
    }
    
    public void commit(Xid aXid, boolean aOnePhase) throws XAException
    {
        Error.checkXidAndThrowXAException(aXid);
        int sFlags = TMNOFLAGS;
        if (aOnePhase)
        {
            sFlags = sFlags | TMONEPHASE;
        }
        try
        {
            try
            {
                CmProtocol.xaCommit(mContext, aXid, sFlags);
            }
            catch (SQLException ex)
            {
                AltibaseFailover.trySTF(mConnection.failoverContext(), ex);
            }
        }
        catch (SQLException sException)
        {
            Error.throwXaException(sException);
        }
        Error.processXaError(mContext.getError(), mContext.getXAResult());
    }

    public void end(Xid aXid, int aFlags) throws XAException
    {
        Error.checkXidAndThrowXAException(aXid);
        try
        {
            try
            {
                CmProtocol.xaEnd(mContext, aXid, aFlags);
            }
            catch (SQLException ex)
            {
                AltibaseFailover.trySTF(mConnection.failoverContext(), ex);
            }
        }
        catch (SQLException sException)
        {
            Error.throwXaException(sException);
        }
        Error.processXaError(mContext.getError(), mContext.getXAResult());
    }

    public void forget(Xid aXid) throws XAException
    {
        Error.checkXidAndThrowXAException(aXid);
        try
        {
            try
            {
                CmProtocol.xaForget(mContext, aXid);
            }
            catch (SQLException ex)
            {
                AltibaseFailover.trySTF(mConnection.failoverContext(), ex);
            }
        }
        catch (SQLException sException)
        {
            Error.throwXaException(sException);
        }
        Error.processXaError(mContext.getError(), mContext.getXAResult());
    }

    public int getTransactionTimeout() throws XAException
    {
        try
        {
            return mConnection.getTransTimeout();
        }
        catch (SQLException sException)
        {
            Error.throwXaException(sException);
            return 0;
        }
    }

    public boolean isSameRM(XAResource aXares) throws XAException
    {
        return (this == aXares);
    }

    public int prepare(Xid aXid) throws XAException
    {
        Error.checkXidAndThrowXAException(aXid);
        try
        {
            try
            {
                CmProtocol.xaPrepare(mContext, aXid);
            }
            catch (SQLException ex)
            {
                AltibaseFailover.trySTF(mConnection.failoverContext(), ex);
            }
        }
        catch (SQLException sException)
        {
            Error.throwXaException(sException);
        }
        Error.processXaError(mContext.getError(), mContext.getXAResult());
        return mContext.getXAResult().getResultValue();
    }

    public Xid[] recover(int aFlag) throws XAException
    {
        try
        {
            try
            {
                CmProtocol.xaRecover(mContext, aFlag);
            }
            catch (SQLException ex)
            {
                AltibaseFailover.trySTF(mConnection.failoverContext(), ex);
            }
        }
        catch (SQLException sException)
        {
            Error.throwXaException(sException);
        }
        Error.processXaError(mContext.getError(), null);
        int sXaResult = mContext.getXAResult().getResultValue();
        if (sXaResult < 0)
        {
            Error.throwXAException(ErrorDef.XA_RECOVER_FAIL, sXaResult);
        }
        return mContext.getXidResult().getXids();
    }

    public void rollback(Xid aXid) throws XAException
    {
        Error.checkXidAndThrowXAException(aXid);
        try
        {
            try
            {
                CmProtocol.xaRollback(mContext, aXid);
            }
            catch (SQLException ex)
            {
                AltibaseFailover.trySTF(mConnection.failoverContext(), ex);
            }
        }
        catch (SQLException sException)
        {
            Error.throwXaException(sException);
        }
        Error.processXaError(mContext.getError(), mContext.getXAResult());
    }

    public boolean setTransactionTimeout(int aSeconds) throws XAException
    {
        try
        {
            mConnection.setTransTimeout(aSeconds);
        }
        catch (SQLException sException)
        {
            Error.throwXaException(sException);
            return false;
        }
        return true;
    }

    public void start(Xid aXid, int aFlags) throws XAException
    {
        Error.checkXidAndThrowXAException(aXid);
        try
        {
            try
            {
                CmProtocol.xaStart(mContext, aXid, aFlags);
            }
            catch (SQLException ex)
            {
                AltibaseFailover.trySTF(mConnection.failoverContext(), ex);
            }
        }
        catch (SQLException sException)
        {
            Error.throwXaException(sException);
        }
        Error.processXaError(mContext.getError(), mContext.getXAResult());
    }

    /**
     * open한 상태인지 확인한다.
     *
     * @return open했으면 <tt>true</tt>, 아니면 <tt>false</tt>
     */
    public boolean isOpen()
    {
        return mIsOpen;
    }
}
