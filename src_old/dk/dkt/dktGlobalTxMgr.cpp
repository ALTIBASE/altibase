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

#include <dki.h>
#include <dktGlobalTxMgr.h>
#include <iduCheckLicense.h>
#include <dksSessionMgr.h>
#include <smiMisc.h>

#define EPOCHTIME_20170101   ( 1483228800 ) /* ( ( ( ( (2017) - (1970) ) * 365 ) * 24 ) *3600 ) + a */

#define GLOBAL_COORDINATOR_HASH_COUNT ( 1024 ) /* BUG-48501 */

iduMemPool                 dktGlobalTxMgr::mGlobalCoordinatorPool;
SLong                      dktGlobalTxMgr::mUniqueGlobalTxSeq;
dktGlobalCoordinatorList * dktGlobalTxMgr::mGlobalCoordinatorList;

void dktGlobalTxMgr::lockRead( UInt aIdx )
{
    (void)mGlobalCoordinatorList[aIdx].mLatch.lockRead( NULL, NULL );
}

void dktGlobalTxMgr::lockWrite( UInt aIdx )
{
    (void)mGlobalCoordinatorList[aIdx].mLatch.lockWrite( NULL, NULL );
}

void dktGlobalTxMgr::unlock( UInt aIdx )
{
    (void)mGlobalCoordinatorList[aIdx].mLatch.unlock();
}

dktNotifier dktGlobalTxMgr::mNotifier;
UChar       dktGlobalTxMgr::mMacAddr[ACP_SYS_MAC_ADDR_LEN];
UInt        dktGlobalTxMgr::mInitTime = 0;
SInt        dktGlobalTxMgr::mProcessID = 0;
/************************************************************************
 * Description : Global transaction manager 를 초기화한다.
 *
 ************************************************************************/
IDE_RC  dktGlobalTxMgr::initializeStatic()
{
    time_t sTime       = 0;
    mUniqueGlobalTxSeq = 0;

    SChar sLatchName[128];
    UInt  sState = 0;
    UInt  i;

    IDE_TEST( mGlobalCoordinatorPool.initialize( IDU_MEM_DK,
                                                (SChar*)"GLOBAL_COORDINATOR",
                                                1,        /* List Count */
                                                ID_SIZEOF( dktGlobalCoordinator ),
                                                1024,     /* itemcount per a chunk */
                                                IDU_AUTOFREE_CHUNK_LIMIT,
                                                ID_TRUE,  /* Use Mutex */
                                                IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,
                                                ID_FALSE, /* ForcePooling */
                                                ID_TRUE,  /* GarbageCollection */
                                                ID_TRUE,  /* HWCacheLine */
                                                IDU_MEMPOOL_TYPE_LEGACY  /* mempool type */ )
              != IDE_SUCCESS );
    sState = 1;

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_DK,
                                       ID_SIZEOF( dktGlobalCoordinatorList ) * GLOBAL_COORDINATOR_HASH_COUNT,
                                       (void **)&mGlobalCoordinatorList,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC_GLOBAL_COORDINATOR );
    sState = 2;

    /* Global coordinator 객체들의 관리를 위한 list 초기화 */
    for ( i = 0; i < GLOBAL_COORDINATOR_HASH_COUNT; i++ )
    {
        idlOS::snprintf( sLatchName,
                         ID_SIZEOF(sLatchName),
                         "DKT_GLOBAL_COORDINATOR_LIST_LATCH_%"ID_UINT32_FMT,
                         i );

        IDE_ASSERT( mGlobalCoordinatorList[i].mLatch.initialize( sLatchName )
                    == IDE_SUCCESS );

        IDU_LIST_INIT( &(mGlobalCoordinatorList[i].mHead) );

        mGlobalCoordinatorList[i].mCnt = 0;
    }
    sState = 3;

    IDE_TEST( mNotifier.initialize() != IDE_SUCCESS );

    if ( ( DKU_DBLINK_ENABLE == DK_ENABLE ) ||
         ( sdi::isShardEnable() == ID_TRUE ) )
    {
        IDE_TEST_RAISE( mNotifier.start() != IDE_SUCCESS, ERR_NOTIFIER_START );
    }
    else
    {
        /* Nothing to do */
    }

    idlOS::memcpy( mMacAddr, iduCheckLicense::mLicense.mMacAddr[0].mAddr, ACP_SYS_MAC_ADDR_LEN );

    sTime = idlOS::time(NULL);

    /* well.... it's not going to happen...! */
    IDE_TEST_RAISE(  sTime == -1, ERR_INTERNAL_ERROR );

    mInitTime = ( sTime - EPOCHTIME_20170101 );/*sTime is always bigger*/

    mProcessID = (SInt)idlOS::getpid();

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOTIFIER_START );
    {
        ideLog::log( DK_TRC_LOG_FORCE, DK_TRC_T_NOTIFIER_THREAD );
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DK_INTERNAL_ERROR,
                                  "[dktGlobalTxMgr::initializeStatic] ERROR NOTIFIER START" ) );
    }
    IDE_EXCEPTION( ERR_INTERNAL_ERROR );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DK_INTERNAL_ERROR ) );
    }
    IDE_EXCEPTION( ERR_MEMORY_ALLOC_GLOBAL_COORDINATOR );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION_END;

    switch ( sState )
    {
        case 3 :
            for ( i = 0; i < GLOBAL_COORDINATOR_HASH_COUNT; i++ )
            {
                IDE_ASSERT( mGlobalCoordinatorList[i].mLatch.destroy() == IDE_SUCCESS );

                IDU_LIST_INIT( &(mGlobalCoordinatorList[i].mHead) );
            }
        case 2 :
            IDE_ASSERT( iduMemMgr::free(mGlobalCoordinatorList) == IDE_SUCCESS );
        case 1 :
            IDE_ASSERT( mGlobalCoordinatorPool.destroy() == IDE_SUCCESS );
        default :
            break;
    }

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Global transaction manager 를 정리한다.
 *
 ************************************************************************/
