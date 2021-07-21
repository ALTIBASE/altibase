/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduLatch.h 84854 2019-01-31 09:47:48Z yoonhee.kim $
 **********************************************************************/

#ifndef _O_IDU_LATCH_H_
#define _O_IDU_LATCH_H_ 1

#include <idl.h>
#include <iduMemDefs.h>
#include <iduLatchObj.h>

class iduLatch
{
public:
    static IDE_RC       initializeStatic(iduPeerType aType);
    static IDE_RC       destroyStatic();
    static SInt         mLatchSpinCount;

    IDE_RC initialize(SChar* aName = (SChar*)"UNNAMED_LATCH");
    IDE_RC initialize(SChar* aName, iduLatchType aLatchType);
    IDE_RC destroy   (void);

    /*
     * 초기화시 명시된 wait event의 wait time을 측정하려면
     * Session 통계자료구조인 idvSQL를 인자로 전달해야한다.
     * TIMED_STATISTICS 프로퍼티가 1 이어야 측정된다.
     */
    IDE_RC tryLockRead(idBool*  aSuccess);
    IDE_RC tryLockWrite(idBool* aSuccess);
    IDE_RC lockRead     (idvSQL* aStatSQL, void* aWeArgs );
    IDE_RC lockWrite    (idvSQL* aStatSQL, void* aWeArgs );
    IDE_RC unlock       (void);
    IDE_RC unlockWriteAll    (void);

    inline IDE_RC unlock(UInt,void*) {return unlock();}

    /*
     * Get, Miss Count, Thread ID 가져오기
     */
    inline ULong getReadCount()     {return mLatch->mGetReadCount;  }
    inline ULong getWriteCount()    {return mLatch->mGetWriteCount; }
    inline ULong getReadMisses()    {return mLatch->mReadMisses;    }
    inline ULong getWriteMisses()   {return mLatch->mWriteMisses;   }
    inline ULong getOwnerThreadID() {return mLatch->mWriteThreadID; }
    inline ULong getSleepCount()    {return mLatch->mSleepCount;    }

    iduLatchMode getLatchMode();
    IDE_RC dump();

    static idBool isLatchSame( void *aLhs, void *aRhs );

    static iduMutex     mIdleLock[IDU_LATCH_TYPE_MAX];
    static iduLatchObj* mIdle[IDU_LATCH_TYPE_MAX];
    static iduMutex     mInfoLock;
    static iduLatchObj* mInfo;
    static iduPeerType  mPeerType;

    static inline iduLatchCore* getFirstInfo() {return mInfo;}

    inline const iduLatchObj* getLatchCore(void) {return mLatch;}

    static void unlockWriteAllMyThread();
private:
    iduLatchObj*        mLatch;

    /*
     * Project 2408
     * Pool for latches
     */
    static iduMemSmall  mLatchPool;
    static idBool       mUsePool;
    static IDE_RC       allocLatch(ULong, iduLatchObj**);
    static IDE_RC       freeLatch(iduLatchObj*);
};

inline IDE_RC iduLatch::tryLockRead(idBool* aSuccess)
{
    return mLatch->tryLockRead(aSuccess);
}

inline IDE_RC iduLatch::tryLockWrite(idBool* aSuccess)
{
    return mLatch->tryLockWrite(aSuccess);
}

inline IDE_RC iduLatch::lockRead(idvSQL* aStatSQL, void* aWeArgs )
{
    return mLatch->lockRead(aStatSQL, aWeArgs);
}

inline IDE_RC iduLatch::lockWrite(idvSQL* aStatSQL, void* aWeArgs )
{
    return mLatch->lockWrite(aStatSQL, aWeArgs);
}

inline IDE_RC iduLatch::unlock(void)
{
    return mLatch->unlock();
}
inline IDE_RC iduLatch::unlockWriteAll(void)
{
    return mLatch->unlockWriteAll();
}

// WARNING : Only for Debug or Verify
inline iduLatchMode iduLatch::getLatchMode()
{
#define IDE_FN "iduLatch::getLatchMode()"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_ASSERT(mLatch != NULL);

    if (mLatch->mMode == 0)
    {
        return IDU_LATCH_NONE;
    }
    else
    {
        if (mLatch->mMode > 0)
        {
            return IDU_LATCH_READ;
        }
        else
        {
            return IDU_LATCH_WRITE;
        }
    }
#undef IDE_FN
}

/* --------------------------------------------------------------------
 * 2개의 latch가 같은 지 비교한다.
 * sdrMiniTrans에서 스택에 있는 특정 아이템을 찾을 때 사용된다.
 * ----------------------------------------------------------------- */
inline idBool iduLatch::isLatchSame( void *aLhs, void *aRhs )
{
#define IDE_FN "iduLatch::isLatchSame()"

    iduLatchObj *sLhs  = (iduLatchObj *)aLhs;
    iduLatchObj *sRhs  = (iduLatchObj *)aRhs;

    return ( ( sLhs->mMode == sRhs->mMode &&
               sLhs->mWriteThreadID == sRhs->mWriteThreadID ) ?
             ID_TRUE : ID_FALSE );

#undef IDE_FN
}

#endif  // _O_IDU_LATCH_H_
