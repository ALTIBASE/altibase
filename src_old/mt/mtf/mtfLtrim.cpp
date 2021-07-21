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
 * $Id: mtfLtrim.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtl.h>

extern mtfModule mtfLtrim;

extern mtdModule mtdVarchar;
extern mtdModule mtdNvarchar;

static mtcName mtfLtrimFunctionName[1] = {
    { NULL, 5, (void*)"LTRIM" }
};

static IDE_RC mtfLtrimEstimate( mtcNode*     aNode,
                                mtcTemplate* aTemplate,
                                mtcStack*    aStack,
                                SInt         aRemain,
                                mtcCallBack* aCallBack );

mtfModule mtfLtrim = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfLtrimFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfLtrimEstimate
};

static IDE_RC mtfLtrimCalculateFor1Arg( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        void*        aInfo,
                                        mtcTemplate* aTemplate );

static IDE_RC mtfLtrimCalculateFor2Args( mtcNode*     aNode,
                                         mtcStack*    aStack,
                                         SInt         aRemain,
                                         void*        aInfo,
                                         mtcTemplate* aTemplate );

static IDE_RC mtfLtrimCalculateNcharFor1Arg( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        void*        aInfo,
                                        mtcTemplate* aTemplate );

static IDE_RC mtfLtrimCalculateNcharFor2Args( mtcNode*     aNode,
                                         mtcStack*    aStack,
                                         SInt         aRemain,
                                         void*        aInfo,
                                         mtcTemplate* aTemplate );

const mtcExecute mtfExecuteFor1Arg = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfLtrimCalculateFor1Arg,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

const mtcExecute mtfExecuteFor2Args = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfLtrimCalculateFor2Args,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

