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
 * $Id: mtfRatioToReport.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>
#include <mtuProperty.h>

extern mtfModule mtfRatioToReport;

extern mtdModule mtdFloat;
extern mtdModule mtdDouble;
extern mtdModule mtdList;
extern mtdModule mtdBoolean;

static mtcName mtfRatioToReportFunctionName[1] = {
    { NULL, 15, (void*)"RATIO_TO_REPORT" }
};

static IDE_RC mtfRatioToReportInitialize( void );

static IDE_RC mtfRatioToReportFinalize( void );

static IDE_RC mtfRatioToReportEstimate( mtcNode     * aNode,
                                        mtcTemplate * aTemplate,
                                        mtcStack    * aStack,
                                        SInt          aRemain,
                                        mtcCallBack * aCallBack );

mtfModule mtfRatioToReport = {
    3|MTC_NODE_OPERATOR_AGGREGATION | MTC_NODE_FUNCTION_ANALYTIC_TRUE,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfRatioToReportFunctionName,
    NULL,
    mtfRatioToReportInitialize,
    mtfRatioToReportFinalize,
    mtfRatioToReportEstimate
};

static IDE_RC mtfRatioToReportEstimateFloat( mtcNode     * aNode,
                                             mtcTemplate * aTemplate,
                                             mtcStack    * aStack,
                                             SInt          aRemain,
                                             mtcCallBack * aCallBack );

static IDE_RC mtfRatioToReportEstimateDouble( mtcNode     * aNode,
                                              mtcTemplate * aTemplate,
                                              mtcStack    * aStack,
                                              SInt          aRemain,
                                              mtcCallBack * aCallBack );

static mtfSubModule mtfXX[1] = {
    { NULL, mtf::estimateNA }
};

static mtfSubModule mtfRatioToReportEstimates[2] = {
    { mtfRatioToReportEstimates+1, mtfRatioToReportEstimateDouble },
    { NULL                       , mtfRatioToReportEstimateFloat }
};

static mtfSubModule ** mtfTable = NULL;

IDE_RC mtfRatioToReportInitialize( void )
{
    IDE_TEST( mtf::initializeTemplate( &mtfTable,
                                       mtfRatioToReportEstimates,
                                       mtfXX )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfRatioToReportFinalize( void )
{
    IDE_TEST( mtf::finalizeTemplate( &mtfTable )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfRatioToReportEstimate( mtcNode     * aNode,
                                 mtcTemplate * aTemplate,
                                 mtcStack    * aStack,
                                 SInt          aRemain,
                                 mtcCallBack * aCallBack )
{
    const mtfSubModule * sSubModule = NULL;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_DISTINCT_MASK ) == MTC_NODE_DISTINCT_TRUE,
                    ERR_ARGUMENT_NOT_APPLICABLE );

    IDE_TEST_RAISE( ( aStack[1].column->module == &mtdList ) ||
                    ( aStack[1].column->module == &mtdBoolean ),
                    ERR_ARGUMENT_NOT_APPLICABLE );

    IDE_TEST( mtf::getSubModule1Arg( &sSubModule,
                                     mtfTable,
                                     aStack[1].column->module->no )
              != IDE_SUCCESS );

    IDE_TEST( sSubModule->estimate( aNode,
                                    aTemplate,
                                    aStack,
                                    aRemain,
                                    aCallBack )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));
    }
    IDE_EXCEPTION( ERR_ARGUMENT_NOT_APPLICABLE  );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_ARGUMENT_NOT_APPLICABLE));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfRatioToReportInitializeFloat( mtcNode     * aNode,
                                        mtcStack    * aStack,
                                        SInt          aRemain,
                                        void        * aInfo,
                                        mtcTemplate * aTemplate );

IDE_RC mtfRatioToReportAggregateFloat( mtcNode     * aNode,
                                       mtcStack    * aStack,
                                       SInt          aRemain,
                                       void        * aInfo,
                                       mtcTemplate * aTemplate );

IDE_RC mtfRatioToReportFinalizeFloat( mtcNode     * aNode,
                                      mtcStack    * aStack,
                                      SInt          aRemain,
                                      void        * aInfo,
                                      mtcTemplate * aTemplate );

IDE_RC mtfRatioToReportCalculateFloat( mtcNode     * aNode,
                                       mtcStack    * aStack,
                                       SInt          aRemain,
                                       void        * aInfo,
                                       mtcTemplate * aTemplate );

