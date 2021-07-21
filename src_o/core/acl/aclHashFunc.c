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
 
/*******************************************************************************
 * $Id: aclHashFunc.c 7014 2009-08-18 01:17:30Z djin $
 ******************************************************************************/

#include <acpStr.h>
#include <acpCStr.h>
#include <aclHash.h>


static const acp_uint8_t gAclHashBinaryPermut[256] =
{
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
    51, 65, 28,144,254,221, 93,189,194,139,112, 43, 71,109,184,209,
};


/**
 * builtin hash function for 32 bit integer
 * @param aKey key data
 * @param aKeyLength length of key
 * @return hash value
 */
ACP_EXPORT acp_uint32_t aclHashHashInt32(
    const void* aKey,
    acp_size_t  aKeyLength)
{
    acp_uint32_t sValue = *(acp_uint32_t *)aKey;

    ACP_UNUSED(aKeyLength);

    sValue += ~(sValue << 15);
    sValue ^=  (sValue >> 10);
    sValue +=  (sValue << 3);
    sValue ^=  (sValue >> 6);
    sValue += ~(sValue << 11);
    sValue ^=  (sValue >> 16);

    return sValue;
}

/**
 * builtin compare function for 32 bit integer
 * @param aKey1 first key data to compare
 * @param aKey2 second key data to compare
 * @param aKeyLength length of key
 * @return zero if identical, otherwise non-zero
 */
ACP_EXPORT acp_sint32_t aclHashCompInt32(
    const void* aKey1,
    const void* aKey2,
    acp_size_t  aKeyLength)
{
    ACP_UNUSED(aKeyLength);
    return *(acp_sint32_t *)aKey1 - *(acp_sint32_t *)aKey2;
}

/**
 * builtin hash function for 64 bit integer
 * @param aKey key data
 * @param aKeyLength length of key
 * @return hash value
 */
ACP_EXPORT acp_uint32_t aclHashHashInt64(
    const void *aKey,
    acp_size_t aKeyLength)
{
    acp_uint32_t sHigh  = (acp_uint32_t)(*(acp_uint64_t *)aKey >> 32);
    acp_uint32_t sLow   = (acp_uint32_t)(*(acp_uint64_t *)aKey & 0xffffffff);
    acp_uint32_t sValue = sHigh ^ sLow;

    ACP_UNUSED(aKeyLength);
    sValue += ~(sValue << 15);
    sValue ^=  (sValue >> 10);
    sValue +=  (sValue << 3);
    sValue ^=  (sValue >> 6);
    sValue += ~(sValue << 11);
    sValue ^=  (sValue >> 16);

    return sValue;
}

/**
 * builtin compare function for 64 bit integer
 * @param aKey1 first key data to compare
 * @param aKey2 second key data to compare
 * @param aKeyLength length of key
 * @return zero if identical, otherwise non-zero
 */
ACP_EXPORT acp_sint32_t aclHashCompInt64(
    const void* aKey1,
    const void* aKey2,
    acp_size_t  aKeyLength)
{
    acp_sint64_t sValue1 = *(acp_sint64_t *)aKey1;
    acp_sint64_t sValue2 = *(acp_sint64_t *)aKey2;

    ACP_UNUSED(aKeyLength);
    if (sValue1 == sValue2)
    {
        return 0;
    }
    else if (sValue1 < sValue2)
    {
        return -1;
    }
    else
    {
        return 1;
    }
}

/**
 * builtin hash function for C style null terminated string
 * @param aKey pointer to the C style null terminated string
 * @param aKeyLength length of key
 * @return hash value
 */
ACP_EXPORT acp_uint32_t aclHashHashCString(
    const void *aKey,
    acp_size_t aKeyLength)
{
    acp_size_t sLen;

    if (aKey == NULL)
    {
        return 0;
    }
    else
    {
        sLen = acpCStrLen((const acp_char_t*)aKey, aKeyLength);
    }

    return aclHashHashCStringWithLen(aKey, sLen);
}

/**
 * builtin compare function for C style null terminated string
 * @param aKey1 pointer to the first C style null terminated string
 * @param aKey2 pointer to the second C style null terminated string
 * @param aKeyLength length of key
 * @return zero if identical, otherwise non-zero
 */
ACP_EXPORT acp_sint32_t aclHashCompCString(
    const void* aKey1,
    const void* aKey2,
    acp_size_t  aKeyLength)
{
    return acpCStrCmp(aKey1, aKey2, aKeyLength);
}