const mtcExecute mtfExecuteNcharFor1Arg = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfLtrimCalculateNcharFor1Arg,
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
    mtfLtrimCalculateNcharFor2Args,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfLtrimEstimate( mtcNode*     aNode,
                         mtcTemplate* aTemplate,
                         mtcStack*    aStack,
                         SInt      /* aRemain */,
                         mtcCallBack* aCallBack )
{
    const mtdModule* sModules[2];

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) < 1 ||
                    ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) > 2,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST( mtf::getCharFuncResultModule( &sModules[0],
                                            aStack[1].column->module )
              != IDE_SUCCESS );
    sModules[1] = sModules[0];

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
        if( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) == 1 )
        {
            aTemplate->rows[aNode->table].execute[aNode->column] = 
                                                    mtfExecuteNcharFor1Arg;
        }
        else
        {
            aTemplate->rows[aNode->table].execute[aNode->column] = 
                                                    mtfExecuteNcharFor2Args;
        }

        IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                         &mtdNvarchar,
                                         1,
                                         aStack[1].column->precision,
                                         0 )
                  != IDE_SUCCESS );
    }
    else
    {
        if( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) == 1 )
        {
            aTemplate->rows[aNode->table].execute[aNode->column] = 
                                                            mtfExecuteFor1Arg;
        }
        else
        {
            aTemplate->rows[aNode->table].execute[aNode->column] = 
                                                            mtfExecuteFor2Args;
        }

        IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                         &mtdVarchar,  // BUG-16501
                                         1,
                                         aStack[1].column->precision,
                                         0 )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfLeftTrim( const mtlModule * aLanguage,
                    UChar           * aResult,
                    UShort            aResultMaxLen,
                    UShort          * aResultLen,
                    UChar           * aSource,
                    UShort            aSourceLen,
                    UChar           * aTrim,
                    UShort            aTrimLen )
{
/***********************************************************************
 *
 * Description : Left Trim
 *
 * Implementation :
 *
 ***********************************************************************/
    
    UChar  * sSourceIndex;
    UChar  * sSourceFence;
    UChar  * sTrimIndex;
    UChar  * sTrimFence;
    UChar  * sFirst;
    idBool   sIsSame = ID_FALSE;
    UChar    sSourceSize;
    UChar    sTrimSize;
    
    sSourceIndex = aSource;
    sSourceFence = aSource + aSourceLen;
    sFirst = sSourceIndex;

    while ( sSourceIndex < sSourceFence )
    {
        sSourceSize =  mtl::getOneCharSize( sSourceIndex,
                                            sSourceFence,
                                            aLanguage );
        
        // Left->right 방향으로 문자를 읽어가면서 
        // trim 대상 문자열의 문자와 동일한 문자가 있는지 찾음
        sTrimIndex = aTrim;
        sTrimFence = aTrim + aTrimLen;
        
        while ( sTrimIndex < sTrimFence )
        {
            sTrimSize =  mtl::getOneCharSize( sTrimIndex,
                                              sTrimFence,
                                              aLanguage );

            sIsSame = mtc::compareOneChar( sSourceIndex,
                                           sSourceSize,
                                           sTrimIndex,
                                           sTrimSize );
            
            if ( sIsSame == ID_TRUE )
            {
                // 동일한 문자가 있음
                break;
            }
            else
            {
                // 다음 trim 문자로 진행
                // TASK-3420 문자열 처리 정책 개선
                (void)aLanguage->nextCharPtr( & sTrimIndex, sTrimFence );
            }
        }

        if ( sIsSame == ID_FALSE )
        {
            // 동일한 문자가 없는 경우
            sFirst = sSourceIndex;
            break;
        }
        else
        {
            // 다음 source 문자로 진행
            // TASK-3420 문자열 처리 정책 개선
            (void)aLanguage->nextCharPtr( & sSourceIndex, sSourceFence );
        }
    }

    if ( ( sIsSame == ID_TRUE ) && ( sSourceIndex == sSourceFence ) )
    {
        // Source의 모든 문자가 Trim 문자인 경우
        *aResultLen = 0;
    }
    else
    {
        // 동일하지 않은 첫 문자 이후를 결과에 저장
        *aResultLen = sSourceFence - sFirst;
        
        IDE_TEST_RAISE( *aResultLen > aResultMaxLen, ERR_EXCEED_MAX );
        
        idlOS::memcpy( aResult, sFirst, *aResultLen );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXCEED_MAX );
    {
        IDE_SET(ideSetErrorCode( idERR_ABORT_idnReachEnd ));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfLeftTrimFor1ByteSpace( const mtlModule * aLanguage,
                                 UChar           * aResult,
                                 UShort            aResultMaxLen,
                                 UShort          * aResultLen,
                                 UChar           * aSource,
                                 UShort            aSourceLen )
{
/***********************************************************************
 *
 * Description : Left Trim
 *     BUG-10370
 *     1byte space 0x20을 사용하는 charset에 대해 ltrim을 수행한다.
 *
 * Implementation :
 *
 ***********************************************************************/
    
    UChar  * sSourceIndex;
    UChar  * sSourceFence;

    IDE_ASSERT_MSG( aLanguage->id != MTL_UTF16_ID,
                    "aLanguage->id : %"ID_UINT32_FMT"\n",
                    aLanguage->id );

    if ( aSourceLen > 0 )
    {
        sSourceIndex = aSource;
        sSourceFence = aSource + aSourceLen;

        for ( ; sSourceIndex < sSourceFence; sSourceIndex++ )
        {
            if ( *sSourceIndex != *aLanguage->specialCharSet[MTL_SP_IDX] )
            {
                // space가 아닌 경우
                break;
            }
            else
            {
                // Nothing to do.
            }
        }
        
        if ( sSourceIndex == sSourceFence )
        {
            // Source의 모든 문자가 Trim 문자인 경우
            *aResultLen = 0;
        }
        else
        {
            // 동일하지 않은 첫 문자 이후를 결과에 저장
            *aResultLen = sSourceFence - sSourceIndex;
            
            IDE_TEST_RAISE( *aResultLen > aResultMaxLen, ERR_EXCEED_MAX );
            
            idlOS::memcpy( aResult, sSourceIndex, *aResultLen );
        }
    }
    else
    {
        *aResultLen = 0;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXCEED_MAX );
    {
        IDE_SET(ideSetErrorCode( idERR_ABORT_idnReachEnd ));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfLeftTrimFor2ByteSpace( const mtlModule * aLanguage,
                                 UChar           * aResult,
                                 UShort            aResultMaxLen,
                                 UShort          * aResultLen,
                                 UChar           * aSource,
                                 UShort            aSourceLen )
{
/***********************************************************************
 *
 * Description : Left Trim
 *     BUG-10370
 *     2byte space 0x00 0x20을 사용하는 charset에 대해 ltrim을 수행한다.
 *
 * Implementation :
 *
 ***********************************************************************/
    
    UChar  * sSourceIndex;
    UChar  * sSourceFence;

    IDE_ASSERT_MSG( aLanguage->id == MTL_UTF16_ID,
                    "aLanguage->id : %"ID_UINT32_FMT"\n",
                    aLanguage->id );

    if ( aSourceLen > 0 )
    {
        sSourceIndex = aSource;
        sSourceFence = aSource + aSourceLen;

        for ( ; sSourceIndex + 1 < sSourceFence; sSourceIndex += 2 )
        {
            if ( *sSourceIndex !=
                 ((mtlU16Char*)aLanguage->specialCharSet[MTL_SP_IDX])->value1 )
            {
                // space가 아닌 경우
                break;
            }
            else
            {
                // Nothing to do.
            }
            
            if ( *(sSourceIndex + 1) !=
                 ((mtlU16Char*)aLanguage->specialCharSet[MTL_SP_IDX])->value2 )
            {
                // space가 아닌 경우
                break;
            }
            else
            {
                // Nothing to do.
            }
        }
        
        if ( sSourceIndex == sSourceFence )
        {
            // Source의 모든 문자가 Trim 문자인 경우
            *aResultLen = 0;
        }
        else
        {
            // 동일하지 않은 첫 문자 이후를 결과에 저장
            *aResultLen = sSourceFence - sSourceIndex;
        
            IDE_TEST_RAISE( *aResultLen > aResultMaxLen, ERR_EXCEED_MAX );
            
            idlOS::memcpy( aResult, sSourceIndex, *aResultLen );
        }
    }
    else
    {
        *aResultLen = 0;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXCEED_MAX );
    {
        IDE_SET(ideSetErrorCode( idERR_ABORT_idnReachEnd ));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfLtrimCalculateFor1Arg( mtcNode*     aNode,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 void*        aInfo,
                                 mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : Ltrim Calculate with 1 argument
 *
 * Implementation :
 *    LTRIM( char1 )
 *
 *    aStack[0] : 입력한 문자열의 왼쪽에 ' '을 모두 삭제한 값
 *    aStack[1] : char1 ( 입력 문자열 )
 *
 *    ex) LTRIM('   ab' ) ==> 'ab'
 *
 ***********************************************************************/
    
    mtdCharType*   sResult;
    mtdCharType*   sInput;
    
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
        
        IDE_TEST( mtfLeftTrimFor1ByteSpace( aStack[1].column->language,
                                            sResult->value,
                                            aStack[0].column->precision,
                                            &sResult->length,
                                            sInput->value,
                                            sInput->length )
                  != IDE_SUCCESS );

        if ( sResult->length == 0 )
        {
            aStack[0].column->module->null( aStack[0].column,
                                            aStack[0].value );
        }
        else
        {
            // nothing to do
        }
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfLtrimCalculateFor2Args( mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*        aInfo,
                                  mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : Ltrim Calculate with 2 arguments
 *
 * Implementation :
 *    LTRIM( char1, char2 )
 *
 *    aStack[0] : char1의 문자열을 왼쪽부터 char2의 문자와 비교하여
 *                같으면 삭제한다.
 *    aStack[1] : char1 ( 입력 문자열 )
 *    aStack[2] : char2 ( trim 대상 문자열 )
 *
 *    ex) LTRIM( 'abABa', 'ab' )  ==> 'ABa'
 *
 ***********************************************************************/
    
    mtdCharType* sResult;
    mtdCharType* sInput;
    mtdCharType* sTrim;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    if( (aStack[1].column->module->isNull( aStack[1].column,
                                           aStack[1].value ) == ID_TRUE) ||
        (aStack[2].column->module->isNull( aStack[2].column,
                                           aStack[2].value ) == ID_TRUE) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sResult = (mtdCharType*)aStack[0].value;
        sInput  = (mtdCharType*)aStack[1].value;
        sTrim   = (mtdCharType*)aStack[2].value;
        
        IDE_TEST( mtfLeftTrim( aStack[1].column->language,
                               sResult->value,
                               aStack[0].column->precision,
                               &sResult->length,
                               sInput->value,
                               sInput->length,
                               sTrim->value,
                               sTrim->length )
                  != IDE_SUCCESS );

        if ( sResult->length == 0 )
        {
            aStack[0].column->module->null( aStack[0].column,
                                            aStack[0].value );
        }
        else
        {
            // nothing to do
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfLtrimCalculateNcharFor1Arg( mtcNode*     aNode,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      void*        aInfo,
                                      mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : Ltrim Calculate with 1 argument for NCHAR
 *
 * Implementation :
 *    LTRIM( char1 )
 *
 *    aStack[0] : 입력한 문자열의 왼쪽에 ' '을 모두 삭제한 값
 *    aStack[1] : char1 ( 입력 문자열 )
 *
 *    ex) LTRIM('   ab' ) ==> 'ab'
 *
 ***********************************************************************/
    
    const mtlModule * sSrcCharSet;
    mtdNcharType    * sResult;
    mtdNcharType    * sSource;
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
        sResult = (mtdNcharType*)aStack[0].value;
        sSource = (mtdNcharType*)aStack[1].value;

        sSrcCharSet = aStack[1].column->language;

        sResultMaxLen = sSrcCharSet->maxPrecision(aStack[0].column->precision);

        // ------------------------------
        // LeftTrim 공통 함수
        // ------------------------------
        
        if( sSrcCharSet->id == MTL_UTF16_ID )
        {
            IDE_TEST( mtfLeftTrimFor2ByteSpace( aStack[1].column->language,
                                                sResult->value,
                                                sResultMaxLen,
                                                &sResult->length,
                                                sSource->value,
                                                sSource->length )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( mtfLeftTrimFor1ByteSpace( aStack[1].column->language,
                                                sResult->value,
                                                sResultMaxLen,
                                                &sResult->length,
                                                sSource->value,
                                                sSource->length )
                      != IDE_SUCCESS );
        }

        if ( sResult->length == 0 )
        {
            aStack[0].column->module->null( aStack[0].column,
                                            aStack[0].value );
        }
        else
        {
            // nothing to do
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfLtrimCalculateNcharFor2Args( mtcNode*     aNode,
                                       mtcStack*    aStack,
                                       SInt         aRemain,
                                       void*        aInfo,
                                       mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : Ltrim Calculate with 2 arguments for NCHAR
 *
 * Implementation :
 *    LTRIM( char1, char2 )
 *
 *    aStack[0] : char1의 문자열을 왼쪽부터 char2의 문자와 비교하여
 *                같으면 삭제한다.
 *    aStack[1] : char1 ( 입력 문자열 )
 *    aStack[2] : char2 ( trim 대상 문자열 )
 *
 *    ex) LTRIM( 'abABa', 'ab' )  ==> 'ABa'
 *
 ***********************************************************************/
    
    mtdNcharType    * sResult;
    mtdNcharType    * sSource;
    mtdNcharType    * sTrim;
    const mtlModule * sSrcCharSet;
    UShort            sResultMaxLen;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    if( (aStack[1].column->module->isNull( aStack[1].column,
                                           aStack[1].value ) == ID_TRUE) ||
        (aStack[2].column->module->isNull( aStack[2].column,
                                           aStack[2].value ) == ID_TRUE) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sResult = (mtdNcharType*)aStack[0].value;
        sSource = (mtdNcharType*)aStack[1].value;
        sTrim   = (mtdNcharType*)aStack[2].value;

        sSrcCharSet = aStack[1].column->language;

        sResultMaxLen = sSrcCharSet->maxPrecision(aStack[0].column->precision);

        // ------------------------------
        // LeftTrim 공통 함수
        // ------------------------------

        IDE_TEST( mtfLeftTrim( aStack[1].column->language,
                               sResult->value,
                               sResultMaxLen,
                               &sResult->length,
                               sSource->value,
                               sSource->length,
                               sTrim->value,
                               sTrim->length )
                  != IDE_SUCCESS );

        if ( sResult->length == 0 )
        {
            aStack[0].column->module->null( aStack[0].column,
                                            aStack[0].value );
        }
        else
        {
            // nothing to do
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
