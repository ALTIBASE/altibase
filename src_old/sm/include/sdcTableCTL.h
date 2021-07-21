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

# ifndef _O_SDC_TABLE_CTL_H_
# define _O_SDC_TABLE_CTL_H_ 1

# include <sdpDef.h>
# include <sdpPhyPage.h>

# include <sdcDef.h>
# include <sdcRow.h>

# define SDC_UNMASK_CTSLOTIDX( aCTSlotIdx )   \
    ( aCTSlotIdx & SDP_CTS_IDX_MASK )

# define SDC_HAS_BOUND_CTS( aCTSlotIdx )      \
    ( aCTSlotIdx <= SDP_CTS_MAX_IDX )

# define SDC_SET_CTS_LOCK_BIT( aCTSlotNum )    \
    ( (aCTSlotNum) |= SDP_CTS_LOCK_BIT )

# define SDC_CLR_CTS_LOCK_BIT( aCTSlotNum )    \
    ( (aCTSlotNum) &= ~SDP_CTS_LOCK_BIT )

# define SDC_IS_CTS_LOCK_BIT( aCTSlotNum )     \
    ( ((aCTSlotNum) & SDP_CTS_LOCK_BIT) == SDP_CTS_LOCK_BIT )

class sdcTableCTL
{

public:

    static IDE_RC logAndInit( sdrMtx        * aMtx,
                              sdpPhyPageHdr * aPageHdrPtr,
                              UChar           aInitTrans,
                              UChar           aMaxTrans );

    static IDE_RC logAndExtend( sdrMtx        * aMtx,
                                sdpPhyPageHdr * aPageHdrPtr,
                                UChar           aExtendSlotCnt,
                                UChar         * aCTSlotIdx,
                                idBool        * aTrySuccess );


    static void init( sdpPhyPageHdr   * aPageHdrPtr,
                      UChar             aInitTrans,
                      UChar             aMaxTrans );

    static IDE_RC extend( sdpPhyPageHdr   * aPageHdrPtr,
                          UChar             aExtendSlotCnt,
                          UChar           * aCTSlotIdx,
                          idBool          * aTrySuccess );

    static IDE_RC allocCTSAndSetDirty(
                            idvSQL            * aStatistics,
                            sdrMtx            * aFixMtx,
                            sdrMtxStartInfo   * aStartInfo,
                            sdpPhyPageHdr     * aPagePtr,
                            UChar             * aCTSSlotIdx );

    static IDE_RC allocCTS( idvSQL            * aStatistics,
                            sdrMtx            * aFixMtx,
                            sdrMtx            * aLogMtx,
                            sdbPageReadMode     aPageReadMode,
                            sdpPhyPageHdr     * aPagePtr,
                            UChar             * aCTSlotIdx );

    static IDE_RC bind( sdrMtx        * aMtx,
                        scSpaceID       aSpaceID,
                        UChar         * aNewSlotPtr,
                        UChar           aRowCTSlotIdx,
                        UChar           aNewCTSlotIdx,
                        scSlotNum       aRowSlotNum,
                        UShort          aFSCreditSize4RowHdrEx,
                        SShort          aNewFSCreditSize,
                        idBool          aIncDeleteRowCnt );

    static IDE_RC unbind( sdrMtx         * aMtx,
                          UChar          * aNewSlotPtr,
                          UChar            aCTSlotIdxBfrUndo,
                          UChar            aCTSlotIdxAftUndo,
                          SShort           aFSCreditSize,
                          idBool           aDecDeleteRowCnt );

    static IDE_RC canAllocCTS( sdpPhyPageHdr  * aPageHdrPtr );

    static inline sdpCTS * getCTS( sdpCTL  * aHdrPtr,
                                   UChar     aCTSlotIdx );

    static IDE_RC runFastStamping( sdSID         * aTransTSSlotSID,
                                   smSCN         * aFstDskViewSCN,
                                   sdpPhyPageHdr * aPagePtr,
                                   UChar           aCTSlotIdx,
                                   smSCN         * aCommitSCN );

    static IDE_RC stampingAll4RedoValidation( 
                                    idvSQL           * aStatistics,
                                    UChar            * aPagePtr1,
                                    UChar            * aPagePtr2 );

    static IDE_RC runDelayedStamping( idvSQL           * aStatistics,
                                      void             * aTrans,
                                      UChar              aCTSlotIdx,
                                      void             * aObjPtr,
                                      sdbPageReadMode    aPageReadMode,
                                      smSCN              aStmtViewSCN,
                                      idBool           * aTrySuccess,
                                      smTID            * aWait4TransID,
                                      smSCN            * aRowCommitSCN,
                                      SShort           * aFSCreditSize );

