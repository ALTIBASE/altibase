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
import Altibase.jdbc.driver.cm.CmChannel;
import Altibase.jdbc.driver.cm.CmProtocol;
import Altibase.jdbc.driver.cm.CmProtocolContextLob;
import Altibase.jdbc.driver.ex.Error;

public abstract class AltibaseLob
{
    protected long mLocatorId;
    protected long mLobLength;
    protected CmChannel mChannel;
    protected CmProtocolContextLob mContext;
    protected boolean mLobUpdated;
    protected boolean mIsClosed;

    AltibaseLob(long aLocatorId, long aLobLength)
    {
        mLocatorId = aLocatorId;
        mLobLength = aLobLength;
        mLobUpdated = false;
    }
    
    public void open(CmChannel aChannel)
    {
        mChannel = aChannel;
        mContext = new CmProtocolContextLob(aChannel, mLocatorId, mLobLength);
        mIsClosed = false;
    }

    public void truncate(long aLength) throws SQLException
    {
        mLobUpdated = true;
        CmProtocol.truncate(mContext, (int)aLength);
        if (mContext.getError() != null)
        {
            Error.processServerError(null, mContext.getError());
        }
    }

    public boolean isSameConnectionWith(CmChannel aChannel)
    {
        return mContext.channel().equals(aChannel);
    }
    
    public void free() throws SQLException
    {
        if(mContext != null)
        {
            CmProtocol.free(mContext);
        }
        mIsClosed = true;
    }
}
