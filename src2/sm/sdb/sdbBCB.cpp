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
 * PROJ-1568 BUFFER MANAGER RENEWAL
 ***********************************************************************/

/***********************************************************************
 * $$Id:$
 **********************************************************************/
#include <smDef.h>
#include <sdbDef.h>
#include <sdbBCB.h>
#include <smErrorCode.h>

ULong sdbBCB::mTouchUSecInterval;

/***********************************************************************
 * Description :
 *  aFrameMemHandle - [IN] mFrame에 대한 MemoryHandle로서 Free시에 사용한다. 
 *                         (참조:iduMemPool2)
 *  aFrame          - [IN] frame pointer
 *  aBCBID          - [IN] BCB 식별자
 ***********************************************************************/
IDE_RC sdbBCB::initialize( void   *aFrameMemHandle,
                           UChar  *aFrame,
                           UInt    aBCBID )
{
    SChar sMutexName[128];

    IDE_ASSERT( aFrame != NULL );

    /*[BUG-22041] mSpaceID와 mPageID는 BCB의 상태가 free인 이상
     * 이 두값은 의미가 없어 초기화를 하지 않았지만 umr이 발생하는 경우가
     * 있어 0으로 초기화를 한다.*/
    mSpaceID        = 0;
    mPageID         = 0;
    mID             = aBCBID;
    mFrame          = aFrame;
    mFrameMemHandle = aFrameMemHandle;

    setBCBPtrOnFrame( (sdbFrameHdr*)aFrame, this );

    idlOS::snprintf( sMutexName, 
                     ID_SIZEOF(sMutexName),
                     "BCB_LATCH_%"ID_UINT32_FMT, 
                     aBCBID );

    IDE_ASSERT( mPageLatch.initialize( sMutexName )
                == IDE_SUCCESS );

    idlOS::snprintf( sMutexName, 128, "BCB_MUTEX_%"ID_UINT32_FMT, aBCBID );


    /*
     * BUG-28331   [SM] AT-P03 Scalaility-*-Disk-* 에서의 성능 저하 분석 
     *             (2009년 11월 21일 이후)
     */
    IDE_TEST( mMutex.initialize( sMutexName,
                                 IDU_MUTEX_KIND_NATIVE,
                                 IDV_WAIT_INDEX_LATCH_FREE_DRDB_BCB_MUTEX )
              != IDE_SUCCESS );

    idlOS::snprintf( sMutexName, 128, "BCB_READ_IO_MUTEX_%"ID_UINT32_FMT, aBCBID );

    IDE_TEST( mReadIOMutex.initialize(
                  sMutexName,
                  IDU_MUTEX_KIND_NATIVE,
                  IDV_WAIT_INDEX_LATCH_FREE_DRDB_BCB_READ_IO_MUTEX )
              != IDE_SUCCESS );

    /* BUG-24092: [SD] BufferMgr에서 BCB의 Latch Stat을 갱신시 Page Type이
     * Invalid하여 서버가 죽습니다.
     *
     * mPageType은 원하는 Page가 Buffer에 존재하지 않을 경우 ReadPage가
     * 완료후 Set한다. 때문에 ReadPage시 같은 BCB를 요청하는 타 Thread는
     * 대기하는데 이때 mPageType에 대한 통계정보를 갱신한다. 하지만 mPageType
     * 에 대한 정보가 아직 설정되지 않을 수 있다. 이 경우 mPageType을 0으로
     * 읽게한다. 때문에 통계정보가 정확하지 않을 수 있다. 나머지 Member들도
     * 방어적인 차원에서 초기화 합니다. */
    mPageType = SDB_NULL_PAGE_TYPE;

    mState    = SDB_BCB_FREE;
    mSpaceID  = 0;
    mPageID   = 0;

    SM_LSN_INIT( mRecoveryLSN );

    mBCBListType   = SDB_BCB_NO_LIST;
    mBCBListNo     = 0;
    mCPListNo      = 0;
    mTouchCnt      = 0;
    mFixCnt        = 0;
    mReadyToRead   = ID_FALSE;
    mPageReadError = ID_FALSE;
    mWriteCount    = 0;
    mHashBucketNo  = 0;
    /* PROJ-2102 Secondary Buffer */
    mSBCB          = NULL;

    /* BUG-47945 cp list 디버깅 정보 보강
     * setToFree() 함수에서 이전 값 확인하도록 변경되어서,
     * 처음 초기화는 직접 합니다. */
    makeFreeExceptListItem();

    SDB_INIT_BCB_LIST( this );
    SDB_INIT_CP_LIST( this );

    SMU_LIST_INIT_NODE( &mBCBListItem );
    mBCBListItem.mData = this;

    SMU_LIST_INIT_NODE( &mCPListItem );
    mCPListItem.mData  = this;

    SMU_LIST_INIT_NODE( &mHashItem );
    mHashItem.mData    = this;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description :
 *  sdbBCB 소멸자.
 ***********************************************************************/
IDE_RC sdbBCB::destroy()
{
    IDE_ASSERT(mPageLatch.destroy() == IDE_SUCCESS);

    IDE_ASSERT(mMutex.destroy() == IDE_SUCCESS);

    IDE_ASSERT(mReadIOMutex.destroy() == IDE_SUCCESS);

    return IDE_SUCCESS;
}

/***********************************************************************
 * Description :
 *  sm 로그에 dump (BUG-47945 cp list 디버깅 정보 보강)
 * 
 *  aBCB    - [IN]  BCB
 ***********************************************************************/
void sdbBCB::dump()
{
    ideLog::log( IDE_ERR_0,
                 SM_TRC_BUFFER_POOL_BCB_INFO,
                 mID,
                 mState,
                 mFrame,
                 mSpaceID,
                 mPageID,
                 mPageType,
                 mRecoveryLSN.mFileNo,
                 mRecoveryLSN.mOffset,
                 mBCBListType,
                 mBCBListNo,
                 mCPListNo,
                 mTouchCnt,
                 mFixCnt,
                 mReadyToRead,
                 mPageReadError,
                 mWriteCount );
}

/***********************************************************************
 * Description :
 *  SDB_BCB_CLEAN 상태로 무조건 변경한다.
 ***********************************************************************/
void sdbBCB::clearDirty()
{
    SM_LSN_INIT( mRecoveryLSN );

    mState = SDB_BCB_CLEAN;
}

/***********************************************************************
 * Description :
 *  두 BCB가 mPageID와 mSpaceID가 같은지 비교한다.
 *
 *  aLhs    - [IN]  BCB
 *  aRhs    - [IN]  BCB
 ***********************************************************************/
idBool sdbBCB::isSamePageID( void *aLhs, void *aRhs )
{
    if( (((sdbBCB*)aLhs)->mPageID  == ((sdbBCB*)aRhs)->mPageID ) &&
        (((sdbBCB*)aLhs)->mSpaceID == ((sdbBCB*)aRhs)->mSpaceID ))
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}