    static IDE_RC runDelayedStampingAll( idvSQL          * aStatistics,
                                         void            * aTrans,
                                         UChar           * aPagePtr,
                                         sdbPageReadMode   aPageReadMode );

    static IDE_RC logAndRunRowStamping( sdrMtx    * aMtx,
                                        UChar       aCTSlotIdx,
                                        void      * aObjPtr,
                                        SShort      aFSCreditSize,
                                        smSCN     * aCommitSCN );

    static IDE_RC runRowStampingAll( idvSQL          * aStatistics,
                                     sdrMtxStartInfo * aStartInfo,
                                     UChar           * aPagePtr,
                                     sdbPageReadMode   aPageReadMode );

    static IDE_RC logAndRunDelayedRowStamping(
                                        idvSQL          * aStatistics,
                                        sdrMtx          * aMtx,
                                        UChar             aCTSlotIdx,
                                        void            * aObjPtr,
                                        sdbPageReadMode   aPageReadMode,
                                        smTID           * aWait4TransID,
                                        smSCN           * aRowCommitSCN );

    static inline sdpCTL * getCTL( sdpPhyPageHdr  * aPageHdrPtr );

    static IDE_RC runRowStampingOnCTS( sdpCTS * aCTS,
                                       UChar    aCTSlotIdx,
                                       smSCN  * aCommitSCN  );

    static IDE_RC runRowStampingOnRow( UChar      * aSlotPtr,
                                       UChar        aCTSlotIdx,
                                       smSCN      * aCommitSCN );

    static inline sdpCTS * getCTS( sdpPhyPageHdr  * aPagePtr,
                                   UChar            aCTSlotIdx );

    static IDE_RC restoreFSCredit( sdpPhyPageHdr  * aPageHdrPtr,
                                   SShort           aRestoreSize );

    static IDE_RC restoreFSCredit( sdrMtx        * aMtx,
                                   sdpPhyPageHdr * aPageHdrPtr,
                                   SShort          aRestoreSize );

    static IDE_RC checkAndMakeSureRowStamping(
                                    idvSQL          * aStatistics,
                                    sdrMtxStartInfo * aStartInfo,
                                    UChar           * aTargetRow,
                                    sdbPageReadMode   aPageReadMode );

    static IDE_RC checkAndRunSelfAging( idvSQL           * aStatistics,
                                        sdrMtxStartInfo  * aMtx,
                                        sdpPhyPageHdr    * aPageHdrPtr,
                                        sdpSelfAgingFlag * aCheckFlag );

    static IDE_RC runSelfAging( sdpPhyPageHdr * aPageHdrPtr,
                                smSCN         * aSCNtoAging,
                                UShort        * aAgedRowCnt );

    static UInt getTotAgingSize( sdpPhyPageHdr * aPageHdrPtr );

    static IDE_RC runDelayedStampingOnCTS( idvSQL           * aStatistics,
                                           void             * aTrans,
                                           sdpCTS           * aCTS,
                                           sdbPageReadMode    aPageReadMode,
                                           smSCN              aStmtViewSCN,
                                           idBool           * aTrySuccess,
                                           smTID            * aWait4TransID,
                                           smSCN            * aRowCommitSCN,
                                           SShort           * aFSCreditSize );

    static IDE_RC runDelayedStampingOnRow( idvSQL           * aStatistics,
                                           void             * aTrans,
                                           UChar            * aRowSlotPtr,
                                           sdbPageReadMode    aPageReadMode,
                                           smSCN              aStmtViewSCN,
                                           idBool           * aTrySuccess,
                                           smTID            * aWait4TransID,
                                           smSCN            * aRowCommitSCN,
                                           SShort           * aFSCreditSize );

    static IDE_RC bindCTS4REDO( sdpPhyPageHdr * aPageHdrPtr,
                                UChar           aOldCTSlotIdx,
                                UChar           aNewCTSlotIdx,
                                scSlotNum       aRowSlotNum,
                                smSCN         * aFstDskViewSCN,
                                sdSID           aTSSlotSID,
                                SShort          aNewFSCreditSize,
                                idBool          aIncDeleteRowCnt );

    static IDE_RC unbindCTS4REDO( sdpPhyPageHdr  * aPageHdrPtr,
                                  UChar            aCTSlotIdxBfrUndo,
                                  UChar            aCTSlotIdxAftUndo,
                                  SShort           aFSCreditSize,
                                  idBool           aDecDelRowCnt );

