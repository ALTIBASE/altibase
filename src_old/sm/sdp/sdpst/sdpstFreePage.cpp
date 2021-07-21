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
 *
 * $Id: sdpstFreePage.cpp 27229 2008-07-23 17:37:19Z newdaily $
 *
 * 본 파일은 Treelist Managed Segment에서 가용공간 해제 연산 관련 STATIC
 * 인터페이스를 관리한다.
 *
 ***********************************************************************/

# include <sdpstDef.h>

# include <sdpstRtBMP.h>
# include <sdpstLfBMP.h>
# include <sdpstCache.h>
# include <sdpstAllocPage.h>
# include <sdpstFreePage.h>
# include <sdpstSH.h>

# include <sdpstStackMgr.h>


/***********************************************************************
 * Description : [ INTERFACE ] Segment에 페이지를 해제한다
 ***********************************************************************/
IDE_RC sdpstFreePage::freePage( idvSQL       * aStatistics,
                                sdrMtx       * aMtx,
                                scSpaceID      aSpaceID,
                                sdpSegHandle * aSegHandle,
                                UChar        * aPagePtr )
{
    UChar            * sPagePtr;
    sdpstSegHdr      * sSegHdr;
    idBool             sIsTurnON;
    sdpstStack         sItHintStack;
    sdpstStack         sRevStack;
    sdpstSegCache    * sSegCache;
    scPageID           sPageID;
    sdpPageType        sPageType;
    sdpParentInfo      sParentInfo;
    sdpstPBS           sPBS;
    ULong              sSeqNo;

    IDE_ASSERT( aSegHandle != NULL );
    IDE_ASSERT( aPagePtr   != NULL );
    IDE_ASSERT( aMtx       != NULL );

    sSegCache   = (sdpstSegCache*)aSegHandle->mCache;

    sPageID     = sdpPhyPage::getPageID( aPagePtr );
    sSeqNo      = sdpPhyPage::getSeqNo( sdpPhyPage::getHdr(aPagePtr) );
    sPageType   = sdpPhyPage::getPageType( sdpPhyPage::getHdr(aPagePtr) );
    sParentInfo = sdpPhyPage::getParentInfo( aPagePtr );
    sPBS        = (sdpstPBS)
                  ( SDPST_BITSET_PAGETP_DATA | SDPST_BITSET_PAGEFN_FMT );

    /*
     * Page를 Free 하게 되면 완전히 빈 format 페이지가 된다.
     * 따라서 여기서 초기화를 해준다.
     */
    IDE_TEST( sdpstAllocPage::formatPageHdr( aMtx,
                                             aSegHandle,
                                             sdpPhyPage::getHdr( aPagePtr ),
                                             sPageID,
                                             sSeqNo,
                                             sPageType,
                                             sParentInfo.mParentPID,
                                             sParentInfo.mIdxInParent,
                                             sPBS ) != IDE_SUCCESS );

    /* reverse stack은 it->bottom까지의 stack만 수집한다. */
    sdpstStackMgr::initialize( &sRevStack );
    IDE_TEST( sdpstAllocPage::tryToChangeMFNLAndItHint(
                                 aStatistics,
                                 aMtx,
                                 aSpaceID,
                                 aSegHandle,
                                 SDPST_CHANGEMFNL_LFBMP_PHASE,
                                 sParentInfo.mParentPID,
                                 sParentInfo.mIdxInParent,
                                 (void*)&sPBS,
                                 SD_NULL_PID,
                                 SDPST_INVALID_SLOTNO,
                                 &sRevStack ) != IDE_SUCCESS );

    // Format Index Page Count를 증가시킨다.
    if ( sSegCache->mSegType == SDP_SEG_TYPE_INDEX )
    {
        // prepareNewPages 지원을 위해서 index segment 에 대해서만 특별히
        // format 페이지 개수를 관리한다.
        IDE_ASSERT( sdpstLfBMP::isEqFN( sPBS, 
                                        SDPST_BITSET_PAGEFN_FMT )
                    == ID_TRUE );

        // internal slot의 MFNL이 변경되어서 Internal Hint를 변경해야 하거나,
        // Index Segment인 경우에는 Segment Header를 fix한다.
        IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                              aSpaceID,
                                              aSegHandle->mSegPID,            
                                              SDB_X_LATCH,
                                              SDB_WAIT_NORMAL,
                                              SDB_SINGLE_PAGE_READ,
                                              aMtx,
                                              &sPagePtr,
                                              NULL, /* aTrySuccess */
                                              NULL  /* aIsCorruptPage */ )
                  != IDE_SUCCESS );

        sSegHdr = sdpstSH::getHdrPtr( sPagePtr );

        IDE_TEST( sdpstSH::setFreeIndexPageCnt( aMtx,
                                                sSegHdr,
                                                sSegHdr->mFreeIndexPageCnt + 1 )
                  != IDE_SUCCESS );
    }

    if ( (sdpstCache::needToUpdateItHint(sSegCache, SDPST_SEARCH_NEWPAGE) 
          == ID_FALSE) &&
         (sdpstStackMgr::getDepth( &sRevStack ) 
          == SDPST_RTBMP) )
    {
        /* 작은 것을 선택하는 이유는 free page가 발생하면, 
         * slot hint와 page hint에
         * 모두 영향을 미치기 때문이다 */
        sItHintStack = sdpstCache::getMinimumItHint( aStatistics, sSegCache );

        IDE_TEST( checkAndUpdateHintItBMP( aStatistics,
                                           aSpaceID,
                                           aSegHandle->mSegPID,
                                           &sRevStack,
                                           &sItHintStack,
                                           &sIsTurnON ) != IDE_SUCCESS );

        if ( sIsTurnON == ID_TRUE )
        {
            sdpstCache::setUpdateHint4Page( sSegCache, ID_TRUE );
            sdpstCache::setUpdateHint4Slot( sSegCache, ID_TRUE );
        }
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 해당 페이지의 Free 페이지 여부를 반환한다.
 * 
 * BUG-32942 When executing rebuild Index stat, abnormally shutdown
 ***********************************************************************/
idBool sdpstFreePage::isFreePage( UChar * aPagePtr )
{
    idBool      sRet = ID_FALSE;
    sdpstPBS    sPBS;

    sPBS = (sdpstPBS)sdpPhyPage::getState((sdpPhyPageHdr*)aPagePtr);

    if ( sPBS == ((sdpstPBS)(SDPST_BITSET_PAGETP_DATA | SDPST_BITSET_PAGEFN_FMT)) )
    {
        sRet = ID_TRUE;
    }

    return sRet;
}

/***********************************************************************
 * Description : 빈 페이지 탐색을 위한 Hint의 재설정이 필요한지를 판단하고
 *               Update Hint Flag를 재설정한다.
 ***********************************************************************/
IDE_RC sdpstFreePage::checkAndUpdateHintItBMP( idvSQL         * aStatistics,
                                               scSpaceID        aSpaceID,
                                               scPageID         aSegPID,
                                               sdpstStack     * aRevStack,
                                               sdpstStack     * aItHintStack,
                                               idBool         * aIsTurnOn )
{
    idBool             sIsTurnOn;
    sdpstPosItem       sRtPos1;
    sdpstPosItem       sRtPos2;
    SShort             sDist;
    sdpstStack         sStack;

    IDE_ASSERT( aSegPID != SD_NULL_PID );
    IDE_ASSERT( aRevStack != NULL );
    IDE_ASSERT( aItHintStack != NULL );
    IDE_ASSERT( aIsTurnOn    != NULL );

    sIsTurnOn = ID_FALSE;

    /* root bmp까지 MFNL이 변경된 경우에만 Stack을 수집하여 Segment Cache의
     * it hint stack과 비교하여 Update Hint Flag를 on 시킨다.
     * stack은 특별하게 it->root 방향으로 push가 되어 있어 reverse stack
     * 이므로 다음과 같이 거꾸로 체크한다. */
    if ( sdpstStackMgr::getDepth( aRevStack ) == SDPST_RTBMP )
    {
        /* 같은 root bmp가 아닌경우는 실제로 stack의 선후관계를 
         * 정확히 판단해본다
         * 만약 It Hint가 더 크다면, It Hint를 땡겨야하고, 
         * 그렇지 않다면, 그냥 둔다. */
        sRtPos1 = sdpstStackMgr::getSeekPos( aRevStack, SDPST_RTBMP);
        sRtPos2 = sdpstStackMgr::getSeekPos( aItHintStack, SDPST_RTBMP);

        sDist = sdpstStackMgr::getDist( &sRtPos1, &sRtPos2 );

        if ( sDist == SDPST_FAR_AWAY_OFF )
        {
            sdpstStackMgr::initialize( &sStack );
            // 정확한 선후관계를 판단할 수 없기때문에 RevStack을 정렬하고,
            // 비교한다.
            IDE_TEST( makeOrderedStackFromRevStack( aStatistics,
                                          aSpaceID,
                                          aSegPID,
                                          aRevStack,
                                          &sStack ) != IDE_SUCCESS );

            if ( sdpstStackMgr::compareStackPos( aItHintStack, &sStack ) > 0 )
            {
                sIsTurnOn = ID_TRUE;
            }
            else
            {
                sIsTurnOn = ID_FALSE;
            }
        }
        else
        {
            // 동일한 root에서 선후관계를 파악한다.
            if ( sDist < 0 )
            {
                sIsTurnOn = ID_TRUE;
            }
            else
            {
                sIsTurnOn = ID_FALSE;
            }
        }
    }
    else
    {
        // root까지 mfnl이 갱신되지 않았으므로, rescan 여부와 상관이 없다.
    }

    *aIsTurnOn = sIsTurnOn;

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 역 stack으로 순서에 맞는 stack을 만들어낸다.
 ***********************************************************************/
IDE_RC sdpstFreePage::makeOrderedStackFromRevStack( idvSQL      * aStatistics,
                                          scSpaceID     aSpaceID,
                                          scPageID      aSegPID,
                                          sdpstStack  * aRevStack,
                                          sdpstStack  * aOrderedStack )
{
    idBool           sDummy;
    UInt             sLoop;
    sdpstPosItem     sCurrPos;
    scPageID         sCurrRtBMP;
    UChar          * sPagePtr;
    SShort           sRtBMPIdx;
    sdpstBMPHdr    * sRtBMPHdr;
    sdpstStack       sRevStack;
    UInt             sState = 0;

    IDE_DASSERT( aSegPID != SD_NULL_PID );
    IDE_DASSERT( aRevStack != NULL );
    IDE_DASSERT( aOrderedStack != NULL );

    /* reverse stack에는 (it,lfslotno)->(rt,itslotno) 만 들어가 있다. */
    if ( sdpstStackMgr::getDepth( aRevStack ) != SDPST_RTBMP )
    {
        sdpstStackMgr::dump( aRevStack );
        IDE_ASSERT( 0 );
    }

    sRevStack = *aRevStack;
    sCurrPos = sdpstStackMgr::getCurrPos( &sRevStack );

    sRtBMPIdx  = SDPST_INVALID_SLOTNO;
    sCurrRtBMP = aSegPID;
    IDE_ASSERT( sCurrRtBMP != SD_NULL_PID );

    while( sCurrRtBMP != SD_NULL_PID )
    {
        sRtBMPIdx++;
        if ( sCurrPos.mNodePID == sCurrRtBMP )
        {
            break; // found it
        }

        sState = 0;
        IDE_TEST( sdbBufferMgr::fixPageByPID( aStatistics,
                                              aSpaceID,
                                              sCurrRtBMP,
                                              &sPagePtr,
                                              &sDummy ) != IDE_SUCCESS );
        sState = 1;

        sRtBMPHdr  = sdpstSH::getRtBMPHdr(sPagePtr);
        sCurrRtBMP = sdpstRtBMP::getNxtRtBMP(sRtBMPHdr);

        sState = 0;
        IDE_TEST( sdbBufferMgr::unfixPage( aStatistics,
                                           sPagePtr ) != IDE_SUCCESS );
    }

    sdpstStackMgr::push( aOrderedStack,
                         SD_NULL_PID, // root bmp 상위 노드의 PID
                         sRtBMPIdx );

    for( sLoop = 0; sLoop < (UInt)SDPST_ITBMP; sLoop++ )
    {
        sCurrPos = sdpstStackMgr::pop( &sRevStack );
        sdpstStackMgr::push( aOrderedStack, &sCurrPos );
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    if ( sState == 1 )
    {
        IDE_ASSERT( sdbBufferMgr::unfixPage( aStatistics,
                                             sPagePtr ) == IDE_SUCCESS );

    }
    return IDE_FAILURE;
}

