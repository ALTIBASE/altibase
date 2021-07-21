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

#include <mmcMutexPool.h>
#include <mmuProperty.h>

/* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
IDE_RC mmcMutexPool::initialize()
{
    UInt            i;
    UInt            sMutexPoolInitCnt;
    iduListNode    *sListNode = NULL;
    UInt            sState    = 0;
    iduMutex       *sMutex    = NULL;
    iduListNode    *sIterator = NULL;
    iduListNode    *sNodeNext = NULL;

    IDE_TEST(mMutexPool.initialize(IDU_MEM_MMC,
                                   (SChar *)"MMC_MUTEX_POOL",
                                   ID_SCALABILITY_SYS,
                                   ID_SIZEOF(iduMutex),
                                   mmuProperty::getMmcMutexpoolMempoolSize(),
                                   IDU_AUTOFREE_CHUNK_LIMIT,		/* ChunkLimit */
                                   ID_TRUE,							/* UseMutex */
                                   IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte */
                                   ID_FALSE,						/* ForcePooling */
                                   ID_TRUE,							/* GarbageCollection */
                                   ID_TRUE,                         /* HWCacheLine */
                                   IDU_MEMPOOL_TYPE_LEGACY          /* mempool type*/) 
            != IDE_SUCCESS);			
    sState = 1;

    IDE_TEST(mListNodePool.initialize(IDU_MEM_MMC,
                                      (SChar *)"MMC_MUTEXPOOL_LISTNODE_POOL",
                                      ID_SCALABILITY_SYS,
                                      ID_SIZEOF(iduListNode),
                                      mmuProperty::getMmcMutexpoolMempoolSize(),
                                      IDU_AUTOFREE_CHUNK_LIMIT,			/* ChunkLimit */
                                      ID_TRUE,							/* UseMutex */
                                      IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte */
                                      ID_FALSE,							/* ForcePooling */
                                      ID_TRUE,							/* GarbageCollection */
                                      ID_TRUE,                          /* HWCacheLine */
                                      IDU_MEMPOOL_TYPE_LEGACY           /* mempool type*/) 
            != IDE_SUCCESS);			
    sState = 2;

    IDU_LIST_INIT(&mUseList);
    IDU_LIST_INIT(&mFreeList);
    mFreeListCnt = 0;

    /* SESSION_MUTEXPOOL_FREE_LIST_MAXCNT 
       : maxinum count of free-list elements of the session mutexpool
       SESSION_MUTEXPOOL_FREE_LIST_INITCNT 
       : the number of initially initialized mutex in the free-list of the session mutexpool
    */
    sMutexPoolInitCnt = (mmuProperty::getSessionMutexpoolFreelistInitcnt()<=
                         mmuProperty::getSessionMutexpoolFreelistMaxcnt())?
                         mmuProperty::getSessionMutexpoolFreelistInitcnt():
                         mmuProperty::getSessionMutexpoolFreelistMaxcnt();

    sState = 3;
    /* Add ListNodes, each node has a pointer to an initialized iduMutex in a member variable named mObj,
       in mFreeList. */
    for( i = 0 ; i < sMutexPoolInitCnt ; i++ )
    {
        IDE_TEST( allocMutexPoolListNode( &sListNode ) != IDE_SUCCESS );
        IDU_LIST_ADD_LAST(&mFreeList, sListNode);
        mFreeListCnt++;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
 
    /*
     * BUG-40304 An unexpected error occured when listnode allocation fail.
     */
    switch( sState )
    {
        case 3:
            IDU_LIST_ITERATE_SAFE( &mFreeList, sIterator, sNodeNext )
            {
                sMutex = (iduMutex *)(sIterator->mObj);
                IDE_ASSERT(sMutex->destroy() == IDE_SUCCESS);
                /* BUG-47003 */
                (void)mMutexPool.memfree(sMutex);
                (void)mListNodePool.memfree(sIterator);
            }
        case 2:
            mListNodePool.destroy(ID_FALSE);
        case 1:
            mMutexPool.destroy(ID_FALSE);
        case 0:
        default:
            break;
    }

    return IDE_FAILURE;
}

