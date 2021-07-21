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

#include <mmtSessionManager.h>
#include <mmdManager.h>
#include <mmdXa.h>
#include <mmdXid.h>
#include <mmdXidManager.h>
#include <mmuProperty.h>
#include <mmdDef.h>

idBool mmdManager::mInitFlag = ID_FALSE;

IDE_RC mmdManager::initialize()
{
    mmdXid         *sXidObj = NULL;
    /* BUG-18981 */
    ID_XID          sXid;
    timeval         sTime;
    smiCommitState  sTxState;
    SInt            sSlotID = -1;

    IDE_TEST(mmdXidManager::initialize() != IDE_SUCCESS);

    while (1)
    {
        IDE_TEST(smiXaRecover(&sSlotID, &sXid, &sTime, &sTxState) != IDE_SUCCESS);

        if (sSlotID < 0)
        {
            break;
        }

        if (sTxState == SMX_XA_PREPARED)
        {
            IDE_TEST(mmdXidManager::alloc(&sXidObj, &sXid, NULL) != IDE_SUCCESS);

            sXidObj->lock();

            IDE_TEST(sXidObj->attachTrans(sSlotID) != IDE_SUCCESS);
            /* fix BUG-35374 To improve scalability about XA, latch granularity of XID hash should be more better than now
               that is to say , chanage the granularity from global to bucket level. */
            IDE_TEST(mmdXidManager::add(sXidObj,mmdXidManager::getBucketPos(sXidObj->getXid())) != IDE_SUCCESS);

            sXidObj->unlock();
            sXidObj = NULL;
        }
    }

    //fix BUG-27218 XA Load Heurisitc Transaction함수 내용을 명확히 해야 한다.
    IDE_TEST(loadHeuristicTrans(NULL,   /* PROJ-2446 */
                                MMD_LOAD_HEURISTIC_XIDS_AT_STARTUP, 
                                NULL, 
                                NULL) 
             != IDE_SUCCESS);

    mInitFlag = ID_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        if (sXidObj != NULL)
        {
            sXidObj->unlock();
        }
    }

    return IDE_FAILURE;
}

