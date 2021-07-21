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
 * $Id: sdpstCache.cpp 27229 2008-07-23 17:37:19Z newdaily $
 *
 * 본 파일은 Treelist Managed Segment의 Segment Runtime Cache에 대한
 * STATIC 인터페이스를 관리한다.
 *
 ***********************************************************************/

# include <smErrorCode.h>

# include <sdpstCache.h>
# include <sdpPhyPage.h>

# include <sdpstSH.h>
# include <sdpstStackMgr.h>
# include <sdpSegment.h>
# include <sdpstRtBMP.h>
# include <smuProperty.h>

/***********************************************************************
 * Description : Segment 모듈 초기화
 ***********************************************************************/
IDE_RC sdpstCache::initialize( sdpSegHandle * aSegHandle,
                               scSpaceID      aSpaceID,
                               sdpSegType     aSegType,
                               smOID          aTableOID,
                               UInt           aIndexID )
{
    UChar          * sPagePtr;
    sdpstSegCache  * sSegCache;
    sdpstSegHdr    * sSegHdr;
    UInt             sState = 0;

    IDE_ASSERT( aSegHandle != NULL );

    IDE_ASSERT( iduMemMgr::malloc( IDU_MEM_SM_SDP,
                                   ID_SIZEOF(sdpstSegCache),
                                   (void **)&(sSegCache),
                                   IDU_MEM_FORCE )
                == IDE_SUCCESS );
    sState  = 1;

    idlOS::memset( sSegCache, 0x00, ID_SIZEOF(*sSegCache) );

    sdpSegment::initCommonCache( &sSegCache->mCommon,
                                 aSegType,
                                 0, /* aPageCntInExt */
                                 aTableOID,
                                 aIndexID );

    /* Segment WM 갱신을 위한 Latch */
    IDE_ASSERT( (sSegCache->mLatch4WM).initialize((SChar*)"TMS_WM_LATCH") == IDE_SUCCESS );

    /* Segment 공간확장을 위한 Mutex*/
    IDE_ASSERT( sSegCache->mExtendExt.initialize(
                                     (SChar*)"TMS_EXTEND_MUTEX",
                                     IDU_MUTEX_KIND_POSIX,
                                     IDV_WAIT_INDEX_NULL ) == IDE_SUCCESS );

    /* Segment 공간 확장 진행 여부 */
    sSegCache->mOnExtend = ID_FALSE;

    /* Condition Variable 초기화 */
    IDE_TEST_RAISE( sSegCache->mCondVar.initialize((SChar *)"TMS_COND") != IDE_SUCCESS,
                    error_cond_init );

    /* Waiter */
    sSegCache->mWaitThrCnt4Extend = 0;

    /* Segment Type */
    sSegCache->mSegType = aSegType;

    /* TMS It Hint 초기화 */
    clearItHint( sSegCache );

    /* Candidate Child Set을 만들기 위한 Hint */
    sSegCache->mHint4CandidateChild = 0;

    /* Page 탐색 시작 Hint 갱신을 위한 Mutex */
    IDE_ASSERT( (sSegCache->mHint4Page.mLatch4Hint).initialize(
                                     (SChar*)"TMS_ITHINT4PAGE_LATCH")
                == IDE_SUCCESS );

    /* Slot 탐색 시작 Hint 갱신을 위한 Mutex */
    IDE_ASSERT( (sSegCache->mHint4Slot.mLatch4Hint).initialize(
                                     (SChar*)"TMS_ITHINT4PAGE_LATCH")
                == IDE_SUCCESS );

    /* format page count */
    sSegCache->mFmtPageCnt     = 0;

    /* Segment Lst Alloc Page 갱신을 위한 Mutex */
    IDE_ASSERT( sSegCache->mMutex4LstAllocPage.initialize(
                                     (SChar*)"TMS_LST_ALLOC_PAGE",
                                     IDU_MUTEX_KIND_NATIVE,
                                     IDV_WAIT_INDEX_NULL ) == IDE_SUCCESS );

    sSegCache->mUseLstAllocPageHint = ID_FALSE;
    sSegCache->mLstAllocPID         = SD_NULL_PID;
    sSegCache->mLstAllocSeqNo       = 0;

    if( smuProperty::getTmsDelayedAllocHintPageArr() == ID_FALSE )
    {
        allocHintPageArray( sSegCache );
    }
    else
    {
        sSegCache->mHint4DataPage = NULL;
    }

    if ( aSegHandle->mSegPID != SD_NULL_PID )
    {
        /* HWM/format page count를 설정한다. */
        IDE_TEST( sdbBufferMgr::getPageByPID( NULL, /* aStatistics */
                                              aSpaceID,
                                              aSegHandle->mSegPID,
                                              SDB_S_LATCH,
                                              SDB_WAIT_NORMAL,
                                              SDB_SINGLE_PAGE_READ,
                                              NULL, /* sdrMtx */
                                              &sPagePtr,
                                              NULL, /* aTrySuccess */
                                              NULL  /* aIsCorruptPage */ )
                  != IDE_SUCCESS );
        sState = 2;

        sSegHdr = sdpstSH::getHdrPtr( sPagePtr );

        sSegCache->mHWM        = sSegHdr->mHWM;
        sSegCache->mFmtPageCnt = sSegHdr->mTotPageCnt;

        sState = 1;
        IDE_TEST( sdbBufferMgr::releasePage( NULL, (UChar*)sSegHdr )
                  != IDE_SUCCESS );

        /* Internal Hint를 설정한다. */
        IDE_TEST( sdpstRtBMP::rescanItHint(
                                NULL,     /* aStatistics */
                                aSpaceID,
                                aSegHandle->mSegPID,
                                SDPST_SEARCH_NEWSLOT,
                                sSegCache,
                                NULL ) != IDE_SUCCESS );

        IDE_TEST( sdpstRtBMP::rescanItHint(
                                NULL,     /* aStatistics */
                                aSpaceID,
                                aSegHandle->mSegPID,
                                SDPST_SEARCH_NEWPAGE,
                                sSegCache,
                                NULL ) != IDE_SUCCESS );
    }

    aSegHandle->mCache   = (void*)sSegCache;

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_cond_init );
    {
        IDE_SET( ideSetErrorCode(smERR_FATAL_ThrCondInit) );
    }
    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 2:
            IDE_ASSERT( sdbBufferMgr::releasePage( NULL, (UChar*)sSegHdr )
                        == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( iduMemMgr::free( sSegCache ) == IDE_SUCCESS );
        default:
            break;
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Segment Cache의 Hint page array를 할당 및 초기화한다.
 *               BUG-28935 hint page array를 server start후 최초의
 *               get insertable page에서 alloc 하도록 수정
 ***********************************************************************/
void sdpstCache::allocHintPageArray( sdpstSegCache * aSegCache )
{
    lockLstAllocPage( NULL, // aStatistics
                      aSegCache );

    /* Alloc하는 사이에 다른 Thread가 이미 할당 하였는지 확인한다.*/
    if( aSegCache->mHint4DataPage == NULL )
    {
        IDE_ASSERT( iduMemMgr::calloc( IDU_MEM_SM_SDP, 1,
                                       ID_SIZEOF(scPageID) * smuProperty::getTmsHintPageArrSize(),
                                       (void **)&(aSegCache->mHint4DataPage),
                                       IDU_MEM_FORCE )
                    == IDE_SUCCESS );

        /* 혹시 나중에 SM_NULL_PID가 변경되어 0이 아니게 되면
         * calloc으로 할당 받았다고 해도 초기화 되지 않은 것이므로
         * 한번 더 확인한다. */
        IDE_DASSERT( aSegCache->mHint4DataPage[0] == SM_NULL_PID );
    }

    unlockLstAllocPage( aSegCache );
}


/***********************************************************************
 * Description : Segment Cache의 Search Hint정보를 초기화한다.
 ***********************************************************************/
void sdpstCache::clearItHint( void * aSegCache )
{
    sdpstSegCache * sSegCache;
    IDE_ASSERT( aSegCache != NULL );

    sSegCache = (sdpstSegCache *)aSegCache;

    /* Slot 할당을 위한 가용공간탐색 시작 internal bitmap page의 Hint*/
    sdpstStackMgr::initialize( &(sSegCache->mHint4Slot.mHintItStack) );
    sSegCache->mHint4Slot.mUpdateHintItBMP       = ID_FALSE;

    /* Page 할당을 위한 가용공간탐색 시작 internal bitmap page의 Hint*/
    sdpstStackMgr::initialize( &(sSegCache->mHint4Page.mHintItStack) );
    sSegCache->mHint4Page.mUpdateHintItBMP       = ID_FALSE;
}


/***********************************************************************
 * Description : Segment 모듈 해제
 ***********************************************************************/
IDE_RC sdpstCache::destroy( sdpSegHandle * aSegHandle )
{
    sdpstSegCache  * sSegCache;

    IDE_ASSERT( aSegHandle != NULL );

    sSegCache = (sdpstSegCache*)(aSegHandle->mCache);

    IDE_ASSERT( (sSegCache->mHint4Page.mLatch4Hint).destroy()
                == IDE_SUCCESS );
    IDE_ASSERT( (sSegCache->mHint4Slot.mLatch4Hint).destroy()
                == IDE_SUCCESS );

    IDE_TEST_RAISE( sSegCache->mCondVar.destroy() != IDE_SUCCESS,
                    error_cond_destroy );

    // Segment Extent Mutex를 해제한다.
    IDE_ASSERT( sSegCache->mExtendExt.destroy() == IDE_SUCCESS);

    // Segment WM Latch를 해제한다.
    IDE_ASSERT( ((sSegCache->mLatch4WM).destroy()) == IDE_SUCCESS);

    // Segment Last Alloc Page Mutex를 해제한다.
    IDE_ASSERT( sSegCache->mMutex4LstAllocPage.destroy() == IDE_SUCCESS );

    if( sSegCache->mHint4DataPage != NULL )
    {
        IDE_ASSERT( iduMemMgr::free( sSegCache->mHint4DataPage ) == IDE_SUCCESS );
    }

    // Segment Cache를 메모리 해제한다.
    IDE_ASSERT( iduMemMgr::free( aSegHandle->mCache ) == IDE_SUCCESS );
    aSegHandle->mCache = NULL;

    return IDE_SUCCESS;
    IDE_EXCEPTION( error_cond_destroy );
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondDestroy));
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Segment 확장에 대한 Extent Mutex을 획득 혹은 대기
 ***********************************************************************/
