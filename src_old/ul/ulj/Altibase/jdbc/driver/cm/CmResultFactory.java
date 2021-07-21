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

import java.util.HashMap;

public class CmResultFactory
{
    private static HashMap mTypes = new HashMap();

    static
    {
        register(new CmConnectExResult());
        register(new CmBindParamDataOutResult());
        register(new CmBlobGetResult());
        register(new CmClobGetResult());
        register(new CmExecutionResult());
        register(new CmFetchResult());
        register(new CmGetBindParamInfoResult());
        register(new CmGetColumnInfoResult());
        register(new CmGetPropertyResult());
        register(new CmPrepareResult());
        register(new CmGetPlanResult());
        register(new CmXAResult());
        register(new CmXidResult());
        register(new CmSetPropertyResult());       // TASK-7220 고성능 분산공유트랜잭션 정합성  
        register(new CmShardTransactionResult());  // TASK-7220 고성능 분산공유트랜잭션 정합성       
    }

    private static void register(CmResult aResult)
    {
        int sResultOp = Byte.toUnsignedInt(aResult.getResultOp());  // BUG-48775
        mTypes.put(String.valueOf(sResultOp), aResult.getClass());
    }

    public static CmResult getInstance(int aResultOp)
    {
        try
        {
            return (CmResult)(((Class) mTypes.get(String.valueOf(aResultOp))).newInstance());
        }
        catch (Exception e)
        {
            return null;
        }
    }
}
