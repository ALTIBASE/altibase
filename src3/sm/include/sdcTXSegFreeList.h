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
 * $Id: sdcTXSegFreeList.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

# ifndef _O_SDC_TXSEG_FREE_LIST_H_
# define _O_SDC_TXSEG_FREE_LIST_H_ 1

# include <idu.h>
# include <sdcDef.h>
# include <sdcTSSegment.h>
# include <sdcUndoSegment.h>

class sdcTXSegFreeList;

/*
 * 트랜잭션 세그먼트 엔트리 정의
 */
typedef struct sdcTXSegEntry
{
    smuList            mListNode;       // Dbl-Linked List Node

    sdcTXSegFreeList * mFreeList;       // FreeList Pointer
    sdcTXSegStatus     mStatus;         // 엔트리상태
    UInt               mEntryIdx;       // 엔트리순번

    smSCN              mMaxCommitSCN;   // 최종 사용한 트랜잭션의 CSCN

    sdcTSSegment       mTSSegmt;        // TSS Segment
    sdcUndoSegment     mUDSegmt;        // Undo Segment

    sdSID              mTSSlotSID;      // TSS Slot의 SID
    sdRID              mExtRID4TSS;     // 할당한 TSS 페이지의 ExtRID

    sdRID              mFstExtRID4UDS;  // 처음 할당한 Undo 페이지의 ExtRID
    scPageID           mFstUndoPID;     // 처음 할당한 Undo 페이지의 PID
    scSlotNum          mFstUndoSlotNum; // 처음 할당한 Undo Record의 SlotNum

    sdRID              mLstExtRID4UDS;  // 마지막 할당한 Undo 페이지의 ExtRID
    scPageID           mLstUndoPID;     // 마지막 할당한 Undo 페이지의 PID
    scSlotNum          mLstUndoSlotNum; // 마지막 할당한 Undo Record의 SlotNum

} sdcTXSegEntry;

class sdcTXSegFreeList
{

public:

    IDE_RC initialize( sdcTXSegEntry       * aArrEntry,
                       UInt                  aEntryIdx,
                       SInt                  aFstItem,
                       SInt                  aLstItem );

    IDE_RC destroy();

    void   allocEntry( sdcTXSegEntry ** aEntry );

    // BUG-29839 재사용된 undo page에서 이전 CTS를 보려고 할 수 있음.
    // 재현하기 위해 transaction에 특정 segment entry를 binding하는 기능 추가
    void   allocEntryByEntryID( UInt             aEntryID,
                                sdcTXSegEntry ** aEntry );

    void   freeEntry( sdcTXSegEntry * aEntry,
                      idBool          aMoveToFirst );

    static void  tryAllocExpiredEntryByIdx( UInt             aEntryIdx,
                                            sdpSegType       aSegType,
                                            smSCN          * aOldestTransBSCN,
                                            sdcTXSegEntry ** aEntry );

    static void   tryAllocEntryByIdx( UInt             aEntryIdx, 
                                      sdcTXSegEntry ** aEntry );

    IDE_RC lock()   { return mMutex.lock( NULL /* idvSQL* */); }
    IDE_RC unlock() { return mMutex.unlock(); }

    inline void   initEntry4Runtime( sdcTXSegEntry    * aEntry,
                                     sdcTXSegFreeList * aFreeList );

    static inline idBool isEntryExpired( sdcTXSegEntry * aEntry,
                                         sdpSegType      aSegType,
                                         smSCN         * aCheckSCN );

public:

    SInt            mEntryCnt;       /* 총 Entry 개수 */
    SInt            mFreeEntryCnt;   /* Free Entry 개수 */
    smuList         mBase;           /* FreeList의 Base Node */

private:

    iduMutex        mMutex;          /* FreeList의 동시성 제어 */
};

/***********************************************************************
 *
 * Description : 트랜잭션 세그먼트 엔트리 초기화
 *
 * 트랜잭션 세그먼트 엔트리를 초기화한다.
 *
 * aEntry    - [IN] 트랜잭션 세그먼트 Entry 포인터
 * aFreeList - [IN] 트랜잭션 세그먼트 FreeList 포인터
 *
 ***********************************************************************/
inline void sdcTXSegFreeList::initEntry4Runtime( sdcTXSegEntry    * aEntry,
                                                 sdcTXSegFreeList * aFreeList )
{
    IDE_ASSERT( aEntry != NULL );

    aEntry->mFreeList       = aFreeList;

    aEntry->mTSSlotSID      = SD_NULL_SID;
    aEntry->mExtRID4TSS     = SD_NULL_RID;
    aEntry->mFstExtRID4UDS  = SD_NULL_RID;
    aEntry->mLstExtRID4UDS  = SD_NULL_RID;
    aEntry->mFstUndoPID     = SD_NULL_PID;
    aEntry->mFstUndoSlotNum = SC_NULL_SLOTNUM;
    aEntry->mLstUndoPID     = SD_NULL_PID;
    aEntry->mLstUndoSlotNum = SC_NULL_SLOTNUM;
}


/******************************************************************************
 *
 * Description : Steal 연산을 위한 트랜잭션 세그먼트 엔트리 할당 가능성을
 *               Optimistics하게 확인
 *
 * 각 엔트리의 Max CommitSCN이 aCheckSCN 보다 작다는 것을 확인하고,
 * Segment Size가 ExtDir 페이지 2개이상인 경우에 해당 엔트리의 
 * 세그먼트들을 모두 Expired 되었음을 보장한다.
 *
 * aEntry           - [IN] 트랜잭션 세그먼트 엔트리 포인터
 * aSegType         - [IN] Segment Type
 * aOldestTransBSCN - [IN] Active한 트랜잭션들이 가진 Statement 중에서
 *                         가장 오래전에 시작한 Statement의 SCN
 *
 ******************************************************************************/
inline idBool sdcTXSegFreeList::isEntryExpired( 
                                         sdcTXSegEntry * aEntry,
                                         sdpSegType      aSegType,
                                         smSCN         * aOldestTransBSCN )
{
    sdpSegCCache * sSegCache;
    idBool         sResult            = ID_FALSE;
    ULong          sExtDirSizeByBytes = 0 ;

    if ( SM_SCN_IS_LT( aOldestTransBSCN, &aEntry->mMaxCommitSCN ) )
    {
        return sResult;
    }

    switch( aSegType )
    {
        case SDP_SEG_TYPE_TSS:
            sSegCache = 
                (sdpSegCCache*)aEntry->mTSSegmt.getSegHandle()->mCache;
            sExtDirSizeByBytes = 
                         ( smuProperty::getTSSegExtDescCntPerExtDir() *
                           sSegCache->mPageCntInExt * 
                           SD_PAGE_SIZE );
            break;
        
        case SDP_SEG_TYPE_UNDO:
            sSegCache = 
                (sdpSegCCache*)aEntry->mUDSegmt.getSegHandle()->mCache;
            sExtDirSizeByBytes = 
                         ( smuProperty::getUDSegExtDescCntPerExtDir() *
                           sSegCache->mPageCntInExt * 
                           SD_PAGE_SIZE );
            break;

        default:
            IDE_ASSERT( 0 );
            break;
    }

    if ( sSegCache->mSegSizeByBytes >= (sExtDirSizeByBytes* 2) ) 
    {
        sResult = ID_TRUE;
    }

    return sResult;
}


# endif
