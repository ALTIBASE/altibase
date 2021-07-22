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
 * $Id: iduGrowingMemoryHandle.cpp 15368 2006-03-23 01:14:43Z leekmo $
 **********************************************************************/

#include <idl.h>
#include <iduMemMgr.h>
#include <iduGrowingMemoryHandle.h>


// 객체 생성자,파괴자 => 아무일도 수행하지 않는다.
iduGrowingMemoryHandle::iduGrowingMemoryHandle()
{
}
iduGrowingMemoryHandle::~iduGrowingMemoryHandle()
{
}

/*
  Growing Memory Handle을 초기화 한다.

  [IN] aMemoryClient - iduMemMgr에 넘길 메모리 할당 Client
  [IN] aChunkSize - 메모리 할당의 단위인 Chunk의 크기
*/
IDE_RC iduGrowingMemoryHandle::initialize(
           iduMemoryClientIndex   aMemoryClient,
           ULong                  aChunkSize )
{
    /* 객체의 virtual function table초기화 위해서 new를 호출 */
    new (this) iduGrowingMemoryHandle();
    
#ifdef __CSURF__
    IDE_ASSERT( this != NULL );
#endif

    /* 메모리 할당기 초기화 */
    mAllocator.init( aMemoryClient, aChunkSize);
    
    // prepareMemory를 호출하여 할당받아간 메모리 크기의 총합
    mTotalPreparedSize = 0;
        
    return IDE_SUCCESS;
}
    
/*
  Growing Memory Handle을 파괴 한다.
  
  [IN] aHandle - 파괴할 Memory Handle
*/
IDE_RC iduGrowingMemoryHandle::destroy()
{
    /* 메모리 할당기가 할당한 모든 Memory Chunk를 할당해제 */
    mAllocator.destroy();
   
    return IDE_SUCCESS;

}

/*
  Growing Memory Handle이 할당한 메모리를 전부 해제
  
  [IN] aHandle - 파괴할 Memory Handle
*/
IDE_RC iduGrowingMemoryHandle::clear()
{
    /* 메모리 할당기가 할당한 모든 Memory Chunk를 할당해제 */
    mAllocator.clear();

    // prepareMemory를 호출하여 할당받아간 메모리 크기의 총합
    mTotalPreparedSize = 0;
    
    return IDE_SUCCESS;

}

    
/*
  Growing Memory Handle에 aSize이상의 메모리를 사용가능하도록 준비한다.
  
  항상 새로운 메모리를 할당기로부터 할당하여 리턴한다.

  [IN] aSize   - 준비할 메모리 공간의 크기
  [OUT] aPreparedMemory - 준비된 메모리의 주소
*/
IDE_RC iduGrowingMemoryHandle::prepareMemory( UInt    aSize,
                                             void ** aPreparedMemory)
{
    IDE_DASSERT( aSize > 0 );
    IDE_DASSERT( aPreparedMemory != NULL );

    IDE_TEST( mAllocator.alloc( aSize, aPreparedMemory)
              != IDE_SUCCESS );

    mTotalPreparedSize += aSize;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
    이 Memory Handle을 통해 OS로부터 할당받은 메모리의 총량을 리턴
 */
ULong iduGrowingMemoryHandle::getSize()
{
    return mTotalPreparedSize;
}




