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
 * $Id: rpxPJMgr.cpp 85417 2019-05-09 08:36:26Z yoonhee.kim $
 **********************************************************************/

#include <idl.h> // to remove win32 compile warning

#include <qci.h>

#include <rpcManager.h>

#include <rpxPJChild.h>
#include <rpxPJMgr.h>
#include <rpxSender.h>

rpxPJMgr::rpxPJMgr() : idtBaseThread()
{
}

IDE_RC rpxPJMgr::initialize( SChar        * aRepName,
                             smiStatement * aStatement,
                             rpdMeta      * aMeta,
                             RP_SOCKET_TYPE aSocketType,
                             SChar        * aIPAddress,
                             UInt           aPortNo,
                             rpIBLatency    aIBLatency,
                             idBool       * aExitFlag,
                             SInt           aParallelFactor,
                             smiStatement * aParallelSmiStmts )
{
    SInt             i = 0;
    SChar            sName[IDU_MUTEX_NAME_LEN];
    SInt             sStage = 0;
    mStatement     = aStatement;
    mChildCount    = aParallelFactor;
    mExitFlag      = aExitFlag;
    mIsError       = ID_FALSE;
    mPJMgrExitFlag = ID_FALSE;

    IDU_LIST_INIT( &mSyncList );

    idlOS::snprintf( sName, IDU_MUTEX_NAME_LEN, "RP_%s_PJ_MUTEX", aRepName );
    IDE_TEST_RAISE( mJobMutex.initialize( sName,
                                          IDU_MUTEX_KIND_POSIX,
                                          IDV_WAIT_INDEX_NULL )
                    != IDE_SUCCESS, ERR_MUTEX_INIT );
    sStage = 1;

    IDE_TEST( mMemPool.initialize( IDU_MEM_RP_RPX_SENDER,
                                   sName,
                                   1,
                                   ID_SIZEOF(rpxSyncItem),
                                   256,
                                   IDU_AUTOFREE_CHUNK_LIMIT,    /* ChunkLimit */
                                   ID_FALSE,                    /* UseMutex */
                                   8,                           /* AlignByte */
                                   ID_FALSE,                    /* ForcePooling */
                                   ID_TRUE,                     /* GarbageCollection */
                                   ID_TRUE,                     /* HWCacheLine */
                                   IDU_MEMPOOL_TYPE_LEGACY      /* mempool type*/)
             != IDE_SUCCESS);

    sStage = 2;

    mChildArray = NULL;

    IDU_FIT_POINT_RAISE( "rpxPJMgr::initialize::calloc::ChildArray",
                          ERR_MEMORY_ALLOC_CHILD_ARRAY );
    IDE_TEST_RAISE( iduMemMgr::calloc( IDU_MEM_RP_RPX_SYNC,
                                       mChildCount,
                                       ID_SIZEOF(rpxPJChild *),
                                       (void**)&mChildArray,
                                       IDU_MEM_IMMEDIATE )
                   != IDE_SUCCESS, ERR_MEMORY_ALLOC_CHILD_ARRAY );

    for ( i = 0; i < mChildCount; i++ )
    {
        mChildArray[i] = NULL;

        IDU_FIT_POINT_RAISE( "rpxPJMgr::initialize::malloc::Child",
                              ERR_MEMORY_ALLOC_CHILD );
        IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_RP_RPX_SYNC,
                                           ID_SIZEOF( rpxPJChild ),
                                           (void**)&( mChildArray[i] ),
                                           IDU_MEM_IMMEDIATE)
                       != IDE_SUCCESS, ERR_MEMORY_ALLOC_CHILD);

        new ( mChildArray[i] )rpxPJChild();

        IDE_TEST_RAISE( mChildArray[i]->initialize( this,
                                                    aMeta,
                                                    aSocketType,
                                                    aIPAddress,
                                                    aPortNo,
                                                    aIBLatency,
                                                    &( aParallelSmiStmts[i] ),
                                                    mChildCount,
                                                    i,
                                                    &mSyncList,
                                                    mExitFlag ) != IDE_SUCCESS,
                        ERR_INIT );
    }

    IDE_TEST_RAISE( mMutex.initialize( (SChar *)"REPLICATOR_PJ_MANAGER_MUTEX",
                                       IDU_MUTEX_KIND_POSIX,
                                       IDV_WAIT_INDEX_NULL )
                   != IDE_SUCCESS, ERR_MUTEX_INIT );
    sStage = 3;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC_CHILD_ARRAY );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                 "rpxPJMgr::initialize",
                                 "mChildArray" ) );
        IDE_ERRLOG( IDE_RP_0 );
    }
    IDE_EXCEPTION( ERR_MEMORY_ALLOC_CHILD );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "rpxPJMgr::initialize",
                                  "mChildArray[i]" ) );
        IDE_ERRLOG( IDE_RP_0 );
    }
    IDE_EXCEPTION( ERR_INIT );
    {
        (void)iduMemMgr::free( mChildArray[i] );
        mChildArray[i] = NULL;
    }
    IDE_EXCEPTION( ERR_MUTEX_INIT );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_FATAL_ThrMutexInit ) );
        IDE_ERRLOG( IDE_RP_0 );

        IDE_CALLBACK_FATAL( "[Repl PJMgr] Mutex initialization error" );
    }
    IDE_EXCEPTION_END;
    IDE_PUSH();

    if ( mChildArray != NULL )
    {
        for( ; i > 0; i-- )
        {
            mChildArray[i - 1]->destroy();
            (void)iduMemMgr::free( mChildArray[i - 1] );
            mChildArray[i - 1] = NULL;
        }
        (void)iduMemMgr::free( mChildArray );
        mChildArray = NULL;
    }

    switch ( sStage )
    {
        case 3:
            (void)mMutex.destroy();
        case 2:
            (void)mMemPool.destroy( ID_FALSE );
        case 1:
            (void)mJobMutex.destroy();
        default:
            break;
    }

    IDE_POP();
    return IDE_FAILURE;
}

