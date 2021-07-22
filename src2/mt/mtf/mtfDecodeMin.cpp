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
 * $Id: mtfDecodeMin.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>

extern mtfModule mtfDecodeMin;

extern mtdModule mtdFloat;
extern mtdModule mtdDouble;
extern mtdModule mtdBigint;
extern mtdModule mtdList;
extern mtdModule mtdBoolean;
extern mtdModule mtdBinary;

static mtcName mtfDecodeMinFunctionName[1] = {
    { NULL, 10, (void*)"DECODE_MIN" }
};

static IDE_RC mtfDecodeMinEstimate( mtcNode*     aNode,
                                    mtcTemplate* aTemplate,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    mtcCallBack* aCallBack );

mtfModule mtfDecodeMin = {
    2|MTC_NODE_OPERATOR_AGGREGATION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfDecodeMinFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfDecodeMinEstimate
};

IDE_RC mtfDecodeMinInitialize( mtcNode*     aNode,
                               mtcStack*    aStack,
                               SInt         aRemain,
                               void*        aInfo,
                               mtcTemplate* aTemplate );

IDE_RC mtfDecodeMinAggregate( mtcNode*     aNode,
                              mtcStack*    aStack,
                              SInt         aRemain,
                              void*        aInfo,
                              mtcTemplate* aTemplate );

IDE_RC mtfDecodeMinFinalize( mtcNode*     aNode,
                             mtcStack*    aStack,
                             SInt         aRemain,
                             void*        aInfo,
                             mtcTemplate* aTemplate );

IDE_RC mtfDecodeMinCalculate( mtcNode*     aNode,
                              mtcStack*    aStack,
                              SInt         aRemain,
                              void*        aInfo,
                              mtcTemplate* aTemplate );

static const mtcExecute mtfDecodeMinExecute = {
    mtfDecodeMinInitialize,
    mtfDecodeMinAggregate,
    mtf::calculateNA,
    mtfDecodeMinFinalize,
    mtfDecodeMinCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

typedef struct mtfDecodeMinInfo
{
    // 첫번째 인자
    mtcExecute   * sMinColumnExecute;
    mtcNode      * sMinColumnNode;

    // 두번째 인자
    mtcExecute   * sExprExecute;
    mtcNode      * sExprNode;

    // 세번째 인자
    mtcExecute   * sSearchExecute;
    mtcNode      * sSearchNode;

    // return 인자
    mtcColumn    * sReturnColumn;
    void         * sReturnValue;

} mtfDecodeMinInfo;

IDE_RC mtfDecodeMinEstimate( mtcNode*     aNode,
                             mtcTemplate* aTemplate,
                             mtcStack*    aStack,
                             SInt      /* aRemain */,
                             mtcCallBack* aCallBack )
{
#define IDE_FN "IDE_RC mtfDecodeMinEstimate"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtdModule * sTarget;
    const mtdModule * sModules[MTC_NODE_ARGUMENT_COUNT_MAXIMUM];
    mtcStack        * sStack1;
    mtcStack        * sStack2;
    UInt              sBinaryPrecision;
    UInt              sCount;
    UInt              sFence;

    sFence = aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK;

    // 1 혹은 3개의 인자
    IDE_TEST_RAISE( (sFence != 1) && (sFence != 3),
                    ERR_INVALID_FUNCTION_ARGUMENT );

    // PROJ-2002 Column Security
    // min함수는 비교만을 수행하므로 min함수 자체는 복호화가
    // 필요하지 않다. 그러나 min함수가 복호화한 값을 리턴하기
    // 위해서는 마지막 min값에 대해 복호화를 수행할 수 도 있지만
    // 이 경우 암호 타입의 임시 변수를 저장할 공간이 필요하고
    // 또 min이 중첩되는 경우도 있으므로 min함수에 보안 타입이
    // 오는 경우 보안 타입으로 리턴한다. 단, 복호화를 위해
    // 인자의 source를 min함수의 source로 설정한다.
    //
    // ex) select _decrypt(min(i1)) from t1;
    //     select _decrypt(max(min(i2))) from t1 group by i1;
    aNode->baseTable = aNode->arguments->baseTable;
    aNode->baseColumn = aNode->arguments->baseColumn;

    IDE_TEST_RAISE( (aStack[1].column->module == &mtdList) ||
                    (aStack[1].column->module == &mtdBoolean),
                    ERR_CONVERSION_NOT_APPLICABLE );

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

            sModules[0] = aStack[1].column->module;
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
        }
    }
    else
    {
        // Nothing to do.
    }

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfDecodeMinExecute;

    // min 결과를 저장함
    // BUG-23102
    // mtcColumn으로 초기화한다.
    mtc::initializeColumn( aStack[0].column, aStack[1].column );
    // min info 정보를 mtdBinary에 저장
    sBinaryPrecision = ID_SIZEOF(mtfDecodeMinInfo);
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

