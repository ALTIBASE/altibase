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

#include <idvProfile.h>
#include <idtContainer.h>
#include <mmm.h>
#include <mmErrorCode.h>
#include <mmcTask.h>
#include <mmcStatement.h>
#include <mmdManager.h>
#include <mmtAdminManager.h>
#include <mmtServiceThread.h>
#include <mmtSessionManager.h>
#include <mmtThreadManager.h>
#include <mmuOS.h>
#include <mmuProperty.h>
#include <mmtLoadBalancer.h>
#include <mtlTerritory.h>
#include <rpi.h>
#include <idmSNMP.h>

static void logIdleTimeout(mmcTask *aTask, mmcStatement *aStatement, UInt aTimeGap, UInt aTimeout);
static void logQueryTimeout(mmcTask *aTask, mmcStatement *aStatement, UInt aTimeGap, UInt aTimeout);
/* BUG-32885 Timeout for DDL must be distinct to query_timeout or utrans_timeout */
static void logDdlTimeout(mmcTask *aTask, mmcStatement *aStatement, UInt aTimeGap, UInt aTimeout);
static void logFetchTimeout(mmcTask *aTask, mmcStatement *aStatement, UInt aTimeGap, UInt aTimeout);
static void logUTransTimeout(mmcTask *aTask, mmcStatement *aStatement, UInt aTimeGap, UInt aTimeout);
static void logDdlSyncTimeout( mmcTask *aTask, mmcStatement *aStatement, UInt aTimeGap, UInt aTimeout );

enum {
    MMT_TIMEOUT_IDLE = 0,
    MMT_TIMEOUT_QUERY,
    /* BUG-32885 Timeout for DDL must be distinct to query_timeout or utrans_timeout */
    MMT_TIMEOUT_DDL,
    MMT_TIMEOUT_FETCH,
    MMT_TIMEOUT_UTRANS,
    /* PROJ-2677 Timeout for DDL synchronization */
    MMT_TIMEOUT_DDL_SYNC
};


idBool                mmtSessionManager::mRun;
PDL_Time_Value        mmtSessionManager::mCurrentTime;
mmtSessionManagerInfo mmtSessionManager::mInfo;
mmcSessID             mmtSessionManager::mNextSessionID;
iduMemPool            mmtSessionManager::mTaskPool;
iduMemPool            mmtSessionManager::mSessionPool;
iduLatch              mmtSessionManager::mLatch;
/* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
iduMemPool            mmtSessionManager::mListNodePool;
iduMemPool            mmtSessionManager::mSessionIDPool;
iduList               mmtSessionManager::mFreeSessionIDList;
iduMutex              mmtSessionManager::mMutexSessionIDList;

/* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
   BUG-29144  Load Balancing module must be separated from the session manager thread. 
*/
mmtLoadBalancer*      mmtSessionManager::mLoadBalancer;

iduList               mmtSessionManager::mTaskList =
{
    &mmtSessionManager::mTaskList,
    &mmtSessionManager::mTaskList,
    NULL
};

/* PROJ-2451 Concurrent Execute Package */
iduList               mmtSessionManager::mInternalSessionList =
{
    &mmtSessionManager::mInternalSessionList,
    &mmtSessionManager::mInternalSessionList,
    NULL
};

mmtTimeoutInfo        mmtSessionManager::mTimeoutInfo[] =
{
    {
        logIdleTimeout,
        ID_TRUE,
        &mmtSessionManager::mInfo.mIdleTimeoutCount,
        IDV_STAT_INDEX_IDLE_TIMEOUT_COUNT
    },
    {
        logQueryTimeout,
        ID_FALSE,
        &mmtSessionManager::mInfo.mQueryTimeoutCount,
        IDV_STAT_INDEX_QUERY_TIMEOUT_COUNT
    },
    /* BUG-32885 Timeout for DDL must be distinct to query_timeout or utrans_timeout */
    {
        logDdlTimeout,
        ID_FALSE,
        &mmtSessionManager::mInfo.mDdlTimeoutCount,
        IDV_STAT_INDEX_DDL_TIMEOUT_COUNT
    },
    {
        logFetchTimeout,
        ID_TRUE,
        &mmtSessionManager::mInfo.mFetchTimeoutCount,
        IDV_STAT_INDEX_FETCH_TIMEOUT_COUNT
    },
    {
        logUTransTimeout,
        ID_TRUE,
        &mmtSessionManager::mInfo.mUTransTimeoutCount,
        IDV_STAT_INDEX_UTRANS_TIMEOUT_COUNT
    },
    {
        logDdlSyncTimeout,
        ID_FALSE,
        &mmtSessionManager::mInfo.mDdlSyncTimeoutCount,
        IDV_STAT_INDEX_DDL_SYNC_TIMEOUT_COUNT
    }
};

static UInt getChannelInfo(void * /*aBaseObj*/, void *aMember, UChar *aBuf, UInt aBufSize)
{
    mmcTask *sTask = *(mmcTask **)aMember;

    IDE_TEST(cmiGetLinkInfo(sTask->getLink(),
                            (SChar *)aBuf,
                            aBufSize,
                            CMI_LINK_INFO_ALL) != IDE_SUCCESS);

    return idlOS::strlen((SChar *)aBuf);

    IDE_EXCEPTION_END;

    return 0;
}

/* PROJ-2474 SSL/TLS */
static UInt getSslCipherInfo(void * /*aBaseObj*/,
                             void *aMember, 
                             UChar *aBuf, 
                             UInt aBufSize)
{
    mmcTask *sTask = *(mmcTask **)aMember;

    IDE_TEST(cmiGetSslCipherInfo(sTask->getLink(),
                                 (SChar *)aBuf,
                                 aBufSize) != IDE_SUCCESS);

    return idlOS::strlen((SChar *)aBuf);

    IDE_EXCEPTION_END;

    return 0;
}

static UInt getSslPeerCertSubject(void * /*aBaseObj*/, 
                                  void *aMember, 
                                  UChar *aBuf, 
                                  UInt aBufSize)
{
    mmcTask *sTask = *(mmcTask **)aMember;

    IDE_TEST(cmiGetPeerCertSubject(sTask->getLink(),
                                   (SChar *)aBuf,
                                   aBufSize) != IDE_SUCCESS);

    return idlOS::strlen((SChar *)aBuf);

    IDE_EXCEPTION_END;

    return 0;
}

static UInt getSslPeerCertIssuer(void * /*aBaseObj*/,
                                 void *aMember,
                                 UChar *aBuf,
                                 UInt aBufSize)
{
    mmcTask *sTask = *(mmcTask **)aMember;

    IDE_TEST(cmiGetPeerCertIssuer(sTask->getLink(),
                                  (SChar *)aBuf,
                                  aBufSize) != IDE_SUCCESS);

    return idlOS::strlen((SChar *)aBuf);

    IDE_EXCEPTION_END;

    return 0;
}

void logIdleTimeout(mmcTask *aTask, mmcStatement * /*aStatement*/, UInt aTimeGap, UInt aTimeout)
{
    // BUG-25512 Timeout 시 altibase_boot.log에 Client PID를 출력해야합니다.
    mmcSessionInfo    *sInfo = aTask->getSession()->getInfo();
    UChar              sCommName[IDL_IP_ADDR_MAX_LEN];

    getChannelInfo(NULL, &aTask, sCommName, ID_SIZEOF(sCommName));

    ideLog::log(IDE_SERVER_1,
                MM_TRC_IDLE_TIMEOUT,
                aTask->getSession()->getSessionID(),
                sCommName,
                sInfo->mClientPID,
                aTimeout,
                aTimeGap);
}

void logQueryTimeout(mmcTask *aTask, mmcStatement *aStatement, UInt aTimeGap, UInt aTimeout)
{
    // BUG-25512 Timeout 시 altibase_boot.log에 Client PID를 출력해야합니다.
    mmcSessionInfo    *sInfo = aTask->getSession()->getInfo();
    UChar              sCommName[IDL_IP_ADDR_MAX_LEN];
    idmSNMPTrap        sTrap;
    UInt               sIsEnabledAlarm = iduProperty::getSNMPAlarmQueryTimeout();

    getChannelInfo(NULL, &aTask, sCommName, ID_SIZEOF(sCommName));

    aStatement->lockQuery();

    ideLog::log(IDE_SERVER_1,
                MM_TRC_QUERY_TIMEOUT,
                aTask->getSession()->getSessionID(),
                sCommName,
                sInfo->mClientPID,
                aTimeout,
                aTimeGap,
                aStatement->getQueryString() ? aStatement->getQueryString() : "(null)");

    aStatement->unlockQuery();

    /* PROJ-2473 SNMP 지원 */
    if (sIsEnabledAlarm > 0)
    {
        idlOS::snprintf((SChar *)sTrap.mAddress, sizeof(sTrap.mAddress),
                        "%"ID_UINT32_FMT, iduProperty::getPortNo());

        idlOS::snprintf((SChar *)sTrap.mMessage, sizeof(sTrap.mMessage),
                        MM_TRC_SNMP_QUERY_TIMEOUT,
                        aTask->getSession()->getSessionID());

        /* BUG-40283 The third parameter of strncpy() is not correct. */
        idlOS::snprintf((SChar *)sTrap.mMoreInfo, sizeof(sTrap.mMoreInfo),
                        MM_TRC_SNMP_CHECK_BOOT_LOG);

        sTrap.mLevel = 2;
        sTrap.mCode  = SNMP_ALARM_QUERY_TIMOUT;

        idmSNMP::trap(&sTrap);
    }
    else
    {
        /* Nothing */
    }
}

/* BUG-32885 Timeout for DDL must be distinct to query_timeout or utrans_timeout */
void logDdlTimeout(mmcTask *aTask, mmcStatement *aStatement, UInt aTimeGap, UInt aTimeout)
{
    mmcSessionInfo    *sInfo = aTask->getSession()->getInfo();
    UChar              sCommName[IDL_IP_ADDR_MAX_LEN];

    getChannelInfo(NULL, &aTask, sCommName, ID_SIZEOF(sCommName));

    aStatement->lockQuery();

    ideLog::log(IDE_SERVER_1,
                MM_TRC_DDL_TIMEOUT,
                aTask->getSession()->getSessionID(),
                sCommName,
                sInfo->mClientPID,
                aTimeout,
                aTimeGap,
                aStatement->getQueryString() ? aStatement->getQueryString() : "(null)");

    aStatement->unlockQuery();
}

void logFetchTimeout(mmcTask *aTask, mmcStatement *aStatement, UInt aTimeGap, UInt aTimeout)
{
    mmcTransObj       *sTrans = aTask->getSession()->getTransPtr(aStatement);
    // BUG-25512 Timeout 시 altibase_boot.log에 Client PID를 출력해야합니다.
    mmcSessionInfo    *sInfo = aTask->getSession()->getInfo();
    UChar              sCommName[IDL_IP_ADDR_MAX_LEN];
    idmSNMPTrap        sTrap;
    UInt               sIsEnabledAlarm = iduProperty::getSNMPAlarmFetchTimeout();

    getChannelInfo(NULL, &aTask, sCommName, ID_SIZEOF(sCommName));

    aStatement->lockQuery();

    ideLog::log(IDE_SERVER_1,
                MM_TRC_FETCH_TIMEOUT,
                aTask->getSession()->getSessionID(),
                sCommName,
                sInfo->mClientPID,
                aTimeout,
                aTimeGap,
                aStatement->getQueryString() ? aStatement->getQueryString() : "(null)",
                sTrans ? mmcTrans::getTransID(sTrans) : 0);

    aStatement->unlockQuery();

    /* PROJ-2473 SNMP 지원 */
    if (sIsEnabledAlarm > 0)
    {
        idlOS::snprintf((SChar *)sTrap.mAddress, sizeof(sTrap.mAddress),
                        "%"ID_UINT32_FMT, iduProperty::getPortNo());

        idlOS::snprintf((SChar *)sTrap.mMessage, sizeof(sTrap.mMessage),
                        MM_TRC_SNMP_FETCH_TIMEOUT,
                        aTask->getSession()->getSessionID());

        /* BUG-40283 The third parameter of strncpy() is not correct. */
        idlOS::snprintf((SChar *)sTrap.mMoreInfo, sizeof(sTrap.mMoreInfo),
                        MM_TRC_SNMP_CHECK_BOOT_LOG);

        sTrap.mLevel = 2;
        sTrap.mCode  = SNMP_ALARM_FETCH_TIMOUT;

        idmSNMP::trap(&sTrap);
    }
    else
    {
        /* Nothing */
    }
}

void logUTransTimeout(mmcTask *aTask, mmcStatement *aStatement, UInt aTimeGap, UInt aTimeout)
{
    mmcTransObj       *sTrans = NULL;
    // BUG-25512 Timeout 시 altibase_boot.log에 Client PID를 출력해야합니다.
    mmcSessionInfo    *sInfo = aTask->getSession()->getInfo();
    UChar              sCommName[IDL_IP_ADDR_MAX_LEN];
    idmSNMPTrap        sTrap;
    UInt               sIsEnabledAlarm = iduProperty::getSNMPAlarmUtransTimeout();

    getChannelInfo(NULL, &aTask, sCommName, ID_SIZEOF(sCommName));

    if (aStatement != NULL)
    {
        /* BUG-47057 aStatement가 NULL일 경우에 대한 방어코드 */
        sTrans = aTask->getSession()->getTransPtr(aStatement);

        // fix BUG-26994 UTRANS_TIMEOUT시 마지막 쿼리를 altibase_boot.log에 출력하도록 합니다.
        aStatement->lockQuery();

        ideLog::log(IDE_SERVER_1,
                    MM_TRC_UTRANS_TIMEOUT,
                    aTask->getSession()->getSessionID(),
                    sCommName,
                    sInfo->mClientPID,
                    aTimeout,
                    aTimeGap,
                    aStatement->getQueryString() ? aStatement->getQueryString() : "(null)",
                    sTrans != NULL ? mmcTrans::getTransID(sTrans) : 0);

        aStatement->unlockQuery();
    }
    else
    {
        /* BUG-47057 aStatement가 NULL일 경우에 대한 방어코드 */
        sTrans = aTask->getSession()->getTransPtr();

        ideLog::log(IDE_SERVER_1,
                    MM_TRC_UTRANS_TIMEOUT,
                    aTask->getSession()->getSessionID(),
                    sCommName,
                    sInfo->mClientPID,
                    aTimeout,
                    aTimeGap,
                    "(null)",
                    sTrans != NULL ? mmcTrans::getTransID(sTrans) : 0);
    }

    /* PROJ-2473 SNMP 지원 */
    if (sIsEnabledAlarm > 0)
    {
        idlOS::snprintf((SChar *)sTrap.mAddress, sizeof(sTrap.mAddress),
                        "%"ID_UINT32_FMT, iduProperty::getPortNo());

        idlOS::snprintf((SChar *)sTrap.mMessage, sizeof(sTrap.mMessage),
                        MM_TRC_SNMP_UTRANS_TIMEOUT,
                        aTask->getSession()->getSessionID());

        /* BUG-40283 The third parameter of strncpy() is not correct. */
        idlOS::snprintf((SChar *)sTrap.mMoreInfo, sizeof(sTrap.mMoreInfo),
                        MM_TRC_SNMP_CHECK_BOOT_LOG);

        sTrap.mLevel = 2;
        sTrap.mCode  = SNMP_ALARM_UTRANS_TIMOUT;

        idmSNMP::trap(&sTrap);
    }
    else
    {
        /* Nothing */
    }
}

void logDdlSyncTimeout( mmcTask *aTask, mmcStatement *aStatement, UInt aTimeGap, UInt aTimeout )
{
    mmcSessionInfo *sInfo = aTask->getSession()->getInfo();
    UChar           sCommName[IDL_IP_ADDR_MAX_LEN] = { 0, };

    (void)getChannelInfo( NULL, &aTask, sCommName, ID_SIZEOF( sCommName ) );

    aStatement->lockQuery();

    ideLog::log( IDE_SERVER_1,
                 MM_TRC_DDL_SYNC_TIMEOUT,
                 aTask->getSession()->getSessionID(),
                 sCommName,
                 sInfo->mClientPID,
                 aTimeout,
                 aTimeGap,
                 aStatement->getQueryString() ? aStatement->getQueryString() : "(null)" );

    aStatement->unlockQuery();
}

