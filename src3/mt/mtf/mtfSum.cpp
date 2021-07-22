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
 * $Id: mtfSum.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>
#include <mtuProperty.h>

extern mtfModule mtfSum;

extern mtdModule mtdFloat;
extern mtdModule mtdDouble;
extern mtdModule mtdBigint;
extern mtdModule mtdList;
extern mtdModule mtdBoolean;

static mtcName mtfSumFunctionName[1] = {
    { NULL, 3, (void*)"SUM" }
};

static IDE_RC mtfSumInitialize( void );

static IDE_RC mtfSumFinalize( void );

static IDE_RC mtfSumEstimate( mtcNode*     aNode,
                              mtcTemplate* aTemplate,
                              mtcStack*    aStack,
                              SInt         aRemain,
                              mtcCallBack* aCallBack );

mtfModule mtfSum = {
    2|MTC_NODE_OPERATOR_AGGREGATION|MTC_NODE_FUNCTION_WINDOWING_TRUE,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfSumFunctionName,
    NULL,
    mtfSumInitialize,
    mtfSumFinalize,
    mtfSumEstimate
};

static IDE_RC mtfSumEstimateFloat( mtcNode*     aNode,
                                   mtcTemplate* aTemplate,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   mtcCallBack* aCallBack );

static IDE_RC mtfSumEstimateDouble( mtcNode*     aNode,
                                    mtcTemplate* aTemplate,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    mtcCallBack* aCallBack );

static IDE_RC mtfSumEstimateBigint( mtcNode*     aNode,
                                    mtcTemplate* aTemplate,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    mtcCallBack* aCallBack );

static mtfSubModule mtfXX[1] = {
    { NULL, mtf::estimateNA }
};

static mtfSubModule mtfSumEstimates[3] = {
    { mtfSumEstimates+1, mtfSumEstimateDouble },
    { mtfSumEstimates+2, mtfSumEstimateBigint },
    { NULL,              mtfSumEstimateFloat }
};

// BUG-41994
// high performance용 group table
static mtfSubModule mtfSumEstimatesHighPerformance[2] = {
    { mtfSumEstimatesHighPerformance+1, mtfSumEstimateDouble },
    { NULL,                             mtfSumEstimateBigint }
};

static mtfSubModule** mtfTable = NULL;
static mtfSubModule** mtfTableHighPerformance = NULL;