IDE_RC  dktGlobalTxMgr::finalizeStatic()
{
    UInt i;

    if ( ( DKU_DBLINK_ENABLE == DK_ENABLE ) ||
         ( sdi::isShardEnable() == ID_TRUE ) )
    {
        mNotifier.setExit( ID_TRUE );
        IDE_TEST( mNotifier.join() != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    mNotifier.finalize();

    IDE_DASSERT( dktGlobalTxMgr::getActiveGlobalCoordinatorCnt() == 0 );

    for ( i = 0; i < GLOBAL_COORDINATOR_HASH_COUNT; i++ )
    {
        IDE_ASSERT( mGlobalCoordinatorList[i].mLatch.destroy()
                    == IDE_SUCCESS );

        IDU_LIST_INIT( &(mGlobalCoordinatorList[i].mHead) );
    }

    IDE_ASSERT( iduMemMgr::free(mGlobalCoordinatorList) == IDE_SUCCESS );

    IDE_ASSERT( mGlobalCoordinatorPool.destroy() == IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Global coordinator 객체를 생성한다.
 *
 *  aSession            - [IN] Global coordinator 를 생성하는 세션정보
 *  aGlobalCoordinator  - [OUT] 생성된 global coordinator 를 가리키는 
 *                              포인터
 ************************************************************************/
IDE_RC  dktGlobalTxMgr::createGlobalCoordinator( dksDataSession        * aSession,
                                                 UInt                    aLocalTxId,
                                                 dktGlobalCoordinator ** aGlobalCoordinator )
{
    dktGlobalCoordinator * sGlobalCoordinator = NULL;
    UInt                   sState = 0;
    UInt                   sGlobalTxId = DK_INIT_GTX_ID;

    IDU_FIT_POINT_RAISE( "dktGlobalTxMgr::createGlobalCoordinator::malloc::GlobalCoordinator",
                          ERR_MEMORY_ALLOC_GLOBAL_COORDINATOR );

    IDE_TEST_RAISE( mGlobalCoordinatorPool.alloc( (void**)&sGlobalCoordinator )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC_GLOBAL_COORDINATOR );
    sState = 1;

    IDE_TEST( sGlobalCoordinator->initialize( aSession ) != IDE_SUCCESS );
    sState = 2;

    sGlobalTxId = generateGlobalTxId();
   
    sGlobalCoordinator->setLocalTxId( aLocalTxId );
    sGlobalCoordinator->setGlobalTxId( sGlobalTxId );

    addGlobalCoordinatorToList( sGlobalCoordinator );
    sState = 3;

    if ( sGlobalCoordinator->isGTx() == ID_TRUE )
    {
        IDE_TEST( sGlobalCoordinator->createDtxInfo( aLocalTxId,
                                                     sGlobalCoordinator->getGlobalTxId() )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    sGlobalCoordinator->setGlobalTxStatus( DKT_GTX_STATUS_BEGIN );

    *aGlobalCoordinator = sGlobalCoordinator;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC_GLOBAL_COORDINATOR );
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION_END;

    switch ( sState )
    {
        case 3:
            removeGlobalCoordinatorFromList( sGlobalCoordinator );
        case 2:
            (void)sGlobalCoordinator->finalize();
            /* fall through */
        case 1:
            (void)mGlobalCoordinatorPool.memfree( sGlobalCoordinator );
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}
 
UInt dktGlobalTxMgr::getActiveGlobalCoordinatorCnt()
{
    UInt sTxCnt = 0;
    UInt i      = 0;

    for ( i = 0; i < GLOBAL_COORDINATOR_HASH_COUNT; i++ )
    {
        lockRead(i);

        sTxCnt += mGlobalCoordinatorList[i].mCnt;

        unlock(i);
    }

    return sTxCnt;
}

/************************************************************************
 * Description : 입력받은 global coordinator 객체를 제거한다.
 *
 *  aGlobalCoordinator  - [IN] 제거할 global coordinator 를 가리키는 
 *                             포인터
 *
 ************************************************************************/
void  dktGlobalTxMgr::destroyGlobalCoordinator( dktGlobalCoordinator * aGlobalCoordinator )
{
    dktNotifier * sNotifier = NULL;

    IDE_ASSERT( aGlobalCoordinator != NULL );

    /* PROJ-2569 notifier에게 이관해야하므로,
     *  메모리 해제는 commit/rollback이 실행 후 성공 여부를 보고 그곳에서 한다. */
    if ( aGlobalCoordinator->mDtxInfo != NULL  )
    {
       if (  aGlobalCoordinator->mDtxInfo->isEmpty() != ID_TRUE ) 
       {
           sNotifier = getNotifier();

           if ( aGlobalCoordinator->mDtxInfo->mIsPassivePending == ID_TRUE )
           {
               if ( aGlobalCoordinator->getGTxStatus() >= DKT_GTX_STATUS_PREPARE_REQUEST )
               {
                   sNotifier->addDtxInfo( DK_NOTIFY_NORMAL, aGlobalCoordinator->mDtxInfo );
               }
               else
               {
                   aGlobalCoordinator->removeDtxInfo();
               }
           }
           else
           {
               sNotifier->addDtxInfo( DK_NOTIFY_NORMAL, aGlobalCoordinator->mDtxInfo );
           }
    
           IDE_ASSERT( aGlobalCoordinator->mCoordinatorDtxInfoMutex.lock( NULL /*idvSQL* */ ) == IDE_SUCCESS );
           aGlobalCoordinator->mDtxInfo = NULL;
           IDE_ASSERT( aGlobalCoordinator->mCoordinatorDtxInfoMutex.unlock() == IDE_SUCCESS );
        }
        else
        {
            aGlobalCoordinator->removeDtxInfo();
        }
    }
    else
    {
        /* Nothing to do */
    }

    removeGlobalCoordinatorFromList( aGlobalCoordinator );
    /* BUG-37487 : void */
    aGlobalCoordinator->finalize();

    (void)mGlobalCoordinatorPool.memfree( aGlobalCoordinator );

    aGlobalCoordinator = NULL;

    return;
}

/************************************************************************
 * Description : 현재 DK 내에서 수행되고 있는 모든 글로벌 트랜잭션들의 
 *               정보를 얻어온다.
 *              
 *  aInfo       - [IN] 글로벌 트랜잭션들의 정보를 담을 배열포인터
 ************************************************************************/
IDE_RC  dktGlobalTxMgr::getAllGlobalTransactonInfo( dktGlobalTxInfo ** aInfo, UInt * aGTxCnt )
{
    UInt   i       =  0;
    UInt   sGTxCnt = 0;
    UInt   sCnt    = 0;
    idBool sIsLock = ID_FALSE;    

    iduListNode           * sIterator           = NULL;
    dktGlobalCoordinator  * sGlobalCoordinator  = NULL;
    dktGlobalTxInfo       * sInfo               = NULL;

    sGTxCnt = dktGlobalTxMgr::getActiveGlobalCoordinatorCnt();

    if ( sGTxCnt > 0 )
    {
        IDU_FIT_POINT_RAISE( "dkmGetGlobalTransactionInfo::malloc::Info",
                             ERR_MEMORY_ALLOC );
        IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_DK,
                                           ID_SIZEOF( dktGlobalTxInfo ) * sGTxCnt,
                                           (void **)&sInfo,
                                           IDU_MEM_IMMEDIATE )
                        != IDE_SUCCESS, ERR_MEMORY_ALLOC );

        for ( i = 0; i < GLOBAL_COORDINATOR_HASH_COUNT; i++ )
        {
            lockRead(i);
            sIsLock = ID_TRUE;

            IDU_LIST_ITERATE( &mGlobalCoordinatorList[i].mHead, sIterator )
            {
                sGlobalCoordinator = (dktGlobalCoordinator *)sIterator->mObj;

                IDE_TEST_CONT( sCnt == sGTxCnt, FOUND_COMPLETE );

                IDE_TEST( sGlobalCoordinator->getGlobalTransactionInfo( &sInfo[sCnt] )
                        != IDE_SUCCESS );
                sCnt++;
            }

            sIsLock = ID_FALSE;
            unlock(i);
        }
    }

    IDE_EXCEPTION_CONT( FOUND_COMPLETE );

    *aInfo   = sInfo;
    *aGTxCnt = sCnt;

    if ( sIsLock == ID_TRUE )
    {
        sIsLock = ID_FALSE;
        unlock(i);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ) );
    }
    IDE_EXCEPTION_END;

    if ( sInfo != NULL )
    {
        (void)iduMemMgr::free( sInfo );
        sInfo = NULL;
    }
    else
    {
        /* do nothing */
    }

    if ( sIsLock == ID_TRUE )
    {
        sIsLock = ID_FALSE;
        unlock(i);
    }
    else
    {
        /* nothing to do */
    }
    return IDE_FAILURE;
}

/************************************************************************
 * Description : 현재 DK 내에서 수행되고 있는 모든 글로벌 트랜잭션들의 
 *               정보를 얻어온다.
 *              
 *  aInfo       - [IN/OUT] 글로벌 트랜잭션들의 정보를 담을 배열포인터
 *  aRTxCnt     - [IN] Caller 가 요청한 시점에 결정된 remote transaction
 *                     개수만큼만 정보를 가져오기 위한 입력값
 *
 ************************************************************************/
IDE_RC  dktGlobalTxMgr::getAllRemoteTransactonInfo( dktRemoteTxInfo ** aInfo,
                                                    UInt             * aRTxCnt )
{
    idBool sIsLock  = ID_FALSE;
    UInt   sInfoCnt	= 0;
    UInt   sRTxCnt  = 0;
    UInt   i        = 0;

    iduListNode          * sIterator          = NULL;
    dktGlobalCoordinator * sGlobalCoordinator = NULL;
    dktRemoteTxInfo      * sInfo              = NULL;

    dktGlobalTxMgr::getAllRemoteTransactionCount( &sRTxCnt );

    if ( sRTxCnt > 0 )
    {
        IDU_FIT_POINT_RAISE( "dkmGetRemoteTransactionInfo::malloc::Info",
                             ERR_MEMORY_ALLOC );
        IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_DK,
                                           ID_SIZEOF( dktRemoteTxInfo ) * sRTxCnt,
                                           (void **)&sInfo,
                                           IDU_MEM_IMMEDIATE )
                        != IDE_SUCCESS, ERR_MEMORY_ALLOC );

        for ( i = 0; i < GLOBAL_COORDINATOR_HASH_COUNT; i++ )
        {
            lockRead(i);
            sIsLock = ID_TRUE;

            IDU_LIST_ITERATE( &mGlobalCoordinatorList[i].mHead, sIterator )
            {
                sGlobalCoordinator = (dktGlobalCoordinator *)sIterator->mObj;

                IDE_TEST_CONT( sInfoCnt >= sRTxCnt, FOUND_COMPLETE );

                IDE_TEST( sGlobalCoordinator->getRemoteTransactionInfo( sInfo,
                                                                        sRTxCnt,
                                                                        &sInfoCnt /* 누적되는값 */ )
                          != IDE_SUCCESS );
            }

            sIsLock = ID_FALSE;
            unlock(i);
        }
    }

    IDE_EXCEPTION_CONT( FOUND_COMPLETE );

    *aInfo   = sInfo;
    *aRTxCnt = sInfoCnt;

    if ( sIsLock == ID_TRUE )
    {
        sIsLock = ID_FALSE;
        unlock(i);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION ) );
    }

    IDE_EXCEPTION_END;

    if ( sInfo != NULL )
    {
        (void)iduMemMgr::free( sInfo );
        sInfo = NULL;
    }
    else
    {
        /* do nothing */
    }

    if ( sIsLock == ID_TRUE )
    {
        sIsLock = ID_FALSE;
        unlock(i);
    }
    else
	{
		/* nothing to do */
	}

    return IDE_FAILURE;
}