IDE_RC mmtSessionManager::initialize()
{
    mmtLoadBalancer *sLoadBalancer;
    
    mRun                      = ID_FALSE;

    mCurrentTime              = idlOS::gettimeofday();

    mInfo.mTaskCount          = 0;
    mInfo.mSessionCount       = 0;
    mInfo.mBaseTime           = (UInt)mCurrentTime.sec();
    mInfo.mLoginTimeoutCount  = 0;
    mInfo.mIdleTimeoutCount   = 0;
    mInfo.mQueryTimeoutCount  = 0;
    /* BUG-32885 Timeout for DDL must be distinct to query_timeout or utrans_timeout */
    mInfo.mDdlTimeoutCount = 0;
    mInfo.mFetchTimeoutCount  = 0;
    mInfo.mUTransTimeoutCount = 0;
    mInfo.mTerminatedCount    = 0;
    mInfo.mUpdateMaxLogSize   = 0;

    
    mNextSessionID = 0;

    IDE_TEST(mTaskPool.initialize(IDU_MEM_MMC,
                                  (SChar *)"MMC_TASK_POOL",
                                  ID_SCALABILITY_SYS,
                                  ID_SIZEOF(mmcTask),
                                  8,
                                  IDU_AUTOFREE_CHUNK_LIMIT,			/* ChunkLimit */
                                  ID_TRUE,							/* UseMutex */
                                  IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlginByte */
                                  ID_FALSE,							/* ForcePooling */
                                  ID_TRUE,							/* GarbageCollection */
                                  ID_TRUE,                          /* HWCacheLine */
                                  IDU_MEMPOOL_TYPE_LEGACY           /* mempool type*/) 
              != IDE_SUCCESS);			

    IDE_TEST(mSessionPool.initialize(IDU_MEM_MMC,
                                     (SChar *)"MMC_SESSION_POOL",
                                     ID_SCALABILITY_SYS,
                                     ID_SIZEOF(mmcSession),
                                     4,
                                     IDU_AUTOFREE_CHUNK_LIMIT,			/* ChunkLimit */
                                     ID_TRUE,							/* UseMutex */
                                     IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte */
                                     ID_FALSE,							/* ForcePooling */
                                     ID_TRUE,							/* GarbageCollection */
                                     ID_TRUE,                           /* HWCacheLine */
                                     IDU_MEMPOOL_TYPE_LEGACY            /* mempool type*/) 
               != IDE_SUCCESS);			

    IDU_LIST_INIT(&mTaskList);

    /* PROJ-2451 Concurrent Execute Package */
    IDU_LIST_INIT(&mInternalSessionList);

    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    IDE_TEST(mListNodePool.initialize(IDU_MEM_MMC,
                                      (SChar *)"MMC_LISTNODE_POOL",
                                      ID_SCALABILITY_SYS,
                                      ID_SIZEOF(iduListNode),
                                      mmuProperty::getMmtSessionListMempoolSize(),
                                      IDU_AUTOFREE_CHUNK_LIMIT,			/* ChunkLimit */
                                      ID_TRUE,							/* UseMutex */
                                      IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte */
                                      ID_FALSE,							/* ForcePooling */
                                      ID_TRUE,							/* GarbageCollection */
                                      ID_TRUE,                          /* HWCacheLine */
                                      IDU_MEMPOOL_TYPE_LEGACY           /* mempool type*/) 
             != IDE_SUCCESS);			

    IDE_TEST(mSessionIDPool.initialize(IDU_MEM_MMC,
                                       (SChar *)"MMC_SESSIONID_POOL",
                                       ID_SCALABILITY_SYS,
                                       ID_SIZEOF(mmcSessID),
                                       mmuProperty::getMmtSessionListMempoolSize(),
                                       IDU_AUTOFREE_CHUNK_LIMIT,		/* ChunkLimit */
                                       ID_TRUE,							/* UseMutex */
                                       IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte */
                                       ID_FALSE,						/* ForcePooling */
                                       ID_TRUE,							/* GarbageCollection */
                                       ID_TRUE,                          /* HWCacheLine */
                                       IDU_MEMPOOL_TYPE_LEGACY           /* mempool type*/) 
             != IDE_SUCCESS);			

    IDU_LIST_INIT(&mFreeSessionIDList);

    IDE_TEST( mLatch.initialize((SChar *)"MMT_SESSION_MANAGER_MUTEX",//
                                IDU_LATCH_TYPE_NATIVE ) != IDE_SUCCESS );

    IDE_TEST( mMutexSessionIDList.initialize((SChar *)"MMT_FREE_SESSIONID_LIST_MUTEX",
                                            IDU_MUTEX_KIND_POSIX,
                                            IDV_WAIT_INDEX_NULL) != IDE_SUCCESS);

    
/* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
   BUG-29144  Load Balancing module must be separated from the session manager thread. 
   BUG-30489  Memory critical situation must be considered in initializing mmtSessionManager.
*/    

    IDU_FIT_POINT_RAISE( "mmtSessionManager::initialize::malloc::LoadBalancer", 
                          InsufficientMemory );

    IDE_TEST_RAISE( 
        iduMemMgr::malloc( IDU_MEM_MMT,ID_SIZEOF(mmtLoadBalancer),(void**)&sLoadBalancer)
                           != IDE_SUCCESS, InsufficientMemory);

    mLoadBalancer  = new (sLoadBalancer) mmtLoadBalancer();

    //I'm not sure that this code is needed. Because "placement new" returns 
    //the pointer address that is given.But It's easy to see the code below 
    //in altibase. I just added it just in case.
    IDE_TEST( mLoadBalancer == NULL );

    return IDE_SUCCESS;

    IDE_EXCEPTION(InsufficientMemory);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC mmtSessionManager::finalize()
{
    iduListNode *sIterator;
    iduListNode *sDummy;

    IDE_TEST(mTaskPool.destroy() != IDE_SUCCESS);
    IDE_TEST(mSessionPool.destroy() != IDE_SUCCESS);

    IDE_TEST(mLatch.destroy() != IDE_SUCCESS);

    IDE_TEST(mMutexSessionIDList.destroy() != IDE_SUCCESS);

    /* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
    BUG-29144  Load Balancing module must be separated from the session manager thread. 
    */
    IDE_ASSERT( iduMemMgr::free(mLoadBalancer) == IDE_SUCCESS);
    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    IDU_LIST_ITERATE_SAFE( &mFreeSessionIDList, sIterator, sDummy )
    {
        if( sIterator->mObj != NULL )
        {
            IDE_TEST( mSessionIDPool.memfree(sIterator->mObj) != IDE_SUCCESS );
        }
        IDE_TEST( mListNodePool.memfree(sIterator) != IDE_SUCCESS );
    }

    IDE_TEST(mSessionIDPool.destroy() != IDE_SUCCESS);
    IDE_TEST(mListNodePool.destroy()  != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC mmtSessionManager::allocTask(mmcTask **aTask)
{
    mmcTask *sTask;
    UInt     sStage = 0;

    IDU_FIT_POINT_RAISE( "mmtSessionManager::allocTask::alloc::Task",
                          TaskAllocFail );

    IDE_TEST_RAISE(mTaskPool.alloc((void **)&sTask) != IDE_SUCCESS, TaskAllocFail);
    sStage++;

    IDE_TEST(sTask->initialize() != IDE_SUCCESS);
    sStage++;

    lock();
    sStage++;

    IDU_FIT_POINT_RAISE( "mmtSessionManager::allocTask::lock::getTaskCount", 
                         TaskLimitReach );

    /* BUG-28712
     * As the maximum number of clients which can be connected to the server is MAX_CLIENT + 1, 
     * allow to alloc task until the number of tasks > MAX_CLIENT + 1.
     */
    IDE_TEST_RAISE(getTaskCount() > (mmuProperty::getMaxClient() + 1), TaskLimitReach);

    IDU_LIST_ADD_LAST(&mTaskList, sTask->getAllocListNode());

    changeTaskCount(1);

    unlock();
    sStage--;

    *aTask = sTask;

    return IDE_SUCCESS;

    IDE_EXCEPTION(TaskAllocFail);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_TASK_ALLOC_FAILED));
    }
    IDE_EXCEPTION(TaskLimitReach);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_NO_AVAILABLE_TASK));
    }
    IDE_EXCEPTION_END;
    {
        switch (sStage)
        {
            case 3:
                unlock();

            case 2:
                IDE_ASSERT(sTask->finalize() == IDE_SUCCESS);

            case 1:
                IDE_ASSERT(mTaskPool.memfree(sTask) == IDE_SUCCESS);

            default:
                break;
        }
    }

    return IDE_FAILURE;
}

IDE_RC mmtSessionManager::freeTask(mmcTask *aTask)
{
    lock();
    IDU_LIST_REMOVE(aTask->getAllocListNode());
    unlock();

    IDE_TEST_RAISE(aTask->finalize() != IDE_SUCCESS, TaskFreeFail);

    IDE_TEST_RAISE(mTaskPool.memfree(aTask) != IDE_SUCCESS, TaskFreeFail);

    lock();
    changeTaskCount(-1);
    unlock();

    return IDE_SUCCESS;

    IDE_EXCEPTION(TaskFreeFail);
    {
        lock();
        changeTaskCount(-1);
        unlock();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmtSessionManager::allocInternalTask( mmcTask ** aTask,
                                             cmiLink  * aLink )
{
    mmcTask *sTask = NULL;
    UInt     sStage = 0;

    IDU_FIT_POINT_RAISE( "mmtSessionManager::allocInternalTask::alloc::Task",
                          TaskAllocFail );

    IDE_TEST_RAISE(mTaskPool.alloc((void **)&sTask) != IDE_SUCCESS, TaskAllocFail);
    sStage = 1;

    IDE_TEST(sTask->initialize() != IDE_SUCCESS);
    sStage = 2;

    lock();
    IDU_LIST_ADD_LAST(&mTaskList, sTask->getAllocListNode());
    unlock();

    // call cmiAllocCmBlock()
    IDE_TEST( sTask->setLink( aLink ) != IDE_SUCCESS );

    *aTask = sTask;

    return IDE_SUCCESS;

    IDE_EXCEPTION(TaskAllocFail);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_TASK_ALLOC_FAILED));
    }
    IDE_EXCEPTION_END;
    
    switch (sStage)
    {
        case 2:
        case 1:
            IDE_ASSERT(mTaskPool.memfree(sTask) == IDE_SUCCESS);
        default:
            break;
    }

    return IDE_FAILURE;
}

IDE_RC mmtSessionManager::freeInternalTask(mmcTask *aTask)
{
    UInt     sStage = 1;

    /* BUG-44564 cmiAllocCmBlock()으로 할당한 Memory를 cmiFreeCmBlock()으로 해제해야 한다. */
    IDE_TEST( cmiFreeCmBlock( aTask->getProtocolContext() ) != IDE_SUCCESS );

    lock();
    IDU_LIST_REMOVE(aTask->getAllocListNode());
    unlock();

    sStage = 0;
    IDE_TEST(mTaskPool.memfree(aTask) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch ( sStage )
    {
        case 1:
            (void)mTaskPool.memfree( aTask );
        default:
            break;
    }

    return IDE_FAILURE;
}

IDE_RC mmtSessionManager::allocSession(mmcTask *aTask, idBool aIsSysdba)
{
    UShort       sState    = 0;
    mmcSession  *sSession  = NULL;
    iduListNode *sListNode = NULL;
    /* PROJ-2108 Dedicated thread mode which uses less CPU */
    mmtServiceThread *sThread;
    idBool            sUsed = ID_FALSE;/* if sSession is used or not */

    IDU_FIT_POINT_RAISE( "mmtSessionManager::allocSession::alloc::Session", 
                         InsufficientMemory );

    IDE_TEST_RAISE(mSessionPool.alloc((void **)&sSession) != IDE_SUCCESS, InsufficientMemory);

    lock();
    sState++;

    if (aIsSysdba == ID_TRUE)
    {
        IDE_TEST(mmtAdminManager::setTask(aTask) != IDE_SUCCESS);
    }
    else
    {
        /* PROJ-2108 Dedicated thread mode which uses less CPU */
        if ( mmuProperty::getIsDedicatedMode() == 1 )
        {
            /* Thread Full Error */
            IDE_TEST_RAISE(mmtThreadManager::getServiceThreadSocketCount() > mmuProperty::getDedicatedThreadMaxCount(), 
                           ThreadLimitExceeded); 
        }
        
        IDU_FIT_POINT_RAISE( "mmtSessionManager::allocSession::lock::getSessionCount", 
                             TooManySession);

        IDE_TEST_RAISE(getSessionCount() >= mmuProperty::getMaxClient(), TooManySession);
    }

    if (aTask->getSession() == NULL)
    {
        sUsed = ID_TRUE;

        IDE_ASSERT( mMutexSessionIDList.lock(NULL) == IDE_SUCCESS);
        /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
        if ( IDU_LIST_IS_EMPTY(&mFreeSessionIDList) == ID_TRUE )
        {
            IDE_ASSERT( mMutexSessionIDList.unlock() == IDE_SUCCESS);
            
            IDU_FIT_POINT( "mmtSessionManager::allocSession::lock::InitializeByNewSession" );

            /* mFreeSessionIDLIST is empty */
            IDE_TEST(sSession->initialize(aTask, mNextSessionID + 1) != IDE_SUCCESS);
            mNextSessionID++;
        }
        else
        {
            /*Get a first node from mFreeSessioinIDLIST*/
            sListNode = IDU_LIST_GET_FIRST(&mFreeSessionIDList);
            IDE_ASSERT( mMutexSessionIDList.unlock() == IDE_SUCCESS);

            IDE_TEST(sSession->initialize( aTask, *((UInt*)(sListNode->mObj)) ) != IDE_SUCCESS);
            /* Remove the first node from mFreeSessioinIDLIST */
            IDU_LIST_REMOVE(sListNode);
        }

        aTask->setSession(sSession);

        changeSessionCount(1);
    }

    unlock();
    sState--;

    if ( sUsed == ID_FALSE )
    {
        IDE_ASSERT(mSessionPool.memfree(sSession) == IDE_SUCCESS);
    }
    else
    {
        /* Nothing */
    }

    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    if (sListNode != NULL)
    {
        IDE_ASSERT( mSessionIDPool.memfree(sListNode->mObj) == IDE_SUCCESS );
        IDE_ASSERT( mListNodePool.memfree(sListNode) == IDE_SUCCESS );
    }
    else
    {
        /* Nothing */
    }

    return IDE_SUCCESS;

    /* PROJ-2108 Dedicated thread mode which uses less CPU */
    IDE_EXCEPTION(ThreadLimitExceeded)
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_EXCEEDS_DEDICATED_THREAD_MAX_COUNT));
        changeTaskCount(-1);
        sThread = aTask->getThread();
        IDE_TEST(sThread->signalToServiceThread(sThread));
        sThread->stop();
        cmiShutdownLink(aTask->getLink(), CMI_DIRECTION_RD);
    }
    IDE_EXCEPTION(TooManySession)
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_TOO_MANY_SESSION));

        cmiShutdownLink(aTask->getLink(), CMI_DIRECTION_RD);
    }
    IDE_EXCEPTION(InsufficientMemory)
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    /* BUG-40610 - double free 방지 */
    if (sSession != NULL && aTask->getSession() != sSession)
    {
        IDE_ASSERT(mSessionPool.memfree(sSession) == IDE_SUCCESS);
        /* BUG-41456
         * If you'd like to add any extra exception that happens after successfully initializing a session, 
         * then you need to consider using mmcSession::finalize()
         * and changing the session count algorithm with changeSessionCount() appropriately. 
         */
        cmiSetLinkStatistics(aTask->getLink(), NULL);
    }
    if (aIsSysdba == ID_TRUE)
    {
        IDE_ASSERT(mmtAdminManager::unsetTask(aTask) == IDE_SUCCESS);
    }
    if (sState > 0)
    {
        unlock();
    }

    return IDE_FAILURE;
}