IDE_RC sdpstCache::prepareExtendExtOrWait( idvSQL            * aStatistics,
                                        sdpstSegCache * aSegCache,
                                        idBool            * aDoExtendExt )
{

    IDE_ASSERT( aSegCache != NULL );
    IDE_ASSERT( aDoExtendExt != NULL );

    lockExtendExt( aStatistics, aSegCache );

    // Extent 진행 여부를 판단한다.
    if( isOnExtend( aSegCache ) == ID_TRUE )
    {
        /* BUG-44834 특정 장비에서 sprious wakeup 현상이 발생하므로 
                     wakeup 후에도 다시 확인 하도록 while문으로 체크한다.*/
        while ( isOnExtend( aSegCache ) == ID_TRUE )
        {
            // ExrExt Mutex를 획득한 상태에서 대기자를 증가시킨다.
            aSegCache->mWaitThrCnt4Extend++;

            IDE_TEST_RAISE( aSegCache->mCondVar.wait(&(aSegCache->mExtendExt))
                            != IDE_SUCCESS, error_cond_wait );

            aSegCache->mWaitThrCnt4Extend--;
        }
        // 이미 Extend가 완료되었기 때문에 Extent 확장을
        // 연이어 할필요없이 가용공간 탐색을 진행한다.
        *aDoExtendExt = ID_FALSE;
    }
    else
    {
        // 직접 Segment 확장을 수행하기 위해 OnExtend를 On시킨다.
        aSegCache->mOnExtend = ID_TRUE;
        *aDoExtendExt = ID_TRUE;
    }

    unlockExtendExt( aSegCache );

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_cond_wait );
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondWait));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : Segment 확장에 대한 Extent Mutex을 획득 혹은 대기
 *               대기하는 트랜잭션 깨운다.
 ***********************************************************************/
