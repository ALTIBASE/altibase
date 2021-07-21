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

#include <mmErrorCode.h>
#include <mmcEventManager.h>
#include <mmcSession.h>

#define DUMMY_SIGNAL "ALTI$DUMMY"

IDE_RC mmcEventManager::initialize( mmcSession * aSession )
{
    mSession = aSession;

    IDU_LIST_INIT( &mPendingList );

    IDU_LIST_INIT( &mQueue );

    IDU_LIST_INIT( &mList );

    IDE_TEST( mMutex.initialize( (SChar *)"EVENT_MANAGER_MUTEX_FOR_CV",
                                 IDU_MUTEX_KIND_POSIX,
                                 IDV_WAIT_INDEX_NULL ) != IDE_SUCCESS );

    IDE_TEST( mSync.initialize( (SChar *)"EVENT_MANAGER_MUTEX_FOR_SYNC",
                                 IDU_MUTEX_KIND_POSIX,
                                 IDV_WAIT_INDEX_NULL ) != IDE_SUCCESS );

    IDE_TEST( mCV.initialize( (SChar *)"EVENT_MANAGER_CV" ) != IDE_SUCCESS );
                            
    IDE_TEST( mMemory.init( IDU_MEM_MMC ) != IDE_SUCCESS );

    mPollingInterval = 1;

    mSize = 0;

    mPendingSize = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_DASSERT( 0 );

    return IDE_FAILURE;
}

