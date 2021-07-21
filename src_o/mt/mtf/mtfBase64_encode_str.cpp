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
 * BASE64_ENCODE_STR() : 
 * 입력받은 Hexstring을 BASE64 인코딩하여
 * VARCHAR타입의 문자열을 반환한다.
 *
 * ex) SELECT BASE64_ENCODE_STR('AA') FROM DUAL;
 * BASE64_ENCODE_STR('AA') 
 * -----------------------------
 * qg==
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

/* 하위 4바이트만 사용하므로, 대소문자는 구분하지 않는다. */
#define HexToHalfByte(hex) \
    ( ( ('0' <= (hex) ) && ( (hex) <= '9') ) ?  \
      ( (hex) - '0' )                           \
      :                                         \
      ( ( (hex) - 'A' ) + 10 )                  \
    )

#define IsXDigit(hex) \
    ( ( ( ( '0' <= (hex) ) && ( (hex) <= '9' ) ) ||   \
        ( ( 'a' <= (hex) ) && ( (hex) <= 'f' ) ) ||   \
        ( ( 'A' <= (hex) ) && ( (hex) <= 'F' ) ) ) ?  \
      ID_TRUE : ID_FALSE )

extern mtfModule mtfBase64_encode_str;

extern mtdModule mtdVarchar;

static mtcName mtfBase64_encodeFunctionName[1] = {
    { NULL, 17, (void*)"BASE64_ENCODE_STR" }
};

/* Translation Table as described in RFC1113 */

static UChar mtfBase64Table[] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', /* 0x0~0x7*/
    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', /* 0x8~0x1F*/
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', /* 0x10~0x17*/
    'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', /* 0x18~0x1F*/
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', /* 0x20~0x27*/
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v', /* 0x28~0x2F*/
    'w', 'x', 'y', 'z', '0', '1', '2', '3', /* 0x30~0x37*/
    '4', '5', '6', '7', '8', '9', '+', '/'  /* 0x38~0x3F*/
};

static IDE_RC mtfBase64_encodeEstimate( mtcNode*     aNode,
                                        mtcTemplate* aTemplate,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        mtcCallBack* aCallBack );

mtfModule mtfBase64_encode_str = {
    2|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfBase64_encodeFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfBase64_encodeEstimate
};

static IDE_RC mtfBase64_encodeCalculate( mtcNode*     aNode,
                                         mtcStack*    aStack,
                                         SInt         aRemain,
                                         void*        aInfo,
                                         mtcTemplate* aTemplate );

const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfBase64_encodeCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfHexToByte( UChar* aHex, 
                     UChar* aByte )
{
    IDE_TEST_RAISE( IsXDigit( *aHex )     == ID_FALSE, ERR_INVALID_LITERAL );
    IDE_TEST_RAISE( IsXDigit( *(aHex+1) ) == ID_FALSE, ERR_INVALID_LITERAL );

    *aByte  = ( HexToHalfByte( *aHex ) << 4 ) & (0xF0) ;
    *aByte |= ( HexToHalfByte( *(aHex+1) ) ) & (0x0F) ;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_LITERAL )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_INVALID_LITERAL ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

UShort mtfBase64EncodeBlk( UChar* aIn, 
                           UChar* aOut, 
                           UShort aInLen )
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
        aOut[0] = mtfBase64Table[ aIn[0] >> 2 ];
        /* aInt[0](bit 6~7) + aInt[1](4~7) */
        aOut[1] = mtfBase64Table[ ((aIn[0] & 0x03) << 4) |
            ((aIn[1] & 0xf0) >> 4) ];

        sWritten = 4;

        /* aInt[1](0~3) + aInt[2](6~7) */
        if (aInLen > 1)
        {
            aOut[2] = (UChar) (mtfBase64Table[ ((aIn[1] & 0x0f) << 2) |
                                               ((aIn[2] & 0xc0) >> 6) ]);
        }
        else
        {
            aOut[2] = '=';
        }

        /* aInt[2](0~5) */
        if (aInLen > 2)
        {
            aOut[3] = (UChar) (mtfBase64Table[ aIn[2] & 0x3f ]);
        }
        else
        {
            aOut[3] = '=';
        }
    }

    return sWritten;
}

