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
 * $Id: qmoPredicate.h 90192 2021-03-12 02:01:03Z jayce.park $
 *
 * Description :
 *     Predicate Manager
 *
 *     비교 연산자를 포함하는 Predicate들에 대한 전반적인 관리를
 *     담당한다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMO_PREDICATE_H_
#define _O_QMO_PREDICATE_H_ 1

#include <qc.h>
#include <qmsParseTree.h>
#include <qmoStatMgr.h>


//---------------------------------------------------
// Predicate의 columnID 정의
//---------------------------------------------------

/* qmoPredicate.id                                 */
// QMO_COLUMNID_NOT_FOUND : 초기값 columnID, getColumnID() 에서 사용
// QMO_COLUMNID_LIST : LIST에 대한 정의된 columnID
// QMO_COLUMNID_NON_INDEXABLE : non-indexable에 대한 정의된 columnID
#define QMO_COLUMNID_NOT_FOUND             (ID_UINT_MAX-2)
#define QMO_COLUMNID_LIST                  (ID_UINT_MAX-1)
#define QMO_COLUMNID_NON_INDEXABLE         (  ID_UINT_MAX)

//---------------------------------------------------
// Predicate의 flag 정의
//---------------------------------------------------

/* qmoPredicate.flag를 초기화 시킴                    */
# define QMO_PRED_CLEAR                       (0x00000000)

/* qmoPredicate.flag                                  */
// Key Range의 연속적인 사용 가능 여부
// keyRange 추출, 생성시 필요한 정보
# define QMO_PRED_NEXT_KEY_USABLE_MASK        (0x00000001)
# define QMO_PRED_NEXT_KEY_UNUSABLE           (0x00000000)
# define QMO_PRED_NEXT_KEY_USABLE             (0x00000001)

/* qmoPredicate.flag                                  */
// join predicate인지에 대한 정보
# define QMO_PRED_JOIN_PRED_MASK              (0x00000002)
# define QMO_PRED_JOIN_PRED_FALSE             (0x00000000)
# define QMO_PRED_JOIN_PRED_TRUE              (0x00000002)

/* qmoPredicate.flag                                  */
// fixed or variable인지의 정보
# define QMO_PRED_VALUE_MASK                  (0x00000004)
# define QMO_PRED_FIXED                       (0x00000000)
# define QMO_PRED_VARIABLE                    (0x00000004)

/* qmoPredicate.flag                                  */
// joinable predicate 정보
# define QMO_PRED_JOINABLE_PRED_MASK          (0x00000008)
# define QMO_PRED_JOINABLE_PRED_FALSE         (0x00000000)
# define QMO_PRED_JOINABLE_PRED_TRUE          (0x00000008)

/* qmoPredicate.flag                                  */
// 수행가능 join method 설정
# define QMO_PRED_JOINABLE_MASK ( QMO_PRED_INDEX_JOINABLE_MASK |        \
                                  QMO_PRED_HASH_JOINABLE_MASK  |        \
                                  QMO_PRED_SORT_JOINABLE_MASK  |        \
                                  QMO_PRED_MERGE_JOINABLE_MASK )
# define QMO_PRED_JOINABLE_FALSE              (0x00000000)

/* qmoPredicate.flag                                  */
// index nested loop join method 사용가능 정보 설정
# define QMO_PRED_INDEX_JOINABLE_MASK         (0x00000010)
# define QMO_PRED_INDEX_JOINABLE_FALSE        (0x00000000)
# define QMO_PRED_INDEX_JOINABLE_TRUE         (0x00000010)

/* qmoPredicate.flag                                  */
// hash join method 사용가능 정보 설정
# define QMO_PRED_HASH_JOINABLE_MASK          (0x00000020)
# define QMO_PRED_HASH_JOINABLE_FALSE         (0x00000000)
# define QMO_PRED_HASH_JOINABLE_TRUE          (0x00000020)

/* qmoPredicate.flag                                  */
// sort join method 사용가능 정보 설정
# define QMO_PRED_SORT_JOINABLE_MASK          (0x00000040)
# define QMO_PRED_SORT_JOINABLE_FALSE         (0x00000000)
# define QMO_PRED_SORT_JOINABLE_TRUE          (0x00000040)

/* qmoPredicate.flag                                  */
// merge join method 사용가능 정보 설정
# define QMO_PRED_MERGE_JOINABLE_MASK         (0x00000080)
# define QMO_PRED_MERGE_JOINABLE_FALSE        (0x00000000)
# define QMO_PRED_MERGE_JOINABLE_TRUE         (0x00000080)

/* qmoPredicate.flag                                  */
// index nested loop join 수행 가능 방향 설정
# define QMO_PRED_INDEX_DIRECTION_MASK        (0x00000300)
# define QMO_PRED_INDEX_NON_DIRECTION         (0x00000000)
# define QMO_PRED_INDEX_LEFT_RIGHT            (0x00000100)
# define QMO_PRED_INDEX_RIGHT_LEFT            (0x00000200)
# define QMO_PRED_INDEX_BIDIRECTION           (0x00000300)

/* BUG-47509 */
#define QMO_PRED_OR_VALUE_INDEX_MASK          (0x00000400)
#define QMO_PRED_OR_VALUE_INDEX_TRUE          (0x00000400)
#define QMO_PRED_OR_VALUE_INDEX_FALSE         (0x00000000)

/* BUG-47986 */
#define QMO_PRED_JOIN_OR_VALUE_INDEX_MASK     (0x00000800)
#define QMO_PRED_JOIN_OR_VALUE_INDEX_TRUE     (0x00000800)
#define QMO_PRED_JOIN_OR_VALUE_INDEX_FALSE    (0x00000000)

/* qmoPredicate.flag                                  */
// merge join 수행 가능 방향 설정
# define QMO_PRED_MERGE_DIRECTION_MASK        (0x00003000)
# define QMO_PRED_MERGE_NON_DIRECTION         (0x00000000)
# define QMO_PRED_MERGE_LEFT_RIGHT            (0x00001000)
# define QMO_PRED_MERGE_RIGHT_LEFT            (0x00002000)

/* qmoPredicate.flag                                  */
// joinGroup에 포함되었는지 여부
# define QMO_PRED_INCLUDE_JOINGROUP_MASK      (0x00010000)
# define QMO_PRED_INCLUDE_JOINGROUP_FALSE     (0x00000000)
# define QMO_PRED_INCLUDE_JOINGROUP_TRUE      (0x00010000)

