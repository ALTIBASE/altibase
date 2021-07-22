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
 * $Id: $
 *
 * Description : Selectivity Manager
 *
 *        - Unit predicate 에 대한 selectivity 계산
 *        - qmoPredicate 에 대한 selectivity 계산
 *        - qmoPredicate list 에 대한 통합 selectivity 계산
 *        - qmoPredicate wrapper list 에 대한 통합 selectivity 계산
 *        - 각 graph 에 대한 selectivity 계산
 *        - 각 graph 에 대한 output record count 계산
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <qmgDef.h>
#include <qmgSelection.h>
#include <qmgHierarchy.h>
#include <qmgCounting.h>
#include <qmgProjection.h>
#include <qmgGrouping.h>
#include <qmgJoin.h>
#include <qmgLeftOuter.h>
#include <qmgFullOuter.h>
#include <qmo.h>
#include <qmoPredicate.h>
#include <qmoDependency.h>
#include <qmoSelectivity.h>

extern mtfModule mtfEqual;
extern mtfModule mtfNotEqual;
extern mtfModule mtfIsNull;
extern mtfModule mtfIsNotNull;
extern mtfModule mtfGreaterThan;
extern mtfModule mtfGreaterEqual;
extern mtfModule mtfLessThan;
extern mtfModule mtfLessEqual;
extern mtfModule mtfBetween;
extern mtfModule mtfNotBetween;
extern mtfModule mtfLike;
extern mtfModule mtfNotLike;
extern mtfModule mtfEqualAny;
extern mtfModule mtfNotEqualAll;
extern mtfModule mtfDecrypt;
extern mtfModule mtfLnnvl;
extern mtdModule mtdDouble;
extern mtfModule stfEquals;
extern mtfModule stfNotEquals;

