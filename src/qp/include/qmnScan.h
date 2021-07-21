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
 * $Id: qmnScan.h 91237 2021-07-16 04:28:37Z donovan.seo $
 *
 * Description :
 *     SCAN Node
 *     관계형 모델에서 selection을 수행하는 Plan Node 이다.
 *     Storage Manager에 직접 접근하여 selection을 수행한다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMN_SCAN_H_
#define _O_QMN_SCAN_H_ 1

#include <mt.h>
#include <qmc.h>
#include <qmnDef.h>
#include <qmx.h>

//-----------------
// Code Node Flags
//-----------------

/* qmncSCAN.flag                                     */
// Iterator Traverse Direction
// 주의 : smiDef.h 에 있는 값을 사용하는 것은 Buggy함.
#define QMNC_SCAN_TRAVERSE_MASK            (0x00000001)
#define QMNC_SCAN_TRAVERSE_FORWARD         (0x00000000)
#define QMNC_SCAN_TRAVERSE_BACKWARD        (0x00000001)

/* qmncSCAN.flag                                     */
// IN Subquery가 Key Range로 사용되고 있는지의 여부
#define QMNC_SCAN_INSUBQ_KEYRANGE_MASK     (0x00000002)
#define QMNC_SCAN_INSUBQ_KEYRANGE_FALSE    (0x00000000)
#define QMNC_SCAN_INSUBQ_KEYRANGE_TRUE     (0x00000002)

/* qmncSCAN.flag                                     */
// Foreign Key 검사를 위해 Previous 사용 여부
#define QMNC_SCAN_PREVIOUS_ENABLE_MASK     (0x00000004)
#define QMNC_SCAN_PREVIOUS_ENABLE_FALSE    (0x00000000)
#define QMNC_SCAN_PREVIOUS_ENABLE_TRUE     (0x00000004)

/* qmncSCAN.flag                                     */
// 참조되는 table이 fixed table 또는 performance view인지의 정보를 나타냄.
// fixed table 또는 performance view에 대해서는
// table에 대한 IS LOCK을 잡지 않도록 하기 위함.
#define QMNC_SCAN_TABLE_FV_MASK            (0x00000008)
#define QMNC_SCAN_TABLE_FV_FALSE           (0x00000000)
#define QMNC_SCAN_TABLE_FV_TRUE            (0x00000008)

// Proj-1360 Queue
/* qmncSCAN.flag                                     */
// 참조되는 table queue 인경우, dequeue 후에 레코드 삭제를 나타냄
#define QMNC_SCAN_TABLE_QUEUE_MASK         (0x00000010)
#define QMNC_SCAN_TABLE_QUEUE_FALSE        (0x00000000)
#define QMNC_SCAN_TABLE_QUEUE_TRUE         (0x00000010)

// PROJ-1502 PARTITIONED DISK TABLE
/* qmncSCAN.flag                                     */
// partition에 대한 scan인지 여부를
#define QMNC_SCAN_FOR_PARTITION_MASK       (0x00000020)
#define QMNC_SCAN_FOR_PARTITION_FALSE      (0x00000000)
#define QMNC_SCAN_FOR_PARTITION_TRUE       (0x00000020)

// To fix BUG-12742
// index를 반드시 사용해야 하는지 여부를 나타낸다.
#define QMNC_SCAN_FORCE_INDEX_SCAN_MASK    (0x00000040)
#define QMNC_SCAN_FORCE_INDEX_SCAN_FALSE   (0x00000000)
#define QMNC_SCAN_FORCE_INDEX_SCAN_TRUE    (0x00000040)

// Indexable MIN-MAX 최적화가 적용된 경우
#define QMNC_SCAN_NOTNULL_RANGE_MASK       (0x00000080)
#define QMNC_SCAN_NOTNULL_RANGE_FALSE      (0x00000000)
#define QMNC_SCAN_NOTNULL_RANGE_TRUE       (0x00000080)

