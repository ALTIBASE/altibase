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

#ifndef _O_SDB_FLUSH_MGR_H_
#define _O_SDB_FLUSH_MGR_H_ 1

#include <sdbDef.h>
#include <idu.h>
#include <idv.h>
#include <sdbFlusher.h>
#include <sdbFT.h>
#include <smuQueueMgr.h>
#include <sdsBCB.h>

// flushMgr이 한순간에 최대로 유지할 수 있는 작업수
#define SDB_FLUSH_JOB_MAX          (64)
#define SDB_FLUSH_COUNT_UNLIMITED  (ID_ULONG_MAX)

typedef enum
{
    SDB_FLUSH_JOB_NOTHING = 0,
    SDB_FLUSH_JOB_REPLACEMENT_FLUSH,
    SDB_FLUSH_JOB_CHECKPOINT_FLUSH,
    SDB_FLUSH_JOB_DBOBJECT_FLUSH
} sdbFlushJobType;

typedef struct sdbLRUList sdbLRUList;
typedef struct sdbFlushList sdbFlushList;
typedef struct sdbCPListSet sdbCPListSet;

// replace flush를 위해 필요한 자료구조
typedef struct sdbReplaceFlushJobParam
{
    // replace flush를 해야할 flush list
    sdbFlushList    *mFlushList;
    // replace대상이 아닌 BCB들을 옮길 LRU List
    sdbLRUList      *mLRUList;
} sdbReplaceFlushJobParam;

// checkpoint flush를 위해 필요한 자료구조
// BUG-22857 로 인하여 CP List에 DirtyPage가 과다할 경우
// 충분히 정리해 주기 위해서 추가 되었음
typedef struct sdbChkptFlushJobParam
{
    // Restart Recovery시에 Redo할 Page 수
    // 반대로 말하면 Buffer Pool에 남겨둘
    // Dirty Page 수
    ULong             mRedoPageCount;
    // Restart Recovery시에 Redo할 log file 수
    UInt              mRedoLogFileCount;

    sdbCheckpointType mCheckpointType;

} sdbChkptFlushJobParam;

// DB object flush를 위해 필요한 자료구조
typedef struct sdbObjectFlushJobParam
{
    // flush 해야 할 BCB포인터들이 들어있는 큐
    smuQueueMgr *mBCBQueue;
    // flush 해야 할 BCB의 조건이 변경되었을 가능성이 있으므로
    // 다시 확인하기 위한 함수
    sdbFiltFunc  mFiltFunc;
    // mFiltFunc에 반드시 같이 넣어 주는 변수
    void        *mFiltObj;
} sdbObjectFlushJobParam;


// flush job이 끝났을 시 작업을 처리하기 위해
// 필요한 자료구조. 내부에서만 사용한다.
typedef struct sdbFlushJobDoneNotifyParam
{
    // job이 끝날때 까지 대기를 해야 하는데, 이때 필요한 mutex
    iduMutex    mMutex;
    // job이 끝날때 까지 대기를 해야 하는데, 이때 필요한 variable
    iduCond     mCondVar;
    // job이 끝났는지 여부.
    idBool      mJobDone;
} sdbFlushJobDoneNotifyParam;

/* PROJ-2102 Fast Secondaty Buffer */
typedef struct sdbSBufferReplaceFlushJobParam
{
    ULong   mExtentIndex;
} sdbSBufferReplaceFlushJobParam;

typedef struct sdbFlushJob
{
    // flush job type
    sdbFlushJobType             mType;
    // 요청된 flush 페이지 개수
    ULong                       mReqFlushCount;
    // flush작업이 완료 될 때까지 기다려야 하는 경우에 사용함
    sdbFlushJobDoneNotifyParam *mJobDoneParam;

    // flush 작업에 필요한 파라미터 정보
    union
    {
        sdbReplaceFlushJobParam       mReplaceFlush;
        sdbObjectFlushJobParam        mObjectFlush;
        sdbChkptFlushJobParam         mChkptFlush;
        /* PROJ-2102 Fast Secondary Buffer */
        sdbSBufferReplaceFlushJobParam mSBufferReplaceFlush;

    } mFlushJobParam;
} sdbFlushJob;

class sdbFlushMgr
{
public:
    static IDE_RC initialize(UInt aFlusherCount);

    static void startUpFlushers();

    static IDE_RC destroy();

    static void getJob(idvSQL       *aStatistics,
                       sdbFlushJob  *aRetJob);

    static void getDelayedFlushJob( sdbFlushJob *aColdJob,
                                sdbFlushJob *aRetJob );

    static IDE_RC initJobDoneNotifyParam(sdbFlushJobDoneNotifyParam *aParam);

    static IDE_RC waitForJobDone(sdbFlushJobDoneNotifyParam *aParam);

    static void notifyJobDone(sdbFlushJobDoneNotifyParam *aParam);

    static IDE_RC wakeUpAllFlushers();
    static IDE_RC wait4AllFlusher2Do1JobDone();

    static void getMinRecoveryLSN(idvSQL *aStatistics,
                                  smLSN  *aRet);
#if 0 //not used.  
    static void addReqFlushJob(idvSQL         *aStatistics,
                               sdbFlushList   *aFlushList,
                               sdbLRUList     *aLRUList,
                               idBool         *aAdded);
#endif
    static void addReqCPFlushJob(
        idvSQL                        * aStatistics,
        ULong                           aMinFlushCount,
        ULong                           aRedoPageCount,
        UInt                            aRedoLogFileCount,
        sdbFlushJobDoneNotifyParam    * aJobDoneParam,
        idBool                        * aAdded);

