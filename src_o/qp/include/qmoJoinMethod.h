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
 * $Id: qmoJoinMethod.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *    Join Cost를 구하여 다양한 Join Method들 중에서 가장 cost 가 좋은
 *    Join Method를 선택한다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMO_JOIN_METHOD_H_
#define _O_QMO_JOIN_METHOD_H_ 1

#include <qc.h>
#include <qcm.h>
#include <qmgDef.h>
#include <qmoDef.h>
#include <qmoSelectivity.h>

/***********************************************************************
 * [qmoJoinMethodCost의 flag을 위한 상수]
 ***********************************************************************/

// qmoJoinMethodCost.flag
#define QMO_JOIN_METHOD_FLAG_CLEAR             (0x00000000)

// Join Method Type : qmoHint의 QMO_JOIN_METHOD_MASK 와 동일

// qmoJoinMethodCost.flag
// Feasibility : 해당 Join Method 실행 가능성
#define QMO_JOIN_METHOD_FEASIBILITY_MASK       (0x00010000)
#define QMO_JOIN_METHOD_FEASIBILITY_FALSE      (0x00010000)
#define QMO_JOIN_METHOD_FEASIBILITY_TRUE       (0x00000000)

// qmoJoinMethodCost.flag
// Join Direction : Left->Right 또는 Right->Left
#define QMO_JOIN_METHOD_DIRECTION_MASK         (0x00020000)
#define QMO_JOIN_METHOD_DIRECTION_LEFTRIGHT    (0x00000000)
#define QMO_JOIN_METHOD_DIRECTION_RIGHTLEFT    (0x00020000)

// qmoJoinMethodCost.flag
// DISK/MEMORY : 하위 노드 생성시 disk인지, memory인지에 대한 정보
// Two-Pass Sort/Hash Join을 위한 Temp Table의 종류
#define QMO_JOIN_METHOD_LEFT_STORAGE_MASK         (0x00040000)
#define QMO_JOIN_METHOD_LEFT_STORAGE_DISK         (0x00000000)
#define QMO_JOIN_METHOD_LEFT_STORAGE_MEMORY       (0x00040000)

// qmoJoinMethodCost.flag
#define QMO_JOIN_METHOD_RIGHT_STORAGE_MASK        (0x00080000)
#define QMO_JOIN_METHOD_RIGHT_STORAGE_DISK        (0x00000000)
#define QMO_JOIN_METHOD_RIGHT_STORAGE_MEMORY      (0x00080000)

// qmoJoinMethodCost.flag
// left preserved order를 사용
#define QMO_JOIN_METHOD_USE_LEFT_PRES_ORDER_MASK        (0x00100000)
#define QMO_JOIN_METHOD_USE_LEFT_PRES_ORDER_FALSE       (0x00000000)
#define QMO_JOIN_METHOD_USE_LEFT_PRES_ORDER_TRUE        (0x00100000)

// qmoJoinMethodCost.flag
// right preserved order를 사용
#define QMO_JOIN_METHOD_USE_RIGHT_PRES_ORDER_MASK        (0x00200000)
#define QMO_JOIN_METHOD_USE_RIGHT_PRES_ORDER_FALSE       (0x00000000)
#define QMO_JOIN_METHOD_USE_RIGHT_PRES_ORDER_TRUE        (0x00200000)

// qmoJoinMethodCost.flag
// SORT 또는 MERGE JOIN 수행시 Left Node의 종류 결정
// NONE       : Left Node를 생성하지 말것.
// STORE      : Left Node로 SORT노드를 생성하고 Sorting은 하지 않음
// SORTING    : Left Node로 SORT노드를 생성하고 Sorting Option
#define QMO_JOIN_LEFT_NODE_MASK           (0x00C00000)
#define QMO_JOIN_LEFT_NODE_NONE           (0x00000000)
#define QMO_JOIN_LEFT_NODE_STORE          (0x00400000)
#define QMO_JOIN_LEFT_NODE_SORTING        (0x00800000)

// qmoJoinMethodCost.flag
// SORT 또는 MERGE JOIN 수행시 Right Node의 종류 결정
#define QMO_JOIN_RIGHT_NODE_MASK           (0x03000000)
#define QMO_JOIN_RIGHT_NODE_NONE           (0x00000000)
#define QMO_JOIN_RIGHT_NODE_STORE          (0x01000000)
#define QMO_JOIN_RIGHT_NODE_SORTING        (0x02000000)

