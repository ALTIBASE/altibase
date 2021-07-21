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
 * $Id: qmoSubquery.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *     Subquery Manager
 *
 *     질의 처리 중에 등장하는 Subquery에 대한 최적화를 수행한다.
 *     Subquery에 대하여 최적화를 통한 Graph 생성,
 *     Graph를 이용한 Plan Tree 생성을 담당한다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMO_SUBQUERY_H_
#define _O_QMO_SUBQUERY_H_ 1

#include <qc.h>
#include <qmoPredicate.h>
#include <qmgProjection.h>

//---------------------------------------------------
// Subquery를 관리하기 위한 자료 구조
//---------------------------------------------------

enum qmoSubqueryType
{
    QMO_TYPE_NONE = 0,
    QMO_TYPE_A,
    QMO_TYPE_N,
    QMO_TYPE_J,
    QMO_TYPE_JA
};


//---------------------------------------------------
// Subquery를 관리하기 위한 함수
//---------------------------------------------------

class qmoSubquery
{
public:

    //---------------------------------------------------
    // subquery에 대한 plan 생성
    //---------------------------------------------------

    // Subquery에 대한 Plan Tree 생성
    static IDE_RC    makePlan( qcStatement  * aStatement,
                               UShort         aTupleID,
                               qtcNode      * aNode );

    //---------------------------------------------------
    // 여러 구문에서의 subquery에 대한 최적화 팁 적용과 graph 생성
    //---------------------------------------------------

    //---------------------------------
    // where절 subquery에 대한 처리를 한다.
    // subquery 노드를 찾아서,
    // predicate형 또는 expresstion형에 대한 처리를 하게 된다.
    // predicate관리자가 호출
    //---------------------------------
    static IDE_RC    optimize( qcStatement   * aStatement,
                               qtcNode       * aNode,
                               idBool          aTryKeyRange );

    // Subquery에 대한 Graph 생성
    static IDE_RC    makeGraph( qcStatement  * aSubQStatement );

    //---------------------------------
    // Expression형 subquery에 대한 처리
    // qmo::optimizeInsert()에서 호출 :
    //      INSERT INTO T1 VALUES ( (SELECT SUM(I2) FROM T2) );
    // qmo::optimizeUpdate()에서 호출 :
    //      UPDATE SET I1=(SELECT SUM(I1) FROM T2) WHERE I1>1;
    //      UPDATE SET (I1,I2)=(SELECT I1,I2 FROM T2) WHERE I1>1;
    //---------------------------------
    // BUG-32854 모든 서브쿼리에 대해서 MakeGraph 한후에 MakePlan을 해야 한다.
    static IDE_RC    optimizeExprMakeGraph( qcStatement * aStatement,
                                            UInt          aTupleID,
                                            qtcNode     * aNode );

    static IDE_RC    optimizeExprMakePlan( qcStatement * aStatement,
                                           UInt          aTupleID,
                                           qtcNode     * aNode );
private:

    // Subquery type을 판단한다.
    static IDE_RC    checkSubqueryType( qcStatement     * aStatement,
                                        qtcNode         * aSubqueryNode,
                                        qmoSubqueryType * aType );

    // BUG-36575
    static void      checkSubqueryDependency( qcStatement * aStatement,
                                              qmsQuerySet * aQuerySet,
                                              qtcNode     * aSubQNode,
                                              idBool      * aExist );
    
    // select문에 대한 expression형 subquery 처리
    static IDE_RC     optimizeExpr4Select( qcStatement * aStatement,
                                           qtcNode     * aNode);

    // predicate형 subquery에 대한 subquery 최적화
    static IDE_RC    optimizePredicate( qcStatement * aStatement,
                                        qtcNode     * aCompareNode,
                                        qtcNode     * aNode,
                                        qtcNode     * aSubQNode,
                                        idBool        aTryKeyRange );

