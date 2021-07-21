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
 * $Id: smxSavepointMgr.cpp 90824 2021-05-13 05:35:21Z minku.kang $
 **********************************************************************/

#include <idl.h>
#include <idErrorCode.h>
#include <ideErrorMgr.h>
#include <smErrorCode.h>
#include <smm.h>
#include <smr.h>
#include <smxTrans.h>
#include <smxSavepointMgr.h>
#include <smxReq.h>
#include <svrRecoveryMgr.h>

iduMemPool smxSavepointMgr::mSvpPool;

IDE_RC smxSavepointMgr::initializeStatic()
{
    return mSvpPool.initialize(IDU_MEM_SM_SMX,
                               (SChar*)"SAVEPOINT_MEMORY_POOL",
                               ID_SCALABILITY_SYS,
                               ID_SIZEOF(smxSavepoint),
                               SMX_SAVEPOINT_POOL_SIZE,
                               IDU_AUTOFREE_CHUNK_LIMIT,		/* ChunkLimit */
                               ID_TRUE,							/* UseMutex */
                               IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte */
                               ID_FALSE,						/* ForcePooling */
                               ID_TRUE,							/* GarbageCollection */
                               ID_TRUE,                         /* HWCacheLine */
                               IDU_MEMPOOL_TYPE_LEGACY          /* mempool type */);
}

IDE_RC smxSavepointMgr::destroyStatic()
{
    return mSvpPool.destroy();
}

void smxSavepointMgr::initialize(smxTrans * /*a_pTrans*/)
{

    /* Explicit Savepoint List를 초기화 한다. */
    mExpSavepoint.mPrvSavepoint = &mExpSavepoint;
    mExpSavepoint.mNxtSavepoint = &mExpSavepoint;

    /* Implicit Savepoint List를 초기화 한다. */
    mImpSavepoint.mPrvSavepoint = &mImpSavepoint;
    mImpSavepoint.mNxtSavepoint = &mImpSavepoint;

    /* setReplSavepoint에서 참조하기때문에 초기화해야 합니다. */
    mImpSavepoint.mReplImpSvpStmtDepth = 0;

    mReservePsmSvp = ID_FALSE;

    // TASK-7244 PSM partial rollback in Sharding
    mReserveShardPsmSvp = ID_FALSE;

    mPsmSavepoint.mOIDNode = NULL;

    /* BUG-45368 */
    mIsUsedPreparedSP = ID_FALSE;
    
    mDDLInfoCount = 0;
}