IDE_RC mmcEventManager::finalize()
{
    // 리스트/해쉬 테이블에 노드를 추가할때 할당된 메모리는 mMemory.destroy()
    // 에서 모두 해제된다.
    IDU_LIST_INIT( &mPendingList );

    IDU_LIST_INIT( &mQueue );

    IDU_LIST_INIT( &mList );

    IDE_TEST( mMemory.destroy() != IDE_SUCCESS );

    IDE_TEST( mMutex.destroy() != IDE_SUCCESS );

    IDE_TEST( mSync.destroy() != IDE_SUCCESS );

    IDE_TEST( mCV.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;
 
    IDE_EXCEPTION_END;

    IDE_DASSERT( 0 );

    return IDE_FAILURE;
}

IDE_RC mmcEventManager::regist( const SChar * aName,
                                UShort        aNameSize )
{
    SChar       * sName = NULL;
    SInt          sSize = aNameSize;
    iduListNode * sNode = NULL;
    idBool        sLock = ID_FALSE;

    IDE_TEST( lock() != IDE_SUCCESS );
    sLock = ID_TRUE;

    if ( isRegistered( aName,
                       aNameSize ) == ID_FALSE )
    {
        IDE_TEST( mMemory.alloc( sSize + 1,
                                 (void **)&sName ) != IDE_SUCCESS );

        idlOS::memcpy( sName, aName, sSize );

        sName[sSize] = '\0';

        IDE_TEST( mMemory.alloc( ID_SIZEOF(iduListNode),
                                 (void **)&sNode ) != IDE_SUCCESS );

        sNode->mObj = sName;

        IDU_LIST_ADD_LAST( &mList, sNode );

        mSize++;
    }
    else
    {
        /* do nothing */
    }

    sLock = ID_FALSE;
    IDE_TEST( unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sLock == ID_TRUE )
    {
        (void)unlock();
    }

    IDE_DASSERT( 0 );

    return IDE_FAILURE;
}

IDE_RC mmcEventManager::remove(  const SChar * aName,
                                 UShort        aNameSize )
{
    iduListNode * sNode     = NULL;
    iduListNode * sIterator = NULL;
    iduListNode * sNext     = NULL;
    idBool        sLock     = ID_FALSE;

    IDE_TEST( lock() != IDE_SUCCESS );
    sLock = ID_TRUE;

    IDU_LIST_ITERATE( &mList, sIterator )
    {
        if ( idlOS::strncmp( (SChar *)sIterator->mObj,
                             aName,
                             aNameSize ) == 0 )
        {
            sNode = sIterator;
            break;
        }
        else
        {
            /* do nothing */
        }
    }

    if ( sNode != NULL )
    {
        IDU_LIST_REMOVE( sNode );

        IDE_TEST( mMemory.free( sNode->mObj ) != IDE_SUCCESS );

        IDE_TEST( mMemory.free( sNode ) != IDE_SUCCESS ); 

        mSize--;
    }
    else
    {
        /* do nothing */
    }

    // 이벤트 큐에서도 제거되어야 한다.
    IDU_LIST_ITERATE_SAFE( &mQueue, sIterator, sNext )
    {
        if ( idlOS::strncmp( ((mmcEventInfo *)sIterator->mObj)->mName,
                             aName,
                             aNameSize ) == 0 )
        {
            sNode = sIterator;

            IDU_LIST_REMOVE( sNode );

            IDE_TEST( freeNode( sNode ) != IDE_SUCCESS );
        }
        else
        {
            /* do nothing */
        }
    }
 
    sLock = ID_FALSE;
    IDE_TEST( unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sLock == ID_TRUE )
    {
        (void)unlock();
    }

    IDE_DASSERT( 0 );

    return IDE_FAILURE;
}

IDE_RC mmcEventManager::removeall()
{
    iduListNode * sNode     = NULL;
    iduListNode * sNext     = NULL;
    iduListNode * sIterator = NULL;
    idBool        sLock     = ID_FALSE; 

    IDE_TEST( lock() != IDE_SUCCESS );
    sLock = ID_TRUE;

    IDU_LIST_ITERATE_SAFE( &mList, sIterator, sNext )
    {
        sNode = sIterator;

        IDU_LIST_REMOVE( sNode );

        IDE_TEST( mMemory.free( sNode->mObj ) != IDE_SUCCESS );

        IDE_TEST( mMemory.free( sNode ) != IDE_SUCCESS ); 
    }

    mSize = 0;

    // 이벤트 큐에서도 제거되어야 한다.
    IDU_LIST_ITERATE_SAFE( &mQueue, sIterator, sNext )
    {
        sNode = sIterator;

        IDU_LIST_REMOVE( sNode );

        IDE_TEST( freeNode( sNode ) != IDE_SUCCESS );
    }
 
    sLock = ID_FALSE;
    IDE_TEST( unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sLock == ID_TRUE )
    {
        (void)unlock();
    }

    IDE_DASSERT( 0 );

    return IDE_FAILURE;
}

IDE_RC mmcEventManager::setDefaults( SInt aPollingInterval )
{
    mPollingInterval = aPollingInterval;

    return IDE_SUCCESS;
}

IDE_RC mmcEventManager::signal( const SChar * aName,
                                UShort        aNameSize,
                                const SChar * aMessage,
                                UShort        aMessageSize )
{
    mmcEventInfo * sEvent = NULL;
    SInt           sSize;
    iduListNode  * sNode = NULL;

    sEvent = isSignaled( aName, aNameSize );

    if ( sEvent == NULL )
    {
        IDE_TEST( mMemory.alloc( ID_SIZEOF(mmcEventInfo),
                                 (void **)&sEvent ) != IDE_SUCCESS );

        //copy aName
        sSize = aNameSize;
   
        IDE_TEST( mMemory.alloc( sSize + 1,
                                 (void **)&(sEvent->mName) ) != IDE_SUCCESS );

        idlOS::memcpy( sEvent->mName, aName, sSize );

        sEvent->mName[sSize] = '\0';

        //copy aMessage
        sSize = aMessageSize;

        IDE_TEST( mMemory.alloc( sSize + 1,
                                 (void **)&(sEvent->mMessage) ) != IDE_SUCCESS );

        idlOS::memcpy( sEvent->mMessage, aMessage, sSize );

        sEvent->mMessage[sSize] = '\0';

        IDE_TEST( mMemory.alloc( ID_SIZEOF(iduListNode),
                                 (void **)&sNode ) != IDE_SUCCESS );

        sNode->mObj = sEvent;

        IDU_LIST_ADD_LAST( &mPendingList, sNode );

        mPendingSize++;
    }
    else
    {
        IDE_DASSERT( sEvent->mMessage != NULL );

        IDE_TEST( mMemory.free( sEvent->mMessage ) != IDE_SUCCESS );

        //copy aMessage
        sSize = aMessageSize;

        IDE_TEST( mMemory.alloc( sSize + 1,
                                 (void **)&(sEvent->mMessage) ) != IDE_SUCCESS );

        idlOS::memcpy( sEvent->mMessage, aMessage, sSize );

        sEvent->mMessage[sSize] = '\0';
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_DASSERT( 0 );

    return IDE_FAILURE;
}

IDE_RC mmcEventManager::waitany( idvSQL   * aStatistics,
                                 SChar    * aName,
                                 UShort   * aNameSize,
                                 SChar    * aMessage,
                                 UShort   * aMessageSize,
                                 SInt     * aStatus,
                                 const SInt aTimeout )
{
    iduListNode * sNode      = NULL;
    iduListNode * sIterator  = NULL;
    iduListNode * sNext      = NULL;
    SInt          sSize;
    idBool        sFound     = ID_FALSE;
    time_t        sUntilTime;
    IDE_RC        sError;
    SInt          i;
    idBool        sLock      = ID_FALSE;
    idBool        sLockForCV = ID_FALSE;

    for ( i = 0; i < aTimeout; )
    {
        IDE_TEST( lock() != IDE_SUCCESS );
        sLock = ID_TRUE;

        IDE_TEST_RAISE( mSize == 0, ERROR_NO_ALERTS_REGISTERED );

        IDU_LIST_ITERATE_BACK( &mQueue, sIterator )
        {
            sNode = sIterator;

            IDU_LIST_REMOVE( sNode );

            sSize = idlOS::strlen( ((mmcEventInfo *)sNode->mObj)->mName );
       
            idlOS::memcpy( aName,
                           ((mmcEventInfo *)sNode->mObj)->mName,
                           sSize );

            *aNameSize = sSize;
 
            sSize = idlOS::strlen( ((mmcEventInfo *)sNode->mObj)->mMessage );
       
            idlOS::memcpy( aMessage,
                           ((mmcEventInfo *)sNode->mObj)->mMessage,
                           sSize );

            *aMessageSize = sSize; 
               
            IDE_TEST( freeNode( sNode ) != IDE_SUCCESS );      

            *aStatus = 0;

            sFound = ID_TRUE;
        
            IDE_CONT( pass );
        }

        IDE_TEST_CONT( aTimeout == 0, pass );

        sLock = ID_FALSE;
        IDE_TEST( unlock() != IDE_SUCCESS );

        sUntilTime = idlOS::time( NULL ) + mPollingInterval;

        mTV.set( sUntilTime );

        IDE_TEST( lockForCV() != IDE_SUCCESS );
        sLockForCV = ID_TRUE;

        sError = mCV.timedwait( &mMutex,
                                &mTV );

        sLockForCV = ID_FALSE;
        IDE_TEST( unlockForCV() != IDE_SUCCESS );

        IDE_TEST_RAISE( (sError != IDE_SUCCESS) && (mCV.isTimedOut() != ID_TRUE),
                        ERROR_COND_TIMEDWAIT );

        IDE_TEST( iduCheckSessionEvent( aStatistics ) != IDE_SUCCESS );

        i += mPollingInterval;
    }

    IDE_EXCEPTION_CONT( pass );

    if ( sFound == ID_FALSE )
    {
        aName[0]      = '\0';
        *aNameSize    = 0;
        aMessage[0]   = '\0';
        *aMessageSize = 0;
        *aStatus      = 1;
    }
    else
    {
        // aName과 이름이 같은 중복된 이벤트를 제거한다.
        IDU_LIST_ITERATE_SAFE( &mQueue, sIterator, sNext )
        {
            if ( idlOS::strncmp( ((mmcEventInfo *)sIterator->mObj)->mName,
                                 aName,
                                 *aNameSize ) == 0 )
            {
                sNode = sIterator;

                IDU_LIST_REMOVE( sNode );

                IDE_TEST( freeNode( sNode ) != IDE_SUCCESS );
            }
            else
            {
                /* do nothing */
            }
        }
    }

    if ( sLock == ID_TRUE )
    {
        sLock = ID_FALSE;
        IDE_TEST( unlock() != IDE_SUCCESS );
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERROR_NO_ALERTS_REGISTERED );
    {
        IDE_SET( ideSetErrorCode(mmERR_ABORT_NO_ALERTS_REGISTERED) );
    }
    IDE_EXCEPTION( ERROR_COND_TIMEDWAIT );
    {
        IDE_SET( ideSetErrorCode(mmERR_FATAL_ThrCondWait) );
    }
    IDE_EXCEPTION_END;

    if ( sLock == ID_TRUE )
    {
        (void)unlock();
    }

    if ( sLockForCV == ID_TRUE )
    {
        (void)unlockForCV();
    }

    return IDE_FAILURE;
}

IDE_RC mmcEventManager::waitone( idvSQL       * aStatistics,
                                 const SChar  * aName,
                                 const UShort * aNameSize,
                                 SChar        * aMessage,
                                 UShort       * aMessageSize,
                                 SInt         * aStatus,
                                 const SInt     aTimeout )
{
    iduListNode * sNode      = NULL;
    iduListNode * sIterator  = NULL;
    iduListNode * sNext      = NULL;
    SInt          sSize;
    idBool        sFound     = ID_FALSE;
    time_t        sUntilTime;
    SInt          sError;
    SInt          i;
    idBool        sLock      = ID_FALSE;
    idBool        sLockForCV = ID_FALSE;

    for ( i = 0; i < aTimeout; )
    {
        IDE_TEST( lock() != IDE_SUCCESS );
        sLock = ID_TRUE;

        IDU_LIST_ITERATE_BACK( &mQueue, sIterator )
        {
            if ( idlOS::strncmp( ((mmcEventInfo *)sIterator->mObj)->mName,
                                 aName,
                                 *aNameSize ) == 0 )
            {
                sNode = sIterator;

                IDU_LIST_REMOVE( sNode );

                sSize = idlOS::strlen( ((mmcEventInfo *)sNode->mObj)->mMessage );
       
                idlOS::memcpy( aMessage,
                               ((mmcEventInfo *)sNode->mObj)->mMessage,
                               sSize );

                *aMessageSize = sSize; 
               
                IDE_TEST( freeNode( sNode ) != IDE_SUCCESS );      

                *aStatus = 0;

                sFound = ID_TRUE;
        
                IDE_CONT( pass );
            }
            else
            {
                /* do nothing */
            }
        }

        IDE_TEST_CONT( aTimeout == 0, pass );

        sLock = ID_FALSE;
        IDE_TEST( unlock() != IDE_SUCCESS );

        sUntilTime = idlOS::time( NULL ) + mPollingInterval;

        mTV.set( sUntilTime );

        IDE_TEST( lockForCV() != IDE_SUCCESS );
        sLockForCV = ID_TRUE;

        sError = mCV.timedwait( &mMutex,
                                &mTV );

        sLockForCV = ID_FALSE;
        IDE_TEST( unlockForCV() != IDE_SUCCESS );

        IDE_TEST_RAISE( (sError != IDE_SUCCESS) && (mCV.isTimedOut() != ID_TRUE),
                        ERROR_COND_TIMEDWAIT );

        IDE_TEST( iduCheckSessionEvent( aStatistics ) != IDE_SUCCESS );

        i += mPollingInterval;
    }

    IDE_EXCEPTION_CONT( pass );

    if ( sFound == ID_FALSE )
    {
        aMessage[0]   = '\0';
        *aMessageSize = 0;
        *aStatus      = 1;
    }
    else
    {
        // aName과 이름이 같은 중복된 이벤트를 제거한다.
        IDU_LIST_ITERATE_SAFE( &mQueue, sIterator, sNext )
        {
            if ( idlOS::strncmp( ((mmcEventInfo *)sIterator->mObj)->mName,
                                 aName,
                                 *aNameSize ) == 0 )
            {
                sNode = sIterator;

                IDU_LIST_REMOVE( sNode );

                IDE_TEST( freeNode( sNode ) != IDE_SUCCESS );
            }
            else
            {
                /* do nothing */
            }
        }
    }

    if ( sLock == ID_TRUE )
    {
        sLock = ID_FALSE;
        IDE_TEST( unlock() != IDE_SUCCESS );
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERROR_COND_TIMEDWAIT );
    {
        IDE_SET( ideSetErrorCode(mmERR_FATAL_ThrCondWait) );
    }
    IDE_EXCEPTION_END;

    if ( sLock == ID_TRUE )
    {
        (void)unlock();
    }

    if ( sLockForCV == ID_TRUE )
    {
        (void)unlockForCV();
    }

    return IDE_FAILURE;
}

idBool mmcEventManager::isRegistered( const SChar * aName,
                                      UShort        aNameSize )
{
    iduListNode *sNode     = NULL;
    iduListNode *sIterator = NULL;

    IDU_LIST_ITERATE( &mList, sIterator )
    {
        if ( idlOS::strncmp( (SChar *)sIterator->mObj,
                             aName,
                             aNameSize ) == 0 )
        {
            sNode = sIterator;
            break;
        }
        else
        {
            /* do nothing */
        }
    }

    if ( sNode == NULL )
    {
        return ID_FALSE;
    }
    else
    {
        return ID_TRUE;
    }
}

mmcEventInfo * mmcEventManager::isSignaled( const SChar * aName,
                                            UShort        aNameSize )
{
    iduListNode *sNode     = NULL;
    iduListNode *sIterator = NULL;

    IDU_LIST_ITERATE( &mPendingList, sIterator )
    {
        if ( idlOS::strncmp( ((mmcEventInfo *)sIterator->mObj)->mName,
                            aName,
                            aNameSize ) == 0 )
        {
            sNode = sIterator;
            break;
        }
        else
        {
            /* do nothing */
        }
    }

    if ( sNode == NULL )
    {
        return NULL;
    }
    else
    {
        return (mmcEventInfo *)sNode->mObj;
    }
}

IDE_RC mmcEventManager::commit()
{
    iduListNode * sNode     = NULL;
    iduListNode * sNext     = NULL;
    iduListNode * sIterator = NULL;
  
    if ( mPendingSize > 0 )
    {
        IDE_TEST( mmtSessionManager::applySignal( mSession ) != IDE_SUCCESS );

        IDU_LIST_ITERATE_SAFE( &mPendingList, sIterator, sNext )
        {
            sNode = sIterator;

            IDU_LIST_REMOVE( sNode );

            IDE_TEST( freeNode( sNode ) != IDE_SUCCESS );
        }

        mPendingSize = 0;
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_DASSERT( 0 );

    return IDE_FAILURE;
}    

IDE_RC mmcEventManager::rollback( SChar * aSvpName )
{
    iduListNode *sNode     = NULL;
    iduListNode *sNext     = NULL;
    iduListNode *sIterator = NULL;
    idBool       sForce    = ID_FALSE;

    if ( aSvpName == NULL )
    {
        IDU_LIST_ITERATE_SAFE( &mPendingList, sIterator, sNext )
        {
            sNode = sIterator;
        
            IDU_LIST_REMOVE( sNode );

            IDE_TEST( freeNode( sNode ) != IDE_SUCCESS );
        }

        mPendingSize = 0;
    }
    else
    {
        IDU_LIST_ITERATE_SAFE( &mPendingList, sIterator, sNext )
        {
            if ( sForce == ID_FALSE )
            {
                if ( idlOS::strncmp( ((mmcEventInfo *)sIterator->mObj)->mName,
                                     DUMMY_SIGNAL,
                                     idlOS::strlen( DUMMY_SIGNAL ) ) == 0 )
                {
                    if ( idlOS::strncmp( ((mmcEventInfo *)sIterator->mObj)->mMessage,
                                         aSvpName,
                                         idlOS::strlen( aSvpName ) ) == 0 )
                    {
                        sForce = ID_TRUE;

                        sNode = sIterator;

                        IDU_LIST_REMOVE( sNode );

                        IDE_TEST( freeNode( sNode ) != IDE_SUCCESS );
                    }
                    else
                    {
                        /* do nothing */
                    }
                }
                else
                {
                    /* do nothing */
                }
            } 
            else
            {
                sNode = sIterator;

                if ( idlOS::strncmp( ((mmcEventInfo *)sIterator->mObj)->mName,
                                     DUMMY_SIGNAL,
                                     idlOS::strlen( DUMMY_SIGNAL ) ) != 0 )
                {
                    mPendingSize--;
                }
                else
                {
                    /* do nothing */
                }
 
                IDU_LIST_REMOVE( sNode );

                IDE_TEST( freeNode( sNode ) != IDE_SUCCESS );
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_DASSERT( 0 );

    return IDE_FAILURE;
}

IDE_RC mmcEventManager::savepoint( SChar * aSvpName )
{
    const SChar    * sName = DUMMY_SIGNAL;
    mmcEventInfo   * sEvent = NULL;
    SInt             sSize;
    iduListNode    * sNode = NULL;

    IDE_TEST( removeSvp( aSvpName ) != IDE_SUCCESS );

    IDE_TEST( mMemory.alloc( ID_SIZEOF(mmcEventInfo),
                             (void **)&sEvent ) != IDE_SUCCESS );

    //copy sName
    sSize = idlOS::strlen( sName );
   
    IDE_TEST( mMemory.alloc( sSize + 1,
                             (void **)&(sEvent->mName) ) != IDE_SUCCESS );

    idlOS::memcpy( sEvent->mName, sName, sSize );

    sEvent->mName[sSize] = '\0';

    //copy aSvpName 
    sSize = idlOS::strlen( aSvpName );

    IDE_TEST( mMemory.alloc( sSize + 1,
                             (void **)&(sEvent->mMessage) ) != IDE_SUCCESS );

    idlOS::memcpy( sEvent->mMessage, aSvpName, sSize );

    sEvent->mMessage[sSize] = '\0';

    IDE_TEST( mMemory.alloc( ID_SIZEOF(iduListNode),
                             (void **)&sNode ) != IDE_SUCCESS );

    sNode->mObj = sEvent;

    IDU_LIST_ADD_LAST( &mPendingList, sNode );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_DASSERT( 0 );

    return IDE_FAILURE;
}

IDE_RC mmcEventManager::removeSvp( SChar * aSvpName )
{
    iduListNode *sNode     = NULL;
    iduListNode *sIterator = NULL;

    IDU_LIST_ITERATE( &mPendingList, sIterator )
    {
        if ( idlOS::strncmp( ((mmcEventInfo *)sIterator->mObj)->mName,
                             DUMMY_SIGNAL,
                             idlOS::strlen( DUMMY_SIGNAL ) ) == 0 )
        {
            if ( idlOS::strncmp( ((mmcEventInfo *)sIterator->mObj)->mMessage,
                                 aSvpName,
                                 idlOS::strlen( aSvpName ) ) == 0 )
            {
                sNode = sIterator;
       
                IDU_LIST_REMOVE( sNode );

                IDE_TEST( freeNode( sNode ) != IDE_SUCCESS );

                break;
            }
            else
            {
                /* do nothing */
            }
        }
        else
        {
            /* do nothing */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_DASSERT( 0 );

    return IDE_FAILURE;
}

IDE_RC mmcEventManager::freeNode( iduListNode * aNode )
{
    IDE_TEST( mMemory.free( ((mmcEventInfo *)aNode->mObj)->mName ) != IDE_SUCCESS );

    IDE_TEST( mMemory.free( ((mmcEventInfo *)aNode->mObj)->mMessage ) != IDE_SUCCESS );

    IDE_TEST( mMemory.free( aNode->mObj ) != IDE_SUCCESS );

    IDE_TEST( mMemory.free( aNode ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_DASSERT( 0 );

    return IDE_FAILURE;
}

IDE_RC mmcEventManager::apply( mmcSession * aSession )
{
    iduList        * sPendingList  = aSession->getInfo()->mEvent.getPendingList();
    iduListNode    * sNode     = NULL;
    iduListNode    * sIterator = NULL;
    mmcEventInfo   * sEvent    = NULL;
    SInt             sSize;
    idBool           sLock      = ID_FALSE;
    idBool           sLockForCV = ID_FALSE;

    if ( mSession != aSession )
    {
        IDE_TEST( lock() != IDE_SUCCESS );
        sLock = ID_TRUE;
    }
    else
    {
        /* do nothing */
    }

    // 등록된 이벤트 개수가 0이면 바로 리턴한다.
    IDE_TEST_CONT( mSize == 0, pass );

    IDU_LIST_ITERATE( sPendingList, sIterator )
    {
        if ( idlOS::strncmp( ((mmcEventInfo *)sIterator->mObj)->mName,
                             DUMMY_SIGNAL,
                             idlOS::strlen( DUMMY_SIGNAL ) ) == 0 )
        {
            continue;
        }
        else
        {
            /* do nothing */
        }

        if ( isRegistered( ((mmcEventInfo *)sIterator->mObj)->mName,
                           (UShort)idlOS::strlen(((mmcEventInfo *)sIterator->mObj)->mName) ) == ID_TRUE )
        {
            IDE_TEST( mMemory.alloc( ID_SIZEOF(mmcEventInfo),
                                     (void **)&sEvent ) != IDE_SUCCESS );

            //copy aName
            sSize = idlOS::strlen( ((mmcEventInfo *)sIterator->mObj)->mName );
   
            IDE_TEST( mMemory.alloc( sSize + 1,
                                     (void **)&(sEvent->mName) ) != IDE_SUCCESS );

            idlOS::memcpy( sEvent->mName, ((mmcEventInfo *)sIterator->mObj)->mName, sSize );

            sEvent->mName[sSize] = '\0';

            //copy aMessage
            sSize = idlOS::strlen( ((mmcEventInfo *)sIterator->mObj)->mMessage );

            IDE_TEST( mMemory.alloc( sSize + 1,
                                     (void **)&(sEvent->mMessage) ) != IDE_SUCCESS );

            idlOS::memcpy( sEvent->mMessage, ((mmcEventInfo *)sIterator->mObj)->mMessage, sSize );

            sEvent->mMessage[sSize] = '\0';

            IDE_TEST( mMemory.alloc( ID_SIZEOF(iduListNode),
                                     (void **)&sNode ) != IDE_SUCCESS );

            sNode->mObj = sEvent;

            IDU_LIST_ADD_LAST( &mQueue, sNode );
        }
        else
        {
            /* do nothing */
        }
    }

    IDE_TEST( lockForCV() != IDE_SUCCESS );
    sLockForCV = ID_TRUE;
 
    IDE_TEST( mCV.signal() != IDE_SUCCESS );

    sLockForCV = ID_FALSE;
    IDE_TEST( unlockForCV() != IDE_SUCCESS );

    IDE_EXCEPTION_CONT( pass );

    if ( mSession != aSession )
    {
        sLock = ID_FALSE;
        IDE_TEST( unlock() != IDE_SUCCESS );
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( mSession != aSession )
    {
        if ( sLock == ID_TRUE )
        {
            (void)unlock();
        }
    }

    if ( sLockForCV == ID_TRUE )
    {
        (void)unlockForCV();
    }

    IDE_DASSERT( 0 );

    return IDE_FAILURE;
}

void mmcEventManager::dump()
{
    iduListNode *sNode     = NULL;
    iduListNode *sIterator = NULL;

    ideLog::logMessage( IDE_SERVER_0, "##### session [%d]\n", mSession->getSessionID()  );

    ideLog::logMessage( IDE_SERVER_0, "# registered list\n" );
    IDU_LIST_ITERATE( &mList, sIterator )
    {
        sNode = sIterator;

        ideLog::logMessage( IDE_SERVER_0, "  %s\n", (SChar *)sNode->mObj );
    } 

    ideLog::logMessage( IDE_SERVER_0, "# pending list\n" );
    IDU_LIST_ITERATE( &mPendingList, sIterator )
    {
        sNode = sIterator;

        ideLog::logMessage( IDE_SERVER_0, 
                            "  name: %s message: %s\n", 
                            ((mmcEventInfo *)sNode->mObj)->mName,
                            ((mmcEventInfo *)sNode->mObj)->mMessage );
    } 

    ideLog::logMessage( IDE_SERVER_0, "# queue list\n" );
    IDU_LIST_ITERATE( &mQueue, sIterator )
    {
        sNode = sIterator;

        ideLog::logMessage( IDE_SERVER_0, 
                            "  name: %s message: %s\n", 
                            ((mmcEventInfo *)sNode->mObj)->mName,
                            ((mmcEventInfo *)sNode->mObj)->mMessage );
    } 
    ideLog::logMessage( IDE_SERVER_0, "\n" );
}
