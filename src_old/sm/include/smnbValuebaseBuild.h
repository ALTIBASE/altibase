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
 * $Id: smnbValuebaseBuild.h 14768 2006-01-03 00:57:49Z unclee $
 **********************************************************************/

#ifndef _O_SMNB_VALUE_BASE_BUILD_H_
# define _O_SMNB_VALUE_BASE_BUILD_H_ 1

# include <idtBaseThread.h>
# include <iduStackMgr.h>
# include <smnDef.h>
# include <smnbDef.h>
# include <smp.h>
# include <smnbModule.h>

class smnbValuebaseBuild : public idtBaseThread
{
public:
    smnbValuebaseBuild();
    virtual ~smnbValuebaseBuild();

    /* Index Build Main */
    static IDE_RC buildIndex( void            * aTrans,
                              idvSQL          * aStatistics,
                              smcTableHeader  * aTable,
                              smnIndexHeader  * aIndex,
                              UInt              aThreadCnt,
                              idBool            aIsNeedValidation,
                              UInt              aKeySize,
                              UInt              aRunSize,
                              smnGetPageFunc    aGetPageFunc,
                              smnGetRowFunc     aGetRowFunc );

    /* 쓰레드 초기화 */
    IDE_RC initialize( void            * aTrans,
                       smcTableHeader  * aTable,
                       smnIndexHeader  * aIndex,
                       UInt              aThreadCnt,
                       UInt              aMyTid,
                       idBool            aIsNeedValidation,
                       UInt              aRunSize,
                       iduMemPool        aRunPool,
                       UInt              aMaxUnionRunCnt,
                       UInt              aKeySize,
                       idvSQL          * aStatistics,
                       smnGetPageFunc    aGetPageFunc,
                       smnGetRowFunc     aGetRowFunc );

    /* BUG-27403 쓰레드 정리 */
    IDE_RC destroy( );

    void run();
    inline UInt getErrorCode() { return mErrorCode; };
    inline ideErrorMgr* getErrorMgr() { return &mErrorMgr; };
    
public:
    smnbValuebaseBuild * mThreads;
    
private:
    UInt                 mPhase;
    void               * mTrans;
    idvSQL*              mStatistics;
    UInt                 mMaxSortRunKeyCnt;
    UInt                 mMaxMergeRunKeyCnt;
    UInt                 mKeySize;

    UInt                 mMaxUnionRunCnt;

    UInt                 mErrorCode;
    ideErrorMgr          mErrorMgr;
    
    smnbBuildRun       * mFstRun;
    smnbBuildRun       * mLstRun;

    idBool               mIsNeedValidation;
    idBool               mIsUnique;
    
    smcTableHeader     * mTable;
    smnIndexHeader     * mIndex;

    smnGetPageFunc       mGetPageFunc;
    smnGetRowFunc        mGetRowFunc;

    UInt                 mThreadCnt;
    UInt                 mMyTID;
    UInt                 mSortedRunCount;
    UInt                 mMergedRunCount;

    idBool               mIsSuccess;
    
    iduMemPool           mRunPool;
    iduStackMgr          mSortStack;        //BUG-27403 for Quicksort

    // Phase 1. Key Extraction & Sort
    IDE_RC extractNSort( smnbStatistic    * aIndexStat );

    // Phase 2. Merge Runs
    IDE_RC mergeRun( smnbStatistic        * aIndexStat );

    // Phase 2-1. Union Merge Runs
    IDE_RC unionMergeRun( smnbStatistic   * aIndexStat );
    
    // Phase 3. Make Tree
    IDE_RC makeTree( smnbValuebaseBuild   * aThreads,
                     smnbStatistic        * aIndexStat,
                     UInt                   aThreadCnt,
                     smnbIntStack         * aStack );

    IDE_RC isRowUnique( smnbStatistic       * aIndexStat,
                        smnbHeader          * aIndexHeader,
                        smnbKeyInfo         * aKeyInfo1,
                        smnbKeyInfo         * aKeyInfo2,
                        SInt                * aResult );

    IDE_RC moveSortedRun( smnbBuildRun        * aSortRun,
                          UInt                  aSlotCount,
                          UShort              * aKeyOffset );

    IDE_RC quickSort( smnbStatistic       * aIndexStat,
                      SChar               * aBuffer,
                      SInt                  aHead,
                      SInt                  aTail );

    IDE_RC merge( smnbStatistic           * aIndexStat,
                  UInt                      aMergeRunInfoCnt,
                  smnbMergeRunInfo        * aMergeRunInfo );
    
    void swapOffset( UShort * aOffset1,
                     UShort * aOffset2 );

    IDE_RC heapInit( smnbStatistic       * aIndexStat,
                     UInt                  aRunCount,
                     UInt                  aHeapMapCount,
                     SInt                * aHeapMap,
                     smnbMergeRunInfo    * aMergeRunInfo );
    
    IDE_RC heapPop( smnbStatistic        * aIndexStat,
                    UInt                   aMinIdx,
                    idBool               * aIsClosedRun,
                    UInt                   aHeapMapCount,
                    SInt                 * aHeapMap,
                    smnbMergeRunInfo     * aMergeRunInfo );

    SInt compareKeys( smnbStatistic      * aIndexStat,
                      const smnbColumn   * aColumns,
                      const smnbColumn   * aFence,
                      const void         * aKey1,
                      const void         * aKey2 );

    SInt compareKeys( smnbStatistic      * aIndexStat,
                      const smnbColumn   * aColumns,
                      const smnbColumn   * aFence,
                      SChar              * aKey1,
                      SChar              * aKey2,
                      SChar              * aRowPtr1,
                      SChar              * aRowPtr2,
                      idBool             * aIsEqual);

    static IDE_RC threadRun( UInt                 aPhase,
                             UInt                 aThreadCnt,
                             smnbValuebaseBuild * aThreads );

    IDE_RC write2LeafNode( smnbStatistic * aIndexStat,
                           smnbHeader    * aHeader,
                           smnbIntStack  * aStack,
                           SChar         * aRowPtr );

    IDE_RC write2ParentNode( smnbStatistic * aIndexStat,
                             smnbHeader    * aHeader,
                             smnbIntStack  * aStack,
                             SInt            aDepth,
                             smnbNode      * aChildPtr,
                             SChar         * aRowPtr,
                             void          * aKey );

    idBool isNull( smnbHeader              * aHeader,
                   SChar                   * aKeyValue );

    idBool isNullKey( smnbHeader              * aHeader,
                      SChar                   * aKeyValue );

    void makeKeyFromRow( smnbHeader   * aIndex,
                         SChar        * aRow,
                         SChar        * aKey );

    IDE_RC allocRun( smnbBuildRun  ** aRun );

    void freeRun( smnbBuildRun      * aRun );

    SInt getPivot( smnbStatistic * aIndexStat,
                   smnbHeader    * aIndexHeader,
                   SChar         * aBuffer,
                   UShort        * aKeyOffset,
                   SInt            aLeftPos,
                   SInt            aRightPos );

    SInt getPivot4Large( smnbStatistic * aIndexStat,
                         smnbHeader    * aIndexHeader,
                         SChar         * aBuffer,
                         UShort        * aKeyOffset,
                         SInt            aLeftPos,
                         SInt            aRightPos );
};

#endif /* _O_SMNB_VALUE_BASE_BUILD_H_ */