// PROJ-1624 global non-partitioned index
// index table scan 여부를 나타낸다.
#define QMNC_SCAN_INDEX_TABLE_SCAN_MASK    (0x00000200)
#define QMNC_SCAN_INDEX_TABLE_SCAN_FALSE   (0x00000000)
#define QMNC_SCAN_INDEX_TABLE_SCAN_TRUE    (0x00000200)

// PROJ-1624 global non-partitioned index
// rid scan을 반드시 사용해야 하는지 여부를 나타낸다.
#define QMNC_SCAN_FORCE_RID_SCAN_MASK      (0x00000400)
#define QMNC_SCAN_FORCE_RID_SCAN_FALSE     (0x00000000)
#define QMNC_SCAN_FORCE_RID_SCAN_TRUE      (0x00000400)

/* PROJ-1832 New database link */
#define QMNC_SCAN_REMOTE_TABLE_MASK        (0x00000800)
#define QMNC_SCAN_REMOTE_TABLE_FALSE       (0x00000000)
#define QMNC_SCAN_REMOTE_TABLE_TRUE        (0x00000800)

/* BUG-37077 REMOTE_TABLE_STORE */
#define QMNC_SCAN_REMOTE_TABLE_STORE_MASK  (0x00001000)
#define QMNC_SCAN_REMOTE_TABLE_STORE_FALSE (0x00000000)
#define QMNC_SCAN_REMOTE_TABLE_STORE_TRUE  (0x00001000)

/* session temporary table */
#define QMNC_SCAN_TEMPORARY_TABLE_MASK     (0x00002000)
#define QMNC_SCAN_TEMPORARY_TABLE_FALSE    (0x00000000)
#define QMNC_SCAN_TEMPORARY_TABLE_TRUE     (0x00002000)

/* BUG-42639 Mornitaring query */
#define QMNC_SCAN_FAST_SELECT_FIXED_TABLE_MASK  (0x00004000)
#define QMNC_SCAN_FAST_SELECT_FIXED_TABLE_FALSE (0x00000000)
#define QMNC_SCAN_FAST_SELECT_FIXED_TABLE_TRUE  (0x00004000)

/* BUG-49127 */
#define QMNC_SCAN_MOVE_AND_DELETE_MASK  (0x00008000)
#define QMNC_SCAN_MOVE_AND_DELETE_FALSE (0x00000000)
#define QMNC_SCAN_MOVE_AND_DELETE_TRUE  (0x00008000)

//-----------------
// Data Node Flags
//-----------------

/* qmndSCAN.flag                                     */
// First Initialization Done
#define QMND_SCAN_INIT_DONE_MASK           (0x00000001)
#define QMND_SCAN_INIT_DONE_FALSE          (0x00000000)
#define QMND_SCAN_INIT_DONE_TRUE           (0x00000001)

/* qmndSCAN.flag                                     */
// Cursor Status
#define QMND_SCAN_CURSOR_MASK              (0x00000002)
#define QMND_SCAN_CURSOR_CLOSED            (0x00000000)
#define QMND_SCAN_CURSOR_OPEN              (0x00000002)

/* qmndSCAN.flag                                     */
// Constant Filter의 수행 결과
#define QMND_SCAN_ALL_FALSE_MASK           (0x00000004)
#define QMND_SCAN_ALL_FALSE_FALSE          (0x00000000)
#define QMND_SCAN_ALL_FALSE_TRUE           (0x00000004)

/* qmndSCAN.flag                                     */
// IN SUBQUERY KEYRANGE의 생성 성공 여부
// IN SUBQUERY KEYRANGE가 존재할 경우에만 유효하며,
// 더 이상 Key Range를 생성할 수 없을 때 변경된다.
#define QMND_SCAN_INSUBQ_RANGE_BUILD_MASK     (0x00000008)
#define QMND_SCAN_INSUBQ_RANGE_BUILD_SUCCESS  (0x00000000)
#define QMND_SCAN_INSUBQ_RANGE_BUILD_FAILURE  (0x00000008)

