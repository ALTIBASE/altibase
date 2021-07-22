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
 * $Id: qmcSortTempTable.h 86786 2020-02-27 08:04:12Z donovan.seo $
 *
 * Description :
 *   [Sort Temp Table]
 *
 *   A4에서는 Memory 및 Disk Table을 모두 지원한다.
 *   중간 결과를 저장하는 Node들 중 Sort를 이용한 중간 결과를 만드는
 *   Materialized Node들은 Memory Sort Temp Table 또는 Disk Sort Temp
 *   Table을 사용하게 된다.
 *
 *   이 때, 각 노드가 Memory/Disk Sort Temp Table에 따라 서로 다른
 *   동작을 하지 않도록 하기 위하여 다음과 같은 Sort Temp Table을
 *   이용하여 Transparent한 구현 및 동작이 가능하도록 한다.
 *
 *   이러한 개념을 나타낸 그림은 아래와 같다.
 *
 *                                                     -------------
 *                                                  -->[Memory Sort]
 *     ---------------     -------------------      |  -------------
 *     | Plan Node   |---->| Sort Temp Table |------|  -----------
 *     ---------------     -------------------      -->[Disk Sort]
 *                                                     -----------
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMC_SORT_TMEP_TABLE_H_
#define _O_QMC_SORT_TMEP_TABLE_H_ 1

#include <qmcMemSortTempTable.h>
#include <qmcDiskSortTempTable.h>

/* qmcdSortTempTable.flag                                  */
#define QMCD_SORT_TMP_INITIALIZE                 (0x00000000)

/* qmcdSortTempTable.flag                                  */
// Sort Temp Table의 저장 매체
#define QMCD_SORT_TMP_STORAGE_TYPE               (0x00000004)
#define QMCD_SORT_TMP_STORAGE_MEMORY             (0x00000000)
#define QMCD_SORT_TMP_STORAGE_DISK               (0x00000004)

/* PROJ-2201 Innovation in sorting and hashing(temp)
 * QMNC_SORT_SEARCH_MASK 를 계승해 Flag를 설정함 */
/* qmcdSortTempTable.flag                                  */
// Sort Temp Table의 탐색 방법
#define QMCD_SORT_TMP_SEARCH_MASK               (0x00000008)
#define QMCD_SORT_TMP_SEARCH_SEQUENTIAL         (0x00000000)
#define QMCD_SORT_TMP_SEARCH_RANGE              (0x00000008)

// Sort Temp Table 자료 구조
typedef struct qmcdSortTemp
{
    UInt               flag;

    qcTemplate       * mTemplate;
    qmdMtrNode       * recordNode;  // Record 구성 정보
    qmdMtrNode       * sortNode;    // Sort할 Column 정보

    UInt               mtrRowSize;  // 저장 Record의 크기
    UInt               nullRowSize; // 저장 null record의 크기
    SLong              recordCnt;   // 저장된 Record 개수
    qmcMemory        * memoryMgr;   // 저장 Record를 위한 메모리 관리자
    iduMemory        * memory;
    UInt               memoryIdx;

    SChar            * nullRow;     // Memory Temp Table을 위한 Null Row

    // Memory Temp Table을 위한 정보
    smiRange         * range;       // Range 검색을 위한 Key Range
    smiRange         * rangeArea;   // Range 검색을 위한 Key Range Area
    UInt               rangeAreaSize; // Range 검색을 위한 Key Range Area Size

    qmcdMemSortTemp  * memoryTemp;  // Memory Sort Temp Table
    qmcdDiskSortTemp * diskTemp;    // Disk Sort Temp Table

    idBool             existTempType;  // TEMP_TYPE 컬럼이 존재하는지
    void             * insertRow;   // insert를 위한 임시 버퍼
    
} qmcdSortTemp;

class qmcSortTemp
{
public:

    //------------------------------------------------
    // Sort Temp Table의 관리
    //------------------------------------------------

    // Sort Temp Table을 초기화한다.
    static IDE_RC init( qmcdSortTemp * aTempTable,
                        qcTemplate   * aTemplate,
                        UInt           aMemoryIdx,
                        qmdMtrNode   * aRecordNode,
                        qmdMtrNode   * aSortNode,
                        SDouble        aStoreRowCount,
                        UInt           aFlag );

    // Sort Temp Table의 Data를 제거한다.
    static IDE_RC clear( qmcdSortTemp * aTempTable );

    // Sort Temp Table내의 모든 Data의 flag을 초기화한다.
    static IDE_RC clearHitFlag( qmcdSortTemp * aTempTable );

    //------------------------------------------------
    // Sort Temp Table의 구성
    //------------------------------------------------

    // Data를 위한 Memory 할당
    static IDE_RC alloc( qmcdSortTemp  * aTempTable,
                         void         ** aRow );

    // Data를 삽입
    static IDE_RC addRow( qmcdSortTemp * aTempTable,
                          void         * aRow );

    // insert row 생성
    static IDE_RC makeTempTypeRow( qmcdSortTemp  * aTempTable,
                                   void          * aRow,
                                   void         ** aExtRow );
    
    // 저장된 Data들의 정렬을 수행
    static IDE_RC sort( qmcdSortTemp * aTempTable );

