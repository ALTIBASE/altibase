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
 * $Id: qmvOrderBy.cpp 89835 2021-01-22 10:10:02Z andrew.shin $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qtc.h>
#include <qtcDef.h>
#include <qmvOrderBy.h>
#include <qmvQuerySet.h>
#include <qmvQTC.h>
#include <qmn.h>
#include <qcuSqlSourceInfo.h>
#include <qmv.h>

extern mtfModule mtfDecrypt;

IDE_RC qmvOrderBy::searchSiblingColumn( qtcNode  * aExpression,
                                        qtcNode ** aColumn,
                                        idBool   * aFind )
{
    IDU_FIT_POINT_FATAL( "qmvOrderBy::searchSiblingColumn::__FT__" );

    if ( QTC_IS_SUBQUERY( aExpression ) == ID_FALSE )
    {
        if ( ( aExpression->node.module == &(qtc::columnModule ) ) &&
             ( *aFind == ID_FALSE ) )
        {
            *aColumn = aExpression;
            *aFind = ID_TRUE;
        }
        else
        {
            /* Nothing to do */
        }

        if ( *aFind == ID_FALSE )
        {
            if ( aExpression->node.next != NULL )
            {
                IDE_TEST( searchSiblingColumn( (qtcNode *)aExpression->node.next,
                                               aColumn,
                                               aFind )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }

            if ( aExpression->node.arguments != NULL )
            {
                IDE_TEST( searchSiblingColumn( (qtcNode *)aExpression->node.arguments,
                                               aColumn,
                                               aFind )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvOrderBy::validate(qcStatement * aStatement)
{

    qmsParseTree    * sParseTree;
    qmsSortColumns  * sCurrSort;
    qmsQuerySet     * sQuerySet;
    qmsSFWGH        * sSFWGH;
    qtcNode         * sSortColumn;
    qtcOverColumn   * sOverColumn;
    mtcColumn       * sColumn;
    idBool            sFind          = ID_FALSE;
    qtcNode         * sFindColumn    = NULL;
    qtcNode         * sSiblingColumn = NULL;
    qmsSortColumns  * sSibling       = NULL;
    qmsSortColumns  * sSort          = NULL;
    qmsSortColumns    sTemp;
    qcuSqlSourceInfo  sqlInfo;
    qtcNode         * sNewNode = NULL;
    qtcNode         * sPrevNode = NULL;

    IDU_FIT_POINT_FATAL( "qmvOrderBy::validate::__FT__" );

    //---------------
    // 기본 초기화
    //---------------
    sParseTree = (qmsParseTree*) aStatement->myPlan->parseTree;

    //---------------
    // validate 처리 단계 설정
    //---------------
    sParseTree->querySet->processPhase = QMS_VALIDATE_ORDERBY;

    /* BUG-46728 */
    if ( QCU_COERCE_HOST_VAR_TO_VARCHAR > 0 )
    {
        for ( sCurrSort = sParseTree->orderBy;
              sCurrSort != NULL;
              sCurrSort = sCurrSort->next)
        {
            IDE_TEST( qmvQuerySet::searchHostVarAndAddCastOper( aStatement,
                                                                sCurrSort->sortColumn,
                                                                &sNewNode,
                                                                ID_FALSE )
                      != IDE_SUCCESS );

            if ( sNewNode != NULL )
            {
                sCurrSort->sortColumn = sNewNode;

                if ( sPrevNode != NULL )
                {
                    sPrevNode->node.next = (mtcNode*)sNewNode;
                }
                else
                {
                    /* Nothing to do */
               }
            }
            else
            {
                /* Nothing to do */
            }

            sPrevNode = sCurrSort->sortColumn;
        }
    }
    else
    {
        /* Nothing to do */
    }

    if (sParseTree->querySet->setOp == QMS_NONE)
    {
        // This Query doesn't have SET(UNION, INTERSECT, MINUS) operations.
        sSFWGH  = sParseTree->querySet->SFWGH;

        if (sSFWGH->group != NULL)
        {
            IDE_TEST(validateSortWithGroup(aStatement) != IDE_SUCCESS);
        }
        else
        {
            IDE_TEST(validateSortWithoutGroup(aStatement) != IDE_SUCCESS);
        }

        if (sSFWGH->selectType == QMS_DISTINCT)
        {
            for (sCurrSort = sParseTree->orderBy;
                 sCurrSort != NULL;
                 sCurrSort = sCurrSort->next)
            {
                if (sCurrSort->targetPosition <= QMV_EMPTY_TARGET_POSITION)
                {
                    IDE_TEST(qmvQTC::isSelectedExpression(
                                 aStatement,
                                 sCurrSort->sortColumn,
                                 sSFWGH->target)
                             != IDE_SUCCESS);
                }
            }
        }
    }
    else 
    {
        // UNION, UNION ALL, INTERSECT, MINUS
        IDE_TEST(validateSortWithSet(aStatement) != IDE_SUCCESS);
    }

    // check host variable
    for (sCurrSort = sParseTree->orderBy;
         sCurrSort != NULL;
         sCurrSort = sCurrSort->next)
    {
        IDE_FT_ASSERT( sCurrSort->sortColumn != NULL );

        // BUG-27208
        if ( sCurrSort->sortColumn->node.module == &qtc::assignModule )
        {
            sSortColumn = (qtcNode *)
                sCurrSort->sortColumn->node.arguments;
        }
        else
        {
            sSortColumn = sCurrSort->sortColumn;
        }

        // PROJ-1492
        // BUG-42041
        if ( ( MTC_NODE_IS_DEFINED_TYPE( & sSortColumn->node ) == ID_FALSE ) &&
             ( aStatement->calledByPSMFlag == ID_FALSE ) )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & sSortColumn->position );
            IDE_RAISE( ERR_NOT_ALLOW_HOSTVAR );
        }

        // BUG-14094
        if ( ( sSortColumn->node.lflag & MTC_NODE_OPERATOR_MASK )
             == MTC_NODE_OPERATOR_LIST )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & sSortColumn->position );
            IDE_RAISE( ERR_NOT_APPLICABLE_TYPE_IN_ORDER_BY );
        }
        else
        {
            // Nothing to do.
        }

        // BUG-14094
        if ( ( sSortColumn->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
             == MTC_NODE_LOGICAL_CONDITION_TRUE )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & sSortColumn->position );
            IDE_RAISE( ERR_NOT_APPLICABLE_TYPE_IN_ORDER_BY );
        }
        else
        {
            // Nothing to do.
        }

        // BUG-14094
        if ( ( sSortColumn->node.lflag & MTC_NODE_COMPARISON_MASK )
             == MTC_NODE_COMPARISON_TRUE )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & sSortColumn->position );
            IDE_RAISE( ERR_NOT_APPLICABLE_TYPE_IN_ORDER_BY );
        }
        else
        {
            // Nothing to do.
        }

        // BUG-16083
        if ( ( sSortColumn->node.lflag & MTC_NODE_OPERATOR_MASK )
             == MTC_NODE_OPERATOR_SUBQUERY )
        {
            if ( (sSortColumn->node.lflag &
                  MTC_NODE_ARGUMENT_COUNT_MASK) > 1 )
            {
                sqlInfo.setSourceInfo( aStatement,
                                       & sSortColumn->position );
                IDE_RAISE( ERR_NOT_APPLICABLE_TYPE_IN_ORDER_BY );
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

        // BUG-15896
        // BUG-24133
        sColumn = QTC_TMPL_COLUMN( QC_SHARED_TMPLATE(aStatement), sSortColumn );

        if ( ( sColumn->module->id == MTD_BOOLEAN_ID ) ||
             ( sColumn->module->id == MTD_ROWTYPE_ID ) ||
             ( sColumn->module->id == MTD_RECORDTYPE_ID ) ||
             ( sColumn->module->id == MTD_ASSOCIATIVE_ARRAY_ID ) ||
             ( sColumn->module->id == MTD_REF_CURSOR_ID ) )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & sSortColumn->position );
            IDE_RAISE( ERR_NOT_APPLICABLE_TYPE_IN_ORDER_BY );
        }
        else
        {
            // Nothing to do.
        }

        // PROJ-1362
        // Indicator가 가리키는 칼럼이 Equal연산이 불가능한
        // 타입(Lob or Binary Type)인 경우, 에러 반환
        // BUG-22817 : 명시적인 경우와  암시적인 경우 모두 검사해야함
        if ( ( sSortColumn->lflag & QTC_NODE_BINARY_MASK )
             == QTC_NODE_BINARY_EXIST )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & sSortColumn->position );
            IDE_RAISE( ERR_NOT_ALLOWED_INCOMPARABLE_DATA_TYPE );
        }
        else
        {
            // Nothing to do.
        }

        /*
         * PROJ-1789 PROWID
         * ordery by 절에 _prowid 지원하지 않는다.
         */
        if ((sSortColumn->lflag & QTC_NODE_COLUMN_RID_MASK) ==
            QTC_NODE_COLUMN_RID_EXIST)
        {
            sqlInfo.setSourceInfo(aStatement, &sSortColumn->position);
            IDE_RAISE( ERR_NOT_ALLOWED_INCOMPARABLE_DATA_TYPE );
        }
        else
        {
        }

        if ( sSortColumn->overClause != NULL )
        {
            for ( sOverColumn = sSortColumn->overClause->overColumn;
                  sOverColumn != NULL;
                  sOverColumn = sOverColumn->next )
            {
                // 리스트 타입의 사용 여부 확인
                if ( ( sOverColumn->node->node.lflag & MTC_NODE_OPERATOR_MASK )
                     == MTC_NODE_OPERATOR_LIST )
                {
                    sqlInfo.setSourceInfo( aStatement,
                                           & sOverColumn->node->position );
                    IDE_RAISE( ERR_NOT_APPLICABLE_TYPE_IN_ORDER_BY );
                }
                else
                {
                    // Nothing to do.
                }

                // 서브쿼리가 사용 되었을때 타겟 컬럼이 두개이상인지 확인
                if ( ( sOverColumn->node->node.lflag & MTC_NODE_OPERATOR_MASK )
                     == MTC_NODE_OPERATOR_SUBQUERY )
                {
                    if ( ( sOverColumn->node->node.lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) > 1 )
                    {
                        sqlInfo.setSourceInfo( aStatement,
                                               & sOverColumn->node->position );
                        IDE_RAISE( ERR_NOT_APPLICABLE_TYPE_IN_ORDER_BY );
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

                // BUG-35670 over 절에 lob, geometry type 사용 불가
                if ((sOverColumn->node->lflag & QTC_NODE_BINARY_MASK) ==
                    QTC_NODE_BINARY_EXIST)
                {
                    sqlInfo.setSourceInfo( aStatement,
                                           &sOverColumn->node->position );
                    IDE_RAISE( ERR_NOT_APPLICABLE_TYPE_IN_ORDER_BY );
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
    }

    // disconnect constant node
    if (sParseTree->querySet->setOp == QMS_NONE)
    {
        // PROJ-1413
        IDE_TEST( disconnectConstantNode( sParseTree )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    /* PROJ-1715 */
    if ( sParseTree->querySet->SFWGH != NULL )
    {
        if ( sParseTree->querySet->SFWGH->hierarchy != NULL )
        {
            if ( sParseTree->isSiblings == ID_TRUE )
            {
                /* ORDER SIBLING BY 의 컬럼중에 Hierarhy와 관련된 것만 뽑아서
                 * ORDER SIBLING BY를 재 구성한다.
                 */
                sSibling   = &sTemp;
                sTemp.next = NULL;
                for (sCurrSort = sParseTree->orderBy;
                     sCurrSort != NULL;
                     sCurrSort = sCurrSort->next)
                {
                    /* BUG-36707 wrong result using many siblings by column */
                    sFind = ID_FALSE;

                    if ( QTC_IS_PSEUDO( sCurrSort->sortColumn ) == ID_TRUE )
                    {
                        sqlInfo.setSourceInfo( aStatement,
                                               &sCurrSort->sortColumn->position );
                        IDE_RAISE( ERR_NOT_ALLOW_PSEUDO_ORDER_SIBLINGS_BY );
                    }
                    else
                    {
                        /* Nothing to do */
                    }

                    IDE_TEST( searchSiblingColumn( sCurrSort->sortColumn,
                                                   &sFindColumn,
                                                   &sFind )
                              != IDE_SUCCESS );

                    if ( sFind == ID_FALSE )
                    {
                        continue;
                    }
                    else
                    {
                        IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                                                qmsSortColumns,
                                                &sSort )
                                  != IDE_SUCCESS );
                        idlOS::memcpy( sSort, sCurrSort, sizeof( qmsSortColumns ) );
                        sSort->targetPosition = QMV_EMPTY_TARGET_POSITION;
                        sSort->next           = NULL; /* BUG-46184 */
                        IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                                                qtcNode,
                                                &sSiblingColumn )
                                  != IDE_SUCCESS );

                        idlOS::memcpy( sSiblingColumn, sFindColumn, sizeof( qtcNode ) );
                        sSiblingColumn->node.next = NULL;
                        sSiblingColumn->node.arguments = NULL;

                        sSort->sortColumn = sSiblingColumn;
                        sSibling->next    = sSort;
                        sSibling          = sSibling->next;
                    }
                }
                sParseTree->querySet->SFWGH->hierarchy->siblings = sTemp.next;
                sParseTree->orderBy = NULL;
            }
            else
            {
                sParseTree->querySet->SFWGH->hierarchy->siblings = NULL;
            }
        }
        else
        {
            IDE_TEST_RAISE( sParseTree->isSiblings == ID_TRUE,
                            ERR_NOT_ALLOW_ORDER_SIBLINGS_BY );
        }
    }
    else
    {
        IDE_TEST_RAISE( sParseTree->isSiblings == ID_TRUE,
                        ERR_NOT_ALLOW_ORDER_SIBLINGS_BY );
    }

    // BUG-41221 Lateral View에서의 Order By 절 외부참조
    // Sort Column의 depInfo가 QuerySet의 depInfo 밖에 있다면
    // Sort Column의 depInfo를 QuerySet의 lateralDepInfo에 추가한다.
    sQuerySet = sParseTree->querySet;

    for ( sCurrSort = sParseTree->orderBy;
          sCurrSort != NULL;
          sCurrSort = sCurrSort->next )
    {
        if ( qtc::dependencyContains( & sQuerySet->depInfo,
                                      & sCurrSort->sortColumn->depInfo )
             == ID_FALSE )
        {
            // QuerySet의 depInfo 밖에 있으니, 외부 참조
            // 우선 SortColumn의 depInfo를 lateralDepInfo에 설정
            IDE_TEST( qtc::dependencyOr( & sQuerySet->lateralDepInfo,
                                         & sCurrSort->sortColumn->depInfo,
                                         & sQuerySet->lateralDepInfo )
                      != IDE_SUCCESS );
        }
        else
        {
            // QuerySet의 depInfo 안에 있으니, 내부 참조
        }
    }

    // BUG-41967 lateralDepInfo에서 현재 dependency는 제외
    if ( qtc::haveDependencies( & sQuerySet->lateralDepInfo ) == ID_TRUE )
    {
        qtc::dependencyMinus( & sQuerySet->lateralDepInfo,
                              & sQuerySet->depInfo,
                              & sQuerySet->lateralDepInfo );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_ALLOW_HOSTVAR );
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QMV_NOT_ALLOWED_HOSTVAR,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_NOT_APPLICABLE_TYPE_IN_ORDER_BY );
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QMV_NOT_APPLICABLE_TYPE_IN_ORDER_BY,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_NOT_ALLOWED_INCOMPARABLE_DATA_TYPE)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QMV_NOT_ALLOWED_INCOMPARABLE_DATA_TYPE,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_NOT_ALLOW_ORDER_SIBLINGS_BY )
    {
        IDE_SET( ideSetErrorCode(qpERR_ABORT_QMV_NOT_ALLOW_ORDER_SIBLINGS_BY) );
    }
    IDE_EXCEPTION( ERR_NOT_ALLOW_PSEUDO_ORDER_SIBLINGS_BY )
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_UNSUPPORTED_PSEUDO_COLUMN_IN_ORDER_SIBLINGS_BY,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmvOrderBy::validateSortWithGroup(qcStatement * aStatement)
{
/***********************************************************************
 *
 * Description :
 *     GROUP BY가 함께 존재하는 ORDER BY에 대한 Vaildation 수행
 *
 * Implementation :
 *     - ORDER BY에 존재하는 Column이 Target과 동일한 것이 있는 지를
 *       검사하여 Indicator를 작성한다.
 *     - ORDER BY에 존재하는 Column이 GROUP BY에 존재하는 Column인지를
 *       검사한다.
 *
 ***********************************************************************/

    qmsParseTree    * sParseTree;
    qmsSortColumns  * sCurrSort;
    qmsSFWGH        * sSFWGH;
    qmsTarget       * sTarget;
    SInt              sTargetMaxPos = 0;
    SInt              sCurrTargetPos;
    qmsAggNode      * sAggsDepth1;
    qmsAggNode      * sAggsDepth2;
    qcuSqlSourceInfo  sqlInfo;

    qtcNode         * sTargetNode1;
    qtcNode         * sTargetNode2;
    idBool            sIsTrue = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmvOrderBy::validateSortWithGroup::__FT__" );

    sParseTree = (qmsParseTree*) aStatement->myPlan->parseTree;
    sSFWGH  = sParseTree->querySet->SFWGH;
    sAggsDepth1 = sSFWGH->aggsDepth1;
    sAggsDepth2 = sSFWGH->aggsDepth2;

    for (sTarget = sSFWGH->target;
         sTarget != NULL;
         sTarget = sTarget->next)
    {
        sTargetMaxPos++;
    }

    for (sCurrSort = sParseTree->orderBy;
         sCurrSort != NULL;
         sCurrSort = sCurrSort->next)
    {
        //-----------------------------------------------
        // ORDER BY가 indicator인지를 검사
        // 명시적 indicator인 경우
        //     Ex) ORDER BY 1;
        // Target과 동일한 경우(암시적 Indicator)
        //     Ex) SELECT t1.i1 FROM T1 GROUP BY I1 ORDER BY I1;
        //     Ex) SELECT t1.i1 A FROM T1 GROUP BY i1 ORDER BY A;
        //-----------------------------------------------

        //-----------------------------------------------
        // 1. 명시적 indicator인지를 검사
        //-----------------------------------------------

        IDE_TEST(qtc::getSortColumnPosition(sCurrSort, QC_SHARED_TMPLATE(aStatement))
                 != IDE_SUCCESS);

        //-----------------------------------------------
        // 2. 암시적 indicator인지를 검사
        //-----------------------------------------------

        if (sCurrSort->targetPosition < QMV_EMPTY_TARGET_POSITION)
        {
            /* PROJ-2197 PSM Renewal */
            sCurrSort->sortColumn->lflag |= QTC_NODE_COLUMN_CONVERT_TRUE;
            IDE_TEST(qtc::estimate(
                         sCurrSort->sortColumn,
                         QC_SHARED_TMPLATE(aStatement),
                         aStatement,
                         sParseTree->querySet,
                         sSFWGH,
                         NULL )
                     != IDE_SUCCESS);

            if ( ( sCurrSort->sortColumn->node.lflag & MTC_NODE_DML_MASK )
                 == MTC_NODE_DML_UNUSABLE )
            {
                sqlInfo.setSourceInfo( aStatement,
                                       & sCurrSort->sortColumn->position );
                IDE_RAISE( ERR_USE_CURSOR_ATTR );
            }
            else
            {
                // 암시적 Indicator인지 검사한다.
                // Target의 ID와 ORDER BY의 ID가 동일하다면
                // Indicator의 역활을 하게 된다.

                for (sTarget = sSFWGH->target, sCurrTargetPos = 1;
                     sTarget != NULL;
                     sTarget = sTarget->next, sCurrTargetPos++)
                {
                    // PROJ-2002 Column Security
                    // target절에 보안 컬럼이 있는 경우 decrypt함수가 생성될 수 있다.
                    if ( sTarget->targetColumn->node.module == &mtfDecrypt )
                    {
                        sTargetNode1 = (qtcNode *)
                            sTarget->targetColumn->node.arguments;
                    }
                    else
                    {
                        sTargetNode1 = sTarget->targetColumn;
                    }

                    // To Fix PR-8615, PR-8820, PR-9143
                    // GROUP BY와 함께 사용된 Target은
                    // estimation 과정에서 Pass Node로 대체될 수 있다.
                    // 이 때, Target의 Naming에 따라
                    // ORDER BY의 ID는 다음과 같은 다양한 형태로
                    // 결정되어 있을 수 있다.
                    // Ex) ID가 같으나 Pass Node인 경우
                    //     SELECT T1.i1 A FROM T1, T2 GROUP BY i1 ORDER BY i1;
                    // Ex) ID가 다르고 Pass Node인 경우
                    //     SELECT i1 FROM T1 GROUP BY i1 ORDER BY T1.i1;
                    // 즉, 위의 두 경우 모두 암시적 Indicator로
                    // Target Position을 결정할 수 있다.
                    // 따라서, Target 자체의 ID 및 Pass Node일 경우
                    // Argument의 ID와 동일한 ORDER BY ID라면,
                    // 암시적 Indicator로 대체할 수 있다.

                    if ( sTargetNode1->node.module == &qtc::passModule )
                    {
                        sTargetNode2 = (qtcNode *)
                            sTargetNode1->node.arguments;
                    }
                    else
                    {
                        sTargetNode2 = sTargetNode1;
                    }

                    /* BUG-32102
                     * target 과 orderby 절을 잘못 비교하여 결과가 달라짐
                     */
                    IDE_TEST(qtc::isEquivalentExpression( aStatement,
                                                          sTargetNode2,
                                                          sCurrSort->sortColumn,
                                                          &sIsTrue)
                             != IDE_SUCCESS);

                    if ( sIsTrue == ID_TRUE )
                    {
                        sCurrSort->targetPosition = sCurrTargetPos;

                        IDE_TEST(transposePosValIntoTargetPtr( sCurrSort,
                                                               sSFWGH->target )
                                 != IDE_SUCCESS);

                        break;
                    }
                }
            }
        }
        else
        {
            // 명시적 Indicator임.
            IDE_TEST(transposePosValIntoTargetPtr( sCurrSort,
                                                   sSFWGH->target )
                     != IDE_SUCCESS);
            
            // PROJ-1413
            // view 컬럼 참조 노드를 등록한다.
            IDE_TEST( qmvQTC::addViewColumnRefList( aStatement,
                                                    sCurrSort->sortColumn )
                      != IDE_SUCCESS );
        }

        if ( sCurrSort->targetPosition > QMV_EMPTY_TARGET_POSITION )
        {
            //-----------------------------------------------
            // 3. Indicator에 대한 Validation
            //-----------------------------------------------

            // Indicator가 존재하는 경우
            if ( sCurrSort->targetPosition > sTargetMaxPos)
            {
                IDE_RAISE( ERR_INVALID_ORDERBY_POS );
            }

            // To Fix PR-8115
            // TARGET POSITION이 있는 ORDER BY는
            // Order By Indicator 최적화가 적용된다.
            // 아래와 같은 질의의 경우 Target Position이 없는 경우라 하더라도,
            // 동일한 Target이 있는 경우 Target Position을 생성하게 되는데,
            // Ex) SELECT i1, MAX(i2) FROM T1 GROUP BY i1 ORDER BY i1;
            //  --> SELECT i1, MAX(i2) FROM T1 GROUP BY i1 ORDER BY 1;
            //                                                     ^^
            // Order By Indicator 최적화와 Group By Expression의 Pass Node
            // 사용 기법은 동시에 사용될 수 없다.
            // 단, Target에 존재하는 Order By 컬럼일 경우
            // Target과 Group By 의 검사에 의하여 Order By에는
            // Group By 컬럼 이외(Aggregation제외)의 컬럼을 가질 수 없다는
            // 제약 조건을 만족함을 보장할 수 있다.

            // Nothing To Do
        }
        else
        {
            // Indicator가 존재하지 않는 경우

            // Nothing To Do
        }
    }

    //--------------------------------------
    // 4. check group expression
    //--------------------------------------
    
    if (sSFWGH->aggsDepth2 != NULL)
    {
        // order by절에서 추가된 aggsDepth2는 검사해야한다.
        for ( sCurrSort = sParseTree->orderBy;
              sCurrSort != NULL;
              sCurrSort = sCurrSort->next)
        {
            if ( sCurrSort->targetPosition < QMV_EMPTY_TARGET_POSITION )
            {
                if ( QTC_HAVE_AGGREGATE2( sCurrSort->sortColumn ) == ID_TRUE )
                {
                    IDE_TEST( qmvQTC::haveNotNestedAggregate( aStatement, 
                                                              sSFWGH, 
                                                              sCurrSort->sortColumn )
                              != IDE_SUCCESS);
                }
                else
                {
                    /* Nothing to do */
                }
            }
            else
            {
                /* Nothing to do */
            }
        }
        
        /* BUG-39332 group by가 있고 aggsDepth2가 있는 경우는 결과가 1건이므로
         * order by를 제거한다.
         * 그리고 order by절에서 추가된 aggregation은 제거한다.
         */
        sParseTree->orderBy = NULL;
        
        sSFWGH->aggsDepth1 = sAggsDepth1;
        sSFWGH->aggsDepth2 = sAggsDepth2;
    }
    else
    {
        // GROUP BY Expression을 포함하는 Expression인지를 검사.
        for ( sCurrSort = sParseTree->orderBy;
              sCurrSort != NULL;
              sCurrSort = sCurrSort->next )
        {
            if ( sCurrSort->targetPosition < QMV_EMPTY_TARGET_POSITION )
            {
                IDE_TEST( qmvQTC::isGroupExpression( aStatement,
                                                     sSFWGH,
                                                     sCurrSort->sortColumn,
                                                     ID_TRUE ) // make pass node
                          != IDE_SUCCESS );

                // BUG-48128 order by절은 제외
                sSFWGH->thisQuerySet->lflag &= ~QMV_QUERYSET_SCALAR_SUBQ_OUTER_COL_MASK;
                sSFWGH->thisQuerySet->lflag |= QMV_QUERYSET_SCALAR_SUBQ_OUTER_COL_FALSE;
            }
            else
            {
                /* Nothing to do */
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_ORDERBY_POS)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_INVALID_POSITION_IN_ORDERBY));
    }
    IDE_EXCEPTION(ERR_USE_CURSOR_ATTR);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_USING_ATTR_IN_INAPPROPRIATE_CLAUSE,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvOrderBy::validateSortWithoutGroup(qcStatement * aStatement)
{

    qmsParseTree    * sParseTree;
    qmsSortColumns  * sCurrSort;
    qmsSFWGH        * sSFWGH;
    qmsTarget       * sTarget;
    qtcNode         * sTargetNode;
    SInt              sTargetMaxPos = 0;
    SInt              sCurrTargetPos;
    qmsAggNode      * sAggsDepth1;
    qcuSqlSourceInfo  sqlInfo;
    idBool            sIsTrue;

    IDU_FIT_POINT_FATAL( "qmvOrderBy::validateSortWithoutGroup::__FT__" );

    sParseTree = (qmsParseTree*) aStatement->myPlan->parseTree;
    sSFWGH  = sParseTree->querySet->SFWGH;
    sAggsDepth1 = sSFWGH->aggsDepth1;
    
    for (sTarget = sSFWGH->target;
         sTarget != NULL;
         sTarget = sTarget->next)
    {
        sTargetMaxPos++;
    }

    for (sCurrSort = sParseTree->orderBy;
         sCurrSort != NULL;
         sCurrSort = sCurrSort->next)
    {
        IDE_TEST(qtc::getSortColumnPosition(sCurrSort, QC_SHARED_TMPLATE(aStatement))
                 != IDE_SUCCESS);
        
        if (sCurrSort->targetPosition < QMV_EMPTY_TARGET_POSITION)
        {
            /* PROJ-2197 PSM Renewal */
            sCurrSort->sortColumn->lflag |= QTC_NODE_COLUMN_CONVERT_TRUE;
            IDE_TEST(qtc::estimate(
                         sCurrSort->sortColumn,
                         QC_SHARED_TMPLATE(aStatement),
                         aStatement,
                         sParseTree->querySet,
                         sSFWGH,
                         NULL )
                     != IDE_SUCCESS);

            if ( ( sCurrSort->sortColumn->node.lflag & MTC_NODE_DML_MASK )
                 == MTC_NODE_DML_UNUSABLE )
            {
                sqlInfo.setSourceInfo( aStatement,
                                       & sCurrSort->sortColumn->position );
                IDE_RAISE( ERR_USE_CURSOR_ATTR );
            }

            // If column name is alias name,
            // then get position of target
            for (sTarget = sSFWGH->target, sCurrTargetPos = 1;
                 sTarget != NULL;
                 sTarget = sTarget->next, sCurrTargetPos++)
            {
                // PROJ-2002 Column Security
                // target절에 보안 컬럼이 있는 경우 decrypt함수가 생성될 수 있다.
                if ( sTarget->targetColumn->node.module == &mtfDecrypt )
                {
                    sTargetNode = (qtcNode *)
                        sTarget->targetColumn->node.arguments;
                }
                else
                {
                    sTargetNode = sTarget->targetColumn;
                }

                IDE_TEST( qtc::isEquivalentExpression( aStatement,
                                                       sTargetNode,
                                                       sCurrSort->sortColumn,
                                                       &sIsTrue )
                          != IDE_SUCCESS );

                if( sIsTrue == ID_TRUE )
                {
                    sCurrSort->targetPosition = sCurrTargetPos;

                    IDE_TEST(transposePosValIntoTargetPtr( sCurrSort,
                                                           sSFWGH->target )
                             != IDE_SUCCESS);

                    break;
                }
            }
        }
        else if (sCurrSort->targetPosition > QMV_EMPTY_TARGET_POSITION &&
                 sCurrSort->targetPosition <= sTargetMaxPos)
        {
            IDE_TEST(transposePosValIntoTargetPtr( sCurrSort,
                                                   sSFWGH->target )
                     != IDE_SUCCESS);
            
            // PROJ-1413
            // view 컬럼 참조 노드를 등록한다.
            IDE_TEST( qmvQTC::addViewColumnRefList( aStatement,
                                                    sCurrSort->sortColumn )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_RAISE( ERR_INVALID_ORDERBY_POS );
        }
    }

    if (sSFWGH->aggsDepth1 == NULL)
    {
        for (sCurrSort = sParseTree->orderBy;
             sCurrSort != NULL;
             sCurrSort = sCurrSort->next)
        {
            if (sCurrSort->targetPosition < QMV_EMPTY_TARGET_POSITION)
            {
                // BUG-27597
                // order by절에서 analytic func이 아닌 aggregation인 경우 에러
                // aggregation의 argument로 analytic func은 이미 걸러졌음
                if ( ( sCurrSort->sortColumn->lflag & QTC_NODE_ANAL_FUNC_MASK )
                     == QTC_NODE_ANAL_FUNC_ABSENT )
                {
                    IDE_TEST_RAISE(QTC_HAVE_AGGREGATE(sCurrSort->sortColumn)
                                   == ID_TRUE, ERR_CANNOT_HAVE_AGGREGATE);
                }
                else
                {
                    // nothing to do 
                }
            }
        }
    }
    else // if (sSFWGH->aggsDepth1 != NULL)
    {
        if (sSFWGH->aggsDepth2 == NULL)
        {
            // If aggregation exists only in order by clause,
            // select clause is not checked in previous validation function.
            // If aggregation exists in SFWGH,
            // the following check of select clause is double check.
            for (sTarget = sSFWGH->target;
                 sTarget != NULL;
                 sTarget = sTarget->next)
            {
                IDE_TEST(qmvQTC::isAggregateExpression(
                             aStatement, sSFWGH, sTarget->targetColumn)
                         != IDE_SUCCESS);
            }

            // order by절에서 추가된 aggsDepth1는 검사해야한다.
            for ( sCurrSort = sParseTree->orderBy;
                  sCurrSort != NULL;
                  sCurrSort = sCurrSort->next)
            {
                if ( sCurrSort->targetPosition < QMV_EMPTY_TARGET_POSITION )
                {
                    if ( QTC_HAVE_AGGREGATE( sCurrSort->sortColumn ) == ID_TRUE )
                    {
                        IDE_TEST( qmvQTC::isAggregateExpression( aStatement, 
                                                                 sSFWGH, 
                                                                 sCurrSort->sortColumn )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                }
                else
                {
                    /* Nothing to do */
                }
            }
            
            /* BUG-39332 group by가 없고 aggsDepth1이 있는 경우는 결과가 1건이므로
             * order by를 제거한다.
             * 그리고 order by절에서 추가된 aggregation은 제거한다.
             */
            sParseTree->orderBy = NULL;
            
            sSFWGH->aggsDepth1 = sAggsDepth1;
        }
        else // if (sSFWGH->aggsDepth2 != NULL)
        {
            IDE_RAISE( ERR_NESTED_AGGR_WITHOUT_GROUP );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_CANNOT_HAVE_AGGREGATE)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_CANNOT_USE_AGG));
    }
    IDE_EXCEPTION(ERR_INVALID_ORDERBY_POS)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_INVALID_POSITION_IN_ORDERBY));
    }
    IDE_EXCEPTION(ERR_NESTED_AGGR_WITHOUT_GROUP)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_NESTED_AGGR_WITHOUT_GROUP));
    }
    IDE_EXCEPTION(ERR_USE_CURSOR_ATTR);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_USING_ATTR_IN_INAPPROPRIATE_CLAUSE,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvOrderBy::validateSortWithSet(qcStatement * aStatement)
{

    qmsParseTree    * sParseTree;
    qmsSortColumns  * sCurrSort;
    qmsTarget       * sTarget;
    qtcNode         * sTargetNode;
    SInt              sTargetMaxPos = 0;
    SInt              sCurrTargetPos;
    qcuSqlSourceInfo  sqlInfo;

    IDU_FIT_POINT_FATAL( "qmvOrderBy::validateSortWithSet::__FT__" );

    sParseTree = (qmsParseTree*) aStatement->myPlan->parseTree;

    for (sTarget = sParseTree->querySet->target;
         sTarget != NULL;
         sTarget = sTarget->next)
    {
        sTargetMaxPos++;
    }

    for (sCurrSort = sParseTree->orderBy;
         sCurrSort != NULL;
         sCurrSort = sCurrSort->next)
    {
        IDE_TEST(qtc::getSortColumnPosition(sCurrSort, QC_SHARED_TMPLATE(aStatement))
                 != IDE_SUCCESS);

        if (sCurrSort->targetPosition < QMV_EMPTY_TARGET_POSITION)
        {
            // The possible format is only "ORDER BY col"
            // ORDER BY tbl.col => error
            // alias name -> search target -> set sortColumn
            // ERR_INVALID_ORDERBY_EXP_WITH_SET
            /* PROJ-2197 PSM Renewal */
            sCurrSort->sortColumn->lflag |= QTC_NODE_COLUMN_CONVERT_TRUE;
            IDE_TEST(qtc::estimate(
                         sCurrSort->sortColumn,
                         QC_SHARED_TMPLATE(aStatement),
                         aStatement,
                         sParseTree->querySet,
                         NULL,
                         NULL )
                     != IDE_SUCCESS);

            if ( ( sCurrSort->sortColumn->node.lflag & MTC_NODE_DML_MASK )
                 == MTC_NODE_DML_UNUSABLE )
            {
                sqlInfo.setSourceInfo( aStatement,
                                       & sCurrSort->sortColumn->position );
                IDE_RAISE( ERR_USE_CURSOR_ATTR );
            }

            // To Fix PR-9032
            // SET을 포함할 경우 ORDER BY에 PRIOR 를 사용할 수 없다.
            if ( (sCurrSort->sortColumn->lflag & QTC_NODE_PRIOR_MASK)
                 == QTC_NODE_PRIOR_EXIST )
            {
                IDE_RAISE( ERR_INVALID_ORDERBY_POS );
            }
            else
            {
                // go go
            }

            // get position of target
            for (sTarget = sParseTree->querySet->target, sCurrTargetPos = 1;
                 sTarget != NULL;
                 sTarget = sTarget->next, sCurrTargetPos++)
            {
                // PROJ-2002 Column Security
                // target절에 보안 컬럼이 있는 경우 decrypt함수가 생성될 수 있다.
                if ( sTarget->targetColumn->node.module == &mtfDecrypt )
                {
                    sTargetNode = (qtcNode *)
                        sTarget->targetColumn->node.arguments;
                }
                else
                {
                    sTargetNode = sTarget->targetColumn;
                }

                if (sTargetNode->node.table ==
                    sCurrSort->sortColumn->node.table &&
                    sTargetNode->node.column ==
                    sCurrSort->sortColumn->node.column)
                {
                    sCurrSort->targetPosition = sCurrTargetPos;
                    break;
                }
            }

            if (sTarget == NULL)
            {
                if ( sCurrSort->targetPosition < 0 )
                {
                    // BUG-21807 
                    // position 정보가 주어지지 않은 경우,
                    // order by 칼럼을 target list에서 찾지 못했음을
                    // 에러 메시지로 알려주어야 함
                    IDE_RAISE( ERR_NOT_EXIST_SELECT_LIST );
                }
                else
                {
                    // position 정보가 주어진 경우,
                    // position 정보가 잘못 되었음을
                    // 에러 메시지로 알려주어야 함
                    IDE_RAISE( ERR_INVALID_ORDERBY_POS );
                }
            }
        }
        else if (sCurrSort->targetPosition > QMV_EMPTY_TARGET_POSITION &&
                 sCurrSort->targetPosition <= sTargetMaxPos)
        {
            IDE_TEST(transposePosValIntoTargetPtr(
                         sCurrSort,
                         sParseTree->querySet->target )
                     != IDE_SUCCESS);
        }
        else
        {
            IDE_RAISE( ERR_INVALID_ORDERBY_POS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXIST_SELECT_LIST)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_NO_SELECTED_EXPRESSION));
    }
    IDE_EXCEPTION(ERR_INVALID_ORDERBY_POS)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_INVALID_POSITION_IN_ORDERBY));
    }
    IDE_EXCEPTION(ERR_USE_CURSOR_ATTR);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_USING_ATTR_IN_INAPPROPRIATE_CLAUSE,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvOrderBy::transposePosValIntoTargetPtr(
    qmsSortColumns  * aSortColumn,
    qmsTarget       * aTarget )
{

    qmsTarget   * sTarget;
    qtcNode     * sTargetNode;
    SInt          sPosition = 0;
    qcNamePosition sOrgPosition; /* TASK-7219 */

    IDU_FIT_POINT_FATAL( "qmvOrderBy::transposePosValIntoTargetPtr::__FT__" );

    for (sTarget = aTarget; sTarget != NULL; sTarget = sTarget->next)
    {
        sPosition++;

        if (aSortColumn->targetPosition == sPosition)
        {
            break;
        }
    }

    IDE_TEST_RAISE(aSortColumn->targetPosition != sPosition || sTarget == NULL,
                   ERR_ORDERBY_WITH_INVALID_TARGET_POS);

    // PROJ-2415 Grouping Sets Clause
    // Grouping Sets Transform에 의해 Target에 추가된 OrderBy의 Node를
    // Target에 추가되지 않은 OrderBy의 Position이 바라볼 수 없다.
    IDE_TEST_RAISE( ( ( sTarget->targetColumn->lflag & QTC_NODE_GBGS_ORDER_BY_NODE_MASK ) ==
                      QTC_NODE_GBGS_ORDER_BY_NODE_TRUE ) &&
                    ( ( aSortColumn->sortColumn->lflag & QTC_NODE_GBGS_ORDER_BY_NODE_MASK ) !=
                      QTC_NODE_GBGS_ORDER_BY_NODE_TRUE ),
                    ERR_ORDERBY_WITH_INVALID_TARGET_POS );

    // PROJ-2002 Column Security
    // target절에 보안 컬럼이 있는 경우 decrypt함수가 생성될 수 있다.
    if ( sTarget->targetColumn->node.module == &mtfDecrypt )
    {
        sTargetNode = (qtcNode *)
            sTarget->targetColumn->node.arguments;
    }
    else
    {
        sTargetNode = sTarget->targetColumn;

        // PROJ-2179 ORDER BY절에서 참조되었음을 표시
        sTarget->flag &= ~QMS_TARGET_ORDER_BY_MASK;
        sTarget->flag |= QMS_TARGET_ORDER_BY_TRUE;
    }

    /* TASK-7219 */
    SET_POSITION( sOrgPosition, aSortColumn->sortColumn->position );

    // set target expression
    idlOS::memcpy( aSortColumn->sortColumn,
                   sTargetNode,
                   ID_SIZEOF(qtcNode) );

    /* TASK-7219 */
    SET_POSITION( aSortColumn->sortColumn->position, sOrgPosition );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION(ERR_ORDERBY_WITH_INVALID_TARGET_POS)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QMV_INVALID_POSITION_IN_ORDERBY));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvOrderBy::disconnectConstantNode(
    qmsParseTree    * aParseTree )
{

    qmsSortColumns  * sPrevSort = NULL;
    qmsSortColumns  * sCurrSort;

    IDU_FIT_POINT_FATAL( "qmvOrderBy::disconnectConstantNode::__FT__" );

    // disconnect constant
    for (sCurrSort = aParseTree->orderBy;
         sCurrSort != NULL;
         sCurrSort = sCurrSort->next)
    {
        IDE_FT_ASSERT( sCurrSort->sortColumn != NULL );
        
        // PROJ-1413
        // constant를 검사하는 방법 변경
        if ( qtc::isConstNode4OrderBy( sCurrSort->sortColumn ) == ID_TRUE )
        {
            if (sPrevSort == NULL)
            {
                aParseTree->orderBy = sCurrSort->next;
            }
            else
            {
                sPrevSort->next = sCurrSort->next;
            }
        }
        else
        {
            sPrevSort = sCurrSort;
        }
    }

    return IDE_SUCCESS;
}
