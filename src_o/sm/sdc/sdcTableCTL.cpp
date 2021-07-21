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
 * $Id$
 **********************************************************************/

# include <idl.h>

# include <smxTrans.h>
# include <smxTransMgr.h>

# include <sdpPhyPage.h>

# include <sdcRow.h>
# include <sdcTableCTL.h>


/**********************************************************************
 *
 * Description : 페이지 영역에 Touched Transaction Layer 초기화
 *
 * Touche Transaction Slot을 저장하는 영역을 관리한다.
 * 기타 구체적인 동작은 각 CTL 구현모듈에서 관리하며,
 * 본 모듈에서는 물리적인 영역에 대한 처리만을 관리한다.
 *
 * 페이지 구조상에서 보면 CTL 영역은 Logical Header 다음에 위치한다.
 * CTL 영역은 자동확장이 가능한 영역이다.
 *
 * Logical Header 다음에 위치하기 때문에 8바이트 Align 되며,
 * Header 부분도 8 바이트 Align 시킨다.
 *
 * aStatistics - [IN] 통계정보
 * aPageHdrPtr - [IN] 페이지 헤더 시작 포인터
 * aInitTrans  - [IN] 페이지내의 CTS 초기 개수
 * aMaxTrans   - [IN] 페이지내의 CTS 최대 개수
 *
 **********************************************************************/
