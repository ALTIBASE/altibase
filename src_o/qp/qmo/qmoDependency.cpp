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
 * $Id: qmoDependency.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *     Plan Dependency Manager
 *
 *     각 Plan Node 생성 시 Plan 간의 관계를 고려하여,
 *     해당 Plan의 Dependency를 설정하는 절차를 제공한다.
 *
 *     아래와 같이 총 6단계의 절차를 통해 Plan의 Depenendency가 결정된다.
 *
 *     - 1단계 : Set Tuple ID
 *     - 2단계 : Dependencies 생성
 *     - 3단계 : Table Map 보정
 *     - 4단계 : Dependency 결정
 *     - 5단계 : Dependency 보정
 *     - 6단계 : Dependencies 보정
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <qmoSubquery.h>
#include <qmoDependency.h>

IDE_RC
qmoDependency::setDependency( qcStatement * aStatement,
                              qmsQuerySet * aQuerySet,
                              qmnPlan     * aPlan,
                              UInt          aDependencyFlag,
                              UShort        aTupleID,
                              qmsTarget   * aTarget,
                              UInt          aPredCount,
                              qtcNode    ** aPredExpr,
                              UInt          aMtrCount,
                              qmcMtrNode ** aMtrNode )
{
/***********************************************************************
 *
 * Description :
 *    해당 Plan Node의 Dependency 및 Depedencies를 결정한다.
 *    이 외에도 Predicate이나 Expression에 존재하는
 *    Subquery의 Plan Tree를 생성한다.
 *
 *        - aDependencyFlag : 플랜의 Dependency관련된 Flag 를 설정
 *        - aTupleID : Tuple을 가지고 있는 경우 그 값을 셋팅
 *        - aTarget  : Target이 있는 경우 설정
 *        - aPredCount : Predicate 또는 Expression을 구성하는 종류의 개수
 *        - aPredExpr  : 모든 Predicate이나 Expression의 시작 위치
 *            Ex) SCAN 노드의 경우, 다음과 같은 Predicate이 존재함.
 *                - constantFilter, filter, subqueryFilter
 *                - fixKeyRangeOrg, varKeyRange,
 *                - fixKeyFilterOrg, varKeyRange,
 *                - 위의 경우 다음과 같이 값이 설정됨
 *                    - aPredCount : 7
 *                    - aPredExpr  : 해당 predicate의 시작 위치를 가리키는 배열
 *            Cf) 존재하지 않는 경우에도 설정함.
 *
 *        - aMtrCount : 저장 노드의 종류의 개수
 *        - aMtrNode  : 모든 저장 노드의 시작 위치
 *            Ex) AGGR 노드의 경우, 다음과 같은 두 종류의 저장 노드가 존재함.
 *                - myNode, distNode
 *
 * Implementation :
 *     - Tuple ID를 자기는 NODE들은 Table Map에 등록한다.
 *     - Dependency 설정의 각 6 단계를 수행한다.
 *          - 1 단계 : Tuple ID 설정 단계
 *          - 2 단계 : Dependencies 결정 단계
 *          - 3 단계 : Table Map 보정 단계
 *          - 4 단계 : Dependency 결정 단계
 *          - 5 단계 : Dependecny 보정 단계 : 4단계에 포함됨
 *          - 6 단계 : Dependencies 보정 단계
 *
 ***********************************************************************/

    qmsTarget  * sTarget;
    qmcMtrNode * sMtrNode;
    qtcNode    * sTargetNode;
    UInt         i;

    qcDepInfo sDependencies;
    qcDepInfo sTotalDependencies;
    qcDepInfo sMtrDependencies;

    IDU_FIT_POINT_FATAL( "qmoDependency::setDependency::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aQuerySet != NULL );
    IDE_DASSERT( aPlan != NULL );

    //---------------------------------------------------------
    // 1 단계 : Tuple ID 설정 단계
    //---------------------------------------------------------

    qtc::dependencyClear( & aPlan->depInfo );

    if( ( aDependencyFlag & QMO_DEPENDENCY_STEP1_SET_TABLE_MAP_MASK ) ==
        QMO_DEPENDENCY_STEP1_SET_TABLE_MAP_TRUE )
    {
        IDE_TEST( qmoDependency::step1setTupleID( aStatement,
                                                  aTupleID )
                  != IDE_SUCCESS );
    }
    else
    {
        //nothing to do
    }

    //---------------------------------------------------------
    // 2 단계 : Dependencies 결정 단계
    //---------------------------------------------------------

    qtc::dependencyClear( & sTotalDependencies );
    qtc::dependencyClear( & sMtrDependencies );

    //-------------------------------------------
    // Predicate/Expression의 모든 Dependencies추출
    //     - 2단계 또는 6단계에서 사용됨
    //-------------------------------------------

    // Target으로부터 Dependencies추출
    for ( sTarget = aTarget; sTarget != NULL; sTarget = sTarget->next )
    {
        qtc::dependencySetWithDep( & sDependencies ,
                                   & sTotalDependencies );

        // BUG-38228 group by 가 있을때는 target 에 pass node가 있을수 있다.
        if ( sTarget->targetColumn->node.module == &qtc::passModule )
        {
            sTargetNode = (qtcNode*)(sTarget->targetColumn->node.arguments);
        }
        else
        {
            sTargetNode = (qtcNode*)(sTarget->targetColumn);
        }

        IDE_TEST( qtc::dependencyOr( & sTargetNode->depInfo,
                                     & sDependencies,
                                     & sTotalDependencies )
                  != IDE_SUCCESS );
    }

    // Predicate이나 Expression으로부터 추출
    for ( i = 0; i < aPredCount; i++ )
    {
        if ( aPredExpr[i] != NULL )
        {
            qtc::dependencySetWithDep( & sDependencies ,
                                       & sTotalDependencies );
            IDE_TEST( qtc::dependencyOr( & aPredExpr[i]->depInfo,
                                         & sDependencies,
                                         & sTotalDependencies )
                      != IDE_SUCCESS );

        }
        else
        {
            // Nothing To Do
        }
    }

    //-------------------------------------------
    // 유형에 따른 2단계 수행
    //-------------------------------------------

    if ( ( aDependencyFlag & QMO_DEPENDENCY_STEP2_DEP_MASK ) ==
         QMO_DEPENDENCY_STEP2_DEP_WITH_PREDICATE )
    {
        //------------------------------------
        // Predicate이나 Expresssion으로부터
        // dependencies를 결정하는 경우
        //------------------------------------

        IDE_TEST( qmoDependency::step2makeDependencies( aPlan,
                                                        aDependencyFlag,
                                                        aTupleID,
                                                        & sTotalDependencies )
                  != IDE_SUCCESS );
    }
    else
    {
        //------------------------------------
        // Materialization Node들로부터
        // dependencies를 결정하는 경우
        // Join에 참여하는 HASH, SORT 노드가 이에 해당함.
        //------------------------------------

        IDE_DASSERT( aMtrNode != NULL && aMtrCount > 0 );

        for ( i = 0; i < aMtrCount; i++ )
        {
            for ( sMtrNode = aMtrNode[i];
                  sMtrNode != NULL;
                  sMtrNode = sMtrNode->next )
            {
                qtc::dependencySetWithDep( & sDependencies ,
                                           & sMtrDependencies );

                IDE_TEST( qtc::dependencyOr( & sMtrNode->srcNode->depInfo,
                                             & sDependencies,
                                             & sMtrDependencies )
                          != IDE_SUCCESS );
            }
        }

        IDE_TEST( qmoDependency::step2makeDependencies( aPlan,
                                                        aDependencyFlag,
                                                        aTupleID,
                                                        & sMtrDependencies )
                  != IDE_SUCCESS );

        // Materialized Column에 대한 Plan Tree를 생성함.
        for ( i = 0; i < aMtrCount; i++ )
        {
            for ( sMtrNode = aMtrNode[i];
                  sMtrNode != NULL;
                  sMtrNode = sMtrNode->next )
            {
                IDE_TEST( qmoSubquery::makePlan( aStatement,
                                                 aTupleID,
                                                 sMtrNode->srcNode )
                          != IDE_SUCCESS );
            }
        }
    }

    //---------------------------------------------------------
    // 3 단계 : Table Map 보정 단계
    //---------------------------------------------------------

    if ( ( aDependencyFlag & QMO_DEPENDENCY_STEP3_TABLEMAP_REFINE_MASK )
         == QMO_DEPENDENCY_STEP3_TABLEMAP_REFINE_TRUE )
    {
        IDE_TEST( qmoDependency::step3refineTableMap( aStatement,
                                                      aPlan,
                                                      aQuerySet,
                                                      aTupleID )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    // Target Column에 대한 plan tree를 생성한다.
    // 이는 3단계가 지난 후에 처리를 해주도록 한다.
    for ( sTarget = aTarget; sTarget != NULL; sTarget = sTarget->next )
    {
        // BUG-38228 group by 가 있을때는 target 에 pass node가 있을수 있다.
        if ( sTarget->targetColumn->node.module == &qtc::passModule )
        {
            sTargetNode = (qtcNode*)(sTarget->targetColumn->node.arguments);
        }
        else
        {
            sTargetNode = (qtcNode*)(sTarget->targetColumn);
        }

        IDE_TEST( qmoSubquery::makePlan( aStatement,
                                         aTupleID,
                                         sTargetNode )
                  != IDE_SUCCESS );
    }

    //--------------------------------------
    // Plan Tree 생성 및 Host 변수 등록
    //--------------------------------------

    for ( i = 0; i < aPredCount; i++ )
    {
        if ( aPredExpr[i] != NULL )
        {
            // Plan Tree 생성
            IDE_TEST( qmoSubquery::makePlan( aStatement,
                                             aTupleID,
                                             aPredExpr[i] )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing To Do
        }
    }

    //---------------------------------------------------------
    // 4 단계 : Dependency 결정 단계
    // 5 단계 : Dependecny 보정 단계 : 4단계에 포함됨
    //---------------------------------------------------------

    IDE_TEST( qmoDependency::step4decideDependency( aStatement ,
                                                    aPlan,
                                                    aQuerySet)
              != IDE_SUCCESS );

    //---------------------------------------------------------
    // 6 단계 : Dependencies 보정 단계
    //---------------------------------------------------------

    if( ( aDependencyFlag & QMO_DEPENDENCY_STEP6_DEPENDENCIES_REFINE_MASK ) ==
        QMO_DEPENDENCY_STEP6_DEPENDENCIES_REFINE_TRUE )
    {
        IDE_TEST( qmoDependency::step6refineDependencies( aPlan,
                                                          & sTotalDependencies)
                  != IDE_SUCCESS );
    }
    else
    {
        //nothing to do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoDependency::step1setTupleID( qcStatement * aStatement ,
                                UShort        aTupleID )
{
/***********************************************************************
 *
 * Description : Dependency관리중 1단계인 자신의 Tuple ID를 Table Map에
 *               등록한다.
 *
 * Implementation :
 *     - Tuple ID를 자기는 NODE들은 Table Map에 등록한다.
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmoDependency::step1setTupleID::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );

    //----------------------------------
    //Table Map에 등록
    //----------------------------------

    QC_SHARED_TMPLATE(aStatement)->tableMap[aTupleID].dependency = (ULong)aTupleID;

    return IDE_SUCCESS;
}

IDE_RC
qmoDependency::step2makeDependencies( qmnPlan     * aPlan ,
                                      UInt          aDependencyFlag,
                                      UShort        aTupleID ,
                                      qcDepInfo   * aDependencies )
{
/***********************************************************************
 *
 * Description : Dependency관리중 2단계인 대표 Dependency를 위해
 *               dependencies를 구성한다.
 *
 * Implementation :
 *     - 4가지로 구분을 한다.
 *         - 노드의 종류 : 개념상 Base Table을 의미 하는 NODE
 *         - 노드의 종류 : 하위 child의 dependencies를 포함하는 NODE
 *         - 연산의 종류 : 모든 Predicate 및 Expression의 dependencies
 *                         포함
 *         - 연산의 종류 : Materialization과 Predicate이 다른
 *                         dependencies를 가지는 경우
 *
 ***********************************************************************/

    qcDepInfo     sChildDependencies;
    qmnChildren * sChildren;

    IDU_FIT_POINT_FATAL( "qmoDependency::step2makeDependencies::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aPlan != NULL );
    IDE_DASSERT( aDependencies != NULL );

    //----------------------------------
    //노드에 따른 구분
    //----------------------------------

    if( ( aDependencyFlag & QMO_DEPENDENCY_STEP2_BASE_TABLE_MASK )
        == QMO_DEPENDENCY_STEP2_BASE_TABLE_TRUE )
    {
        //개념상 Base Table을 의미하는 NODE
        //Tuple ID로 부터 dependencies 표현
        qtc::dependencySet( aTupleID , & aPlan->depInfo );
    }
    else
    {
        if ( ( aDependencyFlag & QMO_DEPENDENCY_STEP2_SETNODE_MASK )
             == QMO_DEPENDENCY_STEP2_SETNODE_TRUE )
        {
            // PROJ-1358
            // SET 의 경우 dependency를 누적하지 않고,
            // Child의 대표 outer dependency만을 누적한다.
            qtc::dependencySet( aTupleID , & aPlan->depInfo );

            if ( aPlan->children == NULL )
            {
                // Left Child의 대표 dependency 누적
                if( aPlan->left != NULL )
                {
                    if ( (aPlan->left->flag & QMN_PLAN_OUTER_REF_MASK)
                         == QMN_PLAN_OUTER_REF_TRUE )
                    {
                        qtc::dependencySet( (UShort)aPlan->left->outerDependency,
                                            & sChildDependencies );
                        IDE_TEST( qtc::dependencyOr( & aPlan->depInfo,
                                                     & sChildDependencies,
                                                     & aPlan->depInfo )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        // dependency를 누적하지 않는다.
                    }
                }
                else
                {
                    // Nothing to do.
                }

                // Right Child의 대표 dependency 누적
                if( aPlan->right != NULL )
                {
                    if ( (aPlan->right->flag & QMN_PLAN_OUTER_REF_MASK)
                         == QMN_PLAN_OUTER_REF_TRUE )
                    {
                        qtc::dependencySet( (UShort)aPlan->right->outerDependency,
                                            & sChildDependencies );
                        IDE_TEST( qtc::dependencyOr( & aPlan->depInfo,
                                                     & sChildDependencies,
                                                     & aPlan->depInfo )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        // dependency를 누적하지 않는다.
                    }
                }
                else
                {
                    // Nothing to do.
                }
            }
            else
            {
                // PROJ-1486
                // Multiple Children Set 인 경우
                for ( sChildren = aPlan->children;
                      sChildren != NULL;
                      sChildren = sChildren->next )
                {
                    if ( (sChildren->childPlan->flag & QMN_PLAN_OUTER_REF_MASK)
                         == QMN_PLAN_OUTER_REF_TRUE )
                    {
                        qtc::dependencySet( (UShort)sChildren->childPlan->outerDependency,
                                            & sChildDependencies );
                        IDE_TEST( qtc::dependencyOr( & aPlan->depInfo,
                                                     & sChildDependencies,
                                                     & aPlan->depInfo )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        // dependency를 누적하지 않는다.
                    }
                }
            }
        }
        else
        {
            //Base Table이 아니라서, 하위의 Dependencies를 포함하는 NODE
            if( aPlan->left != NULL )
            {
                if( aPlan->right == NULL )
                {
                    qtc::dependencySetWithDep( & aPlan->depInfo,
                                               & aPlan->left->depInfo );
                }
                else
                {
                    IDE_TEST( qtc::dependencyOr( & aPlan->left->depInfo,
                                                 & aPlan->right->depInfo,
                                                 & aPlan->depInfo )
                              != IDE_SUCCESS );
                }
            }
            else
            {
                // Nothing to do.
            }
        }
    }

    //----------------------------------
    //연산에 따른 구분
    //----------------------------------

    //연산과 materialize column들의 dependencies를 추가한다.
    //NODE에 따라서 dependencies는 미리 계산되어서 입력된다.
    IDE_TEST( qtc::dependencyOr( & aPlan->depInfo,
                                 aDependencies,
                                 & aPlan->depInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoDependency::step3refineTableMap( qcStatement * aStatement ,
                                    qmnPlan     * aPlan ,
                                    qmsQuerySet * aQuerySet ,
                                    UShort        aTupleID )
{
/***********************************************************************
 *
 * Description : Dependency관리중 3단계인 Table Map을 보정한다.
 *
 * Implementation :
 *     - Materialization NODE들은 Table Map의 dependency를 보정한다.
 *     - 해당 노드의
 ***********************************************************************/

    qcDepInfo sDependencies;
    SInt      sTableMapIndex;

    IDU_FIT_POINT_FATAL( "qmoDependency::step3refineTableMap::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aPlan != NULL );
    IDE_DASSERT( aQuerySet != NULL );

    qtc::dependencyClear( & sDependencies );

    //----------------------------------
    //dependencies에 해당하는 테이블 추출
    //----------------------------------

    //해당 노드의 dependencies로 부터 해당 querySet에 해당하는
    //dependencies추출
    qtc::dependencyAnd( & aPlan->depInfo,
                        & aQuerySet->depInfo,
                        & sDependencies );

    //추출된 dependencies에 해당하는 테이블 추출
    sTableMapIndex = qtc::getPosFirstBitSet( & sDependencies );

    //----------------------------------
    //Table Map의 보정
    //----------------------------------
    while( sTableMapIndex != QTC_DEPENDENCIES_END )
    {
        //추출된 Table의 dependency에 TupleID 변경
        QC_SHARED_TMPLATE(aStatement)->tableMap[sTableMapIndex].dependency = (ULong)aTupleID;
        sTableMapIndex = qtc::getPosNextBitSet( & sDependencies ,
                                                sTableMapIndex );
    }

    return IDE_SUCCESS;
}

IDE_RC
qmoDependency::step4decideDependency( qcStatement * aStatement ,
                                      qmnPlan     * aPlan ,
                                      qmsQuerySet * aQuerySet )
{
/***********************************************************************
 *
 * Description : Dependency관리중 4단계인 대표 Dependency를 결정한다.
 *
 * Implementation :
 *     - Outer Column Reference가 존재하는지 검사한다.
 *         - plan의 dependencies & NOT(현query의 dependencies)
 *             - Reference가 없는 경우 :
 *                      해당 querySet의 join Order를 참조
 *             - Reference가 있는 경우 :
 *                      상위 querySet의 join Order를 참조
 *     - Dependencies와 Join Order로 부터 가장 오른쪽 Table을 찾는다.
 *     - 아래의 5단계수행은 Outer Column Reference의 유무를 알아야
 *       하므로 4단계에서 같이 처리 한다.
 *
 ***********************************************************************/

    qcDepInfo      sDependencies;
    qcDepInfo      sBaseDepInfo;
    idBool         sHaveDependencies;
    qmsQuerySet  * sQuerySet;
    qmsSFWGH     * sSFWGH;

    IDU_FIT_POINT_FATAL( "qmoDependency::step4decideDependency::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aPlan != NULL );
    IDE_DASSERT( aQuerySet != NULL );

    //----------------------------------
    //대표 Dependency의 결정
    //----------------------------------

    //----------------------------------
    //outer column reference 찾기
    //----------------------------------

    // PROJ-2418
    // Lateral Dependency가 존재하는 경우엔, 기준 dependency를 교체한다
    qtc::dependencyClear( & sBaseDepInfo );

    if ( qtc::haveDependencies ( & aQuerySet->lateralDepInfo ) == ID_TRUE )
    {
        // QuerySet에서 외부 참조를 해야 하는 상황이라면
        // QuerySet의 Lateral Dependencies를 기준
        qtc::dependencySetWithDep( & sBaseDepInfo, & aQuerySet->lateralDepInfo );

        // 이 경우, 첫 번째 경우 (dependencies가 없는 경우) 는
        // 무조건 통과하게 된다. 두 번째 경우로 이동해 대표 dependency를 찾는다.
    }
    else
    {
        // Lateral View가 존재하지 않는 QuerySet인 경우라면
        // 기존과 같이 Plan의 Dependencies를 기준
        qtc::dependencySetWithDep( & sBaseDepInfo, & aPlan->depInfo );
    }

    if( qtc::dependencyContains( & aQuerySet->depInfo,
                                 & sBaseDepInfo ) == ID_TRUE )
    {
        //---------------------------------
        // dependencies가 없는 경우.
        // 즉, outer column reference가 없는 경우
        //---------------------------------

        if( aQuerySet->SFWGH != NULL )
        {
            //현재 querySet의 join order와 aPlan의 dependencies를 통해
            //가장 오른쪽 order를 찾는다.
            sSFWGH = aQuerySet->SFWGH;
            IDE_TEST( findRightJoinOrder( sSFWGH,
                                          &sBaseDepInfo,
                                          &sDependencies )
                      != IDE_SUCCESS );

            sHaveDependencies = qtc::haveDependencies( & sDependencies );
            if( sHaveDependencies == ID_TRUE )
            {
                //찾은 dependencies를 대표 dependencies로 삼는다
                aPlan->dependency = qtc::getPosFirstBitSet( & sDependencies );
            }
            else
            {
                //찾지 못한 경우
                IDE_DASSERT( 0 );
            }
        }
        else
        {
            //즉, SET인 경우에는 SFWGH가 NULL이고 right, left로 달려
            //있는데 이 경우 VIEW가 생성되는데 이는 새로운 Relation의
            //생성이므로, leaf노드 처럼 처리가 되어야 함이다.
            //(즉, 자신의 대표dependency를 가지고 있는다.)
            aPlan->dependency = qtc::getPosFirstBitSet( & sBaseDepInfo );
        }

        // PROJ-1358
        // Outer Reference 존재여부를 설정
        aPlan->flag &= ~QMN_PLAN_OUTER_REF_MASK;
        aPlan->flag |= QMN_PLAN_OUTER_REF_FALSE;
    }
    else
    {
        //---------------------------------
        // dependencies가 있는 경우.
        // 즉, outer column reference가 있는 경우
        //---------------------------------

        // SET등을 표현하는 Query Set일 경우,
        // 상위 Query를 찾기 위해 Right-most Query Set으로 이동한다.
        sQuerySet = aQuerySet;
        while ( sQuerySet->right != NULL )
        {
            sQuerySet = sQuerySet->right;
        }
        sSFWGH = sQuerySet->SFWGH;

        IDE_TEST_RAISE( sSFWGH == NULL, ERR_INVALID_SFWGH );

        while( sSFWGH->outerQuery != NULL )
        {
            //상위 querySet의 join order에서 찾는다.
            //결과가 없으면 다시 상위로 올라간다.
            sSFWGH = sSFWGH->outerQuery;

            // PROJ-1413
            // outerQuery는 view merging에 의해 더이상 유효하지 않을 수 있다.
            // merge된 SFWGH로 찾아가야 한다.
            while ( sSFWGH->mergedSFWGH != NULL )
            {
                sSFWGH = sSFWGH->mergedSFWGH;
            }

            if ( sSFWGH->crtPath != NULL )
            {
                IDE_TEST( findRightJoinOrder( sSFWGH,
                                              &sBaseDepInfo,
                                              &sDependencies )
                          != IDE_SUCCESS );

                sHaveDependencies = qtc::haveDependencies( & sDependencies );

                if( sHaveDependencies == ID_TRUE )
                {
                    //찾은 dependencies를 대표 dependencies로 삼는다
                    aPlan->dependency = qtc::getPosFirstBitSet( & sDependencies );

                    // PROJ-1358
                    // 보정 전의 대표 outer dependency를 기록한다.
                    aPlan->outerDependency = aPlan->dependency;

                    // 5단계 처리
                    // Table Map으로부터 보정된 dependency값을 설정한다.
                    aPlan->dependency =
                        QC_SHARED_TMPLATE(aStatement)->tableMap[aPlan->
                                                      dependency].dependency;
                    break;
                }
                else
                {
                    // Nothing To Do
                }
            }
            else
            {
                // Nothing To Do
            }
        }

        // PROJ-1358
        // Outer Reference 존재여부를 설정
        aPlan->flag &= ~QMN_PLAN_OUTER_REF_MASK;
        aPlan->flag |= QMN_PLAN_OUTER_REF_TRUE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_SFWGH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmoDependency::step4decideDependency",
                                  "SFWGH is null" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoDependency::step6refineDependencies( qmnPlan     * aPlan ,
                                        qcDepInfo   * aDependencies )
{
/***********************************************************************
 *
 * Description : Dependency 관리중 6단계인 상위 NODE를 위한 Dependencies
 *               를 보정한다.
 *
 * Implementation :
 *     - 2단계에서 HASH , SORT는 Materialize column에 대해서만
 *       dependencies를 설정했으므로 Predicate 및 Expression에 대해서도
 *       dependencies를 적용해준다.
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmoDependency::step6refineDependencies::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aPlan != NULL );
    IDE_DASSERT( aDependencies != NULL );

    //----------------------------------
    //dependencies의 보정
    //----------------------------------

    //Predicate 및 Expression들의 dependencies 추가
    //depdencies는 미리 계산되어서 입력된다.
    IDE_TEST( qtc::dependencyOr( & aPlan->depInfo,
                                 aDependencies ,
                                 & aPlan->depInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoDependency::findRightJoinOrder( qmsSFWGH   * aSFWGH ,
                                   qcDepInfo  * aInputDependencies ,
                                   qcDepInfo  * aOutputDependencies )
{
/***********************************************************************
 *
 * Description : 현재 Dependencies와 Join Order로 부터 가장 오른쪽
 *               order의 Table을 찾는다.
 *
 * Implementation :
 *     - qmoTableOrder를 순회 하면서 input dependencies와 AND연산하여
 *       dependencies가 존재할 경우 기록 해둔다.
 *
 *     - dependencies가 없는 경우
 *          -
 ***********************************************************************/

    qmoTableOrder * sTableOrder;
    qcDepInfo       sDependencies;
    qcDepInfo       sRecursiveViewDep;
    idBool          sHaveDependencies;

    IDU_FIT_POINT_FATAL( "qmoDependency::findRightJoinOrder::__FT__" );

    //----------------------------------
    // 적합성 검사
    //----------------------------------

    IDE_DASSERT( aSFWGH != NULL );
    IDE_DASSERT( aInputDependencies != NULL );
    IDE_DASSERT( aOutputDependencies != NULL );

    //----------------------------------
    // 초기화
    //----------------------------------

    sTableOrder = aSFWGH->crtPath->currentCNF->tableOrder;

    //결과 dependencies는 없을 경우 clear되어 리턴된다.
    qtc::dependencyClear( aOutputDependencies );

    // PROJ-2582 recursive with
    // recursive view의 dependency 정보를 생성한다.
    if ( aSFWGH->recursiveViewID != ID_USHORT_MAX )
    {
        qtc::dependencySet( aSFWGH->recursiveViewID,
                            & sRecursiveViewDep );
    }
    else
    {
        qtc::dependencyClear( & sRecursiveViewDep );
    }

    //----------------------------------
    // Join Order에서 오른쪽 Table 검색
    //----------------------------------

    //Join Order를 순회 한다.
    while( sTableOrder != NULL )
    {
        //join order와 입력된 dependencies와 AND연산하여
        //해당 join order에 있는지 검사한다.
        qtc::dependencyAnd( & sTableOrder->depInfo,
                            aInputDependencies ,
                            & sDependencies );

        sHaveDependencies = qtc::haveDependencies( & sDependencies );

        //join order에 가장 마지막에 있는 공통 dependencies가 가장
        //오른쪽이 된다.
        if ( sHaveDependencies == ID_TRUE )
        {
            qtc::dependencySetWithDep( aOutputDependencies ,
                                       & sDependencies );

            // PROJ-2582 recursive with
            // recursive view가 있는 경우 중지하고, recursive view를
            // dependency로 설정하게 한다.
            if ( ( qtc::haveDependencies( & sRecursiveViewDep )
                   == ID_TRUE ) &&
                 ( qtc::dependencyContains( & sDependencies,
                                            & sRecursiveViewDep )
                   == ID_TRUE ) )
            {
                qtc::dependencySetWithDep( aOutputDependencies ,
                                           & sRecursiveViewDep );
                break;
            }
            else
            {
                // Nothing To Do
            }
        }
        else
        {
            // Nothing To Do
        }

        sTableOrder = sTableOrder->next;
    }

    return IDE_SUCCESS;
}