    // transform NJ 최적화 팁 적용
    static IDE_RC   transformNJ( qcStatement     * aStatement,
                                 qtcNode         * aCompareNode,
                                 qtcNode         * aNode,
                                 qtcNode         * aSubQNode,
                                 qmoSubqueryType   aSubQType,
                                 idBool            aSubqueryIsSet,
                                 idBool          * aIsTransform );

    // transform NJ 최적화 팁 적용시
    // predicate column과 subquery target column의 not null 검사
    static IDE_RC   checkNotNull( qcStatement  * aStatement,
                                  qtcNode      * aNode,
                                  qtcNode      * aSubQNode,
                                  idBool       * aIsNotNull );

    // transform NJ, store and search 최적화 팁 적용시
    // subquery target column에 대한 not null 검사
    static IDE_RC   checkNotNullSubQTarget( qcStatement * aStatement,
                                            qtcNode     * aSubQNode,
                                            idBool      * aIsNotNull );

    // transform NJ 최적화 팁 적용시
    // subquery target column에 인덱스가 존재하는지 검사
    static IDE_RC   checkIndex4SubQTarget( qcStatement * aStatement,
                                           qtcNode     * aSubQNode,
                                           idBool      * aIsExistIndex );

    // transform NJ 최적화 팁 적용시
    // 새로운 predicate을 만들어서, subquery의 기존 where절에 연결
    static IDE_RC   makeNewPredAndLink( qcStatement * aStatement,
                                        qtcNode     * aNode,
                                        qtcNode     * aSubQNode );

    // store and search 최적화 팁 적용
    static IDE_RC   storeAndSearch( qcStatement   * aStatement,
                                    qtcNode       * aCompareNode,
                                    qtcNode       * aNode,
                                    qtcNode       * aSubQNode,
                                    qmoSubqueryType aSubQType,
                                    idBool          aSubqueryIsSet );

    // store and search 최적화 팁 적용시
    // IN(=ANY), NOT IN(!=ALL)에 대한 저장방식 지정
    static IDE_RC   setStoreFlagIN( qcStatement * aStatement,
                                    qtcNode     * aCompareNode,
                                    qtcNode     * aNode,
                                    qtcNode     * aSubQNode,
                                    qmsQuerySet * aQuerySet,
                                    qmgPROJ     * sPROJ,
                                    idBool        aSubqueryIsSet );

    // store and search 최적화 팁 적용시
    // =ALL, !=ANY에 대한 저장방식 지정
    static IDE_RC   setStoreFlagEqualAll( qcStatement * aStatement,
                                          qtcNode     * aNode,
                                          qtcNode     * aSubQNode,
                                          qmsQuerySet * aQuerySet,
                                          qmgPROJ     * aPROJ,
                                          idBool        aSubqueryIsSet );

    // IN절의 subquery keyRange 최적화 팁 적용
    static IDE_RC   inSubqueryKeyRange( qtcNode        * aCompareNode,
                                        qtcNode        * aSubQNode,
                                        qmoSubqueryType  aSubQType );

    // subquery keyRange 최적화 팁 적용
    static IDE_RC   subqueryKeyRange( qtcNode        * aCompareNode,
                                      qtcNode        * aSubQNode,
                                      qmoSubqueryType  aSubQType );

    // Expression형 constant subquery 최적화 팁 적용
    static IDE_RC   constantSubquery( qcStatement * aStatement,
                                      qtcNode     * aSubQNode );

    // store and search, IN subquery keyRange, transformNJ 판단시 필요한
    // column cardinality를 구한다.
    static IDE_RC   getColumnCardinality( qcStatement * aStatement,
                                          qtcNode     * aColumnNode,
                                          SDouble     * aCardinality );

    // store and search, IN subquery keyRange, transformNJ 판단시 필요한
    // subquery Target cardinality를 구한다.
    static IDE_RC   getSubQTargetCardinality( qcStatement * aStatement,
                                              qtcNode     * aSubQNode,
                                              SDouble     * aCardinality );
};

#endif /* _O_QMO_SUBQUERY_H_ */
