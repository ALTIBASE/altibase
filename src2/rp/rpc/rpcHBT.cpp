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
 

/*****************************************************************************
 * $Id: rpcHBT.cpp 84294 2018-11-07 04:56:02Z donghyun1 $
 ****************************************************************************/

#include <ide.h>
#include <idp.h>
#include <idl.h>
#include <idtBaseThread.h>

#include <rp.h>
#include <rpdLogAnalyzer.h>
#include <rpdMeta.h>
#include <rpuProperty.h>
#include <rpcHBT.h>
#include <rpcManager.h>

rpcHBT    *rpcHBT::mSelf;
iduMutex   rpcHBT::mMutex;
iduCond    rpcHBT::mCond;
idBool     rpcHBT::mKillFlag;

/*-----------------------------------------------------------------------------
 * Name :
 *     rpcHBT::rpcHBT()
 *
 * Argument :
 *    None:
 *
 * Description :
 *  Dummy func for c++;
 *
 * ---------------------------------------------------------------------------*/

rpcHBT::rpcHBT() : idtBaseThread()
{
}

/*-----------------------------------------------------------------------------
 * Name :
 *     rpcHBT::rpcHBT()
 *
 * Argument :
 *  None
 *
 * Description :
 *  Initialize all Member.
 *
 * ---------------------------------------------------------------------------*/
