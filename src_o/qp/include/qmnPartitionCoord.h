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
 * $Id: qmnPartitionCoord.h 20233 2007-02-01 01:58:21Z shsuh $
 *
 * Description :
 *     Partition Coordinator(PCRD) Node
 *
 *     Partitioned table에 대한 scan을 수행하는 Plan Node 이다.
 *
 *     다음과 같은 기능을 위해 사용된다.
 *         - Partition Coordinator
 *
 *     Multi Children(partition에 대한 SCAN) 에 대한 Data를 리턴한다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMN_PARTITION_COORD_H_
#define _O_QMN_PARTITION_COORD_H_ 1

#include <mt.h>
#include <qmc.h>
#include <qmnDef.h>

//-----------------
// Code Node Flags
//-----------------

// qmncPCRD.flag는 qmncSCAN.flag과 공유한다.

//-----------------
// Data Node Flags
//-----------------

// qmndPCRD.flag
// First Initialization Done
#define QMND_PCRD_INIT_DONE_MASK           (0x00000001)
#define QMND_PCRD_INIT_DONE_FALSE          (0x00000000)
#define QMND_PCRD_INIT_DONE_TRUE           (0x00000001)

#define QMND_PCRD_ALL_FALSE_MASK           (0x00000002)
#define QMND_PCRD_ALL_FALSE_FALSE          (0x00000000)
#define QMND_PCRD_ALL_FALSE_TRUE           (0x00000002)

/* qmndPCRD.flag                                     */
// Cursor Status
#define QMND_PCRD_INDEX_CURSOR_MASK        (0x00000004)
#define QMND_PCRD_INDEX_CURSOR_CLOSED      (0x00000000)
#define QMND_PCRD_INDEX_CURSOR_OPEN        (0x00000004)

/* qmndPCRD.flag                                     */
// IN SUBQUERY KEYRANGE의 생성 성공 여부
// IN SUBQUERY KEYRANGE가 존재할 경우에만 유효하며,
// 더 이상 Key Range를 생성할 수 없을 때 변경된다.
#define QMND_PCRD_INSUBQ_RANGE_BUILD_MASK     (0x00000008)
#define QMND_PCRD_INSUBQ_RANGE_BUILD_SUCCESS  (0x00000000)
#define QMND_PCRD_INSUBQ_RANGE_BUILD_FAILURE  (0x00000008)

class qmcInsertCursor;

typedef struct qmncPCRD
{
    //---------------------------------
    // Code 영역 공통 정보
    //---------------------------------

    qmnPlan          plan;
    UInt             flag;
    UInt             planID;

    //---------------------------------
    // 고유 정보
    //---------------------------------

    qmsTableRef    * tableRef;
    UShort           tupleRowID; // Tuple ID

    /* PROJ-2464 hybrid partitioned table 지원 */
    UShort           partitionedTupleID;

    UInt             selectedPartitionCount;
    UInt             partitionCount;
    
    qtcNode        * partitionKeyRange;
    qtcNode        * partitionFilter;
    qtcNode        * nnfFilter;

    // Index
    qcmIndex       * index;                // selected index

    // Index Table의 정보
    UShort           indexTupleRowID;      // index table tuple id
    const void     * indexTableHandle;     // index table handle
    smSCN            indexTableSCN;        // index table SCN

    // Index Table의 Index 정보
    qcmIndex       * indexTableIndex;      // real index
    
    // Key Range
    qtcNode        * fixKeyRange;          // Fixed Key Range
    qtcNode        * fixKeyRange4Print;    // Printable Fixed Key Range
    qtcNode        * varKeyRange;          // Variable Key Range
    qtcNode        * varKeyRange4Filter;   // Variable Fixed Key Range

    // Key Filter
    qtcNode        * fixKeyFilter;         // Fixed Key Filter
    qtcNode        * fixKeyFilter4Print;   // Printable Fixed Key Filter
    qtcNode        * varKeyFilter;         // Variable Key Filter
    qtcNode        * varKeyFilter4Filter;  // Variable Fixed Key Filter

    // Filter
    qtcNode        * constantFilter;    // Constant Filter
    qtcNode        * filter;            // Filter
    qtcNode        * lobFilter;         // Lob Filter ( BUG-25916 ) 
    qtcNode        * subqueryFilter;    // Subquery Filter
   
    /* PROJ-2705 memory parittion simple query */
    idBool           mIsSimple;          // simple index scan or simple full scan
    qmnValueInfo     mSimpleValues;
    UInt             mSimpleCompareOpCount;
    
    const void     * mPrePruningPartHandle;  // BUG-48800 partition Table Handle
    smSCN            mPrePruningPartSCN;     // BUG-48800 partition Table SCN

    //---------------------------------
    // Display 관련 정보
    //---------------------------------

    qmsNamePosition  tableOwnerName;     // Table Owner Name
    qmsNamePosition  tableName;          // Table Name
    qmsNamePosition  aliasName;          // Alias Name

    const void     * table;               // Table Handle
    smSCN            tableSCN;            // Table SCN

    //PROJ-2249
    qmnRangeSortedChildren * rangeSortedChildrenArray;
} qmncPCRD;

