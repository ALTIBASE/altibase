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
 * $Id: qmgProjection.h 90577 2021-04-13 05:07:51Z jayce.park $
 *
 * Description :
 *     Projection Graph를 위한 정의
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMG_PROJECTION_H_
#define _O_QMG_PROJECTION_H_ 1

#include <qc.h>
#include <qmgDef.h>

//---------------------------------------------------
// Projection Graph의 Define 상수
//---------------------------------------------------

// qmgPROJ.graph.flag
// 통신 버퍼에 쓸 PROJ NODE 생성 여부
#define QMG_PROJ_COMMUNICATION_TOP_PROJ_MASK      (0x10000000)
#define QMG_PROJ_COMMUNICATION_TOP_PROJ_FALSE     (0x00000000)
#define QMG_PROJ_COMMUNICATION_TOP_PROJ_TRUE      (0x10000000)

// qmgPROJ.graph.flag
// View Optimization Type이 View Materialization인지 아닌지에 대한 정보
#define QMG_PROJ_VIEW_OPT_TIP_VMTR_MASK         (0x20000000)
#define QMG_PROJ_VIEW_OPT_TIP_VMTR_FALSE        (0x00000000)
#define QMG_PROJ_VIEW_OPT_TIP_VMTR_TRUE         (0x20000000)

// qmgPROJ.graph.flag
// PROJ-2462 ResultCache
#define QMG_PROJ_TOP_RESULT_CACHE_MASK          (0x40000000)
#define QMG_PROJ_TOP_RESULT_CACHE_FALSE         (0x00000000)
#define QMG_PROJ_TOP_RESULT_CACHE_TRUE          (0x40000000)

/* Proj-1715 */
#define QMG_PROJ_VIEW_OPT_TIP_CMTR_MASK         (0x80000000)
#define QMG_PROJ_VIEW_OPT_TIP_CMTR_FALSE        (0x00000000)
#define QMG_PROJ_VIEW_OPT_TIP_CMTR_TRUE         (0x80000000)

//---------------------------------------------------
// subquery 최적화 팁 적용 정보를 나타낼 flag 정보
//---------------------------------------------------

/* qmgPROJ.subqueryTipFlag                             */
//---------------------------------
// subquery 최적화 팁 적용 정보
//  1. _TIP_NONE : 적용된 subquery 최적화 팁이 없음
//  2. _TIP_TRANSFORMNJ  : transform NJ 적용
//  3. _TIP_STORENSEARCH : store and search 적용
//  4. _TIP_IN_KEYRANGE  : IN절의 subquery keyRange 적용
//  5. _TIP_KEYRANGE     : subquery keyRange 적용
//---------------------------------
# define QMG_PROJ_SUBQUERY_TIP_MASK                (0x00000007)
# define QMG_PROJ_SUBQUERY_TIP_NONE                (0x00000000)
# define QMG_PROJ_SUBQUERY_TIP_TRANSFORMNJ         (0x00000001)
# define QMG_PROJ_SUBQUERY_TIP_STORENSEARCH        (0x00000002)
# define QMG_PROJ_SUBQUERY_TIP_IN_KEYRANGE         (0x00000003)
# define QMG_PROJ_SUBQUERY_TIP_KEYRANGE            (0x00000004)

/* qmgPROJ.subqueryTipFlag                                        */
// store and search 최적화 팁이 적용되었을 경우의 저장방식지정
# define QMG_PROJ_SUBQUERY_STORENSEARCH_MASK       (0x00000070)
# define QMG_PROJ_SUBQUERY_STORENSEARCH_NONE       (0X00000000)
# define QMG_PROJ_SUBQUERY_STORENSEARCH_HASH       (0x00000010)
# define QMG_PROJ_SUBQUERY_STORENSEARCH_LMST       (0x00000020)
# define QMG_PROJ_SUBQUERY_STORENSEARCH_STOREONLY  (0x00000040)

/* qmgPROJ.subqueryTipFlag                                        */
//---------------------------------
// store and search가 HASH로 저장될 경우, not null 검사 여부 지정.
// _HASH_NOTNULLCHECK_FALSE: 적용 컬럼에 대한 not null 검사를 하지 않아도 됨.
// _HASH_NOTNULLCHECK_TRUE : 적용 컬럼에 대한 not null 검사를 해야 됨.
//---------------------------------
# define QMG_PROJ_SUBQUERY_HASH_NOTNULLCHECK_MASK  (0x00000100)
# define QMG_PROJ_SUBQUERY_HASH_NOTNULLCHECK_FALSE (0x00000000)
# define QMG_PROJ_SUBQUERY_HASH_NOTNULLCHECK_TRUE  (0x00000100)

