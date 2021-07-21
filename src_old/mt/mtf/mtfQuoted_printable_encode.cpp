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
 * $Id: mtfQuoted_printable_encode.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

/***********************************************************************
 * QUOTED_PRINTABLE_ENCODE() : 
 * 입력받은 VARBYTE을 QOUTED_PRINTABLE 인코딩하여
 * VARBYTE타입의 문짜열로 반환한다.
 *
 * ex) SELECT QUOTED_PRINTABLE_ENCODE('65203D206D633220697320636F6F6C21')
 * FROM DUAL; -- e = mc2 is cool!'
 * QUOTED_PRINTABLE_ENCODE('65203D206D633220697320636F6F6C21') 
 * -----------------------------
 * 65203D3344206D633220697320636F6F6C21
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

UChar * hexDigitss  = (UChar*) "0123456789ABCDEF";

extern mtfModule mtfQuoted_printable_encode;

extern mtdModule mtdVarchar;
extern mtdModule mtdVarbyte;

static mtcName mtfQuoted_printable_encodeFunctionName[1] = {
    { NULL, 23, (void*)"QUOTED_PRINTABLE_ENCODE" }
};

static IDE_RC mtfQuoted_printable_encodeEstimate( mtcNode     * aNode,
                                                  mtcTemplate * aTemplate,
                                                  mtcStack    * aStack,
                                                  SInt          aRemain,
                                                  mtcCallBack * aCallBack );

mtfModule mtfQuoted_printable_encode = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfQuoted_printable_encodeFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfQuoted_printable_encodeEstimate
};

static IDE_RC mtfQuoted_printable_encodeCalculate( mtcNode     * aNode,
                                                   mtcStack    * aStack,
                                                   SInt          aRemain,
                                                   void*         aInfo,
                                                   mtcTemplate * aTemplate );

const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfQuoted_printable_encodeCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfQuotedPrintableEncode( UChar  * aSrc,
                                 UChar  * aDst,
                                 UShort   aSrcLen,
                                 UShort   aDstLen,
                                 UShort * aWritten )
{
    UShort sSrcIndex = 0;
    UShort sDstIndex = 0;
    UInt   sCol = 0;

    IDE_TEST_RAISE( aSrc == NULL, ERR_UNEXPECTED );
    IDE_TEST_RAISE( aDst == NULL, ERR_UNEXPECTED );
    IDE_TEST_RAISE( aSrcLen == 0, ERR_UNEXPECTED );
    IDE_TEST_RAISE( aDstLen == 0, ERR_UNEXPECTED );

    for ( sSrcIndex = 0, sDstIndex = 0;
          sSrcIndex < aSrcLen;
          sSrcIndex++, sDstIndex++ )
    {
        IDE_TEST_RAISE( sDstIndex > aDstLen, ERR_UNEXPECTED );

        if ( aSrc[sSrcIndex] == '\n' )
        {
            IDE_TEST_RAISE( ( sDstIndex + 2 ) > aDstLen, ERR_UNEXPECTED );

            aDst[sDstIndex] = '=';
            aDst[sDstIndex + 1] = hexDigitss[aSrc[sSrcIndex] >> 4];
            aDst[sDstIndex + 2] = hexDigitss[aSrc[sSrcIndex] & 0xf];
            sCol += 3;
            sDstIndex += 2;
        }
        else if ( aSrc[sSrcIndex] == ' ' )
        {
            if ( ( sSrcIndex + 1 ) < aSrcLen )
            {
                if ( aSrc[sSrcIndex + 1] != '\n' )
                {
                    aDst[sDstIndex] = aSrc[sSrcIndex];
                    sCol++;
                }
                else
                {
                    IDE_TEST_RAISE( ( sDstIndex + 2 ) > aDstLen, ERR_UNEXPECTED );

                    aDst[sDstIndex] = '=';
                    aDst[sDstIndex + 1] = hexDigitss[aSrc[sSrcIndex] >> 4];
                    aDst[sDstIndex + 2] = hexDigitss[aSrc[sSrcIndex] & 0xf];
                    sCol += 3;
                    sDstIndex += 2;
                }
            }
            else
            {
                aDst[sDstIndex] = aSrc[sSrcIndex];
                sCol++;
            }
        } /* these are for RFC 2047 Q encoding */
        else if ( ( aSrc[sSrcIndex] < 33 ) ||
                  ( aSrc[sSrcIndex] > 126 ) ||
                  ( aSrc[sSrcIndex] == '=' ) )
        {
            IDE_TEST_RAISE( ( sDstIndex + 2 ) > aDstLen, ERR_UNEXPECTED );
            
            aDst[sDstIndex] = '=';
            aDst[sDstIndex + 1] = hexDigitss[aSrc[sSrcIndex] >> 4];
            aDst[sDstIndex + 2] = hexDigitss[aSrc[sSrcIndex] & 0xf];
            sCol += 3;
            sDstIndex += 2;
        }
        else
        {
            aDst[sDstIndex] = aSrc[sSrcIndex];
            sCol++;
        }

        if ( sCol > 75 )
        {
            IDE_TEST_RAISE( ( sDstIndex + 1 ) > aDstLen, ERR_UNEXPECTED );
            
            /* soft line break (=/n) */
            aDst[sDstIndex] = '=';
            aDst[sDstIndex + 1] = '\n';
            sCol = 0;
            sDstIndex++;
            sSrcIndex--;
        }
        else
        {
            // nothing to do
        }
    }

    *aWritten = sDstIndex;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNEXPECTED )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "QUOTED_PRINTABLE_ENCODE",
                                  "Unexpected error") );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfQuoted_printable_encodeEstimate( mtcNode     * aNode,
                                           mtcTemplate * aTemplate,
                                           mtcStack    * aStack,
                                           SInt,
                                           mtcCallBack * aCallBack )
{
    const  mtdModule* sModules[1];
    SInt   sPrecision;
    
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
    
    sPrecision =
        ( aStack[1].column->precision * 3 ) + ( ( aStack[1].column->precision / 75 ) * 2 );

    if ( sPrecision > MTD_VARBYTE_PRECISION_MAXIMUM )
    {
        sPrecision = MTD_VARBYTE_PRECISION_MAXIMUM;
    }
    else
    {
        // nothing to do
    }

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     &mtdVarbyte,
                                     1,
                                     sPrecision,
                                     0 )
              != IDE_SUCCESS );    

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT )
    {
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfQuoted_printable_encodeCalculate( mtcNode     * aNode,
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

        IDE_TEST( mtfQuotedPrintableEncode( sSource->value,
                                            sResult->value,
                                            sSource->length,
                                            (UShort)aStack[0].column->precision,
                                            &sResult->length )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
