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
 

/*******************************************************************************
* $Id: mtfVarPop.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
*******************************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>

extern mtfModule mtfVarPop;

extern mtdModule mtdDouble;
extern mtdModule mtdBigint;
extern mtdModule mtdList;
extern mtdModule mtdBoolean;

static mtcName mtfVarPopFunctionName[1] =
{
    { NULL, 7, (void*)"VAR_POP" }
};

static IDE_RC mtfVarPopEstimate( mtcNode*     aNode,
                                 mtcTemplate* aTemplate,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 mtcCallBack* aCallBack );

mtfModule mtfVarPop =
{
    4 | MTC_NODE_OPERATOR_AGGREGATION | MTC_NODE_FUNCTION_WINDOWING_TRUE,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  /* default selectivity (비교 연산자가 아님) */
    mtfVarPopFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfVarPopEstimate
};

IDE_RC mtfVarPopInitialize( mtcNode*     aNode,
                            mtcStack*    aStack,
                            SInt         aRemain,
                            void*        aInfo,
                            mtcTemplate* aTemplate );

IDE_RC mtfVarPopAggregate( mtcNode*     aNode,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           void*        aInfo,
                           mtcTemplate* aTemplate );

IDE_RC mtfVarPopMerge( mtcNode*     aNode,
                       mtcStack*    aStack,
                       SInt         aRemain,
                       void*        aInfo,
                       mtcTemplate* aTemplate );

IDE_RC mtfVarPopFinalize( mtcNode*     aNode,
                          mtcStack*    aStack,
                          SInt         aRemain,
                          void*        aInfo,
                          mtcTemplate* aTemplate );

IDE_RC mtfVarPopCalculate( mtcNode*     aNode,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           void*        aInfo,
                           mtcTemplate* aTemplate );