    static IDE_RC bindRow4REDO( UChar      * aNewSlotPtr,
                                sdSID        aTSSlotRID,
                                smSCN        aFstDskViewSCN,
                                SShort       aFSCreditSizeToWrite,
                                idBool       aIncDelRowCnt );

    static IDE_RC unbindRow4REDO( UChar     * aRowPieceBfrUndo,
                                  SShort      aFSCreditSize,
                                  idBool      aDecDelRowCnt );

    static void getChangedTransInfo( UChar   * aTargetRow,
                                     UChar   * aCTSlotIdx,
                                     void   ** aObjPtr,
                                     SShort  * aFSCreditSize,
                                     smSCN   * aFSCNOrCSCN );

    static IDE_RC getCommitSCN( idvSQL   * aStatistics,
                                void     * aTrans,
                                UChar      aCTSlotIdx,
                                void     * aObjPtr,
                                smSCN      aStmtViewSCN,
                                smTID    * aTID4Wait,
                                smSCN    * aCommitSCN );

    static UInt getCountOfCTS( sdpPhyPageHdr  * aPageHdrPtr );

    /* TASK-4007 [SM] PBT를 위한 기능 추가
     * CTS Dump할 수 있는 기능 추가*/
    static IDE_RC dump( UChar *aPage ,
                        SChar *aOutBuf ,
                        UInt   aOutSize );
private:

    static IDE_RC logAndBindCTS( sdrMtx         * aMtx,
                                 scSpaceID        aSpaceID,
                                 sdpPhyPageHdr  * aPageHdrPtr,
                                 UChar            aOldCTSlotIdx,
                                 UChar            aNewCTSlotIdx,
                                 scSlotNum        aRowSlotNum,
                                 SShort           aNewFSCreditSize,
                                 idBool           aIncDeleteRowCnt );

    static IDE_RC logAndUnbindCTS( sdrMtx         * aMtx,
                                   sdpPhyPageHdr  * aPageHdrPtr,
                                   UChar            aCTSlotIdxBfrUndo,
                                   UChar            aCTSlotIdxAftUndo,
                                   SShort           aFSCreditSize,
                                   idBool           aDecDelRowCnt );

    static IDE_RC logAndBindRow( sdrMtx         * aMtx,
                                 UChar          * aNewSlotPtr,
                                 SShort           aFSCreditSizeToWrite,
                                 idBool           aIncDelRowCnt );

    static IDE_RC logAndUnbindRow( sdrMtx         * aMtx,
                                   UChar          * aNewSlotPtr,
                                   SShort           aFSCreditSize,
                                   idBool           aDecDelRowCnt );

    static sdpSelfAgingFlag canAgingBySelf(
                                   sdpPhyPageHdr * aPageHdrPtr,
                                   smSCN         * aSCNtoAging );

    static IDE_RC logAndRunSelfAging( idvSQL          * aStatistics,
                                      sdrMtxStartInfo * aStartInfo,
                                      sdpPhyPageHdr   * aPageHdrPtr,
                                      smSCN           * aSCNtoAging);

    static inline void incDelRowCntOfCTL( sdpCTL * aCTL );
    static inline void decDelRowCntOfCTL( sdpCTL * aCTL );
    static inline void incBindCTSCntOfCTL( sdpCTL * aCTL );
    static inline void decBindCTSCntOfCTL( sdpCTL * aCTL,
                                           sdpCTS * aCTS );

    static inline void incBindRowCTSCntOfCTL( sdpCTL * aCTL );
    static inline void decBindRowCTSCntOfCTL( sdpCTL * aCTL );

    static inline void incFSCreditOfCTS( sdpCTS * aCTS,
                                         UShort   aFSCredit );
    static inline void decFSCreditOfCTS( sdpCTS * aCTS,
                                         UShort   aFSCredit );


    static inline idBool hasState( UChar    aState, UChar  aStateSet );
    static inline UInt getCnt( sdpCTL * aHdrPtr );
#if 0  //not used
    static idBool isMyTrans( sdSID         * aTransTSSlotSID,
                             smSCN         * aTransOldestBSCN,
                             sdpPhyPageHdr * aPagePtr,
                             UChar           aCTSlotIdx );
#endif
    static idBool isMyTrans( sdSID         * aTransTSSlotSID,
                             smSCN         * aTransOldestBSCN,
                             sdpCTS        * aCTS );

};

/***********************************************************************
 *
 * Description : CTL헤더로부터 총 CTS 개수 반환
 *
 * aCTL  - [IN] CTL 헤더 포인터
 *
 ***********************************************************************/
