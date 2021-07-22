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
 * $Id: qmoRownumPredToLimit.cpp 88025 2020-07-13 05:35:50Z andrew.shin $
 *
 * Description : BUG-40355
 *     Rownum To Limit
 * rownum 을 사용한 질의를 limit 를 이용할도록 변경한다.
 * select * from ( select rownum as r1 from t1 ) where r1 <= 50;
 *   -> select * from ( select rownum as r1 from t1 limit 50 );
 *
 *
 * Implementation :
 *
 **********************************************************************/

#include <qcgPlan.h>
#include <qmoRownumPredToLimit.h>

extern mtfModule mtfEqual;
extern mtfModule mtfLessEqual;
extern mtfModule mtfLessThan;
extern mtfModule mtfGreaterEqual;
extern mtfModule mtfGreaterThan;
extern mtfModule mtfBetween;

IDE_RC qmoRownumPredToLimit::rownumPredToLimitTransform( qcStatement   * aStatement,
                                                         qmsQuerySet   * aQuerySet,
                                                         qmsTableRef   * aViewTableRef,
                                                         qmoPredicate ** aPredicate )
{
    idBool sCanTransform = ID_FALSE;
    UShort sPredPosition = 0;

    IDU_FIT_POINT_FATAL( "qmoRownumPredToLimit::rownumPredToLimitTransform::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_FT_ERROR( aStatement     != NULL );
    IDE_FT_ERROR( aQuerySet      != NULL );
    IDE_FT_ERROR( aViewTableRef  != NULL );
    IDE_FT_ERROR( aPredicate     != NULL );

    //--------------------------------------
    // BUG-41182 view Rownum To Limit
    //--------------------------------------

    sCanTransform = isViewRownumToLimit( aStatement,
                                         aQuerySet,
                                         aViewTableRef );

    if ( sCanTransform == ID_TRUE )
    {
        IDE_TEST ( doViewRownumToLimit( aStatement,
                                        aQuerySet,
                                        aViewTableRef )
                   != IDE_SUCCESS );
    }
    else
    {
        // nothing to do.
    }

    //--------------------------------------
    // 조건 검사
    //--------------------------------------

    sCanTransform = isRownumPredToLimit( aStatement,
                                         aViewTableRef,
                                         *aPredicate,
                                         &sPredPosition );

    //--------------------------------------
    // 트랜스폼 수행
    //--------------------------------------

    if ( sCanTransform == ID_TRUE )
    {
        IDE_TEST ( doRownumPredToLimit( aStatement,
                                        aViewTableRef,
                                        aPredicate,
                                        sPredPosition )
                   != IDE_SUCCESS );
    }
    else
    {
        // nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoRownumPredToLimit::makeLimit( qcStatement  * aStatement,
                                        qmsTableRef  * aViewTableRef,
                                        qtcNode      * aNode,
                                        idBool       * aChanged )
{
    mtcNode      * sNode = (mtcNode*)aNode;
    qmsParseTree * sViewParseTree;
    qtcNode      * sValueNode;
    mtdBigintType  sStartValue;
    mtdBigintType  sCountValue;

    /* TASK-7219 */
    qtcNode      * sValueNode2 = NULL;

    IDE_FT_BEGIN();

    IDU_FIT_POINT_FATAL( "qmoRownumPredToLimit::makeLimit::__FT__" );

    sViewParseTree = (qmsParseTree*)(aViewTableRef->view->myPlan->parseTree);

    if ( sNode->module == &mtfEqual )
    {
        if ( sNode->arguments->module == &qtc::valueModule )
        {
            sValueNode  = (qtcNode*)(sNode->arguments);
        }
        else
        {
            sValueNode  = (qtcNode*)(sNode->arguments->next);
        }

        sStartValue = 1;
        sCountValue = *((mtdBigintType *)QTC_STMT_FIXEDDATA( aStatement, sValueNode ));
    }
    else if ( sNode->module == &mtfLessEqual )
    {
        sValueNode  = (qtcNode*)(sNode->arguments->next);

        sStartValue = 1;
        sCountValue = *((mtdBigintType *)QTC_STMT_FIXEDDATA( aStatement, sValueNode ));
    }
    else if ( sNode->module == &mtfLessThan )
    {
        sValueNode  = (qtcNode*)(sNode->arguments->next);

        sStartValue = 1;
        sCountValue = *((mtdBigintType *)QTC_STMT_FIXEDDATA( aStatement, sValueNode ));
        sCountValue = sCountValue-1;
    }
    else if ( sNode->module == &mtfGreaterEqual )
    {
        sValueNode  = (qtcNode*)(sNode->arguments);

        sStartValue = 1;
        sCountValue = *((mtdBigintType *)QTC_STMT_FIXEDDATA( aStatement, sValueNode ));
        sCountValue = sCountValue;
    }
    else if ( sNode->module == &mtfGreaterThan )
    {
        sValueNode  = (qtcNode*)(sNode->arguments);

        sStartValue = 1;
        sCountValue = *((mtdBigintType *)QTC_STMT_FIXEDDATA( aStatement, sValueNode ));
        sCountValue = sCountValue -1;
    }
    else if ( sNode->module == &mtfBetween )
    {
        sValueNode  = (qtcNode*)(sNode->arguments->next);
        sStartValue = *((mtdBigintType *)QTC_STMT_FIXEDDATA( aStatement, sValueNode ));

        /* TASK-7219 */
        sValueNode2 = sValueNode;

        sValueNode  = (qtcNode*)(sNode->arguments->next->next);
        sCountValue = *((mtdBigintType *)QTC_STMT_FIXEDDATA( aStatement, sValueNode ));

        sCountValue = sCountValue - sStartValue + 1;
    }
    else
    {
        IDE_FT_ERROR( 0 );
    }

    if ( (sStartValue > 0) && (sCountValue > 0) )
    {
        IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmsLimit ),
                                                 (void**)&( sViewParseTree->limit ) )
                     != IDE_SUCCESS );

        qmsLimitI::setStartValue( sViewParseTree->limit, sStartValue );
        qmsLimitI::setCountValue( sViewParseTree->limit, sCountValue );
        SET_EMPTY_POSITION( sViewParseTree->limit->limitPos );

        /* TASK-7219 */
        SET_POSITION( sViewParseTree->limit->count.mPosition, sValueNode->position );

        if ( sValueNode2 == NULL )
        {
            SET_EMPTY_POSITION( sViewParseTree->limit->start.mPosition );
        }
        else
        {
            SET_POSITION( sViewParseTree->limit->start.mPosition, sValueNode2->position );
        }

        *aChanged = ID_TRUE;
    }
    else
    {
        *aChanged = ID_FALSE;
    }

    IDE_FT_END();

    return IDE_SUCCESS;

    IDE_EXCEPTION_SIGNAL()
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_FAULT_TOLERATED ) );
    }
    IDE_EXCEPTION_END;

    IDE_FT_EXCEPTION_BEGIN();

    *aChanged = ID_FALSE;

    IDE_FT_EXCEPTION_END();

    return IDE_FAILURE;
}