IDE_RC mtfSumInitialize( void )
{
    IDE_TEST( mtf::initializeTemplate( &mtfTable,
                                       mtfSumEstimates,
                                       mtfXX )
              != IDE_SUCCESS );
              
    IDE_TEST( mtf::initializeTemplate( &mtfTableHighPerformance,
                                       mtfSumEstimatesHighPerformance,
                                       mtfXX )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfSumFinalize( void )
{
    IDE_TEST( mtf::finalizeTemplate( &mtfTable )
              != IDE_SUCCESS );

    IDE_TEST( mtf::finalizeTemplate( &mtfTableHighPerformance )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfSumEstimate( mtcNode*     aNode,
                       mtcTemplate* aTemplate,
                       mtcStack*    aStack,
                       SInt         aRemain,
                       mtcCallBack* aCallBack )
{
    const mtfSubModule  * sSubModule;
    mtfSubModule       ** sTable;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    // BUG-41994
    aTemplate->arithmeticOpModeRef = ID_TRUE;
    if ( aTemplate->arithmeticOpMode == MTC_ARITHMETIC_OPERATION_PERFORMANCE_LEVEL2 )
    {
        sTable = mtfTableHighPerformance;
    }
    else
    {
        sTable = mtfTable;
    }
    
    IDE_TEST( mtf::getSubModule1Arg( &sSubModule,
                                     sTable,
                                     aStack[1].column->module->no )
              != IDE_SUCCESS );

    IDE_TEST( sSubModule->estimate( aNode,
                                    aTemplate,
                                    aStack,
                                    aRemain,
                                    aCallBack )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* ZONE: FLOAT */

IDE_RC mtfSumInitializeFloat(    mtcNode*     aNode,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 void*        aInfo,
                                 mtcTemplate* aTemplate );

IDE_RC mtfSumAggregateFloat(    mtcNode*     aNode,
                                mtcStack*    aStack,
                                SInt         aRemain,
                                void*        aInfo,
                                mtcTemplate* aTemplate );

IDE_RC mtfSumMergeFloat(    mtcNode*     aNode,
                            mtcStack*    aStack,
                            SInt         aRemain,
                            void*        aInfo,
                            mtcTemplate* aTemplate );

IDE_RC mtfSumFinalizeFloat(    mtcNode*     aNode,
                               mtcStack*    aStack,
                               SInt         aRemain,
                               void*        aInfo,
                               mtcTemplate* aTemplate );

IDE_RC mtfSumCalculateFloat(    mtcNode*     aNode,
                                mtcStack*    aStack,
                                SInt         aRemain,
                                void*        aInfo,
                                mtcTemplate* aTemplate );

static const mtcExecute mtfSumExecuteFloat = {
    mtfSumInitializeFloat,
    mtfSumAggregateFloat,
    mtfSumMergeFloat,
    mtfSumFinalizeFloat,
    mtfSumCalculateFloat,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfSumEstimateFloat( mtcNode*     aNode,
                            mtcTemplate* aTemplate,
                            mtcStack*    aStack,
                            SInt,
                            mtcCallBack* aCallBack )
{
    const mtdModule* sModules[1];

    mtc::makeFloatConversionModule( aStack + 1, &sModules[0] );

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfSumExecuteFloat;

    //IDE_TEST( mtdFloat.estimate( aStack[0].column, 0, 0, 0 )
    //          != IDE_SUCCESS );
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdFloat,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // Sum 결과가 Null인지 아닌지 저장함
    IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                     & mtdBoolean,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( aStack[1].column->module == &mtdList ||
                    aStack[1].column->module == &mtdBoolean,
                    ERR_CONVERSION_NOT_APPLICABLE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfSumInitializeFloat( mtcNode*     aNode,
                              mtcStack*,
                              SInt,
                              void*,
                              mtcTemplate* aTemplate )
{
    const mtcColumn* sColumn;
    mtdNumericType*  sFloat;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;

    sFloat = (mtdNumericType*)((UChar*)aTemplate->rows[aNode->table].row
                               + sColumn[0].column.offset);
    sFloat->length       = 1;
    sFloat->signExponent = 0x80;

    *(mtdBooleanType*) ((UChar*)aTemplate->rows[aNode->table].row
                        + sColumn[1].column.offset) = MTD_BOOLEAN_FALSE;

    return IDE_SUCCESS;
}

IDE_RC mtfSumAggregateFloat( mtcNode*     aNode,
                             mtcStack*    aStack,
                             SInt         aRemain,
                             void*,
                             mtcTemplate* aTemplate )
{
    const mtdModule* sModule;
    mtcNode*         sNode;
    const mtcColumn* sColumn;
    mtdNumericType*  sFloatSum;
    mtdNumericType*  sFloatArgument;
    UChar            sFloatSumBuff[MTD_FLOAT_SIZE_MAXIMUM];
    mtdNumericType*  sFloatSumClone;
    
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

    sModule = aStack[0].column->module;
    if( sModule->isNull( aStack[0].column,
                         aStack[0].value ) != ID_TRUE )
    {
        sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
        sFloatSum = (mtdNumericType*)
            ((UChar*) aTemplate->rows[aNode->table].row
             + sColumn->column.offset);
        sFloatArgument = (mtdNumericType*)aStack[0].value;

        // BUG-42171 The sum window function's value is wrong with nulls first
        // 이전 data도 Null일 수 있으므로 NULL일경우 들어온 Data로 대치시킨다.
        if ( sColumn->module->isNull( sColumn,
                                      sFloatSum ) != ID_TRUE )
        {
            sFloatSumClone = (mtdNumericType*)sFloatSumBuff;
            idlOS::memcpy( sFloatSumClone, sFloatSum, sFloatSum->length + 1 );

            IDE_TEST( mtc::addFloat( sFloatSum,
                                     MTD_FLOAT_PRECISION_MAXIMUM,
                                     sFloatSumClone,
                                     sFloatArgument )
                      != IDE_SUCCESS );
        }
        else
        {
            idlOS::memcpy( sFloatSum, sFloatArgument, sFloatArgument->length + 1 );
        }

        *(mtdBooleanType*)
            ((UChar*) aTemplate->rows[aNode->table].row
             + sColumn[1].column.offset) = MTD_BOOLEAN_TRUE;
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfSumMergeFloat(    mtcNode*     aNode,
                            mtcStack*    ,
                            SInt         ,
                            void*        aInfo,
                            mtcTemplate* aTemplate )
{
    const mtcColumn* sColumn;
    UChar*           sDstRow;
    UChar*           sSrcRow;

    mtdBooleanType   sDstNotNull;
    mtdBooleanType   sSrcNotNull;

    mtdNumericType*  sFloatSum;
    mtdNumericType*  sFloatArgument1;
    mtdNumericType*  sFloatArgument2;
    UChar            sFloatSumBuff[MTD_FLOAT_SIZE_MAXIMUM];

    sDstRow = (UChar*)aTemplate->rows[aNode->table].row;
    sSrcRow = (UChar*)aInfo;
    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;

    sDstNotNull = *(mtdBooleanType*)(sDstRow + sColumn[1].column.offset);
    sSrcNotNull = *(mtdBooleanType*)(sSrcRow + sColumn[1].column.offset);

    // 한쪽이 MTD_BOOLEAN_FALSE 값이여도 초기값이 0 이므로 sum 해도 무방하다.
    if ( (sDstNotNull == MTD_BOOLEAN_TRUE) ||
         (sSrcNotNull == MTD_BOOLEAN_TRUE) )
    {
        sFloatSum       = (mtdNumericType*)(sDstRow + sColumn[0].column.offset);
        sFloatArgument1 = (mtdNumericType*)sFloatSumBuff;
        sFloatArgument2 = (mtdNumericType*)(sSrcRow + sColumn[0].column.offset);

        idlOS::memcpy( sFloatSumBuff, sFloatSum, sFloatSum->length + 1 );

        IDE_TEST( mtc::addFloat( sFloatSum,
                                 MTD_FLOAT_PRECISION_MAXIMUM,
                                 sFloatArgument1,
                                 sFloatArgument2 )
                  != IDE_SUCCESS );

        *(mtdBooleanType*)(sDstRow + sColumn[1].column.offset) = MTD_BOOLEAN_TRUE;
    }
    else
    {
        // Nothing to do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfSumFinalizeFloat( mtcNode* aSumNode,
                            mtcStack*,
                            SInt,
                            void*,
                            mtcTemplate* aTemplate )
{
    mtcColumn* sSumColumn;
    void*      sValueTemp;

    sSumColumn = aTemplate->rows[aSumNode->table].columns + aSumNode->column;

    if( *(mtdBooleanType*)( (UChar*) aTemplate->rows[aSumNode->table].row
                            + sSumColumn[1].column.offset )
        == MTD_BOOLEAN_TRUE )
    {
        // Nothing to do
    }
    else
    {
        sValueTemp = (void*)mtd::valueForModule(
                                 (smiColumn*)sSumColumn,
                                 aTemplate->rows[aSumNode->table].row,
                                 MTD_OFFSET_USE,
                                 sSumColumn->module->staticNull );

        mtdFloat.null( sSumColumn,
                       sValueTemp );
    }

    return IDE_SUCCESS;
}

IDE_RC mtfSumCalculateFloat( mtcNode*     aNode,
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

/* ZONE: DOUBLE */

IDE_RC mtfSumInitializeDouble(    mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*        aInfo,
                                  mtcTemplate* aTemplate );

IDE_RC mtfSumAggregateDouble(    mtcNode*     aNode,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 void*        aInfo,
                                 mtcTemplate* aTemplate );

IDE_RC mtfSumAggregateDoubleFast( mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*        aInfo,
                                  mtcTemplate* aTemplate );

IDE_RC mtfSumMergeDouble(    mtcNode*     aNode,
                             mtcStack*    aStack,
                             SInt         aRemain,
                             void*        aInfo,
                             mtcTemplate* aTemplate );

IDE_RC mtfSumFinalizeDouble(    mtcNode*     aNode,
                                mtcStack*    aStack,
                                SInt         aRemain,
                                void*        aInfo,
                                mtcTemplate* aTemplate );

IDE_RC mtfSumFinalizeDouble(    mtcNode*     aNode,
                                mtcStack*    aStack,
                                SInt         aRemain,
                                void*        aInfo,
                                 mtcTemplate* aTemplate );

IDE_RC mtfSumCalculateDouble(    mtcNode*     aNode,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 void*        aInfo,
                                 mtcTemplate* aTemplate );

static const mtcExecute mtfSumExecuteDouble = {
    mtfSumInitializeDouble,
    mtfSumAggregateDouble,
    mtfSumMergeDouble,
    mtfSumFinalizeDouble,
    mtfSumCalculateDouble,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

// 최적의 sum을 수행하는
// aggregate 함수를 포함하고 있는 execute
static const mtcExecute mtfSumExecuteDoubleFast = {
    mtfSumInitializeDouble,
    mtfSumAggregateDoubleFast,
    mtfSumMergeDouble,
    mtfSumFinalizeDouble,
    mtfSumCalculateDouble,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfSumEstimateDouble( mtcNode*     aNode,
                             mtcTemplate* aTemplate,
                             mtcStack*    aStack,
                             SInt,
                             mtcCallBack* aCallBack )
{
    const mtfModule * sArgModule;
    
    static const mtdModule* sModules[1] = {
        &mtdDouble
    };
    
    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );
    
    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;
    
    aTemplate->rows[aNode->table].execute[aNode->column] = mtfSumExecuteDouble;

    // 최적화된 aggregate 함수
    sArgModule = aNode->arguments->module;

    // mtf::initializeTemplate에서 각 subModule에 대해 
    // estimateBound를 호출하는데 이때에는 node에 module이
    // 안달려있기 때문에 NULL 체크를 해야 한다.
    if( sArgModule != NULL )
    {
        // sum(i1) 처럼 i1가 단일 컬럼이고 conversion되지 않는다면
        // 최적화된 execution을 달아준다.
        
        // BUG-19856
        // view 컬럼인 경우 최적화된 execution을 달지않는다.
        if( ( ( aTemplate->rows[aNode->arguments->table].lflag
                & MTC_TUPLE_VIEW_MASK )
              == MTC_TUPLE_VIEW_FALSE ) &&
            ( idlOS::strncmp((SChar*)sArgModule->names->string, 
                             (const SChar*)"COLUMN", 6 )
              == 0 ) &&
            ( aNode->arguments->conversion == NULL ) )
        {
            aTemplate->rows[aNode->table].execute[aNode->column]
                = mtfSumExecuteDoubleFast;
        }
    }
    
    //IDE_TEST( mtdDouble.estimate( aStack[0].column, 0, 0, 0 )
    //          != IDE_SUCCESS );
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdDouble,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // Sum 결과가 Null인지 아닌지 저장함
    IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                     & mtdBoolean,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( aStack[1].column->module == &mtdList ||
                    aStack[1].column->module == &mtdBoolean,
                    ERR_CONVERSION_NOT_APPLICABLE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfSumInitializeDouble( mtcNode*     aNode,
                               mtcStack*,
                               SInt,
                               void*,
                               mtcTemplate* aTemplate )
{
    const mtcColumn* sColumn;
    
    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    
    *(mtdDoubleType*)((UChar*) aTemplate->rows[aNode->table].row
                      + sColumn->column.offset) = 0;
    *(mtdBooleanType*)((UChar*) aTemplate->rows[aNode->table].row
                       + sColumn[1].column.offset) = MTD_BOOLEAN_FALSE;
    
    return IDE_SUCCESS;
}

IDE_RC mtfSumAggregateDouble( mtcNode*     aNode,
                              mtcStack*    aStack,
                              SInt         aRemain,
                              void*,
                              mtcTemplate* aTemplate )
{
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
    
    // mtdDouble.isNull() 를 호출하는 대신
    // 직접 null 검사를 한다.
    // aStack->value의 데이터 타입을 미리 알기 때문에
    // 직접 null 검사를 하는데 수행 속도를 위해서이다.
    if( ( *(ULong*)(aStack->value) & MTD_DOUBLE_EXPONENT_MASK )
        != MTD_DOUBLE_EXPONENT_MASK )
    {
        sColumn = aTemplate->rows[aNode->table].columns + aNode->column;

        // BUG-42171 The sum window function's value is wrong with nulls first
        // 이전 data도 Null일 수 있으므로 NULL일경우 들어온 Data로 대치시킨다.
        if ( ( *(ULong*)((UChar*)aTemplate->rows[aNode->table].row +
                         sColumn->column.offset ) & MTD_DOUBLE_EXPONENT_MASK )
             != MTD_DOUBLE_EXPONENT_MASK )
        {
            *(mtdDoubleType*)((UChar*)aTemplate->rows[aNode->table].row
                              + sColumn->column.offset) +=
                *(mtdDoubleType*)aStack[0].value;
        }
        else
        {
            *(mtdDoubleType*)((UChar*)aTemplate->rows[aNode->table].row
                              + sColumn->column.offset) =
                *(mtdDoubleType*)aStack[0].value;
        }

        *(mtdBooleanType*)((UChar*)aTemplate->rows[aNode->table].row
                           + sColumn[1].column.offset) = MTD_BOOLEAN_TRUE;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfSumAggregateDoubleFast( mtcNode*     aSumNode,
                                  mtcStack*    aStack,
                                  SInt,
                                  void*,
                                  mtcTemplate* aTemplate )
{
    mtcNode *        sArgumentNode;
    const mtcColumn* sSumColumn;

    sArgumentNode  = aSumNode->arguments;

    aStack->column = aTemplate->rows[sArgumentNode->table].columns
        + sArgumentNode->column;
    aStack->value  = (void*) mtc::value( aStack->column,
                                         aTemplate->rows[sArgumentNode->table].row,
                                         MTD_OFFSET_USE );

    // mtdDouble.isNull() 를 호출하는 대신
    // 직접 null 검사를 한다.
    // aStack->value의 데이터 타입을 미리 알기 때문에
    // 직접 null 검사를 하는데 수행 속도를 위해서이다.
    if( ( *(ULong*)(aStack->value) & MTD_DOUBLE_EXPONENT_MASK )
        != MTD_DOUBLE_EXPONENT_MASK )
    {
        sSumColumn = aTemplate->rows[aSumNode->table].columns
            + aSumNode->column;

        // BUG-42171 The sum window function's value is wrong with nulls first
        // 이전 data도 Null일 수 있으므로 NULL일경우 들어온 Data로 대치시킨다.
        if ( ( *(ULong *)((UChar*)aTemplate->rows[aSumNode->table].row +
                           sSumColumn->column.offset ) & MTD_DOUBLE_EXPONENT_MASK )
             != MTD_DOUBLE_EXPONENT_MASK )
        {
            *(mtdDoubleType*) ((UChar*) aTemplate->rows[aSumNode->table].row
                               + sSumColumn->column.offset) +=
                *(mtdDoubleType*)aStack->value;
        }
        else
        {
            *(mtdDoubleType*) ((UChar*) aTemplate->rows[aSumNode->table].row
                               + sSumColumn->column.offset) =
                *(mtdDoubleType*)aStack->value;
        }
        *(mtdBooleanType*) ((UChar*) aTemplate->rows[aSumNode->table].row
                            + sSumColumn[1].column.offset) = MTD_BOOLEAN_TRUE;
    }

    return IDE_SUCCESS;
}

IDE_RC mtfSumMergeDouble(    mtcNode*     aNode,
                             mtcStack*    ,
                             SInt         ,
                             void*        aInfo,
                             mtcTemplate* aTemplate )
{
    const mtcColumn* sColumn;
    UChar*           sDstRow;
    UChar*           sSrcRow;

    mtdBooleanType   sDstNotNull;
    mtdBooleanType   sSrcNotNull;

    sDstRow = (UChar*)aTemplate->rows[aNode->table].row;
    sSrcRow = (UChar*)aInfo;
    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;

    sDstNotNull = *(mtdBooleanType*)(sDstRow + sColumn[1].column.offset);
    sSrcNotNull = *(mtdBooleanType*)(sSrcRow + sColumn[1].column.offset);

    // 한쪽이 MTD_BOOLEAN_FALSE 값이여도 초기값이 0 이므로 sum 해도 무방하다.
    if ( (sDstNotNull == MTD_BOOLEAN_TRUE) ||
         (sSrcNotNull == MTD_BOOLEAN_TRUE) )
    {
        *(mtdDoubleType*)(sDstRow + sColumn[0].column.offset) +=
        *(mtdDoubleType*)(sSrcRow + sColumn[0].column.offset);

        *(mtdBooleanType*)(sDstRow + sColumn[1].column.offset) = MTD_BOOLEAN_TRUE;
    }
    else
    {
        // Nothing to do
    }

    return IDE_SUCCESS;
}

IDE_RC mtfSumFinalizeDouble( mtcNode* aSumNode,
                             mtcStack*,
                             SInt,
                             void*,
                             mtcTemplate* aTemplate )
{
    mtcColumn* sSumColumn;
    void*      sValueTemp;

    sSumColumn = aTemplate->rows[aSumNode->table].columns + aSumNode->column;

    if( *(mtdBooleanType*)( (UChar*) aTemplate->rows[aSumNode->table].row
                            + sSumColumn[1].column.offset )
        == MTD_BOOLEAN_TRUE )
    {
        // Nothing to do
    }
    else
    {
        sValueTemp = (void*)mtd::valueForModule(
                                 (smiColumn*)sSumColumn,
                                 aTemplate->rows[aSumNode->table].row,
                                 MTD_OFFSET_USE,
                                 sSumColumn->module->staticNull );

        mtdDouble.null( sSumColumn,
                        sValueTemp );
    }

    return IDE_SUCCESS;
}

IDE_RC mtfSumCalculateDouble( mtcNode*     aNode,
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

/* ZONE: BIGINT */

IDE_RC mtfSumInitializeBigint(    mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*        aInfo,
                                  mtcTemplate* aTemplate );

IDE_RC mtfSumAggregateBigint(    mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*        aInfo,
                                  mtcTemplate* aTemplate );

IDE_RC mtfSumAggregateBigintFast( mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*        aInfo,
                                  mtcTemplate* aTemplate );

IDE_RC mtfSumMergeBigint(    mtcNode*     aNode,
                             mtcStack*    aStack,
                             SInt         aRemain,
                             void*        aInfo,
                             mtcTemplate* aTemplate );


IDE_RC mtfSumFinalizeBigint(    mtcNode*     aNode,
                                mtcStack*    aStack,
                                SInt         aRemain,
                                void*        aInfo,
                                mtcTemplate* aTemplate );

IDE_RC mtfSumCalculateBigint(    mtcNode*     aNode,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 void*        aInfo,
                                 mtcTemplate* aTemplate );

static const mtcExecute mtfSumExecuteBigint = {
    mtfSumInitializeBigint,
    mtfSumAggregateBigint,
    mtfSumMergeBigint,
    mtfSumFinalizeBigint,
    mtfSumCalculateBigint,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};


// 최적의 sum을 수행하는
// aggregate 함수를 포함하고 있는 execute
static const mtcExecute mtfSumExecuteBigintFast = {
    mtfSumInitializeBigint,
    mtfSumAggregateBigintFast,
    mtfSumMergeBigint,
    mtfSumFinalizeBigint,
    mtfSumCalculateBigint,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfSumEstimateBigint( mtcNode*     aNode,
                             mtcTemplate* aTemplate,
                             mtcStack*    aStack,
                             SInt,
                             mtcCallBack* aCallBack )
{
    const mtfModule * sArgModule;

    static const mtdModule* sModules[1] = {
        &mtdBigint
    };
    
    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );
    
    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;
    
    aTemplate->rows[aNode->table].execute[aNode->column] = mtfSumExecuteBigint;
    
    // 최적화된 aggregate 함수
    sArgModule = aNode->arguments->module;

    // mtf::initializeTemplate에서 각 subModule에 대해
    // estimateBound를 호출하는데 이때에는 node에 module이
    // 안달려있기 때문에 NULL 체크를 해야 한다.
    if( sArgModule != NULL )
    {
        // sum(i1) 처럼 i1가 단일 컬럼이고 conversion되지 않는다면
        // 최적화된 execution을 달아준다.
        
        // BUG-19856
        // view 컬럼인 경우 최적화된 execution을 달지않는다.
        if( ( ( aTemplate->rows[aNode->arguments->table].lflag
                & MTC_TUPLE_VIEW_MASK )
              == MTC_TUPLE_VIEW_FALSE ) &&
            ( idlOS::strncmp((SChar*)sArgModule->names->string,
                             (const SChar*)"COLUMN", 6 )
              == 0 ) &&
            ( aNode->arguments->conversion == NULL ) )
        {
            aTemplate->rows[aNode->table].execute[aNode->column]
                = mtfSumExecuteBigintFast;
        }
    }

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdBigint,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // Sum 결과가 Null인지 아닌지 저장함
    IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                     & mtdBoolean,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( aStack[1].column->module == &mtdList ||
                    aStack[1].column->module == &mtdBoolean,
                    ERR_CONVERSION_NOT_APPLICABLE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfSumInitializeBigint( mtcNode*     aNode,
                               mtcStack*,
                               SInt,
                               void*,
                               mtcTemplate* aTemplate )
{
    const mtcColumn* sColumn;
    
    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    
    *(mtdBigintType*) ((UChar*)aTemplate->rows[aNode->table].row
                       + sColumn->column.offset) = 0;
    *(mtdBooleanType*)((UChar*)aTemplate->rows[aNode->table].row
                       + sColumn[1].column.offset) = MTD_BOOLEAN_FALSE;
    
    return IDE_SUCCESS;
}

IDE_RC mtfSumAggregateBigint( mtcNode*     aNode,
                              mtcStack*    aStack,
                              SInt         aRemain,
                              void*,
                              mtcTemplate* aTemplate )
{
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
    
    // mtdBigint.isNull() 를 호출하는 대신
    // 직접 null 검사를 한다.
    // aStack->value의 데이터 타입을 미리 알기 때문에
    // 직접 null 검사를 하는데 수행 속도를 위해서이다.
    if( *(mtdBigintType*)aStack->value != MTD_BIGINT_NULL )
    {
        sColumn = aTemplate->rows[aNode->table].columns + aNode->column;

        // BUG-42171 The sum window function's value is wrong with nulls first
        // 이전 data도 Null일 수 있으므로 NULL일경우 들어온 Data로 대치시킨다.
        if ( *(mtdBigintType *)((UChar *)aTemplate->rows[aNode->table].row + sColumn->column.offset )
             != MTD_BIGINT_NULL )
        {
            *(mtdBigintType*)((UChar*) aTemplate->rows[aNode->table].row
                              + sColumn->column.offset) +=
                *(mtdBigintType*)aStack[0].value;
        }
        else
        {
            *(mtdBigintType*)((UChar*) aTemplate->rows[aNode->table].row
                              + sColumn->column.offset) =
                *(mtdBigintType*)aStack[0].value;
        }
        *(mtdBooleanType*)((UChar*) aTemplate->rows[aNode->table].row
                           + sColumn[1].column.offset) = MTD_BOOLEAN_TRUE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfSumAggregateBigintFast( mtcNode*     aSumNode,
                                  mtcStack*    aStack,
                                  SInt,
                                  void*,
                                  mtcTemplate* aTemplate )
{
    mtcNode*         sArgumentNode;
    const mtcColumn* sSumColumn;

    sArgumentNode  = aSumNode->arguments;

    aStack->column = aTemplate->rows[sArgumentNode->table].columns
        + sArgumentNode->column;
    aStack->value  = (void*) mtc::value( aStack->column,
                                         aTemplate->rows[sArgumentNode->table].row,
                                         MTD_OFFSET_USE );

    // mtdBigint.isNull() 를 호출하는 대신
    // 직접 null 검사를 한다.
    // aStack->value의 데이터 타입을 미리 알기 때문에
    // 직접 null 검사를 하는데 수행 속도를 위해서이다.
    if( *(mtdBigintType*)aStack->value != MTD_BIGINT_NULL )
    {
        sSumColumn = aTemplate->rows[aSumNode->table].columns + aSumNode->column;

        // BUG-42171 The sum window function's value is wrong with nulls first
        // 이전 data도 Null일 수 있으므로 NULL일경우 들어온 Data로 대치시킨다.
        if ( *(mtdBigintType *)((UChar *)aTemplate->rows[aSumNode->table].row +
                                sSumColumn->column.offset ) != MTD_BIGINT_NULL )
        {
            *(mtdBigintType*)((UChar*) aTemplate->rows[aSumNode->table].row
                              + sSumColumn->column.offset) +=
                *(mtdBigintType*)aStack->value;
        }
        else
        {
            *(mtdBigintType*)((UChar*) aTemplate->rows[aSumNode->table].row
                              + sSumColumn->column.offset) =
                *(mtdBigintType*)aStack->value;
        }
        *(mtdBooleanType*)((UChar*) aTemplate->rows[aSumNode->table].row
                           + sSumColumn[1].column.offset) = MTD_BOOLEAN_TRUE;
    }

    return IDE_SUCCESS;
}

IDE_RC mtfSumMergeBigint(    mtcNode*     aNode,
                             mtcStack*    ,
                             SInt         ,
                             void*        aInfo,
                             mtcTemplate* aTemplate )
{
    const mtcColumn* sColumn;
    UChar*           sDstRow;
    UChar*           sSrcRow;

    mtdBooleanType   sDstNotNull;
    mtdBooleanType   sSrcNotNull;

    sDstRow = (UChar*)aTemplate->rows[aNode->table].row;
    sSrcRow = (UChar*)aInfo;
    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;

    sDstNotNull = *(mtdBooleanType*)(sDstRow + sColumn[1].column.offset);
    sSrcNotNull = *(mtdBooleanType*)(sSrcRow + sColumn[1].column.offset);

    // 한쪽이 MTD_BOOLEAN_FALSE 값이여도 초기값이 0 이므로 sum 해도 무방하다.
    if ( (sDstNotNull == MTD_BOOLEAN_TRUE) ||
         (sSrcNotNull == MTD_BOOLEAN_TRUE) )
    {
        *(mtdBigintType*)(sDstRow + sColumn[0].column.offset) +=
        *(mtdBigintType*)(sSrcRow + sColumn[0].column.offset);

        *(mtdBooleanType*)(sDstRow + sColumn[1].column.offset) = MTD_BOOLEAN_TRUE;
    }
    else
    {
        // Nothing to do
    }

    return IDE_SUCCESS;
}

IDE_RC mtfSumFinalizeBigint( mtcNode* aSumNode,
                             mtcStack*,
                             SInt,
                             void*,
                             mtcTemplate* aTemplate )
{
    mtcColumn* sSumColumn;
    void*      sValueTemp;

    sSumColumn = aTemplate->rows[aSumNode->table].columns + aSumNode->column;

    if( *(mtdBooleanType*)( (UChar*) aTemplate->rows[aSumNode->table].row
                            + sSumColumn[1].column.offset )
        == MTD_BOOLEAN_TRUE )
    {
        // Nothing to do
    }
    else
    {
        sValueTemp = (void*)mtd::valueForModule(
                                 (smiColumn*)sSumColumn,
                                 aTemplate->rows[aSumNode->table].row,
                                 MTD_OFFSET_USE,
                                 sSumColumn->module->staticNull );

        mtdBigint.null( sSumColumn,
                        sValueTemp );
    }

    return IDE_SUCCESS;
}

IDE_RC mtfSumCalculateBigint( mtcNode*     aNode,
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
 
