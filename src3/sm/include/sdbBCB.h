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
 * $Id:$
 **********************************************************************/

#ifndef _O_SDB_BCB_MGR_H_
#define _O_SDB_BCB_MGR_H_ 1

#include <smDef.h>
#include <sdbDef.h>

#include <iduLatch.h>
#include <iduMutex.h>
#include <idv.h>
#include <idl.h>

class sdbBCB;
class sdsBCB;

/* --------------------------------------------------------------------
 * 특정 BCB가 어떠한 조건을 만족하는지 여부를
 * 검사할때 사용
 * ----------------------------------------------------------------- */
typedef idBool (*sdbFiltFunc)( void *aBCB, void *aFiltAgr );

typedef enum
{
    SDB_BCB_NO_LIST = 0,
    SDB_BCB_LRU_HOT,
    SDB_BCB_LRU_COLD,
    SDB_BCB_PREPARE_LIST,
    SDB_BCB_FLUSH_LIST,
    SDB_BCB_DELAYED_FLUSH_LIST      /* PROJ-2669 */
} sdbBCBListType;

#define SDB_BCB_LIST_NONE      ID_UINT_MAX
#define SDB_CP_LIST_NONE       ID_UINT_MAX

/* --------------------------------------------------------------------
 * BCB의 상태 정의
 * ----------------------------------------------------------------- */
typedef enum
{
    // 현재 사용되지 않는 상태. hash에서 제거되어 있다.
    // mPageID와 mSpaceID를 신뢰할 수 없다.
    SDB_BCB_FREE = 0,

    // 현재 hash에서 접근할 수 있는 상태. 하지만 내용이 변경되지 않았기
    // 때문에, 디스크 IO없이 그냥 replace가 가능하다.
    SDB_BCB_CLEAN,

    // 현재 hash에서 접근할 수 있으며, 내용이 변경된 상태.
    // replace 를 위해선 디스크에 flush가 필요
    SDB_BCB_DIRTY,

    // flusher가 flush를 위해 자신의 내부 버퍼(IOB)에 현 BCB
    // 내용을 저장한 상태. 절대로 replace되어서는 안된다.
    SDB_BCB_INIOB,

    // SDB_BCB_INIOB 상태에서 어떤 트랜잭션이 내용을 변경한 상태.
    SDB_BCB_REDIRTY
} sdbBCBState;

//BUG-48042: Page, BCB, BCB 리스트 노드 세트
typedef struct
{
    UChar       *mTmpFrame;
    sdbBCB      *mTmpBCB;
    smuList     *mTmpNode;
    void        *mTmpFrameMemHandle;
} sdbBuffAreaPtrSet;

//BCB-48042: Parallel 할당을 위한 Job
typedef struct
{
    UInt              mStartBCBID;
    UInt              mJobCnt;
    sdbBuffAreaPtrSet *mPtrSet;
} sdbBuffAreaJobInfo; 

class sdbBCB
{
public:
    // PROJ-2102 공통 부분 
    SD_BCB_PARAMETERS

    // BCB 고유 ID
    UInt            mID;

    // BCB의 상태
    sdbBCBState     mState;
    sdbBCBState     mPrevState;

    // 하나의 Buffer Frame의 주소를 가르킨다. 버퍼 frame의 주소는 페이지
    // size로 align되어있다.
    UChar          *mFrame;

    // mFrame에 대한 MemoryHandle로서 Free시에 사용한다. (참조:iduMemPool2)
    void           *mFrameMemHandle;

    // To fix BUG-13462
    // BCB frame의 페이지 타입
    // 기 정의된 상위 레이어의 페이지 타입을 모르기 때문에 UInt를 사용
    UInt            mPageType;

    // sdbLRUList 또는 sdbFlushList 또는 sdbPrepareList중 하나에 속할 수 있다.
    // List를 위한 자료구조
    smuList         mBCBListItem;
    sdbBCBListType  mBCBListType;
    
    // 다중화된 리스트중에서 자신이 속한 리스트의 식별자
    UInt            mBCBListNo;

    // BCB를 접근한 횟수
    UInt            mTouchCnt;

    // mState를 변경하기 위해서, 또는 mFixCount와 mTouchCnt를 변경하기 위해서
    // 잡아야 하는 mutex. 보통 BCBMutex라고 하면 이것을 뜻한다.
    iduMutex        mMutex;
    
    // 페이지에 대한 래치. 페이지 자체에 대한 동시성 제어
    iduLatch     mPageLatch;
    
