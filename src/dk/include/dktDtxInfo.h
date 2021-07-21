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
 
#ifndef _O_DKT_DTX_INFO_H_
#define _O_DKT_DTX_INFO_H_ 1

#include <idl.h>
#include <dkDef.h>
#include <dkt.h>
#include <smiDef.h>
#include <sdi.h>

class dktDtxInfo
{
private:

public:
    smTID          mLocalTxId;
    UInt           mResult;
    smLSN          mPrepareLSN;
    UInt           mGlobalTxId;
    iduList        mBranchTxInfo;
    UInt           mBranchTxCount;
    dktLinkerType  mLinkerType;
    iduListNode    mNode;
    ID_XID         mXID;
    ID_XID         mGlobalXID;
    idBool         mIsFailoverRequestNode;
    idBool         mIsPassivePending;
    smSCN          mGlobalCommitSCN;
    smiTransNode * mFailoverTrans;
    iduMutex       mDtxInfoGlobalTxResultMutex;

    IDE_RC initialize( ID_XID * aGlobalXID,
                       UInt     aLocalTxId, 
                       UInt     aGlobalTxId,
                       idBool   aIsRequestNode );

    void finalize( );

    IDE_RC removeDtxBranchTx( ID_XID * aXID );
    IDE_RC removeDtxBranchTx( dktDtxBranchTxInfo * aDtxBranchTxInfo );
    void removeAllBranchTx();

    IDE_RC addDtxBranchTx( ID_XID * aXID, SChar * aTarget );
    IDE_RC addDtxBranchTx( ID_XID               * aXID,
                           sdiCoordinatorType     aCoordinatorType,
                           SChar                * aNodeName,
                           SChar                * aUserName,
                           SChar                * aUserPassword,
                           SChar                * aDataServerIP,
                           UShort                 aDataPortNo,
                           UShort                 aConnectType );
    IDE_RC addDtxBranchTx( dktDtxBranchTxInfo * aDtxBranchTxInfo );
    dktDtxBranchTxInfo * getDtxBranchTx( ID_XID * aXID );

    void dumpBranchTx( SChar  * aBuf,
                       SInt     aBufSize,
                       UInt   * aBranchTxCnt ); /* out */

    UInt estimateSerializeBranchTx();
    IDE_RC serializeBranchTx( UChar * aBranchTxInfo, UInt aSize );
    IDE_RC unserializeAndAddDtxBranchTx( UChar * aBranchTxInfo, UInt aSize );
    IDE_RC copyString( UChar ** aBuffer,
                       UChar * aFence,
                       SChar * aString );

    inline dktLinkerType getLinkerType();
    inline smLSN * getPrepareLSN( void );
    inline UInt getFileNo( void );
    inline idBool isEmpty();

    inline void globalTxResultLock();
    inline void globalTxResultUnlock();
};

class dktXid
{
public:
    static void   initXID( ID_XID * aXID );
    static void   copyXID( ID_XID * aDst, ID_XID * aSrc );
    static idBool isEqualXID( ID_XID * aXID1, ID_XID * aXID2 );
    static void   copyGlobalXID( ID_XID * aDst, ID_XID * aSrc );
    static idBool isEqualGlobalXID( ID_XID * aXID1, ID_XID * aXID2 );

    static UChar  sizeofXID( ID_XID * aXID );

    static UInt   getGlobalTxIDFromXID( ID_XID * aXID );
    static UInt   getLocalTxIDFromXID( ID_XID * aXID );
};

inline dktLinkerType dktDtxInfo::getLinkerType()
{
    return mLinkerType;
}

inline smLSN * dktDtxInfo::getPrepareLSN( void )
{
    return &mPrepareLSN;
}

inline idBool dktDtxInfo::isEmpty() 
{ 
    if ( mBranchTxCount == 0 )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

inline void dktDtxInfo::globalTxResultLock()
{
    IDE_ASSERT( mDtxInfoGlobalTxResultMutex.lock( NULL /*idvSQL* */  ) == IDE_SUCCESS );
}

inline void dktDtxInfo::globalTxResultUnlock()
{
    IDE_ASSERT( mDtxInfoGlobalTxResultMutex.unlock() == IDE_SUCCESS );
}

#endif  /* _O_DKT_DTX_INFO_H_ */
