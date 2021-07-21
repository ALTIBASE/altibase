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

import Altibase.jdbc.driver.sharding.core.DistTxInfo;

public class CmProtocolContext
{
    private CmChannel     mChannel;
    private CmErrorResult mError      = null;
    private CmResult[]    mResults;
    
    // PROJ-2733 DistTxInfo
    private static Object mlock        = new Object();
    private static long   mSCN         = 0;  // cli의 Env->mSCN에 대응. 여러 Connection이 공유한다.
    private DistTxInfo    mDistTxInfo; 

    public CmProtocolContext(CmChannel aChannel)
    {
        this();
        mChannel = aChannel;
    }

    public CmProtocolContext()
    {
        int sOpCount = Byte.toUnsignedInt(CmOperation.DB_OP_COUNT);  // BUG-48775

        // BUG-46513 CmProtocolContextShardConnect, CmProtocolContextShasrdStmt에서
        //           참조하기 때문에 CmResult 초기화 구문을 추가
        mResults = new CmResult[sOpCount];
        mDistTxInfo = new DistTxInfo();
    }

    public void addCmResult(CmResult aResult)
    {
        int sResultOp = Byte.toUnsignedInt(aResult.getResultOp());  // BUG-48775
        mResults[sResultOp] = aResult;
    }
    
    public void clearError()
    {
        mError = null;
    }
    
    public CmErrorResult getError()
    {
        return mError;
    }

    public CmChannel channel()
    {
        return mChannel;
    }
    
    // BUG-42424 AltibasePreparedStatement에서 해당메소드를 이용하기 때문에 public으로 변경
    public CmResult getCmResult(byte aResultOp)
    {
        int      sResultOp = Byte.toUnsignedInt(aResultOp);  // BUG-48775
        CmResult sResult   = mResults[sResultOp];
        if (sResult == null)
        {
            sResult = CmResultFactory.getInstance(sResultOp);
            mResults[sResultOp] = sResult;
        }
        return sResult;
    }
    
    public void addError(CmErrorResult aError)
    {
        if (mError == null)
        {
            mError = aError;
        }
        else
        {
            mError.addError(aError);
        }

        // BUG-46513 smn 에러일때 execute결과를 셋팅하기 위해 Context객체 주입
        mError.setContext(this);
    }

    public void setChannel(CmChannel aChannel)
    {
        this.mChannel = aChannel;
    }

    public void initDistTxInfo()
    {
        mDistTxInfo.initDistTxInfo();
    }

    public DistTxInfo getDistTxInfo()
    {
        return mDistTxInfo;
    }

    public void setDistTxInfo(DistTxInfo aDistTxInfo)
    {
        mDistTxInfo = aDistTxInfo;
    }

    public void updateSCN(long aSCN)
    {
        synchronized(mlock) 
        {
            if (aSCN > mSCN) 
            {
                mSCN = aSCN;
            }
        }
    }

    public static long getSCN()
    {
        synchronized(mlock) 
        {
            return mSCN;
        }
    }
}