/************************************************************************
 * Description : 현재 DK 내에서 수행되고 있는 모든 글로벌 트랜잭션들의 
 *               정보를 얻어온다.
 *              
 *  aInfo       - [IN/OUT] Remote statement 들의 정보를 담을 배열포인터
 *  aStmtCnt    - [IN] Caller 가 요청한 시점에 결정된 remote statement
 *                     개수만큼만 정보를 가져오기 위한 입력값
 *
 ************************************************************************/
IDE_RC  dktGlobalTxMgr::getAllRemoteStmtInfo( dktRemoteStmtInfo * aInfo,
                                              UInt              * aStmtCnt )
{
    idBool                  sIsLock             = ID_FALSE;
    UInt                    sRemoteStmtCnt      = *aStmtCnt;
    UInt                    sInfoCnt            = 0;
    iduListNode            *sIterator           = NULL;
    dktGlobalCoordinator   *sGlobalCoordinator  = NULL;
    UInt                   i = 0;

    for ( i = 0; i < GLOBAL_COORDINATOR_HASH_COUNT; i++ )
    {
        lockRead(i);
        sIsLock = ID_TRUE;

        IDU_LIST_ITERATE( &mGlobalCoordinatorList[i].mHead, sIterator )
        {
            sGlobalCoordinator = (dktGlobalCoordinator *)sIterator->mObj;

            IDE_TEST_CONT( sInfoCnt >= sRemoteStmtCnt, FOUND_COMPLETE );

            IDE_TEST( sGlobalCoordinator->getRemoteStmtInfo( aInfo,
                                                             sRemoteStmtCnt,
                                                             &sInfoCnt ) /* 누적되는 값 */
                      != IDE_SUCCESS );
        }

        sIsLock = ID_FALSE;
        unlock(i);
    }

    IDE_EXCEPTION_CONT( FOUND_COMPLETE );

    if ( sIsLock == ID_TRUE )
    {
        sIsLock = ID_FALSE;
        unlock(i);
    }

    *aStmtCnt = sInfoCnt;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sIsLock == ID_TRUE )
    {
        sIsLock = ID_FALSE;
        unlock(i);
    }
    else
    {
        /* nothing to do */
    }

    return IDE_FAILURE;
}

