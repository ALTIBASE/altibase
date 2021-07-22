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

import Altibase.jdbc.driver.datatype.ListBufferHandle;

public class CmProtocolContextPrepExec extends CmProtocolContextDirExec
{
    private ListBufferHandle mListBufferHandle;

    public CmProtocolContextPrepExec(CmChannel aChannel)
    {
        super(aChannel);
    }
    
    public CmGetBindParamInfoResult getBindParamResult()
    {
        return (CmGetBindParamInfoResult)getCmResult(CmGetBindParamInfoResult.MY_OP);
    }
    
    public CmBindParamDataOutResult getBindParamDataOutResult()
    {
        return (CmBindParamDataOutResult)getCmResult(CmBindParamDataOutResult.MY_OP);
    }

    public ListBufferHandle getListBufferHandle()
    {
        return mListBufferHandle;
    }

    public void setListBufferHandle(ListBufferHandle aListBufferHandle)
    {
        this.mListBufferHandle = aListBufferHandle;
    }

}
