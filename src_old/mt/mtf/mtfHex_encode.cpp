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
 * HEX_ENCODE() : 
 * 입력받은 hex string을 인코딩하여
 * VARCHAR타입의 문자열을 반환한다.
 *
 * ex) SELECT HEX_ENCODE('altibase') FROM DUAL;
 * HEX_ENCODE('altibase')
 * -----------------------------
 * 616C746962617365
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

extern mtfModule mtfHex_encode;

extern mtdModule mtdVarchar;

static mtcName mtfHex_encodeFunctionName[1] = {
    { NULL, 10, (void*)"HEX_ENCODE" }
};

static IDE_RC mtfHex_encodeEstimate( mtcNode*     aNode,
                                     mtcTemplate* aTemplate,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     mtcCallBack* aCallBack );

mtfModule mtfHex_encode = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfHex_encodeFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfHex_encodeEstimate
};

static IDE_RC mtfHex_encodeCalculate( mtcNode*     aNode,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      void*        aInfo,
                                      mtcTemplate* aTemplate );

const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfHex_encodeCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfHex_encodeEstimate( mtcNode*     aNode,
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

    sPrecision = aStack[1].column->precision * 2;
    
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

IDE_RC mtfAscii2Hex( UChar aIn, UChar *aOut )
{
    if ( aIn < 10 )
    {
        *aOut = '0' + aIn;
    }
    else
    {
        *aOut = 'A' + ( aIn - 10 );
    }
    
    return IDE_SUCCESS;
}

IDE_RC mtfHex_encodeCalculate( mtcNode*     aNode,
                               mtcStack*    aStack,
                               SInt         aRemain,
                               void*        aInfo,
                               mtcTemplate* aTemplate )
{
    mtdCharType * sResult;
    mtdCharType * sSource;
    UChar         sHigh;
    UChar         sLow;
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

        for ( i = 0, j = 0; i < sSource->length; i++ )
        {
            IDE_TEST( mtfAscii2Hex( ((sSource->value[i] & 0xF0) >> 4), &sHigh )
                      != IDE_SUCCESS );
            IDE_TEST( mtfAscii2Hex( (sSource->value[i] & 0x0F), &sLow )
                      != IDE_SUCCESS );
            
            sResult->value[j] = sHigh;
            j++;
            sResult->value[j] = sLow;
            j++;
        }
        
        sResult->length = sSource->length + sSource->length;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
