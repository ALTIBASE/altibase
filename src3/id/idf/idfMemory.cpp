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
 
/***********************************************************************
 * $Id: idfMemory.cpp 17293 2006-07-25 03:04:24Z sbjang $
 **********************************************************************/

#include <idfMemory.h>

typedef struct Node
{
    iduListNode *a;
} Node;

idfMemory::idfMemory() 
{
}

idfMemory::~idfMemory() 
{
}

IDE_RC
idfMemory::init( UInt                 aPageSize,
                 UInt                 aAlignSize,
                 idBool               aLock )
{
    IDU_LIST_INIT( &mNodeList );

    mAlignSize = aAlignSize;

    mLock = aLock;

    IDE_TEST( idlOS::thread_mutex_init( &mMutex ) != 0 );

    IDE_TEST( alloc( (void **)&mBufferW, 
                     aPageSize ) != IDE_SUCCESS );

    IDE_TEST( alloc( (void **)&mBufferR, 
                     IDF_ALIGNED_BUFFER_SIZE) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
idfMemory::destroy()
{
    IDE_TEST( freeAll() != IDE_SUCCESS );

    IDE_TEST( idlOS::thread_mutex_destroy( &mMutex ) != 0 );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
idfMemory::alloc( void **aBuffer, size_t aSize )
{
    ULong        sSize   = 0;
    SChar       *sBuffer = NULL;
    iduListNode *sNode   = NULL;
    idBool       sLock   = ID_FALSE;

    sSize = ID_SIZEOF(iduListNode) + ID_SIZEOF(vULong) + mAlignSize + aSize;

    sNode = (iduListNode *)iduMemMgr::mallocRaw( sSize );

    IDE_ASSERT( sNode != NULL );

    IDU_LIST_INIT( sNode );

    sBuffer = (SChar *)sNode + ID_SIZEOF(iduListNode);

    *aBuffer = idlOS::align( sBuffer + ID_SIZEOF(vULong), mAlignSize );

    sNode->mObj = sBuffer;

    if( mLock == ID_TRUE )
    {
        IDE_TEST( lock() != 0 );
        sLock = ID_TRUE;
    }

    IDU_LIST_ADD_LAST( &mNodeList, sNode );

    ((Node *)((SChar *)*aBuffer - ID_SIZEOF(vULong)))->a = sNode;

    if( sLock == ID_TRUE )
    {
        /* BUG-39551 */
        sLock = ID_FALSE;
        IDE_TEST( unlock() != 0 );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sLock == ID_TRUE )
    {
        (void)unlock();
    }

    IDE_ASSERT(0);

    return IDE_FAILURE;
}

IDE_RC
idfMemory::cralloc( void **aBuffer, size_t aSize )
{
    IDE_TEST( alloc( aBuffer, aSize ) != IDE_SUCCESS );

    idlOS::memset( *aBuffer,
                   0,
                   aSize );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
idfMemory::free( void *aBuffer )
{
    iduListNode *sNode   = NULL;
    idBool       sLock   = ID_FALSE;

    if( mLock == ID_TRUE )
    {
        IDE_TEST( lock() != 0 );
        sLock = ID_TRUE;
    }

    sNode = ((Node *)((SChar *)aBuffer - ID_SIZEOF(vULong)))->a;

    IDU_LIST_REMOVE( sNode );

    if( sLock == ID_TRUE )
    {
        // BUG-25420 [CodeSonar] Lock, Unlock 에러 핸들링 오류에 의한 Double Unlock
        // unlock 를 하기전에 세팅을해야 Double Unlock 을 막을수 있다.
        sLock = ID_FALSE;
        IDE_TEST( unlock() != 0 );
    }

    (void)iduMemMgr::freeRaw( sNode );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sLock == ID_TRUE )
    {
        (void)unlock();
    }

    IDE_ASSERT(0);

    return IDE_FAILURE;
}

IDE_RC
idfMemory::freeAll()
{
    iduListNode *sIterator = NULL;
    iduListNode *sNodeNext = NULL;

    IDU_LIST_ITERATE_BACK_SAFE( &mNodeList, sIterator, sNodeNext )
    {
        IDU_LIST_REMOVE( sIterator );

        (void)iduMemMgr::freeRaw( sIterator );
    }

    IDU_LIST_INIT( &mNodeList );

    return IDE_SUCCESS;
}

SInt
idfMemory::lock()
{
    return idlOS::thread_mutex_lock( &mMutex );
}

SInt
idfMemory::unlock()
{
    return idlOS::thread_mutex_unlock( &mMutex );
}
