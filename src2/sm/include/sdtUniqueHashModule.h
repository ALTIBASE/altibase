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
 

#ifndef _O_SDT_UNIQUE_HASH_H_
#define _O_SDT_UNIQUE_HASH_H_ 1

#include <idu.h>
#include <smDef.h>
#include <smnDef.h>
#include <sdtWAMap.h>
#include <sdtDef.h>
#include <sdtTempRow.h>

extern smnIndexModule sdtUniqueHashModule;

class sdtUniqueHashModule
{
public:
    static IDE_RC init( void * aHeader );
    static IDE_RC destroy( void * aHeader);

    /************************ Interface ***********************/
    static IDE_RC insert(void     * aTempTableHeader,
                         smiValue * aValue,
                         UInt       aHashValue,
                         scGRID   * aGRID,
                         idBool   * aResult );
    static IDE_RC openCursor(void * aTempTableHeader,
                             void * aCursor );
    static IDE_RC fetchHashScan(void    * aTempCursor,
                                UChar  ** aRow,
                                scGRID  * aRowGRID );
    static IDE_RC fetchFullScan(void    * aTempCursor,
                                UChar  ** aRow,
                                scGRID  * aRowGRID );
    static IDE_RC closeCursor(void * aTempCursor);

    /*********************** Internal **************************/
    static IDE_RC compareRowAndValue( idBool        * aResult,
                                      const void    * aRow,
                                      void          *, /* aDirectKey */
                                      UInt           , /* aDirectKeyPartialSize */
                                      const scGRID    aGRID,
                                      void          * aData );

    inline static IDE_RC findExactRow(smiTempTableHeader  * aHeader,
                                      sdtWASegment        * aWASegment,
                                      idBool                aResetPage,
                                      const smiCallBack   * aFilter,
                                      UInt                  aHashValue,
                                      UInt                * aMapIdx,
                                      UChar              ** aSrcRowPtr,
                                      scGRID             ** aBucketGRID,
                                      scGRID              * aTargetGRID );
    inline static IDE_RC checkNextRow( smiTempTableHeader  * aHeader,
                                       sdtWASegment        * aWASegment,
                                       const smiCallBack   * aFilter,
                                       UInt                  aHashValue,
                                       scGRID              * aGRID,
                                       UChar              ** aSrcRowPtr,
                                       idBool              * aResult );

    static IDE_RC getNextNPage( sdtWASegment   * aWASegment,
                                smiTempCursor  * aCursor );
};

/**************************************************************************
 * Description :
 * hashValue를 바탕으로 자신과 같은 hashValue 및 value를 가진 Row를
 * 찾습니다.
 * 찾으면 rowPointer 및 RID를 리턴하고, 없으면
 * NULL, NULL_RID를 반환합니다.
 *
 * <IN>
 * aHeader        - 대상 Table
 * aWASegment     - 대상 WASegment
 * aResetPage     - reset page
 * aFilter        - 가져올 Row에 대한 Filter
 * aHashValue     - 탐색할 대상 Hash
 * aMapIndx       - 탐색을 시작할 HashMap의 Index번호
 * <OUT>
 * aRowPtr        - Row를 탐색한 결과
 * aBucketGRID    - Hash값에 해당하는 BucketGRID의 포인터
 * aTargetGRID    - 찾은 Row
 ***************************************************************************/
