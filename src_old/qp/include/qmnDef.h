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
 * $Id: qmnDef.h 86786 2020-02-27 08:04:12Z donovan.seo $
 *
 * Description :
 *     모든 Execution Plan Node 가 가지는 공통 정보를 정의함.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMN_DEF_H_
#define _O_QMN_DEF_H_ 1

#include <qc.h>
#include <qtc.h>
#include <qmc.h>
#include <qmoKeyRange.h>
#include <qmoPredicate.h>
#include <smiDef.h>

/***********************************************************************
 * [Data Plan의 초기화를 위한 정의]
 *
 * Execution Node의 Data Plan의 영역에
 * PREPARE가 완료되었는지, EXECUTION이 완료되었는지를 표현하기 위한
 * mask 값들에 대한 정의이다.
 * 이와 같이 flag의 mask를 사용하는 이유는 Data Plan을 위한 Memory 영역을
 * 모두 초기화하는 부하를 없애고, 필요한 초기화만을 수행하기 위함이다.
 * 따라서, 모든 Data Plan의 flag의 일정 부분을 예약되어 있는데,
 * 그 의미는 다음과 같다.
 * 마지막 Bit : [0001] - first init()을 수행할 지에 대한 여부
 *                     - 0인 경우 수행해야 함.
 **********************************************************************/

//------------------------------------------
// qcTemplate.planFlag
//------------------------------------------
// after PVO, set this flag
// 모든 Plan Node가 firstInit()을 수행할 수 있도록 한다.
#define QMN_PLAN_PREPARE_DONE_MASK           (0x00000000)

/***********************************************************************
 * [DML Cursor를 위한 정의]
 **********************************************************************/

// PROJ-2205 rownum in DML
// DML을 위한 커서의 추가정보
// UPDATE, DELETE, MOVE와 PCRD, SCAN 사이에 FILT같은 다른 노드들이
// 존재할 수 있으므로 cursorInfo를 이용하여 정보를 전달한다.
typedef struct qmnCursorInfo
{
    //-----------------------------------------
    // PCRD, SCAN이 상위의 UPDATE, DELETE, MOVE에게 전달하는 정보
    //-----------------------------------------

    // SCAN인 경우 table cursor 혹은 partition cursor
    // PCRD인 경우 index table cursor
    smiTableCursor  * cursor;

    // SCAN 혹은 PCRD에서 선택된 index (null/local/global)
    qcmIndex        * selectedIndex;
    mtcTuple        * selectedIndexTuple;
    
    /* PROJ-2359 Table/Partition Access Option */
    qcmAccessOption   accessOption;

    //-----------------------------------------
    // UPDATE, DELETE, MOVE가 하위의 PCRD, SCAN에게 전달하는 정보
    //-----------------------------------------
    
    smiColumnList   * updateColumnList;     // update column list
    smiCursorType     cursorType;           // DML cursor type
    idBool            isRowMovementUpdate;  // row movement update인 경우 (insert-delete)
    idBool            inplaceUpdate;        // inplace update
    UInt              lockMode;             // Lock Mode

    /* PROJ-2464 hybrid partitioned table 지원 */
    smiColumnList   * stmtRetryColLst;
    smiColumnList   * rowRetryColLst;
} qmnCursorInfo;

/***********************************************************************
 * [Plan Display를 위한 정의]
 **********************************************************************/

// Plan Tree를 Display하기 위한 String의 한계값을 정의한 상수
// 아래와 같이 최대 길이를 32K로 제한한다.
#define QMN_MAX_PLAN_LENGTH                   (32 * 1024)
#define QMN_PLAN_PRINT_LENGTH                       (256)

#define QMN_PLAN_PRINT_NON_MTR_ID                    (-1)

// Plan Display Option에 대한 정의로
// ALTER SESSION SET EXPLAIN PLAN = ON 과
// ALTER SESSION SET EXPLAIN PLAN = ONLY를 구별하기 위한 상수이다.
typedef enum
{
    QMN_DISPLAY_ALL = 0, // 수행 결과를 모두 Display 함
    QMN_DISPLAY_CODE     // Code 영역의 정보만을 Display 함
} qmnDisplay;

