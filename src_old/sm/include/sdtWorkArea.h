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
 * $Id
 *
 * Sort 및 Hash를 하는 공간으로, 최대한 동시성 처리 없이 동작하도록 구현된다.
 * 따라서 SortArea, HashArea를 아우르는 의미로 사용된다. 즉
 * SortArea, HashArea는WorkArea에 속한다.
 *
 * 원래 PrivateWorkArea를 생각했으나, 첫번째 글자인 P가 여러기지로 사용되기
 * 때문에(예-Page); WorkArea로 변경하였다.
 * 일반적으로 약자 및 Prefix로 WA를 사용하지만, 몇가지 예외가 있다.
 * WCB - BCB와 역할이 같기 때문에, 의미상 WACB보다 WCB로 수정하였다.
 * WPID - WAPID는 좀 길어서 WPID로 줄였다.
 *
 **********************************************************************/

#ifndef _O_SDT_WORKAREA_H_
#define _O_SDT_WORKAREA_H_ 1

#include <idl.h>
#include <smDef.h>
#include <smiDef.h>
#include <sdtDef.h>
#include <iduLatch.h>
#include <smuWorkerThread.h>
#include <sdtTempPage.h>
#include <sdtWAGroup.h>
#include <sdtWASegment.h>

class sdtWorkArea
{
public:
    /******************************************************************
     * WorkArea
     ******************************************************************/
    static IDE_RC initializeStatic();
    static IDE_RC createWA();
    static IDE_RC resetWA();
    static IDE_RC destroyStatic();
    static IDE_RC dropWA();

    static sdtWAState getWAState() { return mWAState; };
    static UInt getWAFlusherCount() { return mWAFlusherCount; };
    static sdtWAFlusher* getWAFlusher(UInt aWAFlusherIdx ) { return &mWAFlusher[aWAFlusherIdx]; };

    static UInt getGlobalWAFlusherIdx()
    {
        mGlobalWAFlusherIdx = ( mGlobalWAFlusherIdx + 1 ) % sdtWorkArea::getWAFlusherCount();
        return mGlobalWAFlusherIdx;
    }

    static IDE_RC allocWAFlushQueue( sdtWAFlushQueue ** aWAFlushQueue )
    { return  mFlushQueuePool.alloc( (void**)aWAFlushQueue ); };

    static IDE_RC freeWAFlushQueue( sdtWAFlushQueue * aWAFlushQueue )
    {  return  mFlushQueuePool.memfree( (void*)aWAFlushQueue ); };

    static UInt getFreeWAExtentCount() { return mFreePool.getTotItemCnt(); };

    /******************************************************************
     * Segment
     ******************************************************************/
    static IDE_RC allocWASegmentAndExtent( idvSQL             * aStatistics,
                                           sdtWASegment      ** aWASegment,
                                           smiTempTableStats ** aStatsPtr,
                                           UInt                 aExtentCount );
    static IDE_RC freeWASegmentAndExtent( sdtWASegment   * aWASegment );


    static void lock()
    {
        IDE_ASSERT( mMutex.lock( NULL ) == IDE_SUCCESS );
    }
    static void unlock()
    {
        IDE_ASSERT( mMutex.unlock() == IDE_SUCCESS );
    }

    /******************************************************************
     * Flusher
     ******************************************************************/
    static IDE_RC   assignWAFlusher( sdtWASegment       * aWASegment );
    static IDE_RC   releaseWAFlusher( sdtWAFlushQueue   * aWAFlushQueuePtr );
    static void     flusherRun( void * aParam );
    static IDE_RC   flushTempPages( sdtWAFlushQueue * aWAFlushQueue );

    static IDE_RC   flushTempPagesInternal( sdtWAFlushQueue   * aWAFlushQueue,
                                            sdtWCB            * aWCBPtr,
                                            UInt                aPageCount );

    static idBool   isSiblingPage( sdtWCB * aPrevWCBPtr,
                                   sdtWCB * aNextWCBPtr );

    static void     incQueueSeq( SInt * aSeq );
    static idBool   isEmptyQueue( sdtWAFlushQueue * aWAFlushQueue );
    static IDE_RC   pushJob( sdtWASegment * aWASegment,
                             scPageID       aWPID );
    static IDE_RC   popJob( sdtWAFlushQueue  * aWAFlushQueue,
                            scPageID         * aWPID,
                            sdtWCB          ** aWCBPtr);


public:
    /***********************************************************
     * FixedTable용 함수들 ( for X$TEMPINFO )
     ***********************************************************/
    static IDE_RC buildTempInfoRecord( void                * aHeader,
                                       iduFixedTableMemory * aMemory );

private:
    static UChar               * mArea;        /*WorkArea가 있는 곳 */
    static UChar               * mAlignedArea; /*8k Align WorkArea가 있는 곳 */
    static ULong                 mWASize;      /*WorkArea의 Byte 크기 */
    static iduMutex              mMutex;
    static iduStackMgr           mFreePool;    /*빈 Extent를 관리하는 곳 */
    static sdtWASegment        * mWASegListHead;
    static iduMemPool            mFlushQueuePool;

    static smuWorkerThreadMgr    mFlusherMgr;
    static UInt                  mGlobalWAFlusherIdx;
    static UInt                  mWAFlusherCount;
    static UInt                  mWAFlushQueueSize;
    static sdtWAFlusher        * mWAFlusher;
    static sdtWAState            mWAState;

};

#endif //_O_SDT_WORK_AREA
