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
import com.altibase.altilinker.adlp.*;
import com.altibase.altilinker.adlp.type.*;
import com.altibase.altilinker.util.TraceLogger;

public class XAPrepareOp extends RequestOperation
{
    public int          mXidCount;
    public AldpXid[]    mXid;
    public String[]     mTargetName;
    
    protected static TraceLogger mTraceLogger         = TraceLogger.getInstance();
    
    public XAPrepareOp()
    {
        super(OpId.XAPrepare, true);
    }

    public ResultOperation newResultOperation()
    {
        return new XAPrepareResultOp();
    }
    
    protected boolean readOperation(CommonHeader aCommonHeader,
                                    ByteBuffer   aOpPayload)
    {
        int i = 0;
        
        // common header
        setCommonHeader(aCommonHeader);
        
        if( aCommonHeader.getDataLength() > 0 )
        {
            // data
            mXidCount = aOpPayload.getInt();

            if( mXidCount > 0 )
            {
                mXid        = new AldpXid[mXidCount];
                mTargetName = new String[mXidCount];

                for( i = 0; i < mXidCount ; i++ )
                {
                    mXid[i] = readAldpXid( aOpPayload );
                    mTargetName[i] = readVariableString( aOpPayload );

                    if ( mTargetName[i] != null )
                    {
                        mTargetName[i].toUpperCase();
                    }
                }
            }
        }
        
        return true;
    }

    protected boolean validate()
    {
        if (getSessionId() == 0)
        {
            return false;
        }
        
        if ( isXA() != true )
        {
            return false;
        }
        
        if ( mXid != null )
        {            
            if ( mXidCount != mXid.length )
            {
                return false;
            }
        }
        
        if ( mTargetName != null )
        {
            if ( mXidCount != mTargetName.length )
            {
                return false;
            }
        }
        
        return true;
    }
}
