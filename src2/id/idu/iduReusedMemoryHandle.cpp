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
 * $Id: iduReusedMemoryHandle.cpp 15368 2006-03-23 01:14:43Z leekmo $
 **********************************************************************/

#include <idl.h>
#include <iduMemMgr.h>
#include <iduReusedMemoryHandle.h>


// 객체 생성자,파괴자 => 아무일도 수행하지 않는다.
iduReusedMemoryHandle::iduReusedMemoryHandle()
{
}
iduReusedMemoryHandle::~iduReusedMemoryHandle()
{
}


/*
  Resizable Memory Handle을 초기화 한다.

  [IN] aMemoryClient - iduMemMgr에 넘길 메모리 할당 Client
*/
IDE_RC iduReusedMemoryHandle::initialize( iduMemoryClientIndex   aMemoryClient )
{
    /* 객체의 virtual function table초기화 위해서 new를 호출 */
    new (this) iduReusedMemoryHandle();
   
#ifdef __CSURF__
    IDE_ASSERT( this != NULL );
#endif   
    
    /* 크기 0, 메모리 NULL로 초기화 */
    mSize   = 0;
    mMemory = NULL;
    mMemoryClient = aMemoryClient;
    
    assertConsistency();
    
    return IDE_SUCCESS;
}
    
/*
  Resizable Memory Handle을 파괴 한다.
  
  [IN] aHandle - 파괴할 Memory Handle
*/
IDE_RC iduReusedMemoryHandle::destroy()
{
    assertConsistency();

    if ( mSize > 0 )
    {
        IDE_TEST( iduMemMgr::free( mMemory ) != IDE_SUCCESS );
        mMemory = NULL;
        mSize = 0;
    }
    
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


    
/*
  Resizable Memory Handle에 aSize이상의 메모리를 사용가능하도록 준비한다.
  이미 aSize이상 메모리가 할당되어 있다면 아무일도 하지 않는다.

  [참고] 메모리단편화 현상을 방지하기 위해 2의 N승의 크기로만 할당한다.

  [IN] aSize   - 준비할 메모리 공간의 크기
  [OUT] aPreparedMemory - 준비된 메모리의 주소
*/

IDE_RC iduReusedMemoryHandle::prepareMemory( UInt aSize,
                                             void ** aPreparedMemory )
{
    IDE_DASSERT( aSize > 0 );
    IDE_DASSERT( aPreparedMemory != NULL);
    
    
    UInt aNewSize ;
    
    assertConsistency();

    if ( mSize < aSize )
    {
        aNewSize = alignUpPowerOfTwo( aSize );

        if ( mSize == 0 )
        {
            IDE_TEST( iduMemMgr::malloc( mMemoryClient,
                                         aNewSize,
                                         & mMemory )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( iduMemMgr::realloc( mMemoryClient,
                                          aNewSize,
                                          & mMemory )
                      != IDE_SUCCESS );
        }
        
        mSize = aNewSize;
        
        assertConsistency();
    }

    *aPreparedMemory = mMemory;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
    이 Memory Handle을 통해 할당받은 메모리의 총량을 리턴
 */
ULong iduReusedMemoryHandle::getSize( void )
{
    assertConsistency();
    
    return mSize;
}


/*
  Resizable Memory Handle의 적합성 검토(ASSERT)
*/
void iduReusedMemoryHandle::assertConsistency()
{
    if ( mSize == 0 )
    {
        IDE_ASSERT( mMemory == NULL );
    }
    else
    {
        IDE_ASSERT( mMemory != NULL );
    }
    
    
    if ( mMemory == NULL )
    {
        IDE_ASSERT( mSize == 0 );
    }
    else
    {
        IDE_ASSERT( mSize > 0 );
    }
}

/*
  aSize를 2의 N승의 크기로 Align up한다.
  
  ex> 1000 => 1024
      2017 => 2048

  [IN] aSize - Align Up할 크기
*/
UInt iduReusedMemoryHandle::alignUpPowerOfTwo( UInt aSize )
{
    IDE_ASSERT ( aSize > 0 );

    --aSize;
    
    aSize |= aSize >> 1;
    aSize |= aSize >> 2;
    aSize |= aSize >> 4;
    aSize |= aSize >> 8;
    aSize |= aSize >> 16;
    
    return ++aSize ;
}

/*
  Resizable Memory Handle 에 일정 이상의 Size의 Memory가 할당 되어 있을경우
  새로운 Size를 가지도록 재조정 해준다.

  [참고] 메모리단편화 현상을 방지하기 위해 2의 N승의 크기로만 할당한다.

  [IN] aSize   - 준비할 메모리 공간의 크기 ( 0보다 커야만 한다 )
*/

IDE_RC iduReusedMemoryHandle::tuneSize( UInt aSize )
{
    IDE_ASSERT( aSize > 0 );
    
    UInt sNewSize ;
    
    assertConsistency();

    /* aSize로 2의 N승이 아닌 값이 넘어 올경우 aSize로 mSize를 먼저 계산하면
     * mSize = 1024, aSize = 1000 같은 경우 같은 Size를 재할당 하려 할수 있으므로
     * alignUp을 먼저 수행해 준다.
     * 내부적으로 aSize가 0일 경우 ASSERT 이므로 aSize는 항상 0보다 커야 한다. */
    sNewSize = alignUpPowerOfTwo( aSize );

    if ( mSize > sNewSize )
    {
        /* sNewSize는 항상 0보다 커야되므로 mSize가 0 즉 NULL일 경우는 없다.  */
        IDE_TEST( iduMemMgr::free( mMemory ) != IDE_SUCCESS );
        mMemory = NULL;
        mSize = 0;

        IDE_TEST( iduMemMgr::malloc( mMemoryClient,
                                     sNewSize,
                                     &mMemory )
                  != IDE_SUCCESS );
        
        mSize = sNewSize;
        
        assertConsistency();
    }
   
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

