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
 * $Id: qmcDiskSortTempTable.h 86786 2020-02-27 08:04:12Z donovan.seo $
 *
 * Disk Sort Temp Table
 *
 **********************************************************************/

#ifndef _O_QMC_DISK_SORT_TMEP_TABLE_H_
#define _O_QMC_DISK_SORT_TMEP_TABLE_H_ 1

#include <qc.h>
#include <qmc.h>
#include <qmcTempTableMgr.h>

#define QMCD_DISK_SORT_TEMP_INITIALIZE            (0x00000000)

//---------------------------------------------------
// [Disk Sort Temp Table 객체]
//    초기화 시점에 대한 설명
//    (I) : 최초 한번만 초기화됨
//    (R) : 최초 삽입 및 검색시에 초기화됨
//    (X) : 가변적임
//---------------------------------------------------

typedef struct qmcdDiskSortTemp
{
    UInt             flag;            // (X)
    qcTemplate     * mTemplate;       // (I)

    void           * tableHandle;     // (I)
    void           * indexHandle;     // (I)
    ULong            mTempTableSize;

    // Record 구성을 위한 자료 구조
    qmdMtrNode     * recordNode;      // (I) Record 구성 정보
    smiTableCursor   insertCursor;    // (R) 삽입을 위한 Cursor
    UInt             recordColumnCnt; // (I) Column 개수 + 1(Hit Flag)
    smiColumnList  * insertColumn;    // (I) Insert Column 정보
    smiValue       * insertValue;     // (I) Insert Value 정보
    SChar          * nullRow;         // (I) Disk Temp Table을 위한 Null Row
    scGRID           nullRID;         // (I) SM의 Null Row의 RID

    // Sorting을 위한 자료 구조
    qmdMtrNode     * sortNode;        // (I) Sort할 컬럼 정보
    UInt             indexColumnCnt;  // (I) 인덱스 컬럼의 개수
    smiColumnList  * indexKeyColumnList; // (I) Key내의 인덱스 컬럼 정보
    mtcColumn      * indexKeyColumn;  // (I) Range검색을 위한 인덱스 컬럼 정보

    // 검색을 위한 자료 구조
    smiSortTempCursor* searchCursor;    // (R) 검색 및 Update를 위한 Cursor
    qtcNode        * rangePredicate;    // (X) Key Range Predicate
    // PROJ-1431 : sparse B-Tree temp index로 구조변경
    // leaf page(data page)에서 row를 비교하기 위해 rowRange가 추가됨
    /* PROJ-2201 : Dense구조로 다시 변경
     * Sparse구조는 BufferMiss상황에서 I/O를 훨씬 유발시킴 */
    smiRange       * keyRange;          // (X) Row Range
    smiRange       * keyRangeArea;      // (X) Row Range Area
    UInt             keyRangeAreaSize;  // (X) Row Range Area Size
    // Update를 위한 자료 구조
    smiColumnList  * updateColumnList;  // (R) Update Column 정보
    smiValue       * updateValues;      // (R) Update Value 정보
    
} qmcdDiskSortTemp;


class qmcDiskSort
{
public:

    //------------------------------------------------
    // Disk Sort Temp Table의 관리
    //------------------------------------------------

    // Temp Table을 초기화
    static IDE_RC    init( qmcdDiskSortTemp * aTempTable,
                           qcTemplate       * aTemplate,
                           qmdMtrNode       * aRecordNode,
                           qmdMtrNode       * aSortNode,
                           SDouble            aStoreRowCount,
                           UInt               aMtrRowSize,     // Mtr Record Size
                           UInt               aFlag );

    /* PROJ-2201 정렬키를 변경한다. */
    static IDE_RC    setSortNode( qmcdDiskSortTemp * aTempTable,
                                  const qmdMtrNode * aSortNode );

    // Temp Table내의 모든 Record의 제거
    static IDE_RC    clear( qmcdDiskSortTemp * aTempTable );

    // Temp Table내의 모든 Record의 Flag을 Clear
    static IDE_RC    clearHitFlag( qmcdDiskSortTemp * aTempTable );

    // Record의 삽입
    static IDE_RC    sort( qmcdDiskSortTemp * aTempTable );

    //------------------------------------------------
    // Disk Sort Temp Table의 구성
    //------------------------------------------------

    // Record의 삽입
    static IDE_RC    insert( qmcdDiskSortTemp * aTempTable, void * aRow );

    // 현재 Cursor 위치의 Record의 Hit Flag을 Setting
    static IDE_RC    setHitFlag( qmcdDiskSortTemp * aTempTable );

    // 현재 Cursor 위치의 Record에 Hit Flag가 있는지 판단
    static idBool    isHitFlagged( qmcdDiskSortTemp * aTempTable );

    //------------------------------------------------
    // Disk Sort Temp Table의 검색
    //------------------------------------------------

    //----------------------------
    // 순차 검색
    //----------------------------