/* qmoPredicate.flag                                  */
// Index Nested Loop Join의 joinable predicate임을 표시하는 정보
// join predicate을 selection graph로 내리게 되고,
// keyRange 추출시, 이 정보가 설정되어 있는 predicate이 우선적으로 처리됨.
# define QMO_PRED_INDEX_NESTED_JOINABLE_MASK  (0x00020000)
# define QMO_PRED_INDEX_NESTED_JOINABLE_FALSE (0x00000000)
# define QMO_PRED_INDEX_NESTED_JOINABLE_TRUE  (0x00020000)

/* qmoPredicate.flag                                  */
// keyRange로 추출된 predicate이
// IN절의 subqueryKeyRange임을 표시하는 정보
# define QMO_PRED_INSUBQ_KEYRANGE_MASK        (0x00040000)
# define QMO_PRED_INSUBQ_KEYRANGE_FALSE       (0x00000000)
# define QMO_PRED_INSUBQ_KEYRANGE_TRUE        (0x00040000)

/* qmoPredicate.flag                                  */
// selectivity 계산 시에
// composite index 사용 가능하여 selectivity 보정이 필요한 경우
# define QMO_PRED_USABLE_COMP_IDX_MASK        (0x00080000)
# define QMO_PRED_USABLE_COMP_IDX_FALSE       (0x00000000)
# define QMO_PRED_USABLE_COMP_IDX_TRUE        (0x00080000)

/* qmoPredicate.flag                                  */
// 개별 indexable predicate에 대해서 IN(subquery)를 포함하는지의 정보
// IN(subquery)에 대해서는 keyRange를 단독으로 구성해야하므로,
// 이 정보는 keyRange 추출시 사용된다.
# define QMO_PRED_INSUBQUERY_MASK             (0x00100000)
# define QMO_PRED_INSUBQUERY_ABSENT           (0x00000000)
# define QMO_PRED_INSUBQUERY_EXIST            (0x00100000)

/* qmoPredicate.flag                                  */
// joinable selectivity 계산시,
// 이미 처리된 predicate에 대한 중복 선택을 방지하기 위한 정보
# define QMO_PRED_SEL_PROCESS_MASK            (0x00200000)
# define QMO_PRED_SEL_PROCESS_FALSE           (0x00000000)
# define QMO_PRED_SEL_PROCESS_TRUE            (0x00200000)

/* qmoPredicate.flag                                  */
// 동일 컬럼리스트에 equal, in 비교연산자 존재여부
# define QMO_PRED_EQUAL_IN_MASK               (0x00400000)
# define QMO_PRED_EQUAL_IN_ABSENT             (0x00000000)
# define QMO_PRED_EQUAL_IN_EXIST              (0x00400000)

/* qmoPredicate.flag                                  */

// view에 대한 push selection되고, where절에 남아 있는 predicate인지의 정보
# define QMO_PRED_PUSH_REMAIN_MASK            (0x00800000)
# define QMO_PRED_PUSH_REMAIN_FALSE           (0x00000000)
# define QMO_PRED_PUSH_REMAIN_TRUE            (0x00800000)

/* qmoPredicate.flag                                  */
// Fix BUG-12934
// constant filter에 대해서는
// subquery의 store and search 최적화팁을 적용하지 않는다.
// 예) 1 in subquery ...
#define QMO_PRED_CONSTANT_FILTER_MASK         (0x01000000)
#define QMO_PRED_CONSTANT_FILTER_TRUE         (0x01000000)
#define QMO_PRED_CONSTANT_FILTER_FALSE        (0x00000000)

/* PROJ-1446 Host variable을 포함한 질의 최적화 */
// 하위에 host 변수를 포함한 질의 최적화 가능한 predicate에 대해
// 정보를 세팅한다.
// 이 정보는 predicate의 selectivity를 execution시에 다시
// 구할 때, 최적화 여부가 없는 predicate들에 대해서는
// 불필요한 작업을 막기 위함이다.
#define QMO_PRED_HOST_OPTIMIZE_MASK           (0x02000000)
#define QMO_PRED_HOST_OPTIMIZE_TRUE           (0x02000000)
#define QMO_PRED_HOST_OPTIMIZE_FALSE          (0x00000000)

/* PROJ-1446 Host variable을 포함한 질의 최적화 */
// more 리스트 중에 하나라도 QMO_PRED_HOST_OPTIMIZE_TRUE가
// 세팅되어 있으면 more list의 맨 처음 predicate는
// QMO_PRED_HEAD_HOST_OPTIMIZE_TRUE가 세팅된다.
#define QMO_PRED_HEAD_HOST_OPTIMIZE_MASK      (0x04000000)
#define QMO_PRED_HEAD_HOST_OPTIMIZE_TRUE      (0x04000000)
#define QMO_PRED_HEAD_HOST_OPTIMIZE_FALSE     (0x00000000)

// QMO_PRED_HEAD_HOST_OPT_TOTAL_TRUE는 more에 두개의
// predicate가 있고 통합 selectivity를 구할 수 있는
// 경우를 의미한다. 이 값은 setTotal에서 세팅된다.
#define QMO_PRED_HEAD_HOST_OPT_TOTAL_MASK     (0x08000000)
#define QMO_PRED_HEAD_HOST_OPT_TOTAL_TRUE     (0x08000000)
#define QMO_PRED_HEAD_HOST_OPT_TOTAL_FALSE    (0x00000000)

/* PROJ-1446 Host variable을 포함한 질의 최적화 */
// qmoPredicate의 mySelectivityOffset과 totalSelectivityOffset에 사용됨.
#define QMO_SELECTIVITY_OFFSET_NOT_USED           UINT_MAX

// PROJ-1446 Host variable을 포함한 질의 최적화
// qmoPredWrapperPool의 pool의 초기 배열 크기
#define QMO_DEFAULT_WRAPPER_POOL_SIZE                 (32)

/* qmoPredicate.flag                                  */
// PROJ-1495
// push_pred hint에 의해 VIEW selection graph 내로 push 된 predicate
#define QMO_PRED_PUSH_PRED_HINT_MASK          (0x10000000)
#define QMO_PRED_PUSH_PRED_HINT_FALSE         (0x00000000)
#define QMO_PRED_PUSH_PRED_HINT_TRUE          (0x10000000)

/* qmoPredicate.flag                                  */
// PROJ-1404
// transformation으로 생성된 된 transitive predicate
#define QMO_PRED_TRANS_PRED_MASK              (0x20000000)
#define QMO_PRED_TRANS_PRED_FALSE             (0x00000000)
#define QMO_PRED_TRANS_PRED_TRUE              (0x20000000)

/* BUG-39298 improve preserved order */
#define QMO_PRED_INDEXABLE_EQUAL_MASK          (0x40000000)
#define QMO_PRED_INDEXABLE_EQUAL_TRUE          (0x40000000)
#define QMO_PRED_INDEXABLE_EQUAL_FALSE         (0x00000000)

