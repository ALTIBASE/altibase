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
 
#include <idl.h>
#include "idsDES.h"

/******************************************************************************
 *  SETKEY()  Generate key schedule for given key and type of cryption        *
 ******************************************************************************/

/* PERMUTED CHOICE 1 (PC1) */
// BUG-25019 클라이언트에서 동일한 변수명 사용시 경고발생
// static 로 선언한다.
static SInt PC1[] =
{
    57, 49, 41, 33, 25, 17, 9,
    1, 58, 50, 42, 34, 26, 18,
    10, 2, 59, 51, 43, 35, 27,
    19, 11, 3, 60, 52, 44, 36,
    63, 55, 47, 39, 31, 23, 15,
    7, 62, 54, 46, 38, 30, 22,
    14, 6, 61, 53, 45, 37, 29,
    21, 13, 5, 28, 20, 12, 4,
};

/* Schedule of left shifts for C and D blocks */
static SInt SHIFTS[] =
{1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1};

/* PERMUTED CHOICE 2 (PC2) */
static SInt PC2[] =
{
    14, 17, 11, 24, 1, 5,
    3, 28, 15, 6, 21, 10,
    23, 19, 12, 4, 26, 8,
    16, 7, 27, 20, 13, 2,
    41, 52, 31, 37, 47, 55,
    30, 40, 51, 45, 33, 48,
    44, 49, 39, 56, 34, 53,
    46, 42, 50, 36, 29, 32,
};

void
idsDES::setkey(
    UChar   *pkey,                  /* 64-bit key packed into 8 bytes     */
    SInt    edflag)                 /* type cryption: 0=encrypt,1=decrypt */
{
    register SInt i, j, k, t1, t2;
    UChar key[64];
    UChar CD[56];

    /* Unpack KEY from 8 bits/byte into 1 bit/byte */
    unpack8(pkey, key);

    /* Permute unpacked key with PC1 to generate C and D */
    for (i = 0; i < 56; i++)
        CD[i] = key[PC1[i] - 1];

    /* Rotate and permute C and D to generate 16 subkeys */
    for (i = 0; i < 16; i++) {
        /* Rotate C and D */
        for (j = 0; j < SHIFTS[i]; j++) {
            t1 = CD[0];
            t2 = CD[28];
            for (k = 0; k < 27; k++) {
                CD[k] = CD[k + 1];
                CD[k + 28] = CD[k + 29];
            }
            CD[27] = t1;
            CD[55] = t2;
        }
        /* Set order of subkeys for type of cryption */
        j = edflag ? 15 - i : i;
        /* Permute C and D with PC2 to generate KS[j] */
        for (k = 0; k < 48; k++)
            KS[j][k] = CD[PC2[k] - 1];
    }
    return;
}



/******************************************************************************
 *  DES()                                                                     *
 ******************************************************************************/


/* INITIAL PERMUTATION (IP) */
// BUG-25019 클라이언트에서 동일한 변수명 사용시 경고발생
// static 로 선언한다.
static SInt IP[] =
{
    58, 50, 42, 34, 26, 18, 10, 2,
    60, 52, 44, 36, 28, 20, 12, 4,
    62, 54, 46, 38, 30, 22, 14, 6,
    64, 56, 48, 40, 32, 24, 16, 8,
    57, 49, 41, 33, 25, 17, 9, 1,
    59, 51, 43, 35, 27, 19, 11, 3,
    61, 53, 45, 37, 29, 21, 13, 5,
    63, 55, 47, 39, 31, 23, 15, 7,
};

/* REVERSE FINAL PERMUTATION (IP-1) */
static SInt RFP[] =
{
    8, 40, 16, 48, 24, 56, 32, 64,
    7, 39, 15, 47, 23, 55, 31, 63,
    6, 38, 14, 46, 22, 54, 30, 62,
    5, 37, 13, 45, 21, 53, 29, 61,
    4, 36, 12, 44, 20, 52, 28, 60,
    3, 35, 11, 43, 19, 51, 27, 59,
    2, 34, 10, 42, 18, 50, 26, 58,
    1, 33, 9, 41, 17, 49, 25, 57,
};

/* E BIT-SELECTION TABLE */
static SInt E[] =
{
    32, 1, 2, 3, 4, 5,
    4, 5, 6, 7, 8, 9,
    8, 9, 10, 11, 12, 13,
    12, 13, 14, 15, 16, 17,
    16, 17, 18, 19, 20, 21,
    20, 21, 22, 23, 24, 25,
    24, 25, 26, 27, 28, 29,
    28, 29, 30, 31, 32, 1,
};

