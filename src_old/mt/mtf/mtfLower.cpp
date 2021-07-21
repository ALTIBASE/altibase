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
 * $Id: mtfLower.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtl.h>

extern mtfModule mtfLower;
extern mtlModule mtlUTF16;

static mtcName mtfLowerFunctionName[1] = {
    { NULL, 5, (void*)"LOWER" }
};

static IDE_RC mtfLowerEstimate( mtcNode*     aNode,
                                mtcTemplate* aTemplate,
                                mtcStack*    aStack,
                                SInt         aRemain,
                                mtcCallBack* aCallBack );

mtfModule mtfLower = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfLowerFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfLowerEstimate
};

static IDE_RC mtfLowerCalculate( mtcNode*     aNode,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 void*        aInfo,
                                 mtcTemplate* aTemplate );

// PROJ-1579 NCHAR
static IDE_RC mtfLowerCalculateChar4MB( mtcNode*     aNode,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      void*        aInfo,
                                      mtcTemplate* aTemplate );

// PROJ-1579 NCHAR
static IDE_RC mtfLowerCalculateNchar4MB( mtcNode*     aNode,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      void*        aInfo,
                                      mtcTemplate* aTemplate );

const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfLowerCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

// PROJ-1579 NCHAR
const mtcExecute mtfExecuteChar4MB = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfLowerCalculateChar4MB,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

