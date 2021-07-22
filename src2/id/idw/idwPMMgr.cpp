/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
/*******************************************************************************
 * $Id$
 ******************************************************************************/
#include <idl.h>
#include <idErrorCode.h>
#include <idwDef.h>
#include <idu.h>
#include <idrLogMgr.h>
#include <idwVLogPMMgr.h>
#include <iduShmList.h>
#include <idwPMMgr.h>
#include <idwWatchDog.h>

acp_sem_t             idwPMMgr::mSemHandle;
iduShmProcMgrInfo   * idwPMMgr::mProcMgrInfo = NULL;
iduShmProcInfo      * idwPMMgr::mProcInfo4MainProcess = NULL;
iduShmProcType        idwPMMgr::mProcType;

static SChar gProcString[][16] = { "DAEMON", "WSERVER" };

/*********************************************************************
 *
 * iduShmProcMgrInfo
 * |-mCurProcCnt
 * |-mPRTable-->[iduShmProcInfo #1][iduShmProcInfo #2] ... [iduShmProcInfo #n]
 * ******************************************************************/
IDE_RC idwPMMgr::initialize( iduShmProcType aProcType )
{
    UInt                i;
    iduShmProcInfo    * sPRTable;
    iduShmProcInfo    * sCurProcInfo;
    iduShmProcMgrInfo * sProcMgrInfo;
    key_t               sSemKey;
    SInt                sNewSemID;
    union semun         sArg;
    idShmAddr           sPRTableAddr;
    idShmAddr           sLatchAddr;
    UInt                sState = 0;
    iduShmProcInfo    * sProcInfo;

    sArg.val = 1;

    sProcMgrInfo = iduShmMgr::getProcMgrInfo();

    sSemKey = iduProperty::getShmDBKey();

    sProcMgrInfo->mCurProcCnt = 0;

    mProcType = IDU_SHM_PROC_TYPE_NULL;

    sPRTable  = sProcMgrInfo->mPRTable;

    sNewSemID = idlOS::semget( sSemKey,
                               IDU_MAX_PROCESS_COUNT/* Sema Value */,
                               0666 | IPC_CREAT/* | IPC_EXCL*/ );

    IDE_TEST_RAISE( sNewSemID < 0, err_sema_create );

    /* Created Semaphore */
    sState = 1;

    sPRTableAddr = IDU_SHM_GET_ADDR_SUBMEMBER( sProcMgrInfo->mAddrSelf,
                                               iduShmProcMgrInfo,
                                               mPRTable );

    for( i = 0; i < IDU_MAX_PROCESS_COUNT; i++ )
    {
        sCurProcInfo = sPRTable + i;

        sCurProcInfo->mSelfAddr  = sPRTableAddr + i * ID_SIZEOF(iduShmProcInfo);
        sCurProcInfo->mState     = IDU_SHM_PROC_STATE_NULL;
        sCurProcInfo->mType      = IDU_SHM_PROC_TYPE_NULL;
        sCurProcInfo->mPID       = 0;
        sCurProcInfo->mLPID      = i;
        sCurProcInfo->mThreadCnt = 0;

        sCurProcInfo->mRestartProcFunc = NULL;

        initThrInfo( sCurProcInfo,
                     IDU_SHM_GET_ADDR_SUBMEMBER( sCurProcInfo->mSelfAddr,
                                                 iduShmProcInfo,
                                                 mMainTxInfo ),
                     IDU_SHM_THR_DEFAULT_ATTR,
                     ID_TRUE, /* Head Node */
                     &sCurProcInfo->mMainTxInfo );

        IDU_SHM_VALIDATE_ADDR_PTR( sCurProcInfo->mMainTxInfo.mSelfAddr,
                                   &sCurProcInfo->mMainTxInfo );

        sCurProcInfo->mSemKey = sSemKey;
        sCurProcInfo->mSemID  = sNewSemID;

        sLatchAddr = IDU_SHM_GET_ADDR_SUBMEMBER( sCurProcInfo->mSelfAddr,
                                                 iduShmProcInfo,
                                                 mLatch );

        iduShmLatchInit( sLatchAddr,
                         IDU_SHM_LATCH_SPIN_COUNT_DEFAULT,
                         &sCurProcInfo->mLatch );

        /* 각 Logical PID에 해당하는 Semaphore를 1로 초기화 한다 */
        IDE_TEST_RAISE( idlOS::semctl( sNewSemID, i, SETVAL, sArg ) != 0,
                        err_sema_set_value );
    }

    mProcMgrInfo = sProcMgrInfo;

    enableResisterNewProc();

    IDE_TEST( idwPMMgr::registerProc( NULL, /* aStatistics */
                                      idlOS::getpid(),
                                      aProcType,
                                      &sProcInfo )
              != IDE_SUCCESS );

    IDE_TEST( idwWatchDog::initialize( IDU_MAX_PROCESS_COUNT )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_sema_create );
    {
        IDE_SET( ideSetErrorCode( idERR_FATAL_SEM_GET ) );
    }
    IDE_EXCEPTION( err_sema_set_value );
    {
        IDE_SET( ideSetErrorCode( idERR_FATAL_idc_SEM_OP ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    switch( sState )
    {
    case 1:
        (void)idlOS::semctl( sNewSemID, 0 /*semnum */, IPC_RMID, sArg );
    default:
        break;
    }
    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC idwPMMgr::destroy()
{
    UInt                i;
    iduShmProcInfo    * sPRTable;
    iduShmProcInfo    * sCurProcInfo;
    iduShmProcMgrInfo * sProcMgrInfo;
    union semun         sArg;

    IDE_TEST( idwWatchDog::destroy() != IDE_SUCCESS );

    IDE_TEST( idwPMMgr::unRegisterProc( NULL/*aStatistics */,
                                        mProcInfo4MainProcess )

              != IDE_SUCCESS );

    sProcMgrInfo = iduShmMgr::getProcMgrInfo();

    IDE_ASSERT( sProcMgrInfo->mCurProcCnt == 0 );

    sPRTable = sProcMgrInfo->mPRTable;
    sArg.val = 1;

    sCurProcInfo = sPRTable;

    for( i = 0; i < IDU_MAX_PROCESS_COUNT; i++ )
    {
        IDE_ASSERT( sCurProcInfo->mState == IDU_SHM_PROC_STATE_NULL );
        IDE_ASSERT( sCurProcInfo->mPID   == 0 );
        IDE_ASSERT( sCurProcInfo->mLPID  == i );

        iduShmLatchDest( &sCurProcInfo->mLatch );

        sCurProcInfo++;
    }

    IDE_TEST_RAISE( idlOS::semctl( sPRTable[0].mSemID,
                                   0 /*semnum*/,
                                   IPC_RMID,
                                   sArg ) != 0,
                    err_sem_destroy );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_sem_destroy );
    {
        IDE_SET( ideSetErrorCode( idERR_FATAL_SEM_DELETE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC idwPMMgr::attach( iduShmProcType aProcType )
{
    iduShmProcInfo * sProcInfo;

    mProcMgrInfo  = iduShmMgr::getProcMgrInfo();

    IDE_TEST( idwPMMgr::registerProc( NULL, /* aStatistics */
                                      idlOS::getpid(),
                                      aProcType,
                                      &sProcInfo )
              != IDE_SUCCESS );

    idwWatchDog::attach();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC idwPMMgr::detach()
{
    IDE_ASSERT( mProcMgrInfo != NULL );

    idwWatchDog::detach();

    IDE_TEST( idwPMMgr::unRegisterProc( NULL/*aStatistics */,
                                        mProcInfo4MainProcess )

              != IDE_SUCCESS );

    mProcMgrInfo = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC idwPMMgr::registerProc( idvSQL            * aStatistics,
                               UInt                aProcessID,
                               iduShmProcType      aProcType,
                               iduShmProcInfo   ** aProcInfo )
{
    ULong             sPID = aProcessID;
    ULong             sOriginValue;
    iduShmProcInfo  * sPRTable;
    iduShmProcInfo  * sCurProcInfo = NULL;
    iduShmProcInfo  * sProcInfoFence = NULL;
    iduShmProcInfo  * sAllocProcInfo;
    UInt              sLoopCnt = 0;
    UInt              sProcCnt = 0;      
    idCoreAcpPset     sPset;

    IDE_ASSERT( aProcInfo   != NULL );
    IDE_ASSERT( mProcInfo4MainProcess == NULL );

    IDE_TEST_RAISE(
        ( mProcMgrInfo->mFlag & IDU_SHM_PROC_REGISTER_MASK ) !=
        IDU_SHM_PROC_REGISTER_ON,
        err_register_proc_disabled );

    ACP_PSET_ZERO( &sPset );

    sPRTable = mProcMgrInfo->mPRTable;

    if( aProcType == IDU_PROC_TYPE_USER )
    {
        sProcInfoFence = sPRTable + IDU_MAX_PROCESS_COUNT;

        while(1)
        {
            sCurProcInfo = sPRTable + IDU_PROC_TYPE_USER;
            sProcCnt     = 0;

            for( ; sCurProcInfo != sProcInfoFence; sCurProcInfo++ )
            {
                if( sCurProcInfo->mPID == 0 )
                {
                    sOriginValue = idCore::acpAtomicCas64( &sCurProcInfo->mPID,
                                                           sPID,
                                                           0 );

                    if( sOriginValue == 0 )
                    {

                        IDE_ASSERT( sCurProcInfo->mState == IDU_SHM_PROC_STATE_NULL );
                        break;
                    }
                }
                sProcCnt++;            
            }

            if( sCurProcInfo != sProcInfoFence )
            {
                sAllocProcInfo = sCurProcInfo;
                break;
            }
            else
            {
                // Available Slot이 없으면 Logging Session을 끊으면
                // 빠져나와야 한다.
                IDE_TEST( iduCheckSessionEvent( aStatistics )
                          != IDE_SUCCESS );

                idlOS::sleep(1);

                sLoopCnt++;

                if( sLoopCnt % 60 == 0 )
                {
                    sLoopCnt = 0;
                    ideLog::log( IDE_SERVER_0,
                                 "No More available process slot in the process table\n" );
                }
            }
        }
    }
    else
    {
        IDE_TEST_RAISE( sPRTable[aProcType].mPID != 0,
                        err_sysproc_already_start );

        sAllocProcInfo = &sPRTable[aProcType];
        sAllocProcInfo->mPID = sPID;
    }

    IDE_TEST( acquireSem4Proc( sAllocProcInfo ) != IDE_SUCCESS );


    if( aStatistics != NULL )
    {
        IDV_WP_SET_PROC_INFO( aStatistics, sAllocProcInfo );
    }

    sAllocProcInfo->mType  = aProcType;
    sAllocProcInfo->mState = IDU_SHM_PROC_STATE_RUN;

    IDE_ASSERT( mProcInfo4MainProcess == NULL );

    mProcInfo4MainProcess = sAllocProcInfo;

    iduShmMgr::setShmCleanModeAtShutdown( mProcInfo4MainProcess );

    iduShmList::initBase( &mProcInfo4MainProcess->mStStatementList,
                          IDU_SHM_GET_ADDR_SUBMEMBER( mProcInfo4MainProcess->mSelfAddr,
                                                      iduShmProcInfo,
                                                      mStStatementList ),
                          IDU_SHM_NULL_ADDR );

    mProcType  = aProcType;

    if( aProcInfo != NULL )
    {
        *aProcInfo = sAllocProcInfo;
    }

    if( iduProperty::getUserProcessCpuAffinity() == ID_TRUE )
    {
        sProcCnt = sProcCnt % idlVA::getProcessorCount();

        ACP_PSET_SET( &sPset, sProcCnt );

        if( idCore::acpPsetBindProcess( &sPset ) != 0 )
        {
            // The failure of binding process is ignorable.
            ideLog::log( IDE_SERVER_0, 
                         "fail to bind process: [PID:%"ID_UINT32_FMT"]\n", 
                         aProcessID );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_register_proc_disabled );
    {
        IDE_SET(ideSetErrorCode(idERR_FATAL_REGISTER_NEW_PROCESS_DISABLED,
                                gProcString[aProcType]));
    }
    IDE_EXCEPTION( err_sysproc_already_start );
    {
        IDE_SET(ideSetErrorCode(idERR_FATAL_SYSTEM_PROCESS_ALREADY_STARTED,
                                gProcString[aProcType]));
    }
    IDE_EXCEPTION_END;

    *aProcInfo = NULL;

    return IDE_FAILURE;
}

IDE_RC idwPMMgr::unRegisterProc( idvSQL         * aStatistics,
                                 iduShmProcInfo * aProcInfo )
{
    aProcInfo->mType  = IDU_SHM_PROC_TYPE_NULL;
    aProcInfo->mState = IDU_SHM_PROC_STATE_NULL;

    IDL_MEM_BARRIER;

    IDE_TEST( releaseSem4Proc( aProcInfo ) != IDE_SUCCESS );


    idCore::acpAtomicSet64( &aProcInfo->mPID, 0 );

    if( aStatistics != NULL )
    {
        IDV_WP_SET_PROC_INFO( aStatistics, NULL );
    }

    mProcInfo4MainProcess = NULL;
    
    if( iduProperty::getUserProcessCpuAffinity() == ID_TRUE )
    {
        (void)idCore::acpPsetUnbindProcess();
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC idwPMMgr::unRegisterProcByWatchDog( idvSQL         * aStatistics,
                                           iduShmProcInfo * aProcInfo )
{
    aProcInfo->mType  = IDU_SHM_PROC_TYPE_NULL;
    aProcInfo->mState = IDU_SHM_PROC_STATE_NULL;

    IDL_MEM_BARRIER;

    idCore::acpAtomicSet64( &aProcInfo->mPID, 0 );

    if( aStatistics != NULL )
    {
        IDV_WP_SET_PROC_INFO( aStatistics, NULL );
    }

    return IDE_SUCCESS;
}

IDE_RC idwPMMgr::allocThrInfo( idvSQL          * aStatistics,
                               UInt              aFlag,
                               iduShmTxInfo   ** aThrInfo )
{
    iduShmProcInfo  * sProcInfo;
    iduShmTxInfo    * sTxInfo  = NULL;
    idrSVP            sVSavepoint;
    UInt              sState = 0;

    IDE_ASSERT( aStatistics != NULL );

    sProcInfo = (iduShmProcInfo*)IDV_WP_GET_PROC_INFO( aStatistics );

    idrLogMgr::setSavepoint( &sProcInfo->mMainTxInfo, &sVSavepoint );

    IDE_TEST( lockProcess( aStatistics, sProcInfo ) != IDE_SUCCESS );
    sState = 1;

    // BUG-36090로 인해 변경
    IDE_TEST( iduShmMgr::allocMemWithoutUndo( aStatistics,
                                              &sProcInfo->mMainTxInfo,
                                              IDU_MEM_ID_THREAD_INFO,
                                              ID_SIZEOF(iduShmTxInfo),
                                              (void**)&sTxInfo )
              != IDE_SUCCESS );

    initThrInfo( sProcInfo,
                 iduShmMgr::getAddr( sTxInfo ),
                 aFlag,
                 ID_FALSE, /* Head Node */
                 sTxInfo );

    iduShmList::addLast( &sProcInfo->mMainTxInfo,
                         &sProcInfo->mMainTxInfo.mNode,
                         &sTxInfo->mNode );


    IDE_TEST( idwVLogPMMgr::writeThreadCount4Proc( &sProcInfo->mMainTxInfo,
                                                   sProcInfo->mSelfAddr,
                                                   sProcInfo->mThreadCnt )
              != IDE_SUCCESS );

    sProcInfo->mThreadCnt++;


    /* Thread Info는 Process 비정상 종료시 WatchDog에 의해서 Free가 된다. 때문에
     * Proces Info의 ThrInfo List에 대한 연결이 완료되면 Undo Log는 불필요하다. */
    IDE_TEST( idrLogMgr::commit( aStatistics, &sProcInfo->mMainTxInfo ) != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( unlockProcess( aStatistics, sProcInfo ) != IDE_SUCCESS );

    *aThrInfo = sTxInfo;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ASSERT( idrLogMgr::abort2Svp( aStatistics,
                                      &sProcInfo->mMainTxInfo,
                                      &sVSavepoint )
                == IDE_SUCCESS );

    if( sState != 0 )
    {
        IDE_ASSERT( unlockProcess( aStatistics, sProcInfo )
                    == IDE_SUCCESS );
    }

    /* Shared Memory 영역에 대한 Recovery이기 때문에 Undo는 In-Memory Log
     * 를 이용한다. */

    return IDE_FAILURE;
}

IDE_RC idwPMMgr::freeThrInfo( idvSQL         * aStatistics,
                              iduShmTxInfo   * aThrInfo )
{
    idrSVP            sSavepoint;
    iduShmProcInfo  * sProcInfo;

    IDE_ASSERT( idrLogMgr::commit( aStatistics, aThrInfo )
                == IDE_SUCCESS );

    /* Process의 Thread들이 공유하는 공간이다. 때문에 MainThread의 LogBuffer를
     * 이용하도록 한다. */
    sProcInfo = idwPMMgr::getProcInfo( aThrInfo->mLPID );

    IDE_ASSERT( lockProcess( aStatistics, sProcInfo ) == IDE_SUCCESS );

    idrLogMgr::setSavepoint( &sProcInfo->mMainTxInfo, &sSavepoint );

    iduShmList::remove( &sProcInfo->mMainTxInfo, &aThrInfo->mNode );


    IDE_ASSERT( idwVLogPMMgr::writeThreadCount4Proc( &sProcInfo->mMainTxInfo,
                                                     sProcInfo->mSelfAddr,
                                                     sProcInfo->mThreadCnt )
                == IDE_SUCCESS );

    sProcInfo->mThreadCnt--;


    IDE_ASSERT( iduShmMgr::freeMem( aStatistics,
                                    &sProcInfo->mMainTxInfo,
                                    &sSavepoint,
                                    aThrInfo )
                == IDE_SUCCESS );

    /* Thread Info는 Process 비정상 종료시 WatchDog에 의해서 Free가 된다. 때문에
     * Proces Info의 ThrInfo List에 대한 연결이 완료되면 Undo Log는 불필요하다. */
    IDE_ASSERT( idrLogMgr::commit( aStatistics, &sProcInfo->mMainTxInfo )
                == IDE_SUCCESS );

    IDE_ASSERT( unlockProcess( aStatistics, sProcInfo ) == IDE_SUCCESS );

    return IDE_SUCCESS;
}

IDE_RC idwPMMgr::getDeadProc( iduShmProcInfo  ** aProcInfo )
{
    UInt              i;
    idBool            sIsAlive;
    UInt              sLPID4ADeadProc;

    sLPID4ADeadProc = IDW_LPID_NULL;

    for( i = 0; i < IDU_MAX_PROCESS_COUNT; i++ )
    {
        IDE_TEST( idwPMMgr::checkProcAliveByLPID( i, &sIsAlive )
                  != IDE_SUCCESS );

        if( sIsAlive == ID_FALSE )
        {
            sLPID4ADeadProc = i;
            break;
        }
    }

    if( sLPID4ADeadProc == IDW_LPID_NULL )
    {
        *aProcInfo = NULL;
    }
    else
    {
        *aProcInfo = getProcInfo( sLPID4ADeadProc );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC idwPMMgr::acquireSem4Proc( iduShmProcInfo  * aProcInfo )
{
    struct sembuf sSemaOP;

    sSemaOP.sem_num = aProcInfo->mLPID;
    sSemaOP.sem_op  = -1;
    sSemaOP.sem_flg = SEM_UNDO;

    IDE_TEST_RAISE( idlOS::semop( aProcInfo->mSemID,
                                  &sSemaOP,
                                  1 ) /* Operation Semaphore Count */
                    != 0, err_sem_acquire );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_sem_acquire );
    {
        IDE_SET( ideSetErrorCode( idERR_FATAL_idc_SEM_OP ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC idwPMMgr::tryAcquireSem4Proc( iduShmProcInfo  * aProcInfo,
                                     idBool          * aAcquired )
{
    struct sembuf sSemaOP;
    SInt          sRC;

    sSemaOP.sem_num = aProcInfo->mLPID;
    sSemaOP.sem_op  = -1;
    sSemaOP.sem_flg = SEM_UNDO | IPC_NOWAIT;

    sRC = idlOS::semop( aProcInfo->mSemID,
                        &sSemaOP,
                        1 /* Operation Semaphore Count */ );

    if( sRC == 0 )
    {
        *aAcquired = ID_TRUE;
    }
    else
    {
        if( errno == EAGAIN )
        {
            *aAcquired = ID_FALSE;
        }
        else
        {
            IDE_RAISE( err_sem_try_acquire );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_sem_try_acquire );
    {
        IDE_SET( ideSetErrorCode( idERR_FATAL_idc_SEM_OP ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC idwPMMgr::releaseSem4Proc( iduShmProcInfo  * aProcInfo )
{
    struct sembuf sSemaOP;

    sSemaOP.sem_num = aProcInfo->mLPID;
    sSemaOP.sem_op  = 1;
    sSemaOP.sem_flg = SEM_UNDO;

    IDE_TEST_RAISE( idlOS::semop( aProcInfo->mSemID,
                                  &sSemaOP,
                                  1 /* Operation Semaphore Count */ )
                    != 0, err_sem_release );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_sem_release );
    {
        IDE_SET( ideSetErrorCode( idERR_FATAL_idc_SEM_OP ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC idwPMMgr::checkProcAliveByLPID( idLPID aLPID, idBool * aIsAlive )
{
    pid_t             sPID;
    UChar             sIsAlive;
    idBool            sSemAcquired;
    iduShmProcInfo  * sPRTable;
    SInt              sRC;
    idBool            sIsProcAlive = ID_TRUE;

    sPRTable = mProcMgrInfo->mPRTable + aLPID;

    sPID = idCore::acpAtomicGet64( &sPRTable->mPID );

    if( ( sPID != 0 ) && ( sPRTable->mState != IDU_SHM_PROC_STATE_RECOVERY ) )
    {
        if( idlOS::getpid() != sPID )
        {
            sRC = idCore::acpProcIsAliveByPID( sPID, &sIsAlive );
            IDE_TEST_RAISE( ( sRC != 0 ) && ( sRC != EPERM ),
                            err_check_proc_alive );

            if( sIsAlive == 0 /* Dead */ )
            {
                if( sPRTable->mState != IDU_SHM_PROC_STATE_NULL )
                {
                    sPRTable->mState = IDU_SHM_PROC_STATE_DEAD;

                    sIsProcAlive = ID_FALSE;
                }
            }
            else
            {
                /* Process가 종료되었는데 mState가 IDU_SHM_PROC_STATE_RUN인것은
                 * 비정상 종료되었다는 것을 의미한다. */
                IDE_TEST( tryAcquireSem4Proc( sPRTable, &sSemAcquired )
                          != IDE_SUCCESS );

                if( sSemAcquired == ID_TRUE )
                {
                    IDL_MEM_BARRIER;

                    if( sPRTable->mState != IDU_SHM_PROC_STATE_NULL )
                    {
                        sPRTable->mState = IDU_SHM_PROC_STATE_DEAD;

                        sIsProcAlive = ID_FALSE;
                    }

                    IDE_TEST( releaseSem4Proc( sPRTable ) != IDE_SUCCESS );
                }
            }
        }
        else
        {
            IDE_ASSERT( sPRTable->mState == IDU_SHM_PROC_STATE_RUN );
        }
    }
    else
    {
        sIsProcAlive = ID_FALSE;
    }

    *aIsAlive = sIsProcAlive;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_check_proc_alive );
    {
        IDE_SET( ideSetErrorCode( idERR_FATAL_PROCESS_CHECK_ALIVE ) );
    }
    IDE_EXCEPTION_END;

    *aIsAlive = ID_FALSE;

    return IDE_FAILURE;
}

IDE_RC idwPMMgr::getNxtAliveProc( iduShmProcInfo ** aProcInfo )
{
    UInt    sFstCheckProcIdx;
    idBool  sIsAlive;
    UInt    i;

    if( *aProcInfo == NULL )
    {
        sFstCheckProcIdx = 0;
    }
    else
    {
        sFstCheckProcIdx = (*aProcInfo)->mLPID + 1;
    }

    for( i = sFstCheckProcIdx; i < IDU_MAX_PROCESS_COUNT; i++ )
    {
        IDE_TEST( checkProcAliveByLPID( i, &sIsAlive ) != IDE_SUCCESS );

        if( sIsAlive == ID_TRUE )
        {
            break;
        }
    }

    if( i != IDU_MAX_PROCESS_COUNT )
    {
        *aProcInfo = mProcMgrInfo->mPRTable + i;
    }
    else
    {
        *aProcInfo = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC idwPMMgr::getNxtDeadProc( iduShmProcInfo ** aProcInfo )
{
    UInt             sFstCheckProcIdx;
    idBool           sIsAlive;
    UInt             i;
    iduShmProcInfo * sProcInfo;

    if( *aProcInfo == NULL )
    {
        sFstCheckProcIdx = 0;
    }
    else
    {
        sFstCheckProcIdx = (*aProcInfo)->mLPID + 1;
    }

    for( i = sFstCheckProcIdx; i < IDU_MAX_PROCESS_COUNT; i++ )
    {
        IDE_TEST( checkProcAliveByLPID( i, &sIsAlive ) != IDE_SUCCESS );

        if( sIsAlive == ID_FALSE )
        {
            sProcInfo = getProcInfo( i );

            if( sProcInfo->mState == IDU_SHM_PROC_STATE_DEAD )
            {
                break;
            }
        }
    }

    if( i != IDU_MAX_PROCESS_COUNT )
    {
        *aProcInfo = mProcMgrInfo->mPRTable + i;
    }
    else
    {
        *aProcInfo = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

iduShmProcInfo * idwPMMgr::getDaemonProcInfo()
{
    return &mProcMgrInfo->mPRTable[IDU_PROC_TYPE_DAEMON];
}

IDE_RC idwPMMgr::checkDaemonProcAlive( idBool * aIsAlive )
{
    iduShmProcInfo    * sProcInfo4Daemon;

    sProcInfo4Daemon = &mProcMgrInfo->mPRTable[IDU_PROC_TYPE_DAEMON];

    return checkProcAliveByLPID( sProcInfo4Daemon->mLPID, aIsAlive );
}

IDE_RC idwPMMgr::getAliveProcCnt( UInt * aAliveProcCnt )
{
    iduShmProcInfo    * sAliveProcInfo = NULL;
    UInt                sAliveProcCnt = 0;

    IDE_ASSERT( aAliveProcCnt != NULL );

    while(1)
    {
        IDE_TEST( getNxtAliveProc( &sAliveProcInfo ) != IDE_SUCCESS );

        if( sAliveProcInfo == NULL )
        {
            break;
        }

        sAliveProcCnt++;
    }

    *aAliveProcCnt = sAliveProcCnt;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aAliveProcCnt = 0;

    return IDE_FAILURE;
}

UInt idwPMMgr::getRegistProcCnt()
{
    UInt                i;
    iduShmProcInfo    * sCurProcInfo;
    UInt                sRegistProcCnt;

    sCurProcInfo   = mProcMgrInfo->mPRTable;
    sRegistProcCnt = 0;

    for( i = 0; i < IDU_MAX_PROCESS_COUNT; i++ )
    {
        if( sCurProcInfo->mState != IDU_SHM_PROC_STATE_NULL )
        {
            sRegistProcCnt++;
        }

        sCurProcInfo++;
    }

    return sRegistProcCnt;
}

iduShmTxInfo * idwPMMgr::getTxInfo( UInt aLProcID, UInt aShmTxID )
{
    iduShmProcInfo  * sShmProcInfo;
    iduShmTxInfo    * sCurShmTxInfo;
    iduShmListNode  * sThrListNode;

    sShmProcInfo  = idwPMMgr::getProcInfo( aLProcID );
    sCurShmTxInfo = &sShmProcInfo->mMainTxInfo;

    while(1)
    {
        if( sCurShmTxInfo->mThrID == aShmTxID )
        {
            break;
        }

        sThrListNode =
            (iduShmListNode*)IDU_SHM_GET_ADDR_PTR_CHECK( sCurShmTxInfo->mNode.mAddrNext );

        sCurShmTxInfo = (iduShmTxInfo*)iduShmList::getDataPtr( sThrListNode );

        if( sCurShmTxInfo == &sShmProcInfo->mMainTxInfo )
        {
            break;
        }
    }

    return sCurShmTxInfo;
}