IDE_RC rpxPJMgr::allocSyncItem( rpdMetaItem * aTable )
{
    idBool sIsAlloc = ID_FALSE;

    rpxSyncItem * sSyncItem = NULL;

    IDE_TEST( mMemPool.alloc( (void **)&sSyncItem ) != IDE_SUCCESS );
    sIsAlloc = ID_TRUE;

    sSyncItem->mTable = aTable;
    sSyncItem->mSyncedTuples = 0;

    IDU_LIST_INIT_OBJ( &( sSyncItem->mNode ), sSyncItem );
    IDU_LIST_ADD_LAST( &mSyncList, &( sSyncItem->mNode ) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sIsAlloc == ID_TRUE )
    {
        (void)( mMemPool.memfree( sSyncItem ) );
    }
    else
    {
        /* nothing to do */
    }

    IDE_WARNING( IDE_RP_0, RP_TRC_SSP_ERR_ALLOC_JOB );

    return IDE_FAILURE;
}

void rpxPJMgr::removeTotalSyncItems()
{
    /* Child 에서 mSyncList 를 사용하고 있기 때문에 Child 가 종료되기 전에 실행시키면 안된다 */

    rpxSyncItem * sSyncItem  = NULL;
    iduListNode * sNode      = NULL;
    iduListNode * sDummy     = NULL;

    IDE_ASSERT( mJobMutex.lock( NULL ) == IDE_SUCCESS );

    if ( IDU_LIST_IS_EMPTY( &mSyncList ) != ID_TRUE )
    {
        IDU_LIST_ITERATE_SAFE( &mSyncList, sNode, sDummy )
        {
            sSyncItem = (rpxSyncItem*)sNode->mObj;
            IDU_LIST_REMOVE( sNode );

            (void)( mMemPool.memfree( sSyncItem ) );
            sSyncItem = NULL;
        }
    }
    else
    {
        /* nothing to do */
    }

    IDE_ASSERT( mJobMutex.unlock() == IDE_SUCCESS );
}

void rpxPJMgr::destroy()
{
    SInt   i;

    for(i = 0; i < mChildCount; i++)
    {
        mChildArray[i]->destroy();
        (void)iduMemMgr::free(mChildArray[i]);
        mChildArray[i] = NULL;
    }
    (void)iduMemMgr::free(mChildArray);
    mChildArray = NULL;
   
    removeTotalSyncItems();

    if(mMutex.destroy() != IDE_SUCCESS)
    {
        IDE_ERRLOG(IDE_RP_0);
    }

    if ( mMemPool.destroy( ID_FALSE ) != IDE_SUCCESS )
    {
        IDE_ERRLOG( IDE_RP_0 );
    }

    if ( mJobMutex.destroy() != IDE_SUCCESS )
    {
        IDE_ERRLOG( IDE_RP_0 );
    }

    return;
}

