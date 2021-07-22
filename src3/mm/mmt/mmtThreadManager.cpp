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

#include <idtCPUSet.h>
#include <mmErrorCode.h>
#include <mmcTask.h>
#include <mmtDispatcher.h>
#include <mmtServiceThread.h>
#include <mmtSessionManager.h>
#include <mmtThreadManager.h>
#include <mmuFixedTable.h>
#include <mmuProperty.h>
#include <mmuAccessList.h>
#include <cmnLink.h> /* used for dispatchCallback: sLinkPeer->mImpl */

#define UNIX_FILE_PATH_LEN 1024
#define IPC_FILE_PATH_LEN  1024

#define START_THREAD_WAIT_TIMEOUT 10
#define STOP_THREAD_WAIT_TIMEOUT  10

#define MMT_MESSAGE_SIZE 512

cmiLink          *mmtThreadManager::mListenLink[CMI_LINK_IMPL_MAX];
mmtDispatcher    *mmtThreadManager::mDispatcher[CMI_DISPATCHER_IMPL_MAX];

idBool            mmtThreadManager::mDispatcherRun = ID_FALSE;

iduMutex          mmtThreadManager::mMutex;

iduMemPool        mmtThreadManager::mServiceThreadPool;
UInt              mmtThreadManager::mServiceThreadLastID;

UInt              mmtThreadManager::mServiceThreadSocketCount;
iduList           mmtThreadManager::mServiceThreadSocketList;

UInt              mmtThreadManager::mServiceThreadIPCCount;
iduList           mmtThreadManager::mServiceThreadIPCList;

/*PROJ-2616*/
UInt              mmtThreadManager::mServiceThreadIPCDACount;
iduList           mmtThreadManager::mServiceThreadIPCDAList;

iduList           mmtThreadManager::mTempDedicateServiceThrList;
iduList           mmtThreadManager::mServiceThreadFreeList;

mmtServiceThread **mmtThreadManager::mIpcServiceThreadArray;

iduMutex         *mmtThreadManager::mIpcServiceThreadMutex;
iduCond          *mmtThreadManager::mIpcServiceThreadCV;

/*PROJ-2616*/
mmtServiceThread **mmtThreadManager::mIPCDAServiceThreadArray;
iduMutex         *mmtThreadManager::mIPCDAServiceThreadMutex;
iduCond          *mmtThreadManager::mIPCDAServiceThreadCV;

/* PROJ-2108 Dedicated thread mode which uses less CPU */
iduMutex          mmtThreadManager::mMutexForThreadManagerSignal;
iduCond           mmtThreadManager::mThreadManagerCV;
UInt              mmtThreadManager::mCoreCount = 0;
idtCPUSet         mmtThreadManager::mThreadManagerPset;
/* BUG-35356 Memory violation may occur when calculating CPU busy count */
mmtCoreInfo      *mmtThreadManager::mCoreInfoArray = NULL;

/* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
   BUG-29335A performance-view about service thread should be strengthened for problem tracking
*/
mmtServiceThreadMgrInfo mmtThreadManager::mServiceThreadMgrInfo;

IDE_RC mmtThreadManager::initialize()
{
    UChar sImpl;
    /* BUG-35356 Memory violation may occur when calculating CPU busy count */
    SInt  sI;
    UInt  sCore = 0;

    /*
     * ListenLink Array Intialize
     */

    for (sImpl = CMI_LINK_IMPL_BASE; sImpl < CMI_LINK_IMPL_MAX; sImpl++)
    {
        mListenLink[sImpl] = NULL;
    }

    /*
     * Dispatcher Array Initialize
     */

    for (sImpl = CMI_DISPATCHER_IMPL_BASE; sImpl < CMI_DISPATCHER_IMPL_MAX; sImpl++)
    {
        mDispatcher[sImpl] = NULL;
    }

    /*
     * Mutex Initialize
     */

    IDE_TEST(mMutex.initialize((SChar *)"MMT_THREAD_MANAGER_MUTEX",
                               IDU_MUTEX_KIND_POSIX,
                               IDV_WAIT_INDEX_NULL) != IDE_SUCCESS);

    /*
     * Service Thread Pool Initialize
     */

    IDE_TEST(mServiceThreadPool.initialize(IDU_MEM_MMT,
                                           (SChar *)"MMT_SERVICE_THREAD_POOL",
                                           1,
                                           ID_SIZEOF(mmtServiceThread),
                                           10,
                                           IDU_AUTOFREE_CHUNK_LIMIT,		/* ChunkLimit */
                                           ID_TRUE,							/* UseMutex */
                                           IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte */
                                           ID_FALSE,						/* ForcePooling */
                                           ID_TRUE,							/* GarbageCollection */
                                           ID_TRUE,                         /* HWCacheLine */
                                           IDU_MEMPOOL_TYPE_LEGACY          /* mempool type*/) 
             != IDE_SUCCESS);			


    /* PROJ-2108 Dedicated thread mode which uses less CPU */
    IDE_TEST(mMutexForThreadManagerSignal.initialize((SChar *)"MMT_THREAD_MANAGER_MUTEX_FOR_SIGNAL",
                                        IDU_MUTEX_KIND_POSIX,
                                        IDV_WAIT_INDEX_NULL) != IDE_SUCCESS);

    IDE_TEST(mThreadManagerCV.initialize() != IDE_SUCCESS);

    mThreadManagerPset.initialize(IDT_FILL);
    mCoreCount          = idtCPUSet::getAvailableCPUCount();

    /* BUG-35137 Memory for CPU busy count should not be allocated when pset is not supported */
    if ( mCoreCount != 0 )
    {
        IDU_FIT_POINT( "mmtThreadManager::initialize::calloc::CoreInfoArray" );

        /* BUG-35356 Memory violation may occur when calculating CPU busy count */
        IDE_TEST( iduMemMgr::calloc( IDU_MEM_MMT,
                                    mCoreCount,
                                    ID_SIZEOF( mmtCoreInfo ),
                                    (void **)&mCoreInfoArray,
                                    IDU_MEM_IMMEDIATE) != IDE_SUCCESS );

        sI = mThreadManagerPset.findFirstCPU();
        IDE_ASSERT(sI != IDT_EMPTY);
        while(sI != IDT_EMPTY)
        {
            mCoreInfoArray[sCore].mCoreNumber = (UInt)sI;
            sCore++;
            sI = mThreadManagerPset.findNextCPU(sI);
        }
    }
    else
    {
        /* do nothing */
    }

    /*
     * Service Thread List Initialize
     */

    mServiceThreadLastID      = 0;
    mServiceThreadSocketCount = 0;
    mServiceThreadIPCCount    = 0;
    /*PROJ-2616*/
    mServiceThreadIPCDACount  = 0;

    IDU_LIST_INIT(&mServiceThreadSocketList);
    IDU_LIST_INIT(&mServiceThreadIPCList);
    /*PROJ-2616*/
    IDU_LIST_INIT(&mServiceThreadIPCDAList);
    IDU_LIST_INIT(&mServiceThreadFreeList);
    IDU_LIST_INIT(&mTempDedicateServiceThrList);
    mIpcServiceThreadArray      = NULL;

    mIpcServiceThreadMutex = NULL;
    mIpcServiceThreadCV    = NULL;
    /*PROJ-2616*/
    mIPCDAServiceThreadArray = NULL;

    /*PROJ-2616*/
    mIPCDAServiceThreadMutex = NULL;
    mIPCDAServiceThreadCV    = NULL;

    /* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
       BUG-29335A performance-view about service thread should be strengthened for problem tracking
    */
    mServiceThreadMgrInfo.mAddThrCount = 0;
    mServiceThreadMgrInfo.mRemoveThrCount = 0;
    mServiceThreadMgrInfo.mReservedThrCnt = 0;


    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    ideLog::log(IDE_SERVER_0, "mmtThreadManager::initialize() failed");
    return IDE_FAILURE;
}