IDE_RC sdpstCache::completeExtendExtAndWakeUp( idvSQL            * aStatistics,
                                            sdpstSegCache * aSegCache )
{
    IDE_ASSERT( aSegCache != NULL );
    IDE_ASSERT( isOnExtend(aSegCache) == ID_TRUE );

    lockExtendExt( aStatistics, aSegCache );

    if ( aSegCache->mWaitThrCnt4Extend > 0 )
    {
        // 대기 트랜잭션을 모두 깨운다.
        IDE_TEST_RAISE(aSegCache->mCondVar.broadcast() != IDE_SUCCESS,
                       error_cond_signal );
    }

    // Segment 확장 진행을 완료하였음을 설정한다.
    aSegCache->mOnExtend = ID_FALSE;

    unlockExtendExt( aSegCache );

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_cond_signal );
    {
        IDE_SET( ideSetErrorCode(smERR_FATAL_ThrCondSignal) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : 가용공간 탐색시 시작 it-bmp 페이지의 위치이력을
 *               후진시킨다.
 ***********************************************************************/
void sdpstCache::setItHintIfLT( idvSQL            * aStatistics,
                                sdpstSegCache     * aSegCache,
                                sdpstSearchType     aSearchType,
                                sdpstStack        * aStack,
                                idBool            * aItHintFlag )
{
    IDE_ASSERT( aSegCache != NULL );
    IDE_ASSERT( aStack != NULL );
    IDE_ASSERT( sdpstStackMgr::getDepth(aStack)
                == SDPST_ITBMP );
    IDE_ASSERT( aSearchType == SDPST_SEARCH_NEWSLOT ||
                aSearchType == SDPST_SEARCH_NEWPAGE );
    

    /* break 문은 default에만 있다. */
    switch( aSearchType )
    {
        case SDPST_SEARCH_NEWPAGE:
            lockPageHint4Write( aStatistics, aSegCache );
            if ( sdpstStackMgr::compareStackPos(
                     &(aSegCache->mHint4Page.mHintItStack), aStack) > 0 )
            {
                aSegCache->mHint4Page.mHintItStack = *aStack;
            }
            unlockPageHint( aSegCache );

            if ( aItHintFlag != NULL )
            {
                setUpdateHint4Page( aSegCache, *aItHintFlag );
            }
            break;

        case SDPST_SEARCH_NEWSLOT:
            lockSlotHint4Write( aStatistics, aSegCache );
            if ( sdpstStackMgr::compareStackPos(
                     &(aSegCache->mHint4Slot.mHintItStack), aStack) > 0 )
            {
                aSegCache->mHint4Slot.mHintItStack = *aStack;
                if  ( aSegCache->mHint4Slot.mHintItStack.mDepth == 0 )
                {
                    dump( aSegCache );
                    IDE_ASSERT( 0 );
                }
            }
            unlockSlotHint( aSegCache );

            if ( aItHintFlag != NULL )
            {
                setUpdateHint4Slot( aSegCache, *aItHintFlag );
            }

            break;
        default:
            ideLog::log( IDE_SERVER_0,
                         "SearchType: %u\n",
                         aSearchType );
            IDE_ASSERT(0);
            break;
    }
}

/***********************************************************************
 * Description : 가용공간 탐색시 시작 it-bmp 페이지의 위치이력을 전진
 *               시킨다.
 ***********************************************************************/
void sdpstCache::setItHintIfGT( idvSQL            * aStatistics,
                                sdpstSegCache     * aSegCache,
                                sdpstSearchType     aSearchType,
                                sdpstStack        * aStack )
{
    IDE_ASSERT( aSegCache != NULL );
    IDE_ASSERT( aStack != NULL );
    IDE_ASSERT( sdpstStackMgr::getDepth(aStack)
                == SDPST_ITBMP );

    switch( aSearchType )
    {
        case SDPST_SEARCH_NEWSLOT:

            lockSlotHint4Write( aStatistics, aSegCache );
            if ( sdpstStackMgr::compareStackPos(
                     &(aSegCache->mHint4Slot.mHintItStack), aStack) < 0 )
            {
                aSegCache->mHint4Slot.mHintItStack = *aStack;
                if ( aSegCache->mHint4Slot.mHintItStack.mDepth == SDPST_EMPTY )
                {
                    dump( aSegCache );
                    IDE_ASSERT( 0 );
                }

            }
            unlockSlotHint( aSegCache );
            break;

        case SDPST_SEARCH_NEWPAGE:

            lockPageHint4Write( aStatistics, aSegCache );
            if ( sdpstStackMgr::compareStackPos(
                     &(aSegCache->mHint4Page.mHintItStack), aStack) < 0 )
            {
                aSegCache->mHint4Page.mHintItStack = *aStack;
            }
            unlockPageHint( aSegCache );
            break;

        default:
            ideLog::log( IDE_SERVER_0,
                         "SearchType: %u\n",
                         aSearchType );
            IDE_ASSERT(0);
            break;
    }
}

/***********************************************************************
 * Description : HWM 을 복사한다.
 ***********************************************************************/
void sdpstCache::copyHWM( idvSQL            * aStatistics,
                          sdpstSegCache     * aSegCache,
                          sdpstWM           * aWM )
{
    IDE_ASSERT( aSegCache != NULL );
    IDE_ASSERT( aWM       != NULL );

    lockHWM4Read( aStatistics, aSegCache );
    *aWM = aSegCache->mHWM;
    unlockHWM( aSegCache );
}

/***********************************************************************
 * Description : 가용공간 탐색시 시작 it-bmp 페이지의 위치이력을
 *               복사한다.
 ***********************************************************************/
void sdpstCache::copyItHint( idvSQL            * aStatistics,
                             sdpstSegCache     * aSegCache,
                             sdpstSearchType     aSearchType,
                             sdpstStack        * aHintStack )
{
    IDE_ASSERT( aSegCache != NULL );

    switch( aSearchType )
    {
        case SDPST_SEARCH_NEWSLOT:

            lockSlotHint4Read( aStatistics, aSegCache );
            *aHintStack = aSegCache->mHint4Slot.mHintItStack;
            unlockSlotHint( aSegCache );

            break;

        case SDPST_SEARCH_NEWPAGE:

            lockPageHint4Read( aStatistics, aSegCache );
            *aHintStack = aSegCache->mHint4Page.mHintItStack;
            unlockPageHint( aSegCache );

            break;

        default:
            ideLog::log( IDE_SERVER_0,
                         "SearchType: %u\n",
                         aSearchType );
            IDE_ASSERT(0);
            break;
    }
}

/***********************************************************************
 * Description : 
 ***********************************************************************/
void sdpstCache::initItHint( idvSQL            * aStatistics,
                             sdpstSegCache     * aSegCache,
                             scPageID            aRtBMP,
                             scPageID            aItBMP )
{
    sdpstStack  sStack;

    sdpstStackMgr::initialize( &sStack );
    sdpstStackMgr::push( &sStack, SD_NULL_PID, 0 ); /* Virtual BMP */
    sdpstStackMgr::push( &sStack, aRtBMP, 0 );      /* Rt BMP */
    sdpstStackMgr::push( &sStack, aItBMP, 0 );      /* It BMP */

    sdpstCache::setItHintIfLT( aStatistics,
                               aSegCache,
                               SDPST_SEARCH_NEWSLOT,
                               &sStack,
                               NULL );

    sdpstCache::setItHintIfLT( aStatistics,
                               aSegCache,
                               SDPST_SEARCH_NEWPAGE,
                               &sStack,
                               NULL );
}

/***********************************************************************
 * Description : it hint 중에 최소 hint 위치를 반환한다.
 ***********************************************************************/
sdpstStack sdpstCache::getMinimumItHint( idvSQL        * aStatistics,
                                         sdpstSegCache * aSegCache )
{
    sdpstStack         sItHint4Page;
    sdpstStack         sItHint4Slot;

    IDE_ASSERT( aSegCache != NULL );
    /*
     * free page가 발생한 위치가 it hint 페이지보다 앞이라면,
     * Segment Cache의 Hint Flag를 On 시켜야 한다.
     */
    sdpstCache::copyItHint( aStatistics,
                            aSegCache,
                            SDPST_SEARCH_NEWPAGE,
                            &sItHint4Page );

    if ( sdpstStackMgr::getDepth( &sItHint4Page ) != SDPST_ITBMP )
    {
        sdpstStackMgr::dump( &sItHint4Page );
        dump( aSegCache );
        IDE_ASSERT( 0 );
    }

    sdpstCache::copyItHint( aStatistics,
                            aSegCache,
                            SDPST_SEARCH_NEWSLOT,
                            &sItHint4Slot );

    if ( sdpstStackMgr::getDepth( &sItHint4Slot ) != SDPST_ITBMP )
    {
        sdpstStackMgr::dump( &sItHint4Slot );
        dump( aSegCache );
        IDE_ASSERT( 0 );
    }

    /* 작은 것을 선택하는 이유는 free page가 발생하면, slot hint와 page hint에
     * 모두 영향을 미치기 때문이다 */
    if ( sdpstStackMgr::compareStackPos( &sItHint4Slot, &sItHint4Page ) > 0 )
    {
        return sItHint4Page;
    }
    else
    {
        return sItHint4Slot;
    }
}

/***********************************************************************
 * Description : [INTERFACE] Hint 정보를 반환한다.
 ***********************************************************************/
void sdpstCache::getHintPosInfo( idvSQL          * aStatistics,
                                 void            * aSegCache,
                                 sdpHintPosInfo  * aHintPosInfo )
{
    sdpstStack         sItHint;
    sdpstPosItem       sPosItem;

    IDE_ASSERT( aSegCache    != NULL );
    IDE_ASSERT( aHintPosInfo != NULL );

    sdpstCache::copyItHint( aStatistics,
                            (sdpstSegCache*)aSegCache,
                            SDPST_SEARCH_NEWSLOT,
                            &sItHint );

    if ( sdpstStackMgr::getDepth( &sItHint ) != SDPST_ITBMP )
    {
        sdpstStackMgr::dump( &sItHint );
        dump( (sdpstSegCache*)aSegCache );
        IDE_ASSERT( 0 );
    }

    sPosItem = sdpstStackMgr::getSeekPos( &sItHint,
                                          SDPST_VIRTBMP );

    aHintPosInfo->mSPosVtPID = sPosItem.mNodePID;
    aHintPosInfo->mSRtBMPIdx = sPosItem.mIndex;

    sPosItem = sdpstStackMgr::getSeekPos( &sItHint,
                                          SDPST_RTBMP );

    aHintPosInfo->mSPosRtPID = sPosItem.mNodePID;
    aHintPosInfo->mSItBMPIdx = sPosItem.mIndex;

    sPosItem = sdpstStackMgr::getSeekPos( &sItHint,
                                          SDPST_ITBMP );

    aHintPosInfo->mSPosItPID = sPosItem.mNodePID;
    aHintPosInfo->mSLfBMPIdx = sPosItem.mIndex;

    aHintPosInfo->mSRsFlag =
        (((sdpstSegCache*)aSegCache)->mHint4Slot.mUpdateHintItBMP
         == ID_TRUE ? 1 : 0 );
    aHintPosInfo->mSStFlag = 0; /* StopOnLstFullItHint flag */

    sdpstCache::copyItHint( aStatistics,
                            (sdpstSegCache*)aSegCache,
                            SDPST_SEARCH_NEWPAGE,
                            &sItHint );

    if ( sdpstStackMgr::getDepth( &sItHint ) != SDPST_ITBMP )
    {
        sdpstStackMgr::dump( &sItHint );
        dump( (sdpstSegCache*)aSegCache );
        IDE_ASSERT( 0 );
    }

    sPosItem = sdpstStackMgr::getSeekPos( &sItHint,
                                          SDPST_VIRTBMP );

    aHintPosInfo->mPPosVtPID = sPosItem.mNodePID;
    aHintPosInfo->mPRtBMPIdx = sPosItem.mIndex;

    sPosItem = sdpstStackMgr::getSeekPos( &sItHint,
                                          SDPST_RTBMP );

    aHintPosInfo->mPPosRtPID = sPosItem.mNodePID;
    aHintPosInfo->mPItBMPIdx = sPosItem.mIndex;

    sPosItem = sdpstStackMgr::getSeekPos( &sItHint,
                                          SDPST_ITBMP );

    aHintPosInfo->mPPosItPID = sPosItem.mNodePID;
    aHintPosInfo->mPLfBMPIdx = sPosItem.mIndex;

    aHintPosInfo->mPRsFlag =
        (((sdpstSegCache*)aSegCache)->mHint4Page.mUpdateHintItBMP
         == ID_TRUE ? 1 : 0 );
    aHintPosInfo->mPStFlag = 0; /* StopOnLstFullItHint flag */
}

/***********************************************************************
 * Description : [INTERFACE] Segment의 Format Page Count를 반환한다.
 ***********************************************************************/
IDE_RC sdpstCache::getFmtPageCnt( idvSQL          * /*aStatistics*/,
                                  scSpaceID         /*aSpaceID*/,
                                  sdpSegHandle    * aSegHandle,
                                  ULong           * aFmtPageCnt )
{
    sdpstSegCache   * sSegCache;

    IDE_ASSERT( aSegHandle  != NULL );
    IDE_ASSERT( aFmtPageCnt != NULL );

    sSegCache = (sdpstSegCache*)aSegHandle->mCache;

    *aFmtPageCnt = sSegCache->mFmtPageCnt;

    return IDE_SUCCESS;
}

/***********************************************************************
 * Description : [INTERFACE] Segment Cache에 LstAllocPage를 설정한다.
 ***********************************************************************/
IDE_RC sdpstCache::setLstAllocPage( idvSQL          * aStatistics,
                                    sdpSegHandle    * aSegHandle,
                                    scPageID          aLstAllocPID,
                                    ULong             aLstAllocSeqNo )
{
    sdpstSegCache   * sSegCache;

    IDE_ASSERT( aSegHandle   != NULL );
    IDE_ASSERT( aLstAllocPID != SD_NULL_PID );

    sSegCache = (sdpstSegCache*)aSegHandle->mCache;

    IDE_ASSERT( sSegCache != NULL );

    /* 정말로 갱신해야 하는지 lock을 잡고 확인해본다. */
    lockLstAllocPage( aStatistics, sSegCache );

    /* 이전 마지막 할당된 페이지 이후 페이지이면, 설정한다. */
    if ( sSegCache->mLstAllocSeqNo <= aLstAllocSeqNo )
    {
        sSegCache->mUseLstAllocPageHint = ID_TRUE;
        sSegCache->mLstAllocPID         = aLstAllocPID;
        sSegCache->mLstAllocSeqNo       = aLstAllocSeqNo;
    }

    unlockLstAllocPage( sSegCache );

    return IDE_SUCCESS;
}

/***********************************************************************
 * Description : Segment Cache에 LstAllocPage를 설정한다.
 ***********************************************************************/
IDE_RC sdpstCache::setLstAllocPage4AllocPage( idvSQL          * aStatistics,
                                              sdpSegHandle    * aSegHandle,
                                              scPageID          aLstAllocPID,
                                              ULong             aLstAllocSeqNo )
{
    sdpstSegCache   * sSegCache;

    IDE_ASSERT( aSegHandle   != NULL );
    IDE_ASSERT( aLstAllocPID != SD_NULL_PID );

    sSegCache = (sdpstSegCache*)aSegHandle->mCache;

    IDE_ASSERT( sSegCache != NULL );

    /* 정말로 갱신해야 하는지 lock을 잡고 확인해본다. */
    lockLstAllocPage( aStatistics, sSegCache );

    /* 이전 마지막 할당된 페이지 이후 페이지이면, 설정한다. */
    if ( sSegCache->mLstAllocSeqNo < aLstAllocSeqNo )
    {
        sSegCache->mLstAllocPID   = aLstAllocPID;
        sSegCache->mLstAllocSeqNo = aLstAllocSeqNo;
    }

    unlockLstAllocPage( sSegCache );

    return IDE_SUCCESS;
}

/***********************************************************************
 * Description : Segment Hint Data Page를 가져온다.
 ***********************************************************************/
void sdpstCache::getHintDataPage( idvSQL            * aStatistics,
                                  sdpstSegCache     * aSegCache,
                                  scPageID          * aHintDataPID )
{
    UInt        sSID;
    UInt        sHintIdx;

    IDE_ASSERT( aSegCache    != NULL );
    IDE_ASSERT( aHintDataPID != NULL );

    sSID = idvManager::getSessionID( aStatistics );

    sHintIdx = sSID % smuProperty::getTmsHintPageArrSize();

    if( aSegCache->mHint4DataPage == NULL )
    {
        allocHintPageArray( aSegCache );
    }

    *aHintDataPID = aSegCache->mHint4DataPage[sHintIdx];
}

/***********************************************************************
 * Description : Segment Hint Data Page를 설정한다.
 ***********************************************************************/
void sdpstCache::setHintDataPage( idvSQL            * aStatistics,
                                  sdpstSegCache     * aSegCache,
                                  scPageID            aHintDataPID )
{
    UInt        sSID;
    UInt        sHintIdx;

    IDE_ASSERT( aSegCache    != NULL );

    sSID = idvManager::getSessionID( aStatistics );

    sHintIdx = sSID % smuProperty::getTmsHintPageArrSize();

    if( aSegCache->mHint4DataPage == NULL )
    {
        allocHintPageArray( aSegCache );
    }

    aSegCache->mHint4DataPage[sHintIdx] = aHintDataPID;
}

/***********************************************************************
 * Description : [INTERFACE] Segment Cache Info 를 가져온다.
 ***********************************************************************/
IDE_RC sdpstCache::getSegCacheInfo( idvSQL          * aStatistics,
                                    sdpSegHandle    * aSegHandle,
                                    sdpSegCacheInfo * aSegCacheInfo )
{
    sdpstSegCache   * sSegCache;

    IDE_ASSERT( aSegHandle    != NULL );
    IDE_ASSERT( aSegCacheInfo != NULL );

    sSegCache = (sdpstSegCache*)aSegHandle->mCache;

    IDE_ASSERT( sSegCache != NULL );

    lockLstAllocPage( aStatistics, sSegCache );

    aSegCacheInfo->mUseLstAllocPageHint = sSegCache->mUseLstAllocPageHint;
    aSegCacheInfo->mLstAllocPID         = sSegCache->mLstAllocPID;
    aSegCacheInfo->mLstAllocSeqNo       = sSegCache->mLstAllocSeqNo;

    unlockLstAllocPage( sSegCache );

    return IDE_SUCCESS;
}

void sdpstCache::dump( sdpstSegCache * aSegCache )
{
    IDE_ASSERT( aSegCache != NULL );

    ideLog::logMem( IDE_SERVER_0,
                    (UChar*)aSegCache,
                    ID_SIZEOF( sdpstSegCache ) );

    ideLog::log( IDE_SERVER_0,
                 "-------------------- SegCache Begin --------------------\n"
                 "mCommon.mTmpSegHead      : %u\n"
                 "mCommon.mTmpSegTail      : %u\n"
                 "mCommon.mSegType         : %u\n"
                 "mCommon.mSegSizeByBytes  : %llu\n"
                 "mCommon.mPageCntInExt    : %u\n"
                 "mCommon.mMetaPID         : %u\n"
                 "mOnExtend                : %u\n"
                 "mWaitThrCnt4Extend       : %u\n"
                 "mSegType                 : %u\n"
                 "mTableOID                : %lu\n"
                 "mHint4Slot.mUpdateHintItBMP : %u\n"
                 "mHint4Slot.stack.mDepth: %d\n"
                 "mHint4Slot.stack.mPosition[VT].mNodePID: %u\n"
                 "mHint4Slot.stack.mPosition[VT].mIndex: %u\n"
                 "mHint4Slot.stack.mPosition[RT].mNodePID: %u\n"
                 "mHint4Slot.stack.mPosition[RT].mIndex: %u\n"
                 "mHint4Slot.stack.mPosition[IT].mNodePID: %u\n"
                 "mHint4Slot.stack.mPosition[IT].mIndex: %u\n"
                 "mHint4Slot.stack.mPosition[LF].mNodePID: %u\n"
                 "mHint4Slot.stack.mPosition[LF].mIndex: %u\n"
                 "mHint4Page.mUpdateHintItBMP : %u\n"
                 "mHint4Page.stack.mDepth: %d\n"
                 "mHint4Page.stack.mPosition[VT].mNodePID: %u\n"
                 "mHint4Page.stack.mPosition[VT].mIndex: %u\n"
                 "mHint4Page.stack.mPosition[RT].mNodePID: %u\n"
                 "mHint4Page.stack.mPosition[RT].mIndex: %u\n"
                 "mHint4Page.stack.mPosition[IT].mNodePID: %u\n"
                 "mHint4Page.stack.mPosition[IT].mIndex: %u\n"
                 "mHint4Page.stack.mPosition[LF].mNodePID: %u\n"
                 "mHint4Page.stack.mPosition[LF].mIndex: %u\n"
                 "mHint4CandidateChild: %u\n"
                 "mFmtPageCnt: %llu\n"
                 "HWM.mPID: %u\n"
                 "HWM.mExtDirPID: %u\n"
                 "HWM.mSlotNoInExtDir: %d\n"
                 "HWM.stack.mDepth: %d\n"
                 "HWM.stack.mPosition[VT].mNodePID: %u\n"
                 "HWM.stack.mPosition[VT].mIndex: %u\n"
                 "HWM.stack.mPosition[RT].mNodePID: %u\n"
                 "HWM.stack.mPosition[RT].mIndex: %u\n"
                 "HWM.stack.mPosition[IT].mNodePID: %u\n"
                 "HWM.stack.mPosition[IT].mIndex: %u\n"
                 "HWM.stack.mPosition[LF].mNodePID: %u\n"
                 "HWM.stack.mPosition[LF].mIndex: %u\n"
                 "mLstAllocPID: %u\n"
                 "--------------------  SegCache End  --------------------\n",
                 aSegCache->mCommon.mTmpSegHead,
                 aSegCache->mCommon.mTmpSegTail,
                 aSegCache->mCommon.mSegType,
                 aSegCache->mCommon.mSegSizeByBytes,
                 aSegCache->mCommon.mPageCntInExt,
                 aSegCache->mCommon.mMetaPID,
                 aSegCache->mOnExtend,
                 aSegCache->mWaitThrCnt4Extend,
                 aSegCache->mSegType,
                 aSegCache->mCommon.mTableOID,
                 aSegCache->mHint4Slot.mUpdateHintItBMP,
                 aSegCache->mHint4Slot.mHintItStack.mDepth,
                 aSegCache->mHint4Slot.mHintItStack.mPosition[SDPST_VIRTBMP].mNodePID,
                 aSegCache->mHint4Slot.mHintItStack.mPosition[SDPST_VIRTBMP].mIndex,
                 aSegCache->mHint4Slot.mHintItStack.mPosition[SDPST_RTBMP].mNodePID,
                 aSegCache->mHint4Slot.mHintItStack.mPosition[SDPST_RTBMP].mIndex,
                 aSegCache->mHint4Slot.mHintItStack.mPosition[SDPST_ITBMP].mNodePID,
                 aSegCache->mHint4Slot.mHintItStack.mPosition[SDPST_ITBMP].mIndex,
                 aSegCache->mHint4Slot.mHintItStack.mPosition[SDPST_LFBMP].mNodePID,
                 aSegCache->mHint4Slot.mHintItStack.mPosition[SDPST_LFBMP].mIndex,

                 aSegCache->mHint4Page.mUpdateHintItBMP,
                 aSegCache->mHint4Page.mHintItStack.mDepth,
                 aSegCache->mHint4Page.mHintItStack.mPosition[SDPST_VIRTBMP].mNodePID,
                 aSegCache->mHint4Page.mHintItStack.mPosition[SDPST_VIRTBMP].mIndex,
                 aSegCache->mHint4Page.mHintItStack.mPosition[SDPST_RTBMP].mNodePID,
                 aSegCache->mHint4Page.mHintItStack.mPosition[SDPST_RTBMP].mIndex,
                 aSegCache->mHint4Page.mHintItStack.mPosition[SDPST_ITBMP].mNodePID,
                 aSegCache->mHint4Page.mHintItStack.mPosition[SDPST_ITBMP].mIndex,
                 aSegCache->mHint4Page.mHintItStack.mPosition[SDPST_LFBMP].mNodePID,
                 aSegCache->mHint4Page.mHintItStack.mPosition[SDPST_LFBMP].mIndex,

                 aSegCache->mHint4CandidateChild,
                 aSegCache->mFmtPageCnt,
                 aSegCache->mHWM.mWMPID,
                 aSegCache->mHWM.mExtDirPID,
                 aSegCache->mHWM.mSlotNoInExtDir,
                 aSegCache->mHWM.mStack.mDepth,
                 aSegCache->mHWM.mStack.mPosition[SDPST_VIRTBMP].mNodePID,
                 aSegCache->mHWM.mStack.mPosition[SDPST_VIRTBMP].mIndex,
                 aSegCache->mHWM.mStack.mPosition[SDPST_RTBMP].mNodePID,
                 aSegCache->mHWM.mStack.mPosition[SDPST_RTBMP].mIndex,
                 aSegCache->mHWM.mStack.mPosition[SDPST_ITBMP].mNodePID,
                 aSegCache->mHWM.mStack.mPosition[SDPST_ITBMP].mIndex,
                 aSegCache->mHWM.mStack.mPosition[SDPST_LFBMP].mNodePID,
                 aSegCache->mHWM.mStack.mPosition[SDPST_LFBMP].mIndex,
                 aSegCache->mLstAllocPID );
}
