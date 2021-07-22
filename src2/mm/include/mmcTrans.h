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

#ifndef _O_MMC_TRANS_H_
#define _O_MMC_TRANS_H_ 1


#include <idl.h>
#include <ide.h>
#include <idu.h>
#include <mmcDef.h>
#include <smi.h>
#include <sdi.h>
class mmcSession;

#define MMC_DDL_BEGIN_SAVEPOINT SMI_DDL_BEGIN_SAVEPOINT 

#define MMC_SHARED_TRANS_TRACE( __aSession, __aTrans, __aStr )  \
    do                                                          \
    {                                                           \
        if ( IDL_LIKELY_FALSE( IDE_TRC_SD_32 != 0 ) )           \
        {                                                       \
            mmcTrans::shardedTransTrace( (__aSession),          \
                                         (__aTrans),            \
                                         (__aStr) );            \
        }                                                       \
    } while (0)

#define MMC_SHARED_PREPARE_TRANS_TRACE( __aSession, __aTrans, __aStr, __aXID )  \
    do                                                                          \
    {                                                                           \
        if ( IDL_LIKELY_FALSE( IDE_TRC_SD_32 != 0 ) )                           \
        {                                                                       \
            mmcTrans::shardedTransTrace( (__aSession),                          \
                                         (__aTrans),                            \
                                         (__aStr),                              \
                                         (__aXID) );                            \
        }                                                                       \
    } while (0)

#define MMC_END_PENDING_TRANS_TRACE( __aSession, __aSmiTrans, __aXID, __aIsCommit, __aStr ) \
    do                                                                                      \
    {                                                                                       \
        if ( IDL_LIKELY_FALSE( IDE_TRC_SD_32 != 0 ) )                                       \
        {                                                                                   \
            mmcTrans::endPendingTrace( (__aSession),                                        \
                                       (__aSmiTrans),                                       \
                                       (__aXID),                                            \
                                       (__aIsCommit),                                       \
                                       (__aStr) );                                          \
        }                                                                                   \
    } while (0)
 
typedef enum
{
    MMC_TRANS_STATE_NONE            = 0,
    MMC_TRANS_STATE_INIT_DONE       ,
    MMC_TRANS_STATE_BEGIN           ,
    MMC_TRANS_STATE_PREPARE         ,
    MMC_TRANS_STATE_END             ,
    MMC_TRANS_STATE_MAX             ,
} mmcTransState;

typedef enum
{
    MMC_TRANS_FIX_NO_OPT    = 0x00,
    MMC_TRANS_FIX_RECURSIVE = 0x01,
} mmcTransFixFlag;

#define MMC_TRANS_NULL_SLOT_NO (SM_NULL_TX_SLOT_NO)

typedef struct TxInfo
{
    mmcTransState   mState;
    idBool          mIsBroken;
    UInt            mAllocRefCnt;
    smSCN           mCommitSCN;
    UInt            mPrepareSlot;
    UInt            mTransRefCnt;
    mmcSession    * mDelegatedSessions;
    mmcSession    * mUserSession;
    SChar           mNodeName[SDI_NODE_NAME_MAX_SIZE + 1];
} mmcTxInfo;

typedef struct TxConcurrency
{
    iduMutex        mMutex;
    iduCond         mCondVar;

    idBool          mAllowRecursive;
    SInt            mFixCount;
    SInt            mWaiterCount;
    ULong           mFixOwner;
    SInt            mBlockCount;
} mmcTxConcurrency;

typedef struct mmcPendingTx
{
    ID_XID      mXID;
    UInt        mSlotID;
    iduListNode mListNode;
} mmcPendingTx;

/* PROJ-2701 online data rebuild & sharding n-transaction sharing */
typedef struct mmcTransShareInfo
{
    sdiShardPin         mShardPin;
    iduListNode         mListNode;

    mmcTxInfo           mTxInfo;
    mmcTxConcurrency    mConcurrency;
} mmcTransShareInfo;

typedef struct mmcTransObj
{
    smiTrans           mSmiTrans;
    mmcTransShareInfo *mShareInfo;
} mmcTransObj;