void rpxPJMgr::run()
{
    SInt        i;
    SInt        sEnd;
    SInt        sPos         = 0;
    SInt        sStartCount  = 0;

    idBool      sIsChildFail   = ID_FALSE;

    IDE_CLEAR();

    IDE_ASSERT(mMutex.lock(NULL /*idvSQL* */) == IDE_SUCCESS);
    sPos = 1;

    // PJChild 중 하나라도 시작했으면, 정상으로 취급한다.
    for(i = 0; i < mChildCount; i++)
    {
        IDU_FIT_POINT( "rpxPJMgr::run::Thread::mChildArray",
                       idERR_ABORT_THR_CREATE_FAILED,
                       "rpxPJMgr::run",
                       "mChildArray" );
        if(mChildArray[i]->start() != IDE_SUCCESS)
        {
            IDE_ERRLOG(IDE_RP_0);
            ideLog::log(IDE_RP_0, RP_TRC_PJM_ERR_ONE_CHILD_START, i);
        }
        else
        {
            sStartCount++;
        }
    }
    IDE_TEST_RAISE(sStartCount == 0, ERR_ALL_CHILD_START);

    sPos = 0;
    IDE_ASSERT(mMutex.unlock() == IDE_SUCCESS);

    /* ------------------------------------------------
     * 모든 작업이 끝날 때 까지 대기
     * ----------------------------------------------*/
    while( 1 )  // 대기하는 곳에서는 실패하지 않아야 한다.
    {
        sEnd  = 0;
        for (i = 0; i < mChildCount; i++)
        {
            if ((mChildArray[i]->getStatus() & RPX_PJ_SIGNAL_EXIT)
                == RPX_PJ_SIGNAL_EXIT)
            {
                sEnd++;
            }
        }

        if (sEnd == sStartCount) // 모두가 종료한 상태임
        {
            break;
        }
        PDL_Time_Value sPDL_Time_Value;
        sPDL_Time_Value.initialize(1);
        idlOS::sleep(sPDL_Time_Value);
    }

    for (i = 0; i < mChildCount; i++)
    {
        if((mChildArray[i]->getStatus() & RPX_PJ_SIGNAL_ERROR)
           == RPX_PJ_SIGNAL_ERROR)
        {
            ideLog::log(IDE_RP_0, RP_TRC_PJM_ERR_CHILD_FAIL, i);
            sIsChildFail = ID_TRUE;
        }
    }

    IDE_TEST( sIsChildFail == ID_TRUE );

    mPJMgrExitFlag = ID_TRUE;

    return;
    //이 함수는 에러를 반환하지 않고 에러를 설정할 필요가 없으므로,
    //에러코드를 설정하지 않고 아래와 같이 사용해도 된다.
    IDE_EXCEPTION( ERR_ALL_CHILD_START );
    {
        ideLog::log( IDE_RP_0, RP_TRC_PJM_ERR_ALL_CHILD_START );
    }
    IDE_EXCEPTION_END;

    mIsError = ID_TRUE;

    if(sPos == 1)
    {
        IDE_ASSERT(mMutex.unlock() == IDE_SUCCESS);
    }

    mPJMgrExitFlag = ID_TRUE;

    return;
}
/*
 *
 */
ULong rpxPJMgr::getSyncedCount( SChar *aTableName )
{
    ULong         sCount     = ID_ULONG_MAX;

    rpxSyncItem * sSyncItem  = NULL;
    iduListNode * sNode      = NULL;
    iduListNode * sDummy     = NULL;

    IDE_ASSERT( mJobMutex.lock( NULL ) == IDE_SUCCESS );

    if ( aTableName != NULL )
    {
        if ( IDU_LIST_IS_EMPTY( &mSyncList ) != ID_TRUE )
        {
            IDU_LIST_ITERATE_SAFE( &mSyncList, sNode, sDummy )
            {
                sSyncItem = (rpxSyncItem*)sNode->mObj;

                if( idlOS::strncmp( sSyncItem->mTable->mItem.mLocalTablename,
                                    aTableName,
                                    QC_MAX_OBJECT_NAME_LEN ) == 0 )

                {
                    sCount = sSyncItem->mSyncedTuples;
                    break;
                }
                else
                {
                    /* nothing to do */
                }
            }
        }
        else
        {
            /* nothing to do */
        }
    }
    else
    {
        /* nothing to do */
    }

    IDE_ASSERT( mJobMutex.unlock() == IDE_SUCCESS );

    return sCount;
}