// PROJ-1579 NCHAR
const mtcExecute mtfExecuteNchar4MB = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfLowerCalculateNchar4MB,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfLowerEstimate( mtcNode*     aNode,
                         mtcTemplate* aTemplate,
                         mtcStack*    aStack,
                         SInt      /* aRemain */,
                         mtcCallBack* aCallBack )
{
    const mtdModule* sModules[1];

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST( mtf::getCharFuncResultModule( &sModules[0],
                                            aStack[1].column->module )
              != IDE_SUCCESS );

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    // PROJ-1579 NCHAR
    if( aStack[1].column->language->id == MTL_ASCII_ID )
    {
        aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;
    }
    else
    {
        if( (aStack[1].column->module->id == MTD_NCHAR_ID) ||
            (aStack[1].column->module->id == MTD_NVARCHAR_ID) )
        {
            aTemplate->rows[aNode->table].execute[aNode->column] =
                                                        mtfExecuteNchar4MB;
        }
        else
        {
            aTemplate->rows[aNode->table].execute[aNode->column] =
                                                        mtfExecuteChar4MB;
        }
    }

    /*
    IDE_TEST( sModules[0]->estimate( aStack[0].column,
                                     1,
                                     aStack[1].column->precision,
                                     0 )
              != IDE_SUCCESS );
    */

    // PROJ-1579 NCHAR
    // ASCII 이외의 문자에 대한 대소문자 변환에서도
    // precision이 변하지는 않는다.
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     sModules[0],
                                     1,
                                     aStack[1].column->precision,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfLowerCalculate( mtcNode*     aNode,
                          mtcStack*    aStack,
                          SInt         aRemain,
                          void*        aInfo,
                          mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : Lower Calculate
 *
 * Implementation :
 *    LOWER( char )
 *
 *    aStack[0] : 주어진 문자열을 소문자로 변환한 값
 *    aStack[1] : char ( 주어진 문자열 )
 *
 *    ex) LOWER( 'ABC' ) ==> result : abc
 *
 ***********************************************************************/
    
    mtdCharType*   sResult;
    mtdCharType*   sInput;
    UChar*         sCurResult;
    UChar*         sCurInput;
    UChar*         sFence;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );
    
    if( aStack[1].column->module->isNull( aStack[1].column,
                                          aStack[1].value ) == ID_TRUE )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sResult = (mtdCharType*)aStack[0].value;
        sInput  = (mtdCharType*)aStack[1].value;

        // sResult->length에 주어진 문자열의 길이 저장
        sResult->length = sInput->length;

        // sResult->value에 주어진 문자열을 저장하되,
        // 대문자는 소문자로 변환하여 저장한다.
        for( sCurResult = sResult->value,
             sCurInput  = sInput->value,
             sFence    = sCurInput + sInput->length;
             sCurInput < sFence;
             sCurInput++, sCurResult++ )
        {
            if( *sCurInput >= 'A' && *sCurInput <= 'Z' )
            {
                // 대문자인 경우, 소문자로 변환하여 결과에 저장
                *sCurResult = *sCurInput + 0x20;
            }
            else
            {
                *sCurResult = *sCurInput;
            }
        }
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfLowerCalculateChar4MB( mtcNode*     aNode,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 void*        aInfo,
                                 mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : 
 *      PROJ-1579 NCHAR
 *      CHAR 타입에 대한 소문자 변환
 *
 * Implementation :
 *    LOWER( char )
 *
 *    aStack[0] : 입력 문자열을 소문자로 변환한 값
 *    aStack[1] : char ( 입력 문자열 )
 *
 *    ex) LOWER( 'Capital' ) ==> 'CAPITAL'
 *
 ***********************************************************************/
    
    mtdCharType        * sResult;
    mtdCharType        * sSource;
    UChar              * sSourceIndex;
    UChar              * sTempIndex;
    UChar              * sSourceFence;
    UChar              * sResultValue;
    UChar              * sResultFence;

    const mtlModule    * sSrcCharSet;
    const mtlModule    * sDestCharSet;
    idnCharSetList       sIdnSrcCharSet;
    idnCharSetList       sIdnDestCharSet;
    SInt                 sSrcRemain = 0;
    SInt                 sDestRemain = 0;
    SInt                 sTempRemain = 0;
    mtlU16Char           sU16Result;
    SInt                 sU16ResultLen;
    mtlU16Char           sLowerResult;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );
    
    if( aStack[1].column->module->isNull( aStack[1].column,
                                          aStack[1].value ) == ID_TRUE )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sResult = (mtdCharType*)aStack[0].value;
        sSource = (mtdCharType*)aStack[1].value;

        sSourceIndex = sSource->value;
        sSrcRemain   = sSource->length;
        sSourceFence = sSourceIndex + sSrcRemain;

        // 변환 결과의 크기를 체크하기 위함
        sDestRemain = aStack[0].column->precision;

        sResultValue = sResult->value;
        sResultFence = sResultValue + sDestRemain;

        // 결과의 길이와 입력의 길이는 같다.
        sResult->length = sSource->length;

        sSrcCharSet = aStack[1].column->language;
        sDestCharSet = & mtlUTF16;

        // --------------------------------------------------------
        // 아래와 같은 작업이 필요하다.
        // 1. SrcCharSet => UTF16으로 변환
        // 2. 대소문자 변환표 적용
        // 3. UTF16 => SrcCharSet으로 변환
        // --------------------------------------------------------

        sIdnSrcCharSet = mtl::getIdnCharSet( sSrcCharSet );
        sIdnDestCharSet = mtl::getIdnCharSet( sDestCharSet );

        while( sSourceIndex < sSourceFence )
        {
            IDE_TEST_RAISE( sResultValue >= sResultFence,
                            ERR_INVALID_DATA_LENGTH );

            sTempRemain = sDestRemain;

            // 7bit ASCII일 경우에는 변환표 적용이 필요없다.
            if( IDN_IS_ASCII( *sSourceIndex ) == ID_TRUE )
            {
                if( *sSourceIndex >= 'A' && *sSourceIndex <= 'Z' )
                {
                    *sResultValue = *sSourceIndex + 0x20;
                }
                else
                {
                    *sResultValue = *sSourceIndex;
                }
                sDestRemain -= 1;
            }
            else
            {
                sU16ResultLen = MTL_UTF16_PRECISION;

                // 1. SrcCharSet => DestCharSet(UTF16)으로 변환
                IDE_TEST( convertCharSet( sIdnSrcCharSet,
                                          sIdnDestCharSet,
                                          sSourceIndex,
                                          sSrcRemain,
                                          & sU16Result,
                                          & sU16ResultLen,
                                          -1 /* mNlsNcharConvExcp */ )
                          != IDE_SUCCESS );

                /* BUG-42671 깨진 문자가 있을 경우
                 * UTF16의 Replaceement character은 2Byte인 반명
                 * utf8의 Replacement character은 3byte로 인해
                 * DestRemain은 3byte씩 줄로 Src는 1byte씩 증가되어
                 * 결국 진행되다보변 버퍼가 모자르게 되므로
                 * utf18의 ? 을 utf8 의 ? 로 변환하지 않고 글자 그대로
                 * 를 copy 한다
                 */
                if ( ( sU16Result.value1 == 0xff ) &&
                     ( sU16Result.value2 == 0xfd ) )
                {
                    *sResultValue = *sSourceIndex;
                    sDestRemain--;
                }
                else
                {
                    // 2. 대소문자 변환표 적용
                    // IDN_NLS_CASE_UNICODE_MAX보다 작을 경우에만 변환한다.
                    // IDN_NLS_CASE_UNICODE_MAX보다 크면 대소문자 변환이
                    // 의미가 없으므로 그대로 copy한다.
                    mtl::getUTF16LowerStr( &sLowerResult, &sU16Result );

                    // 3. DestCharSet(UTF16) => SrcCharSet으로 변환
                    IDE_TEST( convertCharSet( sIdnDestCharSet,
                                              sIdnSrcCharSet,
                                              & sLowerResult,
                                              MTL_UTF16_PRECISION,
                                              sResultValue,
                                              & sDestRemain,
                                              -1 /* mNlsNcharConvExcp */ )
                              != IDE_SUCCESS );
                }
            }

            sTempIndex = sSourceIndex;

            (void)sSrcCharSet->nextCharPtr( & sSourceIndex, sSourceFence );

            sResultValue += (sTempRemain - sDestRemain);
            
            sSrcRemain -= ( sSourceIndex - sTempIndex );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_DATA_LENGTH );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_VALIDATE_INVALID_LENGTH));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfLowerCalculateNchar4MB( mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*        aInfo,
                                  mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : 
 *      PROJ-1579 NCHAR
 *      NCHAR 타입에 대한 소문자 변환
 *
 * Implementation :
 *    LOWER( nchar )
 *
 *    aStack[0] : 입력 문자열을 소문자로 변환한 값
 *    aStack[1] : nchar ( 입력 문자열 )
 *
 *    ex) LOWER( 'Capital' ) ==> 'CAPITAL'
 *
 ***********************************************************************/
    
    mtdNcharType       * sResult;
    mtdNcharType       * sSource;
    UChar              * sSourceIndex;
    UShort               sSourceLen;
    UChar              * sTempIndex;
    UChar              * sSourceFence;
    UChar              * sResultValue;
    UChar              * sResultFence;

    const mtlModule    * sSrcCharSet;
    const mtlModule    * sDestCharSet;
    idnCharSetList       sIdnSrcCharSet;
    idnCharSetList       sIdnDestCharSet;
    SInt                 sSrcRemain = 0;
    SInt                 sDestRemain = 0;
    SInt                 sTempRemain = 0;
    mtlU16Char           sU16Result;
    SInt                 sU16ResultLen;
    mtlU16Char           sLowerResult;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );
    
    if( aStack[1].column->module->isNull( aStack[1].column,
                                          aStack[1].value ) == ID_TRUE )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sResult = (mtdNcharType*)aStack[0].value;
        sSource = (mtdNcharType*)aStack[1].value;

        // UTF8 or UTF16
        sSrcCharSet = aStack[1].column->language;
        sDestCharSet = & mtlUTF16;

        sSourceIndex = sSource->value;
        sSourceLen   = sSource->length;
        sSrcRemain   = sSourceLen;
        sSourceFence = sSourceIndex + sSrcRemain;

        sResultValue = sResult->value;
        sDestRemain  = sSrcCharSet->maxPrecision(aStack[0].column->precision);
        sResultFence = sResultValue + sDestRemain;

        // 결과의 길이와 입력의 길이는 같다.
        sResult->length = sSource->length;

        // ------------------------------------
        // 대소문자 변환표를 적용하기 위해
        // UTF16 캐릭터 셋으로 변환한다.
        // ------------------------------------
        if( sSrcCharSet->id == MTL_UTF16_ID )
        {
            while( sSourceIndex < sSourceFence )
            {
                IDE_TEST_RAISE( sResultValue >= sResultFence,
                                ERR_INVALID_DATA_LENGTH );
                
                mtl::getUTF16LowerStr( (mtlU16Char*)sResultValue,
                                       (mtlU16Char*)sSourceIndex );

                (void)sSrcCharSet->nextCharPtr( & sSourceIndex, sSourceFence );

                sResultValue += MTL_UTF16_PRECISION;
            }
        }
        else
        {
            // --------------------------------------------------------
            // SrcCharSet이 UTF16이 아니면 아래와 같은 작업이 필요하다.
            // 1. SrcCharSet(UTF8) => UTF16으로 변환
            // 2. 대소문자 변환표 적용
            // 3. UTF16 => SrcCharSet(UTF8)으로 변환
            // --------------------------------------------------------

            sIdnSrcCharSet = mtl::getIdnCharSet( sSrcCharSet );
            sIdnDestCharSet = mtl::getIdnCharSet( sDestCharSet );

            while( sSourceIndex < sSourceFence )
            {
                IDE_TEST_RAISE( sResultValue >= sResultFence,
                                ERR_INVALID_DATA_LENGTH );

                sTempRemain = sDestRemain;
                
                // 7bit ASCII일 경우에는 변환표 적용이 필요없다.
                if( IDN_IS_ASCII( *sSourceIndex ) == ID_TRUE )
                {
                    if( *sSourceIndex >= 'A' && *sSourceIndex <= 'Z' )
                    {
                        *sResultValue = *sSourceIndex + 0x20;
                    }
                    else
                    {
                        *sResultValue = *sSourceIndex;
                    }
                    sDestRemain -= 1;
                }
                else
                {
                    sU16ResultLen = MTL_UTF16_PRECISION;

                    // 1. SrcCharSet(UTF8) => DestCharSet(UTF16)으로 변환
                    IDE_TEST( convertCharSet( sIdnSrcCharSet,
                                              sIdnDestCharSet,
                                              sSourceIndex,
                                              sSrcRemain,
                                              & sU16Result,
                                              & sU16ResultLen,
                                              -1 /* mNlsNcharConvExcp */ )
                              != IDE_SUCCESS );

                    /* BUG-42671 깨진 문자가 있을 경우
                     * UTF16의 Replaceement character은 2Byte인 반명
                     * utf8의 Replacement character은 3byte로 인해
                     * DestRemain은 3byte씩 줄로 Src는 1byte씩 증가되어
                     * 결국 진행되다보변 버퍼가 모자르게 되므로
                     * utf18의 ? 을 utf8 의 ? 로 변환하지 않고 글자 그대로
                     * 를 copy 한다
                     */
                    if ( ( sU16Result.value1 == 0xff ) &&
                         ( sU16Result.value2 == 0xfd ) )
                    {
                        *sResultValue = *sSourceIndex;
                        sDestRemain--;
                    }
                    else
                    {
                        // 2. 대소문자 변환표 적용
                        // IDN_NLS_CASE_UNICODE_MAX보다 작을 경우에만 변환한다.
                        // IDN_NLS_CASE_UNICODE_MAX보다 크면 대소문자 변환이
                        // 의미가 없으므로 그대로 copy한다.

                        mtl::getUTF16LowerStr( &sLowerResult, &sU16Result );
                        
                        // 3. DestCharSet(UTF16) => SrcCharSet(UTF8)으로 변환
                        IDE_TEST( convertCharSet( sIdnDestCharSet,
                                                  sIdnSrcCharSet,
                                                  & sLowerResult,
                                                  MTL_UTF16_PRECISION,
                                                  sResultValue,
                                                  & sDestRemain,
                                                  -1 /* mNlsNcharConvExcp */ )
                                  != IDE_SUCCESS );
                    }
                }

                sTempIndex = sSourceIndex;

                (void)sSrcCharSet->nextCharPtr( & sSourceIndex, sSourceFence );

                sResultValue += (sTempRemain - sDestRemain);
                
                sSrcRemain -= ( sSourceIndex - sTempIndex );
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_DATA_LENGTH );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_VALIDATE_INVALID_LENGTH));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
