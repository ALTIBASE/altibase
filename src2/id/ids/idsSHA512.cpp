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
#include <idsSHA512.h>

#define ASCII2HEX(c) (((c) < 10) ? ('0' + (c)) : ('A' + ((c) - 10)))

/* Rotate Right ( circular right shift ) */
#define ROTR( bits, word ) \
    ((((word) >> (bits)) & ID_ULONG(0xFFFFFFFFFFFFFFFF) ) | \
      ((word) << (64-(bits))))

/* SHA-512 Functions described in FIPS 180-4 Ch.4.1.3 */
#define CH(x,y,z) \
    (((x) & (y)) ^ ( ~(x) & (z) ))

#define MAJ(x,y,z) \
    (((x) & (y)) ^ ((x) & (z)) ^ ((y) &(z)))

#define USIGMA0(x) \
    (ROTR(28,(x)) ^ ROTR(34,(x)) ^ ROTR(39,(x)))

#define USIGMA1(x) \
    (ROTR(14,(x)) ^ ROTR(18,(x)) ^ ROTR(41,(x)))

#define LSIGMA0(x) \
    (ROTR(1,(x)) ^ ROTR(8,(x)) ^ ((x) >> 7))

#define LSIGMA1(x) \
    (ROTR(19,(x)) ^ ROTR(61,(x)) ^ ((x) >> 6))

/* SHA-512 Block size */
#define SHA512_BLK_SIZE (128)

void idsSHA512::digestToHexCode( UChar * aByte, 
                                 UChar * aMessage,
                                 ULong   aMessageLen )
{
    ULong        sRemainsLen = aMessageLen;
    ULong sDigest[8]  = {
        /* SHA-512 Initial hash value described in FIPS 180-4 Ch.5.3.5 */
        ID_ULONG(0x6A09E667F3BCC908),
        ID_ULONG(0xBB67AE8584CAA73B),
        ID_ULONG(0x3C6EF372FE94F82B),
        ID_ULONG(0xA54FF53A5F1D36F1),
        ID_ULONG(0x510E527FADE682D1),
        ID_ULONG(0x9B05688C2B3E6C1F),
        ID_ULONG(0x1F83D9ABFB41BD6B),
        ID_ULONG(0x5BE0CD19137E2179)
    };

    while( sRemainsLen >= SHA512_BLK_SIZE )
    {
        processBlock( sDigest, aMessage );
        aMessage    += SHA512_BLK_SIZE;
        sRemainsLen -= SHA512_BLK_SIZE;
    }
    processRemains( sDigest, aMessage, aMessageLen, sRemainsLen );

    makeHexCode( aByte, sDigest );
}

void idsSHA512::digest( UChar * aResult, UChar * aMessage, ULong aMessageLen )
{
    UChar sByte[64];

    digestToHexCode( sByte, aMessage, aMessageLen );

    convertHexToString( aResult, sByte );
}

void idsSHA512::processRemains( ULong * aDigest,
                                UChar * aMessage,
                                ULong   aMessageLen,
                                ULong   aRemainsLen )
{
    UInt  i;
    UChar sBlock[ SHA512_BLK_SIZE ] = { 0 , };
    ULong sMessageLenBitHigh        = 0;
    ULong sMessageLenBitLow         = 0;

    for( i = 0 ; i < aRemainsLen ; i++ )
    {
        sBlock[i] = aMessage[i];
    }
    /* Append the bit '1' to the end of the message */
    sBlock[aRemainsLen] = 0x80;

    if ( aRemainsLen >= 112 )
    {
        processBlock( aDigest, sBlock );
        idlOS::memset( sBlock, 0, 112 );
    }
    else
    {
        // Nothing to do 
    }

    sMessageLenBitHigh = ( aMessageLen & ID_ULONG(0xE000000000000000) ) >> 56;
    sMessageLenBitLow  = aMessageLen << 3;

    sBlock[112] = ( sMessageLenBitHigh >> 56 ) & 0xFF;
    sBlock[113] = ( sMessageLenBitHigh >> 48 ) & 0xFF;
    sBlock[114] = ( sMessageLenBitHigh >> 40 ) & 0xFF;
    sBlock[115] = ( sMessageLenBitHigh >> 32 ) & 0xFF;
    sBlock[116] = ( sMessageLenBitHigh >> 24 ) & 0xFF;
    sBlock[117] = ( sMessageLenBitHigh >> 16 ) & 0xFF;
    sBlock[118] = ( sMessageLenBitHigh >>  8 ) & 0xFF;
    sBlock[119] =   sMessageLenBitHigh         & 0xFF;
    
    sBlock[120] = ( sMessageLenBitLow >> 56 ) & 0xFF;
    sBlock[121] = ( sMessageLenBitLow >> 48 ) & 0xFF;
    sBlock[122] = ( sMessageLenBitLow >> 40 ) & 0xFF;
    sBlock[123] = ( sMessageLenBitLow >> 32 ) & 0xFF;
    sBlock[124] = ( sMessageLenBitLow >> 24 ) & 0xFF;
    sBlock[125] = ( sMessageLenBitLow >> 16 ) & 0xFF;
    sBlock[126] = ( sMessageLenBitLow >>  8 ) & 0xFF;
    sBlock[127] =   sMessageLenBitLow         & 0xFF;

    processBlock( aDigest, sBlock );
}

