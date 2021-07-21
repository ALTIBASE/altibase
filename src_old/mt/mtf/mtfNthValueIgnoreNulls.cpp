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
 * $Id: mtfNthValueIgnoreNulls.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>

extern mtfModule mtfNthValueIgnoreNulls;

extern mtdModule mtdList;
extern mtdModule mtdBoolean;
extern mtdModule mtdBigint;

#define MTF_NTH_VALUE_IGNORE_NULLS_INITIALIZE (-1)
#define MTF_NTH_VALUE_IGNORE_NULLS_FIANLIZE   (-2)

static mtcName mtfNthValueName[1] = {
    { NULL,  22, ( void * )"NTH_VALUE_IGNORE_NULLS" }
};

static IDE_RC mtfNthValueIgnoreNullsEstimate( mtcNode     * aNode,
                                              mtcTemplate * aTemplate,
                                              mtcStack    * aStack,
                                              SInt          aRemain,
                                              mtcCallBack * aCallBack );

mtfModule mtfNthValueIgnoreNulls = {
    2 | MTC_NODE_OPERATOR_AGGREGATION  |
        MTC_NODE_FUNCTION_ANALYTIC_TRUE |
        MTC_NODE_FUNCTION_WINDOWING_TRUE,
    ~( MTC_NODE_INDEX_MASK ),
    1.0, /* default selectivity ( 비교 연산자가 아님 ) */
    mtfNthValueName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfNthValueIgnoreNullsEstimate
};

IDE_RC mtfNthValueIgnoreNullsInitialize( mtcNode     * aNode,
                                         mtcStack    * aStack,
                                         SInt          aRemain,
                                         void        * aInfo,
                                         mtcTemplate * aTemplate );

IDE_RC mtfNthValueIgnoreNullsAggregate( mtcNode     * aNode,
                                        mtcStack    * aStack,
                                        SInt          aRemain,
                                        void        * aInfo,
                                        mtcTemplate * aTemplate );

IDE_RC mtfNthValueIgnoreNullsFinalize( mtcNode     * aNode,
                                       mtcStack    * aStack,
                                       SInt          aRemain,
                                       void        * aInfo,
                                       mtcTemplate * aTemplate );

IDE_RC mtfNthValueIgnoreNullsCalculate( mtcNode     * aNode,
                                        mtcStack    * aStack,
                                        SInt          aRemain,
                                        void        * aInfo,
                                        mtcTemplate * aTemplate );

static const mtcExecute mtfExecute = {
    mtfNthValueIgnoreNullsInitialize,
    mtfNthValueIgnoreNullsAggregate,
    mtf::calculateNA,
    mtfNthValueIgnoreNullsFinalize,
    mtfNthValueIgnoreNullsCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfNthValueIgnoreNullsEstimate( mtcNode     * aNode,
                                       mtcTemplate * aTemplate,
                                       mtcStack    * aStack,
                                       SInt          /* aRemain */,
                                       mtcCallBack * aCallBack )
{
    mtcNode         * sMtc;
    const mtdModule * sModules[1] = { &mtdBigint };

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 2,
                      ERR_INVALID_FUNCTION_ARGUMENT );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_DISTINCT_MASK ) == MTC_NODE_DISTINCT_TRUE,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST_RAISE( aStack[1].column->module == &mtdList ||
                    aStack[1].column->module == &mtdBoolean,
                    ERR_CONVERSION_NOT_APPLICABLE );

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;
    aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo = NULL;

    mtc::initializeColumn( aStack[0].column, aStack[1].column );

    sMtc = aNode->arguments;

    IDE_TEST_RAISE( ( aTemplate->rows[sMtc->next->table].lflag & MTC_TUPLE_TYPE_MASK )
                    != MTC_TUPLE_TYPE_CONSTANT,
                    ERR_ARGUMENT_NOT_APPLICABLE );

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        sMtc->next,
                                        aTemplate,
                                        aStack + 2,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    /* 내부 Counting 을 위한 컬럼 */
    IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                     &mtdBigint,
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
    IDE_EXCEPTION(ERR_ARGUMENT_NOT_APPLICABLE  );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_ARGUMENT_NOT_APPLICABLE));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfNthValueIgnoreNullsInitialize( mtcNode     * aNode,
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

    *(mtdBigintType*) ((UChar*)aTemplate->rows[aNode->table].row
                        + sColumn[1].column.offset) = MTF_NTH_VALUE_IGNORE_NULLS_INITIALIZE;

    return IDE_SUCCESS;
}