    // 첫번째 순차 검색
    static IDE_RC    getFirstSequence( qmcdDiskSortTemp * aTempTable,
                                       void            ** aRow );

    // 다음 순차 검색
    static IDE_RC    getNextSequence( qmcdDiskSortTemp * aTempTable,
                                      void             ** aRow );

    //----------------------------
    // Range 검색
    //----------------------------

    // 첫번째 Range 검색
    static IDE_RC    getFirstRange( qmcdDiskSortTemp * aTempTable,
                                    qtcNode          * aRangePredicate,
                                    void            ** aRow );

    // 다음 Range 검색
    static IDE_RC    getNextRange( qmcdDiskSortTemp * aTempTable,
                                   void            ** aRow );

    //----------------------------
    // Hit 검색
    //----------------------------

    static IDE_RC    getFirstHit( qmcdDiskSortTemp * aTempTable,
                                  void            ** aRow );
    static IDE_RC    getNextHit( qmcdDiskSortTemp * aTempTable,
                                 void            ** aRow );

    //----------------------------
    // Non-Hit 검색
    //----------------------------

    static IDE_RC    getFirstNonHit( qmcdDiskSortTemp * aTempTable,
                                     void            ** aRow );
    static IDE_RC    getNextNonHit( qmcdDiskSortTemp * aTempTable,
                                    void            ** aRow );

    //----------------------------
    // NULL Row 검색
    //----------------------------

    static IDE_RC    getNullRow( qmcdDiskSortTemp * aTempTable,
                                 void            ** aRow );


    //------------------------------------------------
    // Disk Sort Temp Table의 기타 함수
    //------------------------------------------------

    // 현재 Cursor 위치 저장
    static IDE_RC    getCurrPosition( qmcdDiskSortTemp * aTempTable,
                                      smiCursorPosInfo * aCursorInfo );

    // 현재 Cursor 위치의 변경
    static IDE_RC    setCurrPosition( qmcdDiskSortTemp * aTempTable,
                                      smiCursorPosInfo * aCursorInfo );

    // View Scan등에서 접근하기 위한 Information 추출
    static IDE_RC    getCursorInfo( qmcdDiskSortTemp * aTempTable,
                                    void            ** aTableHandle,
                                    void            ** aIndexHandle );

    // Plan Display를 위한 정보 획득
    static IDE_RC    getDisplayInfo( qmcdDiskSortTemp * aTempTable,
                                     ULong            * aPageCount,
                                     SLong            * aRecordCount );

    //------------------------------------------------
    // Window Sort (WNST)를 위한 기능
    //------------------------------------------------

    // update를 수행할 column list를 설정
    // 주의: Disk Sort Temp의 경우 hitFlag와 함께 사용 불가
    static IDE_RC setUpdateColumnList( qmcdDiskSortTemp * aTempTable,
                                       const qmdMtrNode * aUpdateColumns );

    // 현재 검색 중인 위치의 row를 update
    static IDE_RC updateRow( qmcdDiskSortTemp * aTempTable );

private:

    //------------------------------------------------
    // 초기화를 위한 함수
    //------------------------------------------------
    // Temp Table Size의 예측
    static void    tempSizeEstimate( qmcdDiskSortTemp * aTempTable,
                                     SDouble            aStoreRowCount,
                                     UInt               aMtrRowSize );

    // Disk Temp Table의 생성
    static IDE_RC    createTempTable( qmcdDiskSortTemp * aTempTable );

    // Insert Column List를 위한 정보 구성
    static IDE_RC    setInsertColumnList( qmcdDiskSortTemp * aTempTable );

    // Index Key내에서의 Index Column 정보의 구축
    static IDE_RC    makeIndexColumnInfoInKEY( qmcdDiskSortTemp * aTempTable);

    //------------------------------------------------
    // 삽입 및 검색을 위한 함수
    //------------------------------------------------

    static IDE_RC    openCursor( qmcdDiskSortTemp  * aTempTable,
                                 UInt                aFlag,
                                 smiRange          * aKeyRange,
                                 smiRange          * aKeyFilter,
                                 smiCallBack       * aRowFilter,
                                 smiSortTempCursor** aTargetCursor );

        // Insert를 위한 Value 정보 구성
    static IDE_RC    makeInsertSmiValue ( qmcdDiskSortTemp * aTempTable );

    // PROJ-1431 : Key and Row Range를 생성한다.
    static IDE_RC    makeRange( qmcdDiskSortTemp * aTempTable );

    //------------------------------------------------
    // Update 처리를 위한 함수
    //------------------------------------------------

    // Update를 위한 Value 정보 구성
    // Hit Flag의 update와 WNST의 update에서 사용
    static IDE_RC    makeUpdateSmiValue ( qmcdDiskSortTemp * aTempTable );

    //------------------------------------------------
    // Hit Flag 처리를 위한 함수
    //------------------------------------------------

    static void setColumnLengthType( qmcdDiskSortTemp * aTempTable );

};

#endif /* _O_QMC_DISK_SORT_TMEP_TABLE_H_ */
