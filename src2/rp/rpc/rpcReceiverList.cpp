
#include <rpcReceiverList.h>

IDE_RC rpcReceiverList::initialize( const UInt    aMaxReceiverCount )
{
    UInt    i = 0;

    IDE_TEST_RAISE( mReceiverMutex.initialize((SChar *)"REPL_RECEIVER_MUTEX",
                                              IDU_MUTEX_KIND_POSIX,
                                              IDV_WAIT_INDEX_NULL)
                    != IDE_SUCCESS, ERR_MUTEX_INIT );
    
    IDU_FIT_POINT_RAISE( "rpcManager::initialize::calloc::ReceiverList",
                         ERR_MEMORY_ALLOC_RECEIVER_LIST );
    IDE_TEST_RAISE( iduMemMgr::calloc( IDU_MEM_RP_RPC,
                                       aMaxReceiverCount,
                                       ID_SIZEOF(rpcReceiver),
                                       (void**)&mReceiverList,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC_RECEIVER_LIST );

    for ( i = 0; i < aMaxReceiverCount; i++ )
    {
        mReceiverList[i].mReceiver = NULL;
        mReceiverList[i].mStatus = RPC_RECEIVER_LIST_UNUSED;
    }

    mMaxReceiverCount = aMaxReceiverCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MUTEX_INIT );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode(rpERR_FATAL_ThrMutexInit ) );
        IDE_ERRLOG( IDE_RP_0 );

        IDE_CALLBACK_FATAL( "[Repl Manager] Mutex initialization error" );
    }
    IDE_EXCEPTION( ERR_MEMORY_ALLOC_RECEIVER_LIST );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "rpcReceiver::initialize",
                                  "mReceiverList" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void rpcReceiverList::finalize( void )
{
    UInt        i = 0;

    for ( i = 0; i < mMaxReceiverCount; i++ )
    {
        IDE_DASSERT( mReceiverList[i].mStatus == RPC_RECEIVER_LIST_UNUSED );

        if ( mReceiverList[i].mStatus != RPC_RECEIVER_LIST_UNUSED )
        {
            ideLog::log( IDE_RP_0, 
                         "Receiver(%s) is remained", 
                         (mReceiverList[i].mReceiver)->getRepName() );
        }
    }

    iduMemMgr::free( mReceiverList );
    mReceiverList = NULL;
}

void rpcReceiverList::lock( void )
{
    IDE_ASSERT( mReceiverMutex.lock( NULL /* idvSQL* */ ) == IDE_SUCCESS );
}

void rpcReceiverList::tryLock( idBool   & aIsLock )
{
    IDE_ASSERT( mReceiverMutex.trylock( aIsLock ) == IDE_SUCCESS );
    
}

void rpcReceiverList::unlock( void )
{
    IDE_ASSERT( mReceiverMutex.unlock() == IDE_SUCCESS );
}

IDE_RC rpcReceiverList::getUnusedIndexAndReserve( UInt  * aReservedIndex )
{

    UInt    i = 0;

    for ( i = 0; i < mMaxReceiverCount; i++ )
    {
        if ( mReceiverList[i].mStatus == RPC_RECEIVER_LIST_UNUSED )
        {
            break;
        }
    }

    IDU_FIT_POINT_RAISE( "rpcManager::getUnusedReceiverIndexFromReceiverList::Erratic::rpERR_ABORT_RP_MAXIMUM_THREADS_REACHED",
                         ERR_REACH_MAX );
    IDE_TEST_RAISE( i >= mMaxReceiverCount,
                    ERR_REACH_MAX );

    *aReservedIndex = i;
    mReceiverList[i].mStatus = RPC_RECEIVER_LIST_RESERVED;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_REACH_MAX );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_MAXIMUM_THREADS_REACHED, i ) );
        IDE_ERRLOG( IDE_RP_0 );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpcReceiverList::setAndStartReceiver( const UInt       aReservedIndex,
                                             rpxReceiver    * aReceiver )
{
    idBool      sIsLocked = ID_FALSE;

    lock();
    sIsLocked = ID_TRUE;

    IDE_TEST( aReceiver->start() != IDE_SUCCESS );

    setReceiver( aReservedIndex, aReceiver );

    sIsLocked = ID_FALSE;
    unlock();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsLocked == ID_TRUE )
    {
        sIsLocked = ID_FALSE;
        unlock();
    }

    IDE_POP();

    return IDE_FAILURE;
}

void rpcReceiverList::setReceiver( const UInt       aReservedIndex,
                                   rpxReceiver    * aReceiver )
{
    IDE_DASSERT( mReceiverList[aReservedIndex].mStatus == RPC_RECEIVER_LIST_RESERVED );

    mReceiverList[aReservedIndex].mReceiver = aReceiver;
    mReceiverList[aReservedIndex].mStatus = RPC_RECEIVER_LIST_USED;
}

void rpcReceiverList::unsetReceiver( const UInt     aReservedIndex )
{
    mReceiverList[aReservedIndex].mReceiver = NULL;
    mReceiverList[aReservedIndex].mStatus = RPC_RECEIVER_LIST_UNUSED;
}

rpxReceiver * rpcReceiverList::getReceiver( const SChar  * aReplName )
{
    UInt              i = 0;
    rpxReceiver     * sReceiver = NULL;

    for ( i = 0; i < mMaxReceiverCount; i++ )
    {
        if ( mReceiverList[i].mStatus == RPC_RECEIVER_LIST_USED )
        {
            if ( mReceiverList[i].mReceiver->isYou( aReplName ) == ID_TRUE )
            {
                sReceiver = mReceiverList[i].mReceiver;
                break;
            }
        }
    }

    return sReceiver;
}

rpxReceiver * rpcReceiverList::getReceiver( const UInt aIndex )
{
    rpxReceiver     * sReceiver = NULL;

    if ( mReceiverList[aIndex].mStatus == RPC_RECEIVER_LIST_USED )
    {
        IDE_DASSERT( mReceiverList[aIndex].mReceiver != NULL );

        sReceiver = mReceiverList[aIndex].mReceiver;
    }
    else
    {
        IDE_DASSERT( mReceiverList[aIndex].mReceiver == NULL );

        sReceiver = NULL;
    }

    return sReceiver;
}
