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
 * $Id $
 **********************************************************************/

#include <sdtTempRow.h>

/**************************************************************************
 * Description :
 * 삽입할 새 페이지를 할당받음
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWAGroupID     - 대상 Group ID
 * aPrevWCBPtr    - 새롭게 할당한 페이지의 이전 page의 WCB
 * aPageType      - 새 Page의 Type
 * <OUT>
 * aRowPageCount  - Row 하나를 기록하기 위해 할당한 Page의 개수
 * aNewWCBPtr     - 할당받은 새 페이지의 WCB Ptr
 ***************************************************************************/
IDE_RC sdtTempRow::allocNewPage( sdtSortSegHdr    * aWASegment,
                                 sdtGroupID         aWAGroupID,
                                 sdtWCB           * aPrevWCBPtr,
                                 sdtTempPageType    aPageType,
                                 UInt             * aRowPageCount,
                                 sdtWCB          ** aNewWCBPtr )
{
    scPageID        sTargetWPID = SD_NULL_PID;
    scPageID        sTargetNPID = SD_NULL_PID;
    scPageID        sPrevPID;
    UChar         * sWAPagePtr;
    sdtWCB        * sTargetWCBPtr;
    sdtSortGroup  * sGrpInfo;

    IDE_ERROR( aWAGroupID != SDT_WAGROUPID_NONE );

    sGrpInfo = sdtSortSegment::getWAGroupInfo( aWASegment, aWAGroupID );

    if( aPrevWCBPtr != NULL )
    {
        /* 이전 page와 동일한 WCB를 할당받는 것을 방지한다. */
        sdtSortSegment::fixWAPage( aPrevWCBPtr );
    }

    switch( sGrpInfo->mPolicy )
    {
        case SDT_WA_REUSE_INMEMORY:
            IDE_TEST( sdtSortSegment::getFreeWAPageINMEMORY( aWASegment,
                                                             sGrpInfo,
                                                             &sTargetWCBPtr )
                      != IDE_SUCCESS );
            break;
        case SDT_WA_REUSE_FIFO:
            IDE_TEST( sdtSortSegment::getFreeWAPageFIFO( aWASegment,
                                                         sGrpInfo,
                                                         &sTargetWCBPtr )
                      != IDE_SUCCESS );
            break;
        case SDT_WA_REUSE_LRU:
            IDE_TEST( sdtSortSegment::getFreeWAPageLRU( aWASegment,
                                                        sGrpInfo,
                                                        &sTargetWCBPtr )
                      != IDE_SUCCESS );
            break;
        default:
            IDE_ERROR( 0 );
            break;
    }

    if( aPrevWCBPtr != NULL )
    {
        /* 이전 page와 동일한 WCB를 할당받으면 안된다. */
        IDE_DASSERT( sTargetWCBPtr != aPrevWCBPtr );

        sdtSortSegment::unfixWAPage( aPrevWCBPtr );
    }

    if ( sTargetWCBPtr == NULL )
    {
        /* Free 페이지 없음 */
        *aNewWCBPtr = NULL;
    }
    else
    {
        if ( sGrpInfo->mPolicy == SDT_WA_REUSE_INMEMORY )
        {
            if ( aPrevWCBPtr != NULL )
            {
                /* 이전페이지와의 링크를 연결함 */
                sWAPagePtr = sdtSortSegment::getWAPagePtr( aWASegment,
                                                           aWAGroupID,
                                                           aPrevWCBPtr );

                sPrevPID    = sdtSortSegment::getWPageID( aWASegment,
                                                          aPrevWCBPtr );

                sTargetWPID = sdtSortSegment::getWPageID( aWASegment,
                                                          sTargetWCBPtr );

                sdtTempPage::setNextPID( sWAPagePtr,
                                         sTargetWPID );
                (*aRowPageCount) ++;
            }
            else
            {
                sPrevPID = SD_NULL_PID;
            }
        }
        else
        {
            /* NPage를 할당해서 Binding시켜야 함 */
            IDE_TEST( sdtSortSegment::allocAndAssignNPage( aWASegment,
                                                           sTargetWCBPtr )
                      != IDE_SUCCESS );
            sTargetNPID = sdtSortSegment::getNPID( sTargetWCBPtr );

            if ( aPrevWCBPtr != NULL )
            {
                /* 이전페이지와 링크를 연결함 */
                sPrevPID   = sdtSortSegment::getNPID( aPrevWCBPtr );
                sWAPagePtr = sdtSortSegment::getWAPagePtr( aWASegment,
                                                           aWAGroupID,
                                                           aPrevWCBPtr );
                sdtTempPage::setNextPID( sWAPagePtr,
                                         sTargetNPID );

                IDE_ASSERT( aPrevWCBPtr->mWPState != SDT_WA_PAGESTATE_INIT );
                aPrevWCBPtr->mWPState = SDT_WA_PAGESTATE_DIRTY;

                (*aRowPageCount) ++;
            }
            else
            {
                sPrevPID = SD_NULL_PID;
            }
        }

        sWAPagePtr = sdtSortSegment::getWAPagePtr( aWASegment,
                                                   aWAGroupID,
                                                   sTargetWCBPtr );
        sdtTempPage::init( (sdtSortPageHdr*)sWAPagePtr,
                           aPageType,
                           sPrevPID,    /* Prev */
                           sTargetNPID, /* Self */
                           SD_NULL_PID );/* Next */

        (*aNewWCBPtr) = sTargetWCBPtr;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/**************************************************************************
 * Description :
 * page에 존재하는 Row 전체를 rowInfo형태로 읽는다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWAGroupID     - 대상 Group ID
 * aGRID          - Row의 위치 GRID
 * aValueLength   - Row의 Value부분의 총 길이
 * aRowBuffer     - 쪼개진 Column을 저장하기 위한 Buffer
 * <OUT>
 * aTRPInfo       - Fetch한 결과
 ***************************************************************************/
IDE_RC sdtTempRow::fetchByGRID( sdtSortSegHdr   * aWASegment,
                                sdtGroupID        aGroupID,
                                scGRID            aGRID,
                                UInt              aValueLength,
                                UChar           * aRowBuffer,
                                sdtSortScanInfo * aTRPInfo )
{
    IDE_TEST( sdtSortSegment::getPagePtrByGRID( aWASegment,
                                                aGroupID,
                                                aGRID,
                                                (UChar**)&aTRPInfo->mTRPHeader )
              != IDE_SUCCESS );
    IDE_DASSERT( SM_IS_FLAG_ON( aTRPInfo->mTRPHeader->mTRFlag, SDT_TRFLAG_HEAD ) );

#ifdef DEBUG
    if ( ( SC_MAKE_SPACE( aGRID ) != SDT_SPACEID_WAMAP ) &&
         ( SC_MAKE_SPACE( aGRID ) != SDT_SPACEID_WORKAREA ) )
    {
        sdtWCB * sWCBPtr = sdtSortSegment::findWCB( aWASegment, SC_MAKE_PID( aGRID ) );

        IDE_ASSERT( sdtSortSegment::isFixedPage( sWCBPtr ) == ID_TRUE );
    }
#endif
    aTRPInfo->mFetchEndOffset = aValueLength;

    IDE_TEST( fetch( aWASegment,
                     aGroupID,
                     aRowBuffer,
                     aTRPInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    smuUtility::dumpFuncWithBuffer( IDE_DUMP_0,
                                    smuUtility::dumpUInt,
                                    (void*)&aGroupID );
    smuUtility::dumpFuncWithBuffer( IDE_DUMP_0,
                                    smuUtility::dumpGRID,
                                    &aGRID );

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * Row가 Chaining, 즉 다른 Page에 걸쳐 기록될 경우 사용되며, 걸친 부분을
 * Fetch해온다.
 *
 * <IN>
 * aWASegment     - 대상 WASegment
 * aWAGroupID     - 대상 Group ID
 * aRowBuffer     - 쪼개진 Column을 저장하기 위한 Buffer
 * <OUT>
 * aTRPInfo       - Fetch한 결과
 ***************************************************************************/
IDE_RC sdtTempRow::fetchChainedRowPiece( sdtSortSegHdr        * aWASegment,
                                         sdtGroupID             aGroupID,
                                         UChar                * aRowBuffer,
                                         sdtSortScanInfo      * aTRPInfo )
{
    UChar        * sCursor;
    sdtSortTRPHdr* sTRPHeader;
    UChar          sFlag;
    UInt           sRowBufferOffset = 0;
    UInt           sTRPHeaderSize;

    sTRPHeader     = aTRPInfo->mTRPHeader;
    sFlag          = sTRPHeader->mTRFlag;
    sTRPHeaderSize = SDT_TR_HEADER_SIZE( sFlag );

    sCursor = ((UChar*)sTRPHeader) + sTRPHeaderSize;

    aTRPInfo->mValuePtr  = aRowBuffer;
    idlOS::memcpy( aRowBuffer + sRowBufferOffset,
                   sCursor,
                   sTRPHeader->mValueLength );

    sRowBufferOffset       += sTRPHeader->mValueLength;
    aTRPInfo->mValueLength  = sTRPHeader->mValueLength;

    /* Chainig Row일경우, 자신이 ReadPage하면서
     * HeadRow가 unfix될 수 있기에, Header를 복사해두고 이용함*/
#ifdef DEBUG
    sdtSortTRPHdr  sTRPHeaderBuffer;
    idlOS::memcpy( &sTRPHeaderBuffer,
                   aTRPInfo->mTRPHeader,
                   sTRPHeaderSize );
#endif

    while ( aTRPInfo->mFetchEndOffset > sRowBufferOffset )
    {
        IDE_ERROR ( SM_IS_FLAG_ON( sFlag, SDT_TRFLAG_NEXTGRID ) );
        IDE_ERROR ( SC_GRID_IS_NOT_NULL( sTRPHeader->mNextGRID ) );

        IDE_TEST( sdtSortSegment::getPagePtrByGRID( aWASegment,
                                                    aGroupID,
                                                    sTRPHeader->mNextGRID,
                                                    &sCursor )
                  != IDE_SUCCESS );
        sTRPHeader = (sdtSortTRPHdr*)sCursor;
        sFlag      = sTRPHeader->mTRFlag;
        sCursor   += SDT_TR_HEADER_SIZE( sFlag );

        idlOS::memcpy( aRowBuffer + sRowBufferOffset,
                       sCursor,
                       sTRPHeader->mValueLength);

        sRowBufferOffset       += sTRPHeader->mValueLength;
        aTRPInfo->mValueLength += sTRPHeader->mValueLength;
    }

#ifdef DEBUG
    IDE_ASSERT( idlOS::memcmp( &sTRPHeaderBuffer,
                               aTRPInfo->mTRPHeader,
                               sTRPHeaderSize ) == 0 );
#endif
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    smuUtility::dumpFuncWithBuffer( IDE_DUMP_0,
                                    smuUtility::dumpUInt,
                                    (void*)&aGroupID );
    smuUtility::dumpFuncWithBuffer( IDE_DUMP_0,
                                    dumpTempTRPHeader,
                                    (void*)aTRPInfo->mTRPHeader );

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
 * aValueList     - 갱신할 Value List
 ***************************************************************************/
IDE_RC sdtTempRow::updateChainedRowPiece( smiSortTempCursor* aTempCursor,
                                          smiValue         * aValueList )
{
    const smiColumnList  * sUpdateColumn;
    const smiColumn* sColumn;
    smiValue       * sValueList;
    UChar          * sRowPos      = NULL;
    sdtSortTRPHdr  * sTRPHeader   = NULL;
    scGRID           sGRID;
    sdtWCB         * sWCBPtr;
    UInt             sBeginOffset = 0;
    UInt             sEndOffset;
    UInt             sDstOffset;
    UInt             sSrcOffset;
    UInt             sSize;
    idBool           sIsWrite;
    idBool           sNeedDirty;

    if ( ( SC_GRID_IS_NULL( aTempCursor->mGRID ) ) ||
         ( SC_MAKE_SPACE( aTempCursor->mGRID ) == SDT_SPACEID_WORKAREA ) ||
         ( SC_MAKE_SPACE( aTempCursor->mGRID ) == SDT_SPACEID_WAMAP ) )
    {
        /* InMemory 연산이기에 SetDirty가 필요 없다 */
        IDE_DASSERT((((smiTempTableHeader*)aTempCursor->mTTHeader)->mTTState != SMI_TTSTATE_SORT_MERGE ) &&
                    (((smiTempTableHeader*)aTempCursor->mTTHeader)->mTTState != SMI_TTSTATE_SORT_MERGESCAN ));

        sNeedDirty = ID_FALSE;
    }
    else
    {
        IDE_DASSERT( SC_MAKE_SPACE( aTempCursor->mGRID ) ==
                     ((sdtSortSegHdr*)aTempCursor->mWASegment)->mSpaceID );

        sNeedDirty = ID_TRUE;
    }

    SC_COPY_GRID( aTempCursor->mGRID, sGRID );
    sRowPos = aTempCursor->mRowPtr;

    while( 1 )
    {
        sTRPHeader = (sdtSortTRPHdr*)sRowPos;
        sEndOffset = sBeginOffset + sTRPHeader->mValueLength;

        sRowPos   += SDT_TR_HEADER_SIZE( sTRPHeader->mTRFlag );

        sUpdateColumn = aTempCursor->mUpdateColumns;
        sValueList    = aValueList;
        sIsWrite      = ID_FALSE;

        while( sUpdateColumn != NULL )
        {
            sColumn = ( smiColumn*)sUpdateColumn->column;

            if (( sEndOffset > sColumn->offset ) &&                         // 시작점이 end 보다 작고
                ( sBeginOffset <= (sColumn->offset + sValueList->length ))) // 끝 점이 Begin보다 크다
            {
                sSize = sValueList->length;
                if ( sColumn->offset < sBeginOffset ) /* 왼쪽 시작 부분이 잘렸는가? */
                {
                    // 잘렸다.
                    sDstOffset = 0;
                    sSrcOffset = sBeginOffset - sColumn->offset;
                    sSize -= sSrcOffset;
                }
                else
                {
                    // 안잘렸다.
                    sDstOffset = sColumn->offset - sBeginOffset;
                    sSrcOffset = 0;
                }

                /* 오른쪽 끝 부분이 잘렸는가? */
                if ( (sColumn->offset + sValueList->length ) > sEndOffset )
                {
                    // 잘렸다.
                    sSize -= (sColumn->offset + sValueList->length - sEndOffset);
                }

                idlOS::memcpy( sRowPos + sDstOffset,
                               ((UChar*)sValueList->value) + sSrcOffset,
                               sSize );
                sIsWrite = ID_TRUE;
            }

            sValueList ++;
            sUpdateColumn = sUpdateColumn->next;
        }

        sBeginOffset = sEndOffset;

        if (( sIsWrite == ID_TRUE ) && ( sNeedDirty == ID_TRUE ))
        {
            /* 반드시 있어야 한다. */
            sWCBPtr = sdtSortSegment::findWCB( (sdtSortSegHdr*)aTempCursor->mWASegment,
                                               SC_MAKE_PID( sGRID ));
            IDE_ERROR( sWCBPtr != NULL );
            IDE_ASSERT( sWCBPtr->mWPState != SDT_WA_PAGESTATE_INIT );

            sWCBPtr->mWPState = SDT_WA_PAGESTATE_DIRTY;
        }

        if ( aTempCursor->mUpdateEndOffset <= sEndOffset )
        {
            // update 완료
            break;
        }

        IDE_ERROR( SM_IS_FLAG_ON( sTRPHeader->mTRFlag, SDT_TRFLAG_NEXTGRID ) );
        IDE_ERROR( SC_GRID_IS_NOT_NULL( sTRPHeader->mNextGRID ) );

        SC_COPY_GRID( sTRPHeader->mNextGRID, sGRID );
        IDE_TEST( sdtSortSegment::getPagePtrByGRID( (sdtSortSegHdr*)aTempCursor->mWASegment,
                                                    aTempCursor->mWAGroupID,
                                                    sGRID,
                                                    &sRowPos )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 *    공용 변수들에 대한 DumpFunction 
 ***************************************************************************/
void sdtTempRow::dumpTempTRPHeader( void       * aTRPHeader,
                                    SChar      * aOutBuf,
                                    UInt         aOutSize )
{
    sdtSortTRPHdr* sTRPHeader = (sdtSortTRPHdr*)aTRPHeader;

    (void)idlVA::appendFormat( aOutBuf,
                               aOutSize,
                               "DUMP TRPHeader:\n"
                               "mFlag        : %"ID_UINT32_FMT"\n"
                               "mHitSequence : %"ID_UINT32_FMT"\n"
                               "mValueLength : %"ID_UINT32_FMT"\n",
                               sTRPHeader->mTRFlag,
                               sTRPHeader->mHitSequence,
                               sTRPHeader->mValueLength );

    if ( SDT_TR_HEADER_SIZE( sTRPHeader->mTRFlag ) == SDT_TR_HEADER_SIZE_FULL )
    {
        (void)idlVA::appendFormat( aOutBuf,
                                   aOutSize,
                                   "mNextGRID    : <%"ID_UINT32_FMT
                                   ",%"ID_UINT32_FMT
                                   ",%"ID_UINT32_FMT">\n"
                                   "mChildGRID   : <%"ID_UINT32_FMT
                                   ",%"ID_UINT32_FMT
                                   ",%"ID_UINT32_FMT">\n",
                                   sTRPHeader->mNextGRID.mSpaceID,
                                   sTRPHeader->mNextGRID.mPageID,
                                   sTRPHeader->mNextGRID.mOffset,
                                   sTRPHeader->mChildGRID.mSpaceID,
                                   sTRPHeader->mChildGRID.mPageID,
                                   sTRPHeader->mChildGRID.mOffset );
    }

    return;
}


void sdtTempRow::dumpTempTRPInfo4Insert( void       * aTRPInfo,
                                         SChar      * aOutBuf,
                                         UInt         aOutSize )
{
    sdtSortInsertInfo * sTRPInfo = (sdtSortInsertInfo*)aTRPInfo;
    UInt         sSize;
    UInt         i;
    UInt         sDumpSize;

    dumpTempTRPHeader( &sTRPInfo->mTRPHeader, aOutBuf, aOutSize );

    for ( i = 0 ; i < sTRPInfo->mColumnCount ; i++ )
    {
        (void)idlVA::appendFormat( aOutBuf,
                                   aOutSize,
                                   "[%2"ID_UINT32_FMT"] Length:%4"ID_UINT32_FMT"\n",
                                   i,
                                   sTRPInfo->mValueList[ i ].length );

        (void)idlVA::appendFormat( aOutBuf,
                                   aOutSize,
                                   "Value : ");
        sSize     = idlOS::strlen( aOutBuf );
        sDumpSize = sTRPInfo->mValueList[ i ].length;
        if ( sDumpSize > 64 )
        {
            /* IDE_MESSAGE_SIZE 를 넘으면 출력안될수도 있으므로 
               임의의 값으로 자름  */
            sDumpSize = 64;
        }
        IDE_TEST( ideLog::ideMemToHexStr( (UChar*)sTRPInfo->mValueList[ i ].value,
                                          sDumpSize ,
                                          IDE_DUMP_FORMAT_VALUEONLY,
                                          aOutBuf + sSize,
                                          aOutSize - sSize )
                  != IDE_SUCCESS );
        (void)idlVA::appendFormat( aOutBuf, aOutSize, "\n\n" );
    }

    return;

    IDE_EXCEPTION_END;

    return;
}

void sdtTempRow::dumpTempPageByRow( void  * aPagePtr,
                                    SChar * aOutBuf,
                                    UInt    aOutSize )
{
    UChar     * sPagePtr = (UChar*)aPagePtr;
    UChar     * sRowPtr;
    UChar     * sValuePtr;
    UInt        sSlotValue;
    UInt        sSlotCount;
    UInt        i;
    UInt        sSize;
    UInt        sDumpSize;
    sdtSortTRPHdr* sTRPHeader;

    sSlotCount = sdtTempPage::getSlotCount( sPagePtr );

    for ( i = 0 ; i < sSlotCount ; i ++ )
    {
        sSlotValue = sdtTempPage::getSlotOffset( sPagePtr, i );

        sRowPtr = sPagePtr + sSlotValue;

        sTRPHeader = (sdtSortTRPHdr*)(sRowPtr);
        sValuePtr  = sRowPtr + SDT_TR_HEADER_SIZE( sTRPHeader->mTRFlag );

        (void)idlVA::appendFormat( aOutBuf,
                                   aOutSize,
                                   "[%4"ID_UINT32_FMT"] ",i);
        dumpTempTRPHeader( sTRPHeader, aOutBuf, aOutSize );

        (void)idlVA::appendFormat( aOutBuf,
                                   aOutSize,
                                   "Value : ");
        sSize     = idlOS::strlen( aOutBuf );
        sDumpSize = sTRPHeader->mValueLength;
        if ( sDumpSize > 64 )
        {
            /* IDE_MESSAGE_SIZE 를 넘으면 출력안될수도 있으므로 
               임의의 값으로 자름  */
            sDumpSize = 64;
        }
        IDE_TEST( ideLog::ideMemToHexStr( (UChar*)sValuePtr,
                                          sDumpSize,
                                          IDE_DUMP_FORMAT_VALUEONLY,
                                          aOutBuf + sSize,
                                          aOutSize - sSize )
                  != IDE_SUCCESS );
        (void)idlVA::appendFormat( aOutBuf, aOutSize, "\n\n" );
    }

    return;

    IDE_EXCEPTION_END;

    return;
}

/**************************************************************************
 * Description :
 * Row의 특정 Column을 갱신하다 실패 했을때 Column 정보들을 출력한다.
 ***************************************************************************/
void sdtTempRow::dumpRowWithCursor( void   * aTempCursor,
                                    SChar  * aOutBuf,
                                    UInt     aOutSize )
{

    smiSortTempCursor   * sCursor       = (smiSortTempCursor*)aTempCursor;
    smiTempTableHeader  * sHeader       = sCursor->mTTHeader;
    sdtSortSegHdr       * sWASeg        = (sdtSortSegHdr*)sHeader->mWASegment;
    const smiColumnList * sUpdateColumn;
    UChar               * sRowPos       = NULL;
    sdtSortTRPHdr       * sTRPHeader    = NULL;
    scGRID                sGRID         = sCursor->mGRID;
    UInt                  sBeginOffset  = 0;
    UInt                  sEndOffset    = 0;
    UInt                  sSize         = 0;
    UInt                  sDumpSize     = 0;

    IDE_ERROR( sCursor != NULL );

    (void)idlVA::appendFormat( aOutBuf,
                               aOutSize,
                               "DUMP TEMPROW\n");
    while( 1 )
    {
        if ( sRowPos == NULL )
        {
            sRowPos = sCursor->mRowPtr;
        }
        else
        {
            IDE_TEST( sdtSortSegment::getPagePtrByGRID( sWASeg,
                                                        sCursor->mWAGroupID,
                                                        sGRID,
                                                        &sRowPos )
                      != IDE_SUCCESS );
        }

        (void)idlVA::appendFormat( aOutBuf,
                                   aOutSize,
                                   "GRID  : %4"ID_UINT32_FMT","
                                   "%4"ID_UINT32_FMT","
                                   "%4"ID_UINT32_FMT"\n",
                                   sGRID.mSpaceID,
                                   sGRID.mPageID,
                                   sGRID.mOffset );

        sTRPHeader = (sdtSortTRPHdr*)sRowPos;
        sEndOffset = sBeginOffset + sTRPHeader->mValueLength;

        sRowPos   += SDT_TR_HEADER_SIZE( sTRPHeader->mTRFlag );

        sUpdateColumn = sCursor->mUpdateColumns;

        if ( sUpdateColumn != NULL )
        {
            (void)idlVA::appendFormat( aOutBuf,
                                       aOutSize,
                                       "length: %"ID_UINT32_FMT"\n",
                                       sTRPHeader->mValueLength );

            (void)idlVA::appendFormat( aOutBuf,
                                       aOutSize,
                                       "value : ");
            sSize = idlOS::strlen( aOutBuf );
            sDumpSize = sTRPHeader->mValueLength;
            if ( sDumpSize > 64 )
            {
                /* IDE_MESSAGE_SIZE 를 넘으면 출력안될수도 있으므로 
                   임의의 값으로 자름  */
                sDumpSize = 64;
            }
            IDE_TEST( ideLog::ideMemToHexStr( sRowPos,
                                              sDumpSize,
                                              IDE_DUMP_FORMAT_VALUEONLY,
                                              aOutBuf + sSize,
                                              aOutSize - sSize )
                      != IDE_SUCCESS );
            (void)idlVA::appendFormat( aOutBuf, aOutSize, "\n\n" );
        }
        sBeginOffset = sEndOffset;

        if ( SM_IS_FLAG_ON( sTRPHeader->mTRFlag, SDT_TRFLAG_NEXTGRID ) )
        {
            SC_COPY_GRID( sTRPHeader->mNextGRID, sGRID ) ;
        }
        else
        {
            break;
        }
    }

    return;

    IDE_EXCEPTION_END;

    (void)idlVA::appendFormat(aOutBuf,
                              aOutSize,
                              "DUMP TEMPCURSOR ERROR\n" );
    return;
}