typedef enum mmcTransEndAction
{
    MMC_TRANS_DO_NOTHING               = 0,
    MMC_TRANS_SESSION_ONLY_END         = 1,
    MMC_TRANS_END                      = 2
} mmcTransEndAction;

class mmcTxConcurrencyDump
{
  public:
    idBool          mAllowRecursive;
    SInt            mFixCount;
    ULong           mFixOwner;
    idBool          mIsStored;

  public:
    void init()
    {
        mAllowRecursive = ID_FALSE;
        mFixCount       = 0;
        mFixOwner       = (ULong)PDL_INVALID_HANDLE;
        mIsStored       = ID_FALSE;
    }

    idBool isStored()
    {
        return mIsStored;
    }

    void store( mmcTxConcurrency * aConcurrency )
    {
        mFixCount       = aConcurrency->mFixCount;
        mFixOwner       = aConcurrency->mFixOwner;
        mAllowRecursive = aConcurrency->mAllowRecursive;
        mIsStored       = ID_TRUE;
    }

    void restore( mmcTxConcurrency * aConcurrency )
    {
        aConcurrency->mFixCount       = mFixCount;
        aConcurrency->mFixOwner       = mFixOwner;
        aConcurrency->mAllowRecursive = mAllowRecursive;
        init();
    }
};

class mmcTrans
{
public:
    /* destroy and free and process for session
     * if aSession is null then this function just do destroy and free.
     */
    static IDE_RC free(mmcSession * aSession, mmcTransObj * aTrans);

    /* alloc and transaction initialize and process for session 
     * if aSession is null then this function just do alloc and initialize.
     */
    static IDE_RC alloc(mmcSession * aSession, mmcTransObj **aTrans);
    static void   beginXA( mmcTransObj * aTrans,
                           idvSQL      * aStatistics,
                           UInt          aFlag );
    static void   begin( mmcTransObj * aTrans,
                         idvSQL      * aStatistics,
                         UInt          aFlag,
                         mmcSession  * aSession,
                         idBool      * aIsDummyBegin );
    static IDE_RC endPending( mmcSession * aSession,
                              ID_XID     * aXID,
                              idBool       aIsCommit,
                              smSCN      * aCommitSCN );
    static IDE_RC endPendingBySlotN( mmcTransObj * aTrans,
                                     mmcSession  * aSession,
                                     ID_XID      * aXID,
                                     idBool        aIsCommit,
                                    idBool         aMySelf,
                                     smSCN       * aCommitSCN );
    static IDE_RC endPendingSharedTx( mmcSession * aSession,
                                      ID_XID     * aXID,
                                      idBool       aIsCommit,
                                      smSCN      * aGlobalCommitSCN );
    static IDE_RC prepareForShard( mmcTransObj * aTrans,
                                   mmcSession  * aSession,
                                   ID_XID      * aXID,
                                   idBool      * aReadOnly,
                                   smSCN       * aPrepareSCN );
    static IDE_RC prepareXA( mmcTransObj * aTrans, 
                             ID_XID      * aXID , 
                             mmcSession  *aSession );

    static IDE_RC commitXA( mmcTransObj * aTrans,
                            mmcSession  * aSession,
                            UInt aTransReleasePolicy );
    
    static IDE_RC commit( mmcTransObj* aTrans,
                          mmcSession * aSession,
                          UInt         aTransReleasePolicy = SMI_RELEASE_TRANSACTION );

    static IDE_RC commit4Prepare( mmcTransObj                       * aTrans,
                                  mmcSession                        * aSession,
                                  mmcTransEndAction                   aTransEndAction );

    static IDE_RC rollback4Prepare( mmcTransObj                        * aTrans,
                                    mmcSession                         * aSession,
                                    mmcTransEndAction                    aTransEndAction );

    static IDE_RC commitForceDatabaseLink( mmcTransObj * aTrans,
                                           mmcSession  * aSession,
                                           UInt aTransReleasePolicy = SMI_RELEASE_TRANSACTION );

    static IDE_RC rollbackXA( mmcTransObj * aTrans,
                              mmcSession  * aSession,
                              UInt aTransReleasePolicy );

