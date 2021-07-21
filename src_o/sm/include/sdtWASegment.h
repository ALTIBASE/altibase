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

#ifndef _O_SDT_WA_SEGMENT_H_
#define _O_SDT_WA_SEGMENT_H_ 1

#include <idl.h>

#include <smDef.h>
#include <smiMisc.h>
#include <smiDef.h>
#include <sdtDef.h>
#include <sdtWorkArea.h>
#include <sctTableSpaceMgr.h>

class sdtWorkArea;
class sdtWASegment
{
public:
    /* 마지막으로 접근한 WAPage. Hint 및 Dump하기 위해 사용됨 */
    /* getNPageToWCB, getWAPagePtr, getWCB 용 Hint */
    sdtWCB             * mHintWCBPtr;

    sdtWAMapHdr          mNExtentMap;     /*NormalExtent들을 보관하는 Map*/
    UInt                 mNExtentCount;   /*NextentArray이 가진 Extent의 개수*/
    UInt                 mNPageCount;     /*할당한 NPage의 개수 */
    UInt                 mWAExtentCount;
    sdtWAMapHdr          mSortHashMapHdr; /*KeyMap, Heap, PIDList등의 Map */

    scSpaceID            mSpaceID;
    UInt                 mPageSeqInLFE;   /*LFE(LastFreeExtent)내에서 할당해서
                                            건내준페이지 번호 */
    void               * mLastFreeExtent; /*NExtentArray의 마지막의 Extent*/
    sdtWAGroup           mGroup[ SDT_WAGROUPID_MAX ];  /*WAGroup에 대한 정보*/
    iduStackMgr          mFreeNPageStack; /*FreenPage를 재활용용 */
    sdtWAFlushQueue    * mFlushQueue;     /*FlushQueue */
    idBool               mDiscardPage;    /*쫓겨난 Page가 있는가?*/

    idvSQL             * mStatistics;
    smiTempTableStats ** mStatsPtr;
    sdtWASegment       * mNext;
    sdtWASegment       * mPrev;

    /* 페이지 탐색용 Hash*/
    sdtWCB            ** mNPageHashPtr;
    UInt                 mNPageHashBucketCnt;

    /******************************************************************
     * Segment
     ******************************************************************/
    static IDE_RC createWASegment(idvSQL             * aStatistics,
                                  smiTempTableStats ** aStatsPtr,
                                  idBool               aLogging,
                                  scSpaceID            aSpaceID,
                                  ULong                aSize,
                                  sdtWASegment      ** aWASegment );

    static UInt   getWASegmentPageCount(sdtWASegment * aWASegment );

    static IDE_RC dropWASegment(sdtWASegment * aWASegment,
                                idBool         aWait4Flush);

    inline static void updateTempStats( smiTempTableHeader * aHeader );

    /******************************************************************
     * Group
     ******************************************************************/
    static IDE_RC createWAGroup( sdtWASegment     * aWASegment,
                                 sdtWAGroupID       aWAGroupID,
                                 UInt               aPageCount,
                                 sdtWAReusePolicy   aPolicy );
    static IDE_RC resetWAGroup( sdtWASegment      * aWASegment,
                                sdtWAGroupID        aWAGroupID,
                                idBool              aWait4Flush );
    static IDE_RC clearWAGroup( sdtWASegment      * aWASegment,
                                sdtWAGroupID        aWAGroupID );

    /************ Group 초기화 **************************/
    static void initInMemoryGroup( sdtWAGroup     * aGrpInfo );
    static void initFIFOGroup( sdtWAGroup     * aGrpInfo );
    static void initLRUGroup( sdtWASegment   * aWASegment,
                              sdtWAGroup     * aGrpInfo );

    static void initWCB( sdtWASegment * aWASegment,
                         sdtWCB       * aWCBPtr,
                         scPageID       aWPID );
    static void bindWCB( sdtWASegment * aWASegment,
                         sdtWCB       * aWCBPtr,
                         scPageID       aWPID );

    /************ Hint 처리 ***************/
    inline static scPageID getHintWPID( sdtWASegment * aWASegment,
                                        sdtWAGroupID   aWAGroupID );
    inline static IDE_RC setHintWPID( sdtWASegment * aWASegment,
                                      sdtWAGroupID   aWAGroupID,
                                      scPageID       aHintWPID );
    inline static sdtWAGroup *getWAGroupInfo( sdtWASegment * aWASegment,
                                              sdtWAGroupID   aWAGroupID );

    inline static idBool isInMemoryGroup( sdtWASegment * aWASegment,
                                          sdtWAGroupID   aWAGroupID );
    inline static idBool isInMemoryGroupByPID( sdtWASegment * aWASegment,
                                               scPageID       aWPID );

    static UInt   getWAGroupPageCount( sdtWASegment * aWASegment,
                                       sdtWAGroupID   aWAGroupID );
    static UInt   getAllocableWAGroupPageCount( sdtWASegment * aWASegment,
                                                sdtWAGroupID   aWAGroupID );

    static scPageID getFirstWPageInWAGroup( sdtWASegment * aWASegment,
                                            sdtWAGroupID   aWAGroupID );
    static scPageID getLastWPageInWAGroup( sdtWASegment * aWASegment,
                                           sdtWAGroupID   aWAGroupID );

    static IDE_RC getFreeWAPage( sdtWASegment * aWASegment,
                                 sdtWAGroupID   aWAGroupID,
                                 scPageID     * aRetPID );
    static IDE_RC mergeWAGroup(sdtWASegment     * aWASegment,
                               sdtWAGroupID       aWAGroupID1,
                               sdtWAGroupID       aWAGroupID2,
                               sdtWAReusePolicy   aPolicy );
    static IDE_RC splitWAGroup(sdtWASegment     * aWASegment,
                               sdtWAGroupID       aWAGroupIDSrc,
                               sdtWAGroupID       aWAGroupIDDst,
                               sdtWAReusePolicy   aPolicy );