IDE_RC
qmoSelectivity::setMySelectivity( qcStatement   * aStatement,
                                  qmoStatistics * aStatInfo,
                                  qcDepInfo     * aDepInfo,
                                  qmoPredicate  * aPredicate )
{
/******************************************************************************
 *
 * Description : JOIN graph 제외한 qmoPredicate.mySelectivity 계산
 *
 * Implementation : 단위 predicate 에 대한 unit selectivity 를 획득하여
 *                  qmoPredicate.mySelectivity 에 세팅
 *
 *     1. qmoPredicate 이 복수 개의 unit predicate 로 구성되었을 경우
 *        ex) i1=1 or i1<1, t1.i1>=3 or t1.i2<=5
 *        S = 1-PRODUCT(1-USn)  (OR 의 확률계산식)
 *     2. qmoPredicate 이 한 개의 unit predicate 로 구성되었을 경우
 *        ex) i1=1, t1.i1>=t1.i2
 *        S = US(unit selectivity)
 *
 *****************************************************************************/

    SDouble    sSelectivity;
    SDouble    sUnitSelectivity;
    qtcNode  * sCompareNode;
    idBool     sIsDNF;

    IDU_FIT_POINT_FATAL( "qmoSelectivity::setMySelectivity::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aStatInfo  != NULL );
    IDE_DASSERT( aPredicate != NULL );

    //--------------------------------------
    // Selectivity 계산
    //--------------------------------------

    sIsDNF = ID_FALSE;

    if( ( aPredicate->node->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
        == MTC_NODE_LOGICAL_CONDITION_TRUE )
    {
        sCompareNode = (qtcNode *)(aPredicate->node->node.arguments);
    }
    else
    {
        sCompareNode = aPredicate->node;
        sIsDNF = ID_TRUE;
    }

    if( sCompareNode->node.next != NULL && sIsDNF == ID_FALSE )
    {
        // 1. OR 하위에 비교연산자가 여러개 있는 경우,
        //    OR 논리연산자에 대한 selectivity 계산.
        //    1 - (1-a)(1-b).....
        // ex) i1=1 or i1<1, t1.i1>=3 or t1.i2<=5

        sSelectivity = 1;

        while( sCompareNode != NULL )
        {
            IDE_TEST( getUnitSelectivity( QC_SHARED_TMPLATE(aStatement),
                                          aStatInfo,
                                          aDepInfo,
                                          aPredicate,
                                          sCompareNode,
                                          & sUnitSelectivity,
                                          ID_FALSE )
                      != IDE_SUCCESS );

            sSelectivity *= ( 1 - sUnitSelectivity );
            sCompareNode = (qtcNode *)(sCompareNode->node.next);
        }

        sSelectivity = ( 1 - sSelectivity );
    }
    else
    {
        // 2. OR 노드 하위에 비교연산자가 하나만 존재
        //    ex) i1=1, t1.i1>=t1.i2

        IDE_TEST( getUnitSelectivity( QC_SHARED_TMPLATE(aStatement),
                                      aStatInfo,
                                      aDepInfo,
                                      aPredicate,
                                      sCompareNode,
                                      & sSelectivity,
                                      ID_FALSE )
                  != IDE_SUCCESS );
    }

    aPredicate->mySelectivity = sSelectivity;

    IDE_DASSERT_MSG( sSelectivity >= 0 && sSelectivity <= 1,
                     "Selectivity : %"ID_DOUBLE_G_FMT"\n",
                     sSelectivity );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoSelectivity::setTotalSelectivity( qcStatement   * aStatement,
                                     qmoStatistics * aStatInfo,
                                     qmoPredicate  * aPredicate )
{
/******************************************************************************
 *
 * Description : qmoPredicate list 에 대한 재배치가 완료된 후
 *               qmoPredicate.id 별(동일 컬럼)로 more list 에 대해
 *               qmoPredicate.totalSelectivity(이하 TS) 설정
 *
 *   PROJ-1446 : more list 를 대상으로 한 qmoPredicate 중 하나라도
 *               QMO_PRED_HOST_OPTIMIZE_TRUE (호스트 변수를 포함) 이면
 *               more list 의 첫번째 predicate 에 QMO_PRED_HEAD_HOST_OPTIMIZE_TRUE
 *               세팅 (List나 non indexable일 경우 호스트 최적화 여부가 없음)
 *
 * Implementation :
 *
 *     1. LIST 또는 non-indexable : S = PRODUCT(MS)
 *     2. Indexable
 *        => more list 의 첫번째 predicate 에 QMO_PRED_HEAD_HOST_OPTIMIZE_TRUE 세팅
 *     2.1. 2개의 qmoPredicate : S = integrate selectivity
 *     2.2. Etc : S = PRODUCT(MS)
 *
 *****************************************************************************/

    SDouble        sTotalSelectivity;
    qmoPredicate * sPredicate;
    qmoPredicate * sMorePredicate;

    IDU_FIT_POINT_FATAL( "qmoSelectivity::setTotalSelectivity::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aStatInfo  != NULL );
    IDE_DASSERT( aPredicate != NULL );

    //--------------------------------------
    // 기본 초기화
    //--------------------------------------

    sTotalSelectivity = 1;
    sPredicate = aPredicate;

    sPredicate->flag &= ~QMO_PRED_HEAD_HOST_OPTIMIZE_MASK;
    sPredicate->flag |= QMO_PRED_HEAD_HOST_OPTIMIZE_FALSE;

    //--------------------------------------
    // 컬럼별 대표 selectivity 계산
    //--------------------------------------

    if( sPredicate->id == QMO_COLUMNID_NON_INDEXABLE ||
        sPredicate->id == QMO_COLUMNID_LIST )
    {
        sMorePredicate = sPredicate;
        while( sMorePredicate != NULL )
        {
            sTotalSelectivity *= sMorePredicate->mySelectivity;
            sMorePredicate = sMorePredicate->more;
        }
    }
    else
    {
        //--------------------------------------
        // PROJ-1446 : Host variable 을 포함한 질의 최적화
        // more list 를 대상으로 한 qmoPredicate 중 하나라도
        // QMO_PRED_HOST_OPTIMIZE_TRUE (호스트 변수를 포함) 이면
        // more list 의 첫번째 predicate 에 QMO_PRED_HEAD_HOST_OPTIMIZE_TRUE 세팅
        // (List나 non indexable일 경우 호스트 최적화 여부가 없음)
        //--------------------------------------

        sMorePredicate = sPredicate;
        while( sMorePredicate != NULL )
        {
            if( ( sMorePredicate->flag & QMO_PRED_HOST_OPTIMIZE_MASK )
                == QMO_PRED_HOST_OPTIMIZE_TRUE )
            {
                sPredicate->flag &= ~QMO_PRED_HEAD_HOST_OPTIMIZE_MASK;
                sPredicate->flag |= QMO_PRED_HEAD_HOST_OPTIMIZE_TRUE;
            }
            else
            {
                // Nothing to do.
            }
            sMorePredicate = sMorePredicate->more;
        }

        if( sPredicate->more != NULL &&
            sPredicate->more->more == NULL )
        {
            IDE_TEST( getIntegrateMySelectivity( QC_SHARED_TMPLATE(aStatement),
                                                 aStatInfo,
                                                 sPredicate,
                                                 & sTotalSelectivity,
                                                 ID_FALSE )
                      != IDE_SUCCESS );
        }
        else
        {
            sMorePredicate = sPredicate;
            while( sMorePredicate != NULL )
            {
                sTotalSelectivity *= sMorePredicate->mySelectivity;
                sMorePredicate = sMorePredicate->more;
            }
        }
    }

    // 첫번째 qmoPredicate에 total selectivity 저장
    aPredicate->totalSelectivity = sTotalSelectivity;

    IDE_DASSERT_MSG( sTotalSelectivity >= 0 && sTotalSelectivity <= 1,
                     "TotalSelectivity : %"ID_DOUBLE_G_FMT"\n",
                     sTotalSelectivity );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoSelectivity::recalculateSelectivity( qcTemplate    * aTemplate,
                                        qmoStatistics * aStatInfo,
                                        qcDepInfo     * aDepInfo,
                                        qmoPredicate  * aPredicate )
{
/******************************************************************************
 *
 * Description : Execution 단계(host 변수에 값이 바인딩된 후)에서
 *               qmo::optimizeForHost 호출시 scan method 재구축을 위해
 *               qmoPredicate 관련한 selectivity 를 재계산하고 Offset 에
 *               totalSelectivity 를 재설정한다.
 *
 * Implementation :
 *
 *            1. 호스트 변수를 포함하는 qmoPredicate more list
 *               - setMySelectivityOffset 계산
 *               - setTotalSelectivityOffset 계산
 *            2. 호스트 변수를 포함하지 않는 qmoPredicate
 *               - 다시 구할 필요 없음
 *
 *****************************************************************************/
 
    qmoPredicate     * sNextPredicate;
    qmoPredicate     * sMorePredicate;

    IDU_FIT_POINT_FATAL( "qmoSelectivity::recalculateSelectivity::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aTemplate  != NULL );
    IDE_DASSERT( aStatInfo  != NULL );
    IDE_DASSERT( aDepInfo   != NULL );
    IDE_DASSERT( aPredicate != NULL );

    //--------------------------------------
    // Selectivity 계산
    //--------------------------------------

    sNextPredicate = aPredicate;

    while( sNextPredicate != NULL )
    {
        if( ( sNextPredicate->flag & QMO_PRED_HEAD_HOST_OPTIMIZE_MASK )
            == QMO_PRED_HEAD_HOST_OPTIMIZE_TRUE )
        {
            for( sMorePredicate = sNextPredicate;
                 sMorePredicate != NULL;
                 sMorePredicate = sMorePredicate->more )
            {
                IDE_TEST( setMySelectivityOffset( aTemplate,
                                                  aStatInfo,
                                                  aDepInfo,
                                                  sMorePredicate )
                          != IDE_SUCCESS );
            }

            IDE_TEST( setTotalSelectivityOffset( aTemplate,
                                                 aStatInfo,
                                                 sNextPredicate )
                      != IDE_SUCCESS );
        }
        else
        {
            // more list 전체가 호스트 변수를 가지고 있지 않으므로
            // total selectivity를 다시 구할 필요가 없다.
            // Nothing to do.
        }

        sNextPredicate = sNextPredicate->next;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoSelectivity::getSelectivity4KeyRange( qcTemplate      * aTemplate,
                                         qmoStatistics   * aStatInfo,
                                         qmoIdxCardInfo  * aIdxCardInfo,
                                         qmoPredWrapper  * aKeyRange,
                                         SDouble         * aSelectivity,
                                         idBool            aInExecutionTime )
{
    UInt              sPredCnt;
    qmoPredWrapper  * sPredWrapper;
    qmoPredicate    * sPredicate;
    qtcNode         * sCompareNode;
    SDouble           sSelectivity;

    IDU_FIT_POINT_FATAL( "qmoSelectivity::getSelectivity4KeyRange::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aTemplate    != NULL );
    IDE_DASSERT( aStatInfo    != NULL );
    IDE_DASSERT( aIdxCardInfo != NULL );
    IDE_DASSERT( aSelectivity != NULL );

    for( sPredCnt = 0, sPredWrapper = aKeyRange;
         sPredWrapper != NULL;
         sPredCnt++, sPredWrapper = sPredWrapper->next )
    {
        sPredicate = sPredWrapper->pred;

        if( ( sPredicate->node->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
            == MTC_NODE_LOGICAL_CONDITION_TRUE )
        {
            sCompareNode = (qtcNode *)(sPredicate->node->node.arguments);
        }
        else
        {
            sCompareNode = sPredicate->node;
        }

        if( sCompareNode->node.next != NULL )
        {
            // or 조건이 여러개 붇어 있는 경우
            break;
        }

        if( sCompareNode->node.module != &mtfEqual )
        {
            // mtfEqual 이외에 다른 조건이 붇은 경우
            break;
        }
    }

    if( aStatInfo->isValidStat == ID_TRUE &&
        aIdxCardInfo->index->keyColCount == sPredCnt )
    {
        // BUG-48120
        if ( (QCU_OPTIMIZER_INDEX_COST_MODE & QMG_OPTI_INDEX_COST_MODE_1_MASK) == 1 )
        {
            aIdxCardInfo->flag &= ~QMO_STAT_CARD_ALL_EQUAL_IDX_MASK;
            aIdxCardInfo->flag |= QMO_STAT_CARD_ALL_EQUAL_IDX_TRUE;

            // scan method 비교를 위한 cost에서는 pred selectivity로 한다.
            IDE_TEST( getSelectivity4PredWrapper( aTemplate,
                                                  aKeyRange,
                                                  &sSelectivity,
                                                  aInExecutionTime )
                      != IDE_SUCCESS );
        }
        else
        {
            // BUG-48210 이전과 동일하게
            sSelectivity = 1 / aIdxCardInfo->KeyNDV;
        }
    }
    else
    {
        IDE_TEST( getSelectivity4PredWrapper( aTemplate,
                                              aKeyRange,
                                              & sSelectivity,
                                              aInExecutionTime )
                  != IDE_SUCCESS );
    }

    *aSelectivity = sSelectivity;

    IDE_DASSERT_MSG( sSelectivity >= 0 && sSelectivity <= 1,
                     "Selectivity : %"ID_DOUBLE_G_FMT"\n",
                     sSelectivity );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoSelectivity::getSelectivity4PredWrapper( qcTemplate      * aTemplate,
                                            qmoPredWrapper  * aPredWrapper,
                                            SDouble         * aSelectivity,
                                            idBool            aInExecutionTime )
{
/******************************************************************************
 *
 * Description : qmoPredWrapper list 에 대한 통합 selecltivity 반환
 *               (qmgSelection 의 access method 에 대한 selectivity 계산시 호출)
 *
 * Implementation : S = PRODUCT( Predicate selectivity for wrapper list )
 *
 *           1. qmoPredWrapper list 가 NULL 이면
 *              Predicate selectivity = 1
 *
 *           2. qmoPredWrapper list 가 NULL 이 아니면
 *           2.1. qmoPredicate 이 LIST 형태이면
 *                Predicate selectivity = MS(mySelectivity)
 *           2.2  qmoPredicate 이 LIST 형태가 아니면
 *                Predicate selectivity = TS(totalSelectivity)
 *
 *****************************************************************************/

    qmoPredWrapper  * sPredWrapper;
    SDouble           sSelectivity;

    IDU_FIT_POINT_FATAL( "qmoSelectivity::getSelectivity4PredWrapper::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aTemplate != NULL );
    IDE_DASSERT( aSelectivity != NULL );

    //--------------------------------------
    // Selectivity 계산
    //--------------------------------------

    sSelectivity = 1;
    sPredWrapper = aPredWrapper;

    while( sPredWrapper != NULL )
    {
        if( sPredWrapper->pred->id == QMO_COLUMNID_LIST )
        {
            sSelectivity *= getMySelectivity( aTemplate,
                                              sPredWrapper->pred,
                                              aInExecutionTime );
        }
        else
        {
            sSelectivity *= getTotalSelectivity( aTemplate,
                                                 sPredWrapper->pred,
                                                 aInExecutionTime );
        }

        sPredWrapper = sPredWrapper->next;
    }

    *aSelectivity = sSelectivity;

    IDE_DASSERT_MSG( sSelectivity >= 0 && sSelectivity <= 1,
                     "Selectivity : %"ID_DOUBLE_G_FMT"\n",
                     sSelectivity );

    return IDE_SUCCESS;
}


IDE_RC
qmoSelectivity::getTotalSelectivity4PredList( qcStatement  * aStatement,
                                              qmoPredicate * aPredList,
                                              SDouble      * aSelectivity,
                                              idBool         aInExecutionTime )
{
/******************************************************************************
 *
 * Description : qmgSelection, qmgHierarchy graph 의 qmoPredicate list 에 대한
 *               통합 selecltivity 반환
 *
 * Implementation : S = PRODUCT( totalSelectivity for qmoPredicate list )
 *
 *       cf) aInExecutionTime (qmo::optimizeForHost 참조)
 *           (aStatement->myPlan->scanDecisionFactors == NULL)
 *        1. qmgSelection
 *         - Prepare time : ID_FALSE
 *         - Execution time : ID_FALSE
 *        2. qmgHierarchy : 항상 ID_FALSE
 *
 *****************************************************************************/

    qcTemplate    * sTemplate;
    qmoPredicate  * sPredicate;
    SDouble         sSelectivity;

    IDU_FIT_POINT_FATAL( "qmoSelectivity::getTotalSelectivity4PredList::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement   != NULL );
    IDE_DASSERT( aSelectivity != NULL );

    sSelectivity = 1;
    sPredicate = aPredList;

    if( aInExecutionTime == ID_TRUE )
    {
        sTemplate = QC_PRIVATE_TMPLATE( aStatement );
    }
    else
    {
        sTemplate = QC_SHARED_TMPLATE( aStatement );
    }

    while( sPredicate != NULL )
    {
        sSelectivity *= getTotalSelectivity( sTemplate,
                                             sPredicate,
                                             aInExecutionTime );

        sPredicate = sPredicate->next;
    }

    *aSelectivity = sSelectivity;

    IDE_DASSERT_MSG( sSelectivity >= 0 && sSelectivity <= 1,
                     "Selectivity : %"ID_DOUBLE_G_FMT"\n",
                     sSelectivity );

    return IDE_SUCCESS;
}


IDE_RC
qmoSelectivity::setMySelectivity4Join( qcStatement  * aStatement,
                                       qmgGraph     * aGraph,
                                       qmoPredicate * aJoinPredList,
                                       idBool         aIsSetNext )
{
/******************************************************************************
 *
 * Description : Join predicate list 에 대해 qmoPredicate.mySelectivity 계산
 *
 * Implementation : join selectivity 계산 및
 *                  다음 인덱스 사용가능 여부 설정(QMO_PRED_NEXT_KEY_USABLE_MASK)
 *
 *     1. aIsSetNext 가 TRUE 일 경우 : qmoPredicate list 전체에 대해 수행
 *     => join predicate 으로 이미 분류
 *
 *     1.1. qmoPredicate 이 복수 개의 unit predicate 로 구성되었을 경우
 *          ex) t1.i1=t2.i1 or t1.i2=t2.i2, t1.i1>t2.i1 or t1.i2<t2.i2
 *          S = 1-PRODUCT(1-US)n    (OR 확률계산식)
 *
 *     1.2. qmoPredicate 이 한 개의 unit predicate 로 구성되었을 경우
 *          ex) t1.i1=t2.i1, t1.i1>t2.i1, t1.i2<t2.i2+1
 *          S = US (unit selectivity)
 *
 *     2. aIsSetNext 가 FALSE 일 경우 : qmoPredicate 하나에 대해 수행
 *     => Join order 결정시 graph::optimize 수행전이라 predicate 분류가 불완전,
 *        이 때 child graph 와 각 qmoPredicate 의 dependency 를 비교하여
 *        join 관련 predicate일 때도 본 함수를 호출한다.
 *        이 때는 하나의 qmoPredicat 에 대한 mySelectiity 만 구한다.
 *
 *        < mySelectivity 계산 시점 >
 *     1. baseGraph 의 경우 optimize 과정에서 최초로 계산
 *     2. qmoCnfMgr::joinOrdering 을 통해 생성된 qmgJOIN.graph 의 경우
 *      - 일부 qmoPredicate.mySelectivity 가 일차적으로 계산되고
 *        (ordered hint적용시 join group에 predicate연결을 하지 않기 때문)
 *      - qmgJoin::optimize 를 통해 관련 predicate 이 확정적으로
 *        재배치 된 후 제대로 계산된다.
 *
 *****************************************************************************/

    qmoPredicate   * sPredicate;
    qtcNode        * sCompareNode;
    SDouble          sSelectivity;
    SDouble          sUnitSelectivity;
    idBool           sIsDNF;
    idBool           sIsUsableNextKey;
    idBool           sIsEqual;

    IDU_FIT_POINT_FATAL( "qmoSelectivity::setMySelectivity4Join::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement != NULL );

    //--------------------------------------
    // Selectivity 계산
    // 다음인덱스 사용가능여부 설정
    //--------------------------------------

    sIsUsableNextKey = ID_TRUE;
    sPredicate = aJoinPredList;

    while( sPredicate != NULL )
    {
        sSelectivity = 1;
        sUnitSelectivity = 1;
        sIsDNF = ID_FALSE;

        if( ( sPredicate->node->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
            == MTC_NODE_LOGICAL_CONDITION_TRUE )
        {
            sCompareNode = (qtcNode *)(sPredicate->node->node.arguments);
        }
        else
        {
            sCompareNode = sPredicate->node;
            sIsDNF = ID_TRUE;
        }

        if( sCompareNode->node.next != NULL && sIsDNF == ID_FALSE )
        {
            // 1.1. OR 하위에 비교연산자가 여러개 있는 경우,
            //      OR 논리연산자에 대한 selectivity 계산.
            //      1 - (1-a)(1-b).....
            // ex) t1.i1=t2.i1 or t1.i2=t2.i2, t1.i1>t2.i1 or t1.i2<t2.i2

            while( sCompareNode != NULL )
            {
                IDE_TEST( getUnitSelectivity4Join( aStatement,
                                                   aGraph,
                                                   sCompareNode,
                                                   & sIsEqual,
                                                   & sUnitSelectivity )
                          != IDE_SUCCESS );

                sSelectivity *= ( 1 - sUnitSelectivity );

                if( sIsUsableNextKey == ID_TRUE )
                {
                    sIsUsableNextKey = sIsEqual;
                }
                else
                {
                    // Nothing To Do
                }

                sCompareNode = (qtcNode *)(sCompareNode->node.next);
            }

            sSelectivity = ( 1 - sSelectivity );
        }
        else
        {
            // 1.2. OR 노드 하위에 비교연산자가 하나만 존재
            // ex) t1.i1=t2.i1, t1.i1>t2.i1, t1.i2<t2.i2+1

            IDE_TEST( getUnitSelectivity4Join( aStatement,
                                               aGraph,
                                               sCompareNode,
                                               & sIsUsableNextKey,
                                               & sSelectivity )
                      != IDE_SUCCESS );
        }

        sPredicate->mySelectivity = sSelectivity;

        IDE_DASSERT_MSG( sSelectivity >= 0 && sSelectivity <= 1,
                         "Selectivity : %"ID_DOUBLE_G_FMT"\n",
                         sSelectivity );

        if( sIsUsableNextKey == ID_TRUE )
        {
            sPredicate->flag &= ~QMO_PRED_NEXT_KEY_USABLE_MASK;
            sPredicate->flag |= QMO_PRED_NEXT_KEY_USABLE;
        }
        else
        {
            sPredicate->flag &= ~QMO_PRED_NEXT_KEY_USABLE_MASK;
            sPredicate->flag |= QMO_PRED_NEXT_KEY_UNUSABLE;
        }

        if ( aIsSetNext == ID_FALSE )
        {
            // To Fix BUG-10542
            // 2. Predicate list 가 아닌 하나의 qmoPredicate 에 대해서만 구함
            break;
        }
        else
        {
            sPredicate = sPredicate->next;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoSelectivity::setMySelectivity4OneTable( qcStatement  * aStatement,
                                           qmgGraph     * aLeftGraph,
                                           qmgGraph     * aRightGraph,
                                           qmoPredicate * aOneTablePred )
{
/******************************************************************************
 *
 * Description : Outer join 의 onConditionCNF->oneTablePredicate 에 대한
 *               qmoPredicate.mySelectivity 계산
 *
 * Implementation :
 *
 *          Child graph 의 depInfo 를 고려하여
 *       1. 하위 노드 역시 join graph 일 경우 setMySelectivity4Join 수행
 *       2. 그 외 setMySelectivity 수행
 *
 *          < mySelectivity 계산 시점 >
 *       1. baseGraph 의 경우 optimize 과정에서 최초로 계산
 *       2. qmoCnfMgr::joinOrdering 을 통해 생성된 qmgJOIN.graph 의 경우
 *        - 일부 qmoPredicate.mySelectivity 가 일차적으로 계산되고
 *          (ordered hint적용시 join group에 predicate연결을 하지 않기 때문)
 *        - qmgJoin::optimize 를 통해 관련 predicate 이 확정적으로
 *          재배치 된 후 제대로 계산된다.
 *
 *****************************************************************************/

    qmoPredicate  * sOneTablePred;
    qcDepInfo       sDependencies;

    IDU_FIT_POINT_FATAL( "qmoSelectivity::setMySelectivity4OneTable::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement  != NULL );
    IDE_DASSERT( aLeftGraph  != NULL );
    IDE_DASSERT( aRightGraph != NULL );

    //--------------------------------------
    // mySelectivity 계산
    //--------------------------------------

    sOneTablePred = aOneTablePred;

    while( sOneTablePred != NULL )
    {
        sOneTablePred->id = QMO_COLUMNID_NON_INDEXABLE;

        // To Fix BUG-8742
        qtc::dependencyAnd( & sOneTablePred->node->depInfo,
                            & aLeftGraph->depInfo,
                            & sDependencies );

        //------------------------------------------
        // To Fix BUG-10542
        // outer join 계열 하위가 outer join일 경우,
        // outer join인 하위 graph의 one table predicate의 selectivity는
        // join predicate 이기 때문에 join selectivity 구하는 함수를 사용해서
        // selectivity를 구해야 함
        //------------------------------------------

        if ( qtc::dependencyEqual( & sDependencies,
                                   & qtc::zeroDependencies ) == ID_TRUE )
        {
            if ( aRightGraph->myFrom->joinType == QMS_NO_JOIN )
            {
                IDE_TEST( setMySelectivity( aStatement,
                                            aRightGraph->myFrom->tableRef->statInfo,
                                            & aRightGraph->depInfo,
                                            sOneTablePred )
                          != IDE_SUCCESS );
            }
            else
            {
                IDE_TEST( setMySelectivity4Join( aStatement,
                                                 aRightGraph,
                                                 sOneTablePred,
                                                 ID_FALSE )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            if ( aLeftGraph->myFrom->joinType == QMS_NO_JOIN )
            {
                IDE_TEST( setMySelectivity( aStatement,
                                            aLeftGraph->myFrom->tableRef->statInfo,
                                            & aLeftGraph->depInfo,
                                            sOneTablePred )
                          != IDE_SUCCESS );
            }
            else
            {
                IDE_TEST( setMySelectivity4Join( aStatement,
                                                 aLeftGraph,
                                                 sOneTablePred,
                                                 ID_FALSE )
                          != IDE_SUCCESS );
            }
        }

        sOneTablePred = sOneTablePred->next;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoSelectivity::setJoinSelectivity( qcStatement  * aStatement,
                                    qmgGraph     * aGraph,
                                    qmoPredicate * aJoinPred,
                                    SDouble      * aSelectivity )
{
/******************************************************************************
 *
 * Description : qmgJoin, qmgSemiJoin, qmgAntiJoin 에 대한 selectivity 계산
 *               (WHERE clause, ON clause 모두 고려된 상태)
 *
 * Implementation :
 *
 *       1. Join graph 관련 mySelectivity 계산
 *       2. graph->myPredicate list 에 대한 통합 selectivity 획득
 *            S = PRODUCT( mySelectivity for graph->myPredicate)
 *
 *      cf) mySelectivity 계산 시점
 *       1. baseGraph 의 경우 optimize 과정에서 최초로 계산
 *       2. qmoCnfMgr::joinOrdering 을 통해 생성된 qmgJOIN.graph 의 경우
 *        - 일부 qmoPredicate.mySelectivity 가 일차적으로 계산되고
 *          (ordered hint적용시 join group에 predicate연결을 하지 않기 때문)
 *        - qmgJoin::optimize 를 통해 관련 predicate 이 확정적으로
 *          재배치 된 후 제대로 계산된다.
 *
 *****************************************************************************/

    SDouble sSelectivity = 1;

    IDU_FIT_POINT_FATAL( "qmoSelectivity::setJoinSelectivity::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement   != NULL );
    IDE_DASSERT( aGraph       != NULL );
    IDE_DASSERT( aSelectivity != NULL );

    //--------------------------------------
    // 1. mySelectivity 계산
    //--------------------------------------

    IDE_TEST( setMySelectivity4Join( aStatement,
                                     aGraph,
                                     aJoinPred,
                                     ID_TRUE )
              != IDE_SUCCESS );

    //--------------------------------------
    // 2. 통합 selectivity 계산
    //--------------------------------------

    switch( aGraph->type )
    {
        case QMG_INNER_JOIN:
            IDE_TEST( getMySelectivity4PredList( aJoinPred,
                                                 & sSelectivity )
                      != IDE_SUCCESS );
            break;

        case QMG_SEMI_JOIN:
            IDE_TEST( getSemiJoinSelectivity( aStatement,
                                              aGraph,
                                              & aGraph->left->depInfo,
                                              aJoinPred,
                                              ID_TRUE,   // Left semi
                                              ID_TRUE,   // List
                                              & sSelectivity )
                      != IDE_SUCCESS );
            break;

        case QMG_ANTI_JOIN:
            IDE_TEST( getAntiJoinSelectivity( aStatement,
                                              aGraph,
                                              & aGraph->left->depInfo,
                                              aJoinPred,
                                              ID_TRUE,   // Left anti
                                              ID_TRUE,   // List
                                              & sSelectivity )
                      != IDE_SUCCESS );
            break;

        default:
            IDE_DASSERT(0);
            break;
    }

    *aSelectivity = sSelectivity;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoSelectivity::setOuterJoinSelectivity( qcStatement  * aStatement,
                                         qmgGraph     * aGraph,
                                         qmoPredicate * aWherePred,
                                         qmoPredicate * aOnJoinPred,
                                         qmoPredicate * aOneTablePred,
                                         SDouble      * aSelectivity )
{
/******************************************************************************
 *
 * Description : qmgLOJN, qmgFOJN 에 대한 selectivity 계산
 *
 * Implementation :
 *
 *     1. Predicate list 에 대한 mySelectivity 세팅
 *     2. Selectivity 계산 : S = PRODUCT(MS for on clause)
 *
 *****************************************************************************/

    SDouble sOnJoinSelectivity;
    SDouble sOneTableSelectivity;

    IDU_FIT_POINT_FATAL( "qmoSelectivity::setOuterJoinSelectivity::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement    != NULL );
    IDE_DASSERT( aGraph        != NULL );
    IDE_DASSERT( aGraph->left  != NULL );
    IDE_DASSERT( aGraph->right != NULL );
    IDE_DASSERT( aSelectivity  != NULL );

    //--------------------------------------
    // mySelectivity 세팅
    //--------------------------------------

    // WHERE 절의 join predicate 에 대한 mySelectivity
    IDE_TEST( setMySelectivity4Join( aStatement,
                                     aGraph,
                                     aWherePred,
                                     ID_TRUE )
              != IDE_SUCCESS );

    // ON 절의 join predicate 에 대한 mySelectivity
    IDE_TEST( setMySelectivity4Join( aStatement,
                                     aGraph,
                                     aOnJoinPred,
                                     ID_TRUE )
              != IDE_SUCCESS );

    // ON 절의 one table predicate 에 대한 mySelectivity
    IDE_TEST( setMySelectivity4OneTable( aStatement,
                                         aGraph->left,
                                         aGraph->right,
                                         aOneTablePred )
              != IDE_SUCCESS );

    //--------------------------------------
    // Selectivity 계산
    //--------------------------------------

    // ON 절에 대한 join selectivity 획득
    IDE_TEST( getMySelectivity4PredList( aOnJoinPred,
                                         & sOnJoinSelectivity )
              != IDE_SUCCESS );

    // ON 절에 대한 one table predicate selectivity 획득
    IDE_TEST( getMySelectivity4PredList( aOneTablePred,
                                         & sOneTableSelectivity )
              != IDE_SUCCESS );

    *aSelectivity = sOnJoinSelectivity * sOneTableSelectivity;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoSelectivity::setLeftOuterOutputCnt( qcStatement  * aStatement,
                                       qmgGraph     * aGraph,
                                       qcDepInfo    * aLeftDepInfo,
                                       qmoPredicate * aWherePred,
                                       qmoPredicate * aOnJoinPred,
                                       qmoPredicate * aOneTablePred,
                                       SDouble        aLeftOutputCnt,
                                       SDouble        aRightOutputCnt,
                                       SDouble      * aOutputRecordCnt )
{
/******************************************************************************
 *
 * Description : qmgLeftOuter 에 대한 output record count 계산
 *
 * Implementation : 계산식은 다음과 같다.
 *
 *    Output record count = Selectivity(where 절) * Input record count
 *                        = Selectivity(where 절)
 *                          * ( LeftAntiCnt(left anti join for on clause)
 *                            + OnJoinCnt(join for on clause) )
 *
 *  - LeftAntiCnt(anti join)
 *              = T(L) * Selectivity(left anti join for on clause)
 *              = T(L) * PRODUCT(left anti join selectivity for on clause)
 *  - OnJoinCnt(inner join)
 *              = T(L) * T(R) * Selectivity(join selectivity for on clause)
 *              = T(L) * T(R) * PRODUCT(MS for on clause)
 *
 *    cf) Left anti join selectivity = 1 - Right NDV / MAX(Left NDV, Right NDV)
 *        Join selectivity = 1 / MAX(Left NDV, Right NDV)
 *
 *****************************************************************************/

    SDouble   sWhereSelectivity;
    SDouble   sLeftAntiSelectivity;
    SDouble   sOnJoinSelectivity;
    SDouble   sOneTableSelectivity;
    SDouble   sLeftAntiCnt;
    SDouble   sOnJoinCnt;
    SDouble   sOutputRecordCnt;

    IDU_FIT_POINT_FATAL( "qmoSelectivity::setLeftOuterOutputCnt::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement       != NULL );
    IDE_DASSERT( aLeftDepInfo     != NULL );
    IDE_DASSERT( aOutputRecordCnt != NULL );
    IDE_DASSERT( aLeftOutputCnt  > 0 );
    IDE_DASSERT( aRightOutputCnt > 0 );

    //--------------------------------------
    // Selectivity 계산
    //--------------------------------------

    // WHERE 절에 대한 join selectivity 획득
    IDE_TEST( getMySelectivity4PredList( aWherePred,
                                         & sWhereSelectivity )
              != IDE_SUCCESS );

    // ON 절에 대한 join selectivity 획득
    IDE_TEST( getMySelectivity4PredList( aOnJoinPred,
                                         & sOnJoinSelectivity )
              != IDE_SUCCESS );

    // ON 절에 대한 one table predicate selectivity 획득
    IDE_TEST( getMySelectivity4PredList( aOneTablePred,
                                         & sOneTableSelectivity )
              != IDE_SUCCESS );

    // ON 절에 대한 left anti selectivity 획득
    IDE_TEST( getAntiJoinSelectivity( aStatement,
                                      aGraph,
                                      aLeftDepInfo,
                                      aOnJoinPred,
                                      ID_TRUE,   // Left anti
                                      ID_TRUE,   // List
                                      & sLeftAntiSelectivity )
              != IDE_SUCCESS );

    //--------------------------------------
    // Output record count 계산
    //--------------------------------------

    sLeftAntiCnt = aLeftOutputCnt * sLeftAntiSelectivity;
    sOnJoinCnt = aLeftOutputCnt * aRightOutputCnt *
        sOnJoinSelectivity * sOneTableSelectivity;

    sOutputRecordCnt = sWhereSelectivity * ( sLeftAntiCnt + sOnJoinCnt );

    // Output count 보정
    *aOutputRecordCnt = ( sOutputRecordCnt < 1 ) ? 1: sOutputRecordCnt;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoSelectivity::setFullOuterOutputCnt( qcStatement  * aStatement,
                                       qmgGraph     * aGraph,
                                       qcDepInfo    * aLeftDepInfo,
                                       qmoPredicate * aWherePred,
                                       qmoPredicate * aOnJoinPred,
                                       qmoPredicate * aOneTablePred,
                                       SDouble        aLeftOutputCnt,
                                       SDouble        aRightOutputCnt,
                                       SDouble      * aOutputRecordCnt )
{
/******************************************************************************
 *
 * Description : qmgFullOuter 에 대한 output record count 계산
 *
 * Implementation : 계산식은 다음과 같다.
 *
 *    Output record count = Selectivity(where 절) * Input record count
 *                        = Selectivity(where 절)
 *                          * ( LeftAntiCnt(left anti join for on clause)
 *                            + RightAntiCnt(right anti join for on clause)
 *                            + OnJoinCnt(join for on clause) )
 *
 *  - LeftAntiCnt(anti join)
 *              = T(L) * Selectivity(left anti join for on clause)
 *              = T(L) * PRODUCT( left anti join selectivity for on clause )
 *  - RightAntiCnt(anti join)
 *              = T(R) * Selectivity(right anti join for on clause)
 *              = T(R) * PRODUCT( right anti join selectivity for on clause )
 *  - OnJoinCnt(inner join)
 *              = T(L) * T(R) * Selectivity( join selectivity for on clause)
 *              = T(L) * T(R) * PRODUCT(MS for on clause )
 *
 *    cf) Left anti join selectivity = 1 - Right NDV / MAX( Left NDV, Right NDV )
 *        Right anti join selectivity = 1 - Left NDV / MAX( Left NDV, Right NDV )
 *        Join selectivity = 1 / MAX( Left NDV, Right NDV )
 *
 *****************************************************************************/

    SDouble   sWhereSelectivity;
    SDouble   sLeftAntiSelectivity;
    SDouble   sRightAntiSelectivity;
    SDouble   sOnJoinSelectivity;
    SDouble   sOneTableSelectivity;
    SDouble   sLeftAntiCnt;
    SDouble   sRightAntiCnt;
    SDouble   sOnJoinCnt;
    SDouble   sOutputRecordCnt;

    IDU_FIT_POINT_FATAL( "qmoSelectivity::setFullOuterOutputCnt::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement       != NULL );
    IDE_DASSERT( aLeftDepInfo     != NULL );
    IDE_DASSERT( aOutputRecordCnt != NULL );
    IDE_DASSERT( aLeftOutputCnt  > 0 );
    IDE_DASSERT( aRightOutputCnt > 0 );

    //--------------------------------------
    // Selectivity 계산
    //--------------------------------------

    // WHERE 절에 대한 join selectivity 획득
    IDE_TEST( getMySelectivity4PredList( aWherePred,
                                         & sWhereSelectivity )
              != IDE_SUCCESS );

    // ON 절에 대한 join selectivity 획득
    IDE_TEST( getMySelectivity4PredList( aOnJoinPred,
                                         & sOnJoinSelectivity )
              != IDE_SUCCESS );

    // ON 절에 대한 one table predicate selectivity 획득
    IDE_TEST( getMySelectivity4PredList( aOneTablePred,
                                         & sOneTableSelectivity )
              != IDE_SUCCESS );

    // ON 절에 대한 left anti selectivity 획득
    IDE_TEST( getAntiJoinSelectivity( aStatement,
                                      aGraph,
                                      aLeftDepInfo,
                                      aOnJoinPred,
                                      ID_TRUE,   // Left anti
                                      ID_TRUE,   // List
                                      & sLeftAntiSelectivity )
              != IDE_SUCCESS );

    // ON 절에 대한 right anti selectivity 획득
    IDE_TEST( getAntiJoinSelectivity( aStatement,
                                      aGraph,
                                      aLeftDepInfo,
                                      aOnJoinPred,
                                      ID_FALSE,  // Right anti
                                      ID_TRUE,   // List
                                      & sRightAntiSelectivity )
              != IDE_SUCCESS );

    //--------------------------------------
    // Output record count 계산
    //--------------------------------------

    sLeftAntiCnt = aLeftOutputCnt * sLeftAntiSelectivity;
    sRightAntiCnt = aRightOutputCnt * sRightAntiSelectivity;
    sOnJoinCnt = aLeftOutputCnt * aRightOutputCnt *
        sOnJoinSelectivity * sOneTableSelectivity;

    sOutputRecordCnt =
        ( sLeftAntiCnt + sRightAntiCnt + sOnJoinCnt ) * sWhereSelectivity;

    // Output count 보정
    *aOutputRecordCnt = ( sOutputRecordCnt < 1 ) ? 1: sOutputRecordCnt;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoSelectivity::setJoinOrderFactor( qcStatement  * aStatement,
                                    qmgGraph     * aJoinGraph,
                                    qmoPredicate * aJoinPred,
                                    SDouble      * aJoinOrderFactor,
                                    SDouble      * aJoinSize )
{
/******************************************************************************
 *
 * Description : Join ordering 을 위한 JoinSize 와 JoinOrderFactor 계산
 *
 * Implementation :
 *
 *    (1) getSelectivity4JoinOrder 를 통한 selectivity 획득
 *    (2) JoinSize
 *        - SCAN or PARTITION : [left output] * [right output] * Selectivity
 *        - Etc (join) : [left input] * [right input] * Selectivity
 *    (3) JoinOrderFactor :
 *        - SCAN or PARTITION : JoinSize / ( [left input] + [right input] )
 *        - Etc (join) : JoinSize / ( [left output] + [right output] )
 *
 *    cf) Outer join 의 경우 ON 절의 join predicate 만 고려
 *        => ON 절의 one table predicate 은 제외
 *
 *****************************************************************************/

    qmgGraph * sLeft;
    qmgGraph * sRight;
    SDouble    sInputRecordCnt;
    SDouble    sLeftOutput;
    SDouble    sRightOutput;
    SDouble    sSelectivity;
    SDouble    sJoinSize;
    SDouble    sJoinOrderFactor;

    IDU_FIT_POINT_FATAL( "qmoSelectivity::setJoinOrderFactor::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement       != NULL );
    IDE_DASSERT( aJoinGraph       != NULL );
    IDE_DASSERT( aJoinOrderFactor != NULL );
    IDE_DASSERT( aJoinSize        != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sLeft  = aJoinGraph->left;
    sRight = aJoinGraph->right;
    sSelectivity = 1.0;

    //------------------------------------------
    // Join Predicate의 selectivity 계산
    //------------------------------------------

    // To Fix PR-8266
    if ( aJoinPred != NULL )
    {
        IDE_TEST( getSelectivity4JoinOrder( aStatement,
                                            aJoinGraph,
                                            aJoinPred,
                                            & sSelectivity )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    //------------------------------------------
    // JoinSize 계산
    //------------------------------------------
    switch ( sLeft->type )
    {
        case QMG_SELECTION :
        case QMG_PARTITION :
        case QMG_SHARD_SELECT :    // PROJ-2638
            sLeftOutput     = sLeft->costInfo.inputRecordCnt;
            sInputRecordCnt = sLeft->costInfo.outputRecordCnt;
            break;

        case QMG_INNER_JOIN :
        case QMG_SEMI_JOIN :
        case QMG_ANTI_JOIN :
            sLeftOutput     = ((qmgJOIN*)sLeft)->joinSize;
            sInputRecordCnt = sLeftOutput;
            break;

        case QMG_LEFT_OUTER_JOIN :
            sLeftOutput     = ((qmgLOJN*)sLeft)->joinSize;
            sInputRecordCnt = sLeftOutput;
            break;

        case QMG_FULL_OUTER_JOIN :
            sLeftOutput     = ((qmgFOJN*)sLeft)->joinSize;
            sInputRecordCnt = sLeftOutput;
            break;

        default :
            IDE_DASSERT(0);
            IDE_TEST( IDE_FAILURE );
    }

    switch ( sRight->type )
    {
        case QMG_SELECTION :
        case QMG_PARTITION :
        case QMG_SHARD_SELECT :    // PROJ-2638
            sRightOutput     = sRight->costInfo.inputRecordCnt;
            sInputRecordCnt *= sRight->costInfo.outputRecordCnt;
            break;

        case QMG_INNER_JOIN :
        case QMG_SEMI_JOIN :
        case QMG_ANTI_JOIN :
            sRightOutput     = ((qmgJOIN*)sRight)->joinSize;
            sInputRecordCnt *= sRightOutput;
            break;

        case QMG_LEFT_OUTER_JOIN :
            sRightOutput     = ((qmgLOJN*)sRight)->joinSize;
            sInputRecordCnt *= sRightOutput;
            break;

        case QMG_FULL_OUTER_JOIN :
            sRightOutput     = ((qmgFOJN*)sRight)->joinSize;
            sInputRecordCnt *= sRightOutput;
            break;

        default :
            IDE_DASSERT(0);
            IDE_TEST( IDE_FAILURE );
    }

    sJoinSize = sInputRecordCnt * sSelectivity;

    switch( aJoinGraph->type )
    {
        case QMG_SEMI_JOIN:
            // |R SJ S| = MIN(|R|, |R JOIN S|)
            if( sJoinSize > sLeft->costInfo.outputRecordCnt )
            {
                sJoinSize = sLeft->costInfo.outputRecordCnt;
            }
            else
            {
                // Nothing to do.
            }
            break;
        case QMG_ANTI_JOIN:
            // |R AJ S| = |R| x 0.1,              if SF(R SJ S) = 1 (i.e. |R SJ S| = |R|)
            //            |R| x (1 - SF(R SJ S)), otherwise
            if( sJoinSize > sLeft->costInfo.outputRecordCnt )
            {
                sJoinSize = sLeft->costInfo.outputRecordCnt * 0.1;
            }
            else
            {
                sJoinSize = sLeft->costInfo.outputRecordCnt - sJoinSize;
            }
            break;
        default:
            // Inner join 시
            break;
    }


    // To Fix PR-8005
    // 0이 되는 것을 방지하여야 함
    sJoinSize = ( sJoinSize < 1 ) ? 1 : sJoinSize;

    //------------------------------------------
    // JoinOrderFactor 계산
    //------------------------------------------
    sJoinOrderFactor = sJoinSize / ( sLeftOutput + sRightOutput );

    IDE_DASSERT_MSG( sSelectivity >= 0 && sSelectivity <= 1,
                     "Selectivity : %"ID_DOUBLE_G_FMT"\n",
                     sSelectivity );

    *aJoinOrderFactor = sJoinOrderFactor;
    *aJoinSize        = sJoinSize;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoSelectivity::setHierarchySelectivity( qcStatement  * aStatement,
                                         qmoPredicate * aWherePredList,
                                         qmoCNF       * aConnectByCNF,
                                         qmoCNF       * aStartWithCNF,
                                         idBool         aInExecutionTime,
                                         SDouble      * aSelectivity )
{
/******************************************************************************
 *
 * Description : qmgHierarchy 에 대한 selectivity 계산
 *
 * Implementation :
 *
 *     1. 각 조건절에 대한 selectivity 획득
 *     1.1. whereSelectivity = PRODUCT(selectivity for where clause)
 *          (where predicate 은 qmgSelection 으로 push 되지 않음)
 *     1.2. connectBySelectivity = PRODUCT(selectivity for connect by clause)
 *     1.3. startWithSelectivity = PRODUCT(selectivity for start with clause)
 *
 *     2. qmgHierarchy selectivity 획득
 *        S = whereSelectivity * connectBySelectivity * startWithSelectivity
 *
 *****************************************************************************/

    SDouble    sWhereSelectivity;
    SDouble    sConnectBySelectivity;
    SDouble    sStartWithSelectivity;
    SDouble    sSelectivity;

    IDU_FIT_POINT_FATAL( "qmoSelectivity::setHierarchySelectivity::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement    != NULL );
    IDE_DASSERT( aConnectByCNF != NULL );
    IDE_DASSERT( aSelectivity  != NULL );

    //--------------------------------------
    // Selectivity 계산
    //--------------------------------------

    IDE_TEST( getTotalSelectivity4PredList( aStatement,
                                            aWherePredList,
                                            & sWhereSelectivity,
                                            aInExecutionTime )
              != IDE_SUCCESS );

    IDE_TEST( getTotalSelectivity4PredList( aStatement,
                                            aConnectByCNF->oneTablePredicate,
                                            & sConnectBySelectivity,
                                            aInExecutionTime )
              != IDE_SUCCESS );

    if( aStartWithCNF != NULL )
    {
        IDE_TEST( getTotalSelectivity4PredList( aStatement,
                                                aStartWithCNF->oneTablePredicate,
                                                & sStartWithSelectivity,
                                                aInExecutionTime )
                  != IDE_SUCCESS );
    }
    else
    {
        sStartWithSelectivity = 1;
    }

    sSelectivity = sWhereSelectivity *
                   sConnectBySelectivity *
                   sStartWithSelectivity;

    *aSelectivity = sSelectivity;

    IDE_DASSERT_MSG( sSelectivity >= 0 && sSelectivity <= 1,
                     "Selectivity : %"ID_DOUBLE_G_FMT"\n",
                     sSelectivity );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoSelectivity::setCountingSelectivity( qmoPredicate * aStopkeyPred,
                                        SLong          aStopRecordCnt,
                                        SDouble        aInputRecordCnt,
                                        SDouble      * aSelectivity )
{
/******************************************************************************
 *
 * Description : qmgCounting 에 대한 selectivity 계산
 *
 * Implementation :
 *
 *     1. stopkeyPredicate 존재
 *      - OR 최상위노드를 갖는 unit predicate 으로 구성(CNF)
 *      - rownum = 1
 *      - rownum lessthan(<, <=, >, >=) 상수,호스트변수
 *     1.1. stopkeyPredicate 에 호스트 변수가 존재 (mtfEqual 불가)
 *          S = Default selectivity for opearator
 *     1.2. stopkeyPredicate 에 호스트 변수 없음
 *          S = MIN( stopRecordCount, InputRecordCount ) / InputRecordCount
 *     2. stopkeyPredicate 이 NULL : S = 1
 *
 *****************************************************************************/

    qmoPredicate  * sStopkeyPred;
    qtcNode       * sCompareNode;
    SDouble         sSelectivity;

    IDU_FIT_POINT_FATAL( "qmoSelectivity::setCountingSelectivity::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aSelectivity != NULL );
    IDE_DASSERT( aInputRecordCnt > 0  );

    //--------------------------------------
    // Selectivity 계산
    //--------------------------------------

    sStopkeyPred = aStopkeyPred;

    if ( sStopkeyPred != NULL )
    {

        sCompareNode = (qtcNode *)sStopkeyPred->node->node.arguments;

        // -2 이하일 수 없다.
        IDE_DASSERT( aStopRecordCnt >= -1 );
        IDE_DASSERT( ( sStopkeyPred->node->node.lflag & MTC_NODE_OPERATOR_MASK )
                     == MTC_NODE_OPERATOR_OR )
        IDE_DASSERT( sCompareNode->node.module == &mtfEqual ||
                     sCompareNode->node.module == &mtfLessThan ||
                     sCompareNode->node.module == &mtfLessEqual ||
                     sCompareNode->node.module == &mtfGreaterThan ||
                     sCompareNode->node.module == &mtfGreaterEqual );

        if( aStopRecordCnt == -1 )
        {
            // host 변수 존재
            // ex) rownum = :a 는 stopkeyPredicate 으로 분류되지 않음
            IDE_DASSERT( sCompareNode->node.module != &mtfEqual );

            sSelectivity = sCompareNode->node.module->selectivity;
        }
        else
        {
            sSelectivity = IDL_MIN( aInputRecordCnt, (SDouble)aStopRecordCnt )
                           / aInputRecordCnt;
        }
    }
    else
    {
        sSelectivity = 1;
    }

    *aSelectivity = sSelectivity;

    IDE_DASSERT_MSG( sSelectivity >= 0 && sSelectivity <= 1,
                     "Selectivity : %"ID_DOUBLE_G_FMT"\n",
                     sSelectivity );

    return IDE_SUCCESS;
}


IDE_RC
qmoSelectivity::setCountingOutputCnt( qmoPredicate * aStopkeyPred,
                                      SLong          aStopRecordCnt,
                                      SDouble        aInputRecordCnt,
                                      SDouble      * aOutputRecordCnt )
{
/******************************************************************************
 *
 * Description : qmgCounting 에 대한 outputRecordCnt 계산
 *
 * Implementation :
 *
 *     1. stopkeyPredicate 존재
 *      - OR 최상위노드를 갖는 unit predicate 으로 구성(CNF)
 *      - rownum = 1
 *      - rownum lessthan(<, <=, >, >=) 상수,호스트변수
 *     1.1. stopkeyPredicate 에 호스트 변수가 존재 (mtfEqual 불가)
 *          outputRecordCnt = InputRecordCount
 *     1.2. stopkeyPredicate 에 호스트 변수 없음
 *          outputRecordCnt = MIN( stopRecordCount, InputRecordCount )
 *     2. stopkeyPredicate 이 NULL
 *        outputRecordCnt = InputRecordCount
 *     3. outputRecordCnt 보정
 *
 *****************************************************************************/

    SDouble sOutputRecordCnt;

    IDU_FIT_POINT_FATAL( "qmoSelectivity::setCountingOutputCnt::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aOutputRecordCnt != NULL );
    IDE_DASSERT( aInputRecordCnt  > 0 );

    //--------------------------------------
    // Output count 계산
    //--------------------------------------

    if( aStopkeyPred != NULL )
    {
        // -2 이하일 수 없다.
        IDE_DASSERT( aStopRecordCnt >= -1 );

        if( aStopRecordCnt == -1 )
        {
            // host 변수 존재
            sOutputRecordCnt = aInputRecordCnt;
        }
        else
        {
            // host 변수 없음
            sOutputRecordCnt = IDL_MIN( aInputRecordCnt, (SDouble)aStopRecordCnt );
        }
    }
    else
    {
        sOutputRecordCnt = aInputRecordCnt;
    }

    // outputRecordCnt 최소값 보정
    *aOutputRecordCnt = ( sOutputRecordCnt < 1 ) ? 1 : sOutputRecordCnt;

    return IDE_SUCCESS;
}


IDE_RC
qmoSelectivity::setProjectionSelectivity( qmsLimit * aLimit,
                                          SDouble    aInputRecordCnt,
                                          SDouble  * aSelectivity )
{
/******************************************************************************
 *
 * Description : qmgProjection 에 대한 selectivity 계산
 *
 * Implementation :
 *
 *       1. LIMIT 절 존재
 *       1.1. start, count 모두 fixed value
 *          - start > inputRecordCnt : S = 0
 *          - start == inputRecordCnt : S = 1 / inputRecordCnt
 *          - inputRecordCnt - start + 1 >= count : S = count / inputRecordCnt
 *          - Etc : S = (inputRecordCnt - start + 1) / inputRecordCnt
 *i                   = 1 - ( (start-1) / inputRecordCnt )
 *       1.2. start, count 하나라도 variable value : mtfLessThan.selectivity
 *
 *       2. LIMIT 절 없음 : S = 1
 *
 *****************************************************************************/

    SDouble sLimitStart;
    SDouble sLimitCount;
    SDouble sSelectivity;

    IDU_FIT_POINT_FATAL( "qmoSelectivity::setProjectionSelectivity::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aSelectivity != NULL );
    IDE_DASSERT( aInputRecordCnt > 0  );

    //--------------------------------------
    // Limit 을 반영한 selectivity 계산
    //--------------------------------------

    if( aLimit != NULL )
    {
        // 1. LIMIT 절 존재

        if ( ( qmsLimitI::hasHostBind( qmsLimitI::getStart( aLimit ) )
               == ID_FALSE ) &&
             ( qmsLimitI::hasHostBind( qmsLimitI::getCount( aLimit ) )
               == ID_FALSE ) )
        {
            // 1.1. start, count 모두 fixed value
            // BUGBUG : ULong->SDouble
            sLimitStart = ID_ULTODB(qmsLimitI::getStartConstant(aLimit));
            sLimitCount = ID_ULTODB(qmsLimitI::getCountConstant(aLimit));

            if (sLimitStart > aInputRecordCnt)
            {
                sSelectivity = 0;
            }
            else if (sLimitStart == aInputRecordCnt)
            {
                sSelectivity = 1 / aInputRecordCnt;
            }
            else
            {
                if( aInputRecordCnt - sLimitStart + 1 >= sLimitCount )
                {
                    sSelectivity = sLimitCount / aInputRecordCnt;
                }
                else
                {
                    sSelectivity = 1 - ( (sLimitStart - 1) / aInputRecordCnt );
                }
            }
        }
        else
        {
            // 1.2. start, count 하나라도 variable value
            sSelectivity = mtfLessThan.selectivity;
        }
    }
    else
    {
        // 2. LIMIT 절 없음
        sSelectivity = 1;
    }

    *aSelectivity = sSelectivity;

    IDE_DASSERT_MSG( sSelectivity >= 0 && sSelectivity <= 1,
                     "Selectivity : %"ID_DOUBLE_G_FMT"\n",
                     sSelectivity );

    return IDE_SUCCESS;
}


IDE_RC
qmoSelectivity::setGroupingSelectivity( qmgGraph     * aGraph,
                                        qmoPredicate * aHavingPred,
                                        SDouble        aInputRecordCnt,
                                        SDouble      * aSelectivity )
{
/******************************************************************************
 *
 * Description : qmgGrouping 에 대한 selectivity 계산
 *
 * Implementation :
 *
 *       1. QMG_GROP_TYPE_NESTED : S = 1/ inputRecordCnt
 *       2. Etc
 *       2.1. QMG_GROP_OPT_TIP_COUNT_STAR or
 *            QMG_GROP_OPT_TIP_INDEXABLE_MINMAX :
 *            S = 1/ inputRecordCnt
 *       2.2. QMG_GROP_OPT_TIP_NONE (default) or
 *            QMG_GROP_OPT_TIP_INDEXABLE_GROUPBY or
 *            QMG_GROP_OPT_TIP_INDEXABLE_DISTINCTAGG
 *          - HAVING 절 존재 : S = PRODUCT(DS for HAVING clause)
 *            having Predicate의 selectivity 계산 함수 구현되지 않았음
 *            따라서 compare node의 default selectivity로 설정함
 *          - 그 외 : S = 1
 *       2.3. QMS_GROUPBY_ROLLUP :
 *            S = QMO_SELECTIVITY_ROLLUP_FACTOR (0.75)
 *       2.4. QMS_GROUPBY_CUBE or
 *            QMS_GROUPBY_GROUPING_SETS : S = 1
 *
 *****************************************************************************/

    qmoPredicate  * sHavingPred;
    SDouble         sSelectivity;

    IDU_FIT_POINT_FATAL( "qmoSelectivity::setGroupingSelectivity::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aGraph       != NULL );
    IDE_DASSERT( aSelectivity != NULL );
    IDE_DASSERT( aInputRecordCnt > 0  );

    //--------------------------------------
    // 기본 초기화
    //--------------------------------------

    sHavingPred = aHavingPred;

    // BUG-38132
    // HAVING 절의 경우 grouping 을 거친후 처리하기 때문에
    // selectivity 를 낮추어야 한다.
    if ( sHavingPred != NULL )
    {
        sSelectivity = QMO_SELECTIVITY_UNKNOWN;
    }
    else
    {
        sSelectivity = QMO_SELECTIVITY_NOT_EXIST;
    }

    //--------------------------------------
    // Selectivity 계산
    //--------------------------------------

    if( ( aGraph->flag & QMG_GROP_TYPE_MASK ) == QMG_GROP_TYPE_NESTED )
    {
        sSelectivity = 1 / aInputRecordCnt;
    }
    else
    {
        switch( aGraph->flag & QMG_GROP_OPT_TIP_MASK )
        {
            case QMG_GROP_OPT_TIP_COUNT_STAR:
            case QMG_GROP_OPT_TIP_INDEXABLE_MINMAX:
                sSelectivity = 1 / aInputRecordCnt;
                break;

            case QMG_GROP_OPT_TIP_NONE:
            case QMG_GROP_OPT_TIP_INDEXABLE_GROUPBY:
            case QMG_GROP_OPT_TIP_INDEXABLE_DISTINCTAGG:
                while( sHavingPred != NULL )
                {
                    // having 절 selectivity 계산
                    // qmgGrouping::init 에서 having 절은 CNF 로 normalize

                    sSelectivity
                        *= sHavingPred->node->node.arguments->module->selectivity;
                    sHavingPred = sHavingPred->next;
                }
                break;

            // PROJ-1353
            case QMG_GROP_OPT_TIP_ROLLUP:
                sSelectivity = QMO_SELECTIVITY_ROLLUP_FACTOR;
                break;

            case QMG_GROP_OPT_TIP_CUBE:
            case QMG_GROP_OPT_TIP_GROUPING_SETS:
                break;

            default:
                IDE_DASSERT(0);
                break;
        }
    }

    *aSelectivity = sSelectivity;

    IDE_DASSERT_MSG( sSelectivity >= 0 && sSelectivity <= 1,
                     "Selectivity : %"ID_DOUBLE_G_FMT"\n",
                     sSelectivity );

    return IDE_SUCCESS;
}


IDE_RC
qmoSelectivity::setGroupingOutputCnt( qcStatement      * aStatement,
                                      qmgGraph         * aGraph,
                                      qmsConcatElement * aGroupBy,
                                      SDouble            aInputRecordCnt,
                                      SDouble            aSelectivity,
                                      SDouble          * aOutputRecordCnt )
{
/******************************************************************************
 *
 * Description : qmgGrouping 에 대한 outputRecordCnt 계산
 *
 * Implementation : outputRecordCnt 획득
 *
 *       1. groupBy 절이 NULL 아니고
 *       1.1. QMG_GROP_TYPE_NESTED
 *            outputRecordCnt = inputRecordCnt * selectivity
 *       1.2  QMG_GROP_OPT_TIP_COUNT_STAR or
 *            QMG_GROP_OPT_TIP_INDEXABLE_MINMAX
 *            outputRecordCnt = inputRecordCnt * selectivity
 *       1.3. QMG_GROP_OPT_TIP_NONE (default) or
 *            QMG_GROP_OPT_TIP_INDEXABLE_GROUPBY or
 *            QMG_GROP_OPT_TIP_INDEXABLE_DISTINCTAGG
 *          - inputRecordCnt 보정
 *            group by 컬럼이 모두 one column list 이고
 *            group by 를 구성하는 모든 table 에 대해 통계정보 수집 :
 *            inputRecordCnt = MIN( inputRecordCnt, PRODUCT(columnNDV) )
 *          - outputRecordCnt = inputRecordCnt * selectivity
 *       1.4. QMS_GROUPBY_ROLLUP
 *            outputRecordCnt = inputRecordCnt *
 *                              QMO_SELECTIVITY_ROLLUP_FACTOR (0.75)
 *       1.5. QMS_GROUPBY_CUBE or
 *            QMS_GROUPBY_GROUPING_SETS
 *            outputRecordCnt = inputRecordCnt
 *       2. groupBy 절이 NULL : outputRecordCnt = 1
 *       3. outputRecordCnt 최소값 보정
 *
 *****************************************************************************/

    qtcNode          * sNode;
    qmsConcatElement * sGroupBy;
    qmoStatistics    * sStatInfo;
    qmoColCardInfo   * sColCardInfo;
    SDouble            sColumnNDV;
    SDouble            sInputRecordCnt;
    SDouble            sOutputRecordCnt;
    idBool             sAllColumn;

    IDU_FIT_POINT_FATAL( "qmoSelectivity::setGroupingOutputCnt::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement       != NULL );
    IDE_DASSERT( aGraph           != NULL );
    IDE_DASSERT( aOutputRecordCnt != NULL );
    IDE_DASSERT( aInputRecordCnt > 0 );

    //--------------------------------------
    // outputRecordCnt 계산
    //--------------------------------------

    sColumnNDV = 1;
    sOutputRecordCnt = 0;
    sAllColumn = ID_TRUE;

    if( aGroupBy != NULL )
    {
        if( ( aGraph->flag & QMG_GROP_TYPE_MASK ) == QMG_GROP_TYPE_NESTED )
        {
            sOutputRecordCnt = aInputRecordCnt * aSelectivity;
        }
        else
        {
            switch( aGraph->flag & QMG_GROP_OPT_TIP_MASK )
            {
                case QMG_GROP_OPT_TIP_COUNT_STAR:
                case QMG_GROP_OPT_TIP_INDEXABLE_MINMAX:
                    sOutputRecordCnt = aInputRecordCnt * aSelectivity;
                    break;

                case QMG_GROP_OPT_TIP_NONE:
                case QMG_GROP_OPT_TIP_INDEXABLE_GROUPBY:
                case QMG_GROP_OPT_TIP_INDEXABLE_DISTINCTAGG:

                    // BUG-38444 grouping 그래프의 output record count가 잘못 계산됨
                    // qmsConcatElement 구조체의 next 를 따라가야함
                    for ( sGroupBy = aGroupBy;
                          sGroupBy != NULL;
                          sGroupBy = sGroupBy->next )
                    {
                        sNode = sGroupBy->arithmeticOrList;

                        if ( QTC_IS_COLUMN( aStatement, sNode ) == ID_TRUE )
                        {
                            sStatInfo = QC_SHARED_TMPLATE(aStatement)->
                                        tableMap[sNode->node.table].
                                        from->tableRef->statInfo;

                            if( sStatInfo->isValidStat == ID_TRUE )
                            {
                                // group by 컬럼이 모두 one column list
                                sColCardInfo = sStatInfo->colCardInfo;
                                sColumnNDV  *= sColCardInfo[sNode->node.column].columnNDV;
                            }
                            else
                            {
                                // group by 컬럼이 모두 one column list
                                sColCardInfo = sStatInfo->colCardInfo;
                                sColumnNDV   = sColCardInfo[sNode->node.column].columnNDV;
                            }
                        }
                        else
                        {
                            sAllColumn = ID_FALSE;
                            break;
                        }
                    }

                    // inputRecordCnt 보정
                    if ( sAllColumn == ID_TRUE )
                    {
                        sInputRecordCnt = IDL_MIN( aInputRecordCnt, sColumnNDV );
                    }
                    else
                    {
                        sInputRecordCnt = aInputRecordCnt;
                    }

                    sOutputRecordCnt = sInputRecordCnt * aSelectivity;
                    break;

                // PROJ-1353
                case QMG_GROP_OPT_TIP_ROLLUP:
                    sOutputRecordCnt
                        = aInputRecordCnt * QMO_SELECTIVITY_ROLLUP_FACTOR;
                    break;

                case QMG_GROP_OPT_TIP_CUBE:
                case QMG_GROP_OPT_TIP_GROUPING_SETS:
                    sOutputRecordCnt = aInputRecordCnt;
                    break;

                default:
                    IDE_DASSERT(0);
                    break;
            }
        }
    }
    else
    {
        sOutputRecordCnt = 1;
    }

    // outputRecordCnt 최소값 보정
    *aOutputRecordCnt = ( sOutputRecordCnt < 1 ) ? 1 : sOutputRecordCnt;

    return IDE_SUCCESS;
}


IDE_RC
qmoSelectivity::setDistinctionOutputCnt( qcStatement * aStatement,
                                         qmsTarget   * aTarget,
                                         SDouble       aInputRecordCnt,
                                         SDouble     * aOutputRecordCnt )
{
/******************************************************************************
 *
 * Description : qmgDistinction 에 대한 outputRecordCnt 계산
 *
 * Implementation :
 *
 *      1. outputRecordCnt 획득
 *      1.1. target 을 구성하는 모든 table 에 대해 통계정보 수집이고
 *           target 컬럼이 모두 one column list 이고
 *           prowid pseudo column 이 아니면 :
 *           outputRecordCnt = MIN( inputRecordCnt, PRODUCT(columnNDV) )
 *      1.2. 그 외 : outputRecordCnt = inputRecordCnt
 *      2. outputRecordCnt 최소값 보정
 *
 *****************************************************************************/

    qmsTarget      * sTarget;
    qtcNode        * sNode;
    qmoStatistics  * sStatInfo;
    qmoColCardInfo * sColCardInfo;
    SDouble          sColumnNDV;
    SDouble          sOutputRecordCnt;
    idBool           sAllColumn;

    IDU_FIT_POINT_FATAL( "qmoSelectivity::setDistinctionOutputCnt::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement       != NULL );
    IDE_DASSERT( aTarget          != NULL );
    IDE_DASSERT( aOutputRecordCnt != NULL );
    IDE_DASSERT( aInputRecordCnt > 0 );

    //--------------------------------------
    // 기본 초기화
    //--------------------------------------

    sColumnNDV = 1;
    sAllColumn = ID_TRUE;
    sTarget = aTarget;

    //--------------------------------------
    // outputRecordCnt 계산
    //--------------------------------------

    // 1. outputRecordCnt 획득
    while( sTarget != NULL )
    {
        sNode = sTarget->targetColumn;

        // BUG-20272
        if( sNode->node.module == &mtfDecrypt )
        {
            sNode = (qtcNode*) sNode->node.arguments;
        }
        else
        {
            // Nothing to do.
        }

        // prowid pseudo column 이 올 수 없음
        IDE_DASSERT( sNode->node.column != MTC_RID_COLUMN_ID );

        if( QTC_IS_COLUMN( aStatement, sNode ) == ID_TRUE )
        {
            // target 컬럼이 모두 one column list
            sStatInfo = QC_SHARED_TMPLATE(aStatement)->
                        tableMap[sNode->node.table].
                        from->tableRef->statInfo;

            if( sStatInfo->isValidStat == ID_TRUE )
            {
                sColCardInfo = sStatInfo->colCardInfo;
                sColumnNDV *= sColCardInfo[sNode->node.column].columnNDV;
            }
            else
            {
                sAllColumn = ID_FALSE;
                break;
            }
        }
        else
        {
            sAllColumn = ID_FALSE;
            break;
        }

        sTarget = sTarget->next;
    }

    if ( sAllColumn == ID_TRUE )
    {
        sOutputRecordCnt = IDL_MIN( aInputRecordCnt, sColumnNDV );
    }
    else
    {
        sOutputRecordCnt = aInputRecordCnt;
    }

    // 2. outputRecordCnt 최소값 보정
    *aOutputRecordCnt = ( sOutputRecordCnt < 1 ) ? 1 : sOutputRecordCnt;

    return IDE_SUCCESS;
}


IDE_RC
qmoSelectivity::setSetOutputCnt( qmsSetOpType   aSetOpType,
                                 SDouble        aLeftOutputRecordCnt,
                                 SDouble        aRightOutputRecordCnt,
                                 SDouble      * aOutputRecordCnt )
{
/******************************************************************************
 *
 * Description : qmgSet 에 대한 outputRecordCnt 계산
 *
 * Implementation :
 *
 *     1. outputRecordCnt 획득
 *     1.1. QMS_UNION_ALL
 *          outputRecordCnt = left outputRecordCnt + right outputRecordCnt
 *     1.2. QMS_UNION
 *          outputRecordCnt = (left outputRecordCnt + right outputRecordCnt) / 2
 *     1.3. Etc ( QMS_MINUS, QMS_INTERSECT )
 *          outputRecordCnt = left outputRecordCnt / 2
 *
 *     2. outputRecordCnt 최소값 보정
 *
 *    cf) 1.2, 1.3 에 한해 outputRecordCnt 보정 idea
 *        target 을 구성하는 모든 table 에 대해 통계정보 수집이고
 *        target 컬럼이 모두 one column list 이면
 *        (SET 의 경우 prowid pseudo column 은 target 에 올 수 없음) :
 *        outputRecordCnt = MIN( outputRecordCnt, PRODUCT(columnNDV) )
 *     => validation 과정에서 tuple 을 할당받아 target 을 새로 생성
 *        tablemap[table].from 이 NULL 이 되어
 *        one column list 및 statInfo 획득 불가
 *
 *****************************************************************************/

    SDouble sOutputRecordCnt = 0;

    IDU_FIT_POINT_FATAL( "qmoSelectivity::setSetOutputCnt::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aOutputRecordCnt != NULL );
    IDE_DASSERT( aLeftOutputRecordCnt  > 0 );
    IDE_DASSERT( aRightOutputRecordCnt > 0 );

    //--------------------------------------
    // outputRecordCnt 계산
    //--------------------------------------

    // 1. outputRecordCnt 획득
    switch( aSetOpType )
    {
        case QMS_UNION_ALL:
            sOutputRecordCnt = aLeftOutputRecordCnt + aRightOutputRecordCnt;
            break;
        case QMS_UNION:
            sOutputRecordCnt
                = ( aLeftOutputRecordCnt + aRightOutputRecordCnt ) / 2;
            break;
        case QMS_MINUS:
        case QMS_INTERSECT:
            sOutputRecordCnt = aLeftOutputRecordCnt / 2;
            break;
        default:
            IDE_DASSERT(0);
            break;
    }

    // 2. outputRecordCnt 최소값 보정
    *aOutputRecordCnt = ( sOutputRecordCnt < 1 ) ? 1 : sOutputRecordCnt;

    return IDE_SUCCESS;
}


IDE_RC
qmoSelectivity::setMySelectivityOffset( qcTemplate    * aTemplate,
                                        qmoStatistics * aStatInfo,
                                        qcDepInfo     * aDepInfo,
                                        qmoPredicate  * aPredicate )
{
/******************************************************************************
 *
 * Description : Execution 단계(host 변수에 값이 바인딩된 후)에서
 *               qmo::optimizeForHost 호출시 scan method 재구축을 위해
 *               template->data + qmoPredicate.mySelectivityOffset 에
 *               mySelectivity 를 재설정한다.
 *
 * Implementation :
 *
 *****************************************************************************/

    SDouble   * sSelectivity;
    SDouble     sUnitSelectivity;
    qtcNode   * sCompareNode;
    idBool      sIsDNF;
    UChar     * sData;

    IDU_FIT_POINT_FATAL( "qmoSelectivity::setMySelectivityOffset::__FT__" );

    IDE_DASSERT( aTemplate   != NULL );
    IDE_DASSERT( aStatInfo   != NULL );
    IDE_DASSERT( aDepInfo    != NULL );
    IDE_DASSERT( aPredicate  != NULL );

    //--------------------------------------
    // Selectivity 계산
    //--------------------------------------

    // 이 함수는 host 변수가 존재하는 more list의 predicate 들에 대해서만
    // 호출된다. 따라서 indexable column이어야 한다.

    if( ( aPredicate->flag & QMO_PRED_HOST_OPTIMIZE_MASK )
        == QMO_PRED_HOST_OPTIMIZE_TRUE )
    {
        IDE_DASSERT( aPredicate->id != QMO_COLUMNID_NON_INDEXABLE );

        sIsDNF = ID_FALSE;
        sData = aTemplate->tmplate.data;
        sSelectivity = (SDouble*)( sData + aPredicate->mySelectivityOffset );

        if( ( aPredicate->node->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
            == MTC_NODE_LOGICAL_CONDITION_TRUE )
        {
            // CNF인 경우
            sCompareNode = (qtcNode *)(aPredicate->node->node.arguments);
        }
        else
        {
            // DNF인 경우
            sCompareNode = aPredicate->node;
            sIsDNF = ID_TRUE;
        }

        if( sCompareNode->node.next != NULL && sIsDNF == ID_FALSE )
        {
            // CNF이면서 OR 하위에 비교연산자가 여러개 있는 경우,
            // OR 논리연산자에 대한 selectivity 계산.
            // 1 - (1-a)(1-b).....

            // DNF 는 node->next가 존재할 수 있음.

            *sSelectivity = 1;

            while( sCompareNode != NULL )
            {
                IDE_TEST( getUnitSelectivity( aTemplate,
                                              aStatInfo,
                                              aDepInfo,
                                              aPredicate,
                                              sCompareNode,
                                              & sUnitSelectivity,
                                              ID_TRUE )
                          != IDE_SUCCESS );

                *sSelectivity *= ( 1 - sUnitSelectivity );

                sCompareNode = (qtcNode *)(sCompareNode->node.next);
            }

            *sSelectivity = ( 1 - *sSelectivity );
        }
        else
        {
            // DNF 또는 CNF 로 비교 연산자가 하나만 있는 경우
            IDE_TEST( getUnitSelectivity( aTemplate,
                                          aStatInfo,
                                          aDepInfo,
                                          aPredicate,
                                          sCompareNode,
                                          sSelectivity,
                                          ID_TRUE )
                      != IDE_SUCCESS );
        }

    }
    else
    {
        // Host 변수를 가지고 있지 않으므로 prepare 시점에서 구한
        // predicate->mySelectivity를 그대로 사용한다.
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


SDouble
qmoSelectivity::getMySelectivity( qcTemplate   * aTemplate,
                                  qmoPredicate * aPredicate,
                                  idBool         aInExecutionTime )
{
/******************************************************************************
 *
 * Description : PROJ-1446 Host variable을 포함한 질의 최적화
 *               aInExecutionTime 을 인자로 받는 함수들에서
 *               qmoPredicate 에 대한 mySelectivity 를 얻어올 때 이용한다.
 *
 * Implementation :
 *
 *     1. Execution 단계(host 변수에 값이 바인딩된 후)에서
 *        qmo::optimizeForHost 에 의한 호출이고 QMO_PRED_HOST_OPTIMIZE_TRUE 이면
 *        template->data + qmoPredicate.mySelectivityOffset 에 설정된 값을 반환
 *     2. 그 외의 경우
 *        qmoPredicate.mySelectivity 를 반환
 *
 *****************************************************************************/

    UChar   *sData;
    SDouble  sSelectivity;

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aTemplate  != NULL );
    IDE_DASSERT( aPredicate != NULL );

    if( ( aInExecutionTime == ID_TRUE ) &&
        ( aPredicate->flag & QMO_PRED_HOST_OPTIMIZE_MASK )
        == QMO_PRED_HOST_OPTIMIZE_TRUE )
    {
        sData = aTemplate->tmplate.data;
        sSelectivity = *(SDouble*)( sData + aPredicate->mySelectivityOffset );
    }
    else
    {
        sSelectivity = aPredicate->mySelectivity;
    }

    return sSelectivity;
}


IDE_RC
qmoSelectivity::setTotalSelectivityOffset( qcTemplate    * aTemplate,
                                           qmoStatistics * aStatInfo,
                                           qmoPredicate  * aPredicate )
{
/******************************************************************************
 *
 * Description : Execution 단계(host 변수에 값이 바인딩된 후)에서
 *               qmo::optimizeForHost 호출시 scan method 재구축을 위해
 *               template->data + qmoPredicate.totalSelectivityOffset 에
 *               totalSelectivity 를 재설정한다.
 *
 * Implementation :
 *
 *     1. 통합 selectivity 를 구할 수 있는 조건 : getIntegrateMySelectivity
 *        => setTotalSelectivity 수행 당시 QMO_PRED_HEAD_HOST_OPT_TOTAL_TRUE 세팅
 *     2. 통합 selectivity 를 구할 수 없는 조건 : PRODUCT( getMySelectivity )
 *
 *****************************************************************************/

    SDouble      * sTotalSelectivity;
    SDouble        sTempSelectivity;
    qmoPredicate * sMorePredicate;
    UChar        * sData;

    IDU_FIT_POINT_FATAL( "qmoSelectivity::setTotalSelectivityOffset::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aTemplate  != NULL );
    IDE_DASSERT( aStatInfo  != NULL );
    IDE_DASSERT( aPredicate != NULL );
    IDE_DASSERT( ( aPredicate->flag & QMO_PRED_HEAD_HOST_OPTIMIZE_MASK )
                 == QMO_PRED_HEAD_HOST_OPTIMIZE_TRUE );

    //--------------------------------------
    // 통합 selectivity 계산
    //--------------------------------------

    sData = aTemplate->tmplate.data;
    sTotalSelectivity = (SDouble*)( sData + aPredicate->totalSelectivityOffset );

    if( ( aPredicate->flag & QMO_PRED_HEAD_HOST_OPT_TOTAL_MASK )
        == QMO_PRED_HEAD_HOST_OPT_TOTAL_TRUE )
    {
        // 통합 selectivity 를 구할 수 있는 조건
        IDE_TEST( getIntegrateMySelectivity( aTemplate,
                                             aStatInfo,
                                             aPredicate,
                                             sTotalSelectivity,
                                             ID_TRUE )
                  != IDE_SUCCESS );
    }
    else
    {
        // 통합 selectivity 를 구할 수 없는 조건
        sTempSelectivity = 1;
        sMorePredicate = aPredicate;

        while( sMorePredicate != NULL )
        {
            sTempSelectivity *= getMySelectivity( aTemplate,
                                                  sMorePredicate,
                                                  ID_TRUE );
            sMorePredicate = sMorePredicate->more;
        }

        *sTotalSelectivity = sTempSelectivity;
    }

    IDE_DASSERT_MSG( *sTotalSelectivity >= 0 && *sTotalSelectivity <= 1,
                     "TotalSelectivity : %"ID_DOUBLE_G_FMT"\n",
                     *sTotalSelectivity );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


SDouble
qmoSelectivity::getTotalSelectivity( qcTemplate   * aTemplate,
                                     qmoPredicate * aPredicate,
                                     idBool         aInExecutionTime )
{
/******************************************************************************
 *
 * Description : PROJ-1446 Host variable을 포함한 질의 최적화
 *               aInExecutionTime 을 인자로 받는 함수들에서
 *               qmoPredicate 에 대한 totalSelectivity 를 얻어올 때 이용한다.
 *
 * Implementation :
 *
 *     1. Execution 단계(host 변수에 값이 바인딩된 후)에서
 *        qmo::optimizeForHost 에 의한 호출이고 QMO_PRED_HOST_OPTIMIZE_TRUE 이면
 *        template->data + qmoPredicate.totalSelectivityOffset 에 설정된 값을 반환
 *     2. 그 외의 경우
 *        qmoPredicate.totalSelectivity 를 반환
 *
 *****************************************************************************/

    UChar   *sData;
    SDouble  sSelectivity;

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aTemplate  != NULL );
    IDE_DASSERT( aPredicate != NULL );

    if( ( aInExecutionTime == ID_TRUE ) &&
        ( aPredicate->flag & QMO_PRED_HEAD_HOST_OPTIMIZE_MASK )
        == QMO_PRED_HEAD_HOST_OPTIMIZE_TRUE )
    {
        sData = aTemplate->tmplate.data;
        sSelectivity = *(SDouble*)( sData + aPredicate->totalSelectivityOffset );
    }
    else
    {
        sSelectivity = aPredicate->totalSelectivity;
    }

    return sSelectivity;
}


IDE_RC
qmoSelectivity::getIntegrateMySelectivity( qcTemplate    * aTemplate,
                                           qmoStatistics * aStatInfo,
                                           qmoPredicate  * aPredicate,
                                           SDouble       * aSelectivity,
                                           idBool          aInExecutionTime )
{
/******************************************************************************
 *
 * Description : More list 가 2개인 indexable qmoPredicate 에 대해
 *               Integrate 가능한 경우 MIN-MAX selectivity 적용하여
 *               total selectivity 를 계산한다.
 *
 * Implementation :
 *
 *     1. Integrate 대상 검출
 *        => PROJ-2242 와 관련하여 filter subsumption 이 수행되었다는 가정
 *     1.1. 각 qmoPredicate 의 arguments(unit predicate) 갯수가 1개
 *     1.2. 각 qmoPredicate 의 operator 는 (>, >=, <, <=) 에 해당
 *     1.3. 각 qmoPredicate 의 operator 는 각각 다른 방향성
 *     1.4. Column (QMO_STAT_MINMAX_COLUMN_SET_FALSE)
 *          ex) FROM ( SELECT ( SELECT T2.I1 FROM T2 LIMIT 1 )A FROM T1 )V1
 *              WHERE V1.A > 1;
 *     1.5. Execution time
 *     1.6. column 노드는 MTD_SELECTIVITY_ENABLE
 *     1.7. value 노드는 fixed variable 형태
 *       => 1.1~7 조건 만족시 QMO_PRED_HEAD_HOST_OPT_TOTAL_MASK 세팅
 *     1.8. column 노드는 MTD_GROUP_MISC 가 아닌 것
 *          ex) MTD_GROUP_NUMBER, MTD_GROUP_TEXT, MTD_GROUP_DATE, MTD_GROUP_INTERVAL
 *     1.9. column 노드와 value 노드는 같은 group
 *     1.10. 통계정보 수집
 *
 *     2. Integrate selectivity 계산
 *     2.1. Integrate 대상 : S = MIN-MAX selectivity
 *          => 닫힌 범위에 대한 보정
 *     2.2 Etc : S = PRODUCT( MS )
 *
 *     cf) MIN-MAX selectivity : 두개의 predicate에서 max/min value node를 찾는다.
 *       (1) >, >= 이면,
 *         - indexArgument = 0 : min value ( 예: i1>5 )
 *         - indexArgument = 1 : max value ( 예: 5>i1 )
 *       (2) <, <= 이면,
 *         - indexArgument = 0 : max value ( 예: i1<1 )
 *         - indexArgument = 1 : min value ( 예: 1<i1 )
 *       (3) (1),(2)의 수행결과,
 *         - min/max value node가 모두 존재하면, 통합 selectivity 를 구함
 *
 *****************************************************************************/

    qmoColCardInfo  * sColCardInfo;
    qmoPredicate    * sPredicate;
    qmoPredicate    * sMorePredicate;
    qtcNode         * sCompareNode;
    qtcNode         * sColumnNode   = NULL;
    qtcNode         * sMinValueNode = NULL;
    qtcNode         * sMaxValueNode = NULL;
    mtcColumn       * sColumnColumn;
    mtcColumn       * sMinValueColumn;
    mtcColumn       * sMaxValueColumn;
    void            * sColumnMaxValue;
    void            * sColumnMinValue;
    void            * sMinValue;
    void            * sMaxValue;
    UInt              sColumnType = 0;
    UInt              sMaxValueType = 0;
    UInt              sMinValueType = 0;
    mtdDoubleType     sDoubleColumnMaxValue;
    mtdDoubleType     sDoubleColumnMinValue;
    mtdDoubleType     sDoubleMaxValue;
    mtdDoubleType     sDoubleMinValue;

    SDouble           sColumnNDV;
    SDouble           sSelectivity;
    // sBoundForm : min/max bound 형태
    // 0:   open,   open ( x <  i1 <  y )
    // 1:   open, closed ( x <  i1 <= y )
    //    closed,   open ( x <= i1 <  y )
    // 2: closed, closed ( x <= i1 <= y )
    SDouble           sBoundForm = 0;
    idBool            sIsIntegrate;

    IDU_FIT_POINT_FATAL( "qmoSelectivity::getIntegrateMySelectivity::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aTemplate    != NULL );
    IDE_DASSERT( aStatInfo    != NULL );
    IDE_DASSERT( aPredicate   != NULL );
    IDE_DASSERT( aSelectivity != NULL );
    IDE_DASSERT( aPredicate->more       != NULL &&
                 aPredicate->more->more == NULL );

    //--------------------------------------
    // 기본 초기화
    //--------------------------------------

    sPredicate = aPredicate;
    sColCardInfo = aStatInfo->colCardInfo;
    sSelectivity = 1;
    sIsIntegrate = ID_TRUE;

    //--------------------------------------
    // 1. Integrate 대상 검출
    //--------------------------------------

    sMorePredicate = sPredicate;

    while( sMorePredicate != NULL )
    {
        // BUG-43482
        if ( ( aInExecutionTime == ID_TRUE ) &&
             ( ( sMorePredicate->flag & QMO_PRED_HOST_OPTIMIZE_MASK ) != QMO_PRED_HOST_OPTIMIZE_TRUE ) )
        {
            sIsIntegrate = ID_FALSE;
            sMinValueNode = NULL;
            sMaxValueNode = NULL;
            break;
        }
        else
        {
            // Nothing To Do
        }

        // 1.1. 각 qmoPredicate 의 arguments(unit predicate) 갯수가 1개
        // => compare node 추출

        if( ( sMorePredicate->node->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
            == MTC_NODE_LOGICAL_CONDITION_TRUE )
        {
            sCompareNode = (qtcNode *)(sMorePredicate->node->node.arguments);

            if( sCompareNode->node.next != NULL )
            {
                // OR 노드 하위에 비교연산자가 두개이상 존재하는 경우,
                // 통합 selectivity를 구할 수 없다.
                // 예: i1>1 or i1<2
                sIsIntegrate = ID_FALSE;
                break;
            }
            else
            {
                // Nothing To Do
            }
        }
        else
        {
            // DNF
            sCompareNode = sMorePredicate->node;
        }
        
        // 1.2. 각 qmoPredicate 의 operator 는 (>, >=, <, <=) 에 해당
        // => column node, min/max value node 획득

        switch( sCompareNode->node.module->lflag & MTC_NODE_OPERATOR_MASK )
        {
            case( MTC_NODE_OPERATOR_GREATER_EQUAL ) :
                // >=
                sBoundForm++;        // 닫힌 정도 계산
                /* fall through */
            case( MTC_NODE_OPERATOR_GREATER ) :
                // >
                if( sCompareNode->indexArgument == 0 )
                {
                    sColumnNode = (qtcNode *)(sCompareNode->node.arguments);
                    sMinValueNode =
                        (qtcNode *)(sCompareNode->node.arguments->next);
                }
                else
                {
                    sColumnNode = (qtcNode *)(sCompareNode->node.arguments->next);
                    sMaxValueNode = (qtcNode *)(sCompareNode->node.arguments);
                }
                break;
            case( MTC_NODE_OPERATOR_LESS_EQUAL ) :
                // <=
                sBoundForm++;        // 닫힌 정도 계산
                /* fall through */
            case( MTC_NODE_OPERATOR_LESS ) :
                // <
                if( sCompareNode->indexArgument == 0 )
                {
                    sColumnNode = (qtcNode *)(sCompareNode->node.arguments);
                    sMaxValueNode
                        = (qtcNode *)(sCompareNode->node.arguments->next);
                }
                else
                {
                    sColumnNode = (qtcNode *)(sCompareNode->node.arguments->next);
                    sMinValueNode = (qtcNode *)(sCompareNode->node.arguments);
                }
                break;
            default :
                // (>, >=, <, <=) 이 외의 operator
                sIsIntegrate = ID_FALSE;
                break;
        }

        sMorePredicate = sMorePredicate->more;
    }

    // 1.3. 각 qmoPredicate 의 operator 는 각각 다른 방향성

    sIsIntegrate = ( sMinValueNode != NULL && sMaxValueNode != NULL ) ?
                   ID_TRUE: ID_FALSE;

    // 1.4. Column (QMO_STAT_MINMAX_COLUMN_SET_FALSE)
    //  ex) FROM ( SELECT ( SELECT T2.I1 FROM T2 LIMIT 1 )A FROM T1 )V1
    //      WHERE V1.A > 1 AND V1.A < 3;

    if( sIsIntegrate == ID_TRUE )
    {
        // BUG-16265
        // view의 target이 subquery인 경우 실제 column정보는 있지만
        // subquery node의 module에 mtdNull module 이 달리고
        // MTD_SELECTIVITY_DISABLE 이므로 통계정보를 저장하지 않는다.
        // 이런 경우 colCardInfo의 flag로 통계정보가 있는지 판단하고,
        // 없으면 default로 계산된다.

        if( ( sColCardInfo[sColumnNode->node.column].flag &
              QMO_STAT_MINMAX_COLUMN_SET_MASK )
            == QMO_STAT_MINMAX_COLUMN_SET_FALSE )
        {
            sIsIntegrate = ID_FALSE;
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    if( sIsIntegrate == ID_TRUE )
    {
        //--------------------------------------
        // 1.5. Execution time
        // 1.6. column 노드는 MTD_SELECTIVITY_ENABLE
        // 1.7. value 노드는 fixed variable 형태
        //   => 1.1~7 조건 만족시 QMO_PRED_HEAD_HOST_OPT_TOTAL_MASK 세팅
        // 1.8. column 노드는 MTD_GROUP_MISC 가 아닌 것
        // 1.9. column 노드와 value 노드는 같은 group
        // 1.10. 통계정보 수집
        //--------------------------------------

        IDE_DASSERT( sColumnNode != NULL );

        sColumnColumn = &( aTemplate->tmplate.
                           rows[sColumnNode->node.table].
                           columns[sColumnNode->node.column] );

        //--------------------------------------------
        // 다음 세가지 경우는,
        // 통합 selectivity를 계산할 수 없는 경우이므로,
        // 두개 predicate의 selectivity를 곱한다.
        // (1) predicate이 variable로 분류되는 경우
        // (2) 해당 컬럼의 min/max value가 없는 경우
        // (3) 날짜, 숫자형 이외의 dataType인 경우
        // BUG-38758
        // (4) deterministic function은 fixed predicate이지만 계산할 수 없다.
        //--------------------------------------------
        // fix BUG-13708
        // 동일컬럼리스트에 연결된 두개의 predicate에 대해 모두
        // 호스트 변수가 존재하는지 검사

        if( ( ( aInExecutionTime == ID_FALSE ) &&
              ( QMO_PRED_IS_VARIABLE( aPredicate ) == ID_TRUE ||
                QMO_PRED_IS_VARIABLE( aPredicate->more ) == ID_TRUE ) )
            ||
            ( ( sColumnColumn->module->flag & MTD_SELECTIVITY_MASK )
              == MTD_SELECTIVITY_DISABLE )
            ||
            ( ( ( aPredicate->node->lflag & QTC_NODE_PROC_FUNCTION_MASK )
                == QTC_NODE_PROC_FUNCTION_TRUE ) &&
              ( ( aPredicate->node->lflag & QTC_NODE_PROC_FUNC_DETERMINISTIC_MASK )
                == QTC_NODE_PROC_FUNC_DETERMINISTIC_TRUE ) )
          )

        {
            sIsIntegrate = ID_FALSE;

            // PROJ-1446 Host variable을 포함한 질의 최적화
            // 호스트 변수를 포함하고 통합 selectivity를 구해야 하는 경우
            // aPredicate의 flag에 정보를 표시해둔다.
            // 이 정보는 execution time에 total selectivity를 다시 구할 때
            // 통합 selectivity를 구할 수 있는지의 여부를
            // 쉽게 알아오기 위해서이다.

            if( ( aPredicate->flag & QMO_PRED_HEAD_HOST_OPTIMIZE_MASK )
                == QMO_PRED_HEAD_HOST_OPTIMIZE_TRUE )
            {
                aPredicate->flag &= ~QMO_PRED_HEAD_HOST_OPT_TOTAL_MASK;
                aPredicate->flag |= QMO_PRED_HEAD_HOST_OPT_TOTAL_TRUE;
            }
            else
            {
                aPredicate->flag &= ~QMO_PRED_HEAD_HOST_OPT_TOTAL_MASK;
                aPredicate->flag |= QMO_PRED_HEAD_HOST_OPT_TOTAL_FALSE;
            }
        }
        else
        {
            sColumnType = ( sColumnColumn->module->flag & MTD_GROUP_MASK );

            // minValue, maxValue 획득

            IDE_TEST( qtc::calculate( sMinValueNode, aTemplate )
                      != IDE_SUCCESS );
            sMinValue = aTemplate->tmplate.stack->value;
            sMinValueColumn = aTemplate->tmplate.stack->column;
            sMinValueType = ( sMinValueColumn->module->flag & MTD_GROUP_MASK );

            IDE_TEST( qtc::calculate( sMaxValueNode, aTemplate )
                      != IDE_SUCCESS );
            sMaxValue = aTemplate->tmplate.stack->value;
            sMaxValueColumn = aTemplate->tmplate.stack->column;
            sMaxValueType = ( sMaxValueColumn->module->flag & MTD_GROUP_MASK );

            if( ( sColumnType != MTD_GROUP_MISC ) &&
                ( sColumnType == sMaxValueType ) &&
                ( sColumnType == sMinValueType ) )
            {
                sIsIntegrate = ( aStatInfo->isValidStat == ID_TRUE ) ?
                               ID_TRUE: ID_FALSE;
            }
            else
            {
                sIsIntegrate = ID_FALSE;
            }
        }
    }
    else
    {
        // Nothing to do.
    }

    //--------------------------------------
    // 2. Integrate selectivity 계산
    //--------------------------------------

    if( sIsIntegrate == ID_TRUE )
    {
        IDE_DASSERT( aStatInfo->isValidStat == ID_TRUE );

        sColumnNDV = sColCardInfo[sColumnNode->node.column].columnNDV;
        sColumnMaxValue =
            (void *)(sColCardInfo[sColumnNode->node.column].maxValue);
        sColumnMinValue =
            (void *)(sColCardInfo[sColumnNode->node.column].minValue);

        //--------------------------------------
        // 동일 컬럼에 대한 통합 selectivity를 계산한다.
        // (1) 컬럼이 date type인 경우,
        //     : 해당 컬럼의 mtdModule 사용
        // (2) 컬럼이 숫자형계열인  경우,
        //     : value의 값이 모두 다른 data type일 수 있으므로,
        //       double type으로 변환하고,
        //       이 변환된 값으로 mtdDouble.module을 사용해서 selectivity를 구함.
        //--------------------------------------

        if ( sColumnType == MTD_GROUP_NUMBER )
        {
            // PROJ-1364
            // 숫자형 계열인 경우
            // 동일계열의 인덱스 사용으로 인해
            // 두 비교대상의 data type이 틀릴 수 있으므로
            // double로 변환해서, selectivity를 구한다.
            // 예) smallint_col > 3 and smallint_col < numeric'9'
            // integrateSelectivity()

            IDE_TEST ( getConvertToDoubleValue( sColumnColumn,
                                                sMaxValueColumn,
                                                sMinValueColumn,
                                                sColumnMaxValue,
                                                sColumnMinValue,
                                                sMaxValue,
                                                sMinValue,
                                                &sDoubleColumnMaxValue,
                                                &sDoubleColumnMinValue,
                                                &sDoubleMaxValue,
                                                &sDoubleMinValue )
                       != IDE_SUCCESS );

            sSelectivity = mtdDouble.selectivity( (void *)&sDoubleColumnMaxValue,
                                                  (void *)&sDoubleColumnMinValue,
                                                  (void *)&sDoubleMaxValue,
                                                  (void *)&sDoubleMinValue,
                                                  sBoundForm / sColumnNDV,
                                                  aStatInfo->totalRecordCnt );
        }
        else
        {
            // PROJ-1484
            // 숫자형이 아닌 경우 (문자열 선택도 추정 추가)
            // ex) DATE, CHAR(n), VARCHAR(n)

            sSelectivity = sColumnColumn->module->selectivity(
                sColumnMaxValue,
                sColumnMinValue,
                sMaxValue,
                sMinValue,
                sBoundForm / sColumnNDV,
                aStatInfo->totalRecordCnt );
        }

    }
    else
    {
        sMorePredicate = sPredicate;
        while( sMorePredicate != NULL )
        {
            sSelectivity *= getMySelectivity( aTemplate,
                                              sMorePredicate,
                                              aInExecutionTime );

            sMorePredicate = sMorePredicate->more;
        }
    }

    *aSelectivity = sSelectivity;

    IDE_DASSERT_MSG( sSelectivity >= 0 && sSelectivity <= 1,
                     "sSelectivity : %"ID_DOUBLE_G_FMT"\n",
                     sSelectivity );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoSelectivity::getUnitSelectivity( qcTemplate    * aTemplate,
                                    qmoStatistics * aStatInfo,
                                    qcDepInfo     * aDepInfo,
                                    qmoPredicate  * aPredicate,
                                    qtcNode       * aCompareNode,
                                    SDouble       * aSelectivity,
                                    idBool          aInExecutionTime )
{
/******************************************************************************
 *
 * Description : JOIN 을 제외한 unit predicate 의 unit selectivity 반환
 *
 * Implementation :
 *
 *     1. =, != (<>) : getEqualSelectivity 수행
 *     2. IS NULL, IS NOT NULL : getIsNullSelectivity 수행
 *     3. >, >=, <, <=, BETWEEN, NOT BETWEEN :
 *        getGreaterLessBeetweenSelectivity 수행
 *     4. LIKE, NOT LIKE : getLikeSelectivity 수행
 *     5. IN, NOT IN : getInSelectivity 수행
 *     6. EQUALS : getEqualsSelectivity 수행
 *     7. LNNVL : getLnnvlSelectivity 수행
 *     8. Etc : default selectivity
 *
 *****************************************************************************/

    SDouble     sSelectivity;

    IDU_FIT_POINT_FATAL( "qmoSelectivity::getUnitSelectivity::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aTemplate    != NULL );
    IDE_DASSERT( aStatInfo    != NULL );
    IDE_DASSERT( aDepInfo     != NULL );
    IDE_DASSERT( aPredicate   != NULL );
    IDE_DASSERT( aCompareNode != NULL );
    IDE_DASSERT( aSelectivity != NULL );

    // BUG-41748
    // BUG-42283 host variable predicate 에 대한 selectivity 는 default selectivity 를 사용한다.
    // (ex) WHERE i1 = :a AND i2 = :a OR :a is null
    //                                   ^^^^^^^^^^
    if ( ( ( aCompareNode->lflag & QTC_NODE_COLUMN_RID_MASK ) == QTC_NODE_COLUMN_RID_EXIST ) ||
         ( qtc::haveDependencies( &aCompareNode->depInfo ) == ID_FALSE ) )
    {
        sSelectivity = aCompareNode->node.module->selectivity;
    }
    else
    {
        //--------------------------------------
        // Unit selectivity 계산
        //--------------------------------------

        if( ( aCompareNode->node.module == &mtfEqual ) ||
            ( aCompareNode->node.module == &mtfNotEqual ) )
        {
            //---------------------------------------
            // =, !=(<>)
            //---------------------------------------
            IDE_TEST( getEqualSelectivity( aTemplate,
                                           aStatInfo,
                                           aDepInfo,
                                           aPredicate,
                                           aCompareNode,
                                           & sSelectivity )
                      != IDE_SUCCESS );
        }
        else if( ( aCompareNode->node.module == &mtfIsNull ) ||
                 ( aCompareNode->node.module == &mtfIsNotNull ) )
        {
            //---------------------------------------
            // IS NULL, IS NOT NULL
            //---------------------------------------
            IDE_TEST( getIsNullSelectivity( aTemplate,
                                            aStatInfo,
                                            aPredicate,
                                            aCompareNode,
                                            & sSelectivity )
                      != IDE_SUCCESS );
        }
        else if( ( aCompareNode->node.module == &mtfGreaterThan ) ||
                 ( aCompareNode->node.module == &mtfGreaterEqual ) ||
                 ( aCompareNode->node.module == &mtfLessThan ) ||
                 ( aCompareNode->node.module == &mtfLessEqual ) ||
                 ( aCompareNode->node.module == &mtfBetween ) ||
                 ( aCompareNode->node.module == &mtfNotBetween ) )
        {
            //---------------------------------------
            // >, <, >=, <=, BETWEEN, NOT BETWEEN
            //---------------------------------------
            IDE_TEST( getGreaterLessBeetweenSelectivity( aTemplate,
                                                         aStatInfo,
                                                         aPredicate,
                                                         aCompareNode,
                                                         & sSelectivity,
                                                         aInExecutionTime )
                      != IDE_SUCCESS );
        }
        else if( ( aCompareNode->node.module == &mtfLike ) ||
                 ( aCompareNode->node.module == &mtfNotLike ) )
        {
            //---------------------------------------
            // LIKE, NOT LIKE
            //---------------------------------------
            IDE_TEST( getLikeSelectivity( aTemplate,
                                          aStatInfo,
                                          aPredicate,
                                          aCompareNode,
                                          & sSelectivity,
                                          aInExecutionTime )
                      != IDE_SUCCESS );

        }
        else if( ( aCompareNode->node.module == &mtfEqualAny ) ||
                 ( aCompareNode->node.module == &mtfNotEqualAll ) )
        {
            //---------------------------------------
            // IN(=ANY), NOT IN(!=ALL)
            //---------------------------------------
            IDE_TEST( getInSelectivity( aTemplate,
                                        aStatInfo,
                                        aDepInfo,
                                        aPredicate,
                                        aCompareNode,
                                        & sSelectivity )
                      != IDE_SUCCESS );
        }
        else if( ( aCompareNode->node.module == &stfEquals ) ||
                 ( aCompareNode->node.module == &stfNotEquals ) )
        {
            //---------------------------------------
            // EQUALS (Spatial relational operator)
            //---------------------------------------
            IDE_TEST( getEqualsSelectivity( aStatInfo,
                                            aPredicate,
                                            aCompareNode,
                                            & sSelectivity )
                      != IDE_SUCCESS );
        }
        else if( aCompareNode->node.module == &mtfLnnvl )
        {
            //---------------------------------------
            // LNNVL (logical not null value operator)
            //---------------------------------------
            IDE_TEST( getLnnvlSelectivity( aTemplate,
                                           aStatInfo,
                                           aDepInfo,
                                           aPredicate,
                                           aCompareNode,
                                           & sSelectivity,
                                           aInExecutionTime )
                      != IDE_SUCCESS );
        }
        else
        {
            //---------------------------------------
            // {>,>=,<,<=}ANY, {>,>=,<,<=}ALL 인 경우
            // qtc::hostConstantWrapperModule 인 경우 ( BUG-39036 )
            // default selectivity 로 계산
            //---------------------------------------
            sSelectivity = aCompareNode->node.module->selectivity;
        }
    }

    *aSelectivity = sSelectivity;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoSelectivity::getEqualSelectivity( qcTemplate    * aTemplate,
                                     qmoStatistics * aStatInfo,
                                     qcDepInfo     * aDepInfo,
                                     qmoPredicate  * aPredicate,
                                     qtcNode       * aCompareNode,
                                     SDouble       * aSelectivity )
{
/******************************************************************************
 *
 * Description : =, !=(<>) 에 대한 unit selectivity 반환 (one table)
 *
 * Implementation :
 *
 *   1. Indexable predicate
 *   => !=, one side column LIST 는 non-indexable 이지만
 *      selectivity 획득이 가능하므로 indexable 의 selectivity로 계산
 *      ex) (i1, i2) != (1,1)
 *   1.1. 통계정보 수집
 *      - column LIST : S = PRODUCT(1/NDVn)
 *        ex) (t1.i1,t1.i2)=(select t2.i1, t2.i2 from t2)
 *            (i1,i2)=(1,2), (i1,i2)=((1,2)), (i1,i2)=(1,:a), (i1,i2)=(:a,:a)
 *      - one column : S = 1/NDV
 *        ex) t1.i1=(select t2.i1 from t2), i1=1, i1=(1), i1=:a
 *   1.2. 통계정보 미수집
 *      - column LIST : S = PRODUCT(DSn)
 *      - one column : S = DS
 *
 *   2. Non-indexable predicate
 *   2.1. column LIST : S = PRODUCT(DSn)
 *        ex) (i1,1)=(1,1), (i1,i2)=(1,'1'), (i1,1)=(1,:a), (i1,1)=(1,i2)
 *            (t1.i1,1)=(select i1, i2 from t2)
 *   2.2. Etc (양쪽 모두 LIST 형태가 아님)
 *     => 통계정보 수집 (1/NDV), 통계정보 미수집 (DS)
 *      - OneColumn and OneColumn : S = MIN( 1/leftNDV, 1/rightNDV )
 *        ex) i1=i2, i1=i1
 *      - OneColumn and Etc : S = MIN( 1/columnNDV , DS )
 *        ex) i1=i2+1, i1=i2+:a, i1=(J/JA type SUBQ)
 *      - Etc and Etc : S = DS
 *        ex) i1+1=i2+1, i1+i2=1
 *
 *   3. !=(<>) 보정
 *
 *****************************************************************************/

    qmoColCardInfo * sColCardInfo;
    qtcNode        * sColumnNode;
    qtcNode        * sColumn;
    qtcNode        * sLeftNode;
    qtcNode        * sRightNode;
    SDouble          sDefaultSelectivity;
    SDouble          sLeftSelectivity;
    SDouble          sRightSelectivity;
    SDouble          sSelectivity;
    idBool           sIsIndexable;

    IDU_FIT_POINT_FATAL( "qmoSelectivity::getEqualSelectivity::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aTemplate    != NULL );
    IDE_DASSERT( aStatInfo    != NULL );
    IDE_DASSERT( aDepInfo     != NULL );
    IDE_DASSERT( aPredicate   != NULL );
    IDE_DASSERT( aCompareNode != NULL );
    IDE_DASSERT( aCompareNode->node.module == &mtfEqual ||
                 aCompareNode->node.module == &mtfNotEqual );
    IDE_DASSERT( aSelectivity != NULL );

    //--------------------------------------
    // 기본 초기화
    //--------------------------------------

    sSelectivity = 1;
    sDefaultSelectivity = mtfEqual.selectivity;
    sColCardInfo = aStatInfo->colCardInfo;

    //--------------------------------------
    // Selectivity 계산
    //--------------------------------------

    // column 노드 획득
    if( aCompareNode->indexArgument == 0 )
    {
        sColumnNode = (qtcNode *)(aCompareNode->node.arguments);
    }
    else
    {
        sColumnNode = (qtcNode *)(aCompareNode->node.arguments->next);
    }

    // BUG-7814 : (i1, i2) != (1,2)
    // - != operator 를 사용한 column LIST 형태는 non-indexable 로 분류
    // - PROJ-2242 : default selectivity 회피를 위해 indexable 처럼 처리

    if( ( aCompareNode->node.module == &mtfNotEqual ) &&
        ( sColumnNode->node.lflag & MTC_NODE_OPERATOR_MASK )
        == MTC_NODE_OPERATOR_LIST )
    {
        IDE_TEST( qmoPred::isExistColumnOneSide( aTemplate->stmt,
                                                 aCompareNode,
                                                 aDepInfo,
                                                 & sIsIndexable )
                  != IDE_SUCCESS );
    }
    else
    {
        sIsIndexable = ( aPredicate->id == QMO_COLUMNID_NON_INDEXABLE ) ?
            ID_FALSE: ID_TRUE;
    }

    if( sIsIndexable == ID_TRUE )
    {
        // 1. Indexable predicate

        if( aStatInfo->isValidStat == ID_TRUE )
        {
            // 1.1. 통계정보 수집

            if( ( sColumnNode->node.lflag & MTC_NODE_OPERATOR_MASK )
                == MTC_NODE_OPERATOR_LIST )
            {
                // - column LIST
                sColumn = (qtcNode *)(sColumnNode->node.arguments);
                while( sColumn != NULL )
                {
                    sSelectivity *=
                        1 / sColCardInfo[sColumn->node.column].columnNDV;
                    sColumn = (qtcNode *)(sColumn->node.next);
                }
            }
            else if ( QTC_TEMPLATE_IS_COLUMN( aTemplate, sColumnNode ) == ID_TRUE )
            {
                // - one column
                sSelectivity =
                    1 / sColCardInfo[sColumnNode->node.column].columnNDV;
            }
            else
            {
                // BUG-40074 상수값도 indexable 일수 있다.
                // - const value
                sSelectivity = sDefaultSelectivity;
            }
        }
        else
        {
            // 1.2. 통계정보 미수집

            if( ( sColumnNode->node.lflag & MTC_NODE_OPERATOR_MASK )
                == MTC_NODE_OPERATOR_LIST )
            {
                // - column LIST
                sColumn = (qtcNode *)(sColumnNode->node.arguments);

                while( sColumn != NULL )
                {
                    sSelectivity *= sDefaultSelectivity;
                    sColumn = (qtcNode *)(sColumn->node.next);
                }
            }
            else
            {
                // - one column
                sSelectivity = sDefaultSelectivity;
            }
        }
    }
    else
    {
        // 2. Non-indexable predicate

        if( ( sColumnNode->node.lflag & MTC_NODE_OPERATOR_MASK )
            == MTC_NODE_OPERATOR_LIST )
        {
            // 2.1. Column LIST 형태

            sColumn = (qtcNode *)(sColumnNode->node.arguments);

            while( sColumn != NULL )
            {
                sSelectivity *= sDefaultSelectivity;
                sColumn = (qtcNode *)(sColumn->node.next);
            }
        }
        else
        {
            // 2.2. 양쪽 모두 LIST 형태가 아님

            // - OneColumn and OneColumn : S = MIN( 1/leftNDV, 1/rightNDV )
            // - OneColumn and Etc : S = MIN( 1/columnNDV , DS )
            // - Etc and Etc : S = DS

            sLeftNode  = (qtcNode *)(aCompareNode->node.arguments);
            sRightNode = (qtcNode *)(aCompareNode->node.arguments->next);


            // PR-20753 :
            // one column 이고 outer column이 아닌경우에
            // 두 operand에 대한 columnNDV 를 구한다.

            if( ( QTC_TEMPLATE_IS_COLUMN( aTemplate, sLeftNode ) == ID_TRUE ) &&
                ( qtc::dependencyContains( aDepInfo,
                                           &sLeftNode->depInfo) == ID_TRUE ) &&
                ( aStatInfo->isValidStat == ID_TRUE ) )
            {
                sLeftSelectivity =
                    1 / sColCardInfo[sLeftNode->node.column].columnNDV;
            }
            else
            {
                sLeftSelectivity = sDefaultSelectivity;
            }

            if( ( QTC_TEMPLATE_IS_COLUMN( aTemplate, sRightNode ) == ID_TRUE ) &&
                ( qtc::dependencyContains( aDepInfo,
                                           &sRightNode->depInfo) == ID_TRUE ) &&
                ( aStatInfo->isValidStat == ID_TRUE ) )
            {
                sRightSelectivity =
                    1 / sColCardInfo[sRightNode->node.column].columnNDV;
            }
            else
            {
                sRightSelectivity = sDefaultSelectivity;
            }

            sSelectivity = IDL_MIN( sLeftSelectivity, sRightSelectivity );
        }
    }

    /* BUG-47702 <> 연산자가 있을 경우 Selectivity가 1인 INDEX에 대한 잘못된  index 선택 오류
     * NOT EQUAL 의 Selectivity 가 1 인경우 즉 모두 같은 값인경우 selectivity를 1로 준다.
     */
    if ( aCompareNode->node.module == &mtfNotEqual )
    {
        if ( sSelectivity < 1 )
        {
            sSelectivity = 1 - sSelectivity;
        }
        else
        {
            sSelectivity = 1;
        }
    }
    else
    {
        /* Nothing to do */
    }

    *aSelectivity = sSelectivity;

    IDE_DASSERT_MSG( sSelectivity >= 0 && sSelectivity <= 1,
                     "Selectivity : %"ID_DOUBLE_G_FMT"\n",
                     sSelectivity );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoSelectivity::getIsNullSelectivity( qcTemplate    * aTemplate,
                                      qmoStatistics * aStatInfo,
                                      qmoPredicate  * aPredicate,
                                      qtcNode       * aCompareNode,
                                      SDouble       * aSelectivity )
{
/******************************************************************************
 *
 * Description : IS NULL, IS NOT NULL 에 대한 unit selectivity 반환
 *
 * Implementation : 통계정보가 없을 경우 operator 의 DS(default selectivity),
 *                  통계정보가 있을 경우 아래 식을 따른다.
 *
 *     1. 통계정보 미수집 또는 Non-indexable predicate : S = DS
 *        ex) i1+1 IS NULL, i1+i2 IS NULL
 *     2. 통계정보 수집이고 Indexable predicate
 *        ex) i1 IS NULL
 *     2.1. NULL value count 가 0 이 아니면 : S = NULL valueCnt / totalRecordCnt
 *     2.2. NULL value count 가 0 이면
 *        - NOT NULL constraint 이면 : S = 0
 *        - NOT NULL constraint 아니면 : S = 1 / columnNDV
 *
 *****************************************************************************/

    qtcNode     * sColumnNode;
    mtcColumn   * sColumnColumn;
    SDouble       sTotalRecordCnt;
    SDouble       sNullValueCnt;
    SDouble       sSelectivity;

    IDU_FIT_POINT_FATAL( "qmoSelectivity::getIsNullSelectivity::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aTemplate    != NULL );
    IDE_DASSERT( aStatInfo    != NULL );
    IDE_DASSERT( aPredicate   != NULL );
    IDE_DASSERT( aCompareNode != NULL );
    IDE_DASSERT( aCompareNode->node.module == &mtfIsNull ||
                 aCompareNode->node.module == &mtfIsNotNull );
    IDE_DASSERT( aSelectivity != NULL );

    //--------------------------------------
    // Selectivity 계산
    //--------------------------------------

    if( ( aStatInfo->isValidStat == ID_FALSE ) ||
        ( aPredicate->id == QMO_COLUMNID_NON_INDEXABLE ) )
    {
        // 1. 통계정보 미수집 또는 Non-indexable predicate
        sSelectivity = aCompareNode->node.module->selectivity;
    }
    else
    {
        // 2. 통계정보 수집이고 Indexable predicate
        sColumnNode = (qtcNode *)(aCompareNode->node.arguments);
        sNullValueCnt =
            aStatInfo->colCardInfo[sColumnNode->node.column].nullValueCount;
        sTotalRecordCnt = ( aStatInfo->totalRecordCnt < sNullValueCnt ) ?
                            sNullValueCnt : aStatInfo->totalRecordCnt;

        sColumnColumn = &( aTemplate->tmplate.
                           rows[sColumnNode->node.table].
                           columns[sColumnNode->node.column] );

        sSelectivity = ( sNullValueCnt > 0 ) ?
                       ( sNullValueCnt / sTotalRecordCnt ): // NULL value 존재
                       ( ( sColumnColumn->flag & MTC_COLUMN_NOTNULL_MASK )
                         == MTC_COLUMN_NOTNULL_TRUE ) ?     // NULL value 미존재
                       0:                       // NOT NULL constraints 일 경우
                       ( 1 / sTotalRecordCnt ); // NOT NULL constraints 아닐 경우

        // IS NOT NULL selectivity
        sSelectivity = ( aCompareNode->node.module == &mtfIsNotNull ) ?
                       ( 1 - sSelectivity ):
                       sSelectivity;
    }

    *aSelectivity = sSelectivity;

    IDE_DASSERT_MSG( sSelectivity >= 0 && sSelectivity <= 1,
                     "Selectivity : %"ID_DOUBLE_G_FMT"\n",
                     sSelectivity );

    return IDE_SUCCESS;
}

IDE_RC
qmoSelectivity::getGreaterLessBeetweenSelectivity(
        qcTemplate    * aTemplate,
        qmoStatistics * aStatInfo,
        qmoPredicate  * aPredicate,
        qtcNode       * aCompareNode,
        SDouble       * aSelectivity,
        idBool          aInExecutionTime )
{
/******************************************************************************
 *
 * Description : >, >=, <, <=, BETWEEN, NOT BETWEEN 에 대한 unit selectivity 계산
 *               QMO_PRED_HOST_OPTIMIZE_TRUE 세팅
 *               - Indexable predicate 이고 (one column)
 *               - Variable value (prepare time) 이고
 *               - QMO_PRED_IS_DYNAMIC_OPTIMIZABLE 이고
 *               - MTD_SELECTIVITY_ENABLE column
 *
 *           cf) Quantifier operator 에는 operand 로 LIST 형태가 올 수 없다.
 *
 * Implementation : 아래 1~2 에 대해 default selectivity 적용
 *                  아래 3 에 대해 Min-Max selectivity 적용
 *
 *   1. Indexable predicate 이고 (one column)
 *      MTD_SELECTIVITY_ENABLE column 이고
 *      Fixed value 또는 variable value(execution time) 를 대상으로
 *   1.1. Value (외부 컬럼 참조)
 *        ex) from t1 where exists (select i1 from t2 where i1 <= t1.i1)
 *                                                              |_ 이 부분
 *   1.2. Column (QMO_STAT_MINMAX_COLUMN_SET_FALSE)
 *        ex) FROM ( SELECT ( SELECT T2.I1 FROM T2 LIMIT 1 )A FROM T1 )V1
 *            WHERE V1.A > 1;
 *   1.3. Column (MTD_GROUP_MISC)
 *        ex) columnType 이 TEXT, NUMBER, DATE, INTERVAL group 에 속하지 않음
 *   1.4. Column and Value (columnType != valueType)
 *   1.5. 통계정보 미수집
 *
 *   2. Non-indexable predicate 또는
 *      MTD_SELECTIVITY_DISABLE column 또는
 *      Variable value (prepare time) 
 *      ex) 1 BETWEEN i1 AND 2, i1+i2 >= 1
 *          i1 BETWEEN 1 AND 2, i1 >= 1 (i1 이 연산불가 타입)
 *          i1 BETWEEN :a AND 2, i1 >= :a (prepare time)
 *
 *   3. Etc (위 조건 이외)
 *      ex) i1>1, i1<1, i1>=1, i1<=1, i1 BETWEEN 1 AND 2
 *
 *****************************************************************************/

    qmoColCardInfo * sColCardInfo;
    qtcNode        * sColumnNode;
    qtcNode        * sValueNode;
    qtcNode        * sValue;
    mtcColumn      * sColumnColumn;
    UInt             sColumnType;
    UInt             sValueType;
    SDouble          sSelectivity;
    idBool           sIsDefaultSelectivity;

    IDU_FIT_POINT_FATAL( "qmoSelectivity::getGreaterLessBeetweenSelectivity::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aTemplate    != NULL );
    IDE_DASSERT( aStatInfo    != NULL );
    IDE_DASSERT( aPredicate   != NULL );
    IDE_DASSERT( aCompareNode != NULL );
    IDE_DASSERT( aCompareNode->node.module == &mtfGreaterThan ||
                 aCompareNode->node.module == &mtfGreaterEqual ||
                 aCompareNode->node.module == &mtfLessThan ||
                 aCompareNode->node.module == &mtfLessEqual ||
                 aCompareNode->node.module == &mtfBetween ||
                 aCompareNode->node.module == &mtfNotBetween );
    IDE_DASSERT( aSelectivity != NULL );

    //---------------------------------------
    // 기본 초기화
    //---------------------------------------

    sIsDefaultSelectivity = ID_FALSE;
    sColCardInfo = aStatInfo->colCardInfo;

    // column node와 value node 획득
    if( aCompareNode->indexArgument == 0 )
    {
        sColumnNode = (qtcNode *)(aCompareNode->node.arguments);
        sValueNode  = (qtcNode *)(aCompareNode->node.arguments->next);
    }
    else
    {
        sColumnNode = (qtcNode *)(aCompareNode->node.arguments->next);
        sValueNode  = (qtcNode *)(aCompareNode->node.arguments);
    }

    sColumnColumn = &( aTemplate->tmplate.
                       rows[sColumnNode->node.table].
                       columns[sColumnNode->node.column] );

    sColumnType = ( sColumnColumn->module->flag & MTD_GROUP_MASK );

    //--------------------------------------
    // QMO_PRED_HOST_OPTIMIZE_TRUE 세팅
    //--------------------------------------

    // BUG-43065 외부 참조 컬럼이 있을때는 host 변수 최적화를 하면 안됨
    if( ( aPredicate->id != QMO_COLUMNID_NON_INDEXABLE ) &&
        ( aInExecutionTime == ID_FALSE ) &&
        ( QMO_PRED_IS_VARIABLE( aPredicate ) == ID_TRUE ) &&
        ( QMO_PRED_IS_DYNAMIC_OPTIMIZABLE( aPredicate ) == ID_TRUE ) &&
        ( ( sColumnColumn->module->flag & MTD_SELECTIVITY_MASK )
          != MTD_SELECTIVITY_DISABLE ) &&
        ( qtc::haveDependencies( &sValueNode->depInfo ) == ID_FALSE ) )
    {
        // Indexable predicate 이고 (one column)
        // Variable value (prepare time) 이고
        // QMO_PRED_IS_DYNAMIC_OPTIMIZABLE 이고
        // MTD_SELECTIVITY_ENABLE column

        // qmoPredicate->flag에 host optimization flag를 세팅한다.
        aPredicate->flag &= ~QMO_PRED_HOST_OPTIMIZE_MASK;
        aPredicate->flag |= QMO_PRED_HOST_OPTIMIZE_TRUE;
    }
    else
    {
        // Nothing to do.
    }

    //--------------------------------------
    // Default selectivity 적용 대상 추출
    //--------------------------------------

    if( ( aPredicate->id != QMO_COLUMNID_NON_INDEXABLE ) &&
        ( ( aInExecutionTime == ID_TRUE ) ||
          ( QMO_PRED_IS_VARIABLE( aPredicate ) == ID_FALSE ) ) &&
        ( ( sColumnColumn->module->flag & MTD_SELECTIVITY_MASK )
          != MTD_SELECTIVITY_DISABLE ) &&
        ( qtc::haveDependencies( &sValueNode->depInfo ) == ID_FALSE ) )
    {
        // 1. Indexable predicate 이고 (one column)
        //    MTD_SELECTIVITY_ENABLE column 이고
        //    Fixed value 또는 variable value(execution time)

        // BUG-32415 : BETWEEN/NOT BETWEEN과 같이 인자가 2개인 경우 고려

        sValue = sValueNode;
        while( sValue != NULL )
        {
            // BUG-30401 : Selectivity 를 구할 수 있는 조건인지 확인한다.
            // Dependency가 zero라면 bind 변수 또는 상수이므로
            // selectivity를 구할 수 있다.

            if( qtc::dependencyEqual( &sValue->depInfo,
                                      &qtc::zeroDependencies ) == ID_TRUE )
            {
                // sValueNode 가 Bind 변수나 상수인 경우
                // Nothing to do.
            }
            else
            {
                // 1.1. Value (외부 컬럼 참조)
                // ex) from t1 where exists 
                //     (select i1 from t2 where i1 between t1.i1 and 1)
                //                                        ^^^^^
                // ex) from t1 
                //     where exists (select i1 from t2 where i1 >= t1.i1)
                //                                                 ^^^^^
                // ex) hierarchical query 의 WHERE 절에 쓰인경우 (view 구성)
                //     from t1 where i1>1 connect by i1=i2
                //                   ^^

                sIsDefaultSelectivity = ID_TRUE;
                break;
            }
            sValue = (qtcNode*)(sValue->node.next);
        }

        if( sIsDefaultSelectivity == ID_FALSE )
        {
            // BUG-16265
            // view의 target이 subquery인 경우 실제 column정보는 있지만
            // subquery node의 module에 mtdNull module 이 달리고
            // MTD_SELECTIVITY_DISABLE 이므로 통계정보를 저장하지 않는다.
            // 이런 경우 colCardInfo의 flag로 통계정보가 있는지 판단하고,
            // 없으면 default로 계산된다.

            if( ( sColCardInfo[sColumnNode->node.column].flag &
                  QMO_STAT_MINMAX_COLUMN_SET_MASK )
                == QMO_STAT_MINMAX_COLUMN_SET_FALSE )
            {
                // 1.2. Column (QMO_STAT_MINMAX_COLUMN_SET_FALSE)
                // ex) FROM ( SELECT ( SELECT T2.I1 FROM T2 LIMIT 1 )A FROM T1 )V1
                //     WHERE V1.A > 1;

                sIsDefaultSelectivity = ID_TRUE;
            }
            else
            {
                // PROJ-2242 : default selectivity 보정을 회피하기 위해
                //             getMinMaxSelectivity 에서 옮김

                // BUG-22064
                // non-indexable하게 bind하는 경우 default selectivity를 사용

                if( sColumnType == MTD_GROUP_MISC )
                {
                    // 1.3. Column (MTD_GROUP_MISC)

                    sIsDefaultSelectivity = ID_TRUE;
                }
                else
                {
                    // 1.4. Column and Value (columnType != valueType)

                    // BUG-32415 : BETWEEN/NOT BETWEEN과 같이 인자가 2개인 경우 고려
                    sValue = sValueNode;
                    while( sValue != NULL )
                    {
                        // BUG-38758
                        // deterministic function은 fixed predicate이지만
                        // optimize단계에서 계산할 수 없다.
                        if ( ( (sValue->lflag & QTC_NODE_PROC_FUNCTION_MASK)
                               == QTC_NODE_PROC_FUNCTION_TRUE ) &&
                             ( (sValue->lflag & QTC_NODE_PROC_FUNC_DETERMINISTIC_MASK)
                               == QTC_NODE_PROC_FUNC_DETERMINISTIC_TRUE ) )
                        {
                            sIsDefaultSelectivity = ID_TRUE;
                            break;
                        }
                        else
                        {
                            IDE_TEST( qtc::calculate( sValue, aTemplate )
                                      != IDE_SUCCESS );
                        }

                        sValueType =
                            ( aTemplate->tmplate.stack->column->module->flag &
                              MTD_GROUP_MASK );

                        if( sColumnType != sValueType )
                        {
                            sIsDefaultSelectivity = ID_TRUE;
                            break;
                        }
                        else
                        {
                            // Nothing to do.
                        }
                        sValue = (qtcNode *)(sValue->node.next);
                    }
                }
            }
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // 2. Non-indexable predicate 또는
        //    MTD_SELECTIVITY_DISABLE column 또는
        //    Variable value
        //    - (1) predicate이 variable로 분류되는 경우,
        //    - (2) 해당 컬럼의 min/max value가 없는 경우
        // => selectivity를 계산할 수 없는 경우로 default selectivity

        sIsDefaultSelectivity = ID_TRUE;
    }

    //--------------------------------------
    // Selectivity 계산
    //--------------------------------------

    if( ( aStatInfo->isValidStat == ID_TRUE ) &&
        ( sIsDefaultSelectivity == ID_FALSE ) )
    {
        // Selectivity를 구할 수 있는 경우로,
        // MIN/MAX를 이용한 selectivity를 구한다.

        IDE_TEST( getMinMaxSelectivity( aTemplate,
                                        aStatInfo,
                                        aCompareNode,
                                        sColumnNode,
                                        sValueNode,
                                        & sSelectivity )
                  != IDE_SUCCESS );

    }
    else
    {
        sSelectivity = aCompareNode->node.module->selectivity;
    }

    *aSelectivity = sSelectivity;

    IDE_DASSERT_MSG( sSelectivity >= 0 && sSelectivity <= 1,
                     "Selectivity : %"ID_DOUBLE_G_FMT"\n",
                     sSelectivity );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoSelectivity::getMinMaxSelectivity( qcTemplate    * aTemplate,
                                      qmoStatistics * aStatInfo,
                                      qtcNode       * aCompareNode,
                                      qtcNode       * aColumnNode,
                                      qtcNode       * aValueNode,
                                      SDouble       * aSelectivity )
{
/******************************************************************************
 *
 * Description : MIN, MAX 를 이용한 selectivity 반환
 *               MTD_SELECTIVITY_ENABLE 이고 MTD_GROUP_MISC 가 아닌
 *               아래와 같은 타입에 한해 본 함수가 수행된다.
 *             - MTD_GROUP_TEXT : mtdVarchar, mtdChar, mtdNVarchar, mtdNChar,
 *             - MTD_GROUP_DATE : mtdDate
 *             - MTD_GROUP_NUMBER : mtdInteger, mtdDouble, mtdBigint,
 *                                  mtdReal, mtdSmallint, mtdFloat
 *
 *     부등호 연산자의 selectivity 계산
 *
 *     1. i1 > N
 *        . selectivity = ( MAX - N ) / ( MAX - MIN )
 *
 *     2. i1 < N
 *        . selectivity = ( N - MIN ) / ( MAX - MIN )
 *
 *     3. BETWEEN
 *        between의 경우는 max/min value를 찾고, value의 적합성까지
 *        검사하기 위해서는, 각 데이터 타입별로 고려되어야 하므로
 *        구현의 복잡도가 높아지기 때문에, min/max value를 다음과 같이
 *        지정한다. [value에 대한 적합성 검사는 mt에서 selectivity
 *        계산 도중에 detect되고, 이 경우, default selectivity를 반환한다.]
 *
 *        i1 between A(min value) and B(max value)
 *        . selectivity = ( max value - min value ) / ( MAX - MIN )
 *
 * Implementation :
 *
 *     1. column과 const value노드를 찾는다.
 *     2. column의 mtdModule->selectivity() 함수를 이용한다.
 *        mt에서는 날짜형과 숫자형에 대해서 그 dataType에 맞는
 *        selectivity 계산 함수[mtdModule->selectivity]를 제공한다.
 *
 *****************************************************************************/

    qmoColCardInfo  * sColCardInfo;
    UInt              sColumnType = 0;
    mtcColumn       * sColumnColumn;
    mtcColumn       * sMinValueColumn;
    mtcColumn       * sMaxValueColumn;
    void            * sColumnMaxValue;
    void            * sColumnMinValue;
    void            * sMaxValue;
    void            * sMinValue;
    mtdDoubleType     sDoubleColumnMaxValue;
    mtdDoubleType     sDoubleColumnMinValue;
    mtdDoubleType     sDoubleMaxValue;
    mtdDoubleType     sDoubleMinValue;

    SDouble           sColumnNDV;
    SDouble           sSelectivity;
    SDouble           sBoundForm;

    IDU_FIT_POINT_FATAL( "qmoSelectivity::getMinMaxSelectivity::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aTemplate    != NULL );
    IDE_DASSERT( aStatInfo    != NULL );
    IDE_DASSERT( aCompareNode != NULL );
    IDE_DASSERT( aColumnNode  != NULL );
    IDE_DASSERT( aValueNode   != NULL );
    IDE_DASSERT( aSelectivity != NULL );
    IDE_DASSERT( aStatInfo->isValidStat == ID_TRUE );

    //--------------------------------------
    // 기본 초기화
    //--------------------------------------

    sColCardInfo    = aStatInfo->colCardInfo;
    sColumnNDV      = sColCardInfo[aColumnNode->node.column].columnNDV;
    sColumnMaxValue =
        (void *)(sColCardInfo[aColumnNode->node.column].maxValue);
    sColumnMinValue =
        (void *)(sColCardInfo[aColumnNode->node.column].minValue);

    sColumnColumn = &( aTemplate->tmplate.
                       rows[aColumnNode->node.table].
                       columns[aColumnNode->node.column] );
    sColumnType = ( sColumnColumn->module->flag & MTD_GROUP_MASK );

    switch( aCompareNode->node.module->lflag & MTC_NODE_OPERATOR_MASK )
    {
        case( MTC_NODE_OPERATOR_GREATER ) :
        case( MTC_NODE_OPERATOR_GREATER_EQUAL ) :
            // >, >=
            if( aCompareNode->indexArgument == 0 )
            {
                // 예: i1>1, i1>=1
                sMaxValue = sColumnMaxValue;
                sMaxValueColumn = sColumnColumn;

                IDE_TEST( qtc::calculate( aValueNode, aTemplate )
                          != IDE_SUCCESS );
                sMinValue = aTemplate->tmplate.stack->value;
                sMinValueColumn = aTemplate->tmplate.stack->column;
            }
            else
            {
                // 예: 1>i1, 1>=i1
                IDE_TEST( qtc::calculate( aValueNode, aTemplate )
                          != IDE_SUCCESS );
                sMaxValue = aTemplate->tmplate.stack->value;
                sMaxValueColumn = aTemplate->tmplate.stack->column;

                sMinValue = sColumnMinValue;
                sMinValueColumn = sColumnColumn;
            }

            break;
        case( MTC_NODE_OPERATOR_LESS ) :
        case( MTC_NODE_OPERATOR_LESS_EQUAL ) :
            // <, <=
            if( aCompareNode->indexArgument == 0 )
            {
                // 예: i1<1, i1<=1
                IDE_TEST( qtc::calculate( aValueNode, aTemplate )
                          != IDE_SUCCESS );
                sMaxValue = aTemplate->tmplate.stack->value;
                sMaxValueColumn = aTemplate->tmplate.stack->column;

                sMinValue = sColumnMinValue;
                sMinValueColumn = sColumnColumn;
            }
            else
            {
                // 예: 1<i1, 1<=i1
                sMaxValue = sColumnMaxValue;
                sMaxValueColumn = sColumnColumn;

                IDE_TEST( qtc::calculate( aValueNode, aTemplate )
                          != IDE_SUCCESS );
                sMinValue = aTemplate->tmplate.stack->value;
                sMinValueColumn = aTemplate->tmplate.stack->column;
            }
            break;
        case( MTC_NODE_OPERATOR_RANGED ) :
        case( MTC_NODE_OPERATOR_NOT_RANGED ) :
            // BETWEEN, NOT BETWEEN
            // 예: between A(min value) AND B(max value)
            IDE_TEST( qtc::calculate( aValueNode,
                                      aTemplate ) != IDE_SUCCESS );
            sMinValue = aTemplate->tmplate.stack->value;
            sMinValueColumn = aTemplate->tmplate.stack->column;

            IDE_TEST( qtc::calculate( (qtcNode *)(aValueNode->node.next),
                                      aTemplate )
                         != IDE_SUCCESS );
            sMaxValue = aTemplate->tmplate.stack->value;
            sMaxValueColumn = aTemplate->tmplate.stack->column;

            break;
        default :
            IDE_RAISE( ERR_INVALID_NODE_OPERATOR );
    }

    // 보정인자 획득
    sBoundForm = ( aCompareNode->node.module == &mtfLessEqual ||
                   aCompareNode->node.module == &mtfGreaterEqual ) ?
                 ( 1 / sColumnNDV ):
                 ( aCompareNode->node.module == &mtfBetween ||
                   aCompareNode->node.module == &mtfNotBetween ) ?
                 ( 2 / sColumnNDV ): 0;

    //--------------------------------------
    // MIN/MAX를 이용한 selectivity 계산
    //--------------------------------------

    // BUG-17791
    if( sColumnType == MTD_GROUP_NUMBER )
    {
        // PROJ-1364
        // 숫자형 계열인 경우
        // 동일계열의 인덱스 사용으로 인해
        // 두 비교대상의 data type이 틀릴 수 있으므로
        // double로 변환해서, selectivity를 구한다.
        // 예) smallint_col > 3.5

        IDE_TEST ( getConvertToDoubleValue( sColumnColumn,
                                            sMaxValueColumn,
                                            sMinValueColumn,
                                            sColumnMaxValue,
                                            sColumnMinValue,
                                            sMaxValue,
                                            sMinValue,
                                            & sDoubleColumnMaxValue,
                                            & sDoubleColumnMinValue,
                                            & sDoubleMaxValue,
                                            & sDoubleMinValue )
                   != IDE_SUCCESS );

        sSelectivity = mtdDouble.selectivity( (void *)&sDoubleColumnMaxValue,
                                              (void *)&sDoubleColumnMinValue,
                                              (void *)&sDoubleMaxValue,
                                              (void *)&sDoubleMinValue,
                                              sBoundForm,
                                              aStatInfo->totalRecordCnt );
    }
    else
    {
        // PROJ-1484
        // 숫자형이 아닌 경우 (문자열 선택도 추정 추가)
        // ex) DATE, CHAR(n), VARCHAR(n)

        sSelectivity = sColumnColumn->module->selectivity( sColumnMaxValue,
                                                           sColumnMinValue,
                                                           sMaxValue,
                                                           sMinValue,
                                                           sBoundForm,
                                                           aStatInfo->totalRecordCnt );
    }


    // NOT BETWEEN selectivity
    sSelectivity = ( aCompareNode->node.module == &mtfNotBetween ) ?
                   ( 1 - sSelectivity ): sSelectivity;

    *aSelectivity = sSelectivity;

    IDE_DASSERT_MSG( sSelectivity >= 0 && sSelectivity <= 1,
                     "Selectivity : %"ID_DOUBLE_G_FMT"\n",
                     sSelectivity );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_NODE_OPERATOR );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmoSelectivity::getMinMaxSelectivity",
                                  "Invalid node operator" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoSelectivity::getConvertToDoubleValue( mtcColumn     * aColumnColumn,
                                         mtcColumn     * aMaxValueColumn,
                                         mtcColumn     * aMinValueColumn,
                                         void          * aColumnMaxValue,
                                         void          * aColumnMinValue,
                                         void          * aMaxValue,
                                         void          * aMinValue,
                                         mtdDoubleType * aDoubleColumnMaxValue,
                                         mtdDoubleType * aDoubleColumnMinValue,
                                         mtdDoubleType * aDoubleMaxValue,
                                         mtdDoubleType * aDoubleMinValue )
{
/***********************************************************************
 *
 * Description : NUMBER group 의 double 형 변환값 반환
 *
 * Implementation : double type으로의 conversion 수행
 *
 ***********************************************************************/

    mtdValueInfo  sColumnMaxInfo;
    mtdValueInfo  sColumnMinInfo;
    mtdValueInfo  sValueMaxInfo;
    mtdValueInfo  sValueMinInfo;

    IDU_FIT_POINT_FATAL( "qmoSelectivity::getConvertToDoubleValue::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aColumnColumn         != NULL );
    IDE_DASSERT( aMaxValueColumn       != NULL );
    IDE_DASSERT( aMinValueColumn       != NULL );
    IDE_DASSERT( aColumnMaxValue       != NULL );
    IDE_DASSERT( aColumnMinValue       != NULL );
    IDE_DASSERT( aMaxValue             != NULL );
    IDE_DASSERT( aMinValue             != NULL );
    IDE_DASSERT( aDoubleColumnMaxValue != NULL );
    IDE_DASSERT( aDoubleColumnMinValue != NULL );
    IDE_DASSERT( aDoubleMaxValue       != NULL );
    IDE_DASSERT( aDoubleMinValue       != NULL );

    //--------------------------------------
    // Double 형 변환
    //--------------------------------------

    // PROJ-1364
    // 숫자형 계열인 경우
    // 동일계열의 인덱스 사용으로 인해
    // 두 비교대상의 data type이 틀릴 수 있으므로
    // double로 변환해서, selectivity를 구한다.
    // 예) smallint_col < numeric'9'
    // 예) smallint_col > 3 and smallint_col < numeric'9'

    sColumnMaxInfo.column = aColumnColumn;
    sColumnMaxInfo.value  = aColumnMaxValue;
    sColumnMaxInfo.flag   = MTD_OFFSET_USELESS;

    mtd::convertToDoubleType4MtdValue( &sColumnMaxInfo,
                                       aDoubleColumnMaxValue );

    sColumnMinInfo.column = aColumnColumn;
    sColumnMinInfo.value  = aColumnMinValue;
    sColumnMinInfo.flag   = MTD_OFFSET_USELESS;

    mtd::convertToDoubleType4MtdValue( &sColumnMinInfo,
                                       aDoubleColumnMinValue );

    sValueMaxInfo.column = aMaxValueColumn;
    sValueMaxInfo.value  = aMaxValue;
    sValueMaxInfo.flag   = MTD_OFFSET_USELESS;

    mtd::convertToDoubleType4MtdValue( &sValueMaxInfo,
                                       aDoubleMaxValue );

    sValueMinInfo.column = aMinValueColumn;
    sValueMinInfo.value  = aMinValue;
    sValueMinInfo.flag   = MTD_OFFSET_USELESS;

    mtd::convertToDoubleType4MtdValue( &sValueMinInfo,
                                       aDoubleMinValue );

    return IDE_SUCCESS;
}


IDE_RC
qmoSelectivity::getLikeSelectivity( qcTemplate    * aTemplate,
                                    qmoStatistics * aStatInfo,
                                    qmoPredicate  * aPredicate,
                                    qtcNode       * aCompareNode,
                                    SDouble       * aSelectivity,
                                    idBool          aInExecutionTime )
{
/******************************************************************************
 *
 * Description : LIKE 에 대한 unit selectivity 계산하고
 *               QMO_PRED_HOST_OPTIMIZE_TRUE 세팅
 *               - QMO_PRED_IS_DYNAMIC_OPTIMIZABLE 이고
 *               - Indexable predicate 이고 (one column)
 *               - Variable value (prepare time) 이고
 *               - MTD_SELECTIVITY_ENABLE column
 *
 *   cf) NOT LIKE 는 MTC_NODE_INDEX_UNUSABLE 로 항상 QMO_COLUMNID_NON_INDEXABLE
 *
 * Implementation :
 *
 *   1. 통계정보 수집이고
 *      Indexable predicate (one column and value) 이고
 *      Fixed value 또는 variable value (execution time) 이고
 *      MTD_SELECTIVITY_ENABLE column
 *   1.1. Exact match : S = 1 / columnNDV
 *        ex) i1 LIKE 'ab'
 *   1.2. Prefix or Pattern match) : S = DS
 *        ex) Prefix match : i1 LIKE 'ab%', i1 LIKE 'ab%d', i1 LIKE 'ab%d%'
 *            Pattern match : i1 LIKE '%ab' (실제 non-indexable 로 처리)
 *
 *   2. 통계정보 미수집 또는
 *      Non-indexable predicate 또는
 *      Variable value (prepare time) 또는
 *      MTD_SELECTIVITY_DISABLE column : S = DS
 *      ex) t1.i1 LIKE t2.i2, i1 LIKE (select 'a' from t2),
 *          i1 LIKE '1%' (i1 이 연산불가 타입),
 *          i1 LIKE :a (prepare time)
 *
 *****************************************************************************/

    qmoColCardInfo * sColCardInfo;
    mtcColumn      * sColumnColumn;
    qtcNode        * sColumnNode;
    qtcNode        * sValueNode;
    SDouble          sColumnNDV;
    SDouble          sDefaultSelectivity;
    SDouble          sSelectivity;

    mtdCharType      sEscapeEmpty = { 0, { 0 } };
    mtdCharType    * sPattern;
    mtdCharType    * sEscape;
    UShort           sKeyLength;

    IDU_FIT_POINT_FATAL( "qmoSelectivity::getLikeSelectivity::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aTemplate    != NULL );
    IDE_DASSERT( aStatInfo    != NULL );
    IDE_DASSERT( aPredicate   != NULL );
    IDE_DASSERT( aCompareNode != NULL );
    IDE_DASSERT( aCompareNode->node.module == &mtfLike ||
                 aCompareNode->node.module == &mtfNotLike );
    IDE_DASSERT( aSelectivity != NULL );

    //--------------------------------------
    // 기본 초기화
    //--------------------------------------

    sColCardInfo = aStatInfo->colCardInfo;
    sDefaultSelectivity = aCompareNode->node.module->selectivity;

    // column node와 value node 획득
    if( aCompareNode->indexArgument == 0 )
    {
        sColumnNode = (qtcNode *)(aCompareNode->node.arguments);
        sValueNode  = (qtcNode *)(aCompareNode->node.arguments->next);
    }
    else
    {
        sColumnNode = (qtcNode *)(aCompareNode->node.arguments->next);
        sValueNode  = (qtcNode *)(aCompareNode->node.arguments);
    }

    sColumnColumn = &( aTemplate->tmplate.
                       rows[sColumnNode->node.table].
                       columns[sColumnNode->node.column] );

    //--------------------------------------
    // QMO_PRED_HOST_OPTIMIZE_TRUE 세팅
    //--------------------------------------

    // BUG-43065 외부 참조 컬럼이 있을때는 host 변수 최적화를 하면 안됨
    if( ( QMO_PRED_IS_DYNAMIC_OPTIMIZABLE( aPredicate ) == ID_TRUE ) &&
        ( aPredicate->id != QMO_COLUMNID_NON_INDEXABLE ) &&
        ( aInExecutionTime == ID_FALSE ) &&
        ( QMO_PRED_IS_VARIABLE( aPredicate ) == ID_TRUE ) &&
        ( ( sColumnColumn->module->flag & MTD_SELECTIVITY_MASK )
          != MTD_SELECTIVITY_DISABLE ) &&
        ( qtc::haveDependencies( &sValueNode->depInfo ) == ID_FALSE ) )
    {
        // qmoPredicate->flag에 host optimization flag를 세팅한다.
        aPredicate->flag &= ~QMO_PRED_HOST_OPTIMIZE_MASK;
        aPredicate->flag |= QMO_PRED_HOST_OPTIMIZE_TRUE;
    }
    else
    {
        // Nothing to do.
    }

    //--------------------------------------
    // Selectivity 계산
    //--------------------------------------

    if( ( aStatInfo->isValidStat == ID_TRUE ) &&
        ( aPredicate->id != QMO_COLUMNID_NON_INDEXABLE ) &&
        ( ( aInExecutionTime == ID_TRUE ) ||
          ( QMO_PRED_IS_VARIABLE( aPredicate ) == ID_FALSE ) ) &&
        ( ( sColumnColumn->module->flag & MTD_SELECTIVITY_MASK )
          != MTD_SELECTIVITY_DISABLE ) &&
        ( qtc::haveDependencies( &sValueNode->depInfo ) == ID_FALSE ) )
    {
        // NOT LIKE 는 MTC_NODE_INDEX_UNUSABLE 로 QMO_COLUMNID_NON_INDEXABLE
        IDE_DASSERT( aCompareNode->node.module == &mtfLike );

        // 1. 통계정보 수집이고
        //    Indexable predicate 이고
        //    Fixed value 또는 Variable value (execution time) 이고
        //    MTD_SELECTIVITY_ENABLE column

        // LIKE predicate 의 패턴 문자열 획득
        IDE_TEST( qtc::calculate( sValueNode, aTemplate ) != IDE_SUCCESS );

        sPattern = (mtdCharType*)aTemplate->tmplate.stack->value;

        if( sValueNode->node.next != NULL )
        {
            IDE_TEST( qtc::calculate( (qtcNode *)(sValueNode->node.next),
                                      aTemplate )
                      != IDE_SUCCESS );
            sEscape = (mtdCharType*)aTemplate->tmplate.stack->value;
        }
        else
        {
            // escape 문자가 존재하지 않는 경우
            sEscape = &sEscapeEmpty;
        }

        // Prefix key length 획득
        IDE_TEST( getLikeKeyLength( (const mtdCharType*) sPattern,
                                    (const mtdCharType*) sEscape,
                                    & sKeyLength )
                  != IDE_SUCCESS );

        // Prefix key length 를 이용하여 match type 판단
        if( sKeyLength != 0 && sKeyLength == sPattern->length )
        {
            // 1.1. Exact match
            sColumnNDV = sColCardInfo[sColumnNode->node.column].columnNDV;
            sSelectivity = 1 / sColumnNDV;
        }
        else
        {
            // 1.2. Pattern match(sKeyLength == 0) : 사실상 non-indexable
            //      Prefix match(sKeyLength != sPattern->length)

            sSelectivity = sDefaultSelectivity;
        }
    }
    else
    {
        // 2. 통계정보 미수집 또는
        //    Non-indexable predicate 또는
        //    Variable value (prepare time) 또는
        //    MTD_SELECTIVITY_DISABLE column
        //    (범위 값에 대한 선택도를 추정할 수 없는 data type)

        sSelectivity = sDefaultSelectivity;
    }

    *aSelectivity = sSelectivity;

    IDE_DASSERT_MSG( sSelectivity >= 0 && sSelectivity <= 1,
                     "Selectivity : %"ID_DOUBLE_G_FMT"\n",
                     sSelectivity );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoSelectivity::getLikeKeyLength( const mtdCharType   * aSource,
                                  const mtdCharType   * aEscape,
                                  UShort              * aKeyLength )
{
/******************************************************************************
 * Name: getLikeKeyLength() -- LIKE KEY의 prefix key의 길이를 얻어옴
 *
 * Arguments:
 * aSource: LIKE절의 매칭 문자열
 * aEscape: escape 문자
 * aKeyLen: prefix key의 길이 (output)
 *
 * Description:
 * i1 LIKE 'K%' 또는 i1 LIKE 'K_' 에서
 * K의 길이를 얻어 오는 함수
 *
 * 1) exact   match:  aKeyLen == aSource->length
 * 2) prefix  match:  aKeyLen <  aSource->length &&
 * aKeyLen != 0
 * 3) pattern match:  aKeyLen == 0
 *
 * 이 함수는 선택도를 추정하기 위해 LIKE절의 매칭 종류를 파악한다.
 * 따라서 멀티 바이트 문자셋(예:한글)을 고려하여 키의 길이를 파악하지 않는다.
 *
 *****************************************************************************/

    UChar       sEscape;     // escape 문자
    idBool      sNullEscape;
    UShort      sIdx;        // loop counter

    IDU_FIT_POINT_FATAL( "qmoSelectivity::getLikeKeyLength::__FT__" );

    IDE_DASSERT( aKeyLength != NULL );
    IDE_DASSERT( aSource    != NULL );
    IDE_DASSERT( aEscape    != NULL );

    // escape 문자 설정
    if( aEscape->length < 1 )
    {
        // escape 문자가 설정되어 있지 않은 경우
        sNullEscape = ID_TRUE;
    }
    else if( aEscape->length == 1 )
    {
        // escape 문자 할당
        sEscape = *(aEscape->value);
        sNullEscape = ID_FALSE;
    }
    else
    {
        // escape 문자의 길이가 1을 넘으면 안된다.
        IDE_RAISE( ERR_INVALID_ESCAPE );
    }

    sIdx = 0;
    *aKeyLength = 0;
    while( sIdx < aSource->length )
    {
        if( (sNullEscape == ID_FALSE) && (aSource->value[sIdx] == sEscape) )
        {
            // To Fix PR-13004
            // ABR 방지를 위하여 증가시킨 후 검사하여야 함
            sIdx++;

            // escape 문자인 경우,
            // escape 다음 문자가 '%','_' 문자인지 검사
            IDE_TEST_RAISE( sIdx >= aSource->length, ERR_INVALID_LITERAL );

            // To Fix BUG-12578
            IDE_TEST_RAISE( aSource->value[sIdx] != (UShort)'%' &&
                            aSource->value[sIdx] != (UShort)'_' &&
                            aSource->value[sIdx] != sEscape,
                            ERR_INVALID_LITERAL );
        }
        else if( aSource->value[sIdx] == (UShort)'%' ||
                 aSource->value[sIdx] == (UShort)'_' )
        {
            // 특수문자인 경우
            break;
        }
        else
        {
            // 일반 문자인 경우
        }
        sIdx++;
    }

    // get key length
    *aKeyLength = sIdx;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_ESCAPE )
    {
        IDE_SET(ideSetErrorCode( mtERR_ABORT_INVALID_ESCAPE ));
    }
    IDE_EXCEPTION( ERR_INVALID_LITERAL )
    {
        IDE_SET(ideSetErrorCode( mtERR_ABORT_INVALID_LITERAL_AFTER_ESCAPE ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoSelectivity::getInSelectivity( qcTemplate    * aTemplate,
                                  qmoStatistics * aStatInfo,
                                  qcDepInfo     * aDepInfo,
                                  qmoPredicate  * aPredicate,
                                  qtcNode       * aCompareNode,
                                  SDouble       * aSelectivity )
{
/******************************************************************************
 *
 * Description : IN(=ANY), NOT IN(!=ALL) 에 대한 unit selectivity 를 계산하고
 *               Indexable IN operator 에 한해
 *               QMO_PRED_INSUBQUERY_MASK (subquery keyRange 정보) 설정
 *
 * Implementation : Selectivity 계산식
 *
 *     1. 통계정보 수집이고 Indexable predicate 이고 LIST, Subquery value 형태
 *        ex) i1 IN (1,2), (i1,i2) IN ((1,1)), (i1,i2) IN ((1,1),(2,2))
 *            i1 IN (select i1 from t2), (i1,i2) IN (select i1,i2 from t2)
 *     => NOT IN, one side column LIST 는 non-indexable 이지만
 *        selectivity 획득이 가능하므로 indexable selectivity 계산식 적용
 *        ex) (i1, i2) NOT IN ((1,1))
 *      - ColumnLIST 에 대한 AND 확률 계산식 (n : column number)
 *        S(AND) = 1 / columnNDV = 1 / PRODUCT( columnNDVn )
 *      - ValueLIST 에 대한 OR 확률 계산식 (m : value number)
 *        S(OR) = 1 - PRODUCTm( 1 - S(AND) )
 *              = 1 - PRODUCTm( 1 - ( 1/columnNDVn ) )
 *
 *     2. 통계정보 미수집 또는 Non-indexable predicate
 *        S = PRODUCT( DSn )
 *        ex) i1 IN (i2,2), i1+1 IN (1,2), (i1,1) IN ((1,1), (2,1))
 *     => DS 를 사용하면 value list 의 인자수가 증가할수록 1 로 수렴
 *     => 아래와 같은 이유로 PRODUCT( DSn ) 을 채택
 *        Selectivity[i1 IN (1,2)] >
 *        Selectivity[(i1 IN (1,2)) AND (i2 IN (1,2))] >
 *        Selectivity[(i1, i2) IN ((1,1), (2,2))]
 * 
 *     3. NOT IN 보정
 *
 *****************************************************************************/

    qmoColCardInfo * sColCardInfo;
    qtcNode        * sColumnNode;
    qtcNode        * sValueNode;
    qtcNode        * sColumn;
    qtcNode        * sValue;
    SDouble          sColumnNDV;
    SDouble          sDefaultSelectivity;
    SDouble          sSelectivity;
    idBool           sIsIndexable;
    ULong            sValueCount;

    IDU_FIT_POINT_FATAL( "qmoSelectivity::getInSelectivity::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aTemplate    != NULL );
    IDE_DASSERT( aStatInfo    != NULL );
    IDE_DASSERT( aDepInfo     != NULL );
    IDE_DASSERT( aPredicate   != NULL );
    IDE_DASSERT( aCompareNode != NULL );
    IDE_DASSERT( aCompareNode->node.module == &mtfEqualAny ||
                 aCompareNode->node.module == &mtfNotEqualAll );
    IDE_DASSERT( aSelectivity != NULL );

    //--------------------------------------
    // 기본 초기화
    //--------------------------------------

    sColumnNDV = 1;
    sSelectivity = 1;
    sDefaultSelectivity = mtfEqualAny.selectivity;

    // column node와 value node 획득
    if( aCompareNode->indexArgument == 0 )
    {
        sColumnNode = (qtcNode *)(aCompareNode->node.arguments);
        sValueNode  = (qtcNode *)(aCompareNode->node.arguments->next);
    }
    else
    {
        sColumnNode = (qtcNode *)(aCompareNode->node.arguments->next);
        sValueNode  = (qtcNode *)(aCompareNode->node.arguments);
    }

    //--------------------------------------
    // IN subquery keyRange 정보 설정.
    //--------------------------------------
    // indexable predicate인 경우,
    // OR 노드 하위에 비교연산자가 여러개 있는 경우,
    // subquery가 존재하지 않는다.

    if( ( aPredicate->id != QMO_COLUMNID_NON_INDEXABLE ) &&
        ( aCompareNode->node.module == &mtfEqualAny ) )
    {
        if( ( sValueNode->node.lflag & MTC_NODE_OPERATOR_MASK )
            == MTC_NODE_OPERATOR_SUBQUERY )
        {
            aPredicate->flag &= ~QMO_PRED_INSUBQUERY_MASK;
            aPredicate->flag |= QMO_PRED_INSUBQUERY_EXIST;
        }
        else
        {
            aPredicate->flag &= ~QMO_PRED_INSUBQUERY_MASK;
            aPredicate->flag |= QMO_PRED_INSUBQUERY_ABSENT;
        }
    }
    else
    {
        // Nothing to do.
    }

    //--------------------------------------
    // Selectivity 계산
    //--------------------------------------

    // BUG-7814 : (i1, i2) NOT IN ((1,2))
    // - NOT IN operator 를 사용한 column LIST 형태는 non-indexable 로 분류
    // - PROJ-2242 : default selectivity 회피를 위해 indexable 처럼 처리

    if( ( aCompareNode->node.module == &mtfNotEqualAll ) &&
        ( sColumnNode->node.lflag & MTC_NODE_OPERATOR_MASK )
        == MTC_NODE_OPERATOR_LIST )
    {
        IDE_TEST( qmoPred::isExistColumnOneSide( aTemplate->stmt,
                                                 aCompareNode,
                                                 aDepInfo,
                                                 & sIsIndexable )
                  != IDE_SUCCESS );
    }
    else
    {
        sIsIndexable = ( aPredicate->id == QMO_COLUMNID_NON_INDEXABLE ) ?
                       ID_FALSE: ID_TRUE;
    }

    if ( ( aStatInfo->isValidStat == ID_TRUE ) &&
         ( sIsIndexable == ID_TRUE ) )
    {
        if ( ( sValueNode->node.lflag & MTC_NODE_OPERATOR_MASK )
            == MTC_NODE_OPERATOR_LIST )
        {
            // 1. 통계정보 수집 이고
            //    Indexable predicate (NOT IN one side column LIST 포함)
            //    LIST value 형태
            // ex) (i1,i2) IN ((1,1),(2,2)) -> (i1=1 AND i2=1) OR (i1=2 AND i2=2)
            // => Column LIST 에 대한 AND 확률식, Value LIST 에 대한 OR 확률식
 
            // columnNDV 획득 (AND 확률식)
            sColCardInfo = aStatInfo->colCardInfo;
 
            if ( ( sColumnNode->node.lflag & MTC_NODE_OPERATOR_MASK )
                 == MTC_NODE_OPERATOR_LIST )
            {
                // (i1,i2) IN ((1,1),(2,2))
                sColumn = (qtcNode *)(sColumnNode->node.arguments);
 
                while ( sColumn != NULL )
                {
                    sColumnNDV *= sColCardInfo[sColumn->node.column].columnNDV;
                    sColumn = (qtcNode *)(sColumn->node.next);
                }
            }
            else
            {
                // i1 IN (1,2)
                sColumnNDV = sColCardInfo[sColumnNode->node.column].columnNDV;
            }
 
            // selectivity 획득 (OR 확률식)
            sValue = (qtcNode *)(sValueNode->node.arguments);
 
            while ( sValue != NULL )
            {
                sSelectivity *= ( 1 - ( 1 / sColumnNDV ) );
                sValue = (qtcNode *)(sValue->node.next);
            }
 
            sSelectivity = ( 1 - sSelectivity );
        }
        else if ( ( sValueNode->node.lflag & MTC_NODE_OPERATOR_MASK )
            == MTC_NODE_OPERATOR_SUBQUERY )
        {
            // columnNDV 획득 (AND 확률식)
            sColCardInfo = aStatInfo->colCardInfo;
 
            if ( ( sColumnNode->node.lflag & MTC_NODE_OPERATOR_MASK )
                 == MTC_NODE_OPERATOR_LIST )
            {
                // (i1,i2) IN (subquery)
                sColumn = (qtcNode *)(sColumnNode->node.arguments);
 
                while ( sColumn != NULL )
                {
                    sColumnNDV *= sColCardInfo[sColumn->node.column].columnNDV;
                    sColumn = (qtcNode *)(sColumn->node.next);
                }
            }
            else
            {
                // i1 IN (subquery)
                sColumnNDV = sColCardInfo[sColumnNode->node.column].columnNDV;
            }

            // selectivity 획득 (OR 확률식)
            sValueCount = DOUBLE_TO_UINT64( sValueNode->subquery->myPlan->graph->costInfo.outputRecordCnt );

            while ( sValueCount != 0 )
            {
                sSelectivity *= ( 1 - ( 1 / sColumnNDV ) );
                sValueCount--;
            }
 
            sSelectivity = ( 1 - sSelectivity );
        }
        else
        {
            IDE_DASSERT( 0 );
        } 
    }
    else
    {
        // 2. 통계정보 미수집 또는
        //    Non-indexable predicate (NOT IN one side column LIST 제외)
        // => Column LIST 에 대한 AND 확률식

        if ( ( sColumnNode->node.lflag & MTC_NODE_OPERATOR_MASK )
            == MTC_NODE_OPERATOR_LIST )
        {
            sColumn = (qtcNode *)(sColumnNode->node.arguments);

            while ( sColumn != NULL )
            {
                sSelectivity *= sDefaultSelectivity;
                sColumn = (qtcNode *)(sColumn->node.next);
            }
        }
        else
        {
            sSelectivity = sDefaultSelectivity;
        }
    }

    // NOT IN (!=ALL) selectivity
    /* BUG-47702 <> 연산자가 있을 경우 Selectivity가 1인 INDEX에 대한 잘못된  index 선택 오류
     * NOT EQUAL 의 Selectivity 가 1 인경우 즉 모두 같은 값인경우 selectivity를 1로 준다.
     */
    if ( aCompareNode->node.module == &mtfNotEqualAll )
    {
        if ( sSelectivity < 1 )
        {
            sSelectivity = 1 - sSelectivity;
        }
        else
        {
            sSelectivity = 1;
        }
    }
    else
    {
        /* Nothing to do */
    }

    *aSelectivity = sSelectivity;

    IDE_DASSERT_MSG( sSelectivity >= 0 && sSelectivity <= 1,
                     "Selectivity : %"ID_DOUBLE_G_FMT"\n",
                     sSelectivity );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoSelectivity::getEqualsSelectivity( qmoStatistics * aStatInfo,
                                      qmoPredicate  * aPredicate,
                                      qtcNode       * aCompareNode,
                                      SDouble       * aSelectivity )
{
/******************************************************************************
 *
 * Description : EQUALS, NOT EQUALS 에 대한 unit selectivity 반환
 *           cf) NOT EQUALS 는 MTC_NODE_INDEX_UNUSABLE 로
 *               항상 QMO_COLUMNID_NON_INDEXABLE
 *
 * Implementation :
 *
 *      1. 통계정보 미수집 또는 Non-indexable predicate : S = DS
 *         ex) where EQUALS(TB1.F2, TB1.F2)  --> (TB1.F2 : GEOMETRY type)
 *      2. 통계정보 수집이고 Indexable predicate : S = 1 / columnNDV
 *         ex) where EQUALS(TB1.F2, GEOMETRY'POINT(3 3))
 *
 *****************************************************************************/

    qtcNode          * sColumnNode;
    qmoColCardInfo   * sColCardInfo;
    SDouble            sSelectivity;

    IDU_FIT_POINT_FATAL( "qmoSelectivity::getEqualsSelectivity::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatInfo    != NULL );
    IDE_DASSERT( aPredicate != NULL );
    IDE_DASSERT( aCompareNode != NULL );
    IDE_DASSERT( aCompareNode->node.module == &stfEquals ||
                 aCompareNode->node.module == &stfNotEquals );
    IDE_DASSERT( aSelectivity != NULL );

    //--------------------------------------
    // Selectivity 계산
    //--------------------------------------

    if( ( aStatInfo->isValidStat == ID_FALSE ) ||
        ( aPredicate->id == QMO_COLUMNID_NON_INDEXABLE ) )
    {
        // 1. 통계정보 미수집 또는 Non-indexable predicate
        sSelectivity = aCompareNode->node.module->selectivity;
    }
    else
    {
        // 2. 통계정보 수집이고 Indexable predicate
        IDE_DASSERT( aCompareNode->node.module == &stfEquals );

        // column node와 value node 획득
        if( aCompareNode->indexArgument == 0 )
        {
            sColumnNode = (qtcNode *)(aCompareNode->node.arguments);
        }
        else
        {
            sColumnNode = (qtcNode *)(aCompareNode->node.arguments->next);
        }

        sColCardInfo = aStatInfo->colCardInfo;
        sSelectivity = 1 / sColCardInfo[sColumnNode->node.column].columnNDV;

        // NOTEQUALS selectivity
        sSelectivity = ( aCompareNode->node.module == &stfNotEquals ) ?
                       1 - sSelectivity: sSelectivity;
    }

    *aSelectivity = sSelectivity;

    IDE_DASSERT_MSG( sSelectivity >= 0 && sSelectivity <= 1,
                     "Selectivity : %"ID_DOUBLE_G_FMT"\n",
                     sSelectivity );

    return IDE_SUCCESS;
}

IDE_RC
qmoSelectivity::getLnnvlSelectivity( qcTemplate    * aTemplate,
                                     qmoStatistics * aStatInfo,
                                     qcDepInfo     * aDepInfo,
                                     qmoPredicate  * aPredicate,
                                     qtcNode       * aCompareNode,
                                     SDouble       * aSelectivity,
                                     idBool          aInExecutionTime )
{
/******************************************************************************
 *
 * Description : LNNVL 의 unit selectivity 반환 (one table)
 *
 * Implementation :
 *
 *       LNNVL operator 의 인자 형태에 따라 다음과 같이 수행한다.
 *
 *       1. OR, AND (not normal form) : S = DS
 *       2. 중첩된 LNNVL : S = 1 - S(LNNVL)
 *       3. Compare predicate (only one) : S = 1 - S(one predicate)
 *
 *****************************************************************************/

    qtcNode   * sNode;
    SDouble     sSelectivity;

    IDU_FIT_POINT_FATAL( "qmoSelectivity::getLnnvlSelectivity::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aTemplate    != NULL );
    IDE_DASSERT( aStatInfo    != NULL );
    IDE_DASSERT( aDepInfo     != NULL );
    IDE_DASSERT( aPredicate   != NULL );
    IDE_DASSERT( aCompareNode != NULL );
    IDE_DASSERT( aSelectivity != NULL );
    IDE_DASSERT( aCompareNode->node.module == &mtfLnnvl );
    IDE_DASSERT( aCompareNode->node.arguments != NULL );

    sNode = (qtcNode *)aCompareNode->node.arguments;

    IDE_DASSERT( ( sNode->node.lflag & MTC_NODE_COMPARISON_MASK )
                 == MTC_NODE_COMPARISON_TRUE );

    if( sNode->node.module == &mtfLnnvl )
    {
        // 중첩된 LNNVL
        IDE_TEST( getLnnvlSelectivity( aTemplate,
                                       aStatInfo,
                                       aDepInfo,
                                       aPredicate,
                                       sNode,
                                       & sSelectivity,
                                       aInExecutionTime )
                  != IDE_SUCCESS );

        sSelectivity = 1 - sSelectivity;
    }
    else
    {
        // Compare predicate
        IDE_DASSERT( ( sNode->node.lflag & MTC_NODE_COMPARISON_MASK )
                     == MTC_NODE_COMPARISON_TRUE );

        IDE_TEST( getUnitSelectivity( aTemplate,
                                      aStatInfo,
                                      aDepInfo,
                                      aPredicate,
                                      sNode,
                                      & sSelectivity,
                                      aInExecutionTime )
                  != IDE_SUCCESS );

        sSelectivity = 1 - sSelectivity;
    }

    *aSelectivity = sSelectivity;

    IDE_DASSERT_MSG( sSelectivity >= 0 && sSelectivity <= 1,
                     "Selectivity : %"ID_DOUBLE_G_FMT"\n",
                     sSelectivity );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

SDouble qmoSelectivity::getEnhanceSelectivity4Join( qcStatement  * aStatement,
                                                    qmgGraph     * aGraph,
                                                    qmoStatistics* aStatInfo,
                                                    qtcNode      * aNode )
{
/******************************************************************************
 *
 * Description : BUG-37918 join selectivity 개선
 *
 * Implementation : 단순하게 그래프의 output 보다 NDV 가 큰 경우에는
 *                  output 을 대신 사용하도록 합니다.
 *
 *
 *****************************************************************************/

    SDouble          sSelectivity;
    qmgGraph       * sGraph;
    qcDepInfo        sNodeDepInfo;

    IDE_DASSERT( aStatInfo->isValidStat == ID_TRUE );
    IDE_DASSERT( QTC_IS_COLUMN( aStatement, aNode ) == ID_TRUE );

    if( aGraph != NULL )
    {
        qtc::dependencySet( aNode->node.table,
                            &sNodeDepInfo );

        if( qtc::dependencyContains( &aGraph->left->depInfo,
                                     &sNodeDepInfo ) == ID_TRUE )
        {
            sGraph = aGraph->left;
        }
        else if( qtc::dependencyContains( &aGraph->right->depInfo,
                                          &sNodeDepInfo ) == ID_TRUE )
        {
            sGraph = aGraph->right;
        }
        else
        {
            // 외부 참조 컬럼인 경우
            sGraph = NULL;
        }

        if( sGraph != NULL )
        {
            sSelectivity = IDL_MIN( aStatInfo->colCardInfo[aNode->node.column].columnNDV,
                                    sGraph->costInfo.outputRecordCnt );

            sSelectivity = 1 / sSelectivity;
        }
        else
        {
            sSelectivity = 1.0;
        }
    }
    else
    {
        sSelectivity = 1 / aStatInfo->colCardInfo[aNode->node.column].columnNDV;
    }

    IDE_DASSERT_MSG( sSelectivity >= 0 && sSelectivity <= 1,
                     "Selectivity : %"ID_DOUBLE_G_FMT"\n",
                     sSelectivity );

    return sSelectivity;
}

IDE_RC
qmoSelectivity::getUnitSelectivity4Join( qcStatement  * aStatement,
                                         qmgGraph     * aGraph,
                                         qtcNode      * aCompareNode,
                                         idBool       * aIsEqual,
                                         SDouble      * aSelectivity )
{
/******************************************************************************
 *
 * Description : qmgJoin 관련 qmoPredicate 의 unit predicate 에 대해
 *               unit selectivity 를 계산하고
 *               (=) operator 에 한해 다음인덱스 사용가능여부 반환
 *
 * Implementation :
 *
 *        1. =, != operator
 *        1.1. One column & one column : S = 1 / max( NDV(L), NDV(R) )
 *             ex) t1.i1=t2.i1
 *           - 양 쪽 컬럼 통계정보 있으면 : S = min( 1/NDV(L), 1/NDV(R) )
 *           - 한 쪽 컬럼 통계정보 있으면 : S = min( 1/NDV, DS )
 *           - 양 쪽 컬럼 통계정보 없으면 : S = DS
 *        1.2. One column & Etc
 *             ex) t1.i1=t2.i1+1, 1=t1.i1 or t2.i1=1 에서 각 unit predicate
 *           - 컬럼 통계정보 있으면 : S = min( 1/NDV, DS )
 *           - 컬럼 통계정보 없으면 : S = DS
 *             ex) t1.i1=t2.i1
 *        1.3. Etc & Etc : S = DS
 *             ex) t1.i1+t2.i1=1
 *        cf) != operator : S = 1 - S(=)
 *
 *        2. Etc. : S = DS
 *           ex) t1.i1 > t2.i1
 *
 *****************************************************************************/

    qmoStatistics  * sLeftStatInfo;
    qmoStatistics  * sRightStatInfo;
    qtcNode        * sLeftNode;
    qtcNode        * sRightNode;
    SDouble          sLeftSelectivity;
    SDouble          sRightSelectivity;
    SDouble          sSelectivity;
    idBool           sIsEqual = ID_TRUE;

    IDU_FIT_POINT_FATAL( "qmoSelectivity::getUnitSelectivity4Join::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement   != NULL );
    IDE_DASSERT( aCompareNode != NULL );
    IDE_DASSERT( aIsEqual     != NULL );
    IDE_DASSERT( aSelectivity != NULL );

    //--------------------------------------
    // Selectivity 계산
    //--------------------------------------

    // BUG-41876
    if ( ( aCompareNode->lflag & QTC_NODE_COLUMN_RID_MASK ) == QTC_NODE_COLUMN_RID_EXIST )
    {
        sSelectivity = aCompareNode->node.module->selectivity;
    }
    else
    {
        if( ( aCompareNode->node.module == &mtfEqual ) ||
            ( aCompareNode->node.module == &mtfNotEqual ) )
        {
            // 1. =, <> operator 에 한해 다음과 같이 계산

            sSelectivity = mtfEqual.selectivity;

            sLeftNode = (qtcNode *)(aCompareNode->node.arguments);
            sRightNode = (qtcNode *)(aCompareNode->node.arguments->next);

            if( QTC_IS_COLUMN( aStatement, sLeftNode ) == ID_TRUE )
            {
                sLeftStatInfo =
                    QC_SHARED_TMPLATE(aStatement)->tableMap[sLeftNode->node.table].
                    from->tableRef->statInfo;

                if( sLeftStatInfo->isValidStat == ID_TRUE )
                {
                    sLeftSelectivity = getEnhanceSelectivity4Join( aStatement,
                                                                   aGraph,
                                                                   sLeftStatInfo,
                                                                   sLeftNode );
                }
                else
                {
                    sLeftSelectivity = sSelectivity;
                }
            }
            else
            {
                sLeftSelectivity = sSelectivity;
            }

            if( QTC_IS_COLUMN( aStatement, sRightNode ) == ID_TRUE )
            {
                sRightStatInfo =
                    QC_SHARED_TMPLATE(aStatement)->tableMap[sRightNode->node.table].
                    from->tableRef->statInfo;

                if( sRightStatInfo->isValidStat == ID_TRUE )
                {
                    sRightSelectivity = getEnhanceSelectivity4Join( aStatement,
                                                                    aGraph,
                                                                    sRightStatInfo,
                                                                    sRightNode );
                }
                else
                {
                    sRightSelectivity = sSelectivity;
                }
            }
            else
            {
                sRightSelectivity = sSelectivity;
            }

            sSelectivity = IDL_MIN( sLeftSelectivity, sRightSelectivity );

            // NOT EQUAL selectivity
            sSelectivity = ( aCompareNode->node.module == &mtfNotEqual ) ?
                           1 - sSelectivity: sSelectivity;
        }
        else
        {
            // 2. Etc operator

            sSelectivity = aCompareNode->node.module->selectivity;
            sIsEqual = ID_FALSE;
        }
    }

    *aSelectivity = sSelectivity;
    *aIsEqual = sIsEqual;

    IDE_DASSERT_MSG( sSelectivity >= 0 && sSelectivity <= 1,
                     "Selectivity : %"ID_DOUBLE_G_FMT"\n",
                     sSelectivity );

    return IDE_SUCCESS;
}


IDE_RC
qmoSelectivity::getMySelectivity4PredList( qmoPredicate  * aPredList,
                                           SDouble       * aSelectivity )
{
/******************************************************************************
 *
 * Description : Join 관련 graph 의 qmoPredicate list 에 대해
 *               통합된 mySelecltivity 를 반환한다.
 *
 * Implementation : S = PRODUCT( mySelectivity for qmoPredicate list )
 *
 *****************************************************************************/

    qmoPredicate  * sPredicate;
    SDouble         sSelectivity;

    IDU_FIT_POINT_FATAL( "qmoSelectivity::getMySelectivity4PredList::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aSelectivity != NULL );
    
    //--------------------------------------
    // Selectivity 계산
    //--------------------------------------

    sSelectivity = 1;
    sPredicate = aPredList;

    while( sPredicate != NULL )
    {
        // BUG-37778 disk hash temp table size estimate
        // tpc-H Q20 에서 너무 작은 join selectivity 가 계산됨
        sSelectivity = IDL_MIN( sSelectivity, sPredicate->mySelectivity);
        // sSelectivity *= sPredicate->mySelectivity;

        sPredicate = sPredicate->next;
    }

    *aSelectivity = sSelectivity;

    IDE_DASSERT_MSG( sSelectivity >= 0 && sSelectivity <= 1,
                     "Selectivity : %"ID_DOUBLE_G_FMT"\n",
                     sSelectivity );

    return IDE_SUCCESS;
}


IDE_RC
qmoSelectivity::getSemiJoinSelectivity( qcStatement   * aStatement,
                                        qmgGraph      * aGraph,
                                        qcDepInfo     * aLeftDepInfo,
                                        qmoPredicate  * aPredList,
                                        idBool          aIsLeftSemi,
                                        idBool          aIsSetNext,
                                        SDouble       * aSelectivity )
{
/******************************************************************************
 *
 * Description : qmoPredicate list 또는 qmoPredicate 에 대한
 *               semi join selectivity 를 반환
 *
 * Implementation : 
 *
 *     1. aIsSetNext 가 TRUE 일 경우 qmoPredicate list 전체에 대해 수행
 *        Selectivity = PRODUCT( MS semi join for qmoPredicate )
 *
 *     1.1. qmoPredicate 이 복수 개의 unit predicate 로 구성되었을 경우
 *          ex) t1.i1=t2.i1 or t1.i2=t2.i2, t1.i1>t2.i1 or t1.i2<t2.i2
 *          MS(mySelectivity) = 1-PRODUCT(1-US)n    (OR 확률계산식)
 *
 *     1.2. qmoPredicate 이 한 개의 unit predicate 로 구성되었을 경우
 *          ex) t1.i1=t2.i1, t1.i1>t2.i1, t1.i2<t2.i2+1
 *          MS(mySelectivity) = US (unit semi join selectivity)
 *
 *     2. aIsSetNext 가 FALSE 일 경우 qmoPredicate 하나에 대해 수행
 *        Selectivity = semi join MySelectivity for qmoPredicate
 *                    = US (unit semi join selectivity)
 *
 *****************************************************************************/

    qmoPredicate   * sPredicate;
    qtcNode        * sCompareNode;
    SDouble          sSelectivity;
    SDouble          sMySelectivity;
    SDouble          sUnitSelectivity;
    idBool           sIsDNF;

    IDU_FIT_POINT_FATAL( "qmoSelectivity::getSemiJoinSelectivity::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement   != NULL );
    IDE_DASSERT( aLeftDepInfo != NULL );
    IDE_DASSERT( aSelectivity != NULL );

    //--------------------------------------
    // Selectivity 계산
    //--------------------------------------

    sSelectivity = 1;
    sPredicate   = aPredList;
    
    while( sPredicate != NULL )
    {
        sIsDNF = ID_FALSE;

        if( ( sPredicate->node->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
            == MTC_NODE_LOGICAL_CONDITION_TRUE )
        {
            sCompareNode = (qtcNode *)(sPredicate->node->node.arguments);
        }
        else
        {
            sCompareNode = sPredicate->node;
            sIsDNF = ID_TRUE;
        }

        //--------------------------------------
        // Semi join my selectivity 획득
        //--------------------------------------
        if( sCompareNode->node.next != NULL && sIsDNF == ID_FALSE )
        {
            // 1.1. OR 하위에 비교연산자가 여러개 있는 경우,
            //      OR 논리연산자에 대한 selectivity 계산.
            //      1 - (1-a)(1-b).....
            // ex) t1.i1=t2.i1 or t1.i2=t2.i2, t1.i1>t2.i1 or t1.i2<t2.i2

            sMySelectivity = 1;
            while( sCompareNode != NULL )
            {
                IDE_TEST( getUnitSelectivity4Semi( aStatement,
                                                   aGraph,
                                                   aLeftDepInfo,
                                                   sCompareNode,
                                                   aIsLeftSemi,
                                                   & sUnitSelectivity )
                          != IDE_SUCCESS );

                sMySelectivity *= ( 1 - sUnitSelectivity );

                sCompareNode = (qtcNode *)(sCompareNode->node.next);
            }

            sMySelectivity = ( 1 - sMySelectivity );
        }
        else
        {
            // 1.2. OR 노드 하위에 비교연산자가 하나만 존재
            // ex) t1.i1=t2.i1, t1.i1>t2.i1, t1.i2<t2.i2+1

            IDE_TEST( getUnitSelectivity4Semi( aStatement,
                                               aGraph,
                                               aLeftDepInfo,
                                               sCompareNode,
                                               aIsLeftSemi,
                                               & sMySelectivity )
                      != IDE_SUCCESS );
        }

        sSelectivity *= sMySelectivity;

        IDE_DASSERT_MSG( sMySelectivity >= 0 && sMySelectivity <= 1,
                         "mySelectivity : %"ID_DOUBLE_G_FMT"\n",
                         sMySelectivity );

        if (aIsSetNext == ID_FALSE)
        {
            // 2. Predicate list 가 아닌 하나의 qmoPredicate 에 대해서만 구함
            break;
        }
        else
        {
            sPredicate = sPredicate->next;
        }
    }

    *aSelectivity = sSelectivity;

    IDE_DASSERT_MSG( sSelectivity >= 0 && sSelectivity <= 1,
                     "Selectivity : %"ID_DOUBLE_G_FMT"\n",
                     sSelectivity );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoSelectivity::getAntiJoinSelectivity( qcStatement   * aStatement,
                                        qmgGraph      * aGraph,
                                        qcDepInfo     * aLeftDepInfo,
                                        qmoPredicate  * aPredList,
                                        idBool          aIsLeftAnti,
                                        idBool          aIsSetNext,
                                        SDouble       * aSelectivity )
{
/******************************************************************************
 *
 * Description : qmoPredicate list 또는 qmoPredicate 에 대한
 *               anti join selectivity 를 반환
 *
 * Implementation : 
 *
 *     1. aIsSetNext 가 TRUE 일 경우 qmoPredicate list 전체에 대해 수행
 *        Selectivity = PRODUCT( MS anti join for qmoPredicate )
 *
 *     1.1. qmoPredicate 이 복수 개의 unit predicate 로 구성되었을 경우
 *          ex) t1.i1=t2.i1 or t1.i2=t2.i2, t1.i1>t2.i1 or t1.i2<t2.i2
 *          MS(mySelectivity) = 1-PRODUCT(1-US)n    (OR 확률계산식)
 *
 *     1.2. qmoPredicate 이 한 개의 unit predicate 로 구성되었을 경우
 *          ex) t1.i1=t2.i1, t1.i1>t2.i1, t1.i2<t2.i2+1
 *          MS(mySelectivity) = US (unit anti join selectivity)
 *
 *     2. aIsSetNext 가 FALSE 일 경우 qmoPredicate 하나에 대해 수행
 *        Selectivity = anti join MySelectivity for qmoPredicate
 *                    = US (unit anti join selectivity)
 *
 *****************************************************************************/

    qmoPredicate   * sPredicate;
    qtcNode        * sCompareNode;
    SDouble          sSelectivity;
    SDouble          sMySelectivity;
    SDouble          sUnitSelectivity;
    idBool           sIsDNF;

    IDU_FIT_POINT_FATAL( "qmoSelectivity::getAntiJoinSelectivity::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement   != NULL );
    IDE_DASSERT( aLeftDepInfo != NULL );
    IDE_DASSERT( aSelectivity != NULL );

    //--------------------------------------
    // Selectivity 계산
    //--------------------------------------

    sSelectivity = 1;
    sPredicate   = aPredList;
    
    while( sPredicate != NULL )
    {
        sIsDNF = ID_FALSE;

        if( ( sPredicate->node->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
            == MTC_NODE_LOGICAL_CONDITION_TRUE )
        {
            sCompareNode = (qtcNode *)(sPredicate->node->node.arguments);
        }
        else
        {
            sCompareNode = sPredicate->node;
            sIsDNF = ID_TRUE;
        }

        //--------------------------------------
        // Anti join my selectivity 획득
        //--------------------------------------
        if( sCompareNode->node.next != NULL && sIsDNF == ID_FALSE )
        {
            // 1.1. OR 하위에 비교연산자가 여러개 있는 경우,
            //      OR 논리연산자에 대한 selectivity 계산.
            //      1 - (1-a)(1-b).....
            // ex) t1.i1=t2.i1 or t1.i2=t2.i2, t1.i1>t2.i1 or t1.i2<t2.i2

            sMySelectivity = 1;
            while( sCompareNode != NULL )
            {
                IDE_TEST( getUnitSelectivity4Anti( aStatement,
                                                   aGraph,
                                                   aLeftDepInfo,
                                                   sCompareNode,
                                                   aIsLeftAnti,
                                                   & sUnitSelectivity )
                          != IDE_SUCCESS );

                sMySelectivity *= ( 1 - sUnitSelectivity );

                sCompareNode = (qtcNode *)(sCompareNode->node.next);
            }

            sMySelectivity = ( 1 - sMySelectivity );
        }
        else
        {
            // 1.2. OR 노드 하위에 비교연산자가 하나만 존재
            // ex) t1.i1=t2.i1, t1.i1>t2.i1, t1.i2<t2.i2+1

            IDE_TEST( getUnitSelectivity4Anti( aStatement,
                                               aGraph,
                                               aLeftDepInfo,
                                               sCompareNode,
                                               aIsLeftAnti,
                                               & sMySelectivity )
                      != IDE_SUCCESS );
        }

        sSelectivity *= sMySelectivity;

        IDE_DASSERT_MSG( sMySelectivity >= 0 && sMySelectivity <= 1,
                         "mySelectivity : %"ID_DOUBLE_G_FMT"\n",
                         sMySelectivity );

        if (aIsSetNext == ID_FALSE)
        {
            // 2. Predicate list 가 아닌 하나의 qmoPredicate 에 대해서만 구함
            break;
        }
        else
        {
            sPredicate = sPredicate->next;
        }
    }

    *aSelectivity = sSelectivity;

    IDE_DASSERT_MSG( sSelectivity >= 0 && sSelectivity <= 1,
                     "Selectivity : %"ID_DOUBLE_G_FMT"\n",
                     sSelectivity );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoSelectivity::getUnitSelectivity4Anti( qcStatement  * aStatement,
                                         qmgGraph     * aGraph,
                                         qcDepInfo    * aLeftDepInfo,
                                         qtcNode      * aCompareNode,
                                         idBool         aIsLeftAnti,
                                         SDouble      * aSelectivity )
{
/******************************************************************************
 *
 * Description : Join 관련 qmoPredicate 의 하위 unit predicate 에 대해
 *               unit anti join selectivity 반환
 *
 * Implementation : 계산식은 다음과 같다.
 *
 *        1. One column = One column 형태
 *           ex) t1.i1=t2.i1
 *        1.1. 양 쪽 컬럼 통계정보 모두 있으면 : S = 1 - Semi join selectivity
 *        1.2. 양 쪽 컬럼 통계정보 하나라도 없으면 : S = Defalut selectivity
 *
 *        2. Etc : S = Defalut selectivity
 *           ex) t1.i1 > t2.i1, t1.i1=t2.i1+1,
 *               1=t1.i1 or t2.i1=1 에서 각 unit predicate
 *
 *****************************************************************************/

    qtcNode        * sLeftNode;
    qtcNode        * sRightNode;
    qmoStatistics  * sLeftStatInfo;
    qmoStatistics  * sRightStatInfo;
    SDouble          sSelectivity;

    IDU_FIT_POINT_FATAL( "qmoSelectivity::getUnitSelectivity4Anti::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement   != NULL );
    IDE_DASSERT( aLeftDepInfo != NULL );
    IDE_DASSERT( aCompareNode != NULL );
    IDE_DASSERT( aSelectivity != NULL );

    // BUG-41876
    IDE_DASSERT( ( aCompareNode->lflag & QTC_NODE_COLUMN_RID_MASK ) == QTC_NODE_COLUMN_RID_ABSENT );

    //--------------------------------------
    // Unit selectivity 계산
    //--------------------------------------

    sLeftNode = (qtcNode *)(aCompareNode->node.arguments);
    sRightNode = (qtcNode *)(aCompareNode->node.arguments->next);

    if( ( aCompareNode->node.module == &mtfEqual ) &&
        ( QTC_IS_COLUMN( aStatement, sLeftNode ) == ID_TRUE ) &&
        ( QTC_IS_COLUMN( aStatement, sRightNode ) == ID_TRUE ) )
    {
        // 1. One column = One column 형태 : t1.i1=t2.i1

        sLeftStatInfo =
            QC_SHARED_TMPLATE(aStatement)->tableMap[sLeftNode->node.table].
            from->tableRef->statInfo;

        sRightStatInfo =
            QC_SHARED_TMPLATE(aStatement)->tableMap[sRightNode->node.table].
            from->tableRef->statInfo;

        if( ( sLeftStatInfo->isValidStat == ID_TRUE ) &&
            ( sRightStatInfo->isValidStat == ID_TRUE ) )
        {
            // 1.1. 양 쪽 컬럼 통계정보 모두 있으면
            IDE_TEST( getUnitSelectivity4Semi( aStatement,
                                               aGraph,
                                               aLeftDepInfo,
                                               aCompareNode,
                                               aIsLeftAnti,
                                               & sSelectivity )
                      != IDE_SUCCESS );

            sSelectivity = 1 - sSelectivity;
        }
        else
        {
            // 1.2. 양 쪽 컬럼 통계정보 하나라도 없으면
            sSelectivity = aCompareNode->node.module->selectivity;
        }
    }
    else
    {
        // 2. Etc : t1.i1 > t2.i1, t1.i1=t2.i1+1,
        //          1=t1.i1 or t2.i1=1 에서 각 unit predicate
        sSelectivity = aCompareNode->node.module->selectivity;
    }

    *aSelectivity = sSelectivity;

    IDE_DASSERT_MSG( sSelectivity >= 0 && sSelectivity <= 1,
                     "Selectivity : %"ID_DOUBLE_G_FMT"\n",
                     sSelectivity );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoSelectivity::getUnitSelectivity4Semi( qcStatement  * aStatement,
                                         qmgGraph     * aGraph,
                                         qcDepInfo    * aLeftDepInfo,
                                         qtcNode      * aCompareNode,
                                         idBool         aIsLeftSemi,
                                         SDouble      * aSelectivity )
{
/******************************************************************************
 *
 * Description : Join 관련 qmoPredicate 의 하위 unit predicate 에 대해
 *               unit semi join selectivity 반환
 *
 * Implementation :
 *
 *   1. One column = One column 형태
 *      ex) t1.i1=t2.i1
 *   1.1. 양 쪽 컬럼 통계정보 모두 있으면
 *      - Left semi join selectivity = Right NDV / MAX( Left NDV, Right NDV )
 *      - Right semi join selectivity = Left NDV / MAX( Left NDV, Right NDV )
 *   1.2. 양 쪽 컬럼 통계정보 하나라도 없으면
 *        Semi join selectivity = Defalut selectivity
 *
 *   2. Etc : Semi join selectivity = Default selectivity
 *      ex) t1.i1 > t2.i1, t1.i1=t2.i1+1,
 *          1=t1.i1 or t2.i1=1 에서 각 unit predicate
 *
 *****************************************************************************/

    qtcNode        * sLeftNode;
    qtcNode        * sRightNode;
    qmoStatistics  * sLeftStatInfo;
    qmoStatistics  * sRightStatInfo;
    SDouble          sLeftSelectivity;
    SDouble          sRightSelectivity;
    SDouble          sMinSelectivity;
    SDouble          sSelectivity;

    IDU_FIT_POINT_FATAL( "qmoSelectivity::getUnitSelectivity4Semi::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement   != NULL );
    IDE_DASSERT( aLeftDepInfo != NULL );
    IDE_DASSERT( aCompareNode != NULL );
    IDE_DASSERT( aSelectivity != NULL );

    // BUG-41876
    IDE_DASSERT( ( aCompareNode->lflag & QTC_NODE_COLUMN_RID_MASK ) == QTC_NODE_COLUMN_RID_ABSENT );

    //--------------------------------------
    // Unit selectivity 계산
    //--------------------------------------

    sLeftNode = (qtcNode *)(aCompareNode->node.arguments);
    sRightNode = (qtcNode *)(aCompareNode->node.arguments->next);

    if( ( aCompareNode->node.module == &mtfEqual ) &&
        ( QTC_IS_COLUMN( aStatement, sLeftNode ) == ID_TRUE ) &&
        ( QTC_IS_COLUMN( aStatement, sRightNode ) == ID_TRUE ) )
    {
        // 1. One column = One column 형태 : t1.i1=t2.i1

        if ( qtc::dependencyContains( aLeftDepInfo,
                                      & sLeftNode->depInfo ) == ID_TRUE )
        {
            // Nothing to do.
        }
        else
        {
            sLeftNode = (qtcNode *)(aCompareNode->node.arguments->next);
            sRightNode = (qtcNode *)(aCompareNode->node.arguments);
        }

        // 통계정보 획득
        sLeftStatInfo =
            QC_SHARED_TMPLATE(aStatement)->tableMap[sLeftNode->node.table].
            from->tableRef->statInfo;

        sRightStatInfo =
            QC_SHARED_TMPLATE(aStatement)->tableMap[sRightNode->node.table].
            from->tableRef->statInfo;

        if( ( sLeftStatInfo->isValidStat == ID_TRUE ) &&
            ( sRightStatInfo->isValidStat == ID_TRUE ) )
        {
            // 1.1. 양 쪽 컬럼 통계정보 모두 있으면

            sLeftSelectivity = getEnhanceSelectivity4Join(
                                        aStatement,
                                        aGraph,
                                        sLeftStatInfo,
                                        sLeftNode );

            sRightSelectivity = getEnhanceSelectivity4Join(
                                        aStatement,
                                        aGraph,
                                        sRightStatInfo,
                                        sRightNode );

            sMinSelectivity = IDL_MIN( sLeftSelectivity, sRightSelectivity );

            if( aIsLeftSemi == ID_TRUE )
            {
                // Left semi join selectivity
                sSelectivity = sMinSelectivity / sRightSelectivity;
            }
            else
            {
                // Right semi join selectivity
                sSelectivity = sMinSelectivity / sLeftSelectivity;
            }
        }
        else
        {
            // 1.2. 양 쪽 컬럼 통계정보 하나라도 없으면
            sSelectivity = aCompareNode->node.module->selectivity;
        }
    }
    else
    {
        // 2. Etc : t1.i1 > t2.i1, t1.i1=t2.i1+1,
        //          1=t1.i1 or t2.i1=1 에서 각 unit predicate
        sSelectivity = aCompareNode->node.module->selectivity;
    }

    *aSelectivity = sSelectivity;

    IDE_DASSERT_MSG( sSelectivity >= 0 && sSelectivity <= 1,
                     "Selectivity : %"ID_DOUBLE_G_FMT"\n",
                     sSelectivity );

    return IDE_SUCCESS;
}


IDE_RC
qmoSelectivity::getSelectivity4JoinOrder( qcStatement  * aStatement,
                                          qmgGraph     * aJoinGraph,
                                          qmoPredicate * aJoinPred,
                                          SDouble      * aSelectivity )
{
/******************************************************************************
 *
 * Description : Join ordering 에 사용되는 joinOrderFactor 를 구하기 위한
 *               join selectivity 반환
 *
 * Implementation :
 *
 *       - Join ordering 의 대상은 최적화 이전이므로 predicate 미구분 상태
 *       - Join ordering 의 대상은 모두 qmgJoin 이어야 하나
 *         joinOrderFactor 를 구하는 과정에서 자식 노드가 outer join 일 경우
 *         자식 노드의 joinOrderFactor 를 이용하므로
 *         PROJ-2242 반영전 기존코드를 유지한다.
 *         => join ordering 개선시 고려하는 것이 좋을 듯 합니다.
 *
 *       1. Join selectivity 획득
 *          S = PRODUCT( qmoPredicate.mySelectivity for current graph depinfo )
 *       2. Join selectivity 보정
 *          child graph 가 SELECTION, PARTITION 일 경우에 한함
 *
 *****************************************************************************/

    qmoPredicate   * sJoinPred;
    qmoPredicate   * sOneTablePred;
    qmoStatistics  * sStatInfo;
    qmoIdxCardInfo * sIdxCardInfo;
    UInt             sIdxCnt;
    idBool           sIsRevise;
    idBool           sIsCurrent;
    SDouble          sJoinSelectivity;
    SDouble          sReviseSelectivity;
    SDouble          sOneTableSelectivity;
    UInt             i;

    IDU_FIT_POINT_FATAL( "qmoSelectivity::getSelectivity4JoinOrder::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aJoinGraph != NULL );
    IDE_DASSERT( aJoinPred  != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sJoinPred  = aJoinPred;
    sOneTablePred = NULL;
    sIsCurrent = ID_FALSE;
    sJoinSelectivity     = 1;
    sReviseSelectivity   = 1;
    sOneTableSelectivity = 1;

    //------------------------------------------
    // Selectivity 계산
    // - qmgJOIN.graph 의 최적화 이전이므로 predicate 미구분 상태
    //------------------------------------------

    while( sJoinPred != NULL )
    {
        // To Fix PR-8005
        // Join Predicate의 Dependencies를 넘겨야 함.
        IDE_TEST( qmo::currentJoinDependencies4JoinOrder( aJoinGraph,
                                                          & sJoinPred->node->depInfo,
                                                          & sIsCurrent )
                  != IDE_SUCCESS );

        if ( sIsCurrent == ID_TRUE )
        {
            // 현재 Join Graph의 join predicate인 경우
            sJoinSelectivity *= sJoinPred->mySelectivity;
        }
        else
        {
            // Nothing to do.
        }

        sJoinPred = sJoinPred->next;
    }

    //------------------------------------------
    // Selectivity 보정
    //------------------------------------------
    // PROJ-1502 PARTITIONED DISK TABLE
    // partitioned table에 대한 selectivity도 보정.

    if ( ( aJoinGraph->left->type == QMG_SELECTION ) ||
         ( aJoinGraph->left->type == QMG_PARTITION ) )
    {
        sStatInfo = aJoinGraph->left->myFrom->tableRef->statInfo;
        sIdxCnt = sStatInfo->indexCnt;

        for ( i = 0; i < sIdxCnt; i++ )
        {
            sIdxCardInfo = & sStatInfo->idxCardInfo[i];

            IDE_TEST( getReviseSelectivity4JoinOrder( aStatement,
                                                      aJoinGraph,
                                                      & aJoinGraph->left->depInfo,
                                                      sIdxCardInfo,
                                                      aJoinPred,
                                                      & sReviseSelectivity,
                                                      & sIsRevise )
                      != IDE_SUCCESS );

            if ( sIsRevise  == ID_TRUE )
            {
                if ( sReviseSelectivity > sJoinSelectivity )
                {
                    sJoinSelectivity = sReviseSelectivity;
                }
                else
                {
                    // Nothing to do.
                }
            }
            else
            {
                // Nothing to do.
            }
        }
    }
    else
    {
        // Nothing to do.
    }

    if ( ( aJoinGraph->right->type == QMG_SELECTION ) ||
         ( aJoinGraph->right->type == QMG_PARTITION ) )
    {
        sStatInfo = aJoinGraph->right->myFrom->tableRef->statInfo;
        sIdxCnt = sStatInfo->indexCnt;

        for ( i = 0; i < sIdxCnt; i++ )
        {
            sIdxCardInfo = & sStatInfo->idxCardInfo[i];

            IDE_TEST( getReviseSelectivity4JoinOrder( aStatement,
                                                      aJoinGraph,
                                                      & aJoinGraph->right->depInfo,
                                                      sIdxCardInfo,
                                                      aJoinPred,
                                                      & sReviseSelectivity,
                                                      & sIsRevise )
                      != IDE_SUCCESS );

            if ( sIsRevise  == ID_TRUE )
            {
                if ( sReviseSelectivity > sJoinSelectivity )
                {
                    sJoinSelectivity = sReviseSelectivity;
                }
                else
                {
                    // Nothing to do.
                }
            }
            else
            {
                // Nothing to do.
            }
        }
    }
    else
    {
        // Nothing to do.
    }

    //------------------------------------------
    // Left Outer Join과 Full Outer Join인 경우,
    // on Condition CNF의 one table predicate도 곱해야 함
    //------------------------------------------

    switch( aJoinGraph->type )
    {
        case QMG_INNER_JOIN :
        case QMG_SEMI_JOIN :
        case QMG_ANTI_JOIN :
            sOneTablePred = NULL;
            break;
        case QMG_LEFT_OUTER_JOIN :
            sOneTablePred = ((qmgLOJN*)aJoinGraph)->onConditionCNF->oneTablePredicate;
            break;
        case QMG_FULL_OUTER_JOIN :
            sOneTablePred = ((qmgFOJN*)aJoinGraph)->onConditionCNF->oneTablePredicate;
            break;
        default :
            IDE_DASSERT( 0 );
            break;
    }

    // ON 절에 대한 one table predicate selectivity 획득
    IDE_TEST( getMySelectivity4PredList( sOneTablePred,
                                         & sOneTableSelectivity )
              != IDE_SUCCESS );

    *aSelectivity = sJoinSelectivity * sOneTableSelectivity;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoSelectivity::getReviseSelectivity4JoinOrder( 
                                        qcStatement     * aStatement,
                                        qmgGraph        * aJoinGraph,
                                        qcDepInfo       * aChildDepInfo,
                                        qmoIdxCardInfo  * aIdxCardInfo,
                                        qmoPredicate    * aPredicate,
                                        SDouble         * aSelectivity,
                                        idBool          * aIsRevise )
{
/***********************************************************************
 *
 * Description : Join ordering 에 사용되는 joinOrderFactor 를 구하기 위한
 *               join selectivity 의 보정값 반환
 *
 * Implementation :
 *    (1) Selectivity 보정 여부 검사
 *        A. Composite Index인 경우
 *        B. Index의 각 칼럼이 Predicate에 모두 존재하고 그 predicate이 등호인
 *           경우
 *    (2) Selectivity 보정
 *        A. 각 Join Predicate에 대하여 다음을 수행한다.
 *           - 보정이 필요한 칼럼인 경우 : nothing to do
 *           - 보정이 필요없는 칼럼인 경우
 *             sSelectivity = sSelectivity * 현재 Predicate의 selectivity
 *        C. 보정이 필요한 칼럼이 하나라도 존재한 경우
 *           sSelectivity = sSelectivity * (1/composite index의 cardinality)
 *
 ***********************************************************************/

    mtcColumn      * sKeyCols;
    UInt             sKeyColCnt;
    qmoPredicate   * sCurPred;
    UInt             sIdxKeyColID;
    UInt             sColumnID;
    idBool           sIsEqualPredicate;
    idBool           sExistCommonID;
    idBool           sIsRevise;
    SDouble          sSelectivity;
    idBool           sIsCurrent;
    UInt             i;

    IDU_FIT_POINT_FATAL( "qmoSelectivity::getReviseSelectivity4JoinOrder::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement    != NULL );
    IDE_DASSERT( aJoinGraph    != NULL );
    IDE_DASSERT( aChildDepInfo != NULL ); // 보정 대상 qmgSelection or qmgPartition
    IDE_DASSERT( aIdxCardInfo  != NULL );
    IDE_DASSERT( aPredicate    != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sKeyCols           = aIdxCardInfo->index->keyColumns;
    sKeyColCnt         = aIdxCardInfo->index->keyColCount;
    sIsRevise          = ID_TRUE;
    sSelectivity       = 1;
    sIsCurrent         = ID_FALSE;

    //------------------------------------------
    // Join Predicate 초기화
    //------------------------------------------

    for ( sCurPred = aPredicate; sCurPred != NULL; sCurPred = sCurPred->next )
    {
        sCurPred->flag &= ~QMO_PRED_USABLE_COMP_IDX_MASK;
    }


    //------------------------------------------
    // Selectivity 보정 여부 검사
    //------------------------------------------

    if ( sKeyColCnt >= 2 )
    {
        //------------------------------------------
        // Composite Index 인 경우
        //------------------------------------------

        for ( i = 0; i < sKeyColCnt; i++ )
        {
            //------------------------------------------
            // Index의 각 칼럼이 Predicate에 존재하고,
            // 그 Predicate이 등호인지 검사
            //------------------------------------------

            sExistCommonID = ID_FALSE;

            sIdxKeyColID = sKeyCols[i].column.id;

            for ( sCurPred  = aPredicate;
                  sCurPred != NULL;
                  sCurPred  = sCurPred->next )
            {
                IDE_TEST( qmo::currentJoinDependencies4JoinOrder( aJoinGraph,
                                                                  & sCurPred->node->depInfo,
                                                                  & sIsCurrent )
                          != IDE_SUCCESS );

                if ( sIsCurrent == ID_FALSE )
                {
                    // 현재 predicate이 해당 Join의 join predicate이 아니면
                    // selectivity 계산에 포함시키지 않음
                    // 따라서, nothing to do
                }
                else
                {
                    // 현재 qmgJoin의 joinPredicate의 columnID
                    IDE_TEST( qmoPred::setColumnIDToJoinPred( aStatement,
                                                              sCurPred,
                                                              aChildDepInfo )
                              != IDE_SUCCESS );

                    sColumnID = sCurPred->id;

                    if ( sIdxKeyColID == sColumnID )
                    {
                        // 현 칼럼 ID와 동일
                        IDE_TEST( isEqualPredicate( sCurPred,
                                                    & sIsEqualPredicate )
                                  != IDE_SUCCESS );

                        if ( sIsEqualPredicate == ID_TRUE )
                        {
                            // Predicate이 등호
                            sExistCommonID = ID_TRUE;
                            sCurPred->flag &= ~QMO_PRED_USABLE_COMP_IDX_MASK;
                            sCurPred->flag |= QMO_PRED_USABLE_COMP_IDX_TRUE;
                        }
                        else
                        {
                            // Predicate이 등호가 아님 : nothing to do
                        }
                    }
                    else
                    {
                        // 현 칼럼 ID와 틀림 : nothing to do
                    }
                }
            }
            if ( sExistCommonID == ID_TRUE )
            {
                // 인덱스의 현재 칼럼과 동일한 column이 join predicate에
                // 존재하면, 다음 칼럼으로 진행
            }
            else
            {
                // Predicate 들 중에서 인덱스의 현재 column과 동일한 칼럼을
                // 가지는 등호 Predicate이 존재하지 않으면 composite index의
                // 모든 column에 대하여 Predicate이 있는 경우라는 조건을
                // 위배하므로 Selectivity 보정할 수 없음
                sIsRevise = ID_FALSE;
                break;
            }
        }
    }
    else
    {
        //------------------------------------------
        // Composite Index 가 아닌 경우 : selectivity 보정 필요없음
        //------------------------------------------
    }


    //------------------------------------------
    // 보정 여부에 따른 Selectivity 계산
    //------------------------------------------

    if ( sIsRevise == ID_TRUE )
    {
        // Selectivity 보정 필요함
        for ( sCurPred  = aPredicate;
              sCurPred != NULL;
              sCurPred  = sCurPred->next )
        {
            IDE_TEST( qmo::currentJoinDependencies4JoinOrder( aJoinGraph,
                                                              & sCurPred->node->depInfo,
                                                              & sIsCurrent )
                      != IDE_SUCCESS );

            if ( sIsCurrent == ID_TRUE )
            {
                if ( ( sCurPred->flag & QMO_PRED_USABLE_COMP_IDX_MASK )
                     == QMO_PRED_USABLE_COMP_IDX_TRUE )
                {
                    // skip
                }
                else
                {
                    sSelectivity *= sCurPred->mySelectivity;
                }
            }
            else
            {
                // 현재 predicate이 해당 Join의 join predicate이 아니면
                // selectivity 계산에 포함시키지 않음
                // 따라서, nothing to do
            }
        }
        sSelectivity *= ( 1 / aIdxCardInfo->KeyNDV);
    }
    else
    {
        // Selectivity 보정 필요 없음 : nothing to do
    }

    *aSelectivity = sSelectivity;
    *aIsRevise = sIsRevise;

    IDE_DASSERT_MSG( sSelectivity >= 0 && sSelectivity <= 1,
                     "Selectivity : %"ID_DOUBLE_G_FMT"\n",
                     sSelectivity );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoSelectivity::isEqualPredicate( qmoPredicate * aPredicate,
                                  idBool       * aIsEqual )
{
/***********************************************************************
 *
 * Description : = Predicate 인지 검사
 *
 * Implementation :
 *    (1) OR Predicate 검사
 *    (2) OR Predicate이 아닌 경우, = Predicate 검사
 *
 *     ex )
 *         < OR Predicate >
 *
 *           (OR)
 *             |
 *            (=)----------(=)
 *             |            |
 *         (T1.I1)-(1)    (T2.I1)-(2)
 *
 *         < Equal Predicate >
 *
 *           (OR)              (OR)
 *             |                 |
 *            (=)               (=)
 *             |                 |
 *          (T1.I1)-(1)      (T1.I1)-(T2.I1)
 *
 *
 ***********************************************************************/

    mtcNode * sNode;
    idBool    sIsOrPredicate;

    IDU_FIT_POINT_FATAL( "qmoSelectivity::isEqualPredicate::__FT__" );

    sIsOrPredicate = ID_FALSE;
    sNode = &aPredicate->node->node;

    // Or Predicate인지 검사
    if ( ( sNode->lflag & MTC_NODE_OPERATOR_MASK ) == MTC_NODE_OPERATOR_OR )
    {
        if ( sNode->arguments->next == NULL )
        {
            sIsOrPredicate = ID_FALSE;
            sNode = sNode->arguments;
        }
        else
        {
            sIsOrPredicate = ID_TRUE;
        }
    }
    else
    {
        sIsOrPredicate = ID_FALSE;
    }

    if ( sIsOrPredicate == ID_FALSE )
    {
        if ( sNode->module == & mtfEqual )
        {
            *aIsEqual = ID_TRUE;
        }
        else
        {
            *aIsEqual = ID_FALSE;
        }
    }
    else
    {
        *aIsEqual = ID_FALSE;
    }

    return IDE_SUCCESS;
}

IDE_RC qmoSelectivity::setSetRecursiveOutputCnt( SDouble   aLeftOutputRecordCnt,
                                                 SDouble   aRightOutputRecordCnt,
                                                 SDouble * aOutputRecordCnt )
{
/******************************************************************************
 *
 * Description : PROJ-2582 recursvie with
 *      qmgSetRecursive 에 대한 outputRecordCnt 계산
 *
 * Implementation :
 *
 *     1. outputRecordCnt 획득
 *         left outputRecordCnt는 레코드 개수 만큼 쌓인다.
 *         right outputRecordCnt는 left의 레코드 개수 만큼 recursive 하게 수행 된다.
 *
 *         recursive selectivity = T(R) / T(L) = S
 *
 *         output record count = T(L) + T(R)     + S * T(R)   + S^2 * T(R) + ...
 *                             = T(L) + S * T(L) + S^2 * T(L) + S^3 * T(L) + ...
 *                             = T(L) * T(L) / ( T(L) - T(R) )
 *
 *     2. outputRecordCnt 최소값 보정
 *
 *****************************************************************************/

    SDouble sOutputRecordCnt = 0;
    SDouble sLeftOutputRecordCnt = 0;
    SDouble sRightOutputRecordCnt = 0;

    IDU_FIT_POINT_FATAL( "qmoSelectivity::setSetRecursiveOutputCnt::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aOutputRecordCnt != NULL );
    IDE_DASSERT( aLeftOutputRecordCnt  > 0 );
    IDE_DASSERT( aRightOutputRecordCnt > 0 );

    //--------------------------------------
    // LeftOutputRecordCnt, RightOutputRecordCnt 보정
    //--------------------------------------

    sLeftOutputRecordCnt  = aLeftOutputRecordCnt;
    sRightOutputRecordCnt = aRightOutputRecordCnt;

    // R이 더 큰 경우는 무한반복하는 경우로 보고
    // R이 더 작은 경우에 대해서만 예측하도록 한다.
    // R이 더 작도록 보정하여 예측한다.
    if ( sLeftOutputRecordCnt <= sRightOutputRecordCnt )
    {
        sRightOutputRecordCnt = sLeftOutputRecordCnt * 0.9;
    }
    else
    {
        // Nothing to do.
    }
    
    //--------------------------------------
    // outputRecordCnt 계산
    //--------------------------------------
    
    // 1. outputRecordCnt 획득
    sOutputRecordCnt = sLeftOutputRecordCnt * sLeftOutputRecordCnt /
        ( sLeftOutputRecordCnt - sRightOutputRecordCnt );

    // 2. outputRecordCnt 최소값 보정
    if ( sOutputRecordCnt < 1 )
    {
        *aOutputRecordCnt = 1;
    }
    else
    {
        *aOutputRecordCnt = sOutputRecordCnt;
    }

    return IDE_SUCCESS;
}
