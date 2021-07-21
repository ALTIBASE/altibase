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
 * $Id: sdnbBUBuild.h
 *
 * Description :
 *
 * Implementation Of Parallel Building Disk Index
 *
 **********************************************************************/

#ifndef _O_SDNB_BUBUILD_H_
#define _O_SDNB_BUBUILD_H_ 1

#include <idu.h>
#include <idtBaseThread.h>
#include <idv.h>
#include <smcDef.h>
#include <smnDef.h>
#include <sdnbDef.h>
#include <sdnbModule.h>
#include <smuQueueMgr.h>

class sdnbBUBuild : public idtBaseThread
{
public:

    /* 쓰레드 초기화 */
    IDE_RC initialize( UInt             aTotalThreadCnt,
                       UInt             aID,
                       smcTableHeader * aTable,
                       smnIndexHeader * aIndex,
                       UInt             aMergePageCnt,
                       UInt             aAvailablePageSize,
                       UInt             aSortAreaPageCnt,
                       UInt             aInsertableKeyCnt,
                       idBool           aIsNeedValidation,
                       UInt             aBuildFlag,
                       idvSQL*          aStatistics );

    IDE_RC destroy();         /* 쓰레드 해제 */

    /* Index Build Main */
    static IDE_RC main( idvSQL          *aStatistics,
                        smcTableHeader  *aTable,
                        smnIndexHeader  *aIndex,
                        UInt             aThreadCnt,
                        ULong            aTotalSortAreaSize,
                        UInt             aTotalMergePageCnt,
                        idBool           aIsNeedValidation,
                        UInt             aBuildFlag,
                        UInt             aStatFlag );
    
    sdnbBUBuild();
    virtual ~sdnbBUBuild();

    inline UInt getErrorCode() { return mErrorCode; };
    inline ideErrorMgr* getErrorMgr() { return &mErrorMgr; };
    
    static IDE_RC updateStatistics( idvSQL         * aStatistics,
                                    sdrMtx         * aMtx,
                                    smnIndexHeader * aIndex,
                                    scPageID         aMinNode,
                                    scPageID         aMaxNode,
                                    SLong            aNumPages,
                                    SLong            aClusteringFactor,
                                    SLong            aIndexHeight,
                                    SLong            aNumDist,
                                    SLong            aKeyCount,
                                    UInt             aStatFlag );

private:
    UInt               mLeftSizeThreshold;  // 추출키를 저장할지 말지를 나누는 기준

    idBool             mFinished;    /* 쓰레드 실행 여부 flag */
    idBool *           mContinue;    /* 쓰레드 중단 여부 flag */
    UInt               mErrorCode;
    ideErrorMgr        mErrorMgr;

    UInt               mTotalThreadCnt;
    UInt               mTotalMergeCnt;
    UInt               mID;          /* 쓰레드 번호 */
    
    idvSQL*            mStatistics; /* TASK-2356 Altibase Wait Interface
                                     * 통계정보를 정확히 측정하려면,
                                     * 쓰레드 개수만큼 있어야 한다.
                                     * 현재는 통계정보를 수집하지 않고,
                                     * SessionEvent만 체크하는 용도로 사용한다. */
    void             * mTrans;
    smcTableHeader   * mTable;
    smnIndexHeader   * mIndex;
    sdnbLKey         **mKeyMap;       /* 작업공간   ref-map */
    UChar            * mKeyBuffer;    /* 작업공간   Pointer */
    UInt               mKeyBufferSize;/* 작업공간의 크기 */
    iduStackMgr        mSortStack;        //BUG-27403 for Quicksort

    UInt               mInsertableMaxKeyCnt; /* 작업 공간에서 작업 가능한
                                             KeyMap의 최대 개수 */
    smuQueueMgr        mPIDBlkQueue;      /* merge block에 대한 queue */
    UInt               mFreePageCnt;
    idBool             mIsNeedValidation;
    sdrMtxLogMode      mLoggingMode;
    idBool             mIsForceMode;
    UInt               mBuildFlag;
    UInt               mPhase;         /* 쓰레드의 작업 단계 */
    iduStackMgr        mFreePage;      /* Free temp page head */
    UInt               mMergePageCount;
    idBool             mIsSuccess;

