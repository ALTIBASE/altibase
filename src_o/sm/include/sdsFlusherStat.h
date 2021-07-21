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

#ifndef  _O_SDS_FLUSER_STAT_H_
#define  _O_SDS_FLUSER_STAT_H_ 1

#include <idl.h>
#include <idu.h>
#include <idv.h>
#include <idTypes.h>
#include <smDef.h>
#include <sdbDef.h>
#include <sdpDef.h>
#include <sdsDef.h>
#include <sdbBCB.h>

class sdsFlusherStat
{
public:
    inline void initialize( UInt aFlusherID );
    inline void applyStart( void );
    inline void applyFinish( void );
    inline void applyINIOBCount( UInt aINIOBCount );
    inline void applyReplaceFlushJob( void );
    inline void applyCheckpointFlushJob( void );
    inline void applyObjectFlushJob( void );
    inline void applyReplaceFlushJobDone( void );
    inline void applyCheckpointFlushJobDone( void );
    inline void applyObjectFlushJobDone( void );
    inline void applyIOBegin( void );
    inline void applyIODone( void );
    inline void applyReplaceFlushPages( void );
    inline void applyCheckpointFlushPages( void );
    inline void applyObjectFlushPages( void );
    inline void applyReplaceSkipPages( void );
    inline void applyCheckpointSkipPages( void );
    inline void applyObjectSkipPages( void );
    inline void applyLastSleepSec( UInt aLastSleepSec );
    inline void applyWakeUpsByTimeout( void );
    inline void applyWakeUpsBySignal( void );
    inline void applyTotalSleepSec( UInt aSec );
    /* BUG-32670    [sm-disk-resource] add IO Stat information 
     * for analyzing storage performance. */
    inline void applyTotalFlushPages( UInt aCnt );
    inline void applyTotalDWTimeUSec( UInt aUSec );
    inline void applyTotalWriteTimeUSec( UInt aUSec );
    inline void applyTotalSyncTimeUSec( UInt aUSec );
    inline void applyTotalFlushTempPages( UInt aCnt );
    inline void applyTotalTempWriteTimeUSec( UInt aUSec );
public:
   inline sdsFlusherStatData* getStatData( void );

private:
    sdsFlusherStatData  mStatData;
};

void sdsFlusherStat::initialize( UInt aFlusherID )
{
    mStatData.mID                    = aFlusherID;
    mStatData.mAlive                 = 0;
    mStatData.mCurrentJob            = 0;
    mStatData.mIOing                 = 0;
    mStatData.mINIOBCount            = 0;
    mStatData.mReplacementFlushJobs  = 0;
    mStatData.mReplacementFlushPages = 0;
    mStatData.mReplacementSkipPages  = 0;
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
    /* BUG-32670    [sm-disk-resource] add IO Stat information 
     * for analyzing storage performance. */
    mStatData.mTotalFlushPages       = 0;
    mStatData.mTotalDWTime           = 0;
    mStatData.mTotalWriteTime        = 0;
    mStatData.mTotalSyncTime         = 0;
    mStatData.mTotalFlushTempPages   = 0;
    mStatData.mTotalTempWriteTime    = 0;
}

sdsFlusherStatData* sdsFlusherStat::getStatData( void )
{
    return &mStatData;
}

void sdsFlusherStat::applyStart( void )
{
    mStatData.mAlive = SDS_FLUSHER_STARTED;
}

void sdsFlusherStat::applyFinish( void )
{
    mStatData.mAlive = SDS_FLUSHER_FINISH;
}

void sdsFlusherStat::applyINIOBCount( UInt aINIOBCount ) 
{
    mStatData.mINIOBCount = aINIOBCount;
}

void sdsFlusherStat::applyReplaceFlushJob( void )
{
    mStatData.mCurrentJob = SDS_FLUSHJOB_REPLACEMENT;
}

void sdsFlusherStat::applyCheckpointFlushJob( void )
{
    mStatData.mCurrentJob = SDS_FLUSHJOB_CHECKPOINT;
}

void sdsFlusherStat::applyObjectFlushJob( void )
{
    mStatData.mCurrentJob = SDS_FLUSHJOB_OBJECT;
}

void sdsFlusherStat::applyReplaceFlushJobDone( void )
{
    mStatData.mCurrentJob = SDS_FLUSHJOB_NONE;
    mStatData.mReplacementFlushJobs++;
}

void sdsFlusherStat::applyCheckpointFlushJobDone( void )
{
    mStatData.mCurrentJob = SDS_FLUSHJOB_NONE;
    mStatData.mCheckpointFlushJobs++;
}

void sdsFlusherStat::applyObjectFlushJobDone( void )
{
    mStatData.mCurrentJob = SDS_FLUSHJOB_NONE;
    mStatData.mObjectFlushJobs++;
}

void sdsFlusherStat::applyIOBegin( void )
{
    mStatData.mIOing = SDS_IO_IN_PROGRESS;
}

void sdsFlusherStat::applyIODone( void )
{
    mStatData.mIOing = SDS_IO_DONE;
}

void sdsFlusherStat::applyReplaceFlushPages( void )
{
    mStatData.mReplacementFlushPages++;
}

void sdsFlusherStat::applyCheckpointFlushPages( void )
{
    mStatData.mCheckpointFlushPages++;
}

void sdsFlusherStat::applyObjectFlushPages( void )
{
    mStatData.mObjectFlushPages++;
}

void sdsFlusherStat::applyReplaceSkipPages( void )
{
    mStatData.mReplacementSkipPages++;
}

void sdsFlusherStat::applyCheckpointSkipPages( void )
{
    mStatData.mCheckpointSkipPages++;
}

void sdsFlusherStat::applyObjectSkipPages( void )
{
    mStatData.mObjectSkipPages++;
}

void sdsFlusherStat::applyLastSleepSec( UInt aLastSleepSec )
{
    mStatData.mLastSleepSec = aLastSleepSec;
}

void sdsFlusherStat::applyWakeUpsByTimeout( void )
{
    mStatData.mWakeUpsByTimeout++;
}

void sdsFlusherStat::applyWakeUpsBySignal( void )
{
    mStatData.mWakeUpsBySignal++;
}

void sdsFlusherStat::applyTotalSleepSec( UInt aSec )
{
    mStatData.mTotalSleepSec += aSec;
}

void sdsFlusherStat::applyTotalFlushPages( UInt aCnt )
{
    mStatData.mTotalFlushPages += aCnt;
}

void sdsFlusherStat::applyTotalDWTimeUSec( UInt aUSec )
{
    mStatData.mTotalDWTime += aUSec;
}

void sdsFlusherStat::applyTotalWriteTimeUSec( UInt aUSec )
{
    mStatData.mTotalWriteTime += aUSec;
}

void sdsFlusherStat::applyTotalSyncTimeUSec( UInt aUSec )
{
    mStatData.mTotalSyncTime += aUSec;
}

void sdsFlusherStat::applyTotalFlushTempPages( UInt aCnt )
{
    mStatData.mTotalFlushTempPages += aCnt;
}

void sdsFlusherStat::applyTotalTempWriteTimeUSec( UInt aUSec )
{
    mStatData.mTotalTempWriteTime += aUSec;
}
#endif 
