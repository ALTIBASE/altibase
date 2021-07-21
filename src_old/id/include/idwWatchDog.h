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
 
/*******************************************************************************
 * $Id$
 ******************************************************************************/

#if !defined(_O_IDW_WATCH_DOG_H_)
#define _O_IDW_WATCH_DOG_H_ 1

#include <iduCond.h>
#include <idrRecProcess.h>

typedef void (*iduShmStStmtRemoveFunc)(idvSQL *, iduShmListNode *, void *);

typedef enum
{
    ID_WATCHDOG_BEGIN = 0,
    ID_WATCHDOG_RUN,
    ID_WATCHDOG_END
}idWatchDogStatus;

typedef struct idwStWatchDogInfo
{
    idShmAddr   mAddrSelf;
    ULong       mDeadProcessCnt;
} idwStWatchDogInfo;

class idwWatchDog : public idtBaseThread
{
private:
    static idrRecProcess * mArrRecProcess;
    static UInt            mMaxRecProcessCnt;

    static iduCond         mCV;
    static iduMutex        mMutex;
    static idBool          mFinish;

    static idwStWatchDogInfo * mStWatchDogInfo;

public:
    /* idrRecProcess에서 프로세스 정보와 연결된 StStmtInfo를
     * 제거하기 위하여 호출되는 콜백 함수 포인터.
    * mmcStatementManager에 있는 removeStStmtInfo가 함수의 
    * 포인터가 등록된다.
     */
    static iduShmStStmtRemoveFunc mStStmtRmFunc;
    static idWatchDogStatus       mWatchDogStatus;

public:
    idwWatchDog();
    virtual ~idwWatchDog();

    IDE_RC startThread();
    IDE_RC shutdown();

    virtual void run();

    static IDE_RC initialize( UInt aMaxRecProcessCnt );
    static IDE_RC destroy();

    static inline IDE_RC lock( idvSQL * aStatistics );
    static inline IDE_RC unlock();

    static IDE_RC shutdownAllRecProc();

    static inline ULong getDeadProcCnt() { return mStWatchDogInfo->mDeadProcessCnt; };

    static void setStStmtRemoveFunc( iduShmStStmtRemoveFunc aFuncPtr )
    {
        idwWatchDog::mStStmtRmFunc = aFuncPtr;
    };

    static void attach();
    static void detach();
};

inline IDE_RC idwWatchDog::lock( idvSQL * aStatistics )
{
    return mMutex.lock( aStatistics );
}

inline IDE_RC idwWatchDog::unlock()
{
    return mMutex.unlock();
}

#endif /* _O_IDW_WATCH_DOG_H_ */
