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

#ifndef _O_SDS_FLUSHER_H_
#define _O_SDS_FLUSHER_H_ 1

#include <idu.h>
#include <idv.h>
#include <idtBaseThread.h>

#include <sdbDef.h>
#include <sdbLRUList.h>
#include <sdbFlushList.h>
#include <sdbFT.h>
#include <sddDWFile.h>
#include <smuQueueMgr.h>

#include <sdsBCB.h>
#include <sdsFlusherStat.h>

class sdsFlusher : public idtBaseThread
{
public:
    sdsFlusher();

    ~sdsFlusher();

    IDE_RC start();

    void run ( void );

    IDE_RC initialize( UInt  aFlusherID,
                       UInt  aPageSize, /* SD_PAGE_SIZE*/
                       UInt  aPageCount );

    IDE_RC destroy( void );

    void wait4OneJobDone( void );

    IDE_RC finish( idvSQL *aStatistics, idBool *aNotStarted );

    IDE_RC flushForReplacement( idvSQL   * aStatistics,
                                ULong      aReqFlushCount,
                                ULong      aIndex,
                                ULong    * aFlushedCount );

    IDE_RC flushForCheckpoint( idvSQL   * aStatistics,
                               ULong      aMinFlushCount,
                               ULong      aRedoDirtyPageCnt,
                               UInt       aRedoLogFileCount,
                               ULong    * aFlushedCount);

    IDE_RC flushForObject( idvSQL       * aStatistics,
                           sdsFiltFunc    aFiltFunc,
                           void         * aFiltAgr,
                           smuQueueMgr  * aBCBQueue,
                           ULong        * aFlushedCount);

    IDE_RC wakeUpSleeping( idBool *aWaken );

    void getMinRecoveryLSN( idvSQL *aStatistics,
                            smLSN  *aRet );

    UInt getWaitInterval( ULong aFlushedCount );

    inline sdsFlusherStat *getStat( void );

    inline idBool isRunning( void );

private:

    IDE_RC writeIOB( idvSQL *aStatistics );

    IDE_RC copyToIOB( idvSQL *aStatistics,   sdsBCB *aBCB );

    IDE_RC flushTempPage( idvSQL *aStatistics,
                          sdsBCB *aBCB );

    IDE_RC syncAllFile4Flush( idvSQL * aStatistics,
                              UInt     aSyncArrCnt );

private:
    /* Data page 크기. 현재는 8K */
    UInt            mPageSize;

    /* flusher 쓰레드 시작 여부. run시에 ID_TRUE로 설정  */
    idBool          mStarted;

    /* flusher 쓰레드를 종료 시키기 위해서 외부에서 ID_TRUE로 설정한다. */
    idBool          mFinish;

    /* 플러셔 식별자 */
    UInt            mFlusherID;

    /* 현재 IOB에 복사된 page개수 */
    UInt            mIOBPos;

    /* IOB가 최대로 가질 수 있는 페이지 개수 */
    UInt            mIOBPageCount;

    /* 각 mIOB의 메모리 공간 주소값을 가지고 있는 배열.
     * 만약 3번째 mIOB의 주소값을 접근한다면, mIOBPtr[3]이런 식으로 접근 */
    UChar        ** mIOBPtr;

    /* mIOB에 저장된 각 frame의 BCB를 포인팅하고 있는 array */
    sdsBCB       ** mIOBBCBArray;

    /* mIOBSpace를 8K align한 메모리 공간.
     * 실제 mIOB를 접근할때는 이곳으로 접근한다. */
    UChar         * mIOB;

    /* 실제 IOB 메모리 공간. 8K align되어 있지 않고
     * 실제 필요한 양보다 8K더 크다. 실제 메모리 할당및 해제에만 쓰인다. */
    UChar         * mIOBSpace;

    /* mMinRecoveryLSN 설정에 대한 동시성을 유지하기 위한 뮤텍스 */
    iduMutex        mMinRecoveryLSNMutex;

    /* mIOB에 저장된 page의 recoveryLSN중 가장 작은 값  */
    smLSN           mMinRecoveryLSN;

    /* mIOB에 저장된 page의 pageLSN값중 가장 큰값. WAL을 지키기 위해 page flush전에
     * log flush를 위해 쓰임 */
    smLSN           mMaxPageLSN;

    /* 하나의 작업을 끝마친 flusher가 쉬어야 할 시간을 지정해 준다. */
    UInt            mWaitTime;

    /* 플러시 하는  buffer pool에 속한 checkpoint List set*/
    sdbCPListSet  * mCPListSet;

    // 통계 정보를 위한 private session 및 statistics 정보
    idvSQL          mStatistics;
    idvSession      mCurrSess;

    // 쓰레드 제어를 위한 맴버
    iduCond         mCondVar;
    iduMutex        mRunningMutex;

    // DWFile 객체를 flusher마다 가지고 있는다.
    sddDWFile       mDWFile;

    sdsMeta       * mMeta;

    sdsFile       * mSBufferFile;

    sdsBufferArea * mBufferArea;
    // 통계정보를 수집하는 객체
    sdsFlusherStat mFlusherStat;

    // WritePage후 Sync할 TBSID들
    sdbSyncFileInfo * mFileInfoArray;
};

sdsFlusherStat* sdsFlusher::getStat( void )
{
    return &mFlusherStat;
}

idBool sdsFlusher::isRunning( void )
{
    if( (mStarted == ID_TRUE) && (mFinish == ID_FALSE) )
    {
        return ID_TRUE;
    }
    return ID_FALSE;
}
#endif // _O_SDS_FLUSHER_H_

