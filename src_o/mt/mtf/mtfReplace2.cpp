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
 * $Id: mtfReplace2.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtl.h>

extern mtfModule mtfReplace2;


static mtcName mtfReplace2FunctionName[2] = {
    { mtfReplace2FunctionName+1, 7, (void*)"REPLACE"  }, // fix BUG-17653
    { NULL,                      8, (void*)"REPLACE2" }
};

static IDE_RC mtfReplace2Estimate( mtcNode*     aNode,
                                   mtcTemplate* aTemplate,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   mtcCallBack* aCallBack );

mtfModule mtfReplace2 = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfReplace2FunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfReplace2Estimate
};

static IDE_RC mtfReplace2CalculateFor2Args( mtcNode*     aNode,
                                            mtcStack*    aStack,
                                            SInt         aRemain,
                                            void*        aInfo,
                                            mtcTemplate* aTemplate );

static IDE_RC mtfReplace2CalculateFor3Args( mtcNode*     aNode,
                                            mtcStack*    aStack,
                                            SInt         aRemain,
                                            void*        aInfo,
                                            mtcTemplate* aTemplate );

static IDE_RC mtfReplace2CalculateNcharFor2Args( mtcNode*     aNode,
                                            mtcStack*    aStack,
                                            SInt         aRemain,
                                            void*        aInfo,
                                            mtcTemplate* aTemplate );

static IDE_RC mtfReplace2CalculateNcharFor3Args( mtcNode*     aNode,
                                            mtcStack*    aStack,
                                            SInt         aRemain,
                                            void*        aInfo,
                                            mtcTemplate* aTemplate );

const mtcExecute mtfExecuteFor2Args = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfReplace2CalculateFor2Args,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

const mtcExecute mtfExecuteFor3Args = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfReplace2CalculateFor3Args,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

const mtcExecute mtfExecuteNcharFor2Args = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfReplace2CalculateNcharFor2Args,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

const mtcExecute mtfExecuteNcharFor3Args = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfReplace2CalculateNcharFor3Args,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

/*-----------------------------------------------------------------------------
 * Name :
 *    IDE_RC mtfReplace2Estimate()
 *
 * Argument :
 *    aNode - 입력정보
 *    aStack - 입력값
 *
 * Description :
 *    1. 아규먼트 입력개수가 2개인지 3개인지 , 아니면 에러!
 *    2. 입력이 2개면 *sModules[0]->language->replace22 를 실행
 *    3. precision 계산
 *     or
 *    2. 입력이 3개면 *sModules[0]->language->replace23 를 실행
 *    3. precision 계산
 * ---------------------------------------------------------------------------*/

