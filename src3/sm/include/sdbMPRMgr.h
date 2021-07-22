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

#ifndef _O_SDB_MPR_MGR_H_
#define _O_SDB_MPR_MGR_H_ 1

#include <sdp.h>

class sdbMPRMgr
{
public:
    /* BUG-33762 The MPR should uses the mempool instead allocBuff4DirectIO
     * function. */
    static IDE_RC initializeStatic();

    static IDE_RC destroyStatic();

    IDE_RC initialize( idvSQL           * aStatistics,
                       scSpaceID          aSpaceID,
                       sdpSegHandle     * aSegHandle,
                       sdbMPRFilterFunc   aFilter );

    IDE_RC destroy();

    IDE_RC beforeFst();

    IDE_RC getNxtPageID( void     * aFilterData,
                         scPageID * aPageID );
                         
    UChar* getCurPagePtr();

    /* TASK-4990 changing the method of collecting index statistics
     * MPR의 ParallelScan기능을 Sampling으로 이용하기 위해,
     * 기존 ParallalScan을 위한 Filter를 기본으로 제공한다. */
    static idBool filter4PScan( ULong   aExtSeq,
                                void  * aFilterData );
    /* Sampling 및 ParallelScan */
    static idBool filter4SamplingAndPScan( ULong   aExtSeq,
                                           void  * aFilterData );

private:
    IDE_RC initMPRKey();
    IDE_RC destMPRKey();

    IDE_RC fixPage( sdRID            aExtRID,
                    scPageID         aPageID );

    UInt getMPRCnt( sdRID    aCurReadExtRID,
                    scPageID aFstReadPID );

private:
    // 통계 정보
    idvSQL            *mStatistics;

    // SCAN에 참여할 Thread Count
    sdbMPRFilterFunc   mFilter;

    // Segment연산자 테이블
    sdpSegMgmtOp      *mSegMgmtOP;

    // Buffer Pool Pointer
    sdbBufferPool     *mBufferPool;

    // TableSpace ID
    scSpaceID          mSpaceID;

    // Segment Handle
    sdpSegHandle      *mSegHandle;

    // Segment Info로서 Segment Scan시 필요한 정보가 있다.
    sdpSegInfo         mSegInfo;

    // Segment Cache Info로서 Segment Scan시 필요한 정보가 있다.
    sdpSegCacheInfo    mSegCacheInfo;

    // 현재 읽고 있는 페이지가 있는 Extent Info
    sdpExtInfo         mCurExtInfo;

    // Multi Page Read Key
    sdbMPRKey          mMPRKey;

    // Multi Page Read Count로 이 단위로 DataFile을 Read한다.
    UInt               mMPRCnt;

    // 현재 읽고 있는 Extent RID
    sdRID              mCurExtRID;

    // 현재 Extent Sequence Number
    ULong              mCurExtSeq;

    // 현재 읽고 있는 페이지 ID
    scPageID           mCurPageID;

    // Segment에서 마지막으로 할당된 페이지 ID와 SeqNo
    scPageID           mLstAllocPID;
    ULong              mLstAllocSeqNo;

    // Segment에 마지막 PID, SeqNo를 갱신해야 하는가?
    idBool             mFoundLstAllocPage;

    // MPR에 의해서 Buffer Pool에 Page가 Pin되어있는가?
    idBool             mIsFetchPage;

    // MPR로 읽은 페이들을 Caching할 것인가?
    idBool             mIsCachePage;

    /* BUG-33762 [sm-disk-resource] The MPR should uses the mempool instead
     * allocBuff4DirectIO function. */
    static iduMemPool mReadIOBPool;
};

#endif // _O_SDB_MPR_MGR_H_