typedef struct qmndPCRD
{
    //---------------------------------
    // Data 영역 공통 정보
    //---------------------------------

    qmndPlan                plan;
    doItFunc                doIt;
    UInt                  * flag;

    //---------------------------------
    // Partition SCAN 고유 정보
    //---------------------------------

    smiRange              * partitionFilterArea;
    smiRange              * partitionFilter;
    UInt                    partitionFilterSize;
    
    idBool                  isRowMovementUpdate;

    //---------------------------------
    // Disk Table 관련 정보
    //---------------------------------

    void                  * nullRow;  // Disk Table을 위한 null row
    scGRID                  nullRID;

    /* PROJ-2464 hybrid partitioned table 지원 */
    void                  * diskRow;

    qmnChildren          ** childrenArea;
    UInt                    selectedChildrenCount;
    UInt                    curChildNo;

    // PROJ-1705
    // 1. update, delete의 경우 trigger row가 필요여부 정보 
    //   이 정보는 cusor open시 fetch column정보구성에 쓰이며,
    //   trigger row가 필요한 경우 ( 테이블 전체 컬럼 패치 )
    //   trigger row가 필요하지 않은 경우 ( validation시 수집한 컬럼에 대해서 패치 )
    // 2. partitioned table의 update시
    //    rowMovement인 경우 테이블 전체 컬럼 패치 
    idBool                  isNeedAllFetchColumn;    

    //---------------------------------
    // Index Table Scan을 위한 정보
    //---------------------------------

    mtcTuple              * indexTuple;
    
    // fix BUG-9052
    // subquery filter가 outer column 참조시
    // outer column을 참조한 store and search를
    // 재수행하도록 하기 위해서 ... qmnSCAN::readRow()함수주석참조
    // printPlan()내에서 ACCESS count display시
    // DataPlan->plan.myTuple->modify에서 이 값을 빼주도록 한다.
    UInt                    subQFilterDepCnt;

    smiTableCursor          cursor;          // Cursor
    smiCursorProperties     cursorProperty;  // Cursor 관련 정보
    smiCursorType           cursorType;      // PROJ-1502 PARTITIONED DISK TABLE
    UInt                    lockMode;        // Lock Mode

    //---------------------------------
    // Update를 위한 Column List
    //---------------------------------

    smiColumnList         * updateColumnList;   // Update가 아닌 경우, NULL로 설정
    idBool                  inplaceUpdate;      // inplace update

    //---------------------------------
    // Predicate 종류
    //---------------------------------

    smiRange              * fixKeyRangeArea; // Fixed Key Range 영역
    smiRange              * fixKeyRange;     // Fixed Key Range
    UInt                    fixKeyRangeSize; // Fixed Key Range 크기

    smiRange              * fixKeyFilterArea; //Fixed Key Filter 영역
    smiRange              * fixKeyFilter;    // Fixed Key Filter
    UInt                    fixKeyFilterSize;// Fixed Key Filter 크기

    smiRange              * varKeyRangeArea; // Variable Key Range 영역
    smiRange              * varKeyRange;     // Variable Key Range
    UInt                    varKeyRangeSize; // Variable Key Range 크기

    smiRange              * varKeyFilterArea; //Variable Key Filter 영역
    smiRange              * varKeyFilter;    // Variable Key Filter
    UInt                    varKeyFilterSize;// Variable Key Filter 크기

    smiRange              * notNullKeyRange; // Not Null Key Range
    
    smiRange              * keyRange;        // 최종 Key Range
    smiRange              * keyFilter;       // 최종 Key Filter

    // Filter 관련 CallBack 정보
    smiCallBack             callBack;        // Filter CallBack
    qtcSmiCallBackDataAnd   callBackDataAnd; //
    qtcSmiCallBackData      callBackData[3]; // 세 종류의 Filter가 가능함.
    
    //---------------------------------
    // Merge Join 관련 정보
    //---------------------------------

    smiCursorPosInfo        cursorInfo;

    //---------------------------------
    // Child Index 관련 정보
    //---------------------------------
    
    qmnChildrenIndex      * childrenIndex;

    // PROJ-2249 range partition filter intersect count
    UInt                  * rangeIntersectCountArray;
    
} qmndPCRD;

