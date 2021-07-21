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

#include <dkdDataBufferMgr.h>

UInt    dkdDataBufferMgr::mBufferBlockSize;
UInt    dkdDataBufferMgr::mMaxBufferBlockCnt;
UInt    dkdDataBufferMgr::mUsedBufferBlockCnt;
UInt    dkdDataBufferMgr::mRecordBufferAllocRatio; /* BUG-36895: SDouble -> UInt */

iduMemAllocator    *dkdDataBufferMgr::mAllocator = NULL; /* BUG-37215 */
iduMutex            dkdDataBufferMgr::mDbmMutex;

/***********************************************************************
 * Description: Data buffer manager 를 초기화한다.
 *
 **********************************************************************/
IDE_RC  dkdDataBufferMgr::initializeStatic()
{
    /* >> BUG-37215 */
    idCoreAclMemAllocType   sAllocType      = (idCoreAclMemAllocType)iduMemMgr::getAllocatorType();
    idCoreAclMemTlsfInit    sAllocInit      = {0};
    SInt                    sAllocatorState = 0;

    mAllocator = NULL;

    /* << BUG-37215 */
    mBufferBlockSize    = DKU_DBLINK_DATA_BUFFER_BLOCK_SIZE * DKD_BUFFER_BLOCK_SIZE_UNIT_MB;
    mMaxBufferBlockCnt  = DKU_DBLINK_DATA_BUFFER_BLOCK_COUNT;
    mUsedBufferBlockCnt = 0;

    /* >> BUG-36895 */
    if ( DKU_DBLINK_DATA_BUFFER_ALLOC_RATIO > 0 )
    {
        if ( ( DKU_DBLINK_DATA_BUFFER_ALLOC_RATIO / 100 ) >= 1 )
        {
            mRecordBufferAllocRatio = 100;
        }
        else
        {
            mRecordBufferAllocRatio = DKU_DBLINK_DATA_BUFFER_ALLOC_RATIO;
        }
    }
    else
    {
        mRecordBufferAllocRatio = 0;
    }
    /* << BUG-36895 */

    if( sAllocType != ACL_MEM_ALLOC_LIBC )
    {
        IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_DK,
                                           ID_SIZEOF( iduMemAllocator ),
                                           (void**)&mAllocator,
                                           IDU_MEM_IMMEDIATE )
                        != IDE_SUCCESS, ERR_MEMORY_ALLOC_ALLOCATOR );
        sAllocatorState = 1;

        /* >> BUG-37215 */
        IDE_TEST( iduMemMgr::createAllocator( mAllocator,
                                              sAllocType,
                                              &sAllocInit )
                  != IDE_SUCCESS );
        /* << BUG-37215 */

        sAllocatorState = 2;
    }
    else
    {
        /* do nothing */
    }

    /* Data buffer manager 에 대한 동시접근을 막기 위한 mutex 초기화 */
    IDE_TEST_RAISE( mDbmMutex.initialize( (SChar *)"DKD_DATA_BUFFER_MGR_MUTEX",
                                          IDU_MUTEX_KIND_POSIX,
                                          IDV_WAIT_INDEX_NULL )
                    != IDE_SUCCESS, ERR_MUTEX_INIT );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC_ALLOCATOR )
    {
        IDE_ERRLOG( IDE_DK_0 );
        IDE_SET( ideSetErrorCode( dkERR_ABORT_MEMORY_ALLOCATION,
                                  "dkdDataBufferMgr::initializeStatic",
                                  "mAllocator" ) );
    }
    IDE_EXCEPTION( ERR_MUTEX_INIT );
    {
        IDE_SET( ideSetErrorCode( dkERR_FATAL_ThrMutexInit ) );
    }
    IDE_EXCEPTION_END;

    /* >> BUG-37215 */
    IDE_PUSH();

    switch ( sAllocatorState  )
    {
        case 2:
            (void)iduMemMgr::freeAllocator( mAllocator );
            /* fall through */
        case 1:
            (void)iduMemMgr::free( mAllocator );
            break;
        default:
            break;
    }

    IDE_POP();
    /* << BUG-37215 */

    return IDE_FAILURE;
}

/************************************************************************
 * Description : Data buffer manager 를 정리한다.
 *
 ************************************************************************/
IDE_RC  dkdDataBufferMgr::finalizeStatic()
{
    idCoreAclMemAllocType sAllocType  = (idCoreAclMemAllocType)iduMemMgr::getAllocatorType();

    (void)mDbmMutex.destroy();

    if( sAllocType != ACL_MEM_ALLOC_LIBC )
    {
        /* BUG-37215 */
        (void)iduMemMgr::freeAllocator( mAllocator );

        (void)iduMemMgr::free( mAllocator );
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;
}

/************************************************************************
 * Warning     : 이 함수는 현재 사용하지 않으나 추후 버퍼할당 알고리즘의 
 *               변경을 통해 사용할 수도 있음. 리뷰 후 삭제 여부 결정
 *
 * Description : 입력받은 block 갯수만큼 record buffer 를 할당한다.
 *               할당 가능성 여부는 caller 에서 한다. 따라서 aBlockCnt
 *               가 0 인 경우는 FAILURE 로 처리한다.
 *
 *  aSize       - [OUT] 할당받은 record buffer size
 *  aRecBuf     - [OUT] 할당받은 record buffer 를 가리키는 포인터 
 *
 ************************************************************************/
IDE_RC  dkdDataBufferMgr::allocRecordBuffer( UInt  *aSize, void  **aRecBuf )
{
    UInt        sAllocRecBufSize;
    UInt        sAllocableBlkCnt;
    void       *sRecBuf = NULL;

    IDE_ASSERT( mDbmMutex.lock( NULL /*idvSQL* */ ) == IDE_SUCCESS );

    sAllocableBlkCnt = getAllocableBufferBlockCnt();
    sAllocRecBufSize = sAllocableBlkCnt * mBufferBlockSize;

    if ( sAllocRecBufSize > 0 )
    {
        /* TODO: */
    }
    else
    {
        sRecBuf = NULL;
    }

    IDE_ASSERT( mDbmMutex.unlock() == IDE_SUCCESS );

    *aSize   = sAllocRecBufSize;
    *aRecBuf = sRecBuf;

    return IDE_SUCCESS;
}

/************************************************************************
 * Warning     : 이 함수는 현재 사용하지 않으나 추후 버퍼할당 알고리즘의 
 *               변경을 통해 사용할 수도 있음. 리뷰 후 삭제 여부 결정
 *
 * Description : 입력받은 record buffer 를 반환한다.
 *
 *  aRecBuf     - [IN] 반환할 record buffer 를 가리키는 포인터 
 *  aBlockCnt   - [IN] 반환할 buffer block 의 갯수
 *
 ************************************************************************/
IDE_RC  dkdDataBufferMgr::deallocRecordBuffer( void *aRecBuf, UInt aBlockCnt )
{
    IDE_ASSERT( aRecBuf != NULL );
    IDE_ASSERT( mDbmMutex.lock( NULL /*idvSQL* */ ) == IDE_SUCCESS );

    (void)iduMemMgr::free( aRecBuf );

    mUsedBufferBlockCnt -= aBlockCnt;

    IDE_ASSERT( mDbmMutex.unlock() == IDE_SUCCESS );

    return IDE_SUCCESS;
}


