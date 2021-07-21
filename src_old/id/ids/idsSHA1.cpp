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
 
#include <ide.h>
#include <idsSHA1.h>

#define circularShift(bits,word) \
                ((((word) << (bits)) & 0xFFFFFFFF) | \
                ((word) >> (32-(bits))))

#define ASCII2HEX(c)     (((c) < 10) ? ('0' + (c)) : ('A' + ((c) - 10)))

IDE_RC idsSHA1::digestToHexCode( UChar       *aByte,
                                 const UChar *aMessage,
                                 UInt         aMessageLen )
{
    UChar sBlock[64] = {0, };
    UInt  sDigest[5];
    UInt  sIndex = 0;
    UInt  sMessageLenBitLow = 0;
    UInt  sMessageLenBitHigh = 0;

    sDigest[0]      = 0x67452301;
    sDigest[1]      = 0xEFCDAB89;
    sDigest[2]      = 0x98BADCFE;
    sDigest[3]      = 0x10325476;
    sDigest[4]      = 0xC3D2E1F0;

    while( aMessageLen-- )
    {
        sBlock[sIndex++] = *aMessage & 0xFF;

        sMessageLenBitLow += 8;
        sMessageLenBitLow &= 0xFFFFFFFF;

        if( sMessageLenBitLow == 0 )
        {
            sMessageLenBitHigh++;
            sMessageLenBitHigh &= 0xFFFFFFFF;

            IDE_TEST_RAISE( sMessageLenBitHigh == 0, message_overflow );
        }

        if( sIndex == 64 )
        {
            processMessageBlock( sDigest, sBlock );
            sIndex = 0;
        }

        aMessage++;
    }
    padMessage( sDigest, sBlock, sIndex, sMessageLenBitLow, sMessageLenBitHigh);

    makeHexCode( aByte, sDigest );

    return IDE_SUCCESS;

    IDE_EXCEPTION( message_overflow );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_MESSAGE_OVERFLOW));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC idsSHA1::digest( UChar       *aResult,
                        const UChar *aMessage,
                        UInt         aMessageLen )
{
    UChar sByte[20];

    IDE_TEST( digestToHexCode( sByte,
                               aMessage,
                               aMessageLen )
              != IDE_SUCCESS );
        
    convertHexToString( aResult, sByte );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void idsSHA1::processMessageBlock( UInt *aDigest, UChar *aBlock )
{
    const UInt K[] =            /* Constants defined in SHA-1   */      
    {
        0x5A827999,
        0x6ED9EBA1,
        0x8F1BBCDC,
        0xCA62C1D6
    };
    SInt    i;
    UInt    temp;               /* Temporary word value         */
    UInt    W[80];              /* Word sequence                */
    UInt    A, B, C, D, E;      /* Word buffers                 */

    /*
     *  Initialize the first 16 words in the array W
     */
    for( i = 0; i < 16; i++ )
    {
        W[i]  = ( (UInt)aBlock[i * 4] )     << 24;
        W[i] |= ( (UInt)aBlock[i * 4 + 1] ) << 16;
        W[i] |= ( (UInt)aBlock[i * 4 + 2] ) << 8;
        W[i] |= ( (UInt)aBlock[i * 4 + 3] );
    }

    for( i = 16; i < 80; i++ )
    {
       W[i] = circularShift(1, W[i-3] ^ W[i-8] ^ W[i-14] ^ W[i-16]);
    }

    A = aDigest[0];
    B = aDigest[1];
    C = aDigest[2];
    D = aDigest[3];
    E = aDigest[4];

    for( i = 0; i < 20; i++ )
    {
        temp =  circularShift(5, A) + ((B & C) | ((~B) & D)) + E + W[i] + K[0];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = circularShift(30, B);
        B = A;
        A = temp;
    }

    for( i = 20; i < 40; i++ )
    {
        temp = circularShift(5, A) + (B ^ C ^ D) + E + W[i] + K[1];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = circularShift(30, B);
        B = A;
        A = temp;
    }

    for( i = 40; i < 60; i++ )
    {
        temp = circularShift(5, A) + 
               ((B & C) | (B & D) | (C & D)) + E + W[i] + K[2];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = circularShift(30, B);
        B = A;
        A = temp;
    }

    for( i = 60; i < 80; i++ )
    {
        temp = circularShift(5, A) + (B ^ C ^ D) + E + W[i] + K[3];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = circularShift(30, B);
        B = A;
        A = temp;
    }

    aDigest[0] = (aDigest[0] + A) & 0xFFFFFFFF;
    aDigest[1] = (aDigest[1] + B) & 0xFFFFFFFF;
    aDigest[2] = (aDigest[2] + C) & 0xFFFFFFFF;
    aDigest[3] = (aDigest[3] + D) & 0xFFFFFFFF;
    aDigest[4] = (aDigest[4] + E) & 0xFFFFFFFF;
}

void idsSHA1::padMessage( UInt  *aDigest,
                          UChar *aBlock,
                          UInt   aIndex,
                          UInt   aMessageLenBitLow,
                          UInt   aMessageLenBitHigh )
{
    if( aIndex > 55 )
    {
        aBlock[aIndex++] = 0x80;
        while( aIndex < 64 )
        {
            aBlock[aIndex++] = 0;
        }

        processMessageBlock( aDigest, aBlock );
        aIndex = 0;

        while( aIndex < 56 )
        {
            aBlock[aIndex++] = 0;
        }
    }
    else
    {
        aBlock[aIndex++] = 0x80;
        while( aIndex < 56 )
        {
            aBlock[aIndex++] = 0;
        }
    }

    aBlock[56] = ( aMessageLenBitHigh >> 24 ) & 0xFF;
    aBlock[57] = ( aMessageLenBitHigh >> 16 ) & 0xFF;
    aBlock[58] = ( aMessageLenBitHigh >> 8  ) & 0xFF;
    aBlock[59] = ( aMessageLenBitHigh       ) & 0xFF;
    aBlock[60] = ( aMessageLenBitLow  >> 24 ) & 0xFF;
    aBlock[61] = ( aMessageLenBitLow  >> 16 ) & 0xFF;
    aBlock[62] = ( aMessageLenBitLow  >> 8  ) & 0xFF;
    aBlock[63] = ( aMessageLenBitLow        ) & 0xFF;

    processMessageBlock( aDigest, aBlock );
}

void idsSHA1::makeHexCode( UChar *aByte, UInt *aDigest )
{
    UInt  i;

    for( i = 0; i < 5; i++ )
    {
        *aByte++ = (*aDigest >> 24) & 0xFF;
        *aByte++ = (*aDigest >> 16) & 0xFF;
        *aByte++ = (*aDigest >> 8) & 0xFF;
        *aByte++ = *aDigest & 0xFF;

        aDigest++;
    }
}

void idsSHA1::convertHexToString( UChar *aString, UChar *aByte )
{
    UInt  i;

    for( i = 0; i < 20; i++ )
    {
        *aString++ = ASCII2HEX( ( *aByte & 0xF0 ) >> 4 );
        *aString++ = ASCII2HEX( *aByte & 0x0F );

        aByte++;
    }
}
