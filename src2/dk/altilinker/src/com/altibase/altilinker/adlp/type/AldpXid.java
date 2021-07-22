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
 
package com.altibase.altilinker.adlp.type;

import java.nio.ByteBuffer;
import javax.transaction.xa.Xid;
import com.altibase.altilinker.util.AltilinkerXid;

public class AldpXid extends Object
{
    public static final int   MAXGTRIDSIZE = 18;
    public static final int   MAXBQUALSIZE = 4;
    public static final int   DATASIZE  = MAXGTRIDSIZE + MAXBQUALSIZE;
    public static final int   SIZEOF_XID   = DATASIZE; // ID_SIZEOF( XID )
    // formatId , GTxId len,  bqual len, data length
    private int             mFormatId = 0;
    private byte[]          mGlobalTransactionId;
    private byte[]          mBranchQualifier;

    private int             mErrCode   = 0; // 0 means SUCCESS 
    // mErrCode would be used, 
    // when process Task related with ResultOperation

    public AldpXid()
    {
        
    }
    
    public AldpXid( Xid aXid )
    {
        setData( aXid.getFormatId(), 
                 aXid.getGlobalTransactionId(), 
                 aXid.getBranchQualifier() );
    }

    public AldpXid( Xid aXid, int aErrCode )
    {
        setData( aXid.getFormatId(), 
                 aXid.getGlobalTransactionId(), 
                 aXid.getBranchQualifier() );

        mErrCode = aErrCode;
    }

    public AldpXid( int     aFormatId,
                    byte[]  aGlobalTransactionId,
                    byte[]  aBranchQualifier )
    {
        setData( aFormatId, 
                 aGlobalTransactionId, 
                 aBranchQualifier );
    }

    public AldpXid( int    aFormatId, 
                    int    aGlobalTransactionIdLength, 
                    int    aBranchQualifierLength,
                    byte[] aData )
    {
        setData( aFormatId,
                 aGlobalTransactionIdLength,
                 aBranchQualifierLength,
                 aData );
    }

    public AldpXid( ByteBuffer aBuffer )
    {
        setDataFromByteBuffer( aBuffer );
    }

    public void setDataFromByteBuffer( ByteBuffer aBuffer )
    {
        byte[] aXIDData                 = new byte[DATASIZE];
        
        int  sFormatId                  = 0;
        int  sGlobalTransactionIdLength = MAXGTRIDSIZE;
        int  sBranchQualifierLength     = MAXBQUALSIZE;

        aBuffer.get( aXIDData, 0, DATASIZE );

        setData( sFormatId, 
                 sGlobalTransactionIdLength, 
                 sBranchQualifierLength, 
                 aXIDData );

        aXIDData = null;
    }

    public void setData( int    aFormatId, 
                         int    aGlobalTransactionIdLength, 
                         int    aBranchQualifierLength,
                         byte[] aData )
    {
        mFormatId            = aFormatId;
        mGlobalTransactionId = new byte[MAXGTRIDSIZE];
        mBranchQualifier     = new byte[MAXBQUALSIZE];

        /* XID format
         *  0    17 18     21 
         * | GTxID | BQual   |
         */
        System.arraycopy( aData, 0, 
                          mGlobalTransactionId, 0, 
                          MAXGTRIDSIZE );

        System.arraycopy( aData, MAXGTRIDSIZE, 
                          mBranchQualifier, 0, 
                          MAXBQUALSIZE );
    }


    public void setData( int     aFormatId,
                         byte[]  aGlobalTransactionId,
                         byte[]  aBranchQualifier )
    {
        mFormatId                = aFormatId;
        mGlobalTransactionId     = (byte[])aGlobalTransactionId.clone();
        mBranchQualifier         = (byte[])aBranchQualifier.clone();
    }


    public int getFormatId()
    {
        return mFormatId;
    }

    public byte[] getGlobalTransactionId()
    {
        return mGlobalTransactionId;
    }

    public byte[] getBranchQualifier()
    {
        return mBranchQualifier;
    }

    public ByteBuffer getByteBuffer()
    {
        ByteBuffer sBuffer = ByteBuffer.allocate( SIZEOF_XID );
        
        sBuffer.put( mGlobalTransactionId, 0, MAXGTRIDSIZE );
        sBuffer.put( mBranchQualifier, 0, MAXBQUALSIZE );
        
        sBuffer.rewind();

        return sBuffer;
    }

    public int getErrorCode()
    {
        return mErrCode;
    }
    
    public Xid convertToXid()
    {
        AltilinkerXid sXid = new AltilinkerXid( mFormatId, 
                                                (byte[])mGlobalTransactionId.clone(), 
                                                (byte[])mBranchQualifier.clone() );
        return (Xid)sXid;
    }
}