/* TASK-7219 Non-shard DML */
#define QMO_PRED_PUSHED_FORCE_PRED_MASK        (0x80000000)
#define QMO_PRED_PUSHED_FORCE_PRED_TRUE        (0x80000000)
#define QMO_PRED_PUSHED_FORCE_PRED_FALSE       (0x00000000)

/* qmoPredicate.flag                                  */
// variable predicate의 판단
// variable 판단 정보
//   (1) join predicate 존재, (2) subquery 존재
//   (3) prior column 존재  ,
//   fix BUG-10524 (4) SYSDATE column 존재
//   (5) procedure variable의 존재
//   (6) host 변수 존재
//   To Fix PR-10209
//   (7) Value가 Variable인지를 판단

# define QMO_PRED_IS_VARIABLE( aPredicate )                                \
    (                                                                      \
       (  ( ( (aPredicate)->flag & QMO_PRED_JOIN_PRED_MASK )               \
             == QMO_PRED_JOIN_PRED_TRUE )                                  \
          ||                                                               \
          ( ( (aPredicate)->flag & QMO_PRED_VALUE_MASK )                   \
             == QMO_PRED_VARIABLE )                                        \
          ||                                                               \
          ( QTC_IS_VARIABLE( (aPredicate)->node )                          \
             == ID_TRUE )                                                  \
       )                                                                   \
       ? ID_TRUE : ID_FALSE                                                \
    )

#define QMO_PRED_IS_DYNAMIC_OPTIMIZABLE( aPredicate )                      \
    QTC_IS_DYNAMIC_CONSTANT( (aPredicate)->node )

//---------------------------------------------------
// [subquery에 대한 graph 생성여부를 판단.]
//  1. subquery에 대한 graph 생성여부를 판단하는 이유는 다음과 같다.
//     (1) subquery관리자가 subquery의 graph를 중복생성하지 않도록
//     (2) view 최적화 팁 적용시 graph 생성여부 확인
//  2. subquery에 대한 qcStatement자료구조를 참조해서,
//     이 자료구조의 graph가 NULL이 아닌지를 검사한다.
//---------------------------------------------------

//# define IS_EXIST_GRAPH_IN_SUBQUERY( aSubStatement )
//    ( ( aSubStatement->graph != NULL ) ? ID_TRUE : ID_FALSE )

//---------------------------------------------------
// [subquery에 대한 plan 생성여부 판단]
//  1. subquery 관리자가 subquery에 대한 plan을 중복생성하지 않기 위해.
//  2. subquery에 대한 qcStatement자료구조를 참조해서,
//     이 자료구조의 plan이 NULL이 아닌지를 검사한다.
//---------------------------------------------------

//# define IS_EXIST_PLAN_IN_SUBQUERY( aSubStatement )
//    ( ( aSubStatement->plan != NULL ) ? ID_TRUE : ID_FALSE )

//---------------------------------------------------
// Predicate을 관리하기 위한 자료 구조
//---------------------------------------------------

typedef struct qmoPredicate
{

    // filter ordering시 플랫폼간의 diff 방지를 위해
    // qsort시 compare함수내에서 사용됨.
    UInt       idx;

    // predicate에 대한 각종 정보
    UInt       flag;

    //-----------------------------------------------
    // [ id ] columnID
    // (1)한 컬럼인 경우: smiColum.id
    // (2)LIST: 정의된 columnID
    // (3)N/A : 정의된 columnID  (N/A=non-indexable)
    //-----------------------------------------------
    UInt       id;

    qtcNode  * node;        // OR나 비교연산자 단위의 predicate


    //-----------------------------------------------
    // [ selectivity ]
    // mySelectivity    : predicate의 개별 selectivity
    // totalSelectivity : qmoPredicate.id가 같은 모든 predicate에 대한
    //                    대표 selectivity.
    //                    qmoPredicate->more의 연결리스트 중
    //                    첫번째 qmoPredicate에만 이 정보가 설정된다.
    //-----------------------------------------------
    SDouble    mySelectivity;    // predicate의 개별 selectivity
    SDouble    totalSelectivity; // 컬럼의 대표 selectivity

    // PROJ-1446 Host variable을 포함한 질의 최적화
    UInt       mySelectivityOffset;   // 바인딩 후 my sel.을 구하기 위해
    UInt       totalSelectivityOffset;// 바인딩 후 total sel.을 구하기 위해

    //-----------------------------------------------
    // qmoPredicate 연결관련 자료구조
    // next : qmoPredicate.id가 다른 predicate의 연결
    // more : qmoPredicate.id가 같은 predicate의 연결
    //-----------------------------------------------
    qmoPredicate  * next;        // qmoPredicate.id가 다른 predicate의 연결
    qmoPredicate  * more;        // qmoPredicate.id가 같은 predicate의 연결

} qmoPredicate;

//---------------------------------------------------
// qmoPredicate 연결정보를 관리하기 위한 자료구조
//---------------------------------------------------

// 각 join selectivity 계산에 포함된 predicate의 연결정보를 위한 자료구조.
// 이는, 선택된 join method의 predicate을 쉽게 찾기 위함이다.
typedef struct qmoPredInfo
{
    qmoPredicate  * predicate;

    //-----------------------------------------------
    // 연결관련 자료구조
    // next : qmoPredicate.id가 다른 predicate의 연결
    // more : qmoPredicate.id가 같은 predicate의 연결
    //-----------------------------------------------
    qmoPredInfo   * next     ;
    qmoPredInfo   * more     ;
} qmoPredInfo;

// PROJ-1446 Host variable을 포함한 질의 최적화
// predicate들의 연결 정보를 관리한다.
// filter/key range/key filter/subquery filter를 분류한 결과 타입
typedef struct qmoPredWrapper
{
    qmoPredicate       * pred;
    qmoPredWrapper     * prev;
    qmoPredWrapper     * next;
} qmoPredWrapper;

typedef struct qmoPredWrapperPool
{
    iduVarMemList  * prepareMemory; //fix PROJ-1596
    iduMemory      * executeMemory; //fix PROJ-1436
    qmoPredWrapper   base[QMO_DEFAULT_WRAPPER_POOL_SIZE];
    qmoPredWrapper * current;
    UInt             used;
} qmoPredWrapperPool;

//---------------------------------------------------
// predicate type
//---------------------------------------------------

typedef enum
{
    QMO_KEYRANGE = 0,
    QMO_KEYFILTER,
    QMO_FILTER
} qmoPredType;

// nodeTransform 계열 함수들이 사용하는 구조체
typedef struct qmoPredTrans
{
    // qmoPredTrans를 생성할 때 필요한 멤버들
    qcStatement  * statement;

    // qmoPredTransI에서 내부적으로 사용하는 멤버들
    SChar          condition[3];
    SChar          logical[4];
    idBool         isGroupComparison;   // BUG-15816

    qmsQuerySet  * myQuerySet; // BUG-18242
} qmoPredTrans;

