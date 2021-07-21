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
 * $Id$
 **********************************************************************/

/***********************************************************************
 * PROJ-2102 The Fast Secondary Buffer
 **********************************************************************/

#ifndef _O_SDS_FLUSHMGR_H_
#define _O_SDS_FLUSHMGR_H_ 1

#include <idv.h>
#include <idl.h>
#include <iduLatch.h>
#include <iduMutex.h>

#include <smuQueueMgr.h>
#include <sdbFlushMgr.h>
#include <sdbFlusher.h>

#include <sdsFlusherStat.h>

class sdsFlushMgr
{
public:
    static IDE_RC initialize( UInt aFlusherCount );

    static void startUpFlushers( void );

    static IDE_RC destroy( void );

    static void getJob( idvSQL       *aStatistics,
                        sdbFlushJob  *aRetJob  );

    static IDE_RC initJobDoneNotifyParam( sdbFlushJobDoneNotifyParam *aParam );

    static IDE_RC waitForJobDone( sdbFlushJobDoneNotifyParam *aParam );

    static void notifyJobDone( sdbFlushJobDoneNotifyParam *aParam );

    static IDE_RC wakeUpAllFlushers( void );
    
    static IDE_RC wait4AllFlusher2Do1JobDone( void );

    static void getMinRecoveryLSN( idvSQL *aStatistics,
                                   smLSN  *aRet );

    static void addReqReplaceFlushJob( idvSQL         *aStatistics,
                                       sdbFlushJobDoneNotifyParam *aJobDoneParam,
                                       idBool         *aAdded,
                                       idBool         *aSkip );

    static void addReqCPFlushJob( idvSQL                     *aStatistics,
                                  ULong                       aMinFlushCount,
                                  ULong                       aRedoPageCount,
                                  UInt                        aRedoLogFileCount,
                                  sdbFlushJobDoneNotifyParam *aJobDoneParam,
                                  idBool                     *aAdded );

    static void addReqObjectFlushJob( idvSQL                     *aStatistics,
                                      smuQueueMgr                *aBCBQueue,
                                      sdsFiltFunc                 aFiltFunc,
                                      void                       *aFiltObj,
                                      sdbFlushJobDoneNotifyParam *aJobDoneParam,
                                      idBool                     *aAdded );

    static IDE_RC turnOffFlusher( idvSQL *aStatistics, UInt aFlusherID );

    static IDE_RC turnOnFlusher( UInt aFlusherID );

    static IDE_RC turnOnAllflushers( void );

    static IDE_RC flushPagesInExtent( idvSQL    * aStatistics );

    static IDE_RC flushDirtyPagesInCPList( idvSQL * aStatistics,
                                           idBool   aFlushAll );

    static IDE_RC flushObjectDirtyPages( idvSQL       * aStatistics,
                                         smuQueueMgr  * aBCBQueue,
                                         sdsFiltFunc    aFiltFunc,
                                         void         * aFiltObj );
    static idBool isBusyCondition( void );

    static idBool isCond4ReplaceFlush( void );

    static inline UInt getReqJobCount( void );

    static inline void updateLastFlushedTime( void );

    static inline sdsFlusherStat *getFlusherStat( UInt aFlusherID );

    static inline UInt getFlusherCount( void );

    static UInt getActiveFlusherCount( void );

    static void limitCPFlusherCount( idvSQL      * aStatistics, 
                                     sdbFlushJob * aRetJob,
                                     idBool        aAlreadyLocked );

private:
    static inline void initJob( sdbFlushJob   * aJob );

    static inline void incPos( UInt  * aPos );

    static void getReplaceFlushJob( sdbFlushJob   * aRetJob );

    static void getCPFlushJob( idvSQL  * aStatistics, sdbFlushJob  * aRetJob );

    static void getReqJob( idvSQL   * aStatistics, sdbFlushJob  * aRetJob );

private:
    // Job을 등록할때 사용하는 자료구조
    static sdbFlushJob      mReqJobQueue[SDB_FLUSH_JOB_MAX];

    // Job을 등록할때 사용하는 변수,
    static UInt             mReqJobAddPos;

    // Job을 가져 올때 사용하는 변수,
    static UInt             mReqJobGetPos;

    // Job을 등록하고 가져올때 사용하는 mutex (flusher. service)
    static iduMutex         mReqJobMutex;

    // BUG-26476
    // checkpoint 수행과 flusher control을 위한 mutex
    static iduLatch      mFCLatch; // flusher control latch

    // flusher를 배열형태로 가지고 있다.
    static sdsFlusher     * mFlushers;

    // sdsFlushMgr이 최대로 가질 수 있는 flusher갯수
    static UInt             mMaxFlusherCount;

    // 현시점에 checkpointFlush를 수행하는 Flusher 개수
    static UInt             mActiveCPFlusherCount;

    // 마지막에 flush한 시간
    static idvTime          mLastFlushedTime;

    // flush Mgr이 작업해야할  checkpoint list
    static sdbCPListSet   * mCPListSet;

    static idBool          mServiceable; 

    static idBool          mInitialized; 
};

/*************************************************************************
 *  description:
 *************************************************************************/
void sdsFlushMgr::initJob( sdbFlushJob *aJob )
{
    aJob->mType            = SDB_FLUSH_JOB_NOTHING;
    aJob->mReqFlushCount   = 0;
    aJob->mJobDoneParam    = NULL;
}

/************************************************************************
 *  description:
 ************************************************************************/
void sdsFlushMgr::incPos( UInt *aPos )
{
    UInt sPos = *aPos;

    IDE_ASSERT( sPos < SDB_FLUSH_JOB_MAX );

    sPos += 1;
    if( sPos == SDB_FLUSH_JOB_MAX )
    {
        sPos = 0;
    }
    *aPos = sPos;
}

UInt sdsFlushMgr::getFlusherCount()
{
    return mMaxFlusherCount;
}

sdsFlusherStat *sdsFlushMgr::getFlusherStat( UInt aFlusherID )
{
    return mFlushers[aFlusherID].getStat();
}

UInt sdsFlushMgr::getReqJobCount()
{
    UInt sRet = 0;
    UInt i;

    for( i = 0; i < SDB_FLUSH_JOB_MAX; i++ )
    {
        if( mReqJobQueue[mReqJobAddPos].mType != SDB_FLUSH_JOB_NOTHING )
        {
            sRet++;
        }
    }
    return sRet;
}

void sdsFlushMgr::updateLastFlushedTime()
{
    IDV_TIME_GET( &mLastFlushedTime );
}
#endif
