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
 * $Id: mtfDecodeAvg.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>

extern mtfModule mtfDecodeAvg;

extern mtdModule mtdFloat;
extern mtdModule mtdDouble;
extern mtdModule mtdBigint;
extern mtdModule mtdList;
extern mtdModule mtdBoolean;
extern mtdModule mtdBinary;

static mtcName mtfDecodeAvgFunctionName[1] = {
    { NULL, 10, (void*)"DECODE_AVG" }
};

static IDE_RC mtfDecodeAvgInitialize( void );

static IDE_RC mtfDecodeAvgFinalize( void );

static IDE_RC mtfDecodeAvgEstimate( mtcNode*     aNode,
                                    mtcTemplate* aTemplate,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    mtcCallBack* aCallBack );

mtfModule mtfDecodeAvg = {
    2|MTC_NODE_OPERATOR_AGGREGATION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfDecodeAvgFunctionName,
    NULL,
    mtfDecodeAvgInitialize,
    mtfDecodeAvgFinalize,
    mtfDecodeAvgEstimate
};

static IDE_RC mtfDecodeAvgEstimateFloat( mtcNode*     aNode,
                                         mtcTemplate* aTemplate,
                                         mtcStack*    aStack,
                                         SInt         aRemain,
                                         mtcCallBack* aCallBack );

IDE_RC mtfDecodeAvgInitializeFloat( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate );

IDE_RC mtfDecodeAvgAggregateFloat( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate );

IDE_RC mtfDecodeAvgFinalizeFloat( mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*        aInfo,
                                  mtcTemplate* aTemplate );

IDE_RC mtfDecodeAvgCalculateFloat( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate );

static const mtcExecute mtfDecodeAvgExecuteFloat = {
    mtfDecodeAvgInitializeFloat,
    mtfDecodeAvgAggregateFloat,
    mtf::calculateNA,
    mtfDecodeAvgFinalizeFloat,
    mtfDecodeAvgCalculateFloat,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

static IDE_RC mtfDecodeAvgEstimateDouble( mtcNode*     aNode,
                                          mtcTemplate* aTemplate,
                                          mtcStack*    aStack,
                                          SInt         aRemain,
                                          mtcCallBack* aCallBack );

IDE_RC mtfDecodeAvgInitializeDouble( mtcNode*     aNode,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     void*        aInfo,
                                     mtcTemplate* aTemplate );

IDE_RC mtfDecodeAvgAggregateDouble( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate );

IDE_RC mtfDecodeAvgFinalizeDouble( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate );

IDE_RC mtfDecodeAvgCalculateDouble( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate );

static const mtcExecute mtfDecodeAvgExecuteDouble = {
    mtfDecodeAvgInitializeDouble,
    mtfDecodeAvgAggregateDouble,
    mtf::calculateNA,
    mtfDecodeAvgFinalizeDouble,
    mtfDecodeAvgCalculateDouble,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

static IDE_RC mtfDecodeAvgEstimateBigint( mtcNode*     aNode,
                                          mtcTemplate* aTemplate,
                                          mtcStack*    aStack,
                                          SInt         aRemain,
                                          mtcCallBack* aCallBack );

IDE_RC mtfDecodeAvgInitializeBigint( mtcNode*     aNode,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     void*        aInfo,
                                     mtcTemplate* aTemplate );

IDE_RC mtfDecodeAvgAggregateBigint( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate );

IDE_RC mtfDecodeAvgFinalizeBigint( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate );

IDE_RC mtfDecodeAvgCalculateBigint( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate );

static const mtcExecute mtfDecodeAvgExecuteBigint = {
    mtfDecodeAvgInitializeBigint,
    mtfDecodeAvgAggregateBigint,
    mtf::calculateNA,
    mtfDecodeAvgFinalizeBigint,
    mtfDecodeAvgCalculateBigint,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

typedef struct mtfDecodeAvgInfo
{
    // 첫번째 인자
    mtcExecute   * sAvgColumnExecute;
    mtcNode      * sAvgColumnNode;

    // 두번째 인자
    mtcExecute   * sExprExecute;
    mtcNode      * sExprNode;

    // 세번째 인자
    mtcExecute   * sSearchExecute;
    mtcNode      * sSearchNode;

    // return 인자
    mtcColumn    * sReturnColumn;
    void         * sReturnValue;

    // 임시변수
    ULong          sSumCount;
    mtdBigintType  sBigintSum;
} mtfDecodeAvgInfo;

static mtfSubModule mtfXX[1] = {
    { NULL, mtf::estimateNA }
};

static mtfSubModule mtfDecodeAvgEstimates[3] = {
    { mtfDecodeAvgEstimates+1, mtfDecodeAvgEstimateFloat  },
    { mtfDecodeAvgEstimates+2, mtfDecodeAvgEstimateDouble },
    { NULL,                    mtfDecodeAvgEstimateBigint }
};

static mtfSubModule** mtfTable = NULL;

IDE_RC mtfDecodeAvgInitialize( void )
{
    return mtf::initializeTemplate( &mtfTable, mtfDecodeAvgEstimates, mtfXX );
}

IDE_RC mtfDecodeAvgFinalize( void )
{
    return mtf::finalizeTemplate( &mtfTable );
}

IDE_RC mtfDecodeAvgEstimate( mtcNode*     aNode,
                             mtcTemplate* aTemplate,
                             mtcStack*    aStack,
                             SInt         aRemain,
                             mtcCallBack* aCallBack )
{
#define IDE_FN "IDE_RC mtfDecodeAvgEstimate"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtfSubModule* sSubModule;
    UInt                sFence;

    sFence = aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK;

    // 1 혹은 3개의 인자
    IDE_TEST_RAISE( (sFence != 1) && (sFence != 3),
                    ERR_INVALID_FUNCTION_ARGUMENT );

    IDE_TEST_RAISE( aStack[1].column->module == &mtdList,
                    ERR_CONVERSION_NOT_APPLICABLE );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

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

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

/* ZONE: FLOAT */

IDE_RC mtfDecodeAvgEstimateFloat( mtcNode*     aNode,
                                  mtcTemplate* aTemplate,
                                  mtcStack*    aStack,
                                  SInt,
                                  mtcCallBack* aCallBack )
{
#define IDE_FN "IDE_RC mtfDecodeAvgEstimateFloat"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtdModule * sTarget;
    const mtdModule * sModules[MTC_NODE_ARGUMENT_COUNT_MAXIMUM];
    mtcStack        * sStack1;
    mtcStack        * sStack2;
    UInt              sBinaryPrecision;
    UInt              sCount;
    UInt              sFence;

    sFence = aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK;

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    if ( sFence == 3 )
    {
        if ( aStack[2].column->module != &mtdList )
        {
            IDE_TEST( mtf::getComparisonModule(
                          &sTarget,
                          aStack[2].column->module->no,
                          aStack[3].column->module->no )
                      != IDE_SUCCESS );

            IDE_TEST_RAISE( sTarget == NULL,
                            ERR_CONVERSION_NOT_APPLICABLE );

            // To Fix PR-15208
            IDE_TEST_RAISE( mtf::isEquiValidType( sTarget ) != ID_TRUE,
                            ERR_CONVERSION_NOT_APPLICABLE );

            sModules[0] = & mtdFloat;
            sModules[1] = sTarget;
            sModules[2] = sTarget;

            IDE_TEST( mtf::makeConversionNodes( aNode,
                                                aNode->arguments,
                                                aTemplate,
                                                aStack + 1,
                                                aCallBack,
                                                sModules )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST_RAISE( aStack[3].column->module != &mtdList,
                            ERR_CONVERSION_NOT_APPLICABLE );

            IDE_TEST_RAISE( (aStack[2].column->precision !=
                             aStack[3].column->precision) ||
                            (aStack[2].column->precision <= 0),
                            ERR_INVALID_FUNCTION_ARGUMENT );
            sStack1 = (mtcStack*)aStack[2].value;
            sStack2 = (mtcStack*)aStack[3].value;

            for( sCount = 0, sFence = aStack[2].column->precision;
                 sCount < sFence;
                 sCount++ )
            {
                IDE_TEST( mtf::getComparisonModule(
                              &sTarget,
                              sStack1[sCount].column->module->no,
                              sStack2[sCount].column->module->no )
                          != IDE_SUCCESS );

                IDE_TEST_RAISE( (sTarget == NULL) || (sTarget == &mtdList),
                                ERR_CONVERSION_NOT_APPLICABLE );

                // To Fix PR-15208
                IDE_TEST_RAISE( mtf::isEquiValidType( sTarget ) != ID_TRUE,
                                ERR_CONVERSION_NOT_APPLICABLE );

                sModules[sCount] = sTarget;
            }
            IDE_TEST( mtf::makeConversionNodes( aNode,
                                                aNode->arguments->next->arguments,
                                                aTemplate,
                                                sStack1,
                                                aCallBack,
                                                sModules )
                      != IDE_SUCCESS );
            IDE_TEST( mtf::makeConversionNodes( aNode,
                                                aNode->arguments->next->next->arguments,
                                                aTemplate,
                                                sStack2,
                                                aCallBack,
                                                sModules )
                      != IDE_SUCCESS );
            sModules[0] = & mtdFloat;
            sModules[1] = & mtdList;
            sModules[2] = & mtdList;

            IDE_TEST( mtf::makeConversionNodes( aNode,
                                                aNode->arguments,
                                                aTemplate,
                                                aStack + 1,
                                                aCallBack,
                                                sModules )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        sModules[0] = & mtdFloat;
        IDE_TEST( mtf::makeConversionNodes( aNode,
                                            aNode->arguments,
                                            aTemplate,
                                            aStack + 1,
                                            aCallBack,
                                            sModules )
                  != IDE_SUCCESS );
    }

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfDecodeAvgExecuteFloat;

    // avg 결과를 저장함
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdFloat,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );
    // avg info 정보를 mtdBinary에 저장
    sBinaryPrecision = ID_SIZEOF(mtfDecodeAvgInfo);
    IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                     & mtdBinary,
                                     1,
                                     sBinaryPrecision,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC mtfDecodeAvgInitializeFloat( mtcNode*     aNode,
                                    mtcStack*,
                                    SInt,
                                    void*,
                                    mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeAvgInitializeFloat"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtcColumn   * sColumn;
    mtcNode           * sArgNode[3];
    mtdNumericType    * sFloat;
    mtdBinaryType     * sValue;
    mtfDecodeAvgInfo  * sInfo;
    UInt                sFence;

    sFence = aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sValue = (mtdBinaryType*)
        ((UChar*)aTemplate->rows[aNode->table].row + sColumn[1].column.offset);

    // BUG-43709
    sValue->mLength = sColumn[1].precision;
    sInfo = (mtfDecodeAvgInfo*)(sValue->mValue);

    //-----------------------------
    // avg info 초기화
    //-----------------------------

    sArgNode[0] = aNode->arguments;

    // avg column 설정
    sInfo->sAvgColumnExecute = aTemplate->rows[sArgNode[0]->table].execute + sArgNode[0]->column;
    sInfo->sAvgColumnNode    = sArgNode[0];

    if ( sFence == 3 )
    {
        sArgNode[1] = sArgNode[0]->next;
        sArgNode[2] = sArgNode[1]->next;

        // expression column 설정
        sInfo->sExprExecute = aTemplate->rows[sArgNode[1]->table].execute + sArgNode[1]->column;
        sInfo->sExprNode    = sArgNode[1];

        // search value 설정
        sInfo->sSearchExecute = aTemplate->rows[sArgNode[2]->table].execute + sArgNode[2]->column;
        sInfo->sSearchNode    = sArgNode[2];
    }
    else
    {
        sInfo->sExprExecute = NULL;
        sInfo->sExprNode    = NULL;

        sInfo->sSearchExecute = NULL;
        sInfo->sSearchNode    = NULL;
    }

    // return column 설정
    sInfo->sReturnColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sInfo->sReturnValue  = (void *)
        ((UChar*) aTemplate->rows[aNode->table].row + sInfo->sReturnColumn->column.offset);

    // 임시변수 초기화
    sInfo->sSumCount  = 0;
    sInfo->sBigintSum = 0;

    //-----------------------------
    // avg 결과를 초기화
    //-----------------------------
    sFloat = (mtdNumericType*) sInfo->sReturnValue;
    sFloat->length       = 1;
    sFloat->signExponent = 0x80;

    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC mtfDecodeAvgAggregateFloat( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*,
                                   mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeAvgAggregateFloat"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtdModule   * sModule;
    const mtcColumn   * sColumn;
    mtdNumericType    * sFloatSum;
    mtdNumericType    * sFloatArgument;
    UChar               sFloatSumBuff[MTD_FLOAT_SIZE_MAXIMUM];
    mtdNumericType    * sFloatSumClone;
    mtdBinaryType     * sValue;
    mtfDecodeAvgInfo  * sInfo;
    mtdValueInfo        sValueInfo1;
    mtdValueInfo        sValueInfo2;
    mtcStack          * sStack1;
    mtcStack          * sStack2;
    SInt                sCompare;
    UInt                sCount;
    UInt                sFence;

    sFence = aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sValue = (mtdBinaryType*)((UChar*)aTemplate->rows[aNode->table].row
                              + sColumn[1].column.offset);
    sInfo = (mtfDecodeAvgInfo*)(sValue->mValue);

    if ( sFence == 3 )
    {
        IDE_TEST_RAISE( aRemain < 2, ERR_STACK_OVERFLOW );

        // 두번째 인자
        IDE_TEST( sInfo->sExprExecute->calculate( sInfo->sExprNode,
                                                  aStack,
                                                  aRemain,
                                                  sInfo->sExprExecute->calculateInfo,
                                                  aTemplate )
                  != IDE_SUCCESS );

        if( sInfo->sExprNode->conversion != NULL )
        {
            IDE_TEST( mtf::convertCalculate( sInfo->sExprNode,
                                             aStack,
                                             aRemain,
                                             NULL,
                                             aTemplate )
                      != IDE_SUCCESS );
        }

        // 세번째 인자
        IDE_TEST( sInfo->sSearchExecute->calculate( sInfo->sSearchNode,
                                                    aStack + 1,
                                                    aRemain - 1,
                                                    sInfo->sSearchExecute->calculateInfo,
                                                    aTemplate )
                  != IDE_SUCCESS );

        if( sInfo->sSearchNode->conversion != NULL )
        {
            IDE_TEST( mtf::convertCalculate( sInfo->sSearchNode,
                                             aStack + 1,
                                             aRemain - 1,
                                             NULL,
                                             aTemplate )
                      != IDE_SUCCESS );
        }

        // decode 연산수행
        if ( aStack[0].column->module != &mtdList )
        {
            IDE_DASSERT( aStack[0].column->module == aStack[1].column->module );

            sModule = aStack[0].column->module;

            if ( ( sModule->isNull( aStack[0].column,
                                    aStack[0].value ) == ID_TRUE ) ||
                 ( sModule->isNull( aStack[1].column,
                                    aStack[1].value ) == ID_TRUE ) )
            {
                sCompare = -1;
            }
            else
            {
                sValueInfo1.column = aStack[0].column;
                sValueInfo1.value  = aStack[0].value;
                sValueInfo1.flag   = MTD_OFFSET_USELESS;

                sValueInfo2.column = aStack[1].column;
                sValueInfo2.value  = aStack[1].value;
                sValueInfo2.flag   = MTD_OFFSET_USELESS;

                // 두번째 인자와 세번째 인자의 비교
                sCompare = sModule->logicalCompare[MTD_COMPARE_ASCENDING]( &sValueInfo1,
                                                                           &sValueInfo2 );
            }
        }
        else
        {
            IDE_DASSERT( aStack[0].column->module == &mtdList );
            IDE_DASSERT( (aStack[0].column->precision ==
                          aStack[1].column->precision) &&
                         (aStack[0].column->precision > 0) );

            sStack1 = (mtcStack*)aStack[0].value;
            sStack2 = (mtcStack*)aStack[1].value;

            for( sCount = 0, sFence = aStack[0].column->precision, sCompare = 0;
                 (sCount < sFence) && (sCompare == 0);
                 sCount++ )
            {
                IDE_DASSERT( sStack1[sCount].column->module == sStack2[sCount].column->module );

                sModule = sStack1[sCount].column->module;

                if ( ( sModule->isNull( sStack1[sCount].column,
                                        sStack1[sCount].value ) == ID_TRUE ) ||
                     ( sModule->isNull( sStack2[sCount].column,
                                        sStack2[sCount].value ) == ID_TRUE ) )
                {
                    sCompare = -1;
                }
                else
                {
                    sValueInfo1.column = sStack1[sCount].column;
                    sValueInfo1.value  = sStack1[sCount].value;
                    sValueInfo1.flag   = MTD_OFFSET_USELESS;

                    sValueInfo2.column = sStack2[sCount].column;
                    sValueInfo2.value  = sStack2[sCount].value;
                    sValueInfo2.flag   = MTD_OFFSET_USELESS;

                    sCompare = sModule->logicalCompare[MTD_COMPARE_ASCENDING]( &sValueInfo1,
                                                                               &sValueInfo2 );
                }
            }
        }
    }
    else
    {
        IDE_TEST_RAISE( aRemain < 1, ERR_STACK_OVERFLOW );
        sCompare = 0;
    }

    // 첫번째 인자
    IDE_TEST( sInfo->sAvgColumnExecute->calculate( sInfo->sAvgColumnNode,
                                                   aStack,
                                                   aRemain,
                                                   sInfo->sAvgColumnExecute->calculateInfo,
                                                   aTemplate )
              != IDE_SUCCESS );

    if( sInfo->sAvgColumnNode->conversion != NULL )
    {
        IDE_TEST( mtf::convertCalculate( sInfo->sAvgColumnNode,
                                         aStack,
                                         aRemain,
                                         NULL,
                                         aTemplate )
                  != IDE_SUCCESS );
    }

    IDE_DASSERT( aStack[0].column->module == & mtdFloat );

    sModule = aStack[0].column->module;

    if ( sModule->isNull( aStack[0].column,
                          aStack[0].value ) != ID_TRUE )
    {
        if ( sCompare == 0 )
        {
            sFloatSum      = (mtdNumericType*)sInfo->sReturnValue;
            sFloatArgument = (mtdNumericType*)aStack[0].value;
            sFloatSumClone = (mtdNumericType*)sFloatSumBuff;
            idlOS::memcpy( sFloatSumClone, sFloatSum, sFloatSum->length + 1 );

            IDE_TEST( mtc::addFloat( sFloatSum,
                                     MTD_FLOAT_PRECISION_MAXIMUM,
                                     sFloatSumClone,
                                     sFloatArgument )
                      != IDE_SUCCESS );
            sInfo->sSumCount++;
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC mtfDecodeAvgFinalizeFloat( mtcNode*     aNode,
                                  mtcStack*,
                                  SInt,
                                  void*,
                                  mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeAvgFinalizeFloat"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtcColumn   * sColumn;
    mtdBinaryType     * sValue;
    mtfDecodeAvgInfo  * sInfo;
    mtdNumericType    * sFloatResult;
    UChar               sFloatSumBuff[MTD_FLOAT_SIZE_MAXIMUM];
    mtdNumericType    * sFloatSum;
    UChar               sFloatCountBuff[MTD_FLOAT_SIZE_MAXIMUM];
    mtdNumericType    * sFloatCount;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sValue = (mtdBinaryType*)((UChar*)aTemplate->rows[aNode->table].row
                              + sColumn[1].column.offset);
    sInfo = (mtfDecodeAvgInfo*)(sValue->mValue);

    if( sInfo->sSumCount == 0 )
    {
        mtdFloat.null( sInfo->sReturnColumn,
                       sInfo->sReturnValue );
    }
    else
    {
        // sum
        sFloatResult   = (mtdNumericType*)sInfo->sReturnValue;
        sFloatSum = (mtdNumericType*)sFloatSumBuff;
        idlOS::memcpy( sFloatSum, sFloatResult, sFloatResult->length + 1 );

        // count
        sFloatCount = (mtdNumericType*)sFloatCountBuff;
        mtc::makeNumeric( sFloatCount, (SLong)sInfo->sSumCount );

        IDE_TEST( mtc::divideFloat( sFloatResult,
                                    MTD_FLOAT_PRECISION_MAXIMUM,
                                    sFloatSum,
                                    sFloatCount )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC mtfDecodeAvgCalculateFloat( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt,
                                   void*,
                                   mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeAvgCalculateFloat"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    aStack->column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack->value  = (void*)( (UChar*) aTemplate->rows[aNode->table].row
                              + aStack->column->column.offset );

    return IDE_SUCCESS;

#undef IDE_FN
}

/* ZONE: DOUBLE */

IDE_RC mtfDecodeAvgEstimateDouble( mtcNode*     aNode,
                                   mtcTemplate* aTemplate,
                                   mtcStack*    aStack,
                                   SInt,
                                   mtcCallBack* aCallBack )
{
#define IDE_FN "IDE_RC mtfDecodeAvgEstimateDouble"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtdModule * sTarget;
    const mtdModule * sModules[MTC_NODE_ARGUMENT_COUNT_MAXIMUM];
    mtcStack        * sStack1;
    mtcStack        * sStack2;
    UInt              sBinaryPrecision;
    UInt              sCount;
    UInt              sFence;

    sFence = aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK;

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    if ( sFence == 3 )
    {
        if ( aStack[2].column->module != &mtdList )
        {
            IDE_TEST( mtf::getComparisonModule(
                          &sTarget,
                          aStack[2].column->module->no,
                          aStack[3].column->module->no )
                      != IDE_SUCCESS );

            IDE_TEST_RAISE( sTarget == NULL,
                            ERR_CONVERSION_NOT_APPLICABLE );

            // To Fix PR-15208
            IDE_TEST_RAISE( mtf::isEquiValidType( sTarget ) != ID_TRUE,
                            ERR_CONVERSION_NOT_APPLICABLE );
            sModules[0] = & mtdDouble;
            sModules[1] = sTarget;
            sModules[2] = sTarget;

            IDE_TEST( mtf::makeConversionNodes( aNode,
                                                aNode->arguments,
                                                aTemplate,
                                                aStack + 1,
                                                aCallBack,
                                                sModules )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST_RAISE( aStack[3].column->module != &mtdList,
                            ERR_CONVERSION_NOT_APPLICABLE );
            IDE_TEST_RAISE( (aStack[2].column->precision !=
                             aStack[3].column->precision) ||
                            (aStack[2].column->precision <= 0),
                            ERR_INVALID_FUNCTION_ARGUMENT );

            sStack1 = (mtcStack*)aStack[2].value;
            sStack2 = (mtcStack*)aStack[3].value;

            for( sCount = 0, sFence = aStack[2].column->precision;
                 sCount < sFence;
                 sCount++ )
            {
                IDE_TEST( mtf::getComparisonModule(
                              &sTarget,
                              sStack1[sCount].column->module->no,
                              sStack2[sCount].column->module->no )
                          != IDE_SUCCESS );
                IDE_TEST_RAISE( (sTarget == NULL) || (sTarget == &mtdList),
                                ERR_CONVERSION_NOT_APPLICABLE );
                // To Fix PR-15208
                IDE_TEST_RAISE( mtf::isEquiValidType( sTarget ) != ID_TRUE,
                                ERR_CONVERSION_NOT_APPLICABLE );
                sModules[sCount] = sTarget;
            }

            IDE_TEST( mtf::makeConversionNodes( aNode,
                                                aNode->arguments->next->arguments,
                                                aTemplate,
                                                sStack1,
                                                aCallBack,
                                                sModules )
                      != IDE_SUCCESS );

            IDE_TEST( mtf::makeConversionNodes( aNode,
                                                aNode->arguments->next->next->arguments,
                                                aTemplate,
                                                sStack2,
                                                aCallBack,
                                                sModules )
                      != IDE_SUCCESS );
            sModules[0] = & mtdDouble;
            sModules[1] = & mtdList;
            sModules[2] = & mtdList;

            IDE_TEST( mtf::makeConversionNodes( aNode,
                                                aNode->arguments,
                                                aTemplate,
                                                aStack + 1,
                                                aCallBack,
                                                sModules )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        sModules[0] = & mtdDouble;

        IDE_TEST( mtf::makeConversionNodes( aNode,
                                            aNode->arguments,
                                            aTemplate,
                                            aStack + 1,
                                            aCallBack,
                                            sModules )
                  != IDE_SUCCESS );
    }

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfDecodeAvgExecuteDouble;

    // avg 결과를 저장함
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdDouble,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // avg info 정보를 mtdBinary에 저장
    sBinaryPrecision = ID_SIZEOF(mtfDecodeAvgInfo);
    IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                     & mtdBinary,
                                     1,
                                     sBinaryPrecision,
                                     0 )
              != IDE_SUCCESS );
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
#undef IDE_FN
}

IDE_RC mtfDecodeAvgInitializeDouble( mtcNode*     aNode,
                                     mtcStack*,
                                     SInt,
                                     void*,
                                     mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeAvgInitializeDouble"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtcColumn   * sColumn;
    mtcNode           * sArgNode[3];
    mtdBinaryType     * sValue;
    mtfDecodeAvgInfo  * sInfo;
    UInt                sFence;

    sFence = aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sValue = (mtdBinaryType*)
        ((UChar*)aTemplate->rows[aNode->table].row + sColumn[1].column.offset);

    // BUG-43709
    sValue->mLength = sColumn[1].precision;
    sInfo = (mtfDecodeAvgInfo*)(sValue->mValue);

    //-----------------------------
    // avg info 초기화
    //-----------------------------
    sArgNode[0] = aNode->arguments;

    // avg column 설정
    sInfo->sAvgColumnExecute = aTemplate->rows[sArgNode[0]->table].execute + sArgNode[0]->column;
    sInfo->sAvgColumnNode    = sArgNode[0];

    if ( sFence == 3 )
    {
        sArgNode[1] = sArgNode[0]->next;
        sArgNode[2] = sArgNode[1]->next;

        // expression column 설정
        sInfo->sExprExecute = aTemplate->rows[sArgNode[1]->table].execute + sArgNode[1]->column;
        sInfo->sExprNode    = sArgNode[1];

        // search value 설정
        sInfo->sSearchExecute = aTemplate->rows[sArgNode[2]->table].execute + sArgNode[2]->column;
        sInfo->sSearchNode    = sArgNode[2];
    }
    else
    {
        sInfo->sExprExecute = NULL;
        sInfo->sExprNode    = NULL;

        sInfo->sSearchExecute = NULL;
        sInfo->sSearchNode    = NULL;
    }

    // return column 설정
    sInfo->sReturnColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sInfo->sReturnValue  = (void *)
        ((UChar*) aTemplate->rows[aNode->table].row + sInfo->sReturnColumn->column.offset);

    // 임시변수 초기화
    sInfo->sSumCount  = 0;
    sInfo->sBigintSum = 0;

    //-----------------------------
    // avg 결과를 초기화
    //-----------------------------

    *(mtdDoubleType*)(sInfo->sReturnValue) = 0;

    return IDE_SUCCESS;
#undef IDE_FN
}

IDE_RC mtfDecodeAvgAggregateDouble( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*,
                                    mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeAvgAggregateDouble"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtdModule   * sModule;
    const mtcColumn   * sColumn;
    mtdBinaryType     * sValue;
    mtfDecodeAvgInfo  * sInfo;
    mtdValueInfo        sValueInfo1;
    mtdValueInfo        sValueInfo2;
    mtcStack          * sStack1;
    mtcStack          * sStack2;
    SInt                sCompare;
    UInt                sCount;
    UInt                sFence;

    sFence = aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sValue = (mtdBinaryType*)((UChar*)aTemplate->rows[aNode->table].row
                              + sColumn[1].column.offset);
    sInfo = (mtfDecodeAvgInfo*)(sValue->mValue);

    if ( sFence == 3 )
    {
        IDE_TEST_RAISE( aRemain < 2, ERR_STACK_OVERFLOW );

        // 두번째 인자
        IDE_TEST( sInfo->sExprExecute->calculate( sInfo->sExprNode,
                                                  aStack,
                                                  aRemain,
                                                  sInfo->sExprExecute->calculateInfo,
                                                  aTemplate )
                  != IDE_SUCCESS );

        if( sInfo->sExprNode->conversion != NULL )
        {
            IDE_TEST( mtf::convertCalculate( sInfo->sExprNode,
                                             aStack,
                                             aRemain,
                                             NULL,
                                             aTemplate )
                      != IDE_SUCCESS );
        }

        // 세번째 인자
        IDE_TEST( sInfo->sSearchExecute->calculate( sInfo->sSearchNode,
                                                    aStack + 1,
                                                    aRemain - 1,
                                                    sInfo->sSearchExecute->calculateInfo,
                                                    aTemplate )
                  != IDE_SUCCESS );

        if( sInfo->sSearchNode->conversion != NULL )
        {
            IDE_TEST( mtf::convertCalculate( sInfo->sSearchNode,
                                             aStack + 1,
                                             aRemain - 1,
                                             NULL,
                                             aTemplate )
                      != IDE_SUCCESS );
        }

        // decode 연산수행
        if ( aStack[0].column->module != &mtdList )
        {
            IDE_DASSERT( aStack[0].column->module == aStack[1].column->module );

            sModule = aStack[0].column->module;

            if ( ( sModule->isNull( aStack[0].column,
                                    aStack[0].value ) == ID_TRUE ) ||
                 ( sModule->isNull( aStack[1].column,
                                    aStack[1].value ) == ID_TRUE ) )
            {
                sCompare = -1;
            }
            else
            {
                sValueInfo1.column = aStack[0].column;
                sValueInfo1.value  = aStack[0].value;
                sValueInfo1.flag   = MTD_OFFSET_USELESS;

                sValueInfo2.column = aStack[1].column;
                sValueInfo2.value  = aStack[1].value;
                sValueInfo2.flag   = MTD_OFFSET_USELESS;

                // 두번째 인자와 세번째 인자의 비교
                sCompare = sModule->logicalCompare[MTD_COMPARE_ASCENDING]( &sValueInfo1,
                                                                           &sValueInfo2 );
            }
        }
        else
        {
            IDE_DASSERT( aStack[0].column->module == &mtdList );

            IDE_DASSERT( (aStack[0].column->precision ==
                          aStack[1].column->precision) &&
                         (aStack[0].column->precision > 0) );

            sStack1 = (mtcStack*)aStack[0].value;
            sStack2 = (mtcStack*)aStack[1].value;

            for( sCount = 0, sFence = aStack[0].column->precision, sCompare = 0;
                 (sCount < sFence) && (sCompare == 0);
                 sCount++ )
            {
                IDE_DASSERT( sStack1[sCount].column->module == sStack2[sCount].column->module );

                sModule = sStack1[sCount].column->module;

                if ( ( sModule->isNull( sStack1[sCount].column,
                                        sStack1[sCount].value ) == ID_TRUE ) ||
                     ( sModule->isNull( sStack2[sCount].column,
                                        sStack2[sCount].value ) == ID_TRUE ) )
                {
                    sCompare = -1;
                }
                else
                {
                    sValueInfo1.column = sStack1[sCount].column;
                    sValueInfo1.value  = sStack1[sCount].value;
                    sValueInfo1.flag   = MTD_OFFSET_USELESS;

                    sValueInfo2.column = sStack2[sCount].column;
                    sValueInfo2.value  = sStack2[sCount].value;
                    sValueInfo2.flag   = MTD_OFFSET_USELESS;

                    sCompare = sModule->logicalCompare[MTD_COMPARE_ASCENDING]( &sValueInfo1,
                                                                               &sValueInfo2 );
                }
            }
        }
    }
    else
    {
        IDE_TEST_RAISE( aRemain < 1, ERR_STACK_OVERFLOW );
        sCompare = 0;
    }

    // 첫번째 인자
    IDE_TEST( sInfo->sAvgColumnExecute->calculate( sInfo->sAvgColumnNode,
                                                   aStack,
                                                   aRemain,
                                                   sInfo->sAvgColumnExecute->calculateInfo,
                                                   aTemplate )
              != IDE_SUCCESS );
    if( sInfo->sAvgColumnNode->conversion != NULL )
    {
        IDE_TEST( mtf::convertCalculate( sInfo->sAvgColumnNode,
                                         aStack,
                                         aRemain,
                                         NULL,
                                         aTemplate )
                  != IDE_SUCCESS );
    }

    IDE_DASSERT( aStack[0].column->module == & mtdDouble );

    sModule = aStack[0].column->module;

    if ( sModule->isNull( aStack[0].column,
                          aStack[0].value ) != ID_TRUE )
    {
        if ( sCompare == 0 )
        {
            *(mtdDoubleType*)(sInfo->sReturnValue) += *(mtdDoubleType*) aStack[0].value;
            sInfo->sSumCount++;
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC mtfDecodeAvgFinalizeDouble( mtcNode*     aNode,
                                   mtcStack*,
                                   SInt,
                                   void*,
                                   mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeAvgFinalizeDouble"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtcColumn   * sColumn;
    mtdBinaryType     * sValue;
    mtfDecodeAvgInfo  * sInfo;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sValue = (mtdBinaryType*)((UChar*)aTemplate->rows[aNode->table].row
                              + sColumn[1].column.offset);
    sInfo = (mtfDecodeAvgInfo*)(sValue->mValue);

    if( sInfo->sSumCount == 0 )
    {
        mtdDouble.null( sInfo->sReturnColumn,
                        sInfo->sReturnValue );
    }
    else
    {
        *(mtdDoubleType*)(sInfo->sReturnValue) /= sInfo->sSumCount;
    }

    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC mtfDecodeAvgCalculateDouble( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt,
                                    void*,
                                    mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeAvgCalculateDouble"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    aStack->column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack->value  = (void*)( (UChar*) aTemplate->rows[aNode->table].row
                              + aStack->column->column.offset );

    return IDE_SUCCESS;
#undef IDE_FN
}

/* ZONE: BIGINT */

IDE_RC mtfDecodeAvgEstimateBigint( mtcNode*     aNode,
                                   mtcTemplate* aTemplate,
                                   mtcStack*    aStack,
                                   SInt,
                                   mtcCallBack* aCallBack )
{
#define IDE_FN "IDE_RC mtfDecodeAvgEstimateBigint"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtdModule * sTarget;
    const mtdModule * sModules[MTC_NODE_ARGUMENT_COUNT_MAXIMUM];
    mtcStack        * sStack1;
    mtcStack        * sStack2;
    UInt              sBinaryPrecision;
    UInt              sCount;
    UInt              sFence;

    sFence = aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK;

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    if ( sFence == 3 )
    {
        if ( aStack[2].column->module != &mtdList )
        {
            IDE_TEST( mtf::getComparisonModule(
                          &sTarget,
                          aStack[2].column->module->no,
                          aStack[3].column->module->no )
                      != IDE_SUCCESS );

            IDE_TEST_RAISE( sTarget == NULL,
                            ERR_CONVERSION_NOT_APPLICABLE );

            // To Fix PR-15208
            IDE_TEST_RAISE( mtf::isEquiValidType( sTarget ) != ID_TRUE,
                            ERR_CONVERSION_NOT_APPLICABLE );

            sModules[0] = & mtdBigint;
            sModules[1] = sTarget;
            sModules[2] = sTarget;

            IDE_TEST( mtf::makeConversionNodes( aNode,
                                                aNode->arguments,
                                                aTemplate,
                                                aStack + 1,
                                                aCallBack,
                                                sModules )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST_RAISE( aStack[3].column->module != &mtdList,
                            ERR_CONVERSION_NOT_APPLICABLE );
            IDE_TEST_RAISE( (aStack[2].column->precision !=
                             aStack[3].column->precision) ||
                            (aStack[2].column->precision <= 0),
                            ERR_INVALID_FUNCTION_ARGUMENT );

            sStack1 = (mtcStack*)aStack[2].value;
            sStack2 = (mtcStack*)aStack[3].value;

            for( sCount = 0, sFence = aStack[2].column->precision;
                 sCount < sFence;
                 sCount++ )
            {
                IDE_TEST( mtf::getComparisonModule(
                              &sTarget,
                              sStack1[sCount].column->module->no,
                              sStack2[sCount].column->module->no )
                          != IDE_SUCCESS );
                IDE_TEST_RAISE( (sTarget == NULL) || (sTarget == &mtdList),
                                ERR_CONVERSION_NOT_APPLICABLE );
                // To Fix PR-15208
                IDE_TEST_RAISE( mtf::isEquiValidType( sTarget ) != ID_TRUE,
                                ERR_CONVERSION_NOT_APPLICABLE );
                sModules[sCount] = sTarget;
            }

            IDE_TEST( mtf::makeConversionNodes( aNode,
                                                aNode->arguments->next->arguments,
                                                aTemplate,
                                                sStack1,
                                                aCallBack,
                                                sModules )
                      != IDE_SUCCESS );

            IDE_TEST( mtf::makeConversionNodes( aNode,
                                                aNode->arguments->next->next->arguments,
                                                aTemplate,
                                                sStack2,
                                                aCallBack,
                                                sModules )
                      != IDE_SUCCESS );

            sModules[0] = & mtdBigint;
            sModules[1] = & mtdList;
            sModules[2] = & mtdList;

            IDE_TEST( mtf::makeConversionNodes( aNode,
                                                aNode->arguments,
                                                aTemplate,
                                                aStack + 1,
                                                aCallBack,
                                                sModules )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        sModules[0] = & mtdBigint;
        IDE_TEST( mtf::makeConversionNodes( aNode,
                                            aNode->arguments,
                                            aTemplate,
                                            aStack + 1,
                                            aCallBack,
                                            sModules )
                  != IDE_SUCCESS );
    }

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfDecodeAvgExecuteBigint;

    // avg 결과를 저장함
    // bigint avg의 결과는 double이 아니라 float type임
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdFloat,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // avg info 정보를 mtdBinary에 저장
    sBinaryPrecision = ID_SIZEOF(mtfDecodeAvgInfo);
    IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                     & mtdBinary,
                                     1,
                                     sBinaryPrecision,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC mtfDecodeAvgInitializeBigint( mtcNode*     aNode,
                                     mtcStack*,
                                     SInt,
                                     void*,
                                     mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeAvgInitializeBigint"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtcColumn   * sColumn;
    mtcNode           * sArgNode[3];
    mtdNumericType    * sFloat;
    mtdBinaryType     * sValue;
    mtfDecodeAvgInfo  * sInfo;
    UInt                sFence;

    sFence = aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sValue = (mtdBinaryType*)
        ((UChar*)aTemplate->rows[aNode->table].row + sColumn[1].column.offset);

    // BUG-43709
    sValue->mLength = sColumn[1].precision;
    sInfo = (mtfDecodeAvgInfo*)(sValue->mValue);

    //-----------------------------
    // avg info 초기화
    //-----------------------------
    sArgNode[0] = aNode->arguments;

    // avg column 설정
    sInfo->sAvgColumnExecute = aTemplate->rows[sArgNode[0]->table].execute + sArgNode[0]->column;
    sInfo->sAvgColumnNode    = sArgNode[0];

    if ( sFence == 3 )
    {
        sArgNode[1] = sArgNode[0]->next;
        sArgNode[2] = sArgNode[1]->next;

        // expression column 설정
        sInfo->sExprExecute = aTemplate->rows[sArgNode[1]->table].execute + sArgNode[1]->column;
        sInfo->sExprNode    = sArgNode[1];

        // search value 설정
        sInfo->sSearchExecute = aTemplate->rows[sArgNode[2]->table].execute + sArgNode[2]->column;
        sInfo->sSearchNode    = sArgNode[2];
    }
    else
    {
        sInfo->sExprExecute = NULL;
        sInfo->sExprNode    = NULL;

        sInfo->sSearchExecute = NULL;
        sInfo->sSearchNode    = NULL;
    }
    // return column 설정
    sInfo->sReturnColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sInfo->sReturnValue  = (void *)
        ((UChar*) aTemplate->rows[aNode->table].row + sInfo->sReturnColumn->column.offset);

    // 임시변수 초기화
    sInfo->sSumCount = 0;
    sInfo->sBigintSum = 0;
    //-----------------------------
    // avg 결과를 초기화
    //-----------------------------

    sFloat = (mtdNumericType*) sInfo->sReturnValue;
    sFloat->length       = 1;
    sFloat->signExponent = 0x80;

    return IDE_SUCCESS;
#undef IDE_FN
}

IDE_RC mtfDecodeAvgAggregateBigint( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*,
                                    mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeAvgAggregateBigint"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtdModule   * sModule;
    const mtcColumn   * sColumn;
    mtdBinaryType     * sValue;
    mtfDecodeAvgInfo  * sInfo;
    mtdValueInfo        sValueInfo1;
    mtdValueInfo        sValueInfo2;
    mtcStack          * sStack1;
    mtcStack          * sStack2;
    SInt                sCompare;
    UInt                sCount;
    UInt                sFence;

    sFence = aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sValue = (mtdBinaryType*)((UChar*)aTemplate->rows[aNode->table].row
                              + sColumn[1].column.offset);
    sInfo = (mtfDecodeAvgInfo*)(sValue->mValue);

    if ( sFence == 3 )
    {
        IDE_TEST_RAISE( aRemain < 2, ERR_STACK_OVERFLOW );

        // 두번째 인자
        IDE_TEST( sInfo->sExprExecute->calculate( sInfo->sExprNode,
                                                  aStack,
                                                  aRemain,
                                                  sInfo->sExprExecute->calculateInfo,
                                                  aTemplate )
                  != IDE_SUCCESS );

        if( sInfo->sExprNode->conversion != NULL )
        {
            IDE_TEST( mtf::convertCalculate( sInfo->sExprNode,
                                             aStack,
                                             aRemain,
                                             NULL,
                                             aTemplate )
                      != IDE_SUCCESS );
        }

        // 세번째 인자
        IDE_TEST( sInfo->sSearchExecute->calculate( sInfo->sSearchNode,
                                                    aStack + 1,
                                                    aRemain - 1,
                                                    sInfo->sSearchExecute->calculateInfo,
                                                    aTemplate )
                  != IDE_SUCCESS );

        if( sInfo->sSearchNode->conversion != NULL )
        {
            IDE_TEST( mtf::convertCalculate( sInfo->sSearchNode,
                                             aStack + 1,
                                             aRemain - 1,
                                             NULL,
                                             aTemplate )
                      != IDE_SUCCESS );
        }

        // decode 연산수행
        if ( aStack[0].column->module != &mtdList )
        {
            IDE_DASSERT( aStack[0].column->module == aStack[1].column->module );

            sModule = aStack[0].column->module;
            if ( ( sModule->isNull( aStack[0].column,
                                    aStack[0].value ) == ID_TRUE ) ||
                 ( sModule->isNull( aStack[1].column,
                                    aStack[1].value ) == ID_TRUE ) )
            {
                sCompare = -1;
            }
            else
            {
                sValueInfo1.column = aStack[0].column;
                sValueInfo1.value  = aStack[0].value;
                sValueInfo1.flag   = MTD_OFFSET_USELESS;

                sValueInfo2.column = aStack[1].column;
                sValueInfo2.value  = aStack[1].value;
                sValueInfo2.flag   = MTD_OFFSET_USELESS;
                // 두번째 인자와 세번째 인자의 비교
                sCompare = sModule->logicalCompare[MTD_COMPARE_ASCENDING]( &sValueInfo1,
                                                                           &sValueInfo2 );
            }
        }
        else
        {
            IDE_DASSERT( aStack[0].column->module == &mtdList );
            IDE_DASSERT( (aStack[0].column->precision ==
                          aStack[1].column->precision) &&
                         (aStack[0].column->precision > 0) );

            sStack1 = (mtcStack*)aStack[0].value;
            sStack2 = (mtcStack*)aStack[1].value;

            for( sCount = 0, sFence = aStack[0].column->precision, sCompare = 0;
                 (sCount < sFence) && (sCompare == 0);
                 sCount++ )
            {
                IDE_DASSERT( sStack1[sCount].column->module == sStack2[sCount].column->module );
                sModule = sStack1[sCount].column->module;

                if ( ( sModule->isNull( sStack1[sCount].column,
                                        sStack1[sCount].value ) == ID_TRUE ) ||
                     ( sModule->isNull( sStack2[sCount].column,
                                        sStack2[sCount].value ) == ID_TRUE ) )
                {
                    sCompare = -1;
                }
                else
                {
                    sValueInfo1.column = sStack1[sCount].column;
                    sValueInfo1.value  = sStack1[sCount].value;
                    sValueInfo1.flag   = MTD_OFFSET_USELESS;

                    sValueInfo2.column = sStack2[sCount].column;
                    sValueInfo2.value  = sStack2[sCount].value;
                    sValueInfo2.flag   = MTD_OFFSET_USELESS;

                    sCompare = sModule->logicalCompare[MTD_COMPARE_ASCENDING]( &sValueInfo1,
                                                                               &sValueInfo2 );
                }
            }
        }
    }
    else
    {
        IDE_TEST_RAISE( aRemain < 1, ERR_STACK_OVERFLOW );
        sCompare = 0;
    }

    // 첫번째 인자
    IDE_TEST( sInfo->sAvgColumnExecute->calculate( sInfo->sAvgColumnNode,
                                                   aStack,
                                                   aRemain,
                                                   sInfo->sAvgColumnExecute->calculateInfo,
                                                   aTemplate )
              != IDE_SUCCESS );

    if( sInfo->sAvgColumnNode->conversion != NULL )
    {
        IDE_TEST( mtf::convertCalculate( sInfo->sAvgColumnNode,
                                         aStack,
                                         aRemain,
                                         NULL,
                                         aTemplate )
                  != IDE_SUCCESS );
    }

    IDE_DASSERT( aStack[0].column->module == & mtdBigint );

    sModule = aStack[0].column->module;

    if ( sModule->isNull( aStack[0].column,
                          aStack[0].value ) != ID_TRUE )
    {
        if ( sCompare == 0 )
        {
            sInfo->sBigintSum += *(mtdBigintType*) aStack[0].value;
            sInfo->sSumCount++;
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
#undef IDE_FN
}

IDE_RC mtfDecodeAvgFinalizeBigint( mtcNode*     aNode,
                                   mtcStack*,
                                   SInt,
                                   void*,
                                   mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeAvgFinalizeBigint"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtcColumn   * sColumn;
    mtdBinaryType     * sValue;
    mtfDecodeAvgInfo  * sInfo;

    mtdNumericType    * sFloatSum;
    mtdNumericType    * sFloatCount;
    UChar               sFloatSumBuff[MTD_FLOAT_SIZE_MAXIMUM];
    UChar               sFloatCountBuff[MTD_FLOAT_SIZE_MAXIMUM];

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sValue = (mtdBinaryType*)((UChar*)aTemplate->rows[aNode->table].row
                              + sColumn[1].column.offset);
    sInfo = (mtfDecodeAvgInfo*)(sValue->mValue);

    if( sInfo->sSumCount == 0 )
    {
        mtdFloat.null( sInfo->sReturnColumn,
                       sInfo->sReturnValue );
    }
    else
    {
        sFloatSum = (mtdNumericType*)sFloatSumBuff;
        mtc::makeNumeric( sFloatSum, (SLong)(sInfo->sBigintSum) );
        sFloatCount = (mtdNumericType*)sFloatCountBuff;
        mtc::makeNumeric( sFloatCount, (SLong)sInfo->sSumCount );

        IDE_TEST( mtc::divideFloat( (mtdNumericType*)(sInfo->sReturnValue),
                                    MTD_FLOAT_PRECISION_MAXIMUM,
                                    sFloatSum,
                                    sFloatCount )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC mtfDecodeAvgCalculateBigint( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt,
                                    void*,
                                    mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeAvgCalculateBigint"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    aStack->column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack->value  = (void*)( (UChar*) aTemplate->rows[aNode->table].row
                              + aStack->column->column.offset );

    return IDE_SUCCESS;
#undef IDE_FN
}