IDE_RC mtfDecodeMinInitialize( mtcNode*     aNode,
                               mtcStack*,
                               SInt,
                               void*,
                               mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeMinInitialize"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtcColumn   * sColumn;
    mtcNode           * sArgNode[3];
    mtdBinaryType     * sValue;
    mtfDecodeMinInfo  * sInfo;
    UInt                sFence;

    sFence = aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK;

    sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
    sValue = (mtdBinaryType*)
        ((UChar*)aTemplate->rows[aNode->table].row + sColumn[1].column.offset);

    // BUG-43709
    sValue->mLength = sColumn[1].precision;
    sInfo = (mtfDecodeMinInfo*)(sValue->mValue);

    //-----------------------------
    // min info 초기화
    //-----------------------------
    sArgNode[0] = aNode->arguments;

    // min column 설정
    sInfo->sMinColumnExecute = aTemplate->rows[sArgNode[0]->table].execute + sArgNode[0]->column;
    sInfo->sMinColumnNode    = sArgNode[0];

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

    //-----------------------------
    // min 결과를 초기화
    //-----------------------------

    sInfo->sReturnColumn->module->null( sInfo->sReturnColumn,
                                        sInfo->sReturnValue );

    return IDE_SUCCESS;
#undef IDE_FN
}

IDE_RC mtfDecodeMinAggregate( mtcNode*     aNode,
                              mtcStack*    aStack,
                              SInt         aRemain,
                              void*,
                              mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeMinAggregate"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtdModule   * sModule;
    const mtcColumn   * sColumn;
    mtdBinaryType     * sValue;
    mtfDecodeMinInfo  * sInfo;
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
    sInfo = (mtfDecodeMinInfo*)(sValue->mValue);

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
        IDE_TEST( sInfo->sMinColumnExecute->calculate( sInfo->sMinColumnNode,
                                                       aStack,
                                                       aRemain,
                                                       sInfo->sMinColumnExecute->calculateInfo,
                                                       aTemplate )
                  != IDE_SUCCESS );

        IDE_DASSERT( sInfo->sMinColumnNode->conversion == NULL );
        sModule = aStack[0].column->module;

        sValueInfo1.column = sInfo->sReturnColumn;
        sValueInfo1.value  = sInfo->sReturnValue;
        sValueInfo1.flag   = MTD_OFFSET_USELESS;

        sValueInfo2.column = aStack[0].column;
        sValueInfo2.value  = aStack[0].value;
        sValueInfo2.flag   = MTD_OFFSET_USELESS;

        if ( sModule->logicalCompare[MTD_COMPARE_ASCENDING]( &sValueInfo1,
                                                             &sValueInfo2 ) > 0 )
        {
            idlOS::memcpy( sInfo->sReturnValue,
                           aStack[0].value,
                           sModule->actualSize( aStack[0].column,
                                                aStack[0].value ) );
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

IDE_RC mtfDecodeMinFinalize( mtcNode*,
                             mtcStack*,
                             SInt,
                             void*,
                             mtcTemplate* )
{
#define IDE_FN "IDE_RC mtfDecodeMinFinalize"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC mtfDecodeMinCalculate( mtcNode*     aNode,
                              mtcStack*    aStack,
                              SInt,
                              void*,
                              mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC mtfDecodeMinCalculate"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    aStack->column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack->value  = (void*)( (UChar*) aTemplate->rows[aNode->table].row
                              + aStack->column->column.offset );

    return IDE_SUCCESS;
#undef IDE_FN
}