    virtual void run();                         /* main 실행 루틴 */

    /* Phase 1. Key Extraction & In-Memory Sort */
    IDE_RC extractNSort( sdnbStatistic * aIndexStat );
    /* Phase 2. merge sorted run */
    IDE_RC merge( sdnbStatistic * aIndexStat );
    /* Phase 3. make tree */    
    IDE_RC makeTree( sdnbBUBuild    * aThreads, 
                     UInt             aThreadCnt,
                     UInt             aMergePageCnt,
                     sdnbStatistic  * aIndexStat,
                     UInt             aStatFlag );

    /* 쓰레드 작업 시작 루틴 */
    static IDE_RC threadRun( UInt         aPhase,
                             UInt         aThreadCnt,
                             sdnbBUBuild *aThreads );
    
    /* quick sort with KeyInfoMap*/
    IDE_RC quickSort( sdnbStatistic *aIndexStat,
                      UInt           aHead,
                      UInt           aTail );

    /* check sorted block */
    idBool checkSort( sdnbStatistic *aIndexStat,
                      UInt           aHead,
                      UInt           aTail );

    /* 작업 공간에 있는 sorted block을 Temp 공간으로 이동 */
    IDE_RC storeSortedRun( UInt    aHead,
                           UInt    aTail,
                           UInt  * aLeftPos,
                           UInt  * aLeftSize);

    IDE_RC preparePages( UInt aNeedPageCnt );

    /* Temp 공간을 위한 free list 유지 */
    IDE_RC allocPage( sdrMtx         * aMtx,
                      scPageID       * aPageID,
                      sdpPhyPageHdr ** aPageHdr,
                      sdnbNodeHdr   ** aNodeHdr,
                      sdpPhyPageHdr  * aPrevPageHdr,
                      scPageID       * sPrevPID,
                      scPageID       * sNextPID );

    IDE_RC freePage( scPageID aPageID );
    IDE_RC mergeFreePage( sdnbBUBuild  * aThreads,
                          UInt           aThreadCnt );
    IDE_RC removeFreePage();
    
    /* KeyValue로부터 KeySize를 얻는다*/
    UShort getKeySize( UChar *aKeyValue );

    /* KeyMap에서 swap */
    void   swapKeyMap( UInt aPos1,
                       UInt aPos2 );

    IDE_RC heapInit( sdnbStatistic    * aIndexStat,
                     UInt               aRunCount,
                     UInt               aHeapMapCount,
                     SInt             * aHeapMap,
                     sdnbMergeRunInfo * aMergeRunInfo );

    IDE_RC heapPop( sdnbStatistic    * aIndexStat,
                    UInt               aMinIdx,
                    idBool           * aIsClosedRun,
                    UInt               aHeapMapCount,
                    SInt             * aHeapMap,
                    sdnbMergeRunInfo * aMergeRunInfo );
    
    IDE_RC write2LeafNode( sdnbStatistic  *aIndexStat,
                           sdrMtx         *aMtx,
                           sdpPhyPageHdr **aPage,
                           sdnbStack      *aStack,
                           SShort         *aSlotSeq,
                           sdnbKeyInfo    *aKeyInfo,
                           idBool         *aIsDupValue );
    
    IDE_RC write2ParentNode( sdrMtx         *aMtx,
                             sdnbStack      *aStack,
                             UShort          aHeight,
                             sdnbKeyInfo    *aKeyInfo,
                             scPageID        aPrevPID,
                             scPageID        aChildPID );    
    
    IDE_RC getKeyInfoFromLKey( scPageID      aPageID,
                               SShort        aSlotSeq,
                               sdpPhyPageHdr *aPage,
                               sdnbKeyInfo   *aKeyInfo );
    // To fix BUG-17732
    idBool isNull( sdnbHeader * aHeader,
                   UChar      * aRow );
};

#endif // _O_SDNB_BUBUILD_H_