    static IDE_RC dropAllWAGroup(sdtWASegment * aWASegment,
                                 idBool         aWait4Flush);
    static IDE_RC dropWAGroup(sdtWASegment * aWASegment,
                              sdtWAGroupID   aWAGroupID,
                              idBool         aWait4Flush);



    /******************************************************************
     * Page 주소 관련 Operation
     ******************************************************************/
    inline static void convertFromWGRIDToNGRID( sdtWASegment * aWASegment,
                                                scGRID         aWGRID,
                                                scGRID       * aNGRID );
    inline static IDE_RC getWCBByNPID( sdtWASegment * aWASegment,
                                       sdtWAGroupID   aWAGroupID,
                                       scSpaceID      aNSpaceID,
                                       scPageID       aNPID,
                                       sdtWCB      ** aRetWCB );
    inline static IDE_RC getWPIDFromGRID(sdtWASegment * aWASegment,
                                         sdtWAGroupID   aWAGroupID,
                                         scGRID         aGRID,
                                         scPageID     * aRetPID );

    /******************************************************************
     * Page 상태 관련 Operation
     ******************************************************************/
    /*************************  DirtyPage  ****************************/
    inline static IDE_RC setDirtyWAPage(sdtWASegment * aWASegment,
                                        scPageID       aWPID);
    static IDE_RC writeDirtyWAPages(sdtWASegment * aWASegment,
                                    sdtWAGroupID   aWAGroupID );
    static IDE_RC makeInitPage( sdtWASegment * aWASegment,
                                scPageID       aWPID,
                                idBool         aWait4Flush);
    inline static void bookedFree(sdtWASegment * aWASegment,
                                  scPageID       aWPID);

    /*************************  Fix Page  ****************************/
    inline static IDE_RC fixPage(sdtWASegment * aWASegment,
                                 scPageID       aWPID);
    inline static IDE_RC unfixPage(sdtWASegment * aWASegment,
                                   scPageID       aWPID);

    /*************************    State    ****************************/
    inline static idBool isFreeWAPage(sdtWASegment * aWASegment,
                                      scPageID       aWPID);
    inline static sdtWAPageState getWAPageState(sdtWASegment   * aWASegment,
                                                scPageID         aWPID );
    inline static sdtWAPageState getWAPageState(sdtWCB * aWCBPtr );
    inline static IDE_RC setWAPageState(sdtWASegment * aWASegment,
                                        scPageID       aWPID,
                                        sdtWAPageState aState );
    inline static void setWAPageState( sdtWCB         * aWCBPtr,
                                       sdtWAPageState   aState );
    inline static IDE_RC getSiblingWCBPtr( sdtWCB  * aWCBPtr,
                                           UInt      aWPID,
                                           sdtWCB ** aTargetWCBPtr );
    inline static void checkAndSetWAPageState( sdtWCB         * aWCBPtr,
                                               sdtWAPageState   aChkState,
                                               sdtWAPageState   aAftState,
                                               sdtWAPageState * aBefState);
    static idBool isFixedPage(sdtWASegment * aWASegment,
                              scPageID       aWPID );

    /******************************************************************
     * Page정보관련 Operation
     ******************************************************************/
    inline static IDE_RC getPagePtrByGRID(sdtWASegment  * aWASegment,
                                          sdtWAGroupID    aGrpID,
                                          scGRID          aGRID,
                                          UChar        ** aNSlotPtr,
                                          idBool        * aIsValidSlot );

    inline static IDE_RC getPageWithFix( sdtWASegment  * aWASegment,
                                         sdtWAGroupID    aGrpID,
                                         scGRID          aGRID,
                                         UInt          * aWPID,
                                         UChar        ** aWAPagePtr,
                                         UInt          * aSlotCount );
    inline static IDE_RC getPage(sdtWASegment  * aWASegment,
                                 sdtWAGroupID    aGrpID,
                                 scGRID          aGRID,
                                 UInt          * aWPID,
                                 UChar        ** aWAPagePtr,
                                 UInt          * aSlotCount );
    inline static void getSlot( UChar         * aWAPagePtr,
                                UInt            aSlotCount,
                                UInt            aSlotNo,
                                UChar        ** aNSlotPtr,
                                idBool        * aIsValid );


    inline static sdtWCB * getWCB(sdtWASegment  * aWASegment,
                                  scPageID        aWPID );

    inline static UChar  * getWAPagePtr( sdtWASegment  * aWASegment,
                                         sdtWAGroupID    aGroupID,
                                         scPageID        aWPID );
    inline static sdtWCB * getWCBInternal(sdtWASegment  * aWASegment,
                                          scPageID        aWPID );
    inline static sdtWCB* getWCBInExtent( UChar * aExtentPtr,
                                          UInt    aIdx );
    inline static UChar* getFrameInExtent( UChar * aExtentPtr,
                                           UInt    aIdx );
    static scPageID       getNPID( sdtWASegment * aWASegment,
                                   scPageID       aWPID );

    /*************************************************
     * NormalPage Operation
     *************************************************/
    static IDE_RC assignNPage(sdtWASegment * aWASegment,
                              scPageID       aWPID,
                              scSpaceID      aNSpaceID,
                              scPageID       aNPID);
    static IDE_RC unassignNPage(sdtWASegment * aWASegment,
                                scPageID       aWPID );
    static IDE_RC moveWPage( sdtWASegment * aWASegment,
                             sdtWAGroupID   aSrcGroupID,
                             scPageID       aSrcWPID,
                             sdtWAGroupID   aDstGroupID,
                             scPageID       aDstWPID );