/************************************************************************
 * Description : 현재 DK 내에서 수행되고 있는 모든 remote transaction 의 
 *               개수를 구한다.
 *              
 *  aCount       - [OUT] 모든 remote transaction 의 개수
 *
 ************************************************************************/
void dktGlobalTxMgr::getAllRemoteTransactionCount( UInt  *aCount )
{
    UInt                   sRemoteTxCnt       = 0;
    dktGlobalCoordinator * sGlobalCoordinator = NULL;
    UInt                   i                  = 0;
    iduListNode          * sIterator          = NULL;

    for ( i = 0; i < GLOBAL_COORDINATOR_HASH_COUNT; i++ )
    {
        lockRead(i);

        IDU_LIST_ITERATE( &mGlobalCoordinatorList[i].mHead, sIterator )
        {
            sGlobalCoordinator = (dktGlobalCoordinator *)sIterator->mObj;
            sRemoteTxCnt += sGlobalCoordinator->getRemoteTxCount();
        }

        unlock(i);
    }

    *aCount = sRemoteTxCnt;
}

/************************************************************************
 * Description : 현재 DK 내에서 수행되고 있는 모든 remote statement 의 
 *               개수를 구한다.
 *              
 *  aCount       - [OUT] 모든 remote statement 의 개수
 *
 ************************************************************************/
