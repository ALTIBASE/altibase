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
 * $Id: mtfTranslate.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtl.h>

extern mtfModule mtfTranslate;


static mtcName mtfTranslateFunctionName[1] = {
    { NULL, 9, (void*)"TRANSLATE"    }
};

static IDE_RC mtfTranslateEstimate( mtcNode*     aNode,
                                    mtcTemplate* aTemplate,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    mtcCallBack* aCallBack );

mtfModule mtfTranslate = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfTranslateFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfTranslateEstimate
};

static IDE_RC mtfTranslateCalculate( mtcNode*     aNode,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     void*        aInfo,
                                     mtcTemplate* aTemplate );

static IDE_RC mtfTranslateCalculateNchar( mtcNode*     aNode,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     void*        aInfo,
                                     mtcTemplate* aTemplate );

const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfTranslateCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

const mtcExecute mtfExecuteNchar = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfTranslateCalculateNchar,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

/*-----------------------------------------------------------------------------
 * Name :
 *    IDE_RC mtfTranslateEstimate()
 *
 * Argument :
 *    aNode - 입력정보
 *    aStack - 입력값
 *
 * Description :
 *    1. 아규먼트 입력개수가 3개인지 , 아니면 에러!
 *    2. *sModules[0]->language->translate 를 실행
 *    3. precision 계산
 * ---------------------------------------------------------------------------*/
IDE_RC mtfTranslateEstimate( mtcNode*     aNode,
                             mtcTemplate* aTemplate,
                             mtcStack*    aStack,
                             SInt      /* aRemain */,
                             mtcCallBack* aCallBack )
{
    const mtdModule* sModules[3];
    SInt             sPrecision;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 3,
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
        aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecuteNchar;

        if( aStack[1].column->language->id == MTL_UTF16_ID )
        {
            sPrecision = IDL_MIN( aStack[1].column->precision * 2,
                                  (SInt)MTD_UTF16_NCHAR_PRECISION_MAXIMUM );
        }
        else
        {
            sPrecision = IDL_MIN( aStack[1].column->precision * 2,
                                  (SInt)MTD_UTF8_NCHAR_PRECISION_MAXIMUM );
        }
    }
    else
    {
        aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

        sPrecision = IDL_MIN( aStack[1].column->precision * 2,
                              MTD_CHAR_PRECISION_MAXIMUM );
    }


    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     sModules[0],
                                     1,
                                     sPrecision,
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