// PROJ-2551 simple query 최적화
typedef enum
{
    QMN_VALUE_TYPE_INIT = 0,
    QMN_VALUE_TYPE_CONST_VALUE,
    QMN_VALUE_TYPE_HOST_VALUE,
    QMN_VALUE_TYPE_COLUMN,
    QMN_VALUE_TYPE_PROWID,
    QMN_VALUE_TYPE_SYSDATE,
    QMN_VALUE_TYPE_TO_CHAR
} qmnValueType;

typedef enum
{
    QMN_VALUE_OP_INIT = 0,
    QMN_VALUE_OP_ASSIGN,
    QMN_VALUE_OP_ADD,
    QMN_VALUE_OP_SUB,
    QMN_VALUE_OP_EQUAL,
    QMN_VALUE_OP_LT,
    QMN_VALUE_OP_LE,
    QMN_VALUE_OP_GT,
    QMN_VALUE_OP_GE
} qmnValueOp;

typedef struct qmnValueColumnInfo
{
    UShort      table;
    mtcColumn   column;
    void      * info;
} qmnValueColumnInfo;
    
typedef struct qmnValueInfo
{
    mtcColumn      column;
    qmnValueType   type;
    qmnValueOp     op;
    idBool         isQueue; // BUG-45715 support ENQUEUE

    union
    {
        qmnValueColumnInfo    columnVal;
        void                * constVal;
        UShort                id;  // bind param id
    } value;
    
} qmnValueInfo;

#define QMN_INIT_VALUE_INFO( val )      \
{                                       \
    (val)->type = QMN_VALUE_TYPE_INIT;  \
    (val)->op = QMN_VALUE_OP_INIT;      \
    (val)->isQueue = ID_FALSE;          \
    (val)->value.constVal = NULL;       \
}

/***********************************************************************
 * [Code Plan을 위한 정의]
 *
 **********************************************************************/

// Execution Plan 의 종류
typedef enum
{
    QMN_NONE = 0,
    QMN_SCAN,      // SCAN plan
    QMN_VIEW,      // VIEW plan
    QMN_PROJ,      // PROJect plan
    QMN_FILT,      // FILTer plan
    QMN_SORT,      // SORT plan
    QMN_LMST,      // LiMit SorT plan  (A4에서 새로 추가)
    QMN_HASH,      // HASH plan
    QMN_HSDS,      // HaSh DiStinct plan
    QMN_GRAG,      // GRoup AGgregation plan
    QMN_AGGR,      // AGGRegation plan
    QMN_GRBY,      // GRoup BY plan
    QMN_CUNT,      // CoUNT(*) plan    (A4에서 새로 추가)
    QMN_CONC,      // CONCatenation plan
    QMN_JOIN,      // JOIN plan
    QMN_MGJN,      // MerGe JoiN plan
    QMN_BUNI,      // Bag UNIon plan
    QMN_SITS,      // Set InTerSect plan
    QMN_SDIF,      // Set DIFference plan
    QMN_LOJN,      // Left Outer JoiN plan
    QMN_FOJN,      // Full Outer JoiN plan
    QMN_AOJN,      // Anti left Outer JoiN plan
    QMN_VMTR,      // View MaTeRialize plan
    QMN_VSCN,      // View SCaN plan
    QMN_GRST,      // GRouping SeTs plan
    QMN_MULTI_BUNI,// Multiple Bag UNIon plan
    QMN_PCRD,      // PROJ-1502 Partition CoRDinator plan
    QMN_CNTR,      // PROJ-1405 CouNTeR plan
    QMN_WNST,      // PROJ-1762 Analytic Function
    QMN_CNBY,      // PROJ-1715
    QMN_CMTR,      // PROJ-1715
    QMN_INST,      // PROJ-2205
    QMN_MTIT,      // BUG-36596 multi-table insert
    QMN_UPTE,      // PROJ-2205
    QMN_DETE,      // PROJ-2205
    QMN_MOVE,      // PROJ-2205
    QMN_MRGE,      // PROJ-2205
    QMN_ROLL,      // PROJ-1353
    QMN_CUBE,      // PROJ-1353
    QMN_PRLQ,      // PROJ-1071 Parallel Query
    QMN_PPCRD,     // PROJ-1071 Parallel Query
    QMN_PSCRD,     /* PROJ-2402 Parallel Table Scan */
    QMN_SREC,      // PROJ-2582 recursive with
    QMN_DLAY,      // BUG-43493 delayed execution
    QMN_SDSE,      // PROJ-2638
    QMN_SDEX,      // PROJ-2638
    QMN_SDIN,      // PROJ-2638
    QMN_MAX_MAX
} qmnType;

