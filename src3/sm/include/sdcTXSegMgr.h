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
 * $Id: sdcTXSegMgr.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

# ifndef _O_SDC_TXSEG_MGR_H_
# define _O_SDC_TXSEG_MGR_H_ 1

# include <idu.h>
# include <sdcDef.h>
# include <sdcTXSegFreeList.h>
# include <sdcTSSegment.h>
# include <sdcUndoSegment.h>

class sdcTXSegMgr
{

public:

    static IDE_RC createSegs( idvSQL   * aStatistics,
                              void     * aTrans );

    static IDE_RC resetSegs( idvSQL * aStatistics );

    static IDE_RC initialize( idBool aIsAttachSegment );

    static IDE_RC rebuild();

    static IDE_RC destroy();

    static IDE_RC allocEntry( idvSQL          * aStatistics,
                              sdrMtxStartInfo * aStartInfo, 
                              sdcTXSegEntry  ** aEntry );

    // BUG-29839 재사용된 undo page에서 이전 CTS를 보려고 할 수 있음.
    // 재현하기 위해 transaction에 특정 segment entry를 binding하는 기능 추가
    static IDE_RC allocEntryBySegEntryID( UInt             aEntryID,
                                          sdcTXSegEntry ** aEntry );

    static IDE_RC tryStealFreeExtsFromOtherEntry(
                                        idvSQL          * aStatistics,
                                        sdrMtxStartInfo * aStartInfo,
                                        sdpSegType        aFromSegType,
                                        sdpSegType        aToSegType,
                                        sdcTXSegEntry   * aToEntry,
                                        smSCN           * aSysMinDskViewSCN,
                                        idBool          * aTrySuccess );

    static inline void freeEntry( sdcTXSegEntry  * aEntry,
                                  idBool           aMoveToFirst );

    static inline UInt getTotEntryCnt();

    static idBool isTSSegPID( scPageID aSegPID );

    static idBool isUDSegPID( scPageID aSegPID );

    static inline sdcTSSegment* getTSSegPtr( sdcTXSegEntry * aEntry );

    static inline sdcUndoSegment* getUDSegPtr( sdcTXSegEntry * aEntry );

    static IDE_RC markSCN( idvSQL        * aStatistics,
                           sdcTXSegEntry * aEntry,
                           smSCN         * aCommitSCN );

    /* BUG-31055 Can not reuse undo pages immediately after it is used to 
     *           aborted transaction */
    static IDE_RC shrinkExts( idvSQL        * aStatistics,
                              void          * aTrans,
                              sdcTXSegEntry * aEntry );

    static IDE_RC adjustEntryCount( UInt   aEntryCnt,
                                    UInt * aAdjustEntryCnt );

    static idBool isModifiedEntryCnt( UInt  aNewAdjustEntryCnt );

    static inline sdcTXSegEntry* getEntryByIdx( UInt aEntryIdx );

    static void tryAllocEntryByIdx( UInt             aEntryIdx,
                                    sdcTXSegEntry ** aEntry );
private:

    static IDE_RC tryStealFreeExts( idvSQL          * aStatistics,
                                    sdrMtxStartInfo * aStartInfo,
                                    sdpSegType        aFromSegType,
                                    sdpSegType        aToSegType,
                                    sdcTXSegEntry   * aFrEntry,
                                    sdcTXSegEntry   * aToEntry,
                                    idBool          * aTrySuccess );

    static void tryAllocExpiredEntry( UInt             aStartEntryIdx,
                                      sdpSegType       aSegType,
                                      smSCN          * aOldestTransBSCN,
                                      sdcTXSegEntry ** aEntry );

    static IDE_RC createTSSegs( idvSQL          * aStatistics,
                                sdrMtxStartInfo * aStartInfo );

    static IDE_RC createUDSegs( idvSQL          * aStatistics,
                                sdrMtxStartInfo * aStartInfo );

    static IDE_RC attachSegToEntry( sdcTXSegEntry  * aEntry,
                                    UInt             aEntryIdx );

    static void initEntry( sdcTXSegEntry  * aEntry,
                           UInt             aEntryIdx );

    static void finiEntry( sdcTXSegEntry * aEntry );

public:

    static sdcTXSegFreeList  * mArrFreeList;
    static sdcTXSegEntry     * mArrEntry;
    static UInt                mTotEntryCnt;
    static UInt                mFreeListCnt;

    static UInt                mCurFreeListIdx;
    static UInt                mCurEntryIdx4Steal;
    static idBool              mIsAttachSegment;

    enum { CONV_BUFF_SIZE = 8 }; // for idp::update
};


/***********************************************************************
 *
 * Description : 할당받은 트랜잭션 세그먼트 엔트리 해제
 *
 * aEntry - [IN] 트랜잭션 세그먼트 엔트리 포인터
 *
 ***********************************************************************/
inline void sdcTXSegMgr::freeEntry( sdcTXSegEntry * aEntry,
                                    idBool          aMoveToFirst )
{
    IDE_ASSERT( aEntry->mStatus == SDC_TXSEG_ONLINE );

    aEntry->mFreeList->freeEntry( aEntry, aMoveToFirst );
    return;
}

/***********************************************************************
 *
 * Description : 트랜잭션 세그먼트 엔트리 개수 반환
 *
 ***********************************************************************/
inline UInt sdcTXSegMgr::getTotEntryCnt()
{
   return mTotEntryCnt;
}

/***********************************************************************
 *
 * Description : Entry ID에 해당하는 Entry 포인터 반환
 *
 * aEntryIdx  - [IN] 트랜잭션 세그먼트 엔트리 ID
 *
 * [ 반환값 ]
 *
 * 트랜잭션 세그먼트 Entry 포인터
 *
 ***********************************************************************/
inline sdcTXSegEntry * sdcTXSegMgr::getEntryByIdx( UInt aEntryIdx )
{
    return &mArrEntry[ aEntryIdx ];
}

/***********************************************************************
 *
 * Description : Entry로부터 TSS 객체 포인터 반환
 *
 * aEntry  - [IN] 트랜잭션의 트랜잭션 세그먼트 Entry 포인터
 *
 * [ 반환값 ]
 *
 * TSS의 객체 포인터
 *
 ***********************************************************************/
inline sdcTSSegment* sdcTXSegMgr::getTSSegPtr( sdcTXSegEntry * aEntry )
{
    return &(aEntry->mTSSegmt);
}

/***********************************************************************
 *
 * Description : Entry로부터 UDS 객체 포인터 반환
 *
 * aEntry  - [IN] 트랜잭션의 트랜잭션 세그먼트 Entry 포인터
 *
 * [ 반환값 ]
 *
 * UDS의 객체 포인터
 *
 ***********************************************************************/
inline sdcUndoSegment* sdcTXSegMgr::getUDSegPtr( sdcTXSegEntry * aEntry )
{
    return &(aEntry->mUDSegmt);
}

# endif