// qmoPredTrans 구조체를 manage하는 클래스
// 모두 static 함수들로 구성되어 있다.
class qmoPredTransI
{
public:

    static void initPredTrans( qmoPredTrans  * aPredTrans,
                               qcStatement   * aStatement,
                               qtcNode       * aCompareNode,
                               qmsQuerySet   * aQuerySet );

    static IDE_RC makeConditionNode( qmoPredTrans * aPredTrans,
                                     qtcNode      * aArgumentNode,
                                     qtcNode     ** aResultNode );

    static IDE_RC makeLogicalNode( qmoPredTrans * aPredTrans,
                                   qtcNode      * aArgumentNode,
                                   qtcNode     ** aResultNode );

    static IDE_RC makeAndNode( qmoPredTrans * aPredTrans,
                               qtcNode      * aArgumentNode,
                               qtcNode     ** aResultNode );

    static IDE_RC copyNode( qmoPredTrans * aPredTrans,
                            qtcNode      * aNode,
                            qtcNode     ** aResultNode );

    static IDE_RC makeSubQWrapperNode( qmoPredTrans * aPredTrans,
                                       qtcNode      * aValueNode,
                                       qtcNode     ** aResultNode );

    static IDE_RC makeIndirectNode( qmoPredTrans * aPredTrans,
                                    qtcNode      * aValueNode,
                                    qtcNode     ** aResultNode );

private:
    static IDE_RC makePredNode( qcStatement  * aStatement,
                                qtcNode      * aArgumentNode,
                                SChar        * aOperator,
                                qtcNode     ** aResultNode );

    static IDE_RC estimateConditionNode( qmoPredTrans * aPredTrans,
                                         qtcNode      * aConditionNode );

    static IDE_RC estimateLogicalNode( qmoPredTrans * aPredTrans,
                                       qtcNode      * aLogicalNode );

    static void setLogicalNCondition( qmoPredTrans * aPredTrans,
                                      qtcNode      * aCompareNode );
};

class qmoPredWrapperI
{
public:


    static IDE_RC initializeWrapperPool( iduVarMemList      * aMemory, //fix PROJ-1596
                                         qmoPredWrapperPool * aWrapperPool );

    static IDE_RC initializeWrapperPool( iduMemory          * aMemory,
                                         qmoPredWrapperPool * aWrapperPool );

    static IDE_RC createWrapperList( qmoPredicate       * aPredicate,
                                     qmoPredWrapperPool * aWrapperPool,
                                     qmoPredWrapper    ** aResult );

    static IDE_RC extractWrapper( qmoPredWrapper  * aTarget,
                                  qmoPredWrapper ** aFrom );

    static IDE_RC moveAll( qmoPredWrapper ** aFrom,
                           qmoPredWrapper ** aTo );

    static IDE_RC moveWrapper( qmoPredWrapper  * aTarget,
                               qmoPredWrapper ** aFrom,
                               qmoPredWrapper ** aTo );

    static IDE_RC addTo( qmoPredWrapper  * aTarget,
                         qmoPredWrapper ** aTo );

    static IDE_RC addPred( qmoPredicate       * aPredicate,
                           qmoPredWrapper    ** aWrapperList,
                           qmoPredWrapperPool * aWrapperPool );

private:

    static IDE_RC newWrapper( qmoPredWrapperPool * aWrapperPool,
                              qmoPredWrapper    ** aNewWrapper );
};

//---------------------------------------------------
// Predicate을 관리하기 위한 함수
//---------------------------------------------------
class qmoPred
{
public:

    //-----------------------------------------------
    // Graph 에서 호출하는 함수
    //-----------------------------------------------

    // PROJ-1502 PARTITIONED DISK TABLE
    // composite key와 관련된 flag를 predicate에 세팅함.
    // 동일컬럼리스트에 equal(IN) 연산자 존재여부 세팅
    // 연속적인 키 컬럼 사용가능여부 세팅
    static void setCompositeKeyUsableFlag( qmoPredicate * aPredicate );

    // selection graph의 predicate 재배치
    static IDE_RC  relocatePredicate( qcStatement     * aStatement,
                                      qmoPredicate    * aInPredicate,
                                      qcDepInfo       * aTableDependencies,
                                      qcDepInfo       * aOuterDependencies,
                                      qmoStatistics   * aStatiscalData,
                                      qmoPredicate   ** aOutPredicate );

    // partition graph의 predicate재배치
    static IDE_RC  relocatePredicate4PartTable(
        qcStatement        * aStatement,
        qmoPredicate       * aInPredicate,
        qcmPartitionMethod   aPartitionMethod,
        qcDepInfo          * aTableDependencies,
        qcDepInfo          * aOuterDependencies,
        qmoPredicate      ** aOutPredicate );

    // Join Predicate에 대한 분류
    static IDE_RC  classifyJoinPredicate( qcStatement  * aStatement,
                                          qmoPredicate * aPredicate,
                                          qmgGraph     * aLeftChildGraph,
                                          qmgGraph     * aRightChildGraph,
                                          qcDepInfo    * aFromDependencies );

    // Index Nested Loop Join의 push down joinable predicate를 생성.
    static IDE_RC  makeJoinPushDownPredicate( qcStatement    * aStatement,
                                              qmoPredicate   * aNewPredicate,
                                              qcDepInfo      * aRightDependencies );

    // Index Nested Loop Join의 push down non-joinable prediate
    static IDE_RC  makeNonJoinPushDownPredicate( qcStatement   * aStatement,
                                                 qmoPredicate  * aNewPredicate,
                                                 qcDepInfo     * aRightDependencies,
                                                 UInt            aDirection );

    // PROJ-1405
    // rownum predicate의 판단
    static IDE_RC  isRownumPredicate( qmoPredicate  * aPredicate,
                                      idBool        * aIsTrue );

    // constant predicate의 판단
    static IDE_RC  isConstantPredicate( qmoPredicate  * aPredicate,
                                        qcDepInfo     * aFromDependencies,
                                        idBool        * aIsTrue );

    // one table predicate의 판단
    static IDE_RC  isOneTablePredicate( qmoPredicate  * aPredicate,
                                        qcDepInfo     * aFromDependencies,
                                        qcDepInfo     * aTableDependencies,
                                        idBool        * aIsTrue );

    // To Fix PR-11460, PR-11461
    // Subquery Predicate에 대한 최적화 시 일반 Table만이
    // In Subquery KeyRange 와 Subquery KeyRange 최적화가 가능하다.
    // 이를 구분한다 [aTryKeyRange]

