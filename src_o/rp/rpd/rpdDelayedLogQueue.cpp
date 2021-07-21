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
 * $Id: 
 **********************************************************************/

#include <idu.h>

#include <rpError.h>

#include <rpdDelayedLogQueue.h>

IDE_RC rpdDelayedLogQueue::initialize( SChar        * aRepName )
{
    SChar       sName[IDU_MUTEX_NAME_LEN + 1] = { 0, };
    idBool      sIsPoolInitialized = ID_FALSE;

    IDU_FIT_POINT( "rpdDelayedLogQueue::initialize::initialize::mNodePool",
                   rpERR_ABORT_RP_INTERNAL_ARG,
                   "mNodePool.initialize" );
    idlOS::snprintf( sName, IDU_MUTEX_NAME_LEN, "RP_%s_DELAYED_LOG_QUEUE", aRepName );
    IDE_TEST( mNodePool.initialize( IDU_MEM_RP_RPD,
                                    sName,
                                    1,
                                    ID_SIZEOF( rpdDelayedLogQueueNode ),
                                    1 /* not use */,
                                    IDU_AUTOFREE_CHUNK_LIMIT,			/* ChunkLimit */
                                    ID_FALSE,							/* UseMutex */
                                    IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte */
                                    ID_FALSE,							/* ForcePoolling */
                                    ID_TRUE,							/* GarbageCollection */
                                    ID_TRUE,                            /* HWCacheLine */
                                    IDU_MEMPOOL_TYPE_LEGACY             /* mempool type*/ ) 
              != IDE_SUCCESS);			
    sIsPoolInitialized = ID_TRUE;

    mHead = NULL;
    mTail = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsPoolInitialized == ID_TRUE )
    {
        (void)mNodePool.destroy( ID_FALSE );
    }
    else
    {
        /* do nothing */
    }

    IDE_POP();

    return IDE_FAILURE;
}

void rpdDelayedLogQueue::finalize( void )
{
    (void)dequeueALL();
    (void)mNodePool.destroy( ID_FALSE );
}

IDE_RC rpdDelayedLogQueue::initailizeNode( rpdDelayedLogQueueNode     ** aNode )
{
    rpdDelayedLogQueueNode      * sNode = NULL;
    idBool          sIsAlloced = ID_FALSE;

    *aNode = NULL;

    IDU_FIT_POINT_RAISE( "rpdDelayedLogQueue::initializeNode::alloc::sNode",
                         ERR_MEMORY_ALLOC );
    IDE_TEST_RAISE( mNodePool.alloc( (void**)&sNode ) 
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC );
    sIsAlloced = ID_TRUE;

    sNode->mLogPtr = NULL;
    SM_LSN_INIT( sNode->mReadLSN );
    sNode->mNext = NULL;

    *aNode = sNode;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC )
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "initializeNode",
                                  "sNode" ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsAlloced == ID_TRUE )
    {
        (void)mNodePool.memfree( sNode );
        sNode = NULL;
    }
    else
    {
        /* do nohting */
    }

    IDE_POP();

    return IDE_FAILURE;
}

void rpdDelayedLogQueue::setObject( rpdDelayedLogQueueNode  * aNode,
                                    void                    * aLogPtr,
                                    smLSN                   * aReadLSN )
{
    aNode->mLogPtr = aLogPtr;
    SM_SET_LSN( aNode->mReadLSN,
                aReadLSN->mFileNo,
                aReadLSN->mOffset );
}

IDE_RC rpdDelayedLogQueue::enqueue( void   * aLogPtr, smLSN * aReadLSN )
{
    rpdDelayedLogQueueNode      * sNode = NULL;

    IDE_DASSERT( aLogPtr != NULL );

    IDE_TEST( initailizeNode( &sNode ) != IDE_SUCCESS );

    setObject( sNode, 
               aLogPtr,
               aReadLSN );

    /*
     *  Update mHead
     */
    if ( mHead == NULL )
    {
        mHead = sNode;
    }
    else
    {
        /* do nothing */
    }

    /*
     *  Update mTail
     */
    if ( mTail != NULL )
    {
        mTail->mNext = sNode;
    }
    else
    {
        /* do nothing */
    }
    mTail = sNode;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    

    return IDE_FAILURE;
}

IDE_RC rpdDelayedLogQueue::dequeue( void        ** aLogPtr,
                                    smLSN        * aReadLSN,
                                    idBool       * aIsEmpty )
{
    rpdDelayedLogQueueNode  * sNode = NULL;
    idBool                    sIsEmpty = ID_FALSE;

    *aLogPtr = NULL;

    sIsEmpty = isEmpty();
    if ( sIsEmpty == ID_FALSE )
    {
        sNode = mHead;

        /*
         *  Update Head
         */
        mHead = mHead->mNext;

        /*
         *  Update Tail
         */
        if ( mTail == sNode )
        {
            mTail = NULL;
        }
        else
        {
            /* do nothing */
        }

        *aLogPtr = sNode->mLogPtr;
    
        SM_SET_LSN( *aReadLSN,
                    sNode->mReadLSN.mFileNo,
                    sNode->mReadLSN.mOffset );
        IDE_TEST( mNodePool.memfree( sNode ) != IDE_SUCCESS );
    }
    else
    {
        /* do nothig */
    }

    *aIsEmpty = sIsEmpty;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdDelayedLogQueue::dequeueALL()
{
    SChar       * sLogTempPtr = NULL;
    smLSN         sReadTempLSN;
    idBool        sIsEmpty = ID_FALSE;

    while ( sIsEmpty == ID_FALSE )
    {
        sLogTempPtr = NULL;
        IDE_TEST( dequeue( (void**)&sLogTempPtr,
                           &sReadTempLSN,
                           &sIsEmpty )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool rpdDelayedLogQueue::isEmpty( void )
{
    idBool      sRC = ID_FALSE;

    if ( mHead != NULL )
    {
        sRC = ID_FALSE;
    }
    else
    {
        sRC = ID_TRUE;
    }

    return sRC;
}
