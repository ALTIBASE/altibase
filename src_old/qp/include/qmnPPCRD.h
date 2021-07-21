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
 * $Id: qmnPPCRD.h 90438 2021-04-02 08:20:57Z ahra.cho $
 *
 * Description :
 *     Parallel Partition Coordinator(PPCRD) Node
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

#ifndef _O_QMN_PPCRD_H_
#define _O_QMN_PPCRD_H_ 1

#include <mt.h>
#include <qmc.h>
#include <qmnDef.h>

//-----------------
// Code Node Flags
//-----------------

// qmncPPCRD.flag는 qmncSCAN.flag과 공유한다.

//-----------------
// Data Node Flags
//-----------------

// qmndPPCRD.flag
// First Initialization Done
#define QMND_PPCRD_INIT_DONE_MASK           (0x00000001)
#define QMND_PPCRD_INIT_DONE_FALSE          (0x00000000)
#define QMND_PPCRD_INIT_DONE_TRUE           (0x00000001)

#define QMND_PPCRD_ALL_FALSE_MASK           (0x00000002)
#define QMND_PPCRD_ALL_FALSE_FALSE          (0x00000000)
#define QMND_PPCRD_ALL_FALSE_TRUE           (0x00000002)

/* qmndPPCRD.flag                                     */
// Cursor Status
#define QMND_PPCRD_INDEX_CURSOR_MASK        (0x00000004)
#define QMND_PPCRD_INDEX_CURSOR_CLOSED      (0x00000000)
#define QMND_PPCRD_INDEX_CURSOR_OPEN        (0x00000004)

/* qmndPPCRD.flag                                     */
// IN SUBQUERY KEYRANGE의 생성 성공 여부
// IN SUBQUERY KEYRANGE가 존재할 경우에만 유효하며,
// 더 이상 Key Range를 생성할 수 없을 때 변경된다.
#define QMND_PPCRD_INSUBQ_RANGE_BUILD_MASK     (0x00000008)
#define QMND_PPCRD_INSUBQ_RANGE_BUILD_SUCCESS  (0x00000000)
#define QMND_PPCRD_INSUBQ_RANGE_BUILD_FAILURE  (0x00000008)

/*
 * BUG-38294
 * mapping info: which SCAN run by which PRLQ
 */
typedef struct qmnPRLQChildMap
{
    qmnPlan* mSCAN;
    qmnPlan* mPRLQ;
} qmnPRLQChildMap;

typedef struct qmncPPCRD
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
    UInt             PRLQCount;
    
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
    
    // Filter
    qtcNode        * constantFilter;    // Constant Filter 
    qtcNode        * subqueryFilter;    // Subquery Filter
    
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
} qmncPPCRD;

typedef struct qmndPPCRD
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

    qmnChildren          ** childrenSCANArea;
    qmnChildren           * childrenPRLQArea;
    UInt                    selectedChildrenCount;
    UInt                    lastChildNo;
    qmnChildren           * curPRLQ;
    qmnChildren           * prevPRLQ;

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

    /* BUG-38294 */
    UInt                    mSCANToPRLQMapCount; 
    qmnPRLQChildMap       * mSCANToPRLQMap;
} qmndPPCRD;

class qmnPPCRD
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

private:

    //------------------------
    // mapping by doIt() function pointer
    //------------------------

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
    // 초기화 관련 함수
    //------------------------
    
    // 최초 초기화
    static IDE_RC firstInit( qcTemplate * aTemplate,
                             qmncPPCRD  * aCodePlan,
                             qmndPPCRD  * aDataPlan );

    // partition filter를 위한 공간 할당
    static IDE_RC allocPartitionFilter( qcTemplate * aTemplate,
                                        qmncPPCRD  * aCodePlan,
                                        qmndPPCRD  * aDataPlan );

    
    //------------------------
    // Plan Display 관련 함수
    //------------------------
    
    // Predicate의 상세 정보를 출력한다.
    static IDE_RC printPredicateInfo( qcTemplate   * aTemplate,
                                      qmncPPCRD    * aCodePlan,
                                      ULong          aDepth,
                                      iduVarString * aString );

    // Partition Key Range의 상세 정보를 출력
    static IDE_RC printPartKeyRangeInfo( qcTemplate   * aTemplate,
                                         qmncPPCRD    * aCodePlan,
                                         ULong          aDepth,
                                         iduVarString * aString );

    // Partition Filter의 상세 정보를 출력
    static IDE_RC printPartFilterInfo( qcTemplate   * aTemplate,
                                       qmncPPCRD    * aCodePlan,
                                       ULong          aDepth,
                                       iduVarString * aString );

    // Filter의 상세 정보를 출력
    static IDE_RC printFilterInfo( qcTemplate   * aTemplate,
                                   qmncPPCRD    * aCodePlan,
                                   ULong          aDepth,
                                   iduVarString * aString );

    static IDE_RC printAccessInfo( qmndPPCRD    * aDataPlan,
                                   iduVarString * aString,
                                   qmnDisplay     aMode );

    static IDE_RC makePartitionFilter( qcTemplate * aTemplate,
                                       qmncPPCRD  * aCodePlan,
                                       qmndPPCRD  * aDataPlan );

    static IDE_RC partitionFiltering( qcTemplate * aTemplate,
                                      qmncPPCRD  * aCodePlan,
                                      qmndPPCRD  * aDataPlan );

    static void makeChildrenSCANArea( qcTemplate * aTemplate,
                                      qmnPlan    * aPlan );
    
    static void makeChildrenPRLQArea( qcTemplate * aTemplate,
                                      qmnPlan    * aPlan );
};

#endif /* _O_QMN_PPCRD_H_ */