//---------------------------------------------------
// Projection Graph 를 관리하기 위한 자료 구조
//    - Top Graph 여부, indexable MIN MAX, VMTR 여부는 flag를 통해 알아냄
//---------------------------------------------------

typedef struct qmgPROJ
{
    qmgGraph   graph;    // 공통 Graph 정보

    qmsLimit  * limit;   // limit 정보
    qtcNode   * loopNode;// loop 정보
    qmsTarget * target;  // target 정보

    //-----------------------------------------------
    // subquery 최적화 팁 적용 정보를 위한 자료구조
    // subqueryTipFlag : subquery 최적화 팁 적용 정보
    // storeNSearchPred: store and search가 HASH로 수행될 경우에
    //                   plan tree에서 처리할 수 있도록
    //                   상위 predicate 정보를 달아준다.
    // hashBucketCnt   : store and search가 HASH로 수행될 경우 필요 
    //-----------------------------------------------
    
    UInt            subqueryTipFlag;
    qtcNode       * storeNSearchPred;
    UInt            hashBucketCnt;
} qmgPROJ;

//---------------------------------------------------
// Projection Graph 를 관리하기 위한 함수
//---------------------------------------------------

class qmgProjection
{
public:
    // Graph 의 초기화
    static IDE_RC  init( qcStatement * aStatement,
                         qmsQuerySet * aQuerySet,
                         qmgGraph    * aChildGraph,
                         qmgGraph   ** aGraph );

    // Graph의 최적화 수행
    static IDE_RC  optimize( qcStatement * aStatement, qmgGraph * aGraph );

    // Graph의 Plan Tree 생성
    static IDE_RC  makePlan( qcStatement * aStatement, const qmgGraph * aParent, qmgGraph * aGraph );

    // Graph의 공통 정보를 출력함.
    static IDE_RC  printGraph( qcStatement  * aStatement,
                               qmgGraph     * aGraph,
                               ULong          aDepth,
                               iduVarString * aString );

    static IDE_RC finalizePreservedOrder( qmgGraph * aGraph );

private:
    // Projection Graph의 preserved order를 생성한다.
    static IDE_RC makePreservedOrder( qcStatement       * aStatement,
                                      qmsTarget         * aTarget,
                                      qmgPreservedOrder * aChildPreservedOrder,
                                      qmgPreservedOrder ** aMyPreservedOrder );

    //-----------------------------------------
    // To Fix PR-7307
    // makePlan()을 위한 함수
    //-----------------------------------------

    static IDE_RC  makeChildPlan( qcStatement * aStatement,
                                  qmgPROJ     * aGraph );

    static IDE_RC  makePlan4Proj( qcStatement * aStatement,
                                  qmgPROJ     * aGraph,
                                  UInt          aPROJFlag );

    // View Materialization을 위한 Plan Tree 구성
    static IDE_RC  makePlan4ViewMtr( qcStatement    * aStatement,
                                     qmgPROJ        * aGraph,
                                     UInt             aPROJFlag );

    // Store And Search를 위한 Plan Tree 구성
    static IDE_RC  makePlan4StoreSearch( qcStatement    * aStatement,
                                         qmgPROJ        * aGraph,
                                         UInt             aPROJFlag );

    // In Subquery KeyRange를 위한 Plan Tree 구성
    static IDE_RC  makePlan4InKeyRange( qcStatement * aStatement,
                                        qmgPROJ     * aGraph,
                                        UInt          aPROJFlag );

    // Subquery KeyRange를 위한 Plan Tree 구성
    static IDE_RC  makePlan4KeyRange( qcStatement * aStatement,
                                      qmgPROJ     * aGraph,
                                      UInt          aPROJFlag );

    // BUG-48779 Projection의 limit을 scan으로 내려줄 수 있는지 확인한다.
    static void canPushDownLimitIntoScan( qmgGraph * aGraph,
                                          idBool   * aIsScanLimit );
};

#endif /* _O_QMG_PROJECTION_H_ */

