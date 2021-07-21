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
 * $Id: mtfAvg.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>
#include <mtuProperty.h>

extern mtfModule mtfAvg;

extern mtdModule mtdFloat;
extern mtdModule mtdDouble;
extern mtdModule mtdBigint;
extern mtdModule mtdNull;
extern mtdModule mtdList;
extern mtdModule mtdBoolean;

static mtcName mtfAvgFunctionName[1] = {
    { NULL, 3, (void*)"AVG" }
};

static IDE_RC mtfAvgInitialize( void );

static IDE_RC mtfAvgFinalize( void );

static IDE_RC mtfAvgEstimate( mtcNode*     aNode,
                              mtcTemplate* aTemplate,
                              mtcStack*    aStack,
                              SInt         aRemain,
                              mtcCallBack* aCallBack );

mtfModule mtfAvg = {
    4|MTC_NODE_OPERATOR_AGGREGATION|MTC_NODE_FUNCTION_WINDOWING_TRUE,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfAvgFunctionName,
    NULL,
    mtfAvgInitialize,
    mtfAvgFinalize,
    mtfAvgEstimate
};

static IDE_RC mtfAvgEstimateFloat( mtcNode*     aNode,
                                   mtcTemplate* aTemplate,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   mtcCallBack* aCallBack );

static IDE_RC mtfAvgEstimateDouble( mtcNode*     aNode,
                                    mtcTemplate* aTemplate,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    mtcCallBack* aCallBack );

static IDE_RC mtfAvgEstimateBigint( mtcNode*     aNode,
                                    mtcTemplate* aTemplate,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    mtcCallBack* aCallBack );

static mtfSubModule mtfXX[1] = {
    { NULL, mtf::estimateNA }
};

static mtfSubModule mtfAvgEstimates[3] = {
    { mtfAvgEstimates+1, mtfAvgEstimateDouble },
    { mtfAvgEstimates+2, mtfAvgEstimateBigint },
    { NULL,              mtfAvgEstimateFloat }
};

// BUG-41994
// high performance용 group table
static mtfSubModule mtfAvgEstimatesHighPerformance[2] = {
    { mtfAvgEstimatesHighPerformance+1, mtfAvgEstimateDouble },
    { NULL,                             mtfAvgEstimateBigint }
};

static mtfSubModule** mtfTable = NULL;
static mtfSubModule** mtfTableHighPerformance = NULL;