IDE_RC mmtThreadManager::finalize()
{
    SInt sImpl;
    UInt sWaitCount = 0;
    
    /*
     * Dispatcher Free
     */

    for (sImpl = CMI_DISPATCHER_IMPL_BASE; sImpl < CMI_DISPATCHER_IMPL_MAX; sImpl++)
    {
        IDE_TEST(freeDispatcher((cmiDispatcherImpl)sImpl) != IDE_SUCCESS);
    }

    /*
     * ListenLink Free
     */

    for (sImpl = CMI_LINK_IMPL_BASE; sImpl < CMI_LINK_IMPL_MAX; sImpl++)
    {
        if (mListenLink[sImpl] != NULL)
        {
            IDE_TEST(cmiFreeLink(mListenLink[sImpl]) != IDE_SUCCESS);
        }
    }

    /*
     * Service Thread Free
     */
    while( 1)
    {
        /* fix BUG-31476 An abnormal exit might happen during free service threads in shutdown process*/
        lock();
        if( (IDU_LIST_IS_EMPTY(&mTempDedicateServiceThrList)) &&
            (IDU_LIST_IS_EMPTY(&mServiceThreadSocketList)) &&
            (IDU_LIST_IS_EMPTY(&mServiceThreadIPCList)) &&
            (IDU_LIST_IS_EMPTY(&mServiceThreadIPCDAList)) )
        {
            unlock();
            break;
        }
        else
        {
            unlock();
            sWaitCount++;
            ideLog::log(IDE_SERVER_0,
                    "wait for threads to end [%u secs]",
                     sWaitCount);
            idlOS::sleep(1);
        }
    }//while
    
    freeServiceThreads(&mServiceThreadFreeList);
    IDE_TEST(mServiceThreadPool.destroy() != IDE_SUCCESS);

    IDE_TEST(mMutex.destroy() != IDE_SUCCESS);

    /* PROJ-2108 Dedicated thread mode which uses less CPU */
    IDE_TEST( mThreadManagerCV.initialize() != IDE_SUCCESS );
    IDE_TEST( mMutexForThreadManagerSignal.destroy() != IDE_SUCCESS );

    /* BUG-35356 Memory violation may occur when calculating CPU busy count */
    if ( mCoreInfoArray != NULL )
    {
        IDE_ASSERT( iduMemMgr::free( mCoreInfoArray ) == IDE_SUCCESS );
        mCoreInfoArray = NULL;
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC mmtThreadManager::setupProtocolCallback()
{
    UInt i;

    /* proj_2160 cm_type removal */
    /* callbacks for A5 */
    for (i = 0; i < CMP_OP_DB_MAX_A5; i++)
    {
        gCmpModuleDB.mCallbackFunctionA5[i] = mmtServiceThread::invalidProtocolA5;
    }
    gCmpModuleDB.mCallbackFunctionA5[CMP_OP_DB_ErrorInfo       ] = mmtServiceThread::errorInfoProtocolA5;
    gCmpModuleDB.mCallbackFunctionA5[CMP_OP_DB_Connect         ] = mmtServiceThread::connectProtocolA5;
    gCmpModuleDB.mCallbackFunctionA5[CMP_OP_DB_Disconnect      ] = mmtServiceThread::disconnectProtocolA5;
    gCmpModuleDB.mCallbackFunctionA5[CMP_OP_DB_PropertyGet     ] = mmtServiceThread::propertyGetProtocolA5;
    gCmpModuleDB.mCallbackFunctionA5[CMP_OP_DB_PropertySet     ] = mmtServiceThread::propertySetProtocolA5;
    gCmpModuleDB.mCallbackFunctionA5[CMP_OP_DB_Prepare         ] = mmtServiceThread::prepareProtocolA5;
    gCmpModuleDB.mCallbackFunctionA5[CMP_OP_DB_PlanGet         ] = mmtServiceThread::planGetProtocolA5;
    gCmpModuleDB.mCallbackFunctionA5[CMP_OP_DB_ColumnInfoGet   ] = mmtServiceThread::columnInfoGetProtocolA5;
    gCmpModuleDB.mCallbackFunctionA5[CMP_OP_DB_ColumnInfoSet   ] = mmtServiceThread::columnInfoSetProtocolA5;
    gCmpModuleDB.mCallbackFunctionA5[CMP_OP_DB_ParamInfoGet    ] = mmtServiceThread::paramInfoGetProtocolA5;
    gCmpModuleDB.mCallbackFunctionA5[CMP_OP_DB_ParamInfoSet    ] = mmtServiceThread::paramInfoSetProtocolA5;
    gCmpModuleDB.mCallbackFunctionA5[CMP_OP_DB_ParamInfoSetList] = mmtServiceThread::paramInfoSetListProtocolA5;
    gCmpModuleDB.mCallbackFunctionA5[CMP_OP_DB_ParamDataIn     ] = mmtServiceThread::paramDataInProtocolA5;
    gCmpModuleDB.mCallbackFunctionA5[CMP_OP_DB_ParamDataInList ] = mmtServiceThread::paramDataInListProtocolA5;
    gCmpModuleDB.mCallbackFunctionA5[CMP_OP_DB_Execute         ] = mmtServiceThread::executeProtocolA5;
    gCmpModuleDB.mCallbackFunctionA5[CMP_OP_DB_FetchMove       ] = mmtServiceThread::fetchMoveProtocolA5;
    gCmpModuleDB.mCallbackFunctionA5[CMP_OP_DB_Fetch           ] = mmtServiceThread::fetchProtocolA5;
    gCmpModuleDB.mCallbackFunctionA5[CMP_OP_DB_Free            ] = mmtServiceThread::freeProtocolA5;
    gCmpModuleDB.mCallbackFunctionA5[CMP_OP_DB_Transaction     ] = mmtServiceThread::transactionProtocolA5;
    gCmpModuleDB.mCallbackFunctionA5[CMP_OP_DB_LobGetSize      ] = mmtServiceThread::lobGetSizeProtocolA5;
    gCmpModuleDB.mCallbackFunctionA5[CMP_OP_DB_LobGet          ] = mmtServiceThread::lobGetProtocolA5;
    gCmpModuleDB.mCallbackFunctionA5[CMP_OP_DB_LobGetBytePosCharLen] = mmtServiceThread::lobGetBytePosCharLenProtocolA5;
    gCmpModuleDB.mCallbackFunctionA5[CMP_OP_DB_LobGetCharPosCharLen] = mmtServiceThread::lobGetCharPosCharLenProtocolA5;
    gCmpModuleDB.mCallbackFunctionA5[CMP_OP_DB_LobBytePos      ] = mmtServiceThread::lobBytePosProtocolA5;
    gCmpModuleDB.mCallbackFunctionA5[CMP_OP_DB_LobCharLength   ] = mmtServiceThread::lobCharLengthProtocolA5;
    gCmpModuleDB.mCallbackFunctionA5[CMP_OP_DB_LobPutBegin     ] = mmtServiceThread::lobPutBeginProtocolA5;
    gCmpModuleDB.mCallbackFunctionA5[CMP_OP_DB_LobPut          ] = mmtServiceThread::lobPutProtocolA5;
    gCmpModuleDB.mCallbackFunctionA5[CMP_OP_DB_LobPutEnd       ] = mmtServiceThread::lobPutEndProtocolA5;
    gCmpModuleDB.mCallbackFunctionA5[CMP_OP_DB_LobFree         ] = mmtServiceThread::lobFreeProtocolA5;
    gCmpModuleDB.mCallbackFunctionA5[CMP_OP_DB_LobFreeAll      ] = mmtServiceThread::lobFreeAllProtocolA5;
    gCmpModuleDB.mCallbackFunctionA5[CMP_OP_DB_XaOperation     ] = mmtServiceThread::xaOperationProtocolA5;
    gCmpModuleDB.mCallbackFunctionA5[CMP_OP_DB_XaTransaction   ] = mmtServiceThread::xaTransactionProtocolA5;
    
    /* callbacks for A7 or higher */
    for (i = 0; i < CMP_OP_DB_MAX; i++)
    {
        gCmpModuleDB.mCallbackFunction[i] = mmtServiceThread::invalidProtocol;
    }
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, Connect),
                            mmtServiceThread::connectProtocol) != IDE_SUCCESS);
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, Disconnect),
                            mmtServiceThread::disconnectProtocol) != IDE_SUCCESS);
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, PropertyGet),
                            mmtServiceThread::propertyGetProtocol) != IDE_SUCCESS);
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, PropertySet),
                            mmtServiceThread::propertySetProtocol) != IDE_SUCCESS);
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, Prepare),
                            mmtServiceThread::prepareProtocol) != IDE_SUCCESS);
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, PlanGet),
                            mmtServiceThread::planGetProtocol) != IDE_SUCCESS);
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, ColumnInfoGet),
                            mmtServiceThread::columnInfoGetProtocol) != IDE_SUCCESS);
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, ParamInfoGet),
                            mmtServiceThread::paramInfoGetProtocol) != IDE_SUCCESS);
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, ParamInfoSetList),
                            mmtServiceThread::paramInfoSetListProtocol) != IDE_SUCCESS);
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, ParamDataIn),
                            mmtServiceThread::paramDataInProtocol) != IDE_SUCCESS);
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, ParamDataInList),
                            mmtServiceThread::paramDataInListProtocol) != IDE_SUCCESS);
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, Execute),
                            mmtServiceThread::executeProtocol) != IDE_SUCCESS);
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, FetchMove),
                            mmtServiceThread::fetchMoveProtocol) != IDE_SUCCESS);
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, Fetch),
                            mmtServiceThread::fetchProtocol) != IDE_SUCCESS);
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, Free),
                            mmtServiceThread::freeProtocol) != IDE_SUCCESS);
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, Cancel),
                            mmtServiceThread::cancelProtocol) != IDE_SUCCESS);
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, Transaction),
                            mmtServiceThread::transactionProtocol) != IDE_SUCCESS);
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, LobGetSize),
                            mmtServiceThread::lobGetSizeProtocol) != IDE_SUCCESS);
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, LobGet),
                            mmtServiceThread::lobGetProtocol) != IDE_SUCCESS);
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, LobGetBytePosCharLen),
                            mmtServiceThread::lobGetBytePosCharLenProtocol) != IDE_SUCCESS);
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, LobGetCharPosCharLen),
                            mmtServiceThread::lobGetCharPosCharLenProtocol) != IDE_SUCCESS);
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, LobBytePos),
                            mmtServiceThread::lobBytePosProtocol) != IDE_SUCCESS);
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, LobCharLength),
                            mmtServiceThread::lobCharLengthProtocol) != IDE_SUCCESS);
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, LobPutBegin),
                            mmtServiceThread::lobPutBeginProtocol) != IDE_SUCCESS);
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, LobPut),
                            mmtServiceThread::lobPutProtocol) != IDE_SUCCESS);
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, LobPutEnd),
                            mmtServiceThread::lobPutEndProtocol) != IDE_SUCCESS);
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, LobFree),
                            mmtServiceThread::lobFreeProtocol) != IDE_SUCCESS);
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, LobFreeAll),
                            mmtServiceThread::lobFreeAllProtocol) != IDE_SUCCESS);
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, XaOperation),
                            mmtServiceThread::xaOperationProtocol) != IDE_SUCCESS);
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, XaTransaction),
                            mmtServiceThread::xaTransactionProtocol) != IDE_SUCCESS);
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, Handshake),
                            mmtServiceThread::handshakeProtocol) != IDE_SUCCESS);

    /* PROJ-2177 User Interface - Cancel */
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, PrepareByCID),
                            mmtServiceThread::prepareByCIDProtocol) != IDE_SUCCESS);
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, CancelByCID),
                            mmtServiceThread::cancelByCIDProtocol) != IDE_SUCCESS);

    /* PROJ-2047 Strengthening LOB - Added Interfaces */
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, LobTrim),
                            mmtServiceThread::lobTrimProtocol) != IDE_SUCCESS);

    /* BUG-38496 */
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, ConnectEx),
                            mmtServiceThread::connectProtocol) != IDE_SUCCESS);

    /* BUG-39463 Add new fetch protocol that can request over 65535 rows. */
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, FetchV2),
                            mmtServiceThread::fetchProtocol) != IDE_SUCCESS);

    /* BUG-41793 Keep a compatibility among tags */
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, PropertySetV2),
                            mmtServiceThread::propertySetProtocol) != IDE_SUCCESS);

    /* BUG-44572 */
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, ExecuteV2),
                            mmtServiceThread::executeProtocol) != IDE_SUCCESS);
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, ParamDataInListV2),
                            mmtServiceThread::paramDataInListProtocol) != IDE_SUCCESS);

    /* PROJ-2598 Shard */
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, ShardNodeGetList),
                            mmtServiceThread::shardNodeGetListProtocol) != IDE_SUCCESS);

    /* PROJ-2622 Shard Retry Execution */
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, ShardNodeUpdateList),
                            mmtServiceThread::shardNodeUpdateListProtocol) != IDE_SUCCESS);

    /* PROJ-2598 Shard pilot(shard analyze) */
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, ShardAnalyze),
                            mmtServiceThread::shardAnalyzeProtocol) != IDE_SUCCESS);

    /* PROJ-2658 altibase sharding */
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, ShardHandshake),
                            mmtServiceThread::shardHandshakeProtocol) != IDE_SUCCESS);

    /* BUG-45411 client-side global transaction */
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, ShardTransaction),
                            mmtServiceThread::shardTransactionProtocol) != IDE_SUCCESS);

    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, ShardPrepare),
                            mmtServiceThread::shardPrepareProtocol) != IDE_SUCCESS);

    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, ShardEndPendingTx),
                            mmtServiceThread::shardEndPendingTxProtocol) != IDE_SUCCESS);

    /* BUG-46785 Shard statement partial rollback */
    IDE_TEST( cmiSetCallback( CMI_PROTOCOL( DB, SetSavepoint ),
                              mmtServiceThread::setSavepointProtocol ) != IDE_SUCCESS );

    IDE_TEST( cmiSetCallback( CMI_PROTOCOL( DB, RollbackToSavepoint ),
                              mmtServiceThread::rollbackToSavepointProtocol ) != IDE_SUCCESS );

    IDE_TEST( cmiSetCallback( CMI_PROTOCOL( DB, ShardStmtPartialRollback ),
                              mmtServiceThread::shardStmtPartialRollback ) != IDE_SUCCESS );

    IDE_TEST( cmiSetCallback( CMI_PROTOCOL( DB, ShardNodeReport ),
                              mmtServiceThread::shardNodeReport) != IDE_SUCCESS );

    /* PROJ-2733-Protocol */
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, ConnectV3),
                            mmtServiceThread::connectProtocol) != IDE_SUCCESS);
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, ExecuteV3),
                            mmtServiceThread::executeProtocol) != IDE_SUCCESS);
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, ParamDataInListV3),
                            mmtServiceThread::paramDataInListProtocol) != IDE_SUCCESS);
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, PropertySetV3),
                            mmtServiceThread::propertySetProtocol) != IDE_SUCCESS);
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, ShardTransactionV3),
                            mmtServiceThread::shardTransactionProtocol) != IDE_SUCCESS);
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, ShardPrepareV3),
                            mmtServiceThread::shardPrepareProtocol) != IDE_SUCCESS);
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, ShardEndPendingTxV3),
                            mmtServiceThread::shardEndPendingTxProtocol) != IDE_SUCCESS);

    /* PROJ-2728 Sharding LOB */
    IDE_TEST( cmiSetCallback( CMI_PROTOCOL( DB, LobGetSizeV3 ),
                              mmtServiceThread::lobGetSizeProtocol ) != IDE_SUCCESS );

    IDE_TEST( cmiSetCallback( CMI_PROTOCOL( DB, FetchV3 ),
                              mmtServiceThread::fetchProtocol ) != IDE_SUCCESS );

    /* BUG-48775 */
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, PrepareV3),
                            mmtServiceThread::prepareProtocol) != IDE_SUCCESS);
    IDE_TEST(cmiSetCallback(CMI_PROTOCOL(DB, PrepareByCIDV3),
                            mmtServiceThread::prepareByCIDProtocol) != IDE_SUCCESS);

    /* bug-33841: ipc thread's state is wrongly displayed */
    (void) cmiSetCallbackSetExecute(mmtServiceThread::setExecuteCallback);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

iduMutex* mmtThreadManager::getIpcServiceThreadMutex(SInt aIpcServiceThreadID)
{
    return &(mIpcServiceThreadMutex[aIpcServiceThreadID]);
}

iduCond*    mmtThreadManager::getIpcServiceThreadCV(SInt aIpcServiceThreadID)
{
    return &(mIpcServiceThreadCV[aIpcServiceThreadID]);
}

/*PROJ-2616*/
iduMutex* mmtThreadManager::getIPCDAServiceThreadMutex( SInt aIPCDAServiceThreadID )
{
    return &(mIPCDAServiceThreadMutex[aIPCDAServiceThreadID]);
}

/*PROJ-2616*/
iduCond*  mmtThreadManager::getIPCDAServiceThreadCV( SInt aIPCDAServiceThreadID )
{
    return &(mIPCDAServiceThreadCV[aIPCDAServiceThreadID]);
}

