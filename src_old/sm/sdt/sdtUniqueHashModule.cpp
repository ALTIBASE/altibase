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

#include <sdtUniqueHashModule.h>
#include <sdtWorkArea.h>
#include <sdtWAMap.h>
#include <sdtDef.h>
#include <sdtTempRow.h>
#include <sdtTempPage.h>

/* Filtering을 이용해 Row를 찾기 위함 */
typedef struct sdtTempCompData
{
    smiTempTableHeader *mHeader;
    const smiValue     *mValueList;
} sdtTempCompData;

/**************************************************************************
 * Description :
 * WorkArea를 할당받고 temptableHeader등을 초기화 해준다.
 * HashGroup, SubGroup을 만들어준다.
 *
 * <IN>
 * aHeader        - 대상 Table
 ***************************************************************************/
IDE_RC sdtUniqueHashModule::init( void * aHeader )
{
    smiTempTableHeader * sHeader = (smiTempTableHeader*)aHeader;
    sdtWASegment       * sWASegment;
    UInt                 sInitGroupPageCount;
    UInt                 sHashGroupPageCount;
    UInt                 sSubGroupPageCount;

    sWASegment = (sdtWASegment*)sHeader->mWASegment;

    sInitGroupPageCount = sdtWASegment::getAllocableWAGroupPageCount(
        sWASegment,
        SDT_WAGROUPID_INIT );
    sHashGroupPageCount = sInitGroupPageCount
        * sHeader->mWorkGroupRatio / 100;
    sSubGroupPageCount  = sInitGroupPageCount - sHashGroupPageCount;

    IDE_TEST( sdtWASegment::createWAGroup( sWASegment,
                                           SDT_WAGROUPID_HASH,
                                           sHashGroupPageCount,
                                           SDT_WA_REUSE_INMEMORY )
              != IDE_SUCCESS );
    IDE_TEST( sdtWASegment::createWAGroup( sWASegment,
                                           SDT_WAGROUPID_SUB,
                                           sSubGroupPageCount,
                                           SDT_WA_REUSE_LRU )
              != IDE_SUCCESS );

    IDE_TEST( sdtWAMap::create( sWASegment,
                                SDT_WAGROUPID_HASH,
                                SDT_WM_TYPE_GRID,
                                sHashGroupPageCount * SD_PAGE_SIZE
                                / ID_SIZEOF( scGRID ) - 1,
                                1, /* aVersionCount */
                                (void*)&sWASegment->mSortHashMapHdr )
              != IDE_SUCCESS );
    sHeader->mModule.mInsert        = insert;
    sHeader->mModule.mSort          = NULL;
    sHeader->mModule.mOpenCursor    = openCursor;
    sHeader->mModule.mCloseCursor   = closeCursor;
    sHeader->mStatsPtr->mExtraStat1 =
        sdtWAMap::getSlotCount(&sWASegment->mSortHashMapHdr );

    sHeader->mFetchGroupID   = SDT_WAGROUPID_SUB;
    sHeader->mTTState        = SMI_TTSTATE_UNIQUEHASH;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * 정리한다. WorkArea및 Cursor등은 smiTemp에서 알아서 한다.
 *
 * <IN>
 * aHeader        - 대상 Table
 ***************************************************************************/
IDE_RC sdtUniqueHashModule::destroy( void * aHeader )
{
    /* BUG-39696 */
    IDE_ASSERT( aHeader != NULL );

    smiTempTableHeader * sHeader = (smiTempTableHeader*)aHeader;

    /* 종료되면서 예측 통계치를 계산한다. */
    /* Optimal(InMemory)은 모든 데이터가 HashArea에 담길 크기면 된다. */
    sHeader->mStatsPtr->mEstimatedOptimalHashSize =
        (ULong)(( SDT_TR_HEADER_SIZE_FULL + sHeader->mRowSize )
                * sHeader->mRowCount
                * 100 / ( 100 -  sHeader->mWorkGroupRatio ) * 1.2);

    return IDE_SUCCESS;
}

/**************************************************************************
 * Description :
 * Uniquness를 Check하며 데이터를 삽입한다.
 *
 * <IN>
 * aTable           - 대상 Table
 * aValue           - 삽입할 Value
 * aHashValue       - 삽입할 HashValue (HashTemp만 유효 )
 * <OUT>
 * aGRID            - 삽입한 위치
 * aResult          - 삽입이 성공하였는가?(UniqueViolation Check용 )
 ***************************************************************************/
IDE_RC sdtUniqueHashModule::insert( void     * aHeader,
                                    smiValue * aValue,
                                    UInt       aHashValue,
                                    scGRID   * aGRID,
                                    idBool   * aResult )
{
    smiTempTableHeader * sHeader = (smiTempTableHeader *)aHeader;
    sdtWASegment       * sWASeg = (sdtWASegment*)sHeader->mWASegment;
    sdtTempCompData     sData;
    smiCallBack          sFilter;
    sdtTRPInfo4Insert    sTRPInfo4Insert;
    sdtTRInsertResult    sTRInsertResult;
    UChar              * sRowPtr;
    UInt                 sIdx;
    scGRID             * sBucketGRIDPtr;
    scGRID               sTargetGRID;

    /*************************** 중복체크 *******************************/
    /* sdtTempRow::insertByHVIndex의 1373 line 참조 */
    if ( SM_IS_FLAG_ON( sHeader->mTTFlag,  SMI_TTFLAG_UNIQUE ) )
    {
        sData.mHeader     = sHeader;
        sData.mValueList  = aValue;
        *aResult = ID_FALSE;

        sFilter.mHashVal = aHashValue;
        sFilter.callback = compareRowAndValue;
        sFilter.data     = &sData;
        IDE_TEST( findExactRow( sHeader,
                                sWASeg,
                                ID_TRUE, /* reset page */
                                &sFilter,
                                aHashValue,
                                &sIdx,
                                &sRowPtr,
                                &sBucketGRIDPtr,
                                &sTargetGRID)
                  != IDE_SUCCESS );

        /* UniqueViolation Check */
        IDE_TEST_CONT( !SC_GRID_IS_NULL( sTargetGRID ),
                       ERR_UNIQUENESS_VIOLATION );
    }
    else
    {
        /* Uniqueness Check할 필요 없으니, Bucket만 조회함 */
        sIdx = aHashValue % sdtWAMap::getSlotCount( &sWASeg->mSortHashMapHdr );

        IDE_TEST( sdtWAMap::getSlotPtrWithCheckState( &sWASeg->mSortHashMapHdr,
                                                      sIdx,
                                                      ID_TRUE, /* reset page */
                                                      (void**)&sBucketGRIDPtr )
                  != IDE_SUCCESS );
    }

    /**************************** 삽입 *********************************/
    *aResult = ID_TRUE;
    sdtTempRow::makeTRPInfo( SDT_TRFLAG_HEAD |
                             SDT_TRFLAG_CHILDGRID,
                             0,                /*HitSequence */
                             aHashValue,
                             *sBucketGRIDPtr,  /* aChildGRID*/
                             SC_NULL_GRID,     /* aNextGRID */
                             sHeader->mRowSize,
                             sHeader->mColumnCount,
                             sHeader->mColumns,
                             aValue,
                             &sTRPInfo4Insert );

    IDE_TEST( sdtTempRow::append( sWASeg,
                                  SDT_WAGROUPID_SUB,
                                  SDT_TEMP_PAGETYPE_UNIQUEROWS,
                                  0, /*CuttingOfffset*/
                                  &sTRPInfo4Insert,
                                  &sTRInsertResult )
              != IDE_SUCCESS );
    IDE_ERROR( sTRInsertResult.mComplete == ID_TRUE );

    /* Bucket의 GRID값을 변경한다.
     * IDE_TEST( sdtWAMap::set( &sWASeg->mSortHashMapHdr,
     *                          sIdx,
     *                          &sTRInsertResult.mHeadRowpieceGRID )
     *          != IDE_SUCCESS );
     * 와 동일함 */
    *sBucketGRIDPtr = sTRInsertResult.mHeadRowpieceGRID;

    *aGRID = sTRInsertResult.mHeadRowpieceGRID;

    IDE_EXCEPTION_CONT( ERR_UNIQUENESS_VIOLATION );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***************************************************************************
 * Description :
 * 커서를 엽니다.
 *
 * <IN>
 * aHeader        - 대상 Table
 * <OUT>
 * aCursor        - 반환값
 ***************************************************************************/
IDE_RC sdtUniqueHashModule::openCursor( void * aHeader,
                                        void * aCursor )
{
    smiTempTableHeader * sHeader;
    smiTempCursor      * sCursor = (smiTempCursor *)aCursor;

    sCursor->mWAGroupID = SDT_WAGROUPID_SUB;
    IDE_DASSERT( SM_IS_FLAG_ON( sCursor->mTCFlag, SMI_TCFLAG_FORWARD ) );
    IDE_DASSERT( SM_IS_FLAG_OFF( sCursor->mTCFlag, SMI_TCFLAG_ORDEREDSCAN ) );
    
#ifdef DEBUG
    /* Cache 정보들 : UniqueHash 는 사용하지 않는다. */
    sCursor->mWPID      = SC_NULL_PID;
    sCursor->mWAPagePtr = NULL;
    sCursor->mSlotCount = SDT_WASLOT_UNUSED;
#endif

    /* Row의 위치 : page를 fix 하지 않으므로 바뀔수 있어서 사용시 주의 필요 */
    sCursor->mRowPtr = NULL;

    if ( SM_IS_FLAG_ON( sCursor->mTCFlag, SMI_TCFLAG_HASHSCAN ) )
    {
        sCursor->mFetch     = fetchHashScan;

        /* HashValue를 사용하여 탐색함 */
        sCursor->mGRID      = SC_NULL_GRID;
        sCursor->mChildGRID = SC_NULL_GRID;
    }
    else
    {
        IDE_ERROR( SM_IS_FLAG_ON( sCursor->mTCFlag, SMI_TCFLAG_FULLSCAN ) );

        sCursor->mFetch     = fetchFullScan;
        sCursor->mSeq       = 0;
        sHeader             = (smiTempTableHeader *)aHeader;
        IDE_TEST( getNextNPage( (sdtWASegment*)sHeader->mWASegment, sCursor )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * 커서로부터 HashValue를 바탕으로 다음 Row를 가져옵니다.
 *
 * <IN>
 * aCursor        - 대상 Cursor
 * <OUT>
 * aRow           - 대상 Row
 * aGRID          - 가져온 Row의 GRID
 ***************************************************************************/
IDE_RC sdtUniqueHashModule::fetchHashScan( void           * aCursor,
                                           UChar         ** aRow,
                                           scGRID         * aRowGRID )
{
    smiTempCursor      * sCursor = (smiTempCursor *)aCursor;
    smiTempTableHeader * sHeader = sCursor->mTTHeader;
    sdtWASegment       * sWASeg  = (sdtWASegment*)sHeader->mWASegment;
    idBool               sResult = ID_FALSE;
    idBool               sIsValidSlot = ID_FALSE;
    sdtTRPHeader       * sTRPHeader   = NULL;
    scGRID             * sDummyBucketGRIDPtr;
    UInt                 sIdx = 0;
    UChar              * sPageStartPtr = NULL;
    scPageID             sNPID         = SC_NULL_PID;

    IDE_DASSERT( SM_IS_FLAG_ON( sCursor->mTCFlag, SMI_TCFLAG_HASHSCAN ) );
    /* Cache 정보들 : UniqueHash 는 사용하지 않는다. */
    IDE_DASSERT( sCursor->mWPID      == SC_NULL_PID ) ;
    IDE_DASSERT( sCursor->mWAPagePtr == NULL );
    IDE_DASSERT( sCursor->mSlotCount == SDT_WASLOT_UNUSED );

    do
    {
        if ( SC_GRID_IS_NULL( sCursor->mGRID ) )
        {
            /* 최초 탐색 */
            IDE_TEST( findExactRow( sHeader,
                                    sWASeg,
                                    ID_FALSE, /* reset page */
                                    NULL,     /* aRowFilter */
                                    sCursor->mHashValue,
                                    &sIdx,
                                    (UChar**)&sTRPHeader,
                                    &sDummyBucketGRIDPtr,
                                    &sCursor->mGRID )
                      != IDE_SUCCESS );
        }
        else
        {
            /* 다음 Row를 찾아감 */
            sCursor->mGRID = sCursor->mChildGRID;

#ifdef DEBUG
            if ( !SC_GRID_IS_NULL( sCursor->mChildGRID ) )
            {
                IDE_DASSERT( sctTableSpaceMgr::isTempTableSpace( SC_MAKE_SPACE( sCursor->mChildGRID ) ) == ID_TRUE ); 
            }
#endif

            if ( !SC_GRID_IS_NULL( sCursor->mChildGRID ) )
            {
                IDE_TEST( sdtWASegment::getPagePtrByGRID( sWASeg,
                                                          sCursor->mWAGroupID,
                                                          sCursor->mGRID,
                                                          (UChar**)&sTRPHeader,
                                                          &sIsValidSlot )
                          != IDE_SUCCESS );
                IDE_ERROR( sIsValidSlot == ID_TRUE );
            }
        }

        if ( SC_GRID_IS_NULL( sCursor->mGRID ) )
        {
            sCursor->mChildGRID = SC_NULL_GRID;
            break;
        }
        else
        {
            /* BUG-45474 hash_area_size가 부족하면 비정상종료하거나 SQL 의 결과가 중복된 값이 출력될 수 있습니다. */
            if ( ( SC_MAKE_SPACE( sCursor->mGRID ) != SDT_SPACEID_WAMAP ) &&
                 ( SC_MAKE_SPACE( sCursor->mGRID ) != SDT_SPACEID_WORKAREA ) )
            {
                sPageStartPtr = sdtTempPage::getPageStartPtr( sTRPHeader );
                sNPID         = sdtTempPage::getPID( sPageStartPtr );

                if ( SC_MAKE_PID( sCursor->mGRID ) != sNPID )
                {
                    IDE_TEST( sdtWASegment::getPagePtrByGRID( sWASeg,
                                                              sCursor->mWAGroupID,
                                                              sCursor->mGRID,
                                                              (UChar**)&sTRPHeader,
                                                              &sIsValidSlot )
                            != IDE_SUCCESS );
                    IDE_ERROR( sIsValidSlot == ID_TRUE );
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

            IDE_DASSERT( SM_IS_FLAG_ON( sTRPHeader->mTRFlag, SDT_TRFLAG_CHILDGRID ) );
            IDE_DASSERT( SM_IS_FLAG_ON( sTRPHeader->mTRFlag, SDT_TRFLAG_HEAD ) );

            sCursor->mChildGRID = sTRPHeader->mChildGRID;

            IDE_TEST( sdtTempRow::filteringAndFetch( sCursor,
                                                     (UChar*)sTRPHeader,
                                                     aRow,
                                                     aRowGRID,
                                                     &sResult )
                      != IDE_SUCCESS );
        }
    }
    while( sResult == ID_FALSE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/**************************************************************************
 * Description :
 * 커서로부터 모든 Row를 대상으로 FetchNext로 가져옵니다.
 *
 * <IN>
 * aCursor        - 대상 Cursor
 * <OUT>
 * aRow           - 대상 Row
 * aGRID          - 가져온 Row의 GRID
 ***************************************************************************/
IDE_RC sdtUniqueHashModule::fetchFullScan(void           * aCursor,
                                          UChar         ** aRow,
                                          scGRID         * aRowGRID )
{
    smiTempCursor      * sCursor = (smiTempCursor *)aCursor;
    smiTempTableHeader * sHeader = sCursor->mTTHeader;
    sdtWASegment       * sWASeg  = (sdtWASegment*)sHeader->mWASegment;
    idBool               sResult = ID_FALSE;
    idBool               sIsValidSlot = ID_FALSE;
    sdtTRPHeader       * sTRPHeader = NULL;

    IDE_DASSERT( SM_IS_FLAG_ON( sCursor->mTCFlag, SMI_TCFLAG_FULLSCAN ) );

    while( !SC_GRID_IS_NULL( sCursor->mChildGRID ) )
    {
        IDE_TEST( sdtWASegment::getPagePtrByGRID( sWASeg,
                                                  sCursor->mWAGroupID,
                                                  sCursor->mChildGRID,
                                                  (UChar**)&sTRPHeader,
                                                  &sIsValidSlot )
                  != IDE_SUCCESS );
        IDE_ERROR( sIsValidSlot == ID_TRUE );

        IDE_DASSERT( SM_IS_FLAG_ON( sTRPHeader->mTRFlag, SDT_TRFLAG_CHILDGRID ) );
        IDE_DASSERT( SM_IS_FLAG_ON( sTRPHeader->mTRFlag, SDT_TRFLAG_HEAD ) );

        sCursor->mGRID      = sCursor->mChildGRID;
        sCursor->mChildGRID = sTRPHeader->mChildGRID;

#ifdef DEBUG
        if ( !SC_GRID_IS_NULL( sCursor->mChildGRID ) )
        {
            IDE_DASSERT( sctTableSpaceMgr::isTempTableSpace( SC_MAKE_SPACE( sCursor->mChildGRID ) ) == ID_TRUE ); 
        }
#endif
        IDE_TEST( sdtTempRow::filteringAndFetch( sCursor,
                                                 (UChar*)sTRPHeader,
                                                 aRow,
                                                 aRowGRID,
                                                 &sResult )
                  != IDE_SUCCESS );

        if ( SC_GRID_IS_NULL( sCursor->mChildGRID ) )
        {
            /* 이번 Bucket에 대한 조회는 끝났음 */
            IDE_TEST( getNextNPage( sWASeg, sCursor ) != IDE_SUCCESS );
        }
        if ( sResult == ID_TRUE )
        {
            break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * 커서를 닫습니다.
 *
 * <IN>
 * aCursor        - 대상 Cursor
 ***************************************************************************/
IDE_RC sdtUniqueHashModule::closeCursor(void * /*aTempCursor*/ )
{
    return IDE_SUCCESS;
}

/**************************************************************************
 * Description :
 * Row 비교용 Filter
 *
 * <OUT>
 * aResult - 결과값
 * <IN>
 * aRow    - 대상 Row
 * aGRID   - 대상 Row의 GRID
 * aData   - 비교할 Filter Data
 ***************************************************************************/
IDE_RC sdtUniqueHashModule::compareRowAndValue( idBool         * aResult,
                                                const void     * aRow,
                                                void           *, /* aDirectKey */
                                                UInt            , /* aDirectKeyPartialSize */
                                                const scGRID    , /* aGRID */
                                                void         * aData )
{
    sdtTempCompData    * sData      = (sdtTempCompData*)aData;
    smiTempTableHeader * sHeader    = sData->mHeader;
    const smiValue     * sValueList = sData->mValueList;
    smiTempColumn      * sKeyColumn = NULL;
    smiValueInfo         sValue1;
    smiValueInfo         sValue2;
    UInt                 sColumnCount = 0;

    *aResult = ID_TRUE;
    sKeyColumn = sHeader->mKeyColumnList;

    IDE_ERROR( sKeyColumn != NULL );
    while( sKeyColumn != NULL )
    {
        IDE_ERROR( ++sColumnCount   <= sHeader->mColumnCount );
        IDE_ERROR( sKeyColumn->mIdx <  sHeader->mColumnCount );

        /* PROJ-2180 valueForModule
           SMI_OFFSET_USELESS 로 비교하는 컬럼은 mBlankColumn 을 사용해야 한다.
           Compare 함수에서 valueForModule 을 호출하지 않고
           offset 을 사용할수 있기 때문이다. */
        sValue1.column = sHeader->mBlankColumn;
        sValue1.value  = sValueList[ sKeyColumn->mIdx ].value;
        sValue1.length = sValueList[ sKeyColumn->mIdx ].length;
        sValue1.flag   = SMI_OFFSET_USELESS;

        sValue2.column = &sHeader->mColumns[  sKeyColumn->mIdx ].mColumn;
        sValue2.value  = aRow;
        sValue2.length = 0;
        sValue2.flag   = SMI_OFFSET_USE;

        if ( sKeyColumn->mCompare( &sValue1,
                                   &sValue2 ) != 0 )
        {
            *aResult = ID_FALSE;
            break;
        }
        else
        {
            /* nothing  to do */
        }
        sKeyColumn = sKeyColumn->mNextKeyColumn;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * FullScan일때, Scan할 다음 Page정보를 가져와 설정한다.
 *
 * <IN>
 * aCursor        - 대상 Cursor
 * <OUT>
 * aRow           - 대상 Row
 * aGRID          - 가져온 Row의 GRID
 ***************************************************************************/
IDE_RC sdtUniqueHashModule::getNextNPage( sdtWASegment   * aWASegment,
                                          smiTempCursor  * aCursor )
{
    scGRID             * sBucketGRIDPtr;

    do
    {
        if ( aCursor->mSeq < sdtWAMap::getSlotCount( &aWASegment->mSortHashMapHdr ) )
        {
            IDE_TEST( sdtWAMap::getSlotPtrWithCheckState(
                                                      &aWASegment->mSortHashMapHdr,
                                                      aCursor->mSeq,
                                                      ID_FALSE, /* resetPage */
                                                      (void**)&sBucketGRIDPtr )
                      != IDE_SUCCESS );
            aCursor->mChildGRID = *sBucketGRIDPtr;
            aCursor->mSeq ++;

#ifdef DEBUG
            if ( !SC_GRID_IS_NULL( aCursor->mChildGRID ) )
            {
                IDE_DASSERT( sctTableSpaceMgr::isTempTableSpace( SC_MAKE_SPACE( aCursor->mChildGRID ) ) == ID_TRUE ); 
            }
#endif
        }
        else
        {
            SC_MAKE_NULL_GRID( aCursor->mChildGRID );
            break;
        }
    }
    while( SC_GRID_IS_NULL( aCursor->mChildGRID ) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
