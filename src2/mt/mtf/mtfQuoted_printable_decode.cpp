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
 * $Id: mtfQuoted_printable_decode.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

/***********************************************************************
 * QUOTED_PRINTABLE_DECODE() : 
 * 입력받은 ARBYTE을 QOUTED_PRINTABLE 디코딩하여
 * VARBYTE타입의 문짜열로 반환한다.
 *
 * ex) SELECT QUOTED_PRINABLE_DECODE('65203D3344206D633220697320636F6F6C21') FROM DUAL;
 * QUOTED_PRINABLE_DECODE('e =3D mc2 is cool!') 
 * -----------------------------
 * 65203D206D633220697320636F6F6C21
 * 1 row selected.
 *
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtl.h>

UChar *hexDigitsUpper = (UChar*)"0123456789ABCDEF";
UChar *hexDigitsLower = (UChar*)"0123456789abcdef";

extern mtfModule mtfQuoted_printable_decode;

extern mtdModule mtdVarchar;
extern mtdModule mtdVarbyte;

static mtcName mtfQuoted_printable_decodeFunctionName[1] = {
    { NULL, 23, (void*)"QUOTED_PRINTABLE_DECODE" }
};

static IDE_RC mtfQuoted_printable_decodeEstimate( mtcNode     * aNode,
                                                  mtcTemplate * aTemplate,
                                                  mtcStack    * aStack,
                                                  SInt          aRemain,
                                                  mtcCallBack * aCallBack );

mtfModule mtfQuoted_printable_decode = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfQuoted_printable_decodeFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfQuoted_printable_decodeEstimate
};

static IDE_RC mtfQuoted_printable_decodeCalculate( mtcNode     * aNode,
                                                   mtcStack    * aStack,
                                                   SInt          aRemain,
                                                   void        * aInfo,
                                                   mtcTemplate * aTemplate );

const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfQuoted_printable_decodeCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfQuotedPrintableDecode( UChar  * aSrc,
                                 UChar  * aDst,
                                 UShort   aSrcLen,
                                 UShort * aWritten )
{
    UShort   sSrcIndex = 0;
    UShort   sDstIndex = 0;
    UChar  * sFristDigit = NULL;
    UChar  * sSecondDigit = NULL;
    
    IDE_TEST_RAISE( aSrc == NULL, ERR_UNEXPECTED );
    IDE_TEST_RAISE( aDst == NULL, ERR_UNEXPECTED );
    IDE_TEST_RAISE( aSrcLen == 0, ERR_UNEXPECTED );
    IDE_TEST_RAISE( aWritten == NULL, ERR_UNEXPECTED );

    for ( sSrcIndex = 0,  sDstIndex = 0;
          sSrcIndex < aSrcLen;
          sSrcIndex++, sDstIndex++ )
    {
        if ( ( aSrc[sSrcIndex] != '=' ) && ( aSrc[sSrcIndex] != '\n' ) )
        {
            aDst[sDstIndex] = aSrc[sSrcIndex];
        }
        else
        {
            if ( aSrc[sSrcIndex] == '=' )
            {
                if ( ( sSrcIndex + 2 ) < aSrcLen )
                {
                    if ( ( sFristDigit = (UChar*)idlOS::strchr( (SChar*)hexDigitsUpper,
                                                       aSrc[sSrcIndex + 1] ) ) &&
                         ( sSecondDigit = (UChar*)idlOS::strchr( (SChar*)hexDigitsUpper,
                                                                 aSrc[sSrcIndex + 2] ) ) )
                    {
                        aDst[sDstIndex] =
                            ( sFristDigit - hexDigitsUpper ) * 16 + ( sSecondDigit - hexDigitsUpper );
                        sSrcIndex += 2;
                    }
                    else if ( ( sFristDigit = (UChar*)idlOS::strchr( (SChar*)hexDigitsLower,
                                                            aSrc[sSrcIndex + 1] )) &&
                              ( sSecondDigit = (UChar*)idlOS::strchr( (SChar*)hexDigitsLower,
                                                            aSrc[sSrcIndex + 2] ) ) )
                    {
                        aDst[sDstIndex] =
                            ( sFristDigit - hexDigitsLower ) * 16 + ( sSecondDigit - hexDigitsLower );
                        sSrcIndex += 2;
                    }
                    else if ( aSrc[sSrcIndex + 1] == '\n' )
                    {
                        /* soft line break (=/n) */
                        aDst[sDstIndex] = aSrc[sSrcIndex + 2];
                        sSrcIndex += 2;
                    }
                    else if ( ( aSrc[sSrcIndex + 1] == ' ' ) || ( aSrc[sSrcIndex + 1] == '\t' ) )
                    {
                        /* garbage added in transit */
                        aDst[sDstIndex] = aSrc[sSrcIndex + 1];
                        sSrcIndex++;
                    }
                    else
                    {
                        IDE_RAISE( ERR_INVALID_ENCODED_STRING );
                    }
                }
                else
                {
                    aDst[sDstIndex] = aSrc[sSrcIndex];
                }
            }
            else if ( aSrc[sSrcIndex] == '\n' )
            {
                aDst[sDstIndex] = aSrc[sSrcIndex];
            }
            else
            {
                IDE_RAISE( ERR_UNEXPECTED );
            }
        }
    }

    *aWritten = sDstIndex;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNEXPECTED )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "QUOTED_PRINTABLE_DECODE",
                                  "Unexpected Error" ) );
    }
    IDE_EXCEPTION( ERR_INVALID_ENCODED_STRING )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_INVALID_ENCODED_STRING ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfQuoted_printable_decodeEstimate( mtcNode     * aNode,
                                           mtcTemplate * aTemplate,
                                           mtcStack    * aStack,
                                           SInt,
                                           mtcCallBack * aCallBack )
{
    const  mtdModule * sModules[1];
    SInt               sPrecision;
    
    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );
    
    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;
    
    sModules[0] = &mtdVarbyte;
    
    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    sPrecision = aStack[1].column->precision;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     &mtdVarbyte,
                                     1,
                                     sPrecision,
                                     0 )
              != IDE_SUCCESS );
        
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT )
    {
    IDE_SET( ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT ) );
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfQuoted_printable_decodeCalculate( mtcNode     * aNode,
                                            mtcStack    * aStack,
                                            SInt          aRemain,
                                            void        * aInfo,
                                            mtcTemplate * aTemplate )
{
    mtdByteType     * sResult = NULL;
    mtdByteType     * sSource = NULL;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );
    
    if ( aStack[1].column->module->isNull( aStack[1].column,
                                           aStack[1].value ) == ID_TRUE )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sResult = (mtdByteType*) aStack[0].value;
        sSource = (mtdByteType*) aStack[1].value;

        IDE_TEST( mtfQuotedPrintableDecode( sSource->value,
                                            sResult->value,
                                            sSource->length,
                                            &sResult->length )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
