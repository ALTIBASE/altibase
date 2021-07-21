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
 * $Id: qmoConstExpr.cpp 23857 2008-03-19 02:36:53Z sungminee $
 **********************************************************************/

#include <idl.h>
#include <qcg.h>
#include <qtc.h>
#include <qmoConstExpr.h>
#include <qcuSqlSourceInfo.h>

IDE_RC
qmoConstExpr::processConstExpr( qcStatement  * aStatement,
                                qmsSFWGH     * aSFWGH )
{
/***********************************************************************
 *
 * Description : PROJ-1413 Simple View Merging
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsTarget         * sTarget;
    qmsConcatElement  * sElement;
    qmsConcatElement  * sSubElement;
    qtcNode           * sList;

    qtcCallBackInfo sCallBackInfo = {
        QC_SHARED_TMPLATE(aStatement),
        QC_QMP_MEM(aStatement),
        aStatement,
        NULL,
        NULL,
        NULL
    };
    mtcCallBack sCallBack = {
        & sCallBackInfo,
        MTC_ESTIMATE_ARGUMENTS_ENABLE,
        qtc::alloc,
        NULL
    };

    IDU_FIT_POINT_FATAL( "qmoConstExpr::processConstExpr::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aSFWGH != NULL );

    //---------------------------------------------------
    // constant expression 노드 변환
    //---------------------------------------------------

    // simple view merging에 의해 필요한 경우만 처리한다.
    if ( aSFWGH->isTransformed == ID_TRUE )
    {
        //---------------------------------------------------
        // WHERE 절의 노드 변환
        //---------------------------------------------------

        if ( aSFWGH->where != NULL )
        {
            IDE_TEST( processNode( aSFWGH->where,
                                   & QC_SHARED_TMPLATE( aStatement )->tmplate,
                                   QC_SHARED_TMPLATE( aStatement )->tmplate.stack,
                                   QC_SHARED_TMPLATE( aStatement )->tmplate.stackCount,
                                   & sCallBack )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }

        //---------------------------------------------------
        // GROUP BY 절의 노드 변환
        //---------------------------------------------------

        for ( sElement  = aSFWGH->group;
              sElement != NULL;
              sElement  = sElement->next )
        {
            /* PROJ-1353 */
            if ( sElement->type == QMS_GROUPBY_NORMAL )
            {
                IDE_TEST( processNode( sElement->arithmeticOrList,
                                       & QC_SHARED_TMPLATE( aStatement )->tmplate,
                                       QC_SHARED_TMPLATE( aStatement )->tmplate.stack,
                                       QC_SHARED_TMPLATE( aStatement )->tmplate.stackCount,
                                       & sCallBack )
                          != IDE_SUCCESS );
            }
            else
            {
                for ( sSubElement  = sElement->arguments;
                      sSubElement != NULL;
                      sSubElement  = sSubElement->next )
                {
                    if ( ( sSubElement->arithmeticOrList->node.lflag & MTC_NODE_OPERATOR_MASK )
                         == MTC_NODE_OPERATOR_LIST )
                    {
                        for ( sList  = (qtcNode *)sSubElement->arithmeticOrList->node.arguments;
                              sList != NULL;
                              sList  = ( qtcNode * )sList->node.next )
                        {
                            IDE_TEST( processNode( sList,
                                                   & QC_SHARED_TMPLATE( aStatement )->tmplate,
                                                   QC_SHARED_TMPLATE( aStatement )->tmplate.stack,
                                                   QC_SHARED_TMPLATE( aStatement )->tmplate.stackCount,
                                                   & sCallBack )
                                      != IDE_SUCCESS );
                        }
                    }
                    else
                    {
                        IDE_TEST( processNode( sSubElement->arithmeticOrList,
                                               & QC_SHARED_TMPLATE( aStatement )->tmplate,
                                               QC_SHARED_TMPLATE( aStatement )->tmplate.stack,
                                               QC_SHARED_TMPLATE( aStatement )->tmplate.stackCount,
                                               & sCallBack )
                                  != IDE_SUCCESS );
                    }
                }
            }
        }

        //---------------------------------------------------
        // target list의 노드 변환
        //---------------------------------------------------

        for ( sTarget  = aSFWGH->target;
              sTarget != NULL;
              sTarget  = sTarget->next )
        {
            IDE_TEST( processNode( sTarget->targetColumn,
                                   & QC_SHARED_TMPLATE( aStatement )->tmplate,
                                   QC_SHARED_TMPLATE( aStatement )->tmplate.stack,
                                   QC_SHARED_TMPLATE( aStatement )->tmplate.stackCount,
                                   & sCallBack )
                      != IDE_SUCCESS );
        }

        //---------------------------------------------------
        // HAVING 절의 노드 변환
        //---------------------------------------------------

        if ( aSFWGH->having != NULL )
        {
            IDE_TEST( processNode( aSFWGH->having,
                                   & QC_SHARED_TMPLATE( aStatement )->tmplate,
                                   QC_SHARED_TMPLATE( aStatement )->tmplate.stack,
                                   QC_SHARED_TMPLATE( aStatement )->tmplate.stackCount,
                                   & sCallBack )
                      != IDE_SUCCESS );
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

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoConstExpr::processConstExprForOrderBy( qcStatement  * aStatement,
                                          qmsParseTree * aParseTree )
{
/***********************************************************************
 *
 * Description : PROJ-1413 Simple View Merging
 *
 * Implementation :
 *
 ***********************************************************************/

    qmsSortColumns  * sSortColumn;

    qtcCallBackInfo sCallBackInfo = {
        QC_SHARED_TMPLATE(aStatement),
        QC_QMP_MEM(aStatement),
        aStatement,
        NULL,
        NULL,
        NULL
    };
    mtcCallBack sCallBack = {
        & sCallBackInfo,
        MTC_ESTIMATE_ARGUMENTS_ENABLE,
        qtc::alloc,
        NULL
    };

    IDU_FIT_POINT_FATAL( "qmoConstExpr::processConstExprForOrderBy::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aParseTree != NULL );

    //------------------------------------------
    // validation 수행
    //------------------------------------------

    // simple view merging에 의해 필요한 경우만 처리한다.
    if ( aParseTree->isTransformed == ID_TRUE )
    {
        if ( aParseTree->orderBy != NULL )
        {
            for ( sSortColumn  = aParseTree->orderBy;
                  sSortColumn != NULL;
                  sSortColumn  = sSortColumn->next )
            {
                IDE_TEST( processNode( sSortColumn->sortColumn,
                                       & QC_SHARED_TMPLATE( aStatement )->tmplate,
                                       QC_SHARED_TMPLATE( aStatement )->tmplate.stack,
                                       QC_SHARED_TMPLATE( aStatement )->tmplate.stackCount,
                                       & sCallBack )
                          != IDE_SUCCESS );
            }
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

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoConstExpr::processNode( qtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack )
{
/***********************************************************************
 *
 * Description : PROJ-1413 Simple View Merging
 *
 * Implementation :
 *
 ***********************************************************************/

    qtcNode           * sNode;
    qtcCallBackInfo   * sInfo;
    mtcStack          * sStack;
    SInt                sRemain;
    qcuSqlSourceInfo    sqlInfo;
    UInt                sSqlCode;

    IDU_FIT_POINT_FATAL( "qmoConstExpr::processNode::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aNode != NULL );
    IDE_DASSERT( aTemplate != NULL );
    IDE_DASSERT( aCallBack != NULL );

    //------------------------------------------
    // 초기화
    //------------------------------------------

    sInfo = (qtcCallBackInfo*) aCallBack->info;

    //------------------------------------------
    // 노드 트리 순회
    //------------------------------------------

    for( sNode  = (qtcNode*) aNode->node.arguments;
         sNode != NULL;
         sNode  = (qtcNode*) sNode->node.next )
    {
        if ( sNode->node.module == & qtc::subqueryModule )
        {
            // subquery 노드는 순회하지 않는다.

            // Nothing to do.
        }
        else if ( sNode->node.module == & qtc::passModule )
        {
            // pass 노드는 순회하지 않는다.

            // Nothing to do.
        }
        else
        {
            IDE_TEST( processNode( sNode,
                                   aTemplate,
                                   aStack,
                                   aRemain,
                                   aCallBack )
                      != IDE_SUCCESS );
        }
    }

    //------------------------------------------
    // constant exprssion의 상수 변환
    //------------------------------------------

    // Constant Expression에 대한 선처리를 시도한다.
    IDE_TEST( qtc::preProcessConstExpr( sInfo->statement,
                                        aNode,
                                        aTemplate,
                                        aStack,
                                        aRemain,
                                        aCallBack )
              != IDE_SUCCESS );

    if ( ( aNode->node.lflag & MTC_NODE_REESTIMATE_MASK )
         == MTC_NODE_REESTIMATE_TRUE )
    {
        // BUG-37483 node의 estimate시 argument의 stack을 쌓아준다.
        for( sNode  = (qtcNode*) aNode->node.arguments,
                 sStack = aStack + 1, sRemain = aRemain - 1;
             sNode != NULL;
             sNode  = (qtcNode*) sNode->node.next,
                 sStack++, sRemain-- )
        {
            // BUG-33674
            IDE_TEST_RAISE( sRemain < 1, ERR_STACK_OVERFLOW );

            sNode = (qtcNode*) mtf::convertedNode( (mtcNode*)sNode,
                                                   aTemplate );

            sStack->column = aTemplate->rows[sNode->node.table].columns
                + sNode->node.column;
        }

        if ( aNode->node.module->estimate( (mtcNode*)aNode,
                                           aTemplate,
                                           aStack,
                                           aRemain,
                                           aCallBack )
             != IDE_SUCCESS )
        {
            sqlInfo.setSourceInfo( sInfo->statement,
                                   & aNode->position );
            IDE_RAISE( ERR_PASS );
        }
        else
        {
            // Nothing To Do
        }

        // PROJ-1413
        // re-estimate를 수행했으므로 다음 estimate를 위해
        // re-estimate를 꺼둔다.
        aNode->node.lflag &= ~MTC_NODE_REESTIMATE_MASK;
        aNode->node.lflag |= MTC_NODE_REESTIMATE_FALSE;
    }
    else
    {
        // Nothing To Do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_PASS );
    {
        // sqlSourceInfo가 없는 error라면.
        if ( ideHasErrorPosition() == ID_FALSE )
        {
            sSqlCode = ideGetErrorCode();

            (void)sqlInfo.initWithBeforeMessage(sInfo->memory);
            IDE_SET(
                ideSetErrorCode(qpERR_ABORT_QSX_SQLTEXT_WRAPPER,
                                sqlInfo.getBeforeErrMessage(),
                                sqlInfo.getErrMessage()));
            (void)sqlInfo.fini();

            // overwrite wrapped errorcode to original error code.
            ideGetErrorMgr()->Stack.LastError = sSqlCode;
        }
        else
        {
            // Nothing to do.
        }
    }
    IDE_EXCEPTION( ERR_STACK_OVERFLOW )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