IDE_RC mtfAvgInitialize( void )
{
    IDE_TEST( mtf::initializeTemplate( &mtfTable,
                                       mtfAvgEstimates,
                                       mtfXX )
              != IDE_SUCCESS );

    IDE_TEST( mtf::initializeTemplate( &mtfTableHighPerformance,
                                       mtfAvgEstimatesHighPerformance,
                                       mtfXX )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfAvgFinalize( void )
{
    IDE_TEST( mtf::finalizeTemplate( &mtfTable )
              != IDE_SUCCESS );

    IDE_TEST( mtf::finalizeTemplate( &mtfTableHighPerformance )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfAvgEstimate( mtcNode*     aNode,
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

IDE_RC mtfAvgInitializeFloat(    mtcNode*     aNode,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 void*        aInfo,
                                 mtcTemplate* aTemplate );

IDE_RC mtfAvgAggregateFloat(    mtcNode*     aNode,
                                mtcStack*    aStack,
                                SInt         aRemain,
                                void*        aInfo,
                                mtcTemplate* aTemplate );

IDE_RC mtfAvgMergeFloat(    mtcNode*     aNode,
                            mtcStack*    aStack,
                            SInt         aRemain,
                            void*        aInfo,
                            mtcTemplate* aTemplate );

IDE_RC mtfAvgFinalizeFloat(    mtcNode*     aNode,
                               mtcStack*    aStack,
                               SInt         aRemain,
                               void*        aInfo,
                               mtcTemplate* aTemplate );

IDE_RC mtfAvgCalculateFloat(    mtcNode*     aNode,
                                mtcStack*    aStack,
                                SInt         aRemain,
                                void*        aInfo,
                                mtcTemplate* aTemplate );

static const mtcExecute mtfAvgExecuteFloat = {
    mtfAvgInitializeFloat,
    mtfAvgAggregateFloat,
    mtfAvgMergeFloat,
    mtfAvgFinalizeFloat,
    mtfAvgCalculateFloat,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfAvgEstimateFloat( mtcNode*     aNode,
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
    
    aTemplate->rows[aNode->table].execute[aNode->column] = mtfAvgExecuteFloat;

    /*
    IDE_TEST( mtdFloat.estimate( aStack[0].column + 0, 0, 0, 0 )
              != IDE_SUCCESS );
    IDE_TEST( mtdFloat.estimate( aStack[0].column + 1, 0, 0, 0 )
              != IDE_SUCCESS );
    IDE_TEST( mtdBigint.estimate( aStack[0].column + 2, 0, 0, 0 )
              != IDE_SUCCESS );
    IDE_TEST( mtdNull.estimate( aStack[0].column + 3, 0, 0, 0 )
              != IDE_SUCCESS );
    */
    IDE_TEST( mtc::initializeColumn( aStack[0].column + 0,
                                     & mtdFloat,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                     & mtdFloat,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    IDE_TEST( mtc::initializeColumn( aStack[0].column + 2,
                                     & mtdBigint,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    IDE_TEST( mtc::initializeColumn( aStack[0].column + 3,
                                     & mtdNull,
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

IDE_RC mtfAvgInitializeFloat( mtcNode*     aNode,
                              mtcStack*,
                              SInt,
                              void*,
                              mtcTemplate* aTemplate )
{
    const mtcColumn* sColumn;
    mtdNumericType*  sFloat;
    
    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    
    sFloat = (mtdNumericType*)
        ( (UChar*) aTemplate->rows[aNode->table].row
          + sColumn[0].column.offset );
    sFloat->length       = 1;
    sFloat->signExponent = 0x80;
    sFloat = (mtdNumericType*)
        ( (UChar*) aTemplate->rows[aNode->table].row
          + sColumn[1].column.offset);
    sFloat->length       = 1;
    sFloat->signExponent = 0x80;
    *(mtdBigintType*)
        ( (UChar*) aTemplate->rows[aNode->table].row
          + sColumn[2].column.offset ) = 0;
    
    return IDE_SUCCESS;
}

IDE_RC mtfAvgAggregateFloat( mtcNode*     aNode,
                             mtcStack*    aStack,
                             SInt         aRemain,
                             void*,
                             mtcTemplate* aTemplate )
{
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
    
    if( aStack[0].column->module->isNull( aStack[0].column,
                                          aStack[0].value ) != ID_TRUE )
    {
        sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
        sFloatSum = (mtdNumericType*)
            ( (UChar*) aTemplate->rows[aNode->table].row
              + sColumn[1].column.offset ); 
        sFloatArgument = (mtdNumericType*)aStack[0].value;
        sFloatSumClone = (mtdNumericType*)sFloatSumBuff;
        idlOS::memcpy( sFloatSumClone, sFloatSum, sFloatSum->length + 1 );

        IDE_TEST( mtc::addFloat( sFloatSum,
                                 MTD_FLOAT_PRECISION_MAXIMUM,
                                 sFloatSumClone,
                                 sFloatArgument )
                  != IDE_SUCCESS );
        
        *(mtdBigintType*)
            ( (UChar*) aTemplate->rows[aNode->table].row
              + sColumn[2].column.offset ) += 1;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfAvgMergeFloat(    mtcNode*     aNode,
                            mtcStack*    ,
                            SInt         ,
                            void*        aInfo,
                            mtcTemplate* aTemplate )
{
    const mtcColumn* sColumn;
    UChar*           sDstRow;
    UChar*           sSrcRow;

    mtdNumericType*  sFloatSum;
    mtdNumericType*  sFloatArgument1;
    mtdNumericType*  sFloatArgument2;
    UChar            sFloatSumBuff[MTD_FLOAT_SIZE_MAXIMUM];

    sDstRow = (UChar*)aTemplate->rows[aNode->table].row;
    sSrcRow = (UChar*)aInfo;
    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;

    //----------------------------------------
    // Merge 작업을 수행한다.
    //----------------------------------------

    // Sum
    sFloatSum       = (mtdNumericType*)(sDstRow + sColumn[1].column.offset);
    sFloatArgument1 = (mtdNumericType*)sFloatSumBuff;
    sFloatArgument2 = (mtdNumericType*)(sSrcRow + sColumn[1].column.offset);

    idlOS::memcpy( sFloatSumBuff, sFloatSum, sFloatSum->length + 1 );

    IDE_TEST( mtc::addFloat( sFloatSum,
                             MTD_FLOAT_PRECISION_MAXIMUM,
                             sFloatArgument1,
                             sFloatArgument2 )
              != IDE_SUCCESS );

    // Count
    *(mtdBigintType*)(sDstRow + sColumn[2].column.offset) +=
    *(mtdBigintType*)(sSrcRow + sColumn[2].column.offset);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfAvgFinalizeFloat( mtcNode*     aNode,
                            mtcStack*,
                            SInt,
                            void*,
                            mtcTemplate* aTemplate )
{
    const mtcColumn* sColumn;
    mtdBigintType    sCount;
    mtdNumericType*  sFloatResult;
    mtdNumericType*  sFloatSum;
    UChar            sFloatCountBuff[MTD_FLOAT_SIZE_MAXIMUM];
    mtdNumericType*  sFloatCount;
    void*            sValue;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    
    sCount = *(mtdBigintType*)
        ( (UChar*) aTemplate->rows[aNode->table].row
          + sColumn[2].column.offset );
    if( sCount == 0 )
    {
        sValue = (void*)mtd::valueForModule(
                             (smiColumn*)sColumn + 0,
                             aTemplate->rows[aNode->table].row,
                             MTD_OFFSET_USE,
                             sColumn->module->staticNull );

        sColumn[0].module->null( sColumn + 0, sValue );
    }
    else
    {
        sFloatResult = (mtdNumericType*)
            ( (UChar*) aTemplate->rows[aNode->table].row
              + sColumn[0].column.offset );
        sFloatSum = (mtdNumericType*)
            ( (UChar*) aTemplate->rows[aNode->table].row
              + sColumn[1].column.offset );

        sFloatCount = (mtdNumericType*)sFloatCountBuff;

        mtc::makeNumeric( sFloatCount, (SLong)sCount );

        IDE_TEST( mtc::divideFloat( sFloatResult,
                                    MTD_FLOAT_PRECISION_MAXIMUM,
                                    sFloatSum,
                                    sFloatCount )
                  != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfAvgCalculateFloat( mtcNode*     aNode,
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

IDE_RC mtfAvgInitializeDouble(    mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*        aInfo,
                                  mtcTemplate* aTemplate );

IDE_RC mtfAvgAggregateDouble( mtcNode*     aNode,
                              mtcStack*    aStack,
                              SInt         aRemain,
                              void*        aInfo,
                              mtcTemplate* aTemplate );

IDE_RC mtfAvgAggregateDoubleFast( mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*        aInfo,
                                  mtcTemplate* aTemplate );

IDE_RC mtfAvgMergeDouble(    mtcNode*     aNode,
                             mtcStack*    aStack,
                             SInt         aRemain,
                             void*        aInfo,
                             mtcTemplate* aTemplate );

IDE_RC mtfAvgFinalizeDouble(    mtcNode*     aNode,
                                mtcStack*    aStack,
                                SInt         aRemain,
                                void*        aInfo,
                                mtcTemplate* aTemplate );

IDE_RC mtfAvgFinalizeDouble(    mtcNode*     aNode,
                                mtcStack*    aStack,
                                SInt         aRemain,
                                void*        aInfo,
                                mtcTemplate* aTemplate );

IDE_RC mtfAvgCalculateDouble(    mtcNode*     aNode,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 void*        aInfo,
                                 mtcTemplate* aTemplate );

static const mtcExecute mtfAvgExecuteDouble = {
    mtfAvgInitializeDouble,
    mtfAvgAggregateDouble,
    mtfAvgMergeDouble,
    mtfAvgFinalizeDouble,
    mtfAvgCalculateDouble,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};


// 최적의 sum을 수행하는
// aggregate 함수를 포함하고 있는 execute
static const mtcExecute mtfAvgExecuteDoubleFast = {
    mtfAvgInitializeDouble,
    mtfAvgAggregateDoubleFast,
    mtfAvgMergeDouble,
    mtfAvgFinalizeDouble,
    mtfAvgCalculateDouble,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};  

IDE_RC mtfAvgEstimateDouble( mtcNode*     aNode,
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
    
    aTemplate->rows[aNode->table].execute[aNode->column] = mtfAvgExecuteDouble;
    
    // 최적화된 aggregate 함수
    sArgModule = aNode->arguments->module;
    if( sArgModule != NULL )
    {
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
                = mtfAvgExecuteDoubleFast;
        }
    }

    /*
    IDE_TEST( mtdDouble.estimate( aStack[0].column + 0, 0, 0, 0 )
              != IDE_SUCCESS );
    IDE_TEST( mtdDouble.estimate( aStack[0].column + 1, 0, 0, 0 )
              != IDE_SUCCESS );
    IDE_TEST( mtdBigint.estimate( aStack[0].column + 2, 0, 0, 0 )
              != IDE_SUCCESS );
    IDE_TEST( mtdNull.estimate( aStack[0].column + 3, 0, 0, 0 )
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
                                     & mtdBigint,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    IDE_TEST( mtc::initializeColumn( aStack[0].column + 3,
                                     & mtdNull,
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

IDE_RC mtfAvgInitializeDouble( mtcNode*     aNode,
                               mtcStack*,
                               SInt,
                               void*,
                               mtcTemplate* aTemplate )
{
    const mtcColumn* sColumn;
    
    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    
    *(mtdDoubleType*)( (UChar*) aTemplate->rows[aNode->table].row
                       + sColumn[1].column.offset) = 0;
    *(mtdBigintType*)( (UChar*) aTemplate->rows[aNode->table].row
                       + sColumn[2].column.offset) = 0;
    
    return IDE_SUCCESS;
}

IDE_RC mtfAvgAggregateDouble( mtcNode*     aNode,
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
        *(mtdDoubleType*)
            ( (UChar*) aTemplate->rows[aNode->table].row
              + sColumn[1].column.offset) += *(mtdDoubleType*)aStack[0].value;
        *(mtdBigintType*)
            ( (UChar*) aTemplate->rows[aNode->table].row
              + sColumn[2].column.offset) += 1;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfAvgAggregateDoubleFast( mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt,
                                  void*,
                                  mtcTemplate* aTemplate )
{
    mtcNode*         sNode;
    const mtcColumn* sColumn;

    sNode  = aNode->arguments;

    aStack->column = & aTemplate->rows[sNode->table].columns[sNode->column];
    aStack->value  = (void*) mtc::value( aStack->column,
                                         aTemplate->rows[sNode->table].row,
                                         MTD_OFFSET_USE );

    // mtdDouble.isNull() 를 호출하는 대신
    // 직접 null 검사를 한다.
    // aStack->value의 데이터 타입을 미리 알기 때문에
    // 직접 null 검사를 하는데 수행 속도를 위해서이다.
    if( ( *(ULong*)(aStack->value) & MTD_DOUBLE_EXPONENT_MASK )
        != MTD_DOUBLE_EXPONENT_MASK )
    {
        sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
        *(mtdDoubleType*)((UChar*) aTemplate->rows[aNode->table].row
                          + sColumn[1].column.offset)
            += *(mtdDoubleType*)aStack[0].value;
        *(mtdBigintType*)((UChar*) aTemplate->rows[aNode->table].row
                          + sColumn[2].column.offset) += 1;
    }

    return IDE_SUCCESS;
}

IDE_RC mtfAvgMergeDouble(    mtcNode*     aNode,
                             mtcStack*    ,
                             SInt         ,
                             void*        aInfo,
                             mtcTemplate* aTemplate )
{
    const mtcColumn* sColumn;
    UChar*           sDstRow;
    UChar*           sSrcRow;

    sDstRow = (UChar*)aTemplate->rows[aNode->table].row;
    sSrcRow = (UChar*)aInfo;
    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;

    //----------------------------------------
    // Merge 작업을 수행한다.
    //----------------------------------------

    // Sum
    *(mtdDoubleType*)(sDstRow + sColumn[1].column.offset) +=
    *(mtdDoubleType*)(sSrcRow + sColumn[1].column.offset);

    // Count
    *(mtdBigintType*)(sDstRow + sColumn[2].column.offset) +=
    *(mtdBigintType*)(sSrcRow + sColumn[2].column.offset);

    return IDE_SUCCESS;
}

IDE_RC mtfAvgFinalizeDouble( mtcNode*     aNode,
                             mtcStack*,
                             SInt,
                             void*,
                             mtcTemplate* aTemplate )
{
    const mtcColumn* sColumn;
    mtdBigintType    sCount;
    void*            sValue;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    
    sCount = *(mtdBigintType*)
        ( (UChar*) aTemplate->rows[aNode->table].row
          + sColumn[2].column.offset);
    if( sCount == 0 )
    {
        sValue = (void*)mtd::valueForModule(
                             (smiColumn*)sColumn + 0,
                             aTemplate->rows[aNode->table].row,
                             MTD_OFFSET_USE,
                             sColumn->module->staticNull );

        sColumn[0].module->null( sColumn + 0, sValue );
    }
    else
    {
        *(mtdDoubleType*)
            ( (UChar*) aTemplate->rows[aNode->table].row
              + sColumn[0].column.offset ) =
            *(mtdDoubleType*)
            ( (UChar*) aTemplate->rows[aNode->table].row
              + sColumn[1].column.offset ) / sCount;
    }
    
    return IDE_SUCCESS;
}

IDE_RC mtfAvgCalculateDouble( mtcNode*     aNode,
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

IDE_RC mtfAvgInitializeBigint(    mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*        aInfo,
                                  mtcTemplate* aTemplate );

IDE_RC mtfAvgAggregateBigint( mtcNode*     aNode,
                              mtcStack*    aStack,
                              SInt         aRemain,
                              void*        aInfo,
                              mtcTemplate* aTemplate );

IDE_RC mtfAvgAggregateBigintFast( mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*        aInfo,
                                  mtcTemplate* aTemplate );

IDE_RC mtfAvgMergeBigint(    mtcNode*     aNode,
                             mtcStack*    aStack,
                             SInt         aRemain,
                             void*        aInfo,
                             mtcTemplate* aTemplate );

IDE_RC mtfAvgFinalizeBigint(    mtcNode*     aNode,
                                mtcStack*    aStack,
                                SInt         aRemain,
                                void*        aInfo,
                                mtcTemplate* aTemplate );

IDE_RC mtfAvgCalculateBigint(    mtcNode*     aNode,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 void*        aInfo,
                                 mtcTemplate* aTemplate );

static const mtcExecute mtfAvgExecuteBigint = {
    mtfAvgInitializeBigint,
    mtfAvgAggregateBigint,
    mtfAvgMergeBigint,
    mtfAvgFinalizeBigint,
    mtfAvgCalculateBigint,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

// 최적의 sum을 수행하는
// aggregate 함수를 포함하고 있는 execute
static const mtcExecute mtfAvgExecuteBigintFast = {
    mtfAvgInitializeBigint,
    mtfAvgAggregateBigintFast,
    mtfAvgMergeBigint,
    mtfAvgFinalizeBigint,
    mtfAvgCalculateBigint,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfAvgEstimateBigint( mtcNode*     aNode,
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
    
    aTemplate->rows[aNode->table].execute[aNode->column] = mtfAvgExecuteBigint;
    
    // 최적화된 aggregate 함수
    sArgModule = aNode->arguments->module;
    if( sArgModule != NULL )
    {
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
                = mtfAvgExecuteBigintFast;
        }
    }

    /*
    IDE_TEST( mtdFloat.estimate( aStack[0].column + 0, 0, 0, 0 )
              != IDE_SUCCESS );
    IDE_TEST( mtdBigint.estimate( aStack[0].column + 1, 0, 0, 0 )
              != IDE_SUCCESS );
    IDE_TEST( mtdBigint.estimate( aStack[0].column + 2, 0, 0, 0 )
              != IDE_SUCCESS );
    IDE_TEST( mtdBigint.estimate( aStack[0].column + 3, 0, 0, 0 )
              != IDE_SUCCESS );
    */

    IDE_TEST( mtc::initializeColumn( aStack[0].column + 0,
                                     & mtdFloat,
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

    IDE_TEST( mtc::initializeColumn( aStack[0].column + 2,
                                     & mtdBigint,
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
    
    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfAvgInitializeBigint( mtcNode*     aNode,
                               mtcStack*,
                               SInt,
                               void*,
                               mtcTemplate* aTemplate )
{
    const mtcColumn* sColumn;
    
    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    
    *(mtdBigintType*)
        ( (UChar*) aTemplate->rows[aNode->table].row
         + sColumn[1].column.offset) = 0;
    *(mtdBigintType*)
        ( (UChar*) aTemplate->rows[aNode->table].row
          + sColumn[2].column.offset) = 0;
    
    return IDE_SUCCESS;
}

IDE_RC mtfAvgAggregateBigint( mtcNode*     aNode,
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
        *(mtdBigintType*)
            ( (UChar*) aTemplate->rows[aNode->table].row
              + sColumn[1].column.offset) += *(mtdBigintType*)aStack[0].value;
        *(mtdBigintType*)
            ( (UChar*) aTemplate->rows[aNode->table].row
              + sColumn[2].column.offset) += 1;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfAvgAggregateBigintFast( mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt,
                                  void*,
                                  mtcTemplate* aTemplate )
{
    mtcNode*         sNode;
    const mtcColumn* sColumn;

    sNode  = aNode->arguments;

    aStack->column = aTemplate->rows[sNode->table].columns + sNode->column;
    aStack->value  = (void*) mtc::value( aStack->column,
                                         aTemplate->rows[sNode->table].row,
                                         MTD_OFFSET_USE );

    // mtdBigint.isNull() 를 호출하는 대신
    // 직접 null 검사를 한다.
    // aStack->value의 데이터 타입을 미리 알기 때문에
    // 직접 null 검사를 하는데 수행 속도를 위해서이다.
    if( *(mtdBigintType*)aStack->value != MTD_BIGINT_NULL )
    {
        sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
        *(mtdBigintType*)
            ((UChar*) aTemplate->rows[aNode->table].row
             + sColumn[1].column.offset) +=
            *(mtdBigintType*)aStack[0].value;
        *(mtdBigintType*)
            ((UChar*)aTemplate->rows[aNode->table].row
             + sColumn[2].column.offset) += 1;
    }

    return IDE_SUCCESS;
}

IDE_RC mtfAvgMergeBigint(    mtcNode*     aNode,
                             mtcStack*    ,
                             SInt         ,
                             void*        aInfo,
                             mtcTemplate* aTemplate )
{
    const mtcColumn* sColumn;
    UChar*           sDstRow;
    UChar*           sSrcRow;

    sDstRow = (UChar*)aTemplate->rows[aNode->table].row;
    sSrcRow = (UChar*)aInfo;
    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;

    //----------------------------------------
    // Merge 작업을 수행한다.
    //----------------------------------------

    // Sum
    *(mtdBigintType*)(sDstRow + sColumn[1].column.offset) +=
    *(mtdBigintType*)(sSrcRow + sColumn[1].column.offset);

    // Count
    *(mtdBigintType*)(sDstRow + sColumn[2].column.offset) +=
    *(mtdBigintType*)(sSrcRow + sColumn[2].column.offset);

    return IDE_SUCCESS;
}

IDE_RC mtfAvgFinalizeBigint( mtcNode*     aNode,
                             mtcStack*,
                             SInt,
                             void*,
                             mtcTemplate* aTemplate )
{
    const mtcColumn* sColumn;
    mtdBigintType    sCount;
    mtdBigintType    sSum;
    void*            sValue;

    mtdNumericType*  sFloatSum;
    mtdNumericType*  sFloatCount;
    UChar            sFloatSumBuff[MTD_FLOAT_SIZE_MAXIMUM];
    UChar            sFloatCountBuff[MTD_FLOAT_SIZE_MAXIMUM];

    
    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    
    sCount = *(mtdBigintType*)
        ( (UChar*) aTemplate->rows[aNode->table].row
          + sColumn[2].column.offset);
    if( sCount == 0 )
    {
        sValue = (void*)mtd::valueForModule(
                             (smiColumn*)sColumn + 0,
                             aTemplate->rows[aNode->table].row,
                             MTD_OFFSET_USE,
                             sColumn->module->staticNull );

        sColumn[0].module->null( sColumn + 0, sValue );
    }
    else
    {
        sSum = *(mtdBigintType*)
            ( (UChar*) aTemplate->rows[aNode->table].row
              + sColumn[1].column.offset);

        sFloatSum = (mtdNumericType*)sFloatSumBuff;
        mtc::makeNumeric( sFloatSum, (SLong)sSum );

        sFloatCount = (mtdNumericType*)sFloatCountBuff;
        mtc::makeNumeric( sFloatCount, (SLong)sCount );

        IDE_TEST( mtc::divideFloat( (mtdNumericType*)
                                    ( (UChar*) aTemplate->rows[aNode->table].row
                                      + sColumn->column.offset),
                                    MTD_FLOAT_PRECISION_MAXIMUM,
                                    sFloatSum,
                                    sFloatCount )
                  != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfAvgCalculateBigint( mtcNode*     aNode,
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
 