    static IDE_RC rollback( mmcTransObj * aTrans,
                            mmcSession  * aSession,
                            const SChar * aSavePoint,
                            UInt          aTransReleasePolicy = SMI_RELEASE_TRANSACTION );
    static IDE_RC rollbackForceDatabaseLink( mmcTransObj * aTrans,
                                             mmcSession * aSession,
                                             UInt aTransReleasePolicy = SMI_RELEASE_TRANSACTION );
    static IDE_RC savepoint( mmcTransObj * aTrans,
                             mmcSession  * aSession,
                             const SChar * aSavePoint );

    /* BUG-46785 Shard statement partial rollback */
    static IDE_RC shardStmtPartialRollback( mmcTransObj * aTrans, mmcSession * aSession );

    static IDE_RC rollbackRaw( mmcTransObj * aTrans,
                               mmcSession  * aSession,
                               ULong       * aEventFlag,
                               UInt          aTransReleasePolicy );

    static IDE_RC commitRaw( mmcTransObj * aTrans,
                             mmcSession  * aSession,
                             ULong       * aEventFlag,
                             UInt          aTransReleasePolicy,
                             smSCN       * aCommitSCN );

    static void   beginRaw( mmcTransObj * aTrans,
                            idvSQL * aStatistics,
                            UInt     aFlag,
                            ULong  * aSessionEventFlag );

    static void clearAndSetSessionInfoAfterBegin( mmcSession  * aSession,
                                                  mmcTransObj * aTrans );

    static inline smiTrans*     getSmiTrans(mmcTransObj * aObj);
    static inline smTID         getTransID(mmcTransObj * aObj);
    static inline UInt          getFirstUpdateTime(mmcTransObj * aObj);
    static inline smiStatement* getSmiStatement(mmcTransObj * aObj);

    static inline IDE_RC    isReadOnly(mmcTransObj * aObj, idBool * aIsReadOnly);
    static inline void      reservePsmSvp(mmcTransObj * aObj, idBool aIsShard);
    static inline void      clearPsmSvp(mmcTransObj * aObj);
    static inline IDE_RC    abortToPsmSvp(mmcTransObj * aObj);
    static inline idBool    isShardPsmSvpReserved(mmcTransObj * aObj);
    static inline idBool    isReusableRollback(mmcTransObj * aObj);
    static inline void      attachXA(mmcTransObj * aTrans, UInt aSlotID);

    /* PROJ-2701 online data rebuild & sharding n-transaction sharing */
    static inline SChar*    getShardNodeName(mmcTransObj * aTrans);
    static inline void      setTransShardNodeName(mmcTransObj * aTrans, SChar * aNodeName);
    static inline void      unsetTransShardNodeName(mmcTransObj * aTrans);
    static void             removeDelegatedSession(mmcTransShareInfo * aShareInfo, mmcSession * aSession);
    static void             addDelegatedSession(mmcTransShareInfo * aShareInfo, mmcSession * aSession);
    
    static inline IDE_RC    collectPrepareSCN( mmcSession * aSession, smSCN * aPrepareSCN );
    static inline void      deployGlobalCommitSCN( mmcSession * aSession, smSCN * aGlobalCommitSCN );

    static void fixSharedTrans( mmcTransObj * aTrans,
                                mmcSessID     aSessionID );
    static IDE_RC fixSharedTrans4Statement( mmcTransObj     * aTrans,
                                            mmcSession      * aSession,
                                            mmcTransFixFlag   aFlag );
    static void pauseFix( mmcTransObj          * aTrans,
                          mmcTxConcurrencyDump * aDump,
                          mmcSessID              aSessionID );
    static void resumeFix( mmcTransObj          * aTrans,
                           mmcTxConcurrencyDump * aDump,
                           mmcSessID              aSessionID );
    static void unfixSharedTrans( mmcTransObj *aTrans, mmcSessID aSessionID );
    static idBool isSharableTransBegin( mmcTransObj * aObj );