IDE_RC sdcTableCTL::logAndInit( sdrMtx         * aMtx,
                                sdpPhyPageHdr  * aPageHdrPtr,
                                UChar            aInitTrans,
                                UChar            aMaxTrans )
{
    IDE_ERROR( aMtx        != NULL );
    IDE_ERROR( aPageHdrPtr != NULL );
    IDE_ERROR( aMaxTrans  <= SDP_CTS_MAX_CNT );

    init( aPageHdrPtr, aInitTrans, aMaxTrans );

    IDE_TEST( sdrMiniTrans::writeLogRec( aMtx,
                                         (UChar*)aPageHdrPtr,
                                         NULL,
                                         ID_SIZEOF( aInitTrans ) +
                                         ID_SIZEOF( aMaxTrans ),
                                         SDR_SDC_INIT_CTL )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   (UChar*)&(aInitTrans),
                                   ID_SIZEOF(aInitTrans) )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   (UChar*)&(aMaxTrans),
                                   ID_SIZEOF(aMaxTrans) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : Touched Transaction Layer 초기화
 *
 * 페이지의 Touched Transaction Layer를 초기화하면서
 * sdpPhyPageHdr의 FreeSize 및 각종 Offset을 설정한다.
 *
 * Physical Page에 초기 CTS 개수만큼 CTL 확장을 요청한다.
 *
 * 최대 CTS 개수는 페이지 생성시점에 결정되어진다.
 *
 * aPageHdrPtr - [IN] 페이지 헤더 시작 포인터
 * aInitTrans  - [IN] 페이지내의 CTS 초기 개수
 * aMaxTrans   - [IN] 페이지내의 CTS 최대 개수
 *
 ***********************************************************************/
void sdcTableCTL::init( sdpPhyPageHdr   * aPageHdrPtr,
                        UChar             aInitTrans,
                        UChar             aMaxTrans )
{
    sdpCTL   * sCTL;
    UChar      sCTSlotIdx;
    idBool     sTrySuccess = ID_FALSE;

    IDE_ASSERT( aPageHdrPtr != NULL );
    IDE_ASSERT( idlOS::align8( ID_SIZEOF( sdpCTL ) )
                == ID_SIZEOF( sdpCTL ) );

    sdpPhyPage::initCTL( aPageHdrPtr,
                         ID_SIZEOF( sdpCTL ),
                         (UChar**)&sCTL );

    sCTL->mMaxCTSCnt     = aMaxTrans;
    sCTL->mTotCTSCnt     = 0;
    sCTL->mBindCTSCnt    = 0;
    sCTL->mDelRowCnt     = 0;
    sCTL->mCandAgingCnt  = 0;
    sCTL->mRowBindCTSCnt = 0;

    /*
     * aSCN4Aging은 Self-Aging이 가능한 SCN을 나타내며, 확실하게
     * Self-Aging을 할 수 있는 Row Piece 들이 존재하지 않거나 몇개나
     * 존재할 지 모르는 경우에는 무한대값을 가진다.
     */
    SM_SET_SCN_INFINITE( &(sCTL->mSCN4Aging) );

    if ( aInitTrans > 0 )
    {
       IDE_ASSERT( extend( aPageHdrPtr, aInitTrans, &sCTSlotIdx, &sTrySuccess ) 
                   == IDE_SUCCESS);

       /* 페이지 생성시의 CTL 확장은 반드시 성공한다. 왜냐하면 빈공간이 많으니까 */
       IDE_ASSERT( sTrySuccess == ID_TRUE );
    }
}


/**********************************************************************
 * Description : 페이지 영역에 Touched Transaction Layer 초기화
 *
 * Touche Transaction Slot을 저장하는 영역을 관리한다.
 * 기타 구체적인 동작은 각 CTL 구현모듈에서 관리하며,
 * 본 모듈에서는 물리적인 영역에 대한 처리만을 관리한다.

 * 페이지 구조상에서 보면 CTL 영역은 Logical Header 다음에 위치한다.
 * CTL 영역은 자동확장이 가능한 영역이다.
 *
 * Logical Header 다음에 위치하기 때문에 8바이트 Align 되며,
 * Header 부분도 8 바이트 Align 시킨다.
 *
 * aStatistics    - [IN]  통계정보
 * aMtx           - [IN]  mtx 포인터
 * aExtendSlotCnt - [IN]  확장할 CTS 개수
 * aCTSlotIdx     - [OUT] 확장된 위치의 CTS 번호
 * aTrySuccess    - [OUT] 확장성공여부
 *
 **********************************************************************/
IDE_RC sdcTableCTL::logAndExtend( sdrMtx         * aMtx,
                                  sdpPhyPageHdr  * aPageHdrPtr,
                                  UChar            aExtendSlotCnt,
                                  UChar          * aCTSlotIdx,
                                  idBool         * aTrySuccess )
{
    UChar    sCTSlotIdx;
    idBool   sTrySuccess = ID_FALSE;

    IDE_ERROR( aMtx        != NULL );
    IDE_ERROR( aPageHdrPtr != NULL );

    IDE_ERROR( extend( aPageHdrPtr,
                       aExtendSlotCnt,
                       &sCTSlotIdx,
                       &sTrySuccess )
               == IDE_SUCCESS );

    IDE_TEST_CONT( sTrySuccess != ID_TRUE, CONT_ERR_EXTEND );

    IDE_TEST( sdrMiniTrans::writeLogRec( aMtx,
                                         (UChar*)aPageHdrPtr,
                                         NULL,
                                         ID_SIZEOF( aExtendSlotCnt ),
                                         SDR_SDC_EXTEND_CTL )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   (UChar*)&aExtendSlotCnt,
                                   ID_SIZEOF( aExtendSlotCnt ))
              != IDE_SUCCESS );

    IDE_EXCEPTION_CONT( CONT_ERR_EXTEND );

    *aTrySuccess = sTrySuccess;
    *aCTSlotIdx  = sCTSlotIdx;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aCTSlotIdx  = SDP_CTS_IDX_NULL;
    *aTrySuccess = ID_FALSE;

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description : Touched Transaction Layer 확장
 *
 * Data 페이지의 주어진 개수만큼 CTS를 확장한다. 만약 확장할 개수가 Max CTS 개수에
 * Align 되지 않는 경우는 실패할 수도 있다. 예를들어 현재 119개가 있는데 2개를 더
 * 확장해야하는 경우 max 120개라면 Error가 발생한다.
 * ( 참고로 Index는 2개씩 증가하고, Data 페이지는 1개씩 증가한다.)
 * Data Page에서는 저런 경우 실패할 일은 없다.
 *
 * aPageHdrPtr     - [IN]  Data 페이지 헤더
 * aExtendSlotCnt  - [IN]  확장할 CTS 개수
 * aCTSlotIdx      - [OUT] 확장된 위치의 CTS 번호
 * aTrySuccess     - [OUT] 확장성공여부
 *
 ***********************************************************************/
IDE_RC sdcTableCTL::extend( sdpPhyPageHdr   * aPageHdrPtr,
                            UChar             aExtendSlotCnt,
                            UChar           * aCTSlotIdx,
                            idBool          * aTrySuccess )
{
    UChar    i;
    sdpCTS * sCTS;
    sdpCTL * sCTL;
    UChar    sCTSlotIdx;
    idBool   sTrySuccess = ID_FALSE;

    IDE_ASSERT( aPageHdrPtr != NULL );
    IDE_ASSERT( idlOS::align8(ID_SIZEOF(sdpCTS)) == ID_SIZEOF(sdpCTS) );

    sCTSlotIdx = SDP_CTS_IDX_NULL;
    sCTL       = (sdpCTL*)sdpPhyPage::getCTLStartPtr( (UChar*)aPageHdrPtr );

    if ( (sCTL->mTotCTSCnt + aExtendSlotCnt) <= sCTL->mMaxCTSCnt )
    {
        IDE_ERROR( sdpPhyPage::extendCTL( aPageHdrPtr,
                                          aExtendSlotCnt,
                                          ID_SIZEOF( sdpCTS ),
                                          (UChar**)&sCTS,
                                          &sTrySuccess )
                  == IDE_SUCCESS );

        if ( sTrySuccess == ID_TRUE )
        {
            for ( i = 0; i < aExtendSlotCnt; i++ )
            {
                sCTS->mTSSPageID     = SD_NULL_PID;
                sCTS->mTSSlotNum     = 0;
                sCTS->mFSCredit      = 0;
                /* 확장이후로 한번도 할당된적이 없는 상태 */
                sCTS->mStat          = SDP_CTS_STAT_NUL;
                sCTS->mRefCnt        = 0;
                sCTS->mRefRowSlotNum[0] = SC_NULL_SLOTNUM;
                sCTS->mRefRowSlotNum[1] = SC_NULL_SLOTNUM;

                /* 할당되지 않은 경우에는 mFSCNOrCSCN은 0으로 초기화해준다. */
                SM_INIT_SCN( &sCTS->mFSCNOrCSCN );

                sCTS++;
            }

            sCTSlotIdx        = sCTL->mTotCTSCnt;
            sCTL->mTotCTSCnt += aExtendSlotCnt;
        }
    }
    else
    {
        sTrySuccess = ID_FALSE;
    }

    *aCTSlotIdx  = sCTSlotIdx;
    *aTrySuccess = sTrySuccess;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : CTS 할당 및 Set Dirty ( Wrapper-Function )
 *
 * 별도의 mtx로 로깅을 수행하면서, 페이지에 가용한 CTS를 할당한다.
 * 예를들어 FixMtx로 로깅이 되어 있는경우에는 데이타 페이지에 대한 Latch를 해제했다가
 * 다시 잡을 수 없으므로 별도의 Mtx로 처리한다.
 * 이와 다른 경우는 Data 페이지의 생성과 함께 CTS를 할당하는 경우인데, 이 경우는
 * 별도의 Mtx를 사용할 수 없다.
 *
 * aStatistics         - [IN]  통계정보
 * aFixMtx             - [IN]  데이타페이지에 X-Latch를 획득한 mtx 포인터
 * aStartInfo          - [IN]  별도의 Logging용 Mtx를 위한 mtx 시작정보
 * aPageHdrPtr         - [IN]  데이타페이지 헤더 시작 포인터
 * aCTSlotIdx          - [OUT] 할당된 CTS 번호
 *
 ***********************************************************************/
IDE_RC sdcTableCTL::allocCTSAndSetDirty( idvSQL          * aStatistics,
                                         sdrMtx          * aFixMtx,
                                         sdrMtxStartInfo * aStartInfo,
                                         sdpPhyPageHdr   * aPageHdrPtr,
                                         UChar           * aCTSlotIdx )
{
    sdrMtx          sLogMtx;
    UInt            sState = 0;

    IDE_ERROR( aPageHdrPtr != NULL );
    IDE_ERROR( aCTSlotIdx  != NULL );

    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sLogMtx,
                                   aStartInfo,
                                   ID_TRUE,/*MtxUndoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT )
              != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( allocCTS( aStatistics,
                        aFixMtx,
                        &sLogMtx,
                        SDB_SINGLE_PAGE_READ,
                        aPageHdrPtr,
                        aCTSlotIdx ) != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit( &sLogMtx ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback( &sLogMtx ) == IDE_SUCCESS );
    }

    *aCTSlotIdx = SDP_CTS_IDX_NULL;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : CTS 할당
 *
 * 페이지내의 CTS는 최대 CTS 개수까지 확장이 가능하지만 어쨌거나 유한한 자원이다.
 * CTS 할당 알고리즘은 최대한 확장을 자재하는 방법을 선택하면, 최대한 초기화되었던
 * CTS를 어떻게든 재사용할것을 고민한다.
 *
 * 다음과 같은 우선순위로 CTS를 할당한다.
 *
 * 1. CTS TimeStamping 되어 있는 CTS를 Row TimeStamping을
 *    수행하고, CTS를 재사용가능하게 한다
 *    적어도 한번 TotCTSCnt 개수만큼 모두 확인한다. 중간에 할당가능하다고 확인과정을
 *    멈추지는 않는다.
 *    Delayed Row Stamping 확인과정을 수행할 CTS들을 Cache 해둔다.
 *    암튼 모두 확인후에 Row Stamping된 제일 작은 번호를 가진 CTS를 할당한다.
 *
 * 2. Delayed Row Stamping으로 확인해볼 CTS들을 확인하여
 *    1개만이라도 할당가능하다면 확인과정을 종료하고 할당한다.
 *
 * 3. 이미 초기화된 NULL 상태의 CTS가 존재하다면, 할당한다.
 *
 * 4. CTL을 확장을 통해서 새롭게 초기화된 CTS를 할당해간다.
 *
 * aStatistics         - [IN]  통계정보
 * aFixMtx             - [IN]  페이지를 X-Latch로 fix한 Mtx
 * aLogMtx             - [IN]  Logging을 위한 Mtx
 * aPageReadMode       - [IN]  page read mode(SPR or MPR)
 * aPageHdrPtr         - [IN]  데이타페이지 헤더 시작 포인터
 * aCTSlotIdx          - [OUT] 할당된 CTS 번호
 *
 ***********************************************************************/
IDE_RC sdcTableCTL::allocCTS( idvSQL          * aStatistics,
                              sdrMtx          * aFixMtx,
                              sdrMtx          * aLogMtx,
                              sdbPageReadMode   aPageReadMode,
                              sdpPhyPageHdr   * aPageHdrPtr,
                              UChar           * aCTSlotIdx )
{
    sdpCTL        * sCTL;
    sdpCTS        * sCTS;
    UChar           sCTSIdx;
    UChar           sTotCnt;
    UChar           sAllocCTSIdx;
    sdSID           sTransTSSlotSID;
    smSCN           sFstDskViewSCN;
    UChar           sFstNullCTSIdx;
    idBool          sIsNeedSetDirty = ID_FALSE;
    UChar           sActCTSCnt;
    idBool          sTrySuccess = ID_FALSE;
    UChar           sArrActCTSIdx[ SDP_CTS_MAX_CNT ];
    smTID           sWait4TransID;
    smSCN           sRowCommitSCN;

    IDE_ERROR( aLogMtx     != NULL );
    IDE_ERROR( aPageHdrPtr != NULL );
    IDE_ERROR( aCTSlotIdx  != NULL );

    sTransTSSlotSID = smxTrans::getTSSlotSID( aLogMtx->mTrans );
    sFstDskViewSCN  = smxTrans::getFstDskViewSCN( aLogMtx->mTrans );

    sFstNullCTSIdx  = SDP_CTS_IDX_NULL;
    sAllocCTSIdx    = SDP_CTS_IDX_NULL;

    sCTL    = getCTL( aPageHdrPtr );
    sTotCnt = getCnt( sCTL );

    IDE_TEST_CONT( sTotCnt == 0, cont_skip_allocate_cts );

    /*
     * 1. CTS TimeStamping 되어 있는 CTS를 Row TimeStamping을
     *    수행하고, CTS를 재사용가능하게 한다.
     */
    for( sCTSIdx = 0, sActCTSCnt = 0; sCTSIdx < sTotCnt; sCTSIdx++ )
    {
        sCTS = getCTS( sCTL, sCTSIdx );

        // 'A', 'R', 'T', 'N', 'O'
        if ( hasState( sCTS->mStat, SDP_CTS_STAT_ACT ) == ID_TRUE )
        {
            if ( isMyTrans( &sTransTSSlotSID,
                            &sFstDskViewSCN,
                            sCTS ) == ID_TRUE )
            {
                // 'A'상태인 경우 자신이 이미 바인딩한 CTS를 찾으면 바로 할당한다.
                sAllocCTSIdx = sCTSIdx;
                break;
            }

            sArrActCTSIdx[sActCTSCnt++] = sCTSIdx;
        }
        else
        {
            // 'R', 'T', 'N', 'O'
            if ( hasState( sCTS->mStat, SDP_CTS_STAT_NUL ) == ID_TRUE )
            {
                // 이후로 모두 NUL 상태일 것이기 때문에 여기서
                // CTS 확인과정을 완료한다.
                sFstNullCTSIdx = sCTSIdx;
                break;
            }

            // 'R', 'T', 'O'
            if ( hasState( sCTS->mStat, SDP_CTS_STAT_CTS ) == ID_TRUE )
            {
                // Soft Row Stamping 'T' -> 'R'
                IDE_TEST( logAndRunRowStamping( aLogMtx,
                                                sCTSIdx,
                                                (void*)sCTS,
                                                sCTS->mFSCredit,
                                                &sCTS->mFSCNOrCSCN )
                          != IDE_SUCCESS );

                sIsNeedSetDirty = ID_TRUE;
            }
            else
            {
                // 'R', 'O' 상태만 가능하다
            }

            // RTS 된 CTS 중 가장 작은 CTS를 선택한다.
            if ( sAllocCTSIdx == SDP_CTS_IDX_NULL )
            {
                sAllocCTSIdx = sCTSIdx;
            }
        }
    }

    IDE_TEST_CONT( sAllocCTSIdx != SDP_CTS_IDX_NULL,
                    cont_success_allocate_cts );

    /*
     * 2. Delayed Row TimeStamping을 시도해볼 CTS가 존재하는 경우
     */
    for( sCTSIdx = 0; sCTSIdx < sActCTSCnt; sCTSIdx++ )
    {
        sCTS = getCTS( sCTL, sArrActCTSIdx[ sCTSIdx ] );

        IDE_ERROR( hasState( sCTS->mStat, SDP_CTS_STAT_ACT )
                    == ID_TRUE );

        // 'A' -> 'R' 상태로 변경시도
        IDE_TEST( logAndRunDelayedRowStamping(
                                  aStatistics,
                                  aLogMtx,
                                  sArrActCTSIdx[ sCTSIdx ],
                                  (void*)sCTS,
                                  aPageReadMode,
                                  &sWait4TransID,  
                                  &sRowCommitSCN ) != IDE_SUCCESS );

        // Row TimeStamping이 완료되었으므로 할당
        if ( hasState( sCTS->mStat, SDP_CTS_STAT_RTS ) == ID_TRUE )
        {
            sAllocCTSIdx    = sArrActCTSIdx[ sCTSIdx ];
            sIsNeedSetDirty = ID_TRUE;
            break;
        }
    }

    IDE_TEST_CONT( sAllocCTSIdx != SDP_CTS_IDX_NULL,
                    cont_success_allocate_cts );

    /*
     * 3. 정말로 재사용할 것이 존재하지 않는다면 NULL 상태의 CTS를 사용한다.
     */
    if ( sFstNullCTSIdx != SDP_CTS_IDX_NULL )
    {
        IDE_ERROR( sTotCnt > sFstNullCTSIdx );

        sAllocCTSIdx = sFstNullCTSIdx;
        IDE_CONT( cont_success_allocate_cts );
    }

    /*
     * 4. CTL 확장을 통하여 CTS를 확보한다.
     *
     *    CTS를 모두 검색하고도 가용한 CTS가 존재하지 않는다면,
     *    CTL 영역을 확장한다.
     */
    IDE_TEST( logAndExtend( aLogMtx,
                            aPageHdrPtr,
                            1,       /* aCTSCnt */
                            &sAllocCTSIdx,
                            &sTrySuccess ) != IDE_SUCCESS );

    if ( sTrySuccess == ID_TRUE )
    {
        sIsNeedSetDirty = ID_TRUE;
    }

    IDE_EXCEPTION_CONT( cont_success_allocate_cts );

    /* To Fix BUG-23241 [F1-valgrind] Insert시 AllocCTS과정에서
     * RowTimeStamping 한 후 SetDirtyPage를 수행하지 않아
     * 서버비정상종료시 구동실패
     * Page를 변경하고 SetDirtyPage를 하지 않으면, 로그LSN이
     * BCB에 반영되지 않아서 PageLSN이 설정되지 않는다.
     * insert 시에는 aFixMtx == NULL 이며, Logging용 aLogMtx를 별도로
     * 사용하기 때문에 setDirtyPage를 해주어야 한다.
     * 하지만, 그외 연산 update/delete 에서는 aFixMtx == aLogMtx이며,
     * x-latch를 획득하고 내려오기 때문에 setdirty를 해줄 필요없다 */
    if ( (sIsNeedSetDirty == ID_TRUE) && (aFixMtx != aLogMtx) )
    {
        IDE_TEST( sdrMiniTrans::setDirtyPage( aLogMtx, (UChar*)aPageHdrPtr )
                  != IDE_SUCCESS );
    }

    IDE_EXCEPTION_CONT( cont_skip_allocate_cts );

    *aCTSlotIdx = sAllocCTSIdx;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aCTSlotIdx = SDP_CTS_IDX_NULL;

    if ( (sIsNeedSetDirty == ID_TRUE) && (aFixMtx != aLogMtx) )
    {
       (void)sdrMiniTrans::setDirtyPage( aLogMtx, (UChar*)aPageHdrPtr );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : 자신이 바인딩한 CTS인지 판단한다.
 *
 * 다음과 같이 CTS에 기록된 TSSlotSID와 TransBSCN을 고려하여 트랜잭션 자신이
 * 바인딩한 CTS인지를 정확하게 판단할 수 있다.
 *
 * 일단 동일한 트랜잭션이라면 TSSlotSID는 동일해야하지만, TSS는 재사용이 가능하기 때문에
 * 다른 트랜잭션 간에도 동일할수 있다.
 * 그래서 CTS를 바인딩한 트랜잭션의 TransBSCN이 현재 트랜잭션과 동일한것도 비교해야하는데
 * 이것은 TransBSCN이 같은 경우에는 TSS 재사용되지 않았다는 것을 보장해주기 때문이다.
 *
 * aTransTSSlotSID - [IN] CTS에 기록된 TSSlotSID
 * aTransBSCN      - [IN] CTS에 기록된 바인딩한 트랜잭션의 Begin SCN 포인터
 * aPagePtr        - [IN] 데이타 페이지의 헤더 시작 포인터
 * aCTSlotIdx      - [IN] 대상 CTSlot 번호
 *
 ***********************************************************************/
#if 0 // not used
idBool sdcTableCTL::isMyTrans( sdSID         * aTransTSSlotSID,
                               smSCN         * aTransBSCN,
                               sdpPhyPageHdr * aPagePtr,
                               UChar           aTSSlotIdx )
{
    sdpCTS * sCTS;

    sCTS = getCTS( aPagePtr, aTSSlotIdx );

    if ( (SD_MAKE_PID( *aTransTSSlotSID )    == sCTS->mTSSPageID) &&
         (SD_MAKE_OFFSET( *aTransTSSlotSID ) == sCTS->mTSSlotNum) )
    {
        if ( SM_SCN_IS_EQ( &(sCTS->mFSCNOrCSCN), aTransBSCN ) )
        {
            return ID_TRUE;
        }
    }

    return ID_FALSE;
}
#endif

/***********************************************************************
 *
 * Description : 자신이 바인딩한 CTS인지 판단한다.
 *
 * 위의 sdcTableCTL::isMyTrans() 함수와는
 * 매개변수로 CTS index 대신 CTS pointer를 받는 것만 유일하게 다르다. 
 *
 ***********************************************************************/
idBool sdcTableCTL::isMyTrans( sdSID         * aTransTSSlotSID,
                               smSCN         * aTransBSCN,
                               sdpCTS        * aCTS )
{
    if ( ( SD_MAKE_PID( *aTransTSSlotSID )    == aCTS->mTSSPageID ) &&
         ( SD_MAKE_OFFSET( *aTransTSSlotSID ) == aCTS->mTSSlotNum ) )
    {
        if ( SM_SCN_IS_EQ( &(aCTS->mFSCNOrCSCN), aTransBSCN ) )
        {
            return ID_TRUE;
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

    return ID_FALSE;
}

/***********************************************************************
 *
 * Description : 트랜잭션 정보를 CTS에 기록하거나 Row에 바인딩한다.
 *
 * 갱신할 페이지에서 CTS를 할당한 경우에는 CTS에 트랜잭션 정보를 바인딩하고,
 * 할당받지 못한 경우에는 RowPiece내에 CTS 정보를 기록한다.
 *
 *
 * aStatistics         - [IN]  통계정보
 * aMtx                - [IN]  Mtx 포인터
 * aSpaceID            - [IN]  데이타페이지가 소속된 테이블스페이스 ID
 * aRowPiecePtr        - [IN]  갱신할 RowPiece 의 포인터
 * aRowCTSlotIdx       - [IN]  Row Piece의 CTS 번호 (이미 바인딩된 Row일수도 있고
 *                                             그렇지 않을 수도 있다. )
 * aAllocCTSlotIdx     - [IN]  트랜잭션이 갱신을 위해 할당한 CTS 번호
 * aRowSlotNum         - [IN]  CTS에 Caching할 RowPiece SlotEntry 번호
 * aFSCreditSize       - [IN]  페이지에 예약할 Free Space Credit 크기
 * aIncDelRowCnt       - [IN]  Delete 연산을 위한 바인딩인 경우 TRUE,
 *                             Delete Row 개수를 증가시킨다.
 **********************************************************************/
IDE_RC sdcTableCTL::bind( sdrMtx               * aMtx,
                          scSpaceID              aSpaceID,
                          UChar                * aNewSlotPtr,
                          UChar                  aOldCTSlotIdx,
                          UChar                  aNewCTSlotIdx,
                          scSlotNum              aRowSlotNum,
                          UShort                 aFSCreditSize4RowHdrEx,
                          SShort                 aNewFSCreditSize,
                          idBool                 aIncDelRowCnt )
{
    UChar     sOldCTSlotIdx;
    UChar     sNewCTSlotIdx;
    SShort    sNewFSCreditSize;

    IDE_ERROR( aNewSlotPtr != NULL );

    sOldCTSlotIdx = SDC_UNMASK_CTSLOTIDX( aOldCTSlotIdx );
    sNewCTSlotIdx = SDC_UNMASK_CTSLOTIDX( aNewCTSlotIdx );

    sNewFSCreditSize = aFSCreditSize4RowHdrEx;

    if( aNewFSCreditSize > 0 )
    {
        sNewFSCreditSize += aNewFSCreditSize;
    }

    if( SDC_HAS_BOUND_CTS(aNewCTSlotIdx) )
    {
#ifdef DEBUG
        if ( sOldCTSlotIdx != sNewCTSlotIdx )
        {
            IDE_ASSERT( !SDC_HAS_BOUND_CTS(sOldCTSlotIdx) ||
                        !SDC_HAS_BOUND_CTS(sNewCTSlotIdx) );
        }
#endif

        IDE_TEST( logAndBindCTS( aMtx,
                                 aSpaceID,
                                 sdpPhyPage::getHdr(aNewSlotPtr),
                                 sOldCTSlotIdx,
                                 sNewCTSlotIdx,
                                 aRowSlotNum,
                                 aNewFSCreditSize,
                                 aIncDelRowCnt )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_ERROR( sNewCTSlotIdx  == SDP_CTS_IDX_NULL );
        IDE_ERROR( (sOldCTSlotIdx == SDP_CTS_IDX_NULL) ||
                   (sOldCTSlotIdx == SDP_CTS_IDX_UNLK) );

        IDE_TEST( logAndBindRow( aMtx,
                                 aNewSlotPtr,
                                 sNewFSCreditSize,
                                 aIncDelRowCnt )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : CTS를 언바인딩하거나 혹은 RowPiece의 CTS 정보를 언바인딩한다.
 *
 * aMtx              - [IN] Mtx 포인터
 * aRowPieceBfrUndo  - [IN] 언바인딩할 RowPiece의 포인터
 * aCTSlotIdxBfrUndo - [IN] 언두하기 전에 Row Piece 헤더의 CTS 번호
 * aCTSlotIdxAftUndo - [IN] 언두한 후에 Row Piece 헤더의 CTS 번호
 *                          중복 갱신이 된경우는 언두하기전이나 후나 CTS 번호가
 *                          동일하면, 그렇지 않다면 SDP_CTS_IDX_UNLK 이어야 한다.
 * aFSCreditSize     - [IN] 페이지에 예약할 Free Space Credit 크기
 * aDecDelRowCnt     - [IN] Delete 연산을 위한 언바인딩인 경우 TRUE,
 *                          Delete Row 개수를 감소시킨다.
 *
 ***********************************************************************/
IDE_RC sdcTableCTL::unbind( sdrMtx         * aMtx,
                            UChar          * aRowPieceBfrUndo,
                            UChar            aCTSlotIdxBfrUndo,
                            UChar            aCTSlotIdxAftUndo,
                            SShort           aFSCreditSize,
                            idBool           aDecDelRowCnt )
{
    UChar  sCTSlotIdxBfrUndo;
    UChar  sCTSlotIdxAftUndo;

    IDE_ERROR( aRowPieceBfrUndo != NULL );

    sCTSlotIdxBfrUndo = SDC_UNMASK_CTSLOTIDX( aCTSlotIdxBfrUndo );
    sCTSlotIdxAftUndo = SDC_UNMASK_CTSLOTIDX( aCTSlotIdxAftUndo );

    if ( SDC_HAS_BOUND_CTS(sCTSlotIdxBfrUndo) )
    {
        /* 페이지에서 CTS를 할당받은 경우에는 RowHdr의 CTSIdx는
         * Undo 이전에는 0~125 범위의 유효한 값만 올수 있다. */
        IDE_TEST( logAndUnbindCTS( aMtx,
                                   sdpPhyPage::getHdr(aRowPieceBfrUndo),
                                   sCTSlotIdxBfrUndo,
                                   sCTSlotIdxAftUndo,
                                   aFSCreditSize,
                                   aDecDelRowCnt )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_ERROR( sCTSlotIdxBfrUndo  == SDP_CTS_IDX_NULL );
        IDE_ERROR( (sCTSlotIdxAftUndo == SDP_CTS_IDX_NULL) ||
                   (sCTSlotIdxAftUndo == SDP_CTS_IDX_UNLK) );

        IDE_TEST( logAndUnbindRow( aMtx,
                                   aRowPieceBfrUndo,
                                   aFSCreditSize,
                                   aDecDelRowCnt )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description : CTS에 바인딩한다.
 *
 * 데이타페이지에서 동일한 트랜잭션이 갱신한 RowPiece들은 하나의 CTS에만 바인딩 될
 * 수 있으며, 각각 서로 다른 Row Piece를 갱신하는 경우에는 CTS의 RefCnt를 증가시키는
 * 것만으로 바인딩연산을 수행한다. 만일 동일한 Row Piece를 중복 갱신한 경우에는 RefCnt
 * 를 증가시키지 않는다.
 * 만약 RefCnt를 갱신할때마다 증가시킨다면 중복 Row Piece에 대해서 수만번 갱신되는
 * 경우에 2바이트인 RefCnt로 충분하지 않을 것이다.
 *
 * 또한 바인딩과정에서 함께 처리하는 것이 2가지가 있는 갱신연산에 의해 예약되어야 할
 * Free Space Credit에 대한 처리와 Delete된 Row의 개수등이 그것이다.
 * 이는 별도의 로깅으로 바인딩 함수안에서 처리된다.
 *
 * aStatistics      - [IN] 통계정보
 * aMtx             - [IN] Mtx 포인터
 * aSpaceID         - [IN] 데이타페이지가 소속된 테이블스페이스 ID
 * aPageHdrPtr      - [IN] 데이타페이지의 헤더 시작 포인터
 * aOldCTSlotIdx    - [IN] Row Piece의 CTS 번호 (이미 바인딩된 Row일수도 있고
 *                                             그렇지 않을 수도 있다. )
 * aNewCTSlotIdx    - [IN] 트랜잭션이 갱신을 위해 할당한 CTS 번호
 * aRowSlotNum      - [IN] CTS에 Caching할 RowPiece SlotEntry 번호
 * aNewFSCreditSize - [IN] 페이지에 예약할 Free Space Credit 크기
 * aIncDelRowCnt    - [IN] Delete 연산을 위한 바인딩인 경우 TRUE,
 *                         Delete Row 개수를 증가시킨다.
 *
 ***********************************************************************/
IDE_RC sdcTableCTL::logAndBindCTS( sdrMtx         * aMtx,
                                   scSpaceID        aSpaceID,
                                   sdpPhyPageHdr  * aPageHdrPtr,
                                   UChar            aOldCTSlotIdx,
                                   UChar            aNewCTSlotIdx,
                                   scSlotNum        aRowSlotNum,
                                   SShort           aNewFSCreditSize,
                                   idBool           aIncDelRowCnt )
{
    sdpCTS        * sCTS;
    idBool          sIsActiveCTS;
    UInt            sLogSize;
    sdSID           sTSSlotSID;
    smSCN           sFstDskViewSCN;

    IDE_ERROR( aPageHdrPtr != NULL );

    sTSSlotSID = SD_NULL_SID;

    SM_INIT_SCN( &sFstDskViewSCN );

    sLogSize = ID_SIZEOF(UChar)     + // aRowCTSlotIdx
               ID_SIZEOF(UChar)     + // aNewCTSlotIdx
               ID_SIZEOF(scSlotNum) + // aSlotNum
               ID_SIZEOF(SShort)    + // aFSCreditSize
               ID_SIZEOF(idBool);     // aIncDelRowCnt

    sCTS         = getCTS( aPageHdrPtr, aNewCTSlotIdx );
    sIsActiveCTS = hasState( sCTS->mStat, SDP_CTS_STAT_ACT );

    if ( sIsActiveCTS == ID_FALSE )
    {
        // 해당 페이지에서 트랜잭션이 처음으로 CTS를 바인딩하는 경우이다.
        sLogSize += (ID_SIZEOF(smSCN) + ID_SIZEOF(sdSID));

        sFstDskViewSCN  = smxTrans::getFstDskViewSCN( aMtx->mTrans );
        sTSSlotSID      = smxTrans::getTSSlotSID( aMtx->mTrans );

        IDE_TEST( ((smxTrans*)aMtx->mTrans)->addTouchedPage(
                    aSpaceID,
                    aPageHdrPtr->mPageID,
                    aNewCTSlotIdx ) != IDE_SUCCESS );
    }
    else
    {
        /* CTS가 이미 트랜잭션에 바인딩 된 경우에는
           RefCnt, FSCredit와 DeleteCnt만 증가시킨다. */
    }

/*
    ideLog::log(IDE_SERVER_0, "<tid %d bind> pid(%d), ctslotidx(%x,o(%x)), slotnum(%d), tss(%d,%d)\n ",
                              smxTrans::getTransID( aMtx->mTrans ),
                              aPageHdrPtr->mPageID,
                              aNewCTSlotIdx,
                              aOldCTSlotIdx,
                              aRowSlotNum,
                              SD_MAKE_PID( sTSSlotSID),
                              SD_MAKE_OFFSET( sTSSlotSID) );
*/

    IDE_TEST( bindCTS4REDO( aPageHdrPtr,
                            aOldCTSlotIdx,
                            aNewCTSlotIdx,
                            aRowSlotNum,
                            &sFstDskViewSCN,
                            sTSSlotSID,
                            aNewFSCreditSize,
                            aIncDelRowCnt )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::writeLogRec(aMtx,
                                        (UChar*)sCTS,
                                        NULL,
                                        sLogSize,
                                        SDR_SDC_BIND_CTS )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   &aOldCTSlotIdx,
                                   ID_SIZEOF( UChar ) )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   &aNewCTSlotIdx,
                                   ID_SIZEOF( UChar ) )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   &aRowSlotNum,
                                   ID_SIZEOF( scSlotNum ) )
              != IDE_SUCCESS );

    if ( sIsActiveCTS == ID_FALSE )
    {
        IDE_TEST( sdrMiniTrans::write( aMtx,
                                       &sFstDskViewSCN,
                                       ID_SIZEOF( smSCN ) )
                  != IDE_SUCCESS );

        IDE_TEST( sdrMiniTrans::write( aMtx,
                                       &sTSSlotSID,
                                       ID_SIZEOF( sdSID ) )
                  != IDE_SUCCESS );
    }

    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   &aNewFSCreditSize,
                                   ID_SIZEOF( SShort ) )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   &aIncDelRowCnt,
                                   ID_SIZEOF( idBool ) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : Row에 바인딩한다.
 *
 * Parameters:
 *  aStatistics           - [IN] 통계정보
 *  aMtx                  - [IN] Mtx 포인터
 *  aNewSlotPtr           - [IN] 새로운 Rowpiece 포인터
 *  aFSCreditSizeToWrite  - [IN] 이번에 갱신으로 인해 예약해야할 FSCreditSize
 *  aIncDelRowCnt         - [IN] Delete 연산을 위한 바인딩인 경우 TRUE,
 *                               Delete Row 개수를 증가시킨다.
 *
 ***********************************************************************/
IDE_RC sdcTableCTL::logAndBindRow( sdrMtx           * aMtx,
                                   UChar            * aNewSlotPtr,
                                   SShort             aFSCreditSizeToWrite,
                                   idBool             aIncDelRowCnt )
{
    UInt    sLogSize;
    smSCN   sFstDskViewSCN;
    sdSID   sTSSlotSID;

    IDE_ERROR( aNewSlotPtr          != NULL );
    IDE_ERROR( aFSCreditSizeToWrite >= 0 );

    sLogSize = ( ID_SIZEOF(sdSID)  +
                 ID_SIZEOF(smSCN)  +
                 ID_SIZEOF(UShort) +
                 ID_SIZEOF(idBool) );

    sFstDskViewSCN = smxTrans::getFstDskViewSCN( aMtx->mTrans );
    sTSSlotSID     = smxTrans::getTSSlotSID( aMtx->mTrans );

    IDE_TEST( bindRow4REDO( aNewSlotPtr,
                            sTSSlotSID,
                            sFstDskViewSCN,
                            aFSCreditSizeToWrite,
                            aIncDelRowCnt )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::writeLogRec(aMtx,
                                        (UChar*)aNewSlotPtr,
                                        (UChar*)NULL,
                                        sLogSize,
                                        SDR_SDC_BIND_ROW )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   &sTSSlotSID,
                                   ID_SIZEOF(sdSID) )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   &sFstDskViewSCN,
                                   ID_SIZEOF(smSCN) )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   &aFSCreditSizeToWrite,
                                   ID_SIZEOF(UShort) )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   &aIncDelRowCnt,
                                   ID_SIZEOF(idBool) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : 트랜잭션이 Row를 CTS에 바인딩한다.
 *
 * 실제로 Row Piece 헤더에 CTS 번호를 기록하는 것으로 바인딩작업이 수행되어야 하지만,
 * CTS 바인딩 함수 호출 이후에 sdcRow::writeRowHdr() 함수를 통해서 바인딩된 CTS
 * 번호를 Row에 기록한다. 즉, 본 함수에서는 CTS에 대한 바인딩 연산만 수행한다.
 *
 * aPageHdrPtr       - [IN] 데이타페이지 헤더 시작 포인터
 * aOldCTSlotIdx     - [IN] 이전에 바인딩된 CTS 번호
 * aNewCTSlotIdx     - [IN] 바인딩할 CTS 번호
 * aRowSlotNum       - [IN] CTS에 Caching할 RowPiece SlotEntry 번호
 * aFstDskViewSCN    - [IN] 트랜잭션의 첫번째 Disk Stmt의 ViewSCN
 * aTSSlotSID        - [IN] 바인딩할 트랜잭션의 TSSlot SID
 * aNewFSCreditSize  - [IN] 페이지에 예약할 Free Space Credit 크기
 * aIncDelRowCnt     - [IN] Delete 연산을 위한 바인딩인 경우 TRUE,
 *                          Delete Row 개수를 증가시킨다.
 *
 ***********************************************************************/
IDE_RC sdcTableCTL::bindCTS4REDO( sdpPhyPageHdr * aPageHdrPtr,
                                  UChar           aOldCTSlotIdx,
                                  UChar           aNewCTSlotIdx,
                                  scSlotNum       aRowSlotNum,
                                  smSCN         * aFstDskViewSCN,
                                  sdSID           aTSSlotSID,
                                  SShort          aNewFSCreditSize,
                                  idBool          aIncDelRowCnt )
{
    sdpCTL  * sCTL;
    sdpCTS  * sCTS;
    UShort    sRefCnt;

    IDE_ERROR( aPageHdrPtr   != NULL );
    IDE_ERROR( aNewCTSlotIdx != SDP_CTS_IDX_NULL );

    sCTL = getCTL( aPageHdrPtr );
    sCTS = getCTS( aPageHdrPtr, aNewCTSlotIdx );

    // 이미 CTS에 바인딩된 Row를 다시 갱신하는 경우에는
    // CTS를 바인딩 하지 않는다.
    IDE_TEST_CONT( aOldCTSlotIdx == aNewCTSlotIdx,
                    cont_already_bound );

    if( hasState( sCTS->mStat, SDP_CTS_STAT_ACT ) == ID_TRUE )
    {
        // 이미 바인딩되어 있는 경우에는 RefCnt만 증가시킨다.
        sRefCnt = sCTS->mRefCnt;
        sRefCnt++;
    }
    else
    {
        // 해당 페이지에서 최초 바인딩인 경우

        // 'N', 'R', 'O'
        IDE_ERROR( hasState( sCTS->mStat, SDP_CTS_SS_FREE )
                   == ID_TRUE );

        sCTS->mStat      = SDP_CTS_STAT_ACT;
        sRefCnt          = 1;
        sCTS->mTSSPageID    = SD_MAKE_PID( aTSSlotSID );
        sCTS->mTSSlotNum = SD_MAKE_OFFSET( aTSSlotSID );

        SM_SET_SCN( &(sCTS->mFSCNOrCSCN), aFstDskViewSCN );

        sCTS->mRefRowSlotNum[0] = SC_NULL_SLOTNUM;
        sCTS->mRefRowSlotNum[1] = SC_NULL_SLOTNUM;

        /* 바인딩 CTS 개수는 최초 바인딩될때 증가시키고, 트랜잭션이 롤백이
         * 발생하여 페이지내에서의 마지막 언바인딩을 수행할때 감소시킨다.
         * 커밋된 경우에 Row Stamping이 발생하는 경우에도 감소된다. */
        incBindCTSCntOfCTL( sCTL );
    }

    /* Row Piece에 대한 Slot Entry 번호는 2개정도만 Cache한다.
     * 과연 한페이지당 일반적으로 2개의 Row Piece정도만 갱신하는 것이
     * 일반적인가?? 에 대한 의혹이 제기되지만, 2개 이하일 경우가 있다면
     * Row Stamping시에 Slot Directory를 FullScan하는 경우가 제거되므로
     * 이러한 자료구조가 설계되었다. 참고로 오라클에는 없는 자료구조이다. */
    if ( sRefCnt <= SDP_CACHE_SLOT_CNT )
    {
        sCTS->mRefRowSlotNum[ sRefCnt-1 ] = aRowSlotNum;
    }

    sCTS->mRefCnt = sRefCnt;

    IDE_EXCEPTION_CONT( cont_already_bound );

    if ( aNewFSCreditSize > 0 )
    {
        incFSCreditOfCTS( sCTS, aNewFSCreditSize );
    }

    if ( aIncDelRowCnt == ID_TRUE )
    {
        incDelRowCntOfCTL( sCTL );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : 트랜잭션이 Row에 바인딩한다.
 *
 * aNewSlotPtr               - [IN] Realloc한 새로운 RowPiece의 포인터
 * aTSSlotSID                - [IN] 바인딩할 트랜잭션의 TSSlot SID
 * aFstDskViewSCN            - [IN] 트랜잭션의 첫번째 Disk Stmt의 ViewSCN
 * aFSCreditSizeToWrite      - [IN] RowExCTSInfo에 누적된 FSCreditSize
 * aIncDelRowCnt             - [IN] Delete 연산을 위한 바인딩인 경우 TRUE,
 *                                  Delete Row 개수를 증가시킨다.
 *
 ***********************************************************************/
IDE_RC sdcTableCTL::bindRow4REDO( UChar      * aNewSlotPtr,
                                  sdSID        aTSSlotSID,
                                  smSCN        aFstDskViewSCN,
                                  SShort       aFSCreditSizeToWrite,
                                  idBool       aIncDelRowCnt )
{
    sdpCTL          * sCTL;
    sdcRowHdrExInfo   sRowHdrExInfo;

    IDE_ERROR( aNewSlotPtr         != NULL );
    IDE_ERROR( aFSCreditSizeToWrite >= 0 );

    SDC_INIT_ROWHDREX_INFO( &sRowHdrExInfo,
                            aTSSlotSID,
                            aFSCreditSizeToWrite,
                            aFstDskViewSCN )

    sCTL = getCTL( sdpPhyPage::getHdr(aNewSlotPtr) );
    incBindRowCTSCntOfCTL( sCTL );

    sdcRow::writeRowHdrExInfo( aNewSlotPtr, &sRowHdrExInfo );

    if ( aIncDelRowCnt == ID_TRUE )
    {
        incDelRowCntOfCTL( sCTL );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description : CTS 언방이딩
 *
 * 트랜잭션이 롤백이 되면 바인딩한 CTS를 언바인딩하면서 데이타페이지에 갱신했던
 * Row Piece에 대한 Undo를 수행한다.
 *
 * aMtx              - [IN] Mtx 포인터
 * aPageHdrPtr       - [IN] 데이타페이지의 헤더 시작 포인터
 * aCTSlotIdxBfrUndo - [IN] 언두하기 전에 Row Piece 헤더의 CTS 번호
 * aCTSlotIdxAftUndo - [IN] 언두한 후에 Row Piece 헤더의 CTS 번호
 *                          중복 갱신이 된경우는 언두하기전이나 후나 CTS 번호가
 *                          동일하면, 그렇지 않다면 SDP_CTS_IDX_UNLK 이어야 한다.
 * aFSCreditSize     - [IN] 페이지에 예약할 Free Space Credit 크기
 * aDecDelRowCnt     - [IN] Delete 연산을 위한 언바인딩인 경우 TRUE,
 *                          Delete Row 개수를 감소시킨다.
 *
 ***********************************************************************/
IDE_RC sdcTableCTL::logAndUnbindCTS( sdrMtx        * aMtx,
                                     sdpPhyPageHdr * aPageHdrPtr,
                                     UChar           aCTSlotIdxBfrUndo,
                                     UChar           aCTSlotIdxAftUndo,
                                     SShort          aFSCreditSize,
                                     idBool          aDecDelRowCnt )
{
    UInt  sLogSize;

    sLogSize = (ID_SIZEOF(UChar) * 2)       + // CTSlotIdx * 2
                ID_SIZEOF(aFSCreditSize)    +
                ID_SIZEOF(aDecDelRowCnt);

    IDE_TEST( unbindCTS4REDO( aPageHdrPtr,
                              aCTSlotIdxBfrUndo,
                              aCTSlotIdxAftUndo,
                              aFSCreditSize,
                              aDecDelRowCnt )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::writeLogRec(aMtx,
                                        (UChar*)aPageHdrPtr,
                                        NULL,
                                        sLogSize,
                                        SDR_SDC_UNBIND_CTS )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   &aCTSlotIdxBfrUndo,
                                   ID_SIZEOF(aCTSlotIdxBfrUndo) )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   &aCTSlotIdxAftUndo,
                                   ID_SIZEOF(aCTSlotIdxAftUndo) )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   &aFSCreditSize,
                                   ID_SIZEOF(aFSCreditSize) )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   &aDecDelRowCnt,
                                   ID_SIZEOF(aDecDelRowCnt) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : 트랜잭션이 CTS를 Row로부터 언바인딩 한다.
 *
 * 중복 갱신에 대한 Undo가 아닌경우에는 CTS 바인딩시 증가시켰던 RefCnt를
 * 감소시킨다. 또한 바인딩시 CTS에 예약해두었던 Free Space Credit가 존재한다면,
 * CTS의 누적 FSC로부터 해당 트랜잭션의 언두과정에서 복원했던 Free Space Credit
 * 만큼 빼준다.
 *
 * aPageHdrPtr       - [IN] 데이타페이지의 헤더 시작 포인터
 * aCTSlotIdxBfrUndo - [IN] 언두하기 전에 Row Piece 헤더의 CTS 번호
 * aCTSlotIdxAftUndo - [IN] 언두한 후에 Row Piece 헤더의 CTS 번호
 * aFSCreditSize     - [IN] 페이지에 예약할 Free Space Credit 크기
 *
 ***********************************************************************/
IDE_RC sdcTableCTL::unbindCTS4REDO( sdpPhyPageHdr   * aPageHdrPtr,
                                    UChar             aCTSlotIdxBfrUndo,
                                    UChar             aCTSlotIdxAftUndo,
                                    SShort            aFSCreditSize,
                                    idBool            aDecDelRowCnt )
{
    sdpCTL         * sCTL;
    sdpCTS         * sCTS;

    IDE_ERROR( aPageHdrPtr       != NULL );
    IDE_ERROR( aCTSlotIdxBfrUndo != SDP_CTS_IDX_UNLK );

    sCTL = getCTL( aPageHdrPtr );
    sCTS = getCTS( aPageHdrPtr, aCTSlotIdxBfrUndo );

    IDE_ERROR( sCTS->mRefCnt > 0 );
    IDE_ERROR( hasState( sCTS->mStat, SDP_CTS_STAT_ACT ) == ID_TRUE );

    // Undo가 되어서 이전 Row Piece가 다른 트랜잭션에 의해 Commit된
    // 경우에는 RefCnt를 감소시킨다.
    if( aCTSlotIdxAftUndo != aCTSlotIdxBfrUndo )
    {
        // BUG-27718 5.3.3 Dog Food] 제일저축은행 BMT시나리오에서 startup실패
        IDE_ERROR( (aCTSlotIdxAftUndo == SDP_CTS_IDX_UNLK) ||
                   (aCTSlotIdxAftUndo == SDP_CTS_IDX_NULL) );
        sCTS->mRefCnt--;
    }
    else
    {
        // 그렇지 않은 경우는 자신이 동일한 Row Piece를 중복 갱신한
        // 것이여서 RefCnt를 증가시키지 않았으므로,감소시키지도
        // 않는다. 단, FSCredit는 감소시켜야한다.
        IDE_ERROR( aCTSlotIdxAftUndo != SDP_CTS_IDX_UNLK );
    }

    if ( aDecDelRowCnt == ID_TRUE )
    {
        decDelRowCntOfCTL( sCTL );
    }

    if ( aFSCreditSize > 0 )
    {
        IDE_ERROR( sCTS->mFSCredit >= aFSCreditSize );

        IDE_TEST( restoreFSCredit( aPageHdrPtr, aFSCreditSize ) 
                  != IDE_SUCCESS );

        decFSCreditOfCTS( sCTS, aFSCreditSize );
    }

    decBindCTSCntOfCTL( sCTL, sCTS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : Unbind Row
 *
 * 트랜잭션이 롤백이 되면 바인딩한 CTS를 언바인딩하면서 데이타페이지에 갱신했던
 * Row Piece에 대한 Undo를 수행한다.
 *
 * aMtx              - [IN] Mtx 포인터
 * aRowPieceBfrUndo  - [IN] 데이타페이지의 헤더 시작 포인터
 * aFSCreditSize     - [IN] 페이지에 예약할 Free Space Credit 크기
 * aDecDelRowCnt     - [IN] Delete 연산을 위한 언바인딩인 경우 TRUE,
 *                          Delete Row 개수를 감소시킨다.
 *
 ***********************************************************************/
IDE_RC sdcTableCTL::logAndUnbindRow( sdrMtx      * aMtx,
                                     UChar       * aRowPieceBfrUndo,
                                     SShort        aFSCreditSize,
                                     idBool        aDecDelRowCnt )
{
    UInt  sLogSize;

    sLogSize = ID_SIZEOF(aFSCreditSize) + ID_SIZEOF(aDecDelRowCnt);

    IDE_TEST( unbindRow4REDO( aRowPieceBfrUndo,
                              aFSCreditSize,
                              aDecDelRowCnt )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::writeLogRec(aMtx,
                                        (UChar*)aRowPieceBfrUndo,
                                        NULL,
                                        sLogSize,
                                        SDR_SDC_UNBIND_ROW )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   &aFSCreditSize,
                                   ID_SIZEOF(aFSCreditSize) )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   &aDecDelRowCnt,
                                   ID_SIZEOF(aDecDelRowCnt) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : Row bound CTS를 unbound한다.
 *
 * aRowPieceBfrUndo  - [IN] 데이타페이지의 헤더 시작 포인터
 * aCTSlotIdxBfrUndo - [IN] 언두하기 전에 Row Piece 헤더의 CTS 번호
 * aCTSlotIdxAftUndo - [IN] 언두한 후에 Row Piece 헤더의 CTS 번호
 * aFSCreditSize     - [IN] 페이지에 예약할 Free Space Credit 크기
 *
 ***********************************************************************/
IDE_RC sdcTableCTL::unbindRow4REDO( UChar     * aRowPieceBfrUndo,
                                  SShort      aFSCreditSize,
                                  idBool      aDecDelRowCnt )
{
    sdpCTL         * sCTL;
    sdpPhyPageHdr  * sPageHdrPtr;
    UShort           sFSCreditSize;

    IDE_ERROR( aRowPieceBfrUndo != NULL );

    SDC_GET_ROWHDR_FIELD( aRowPieceBfrUndo,
                          SDC_ROWHDR_FSCREDIT,
                          &sFSCreditSize );

    sPageHdrPtr = sdpPhyPage::getHdr(aRowPieceBfrUndo);

    sCTL = getCTL( sPageHdrPtr );
    decBindRowCTSCntOfCTL( sCTL );

    if ( aDecDelRowCnt == ID_TRUE )
    {
        decDelRowCntOfCTL( sCTL );
    }

    if ( aFSCreditSize > 0 )
    {
        IDE_ERROR( sFSCreditSize >= aFSCreditSize );

        IDE_TEST( restoreFSCredit( sPageHdrPtr, aFSCreditSize )
                  != IDE_SUCCESS );
        sFSCreditSize -= aFSCreditSize;

        SDC_SET_ROWHDR_FIELD( aRowPieceBfrUndo,
                              SDC_ROWHDR_FSCREDIT,
                              &sFSCreditSize );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*********************************************************************
 *
 * Description : Page에 대하여 Fast CTS TimeStamping 수행
 *
 * 트랜잭션 커밋과정에서 No-Logging 모드로 CTS에 트랜잭션의 CommitSCN을
 * 설정한다. 트랜잭션이 방문했던 페이지에 대해서 재방문을 하는 것인데,
 * 만약 Partial Rollback등으로 인해서 방문했던 페이지에 자신에 해당하는
 * CTS가 없을 수도 있으므로 반드시 자신의 CTS인지여부를 판단한 후 Stamping을
 * 수행한다.
 *
 * aTransTSSlotSID- [IN] 트랜잭션의 TSSlotSID 의 포인터
 * aFstDskViewSCN - [IN] 트랜잭션의 첫번째 Disk Stmt의 ViewSCN
 * aPagePtr       - [IN] 페이지 헤더 시작 포인터
 * aCTSlotIdx     - [IN] 트랜잭션이 DML연산으로 방문했을때 바인딩했던 CTS 번호
 * aCommitSCN     - [IN] 트랜잭션이 할당한 CommitSCN의 포인터
 *
 *********************************************************************/
IDE_RC sdcTableCTL::runFastStamping( sdSID         * aTransTSSlotSID,
                                     smSCN         * aFstDskViewSCN,
                                     sdpPhyPageHdr * aPagePtr,
                                     UChar           aCTSlotIdx,
                                     smSCN         * aCommitSCN )
{
    sdpCTS  * sCTS;

    sCTS = getCTS( (sdpPhyPageHdr*)aPagePtr, aCTSlotIdx );

    IDE_ERROR( SM_SCN_IS_NOT_INIT( *aCommitSCN ) );

    /* 자신이 바인딩했던 CTS는 ACT 상태이다. 물론 다른 트랜잭션이 사용중이라도
     * ACT 상태이므로 자신의 것인지 반드시 확인한다. */
    if( hasState( sCTS->mStat, SDP_CTS_STAT_ACT ) == ID_TRUE )
    {
        if( isMyTrans( aTransTSSlotSID,
                       aFstDskViewSCN,
                       sCTS ) == ID_TRUE )
        {
            SM_SET_SCN( &(sCTS->mFSCNOrCSCN), aCommitSCN );
            sCTS->mStat = SDP_CTS_STAT_CTS;
        }
    }
    else
    {
        // ACT가 아닌경우는 자신이 바인딩했다가 Partial Rollback으로 인해
        // 언바인딩한 후에 ROL 상태로 남아 있거나, 다른 트랜잭션이 사용후
        // Stamping을 수행한 경우이다.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 *
 * Description : DRDBRedo Valdation을 위해 FastStamping을 시도합니다.
 *
 * 1) Stamping을 했느냐 하지 않았느냐에 따라 SCN이 다를 수 있기 때문이니다.
 * 2) Stamping 되었어도 다시 Stamping을 시도합니다. TSS가 완전히 재활용
 *    되어 CommitSCN이 0일 수도 있기 때문입니다.
 * 3) 두 페이지를 동시에 Stamping합니다. 두번에 걸쳐 Stamping하면
 *    getCommitSCN의 타이밍에 따라, 한쪽은 Stamping 되면서 다른 한쪽은
 *    Stamping 안될 수도 있기 때문입니다.
 *
 * aStatistics    - [IN] Dummy통계정보
 * aPagePtr1      - [IN] 페이지 헤더 시작 포인터
 * aPagePtr2      - [IN] 페이지 헤더 시작 포인터
 *
 *********************************************************************/
IDE_RC sdcTableCTL::stampingAll4RedoValidation( 
                                            idvSQL           * aStatistics,
                                            UChar            * aPagePtr1,
                                            UChar            * aPagePtr2 )
{
    sdpCTL          * sCTL1;
    sdpCTS          * sCTS1;
    sdpCTL          * sCTL2;
    sdpCTS          * sCTS2;
    smTID             sDummyTID4Wait;
    UInt              sTotCTSCnt;
    UChar             sIdx;
    sdSID             sTSSlotSID;
    smSCN             sRowCommitSCN;
    SInt              sSlotIdx;
    UChar           * sSlotDirPtr1;
    UChar           * sSlotDirPtr2;
    UChar           * sSlotPtr;
    UShort            sTotSlotCnt;
    UChar             sCTSlotIdx;
    sdcRowHdrExInfo   sRowHdrExInfo;
 
    /************************************************************************
     * CTSBoundChangeInfo에 대해 FastStamping 시도
     ***********************************************************************/
    sCTL1                = getCTL( (sdpPhyPageHdr*)aPagePtr1);
    sCTL2                = getCTL( (sdpPhyPageHdr*)aPagePtr2);

    IDE_ERROR( sCTL1->mTotCTSCnt == sCTL2->mTotCTSCnt );

    sTotCTSCnt           = sCTL1->mTotCTSCnt;
    for ( sIdx = 0; sIdx < sTotCTSCnt; sIdx++ )
    {
        sCTS1  = getCTS( sCTL1, sIdx );
        sCTS2  = getCTS( sCTL2, sIdx );

        /* Stamping 된 상태라도, 다시 Stampnig을 합니다.
         * 왜냐하면 TSS가 완전히 재활용되어 CommtSCN이 0인 상태일 수도
         * 있기 때문입니다. */
        if( ( hasState( sCTS1->mStat, SDP_CTS_STAT_ACT ) == ID_TRUE ) ||
            ( hasState( sCTS1->mStat, SDP_CTS_STAT_CTS ) == ID_TRUE ) )
        {
            IDE_ERROR( hasState( sCTS2->mStat, SDP_CTS_STAT_ACT ) || 
                       hasState( sCTS2->mStat, SDP_CTS_STAT_CTS ) );

            sTSSlotSID = SD_MAKE_SID( sCTS1->mTSSPageID, sCTS1->mTSSlotNum );

            IDE_ERROR( sTSSlotSID == 
                       SD_MAKE_SID( sCTS2->mTSSPageID, sCTS2->mTSSlotNum ) );

            IDE_TEST( sdcTSSegment::getCommitSCN( aStatistics,
                                                  NULL,        /* aTrans */
                                                  sTSSlotSID,
                                                  &(sCTS1->mFSCNOrCSCN),
                                                  SM_SCN_INIT, /* aStmtViewSCN */
                                                  &sDummyTID4Wait,
                                                  &sRowCommitSCN )
                      != IDE_SUCCESS );

            if ( SM_SCN_IS_NOT_INFINITE( sRowCommitSCN ) )
            {
                SM_SET_SCN( &(sCTS1->mFSCNOrCSCN), &sRowCommitSCN );
                sCTS1->mStat  = SDP_CTS_STAT_CTS;

                SM_SET_SCN( &(sCTS2->mFSCNOrCSCN), &sRowCommitSCN );
                sCTS2->mStat  = SDP_CTS_STAT_CTS;
            }
            /* 원래 쓰레기값이 들어가도 무방하다. 하지만 RedoValidation에서는
             * 무의미한 Diff가 생긴다. */
            sCTS1->mAlign = 0;
            sCTS2->mAlign = 0;
        }
        else
        {
            /* RowTimeStamping등 재활용 가능한 상태면 불필요한 diff를 없애기
             * 위해 초기화 한다. */
            idlOS::memset( sCTS1, 0, ID_SIZEOF( sdpCTS ) );
            sCTS1->mStat          = SDP_CTS_STAT_NUL;

            idlOS::memset( sCTS2, 0, ID_SIZEOF( sdpCTS ) );
            sCTS2->mStat          = SDP_CTS_STAT_NUL;
        }
    }

    /************************************************************************
     * RowBoundChangeInfo에 대해 Stamping을 시도한다.
     ***********************************************************************/
    sSlotDirPtr1 = sdpPhyPage::getSlotDirStartPtr(aPagePtr1);
    sTotSlotCnt = sdpSlotDirectory::getCount(sSlotDirPtr1);
    sSlotDirPtr2 = sdpPhyPage::getSlotDirStartPtr(aPagePtr2);

    IDE_TEST( sTotSlotCnt != sdpSlotDirectory::getCount( sSlotDirPtr2 ) );

    /* Transaction 정보가 Row에 bound 되었을 경우를 고려하여 모든 Slot을
     * 돌면서 Stamping을 수행한다. */
    for( sSlotIdx = 0; sSlotIdx < sTotSlotCnt; sSlotIdx++ )
    {
        if( sdpSlotDirectory::isUnusedSlotEntry(sSlotDirPtr1, sSlotIdx)
            == ID_TRUE )
        {
            continue;
        }

        IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr1,
                                                           sSlotIdx,
                                                           &sSlotPtr )
                  != IDE_SUCCESS );

        SDC_GET_ROWHDR_1B_FIELD( sSlotPtr,
                                 SDC_ROWHDR_CTSLOTIDX,
                                 sCTSlotIdx );

        /* BoundRow이면 */
        if( !SDC_HAS_BOUND_CTS(sCTSlotIdx) )
        {
            sdcRow::getRowHdrExInfo( sSlotPtr, &sRowHdrExInfo );

            sTSSlotSID    = SD_MAKE_SID( sRowHdrExInfo.mTSSPageID,
                                         sRowHdrExInfo.mTSSlotNum );

            /* Stamping 된 상태라도, 다시 Stampnig을 합니다.
             * 왜냐하면 TSS가 완전히 재활용되어 CommtSCN이 0인 상태일 수도
             * 있기 때문입니다. 
             * 다만 TSS가 Null인 것은 이미 RowStamping도 된것이기 때문에
             * 무시합니다. */
            //if( SDC_CTS_SCN_IS_NOT_COMMITTED( sRowHdrExInfo.mFSCNOrCSCN) )
            if( sTSSlotSID != SD_NULL_SID )
            {
                IDE_TEST( sdcTSSegment::getCommitSCN( 
                                                    aStatistics,
                                                    NULL,        /* aTrans */
                                                    sTSSlotSID,
                                                    &sRowHdrExInfo.mFSCNOrCSCN,
                                                    SM_SCN_INIT, /* aStmtViewSCN */
                                                    &sDummyTID4Wait,
                                                    &sRowCommitSCN )
                          != IDE_SUCCESS );

                if ( SM_SCN_IS_NOT_INFINITE(sRowCommitSCN) )
                {
                    /* Page1에 대해 */
                    SDC_SET_ROWHDR_FIELD( sSlotPtr,
                                          SDC_ROWHDR_FSCNORCSCN,
                                          &sRowCommitSCN );

                    /* Page2에 대해 */
                    IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( 
                                                                sSlotDirPtr2,
                                                                sSlotIdx,
                                                                &sSlotPtr )
                              != IDE_SUCCESS );
                    SDC_SET_ROWHDR_FIELD( sSlotPtr,
                                          SDC_ROWHDR_FSCNORCSCN,
                                          &sRowCommitSCN );
                }
            }
        }
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*********************************************************************
 *
 * Description : Delayed CTS TimeStamping 수행
 *
 * Delayed Row TimeStamping은 Row의 GetValidVersion이나 canUpdateRowPiece
 * 연산과정에서 Row의 CommitSCN을 알고자 할때 수행된다.
 * 본 연산은 Row와 연관된 일반적으로 TSS에 방문하여 해당 Row를 갱신한 트랜잭션의
 * 상태를 보고 완료가 된 경우에 No-Logging 모드로 CTS TimeStamping을
 * 수행한다. Delayed의 의미는 갱신트랜잭션이 직접 커밋과정에서 수행한
 * CTS TimeStamping이 아니라, 이 후의 다른 트랜잭션에 의해서 지연되어 TimeStamping
 * 이 된다는 것이다.
 *
 * aStatistics    - [IN] 통계정보
 * aTrans         - [IN] 자신의 TX의 포인터
 * aCTSlotIdx     - [IN]
 * aObjPtr        - [IN] CTS 포인터 혹은 RowPiece 포인터
 * aPageReadMode  - [IN] page read mode(SPR or MPR)
 * aStmtViewSCN   - [IN] 올바른 버전의 row를 읽기 위한 Statement viewscn
 * aTrySuccess    - [OUT] Delayed CTS TimeStamping의 성공여부
 * aWait4TransID  - [OUT] 대기해야할 대상 트랜잭션의 ID
 * aRowCommitSCN  - [OUT] 대상 트랜잭션의 CommitSCN
 * aFSCreditSize  - [OUT] CTS의 누적 FSCreditSize
 *
 *********************************************************************/
IDE_RC sdcTableCTL::runDelayedStamping( idvSQL           * aStatistics,
                                        void             * aTrans,
                                        UChar              aCTSlotIdx,
                                        void             * aObjPtr,
                                        sdbPageReadMode    aPageReadMode,
                                        smSCN              aStmtViewSCN,
                                        idBool           * aTrySuccess,
                                        smTID            * aWait4TransID,
                                        smSCN            * aRowCommitSCN,
                                        SShort           * aFSCreditSize )
{
    IDE_ERROR( aObjPtr       != NULL );
    IDE_ERROR( aTrySuccess   != NULL );
    IDE_ERROR( aWait4TransID != NULL );
    IDE_ERROR( aRowCommitSCN != NULL );
    IDE_ERROR( aFSCreditSize != NULL );

    if ( SDC_HAS_BOUND_CTS(aCTSlotIdx) )
    {
        IDE_TEST( runDelayedStampingOnCTS( aStatistics,
                                           aTrans,
                                           (sdpCTS*)aObjPtr,
                                           aPageReadMode,
                                           aStmtViewSCN,
                                           aTrySuccess,
                                           aWait4TransID,
                                           aRowCommitSCN,
                                           aFSCreditSize )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_ERROR( aCTSlotIdx == SDP_CTS_IDX_NULL );

        IDE_TEST( runDelayedStampingOnRow( aStatistics,
                                           aTrans,
                                           (UChar*)aObjPtr,
                                           aPageReadMode,
                                           aStmtViewSCN,
                                           aTrySuccess,
                                           aWait4TransID,
                                           aRowCommitSCN,
                                           aFSCreditSize )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*********************************************************************
 * Description : 대상 페이지의 모든 slot들에 대하여 Delayed TimeStamping을
 *               수행한다.
 *
 * BUG-48353 : 이 함수는 Stamping 또는 캐쉬 페이지를 만들때만 사용한다.
 *             따라서 Pending Wait를 하지않는다.
 *
 * aStatistics         - [IN] 통계정보
 * aTrans              - [IN] 자신의 TX의 포인터
 * aPageptr            - [IN] 페이지 시작 포인터
 * aPageReadMode       - [IN] page read mode(SPR or MPR)
 *********************************************************************/
IDE_RC sdcTableCTL::runDelayedStampingAll( idvSQL          * aStatistics,
                                           void            * aTrans,
                                           UChar           * aPagePtr,
                                           sdbPageReadMode   aPageReadMode )
{
    smSCN       sMyFstDskViewSCN;
    sdSID       sMyTSSlotSID;
    UShort      sSlotIdx;   /* BUG-39748 */
    UChar     * sSlotDirPtr;
    UChar     * sSlotPtr;
    UShort      sTotSlotCnt;
    UChar       sCTSlotIdx;
    SShort      sFSCreditSize;
    smSCN       sRowCSCN;
    smSCN       sFstDskViewSCN;
    idBool      sTrySuccess;
    smTID       sDummyTID4Wait;
    sdpCTL    * sCTL;
    UInt        sTotCTSCnt;
    UChar       sIdx;
    sdpCTS    * sCTS;

    IDE_ERROR( aPagePtr != NULL );

    /* CTSBoundChangeInfo에 대해 Stamping을 먼저 시도한다. */
    sCTL                 = getCTL( (sdpPhyPageHdr*)aPagePtr );
    sTotCTSCnt           = sCTL->mTotCTSCnt;
    for( sIdx = 0; sIdx < sTotCTSCnt; sIdx++ )
    {
        sCTS  = getCTS( sCTL, sIdx );
        if( hasState( sCTS->mStat, SDP_CTS_STAT_ACT ) == ID_TRUE )
        {
            IDE_TEST( runDelayedStampingOnCTS( aStatistics,
                                               NULL,
                                               sCTS,
                                               aPageReadMode,
                                               SM_SCN_INIT,
                                               &sTrySuccess,
                                               &sDummyTID4Wait,
                                               &sRowCSCN,
                                               &sFSCreditSize )
                      != IDE_SUCCESS );
        }
    }

    /* TASK-6105 
     * Row Bind CTS가 존재할때만 Row에 bound된 Transaction정보를 Stamping한다 */
    if ( sCTL->mRowBindCTSCnt != 0)
    {
        /* RowBoundChangeInfo에 대해 Stamping을 시도한다. */
        sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr(aPagePtr);
        sTotSlotCnt = sdpSlotDirectory::getCount(sSlotDirPtr);

        sMyFstDskViewSCN  = smxTrans::getFstDskViewSCN( aTrans );
        sMyTSSlotSID = smxTrans::getTSSlotSID( aTrans );

        /* Transaction 정보가 Row에 bound 되었을 경우를 고려하여 모든 Slot을
         * 돌면서 Stamping을 수행한다. */
        for ( sSlotIdx = 0; sSlotIdx < sTotSlotCnt; sSlotIdx++ )
        {
            if( sdpSlotDirectory::isUnusedSlotEntry(sSlotDirPtr, sSlotIdx)
                == ID_TRUE )
            {
                continue;
            }
 
            IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr,
                                                               sSlotIdx,   
                                                               &sSlotPtr )
                      != IDE_SUCCESS );
 
            SDC_GET_ROWHDR_1B_FIELD( sSlotPtr, SDC_ROWHDR_CTSLOTIDX, sCTSlotIdx );
            SDC_GET_ROWHDR_FIELD( sSlotPtr, SDC_ROWHDR_FSCNORCSCN, &sRowCSCN );
 
            if ( (sCTSlotIdx == SDP_CTS_IDX_NULL) &&
                 SDC_CTS_SCN_IS_NOT_COMMITTED(sRowCSCN) )
            {
                if( sdcRow::isMyTransUpdating( aPagePtr,
                                               sSlotPtr,
                                               sMyFstDskViewSCN,
                                               sMyTSSlotSID,
                                               &sFstDskViewSCN )
                    == ID_FALSE )
 
                {
                    IDE_TEST( runDelayedStampingOnRow( aStatistics,
                                                       NULL,
                                                       sSlotPtr,
                                                       aPageReadMode,
                                                       SM_SCN_INIT,
                                                       &sTrySuccess,
                                                       &sDummyTID4Wait,
                                                       &sRowCSCN,
                                                       &sFSCreditSize )
                              != IDE_SUCCESS );
                }
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 *
 * Description : Delayed CTS TimeStamping 수행
 *
 * Delayed Row TimeStamping은 Row의 GetValidVersion이나 canUpdateRowPiece
 * 연산과정에서 Row의 CommitSCN을 알고자 할때 수행된다.
 * 본 연산은 Row와 연관된 일반적으로 TSS에 방문하여 해당 Row를 갱신한 트랜잭션의
 * 상태를 보고 완료가 된 경우에 No-Logging 모드로 CTS TimeStamping을
 * 수행한다. Delayed의 의미는 갱신트랜잭션이 직접 커밋과정에서 수행한
 * CTS TimeStamping이 아니라, 이 후의 다른 트랜잭션에 의해서 지연되어 TimeStamping
 * 이 된다는 것이다.
 *
 * aStatistics    - [IN] 통계정보
 * aTrans         - [IN] 자신의 TX의 포인터
 * aCTS           - [IN] CTS 포인터
 * aPageReadMode  - [IN] page read mode(SPR or MPR)
 * aStmtViewSCN   - [IN] 올바른 버전의 row를 읽기 위한 Statement viewscn
 * aTrySuccess    - [OUT] Delayed CTS TimeStamping의 성공여부
 * aWait4TransID  - [OUT] 대기해야할 대상 트랜잭션의 ID
 * aRowCommitSCN  - [OUT] 대상 트랜잭션의 CommitSCN
 * aFSCreditSize  - [OUT] CTS의 누적 FSCreditSize
 *
 *********************************************************************/
IDE_RC sdcTableCTL::runDelayedStampingOnCTS( idvSQL           * aStatistics,
                                             void             * aTrans,
                                             sdpCTS           * aCTS,
                                             sdbPageReadMode    aPageReadMode,
                                             smSCN              aStmtViewSCN,
                                             idBool           * aTrySuccess,
                                             smTID            * aWait4TransID,
                                             smSCN            * aRowCommitSCN,
                                             SShort           * aFSCreditSize )
{
    sdSID         sTSSlotSID;
    UChar       * sPageHdr;
    idBool        sTrySuccess = ID_FALSE;

    *aFSCreditSize  = aCTS->mFSCredit;

    sTSSlotSID = SD_MAKE_SID( aCTS->mTSSPageID, aCTS->mTSSlotNum );
    IDE_ERROR( SDC_CTS_SCN_IS_NOT_COMMITTED(aCTS->mFSCNOrCSCN) );

    IDE_TEST( sdcTSSegment::getCommitSCN( aStatistics,
                                          aTrans,
                                          sTSSlotSID,
                                          &(aCTS->mFSCNOrCSCN),
                                          aStmtViewSCN,
                                          aWait4TransID,
                                          aRowCommitSCN )
              != IDE_SUCCESS );

    if ( SM_SCN_IS_NOT_INFINITE( *aRowCommitSCN ) )
    {
        sPageHdr = sdpPhyPage::getPageStartPtr( aCTS );

        /*
         * GetValidVersion 연산시에는 페이지에 S-latch 가 획득
         * 되어 있는 경우에는 X-latch로 조정해준다.
         * (내부적으로 풀고,다시 잡는다, 단, 나 자신만 페이지에 latch를
         * 획득한 경우에 한해서이다. )
         */
        sdbBufferMgr::tryEscalateLatchPage( aStatistics,
                                            sPageHdr,
                                            aPageReadMode,
                                            &sTrySuccess );

        if( sTrySuccess == ID_TRUE )
        {
            sdbBufferMgr::setDirtyPageToBCB( aStatistics,
                                             sPageHdr );
            aCTS->mStat = SDP_CTS_STAT_CTS;
            SM_SET_SCN( &(aCTS->mFSCNOrCSCN), aRowCommitSCN );

            *aTrySuccess = ID_TRUE;
        }
        else
        {
            *aTrySuccess = ID_FALSE;
        }
    }
    else
    {
        *aTrySuccess = ID_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 *
 * Description : Delayed Row TimeStamping 수행
 *
 * Delayed Row TimeStamping은 Row의 GetValidVersion이나 canUpdateRowPiece
 * 연산과정에서 Row의 CommitSCN을 알고자 할때 수행된다.
 * 본 연산은 Row와 연관된 일반적으로 TSS에 방문하여 해당 Row를 갱신한 트랜잭션의
 * 상태를 보고 완료가 된 경우에 No-Logging 모드로 CTS TimeStamping을
 * 수행한다. Delayed의 의미는 갱신트랜잭션이 직접 커밋과정에서 수행한
 * CTS TimeStamping이 아니라, 이 후의 다른 트랜잭션에 의해서 지연되어 TimeStamping
 * 이 된다는 것이다.
 *
 * aStatistics    - [IN] 통계정보
 * aTrans         - [IN] 자신의 TX의 포인터
 * aRowSlotPtr    - [IN] RowSlotPtr
 * aPageReadMode  - [IN] page read mode(SPR or MPR)
 * aStmtViewSCN   - [IN] 올바른 버전의 row를 읽기 위한 Statement viewscn
 * aTrySuccess    - [OUT] Delayed CTS TimeStamping의 성공여부
 * aWait4TransID  - [OUT] 대기해야할 대상 트랜잭션의 ID
 * aRowCommitSCN  - [OUT] 대상 트랜잭션의 CommitSCN
 * aFSCreditSize  - [OUT] RowPiece의 Extra 중 누적 FSCreditSize
 *
 *********************************************************************/
IDE_RC sdcTableCTL::runDelayedStampingOnRow( idvSQL           * aStatistics,
                                             void             * aTrans,
                                             UChar            * aRowSlotPtr,
                                             sdbPageReadMode    aPageReadMode,
                                             smSCN              aStmtViewSCN,
                                             idBool           * aTrySuccess,
                                             smTID            * aWait4TransID,
                                             smSCN            * aRowCommitSCN,
                                             SShort           * aFSCreditSize )
{
    sdSID            sTSSlotSID;
    UChar          * sPageHdr;
    idBool           sTrySuccess = ID_FALSE;
    sdcRowHdrExInfo  sRowHdrExInfo;
    sdpCTL         * sCTL;

    IDE_ERROR( aRowSlotPtr   != NULL );
    IDE_ERROR( aTrySuccess   != NULL );
    IDE_ERROR( aWait4TransID != NULL );
    IDE_ERROR( aRowCommitSCN != NULL );
    IDE_ERROR( aFSCreditSize != NULL );

    sdcRow::getRowHdrExInfo( aRowSlotPtr, &sRowHdrExInfo );

    IDE_DASSERT( SDC_CTS_SCN_IS_NOT_COMMITTED(sRowHdrExInfo.mFSCNOrCSCN) );

    *aFSCreditSize = sRowHdrExInfo.mFSCredit;
    sTSSlotSID     = SD_MAKE_SID( sRowHdrExInfo.mTSSPageID,
                                  sRowHdrExInfo.mTSSlotNum );

    IDE_TEST( sdcTSSegment::getCommitSCN( aStatistics,
                                          aTrans,
                                          sTSSlotSID,
                                          &sRowHdrExInfo.mFSCNOrCSCN,
                                          aStmtViewSCN,
                                          aWait4TransID,
                                          aRowCommitSCN )
              != IDE_SUCCESS );

    if( SM_SCN_IS_NOT_INFINITE(*aRowCommitSCN) )
    {
        sPageHdr = sdpPhyPage::getPageStartPtr( aRowSlotPtr );

        /*
         * GetValidVersion 연산시에는 페이지에 S-latch 가 획득
         * 되어 있는 경우에는 X-latch로 조정해준다.
         * (내부적으로 풀고,다시 잡는다, 단, 나 자신만 페이지에 latch를
         * 획득한 경우에 한해서이다. )
         */
        sdbBufferMgr::tryEscalateLatchPage( aStatistics,
                                            sPageHdr,
                                            aPageReadMode,
                                            &sTrySuccess );

        if( sTrySuccess == ID_TRUE )
        {
            sdbBufferMgr::setDirtyPageToBCB( aStatistics,
                                             sPageHdr );

            SDC_SET_ROWHDR_FIELD( aRowSlotPtr,
                                  SDC_ROWHDR_FSCNORCSCN,
                                  aRowCommitSCN );

            sCTL = getCTL( (sdpPhyPageHdr*)sPageHdr );
            decBindRowCTSCntOfCTL( sCTL );
            
            *aTrySuccess = ID_TRUE;
        }
        else
        {
            *aTrySuccess = ID_FALSE;
        }
    }
    else
    {
        *aTrySuccess = ID_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 *
 * Description : Delayed Row TimeStamping 및 로깅 수행
 *
 * CTS가 Active 상태인 경우에 CTS를 소유한 트랜잭션이 커밋을 했다면
 * CommitSCN을 판독하여 CTS에 Nologging으로 설정하고
 * (Delayed CTS TimeStamping) Row TimeStamping을 수행한다.
 *
 * aStatistics    - [IN] 통계정보
 * aMtx           - [IN] Mtx 포인터
 * aCTSlotIdx     - [IN] TimeStamping할 CTS 번호
 * aObjPtr        - [IN] CTS 포인터 혹은 RowPiece 포인터
 * aPageReadMode  - [IN] page read mode(SPR or MPR)
 * aWait4TransID  - [OUT] 대기해야할 대상 트랜잭션의 ID
 * aRowCommitSCN  - [OUT] 대상 트랜잭션의 CommitSCN
 *
 *********************************************************************/
IDE_RC sdcTableCTL::logAndRunDelayedRowStamping(
                         idvSQL          * aStatistics,
                         sdrMtx          * aMtx,
                         UChar             aCTSlotIdx,
                         void            * aObjPtr,
                         sdbPageReadMode   aPageReadMode,
                         smTID           * aWait4TransID,
                         smSCN           * aRowCommitSCN )
{
    SShort   sFSCreditSize;
    idBool   sTrySuccess;

    IDE_ERROR( aMtx    != NULL );
    IDE_ERROR( aObjPtr != NULL );

    IDE_TEST( runDelayedStamping( aStatistics,
                                  NULL,        /* aTrans */
                                  aCTSlotIdx,
                                  aObjPtr,
                                  aPageReadMode,
                                  SM_SCN_INIT, /* aStmtViewSCN */
                                  &sTrySuccess,
                                  aWait4TransID,
                                  aRowCommitSCN,
                                  &sFSCreditSize )
              != IDE_SUCCESS );

    if ( sTrySuccess == ID_TRUE )
    {
        IDE_TEST( logAndRunRowStamping( aMtx,
                                        aCTSlotIdx,
                                        aObjPtr,
                                        sFSCreditSize,
                                        aRowCommitSCN )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 *
 * Description : Row TimeStamping 및 로깅 수행
 *
 * Row TimeStamping은 CTS TimeStamping이 되어 있다는 전제하에 Logging 모드로
 * 수행된다. Row와 CTS의 연결관계를 정리하면서, CTS에 기록된 CommitSCN을
 * RowPiece 헤더에 기록하는 일련의 작업을 Row TimeStamping이라고 한다.
 *
 * aStatistics    - [IN] 통계정보
 * aMtx           - [IN] Mtx 포인터
 * aCTSlotIdx     - [IN] TimeStamping할 CTS 번호
 * aObjPtr        - [IN] CTS 포인터 혹은 RowSlotPtr
 * aFSCreditSize  - [IN] 누적된 FSCreditSize
 * aCommitSCN     - [IN] Stamping할 CommitSCN
 *
 *********************************************************************/
IDE_RC sdcTableCTL::logAndRunRowStamping( sdrMtx       * aMtx,
                                          UChar          aCTSlotIdx,
                                          void         * aObjPtr,
                                          SShort         aFSCreditSize,
                                          smSCN        * aCommitSCN )
{
    IDE_ERROR( aMtx    != NULL );
    IDE_ERROR( aObjPtr != NULL );
    IDE_ERROR( SM_SCN_IS_NOT_INFINITE(*aCommitSCN) == ID_TRUE );

    if( aFSCreditSize > 0 )
    {
        IDE_TEST( restoreFSCredit( aMtx,
                                   sdpPhyPage::getHdr((UChar*)aObjPtr),
                                   aFSCreditSize )
                  != IDE_SUCCESS );
    }

    /* Logging을 했으면, setDirty를 무조건 해야 함. */
    IDE_TEST( sdrMiniTrans::setDirtyPage( aMtx, (UChar*)aObjPtr )
              != IDE_SUCCESS );

    if( SDC_HAS_BOUND_CTS(aCTSlotIdx) == ID_TRUE )
    {
        IDE_TEST( runRowStampingOnCTS( (sdpCTS*)aObjPtr, 
                                       aCTSlotIdx, 
                                       aCommitSCN )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_DASSERT( aCTSlotIdx == SDP_CTS_IDX_NULL );

        IDE_TEST( runRowStampingOnRow( (UChar*)aObjPtr, 
                                       aCTSlotIdx, 
                                       aCommitSCN )
                  != IDE_SUCCESS );
    }

    IDE_TEST( sdrMiniTrans::writeLogRec( aMtx,
                                         (UChar*)aObjPtr,
                                         NULL,
                                         ID_SIZEOF( UChar ) +
                                         ID_SIZEOF( smSCN ),
                                         SDR_SDC_ROW_TIMESTAMPING )
            != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   &aCTSlotIdx,
                                   ID_SIZEOF( UChar ) )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   aCommitSCN,
                                   ID_SIZEOF( smSCN ) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 *
 * Description : Row TimeStamping 수행
 *
 * TimeStamping을 수행할 CTS와 연결된 모든 Row에 대해서 연결관계를 정리하고,
 * CommitSCN을 RowPiece 헤더에 설정한다. 일반적으로 CTS와 연결된 RowPiece의
 * 개수가 3개이상이면 SlotDirectory를 순차검색하여 관련 RowPiece를 찾아야하며,
 * 2개(SDP_CACHE_SLOT_CNT)이하인경우에는 Cache된 Slot Entry 번호를 통해서 바로 찾아간다.
 *
 * 만약 Row가 Delete가 되었다면, CommitSCN에 Delete Bit을 설정하여 RowPiece
 * 헤더에 기록하며, 해당 페이지에 바인딩된 CTS의 개수는 하나가 감소하게 된다.
 *
 * Row TimeStamping된 CTS의 상태는 'R'(RTS)이다
 *
 * aCTS           - [IN] CTS 포인터
 * aCTSlotIdx     - [IN] TimeStamping할 CTS 번호
 * aCommitSCN     - [IN] RowPiece 헤더에 설정할 CommitSCN 포인터
 *
 *********************************************************************/
IDE_RC sdcTableCTL::runRowStampingOnCTS( sdpCTS     * aCTS,
                                         UChar        aCTSlotIdx,
                                         smSCN      * aCommitSCN )
{
    UInt                i;
    UShort              sSlotSeq;
    UShort              sTotSlotCnt;
    UShort              sSlotCnt;
    UChar             * sSlotDirPtr;
    UChar             * sSlotPtr;
    UShort              sRefCnt;
    UChar             * sPagePtr;
    UChar               sCTSlotIdx;
    smSCN               sCommitSCN;
    sdpCTL            * sCTL;
    sdcRowHdrExInfo     sRowHdrExInfo;

    IDE_ERROR( aCTS          != NULL );
    IDE_ERROR( aCTS->mRefCnt != 0 );

    /* FIT/ART/sm/Bugs/BUG-23648/BUG-23648.tc */
    IDU_FIT_POINT( "1.BUG-23648@sdcTableCTL::runRowStampingOnCTS" );

    sPagePtr = sdpPhyPage::getPageStartPtr((UChar*)aCTS);
    SM_SET_SCN( &sCommitSCN, aCommitSCN );

    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr(sPagePtr);
    sTotSlotCnt = sdpSlotDirectory::getCount(sSlotDirPtr);
    sRefCnt     = aCTS->mRefCnt;

    sSlotCnt = ( sRefCnt > SDP_CACHE_SLOT_CNT ) ? sTotSlotCnt : sRefCnt;

    for( i = 0; i < sSlotCnt; i++ )
    {
        if( sRefCnt <= SDP_CACHE_SLOT_CNT )
        {
            sSlotSeq = aCTS->mRefRowSlotNum[i];
        }
        else
        {
            sSlotSeq = i;

            if( sdpSlotDirectory::isUnusedSlotEntry( sSlotDirPtr, sSlotSeq ) 
                == ID_TRUE )
            {
                continue;
            }
        }

        IDE_ERROR( sTotSlotCnt > sSlotSeq );

        IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr, 
                                                           sSlotSeq,
                                                           &sSlotPtr )
                  != IDE_SUCCESS );

        SDC_GET_ROWHDR_1B_FIELD( sSlotPtr,
                                 SDC_ROWHDR_CTSLOTIDX,
                                 sCTSlotIdx );

        sCTSlotIdx = SDC_UNMASK_CTSLOTIDX( sCTSlotIdx );

        if( sCTSlotIdx == aCTSlotIdx )
        {
            sCTSlotIdx = SDP_CTS_IDX_UNLK;

            SDC_SET_ROWHDR_1B_FIELD( sSlotPtr,
                                     SDC_ROWHDR_CTSLOTIDX,
                                     sCTSlotIdx );

            SDC_STAMP_ROWHDREX_INFO( &sRowHdrExInfo,
                                     sCommitSCN );

            sdcRow::writeRowHdrExInfo( sSlotPtr, &sRowHdrExInfo );

            IDE_ERROR( aCTS->mRefCnt > 0 );

            aCTS->mRefCnt--;
        }
    }

    IDE_ERROR( aCTS->mRefCnt == 0 );

    // clear reference slot number
    aCTS->mRefRowSlotNum[0] = SC_NULL_SLOTNUM;
    aCTS->mRefRowSlotNum[1] = SC_NULL_SLOTNUM;
    aCTS->mFSCredit      = 0;
    aCTS->mStat          = SDP_CTS_STAT_RTS;

    sCTL = getCTL( (sdpPhyPageHdr*)sPagePtr );

    decBindCTSCntOfCTL( sCTL, aCTS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 *
 * Description : RowExCTS에 대한 Row TimeStamping 수행
 *
 * aSlotPtr       - [IN] RowSlot 포인터
 * aCTSlotIdx     - [IN] TimeStamping할 CTS 번호
 * aCommitSCN     - [IN] RowPiece 헤더에 설정할 CommitSCN 포인터
 *
 *********************************************************************/
IDE_RC sdcTableCTL::runRowStampingOnRow( UChar      * aSlotPtr,
                                         UChar        aCTSlotIdx,
                                         smSCN      * aCommitSCN )
{
    UChar           sCTSlotIdx;
    smSCN           sCommitSCN;
    sdcRowHdrExInfo sRowHdrExInfo;

    IDE_ERROR( aSlotPtr   != NULL );
    IDE_ERROR( aCTSlotIdx == SDP_CTS_IDX_NULL );

    SM_GET_SCN( &sCommitSCN, aCommitSCN );

    sCTSlotIdx = SDP_CTS_IDX_UNLK;

    SDC_SET_ROWHDR_1B_FIELD( aSlotPtr,
                             SDC_ROWHDR_CTSLOTIDX,
                             sCTSlotIdx );

    SDC_STAMP_ROWHDREX_INFO( &sRowHdrExInfo,
                             sCommitSCN );

    sdcRow::writeRowHdrExInfo(aSlotPtr, &sRowHdrExInfo);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *  
 * Description : 데이타페이지에 예약해두었던 Free Space Credit 반환
 *
 * 트랜잭션이 커밋된 경우이거나 롤백하는 경우 예약해두었던 Free Space Credit를
 * 반환한다. 커밋한 경우에는 반환된 공간이 가용공간으로써 다른 트랜잭션이 사용할
 수               
 * 있게되며, 롤백한 경우에는 반환된 공간이 자신이 롤백할때 필요한 가용공간으로
 * 사용된다.                  
 * 즉, 데이타에 대한 갱신으로 인해서 이전 RowPiece 크기보다 더 작게 갱신되는 경>우에
 * Free Space Credit가 예약이 된다.
 *  
 * aMtx          - [IN] Mtx 포인터
 * aPageHdrPtr   - [IN] 페이지 헤더 시작 포인터
 * aRestoreSize  - [IN] 페이지에 반환할 가용공간의 크기 (FreeSpaceCredit)
 *  
 **********************************************************************/
IDE_RC sdcTableCTL::restoreFSCredit( sdrMtx         * aMtx,
                                     sdpPhyPageHdr  * aPageHdrPtr,
                                     SShort           aRestoreSize )
{       
    UShort     sNewAFS;             
                                    
    IDE_ASSERT( aPageHdrPtr != NULL );
    IDE_ASSERT( aRestoreSize > 0 );
    IDE_ASSERT( aRestoreSize < (SShort)SD_PAGE_SIZE );

    sNewAFS = sdpPhyPage::getAvailableFreeSize( aPageHdrPtr ) +
              aRestoreSize; 
        
    IDE_TEST( sdpPhyPage::logAndSetAvailableFreeSize(
                                aPageHdrPtr,
                                sNewAFS,
                                aMtx) != IDE_SUCCESS );
        
    return IDE_SUCCESS;
            
    IDE_EXCEPTION_END;
        
    return IDE_FAILURE;
}       

/***********************************************************************
 *
 * Description : 데이타페이지에 예약해두었던 Free Space Credit 반환
 *
 * 트랜잭션이 커밋된 경우이거나 롤백하는 경우 예약해두었던 Free Space Credit를
 * 반환한다. 커밋한 경우에는 반환된 공간이 가용공간으로써 다른 트랜잭션이 사용할 수
 * 있게되며, 롤백한 경우에는 반환된 공간이 자신이 롤백할때 필요한 가용공간으로
 * 사용된다.
 * 즉, 데이타에 대한 갱신으로 인해서 이전 RowPiece 크기보다 더 작게 갱신되는 경우에
 * Free Space Credit가 예약이 된다.
 *
 * aPageHdrPtr   - [IN] 페이지 헤더 시작 포인터
 * aRestoreSize  - [IN] 페이지에 반환할 가용공간의 크기 (FreeSpaceCredit)
 *
 **********************************************************************/
IDE_RC sdcTableCTL::restoreFSCredit( sdpPhyPageHdr  * aPageHdrPtr,
                                     SShort           aRestoreSize )
{
    UShort     sNewAFS;

    IDE_ERROR( aPageHdrPtr     != NULL );
    IDE_ERROR( aRestoreSize > 0 );
    IDE_ERROR( aRestoreSize < (SShort)SD_PAGE_SIZE );

    sNewAFS = sdpPhyPage::getAvailableFreeSize( aPageHdrPtr ) +
              aRestoreSize;

    sdpPhyPage::setAvailableFreeSize( aPageHdrPtr, sNewAFS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description : Target Row Piece에 대한 Row Stamping을 반드시 수행
 *
 * Head RowPiece의 경우에는 CanUpdateRowPiece 과정에서 반드시 Row Stamping이
 * 완료된 후 해당 Row에 대한 갱신연산이 수행됨을 보장하지만, Head RowPiece가 아닌
 * 경우에 대해서는 CanUpdateRowPiece 단계가 생략되기 때문에 (왜냐하면, Head
 * RowPiece에 대해서 한번만 수행하면 되기 때문이다) Row Stamping을 해주기 위해
 * 해당 함수를 호출한다.
 *
 * 이미 Row Stamping이 완료된 RowPiece인 경우에는 IDE_SUCCESS 를 반환하면 된다.
 *
 * BUG-48353 : 이 함수는head piece 를 제외한 piece 일때 호출되는 함수이다.
 *             따라서 여기서 Pending Wait가 발생할 수 없다.
 *             (왜냐하면 head piece 처리시 이미 Pending Wait 했을것이기 때문이다.)
 *
 * aStatistics   - [IN] 통계정보
 * aStartInfo    - [IN] Mtx StartInfo 포인터
 * aTargetRow    - [IN] Row TimeStamping을 수행할 대상 RowPiece 포인터
 * aPageReadMode - [IN] page read mode(SPR or MPR)
 *
 **********************************************************************/
IDE_RC sdcTableCTL::checkAndMakeSureRowStamping(
                                    idvSQL          * aStatistics,
                                    sdrMtxStartInfo * aStartInfo,
                                    UChar           * aTargetRow,
                                    sdbPageReadMode   aPageReadMode )
{
    UChar           sCTSlotIdx;
    smSCN           sMyFstDskViewSCN;
    sdSID           sMyTransTSSlotSID;
    sdpPhyPageHdr * sPageHdr;
    UInt            sState = 0;
    sdrMtx          sMtx;
    smTID           sWait4TransID;
    smSCN           sRowCommitSCN;
    smSCN           sFstDskViewSCN;
    SShort          sFSCreditSize;
    void          * sObjPtr;

    IDE_ERROR( sdcRow::isHeadRowPiece( aTargetRow ) == ID_FALSE );

    SDC_GET_ROWHDR_1B_FIELD(aTargetRow, SDC_ROWHDR_CTSLOTIDX, sCTSlotIdx);
    sCTSlotIdx = SDC_UNMASK_CTSLOTIDX(sCTSlotIdx);

    IDE_TEST_CONT( sCTSlotIdx == SDP_CTS_IDX_UNLK,
                   already_row_stamping );

    getChangedTransInfo( aTargetRow,
                         &sCTSlotIdx,
                         &sObjPtr,
                         &sFSCreditSize,
                         &sRowCommitSCN );

    sPageHdr = sdpPhyPage::getHdr( aTargetRow );

    /* BUG-24906 DML중 UndoRec 기록하다 UNDOTBS 공간부족하면 서버사망 !!
     * 본 함수에서 별도의 Mtx로 Logging을 수행하는 것은 해당 Update/Delete
     * 연산에 대한 UndoRec 기록할때 mtx 가 Rollback이 될수 없는 Logical한
     * 연산이기 때문이다. */
    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sMtx,
                                   aStartInfo,
                                   ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT )
              != IDE_SUCCESS );
    sState = 1;

    if ( SDC_CTS_SCN_IS_NOT_COMMITTED(sRowCommitSCN) )
    {
        sMyFstDskViewSCN  = smxTrans::getFstDskViewSCN( sMtx.mTrans );
        sMyTransTSSlotSID = smxTrans::getTSSlotSID( sMtx.mTrans );

        if ( sdcRow::isMyTransUpdating( (UChar*)sPageHdr,
                                        aTargetRow,
                                        sMyFstDskViewSCN,
                                        sMyTransTSSlotSID,
                                        &sFstDskViewSCN ) == ID_FALSE )
        {
            IDE_TEST( logAndRunDelayedRowStamping(
                                      aStatistics,
                                      &sMtx,
                                      sCTSlotIdx,
                                      sObjPtr,
                                      aPageReadMode,
                                      &sWait4TransID, 
                                      &sRowCommitSCN ) != IDE_SUCCESS );
        }
    }
    else
    {
        IDE_TEST( logAndRunRowStamping( &sMtx,
                                        sCTSlotIdx,
                                        sObjPtr,
                                        sFSCreditSize,
                                        &sRowCommitSCN )
                  != IDE_SUCCESS );
    }

    IDE_TEST( sdrMiniTrans::setDirtyPage( &sMtx, (UChar*)sPageHdr )
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );

    IDE_EXCEPTION_CONT( already_row_stamping );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_PUSH();
        IDE_ASSERT( sdrMiniTrans::rollback(&sMtx) == IDE_SUCCESS );
        IDE_POP();
    }

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : RowPiece로부터 CommitSCN을 반환한다.
 *
 * aTargetRowPtr  - [IN]  MSCNOrCSCN을 알고 싶은 TargetRow 포인터
 * aCTSlotIdx     - [OUT] CTSlotIdx
 * aObjPtr        - [OUT] Object Ptr
 * aFSCreditSize  - [OUT] FSCreditSize
 * aMSCNOrCSCN    - [OUT] 판독된 MSCNOrCSCN
 *
 **********************************************************************/
void sdcTableCTL::getChangedTransInfo( UChar   * aTargetRow,
                                       UChar   * aCTSlotIdx,
                                       void   ** aObjPtr,
                                       SShort  * aFSCreditSize,
                                       smSCN   * aFSCNOrCSCN )
{
    void    * sObjPtr = NULL;
    sdpCTS  * sCTS;
    UChar     sCTSlotIdx;
    smSCN     sFSCNOrCSCN;
    SShort    sFSCreditSize = 0;

    SM_INIT_SCN( &sFSCNOrCSCN );

    SDC_GET_ROWHDR_1B_FIELD( aTargetRow,
                             SDC_ROWHDR_CTSLOTIDX,
                             sCTSlotIdx );

    sCTSlotIdx = SDC_UNMASK_CTSLOTIDX(sCTSlotIdx);
    
    // SDP_CTS_IDX_UNLK 가 아닌걸 확인하고 왔지만 
    // 디버그에서만 죽이자.         
    IDE_DASSERT( sCTSlotIdx != SDP_CTS_IDX_UNLK );
    IDE_TEST_CONT( sCTSlotIdx == SDP_CTS_IDX_UNLK,
                   cont_has_finished );

    if ( SDC_HAS_BOUND_CTS(sCTSlotIdx) )
    {
        sCTS = getCTS( sdpPhyPage::getHdr( aTargetRow ),
                       sCTSlotIdx );

        sFSCreditSize = sCTS->mFSCredit;
        SM_SET_SCN( &sFSCNOrCSCN, &sCTS->mFSCNOrCSCN );

        sObjPtr = sCTS;
    }
    else
    {
        IDE_ASSERT( sCTSlotIdx == SDP_CTS_IDX_NULL );

        SDC_GET_ROWHDR_FIELD( aTargetRow,
                              SDC_ROWHDR_FSCNORCSCN,
                              &sFSCNOrCSCN );

        SDC_GET_ROWHDR_FIELD( aTargetRow,
                              SDC_ROWHDR_FSCREDIT,
                              &sFSCreditSize );

        sObjPtr = aTargetRow;
    }

    IDE_EXCEPTION_CONT( cont_has_finished );

    *aCTSlotIdx     = sCTSlotIdx;
    *aObjPtr        = sObjPtr;
    *aFSCreditSize  = sFSCreditSize;
    SM_SET_SCN( aFSCNOrCSCN, &sFSCNOrCSCN );
}


/***********************************************************************
 * Description : TSS로부터 CommitSCN을 반환한다.
 *
 * aStatistics      - [IN]  통계정보
 * aTrans           - [IN]  자신의 TX의 포인터
 * aCTSlotIdx       - [IN]  Target Row의 CTSlotIdx
 * aObjPtr          - [IN]  Target Row의 CTS 또는 Row Slot의 포인터
 * aStmtViewSCN     - [IN]  올바른 버전의 row를 읽기 위한 Statement viewscn
 * aTID4Wait        - [OUT] 만약 TSS를 소유한 TX가 Active 상태인 경우 해당 TX의 TID
 * aCommitSCN       - [OUT] TX의 CommitSCN이나 Unbound CommitSCN 혹은 TSS를
 *                          소유한 TX의 BeginSCN일수 있다.
 **********************************************************************/
IDE_RC sdcTableCTL::getCommitSCN( idvSQL   * aStatistics,
                                  void     * aTrans,
                                  UChar      aCTSlotIdx,
                                  void     * aObjPtr,
                                  smSCN      aStmtViewSCN,
                                  smTID    * aTID4Wait,
                                  smSCN    * aCommitSCN )
{
    sdpCTS            * sCTS;
    UChar             * sRowSlotPtr;
    sdcRowHdrExInfo     sRowHdrExInfo;
    sdSID               sTSSlotSID;

    IDE_ERROR( aObjPtr         != NULL );
    IDE_ERROR( aTID4Wait       != NULL );
    IDE_ERROR( aCommitSCN      != NULL );

    /* CTS에 Transaction 정보가 bound 되어 있는 경우 */
    if( SDC_HAS_BOUND_CTS(aCTSlotIdx) )
    {
        sCTS = (sdpCTS*)aObjPtr;

        sTSSlotSID = SD_MAKE_SID( sCTS->mTSSPageID, sCTS->mTSSlotNum );
        IDE_ERROR( SDC_CTS_SCN_IS_NOT_COMMITTED(sCTS->mFSCNOrCSCN) );

        IDE_TEST( sdcTSSegment::getCommitSCN( aStatistics,
                                              aTrans,
                                              sTSSlotSID,
                                              &(sCTS->mFSCNOrCSCN),
                                              aStmtViewSCN,
                                              aTID4Wait,
                                              aCommitSCN )
                  != IDE_SUCCESS );
    }
    else /* Row에 Transaction 정보가 bound 되어 있는 경우 */
    {
        IDE_ERROR( aCTSlotIdx == SDP_CTS_IDX_NULL );

        sRowSlotPtr = (UChar*)aObjPtr;
        sdcRow::getRowHdrExInfo( sRowSlotPtr, &sRowHdrExInfo );
        sTSSlotSID = SD_MAKE_SID( sRowHdrExInfo.mTSSPageID,
                                  sRowHdrExInfo.mTSSlotNum );

        IDE_TEST( sdcTSSegment::getCommitSCN( aStatistics,
                                              aTrans,
                                              sTSSlotSID,
                                              &sRowHdrExInfo.mFSCNOrCSCN,
                                              aStmtViewSCN,
                                              aTID4Wait,
                                              aCommitSCN )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



/***********************************************************************
 *
 * Description : 데이타페이지의 Delete된 RowPiece 대한 SelfAging 및 로깅 수행
 *
 * aStatistics  - [IN] 통계정보
 * aStartInfo   - [IN] 로깅을 위한 Mtx 시작정보
 * aPageHdrPtr  - [IN] 페이지 헤더 시작 포인터
 * aSCNtoAging  - [IN] 가장 오랜된 SSCN으로 Self-Aging의 기준이 되는 SSCN -> SysMinDskViewSCN
 **********************************************************************/
IDE_RC sdcTableCTL::logAndRunSelfAging( idvSQL          * aStatistics,
                                        sdrMtxStartInfo * aStartInfo,
                                        sdpPhyPageHdr   * aPageHdrPtr,
                                        smSCN           * aSCNtoAging )
{
    sdrMtx   sLogMtx;
    UInt     sState      = 0;
    UShort   sAgedRowCnt = 0;

    IDE_ERROR( aStartInfo  != NULL );
    IDE_ERROR( aSCNtoAging != NULL );

    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sLogMtx,
                                   aStartInfo,
                                   ID_TRUE,/*MtxUndoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT )
              != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( runSelfAging( aPageHdrPtr, aSCNtoAging, &sAgedRowCnt )
              != IDE_SUCCESS );

    if( sAgedRowCnt == 0 )
    {
        IDE_TEST( runDelayedStampingAll( aStatistics,
                                         aStartInfo->mTrans,
                                         (UChar*)aPageHdrPtr,
                                         SDB_SINGLE_PAGE_READ )
                  != IDE_SUCCESS );
    }

    /* Logging을 했으면, setDirty를 무조건 해야 함. */

    /* BUG-31508 [sm-disk-collection]does not set dirty page
     *           when executing self aging in DRDB.
     * AgedRow, 즉 FreeSlot된 Record가 존재할 경우 반드시 dirty시켜야함.
     * 그렇지 않을 경우, FreeSlot된 Record들이 Disk에 저장 안됨*/
    IDE_TEST( sdrMiniTrans::setDirtyPage( &sLogMtx,
                                          (UChar*)aPageHdrPtr )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::writeLogRec( &sLogMtx,
                                         (UChar*)aPageHdrPtr,
                                         (UChar*)aSCNtoAging,
                                         ID_SIZEOF(smSCN),
                                         SDR_SDC_DATA_SELFAGING )
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit( &sLogMtx ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback( &sLogMtx ) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : 데이타페이지의 Delete된 RowPiece 대한 SelfAging 수행
 *
 * 데이타페이지에서는 Deleted 커밋된 Row들도 어느정도 데이타페이지에 내버려둔다.
 * 이는 RowPiece 단위 Consistent Read를 지원하기 위한 것인데 만약, Delete된
 * Row의 이전 버전 즉 Delete되기 전 RowPiece를 봐야하는 Statement가 존재할 경우
 * Delete된 Row의 UndoSID를 가지고 이전버전을 생성해야한다. 그렇기 때문에
 * 그 RowPiece의 삭제되기 이전버전을 볼 가능성이 있는 이러한 Statement 들이 모두
 * 완료되기 전까지는 Delete된 RowPiece를 데이타페이지에서 제거해서는 안된다.
 *
 * 즉, Self-Aging은 볼 가능성이 있는 Statement가 모두 없어진 RowPiece들을 페이지
 * 가용공간으로 모두 반환하는 일련의 작업을 의미한다.
 *
 * Self-Aging을 하기 위해서는 SlotDirectory를 순차 검색하면서 진행하는데, 이때 다음
 * Self-Aging에 대상이 되는 RowPiece 개수는 최대 CommitSCN을 구하기도 한다.
 *
 * aPageHdrPtr  - [IN] 페이지 헤더 시작 포인터
 * aSCNtoAging  - [IN] 가장 오랜된 SSCN으로 Self-Aging의 기준이 되는 SSCN -> SysMinDskViewSCN
 *
 **********************************************************************/
IDE_RC sdcTableCTL::runSelfAging( sdpPhyPageHdr * aPageHdrPtr,
                                  smSCN         * aSCNtoAging,
                                  UShort        * aAgedRowCnt )
{
    UChar       sCTSlotIdx;
    scSlotNum   sSlotNum;
    UChar    *  sSlotDirPtr;
    UChar    *  sSlotPtr;
    UShort      sTotSlotCnt;
    smSCN       sNewSCNtoAging;
    smSCN       sCommitSCN;
    UShort      sNewAgingDelRowCnt;
    sdpCTL   *  sCTL;
    UShort      sSlotSize;
    UShort      sAgedRowCnt = 0;

    IDE_ASSERT( aPageHdrPtr    != NULL );
    IDE_ASSERT( aSCNtoAging != NULL );

    IDE_ASSERT( (aPageHdrPtr->mPageType == SDP_PAGE_DATA) );

    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr((UChar*)aPageHdrPtr);
    sTotSlotCnt = sdpSlotDirectory::getCount(sSlotDirPtr);
    sCTL        = getCTL( aPageHdrPtr );

    SM_INIT_SCN( &sNewSCNtoAging );
    sNewAgingDelRowCnt = 0;
    *aAgedRowCnt       = 0;

    for ( sSlotNum = 0; sSlotNum < sTotSlotCnt; sSlotNum ++ )
    {
        if( sdpSlotDirectory::isUnusedSlotEntry(sSlotDirPtr, sSlotNum)
            == ID_FALSE )
        {
            IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr,
                                                               sSlotNum,
                                                               &sSlotPtr )
                      != IDE_SUCCESS );

            if ( sdcRow::isDeleted( sSlotPtr ) == ID_TRUE )
            {
                SDC_GET_ROWHDR_1B_FIELD( sSlotPtr,
                                         SDC_ROWHDR_CTSLOTIDX,
                                         sCTSlotIdx );

                if( sCTSlotIdx != SDP_CTS_IDX_UNLK )
                {
                    continue;
                }

                SDC_GET_ROWHDR_FIELD( sSlotPtr,
                                      SDC_ROWHDR_FSCNORCSCN,
                                      &sCommitSCN );

                if( SM_SCN_IS_LE( &sCommitSCN, aSCNtoAging ))
                {
                    sSlotSize = sdcRow::getRowPieceSize( sSlotPtr );
                    IDE_ASSERT( sSlotSize == SDC_ROWHDR_SIZE );

                    IDE_TEST( sdpPhyPage::freeSlot4SID( aPageHdrPtr,
                                                        sSlotPtr,
                                                        sSlotNum,
                                                        sSlotSize )
                              != IDE_SUCCESS );

                    decDelRowCntOfCTL( sCTL );
                    sAgedRowCnt++;
                    continue;
                }
                else
                {
                    if ( SM_SCN_IS_GT(&sCommitSCN, &sNewSCNtoAging))
                    {
                        SM_SET_SCN( &sNewSCNtoAging, &sCommitSCN );
                    }

                    sNewAgingDelRowCnt++;
                }
            }
        }
    }

    if ( sNewAgingDelRowCnt > 0 )
    {
        IDE_ASSERT( SM_SCN_IS_NOT_INIT( sNewSCNtoAging ) );
        SM_SET_SCN( &(sCTL->mSCN4Aging), &sNewSCNtoAging );
    }
    else
    {
        IDE_ASSERT( SM_SCN_IS_INIT( sNewSCNtoAging ) );
        SM_SET_SCN_INFINITE( &sCTL->mSCN4Aging );
    }

    sCTL->mCandAgingCnt = sNewAgingDelRowCnt;
    *aAgedRowCnt = sAgedRowCnt;

    IDE_DASSERT( sdpPhyPage::validate(aPageHdrPtr) == ID_TRUE );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : 데이타페이지에 대해서 SelfAging 가능여부 판단 및 SelfAging 수행
 *
 * 일단 매번 페이지를 Self-Aging을 위해서 접근할 때마다 SlotDirectory를 순차검색을
 * 하는 것은 비용이크기 때문에 어느정도 바로 판단할 수 있는 정보를 기반으로 판단한다.
 * 예를들어, Delete된 RowPiece의 개수(모두 Self-Aging대상일 수도 그렇지 않을수도
 * 있다 )라든지, Self-Aging 할 수 있는 RowPiece 의 개수 및 최대 CommitSCN등을
 * 보고 판단한다.
 *
 * 한번 SelfAing을 수행했다면 한번더 현재의 Self-Aging에 대한 상태를 확인하여 반환한다.
 *
 * aStatistics  - [IN] 통계정보
 * aStartInfo   - [IN] 로깅을 위한 Mtx 시작정보
 * aPageHdrPtr  - [IN] 페이지 헤더 시작 포인터
 * aCheckFlag   - [OUT] 페이지의 최종 Self-Aging 상태
 *
 **********************************************************************/
IDE_RC sdcTableCTL::checkAndRunSelfAging( idvSQL           * aStatistics,
                                          sdrMtxStartInfo  * aStartInfo,
                                          sdpPhyPageHdr    * aPageHdrPtr,
                                          sdpSelfAgingFlag * aCheckFlag )
{
    sdpSelfAgingFlag sCheckFlag;
    smSCN            sSCNtoAging;

    IDE_ERROR( aStartInfo != NULL );
    IDE_ERROR( aPageHdrPtr   != NULL );
    IDE_ERROR( aCheckFlag != NULL );

    SMX_GET_MIN_DISK_VIEW( &sSCNtoAging );
    sCheckFlag = canAgingBySelf( aPageHdrPtr, &sSCNtoAging );

    if ( (sCheckFlag == SDP_SA_FLAG_CHECK_AND_AGING) ||
         (sCheckFlag == SDP_SA_FLAG_AGING_DEAD_ROWS) )
    {
        IDE_TEST( logAndRunSelfAging( aStatistics,
                                      aStartInfo,
                                      aPageHdrPtr,
                                      &sSCNtoAging )/* xxxx isDoaging*/
                  != IDE_SUCCESS );

        sCheckFlag = canAgingBySelf( aPageHdrPtr, &sSCNtoAging );
    }

    *aCheckFlag = sCheckFlag;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : 데이타페이지의 Self-Aging 가능 여부 판단
 *
 * aPageHdrPtr     - [IN] 페이지 헤더 시작 포인터
 * aSCNtoAging  - [IN] 가장 오랜된 SSCN으로 Self-Aging의 기준이 되는 SSCN
 *
 ***********************************************************************/
sdpSelfAgingFlag sdcTableCTL::canAgingBySelf(
                        sdpPhyPageHdr * aPageHdrPtr,
                        smSCN         * aSCNtoAging )
{
    sdpCTL           * sCTL;
    sdpSelfAgingFlag   sResult;

    sCTL = getCTL( aPageHdrPtr );

    if ( sCTL->mDelRowCnt > 0 )
    {
        if ( SM_SCN_IS_INFINITE(sCTL->mSCN4Aging) )
        {
            sResult = SDP_SA_FLAG_CHECK_AND_AGING;
        }
        else
        {
            if ( SM_SCN_IS_GE( aSCNtoAging, &sCTL->mSCN4Aging) )
            {
                IDE_ASSERT( sCTL->mCandAgingCnt > 0 );
                sResult = SDP_SA_FLAG_AGING_DEAD_ROWS;
            }
            else
            {
                sResult = SDP_SA_FLAG_CANNOT_AGING;
            }
        }
    }
    else
    {
        IDE_ASSERT( SM_SCN_IS_INFINITE( sCTL->mSCN4Aging ));
        sResult = SDP_SA_FLAG_NOTHING_TO_AGING;
    }

    return sResult;
}

/*********************************************************************
 *
 * Description : Row TimeStamping For ALTER TABLE AGING
 *
 * ALTER TABLE <<TABLE명>> AGING; 구문으로 사용자가 Row TimeStaping을
 * 명시적으로 할 수 있게 한다.
 *
 * aStatistics         - [IN] 통계정보
 * aStartInfo          - [IN] mtx start info
 * aPageptr            - [IN] 페이지 시작 포인터
 * aPageReadMode       - [IN] page read mode(SPR or MPR)
 *
 *********************************************************************/
IDE_RC sdcTableCTL::runRowStampingAll(
                       idvSQL          * aStatistics,
                       sdrMtxStartInfo * aStartInfo,
                       UChar           * aPagePtr,
                       sdbPageReadMode   aPageReadMode )
{
    sdrMtx     sMtx;
    sdpCTL   * sCTL;
    UChar      sIdx;
    sdpCTS   * sCTS;
    UInt       sTotCTSCnt;
    UChar      sStat;
    idBool     sTrySuccess;
    UInt       sState = 0;
    UInt       sStampingSuccessCnt = 0;

    IDE_ERROR( aPagePtr != NULL );

    sCTL = getCTL( (sdpPhyPageHdr*)aPagePtr );

    /* Page에 Row Stamping할 대상 CTS가 존재하지 않는다면
     * IDE_SUCCESS를 반환하고 완료한다. */
    IDE_TEST_CONT( sCTL->mBindCTSCnt == 0, skip_soft_stamping );

    /* S-latch가 획득되 들어오기 때문에 X-latch로 전환해줄수 있다면 전환한다.
     * 전환 못한 경우는 IDE_SUCCESS를 반환하고 완료한다. */
    sdbBufferMgr::tryEscalateLatchPage( aStatistics,
                                        aPagePtr,
                                        aPageReadMode,
                                        &sTrySuccess );

    IDE_TEST_CONT( sTrySuccess == ID_FALSE, skip_soft_stamping );

    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sMtx,
                                   aStartInfo,
                                   ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT )
              != IDE_SUCCESS );
    sState = 1;

    sTotCTSCnt = sCTL->mTotCTSCnt;

    for ( sIdx = 0; sIdx < sTotCTSCnt; sIdx++ )
    {
        sCTS  = getCTS( sCTL, sIdx );
        sStat = sCTS->mStat;

        if ( hasState( sStat, SDP_CTS_STAT_CTS ) == ID_TRUE )
        {
            IDE_TEST( logAndRunRowStamping( &sMtx,
                                            (UChar)sIdx,
                                            (void*)sCTS,
                                            sCTS->mFSCredit,
                                            &sCTS->mFSCNOrCSCN )
                      != IDE_SUCCESS );

            sStampingSuccessCnt++;
        }
    }

    if ( sStampingSuccessCnt > 0 )
    {
        IDE_TEST( sdrMiniTrans::setDirtyPage( &sMtx, aPagePtr )
                  != IDE_SUCCESS );
    }

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );

    IDE_EXCEPTION_CONT( skip_soft_stamping );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback(&sMtx) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : CTL의 Delete된 Row중에 잠정적인 Self-Aging 대상
 *               Row Piece 개수를 고려하여 대략적인 Aging할
 *               총 가용공간 크기 반환
 *
 * aMtx         - [IN] Mtx 포인터
 * aPageHdrPtr  - [IN] 페이지 헤더 포인터
 *
 ***********************************************************************/
UInt sdcTableCTL::getTotAgingSize( sdpPhyPageHdr * aPageHdrPtr )
{
    sdpCTL * sCTL;

    sCTL = getCTL( aPageHdrPtr );

    return (sCTL->mDelRowCnt * SDC_MIN_ROWPIECE_SIZE);
}

/***********************************************************************
 *
 * Description : CTL占쏙옙占쏙옙占싸븝옙占쏙옙 占쏙옙 CTS 占쏙옙占쏙옙 占쏙옙환
 ***********************************************************************/
UInt sdcTableCTL::getCountOfCTS( sdpPhyPageHdr * aPageHdr )
{
    return getCnt(getCTL(aPageHdr));
}


/***********************************************************************
 * TASK-4007 [SM]PBT를 위한 기능 추가
 * Description : page를 Dump하여 table의 ctl을 출력한다.
 *
 * BUG-28379 [SD] sdnbBTree::dumpNodeHdr( UChar *aPage ) 내에서
 * local Array의 ptr를 반환하고 있습니다.
 * Local Array대신 OutBuf를 받아 리턴하도록 수정합니다.
 ***********************************************************************/
IDE_RC sdcTableCTL::dump( UChar *aPage ,
                          SChar *aOutBuf ,
                          UInt   aOutSize )
{
    sdpCTL        * sCTL;
    sdpCTS        * sCTS;
    UInt            sIdx;

    IDE_DASSERT( aPage != NULL );
    IDE_DASSERT( sdpPhyPage::getPageStartPtr(aPage) == aPage );

    sCTL        = getCTL( (sdpPhyPageHdr*)aPage );

    idlOS::snprintf( aOutBuf,
                     aOutSize,
"----------- Table CTL Begin ----------\n"
"mSCN4Aging    : %"ID_UINT64_FMT"\n"
"mCandAgingCnt : %"ID_UINT32_FMT"\n"
"mDelRowCnt    : %"ID_UINT32_FMT"\n"
"mTotCTSCnt    : %"ID_UINT32_FMT"\n"
"mBindCTSCnt   : %"ID_UINT32_FMT"\n"
"mMaxCTSCnt    : %"ID_UINT32_FMT"\n"
"---+---------------------------------------------------------------------------\n"
"   |mTSSPageID TSSlotNum mFSCredit mStat mAlign mRefCnt mRefSlotNum mFSCNOrCSCN\n"
"---+---------------------------------------------------------------------------\n",
                     SM_SCN_TO_LONG( sCTL->mSCN4Aging ),
                     sCTL->mCandAgingCnt,
                     sCTL->mDelRowCnt,
                     sCTL->mTotCTSCnt,
                     sCTL->mBindCTSCnt,
                     sCTL->mMaxCTSCnt );

    for ( sIdx = 0; sIdx < sCTL->mTotCTSCnt ; sIdx++ )
    {
        sCTS  = getCTS( sCTL, sIdx );

        idlVA::appendFormat( aOutBuf,
                             aOutSize,
                             "%3"ID_UINT32_FMT"|"
                             "%-10"ID_UINT32_FMT
                             " %-9"ID_UINT32_FMT
                             " %-9"ID_UINT32_FMT
                             " %-5"ID_UINT32_FMT
                             " %-6"ID_UINT32_FMT
                             " %-7"ID_UINT32_FMT
                             " [%-4"ID_UINT32_FMT","
                             "%-4"ID_UINT32_FMT"]"
                             " %-11"ID_UINT64_FMT"\n",
                             sIdx,
                             sCTS->mTSSPageID,
                             sCTS->mTSSlotNum,
                             sCTS->mFSCredit,
                             sCTS->mStat,
                             sCTS->mAlign,
                             sCTS->mRefCnt,
                             sCTS->mRefRowSlotNum[ 0 ],
                             sCTS->mRefRowSlotNum[ 1 ],
                             SM_SCN_TO_LONG( sCTS->mFSCNOrCSCN ) );
    }

    idlVA::appendFormat( aOutBuf,
                         aOutSize,
                         "----------- Table CTL End   ----------\n" );
    return IDE_SUCCESS;
}