IDE_RC mmtSessionManager::freeSession(mmcTask *aTask)
{
    mmcSession  *sSession     = NULL;
    mmcSessID   *sSid         = NULL;
    iduListNode *sListNode    = NULL;

    lock();

    sSession = aTask->getSession();

    if (sSession != NULL)
    {
        aTask->setSession(NULL);
    }

    unlock();

    if (sSession != NULL)
    {
        IDU_FIT_POINT_RAISE( "mmtSessionManager::freeSession::alloc::ListNode",
                              InsufficientMemory );

        /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
        IDE_TEST_RAISE(mListNodePool.alloc((void **)&sListNode) != IDE_SUCCESS,
                       InsufficientMemory);
        
        IDU_FIT_POINT_RAISE( "mmtSessionManager::freeSession::alloc::Sid",
                              InsufficientMemory );

        IDE_TEST_RAISE(mSessionIDPool.alloc((void **)&sSid) != IDE_SUCCESS,
                       InsufficientMemory);
        *sSid = sSession->getSessionID();

        IDU_LIST_INIT_OBJ(sListNode, sSid);

        sSession->preFinalizeShardSession();

        IDE_TEST_RAISE(sSession->finalize() != IDE_SUCCESS, SessionFreeFail);

        IDE_TEST_RAISE(mSessionPool.memfree(sSession) != IDE_SUCCESS, SessionFreeFail);

        lock();
        changeSessionCount(-1);
        unlock();

        IDE_ASSERT( mMutexSessionIDList.lock(NULL) == IDE_SUCCESS);
        /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
        /*Insert the session ID value to last of mFreeSessioinIDLIST*/
        IDU_LIST_ADD_LAST(&mFreeSessionIDList, sListNode);
        IDE_ASSERT( mMutexSessionIDList.unlock() == IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(SessionFreeFail);
    {
        lock();
        changeSessionCount(-1);
        unlock();

        /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
        IDE_ASSERT( mMutexSessionIDList.lock(NULL) == IDE_SUCCESS);
        IDU_LIST_ADD_LAST(&mFreeSessionIDList, sListNode);
        IDE_ASSERT( mMutexSessionIDList.unlock() == IDE_SUCCESS);
    }
    IDE_EXCEPTION(InsufficientMemory);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
        if( sListNode != NULL )
        {
            mListNodePool.memfree(sListNode);
        }
        if( sSid != NULL )
        {
            mSessionIDPool.memfree(sSid);
        }
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC mmtSessionManager::shutdown()
{
    mmcTask     *sTask;
    iduListNode *sIterator;
    UInt         sCount = 0;
    //fix BUG-24362 dequeue 대기 세션이 close될때 , session의 queue 설정에서
    // 동시성에 문제가 있음.
    mmcSession   *sSession;
    UInt          sShutdownTimeout = mmuProperty::getShutdownImmediateTimeout();
    idBool        sIsFirst         = ID_TRUE;

    /*
     * SHUTDOWN IMMEDIATE이면 모든 세션에 세션 이벤트 전송
     */

    if (mmm::getServerStatus() == ALTIBASE_STATUS_SHUTDOWN_IMMEDIATE)
    {
        lock();

        IDU_LIST_ITERATE(&mTaskList, sIterator)
        {
            sTask = (mmcTask *)sIterator->mObj;
            sSession = sTask->getSession();
            
            if (sSession  != NULL)
            {
                IDU_SESSION_SET_CLOSED(*sSession->getEventFlag());

                //fix BUG-24362 dequeue 대기 세션이 close될때 , session의 queue 설정에서
                // 동시성에 문제가 있음.
                wakeupEnqueWaitIfNessary(sTask, sSession);
            }
            cmiShutdownLink(sTask->getLink(), CMI_DIRECTION_RD);
            // bug-28227: ipc: server stop failed when idle cli exists
            // idle한 ipc client들이 종료할때까지 대기하는 서비스
            // 쓰레드들이 있으면, server stop시 무한대기하게 된다.
            // 따라서, 강제 종료하도록 만든다.
            // cmiShutdownLink 함수를 변경안하고, cmiShutdownLinkForce
            // 새로 만든 이유는 여기저기서(lk, rp) 해당함수를 사용
            // 하기 때문이다. 여기서는 실제 IPC shutdown만 진행.
            cmiShutdownLinkForce(sTask->getLink());
        }

        unlock();
    }

    /*
     * 1초에 한번씩 Task 검사. SHUTDOWN IMMEDIATE일 경우 SHUTDOWN TIMEOUT까지만 대기
     */

    while ( ( getSessionCount() > 0 ) ||
            ( dkiGetDtxInfoCnt() > 0 ) )
    {
        if ( ( sIsFirst == ID_TRUE ) &&
             ( dkiGetDtxInfoCnt() > 0 ) )
        {
            /* BUG-47863 
               Notifier에 등록된 TX 정보가 있으면 최초로 한번만 altibase_boot.log에 남긴다. */
            sIsFirst = ID_FALSE;
            (void)dkiPrintNotifierInfo();
        }

        switch (mmm::getServerStatus())
        {
            case ALTIBASE_STATUS_SHUTDOWN_IMMEDIATE:
                // fix BUG-30566
                if (sShutdownTimeout != 0)
                {
                    IDE_TEST_RAISE(sCount >= sShutdownTimeout, ShutdownTimedOut);
                }
                break;

            case ALTIBASE_STATUS_SHUTDOWN_NORMAL:
                checkAllTask();
                break;
        }

        idlOS::sleep(1);
        IDE_CALLBACK_SEND_SYM(".");
        sCount++;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ShutdownTimedOut);
    {
        /*BUG-41590  shutdown immediate시 task가 남아 있다면 사용자가 알도록 한다. */
        IDE_CALLBACK_SEND_MSG((SChar *)MM_SESSION_MANAGER_SHUTDOWN_TASK_REMAIN);

        lock();

        ideLogEntry sLog(IDE_SERVER_0);
        sLog.append(MM_TRC_SESSION_MANAGER_SHUTDOWN_FAILED);

        IDU_LIST_ITERATE(&mTaskList, sIterator)
        {
            sTask = (mmcTask *)sIterator->mObj;

            if (sTask->getSession() != NULL)
            {
                sLog.append("\n");

                sTask->log(sLog);
            }
        }

        sLog.write();

        unlock();

        if ( dkiGetDtxInfoCnt() > 0 )
        {
            /* BUG-47863 
               Notifier에 등록된 TX 정보가 있다면 altibase_boot.log에 남긴다. */
            (void)dkiPrintNotifierInfo();
        }

    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**
 * session을 강제 종료한다.
 *
 * 강제 종료를 요청한 세션 자신은 제외한다.
 *
 * @param [in]  aWorkerSession 강제 종료를 요청한 세션
 * @param [in]  aTargetSessID  종료할 세션의 ID (ref. V$SESSION.ID)
 * @param [in]  aUserName      종료할 세션의 사용자 이름 (ref. SYSTEM_.SYS_USERS_.USER_NAME)
 *                             특정 사용자로 접속한 모든 세션을 닫고자 할 경우 설정.
 * @param [in]  aCloseAll      모든 세션을 닫을지 여부.
 *                             ID_TRUE 일 경우 aTargetSessID, aUserName은 무시.
 * @param [out] aResultCount   강제 종료한 세션의 갯수
 *
 * @return 성공하면 IDE_SUCCESS, 아니면 IDE_FAILURE
 */
IDE_RC mmtSessionManager::terminate( mmcSession *aWorkerSession,
                                     mmcSessID   aTargetSessID,
                                     SChar      *aUserName,
                                     UInt        aUserNameLen,
                                     idBool      aCloseAll,
                                     SInt       *aResultCount )
{
    #define IS_EQUAL_VALID_STR(aS1, aS2, aS2Len) \
    ( \
         (aS1 != NULL) && (aS2 != NULL) && \
         (idlOS::strlen(aS1) == aS2Len) && \
         (idlOS::strncmp(aS1, aS2, aS2Len) == 0) \
    )

    mmcTask     *sTask;
    iduListNode *sIterator;
    mmcSession  *sSession;
    SChar       *sLoginID;

    IDE_DASSERT( aWorkerSession != NULL );
    IDE_DASSERT( aResultCount != NULL );

    *aResultCount = 0;

    // bug-19279 remote sysdba enable + sys can kill session
    // 변경전: target session이 sysdba 세션이면 세션 kill 할수 없음
    // 변경후: target session이 sysdba 세션이라도 세션 kill 가능
    // (if 주석 처리, 나중에 변경소지 있으므로 남겨둔다)
    // 변경이유: sys 계정은 세션 kill 능력을 가지게 된다. sys 계정은
    // 경우에 따라 다른 sysdba 세션을 kill 할 필요가 있다.
    // ex) a가 원격으로 sysdba로 접속후 사용을 안하고 있다.
    // b가 급하게 sysdba로 접속할 필요가 있는데 sysdba는 동시 1 세션만
    // 가능하므로 접속을 못하게 된다.
    // 이때, b가 sys로 접속해 a의 sysdba 세션을 강제로 kill 한다.
    // 이제 sysdba 세션이 없으므로, b는 다시 sysdba로 접속가능하다

    /* BUGBUG (2016-07-14) QP에서 확인하고 오는데, ASSERT가 나을까? */
    IDE_TEST_RAISE( aWorkerSession->getUserInfo()->loginUserID != QC_SYS_USER_ID,
                    PERMITION_ERROR );

    lock();

    IDU_LIST_ITERATE(&mTaskList, sIterator)
    {
        sTask = (mmcTask *)sIterator->mObj;
        sSession = sTask->getSession();
        sLoginID = sSession->getUserInfo()->loginID;

        if ( (sSession == NULL) ||
             (sSession->getSessionID() == aWorkerSession->getSessionID()) ||
             ( ( aWorkerSession->getShardPIN() != SDI_SHARD_PIN_INVALID ) &&
               ( sSession->getShardPIN() == aWorkerSession->getShardPIN()) ) )
        {
            continue;
        }
        else if ( (aCloseAll == ID_TRUE) ||
                  ((aTargetSessID != 0) && (sSession->getSessionID() == aTargetSessID)) ||
                  IS_EQUAL_VALID_STR(sLoginID, aUserName, aUserNameLen) )
        {
            IDU_SESSION_SET_CLOSED(*sSession->getEventFlag());

            if (cmiShutdownLink(sTask->getLink(), CMI_DIRECTION_RD) != IDE_SUCCESS)
            {
                IDE_WARNING(IDE_SERVER_0, MM_TRC_CM_SHUTDOWN_FAILURE);
            }

            //PROJ-1677 DEQUEUE
            //fix BUG-24362 dequeue 대기 세션이 close될때 , session의 queue 설정에서
            // 동시성에 문제가 있음.
            wakeupEnqueWaitIfNessary(sTask, sSession);

            mInfo.mTerminatedCount++;

            IDV_SESS_ADD_DIRECT(sSession->getStatistics(),
                                IDV_STAT_INDEX_SESSION_TERMINATED_COUNT,
                                1);

            (*aResultCount)++;

            // BUG-26088 mmcStatement::beginDB() 에서 valgrind 오류가 발생합니다.
            // 로그 남기는 부분을 실제 session close 하는곳으로 옴깁니다.
            if (sSession->getSessionID() == mmtAdminManager::getSessionID())
            {
                ideLog::log(IDE_SERVER_0,
                            MM_TRC_SYSDBA_SESSION_CLOSED,
                            sSession->getSessionID(),
                            aWorkerSession->getSessionID());
            }

            if ( (aCloseAll == ID_FALSE) && (aUserName == NULL) )
            {
                break;
            }
        }
    }

    unlock();

    return IDE_SUCCESS;

    IDE_EXCEPTION( PERMITION_ERROR )
    {
        IDE_SET( ideSetErrorCode(mmERR_ABORT_SESSION_CLOSE_NOT_PERMITTED) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

    #undef IS_EQUAL_VALID_STR
}

void mmtSessionManager::run()
{
    PDL_Time_Value sSleepTime;
    UInt           sCompactTime = 0;
    UInt           sDetectTime  = 0;

    mRun = ID_TRUE;

#if defined(WRS_VXWORKS) 
    if( taskPrioritySet( taskIdSelf(), 99 ) != OK ) 
    { 
        ideLog::logMessage( IDE_SERVER_0, 
                            "Session manager taskPrioritySet(99) error: %d", 
                            errno ); 
    } 
#endif 
    IDE_ASSERT(mLoadBalancer->start() == IDE_SUCCESS);
    /* [BUG-29592] A responsibility for updating  a global session time have better to be assigned to
       session manager    rather than load balancer*/
    IDE_ASSERT(mLoadBalancer->waitToStart(MMT_THR_CREATE_WAIT_TIME) == IDE_SUCCESS);

    while (mRun == ID_TRUE)
    {

        // 1 sec
        sSleepTime.set(1, 0);
        idlOS::sleep(sSleepTime);
        // [BUG-29592] A responsibility for updating  a global session time have better to be assigned to session manager
        //rather than load balancer
        mCurrentTime    = idlOS::gettimeofday();
        mInfo.mBaseTime = (UInt)mCurrentTime.sec();
        
        if ((getBaseTime() - sCompactTime) >= mmuProperty::getMemoryCompactTime())
        {
            // fix BUG-37960
            (void)mmuOS::heapmin();

            sCompactTime = getBaseTime();
        }

        if ((getBaseTime() - sDetectTime) >= mmuProperty::getCmDetectTime())
        {

            checkAllTask();

            idvManager::setLinkCheckTime(getBaseTime());

            if ((mmuProperty::getXaComplete() > 0) && (mmuProperty::getXaTimeout() > 0))
            {
                mmdManager::checkXaTimeout();
            }

            sDetectTime = getBaseTime();
        }//if
    }//while
}

/* BUG-48515 */
idBool mmtSessionManager::isRun()
{
    return mRun;
}

void mmtSessionManager::stop()
{
    mRun = ID_FALSE;
    mLoadBalancer->stop();
    IDE_ASSERT(mLoadBalancer->join() == IDE_SUCCESS);
}

void mmtSessionManager::logSessionOverview(ideLogEntry &aLog)
{
    aLog.appendFormat(MM_TRC_SESSION_OVERVIEW ,
                      mNextSessionID + 1,
                      mmuProperty::getMaxClient(),
                      getSessionCount(),
                      mmtThreadManager::getServiceThreadSocketCount() +
                      mmtThreadManager::getServiceThreadIPCCount());
}

/*
 * BUG-39098 The altibase_error.log doesn't have session info
 *           for debug when server ASSERT.
 *
 * aThreadID type을 UInt -> ULong <idlOS::getThreadID()의 리턴타입>으로 변경.
 */
idBool mmtSessionManager::logTaskOfThread(ideLogEntry &aLog, ULong aThreadID)
{
    mmtServiceThread *sThread;
    mmcSession       *sSession;
    mmcTask          *sTask;
    iduListNode      *sIterator;

    IDU_LIST_ITERATE(&mTaskList, sIterator)
    {
        sTask    = (mmcTask *)sIterator->mObj;
        sSession = sTask->getSession();
        sThread  = sTask->getThread();

        // PROJ-2118 Bug Reporting
        if ( sSession != NULL )
        {
            sSession->dumpSessionProperty( aLog, sSession );
        }
        
        if ((sThread != NULL) && (idlOS::getThreadID(sThread->getTid()) == aThreadID))
        {
            sTask->log(aLog);

            return ID_TRUE;
        }
    }

    return ID_FALSE;
}

void mmtSessionManager::checkAllTask()
{
    mmcTask      *sTask;
    iduListNode  *sIterator;
    mmcSession   *sSession;
    SChar         sConnection[IDL_IP_ADDR_MAX_LEN];

    UInt          sIndexTaskSchedMax = IDV_STAT_INDEX_OPTM_TASK_SCHEDULE_MAX;
    ULong        *sSysSchedMaxTime;
    ULong         sSessSchedMaxTime;
    idvSession   *sStatSess;

    /* bug-35395: task schedule time added to v$sysstat
       직접 update 할 것이므로, pointer를 구해온다 */
    sSysSchedMaxTime = &(gSystemInfo.mStatEvent[sIndexTaskSchedMax].mValue);

    lock();

    IDU_LIST_ITERATE(&mTaskList, sIterator)
    {
        sTask = (mmcTask *)sIterator->mObj;

        if (sTask->getSession() == NULL)
        {
            /*
             * Login Timeout 검사
             */

            if ((mmuProperty::getLoginTimeout() > 0) &&
                (mmuProperty::getLoginTimeout() <= (getBaseTime() - sTask->getConnectTime())))
            {
                // fix BUG-27965 이미 체크한 task는 제외
                if (sTask->isLoginTimeoutTask() == ID_FALSE)
                {
                    cmiShutdownLink(sTask->getLink(), CMI_DIRECTION_RD);

                    // fix BUG-27965 Connection 정보 출력
                    if (cmiGetLinkInfo(sTask->getLink(),
                                       sConnection,
                                       ID_SIZEOF(sConnection),
                                       CMI_LINK_INFO_ALL) == IDE_SUCCESS)
                    {
                        ideLog::log(IDE_SERVER_1, MM_TRC_LOGIN_TIMEOUT, sConnection);
                    }

                    sTask->setLoginTimeoutTask(ID_TRUE);
                    mInfo.mLoginTimeoutCount++;
                }
            }
        }
        else
        {
            /*
             * Admin Task는 Session Timeout 체크를 하지 않는다.
             */
            if (mmtAdminManager::getTask() != sTask)
            {
                sSession = sTask->getSession();
                //PROJ-1677 DEQUEUE
                if( sTask->getTaskState() ==  MMC_TASK_STATE_QUEUEWAIT)
                {

                    if(iduCheckSessionEvent(sSession->getStatSQL()) != IDE_SUCCESS)
                    {
                        // close되었음.
                        //fix BUG-24362 dequeue 대기 세션이 close될때 , session의 queue 설정에서
                        // 동시성에 문제가 있음.
                        wakeupEnqueWaitIfNessary(sTask, sSession);
                    }
                    /* BUG-33067 Utrans timeout should be checked for dequeue */
                    else
                    {
                        checkSessionTimeout(sTask);
                    }
                }
                else
                {
                    if (!IDU_SESSION_CHK_CLOSED(*sSession->getEventFlag()))
                    {
                        // close되지 않은 session.
                        checkSessionTimeout(sTask);
                    }
                }//else

                /* bug-35395: task schedule time added to v$sysstat
                   system의 schedule max time은 세션중 가장 큰 값만 저장.
                   누적값이 아니므로 applyStatisticsToSystem 에서는 무시 */
                if (iduProperty::getTimedStatistics() == IDV_TIMED_STATISTICS_ON)
                {
                    sStatSess = sSession->getStatistics();
                    sSessSchedMaxTime = sStatSess->mStatEvent[sIndexTaskSchedMax].mValue;
                    if (sSessSchedMaxTime > *sSysSchedMaxTime)
                    {
                        *sSysSchedMaxTime = sSessSchedMaxTime;
                    }
                }
            }
            /*
             * 주기적으로 세션의 통계정보를 반영한다.
             */
            sTask->getSession()->applyStatisticsToSystem();
        }
    }

    unlock();

    /*
     * 자동으로 반영되지 않는 시스템 통계정보를 반영한다.
     */

    IDV_SYS_SET(IDV_STAT_INDEX_LOGON_CURR, (ULong)getTaskCount());

    IDV_SYS_SET(IDV_STAT_INDEX_DETECTOR_BASE_TIME, (ULong)getBaseTime());

    // fix BUG-30731
    mmcStatementManager::applyStatisticsForSystem();

    smiApplyStatisticsForSystem();
    /* fix BUG-31545 replication statistics */
    rpi::applyStatisticsForSystem();

    idvProfile::writeSystem();
    idvProfile::writeMemInfo();
    (void)idvProfile::flushAllBufferToFile();

    /*
     * MsgLog의 내용을 flush 시킨다.
     */

    ideLog::flushAllModuleLogs();
}

void mmtSessionManager::checkSessionTimeout(mmcTask *aTask)
{
    mmcSession   *sSession = aTask->getSession();
    mmcStatement *sStatement;
    mmcTransObj  *sTrans;
    iduListNode  *sIterator;
    iduList      *sStmtList = NULL;

    /*
     * Close Event가 세팅된 세션은 검사하지 않는다.
     */


    /*
     * Idle Timeout 검사
     */

    if (checkTimeoutEvent(aTask,
                          NULL,
                          sSession->getIdleStartTime(),
                          sSession->getIdleTimeout(),
                          MMT_TIMEOUT_IDLE) == ID_TRUE)
    {
        return;
    }

    /*
     * Statement Timeout 검사
     */

    sSession->lockForStmtList();

    // bug-25988: utrans_timeout never occured if stmt already freed
    // autocommit off 상태에서 DML을 수행하여 TX가 살아있을 때,
    // 해당 stmt가 이미 close 된 경우에도 utrans_timeout은 발생해야함.
    // 수정전: session의 stmtlist가 비어있으면, timeout 검사안함.
    // 수정후
    // : session의 stmtlist가 비어있어도 autocommit off이고
    // TX가 있으면 utrans_timeout 검사
    sStmtList = sSession->getStmtList();
    if (IDU_LIST_IS_EMPTY(sStmtList))
    {
        if (sSession->getCommitMode() == MMC_COMMITMODE_NONAUTOCOMMIT)
        {
            sTrans     = sSession->getTransPtr();
            if (sTrans != NULL)
            {
                // 2번째 인자로 stmt가 없으므로 NULL로 넘긴다
                // stmt가 사용되는 곳:
                // logUtransTimeout 안의 getTrans(stmt..)에서 무시됨
                checkTimeoutEvent(aTask,
                        NULL,
                        mmcTrans::getFirstUpdateTime(sTrans),
                        sSession->getUTransTimeout(),
                        MMT_TIMEOUT_UTRANS);
            }

        }

    }
    // stmt list에 stmt가 있는 경우 순회하면서 timeout 검사
    else
    {
        IDU_LIST_ITERATE(sStmtList, sIterator)
        {
            sStatement = (mmcStatement *)sIterator->mObj;
            sTrans     = sSession->getTransPtr(sStatement);

            /*
             * Fetch Timeout 검사
             */

            if (checkTimeoutEvent(aTask,
                        sStatement,
                        sStatement->getFetchStartTime(),
                        sSession->getFetchTimeout(),
                        MMT_TIMEOUT_FETCH) == ID_TRUE)
            {
                break;
            }

            /* BUG-32885 Timeout for DDL must be distinct to query_timeout or utrans_timeout */
            if ( qciMisc::isStmtDDL(sStatement->getStmtType()) == ID_FALSE )
            {
                /*
                 * UTrans Timeout 검사
                 */
                if ((sTrans != NULL) &&
                    (checkTimeoutEvent(aTask,
                                       sStatement,
                                       mmcTrans::getFirstUpdateTime(sTrans),
                                       sSession->getUTransTimeout(),
                                       MMT_TIMEOUT_UTRANS) == ID_TRUE))
                {
                    break;
                }

                /*
                 * Query Timeout 검사
                 */
                (void)checkTimeoutEvent( aTask,
                                         sStatement,
                                         sStatement->getQueryStartTime(),
                                         sSession->getQueryTimeout(),
                                         MMT_TIMEOUT_QUERY );
            }
            else
            {
                /*
                 * DDL Timeout 검사
                 */
                (void)checkTimeoutEvent( aTask,
                                         sStatement,
                                         sStatement->getQueryStartTime(),
                                         sSession->getDdlTimeout(),
                                         MMT_TIMEOUT_DDL );

                if ( sSession->getReplicationDDLSync() == 1 )
                {
                    (void)checkTimeoutEvent( aTask,
                                             sStatement,
                                             sStatement->getQueryStartTime(),
                                             sSession->getReplicationDDLSyncTimeout(),
                                             MMT_TIMEOUT_DDL_SYNC );
                }
                else
                {
                    /* nothing to do */
                }
            }
        }
    }

    sSession->unlockForStmtList();
}

/* PROJ-2177 User Interface - Cancel */
/**
 * Cancel 이벤트를 설정한다.
 *
 * Cancel 이벤트는 Session Event이므로
 * Session에서 수행중인 Statement를 바꾸는 동작과 동시에 일어나서는 안된다.
 *
 * @param aStmtID statement id
 * @return 성공하면 IDE_SUCCESS, 해당하는 statement나 session이 없으면 IDE_FAILURE
 */
IDE_RC mmtSessionManager::setCancelEvent(mmcStmtID aStmtID)
{
    mmcStatement    *sStmt      = NULL;
    mmcSession      *sSession   = NULL;

    IDE_TEST(mmcStatementManager::findStatement(&sStmt, NULL, aStmtID) != IDE_SUCCESS);

    sSession = sStmt->getSession();
    /* 이런일은 일어나선 안된다. 만약 발생했다면, Stmt관리에 구멍이 있는 것. */
    IDE_ASSERT(sSession != NULL);

    if ((sStmt->isExecuting() == ID_TRUE)
     && (sSession->getInfo()->mCurrStmtID == aStmtID))
    {
        IDU_SESSION_SET_CANCELED(*sSession->getEventFlag());
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* BUG-25020 
 * 해당 Statement를 Query Timeout 상태로 강제로 변경한다.
 * P.S. 이 함수는 XA에서 해당 Xid가 Active 상태일 경우 TM으로 부터 Rollback 명령을 받았을 경우
 * 해당 Statement를 정리하기 위해 QueryTimeout을 발생시키기 위해서 추가됨.
 *
 * BUG-25323
 * 해당 Session이 종료하는 과정에면.. Task값이 NULL이 될 수 있다.
 * 따라서 Task에서 Session을 얻는게 아니라 Session값을 인자로 하여 직접 넘겨 받도록 해야한다.
 *
 * BUG-42866 
 * The query timeout of this function must be separately considered 
 * from the general timeout rule HDB has added by BUG-38472 
 * since this timeout is to roll the performed XA transaction back when a XA_ROLLBACK has arrived,
 * not a real query timeout (BUG-25020).
 */
void mmtSessionManager::setQueryTimeoutEvent(mmcSession  *aSession,     
                                             mmcStmtID    aStmtID)
{
    mmcStatement   * sStatement;
    idBool           sEventOccured = ID_FALSE;

    mmtTimeoutInfo * sTimeoutInfo;

    IDE_TEST( aSession == NULL );

    sTimeoutInfo = &mTimeoutInfo[MMT_TIMEOUT_QUERY];

    IDE_TEST( mmcStatementManager::findStatement( &sStatement, aSession, aStmtID )
              != IDE_SUCCESS );

    /* BUG-38472 Query timeout applies to one statement. */
    if ( sStatement->getTimeoutEventOccured() != ID_TRUE )
    {
        sEventOccured = ID_TRUE;
    }
    else
    {
        /* Nothing to do */
    }

    /* BUG-38472 Query timeout applies to one statement. */
    if ( !IDU_SESSION_CHK_TIMEOUT( *aSession->getEventFlag(),
                                   sStatement->getStmtID() ) )
    {
        IDU_SESSION_SET_TIMEOUT( *aSession->getEventFlag(), aStmtID );
    }
    else
    {
        /* Nothing to do */
    }

    if ( sEventOccured == ID_TRUE )
    {
        (*sTimeoutInfo->mCount)++;

        IDV_SESS_ADD_DIRECT( aSession->getStatistics(), sTimeoutInfo->mStatIndex, 1 );
    }
    else
    {
        /* Nothing to do */
    }

    IDE_EXCEPTION_END;
}
idBool mmtSessionManager::checkTimeoutEvent(mmcTask      *aTask,
                                            mmcStatement *aStatement,
                                            UInt          aStartTime,
                                            UInt          aTimeout,
                                            UInt          aTimeoutIndex)
{
    mmtTimeoutInfo *sTimeoutInfo;
    UInt            sTimeGap;
    idBool          sEventOccured = ID_FALSE;

    /*
     * Timeout이 설정되어 있고 TimeGap이 Timeout보다 크면 Timeout Event 발생
     */

    //fix [BUG-27123 : mm/NEW] [5.3.3 release Code-Sonar] mm Ignore return values series 2
    if ( (aStartTime > 0) && (aTimeout > 0) )
    {
        // BUG-25231 BaseTIme 보다 StartTime이 더 큰 경우, Timeout이 발생하지 않아야 합니다.
        if( getBaseTime() <= aStartTime )
        {
            sTimeGap = 0;
        }
        else
        {
            sTimeGap = getBaseTime() - aStartTime;
        }
        //fix [BUG-27123 : mm/NEW] [5.3.3 release Code-Sonar] mm Ignore return values series 2
        /* 
         * BUG-35376 The query's execution time must be guaranteed
         *           until 'QUERY_TIMEOUT' at least.
         */
        if(sTimeGap >= aTimeout + 1)
        {   
            sTimeoutInfo = &mTimeoutInfo[aTimeoutIndex];

            if (sTimeoutInfo->mShouldClose == ID_TRUE)
            {
                if (IDU_SESSION_CHK_CLOSED(*aTask->getSession()->getEventFlag()) == 0)
                {
                    sTimeoutInfo->mLogFunc(aTask, aStatement, sTimeGap, aTimeout);
                    sEventOccured = ID_TRUE;
                }
                else
                {
                    /* Nothing to do */
                }

                IDU_SESSION_SET_CLOSED(*aTask->getSession()->getEventFlag());

                cmiShutdownLink(aTask->getLink(), CMI_DIRECTION_RD);
            }
            else
            {
                // bug-26974: codesonar: statement null ref.
                // 본 함수는 checkSessionTimeout 에서만 호출하고
                // 로직상 여기 수행시 statement가 null일 수 없지만
                // 방어용으로 검사 수행.
                if (aStatement != NULL)
                {
                    /* BUG-38472 Query timeout applies to one statement. */
                    if ( aStatement->getTimeoutEventOccured() != ID_TRUE )
                    {
                        aStatement->setTimeoutEventOccured( ID_TRUE );

                        sTimeoutInfo->mLogFunc(aTask, aStatement, sTimeGap, aTimeout);
                        sEventOccured = ID_TRUE;
                    }
                    else
                    {
                        /* Nothing to do */
                    }

                    /* BUG-38472 Query timeout applies to one statement. */
                    if ( ( aTask->getSession()->getInfo()->mCurrStmtID == aStatement->getStmtID() ) &&
                         ( !IDU_SESSION_CHK_TIMEOUT( *aTask->getSession()->getEventFlag(),
                                                     aStatement->getStmtID() ) ) )
                    {
                        IDU_SESSION_SET_TIMEOUT( *aTask->getSession()->getEventFlag(),
                                                 aStatement->getStmtID() );
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                }
                // 만약 null이면 log를 남기고, timeout은 발생시키지 않음.
                // log가 발생하면 왜 statement가 null인지 확인 필요.
                else
                {
                    ideLog::log(IDE_SERVER_0,
                                "checkTimeoutEvent: statement is NULL. "
                                "timeout [type:%d] ignored", aTimeoutIndex);
                }
            }

            if ( sEventOccured == ID_TRUE )
            {
                (*sTimeoutInfo->mCount)++;

                IDV_SESS_ADD_DIRECT(aTask->getSession()->getStatistics(), sTimeoutInfo->mStatIndex, 1);
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            /* Nothing to do */
        }
    }

    return sEventOccured;
}

 //fix BUG-24362 dequeue 대기 세션이 close될때 , session의 queue 설정에서
 // 동시성에 문제가 있음.
void mmtSessionManager::wakeupEnqueWaitIfNessary(mmcTask*  aTask,
                                                 mmcSession * aSession)
{

    //fix BUG-24362 dequeue 대기 세션이 close될때 , session의 queue 설정에서
    // 동시성에 문제가 있음.
    mmqQueueInfo *sQueueInfo;
    
    if( aTask->getTaskState() ==  MMC_TASK_STATE_QUEUEWAIT)
    {
        //fix BUG-24362 dequeue 대기 세션이 close될때 , session의 queue 설정에서
        // 동시성에 문제가 있음.
        sQueueInfo = aSession->getQueueInfo();
        
        if( sQueueInfo != NULL)
        {
            sQueueInfo->lock();
            
            (void)sQueueInfo->broadcastEnqueue();
            sQueueInfo->unlock();
        }
        else
        {
            //nothing to do.
        }    
    }
    else
    {
        //nothing to do
    }
}

/*
 * Fixed Table Definition for SESSION
 */


static iduFixedTableColDesc gSESSIONColDesc[] =
{
    {
        (SChar *)"ID",
        offsetof(mmcSessionInfo4PerfV, mSessionID),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mSessionID),
        /*BUG-42791 큰값일 경우 v$에서 session id값이 음수가 나오는 경우가 있습니다.*/
        IDU_FT_TYPE_UBIGINT | IDU_FT_COLUMN_INDEX,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"TRANS_ID",
        offsetof(mmcSessionInfo4PerfV, mTransID),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mTransID),
        //fix BUG-24289 V$SESSION의 Trans_id를 BIGINT으로 display.
        IDU_FT_TYPE_UBIGINT | IDU_FT_COLUMN_INDEX,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"TASK_STATE",
        offsetof(mmcSessionInfo4PerfV, mTaskState),
        IDU_FT_SIZEOF_INTEGER,
        IDU_FT_TYPE_UINTEGER | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"EVENTFLAG",
        offsetof(mmcSessionInfo4PerfV, mEventFlag),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mEventFlag),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"COMM_NAME",
        offsetof(mmcSessionInfo4PerfV, mCommName),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mCommName) - 1,
        IDU_FT_TYPE_VARCHAR | IDU_FT_COLUMN_INDEX,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"XA_SESSION_FLAG",
        offsetof(mmcSessionInfo4PerfV, mXaSessionFlag),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mXaSessionFlag),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"XA_ASSOCIATE_FLAG",
        offsetof(mmcSessionInfo4PerfV, mXaAssocState),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mXaAssocState),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"QUERY_TIME_LIMIT",
        offsetof(mmcSessionInfo4PerfV, mQueryTimeout),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mQueryTimeout),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"DDL_TIME_LIMIT",
        offsetof(mmcSessionInfo4PerfV, mDdlTimeout),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mDdlTimeout),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"FETCH_TIME_LIMIT",
        offsetof(mmcSessionInfo4PerfV, mFetchTimeout),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mFetchTimeout),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"UTRANS_TIME_LIMIT",
        offsetof(mmcSessionInfo4PerfV, mUTransTimeout),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mUTransTimeout),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"IDLE_TIME_LIMIT",
        offsetof(mmcSessionInfo4PerfV, mIdleTimeout),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mIdleTimeout),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"IDLE_START_TIME",
        offsetof(mmcSessionInfo4PerfV, mIdleStartTime),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mIdleStartTime),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"ACTIVE_FLAG",
        offsetof(mmcSessionInfo4PerfV, mActivated),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mActivated),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"OPENED_STMT_COUNT",
        offsetof(mmcSessionInfo4PerfV, mOpenStmtCount),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mOpenStmtCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"CLIENT_PACKAGE_VERSION",
        offsetof(mmcSessionInfo4PerfV, mClientPackageVersion),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mClientPackageVersion) - 1,
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"CLIENT_PROTOCOL_VERSION",
        offsetof(mmcSessionInfo4PerfV, mClientProtocolVersion),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mClientProtocolVersion) - 1,
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"CLIENT_PID",
        offsetof(mmcSessionInfo4PerfV, mClientPID),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mClientPID),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"CLIENT_TYPE",
        offsetof(mmcSessionInfo4PerfV, mClientType),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mClientType) - 1,
        IDU_FT_TYPE_VARCHAR | IDU_FT_COLUMN_INDEX,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"CLIENT_APP_INFO",
        offsetof(mmcSessionInfo4PerfV, mClientAppInfo),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mClientAppInfo) - 1,
        IDU_FT_TYPE_VARCHAR | IDU_FT_COLUMN_INDEX,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"MODULE",
        offsetof( mmcSessionInfo4PerfV, mModuleInfo ),
        IDU_FT_SIZEOF( mmcSessionInfo4PerfV, mModuleInfo ) - 1,
        IDU_FT_TYPE_VARCHAR,
        NULL /*mConvCallback*/,
        0    /*mColOffset*/,
        0    /*mColSize*/,
        NULL /*mTableName*/ // for internal use
    },
    {
        (SChar *)"ACTION",
        offsetof( mmcSessionInfo4PerfV, mActionInfo ),
        IDU_FT_SIZEOF( mmcSessionInfo4PerfV, mActionInfo ) - 1,
        IDU_FT_TYPE_VARCHAR,
        NULL /*mConvCallback*/,
        0    /*mColOffset*/,
        0    /*mColSize*/,
        NULL /*mTableName*/ // for internal use
    },
    {
        (SChar *)"CLIENT_NLS",
        offsetof(mmcSessionInfo4PerfV, mNlsUse),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mNlsUse) - 1,
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"DB_USERNAME",
        offsetof(mmcSessionInfo4PerfV, mUserInfo) + offsetof(qciUserInfo, loginID),
        IDU_FT_SIZEOF(qciUserInfo, loginID) - 1,
        IDU_FT_TYPE_VARCHAR | IDU_FT_COLUMN_INDEX,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"DB_USERID",
        offsetof(mmcSessionInfo4PerfV, mUserInfo) + offsetof(qciUserInfo, loginUserID),
        IDU_FT_SIZEOF(qciUserInfo, loginUserID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"DEFAULT_TBSID",
        offsetof(mmcSessionInfo4PerfV, mUserInfo) + offsetof(qciUserInfo, tablespaceID),
        IDU_FT_SIZEOF(qciUserInfo, tablespaceID),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"DEFAULT_TEMP_TBSID",
        offsetof(mmcSessionInfo4PerfV, mUserInfo) + offsetof(qciUserInfo, tempTablespaceID),
        IDU_FT_SIZEOF(qciUserInfo, tempTablespaceID),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"SYSDBA_FLAG",
        offsetof(mmcSessionInfo4PerfV, mUserInfo) + offsetof(qciUserInfo, mIsSysdba),
        IDU_FT_SIZEOF(qciUserInfo, mIsSysdba),
        IDU_FT_TYPE_UINTEGER | IDU_FT_COLUMN_INDEX,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"AUTOCOMMIT_FLAG",
        offsetof(mmcSessionInfo4PerfV, mCommitMode),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mCommitMode),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"SESSION_STATE",
        offsetof(mmcSessionInfo4PerfV, mSessionState),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mSessionState),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"ISOLATION_LEVEL",
        offsetof(mmcSessionInfo4PerfV, mIsolationLevel),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mIsolationLevel),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"REPLICATION_MODE",
        offsetof(mmcSessionInfo4PerfV, mReplicationMode),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mReplicationMode),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"TRANSACTION_MODE",
        offsetof(mmcSessionInfo4PerfV, mTransactionMode),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mTransactionMode),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"COMMIT_WRITE_WAIT_MODE",
        offsetof(mmcSessionInfo4PerfV, mCommitWriteWaitMode),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mCommitWriteWaitMode),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"TRX_UPDATE_MAX_LOGSIZE",
        offsetof(mmcSessionInfo4PerfV, mUpdateMaxLogSize),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mUpdateMaxLogSize),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"OPTIMIZER_MODE",
        offsetof(mmcSessionInfo4PerfV, mOptimizerMode),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mOptimizerMode),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"HEADER_DISPLAY_MODE",
        offsetof(mmcSessionInfo4PerfV, mHeaderDisplayMode),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mHeaderDisplayMode),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"CURRENT_STMT_ID",
        offsetof(mmcSessionInfo4PerfV, mCurrStmtID),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mCurrStmtID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"STACK_SIZE",
        offsetof(mmcSessionInfo4PerfV, mStackSize),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mStackSize),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"DEFAULT_DATE_FORMAT",
        offsetof(mmcSessionInfo4PerfV, mDateFormat),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mDateFormat) - 1,
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"PARALLEL_DML_MODE",
        offsetof(mmcSessionInfo4PerfV, mParallelDmlMode),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mParallelDmlMode),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {   // To Fix BUG-18721
        (SChar *)"LOGIN_TIME",
        offsetof(mmcSessionInfo4PerfV, mConnectTime),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mConnectTime),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"NLS_NCHAR_CONV_EXCP",
        offsetof(mmcSessionInfo4PerfV, mNlsNcharConvExcp),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mNlsNcharConvExcp),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"NLS_NCHAR_LITERAL_REPLACE",
        offsetof(mmcSessionInfo4PerfV, mNlsNcharLiteralReplace),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mNlsNcharLiteralReplace),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"AUTO_REMOTE_EXEC",
        offsetof(mmcSessionInfo4PerfV, mAutoRemoteExec),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mAutoRemoteExec),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"FAILOVER_SOURCE",
        offsetof(mmcSessionInfo4PerfV, mFailOverSource),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mFailOverSource) - 1,
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"NLS_TERRITORY",
        offsetof(mmcSessionInfo4PerfV, mNlsTerritory),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mNlsTerritory) - 1,
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"NLS_ISO_CURRENCY",
        offsetof(mmcSessionInfo4PerfV, mNlsISOCurrency),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mNlsISOCurrency) - 1,
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"NLS_CURRENCY",
        offsetof(mmcSessionInfo4PerfV, mNlsCurrency),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mNlsCurrency) - 1,
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"NLS_NUMERIC_CHARACTERS",
        offsetof(mmcSessionInfo4PerfV, mNlsNumChar),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mNlsNumChar) - 1,
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"TIME_ZONE",
        offsetof(mmcSessionInfo4PerfV, mTimezoneString),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mTimezoneString) - 1,
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL // for internal use
    },
    /* PROJ-2047 Strengthening LOB - LOBCACHE */
    {
        (SChar *)"LOB_CACHE_THRESHOLD",
        offsetof(mmcSessionInfo4PerfV, mLobCacheThreshold),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mLobCacheThreshold),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"QUERY_REWRITE_ENABLE",
        offsetof(mmcSessionInfo4PerfV, mQueryRewriteEnable),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mQueryRewriteEnable),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"DBLINK_GLOBAL_TRANSACTION_LEVEL",
        offsetof(mmcSessionInfo4PerfV, mDblinkGlobalTransactionLevel),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV,mDblinkGlobalTransactionLevel),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"DBLINK_REMOTE_STATEMENT_AUTOCOMMIT",
        offsetof(mmcSessionInfo4PerfV, mDblinkRemoteStatementAutoCommit),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV,mDblinkRemoteStatementAutoCommit),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar *)"MAX_STATEMENTS_PER_SESSION",
        offsetof(mmcSessionInfo4PerfV, mMaxStatementsPerSession ),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mMaxStatementsPerSession),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    /* PROJ-2474 SSL/TLS */
    {    
        (SChar *)"SSL_CIPHER",
        offsetof(mmcSessionInfo4PerfV, mSslCipher),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mSslCipher) - 1,
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL // for internal use
    },   
    {    
        (SChar *)"SSL_CERTIFICATE_SUBJECT",
        offsetof(mmcSessionInfo4PerfV, mSslPeerCertSubject),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mSslPeerCertSubject) - 1,
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL // for internal use
    },   
    {    
        (SChar *)"SSL_CERTIFICATE_ISSUER",
        offsetof(mmcSessionInfo4PerfV, mSslPeerCertIssuer),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mSslPeerCertIssuer) - 1,
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"REPLICATION_DDL_SYNC",
        offsetof( mmcSessionInfo4PerfV, mReplicationDDLSync ),
        IDU_FT_SIZEOF( mmcSessionInfo4PerfV, mReplicationDDLSync ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"REPLICATION_DDL_SYNC_TIMELIMIT",
        offsetof( mmcSessionInfo4PerfV, mReplicationDDLSyncTimeout ),
        IDU_FT_SIZEOF( mmcSessionInfo4PerfV, mReplicationDDLSyncTimeout ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {    
        (SChar *)"SHARD_PIN",
        offsetof(mmcSessionInfo4PerfV, mShardPinStr),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mShardPinStr) - 1,
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL
    },
    {   /* BUG-46090 Meta Node SMN 전파 */
        (SChar *)"SHARD_META_NUMBER",
        offsetof( mmcSessionInfo4PerfV, mShardMetaNumber ),
        IDU_FT_SIZEOF( mmcSessionInfo4PerfV, mShardMetaNumber ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"SHARD_CLIENT",
        offsetof(mmcSessionInfo4PerfV, mShardClient),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mShardClient),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar *)"SHARD_SESSION_TYPE",
        offsetof(mmcSessionInfo4PerfV, mShardSessionType),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mShardSessionType),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar *)"MESSAGE_CALLBACK",  /* BUG-46019 */
        offsetof(mmcSessionInfo4PerfV, mMessageCallback),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mMessageCallback),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"GLOBAL_TRANSACTION_LEVEL",
        offsetof(mmcSessionInfo4PerfV, mGlobalTransactionLevel),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV,mGlobalTransactionLevel),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {   /* BUG-47655 Session 별 Transaction 할당 재시도 횟수 */
        (SChar *)"ALLOC_TRANSACTION_RETRY_COUNT",
        offsetof(mmcSessionInfo4PerfV, mAllocTransRetryCount),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mAllocTransRetryCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    }, // PROJ-2727
    {
        (SChar *)"NORMALFORM_MAXIMUM",
        offsetof(mmcSessionInfo4PerfV, mNormalFormMaximum),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV,mNormalFormMaximum),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar *)"__OPTIMIZER_DEFAULT_TEMP_TBS_TYPE",
        offsetof(mmcSessionInfo4PerfV, mOptimizerDefaultTempTbsType),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV,mOptimizerDefaultTempTbsType),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar *)"SHARD_INTERNAL_LOCAL_OPERATION",
        offsetof(mmcSessionInfo4PerfV, mShardInternalLocalOperation),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mShardInternalLocalOperation),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar *)"ST_OBJECT_BUFFER_SIZE",
        offsetof(mmcSessionInfo4PerfV, mSTObjBufSize),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV,mSTObjBufSize),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar *)"TRCLOG_DETAIL_PREDICATE",
        offsetof(mmcSessionInfo4PerfV, mTrclogDetailPredicate),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV,mTrclogDetailPredicate),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar *)"OPTIMIZER_DISK_INDEX_COST_ADJ",
        offsetof(mmcSessionInfo4PerfV, mOptimizerDiskIndexCostAdj),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV,mOptimizerDiskIndexCostAdj),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar *)"OPTIMIZER_MEMORY_INDEX_COST_ADJ",
        offsetof(mmcSessionInfo4PerfV, mOptimizerMemoryIndexCostAdj),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV,mOptimizerMemoryIndexCostAdj),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar *)"RECYCLEBIN_ENABLE",
        offsetof(mmcSessionInfo4PerfV, mRecyclebinEnable),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV,mRecyclebinEnable),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar *)"__USE_OLD_SORT",
        offsetof(mmcSessionInfo4PerfV, mUseOldSort),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV,mUseOldSort),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar *)"ARITHMETIC_OPERATION_MODE",
        offsetof(mmcSessionInfo4PerfV, mArithmeticOpMode),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV,mArithmeticOpMode),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar *)"RESULT_CACHE_ENABLE",
        offsetof(mmcSessionInfo4PerfV, mResultCacheEnable),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV,mResultCacheEnable),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar *)"TOP_RESULT_CACHE_MODE",
        offsetof(mmcSessionInfo4PerfV, mTopResultCacheMode),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV,mTopResultCacheMode),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar *)"OPTIMIZER_AUTO_STATS",
        offsetof(mmcSessionInfo4PerfV, mOptimizerAutoStats),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV,mOptimizerAutoStats),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar *)"__OPTIMIZER_TRANSITIVITY_OLD_RULE",
        offsetof(mmcSessionInfo4PerfV, mOptimizerTransitivityOldRule),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV,mOptimizerTransitivityOldRule),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar *)"OPTIMIZER_PERFORMANCE_VIEW",
        offsetof(mmcSessionInfo4PerfV, mOptimizerPerformanceView),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV,mOptimizerPerformanceView),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar *)"__PRINT_OUT_ENABLE",
        offsetof(mmcSessionInfo4PerfV, mPrintOutEnable),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV,mPrintOutEnable),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
        },
    {
        (SChar *)"TRCLOG_DETAIL_SHARD",
        offsetof(mmcSessionInfo4PerfV, mTrclogDetailShard),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV,mTrclogDetailShard),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar *)"SERIAL_EXECUTE_MODE",
        offsetof(mmcSessionInfo4PerfV, mSerialExecuteMode),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV,mSerialExecuteMode),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
        },
    {
        (SChar *)"TRCLOG_DETAIL_INFORMATION",
        offsetof(mmcSessionInfo4PerfV, mTrcLogDetailInformation),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV,mTrcLogDetailInformation),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar *)"__REDUCE_PARTITION_PREPARE_MEMORY",
        offsetof(mmcSessionInfo4PerfV, mReducePartPrepareMemory),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV,mReducePartPrepareMemory),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar *)"TRANSACTIONAL_DDL",
        offsetof(mmcSessionInfo4PerfV, mTransactionalDDL),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mTransactionalDDL),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"GLOBAL_DDL",
        offsetof(mmcSessionInfo4PerfV, mGlobalDDL),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mGlobalDDL),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"GCTX_COORD_SCN",
        offsetof( mmcSessionInfo4PerfV, mGCTxCommitInfo.mCoordSCN ),
        IDU_FT_SIZEOF( mmcSessionInfo4PerfV, mGCTxCommitInfo.mCoordSCN ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar *)"GCTX_PREPARE_SCN",
        offsetof( mmcSessionInfo4PerfV, mGCTxCommitInfo.mPrepareSCN ),
        IDU_FT_SIZEOF( mmcSessionInfo4PerfV, mGCTxCommitInfo.mPrepareSCN ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar *)"GCTX_GLOBAL_COMMIT_SCN",
        offsetof( mmcSessionInfo4PerfV, mGCTxCommitInfo.mGlobalCommitSCN ),
        IDU_FT_SIZEOF( mmcSessionInfo4PerfV, mGCTxCommitInfo.mGlobalCommitSCN ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar *)"GCTX_LAST_SYSTEM_SCN",
        offsetof( mmcSessionInfo4PerfV, mGCTxCommitInfo.mLastSystemSCN ),
        IDU_FT_SIZEOF( mmcSessionInfo4PerfV, mGCTxCommitInfo.mLastSystemSCN ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar *)"SHARD_STATEMENT_RETRY",
        offsetof( mmcSessionInfo4PerfV, mShardStatementRetry ),
        IDU_FT_SIZEOF( mmcSessionInfo4PerfV, mShardStatementRetry ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar *)"INDOUBT_FETCH_TIMEOUT",
        offsetof( mmcSessionInfo4PerfV, mIndoubtFetchTimeout ),
        IDU_FT_SIZEOF( mmcSessionInfo4PerfV, mIndoubtFetchTimeout ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar *)"INDOUBT_FETCH_METHOD",
        offsetof( mmcSessionInfo4PerfV, mIndoubtFetchMethod ),
        IDU_FT_SIZEOF( mmcSessionInfo4PerfV, mIndoubtFetchMethod ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        (SChar *)"SHARD_DDL_LOCK_TIMEOUT",
        offsetof(mmcSessionInfo4PerfV, mShardDDLLockTimeout),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mShardDDLLockTimeout),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"SHARD_DDL_LOCK_TRY_COUNT",
        offsetof(mmcSessionInfo4PerfV, mShardDDLLockTryCount),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mShardDDLLockTryCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"DDL_LOCK_TIMEOUT",
        offsetof(mmcSessionInfo4PerfV, mDDLLockTimeout),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mDDLLockTimeout),
        IDU_FT_TYPE_INTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"__OPTIMIZER_PLAN_HASH_OR_SORT_METHOD",
        offsetof(mmcSessionInfo4PerfV, mPlanHashOrSortMethod),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mPlanHashOrSortMethod),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"__OPTIMIZER_BUCKET_COUNT_MAX",
        offsetof(mmcSessionInfo4PerfV, mBucketCountMax),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mBucketCountMax),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"__OPTIMIZER_ELIMINATE_COMMON_SUBEXPRESSION",
        offsetof(mmcSessionInfo4PerfV, mEliminateCommonSubexpression),
        IDU_FT_SIZEOF(mmcSessionInfo4PerfV, mEliminateCommonSubexpression),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"LAST_SHARD_META_NUMBER",
        offsetof( mmcSessionInfo4PerfV, mLastShardMetaNumber ),
        IDU_FT_SIZEOF( mmcSessionInfo4PerfV, mLastShardMetaNumber ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar *)"RECEIVED_SHARD_META_NUMBER",
        offsetof( mmcSessionInfo4PerfV, mReceivedShardMetaNumber ),
        IDU_FT_SIZEOF( mmcSessionInfo4PerfV, mReceivedShardMetaNumber ),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL
    },
    {
        (SChar *)"SHARD_STMT_EXEC_SEQ",
        offsetof( mmcSessionInfo4PerfV, mStmtExecSeqForShardTx ),
        IDU_FT_SIZEOF( mmcSessionInfo4PerfV, mStmtExecSeqForShardTx ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0, NULL // for internal use
    }
};