static const mtcExecute mtfRatioToReportExecuteFloat = {
    mtfRatioToReportInitializeFloat,
    mtfRatioToReportAggregateFloat,
    NULL,
    mtfRatioToReportFinalizeFloat,
    mtfRatioToReportCalculateFloat,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfRatioToReportEstimateFloat( mtcNode     * aNode,
                                      mtcTemplate * aTemplate,
                                      mtcStack    * aStack,
                                      SInt,
                                      mtcCallBack * aCallBack )
{
    const mtdModule * sModules[1];

    mtc::makeFloatConversionModule( aStack + 1, &sModules[0] );

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfRatioToReportExecuteFloat;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     &mtdFloat,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // Sum 결과가 Null인지 아닌지 저장함
    IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                     &mtdBoolean,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    IDE_TEST( mtc::initializeColumn( aStack[0].column + 2,
                                     &mtdFloat,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfRatioToReportInitializeFloat( mtcNode     * aNode,
                                        mtcStack    *,
                                        SInt         ,
                                        void        *,
                                        mtcTemplate * aTemplate )
{
    const mtcColumn * sColumn;
    mtdNumericType  * sFloat = NULL;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;

    sFloat = (mtdNumericType*)((UChar*)aTemplate->rows[aNode->table].row
                               + sColumn[0].column.offset);
    sFloat->length       = 1;
    sFloat->signExponent = 0x80;

    *(mtdBooleanType*) ((UChar*)aTemplate->rows[aNode->table].row
                        + sColumn[1].column.offset) = MTD_BOOLEAN_FALSE;

    sFloat = (mtdNumericType*)((UChar*)aTemplate->rows[aNode->table].row
                               + sColumn[2].column.offset);
    sFloat->length       = 1;
    sFloat->signExponent = 0x80;

    return IDE_SUCCESS;
}

IDE_RC mtfRatioToReportAggregateFloat( mtcNode     * aNode,
                                       mtcStack    * aStack,
                                       SInt          aRemain,
                                       void        *,
                                       mtcTemplate * aTemplate )
{
    const mtdModule * sModule;
    mtcNode         * sNode = NULL;
    const mtcColumn * sColumn;
    mtdNumericType  * sFloatSum = NULL;
    mtdNumericType  * sFloatArgument = NULL;
    UChar             sFloatSumBuff[MTD_FLOAT_SIZE_MAXIMUM];
    mtdNumericType  * sFloatSumClone = NULL;

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

    if ( sNode->conversion != NULL )
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

    sModule = aStack[0].column->module;
    if ( sModule->isNull( aStack[0].column,
                          aStack[0].value ) != ID_TRUE )
    {
        sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
        sFloatSum = (mtdNumericType*)
            ((UChar*) aTemplate->rows[aNode->table].row
             + sColumn[2].column.offset);
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
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfRatioToReportFinalizeFloat( mtcNode     * aNode,
                                      mtcStack    * aStack,
                                      SInt          aRemain,
                                      void        * ,
                                      mtcTemplate * aTemplate )
{
    mtcColumn       * sSumColumn = NULL;
    void            * sValueTemp = NULL;
    mtcNode         * sNode      = NULL;
    mtdNumericType  * sTotalSum  = NULL;
    mtdNumericType  * sFloatArgument = NULL;
    mtdNumericType  * sResult    = NULL;

    sSumColumn = aTemplate->rows[aNode->table].columns + aNode->column;

    if ( *(mtdBooleanType*)( (UChar*) aTemplate->rows[aNode->table].row
                             + sSumColumn[1].column.offset )
        == MTD_BOOLEAN_TRUE )
    {
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

        if ( sNode->conversion != NULL )
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

        sResult = (mtdNumericType*)
                        ((UChar*) aTemplate->rows[aNode->table].row
                                      + sSumColumn[0].column.offset);
        sTotalSum = (mtdNumericType*)
                        ((UChar*) aTemplate->rows[aNode->table].row
                                      + sSumColumn[2].column.offset);

        sFloatArgument = (mtdNumericType*)aStack[0].value;

        IDE_TEST( mtc::divideFloat( sResult,
                                    MTD_FLOAT_PRECISION_MAXIMUM,
                                    sFloatArgument,
                                    sTotalSum )
                  != IDE_SUCCESS );
    }
    else
    {
        sValueTemp = (void*)mtd::valueForModule(
                                 (smiColumn*)sSumColumn,
                                 aTemplate->rows[aNode->table].row,
                                 MTD_OFFSET_USE,
                                 sSumColumn->module->staticNull );

        mtdFloat.null( sSumColumn,
                       sValueTemp );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfRatioToReportCalculateFloat( mtcNode     * aNode,
                                       mtcStack    * aStack,
                                       SInt         ,
                                       void        *,
                                       mtcTemplate * aTemplate )
{
    aStack->column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack->value  = (void*)( (UChar*) aTemplate->rows[aNode->table].row
                              + aStack->column->column.offset );

    return IDE_SUCCESS;
}

IDE_RC mtfRatioToReportInitializeDouble( mtcNode     * aNode,
                                         mtcStack    * aStack,
                                         SInt          aRemain,
                                         void        * aInfo,
                                         mtcTemplate * aTemplate );

IDE_RC mtfRatioToReportAggregateDouble( mtcNode     * aNode,
                                        mtcStack    * aStack,
                                        SInt          aRemain,
                                        void        * aInfo,
                                        mtcTemplate * aTemplate );

IDE_RC mtfRatioToReportAggregateDoubleFast( mtcNode     * aSumNode,
                                            mtcStack    * aStack,
                                            SInt          aRemain,
                                            void        * aInfo,
                                            mtcTemplate * aTemplate );

IDE_RC mtfRatioToReportFinalizeDouble( mtcNode     * aNode,
                                       mtcStack    * aStack,
                                       SInt          aRemain,
                                       void        * aInfo,
                                       mtcTemplate * aTemplate );

IDE_RC mtfRatioToReportFinalizeDoubleFast( mtcNode     * aSumNode,
                                           mtcStack    * aStack,
                                           SInt          aRemain,
                                           void        * aInfo,
                                           mtcTemplate * aTemplate );

IDE_RC mtfRatioToReportCalculateDouble( mtcNode     * aNode,
                                        mtcStack    * aStack,
                                        SInt          aRemain,
                                        void        * aInfo,
                                        mtcTemplate * aTemplate );

static const mtcExecute mtfRatioToReportExecuteDouble = {
    mtfRatioToReportInitializeDouble,
    mtfRatioToReportAggregateDouble,
    NULL,
    mtfRatioToReportFinalizeDouble,
    mtfRatioToReportCalculateDouble,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

// 최적의 sum을 수행하는
// aggregate 함수를 포함하고 있는 execute
static const mtcExecute mtfRatioToReportExecuteDoubleFast = {
    mtfRatioToReportInitializeDouble,
    mtfRatioToReportAggregateDoubleFast,
    NULL,
    mtfRatioToReportFinalizeDoubleFast,
    mtfRatioToReportCalculateDouble,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfRatioToReportEstimateDouble( mtcNode     * aNode,
                                       mtcTemplate * aTemplate,
                                       mtcStack    * aStack,
                                       SInt          ,
                                       mtcCallBack * aCallBack )
{
    const mtfModule * sArgModule;

    static const mtdModule * sModules[1] = {
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

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfRatioToReportExecuteDouble;

    // 최적화된 aggregate 함수
    sArgModule = aNode->arguments->module;

    // mtf::initializeTemplate에서 각 subModule에 대해
    // estimateBound를 호출하는데 이때에는 node에 module이
    // 안달려있기 때문에 NULL 체크를 해야 한다.
    if ( sArgModule != NULL )
    {
        // sum(i1) 처럼 i1가 단일 컬럼이고 conversion되지 않는다면
        // 최적화된 execution을 달아준다.

        // BUG-19856
        // view 컬럼인 경우 최적화된 execution을 달지않는다.
        if ( ( ( aTemplate->rows[aNode->arguments->table].lflag
                 & MTC_TUPLE_VIEW_MASK )
               == MTC_TUPLE_VIEW_FALSE ) &&
             ( idlOS::strncmp((SChar*)sArgModule->names->string,
                              (const SChar*)"COLUMN", 6 )
               == 0 ) &&
             ( aNode->arguments->conversion == NULL ) )
        {
            aTemplate->rows[aNode->table].execute[aNode->column]
                = mtfRatioToReportExecuteDoubleFast;
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

    //IDE_TEST( mtdDouble.estimate( aStack[0].column, 0, 0, 0 )
    //          != IDE_SUCCESS );
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     &mtdDouble,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // Sum 결과가 Null인지 아닌지 저장함
    IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                     &mtdBoolean,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    IDE_TEST( mtc::initializeColumn( aStack[0].column + 2,
                                     &mtdDouble,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfRatioToReportInitializeDouble( mtcNode     * aNode,
                                         mtcStack    *,
                                         SInt         ,
                                         void        *,
                                         mtcTemplate * aTemplate )
{
    const mtcColumn * sColumn;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;

    *(mtdDoubleType*)((UChar*) aTemplate->rows[aNode->table].row
                      + sColumn->column.offset) = 0;
    *(mtdBooleanType*)((UChar*) aTemplate->rows[aNode->table].row
                       + sColumn[1].column.offset) = MTD_BOOLEAN_FALSE;

    *(mtdDoubleType*)((UChar*) aTemplate->rows[aNode->table].row
                      + sColumn[2].column.offset) = 0;
    return IDE_SUCCESS;
}

IDE_RC mtfRatioToReportAggregateDouble( mtcNode     * aNode,
                                        mtcStack    * aStack,
                                        SInt          aRemain,
                                        void        *,
                                        mtcTemplate * aTemplate )
{
    mtcNode         * sNode = NULL;
    const mtcColumn * sColumn;

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

    if ( sNode->conversion != NULL )
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

    // mtdDouble.isNull() 를 호출하는 대신
    // 직접 null 검사를 한다.
    // aStack->value의 데이터 타입을 미리 알기 때문에
    // 직접 null 검사를 하는데 수행 속도를 위해서이다.
    if ( ( *(ULong*)(aStack->value) & MTD_DOUBLE_EXPONENT_MASK )
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
                              + sColumn[2].column.offset) +=
                *(mtdDoubleType*)aStack[0].value;
        }
        else
        {
            *(mtdDoubleType*)((UChar*)aTemplate->rows[aNode->table].row
                              + sColumn[2].column.offset) =
                *(mtdDoubleType*)aStack[0].value;
        }

        *(mtdBooleanType*)((UChar*)aTemplate->rows[aNode->table].row
                           + sColumn[1].column.offset) = MTD_BOOLEAN_TRUE;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfRatioToReportFinalizeDouble( mtcNode     * aNode,
                                       mtcStack    * aStack,
                                       SInt          aRemain,
                                       void        * ,
                                       mtcTemplate * aTemplate )
{
    mtcColumn       * sColumn = NULL;
    void            * sValueTemp = NULL;
    mtcNode         * sNode = NULL;
    mtdDoubleType     sTotalSum;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;

    if ( *(mtdBooleanType*)( (UChar*) aTemplate->rows[aNode->table].row
                             + sColumn[1].column.offset )
         == MTD_BOOLEAN_TRUE )
    {
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

        if ( sNode->conversion != NULL )
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
        sTotalSum = *(mtdDoubleType*)((UChar*)aTemplate->rows[aNode->table].row
                                      + sColumn[2].column.offset);

        *(mtdDoubleType*)((UChar*)aTemplate->rows[aNode->table].row
                          + sColumn->column.offset) = *(mtdDoubleType*)aStack[0].value / sTotalSum;
    }
    else
    {
        sValueTemp = (void*)mtd::valueForModule(
                                 (smiColumn*)sColumn,
                                 aTemplate->rows[aNode->table].row,
                                 MTD_OFFSET_USE,
                                 sColumn->module->staticNull );

        mtdDouble.null( sColumn,
                        sValueTemp );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfRatioToReportAggregateDoubleFast( mtcNode     * aSumNode,
                                            mtcStack    * aStack,
                                            SInt          ,
                                            void        * ,
                                            mtcTemplate * aTemplate )
{
    mtcNode         * sArgumentNode = NULL;
    const mtcColumn * sSumColumn;

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
    if ( ( *(ULong*)(aStack->value) & MTD_DOUBLE_EXPONENT_MASK )
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
                               + sSumColumn[2].column.offset) +=
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
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;
}

IDE_RC mtfRatioToReportFinalizeDoubleFast( mtcNode     * aSumNode,
                                           mtcStack    * aStack,
                                           SInt          ,
                                           void        * ,
                                           mtcTemplate * aTemplate )
{
    mtcColumn     * sSumColumn = NULL;
    void          * sValueTemp = NULL;
    mtcNode       * sArgumentNode = NULL;
    mtdDoubleType   sTotalSum;

    sSumColumn = aTemplate->rows[aSumNode->table].columns + aSumNode->column;

    if ( *(mtdBooleanType*)( (UChar*) aTemplate->rows[aSumNode->table].row
                             + sSumColumn[1].column.offset )
         == MTD_BOOLEAN_TRUE )
    {
        sArgumentNode  = aSumNode->arguments;

        aStack->column = aTemplate->rows[sArgumentNode->table].columns
            + sArgumentNode->column;
        aStack->value  = (void*) mtc::value( aStack->column,
                                             aTemplate->rows[sArgumentNode->table].row,
                                             MTD_OFFSET_USE );

        sTotalSum = *(mtdDoubleType*) ((UChar*) aTemplate->rows[aSumNode->table].row
                                      + sSumColumn[2].column.offset);
        *(mtdDoubleType*) ((UChar*) aTemplate->rows[aSumNode->table].row
                           + sSumColumn->column.offset) = *(mtdDoubleType *)aStack->value / sTotalSum;
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
IDE_RC mtfRatioToReportCalculateDouble( mtcNode     * aNode,
                                        mtcStack    * aStack,
                                        SInt          ,
                                        void        * ,
                                        mtcTemplate * aTemplate )
{
    aStack->column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack->value  = (void*)( (UChar*) aTemplate->rows[aNode->table].row
                              + aStack->column->column.offset );

    return IDE_SUCCESS;
}

