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
 

/***********************************************************************
 * $id$
 **********************************************************************/

#ifndef _O_DKT_REMOTE_TX_H_
#define _O_DKT_REMOTE_TX_H_ 1

#include <idTypes.h>
#include <dkt.h>
#include <dktRemoteStmt.h>
#include <sdi.h>

class dktRemoteTx
{
private:
    idBool          mIsTimedOut;

    /*
    UShort          mLinkObjId;
    */
    UShort          mRemoteNodeSessionId;
    SShort          mStmtIdBase;

    UInt            mLinkerSessionId;
    UInt            mId;
    UInt            mGlobalTxId;
    UInt            mLocalTxId;
    dktRTxStatus    mStatus;            /* remote transaction's status */
    UInt            mStmtCnt;

    SLong           mCurRemoteStmtId;
    SLong           mNextRemoteStmtId;

    SChar           mTargetName[DK_NAME_LEN + 1];
    dktLinkerType   mLinkerType;
    sdiConnectInfo *mDataNode;          /* shard node */

    iduList         mRemoteStmtList;    /* remote statement list */
    iduList         mSavepointList;     /* savepoint list */

    iduMutex        mDktRStmtMutex;

public:

    iduListNode  mNode;
    ID_XID       mXID;
    idBool       mIsXAStarted;

    /* >> BUG-37487 */
    /* Initialize / Finalize */
    IDE_RC          initialize( UInt            aSessionId,
                                SChar          *aTargetName,
                                dktLinkerType   aLinkerType,
                                sdiConnectInfo *aDataNode,
                                UInt            aGlobalTxId,
                                UInt            aLocalTxId,
                                UInt            aRTxId );

    void            finalize();

    /* Remote statement list 를 정리한다. */
    void            destroyAllRemoteStmt(); 

    /* Savepoint list 를 정리한다. */
    void            deleteAllSavepoint();
    /* << BUG-37487 */

    /* 새로운 remote statement 를 하나 생성하여 list 에 추가한다. */
    IDE_RC          createRemoteStmt( UInt             aStmtType,
                                      SChar           *aStmtStr,
                                      dktRemoteStmt  **aRemoteStmt );

    /* 해당하는 remote statement 를 remote statement list 로부터 제거한다. */
    /* BUG-37487 */
    void            destroyRemoteStmt( dktRemoteStmt   *aRemoteStmt ); 

    /* list 에서 해당 id 를 갖는 remote statement node 를 찾아 반환한다. */
    dktRemoteStmt*  findRemoteStmt( SLong    aRemoteStmtId );

    /* 이 remote transaction object 에 remote statement 가 없는지 검사한다. */
    idBool          isEmptyRemoteTx();

    /* Remote statement id 를 생성한다. */
    SLong           generateRemoteStmtId();

    /* Savepoint 설정 및 검색 */
    IDE_RC          setSavepoint( const SChar   *aSavepointName );

    dktSavepoint*   findSavepoint( const SChar   *aSavepointName );

    /* BUG-37487 */
    void            removeSavepoint( const SChar  *aSavepointName );

    /* BUG-37512 */
    void            removeAllNextSavepoint( const SChar *aSavepointName );

    /* PV: 이 remote transaction 의 정보를 얻는다. */
    IDE_RC          getRemoteTxInfo( dktRemoteTxInfo    *aInfo );

    IDE_RC          getRemoteStmtInfo( dktRemoteStmtInfo    *aInfo,
                                       UInt                  aStmtCnt,
                                       UInt                 *aInfoCnt );

    IDE_RC          freeAndDestroyAllRemoteStmt( dksSession *aSession, UInt  aSessionId );
    inline UInt     getRemoteStmtCount();

    /* mIsPrepared 의 값을 확인한다. */
    inline idBool   isPrepared();

    inline void     setRemoteTransactionStatus( UInt aStatus );
    inline UInt     getRemoteTransactionStatus();

    
    /* Functions for setting and getting member value */
    inline void     setStatus( dktRTxStatus aStatus );
    inline dktRTxStatus    getStatus();

    inline void     setRemoteNodeSessionId( UShort aId );
    inline UShort   getRemoteNodeSessionId();

    inline UInt     getRemoteTransactionId();

    inline SChar*   getTargetName();
    inline dktLinkerType   getLinkerType();
    inline sdiConnectInfo *getDataNode();
};

inline UInt     dktRemoteTx::getRemoteStmtCount()
{
    UInt sStmtCnt = 0;

    IDE_ASSERT( mDktRStmtMutex.lock( NULL /*idvSQL* */ ) == IDE_SUCCESS );

    sStmtCnt = mStmtCnt;

    IDE_ASSERT( mDktRStmtMutex.unlock() == IDE_SUCCESS );

    return sStmtCnt;
}

inline void dktRemoteTx::setStatus( dktRTxStatus aStatus )
{
    mStatus = aStatus;
}

inline dktRTxStatus dktRemoteTx::getStatus()
{
    return mStatus;
}

inline void dktRemoteTx::setRemoteNodeSessionId( UShort  aId )
{
    mRemoteNodeSessionId = aId;
}

inline UShort dktRemoteTx::getRemoteNodeSessionId()
{
    return mRemoteNodeSessionId;
}

inline UInt dktRemoteTx::getRemoteTransactionId()
{
    return mId;
}

inline SChar*   dktRemoteTx::getTargetName()
{
    return mTargetName;
}

inline dktLinkerType dktRemoteTx::getLinkerType()
{
    return mLinkerType;
}

inline sdiConnectInfo * dktRemoteTx::getDataNode()
{
    return mDataNode;
}

#endif /* _O_DKT_REMOTE_TX_H_ */
