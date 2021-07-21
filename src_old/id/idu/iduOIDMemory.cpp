/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduOIDMemory.cpp 67683 2014-11-24 10:39:13Z djin $
 *
 *  BUG-22877 동시성 문제로 alloc과 free 별도로 있던 mutex를 하나로 줄였다.
 *  기존에 두 mutex로 제어하기 위해 allock list의 첫 page를 free하지
 *  않고 남겨 두었다가, free할 수 있게 되었을 때 모아서 shrink하였으나,
 *  이제 첫 page도 무조건 free하여 남겨지는 page가 없으므로
 *  한 page씩만 free된다. shrink()를 제거하고 memFree()에서
 *  직접 page를 free하도록 변경하였다.
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <idErrorCode.h>
#include <ideLog.h>
#include <iduOIDMemory.h>

/******************************************************************************
 * Description :
 ******************************************************************************/
iduOIDMemory::iduOIDMemory()
{
}

/******************************************************************************
 * Description :
 ******************************************************************************/
iduOIDMemory::~iduOIDMemory()
{
}

/******************************************************************************
 * Description : 맴버 변수 초기화
 * aIndex     - [IN] Memory Client Index
 * aElemSize  - [IN] element의 하나의 크기
 * aElemCount - [IN] 한 page당 element의 수
 ******************************************************************************/