    // predicate에 존재하는 모든 subquery에 대한 처리
    static IDE_RC  optimizeSubqueries( qcStatement  * aStatement,
                                       qmoPredicate * aPredicate,
                                       idBool         aTryKeyRange );

    // qtcNode에 존재하는 모든 subquery에 대한 처리
    static IDE_RC  optimizeSubqueryInNode( qcStatement * aStatement,
                                           qtcNode     * aNode,
                                           idBool        aTryKeyRange,
                                           idBool        aConstantPred );

    // joinable predicate과 non joinable predicate을 분리한다.
    static IDE_RC  separateJoinPred( qmoPredicate  * aPredicate,
                                     qmoPredInfo   * aJoinablePredInfo,
                                     qmoPredicate ** aJoinPred,
                                     qmoPredicate ** aNonJoinPred );

    // Join predicate에 대한 columnID를 qmoPredicate.id에 저장.
    static IDE_RC  setColumnIDToJoinPred( qcStatement  * aStatement,
                                          qmoPredicate * aPredicate,
                                          qcDepInfo    * aDependencies );

    // Join predicate에 대한 column node를 반환.
    static qtcNode* getColumnNodeOfJoinPred( qcStatement  * aStatement,
                                             qmoPredicate * aPredicate,
                                             qcDepInfo    * aDependencies );
    
    // Predicate의 Indexable 여부 판단
    static IDE_RC  isIndexable( qcStatement   * aStatement,
                                qmoPredicate  * aPredicate,
                                qcDepInfo     * aTableDependencies,
                                qcDepInfo     * aOuterDependencies,
                                idBool        * aIsIndexable );

    static IDE_RC createPredicate( iduVarMemList * aMemory, //fix PROJ-1596
                                   qtcNode       * aNode,
                                   qmoPredicate ** aNewPredicate );

    // PROJ-1502 PARTITIONED DISK TABLE
    // Predicate의 Partition Prunable 여부 판단
    static IDE_RC  isPartitionPrunable(
        qcStatement        * aStatement,
        qmoPredicate       * aPredicate,
        qcmPartitionMethod   aPartitionMethod,
        qcDepInfo          * aTableDependencies,
        qcDepInfo          * aOuterDependencies,
        idBool             * aIsPartitionPrunable );

    // PROJ-1405 ROWNUM
    // rownum predicate을 stopkey predicate과 filter predicate로 분리한다.
    static IDE_RC  separateRownumPred( qcStatement   * aStatement,
                                       qmsQuerySet   * aQuerySet,
                                       qmoPredicate  * aPredicate,
                                       qmoPredicate ** aStopkeyPred,
                                       qmoPredicate ** aFilterPred,
                                       SLong         * aStopRecordCount );

    //-----------------------------------------------
    // Plan 에서 호출하는 함수
    //-----------------------------------------------

    // ROWNUM column의 판단
    static idBool  isROWNUMColumn( qmsQuerySet  * aQuerySet,
                                   qtcNode      * aNode );

    // prior column의 존재 판단
    static IDE_RC  setPriorNodeID( qcStatement  * aStatement,
                                   qmsQuerySet  * aQuerySet ,
                                   qtcNode      * aNode       );

    // keyRange, keyFilter로 추출된 qmoPredicate의 연결정보로,
    // 하나의 qtcNode의 그룹으로 연결정보를 만든다.
    static IDE_RC  linkPredicate( qcStatement   * aStatement,
                                  qmoPredicate  * aPredicate,
                                  qtcNode      ** aNode       );

    // filter로 추출된 qmoPredicate의 연결정보로,
    // 하나의 qtcNode의 그룹으로 연결정보를 만든다.
    static IDE_RC  linkFilterPredicate( qcStatement   * aStatement,
                                        qmoPredicate  * aPredicate,
                                        qtcNode      ** aNode      );

    // BUG-38971 subQuery filter 를 정렬할 필요가 있습니다.
    static IDE_RC  sortORNodeBySubQ( qcStatement   * aStatement,
                                     qtcNode       * aNode );

    // BUG-35155 Partial CNF
    // linkFilterPredicate 에서 만들어진 qtcNode 그룹에 NNF 필터를 추가한다.
    static IDE_RC addNNFFilter4linkedFilter( qcStatement   * aStatement,
                                             qtcNode       * aNNFFilter,
                                             qtcNode      ** aNode      );

    static IDE_RC makeRangeAndFilterPredicate( qcStatement   * aStatement,
                                               qmsQuerySet   * aQuerySet,
                                               idBool          aIsMemory,
                                               qmoPredicate  * aPredicate,
                                               qcmIndex      * aIndex,
                                               qmoPredicate ** aKeyRange,
                                               qmoPredicate ** aKeyFilter,
                                               qmoPredicate ** aFilter,
                                               qmoPredicate ** aLobFilter,
                                               qmoPredicate ** aSubqueryFilter );

    // keyRange, keyFilter, filter,
    // outerFilter, outerMtrFilter,
    // subqueryFilter 추출 함수
    static IDE_RC  extractRangeAndFilter( qcStatement        * aStatement,
                                          qcTemplate         * aTemplate,
                                          idBool               aIsMemory,
                                          idBool               aInExecutionTime,
                                          qcmIndex           * aIndex,
                                          qmoPredicate       * aPredicate,
                                          qmoPredWrapper    ** aKeyRange,
                                          qmoPredWrapper    ** aKeyFilter,
                                          qmoPredWrapper    ** aFilter,
                                          qmoPredWrapper    ** aLobFilter,
                                          qmoPredWrapper    ** aSubqueryFilter,
                                          qmoPredWrapperPool * aWrapperPool );

    // PROJ-1502 PARTITIONED DISK TABLE
    // partition key range predicate를 추출하고,
    // subquery filter predicate를 추출한다.
    // 나머지 predicate들도 따로 분류한다.
    static IDE_RC makePartKeyRangePredicate(
        qcStatement        * aStatement,
        qmsQuerySet        * aQuerySet,
        qmoPredicate       * aPredicate,
        qcmColumn          * aPartKeyColumns,
        qcmPartitionMethod   aPartitionMethod,
        qmoPredicate      ** aPartKeyRange );

    static IDE_RC makePartFilterPredicate(
        qcStatement        * aStatement,
        qmsQuerySet        * aQuerySet,
        qmoPredicate       * aPredicate,
        qcmColumn          * aPartKeyColumns,
        qcmPartitionMethod   aPartitionMethod,
        qmoPredicate      ** aPartFilter,
        qmoPredicate      ** aRemain,
        qmoPredicate      ** aSubqueryFilter );