IDE_RC mmtSessionManager::buildRecordForSESSION(idvSQL              * /*aStatistics*/,
                                                void        *aHeader,
                                                void        * /* aDumpObj */,
                                                iduFixedTableMemory *aMemory)
{
    mmcTask        *sTask;
    iduListNode    *sIterator;
    mmcSession     *sSess;
    mmcSessionInfo *sSession;
    mmcSessionInfo4PerfV sSessionInfo;
    void           * sIndexValues[7];

    lockRead();

    IDU_LIST_ITERATE(&mTaskList, sIterator)
    {
        sTask = (mmcTask *)sIterator->mObj;
        sSess = sTask->getSession();

        if ( sSess != NULL )
        {
            sSession = sSess->getInfo();

            //fix BUG-23656 session,xid ,transaction을 연계한 performance view를 제공하고,
            //그들간의 관계를 정확히 유지해야 함.
            // transaction을 사용하는 session의 변경.
            /* BUG-44967 */
            sSessionInfo.mTransID                = sSession->mTransID;
            sSessionInfo.mSessionID              = sSession->mSessionID;
            sSessionInfo.mTaskState              = sSession->mTaskState;
            sSessionInfo.mEventFlag              = sSession->mEventFlag;
            sSessionInfo.mAllocTransRetryCount   = sSession->mAllocTransRetryCount; // BUG-47655

            getChannelInfo(NULL, &sTask, sSessionInfo.mCommName,
                           ID_SIZEOF(sSessionInfo.mCommName));

            /* BUG-43006 FixedTable Indexing Filter
             * Column Index 를 사용해서 전체 Record를 생성하지않고
             * 부분만 생성해 Filtering 한다.
             * 1. void * 배열에 IDU_FT_COLUMN_INDEX 로 지정된 컬럼에
             * 해당하는 값을 순서대로 넣어주어야 한다.
             * 2. IDU_FT_COLUMN_INDEX의 컬럼에 해당하는 값을 모두 넣
             * 어 주어야한다.
             */
            sIndexValues[0] = &sSessionInfo.mSessionID;
            sIndexValues[1] = &sSessionInfo.mTransID;
            sIndexValues[2] = &sSessionInfo.mCommName;
            sIndexValues[3] = &sSession->mClientType;
            sIndexValues[4] = &sSession->mClientAppInfo;
            sIndexValues[5] = &sSession->mUserInfo.loginID;
            sIndexValues[6] = &sSession->mUserInfo.mIsSysdba;
            if ( iduFixedTable::checkKeyRange( aMemory,
                                               gSESSIONColDesc,
                                               sIndexValues )
                 == ID_FALSE )
            {
                continue;
            }
            else
            {
                /* Nothing to do */
            }
            /* PROJ-2474 SSL/TLS */
            (void)getSslCipherInfo(NULL, &sTask, sSessionInfo.mSslCipher,
                                   ID_SIZEOF(sSessionInfo.mSslCipher));

            (void)getSslPeerCertSubject(NULL, &sTask, sSessionInfo.mSslPeerCertSubject,
                                        ID_SIZEOF(sSessionInfo.mSslPeerCertSubject));

            (void)getSslPeerCertIssuer(NULL, &sTask, sSessionInfo.mSslPeerCertIssuer,
                                       ID_SIZEOF(sSessionInfo.mSslPeerCertIssuer));

            sSessionInfo.mXaSessionFlag          = sSession->mXaSessionFlag;
            sSessionInfo.mXaAssocState           = sSession->mXaAssocState;
            sSessionInfo.mQueryTimeout           = sSession->mQueryTimeout;
            /* BUG-32885 Timeout for DDL must be distinct to query_timeout or utrans_timeout */
            sSessionInfo.mDdlTimeout             = sSession->mDdlTimeout;
            sSessionInfo.mFetchTimeout           = sSession->mFetchTimeout;
            sSessionInfo.mUTransTimeout          = sSession->mUTransTimeout;
            sSessionInfo.mIdleTimeout            = sSession->mIdleTimeout;
            sSessionInfo.mIdleStartTime          = sSession->mIdleStartTime;
            sSessionInfo.mActivated              = sSession->mActivated;
            sSessionInfo.mOpenStmtCount          = sSession->mOpenStmtCount;
            idlOS::memcpy( sSessionInfo.mClientPackageVersion,
                           sSession->mClientPackageVersion,
                           ID_SIZEOF(SChar) * (MMC_CLIENTINFO_MAX_LEN + 1) );
            idlOS::memcpy( sSessionInfo.mClientProtocolVersion,
                           sSession->mClientProtocolVersion,
                           ID_SIZEOF(SChar) * (MMC_CLIENTINFO_MAX_LEN + 1) );
            sSessionInfo.mClientPID = sSession->mClientPID;
            idlOS::memcpy( sSessionInfo.mClientType,
                           sSession->mClientType,
                           ID_SIZEOF(SChar) * (MMC_CLIENTINFO_MAX_LEN + 1) );
            idlOS::memcpy( sSessionInfo.mClientAppInfo,
                           sSession->mClientAppInfo,
                           ID_SIZEOF(SChar) * (MMC_APPINFO_MAX_LEN + 1) );
            idlOS::memcpy( sSessionInfo.mModuleInfo,
                           sSession->mModuleInfo,
                           ID_SIZEOF(SChar) * (MMC_APPINFO_MAX_LEN + 1) );
            idlOS::memcpy( sSessionInfo.mActionInfo,
                           sSession->mActionInfo,
                           ID_SIZEOF(SChar) * (MMC_APPINFO_MAX_LEN + 1) );
            idlOS::memcpy( sSessionInfo.mNlsUse,
                           sSession->mNlsUse,
                           ID_SIZEOF(SChar) * (QC_MAX_NAME_LEN + 1) );
            sSessionInfo.mUserInfo               = sSession->mUserInfo;
            sSessionInfo.mCommitMode             = sSession->mCommitMode;
            sSessionInfo.mSessionState           = sSession->mSessionState;
            sSessionInfo.mIsolationLevel         = sSession->mIsolationLevel;
            sSessionInfo.mReplicationMode        = sSession->mReplicationMode;
            sSessionInfo.mTransactionMode        = sSession->mTransactionMode;
            sSessionInfo.mCommitWriteWaitMode    = sSession->mCommitWriteWaitMode;
            sSessionInfo.mUpdateMaxLogSize       = sSession->mUpdateMaxLogSize;
            sSessionInfo.mOptimizerMode          = sSession->mOptimizerMode;
            sSessionInfo.mHeaderDisplayMode      = sSession->mHeaderDisplayMode;
            sSessionInfo.mCurrStmtID             = sSession->mCurrStmtID;
            sSessionInfo.mStackSize              = sSession->mStackSize;
            idlOS::memcpy( sSessionInfo.mDateFormat,
                           sSession->mDateFormat,
                           ID_SIZEOF(SChar) * (MMC_DATEFORMAT_MAX_LEN + 1) );
            sSessionInfo.mParallelDmlMode        = sSession->mParallelDmlMode;
            sSessionInfo.mConnectTime            = sSession->mConnectTime;
            sSessionInfo.mNlsNcharConvExcp       = sSession->mNlsNcharConvExcp;
            sSessionInfo.mNlsNcharLiteralReplace = sSession->mNlsNcharLiteralReplace;
            sSessionInfo.mAutoRemoteExec         = sSession->mAutoRemoteExec;
            /* PROJ-2047 Strengthening LOB - LOBCACHE */
            sSessionInfo.mLobCacheThreshold      = sSession->mLobCacheThreshold;

            sSessionInfo.mReplicationDDLSync        = sSession->mReplicationDDLSync;
            sSessionInfo.mReplicationDDLSyncTimeout = sSession->mReplicationDDLSyncTimeout;

            /* BUG-31390 Failover info for v$session */
            idlOS::memcpy( sSessionInfo.mFailOverSource,
                           sSession->mFailOverSource,
                           IDL_IP_ADDR_MAX_LEN );

            /* PROJ-2208 Multi Currency */
            IDE_TEST( mtlTerritory::setNlsTerritoryName( sSession->mNlsTerritory,
                                                         sSessionInfo.mNlsTerritory)
                      != IDE_SUCCESS );

            IDE_TEST( mtlTerritory::setNlsISOCurrencyName( sSession->mNlsISOCurrency,
                                                           sSessionInfo.mNlsISOCurrency)
                      != IDE_SUCCESS );
            idlOS::memcpy( sSessionInfo.mNlsCurrency,
                           sSession->mNlsCurrency,
                           MTL_TERRITORY_CURRENCY_LEN );
            sSessionInfo.mNlsCurrency[MTL_TERRITORY_CURRENCY_LEN] = 0;

            idlOS::memcpy( sSessionInfo.mNlsNumChar,
                           sSession->mNlsNumChar,
                           MTL_TERRITORY_NUMERIC_CHAR_LEN );
            sSessionInfo.mNlsNumChar[MTL_TERRITORY_NUMERIC_CHAR_LEN] = 0;

            /* PROJ-2209 DBTIMEZONE */
            idlOS::memcpy ( sSessionInfo.mTimezoneString,
                            sSession->mTimezoneString,
                            MMC_TIMEZONE_MAX_LEN + 1 );

            /*
             * Database Link session property
             */ 
            sSessionInfo.mDblinkGlobalTransactionLevel = sSession->mGlobalTransactionLevel;
            sSessionInfo.mGlobalTransactionLevel = sSession->mGlobalTransactionLevel;

            sSessionInfo.mDblinkRemoteStatementAutoCommit = sSession->mDblinkRemoteStatementAutoCommit;
                            
            /* PROJ-1090 Function-based Index */
            sSessionInfo.mQueryRewriteEnable = sSession->mQueryRewriteEnable;

            sSessionInfo.mMaxStatementsPerSession = sSession->mMaxStatementsPerSession;

            sdi::shardPinToString( sSessionInfo.mShardPinStr,
                                   ID_SIZEOF( sSessionInfo.mShardPinStr ),
                                   sSession->mShardPin );

            /* BUG-46090 Meta Node SMN 전파 */
            sSessionInfo.mShardMetaNumber = sSession->mShardMetaNumber;

            /* BUG-45707 */
            sSessionInfo.mShardClient = sSession->mShardClient;
            sSessionInfo.mShardSessionType = sSession->mShardSessionType;

            sSessionInfo.mMessageCallback = sSession->mMessageCallback;  /* BUG-46019 */
            sSessionInfo.mShardInternalLocalOperation = sSession->mShardInternalLocalOperation;
            // PROJ-2727
            sSessionInfo.mNormalFormMaximum            = sSession->mNormalFormMaximum;
            sSessionInfo.mOptimizerDefaultTempTbsType  = sSession->mOptimizerDefaultTempTbsType;
            sSessionInfo.mSTObjBufSize                 = sSession->mSTObjBufSize;
            sSessionInfo.mTrclogDetailPredicate        = sSession->mTrclogDetailPredicate;
            sSessionInfo.mOptimizerDiskIndexCostAdj    = sSession->mOptimizerDiskIndexCostAdj;
            sSessionInfo.mOptimizerMemoryIndexCostAdj  = sSession->mOptimizerMemoryIndexCostAdj;
            sSessionInfo.mRecyclebinEnable             = sSession->mRecyclebinEnable;
            sSessionInfo.mUseOldSort                   = sSession->mUseOldSort;
            sSessionInfo.mArithmeticOpMode             = sSession->mArithmeticOpMode;
            sSessionInfo.mResultCacheEnable            = sSession->mResultCacheEnable;
            sSessionInfo.mTopResultCacheMode           = sSession->mTopResultCacheMode;
            sSessionInfo.mOptimizerAutoStats           = sSession->mOptimizerAutoStats;
            sSessionInfo.mOptimizerTransitivityOldRule = sSession->mOptimizerTransitivityOldRule;
            sSessionInfo.mOptimizerPerformanceView     = sSession->mOptimizerPerformanceView;
            sSessionInfo.mPrintOutEnable               = sSession->mPrintOutEnable;
            sSessionInfo.mTrclogDetailShard            = sSession->mTrclogDetailShard;
            sSessionInfo.mSerialExecuteMode            = sSession->mSerialExecuteMode;
            sSessionInfo.mTrcLogDetailInformation      = sSession->mTrcLogDetailInformation;
            sSessionInfo.mReducePartPrepareMemory      = sSession->mReducePartPrepareMemory;            
            sSessionInfo.mTransactionalDDL             = sSession->mTransactionalDDL;
            sSessionInfo.mGlobalDDL                    = sSession->mGlobalDDL;
            sSessionInfo.mShardDDLLockTimeout          = sSession->mShardDDLLockTimeout;
            sSessionInfo.mShardDDLLockTryCount         = sSession->mShardDDLLockTryCount;
            sSessionInfo.mDDLLockTimeout               = sSession->mDDLLockTimeout;

            /* BUG-48132 */
            sSessionInfo.mPlanHashOrSortMethod = sSession->mPlanHashOrSortMethod;

            /* BUG-48161 */
            sSessionInfo.mBucketCountMax = sSession->mBucketCountMax;

            /* BUG-48348 */
            sSessionInfo.mEliminateCommonSubexpression = sSession->mEliminateCommonSubexpression;

            sSess->getCoordSCN( sSess->getShardClientInfo(), &sSessionInfo.mGCTxCommitInfo.mCoordSCN );
            SM_SET_SCN( &sSessionInfo.mGCTxCommitInfo.mPrepareSCN, &sSession->mGCTxCommitInfo.mPrepareSCN );
            SM_SET_SCN( &sSessionInfo.mGCTxCommitInfo.mGlobalCommitSCN, &sSession->mGCTxCommitInfo.mGlobalCommitSCN );
            SM_SET_SCN( &sSessionInfo.mGCTxCommitInfo.mLastSystemSCN, &sSession->mGCTxCommitInfo.mLastSystemSCN );

            sSessionInfo.mShardStatementRetry          = sSession->mShardStatementRetry;
            sSessionInfo.mIndoubtFetchTimeout          = sSession->mIndoubtFetchTimeout;
            sSessionInfo.mIndoubtFetchMethod           = sSession->mIndoubtFetchMethod;

            sSessionInfo.mLastShardMetaNumber          = sSession->mLastShardMetaNumber;
            sSessionInfo.mReceivedShardMetaNumber      = sSession->mReceivedShardMetaNumber;

            /* TASK-7219 Non-shard DML */
            sSessionInfo.mStmtExecSeqForShardTx        = sSession->mStmtExecSeqForShardTx;

            IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                                aMemory,
                                                (void *)&sSessionInfo) != IDE_SUCCESS);
         }
    }

    unlock();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    unlock();

    return IDE_FAILURE;
}

