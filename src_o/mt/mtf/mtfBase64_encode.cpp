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
 * $Id: mtfBase64_encode.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

/***********************************************************************
 * BASE64_ENCODE() : 
 * 입력받은 VARBYTE을 BASE64 인코딩하여
 * VARBYTE타입으로 반환한다.
 *
 * ex) SELECT BASE64_ENCODE('AA') FROM DUAL;
 * BASE64_ENCODE_STR('AA') 
 * -----------------------------
 * 71673D3D
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

extern mtfModule mtfBase64_encode;

extern mtdModule mtdVarbyte;

static mtcName mtfBase64EncodeFunctionName[1] = {
    { NULL, 13, (void*)"BASE64_ENCODE" }
};

/* Translation Table as described in RFC1113 */

static UChar mtfBase64EncodeTable[] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', /* 0x0~0x7*/
    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', /* 0x8~0x1F*/
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', /* 0x10~0x17*/
    'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', /* 0x18~0x1F*/
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', /* 0x20~0x27*/
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v', /* 0x28~0x2F*/
    'w', 'x', 'y', 'z', '0', '1', '2', '3', /* 0x30~0x37*/
    '4', '5', '6', '7', '8', '9', '+', '/'  /* 0x38~0x3F*/
};

static IDE_RC mtfBase64EncodeEstimate( mtcNode     * aNode,
                                       mtcTemplate * aTemplate,
                                       mtcStack    * aStack,
                                       SInt          aRemain,
                                       mtcCallBack * aCallBack );

mtfModule mtfBase64_encode = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfBase64EncodeFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfBase64EncodeEstimate
};

static IDE_RC mtfBase64EncodeCalculate( mtcNode     * aNode,
                                        mtcStack    * aStack,
                                        SInt          aRemain,
                                        void        * aInfo,
                                        mtcTemplate * aTemplate );

const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfBase64EncodeCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

UShort mtfBase64EncodeBlk2Varbyte( UChar * aIn,
                                   UChar * aOut,
                                   UShort  aInLen )
{
    UShort sWritten = 0;

    /* Encoding data size <= 3 */
    if ( aInLen > 3 )
    {
        // Nothing to do
    }
    else
    {
        /* aInt[0](bit 0~6) */
        aOut[0] = mtfBase64EncodeTable[ aIn[0] >> 2 ];
        /* aInt[0](bit 6~7) + aInt[1](4~7) */
        aOut[1] = mtfBase64EncodeTable[ ((aIn[0] & 0x03) << 4) |
                                        ((aIn[1] & 0xf0) >> 4) ];

        sWritten = 4;

        /* aInt[1](0~3) + aInt[2](6~7) */
        if (aInLen > 1)
        {
            aOut[2] = (UChar) ( mtfBase64EncodeTable[ ((aIn[1] & 0x0f) << 2) |
                                                      ((aIn[2] & 0xc0) >> 6) ] );
        }
        else
        {
            aOut[2] = '=';
        }

        /* aInt[2](0~5) */
        if (aInLen > 2)
        {
            aOut[3] = (UChar) ( mtfBase64EncodeTable[ aIn[2] & 0x3f ] );
        }
        else
        {
            aOut[3] = '=';
        }
    }

    return sWritten;
}

IDE_RC mtfBase64Encode2Varbyte( UChar  * aSrc,
                                UChar  * aDst, 
                                UShort   aSrcLen,
                                UShort   aDstLen,
                                UShort * aWritten )
{
    UShort i;
    UShort sWritten;
    UShort sSrcIndex;
    UShort sDstIndex;
    UChar  sEncBuf[3];
    UShort sEncLen;

    IDE_TEST_RAISE( aSrc == NULL, ERR_UNEXPECTED );
    IDE_TEST_RAISE( aDst == NULL, ERR_UNEXPECTED );
    IDE_TEST_RAISE( aSrcLen == 0, ERR_UNEXPECTED );
    IDE_TEST_RAISE( aDstLen == 0, ERR_UNEXPECTED );
    
    sDstIndex = 0;

    for ( sSrcIndex = 0; sSrcIndex < aSrcLen; sSrcIndex += 3 )
    {
        /* Encoding buffer must be set all-0,
         * because base64 encodes the last byte with next byte.
         * If the next byte is not zero,
         * the last byte will be encoded with non-zero byte,
         * and it results error value.
         */
        sEncBuf[0] = 0;
        sEncBuf[1] = 0;
        sEncBuf[2] = 0;

        /* encoding byte */
        if ( aSrcLen - sSrcIndex > 3 )
        {
            sEncLen = 3;
        }
        else
        {
            sEncLen = aSrcLen - sSrcIndex;
        }

        for ( i = 0; i < sEncLen; i++ )
        {
            sEncBuf[i] = aSrc[sSrcIndex + i];
        }

        IDE_TEST_RAISE( ( sDstIndex + 4 ) > aDstLen, ERR_UNEXPECTED_OVERFLOW );

        sWritten = mtfBase64EncodeBlk2Varbyte( sEncBuf,
                                               &aDst[sDstIndex],
                                               sEncLen );
        sDstIndex += sWritten;
    }
    
    *aWritten = sDstIndex;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNEXPECTED )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtfBase64Encode2Varbyte",
                                  "invalid src, dst value and length" ) );
    }
    IDE_EXCEPTION( ERR_UNEXPECTED_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "mtfBase64Encode2Varbyte",
                                  "dst precision overflow" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfBase64EncodeEstimate( mtcNode     * aNode,
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

    sPrecision = ( ( aStack[1].column->precision - 1 ) / 3 + 1 ) * 4;

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

IDE_RC mtfBase64EncodeCalculate( mtcNode     * aNode,
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

        IDE_TEST( mtfBase64Encode2Varbyte( sSource->value,
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