//------------------------------------------------
// Code Plan의 Flag 정의
//------------------------------------------------

// qmnPlan.flag
#define QMN_PLAN_FLAG_CLEAR                  (0x00000000)

// qmnPlan.flag
// Plan Node 가 사용하는 저장 매체의 종류
#define QMN_PLAN_STORAGE_MASK                (0x00000001)
#define QMN_PLAN_STORAGE_MEMORY              (0x00000000)
#define QMN_PLAN_STORAGE_DISK                (0x00000001)

// qmnPlan.flag
// Plan Node 가 outer column reference를 갖는 지의 여부
#define QMN_PLAN_OUTER_REF_MASK              (0x00000002)
#define QMN_PLAN_OUTER_REF_FALSE             (0x00000000)
#define QMN_PLAN_OUTER_REF_TRUE              (0x00000002)

// qmnPlan.flag
// Plan Node 가 temp table을 생성시 fixed rid를 가져야 하는지 여부
#define QMN_PLAN_TEMP_FIXED_RID_MASK         (0x00000004)
#define QMN_PLAN_TEMP_FIXED_RID_FALSE        (0x00000000)
#define QMN_PLAN_TEMP_FIXED_RID_TRUE         (0x00000004)

// qmnPlan.flag
// PROJ-2242
#define QMN_PLAN_JOIN_METHOD_TYPE_MASK       (0x000000F0)
#define QMN_PLAN_JOIN_METHOD_FULL_NL         (0x00000010)
#define QMN_PLAN_JOIN_METHOD_INDEX           (0x00000020)
#define QMN_PLAN_JOIN_METHOD_FULL_STORE_NL   (0x00000030)
#define QMN_PLAN_JOIN_METHOD_ANTI            (0x00000040)
#define QMN_PLAN_JOIN_METHOD_INVERSE_INDEX   (0x00000050)
#define QMN_PLAN_JOIN_METHOD_ONE_PASS_HASH   (0x00000060)
#define QMN_PLAN_JOIN_METHOD_TWO_PASS_HASH   (0x00000070)
#define QMN_PLAN_JOIN_METHOD_INVERSE_HASH    (0x00000080)
#define QMN_PLAN_JOIN_METHOD_ONE_PASS_SORT   (0x00000090)
#define QMN_PLAN_JOIN_METHOD_TWO_PASS_SORT   (0x000000A0)
#define QMN_PLAN_JOIN_METHOD_INVERSE_SORT    (0x000000B0)
#define QMN_PLAN_JOIN_METHOD_MERGE           (0x000000C0)

/* qmnPlan.flag
 * PROJ-1071 Parallel Query
 *
 * parallel execute 할지 말지는
 * 하위에 PRLQ 가 있는지 없는지에 따라 결정됨
 */

#define QMN_PLAN_NODE_EXIST_MASK         (0x00003000)

#define QMN_PLAN_PRLQ_EXIST_MASK         (0x00001000)
#define QMN_PLAN_PRLQ_EXIST_TRUE         (0x00001000)
#define QMN_PLAN_PRLQ_EXIST_FALSE        (0x00000000)

#define QMN_PLAN_MTR_EXIST_MASK          (0x00002000)
#define QMN_PLAN_MTR_EXIST_TRUE          (0x00002000)
#define QMN_PLAN_MTR_EXIST_FALSE         (0x00000000)