IDE_RC mtfTranslateCommon( const mtlModule * aLanguage,
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
 * Description : Translate Calculate
 *
 * Implementation :
 *    TRANSLATE( char, string1, string2 )
 *   
 *    aStack[0] : string1의 각 문자에 대응하는 string2의 각 문자로 교체
 *                된 char
 *    aStack[1] : string1
 *    aStack[2] : string2
 *
 *    ex) SELECT TRANSLATE( gname, 'M','L' ) FROM goods;
 *        --> TM-U200 ==> TL-U200
 *
 ***********************************************************************/
    
    UChar           * sSourceIndex;
    UChar           * sSourceFence;
    UChar           * sSourceIndexPrev;
    UChar           * sFromIndex;
    UChar           * sFromFence;
    UChar           * sToIndex;
    UChar           * sToIndexPrev;
    UChar           * sToFence;
    UShort            sFromStart;
    UShort            sToStart;
    UShort            sResultIndex = 0;
    UShort            sSize;
    idBool            sFound;
    idBool            sIsSame = ID_FALSE;
    UChar             sSourceSize;
    UChar             sFromSize;
    
    sSourceIndex = aSource;
    sSourceFence = sSourceIndex + aSourceLen;

    while ( sSourceIndex < sSourceFence )
    {
        //---------------------------------
        // Source에서 from string과 동일한 문자 찾기
        //---------------------------------
        
        sSourceSize =  mtl::getOneCharSize( sSourceIndex,
                                            sSourceFence,
                                            aLanguage ); 
        
        sFromIndex = aFrom;
        sFromFence = sFromIndex + aFromLen;
        sFromStart = 0;
        
        while ( sFromIndex < sFromFence )
        {
            sFromSize =  mtl::getOneCharSize( sFromIndex,
                                              sFromFence,
                                              aLanguage );

            sIsSame = mtc::compareOneChar( sSourceIndex,
                                           sSourceSize,
                                           sFromIndex,
                                           sFromSize );

            if ( sIsSame == ID_TRUE )
            {
                break;
            }
            else
            {
                // 다음 From 문자로 진행
                (void)aLanguage->nextCharPtr( &sFromIndex, sFromFence );
        
                sFromStart++;
            }
        }

        //---------------------------------
        // - 동일 문자 없는 경우: 기존 문자를 결과에 copy
        // - 동일 문자 있는 경우: From 문자 대신 To 문자를 결과에 copy 
        //---------------------------------

        sSourceIndexPrev = sSourceIndex;
        
        // 다음 source 문자로 진행
        (void)aLanguage->nextCharPtr( & sSourceIndex, sSourceFence );
        
        if ( sIsSame == ID_FALSE )
        {
            sSize = sSourceIndex - sSourceIndexPrev;
            
            IDE_TEST_RAISE( sResultIndex + sSize > aResultMaxLen,
                            ERR_EXCEED_MAX );
            
            idlOS::memcpy( aResult + sResultIndex,
                           sSourceIndexPrev,
                           sSize );

            sResultIndex += sSize;
        }
        else
        {
            //---------------------------------
            // 대응되는 To 문자 문자 찾기
            //---------------------------------
            
            sToIndex = aTo;
            sToFence = sToIndex + aToLen;
            sToStart = 0;
            sFound = ID_FALSE;

            while ( sToIndex < sToFence )
            {
                sToIndexPrev = sToIndex;
            
                (void) aLanguage->nextCharPtr( &sToIndex, sToFence );
                
                if ( sToStart == sFromStart )
                {
                    sFound = ID_TRUE;
                    break;
                }
                else
                {
                    sToStart++;
                }
            }
            
            if ( sFound == ID_TRUE )
            {
                sSize = sToIndex - sToIndexPrev;
                
                IDE_TEST_RAISE( sResultIndex + sSize > aResultMaxLen,
                                ERR_EXCEED_MAX );
                
                idlOS::memcpy( aResult + sResultIndex,
                               sToIndexPrev,
                               sSize );
                sResultIndex += sSize;
            }
            else
            {
                // 동일 문자 찾았지만, 대응되는 To 문자가 없는 경우
            }
        }
    }

    *aResultLen = sResultIndex;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXCEED_MAX );
    {        
        IDE_SET(ideSetErrorCode( idERR_ABORT_idnReachEnd ));
    }    

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfTranslateCalculate( mtcNode*     aNode,
                              mtcStack*    aStack,
                              SInt         aRemain,
                              void*        aInfo,
                              mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : Translate Calculate
 *
 * Implementation :
 *    TRANSLATE( char, string1, string2 )
 *   
 *    aStack[0] : string1의 각 문자에 대응하는 string2의 각 문자로 교체
 *                된 char
 *    aStack[1] : string1
 *    aStack[2] : string2
 *
 *    ex) SELECT TRANSLATE( gname, 'M','L' ) FROM goods;
 *        --> TM-U200 ==> TL-U200
 *
 ***********************************************************************/
    
    mtdCharType     * sResult;
    mtdCharType     * sSource;
    mtdCharType     * sFrom;
    mtdCharType     * sTo;
    //const mtlModule * sLanguage;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );
    
    if( (aStack[1].column->module->isNull( aStack[1].column,
                                           aStack[1].value ) == ID_TRUE) ||
        (aStack[2].column->module->isNull( aStack[2].column,
                                           aStack[2].value ) == ID_TRUE) ||
        (aStack[3].column->module->isNull( aStack[3].column,
                                           aStack[3].value ) == ID_TRUE) )
    {   
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sResult = (mtdCharType*)aStack[0].value;
        sSource = (mtdCharType*)aStack[1].value;
        sFrom   = (mtdCharType*)aStack[2].value;
        sTo     = (mtdCharType*)aStack[3].value;

        //sLanguage = aStack[1].column->language;

        IDE_TEST( mtfTranslateCommon( aStack[1].column->language,
                                      sResult->value,
                                      aStack[0].column->precision,
                                      &sResult->length,
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

IDE_RC mtfTranslateCalculateNchar( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : Translate Calculate for NCHAR
 *
 * Implementation :
 *    TRANSLATE( char, string1, string2 )
 *   
 *    aStack[0] : string1의 각 문자에 대응하는 string2의 각 문자로 교체
 *                된 char
 *    aStack[1] : string1
 *    aStack[2] : string2
 *
 *    ex) SELECT TRANSLATE( gname, 'M','L' ) FROM goods;
 *        --> TM-U200 ==> TL-U200
 *
 ***********************************************************************/
    
    mtdCharType     * sResult;
    mtdCharType     * sSource;
    mtdCharType     * sFrom;
    mtdCharType     * sTo;
    const mtlModule * sLanguage;
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
                                           aStack[2].value ) == ID_TRUE) ||
        (aStack[3].column->module->isNull( aStack[3].column,
                                           aStack[3].value ) == ID_TRUE) )
    {   
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sResult = (mtdCharType*)aStack[0].value;
        sSource = (mtdCharType*)aStack[1].value;
        sFrom   = (mtdCharType*)aStack[2].value;
        sTo     = (mtdCharType*)aStack[3].value;

        sLanguage = aStack[1].column->language;

        sResultMaxLen = sLanguage->maxPrecision(aStack[0].column->precision);

        IDE_TEST( mtfTranslateCommon( aStack[1].column->language,
                                      sResult->value,
                                      sResultMaxLen,
                                      &sResult->length,
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
