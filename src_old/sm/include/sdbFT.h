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

#ifndef  _O_SDB_FT_H_
#define  _O_SDB_FT_H_ 1

#include <idl.h>
#include <idu.h>
#include <idv.h>
#include <smDef.h>
#include <sdbDef.h>
#include <sdpDef.h>
#include <sdbBCB.h>

class sdbFT
{
public:
    static IDE_RC buildRecordForBufferPoolStat(idvSQL              * /*aStatistics*/,
                                               void                *aHeader,
                                               void                */*aDumpObj*/,
                                               iduFixedTableMemory *aMemory);

    static IDE_RC buildRecordForBufferPageInfo(idvSQL              * /*aStatistics*/,
                                               void                *aHeader,
                                               void                */*aDumpObj*/,
                                               iduFixedTableMemory *aMemory);
};

#include <idTypes.h>

typedef struct sdbFlusherStatData
{
    UInt  mID;
    UInt  mAlive;
    UInt  mCurrentJob; // 0: sleep, 1: replacement flush, 2: checkpoint flush
    UInt  mIOing;      // 0: memory, 1: IO
    UInt  mINIOBCount;
    ULong mReplacementFlushJobs;
    ULong mReplacementFlushPages;
    ULong mReplacementSkipPages;

    /* PROJ-2669
     * Normal Flush List 에서 Delayed Flush List 로 이동되어진 BCB 의 수 */
    ULong mReplacementAddDelayedPages;
    /* PROJ-2669
     * Delayed Flush List 로 이동되어진 후 여전히 최근에 사용되어
     * Flush 되지 않은 BCB 의 수                                         */
    ULong mReplacementSkipDelayedPages;
    /* PROJ-2669
     * Delayed Flush List 의 최대 크기를 넘어
     * Delayed Flush List 추가를 하지 않은 BCB 의 수                     */
    ULong mReplacementOverflowDelayedPages;

    ULong mCheckpointFlushJobs;
    ULong mCheckpointFlushPages;
    ULong mCheckpointSkipPages;
    ULong mObjectFlushJobs;
    ULong mObjectFlushPages;
    ULong mObjectSkipPages;
    UInt  mLastSleepSec;
    ULong mWakeUpsByTimeout;
    ULong mWakeUpsBySignal;
    ULong mTotalSleepSec;
    /* BUG-32670    [sm-disk-resource] add IO Stat information 
     * for analyzing storage performance. */
    ULong mTotalFlushPages;

    /* PROJ-2669 Delayed Flush List 에 의해서 Flush 된 BCB 의 수 */
    ULong mTotalFlushDelayedPages;

    ULong mTotalLogSyncTime;
    ULong mTotalDWTime;
    ULong mTotalWriteTime;
    ULong mTotalSyncTime;
    ULong mTotalFlushTempPages;
    ULong mTotalTempWriteTime;
    ULong mTotalCalcChecksumTime;
} sdbFlusherStatData;

class sdbFlusherStat
{
public:
    void initialize(UInt aFlusherID);
    void applyStart();
    void applyFinish();
    void applyINIOBCount(UInt aINIOBCount);
    void applyReplaceFlushJob();
    void applyCheckpointFlushJob();
    void applyObjectFlushJob();
    void applyReplaceFlushJobDone();
    void applyCheckpointFlushJobDone();
    void applyObjectFlushJobDone();
    void applyIOBegin();
    void applyIODone();
    void applyReplaceFlushPages();
    void applyCheckpointFlushPages();
    void applyObjectFlushPages();
    void applyReplaceSkipPages();
    void applyReplaceAddDelayedPages();
    void applyReplaceSkipDelayedPages();
    void applyReplacementOverflowDelayedPages();
    void applyCheckpointSkipPages();
    void applyObjectSkipPages();
    void applyLastSleepSec(UInt aLastSleepSec);
    void applyWakeUpsByTimeout();
    void applyWakeUpsBySignal();
    void applyTotalSleepSec(UInt aSec);
    /* BUG-32670    [sm-disk-resource] add IO Stat information 
     * for analyzing storage performance. */
    void applyTotalFlushPages(UInt aCnt);
    void applyTotalFlushDelayedPages(UInt aCnt);
    void applyTotalLogSyncTimeUSec(UInt aUSec);
    void applyTotalDWTimeUSec(UInt aUSec);
    void applyTotalWriteTimeUSec(UInt aUSec);
    void applyTotalSyncTimeUSec(UInt aUSec);
    void applyTotalFlushTempPages(UInt aCnt);
    void applyTotalTempWriteTimeUSec(UInt aUSec);
    void applyTotalCalcChecksumTimeUSec(UInt aUSec);
    sdbFlusherStatData* getStatData();

private:
    sdbFlusherStatData  mStatData;
};

inline void sdbFlusherStat::initialize(UInt aFlusherID)
{
    mStatData.mID                    = aFlusherID;
    mStatData.mAlive                 = 0;
    mStatData.mCurrentJob            = 0;
    mStatData.mIOing                 = 0;
    mStatData.mINIOBCount            = 0;
    mStatData.mReplacementFlushJobs  = 0;
    mStatData.mReplacementFlushPages = 0;
    mStatData.mReplacementSkipPages  = 0;
    mStatData.mReplacementAddDelayedPages      = 0;
    mStatData.mReplacementSkipDelayedPages     = 0;
    mStatData.mReplacementOverflowDelayedPages = 0;
    mStatData.mCheckpointFlushJobs   = 0;
    mStatData.mCheckpointFlushPages  = 0;
    mStatData.mCheckpointSkipPages   = 0;
    mStatData.mObjectFlushJobs       = 0;
    mStatData.mObjectFlushPages      = 0;
    mStatData.mObjectSkipPages       = 0;
    mStatData.mLastSleepSec          = 0;
    mStatData.mWakeUpsByTimeout      = 0;
    mStatData.mWakeUpsBySignal       = 0;
    mStatData.mTotalSleepSec         = 0;
    mStatData.mTotalFlushPages       = 0;
    mStatData.mTotalFlushDelayedPages= 0;
    mStatData.mTotalLogSyncTime      = 0;
    mStatData.mTotalDWTime           = 0;
    mStatData.mTotalWriteTime        = 0;
    mStatData.mTotalSyncTime         = 0;
    mStatData.mTotalFlushTempPages   = 0;
    mStatData.mTotalTempWriteTime    = 0;
    mStatData.mTotalCalcChecksumTime = 0;
}