IDE_RC smxSavepointMgr::destroy( void * aSmiTrans )
{

    smxSavepoint *sSavepoint;
    smxSavepoint *sNxtSavepoint;

    //free save point buffer
    sSavepoint = mExpSavepoint.mNxtSavepoint;

    while(sSavepoint != &mExpSavepoint)
    {
        if ( sSavepoint->mDDLTargetTableInfo != NULL )
        {
            smiTrans::removeDDLTargetTableInfo( (smiTrans *)aSmiTrans, sSavepoint->mDDLTargetTableInfo );
            sSavepoint->mDDLTargetTableInfo = NULL;    
            mDDLInfoCount -= 1;
        }

        sNxtSavepoint = sSavepoint->mNxtSavepoint;

        if ( sSavepoint != &mPreparedSP )
        {
            IDE_TEST(mSvpPool.memfree(sSavepoint) != IDE_SUCCESS);
        }
        else
        {
            /* nothing to do */
        }

        sSavepoint = sNxtSavepoint;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/*************************************************************************
 * Description: aSavepointName에 해당하는 Explicit Savepoint를
 *              Transaction의 Explicit Savepoint List에서 찾는다. 만약
 *              aDoRemove가 ID_TRUE이면 Savepoint List에서 제거한다.
 *              찾은 Explicit Savepoint를 Return한다. 없으면 NULL
 *
 * aSavepointName - [IN] Savepoint 이름.
 * aDoRemove      - [IN] aSavepointName에 해당하는 Savepoint를 Explicit SVP
 *                       List에서 제거를 원하면 ID_TRUE, 아니면 ID_FALSE
 * ***********************************************************************/
smxSavepoint* smxSavepointMgr::findInExpSvpList(const SChar *aSavepointName,
                                                idBool       aDoRemove)
{
    smxSavepoint *sCurSavepoint = NULL;
    smxSavepoint *sPrvSavepoint = NULL;

    sPrvSavepoint = mExpSavepoint.mPrvSavepoint;

    while(sPrvSavepoint != &mExpSavepoint)
    {
        if(idlOS::strcmp(aSavepointName, sPrvSavepoint->mName) == 0)
        {
            if(aDoRemove == ID_TRUE)
            {
                /* Explicit Savepoint List에서 제거한다. */
                removeFromLst(sPrvSavepoint);
            }

            break;
        }

        sCurSavepoint = sPrvSavepoint;
        sPrvSavepoint = sCurSavepoint->mPrvSavepoint;
    }

    if(sPrvSavepoint == &mExpSavepoint)
    {
        sPrvSavepoint = NULL;
    }

    return sPrvSavepoint;
}

/*************************************************************************
 * Description: aSavepointName에 해당하는 Explicit Savepoint의 존재 여부를
 *              리턴한다.
 *
 * aSavepointName - [IN] Savepoint 이름.
 * ***********************************************************************/
idBool smxSavepointMgr::isExistExpSavepoint(const SChar *aSavepointName)
{
    /* BUG-48489 */
    return ( findInExpSvpList( aSavepointName, ID_FALSE ) != NULL ) ? ID_TRUE : ID_FALSE;
}

/*************************************************************************
 * Description: aStmtDepth에 해당하는 Implicit Savepoint를
 *              Transaction의 Implicit Savepoint List에서 찾는다.
 *              찾은 Implicit Savepoint를 Return한다. 없으면 NULL
 *
 * aStmtDepth - [IN] Savepoint가 설정될때의 Statement의 Depth.
 * ***********************************************************************/
smxSavepoint* smxSavepointMgr::findInImpSvpList(UInt aStmtDepth)
{
    smxSavepoint *sCurSavepoint = NULL;
    smxSavepoint *sPrvSavepoint = NULL;

    sPrvSavepoint = mImpSavepoint.mPrvSavepoint;

    while(sPrvSavepoint != &mImpSavepoint)
    {
        if( sPrvSavepoint->mStmtDepth == aStmtDepth)
        {
            break;
        }

        sCurSavepoint = sPrvSavepoint;
        sPrvSavepoint = sCurSavepoint->mPrvSavepoint;
    }

    if(sPrvSavepoint == &mImpSavepoint)
    {
        sPrvSavepoint = NULL;
    }

    return sPrvSavepoint;
}

/*************************************************************************
 * Description: aStmtDepth 중 가장 큰 값을 반환한다.
 * ***********************************************************************/
UInt smxSavepointMgr::getStmtDepthFromImpSvpList()
{
    smxSavepoint *sCurSavepoint = NULL;
    smxSavepoint *sPrvSavepoint = NULL;
    UInt          sStmtDepth    = 0;

    sPrvSavepoint = mImpSavepoint.mPrvSavepoint;

    while(sPrvSavepoint != &mImpSavepoint)
    {
        if( sPrvSavepoint->mStmtDepth > sStmtDepth)
        {
            sStmtDepth = sPrvSavepoint->mStmtDepth;
        }

        sCurSavepoint = sPrvSavepoint;
        sPrvSavepoint = sCurSavepoint->mPrvSavepoint;
    }

    return sStmtDepth;
}

IDE_RC smxSavepointMgr::setExpSavepoint( smxTrans    * aTrans,
                                         const SChar * aSavepointName,
                                         smOID         aOldTableOID,
                                         UInt          aOldPartOIDCount,
                                         smOID       * aOldPartOIDArray,
                                         smOID         aNewTableOID,
                                         UInt          aNewPartOIDCount,
                                         smOID       * aNewPartOIDArray,
                                         smxOIDNode  * aOIDNode,
                                         smLSN       * aLSN,
                                         svrLSN        aVolLSN,
                                         ULong         aLockSequence )

{
    smxSavepoint          * sSavepoint;
    smiDDLTargetTableInfo * sDDLTargetTableInfo = NULL;

    sSavepoint = findInExpSvpList( aSavepointName, ID_TRUE );

    if( sSavepoint == NULL )
    {
        //alloc new savepoint
        /* smxSavepointMgr_setExpSavepoint_alloc_Savepoint.tc */
        IDU_FIT_POINT("smxSavepointMgr::setExpSavepoint::alloc::Savepoint");
        IDE_TEST( alloc( &sSavepoint ) != IDE_SUCCESS );

        if ( idlOS::strcmp( aSavepointName, SM_DDL_INFO_SAVEPOINT ) != 0 )
        {
            sSavepoint->mDDLTargetTableInfo = NULL;
            idlOS::strncpy( sSavepoint->mName, 
                            aSavepointName,
                            SMX_MAX_SVPNAME_SIZE );
        }
        else
        {
            IDE_TEST( smiTrans::backupDDLTargetTableInfo( (smiTrans*)aTrans->mSmiTransPtr,
                                                          aOldTableOID,
                                                          aOldPartOIDCount,
                                                          aOldPartOIDArray,
                                                          aNewTableOID,
                                                          aNewPartOIDCount,
                                                          aNewPartOIDArray,
                                                          &sDDLTargetTableInfo ) 
                      != IDE_SUCCESS );
            sSavepoint->mDDLTargetTableInfo = sDDLTargetTableInfo;

            mDDLInfoCount += 1;
            idlOS::snprintf( sSavepoint->mName, SMX_MAX_SVPNAME_SIZE,
                             "%s_%"ID_UINT32_FMT"\0",
                             aSavepointName,
                             mDDLInfoCount );
        }
    }

    addToSvpListTail( &mExpSavepoint, sSavepoint );

    sSavepoint->mUndoLSN      = *aLSN;
    sSavepoint->mVolatileLSN  = aVolLSN;
    sSavepoint->mOIDNode      = aOIDNode;
    sSavepoint->mOffset       = aOIDNode->mOIDCnt;
    sSavepoint->mLockSequence = aLockSequence;
    //PROJ-1608 recovery From Replication (recovery를 위해 svp 로그 기록)
    if( (aTrans->mLogTypeFlag == SMR_LOG_TYPE_NORMAL) ||
        (aTrans->mLogTypeFlag == SMR_LOG_TYPE_REPL_RECOVERY) )
    {
        IDE_TEST( smrLogMgr::writeSetSvpLog( NULL, /* idvSQL* */
                                             aTrans, aSavepointName )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sDDLTargetTableInfo != NULL )
    {
        smiTrans::removeDDLTargetTableInfo( (smiTrans*)aTrans->mSmiTransPtr, sDDLTargetTableInfo );
        sDDLTargetTableInfo = NULL;
    }

    return IDE_FAILURE;
}

void smxSavepointMgr::updateOIDList(smxTrans     *aTrans,
                                    smxSavepoint *aSavepoint)
{
    smxOIDNode   *sCurOIDNode;
    UInt          i;
    smxOIDInfo   *sArrOIDList;
    UInt          sOffset;
    smxOIDNode   *sOIDNodeListHead;

    sCurOIDNode  = aSavepoint->mOIDNode;
    sOffset      = aSavepoint->mOffset;
    sOIDNodeListHead = &(aTrans->mOIDList->mOIDNodeListHead);

    IDE_ASSERT( sCurOIDNode != NULL );

    if(sCurOIDNode == sOIDNodeListHead)
    {
        sCurOIDNode = sOIDNodeListHead->mNxtNode;
    }

    while(sCurOIDNode != sOIDNodeListHead)
    {
        sArrOIDList = sCurOIDNode->mArrOIDInfo;

        for(i = sOffset; i < sCurOIDNode->mOIDCnt; i++)
        {
            /*
              - BUG-14127
              Savepoint로 Partial Rollback시 한번 처리된 OID의
              Flag값이 두번 처리되지 않도록 처리.
            */
            if((sArrOIDList[i].mFlag & SM_OID_ACT_SAVEPOINT)
               == SM_OID_ACT_SAVEPOINT)
            {
                /*
                   SM_OID_ACT_SAVEPOINT값을 clear시킨다. 이는
                   다시 현재 savepoint 이전으로 rollback시 다시
                   이 routine으로 들어오는 것을 방지하기 위함이다.
                */
                sArrOIDList[i].mFlag &=
                    smxOidSavePointMask[sArrOIDList[i].mFlag
                         & SM_OID_OP_MASK].mAndMask;

                sArrOIDList[i].mFlag |=
                    smxOidSavePointMask[sArrOIDList[i].mFlag
                         & SM_OID_OP_MASK].mOrMask;
            }
        }

        sCurOIDNode = sCurOIDNode->mNxtNode;
        sOffset = 0;
    }


    /* BUG-32737 [sm-transation] The ager may ignore aging job that made
     * from the PSM failure.
     * PSM Abort일때도 Aging할 수 있도록, setAgingFlag 함 */
    // To Fix BUG-14126
    // Insert OID만 있을 경우, need Aging을 True로 변경해야  Ager에 달림
    aTrans->mOIDList->setAgingFlag( ID_TRUE );

}

IDE_RC smxSavepointMgr::abortToExpSavepoint(smxTrans    *aTrans,
                                            const SChar *aSavepointName)
{
    smxSavepoint *sCurSavepoint;
    smxSavepoint *sNxtSavepoint;

    sCurSavepoint = findInExpSvpList(aSavepointName, ID_FALSE);

    IDE_TEST_RAISE(sCurSavepoint == NULL, err_savepoint_NotFound);

    updateOIDList(aTrans, sCurSavepoint);

    // PROJ-1553 Replication self-deadlock
    // abort to savepoint시 undo작업을 하기 전에 abortToSavepoint log를
    // 먼저 찍도록 바꾼다.
    // abortToSavepoint log는 replication에서만 사용하기 때문에 문제가 없다.
    // PROJ-1608 recovery From Replication (recovery를 위해 svp 로그 기록)
    if( (aTrans->mLogTypeFlag == SMR_LOG_TYPE_NORMAL) ||
        (aTrans->mLogTypeFlag == SMR_LOG_TYPE_REPL_RECOVERY) )
    {
        IDE_TEST(smrLogMgr::writeAbortSvpLog(NULL, /* idvSQL* */
                                             aTrans, aSavepointName)
                 != IDE_SUCCESS);
    }

    //abort Transaction...
    IDE_TEST(smrRecoveryMgr::undoTrans( aTrans->mStatistics,
                                        aTrans,
                                        &sCurSavepoint->mUndoLSN)
             != IDE_SUCCESS);

    /* PROJ-1594 Volatile TBS */
    /* Volatile TBS에 대해서도 undo를 수행한다. */
    IDE_TEST(svrRecoveryMgr::undoTrans( &aTrans->mVolatileLogEnv,
                                        sCurSavepoint->mVolatileLSN )
             != IDE_SUCCESS);

    // BUG-22576
    // partial abort하기 전에 private page list를 table에
    // 달아야 한다.
    IDE_TEST(aTrans->addPrivatePageListToTableOnPartialAbort()
             != IDE_SUCCESS);

    if ( mDDLInfoCount > 0 )
    {
        rollbackDDLTargetTableInfo( aSavepointName );
    }

    IDE_TEST( smLayerCallback::partialItemUnlock( aTrans->mSlotN,
                                                  sCurSavepoint->mLockSequence,
                                                  ID_FALSE )
              != IDE_SUCCESS );

    sCurSavepoint = sCurSavepoint->mNxtSavepoint;

    while(sCurSavepoint != &mExpSavepoint)
    {
        sNxtSavepoint = sCurSavepoint->mNxtSavepoint;

        freeMem(sCurSavepoint);

        sCurSavepoint = sNxtSavepoint;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_savepoint_NotFound);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_NotFoundSavepoint));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*************************************************************************
 * Description: Implicit savepoint를 설정합니다. Explicit과 달리 Savepoint
 *              정보를 저장하기 위해 별도의 메모리를 설정하지 않고
 *              Statement에 있는 Savepoint정보를 이용합니다.
 *
 *              1: Savepoint를 초기화 한다.
 *              2: Implicit Savepoint List에 추가한다.
 *
 * aSavepoint    - [IN] Savepoint가 설정될때의 Statement의 Depth.
 * aStmtDepth    - [IN] Statment Depth
 * aOIDNode      - [IN] Implicit Savepoint가 시작시의 OID List의 마지막 Node
 * aLSN          - [IN] Implicit Savepoint가 설정될때 Transaction이 기록한
 *                      마지막 Log LSN
 * aLockSequence - [IN] Transction이 Lock을 잡기 위해 마지막으로 사용한 Lock
 *                      Sequence
 * ***********************************************************************/
IDE_RC smxSavepointMgr::setImpSavepoint( smxSavepoint ** aSavepoint,
                                         UInt            aStmtDepth,
                                         smxOIDNode    * aOIDNode,
                                         smLSN         * aLSN,
                                         svrLSN          aVolLSN,
                                         ULong           aLockSequence )
{
    smxSavepoint *sNewSavepoint;

    IDE_ASSERT( aSavepoint != NULL );

    /* Implicit Savepoint는 update statement가 begin시 설정되고
     * End시 삭제된다. 때문에 중복된 statment가 리스트에 이미
     * 존재하는 경우는 없다. */
    IDE_DASSERT( findInImpSvpList( aStmtDepth ) == NULL );

    /* smxSavepointMgr_setImpSavepoint_alloc_NewSavepoint.tc */
    IDU_FIT_POINT("smxSavepointMgr::setImpSavepoint::alloc::NewSavepoint");
    IDE_TEST( alloc( &sNewSavepoint ) != IDE_SUCCESS );

    sNewSavepoint->mUndoLSN       = *aLSN;
    sNewSavepoint->mOIDNode       = aOIDNode;
    sNewSavepoint->mOffset        = aOIDNode->mOIDCnt;
    sNewSavepoint->mLockSequence  = aLockSequence;
    sNewSavepoint->mDDLTargetTableInfo = NULL;

    /* BUG-17033: 최상위 Statement가 아닌 Statment에 대해서도
     * Partial Rollback을 지원해야 합니다. */
    sNewSavepoint->mStmtDepth   = aStmtDepth;

    sNewSavepoint->mReplImpSvpStmtDepth
        = SMI_STATEMENT_DEPTH_NULL;

    /* PROJ-1594 Volatile TBS */
    sNewSavepoint->mVolatileLSN = aVolLSN;

    addToSvpListTail( &mImpSavepoint, sNewSavepoint );

    *aSavepoint = sNewSavepoint;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*************************************************************************
 * Description: Implicit Savepoint로그가 설정된 이후 처음으로 Replication
 *              대상 로그(Sender가 보내야 할 로그)가 해당 Transaction의에해
 *              기록될 경우 호출된다. Implicit Savepoint에 mReplImpSvpStmtDepth
 *              에 이전 Implicit Savepoint중 가장 큰 mReplImpSvpStmtDepth + 1을 해서
 *              기록한다. 그리고 이전 Savepoint의 mReplImpSvpStmtDepth이
 *              SMI_STATEMENT_DEPTH_NULL이면 자신의 mReplImpSvpStmtDepth과
 *              동일하게 바꾼다. 이전으로 이동하면서 SMI_STATEMENT_DEPTH_NULL
 *              가 아닐때까지 반복한다.
 *
 *              이에 대한 자세한 사항은 smxDef.h의 smxSavepoint를 참조하시기 바랍니다.
 *
 * aSavepoint   - [IN] Replication Log를 기록한 Statement의 Implicit Savepoint.
 *                     보통 현재 Transaction의 마지막으로 시작한 Savepoint
 * ***********************************************************************/
void smxSavepointMgr::setReplSavepoint( smxSavepoint *aSavepoint )
{
    smxSavepoint *sCurSavepoint;

    sCurSavepoint = aSavepoint->mPrvSavepoint;

    /* 이전 SVP중에서 가장 큰 mReplImpSvpStmtDepth를 찾는다. */
    while( sCurSavepoint != &mImpSavepoint )
    {
        if( sCurSavepoint->mReplImpSvpStmtDepth
            != SMI_STATEMENT_DEPTH_NULL )
        {
            break;
        }

        sCurSavepoint = sCurSavepoint->mPrvSavepoint;
    }

    aSavepoint->mReplImpSvpStmtDepth =
        sCurSavepoint->mReplImpSvpStmtDepth + 1;

    sCurSavepoint = sCurSavepoint->mNxtSavepoint;

    while( sCurSavepoint != aSavepoint )
    {
        IDE_ASSERT( sCurSavepoint->mReplImpSvpStmtDepth ==
                    SMI_STATEMENT_DEPTH_NULL );
        sCurSavepoint->mReplImpSvpStmtDepth =
            aSavepoint->mReplImpSvpStmtDepth;

        sCurSavepoint = sCurSavepoint->mNxtSavepoint;
    }
}

/*************************************************************************
 * Description: Implicit Savepoint를 Implicit Savepoint List에서 제거한다.
 *
 * aSavepoint   - [IN] 제거할 Implicit Savepoint.
 * ***********************************************************************/
IDE_RC smxSavepointMgr::unsetImpSavepoint( smxSavepoint* aSavepoint )
{
    IDE_TEST( freeMem( aSavepoint ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*************************************************************************
 * Description: Implicit Savepoint까지 Partial Rollback한다.
 *
 * aTrans     - [IN] Transaction Pointer
 * aSavepoint - [IN] Partial Rollback할 Implicit Savepoint
 *************************************************************************/
IDE_RC smxSavepointMgr::abortToImpSavepoint( smxTrans*     aTrans,
                                             smxSavepoint* aSavepoint )
{
    // Abort Transaction...
    updateOIDList( aTrans, aSavepoint );

    /*
       PROJ-1553 Replication self-deadlock
       abort to savepoint시 undo작업을 하기 전에 abortToSavepoint log를
       먼저 찍도록 바꾼다.
       abortToSavepoint log는 replication에서만 사용하기 때문에 문제가 없다.
    */
    //PROJ-1608 recovery From Replication (recovery를 위해 svp 로그 기록)
    if( ( ( aTrans->mLogTypeFlag == SMR_LOG_TYPE_NORMAL ) ||
          ( aTrans->mLogTypeFlag == SMR_LOG_TYPE_REPL_RECOVERY ) ) &&
        /* Replication Implicit SVP Log가 기록된적이 있다. */
        ( aSavepoint->mReplImpSvpStmtDepth != SMI_STATEMENT_DEPTH_NULL ) ) // case 3.
    {
        /* BUG-17033: 최상위 Statement가 아닌 Statment에 대해서도
         * Partial Rollback을 지원해야 합니다.

           Implicit SVP Name: SMR_IMPLICIT_SVP_NAME +
                              Savepoint의 mReplImpSvpStmtDepth값 */
        idlOS::snprintf( aSavepoint->mName,
                         SMX_MAX_SVPNAME_SIZE,
                        "%s%d",
                         SMR_IMPLICIT_SVP_NAME,
                         aSavepoint->mReplImpSvpStmtDepth );

        IDE_TEST( smrLogMgr::writeAbortSvpLog( NULL, /* idvSQL* */
                                               aTrans,
                                               aSavepoint->mName )
                  != IDE_SUCCESS );
    }

    IDE_TEST( smrRecoveryMgr::undoTrans( aTrans->mStatistics,
                                         aTrans,
                                         &(aSavepoint->mUndoLSN) )
              != IDE_SUCCESS );

    /* PROJ-1594 Volatile TBS */
    /* Volatile TBS에 대해서도 undo를 수행한다. */
    IDE_TEST(svrRecoveryMgr::undoTrans(&aTrans->mVolatileLogEnv,
                                       aSavepoint->mVolatileLSN)
             != IDE_SUCCESS);

    // BUG-22576
    // partial abort하기 전에 private page list를 table에
    // 달아야 한다.
    IDE_TEST(aTrans->addPrivatePageListToTableOnPartialAbort()
             != IDE_SUCCESS);

    IDE_TEST( aTrans->executePendingList(ID_FALSE) != IDE_SUCCESS );

    IDE_TEST( smLayerCallback::partialItemUnlock( aTrans->mSlotN,
                                                  aSavepoint->mLockSequence,
                                                  ID_FALSE )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 현재 Transaction에서 마지막으로 Begin한 Normal Statment에
 *               Replication을 위한 Savepoint가 설정되었는지 Check하고
 *               설정이 안되어 있다면 Replication을 위해서 Savepoint를
 *               설정한다. 설정되어 있으면 ID_TRUE, else ID_FALSE
 *
 ***********************************************************************/
idBool smxSavepointMgr::checkAndSetImpSVP4Repl()
{
    smxSavepoint *sCurSavepoint;
    idBool        sSetImpSVP = ID_TRUE;

    /* 마지막으로 Begin한 Savepoint를 Implicit Svp List에서
     * 가져온다.*/
    sCurSavepoint = mImpSavepoint.mPrvSavepoint;

    /* 마지막 Statment만이 Update가 가능하기 때문에 마지막 Savepoint
       를 조사하면 된다 */
    IDE_ASSERT( sCurSavepoint != NULL );

    if( sCurSavepoint->mReplImpSvpStmtDepth == SMI_STATEMENT_DEPTH_NULL )
    {
        /* Replication을 위해서 Implicit SVP가 설정된 적이 없기때문에
         * ID_FALSE */
        sSetImpSVP = ID_FALSE;

        /*  SMI_STATEMENT_DEPTH_NULL이 아니면 Implici SVP이후로
         *  Savepoint를 설정한 것이다. */
        setReplSavepoint( sCurSavepoint );
    }

    return sSetImpSVP;
}

void smxSavepointMgr::reservePsmSvp( smxOIDNode  *aOIDNode,
                                     smLSN       *aLSN,
                                     svrLSN       aVolLSN,
                                     ULong        aLockSequence,
                                     idBool       aIsShard )
{
    mPsmSavepoint.mUndoLSN      = *aLSN;
    mPsmSavepoint.mVolatileLSN  = aVolLSN;
    mPsmSavepoint.mOIDNode      = aOIDNode;
    mPsmSavepoint.mOffset       = aOIDNode->mOIDCnt;
    mPsmSavepoint.mLockSequence = aLockSequence;

    mReservePsmSvp = ID_TRUE;

    // TASK-7244 PSM partial rollback in Sharding
    if ( aIsShard == ID_TRUE )
    {
        mReserveShardPsmSvp = ID_TRUE;
    }
}

IDE_RC smxSavepointMgr::writePsmSvp( smxTrans* aTrans )
{
    IDE_DASSERT( mReservePsmSvp == ID_TRUE );
    //PROJ-1608 recovery From Replication (recovery를 위해 svp 로그 기록)
    if( (aTrans->mLogTypeFlag == SMR_LOG_TYPE_NORMAL) ||
        (aTrans->mLogTypeFlag == SMR_LOG_TYPE_REPL_RECOVERY) )
    {
        IDE_TEST(smrLogMgr::writeSetSvpLog( NULL, /* idvSQL* */
                                            aTrans,
                                            SMX_PSM_SAVEPOINT_NAME )
                 != IDE_SUCCESS);
    }

    mReservePsmSvp = ID_FALSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smxSavepointMgr::abortToPsmSvp(smxTrans* aTrans)
{
    /* BUG-45106 reservePsmSvp를 수행하지 않고 해당 함수에 들어올 경우
                 리스트 탐색 중 무한 루프를 돌수 있으므로 실패 처리 해야 한다. */
    if ( mPsmSavepoint.mOIDNode == NULL )
    {   
        ideLog::log( IDE_SM_0, "Invalid PsmSavepoint" );
        IDE_ASSERT( 0 );
    }   
    else
    {   
        /* do nothing... */
    }   

    updateOIDList(aTrans, &mPsmSavepoint);

    // PROJ-1553 Replication self-deadlock
    // abort to savepoint시 undo작업을 하기 전에 abortToSavepoint log를
    // 먼저 찍도록 바꾼다.
    // abortToSavepoint log는 replication에서만 사용하기 때문에 문제가 없다.
    // PROJ-1608 recovery From Replication (recovery를 위해 svp 로그 기록)
    if( ( (aTrans->mLogTypeFlag == SMR_LOG_TYPE_NORMAL) ||
          (aTrans->mLogTypeFlag == SMR_LOG_TYPE_REPL_RECOVERY) ) &&
        (mReservePsmSvp == ID_FALSE) )
    {
        IDE_TEST(smrLogMgr::writeAbortSvpLog( NULL, /* idvSQL* */
                                              aTrans,
                                              SMX_PSM_SAVEPOINT_NAME )
                 != IDE_SUCCESS);
    }

    IDE_TEST(smrRecoveryMgr::undoTrans(aTrans->mStatistics,
                                       aTrans,
                                       &mPsmSavepoint.mUndoLSN)
             != IDE_SUCCESS);

    /* PROJ-1594 Volatile TBS */
    /* Volatile TBS에 대해서도 undo를 수행한다. */
    IDE_TEST(svrRecoveryMgr::undoTrans(&aTrans->mVolatileLogEnv,
                                       mPsmSavepoint.mVolatileLSN)
             != IDE_SUCCESS);

    // BUG-22576
    // partial abort하기 전에 private page list를 table에
    // 달아야 한다.
    IDE_TEST(aTrans->addPrivatePageListToTableOnPartialAbort()
             != IDE_SUCCESS);

    IDE_TEST( smLayerCallback::partialItemUnlock( aTrans->mSlotN,
                                                  mPsmSavepoint.mLockSequence,
                                                  ID_FALSE )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*************************************************************************
 * Description: Implicit Savepoint까지 모든 테이블에 대해 IS락을 해제하고,
 *              temp table일경우는 특별히 IX락도 해제한다.
 *
 * aTrans     - [IN] Savepoint까지
 * aLockSlot  - [IN] Lock Slot Pointer
 * ***********************************************************************/
IDE_RC smxSavepointMgr::unlockSeveralLock( smxTrans *aTrans,
                                           ULong     aLockSequence )
{
    return smLayerCallback::partialItemUnlock( aTrans->mSlotN,
                                               aLockSequence,
                                               ID_TRUE/*Unlock Several Lock*/ );
}

void smxSavepointMgr::rollbackDDLTargetTableInfo( const SChar * aSavepointName )
{
    smxSavepoint *sSavepoint;
    smxSavepoint *sPrvSavepoint;

    //free save point buffer
    sSavepoint = mExpSavepoint.mPrvSavepoint;

    if ( mDDLInfoCount > 0 )
    {
        while ( sSavepoint != &mExpSavepoint )
        {
            if ( sSavepoint->mDDLTargetTableInfo != NULL )
            {
                if ( sSavepoint->mDDLTargetTableInfo->mOldTableInfo != NULL )
                {
                    smiTrans::restoreDDLTargetOldTableInfo( sSavepoint->mDDLTargetTableInfo );
                }
                else if ( sSavepoint->mDDLTargetTableInfo->mNewTableInfo != NULL )
                {
                    smiTrans::destroyDDLTargetNewTableInfo( sSavepoint->mDDLTargetTableInfo );
                }
                else
                {
                    /* nothing to do */
                }

                sSavepoint->mDDLTargetTableInfo = NULL;
                mDDLInfoCount -= 1;
            }

            if ( aSavepointName != NULL )
            {
                if ( idlOS::strcmp( aSavepointName, sSavepoint->mName ) == 0)
                {
                    break;
                }
            }

            sPrvSavepoint = sSavepoint->mPrvSavepoint;
            sSavepoint = sPrvSavepoint;
        }
    }
}