/* PERMUTATION FUNCTION P */
static SInt P[] =
{
    16, 7, 20, 21,
    29, 12, 28, 17,
    1, 15, 23, 26,
    5, 18, 31, 10,
    2, 8, 24, 14,
    32, 27, 3, 9,
    19, 13, 30, 6,
    22, 11, 4, 25,
};

/* 8 S-BOXES */
static SInt S[8][64] =
{
    { 14, 4, 13, 1, 2, 15, 11, 8, 3, 10, 6, 12, 5, 9, 0, 7,
      0, 15, 7, 4, 14, 2, 13, 1, 10, 6, 12, 11, 9, 5, 3, 8,
      4, 1, 14, 8, 13, 6, 2, 11, 15, 12, 9, 7, 3, 10, 5, 0,
      15, 12, 8, 2, 4, 9, 1, 7, 5, 11, 3, 14, 10, 0, 6, 13},

    { 15, 1, 8, 14, 6, 11, 3, 4, 9, 7, 2, 13, 12, 0, 5, 10,
      3, 13, 4, 7, 15, 2, 8, 14, 12, 0, 1, 10, 6, 9, 11, 5,
      0, 14, 7, 11, 10, 4, 13, 1, 5, 8, 12, 6, 9, 3, 2, 15,
      13, 8, 10, 1, 3, 15, 4, 2, 11, 6, 7, 12, 0, 5, 14, 9},

    { 10, 0, 9, 14, 6, 3, 15, 5, 1, 13, 12, 7, 11, 4, 2, 8,
      13, 7, 0, 9, 3, 4, 6, 10, 2, 8, 5, 14, 12, 11, 15, 1,
      13, 6, 4, 9, 8, 15, 3, 0, 11, 1, 2, 12, 5, 10, 14, 7,
      1, 10, 13, 0, 6, 9, 8, 7, 4, 15, 14, 3, 11, 5, 2, 12},

    { 7, 13, 14, 3, 0, 6, 9, 10, 1, 2, 8, 5, 11, 12, 4, 15,
      13, 8, 11, 5, 6, 15, 0, 3, 4, 7, 2, 12, 1, 10, 14, 9,
      10, 6, 9, 0, 12, 11, 7, 13, 15, 1, 3, 14, 5, 2, 8, 4,
      3, 15, 0, 6, 10, 1, 13, 8, 9, 4, 5, 11, 12, 7, 2, 14},

    { 2, 12, 4, 1, 7, 10, 11, 6, 8, 5, 3, 15, 13, 0, 14, 9,
      14, 11, 2, 12, 4, 7, 13, 1, 5, 0, 15, 10, 3, 9, 8, 6,
      4, 2, 1, 11, 10, 13, 7, 8, 15, 9, 12, 5, 6, 3, 0, 14,
      11, 8, 12, 7, 1, 14, 2, 13, 6, 15, 0, 9, 10, 4, 5, 3},

    { 12, 1, 10, 15, 9, 2, 6, 8, 0, 13, 3, 4, 14, 7, 5, 11,
      10, 15, 4, 2, 7, 12, 9, 5, 6, 1, 13, 14, 0, 11, 3, 8,
      9, 14, 15, 5, 2, 8, 12, 3, 7, 0, 4, 10, 1, 13, 11, 6,
      4, 3, 2, 12, 9, 5, 15, 10, 11, 14, 1, 7, 6, 0, 8, 13},

    { 4, 11, 2, 14, 15, 0, 8, 13, 3, 12, 9, 7, 5, 10, 6, 1,
      13, 0, 11, 7, 4, 9, 1, 10, 14, 3, 5, 12, 2, 15, 8, 6,
      1, 4, 11, 13, 12, 3, 7, 14, 10, 15, 6, 8, 0, 5, 9, 2,
      6, 11, 13, 8, 1, 4, 10, 7, 9, 5, 0, 15, 14, 2, 3, 12},

    { 13, 2, 8, 4, 6, 15, 11, 1, 10, 9, 3, 14, 5, 0, 12, 7,
      1, 15, 13, 8, 10, 3, 7, 4, 12, 5, 6, 11, 0, 14, 9, 2,
      7, 11, 4, 1, 9, 12, 14, 2, 0, 6, 10, 13, 15, 3, 5, 8,
      2, 1, 14, 7, 4, 10, 8, 13, 15, 12, 9, 0, 3, 5, 6, 11}
};