/* qmndSCAN.flag                                     */
// plan의 selected method가 세팅되었는지 여부를 구한다.
#define QMND_SCAN_SELECTED_METHOD_SET_MASK    (0x00000010)
#define QMND_SCAN_SELECTED_METHOD_SET_FALSE   (0x00000000)
#define QMND_SCAN_SELECTED_METHOD_SET_TRUE    (0x00000010)

/* qmndSCAN.flag                                     */
// doItFirst에서 cursor의 restart 필요 여부를 구한다.
#define QMND_SCAN_RESTART_CURSOR_MASK       (0x00000020)
#define QMND_SCAN_RESTART_CURSOR_FALSE      (0x00000000)
#define QMND_SCAN_RESTART_CURSOR_TRUE       (0x00000020)

/* PROJ-2632 */
#define QMND_SCAN_SERIAL_EXECUTE_MASK       (0x00000040)
#define QMND_SCAN_SERIAL_EXECUTE_FALSE      (0x00000000)
#define QMND_SCAN_SERIAL_EXECUTE_TRUE       (0x00000040)

/*---------------------------------------------------------------------
 *  Example)
 *      SELECT * FROM T1 WHERE i2 > 3;
 *      ---------------------
 *      | i1 | i2 | i3 | i4 |
 *      ~~~~~~~~~~~~~~~~~~~~~
 *       mtc->mtc->mtc->mtc
 *        |    |    |    |
 *       qtc  qtc  qtc  qtc
 ----------------------------------------------------------------------*/

// PROJ-1446 Host variable을 포함한 질의 최적화
// qmncSCAN에 산재해 있던 각종 filter/key range 정보들을
// 하나의 자료구조로 묶음
typedef struct qmncScanMethod
{
    qcmIndex     * index;

    //---------------------------------
    // Predicate 종류
    //---------------------------------

    // Key Range
    qtcNode      * fixKeyRange;          // Fixed Key Range
    qtcNode      * fixKeyRange4Print;    // Printable Fixed Key Range
    qtcNode      * varKeyRange;          // Variable Key Range
    qtcNode      * varKeyRange4Filter;   // Variable Fixed Key Range

    // Key Filter
    qtcNode      * fixKeyFilter;         // Fixed Key Filter
    qtcNode      * fixKeyFilter4Print;   // Printable Fixed Key Filter
    qtcNode      * varKeyFilter;         // Variable Key Filter
    qtcNode      * varKeyFilter4Filter;  // Variable Fixed Key Filter

    // Filter
    qtcNode      * constantFilter;    // Constant Filter
    qtcNode      * filter;            // Filter
    qtcNode      * lobFilter;       // Lob Filter ( BUG-25916 ) 
    qtcNode      * subqueryFilter;    // Subquery Filter

    // PROJ-1789 PROWID
    qtcNode      * ridRange;
} qmncScanMethod;

