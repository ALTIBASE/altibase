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
 * $Id: qmcDiskSortTempTable.cpp 86786 2020-02-27 08:04:12Z donovan.seo $
 *
 * Description :
 *     Disk Sort Temp Table
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qcg.h>
#include <qdParseTree.h>
#include <qdx.h>
#include <qmoKeyRange.h>
#include <qcm.h>
#include <qcmTableSpace.h>
#include <qmcSortTempTable.h>
#include <qmcDiskSortTempTable.h>
#include <qdbCommon.h>

extern mtdModule mtdInteger;

IDE_RC
qmcDiskSort::init( qmcdDiskSortTemp * aTempTable,
                   qcTemplate       * aTemplate,
                   qmdMtrNode       * aRecordNode,
                   qmdMtrNode       * aSortNode,
                   SDouble            aStoreRowCount,
                   UInt               aMtrRowSize,
                   UInt               aFlag )
{
/***********************************************************************
 *
 * Description :
 *    Disk Temp Table을 초기화한다.
 *
 * Implementation :
 *    - Disk Temp Table의 Member를 초기화한다.
 *    - Record 구성 정보로부터 Disk Temp Table을 생성한다.
 *    - Sort 구성 정보로부터 Index를 생성한다.
 *    - Insert를 위한 Cursor를 연다.
 *
 ***********************************************************************/

#define IDE_FN "qmcDiskSort::init"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcDiskSort::init"));

    UInt i;
    qmdMtrNode * sNode;

    IDE_DASSERT( aTempTable != NULL );
    IDE_DASSERT( aRecordNode != NULL );

    //----------------------------------------------------------------
    // Disk Temp Table 멤버의 초기화
    //----------------------------------------------------------------
    aTempTable->flag      = aFlag;
    aTempTable->mTemplate = aTemplate;

    aTempTable->tableHandle = NULL;
    aTempTable->indexHandle = NULL;

    tempSizeEstimate( aTempTable, aStoreRowCount, aMtrRowSize );

    //---------------------------------------
    // Record 구성을 위한 자료 구조의 초기화
    //---------------------------------------
    aTempTable->recordNode = aRecordNode;

    // [Record Column 개수의 결정]
    // Record Node의 개수로 판단하지 않고,
    // Tuple Set의 정보로부터 Column 개수를 판단한다.
    // 이는 하나의 Record Node가 여러 개의 Column으로 구성될 수 있기 때문이다.

    IDE_DASSERT( aRecordNode->dstTuple->columnCount > 0 );
    aTempTable->recordColumnCnt = aRecordNode->dstTuple->columnCount;

    aTempTable->insertColumn = NULL;
    aTempTable->insertValue = NULL;
    aTempTable->nullRow = NULL;

    //---------------------------------------
    // Sorting을 위한 자료 구조의 초기화
    //---------------------------------------

    aTempTable->sortNode = aSortNode;

    // [Sort Column 개수의 결정]
    // Record Column과 달리 Sort Node 정보로부터 Column 개수를 판단한다.
    // 하나의 Node가 여러 개의 Column으로 구성된다 하더라도
    // Sorting 대상이 되는 Column은 하나이기 때문이다.

    for ( i = 0, sNode = aTempTable->sortNode;
          sNode != NULL;
          sNode = sNode->next, i++ ) ;
    aTempTable->indexColumnCnt = i;

    aTempTable->indexKeyColumnList = NULL;
    aTempTable->indexKeyColumn = NULL;
    aTempTable->searchCursor = NULL;

    aTempTable->rangePredicate = NULL;

    // PROJ-1431 : sparse B-Tree temp index로 구조변경
    // leaf page(data page)에서 row를 비교하기 위해 rowRange가 추가됨
    /* PROJ-2201 : Dense구조로 다시 변경
     * Sparse구조는 BufferMiss상황에서 I/O를 많이 유발시킴 */
    aTempTable->keyRange       = NULL;
    aTempTable->keyRangeArea   = NULL;
    aTempTable->keyRangeAreaSize = 0;

    // 갱신(update)를 위한 자료 구조의 초기화
    aTempTable->updateColumnList = NULL;
    aTempTable->updateValues     = NULL;

    //----------------------------------------------------------------
    // Disk Temp Table 의 생성
    //----------------------------------------------------------------

    IDE_TEST( createTempTable( aTempTable ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

/******************************************************************************
 *
 * Description : BUG-37862 disk sort temp table size estimate
 *                WAMap size + temp table size
 *
 *****************************************************************************/
void  qmcDiskSort::tempSizeEstimate( qmcdDiskSortTemp * aTempTable,
                                     SDouble            aStoreRowCount,
                                     UInt               aMtrRowSize )
{
    ULong        sTempSize;

    // BUG-37862 disk sort temp table size estimate
    // aStoreRowCount 는 disk temp table을 위한 stored row count이다.
    // BUG-43443 temp table에 대해서 work area size를 estimate하는 기능을 off
    if ( (QCU_DISK_TEMP_SIZE_ESTIMATE == 1) &&
         (aStoreRowCount != 0) )
    {
        // sdtSortModule::calcEstimatedStats( smiTempTableHeader * aHeader ) 를 참조함
        sTempSize = (ULong)(SMI_TR_HEADER_SIZE_FULL + aMtrRowSize) * DOUBLE_TO_UINT64(aStoreRowCount);
        sTempSize = (ULong)(sTempSize * 100.0 / smuProperty::getTempSortGroupRatio() * 1.3);

        aTempTable->mTempTableSize = IDL_MIN( sTempSize, smuProperty::getSortAreaSize());
    }
    else
    {
        // 0을 지정하면 SM에서 smuProperty::getSortAreaSize() 로 설정한다.
        aTempTable->mTempTableSize = 0;
    }
}

IDE_RC
qmcDiskSort::clear( qmcdDiskSortTemp * aTempTable )
{
/***********************************************************************
 *
 * Description :
 *    해당 Temp Table을 Truncate한다.
 *
 * Implementation :
 *    열려 있는 커서를 모두 닫고, Table을 Truncate한다.
 *
 ***********************************************************************/

#define IDE_FN "qmcDiskSort::clear"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcDiskSort::clear"));

    IDE_TEST( smiSortTempTable::clear( aTempTable->tableHandle )
              != IDE_SUCCESS );

    aTempTable->searchCursor = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
#undef IDE_FN
}

IDE_RC
qmcDiskSort::clearHitFlag( qmcdDiskSortTemp * aTempTable )
{
/***********************************************************************
 *
 * Description :
 *    Temp Table내에 존재하는 모든 Record의 Hit Flag을 초기화한다.
 *
 * Implementation :
 *    Memory Temp Table과 달리 모든 Record에 대하여 초기화하지 않고,
 *    Hit Flag이 Setting된 것만을 초기화한다.
 *    이는 Hit Flag의 초기화가 Update연산을 발생시켜, Disk I/O 부하가
 *    증폭될 수 있기 때문이다.
 *
 *    - Hit Flag 이 설정된 것을 검사하는 Filter CallBack을 생성한다.
 *    - Hit Flag Search Cursor를 Open한다.
 *    - 다음을 반복한다.
 *        - 부합하는 Record 가 있을 경우, Hit Flag을 Clear한다.
 *        - 해당 Record를 Update한다.
 *
 ***********************************************************************/

#define IDE_FN "qmcDiskSort::clearHitFlag"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcDiskSort::clearHitFlag"));

    smiSortTempTable::clearHitFlag( aTempTable->tableHandle );

    return IDE_SUCCESS;
#undef IDE_FN
}



IDE_RC
qmcDiskSort::insert( qmcdDiskSortTemp * aTempTable, void * aRow )
{
/***********************************************************************
 *
 * Description :
 *    Record를 Temp Table에 삽입한다.
 *
 * Implementation :
 *    - Cursor를 별도로 열지 않는다.  초기화 또는 Clear시 Insert Cursor는
 *      이미 열어 놓는다.
 *    - Row의 Hit Flag을 Clear한다.
 *    - smiValue를 삽입한다
 ***********************************************************************/

#define IDE_FN "qmcDiskSort::insert"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcDiskSort::insert"));

    // 적합성 검사
    // insert value 정보는 dstTuple->row를 중심으로 구성된다.
    // 따라서, aRow의 값은 항상 일정하여야 한다.
    IDE_DASSERT( aRow == aTempTable->recordNode->dstTuple->row );

    // PROJ-1597 Temp record size 제약 제거
    // 매 row마다 smiValue를 재구성한다.
    IDE_TEST( makeInsertSmiValue(aTempTable) != IDE_SUCCESS );

    // Temp Table에 Record를 삽입한다.
    IDE_TEST( smiSortTempTable::insert( aTempTable->tableHandle,
                                        aTempTable->insertValue )
         != IDE_SUCCESS )

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC
qmcDiskSort::setHitFlag( qmcdDiskSortTemp * aTempTable )
{
/***********************************************************************
 *
 * Description :
 *    현재 Cursor 위치의 Record의 Hit Flag을 Setting한다.
 *
 * Implementation :
 *    검색 후에만 호출된다.
 *    이미 검색된 Record에 대한 Hit Flag을 Setting하면 된다.
 *
 ***********************************************************************/

#define IDE_FN "qmcDiskSort::setHitFlag"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcDiskSort::setHitFlag"));

    smiSortTempTable::setHitFlag( aTempTable->searchCursor );

    return IDE_SUCCESS;
#undef IDE_FN
}

idBool qmcDiskSort::isHitFlagged( qmcdDiskSortTemp * aTempTable )
{
/***********************************************************************
 *
 * Description : PROJ-2385
 *    현재 Cursor 위치의 Record에 Hit Flag가 있는지 판단.
 *
 * Implementation :
 *
 ***********************************************************************/

    return smiSortTempTable::isHitFlagged( aTempTable->searchCursor );
}


IDE_RC
qmcDiskSort::getFirstSequence( qmcdDiskSortTemp * aTempTable,
                               void            ** aRow )
{
/***********************************************************************
 *
 * Description :
 *    순차 검색을 한다.
 *
 * Implementation :
 *    Search Cursor를 순차 검색을 위해 열고, 해당 Row를 얻어온다.
 *    Record를 위한 공간 관리(공간 정보의 상실)를
 *    Plan Node에서 주의 깊게 처리하여야 한다.
 *
 ***********************************************************************/

#define IDE_FN "qmcDiskSort::getFirstSequence"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcDiskSort::getFirstSequence"));

    // 적합성 검사
    // To Fix PR-7995
    IDE_DASSERT( *aRow == aTempTable->recordNode->dstTuple->row );

    // 순차 검색 Cursor를 연다.
    IDE_TEST( openCursor(aTempTable, 
                         SMI_TCFLAG_FORWARD | 
                         SMI_TCFLAG_ORDEREDSCAN |
                         SMI_TCFLAG_IGNOREHIT,
                         smiGetDefaultKeyRange(),
                         smiGetDefaultKeyRange(),
                         smiGetDefaultFilter(),
                         &aTempTable->searchCursor )
              != IDE_SUCCESS );

    // Record를 얻어 온다.
    IDE_TEST( smiSortTempTable::fetch( aTempTable->searchCursor,
                                       (UChar**) aRow,
                                       & aTempTable->recordNode->dstTuple->rid )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmcDiskSort::getNextSequence( qmcdDiskSortTemp  * aTempTable,
                              void             ** aRow )
{
/***********************************************************************
 *
 * Description :
 *    다음 순차 검색을 한다.
 *    반드시 getFirstSequence() 이후에 호출되어야 한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmcDiskSort::getNextSequence"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcDiskSort::getNextSequence"));

    // 적합성 검사
    // To Fix PR-7995
    IDE_DASSERT( *aRow == aTempTable->recordNode->dstTuple->row );

    // Record를 얻어 온다.
    IDE_TEST( smiSortTempTable::fetch( aTempTable->searchCursor,
                                       (UChar**) aRow,
                                       & aTempTable->recordNode->dstTuple->rid )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmcDiskSort::getFirstRange( qmcdDiskSortTemp  * aTempTable,
                            qtcNode           * aRangePredicate,
                            void             ** aRow )
{
/***********************************************************************
 *
 * Description :
 *    첫번째 Range 검색을 한다.
 *
 * Implementation :
 *    다음과 같은 절차에 의하여 Range검색을 수행한다.
 *
 *        - 주어진 Predicate을 위한 Key Range를 생성한다.
 *        - Range Cursor를 Open 한다.
 *        - Cursor를 이용하여 조건에 맞는 Row를 가져온다.
 *
 ***********************************************************************/

#define IDE_FN "qmcDiskSort::getFirstRange"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcDiskSort::getFirstRange"));

    // 적합성 검사
    IDE_DASSERT( aRangePredicate != NULL );
    IDE_DASSERT( *aRow == aTempTable->recordNode->dstTuple->row );

    // Key Range 생성
    aTempTable->rangePredicate = aRangePredicate;
    // PROJ-1431 : key range와 row range를 생성
    IDE_TEST( makeRange( aTempTable ) != IDE_SUCCESS );

    // Range Cursor를 Open한다.
    IDE_TEST( openCursor(aTempTable, 
                         SMI_TCFLAG_FORWARD | 
                         SMI_TCFLAG_RANGESCAN |
                         SMI_TCFLAG_IGNOREHIT,
                         aTempTable->keyRange,
                         smiGetDefaultKeyRange(),
                         smiGetDefaultFilter(),
                         &aTempTable->searchCursor )
              != IDE_SUCCESS );

    // Record를 얻어 온다.
    IDE_TEST( smiSortTempTable::fetch( aTempTable->searchCursor,
                                       (UChar**) aRow,
                                       & aTempTable->recordNode->dstTuple->rid )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC
qmcDiskSort::getNextRange( qmcdDiskSortTemp * aTempTable,
                           void            ** aRow )
{
/***********************************************************************
 *
 * Description :
 *    다음 Range 검색을 한다.
 *    반드시 getFirstRange() 이후에 호출되어야 한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmcDiskSort::getNextRange"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcDiskSort::getNextRange"));

    // 적합성 검사
    IDE_DASSERT( *aRow == aTempTable->recordNode->dstTuple->row );

    // Cursor를 이용하여 조건에 맞는 Row를 가져온다.
    IDE_TEST( smiSortTempTable::fetch( aTempTable->searchCursor,
                                       (UChar**) aRow,
                                       & aTempTable->recordNode->dstTuple->rid )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC qmcDiskSort::getFirstHit( qmcdDiskSortTemp * aTempTable,
                                 void            ** aRow )
{
/***********************************************************************
 *
 * Description : PROJ-2385
 *    최초 Hit된 Record를 검색한다.
 *
 * Implementation :
 *    다음과 같은 절차에 의하여 Hit 검색을 수행한다.
 *        - Hit Filter를 생성한다.
 *        - Hit을 위한 Cursor를 연다.
 *        - 조건을 만족하는 Record를 읽어온다.
 *
 ***********************************************************************/

    // 적합성 검사
    IDE_DASSERT( *aRow == aTempTable->recordNode->dstTuple->row );

    IDE_TEST( openCursor(aTempTable, 
                         SMI_TCFLAG_FORWARD | 
                         SMI_TCFLAG_ORDEREDSCAN |
                         SMI_TCFLAG_HIT,
                         smiGetDefaultKeyRange(),
                         smiGetDefaultKeyRange(),
                         smiGetDefaultFilter(),
                         &aTempTable->searchCursor )
              != IDE_SUCCESS );

    // Record를 얻어 온다.
    IDE_TEST( smiSortTempTable::fetch( aTempTable->searchCursor,
                                       (UChar**) aRow,
                                       & aTempTable->recordNode->dstTuple->rid )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qmcDiskSort::getNextHit( qmcdDiskSortTemp * aTempTable,
                                void            ** aRow )
{
/***********************************************************************
 *
 * Description : PROJ-2385
 *    다음 Hit된 Record를 검색한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    // 적합성 검사
    IDE_DASSERT( *aRow == aTempTable->recordNode->dstTuple->row );

    // Cursor를 이용하여 조건에 맞는 Row를 가져온다.
    IDE_TEST( smiSortTempTable::fetch( aTempTable->searchCursor,
                                       (UChar**) aRow,
                                       & aTempTable->recordNode->dstTuple->rid )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmcDiskSort::getFirstNonHit( qmcdDiskSortTemp * aTempTable,
                             void            ** aRow )
{
/***********************************************************************
 *
 * Description :
 *    최초 Hit되지 않은 Record를 검색한다.
 *
 * Implementation :
 *    다음과 같은 절차에 의하여 NonHiit 검색을 수행한다.
 *        - NonHit Filter를 생성한다.
 *        - NonHit을 위한 Cursor를 연다.
 *        - 조건을 만족하는 Record를 읽어온다.
 *
 ***********************************************************************/

#define IDE_FN "qmcDiskSort::getFirstNonHit"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcDiskSort::getFirstNonHit"));

    // 적합성 검사
    IDE_DASSERT( *aRow == aTempTable->recordNode->dstTuple->row );

    IDE_TEST( openCursor(aTempTable, 
                         SMI_TCFLAG_FORWARD | 
                         SMI_TCFLAG_ORDEREDSCAN |
                         SMI_TCFLAG_NOHIT,
                         smiGetDefaultKeyRange(),
                         smiGetDefaultKeyRange(),
                         smiGetDefaultFilter(),
                         &aTempTable->searchCursor )
              != IDE_SUCCESS );

    // Record를 얻어 온다.
    IDE_TEST( smiSortTempTable::fetch( aTempTable->searchCursor,
                                       (UChar**) aRow,
                                       & aTempTable->recordNode->dstTuple->rid )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC
qmcDiskSort::getNextNonHit( qmcdDiskSortTemp * aTempTable,
                            void            ** aRow )
{
/***********************************************************************
 *
 * Description :
 *    다음 Hit되지 않은 Record를 검색한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmcDiskSort::getNextNonHit"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcDiskSort::getNextNonHit"));

    // 적합성 검사
    IDE_DASSERT( *aRow == aTempTable->recordNode->dstTuple->row );

    // Cursor를 이용하여 조건에 맞는 Row를 가져온다.
    IDE_TEST( smiSortTempTable::fetch( aTempTable->searchCursor,
                                       (UChar**) aRow,
                                       & aTempTable->recordNode->dstTuple->rid )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmcDiskSort::getNullRow( qmcdDiskSortTemp * aTempTable,
                         void            ** aRow )
{
/***********************************************************************
 *
 * Description :
 *    NULL ROW를 획득한다.
 *
 * Implementation :
 *    Null Row를 획득한 적이 없다면, Null Row를 획득
 *    저장된 해당 영역에 Null Row를 복사한다.
 *    SM의 Null Row를 저장하는 이유는 빈번한 호출로 인한 Disk I/O를
 *    방지하기 위함이다.
 *
 ***********************************************************************/

#define IDE_FN "qmcDiskSort::getNullRow"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcDiskSort::getNullRow"));

    // 적합성 검사
    IDE_DASSERT( *aRow == aTempTable->recordNode->dstTuple->row );

    // 저장된 Null Row를 해당 영역에 복사해 준다.
    IDE_TEST( smiTempTable::getNullRow( aTempTable->tableHandle,
                                        (UChar**)aRow )
              != IDE_SUCCESS );

    SMI_MAKE_VIRTUAL_NULL_GRID( aTempTable->recordNode->dstTuple->rid );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC
qmcDiskSort::getCurrPosition( qmcdDiskSortTemp * aTempTable,
                              smiCursorPosInfo * aCursorInfo )
{
/***********************************************************************
 *
 * Description :
 *    현재 위치의 Cursor를 저장한다.
 *
 * Implementation :
 *    Cursor 정보와 RID정보를 저장한다.
 *
 *
 ***********************************************************************/

#define IDE_FN "qmcDiskSort::getCurrPosition"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcDiskSort::getCurrPosition"));

    // 적합성 검사
    // Search Cursor가 열려 있어야 함.
    IDE_DASSERT( aTempTable->searchCursor != NULL );
    IDE_DASSERT( aCursorInfo != NULL );

    // Cursor 정보를 저장함.
    IDE_TEST( smiSortTempTable::storeCursor( aTempTable->searchCursor,
                                             (smiTempPosition**)
                                             &aCursorInfo->mCursor.mDTPos.mPos )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC
qmcDiskSort::setCurrPosition( qmcdDiskSortTemp * aTempTable,
                              smiCursorPosInfo * aCursorInfo )
{
/***********************************************************************
 *
 * Description :
 *    지정된 Cursor 위치로 복원시킨다.
 *
 * Implementation :
 *    Cursor를 복원하고 RID로부터 Data를 복원한다.
 *
 ***********************************************************************/

#define IDE_FN "qmcDiskSort::setCurrPosition"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcDiskSort::setCurrPosition"));

    // 적합성 검사
    // Search Cursor가 열려 있어야 함.
    IDE_DASSERT( aTempTable->searchCursor != NULL );
    IDE_DASSERT( aCursorInfo != NULL );

    IDE_TEST( smiSortTempTable::restoreCursor( 
                aTempTable->searchCursor,
                (smiTempPosition*) aCursorInfo->mCursor.mDTPos.mPos,
                (UChar**)&aTempTable->recordNode->dstTuple->row,
                &aTempTable->recordNode->dstTuple->rid )
            != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmcDiskSort::getCursorInfo( qmcdDiskSortTemp * aTempTable,
                            void            ** aTableHandle,
                            void            ** aIndexHandle )
{
/***********************************************************************
 *
 * Description :
 *
 *    View SCAN등에서 직접 접근하기 위한 정보 추출
 *
 * Implementation :
 *
 *    Table Handle과 Index Handle을 넘겨준다.
 *
 ***********************************************************************/

#define IDE_FN "qmcDiskSort::getCursorInfo"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcDiskSort::getCursorInfo"));

    *aTableHandle = aTempTable->tableHandle;
    *aIndexHandle = aTempTable->indexHandle;

    return IDE_SUCCESS;

#undef IDE_FN
}


IDE_RC
qmcDiskSort::getDisplayInfo( qmcdDiskSortTemp * aTempTable,
                             ULong            * aPageCount,
                             SLong            * aRecordCount )
{
/***********************************************************************
 *
 * Description :
 *    Temp Table이 관리하고 있는 Data에 대한 통계 정보 추출
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmcDiskSort::getDisplayInfo"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcDiskSort::getDisplayInfo"));

    smiSortTempTable::getDisplayInfo( aTempTable->tableHandle,
                                      aPageCount,
                                      aRecordCount );
    return IDE_SUCCESS;
#undef IDE_FN
}

IDE_RC
qmcDiskSort::setSortNode( qmcdDiskSortTemp * aTempTable,
                          const qmdMtrNode * aSortNode )
{
/***********************************************************************
 *
 * Description 
 *    현재 정렬키(sortNode)를 변경
 *    Window Sort(WNST)에서는정렬을 반복하기 위해 사용된다.
 *
 * Implementation :
 *
 ***********************************************************************/
#define IDE_FN "qmcDiskSort::setSortNode"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcDiskSort::setSortNode"));

    qmdMtrNode * sNode;
    UInt         i;

    // Search Cursor가 열려 있다면 닫아야 함
    IDE_TEST( smiSortTempTable::closeAllCursor( aTempTable->tableHandle )
              != IDE_SUCCESS );
    aTempTable->searchCursor = NULL;

    /* SortNode 설정함 */
    aTempTable->sortNode = (qmdMtrNode*) aSortNode;

    /* 다시 SortNode의 개수를 세어, IndexColumn의 개수를 알아냄 */
    for ( i = 0, sNode = aTempTable->sortNode;
          sNode != NULL;
          sNode = sNode->next, i++ );
    aTempTable->indexColumnCnt = i;

    IDE_TEST( makeIndexColumnInfoInKEY( aTempTable ) != IDE_SUCCESS );

    IDE_TEST( smiSortTempTable::resetKeyColumn( 
            aTempTable->tableHandle,
            aTempTable->indexKeyColumnList )  // key column list
        != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
#undef IDE_FN    
}


IDE_RC
qmcDiskSort::setUpdateColumnList( qmcdDiskSortTemp * aTempTable,
                                  const qmdMtrNode * aUpdateColumns )
/***********************************************************************
 *
 * Description :
 *    update를 수행할 column list를 설정
 *    주의: hitFlag와 함께 사용 불가
 *
 * Implementation :
 *
 ***********************************************************************/
{
#define IDE_FN "qmcDiskSort::setUpdateColumnList"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcDiskSort::setUpdateColumnList"));

    const qmdMtrNode * sNode;
    smiColumnList    * sColumn;

    // 적합성 검사
    IDE_DASSERT( aUpdateColumns != NULL );

    // Search Cursor가 열려 있다면 닫아야 함
    IDE_TEST( smiSortTempTable::closeAllCursor( aTempTable->tableHandle )
              != IDE_SUCCESS );
    aTempTable->searchCursor = NULL;

    // updateColumnList를 위한 공간 할당
    // 이미 setUpdateColumnListForHitFlag에서 미리 할당되어 있을 것
    if( aTempTable->updateColumnList == NULL )
    {
        // WNST의 경우 update column이 사용되는데,
        // update column의 개수를 미리 모두 확인할 수 없기 때문에
        // 중복으로 메모리 공간을 할당 받기 않기 위해 미리 모든 칼럼에 대해 할당함
        // insertColumn을 위한 공간과 같은 크기
        IDU_LIMITPOINT("qmcDiskSort::setUpdateColumnList::malloc");
        IDE_TEST( aTempTable->mTemplate->stmt->qmxMem->alloc(
                ID_SIZEOF(smiColumnList) * aTempTable->recordColumnCnt,
                (void**) & aTempTable->updateColumnList )
            != IDE_SUCCESS);
    }
    else
    {
        // 이미 할당 된 경우
        // do nothing
    }

    // mtrNode 정보로부터 update를 위한 smiColumnList를 생성한다.
    for( sNode = aUpdateColumns,
         sColumn = aTempTable->updateColumnList;
         sNode != NULL;
         sNode = sNode->next,
         sColumn++)
    {
        // smiColumnList를 위한 공간이 연속적으로 할당되어 있으므로
        // 다음 주소 위치가 next
        sColumn->next = sColumn+1;

        // column 정보를 연결
        sColumn->column = & sNode->dstColumn->column;
        
        if( sNode->next == NULL )
        {
            // 마지막 칼럼은 next를 가지지 않음
            sColumn->next = NULL;
        }
    }

    // update를 위한 smiValue 생성
    IDE_TEST( makeUpdateSmiValue( aTempTable ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmcDiskSort::updateRow( qmcdDiskSortTemp * aTempTable )
/***********************************************************************
 *
 * Description :
 *    현재 검색 중인 위치의 row를 update
 *
 * Implementation :
 *
 ***********************************************************************/
{
#define IDE_FN "qmcDiskSort::updateRow"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcDiskSort::updateRow"));

    // 적합성 검사
    // Search Cursor가 열려 있어야 함.
    IDE_DASSERT( aTempTable->searchCursor != NULL );

    // PROJ-1597 Temp record size 제약 제거
    // update할 value들을 세팅한다.
    makeUpdateSmiValue( aTempTable );

    // Disk Temp Table의 실제 Record를 Update한다.
    IDE_TEST( smiSortTempTable::update( aTempTable->searchCursor, 
                                        aTempTable->updateValues )
         != IDE_SUCCESS )
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;    

#undef IDE_FN
}

IDE_RC
qmcDiskSort::createTempTable( qmcdDiskSortTemp * aTempTable )
{
/***********************************************************************
 *
 * Description :
 *    Disk Temp Table을 생성한다.
 *
 * Implementation :
 *
 *    1.  Disk Temp Table 생성을 위한 정보 구성
 *        - Hit Flag을 위한 자료 구조 초기화
 *        - Column List 정보의 구성
 *        - NULL ROW의 생성
 *    2.  Disk Temp Table 생성
 *    3.  Temp Table Manager 에 Table Handle 등록
 *
 ***********************************************************************/

#define IDE_FN "qmcDiskSort::createTempTable"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcDiskSort::createTempTable"));

    UInt                   sFlag ;

    //-------------------------------------------------
    // 1.  Disk Temp Table 생성을 위한 정보 구성
    //-------------------------------------------------
    if( ( aTempTable->flag & QMCD_SORT_TMP_SEARCH_MASK )
        == QMCD_SORT_TMP_SEARCH_RANGE )
    {
        sFlag = SMI_TTFLAG_RANGESCAN;
    }
    else
    {
        sFlag = SMI_TTFLAG_NONE;
    }

    // Insert Column List 정보의 구성
    IDE_TEST( setInsertColumnList( aTempTable ) != IDE_SUCCESS );

    //-----------------------------------------
    // Key 내에서의 Index Column 정보의 구성
    // createTempTable을 하기 위해서는 key column 정보가 먼저 구축되어 있어야 한다.
    //-----------------------------------------
    IDE_TEST( makeIndexColumnInfoInKEY( aTempTable ) != IDE_SUCCESS );

    //-------------------------------------------------
    // 2.  Disk Temp Table의 생성
    //-------------------------------------------------
    IDE_TEST( smiSortTempTable::create( 
                aTempTable->mTemplate->stmt->mStatistics,
                QCG_GET_SESSION_TEMPSPACE_ID( aTempTable->mTemplate->stmt ),
                aTempTable->mTempTableSize, /* aWorkAreaSize */
                QC_SMI_STMT( aTempTable->mTemplate->stmt ), // smi statement
                sFlag,
                aTempTable->insertColumn,        // Table의 Column 구성
                aTempTable->indexKeyColumnList,  // key column list
                0,                               // WorkGroupRatio
                (const void**) & aTempTable->tableHandle ) // Table 핸들
        != IDE_SUCCESS );
    aTempTable->recordNode->dstTuple->tableHandle = aTempTable->tableHandle;

    //-------------------------------------------------
    // 3.  Table Handle을 Temp Table Manager에 등록
    //-------------------------------------------------
    /* BUG-38290 
     * Temp table manager 의 addTempTable 함수는 동시에 호출될 수 없으므로
     * mutex 를 통한 동시성 제어를 하지 않는다.
     */
    IDE_TEST(
        qmcTempTableMgr::addTempTable( aTempTable->mTemplate->stmt->qmxMem,
                                       aTempTable->mTemplate->tempTableMgr,
                                       aTempTable->tableHandle )
        != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmcDiskSort::setInsertColumnList( qmcdDiskSortTemp * aTempTable )
{
/***********************************************************************
 *
 * Description :
 *
 *    Table의 Column List정보를 구성한다.
 *
 * Implementation :
 *    - Column List를 위한 공간 할당
 *    - Column List에 Column 정보의 연결
 *        - Hit Flag을 위한 정보 연결
 *        - 각 Column 정보의 연결
 *
 ***********************************************************************/

#define IDE_FN "qmcDiskSort::setInsertColumnList"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcDiskSort::setInsertColumnList"));

    UInt i;
    mtcColumn * sColumn;

    // Column 개수만큼 Memory 할당
    IDU_FIT_POINT( "qmcDiskSort::setInsertColumnList::alloc::insertColumn",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST( aTempTable->mTemplate->stmt->qmxMem->alloc(
            ID_SIZEOF(smiColumnList) * aTempTable->recordColumnCnt,
            (void**) & aTempTable->insertColumn ) != IDE_SUCCESS);

    //-------------------------------------------
    // 최초 Column은 Hit Flag을 연결하고,
    // 이후의 Column은 Tuple Set 정보를 이용한다.
    //-------------------------------------------

    // To Fix PR-7995
    // Column ID를 초기화해주어야 함
    // Hit Flag의 Column 정보 연결
    // 각 Column들의 Column 정보 연결
    for ( i = 0, sColumn = aTempTable->recordNode->dstTuple->columns;
          i < aTempTable->recordColumnCnt;
          i++ )
    {
        // 이전 Column List의 연결 정보 구성
        if( i < aTempTable->recordColumnCnt - 1 )
        {
            aTempTable->insertColumn[i].next = & aTempTable->insertColumn[i+1];
        }
        else
        {
            /* 마지막 칼럼이면, Null을 연결 */
            aTempTable->insertColumn[i].next = NULL;
        }
        // To Fix PR-7995
        // Column ID를 초기화해주어야 함
        // 현재 Column List의 Column 정보 연결
        sColumn[i].column.id = i;
        aTempTable->insertColumn[i].column = & sColumn[i].column;
    }

    // To Fix PR-7995
    // Fix UMR
    IDE_ERROR( aTempTable->insertColumn[i-1].next == NULL );

    IDE_TEST( aTempTable->mTemplate->stmt->qmxMem->alloc(
            ID_SIZEOF(smiValue) * aTempTable->recordColumnCnt,
            (void**) & aTempTable->insertValue ) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmcDiskSort::sort( qmcdDiskSortTemp * aTempTable )
{
/***********************************************************************
 *
 * Description :
 *    Temp Table내의 Row들을 정렬한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmcDiskSort::sort"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcDiskSort::sort"));

    return smiSortTempTable::sort( aTempTable->tableHandle );
#undef IDE_FN
}

IDE_RC
qmcDiskSort::makeIndexColumnInfoInKEY( qmcdDiskSortTemp * aTempTable )
{
/***********************************************************************
 *
 * Description :
 *    Index생성 시 Record내에서 Index Key로 포함될 컬럼의 정보를
 *    구성한다.  Disk Index의 경우 Key 값을 별도로 저장하기 때문에,
 *    Index 생성 시, Record내에서의 Index Key Column의 정보와
 *    Key내에서의 Index Key Column정보를 구분하여 처리하여야 한다.
 *
 *    즉, Index Column의 경우 Record내에서의 offset과 Key내에서의
 *    offset이 다르게 된다.
 *
 * Implementation :
 *
 *    - Key Column을 위한 mtcColumn 정보 구축
 *    - Key Column을 위한 smiColumnList 구축
 *
 ***********************************************************************/

#define IDE_FN "qmcDiskSort::makeIndexColumnInfoInKEY"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcDiskSort::makeIndexColumnInfoInKEY"));

    UInt            i;
    UInt            sOffset;
    qmdMtrNode    * sNode;

    //------------------------------------------------------------
    // Key 내에서의 Index 대상 Column의 정보 구성
    // 해당 Column이 Key값으로 저장되었을 때,
    // Record내에서의 offset 정보와 달리 Key 내에서의 offset정보로
    // 변경되어야 한다.
    //------------------------------------------------------------

    // WNST의 경우 update column이 사용되는데,
    // 이후 재정렬되면서 변경될 KeyColumn의 개수를 미리 모두 확인할 수 없기
    // 때문에 중복으로 메모리 공간을 할당 받기 않기 위해 미리 모든 칼럼에
    // 대해 할당함
    if( ( aTempTable->indexKeyColumn == NULL ) &&
        ( aTempTable->indexColumnCnt > 0 ) )
    {
        IDU_FIT_POINT( "qmcDiskSort::makeIndexColumnInfoInKEY::alloc::indexKeyColumn",
                        idERR_ABORT_InsufficientMemory );

        IDE_TEST( aTempTable->mTemplate->stmt->qmxMem->alloc(
                ID_SIZEOF(mtcColumn) * aTempTable->recordColumnCnt,
                (void**) & aTempTable->indexKeyColumn ) != IDE_SUCCESS);

        // Key Column List를 위한 공간 할당
        IDU_FIT_POINT( "qmcDiskSort::makeIndexColumnInfoInKEY::alloc::indexKeyColumnList",
                        idERR_ABORT_InsufficientMemory );

        IDE_TEST( aTempTable->mTemplate->stmt->qmxMem->alloc(
                ID_SIZEOF(smiColumnList) * aTempTable->recordColumnCnt,
                (void**) & aTempTable->indexKeyColumnList ) != IDE_SUCCESS);
    }
    else
    {
        // 이미 할당 된 경우
        // do nothing
    }

    //---------------------------------------
    // Key Column의 mtcColumn 정보 생성
    //---------------------------------------

    sOffset = 0;
    for ( i = 0, sNode = aTempTable->sortNode;
          i < aTempTable->indexColumnCnt;
          i++, sNode = sNode->next )
    {
        // Record내의 Column 정보를 복사
        idlOS::memcpy( & aTempTable->indexKeyColumn[i],
                       sNode->dstColumn,
                       ID_SIZEOF(mtcColumn) );

        // fix BUG-8763
        // Index Column의 경우 다음 Flag을 표시하여야 함.
        aTempTable->indexKeyColumn[i].column.flag &= ~SMI_COLUMN_USAGE_MASK;
        aTempTable->indexKeyColumn[i].column.flag |= SMI_COLUMN_USAGE_INDEX;

        // Offset 재조정
        if ( (aTempTable->indexKeyColumn[i].column.flag
              & SMI_COLUMN_TYPE_MASK ) == SMI_COLUMN_TYPE_FIXED )
        {
            // Fixed Column 일 경우
            // Data Type에 맞는 align 조정
            sOffset =
                idlOS::align( sOffset,
                              aTempTable->indexKeyColumn[i].module->align );
            aTempTable->indexKeyColumn[i].column.offset = sOffset;
            aTempTable->indexKeyColumn[i].column.value = NULL;
            sOffset += aTempTable->indexKeyColumn[i].column.size;
        }
        else
        {
            //------------------------------------------
            // Variable Column일 경우
            // Variable Column Header 크기가 저장됨
            // 이에 맞는 align을 수행해야 함.
            //------------------------------------------
            sOffset = idlOS::align( sOffset, 8 );
            aTempTable->indexKeyColumn[i].column.offset = sOffset;
            aTempTable->indexKeyColumn[i].column.value = NULL;

// PROJ_1705_PEH_TODO            
            // Fixed 영역 내에는 Header만 저장된다.
            sOffset += smiGetVariableColumnSize( SMI_TABLE_DISK );
        }

        if ( (sNode->myNode->flag & QMC_MTR_SORT_ORDER_MASK )
             == QMC_MTR_SORT_ASCENDING )
        {
            // Ascending Order 지정
            aTempTable->indexKeyColumn[i].column.flag &=
                ~SMI_COLUMN_ORDER_MASK;
            aTempTable->indexKeyColumn[i].column.flag |=
                SMI_COLUMN_ORDER_ASCENDING;
        }
        else
        {
            // Descending Order 지정
            aTempTable->indexKeyColumn[i].column.flag &=
                ~SMI_COLUMN_ORDER_MASK;
            aTempTable->indexKeyColumn[i].column.flag |=
                SMI_COLUMN_ORDER_DESCENDING;
        }

        /* PROJ-2435 order by nulls first/last */
        if ( ( sNode->myNode->flag & QMC_MTR_SORT_NULLS_ORDER_MASK ) 
             != QMC_MTR_SORT_NULLS_ORDER_NONE )
        {
            if ( ( sNode->myNode->flag & QMC_MTR_SORT_NULLS_ORDER_MASK ) 
                 == QMC_MTR_SORT_NULLS_ORDER_FIRST )
            {
                aTempTable->indexKeyColumn[i].column.flag &= 
                    ~SMI_COLUMN_NULLS_ORDER_MASK;
                aTempTable->indexKeyColumn[i].column.flag |= 
                    SMI_COLUMN_NULLS_ORDER_FIRST;
            }
            else
            {
                aTempTable->indexKeyColumn[i].column.flag &= 
                    ~SMI_COLUMN_NULLS_ORDER_MASK;
                aTempTable->indexKeyColumn[i].column.flag |= 
                    SMI_COLUMN_NULLS_ORDER_LAST;
            }
        }
        else
        {
            aTempTable->indexKeyColumn[i].column.flag &= 
                ~SMI_COLUMN_NULLS_ORDER_MASK;
            aTempTable->indexKeyColumn[i].column.flag |= 
                SMI_COLUMN_NULLS_ORDER_NONE;
        }
    }

    //---------------------------------------
    // Key Column을 위한 smiColumnList 구축
    // 상위에서 구축한 indexKeyColumn정보를 이용하여 구축한다.
    //---------------------------------------

    // Key Column List의 정보 구성
    for ( i = 0; i < aTempTable->indexColumnCnt; i++ )
    {
        // 상위에서 구성한 Column 정보를 이용
        aTempTable->indexKeyColumnList[i].column =
            & aTempTable->indexKeyColumn[i].column;

        // Array Bound Read를 방지하기 위한 검사.
        if ( (i + 1) < aTempTable->indexColumnCnt )
        {
            aTempTable->indexKeyColumnList[i].next =
                & aTempTable->indexKeyColumnList[i+1];
        }
        else
        {
            aTempTable->indexKeyColumnList[i].next = NULL;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC qmcDiskSort::openCursor( qmcdDiskSortTemp  * aTempTable,
                                UInt                aFlag,
                                smiRange          * aKeyRange,
                                smiRange          * aKeyFilter,
                                smiCallBack       * aRowFilter,
                                smiSortTempCursor** aTargetCursor )
{
/***********************************************************************
 *
 * Description :
 *    GroupFini/Group/Search/HitFlag를 위한 Cursor를 연다.
 *
 * Implementation :
 *    1. Cursor가 한 번도 Open되지 않은 경우,
 *        - Cursor의 Open
 *    2. Cursor가 열려 있는 경우,
 *        - Cursor를 Restart
 *
 ***********************************************************************/
#define IDE_FN "qmcDiskSort::openCursor"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcDiskSort::openCursor"));

    if( *aTargetCursor == NULL )
    {
        //-----------------------------------------
        // 1. Cursor가 열려 있지 않은 경우
        //-----------------------------------------
        IDE_TEST( smiSortTempTable::openCursor( 
                aTempTable->tableHandle,
                aFlag,
                aTempTable->updateColumnList,  // Update Column정보
                aKeyRange,
                aKeyFilter,
                aRowFilter,
                aTargetCursor )
            != IDE_SUCCESS );
    }
    else
    {
        //-----------------------------------------
        // 2. Cursor가 열려 있는 경우
        //-----------------------------------------
        IDE_TEST( smiSortTempTable::restartCursor( 
                *aTargetCursor,
                aFlag,
                aKeyRange,
                aKeyFilter,
                aRowFilter )
            != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC
qmcDiskSort::makeInsertSmiValue( qmcdDiskSortTemp * aTempTable )
{
/***********************************************************************
 *
 * Description :
 *    Insert를 위한 Smi Value 구성 정보를 생성한다.
 *
 * Implementation :
 *    Disk Temp Table의 Record를 위하여 생성된 공간에서
 *    각 Column의 Value 정보를 구성한다.
 *    최초 한 번만 구성하며, 이는 Temp Table이 사라질 때까지
 *    이미 할당된 Record 공간은 절대 변하지 않기 때문이다.
 *
 ***********************************************************************/

#define IDE_FN "qmcDiskSort::makeInsertSmiValue"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcDiskSort::makeInsertSmiValue"));

    UInt         i;
    mtcColumn  * sColumn;

    for ( i = 0; i < aTempTable->recordColumnCnt; i++ )
    {
        // PROJ-1597 Temp record size 제약 제거
        // smiValue의 length, value를 storing format으로 바꾼다.

        sColumn = &aTempTable->recordNode->dstTuple->columns[i];

        aTempTable->insertValue[i].value = 
            (SChar*)aTempTable->recordNode->dstTuple->row
            + sColumn->column.offset;

        aTempTable->insertValue[i].length = 
            sColumn->module->actualSize( sColumn,
                                         (SChar*)aTempTable->recordNode->dstTuple->row
                                         + sColumn->column.offset );
    }

    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC
qmcDiskSort::makeRange( qmcdDiskSortTemp * aTempTable )
{
/***********************************************************************
 *
 * Description :
 *    Range 검색을 위한 Key/Row Range를 생성한다.
 *
 * Implementation :
 *    Range 공간이 없는 경우,
 *        - Range 공간의 크기를 계산하고, 메모리를 할당받는다.
 *    Range 생성
 *        - 주어진 Key Range Predicate을 이용하여 Key Range를 생성한다.
 *
 * Modified :
 *    Key Range를 생성할 때 Row Range도 함께 생성한다.
 *    PROJ-1431에서 Temp B-Tree가 sparse index로 구조변경됨에 따라
 *    data page에서 range가 필요함
 ***********************************************************************/

#define IDE_FN "qmcDiskSort::makeRange"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcDiskSort::makeRange"));

    UInt      sKeyColsFlag;
    qtcNode * sKeyFilter;
    UInt      sCompareType;

    // 적합성 검사
    // Range 검색은 한 Column에 대해서만 가능하다.
    IDE_DASSERT( aTempTable->indexColumnCnt == 1 );

    //-----------------------------------
    // Key Range 공간을 할당받음.
    //-----------------------------------
    if ( aTempTable->keyRangeArea == NULL )
    {
        // Range를 위한 공간을 할당받는다.
        // Predicate의 Value는 바뀌어도 Predicate의 종류는 바뀌지 않는다.
        // 따라서, 한 번만 할당하고 계속 사용할 수 있도록 한다.

        //---------------------------
        // Range의 크기 계산
        //---------------------------

        IDE_TEST(
            qmoKeyRange::estimateKeyRange( aTempTable->mTemplate,
                                           aTempTable->rangePredicate,
                                           &aTempTable->keyRangeAreaSize )
            != IDE_SUCCESS );

        //---------------------------
        // Key Range를 위한 공간을 할당한다.
        //---------------------------
        IDU_FIT_POINT( "qmcDiskSort::makeRange::alloc::keyRangeArea",
                        idERR_ABORT_InsufficientMemory );

        IDE_TEST( aTempTable->mTemplate->stmt->qmxMem->cralloc(
                aTempTable->keyRangeAreaSize,
                (void**) & aTempTable->keyRangeArea ) != IDE_SUCCESS);
    }
    else
    {
        // Nothing To Do
    }

    if ( (aTempTable->sortNode->myNode->flag & QMC_MTR_SORT_ORDER_MASK)
         == QMC_MTR_SORT_ASCENDING )
    {
        sKeyColsFlag = SMI_COLUMN_ORDER_ASCENDING;
    }
    else
    {
        sKeyColsFlag = SMI_COLUMN_ORDER_DESCENDING;
    }

    // 적합성 검사
    // 반드시 Key Range생성이 성공해야 함

    //-----------------------------------
    // Key Range를 생성함.
    //-----------------------------------

    // disk temp table의 key range는 MT 타입간의 compare
    sCompareType = MTD_COMPARE_MTDVAL_MTDVAL;

    IDE_TEST( qmoKeyRange::makeKeyRange(
                  aTempTable->mTemplate,
                  aTempTable->rangePredicate,
                  1,
                  aTempTable->sortNode->func.compareColumn,
                  & sKeyColsFlag,
                  sCompareType,
                  aTempTable->keyRangeArea,
                  aTempTable->keyRangeAreaSize,
                  & (aTempTable->keyRange),
                  & sKeyFilter )
              != IDE_SUCCESS );

    // 적합성 검사
    // 반드시 Key Range생성이 성공해야 함
    IDE_DASSERT( sKeyFilter == NULL );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmcDiskSort::makeUpdateSmiValue( qmcdDiskSortTemp * aTempTable )
{
/***********************************************************************
 *
 * Description :
 *    Update Column을 위한 Smi Value 구성 정보를 생성한다.
 *
 * Implementation :
 *    PROJ-1597 Temp record size 제약 제거를 위해
 *    매번 update를 호출하기 전에 이 함수를 불러줘야 한다.
 *    update value들은 storing 형태이기 때문에 매번 조정해줘야 한다.:
 *
 ***********************************************************************/

#define IDE_FN "qmcDiskSort::makeUpdateSmiValue"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcDiskSort::makeUpdateSmiValue"));
    
    smiColumnList * sColumn;
    smiValue      * sValue;
    mtcColumn     * sMtcColumn;

    if( aTempTable->updateValues == NULL )
    {
        // Update Value 정보를 위한 공간을 할당한다.
        IDU_FIT_POINT( "qmcDiskSort::makeUpdateSmiValue::alloc::updateValues",
                        idERR_ABORT_InsufficientMemory );

        IDE_TEST( aTempTable->mTemplate->stmt->qmxMem->alloc(
                ID_SIZEOF(smiValue) * aTempTable->recordColumnCnt,
                (void**) & aTempTable->updateValues ) != IDE_SUCCESS);
    }
    else
    {
        // nothing to do
    }

    for( sColumn = aTempTable->updateColumnList,
             sValue = aTempTable->updateValues;
         sColumn != NULL;
         sColumn = sColumn->next, sValue++ )
    {
        // smiColumn은 mtcColumn내부에 객체가 존재하기 때문에
        // 같은 주소를 가진다고 볼 수 있다.
        sMtcColumn = (mtcColumn*)sColumn->column;

        sValue->value = 
            (SChar*)aTempTable->recordNode->dstTuple->row
            + sColumn->column->offset;

        sValue->length = sMtcColumn->module->actualSize( 
                        sMtcColumn,
                        (SChar*)aTempTable->recordNode->dstTuple->row
                        + sColumn->column->offset );
    }

    // updateColumnList의 개수가 할당 받은 updateValues보다 많은 경우
    // 발생하지 않는 경우
    IDE_DASSERT( (UInt)(sValue-aTempTable->updateValues) <= aTempTable->recordColumnCnt );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


