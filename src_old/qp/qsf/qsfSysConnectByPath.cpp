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
 * $Id: qsfSysConnectByPath.cpp 89835 2021-01-22 10:10:02Z andrew.shin $
 *
 * SYS_CONNECT_BY_PATH( ColumnName, '\' )
 *  지정된 컬럼에 대해 레벨 1부터 현제 레벨 까지의 값을 구분자로 구분해서 보여준다.
 *  ColumnName은 순수 컬럼만 가능하다.
 *  Delimiter는 항상 CONSTANT만 가능하다.
 *  sSFWGH->hierStack 의 Pseudo Column에 Hierarchy Query의 Stack
 *  포인터가 있다. 이를 통해서 Root Node 부터 현제 까지의 Row를 얻는다.
 *
 *  이 SYS_CONNECT_BY_PATH의 컬럽 타입은 항상 mtdVarchar 이다.
 ***********************************************************************/

#include <qsf.h>
#include <qci.h>
#include <qcg.h>
#include <mtc.h>
#include <qmnConnectBy.h>
#include <qcgPlan.h>
#include <qcuProperty.h>
#include <qmv.h>

extern mtdModule mtdVarchar;

static mtcName qsfSysConnectByPathFunctionName[1] = {
        { NULL, 19, (void *)"SYS_CONNECT_BY_PATH" }
};

static IDE_RC qsfSysConnectByPathEstimate( mtcNode     * aNode,
                                           mtcTemplate * aTemplate,
                                           mtcStack    * aStack,
                                           SInt          aRemain,
                                           mtcCallBack * aCallBack );

IDE_RC qsfSysConnectByPathCalculate( mtcNode     * aNode,
                                     mtcStack    * aStack,
                                     SInt          aRemain,
                                     void        * aInfo,
                                     mtcTemplate * aTemplate );

mtfModule qsfSysConnectByPathModule = {
        1 | MTC_NODE_OPERATOR_MISC |
            MTC_NODE_VARIABLE_TRUE |
            MTC_NODE_FUNCTION_CONNECT_BY_TRUE,
        ~( MTC_NODE_INDEX_MASK ),
        1.0,
        qsfSysConnectByPathFunctionName,
        NULL,
        mtf::initializeDefault,
        mtf::finalizeDefault,
        qsfSysConnectByPathEstimate
};

const mtcExecute qsfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfSysConnectByPathCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

static IDE_RC qsfSysConnectByPathEstimate( mtcNode     * aNode,
                                           mtcTemplate * aTemplate,
                                           mtcStack    * aStack,
                                           SInt          aRemain,
                                           mtcCallBack * aCallBack )
{
    qtcCallBackInfo * sCallBackInfo = NULL;
    qmsSFWGH        * sSFWGH        = NULL;
    UInt              sFence        = 0;
    qtcNode         * sNode         = NULL;
    mtcNode         * sMtc          = NULL;
    const mtdModule * sModules[2]   = { &mtdVarchar, &mtdVarchar };

    sFence = aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK;

    IDE_TEST_RAISE( sFence != 2, ERR_INVALID_FUNCTION_ARGUMENT );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_DISTINCT_MASK ) == MTC_NODE_DISTINCT_TRUE,
                    ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) == MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    sNode = (qtcNode *)aNode->arguments;

    sMtc = sNode->node.next;

    IDE_TEST_RAISE(( aTemplate->rows[sMtc->table].lflag & MTC_TUPLE_TYPE_MASK )
                     != MTC_TUPLE_TYPE_CONSTANT,
                     ERR_ALLOW_ONLY_CONSTANT );

    sCallBackInfo = (qtcCallBackInfo*)(aCallBack->info);
    sSFWGH        = sCallBackInfo->SFWGH;

    /* BUG-39284 The sys_connect_by_path function with Aggregate
     * function is not correct.
     */
    sSFWGH->lflag &= ~QMV_SFWGH_CONNECT_BY_FUNC_MASK;
    sSFWGH->lflag |= QMV_SFWGH_CONNECT_BY_FUNC_TRUE;

    IDE_TEST_RAISE( aRemain < 3, ERR_STACK_OVERFLOW );
    IDE_TEST_RAISE( sSFWGH            == NULL, ERR_NO_HIERARCHY );
    IDE_TEST_RAISE( sSFWGH->hierarchy == NULL, ERR_NO_HIERARCHY );
    IDE_TEST_RAISE( sSFWGH->validatePhase == QMS_VALIDATE_HIERARCHY,
                    ERR_NOT_ALLOW_CLAUSE );

    sMtc = aNode->arguments;

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;
    aTemplate->rows[aNode->table].execute[aNode->column] = qsfExecute;
    aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                                              (void *)&sSFWGH->cnbyStackAddr;

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        sMtc,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    /* mtdVarchar Type으로 초기화 */
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     &mtdVarchar,
                                     1,
                                     QCU_SYS_CONNECT_BY_PATH_PRECISION,
                                     0)
              != IDE_SUCCESS );

    // environment의 기록
    if ( sCallBackInfo->statement != NULL )
    {
        qcgPlan::registerPlanProperty( sCallBackInfo->statement,
                                       PLAN_PROPERTY_SYS_CONNECT_BY_PATH_PRECISION );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_INVALID_FUNCTION_ARGUMENT ));
    }
    IDE_EXCEPTION( ERR_NOT_AGGREGATION )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));
    }
    IDE_EXCEPTION( ERR_ALLOW_ONLY_CONSTANT )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_ARGUMENT_NOT_APPLICABLE ));
    }
    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    {
       IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    }
    IDE_EXCEPTION( ERR_NO_HIERARCHY );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_SYS_CONNECT_BY_PATH_NEED_CONNECT_BY ));
    }
    IDE_EXCEPTION( ERR_NOT_ALLOW_CLAUSE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_NOT_ALLOW_SYS_CONNECT_BY_PATH ));
    }

    IDE_EXCEPTION_END

    return IDE_FAILURE;
}