typedef struct qmncSCAN
{
    //---------------------------------
    // Code 영역 공통 정보
    //---------------------------------

    qmnPlan        plan;
    UInt           flag;
    UInt           planID;

    //---------------------------------
    // SCAN 고유 정보
    //---------------------------------

    UShort         tupleRowID;          // Tuple ID

    const void   * table;               // Table Handle
    smSCN          tableSCN;            // Table SCN

    qcmAccessOption accessOption;       /* PROJ-2359 Table/Partition Access Option */

    void         * dumpObject;          // PROJ-1618 On-line Dump

    UInt           lockMode;            // Lock Mode
    smiCursorProperties cursorProperty; // Cursor Property
    qmsLimit     * limit;               // qmsParseTree의 limit를 복사

    /* PROJ-1832 New database link */
    SChar        * databaseLinkName;
    SChar        * remoteQuery;
    
    // PROJ-2551 simple query 최적화
    idBool         isSimple;    // simple index scan or simple full scan
    UShort         simpleValueCount;
    qmnValueInfo * simpleValues;
    UInt           simpleValueBufSize;  // c-type value를 mt-type value로 변환
    idBool         simpleUnique;  // 최대 1건인 경우
    idBool         simpleRid;     // rid scan인 경우
    UInt           simpleCompareOpCount;
    mtcColumn    * mSimpleColumns;
    
    //---------------------------------
    // Predicate 종류
    //---------------------------------
    // PROJ-1446 Host variable을 포함한 질의 최적화
    // 직접 가지고 있던 각종 filter/key range등의 정보를
    // qmncScanMethod 자료 구조로 묶음
    qmncScanMethod  method;

    // NNF Filter
    qtcNode      * nnfFilter;

    //---------------------------------
    // Display 관련 정보
    //---------------------------------

    qmsNamePosition      remoteUserName;     // Remote User Name
    qmsNamePosition      remoteObjectName;   // Remote Object Name
    
    qmsNamePosition      tableOwnerName;     // Table Owner Name
    qmsNamePosition      tableName;          // Table Name
    qmsNamePosition      aliasName;          // Alias Name

    /* BUG-44520 미사용 Disk Partition의 SCAN Node를 출력하다가,
     *           Partition Name 부분에서 비정상 종료할 수 있습니다.
     *  Lock을 잡지 않고 Meta Cache를 사용하면, 비정상 종료할 수 있습니다.
     *  SCAN Node에서 Partition Name을 보관하도록 수정합니다.
     */
    SChar                partitionName[QC_MAX_OBJECT_NAME_LEN + 1];

    /* BUG-44633 미사용 Disk Partition의 SCAN Node를 출력하다가,
     *           Index Name 부분에서 비정상 종료할 수 있습니다.
     *  Lock을 잡지 않고 Meta Cache를 사용하면, 비정상 종료할 수 있습니다.
     *  SCAN Node에서 Index ID를 보관하도록 수정합니다.
     */
    UInt                 partitionIndexId;

    // PROJ-1502 PARTITIONED DISK TABLE
    qmsPartitionRef    * partitionRef;
    qmsTableRef        * tableRef;

    // PROJ-1446 Host variable을 포함한 질의 최적화
    qmoScanDecisionFactor * sdf;

    /* PROJ-2632 */
    UInt                  mSerialFilterOffset; /* in Execute, Dataplan Offset */
    UInt                  mSerialFilterSize;   /* in Execute, Total Entry Allocate Size */
    UInt                  mSerialFilterCount;  /* in Execute, Total Entry Set Count */
    mtxSerialFilterInfo * mSerialFilterInfo;

} qmncSCAN;

