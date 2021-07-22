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
 
#ifndef _O_DKT_NOTIFIER_H_
#define _O_DKT_NOTIFIER_H_ 1


#include <idl.h>
#include <idtBaseThread.h>
#include <dkt.h>
#include <dksDef.h>
#include <dktDtxInfo.h>
#include <dki.h>

typedef enum
{
    DK_NOTIFY_NONE,
    DK_NOTIFY_NORMAL,
    DK_NOTIFY_FAILOVER
} DK_NOTIFY_TYPE;

class dktNotifier : public idtBaseThread
{
private:
    dksSession * mSession;
    /* 미반영 글로벌 트랜잭션 개수*/
    UInt    mDtxInfoCnt;
    UInt    mFailoverDtxInfoCnt;
    idBool  mExit;
    idBool  mPause;
    idBool  mRestart;
    idBool  mRunFailoverOneCycle;

public:
    /* Notify 대상 글로벌 트랜잭션 리스트 */
    iduList  mDtxInfo;                 /* PROJ-2569 2PC */
    iduList  mFailoverDtxInfo;         /* PROJ-2747 Global Tx Consistent */
    UInt     mSessionId;
    /* mDtxInfoList 에 대한 동시접근을 막기 위한 mutex */
    iduMutex  mNotifierDtxInfoMutex;

public:
    dktNotifier() {};
    virtual ~dktNotifier() {};

    IDE_RC initialize();
    IDE_RC createDtxInfo( DK_NOTIFY_TYPE   aType,
                          ID_XID         * aGlobalXID,
                          UInt             aLocalTxId, 
                          UInt             aGlobalTxId, 
                          idBool           aIsRequestNode,
                          dktDtxInfo    ** aDtxInfo );
    void   addDtxInfo( DK_NOTIFY_TYPE aType, dktDtxInfo * aDtxInfo );
    void   removeDtxInfo( DK_NOTIFY_TYPE aType, dktDtxInfo * aDtxInfo );
   
    idBool setResultPassiveDtxInfo( ID_XID * aXID, idBool aCommit );
    IDE_RC sendResult( UInt * aSendCount );

    idBool setResultFailoverDtxInfo( ID_XID * aXID, 
                                     idBool   aCommit,
                                     smSCN  * aGlobalCommitSCN );

    void   failoverNotify();
    IDE_RC askResultToRequestNode( dktDtxInfo * aDtxInfo, UInt * aResultCode );

    void   notify();
    IDE_RC notifyXaResult( dktDtxInfo    * aDtxInfo,
                           dksSession    * aSession,
                           UInt          * aResultCode,
                           UInt          * aCountFailXID,
                           ID_XID       ** aFailXIDs,
                           SInt         ** aFailErrCodes,
                           UInt          * aCountHeuristicXID,
                           ID_XID       ** aHeuristicXIDs );
    IDE_RC notifyXaResultForDBLink( dktDtxInfo    * aDtxInfo,
                                    dksSession    * aSession,
                                    UInt          * aResultCode,
                                    UInt          * aCountFailXID,
                                    ID_XID       ** aFailXIDs,
                                    SInt         ** aFailErrCodes,
                                    UInt          * aCountHeuristicXID,
                                    ID_XID       ** aHeuristicXIDs );
    IDE_RC notifyXaResultForShard( dktDtxInfo    * aDtxInfo,
                                   dksSession    * aSession,
                                   UInt          * aResultCode,
                                   UInt          * aCountFailXID,
                                   ID_XID       ** aFailXIDs,
                                   SInt         ** aFailErrCodes,
                                   UInt          * aCountHeuristicXID,
                                   ID_XID       ** aHeuristicXIDs );

    IDE_RC notifyOneBranchXaResultForShard( dktDtxInfo          * aDtxInfo,
                                            dktDtxBranchTxInfo  * aDtxBranchTxInfo,
                                            ID_XID              * aXID );

    void freeXaResult( dktLinkerType   aLinkerType,
                       ID_XID        * aFailXIDs,
                       ID_XID        * aHeuristicXIDs,
                       SInt          * aFailErrCodes );

    idBool findDtxInfo( DK_NOTIFY_TYPE aType,
                        UInt aLocalTxId, 
                        UInt  aGlobalTxId, 
                        dktDtxInfo ** aDtxInfo );
    idBool findDtxInfoByXID( DK_NOTIFY_TYPE    aType,
                             idBool            aLocked,
                             ID_XID          * aXID,
                             dktDtxInfo     ** aDtxInfo );

    IDE_RC removeEndedDtxInfo( DK_NOTIFY_TYPE aType,
                               UInt     aLocalTxId,
                               UInt     aGlobalTxId );
    void writeNotifyHeuristicXIDLog( dktDtxInfo   * aDtxInfo,
                                     UInt           aCountHeuristicXID,
                                     ID_XID       * aHeuristicXIDs );
    void writeNotifyFailLog( ID_XID       * aFailXIDs, 
                             SInt         * aFailErrCodes, 
                             dktDtxInfo   * aDtxInfo );
    void finalize();
    void run();
    void destroyDtxInfoList();

    IDE_RC manageDtxInfoListByLog( ID_XID * aXID,
                                   UInt     aLocalTxId,
                                   UInt     aGlobalTxId,
                                   UInt     aBranchTxInfoSize,
                                   UChar  * aBranchTxInfo,
                                   smLSN  * aPrepareLSN,
                                   smSCN  * aGlobalCommitSCN,
                                   UChar    aType );
    IDE_RC setResult( DK_NOTIFY_TYPE aType,
                      UInt    aLocalTxId,
                      UInt    aGlobalTxId,
                      UChar   aResult,
                      smSCN * aGlobalCommitSCN );

    IDE_RC getNotifierTransactionInfo( dktNotifierTransactionInfo ** aInfo,
                                       UInt                        * aInfoCount );
    
    IDE_RC getShardNotifierTransactionInfo( dktNotifierTransactionInfo ** aInfo,
                                            UInt                        * aInfoCount );

    UInt   getAllBranchTxCnt();
    UInt   getAllShardBranchTxCnt();

    IDE_RC addUnCompleteGlobalTxList( iduList * aGlobalTxList );

    void   waitUntilFailoverNotifierRunOneCycle();

    inline void setExit( idBool aIsExit );
    inline void setPause( idBool aIsPause );
    inline void setSession( dksSession * aSession );

    inline UInt getDtxInfoCnt( void );
    
    inline void lock(); 
    inline void unlock();
};

inline void dktNotifier::setExit( idBool aIsExit )
{
    mExit = aIsExit;
}

inline void dktNotifier::setPause( idBool aIsPause )
{
    mPause = aIsPause;
}

inline void dktNotifier::setSession( dksSession * aSession )
{
    mSession = aSession;
}

inline UInt dktNotifier::getDtxInfoCnt()
{
    UInt sDtxInfoCnt;

    (void)(mNotifierDtxInfoMutex.lock(NULL));

    sDtxInfoCnt = mDtxInfoCnt;

    (void)(mNotifierDtxInfoMutex.unlock());

    return sDtxInfoCnt;
}

inline void dktNotifier::lock() 
{ 
    IDE_ASSERT( mNotifierDtxInfoMutex.lock( NULL /*idvSQL* */ ) == IDE_SUCCESS );
}

inline void   dktNotifier::unlock() 
{ 
    IDE_ASSERT( mNotifierDtxInfoMutex.unlock() == IDE_SUCCESS );
}

#endif  /* _O_DKT_NOTIFIER_H_ */
