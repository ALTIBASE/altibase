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
 * $Id$
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>

extern mtfModule mtfRank;

extern mtdModule mtdBigint;

static mtcName mtfRankFunctionName[1] = {
    { NULL, 4, (void*)"RANK" }
};

static IDE_RC mtfRankEstimate( mtcNode*     aNode,
                               mtcTemplate* aTemplate,
                               mtcStack*    aStack,
                               SInt         aRemain,
                               mtcCallBack* aCallBack );

mtfModule mtfRank = {
    2 | MTC_NODE_OPERATOR_AGGREGATION  |
        MTC_NODE_FUNCTION_ANALYTIC_TRUE |
        MTC_NODE_FUNCTION_RANKING_TRUE,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfRankFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfRankEstimate
};

IDE_RC mtfRankInitialize(  mtcNode*     aNode,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           void*        aInfo,
                           mtcTemplate* aTemplate );

IDE_RC mtfRankAggregate(  mtcNode*     aNode,
                          mtcStack*    aStack,
                          SInt         aRemain,
                          void*        aInfo,
                          mtcTemplate* aTemplate );

IDE_RC mtfRankFinalize(  mtcNode*     aNode,
                         mtcStack*    aStack,
                         SInt         aRemain,
                         void*        aInfo,
                         mtcTemplate* aTemplate );

IDE_RC mtfRankCalculate(  mtcNode*     aNode,
                          mtcStack*    aStack,
                          SInt         aRemain,
                          void*        aInfo,
                          mtcTemplate* aTemplate );

static const mtcExecute mtfExecute = {
    mtfRankInitialize,
    mtfRankAggregate,
    mtf::calculateNA,
    mtfRankFinalize,
    mtfRankCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfRankEstimate( mtcNode*     aNode,
                        mtcTemplate* aTemplate,
                        mtcStack*    aStack,
                        SInt,
                        mtcCallBack* /*aCallBack*/ )
{
    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 0,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    //IDE_TEST( mtdBigint.estimate( aStack[0].column, 0, 0, 0 )
    //          != IDE_SUCCESS );
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdBigint,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                     & mtdBigint,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfRankInitialize( mtcNode*     aNode,
                          mtcStack*,
                          SInt,
                          void*,
                          mtcTemplate* aTemplate )
{
    const mtcColumn* sColumn;
    
    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    
    *(mtdBigintType*)
        ( (UChar*) aTemplate->rows[aNode->table].row
          + sColumn[0].column.offset) = 0;

    *(mtdBigintType*)
        ( (UChar*) aTemplate->rows[aNode->table].row
          + sColumn[1].column.offset) = 0;
    
    return IDE_SUCCESS;
}

IDE_RC mtfRankAggregate( mtcNode*     aNode,
                         mtcStack*,
                         SInt,
                         void*        aInfo,
                         mtcTemplate* aTemplate )
{
    const mtcColumn  * sColumn;
    mtcRankValueType   sRankValue;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST_RAISE( aInfo == NULL, ERR_INVALID_FUNCTION_ARGUMENT );
    sRankValue = *(mtcRankValueType*)aInfo;

    // row_number
    *(mtdBigintType*)
        ( (UChar*) aTemplate->rows[aNode->table].row
          + sColumn[1].column.offset) += 1;

    // rank
    if ( ( sRankValue == MTC_RANK_VALUE_FIRST ) ||
         ( sRankValue == MTC_RANK_VALUE_DIFF ) )
    {
        *(mtdBigintType*)
            ( (UChar*) aTemplate->rows[aNode->table].row
              + sColumn[0].column.offset) =
            *(mtdBigintType*)
            ( (UChar*) aTemplate->rows[aNode->table].row
              + sColumn[1].column.offset);
    }
    else
    {
        // Nothing to do.
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfRankFinalize( mtcNode*,
                        mtcStack*,
                        SInt,
                        void*,
                        mtcTemplate* )
{
    return IDE_SUCCESS;
}

IDE_RC mtfRankCalculate( mtcNode*     aNode,
                         mtcStack*    aStack,
                         SInt,
                         void*,
                         mtcTemplate* aTemplate )
{
    aStack->column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack->value  = (void*)( (UChar*) aTemplate->rows[aNode->table].row
                              + aStack->column->column.offset );
    
    return IDE_SUCCESS;
}
