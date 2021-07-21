/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduStackMgr.h 33576 2009-06-27 05:34:37Z newdaily $
 **********************************************************************/

#ifndef _O_IDU_STACKMGR_H_
#define _O_IDU_STACKMGR_H_ 1

#include <idl.h>
#include <idu.h>
#include <iduMutex.h>

typedef struct iduStackPage
{
    struct iduStackPage* mPrev;
    struct iduStackPage* mNext;
    SChar   mData[1];
} iduStackPage;

class iduStackMgr
{
public:
    iduStackMgr();
    ~iduStackMgr();

    IDE_RC  initialize( iduMemoryClientIndex aIndex,
                        ULong aItemSize );
    IDE_RC  destroy();

    IDE_RC  lock()
            { return mMutex.lock( NULL ); }
    IDE_RC  unlock()
            { return mMutex.unlock(); };

    idBool  isEmpty();

    IDE_RC  pop( idBool   aLock,
                 void   * aItem,
                 idBool * aEmtpy);

    IDE_RC  push( idBool   aMutex,
                  void   * aItem );

    inline  ULong  getTotItemCnt();

public:
    iduMemoryClientIndex mIndex;

    /* Stack Page Header */
    iduStackPage  mHead;

    /* Pop될 Item이 있는 Stack Page */
    iduStackPage* mCurPage;

    /* Item Size */
    ULong          mItemSize;

    /* Stack에 있는 전체 Item의 갯수 */
    ULong          mTotItemCnt;

    /* Stack Page Size( Byte ) */
    ULong          mPageSize;

    /* 하나의 Stack Page에 들어갈 Item갯수 */
    ULong          mMaxItemCntInPage;

    /* mCurPage의 Item Count */
    ULong          mItemCntOfCurPage;

private:
    iduMutex      mMutex;
};

ULong iduStackMgr::getTotItemCnt()
{
    return mTotItemCnt;
}

#endif