IDE_RC mmdManager::finalize()
{
    IDE_TEST(mmdXidManager::finalize() != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

void mmdManager::checkXaTimeout()
{
    mmdXid         *sXidObj;
    /* BUG-18981 */
    ID_XID          sXid;
    timeval         sTime;
    smiCommitState  sTxState;
    //smSCN           sDummySCN;
    SInt            sSlotID = -1;
    //fix BUG-26844 mmdXa::rollback이 장시간 진행될때 어떠한 XA call도 진행할수 없습니다
    //Bug Fix를     mmdManager::checkXATimeOut에도 적용해야 합니다.
    UInt            sState = 0;

    //BUG-26163
    PDL_Time_Value sNow;
    //fix BUG-30343 Committing a xid can be failed in replication environment.
    SChar         *sErrorMsg;
    UInt           sTvSec;    /* seconds since Jan. 1, 1970 */
    UChar          sXidString[XID_DATA_MAX_LEN];

    if (mInitFlag != ID_TRUE)
    {
        return;
    }
    while (1)
    {
        IDE_ASSERT(smiXaRecover(&sSlotID, &sXid, &sTime, &sTxState) == IDE_SUCCESS);

        // bug-27571: klocwork warnings
        // 밑의 sTvSec을 구하기 전에 sSlotID를 먼저 검사하도록 순서 변경.
        if (sSlotID < 0)
        {
            break;
        }

        //BUG-26163  XA_INDOUBT_TX_TIMEOUT properties 비정상 동작
        sNow = idlOS::gettimeofday();
        sTvSec = (UInt)sNow.sec();

        IDU_FIT_POINT( "mmdManager::checkXaTimeout::lock::XA_TIMEOUT" );
        
        //BUG-26163 현재 시각이 Prepared로 세팅된 시각보다 클 경우에 비교를 해야한다.
        if (  ( sTxState == SMX_XA_PREPARED ) 
           && ( sTvSec > (UInt)sTime.tv_sec)
           && ( (sTvSec - (UInt)sTime.tv_sec) >= mmuProperty::getXaTimeout() ) )
        {
            //fix BUG-26844 mmdXa::rollback이 장시간 진행될때 어떠한 XA call도 진행할수 없습니다
            //Bug Fix를     mmdManager::checkXATimeOut에도 적용해야 합니다.
            IDE_TEST(mmdXa::fix(&sXidObj,&sXid, MMD_XA_DO_LOG) != IDE_SUCCESS);
            sState = 1;

            if (sXidObj != NULL)
            {
                (void)idaXaConvertXIDToString(NULL, &sXid, sXidString, XID_DATA_MAX_LEN);
                sXidObj->lock();
                //fix BUG-26844 mmdXa::rollback이 장시간 진행될때 어떠한 XA call도 진행할수 없습니다
                //Bug Fix를     mmdManager::checkXATimeOut에도 적용해야 합니다.
                sState = 2;
                if (sXidObj->getState() == MMD_XA_STATE_PREPARED)
                {
                    switch (mmuProperty::getXaComplete())
                    {
                        case 0:
                            break;

                        case 1:
                            //fix BUG-22651 smrLogFileGroup::updateTransLSNInfo, server FATAL
                            //fix BUG-30343 Committing a xid can be failed in replication environment.
                            if(sXidObj->commitTrans(NULL) == IDE_SUCCESS)
                            {    
                                insertHeuristicTrans( NULL, /* PROJ-2446 */ 
                                                      &sXid, 
                                                      QCM_XA_COMMITTED );
                                sXidObj->setState(MMD_XA_STATE_HEURISTICALLY_COMMITTED);
                            }
                            else
                            {
                                //fix BUG-30343 Committing a xid can be failed in replication environment.
                                sErrorMsg =  ideGetErrorMsg(ideGetErrorCode());
                                ideLog::logLine(IDE_XA_0, "Heuristic Commit Error [%s], reason [%s]", sXidString,sErrorMsg);
                            }
                            break;
                        case 2:
                            ideLog::logLine(IDE_XA_0, "Heuristic Rollback Timeout [%s]", sXidString);
         
                            //fix BUG-22651 smrLogFileGroup::updateTransLSNInfo, server FATAL
                            sXidObj->rollbackTrans(NULL);

                            /* bug-36037: invalid xid
                               invalid xid의 경우 insertHeuri 실패를 허용하므로
                               수행결과 체크 안함 */
                            insertHeuristicTrans( NULL, /* PROJ-2446 */
                                                  &sXid, 
                                                  QCM_XA_ROLLBACKED );
                            sXidObj->setState(MMD_XA_STATE_HEURISTICALLY_ROLLBACKED);
                            break;

                        default:
                            IDE_CALLBACK_FATAL("invalid value of property XA_HEURISTIC_COMPLETE");
                            break;
                    }
                }
                //fix BUG-26844 mmdXa::rollback이 장시간 진행될때 어떠한 XA call도 진행할수 없습니다
                //Bug Fix를     mmdManager::checkXATimeOut에도 적용해야 합니다.
                sState = 1;
                sXidObj->unlock();
            }//if sXidObj != NULL
            //fix BUG-26844 mmdXa::rollback이 장시간 진행될때 어떠한 XA call도 진행할수 없습니다
            //Bug Fix를     mmdManager::checkXATimeOut에도 적용해야 합니다.
            sState = 0;
            IDE_ASSERT(mmdXa::unFix(sXidObj,&sXid , MMD_XA_NONE) == IDE_SUCCESS);
        }//if SMX_XA_PREPARED
    }//while
    
    return;
    
    IDE_EXCEPTION_END;
    {
        //fix BUG-26844 mmdXa::rollback이 장시간 진행될때 어떠한 XA call도 진행할수 없습니다
        //Bug Fix를     mmdManager::checkXATimeOut에도 적용해야 합니다.
        switch (sState)
        {
            case 2:
                sXidObj->unlock();
            case 1:
            IDE_ASSERT(mmdXa::unFix(sXidObj,&sXid,MMD_XA_NONE) == IDE_SUCCESS);
            default:
                break;
        }
    }
    
}

 //fix BUG-27218 XA Load Heurisitc Transaction함수 내용을 명확히 해야 한다.
IDE_RC mmdManager::loadHeuristicTrans( idvSQL                     *aStatistics, 
                                       mmdXaLoadHeuristicXidFlag  aLoadHeuristicXidFlag, 
                                       ID_XID                     **aHeuristicXids, 
                                       SInt                       *aHeuristicXidsCnt ) 
{
    smiTrans             sTrans;
    smiStatement         sStmt;
    smiStatement        *sRootStmt;
    qcmXaHeuristicTrans *sHeuristicTrans = NULL;
    SInt                 sMaxHeuristicTrans;
    SInt                 sNumHeuristicTrans;
    ID_XID               sXid;
    mmdXid              *sXidObj         = NULL;
    mmdXaState           sXidState;
    UInt                 sStage          = 0;
    ID_XID              *sHeuristicXids;

    IDE_TEST( sTrans.initialize() != IDE_SUCCESS ); /* PROJ-2446 */
    sStage++;

    IDE_TEST( sTrans.begin( &sRootStmt,
                            aStatistics, /* PROJ-2446 */
                            ( SMI_ISOLATION_NO_PHANTOM     | 
                              SMI_TRANSACTION_NORMAL       | 
                              SMI_TRANSACTION_REPL_DEFAULT | 
                              SMI_COMMIT_WRITE_NOWAIT ) ) 
              != IDE_SUCCESS );
    sStage++;

    IDE_TEST( sStmt.begin( sTrans.getStatistics(), /* PROJ-2446 */
                           sRootStmt, 
                           SMI_STATEMENT_NORMAL | 
                           SMI_STATEMENT_ALL_CURSOR ) 
              != IDE_SUCCESS ); 
    sStage++;

    IDE_TEST(qcmXA::selectAll(&sStmt, &sMaxHeuristicTrans, NULL, 0) != IDE_SUCCESS);

    IDE_TEST_CONT(sMaxHeuristicTrans <= 0,skip_load_heuristic_xids);
    
    IDU_FIT_POINT_RAISE( "mmdManager::loadHeuristicTrans::malloc::HeuristicTrans",
                          InsufficientMemory );
    
    IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_MMD,
                                     ID_SIZEOF(qcmXaHeuristicTrans) * sMaxHeuristicTrans,
                                     (void **)&sHeuristicTrans) != IDE_SUCCESS, InsufficientMemory);

    IDE_TEST(qcmXA::selectAll(&sStmt,
                              &sNumHeuristicTrans,
                              sHeuristicTrans,
                              sMaxHeuristicTrans) != IDE_SUCCESS);

    IDE_ASSERT(sNumHeuristicTrans == sMaxHeuristicTrans);


    //fix BUG-27218 XA Load Heurisitc Transaction함수 내용을 명확히 해야 한다.
    if ( aLoadHeuristicXidFlag == MMD_LOAD_HEURISTIC_XIDS_AT_XA_RECOVER )
    {
        IDU_FIT_POINT_RAISE( "mmdManager::loadHeuristicTrans::malloc::HeuristicXids",
                              InsufficientMemory );

        IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_MMT,
                                         ID_SIZEOF(ID_XID) * sMaxHeuristicTrans,
                                         (void **)&sHeuristicXids) != IDE_SUCCESS, InsufficientMemory);
        *aHeuristicXids = sHeuristicXids;
        for (sNumHeuristicTrans = 0;
             sNumHeuristicTrans < sMaxHeuristicTrans;
             sNumHeuristicTrans++)
        {
            idaXaStringToXid(sHeuristicTrans[sNumHeuristicTrans].formatId,
                             sHeuristicTrans[sNumHeuristicTrans].globalTxId,
                             sHeuristicTrans[sNumHeuristicTrans].branchQualifier,
                             &sXid);
            
            idlOS::memcpy(&sHeuristicXids[sNumHeuristicTrans], &sXid, ID_SIZEOF(ID_XID));
            *aHeuristicXidsCnt = sNumHeuristicTrans+1;
        }//for
    }
    else
    {
        IDE_ASSERT(aLoadHeuristicXidFlag  == MMD_LOAD_HEURISTIC_XIDS_AT_STARTUP);
        for (sNumHeuristicTrans = 0;
             sNumHeuristicTrans < sMaxHeuristicTrans;
             sNumHeuristicTrans++)
        {
            idaXaStringToXid(sHeuristicTrans[sNumHeuristicTrans].formatId,
                             sHeuristicTrans[sNumHeuristicTrans].globalTxId,
                             sHeuristicTrans[sNumHeuristicTrans].branchQualifier,
                             &sXid);

            switch ((SInt)sHeuristicTrans[sNumHeuristicTrans].status)
            {
                case QCM_XA_ROLLBACKED:
                    sXidState = MMD_XA_STATE_HEURISTICALLY_ROLLBACKED;
                    break;
                case QCM_XA_COMMITTED:
                    sXidState = MMD_XA_STATE_HEURISTICALLY_COMMITTED;
                    break;
                default:
                    IDE_ASSERT(0);
                    break;
            }

            //fix BUG-27218 XA Load Heurisitc Transaction함수 내용을 명확히 해야 한다.
            IDE_TEST(mmdXidManager::alloc(&sXidObj, &sXid, NULL)
                    != IDE_SUCCESS);
            // xid list에 달리전이기때문에 Xid Object에 lock이 필요 없다.
            sXidObj->setState(sXidState);
            IDE_TEST(mmdXidManager::add(sXidObj,mmdXidManager::getBucketPos(sXidObj->getXid())) != IDE_SUCCESS);
        }//for
    }

    IDE_TEST(iduMemMgr::free(sHeuristicTrans) != IDE_SUCCESS);

    sHeuristicTrans = NULL;

    IDE_EXCEPTION_CONT(skip_load_heuristic_xids);
    

    IDE_TEST(sStmt.end(SMI_STATEMENT_RESULT_SUCCESS) != IDE_SUCCESS);
    sStage--;

    IDE_TEST(sTrans.commit() != IDE_SUCCESS);
    sStage--;

    IDE_TEST(sTrans.destroy(NULL) != IDE_SUCCESS);
    sStage--;

    return IDE_SUCCESS;
    
    IDE_EXCEPTION(InsufficientMemory);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;
    {
        if (sHeuristicTrans != NULL)
        {
            // fix BUG-28267 [codesonar] Ignored Return Value
            IDE_ASSERT(iduMemMgr::free(sHeuristicTrans) == IDE_SUCCESS);
        }

        switch (sStage)
        {
            case 3:
                sStmt.end(SMI_STATEMENT_RESULT_FAILURE);
            case 2:
                sTrans.rollback();
            case 1:
                sTrans.destroy(NULL);
                break;
        }
    }

    return IDE_FAILURE;
}