    // extractRangeAndFilter 호출후 이 함후를 호출하면
    // 실제 predicate들이 재배치된다.
    static IDE_RC fixPredToRangeAndFilter( qcStatement        * aStatement,
                                           qmsQuerySet        * aQuerySet,
                                           qmoPredWrapper    ** aKeyRange,
                                           qmoPredWrapper    ** aKeyFilter,
                                           qmoPredWrapper    ** aFilter,
                                           qmoPredWrapper    ** aLobFilter,
                                           qmoPredWrapper    ** aSubqueryFilter,
                                           qmoPredWrapperPool * aWrapperPool );

    // columnID의 설정
    static IDE_RC    getColumnID( qcStatement  * aStatement,
                                  qtcNode      * aNode,
                                  idBool         aIsIndexable,
                                  UInt         * aColumnID );

    // LIST 컬럼리스트의 인덱스 사용여부 검사
    static IDE_RC    checkUsableIndex4List( qcTemplate   * aTemplate,
                                            qcmIndex     * aIndex,
                                            qmoPredicate * aPredicate,
                                            qmoPredType  * aPredType );

    // PROJ-1502 PARTITIONED DISK TABLE
    // LIST 컬럼리스트의 partition key 사용여부 검사
    static IDE_RC    checkUsablePartitionKey4List( qcStatement  * aStatement,
                                                   qcmColumn    * aPartKeyColumns,
                                                   qmoPredicate * aPredicate,
                                                   qmoPredType  * aPredType );


    // 비교연산자 각 하위 노드에 해당하는 graph를 찾는다.
    static IDE_RC findChildGraph( qtcNode   * aCompareNode,
                                  qcDepInfo * aFromDependencies,
                                  qmgGraph  * aGraph1,
                                  qmgGraph  * aGraph2,
                                  qmgGraph ** aLeftColumnGraph,
                                  qmgGraph ** aRightColumnGraph );

    // column과 value가 동일 계열의 data type인지를 판단
    static idBool isSameGroupType( qcTemplate  * aTemplate,
                                   qtcNode     * aColumn,
                                   qtcNode     * aValue );

    // PROJ-1446 Host variable을 포함한 질의 최적화
    // filter, key filter, key range를 생성하기 위해서
    // predicate이 필요한데, 이들이 만들어지는 과정에서
    // predicate 연결구조나 qtcNode들의 정보들이 바뀌기 때문에
    // filter등을 두번 생성할 수 없는 구조로 되어 있다.
    // 호스트 변수에 대해 filter등을 후보로 여러벌 만들 필요가 있는데
    // 이를 위해 graph의 predicate를 복사를 해서 사용해야 한다.
    static IDE_RC deepCopyPredicate( iduVarMemList * aMemory, //fix PROJ-1596
                                     qmoPredicate  * aSource,
                                     qmoPredicate ** aResult );

    // PROJ-1502 PARTITIONED DISK TABLE
    // 각 파티션 별로 최적화를 수행하기 위해 predicate를 복사한다.
    // 이 때, subquery는 제외하고 복사한다.
    // partitioned table의 노드를 partition의 노드로 변환한다.
    static IDE_RC copyPredicate4Partition(
        qcStatement   * aStatement,
        qmoPredicate  * aSource,
        qmoPredicate ** aResult,
        UShort          aSourceTable,
        UShort          aDestTable,
        idBool          aCopyVariablePred );

    // PROJ-1502 PARTITIONED DISK TABLE
    // 하나의 predicate를 복사한다.
    static IDE_RC copyOnePredicate4Partition(
        qcStatement   * aStatement,
        qmoPredicate  * aSource,
        qmoPredicate ** aResult,
        UShort          aSourceTable,
        UShort          aDestTable );

    // 실제적인 qmoPredicate 복사 작업을 수행한다.
    // 내부의 qtcNode도 복사한다.
    static IDE_RC copyOnePredicate( iduVarMemList * aMemory, //fix PROJ-1596
                                    qmoPredicate  * aSource,
                                    qmoPredicate ** aResult );

    // fix BUG-19211
    static IDE_RC copyOneConstPredicate( iduVarMemList * aMemory,
                                         qmoPredicate  * aSource,
                                         qmoPredicate ** aResult );

    // PROJ-1446 Host variable을 포함한 질의 최적화
    // 주어진 predicate의 next, more들을 탐색하면서,
    // host 최적화가 가능한지 검사한다.
    static idBool checkPredicateForHostOpt( qmoPredicate * aPredicate );

    // 컬럼별로 연결관계 만들기
    static IDE_RC    linkPred4ColumnID( qmoPredicate * aDestPred,
                                        qmoPredicate * aSourcePred );

    // 재배치가 완료된 각 indexable column에 대한 IN(subquery)에 대한 처리
    static IDE_RC  processIndexableInSubQ( qmoPredicate ** aPredicate );

    // PROJ-1404
    // predicate에서 생성된 transitive predicate을 제거한다.
    static IDE_RC  removeTransitivePredicate( qmoPredicate ** aPredicate,
                                              idBool          aOnlyJoinPred );

    // PROJ-1404
    // 생성된 transitive predicate과 논리적으로 중복된 predicate이 있는 경우
    // 삭제한다.
    static IDE_RC  removeEquivalentTransitivePredicate( qcStatement   * aStatement,
                                                        qmoPredicate ** aPredicate );

    // indexable predicate 판단시, 컬럼이 한쪽에만 존재하는지를 판단
    // (조건4의 판단)
    static IDE_RC isExistColumnOneSide( qcStatement * aStatement,
                                        qtcNode     * aNode,
                                        qcDepInfo   * aTableDependencies,
                                        idBool      * aIsIndexable );

    // BUG-39403
    static idBool hasOnlyColumnCompInPredList( qmoPredInfo * aJoinPredList );

    /* TASK-7219 */
    static IDE_RC isValidPushDownPredicate( qcStatement  * aStatement,
                                            qmgGraph     * aGraph,
                                            qmoPredicate * aPredicate,
                                            idBool       * aIsValid );

    /* TASK-7219 Non-shard DML */
    static void setOutRefColumnForQtcNode( qtcNode   * aNode,
                                           qcDepInfo * aPredicateDepInfo );
private:

    //-----------------------------------------------
    // Base Table에 대한 처리
    //-----------------------------------------------

    // Base Table의 Predicate에 대한 분류
    static IDE_RC  classifyTablePredicate( qcStatement   * aStatement,
                                           qmoPredicate  * aPredicate,
                                           qcDepInfo     * aTableDependencies,
                                           qcDepInfo     * aOuterDependencies,
                                           qmoStatistics * aStatiscalData );

