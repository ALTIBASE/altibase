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
 * $Id: mtfDecodeSum.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>

extern mtfModule mtfDecodeSum;

extern mtdModule mtdFloat;
extern mtdModule mtdDouble;
extern mtdModule mtdBigint;
extern mtdModule mtdList;
extern mtdModule mtdBoolean;
extern mtdModule mtdBinary;

static mtcName mtfDecodeSumFunctionName[1] = {
    { NULL, 10, (void*)"DECODE_SUM" }
};

static IDE_RC mtfDecodeSumInitialize( void );

static IDE_RC mtfDecodeSumFinalize( void );

static IDE_RC mtfDecodeSumEstimate( mtcNode*     aNode,
                                    mtcTemplate* aTemplate,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    mtcCallBack* aCallBack );

mtfModule mtfDecodeSum = {
    2|MTC_NODE_OPERATOR_AGGREGATION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfDecodeSumFunctionName,
    NULL,
    mtfDecodeSumInitialize,
    mtfDecodeSumFinalize,
    mtfDecodeSumEstimate
};

static IDE_RC mtfDecodeSumEstimateFloat( mtcNode*     aNode,
                                         mtcTemplate* aTemplate,
                                         mtcStack*    aStack,
                                         SInt         aRemain,
                                         mtcCallBack* aCallBack );

IDE_RC mtfDecodeSumInitializeFloat( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate );

IDE_RC mtfDecodeSumAggregateFloat( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate );

IDE_RC mtfDecodeSumFinalizeFloat( mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*        aInfo,
                                  mtcTemplate* aTemplate );

IDE_RC mtfDecodeSumCalculateFloat( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate );

static const mtcExecute mtfDecodeSumExecuteFloat = {
    mtfDecodeSumInitializeFloat,
    mtfDecodeSumAggregateFloat,
    mtf::calculateNA,
    mtfDecodeSumFinalizeFloat,
    mtfDecodeSumCalculateFloat,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

static IDE_RC mtfDecodeSumEstimateDouble( mtcNode*     aNode,
                                          mtcTemplate* aTemplate,
                                          mtcStack*    aStack,
                                          SInt         aRemain,
                                          mtcCallBack* aCallBack );

IDE_RC mtfDecodeSumInitializeDouble( mtcNode*     aNode,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     void*        aInfo,
                                     mtcTemplate* aTemplate );

IDE_RC mtfDecodeSumAggregateDouble( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate );

IDE_RC mtfDecodeSumFinalizeDouble( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate );

IDE_RC mtfDecodeSumCalculateDouble( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate );

static const mtcExecute mtfDecodeSumExecuteDouble = {
    mtfDecodeSumInitializeDouble,
    mtfDecodeSumAggregateDouble,
    mtf::calculateNA,
    mtfDecodeSumFinalizeDouble,
    mtfDecodeSumCalculateDouble,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

static IDE_RC mtfDecodeSumEstimateBigint( mtcNode*     aNode,
                                          mtcTemplate* aTemplate,
                                          mtcStack*    aStack,
                                          SInt         aRemain,
                                          mtcCallBack* aCallBack );

IDE_RC mtfDecodeSumInitializeBigint( mtcNode*     aNode,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     void*        aInfo,
                                     mtcTemplate* aTemplate );

IDE_RC mtfDecodeSumAggregateBigint( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate );

IDE_RC mtfDecodeSumFinalizeBigint( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate );

IDE_RC mtfDecodeSumCalculateBigint( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate );

static const mtcExecute mtfDecodeSumExecuteBigint = {
    mtfDecodeSumInitializeBigint,
    mtfDecodeSumAggregateBigint,
    mtf::calculateNA,
    mtfDecodeSumFinalizeBigint,
    mtfDecodeSumCalculateBigint,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

typedef struct mtfDecodeSumInfo
{
    // 첫번째 인자
    mtcExecute   * sSumColumnExecute;
    mtcNode      * sSumColumnNode;

    // 두번째 인자
    mtcExecute   * sExprExecute;
    mtcNode      * sExprNode;

    // 세번째 인자
    mtcExecute   * sSearchExecute;
    mtcNode      * sSearchNode;

    // return 인자
    mtcColumn    * sReturnColumn;
    void         * sReturnValue;
    idBool         sIsNull;

} mtfDecodeSumInfo;

static mtfSubModule mtfXX[1] = {
    { NULL, mtf::estimateNA }
};

static mtfSubModule mtfDecodeSumEstimates[3] = {
    { mtfDecodeSumEstimates+1, mtfDecodeSumEstimateFloat  },
    { mtfDecodeSumEstimates+2, mtfDecodeSumEstimateDouble },
    { NULL,                    mtfDecodeSumEstimateBigint }
};

static mtfSubModule** mtfTable = NULL;

IDE_RC mtfDecodeSumInitialize( void )
{
    return mtf::initializeTemplate( &mtfTable, mtfDecodeSumEstimates, mtfXX );
}

IDE_RC mtfDecodeSumFinalize( void )
{
    return mtf::finalizeTemplate( &mtfTable );
}

IDE_RC mtfDecodeSumEstimate( mtcNode*     aNode,
                             mtcTemplate* aTemplate,
                             mtcStack*    aStack,
                             SInt         aRemain,
                             mtcCallBack* aCallBack )
{
#define IDE_FN "IDE_RC mtfDecodeSumEstimate"
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

IDE_RC mtfDecodeSumEstimateFloat( mtcNode*     aNode,
                                  mtcTemplate* aTemplate,
                                  mtcStack*    aStack,
                                  SInt,
                                  mtcCallBack* aCallBack )
{
#define IDE_FN "IDE_RC mtfDecodeSumEstimateFloat"
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

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfDecodeSumExecuteFloat;

    // sum 결과를 저장함
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdFloat,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // sum info 정보를 mtdBinary에 저장
    sBinaryPrecision = ID_SIZEOF(mtfDecodeSumInfo);

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

IDE_RC mtfDecodeSumInitializeFloat( mtcNode*     aNode,
                                    mtcStack*,
                                    SInt,
                                    void*,
                                    mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeSumInitializeFloat"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtcColumn   * sColumn;
    mtcNode           * sArgNode[3];
    mtdNumericType    * sFloat;
    mtdBinaryType     * sValue;
    mtfDecodeSumInfo  * sInfo;
    UInt                sFence;

    sFence = aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sValue = (mtdBinaryType*)
        ((UChar*)aTemplate->rows[aNode->table].row + sColumn[1].column.offset);

    // BUG-43709
    sValue->mLength = sColumn[1].precision;
    sInfo = (mtfDecodeSumInfo*)(sValue->mValue);

    //-----------------------------
    // sum info 초기화
    //-----------------------------

    sArgNode[0] = aNode->arguments;
    // sum column 설정
    sInfo->sSumColumnExecute = aTemplate->rows[sArgNode[0]->table].execute + sArgNode[0]->column;
    sInfo->sSumColumnNode    = sArgNode[0];

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
    sInfo->sIsNull = ID_TRUE;
    //-----------------------------
    // sum 결과를 초기화
    //-----------------------------
    sFloat = (mtdNumericType*) sInfo->sReturnValue;
    sFloat->length       = 1;
    sFloat->signExponent = 0x80;

    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC mtfDecodeSumAggregateFloat( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*,
                                   mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeSumAggregateFloat"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtdModule   * sModule;
    const mtcColumn   * sColumn;
    mtdNumericType    * sFloatSum;
    mtdNumericType    * sFloatArgument;
    UChar               sFloatSumBuff[MTD_FLOAT_SIZE_MAXIMUM];
    mtdNumericType    * sFloatSumClone;
    mtdBinaryType     * sValue;
    mtfDecodeSumInfo  * sInfo;
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
    sInfo = (mtfDecodeSumInfo*)(sValue->mValue);

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

    if ( sCompare == 0 )
    {
        // 첫번째 인자
        IDE_TEST( sInfo->sSumColumnExecute->calculate( sInfo->sSumColumnNode,
                                                       aStack,
                                                       aRemain,
                                                       sInfo->sSumColumnExecute->calculateInfo,
                                                       aTemplate )
                  != IDE_SUCCESS );
        if( sInfo->sSumColumnNode->conversion != NULL )
        {
            IDE_TEST( mtf::convertCalculate( sInfo->sSumColumnNode,
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
            sFloatSum      = (mtdNumericType*)sInfo->sReturnValue;
            sFloatArgument = (mtdNumericType*)aStack[0].value;
            sFloatSumClone = (mtdNumericType*)sFloatSumBuff;
            idlOS::memcpy( sFloatSumClone, sFloatSum, sFloatSum->length + 1 );

            IDE_TEST( mtc::addFloat( sFloatSum,
                                     MTD_FLOAT_PRECISION_MAXIMUM,
                                     sFloatSumClone,
                                     sFloatArgument )
                      != IDE_SUCCESS );

            if ( sInfo->sIsNull == ID_TRUE )
            {
                sInfo->sIsNull = ID_FALSE;
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

IDE_RC mtfDecodeSumFinalizeFloat( mtcNode*     aNode,
                                  mtcStack*,
                                  SInt,
                                  void*,
                                  mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeSumFinalizeFloat"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtcColumn   * sColumn;
    mtdBinaryType     * sValue;
    mtfDecodeSumInfo  * sInfo;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sValue = (mtdBinaryType*)((UChar*)aTemplate->rows[aNode->table].row
                              + sColumn[1].column.offset);
    sInfo = (mtfDecodeSumInfo*)(sValue->mValue);

    if( sInfo->sIsNull == ID_TRUE )
    {
        mtdFloat.null( sInfo->sReturnColumn,
                       sInfo->sReturnValue );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC mtfDecodeSumCalculateFloat( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt,
                                   void*,
                                   mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeSumCalculateFloat"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    aStack->column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack->value  = (void*)( (UChar*) aTemplate->rows[aNode->table].row
                              + aStack->column->column.offset );

    return IDE_SUCCESS;

#undef IDE_FN
}

/* ZONE: DOUBLE */

IDE_RC mtfDecodeSumEstimateDouble( mtcNode*     aNode,
                                   mtcTemplate* aTemplate,
                                   mtcStack*    aStack,
                                   SInt,
                                   mtcCallBack* aCallBack )
{
#define IDE_FN "IDE_RC mtfDecodeSumEstimateDouble"
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

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfDecodeSumExecuteDouble;

    // sum 결과를 저장함
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdDouble,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // sum info 정보를 mtdBinary에 저장
    sBinaryPrecision = ID_SIZEOF(mtfDecodeSumInfo);

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

IDE_RC mtfDecodeSumInitializeDouble( mtcNode*     aNode,
                                     mtcStack*,
                                     SInt,
                                     void*,
                                     mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeSumInitializeDouble"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtcColumn   * sColumn;
    mtcNode           * sArgNode[3];
    mtdBinaryType     * sValue;
    mtfDecodeSumInfo  * sInfo;
    UInt                sFence;

    sFence = aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sValue = (mtdBinaryType*)
        ((UChar*)aTemplate->rows[aNode->table].row + sColumn[1].column.offset);

    // BUG-43709
    sValue->mLength = sColumn[1].precision;
    sInfo = (mtfDecodeSumInfo*)(sValue->mValue);

    //-----------------------------
    // sum info 초기화
    //-----------------------------
    sArgNode[0] = aNode->arguments;

    // sum column 설정
    sInfo->sSumColumnExecute = aTemplate->rows[sArgNode[0]->table].execute + sArgNode[0]->column;
    sInfo->sSumColumnNode    = sArgNode[0];

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
    sInfo->sIsNull = ID_TRUE;

    //-----------------------------
    // sum 결과를 초기화
    //-----------------------------

    *(mtdDoubleType*)(sInfo->sReturnValue) = 0;

    return IDE_SUCCESS;
#undef IDE_FN
}

IDE_RC mtfDecodeSumAggregateDouble( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*,
                                    mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeSumAggregateDouble"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtdModule   * sModule;
    const mtcColumn   * sColumn;
    mtdBinaryType     * sValue;
    mtfDecodeSumInfo  * sInfo;
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
    sInfo = (mtfDecodeSumInfo*)(sValue->mValue);

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

    if ( sCompare == 0 )
    {
        // 첫번째 인자
        IDE_TEST( sInfo->sSumColumnExecute->calculate( sInfo->sSumColumnNode,
                                                       aStack,
                                                       aRemain,
                                                       sInfo->sSumColumnExecute->calculateInfo,
                                                       aTemplate )
                  != IDE_SUCCESS );
        if( sInfo->sSumColumnNode->conversion != NULL )
        {
            IDE_TEST( mtf::convertCalculate( sInfo->sSumColumnNode,
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
            *(mtdDoubleType*)(sInfo->sReturnValue) += *(mtdDoubleType*) aStack[0].value;

            if ( sInfo->sIsNull == ID_TRUE )
            {
                sInfo->sIsNull = ID_FALSE;
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

IDE_RC mtfDecodeSumFinalizeDouble( mtcNode*     aNode,
                                   mtcStack*,
                                   SInt,
                                   void*,
                                   mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeSumFinalizeDouble"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtcColumn   * sColumn;
    mtdBinaryType     * sValue;
    mtfDecodeSumInfo  * sInfo;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sValue = (mtdBinaryType*)((UChar*)aTemplate->rows[aNode->table].row
                              + sColumn[1].column.offset);
    sInfo = (mtfDecodeSumInfo*)(sValue->mValue);

    if( sInfo->sIsNull == ID_TRUE )
    {
        mtdDouble.null( sInfo->sReturnColumn,
                        sInfo->sReturnValue );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC mtfDecodeSumCalculateDouble( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt,
                                    void*,
                                    mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeSumCalculateDouble"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));
    aStack->column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack->value  = (void*)( (UChar*) aTemplate->rows[aNode->table].row
                              + aStack->column->column.offset );
    return IDE_SUCCESS;
#undef IDE_FN
}

/* ZONE: BIGINT */

IDE_RC mtfDecodeSumEstimateBigint( mtcNode*     aNode,
                                   mtcTemplate* aTemplate,
                                   mtcStack*    aStack,
                                   SInt,
                                   mtcCallBack* aCallBack )
{
#define IDE_FN "IDE_RC mtfDecodeSumEstimateBigint"
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

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfDecodeSumExecuteBigint;

    // sum 결과를 저장함
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdBigint,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    // sum info 정보를 mtdBinary에 저장
    sBinaryPrecision = ID_SIZEOF(mtfDecodeSumInfo);
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

IDE_RC mtfDecodeSumInitializeBigint( mtcNode*     aNode,
                                     mtcStack*,
                                     SInt,
                                     void*,
                                     mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeSumInitializeBigint"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtcColumn   * sColumn;
    mtcNode           * sArgNode[3];
    mtdBinaryType     * sValue;
    mtfDecodeSumInfo  * sInfo;
    UInt                sFence;

    sFence = aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sValue = (mtdBinaryType*)
        ((UChar*)aTemplate->rows[aNode->table].row + sColumn[1].column.offset);

    // BUG-43709
    sValue->mLength = sColumn[1].precision;
    sInfo = (mtfDecodeSumInfo*)(sValue->mValue);

    //-----------------------------
    // sum info 초기화
    //-----------------------------

    sArgNode[0] = aNode->arguments;

    // sum column 설정
    sInfo->sSumColumnExecute = aTemplate->rows[sArgNode[0]->table].execute + sArgNode[0]->column;
    sInfo->sSumColumnNode    = sArgNode[0];

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
    sInfo->sIsNull = ID_TRUE;

    //-----------------------------
    // sum 결과를 초기화
    //-----------------------------

    *(mtdBigintType*)(sInfo->sReturnValue) = 0;

    return IDE_SUCCESS;
#undef IDE_FN
}

IDE_RC mtfDecodeSumAggregateBigint( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*,
                                    mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeSumAggregateBigint"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtdModule   * sModule;
    const mtcColumn   * sColumn;
    mtdBinaryType     * sValue;
    mtfDecodeSumInfo  * sInfo;
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
    sInfo = (mtfDecodeSumInfo*)(sValue->mValue);

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

    if ( sCompare == 0 )
    {
        // 첫번째 인자
        IDE_TEST( sInfo->sSumColumnExecute->calculate( sInfo->sSumColumnNode,
                                                       aStack,
                                                       aRemain,
                                                       sInfo->sSumColumnExecute->calculateInfo,
                                                       aTemplate )
                  != IDE_SUCCESS );

        if( sInfo->sSumColumnNode->conversion != NULL )
        {
            IDE_TEST( mtf::convertCalculate( sInfo->sSumColumnNode,
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
            *(mtdBigintType*)(sInfo->sReturnValue) += *(mtdBigintType*) aStack[0].value;

            if ( sInfo->sIsNull == ID_TRUE )
            {
                sInfo->sIsNull = ID_FALSE;
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

IDE_RC mtfDecodeSumFinalizeBigint( mtcNode*     aNode,
                                   mtcStack*,
                                   SInt,
                                   void*,
                                   mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeSumFinalizeBigint"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtcColumn   * sColumn;
    mtdBinaryType     * sValue;
    mtfDecodeSumInfo  * sInfo;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sValue = (mtdBinaryType*)((UChar*)aTemplate->rows[aNode->table].row
                              + sColumn[1].column.offset);
    sInfo = (mtfDecodeSumInfo*)(sValue->mValue);

    if( sInfo->sIsNull == ID_TRUE )
    {
        mtdBigint.null( sInfo->sReturnColumn,
                        sInfo->sReturnValue );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC mtfDecodeSumCalculateBigint( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt,
                                    void*,
                                    mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeSumCalculateBigint"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));
    aStack->column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack->value  = (void*)( (UChar*) aTemplate->rows[aNode->table].row
                              + aStack->column->column.offset );
    return IDE_SUCCESS;
#undef IDE_FN
}