typedef struct qmndScanFixedTable
{
    UChar               * recRecord;
    UChar               * traversePtr;
    ULong                 readRecordPos;
    iduFixedTableMemory   memory;
} qmndScanFixedTable;
//-----------------------------------------------------------------
// [Data 영역의 SCAN 노드]
// - Predicate 종류
//    A3와 달리 최종 Key Range Pointer를 관리한다.
//    이는 IN SUBQUERY KEYRANGE등과 같이 Key Range 생성의
//    성공, 실패가 함께 발생할 수 있어 Variable Key Range영역을
//    상실할 수 있기 때문이다.
//-----------------------------------------------------------------
typedef struct qmndSCAN
{
    //---------------------------------
    // Data 영역 공통 정보
    //---------------------------------

    qmndPlan                    plan;
    doItFunc                    doIt;
    UInt                      * flag;

    //---------------------------------
    // SCAN 고유 정보
    //---------------------------------

    // fix BUG-9052
    // subquery filter가 outer column 참조시
    // outer column을 참조한 store and search를
    // 재수행하도록 하기 위해서 ... qmnSCAN::readRow()함수주석참조
    // printPlan()내에서 ACCESS count display시
    // DataPlan->plan.myTuple->modify에서 이 값을 빼주도록 한다.
    UInt                        subQFilterDepCnt;

    // PROJ-1071
    smiTableCursor            * cursor;          // Cursor
    smiCursorProperties         cursorProperty;  // Cursor 관련 정보
    smiCursorType               cursorType;      // PROJ-1502 PARTITIONED DISK TABLE
    UInt                        lockMode;        // Lock Mode
    qmndScanFixedTable          fixedTable;         // BUG-42639 Monitoring Query
    smiFixedTableProperties     fixedTableProperty; // BUG-42639 Mornitoring Query

    //---------------------------------
    // Update를 위한 Column List
    //---------------------------------

    smiColumnList             * updateColumnList;   // Update가 아닌 경우, NULL로 설정
    idBool                      inplaceUpdate;      // inplace update

    //---------------------------------
    // Predicate 종류
    //---------------------------------

    smiRange                  * fixKeyRangeArea; // Fixed Key Range 영역
    smiRange                  * fixKeyRange;     // Fixed Key Range
    UInt                        fixKeyRangeSize; // Fixed Key Range 크기

    smiRange                  * fixKeyFilterArea; //Fixed Key Filter 영역
    smiRange                  * fixKeyFilter;    // Fixed Key Filter
    UInt                        fixKeyFilterSize;// Fixed Key Filter 크기

    smiRange                  * varKeyRangeArea; // Variable Key Range 영역
    smiRange                  * varKeyRange;     // Variable Key Range
    UInt                        varKeyRangeSize; // Variable Key Range 크기

    smiRange                  * varKeyFilterArea; //Variable Key Filter 영역
    smiRange                  * varKeyFilter;    // Variable Key Filter
    UInt                        varKeyFilterSize;// Variable Key Filter 크기

    smiRange                  * notNullKeyRange; // Not Null Key Range
    
    smiRange                  * keyRange;        // 최종 Key Range
    smiRange                  * keyFilter;       // 최종 Key Filter
    smiRange                  * ridRange;

    // Filter 관련 CallBack 정보
    smiCallBack                 callBack;        // Filter CallBack
    qtcSmiCallBackDataAnd       callBackDataAnd; //
    qtcSmiCallBackData          callBackData[3]; // 세 종류의 Filter가 가능함.

    //---------------------------------
    // Merge Join 관련 정보
    //---------------------------------

    smiCursorPosInfo            cursorInfo;

    //---------------------------------
    // Disk Table 관련 정보
    //---------------------------------

    void                      * nullRow;  // Disk Table을 위한 null row
    scGRID                      nullRID;

    //---------------------------------
    // Trigger 처리를 위한 정보
    //---------------------------------

    // PROJ-1705
    // 1. update, delete의 경우 trigger row가 필요여부 정보
    //   이 정보는 cusor open시 fetch column정보구성에 쓰이며,
    //   trigger row가 필요한 경우 ( 테이블 전체 컬럼 패치 )
    //   trigger row가 필요하지 않은 경우 ( validation시 수집한 컬럼에 대해서 패치 )
    // 2. partitioned table의 update시
    //    rowMovement인 경우 테이블 전체 컬럼 패치
    idBool                      isNeedAllFetchColumn;    

    // PROJ-1071 Parallel query
    UInt                        mOrgModifyCnt;
    UInt                        mOrgSubQFilterDepCnt;

    /* PROJ-2402 Parallel Table Scan */
    UInt                        mAccessCnt4Parallel;

    /* PROJ-2632 */
    mtxSerialExecuteData      * mSerialExecuteData;
} qmndSCAN;

