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
 * $Id: qsfConnectByRoot.cpp 89835 2021-01-22 10:10:02Z andrew.shin $
 *
 * CONNECT_BY_ROOT ( ColumName )
 *  지정된 컬럼의 Hierarchy에서 레벨 1인 Root Node의 값을 보여준다.
 *  Column_Name에 항상 순수 컬럼만 올 수 있다.
 *  CONNECT BY 구문이 항상 나와야한다.
 *  sSFWGH->hierStack 의 Pseudo Column에 Hierarchy Query의 Stack
 *  포인터가 있다. 이를 통해서 Root Node의 Row를 얻는다.
 ***********************************************************************/

#include <qsf.h>
#include <qci.h>
#include <qcg.h>
#include <mtc.h>
#include <qmnConnectBy.h>
#include <qmv.h>

extern mtdModule mtdList;

static mtcName qsfConnectByRootFunctionName[1] = {
        { NULL, 15, (void *)"CONNECT_BY_ROOT" }
};

static IDE_RC qsfConnectByRootEstimate( mtcNode     * aNode,
                                        mtcTemplate * aTemplate,
                                        mtcStack    * aStack,
                                        SInt          aRemain,
                                        mtcCallBack * aCallback );

IDE_RC qsfConnectByRootCalculate( mtcNode     * aNode,
                                  mtcStack    * aStack,
                                  SInt          aRemain,
                                  void        * aInfo,
                                  mtcTemplate * aTemplate );

mtfModule qsfConnectByRootModule = {
    1 | MTC_NODE_OPERATOR_MISC |
        MTC_NODE_VARIABLE_TRUE |
        MTC_NODE_FUNCTION_CONNECT_BY_TRUE,
    ~(MTC_NODE_INDEX_MASK),
    1.0,
    qsfConnectByRootFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfConnectByRootEstimate
};

const mtcExecute qsfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfConnectByRootCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

static IDE_RC qsfConnectByRootEstimate( mtcNode     * aNode,
                                        mtcTemplate * aTemplate,
                                        mtcStack    * aStack,
                                        SInt          aRemain,
                                        mtcCallBack * aCallBack )
{
    qtcCallBackInfo * sCallBackInfo = NULL;
    qmsSFWGH        * sSFWGH        = NULL;
    UInt              sFence        = 0;

    sFence = aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK;

    IDE_TEST_RAISE( sFence != 1, ERR_INVALID_FUNCTION_ARGUMENT );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_DISTINCT_MASK ) == MTC_NODE_DISTINCT_TRUE,
                    ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) == MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    sCallBackInfo = (qtcCallBackInfo*)(aCallBack->info);
    sSFWGH        = sCallBackInfo->SFWGH;

    /* BUG-39284 The sys_connect_by_path function with Aggregate
     * function is not correct.
     */
    sSFWGH->lflag &= ~QMV_SFWGH_CONNECT_BY_FUNC_MASK;
    sSFWGH->lflag |= QMV_SFWGH_CONNECT_BY_FUNC_TRUE;

    IDE_TEST_RAISE( aRemain < 2, ERR_STACK_OVERFLOW );
    IDE_TEST_RAISE( sSFWGH            == NULL, ERR_NO_HIERARCHY );
    IDE_TEST_RAISE( sSFWGH->hierarchy == NULL, ERR_NO_HIERARCHY );
    IDE_TEST_RAISE( sSFWGH->validatePhase == QMS_VALIDATE_HIERARCHY,
                    ERR_NOT_ALLOW_CLAUSE );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;
    aTemplate->rows[aNode->table].execute[aNode->column] = qsfExecute;
    aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                                              (void *)&sSFWGH->cnbyStackAddr;

    IDE_TEST_RAISE( aStack[1].column->module == &mtdList, ERR_CONVERSION_NOT_APPLICABLE );

    mtc::initializeColumn( aStack[0].column, aStack[1].column );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_INVALID_FUNCTION_ARGUMENT ));
    }
    IDE_EXCEPTION( ERR_NOT_AGGREGATION )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));
    }
    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    {
       IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    }
    IDE_EXCEPTION( ERR_NO_HIERARCHY );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_CONNECT_BY_ROOT_NEED_CONNECT_BY ));
    }
    IDE_EXCEPTION( ERR_NOT_ALLOW_CLAUSE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMV_NOT_ALLOW_CONNECT_BY_ROOT ));
    }
    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));
    }
    IDE_EXCEPTION_END

    return IDE_FAILURE;
}

IDE_RC qsfConnectByRootCalculate( mtcNode     * aNode,
                                  mtcStack    * aStack,
                                  SInt          aRemain,
                                  void        * aInfo,
                                  mtcTemplate * aTemplate )
{
    mtcNode       * sNode  = NULL;
    qtcNode      ** sTmp   = NULL;
    qmnCNBYStack ** sTmp2  = NULL;
    qmnCNBYStack  * sStack = NULL;
    qmnCNBYItem   * sItem  = NULL;
    qmdMtrNode    * sMtrNode;
    mtcColumn     * sOrgColumns = NULL;
    void          * sOrgRow     = NULL;

    IDE_TEST_RAISE( aRemain < 2, ERR_STACK_OVERFLOW );

    IDE_TEST_RAISE( aInfo == NULL, ERR_INTERNAL );
    sTmp   = (qtcNode **)(aInfo);

    IDE_TEST_RAISE( *sTmp == NULL, ERR_INTERNAL );
    sTmp2  = (qmnCNBYStack **)(aTemplate->rows[(*sTmp)->node.table].row);

    IDE_TEST_RAISE( *sTmp2 == NULL, ERR_INTERNAL );
    sStack = *sTmp2;
    sItem  = sStack->items;

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack[0].value  = (void *) mtc::value( aStack[0].column,
                                            aTemplate->rows[aNode->table].row,
                                            MTD_OFFSET_USE );

    sNode = aNode->arguments;

    /* backup view tuple */
    sOrgColumns = aTemplate->rows[sStack->myRowID].columns;
    sOrgRow = aTemplate->rows[sStack->myRowID].row;
    
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

    if ( sNode->column != MTC_RID_COLUMN_ID )
    {
        IDE_TEST( aTemplate->rows[sNode->table].
                  execute[sNode->column].calculate( sNode,
                                                     &aStack[1],
                                                     aRemain,
                                                     aTemplate->rows[sNode->table].
                                                     execute[sNode->column].calculateInfo,
                                                     aTemplate )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( aTemplate->rows[sNode->table].
                  ridExecute->calculate( sNode,
                                         &aStack[1],
                                         aRemain,
                                         NULL,
                                         aTemplate )
                  != IDE_SUCCESS );
    }

    if ( sNode->conversion != NULL )
    {
        IDE_TEST( mtf::convertCalculate( sNode,
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
    
    idlOS::memcpy( aStack[0].value,
                   aStack[1].value,
                   aStack[1].column->module->actualSize( aStack[1].column,
                                                         aStack[1].value ));

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

    IDE_EXCEPTION( ERR_STACK_OVERFLOW )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    }

    IDE_EXCEPTION( ERR_INTERNAL )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qsfConnectByRootCalculate",
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
