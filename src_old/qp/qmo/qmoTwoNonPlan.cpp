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
 * $Id: qmoTwoNonPlan.cpp 86088 2019-08-30 00:33:15Z ahra.cho $
 *
 * Description :
 *     Plan Generator
 *
 *     Two-child Non-Materialized Plan을 생성하기 위한 관리자이다.
 *
 *     다음과 같은 Plan Node의 생성을 관리한다.
 *         - JOIN 노드
 *         - MGJN 노드
 *         - LOJN 노드
 *         - FOJN 노드
 *         - AOJN 노드
 *         - CONC 노드
 *         - BUNI 노드
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <qmsParseTree.h>
#include <qmo.h>
#include <qmoTwoNonPlan.h>
#include <qci.h>

extern mtfModule mtfEqual;

IDE_RC
qmoTwoNonPlan::initJOIN( qcStatement   * aStatement,
                         qmsQuerySet   * aQuerySet,
                         qmsQuerySet   * aViewQuerySet,
                         qtcNode       * aJoinPredicate,
                         qtcNode       * aFilterPredicate,
                         qmoPredicate  * aPredicate,
                         qmnPlan       * aParent,
                         qmnPlan      ** aPlan )
{
/***********************************************************************
 *
 * Description : JOIN 노드를 생성한다
 *
 * Implementation :
 *     + 초기화 작업
 *         - qmncJOIN의 할당 및 초기화
 *     + 메인 작업
 *     + 마무리 작업
 *         - data 영역의 크기 계산
 *         - dependency의 처리
 *         - subquery의 처리
 *
 * TO DO
 *
 ***********************************************************************/

    qmncJOIN          * sJOIN;
    UInt                sDataNodeOffset;

    IDU_FIT_POINT_FATAL( "qmoTwoNonPlan::initJOIN::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aParent != NULL );

    //------------------------------------------------------------
    // 초기화 작업
    //------------------------------------------------------------

    // qmncJOIN의 할당및 초기화
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmncJOIN) , (void **)&sJOIN )
              != IDE_SUCCESS );

    QMO_INIT_PLAN_NODE( sJOIN ,
                        QC_SHARED_TMPLATE(aStatement) ,
                        QMN_JOIN ,
                        qmnJOIN ,
                        qmndJOIN,
                        sDataNodeOffset );

    //------------------------------------------------------------
    // 마무리 작업
    //------------------------------------------------------------

    IDE_TEST( qmc::pushResultDesc( aStatement,
                                   aQuerySet,
                                   ID_FALSE,
                                   aParent->resultDesc,
                                   &sJOIN->plan.resultDesc )
              != IDE_SUCCESS );

    if( aJoinPredicate != NULL )
    {
        IDE_TEST( qmc::appendPredicate( aStatement,
                                        aQuerySet,
                                        &sJOIN->plan.resultDesc,
                                        aJoinPredicate )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    if( aFilterPredicate != NULL )
    {
        sJOIN->filter = aFilterPredicate;

        IDE_TEST( qmc::appendPredicate( aStatement,
                                        aQuerySet,
                                        &sJOIN->plan.resultDesc,
                                        aFilterPredicate )
                  != IDE_SUCCESS );
    }
    else
    {
        sJOIN->filter = NULL;
    }

    // BUG-43077
    // view안에서 참조하는 외부 참조 컬럼들을 Result descriptor에 추가해야 한다.
    if( aViewQuerySet != NULL )
    {
        IDE_TEST( qmc::appendViewPredicate( aStatement,
                                            aViewQuerySet,
                                            &sJOIN->plan.resultDesc,
                                            0 )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }


    // TODO: aFilterPredicate으로 대체해야 한다.
    IDE_TEST( qmc::appendPredicate( aStatement,
                                    aQuerySet,
                                    &sJOIN->plan.resultDesc,
                                    aPredicate )
              != IDE_SUCCESS );

    *aPlan = (qmnPlan *)sJOIN;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmoTwoNonPlan::makeJOIN( qcStatement  * aStatement ,
                         qmsQuerySet  * aQuerySet ,
                         UInt           aGraphType,
                         qmnPlan      * aLeftChild ,
                         qmnPlan      * aRightChild ,
                         qmnPlan      * aPlan )
{
    qmncJOIN          * sJOIN;
    UInt                sDataNodeOffset;

    IDU_FIT_POINT_FATAL( "qmoTwoNonPlan::makeJOIN::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );
    IDE_DASSERT( aLeftChild != NULL );
    IDE_DASSERT( aRightChild != NULL );

    sJOIN = (qmncJOIN *)aPlan;

    sJOIN->plan.left    = aLeftChild;
    sJOIN->plan.right   = aRightChild;

    //----------------------------------
    // Flag 설정
    //----------------------------------

    sJOIN->flag = QMN_PLAN_FLAG_CLEAR;
    sJOIN->plan.flag = QMN_PLAN_FLAG_CLEAR;

    sJOIN->plan.flag       |= (aLeftChild->flag & QMN_PLAN_STORAGE_MASK);
    sJOIN->plan.flag       |= (aRightChild->flag & QMN_PLAN_STORAGE_MASK);

    aPlan->offset = aStatement->myPlan->sTmplate->tmplate.dataSize;
    sDataNodeOffset  = idlOS::align8(aPlan->offset +
                                     ID_SIZEOF(qmndJOIN));

    //data 영역의 크기 계산
    QC_SHARED_TMPLATE(aStatement)->tmplate.dataSize = sDataNodeOffset;

    switch( aGraphType )
    {
        case QMG_INNER_JOIN:
            sJOIN->flag &= ~QMNC_JOIN_TYPE_MASK;
            sJOIN->flag |= QMNC_JOIN_TYPE_INNER;
            break;
        case QMG_SEMI_JOIN:
            sJOIN->flag &= ~QMNC_JOIN_TYPE_MASK;
            sJOIN->flag |= QMNC_JOIN_TYPE_SEMI;
            break;
        case QMG_ANTI_JOIN:
            sJOIN->flag &= ~QMNC_JOIN_TYPE_MASK;
            sJOIN->flag |= QMNC_JOIN_TYPE_ANTI;
            break;
        default:
            IDE_DASSERT( 0 );
    }

    if( sJOIN->filter != NULL )
    {
        IDE_TEST( qmoPred::setPriorNodeID( aStatement ,
                                           aQuerySet ,
                                           sJOIN->filter )
                  != IDE_SUCCESS );

        IDE_TEST( qmoNormalForm::optimizeForm( sJOIN->filter,
                                               &sJOIN->filter )
                  != IDE_SUCCESS );

        IDE_TEST( qmg::changeColumnLocate( aStatement,
                                           aQuerySet,
                                           sJOIN->filter,
                                           ID_USHORT_MAX,
                                           ID_TRUE )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    //----------------------------------
    // dependency 처리 및 subquery의 처리
    //----------------------------------

    IDE_TEST( qmoDependency::setDependency( aStatement ,
                                            aQuerySet ,
                                            & sJOIN->plan ,
                                            QMO_JOIN_DEPENDENCY,
                                            0 ,
                                            NULL ,
                                            1 ,
                                            &sJOIN->filter ,
                                            0 ,
                                            NULL )
              != IDE_SUCCESS );

    IDE_TEST( qmc::filterResultDesc( aStatement,
                                     &sJOIN->plan.resultDesc,
                                     &sJOIN->plan.depInfo,
                                     &(aQuerySet->depInfo) )
              != IDE_SUCCESS );

    /*
     * PROJ-1071 Parallel Query
     * parallel degree
     */
    if ( aLeftChild->mParallelDegree > aRightChild->mParallelDegree )
    {
        sJOIN->plan.mParallelDegree = aLeftChild->mParallelDegree;
    }
    else
    {
        sJOIN->plan.mParallelDegree = aRightChild->mParallelDegree;
    }
    sJOIN->plan.flag &= ~QMN_PLAN_NODE_EXIST_MASK;
    sJOIN->plan.flag |= ((aRightChild->flag | aLeftChild->flag) &
                         QMN_PLAN_NODE_EXIST_MASK);
    
    // PROJ-2551 simple query 최적화
    // index nl join인 경우 fast execute를 수행한다.
    IDE_TEST( checkSimpleJOIN( aStatement, sJOIN ) != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmoTwoNonPlan::initMGJN( qcStatement   * aStatement ,
                         qmsQuerySet   * aQuerySet ,
                         qmoPredicate  * aJoinPredicate ,
                         qmoPredicate  * aFilterPredicate ,
                         qmnPlan       * aParent,
                         qmnPlan      ** aPlan )
{
/***********************************************************************
 *
 * Description : MGJN노드를 생성한다
 *
 * Implementation :
 *     + 초기화 작업
 *         - qmncMGJN의 할당 및 초기화
 *     + 메인 작업
 *         - 하위 노드에 맞는 flag세팅
 *         - MGJN 노드의 구성 ( 입력 Predicate로 부터)
 *         - compareLetfRight 구성 ( 대소 비교를 위한 predicate )
 *         - storedJoinCondition 구성 ( 저장된 결과를 비교 )
 *     + 마무리 작업
 *         - data 영역의 크기 계산
 *         - dependency의 처리
 *         - subquery의 처리
 *
 * TO DO
 *
 ***********************************************************************/

    qmncMGJN          * sMGJN;
    UInt                sDataNodeOffset;

    IDU_FIT_POINT_FATAL( "qmoTwoNonPlan::initMGJN::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );

    //------------------------------------------------------------
    // 초기화 작업
    //------------------------------------------------------------
    // qmncMGJN의 할당및 초기화
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmncMGJN) , (void **)&sMGJN )
              != IDE_SUCCESS );

    QMO_INIT_PLAN_NODE( sMGJN ,
                        QC_SHARED_TMPLATE(aStatement) ,
                        QMN_MGJN ,
                        qmnMGJN ,
                        qmndMGJN,
                        sDataNodeOffset );

    *aPlan = (qmnPlan *)sMGJN;

    IDE_TEST( qmc::pushResultDesc( aStatement,
                                   aQuerySet,
                                   ID_FALSE,
                                   aParent->resultDesc,
                                   &sMGJN->plan.resultDesc )
              != IDE_SUCCESS );

    IDE_TEST( qmc::appendPredicate( aStatement,
                                    aQuerySet,
                                    &sMGJN->plan.resultDesc,
                                    aJoinPredicate )
              != IDE_SUCCESS );

    IDE_TEST( qmc::appendPredicate( aStatement,
                                    aQuerySet,
                                    &sMGJN->plan.resultDesc,
                                    aFilterPredicate )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoTwoNonPlan::makeMGJN( qcStatement  * aStatement ,
                         qmsQuerySet  * aQuerySet ,
                         UInt           aGraphType,
                         qmoPredicate * aJoinPredicate ,
                         qmoPredicate * aFilterPredicate ,
                         qmnPlan      * aLeftChild ,
                         qmnPlan      * aRightChild ,
                         qmnPlan      * aPlan )
{
    qmncMGJN          * sMGJN = (qmncMGJN *)aPlan;
    UInt                sDataNodeOffset;

    UShort              sTupleID;
    UShort              sColumnCount = 0;
    qmcMtrNode        * sMtrNode;

    const mtfModule   * sModule;

    qtcNode           * sCompareNode;
    qtcNode           * sColumnNode;

    qtcNode           * sPredicate; // fix BUG-12282
    qtcNode           * sNode;
    qtcNode           * sOptimizedNode = NULL;

    qtcNode           * sNewNode[2];
    qcNamePosition      sPosition;

    mtcTemplate       * sMtcTemplate;
    qtcNode             sCopiedNode;

    IDU_FIT_POINT_FATAL( "qmoTwoNonPlan::makeMGJN::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );
    IDE_DASSERT( aLeftChild != NULL );
    IDE_DASSERT( aRightChild != NULL );

    //------------------------------------------------------------
    // 초기화 작업
    //------------------------------------------------------------
    sMtcTemplate = & QC_SHARED_TMPLATE(aStatement)->tmplate;

    sMGJN->plan.left    = aLeftChild;
    sMGJN->plan.right   = aRightChild;

    aPlan->offset = aStatement->myPlan->sTmplate->tmplate.dataSize;
    sDataNodeOffset  = idlOS::align8(aPlan->offset +
                                     ID_SIZEOF(qmndMGJN));

    sMGJN->mtrNodeOffset = sDataNodeOffset;

    //----------------------------------
    // Flag 설정
    //----------------------------------

    sMGJN->flag = QMN_PLAN_FLAG_CLEAR;
    sMGJN->plan.flag = QMN_PLAN_FLAG_CLEAR;

    sMGJN->plan.flag       |= (aLeftChild->flag & QMN_PLAN_STORAGE_MASK);
    sMGJN->plan.flag       |= (aRightChild->flag & QMN_PLAN_STORAGE_MASK);

    switch( aGraphType )
    {
        case QMG_INNER_JOIN:
            sMGJN->flag &= ~QMNC_MGJN_TYPE_MASK;
            sMGJN->flag |= QMNC_MGJN_TYPE_INNER;
            break;
        case QMG_SEMI_JOIN:
            sMGJN->flag &= ~QMNC_MGJN_TYPE_MASK;
            sMGJN->flag |= QMNC_MGJN_TYPE_SEMI;
            break;
        case QMG_ANTI_JOIN:
            sMGJN->flag &= ~QMNC_MGJN_TYPE_MASK;
            sMGJN->flag |= QMNC_MGJN_TYPE_ANTI;
            break;
        default:
            IDE_DASSERT( 0 );
    }

    // To Fix PR-8062
    // Child 종류에 따른 Flag 설정
    switch ( aLeftChild->type )
    {
        case QMN_SCAN :
            sMGJN->flag &= ~QMNC_MGJN_LEFT_CHILD_MASK;
            sMGJN->flag |= QMNC_MGJN_LEFT_CHILD_SCAN;
            break;
        case QMN_PCRD :
            sMGJN->flag &= ~QMNC_MGJN_LEFT_CHILD_MASK;
            sMGJN->flag |= QMNC_MGJN_LEFT_CHILD_PCRD;
            break;
        case QMN_SORT :
            sMGJN->flag &= ~QMNC_MGJN_LEFT_CHILD_MASK;
            sMGJN->flag |= QMNC_MGJN_LEFT_CHILD_SORT;
            break;
        case QMN_MGJN :
            sMGJN->flag &= ~QMNC_MGJN_LEFT_CHILD_MASK;
            sMGJN->flag |= QMNC_MGJN_LEFT_CHILD_MGJN;
            break;
        default:
            IDE_DASSERT(0);
            break;
    }

    switch ( aRightChild->type )
    {
        case QMN_SCAN :
            sMGJN->flag &= ~QMNC_MGJN_RIGHT_CHILD_MASK;
            sMGJN->flag |= QMNC_MGJN_RIGHT_CHILD_SCAN;
            break;
        case QMN_PCRD :
            sMGJN->flag &= ~QMNC_MGJN_RIGHT_CHILD_MASK;
            sMGJN->flag |= QMNC_MGJN_RIGHT_CHILD_PCRD;
            break;
        case QMN_SORT :
            sMGJN->flag &= ~QMNC_MGJN_RIGHT_CHILD_MASK;
            sMGJN->flag |= QMNC_MGJN_RIGHT_CHILD_SORT;
            break;
        default:
            IDE_DASSERT(0);
            break;
    }

    //------------------------------------------------------------
    // 메인 작업
    //------------------------------------------------------------

    //----------------------------------
    // Flag 설정
    //----------------------------------

    IDE_TEST( qtc::nextTable( &sTupleID , aStatement, NULL, ID_TRUE, MTC_COLUMN_NOTNULL_TRUE ) // PR-13597
              != IDE_SUCCESS );

    sMtcTemplate->rows[sTupleID].lflag      &= ~MTC_TUPLE_STORAGE_MASK;
    sMtcTemplate->rows[sTupleID].lflag      |= MTC_TUPLE_STORAGE_MEMORY;

    //----------------------------------
    // Join Predicate의 구성
    //----------------------------------

    IDE_TEST( qmoPred::linkPredicate( aStatement ,
                                      aJoinPredicate ,
                                      & sNode )
              != IDE_SUCCESS );
    IDE_TEST( qmoPred::setPriorNodeID( aStatement ,
                                       aQuerySet ,
                                       sNode ) != IDE_SUCCESS );

    // BUG-17921
    IDE_TEST( qmoNormalForm::optimizeForm( sNode,
                                           & sOptimizedNode )
              != IDE_SUCCESS );

    sMGJN->mergeJoinPred = sOptimizedNode;

    // To Fix PR-8062
    if ( aFilterPredicate != NULL )
    {
        IDE_TEST( qmoPred::linkFilterPredicate( aStatement ,
                                                aFilterPredicate ,
                                                & sNode )
                  != IDE_SUCCESS );
        IDE_TEST( qmoPred::setPriorNodeID( aStatement ,
                                           aQuerySet ,
                                           sNode )
                  != IDE_SUCCESS );

        // BUG-17921
        IDE_TEST( qmoNormalForm::optimizeForm( sNode,
                                               & sOptimizedNode )
                  != IDE_SUCCESS );

        sMGJN->joinFilter = sOptimizedNode;
    }
    else
    {
        sMGJN->joinFilter = NULL;
    }

    // To Fix PR-8062
    // Store Join Predicate, Compare Join Predicate 생성
    // 절차를 간소화함.

    //----------------------------------
    // Compare Node 및 Column Node 추출
    //----------------------------------

    sCompareNode = sMGJN->mergeJoinPred;
    while ( 1 )
    {
        if( ( sCompareNode->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
            == MTC_NODE_LOGICAL_CONDITION_TRUE )
        {
            sCompareNode = (qtcNode *)(sCompareNode->node.arguments);
        }
        else
        {
            break;
        }
    }

    if ( sCompareNode->indexArgument == 0 )
    {
        sColumnNode = (qtcNode*) sCompareNode->node.arguments;
    }
    else
    {
        sColumnNode = (qtcNode*) sCompareNode->node.arguments->next;
    }

    sCopiedNode = *sColumnNode;

    //----------------------------------
    // 저장 Column 구성
    //----------------------------------
    IDE_TEST( qmg::makeColumnMtrNode( aStatement,
                                      aQuerySet,
                                      &sCopiedNode,
                                      ID_TRUE,
                                      sTupleID,
                                      0,
                                      & sColumnCount,
                                      & sMGJN->myNode )
              != IDE_SUCCESS );

    // To Fix PR-8062
    // 저장 Column을 유형에 관계 없이 calculate 후 복사하도록 설정한다.
    for ( sMtrNode = sMGJN->myNode;
          sMtrNode != NULL;
          sMtrNode = sMtrNode->next )
    {
        sMtrNode->flag &= ~QMC_MTR_TYPE_MASK;
        sMtrNode->flag |= QMC_MTR_TYPE_CALCULATE_AND_COPY_VALUE;
    }

    //----------------------------------
    // Stored Join Condition 생성
    //----------------------------------

    // fix BUG-23279
    IDE_TEST( qmg::changeColumnLocate( aStatement,
                                       aQuerySet,
                                       sMGJN->mergeJoinPred,
                                       sTupleID,
                                       ID_TRUE )
              != IDE_SUCCESS );

    // Join Predicate을 복사
    IDE_TEST( qtc::cloneQTCNodeTree( QC_QMP_MEM(aStatement),
                                     sMGJN->mergeJoinPred,
                                     &sMGJN->storedMergeJoinPred,
                                     ID_TRUE,
                                     ID_FALSE,
                                     ID_FALSE,
                                     ID_FALSE )
              != IDE_SUCCESS );

    // CompareNode 추출
    sCompareNode = sMGJN->storedMergeJoinPred;
    while ( 1 )
    {
        if( ( sCompareNode->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
            == MTC_NODE_LOGICAL_CONDITION_TRUE )
        {
            sCompareNode = (qtcNode *)(sCompareNode->node.arguments);
        }
        else
        {
            break;
        }
    }

    // Column 노드를 저장 Column으로 대체
    if ( sCompareNode->indexArgument == 0 )
    {
        sColumnNode = (qtcNode*) sCompareNode->node.arguments;
        sCompareNode->node.arguments = (mtcNode*) sMGJN->myNode->dstNode;
        sCompareNode->node.arguments->next = sColumnNode->node.next;
    }
    else
    {
        sCompareNode->node.arguments->next =
            (mtcNode*) sMGJN->myNode->dstNode;
    }

    //----------------------------------
    // 등호 연산자의 경우의 대소 비교 함수 추출
    //----------------------------------

    if ( sCompareNode->node.module == & mtfEqual )
    {
        // Join Predicate을 복사
        IDE_TEST( qtc::cloneQTCNodeTree( QC_QMP_MEM(aStatement),
                                         sMGJN->mergeJoinPred,
                                         &sMGJN->compareLeftRight,
                                         ID_TRUE,
                                         ID_FALSE,
                                         ID_FALSE,
                                         ID_FALSE )
                  != IDE_SUCCESS );

        // CompareNode 추출
        sCompareNode = sMGJN->compareLeftRight;
        while ( 1 )
        {
            if( ( sCompareNode->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
                == MTC_NODE_LOGICAL_CONDITION_TRUE )
            {
                sCompareNode = (qtcNode *)(sCompareNode->node.arguments);
            }
            else
            {
                break;
            }
        }

        SET_EMPTY_POSITION( sPosition );

        if ( sCompareNode->indexArgument == 0 )
        {
            // T1.i1 = T2.i1
            //      MGJN
            //     |    |
            //    T2    T1 인 경우임
            // T1.i1 <= T2.i1 을 생성함.

            // To Fix PR-8062
            // 실제 노드를 생성하여 연결 구조를 변경시킴.
            IDE_TEST( qtc::makeNode( aStatement,
                                     sNewNode,
                                     & sPosition,
                                     (const UChar*) "<=",
                                     2 )
                      != IDE_SUCCESS );

        }
        else
        {
            // T1.i1 = T2.i1
            //      MGJN
            //     |    |
            //    T1    T2 인 경우임
            // T1.i1 >= T2.i1 을 생성함.

            // 실제 노드를 생성하여 연결 구조를 변경시킴.
            IDE_TEST( qtc::makeNode( aStatement,
                                     sNewNode,
                                     & sPosition,
                                     (const UChar*) ">=",
                                     2 )
                      != IDE_SUCCESS );

        }

        sNewNode[0]->node.arguments = sCompareNode->node.arguments;

        IDE_TEST( qtc::estimateNodeWithoutArgument ( aStatement,
                                                     sNewNode[0] )
                  != IDE_SUCCESS );

        idlOS::memcpy( sCompareNode, sNewNode[0], ID_SIZEOF(qtcNode) );
    }
    else
    {
        // 대소 비교 함수를 만들 필요가 없음.
        sMGJN->compareLeftRight = NULL;
    }

    //----------------------------------
    // Tuple column의 할당
    //----------------------------------
    sColumnCount = 0;
    for (sMtrNode = sMGJN->myNode ;
         sMtrNode != NULL ;
         sMtrNode = sMtrNode->next)
    {
        sModule      = sMtrNode->srcNode->node.module;
        sColumnCount += sModule->lflag & MTC_NODE_COLUMN_COUNT_MASK;
    }

    IDE_TEST( qtc::allocIntermediateTuple( aStatement,
                                           & QC_SHARED_TMPLATE(aStatement)->tmplate,
                                           sTupleID ,
                                           sColumnCount ) != IDE_SUCCESS);

    sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_PLAN_MASK;
    sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_PLAN_TRUE;

    sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_STORAGE_MASK;
    sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_STORAGE_MEMORY;

    sMtcTemplate->rows[sTupleID].lflag &= ~MTC_TUPLE_PLAN_MTR_MASK;
    sMtcTemplate->rows[sTupleID].lflag |= MTC_TUPLE_PLAN_MTR_FALSE;

    //----------------------------------
    // mtcColumn , mtcExecute 정보의 구축
    //----------------------------------
    IDE_TEST( qmg::copyMtcColumnExecute( aStatement ,
                                         sMGJN->myNode )
              != IDE_SUCCESS);

    //----------------------------------
    // PROJ-2179 calculate가 필요한 node들의 결과 위치를 설정
    //----------------------------------

    IDE_TEST( qmg::setCalcLocate( aStatement,
                                  sMGJN->myNode )
              != IDE_SUCCESS );

    //------------------------------------------------------------
    // 마무리 작업
    //------------------------------------------------------------
    //data 영역의 크기 계산
    for (sMtrNode = sMGJN->myNode , sColumnCount = 0 ;
         sMtrNode != NULL;
         sMtrNode = sMtrNode->next , sColumnCount++ ) ;

    QC_SHARED_TMPLATE(aStatement)->tmplate.dataSize = sDataNodeOffset +
        sColumnCount * idlOS::align8( ID_SIZEOF(qmdMtrNode) );

    //----------------------------------
    // dependency 처리 및 subquery의 처리
    //----------------------------------

    // fix BUG-12282
    sPredicate = sMGJN->joinFilter;

    //----------------------------------
    // PROJ-1437
    // dependency 설정전에 predicate들의 위치정보 변경. 
    //----------------------------------

    IDE_TEST( qmg::changeColumnLocate( aStatement,
                                       aQuerySet,
                                       sPredicate,
                                       ID_USHORT_MAX,
                                       ID_TRUE )
              != IDE_SUCCESS );

    IDE_TEST( qmoDependency::setDependency( aStatement ,
                                            aQuerySet ,
                                            & sMGJN->plan ,
                                            QMO_MGJN_DEPENDENCY,
                                            0 ,
                                            NULL ,
                                            1 , // fix BUG-12282
                                            &sPredicate ,
                                            0 ,
                                            NULL )
              != IDE_SUCCESS );

    IDE_TEST( qmc::filterResultDesc( aStatement,
                                     &sMGJN->plan.resultDesc,
                                     &sMGJN->plan.depInfo,
                                     &(aQuerySet->depInfo) )
              != IDE_SUCCESS );

    /*
     * PROJ-1071 Parallel Query
     * parallel degree
     */
    if ( aLeftChild->mParallelDegree > aRightChild->mParallelDegree )
    {
        sMGJN->plan.mParallelDegree = aLeftChild->mParallelDegree;
    }
    else
    {
        sMGJN->plan.mParallelDegree = aRightChild->mParallelDegree;
    }
    sMGJN->plan.flag &= ~QMN_PLAN_NODE_EXIST_MASK;
    sMGJN->plan.flag |= ((aRightChild->flag | aLeftChild->flag) &
                         QMN_PLAN_NODE_EXIST_MASK);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoTwoNonPlan::initLOJN( qcStatement   * aStatement ,
                         qmsQuerySet   * aQuerySet ,
                         qmsQuerySet   * aViewQuerySet,
                         qtcNode       * aJoinPredicate,
                         qtcNode       * aFilter,
                         qmoPredicate  * aPredicate,
                         qmnPlan       * aParent ,
                         qmnPlan      ** aPlan )
{
/***********************************************************************
 *
 * Description : LOJN 노드를 생성한다
 *
 * Implementation :
 *     + 초기화 작업
 *         - qmncLOJN의 할당 및 초기화
 *     + 메인 작업
 *         - aFilterPredicate를 Filter로 구성한다
 *     + 마무리 작업
 *         - data 영역의 크기 계산
 *         - dependency의 처리
 *         - subquery의 처리
 *
 * TO DO
 *
 ***********************************************************************/

    qmncLOJN          * sLOJN;
    UInt                sDataNodeOffset;

    IDU_FIT_POINT_FATAL( "qmoTwoNonPlan::initLOJN::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );

    //------------------------------------------------------------
    // 초기화 작업
    //------------------------------------------------------------

    // qmncLOJN의 할당및 초기화
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmncLOJN) , (void **)&sLOJN )
              != IDE_SUCCESS );

    QMO_INIT_PLAN_NODE( sLOJN ,
                        QC_SHARED_TMPLATE(aStatement) ,
                        QMN_LOJN ,
                        qmnLOJN ,
                        qmndLOJN,
                        sDataNodeOffset );

    IDE_TEST( qmc::pushResultDesc( aStatement,
                                   aQuerySet,
                                   ID_FALSE,
                                   aParent->resultDesc,
                                   &sLOJN->plan.resultDesc )
              != IDE_SUCCESS );

    // BUG-38513 join 조건절도 result desc에 추가해주어야함
    if( aJoinPredicate != NULL )
    {
        IDE_TEST( qmc::appendPredicate( aStatement,
                                        aQuerySet,
                                        &sLOJN->plan.resultDesc,
                                        aJoinPredicate )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    if( aFilter != NULL )
    {
        IDE_TEST( qmc::appendPredicate( aStatement,
                                        aQuerySet,
                                        &sLOJN->plan.resultDesc,
                                        aFilter )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    // BUG-47414
    // view안에서 참조하는 외부 참조 컬럼들을 Result descriptor에 추가해야 한다.
    if( aViewQuerySet != NULL )
    {
        IDE_TEST( qmc::appendViewPredicate( aStatement,
                                            aViewQuerySet,
                                            &sLOJN->plan.resultDesc,
                                            0 )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST( qmc::appendPredicate( aStatement,
                                    aQuerySet,
                                    &sLOJN->plan.resultDesc,
                                    aPredicate )
              != IDE_SUCCESS );

    *aPlan = (qmnPlan *)sLOJN;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoTwoNonPlan::makeLOJN( qcStatement  * aStatement ,
                         qmsQuerySet  * aQuerySet ,
                         qtcNode      * aFilter ,
                         qmnPlan      * aLeftChild ,
                         qmnPlan      * aRightChild ,
                         qmnPlan      * aPlan )
{
/***********************************************************************
 *
 * Description : LOJN 노드를 생성한다
 *
 * Implementation :
 *     + 초기화 작업
 *         - qmncLOJN의 할당 및 초기화
 *     + 메인 작업
 *         - aFilterPredicate를 Filter로 구성한다
 *     + 마무리 작업
 *         - data 영역의 크기 계산
 *         - dependency의 처리
 *         - subquery의 처리
 *
 * TO DO
 *
 ***********************************************************************/

    qmncLOJN          * sLOJN = (qmncLOJN *)aPlan;
    UInt                sDataNodeOffset;
    qtcNode           * sPredicate[2];
    qtcNode           * sOptimizedNode;

    IDU_FIT_POINT_FATAL( "qmoTwoNonPlan::makeLOJN::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aLeftChild != NULL );
    IDE_DASSERT( aRightChild != NULL );

    //------------------------------------------------------------
    // 초기화 작업
    //------------------------------------------------------------

    sLOJN->plan.left    = aLeftChild;
    sLOJN->plan.right   = aRightChild;

    //----------------------------------
    // Flag 설정
    //----------------------------------

    sLOJN->flag      = QMN_PLAN_FLAG_CLEAR;
    sLOJN->plan.flag = QMN_PLAN_FLAG_CLEAR;

    sLOJN->plan.flag       |= (aLeftChild->flag & QMN_PLAN_STORAGE_MASK);
    sLOJN->plan.flag       |= (aRightChild->flag & QMN_PLAN_STORAGE_MASK);

    //------------------------------------------------------------
    // 메인 작업
    //------------------------------------------------------------

    if ( aFilter != NULL )
    {
        // BUG-17921
        IDE_TEST( qmoNormalForm::optimizeForm( aFilter,
                                               & sOptimizedNode )
                  != IDE_SUCCESS );

        sLOJN->filter = sOptimizedNode;
    }
    else
    {
        sLOJN->filter = NULL;
    }

    //------------------------------------------------------------
    // 마무리 작업
    //------------------------------------------------------------

    aPlan->offset = aStatement->myPlan->sTmplate->tmplate.dataSize;
    sDataNodeOffset  = idlOS::align8(aPlan->offset +
                                     ID_SIZEOF(qmndLOJN));

    //data 영역의 크기 계산
    QC_SHARED_TMPLATE(aStatement)->tmplate.dataSize = sDataNodeOffset;

    //----------------------------------
    // dependency 처리 및 subquery의 처리
    //----------------------------------

    sPredicate[0] =  sLOJN->filter;

    //----------------------------------
    // PROJ-1437
    // dependency 설정전에 predicate들의 위치정보 변경. 
    //----------------------------------

    IDE_TEST( qmg::changeColumnLocate( aStatement,
                                       aQuerySet,
                                       sPredicate[0],
                                       ID_USHORT_MAX,
                                       ID_TRUE )
              != IDE_SUCCESS );    

    IDE_TEST( qmoDependency::setDependency( aStatement ,
                                            aQuerySet ,
                                            &sLOJN->plan ,
                                            QMO_LOJN_DEPENDENCY,
                                            0 ,
                                            NULL ,
                                            1 ,
                                            sPredicate,
                                            0 ,
                                            NULL )
              != IDE_SUCCESS );

    IDE_TEST( qmc::filterResultDesc( aStatement,
                                     &sLOJN->plan.resultDesc,
                                     &sLOJN->plan.depInfo,
                                     &(aQuerySet->depInfo) )
              != IDE_SUCCESS );

    /*
     * PROJ-1071 Parallel Query
     * parallel degree
     */
    if ( aLeftChild->mParallelDegree > aRightChild->mParallelDegree )
    {
        sLOJN->plan.mParallelDegree = aLeftChild->mParallelDegree;
    }
    else
    {
        sLOJN->plan.mParallelDegree = aRightChild->mParallelDegree;
    }
    sLOJN->plan.flag &= ~QMN_PLAN_NODE_EXIST_MASK;
    sLOJN->plan.flag |= ((aRightChild->flag | aLeftChild->flag) &
                         QMN_PLAN_NODE_EXIST_MASK);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoTwoNonPlan::initFOJN( qcStatement   * aStatement ,
                         qmsQuerySet   * aQuerySet ,
                         qtcNode       * aJoinPredicate,
                         qtcNode       * aFilter ,
                         qmoPredicate  * aPredicate,
                         qmnPlan       * aParent,
                         qmnPlan      ** aPlan )
{
/***********************************************************************
 *
 * Description : FOJN 노드를 생성한다
 *
 * Implementation :
 *     + 초기화 작업
 *         - qmncFOJN의 할당 및 초기화
 *     + 메인 작업
 *         - aFilterPredicate를 Filter로 구성한다
 *     + 마무리 작업
 *         - data 영역의 크기 계산
 *         - dependency의 처리
 *         - subquery의 처리
 *
 * TO DO
 *
 ***********************************************************************/

    qmncFOJN          * sFOJN;
    UInt                sDataNodeOffset;

    IDU_FIT_POINT_FATAL( "qmoTwoNonPlan::initFOJN::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_FT_ASSERT( aStatement != NULL );

    //------------------------------------------------------------
    // 초기화 작업
    //------------------------------------------------------------

    // qmncFOJN의 할당및 초기화
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmncFOJN) , (void **)&sFOJN )
              != IDE_SUCCESS );

    QMO_INIT_PLAN_NODE( sFOJN ,
                        QC_SHARED_TMPLATE(aStatement) ,
                        QMN_FOJN ,
                        qmnFOJN ,
                        qmndFOJN,
                        sDataNodeOffset );

    *aPlan = (qmnPlan *)sFOJN;

    IDE_TEST( qmc::pushResultDesc( aStatement,
                                   aQuerySet,
                                   ID_FALSE,
                                   aParent->resultDesc,
                                   &sFOJN->plan.resultDesc )
              != IDE_SUCCESS );

    // BUG-38513 join 조건절도 result desc에 추가해주어야함 
    if( aJoinPredicate != NULL )
    {
        IDE_TEST( qmc::appendPredicate( aStatement,
                                        aQuerySet,
                                        &sFOJN->plan.resultDesc,
                                        aJoinPredicate )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    if( aFilter != NULL )
    {
        IDE_TEST( qmc::appendPredicate( aStatement,
                                        aQuerySet,
                                        &sFOJN->plan.resultDesc,
                                        aFilter )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST( qmc::appendPredicate( aStatement,
                                    aQuerySet,
                                    &sFOJN->plan.resultDesc,
                                    aPredicate )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoTwoNonPlan::makeFOJN( qcStatement  * aStatement ,
                         qmsQuerySet  * aQuerySet ,
                         qtcNode      * aFilter ,
                         qmnPlan      * aLeftChild ,
                         qmnPlan      * aRightChild ,
                         qmnPlan      * aPlan )
{
/***********************************************************************
 *
 * Description : FOJN 노드를 생성한다
 *
 * Implementation :
 *     + 초기화 작업
 *         - qmncFOJN의 할당 및 초기화
 *     + 메인 작업
 *         - aFilterPredicate를 Filter로 구성한다
 *     + 마무리 작업
 *         - data 영역의 크기 계산
 *         - dependency의 처리
 *         - subquery의 처리
 *
 * TO DO
 *
 ***********************************************************************/

    qmncFOJN          * sFOJN = (qmncFOJN *)aPlan;
    UInt                sDataNodeOffset;
    qtcNode           * sPredicate[2];
    qtcNode           * sOptimizedNode;

    IDU_FIT_POINT_FATAL( "qmoTwoNonPlan::makeFOJN::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_FT_ASSERT( aStatement != NULL );
    IDE_FT_ASSERT( aLeftChild != NULL );
    IDE_FT_ASSERT( aRightChild != NULL );

    //------------------------------------------------------------
    // 초기화 작업
    //------------------------------------------------------------

    sFOJN->plan.left    = aLeftChild;
    sFOJN->plan.right   = aRightChild;

    aPlan->offset = aStatement->myPlan->sTmplate->tmplate.dataSize;
    sDataNodeOffset  = idlOS::align8(aPlan->offset +
                                     ID_SIZEOF(qmndFOJN));

    //----------------------------------
    // Flag 설정
    //----------------------------------

    sFOJN->flag      = QMN_PLAN_FLAG_CLEAR;
    sFOJN->plan.flag = QMN_PLAN_FLAG_CLEAR;

    sFOJN->plan.flag       |= (aLeftChild->flag & QMN_PLAN_STORAGE_MASK);
    sFOJN->plan.flag       |= (aRightChild->flag & QMN_PLAN_STORAGE_MASK);

    //------------------------------------------------------------
    // 메인 작업
    //------------------------------------------------------------

    //flag의 설정
    if (aRightChild->type == QMN_HASH)
    {
        sFOJN->flag &= ~QMNC_FOJN_RIGHT_CHILD_MASK;
        sFOJN->flag |= QMNC_FOJN_RIGHT_CHILD_HASH;
    }
    else if (aRightChild->type == QMN_SORT)
    {
        sFOJN->flag &= ~QMNC_FOJN_RIGHT_CHILD_MASK;
        sFOJN->flag |= QMNC_FOJN_RIGHT_CHILD_SORT;
    }
    else
    {
        IDE_DASSERT(0);
    }

    if ( aFilter != NULL )
    {
        // BUG-17921
        IDE_TEST( qmoNormalForm::optimizeForm( aFilter,
                                               & sOptimizedNode )
                  != IDE_SUCCESS );

        sFOJN->filter = sOptimizedNode;
    }
    else
    {
        sFOJN->filter = NULL;
    }

    //------------------------------------------------------------
    // 마무리 작업
    //------------------------------------------------------------

    //data 영역의 크기 계산
    QC_SHARED_TMPLATE(aStatement)->tmplate.dataSize = sDataNodeOffset;

    //----------------------------------
    // dependency 처리 및 subquery의 처리
    //----------------------------------

    sPredicate[0] =  sFOJN->filter;

    //----------------------------------
    // PROJ-1437
    // dependency 설정전에 predicate들의 위치정보 변경. 
    //----------------------------------

    IDE_TEST( qmg::changeColumnLocate( aStatement,
                                       aQuerySet,
                                       sPredicate[0],
                                       ID_USHORT_MAX,
                                       ID_TRUE )
              != IDE_SUCCESS );    

    IDE_TEST( qmoDependency::setDependency( aStatement ,
                                            aQuerySet ,
                                            &sFOJN->plan ,
                                            QMO_FOJN_DEPENDENCY,
                                            0 ,
                                            NULL ,
                                            1 ,
                                            sPredicate,
                                            0 ,
                                            NULL )
              != IDE_SUCCESS );

    IDE_TEST( qmc::filterResultDesc( aStatement,
                                     &sFOJN->plan.resultDesc,
                                     &sFOJN->plan.depInfo,
                                     &(aQuerySet->depInfo) )
              != IDE_SUCCESS );

    /*
     * PROJ-1071 Parallel Query
     * parallel degree
     */
    if ( aLeftChild->mParallelDegree > aRightChild->mParallelDegree )
    {
        sFOJN->plan.mParallelDegree = aLeftChild->mParallelDegree;
    }
    else
    {
        sFOJN->plan.mParallelDegree = aRightChild->mParallelDegree;
    }
    sFOJN->plan.flag &= ~QMN_PLAN_NODE_EXIST_MASK;
    sFOJN->plan.flag |= ((aRightChild->flag | aLeftChild->flag) &
                         QMN_PLAN_NODE_EXIST_MASK);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoTwoNonPlan::initAOJN( qcStatement   * aStatement ,
                         qmsQuerySet   * aQuerySet ,
                         qtcNode       * aFilter ,
                         qmoPredicate  * aPredicate,
                         qmnPlan       * aParent,
                         qmnPlan      ** aPlan )
{
/***********************************************************************
 *
 * Description : AOJN 노드를 생성한다
 *
 * Implementation :
 *     + 초기화 작업
 *         - qmncAOJN의 할당 및 초기화
 *     + 메인 작업
 *         - aFilterPredicate를 Filter로 구성한다
 *     + 마무리 작업
 *         - data 영역의 크기 계산
 *         - dependency의 처리
 *         - subquery의 처리
 *
 * TO DO
 *
 ***********************************************************************/

    qmncAOJN          * sAOJN;
    UInt                sDataNodeOffset;

    IDU_FIT_POINT_FATAL( "qmoTwoNonPlan::initAOJN::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );

    //------------------------------------------------------------
    // 초기화 작업
    //------------------------------------------------------------

    // qmncAOJN의 할당및 초기화
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmncAOJN) , (void **)&sAOJN )
              != IDE_SUCCESS );

    QMO_INIT_PLAN_NODE( sAOJN ,
                        QC_SHARED_TMPLATE(aStatement) ,
                        QMN_AOJN ,
                        qmnAOJN ,
                        qmndAOJN,
                        sDataNodeOffset );

    *aPlan = (qmnPlan *)sAOJN;

    IDE_TEST( qmc::pushResultDesc( aStatement,
                                   aQuerySet,
                                   ID_FALSE,
                                   aParent->resultDesc,
                                   &sAOJN->plan.resultDesc )
              != IDE_SUCCESS );

    if( aFilter != NULL )
    {
        IDE_TEST( qmc::appendPredicate( aStatement,
                                        aQuerySet,
                                        &sAOJN->plan.resultDesc,
                                        aFilter )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST( qmc::appendPredicate( aStatement,
                                    aQuerySet,
                                    &sAOJN->plan.resultDesc,
                                    aPredicate )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoTwoNonPlan::makeAOJN( qcStatement * aStatement ,
                         qmsQuerySet * aQuerySet ,
                         qtcNode     * aFilter ,
                         qmnPlan     * aLeftChild ,
                         qmnPlan     * aRightChild ,
                         qmnPlan     * aPlan )
{
/***********************************************************************
 *
 * Description : AOJN 노드를 생성한다
 *
 * Implementation :
 *     + 초기화 작업
 *         - qmncAOJN의 할당 및 초기화
 *     + 메인 작업
 *         - aFilterPredicate를 Filter로 구성한다
 *     + 마무리 작업
 *         - data 영역의 크기 계산
 *         - dependency의 처리
 *         - subquery의 처리
 *
 * TO DO
 *
 ***********************************************************************/

    qmncAOJN          * sAOJN = (qmncAOJN *)aPlan;
    UInt                sDataNodeOffset;
    qtcNode           * sPredicate[2];
    qtcNode           * sOptimizedNode;

    IDU_FIT_POINT_FATAL( "qmoTwoNonPlan::makeAOJN::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aLeftChild != NULL );
    IDE_DASSERT( aRightChild != NULL );

    //------------------------------------------------------------
    // 초기화 작업
    //------------------------------------------------------------

    sAOJN->plan.left    = aLeftChild;
    sAOJN->plan.right   = aRightChild;

    aPlan->offset = aStatement->myPlan->sTmplate->tmplate.dataSize;
    sDataNodeOffset  = idlOS::align8(aPlan->offset +
                                     ID_SIZEOF(qmndAOJN));

    //----------------------------------
    // Flag 설정
    //----------------------------------

    sAOJN->flag      = QMN_PLAN_FLAG_CLEAR;
    sAOJN->plan.flag = QMN_PLAN_FLAG_CLEAR;

    sAOJN->plan.flag       |= (aLeftChild->flag & QMN_PLAN_STORAGE_MASK);
    sAOJN->plan.flag       |= (aRightChild->flag & QMN_PLAN_STORAGE_MASK);

    //------------------------------------------------------------
    // 메인 작업
    //------------------------------------------------------------

    if ( aFilter != NULL )
    {
        // BUG-17921
        IDE_TEST( qmoNormalForm::optimizeForm( aFilter,
                                               & sOptimizedNode )
                  != IDE_SUCCESS );

        sAOJN->filter = sOptimizedNode;
    }
    else
    {
        sAOJN->filter = NULL;
    }

    //------------------------------------------------------------
    // 마무리 작업
    //------------------------------------------------------------

    //data 영역의 크기 계산
    QC_SHARED_TMPLATE(aStatement)->tmplate.dataSize = sDataNodeOffset;

    //----------------------------------
    // dependency 처리 및 subquery의 처리
    //----------------------------------

    sPredicate[0] =  sAOJN->filter;

    //----------------------------------
    // PROJ-1437
    // dependency 설정전에 predicate들의 위치정보 변경. 
    //----------------------------------

    IDE_TEST( qmg::changeColumnLocate( aStatement,
                                       aQuerySet,
                                       sPredicate[0],
                                       ID_USHORT_MAX,
                                       ID_TRUE )
              != IDE_SUCCESS );
    
    IDE_TEST( qmoDependency::setDependency( aStatement ,
                                            aQuerySet ,
                                            &sAOJN->plan ,
                                            QMO_AOJN_DEPENDENCY,
                                            0 ,
                                            NULL ,
                                            1 ,
                                            sPredicate,
                                            0 ,
                                            NULL )
              != IDE_SUCCESS );

    IDE_TEST( qmc::filterResultDesc( aStatement,
                                     &sAOJN->plan.resultDesc,
                                     &sAOJN->plan.depInfo,
                                     &(aQuerySet->depInfo) )
              != IDE_SUCCESS );

    /*
     * PROJ-1071 Parallel Query
     * parallel degree
     */
    if ( aLeftChild->mParallelDegree > aRightChild->mParallelDegree )
    {
        sAOJN->plan.mParallelDegree = aLeftChild->mParallelDegree;
    }
    else
    {
        sAOJN->plan.mParallelDegree = aRightChild->mParallelDegree;
    }
    sAOJN->plan.flag &= ~QMN_PLAN_NODE_EXIST_MASK;
    sAOJN->plan.flag |= ((aRightChild->flag | aLeftChild->flag) &
                         QMN_PLAN_NODE_EXIST_MASK);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoTwoNonPlan::initCONC( qcStatement  * aStatement ,
                         qmsQuerySet  * aQuerySet ,
                         qmnPlan      * aParent,
                         qcDepInfo    * aDepInfo,
                         qmnPlan     ** aPlan )
{
/***********************************************************************
 *
 * Description : CONC 노드를 생성한다
 *
 * Implementation :
 *     + 초기화 작업
 *         - qmncCONC의 할당 및 초기화
 *     + 메인 작업
 *     + 마무리 작업
 *         - data 영역의 크기 계산
 *         - dependency의 처리
 *         - subquery의 처리
 *
 * TO DO
 *
 ***********************************************************************/

    qmncCONC    * sCONC;
    UInt          sDataNodeOffset;
    qmcAttrDesc * sItrAttr;

    IDU_FIT_POINT_FATAL( "qmoTwoNonPlan::initCONC::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );

    //------------------------------------------------------------
    // 초기화 작업
    //------------------------------------------------------------

    // qmncCONC의 할당및 초기화
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmncCONC) , (void **)&sCONC )
              != IDE_SUCCESS );

    QMO_INIT_PLAN_NODE( sCONC ,
                        QC_SHARED_TMPLATE(aStatement) ,
                        QMN_CONC ,
                        qmnCONC ,
                        qmndCONC,
                        sDataNodeOffset );

    *aPlan = (qmnPlan *)sCONC;

    IDE_TEST( qmc::pushResultDesc( aStatement,
                                   aQuerySet,
                                   ID_FALSE,
                                   aParent->resultDesc,
                                   &sCONC->plan.resultDesc )
              != IDE_SUCCESS );

    // BUG-38285
    // CONCATENATION 이하의 컬럼에 한해
    // 동일한 ID의 TABLE이 여러개 존재하므로 중간에 materialize되는 곳으로 위치를 변경해서는 안된다.

    for( sItrAttr = sCONC->plan.resultDesc;
         sItrAttr != NULL;
         sItrAttr = sItrAttr->next )
    {
        if ( qtc::dependencyContains( aDepInfo, &sItrAttr->expr->depInfo )
             == ID_TRUE )
        {
            sItrAttr->expr->node.lflag &= ~MTC_NODE_COLUMN_LOCATE_CHANGE_MASK;
            sItrAttr->expr->node.lflag |= MTC_NODE_COLUMN_LOCATE_CHANGE_FALSE;
        }
        else
        {
            // Nothing to do.
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoTwoNonPlan::makeCONC( qcStatement  * aStatement ,
                         qmsQuerySet  * aQuerySet ,
                         qmnPlan      * aLeftChild ,
                         qmnPlan      * aRightChild ,
                         qmnPlan      * aPlan )
{
/***********************************************************************
 *
 * Description : CONC 노드를 생성한다
 *
 * Implementation :
 *     + 초기화 작업
 *         - qmncCONC의 할당 및 초기화
 *     + 메인 작업
 *     + 마무리 작업
 *         - data 영역의 크기 계산
 *         - dependency의 처리
 *         - subquery의 처리
 *
 * TO DO
 *
 ***********************************************************************/

    qmncCONC          *     sCONC = (qmncCONC *)aPlan;
    UInt                sDataNodeOffset;

    IDU_FIT_POINT_FATAL( "qmoTwoNonPlan::makeCONC::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aLeftChild != NULL );
    IDE_DASSERT( aRightChild != NULL );

    //------------------------------------------------------------
    // 초기화 작업
    //------------------------------------------------------------

    sCONC->plan.left    = aLeftChild;
    sCONC->plan.right   = aRightChild;

    aPlan->offset = aStatement->myPlan->sTmplate->tmplate.dataSize;
    sDataNodeOffset  = idlOS::align8(aPlan->offset +
                                     ID_SIZEOF(qmndCONC));

    //----------------------------------
    // Flag 설정
    //----------------------------------

    sCONC->flag      = QMN_PLAN_FLAG_CLEAR;
    sCONC->plan.flag = QMN_PLAN_FLAG_CLEAR;

    sCONC->plan.flag       |= (aLeftChild->flag & QMN_PLAN_STORAGE_MASK);
    sCONC->plan.flag       |= (aRightChild->flag & QMN_PLAN_STORAGE_MASK);

    //------------------------------------------------------------
    // 마무리 작업
    //------------------------------------------------------------

    //data 영역의 크기 계산
    QC_SHARED_TMPLATE(aStatement)->tmplate.dataSize = sDataNodeOffset;

    //----------------------------------
    // dependency 처리 및 subquery의 처리
    //----------------------------------

    IDE_TEST( qmoDependency::setDependency( aStatement ,
                                            aQuerySet ,
                                            & sCONC->plan ,
                                            QMO_CONC_DEPENDENCY,
                                            0 ,
                                            NULL ,
                                            0 ,
                                            NULL ,
                                            0 ,
                                            NULL )
              != IDE_SUCCESS );

    IDE_TEST( qmc::filterResultDesc( aStatement,
                                     &sCONC->plan.resultDesc,
                                     &sCONC->plan.depInfo,
                                     &(aQuerySet->depInfo) )
              != IDE_SUCCESS );

    /*
     * PROJ-1071 Parallel Query
     * parallel degree
     */
    if ( aLeftChild->mParallelDegree > aRightChild->mParallelDegree )
    {
        sCONC->plan.mParallelDegree = aLeftChild->mParallelDegree;
    }
    else
    {
        sCONC->plan.mParallelDegree = aRightChild->mParallelDegree;
    }
    sCONC->plan.flag &= ~QMN_PLAN_NODE_EXIST_MASK;
    sCONC->plan.flag |= ((aRightChild->flag | aLeftChild->flag) &
                         QMN_PLAN_NODE_EXIST_MASK);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoTwoNonPlan::checkSimpleJOIN( qcStatement  * aStatement,
                                qmncJOIN     * aJOIN )
{
/***********************************************************************
 *
 * Description : simple join 노드인지 검사한다.
 *
 * Implementation :
 *     simple join인 경우 fast execute를 수행한다.
 *
 ***********************************************************************/

    qmncJOIN  * sJOIN = aJOIN;
    idBool      sIsSimple = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmoTwoNonPlan::checkSimpleJOIN::__FT__" );

    IDE_TEST_CONT( qciMisc::checkExecFast( aStatement, NULL ) == ID_FALSE,
                   NORMAL_EXIT );

    sIsSimple = ID_TRUE;
    
    if ( ( ( sJOIN->flag & QMNC_JOIN_TYPE_MASK )
           != QMNC_JOIN_TYPE_INNER ) ||
         ( sJOIN->filter != NULL ) )
    {
        sIsSimple = ID_FALSE;
    }
    else
    {
        // Nothing to do.
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );
    
    sJOIN->isSimple = sIsSimple;
    
    return IDE_SUCCESS;
}

IDE_RC qmoTwoNonPlan::initSREC( qcStatement  * aStatement,
                                qmsQuerySet  * aQuerySet,
                                qmnPlan      * aParent,
                                qmnPlan     ** aPlan )
{
/***********************************************************************
 *
 * Description : PROJ-2582 recursive with
 *     SREC 노드를 생성한다
 *
 * Implementation :
 *     + 초기화 작업
 *         - qmncSREC의 할당 및 초기화
 *     + 메인 작업
 *     + 마무리 작업
 *         - data 영역의 크기 계산
 *         - dependency의 처리
 *         - subquery의 처리
 *
 * TO DO
 *
 ***********************************************************************/

    qmncSREC * sSREC = NULL;
    UInt       sDataNodeOffset = 0;

    IDU_FIT_POINT_FATAL( "qmoTwoNonPlan::initSREC::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );

    //------------------------------------------------------------
    // 초기화 작업
    //------------------------------------------------------------

    // qmncSREC의 할당및 초기화
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmncSREC),
                                             (void **)&sSREC )
              != IDE_SUCCESS );

    QMO_INIT_PLAN_NODE( sSREC,
                        QC_SHARED_TMPLATE(aStatement),
                        QMN_SREC,
                        qmnSREC,
                        qmndSREC,
                        sDataNodeOffset );

    *aPlan = (qmnPlan *)sSREC;

    IDE_TEST( qmc::pushResultDesc( aStatement,
                                   aQuerySet,
                                   ID_FALSE,
                                   aParent->resultDesc,
                                   &sSREC->plan.resultDesc )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoTwoNonPlan::makeSREC( qcStatement  * aStatement,
                                qmsQuerySet  * aQuerySet,
                                qmnPlan      * aLeftChild,
                                qmnPlan      * aRightChild,
                                qmnPlan      * aRecursiveChild,
                                qmnPlan      * aPlan )
{
/***********************************************************************
 *
 * Description : PROJ-2582 recursive with
 *      SREC 노드를 생성한다
 *
 * Implementation :
 *     + 초기화 작업
 *         - qmncSREC의 할당 및 초기화
 *     + 메인 작업
 *     + 마무리 작업
 *         - data 영역의 크기 계산
 *         - dependency의 처리
 *         - subquery의 처리
 *
 * TO DO
 *
 ***********************************************************************/

    qmncSREC    * sSREC = (qmncSREC *)aPlan;
    UInt          sDataNodeOffset = 0;

    IDU_FIT_POINT_FATAL( "qmoTwoNonPlan::makeSREC::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aLeftChild != NULL );
    IDE_DASSERT( aRightChild != NULL );
    IDE_DASSERT( aRecursiveChild != NULL );

    //------------------------------------------------------------
    // 초기화 작업
    //------------------------------------------------------------

    sSREC->plan.left    = aLeftChild;
    sSREC->plan.right   = aRightChild;
    
    if ( aRecursiveChild->type == QMN_VSCN )
    {
        sSREC->recursiveChild = aRecursiveChild;
    }
    else if ( aRecursiveChild->type == QMN_FILT )
    {
        IDE_TEST_RAISE( aRecursiveChild->left->type != QMN_VSCN,
                        ERR_INVALID_VSCN );
        
        sSREC->recursiveChild = aRecursiveChild->left;
    }
    else
    {
        IDE_RAISE( ERR_INVALID_VSCN );
    }
    
    aPlan->offset = aStatement->myPlan->sTmplate->tmplate.dataSize;
    sDataNodeOffset  = idlOS::align8(aPlan->offset +
                                     ID_SIZEOF(qmndSREC));

    //----------------------------------
    // Flag 설정
    //----------------------------------

    sSREC->flag      = QMN_PLAN_FLAG_CLEAR;
    sSREC->plan.flag = QMN_PLAN_FLAG_CLEAR;
    
    sSREC->plan.flag       |= (aLeftChild->flag & QMN_PLAN_STORAGE_MASK);
    sSREC->plan.flag       |= (aRightChild->flag & QMN_PLAN_STORAGE_MASK);

    //------------------------------------------------------------
    // 마무리 작업
    //------------------------------------------------------------

    //data 영역의 크기 계산
    QC_SHARED_TMPLATE(aStatement)->tmplate.dataSize = sDataNodeOffset;

    //----------------------------------
    // dependency 처리 및 subquery의 처리
    //----------------------------------

    // To Fix PR-12791
    // PROJ-1358 로 인해 dependency 결정 알고리즘이 수정되었으며
    // Query Set에서 설정한 dependency 정보를 누적해야 함.
    IDE_TEST( qmoDependency::setDependency( aStatement ,
                                            aQuerySet ,
                                            & sSREC->plan ,
                                            QMO_SREC_DEPENDENCY,
                                            (UShort)qtc::getPosFirstBitSet( & aQuerySet->depInfo ),
                                            NULL ,
                                            0 ,
                                            NULL ,
                                            0 ,
                                            NULL )
              != IDE_SUCCESS );

    IDE_TEST( qmc::filterResultDesc( aStatement,
                                     &sSREC->plan.resultDesc,
                                     &sSREC->plan.depInfo,
                                     &(aQuerySet->depInfo) )
              != IDE_SUCCESS );

    /*
     * PROJ-1071 Parallel Query
     * parallel degree
     */
    if ( aLeftChild->mParallelDegree > aRightChild->mParallelDegree )
    {
        sSREC->plan.mParallelDegree = aLeftChild->mParallelDegree;
    }
    else
    {
        sSREC->plan.mParallelDegree = aRightChild->mParallelDegree;
    }
    sSREC->plan.flag &= ~QMN_PLAN_NODE_EXIST_MASK;
    sSREC->plan.flag |= ((aRightChild->flag | aLeftChild->flag) &
                         QMN_PLAN_NODE_EXIST_MASK);
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_VSCN )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmoTwoNonPlan::makeSREC",
                                  "Invalid VSCN" ));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
