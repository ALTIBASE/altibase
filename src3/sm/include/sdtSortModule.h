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

#ifndef _O_SDT_SORT_MODULE_H_
#define _O_SDT_SORT_MODULE_H_ 1

#include <idu.h>
#include <smDef.h>
#include <smiDef.h>
#include <sdtSortDef.h>
#include <sdnbDef.h>
#include <sdtDef.h>
#include <sdtTempRow.h>
#include <sdtWASortMap.h>

class sdtSortModule
{
public:
    static IDE_RC init( smiTempTableHeader * aHeader );
    static IDE_RC destroy( smiTempTableHeader * aHeader);

    /************************* Module Function ***************************/
    inline static IDE_RC insert(smiTempTableHeader  * aHeader,
                                smiValue            * aValue );
    static IDE_RC sort(smiTempTableHeader * aHeader);

private:
    static IDE_RC calcEstimatedStats( smiTempTableHeader * aHeader );

    /***************************************************************
     * Open cursor operations
     ***************************************************************/
    static IDE_RC openCursorInMemoryScan( smiTempTableHeader * aHeader,
                                          smiSortTempCursor  * aCursor );
    static IDE_RC openCursorMergeScan( smiTempTableHeader * aHeader,
                                       smiSortTempCursor  * aCursor );
    static IDE_RC openCursorIndexScan( smiTempTableHeader * aHeader,
                                       smiSortTempCursor  * aCursor );
    static IDE_RC openCursorScan( smiTempTableHeader * aHeader,
                                  smiSortTempCursor  * aCursor );
    static IDE_RC traverseInMemoryScan( smiTempTableHeader * aHeader,
                                        const smiCallBack  * aCallBack,
                                        idBool               aDirection,
                                        SInt               * aSeq );
    static IDE_RC traverseIndexScan( smiTempTableHeader * aHeader,
                                     smiSortTempCursor  * aCursor );
    /***************************************************************
     * fetch operations
     ***************************************************************/
    static IDE_RC fetchInMemoryScanForward( smiSortTempCursor* aCursor,
                                            UChar  ** aRow,
                                            scGRID  * aRowGRID );
    static IDE_RC fetchInMemoryScanBackward(smiSortTempCursor* aCursor,
                                            UChar  ** aRow,
                                            scGRID  * aRowGRID );
    static IDE_RC fetchMergeScan(smiSortTempCursor* aCursor,
                                 UChar  ** aRow,
                                 scGRID  * aRowGRID );
    static IDE_RC fetchIndexScanForward(smiSortTempCursor* aCursor,
                                        UChar  ** aRow,
                                        scGRID  * aRowGRID );
    static IDE_RC fetchIndexScanBackward(smiSortTempCursor* aCursor,
                                         UChar  ** aRow,
                                         scGRID  * aRowGRID );
    static IDE_RC fetchScan(smiSortTempCursor* aCursor,
                            UChar  ** aRow,
                            scGRID  * aRowGRID );
    /***************************************************************
     * store/restore cursor operations
     ***************************************************************/
    static IDE_RC storeCursorInMemoryScan(smiSortTempCursor* aCursor,
                                          smiTempPosition * aPosition ); 
    static IDE_RC restoreCursorInMemoryScan(smiSortTempCursor* aCursor,
                                            smiTempPosition * aPosition );
    static IDE_RC storeCursorMergeScan(smiSortTempCursor* aCursor,
                                       smiTempPosition * aPosition );
    static IDE_RC restoreCursorMergeScan(smiSortTempCursor* aCursor,
                                         smiTempPosition * aPosition );
    static IDE_RC storeCursorIndexScan(smiSortTempCursor* aCursor,
                                       smiTempPosition * aPosition );
    static IDE_RC restoreCursorIndexScan(smiSortTempCursor* aCursor,
                                         smiTempPosition * aPosition );
    static IDE_RC storeCursorScan(smiSortTempCursor* aCursor,
                                  smiTempPosition * aPosition );
    static IDE_RC restoreCursorScan(smiSortTempCursor* aCursor,
                                    smiTempPosition * aPosition );

    /***************************************************************
     * State transition operation
     ***************************************************************/
    static IDE_RC extractNSort(smiTempTableHeader * aHeader);
    static IDE_RC merge(smiTempTableHeader * aHeader);
    static IDE_RC makeIndex(smiTempTableHeader * aHeader);
    static IDE_RC makeLNodes(smiTempTableHeader * aHeade,
                             idBool             * aNeedINode );
    static IDE_RC makeINodes(smiTempTableHeader * aHeader,
                             scPageID           * aChildNPID,
                             idBool             * aNeedMoreINode );
    static IDE_RC inMemoryScan(smiTempTableHeader * aHeader);
    static IDE_RC mergeScan(smiTempTableHeader * aHeader);
    static IDE_RC indexScan(smiTempTableHeader * aHeader);
    static IDE_RC scan(smiTempTableHeader * aHeader);