void
idsDES::des(
    UChar *in,                       /* packed 64-bit INPUT block  */
    UChar *out)                      /* packed 64-bit OUTPUT block */
{
    register SInt i, j, k, t;
    UChar block[64];      /* unpacked 64-bit input/output block */
    UChar LR[64], f[32], preS[48];

    /* Unpack the INPUT block */
    unpack8(in, block);

    /* Permute unpacked input block with IP to generate L and R */
    for (j = 0; j < 64; j++)
        LR[j] = block[IP[j] - 1];

    /* Perform 16 rounds */
    for (i = 0; i < 16; i++) {
        /* Expand R to 48 bits with E and XOR with i-th subkey */
        for (j = 0; j < 48; j++)
            preS[j] = LR[E[j] + 31] ^ KS[i][j];
        /* Map 8 6-bit blocks into 8 4-bit blocks using S-Boxes */
        for (j = 0; j < 8; j++) {
            /* Compute index t into j-th S-box */
            k = 6 * j;
            t = preS[k];
            t = (t << 1) | preS[k + 5];
            t = (t << 1) | preS[k + 1];
            t = (t << 1) | preS[k + 2];
            t = (t << 1) | preS[k + 3];
            t = (t << 1) | preS[k + 4];
            /* Fetch t-th entry from j-th S-box */
            t = S[j][t];
            /* Generate 4-bit block from S-box entry */
            k = 4 * j;
            f[k] = (t >> 3) & 1;
            f[k + 1] = (t >> 2) & 1;
            f[k + 2] = (t >> 1) & 1;
            f[k + 3] = t & 1;
        }
        for (j = 0; j < 32; j++) {
            /* Copy R */
            t = LR[j + 32];
            /* Permute f w/ P and XOR w/ L to generate new R */
            LR[j + 32] = LR[j] ^ f[P[j] - 1];
            /* Copy original R to new L */
            LR[j] = t;
        }
    }

    /* Permute L and R with reverse IP-1 to generate output block */
    for (j = 0; j < 64; j++)
        block[j] = LR[RFP[j] - 1];

    /* Pack data into 8 bits per byte */
    pack8(out, block);
}


/******************************************************************************
 *  PACK8()  Pack 64 bytes at 1 bit/byte into 8 bytes at 8 bits/byte          *
 ******************************************************************************/

void
idsDES::pack8(
    UChar *packed,          /* packed block ( 8 bytes at 8 bits/byte)      */
    UChar *binary)          /* the unpacked block (64 bytes at 1 bit/byte) */
{
    register SInt i, j, k;

    for (i = 0; i < 8; i++) {
        k = 0;
        for (j = 0; j < 8; j++)
            k = (k << 1) + *binary++;
        *packed++ = k;
    }
}

/******************************************************************************
 *  UNPACK8()  Unpack 8 bytes at 8 bits/byte into 64 bytes at 1 bit/byte      *
 ******************************************************************************/

void
idsDES::unpack8(
    UChar *packed,                   /* packed block (8 bytes at 8 bits/byte)   */
    UChar *binary)                   /* unpacked block (64 bytes at 1 bit/byte) */
{
    register SInt i, j, k;

    for (i = 0; i < 8; i++) {
        k = *packed++;
        for (j = 0; j < 8; j++)
            *binary++ = (k >> (7 - j)) & 01;
    }
}

#define ASCII2HEX(c)     ((c < 10) ? ('0' + c) : ('A' + (c - 10)))
#define HEX2ASCII(c)     ((c >= '0' && c <= '9') ? (c - '0') :          \
                          ((c <= 'Z')? (10+c-'A') : (10 + c - 'a'))     \
                          )

void
idsDES::bin2ascii(SChar *in, SChar *out, SInt size)
{
    SChar    c;

    while (size-- != 0 )   {
        c = *in++;
        *out ++ = ASCII2HEX(((c & 0xF0) >> 4 ));
        *out ++ = ASCII2HEX((c & 0x0F));
    }

    *out = '\0';
}

void
idsDES::ascii2bin(SChar *in, SChar *out, SInt size)
{
    SChar   c1, c2;

    while ( size-- != 0 )   {
        c1 = *in++;
        c2 = *in++;

        *out++ = (((HEX2ASCII(c1) << 4)&0xF0) |  HEX2ASCII(c2));
    }
    *out = '\0';
}

void
idsDES::tripleDes (UChar *in, UChar *out, UChar *key, SInt edflag)
{

    setkey ( key + (edflag * 16 ), edflag );
    des ( in, out);
    setkey ( key + 8, !edflag );
    des ( out, in );
    setkey ( key + ((!edflag) * 16), edflag );
    des ( in, out );
}

// To fix BUG-14496
// for CBC(Cipher Block Chaning)
void
idsDES::blockXOR( UChar *aTarget, UChar *aSource )
{
    aTarget[0]^=aSource[0];
    aTarget[1]^=aSource[1];
    aTarget[2]^=aSource[2];
    aTarget[3]^=aSource[3];
    aTarget[4]^=aSource[4];
    aTarget[5]^=aSource[5];
    aTarget[6]^=aSource[6];
    aTarget[7]^=aSource[7];
}