// qmoJoinMethodCost.flag
// BUG-37407 semi, anti join cost
#define QMO_JOIN_SEMI_MASK                 (0x04000000)
#define QMO_JOIN_SEMI_FALSE                (0x00000000)
#define QMO_JOIN_SEMI_TRUE                 (0x04000000)

// qmoJoinMethodCost.flag
#define QMO_JOIN_ANTI_MASK                 (0x08000000)
#define QMO_JOIN_ANTI_FALSE                (0x00000000)
#define QMO_JOIN_ANTI_TRUE                 (0x08000000)

// To Fix PR-11212
// Index Merge Join 여부의 판단을 위한 MASK
#define QMO_JOIN_INDEX_MERGE_MASK       ( QMO_JOIN_METHOD_MASK |        \
                                          QMO_JOIN_LEFT_NODE_MASK |     \
                                          QMO_JOIN_RIGHT_NODE_MASK )
#define QMO_JOIN_INDEX_MERGE_TRUE       ( QMO_JOIN_METHOD_MERGE |       \
                                          QMO_JOIN_LEFT_NODE_NONE |     \
                                          QMO_JOIN_RIGHT_NODE_NONE )

// PROJ-2242
#define QMO_FIRST_ROWS_SET( aLeft , aJoinMethodCost ) \
    { (aLeft)->costInfo.outputRecordCnt *= (aJoinMethodCost)->firstRowsFactor; \
      (aLeft)->costInfo.totalAccessCost *= (aJoinMethodCost)->firstRowsFactor; \
      (aLeft)->costInfo.totalDiskCost   *= (aJoinMethodCost)->firstRowsFactor; \
      (aLeft)->costInfo.totalAllCost    *= (aJoinMethodCost)->firstRowsFactor; \
    }

#define QMO_FIRST_ROWS_UNSET( aLeft , aJoinMethodCost ) \
    { (aLeft)->costInfo.outputRecordCnt /= (aJoinMethodCost)->firstRowsFactor; \
      (aLeft)->costInfo.totalAccessCost /= (aJoinMethodCost)->firstRowsFactor; \
      (aLeft)->costInfo.totalDiskCost   /= (aJoinMethodCost)->firstRowsFactor; \
      (aLeft)->costInfo.totalAllCost    /= (aJoinMethodCost)->firstRowsFactor; \
    }

// BUG-40022
#define QMO_JOIN_DISABLE_NL       (0x00000001)
#define QMO_JOIN_DISABLE_HASH     (0x00000002)
#define QMO_JOIN_DISABLE_SORT     (0x00000004)
#define QMO_JOIN_DISABLE_MERGE    (0x00000008)

//---------------------------------------------------
// join 수행 방향에 관한 정보
//---------------------------------------------------
typedef enum
{
    QMO_JOIN_DIRECTION_LEFT_RIGHT = 0,
    QMO_JOIN_DIRECTION_RIGHT_LEFT

} qmoJoinDirection;

//---------------------------------------------------------------------
// PROJ-2418 
// Join에서, 한 Child Graph가 다른 Child로 외부 참조하는지 여부
//---------------------------------------------------------------------
typedef enum
{
    QMO_JOIN_LATERAL_NONE = 0,   // 한 Graph가 다른 쪽을 참조하지 않음 
    QMO_JOIN_LATERAL_LEFT,       // 왼쪽 Graph가 Lateral View, 오른쪽을 참조
    QMO_JOIN_LATERAL_RIGHT       // 오른쪽 Graph가 Lateral View, 왼쪽을 참조
} qmoJoinLateralDirection;

//---------------------------------------
// Index Nested Loop Join 계산시, 입력인자로 받을 자료구조 정의.
//---------------------------------------
typedef struct qmoJoinIndexNL
{
    // selectivity를 계산하기위한 index정보
    qcmIndex         * index;

    //-----------------------------------------------
    // [predicate 정보]
    // aPredicate      : join predicate의 연결리스트
    // aRightChildPred : join index 활용의 최적화 적용을 위한
    //                   right child graph의 predicate 정보
    //-----------------------------------------------
    qmoPredicate     * predicate;
    qmoPredicate     * rightChildPred;

    // right child graph의 dependencies 정보
    qcDepInfo        * rightDepInfo;
    // 통계정보 자료구조
    qmoStatistics    * rightStatiscalData;

    // left->right or right->left의 join 수행방향.
    qmoJoinDirection   direction;

} qmoJoinIndexNL;