    /***************************************************************
     * SortOperations
     ***************************************************************/
    static IDE_RC sortSortGroup(smiTempTableHeader * aHeader);
    static IDE_RC quickSort( smiTempTableHeader * aHeader,
                             UInt                 aLeftPos,
                             UInt                 aRightPos );
    static IDE_RC mergeSort( smiTempTableHeader * aHeader,
                             SInt                 aLeftBeginPos,
                             SInt                 aLeftEndPos,
                             SInt                 aRightBeginPos,
                             SInt                 aRightEndPos );
    static IDE_RC compareGRIDAndGRID(smiTempTableHeader * aHeader,
                                     sdtGroupID           aGrpID,
                                     scGRID               aSrcGRID,
                                     scGRID               aDstGRID,
                                     SInt               * aResult );
    static IDE_RC compare( smiTempTableHeader * aHeader,
                           sdtSortScanInfo    * aSrcTRPInfo,
                           sdtSortScanInfo    * aDstTRPInfo,
                           SInt               * aResult );


    /***************************************************************
     * Copy or Store row operations
     ***************************************************************/
    static IDE_RC storeSortedRun(smiTempTableHeader * aHeader);
    static IDE_RC copyRowByPtr( smiTempTableHeader * aHeader,
                                UChar              * aSrcPtr,
                                sdtCopyPurpose      aPurpose,
                                sdtTempPageType      aPageType,
                                scGRID               aChildGRID,
                                sdtSortInsertResult* aTRPInfo );
    static IDE_RC copyRowByGRID( smiTempTableHeader * aHeader,
                                 scGRID               aSrcGRID,
                                 sdtCopyPurpose      aPurpose,
                                 sdtTempPageType      aPageType,
                                 scGRID               aChildGRID,
                                 sdtSortInsertResult* aTRPInfo );
    static IDE_RC copyExtraRow( smiTempTableHeader    * aHeader,
                                sdtSortInsertInfo * aTRPInfo );

    /***************************************************************
     * Heap Operation for Merge
     ***************************************************************/
    inline static UInt calcMaxMergeRunCount(
        smiTempTableHeader * aHeader,
        UInt                 aRowPageCount );
    static IDE_RC heapInit(smiTempTableHeader * aHeader);
    static IDE_RC buildLooserTree(smiTempTableHeader * aHeader);
    static IDE_RC heapPop( smiTempTableHeader * aHeader );
    static IDE_RC findAndSetLoserSlot( smiTempTableHeader * aHeader,
                                       UInt                 aPos,
                                       UInt               * aChild );
    static IDE_RC readNextRowByRun( smiTempTableHeader   * aHeader,
                                    sdtTempMergeRunInfo * aRun );
    static IDE_RC readRunPage( smiTempTableHeader   * aHeader,
                               UInt                   aRunNo,
                               UInt                   aPageSeq,
                               scPageID             * aNextPID,
                               idBool                 aReadNextNPID );
    inline static void getGRIDFromRunInfo( smiTempTableHeader   * aHeader,
                                           sdtTempMergeRunInfo * aRunInfo,
                                           scGRID               * aGRID );
    inline static scPageID getWPIDFromRunInfo( smiTempTableHeader * aHeader,
                                               UInt                 aRunNo,
                                               UInt                 aPageSeq );
    static IDE_RC makeMergePosition( smiTempTableHeader  * aHeader,
                                     void               ** aMergePosition );
    static IDE_RC makeMergeRuns( smiTempTableHeader  * aHeader,
                                 void                * aMergePosition );

    static IDE_RC makeScanPosition( smiTempTableHeader  * aHeader,
                                    scPageID           ** aMergePosition );
};

/**************************************************************************
 * Description :
 * 현 상황을 분석하여 MergeRun 최대 개수를 구한다.
 ***************************************************************************/