    inline static UInt getNPageHashValue( sdtWASegment * aWASegment,
                                          scSpaceID      aNSpaceID,
                                          scPageID       aNPID);
    inline static sdtWCB * findWCB( sdtWASegment * aWASegment,
                                    scSpaceID      aNSpaceID,
                                    scPageID       aNPID);
    static IDE_RC readNPage(sdtWASegment * aWASegment,
                            sdtWAGroupID   aWAGroupID,
                            scPageID       aWPID,
                            scSpaceID      aNSpaceID,
                            scPageID       aNPID);
    static IDE_RC writeNPage(sdtWASegment * aWASegment,
                             scPageID       aWPID );
    static IDE_RC pushFreePage(sdtWASegment * aWASegment,
                               scPageID       aNPID);
    static IDE_RC allocAndAssignNPage(sdtWASegment * aWASegment,
                                      scPageID       aTargetPID );
    static IDE_RC allocFreeNExtent( sdtWASegment * aWASegment );
    static IDE_RC freeAllNPage( sdtWASegment * aWASegment );

    static UInt   getNExtentCount( sdtWASegment * aWASegment )
    {
        return aWASegment->mNExtentCount;
    }

    /***********************************************************
     * FullScan용 NPage 순회 함수
     ***********************************************************/
    static UInt getNPageCount( sdtWASegment * aWASegment );
    static IDE_RC getNPIDBySeq( sdtWASegment * aWASegment,
                                UInt           aIdx,
                                scPageID     * aPID );

    static idBool   isWorkAreaTbsID( scSpaceID aSpaceID )
    {
        return ( aSpaceID == SDT_SPACEID_WORKAREA ) ? ID_TRUE : ID_FALSE ;
    }
public:


    /******************************************************************
     * WAExtent
     * TempTableHeader가 배치된 후 그 바로 오른쪽에 WAExtent들의
     * Pointer 가 배치된다.
     ******************************************************************/
    inline static UChar * getWAExtentPtr( sdtWASegment  * aWASegment,
                                          UInt            aIdx );


    static IDE_RC addWAExtentPtr(sdtWASegment * aWASegment,
                                 UChar        * aWAExtentPtr );
    static IDE_RC setWAExtentPtr(sdtWASegment  * aWASegment,
                                 UInt            aIdx,
                                 UChar         * aWAExtentPtr );
public:
    static UInt getExtentIdx( scPageID  aWPID )
    {
        return aWPID / SDT_WAEXTENT_PAGECOUNT ;
    }
    static UInt getPageSeqInExtent( scPageID  aWPID )
    {
        return aWPID & SDT_WAEXTENT_PAGECOUNT_MASK ;
    }

private:
    static IDE_RC moveLRUListToTop( sdtWASegment * aWASegment,
                                    sdtWAGroupID   aWAGroupID,
                                    scPageID       aPID );
    static IDE_RC validateLRUList(sdtWASegment * aWASegment,
                                  sdtWAGroupID   aGroupID );
    static sdtWAGroupID findGroup( sdtWASegment * aWASegment,
                                   scPageID       aPID );


public:
    /***********************************************************
     * Dump용 함수들
     ***********************************************************/
    static void exportWASegmentToFile( sdtWASegment * aWASegment );
    static void importWASegmentFromFile( SChar         * aFileName,
                                         sdtWASegment ** aWASegment,
                                         UChar        ** aPtr,
                                         UChar        ** aAlignedPtr );
    static void dumpWASegment( void           * aWASegment,
                               SChar          * aOutBuf,
                               UInt             aOutSize );
    static void dumpWAGroup( sdtWASegment   * aWASegment,
                             sdtWAGroupID     aWAGroupID ,
                             SChar          * aOutBuf,
                             UInt             aOutSize );
    static void dumpFlushQueue( void           * aWAFQ,
                                SChar          * aOutBuf,
                                UInt             aOutSize );
    static void dumpWCBs( void           * aWASegment,
                          SChar          * aOutBuf,
                          UInt             aOutSize );
    static void dumpWCB( void           * aWCB,
                         SChar          * aOutBuf,
                         UInt             aOutSize );
    static void dumpWAPageHeaders( void           * aWASegment,
                                   SChar          * aOutBuf,
                                   UInt             aOutSize );
} ;


/**************************************************************************
 * Description :
 *      HintPage가져옴
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWAGroupID     - 대상 Group ID
 ***************************************************************************/
scPageID sdtWASegment::getHintWPID( sdtWASegment * aWASegment,
                                    sdtWAGroupID   aWAGroupID )
{
    return sdtWASegment::getWAGroupInfo( aWASegment,aWAGroupID )->mHintWPID;
}

/**************************************************************************
 * Description :
 *      HintPage를 설정함.
 *      HintPage는 unfix되면 안되기에 고정시켜둠
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWAGroupID     - 대상 Group ID
 ***************************************************************************/