    static void addReqDBObjectFlushJob(
                                    idvSQL                     * aStatistics,
                                    smuQueueMgr                * aBCBQueue,
                                    sdbFiltFunc                  aFiltFunc,
                                    void                       * aFiltObj,
                                    sdbFlushJobDoneNotifyParam * aJobDoneParam,
                                    idBool                     * aAdded);

    static IDE_RC turnOffFlusher(idvSQL *aStatistics, UInt aFlusherID);

    static IDE_RC turnOnFlusher(UInt aFlusherID);

    static IDE_RC turnOnAllflushers();

    static IDE_RC flushDirtyPagesInCPList( idvSQL * aStatistics,
                                           idBool   aFlushAll,
                                           idBool   aIsChkptThread );

    static IDE_RC flushObjectDirtyPages(idvSQL       *aStatistics,
                                        smuQueueMgr  *aBCBQueue,
                                        sdbFiltFunc   aFiltFunc,
                                        void         *aFiltObj);
    static idBool isBusyCondition();

    static idBool isCond4ReplaceFlush();

    static ULong getLowFlushLstLen4FF();

    static ULong getLowPrepareLstLen4FF();

    static ULong getHighFlushLstLen4FF();

    static inline UInt getReqJobCount();

    static inline void updateLastFlushedTime();

    static inline sdbFlusherStat* getFlusherStat(UInt aFlusherID);

    static inline UInt getFlusherCount();

    static UInt getActiveFlusherCount();

    static void setDelayedFlushProperty( UInt aDelayedFlushListPct,
                                         UInt aDelayedFlushProtectionTimeMsec );

private:
    static inline void initJob(sdbFlushJob *aJob);

    static inline void incPos(UInt *aPos);

    static void getReplaceFlushJob(sdbFlushJob *aRetJob);

    static void getCPFlushJob(idvSQL *aStatistics, sdbFlushJob *aRetJob);

    static void getReqJob(idvSQL       *aStatistics,
                          sdbFlushJob  *aRetJob);

private:
    // Job을 등록하고 가져올때 사용하는 mutex
    // mReqJobMutex는 트랜잭션 쓰레드가 req job을 등록할때,
    // 그리고 flusher가 getJob할 때 사용된다. 따라서 트랜잭션 쓰레드와
    // flusher들 간에 경합이 발생할 수 있다.
    // 따라서 최소한의 mutex 구간을 유지해야 한다.
    static iduMutex      mReqJobMutex;

    // Job을 등록할때 사용하는 자료구조
    static sdbFlushJob   mReqJobQueue[SDB_FLUSH_JOB_MAX];

    // Job을 등록할때 사용하는 변수,
    // mReqJobQueue[mReqJobAddPos++] = job
    static UInt          mReqJobAddPos;

    // Job을 가져 올때 사용하는 변수,
    // job = mReqJobQueue[mReqJobGetPos++]
    static UInt          mReqJobGetPos;

    // BUG-26476
    // checkpoint 수행과 flusher control을 위한 mutex
    static iduLatch   mFCLatch; // flusher control latch

    // flusher를 배열형태로 가지고 있다.
    static sdbFlusher   *mFlushers;

    // sdbFlushMgr이 최대로 가질 수 있는 flusher갯수
    static UInt          mFlusherCount;

    // 마지막에 flush한 시간
    static idvTime       mLastFlushedTime;
    
    // flush Mgr이 작업해야할 buffer pool에 속해 있는 checkpoint list
    static sdbCPListSet *mCPListSet;
};

void sdbFlushMgr::initJob(sdbFlushJob *aJob)
{
    aJob->mType          = SDB_FLUSH_JOB_NOTHING;
    aJob->mReqFlushCount = 0;
    aJob->mJobDoneParam  = NULL;
}

/***************************************************************************
 *  description:
 *      현재 jobQueue에서의 position을 증가시킨다.
 *      mReqJobQueue는 크기가 고정되어 있기 때문에, SDB_FLUSH_JOB_MAX를
 *      초과하는 경우엔 0으로 된다.
 ***************************************************************************/
void sdbFlushMgr::incPos(UInt *aPos)
{
    UInt sPos = *aPos;

    IDE_ASSERT(sPos < SDB_FLUSH_JOB_MAX);

    sPos += 1;
    if (sPos == SDB_FLUSH_JOB_MAX)
    {
        sPos = 0;
    }
    *aPos = sPos;
}

UInt sdbFlushMgr::getFlusherCount()
{
    return mFlusherCount;
}

sdbFlusherStat* sdbFlushMgr::getFlusherStat(UInt aFlusherID)
{
    return mFlushers[aFlusherID].getStat();
}

UInt sdbFlushMgr::getReqJobCount()
{
    UInt sRet = 0;
    UInt i;

    for (i = 0; i < SDB_FLUSH_JOB_MAX; i++)
    {
        if ( mReqJobQueue[mReqJobAddPos].mType != SDB_FLUSH_JOB_NOTHING )
        {
            sRet++;
        }
    }
    return sRet;
}

void sdbFlushMgr::updateLastFlushedTime()
{
    IDV_TIME_GET(&mLastFlushedTime);
}

#endif // _O_SDB_FLUSH_MGR_H_