idBool qmoRownumPredToLimit::isRownumPredToLimit( qcStatement  * aStatement,
                                                  qmsTableRef  * aViewTableRef,
                                                  qmoPredicate * aPredicate,
                                                  UShort       * aPredPosition )
{
    idBool         sTrans = ID_TRUE;
    qmsParseTree * sViewParseTree;
    qmsTarget    * sTarget;
    qmoPredicate * sPredicate;
    mtcNode      * sNode;
    mtcColumn    * sColumn;
    mtcColumn    * sColumn2;
    UShort         sViewTpuleID;
    UShort         sFindCount;
    UShort         sPosition;
    UShort         sRownumColumn    = 0;
    UShort         sPredPosition;

    sViewParseTree = (qmsParseTree*)(aViewTableRef->view->myPlan->parseTree);
    sViewTpuleID   = aViewTableRef->table;

    if ( sViewParseTree->querySet->setOp == QMS_NONE )
    {
        if ( sViewParseTree->orderBy != NULL )
        {
            // view 안에 order by 이 없어야 한다.
            sTrans = ID_FALSE;
        }
        else
        {
            // nothing to do.
        }

        if ( sViewParseTree->limit != NULL )
        {
            // view 안에 limit 이 없어야 한다.
            sTrans = ID_FALSE;
        }
        else
        {
            // nothing to do.
        }

        if ( sViewParseTree->querySet->SFWGH->rownum == NULL )
        {
            // view 안에 rownum 이 있어야 한다.
            sTrans = ID_FALSE;
        }
        else
        {
            // nothing to do.
        }
    }
    else
    {
        // view 안에 set절이 있을때는 limit을 사용 할 수 없다.
        sTrans = ID_FALSE;
    }

    if ( sTrans == ID_TRUE )
    {
        // rownum은 1개만 존재해야 한다.
        sFindCount = 0;

        for ( sTarget = sViewParseTree->querySet->target,
              sPosition = 0;
              sTarget != NULL;
              sTarget = sTarget->next,
              sPosition++ )
        {
            if( (sTarget->targetColumn->lflag & QTC_NODE_ROWNUM_MASK ) == QTC_NODE_ROWNUM_EXIST)
            {
                if ( sTarget->targetColumn->node.module == &qtc::columnModule )
                {
                    sFindCount++;
                    sRownumColumn = sPosition;
                }
                else
                {
                    // rownum에 연산이 있는 경우에는 하지 않는다.
                    sTrans = ID_FALSE;
                    break;
                }
            }
            else
            {
                // nothing to do.
            }
        }

        if ( sFindCount != 1 )
        {
            sTrans = ID_FALSE;
        }
        else
        {
            // nothing to do.
        }
    }
    else
    {
        // nothing to do.
    }

    if ( sTrans == ID_TRUE )
    {
        // where 절에서 1개만 참조해야 한다.
        sFindCount = 0;

        for ( sPredicate = aPredicate,
              sPosition = 0;
              sPredicate != NULL;
              sPredicate = sPredicate->next,
              sPosition++ )
        {
            sNode = (mtcNode*)(sPredicate->node);

            // 최상위 노드는 OR 노드여야 한다.
            if ( (sNode->lflag & MTC_NODE_OPERATOR_MASK) == MTC_NODE_OPERATOR_OR )
            {
                sNode = sNode->arguments;
            }
            else
            {
                continue;
            }

            // OR 연산은 지원하지 않는다.
            if ( sNode->next != NULL )
            {
                continue;
            }
            else
            {
                // nothing to do.
            }

            // bind 변수가 없어야 한다.
            if ( (sNode->lflag & MTC_NODE_BIND_MASK) == MTC_NODE_BIND_EXIST )
            {
                continue;
            }
            else
            {
                // nothing to do.
            }

            if ( ((qtcNode*)sNode)->depInfo.depCount != 1 )
            {
                continue;
            }
            else
            {
                // nothing to do.
            }

            if ( ((qtcNode*)sNode)->depInfo.depend[0] != sViewTpuleID )
            {
                continue;
            }
            else
            {
                // nothing to do.
            }

            // 다음의 형태만 지원한다.
            if ( (sNode->module == &mtfLessEqual) ||
                 (sNode->module == &mtfLessThan) )
            {
                // rownum <= 상수;
                // rownum < 상수;
                sColumn = QTC_STMT_COLUMN( aStatement, ((qtcNode*)(sNode->arguments->next)));

                if ( (sNode->arguments->module == &qtc::columnModule) &&
                     (sNode->arguments->table  == sViewTpuleID) &&
                     (sNode->arguments->column == sRownumColumn) &&
                     (sNode->arguments->next->module == &qtc::valueModule) &&
                     (sColumn->type.dataTypeId == MTD_BIGINT_ID) )
                {
                    sFindCount++;
                    sPredPosition = sPosition;
                }
                else
                {
                    // nothing to do.
                }
            }
            else if ( (sNode->module == &mtfGreaterEqual) ||
                      (sNode->module == &mtfGreaterThan) )
            {
                // 상수 >= rownum;
                // 상수 > rownum;
                sColumn = QTC_STMT_COLUMN( aStatement, ((qtcNode*)(sNode->arguments)));

                if ( (sNode->arguments->next->module == &qtc::columnModule) &&
                     (sNode->arguments->next->table  == sViewTpuleID) &&
                     (sNode->arguments->next->column == sRownumColumn) && 
                     (sNode->arguments->module == &qtc::valueModule) &&
                     (sColumn->type.dataTypeId == MTD_BIGINT_ID) )
                {
                    sFindCount++;
                    sPredPosition = sPosition;
                }
                else
                {
                    // nothing to do.
                }
            }
            else if ( sNode->module == &mtfBetween )
            {
                sColumn  = QTC_STMT_COLUMN( aStatement, ((qtcNode*)(sNode->arguments->next)));
                sColumn2 = QTC_STMT_COLUMN( aStatement, ((qtcNode*)(sNode->arguments->next->next)));

                // rownum between 상수 and 상수
                if ( (sNode->arguments->module == &qtc::columnModule) &&
                     (sNode->arguments->table  == sViewTpuleID) &&
                     (sNode->arguments->column == sRownumColumn) &&
                     (sNode->arguments->next->module == &qtc::valueModule) && 
                     (sNode->arguments->next->next->module == &qtc::valueModule) &&
                     (sColumn->type.dataTypeId  == MTD_BIGINT_ID) &&
                     (sColumn2->type.dataTypeId == MTD_BIGINT_ID) )
                {
                    sFindCount++;
                    sPredPosition = sPosition;
                }
                else
                {
                    // nothing to do.
                }
            }
            else
            {
                // nothing to do.
            }
        } // for

        if ( sFindCount != 1 )
        {
            sTrans = ID_FALSE;
        }
        else
        {
            // nothing to do.
        }
    }
    else
    {
        // nothing to do.
    }

    if ( sTrans == ID_TRUE )
    {
        *aPredPosition = sPredPosition;
    }
    else
    {
        // nothing to do.
    }

    return sTrans;
}