IDE_RC mtfBase64Encode( UChar*  aSrc, 
                        UChar*  aBuffer,
                        UChar*  aDst, 
                        UShort  aSrcLen, 
                        UShort* aWritten )
{
    UShort i;
    UShort sWritten;
    UShort sSrcIndex;
    UShort sDstIndex;
    UChar  sEncBuf[3];
    UShort sEncLen;
    UShort sRawLen = aSrcLen / 2;

    IDE_TEST_RAISE( aSrc     == NULL, ERR_UNEXPECTED );
    IDE_TEST_RAISE( aDst     == NULL, ERR_UNEXPECTED );
    IDE_TEST_RAISE( aSrcLen  == 0,    ERR_UNEXPECTED );
    IDE_TEST_RAISE( aWritten == NULL, ERR_UNEXPECTED );

    for( i = 0 ; i < sRawLen ; i++ )
    {
        IDE_TEST( mtfHexToByte( &aSrc[ 2 * i ], aBuffer + i ) 
                  != IDE_SUCCESS );
    }

    sDstIndex = 0;

    for (sSrcIndex = 0; sSrcIndex < sRawLen; sSrcIndex += 3)
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
        if ( sRawLen - sSrcIndex > 3 )
        {
            sEncLen = 3;
        }
        else
        {
            sEncLen = sRawLen - sSrcIndex;
        }

        for ( i = 0; i < sEncLen; i++ )
        {
            sEncBuf[i] = aBuffer[sSrcIndex + i];
        }

        sWritten = mtfBase64EncodeBlk( sEncBuf,
                                       &aDst[sDstIndex],
                                       sEncLen );
        sDstIndex += sWritten;
    }
    
    *aWritten = sDstIndex;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNEXPECTED )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "BASE64_ENCODE",
                                  "Unexpected error") );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfBase64_encodeEstimate( mtcNode*     aNode,
                                 mtcTemplate* aTemplate,
                                 mtcStack*    aStack,
                                 SInt,
                                 mtcCallBack* aCallBack )
{
    const mtdModule * sModules[1];
    SInt              sBufferSize;
    SInt              sPrecision;
    
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

    /* BUG-44791 BASE64_ENCODE_STR, BASE64_DECODE_STR 함수에서 잘못된 결과로 계산합니다. */
    sBufferSize = aStack[1].column->precision / 2;
    sPrecision  = ( ( sBufferSize - 1 ) / 3 + 1 ) * 4;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     &mtdVarchar,  // BUG-16501
                                     1,
                                     sPrecision,
                                     0 )
              != IDE_SUCCESS );

    IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                     &mtdVarchar, 
                                     1,
                                     sBufferSize, /* BUG-44791 */
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfBase64_encodeCalculate( mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*        aInfo,
                                  mtcTemplate* aTemplate )
{
    mtdCharType * sResult;
    mtdCharType * sSource;
    mtdCharType * sBuffer;
    mtcColumn   * sColumn;
    
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
        sResult   = (mtdCharType*) aStack[0].value;

        /* BUG-44791 BASE64_ENCODE_STR, BASE64_DECODE_STR 함수에서 잘못된 결과로 계산합니다. */
        sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
        sBuffer = (mtdCharType*)((UChar*)aTemplate->rows[aNode->table].row
                                 + sColumn[1].column.offset);

        sSource   = (mtdCharType*) aStack[1].value;

        IDE_TEST_RAISE( ( sSource->length % 2 ) == 1, ERR_INVALID_LENGTH );

        IDE_TEST( mtfBase64Encode( sSource->value,
                                   sBuffer->value,
                                   sResult->value,
                                   sSource->length,
                                   &sResult->length )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_LENGTH )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_INVALID_LITERAL ) );
    }
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