IDE_RC mmdManager::insertHeuristicTrans( idvSQL             *aStatistics,
                                         ID_XID             *aXid, 
                                         qcmXaStatusType     aStatus )
{
    smiTrans      sTrans;
    smiStatement  sStmt;
    smiStatement *sRootStmt;
    UInt          sStage = 0;

    IDE_TEST( sTrans.initialize() != IDE_SUCCESS ); /* PROJ-2446 */
    sStage++;

    IDE_TEST( sTrans.begin( &sRootStmt,
                            aStatistics,   /* PROJ-2446 */
                            ( SMI_ISOLATION_NO_PHANTOM     | 
                              SMI_TRANSACTION_NORMAL       | 
                              SMI_TRANSACTION_REPL_DEFAULT | 
                              SMI_COMMIT_WRITE_NOWAIT ) ) 
              != IDE_SUCCESS );
    sStage++;

    IDE_TEST( sStmt.begin( sTrans.getStatistics(),    /* PROJ-2446 */
                           sRootStmt, 
                           SMI_STATEMENT_NORMAL | SMI_STATEMENT_ALL_CURSOR ) 
              != IDE_SUCCESS );
    sStage++;

    IDE_TEST(qcmXA::insert(&sStmt, aXid, (SInt)aStatus) != IDE_SUCCESS);

    IDE_TEST(sStmt.end(SMI_STATEMENT_RESULT_SUCCESS) != IDE_SUCCESS);
    sStage--;

    IDE_TEST(sTrans.commit() != IDE_SUCCESS);
    sStage--;

    IDE_TEST(sTrans.destroy(NULL) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        switch (sStage)
        {
            case 3:
                sStmt.end(SMI_STATEMENT_RESULT_FAILURE);
            case 2:
                sTrans.rollback();
            case 1:
                sTrans.destroy(NULL);
                break;
        }
    }

    return IDE_FAILURE;
}

