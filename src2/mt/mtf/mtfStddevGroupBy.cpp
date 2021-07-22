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
 * $Id: mtfStddevGroupBy.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>

extern mtfModule mtfStddevGroupBy;

extern mtdModule mtdDouble;
extern mtdModule mtdBigint;
extern mtdModule mtdList;
extern mtdModule mtdBoolean;

static mtcName mtfStddevGroupByFunctionName[1] = {
    { NULL, 13, (void*)"STDDEVGROUPBY" }
};

static IDE_RC mtfStddevGroupByEstimate( mtcNode*     aNode,
                                        mtcTemplate* aTemplate,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        mtcCallBack* aCallBack );

mtfModule mtfStddevGroupBy = {
    4|MTC_NODE_OPERATOR_AGGREGATION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfStddevGroupByFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfStddevGroupByEstimate
};

IDE_RC mtfStddevGroupByInitialize( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate );

IDE_RC mtfStddevGroupByAggregate( mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*        aInfo,
                                  mtcTemplate* aTemplate );

IDE_RC mtfStddevGroupByFinalize( mtcNode*     aNode,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 void*        aInfo,
                                 mtcTemplate* aTemplate );

IDE_RC mtfStddevGroupByCalculate( mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*        aInfo,
                                  mtcTemplate* aTemplate );

static const mtcExecute mtfExecute = {
    mtfStddevGroupByInitialize,
    mtfStddevGroupByAggregate,
    mtf::calculateNA,
    mtfStddevGroupByFinalize,
    mtfStddevGroupByCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfStddevGroupByEstimate( mtcNode*     aNode,
                                 mtcTemplate* aTemplate,
                                 mtcStack*    aStack,
                                 SInt      /* aRemain */,
                                 mtcCallBack* aCallBack )
{
    static const mtdModule* sModules[1] = {
        &mtdDouble
    };

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    /*
    IDE_TEST( mtdDouble.estimate( aStack[0].column + 0, 0, 0, 0 )
              != IDE_SUCCESS );
    IDE_TEST( mtdDouble.estimate( aStack[0].column + 1, 0, 0, 0 )
              != IDE_SUCCESS );
    IDE_TEST( mtdDouble.estimate( aStack[0].column + 2, 0, 0, 0 )
              != IDE_SUCCESS );
    IDE_TEST( mtdBigint.estimate( aStack[0].column + 3, 0, 0, 0 )
              != IDE_SUCCESS );
    */
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

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfStddevGroupByInitialize( mtcNode*     aNode,
                                   mtcStack*,
                                   SInt,
                                   void*,
                                   mtcTemplate* aTemplate )
{
    const mtcColumn* sColumn;
    
    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    
    *(mtdDoubleType*)
        ( (UChar*) aTemplate->rows[aNode->table].row
          + sColumn[0].column.offset) = 0;
    *(mtdDoubleType*)
        ( (UChar*) aTemplate->rows[aNode->table].row
          + sColumn[1].column.offset) = 0;
    *(mtdDoubleType*)
        ( (UChar*) aTemplate->rows[aNode->table].row
          + sColumn[2].column.offset) = 0;
    *(mtdBigintType*)
        ( (UChar*) aTemplate->rows[aNode->table].row
          + sColumn[3].column.offset) = 0;
    
    return IDE_SUCCESS;
}

IDE_RC mtfStddevGroupByAggregate( mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*,
                                  mtcTemplate* aTemplate )
{
    const mtdModule* sModule;
    mtcNode*         sNode;
    const mtcColumn* sColumn;
    
    // BUG-33674
    IDE_TEST_RAISE( aRemain < 1, ERR_STACK_OVERFLOW );

    sNode  = aNode->arguments;
    IDE_TEST( aTemplate->rows[sNode->table].
              execute[sNode->column].calculate(                         sNode,
                                                                       aStack,
                                                                      aRemain,
           aTemplate->rows[sNode->table].execute[sNode->column].calculateInfo,
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
    
    sModule = aStack->column->module;
    if( sModule->isNull( aStack[0].column,
                         aStack[0].value ) != ID_TRUE )
    {
        sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
/*        
        *(mtdDoubleType*)((UChar*)aTuple->row+sColumn[1].column.offset) += 
           *(mtdDoubleType*)aStack[0].value * *(mtdDoubleType*)aStack[0].value;
        *(mtdDoubleType*)((UChar*)aTuple->row+sColumn[2].column.offset) += 
                                              *(mtdDoubleType*)aStack[0].value;
        *(mtdBigintType*)((UChar*)aTuple->row+sColumn[3].column.offset) += 1;
*/
        *(mtdDoubleType*)
            ( (UChar*) aTemplate->rows[aNode->table].row
              + sColumn[1].column.offset) += *(mtdDoubleType*)aStack[0].value;
        *(mtdDoubleType*)
            ( (UChar*) aTemplate->rows[aNode->table].row
              + sColumn[2].column.offset) +=
            *(mtdDoubleType*)
            ( (SChar*) (aStack[0].value) - aStack[0].column->column.offset
              + ((mtcColumn*)(aStack[0].column)+1)->column.offset);
        *(mtdBigintType*)
            ( (UChar*) aTemplate->rows[aNode->table].row
              + sColumn[3].column.offset) +=
            *(mtdBigintType*)
            ( (SChar*) (aStack[0].value) - aStack[0].column->column.offset
              + ((mtcColumn*) (aStack[0].column)+2)->column.offset);
        
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfStddevGroupByFinalize( mtcNode*     aNode,
                                 mtcStack*,
                                 SInt,
                                 void*,
                                 mtcTemplate* aTemplate )
{
    const mtcColumn* sColumn;
    mtdBigintType    sCount;
    mtdDoubleType    sPow;
    mtdDoubleType    sSum;
    void*            sValueTemp;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    
    sCount = *(mtdBigintType*)
        ( (UChar*) aTemplate->rows[aNode->table].row+sColumn[3].column.offset);
    if( sCount == 0 )
    {
        sValueTemp = (void*)mtd::valueForModule(
                                 (smiColumn*)sColumn + 0,
                                 aTemplate->rows[aNode->table].row,
                                 MTD_OFFSET_USE,
                                 sColumn->module->staticNull );

        sColumn[0].module->null( sColumn + 0,
                                 sValueTemp );
    }
    else if( sCount == 1 )
    {
        *(mtdDoubleType*)
            ( (UChar*) aTemplate->rows[aNode->table].row
              + sColumn[0].column.offset) = 0;
    }
    else
    {
        sPow = *(mtdDoubleType*)
            ( (UChar*) aTemplate->rows[aNode->table].row
              + sColumn[1].column.offset);
        sSum = *(mtdDoubleType*)
            ( (UChar*) aTemplate->rows[aNode->table].row
              + sColumn[2].column.offset);
        *(mtdDoubleType*)
            ( (UChar*) aTemplate->rows[aNode->table].row
              + sColumn[0].column.offset) =
            idlOS::sqrt( idlOS::fabs( ( sPow - sSum * sSum / sCount ) / ( sCount - 1 ) ) );
    }
    
    return IDE_SUCCESS;
}

IDE_RC mtfStddevGroupByCalculate( mtcNode*     aNode,
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
 