IDE_RC qsfSysConnectByPathCalculate( mtcNode     * aNode,
                                     mtcStack    * aStack,
                                     SInt          aRemain,
                                     void        * aInfo,
                                     mtcTemplate * aTemplate )
{
    mtcNode       * sNode1       = NULL;
    mtcNode       * sNode2       = NULL;
    qtcNode      ** sTmp         = NULL;
    qmnCNBYStack ** sTmp2        = NULL;
    qmnCNBYStack  * sStack       = NULL;
    qmnCNBYStack  * sFirstStack  = NULL;
    qmnCNBYItem   * sItem        = NULL;
    mtdCharType   * sDelimiter   = NULL;
    mtdCharType   * sSource      = NULL;
    mtdCharType   * sDest        = NULL;
    UInt            sLevel       = 0;
    UInt            i            = 0;
    UInt            sOffset      = 0;
    qmdMtrNode    * sMtrNode     = NULL;
    mtcColumn     * sOrgColumns  = NULL;
    void          * sOrgRow      = NULL;
    
    IDE_TEST_RAISE( aRemain < 3, ERR_STACK_OVERFLOW );
    IDE_TEST_RAISE( aInfo == NULL, ERR_INTERNAL );
    sTmp   = (qtcNode **)(aInfo);

    IDE_TEST_RAISE( *sTmp == NULL, ERR_INTERNAL );
    sTmp2 = (qmnCNBYStack **)(aTemplate->rows[(*sTmp)->node.table].row);

    IDE_TEST_RAISE( *sTmp2 == NULL, ERR_INTERNAL );
    sStack = *sTmp2;

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack[0].value  = (void *) mtc::value( aStack[0].column,
                                            aTemplate->rows[aNode->table].row,
                                            MTD_OFFSET_USE );
    sNode1 = aNode->arguments;
    sNode2 = sNode1->next;

    /* backup view tuple */
    sOrgColumns = aTemplate->rows[sStack->myRowID].columns;
    sOrgRow = aTemplate->rows[sStack->myRowID].row;
    
    //--------------------------------------
    // calculate argument 2 (delimiter)
    //--------------------------------------

    if ( sNode2->column != MTC_RID_COLUMN_ID )
    {
        IDE_TEST( aTemplate->rows[sNode2->table].
                  execute[sNode2->column].calculate( sNode2,
                                                     &aStack[2],
                                                     aRemain,
                                                     aTemplate->rows[sNode2->table].
                                                     execute[sNode2->column].calculateInfo,
                                                     aTemplate )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( aTemplate->rows[sNode2->table].
                  ridExecute->calculate( sNode2,
                                         &aStack[2],
                                         aRemain,
                                         NULL,
                                         aTemplate )
                  != IDE_SUCCESS );
    }
    
    if ( sNode2->conversion != NULL )
    {
        IDE_TEST( mtf::convertCalculate( sNode2,
                                         &aStack[2],
                                         aRemain,
                                         NULL,
                                         aTemplate )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    sDelimiter = (mtdCharType *)aStack[2].value;
    
    //--------------------------------------
    // calculate argument 1 (expression)
    //--------------------------------------

    sDest = (mtdCharType *)aStack[0].value;

    sFirstStack = sStack;
    for ( sLevel = 0; sLevel < sFirstStack->currentLevel; sLevel++ )
    {
        if ( sDelimiter != NULL )
        {
            IDE_TEST_RAISE( sOffset + sDelimiter->length >
                            (UInt)aStack[0].column->precision,
                            ERR_INVALID_LENGTH );

            idlOS::memcpy( (UChar *)sDest->value + sOffset,
                           (UChar *)sDelimiter->value,
                           sDelimiter->length );
            sOffset += sDelimiter->length;
        }
        else
        {
            /* Nothing to do */
        }

        /* 64레벨 이상일 경우 다음 스택을 찾아야한다. */
        if ( sLevel < QMND_CNBY_BLOCKSIZE )
        {
            i = sLevel;
        }
        else
        {
            i = sLevel % QMND_CNBY_BLOCKSIZE;
            if ( i == 0 )
            {
                sStack = sStack->next;
            }
            else
            {
                /* Nothing to do */
            }
        }

        sItem = &sStack->items[i];

        // PROJ-2362 memory temp 저장 효율성 개선
        /* PROJ-2641 Hierarchy Query Index
         * Table에 대한 Hierarchy query는 baseMTR이 NULL 이다.
         */
        if ( ( QCU_REDUCE_TEMP_MEMORY_ENABLE == 1 ) &&
             ( sStack->baseMTR != NULL ) )
        {
            for ( sMtrNode = sStack->baseMTR->recordNode;
                  sMtrNode != NULL;
                  sMtrNode = sMtrNode->next )
            {
                if ( SMI_COLUMN_TYPE_IS_TEMP( sMtrNode->dstColumn->column.flag )
                     == ID_TRUE )
                {
                    IDE_TEST( sMtrNode->func.setTuple( (qcTemplate*)aTemplate,
                                                       sMtrNode,
                                                       (void *)sItem->rowPtr )
                              != IDE_SUCCESS );
                }
                else
                {
                    /* Nothing to do */
                }
            }
            
            /* BUG-40027
             * temp type이 있으므로 column정보까지 변경해야 한다.
             */
            aTemplate->rows[sStack->myRowID].columns =
                aTemplate->rows[sStack->baseRowID].columns;
        }
        else
        {
            /* Nothing to do */
        }

        /* BUG-39848
         * arguments에 외부 참조컬럼이 있는 subquery가 있고, store되는 경우
         * connect by를 reference하고 있기때문에 modify count를 변경해야한다.
         */
        aTemplate->rows[sStack->myRowID].row = sItem->rowPtr;
        aTemplate->rows[sStack->myRowID].modify++;

        if ( sStack->myRowID != sStack->baseRowID )
        {
            /* BUG-39611 baseTuple에 row Pointer를 저장후 arguments에
             * 대한 calculate 를 수행한다.
             */
            aTemplate->rows[sStack->baseRowID].row = sItem->rowPtr;
            aTemplate->rows[sStack->baseRowID].modify++;
        }
        else
        {
            /* Nothing to do */
        }

        if ( sNode1->column != MTC_RID_COLUMN_ID )
        {
            IDE_TEST( aTemplate->rows[sNode1->table].
                      execute[sNode1->column].calculate( sNode1,
                                                         &aStack[1],
                                                         aRemain,
                                                         aTemplate->rows[sNode1->table].
                                                         execute[sNode1->column].calculateInfo,
                                                         aTemplate )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( aTemplate->rows[sNode1->table].
                      ridExecute->calculate( sNode1,
                                             &aStack[1],
                                             aRemain,
                                             NULL,
                                             aTemplate )
                      != IDE_SUCCESS );
        }

        if ( sNode1->conversion != NULL )
        {
            IDE_TEST( mtf::convertCalculate( sNode1,
                                             &aStack[1],
                                             aRemain,
                                             NULL,
                                             aTemplate )
                     != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }

        sSource = (mtdCharType *)aStack[1].value;

        if ( sSource->length > 0 )
        {
            IDE_TEST_RAISE( sOffset + sSource->length >
                            (UInt)aStack[0].column->precision,
                            ERR_INVALID_LENGTH );

            idlOS::memcpy( (UChar *)sDest->value + sOffset,
                           (UChar *)sSource->value,
                           sSource->length );
            sOffset += sSource->length;
        }
        else
        {
            /* Nothing to do */
        }
    }

    sDest->length = sOffset;

    /* restore view tuple */
    if ( QCU_REDUCE_TEMP_MEMORY_ENABLE == 1 )
    {
        aTemplate->rows[sStack->myRowID].columns = sOrgColumns;
    }
    else
    {
        /* Nothing to do */
    }
    aTemplate->rows[sStack->myRowID].row = sOrgRow;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    {
       IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    }
    IDE_EXCEPTION( ERR_INVALID_LENGTH )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LENGTH));
    }
    IDE_EXCEPTION( ERR_INTERNAL )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qsfSysConnectByPathCalculate",
                                  "The stack pointer is NULL" ));
    }

    /* restore view tuple */
    if ( sOrgColumns != NULL )
    {
        aTemplate->rows[sStack->myRowID].columns = sOrgColumns;
    }
    else
    {
        /* Nothing to do */
    }
    if ( sOrgRow != NULL )
    {
        aTemplate->rows[sStack->myRowID].row = sOrgRow;
    }
    else
    {
        /* Nothing to do */
    }
    
    IDE_EXCEPTION_END

    return IDE_FAILURE;
}