class qmnPCRD
{
public:

    //------------------------
    // Base Function Pointer
    //------------------------

    // 초기화
    static IDE_RC init( qcTemplate * aTemplate,
                        qmnPlan    * aPlan );

    // 수행 함수
    static IDE_RC doIt( qcTemplate * aTemplate,
                        qmnPlan    * aPlan,
                        qmcRowFlag * aFlag );

    // Null Padding
    static IDE_RC padNull( qcTemplate * aTemplate,
                           qmnPlan    * aPlan );

    // Plan 정보 출력
    static IDE_RC printPlan( qcTemplate   * aTemplate,
                             qmnPlan      * aPlan,
                             ULong          aDepth,
                             iduVarString * aString,
                             qmnDisplay     aMode );

    //------------------------
    // mapping by doIt() function pointer
    //------------------------

    // 기본수행 함수.
    // TODO1502:
    // doItFirst, doItNext로 쪼개져야 함.

    static IDE_RC doItFirst( qcTemplate * aTemplate,
                             qmnPlan    * aPlan,
                             qmcRowFlag * aFlag );

    static IDE_RC doItNext( qcTemplate * aTemplate,
                            qmnPlan    * aPlan,
                            qmcRowFlag * aFlag );

    // 호출되어서는 안됨
    static IDE_RC doItDefault( qcTemplate * aTemplate,
                               qmnPlan    * aPlan,
                               qmcRowFlag * aFlag );

    // 항상 조건을 만족할 수 없는 경우
    static IDE_RC doItAllFalse( qcTemplate * aTemplate,
                                qmnPlan    * aPlan,
                                qmcRowFlag * aFlag );

    //------------------------
    // Direct External Call
    //------------------------

    // Cursor를 저장함
    static IDE_RC storeCursor( qcTemplate * aTemplate,
                               qmnPlan    * aPlan );

    // 저장한 Cursor 위치로 복원함
    static IDE_RC restoreCursor( qcTemplate * aTemplate,
                                 qmnPlan    * aPlan );

    // khshim moved this function from private section to public.
    // for Referential integrity check upon delete/update
    // Key Range, Key Filter, Filter 생성
    static IDE_RC makeKeyRangeAndFilter( qcTemplate * aTemplate,
                                         qmncPCRD   * aCodePlan,
                                         qmndPCRD   * aDataPlan );

private:

    //------------------------
    // 초기화 관련 함수
    //------------------------
    
    // 최초 초기화
    static IDE_RC firstInit( qcTemplate * aTemplate,
                             qmncPCRD   * aCodePlan,
                             qmndPCRD   * aDataPlan );

    // partition filter를 위한 공간 할당
    static IDE_RC allocPartitionFilter( qcTemplate * aTemplate,
                                        qmncPCRD   * aCodePlan,
                                        qmndPCRD   * aDataPlan );

    // Fixed Key Range를 위한 Range 공간 할당
    static IDE_RC allocFixKeyRange( qcTemplate * aTemplate,
                                    qmncPCRD   * aCodePlan,
                                    qmndPCRD   * aDataPlan );

    // Fixed Key Filter를 위한 Range 공간 할당
    static IDE_RC allocFixKeyFilter( qcTemplate * aTemplate,
                                     qmncPCRD   * aCodePlan,
                                     qmndPCRD   * aDataPlan );

    // Variable Key Range를 위한 Range 공간 할당
    static IDE_RC allocVarKeyRange( qcTemplate * aTemplate,
                                    qmncPCRD   * aCodePlan,
                                    qmndPCRD   * aDataPlan );

    // Variable Key Filter를 위한 Range 공간 할당
    static IDE_RC allocVarKeyFilter( qcTemplate * aTemplate,
                                     qmncPCRD   * aCodePlan,
                                     qmndPCRD   * aDataPlan );

