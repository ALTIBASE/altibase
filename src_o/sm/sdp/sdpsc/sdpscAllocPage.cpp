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
 * $Id: sdpscAllocPage.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * 본 파일은 Circular-List Managed Segment에서 가용공간 할당 연산 관련 STATIC
 * 인터페이스를 관리한다.
 *
 ***********************************************************************/

# include <sdpReq.h>
# include <sdpPhyPage.h>

# include <sdpscSH.h>
# include <sdpscED.h>

# include <sdpscSegDDL.h>
# include <sdpscAllocPage.h>


/***********************************************************************
 * Description : 페이지를 생성하고, 필요한 페이지 타입 초기화 및 logical
 *               Header를 초기화한다.
 *
 * aStatistics   - [IN]  통계정보
 * aSpaceID      - [IN]  테이블스페이스 ID
 * aNewPageID    - [IN]  생성할 페이지 ID
 * aPageType     - [IN]  생성할 페이지의 타입
 * aParentInfo   - [IN]  생성할 페이지 헤더에 기록할 상위 노드의 정보
 *                       없으면 기록하지 않음
 * aPageBitSet   - [IN]  페이지 가용공간의 상태값
 * aMtx4Latch    - [IN]  페이지에 대한 X-Latch를 유지해야할 Mtx 포인터
 * aMtx4Logging  - [IN]  페이지 생성로그를 기록하기 위한 Mtx 포인터
 *                       aMtx4Latch와 동일한 포인터일수도 있다.
 * aNewPagePtr   - [OUT] 생성된 새 페이지의 시작 포인터
 *
 ***********************************************************************/
