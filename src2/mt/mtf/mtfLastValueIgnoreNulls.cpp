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
 * $Id: mtfLastValueIgnoreNulls.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>

extern mtfModule mtfLastValueIgnoreNulls;

extern mtdModule mtdList;
extern mtdModule mtdBoolean;

static mtcName mtfLastValueIgnoreNullsName[1] = {
    { NULL,  23, ( void * )"LAST_VALUE_IGNORE_NULLS" }
};

static IDE_RC mtfLastValueIgnoreNullsEstimate( mtcNode     * aNode,
                                               mtcTemplate * aTemplate,
                                               mtcStack    * aStack,
                                               SInt          aRemain,
                                               mtcCallBack * aCallBack );

mtfModule mtfLastValueIgnoreNulls = {
    1 | MTC_NODE_OPERATOR_AGGREGATION  |
        MTC_NODE_FUNCTION_ANALYTIC_TRUE |
        MTC_NODE_FUNCTION_WINDOWING_TRUE,
    ~( MTC_NODE_INDEX_MASK ),
    1.0, /* default selectivity ( 비교 연산자가 아님 ) */
    mtfLastValueIgnoreNullsName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfLastValueIgnoreNullsEstimate
};

IDE_RC mtfLastValueIgnoreNullsInitialize( mtcNode     * aNode,
                                          mtcStack    * aStack,
                                          SInt          aRemain,
                                          void        * aInfo,
                                          mtcTemplate * aTemplate );

IDE_RC mtfLastValueIgnoreNullsAggregate( mtcNode     * aNode,
                                         mtcStack    * aStack,
                                         SInt          aRemain,
                                         void        * aInfo,
                                         mtcTemplate * aTemplate );

IDE_RC mtfLastValueIgnoreNullsFinalize( mtcNode     * aNode,
                                        mtcStack    * aStack,
                                        SInt          aRemain,
                                        void        * aInfo,
                                        mtcTemplate * aTemplate );

IDE_RC mtfLastValueIgnoreNullsCalculate( mtcNode     * aNode,
                                         mtcStack    * aStack,
                                         SInt          aRemain,
                                         void        * aInfo,
                                         mtcTemplate * aTemplate );

static const mtcExecute mtfExecute = {
    mtfLastValueIgnoreNullsInitialize,
    mtfLastValueIgnoreNullsAggregate,
    mtf::calculateNA,
    mtfLastValueIgnoreNullsFinalize,
    mtfLastValueIgnoreNullsCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfLastValueIgnoreNullsEstimate( mtcNode     * aNode,
                                        mtcTemplate * aTemplate,
                                        mtcStack    * aStack,
                                        SInt          /* aRemain */,
                                        mtcCallBack * /* aCallBack */ )
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

IDE_RC mtfLastValueIgnoreNullsInitialize( mtcNode     * aNode,
                                          mtcStack    * /* aStack  */,
                                          SInt          /* aRemain */,
                                          void        * /* aInfo   */,
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

    return IDE_SUCCESS;
}

IDE_RC mtfLastValueIgnoreNullsAggregate( mtcNode     * aNode,
                                         mtcStack    * aStack,
                                         SInt          aRemain,
                                         void        * /* aInfo */,
                                         mtcTemplate * aTemplate )
{
    mtcNode         * sNode;
    const mtdModule * sModule;

    IDE_TEST_RAISE( aRemain < 2, ERR_STACK_OVERFLOW );

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

    if( aStack[1].column->module->isNull( aStack[1].column,
                                          aStack[1].value ) != ID_TRUE )
    {
        idlOS::memcpy( aStack[0].value,
                       aStack[1].value,
                       sModule->actualSize( aStack[1].column,
                                            aStack[1].value ) );
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

IDE_RC mtfLastValueIgnoreNullsFinalize( mtcNode     * /* aNode     */,
                                        mtcStack    * /* aStack    */,
                                        SInt          /* aRemain   */,
                                        void        * /* aInfo     */,
                                        mtcTemplate * /* aTemplate */ )
{
    return IDE_SUCCESS;
}

IDE_RC mtfLastValueIgnoreNullsCalculate( mtcNode     * aNode,
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