//---------------------------------------
// Anti Outer Join 계산시, 입력인자로 받을 자료구조 정의.
//---------------------------------------
typedef struct qmoJoinAntiOuter
{
    // selectivity를 계산하기위한 index정보 (right graph index)
    qcmIndex         * index;

    // join predicate의 연결리스트
    qmoPredicate     * predicate;

    // right child graph의 dependencies 정보
    qcDepInfo        * rightDepInfo;

    //-----------------------------------------------
    // left graph의 인덱스 정보
    // predicate 하위의 모든컬럼에 인덱스가 존재하는지를 검사하기 위한 정보
    //-----------------------------------------------
    UInt               leftIndexCnt;
    qmoIdxCardInfo   * leftIndexInfo;

} qmoJoinAntiOuter;

//----------------------------------------------------
// Join Graph의 Left인지 Right인지의 구분
//----------------------------------------------------
typedef enum
{
    QMO_JOIN_CHILD_LEFT = 0,
    QMO_JOIN_CHILD_RIGHT
} qmoJoinChild;

/***********************************************************************
 * [Join Method Cost 를 구하기 위한 자료 구조]
 ***********************************************************************/

//----------------------------------------------------
// qmoJoinMethodCost 자료 구조
//----------------------------------------------------
typedef struct qmoJoinMethodCost
{
    // Join Method Type, Feasibility, Join Direction, 하위 DISK/MEMORY 정보
    UInt            flag;
    SDouble         selectivity;
    SDouble         firstRowsFactor;
    qmoPredInfo   * joinPredicate;    // joinable Predicate 정보

    //---------------------------------------------
    // 비용 정보
    //---------------------------------------------

    SDouble         accessCost;
    SDouble         diskCost;
    SDouble         totalCost;


    //---------------------------------------------
    // Index 정보 : selectivity 계산 시에 필요함
    //    - Index Nested Loop Join, Anti Outer Join일 경우에만 사용
    //    - 그 외의 경우에는 NULL 값을 가진다.
    //---------------------------------------------

    qmoIdxCardInfo  * rightIdxInfo;
    qmoIdxCardInfo  * leftIdxInfo;

    // two pass hash join hint에 temp table 개수가 존재하는 경우에만 설정
    UInt            hashTmpTblCntHint;

} qmoJoinMethodCost;

//---------------------------------------------------
// Join Method 자료 구조
//
//    - flag : QMO_JOIN_METHOD_NL
//             QMO_JOIN_METHOD_HASH
//             QMO_JOIN_METHOD_SORT
//             QMO_JOIN_METHOD_MERGE
//    - selectedJoinMethod : 해당 joinMethodCost 중
//             가장 cost가 좋은 JoinMethodCost
//    - joinMethodCnt : 해당 Join Method 개수
//    - joinMethodCost : joinMethod와 그의 cost
//
//---------------------------------------------------
typedef struct qmoJoinMethod
{
    UInt                flag;

    //---------------------------------------------
    // 선택된 Join Method
    //    - selectedJoinMethod : 가장 cost가 좋은 join method
    //    - hintJoinMethod : Join Method Hint에서 table order까지 만족하는
    //                       Join Method
    //    - hintJoinMethod2 : Join Method Hint에서 Method만 만족하는
    //                        Join Method
    //---------------------------------------------

    qmoJoinMethodCost * selectedJoinMethod;
    qmoJoinMethodCost * hintJoinMethod;
    qmoJoinMethodCost * hintJoinMethod2;

    UInt                joinMethodCnt;
    qmoJoinMethodCost * joinMethodCost;
} qmoJoinMethod;


//---------------------------------------------------
// Join Method를 관리하는 함수
//---------------------------------------------------

class qmoJoinMethodMgr
{
public:

    // Join Method 자료 구조의 초기화
    static IDE_RC init( qcStatement    * aStatement,
                        qmgGraph       * aGraph,
                        SDouble          aFirstRowsFactor,
                        qmoPredicate   * aJoinPredicate,
                        UInt             aJoinMethodType,
                        qmoJoinMethod  * aJoinMethod );

    // Join Method 선택
    static IDE_RC getBestJoinMethodCost( qcStatement   * aStatement,
                                         qmgGraph      * aGraph,
                                         qmoPredicate  * aJoinPredicate,
                                         qmoJoinMethod * aJoinMethod );

    // Graph의 공통 정보를 출력함.
    static IDE_RC     printJoinMethod( qmoJoinMethod * aMethod,
                                       ULong           aDepth,
                                       iduVarString  * aString );

private:
    // Join Method의 Cost 계산
    static IDE_RC getJoinCost( qcStatement       * aStatement,
                               qmoJoinMethodCost * aMethod,
                               qmoPredicate      * aJoinPredicate,
                               qmgGraph          * aLeft,
                               qmgGraph          * aRight);


    // Full Nested Loop Join의 Cost 계산
    static IDE_RC getFullNestedLoop( qcStatement       * aStatement,
                                     qmoJoinMethodCost * aMethod,
                                     qmoPredicate      * aJoinPredicate,
                                     qmgGraph          * aLeft,
                                     qmgGraph          * aRight);

    // Full Store Nested Loop Join의 Cost 계산
    static IDE_RC getFullStoreNestedLoop( qcStatement       * aStatement,
                                          qmoJoinMethodCost * aMethod,
                                          qmoPredicate      * aJoinPredicate,
                                          qmgGraph          * aLeft,
                                          qmgGraph          * aRight);

    // Index Nested Loop Join의 Cost 계산
    static IDE_RC getIndexNestedLoop( qcStatement       * aStatement,
                                      qmoJoinMethodCost * aMethod,
                                      qmoPredicate      * aJoinPredicate,
                                      qmgGraph          * aLeft,
                                      qmgGraph          * aRight);

    // Anti Outer Join의 Cost 계산
    static IDE_RC getAntiOuter( qcStatement       * aStatement,
                                qmoJoinMethodCost * aMethod,
                                qmoPredicate      * aJoinPredicate,
                                qmgGraph          * aLeft,
                                qmgGraph          * aRight);

    // One Pass Sort Join의 Cost 계산
    static IDE_RC getOnePassSort( qcStatement       * aStatement,
                                  qmoJoinMethodCost * aMethod,
                                  qmoPredicate      * aJoinPredicate,
                                  qmgGraph          * aLeft,
                                  qmgGraph          * aRight);

    // Two Pass Sort Join의 Cost 계산
    static IDE_RC getTwoPassSort( qcStatement       * aStatement,
                                  qmoJoinMethodCost * aMethod,
                                  qmoPredicate      * aJoinPredicate,
                                  qmgGraph          * aLeft,
                                  qmgGraph          * aRight);

    // Inverse Sort Join의 Cost 계산
    static IDE_RC getInverseSort( qcStatement       * aStatement,
                                  qmoJoinMethodCost * aMethod,
                                  qmoPredicate      * aJoinPredicate,
                                  qmgGraph          * aLeft,
                                  qmgGraph          * aRight);

    // One Pass Hash Join의 Cost 계산
    static IDE_RC getOnePassHash( qcStatement       * aStatement,
                                  qmoJoinMethodCost * aMethod,
                                  qmoPredicate      * aJoinPredicate,
                                  qmgGraph          * aLeft,
                                  qmgGraph          * aRight);

    // Two Pass Hash Join의 Cost 계산
    static IDE_RC getTwoPassHash( qcStatement       * aStatement,
                                  qmoJoinMethodCost * aMethod,
                                  qmoPredicate      * aJoinPredicate,
                                  qmgGraph          * aLeft,
                                  qmgGraph          * aRight);

    // Inverse Hash Join의 Cost 계산
    static IDE_RC getInverseHash( qcStatement       * aStatement,
                                  qmoJoinMethodCost * aMethod,
                                  qmoPredicate      * aJoinPredicate,
                                  qmgGraph          * aLeft,
                                  qmgGraph          * aRight);