    // Not Null Key Range를 위한 Range 공간 할당
    static IDE_RC allocNotNullKeyRange( qcTemplate * aTemplate,
                                        qmncPCRD   * aCodePlan,
                                        qmndPCRD   * aDataPlan );
    
    //------------------------
    // Plan Display 관련 함수
    //------------------------
    
    // Predicate의 상세 정보를 출력한다.
    static IDE_RC printPredicateInfo( qcTemplate   * aTemplate,
                                      qmncPCRD     * aCodePlan,
                                      ULong          aDepth,
                                      iduVarString * aString );

    // Partition Key Range의 상세 정보를 출력
    static IDE_RC printPartKeyRangeInfo( qcTemplate   * aTemplate,
                                         qmncPCRD     * aCodePlan,
                                         ULong          aDepth,
                                         iduVarString * aString );

    // Partition Filter의 상세 정보를 출력
    static IDE_RC printPartFilterInfo( qcTemplate   * aTemplate,
                                       qmncPCRD     * aCodePlan,
                                       ULong          aDepth,
                                       iduVarString * aString );

    // Key Range의 상세 정보를 출력
    static IDE_RC printKeyRangeInfo( qcTemplate   * aTemplate,
                                     qmncPCRD     * aCodePlan,
                                     ULong          aDepth,
                                     iduVarString * aString );

    // Key Filter의 상세 정보를 출력
    static IDE_RC printKeyFilterInfo( qcTemplate   * aTemplate,
                                      qmncPCRD     * aCodePlan,
                                      ULong          aDepth,
                                      iduVarString * aString );

    // Filter의 상세 정보를 출력
    static IDE_RC printFilterInfo( qcTemplate   * aTemplate,
                                   qmncPCRD     * aCodePlan,
                                   ULong          aDepth,
                                   iduVarString * aString );

    static IDE_RC printAccessInfo( qmndPCRD     * aDataPlan,
                                   iduVarString * aString,
                                   qmnDisplay     aMode );

    //------------------------
    // PCRD의 doIt() 관련 함수
    //------------------------
    
    static IDE_RC readRow( qcTemplate * aTemplate,
                           qmncPCRD   * aCodePlan,
                           qmndPCRD   * aDataPlan,
                           qmcRowFlag * aFlag );

    static IDE_RC makePartitionFilter( qcTemplate * aTemplate,
                                       qmncPCRD   * aCodePlan,
                                       qmndPCRD   * aDataPlan );

    static IDE_RC partitionFiltering( qcTemplate * aTemplate,
                                      qmncPCRD   * aCodePlan,
                                      qmndPCRD   * aDataPlan );

    static IDE_RC makeChildrenArea( qcTemplate * aTemplate,
                                    qmnPlan    * aPlan );
    
    static IDE_RC makeChildrenIndex( qmndPCRD   * aDataPlan );
    
    // index cursor로 partition row를 읽는다.
    static IDE_RC readRowWithIndex( qcTemplate * aTemplate,
                                    qmncPCRD   * aCodePlan,
                                    qmndPCRD   * aDataPlan,
                                    qmcRowFlag * aFlag );

    // index cursor로 index row를 읽는다.
    static IDE_RC readIndexRow( qcTemplate * aTemplate,
                                qmncPCRD   * aCodePlan,
                                qmndPCRD   * aDataPlan,
                                void      ** aIndexRow );
    
    // IN Subquery KeyRange를 위한 재시도
    static IDE_RC reReadIndexRow4InSubRange( qcTemplate * aTemplate,
                                             qmncPCRD   * aCodePlan,
                                             qmndPCRD   * aDataPlan,
                                             void      ** aIndexRow );
    
    // index row로 partition row를 읽는다.
    static IDE_RC readRowWithIndexRow( qcTemplate * aTemplate,
                                       qmndPCRD   * aDataPlan,
                                       qmcRowFlag * aFlag );
    
    //------------------------
    // Cursor 관련 함수
    //------------------------

    // Cursor의 Open
    static IDE_RC openIndexCursor( qcTemplate * aTemplate,
                                   qmncPCRD   * aCodePlan,
                                   qmndPCRD   * aDataPlan );

    // Cursor의 Restart
    static IDE_RC restartIndexCursor( qmncPCRD   * aCodePlan,
                                      qmndPCRD   * aDataPlan );
};

#endif /* _O_QMN_PARTITION_COORD_H_ */