IDE_RC dktGlobalTxMgr::getAllRemoteStmtCount( UInt *aCount )
{
    UInt                   sRemoteStmtCnt     = 0;
    dktGlobalCoordinator * sGlobalCoordinator = NULL;
    UInt                   i                  = 0;
    iduListNode          * sIterator          = NULL;

    for ( i = 0; i < GLOBAL_COORDINATOR_HASH_COUNT; i++ )
    {
        lockRead(i);

        IDU_LIST_ITERATE( &mGlobalCoordinatorList[i].mHead, sIterator )
        {
            sGlobalCoordinator = (dktGlobalCoordinator *)sIterator->mObj;
            sRemoteStmtCnt += sGlobalCoordinator->getAllRemoteStmtCount();
        }

        unlock(i);
    }

    *aCount = sRemoteStmtCnt;

    return IDE_SUCCESS;
}

/************************************************************************
 * Description : 입력받은 global coordinator 를 관리대상으로 추가한다. 
 *              
 *  aGlobalCoordinator  - [IN] List 에 추가할 global coordinator
 *
 ************************************************************************/
void dktGlobalTxMgr::addGlobalCoordinatorToList( dktGlobalCoordinator * aGlobalCoordinator )
{
    UInt sIdx;

    IDE_DASSERT( aGlobalCoordinator->getGlobalTxId() != DK_INIT_GTX_ID );
    IDE_DASSERT( aGlobalCoordinator->getGlobalTxId() != 0 );

    sIdx = ( aGlobalCoordinator->getGlobalTxId() % GLOBAL_COORDINATOR_HASH_COUNT );

    lockWrite(sIdx);

    IDU_LIST_INIT_OBJ( &(aGlobalCoordinator->mNode), aGlobalCoordinator );
    IDU_LIST_ADD_LAST( &(mGlobalCoordinatorList[sIdx].mHead), &(aGlobalCoordinator->mNode) );

    mGlobalCoordinatorList[sIdx].mCnt++;

    unlock(sIdx);
}

