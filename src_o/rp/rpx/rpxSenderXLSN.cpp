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
 * $Id: rpxSenderXLSN.cpp 90578 2021-04-13 05:32:12Z minku.kang $
 **********************************************************************/

#include <idl.h>
#include <ide.h>

#include <smi.h>
#include <smErrorCode.h>
#include <rpDef.h>
#include <rpuProperty.h>
#include <rpcManager.h>
#include <rpxSender.h>



//===================================================================
//
// Name:          updateXLSN
//
// Return Value:  IDE_SUCCESS/IDE_FAILURE
//
// Argument:      smLSN lsn
//
// Called By:     emptyReplBuffer()
//
// Description:
//
//===================================================================

IDE_RC rpxSender::updateXSN(smSN aSN)
{
    smiTrans          sTrans;
    SInt              sStage = 0;
    idBool            sIsTxBegin = ID_FALSE;
    smiStatement     *spRootStmt;
    UInt              sFlag = 0;

    //----------------------------------------------------------------//
    // no commit log occurred
    //----------------------------------------------------------------//
    if((aSN == SM_SN_NULL) || (aSN == 0))
    {
        return IDE_SUCCESS;
    }

    // Do not need to update XLSN again with the same value
    if(aSN == getRestartSN())
    {
        return IDE_SUCCESS;
    }

    if(isParallelChild() == ID_TRUE)
    {
        return IDE_SUCCESS;
    }

    /* PROJ-1442 Replication Online 중 DDL 허용
     * Service Thread의 Transaction으로 구동되고 있는 경우,
     * Service Thread의 Transaction을 사용한다.
     */
    if(mSvcThrRootStmt != NULL)
    {
        spRootStmt = mSvcThrRootStmt;
        IDU_FIT_POINT( "rpxSender::updateXSN::Erratic::rpERR_ABORT_RP_SENDER_UPDATE_XSN" ); 
        IDE_TEST(rpcManager::updateXSN(spRootStmt,
                                        mMeta.mReplication.mRepName,
                                        aSN)
                 != IDE_SUCCESS);

        mMeta.mReplication.mXSN = aSN;
        setRestartSN(aSN);
    }
    else
    {
        IDE_TEST(sTrans.initialize() != IDE_SUCCESS);
        sStage = 1;

        sFlag = (sFlag & ~SMI_ISOLATION_MASK) | (UInt)RPU_ISOLATION_LEVEL;
        sFlag = (sFlag & ~SMI_TRANSACTION_MASK) | SMI_TRANSACTION_NORMAL;
        sFlag = (sFlag & ~SMI_TRANSACTION_REPL_MASK) | SMI_TRANSACTION_REPL_NONE;
        sFlag = (sFlag & ~SMI_COMMIT_WRITE_MASK) | SMI_COMMIT_WRITE_NOWAIT;

        IDE_TEST(sTrans.begin(&spRootStmt, NULL, sFlag, SMX_NOT_REPL_TX_ID)
                 != IDE_SUCCESS);
        sIsTxBegin = ID_TRUE;
        sStage = 2;

        IDE_TEST(rpcManager::updateXSN(spRootStmt,
                                        mMeta.mReplication.mRepName,
                                        aSN)
                 != IDE_SUCCESS);

        sStage = 1;
        IDE_TEST(sTrans.commit() != IDE_SUCCESS);
        sIsTxBegin = ID_FALSE;

        mMeta.mReplication.mXSN = aSN;
        setRestartSN(aSN);

        sStage = 0;
        IDE_TEST(sTrans.destroy( NULL ) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch(sStage)
    {
        case 2:
            IDE_ASSERT(sTrans.rollback() == IDE_SUCCESS);
            sIsTxBegin = ID_FALSE;
        case 1:
            if(sIsTxBegin == ID_TRUE)
            {
                IDE_ASSERT(sTrans.rollback() == IDE_SUCCESS);
                sIsTxBegin = ID_FALSE;
            }
            (void)sTrans.destroy( NULL );
        default:
            break;
    }

    IDE_ERRLOG(IDE_RP_0);
    IDE_SET(ideSetErrorCode(rpERR_ABORT_RP_SENDER_UPDATE_XSN));
    IDE_ERRLOG(IDE_RP_0);

    return IDE_FAILURE;
}

IDE_RC rpxSender::findAndUpdateInvalidMaxSN( smiStatement    * aSmiStmt,
                                             rpdReplSyncItem * aSyncList,
                                             rpdReplItems    * aReplItems,
                                             smSN              aSN)
{
    if ( aSyncList != NULL )
    {
        if ( isSyncItem( aSyncList,
                         aReplItems->mLocalUsername,
                         aReplItems->mLocalTablename,
                         aReplItems->mLocalPartname ) == ID_TRUE )
        {
            IDE_TEST( updateInvalidMaxSN( aSmiStmt,
                                          aReplItems,
                                          aSN )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ERRLOG(IDE_RP_0);

    return IDE_FAILURE;
}

IDE_RC rpxSender::updateInvalidMaxSN(smiStatement * aSmiStmt,
                                     rpdReplItems * aReplItems,
                                     smSN           aSN)
{
    // Do not need to update INVALID_MAX_SN again with the same value
    if((aReplItems->mInvalidMaxSN >= (ULong)aSN) ||
       (aSN == SM_SN_NULL) ||
       (isParallelChild() == ID_TRUE))
    {
        // Do nothing
    }
    else
    {
        /* PROJ-1915 off-line sender의 경우 Meta 갱신을 하지 않는다. */
        if(mCurrentType != RP_OFFLINE)
        {
            IDU_FIT_POINT( "rpxSender::updateInvalidMaxSN::Erratic::rpERR_ABORT_RP_SENDER_UPDATE_INVALID_MAX_SN" ); 
            IDE_TEST(rpcManager::updateInvalidMaxSN(aSmiStmt, aReplItems, aSN)
                     != IDE_SUCCESS);

            IDE_TEST( rpcManager::updateOldInvalidMaxSN( aSmiStmt,
                                                         aReplItems,
                                                         aSN )
                      != IDE_SUCCESS );
        }

        aReplItems->mInvalidMaxSN = (ULong)aSN;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ERRLOG(IDE_RP_0);
    IDE_SET(ideSetErrorCode(rpERR_ABORT_RP_SENDER_UPDATE_INVALID_MAX_SN));

    return IDE_FAILURE;
}

// To Fix PR-4099
IDE_RC rpxSender::initXSN( smSN aReceiverXSN )
{
    smSN          sCurrentSN;
    smiTrans      sTrans;
    SInt          sStage = 0;
    idBool        sIsTxBegin = ID_FALSE;
    smiStatement *spRootStmt;
    /* 반드시 Normal로 하여야 한다. 그렇게 하지 않으면
       Savepoint시 Log가 기록되지 않는다. */
    static UInt   sFlag = RPU_ISOLATION_LEVEL | SMI_TRANSACTION_NORMAL |
                          SMI_TRANSACTION_REPL_NONE | SMX_COMMIT_WRITE_NOWAIT;

    sCurrentSN = SM_SN_NULL;
    updateOldMaxXSN();
    
    switch ( mCurrentType )
    {
        case RP_NORMAL:
        case RP_PARALLEL:
        case RP_START_CONDITIONAL:
            if ( mMeta.mReplication.mXSN == SM_SN_NULL )
            {
                // if first start
                // parallel child는 최초에 시작할 수 없으므로,
                // 항상 mXSN이 NULL이 될 수 없다.
                IDE_DASSERT(isParallelChild() != ID_TRUE);

                IDE_TEST( sTrans.initialize() != IDE_SUCCESS );
                sStage = 1;

                IDE_TEST( sTrans.begin(&spRootStmt,
                                       NULL,
                                       sFlag,
                                       SMX_NOT_REPL_TX_ID )
                          != IDE_SUCCESS );
                sIsTxBegin = ID_TRUE;
                sStage = 2;

                /* BUG-14528 : Checkpoint Thread가 Replication이 읽을
                 * 파일을 지우지 못하도록 Savepoint로그 기록
                 *
                 * 4.3.7 까지는 checkpoint thread와 alter replication rep start를
                 * 수행하는 service thread, 그리고 sender thread 사이에
                 * 다음의 예와 같은 문제가 있었음
                 * 예>
                 * 1> service thread는 replication이 처음 시작할 때
                 *    smiGetLastValidGSN( &sCurrentSN )를 통해 로그파일 1번에
                 *   포함된 가장 최근의 SN을 얻어옴
                 * 2> bulk update가 발생하여 로그파일이 10번까지 생성됨
                 *  3> checkpoint thread가 replication에서 필요한 최소 파일을
                 *     메타 테이블(sys_replications_)에서 QP에 있는
                 *     getMinimumSN()함수를 통해 가져옴(-1: SM_SN_NULL)
                 *  4> checkpoint thread는 1번 로그파일 까지 지워도 된다고 판단하여
                 *     1번 로그파일을 지움
                 * 5> service thread가 다시 시작되어 sys_replications_에 XSN을
                 *    sCurrentSN로 갱신하고(updateXSN), sender thread는  sCurrentSN(mXSN)부터
                 *   로그를 읽으려고 하나 로그파일이 지워져서 비정상 종료하게 됨
                 *
                 * 4.3.9에서 getMinimumSN 함수가 RP쪽으로 옮겨오면서 getMinimumSN
                 * 함수 안에서 sendermutex를 잡게 되었다.
                 * checkpoint thread에서 getMinimumSN을 수행하는 것과 sender thread가
                 * initXSN() 함수를 호출하는 것이 (이 함수 호출전에 sendermutex를 잡음)
                 * 서로 동시에 수행될 수 없게 되었기 때문에,
                 * BUG-14528의 문제는 더 이상 발생하지 않는다.
                 * 그러나  savepoint로그를 찍는 것은 동작에 영향을 주지
                 * 않으므로 예방 차원에서 제거하지 않는다.
                 * 만약 BUG-14528과 같은 문제가 또 발생한다면 RP_START_QUICK
                 * RP_START_SYNC_ONLY/RP_START_SYNC도 고려해야 한다.
                 */

                IDE_TEST(sTrans.savepoint(RPX_SENDER_SVP_NAME,
                                          NULL)
                         != IDE_SUCCESS);

                /* For Parallel Logging: LSN을 SN으로 변경 PRJ-1464  */
                IDE_ASSERT(smiGetLastValidGSN(&sCurrentSN) == IDE_SUCCESS);
                mXSN = sCurrentSN;

                // update XLSN
                IDE_TEST(updateXSN(mXSN) != IDE_SUCCESS);

                sStage = 1;
                IDE_TEST( sTrans.commit() != IDE_SUCCESS );
                sIsTxBegin = ID_FALSE;

                sStage = 0;
                IDE_TEST( sTrans.destroy( NULL ) != IDE_SUCCESS );
            }
            else
            {
                mXSN = mMeta.mReplication.mXSN;
                mSkipXSN = aReceiverXSN;

                /* For Parallel Logging: LSN을 SN으로 변경 PRJ-1464  */
                IDE_ASSERT(smiGetLastUsedGSN(&sCurrentSN) == IDE_SUCCESS);
                
                IDU_FIT_POINT_RAISE( "rpxSender::initXSN::Erratic::rpERR_ABORT_INVALID_XSN",
                                     ERR_INVALID_XSN ); 
                IDE_TEST_RAISE( mXSN > sCurrentSN, ERR_INVALID_XSN);
            }
            mCommitXSN = mXSN;

            IDE_TEST_RAISE(mXSN == SM_SN_NULL, ERR_INVALID_XSN);
            break;

        case RP_QUICK:
            IDE_ASSERT(smiGetLastValidGSN(&sCurrentSN) == IDE_SUCCESS);
            mXSN = sCurrentSN;

            IDE_TEST(updateXSN(mXSN) != IDE_SUCCESS);
            mCommitXSN = mXSN;
            break;

        case RP_SYNC:
        case RP_SYNC_CONDITIONAL:
        case RP_SYNC_ONLY://fix BUG-9023
            if(mMeta.mReplication.mXSN == SM_SN_NULL)
            {
                sCurrentSN = smiGetValidMinSNOfAllActiveTrans();
                mXSN = sCurrentSN;

                IDE_TEST(updateXSN(mXSN) != IDE_SUCCESS);
            }
            else
            {
                mXSN = mMeta.mReplication.mXSN;
                mSkipXSN = aReceiverXSN;
            }

            mCommitXSN = mXSN;
            break;

        case RP_RECOVERY:
            IDE_DASSERT(mSNMapMgr != NULL);
            mXSN = mSNMapMgr->getMinReplicatedSN();
            mCommitXSN = mXSN;
            // 이전 receiver가 begin만 받고 종료된 경우 smMap의 최소 SN이 SM_SN_NULL이 될 수 있음
            // 이 경우에는 복구할 정보가 없으므로 종료해야함.
            if(mXSN == SM_SN_NULL)
            {
                mExitFlag = ID_TRUE;
            }
            break;

        /* PROJ-1915 */
        case RP_OFFLINE:
            mXSN = mMeta.mReplication.mXSN;
            mSkipXSN = aReceiverXSN;
            mCommitXSN = mXSN;
            break;

        case RP_XLOGFILE_FAILBACK_MASTER:
            /* 이 센더는 incremental sync용으로 임시로 사용하는 것으로 updateXSN을 하면 안된다 */
            mXSN = SM_SN_NULL;
            break;

        case RP_XLOGFILE_FAILBACK_SLAVE:
            /* 이중화를 최초 시작하는 경우 여기까지 올 수 없다. */
            IDE_DASSERT ( mMeta.mReplication.mXSN != SM_SN_NULL );

            mXSN = mMeta.mReplication.mXSN;
            mSkipXSN = aReceiverXSN;

            /* For Parallel Logging: LSN을 SN으로 변경 PRJ-1464  */
            IDE_ASSERT(smiGetLastUsedGSN(&sCurrentSN) == IDE_SUCCESS);

            IDU_FIT_POINT_RAISE( "rpxSender::initXSN::Erratic::rpERR_ABORT_INVALID_XSN",
                                 ERR_INVALID_XSN ); 
            IDE_TEST_RAISE( mXSN > sCurrentSN, ERR_INVALID_XSN);

            break;

        default:
            IDE_CALLBACK_FATAL("[Repl Sender] Invalid Option");
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_XSN);
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_INVALID_XSN,
                                mXSN,
                                sCurrentSN ));
    }
    IDE_EXCEPTION_END;
    IDE_ERRLOG(IDE_RP_0);
    IDE_SET(ideSetErrorCode(rpERR_ABORT_RP_SENDER_START));
    IDE_ERRLOG(IDE_RP_0);

    switch(sStage)
    {
        case 2:
            IDE_ASSERT(sTrans.rollback() == IDE_SUCCESS);
            sIsTxBegin = ID_FALSE;
        case 1:
            if(sIsTxBegin == ID_TRUE)
            {
                IDE_ASSERT(sTrans.rollback() == IDE_SUCCESS);
                sIsTxBegin = ID_FALSE;
            }
            (void)sTrans.destroy( NULL );
        default:
            break;
    }

    return IDE_FAILURE;
}
/*
return value: 
mSenderInfo가 초기화 되지 않은 경우에 SM_SN_NULL를 반환하며,
그렇지 않으면 mSenderInfo의 LastCommitSN을 반환
SM_SN_NULL를 반환하는 이유는 Sender가 정상적으로 초기화 되기 전에
FLUSH에 의해 이 함수가 호출되면 FLUSH는 대기하지 않고 지나갈 수
있도록 하기 위함임
*/
smSN
rpxSender::getRmtLastCommitSN()
{
    smSN sRmtLastCommitSN = SM_SN_NULL;
    if(mSenderInfo != NULL)
    {
        sRmtLastCommitSN = mSenderInfo->getRmtLastCommitSN();
    }
    return sRmtLastCommitSN;
}

smSN rpxSender::getLastProcessedSN()
{
    smSN sLastProcessedSN = SM_SN_NULL;
    if(mSenderInfo != NULL)
    {
        sLastProcessedSN = mSenderInfo->getLastProcessedSN();
    }
    return sLastProcessedSN;
}

smSN
rpxSender::getRestartSN()
{
    smSN sRestartSN = SM_SN_NULL;

    if(mSenderInfo != NULL)
    {
        sRestartSN = mSenderInfo->getRestartSN();
    }

    return sRestartSN;
}

void 
rpxSender::setRestartSN(smSN aSN)
{
    if(mSenderInfo != NULL)
    {
        mSenderInfo->setRestartSN(aSN);
    }
} 

void 
rpxSender::updateOldMaxXSN()
{
    if ( ( mXSN > mOldMaxXSN ) && ( mXSN != SM_SN_NULL ) ) 
    {
        mOldMaxXSN = mXSN;
    }
    else
    {
        /* Nothing to do */
    }
}

smSN rpxSender::getLastArrivedSN()
{
    smSN sLastArrivedSN = SM_SN_NULL;
    if(mSenderInfo != NULL)
    {
        sLastArrivedSN = mSenderInfo->getLastArrivedSN();
    }
    return sLastArrivedSN;
}