    static inline idBool isShareableTrans(mmcTransObj *aTrans)
    {
        idBool sIsShared = ID_FALSE;

        if ( aTrans != NULL )
        {
            if ( aTrans->mShareInfo != NULL )
            {
                sIsShared = ID_TRUE;
            }
        }
        return sIsShared;
    }
    static inline idBool isUserConnectedNode( mmcTransObj * aObj )
    {
        idBool sRet = ID_FALSE;

        IDE_DASSERT( aObj->mShareInfo != NULL );
        if ( aObj->mShareInfo != NULL )
        {
            sRet = ( aObj->mShareInfo->mTxInfo.mUserSession != NULL )
                   ? ID_TRUE : ID_FALSE;
        }

        return sRet;
    }
    static inline void setLocalTransactionBroken( mmcTransObj * aObj,
                                                  mmcSessID     aSessionID,
                                                  idBool        aBroken )
    {
        /* fixSharedTrans 또는
         * mmcTransObj->mShareInfo->mConcurrency.mMutex 에 의해 보호되어야 한다.
         */
#if defined(DEBUG)
        mmcTxConcurrency * sConcurrency = NULL; 
        idBool             sIsLocked    = ID_FALSE;

        if ( aObj->mShareInfo != NULL )
        {
            sConcurrency = &aObj->mShareInfo->mConcurrency;

            IDE_ASSERT( sConcurrency->mMutex.trylock( sIsLocked ) == IDE_SUCCESS );
            if ( sIsLocked == ID_TRUE )
            {
                IDE_ASSERT( sConcurrency->mFixCount > 0 );
                IDE_ASSERT( sConcurrency->mFixOwner == aSessionID );
                /* fixSharedTrans 에 의해 보호되어 있다.
                 * 문제없음.
                 */
                IDE_ASSERT( sConcurrency->mMutex.unlock() == IDE_SUCCESS );
            }
            else
            {
                /* mmcTransObj->mShareInfo->mConcurrency.mMutex 에 의해 보호되어 있다.
                 * 내 세션에서 mutex 잠금을 했는지 알수 없다.
                 * 추후에는 fixSharedTrans 처럼 Session ID 를 기록하고
                 * mutex 만 잠그는 API 가 필요할지도 모르겠다.
                 */
            }
        }
#endif
        ACP_UNUSED( aSessionID );

        if ( aObj->mShareInfo != NULL )
        {
            if ( aBroken == ID_TRUE )
            {
                switch ( aObj->mShareInfo->mTxInfo.mState )
                {
                    case MMC_TRANS_STATE_BEGIN:
                    case MMC_TRANS_STATE_PREPARE:
                        aObj->mShareInfo->mTxInfo.mIsBroken = aBroken;
                        break;
                    default:
                        break;
                }
            }
            else
            {
                aObj->mShareInfo->mTxInfo.mIsBroken = aBroken;
            }
        }
    }
    static inline idBool getLocalTransactionBroken( mmcTransObj * aObj )
    {
        if ( aObj->mShareInfo != NULL )
        {
            return aObj->mShareInfo->mTxInfo.mIsBroken;
        }

        return ID_FALSE;
    }

    static void shardedTransTrace( mmcSession *aSession,
                                   mmcTransObj *aTrans,
                                   const SChar *aStr,
                                   ID_XID      * aXID = NULL );

    static void   setDDLStatementCount( mmcTransObj *aTrans, UInt aCount );
    static UInt   getDDLStatementCount( mmcTransObj *aTrans );

    static const SChar *decideTotalRollback(mmcTransObj *aTrans, const SChar *aSavePoint);

private:

    /* PROJ-2701 online data rebuild & sharding n-transaction sharing */
    static IDE_RC commitShareableTrans( mmcTransObj * aTrans,
                                        mmcSession  * aSession,
                                        UInt          aTransReleasePolicy,
                                        smSCN       * aCommitSCN,
                                        mmcTransEndAction   aTransEndAction );

    static IDE_RC commitLocal(mmcTransObj * aTrans,
                              mmcSession  * aSession,
                              UInt          aTransReleasePolicy = SMI_RELEASE_TRANSACTION,
                              idBool        aIsSqlPrepare = ID_FALSE,
                              mmcTransEndAction aTransEndAction = MMC_TRANS_END );

    static IDE_RC rollbackLocal(mmcTransObj * aTrans,
                                mmcSession * aSession,
                                const SChar * aSavePoint,
                                UInt aTransReleasePolicy = SMI_RELEASE_TRANSACTION,
                                idBool aIsSqlPrepare = ID_FALSE,
                                mmcTransEndAction aTransEndAction = MMC_TRANS_END );

