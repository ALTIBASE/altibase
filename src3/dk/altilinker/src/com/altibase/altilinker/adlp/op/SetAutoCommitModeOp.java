/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
package com.altibase.altilinker.adlp.op;

import java.nio.ByteBuffer;

import com.altibase.altilinker.adlp.CommonHeader;
import com.altibase.altilinker.adlp.RequestOperation;
import com.altibase.altilinker.adlp.ResultOperation;

public class SetAutoCommitModeOp extends RequestOperation
{
    public boolean mAutoCommitMode = false;
    
    public SetAutoCommitModeOp()
    {
        super(OpId.SetAutoCommitMode, true);
    }

    public ResultOperation newResultOperation()
    {
        return new SetAutoCommitModeResultOp();
    }
    
    protected boolean readOperation(CommonHeader aCommonHeader,
                                    ByteBuffer   aOpPayload)
    {
        // common header
        setCommonHeader(aCommonHeader);
        
        mAutoCommitMode = (getOpSpecificParameter() != 0) ? true : false;

        return true;
    }

    protected boolean validate()
    {
        if (getSessionId() == 0)
        {
            return false;
        }
        
        byte sOpSpecificParameter = getOpSpecificParameter();
        
        if (sOpSpecificParameter != 0 && sOpSpecificParameter != 1)
        {
            return false;
        }
        
        return true;
    }
}