    // 페이지 래치를 잡지 않는 fixPage연산을 위한 mutex.
    iduMutex        mReadIOMutex;
    
    // 현 BCB를 가장 마지막에 mTouchCnt를 변경한 시간
    idvTime         mLastTouchedTime;

    /* PROJ-2669 실제 마지막 BCB Touch 시간 */
    idvTime         mLastTouchCheckTime;
    
    // 현 BCB에 대해서 fix하고 있는 쓰레드 갯수
    UInt            mFixCnt;

    // 페이지를 읽어도 되는지 여부
    idBool          mReadyToRead;
    
    /* 페이지를 디스크에서 읽어왔는데, 이것이 에러일 경우에 여기에
     * ID_TRUE 로 설정한다. 이것이 필요한 이유는 no latch로 페이지를
     * 접근하는 쓰레드가 현재 페이지가 에러가 난 페이지인지 알아야 하기
     * 때문이다.*/
    idBool          mPageReadError;

    // 통계정보를 위한 자료
    idvTime         mCreateOrReadTime;   // 마지막 createPage 또는 loadPage한 시간
    UInt            mWriteCount;
    // PROJ-2102 : 대응 되는 Secondary Buffer BCB
    sdsBCB        * mSBCB;

public:
    static ULong mTouchUSecInterval;

public:
    /* 주의!!
     * 여기있는 메소드들은 어떠한것도 동시성을 보장해주지 않는다.
     * 외부에서 동시성을 컨트롤 해야 한다.
     * */
    IDE_RC initialize(void *aFrameMemHandle, UChar *aFrame, UInt aBCBID);

    IDE_RC destroy();

    //dirty상태를 clean상태로 변경하고, 관련된 변수를 초기화 한다.
    void clearDirty();

    inline idBool isFree();
    inline void setToFree();
    inline void makeFreeExceptListItem();

    inline void clearTouchCnt();

    inline void updateTouchCnt();

    inline void lockPageXLatch(idvSQL *aStatistics);
    inline void lockPageSLatch(idvSQL *aStatistics);
    inline void tryLockPageXLatch( idBool *aSuccess);
    inline void tryLockPageSLatch( idBool *aSuccess );

    inline void unlockPageLatch(void);

    inline void lockBCBMutex(idvSQL *aStatistics);
    inline void unlockBCBMutex();

    inline void lockReadIOMutex(idvSQL *aStatistics);
    inline void unlockReadIOMutex(void);

    inline void incFixCnt();
    inline void decFixCnt();

    static  idBool isSamePageID(void *aLhs,
                                void *aRhs);

    void dump();
    static inline IDE_RC dump(void *aBCB);

    static inline void setBCBPtrOnFrame( sdbFrameHdr   *aFrame,
                                         sdbBCB        *aBCBPtr);

    static inline void setSpaceIDOnFrame( sdbFrameHdr   *aFrame,
                                          scSpaceID      aSpaceID);

    static inline void setPageLSN( sdbFrameHdr    *aFrame,
                                   smLSN           aLSN );
    
    inline void lockBufferAreaX(idvSQL   *aStatistics);
    inline void lockBufferAreaS(idvSQL   *aStatistics);

};