IDE_RC sdtWASegment::setHintWPID( sdtWASegment * aWASegment,
                                  sdtWAGroupID   aWAGroupID,
                                  scPageID       aHintWPID )
{
    scPageID   * sHintWPIDPtr;

    sHintWPIDPtr =
        &sdtWASegment::getWAGroupInfo( aWASegment,aWAGroupID )->mHintWPID;
    if ( *sHintWPIDPtr != aHintWPID )
    {
        if ( *sHintWPIDPtr != SC_NULL_PID )
        {
            IDE_TEST( unfixPage( aWASegment, *sHintWPIDPtr ) != IDE_SUCCESS );
        }

        *sHintWPIDPtr = aHintWPID;

        if ( *sHintWPIDPtr != SC_NULL_PID )
        {
            IDE_TEST( fixPage( aWASegment, *sHintWPIDPtr ) != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    ideLog::log( IDE_DUMP_0,
                 "PrevHintWPID : %u\n"
                 "NextHintWPID : %u\n",
                 *sHintWPIDPtr,
                 aHintWPID );

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * GroupInfo의 포인터 가져온다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWAGroupID     - 대상 Group ID
 ***************************************************************************/
sdtWAGroup *sdtWASegment::getWAGroupInfo( sdtWASegment * aWASegment,
                                          sdtWAGroupID   aWAGroupID )
{
    return &aWASegment->mGroup[ aWAGroupID ];
}

/**************************************************************************
 * Description :
 *      InMemoryGroup인지 판단함.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWAGroupID     - 대상 Group ID
 ***************************************************************************/
idBool sdtWASegment::isInMemoryGroup( sdtWASegment * aWASegment,
                                      sdtWAGroupID   aWAGroupID )
{
    sdtWAGroup * sWAGrpInfo = sdtWASegment::getWAGroupInfo( aWASegment,
                                                            aWAGroupID );

    if ( sWAGrpInfo->mPolicy == SDT_WA_REUSE_INMEMORY )
    {
        return ID_TRUE;
    }

    return ID_FALSE;
}

/**************************************************************************
 * Description :
 *      WPID를 바탕으로 Group을 찾아 InMemoryGroup인지 판단함.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWPID          - WAPageID
 ***************************************************************************/
idBool sdtWASegment::isInMemoryGroupByPID( sdtWASegment * aWASegment,
                                           scPageID       aWPID )
{
    return isInMemoryGroup( aWASegment,
                            findGroup( aWASegment, aWPID ) );
}


/**************************************************************************
 * Description :
 *      나중에 unassign될때 Free되도록, 표시해둔다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWPID          - WAPageID
 ***************************************************************************/
void sdtWASegment::bookedFree(sdtWASegment * aWASegment,
                              scPageID       aWPID)
{
    sdtWCB   * sWCBPtr;

    sWCBPtr = getWCB( aWASegment, aWPID );
    if ( sWCBPtr->mNPageID == SC_NULL_PID )
    {
        /* 할당 되어있지 않으면, 무시해도 된다. */
    }
    else
    {
        sWCBPtr->mBookedFree = ID_TRUE;
    }
}

/**************************************************************************
 * Description :
 *      해당 WAPage가 discard되지 않도록 고정한다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWPID          - WAPageID
 ***************************************************************************/
IDE_RC sdtWASegment::fixPage(sdtWASegment * aWASegment,
                             scPageID       aWPID)
{
    sdtWCB   * sWCBPtr;

    sWCBPtr = getWCB( aWASegment, aWPID );

    sWCBPtr->mFix ++;
    IDE_ERROR( sWCBPtr->mFix > 0 );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 *      FixPage를 풀어 Discard를 허용한다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWPID          - WAPageID
 ***************************************************************************/
IDE_RC sdtWASegment::unfixPage(sdtWASegment * aWASegment,
                               scPageID       aWPID)
{
    sdtWCB   * sWCBPtr;

    sWCBPtr = getWCB( aWASegment, aWPID );

    sWCBPtr->mFix --;
    IDE_ERROR( sWCBPtr->mFix >= 0 );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 *     Free, 즉 Write할 필요 없이 재활용 가능한 페이지인지 확인한다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWPID          - WAPageID
 ***************************************************************************/
idBool sdtWASegment::isFreeWAPage(sdtWASegment * aWASegment,
                                  scPageID       aWPID)
{
    sdtWAPageState   sState = getWAPageState( aWASegment, aWPID );

    if ( ( sState == SDT_WA_PAGESTATE_NONE ) ||
         ( sState == SDT_WA_PAGESTATE_INIT ) ||
         ( sState == SDT_WA_PAGESTATE_CLEAN ) )
    {
        return ID_TRUE;
    }
    else
    {
        /* nothing to do */
    }
    return ID_FALSE;
}

/**************************************************************************
 * Description :
 *     Page의 상태를 알아낸다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWPID          - WAPageID
 ***************************************************************************/
sdtWAPageState sdtWASegment::getWAPageState(sdtWASegment   * aWASegment,
                                            scPageID         aWPID )
{
    return getWAPageState( getWCB( aWASegment, aWPID ) );
}

/**************************************************************************
 * Description :
 *     Page의 상태를 알아낸다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWPID          - WAPageID
 * <OUT>
 * aState         - Page의 상태
 ***************************************************************************/
sdtWAPageState sdtWASegment::getWAPageState( sdtWCB         * aWCBPtr )
{
    if ( ( aWCBPtr->mWPState != SDT_WA_PAGESTATE_IN_FLUSHQUEUE ) &&
         ( aWCBPtr->mWPState != SDT_WA_PAGESTATE_WRITING ) )
    {
        /* 동시성 이슈가 없는 상태는 그냥 가져온다.
         *
         * 왜냐하면, 동시성 이슈가 있도록 변경하는 것은 ServiceThread의
         * writeNPage다. 따라서 ServiceThread자신은 이 getWAPageState를
         * 불러도 문제 없다.
         *
         * Flusher는 checkAndSetWAPageState 로 상태를 확인하기에 문제 없다.*/
        return aWCBPtr->mWPState;
    }
    else
    {
        return (sdtWAPageState)idCore::acpAtomicGet32( &aWCBPtr->mWPState );
    }
}

/**************************************************************************
 * Description :
 *     Page의 상태를 설정한다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWPID          - WAPageID
 * aState         - 설정할 Page의 상태
 ***************************************************************************/
IDE_RC sdtWASegment::setWAPageState(sdtWASegment * aWASegment,
                                    scPageID       aWPID,
                                    sdtWAPageState aState )
{
    setWAPageState( getWCB( aWASegment, aWPID ), aState );

    return IDE_SUCCESS;
}

/**************************************************************************
 * Description :
 *     Page의 상태를 설정한다.
 *
 * <IN>
 * aWCBPtr        - 대상 WCB
 * aState         - 설정할 Page의 상태
 ***************************************************************************/
void sdtWASegment::setWAPageState( sdtWCB         * aWCBPtr,
                                   sdtWAPageState   aState )
{
    if ( ( aWCBPtr->mWPState != SDT_WA_PAGESTATE_IN_FLUSHQUEUE ) &&
         ( aWCBPtr->mWPState != SDT_WA_PAGESTATE_WRITING ) )
    {
        /* 동시성 이슈가 없는 상태는 그냥 가져온다.
         *
         * 왜냐하면, 동시성 이슈가 있도록 변경하는 것은 ServiceThread의
         * writeNPage다. 따라서 ServiceThread자신은 이 getWAPageState를
         * 불러도 문제 없다.
         *
         * Flusher는 checkAndSetWAPageState 로 상태를 확인하기에 문제 없다.*/
        aWCBPtr->mWPState = aState;
    }
    else
    {
        (void)idCore::acpAtomicSet32( &aWCBPtr->mWPState, aState );
    }
}

/**************************************************************************
 * Description :
 *     해당 WCB의 이웃 WCB를 얻어온다.
 *
 * <IN>
 * aWCBPtr        - 대상 WCB
 * aWPID          - 이웃 페이지와의 페이지 간격
 * <OUT>
 * aWCBPtr        - 얻은 WCB
 ***************************************************************************/
IDE_RC sdtWASegment::getSiblingWCBPtr( sdtWCB  * aWCBPtr,
                                       UInt      aWPID,
                                       sdtWCB ** aTargetWCBPtr )
{
    ( *aTargetWCBPtr ) = aWCBPtr - aWPID;

    IDE_ERROR( (*aTargetWCBPtr)->mWPageID == aWCBPtr->mWPageID - aWPID );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 *     Page의 상태를 확인하고, 자신이 예상한 상태가 맞으면 변경한다.
 *
 * <IN>
 * aWCBPtr        - 대상 WCB
 * aChkState      - 예상한 페이지의 상태
 * aAftState      - 예상이 맞을 경우 설정할 페이지의 상태
 * <OUT>
 * aBefState      - 변경하기 전 페이지의 진짜 상태
 ***************************************************************************/
void sdtWASegment::checkAndSetWAPageState( sdtWCB         * aWCBPtr,
                                           sdtWAPageState   aChkState,
                                           sdtWAPageState   aAftState,
                                           sdtWAPageState * aBefState)
{
    *aBefState = (sdtWAPageState)idCore::acpAtomicCas32( &aWCBPtr->mWPState,
                                                         aAftState,
                                                         aChkState );
}

/**************************************************************************
 * Description :
 *     Page 주소로 HashValue생성
 *
 * <IN>
 * aNSpaceID,aNPID- Page주소
 ***************************************************************************/
UInt sdtWASegment::getNPageHashValue( sdtWASegment * aWASegment,
                                      scSpaceID      aNSpaceID,
                                      scPageID       aNPID )
{
    return ( aNSpaceID + aNPID ) % aWASegment->mNPageHashBucketCnt;
}

/**************************************************************************
 * Description :
 *     NPageHash를 뒤져서 해당 NPage를 가진 WPage를 찾아냄
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aNSpaceID,aNPID- Page주소
 ***************************************************************************/
sdtWCB * sdtWASegment::findWCB( sdtWASegment * aWASegment,
                                scSpaceID      aNSpaceID,
                                scPageID       aNPID )
{
    UInt       sHashValue = getNPageHashValue( aWASegment, aNSpaceID, aNPID );
    sdtWCB   * sWCBPtr;

    sWCBPtr = aWASegment->mNPageHashPtr[ sHashValue ];

    while ( sWCBPtr != NULL )
    {
        /* TableSpaceID는 같아야 하며, HashValue도 맞아야 함 */
        IDE_DASSERT( sWCBPtr->mNSpaceID == aWASegment->mSpaceID );
        IDE_DASSERT( getNPageHashValue( aWASegment, sWCBPtr->mNSpaceID, sWCBPtr->mNPageID )
                     == sHashValue );

        if ( sWCBPtr->mNPageID == aNPID )
        {
            IDE_DASSERT( sWCBPtr->mNSpaceID == aNSpaceID );
            break;
        }
        else
        {
            /* nothing to do */
        }

        sWCBPtr  = sWCBPtr->mNextWCB4Hash;
    }

    return sWCBPtr;
}

/**************************************************************************
 * Description :
 *     WA에서의 GRID를 NGRID로 변경함
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWGRID         - 변경할 원본 WGRID
 * <OUT>
 * aNGRID         - 얻은 값
 ***************************************************************************/
void sdtWASegment::convertFromWGRIDToNGRID( sdtWASegment * aWASegment,
                                            scGRID         aWGRID,
                                            scGRID       * aNGRID )
{
    sdtWCB         * sWCBPtr;
    sdtWAPageState   sWAPageState;

    if ( SC_MAKE_SPACE( aWGRID ) == SDT_SPACEID_WORKAREA )
    {
        /* WGRID여야 하고, InMemoryGroup아니어야 함 */
        sWCBPtr = getWCB( aWASegment,SC_MAKE_PID( aWGRID ) );
        sWAPageState = getWAPageState( sWCBPtr );

        /* NPID와 Assign 되어야만 의미 있음 */
        if ( ( sWAPageState != SDT_WA_PAGESTATE_NONE ) &&
             ( sWAPageState != SDT_WA_PAGESTATE_INIT ) )
        {
            SC_MAKE_GRID( *aNGRID,
                          sWCBPtr->mNSpaceID,
                          sWCBPtr->mNPageID,
                          SC_MAKE_OFFSET( aWGRID ) );
        }
        else
        {
            /* nothing to do */
        }
    }
    else
    {
        /* nothing to do */
    }
}

/**************************************************************************
 * Description :
 *     WASegment내에서 해당 NPage를 찾는다.
 *     페이지가 없으면, Group에서 VictimPage를 찾아서 읽어서 돌려준다.
 *     그렇게 찾아서 WCB를 반환한다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWAGroupID     - 대상 Group ID
 * aNSpaceID,aNPID- Page주소
 * <OUT>
 * aRetWCB        - 얻은 값
 ***************************************************************************/
IDE_RC sdtWASegment::getWCBByNPID(sdtWASegment * aWASegment,
                                  sdtWAGroupID   aWAGroupID,
                                  scSpaceID      aNSpaceID,
                                  scPageID       aNPID,
                                  sdtWCB      ** aRetWCB )
{
    scPageID   sWPID=SD_NULL_PID;

    if ( ( aWASegment->mHintWCBPtr->mNPageID == aNPID ) )
    {
        IDE_DASSERT( aWASegment->mHintWCBPtr->mNSpaceID == aNSpaceID );

        (*aRetWCB) = aWASegment->mHintWCBPtr;
    }
    else
    {
        (*aRetWCB) = findWCB( aWASegment, aNSpaceID, aNPID );

        if ( (*aRetWCB) == NULL )
        {
            // WAGroupID NONE으로 Free Page를 할당 할 수 없음
            IDE_ERROR( aWAGroupID != SDT_WAGROUPID_NONE );

            /* WA상에 페이지가 존재하지 않을 경우, 빈 페이지를 Group에 할당받아
             * Read함*/
            IDE_TEST( getFreeWAPage( aWASegment,
                                     aWAGroupID,
                                     &sWPID )
                      != IDE_SUCCESS );
            IDE_ERROR( sWPID != SD_NULL_PID );

            IDE_TEST( readNPage( aWASegment,
                                 aWAGroupID,
                                 sWPID,
                                 aNSpaceID,
                                 aNPID )
                      != IDE_SUCCESS );
            (*aRetWCB) = getWCB( aWASegment, sWPID ) ;
        }
        else
        {
            /* nothing to do */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    ideLog::log( IDE_DUMP_0,
                 "aWAGroupID : %u\n"
                 "aNSpaceID  : %u\n"
                 "aNPID      : %u\n"
                 "sWPID      : %u\n",
                 aWAGroupID,
                 aNSpaceID,
                 aNPID,
                 sWPID );

    IDE_DASSERT( 0 );

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * GRID를 바탕으로 WPID를 가져온다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWAGroupID     - 대상 Group ID
 * aGRID          - 가져올 원본 GRID
 * <OUT>
 * aRetPID        - 결과 WPID
 ***************************************************************************/
IDE_RC sdtWASegment::getWPIDFromGRID(sdtWASegment * aWASegment,
                                     sdtWAGroupID   aWAGroupID,
                                     scGRID         aGRID,
                                     scPageID     * aRetPID )
{
    sdtWCB   * sWCBPtr;

    if ( SC_MAKE_SPACE( aGRID ) == SDT_SPACEID_WORKAREA )
    {
        (*aRetPID) = SC_MAKE_PID( aGRID );
    }
    else
    {
        IDE_TEST( sdtWASegment::getWCBByNPID( aWASegment,
                                              aWAGroupID,
                                              SC_MAKE_SPACE( aGRID ),
                                              SC_MAKE_PID( aGRID ),
                                              &sWCBPtr )
                  != IDE_SUCCESS );
        (*aRetPID) = sWCBPtr->mWPageID;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 *     DirtyPage로 설정한다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWPID          - 대상 Page
 ***************************************************************************/
IDE_RC sdtWASegment::setDirtyWAPage( sdtWASegment * aWASegment,
                                     scPageID       aWPID )
{
    sdtWCB         * sWCBPtr;
    sdtWAPageState   sWAPageState;

    sWCBPtr      = getWCB( aWASegment, aWPID );
    sWAPageState = getWAPageState( sWCBPtr );

    if ( ( sWAPageState != SDT_WA_PAGESTATE_NONE ) &&
         ( sWAPageState != SDT_WA_PAGESTATE_INIT ) )
    {
        if ( sWAPageState == SDT_WA_PAGESTATE_CLEAN )
        {
            IDE_TEST( setWAPageState( aWASegment,
                                      aWPID,
                                      SDT_WA_PAGESTATE_DIRTY )
                      != IDE_SUCCESS );

        }
        else
        {
            /* Redirty */
            checkAndSetWAPageState( sWCBPtr,
                                    SDT_WA_PAGESTATE_WRITING,
                                    SDT_WA_PAGESTATE_DIRTY,
                                    &sWAPageState );
        }
    }
    else
    {
        IDE_DASSERT( isInMemoryGroup( aWASegment,
                                      findGroup( aWASegment, aWPID ) )
                     == ID_TRUE );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 *     WGRID를 바탕으로 해당 Slot의 포인터를 반환한다.
 *
 * <IN>
 * aWAPagePtr     - 대상 WAPage의 포인터
 * aGroupID       - 대상 GroupID
 * aGRID          - 대상 GRID 값
 * <OUT>
 * aNSlotPtr      - 얻은 슬롯
 * aIsValid       - Slot번호가 올바른가? ( Page가 비었던가, overflow라던가 )
 ***************************************************************************/
IDE_RC sdtWASegment::getPagePtrByGRID( sdtWASegment  * aWASegment,
                                       sdtWAGroupID    aGroupID,
                                       scGRID          aGRID,
                                       UChar        ** aNSlotPtr,
                                       idBool        * aIsValid )

{
    UChar         * sWAPagePtr = NULL;
    UInt            sSlotCount = 0;
    scPageID        sWPID      = SC_NULL_PID;

    IDE_TEST( getPage( aWASegment,
                       aGroupID,
                       aGRID,
                       &sWPID,
                       &sWAPagePtr,
                       &sSlotCount )
              != IDE_SUCCESS );
    getSlot( sWAPagePtr,
             sSlotCount,
             SC_MAKE_OFFSET( aGRID ),
             aNSlotPtr,
             aIsValid );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 *     getPage와 동일하되, 해당 Page를 Fix시키기도 한다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aGroupID       - 대상 GroupID
 * aGRID          - 대상 GRID 값
 * <OUT>
 * aWPID          - 얻은 WPID
 * aWAPagePtr     - 얻은 WAPage의 위치
 * aSlotCount     - 해당 page의 SlotCount
 ***************************************************************************/
IDE_RC sdtWASegment::getPageWithFix( sdtWASegment  * aWASegment,
                                     sdtWAGroupID    aGroupID,
                                     scGRID          aGRID,
                                     UInt          * aWPID,
                                     UChar        ** aWAPagePtr,
                                     UInt          * aSlotCount )
{
    if ( *aWPID != SC_NULL_PID )
    {
        IDE_DASSERT( *aWAPagePtr != NULL );
        IDE_TEST( unfixPage( aWASegment, *aWPID ) != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    IDE_TEST( getPage( aWASegment,
                       aGroupID,
                       aGRID,
                       aWPID,
                       aWAPagePtr,
                       aSlotCount )
              != IDE_SUCCESS );

    IDE_TEST( fixPage( aWASegment, *aWPID ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 *     GRID를 바탕으로 Page에 대한 정보를 얻는다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aGroupID       - 대상 GroupID
 * aGRID          - 대상 GRID 값
 * <OUT>
 * aWPID          - 얻은 WPID
 * aWAPagePtr     - 얻은 WAPage의 위치
 * aSlotCount     - 해당 page의 SlotCount
 ***************************************************************************/
IDE_RC sdtWASegment::getPage( sdtWASegment  * aWASegment,
                              sdtWAGroupID    aGroupID,
                              scGRID          aGRID,
                              UInt          * aWPID,
                              UChar        ** aWAPagePtr,
                              UInt          * aSlotCount )
{
    sdtWCB    * sWCBPtr;
    scSpaceID   sSpaceID;
    scPageID    sPageID;

    IDE_DASSERT( SC_GRID_IS_NULL( aGRID ) == ID_FALSE );

    sSpaceID = SC_MAKE_SPACE( aGRID );
    sPageID  = SC_MAKE_PID( aGRID );

    if ( isWorkAreaTbsID( sSpaceID ) == ID_TRUE )
    {
        (*aWPID)      = sPageID;
        (*aWAPagePtr) = getWAPagePtr( aWASegment, aGroupID, *aWPID );
    }
    else
    {
        IDE_TEST_RAISE( sctTableSpaceMgr::isTempTableSpace( sSpaceID ) != ID_TRUE,
                        tablespaceID_error );

        IDE_TEST( getWCBByNPID( aWASegment,
                                aGroupID,
                                sSpaceID,
                                sPageID,
                                &sWCBPtr )
                  != IDE_SUCCESS );
        (*aWPID)      = sWCBPtr->mWPageID;
        (*aWAPagePtr) = sWCBPtr->mWAPagePtr;
    }

    IDE_TEST( sdtTempPage::getSlotCount( *aWAPagePtr,
                                         aSlotCount )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( tablespaceID_error );
    {
        ideLog::log( IDE_SM_0,
                      "[FAILURE] Fatal error during fetch disk temp table"
                      "(Tablespace ID : %"ID_UINT32_FMT", Page ID : %"ID_UINT32_FMT")",
                      sSpaceID,
                      sPageID );
        IDE_SET( ideSetErrorCode( smERR_ABORT_INTERNAL ) );
    } 
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
/**************************************************************************
 * Description :
 *     getPage한 정보를 바탕으로  Slot도 얻는다.
 *
 * <IN>
 * aWAPagePtr     - 대상 WAPage의 포인터
 * aSlotCount     - 대상 Page의 Slot 개수
 * aSlotNo        - 얻을 슬롯 번호
 * <OUT>
 * aNSlotPtr      - 얻은 슬롯
 * aIsValid       - Slot번호가 올바른가? ( Page가 비었던가, overflow라던가 )
 ***************************************************************************/
void sdtWASegment::getSlot( UChar         * aWAPagePtr,
                            UInt            aSlotCount,
                            UInt            aSlotNo,
                            UChar        ** aNSlotPtr,
                            idBool        * aIsValid )
{
    if ( aSlotCount > aSlotNo )
    {
        *aNSlotPtr = aWAPagePtr +
            sdtTempPage::getSlotOffset( aWAPagePtr, (scSlotNum)aSlotNo );
        *aIsValid = ID_TRUE;
    }
    else
    {
        /* Slot을 넘어간 경우.*/
        *aNSlotPtr    = aWAPagePtr;
        *aIsValid     = ID_FALSE;
    }
}

/**************************************************************************
 * Description :
 *     WPID를 바탕으로 WCB의 위치를 반환한다.
 *
 * <Warrning>
 * Flusher에서는 반드시 getWCB, getWAPagePtr등의 Hint를 사용하는 함수를
 * 사용하면 안된다.
 * 해당 함수들은 ServiceThread 혼자 접근한다는 전제가 있기 때문에,
 * Flusher는 사용하면 안된다. getWCBInternal등으로 처리해야 한다.
 * <Warrning>
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWPID          - 대상 Page
 ***************************************************************************/
sdtWCB * sdtWASegment::getWCB(sdtWASegment  * aWASegment,
                              scPageID        aWPID )
{
    if ( aWASegment->mHintWCBPtr->mWPageID != aWPID )
    {
        aWASegment->mHintWCBPtr = getWCBInternal( aWASegment, aWPID );
    }
    else
    {
        /* nothing to do */
    }

    return aWASegment->mHintWCBPtr;
}

/**************************************************************************
 * Description :
 *     WPID를 바탕으로 WAPage의 포인터를 반환한다.
 *
 * <Warrning>
 * Flusher에서는 반드시 getWCB, getWAPagePtr등의 Hint를 사용하는 함수를
 * 사용하면 안된다.
 * 해당 함수들은 ServiceThread 혼자 접근한다는 전제가 있기 때문에,
 * Flusher는 사용하면 안된다. getWCBInternal등으로 처리해야 한다.
 * <Warrning>
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aGroupID       - 대상 GroupID
 * aWPID          - 대상 Page
 ***************************************************************************/
UChar * sdtWASegment::getWAPagePtr(sdtWASegment  * aWASegment,
                                   sdtWAGroupID    aGroupID,
                                   scPageID        aWPID )
{
    if ( aWASegment->mHintWCBPtr->mWPageID != aWPID )
    {
        IDE_ASSERT( moveLRUListToTop( aWASegment,
                                      aGroupID,
                                      aWPID )
                    == IDE_SUCCESS );

        aWASegment->mHintWCBPtr = getWCBInternal( aWASegment, aWPID );
    }
    else
    {
        /* nothing to do */
    }

    return aWASegment->mHintWCBPtr->mWAPagePtr;
}

/**************************************************************************
 * Description :
 *     WPID를 바탕으로 WCB의 위치를 반환한다.
 *     단, Hint를 이용하지 않는다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWPID          - 대상 Page
 ***************************************************************************/
sdtWCB * sdtWASegment::getWCBInternal( sdtWASegment  * aWASegment,
                                       scPageID        aWPID )
{
    return  getWCBInExtent( getWAExtentPtr( aWASegment,
                                            getExtentIdx(aWPID) ),
                            getPageSeqInExtent(aWPID) );
}

/**************************************************************************
 * Description :
 *     Extent내에서 WPage의 위치를 반환함.
 *
 * <IN>
 * aExtentPtr     - Extent의 위치
 * aIdx           - Extent내 Frame의 번호
 ***************************************************************************/
UChar* sdtWASegment::getFrameInExtent( UChar * aExtentPtr,
                                       UInt    aIdx )
{
    IDE_DASSERT( aIdx <= SDT_WAEXTENT_PAGECOUNT );

    return aExtentPtr + (aIdx * SD_PAGE_SIZE);
}

/**************************************************************************
 * Description :
 *     Extent내에서 WCB의 위치를 반환함.
 *
 * <IN>
 * aExtentPtr     - Extent의 위치
 * aIdx           - Extent내 WCB의 번호
 ***************************************************************************/
sdtWCB* sdtWASegment::getWCBInExtent( UChar * aExtentPtr,
                                      UInt    aIdx )
{
    /* Extent내 마지막 Frame 뒤에부터 WCB가 배치됨 */
    sdtWCB * sWCBStartPtr =
        (sdtWCB*)getFrameInExtent( aExtentPtr, SDT_WAEXTENT_PAGECOUNT );

    IDE_DASSERT( aIdx < SDT_WAEXTENT_PAGECOUNT );

    return &sWCBStartPtr[ aIdx ];
}

/**************************************************************************
 * Description :
 *     WAExtent의 위치를 반환한다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aIdx           - Extent의 번호
 * <OUT>
 * aWAExtenntPtr  - 얻은 Extent의 포인터 위치
 ***************************************************************************/
UChar * sdtWASegment::getWAExtentPtr(sdtWASegment  * aWASegment,
                                     UInt            aIdx )
{
    UInt     sOffset;
    UChar ** sSlotPtr;

    IDE_DASSERT( aIdx < aWASegment->mWAExtentCount );

    sOffset = ID_SIZEOF( sdtWASegment ) + (ID_SIZEOF( UChar* ) * aIdx);

    /* 0번 Extent에 들어야 한다. */
    IDE_DASSERT( getExtentIdx(sOffset / SD_PAGE_SIZE) == 0 );
    IDE_DASSERT( ( sOffset % ID_SIZEOF( UChar *) )  == 0 );

    /* WASegment는 반드시 WAExtent의 가장 앞쪽에 배치되고,
     * 0번 Extent내에 모두 배치되어 연속성을 가지기 때문에
     * 탐색이 가능하다 */
    sSlotPtr = (UChar**)(((UChar*)aWASegment) + sOffset);

    IDE_DASSERT( *sSlotPtr != NULL );
    IDE_DASSERT( ( (vULong)*sSlotPtr % SDT_WAEXTENT_SIZE ) == 0 );

    return *sSlotPtr;
}

/**************************************************************************
 * Description : Temp table 관련 통계정보를 갱신한다.
 *
 * <IN>
 * aHeader     - 대상 Temp Table Header
 ***************************************************************************/
void sdtWASegment::updateTempStats( smiTempTableHeader * aHeader )
{
    sdtWASegment      * sWASegment = (sdtWASegment*)aHeader->mWASegment;
    smiTempTableStats * sStats     = aHeader->mStatsPtr;
    sdtWAFlushQueue   * sWAFQ      = sWASegment->mFlushQueue;

    sStats->mWorkAreaSize   = getWASegmentPageCount( sWASegment ) * SD_PAGE_SIZE;
    sStats->mNormalAreaSize = getNExtentCount( sWASegment )
                              * SDT_WAEXTENT_PAGECOUNT 
                              * SD_PAGE_SIZE;

    /* clear 등으로 하나의 통계가 여러 WASegment, Flusher를 거칠 수도 있다.
     * 대입이 아닌 누적으로 갱신해야 한다. */
    sStats->mWriteCount += sWAFQ->mWriteCount;
    sWAFQ->mWriteCount   = 0;

    sStats->mWritePageCount += sWAFQ->mWritePageCount;
    sWAFQ->mWritePageCount   = 0;

    sStats->mRedirtyCount += sWAFQ->mRedirtyCount;
    sWAFQ->mRedirtyCount   = 0;

    return;
}

#endif //_O_SDT_WA_SEGMENT_H_