IDE_RC mtfNthValueIgnoreNullsAggregate( mtcNode     * aNode,
                                        mtcStack    * aStack,
                                        SInt          aRemain,
                                        void        * /* aInfo */,
                                        mtcTemplate * aTemplate )
{
    const mtcColumn * sColumn;
    const mtdModule * sModule;
    SLong             sCount;
    SLong             sNth;
    idBool            sIsNull;

    IDE_TEST_RAISE( aRemain < 2, ERR_STACK_OVERFLOW );

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     NULL,
                                     aTemplate )
              != IDE_SUCCESS );

    sCount = (SLong)(*(mtdBigintType*) ((UChar*)aTemplate->rows[aNode->table].row
                                        + sColumn[1].column.offset));

    sIsNull = aStack[1].column->module->isNull( aStack[1].column,
                                                aStack[1].value );

    if ( sCount == MTF_NTH_VALUE_IGNORE_NULLS_INITIALIZE )
    {
        sNth = (SLong)(*(mtdBigintType*)aStack[2].value);

        IDE_TEST_RAISE( sNth < 1, ERR_ARGUMENT4_VALUE_OUT_OF_RANGE );

        if ( sIsNull == ID_TRUE )
        {
            sCount = sNth + 1;
        }
        else
        {
            sCount = sNth;
        }
        *(mtdBigintType*) ((UChar*)aTemplate->rows[aNode->table].row
                            + sColumn[1].column.offset) = sCount;
    }
    else
    {
        if ( sIsNull != ID_TRUE )
        {
            if ( sCount > 0 )
            {
                --sCount;
                *(mtdBigintType*) ((UChar*)aTemplate->rows[aNode->table].row
                                    + sColumn[1].column.offset) = sCount;
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

    if ( sCount == 1 )
    {
        aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;
        aStack[0].value  = (void*)( (UChar*) aTemplate->rows[aNode->table].row
                                    + aStack[0].column->column.offset );

        if ( sIsNull != ID_TRUE )
        {
            sModule = aStack[0].column->module;

            idlOS::memcpy( aStack[0].value,
                           aStack[1].value,
                           sModule->actualSize( aStack[1].column,
                                                aStack[1].value ) );
            *(mtdBigintType*) ((UChar*)aTemplate->rows[aNode->table].row
                                + sColumn[1].column.offset) = MTF_NTH_VALUE_IGNORE_NULLS_FIANLIZE;
        }
        else
        {
            *(mtdBigintType*) ((UChar*)aTemplate->rows[aNode->table].row
                                + sColumn[1].column.offset) = sCount + 1;
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
    IDE_EXCEPTION( ERR_ARGUMENT4_VALUE_OUT_OF_RANGE );
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_ARGUMENT_VALUE_OUT_OF_RANGE,
                                  sNth ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfNthValueIgnoreNullsFinalize( mtcNode     * /* aNode     */,
                                       mtcStack    * /* aStack    */,
                                       SInt          /* aRemain   */,
                                       void        * /* aInfo     */,
                                       mtcTemplate * /* aTemplate */ )
{
    return IDE_SUCCESS;
}

IDE_RC mtfNthValueIgnoreNullsCalculate( mtcNode     * aNode,
                                        mtcStack    * aStack,
                                        SInt          /* aRemain */,
                                        void        * /* aInfo   */,
                                        mtcTemplate * aTemplate )
{
    aStack->column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack->value  = ( void * )( ( UChar * )aTemplate->rows[aNode->table].row
                                 + aStack->column->column.offset );

    return IDE_SUCCESS;
}