IDE_RC iduOIDMemory::initialize(iduMemoryClientIndex aIndex,
                                vULong               aElemSize,
                                vULong               aElemCount )
{
    mIndex = aIndex;

    // 1. Initialize Item Size
    mElemSize = idlOS::align( aElemSize, sizeof(SDouble) );
    mItemSize = idlOS::align( mElemSize + sizeof(iduOIDMemItem), sizeof(SDouble) );

    // 2. Initialize Item Count and Page Size
    mItemCnt  = aElemCount;
    mPageSize = idlOS::align( sizeof(iduOIDMemAllocPage) + (mItemSize * mItemCnt),
                              sizeof(SDouble) );

    // 3. Initialize Other Members
    mAllocPageHeader.prev = &mAllocPageHeader;
    mAllocPageHeader.next = &mAllocPageHeader;
    mAllocPageHeader.freeIncCount  = 0;
    mAllocPageHeader.allocIncCount = 0;
    mPageCntInAllocLst    = 0;

    mFreePageHeader.next = NULL;
    mPageCntInFreeLst    = 0;

    // 4. Initialize Mutex
    IDE_TEST_RAISE( mMutex.initialize((SChar *)"OID_MEMORY_MUTEX",
                                           IDU_MUTEX_KIND_NATIVE,
                                           IDV_WAIT_INDEX_NULL )
                    != IDE_SUCCESS,error_mutex_init );

    return IDE_SUCCESS;

    IDE_EXCEPTION(error_mutex_init);
    {
        IDE_SET_AND_DIE(ideSetErrorCode(idERR_FATAL_ThrMutexInit));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/******************************************************************************
 * Description : 멤버 변수 종료 및 메모리 헤제
 ******************************************************************************/
IDE_RC iduOIDMemory::destroy()
{
    iduOIDMemAllocPage * sNxtAllocPage;
    iduOIDMemAllocPage * sCurAllocPage;
    iduOIDMemFreePage  * sNxtFreePage;
    iduOIDMemFreePage  * sCurFreePage;

    // alloc page list 메모리 헤제
    sCurAllocPage = mAllocPageHeader.prev;

    while( sCurAllocPage != &mAllocPageHeader )
    {
        sNxtAllocPage = sCurAllocPage->next;
        IDE_TEST(iduMemMgr::free(sCurAllocPage)
                 != IDE_SUCCESS);
        sCurAllocPage = sNxtAllocPage;
    }

    mAllocPageHeader.next = &mAllocPageHeader;
    mAllocPageHeader.prev = &mAllocPageHeader;

    // free page list 메모리 헤제
    sCurFreePage = mFreePageHeader.next;

    while( sCurFreePage != NULL )
    {
        sNxtFreePage = sCurFreePage->next;

        IDE_TEST(iduMemMgr::free(sCurFreePage)
                 != IDE_SUCCESS);

        sCurFreePage = sNxtFreePage;
    }

    mFreePageHeader.next = NULL;

    IDE_TEST_RAISE( mMutex.destroy() != IDE_SUCCESS,
                    error_mutex_destroy );

    return IDE_SUCCESS;

    IDE_EXCEPTION(error_mutex_destroy);
    {
        IDE_SET_AND_DIE(ideSetErrorCode(idERR_FATAL_ThrMutexDestroy));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/******************************************************************************
 * Description : aMem에 메모리 할당
 * aMem  - [OUT] 할당받은 메모리주소를 반환
 ******************************************************************************/
IDE_RC iduOIDMemory::alloc(void ** aMem)
{
    vULong sHeaderSize;
    UInt   sState = 0;

    sHeaderSize = idlOS::align( sizeof(iduOIDMemAllocPage), sizeof(SDouble) );
    *aMem = NULL;

    // 1. Lock 획득
    IDE_ASSERT( lockMtx() == IDE_SUCCESS );
    sState = 1;

    while( *aMem == NULL )
    {
        // 2. alloc page list에 free item이 있는지 확인
        if( (mAllocPageHeader.next != &mAllocPageHeader) &&
            (mAllocPageHeader.next->allocIncCount < mItemCnt) )
        {
            *aMem = (iduOIDMemItem *)
                    ((SChar *)mAllocPageHeader.next + sHeaderSize +
                     (mAllocPageHeader.next->allocIncCount * mItemSize));
            mAllocPageHeader.next->allocIncCount++;
        }
        else
        {
            // 2. alloc page list에 page가 하나도 없거나
            //    page가 있더라고 free item이 없을 경우,
            //    kernel or free page list로 부터 메모리를 할당받는다.
            IDE_TEST( grow() != IDE_SUCCESS );
        }

    } //while

    // 3. Lock 해제
    sState = 0;
    IDE_ASSERT( unlockMtx() == IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if(sState != 0)
    {
        IDE_ASSERT( unlockMtx() == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/******************************************************************************
 * Description : aMem의 메모리 반납
 * aMem  - [IN] 반납할 element 메모리주소
 ******************************************************************************/
IDE_RC iduOIDMemory::memfree(void * aMem)
{
    iduOIDMemAllocPage * sFreePage;
    iduOIDMemFreePage  * sFreePage2OS = NULL;
    iduOIDMemItem      * sMemItem;

    IDE_ASSERT( lockMtx() == IDE_SUCCESS );

    sMemItem  = (iduOIDMemItem *)((SChar *)aMem + mElemSize);
    sFreePage = sMemItem->myPage;

    // 1. 해당 page의 freeIncCount 증가.
    sFreePage->freeIncCount++;

    // 2 해당 page의 freeIncCount와 mItemCnt 비교
    if( sFreePage->freeIncCount == mItemCnt )
    {
        // remove from alloc page list
        sFreePage->next->prev = sFreePage->prev;
        sFreePage->prev->next = sFreePage->next;

        // count 저장
        mPageCntInAllocLst--;

        // free page count가
        // IDU_OID_MEMORY_AUTOFREE_LIMIT 이상이면 kernel로 반납.

        if( mPageCntInFreeLst > IDU_OID_MEMORY_AUTOFREE_LIMIT )
        {
            // lock잡고 free하는 것을 피하기 위해 별도로 저장해
            // 두었다가 이후에 lock을 풀고 free 한다.
            sFreePage2OS = (iduOIDMemFreePage*)sFreePage;
        }
        else
        {
            //  free page list에 연결
            ((iduOIDMemFreePage*)sFreePage)->next
                                 = mFreePageHeader.next;
            mFreePageHeader.next = (iduOIDMemFreePage*)sFreePage;

            // count 저장
            mPageCntInFreeLst++;
        }
    }
    else
    {
        // 해당 page에 아직 free 되지 않은 Item들이 남아 있다.
        IDE_ASSERT( sFreePage->freeIncCount < mItemCnt );
    }

    IDE_ASSERT( unlockMtx() == IDE_SUCCESS );

    if( sFreePage2OS != NULL )
    {
        IDE_TEST( iduMemMgr::free( sFreePage2OS )
                  != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    IDE_ASSERT( unlockMtx() == IDE_SUCCESS );

    return IDE_FAILURE;
}

/******************************************************************************
 * Description : mFreePageHeader or kernel로부터 메모리 할당.
 ******************************************************************************/
void iduOIDMemory::dumpState(SChar * aBuffer, UInt aBufferSize)
{
    idlOS::snprintf( aBuffer, aBufferSize, "MemMgr Used Memory:%"ID_UINT64_FMT"\n",
                     ((mPageCntInAllocLst + mPageCntInFreeLst ) * mPageSize) );
}

/******************************************************************************
 * Description :
 ******************************************************************************/
void iduOIDMemory::status()
{
    SChar sBuffer[IDE_BUFFER_SIZE];

    IDE_CALLBACK_SEND_SYM_NOLOG( "     Mutex Internal State\n" );
    mMutex.status();

    idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer),
                     " Memory Usage: %"ID_UINT32_FMT" KB\n",
                     (UInt)((mPageCntInAllocLst + mPageCntInFreeLst ) * mPageSize)/1024 );
    IDE_CALLBACK_SEND_SYM_NOLOG( sBuffer );
}


/******************************************************************************
 * Description :
 ******************************************************************************/
ULong iduOIDMemory::getMemorySize()
{
    ULong sSize;

    sSize = ( mPageCntInAllocLst + mPageCntInFreeLst ) * mPageSize;

    return sSize;
}

/******************************************************************************
 * Description : mFreePageHeader or kernel로부터 메모리 할당.
 ******************************************************************************/
IDE_RC iduOIDMemory::grow()
{
    vULong i;
    iduOIDMemAllocPage * sMemPage;
    iduOIDMemItem      * sMemItem;
    vULong               sPageHeaderSize;

    sPageHeaderSize = idlOS::align( sizeof(iduOIDMemAllocPage), sizeof(SDouble) );

    // 1. free page list에 페이지가 존재하는지 검사.
    if( mFreePageHeader.next != NULL )
    {
        // 2. alloc page list에 할당할 page 주소저장 & free page list에서 page 제거
        sMemPage = (iduOIDMemAllocPage*)mFreePageHeader.next;
        mFreePageHeader.next = mFreePageHeader.next->next;

        // count 저장
        IDE_ASSERT( mPageCntInFreeLst > 0 );
        mPageCntInFreeLst--;
    }
    else
    {
        // 2. free page list에 페이지가 하나만 존재할 경우, 커널로부터 할당.
        IDE_TEST(iduMemMgr::malloc(mIndex,
                                   mPageSize,
                                   (void**)&sMemPage,
                                   IDU_MEM_FORCE)
                 != IDE_SUCCESS);

        // 각 element들의 iduMemItem 초기화
        for( i = 0; i < mItemCnt; i++ )
        {
            sMemItem = (iduOIDMemItem *)((UChar *)sMemPage + sPageHeaderSize
                       + (i * mItemSize) + mElemSize);
            sMemItem->myPage = sMemPage;
        }
    }

    // 3. alloc page list에 연결
    sMemPage->freeIncCount  = 0;
    sMemPage->allocIncCount = 0;
    sMemPage->next = mAllocPageHeader.next;
    sMemPage->prev = mAllocPageHeader.next->prev;

    mAllocPageHeader.next->prev = sMemPage;
    mAllocPageHeader.next = sMemPage;

    // count 저장
    mPageCntInAllocLst++;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
