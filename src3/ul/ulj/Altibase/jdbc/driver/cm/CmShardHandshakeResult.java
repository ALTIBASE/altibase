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

package Altibase.jdbc.driver.cm;

public class CmShardHandshakeResult extends CmProtocolContext
{
    private byte mMajorVer;
    private byte mMinorVer;
    private byte mPatchVer;
    private byte mFlag;

    public CmShardHandshakeResult()
    {
    }

    public void setError(int aErrorCode, String aErrorMessage)
    {
        CmErrorResult sError = new CmErrorResult(CmOperationDef.DB_OP_SHARD_HANDSHAKE_RESULT,
                                                 0,
                                                 aErrorCode,
                                                 aErrorMessage,
                                                 false);
        addError(sError);
    }

    public void setResult(byte aMajorVer, byte aMinorVer, byte aPatchVer, byte aFlags)
    {
        this.mMajorVer = aMajorVer;
        this.mMinorVer = aMinorVer;
        this.mPatchVer = aPatchVer;
        this.mFlag     = aFlags;
    }

    @Override
    public String toString()
    {
        final StringBuilder sSb = new StringBuilder("CmShardHandshakeResult{");
        sSb.append("mMajorVer=").append(mMajorVer);
        sSb.append(", mMinorVer=").append(mMinorVer);
        sSb.append(", mPatchVer=").append(mPatchVer);
        sSb.append(", mFlag=").append(mFlag);
        sSb.append('}');
        return sSb.toString();
    }
}
