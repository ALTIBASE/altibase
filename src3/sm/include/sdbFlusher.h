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

#ifndef _O_SDB_FLUSHER_H_
#define _O_SDB_FLUSHER_H_ 1

#include <sdbDef.h>
#include <idu.h>
#include <idv.h>
#include <idtBaseThread.h>
#include <sdbBCB.h>
#include <sdbLRUList.h>
#include <sdbFlushList.h>
#include <sdbFT.h>
#include <sdbCPListSet.h>
#include <sddDWFile.h>
#include <smuQueueMgr.h>


typedef enum
{
    SDB_CHECKPOINT_BY_CHKPT_THREAD = 0,
    SDB_CHECKPOINT_BY_FLUSH_THREAD
} sdbCheckpointType;

struct sdbFlushJob;

class sdbFlusher : public idtBaseThread
{
public:
    sdbFlusher();

    ~sdbFlusher();

    IDE_RC start();

    void run();

    IDE_RC initialize(UInt          aFlusherID,
                      UInt          aPageSize,
                      UInt          aPageCount,
                      sdbCPListSet *aCPListSet);

    IDE_RC destroy();

    void wait4OneJobDone();

    IDE_RC finish(idvSQL *aStatistics, idBool *aNotStarted);

    IDE_RC flushForReplacement(idvSQL         *aStatistics,
                               ULong           aReqFlushCount,
                               sdbFlushList   *aFlushList,
                               sdbLRUList     *aLRUList,
                               ULong          *aFlushedCount);

    IDE_RC flushForCheckpoint( idvSQL          * aStatistics,
                               ULong             aMinFlushCount,
                               ULong             aRedoDirtyPageCnt,
                               UInt              aRedoLogFileCount,
                               sdbCheckpointType aCheckpointType,
                               ULong           * aFlushedCount );

    IDE_RC flushDBObjectBCB(idvSQL       *aStatistics,
                            sdbFiltFunc   aFiltFunc,
                            void         *aFiltAgr,
                            smuQueueMgr  *aBCBQueue,
                            ULong        *aFlushedCount);

    void applyStatisticsToSystem();

    IDE_RC wakeUpOnlyIfSleeping(idBool *aWaken);

    void getMinRecoveryLSN(idvSQL *aStatistics,
                           smLSN  *aRet);

    UInt getWaitInterval(ULong aFlushedCount);

    inline sdbFlusherStat* getStat();

    inline idBool isRunning();

    void setDelayedFlushProperty( UInt aDelayedFlushListPct,
                                  UInt aDelayedFlushProtectionTimeMsec );

private:

    void addToLocalList( sdbBCB **aHead,
                         sdbBCB **aTail,
                         UInt    *aCount,
                         sdbBCB  *aBCB );

    IDE_RC writeIOB( idvSQL *aStatistics, 
                     idBool  aMoveToPrepare,
                     idBool  aMoveToSBuffer );

    IDE_RC copyToIOB( idvSQL *aStatistics,
                      sdbBCB *aBCB,
                      idBool  aMoveToPrepare,
                      idBool  aMoveToSBuffer );

    IDE_RC flushTempPage(idvSQL *aStatistics,
                         sdbBCB *aBCB,
                         idBool  aMoveToPrepare);

    IDE_RC copyTempPage(idvSQL *aStatistics,
                        sdbBCB *aBCB,
                        idBool  aMoveToPrepare,
                        idBool  aMoveToSBuffer );

    IDE_RC syncAllFile4Flush( idvSQL   * aStatistics );

     /* PROJ-2102 Secondary Buffer */
    inline idBool isSBufferServiceable( void );

    inline idBool needToUseDWBuffer( idBool aMoveToSBuffer );

    inline idBool needToMovedownSBuffer( idBool    aMoveToSBuffer,  
                                         sdbBCB  * aBCB );

    inline idBool needToSkipBCB( sdbBCB  * aBCB );

    inline idBool needToSyncTBS( idBool aMoveToSBuffer );

    inline idBool isHotBCBByFlusher( sdbBCB *aBCB );

    IDE_RC delayedFlushForReplacement( idvSQL      * aStatistics,
                                       sdbFlushJob * aNormalFlushJob,
                                       ULong       * aRetFlushedCount );
private:
    /* Data page 크기. 현재는 8K */
    UInt           mPageSize;

    /* flusher 쓰레드 시작 여부. run시에 ID_TRUE로 설정  */
    idBool         mStarted;

