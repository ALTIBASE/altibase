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

public class CmHandshakeResult
{
    private int mErrorCode;
    private String mErrorMessage;    
    private byte mModuleId;
    private byte mMajorVer;
    private byte mMinorVer;
    private byte mPatchVer;
    private byte mFlags;

    public CmHandshakeResult()
    {
    }
    
    public int getErrorCode()
    {
        return mErrorCode;
    }

    public String getErrorMessage()
    {
        return mErrorMessage;
    }
    
    public byte getModuleId()
    {
        return mModuleId;
    }

    public byte getMajorVersion()
    {
        return mMajorVer;
    }
    
    public byte getMinorVersion()
    {
    	return mMinorVer;
    }

    public byte getPatchVersion()
    {
    	return mPatchVer;
    }
    
    void setError(int aErrorCode, String aErrorMessage)
    {
        mErrorCode = aErrorCode;
        mErrorMessage = aErrorMessage;
    }
    
    void setResult(byte aModuleId, byte aMajorVer, byte aMinorVer, byte aPatchVer, byte aFlags)
    {
        mModuleId = aModuleId;
        mMajorVer = aMajorVer;
        mMinorVer = aMinorVer;
        mPatchVer = aPatchVer;
        mFlags = aFlags;
    }
}