inline UInt sdcTableCTL::getCnt( sdpCTL * aCTL )
{
    return aCTL->mTotCTSCnt;
}

/***********************************************************************
 *
 * Description : 페이지 포인터로부터 CTL 헤더 반환
 *
 * aPageHdrPtr - [IN] 페이지 헤더 시작 포인터
 *
 ***********************************************************************/
inline sdpCTL* sdcTableCTL::getCTL( sdpPhyPageHdr  * aPageHdrPtr )
{
    return (sdpCTL*)sdpPhyPage::getCTLStartPtr( (UChar*)aPageHdrPtr );
}

/***********************************************************************
 *
 * Description : 페이지 포인터로부터 CTS 포인터 반환
 *
 * aPageHdrPtr   - [IN] 페이지 헤더 시작 포인터
 * aCTSlotIdx - [IN] 반환할 CTS의 번호
 *
 ***********************************************************************/
inline sdpCTS * sdcTableCTL::getCTS( sdpPhyPageHdr  * aPageHdrPtr,
                                     UChar            aCTSlotIdx )
{
    return getCTS( getCTL(aPageHdrPtr) , aCTSlotIdx );
}

/***********************************************************************
 *
 * Description : CTL 헤더로부터 CTS 포인터 반환
 *
 * aCTL       - [IN] CTL 헤더 포인터
 * aCTSlotIdx - [IN] 반환할 CTS의 번호
 *
 * [ 반환값 ]
 *
 * aCTSlotIdx 에 해당하는 CTS 포인터 반환
 *
 ***********************************************************************/
inline sdpCTS * sdcTableCTL::getCTS( sdpCTL * aHdrPtr,
                                     UChar    aCTSlotIdx )
{
    return (sdpCTS*)( (UChar*)aHdrPtr + ID_SIZEOF( sdpCTL ) +
                      (ID_SIZEOF( sdpCTS ) * aCTSlotIdx ) );
}

/***********************************************************************
 *
 * Description : CTS의 상태 확인
 *
 * aState     - [IN] CTS의 상태
 * aStateSet  - [IN] 확인해볼 상태의 집합
 *
 * [ 반환값 ]
 *
 * CTS의 상태값이 집합중에 하나라도 일치한다면 ID_TRUE를 반환하고
 * 그렇지 않다면, ID_FALSE를 반환한다.
 *
 * 예를들어, CTS의 상태가 SDP_CTS_STAT_RTS라고 할때,
 *         확인하고자하는 StateSet이 (SDP_CTS_STAT_RTS|SDC_CTS_STAT_CTS)
 *         라고 한다면 하나의 상태가 일치하기 때문에 TRUE를 반환한다.
 *
 ***********************************************************************/
inline idBool sdcTableCTL::hasState( UChar    aState,
                                     UChar    aStateSet )
{
    idBool sHasState;

    if ( ( aState & aStateSet ) != 0 )
    {
        sHasState = ID_TRUE;
    }
    else
    {
        sHasState = ID_FALSE;
    }

    return sHasState;
}

/***********************************************************************
 *
 * Description : CTS의 Free Space Credit 값을 증가
 *
 * 트랜잭션의 롤백을 대비하여 트랜잭션 완료전까지 반드시 확보해 두어야 하는 페이지
 * 가용공간을 누적시켜둔다. 해당 트랜잭션이 완료되기 전까지는 해당 페이지에서 누적된
 * 가용공간이 해제되어 다른 트랜잭션에 의해서 할당되지 못하도록 한다.
 *
 * 즉, 트랜잭션이 롤백하는 경우에 반드시 롤백이 성공해야하기 때문이다.
 *
 * aCTS       - [IN] CTS 포인터
 * aFSCredit  - [IN] 증가시킬 FreeSpaceCredit 크기 (>0)
 *
 ***********************************************************************/
inline void sdcTableCTL::incFSCreditOfCTS( sdpCTS   * aCTS,
                                           UShort     aFSCredit )
{
   IDE_DASSERT( aCTS != NULL );
   aCTS->mFSCredit += aFSCredit;

   IDE_ASSERT( aCTS->mFSCredit < SD_PAGE_SIZE );
}

