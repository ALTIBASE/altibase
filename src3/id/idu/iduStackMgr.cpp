/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduStackMgr.cpp 66405 2014-08-13 07:15:26Z djin $
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <ideLog.h>
#include <idErrorCode.h>
#include <iduStackMgr.h>

iduStackMgr::iduStackMgr()
{
}

iduStackMgr::~iduStackMgr()
{
}

/***********************************************************************
 * Description : Stack Manager를 최기화 한다.
 *
 * aIndex     - [IN] Memory Manager Index로서 어느 메모리를 사용하는지를
 *                   표시때 사용
 * aItemSize  - [IN] Item의 크기
 **********************************************************************/
IDE_RC iduStackMgr::initialize( iduMemoryClientIndex aIndex,
                                ULong                aItemSize )
{
    mIndex = aIndex;

    IDE_TEST( mMutex.initialize( (SChar *)"STACK_MANAGER_MUTEX",
                                 IDU_MUTEX_KIND_NATIVE,
                                 IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );

    mPageSize   = idlOS::getpagesize();
    mHead.mPrev = &mHead;
    mHead.mNext = &mHead;

    mCurPage          = &mHead;
    mItemCntOfCurPage = 0;
    mTotItemCnt       = 0;
    mItemSize         = aItemSize;

    mMaxItemCntInPage = ( mPageSize - ID_SIZEOF(iduStackPage) + ID_SIZEOF(SChar) )
        / mItemSize;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Stack Manager를 정리한다. 할당된 메모리를 반환하고 Mutex를 정
 *               리한다.
 *
 **********************************************************************/
IDE_RC iduStackMgr::destroy()
{
    iduStackPage  *sCurStackPage;
    iduStackPage  *sNxtStackPage;

    sCurStackPage = mHead.mNext;

    while( sCurStackPage != &mHead )
    {
        sNxtStackPage = sCurStackPage->mNext;

        IDE_TEST( iduMemMgr::free( sCurStackPage )
                  != IDE_SUCCESS );

        sCurStackPage = sNxtStackPage;
    }

    mHead.mNext = &mHead;
    mHead.mPrev = &mHead;

    IDE_TEST( mMutex.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : Stack에서 마지막으로 push된 Item을 꺼낸다. 만약 비어 있으면
 *               aIsEmpty가 ID_TRUE가 Return된다.
 *
 * aLock - [IN] Lock을 잡고 Pop할지 결정.
 *
 * aItem    - [OUT] Pop된 Item이 저장될 변수
 * aIsEmpty - [OUT] Stack이 비어 있으면 ID_TRUE, 아니면 ID_FALSE
 *
 **********************************************************************/
IDE_RC iduStackMgr::pop( idBool   aLock,
                         void   * aItem,
                         idBool * aIsEmpty )
{
    SInt sState = 0;

    *aIsEmpty = ID_FALSE;

    if( aLock == ID_TRUE )
    {
        IDE_TEST( lock() != IDE_SUCCESS );
        sState = 1;
    }

    if( ( mCurPage == mHead.mNext ) && ( mItemCntOfCurPage == 0 ) )
    {
        IDE_ASSERT( mTotItemCnt == 0 );

        *aIsEmpty = ID_TRUE;
    }
    else
    {
        if( mItemCntOfCurPage == 0 )
        {
            mCurPage = mCurPage->mPrev;
            mItemCntOfCurPage        = mMaxItemCntInPage;
        }

        mItemCntOfCurPage--;

        idlOS::memcpy( aItem,
                       mCurPage->mData + (mItemCntOfCurPage * mItemSize),
                       mItemSize );

        mTotItemCnt--;
    }

    if( aLock == ID_TRUE )
    {
        sState = 0;
        IDE_TEST( unlock() != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_PUSH();
        IDE_ASSERT( unlock() == IDE_SUCCESS );
        IDE_POP();
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 새로운 Item을 Stack에 Push한다.
 *
 * aLock - [IN] Lock을 잡고 Push할지 결정.
 * aItem - [IN] Push될 Item
 *
 **********************************************************************/
IDE_RC iduStackMgr::push( idBool aLock, void* aItem )
{
    SInt          sState   = 0;
    iduStackPage *sNewPage = NULL;

    if( aLock == ID_TRUE )
    {
        IDE_TEST( lock() != IDE_SUCCESS );
        sState = 1;
    }

    if( ( mItemCntOfCurPage == mMaxItemCntInPage ) || ( mCurPage == &mHead ) )
    {
        if( mCurPage->mNext == &mHead )
        {
            IDE_TEST( iduMemMgr::malloc( mIndex,
                                         mPageSize,
                                         (void**)&sNewPage,
                                         IDU_MEM_FORCE )
                     != IDE_SUCCESS );

            sNewPage->mPrev = mHead.mPrev;
            sNewPage->mNext = &mHead;

            mHead.mPrev->mNext = sNewPage;
            mHead.mPrev        = sNewPage;

            mCurPage = sNewPage;
        }
        else
        {
            mCurPage = mCurPage->mNext;
        }

        mItemCntOfCurPage = 0;
    }

    idlOS::memcpy( mCurPage->mData + ( mItemCntOfCurPage * mItemSize ),
                   aItem,
                   mItemSize );

    mItemCntOfCurPage++;
    mTotItemCnt++;

    if( aLock == ID_TRUE )
    {
        sState = 0;
        IDE_TEST( unlock() != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_PUSH();
        IDE_ASSERT( unlock() == IDE_SUCCESS );
        IDE_POP();
    }

    return IDE_FAILURE;
}