/**
 * builtin hash function for string object #acp_str_t
 * @param aKey pointer to the #acp_str_t
 * @param aKeyLength length of key
 * @return hash value
 */
ACP_EXPORT acp_uint32_t aclHashHashString(
    const void *aKey,
    acp_size_t aKeyLength)
{
    ACP_UNUSED(aKeyLength);
    ACE_ASSERT(((acp_str_t *)aKey)->mString != NULL);
    
    if (acpStrGetBuffer((acp_str_t *)aKey) == NULL)
    {
        return 0;
    }
    else
    {
        return aclHashHashCStringWithLen(acpStrGetBuffer((acp_str_t *)aKey),
                                         acpStrGetLength((acp_str_t *)aKey));
    }
}

/**
 * builtin compare function for string object #acp_str_t
 * @param aKey1 pointer to the first #acp_str_t
 * @param aKey2 pointer to the second #acp_str_t
 * @param aKeyLength length of key
 * @return zero if identical, otherwise non-zero
 */
ACP_EXPORT acp_sint32_t aclHashCompString(
    const void* aKey1,
    const void* aKey2,
    acp_size_t  aKeyLength)
{
    ACP_UNUSED(aKeyLength);
    return acpStrCmp((acp_str_t *)aKey1, (acp_str_t *)aKey2, 0);
}

/**
 * builtin supporting hash function for binary
 * @param aPtr pointer to the binary data
 * @param aLen length of the binary data
 * @return hash value
 */
ACP_EXPORT acp_uint32_t aclHashHashBinaryWithLen(const acp_uint8_t *aPtr,
                                                 acp_size_t         aLen)
{
    acp_uint32_t sValue;
    acp_uint32_t sHash0 = 0;
    acp_uint32_t sHash1 = 1;
    acp_uint32_t sHash2 = 2;
    acp_uint32_t sHash3 = 3;

    for (; aLen > 0; aLen--)
    {
        sValue = *aPtr++;
        sHash0 = gAclHashBinaryPermut[sHash0 ^ sValue];
        sHash1 = gAclHashBinaryPermut[sHash1 ^ sValue];
        sHash2 = gAclHashBinaryPermut[sHash2 ^ sValue];
        sHash3 = gAclHashBinaryPermut[sHash3 ^ sValue];
    }

    return (sHash0 << 24) | (sHash1 << 16) | (sHash2 << 8) | sHash3;
}

/**
 * builtin supporting hash function for string
 * @param aPtr pointer to the string data
 * @param aLen length of the string data
 * @return hash value
 */
ACP_EXPORT acp_uint32_t aclHashHashCStringWithLen(
    const acp_char_t *aPtr,
    acp_size_t        aLen)
{
    acp_uint32_t sHash = 0;
    acp_uint32_t sRem;
    acp_uint8_t *sPtr = (acp_uint8_t *)aPtr;
    acp_uint16_t sFirstShort;
    acp_uint16_t sSecondShort;

    sRem = (acp_uint32_t)(aLen & 3);
    aLen = aLen >> 2;

    for (; aLen > 0; aLen--)
    {
        sFirstShort = (((acp_uint16_t)sPtr[0]) << 8) | sPtr[1];
        sSecondShort = (((acp_uint16_t)sPtr[2]) << 8) | sPtr[3];

        sHash += sFirstShort;
        sHash = (sHash << 16) ^ ((sSecondShort << 11) ^ sHash);
        sPtr += 4;
        sHash += sHash >> 11;
    }

    switch (sRem)
    {
    case 3:
        sFirstShort = (((acp_uint16_t)sPtr[0]) << 8) | sPtr[1];
        sHash += sFirstShort;
        sHash ^= sHash << 16;
        sHash ^= sPtr[2] << 18;
        sHash += sHash >> 11;
        break;

    case 2:
        sFirstShort = (((acp_uint16_t)sPtr[0]) << 8) | sPtr[1];
        sHash += sFirstShort;
        sHash ^= sHash << 11;
        sHash += sHash >> 17;
        break;

    case 1:
        sHash += sPtr[0];
        sHash ^= sHash << 10;
        sHash += sHash >> 1;
        break;
    }

    sHash ^= sHash << 3;
    sHash += sHash >> 5;
    sHash ^= sHash << 2;
    sHash += sHash >> 15;
    sHash ^= sHash << 10;

    return sHash;
}