/***********************************************************************
 *
 * Description : CTS의 Free Space Credit 값을 감소시킴
 *
 * 트랜잭션의 롤백을 대비하여 트랜잭션 완료전까지 반드시 확보해 두어야 하는
 * 페이지 가용공간을 해제한만큼 CTS의 Free Space Credit를 빼준다.
 *
 * 트랜잭션이 롤백을 하거나 커밋이후에 RowStamping 과정에서 Free Space Credit는
 * 해제된다.
 *
 * aCTS       - [IN] CTS 포인터
 * aFSCredit  - [IN] 감소시킬 FreeSpaceCredit 크기 (>0)
 *
 ***********************************************************************/
inline void sdcTableCTL::decFSCreditOfCTS( sdpCTS  * aCTS,
                                           UShort    aFSCredit )
{
    IDE_DASSERT( aCTS != NULL );

    IDE_ASSERT( aCTS->mFSCredit >= aFSCredit );
    aCTS->mFSCredit -= aFSCredit;
}

/***********************************************************************
 *
 * Description : CTL의 Delete중인 혹은 Delete된 Row Piece의 개수를 증가
 *
 * 해당 데이타 페이지에 Self-Aging이 필요할지도 모르는 Deleted Row
 * Piece 개수를 증가시킨다
 *
 * aCTL       - [IN] CTL 포인터
 *
 ***********************************************************************/
inline void sdcTableCTL::incDelRowCntOfCTL( sdpCTL * aCTL )
{
    IDE_DASSERT( aCTL != NULL );
    aCTL->mDelRowCnt += 1;
}

/***********************************************************************
 *
 * Description : CTL의 Delete중인 혹은 Delete된 Row Piece의 개수를 감소
 *
 * 해당 데이타 페이지에 Self-Aging을 처리하고 나서 혹은 Delete 연산이 롤백하는 경우에
 * Piece 개수를 감소시킨다.
 *
 * aCTL       - [IN] CTL 포인터
 *
 ***********************************************************************/
inline void sdcTableCTL::decDelRowCntOfCTL( sdpCTL * aCTL )
{
    IDE_DASSERT( aCTL->mDelRowCnt > 0 );
    aCTL->mDelRowCnt -= 1;
}

/***********************************************************************
 *
 * Description : CTL의 바인딩 된 CTS의 개수를 1증가시킨다.
 *
 * aCTL       - [IN] CTL 포인터
 *
 ************************************************************************/
inline void sdcTableCTL::incBindCTSCntOfCTL( sdpCTL * aCTL )
{
    IDE_DASSERT( aCTL != NULL );
    aCTL->mBindCTSCnt++;
}

/***********************************************************************
 *
 * Description : CTL의 바인딩 된 CTS의 개수를 1감소시킨다.
 *
 * aCTL       - [IN] CTL 포인터
 * aCTS       - [IN] CTS 포인터
 *
 ************************************************************************/
inline void sdcTableCTL::decBindCTSCntOfCTL( sdpCTL * aCTL,
                                             sdpCTS * aCTS )
{
    IDE_DASSERT( aCTL != NULL );
    IDE_DASSERT( aCTS != NULL );

    IDE_ASSERT( aCTL->mBindCTSCnt > 0 );

    if ( aCTS->mRefCnt == 0 )
    {
        if ( hasState( aCTS->mStat, SDP_CTS_STAT_ACT ) == ID_TRUE )
        {
            /* Unbind CTS */
            aCTS->mStat = SDP_CTS_STAT_ROL;
            IDE_ASSERT( aCTS->mFSCredit == 0 );
        }
        else
        {
            /* Run row timestamping */
            IDE_ASSERT( hasState( aCTS->mStat, SDP_CTS_STAT_RTS )
                        == ID_TRUE );
        }

        aCTL->mBindCTSCnt--;
    }
}

/***********************************************************************
 *
 * Description : CTL의 Row에 바인딩 된 CTS의 개수를 1증가시킨다.
 *
 * aCTL       - [IN] CTL 포인터
 *
 ************************************************************************/
inline void sdcTableCTL::incBindRowCTSCntOfCTL( sdpCTL * aCTL )
{
    IDE_DASSERT( aCTL != NULL );
    aCTL->mRowBindCTSCnt++;
}

/***********************************************************************
 *
 * Description : CTL의 Row바인딩 된 CTS의 개수를 1감소시킨다.
 *
 * aCTL       - [IN] CTL 포인터
 * aCTS       - [IN] CTS 포인터
 *
 ************************************************************************/
inline void sdcTableCTL::decBindRowCTSCntOfCTL( sdpCTL * aCTL )
{
    IDE_DASSERT( aCTL != NULL );

    IDE_ASSERT( aCTL->mRowBindCTSCnt > 0 );
    aCTL->mRowBindCTSCnt--;
}

# endif // _O_SDC_TABLE_CTL_H_