iduFixedTableDesc gSESSIONTableDesc =
{
    (SChar *)"X$SESSION",
    mmtSessionManager::buildRecordForSESSION,
    gSESSIONColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

/*
 * Fixed Table Definition for SESSIONMGR
 */

static iduFixedTableColDesc gSESSIONMGRColDesc[] =
{
    // task
    {
        (SChar *)"TASK_COUNT",
        offsetof(mmtSessionManagerInfo, mTaskCount),
        IDU_FT_SIZEOF(mmtSessionManagerInfo, mTaskCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"BASE_TIME",
        offsetof(mmtSessionManagerInfo, mBaseTime),
        IDU_FT_SIZEOF(mmtSessionManagerInfo, mBaseTime),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"LOGIN_TIMEOUT_COUNT",
        offsetof(mmtSessionManagerInfo, mLoginTimeoutCount),
        IDU_FT_SIZEOF(mmtSessionManagerInfo, mLoginTimeoutCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"IDLE_TIMEOUT_COUNT",
        offsetof(mmtSessionManagerInfo, mIdleTimeoutCount),
        IDU_FT_SIZEOF(mmtSessionManagerInfo, mIdleTimeoutCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"QUERY_TIMEOUT_COUNT",
        offsetof(mmtSessionManagerInfo, mQueryTimeoutCount),
        IDU_FT_SIZEOF(mmtSessionManagerInfo, mQueryTimeoutCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    /* BUG-32885 Timeout for DDL must be distinct to query_timeout or utrans_timeout */
    {
        (SChar *)"DDL_TIMEOUT_COUNT",
        offsetof(mmtSessionManagerInfo, mDdlTimeoutCount),
        IDU_FT_SIZEOF(mmtSessionManagerInfo, mDdlTimeoutCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"FETCH_TIMEOUT_COUNT",
        offsetof(mmtSessionManagerInfo, mFetchTimeoutCount),
        IDU_FT_SIZEOF(mmtSessionManagerInfo, mFetchTimeoutCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"UTRANS_TIMEOUT_COUNT",
        offsetof(mmtSessionManagerInfo, mUTransTimeoutCount),
        IDU_FT_SIZEOF(mmtSessionManagerInfo, mUTransTimeoutCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"SESSION_TERMINATE_COUNT",
        offsetof(mmtSessionManagerInfo, mTerminatedCount),
        IDU_FT_SIZEOF(mmtSessionManagerInfo, mTerminatedCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0, NULL // for internal use
    }
};

IDE_RC mmtSessionManager::buildRecordForSESSIONMGR(idvSQL              * /*aStatistics*/,
                                                   void      *aHeader,
                                                   void      * /* aDumpObj */,
                                                   iduFixedTableMemory *aMemory)
{
    IDE_TEST(iduFixedTable::buildRecord(aHeader, aMemory, (void *)&mInfo) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

iduFixedTableDesc gSESSIONMGRTableDesc =
{
    (SChar *)"X$SESSIONMGR",
    mmtSessionManager::buildRecordForSESSIONMGR,
    gSESSIONMGRColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};


/*
 * Fixed Table Definition for SESSTAT
 */

static iduFixedTableColDesc gSESSTATColDesc[] =
{
    // task
    {
        (SChar *)"SID",
        offsetof(idvSessStatFT, mSID),
        IDU_FT_SIZEOF_UINTEGER,
        IDU_FT_TYPE_UINTEGER | IDU_FT_COLUMN_INDEX,
        NULL,
        0, 0, NULL // for internal use
    },
    // task
    {
        (SChar *)"SEQNUM",
        offsetof(idvSessStatFT, mStatEvent) + offsetof(idvStatEvent, mEventID),
        IDU_FT_SIZEOF_UINTEGER,
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"VALUE",
        offsetof(idvSessStatFT, mStatEvent) + offsetof(idvStatEvent, mValue),
        IDU_FT_SIZEOF_BIGINT,
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0, NULL // for internal use
    }
};

IDE_RC mmtSessionManager::buildRecordForSESSTAT(idvSQL              * /*aStatistics*/,
                                                void *aHeader,
                                                void * /* aDumpObj */,
                                                iduFixedTableMemory *aMemory)
{
    idvSession    *sSessStat;
    mmcTask       *sTask;
    iduListNode   *sIterator;
    idvSessStatFT  sSessStatFT;
    UInt           i;
    void         * sIndexValues[1];

    lockRead();

    IDU_LIST_ITERATE(&mTaskList, sIterator)
    {
        sTask = (mmcTask *)sIterator->mObj;

        if (sTask->getSession() != NULL)
        {
            sSessStatFT.mSID = sTask->getSession()->getSessionID();

            /* BUG-43006 FixedTable Indexing Filter
             * Indexing Filter를 사용해서 전체 Record를 생성하지않고
             * 부분만 생성해 Filtering 한다.
             * 1. void * 배열에 IDU_FT_COLUMN_INDEX 로 지정된 컬럼에
             * 해당하는 값을 순서대로 넣어주어야 한다.
             * 2. IDU_FT_COLUMN_INDEX의 컬럼에 해당하는 값을 모두 넣
             * 어 주어야한다.
             */
            sIndexValues[0] = &sSessStatFT.mSID;
            if ( iduFixedTable::checkKeyRange( aMemory,
                                               gSESSTATColDesc,
                                               sIndexValues )
                 == ID_FALSE )
            {
                continue;
            }
            else
            {
                /* Nothing to do */
            }
            for (i = 0; i < IDV_STAT_INDEX_MAX; i++)
            {
                sSessStat  = sTask->getSession()->getStatistics();
                
                sSessStatFT.mStatEvent = sSessStat->mStatEvent[i];

                IDU_FIT_POINT("mmtSessionManager::buildRecordForSESSTAT::lock::buildRecord");
                IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                                    aMemory,
                                                    (void *)&sSessStatFT)
                         != IDE_SUCCESS);
            }
        }
    }

    unlock();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    unlock();

    return IDE_FAILURE;
}

iduFixedTableDesc gSESSTATTableDesc =
{
    (SChar *)"X$SESSTAT",
    mmtSessionManager::buildRecordForSESSTAT,
    gSESSTATColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};


/*
 * Fixed Table Definition for SYSSTAT
 */

static iduFixedTableColDesc gSYSSTATColDesc[] =
{
    // task
    {
        (SChar *)"SEQNUM",
        offsetof(idvSessStatFT, mStatEvent) + offsetof(idvStatEvent, mEventID),
        IDU_FT_SIZEOF_UINTEGER,
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"VALUE",
        offsetof(idvSessStatFT, mStatEvent) + offsetof(idvStatEvent, mValue),
        IDU_FT_SIZEOF_BIGINT,
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0, NULL // for internal use
    }
};

IDE_RC mmtSessionManager::buildRecordForSYSSTAT(idvSQL              * /*aStatistics*/,
                                                void *aHeader,
                                                void * /* aDumpObj */,
                                                iduFixedTableMemory *aMemory)
{
    idvSessStatFT sSysStatFT;
    UInt        i;

    for (i = IDV_STAT_INDEX_BEGIN; i < IDV_STAT_INDEX_MAX; i++)
    {
        sSysStatFT.mStatEvent = gSystemInfo.mStatEvent[i];

        IDE_TEST( iduFixedTable::buildRecord(aHeader,
                                             aMemory,
                                             (void *)&sSysStatFT)
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

iduFixedTableDesc gSYSSTATTableDesc =
{
    (SChar *)"X$SYSSTAT",
    mmtSessionManager::buildRecordForSYSSTAT,
    gSYSSTATColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

static UInt callbackForConvertTimeWaited(void *aBaseObj,
                                          void *aMember,
                                          UChar *aBuf,
                                          UInt /*aBufSize*/)
{   
    idvSessWaitFT    *sBase = (idvSessWaitFT*)aBaseObj;
    UInt              sOffset   = (UInt)((vULong)aMember - (vULong)aBaseObj);

    switch (sOffset)
    {
        case offsetof(idvSessWaitFT, mWaitEvent) +
             offsetof(idvWaitEvent, mTimeWaitedMicro):
            {
                *((ULong *)aBuf) = 
                    (sBase->mWaitEvent.mTimeWaitedMicro/1000);
            }
            break;
        default:
            IDE_CALLBACK_FATAL("not support type");
            break;
    }

    return ID_SIZEOF(ULong);
}

static UInt callbackForConvertAverageWait(void *aBaseObj,
                                          void *aMember,
                                          UChar *aBuf,
                                          UInt /*aBufSize*/)
{   
    idvSessWaitFT    *sBase = (idvSessWaitFT*)aBaseObj;
    UInt              sOffset   = (UInt)((vULong)aMember - (vULong)aBaseObj);

    switch (sOffset)
    {
        case offsetof(idvSessWaitFT, mWaitEvent) +
             offsetof(idvWaitEvent, mTimeWaitedMicro):
            {
                if ( sBase->mWaitEvent.mTotalWaits > 0 )
                {
                    *((ULong *)aBuf) = 
                           (sBase->mWaitEvent.mTimeWaitedMicro/1000)/
                           (sBase->mWaitEvent.mTotalWaits);
                }
                else
                {
                    *((ULong *)aBuf) = 0;
                }
            }
            break;
        default:
            IDE_CALLBACK_FATAL("not support type");
            break;
    }

    return ID_SIZEOF(ULong);
}


/*
 * Fixed Table Definition for SESSION_EVENT
 */

static iduFixedTableColDesc gSessionEventColDesc[] =
{
    // task
    {
        (SChar *)"SID",
        offsetof(idvSessWaitFT, mSID),
        IDU_FT_SIZEOF_UINTEGER,
        IDU_FT_TYPE_UINTEGER | IDU_FT_COLUMN_INDEX,
        NULL,
        0, 0, NULL // for internal use
    },
    // task
    {
        (SChar *)"EVENT_ID",
        offsetof(idvSessWaitFT, mWaitEvent) +
        offsetof(idvWaitEvent, mEventID),
        IDU_FT_SIZEOF_UINTEGER,
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"TOTAL_WAITS",
        offsetof(idvSessWaitFT, mWaitEvent) +
        offsetof(idvWaitEvent, mTotalWaits),
        IDU_FT_SIZEOF_BIGINT,
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"TOTAL_TIMEOUTS",
        offsetof(idvSessWaitFT, mWaitEvent) +
        offsetof(idvWaitEvent, mTotalTimeOuts),
        IDU_FT_SIZEOF_BIGINT,
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"TIME_WAITED",
        offsetof(idvSessWaitFT, mWaitEvent) +
        offsetof(idvWaitEvent, mTimeWaitedMicro),
        IDU_FT_SIZEOF_BIGINT,
        IDU_FT_TYPE_UBIGINT,
        callbackForConvertTimeWaited,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"AVERAGE_WAIT",
        offsetof(idvSessWaitFT, mWaitEvent) +
        offsetof(idvWaitEvent, mTimeWaitedMicro),
        IDU_FT_SIZEOF_BIGINT,
        IDU_FT_TYPE_UBIGINT,
        callbackForConvertAverageWait,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"MAX_WAIT",
        offsetof(idvSessWaitFT, mWaitEvent) +
        offsetof(idvWaitEvent, mMaxWait),
        IDU_FT_SIZEOF_BIGINT,
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"TIME_WAITED_MICRO",
        offsetof(idvSessWaitFT, mWaitEvent) +
        offsetof(idvWaitEvent, mTimeWaitedMicro),
        IDU_FT_SIZEOF_BIGINT,
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0, NULL // for internal use
    }
};

IDE_RC mmtSessionManager::buildRecordForSessionEvent(
    idvSQL              * /*aStatistics*/,
    void *aHeader,
    void * /* aDumpObj */,
    iduFixedTableMemory *aMemory)
{
    idvSession    *sSessStat;
    mmcTask       *sTask;
    iduListNode   *sIterator;
    idvSessWaitFT  sSessWaitFT;
    UInt           i;
    void         * sIndexValues[1];

    lockRead();

    IDU_LIST_ITERATE(&mTaskList, sIterator)
    {
        sTask = (mmcTask *)sIterator->mObj;

        if (sTask->getSession() != NULL)
        {
            sSessWaitFT.mSID = sTask->getSession()->getSessionID();

            /* BUG-43006 FixedTable Indexing Filter
             * Indexing Filter를 사용해서 전체 Record를 생성하지않고
             * 부분만 생성해 Filtering 한다.
             * 1. void * 배열에 IDU_FT_COLUMN_INDEX 로 지정된 컬럼에
             * 해당하는 값을 순서대로 넣어주어야 한다.
             * 2. IDU_FT_COLUMN_INDEX의 컬럼에 해당하는 값을 모두 넣
             * 어 주어야한다.
             */
            sIndexValues[0] = &sSessWaitFT.mSID;
            if ( iduFixedTable::checkKeyRange( aMemory,
                                               gSessionEventColDesc,
                                               sIndexValues )
                 == ID_FALSE )
            {
                continue;
            }
            else
            {
                /* Nothing to do */
            }
            for (i = IDV_WAIT_INDEX_BEGIN; i < IDV_WAIT_INDEX_MAX; i++)
            {
                sSessStat  = sTask->getSession()->getStatistics();
                
                sSessWaitFT.mWaitEvent = sSessStat->mWaitEvent[i];

                IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                                    aMemory,
                                                    (void *)&sSessWaitFT)
                         != IDE_SUCCESS);
            }
        }
    }

    unlock();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    unlock();

    return IDE_FAILURE;
}

iduFixedTableDesc gSessionEventTableDesc =
{
    (SChar *)"X$SESSION_EVENT",
    mmtSessionManager::buildRecordForSessionEvent,
    gSessionEventColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};


/*
 * Fixed Table Definition for SYSTEM_EVENT
 */

static iduFixedTableColDesc gSystemEventColDesc[] =
{
    // task
    {
        (SChar *)"EVENT_ID",
        offsetof(idvSessWaitFT, mWaitEvent) + offsetof(idvWaitEvent, mEventID),
        IDU_FT_SIZEOF_UINTEGER,
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"TOTAL_WAITS",
        offsetof(idvSessWaitFT, mWaitEvent) +
        offsetof(idvWaitEvent, mTotalWaits),
        IDU_FT_SIZEOF_BIGINT,
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"TOTAL_TIMEOUTS",
        offsetof(idvSessWaitFT, mWaitEvent) +
        offsetof(idvWaitEvent, mTotalTimeOuts),
        IDU_FT_SIZEOF_BIGINT,
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"TIME_WAITED",
        offsetof(idvSessWaitFT, mWaitEvent) +
        offsetof(idvWaitEvent, mTimeWaitedMicro),
        IDU_FT_SIZEOF_BIGINT,
        IDU_FT_TYPE_UBIGINT,
        callbackForConvertTimeWaited,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"AVERAGE_WAIT",
        offsetof(idvSessWaitFT, mWaitEvent) +
        offsetof(idvWaitEvent, mTimeWaitedMicro),
        IDU_FT_SIZEOF_BIGINT,
        IDU_FT_TYPE_UBIGINT,
        callbackForConvertAverageWait,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"TIME_WAITED_MICRO",
        offsetof(idvSessWaitFT, mWaitEvent) +
        offsetof(idvWaitEvent, mTimeWaitedMicro),
        IDU_FT_SIZEOF_BIGINT,
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"MAX_WAIT",
        offsetof(idvSessWaitFT, mWaitEvent) +
        offsetof(idvWaitEvent, mMaxWait),
        IDU_FT_SIZEOF_BIGINT,
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0, NULL // for internal use
    }
};


IDE_RC mmtSessionManager::buildRecordForSystemEvent(idvSQL              * /*aStatistics*/,
                                                    void *aHeader,
                                                    void * /* aDumpObj */,
                                                    iduFixedTableMemory *aMemory)
{
    idvSessWaitFT sSysWaitFT;
    UInt        i;

    for (i = IDV_WAIT_INDEX_BEGIN; i < IDV_WAIT_INDEX_MAX; i++)
    {
        sSysWaitFT.mWaitEvent = gSystemInfo.mWaitEvent[i];

        IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                            aMemory,
                                            (void *)&sSysWaitFT)
                 != IDE_SUCCESS);
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

iduFixedTableDesc gSystemEventTableDesc =
{
    (SChar *)"X$SYSTEM_EVENT",
    mmtSessionManager::buildRecordForSystemEvent,
    gSystemEventColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

/*
 * Fixed Table Definition for SYSTEM_CONFLICT_PAGE
 */

static iduFixedTableColDesc gSysConflictPageColDesc[] =
{
    // task
    {
        (SChar *)"PAGE_TYPE",
        offsetof(idvSysConflictPageFT, mPageType),
        IDU_FT_SIZEOF_UINTEGER,
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"LATCH_MISS_CNT",
        offsetof(idvSysConflictPageFT, mMissCnt),
        IDU_FT_SIZEOF_BIGINT,
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"LATCH_MISS_TIME",
        offsetof(idvSysConflictPageFT, mMissTime),
        IDU_FT_SIZEOF_BIGINT,
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0, NULL // for internal use
    }
};


IDE_RC mmtSessionManager::buildRecordForSysConflictPage(
    idvSQL              * /*aStatistics*/,
    void *aHeader,
    void * /* aDumpObj */,
    iduFixedTableMemory *aMemory)
{
    UInt                   i;
    idvSysConflictPageFT   sRow;

    for (i = 0; i < IDV_SM_PAGE_TYPE_MAX; i++)
    {
        sRow.mPageType = i;
        sRow.mMissCnt  = gSystemInfo.mMissCnt[i];
        sRow.mMissTime = gSystemInfo.mMissTime[i];

        IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                            aMemory,
                                            (void *)&sRow)
                 != IDE_SUCCESS);
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}


iduFixedTableDesc gSysConflictPageTableDesc =
{
    (SChar *)"X$SYSTEM_CONFLICT_PAGE",
    mmtSessionManager::buildRecordForSysConflictPage,
    gSysConflictPageColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

/* PROJ-2177 User Interface - Cancel */

/**
 * SessionID에 해당하는 Session을 찾는다.
 *
 * @param[out] aSession     session
 * @param[in]  aSessionID   session id
 *
 * @return Session을 찾았으면 IDE_SUCCESS, 아니면 IDE_FAILURE
 */
IDE_RC mmtSessionManager::findSession(mmcSession **aSession, mmcSessID aSessionID)
{
    iduListNode *sIterator;
    mmcTask     *sTask;
    mmcSession  *sSession;

    /* 이런일은 일어나선 안된다. */
    IDE_ASSERT(aSession != NULL);

    *aSession = NULL;

    lock();

    IDU_LIST_ITERATE(&mTaskList, sIterator)
    {
        sTask = (mmcTask *) sIterator->mObj;
        sSession = sTask->getSession();

        if ((sSession != NULL)
         && (sSession->getSessionID() == aSessionID))
        {
            *aSession = sSession;
            break;
        }
    }

    IDE_TEST(*aSession == NULL);

    unlock();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        IDE_SET( ideSetErrorCode(mmERR_ABORT_INVALID_SESSION_ID, aSessionID) );
    }

    unlock();

    return IDE_FAILURE;
}

idBool mmtSessionManager::existSessionByXID( ID_XID *aXID )
{
    iduListNode *sIterator;
    mmcTask     *sTask;
    mmcSession  *sSession;
    idBool       sFound = ID_FALSE;

    lock();

    IDU_LIST_ITERATE(&mTaskList, sIterator)
    {
        sTask = (mmcTask *) sIterator->mObj;
        sSession = sTask->getSession();

        if ((sSession != NULL)
            && (sSession->getTransPrepared() == ID_TRUE))
        {
            if (mmdXid::compFunc(sSession->getTransPreparedXID(), aXID) == 0)
            {
                sFound = ID_TRUE;
                break;
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            /* Nothing to do */
        }
    }

    unlock();

    return sFound;
}

idBool mmtSessionManager::isSysdba(void)
{
    PDL_thread_t      sCurrent;
    mmtServiceThread *sThread;
    mmcSession       *sSession;
    mmcTask          *sTask;
    iduListNode      *sIterator;

    sCurrent = idtContainer::getThreadContainer()->getTid();

    IDU_LIST_ITERATE(&mTaskList, sIterator)
    {
        sTask    = (mmcTask *)sIterator->mObj;
        sSession = sTask->getSession();
        sThread  = sTask->getThread();

        if((sSession != NULL) && (sThread != NULL))
        {
            if(sThread->getTid() == sCurrent)
            {
                return sSession->getUserInfo()->mIsSysdba;
            }
            else
            {
                /* continue */
            }
        }
        else
        {
            /* continue */
        }
    }

    return ID_FALSE;
}

/* PROJ-2451 Concurrent Execute Package */
static iduFixedTableColDesc gINTERNALSESSIONColDesc[] =
{
    {
        (SChar *)"ID",
        offsetof(mmcInternalSessionInfo4PerfV, mSessionID),
        IDU_FT_SIZEOF(mmcInternalSessionInfo4PerfV, mSessionID),
        IDU_FT_TYPE_UBIGINT | IDU_FT_COLUMN_INDEX,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"TRANS_ID",
        offsetof(mmcInternalSessionInfo4PerfV, mTransID),
        IDU_FT_SIZEOF(mmcInternalSessionInfo4PerfV, mTransID),
        //fix BUG-24289 V$SESSION의 Trans_id를 BIGINT으로 display.
        IDU_FT_TYPE_UBIGINT | IDU_FT_COLUMN_INDEX,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"QUERY_TIME_LIMIT",
        offsetof(mmcInternalSessionInfo4PerfV, mQueryTimeout),
        IDU_FT_SIZEOF(mmcInternalSessionInfo4PerfV, mQueryTimeout),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"DDL_TIME_LIMIT",
        offsetof(mmcInternalSessionInfo4PerfV, mDdlTimeout),
        IDU_FT_SIZEOF(mmcInternalSessionInfo4PerfV, mDdlTimeout),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"FETCH_TIME_LIMIT",
        offsetof(mmcInternalSessionInfo4PerfV, mFetchTimeout),
        IDU_FT_SIZEOF(mmcInternalSessionInfo4PerfV, mFetchTimeout),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"UTRANS_TIME_LIMIT",
        offsetof(mmcInternalSessionInfo4PerfV, mUTransTimeout),
        IDU_FT_SIZEOF(mmcInternalSessionInfo4PerfV, mUTransTimeout),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"IDLE_TIME_LIMIT",
        offsetof(mmcInternalSessionInfo4PerfV, mIdleTimeout),
        IDU_FT_SIZEOF(mmcInternalSessionInfo4PerfV, mIdleTimeout),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"IDLE_START_TIME",
        offsetof(mmcInternalSessionInfo4PerfV, mIdleStartTime),
        IDU_FT_SIZEOF(mmcInternalSessionInfo4PerfV, mIdleStartTime),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"ACTIVE_FLAG",
        offsetof(mmcInternalSessionInfo4PerfV, mActivated),
        IDU_FT_SIZEOF(mmcInternalSessionInfo4PerfV, mActivated),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"OPENED_STMT_COUNT",
        offsetof(mmcInternalSessionInfo4PerfV, mOpenStmtCount),
        IDU_FT_SIZEOF(mmcInternalSessionInfo4PerfV, mOpenStmtCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"CLIENT_NLS",
        offsetof(mmcInternalSessionInfo4PerfV, mNlsUse),
        IDU_FT_SIZEOF(mmcInternalSessionInfo4PerfV, mNlsUse) - 1,
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"DB_USERNAME",
        offsetof(mmcInternalSessionInfo4PerfV, mUserInfo) + offsetof(qciUserInfo, loginID),
        IDU_FT_SIZEOF(qciUserInfo, loginID) - 1,
        IDU_FT_TYPE_VARCHAR | IDU_FT_COLUMN_INDEX,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"DB_USERID",
        offsetof(mmcInternalSessionInfo4PerfV, mUserInfo) + offsetof(qciUserInfo, loginUserID),
        IDU_FT_SIZEOF(qciUserInfo, loginUserID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"DEFAULT_TBSID",
        offsetof(mmcInternalSessionInfo4PerfV, mUserInfo) + offsetof(qciUserInfo, tablespaceID),
        IDU_FT_SIZEOF(qciUserInfo, tablespaceID),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"DEFAULT_TEMP_TBSID",
        offsetof(mmcInternalSessionInfo4PerfV, mUserInfo) + offsetof(qciUserInfo, tempTablespaceID),
        IDU_FT_SIZEOF(qciUserInfo, tempTablespaceID),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"SYSDBA_FLAG",
        offsetof(mmcInternalSessionInfo4PerfV, mUserInfo) + offsetof(qciUserInfo, mIsSysdba),
        IDU_FT_SIZEOF(qciUserInfo, mIsSysdba),
        IDU_FT_TYPE_UINTEGER | IDU_FT_COLUMN_INDEX,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"AUTOCOMMIT_FLAG",
        offsetof(mmcInternalSessionInfo4PerfV, mCommitMode),
        IDU_FT_SIZEOF(mmcInternalSessionInfo4PerfV, mCommitMode),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"SESSION_STATE",
        offsetof(mmcInternalSessionInfo4PerfV, mSessionState),
        IDU_FT_SIZEOF(mmcInternalSessionInfo4PerfV, mSessionState),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"ISOLATION_LEVEL",
        offsetof(mmcInternalSessionInfo4PerfV, mIsolationLevel),
        IDU_FT_SIZEOF(mmcInternalSessionInfo4PerfV, mIsolationLevel),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"REPLICATION_MODE",
        offsetof(mmcInternalSessionInfo4PerfV, mReplicationMode),
        IDU_FT_SIZEOF(mmcInternalSessionInfo4PerfV, mReplicationMode),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"TRANSACTION_MODE",
        offsetof(mmcInternalSessionInfo4PerfV, mTransactionMode),
        IDU_FT_SIZEOF(mmcInternalSessionInfo4PerfV, mTransactionMode),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"COMMIT_WRITE_WAIT_MODE",
        offsetof(mmcInternalSessionInfo4PerfV, mCommitWriteWaitMode),
        IDU_FT_SIZEOF(mmcInternalSessionInfo4PerfV, mCommitWriteWaitMode),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"TRX_UPDATE_MAX_LOGSIZE",
        offsetof(mmcInternalSessionInfo4PerfV, mUpdateMaxLogSize),
        IDU_FT_SIZEOF(mmcInternalSessionInfo4PerfV, mUpdateMaxLogSize),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"OPTIMIZER_MODE",
        offsetof(mmcInternalSessionInfo4PerfV, mOptimizerMode),
        IDU_FT_SIZEOF(mmcInternalSessionInfo4PerfV, mOptimizerMode),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"HEADER_DISPLAY_MODE",
        offsetof(mmcInternalSessionInfo4PerfV, mHeaderDisplayMode),
        IDU_FT_SIZEOF(mmcInternalSessionInfo4PerfV, mHeaderDisplayMode),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"CURRENT_STMT_ID",
        offsetof(mmcInternalSessionInfo4PerfV, mCurrStmtID),
        IDU_FT_SIZEOF(mmcInternalSessionInfo4PerfV, mCurrStmtID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"STACK_SIZE",
        offsetof(mmcInternalSessionInfo4PerfV, mStackSize),
        IDU_FT_SIZEOF(mmcInternalSessionInfo4PerfV, mStackSize),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"DEFAULT_DATE_FORMAT",
        offsetof(mmcInternalSessionInfo4PerfV, mDateFormat),
        IDU_FT_SIZEOF(mmcInternalSessionInfo4PerfV, mDateFormat) - 1,
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"PARALLEL_DML_MODE",
        offsetof(mmcInternalSessionInfo4PerfV, mParallelDmlMode),
        IDU_FT_SIZEOF(mmcInternalSessionInfo4PerfV, mParallelDmlMode),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {   // To Fix BUG-18721
        (SChar *)"LOGIN_TIME",
        offsetof(mmcInternalSessionInfo4PerfV, mConnectTime),
        IDU_FT_SIZEOF(mmcInternalSessionInfo4PerfV, mConnectTime),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"NLS_NCHAR_CONV_EXCP",
        offsetof(mmcInternalSessionInfo4PerfV, mNlsNcharConvExcp),
        IDU_FT_SIZEOF(mmcInternalSessionInfo4PerfV, mNlsNcharConvExcp),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"NLS_NCHAR_LITERAL_REPLACE",
        offsetof(mmcInternalSessionInfo4PerfV, mNlsNcharLiteralReplace),
        IDU_FT_SIZEOF(mmcInternalSessionInfo4PerfV, mNlsNcharLiteralReplace),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"AUTO_REMOTE_EXEC",
        offsetof(mmcInternalSessionInfo4PerfV, mAutoRemoteExec),
        IDU_FT_SIZEOF(mmcInternalSessionInfo4PerfV, mAutoRemoteExec),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"FAILOVER_SOURCE",
        offsetof(mmcInternalSessionInfo4PerfV, mFailOverSource),
        IDU_FT_SIZEOF(mmcInternalSessionInfo4PerfV, mFailOverSource) - 1,
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"NLS_TERRITORY",
        offsetof(mmcInternalSessionInfo4PerfV, mNlsTerritory),
        IDU_FT_SIZEOF(mmcInternalSessionInfo4PerfV, mNlsTerritory) - 1,
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"NLS_ISO_CURRENCY",
        offsetof(mmcInternalSessionInfo4PerfV, mNlsISOCurrency),
        IDU_FT_SIZEOF(mmcInternalSessionInfo4PerfV, mNlsISOCurrency) - 1,
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"NLS_CURRENCY",
        offsetof(mmcInternalSessionInfo4PerfV, mNlsCurrency),
        IDU_FT_SIZEOF(mmcInternalSessionInfo4PerfV, mNlsCurrency) - 1,
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"NLS_NUMERIC_CHARACTERS",
        offsetof(mmcInternalSessionInfo4PerfV, mNlsNumChar),
        IDU_FT_SIZEOF(mmcInternalSessionInfo4PerfV, mNlsNumChar) - 1,
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"TIME_ZONE",
        offsetof(mmcInternalSessionInfo4PerfV, mTimezoneString),
        IDU_FT_SIZEOF(mmcInternalSessionInfo4PerfV, mTimezoneString) - 1,
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL // for internal use
    },
    /* PROJ-2047 Strengthening LOB - LOBCACHE */
    {
        (SChar *)"LOB_CACHE_THRESHOLD",
        offsetof(mmcInternalSessionInfo4PerfV, mLobCacheThreshold),
        IDU_FT_SIZEOF(mmcInternalSessionInfo4PerfV, mLobCacheThreshold),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"QUERY_REWRITE_ENABLE",
        offsetof(mmcInternalSessionInfo4PerfV, mQueryRewriteEnable),
        IDU_FT_SIZEOF(mmcInternalSessionInfo4PerfV, mQueryRewriteEnable),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0, NULL // for internal use
    }
};

/* PROJ-2451 Concurrent Execute Package */
IDE_RC mmtSessionManager::buildRecordForINTERNALSESSION(idvSQL * /*aStatistics*/,
                                                        void *aHeader,
                                                        void * /* aDumpObj */,
                                                        iduFixedTableMemory *aMemory)
{
    iduListNode    *sIterator;
    mmcSession     *sSess;
    mmcSessionInfo *sSession;
    mmcInternalSessionInfo4PerfV sSessionInfo;
    void           * sIndexValues[4];

    lockRead();

    IDU_LIST_ITERATE(&mInternalSessionList, sIterator)
    {
        sSess = (mmcSession *)sIterator->mObj;

        if ( sSess != NULL )
        {
            sSession = sSess->getInfo();

            //fix BUG-23656 session,xid ,transaction을 연계한 performance view를 제공하고,
            //그들간의 관계를 정확히 유지해야 함.
            // transaction을 사용하는 session의 변경.
            /* BUG-44967 */
            sSessionInfo.mTransID                = sSession->mTransID;
            sSessionInfo.mSessionID              = sSession->mSessionID;

            sSessionInfo.mQueryTimeout           = sSession->mQueryTimeout;
            /* BUG-32885 Timeout for DDL must be distinct to query_timeout or utrans_timeout */
            sSessionInfo.mDdlTimeout             = sSession->mDdlTimeout;
            sSessionInfo.mFetchTimeout           = sSession->mFetchTimeout;
            sSessionInfo.mUTransTimeout          = sSession->mUTransTimeout;
            sSessionInfo.mIdleTimeout            = sSession->mIdleTimeout;
            sSessionInfo.mIdleStartTime          = sSession->mIdleStartTime;
            sSessionInfo.mActivated              = sSession->mActivated;
            sSessionInfo.mOpenStmtCount          = sSession->mOpenStmtCount;

            /* BUG-43006 FixedTable Indexing Filter
             * Indexing Filter를 사용해서 전체 Record를 생성하지않고
             * 부분만 생성해 Filtering 한다.
             * 1. void * 배열에 IDU_FT_COLUMN_INDEX 로 지정된 컬럼에
             * 해당하는 값을 순서대로 넣어주어야 한다.
             * 2. IDU_FT_COLUMN_INDEX의 컬럼에 해당하는 값을 모두 넣
             * 어 주어야한다.
             */
            sIndexValues[0] = &sSessionInfo.mSessionID;
            sIndexValues[1] = &sSessionInfo.mTransID;
            sIndexValues[2] = &sSession->mUserInfo.loginID;
            sIndexValues[3] = &sSession->mUserInfo.mIsSysdba;
            if ( iduFixedTable::checkKeyRange( aMemory,
                                               gINTERNALSESSIONColDesc,
                                               sIndexValues )
                 == ID_FALSE )
            {
                continue; /* IDU_LIST_ITERATE(&mInternalSessionList, sIterator) */
            }
            else
            {
                /* Nothing to do */
            }
            idlOS::memcpy( sSessionInfo.mNlsUse,
                           sSession->mNlsUse,
                           ID_SIZEOF(SChar) * (QC_MAX_NAME_LEN + 1) );

            sSessionInfo.mUserInfo               = sSession->mUserInfo;
            sSessionInfo.mCommitMode             = sSession->mCommitMode;
            sSessionInfo.mSessionState           = sSession->mSessionState;
            sSessionInfo.mIsolationLevel         = sSession->mIsolationLevel;
            sSessionInfo.mReplicationMode        = sSession->mReplicationMode;
            sSessionInfo.mTransactionMode        = sSession->mTransactionMode;
            sSessionInfo.mCommitWriteWaitMode    = sSession->mCommitWriteWaitMode;
            sSessionInfo.mUpdateMaxLogSize       = sSession->mUpdateMaxLogSize;
            sSessionInfo.mOptimizerMode          = sSession->mOptimizerMode;
            sSessionInfo.mHeaderDisplayMode      = sSession->mHeaderDisplayMode;
            sSessionInfo.mCurrStmtID             = sSession->mCurrStmtID;
            sSessionInfo.mStackSize              = sSession->mStackSize;
            idlOS::memcpy( sSessionInfo.mDateFormat,
                           sSession->mDateFormat,
                           ID_SIZEOF(SChar) * (MMC_DATEFORMAT_MAX_LEN + 1) );
            sSessionInfo.mParallelDmlMode        = sSession->mParallelDmlMode;
            sSessionInfo.mConnectTime            = sSession->mConnectTime;
            sSessionInfo.mNlsNcharConvExcp       = sSession->mNlsNcharConvExcp;
            sSessionInfo.mNlsNcharLiteralReplace = sSession->mNlsNcharLiteralReplace;
            sSessionInfo.mAutoRemoteExec         = sSession->mAutoRemoteExec;

            /* BUG-31390 Failover info for v$session */
            idlOS::memcpy( sSessionInfo.mFailOverSource,
                           sSession->mFailOverSource,
                           IDL_IP_ADDR_MAX_LEN );

            /* PROJ-2208 Multi Currency */
            IDE_TEST( mtlTerritory::setNlsTerritoryName( sSession->mNlsTerritory,
                                                         sSessionInfo.mNlsTerritory)
                      != IDE_SUCCESS );
            IDE_TEST( mtlTerritory::setNlsISOCurrencyName( sSession->mNlsISOCurrency,
                                                           sSessionInfo.mNlsISOCurrency)
                      != IDE_SUCCESS );
            idlOS::memcpy( sSessionInfo.mNlsCurrency,
                           sSession->mNlsCurrency,
                           MTL_TERRITORY_CURRENCY_LEN );
            sSessionInfo.mNlsCurrency[MTL_TERRITORY_CURRENCY_LEN] = 0;

            idlOS::memcpy( sSessionInfo.mNlsNumChar,
                           sSession->mNlsNumChar,
                           MTL_TERRITORY_NUMERIC_CHAR_LEN );
            sSessionInfo.mNlsNumChar[MTL_TERRITORY_NUMERIC_CHAR_LEN] = 0;

            /* PROJ-2209 DBTIMEZONE */
            idlOS::memcpy ( sSessionInfo.mTimezoneString,
                            sSession->mTimezoneString,
                            MMC_TIMEZONE_MAX_LEN + 1 );

            /* PROJ-2047 Strengthening LOB - LOBCACHE */
            sSessionInfo.mLobCacheThreshold      = sSession->mLobCacheThreshold;

            /* PROJ-1090 Function-based Index */
            sSessionInfo.mQueryRewriteEnable = sSession->mQueryRewriteEnable;
            
            IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                                aMemory,
                                                (void *)&sSessionInfo) != IDE_SUCCESS);
        }
        else
        {
            // Nothing to do.
        }
    }

    unlock();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    unlock();

    return IDE_FAILURE;
}

/* PROJ-2451 Concurrent Execute Package */
iduFixedTableDesc gINTERNALSESSIONTableDesc =
{
    (SChar *)"X$INTERNAL_SESSION",
    mmtSessionManager::buildRecordForINTERNALSESSION,
    gINTERNALSESSIONColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

/* PROJ-2451 Concurrent Execute Package */
IDE_RC mmtSessionManager::allocInternalSession( mmcSession  ** aSession,
                                                qciUserInfo  * aUserInfo )
{
    UShort        sState    = 0;
    cmiLink     * sLink = NULL;
    mmcTask     * sTask = NULL;
    mmcSession  * sSession  = NULL;
    iduListNode * sListNode = NULL;
    iduListNode * sISListNode = NULL; // sInternalSessionListNode
    idBool        sIsLocked = ID_FALSE;
    mmcTransObj * sTrans = NULL;
    idBool        sIsDummyBegin = ID_FALSE;

    /* internal link를 생성한다. */
    IDE_TEST( cmiAllocLink( (cmnLink **)&sLink,
                             CMI_LINK_TYPE_PEER_SERVER,
                             CMI_LINK_IMPL_DUMMY )
              != IDE_SUCCESS );
    sState = 1;

    /* internal task를 생성한다. */
    IDE_TEST( allocInternalTask( &sTask, sLink ) != IDE_SUCCESS );
    sState = 2;
    
    IDU_FIT_POINT( "mmtSessionManager::allocInternalSession::alloc::Session" );

    IDE_TEST(mSessionPool.alloc((void **)&sSession) != IDE_SUCCESS);
    sState = 3;

    IDU_FIT_POINT( "mmtSessionManager::allocInternalSession::alloc::ListNode" );

    IDE_TEST(mListNodePool.alloc((void**)&sISListNode) != IDE_SUCCESS);
    sState = 4;

    /* lock sequence : lock() ->  mMutexSessionIDList.lock(NULL) */
    lock();
    IDE_ASSERT( mMutexSessionIDList.lock(NULL) == IDE_SUCCESS);
    sIsLocked = ID_TRUE;

    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    if ( IDU_LIST_IS_EMPTY(&mFreeSessionIDList) )
    {
        IDE_ASSERT( mMutexSessionIDList.unlock() == IDE_SUCCESS);
        IDU_FIT_POINT("mmtSessionManager::allocInternalSession::lock::InitializeByNewSession");

        /*mFreeSessionIDLIST is empty*/
        IDE_TEST(sSession->initialize(sTask, mNextSessionID + 1) != IDE_SUCCESS);
        mNextSessionID++;
    }
    else
    {
        /*Get a first node from mFreeSessioinIDLIST*/
        sListNode = IDU_LIST_GET_FIRST(&mFreeSessionIDList);
        IDE_ASSERT( mMutexSessionIDList.unlock() == IDE_SUCCESS);
        
        IDU_FIT_POINT("mmtSessionManager::allocInternalSession::lock::InitializeByFreeSession");
        IDE_TEST(sSession->initialize( sTask, *((UInt*)(sListNode->mObj)) ) != IDE_SUCCESS);
        /*Remove the first node from mFreeSessioinIDLIST*/
        IDU_LIST_REMOVE(sListNode);
    }

    sTask->setSession(sSession);

    IDU_LIST_INIT_OBJ(sISListNode, sSession);
    IDU_LIST_ADD_LAST(&mInternalSessionList, sISListNode);
    sState = 5;

    sIsLocked = ID_FALSE;
    unlock();

    /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
    if ( sListNode != NULL )
    {
        IDE_ASSERT( mSessionIDPool.memfree(sListNode->mObj) == IDE_SUCCESS );
        IDE_ASSERT( mListNodePool.memfree(sListNode) == IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    sSession->setUserInfo( aUserInfo );
    sSession->setSessionState( MMC_SESSION_STATE_SERVICE );
    ( void )sSession->setCommitMode( MMC_COMMITMODE_NONAUTOCOMMIT );

    /* BUG-42856 Job Scheduler 동작중 fatal
     * AUTO_COMMIT = 0 인 경우 setCommitMode에서 smiTrans를 할당하지 않아서
     * FATAL이 발생한다
     */
    if ( sSession->getTransPtr() == NULL )
    {
        // change a commit mode from autocommit to non auto commit.
        sTrans = sSession->allocTrans();
        mmcTrans::begin( sTrans,
                         sSession->getStatSQL(),
                         sSession->getSessionInfoFlagForTx(),
                         sSession,
                         &sIsDummyBegin );
    }
    else
    {
        /* Nothing to do */
    }

    *aSession = sSession;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sIsLocked == ID_TRUE )
    {
        unlock();
    }
    else
    {
        // Nothing to do.
    }

    switch ( sState )
    {
        case 5:
            lock();
            IDU_LIST_REMOVE(sISListNode);
            unlock();
            
            (void)sSession->finalize();
            /* fall through */
        case 4:
            if ( sISListNode != NULL )
            {
                (void)mListNodePool.memfree(sISListNode);
            }
            else
            {
                // Nothing to do.
            }
            /* fall through */
        case 3:
            if ( sSession != NULL )
            {
                (void)mSessionPool.memfree(sSession);
            }
            else
            {
                // Nothing to do.
            }
        case 2:
            if ( sTask != NULL )
            {
                (void)freeInternalTask( sTask );
            }
            else
            {
                // Nothing to do.
            }
        case 1:
            if ( sLink != NULL )
            {
                (void)cmiFreeLink( sLink );
            }
            else
            {
                // Nothing to do.
            }
        default:
            break;
    }

    return IDE_FAILURE;
}

/* PROJ-2451 Concurrent Execute Package */
IDE_RC mmtSessionManager::freeInternalSession( mmcSession * aSession )
{
    mmcTask     * sTask     = NULL;
    cmiLink     * sLink     = NULL;
    mmcSession  * sSession  = NULL;
    mmcSessID   * sSid      = NULL;
    iduListNode * sListNode = NULL;
    idBool        sIsFound  = ID_FALSE;

    sSession = aSession;

    /* BUG-43259 valgrind error */
    lock();
    if ( sSession != NULL )
    {
        sTask = aSession->getTask();  // internal task
        sTask->setSession(NULL);
    }
    else
    {
        /* Nothing to do */
    }
    unlock();

    if (sSession != NULL)
    {
        sLink = sTask->getLink();     // internal link
        
        IDU_FIT_POINT_RAISE( "mmtSessionManager::freeInternalSession::alloc::Sid",
                              InsufficientMemory );

        IDE_TEST_RAISE(mSessionIDPool.alloc((void **)&sSid) != IDE_SUCCESS,
                       InsufficientMemory);
        *sSid = sSession->getSessionID();

        lock();
        IDU_LIST_ITERATE(&mInternalSessionList, sListNode)
        {
            sSession = (mmcSession*)sListNode->mObj;
            if ( *sSid == sSession->getSessionID() )
            {
                IDU_LIST_REMOVE(sListNode);
                sIsFound = ID_TRUE;
                break;
            }
            else
            {
                // Nothing to do.
            }
        }
        unlock();

        IDE_ERROR( sIsFound == ID_TRUE );

        // 동일한 mempool을 사용하므로 sListNode를 재사용한다.
        // mInternalSessionList => mFreeSessionIDList
        IDU_LIST_INIT_OBJ(sListNode, sSid);

        // BUG-40502
        // If fail to finalize internal session, server might be shut down abnormally.
        IDU_FIT_POINT_RAISE("mmtSessionManager::freeInternalSession::sessionFree::Fail",
                            SessionFreeFail);

        IDE_TEST_RAISE(sSession->finalize() != IDE_SUCCESS, SessionFreeFail);

        IDE_TEST_RAISE(mSessionPool.memfree(sSession) != IDE_SUCCESS, SessionFreeFail);

        IDE_ASSERT( mMutexSessionIDList.lock(NULL) == IDE_SUCCESS);
        /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
        /*Insert the session ID value to last of mFreeSessioinIDLIST*/
        IDU_LIST_ADD_LAST(&mFreeSessionIDList, sListNode);
        IDE_ASSERT( mMutexSessionIDList.unlock() == IDE_SUCCESS);
        
        IDE_ASSERT( freeInternalTask( sTask ) == IDE_SUCCESS);
        IDE_ASSERT( cmiFreeLink( sLink ) == IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(SessionFreeFail);
    {
        IDE_ASSERT( mMutexSessionIDList.lock(NULL) == IDE_SUCCESS);
        /* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
        IDU_LIST_ADD_LAST(&mFreeSessionIDList, sListNode);
        IDE_ASSERT( mMutexSessionIDList.unlock() == IDE_SUCCESS);
        
        IDE_ASSERT( freeInternalTask( sTask ) == IDE_SUCCESS);
        IDE_ASSERT( cmiFreeLink( sLink ) == IDE_SUCCESS );
    }
    IDE_EXCEPTION(InsufficientMemory);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));

        if ( sSid != NULL )
        {
            ( void )mSessionIDPool.memfree(sSid);
        }
        else
        {
            /* Nothing to do */
        }
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// BUG-42464 dbms_alert package
IDE_RC mmtSessionManager::applySignal( mmcSession * aSession )
{
    mmcTask     * sTask     = NULL;
    iduListNode * sIterator = NULL;
    mmcSession  * sSession  = NULL;
    idBool        sLock = ID_FALSE;

    lock();
    sLock = ID_TRUE;

    IDU_LIST_ITERATE(&mTaskList, sIterator)
    {
        sTask = (mmcTask *)sIterator->mObj;

        sSession = sTask->getSession();

        if ( sSession != NULL)
        {
            IDE_TEST( sSession->getInfo()->mEvent.apply( aSession ) != IDE_SUCCESS ); 
        }
        else
        {
            /* do nothing */
        }
    } 

    sLock = ID_FALSE;
    unlock();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sLock == ID_TRUE )
    {
        unlock();
    }

    return IDE_FAILURE;
}

/**
 * PROJ-2626 Snapshot Export
 *
 * 현재 입력받은 aType의 ClientAppInfoType이 존재하는지 검사한다.
 * alter databse begin snapshot 구문 실행시 iLoader Session이
 * 있다면 실패시킨다.
 */
idBool mmtSessionManager::existClientAppInfoType( mmcClientAppInfoType aType )
{
    mmcTask     * sTask     = NULL;
    iduListNode * sIterator = NULL;
    mmcSession  * sSession  = NULL;
    idBool        sIsTrue   = ID_FALSE;

    lock();

    IDU_LIST_ITERATE(&mTaskList, sIterator)
    {
        sTask = (mmcTask *)sIterator->mObj;

        sSession = sTask->getSession();

        if ( sSession != NULL )
        {
            if ( sSession->getClientAppInfoType() == aType )
            {
                sIsTrue = ID_TRUE;
                break;
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            /* Nothing to do */
        }
    }

    unlock();

    return sIsTrue;
}

/**
 * PROJ-2626 Snapshot Export
 *
 * 현재 접속된 세션중에  특정 ClientAppInfo Type의 세션을 끊는다.
 *
 * 1. alter database end snpashot 구문이 실행시 현제 접속된 iloader 세션을
 * 끊는다.
 * 2. snapshot thread에서 memory / disk undo threshold 가 넘어서 snapshot을
 * 종료해야할 경우 현재 접속된 모든 iloader Session을 끊는다.
 */
void mmtSessionManager::terminateAllClientAppInoType( mmcClientAppInfoType aType )
{
    mmcTask     * sTask     = NULL;
    iduListNode * sIterator = NULL;
    mmcSession  * sSession  = NULL;

    lock();

    IDU_LIST_ITERATE(&mTaskList, sIterator)
    {
        sTask = (mmcTask *)sIterator->mObj;

        sSession = sTask->getSession();

        if ( sSession != NULL )
        {
            if ( sSession->getClientAppInfoType() == aType )
            {
                // terminate
                IDU_SESSION_SET_CLOSED(*sSession->getEventFlag());

                if ( cmiShutdownLink(sTask->getLink(), CMI_DIRECTION_RD )
                     != IDE_SUCCESS )
                {
                    IDE_WARNING(IDE_SERVER_0, "shutdown() FAILURE in termiate()");
                }
                else
                {
                    /* Nothing to do */
                }
                //PROJ-1677 DEQUEUE
                //fix BUG-24362 dequeue 대기 세션이 close될때 , session의 queue 설정에서
                // 동시성에 문제가 있음.
                wakeupEnqueWaitIfNessary(sTask, sSession);

                mInfo.mTerminatedCount++;

                IDV_SESS_ADD_DIRECT(sSession->getStatistics(),
                                    IDV_STAT_INDEX_SESSION_TERMINATED_COUNT,
                                    1);
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            /* Nothing to do */
        }
    }

    unlock();
}

/* BUG-48515 */
iduList* mmtSessionManager::getTaskList()
{
    return &mTaskList;
}