    static IDE_RC  classifyPartTablePredicate( qcStatement      * aStatement,
                                               qmoPredicate     * aPredicate,
                                               qcmPartitionMethod aPartitionMethod,
                                               qcDepInfo        * aTableDependencies,
                                               qcDepInfo        * aOuterDependencies );


    // 하나의 비교연산자 노드에 대한 indexable 여부 판단
    static IDE_RC  isIndexableUnitPred( qcStatement * aStatement,
                                        qtcNode     * aNode,
                                        qcDepInfo   * aTableDependencies,
                                        qcDepInfo   * aOuterDependencies,
                                        idBool      * aIsIndexable );

    // PROJ-1502 PARTITIONED DISK TABLE
    // 하나의 비교연산자 노드에 대한 Partition Prunable 여부 판단
    static IDE_RC  isPartitionPrunableOnePred(
        qcStatement        * aStatement,
        qtcNode            * aNode,
        qcmPartitionMethod   aPartitionMethod,
        qcDepInfo          * aTableDependencies,
        qcDepInfo          * aOuterDependencies,
        idBool             * aIsPartitionPrunable );

    // indexable predicate 판단시,
    // column과 value가 동일계열에 속하는 data type인지를 검사
    // (조건5의 판단)
    static IDE_RC checkSameGroupType( qcStatement * aStatement,
                                      qtcNode     * aNode,
                                      idBool      * aIsIndexable );

    // indexable predicate 판단시, value에 대한 조건 검사 (조건6의 판단)
    static IDE_RC isIndexableValue( qcStatement * aStatement,
                                    qtcNode     * aNode,
                                    qcDepInfo   * aOuterDependencies,
                                    idBool      * aIsIndexable );

    // PROJ-1502 PARTITIONED DISK TABLE
    // partition prunable predicate 판단시, value에 대한 조건 검사 (조건6의 판단)
    static IDE_RC isPartitionPrunableValue( qcStatement * aStatement,
                                            qtcNode     * aNode,
                                            qcDepInfo   * aOuterDependencies,
                                            idBool      * aIsPartitionPrunable );

    // LIKE 연산자의 index 사용 가능 여부 판단
    static IDE_RC  isIndexableLIKE( qcStatement  * aStatement,
                                    qtcNode      * aNode,
                                    idBool       * aIsIndexable );

    //-----------------------------------------------
    // Join 에 대한 처리
    //-----------------------------------------------

    // joinable predicate의 판단
    static IDE_RC isJoinablePredicate( qmoPredicate * aPredicate,
                                       qcDepInfo    * aFromDependencies,
                                       qcDepInfo    * aLeftChildDependencies,
                                       qcDepInfo    * aRightChildDependencies,
                                       idBool       * aIsOnlyIndexNestedLoop );

    // 하나의 비교연산자에 대한 joinable predicate 판단
    static IDE_RC isJoinableOnePred( qtcNode     * aNode,
                                     qcDepInfo   * aFromDependencies,
                                     qcDepInfo   * aLeftChildDependencies,
                                     qcDepInfo   * aRightChildDependencies,
                                     idBool      * aIsJoinable );

    // indexable join predicate의 판단
    static IDE_RC  isIndexableJoinPredicate( qcStatement  * aStatement,
                                             qmoPredicate * aPredicate,
                                             qmgGraph     * aLeftChildGraph,
                                             qmgGraph     * aRightChildGraph );

    // sort joinable predicate의 판단 ( key size limit 검사 )
    static IDE_RC  isSortJoinablePredicate( qmoPredicate * aPredicate,
                                            qtcNode      * aNode,
                                            qcDepInfo    * aFromDependencies,
                                            qmgGraph     * aLeftChildGraph,
                                            qmgGraph     * aRightChildGraph );

    // hash joinable predicate의 판단 ( key size limit 검사 )
    static IDE_RC isHashJoinablePredicate( qmoPredicate * aPredicate,
                                           qtcNode      * aNode,
                                           qcDepInfo    * aFromDependencies,
                                           qmgGraph     * aLeftChildGraph,
                                           qmgGraph     * aRightChildGraph );

    // merge joinable predicate의 판단 ( 방향성 검사 )
    static IDE_RC isMergeJoinablePredicate( qmoPredicate * aPredicate,
                                            qtcNode      * aNode,
                                            qcDepInfo    * aFromDependencies,
                                            qmgGraph     * aLeftChildGraph,
                                            qmgGraph     * aRightChildGraph );

    //-----------------------------------------------
    // 기타
    //-----------------------------------------------

    // keyRange 추출
    static IDE_RC  extractKeyRange( qcmIndex        * aIndex,
                                    qmoPredWrapper ** aSource,
                                    qmoPredWrapper ** aKeyRange,
                                    qmoPredWrapper ** aKeyFilter,
                                    qmoPredWrapper ** aSubqueryFilter );

    // PROJ-1502 PARTITIONED DISK TABLE
    // partition keyrange 추출
    static IDE_RC  extractPartKeyRangePredicate(
        qcStatement         * aStatement,
        qmoPredicate        * aPredicate,
        qcmColumn           * aPartKeyColumns,
        qcmPartitionMethod    aPartitionMethod,
        qmoPredWrapper     ** aPartKeyRange,
        qmoPredWrapper     ** aRemain,
        qmoPredWrapper     ** aLobFilter,
        qmoPredWrapper     ** aSubqueryFilter,
        qmoPredWrapperPool  * aWrapperPool );

    // one column에 대한 keyRange 추출 함수
    static IDE_RC  extractKeyRange4Column( qcmIndex        * aIndex,
                                           qmoPredWrapper ** aSource,
                                           qmoPredWrapper ** aKeyRange );

    // PROJ-1502 PARTITIONED DISK TABLE
    // one column에 대한 partition keyRange 추출 함수
    static IDE_RC  extractPartKeyRange4Column( qcmColumn        * aPartKeyColumns,
                                               qcmPartitionMethod aPartitionMethod,
                                               qmoPredWrapper  ** aSource,
                                               qmoPredWrapper  ** aPartKeyRange );


    // keyFilter 추출
    static IDE_RC extractKeyFilter( idBool            aIsMemory,
                                    qcmIndex        * aIndex,
                                    qmoPredWrapper ** aSource,
                                    qmoPredWrapper ** aKeyRange,
                                    qmoPredWrapper ** aKeyFilter,
                                    qmoPredWrapper ** aFilter );

    // one column에 대한 keyRange, keyFilter, filter, subqueryFilter 추출 함수
    static IDE_RC  extractKeyFilter4Column( qcmIndex        * aIndex,
                                            qmoPredWrapper ** aSource,
                                            qmoPredWrapper ** aKeyFilter );

