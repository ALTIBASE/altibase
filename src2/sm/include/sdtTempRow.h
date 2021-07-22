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
 * $Id: $
 **********************************************************************/

#ifndef O_SDT_TEMP_ROW_H_
#define O_SDT_TEMP_ROW_H_ 1

#include <smDef.h>
#include <sdtDef.h>
#include <smiDef.h>
#include <smiTempTable.h>
#include <sdtTempPage.h>
#include <sdtSortSegment.h>

class sdtTempRow
{
public:

    inline static IDE_RC append( sdtSortSegHdr     * aWASegment,
                                 sdtGroupID          aWAGroupID,
                                 sdtTempPageType     aPageType,
                                 UInt                aCuttingOffset,
                                 sdtSortInsertInfo * aTRPInfo,
                                 sdtSortInsertResult * aTRInsertResult );

    static IDE_RC allocNewPage( sdtSortSegHdr    * aWASegment,
                                sdtGroupID         aWAGroupID,
                                sdtWCB           * aPrevWCBPtr,
                                sdtTempPageType    aPageType,
                                UInt             * aRowPageCount,
                                sdtWCB          ** aNewWCBPtr );

    static void   initTRInsertResult( sdtSortInsertResult * aTRInsertResult );

    inline static IDE_RC appendRowPiece(sdtSortSegHdr     * aWASegment,
                                        sdtWCB            * aWCBPtr,
                                        UInt                aCuttingOffset,
                                        sdtSortInsertInfo * aTRPInfo,
                                        sdtSortInsertResult * aTRInsertResult );

    inline static IDE_RC fetch( sdtSortSegHdr   * aWASegment,
                                sdtGroupID        aGroupID,
                                UChar           * aRowBuffer,
                                sdtSortScanInfo * aTRPInfo );

    static IDE_RC fetchChainedRowPiece( sdtSortSegHdr   * aWASegment,
                                        sdtGroupID        aGroupID,
                                        UChar           * aRowBuffer,
                                        sdtSortScanInfo * aTRPInfo );

    static IDE_RC fetchByGRID( sdtSortSegHdr   * aWASegment,
                               sdtGroupID        aGroupID,
                               scGRID            aGRID,
                               UInt              aValueLength,
                               UChar           * aRowBuffer,
                               sdtSortScanInfo * aTRPInfo );

    inline static IDE_RC filteringAndFetchByGRID( smiSortTempCursor* aTempCursor,
                                                  scGRID          aTargetGRID,
                                                  UChar        ** aRow,
                                                  scGRID        * aRowGRID,
                                                  idBool        * aResult);

    inline static IDE_RC filteringAndFetch( smiSortTempCursor * aTempCursor,
                                            sdtSortTRPHdr  * aTRPHeader,
                                            UChar         ** aRow,
                                            scGRID         * aRowGRID,
                                            idBool         * aResult);

private:

    static IDE_RC updateChainedRowPiece( smiSortTempCursor* aTempCursor,
                                         smiValue         * aValueList );
public:
    inline static IDE_RC update( smiSortTempCursor* aTempCursor,
                                 smiValue         * aValueList );
    inline static void setHitFlag( smiSortTempCursor* aTempCursor,
                                   UInt               aHitSeq );
    inline static idBool isHitFlagged( smiSortTempCursor* aTempCursor,
                                       ULong           aHitSeq );

public:
    /* Dump함수들 */
    static void dumpTempTRPHeader( void       * aTRPHeader,
                                   SChar      * aOutBuf,
                                   UInt         aOutSize );
    static void dumpTempTRPInfo4Insert( void       * aTRPInfo,
                                        SChar      * aOutBuf,
                                        UInt         aOutSize );
    static void dumpTempPageByRow( void  * aPagePtr,
                                   SChar * aOutBuf,
                                   UInt    aOutSize );
    static void dumpRowWithCursor( void   * aTempCursor,
                                   SChar  * aOutBuf,
                                   UInt     aOutSize );
};

