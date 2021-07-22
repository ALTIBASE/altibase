/*
 * Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

package Altibase.jdbc.driver.sharding.core;

public class FailoverAlignInfo
{
    private boolean mIsNeedAlign;
    private String  mSQLSTATE;
    private int     mNativeErrorCode;
    private String  mMessageText;
    private int     mNodeId;

    public FailoverAlignInfo(boolean aIsNeedAlign, String aSQLSTATE, int aNativeErrorCode,
                             String aMessageText, int aNodeId)
    {
        mIsNeedAlign = aIsNeedAlign;
        mSQLSTATE = aSQLSTATE;
        mNativeErrorCode = aNativeErrorCode;
        mMessageText = aMessageText;
        mNodeId = aNodeId;
    }

    public boolean isNeedAlign()
    {
        return mIsNeedAlign;
    }

    void setNeedAlign(boolean aIsNeedAlign)
    {
        mIsNeedAlign = aIsNeedAlign;
    }

    public String getSQLSTATE()
    {
        return mSQLSTATE;
    }

    public void setSQLSTATE(String aSQLSTATE)
    {
        mSQLSTATE = aSQLSTATE;
    }

    public int getNativeErrorCode()
    {
        return mNativeErrorCode;
    }

    public String getMessageText()
    {
        return mMessageText;
    }

    public int getNodeId()
    {
        return mNodeId;
    }
}
