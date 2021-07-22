/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 

/***********************************************************************
 * $Id: mtfFirstValueIgnoreNulls.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>

extern mtfModule mtfFirstValueIgnoreNulls;

extern mtdModule mtdList;
extern mtdModule mtdBoolean;

static mtcName mtfFirstValueIgnoreNullsName[1] = {
    { NULL,  24, ( void * )"FIRST_VALUE_IGNORE_NULLS" }
};

static IDE_RC mtfFirstValueIgnoreNullsEstimate( mtcNode     * aNode,
                                                mtcTemplate * aTemplate,
                                                mtcStack    * aStack,
                                                SInt          aRemain,
                                                mtcCallBack * aCallBack );

mtfModule mtfFirstValueIgnoreNulls = {
    2 | MTC_NODE_OPERATOR_AGGREGATION  |
        MTC_NODE_FUNCTION_ANALYTIC_TRUE |
        MTC_NODE_FUNCTION_WINDOWING_TRUE,
    ~( MTC_NODE_INDEX_MASK ),
    1.0, /* default selectivity ( 비교 연산자가 아님 ) */
    mtfFirstValueIgnoreNullsName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfFirstValueIgnoreNullsEstimate
};

IDE_RC mtfFirstValueIgnoreNullsInitialize( mtcNode     * aNode,
                                           mtcStack    * aStack,
                                           SInt          aRemain,
                                           void        * aInfo,
                                           mtcTemplate * aTemplate );

IDE_RC mtfFirstValueIgnoreNullsAggregate( mtcNode     * aNode,
                                          mtcStack    * aStack,
                                          SInt          aRemain,
                                          void        * aInfo,
                                          mtcTemplate * aTemplate );

IDE_RC mtfFirstValueIgnoreNullsFinalize( mtcNode     * aNode,
                                         mtcStack    * aStack,
                                         SInt          aRemain,
                                         void        * aInfo,
                                         mtcTemplate * aTemplate );

IDE_RC mtfFirstValueIgnoreNullsCalculate( mtcNode     * aNode,
                                          mtcStack    * aStack,
                                          SInt          aRemain,
                                          void        * aInfo,
                                          mtcTemplate * aTemplate );

static const mtcExecute mtfExecute = {
    mtfFirstValueIgnoreNullsInitialize,
    mtfFirstValueIgnoreNullsAggregate,
    mtf::calculateNA,
    mtfFirstValueIgnoreNullsFinalize,
    mtfFirstValueIgnoreNullsCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfFirstValueIgnoreNullsEstimate( mtcNode     * aNode,
                                         mtcTemplate * aTemplate,
                                         mtcStack    * aStack,
                                         SInt          /* aRemain */,
                                         mtcCallBack * /*aCallBack */)
{
    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_DISTINCT_MASK ) == MTC_NODE_DISTINCT_TRUE,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aNode->baseTable  = aNode->arguments->baseTable;
    aNode->baseColumn = aNode->arguments->baseColumn;

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST_RAISE( aStack[1].column->module == &mtdList ||
                    aStack[1].column->module == &mtdBoolean,
                    ERR_CONVERSION_NOT_APPLICABLE );

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    mtc::initializeColumn( aStack[0].column, aStack[1].column );

    /* First Value 를 획득 했는지 않했는지를 저장 */
    IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                     & mtdBoolean,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_INVALID_FUNCTION_ARGUMENT ));
    }
    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_CONVERSION_NOT_APPLICABLE ));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfFirstValueIgnoreNullsInitialize( mtcNode     * aNode,
                                           mtcStack    * /* aStack */,
                                           SInt          /* aRemain */,
                                           void        * /* aInfo */,
                                           mtcTemplate * aTemplate )
{
    const mtcColumn * sColumn;
    void            * sValueTemp;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sValueTemp = ( void * )mtd::valueForModule( ( smiColumn * )sColumn,
                                                aTemplate->rows[aNode->table].row,
                                                MTD_OFFSET_USE,
                                                sColumn->module->staticNull );

    sColumn->module->null( sColumn, sValueTemp );

    *(mtdBooleanType*) ((UChar*)aTemplate->rows[aNode->table].row
                        + sColumn[1].column.offset) = MTD_BOOLEAN_FALSE;

    return IDE_SUCCESS;
}

IDE_RC mtfFirstValueIgnoreNullsAggregate( mtcNode     * aNode,
                                          mtcStack    * aStack,
                                          SInt          aRemain,
                                          void        * /* aInfo */,
                                          mtcTemplate * aTemplate )
{
    mtcNode         * sNode;
    mtdBooleanType    sIsSet;
    const mtcColumn * sColumn;
    const mtdModule * sModule;

    IDE_TEST_RAISE( aRemain < 2, ERR_STACK_OVERFLOW );

    sColumn      = aTemplate->rows[aNode->table].columns + aNode->column;
    sNode        = aNode->arguments;

    IDE_TEST( aTemplate->rows[sNode->table].
              execute[sNode->column].calculate(                         sNode,
                                                                   aStack + 1,
                                                                  aRemain - 1,
           aTemplate->rows[sNode->table].execute[sNode->column].calculateInfo,
                                                                    aTemplate )
              != IDE_SUCCESS );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack[0].value  = (void*)( (UChar*) aTemplate->rows[aNode->table].row
                                + aStack[0].column->column.offset );
    sModule          = aStack[0].column->module;

    sIsSet =  *(mtdBooleanType*) ( ( UChar * )aTemplate->rows[aNode->table].row
                                   + sColumn[1].column.offset );

    if (sIsSet == MTD_BOOLEAN_FALSE )
    {
        if( aStack[1].column->module->isNull( aStack[1].column,
                                              aStack[1].value ) != ID_TRUE )
        {
            idlOS::memcpy( aStack[0].value,
                           aStack[1].value,
                           sModule->actualSize( aStack[1].column,
                                                aStack[1].value ) );
            *(mtdBooleanType*) ((UChar*)aTemplate->rows[aNode->table].row
                                    + sColumn[1].column.offset) = MTD_BOOLEAN_TRUE;
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

    IDE_EXCEPTION( ERR_STACK_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_STACK_OVERFLOW ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfFirstValueIgnoreNullsFinalize( mtcNode     * /* aNode     */,
                                         mtcStack    * /* aStack    */,
                                         SInt          /* aRemain   */,
                                         void        * /* aInfo     */,
                                         mtcTemplate * /* aTemplate */ )
{
    return IDE_SUCCESS;
}

IDE_RC mtfFirstValueIgnoreNullsCalculate( mtcNode     * aNode,
                                          mtcStack    * aStack,
                                          SInt          /* aRemain */,
                                          void        * /* aInfo */,
                                          mtcTemplate * aTemplate )
{
    aStack->column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack->value  = ( void * )( ( UChar * )aTemplate->rows[aNode->table].row
                                 + aStack->column->column.offset );
    return IDE_SUCCESS;
}
