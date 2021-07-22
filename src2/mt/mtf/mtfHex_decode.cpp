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
 * $Id$
 **********************************************************************/

/***********************************************************************
 * HEX_DECODE() : 
 * 입력받은 hex string을 디코딩하여
 * VARCHAR타입의 문자열을 반환한다.
 *
 * ex) SELECT HEX_DECODE('616C746962617365') FROM DUAL;
 * HEX_DECODE('616C746962617365')
 * -----------------------------
 * altibase
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

extern mtfModule mtfHex_decode;

extern mtdModule mtdVarchar;

static mtcName mtfHex_decodeFunctionName[1] = {
    { NULL, 10, (void*)"HEX_DECODE" }
};

static IDE_RC mtfHex_decodeEstimate( mtcNode*     aNode,
                                     mtcTemplate* aTemplate,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     mtcCallBack* aCallBack );

mtfModule mtfHex_decode = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfHex_decodeFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfHex_decodeEstimate
};

static IDE_RC mtfHex_decodeCalculate( mtcNode*     aNode,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      void*        aInfo,
                                      mtcTemplate* aTemplate );

const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfHex_decodeCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfHex_decodeEstimate( mtcNode*     aNode,
                              mtcTemplate* aTemplate,
                              mtcStack*    aStack,
                              SInt,
                              mtcCallBack* aCallBack )
{
    const  mtdModule* sModules[1];
    SInt   sPrecision;
    
    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );
    
    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;
    
    sModules[0] = &mtdVarchar;
    
    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );


    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    sPrecision = ( aStack[1].column->precision / 2 ) + ( aStack[1].column->precision % 2 );
    
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     &mtdVarchar,  // BUG-16501
                                     1,
                                     sPrecision,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfHex_decodeCalculate( mtcNode*     aNode,
                               mtcStack*    aStack,
                               SInt         aRemain,
                               void*        aInfo,
                               mtcTemplate* aTemplate )
{
    mtdCharType * sResult;
    mtdCharType * sSource;
    UChar         sHigh;
    UChar         sLow;
    UShort        sSrcLen;
    UShort        sRstLen = 0;
    SInt          i;
    SInt          j;
    
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
        sResult = (mtdCharType*) aStack[0].value;
        sSource = (mtdCharType*) aStack[1].value;

        sSrcLen = ( sSource->length / 2 ) * 2;

        for ( i = 0, j = 0; i < sSrcLen; i+= 2, j++ )
        {
            IDE_TEST( mtf::hex2Ascii( sSource->value[i], &sHigh )
                      != IDE_SUCCESS );
            IDE_TEST( mtf::hex2Ascii( sSource->value[i+1], &sLow )
                      != IDE_SUCCESS );
            
            sResult->value[j] = (sHigh << 4) | sLow;
            sRstLen++;
        }
        
        if ( sSource->length % 2 == 1 )
        {
            IDE_TEST( mtf::hex2Ascii( sSource->value[i], &sHigh )
                      != IDE_SUCCESS );
            
            sResult->value[j] = sHigh << 4;
            sRstLen++;
        }
        else
        {
            // Nothing to do.
        }

        sResult->length = sRstLen;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
