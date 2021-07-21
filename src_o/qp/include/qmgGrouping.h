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
 * $Id: qmgGrouping.h 88575 2020-09-15 02:33:04Z ahra.cho $
 *
 * Description :
 *     Grouping Graph를 위한 정의
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMG_GROUPING_H_
#define _O_QMG_GROUPING_H_ 1

#include <qc.h>
#include <qmgDef.h>
#include <qmoDef.h>

//---------------------------------------------------
// Grouping Graph의 Define 상수
//---------------------------------------------------

// qmgGROP.graph.flag
#define QMG_GROP_TYPE_MASK                      (0x10000000)
#define QMG_GROP_TYPE_GENERAL                   (0x00000000)
#define QMG_GROP_TYPE_NESTED                    (0x10000000)

// qmgGROP.graph.flag
#define QMG_GROP_OPT_TIP_MASK                   (0x0F000000)
#define QMG_GROP_OPT_TIP_NONE                   (0x00000000)
#define QMG_GROP_OPT_TIP_INDEXABLE_GROUPBY      (0x01000000)
#define QMG_GROP_OPT_TIP_INDEXABLE_DISTINCTAGG  (0x02000000)
#define QMG_GROP_OPT_TIP_COUNT_STAR             (0x03000000)
#define QMG_GROP_OPT_TIP_INDEXABLE_MINMAX       (0x04000000)
#define QMG_GROP_OPT_TIP_ROLLUP                 (0x05000000)
#define QMG_GROP_OPT_TIP_CUBE                   (0x06000000)
#define QMG_GROP_OPT_TIP_GROUPING_SETS          (0x07000000)

//---------------------------------------------------
// Grouping Graph 를 관리하기 위한 자료 구조
//---------------------------------------------------

typedef struct qmgGROP
{
    qmgGraph           graph;         // 공통 Graph 정보

    //--------------------------------------------------
    // Grouping Graph 전용 정보
    //    - agg : aggregation 정보 
    //      일반 Grouping인 경우, aggsDepth1로 설정
    //      Nested Grouping인 경우, aggsDepth2로 설정
    //    - group : group by 정보
    //    - having : having 정보
    //    - distAggArg : Aggregation의 argument의 bucket count 정보
    //      (이 경우, Sort based grouping으로만 수행 가능)
    //
    //--------------------------------------------------

    qmsAggNode       * aggregation;
    qmsConcatElement * groupBy;
    qtcNode          * having;
    qmoPredicate     * havingPred;
    qmoDistAggArg    * distAggArg;    
    
    UInt               hashBucketCnt; // Hash Based Grouping인 경우, 설정
    
} qmgGROP;

//---------------------------------------------------
// Grouping Graph 를 관리하기 위한 함수
//---------------------------------------------------

class qmgGrouping
{
public:
    // Graph 의 초기화
    static IDE_RC  init( qcStatement * aStatement,
                         qmsQuerySet * aQuerySet,
                         qmgGraph    * aChildGraph,
                         idBool        aIsNested, 
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
    static IDE_RC makeChildPlan( qcStatement * aStatement,
                                 qmgGROP     * aMyGraph );

    static IDE_RC makePRLQAndChildPlan( qcStatement * aStatement,
                                        qmgGROP     * aMyGraph );

    static IDE_RC makeHashGroup( qcStatement * aStatement,
                                 qmgGROP     * aMyGraph );

    static IDE_RC makeSortGroup( qcStatement * aStatement,
                                 qmgGROP     * aMyGraph );

    static IDE_RC makeCountAll( qcStatement * aStatement,
                                qmgGROP     * aMyGraph );

    static IDE_RC makeIndexMinMax( qcStatement * aStatement,
                                   qmgGROP     * aMyGraph );

    static IDE_RC makeIndexDistAggr( qcStatement * aStatement,
                                     qmgGROP     * aMyGraph );

    // PROJ-1353 Rollup Plan 생성
    static IDE_RC makeRollup( qcStatement    * aStatement,
                              qmgGROP        * aMyGraph );

    // PROJ-1353 Cube Plan 생성
    static IDE_RC makeCube( qcStatement    * aStatement,
                            qmgGROP        * aMyGraph );

    // Grouping Method의 결정(GROUP BY 에 대한 최적화를 수행)
    static IDE_RC setGroupingMethod( qcStatement        * aStatement,
                                     qmgGROP            * aGroupGraph,
                                     SDouble              aRecordSize,
                                     SDouble              aAggrCost );

    // GROUP BY 컬럼 정보를 이용하여Order 자료 구조를 구축한다.
    static IDE_RC makeGroupByOrder( qcStatement        * aStatement,
                                    qmsConcatElement   * aGroupBy,
                                    qmgPreservedOrder ** sNewGroupByOrder );
                                  
    // indexable group by 최적화
    static IDE_RC indexableGroupBy( qcStatement        * aStatement,
                                    qmgGROP            * aGroupGraph,
                                    idBool             * aIndexableGroupBy );

    // count(*) 최적화
    static IDE_RC countStar( qcStatement      * aStatement,
                             qmgGROP          * aGroupGraph,
                             idBool           * aCountStar );
    
    
    // GROUP BY가 없은 경우의 optimize tip을 적용
    static IDE_RC nonGroupOptTip( qcStatement * aStatement,
                                  qmgGROP     * aGroupGraph,
                                  SDouble       aRecordSize,
                                  SDouble       aAggrCost );

    // group column들의 bucket count를 구함
    static IDE_RC getBucketCnt4Group( qcStatement  * aStatement,
                                      qmgGROP      * aGroupGraph,
                                      UInt           aHintBucketCnt,
                                      UInt         * aBucketCnt );

    // Preserved Order 방식을 사용하였을 경우의 비용 계산
    static IDE_RC getCostByPrevOrder( qcStatement      * aStatement,
                                      qmgGROP          * aGroupGraph,
                                      SDouble          * aAccessCost,
                                      SDouble          * aDiskCost,
                                      SDouble          * aTotalCost );

    /* PROJ-1353 Group Extension Preserved Order의 결정 ( ROLLUP ) */
    static IDE_RC setPreOrderGroupExtension( qcStatement * aStatement,
                                             qmgGROP     * aGroupGraph,
                                             SDouble       aRecordSize );

    static idBool checkParallelEnable( qmgGROP * aMyGraph );

    /* BUG-48132 */
    static void getPropertyGroupingMethod( qcStatement        * aStatement,
                                           qmoGroupMethodType * aGroupingMethod );     
};

#endif /* _O_QMG_GROUPING_H_ */