class qmnSCAN
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

    /* PROJ-2402 */
    static IDE_RC readyIt( qcTemplate * aTemplate,
                           qmnPlan    * aPlan,
                           UInt         aTID );

    /* PROJ-1832 New database link */
    static IDE_RC printRemotePlan( qmnPlan      * aPlan,
                                   ULong          aDepth,
                                   iduVarString * aString );

    static IDE_RC printLocalPlan( qcTemplate   * aTemplate,
                                  qmnPlan      * aPlan,
                                  ULong          aDepth,
                                  iduVarString * aString,
                                  qmnDisplay     aMode );

    //------------------------
    // mapping by doIt() function pointer
    //------------------------

    // 호출되어서는 안됨.
    static IDE_RC doItDefault( qcTemplate * aTemplate,
                               qmnPlan    * aPlan,
                               qmcRowFlag * aFlag );

    // 항상 조건을 만족할 수 없는 경우
    static IDE_RC doItAllFalse( qcTemplate * aTemplate,
                                qmnPlan    * aPlan,
                                qmcRowFlag * aFlag );

    // 최초 SCAN을 수행
    static IDE_RC doItFirst( qcTemplate * aTemplate,
                             qmnPlan    * aPlan,
                             qmcRowFlag * aFlag );

    // 다음 SCAN을 수행
    static IDE_RC doItNext( qcTemplate * aTemplate,
                            qmnPlan    * aPlan,
                            qmcRowFlag * aFlag );

    // 최초 Fixed Table SCAN을 수행
    static IDE_RC doItFirstFixedTable( qcTemplate * aTemplate,
                                       qmnPlan    * aPlan,
                                       qmcRowFlag * aFlag );

    // 다음 Fixed Table SCAN을 수행
    static IDE_RC doItNextFixedTable( qcTemplate * aTemplate,
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
                                         qmncSCAN   * aCodePlan,
                                         qmndSCAN   * aDataPlan );

    static IDE_RC makeRidRange(qcTemplate* aTemplate,
                               qmncSCAN  * aCodePlan,
                               qmndSCAN  * aDataPlan);

    // PROJ-1446 Host variable을 포함한 질의 최적화
    // plan에게 data 영역에 selected method가 세팅되었음을 알려준다.
    static IDE_RC notifyOfSelectedMethodSet( qcTemplate * aTemplate,
                                             qmncSCAN   * aCodePlan );

    // PCRD에서 직접 cursor를 열수 있도록 제공함
    static IDE_RC openCursorForPartition( qcTemplate * aTemplate,
                                          qmncSCAN   * aCodePlan,
                                          qmndSCAN   * aDataPlan );

    // PROJ-1624 global non-partitioned index
    // PCRD에서 직접 readRow를 할 수 있도록 제공함
    static IDE_RC readRowFromGRID( qcTemplate * aTemplate,
                                   qmnPlan    * aPlan,
                                   scGRID       aRowGRID,
                                   qmcRowFlag * aFlag );

    // code plan의 qmncScanMethod를 사용할 지,
    // sdf의 candidate들 중의 qmncScanMethod를 사용할 지 판단한다.
    static qmncScanMethod * getScanMethod( qcTemplate * aTemplate,
                                           qmncSCAN   * aCodePlan );

    static void resetExecInfo4Subquery(qcTemplate *aTemplate, qmnPlan *aPlan);

