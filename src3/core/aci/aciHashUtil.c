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
 * $Id: aciHashUtil.c 10969 2010-04-29 09:04:59Z gurugio $
 **********************************************************************/
#include <acp.h>
#include <aciProperty.h>

static const acp_uint8_t gAciHashPermut[ /*256*/ ] = {
 
    1, 87, 49, 12,176,178,102,166,121,193,  6, 84,249,230, 44,163,
    14,197,213,181,161, 85,218, 80, 64,239, 24,226,236,142, 38,200,
    110,177,104,103,141,253,255, 50, 77,101, 81, 18, 45, 96, 31,222,
    25,107,190, 70, 86,237,240, 34, 72,242, 20,214,244,227,149,235,
    97,234, 57, 22, 60,250, 82,175,208,  5,127,199,111, 62,135,248,
    174,169,211, 58, 66,154,106,195,245,171, 17,187,182,179,  0,243,
    132, 56,148, 75,128,133,158,100,130,126, 91, 13,153,246,216,219,
    119, 68,223, 78, 83, 88,201, 99,122, 11, 92, 32,136,114, 52, 10,
    138, 30, 48,183,156, 35, 61, 26,143, 74,251, 94,129,162, 63,152,
    170,  7,115,167,241,206,  3,150, 55, 59,151,220, 90, 53, 23,131,
    125,173, 15,238, 79, 95, 89, 16,105,137,225,224,217,160, 37,123,
    118, 73,  2,157, 46,116,  9,145,134,228,207,212,202,215, 69,229,
    27,188, 67,124,168,252, 42,  4, 29,108, 21,247, 19,205, 39,203,
    233, 40,186,147,198,192,155, 33,164,191, 98,204,165,180,117, 76,
    140, 36,210,172, 41, 54,159,  8,185,232,113,196,231, 47,146,120,
    51, 65, 28,144,254,221, 93,189,194,139,112, 43, 71,109,184,209
};


ACP_EXPORT acp_uint32_t aciHashHashString(acp_uint32_t aHash,
                                          const acp_uint8_t *aValue,
                                          acp_uint32_t aLength)
{
#if defined(ACP_CFG_BIG_ENDIAN)

    acp_uint8_t sHash[4];
    const acp_uint8_t *sFence;

    sFence = aValue + aLength;

    sHash[3] = aHash;
    aHash >>= 8;
    sHash[2] = aHash;
    aHash >>= 8;
    sHash[1] = aHash;
    aHash >>= 8;
    sHash[0] = aHash;

    for( ; aValue < sFence; aValue++ )
    {
        sHash[0] = gAciHashPermut[sHash[0]^*aValue];
        sHash[1] = gAciHashPermut[sHash[1]^*aValue];
        sHash[2] = gAciHashPermut[sHash[2]^*aValue];
        sHash[3] = gAciHashPermut[sHash[3]^*aValue];
    }

    return sHash[0]<<24|sHash[1]<<16|sHash[2]<<8|sHash[3];

#else

    acp_uint8_t sHash[4];
    const acp_uint8_t *sFence;

    sFence = aValue + aLength;

    sHash[3] = aHash;
    aHash >>= 8; 
    sHash[2] = aHash;
    aHash >>= 8; 
    sHash[1] = aHash;
    aHash >>= 8;
    sHash[0] = aHash;

    sFence--;

    for(; sFence >= aValue; sFence--)
    {
        sHash[0] = gAciHashPermut[sHash[0]^*sFence];
        sHash[1] = gAciHashPermut[sHash[1]^*sFence];
        sHash[2] = gAciHashPermut[sHash[2]^*sFence];
        sHash[3] = gAciHashPermut[sHash[3]^*sFence];
    }

    return sHash[0]<<24|sHash[1]<<16|sHash[2]<<8|sHash[3];

#endif
}