    static idBool isUsableNEqualXID( ID_XID * aTargetXID, ID_XID * aSourceXID );

    static IDE_RC findPreparedTrans(ID_XID * aXID,
                                    idBool * aFound,
                                    iduList * aSlotIDList = NULL);

    static IDE_RC doAfterCommit( mmcSession * aSession,
                                 UInt         aTransReleasePolicy,
                                 idBool       aIsSqlPrepare,
                                 smSCN      * aCommitSCN,
                                 ULong        aNewSMN );

    static IDE_RC doAfterRollback( mmcSession  * aSession,
                                   UInt          aTransReleasePolicy,
                                   idBool        aIsSqlPrepare,
                                   const SChar * aSavePoint );

    static void endPendingTrace( mmcSession    * aSession,
                                 smiTrans      * aSmiTrans,
                                 ID_XID        * aXID,
                                 idBool          aIsCommit,
                                 const SChar   * aStr );

    static const SChar * getSharedTransStateString( mmcTransObj * aTrans );

    static void setGlobalTxID4Trans( const SChar * aSavepoint, mmcSession * aSession );
};

/* PROJ-2701 online data rebuild */
inline smiTrans* mmcTrans::getSmiTrans(mmcTransObj * aObj)
{
    return &(aObj->mSmiTrans);
}

inline smTID mmcTrans::getTransID(mmcTransObj * aObj)
{
    return aObj->mSmiTrans.getTransID();
}

inline UInt mmcTrans::getFirstUpdateTime(mmcTransObj * aObj)
{
    return aObj->mSmiTrans.getFirstUpdateTime();
}

inline smiStatement* mmcTrans::getSmiStatement(mmcTransObj * aObj)
{
    return aObj->mSmiTrans.getStatement();
}

inline IDE_RC mmcTrans::isReadOnly(mmcTransObj * aObj, idBool * aIsReadOnly)
{
    return aObj->mSmiTrans.isReadOnly(aIsReadOnly);
}

inline void mmcTrans::reservePsmSvp(mmcTransObj * aObj, idBool aIsShard)
{
    aObj->mSmiTrans.reservePsmSvp(aIsShard);
}

inline void mmcTrans::clearPsmSvp(mmcTransObj * aObj)
{
    aObj->mSmiTrans.clearPsmSvp();
}

inline IDE_RC mmcTrans::abortToPsmSvp(mmcTransObj * aObj)
{
    return aObj->mSmiTrans.abortToPsmSvp();
}

// TASK-7244 PSM partial rollback in Sharding
inline idBool mmcTrans::isShardPsmSvpReserved(mmcTransObj * aObj)
{
    return aObj->mSmiTrans.isShardPsmSvpReserved();
}

inline idBool mmcTrans::isReusableRollback(mmcTransObj * aObj)
{
    return aObj->mSmiTrans.isReusableRollback();
}

inline void mmcTrans::attachXA(mmcTransObj * aTrans, UInt aSlotID)
{
    IDE_ASSERT(mmcTrans::getSmiTrans(aTrans)->attach(aSlotID) == IDE_SUCCESS);
}

inline SChar* mmcTrans::getShardNodeName(mmcTransObj * aTrans)
{
    IDE_DASSERT(aTrans->mShareInfo != NULL);
    return aTrans->mShareInfo->mTxInfo.mNodeName;
}

inline void mmcTrans::setTransShardNodeName(mmcTransObj * aTrans, SChar * aNodeName)
{
    IDE_DASSERT(aTrans->mShareInfo != NULL);
    idlOS::strncpy(aTrans->mShareInfo->mTxInfo.mNodeName, aNodeName, SDI_NODE_NAME_MAX_SIZE);
    aTrans->mShareInfo->mTxInfo.mNodeName[SDI_NODE_NAME_MAX_SIZE] = '\0';
}

inline void mmcTrans::unsetTransShardNodeName(mmcTransObj * aTrans)
{
    IDE_DASSERT(aTrans->mShareInfo != NULL);
    aTrans->mShareInfo->mTxInfo.mNodeName[SDI_NODE_NAME_MAX_SIZE] = '\0';
}

#endif