    // LIST에 대한 keyRange, keyFilter, filter, subqueryFilter 추출 함수
    static IDE_RC  extractRange4LIST( qcTemplate         * aTemplate,
                                      qcmIndex           * aIndex,
                                      qmoPredWrapper    ** aSouece,
                                      qmoPredWrapper    ** aKeyRange,
                                      qmoPredWrapper    ** aKeyFilter,
                                      qmoPredWrapper    ** aFilter,
                                      qmoPredWrapper    ** aSubQFilterList,
                                      qmoPredWrapperPool * aWrapperPool );

    // PROJ-1502 PARTITIONED DISK TABLE
    // LIST에 대한 keyRange, keyFilter, filter, subqueryFilter 추출 함수
    static IDE_RC  extractPartKeyRange4LIST( qcStatement        * aStatement,
                                             qcmColumn          * aPartKeyColumns,
                                             qmoPredWrapper    ** aSouece,
                                             qmoPredWrapper    ** aKeyRange,
                                             qmoPredWrapperPool * aWrapperPool );


    // keyRange와 keyFilter로 분류된 predicate에 대해서
    // quantify 비교연산자에 대한 노드변환, like비교연산자의 filter생성
    static IDE_RC  process4Range( qcStatement        * aStatement,
                                  qmsQuerySet        * aQuerySet,
                                  qmoPredicate       * aRange,
                                  qmoPredWrapper    ** aFilter,
                                  qmoPredWrapperPool * aWrapperPool );

    // qmoPredicate 리스트에서 filter subqueryFilter를 분리한다.
    static IDE_RC separateFilters( qcTemplate         * aTemplate,
                                   qmoPredWrapper     * aSource,
                                   qmoPredWrapper    ** aFilter,
                                   qmoPredWrapper    ** aLobFilter,
                                   qmoPredWrapper    ** aSubqueryFilter,
                                   qmoPredWrapperPool * aWrapperPool );

    // keyRange/keyFilter로 추출된 preidcate의 연결리스트 중 좌측최상단에
    // fixed/variable/InSubquery-keyRange에 대한 정보를 저장한다.
    static IDE_RC setRangeInfo( qmsQuerySet  * aQuerySet,
                                qmoPredicate * aPredicate );

    // To Fix PR-9679
    // LIKE predicate과 같이 Range를 생성하더라도
    // Filter가 필요한 경우에 대한 filter를 만든다.
    static IDE_RC makeFilterNeedPred( qcStatement   * aStatement,
                                      qmoPredicate  * aPredicate,
                                      qmoPredicate ** aLikeFilterPred );



    // keyRange 적용을 위한 노드 변환(quantify 비교연산자와 LIST)
    static IDE_RC nodeTransform( qcStatement  * aStatement,
                                 qtcNode      * aSourceNode,
                                 qtcNode     ** aTransformNode,
                                 qmsQuerySet  * aQuerySet );

    // OR 노드 하위 하나의 비교연산자 노드 단위로 노드 변환
    static IDE_RC nodeTransform4OneCond( qcStatement * aStatement,
                                         qtcNode     * aCompareNode,
                                         qtcNode    ** aTransformNode,
                                         qmsQuerySet * aQuerySet );

    // 하나의 list value node 변환
    static IDE_RC nodeTransform4List( qmoPredTrans * aPredTrans,
                                      qtcNode      * aColumnNode,
                                      qtcNode      * sValueNode,
                                      qtcNode     ** aTransformNode );

    // (i1, i2) in (1,2) 같은 경우
    static IDE_RC nodeTransformListColumnListValue( qmoPredTrans * aPredTrans,
                                                    qtcNode      * aColumnNode,
                                                    qtcNode      * aValueNode,
                                                    qtcNode     ** aTransformNode );

    // (i1, i2) in (1,2) 같은 경우
    static IDE_RC nodeTransformOneColumnListValue( qmoPredTrans * aPredTrans,
                                                   qtcNode      * aColumnNode,
                                                   qtcNode      * aValueNode,
                                                   qtcNode     ** aTransformNode );

    // 하나의 node에 대한 변환
    static IDE_RC nodeTransformOneNode( qmoPredTrans * aPredTrans,
                                        qtcNode      * aColumnNode,
                                        qtcNode      * aValueNode,
                                        qtcNode     ** aTransfromNode );

    // subquery list에 대한 변환
    static IDE_RC nodeTransform4OneRowSubQ( qmoPredTrans * aPredTrans,
                                            qtcNode      * aColumnNode,
                                            qtcNode      * aValueNode,
                                            qtcNode     ** aTransformNode );

    // value node가 SUBQUERY 인 경우의 노드 변환
    static IDE_RC nodeTransform4SubQ( qmoPredTrans * aPredTrans,
                                      qtcNode      * aCompareNode,
                                      qtcNode      * aColumnNode,
                                      qtcNode      * aValueNode,
                                      qtcNode     ** aTransfromNode );

    // indexArgument 설정
    static IDE_RC    setIndexArgument( qtcNode      * aNode,
                                       qcDepInfo    * aDependencies );

    // IN(subquery) 또는 subquery keyRange 최적화 팁 적용 predicate이
    // keyRange로 추출되지 못하고, filter로 빠지는 경우,
    // 설정되어 있는 IN(subquery)최적화 팁 정보를 삭제
    static IDE_RC   removeIndexableSubQTip( qtcNode     * aNode );

    // Filter에 대해 indexable 정보를 제거한다.
    static IDE_RC removeIndexableFromFilter( qmoPredWrapper * aFilter );

    // qmoPredWrapper의 연결 구조대로 qmoPredicate의 연결 정보를 재설정한다.
    static IDE_RC relinkPredicate( qmoPredWrapper * aWrapper );

    // more의 연결들을 끊는다.
    static IDE_RC removeMoreConnection( qmoPredWrapper * aWrapper,
                                        idBool aIfOnlyList );

    // 아래 세 함수는 deepCopyPredicate()로부터 파생된 서브 함수들이다.
    // linkToMore는 aLast의 more에 aNew를 달고,
    // linkToNext는 aLast의 next에 aNew를 단다.
    static void linkToMore( qmoPredicate **aHead,
                            qmoPredicate **aLast,
                            qmoPredicate *aNew );

    static void linkToNext( qmoPredicate **aHead,
                            qmoPredicate **aLast,
                            qmoPredicate *aNew );

    // PROJ-1405 ROWNUM
    static IDE_RC isStopkeyPredicate( qcStatement  * aStatement,
                                      qmsQuerySet  * aQuerySet,
                                      qtcNode      * aNode,
                                      idBool       * aIsStopkey,
                                      SLong        * aStopRecordCount );

    // BUG-39403
    static idBool hasOnlyColumnCompInPredNode( qtcNode * aNode );
};

#endif /* _O_QMO_PREDICATE_H_ */