static const mtcExecute mtfExecute =
{
    mtfVarPopInitialize,
    mtfVarPopAggregate,
    mtfVarPopMerge,
    mtfVarPopFinalize,
    mtfVarPopCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfVarPopEstimate( mtcNode*     aNode,
                          mtcTemplate* aTemplate,
                          mtcStack*    aStack,
                          SInt,
                          mtcCallBack* aCallBack )
{
    static const mtdModule* sModules[1] =
    {
        &mtdDouble
    };

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_DISTINCT_MASK ) ==
                    MTC_NODE_DISTINCT_TRUE,
                    ERR_ARGUMENT_NOT_APPLICABLE );

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )

              != IDE_SUCCESS );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column + 0,
                                     & mtdDouble,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                     & mtdDouble,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    IDE_TEST( mtc::initializeColumn( aStack[0].column + 2,
                                     & mtdDouble,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    IDE_TEST( mtc::initializeColumn( aStack[0].column + 3,
                                     & mtdBigint,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( aStack[1].column->module == &mtdList ||
                    aStack[1].column->module == &mtdBoolean,
                    ERR_CONVERSION_NOT_APPLICABLE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));

    IDE_EXCEPTION( ERR_ARGUMENT_NOT_APPLICABLE );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_ARGUMENT_NOT_APPLICABLE));

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfVarPopInitialize( mtcNode*     aNode,
                            mtcStack*,
                            SInt,
                            void*,
                            mtcTemplate* aTemplate )
{
    const mtcColumn * sColumn = NULL;
    UChar           * sRow    = NULL;
    
    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sRow    = (UChar *)aTemplate->rows[aNode->table].row;
    
    *(mtdDoubleType*)( sRow + sColumn[0].column.offset ) = 0;
    *(mtdDoubleType*)( sRow + sColumn[1].column.offset ) = 0;
    *(mtdDoubleType*)( sRow + sColumn[2].column.offset ) = 0;
    *(mtdBigintType*)( sRow + sColumn[3].column.offset ) = 0;
    
    return IDE_SUCCESS;
}

IDE_RC mtfVarPopAggregate( mtcNode*     aNode,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           void*,
                           mtcTemplate* aTemplate )
{
    const mtdModule * sModule;
    mtcNode         * sNode;
    const mtcColumn * sColumn;
    mtcExecute      * sExecute;
    UChar           * sRow;
    
    IDE_TEST_RAISE( aRemain < 1, ERR_STACK_OVERFLOW );

    sNode    = aNode->arguments;
    sExecute = &aTemplate->rows[sNode->table].execute[sNode->column];

    IDE_TEST( sExecute->calculate( sNode,
                                   aStack,
                                   aRemain,
                                   sExecute->calculateInfo,
                                   aTemplate )
              != IDE_SUCCESS );
    
    if( sNode->conversion != NULL )
    {
        IDE_TEST( mtf::convertCalculate( sNode,
                                         aStack,
                                         aRemain,
                                         NULL,
                                         aTemplate )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }
    
    sModule = aStack->column->module;
    if( sModule->isNull( aStack[0].column, aStack[0].value ) != ID_TRUE )
    {
        sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
        sRow    = (UChar *)aTemplate->rows[aNode->table].row;

        *(mtdDoubleType *)( sRow + sColumn[1].column.offset ) +=
            ( *(mtdDoubleType*) aStack[0].value ) * ( *(mtdDoubleType*)aStack[0].value );

        *(mtdDoubleType *)( sRow + sColumn[2].column.offset ) +=
            *(mtdDoubleType*)aStack[0].value;

        *(mtdBigintType *)( sRow + sColumn[3].column.offset ) += 1;
    }
    else
    {
        /* Nothing to do */
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfVarPopMerge( mtcNode*     aNode,
                       mtcStack*    ,
                       SInt         ,
                       void*        aInfo,
                       mtcTemplate* aTemplate )
{
    const mtcColumn * sColumn = NULL;
    UChar           * sDstRow = NULL;
    UChar           * sSrcRow = NULL;

    sDstRow = (UChar*)aTemplate->rows[aNode->table].row;
    sSrcRow = (UChar*)aInfo;
    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;

    /* Pow */
    *(mtdDoubleType*)(sDstRow + sColumn[1].column.offset) +=
    *(mtdDoubleType*)(sSrcRow + sColumn[1].column.offset);

    /* Sum */
    *(mtdDoubleType*)(sDstRow + sColumn[2].column.offset) +=
    *(mtdDoubleType*)(sSrcRow + sColumn[2].column.offset);

    /* Count */
    *(mtdBigintType*)(sDstRow + sColumn[3].column.offset) +=
    *(mtdBigintType*)(sSrcRow + sColumn[3].column.offset);

    return IDE_SUCCESS;
}

IDE_RC mtfVarPopFinalize( mtcNode*     aNode,
                          mtcStack*,
                          SInt,
                          void*,
                          mtcTemplate* aTemplate )
{
    const mtcColumn * sColumn;
    mtdBigintType     sCount;
    mtdDoubleType     sPow;
    mtdDoubleType     sSum;
    void            * sValueTemp;
    UChar           * sRow;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sRow    = (UChar *)aTemplate->rows[aNode->table].row;
    sCount  = *(mtdBigintType *)( sRow + sColumn[3].column.offset );

    if( sCount < 1 )
    {
        sValueTemp = (void*)mtd::valueForModule( 
                                 (smiColumn*)sColumn + 0,
                                 (void *)sRow,
                                 MTD_OFFSET_USE,
                                 sColumn->module->staticNull );

        sColumn[0].module->null( sColumn + 0, sValueTemp );
    }
    else
    {
        sPow = *(mtdDoubleType*)( sRow + sColumn[1].column.offset );
        sSum = *(mtdDoubleType*)( sRow + sColumn[2].column.offset );

        *(mtdDoubleType*)( sRow + sColumn[0].column.offset ) =
            idlOS::fabs( ( sPow - sSum * sSum / sCount ) / ( sCount ) );
    }
    
    return IDE_SUCCESS;
}

IDE_RC mtfVarPopCalculate( mtcNode*     aNode,
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