    // Limit Sorting을 수행함
    static IDE_RC shiftAndAppend( qmcdSortTemp * aTempTable,
                                  void         * aRow,
                                  void        ** aReturnRow );

    // Min-Max 저장을 위한 Limit Sorting을 수행함
    static IDE_RC changeMinMax( qmcdSortTemp * aTempTable,
                                void         * aRow,
                                void        ** aReturnRow );

    //------------------------------------------------
    // Sort Temp Table의 검색
    //------------------------------------------------

    //-------------------------
    // 순차 검색
    //-------------------------

    // 첫번째 순차 검색 (ORDER BY, Two Pass Sort Join의 Left에서 사용)
    static IDE_RC getFirstSequence( qmcdSortTemp * aTempTable,
                                    void        ** aRow );

    // 두번째 순차 검색
    static IDE_RC getNextSequence( qmcdSortTemp * aTempTable,
                                   void        ** aRow );

    //-------------------------
    // Range 검색
    //-------------------------

    // 첫번째 Range 검색 (Sort Join의 Right에서 사용)
    static IDE_RC getFirstRange( qmcdSortTemp * aTempTable,
                                 qtcNode      * aRangePredicate,
                                 void        ** aRow );

    // 두번째 Range 검색
    static IDE_RC getNextRange( qmcdSortTemp * aTempTable,
                                void        ** aRow );

    //-------------------------
    // Hit 검색
    //-------------------------

    // 첫번째 Hit Record 검색
    static IDE_RC getFirstHit( qmcdSortTemp * aTempTable,
                               void        ** aRow );

    // 두번째 Hit Record 검색
    static IDE_RC getNextHit( qmcdSortTemp * aTempTable,
                              void        ** aRow );

    //-------------------------
    // Non-Hit 검색
    //-------------------------

    // 첫번째 Non Hit Record 검색
    static IDE_RC getFirstNonHit( qmcdSortTemp * aTempTable,
                                  void        ** aRow );

    // 두번째 Non Hit Record 검색
    static IDE_RC getNextNonHit( qmcdSortTemp * aTempTable,
                                 void        ** aRow );

    // Null Row 검색 (저장된 Null Row 획득)
    static IDE_RC getNullRow( qmcdSortTemp * aTempTable,
                              void        ** aRow );

    //------------------------------------------------
    // Sort Temp Table의 기타 함수
    //------------------------------------------------

    // 검색된 Record에 Hit Flag 설정
    static IDE_RC setHitFlag( qmcdSortTemp * aTempTable );

    // 검색된 Record의 Hit Flag가 설정되어 있는지 확인
    static idBool isHitFlagged( qmcdSortTemp * aTempTable );

    // 현재 Cursor 위치를 저장 (Merge Join에서 사용)
    static IDE_RC storeCursor( qmcdSortTemp     * aTempTable,
                               smiCursorPosInfo * aCursorInfo );

    // 지정된 Cursor 위치로 복원 (Merge Join에서 사용)
    static IDE_RC restoreCursor( qmcdSortTemp     * aTempTable,
                                 smiCursorPosInfo * aCursorInfo );

    // View Scan등에서 접근하기 위한 Information 추출
    static IDE_RC getCursorInfo( qmcdSortTemp     * aTempTable,
                                 void            ** aTableHandle,
                                 void            ** aIndexHandle,
                                 qmcdMemSortTemp ** aMemSortTemp,
                                 qmdMtrNode      ** aMemSortRecord );

    // Plan Display를 위한 정보 획득
    static IDE_RC getDisplayInfo( qmcdSortTemp * aTempTable,
                                  ULong        * aDiskPageCount,
                                  SLong        * aRecordCount );

    //------------------------------------------------
    // Window Sort (WNST)를 위한 기능
    //------------------------------------------------
    
    // 초기화되고, 데이터가 입력된 Temp Table의 정렬키를 변경
    // (Window Sort에서 사용)
    static IDE_RC setSortNode( qmcdSortTemp     * aTempTable,
                               const qmdMtrNode * aSortNode );

    // update를 수행할 column list를 설정
    // 주의: Disk Sort Temp의 경우 hitFlag와 함께 사용 불가
    static IDE_RC setUpdateColumnList( qmcdSortTemp     * aTempTable,
                                       const qmdMtrNode * aUpdateColumns );

    // 현재 검색 중인 위치의 row를 update
    static IDE_RC updateRow( qmcdSortTemp * aTempTable );

    /* PROJ-1715 Get Last Key */
    static IDE_RC getLastKey( qmcdSortTemp * aTempTable,
                              SLong        * aLastKey );
    /* PROJ-1715 Set Last Key */
    static IDE_RC setLastKey( qmcdSortTemp * aTemptable,
                              SLong          aLastKey );

private:

    //------------------------------------------------
    // 초기화를 위한 함수
    //------------------------------------------------

    // Memory Temp Table을 위한 NULL ROW 생성
    static IDE_RC makeMemNullRow( qmcdSortTemp * aTempTable );

    //------------------------------------------------
    // Range 검색을 위한 함수
    //------------------------------------------------

    // Memory Temp Table을 위한 Key Range 생성
    static IDE_RC makeMemKeyRange( qmcdSortTemp * aTempTable,
                                   qtcNode      * aRangePredicate );

};

#endif /* _O_QMC_SORT_TMEP_TABLE_H_ */