    // Merge Join의 Cost 계산
    static IDE_RC getMerge( qcStatement       * aStatement,
                            qmoJoinMethodCost * aMethod,
                            qmoPredicate      * aJoinPredicate,
                            qmgGraph          * aLeft,
                            qmgGraph          * aRight);

    // Nested Loop Join Method Cost의 초기화
    static IDE_RC initJoinMethodCost4NL( qcStatement             * aStatement,
                                         qmgGraph                * aGraph,
                                         qmoPredicate            * aJoinPredicate,
                                         qmoJoinLateralDirection   aLateralDirection,
                                         qmoJoinMethodCost      ** aMethod,
                                         UInt                    * aMethodCnt );

    // Hash Join Method Cost의 초기화
    static IDE_RC initJoinMethodCost4Hash( qcStatement             * aStatement,
                                           qmgGraph                * aGraph,
                                           qmoPredicate            * aJoinPredicate,
                                           qmoJoinLateralDirection   aLateralDirection,
                                           qmoJoinMethodCost      ** aMethod,
                                           UInt                    * aMethodCnt );

    // Sort Join Method Cost의 초기화
    static IDE_RC initJoinMethodCost4Sort( qcStatement             * aStatement,
                                           qmgGraph                * aGraph,
                                           qmoPredicate            * aJoinPredicate,
                                           qmoJoinLateralDirection   aLateralDirection,
                                           qmoJoinMethodCost      ** aMethod,
                                           UInt                    * aMethodCnt );


    // Merget Join의 Join Method Cost의 초기화
    static IDE_RC initJoinMethodCost4Merge(qcStatement             * aStatement,
                                           qmgGraph                * aGraph,
                                           qmoPredicate            * aJoinPredicate,
                                           qmoJoinLateralDirection   aLateralDirection,
                                           qmoJoinMethodCost      ** aMethod,
                                           UInt                    * aMethodCnt );

    // Join Method Cost에  Join Method Type과 direction 정보를 설정하는 함수
    static IDE_RC setFlagInfo( qmoJoinMethodCost * aJoinMethodCost,
                               UInt                aFirstTypeFlag,
                               UInt                aJoinMethodCnt,
                               idBool              aIsLeftOuter );

    // merge join에서 preserved order 사용 가능 여부를 알아냄
    static IDE_RC usablePreservedOrder4Merge( qcStatement    * aStatement,
                                              qmoPredInfo    * aJoinPred,
                                              qmgGraph       * aGraph,
                                              qmoAccessMethod ** aAccessMethod,
                                              idBool         * aUsable );

    // Join Method Hint에 맞는 qmoJoinMethodCost를 찾음
    static IDE_RC setJoinMethodHint( qmoJoinMethod      * aJoinMethod,
                                     qmsJoinMethodHints * aJoinMethodHints,
                                     qmgGraph           * aGraph,
                                     UInt                 aNoUseHintMask,
                                     qmoJoinMethodCost ** aSameTableOrder,
                                     qmoJoinMethodCost ** aDiffTableOrder );

    // Sort Join등을 위하여 Preserved Order를 사용 가능한지 검사
    static IDE_RC canUsePreservedOrder( qcStatement       * aStatement,
                                        qmoJoinMethodCost * aJoinMethodCost,
                                        qmgGraph          * aGraph,
                                        qmoJoinChild        aChildPosition,
                                        idBool            * aUsable );

    // full nested loop, full store nested loop join의 사용여부 검사
    static IDE_RC usableJoinMethodFullNL( qmgGraph     * aGraph,
                                          qmoPredicate * aJoinPredicate,
                                          idBool       * aIsUsable );

    // push_pred hints에 따른 join order 변경
    static IDE_RC forcePushPredHint( qmgGraph      * aGraph,
                                     qmoJoinMethod * aJoinMethod );


    // PROJ-2582 recursive with
    // recursive view에 따른 join order 변경
    static IDE_RC forceJoinOrder4RecursiveView( qmgGraph      * aGraph,
                                                qmoJoinMethod * aJoinMethod );

    /* PROJ-2242 */

    // Nested loop join 관련한 qmoPredInfo list 획득
    static IDE_RC setJoinPredInfo4NL( qcStatement         * aStatement,
                                      qmoPredicate        * aJoinPredicate,
                                      qmoIdxCardInfo      * aRightIndexInfo,
                                      qmgGraph            * aRightGraph,
                                      qmoJoinMethodCost   * aMethodCost );