IDE_RC mtfReplace2Estimate( mtcNode*     aNode,
                            mtcTemplate* aTemplate,
                            mtcStack*    aStack,
                            SInt      /* aRemain */,
                            mtcCallBack* aCallBack )
{
    const mtdModule* sModules[3];
    SInt             sReturnLength;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) < 2 ||
                    ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) > 3,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST( mtf::getCharFuncResultModule( &sModules[0],
                                            aStack[1].column->module )
              != IDE_SUCCESS );

    sModules[1] = sModules[0];
    sModules[2] = sModules[0];

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    // PROJ-1579 NCHAR
    if( (aStack[1].column->module->id == MTD_NCHAR_ID) ||
        (aStack[1].column->module->id == MTD_NVARCHAR_ID) )
    {
        if( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) == 2 )
        {
            aTemplate->rows[aNode->table].execute[aNode->column] =
                                                        mtfExecuteNcharFor2Args;

            sReturnLength = aStack[1].column->precision;
        }
        else
        {
            aTemplate->rows[aNode->table].execute[aNode->column] =
                                                        mtfExecuteNcharFor3Args;

            if( aStack[2].column->precision > 0 )
            {
                /* BUG-44082 REPLACE(CHAR, NCHAR, NCHAR/CHAR)가 실패할 수 있습니다.
                 * BUG-44129 REPLACE2()에 Pattern String을 Column으로 지정하면, 실패할 수 있습니다.  
                 * BUG-44137 REPLACE(NCHAR, CHAR, NCHAR/CHAR)가 실패할 수 있습니다.
                 */
                if( aStack[1].column->language->id == MTL_UTF16_ID )
                {
                    sReturnLength = IDL_MIN( ( aStack[1].column->precision * aStack[3].column->precision ),
                                             (SInt)MTD_UTF16_NCHAR_PRECISION_MAXIMUM );
                }
                else
                {
                    sReturnLength = IDL_MIN( ( aStack[1].column->precision * aStack[3].column->precision ),
                                             (SInt)MTD_UTF8_NCHAR_PRECISION_MAXIMUM );
                }
            }
            else
            {
                sReturnLength = aStack[1].column->precision;
            }
        }
    }
    else
    {
        if( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) == 2 )
        {
            aTemplate->rows[aNode->table].execute[aNode->column] =
                                                        mtfExecuteFor2Args;

            sReturnLength = aStack[1].column->precision;
        }
        else
        {
            aTemplate->rows[aNode->table].execute[aNode->column] =
                                                        mtfExecuteFor3Args;

            // To fix BUG-14491
            // arg1 : source string length
            // arg2 : pattern string length
            // arg3 : replace string length
            // ret  : return string length **
            // (1) = arg1 / arg2 : source에서 나올 수 있는 최대 패턴 개수
            // (2) = (1) * arg3  : 최대 패턴 개수 * 바뀔 스트링의 길이
            // To fix BUG-15085
            // 맞지 않는 패턴을 최대로 계산하여야 한다.
            // (3) = arg1 : source에서 나올 수 있는 최대의 맞지않는 패턴 개수
            // (4) = (2) + (3)   : 패턴에 최대한 들어맞는다고 할때의 최종 길이
            // 하지만 CHAR의 precision maximum에 넘을 수 없기 때문에 다음과 같이
            // 제약을 둔다.
            // ret = MIN( (4), MTD_CHAR_PRECISION_MAXIMUM )
            //
            // 공식을 풀면 다음과 같음.
            // ret = MIN( ((arg1 / arg2) * arg3) + (arg1),
            //            MTD_CHAR_PRECISION_MAXIMUM )
            // precision이 0인 데이터가 올 경우를 고려해야 함.
            // ex) create table t1( i1 varchar(0) );
            //     insert into t1 values( null );
            //     select replace2( 'aaa', i1, 'bb' ) from t1;
            // 위와 같은 경우 argument 2에 precision이 0인 값이 올 수 있다.
            // precision이 0이면 divide by zero가 발생하므로 이를 고려하여
            // precision이 0보다 크거나 그렇지 않은 경우로 나눈다.

            if( aStack[2].column->precision > 0 )
            {
                /* BUG-44082 REPLACE(CHAR, NCHAR, NCHAR/CHAR)가 실패할 수 있습니다.
                 * BUG-44129 REPLACE2()에 Pattern String을 Column으로 지정하면, 실패할 수 있습니다.  
                 * BUG-44137 REPLACE(NCHAR, CHAR, NCHAR/CHAR)가 실패할 수 있습니다.
                 */
                sReturnLength = IDL_MIN( ( aStack[1].column->precision * aStack[3].column->precision ),
                                         MTD_CHAR_PRECISION_MAXIMUM );
            }
            else
            {
                sReturnLength = aStack[1].column->precision;
            }

            // PROJ-1579 NCHAR
            // NCHAR의 precision은 문자 수이기 때문에 langauge에 맞게 최대
            // byte precision을 구한다.
            if( (aStack[3].column->module->id == MTD_NCHAR_ID) ||
                (aStack[3].column->module->id == MTD_NVARCHAR_ID) )
            {
                if( aStack[3].column->language->id == MTL_UTF16_ID )
                {
                    sReturnLength *= MTL_UTF16_PRECISION;
                }
                else
                {
                    sReturnLength *= MTL_UTF8_PRECISION;
                }
            }
            else
            {
                // Nothing to do
            }
        }
    }

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     sModules[0],
                                     1,
                                     sReturnLength,
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