#define QMN_PLAN_INIT_THREAD_ID          (0)

#define QMN_PLAN_RESULT_CACHE_EXIST_MASK  (0x00004000)
#define QMN_PLAN_RESULT_CACHE_EXIST_TRUE  (0x00004000)
#define QMN_PLAN_RESULT_CACHE_EXIST_FALSE (0x00000000)

#define QMN_PLAN_GARG_PARALLEL_MASK       (0x00008000)
#define QMN_PLAN_GARG_PARALLEL_TRUE       (0x00008000)
#define QMN_PLAN_GARG_PARALLEL_FALSE      (0x00000000)

#define QMN_PLAN_DEFAULT_DEPENDENCY_VALUE (0xFFFFFFFF)

//------------------------------------------------
// Code Plan Node가 공통적으로 가지는 함수 포인터
//------------------------------------------------

// 각 Plan Node의 초기화를 수행하는 함수
typedef IDE_RC (* initFunc ) ( qcTemplate * aTemplate,
                               qmnPlan    * aNode );

// 각 Plan Node의 고유의 기능을 수행하는 함수
typedef IDE_RC (* doItFunc ) ( qcTemplate * aTemplate,
                               qmnPlan    * aNode,
                               qmcRowFlag * aFlag );

// PROJ-2444
// parallel plan 을 수행하기 전에 반드시 해야하는 과정을 수행하는 함수
typedef IDE_RC (* readyItFunc ) ( qcTemplate * aTemplate,
                                  qmnPlan    * aNode,
                                  UInt         aTID );

// 각 Plan Node의 Null Padding을 수행하는 함수
typedef IDE_RC (* padNullFunc) ( qcTemplate * aTemplate,
                                 qmnPlan    * aNode );

// 각 Plan Node의 Plan 정보를 Display하는 함수
typedef IDE_RC (* printPlanFunc) ( qcTemplate   * aTemplate,
                                   qmnPlan      * aNode,
                                   ULong          aDepth,
                                   iduVarString * aString,
                                   qmnDisplay     aMode );

/***********************************************************************
 *  [ Plan 의 구분 ]
 *
 *  ---------------------------------------------
 *    Plan      |  left   |  right  | children
 *  ---------------------------------------------
 *  unary plan  |    O    |  null   |   null
 *  ---------------------------------------------
 *  binary plan |    O    |    O    |   null
 *  ---------------------------------------------
 *  multi  plan |   null  |  null   |    O
 *  ----------------------------------------------
 *
 **********************************************************************/

/***********************************************************************
 * Multiple Children을 구성하기 위한 자료 구조
 *      ( PROJ-1486 Multiple Bag Union )
 **********************************************************************/

typedef struct qmnChildren
{
    struct qmnPlan     * childPlan;     // Child Plan
    struct qmnChildren * next;
} qmnChildren;

typedef struct qmnChildrenIndex
{
    smOID                childOID;      // child partition oid
    struct qmnPlan     * childPlan;     // Child Plan
} qmnChildrenIndex;

// PROJ-2209 prepare phase partition reference sort
typedef struct qmnRangeSortedChildren
{
    struct qmnChildren * children;       // sorted children
    qcmColumn          * partKeyColumns; // partition key column
    qmsPartitionRef    * partitionRef;   // sorted partition reference
}qmnRangeSortedChildren;

//----------------------------------------------
// 모든 Code Node들이 공통으로 가지는 Plan 구조
//----------------------------------------------

typedef struct qmnPlan
{
    qmnType       type;            // plan type
    UInt          flag;            // 기타 Mask 정보
    UInt          offset;          // Data Node offset in templete

    SDouble       qmgOuput;        // PROJ-2242
    SDouble       qmgAllCost;      // PROJ-2242

    initFunc      init;            // init function
    readyItFunc   readyIt;         // readyIt function
    doItFunc      doIt;            // doIt function
    padNullFunc   padNull;         // null padding function
    printPlanFunc printPlan;       // print plan function

    // Invariant 영역 처리를 위한 dependency
    // Plan Node 가 가지는 dependencies 와 대표 dependency
    qcDepInfo     depInfo;
    ULong         dependency;
    ULong         outerDependency; // 대표 outer dependency

    qmnPlan     * left;            // left child node
    qmnPlan     * right;           // right child node
    qmnChildren * children;        // multi children node
    qmnChildren * childrenPRLQ;    // multi children node for parallel queue

    qmcAttrDesc * resultDesc;

    UInt          mParallelDegree;
} qmnPlan;

