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
 * $Id: mtfNtile.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>

extern mtfModule mtfNtile;

extern mtdModule mtdList;
extern mtdModule mtdBoolean;
extern mtdModule mtdBigint;

static mtcName mtfNtileName[1] = {
    { NULL,  5, ( void * )"NTILE" }
};

static IDE_RC mtfNtileEstimate( mtcNode     * aNode,
                               mtcTemplate * aTemplate,
                               mtcStack    * aStack,
                               SInt          aRemain,
                               mtcCallBack * aCallBack );

mtfModule mtfNtile = {
    1 | MTC_NODE_OPERATOR_AGGREGATION  |
        MTC_NODE_FUNCTION_ANALYTIC_TRUE |
        MTC_NODE_FUNCTION_RANKING_TRUE,
    ~( MTC_NODE_INDEX_MASK ),
    1.0, /* default selectivity ( 비교 연산자가 아님 ) */
    mtfNtileName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfNtileEstimate
};

IDE_RC mtfNtileInitialize( mtcNode     * aNode,
                           mtcStack    * aStack,
                           SInt          aRemain,
                           void        * aInfo,
                           mtcTemplate * aTemplate );

IDE_RC mtfNtileAggregate( mtcNode     * aNode,
                          mtcStack    * aStack,
                          SInt          aRemain,
                          void        * aInfo,
                          mtcTemplate * aTemplate );

IDE_RC mtfNtileFinalize( mtcNode     * aNode,
                         mtcStack    * aStack,
                         SInt          aRemain,
                         void        * aInfo,
                         mtcTemplate * aTemplate );

IDE_RC mtfNtileCalculate( mtcNode     * aNode,
                          mtcStack    * aStack,
                          SInt          aRemain,
                          void        * aInfo,
                          mtcTemplate * aTemplate );

static const mtcExecute mtfExecute = {
    mtfNtileInitialize,
    mtfNtileAggregate,
    mtf::calculateNA,
    mtfNtileFinalize,
    mtfNtileCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfNtileEstimate( mtcNode     * aNode,
                         mtcTemplate * aTemplate,
                         mtcStack    * aStack,
                         SInt          /* aRemain */,
                         mtcCallBack * aCallBack )
{
    const mtdModule * sModules[1] = { &mtdBigint };

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_DISTINCT_MASK ) == MTC_NODE_DISTINCT_TRUE,
                    ERR_ARGUMENT_NOT_APPLICABLE );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST_RAISE( ( aStack[1].column->module == &mtdList ) ||
                    ( aStack[1].column->module == &mtdBoolean ),
                    ERR_ARGUMENT_NOT_APPLICABLE );

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
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
    IDE_EXCEPTION( ERR_ARGUMENT_NOT_APPLICABLE  );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_ARGUMENT_NOT_APPLICABLE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfNtileInitialize( mtcNode     * aNode,
                           mtcStack    * /* aStack  */,
                           SInt          /* aRemain */,
                           void        * /* aInfo   */,
                           mtcTemplate * aTemplate )
{
    const mtcColumn * sColumn;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;

    *(mtdBigintType*)((UChar*)aTemplate->rows[aNode->table].row +
                      sColumn->column.offset) = MTD_BIGINT_NULL;

    return IDE_SUCCESS;
}

IDE_RC mtfNtileAggregate( mtcNode     * aNode,
                          mtcStack    * /* aStack  */,
                          SInt          /* aRemain */,
                          void        * /* aInfo   */,
                          mtcTemplate * aTemplate )
{
    const mtcColumn * sColumn;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;

    if ( *(mtdBigintType *)((UChar*)aTemplate->rows[aNode->table].row +
                            sColumn->column.offset )
         == MTD_BIGINT_NULL )
    {
        *(mtdBigintType*)((UChar*) aTemplate->rows[aNode->table].row +
                          sColumn->column.offset) = 1;
    }
    else
    {
        *(mtdBigintType*)((UChar*) aTemplate->rows[aNode->table].row +
                          sColumn->column.offset) += 1;
    }

    return IDE_SUCCESS;
}

IDE_RC mtfNtileFinalize( mtcNode     * /* aNode   */,
                         mtcStack    * /* aStack  */,
                         SInt          /* aRemain */,
                         void        * /* aInfo   */,
                         mtcTemplate * /*aTemplate*/ )
{
    return IDE_SUCCESS;
}

IDE_RC mtfNtileCalculate( mtcNode     * aNode,
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