idBool sdbBCB::isFree()
{
    if( (mState == SDB_BCB_FREE ) &&
        (mFixCnt == 0))
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

void sdbBCB::makeFreeExceptListItem()
{
    IDE_ASSERT( mFixCnt == 0 );

    mState    = SDB_BCB_FREE;

    SM_LSN_MAX( mRecoveryLSN );
    mTouchCnt = 0;

    IDV_TIME_INIT( &mLastTouchedTime );
    IDV_TIME_INIT( &mLastTouchCheckTime );
    IDV_TIME_INIT( &mCreateOrReadTime );

    mWriteCount    = 0;
    mReadyToRead   = ID_FALSE;
    mPageReadError = ID_FALSE;
}

// flag만 설정하기로, 나머지는   clean으로 될때 초기화
void sdbBCB::setToFree()
{
    makeFreeExceptListItem();

    SDB_INIT_BCB_LIST( this );

    if (( mCPListItem.mNext != NULL ) ||
        ( mCPListItem.mPrev != NULL ) ||
        ( mCPListNo != SDB_CP_LIST_NONE ))
    {
        /* BUG-47945 cp list 디버깅 정보 보강 */
        dump();
        IDE_ASSERT(0);
    }

    SDB_INIT_CP_LIST( this );
}

void sdbBCB::lockPageXLatch(idvSQL  * aStatistics)
{
    idvWeArgs   sWeArgs;

    IDV_WEARGS_SET( &sWeArgs, 
                    IDV_WAIT_INDEX_LATCH_BUFFER_BUSY_WAITS,
                    mSpaceID, mPageID, mPageType );

    IDE_ASSERT( mPageLatch.lockWrite( aStatistics,
                                      &sWeArgs )
                == IDE_SUCCESS );
}

void sdbBCB::lockPageSLatch(idvSQL * aStatistics)
{
    idvWeArgs   sWeArgs;

    IDV_WEARGS_SET( &sWeArgs, 
                    IDV_WAIT_INDEX_LATCH_BUFFER_BUSY_WAITS,
                    mSpaceID, mPageID, mPageType );

    IDE_ASSERT( mPageLatch.lockRead(
                    aStatistics,
                    &sWeArgs ) == IDE_SUCCESS );
}

void sdbBCB::tryLockPageSLatch( idBool *aSuccess )
{
    IDE_ASSERT( mPageLatch.tryLockRead( aSuccess ) == IDE_SUCCESS );
}

void sdbBCB::tryLockPageXLatch( idBool *aSuccess )
{
    IDE_ASSERT( mPageLatch.tryLockWrite( aSuccess ) == IDE_SUCCESS );
}

void sdbBCB::unlockPageLatch()
{
    IDE_ASSERT( mPageLatch.unlock() == IDE_SUCCESS );
}

void sdbBCB::lockBCBMutex( idvSQL *aStatistics)
{
    IDE_ASSERT( mMutex.lock( aStatistics) == IDE_SUCCESS );
}

void sdbBCB::unlockBCBMutex(void)
{
    IDE_ASSERT( mMutex.unlock() == IDE_SUCCESS );
}

void sdbBCB::incFixCnt()
{
    mFixCnt++;
}

void sdbBCB::decFixCnt()
{
    IDE_DASSERT( mFixCnt != 0);
    mFixCnt--;
}

void sdbBCB::updateTouchCnt()
{
    idvTime sCurrentTime;
    ULong   sTime;

    // 여러번 touch 된다고 해서 다 touchCount를 울리지 않고
    // 마지막 touch하고, sdbBCB::mTouchUSecInterval지나고 나서야
    // touch count를 올린다.
    IDV_TIME_GET( &sCurrentTime );

    if ( sdbBCB::mTouchUSecInterval > 0 )
    {
        sTime = IDV_TIME_DIFF_MICRO( &mLastTouchedTime, &sCurrentTime );

        /* PROJ-2669 실제 마지막 BCB Touch 시간 */
        mLastTouchCheckTime = sCurrentTime;

        if ( sTime > mTouchUSecInterval )
        {
            mLastTouchedTime = sCurrentTime;
            mTouchCnt++;
        }
    }
    else
    {
        /* PROJ-2669 실제 마지막 BCB Touch 시간 */
        mLastTouchCheckTime = sCurrentTime;

        mTouchCnt++;
    }
}

/* BUG-47945 cp list 디버깅 정보 보강 */
IDE_RC sdbBCB::dump(void *aBCB)
{
    IDE_ERROR( aBCB != NULL );

    ((sdbBCB*)aBCB)->dump();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void sdbBCB::clearTouchCnt()
{
    mTouchCnt = 1;
}


/***********************************************************************
 * Description :
 *  aFrame      - [IN]  Page pointer
 *  aBCB        - [IN]  해당 BCB
 ***********************************************************************/
void sdbBCB::setBCBPtrOnFrame( sdbFrameHdr   *aFrame,
                               sdbBCB        *aBCBPtr)
{
    aFrame->mBCBPtr = aBCBPtr;
}

/***********************************************************************
 * Description :
 *  aFrame      - [IN] 해당 page pointer
 *  aSpaceID    - [IN]  해당 table space ID
 ***********************************************************************/
void sdbBCB::setSpaceIDOnFrame( sdbFrameHdr   *aFrame,
                                scSpaceID      aSpaceID)
{
    aFrame->mSpaceID = aSpaceID;
}

/***********************************************************************
 * Description :
 *  aFrame      - [IN] 해당 page pointer
 *  aLSN        - [IN] PAGE LSN
 ***********************************************************************/
void sdbBCB::setPageLSN( sdbFrameHdr    *aFrame,
                         smLSN           aLSN )
{
    aFrame->mPageLSN = aLSN;
}

#endif //_O_SDB_BCB_MGR_H_
