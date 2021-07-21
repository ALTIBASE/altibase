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
 * BASE64_DECODE_STR() : 
 * 입력받은 문자열을 BASE64 디코딩하여
 * VARCHAR타입의 Hexstring을 반환한다.
 *
 * ex) SELECT BASE64_DECODE_STR('qg==') FROM DUAL;
 * BASE64_DECODE_STR('qg==') 
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

#define HalfByteToHex(halfByte)                         \
    ( (  (0 <= (halfByte)) && ((halfByte) <= 9) ) ?     \
      ( (halfByte) + '0' )                              \
      :                                                 \
      ( ((halfByte) - 10) + 'A' )                       \
        )

#define MTF_IS_NOT_IN_TABLE ( (UChar) (0xFF) )
#define MTF_IS_PADDING_CHAR ( (UChar) (0xFE) )

extern mtfModule mtfBase64_decode_str;

extern mtdModule mtdVarchar;

static mtcName mtfBase64_decodeFunctionName[1] = {
    { NULL, 17, (void*)"BASE64_DECODE_STR" }
};

/* Translation Table as described in RFC1113 
 * (defined in mtfBase64_encode) */
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

static IDE_RC mtfBase64_decodeEstimate( mtcNode*     aNode,
                                        mtcTemplate* aTemplate,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        mtcCallBack* aCallBack );

mtfModule mtfBase64_decode_str = {
    2|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfBase64_decodeFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfBase64_decodeEstimate
};

static IDE_RC mtfBase64_decodeCalculate( mtcNode*     aNode,
                                         mtcStack*    aStack,
                                         SInt         aRemain,
                                         void*        aInfo,
                                         mtcTemplate* aTemplate );

const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfBase64_decodeCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

void mtfByteToHex( UChar  aByte,
                   UChar* aHex   )
{
    *aHex++ = HalfByteToHex( ((SChar)aByte >> 4) & (0x0F) ) ;
    *aHex = HalfByteToHex( aByte & (0x0F) );
}

UChar mtfSearchTable( UChar aValue )
{
    UChar i;
    UChar sRet = MTF_IS_NOT_IN_TABLE;

    if ( aValue == '=' )
    {
        sRet = MTF_IS_PADDING_CHAR;
    }
    else
    {
        for( i = 0 ; i < 64 ; i++ )
        {
            if ( aValue == mtfBase64Table[i] )
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

IDE_RC mtfBase64DecodeBlk( UChar*  aIn, 
                           UChar*  aOut,
                           UShort* aLength ) 
{
    /* decoding message always is longer than 1 */
    UShort sWritten = 1;

    IDE_TEST_RAISE( ( aIn[0] == MTF_IS_NOT_IN_TABLE ) ||
                    ( aIn[1] == MTF_IS_NOT_IN_TABLE ) ||
                    ( aIn[2] == MTF_IS_NOT_IN_TABLE ) ||
                    ( aIn[3] == MTF_IS_NOT_IN_TABLE ) ||
                    ( aIn[0] == MTF_IS_PADDING_CHAR ) ||
                    ( aIn[1] == MTF_IS_PADDING_CHAR ),
                    ERR_INVALID_LITERAL );

    aOut[ 0 ] = (UChar) (aIn[0] << 2 | aIn[1] >> 4);

    if ( aIn[2] != MTF_IS_PADDING_CHAR )
    {
        aOut[ 1 ] = (UChar) (aIn[1] << 4 | aIn[2] >> 2);
        sWritten++;
    
        if ( aIn[3] != MTF_IS_PADDING_CHAR )
        {
            aOut[ 2 ] = (UChar) (((aIn[2] << 6) & 0xc0) | aIn[3]);
            sWritten++;         
        }
        else
        {
            aOut[2] =  (UChar) ((aIn[2] << 6) & 0xc0);
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

IDE_RC mtfBase64Decode( UChar*  aSrc,
                        UChar*  aBuffer,
                        UChar*  aDst,
                        UShort  aSrcLen,
                        UShort* aWritten )
{
    UShort i;
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

        sIn[0] = mtfSearchTable( aSrc[ sSrcIndex + 0 ] );
        sIn[1] = mtfSearchTable( aSrc[ sSrcIndex + 1 ] );
        sIn[2] = mtfSearchTable( aSrc[ sSrcIndex + 2 ] );
        sIn[3] = mtfSearchTable( aSrc[ sSrcIndex + 3 ] );

        IDE_TEST( mtfBase64DecodeBlk(sIn, aBuffer + sRawIndex, &sDecodeLength )
                  != IDE_SUCCESS );

        sRawIndex += sDecodeLength;
    }

    for ( i = 0 ; i < sRawIndex ; i++ )
    {
        mtfByteToHex( aBuffer[i], &aDst[ i * 2 ] );
    }

    *aWritten = sRawIndex * 2;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_LITERAL )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_INVALID_LITERAL ) );
    }
    IDE_EXCEPTION( ERR_UNEXPECTED )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_UNEXPECTED_ERROR,
                                  "BASE64_DECODE",
                                  "Unexpected Error" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfBase64_decodeEstimate( mtcNode*     aNode,
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
    sBufferSize = aStack[1].column->precision;
    sPrecision  = ( ( sBufferSize - 1 ) / 4 + 1 ) * 3 * 2;

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

IDE_RC mtfBase64_decodeCalculate( mtcNode*     aNode,
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

        IDE_TEST_RAISE( ( sSource->length % 4 ) != 0, ERR_INVALID_LENGTH );

        IDE_TEST( mtfBase64Decode( sSource->value,
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
