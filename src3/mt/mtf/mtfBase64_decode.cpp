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
 * $Id: mtfBase64_decode.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

/***********************************************************************
 * BASE64_DECODE() : 
 * 입력받은 VARBYTE을 BASE64 디코딩하여
 * VARBYTE타입으로 반환한다.
 *
 * ex) SELECT BASE64_DECODE('71673D3D') FROM DUAL;
 * BASE64_DECODE('71673D3D') 
 * -----------------------------
 * AA
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

#define MTF_IS_NOT_IN_TABLE_VARBYTE ( (UChar) (0xFF) )
#define MTF_IS_PADDING_CHAR_VARBYTE ( (UChar) (0xFE) )

extern mtfModule mtfBase64_decode;

extern mtdModule mtdVarbyte;

static mtcName mtfBase64DecodeFunctionName[1] = {
    { NULL, 13, (void*)"BASE64_DECODE" }
};

/* Translation Table as described in RFC1113 
 * (defined in mtfBase64_encode) */
static UChar mtfBase64DecodeTable[] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', /* 0x0~0x7*/
    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', /* 0x8~0x1F*/
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', /* 0x10~0x17*/
    'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', /* 0x18~0x1F*/
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', /* 0x20~0x27*/
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v', /* 0x28~0x2F*/
    'w', 'x', 'y', 'z', '0', '1', '2', '3', /* 0x30~0x37*/
    '4', '5', '6', '7', '8', '9', '+', '/'  /* 0x38~0x3F*/
};

static IDE_RC mtfBase64DecodeEstimate( mtcNode     * aNode,
                                       mtcTemplate * aTemplate,
                                       mtcStack    * aStack,
                                       SInt          aRemain,
                                       mtcCallBack * aCallBack );

mtfModule mtfBase64_decode = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfBase64DecodeFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfBase64DecodeEstimate
};

static IDE_RC mtfBase64DecodeCalculate( mtcNode     * aNode,
                                        mtcStack    * aStack,
                                        SInt          aRemain,
                                        void        * aInfo,
                                        mtcTemplate * aTemplate );

const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfBase64DecodeCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

UChar mtfSearchDecodeTable( UChar aValue )
{
    UChar i;
    UChar sRet = MTF_IS_NOT_IN_TABLE_VARBYTE;

    if ( aValue == '=' )
    {
        sRet = MTF_IS_PADDING_CHAR_VARBYTE;
    }
    else
    {
        for ( i = 0 ; i < 64 ; i++ )
        {
            if ( aValue == mtfBase64DecodeTable[i] )
            {
                sRet = i;
            }
            else
            {
                // Nothing to do
            }
        }
    }

    return sRet;
}

IDE_RC mtfBase64DecodeBlk2Varbyte( UChar  * aIn, 
                                   UChar  * aOut,
                                   UShort * aLength ) 
{
    /* decoding message always is longer than 1 */
    UShort sWritten = 1;

    IDE_TEST_RAISE( ( aIn[0] == MTF_IS_NOT_IN_TABLE_VARBYTE ) ||
                    ( aIn[1] == MTF_IS_NOT_IN_TABLE_VARBYTE ) ||
                    ( aIn[2] == MTF_IS_NOT_IN_TABLE_VARBYTE ) ||
                    ( aIn[3] == MTF_IS_NOT_IN_TABLE_VARBYTE ) ||
                    ( aIn[0] == MTF_IS_PADDING_CHAR_VARBYTE ) ||
                    ( aIn[1] == MTF_IS_PADDING_CHAR_VARBYTE ),
                    ERR_INVALID_LITERAL );

    aOut[ 0 ] = (UChar) (aIn[0] << 2 | aIn[1] >> 4);

    if ( aIn[2] != MTF_IS_PADDING_CHAR_VARBYTE )
    {
        aOut[ 1 ] = (UChar) (aIn[1] << 4 | aIn[2] >> 2);
        sWritten++;
    
        if ( aIn[3] != MTF_IS_PADDING_CHAR_VARBYTE )
        {
            aOut[ 2 ] = (UChar) (((aIn[2] << 6) & 0xc0) | aIn[3]);
            sWritten++;         
        }
        else
        {
            aOut[2] = (UChar) ((aIn[2] << 6) & 0xc0);
        }
    }
    else
    {
        aOut[1] = (UChar) (aIn[1] << 4);
    }
    
    *aLength = sWritten;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_LITERAL )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_INVALID_LITERAL ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfBase64Decode2Varbyte( UChar  * aSrc,
                                UChar  * aDst,
                                UShort   aSrcLen,
                                UShort * aWritten )
{
    UShort sSrcIndex     = 0;
    UShort sRawIndex     = 0;
    UShort sDecodeLength = 0;
    UChar  sIn[4];

    IDE_TEST_RAISE( aSrc     == NULL, ERR_UNEXPECTED );
    IDE_TEST_RAISE( aDst     == NULL, ERR_UNEXPECTED );
    IDE_TEST_RAISE( aSrcLen  == 0,    ERR_UNEXPECTED );
    IDE_TEST_RAISE( aWritten == NULL, ERR_UNEXPECTED );

    for ( sSrcIndex = 0;
          sSrcIndex < aSrcLen;
          sSrcIndex += 4 )
    {
        IDE_TEST_RAISE( ( sDecodeLength % 3 != 0 ) ||
                        ( ( sSrcIndex + 3 ) >= aSrcLen ), ERR_INVALID_LITERAL );

        sIn[0] = mtfSearchDecodeTable( aSrc[ sSrcIndex + 0 ] );
        sIn[1] = mtfSearchDecodeTable( aSrc[ sSrcIndex + 1 ] );
        sIn[2] = mtfSearchDecodeTable( aSrc[ sSrcIndex + 2 ] );
        sIn[3] = mtfSearchDecodeTable( aSrc[ sSrcIndex + 3 ] );

        IDE_TEST( mtfBase64DecodeBlk2Varbyte( sIn, aDst + sRawIndex, &sDecodeLength )
                  != IDE_SUCCESS );

        sRawIndex += sDecodeLength;
    }

    *aWritten = sRawIndex;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_LITERAL )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_INVALID_LITERAL ) );
    }
    IDE_EXCEPTION( ERR_UNEXPECTED )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtfBase64Decode2Varbyte",
                                  "invalid src, dst value and length" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfBase64DecodeEstimate( mtcNode     * aNode,
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
        IDE_SET(ideSetErrorCode( mtERR_ABORT_INVALID_FUNCTION_ARGUMENT ) );
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfBase64DecodeCalculate( mtcNode     * aNode,
                                 mtcStack    * aStack,
                                 SInt          aRemain,
                                 void        * aInfo,
                                 mtcTemplate * aTemplate )
{
    mtdByteType * sResult = NULL;
    mtdByteType * sSource = NULL;
    
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

        IDE_TEST( mtfBase64Decode2Varbyte( sSource->value,
                                           sResult->value,
                                           sSource->length,
                                           &sResult->length )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