//----------------------------------------------
// 모든 Data Plan 들이 공통으로 가지는 Plan 구조
//----------------------------------------------
typedef struct qmndPlan
{
    mtcTuple      * myTuple;        // Tuple 위치
    UInt            mTID;           // Node 가 사용한 thread id
} qmndPlan;

//----------------------------------------------
// Key Range, Key Filter, Filter생성을 위한 자료 구조
//----------------------------------------------

typedef struct qmnCursorPredicate
{
    // Input Information
    qcmIndex              * index;               // Input: Index 정보
    UShort                  tupleRowID;          // Input: Tuple 정보

    smiRange              * fixKeyRangeArea;     // Input: Fixed Key Range Area
    smiRange              * fixKeyRange;         // Input: Fixed Key Range
    qtcNode               * fixKeyRangeOrg;      // Input: Fixed Key Range Node
    UInt                    fixKeyRangeSize;

    smiRange              * varKeyRangeArea;     // Input: Variable KeyRange Area
    smiRange              * varKeyRange;         // Input: Variable Key Range
    qtcNode               * varKeyRangeOrg;      // Input: Variable Key Range Node
    qtcNode               * varKeyRange4FilterOrg;   // Input: Variable Fixed Key Range Node
    UInt                    varKeyRangeSize;

    smiRange              * fixKeyFilterArea;    // Input: Fixed Key Filter Area
    smiRange              * fixKeyFilter;        // Input: Fixed Key Filter
    qtcNode               * fixKeyFilterOrg;     // Input: Fixed Key Filter Node
    UInt                    fixKeyFilterSize;

    smiRange              * varKeyFilterArea;    // Input: Var Key Filter Area
    smiRange              * varKeyFilter;        // Input: variable Key Filter
    qtcNode               * varKeyFilterOrg;     // Input: Variable Key Filter Node
    qtcNode               * varKeyFilter4FilterOrg;  // Input: Variable Fixed Key Filter Node
    UInt                    varKeyFilterSize;

    smiRange              * notNullKeyRange;     // Input: Not Null Key Range

    qtcNode               * filter;              // Input: Filter

    // Output Information
    smiCallBack           * filterCallBack;      // Output: Filter CallBack
    qtcSmiCallBackDataAnd * callBackDataAnd;     // Output: Filter And
    qtcSmiCallBackData    * callBackData;        // Output: Filter Data

    /* PROJ-2632 */
    mtxSerialFilterInfo   * mSerialFilterInfo;
    mtxSerialExecuteData  * mSerialExecuteData;

    smiRange              * keyRange;            // Output: Key Range
    smiRange              * keyFilter;           // Output: Key Filter
} qmnCursorPredicate;

/* PROJ-1353 GROUPING, GROUPING_ID Funciton의 공통사용 구조체 */
typedef struct qmnGrouping
{
    SInt   type;
    SInt * index;
} qmnGrouping;

/* PROJ-2339 Inverse Hash Join, Full Ouhter Join에서 공통사용 */
typedef IDE_RC (* setHitFlagFunc) ( qcTemplate * aTemplate,
                                     qmnPlan    * aPlan );

/* PROJ-2339 Inverse Hash Join 에서 사용 */
typedef idBool (* isHitFlaggedFunc) ( qcTemplate * aTemplate,
                                      qmnPlan    * aPlan );

/* BUG-42467 */
typedef enum qmndResultCacheStatus
{
    QMND_RESULT_CACHE_INIT = 0,
    QMND_RESULT_CACHE_HIT,
    QMND_RESULT_CACHE_MISS
} qmndResultCacheStatus;