IDE_RC qmoRownumPredToLimit::doRownumPredToLimit( qcStatement   * aStatement,
                                                  qmsTableRef   * aViewTableRef,
                                                  qmoPredicate ** aPredicate,
                                                  UShort          aPredPosition )
{
    idBool         sChanged = ID_FALSE;
    qmoPredicate * sPredicate;
    qmoPredicate** sPrev;
    UShort         sPosition;

    IDU_FIT_POINT_FATAL( "qmoRownumPredToLimit::doRownumPredToLimit::__FT__" );

    // find predicate position
    for ( sPrev       = aPredicate,
          sPredicate  = *aPredicate,
          sPosition   = 0;
          sPredicate != NULL && sPosition < aPredPosition;
          sPredicate  = sPredicate->next,
          sPosition++ )
    {
        sPrev = &(sPredicate->next);
    }

    IDE_TEST( makeLimit( aStatement,
                         aViewTableRef,
                         (qtcNode*)sPredicate->node->node.arguments,
                         &sChanged )
              != IDE_SUCCESS );

    if ( sChanged == ID_TRUE )
    {
        // where 절에서 제거한다.
        *sPrev = sPredicate->next;
    }
    else
    {
        // nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool qmoRownumPredToLimit::isViewRownumToLimit( qcStatement  * aStatement,
                                                  qmsQuerySet  * aQuerySet,
                                                  qmsTableRef  * aViewTableRef )
{
    mtcNode      * sNode;
    qtcNode      * sValueNode;
    qmsParseTree * sViewParseTree;
    mtcColumn    * sColumn;
    mtcColumn    * sColumn2;
    mtdBigintType  sValue;

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------
    IDE_FT_ERROR( aStatement     != NULL );
    IDE_FT_ERROR( aQuerySet      != NULL );
    IDE_FT_ERROR( aViewTableRef  != NULL );

    //------------------------------------------
    // 조건 검사
    //------------------------------------------
    sNode = (mtcNode*)aQuerySet->SFWGH->where;
    sViewParseTree = (qmsParseTree*)(aViewTableRef->view->myPlan->parseTree);

    if ( sViewParseTree->limit != NULL )
    {
        IDE_CONT( INVALID_FORM );
    }
    else
    {
        // nothing to do.
    }

    if ( sNode != NULL )
    {
        if ( (sNode->lflag & MTC_NODE_BIND_MASK) == MTC_NODE_BIND_EXIST )
        {
            IDE_CONT( INVALID_FORM );
        }
        else
        {
            // nothing to do.
        }

        if ( (((qtcNode*)sNode)->lflag & QTC_NODE_ROWNUM_MASK)
              ==QTC_NODE_ROWNUM_ABSENT )
        {
            IDE_CONT( INVALID_FORM );
        }
        else
        {
            // nothing to do
        }

        if ( qtc::haveDependencies( &(((qtcNode*)sNode)->depInfo) ) == ID_TRUE )
        {
            IDE_CONT( INVALID_FORM );
        }
        else
        {
            // nothing to do
        }

        if ( sNode->next != NULL )
        {
            IDE_CONT( INVALID_FORM );
        }
        else
        {
            // nothing to do
        }

        // 다음의 형태만 지원한다.
        if ( sNode->module == &mtfEqual )
        {
            // rownum = 상수;
            // 상수 = rownum;
            if ( (sNode->arguments->module == &qtc::columnModule) &&
                 (sNode->arguments->next->module == &qtc::valueModule) )
            {
                sValueNode  = (qtcNode*)(sNode->arguments->next);
            }
            else if ( (sNode->arguments->module == &qtc::valueModule) &&
                      (sNode->arguments->next->module == &qtc::columnModule) )
            {
                sValueNode  = (qtcNode*)(sNode->arguments);
            }
            else
            {
                IDE_CONT( INVALID_FORM );
            }

            sColumn = QTC_STMT_COLUMN( aStatement, sValueNode );

            if ( sColumn->type.dataTypeId == MTD_BIGINT_ID )
            {
                sValue = *((mtdBigintType *)QTC_STMT_FIXEDDATA( aStatement,
                                                                sValueNode ));

                if ( sValue != 1 )
                {
                    IDE_CONT( INVALID_FORM );
                }
                else
                {
                    // nothing to do
                }
            }
            else
            {
                IDE_CONT( INVALID_FORM );
            }
        }
        else if ( (sNode->module == &mtfLessEqual) ||
                  (sNode->module == &mtfLessThan) )
        {
            // rownum <= 상수;
            // rownum < 상수;
            sColumn = QTC_STMT_COLUMN( aStatement, ((qtcNode*)(sNode->arguments->next)));

            if ( (sNode->arguments->module == &qtc::columnModule) &&
                (sNode->arguments->next->module == &qtc::valueModule) &&
                (sColumn->type.dataTypeId == MTD_BIGINT_ID) )
            {
                // nothing to do
            }
            else
            {
                IDE_CONT( INVALID_FORM );
            }
        }
        else if ( (sNode->module == &mtfGreaterEqual) ||
                  (sNode->module == &mtfGreaterThan) )
        {
            // 상수 >= rownum;
            // 상수 > rownum;
            sColumn = QTC_STMT_COLUMN( aStatement, ((qtcNode*)(sNode->arguments)));

            if ( (sNode->arguments->next->module == &qtc::columnModule) &&
                 (sNode->arguments->module == &qtc::valueModule) &&
                 (sColumn->type.dataTypeId == MTD_BIGINT_ID) )
            {
                // nothing to do
            }
            else
            {
                IDE_CONT( INVALID_FORM );
            }
        }
        else if ( sNode->module == &mtfBetween )
        {
            sColumn  = QTC_STMT_COLUMN( aStatement, ((qtcNode*)(sNode->arguments->next)));
            sColumn2 = QTC_STMT_COLUMN( aStatement, ((qtcNode*)(sNode->arguments->next->next)));

            // rownum between 상수 and 상수
            if ( (sNode->arguments->module == &qtc::columnModule) &&
                 (sNode->arguments->next->module == &qtc::valueModule) && 
                 (sNode->arguments->next->next->module == &qtc::valueModule) &&
                 (sColumn->type.dataTypeId  == MTD_BIGINT_ID) &&
                 (sColumn2->type.dataTypeId == MTD_BIGINT_ID) )
            {
                // nothing to do
            }
            else
            {
                IDE_CONT( INVALID_FORM );
            }
        }
        else
        {
            IDE_CONT( INVALID_FORM );
        }
    }
    else
    {
        IDE_CONT( INVALID_FORM );
    }

    return ID_TRUE;

    IDE_EXCEPTION_CONT( INVALID_FORM );

    IDE_EXCEPTION_END;

    return ID_FALSE;
}

IDE_RC qmoRownumPredToLimit::doViewRownumToLimit( qcStatement  * aStatement,
                                                  qmsQuerySet  * aQuerySet,
                                                  qmsTableRef  * aViewTableRef )
{
    idBool         sChanged = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmoRownumPredToLimit::doViewRownumToLimit::__FT__" );

    IDE_TEST( makeLimit( aStatement,
                         aViewTableRef,
                         aQuerySet->SFWGH->where,
                         & sChanged )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
