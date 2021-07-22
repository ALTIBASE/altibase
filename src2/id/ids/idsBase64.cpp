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
 * $Id: idsBase64.cpp
 **********************************************************************/

#include "idsBase64.h"

/* Translation Table as described in RFC1113 */
static const UChar base64Table[] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', /* 0x0~0x7*/
    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', /* 0x8~0x1F*/
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', /* 0x10~0x17*/
    'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', /* 0x18~0x1F*/
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', /* 0x20~0x27*/
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v', /* 0x28~0x2F*/
    'w', 'x', 'y', 'z', '0', '1', '2', '3', /* 0x30~0x37*/
    '4', '5', '6', '7', '8', '9', '+', '/'  /* 0x38~0x3F*/
};

/* encode 3 8-bit binary bytes as 4 '6-bit' characters */
void idsBase64::base64EncodeBlk( UChar * aIn, UChar * aOut, UInt aLen )
{
    aOut[0] = base64Table[ aIn[0] >> 2 ];
    aOut[1] = base64Table[ ((aIn[0] & 0x03) << 4) | ((aIn[1] & 0xf0) >> 4) ];
    aOut[2] = ( aLen > 1 ? base64Table[ ((aIn[1] & 0x0f) << 2) | ((aIn[2] & 0xc0) >> 6) ] : '=' );
    aOut[3] = ( aLen > 2 ? base64Table[ aIn[2] & 0x3f ] : '=' );
    aOut[4] = '\0';
}

/* base64 encode a stream, adding padding if needed */
IDE_RC idsBase64::base64Encode( UChar * aSrc, UInt aSrcLen, UChar ** aDst, UInt * aDstLen )
{
    UChar    sIn[3];            /* encoding이 되는 대상 */
    UChar    sOut[5];           /* encoding 결과*/
    UChar  * sDst;
    UInt     sDstLen     = 0;   /* 최종 result size */ 
    UInt     sSrcTextLen = 0;
    UInt     sIndex      = 0;
    UInt     sLen        = 0; 
    UInt     sBlockCnt   = 0; 

    IDE_DASSERT( aSrc != NULL );
    IDE_DASSERT( aSrcLen != 0 );
    IDE_DASSERT( aDst != NULL );

    sDst = (*aDst);
    idlOS::memset( sIn, 0x00, 3 );
    idlOS::memset( sOut, 0x00, 5 );

    while ( sSrcTextLen < aSrcLen )
    {
        sLen = 0;

        for ( sIndex = 0 ; sIndex < 3 ; sIndex++)
        {
            sIn[sIndex] = aSrc[sSrcTextLen];

            if ( sSrcTextLen != aSrcLen )
            {
                sLen++;
                sSrcTextLen++;
            }
            else
            {
                sIn[sIndex] = 0;
            }
        }

        if ( sLen > 0 )
        {
            base64EncodeBlk( sIn, sOut, sLen );
            idlOS::memcpy( sDst + sDstLen,
                           sOut,
                           idlOS::strlen((SChar*)sOut) );
            sDstLen += idlOS::strlen((SChar*)sOut);
            sBlockCnt++;
        }
        else
        {
            // Nothing to do.
        }
    }

    sDst[sDstLen] = '\0';
    (*aDstLen ) = sDstLen;

    return IDE_SUCCESS;
} 

void idsBase64::searchTable( UChar aValue, const UChar ** aValuePos )
{
    SInt          sIndex = 0;
    const UChar * sPos   = NULL;

    IDE_DASSERT( aValue != '=' );
    IDE_DASSERT( aValuePos != NULL );

    for( sIndex = 0 ; sIndex < 64 ; sIndex++ )
    {
        if ( aValue == base64Table[sIndex] )
        {
            sPos = &base64Table[sIndex];
            break;
        }
        else
        {
            sPos = NULL;
        }
    }

    (*aValuePos) = sPos;
}

/* decode 4 '6-bit' characters into 3 8-bit binary bytes */
void idsBase64::base64DecodeBlk( UChar * aIn, UChar * aOut )
{
    aOut[0] = (aIn[0] << 2) | (aIn[1] >> 4);
    aOut[1] = (aIn[1] << 4) | (aIn[2] >> 2);
    aOut[2] = (aIn[2] << 6) | (aIn[3] >> 0);
    aOut[3] = '\0';
}

IDE_RC idsBase64::base64Decode( UChar * aSrc, UInt aSrcLen, UChar ** aDst, UInt * aDstLen )
{

    UChar         sIn[4];               /* decoding이 되는 대상 */
    UChar         sOut[4];              /* decoding 결과*/
    UChar       * sDst;
    UInt          sDstLen     = 0;   /* 최종 result size */ 
    UInt          sSrcTextLen = 0;
    UInt          sIndex      = 0;
    UChar         sLetter;
    const UChar * sLetterPos;

    IDE_DASSERT( aSrc != NULL );
    IDE_DASSERT( aSrcLen != 0 );
    IDE_DASSERT( aDst != NULL );
    IDE_DASSERT( aDstLen != NULL );

    sDst = (*aDst);
    idlOS::memset( sIn, 0x00, 4 );
    idlOS::memset( sOut, 0x00, 4 );

    while ( sSrcTextLen < aSrcLen )
    {
        sLetter = aSrc[sSrcTextLen];

        if ( sLetter == '=')
        {
            base64DecodeBlk( sIn, sOut );
            idlOS::memcpy( sDst + sDstLen, sOut, 3 );
            sDstLen += 3;
            break;
        }
        else
        {
            // Nothing to do.
        }

        searchTable( sLetter, &sLetterPos );

        if ( sLetterPos != NULL )
        {
            sIn[sIndex] = sLetterPos - base64Table;
            sIndex = (sIndex + 1) % 4;

            if ( sIndex == 0 )
            {
                base64DecodeBlk( sIn, sOut );
                idlOS::memcpy( sDst + sDstLen, sOut, 3 );
                sDstLen += 3;

                /* sIn 초기화 */
                sIn[0] = sIn[1] = sIn[2] = sIn[3] = 0;
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            // Nothing to do.
        }

        sSrcTextLen++;
    }

    sDst[sDstLen] = '\0';

    (*aDstLen) = sDstLen;

    return IDE_SUCCESS;
} 