IDE_RC sdpscAllocPage::createPage( idvSQL           * aStatistics,
                                   scSpaceID          aSpaceID,
                                   scPageID           aNewPageID,
                                   sdpPageType        aPageType,
                                   sdpParentInfo    * aParentInfo,
                                   sdpscPageBitSet    aPageBitSet,
                                   sdrMtx           * aMtx4Latch,
                                   sdrMtx           * aMtx4Logging,
                                   UChar           ** aNewPagePtr )
{
    UChar         * sNewPagePtr;

    IDE_DASSERT( aNewPageID   != SD_NULL_PID );
    IDE_DASSERT( aParentInfo  != NULL );
    IDE_DASSERT( aMtx4Latch   != NULL );
    IDE_DASSERT( aMtx4Logging != NULL );
    IDE_DASSERT( aNewPagePtr  != NULL );

    IDE_TEST( sdbBufferMgr::createPage( aStatistics,
                                        aSpaceID,
                                        aNewPageID,
                                        aPageType,
                                        aMtx4Latch,
                                        &sNewPagePtr ) != IDE_SUCCESS );

    IDE_TEST( formatPageHdr( (sdpPhyPageHdr*)sNewPagePtr,
                             aNewPageID,
                             aPageType,
                             aParentInfo,
                             aPageBitSet,
                             aMtx4Logging ) != IDE_SUCCESS );

    if ( aMtx4Latch != aMtx4Logging )
    {
        IDE_TEST( sdrMiniTrans::setDirtyPage( aMtx4Logging, sNewPagePtr )
                  != IDE_SUCCESS );
    }

    *aNewPagePtr = sNewPagePtr;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : phyical hdr와 logical hdr를 page type에 맞게 format한다.
 *
 * aNewPagePtr   - [IN] 초기화할 페이지의 시작 포인터
 * aNewPageID    - [IN] 생성할 페이지 ID
 * aPageType     - [IN] 생성할 페이지의 타입
 * aParentInfo   - [IN] 생성할 페이지 헤더에 기록할 상위 노드의 정보
 *                      없으면 기록하지 않음
 * aPageBitSet   - [IN] 페이지 가용공간의 상태값
 * aMtx          - [IN] 페이지 생성로그를 기록하기 위한 Mtx 포인터
 *
 ***********************************************************************/
IDE_RC sdpscAllocPage::formatPageHdr( sdpPhyPageHdr    * aNewPagePtr,
                                      scPageID           aNewPageID,
                                      sdpPageType        aPageType,
                                      sdpParentInfo    * aParentInfo,
                                      sdpscPageBitSet    aPageBitSet,
                                      sdrMtx           * aMtx )
{
    IDE_ASSERT( aMtx        != NULL );
    IDE_ASSERT( aNewPagePtr != NULL );
    IDE_ASSERT( aNewPageID  != SD_NULL_PID );
    IDE_ASSERT( aParentInfo != NULL );

    IDE_TEST( sdpPhyPage::logAndInit( aNewPagePtr,
                                      aNewPageID,
                                      aParentInfo,
                                      (UShort)aPageBitSet,
                                      aPageType,
                                      SM_NULL_OID,   /* UndoSegment   */
                                      SM_NULL_INDEX_ID,
                                      aMtx ) != IDE_SUCCESS );
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : [ INTERFACE ] Append 방식의 Page 할당 연산
 *
 * aStatistics      - [IN] 통계 정보
 * aSpaceID         - [IN] TableSpace ID
 * aSegHandle       - [IN] Segment Handle
 * aPrvAllocExtRID  - [IN] 이전에 Page를 할당받았던 Extent RID
 * aPrvAllocPageID  - [IN] 이전에 할당받은 PageID
 * aMtx             - [IN] Mini Transaction Pointer
 * aAllocExtRID     - [OUT] 새로운 Page가 할당된 Extent RID
 * aAllocPID        - [OUT] 새롭게 할당받은 PageID
 *
 ***********************************************************************/
IDE_RC sdpscAllocPage::allocNewPage4Append(
                            idvSQL              * aStatistics,
                            sdrMtx              * aMtx,
                            scSpaceID             aSpaceID,
                            sdpSegHandle        * aSegHandle,
                            sdRID                 aPrvAllocExtRID,
                            scPageID              aFstPIDOfPrvAllocExt,
                            scPageID              aPrvAllocPageID,
                            idBool                aIsLogging,
                            sdpPageType           aPageType,
                            sdRID               * aAllocExtRID,
                            scPageID            * aFstPIDOfAllocExt,
                            scPageID            * aAllocPID,
                            UChar              ** aAllocPagePtr )
{
    UChar          * sAllocPagePtr;
    sdpParentInfo    sParentInfo;

    IDE_ASSERT( aSegHandle    != NULL );
    IDE_ASSERT( aMtx          != NULL );
    IDE_ASSERT( aAllocExtRID  != NULL );
    IDE_ASSERT( aAllocPID     != NULL );
    IDE_ASSERT( aAllocPagePtr != NULL );
    IDE_ASSERT( aIsLogging    == ID_TRUE );

    IDE_TEST( sdpscExtDir::allocNewPageInExt( aStatistics,
                                              aMtx,
                                              aSpaceID,
                                              aSegHandle,
                                              aPrvAllocExtRID,
                                              aFstPIDOfPrvAllocExt,
                                              aPrvAllocPageID,
                                              aAllocExtRID,
                                              aFstPIDOfAllocExt,
                                              aAllocPID,
                                              &sParentInfo ) != IDE_SUCCESS );

    IDE_ERROR( *aAllocExtRID      != SD_NULL_RID );
    IDE_ERROR( *aFstPIDOfAllocExt != SD_NULL_PID );
    IDE_ERROR( *aAllocPID         != SD_NULL_PID );

    IDE_TEST( createPage( aStatistics,
                          aSpaceID,
                          *aAllocPID,
                          aPageType,
                          &sParentInfo,
                          (sdpscPageBitSet)
                          (SDPSC_BITSET_PAGETP_DATA|
                           SDPSC_BITSET_PAGEFN_FUL),
                          aMtx,
                          aMtx,
                          &sAllocPagePtr ) != IDE_SUCCESS );

    *aAllocPagePtr = sAllocPagePtr;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aAllocPagePtr = NULL;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : [ INTERFACE ] Append 방식의 Page 확보
 *
 * (1) 현재 Extent에 이전 할당받은 페이지 이후로 New Page가 존재한다면 SUCCESS!!
 * (2) 현재 Extent로부터 이전 할당받은 페이지가 마지막 Page였다면 다음 Extent가
 *     존재하는지 확인한다. 존재한다면 SUCCESS 그렇지 않으면 (3)
 * (3) 새로운 Extent를 할당한다. 할당했으면 SUCCESS, 그렇지 않으면 SpaceNotEnough
 *     에러 발생한다.
 *
 * aStatistics          - [IN] 통계 정보
 * aMtx                 - [IN] Mini Transaction Pointer
 * aSpaceID             - [IN] TableSpace ID
 * aSegHandle           - [IN] Segment Handle
 * aPrvAllocExtRID      - [IN] 이전에 Page를 할당받았던 Extent RID
 * aFstPIDOfPrvAllocExt - [IN] 이전에 Page를 할당받았던 Extent RID
 * aPrvAllocPageID      - [IN] 이전에 할당받은 PageID
 * aPageType            - [IN] 확보할 페이지 타입
 *
 ***********************************************************************/
IDE_RC sdpscAllocPage::prepareNewPage4Append(
                            idvSQL              * aStatistics,
                            sdrMtx              * aMtx,
                            scSpaceID             aSpaceID,
                            sdpSegHandle        * aSegHandle,
                            sdRID                 aPrvAllocExtRID,
                            scPageID              aFstPIDOfPrvAllocExt,
                            scPageID              aPrvAllocPageID )
{
    sdRID            sAllocExtRID;
    scPageID         sFstPIDOfAllocExt;
    scPageID         sAllocPID;
    sdpParentInfo    sParentInfo;

    IDE_ASSERT( aSegHandle    != NULL );
    IDE_ASSERT( aMtx          != NULL );

    /* 여기서는 페이지를 할당해서 반환하지 않고,
     * 할당할 수 있는 페이지 ID를 반환할뿐이다. */
    IDE_TEST( sdpscExtDir::allocNewPageInExt( aStatistics,
                                              aMtx,
                                              aSpaceID,
                                              aSegHandle,
                                              aPrvAllocExtRID,
                                              aFstPIDOfPrvAllocExt,
                                              aPrvAllocPageID,
                                              &sAllocExtRID,
                                              &sFstPIDOfAllocExt,
                                              &sAllocPID,
                                              &sParentInfo ) != IDE_SUCCESS );

    IDE_ERROR( sAllocExtRID      != SD_NULL_RID );
    IDE_ERROR( sFstPIDOfAllocExt != SD_NULL_PID );
    IDE_ERROR( sAllocPID         != SD_NULL_PID );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