IDE_RC rpcHBT::initialize()
{
    UInt         sStage = 0;

    mSelf               = this;
    mKillFlag           = ID_FALSE;

    IDE_TEST_RAISE(mMutex.initialize((SChar*)"rpcHBT Mutex",
                                     IDU_MUTEX_KIND_POSIX,
                                     IDV_WAIT_INDEX_NULL)
                   != IDE_SUCCESS, ERR_MUTEX_INIT);
    sStage = 1;

    IDE_TEST_RAISE(mCond.initialize() != IDE_SUCCESS, ERR_COND_INIT);
    sStage = 2;

    IDU_LIST_INIT( &mHostRscList );

    IDE_TEST( rpnPollInitialize( &mPoll,
                                 RPU_REPLICATION_MAX_COUNT )
              != IDE_SUCCESS );
    sStage = 3;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_MUTEX_INIT);
    {
        IDE_ERRLOG(IDE_RP_1);
        IDE_SET(ideSetErrorCode(rpERR_FATAL_ThrMutexInit));
        IDE_ERRLOG(IDE_RP_1);

        IDE_CALLBACK_FATAL("[Repl HBT] Mutex initialization error");
    }
    IDE_EXCEPTION(ERR_COND_INIT);
    {
        IDE_SET(ideSetErrorCode(rpERR_FATAL_ThrCondInit));
        IDE_ERRLOG(IDE_RP_1);

        IDE_CALLBACK_FATAL("[Repl HBT] idlOS::cond_init() error");
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 3:
            rpnPollFinalize( &mPoll );
        case 2:
            (void)mCond.destroy();
        case 1:
            (void)mMutex.destroy();
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*-----------------------------------------------------------------------------
 * Name :
 *     rpcHBT::destroy()
 *
 * Argument :
 *    None:
 *
 * Description :
 *  Called when Server Shutdown.
 * ---------------------------------------------------------------------------*/
void rpcHBT::destroy()
{
    rpnPollFinalize( &mPoll );

    if(mCond.destroy() != IDE_SUCCESS)
    {
        IDE_ERRLOG(IDE_RP_1);
    }

    if(mMutex.destroy() != IDE_SUCCESS)
    {
        IDE_ERRLOG(IDE_RP_1);
    }

    return;
}


/*-----------------------------------------------------------------------------
 * Name :
 *     rpcHBT::stop()
 *
 * Argument :
 *    None:
 *
 * Description :
 *  Called when Server Shutdown.
 *  before call this function,
 *  all Host object shoule be end. else, IDE_ASSERT() will complain
 *
 * ---------------------------------------------------------------------------*/
void rpcHBT::stop()
{
    IDE_ASSERT(lock() == IDE_SUCCESS);
    IDE_ASSERT(mCond.signal() == IDE_SUCCESS);

    mKillFlag = ID_TRUE;
    IDE_ASSERT(unlock() == IDE_SUCCESS);

    if(join() != IDE_SUCCESS)
    {
        IDE_SET(ideSetErrorCode(rpERR_ABORT_RP_JOIN_THREAD));
        IDE_ERRLOG(IDE_RP_1);
        IDE_CALLBACK_FATAL("[Repl HBT] Thread join error");
    }

    // 모든 클라이언트가 종료해야 됨.
    IDE_DASSERT( IDU_LIST_IS_EMPTY( &mHostRscList ) == ID_TRUE );

    if ( IDU_LIST_IS_EMPTY( &mHostRscList ) != ID_TRUE )
    {
        ideLog::log(IDE_RP_1, RP_TRC_HBT_NTC_REMAINED_HOST);
    }
    else
    {
        /* do nothing */
    }

    return;
}

/*-----------------------------------------------------------------------------
 * Name :
 *     rpcHBT::insertList() :
 *
 * Argument :
 *    aRsc - Host Info to add list
 *
 * Description :
 *    add to List the Host Information Node.
 *    Caution!! : be sure to lock the mutex before call this function.
 *
 * ---------------------------------------------------------------------------*/
void rpcHBT::insertList( rpcHostRsc * aRsc )
{
    IDU_LIST_ADD_LAST( &(mSelf->mHostRscList), &(aRsc->mNode) );
}

/*-----------------------------------------------------------------------------
 * Name :
 *     rpcHBT::deleteList() :
 *
 * Argument :
 *    aRsc - Host Info to delete from list
 *
 * Description :
 *    delete the Host Information Node from list
 *    Caution!! : be sure to lock the mutex before call this function.
 *
 * ---------------------------------------------------------------------------*/
void rpcHBT::deleteList( rpcHostRsc * aRsc )
{
    IDU_LIST_REMOVE( &(aRsc->mNode) );
}

/*-----------------------------------------------------------------------------
 * Name :
 *     rpcHBT::run() :  Thread Loop
 *
 * Argument :
 *    None
 *
 * Description :
 *    wakeup every REPLICATION_HBT_DETECT_TIME second.
 *    and, check the remote host validity. (see also, detectFault())
 *
 * ---------------------------------------------------------------------------*/
void rpcHBT::run()
{
    IDE_RC             sRC = IDE_FAILURE;
    SInt               sTimeoutSec = 0;
    UInt               sSpendTimeoutSec = 0;
    PDL_Time_Value     sCheckTime;
    PDL_Time_Value     sSleepTime;

    IDE_ASSERT(lock() == IDE_SUCCESS);

    sCheckTime.initialize();
    sSleepTime.initialize();

    while(mKillFlag != ID_TRUE)
    {
        IDE_CLEAR();

        sTimeoutSec = RPU_REPLICATION_HBT_DETECT_TIME - sSpendTimeoutSec;
        sSpendTimeoutSec = 0;

        if ( sTimeoutSec > 0 )
        {
            /* do nothing */
        }
        else
        {
            sTimeoutSec = 1;
        }

        sSleepTime.set( sTimeoutSec );
        sCheckTime = idlOS::gettimeofday() + sSleepTime;

        sRC = mCond.timedwait( &mMutex, &sCheckTime, IDU_IGNORE_TIMEDOUT );
        /* ------------------------------------------------
         *  위의 결과는 두가지의 동작에 의해 결정
         *  1. timeout 상황
         *     이때 err = -1, errno = ETIME
         *  2. shutdown시 signal이 날아온 상황
         *     이때 err = 0
         *
         * 위의 두경우가 아니라면 에러임.
         * ----------------------------------------------*/
        if (sRC != IDE_SUCCESS)
        {
            IDE_WARNING(IDE_RP_1, RP_TRC_HBT_ERR_RUN_COND_TIMEDWAIT);
        }
        else
        {
            ideLog::log( IDE_RP_1, RP_TRC_HBT_NTC_DETECT_FAULT_PRECD );
            runHBT( &sSpendTimeoutSec );
        }
    }

    IDE_ASSERT(unlock() == IDE_SUCCESS);

    return;
}

/*-----------------------------------------------------------------------------
 * Name :
 *     rpcHBT::initHostRsc
 *
 * Argument :
 *    aRsc  - resource object :
 *    aIP   - Ip Address to Find
 *    aPort - Port Number to Find
 *
 * Description :
 *  Just Initialization of the Host Resource.
 *
 * ---------------------------------------------------------------------------*/
void rpcHBT::initHostRsc( rpcHostRsc * aRsc, 
                          SChar      * aIP, 
                          SInt         aPort )
{
    idlOS::memset(aRsc, 0, ID_SIZEOF(rpcHostRsc));

    aRsc->mRefCount  = 1;
    aRsc->mStatus    = RP_HOST_STATUS_NONE;
    aRsc->mWaterMark = 0;
    aRsc->mFault     = ID_FALSE;
    aRsc->mShouldBeCleaned = ID_FALSE;

    idlOS::strcpy(aRsc->mHostIP, aIP);
    aRsc->mPort      = aPort;
    aRsc->mErrorNumber = 0;

    IDU_LIST_INIT_OBJ( &(aRsc->mNode), aRsc );
}

/*-----------------------------------------------------------------------------
 * Name :
 *     rpcHBT::finalHostRsc
 *
 * Argument :
 *    aRsc  - resource object :
 *    aIP   - Ip Address to Find
 *    aPort - Port Number to Find
 *
 * Description :
 *  Just Initialization of the Host Resource.
 *
 * ---------------------------------------------------------------------------*/

void rpcHBT::finalHostRsc( rpcHostRsc * /* aRsc */ )
{
}

/*-----------------------------------------------------------------------------
 * Name :
 *     rpcHBT::findHost
 *
 * Argument :
 *    aIP   - Ip Address to Find
 *    aPort - Port Number to Find
 *
 * Description :
 *  find the Host Node from the List.
 *  if found, return the Node Pointer,
 *  else, return NULL;
 *
 *    Caution!! : be sure to lock the mutex before call this function.
 * ---------------------------------------------------------------------------*/
rpcHostRsc* rpcHBT::findHost(SChar *aIP, SInt aPort)
{
    iduListNode     * sNode = NULL;
    iduListNode     * sDummy = NULL;
    rpcHostRsc      * sRsc = NULL;

    IDU_LIST_ITERATE_SAFE( &(rpcHBT::mSelf->mHostRscList), sNode, sDummy )
    {
        sRsc = (rpcHostRsc*)sNode->mObj;
        if ( ( idlOS::strncmp( sRsc->mHostIP, aIP, QCI_MAX_NAME_LEN ) == 0 ) &&
             ( sRsc->mPort == aPort ) && 
             ( sRsc->mShouldBeCleaned == ID_FALSE ) )
        {
            break;
        }
        else
        {
            sRsc = NULL;
        }
    }

    return sRsc;
}

/*-----------------------------------------------------------------------------
 * Name :
 *     rpcHBT::registHost() : regiser the host info specified by qrxSender
 *
 * Argument :
 *    aRsc  - resource object : return value
 *    aIP   - Ip Address
 *    aPort - Port Number
 *
 * Description :
 *    first, find whether there same host exist or not.
 *    second, if don's exist, make new object, and link to list.
 *            already exist, add the reference count.
 * ---------------------------------------------------------------------------*/

IDE_RC rpcHBT::registHost( void **aRsc, SChar *aIP, SInt aPort )
{
    idBool sIsMemAlloc   = ID_FALSE;
    idBool sIsHostInsert = ID_FALSE;
    idBool sIsHostLock   = ID_FALSE;

    rpcHostRsc  * sRsc = NULL;

    IDE_ASSERT(lock() == IDE_SUCCESS);
    sIsHostLock = ID_TRUE;

    sRsc = findHost(aIP, aPort);

    if ( sRsc == NULL)
    {
        IDU_FIT_POINT_RAISE( "rpcHBT::registHost::malloc::Rsc",
                              ERR_MEMORY_ALLOC_RSC );
        IDE_TEST_RAISE(iduMemMgr::malloc(IDU_MEM_RP_RPC,
                                         ID_SIZEOF(rpcHostRsc),
                                         (void**)&sRsc,
                                         IDU_MEM_IMMEDIATE)
                       != IDE_SUCCESS, ERR_MEMORY_ALLOC_RSC);
        sIsMemAlloc = ID_TRUE;

        initHostRsc( sRsc, aIP, aPort );
        insertList( sRsc );
        sIsHostInsert = ID_TRUE;
    }
    else
    {
        sRsc->mRefCount++;
    }

    *aRsc = sRsc;

    IDE_ASSERT(mCond.signal() == IDE_SUCCESS);

    sIsHostLock = ID_FALSE;
    IDE_ASSERT(unlock() == IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_MEMORY_ALLOC_RSC);
    {
        IDE_ERRLOG(IDE_RP_1);
        IDE_SET(ideSetErrorCode(rpERR_ABORT_MEMORY_ALLOC,
                                "rpcHBT::registHost",
                                "aRsc"));
    }
    IDE_EXCEPTION_END;
    IDE_PUSH();

    if(sIsHostInsert == ID_TRUE)
    {
        deleteList((rpcHostRsc *)*aRsc);
        finalHostRsc((rpcHostRsc *)*aRsc);
    }

    if ( sIsHostLock == ID_TRUE )
    {
        sIsHostLock = ID_FALSE;
        IDE_ASSERT( unlock() == IDE_SUCCESS );
    }

    if ( sIsMemAlloc == ID_TRUE )
    {
        (void)iduMemMgr::free( sRsc );
        sRsc = NULL;
    }
    else
    {
        /* do nothing */
    }

    *aRsc = NULL;

    IDE_POP();

    return IDE_FAILURE;
}

/*-----------------------------------------------------------------------------
 * Name :
 *     rpcHBT::unregistHost() : unregiser the host info specified by qrxSender
 *
 * Argument :
 *    aRsc  - resource object
 *
 * Description :
 *    first, check the reference count,
 *           if the count is 1, close the info & delete from list
 *           if the count is over 1, just decrease the count and return;
 * ---------------------------------------------------------------------------*/

void rpcHBT::unregistHost(void *aRsc)
{
    rpcHostRsc *sRsc;

    sRsc = (rpcHostRsc *)aRsc;

    IDE_ASSERT(sRsc != NULL);

    IDE_ASSERT(lock() == IDE_SUCCESS);
    
    IDE_DASSERT(findHost(sRsc->mHostIP, sRsc->mPort) != NULL);
    
    IDE_DASSERT(sRsc->mRefCount > 0);

    sRsc->mRefCount--;
    if (sRsc->mRefCount == 0)
    {
        if ( sRsc->mStatus == RP_HOST_STATUS_NONE ) 
        {
            deleteList(sRsc);
            finalHostRsc(sRsc);

            (void)iduMemMgr::free(sRsc);
            sRsc = NULL;
        }
        else
        {
            sRsc->mShouldBeCleaned = ID_TRUE;
        }
    }
    else
    {
        /* nothing to do */
    }

    IDE_ASSERT(unlock() == IDE_SUCCESS);

    return;
}

/*-----------------------------------------------------------------------------
 * Name :
 *     rpcHBT::status
 *
 * Argument :
 *    aRsc : Host Resource Object
 *
 * Description :
 *  Just Print Status info of the Host
 *
 * ---------------------------------------------------------------------------*/

void rpcHBT::status(rpcHostRsc *aRsc)
{
    ideLog::log(IDE_RP_1, RP_TRC_HBT_NTC_HOST_STATUS,
                aRsc->mHostIP,
                (UInt)aRsc->mPort,
                (UInt)aRsc->mRefCount,
                (UInt)aRsc->mStatus,
                ( aRsc->mFault == ID_TRUE ) ? "Yes Fault!!" : "No Fault");
}

void rpcHBT::initializeHostRscList( void )
{
    iduListNode * sNode = NULL;
    rpcHostRsc  * sRsc = NULL;

    IDU_LIST_ITERATE( &mHostRscList, sNode )
    {
        sRsc = (rpcHostRsc*)sNode->mObj;
        if ( rpnSocketInitailize( &(sRsc->mSocket),
                                  sRsc->mHostIP,
                                  QCI_MAX_NAME_LEN + 1,
                                  (UShort)sRsc->mPort,
                                  ID_FALSE /* non-Block */ )
             == IDE_SUCCESS )
        {
            sRsc->mStatus = RP_HOST_STATUS_INIT;
        }
        else
        {
            sRsc->mErrorNumber = errno;

            IDE_ERRLOG( IDE_RP_0 );
            ideLog::log( IDE_RP_0, RP_TRC_HBT_ERR_SOCKET_OPEN_FAIL,
                         sRsc->mHostIP,
                         (UInt)sRsc->mPort,
                         (UInt)sRsc->mRefCount,
                         (UInt)sRsc->mStatus,
                         sRsc->mWaterMark );

            sRsc->mStatus = RP_HOST_STATUS_INIT_ERROR;
        }
    }
}

void rpcHBT::connectFromHostRsc( rpcHostRsc * aRsc )
{
    if ( rpnSocketConnect( &(aRsc->mSocket) )
         == IDE_SUCCESS )
    {
        rpnSocketFinalize( &(aRsc->mSocket) );
        aRsc->mStatus = RP_HOST_STATUS_CONNECTED;
        ideLog::log( IDE_RP_1, RP_TRC_HBT_NTC_CONNECTED,
                     aRsc->mHostIP,
                     (UInt)aRsc->mPort,
                     (UInt)aRsc->mRefCount,
                     (UInt)aRsc->mStatus);
    }
    else
    {
        if ( ideGetErrorCode() == rpERR_ABORT_TIMEOUT_EXCEED )
        {
            aRsc->mStatus = RP_HOST_STATUS_CONNECTING;
        }
        else
        {
            aRsc->mErrorNumber = errno;

            IDE_ERRLOG( IDE_RP_0 );
            ideLog::log( IDE_RP_0, RP_TRC_HBT_ERR_FAULT_DETECTED,
                         aRsc->mHostIP,
                         (UInt)aRsc->mPort,
                         (UInt)aRsc->mRefCount,
                         (UInt)aRsc->mStatus,
                         aRsc->mWaterMark );

            aRsc->mStatus = RP_HOST_STATUS_ERROR;

            rpnSocketFinalize( &(aRsc->mSocket) );
        }
    }
}

void rpcHBT::connectFromHostRscList( void )
{
    iduListNode * sNode = NULL;
    rpcHostRsc  * sRsc = NULL;

    IDU_LIST_ITERATE( &mHostRscList, sNode )
    {
        sRsc = (rpcHostRsc*)sNode->mObj;

        if ( sRsc->mStatus == RP_HOST_STATUS_INIT )
        {
            rpcHBT::connectFromHostRsc( sRsc );
        }
        else
        {
            /* do nothing */
        }
    }
}

void rpcHBT::checkFaultFromHostRscList( void )
{
    iduListNode * sNode = NULL;
    rpcHostRsc  * sRsc = NULL;

    IDU_LIST_ITERATE( &mHostRscList, sNode )
    {
        sRsc = (rpcHostRsc*)sNode->mObj;

        switch ( sRsc->mStatus )
        {
            case RP_HOST_STATUS_CONNECTED:
                sRsc->mWaterMark = 0;
                break;

            case RP_HOST_STATUS_NONE:
            case RP_HOST_STATUS_INIT_ERROR:
                break;

            case RP_HOST_STATUS_CONNECTING:
                rpnPollRemoveSocket( &mPoll,
                                     &(sRsc->mSocket) );
                /* fall through */
            case RP_HOST_STATUS_INIT:
                rpnSocketFinalize( &(sRsc->mSocket) );
                ideLog::log( IDE_RP_0, RP_TRC_HBT_ERR_FAULT_DETECTED,
                             sRsc->mHostIP,
                             (UInt)sRsc->mPort,
                             (UInt)sRsc->mRefCount,
                             (UInt)sRsc->mStatus,
                             sRsc->mWaterMark );

                sRsc->mStatus = RP_HOST_STATUS_ERROR;

                /* fall through */
            case RP_HOST_STATUS_ERROR:
                sRsc->mWaterMark++;
                break;

            default:
                IDE_DASSERT( 0 );
                break;
        }

        if ( ( sRsc->mWaterMark >= RPU_REPLICATION_HBT_DETECT_HIGHWATER_MARK ) &&
             ( sRsc->mWaterMark != 0 ) )
        {
            sRsc->mFault = ID_TRUE;
            ideLog::log( IDE_RP_0, RP_TRC_HBT_NTC_HOST_STATUS,
                         sRsc->mHostIP,
                         (UInt)sRsc->mPort,
                         (UInt)sRsc->mRefCount,
                         (UInt)sRsc->mStatus,
                         sRsc->mErrorNumber,
                         "Fault Detected" );
        }
        else
        {
            /* do nothing */
        }

        sRsc->mStatus = RP_HOST_STATUS_NONE;
    }
}

static IDE_RC rpnPollInCallback( rpnPoll   * aPoll,
                                 void      * aData )
{
    rpcHostRsc  * sRsc = NULL;
    SInt          sErrorNumber = 0;
    ULong         sErrorLength = 0;

    sRsc = (rpcHostRsc*)aData;

    sErrorLength = ID_SIZEOF( sErrorNumber );

    if ( rpnSocketGetOpt( &(sRsc->mSocket),
                          SOL_SOCKET,
                          SO_ERROR,
                          (void*)&sErrorNumber,
                          &sErrorLength )
         == IDE_SUCCESS )
    {
        if ( ( sErrorNumber != 0 ) &&
             ( rpnSocketIsConnecting( sErrorNumber ) == ID_FALSE ) )
        {
            ideLog::log( IDE_RP_0, RP_TRC_HBT_ERR_PRECD_GET_SOCKOPT, sErrorNumber );
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        sErrorNumber = errno;
        IDE_ERRLOG( IDE_RP_0 );
    }

    if ( rpnSocketIsConnecting( sErrorNumber ) == ID_FALSE )
    {
        if ( sErrorNumber == 0 )
        {
            sRsc->mStatus = RP_HOST_STATUS_CONNECTED;

            ideLog::log( IDE_RP_1, RP_TRC_HBT_NTC_CONNECTED,
                         sRsc->mHostIP,
                         (UInt)sRsc->mPort,
                         (UInt)sRsc->mRefCount,
                         (UInt)sRsc->mStatus );
        }
        else
        {
            sRsc->mErrorNumber = sErrorNumber;
            ideLog::log( IDE_RP_0, RP_TRC_HBT_ERR_FAULT_DETECTED,
                         sRsc->mHostIP,
                         (UInt)sRsc->mPort,
                         (UInt)sRsc->mRefCount,
                         (UInt)sRsc->mStatus,
                         sRsc->mWaterMark );

            sRsc->mStatus = RP_HOST_STATUS_ERROR;
        }

        rpnPollRemoveSocket( aPoll,
                             &(sRsc->mSocket) );
        rpnSocketFinalize( &(sRsc->mSocket) );
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;
}


void rpcHBT::prepareToDetectConnectionEvent( void )
{
    iduListNode * sNode = NULL;
    rpcHostRsc  * sRsc = NULL;

    IDU_LIST_ITERATE( &mHostRscList, sNode )
    {
        sRsc = (rpcHostRsc*)sNode->mObj;

        if ( sRsc->mStatus == RP_HOST_STATUS_CONNECTING )
        {
            if ( rpnPollAddSocket( &mPoll,
                                   &(sRsc->mSocket),
                                   (void*)sRsc,
                                   RPN_POLL_OUT )
                 != IDE_SUCCESS )
            {
                IDE_SET( ideSetErrorCode( rpERR_ABORT_POLL_ADD_SOCK ) );
                IDE_ERRLOG( IDE_RP_0 );
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            /* do nothing */
        }
    }
}

void rpcHBT::detectConnectionEvent( UInt          aTimeoutSec,
                                    UInt        * aSpentTimeSec )
{
    UInt         sSpentTimeSec = 0;
    time_t       sStartTimeSec = 0;
    time_t       sEndTimeSec = 0;

    sStartTimeSec = idlOS::time();
    
    while ( ( rpnPollGetCount( &mPoll ) != 0 ) &&
            ( sSpentTimeSec < aTimeoutSec ) &&
            ( mKillFlag != ID_TRUE ) )
    {

        (void)rpnPollDispatch( &mPoll,
                               1000,
                               &rpnPollInCallback );

        sEndTimeSec = idlOS::time();
        
        if ( sStartTimeSec == -1 || sEndTimeSec == -1 )
        {
            sSpentTimeSec = sSpentTimeSec + 1;
        }
        else
        {
            sSpentTimeSec = sEndTimeSec - sStartTimeSec;
        }
    }

    *aSpentTimeSec = sSpentTimeSec;
}

void rpcHBT::cleanupHostRscList( void )
{
    rpcHostRsc  * sRsc = NULL;
    iduListNode * sNode = NULL;
    iduListNode * sDummy = NULL;

    IDU_LIST_ITERATE_SAFE( &mHostRscList, sNode, sDummy )
    {
        sRsc = (rpcHostRsc*)sNode->mObj;

        if ( sRsc->mShouldBeCleaned == ID_TRUE )
        {
            deleteList(sRsc);
            finalHostRsc(sRsc);

            (void)iduMemMgr::free(sRsc);
            sRsc = NULL;
        }
        else
        {
            /* do nothing */
        }
    }
}

void rpcHBT::runHBT( UInt    * aSpentTimeSec )
{
    UInt    sSpendTimeoutSec = 0;

    if ( IDU_LIST_IS_EMPTY( &mHostRscList ) == ID_FALSE )
    {
        /* Init */
        initializeHostRscList();

        /* Try Connect */
        connectFromHostRscList();

        /* register event to poller */
        prepareToDetectConnectionEvent();

        IDE_ASSERT( unlock() == IDE_SUCCESS );

        /* detect event from poller */
        detectConnectionEvent( RPU_REPLICATION_HBT_DETECT_TIME,
                               &sSpendTimeoutSec );

        IDE_ASSERT( lock() == IDE_SUCCESS );

        /* check Fault */
        checkFaultFromHostRscList();

        /* clean up */
        cleanupHostRscList();

        IDE_DASSERT( rpnPollGetCount( &mPoll ) == 0 );
    }
    else
    {
        /* clean up */
        cleanupHostRscList();

        sSpendTimeoutSec = 0;
    }

    *aSpentTimeSec = sSpendTimeoutSec;
}