UInt sdtSortModule::calcMaxMergeRunCount(
    smiTempTableHeader * aHeader,
    UInt                 aRowPageCount )
{
    ULong                  sSortGroupSize;
    sdtSortSegHdr        * sWASeg = (sdtSortSegHdr*)aHeader->mWASegment;
    UInt                   sMergeRunCount;

    /* 마지막에 Slot 둘 빼는 이유는,
     * 하나를 빼서 ZeroBase를 OneBase로 변경하고
     * 하나를 더 빼서 (예약해두어서) Run이 하나도 없어도 정상동작하기 위함 */
    sSortGroupSize =
        sdtSortSegment::getAllocableWAGroupPageCount( sWASeg, SDT_WAGROUPID_SORT )
        * SD_PAGE_SIZE
        - ID_SIZEOF( sdtTempMergeRunInfo ) * 2;

    /* 한번에 Merge할 수 있는 Run의 개수를 계산한다.
     *
     * 공식은 다음같다.
     * MaxMergeRunCount = SortGroupSize / ( SlotSize * 3 + RunSize );
     * (여기서 Run은 실제로 Run의 내용을 갖고 있는 Page, 8192이다.)
     * 이는 Run 하나당 SlotSize*3 + RunSize 만큼이 필요하다는 것이다.
     * RunSize는 당연히 그렇다지만 Slot이 * 3만큼 추정되는 이유는 다음과 같다.
     *
     * Slot은 2의 제곱 만큼 커져가며 확장한다.
     * 1개의 Run은 1개의 Slot을,
     * 2개의 Run은 3개의 Slot을,
     * 4개의 Run은 7개의 Slot을 필요로한다.
     * 1,2,4,8, 이런식으로 커져가기 때문에, 이상적으로는 SlotCount*2-1 이다.
     *
     * 가령 64개일 경우, 127개가 필요하다. 1+2+4+8+16+32+64 = 127이기 때문이다.
     * 하지만 최악의 경우, 즉 Slot이 65개일 경우,
     * 1+2+4+8+16+32+64+65 = 192개가 필요하다.
     * 따라서 3을 곱하면 최악의 경우를 고려할 수 있기에, *3 한다. */

    /* 추가로 MaxRowPageCount 즉 하나의 Row가 사용하는 최대 Page개수에 곱하기2
     * 를 하여 사용하는데, 이는 Row를 두개 올리기 위함이다.
     * 그래야 하나의 Row를 Fetch대상으로 정하고 다음 Row를 미리
     * HeapPop해놔도, Fetch대상인 Row가 내려가지 않는다. */
    sMergeRunCount = sSortGroupSize /
        ( ID_SIZEOF( sdtTempMergeRunInfo ) * 3 +
          aRowPageCount * 2 * SD_PAGE_SIZE );

    return sMergeRunCount;
}

/**************************************************************************
 * Description :
 *      RunInfo를 바탕으로, 그 RunInfo가 가리키는 GRID를 얻어낸다.
 * <IN>
 * aHeader        - 대상 Table
 * aRunInfo       - 원본이 되는 RunInfo
 * <OUT>
 * aGRID          - 해당 Run이 가리키는 위치
 ***************************************************************************/
void sdtSortModule::getGRIDFromRunInfo( smiTempTableHeader   * aHeader,
                                        sdtTempMergeRunInfo * aRunInfo,
                                        scGRID               * aGRID )
{
    SC_MAKE_GRID( *aGRID,
                  SDT_SPACEID_WORKAREA,
                  getWPIDFromRunInfo( aHeader,
                                      aRunInfo->mRunNo,
                                      aRunInfo->mPIDSeq ),
                  aRunInfo->mSlotNo );
}

/**************************************************************************
 * Description :
 *      RunInfo를 바탕으로, 그 대상의 WPID를 얻어냄
 *
 *
 * +-------------+---------------+---------------+---------------+
 * | WAMap(Heap) |     Run 2     |     Run 1     |     Run 0     |
 * |             +---+---+---+---+---+---+---+---+---+---+---+---+
 * |             | 3 | 2 | 1 | 0 | 3 | 2 | 1 | 0 | 3 | 2 | 1 | 0 |
 * |             |   |   |   |   |   |   |   |   |   |   |   |   |
 * +-------------+---+---+---+---+---+---+---+---+---+---+---+---+
 *                <------------->                                ^
 *                 MergeRunSize                             sLastWPID
 *
 *
 * <IN>
 * aHeader        - 대상 Table
 * aRunInfo       - 원본이 되는 RunInfo
 * <OUT>
 * aGRID          - 해당 Run이 가리키는 위치
 ***************************************************************************/
scPageID sdtSortModule::getWPIDFromRunInfo( smiTempTableHeader * aHeader,
                                            UInt                 aRunNo,
                                            UInt                 aPageSeq )
{
    scPageID sRetPID;
    scPageID sLastWPID;

    IDE_ASSERT( aHeader->mMergeRunSize > 0 );

    if ( aRunNo == SDT_TEMP_RUNINFO_NULL )
    {
        return SC_NULL_PID;
    }

    sLastWPID = sdtSortSegment::getLastWPageInWAGroup(
        (sdtSortSegHdr*)aHeader->mWASegment,
        SDT_WAGROUPID_SORT ) - 1;

    sRetPID = sLastWPID -
        ( ( aHeader->mMergeRunSize * aRunNo )
          + ( aPageSeq % aHeader->mMergeRunSize ) );

    IDE_ASSERT( sRetPID <= sLastWPID );
    IDE_ASSERT( sRetPID >=  sdtSortSegment::getFirstWPageInWAGroup(
                    (sdtSortSegHdr*)aHeader->mWASegment,
                    SDT_WAGROUPID_SORT ) );

    return sRetPID;
}