IDE_RC mmcMutexPool::finalize()
{
    iduMutex     *sMutex    = NULL;
    iduListNode  *sIterator = NULL;
    iduListNode  *sNodeNext = NULL;

    /* Actually mUseList should be empty. */
    IDU_LIST_ITERATE_SAFE( &mUseList, sIterator, sNodeNext )
    {
        sMutex = (iduMutex *)(sIterator->mObj);
        IDE_ASSERT(sMutex->destroy() == IDE_SUCCESS);
        IDE_TEST( mMutexPool.memfree(sMutex) != IDE_SUCCESS );
        IDE_TEST( mListNodePool.memfree(sIterator) != IDE_SUCCESS );
    }

    IDU_LIST_ITERATE_SAFE( &mFreeList, sIterator, sNodeNext )
    {
        sMutex = (iduMutex *)(sIterator->mObj);
        IDE_ASSERT(sMutex->destroy() == IDE_SUCCESS);
        IDE_TEST( mMutexPool.memfree(sMutex) != IDE_SUCCESS );
        IDE_TEST( mListNodePool.memfree(sIterator) != IDE_SUCCESS );
    }
    IDE_TEST( mMutexPool.destroy() != IDE_SUCCESS );
    IDE_TEST( mListNodePool.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* The function that requests a mutex from mFreeList in the mutex pool. */
IDE_RC mmcMutexPool::getMutexFromPool( iduMutex **aMutexObj, SChar *aMutexName )
{
    iduListNode *sListNode = NULL;

    if( mFreeListCnt > 0 )
    {
        sListNode = IDU_LIST_GET_FIRST(&mFreeList);
        IDU_LIST_REMOVE(sListNode);
        mFreeListCnt--;
        IDU_LIST_ADD_LAST(&mUseList, sListNode);
    }
    else
    {   /* If there is no freed mutex, create new one. */
        IDE_TEST( allocMutexPoolListNode( &sListNode ) != IDE_SUCCESS );
        IDU_LIST_ADD_LAST(&mUseList, sListNode);
    }

    if ( aMutexName != NULL )
    {
        /* Reset statistic info and name of a Mutex */
        ((iduMutex *)(sListNode->mObj))->resetStat();
        ((iduMutex *)(sListNode->mObj))->setName( aMutexName );
    }
    else
    {
        ((iduMutex *)(sListNode->mObj))->resetStat();
        ((iduMutex *)(sListNode->mObj))->setName( (SChar*)"UKNOWN_MUTEX_OBJ" );
    }

    *aMutexObj = (iduMutex *)(sListNode->mObj);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* The function that frees a mutex from mUseList in the mutex pool. */
IDE_RC mmcMutexPool::freeMutexFromPool( iduMutex *aMutexObj )
{
    iduMutex    *sMutex;
    iduListNode *sIterator;
    iduListNode *sDummy;

    /* searched by pointer of iduMutex which attempts to be freed */
    IDU_LIST_ITERATE_SAFE( &mUseList, sIterator, sDummy )
    {
        sMutex = (iduMutex *)(sIterator->mObj);
        if( sMutex == aMutexObj )
        {
            IDU_LIST_REMOVE( sIterator );

            /* SESSION_MUTEXPOOL_FREE_LIST_MAXCNT 
                : maxinum count of free-list elements of the session mutexpool */
            if( mFreeListCnt < mmuProperty::getSessionMutexpoolFreelistMaxcnt() )
            {
                IDU_LIST_ADD_LAST(&mFreeList, sIterator);
                mFreeListCnt++;
            }
            else
            {
                IDE_TEST( sMutex->destroy() != IDE_SUCCESS );
                mMutexPool.memfree(sMutex);
                mListNodePool.memfree(sIterator);
            }

            break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* The function that allocates/initializes iduListNode and iduMutex. */
IDE_RC mmcMutexPool::allocMutexPoolListNode( iduListNode **aListNode )
{
    iduMutex *sMutex    = NULL;

    IDU_FIT_POINT( "mmcMutexPool::allocMutexPoolListNode::alloc::ListNode" );
    IDE_TEST(mListNodePool.alloc((void **)aListNode) != IDE_SUCCESS);

    IDU_FIT_POINT( "mmcMutexPool::allocMutexPoolListNode::alloc::Mutex" );
    IDE_TEST(mMutexPool.alloc((void **)&sMutex) != IDE_SUCCESS);

    IDE_TEST(sMutex->initialize((SChar *)"MMC_SESSION_MUTEXPOOL_MUTEX",
                                IDU_MUTEX_KIND_NATIVE,
                                IDV_WAIT_INDEX_NULL ) != IDE_SUCCESS);
    IDU_LIST_INIT_OBJ(*aListNode, sMutex);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        if( sMutex != NULL )
        {
            mMutexPool.memfree(sMutex);
        }
        if( *aListNode != NULL )
        {
            mListNodePool.memfree(*aListNode);
        }
    }

    return IDE_FAILURE;
}