IDE_RC sdtUniqueHashModule::findExactRow( smiTempTableHeader  * aHeader,
                                          sdtWASegment        * aWASeg,
                                          idBool                aResetPage,
                                          const smiCallBack   * aFilter,
                                          UInt                  aHashValue,
                                          UInt                * aMapIdx,
                                          UChar              ** aSrcRowPtr,
                                          scGRID             ** aBucketGRID,
                                          scGRID              * aTargetGRID )
{
    idBool              sResult;
    UInt                sLoop = 0;

    /*MapSize를 바탕으로, Map내 SlotIndex를 찾음 */
    *aMapIdx = aHashValue % sdtWAMap::getSlotCount( &aWASeg->mSortHashMapHdr );

    IDE_TEST( sdtWAMap::getSlotPtrWithCheckState( &aWASeg->mSortHashMapHdr,
                                                  *aMapIdx,
                                                  aResetPage,
                                                  (void**)aBucketGRID )
              != IDE_SUCCESS );
    *aTargetGRID = **aBucketGRID;

    while( !SC_GRID_IS_NULL( *aTargetGRID ) )
    {
        sLoop ++;
        IDE_TEST( checkNextRow( aHeader,
                                aWASeg,
                                aFilter,
                                aHashValue,
                                aTargetGRID,
                                aSrcRowPtr,
                                &sResult )
                  != IDE_SUCCESS );
        if( sResult == ID_TRUE )
        {
            /* 찾았다! */
            break;
        }
    }
    aHeader->mStatsPtr->mExtraStat2 =
        IDL_MAX( aHeader->mStatsPtr->mExtraStat2, sLoop );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**************************************************************************
 * Description :
 * Row의 다음 Row에 대해 HaahValue가 같은지, 가져올 대상인지 체크함
 *
 * <IN>
 * aHeader        - 대상 Table
 * aWASegment     - 대상 WASegment
 * aFilter        - 가져올 Row에 대한 Filter
 * aHashValue     - 탐색할 대상 Hash
 * <OUT>
 * aGRID          - 대상 Row의 위치
 * aSrcRowPtr     - Row를 탐색한 결과
 * aResult        - 탐색 성공 여부
 ***************************************************************************/
IDE_RC sdtUniqueHashModule::checkNextRow( smiTempTableHeader  * aHeader,
                                          sdtWASegment        * aWASegment,
                                          const smiCallBack   * aFilter,
                                          UInt                  aHashValue,
                                          scGRID              * aGRID,
                                          UChar              ** aSrcRowPtr,
                                          idBool              * aResult )
{
    sdtTRPHeader      * sTRPHeader = NULL;
    sdtTRPInfo4Select   sTRPInfo;
    idBool              sIsValidSlot = ID_FALSE;
    scGRID              sChildGRID;

    IDE_TEST( sdtWASegment::getPagePtrByGRID( aWASegment,
                                              SDT_WAGROUPID_SUB,
                                              *aGRID,
                                              (UChar**)&sTRPHeader,
                                              &sIsValidSlot )
              != IDE_SUCCESS );
    IDE_ERROR( sIsValidSlot == ID_TRUE );
    IDE_ERROR( SM_IS_FLAG_ON( sTRPHeader->mTRFlag, SDT_TRFLAG_HEAD ) );

    *aSrcRowPtr =(UChar*) sTRPHeader;

    /*BUG-45474 fetch 중에 replace 가 발생할수 있으므로 GRID를 저장해 두고 사용한다. */
    sChildGRID = sTRPHeader->mChildGRID;
#ifdef DEBUG
    if ( !SC_GRID_IS_NULL( sChildGRID ) )
    {
        IDE_DASSERT( sctTableSpaceMgr::isTempTableSpace( SC_MAKE_SPACE( sChildGRID ) ) == ID_TRUE );
    }
#endif

    if( sTRPHeader->mHashValue == aHashValue )
    {
        if( aFilter != NULL )
        {
            SC_COPY_GRID( *aGRID, sTRPInfo.mTRPHGRID );
            IDE_TEST( sdtTempRow::fetch( aWASegment,
                                         SDT_WAGROUPID_SUB,
                                         (UChar*)sTRPHeader,
                                         aHeader->mRowSize,
                                         aHeader->mRowBuffer4Fetch,
                                         &sTRPInfo )
                      != IDE_SUCCESS );

            IDE_TEST( aFilter->callback( aResult,
                                         sTRPInfo.mValuePtr,
                                         NULL,
                                         0,
                                         SC_NULL_GRID,
                                         aFilter->data)
                      != IDE_SUCCESS );

            if( *aResult == ID_FALSE )
            {
                *aGRID = sChildGRID;
            }
        }
        else
        {
            *aResult = ID_TRUE;
        }
    }
    else
    {
        /* HashValue조차 다름 */
        *aGRID = sChildGRID;
        *aResult = ID_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}


#endif /* _O_SDT_UNIQUE_HASH_H_ */