void dktGlobalTxMgr::removeGlobalCoordinatorFromList( dktGlobalCoordinator * aGlobalCoordinator )
{
    UInt sIdx;

    IDE_DASSERT( aGlobalCoordinator->getGlobalTxId() != DK_INIT_GTX_ID );
    IDE_DASSERT( aGlobalCoordinator->getGlobalTxId() != 0 );

    sIdx = ( aGlobalCoordinator->getGlobalTxId() % GLOBAL_COORDINATOR_HASH_COUNT );

    lockWrite(sIdx);

    IDU_LIST_REMOVE( &aGlobalCoordinator->mNode );

    mGlobalCoordinatorList[sIdx].mCnt--;

    unlock(sIdx);
}

/************************************************************************
 * Description : Global transaction id 를 입력받아 해당 글로벌 트랜잭션을
 *               관리하는 global coordinator 를 list 에서 찾아 반환한다.
 *              
 *  aGTxId      - [IN] global transaction id
 *  aGlobalCrd  - [OUT] 해당 글로벌 트랜잭션을 관리하는 global coordinator
 *
 ************************************************************************/
IDE_RC dktGlobalTxMgr::findGlobalCoordinator( UInt                    aGlobalTxId,
                                              dktGlobalCoordinator ** aGlobalCrd )
{
    idBool                  sIsExist            = ID_FALSE;
    iduListNode            *sIterator           = NULL;
    dktGlobalCoordinator   *sGlobalCoordinator  = NULL;

    UInt sIdx = ( aGlobalTxId % GLOBAL_COORDINATOR_HASH_COUNT );

    if ( aGlobalTxId == DK_INIT_GTX_ID )
    {
        *aGlobalCrd = NULL;

        IDE_CONT( SKIP );
    }

    lockRead(sIdx);

    IDU_LIST_ITERATE( &mGlobalCoordinatorList[sIdx].mHead, sIterator )
    {
        sGlobalCoordinator = (dktGlobalCoordinator *)sIterator->mObj;

        if ( sGlobalCoordinator->getGlobalTxId() == aGlobalTxId )
        {
            sIsExist = ID_TRUE;
            break;
        }
    }

    unlock(sIdx);

    if ( sIsExist == ID_TRUE )
    {
        *aGlobalCrd = sGlobalCoordinator;
    }
    else
    {
        *aGlobalCrd = NULL;
    }

    IDE_EXCEPTION_CONT( SKIP );

    return IDE_SUCCESS;
}