private:

    //------------------------
    // 초기화 관련 함수
    //------------------------

    // 최초 초기화
    static IDE_RC firstInit( qcTemplate * aTemplate,
                             qmncSCAN   * aCodePlan,
                             qmndSCAN   * aDataPlan );

    // Fixed Key Range를 위한 Range 공간 할당
    static IDE_RC allocFixKeyRange( qcTemplate * aTemplate,
                                    qmncSCAN   * aCodePlan,
                                    qmndSCAN   * aDataPlan );

    // Fixed Key Filter를 위한 Range 공간 할당
    static IDE_RC allocFixKeyFilter( qcTemplate * aTemplate,
                                     qmncSCAN   * aCodePlan,
                                     qmndSCAN   * aDataPlan );

    // Variable Key Range를 위한 Range 공간 할당
    static IDE_RC allocVarKeyRange( qcTemplate * aTemplate,
                                    qmncSCAN   * aCodePlan,
                                    qmndSCAN   * aDataPlan );

    // Variable Key Filter를 위한 Range 공간 할당
    static IDE_RC allocVarKeyFilter( qcTemplate * aTemplate,
                                     qmncSCAN   * aCodePlan,
                                     qmndSCAN   * aDataPlan );

    // Not Null Key Range를 위한 Range 공간 할당
    static IDE_RC allocNotNullKeyRange( qcTemplate * aTemplate,
                                        qmncSCAN   * aCodePlan,
                                        qmndSCAN   * aDataPlan );

    static IDE_RC allocRidRange(qcTemplate* aTemplate,
                                qmncSCAN  * aCodePlan,
                                qmndSCAN  * aDataPlan);

    //------------------------
    // Cursor 관련 함수
    //------------------------

    // Cursor의 Open
    static IDE_RC openCursor( qcTemplate * aTemplate,
                              qmncSCAN   * aCodePlan,
                              qmndSCAN   * aDataPlan );

    // Cursor의 Restart
    static IDE_RC restartCursor( qmncSCAN   * aCodePlan,
                                 qmndSCAN   * aDataPlan );

    // cursor close will be executed by aTemplate->cursorMgr
    //     after execution of a query
    // static IDE_RC closeCursor( qmncSCAN * aCodePlan,
    //                            qmndSCAN * aDataPlan );

    //------------------------
    // SCAN의 doIt() 관련 함수
    //------------------------

    // 하나의 record를 읽음
    static IDE_RC readRow( qcTemplate * aTemplate,
                           qmncSCAN   * aCodePlan,
                           qmndSCAN   * aDataPlan,
                           qmcRowFlag * aFlag);

    // 하나의 FixedTable record를 읽음
    static IDE_RC readRowFixedTable( qcTemplate * aTemplate,
                                     qmncSCAN   * aCodePlan,
                                     qmndSCAN   * aDataPlan,
                                     qmcRowFlag * aFlag );

    // IN Subquery KeyRange를 위한 재시도
    static IDE_RC reRead4InSubRange( qcTemplate * aTemplate,
                                     qmncSCAN   * aCodePlan,
                                     qmndSCAN   * aDataPlan,
                                     void      ** aRow );

    //------------------------
    // Plan Display 관련 함수
    //------------------------

    // Access정보를 출력한다.
    static IDE_RC printAccessInfo( qmncSCAN     * aCodePlan,
                                   qmndSCAN     * aDataPlan,
                                   iduVarString * aString,                                   
                                   qmnDisplay      aMode );

    // Predicate의 상세 정보를 출력한다.
    static IDE_RC printPredicateInfo( qcTemplate   * aTemplate,
                                      qmncSCAN     * aCodePlan,
                                      ULong          aDepth,
                                      iduVarString * aString,
                                      qmnDisplay     aMode );

    // Key Range의 상세 정보를 출력
    static IDE_RC printKeyRangeInfo( qcTemplate   * aTemplate,
                                     qmncSCAN     * aCodePlan,
                                     ULong          aDepth,
                                     iduVarString * aString );

    // Key Filter의 상세 정보를 출력
    static IDE_RC printKeyFilterInfo( qcTemplate   * aTemplate,
                                      qmncSCAN     * aCodePlan,
                                      ULong          aDepth,
                                      iduVarString * aString );

    // Filter의 상세 정보를 출력
    static IDE_RC printFilterInfo( qcTemplate   * aTemplate,
                                   qmncSCAN     * aCodePlan,
                                   ULong          aDepth,
                                   iduVarString * aString,
                                   qmnDisplay     aMode );

    //fix BUG-17553
    static void addAccessCount( qmncSCAN   * aPlan,
                                qcTemplate * aTemplate,
                                ULong        aCount );
};

#endif /* _O_QMN_SCAN_H_ */