/**************************************************************************
 * Description :
 * 데이터를 삽입한다. InsertNSort, Insert 상태여야
 * 한다.
 *
 * <IN>
 * aTable     - 대상 Table
 * aValue     - 삽입할 Value
 * aHashValue - 삽입할 HashValue (HashTemp만 유효 )
 * <OUT>
 * aGRID      - 삽입한 위치
 * aResult    - 삽입이 성공하였는가?(UniqueViolation Check용 )
 ***************************************************************************/
IDE_RC sdtSortModule::insert( smiTempTableHeader  * aHeader,
                              smiValue            * aValue )
{
    sdtSortSegHdr     * sWASeg = (sdtSortSegHdr*)aHeader->mWASegment;
    sdtSortInsertResult sTRInsertResult;
    sdtSortInsertInfo   sScanInfo;
    UInt                sWAMapIdx;
    idBool              sResetSortGroup = ID_FALSE;

    IDE_ERROR( (aHeader->mTTState == SMI_TTSTATE_SORT_INSERTNSORT) ||
               (aHeader->mTTState == SMI_TTSTATE_SORT_INSERTONLY) );

    if( aHeader->mCheckCnt > SMI_TT_STATS_INTERVAL )
    {
        IDE_TEST( iduCheckSessionEvent( aHeader->mStatistics )
                  != IDE_SUCCESS );
        aHeader->mCheckCnt = 0;
    }

    while( 1 )
    {
        sScanInfo.mTRPHeader.mTRFlag      = SDT_TRFLAG_HEAD;
        sScanInfo.mTRPHeader.mHitSequence = 0;
        sScanInfo.mTRPHeader.mValueLength = 0; /* append하면서 설정됨 */
        SC_MAKE_NULL_GRID( sScanInfo.mTRPHeader.mNextGRID );
        SC_MAKE_NULL_GRID( sScanInfo.mTRPHeader.mChildGRID );
        sScanInfo.mColumnCount = aHeader->mColumnCount;
        sScanInfo.mColumns     = aHeader->mColumns;
        sScanInfo.mValueLength = aHeader->mRowSize;
        sScanInfo.mValueList   = aValue;

        IDE_TEST( sdtTempRow::append( sWASeg,
                                      aHeader->mSortGroupID,
                                      SDT_TEMP_PAGETYPE_INMEMORYGROUP,
                                      0, /* CuttingOffset */
                                      &sScanInfo,
                                      &sTRInsertResult )
                  != IDE_SUCCESS );

        /* WAMap에 슬롯 삽입 */
        if ( sTRInsertResult.mComplete == ID_TRUE )
        {
            /* 첫 RowPiece까지 온전히 삽입하였음 */
            IDE_TEST( sdtWASortMap::expand(
                          &sWASeg->mSortMapHdr,
                          SC_MAKE_PID( sTRInsertResult.mHeadRowpieceGRID ),
                          &sWAMapIdx )
                      != IDE_SUCCESS );

            if ( sWAMapIdx != SDT_WASLOT_UNUSED  )
            {
                /* Slot확장 성공 */
                IDE_ERROR( SC_GRID_IS_NOT_NULL(
                               sTRInsertResult.mHeadRowpieceGRID ) );
                IDE_TEST( sdtWASortMap::setvULong(
                              &sWASeg->mSortMapHdr,
                              sWAMapIdx,
                              (vULong*)&sTRInsertResult.mHeadRowpiecePtr )
                          != IDE_SUCCESS );
                break;
            }
        }

        /* Reset 한번 했는데도 삽입 실패하는건 있을 수 없음 */
        IDE_ERROR( sResetSortGroup == ID_FALSE );

        /* BUG-46438 한번에 한 건도 넣지 못할 정도로 공간이 작다면,
         * sort temp를 사용 할 수 없다.*/
        IDE_TEST_RAISE( sdtWASortMap::getSlotCount( &sWASeg->mSortMapHdr ) == 0 ,
                        error_invalid_sortareasize );

        /* 공간부족으로 Row나 KeySlot을 삽입에 실패하면
         * 해당 런을 정렬해서 내림 */
        if ( aHeader->mTTState == SMI_TTSTATE_SORT_INSERTNSORT )
        {
            IDE_TEST( sortSortGroup( aHeader ) != IDE_SUCCESS );
        }
        IDE_TEST( storeSortedRun(aHeader) != IDE_SUCCESS );
        sResetSortGroup = ID_TRUE;
    }

    aHeader->mRowCount++;

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_invalid_sortareasize);
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INVALID_SORTAREASIZE ) );
    }
    IDE_EXCEPTION_END;

    smiTempTable::checkAndDump( aHeader );

    return IDE_FAILURE;
}

#endif /* _O_SDT_SORT_MODULE_H_ */