/* PROJ-2462 Result Cache */
typedef struct qmndResultCache
{
    UInt                 * flag;
    UInt                   missCount;
    UInt                   hitCount;
    iduMemory            * memory;
    UInt                   memoryIdx;
    SLong                  tablesModify;
    qmndResultCacheStatus  status;
} qmndResultCache;

//----------------------------------------------
// 모든 Plan이 공통적으로 사용하는 기능
//----------------------------------------------

class qmn
{
public:
    // Subquery의 Plan을 Display함
    static IDE_RC printSubqueryPlan( qcTemplate   * aTemplate,
                                     qtcNode      * aSubQuery,
                                     ULong          aDepth,
                                     iduVarString * aString,
                                     qmnDisplay     aMode );

    // PROJ-2242
    static void printMTRinfo( iduVarString * aString,
                              ULong          aDepth,
                              qmcMtrNode   * aMtrNode,
                              const SChar  * aMtrName,
                              UShort         aSelfID,
                              UShort         aRefID1,
                              UShort         aRefID2 );

    // PROJ-2242
    static void printJoinMethod( iduVarString * aString,
                                 UInt           aqmnflag );

    // PROJ-2242
    static void printCost( iduVarString * aString,
                           SDouble        aCost );

    // SCAN, HIER, CUNT 등 Cursor를 사용하는 노드에서 사용
    // Key Range, Key Filter, Filter를 생성함.
    static IDE_RC makeKeyRangeAndFilter( qcTemplate         * aTemplate,
                                         qmnCursorPredicate * aPredicate );

    // PROJ-1446 Host variable을 포함한 질의 최적화
    // scan 또는 cunt plan에 대해 table handle와 table scn을 얻는다.
    static IDE_RC getReferencedTableInfo( qmnPlan     * aPlan,
                                          const void ** aTableHandle,
                                          smSCN       * aTableSCN,
                                          idBool      * aIsFixedTable );

    // qmo::optimizeForHost에서 selected method를 세팅한 후
    // plan에게 알려준다.
    static IDE_RC notifyOfSelectedMethodSet( qcTemplate * aTemplate,
                                             qmnPlan    * aPlan );

    // PROJ-1705
    // 디스크테이블에 대한 null row 생성
    static IDE_RC makeNullRow( mtcTuple   * aTuple,
                               void       * aNullRow );
    
    static IDE_RC printResult( qcTemplate        * aTemplate,
                               ULong               aDepth,
                               iduVarString      * aString,
                               const qmcAttrDesc * aResultDesc );

    static void printSpaceDepth(iduVarString* aString, ULong aDepth);

    // PROJ-2462 Result Cache
    static void printResultCacheInfo( iduVarString    * aString,
                                      ULong             aDepth,
                                      qmnDisplay        aMode,
                                      idBool            aIsInit,
                                      qmndResultCache * aResultData );

    // PROJ-2462 Result Cache
    static void printResultCacheRef( iduVarString    * aString,
                                     ULong             aDepth,
                                     qcComponentInfo * aComInfo );

    static void setDisplayInfo( qmsTableRef      * aTableRef,
                                qmsNamePosition  * aTableOwnerName,
                                qmsNamePosition  * aTableName,
                                qmsNamePosition  * aAliasName );

private:

    // Key Range의 생성
    static IDE_RC makeKeyRange ( qcTemplate         * aTemplate,
                                 qmnCursorPredicate * aPredicate,
                                 qtcNode           ** aFilter );

    // Key Filter의 생성
    static IDE_RC makeKeyFilter( qcTemplate         * aTemplate,
                                 qmnCursorPredicate * aPredicate,
                                 qtcNode           ** aFilter );

    // Filter CallBack 생성
    static IDE_RC makeFilter( qcTemplate         * aTemplate,
                              qmnCursorPredicate * aPredicate,
                              qtcNode            * aFirstFilter,
                              qtcNode            * aSecondFilter,
                              qtcNode            * aThirdFilter );
};

#endif /* _O_QMN_DEF_H_ */