IDE_RC mtfReplace2Calculate( const mtlModule * aLanguage,
                             UChar           * aResult,
                             UShort            aResultMaxLen,
                             UShort          * aResultLen,
                             UChar           * aSource,
                             UShort            aSourceLen,
                             UChar           * aFrom,
                             UShort            aFromLen,
                             UChar           * aTo,
                             UShort            aToLen )
{
/***********************************************************************
 *
 * Description : Replace2 Calculate
 *
 * Implementation :
 *
 ***********************************************************************/
    UChar   * sSourceIndex;
    UChar   * sSourceFence;
    UChar   * sCurSourceIndex;
    UChar   * sFromIndex;
    UChar   * sFromFence;
    UChar   * sNextIndex;
    idBool    sIsSame = ID_FALSE;
    UShort    sResultFence = aResultMaxLen;
    UShort    sResultLen = 0;
    UChar     sSourceSize;
    UChar     sFromSize;

    if ( aFromLen == 0 )
    {
        //------------------------------------
        // From이 NULL인 경우, Source를 그대로 Result에 복사한다
        //------------------------------------
        
        idlOS::memcpy( aResult, aSource, aSourceLen );
        
        sResultLen = aSourceLen;
    }
    else
    {
        //------------------------------------
        // Source에서 From과 동일한 string를 제외한 문자만 결과에 저장
        //------------------------------------

        sSourceIndex = aSource;
        sSourceFence = sSourceIndex + aSourceLen;
        
        while ( sSourceIndex < sSourceFence )
        {
            //------------------------------------
            // from string과 동일한 string 존재하는지 검사
            //------------------------------------

            sIsSame = ID_FALSE;
            sCurSourceIndex = sSourceIndex;
            
            sFromIndex = aFrom;
            sFromFence = sFromIndex + aFromLen;
            
            while ( sFromIndex < sFromFence )
            {
                sSourceSize =  mtl::getOneCharSize( sCurSourceIndex,
                                                    sSourceFence,
                                                    aLanguage ); 
                
                sFromSize =  mtl::getOneCharSize( sFromIndex,
                                                  sFromFence,
                                                  aLanguage );

                sIsSame = mtc::compareOneChar( sCurSourceIndex,
                                               sSourceSize,
                                               sFromIndex,
                                               sFromSize );
                
                if ( sIsSame == ID_FALSE )
                {
                    break;
                }
                else
                {
                    // 동일하면 다음 문자로 진행
                    (void)aLanguage->nextCharPtr( & sFromIndex, sFromFence );
                                        
                    (void)aLanguage->nextCharPtr( & sCurSourceIndex, sSourceFence );
                    
                    // Source는 마지막 문자인데, From은 마지막 문자가 아닌 경우
                    if ( sCurSourceIndex == sSourceFence )
                    {
                        if ( sFromIndex < sFromFence )
                        {
                            sIsSame = ID_FALSE;
                            break;
                        }
                        else
                        {
                            // nothing to do
                        }
                    }
                    else
                    {
                        // nothing to do
                    }
                }
            }

            if ( sIsSame == ID_TRUE )
            {
                //-------------------------------------------
                // from string과 동일한 string이 존재하는 경우
                //-------------------------------------------
                
                if ( aTo == NULL )
                {
                    // To가 NULL인 경우, From에 해당하는 문자는 제거

                    // Nothing to do.
                }
                else
                {
                    if ( aToLen > 0 )
                    {
                        // To string을 From string 대신에 삽입
                        IDE_TEST_RAISE( sResultLen + aToLen > sResultFence,
                                        ERR_INVALID_LENGTH );
                        
                        idlOS::memcpy( aResult + sResultLen,
                                       aTo,
                                       aToLen);
                        sResultLen += aToLen;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
                
                sSourceIndex = sCurSourceIndex;
            }
            else
            {
                //------------------------------------
                // from string과 동일한 string이 존재하지 않는 경우
                //------------------------------------

                // To Fix BUG-12606, 12696
                // 다음 문자로 진행
                sNextIndex = sSourceIndex;
                
                (void)aLanguage->nextCharPtr( & sNextIndex, sSourceFence );
                
                // 현재 문자를 결과에 저장
                // 현재 문자가 1byte 이상인 경우에도 정상 수행하기 위하여
                // 다음 문자 이전까지의 문자를 result에 copy
                idlOS::memcpy( aResult + sResultLen,
                               sSourceIndex,
                               sNextIndex - sSourceIndex );
                sResultLen += sNextIndex - sSourceIndex;
                
                sSourceIndex = sNextIndex;
            }
        }
    }

    *aResultLen = sResultLen;

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_LENGTH );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LENGTH));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfReplace2CalculateFor2Args( mtcNode*     aNode,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     void*        aInfo,
                                     mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : Replace2 Calculate with 2 arguments
 *
 * Implementation :
 *    REPLACE2( char, string1 )
 *
 *    aStack[0] : char 중 string1에 해당하는 부분이 삭제됨
 *    aStack[1] : char 
 *    aStack[2] : string1 ( 치환 대상 문자 ) 
 *
 *    ex) REPLACE2( dname, '팀' )
 *        ==> '개발팀'이 '개발'로 출력됨, 즉 치환 대상 문자 삭제
 *
 ***********************************************************************/
    
    mtdCharType* sResult;
    mtdCharType* sSource;
    mtdCharType* sFrom;
    
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
        //------------------------------------
        // 기본 초기화
        //------------------------------------
        
        sResult   = (mtdCharType*)aStack[0].value;
        sSource   = (mtdCharType*)aStack[1].value;
        sFrom     = (mtdCharType*)aStack[2].value;

        // replace
        IDE_TEST( mtfReplace2Calculate( aStack[1].column->language,
                                        sResult->value,
                                        aStack[0].column->precision,
                                        & sResult->length,
                                        sSource->value,
                                        sSource->length,
                                        sFrom->value,
                                        sFrom->length,
                                        NULL,
                                        0 )
                  != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfReplace2CalculateFor3Args( mtcNode*     aNode,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     void*        aInfo,
                                     mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : Replace2 Calculate with 3 arguments
 *
 * Implementation :
 *    REPLACE2( char, string1, string2 )
 *
 *    aStack[0] : char 중 string1에 해당하는 부분이 삭제됨
 *    aStack[1] : char 
 *    aStack[2] : string1 ( 치환 대상 문자 )
 *    aStack[3] : string2 ( 치환 문자 ) 
 *
 *    ex) REPLACE2( dname, '팀', '부문' )
 *        ==> '개발팀'이 '개발부문'로 출력됨,
 *             즉 치환 대상 문자가 치환문자로 변경되어 출력됨
 *
 ***********************************************************************/
    
    mtdCharType* sResult;
    mtdCharType* sSource;
    mtdCharType* sFrom;
    mtdCharType* sTo;
    
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
        sResult   = (mtdCharType*)aStack[0].value;
        sSource   = (mtdCharType*)aStack[1].value;
        sFrom     = (mtdCharType*)aStack[2].value;
        sTo       = (mtdCharType*)aStack[3].value;

        // replace
        IDE_TEST( mtfReplace2Calculate( aStack[1].column->language,
                                        sResult->value,
                                        aStack[0].column->precision,
                                        & sResult->length,
                                        sSource->value,
                                        sSource->length,
                                        sFrom->value,
                                        sFrom->length,
                                        sTo->value,
                                        sTo->length )
                  != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfReplace2CalculateNcharFor2Args( mtcNode*     aNode,
                                          mtcStack*    aStack,
                                          SInt         aRemain,
                                          void*        aInfo,
                                          mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : Replace2 Calculate with 2 arguments for NCHAR
 *
 * Implementation :
 *    REPLACE2( char, string1 )
 *
 *    aStack[0] : char 중 string1에 해당하는 부분이 삭제됨
 *    aStack[1] : char 
 *    aStack[2] : string1 ( 치환 대상 문자 ) 
 *
 *    ex) REPLACE2( dname, '팀' )
 *        ==> '개발팀'이 '개발'로 출력됨, 즉 치환 대상 문자 삭제
 *
 ***********************************************************************/
    
    mtdNcharType    * sResult;
    mtdNcharType    * sSource;
    mtdNcharType    * sFrom;
    const mtlModule * sSrcCharSet;
    UShort            sResultMaxLen;

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
        //------------------------------------
        // 기본 초기화
        //------------------------------------

        sResult = (mtdNcharType*)aStack[0].value;
        sSource = (mtdNcharType*)aStack[1].value;
        sFrom   = (mtdNcharType*)aStack[2].value;

        sSrcCharSet = aStack[1].column->language;

        sResultMaxLen = sSrcCharSet->maxPrecision(aStack[0].column->precision);

        // ------------------------------
        // Replace2 공통 함수
        // ------------------------------

        // replace
        IDE_TEST( mtfReplace2Calculate( aStack[1].column->language,
                                        sResult->value,
                                        sResultMaxLen,
                                        & sResult->length,
                                        sSource->value,
                                        sSource->length,
                                        sFrom->value,
                                        sFrom->length,
                                        NULL,
                                        0 )
                  != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfReplace2CalculateNcharFor3Args( mtcNode*     aNode,
                                          mtcStack*    aStack,
                                          SInt         aRemain,
                                          void*        aInfo,
                                          mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : Replace2 Calculate with 3 arguments for NCHAR
 *
 * Implementation :
 *    REPLACE2( char, string1, string2 )
 *
 *    aStack[0] : char 중 string1에 해당하는 부분이 삭제됨
 *    aStack[1] : char 
 *    aStack[2] : string1 ( 치환 대상 문자 )
 *    aStack[3] : string2 ( 치환 문자 ) 
 *
 *    ex) REPLACE2( dname, '팀', '부문' )
 *        ==> '개발팀'이 '개발부문'로 출력됨,
 *             즉 치환 대상 문자가 치환문자로 변경되어 출력됨
 *
 ***********************************************************************/
    
    mtdNcharType    * sResult;
    mtdNcharType    * sSource;
    mtdNcharType    * sFrom;
    mtdNcharType    * sTo;
    const mtlModule * sSrcCharSet;
    UShort            sResultMaxLen;
    
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
        //------------------------------------
        // 기본 초기화
        //------------------------------------

        sResult = (mtdNcharType*)aStack[0].value;
        sSource = (mtdNcharType*)aStack[1].value;
        sFrom   = (mtdNcharType*)aStack[2].value;
        sTo     = (mtdNcharType*)aStack[3].value;

        sSrcCharSet = aStack[1].column->language;

        sResultMaxLen = sSrcCharSet->maxPrecision(aStack[0].column->precision);

        // ------------------------------
        // Replace2 공통 함수
        // ------------------------------

        // replace
        IDE_TEST( mtfReplace2Calculate( aStack[1].column->language,
                                        sResult->value,
                                        sResultMaxLen,
                                        & sResult->length,
                                        sSource->value,
                                        sSource->length,
                                        sFrom->value,
                                        sFrom->length,
                                        sTo->value,
                                        sTo->length )
                  != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