void idsSHA512::processBlock( ULong * aDigest,
                              UChar * aBlock )
{
    UInt         i;
    ULong        W[80];
    ULong        A, B, C, D, E, F, G, H, T1, T2; // 8 working variables and 2 temporary words
    static ULong K[] =
    {
        /* SHA-512 Constants described in FIPS 180-4 Ch.4.2.3 */
        ID_ULONG(0x428A2F98D728AE22),
        ID_ULONG(0x7137449123EF65CD),
        ID_ULONG(0xB5C0FBCFEC4D3B2F),
        ID_ULONG(0xE9B5DBA58189DBBC),
        ID_ULONG(0x3956C25BF348B538),
        ID_ULONG(0x59F111F1B605D019),
        ID_ULONG(0x923F82A4AF194F9B),
        ID_ULONG(0xAB1C5ED5DA6D8118),
        ID_ULONG(0xD807AA98A3030242),
        ID_ULONG(0x12835B0145706FBE),
        ID_ULONG(0x243185BE4EE4B28C),
        ID_ULONG(0x550C7DC3D5FFB4E2),
        ID_ULONG(0x72BE5D74F27B896F),
        ID_ULONG(0x80DEB1FE3B1696B1),
        ID_ULONG(0x9BDC06A725C71235),
        ID_ULONG(0xC19BF174CF692694),
        ID_ULONG(0xE49B69C19EF14AD2),
        ID_ULONG(0xEFBE4786384F25E3),
        ID_ULONG(0x0FC19DC68B8CD5B5),
        ID_ULONG(0x240CA1CC77AC9C65),
        ID_ULONG(0x2DE92C6F592B0275),
        ID_ULONG(0x4A7484AA6EA6E483),
        ID_ULONG(0x5CB0A9DCBD41FBD4),
        ID_ULONG(0x76F988DA831153B5),
        ID_ULONG(0x983E5152EE66DFAB),
        ID_ULONG(0xA831C66D2DB43210),
        ID_ULONG(0xB00327C898FB213F),
        ID_ULONG(0xBF597FC7BEEF0EE4),
        ID_ULONG(0xC6E00BF33DA88FC2),
        ID_ULONG(0xD5A79147930AA725),
        ID_ULONG(0x06CA6351E003826F),
        ID_ULONG(0x142929670A0E6E70),
        ID_ULONG(0x27B70A8546D22FFC),
        ID_ULONG(0x2E1B21385C26C926),
        ID_ULONG(0x4D2C6DFC5AC42AED),
        ID_ULONG(0x53380D139D95B3DF),
        ID_ULONG(0x650A73548BAF63DE),
        ID_ULONG(0x766A0ABB3C77B2A8),
        ID_ULONG(0x81C2C92E47EDAEE6),
        ID_ULONG(0x92722C851482353B),
        ID_ULONG(0xA2BFE8A14CF10364),
        ID_ULONG(0xA81A664BBC423001),
        ID_ULONG(0xC24B8B70D0F89791),
        ID_ULONG(0xC76C51A30654BE30),
        ID_ULONG(0xD192E819D6EF5218),
        ID_ULONG(0xD69906245565A910),
        ID_ULONG(0xF40E35855771202A),
        ID_ULONG(0x106AA07032BBD1B8),
        ID_ULONG(0x19A4C116B8D2D0C8),
        ID_ULONG(0x1E376C085141AB53),
        ID_ULONG(0x2748774CDF8EEB99),
        ID_ULONG(0x34B0BCB5E19B48A8),
        ID_ULONG(0x391C0CB3C5C95A63),
        ID_ULONG(0x4ED8AA4AE3418ACB),
        ID_ULONG(0x5B9CCA4F7763E373),
        ID_ULONG(0x682E6FF3D6B2B8A3),
        ID_ULONG(0x748F82EE5DEFB2FC),
        ID_ULONG(0x78A5636F43172F60),
        ID_ULONG(0x84C87814A1F0AB72),
        ID_ULONG(0x8CC702081A6439EC),
        ID_ULONG(0x90BEFFFA23631E28),
        ID_ULONG(0xA4506CEBDE82BDE9),
        ID_ULONG(0xBEF9A3F7B2C67915),
        ID_ULONG(0xC67178F2E372532B),
        ID_ULONG(0xCA273ECEEA26619C),
        ID_ULONG(0xD186B8C721C0C207),
        ID_ULONG(0xEADA7DD6CDE0EB1E),
        ID_ULONG(0xF57D4F7FEE6ED178),
        ID_ULONG(0x06F067AA72176FBA),
        ID_ULONG(0x0A637DC5A2C898A6),
        ID_ULONG(0x113F9804BEF90DAE),
        ID_ULONG(0x1B710B35131C471B),
        ID_ULONG(0x28DB77F523047D84),
        ID_ULONG(0x32CAAB7B40C72493),
        ID_ULONG(0x3C9EBE0A15C9BEBC),
        ID_ULONG(0x431D67C49C100D4C),
        ID_ULONG(0x4CC5D4BECB3E42B6),
        ID_ULONG(0x597F299CFC657E2A),
        ID_ULONG(0x5FCB6FAB3AD6FAEC),
        ID_ULONG(0x6C44198C4A475817)
    };
   
    /* FoULLowing steps are described in FIPS 180-4 Ch.6.4 */
    /* 1. Prepare the message schedule */
    for ( i = 0 ; i < 16 ; i++ )
    {
        W[i]  = ( (ULong) aBlock[i * 8    ] ) << 56;
        W[i] |= ( (ULong) aBlock[i * 8 + 1] ) << 48;
        W[i] |= ( (ULong) aBlock[i * 8 + 2] ) << 40;
        W[i] |= ( (ULong) aBlock[i * 8 + 3] ) << 32;
        W[i] |= ( (ULong) aBlock[i * 8 + 4] ) << 24;
        W[i] |= ( (ULong) aBlock[i * 8 + 5] ) << 16;
        W[i] |= ( (ULong) aBlock[i * 8 + 6] ) <<  8;
        W[i] |= ( (ULong) aBlock[i * 8 + 7] );
    }

    for ( i = 16 ; i < 80 ; i++ )
    {
        W[i] = LSIGMA1(W[i-2]) + W[i-7] + LSIGMA0(W[i-15]) + W[i-16];
    }

    /* 2. Initialize 8 working variables( a,b,c,d,e,f,g,h ) with the previous hash value */
    A = aDigest[0];
    B = aDigest[1];
    C = aDigest[2];
    D = aDigest[3];
    E = aDigest[4];
    F = aDigest[5];
    G = aDigest[6];
    H = aDigest[7];

    /* 3. Compute */
    for ( i = 0 ; i < 80 ; i++ )
    {
        T1 = H + USIGMA1(E) + CH(E,F,G) + K[i] + W[i];
        T2 = USIGMA0(A) + MAJ(A,B,C);
        H = G;
        G = F;
        F = E;
        E = D + T1;
        D = C;
        C = B;
        B = A;
        A = T1 + T2;
    }

    /* 4. Compute hash value */
    aDigest[0] += A;
    aDigest[1] += B;
    aDigest[2] += C;
    aDigest[3] += D;
    aDigest[4] += E;
    aDigest[5] += F;
    aDigest[6] += G;
    aDigest[7] += H;
}

void idsSHA512::makeHexCode( UChar * aByte, ULong * aDigest )
{
    UInt i;

    for ( i = 0; i < 8; i++ )
    {
        *aByte++ = ( *aDigest >> 56 ) & 0xFF;
        *aByte++ = ( *aDigest >> 48 ) & 0xFF;
        *aByte++ = ( *aDigest >> 40 ) & 0xFF;
        *aByte++ = ( *aDigest >> 32 ) & 0xFF;
        *aByte++ = ( *aDigest >> 24 ) & 0xFF;
        *aByte++ = ( *aDigest >> 16 ) & 0xFF;
        *aByte++ = ( *aDigest >>  8 ) & 0xFF;
        *aByte++ =   *aDigest         & 0xFF;

        aDigest++;
    }
}

void idsSHA512::convertHexToString( UChar * aString, UChar * aByte )
{
    UInt i;
    for( i = 0 ; i < 64 ; i++ )
    {
        *aString++ = ASCII2HEX( ( *aByte & 0xF0 ) >> 4 );
        *aString++ = ASCII2HEX( *aByte & 0x0F );

        aByte++;
    }
}