IDE_RC mmdManager::removeHeuristicTrans( idvSQL     *aStatistics,
                                         ID_XID     *aXid )
{
    smiTrans      sTrans;
    smiStatement  sStmt;
    smiStatement *sRootStmt;
    UInt          sStage     = 0;
    idBool        sIsRemoved = ID_FALSE;

    IDE_TEST( sTrans.initialize() != IDE_SUCCESS ); /* PROJ-2446 */
    sStage++;

    IDE_TEST( sTrans.begin( &sRootStmt,
                            aStatistics,   /* PROJ-2446 */
                            ( SMI_ISOLATION_NO_PHANTOM      | 
                              SMI_TRANSACTION_NORMAL        | 
                              SMI_TRANSACTION_REPL_DEFAULT  | 
                              SMI_COMMIT_WRITE_NOWAIT ) ) 
              != IDE_SUCCESS );
    sStage++;

    IDE_TEST( sStmt.begin( sTrans.getStatistics(),    /* PROJ-2446 */
                           sRootStmt, 
                           SMI_STATEMENT_NORMAL | 
                           SMI_STATEMENT_ALL_CURSOR ) 
              != IDE_SUCCESS ); 
    sStage++;

    IDE_TEST(qcmXA::remove(&sStmt, aXid, &sIsRemoved) != IDE_SUCCESS);
    IDE_TEST(sIsRemoved == ID_FALSE);

    IDE_TEST(sStmt.end(SMI_STATEMENT_RESULT_SUCCESS) != IDE_SUCCESS);
    sStage--;

    IDE_TEST(sTrans.commit() != IDE_SUCCESS);
    sStage--;

    IDE_TEST(sTrans.destroy(NULL) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        switch (sStage)
        {
            case 3:
                sStmt.end(SMI_STATEMENT_RESULT_FAILURE);
            case 2:
                sTrans.rollback();
            case 1:
                sTrans.destroy(NULL);
                break;
        }
    }

    return IDE_FAILURE;
}

IDE_RC mmdManager::showTransactions()
{
    /*
     * BUGBUG
     */

    return IDE_SUCCESS;
}

IDE_RC mmdManager::rollbackTransaction(smTID /*aTid*/)
{
    /*
     * BUGBUG
     */

    return IDE_SUCCESS;
}

IDE_RC mmdManager::commitTransaction(smTID /*aTid*/)
{
    /*
     * BUGBUG
     */

    return IDE_SUCCESS;
}