IDE_RC mmtThreadManager::signalToThreadManager(void)
{
    IDE_ASSERT(mMutexForThreadManagerSignal.lock(NULL) == IDE_SUCCESS);
    IDE_TEST(mThreadManagerCV.signal());
    IDE_ASSERT(mMutexForThreadManagerSignal.unlock() == IDE_SUCCESS);
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC mmtThreadManager::addListener(cmiLinkImpl aLinkImpl, SChar *aMsgBuffer, UInt aMsgBufferSize)
{
    cmiListenArg      sListenArg;
    cmiDispatcherImpl sDispatcherImpl = CMI_DISPATCHER_IMPL_INVALID;
    SChar             sAddrBufferUnix[UNIX_FILE_PATH_LEN];
    SChar             sAddrBufferIpc[IPC_FILE_PATH_LEN];

    /*PROJ-2616*/
    SChar             sAddrBufferIPCDA[IPC_FILE_PATH_LEN];

    IDE_TEST_RAISE(cmiIsSupportedLinkImpl(aLinkImpl) != ID_TRUE, UnableToMakeListener);

    switch (aLinkImpl)
    {
        case CMI_LINK_IMPL_TCP:
            //TASK-3873 Static Analysis Code Sonar null pointer de-reference
            /* BUG-41168 SSL extension */
            IDE_TEST_RAISE(mmuProperty::getTcpEnable() == ID_FALSE, UnableToMakeListener);
            break;

        case CMI_LINK_IMPL_SSL:
            /* PROJ-2474 SSL/TLS */
            IDE_TEST_RAISE(mmuProperty::getSslEnable() == ID_FALSE, UnableToMakeListener);
            break;

        case CMI_LINK_IMPL_UNIX:
            sListenArg.mUNIX.mFilePath = sAddrBufferUnix;
            break;

        case CMI_LINK_IMPL_IPC:
            IDE_TEST_RAISE(mmuProperty::getIpcChannelCount() == 0, UnableToMakeListener);
            sListenArg.mIPC.mFilePath  = sAddrBufferIpc;
            break;

        case CMI_LINK_IMPL_IPCDA:/*PROJ-2616*/
            IDE_TEST_RAISE(mmuProperty::getIPCDAChannelCount() == 0, UnableToMakeListener);
            sListenArg.mIPCDA.mFilePath = sAddrBufferIPCDA;
            break;

        case CMI_LINK_IMPL_IB: /* PROJ-2681 */
            IDE_TEST_RAISE((mmuProperty::getIBEnable() == ID_FALSE) ||
                           (mmuProperty::getIBListenerDisable() == ID_TRUE), UnableToMakeListener);
            break;

        default:
            //TASK-3873 Static Analysis Code Sonar null pointer de-reference
            IDE_ASSERT(0);
            break;
    }

    IDE_TEST_RAISE(makeListenArg(&sListenArg,
                                 aLinkImpl,
                                 aMsgBuffer,
                                 aMsgBufferSize) != IDE_SUCCESS,
                   UnableToMakeListener);

    if (mListenLink[aLinkImpl] == NULL)
    {
        IDE_TEST(cmiAllocLink(&mListenLink[aLinkImpl],
                              CMI_LINK_TYPE_LISTEN,
                              aLinkImpl) != IDE_SUCCESS);

        sDispatcherImpl = cmiDispatcherImplForLinkImpl(aLinkImpl);

        // bug-27571: klocwork warnings: Array index out of bounds
        // invalid 값을 array index로 사용하면 array를 넘어서게 됨.
        IDE_TEST(sDispatcherImpl == CMI_DISPATCHER_IMPL_INVALID);


        IDE_TEST(allocDispatcher(sDispatcherImpl) != IDE_SUCCESS);

        if (mDispatcherRun == ID_TRUE)
        {
            IDE_TEST(cmiListenLink(mListenLink[aLinkImpl], &sListenArg) != IDE_SUCCESS);

            IDE_TEST(mDispatcher[sDispatcherImpl]->addLink(mListenLink[aLinkImpl]) != IDE_SUCCESS);

            if (mDispatcher[sDispatcherImpl]->isStarted() != ID_TRUE)
            {
                IDE_TEST(mDispatcher[sDispatcherImpl]->start() != IDE_SUCCESS);
                IDE_TEST(mDispatcher[sDispatcherImpl]->waitToStart() != IDE_SUCCESS);
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(UnableToMakeListener);
    {
        IDE_SET(ideSetErrorCode(mmERR_IGNORE_UNABLE_TO_MAKE_LISTENER));
    }
    IDE_EXCEPTION_END;
    {
        if (mListenLink[aLinkImpl] != NULL)
        {
            cmiFreeLink(mListenLink[aLinkImpl]);
            mListenLink[aLinkImpl] = NULL;
        }

        if (sDispatcherImpl != CMI_DISPATCHER_IMPL_INVALID)
        {
            freeDispatcher(sDispatcherImpl);
        }
    }

    return IDE_FAILURE;
}

IDE_RC mmtThreadManager::startListener()
{
    cmiListenArg      sListenArg;
    cmiDispatcherImpl sDispatcherImpl;
    SInt              sImpl;
    SChar             sAddrBufferUnix[UNIX_FILE_PATH_LEN];
    SChar             sAddrBufferIpc[IPC_FILE_PATH_LEN];
    SChar             sAddrBufferIPCDA[IPC_FILE_PATH_LEN];

    for (sImpl = CMI_LINK_IMPL_BASE; sImpl < CMI_LINK_IMPL_MAX; sImpl++)
    {
        if (mListenLink[sImpl] != NULL)
        {
            switch(sImpl)
            {
                case CMI_LINK_IMPL_UNIX:
                    sListenArg.mUNIX.mFilePath = sAddrBufferUnix;
                    break;
                case CMI_LINK_IMPL_IPC:
                    sListenArg.mIPC.mFilePath  = sAddrBufferIpc;
                    break;
                case CMI_LINK_IMPL_IPCDA:
                    sListenArg.mIPCDA.mFilePath = sAddrBufferIPCDA;
                    break;
                default:
                    break;
            }

            IDE_TEST(makeListenArg(&sListenArg,
                                   (cmiLinkImpl)sImpl,
                                   NULL,
                                   0) != IDE_SUCCESS);

            IDE_TEST(cmiListenLink(mListenLink[sImpl], &sListenArg) != IDE_SUCCESS);

            sDispatcherImpl = cmiDispatcherImplForLinkImpl((cmiLinkImpl)sImpl);
            // bug-26979: codesonar: buffer-overrun
            // invalid 값을 array index로 사용하면 array를 넘어서게 됨.
            IDE_TEST(sDispatcherImpl == CMI_DISPATCHER_IMPL_INVALID);

            IDE_TEST(mDispatcher[sDispatcherImpl]->addLink(mListenLink[sImpl]) != IDE_SUCCESS);
        }
    }

    for (sImpl = CMI_DISPATCHER_IMPL_BASE; sImpl < CMI_DISPATCHER_IMPL_MAX; sImpl++)
    {
        if (mDispatcher[sImpl] != NULL)
        {
            IDE_TEST(mDispatcher[sImpl]->start() != IDE_SUCCESS);
            IDE_TEST(mDispatcher[sImpl]->waitToStart() != IDE_SUCCESS);
        }
    }

    mDispatcherRun = ID_TRUE;
  
    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC mmtThreadManager::stopListener()
{
    mmtDispatcher *sDispatcher;
    UChar          sImpl;

    mDispatcherRun = ID_FALSE;

    /*
     * Listen Dispatcher를 종료한다. Service Dispatcher는 finalize() 호출시 종료한다.
     */

    for (sImpl = CMI_DISPATCHER_IMPL_BASE; sImpl < CMI_DISPATCHER_IMPL_MAX; sImpl++)
    {
        sDispatcher = mDispatcher[sImpl];

        if (sDispatcher != NULL)
        {
            sDispatcher->stop();

            IDE_TEST(sDispatcher->join() != IDE_SUCCESS);
        }
    }

    /*
     * Listen Link를 닫는다.
     */

    for (sImpl = CMI_LINK_IMPL_BASE; sImpl < CMI_LINK_IMPL_MAX; sImpl++)
    {
        if (mListenLink[sImpl] != NULL)
        {
            IDE_TEST(cmiCloseLink(mListenLink[sImpl]) != IDE_SUCCESS);
        }
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC mmtThreadManager::startServiceThreads()
{
    UInt      sCount;
    iduMutex* sMutex;
    SChar     sBuffer[128];
    UInt      sIpcChannelCount;
    /* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
    */
    UInt      sDummy;

    /*PROJ-2616*/
    UInt      sIPCDAChannelCount = mmuProperty::getIPCDAChannelCount();

    /*
     * 주 Thread들을 생성
     */

    // Socket Service Thread
    /* PROJ-2108 Dedicated thread mode which uses less CPU */
    if ( mmuProperty::getIsDedicatedMode() == 0 )
    {
        for (sCount = 0; sCount < mmuProperty::getMultiplexingThreadCount(); sCount++)
        {
            /* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase */
            IDE_TEST(startServiceThread(MMC_SERVICE_THREAD_TYPE_SOCKET, 1, &sDummy) != IDE_SUCCESS);
        }
    }
    else
    {
        for (sCount = 0; sCount < mmuProperty::getDedicatedThreadInitCount(); sCount++)
        {
            IDE_TEST(startServiceThread(MMC_SERVICE_THREAD_TYPE_DEDICATED, 1, &sDummy) != IDE_SUCCESS);
        }
    }

    sIpcChannelCount = mmuProperty::getIpcChannelCount();
    if (sIpcChannelCount > 0)
    {
        IDU_FIT_POINT_RAISE( "mmtThreadManager::startServiceThreads::malloc::IpcServiceThreadArray",
                              InsufficientMemory );
        
        IDE_TEST_RAISE( iduMemMgr::malloc(IDU_MEM_MMT,
                                   ID_SIZEOF(mmtServiceThread*) * sIpcChannelCount,
                                   (void **)&mIpcServiceThreadArray,
                                   IDU_MEM_IMMEDIATE) != IDE_SUCCESS, InsufficientMemory );

        IDU_FIT_POINT_RAISE( "mmtThreadManager::startServiceThreads::malloc::IpcServiceThreadMutex",
                              InsufficientMemory );

        IDE_TEST_RAISE( iduMemMgr::malloc(IDU_MEM_MMT,
                                   ID_SIZEOF(iduMutex) * sIpcChannelCount,
                                   (void **)&mIpcServiceThreadMutex,
                                   IDU_MEM_IMMEDIATE) != IDE_SUCCESS, InsufficientMemory );

        IDU_FIT_POINT_RAISE( "mmtThreadManager::startServiceThreads::malloc::IpcServiceThreadCV",
                              InsufficientMemory );
        
        IDE_TEST_RAISE( iduMemMgr::malloc(IDU_MEM_MMT,
                                   ID_SIZEOF(PDL_cond_t) * sIpcChannelCount,
                                   (void **)&mIpcServiceThreadCV,
                                   IDU_MEM_IMMEDIATE) != IDE_SUCCESS, InsufficientMemory );

        // Ipc Service Thread
        for (sCount = 0; sCount < sIpcChannelCount; sCount++)
        {
            idlOS::sprintf(sBuffer, "IPC_SERVICE_THREAD_MUTEX_%d", sCount);
            sMutex = new (&mIpcServiceThreadMutex[sCount])iduMutex();
            IDE_TEST( sMutex == NULL );     //BUG-28994 [CodeSonar]Null Pointer Dereference
            IDE_TEST(sMutex->initialize(sBuffer,
                                        IDU_MUTEX_KIND_POSIX,
                                        IDV_WAIT_INDEX_NULL) != IDE_SUCCESS);

            IDE_TEST_RAISE(mIpcServiceThreadCV[sCount].initialize()
                           != IDE_SUCCESS, ContInitError);
        }

        for (sCount = 0; sCount < sIpcChannelCount; sCount++)
        {
            /* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase */
            IDE_TEST(startServiceThread(MMC_SERVICE_THREAD_TYPE_IPC, 1, &sDummy) != IDE_SUCCESS);
        }
    }
    
    /*PROJ-2616*/
    if (sIPCDAChannelCount > 0)
    {
        IDU_FIT_POINT_RAISE( "mmtThreadManager::startServiceThreads::malloc::IPCDAServiceThreadArray",
                              InsufficientMemory );

        IDE_TEST_RAISE( iduMemMgr::malloc(IDU_MEM_MMT,
                                          ID_SIZEOF(mmtServiceThread*) * sIPCDAChannelCount,
                                          (void **)&mIPCDAServiceThreadArray,
                                          IDU_MEM_IMMEDIATE) != IDE_SUCCESS, InsufficientMemory );

        IDU_FIT_POINT_RAISE( "mmtThreadManager::startServiceThreads::malloc::IPCDAServiceThreadMutex",
                              InsufficientMemory );

        IDE_TEST_RAISE( iduMemMgr::malloc(IDU_MEM_MMT,
                                          ID_SIZEOF(iduMutex) * sIPCDAChannelCount,
                                          (void **)&mIPCDAServiceThreadMutex,
                                          IDU_MEM_IMMEDIATE) != IDE_SUCCESS, InsufficientMemory );

        IDU_FIT_POINT_RAISE( "mmtThreadManager::startServiceThreads::malloc::IPCDAServiceThreadCV",
                              InsufficientMemory );

        IDE_TEST_RAISE( iduMemMgr::malloc(IDU_MEM_MMT,
                                          ID_SIZEOF(PDL_cond_t) * sIPCDAChannelCount,
                                          (void **)&mIPCDAServiceThreadCV,
                                          IDU_MEM_IMMEDIATE) != IDE_SUCCESS, InsufficientMemory );

        /* IPCDA Service Thread */
        for (sCount = 0; sCount < sIPCDAChannelCount; sCount++)
        {
            idlOS::sprintf(sBuffer, "IPCDA_SERVICE_THREAD_MUTEX_%d", sCount);
            sMutex = new (&mIPCDAServiceThreadMutex[sCount])iduMutex();
            IDE_TEST( sMutex == NULL );
            IDE_TEST(sMutex->initialize(sBuffer,
                                        IDU_MUTEX_KIND_POSIX,
                                        IDV_WAIT_INDEX_NULL) != IDE_SUCCESS);

            IDE_TEST_RAISE(mIPCDAServiceThreadCV[sCount].initialize()
                           != IDE_SUCCESS, ContInitError);
        }

        for (sCount = 0; sCount < sIPCDAChannelCount; sCount++)
        {
            /* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase */
            IDE_TEST(startServiceThread(MMC_SERVICE_THREAD_TYPE_IPCDA, 1, &sDummy) != IDE_SUCCESS);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ContInitError);
    {
        IDE_SET(ideSetErrorCode(mmERR_FATAL_THREAD_CONDITION_INIT));
    }
    IDE_EXCEPTION(InsufficientMemory);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;
    // BUG-18832
    if(mIpcServiceThreadCV != NULL)
    {
        IDE_ASSERT(iduMemMgr::free(mIpcServiceThreadCV) == IDE_SUCCESS);
        mIpcServiceThreadCV = NULL;
    }

    if(mIpcServiceThreadMutex != NULL)
    {
        IDE_ASSERT(iduMemMgr::free(mIpcServiceThreadMutex) == IDE_SUCCESS);
        mIpcServiceThreadMutex = NULL;
    }

    if(mIpcServiceThreadArray != NULL)
    {
        IDE_ASSERT(iduMemMgr::free(mIpcServiceThreadArray) == IDE_SUCCESS);
        mIpcServiceThreadArray = NULL;
    }

    /*PROJ-2616*/
    if(mIPCDAServiceThreadCV != NULL)
    {
        IDE_ASSERT(iduMemMgr::free(mIPCDAServiceThreadCV) == IDE_SUCCESS);
        mIPCDAServiceThreadCV = NULL;
    }

    /*PROJ-2616*/
    if(mIPCDAServiceThreadMutex != NULL)
    {
        IDE_ASSERT(iduMemMgr::free(mIPCDAServiceThreadMutex) == IDE_SUCCESS);
        mIPCDAServiceThreadMutex = NULL;
    }

    /*PROJ-2616*/
    if(mIPCDAServiceThreadArray != NULL)
    {
        IDE_ASSERT(iduMemMgr::free(mIPCDAServiceThreadArray) == IDE_SUCCESS);
        mIPCDAServiceThreadArray = NULL;
    }
    return IDE_FAILURE;
}

IDE_RC mmtThreadManager::stopServiceThreads()
{
    mmtServiceThread *sThread;
    iduListNode      *sIterator;
    iduListNode      *sNodeNext;
    UInt              sWaitCount = 0;
    UInt              sIpcID;
    UInt              sCount;
    IDE_RC            sRC;

    lock();


    IDU_LIST_ITERATE_SAFE(&mServiceThreadSocketList, sIterator, sNodeNext)
    {
        sThread = (mmtServiceThread *)sIterator->mObj;
        sThread->stop();

        /*
         * BUG-37038
         *
         * sThread->stop()이 먼저 실행되어야 한다.
         * 순서가 바뀌면 mutex destroy()시 실패(EBUSY)할 수 있다.
         */
        IDL_MEM_BARRIER;

        /* PROJ-2108 Dedicated thread mode which uses less CPU */
        /* BUG-37038 DEDICATED TYPE인 경우에만 signal을 보낸다. */
        if (sThread->getServiceThreadType() == MMC_SERVICE_THREAD_TYPE_DEDICATED)
        {
            IDE_TEST(sThread->signalToServiceThread(sThread));
        }
        else
        {
            /* Nothing */
        }
    }

    IDU_LIST_ITERATE_SAFE(&mServiceThreadIPCList, sIterator, sNodeNext)
    {
        sThread = (mmtServiceThread *)sIterator->mObj;

        sThread->stop();
        sIpcID = sThread->getIpcID();

        /*
         * BUG-37038
         *
         * sThread->stop()이 먼저 실행되어야 한다.
         * 순서가 바뀌면 mutex destroy()시 실패(EBUSY)할 수 있다.
         */
        IDL_MEM_BARRIER;

        sRC = getIpcServiceThreadCV(sIpcID)->signal();
        if(sRC != IDE_SUCCESS)
        {
            unlock();
            IDE_RAISE(CondSignalError);
        }
    }

    /*PROJ-2616*/
    IDU_LIST_ITERATE_SAFE(&mServiceThreadIPCDAList, sIterator, sNodeNext)
    {
        sThread = (mmtServiceThread *)sIterator->mObj;

        sThread->stop();
        sIpcID = sThread->getIpcID();

        /*
         * BUG-37038
         *
         * sThread->stop()이 먼저 실행되어야 한다.
         * 순서가 바뀌면 mutex destroy()시 실패(EBUSY)할 수 있다.
         */
        IDL_MEM_BARRIER;

        sRC = getIPCDAServiceThreadCV(sIpcID)->signal();
        if(sRC != IDE_SUCCESS)
        {
            unlock();
            IDE_RAISE(CondSignalError);
        }
    }

    //PROJ-1677 DEQUEUE
    IDU_LIST_ITERATE_SAFE(&mTempDedicateServiceThrList, sIterator, sNodeNext)
    {
        sThread = (mmtServiceThread *)sIterator->mObj;

        sThread->stop();
    }

    unlock();

    //==========================================================
    // bug-22948: shutdown 중에 thread를 완전히 종료시키지 못해
    // 추후 mutex_destroy 도중 EBUSY로 비정상 종료함
    // 수정사항: 기존에 10초(STOP_THREAD_WAIT_TIMEOUT)만
    // 기다리던 것을 종료할때까지 무한 대기하도록 수정함
    while ((getServiceThreadSocketCount() > 0) ||
            (getServiceThreadIPCCount() > 0))
    {
        idlOS::sleep(1);
        sWaitCount++;
        // for debug
        ideLog::log(IDE_SERVER_0,
                    "wait for threads [sock: %u, IPC: %u]"
                    "to end [%u secs]",
                    getServiceThreadSocketCount(),
                    getServiceThreadIPCCount(),
                    sWaitCount);
    }

    for (sCount = 0; sCount < mmuProperty::getIpcChannelCount(); sCount++)
    {
        IDE_TEST_RAISE(getIpcServiceThreadCV(sCount)->destroy() != IDE_SUCCESS, CondDestroyError);
        IDE_ASSERT(getIpcServiceThreadMutex(sCount)->destroy() == IDE_SUCCESS);
    }

    for(sCount = 0; sCount < mmuProperty::getIPCDAChannelCount(); sCount++)
    {
        IDE_TEST_RAISE(getIPCDAServiceThreadCV(sCount)->destroy() != IDE_SUCCESS, CondDestroyError);
        IDE_ASSERT(getIPCDAServiceThreadMutex(sCount)->destroy() == IDE_SUCCESS);
    }

    if(mIpcServiceThreadCV != NULL)
    {
        IDE_TEST(iduMemMgr::free(mIpcServiceThreadCV) != IDE_SUCCESS);
        mIpcServiceThreadCV = NULL;
    }

    if(mIpcServiceThreadMutex != NULL)
    {
        IDE_TEST(iduMemMgr::free(mIpcServiceThreadMutex) != IDE_SUCCESS);
        mIpcServiceThreadMutex = NULL;
    }

    // BUG-18832
    if(mIpcServiceThreadArray != NULL)
    {
        IDE_TEST(iduMemMgr::free(mIpcServiceThreadArray) != IDE_SUCCESS);
        mIpcServiceThreadArray = NULL;
    }

    /*PROJ-2616*/
    if(mIPCDAServiceThreadCV != NULL)
    {
        IDE_TEST(iduMemMgr::free(mIPCDAServiceThreadCV) != IDE_SUCCESS);
        mIPCDAServiceThreadCV = NULL;
    }

    /*PROJ-2616*/
    if(mIPCDAServiceThreadMutex != NULL)
    {
        IDE_TEST(iduMemMgr::free(mIPCDAServiceThreadMutex) != IDE_SUCCESS);
        mIPCDAServiceThreadMutex = NULL;
    }

    /*PROJ-2616*/
    if(mIPCDAServiceThreadArray != NULL)
    {
        IDE_TEST(iduMemMgr::free(mIPCDAServiceThreadArray) != IDE_SUCCESS);
        mIPCDAServiceThreadArray = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(CondSignalError);
    {
        IDE_SET(ideSetErrorCode(mmERR_FATAL_THREAD_CONDITION_SIGNAL));
    }
    IDE_EXCEPTION(CondDestroyError);
    {
        IDE_SET(ideSetErrorCode(mmERR_FATAL_THREAD_CONDITION_DESTROY));
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC getIPv6Info(UInt *aIPv6,
                          SChar *aIPv6Str,
                          UInt aIPv6StrLen)
{
    IDE_TEST_RAISE(aIPv6Str == NULL, FAILED_TO_GET_IPV6_INFO);

    /* proj-1538 ipv6 */
    if (*aIPv6 == NET_CONN_IP_STACK_V4_ONLY)
    {
        idlOS::snprintf(aIPv6Str, aIPv6StrLen, "[IPV4]");
    }
    else if (*aIPv6 == NET_CONN_IP_STACK_V6_DUAL)
    {
        idlOS::snprintf(aIPv6Str, aIPv6StrLen, "[IPV6-DUAL]");
    }
    else
    {
        /* hpux 11.11 not defined IPV6_V6ONLY */
# if defined(IPV6_V6ONLY)
        idlOS::snprintf(aIPv6Str, aIPv6StrLen, "[IPV6-ONLY]");
# else
        idlOS::snprintf(aIPv6Str, aIPv6StrLen, "[IPV6-DUAL] v6only not supported");
        *aIPv6 = NET_CONN_IP_STACK_V6_DUAL;
# endif
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(FAILED_TO_GET_IPV6_INFO)
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_GET_IPV6_INFO));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmtThreadManager::makeListenArg(cmiListenArg *aListenArg,
                                       cmiLinkImpl   aImpl,
                                       SChar        *aMsgBuffer,
                                       UInt          aMsgBufferSize)
{
    SChar sIPv6Str[MM_IP_VER_STRLEN_ON_BOOT + 1] = {0,};
    UInt  sRet = IDE_SUCCESS;

    switch (aImpl)
    {
        case CMI_LINK_IMPL_TCP:
            aListenArg->mTCP.mPort       = mmuProperty::getPortNo();
            aListenArg->mTCP.mMaxListen  = mmuProperty::getMaxListen();

            aListenArg->mTCP.mIPv6       = iduProperty::getNetConnIpStack();

            sRet = getIPv6Info(&aListenArg->mTCP.mIPv6, sIPv6Str, MM_IP_VER_STRLEN_ON_BOOT);
            IDE_TEST(sRet != IDE_SUCCESS);

            if (aMsgBuffer != NULL)
            {
                idlOS::snprintf(aMsgBuffer,
                                aMsgBufferSize,
                                "TCP on port %" ID_UINT32_FMT" %s",
                                aListenArg->mTCP.mPort,
                                sIPv6Str);
            }
            else
            {
                /* aMsgBuffer is NULL. */
            }

            break;

        case CMI_LINK_IMPL_SSL:
            aListenArg->mSSL.mPort       = mmuProperty::getSslPortNo();
            aListenArg->mSSL.mMaxListen  = mmuProperty::getSslMaxListen();

            aListenArg->mSSL.mIPv6       = iduProperty::getNetConnIpStack();

            sRet = getIPv6Info(&aListenArg->mSSL.mIPv6, sIPv6Str, MM_IP_VER_STRLEN_ON_BOOT);
            IDE_TEST(sRet != IDE_SUCCESS);

            if (aMsgBuffer != NULL)
            {
                idlOS::snprintf(aMsgBuffer,
                                aMsgBufferSize,
                                "SSL on port %" ID_UINT32_FMT" %s",
                                aListenArg->mSSL.mPort,
                                sIPv6Str);
            }

            break;

        case CMI_LINK_IMPL_UNIX:
            IDE_DASSERT(aListenArg->mUNIX.mFilePath != NULL);

            idlOS::snprintf(aListenArg->mUNIX.mFilePath,
                            UNIX_FILE_PATH_LEN,
                            "%s",
                            mmuProperty::getUnixdomainFilepath());

            aListenArg->mUNIX.mMaxListen = mmuProperty::getMaxListen();

            if (aMsgBuffer != NULL)
            {
                idlOS::snprintf(aMsgBuffer, aMsgBufferSize, "UNIX");
            }

            break;

        case CMI_LINK_IMPL_IPC:
            IDE_DASSERT(aListenArg->mIPC.mFilePath != NULL);

            /* BUG-35332 The socket files can be moved */
            idlOS::snprintf(aListenArg->mIPC.mFilePath,
                            IPC_FILE_PATH_LEN,
                            "%s",
                            mmuProperty::getIpcFilepath());

            aListenArg->mIPC.mMaxListen = mmuProperty::getIpcChannelCount();

            if (aMsgBuffer != NULL)
            {
                idlOS::snprintf(aMsgBuffer, aMsgBufferSize, "IPC");
            }
            break;

        case CMI_LINK_IMPL_IPCDA:    /*PROJ-2616*/
            IDE_DASSERT(aListenArg->mIPCDA.mFilePath != NULL);
            idlOS::snprintf(aListenArg->mIPCDA.mFilePath,
                            IPC_FILE_PATH_LEN,
                            "%s",
                            mmuProperty::getIPCDAFilepath());

            aListenArg->mIPCDA.mMaxListen = mmuProperty::getIPCDAChannelCount();

            if (aMsgBuffer != NULL)
            {
                idlOS::snprintf(aMsgBuffer, aMsgBufferSize, "IPCDA");
            }
            break;

        case CMI_LINK_IMPL_IB: /* PROJ-2681 */
            aListenArg->mIB.mPort       = mmuProperty::getIBPortNo();
            aListenArg->mIB.mMaxListen  = mmuProperty::getIBMaxListen();
            aListenArg->mIB.mLatency    = mmuProperty::getIBLatency();
            aListenArg->mIB.mConChkSpin = mmuProperty::getIBConChkSpin();

            aListenArg->mIB.mIPv6       = iduProperty::getNetConnIpStack();

            sRet = getIPv6Info(&aListenArg->mIB.mIPv6, sIPv6Str, MM_IP_VER_STRLEN_ON_BOOT);
            IDE_TEST(sRet != IDE_SUCCESS);

            if (aMsgBuffer != NULL)
            {
                idlOS::snprintf(aMsgBuffer,
                                aMsgBufferSize,
                                "IB on port %" ID_UINT32_FMT" %s",
                                aListenArg->mTCP.mPort,
                                sIPv6Str);
            }
            break;

        default:
            IDE_RAISE(UnsupportedNetworkProtocol);
            break; 
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(UnsupportedNetworkProtocol)
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_UNSUPPORTED_NETWORK_PROTOCOL));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmtThreadManager::allocDispatcher(cmiDispatcherImpl aImpl)
{
    mmtDispatcher *sDispatcher = NULL;

    /*
     * Dispatcher 생성
     */

    if (mDispatcher[aImpl] == NULL)
    {
        /*
         * Dispatcher 메모리 할당
         */

        IDU_FIT_POINT( "mmtThreadManager::allocDispatcher::malloc::Dispatcher" );

        IDE_TEST(iduMemMgr::malloc(IDU_MEM_MMT,
                                   ID_SIZEOF(mmtDispatcher),
                                   (void **)&sDispatcher,
                                   IDU_MEM_IMMEDIATE) != IDE_SUCCESS);

        /*
         * Dispatcher 초기화
         */

        sDispatcher = new (sDispatcher) mmtDispatcher();

        IDE_TEST(sDispatcher == NULL);

        IDE_TEST(sDispatcher->initialize(aImpl) != IDE_SUCCESS);

        mDispatcher[aImpl] = sDispatcher;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        if (sDispatcher != NULL)
        {
            // fix BUG-28267 [codesonar] Ignored Return Value
            IDE_ASSERT(iduMemMgr::free(sDispatcher) == IDE_SUCCESS);
        }
    }

    return IDE_FAILURE;
}

IDE_RC mmtThreadManager::freeDispatcher(cmiDispatcherImpl aImpl)
{
    if (mDispatcher[aImpl] != NULL)
    {
        IDE_TEST(mDispatcher[aImpl]->finalize() != IDE_SUCCESS);

        IDE_TEST(iduMemMgr::free(mDispatcher[aImpl]) != IDE_SUCCESS);

        mDispatcher[aImpl] = NULL;
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}
//fix BUG-19464
IDE_RC mmtThreadManager::allocServiceThread(mmcServiceThreadType       aServiceThreadType,
                                            mmtServiceThreadStartFunc  aServiceThreadStartFunc,
                                            mmtServiceThread           **aServiceThread)
{
    mmtServiceThread *sThread = NULL;

    IDU_FIT_POINT( "mmtThreadManager::allocServiceThread::alloc::Thread" );

    IDE_TEST(mServiceThreadPool.alloc((void **)&sThread) != IDE_SUCCESS);

    sThread = new (sThread) mmtServiceThread();

    IDE_TEST(sThread == NULL);

    IDE_TEST(sThread->initialize(aServiceThreadType,
                                 aServiceThreadStartFunc) != IDE_SUCCESS);

    *aServiceThread = sThread;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        if (sThread != NULL)
        {
            mServiceThreadPool.memfree(sThread);
        }
    }

    return IDE_FAILURE;
}

IDE_RC mmtThreadManager::freeServiceThread(mmtServiceThread *aServiceThread)
{
    IDE_ASSERT(aServiceThread->finalize() == IDE_SUCCESS);

    IDE_ASSERT(mServiceThreadPool.memfree(aServiceThread) == IDE_SUCCESS);

    return IDE_SUCCESS;
}

 /* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
    기존과달리 한개씩 service thread를 추가하는것이 아니라,
    통계치 계산에 의하여 적절한 service thread갯수만큼
    생성한다.
 */
IDE_RC mmtThreadManager::startServiceThread(mmcServiceThreadType aServiceThreadType,UInt aNewThrCnt, UInt *aAllocateThrCnt)
{
    UInt               i;
    mmtServiceThread *sServiceThread = NULL;

    //fix BUG-19464
    *aAllocateThrCnt = 0;
    
    for(i = 0;  i < aNewThrCnt; i++)
    {    
        IDE_TEST(allocServiceThread(aServiceThreadType,
                                    mmtThreadManager::serviceThreadStarted,
                                    &sServiceThread) != IDE_SUCCESS);

        IDU_FIT_POINT_RAISE( "mmtThreadManager::startServiceThread::Thread::ServieeThread", 
                              StartFail );

        IDE_TEST_RAISE(sServiceThread->start() != IDE_SUCCESS, StartFail);
        *aAllocateThrCnt = *aAllocateThrCnt+1;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(StartFail);
    {
        IDE_ASSERT(freeServiceThread(sServiceThread) == IDE_SUCCESS);
    }
    IDE_EXCEPTION_END;
    if( *aAllocateThrCnt == 0)
    {
        return IDE_FAILURE;
    }
    else
    {
        return IDE_SUCCESS;
    }
}
/* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
   BUG-29083 When a task is assigned to service-thread, dispatcher make effort to choose a proper service-thread.
   busy degree가 제일 작은 service thread에게 task분배를 한다.
 */
mmtServiceThread* mmtThreadManager::chooseServiceThread()
{
    iduListNode      *sIterator;
    mmtServiceThread *sThread;
    mmtServiceThread *sMinBusyDegreeThread = NULL;
    ULong             sMinBusyDegree = ID_ULONG_MAX;
    ULong             sCurrentBusyDegree;


    IDU_LIST_ITERATE(&mServiceThreadSocketList, sIterator)
    {
        sThread = (mmtServiceThread *)sIterator->mObj;
        sCurrentBusyDegree = sThread->getBusyDegree(ID_TRUE);
        if ((sMinBusyDegreeThread   == NULL) ||
            (sCurrentBusyDegree < sMinBusyDegree))
        {
            sMinBusyDegreeThread = sThread;
            sMinBusyDegree = sCurrentBusyDegree;
            if( sMinBusyDegree == 0)
            {
                break;
            }
        }
    }//IDU_LIST_ITERATE
    
    return sMinBusyDegreeThread;
}

/* PROJ-2108 Dedicated thread mode which uses less CPU */
mmtServiceThread* mmtThreadManager::chooseDedicatedServiceThread()
{
    iduListNode      *sIterator;
    mmtServiceThread *sThread;
    mmtServiceThread *sIdleThread = NULL;

    IDU_LIST_ITERATE(&mServiceThreadSocketList, sIterator)
    {
        sThread = (mmtServiceThread *)sIterator->mObj;
        /* find idle thread which has no task */
        if ( sThread->getAssignedTasks() == 0 )
        {
            sIdleThread = sThread;
            break;
        }
    }//IDU_LIST_ITERATE
    
    return sIdleThread;
}

void mmtThreadManager::freeServiceThreads(iduList *aList)
{
    mmtServiceThread *sServiceThread;
    iduListNode      *sIterator;
    iduListNode      *sNodeNext;

    IDU_LIST_ITERATE_SAFE(aList, sIterator, sNodeNext)
    {
        sServiceThread = (mmtServiceThread *)sIterator->mObj;

        IDE_ASSERT(sServiceThread->join() == IDE_SUCCESS);

        /* BUG-45274 reference http://nok.altibase.com/pages/viewpage.action?pageId=40570104  */
        ideLog::logLine(IDE_LB_1, "L1 Free ServiceThread(%"ID_UINT32_FMT")",
                        sServiceThread->getServiceThreadID());
        
        IDE_ASSERT(freeServiceThread(sServiceThread) == IDE_SUCCESS);
    }
}

IDE_RC mmtThreadManager::addSocketTask(mmcTask *aTask)
{
    UInt              sWaitCount = 0;
    mmtServiceThread *sChosenThread;

    while (1)
    {
        /* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
           instruction pipe-line때문에 이 코드가 들어감.
         */
        ID_SERIAL_BEGIN(lock());
        ID_SERIAL_EXEC(sChosenThread = chooseServiceThread(),1);
        if(sChosenThread != NULL)
        {
            ID_SERIAL_EXEC(sChosenThread->addTask(aTask),2);
            ID_SERIAL_EXEC(unlock(),3);
            break;
        }
        else
        {    
            ID_SERIAL_EXEC(unlock(),2);
            ID_SERIAL_EXEC(idlOS::sleep(1),3);
            IDE_TEST_RAISE(sWaitCount >= START_THREAD_WAIT_TIMEOUT, AddTaskTimedOut);
        }//else
        ID_SERIAL_END(sWaitCount++);

    }//while

    return IDE_SUCCESS;

    IDE_EXCEPTION(AddTaskTimedOut);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_ADD_TASK_TIMED_OUT));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
 /* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
 */
void  mmtThreadManager::assignSocketTask(mmcTask *aTask)
{
    mmtServiceThread *sChosenThread;
    
    sChosenThread = chooseServiceThread();
    
    IDE_ASSERT(sChosenThread != NULL);
    
    sChosenThread->addTask(aTask);
}

/* PROJ-2108 Dedicated thread mode which uses less CPU */
IDE_RC mmtThreadManager::addDedicatedTask(mmcTask *aTask)
{
    mmtServiceThread *sChosenThread;

    /* PROJ-2108 Dedicated thread mode which uses less CPU */
    UInt sDummy;
    UInt sNumberofThreads;
    UInt sWaitCount = 0;
    PDL_Time_Value sSleepTime;
    sSleepTime.set(0, 1000);

    while (1)
    {
        sNumberofThreads = getServiceThreadSocketCount();
        ID_SERIAL_BEGIN(sChosenThread = chooseDedicatedServiceThread());
        if ( sChosenThread != NULL )
        {
            ID_SERIAL_EXEC(sChosenThread->addTask(aTask),1);
            ID_SERIAL_EXEC(IDE_TEST_RAISE(sChosenThread->signalToServiceThread(sChosenThread) 
                                          != IDE_SUCCESS, ThreadStartFail),2);
            break;
        }
        else
        {
            if ( (sNumberofThreads < mmuProperty::getDedicatedThreadMaxCount()) ||
                 (sWaitCount >= START_THREAD_WAIT_TIMEOUT) )
            {
                ID_SERIAL_EXEC(IDE_ASSERT(mMutexForThreadManagerSignal.lock(NULL) 
                                          == IDE_SUCCESS),1);
                ID_SERIAL_EXEC(IDE_TEST_RAISE(startServiceThread(MMC_SERVICE_THREAD_TYPE_DEDICATED, 1, &sDummy) 
                                              != IDE_SUCCESS, ThreadStartFail),2);
                /* Wait for the service thread until it is added to the service thread list */
                ID_SERIAL_EXEC(mThreadManagerCV.wait(&mMutexForThreadManagerSignal), 3);
                ID_SERIAL_EXEC(IDE_ASSERT(mMutexForThreadManagerSignal.unlock() == IDE_SUCCESS),4);
            }
            /* if the number of thread = DEDICATED_THREAD_MAX_COUNT, 
             * waits for ( 1 msec * 10 ) since it takes a while
             * for server to recognize disconnection of client.
             */
            else
            {
                    idlOS::select(0, NULL, NULL, NULL, sSleepTime);
            }
        }//else
        ID_SERIAL_END(sWaitCount++);
    }//while

    return IDE_SUCCESS;

    IDE_EXCEPTION(ThreadStartFail);
    {
        unlock();
        mmtThreadManager::logError(MM_TRC_THREAD_MANAGER_START_THREAD_FAILED);
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mmtThreadManager::addIpcTask(mmcTask *aTask)
{
    cmiLink*          sLink;
    SInt              sChannelID;
    mmtServiceThread* sIpcServiceThread;
    iduMutex*         sIpcServiceThreadMutex;
    iduCond*          sIpcServiceThreadCV;
    idBool            sAddTask = ID_FALSE;
    iduList           sTaskList;
    SInt              sRC;

    // 1. 클라이언트 Channel 할당
    sLink = aTask->getLink();
    IDE_TEST(cmiAllocChannel(sLink, &sChannelID) != IDE_SUCCESS);

    // fix BUG-25102
    // 클라이언트가 정상 접속이 된 후 TASK를 Thread에 추가하도록 한다.
    // 2. 클라이언트에게 IPC 정보 전송
    IDE_TEST(cmiHandshake(sLink) != IDE_SUCCESS);

    // 3. IPC Service Thread에 Task 추가
    sIpcServiceThread = mIpcServiceThreadArray[sChannelID];
    sIpcServiceThread->addTask(aTask);
    sAddTask = ID_TRUE;

    // 4. IPC Service Thread 동작
    sIpcServiceThreadMutex = &mIpcServiceThreadMutex[sChannelID];
    sIpcServiceThreadCV    = &mIpcServiceThreadCV[sChannelID];

    IDE_ASSERT(sIpcServiceThreadMutex->lock(NULL /* idvSQL* */) == IDE_SUCCESS);

    sRC = sIpcServiceThreadCV->signal();
    if(sRC != IDE_SUCCESS)
    {
        IDE_ASSERT(sIpcServiceThreadMutex->unlock() == IDE_SUCCESS);
        IDE_RAISE(CondSignalError);
    }

    IDE_ASSERT(sIpcServiceThreadMutex->unlock() == IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(CondSignalError);
    {
        IDE_SET(ideSetErrorCode(mmERR_FATAL_THREAD_CONDITION_SIGNAL));
    }
    IDE_EXCEPTION_END;

    if (sAddTask == ID_TRUE)
    {
        IDU_LIST_INIT(&sTaskList);
        sIpcServiceThread->removeAllTasks(&sTaskList, MMT_SERVICE_THREAD_NO_LOCK);
    }

    cmiCloseLink(sLink);

    return IDE_FAILURE;
}

/*****************************************************
 * PROJ-2616
 * IPCDA 연결에 대하여 테스크 스택에 추가하는 함수
 *
 * aTask [in] - mmaTack
 *****************************************************/
IDE_RC mmtThreadManager::addIPCDATask(mmcTask *aTask)
{
    cmiLink*          sLink;
    SInt              sChannelID;
    mmtServiceThread* sIPCDAServiceThread      = NULL;
    iduMutex*         sIPCDAServiceThreadMutex = NULL;
    iduCond*          sIPCDAServiceThreadCV    = NULL;
    idBool            sAddTask = ID_FALSE;
    iduList           sTaskList;
    SInt              sRC;

    UInt              sThreadCnt = mmuProperty::getIPCDAChannelCount();

    /* 1. 클라이언트 Channel 할당 */
    sLink = aTask->getLink();
    IDE_TEST(cmiAllocChannel(sLink, &sChannelID) != IDE_SUCCESS);

    cmnLinkPeerInitIPCDA(aTask->getProtocolContext(), sChannelID );

    /* fix BUG-25102 */
    /* 클라이언트가 정상 접속이 된 후 TASK를 Thread에 추가하도록 한다. */
    /* 2. 클라이언트에게 IPC 정보 전송 */
    IDE_TEST(cmiHandshake(sLink) != IDE_SUCCESS);

    /* 3. IPC Service Thread에 Task 추가 */
    sIPCDAServiceThread = mIPCDAServiceThreadArray[sThreadCnt == 1?0:sChannelID%sThreadCnt];
    sIPCDAServiceThread->addTask(aTask);

    sAddTask = ID_TRUE;
    if (sIPCDAServiceThread->getReadyTaskCount() == 1)
    {
        /* 4. IPC Service Thread 동작 */
        sIPCDAServiceThreadMutex = &mIPCDAServiceThreadMutex[sThreadCnt == 1?0:sChannelID%sThreadCnt];
        sIPCDAServiceThreadCV    = &mIPCDAServiceThreadCV[sThreadCnt == 1?0:sChannelID%sThreadCnt];

        IDE_ASSERT(sIPCDAServiceThreadMutex->lock(NULL /* idvSQL* */) == IDE_SUCCESS);


        sRC = sIPCDAServiceThreadCV->signal();
        if(sRC != IDE_SUCCESS)
        {
            IDE_ASSERT(sIPCDAServiceThreadMutex->unlock() == IDE_SUCCESS);
            IDE_RAISE(CondSignalError);
        }
        IDE_ASSERT(sIPCDAServiceThreadMutex->unlock() == IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(CondSignalError);
    {
        IDE_SET(ideSetErrorCode(mmERR_FATAL_THREAD_CONDITION_SIGNAL));
    }
    IDE_EXCEPTION_END;

    if (sAddTask == ID_TRUE)
    {
        IDU_LIST_INIT(&sTaskList);
        sIPCDAServiceThread->removeAllTasks(&sTaskList, MMT_SERVICE_THREAD_NO_LOCK);
    }

    cmiCloseLink(sLink);

    return IDE_FAILURE;
}

/* PROJ-2108 Dedicated thread mode which uses less CPU */
void mmtThreadManager::stopExpandedIdleServiceThreads()
{
    mmtServiceThread *sThread;
    iduListNode      *sIterator;
    iduListNode      *sNodeNext;
    UInt              sNumberofThreads; 
    UInt              sInitThreads;
    UInt              sCount = 0;
    lock();
    sNumberofThreads = getServiceThreadSocketCount();
    sInitThreads = mmuProperty::getDedicatedThreadInitCount();
    sCount = sNumberofThreads - sInitThreads;
    IDU_LIST_ITERATE_BACK_SAFE(&mServiceThreadSocketList, sIterator, sNodeNext)
    {
        if ( sCount <= 0 )
        {
            break;
        }
        sThread = (mmtServiceThread *)sIterator->mObj;
        if ( sThread->getAssignedTasks() == 0 )
        {
            sThread->stop();
            sThread->signalToServiceThread(sThread);
            sCount--;
        }
    }
    unlock();
}

void mmtThreadManager::checkServiceThreads()
{
    iduList           sFreeThreadList;
    iduList           sBusyThreadList;
    iduList           sIdleThreadList;
    iduList           sTaskList;
    iduListNode      *sIterator;
    iduListNode      *sNodeNext;
    mmcTask          *sTask;
    mmtServiceThread *sThread;
    UInt              sThrCnt = 0;
    UInt              sSumOfTaskCnt = 0;
    // a busy thread 의 list에 달려 있는 task 갯수 .
    UInt              sSumOfTaskCntOnBusyThrLst = 0;
    UInt              sSumOfTaskCntOnIdleThr = 0;
    UInt              sTaskCnt    = 0;
    UInt              sIdleThrCnt = 0;
    UInt              sAvgTaskCntPerThr = 0;
    UInt              sAvgTaskCntPerIdleThr = 0;
    UInt              sCntForNewThr;

    /* BUG-45274 */
    UInt              sPreAddTaskCount = 0;

    IDU_LIST_INIT(&sFreeThreadList);
    IDU_LIST_INIT(&sBusyThreadList);
    IDU_LIST_INIT(&sIdleThreadList);
    /*
     * Free List에 달린 Thread를 Free한다.
     *
     * 서비스 쓰레드가 종료할 때 쓰레드가 가지고 있던 태스크들을
     * 다른 쓰레드로 옮기기 위해 addTask하면서 lock-unlock을 반복하므로
     * thr_join을 하는 freeServiceThreads함수를 unlock하고 수행해야 한다.
     * 이를 위해 Free List를 옮겨놓고 unlock한다.
     */

    
    lock();

    IDU_LIST_JOIN_LIST(&sFreeThreadList, &mServiceThreadFreeList);

    unlock();

    freeServiceThreads(&sFreeThreadList);

    /*
     * 모든 Thread들을 검사하여 다음을 수행한다.
     *
     * 1. Idle Thread의 수를 MULTIPEXLING_THREAD_COUNT로 유지 (나머지는 stop시킴)
     * 2. Idle Thread중 Task 수가 가장 많은 Thread와 가장 적은 Thread를 판별
     * 3. Busy Thread List, Idle Thread List를 구성
     *
     * 수행 중 서비스 쓰레드에 호출하는 함수는 deadlock을 방지하고 수행시간을
     * 줄이기 위해 lock이 아니라 trylock을 사용하는 함수만 사용해야 한다.
     */

    lock();

    IDU_LIST_ITERATE(&mServiceThreadSocketList, sIterator)
    {
        sThread = (mmtServiceThread *)sIterator->mObj;
        sThrCnt++;
        sTaskCnt  =  sThread->getAssignedTasks();        
        sSumOfTaskCnt += sTaskCnt;
        
        if (sThread->checkBusy() == ID_TRUE)
        {
             /* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
                fix BUG-29117 approach to load-balance is too naive when there is no idle thread,
                fix BUG-29144 approach to load-balance is too naive when there is a few idle thread
                
                thread당 assign된 average task count,
                SUM(busy thread의 task list에 있는 task 개수),
                idle thread 갯수 ,
                thread 총개수를 구한다.
              */
            //execute하고 있는 task는 제외하기위하여.
            if(sTaskCnt > 0)
            {
                
                sSumOfTaskCntOnBusyThrLst += (sTaskCnt - 1);
            }
            if(sThread->getLifeSpan() == 0)
            {   
                sThread->incLifeSpan(mmuProperty::getServiceThrInitialLifeSpan());
            }
            IDU_LIST_ADD_LAST(&sBusyThreadList, sThread->getCheckThreadListNode());
        }
        else
        {
            // 현재 쓰레드는 IDLE한 쓰레드이다.
            sSumOfTaskCntOnIdleThr += sTaskCnt;
            sIdleThrCnt++;
            /* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
               fix BUG-29238 The list of idle thread should be sorted by a degree of busy
            */
            insert2IdleThrList(&sIdleThreadList,sThread);
            if( (sTaskCnt  < mmuProperty::getMinTaskCntForThrLive()) && (sThread->getState() == MMT_SERVICE_THREAD_STATE_POLL) )
            {
                sThread->decLifeSpan();
            }
        }//else
    }//IDU_LIST_ITERATE

    /* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
       service thread 당 평균 Task갯수 를 구한다.
    */
    //fix BUG-29514 code-sonar mmXXX serires.
    IDE_TEST_CONT(sThrCnt == 0, nextLoadBalance);
    sAvgTaskCntPerThr     = (sSumOfTaskCnt / sThrCnt);
    
    if (IDU_LIST_IS_EMPTY(&sIdleThreadList) == ID_TRUE)
    {
        /*
         * 모든 Thread가 Busy하면 Thread를 더 만든다.
         */
        IDE_TEST_CONT(sSumOfTaskCntOnBusyThrLst == 0, nextLoadBalance);
        IDE_TEST_CONT( sSumOfTaskCnt <= sThrCnt,nextLoadBalance);
        //fix BUG-29117,approach to load-balance is too naive when there is no idle thread.

        /* BUG-45274 reference http://nok.altibase.com/pages/viewpage.action?pageId=40570104  */
        ideLog::logLine(IDE_LB_1, "L6 Every ServiceThread is Busy, so it creates NewServiceThread");

        sCntForNewThr = (sSumOfTaskCntOnBusyThrLst/ sAvgTaskCntPerThr);

        ideLog::logLine(IDE_LB_1, "L6 NewServiceThreadCount(%"ID_UINT32_FMT") = SumOfTaskCntOnBusyThrLst(%"ID_UINT32_FMT") / AvgTaskCntPerThr(%"ID_UINT32_FMT")",
                        sCntForNewThr, sSumOfTaskCntOnBusyThrLst, sAvgTaskCntPerThr);

        if(sCntForNewThr == 0)
        {
            sCntForNewThr = 1;
            ideLog::logLine(IDE_LB_1, "L6 Change to 1 because NewServiceThreadCount is 0");
        }//if
        // 필요이상으로 thread 갯수 생성을 방지하기 위하여  보정한다.
        IDE_TEST_CONT(adjustCountForNewThr(sThrCnt, sSumOfTaskCnt ,&sCntForNewThr) != IDE_SUCCESS,
                       nextLoadBalance);
        IDE_TEST(createNewThr(sThrCnt,sCntForNewThr) != IDE_SUCCESS);
    }
    else
    {
        /*
           idle threads간에 task 대기열의 균형을 맞추기 위하여 ,
           task 분배를 수행한다.
        */
        distributeTasksAmongIdleThreads(&sIdleThreadList);
        /* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
           busy thread list가 비어있거 나, 비어있지않더라도 busy thread의 task가 없는 경우에
           skip하고, assigned된 task가 한개도 없는 a service thread를
           종료시킨다.
        */
        if(IDU_LIST_IS_EMPTY(&sBusyThreadList) == ID_TRUE ||(sSumOfTaskCntOnBusyThrLst == 0))
        {
            tryToStopIdleThreads(sThrCnt,&sIdleThreadList);
        }
        else
        {

            /* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
               a idle thread에 할당된 평균 average task갯수를 구한다.
            */
            //fix BUG-29514 code-sonar mmXXX serires.
            // idle thread list가 empty가 아니기때문에 divide by zero가 될수 없지만,
            // conde-sonar를 위해 방어코드를 넣는다.
            IDE_TEST_CONT(sIdleThrCnt  == 0 ,nextLoadBalance);
            sAvgTaskCntPerIdleThr = (sSumOfTaskCntOnBusyThrLst + sSumOfTaskCntOnIdleThr)  / sIdleThrCnt;
            /* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
               a idle thread에 할당된 평균 average task갯수가 a service thread에 할당된 평균 task갯수보다
               큰지를 확인
            */
            if(sAvgTaskCntPerIdleThr > sAvgTaskCntPerThr)
            {
                // 추가로 service thread를 생성할지를 판단.
                if(needToCreateThr(sSumOfTaskCnt,sThrCnt,sAvgTaskCntPerIdleThr,sAvgTaskCntPerThr ) == ID_TRUE)
                {
                    /* BUG-45274 reference http://nok.altibase.com/pages/viewpage.action?pageId=40570104  */
                    ideLog::logLine(IDE_LB_1, "L9 Too many Task's allocated to IdleServiceThread.");

                    // 새로 생성할 thread갯수에서 기존 확보한 idle thread는 고려해야 한다.
                    sCntForNewThr = (((sSumOfTaskCntOnBusyThrLst+ sSumOfTaskCntOnIdleThr)/ sAvgTaskCntPerThr) - sIdleThrCnt);

                    ideLog::logLine(IDE_LB_1, "L9 NewServiceThreadCount(%"ID_UINT32_FMT") = ((SumOfTaskCntOnBusyThrLst(%"ID_UINT32_FMT") + SumOfTaskCntOnIdleThr(%"ID_UINT32_FMT"))/ AvgTaskCntPerThr(%"ID_UINT32_FMT")) - IdleThrCnt(%"ID_UINT32_FMT")",
                                            sCntForNewThr,
                                            sSumOfTaskCntOnBusyThrLst,
                                            sSumOfTaskCntOnIdleThr,
                                            sAvgTaskCntPerThr,
                                            sIdleThrCnt);

                    if(sCntForNewThr >  0)
                    {
                        // 필요이상으로 thread 갯수 생성을 방지하기 위하여  보정한다.
                        if( adjustCountForNewThr(sThrCnt, sSumOfTaskCnt ,&sCntForNewThr) == IDE_SUCCESS)
                        {
                            IDE_TEST(createNewThr(sThrCnt,sCntForNewThr) != IDE_SUCCESS);
                            IDE_CONT(nextLoadBalance);
                        }
                    }
                }
            }//if        
            /*
             * Busy Thread의 Task를 Idle Thread로 분배한다.
             */
            IDU_LIST_INIT(&sTaskList);
            
            IDU_LIST_ITERATE(&sBusyThreadList, sIterator)
            {
                sThread = (mmtServiceThread *)sIterator->mObj;
                if(sThread->needToRemove() == ID_TRUE)
                {
                    sThread->removeAllTasks(&sTaskList, MMT_SERVICE_THREAD_LOCK);
                }
            }
            sThread = (mmtServiceThread *)sIdleThreadList.mNext->mObj;
            sPreAddTaskCount = sThread->getAssignedTasks();
            IDU_LIST_ITERATE_SAFE(&sTaskList, sIterator, sNodeNext)
            {
                sTask = (mmcTask *)sIterator->mObj;
                ID_SERIAL_BEGIN(sThread->addTask(sTask));
                ID_SERIAL_END(sTaskCnt  =  sThread->getAssignedTasks());
                sThread->increaseInTaskCount(MMT_SERVICE_THREAD_RUN_IN_BUSY,1);
                //fix BUG-29578 A load balancer should make an effort to saving cost task-migration among idle threads.
                if(sTaskCnt >= sAvgTaskCntPerThr)
                {  
                    /* BUG-45274 reference http://nok.altibase.com/pages/viewpage.action?pageId=40570104  */
                    if( sTaskCnt > sPreAddTaskCount)
                    {
                        ideLog::logLine(IDE_LB_2, "L11 increase Task count(%"ID_UINT32_FMT" --> %"ID_UINT32_FMT") in ServiceThread(%"ID_UINT32_FMT")",
                                        sPreAddTaskCount,
                                        sTaskCnt,
                                        sThread->getServiceThreadID());
                    }

                    sThread = (mmtServiceThread *)sThread->getCheckThreadListNode()->mNext->mObj;
                    if (sThread == NULL)
                    {
                        sThread = (mmtServiceThread *)sIdleThreadList.mNext->mObj;
                    }//if
                    sPreAddTaskCount = sThread->getAssignedTasks();
                }
            }//IDU_LIST_ITERATE_SAFE
        }//inner-else
    }//outer-elase
    
    IDE_EXCEPTION_CONT(nextLoadBalance);
    
    unlock();

    return;
    
    IDE_EXCEPTION_END;

    unlock();
}

/* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
   aCntForNewThr갯수 만큼 service thread를 생성한다.
*/

IDE_RC  mmtThreadManager::createNewThr(UInt aThrCnt,
                                       SInt aCntForNewThr)
{

    UInt              sAllocCntOfThr;
 
    /* bug-31833: server can create threads as MAX_THREAD_COUNT-1, max.
     * A equal sign has to be added in the comparison expression. */
    if ( (aThrCnt + aCntForNewThr) <= mmuProperty::getMultiplexingMaxThreadCount())
    {
        IDE_TEST_RAISE(startServiceThread(MMC_SERVICE_THREAD_TYPE_SOCKET,
                                          aCntForNewThr,
                                          &sAllocCntOfThr)
                       != IDE_SUCCESS, ThreadStartFail);
        mServiceThreadMgrInfo.mAddThrCount += sAllocCntOfThr;
        mServiceThreadMgrInfo.mReservedThrCnt += sAllocCntOfThr;
    }
    else
    {
        /* BUG-45274 reference http://nok.altibase.com/pages/viewpage.action?pageId=40570104  */
        ideLog::logLine(IDE_LB_1, "L10 CurrentServiceThreadCount(%"ID_UINT32_FMT") + NewServiceThreadCount(%"ID_INT32_FMT") > MaxServiceThreadCount(%"ID_UINT32_FMT")",
                        aThrCnt, aCntForNewThr, mmuProperty::getMultiplexingMaxThreadCount());
    }
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(ThreadStartFail);
    {
        mmtThreadManager::logError(MM_TRC_THREAD_MANAGER_START_THREAD_FAILED);
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}
/* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
  필요이상으로 thread 갯수 생성을 방지하기 위하여  보정한다.
*/
IDE_RC  mmtThreadManager::adjustCountForNewThr(UInt  aThrCnt,
                                               UInt  aSumOfTaskCnt,
                                               UInt *aCntForNewThr)
{

    UInt sCntForNewThr = *aCntForNewThr;
    /* BUG-45274 */
    UInt sPreForNewThr = 0;
    /*fix BUG-29850,
      The count of service thread should be controlled by MIN_TASK_COUNT_FOR_THRED_LIVE property. */
    UInt sMaxThrCnt = IDL_MIN(aSumOfTaskCnt,mmuProperty::getMultiplexingMaxThreadCount());
    
    sPreForNewThr =  sCntForNewThr;
    sCntForNewThr =  sCntForNewThr - mServiceThreadMgrInfo.mReservedThrCnt;
    /* BUG-45274 reference http://nok.altibase.com/pages/viewpage.action?pageId=40570104  */
    ideLog::logLine(IDE_LB_1, "L10 NewServiceThreadCount(%"ID_UINT32_FMT") = NewServiceThreadCount(%"ID_UINT32_FMT") - ReservedThrCnt(%"ID_UINT32_FMT")",
                    sCntForNewThr, sPreForNewThr, mServiceThreadMgrInfo.mReservedThrCnt);
    IDE_TEST_RAISE( sCntForNewThr  <= 0 , FormulaError1);
    /*fix BUG-29850,
      The count of service thread should be controlled by MIN_TASK_COUNT_FOR_THRED_LIVE property. */
    IDE_TEST_RAISE( aThrCnt  >=  sMaxThrCnt, FormulaError2);
    
    if( (aThrCnt + sCntForNewThr) >  sMaxThrCnt )
    {
        sPreForNewThr =  sCntForNewThr;
        sCntForNewThr -= (aThrCnt + sCntForNewThr) - sMaxThrCnt;
        ideLog::logLine(IDE_LB_1, "L10 NewServiceThreadCount(%"ID_UINT32_FMT") = NewServiceThreadCount(%"ID_UINT32_FMT") - (ThrCnt(%"ID_UINT32_FMT") + NewServiceThreadCount(%"ID_UINT32_FMT")) - MaxThrCnt(%"ID_UINT32_FMT"))",
                        sCntForNewThr, sPreForNewThr, aThrCnt, sPreForNewThr, sMaxThrCnt);

    }
    IDE_TEST_RAISE( sCntForNewThr  <= 0, FormulaError1);

    *aCntForNewThr = sCntForNewThr;
    /* BUG-45274 reference http://nok.altibase.com/pages/viewpage.action?pageId=40570104  */
    ideLog::logLine(IDE_LB_1, "L10 final adjust NewServiceThreadCount(%"ID_UINT32_FMT")", sCntForNewThr);

    return IDE_SUCCESS;

    /* BUG-45274 reference http://nok.altibase.com/pages/viewpage.action?pageId=40570104  */
    IDE_EXCEPTION(FormulaError1);
    {
        ideLog::logLine(IDE_LB_1, "L10 NewServiceThreadCount(%"ID_UINT32_FMT") <= 0", sCntForNewThr);
    }
    IDE_EXCEPTION(FormulaError2);
    {
        ideLog::logLine(IDE_LB_1, "L10 NewServiceThreadCount(%"ID_UINT32_FMT") >= MaxThrCnt(%"ID_UINT32_FMT")",
                        sCntForNewThr, sMaxThrCnt);
    }
    IDE_EXCEPTION_END;

    ideLog::logLine(IDE_LB_1, "L10 ServiceThread cancel creation");

    return IDE_FAILURE;
}

/* Idle thraed 중
 * Task수가 가장 많은 Thread에서 가장 적은 Thread로 Task를 나눠준다.
 *
 * 옮기는 태스크 수(x)
 *
         * a = sThreadMax->getTaskCount()
         * b = sThreadMin->getTaskCount()
         *
         * x = a - (a + b + 1) / 2 = (a - b - 1) / 2
*/
void mmtThreadManager::distributeTasksAmongIdleThreads(iduList *aIdleThreadList)
{

    iduList           sTaskList;
    mmtServiceThread *sIdleThreadBusyDegreeMin ;
    mmtServiceThread *sIdleThreadBusyDegreeMax;
    iduListNode      *sThrMinNode;
    iduListNode      *sThrMaxNode;
    UInt              sRemovedTaskCount;

    
    IDU_LIST_INIT(&sTaskList);
    
    sThrMaxNode  = IDU_LIST_GET_LAST(aIdleThreadList);
    sIdleThreadBusyDegreeMax   = (mmtServiceThread *)sThrMaxNode->mObj;
    sThrMinNode =  IDU_LIST_GET_FIRST(aIdleThreadList);
    sIdleThreadBusyDegreeMin = (mmtServiceThread *)sThrMinNode->mObj;
    
    if (sIdleThreadBusyDegreeMax != sIdleThreadBusyDegreeMin)
    {
        
        sIdleThreadBusyDegreeMax->removeFewTasks(&sTaskList,
                                                 sIdleThreadBusyDegreeMin->getAssignedTasks(),
                                                 &sRemovedTaskCount);
        if(sRemovedTaskCount > 0)
        {
            sIdleThreadBusyDegreeMin->addTasks(&sTaskList,sRemovedTaskCount);
            sIdleThreadBusyDegreeMin->increaseInTaskCount(MMT_SERVICE_THREAD_RUN_IN_IDLE,sRemovedTaskCount);
            
            /* BUG-45274 reference http://nok.altibase.com/pages/viewpage.action?pageId=40570104  */
            ideLog::logLine(IDE_LB_2, "L7 Move Tasks(%"ID_UINT32_FMT") from MaxTaskIdleThread(id:%"ID_UINT32_FMT",taskCount:%"ID_UINT32_FMT") to MinTaskIdleThread(id:%"ID_UINT32_FMT",taskCount:%"ID_UINT32_FMT")",
                            sRemovedTaskCount,
                            sIdleThreadBusyDegreeMax->getServiceThreadID(), sIdleThreadBusyDegreeMax->getAssignedTasks(),
                            sIdleThreadBusyDegreeMin->getServiceThreadID(), sIdleThreadBusyDegreeMin->getAssignedTasks());

        }
    }
}


IDE_RC mmtThreadManager::threadSleepCallback(ULong /*_aMicroSec_*/, idBool *aWaited)
{
    *aWaited = ID_FALSE;

    return IDE_SUCCESS;
}

IDE_RC mmtThreadManager::threadWakeupCallback()
{
    return IDE_SUCCESS;
}

IDE_RC mmtThreadManager::dispatchCallback(cmiLink *aLink, cmiDispatcherImpl aDispatcherImpl)
{
    struct sockaddr_storage  sAddr;
    idBool   sIPAllowed = ID_TRUE;
    //fix BUG-18025
    cmiLink *sLinkPeer = NULL;
    mmcTask *sTask     = NULL;
    idBool   sIsRemoteIP = ID_FALSE;
    idBool   sLocked   = ID_FALSE;
    /* BUG-46787 */
    SChar    sAddrStr[IDL_IP_ADDR_MAX_LEN] = {'\0', };

    // fix BUG-28140
    // accept시 ECONNABORTED 에러는 무시하고, 다음 connection 요청을 처리한다.
    // 윈도우 : WSAECONNRESET, POSIX : ECONNABORTED
    if (cmiAcceptLink(aLink, &sLinkPeer) != IDE_SUCCESS)
    {
        /* bug-35406: infinite loop when accept is failed
         * socket file을 열지 못하면 무한 루프가능하므로
         * 잠시 sleep 하고 재시도 한다 */
        if ((errno == EMFILE) || (errno == ENFILE))
        {
            idlOS::sleep(1);
        }

        IDE_TEST(errno != ECONNABORTED);
    }

    if (sLinkPeer != NULL)
    {
        /* BUG-44530 SSL에서 ALTIBASE_SOCK_BIND_ADDR 지원 */
        if ( (sLinkPeer->mImpl == CMI_LINK_IMPL_TCP) ||
             (sLinkPeer->mImpl == CMI_LINK_IMPL_SSL) ||
             (sLinkPeer->mImpl == CMI_LINK_IMPL_IB) )
        {
             /* proj-1538 ipv6
              * if tcp used and ipacl entries exist, then check */
            mmuAccessList::lock();
            sLocked = ID_TRUE;
            if ( mmuAccessList::getIPACLCount() > 0 )
            {
                (void) cmiCheckRemoteAccess(sLinkPeer, &sIsRemoteIP);
                if (sIsRemoteIP == ID_TRUE)
                {
                    idlOS::memset(&sAddr, 0x00, ID_SIZEOF(sAddr));
                    IDE_TEST(cmiGetLinkInfo(sLinkPeer, (SChar *)&sAddr, ID_SIZEOF(sAddr),
                                            CMI_LINK_INFO_REMOTE_SOCKADDR)
                            != IDE_SUCCESS);

                    /* BUG-46787 get ip address */
                    IDE_TEST(cmiGetLinkInfo(sLinkPeer, (SChar *)sAddrStr, ID_SIZEOF(sAddrStr),
                                            CMI_LINK_INFO_REMOTE_IP_ADDRESS)
                            != IDE_SUCCESS);

                    (void) mmuAccessList::checkIPACL(&sAddr, sAddrStr, &sIPAllowed);

                    IDE_TEST(sIPAllowed == ID_FALSE);
                }
            }
            sLocked = ID_FALSE;
            mmuAccessList::unlock();
        }

        IDE_TEST(cmiSetLinkBlockingMode(sLinkPeer, ID_FALSE) != IDE_SUCCESS);

        IDE_TEST(mmtSessionManager::allocTask(&sTask) != IDE_SUCCESS);

        IDE_TEST(sTask->setLink(sLinkPeer) != IDE_SUCCESS);

        switch (aDispatcherImpl)
        {
            case CMN_DISPATCHER_IMPL_SOCK:
            case CMN_DISPATCHER_IMPL_IB:
                if ( mmuProperty::getIsDedicatedMode() == 0 )
                {
                    IDE_TEST(addSocketTask(sTask) != IDE_SUCCESS);
                }
                /* PROJ-2108 Dedicated thread mode which uses less CPU */
                else
                {
                    IDE_TEST(addDedicatedTask(sTask) != IDE_SUCCESS);
                }
                break;

            case CMN_DISPATCHER_IMPL_IPC:
                IDE_TEST(addIpcTask(sTask) != IDE_SUCCESS);
                break;

            case CMN_DISPATCHER_IMPL_IPCDA:    /*PROJ-2616*/
                IDE_TEST(addIPCDATask(sTask) != IDE_SUCCESS);
                break;

            default:
                IDE_ASSERT(0);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        if (sLocked == ID_TRUE)
        {
            mmuAccessList::unlock();
        }

        if (sTask != NULL)
        {
            mmtSessionManager::freeTask(sTask);
        }
        else
        {
            if (sLinkPeer != NULL)
            {
                cmiFreeLink(sLinkPeer);
            }
        }
    }

    return IDE_FAILURE;
}

void mmtThreadManager::serviceThreadStarted(mmtServiceThread    *aServiceThread,
                                            mmcServiceThreadType aServiceThreadType,
                                            UInt                *aServiceThreadID)
{
    
    ID_SERIAL_BEGIN(lock());

    mServiceThreadLastID++;

    if (mServiceThreadLastID == 0)
    {
        /* PROJ-2108 Dedicated thread mode which uses less CPU */
        if ( mmuProperty::getIsDedicatedMode() == 0 )
        {    /*PROJ-2616*/
            mServiceThreadLastID = mmuProperty::getMultiplexingThreadCount() +
                                   mmuProperty::getIpcChannelCount() +
                                   mmuProperty::getIPCDAChannelCount() + 1;
        }
        else
        {    /*PROJ-2616*/
            mServiceThreadLastID = mmuProperty::getDedicatedThreadInitCount() +
                                   mmuProperty::getIpcChannelCount() +
                                   mmuProperty::getIPCDAChannelCount() + 1;
        }
    }

    *aServiceThreadID = mServiceThreadLastID;

    switch(aServiceThreadType)
    {
        case MMC_SERVICE_THREAD_TYPE_SOCKET :
            mServiceThreadSocketCount++;
            if (mServiceThreadLastID > mmuProperty::getMultiplexingThreadCount())
            {
                /* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase              
                */
                ID_SERIAL_EXEC(IDU_LIST_ADD_FIRST(&mServiceThreadSocketList,
                                                  aServiceThread->getThreadListNode()),1);
                if(mServiceThreadMgrInfo.mReservedThrCnt > 0 )
                {
                    
                    mServiceThreadMgrInfo.mReservedThrCnt--;
                }
            }
            else
            {    
                ID_SERIAL_EXEC(IDU_LIST_ADD_LAST(&mServiceThreadSocketList,
                                                 aServiceThread->getThreadListNode()),1);
            }
            
            break;
        /* PROJ-2108 Dedicated thread mode which uses less CPU */
        case MMC_SERVICE_THREAD_TYPE_DEDICATED :
            mServiceThreadSocketCount++;
            ID_SERIAL_EXEC(IDU_LIST_ADD_LAST(&mServiceThreadSocketList,
                                             aServiceThread->getThreadListNode()),1);
            break;

        case MMC_SERVICE_THREAD_TYPE_IPC :
            ID_SERIAL_EXEC(IDU_LIST_ADD_LAST(&mServiceThreadIPCList, aServiceThread->getThreadListNode()),1);
            mIpcServiceThreadArray[mServiceThreadIPCCount] = aServiceThread;

            aServiceThread->setIpcID(mServiceThreadIPCCount);
            mServiceThreadIPCCount++;

            break;
        case MMC_SERVICE_THREAD_TYPE_IPCDA :    /*PROJ-2616*/
            ID_SERIAL_EXEC(IDU_LIST_ADD_LAST(&mServiceThreadIPCDAList, aServiceThread->getThreadListNode()),1);
            mIPCDAServiceThreadArray[mServiceThreadIPCDACount] = aServiceThread;

            aServiceThread->setIpcID(mServiceThreadIPCDACount);
            mServiceThreadIPCDACount++;

            break;
        //fix PROJ-1749
        case MMC_SERVICE_THREAD_TYPE_DA :
            break;
        default:
            IDE_ASSERT(0);
    }


    /*
     * 통계 정보 기록: 운영중 Service Thread가 생성된 갯수
     */

    IDV_SYS_ADD(IDV_STAT_INDEX_SERVICE_THREAD_CREATED, 1);
    ID_SERIAL_END(unlock());

    /* BUG-45274 reference http://nok.altibase.com/pages/viewpage.action?pageId=40570104  */
    ideLog::logLine(IDE_LB_1, "Started NewServiceThread(%"ID_UINT32_FMT")", aServiceThread->getServiceThreadID());
}

/* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
   service thread갯수가 multiplexing thread 갯수와 Gap이 SERVICE_THREAD_EXIT_RATE
   이상  벌어지면 life span을 모두 소모하고,
   assign된 task가 하나도 없는 service thread를 종료시킨다.
*/
void mmtThreadManager::tryToStopIdleThreads(UInt     aThrCnt,
                                            iduList *aIdleThreadList)
{
    iduListNode      *sIterator;
    mmtServiceThread *sThread;
    UInt              sThreadCntGapRate;
    
    

    if(aThrCnt  > mmuProperty::getMultiplexingThreadCount() )
    {
        sThreadCntGapRate =  (aThrCnt  * 100) / mmuProperty::getMultiplexingThreadCount();
        if(sThreadCntGapRate >=  mmuProperty::getSerivceThrExitRate())
        {   
            IDU_LIST_ITERATE(aIdleThreadList,sIterator)
            {
                sThread = (mmtServiceThread *)sIterator->mObj;
                if(sThread->getLifeSpan() == 0)
                {
                    if(sThread->getAssignedTasks() < mmuProperty::getMinTaskCntForThrLive())
                    {
                        mServiceThreadMgrInfo.mRemoveThrCount += 1;            
                        /* BUG-38384 A task in a service thread can be a orphan */
                        sThread->setRemoveAllTasks(ID_TRUE);
                        sThread->stop();
                        break;
                    }
                    else
                    {
                        sThread->incLifeSpan((mmuProperty::getServiceThrInitialLifeSpan()  /2));
                    }
                }//if
            }//IDU_LIST_ITERATE
        }
    }//if
}

/* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
   fix BUG-29238 The list of idle thread should be sorted by a degree of busy
*/
void mmtThreadManager::insert2IdleThrList(iduList           *aIdleThreadList,
                                          mmtServiceThread  *aIdleThread)
{
    iduListNode      *sIterator;
    iduListNode      *sNodeNext;
    mmtServiceThread *sThread;
    
    IDU_LIST_ITERATE_SAFE(aIdleThreadList, sIterator,sNodeNext)
    {
        sThread = (mmtServiceThread *)sIterator->mObj;
        
        if(aIdleThread->getBusyDegree(ID_FALSE) < sThread->getBusyDegree(ID_FALSE))
        {
            IDU_LIST_ADD_BEFORE(sIterator,aIdleThread->getCheckThreadListNode());
            break;
        }//if
    }//IDU_LIST_ITERATE
    // max.
    if( sIterator == aIdleThreadList)
    {
        IDU_LIST_ADD_LAST(aIdleThreadList, aIdleThread->getCheckThreadListNode());
    }
}

//fix BUG-19464 
void mmtThreadManager::addServiceThread(mmtServiceThread    *aServiceThread)
{
    
    mServiceThreadLastID++;

    if (mServiceThreadLastID == 0)
    {
        /*PROJ-2616*/
        mServiceThreadLastID = mmuProperty::getMultiplexingThreadCount() +
                               mmuProperty::getIpcChannelCount() +
                               mmuProperty::getIPCDAChannelCount() + 1;
    }
    aServiceThread->setServiceThreadID(mServiceThreadLastID);
    
    
    IDU_LIST_ADD_LAST(&mServiceThreadSocketList,
                      aServiceThread->getThreadListNode());
    
    /*
     * 통계 정보 기록: 운영중 Service Thread가 생성된 갯수
     */

    IDV_SYS_ADD(IDV_STAT_INDEX_SERVICE_THREAD_CREATED, 1);    
}

void mmtThreadManager::noServiceThreadStarted(mmtServiceThread    *  /*aServiceThread*/,
                                              mmcServiceThreadType   /*aServiceThreadType*/,
                                              UInt                *  /*aServiceThreadID*/)
    
{
    //nothing to do.
}

/* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
   fix BUG-29144 approach to load-balance is too naive when there is a few idle thread
   load balance과정에서 추가적으로  idle thread갯수를
   생성할지를 판단하는 함수.

   a idle thread에 할당된 평균 average task갯수가
   a service thread당 평균 Task갯수보다  크고,
   그 비율이 NEW_SERVICE_CREATE_RATE 이상 벌어졌고,
   그 편차가 NEW_SERVICE_CREATE_RATE_GAP 이상 벌어졌을때
   추가적으로
   service thread를  생성하도록 결정한다.
   
*/
idBool  mmtThreadManager::needToCreateThr(UInt  aSumOfTaskCnt,
                                          UInt  aThrCnt,
                                          UInt  aAvgTaskCntPerIdleThr,
                                          UInt  aAvgTaskCntPerThr )
{
    UInt  sRateOfGap;
    UInt  sGap;
    idBool sRetValue;

    if( aThrCnt  >= aSumOfTaskCnt)
    {
        sRetValue = ID_FALSE;
    }
    else
    {
        /*fix BUG-29850,
         The count of service thread should be controlled by MIN_TASK_COUNT_FOR_THRED_LIVE property. */
        if(aAvgTaskCntPerIdleThr <= mmuProperty::getMinTaskCntForThrLive())
        {
            sRetValue = ID_FALSE;
        }
        else
        {
            /*fix BUG-29850,
              The count of service thread should be controlled by MIN_TASK_COUNT_FOR_THRED_LIVE property. */
            if(aAvgTaskCntPerThr < mmuProperty::getMinTaskCntForThrLive())
            {
                sRetValue = ID_FALSE;
            }
            else
            {
            
                sRateOfGap = ((aAvgTaskCntPerIdleThr  * 100) /   aAvgTaskCntPerThr);
                if(sRateOfGap >= mmuProperty::getNewServiceThrCreateRate())
                {

                    sGap = (aAvgTaskCntPerIdleThr - aAvgTaskCntPerThr);
                    if(sGap >  mmuProperty::getNewServiceThrCreateRateGap())
                    {    
                        /* BUG-45274 reference http://nok.altibase.com/pages/viewpage.action?pageId=40570104  */
                        ideLog::logLine(IDE_LB_1, "L9 AvgTaskCntPerIdleThr(%"ID_UINT32_FMT") > AvgTaskCntPerThr(%"ID_UINT32_FMT")",
                                        aAvgTaskCntPerIdleThr, aAvgTaskCntPerThr);
                        ideLog::logLine(IDE_LB_1, "L9 AvgTaskCntPerIdleThr(%"ID_UINT32_FMT") > MIN_TASK_COUNT_FOR_THREAD_LIVE(%"ID_UINT32_FMT")",
                                        aAvgTaskCntPerIdleThr, mmuProperty::getMinTaskCntForThrLive());
                        ideLog::logLine(IDE_LB_1, "L9 AvgTaskCntPerThr(%"ID_UINT32_FMT") >= MIN_TASK_COUNT_FOR_THREAD_LIVE(%"ID_UINT32_FMT")",
                                        aAvgTaskCntPerThr, mmuProperty::getMinTaskCntForThrLive());
                        ideLog::logLine(IDE_LB_1, "L9 TaskAvgRate(%"ID_UINT32_FMT") >= NEW_SERVICE_CREATE_RATE(%"ID_UINT32_FMT")",
                                        sRateOfGap, mmuProperty::getNewServiceThrCreateRate());
                        ideLog::logLine(IDE_LB_1, "L9 TaskAvgGap(%"ID_UINT32_FMT") >= NEW_SERVICE_CREATE_RATE_GAP(%"ID_UINT32_FMT")",
                                        sGap, mmuProperty::getNewServiceThrCreateRateGap());
                        sRetValue = ID_TRUE;
                    }
                    else
                    {
                        sRetValue = ID_FALSE;
                    }
                }
                else
                {
                    sRetValue = ID_FALSE;
                }
            }
        }//else
    }
    return sRetValue;
}
void mmtThreadManager::serviceThreadStopped(mmtServiceThread *aServiceThread)
{
    iduList              sTaskList;
    iduListNode         *sIterator;
    iduListNode          *sNodeNext;
    mmcTask              *sTask;
    mmcServiceThreadType  sThrType;
    

    IDU_LIST_INIT(&sTaskList);
    sThrType = aServiceThread->getServiceThreadType();
    
    ID_SERIAL_BEGIN(lock());

    switch (sThrType)
    {
        case MMC_SERVICE_THREAD_TYPE_SOCKET:
            /* fix BUG-30322 In case of terminating a transformed dedicated service thread,
            the count of service can be negative . */
            /* dedicated service thread종료시 shared service list 의 service thread count
             를 마이너스를 하면 안된다.*/
            if ( (aServiceThread->isRunModeShared() ==  ID_TRUE))
            {
                mServiceThreadSocketCount--;
            }
            else
            {
                //nothing to do
            }
            break;
            
        /* PROJ-2108 Dedicated thread mode which uses less CPU */
        case MMC_SERVICE_THREAD_TYPE_DEDICATED:
            mServiceThreadSocketCount--;
            break;
        case MMC_SERVICE_THREAD_TYPE_IPC:
            mServiceThreadIPCCount--;
            break;
        case MMC_SERVICE_THREAD_TYPE_IPCDA:    /*PROJ-2616*/
            mServiceThreadIPCDACount--;
            break;
        //fix PROJ-1749
        case MMC_SERVICE_THREAD_TYPE_DA:
            break;
        default:
            IDE_ASSERT(0);
    }

    aServiceThread->removeAllTasks(&sTaskList, MMT_SERVICE_THREAD_NO_LOCK);

    ID_SERIAL_EXEC(IDU_LIST_REMOVE(aServiceThread->getThreadListNode()),1);

    ID_SERIAL_EXEC(IDU_LIST_ADD_LAST(&mServiceThreadFreeList, aServiceThread->getThreadListNode()),2);

    ID_SERIAL_END(unlock());

    IDU_LIST_ITERATE_SAFE(&sTaskList, sIterator, sNodeNext)
    {
        sTask = (mmcTask *)sIterator->mObj;

        if (addSocketTask(sTask) != IDE_SUCCESS)
        {
            mmtThreadManager::logError(MM_TRC_THREAD_MANAGER_ADD_TASK_FAILED);

            mmtSessionManager::freeTask(sTask);
        }
    }
}

IDE_RC mmtThreadManager::setupDefaultThreadSignal()
{
    sigset_t sOldSet;
    sigset_t sNewSet;

    idlOS::sigemptyset(&sNewSet);
    idlOS::sigaddset(&sNewSet, SIGINT);

    IDE_TEST(idlOS::pthread_sigmask(SIG_BLOCK, &sNewSet, &sOldSet) != 0);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

void mmtThreadManager::logError(const SChar *aMessage)
{
    ideLogEntry sLog(IDE_SERVER_0);
    ideLog::logErrorMsgInternal(sLog);
    sLog.append(aMessage);
    sLog.write();
}

void mmtThreadManager::logNetworkError(const SChar *aMessage)
{
    // BUG-24993 네트워크 에러 메시지 log 여부
    if(mmuProperty::getNetworkErrorLog() == 1)
    {
        ideLogEntry sLog(IDE_SERVER_2);
        ideLog::logErrorMsgInternal(sLog);
        sLog.append(aMessage);
        sLog.write();
    }
}

void mmtThreadManager::logMemoryError(const SChar *aMessage)
{
    ideLogEntry sLog(IDE_SERVER_3);
    ideLog::logErrorMsgInternal(sLog);
    sLog.append(aMessage);
    sLog.write();
}

idBool mmtThreadManager::logCurrentSessionInfo(ideLogEntry &aLog)
{
    mmtSessionManager::logSessionOverview(aLog);

    aLog.append(MM_TRC_CURRENT_SESSION);
    aLog.append("\n\n\n");

    return mmtSessionManager::logTaskOfThread(aLog, idlOS::getThreadID());
}

void mmtThreadManager::logDisconnectError(mmcTask *aTask, UInt aErrorCode)
{
    SChar       sMessage[MMT_MESSAGE_SIZE];
    UChar       sClientInfo[IDL_IP_ADDR_MAX_LEN];
    cmiLink    *sLink = (cmiLink *)aTask->getLink();
    mmcSession *sSession = aTask->getSession();

    if(mmuProperty::getNetworkErrorLog() == 1)
    {    
        /* get client information */
        cmiGetLinkInfo(sLink, (SChar *)sClientInfo,
                       IDL_IP_ADDR_MAX_LEN, CMI_LINK_INFO_ALL);

        /* make a message */
        idlOS::snprintf(sMessage, sizeof(sMessage), MM_TRC_ABORT_SESSION_DISCONNECTED,
                        E_ERROR_CODE(aErrorCode), errno, ideGetErrorMsg(aErrorCode),
                        sSession->getSessionID(), sClientInfo);

        /* write */
        ideLogEntry sLog(IDE_SERVER_2);
        sLog.append(sMessage);
        sLog.append("\n");
        sLog.write();
    }    
}

IDE_RC mmtThreadManager::buildRecordForServiceThread(idvSQL              * /*aStatistics*/,
                                                     void *aHeader,
                                                     void * /* aDumpObj */,
                                                     iduFixedTableMemory *aMemory)
{
    mmtServiceThread *sServiceThread;
    iduListNode      *sIterator;

    lock();

    IDU_LIST_ITERATE(&mServiceThreadSocketList, sIterator)
    {
        sServiceThread = (mmtServiceThread *)sIterator->mObj;

        IDU_FIT_POINT("mmtThreadManager::buildRecordForServiceThread::lock::buildRecord");

        IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                            aMemory,
                                            (void *)sServiceThread->getInfo()) != IDE_SUCCESS);
    }

    IDU_LIST_ITERATE(&mServiceThreadIPCList, sIterator)
    {
        sServiceThread = (mmtServiceThread *)sIterator->mObj;

        IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                            aMemory,
                                            (void *)sServiceThread->getInfo()) != IDE_SUCCESS);
    }
    IDU_LIST_ITERATE(&mServiceThreadIPCDAList, sIterator)
    {    /*PROJ-2616*/
        sServiceThread = (mmtServiceThread *)sIterator->mObj;

        IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                            aMemory,
                                            (void *)sServiceThread->getInfo()) != IDE_SUCCESS);
    }
    //PROJ-1677 DEQUEUE
    IDU_LIST_ITERATE(&mTempDedicateServiceThrList, sIterator)
    {
        sServiceThread = (mmtServiceThread *)sIterator->mObj;

        IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                            aMemory,
                                            (void *)sServiceThread->getInfo()) != IDE_SUCCESS);
    }
    unlock();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    unlock();

    return IDE_FAILURE;
}
IDE_RC mmtThreadManager::buildRecordForServiceThreadMgr(idvSQL              * /*aStatistics*/,
                                                        void *aHeader,
                                                        void * /* aDumpObj */,
                                                        iduFixedTableMemory *aMemory)
{

    IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                        aMemory,
                                        (void *)&mServiceThreadMgrInfo) != IDE_SUCCESS);
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;


    return IDE_FAILURE;
}

//PROJ-1677
idBool  mmtThreadManager::try2AddServiceThread(mmtServiceThread*  aServiceThread)
{
    idBool sRet;
    lock();
    if( mServiceThreadSocketCount < mmuProperty::getMultiplexingMaxThreadCount())
    {
        mServiceThreadSocketCount++;
        // getout  from temp dedicated thread list   
        IDU_LIST_REMOVE(aServiceThread->getThreadListNode());    
        IDU_LIST_INIT_OBJ(aServiceThread->getThreadListNode(),aServiceThread);
        // re-join service thread list.  
        IDU_LIST_ADD_LAST(&mServiceThreadSocketList,aServiceThread->getThreadListNode());
        sRet = ID_TRUE;
    }
    else
    {
        sRet = ID_FALSE;
    }
    unlock();
    return sRet;
}
//PROJ-1677
IDE_RC mmtThreadManager::removeFromServiceThrList(mmtServiceThread*  aServiceThread)
{
    iduList            sTaskLst;
    mmtServiceThread   *sNewServiceThread = NULL;
    mmcTask            *sTask;
    iduListNode        *sIterator;
    //fix BUG-19405.
    iduListNode        *sNodeNext;
    UChar              sState = 0;
    
    lock();
    sState =1;

    // service thread 가 list에 나감으로써 ,최소값 제약을 깨는 경우.
    if( mServiceThreadSocketCount <= mmuProperty::getMultiplexingThreadCount())
    {

        /* BUG-31197
         * TC/FIT/LimitEnv/Bugs/BUG-31197/BUG-31197.sql
         */
        // 새로운 서비스 쓰레드를 생성한다.
        IDE_TEST(allocServiceThread(MMC_SERVICE_THREAD_TYPE_SOCKET,
                                    mmtThreadManager::noServiceThreadStarted,
                                    &sNewServiceThread) != IDE_SUCCESS);

        // task list를 move  시킨다.
        aServiceThread->moveTaskLists(sNewServiceThread);
        addServiceThread(sNewServiceThread);
                                   
        /* BUG-31197
         * TC/FIT/LimitEnv/Bugs/BUG-31197/BUG-31197-2.sql
         */
        IDE_TEST_RAISE(sNewServiceThread->start() != IDE_SUCCESS, StartFail);

        // service thread list에서 나간다.
        /* BUG-31197 remove from list when allocation and start succeeded */
        IDU_LIST_REMOVE(aServiceThread->getThreadListNode());    
        IDU_LIST_INIT_OBJ(aServiceThread->getThreadListNode(),aServiceThread);
    }
    else
    { 
        /* BUG-31197 */
        IDU_LIST_REMOVE(aServiceThread->getThreadListNode());    
        IDU_LIST_INIT_OBJ(aServiceThread->getThreadListNode(),aServiceThread); 

        //새로운 서비스 service 생성 없이 service thread list에서 나간다.
        IDU_LIST_INIT(&sTaskLst);
        //service thread가 가지고 있던 task list들을 하나의 list로
        //만든다.
        aServiceThread->removeAllTasks(&sTaskLst, MMT_SERVICE_THREAD_NO_LOCK);
        if(IDU_LIST_IS_EMPTY(&sTaskLst) == ID_FALSE)
        {
            //task들을 service thread list상에 있는 서비스 쓰레드에게 분배한다.
            IDU_LIST_ITERATE_SAFE(&sTaskLst,sIterator,sNodeNext)
            {
                sTask = (mmcTask*)sIterator->mObj;
                /* TASK-4324  Applying lessons learned from CPBS-CAESE to altibase
                */
                assignSocketTask(sTask);
            }//IDU_LIST_ITERATE
        }
        else
        {
            //nothing to do.
        }
        mServiceThreadSocketCount--;
    }
    //Add to Temp Dedicated Service List 
    IDU_LIST_ADD_LAST(&mTempDedicateServiceThrList, aServiceThread->getThreadListNode());

    unlock();
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(StartFail);
    {
        /* BUG-31197 
         * remove the added service thread when sNewServiceThread->start() failed
         */
        IDU_LIST_REMOVE(sNewServiceThread->getThreadListNode());    

        // task list 원복
        sNewServiceThread->moveTaskLists(aServiceThread);

        IDE_ASSERT(freeServiceThread(sNewServiceThread) == IDE_SUCCESS);
    }
    
    IDE_EXCEPTION_END;
    
    if(sState != 0)
    {        
        unlock();
    }
    
    return IDE_FAILURE;
}


/*
 * Fixed Table Definition for SERVICE THREAD
 */

static UInt callbackForExecuteTime(void *aBaseObj, void *aMember, UChar *aBuf, UInt /*aBufSize*/)
{
    mmtServiceThreadInfo *sInfo = (mmtServiceThreadInfo *)aBaseObj;

    IDE_ASSERT(&sInfo->mExecuteBegin == aMember);

    return mmuFixedTable::buildConvertTime(&sInfo->mExecuteBegin, &sInfo->mExecuteEnd, aBuf);
}


static iduFixedTableColDesc gServiceThreadColDesc[] =
{
    {
        (SChar *)"ID",
        offsetof(mmtServiceThreadInfo, mServiceThreadID),
        IDU_FT_SIZEOF(mmtServiceThreadInfo, mServiceThreadID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"TYPE",
        offsetof(mmtServiceThreadInfo, mServiceThreadType),
        IDU_FT_SIZEOF(mmtServiceThreadInfo, mServiceThreadType),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"STATE",
        offsetof(mmtServiceThreadInfo, mState),
        IDU_FT_SIZEOF(mmtServiceThreadInfo, mState),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"RUN_MODE",
        offsetof(mmtServiceThreadInfo,mRunMode),
        IDU_FT_SIZEOF(mmtServiceThreadInfo, mRunMode),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"SESSION_ID",
        offsetof(mmtServiceThreadInfo, mSessionID),
        IDU_FT_SIZEOF(mmtServiceThreadInfo, mSessionID),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"STATEMENT_ID",
        offsetof(mmtServiceThreadInfo, mStmtID),
        IDU_FT_SIZEOF(mmtServiceThreadInfo, mStmtID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"START_TIME",
        offsetof(mmtServiceThreadInfo, mStartTime),
        IDU_FT_SIZEOF(mmtServiceThreadInfo, mStartTime),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"EXECUTE_TIME",
        offsetof(mmtServiceThreadInfo, mExecuteBegin),
        IDU_FT_SIZEOF_UBIGINT,
        IDU_FT_TYPE_UBIGINT,
        callbackForExecuteTime,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"TASK_COUNT",
        offsetof(mmtServiceThreadInfo, mTaskCount),
        IDU_FT_SIZEOF(mmtServiceThreadInfo, mTaskCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"READY_TASK_COUNT",
        offsetof(mmtServiceThreadInfo, mReadyTaskCount),
        IDU_FT_SIZEOF(mmtServiceThreadInfo, mReadyTaskCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },

    {
        (SChar *)"IN_TASK_COUNT_FROM_IDLE",
        offsetof(mmtServiceThreadInfo,mInTaskCntFromIdle),
        IDU_FT_SIZEOF(mmtServiceThreadInfo,mInTaskCntFromIdle),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"IN_TASK_COUNT_FROM_BUSY",
        offsetof(mmtServiceThreadInfo,mInTaskCntFromBusy),
        IDU_FT_SIZEOF(mmtServiceThreadInfo,mInTaskCntFromBusy),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"OUT_TASK_COUNT_IN_IDLE_STATE",
        offsetof(mmtServiceThreadInfo,mOutTaskCntInIdle),
        IDU_FT_SIZEOF(mmtServiceThreadInfo,mOutTaskCntInIdle),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"OUT_TASK_COUNT_IN_BUSY_STATE",
        offsetof(mmtServiceThreadInfo,mOutTaskCntInBusy),
        IDU_FT_SIZEOF(mmtServiceThreadInfo,mOutTaskCntInBusy),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    
    {
        (SChar *)"BUSY_EXPERIECENCE_CNT",
        offsetof(mmtServiceThreadInfo,mBusyExperienceCnt),
        IDU_FT_SIZEOF(mmtServiceThreadInfo,mBusyExperienceCnt),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"LIFE_SPAN",
        offsetof(mmtServiceThreadInfo,mLifeSpan),
        IDU_FT_SIZEOF(mmtServiceThreadInfo,mLifeSpan),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"THREAD_ID",
        offsetof(mmtServiceThreadInfo, mThreadID),
        IDU_FT_SIZEOF(mmtServiceThreadInfo, mThreadID),
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

iduFixedTableDesc gServiceThreadTableDesc =
{
    (SChar *)"X$SERVICE_THREAD",
    mmtThreadManager::buildRecordForServiceThread,
    gServiceThreadColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

static iduFixedTableColDesc gServiceThreadMgrColDesc[] =
{
    {
        (SChar *)"ADD_THR_COUNT",
        offsetof(mmtServiceThreadMgrInfo,mAddThrCount ),
        IDU_FT_SIZEOF(mmtServiceThreadMgrInfo,mAddThrCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"REMOVE_THR_COUNT",
        offsetof(mmtServiceThreadMgrInfo,mRemoveThrCount ),
        IDU_FT_SIZEOF(mmtServiceThreadMgrInfo,mRemoveThrCount),
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

iduFixedTableDesc gServiceThreadMgrTableDesc =
{
    (SChar *)"X$SERVICE_THREAD_MGR",
    mmtThreadManager::buildRecordForServiceThreadMgr,
    gServiceThreadMgrColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

// PROJ-1697 Performance view for Protocols

/*
 * Fixed Table
 */
static iduFixedTableColDesc gDBProtocolColDesc[] =
{
    {
        (SChar *)"OP_NAME",
        offsetof(mmtDBProtocol4PerfV, mOpName),
        CMP_OP_NAME_LEN,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"OP_ID",
        offsetof(mmtDBProtocol4PerfV, mOpID),
        IDU_FT_SIZEOF(mmtDBProtocol4PerfV, mOpID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"COUNT",
        offsetof(mmtDBProtocol4PerfV, mCount),
        IDU_FT_SIZEOF(mmtDBProtocol4PerfV, mCount),
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

IDE_RC mmtThreadManager::buildRecordForDBProtocol(idvSQL              * /*aStatistics*/,
                                                  void      *aHeader,
                                                  void      * /* aDumpObj */,
                                                  iduFixedTableMemory *aMemory)
{
    UInt                i;
    mmtDBProtocol4PerfV sDBProtocol4PerfV;

    for( i = 0; i < CMP_OP_DB_MAX; i++ )
    {
        sDBProtocol4PerfV.mOpName = cmiGetOpName( CMP_MODULE_DB, i );
        sDBProtocol4PerfV.mOpID   = i;
        sDBProtocol4PerfV.mCount  = CMP_DB_PROTOCOL_STAT_GET(i);

        IDE_TEST( iduFixedTable::buildRecord( aHeader, 
                                              aMemory, 
                                              (void *)&sDBProtocol4PerfV )
                  != IDE_SUCCESS );
    }
   
    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

iduFixedTableDesc gDBProtocolDesc =
{
    (SChar *)"X$DB_PROTOCOL",
    mmtThreadManager::buildRecordForDBProtocol,
    gDBProtocolColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};


/* PROJ-2108 Dedicated thread mode which uses less CPU */
/* Get the specific core(CPU) index to bind with thread */
/* Busy count of cores are calculated by Round-Robbin method */
UInt mmtThreadManager::getCoreIdxToAssign(void)
{
    UInt sMinBusyCount = ID_UINT_MAX;

    /* BUG-35356 Memory violation may occur when calculating CPU busy count */
    UInt sLastMinBusyIdx  = 0;
    UInt i;

    lock();
    for (i = 0; i < mCoreCount; i++)
    {
        if ( mCoreInfoArray[i].mCoreBusyCount == 0 )
        {
            sLastMinBusyIdx = i;
            break;
        }
        else if ( mCoreInfoArray[i].mCoreBusyCount < sMinBusyCount )
        {
            sLastMinBusyIdx = i;
            sMinBusyCount = mCoreInfoArray[i].mCoreBusyCount;
        }
    }
    mCoreInfoArray[sLastMinBusyIdx].mCoreBusyCount++;

    unlock();
    return sLastMinBusyIdx;
}

/* BUG-35356 Memory violation may occur when calculating CPU busy count */
UInt mmtThreadManager::getCoreNumberFromIdx(UInt aSelectedCoreIdx)
{
    return mCoreInfoArray[aSelectedCoreIdx].mCoreNumber;
}

void mmtThreadManager::decreaseCoreBusyCount( UInt aSelectedCoreIdx )
{
    lock();
    mCoreInfoArray[aSelectedCoreIdx].mCoreBusyCount--;
    unlock();
}

