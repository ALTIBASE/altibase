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
import java.util.*;
import com.altibase.altilinker.adlp.*;
import com.altibase.altilinker.adlp.type.*;
import javax.transaction.xa.Xid;;

public class XAPrepareResultOp extends ResultOperation
{   
    public LinkedList   mRDOnlyXidList = new LinkedList();
    public LinkedList   mFailXidList   = new LinkedList();
    
    // ... Count of Failed XIDs (4)    XID1 (152)  ...Err Code XID2 (152)  Err Code
    
    public XAPrepareResultOp()
    {
        super(OpId.XAPrepareResult, false);
    }

   protected boolean writeOperation(CommonHeader aCommonHeader,
                                     ByteBuffer   aOpPayload)
    {
        Iterator sIter = null;
        
        fillCommonHeader(aCommonHeader);
        
        writeInt( aOpPayload, mRDOnlyXidList.size() );

        if( mRDOnlyXidList.size() > 0 )
        {   
            sIter = mRDOnlyXidList.iterator();
            while( sIter.hasNext() )
            {
                AldpXid sXid = (AldpXid)sIter.next();
                writeBuffer( aOpPayload, sXid.getByteBuffer() );
            }
            
            mRDOnlyXidList.clear();
        }

//      DK에서 첫번째 Error를 로그에 찍어 주므로 첫번째 Error만 보낸다.
//      writeInt( aOpPayload, mFailXidList.size() );
        if ( mFailXidList.size() > 0 )
        {
            writeInt( aOpPayload, 1 );
            
            sIter = mFailXidList.iterator();
            AldpXid sXid = (AldpXid)sIter.next();
            writeBuffer( aOpPayload, sXid.getByteBuffer() );
            writeInt( aOpPayload, sXid.getErrorCode() );
          
           mFailXidList.clear();
        }
        else
        {
            writeInt( aOpPayload, 0 );
        }
        
        return true;
    }
    
    public void addRDOnlyXid( Xid aXid )
    {
        mRDOnlyXidList.add( new AldpXid( aXid ) );
    }
    
    public void addFailXid( Xid aXid, int aErrCode )
    {
        mFailXidList.add( new AldpXid( aXid, aErrCode ) );
    }

}