    // Anti outer 관련한 qmoPredInfo list 획득
    static IDE_RC setJoinPredInfo4AntiOuter( qcStatement       * aStatement,
                                             qmoPredicate      * aJoinPredicate,
                                             qmoIdxCardInfo    * aRightIndexInfo,
                                             qmgGraph          * aRightGraph,
                                             qmgGraph          * aLeftGraph,
                                             qmoJoinMethodCost * aMethodCost );

    // Hash join 관련한 qmoPredInfo list 획득
    static IDE_RC setJoinPredInfo4Hash( qcStatement       * aStatement,
                                        qmoPredicate      * aJoinPredicate,
                                        qmoJoinMethodCost * aMethodCost );

    // Sort join 관련한 qmoPredInfo list 획득
    static IDE_RC setJoinPredInfo4Sort( qcStatement       * aStatement,
                                        qmoPredicate      * aJoinPredicate,
                                        qcDepInfo         * aRightDepInfo,
                                        qmoJoinMethodCost * aMethodCost );

    // Merge join 관련한 qmoPredInfo 획득 (one predicate)
    static IDE_RC setJoinPredInfo4Merge( qcStatement         * aStatement,
                                         qmoPredicate        * aJoinPredicate,
                                         qmoJoinMethodCost   * aMethodCost );

    // Index nested loop join 관련한 qmoPredInfo list 반환
    static IDE_RC getIndexNLPredInfo( qcStatement      * aStatement,
                                      qmoJoinIndexNL   * aIndexNLInfo,
                                      qmoPredInfo     ** aJoinPredInfo );

    // Anti outer 가능한 qmoPredInfo list 반환
    static IDE_RC getAntiOuterPredInfo( qcStatement      * aStatement,
                                        UInt               aMethodCount,
                                        qmoAccessMethod  * aAccessMethod,
                                        qmoJoinAntiOuter * aAntiOuterInfo,
                                        qmoPredInfo     ** aJoinPredInfo,
                                        qmoIdxCardInfo  ** aLeftIdxInfo );

    // Anti outer 가능한 qmoPredicate 검증 후 반환
    // cost 가 가장 좋은 left qmoAccessMethod[].method 반환
    static IDE_RC getAntiOuterPred( qcStatement      * aStatement,
                                    UInt               aKeyColCnt,
                                    UInt               aMethodCount,
                                    qmoAccessMethod  * aAccessMethod,
                                    qmoPredicate     * aPredicate,
                                    qmoPredicate    ** aAntiOuterPred,
                                    qmoIdxCardInfo  ** aSelectedLeftIdxInfo );

    // getIndexNLPredInfo, getAntiOuterPredInfo 수행시
    // 필요한 임시정보 설정 ( columnID, indexArgument )
    static IDE_RC setTempInfo( qcStatement  * aStatement,
                               qmoPredicate * aPredicate,
                               qcDepInfo    * aRightDepInfo,
                               UInt           aDirection );

    // setJoinPredInfo4Sort 수행시,
    // 현재 sort column과 동일한 컬럼이 존재하는지 검사
    static IDE_RC findEqualSortColumn( qcStatement  * aStatement,
                                       qmoPredicate * aPredicate,
                                       qcDepInfo    * aRightDepInfo,
                                       qtcNode      * aSortColumn,
                                       qmoPredInfo  * aPredInfo );
    // BUG-42429
    // index_NL 조인시 index cost 를 다시 계산 하는 함수
    static void adjustIndexCost( qcStatement     * aStatement,
                                 qmgGraph        * aRight,
                                 qmoIdxCardInfo  * aIndexCardInfo,
                                 qmoPredInfo     * aJoinPredInfo,
                                 SDouble         * aMemCost,
                                 SDouble         * aDiskCost );

    // PROJ-2418 
    // Join Graph의 Lateral Position 반환
    static IDE_RC getJoinLateralDirection( qmgGraph                * aJoinGraph,
                                           qmoJoinLateralDirection * aLateralDirection );
};


#endif /* _O_QMO_JOIN_METHOD_H_ */