inline sdbFlusherStatData* sdbFlusherStat::getStatData()
{
    return &mStatData;
}

inline void sdbFlusherStat::applyStart()
{
    mStatData.mAlive = 1;
}

inline void sdbFlusherStat::applyFinish()
{
    mStatData.mAlive = 0;
}

inline void sdbFlusherStat::applyINIOBCount(UInt aINIOBCount)
{
    mStatData.mINIOBCount = aINIOBCount;
}

inline void sdbFlusherStat::applyReplaceFlushJob()
{
    mStatData.mCurrentJob = 1;
}

inline void sdbFlusherStat::applyCheckpointFlushJob()
{
    mStatData.mCurrentJob = 2;
}

inline void sdbFlusherStat::applyObjectFlushJob()
{
    mStatData.mCurrentJob = 3;
}

inline void sdbFlusherStat::applyReplaceFlushJobDone()
{
    mStatData.mCurrentJob = 0;
    mStatData.mReplacementFlushJobs++;
}

inline void sdbFlusherStat::applyCheckpointFlushJobDone()
{
    mStatData.mCurrentJob = 0;
    mStatData.mCheckpointFlushJobs++;
}

inline void sdbFlusherStat::applyObjectFlushJobDone()
{
    mStatData.mCurrentJob = 0;
    mStatData.mObjectFlushJobs++;
}

inline void sdbFlusherStat::applyIOBegin()
{
    mStatData.mIOing = 1;
}

inline void sdbFlusherStat::applyIODone()
{
    mStatData.mIOing = 0;
}

inline void sdbFlusherStat::applyReplaceFlushPages()
{
    mStatData.mReplacementFlushPages++;
}

inline void sdbFlusherStat::applyCheckpointFlushPages()
{
    mStatData.mCheckpointFlushPages++;
}

inline void sdbFlusherStat::applyObjectFlushPages()
{
    mStatData.mObjectFlushPages++;
}

inline void sdbFlusherStat::applyReplaceSkipPages()
{
    mStatData.mReplacementSkipPages++;
}

inline void sdbFlusherStat::applyReplaceAddDelayedPages()
{
    mStatData.mReplacementAddDelayedPages++;
}

inline void sdbFlusherStat::applyReplaceSkipDelayedPages()
{
    mStatData.mReplacementSkipDelayedPages++;
}

inline void sdbFlusherStat::applyReplacementOverflowDelayedPages()
{
    mStatData.mReplacementOverflowDelayedPages++;
}

inline void sdbFlusherStat::applyCheckpointSkipPages()
{
    mStatData.mCheckpointSkipPages++;
}

inline void sdbFlusherStat::applyObjectSkipPages()
{
    mStatData.mObjectSkipPages++;
}

inline void sdbFlusherStat::applyLastSleepSec(UInt aLastSleepSec)
{
    mStatData.mLastSleepSec = aLastSleepSec;
}

inline void sdbFlusherStat::applyWakeUpsByTimeout()
{
    mStatData.mWakeUpsByTimeout++;
}

inline void sdbFlusherStat::applyWakeUpsBySignal()
{
    mStatData.mWakeUpsBySignal++;
}

inline void sdbFlusherStat::applyTotalSleepSec(UInt aSec)
{
    mStatData.mTotalSleepSec += aSec;
}
inline void sdbFlusherStat::applyTotalFlushPages(UInt aCnt)
{
    mStatData.mTotalFlushPages += aCnt;
}

inline void sdbFlusherStat::applyTotalFlushDelayedPages(UInt aCnt)
{
    mStatData.mTotalFlushDelayedPages += aCnt;
}

inline void sdbFlusherStat::applyTotalLogSyncTimeUSec(UInt aUSec)
{
    mStatData.mTotalLogSyncTime += aUSec;
}
inline void sdbFlusherStat::applyTotalDWTimeUSec(UInt aUSec)
{
    mStatData.mTotalDWTime += aUSec;
}
inline void sdbFlusherStat::applyTotalWriteTimeUSec(UInt aUSec)
{
    mStatData.mTotalWriteTime += aUSec;
}
inline void sdbFlusherStat::applyTotalSyncTimeUSec(UInt aUSec)
{
    mStatData.mTotalSyncTime += aUSec;
}
inline void sdbFlusherStat::applyTotalFlushTempPages(UInt aCnt)
{
    mStatData.mTotalFlushTempPages += aCnt;
}
inline void sdbFlusherStat::applyTotalTempWriteTimeUSec(UInt aUSec)
{
    mStatData.mTotalTempWriteTime += aUSec;
}
inline void sdbFlusherStat::applyTotalCalcChecksumTimeUSec(UInt aUSec)
{
    mStatData.mTotalCalcChecksumTime += aUSec;
}



#endif//   _O_SDB_FT_H_