    /* flusher 쓰레드를 종료 시키기 위해서 외부에서 ID_TRUE로 설정한다. */
    idBool         mFinish;

    /* 플러셔 식별자 */
    UInt           mFlusherID;

    /* 현재 IOB에 복사된 page개수 */
    UInt           mIOBPos;

    /* IOB가 최대로 가질 수 있는 페이지 개수 */
    UInt           mIOBPageCount;

    /* 각 mIOB의 메모리 공간 주소값을 가지고 있는 배열.
     * 만약 3번째 mIOB의 주소값을 접근한다면, mIOBPtr[3]이런 식으로 접근 */
    UChar        **mIOBPtr;

    /* mIOB에 저장된 각 frame의 BCB를 포인팅하고 있는 array */
    sdbBCB       **mIOBBCBArray;

    /* mIOBSpace를 8K align한 메모리 공간.
     * 실제 mIOB를 접근할때는 이곳으로 접근한다. */
    UChar         *mIOB;

    /* 실제 IOB 메모리 공간. 8K align되어 있지 않고
     * 실제 필요한 양보다 8K더 크다. 실제 메모리 할당및 해제에만 쓰인다. */
    UChar         *mIOBSpace;

    /* mMinRecoveryLSN 설정에 대한 동시성을 유지하기 위한 뮤텍스 */
    iduMutex       mMinRecoveryLSNMutex;

    /* mIOB에 저장된 page의 recoveryLSN중 가장 작은 값  */
    smLSN          mMinRecoveryLSN;

    /* mIOB에 저장된 page의 pageLSN값중 가장 큰값. WAL을 지키기 위해 page flush전에
     * log flush를 위해 쓰임 */
    smLSN          mMaxPageLSN;

    /* 하나의 작업을 끝마친 flusher가 쉬어야 할 시간을 지정해 준다. */
    UInt           mWaitTime;

    /* 플러시 하는  buffer pool에 속한 checkpoint List set*/
    sdbCPListSet  *mCPListSet;

    /* 플러시 하는  buffer pool */
    sdbBufferPool *mPool;

    // 통계 정보를 위한 private session 및 statistics 정보
    idvSQL         mStatistics;
    idvSession     mCurrSess;
    idvSession     mOldSess;

    // 쓰레드 제어를 위한 맴버
    iduCond        mCondVar;
    iduMutex       mRunningMutex;

    // DWFile 객체를 flusher마다 가지고 있는다.
    sddDWFile      mDWFile;

    // 통계정보를 수집하는 객체
    sdbFlusherStat mStat;

    // WritePage후 Sync할 TBSID들
    sdbSyncFileInfo * mArrSyncFileInfo;

    // Secondary Buffer 가 사용 가능한가.
    idBool         mServiceable;

    /* PROJ-2669
     * Delayed Flush 기능을 on/off 하며
     * 0이 아닌경우 Delayed Flush List 의 최대 길이(percent)를 의미한다 */
    UInt           mDelayedFlushListPct;

    /* PROJ-2669
     * Flush 대상 BCB가 
     * Touch 된 후 시간이 기준 시간(DELAYED_FLUSH_PROTECTION_TIME_MSEC) 이하일 경우
     * Delyaed Flush List 로 옮겨지며 Flush 동작을 나중으로 미룬다.  */
    ULong          mDelayedFlushProtectionTimeUsec;
};

sdbFlusherStat* sdbFlusher::getStat()
{
    return &mStat;
}

idBool sdbFlusher::isRunning()
{
    if ((mStarted == ID_TRUE) && (mFinish == ID_FALSE))
    {
        return ID_TRUE;
    }
    return ID_FALSE;
}

/******************************************************************************
 * Description :
 *   Flush list 에 포함된 BCB를 대상으로 HOT BCB 여부를 판단한다.
 *   기준: 현재 시간 - 마지막 BCB Touch 시간 <= HOT_FLUSH_PROTECTION_TIME_MSEC
 *
 *  aBCB         - [IN]  BCB Pointer
 *  aCurrentTime - [IN]  Current Time
 ******************************************************************************/
inline idBool sdbFlusher::isHotBCBByFlusher( sdbBCB *aBCB )
{
    idvTime sCurrentTime;
    ULong   sTime;

    IDV_TIME_GET( &sCurrentTime );

    sTime = IDV_TIME_DIFF_MICRO( &aBCB->mLastTouchCheckTime, &sCurrentTime );

    return  (idBool)( sTime < mDelayedFlushProtectionTimeUsec );
}
#endif // _O_SDB_FLUSHER_H_

