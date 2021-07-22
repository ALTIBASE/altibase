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
 * $Id: qmvAvgTransform.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 * PROJ-2361
 ***********************************************************************/

#include <qc.h>
#include <qtc.h>
#include <qcuProperty.h>
#include <qcgPlan.h>
#include <qmvAvgTransform.h>
#include <mtcDef.h>

extern mtfModule mtfAvg;
extern mtfModule mtfCount;
extern mtfModule mtfDivide;
extern mtfModule mtfSum;

IDE_RC qmvAvgTransform::doTransform( qcStatement * aStatement,
                                     qmsQuerySet * aQuerySet,
                                     qmsSFWGH    * aSFWGH )
{
    qmsTarget * sTarget;
    idBool      sDummyFlag = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmvAvgTransform::doTransform::__FT__" );

    qcgPlan::registerPlanProperty( aStatement,
                                   PLAN_PROPERTY_AVERAGE_TRANSFORM_ENABLE );

    if ( QCU_AVERAGE_TRANSFORM_ENABLE == 1 )
    {
        for ( sTarget =  aSFWGH->target;
              sTarget != NULL;
              sTarget =  sTarget->next )
        {
            IDE_TEST( transformAvg2SumDivCount( aStatement,
                                                aQuerySet,
                                                sTarget->targetColumn,
                                                &sTarget->targetColumn,
                                                &sDummyFlag )
                      != IDE_SUCCESS );
        }

        if ( aSFWGH->having != NULL )
        {
            IDE_TEST( transformAvg2SumDivCount( aStatement,
                                                aQuerySet,
                                                aSFWGH->having,
                                                &aSFWGH->having,
                                                &sDummyFlag )
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
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvAvgTransform::transformAvg2SumDivCount( qcStatement  * aStatement,
                                              	  qmsQuerySet  * aQuerySet,
                                              	  qtcNode      * aNode,
                                              	  qtcNode     ** aResultNode,
                                              	  idBool       * aNeedEstimate )
{
/***********************************************************************
 *
 * Description :
 *     AVG()함수를 SUM() / COUNT() 노드로 변경한다.
 *
 * Implementation :
 *     AVG()함수이면 AVG()함수의 아규먼트와 같은 SUM() 함수가 SELECT target,
 *     및 having절에 존재 하는지 검사하여 존재 하면 AVG() 를 SUM() / COUNT()
 *     노드로 변경한다.
 *     기존에 estimate된 AVG()노드를 제거한다.
 *     AVG()가 다른 함수에 아규먼트로 사용하였을 경우 노드의 아규먼트로 재귀 호출 한다.
 *
 ***********************************************************************/

    qmsSFWGH   * sSFWGH;
    qtcNode    * sAvgArg1;
    qtcNode    * sAvgArg2;
    qtcNode    * sDivNode[2];
    qtcNode    * sSumNode[2];
    qtcNode    * sCountNode[2];
    idBool       sCheckTransform = ID_FALSE;
    qmsAggNode * sAggrNode;
    qmsAggNode * sAggrPrevNode = NULL;
    qtcNode    * sNode;
    idBool       sNeedEstimate = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmvAvgTransform::transformAvg2SumDivCount::__FT__" );

    sNode = aNode;

    sSFWGH = aQuerySet->SFWGH;

    if ( ( sNode->lflag & QTC_NODE_AGGREGATE_MASK ) == QTC_NODE_AGGREGATE_EXIST )
    {
        if ( ( ( sNode->node.lflag & MTC_NODE_OPERATOR_MASK )
               == MTC_NODE_OPERATOR_AGGREGATION ) &&
             ( sNode->node.module == &mtfAvg ) )
        {
            IDE_TEST( checkTransform( aStatement,
                                      sSFWGH,
                                      sNode,
                                      &sCheckTransform )
                      != IDE_SUCCESS );

            if ( sCheckTransform == ID_TRUE )
            {
                /* AVG()의 아규먼트 노드를 복사한다 */
                IDE_TEST( qtc::cloneQTCNodeTree( QC_QMP_MEM(aStatement),
                                                 (qtcNode *)sNode->node.arguments,
                                                 &sAvgArg1,
                                                 ID_TRUE,   // root의 next는 복사 한다.
                                                 ID_TRUE,   // conversion을 끊는다.
                                                 ID_TRUE,   // constant node까지 복사한다.
                                                 ID_FALSE ) // constant node를 원복하지 않는다.
                          != IDE_SUCCESS );

                IDE_TEST( qtc::cloneQTCNodeTree( QC_QMP_MEM(aStatement),
                                                 (qtcNode *)sNode->node.arguments,
                                                 &sAvgArg2,
                                                 ID_TRUE,   // root의 next는 복사 한다.
                                                 ID_TRUE,   // conversion을 끊는다.
                                                 ID_TRUE,   // constant node까지 복사한다.
                                                 ID_FALSE ) // constant node를 원복하지 않는다.
                          != IDE_SUCCESS );

                /* count 함수를 만든다. */
                IDE_TEST( qtc::makeNode( aStatement,
                                         sCountNode,
                                         &sNode->position,
                                         &mtfCount )
                          != IDE_SUCCESS );

                /* 함수를 연결한다. */
                sCountNode[0]->node.arguments = (mtcNode *)sAvgArg1;
                sCountNode[0]->node.arguments->next = NULL;
                sCountNode[0]->node.next = NULL;

                sCountNode[0]->node.lflag &= ~MTC_NODE_ARGUMENT_COUNT_MASK;
                sCountNode[0]->node.lflag |= 1;

                /* sum 함수를 만든다. */
                IDE_TEST( qtc::makeNode( aStatement,
                                         sSumNode,
                                         &sNode->position,
                                         &mtfSum )
                          != IDE_SUCCESS );

                /* 함수를 연결한다. */
                sSumNode[0]->node.arguments = (mtcNode *)sAvgArg2;
                sSumNode[0]->node.arguments->next = NULL;
                sSumNode[0]->node.next = NULL;

                sSumNode[0]->node.lflag &= ~MTC_NODE_ARGUMENT_COUNT_MASK;
                sSumNode[0]->node.lflag |= 1;

                /* divide 함수를 만든다. */
                IDE_TEST( qtc::makeNode( aStatement,
                                         sDivNode,
                                         &sNode->position,
                                         &mtfDivide )
                          != IDE_SUCCESS );

                /* 함수를 연결한다. */
                sDivNode[0]->node.arguments = (mtcNode *)sSumNode[0];
                sSumNode[0]->node.next = (mtcNode *)sCountNode[0];
                sCountNode[0]->node.next = NULL;
                sDivNode[0]->node.next = sNode->node.next;

                sDivNode[0]->node.lflag &= ~MTC_NODE_ARGUMENT_COUNT_MASK;
                sDivNode[0]->node.lflag |= 2;

                IDE_TEST( qtc::estimate( sDivNode[0],
                                         QC_SHARED_TMPLATE(aStatement),
                                         aStatement,
                                         aQuerySet,
                                         aQuerySet->SFWGH,
                                         NULL )
                          != IDE_SUCCESS );

                /* 이미 estimate된 aggr 노드를 제거 한다. */
                for ( sAggrNode =  sSFWGH->aggsDepth1;
                      sAggrNode != NULL;
                      sAggrNode =  sAggrNode->next )
                {
                    if ( sAggrNode->aggr == sNode )
                    {
                        if (sAggrPrevNode != NULL )
                        {
                            sAggrPrevNode->next = sAggrNode->next;
                        }
                        else
                        {
                            sSFWGH->aggsDepth1 = sAggrNode->next;
                        }
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                    sAggrPrevNode = sAggrNode;
                }

                sNode = sDivNode[0];
                *aNeedEstimate = ID_TRUE;
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

    /* Arguments를 처리한다. */
    if ( sNode->node.arguments != NULL )
    {
        IDE_TEST( transformAvg2SumDivCount( aStatement,
                                            aQuerySet,
                                            (qtcNode *)sNode->node.arguments,
                                            (qtcNode **)&(sNode->node.arguments),
                                            &sNeedEstimate )
                  != IDE_SUCCESS);

        if ( sNeedEstimate == ID_TRUE )
        {
            IDE_TEST( qtc::estimateNodeWithArgument( aStatement,
                                                     sNode )
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

    /* Next를 처리한다. */
    if ( sNode->node.next != NULL )
    {
        IDE_TEST( transformAvg2SumDivCount( aStatement,
                                            aQuerySet,
                                            (qtcNode *)sNode->node.next,
                                            (qtcNode **)&(sNode->node.next),
                                            aNeedEstimate )
                  != IDE_SUCCESS);
    }
    else
    {
        /* Nothing to do */
    }

    *aResultNode = sNode;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvAvgTransform::checkTransform( qcStatement * aStatement,
                                    	qmsSFWGH    * aSFWGH,
                                    	qtcNode     * aAvgNode,
                                    	idBool      * aCheckTransform )
{
/***********************************************************************
 *
 * Description :
 *     avg()의 아규먼트와 같은 아규먼트를 가지는 sum()함수가 있는지 찾는다.
 *
 * Implementation :
 *     DISTINCT 변환 하지 않는다.
 *     PSM function 변환 하지 않는다.
 *     variable built in function은 변환 하지 않는다.
 *     subquery 가 포함된 경우 변환 하지 않는다.
 *     view column인 경우 제외 한다.
 *
 ***********************************************************************/

    qmsAggNode  * sAggrNode;
    idBool        sExistSameArgSum = ID_FALSE;

    idBool        sExistViewColumn = ID_FALSE;
    
    IDU_FIT_POINT_FATAL( "qmvAvgTransform::checkTransform::__FT__" );

    /* DISTINCT 변환 하지 않는다. */
    if ( ( aAvgNode->node.lflag & MTC_NODE_DISTINCT_MASK )
         == MTC_NODE_DISTINCT_TRUE )
    {
        IDE_CONT( NORMAL_EXIT );
    }
    else
    {
        /* Nothing to do */
    }

    /* PSM function은 변환 하지 않는다. */
    if ( ( aAvgNode->lflag & QTC_NODE_PROC_FUNCTION_MASK )
         == QTC_NODE_PROC_FUNCTION_TRUE )
    {
        IDE_CONT( NORMAL_EXIT );
    }
    else
    {
        /* Nothing to do */
    }

    /* variable build-in function은 변환 하지 않는다. */
    if ( ( aAvgNode->lflag & QTC_NODE_VAR_FUNCTION_MASK )
         == QTC_NODE_VAR_FUNCTION_EXIST )
    {
        IDE_CONT( NORMAL_EXIT );
    }
    else
    {
        /* Nothing to do */
    }

    /* subquery를 포함인 경우 변환 하지 않는다. */
    if ( ( aAvgNode->lflag & QTC_NODE_SUBQUERY_MASK )
         == QTC_NODE_SUBQUERY_EXIST )
    {
        IDE_CONT( NORMAL_EXIT );
    }
    else
    {
        /* Nothing to do */
    }

    /* view column인 경우 제외 한다.
     * select sum(a), avg(a) from
     * (
     *     select 1 a from dual
     * );
     */
    IDE_TEST( isExistViewColumn( aStatement,
                                 aAvgNode,
                                 &sExistViewColumn )
              != IDE_SUCCESS );

    if ( sExistViewColumn == ID_TRUE )
    {
        IDE_CONT( NORMAL_EXIT );
    }
    else
    {
        /* Nothing to do */
    }

    for ( sAggrNode =  aSFWGH->aggsDepth1;
          sAggrNode != NULL;
          sAggrNode =  sAggrNode->next )
    {
        IDE_TEST( isExistSameArgSum( aStatement,
                                     sAggrNode->aggr,
                                     aAvgNode,
                                     &sExistSameArgSum )
                  != IDE_SUCCESS );

        if ( sExistSameArgSum == ID_TRUE )
        {
            break;
        }
        else
        {
            /* Nothing to do */
        }
    }

    IDE_EXCEPTION_CONT( NORMAL_EXIT );

    *aCheckTransform = sExistSameArgSum;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmvAvgTransform::isExistViewColumn( qcStatement * aStatement,
                                           qtcNode     * aAvgNode,
                                           idBool      * aIsExistViewRefColumn )
{
    qmsTableRef * sTableRef;
    qmsFrom     * sFrom;
    SInt          i;
    idBool        sExistViewColumn = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmvAvgTransform::isExistViewColumn::__FT__" );

    for ( i = 0; i < qtc::getCountBitSet( &aAvgNode->depInfo ); i++ )
    {
        sFrom = QC_SHARED_TMPLATE(aStatement)->tableMap[qtc::getDependTable( &aAvgNode->depInfo, i)].from;

        if ( sFrom != NULL )
        {
            sTableRef = sFrom->tableRef;

            if ( sTableRef->view != NULL )
            {
                sExistViewColumn = ID_TRUE;
                break;
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

    *aIsExistViewRefColumn = sExistViewColumn;

    return IDE_SUCCESS;
    
}

IDE_RC qmvAvgTransform::isExistSameArgSum( qcStatement * aStatement,
                                       	   qtcNode     * aNode,
                                       	   qtcNode     * aAvgNode,
                                       	   idBool      * aIsExist )
{
/***********************************************************************
 *
 * Description :
 *     SUM() 함수 이면서 aArg와 같은 아규먼트를 사용 하는지 검사 한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    idBool      sIsSame = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmvAvgTransform::isExistSameArgSum::__FT__" );
    
    if ( aNode->node.module == & mtfSum )
    {
        /* SUM의  DISTINCT MASK가 없어야 한다. */
        if ( ( aNode->node.lflag & MTC_NODE_DISTINCT_MASK )
             == MTC_NODE_DISTINCT_FALSE )
        {
            IDE_TEST( qtc::isEquivalentExpression( aStatement,
                                                   (qtcNode *)aNode->node.arguments,
                                                   (qtcNode *)aAvgNode->node.arguments,
                                                   &sIsSame )
                      != IDE_SUCCESS )
                }
        else
        {
            sIsSame = ID_FALSE;
        }
    }
    else
    {
        /* Nothing to do */
    }

    *aIsExist = sIsSame;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