/************************************************************************
 * Description : Linker data session id 를 입력받아 해당 session 에 속한 
 *               global coordinator 를 list 에서 찾아 반환한다.
 *               => session 종료시에 호출된다. 
 *              
 *  aSessionId  - [IN] Linker data session id
 *  aGlobalCrd  - [OUT] 해당 글로벌 트랜잭션을 관리하는 global coordinator
 *
 ************************************************************************/
IDE_RC dktGlobalTxMgr::findGlobalCoordinatorWithSessionId( UInt                    aSessionId,
                                                           dktGlobalCoordinator ** aGlobalCrd )
{
    idBool                 sIsExist           = ID_FALSE;
    iduListNode          * sIterator          = NULL;
    dktGlobalCoordinator * sGlobalCoordinator = NULL;
    UInt                   i                  = 0;

    for ( i = 0; i < GLOBAL_COORDINATOR_HASH_COUNT; i++ )
    {
        lockRead(i);

        IDU_LIST_ITERATE( &mGlobalCoordinatorList[i].mHead, sIterator )
        {
            sGlobalCoordinator = (dktGlobalCoordinator *)sIterator->mObj;

            if ( sGlobalCoordinator->getCurrentSessionId() == aSessionId )
            {
                sIsExist = ID_TRUE;
                unlock(i);
                IDE_CONT( FOUND_COMPLETE );
            }
        }

        unlock(i);
    }

    IDE_EXCEPTION_CONT( FOUND_COMPLETE );

    if ( sIsExist == ID_TRUE )
    {
        *aGlobalCrd = sGlobalCoordinator;
    }
    else
    {
        *aGlobalCrd = NULL;
    }

    return IDE_SUCCESS;
}

/* GlobalTxID를 생성하는 함수.
   설정범위: 1 ~ ( UINT_MAX -1 ) */
UInt dktGlobalTxMgr::generateGlobalTxId()
{
    SLong sSystemGlobalTxId;
    SLong sOld;
    SLong sNew;

    /* BUG-48501
       atomic 관련함수는 signed 변수만 사용한다.
       UInt를 전체사용하기 위해 64bit(signed)를 사용해서 계산후 Uint로 변환한다. */

    while (1)
    {
        sSystemGlobalTxId = acpAtomicGet64( &mUniqueGlobalTxSeq );

        sNew = ( ( ((UInt)(sSystemGlobalTxId + 1)) == DK_INIT_GTX_ID ) ?
                 (SLong)1 : (sSystemGlobalTxId + 1) );

        sOld = acpAtomicCas64( &mUniqueGlobalTxSeq,
                               sNew,
                               sSystemGlobalTxId );

        if ( sOld == sSystemGlobalTxId )
        {
            /* CAS SUCCESS */
            break;
        }
    }

    return (UInt)sNew;
}