/**************************************************************************
 * Description :
 * 해당 WAPage에 Row를 삽입한다.
 * 만약 공간이 부족해 Row가 남았다면 뒤쪽부터 Row를 삽입하고 남은 RowValue값과
 * 지금 삽입한 위치의 RID를 구조체로 묶어 반환한다 . 이때 rowPiece 구조체는
 * rowValue를 smiValue형태로 가지고 있는데, 이 value는 원본 smiValue를
 * pointing하는 형태이다. 만약 해당 Slot에 이미 Value가 있다면, 같은 Row로
 * 판단하고 그냥 밀어넣는다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWAGroupID     - 대상 Group ID
 * aPageType      - 복사 Page의 Type
 * aCuttingOffset - 이 값 뒤쪽의 Row들만 복사한다. 앞쪽은 강제로 남긴다.
 *                  sdtSortModule::copyExtraRow 참조
 * aTRPInfo       - 삽입할 Row
 * <OUT>
 * aTRInsertResult- 삽입한 결과
 ***************************************************************************/
IDE_RC sdtTempRow::append( sdtSortSegHdr     * aWASegment,
                           sdtGroupID          aWAGroupID,
                           sdtTempPageType     aPageType,
                           UInt                aCuttingOffset,
                           sdtSortInsertInfo * aTRPInfo,
                           sdtSortInsertResult * aTRInsertResult )
{
    UInt            sRowPageCount = 1;
    sdtWCB        * sWCBPtr;
    sdtSortGroup  * sGroupInfo = sdtSortSegment::getWAGroupInfo( aWASegment,
                                                                 aWAGroupID );

    IDE_DASSERT( SDT_TR_HEADER_SIZE_BASE ==
                 ( IDU_FT_SIZEOF( sdtSortTRPHdr, mTRFlag )
                   + IDU_FT_SIZEOF( sdtSortTRPHdr, mDummy )
                   + IDU_FT_SIZEOF( sdtSortTRPHdr, mHitSequence )
                   + IDU_FT_SIZEOF( sdtSortTRPHdr, mValueLength ) ) );

    IDE_DASSERT( SDT_TR_HEADER_SIZE_FULL ==
                 ( SDT_TR_HEADER_SIZE_BASE +
                   + IDU_FT_SIZEOF( sdtSortTRPHdr, mNextGRID )
                   + IDU_FT_SIZEOF( sdtSortTRPHdr, mChildGRID )) );

    idlOS::memset( aTRInsertResult, 0, ID_SIZEOF( sdtSortInsertResult ) );

    sWCBPtr = sdtSortSegment::getHintWCB( sGroupInfo );

    if ( sWCBPtr == NULL )
    {
        /* 최초삽입인 경우 */
        IDE_TEST( allocNewPage( aWASegment,
                                aWAGroupID,
                                sWCBPtr,
                                aPageType,
                                &sRowPageCount,
                                &sWCBPtr )
                  != IDE_SUCCESS );
    }

    while( sWCBPtr != NULL )
    {
        IDE_TEST( appendRowPiece( aWASegment,
                                  sWCBPtr,
                                  aCuttingOffset,
                                  aTRPInfo,
                                  aTRInsertResult )
                  != IDE_SUCCESS );

        IDE_DASSERT(( sGroupInfo->mPolicy != SDT_WA_REUSE_INMEMORY ) ||
                    (( sWCBPtr->mNPageID == SM_NULL_PID ) &&
                     ( aTRInsertResult->mHeadRowpieceGRID.mSpaceID != aWASegment->mSpaceID )));

        if ( aTRInsertResult->mComplete == ID_TRUE )
        {
            break;
        }

        IDE_TEST( allocNewPage( aWASegment,
                                aWAGroupID,
                                sWCBPtr,
                                aPageType,
                                &sRowPageCount,
                                &sWCBPtr )
                  != IDE_SUCCESS );
    }

    sdtSortSegment::setHintWCB( sGroupInfo, sWCBPtr );

    aTRInsertResult->mRowPageCount = sRowPageCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    smuUtility::dumpFuncWithBuffer( IDE_DUMP_0,
                                    dumpTempTRPInfo4Insert,
                                    aTRPInfo );

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * RowPiece하나를 한 페이지에 삽입한다.
 * Append에서 사용한다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWPID          - 삽입할 대상 WPID
 * aWPagePtr      - 삽입할 대상 Page의 Pointer 위치
 * aSlotNo        - 삽입할 대상 Slot
 * aCuttingOffset - 이 값 뒤쪽의 Row들만 복사한다. 앞쪽은 강제로 남긴다.
 *                  sdtSortModule::copyExtraRow 참조
 * aTRPInfo       - 삽입할 Row
 * <OUT>
 * aTRInsertResult- 삽입한 결과
 ***************************************************************************/
IDE_RC sdtTempRow::appendRowPiece( sdtSortSegHdr     * aWASegment,
                                   sdtWCB            * aWCBPtr,
                                   UInt                aCuttingOffset,
                                   sdtSortInsertInfo * aTRPInfo,
                                   sdtSortInsertResult * aTRInsertResult )
{
    UInt           sRowPieceSize;
    UInt           sRowPieceHeaderSize;
    UInt           sMinSize;
    UInt           sSlotSize;
    UChar        * sSlotPtr;
    UChar        * sRowPtr;
    UChar        * sWAPagePtr;
    UInt           sBeginOffset;
    UInt           sEndOffset;
    sdtSortTRPHdr* sTRPHeader;
    scPageID       sWPID;
    scSlotNum      sSlotNo;

    IDE_ERROR( aWCBPtr != NULL );

    sWAPagePtr = aWCBPtr->mWAPagePtr;
    sSlotNo = sdtTempPage::allocSlotDir( sWAPagePtr );

    /* Slot할당이 실패한 경우 */
    IDE_TEST_CONT( sSlotNo == SDT_TEMP_SLOT_UNUSED, SKIP );

    sWPID = sdtSortSegment::getWPageID(aWASegment, aWCBPtr);
    /* FreeSpace계산 */
    sRowPieceHeaderSize = SDT_TR_HEADER_SIZE( aTRPInfo->mTRPHeader.mTRFlag );
    sRowPieceSize       = sRowPieceHeaderSize + aTRPInfo->mValueLength;

    IDE_ERROR_MSG( aCuttingOffset < sRowPieceSize,
                   "CuttingOffset : %"ID_UINT32_FMT"\n"
                   "RowPieceSize  : %"ID_UINT32_FMT"\n",
                   aCuttingOffset,
                   sRowPieceHeaderSize );

    if ( SM_IS_FLAG_ON( aTRPInfo->mTRPHeader.mTRFlag, SDT_TRFLAG_UNSPLIT ) )
    {
        sMinSize = sRowPieceSize - aCuttingOffset;
    }
    else
    {
        sMinSize = IDL_MIN( smuProperty::getTempRowSplitThreshold(),
                            sRowPieceSize - aCuttingOffset);
    }

    /* Slot할당을 시도함 */
    sSlotPtr = sdtTempPage::allocSlot( (sdtSortPageHdr*)sWAPagePtr,
                                       sSlotNo,
                                       sRowPieceSize - aCuttingOffset,
                                       sMinSize,
                                       &sSlotSize );
    /* 할당 실패 */
    IDE_TEST_CONT( sSlotSize == 0, SKIP );

    /* 할당한 Slot 설정함 */
    aTRInsertResult->mHeadRowpiecePtr = (sdtSortTRPHdr*)sSlotPtr;

    if ( sSlotSize == sRowPieceSize )
    {
        /* Cutting할 것도 없고, 그냥 전부 Copy하면 됨 */
        aTRInsertResult->mComplete = ID_TRUE;

        /*Header 기록 */
        sTRPHeader = (sdtSortTRPHdr*)sSlotPtr;
        idlOS::memcpy( sTRPHeader,
                       &aTRPInfo->mTRPHeader,
                       sRowPieceHeaderSize );
        sTRPHeader->mValueLength = aTRPInfo->mValueLength;

        /* 원본 Row의 시작 위치 */
        sRowPtr = ((UChar*)aTRPInfo->mValueList[ 0 ].value) - aTRPInfo->mColumns[ 0 ].mColumn.offset;

        idlOS::memcpy( sSlotPtr + sRowPieceHeaderSize,
                       sRowPtr,
                       aTRPInfo->mValueLength );

        /* 삽입 완료함 */
        aTRPInfo->mValueLength = 0;

        SC_MAKE_GRID( aTRInsertResult->mHeadRowpieceGRID,
                      SDT_SPACEID_WORKAREA,
                      sWPID,
                      sSlotNo );

        sdtSortSegment::convertFromWGRIDToNGRID( aWASegment,
                                                 aTRInsertResult->mHeadRowpieceGRID,
                                                 &aTRInsertResult->mHeadRowpieceGRID );

        SC_MAKE_NULL_GRID( aTRPInfo->mTRPHeader.mNextGRID );
    }
    else
    {
        /* Row가 쪼개질 수 있음 */
        /*********************** Range 계산 ****************************/
        /* 기록하는 OffsetRange를 계산함 */
        sBeginOffset = sRowPieceSize - sSlotSize; /* 요구량이 부족한 만큼 */
        sEndOffset   = aTRPInfo->mValueLength;
        IDE_ERROR( sEndOffset > sBeginOffset );

        /*********************** Header 기록 **************************/
        sTRPHeader = (sdtSortTRPHdr*)sSlotPtr;
        aTRInsertResult->mHeadRowpiecePtr = sTRPHeader;

        idlOS::memcpy( sTRPHeader,
                       &aTRPInfo->mTRPHeader,
                       sRowPieceHeaderSize );
        sTRPHeader->mValueLength = sEndOffset - sBeginOffset;

        if ( sBeginOffset == aCuttingOffset )
        {
            /* 요청된 Write 완료 */
            aTRInsertResult->mComplete = ID_TRUE;
        }
        else
        {
            aTRInsertResult->mComplete = ID_FALSE;
        }

        if ( sBeginOffset > 0 )
        {
            /* 남은 앞쪽 부분이 남아는 있으면,
             * 리턴할 TRPHeader에는 NextGRID를 붙이고 (다음에 연결하라고)
             * 기록할 TRPHeader에는 Head를 땐다. ( Head가 아니니까. ) */
            SM_SET_FLAG_ON( aTRPInfo->mTRPHeader.mTRFlag,
                            SDT_TRFLAG_NEXTGRID );
            SM_SET_FLAG_OFF( sTRPHeader->mTRFlag,
                             SDT_TRFLAG_HEAD );
        }

        sSlotPtr += sRowPieceHeaderSize;

        /* 원본 Row의 시작 위치 */
        sRowPtr = ((UChar*)aTRPInfo->mValueList[ 0 ].value)
            - aTRPInfo->mColumns[ 0 ].mColumn.offset;
        idlOS::memcpy( sSlotPtr,
                       sRowPtr + sBeginOffset,
                       sEndOffset - sBeginOffset );

        aTRPInfo->mValueLength -= sTRPHeader->mValueLength;

        SC_MAKE_GRID( aTRInsertResult->mHeadRowpieceGRID,
                      SDT_SPACEID_WORKAREA,
                      sWPID,
                      sSlotNo );

        sdtSortSegment::convertFromWGRIDToNGRID(
            aWASegment,
            aTRInsertResult->mHeadRowpieceGRID,
            &aTRInsertResult->mHeadRowpieceGRID );

        if ( SM_IS_FLAG_ON( aTRPInfo->mTRPHeader.mTRFlag, SDT_TRFLAG_NEXTGRID ) )
        {
            SC_COPY_GRID( aTRInsertResult->mHeadRowpieceGRID,
                          aTRPInfo->mTRPHeader.mNextGRID );
        }
        else
        {
            SC_MAKE_NULL_GRID( aTRPInfo->mTRPHeader.mNextGRID );
        }
    }

    if( aWCBPtr->mWPState != SDT_WA_PAGESTATE_INIT )
    {
        aWCBPtr->mWPState = SDT_WA_PAGESTATE_DIRTY;
    }
    IDE_EXCEPTION_CONT( SKIP );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * Row의 특정 Column을 갱신한다.
 * Update할 Row는 처음 삽입할때부터 충분한 공간을 확보했기 때문에 반드시
 * Row의 구조 변경 없이 성공한다
 *
 * <IN>
 * aTempCursor    - 대상 커서
 * aValue         - 갱신할 Value
 ***************************************************************************/
IDE_RC sdtTempRow::update( smiSortTempCursor * aTempCursor,
                           smiValue          * aValueList )
{
    const smiColumnList * sUpdateColumn;
    const smiColumn * sColumn;
    UChar           * sRowPos      = NULL;
    sdtSortTRPHdr   * sTRPHeader   = NULL;
    sdtWCB          * sWCBPtr;

    if( aTempCursor->mTTHeader->mCheckCnt > SMI_TT_STATS_INTERVAL )
    {
        IDE_TEST( iduCheckSessionEvent( aTempCursor->mTTHeader->mStatistics ) != IDE_SUCCESS);
        aTempCursor->mTTHeader->mCheckCnt = 0;
    }

    sTRPHeader = (sdtSortTRPHdr*)aTempCursor->mRowPtr;

    IDE_ERROR( sTRPHeader != NULL );
    IDE_ERROR( SM_IS_FLAG_ON( sTRPHeader->mTRFlag, SDT_TRFLAG_HEAD ) );

    if ( sTRPHeader->mValueLength >= aTempCursor->mUpdateEndOffset )
    {
        sRowPos = aTempCursor->mRowPtr + SDT_TR_HEADER_SIZE( sTRPHeader->mTRFlag ) ;
        sUpdateColumn = aTempCursor->mUpdateColumns;

        while( sUpdateColumn != NULL )
        {
            sColumn = ( smiColumn*)sUpdateColumn->column;

            IDE_ERROR( (sColumn->offset + aValueList->length )
                       <= sTRPHeader->mValueLength ); // 넘어서면 잘못된 것이다.

            idlOS::memcpy( sRowPos + sColumn->offset,
                           ((UChar*)aValueList->value),
                           aValueList->length );

            aValueList++;
            sUpdateColumn = sUpdateColumn->next;
        }

        if ( ( SC_GRID_IS_NULL( aTempCursor->mGRID ) ) ||
             ( SC_MAKE_SPACE( aTempCursor->mGRID ) == SDT_SPACEID_WORKAREA ) ||
             ( SC_MAKE_SPACE( aTempCursor->mGRID ) == SDT_SPACEID_WAMAP ) )
        {
            // InMemory 연산이기에 SetDirty가 필요 없다
            IDE_DASSERT((((smiTempTableHeader*)aTempCursor->mTTHeader)->mTTState != SMI_TTSTATE_SORT_MERGE ) &&
                        (((smiTempTableHeader*)aTempCursor->mTTHeader)->mTTState != SMI_TTSTATE_SORT_MERGESCAN ));
        }
        else
        {
            sWCBPtr = sdtSortSegment::findWCB( (sdtSortSegHdr*)aTempCursor->mWASegment,
                                               SC_MAKE_PID( aTempCursor->mGRID ));
            IDE_ERROR( sWCBPtr != NULL );
            IDE_ASSERT( sWCBPtr->mWPState != SDT_WA_PAGESTATE_INIT );

            sWCBPtr->mWPState = SDT_WA_PAGESTATE_DIRTY;
        }
    }
    else
    {
        IDE_TEST( updateChainedRowPiece( aTempCursor,
                                         aValueList ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    smuUtility::dumpFuncWithBuffer( IDE_DUMP_0,
                                    dumpRowWithCursor,
                                    aTempCursor );

    smiTempTable::checkAndDump( aTempCursor->mTTHeader );

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * page에 존재하는 Row 전체를 rowInfo형태로 읽는다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWAGroupID     - 대상 Group ID
 * aRowPtr        - Row의 위치 Pointer
 * aValueLength   - Row의 Value부분의 총 길이
 * aRowBuffer     - 쪼개진 Column을 저장하기 위한 Buffer
 * <OUT>
 * aTRPInfo       - Fetch한 결과
 ***************************************************************************/
IDE_RC sdtTempRow::fetch( sdtSortSegHdr        * aWASegment,
                          sdtGroupID             aGroupID,
                          UChar                * aRowBuffer,
                          sdtSortScanInfo      * aTRPInfo )
{
    UChar sFlag = aTRPInfo->mTRPHeader->mTRFlag;

    IDE_DASSERT( SM_IS_FLAG_ON( sFlag, SDT_TRFLAG_HEAD ) );

    if ( SM_IS_FLAG_ON( sFlag, SDT_TRFLAG_NEXTGRID ) )
    {
        IDE_TEST( fetchChainedRowPiece( aWASegment,
                                        aGroupID,
                                        aRowBuffer,
                                        aTRPInfo )
                  != IDE_SUCCESS );
    }
    else
    {
        aTRPInfo->mValuePtr    = ((UChar*)aTRPInfo->mTRPHeader) + SDT_TR_HEADER_SIZE( sFlag );
        aTRPInfo->mValueLength = aTRPInfo->mTRPHeader->mValueLength;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * Filtering거치고 fetch함.
 *
 * <IN>
 * aTempCursor    - 대상 커서
 * aTargetGRID    - 대상 Row의 GRID
 * <OUT>
 * aRow           - 대상 Row를 담을 Bufer
 * aRowGRID       - 대상 Row의 실제 GRID
 * aResult        - Filtering 통과 여부
 ***************************************************************************/
IDE_RC sdtTempRow::filteringAndFetchByGRID( smiSortTempCursor * aTempCursor,
                                            scGRID           aTargetGRID,
                                            UChar         ** aRow,
                                            scGRID         * aRowGRID,
                                            idBool         * aResult)
{
    smiTempTableHeader * sHeader = aTempCursor->mTTHeader;
    sdtSortSegHdr      * sWASeg  = (sdtSortSegHdr*)sHeader->mWASegment;
    sdtSortTRPHdr      * sTRPHeader;

    /* 실제 Fetch하기 전에 먼저 PieceHeader만 보고 First인지 확인 */
    IDE_TEST( sdtSortSegment::getPagePtrByGRID( sWASeg,
                                                aTempCursor->mWAGroupID,
                                                aTargetGRID,
                                                (UChar**)&sTRPHeader )
              != IDE_SUCCESS );

    IDE_DASSERT( SM_IS_FLAG_ON( sTRPHeader->mTRFlag, SDT_TRFLAG_HEAD ) );

    IDE_TEST( filteringAndFetch( aTempCursor,
                                 sTRPHeader,
                                 aRow,
                                 aRowGRID,
                                 aResult )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * Filtering거치고 fetch함.
 *
 * <IN>
 * aTempCursor    - 대상 커서
 * aTargetPtr     - 대상 Row의 Ptr
 * <OUT>
 * aRow           - 대상 Row를 담을 Bufer
 * aRowGRID       - 대상 Row의 실제 GRID
 * aResult        - Filtering 통과 여부
 ***************************************************************************/
IDE_RC sdtTempRow::filteringAndFetch( smiSortTempCursor * aTempCursor,
                                      sdtSortTRPHdr  * aTRPHeader,
                                      UChar         ** aRow,
                                      scGRID         * aRowGRID,
                                      idBool         * aResult)
{
    smiTempTableHeader * sHeader;
    sdtSortSegHdr      * sWASeg;
    sdtSortScanInfo      sTRPInfo;
    UInt                 sFlag;
    idBool               sHit;
    idBool               sNeedHit;

    *aResult = ID_FALSE;

    sHeader = aTempCursor->mTTHeader;
    sWASeg  = (sdtSortSegHdr*)aTempCursor->mWASegment;

    IDE_DASSERT( SM_IS_FLAG_ON( aTRPHeader->mTRFlag, SDT_TRFLAG_HEAD ) );

    sFlag = aTempCursor->mTCFlag;

    /* HItFlag체크가 필요한가? */
    if ( sFlag & SMI_TCFLAG_HIT_MASK )
    {
        /* Hit연산은 크게 세 Flag이다.
         * IgnoreHit -> Hit자체를 상관 안함
         * Hit       -> Hit된 것만 가져옴
         * NoHit     -> Hit 안된 것만 가져옴
         *
         * 위 if문은 IgnoreHit가 아닌지를 구분하는 것이고.
         * 아래 IF문은 Hit인지 NoHit인지 구분하는 것이다. */
        sNeedHit = ( sFlag & SMI_TCFLAG_HIT ) ? ID_TRUE : ID_FALSE;
        sHit = ( aTRPHeader->mHitSequence == sHeader->mHitSequence ) ? ID_TRUE : ID_FALSE;

        IDE_TEST_CONT( sHit != sNeedHit, SKIP );
    }
    else
    {
        /* nothing to do */
    }

    sTRPInfo.mTRPHeader      = aTRPHeader;
    sTRPInfo.mFetchEndOffset = sHeader->mRowSize;

    IDE_TEST( fetch( sWASeg,
                     aTempCursor->mWAGroupID,
                     sHeader->mRowBuffer4Fetch,
                     &sTRPInfo )
              != IDE_SUCCESS );

    IDE_DASSERT( sTRPInfo.mValueLength <= sHeader->mRowSize );

    if ( sFlag & SMI_TCFLAG_FILTER_KEY )
    {
        IDE_TEST( aTempCursor->mKeyFilter->minimum.callback(
                      aResult,
                      sTRPInfo.mValuePtr,
                      NULL,
                      0,
                      SC_NULL_GRID,
                      aTempCursor->mKeyFilter->minimum.data )
                  != IDE_SUCCESS );
        IDE_TEST_CONT( *aResult == ID_FALSE, SKIP );

        IDE_TEST( aTempCursor->mKeyFilter->maximum.callback(
                      aResult,
                      sTRPInfo.mValuePtr,
                      NULL,
                      0,
                      SC_NULL_GRID,
                      aTempCursor->mKeyFilter->maximum.data )
                  != IDE_SUCCESS );
        IDE_TEST_CONT( *aResult == ID_FALSE, SKIP );
    }
    else
    {
        /* nothing to do */
    }

    if ( sFlag & SMI_TCFLAG_FILTER_ROW )
    {
        IDE_TEST( aTempCursor->mRowFilter->callback(
                      aResult,
                      sTRPInfo.mValuePtr,
                      NULL,
                      0,
                      SC_NULL_GRID,
                      aTempCursor->mRowFilter->data )
                  != IDE_SUCCESS );
        IDE_TEST_CONT( *aResult == ID_FALSE, SKIP );
    }
    else
    {
        /* nothing to do */
    }

    *aResult             = ID_TRUE;
    aTempCursor->mRowPtr = (UChar*)aTRPHeader;
    SC_COPY_GRID( aTempCursor->mGRID, *aRowGRID );

    sdtSortSegment::convertFromWGRIDToNGRID( sWASeg,
                                             *aRowGRID,
                                             aRowGRID );

    idlOS::memcpy( *aRow,
                   sTRPInfo.mValuePtr,
                   sTRPInfo.mValueLength );

    IDE_EXCEPTION_CONT( SKIP );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/**************************************************************************
 * Description :
 * HitFlag를 설정합니다.
 *
 * <IN>
 * aCursor      - 갱신할 Row를 가리키는 커서
 * aHitSeq      - 설정할 Hit값
 ***************************************************************************/
void sdtTempRow::setHitFlag( smiSortTempCursor* aTempCursor,
                             UInt               aHitSeq )
{
    sdtSortSegHdr * sWASeg     = (sdtSortSegHdr*)aTempCursor->mTTHeader->mWASegment;
    sdtSortTRPHdr * sTRPHeader = (sdtSortTRPHdr*)aTempCursor->mRowPtr;
    sdtWCB        * sWCBPtr;
    scSpaceID       sSpaceID      = SC_MAKE_SPACE( aTempCursor->mGRID );
    smiTempTableHeader * sHeader = aTempCursor->mTTHeader;

#ifdef DEBUG
    if ( sHeader->mTTState == SMI_TTSTATE_SORT_INMEMORYSCAN )
    {
        IDE_DASSERT( sHeader->mFetchGroupID == SDT_WAGROUPID_NONE );
        IDE_DASSERT( sSpaceID == SDT_SPACEID_WAMAP );
    }
    else
    {
        if ( ( sHeader->mTTState == SMI_TTSTATE_SORT_INDEXSCAN ) ||
             ( sHeader->mTTState == SMI_TTSTATE_SORT_SCAN )  )
        {
            IDE_DASSERT( ( sSpaceID != SDT_SPACEID_WAMAP ) &&
                         ( sSpaceID != SDT_SPACEID_WORKAREA ) );
        }
        else
        {
            IDE_DASSERT( ( sHeader->mTTState == SMI_TTSTATE_SORT_MERGE ) ||
                         ( sHeader->mTTState == SMI_TTSTATE_SORT_MERGESCAN ) );
        }
    }
    IDE_DASSERT( SM_IS_FLAG_ON( sTRPHeader->mTRFlag, SDT_TRFLAG_HEAD ) );
    IDE_DASSERT( ID_SIZEOF( sTRPHeader->mHitSequence ) == ID_SIZEOF( aHitSeq ) );
#endif

    if ( sTRPHeader->mHitSequence != aHitSeq )
    {
        sTRPHeader->mHitSequence = aHitSeq;

        if ( ( SC_GRID_IS_NULL( aTempCursor->mGRID ) ) ||
             ( sSpaceID == SDT_SPACEID_WAMAP ) )
        {
            /* InMemory 연산이기에 SetDirty가 필요 없다 */
        }
        else
        {
            if ( sSpaceID == SDT_SPACEID_WORKAREA )
            {
                /* BUG-44074 merge 중간에 호출되는 경우 GRID 는 heap에 있는 run 정보.
                   run 을 따라가서 setdirty 안해주면 해당 페이지에 hitseq 설정한 정보가 사라질수 있다. */
                if ( ( sHeader->mTTState == SMI_TTSTATE_SORT_MERGE ) ||
                     ( sHeader->mTTState == SMI_TTSTATE_SORT_MERGESCAN ) )
                {
                    /* 반드시 있어야 한다. */
                    sWCBPtr = sdtSortSegment::findWCB( sWASeg,
                                                       SC_MAKE_PID( aTempCursor->mGRID ));
                    IDE_ASSERT( sWCBPtr != NULL );
                    IDE_ASSERT( sWCBPtr->mWPState != SDT_WA_PAGESTATE_INIT );

                    sWCBPtr->mWPState = SDT_WA_PAGESTATE_DIRTY;
                }
                else
                {
                    /* nothing to do */
                }
            }
            else
            {
                /* setDirty! */
                sWCBPtr = sdtSortSegment::findWCB( sWASeg,
                                                   SC_MAKE_PID( aTempCursor->mGRID ));
                IDE_ASSERT( sWCBPtr != NULL );
                IDE_ASSERT( sWCBPtr->mWPState != SDT_WA_PAGESTATE_INIT );

                sWCBPtr->mWPState = SDT_WA_PAGESTATE_DIRTY;
            }
        }//else
    }
    else
    {
        // ( sTRPHeader->mHitSequence == aHitSeq )
        // 값이 같으면 처리 할 필요없다.
    }

    return;
}

/**************************************************************************
 * Description :
 * HitFlag가 있는지 검사한다.
 *
 * <IN>
 * aHitSeq            - 현재 Hit Sequence
 * aIsHitFlagged      - Hit Flag 여부
 ***************************************************************************/
idBool sdtTempRow::isHitFlagged( smiSortTempCursor * aTempCursor,
                                 ULong               aHitSeq )
{
    sdtSortTRPHdr* sTRPHeader = (sdtSortTRPHdr*)aTempCursor->mRowPtr;

    IDE_DASSERT( ID_SIZEOF( sTRPHeader->mHitSequence ) ==
                 ID_SIZEOF( aHitSeq ) );

    // No Type Casting. (because of above assertion code)
    if ( sTRPHeader->mHitSequence == aHitSeq )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

#endif