smLSN dktGlobalTxMgr::getDtxMinLSN( void )
{
    iduListNode          * sIterator           = NULL;
    dktGlobalCoordinator * sGlobalCoordinator  = NULL;
    dktDtxInfo           * sDtxInfo = NULL;
    smLSN                  sCompareLSN;
    smLSN                * sTempLSN = NULL;
    smLSN                  sCurrentLSN;
    UInt                   i;

    SMI_LSN_MAX( sCompareLSN );
    SMI_LSN_INIT( sCurrentLSN );

    for ( i = 0; i < GLOBAL_COORDINATOR_HASH_COUNT; i++ )
    {
        lockRead(i);

        IDU_LIST_ITERATE( &mGlobalCoordinatorList[i].mHead, sIterator )
        {
            sGlobalCoordinator = (dktGlobalCoordinator *)sIterator->mObj;

            IDE_ASSERT( sGlobalCoordinator->mCoordinatorDtxInfoMutex.lock( NULL /*idvSQL* */ )
                        == IDE_SUCCESS );

            if ( sGlobalCoordinator->mDtxInfo != NULL )
            {
                sTempLSN = sGlobalCoordinator->mDtxInfo->getPrepareLSN();

                if ( ( SMI_IS_LSN_INIT( *sTempLSN ) == ID_FALSE ) &&
                     ( isGT( &sCompareLSN, sTempLSN ) == ID_TRUE ) )
                {
                    SM_SET_SCN( &sCompareLSN, sTempLSN );
                }
            }

            IDE_ASSERT( sGlobalCoordinator->mCoordinatorDtxInfoMutex.unlock()
                        == IDE_SUCCESS );
        }

        unlock(i);
    }

    /* getMinLSN From Notifier */
    IDE_ASSERT( mNotifier.mNotifierDtxInfoMutex.lock( NULL /*idvSQL* */ ) == IDE_SUCCESS );
    IDU_LIST_ITERATE( &(mNotifier.mDtxInfo), sIterator )
    {
        sDtxInfo = (dktDtxInfo *)sIterator->mObj;
        sTempLSN = sDtxInfo->getPrepareLSN();

        if ( isGT( &sCompareLSN, sTempLSN )
             == ID_TRUE )
        {
            idlOS::memcpy( &sCompareLSN,
                           sTempLSN,
                           ID_SIZEOF( smLSN ) );
        }
        else
        {
            /* Nothing to do */
        }
    }
    IDE_ASSERT( mNotifier.mNotifierDtxInfoMutex.unlock() == IDE_SUCCESS );

    if ( DKU_DBLINK_RECOVERY_MAX_LOGFILE > 0 )
    {
        (void)smiGetLstLSN( &sCurrentLSN ); /* always return success  */

        if ( sCurrentLSN.mFileNo > sCompareLSN.mFileNo )
        {
            if ( ( sCurrentLSN.mFileNo - sCompareLSN.mFileNo ) <= DKU_DBLINK_RECOVERY_MAX_LOGFILE )
            {
                /* Nothing to do */
            }
            else
            {
                idlOS::memcpy( &sCompareLSN, &sCurrentLSN, ID_SIZEOF( smLSN ) );
            }
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

    return sCompareLSN;
}

IDE_RC dktGlobalTxMgr::getNotifierTransactionInfo( dktNotifierTransactionInfo ** aInfo,
                                                   UInt                        * aInfoCount )
{
    IDE_TEST( mNotifier.getNotifierTransactionInfo( aInfo,
                                                    aInfoCount )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC dktGlobalTxMgr::getShardNotifierTransactionInfo( dktNotifierTransactionInfo ** aInfo,
                                                        UInt                        * aInfoCount )
{
    IDE_TEST( mNotifier.getShardNotifierTransactionInfo( aInfo,
                                                         aInfoCount )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC dktGlobalTxMgr::createGlobalCoordinatorAndSetSessionTxId( dksDataSession        * aSession,
                                                                 UInt                    aLocalTxId,
                                                                 dktGlobalCoordinator ** aGlobalCoordinator )
{
    dktGlobalCoordinator * sGlobalCoordinator = NULL;

    IDE_TEST( createGlobalCoordinator( aSession,
                                       aLocalTxId,
                                       &sGlobalCoordinator )
              != IDE_SUCCESS );

    dksSessionMgr::setDataSessionGlobalTxId( aSession, sGlobalCoordinator->getGlobalTxId() );
    dksSessionMgr::setDataSessionLocalTxId( aSession, aLocalTxId );

    *aGlobalCoordinator = sGlobalCoordinator;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void dktGlobalTxMgr::destroyGlobalCoordinatorAndUnSetSessionTxId( dktGlobalCoordinator * aGlobalCoordinator,
                                                                dksDataSession       * aSession )
{
    destroyGlobalCoordinator( aGlobalCoordinator );

    dksSessionMgr::setDataSessionGlobalTxId( aSession, DK_INIT_GTX_ID );
    dksSessionMgr::setDataSessionLocalTxId( aSession, DK_INIT_LTX_ID );
}

