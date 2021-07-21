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
 * $Id:
 ******************************************************************************/

#include <acpMem.h>
#include <aclCrypt.h>

static void aclCryptByteCopy(acp_uint32_t* aV, acp_uint8_t* aC)
{
    *aV  = aC[0] << 24;
    *aV |= aC[1] << 16;
    *aV |= aC[2] <<  8;
    *aV |= aC[3];
}

static void aclCryptDWordCopy(acp_uint8_t* aC, acp_uint32_t* aV)
{
    aC[0] = ((*aV) >> 24) & 0xFF;
    aC[1] = ((*aV) >> 16) & 0xFF;
    aC[2] = ((*aV) >>  8) & 0xFF;
    aC[3] = ((*aV)      ) & 0xFF;
}

ACP_EXPORT acp_rc_t aclCryptTEAEncipher(void* aPlain,
                             void* aKey,
                             void* aCipher,
                             acp_size_t aTextLength,
                             acp_size_t aKeyLength)
{
    acp_rc_t sRC = ACP_RC_SUCCESS;

    if((aKeyLength != 0) && 
       (0 == (aTextLength % 8)) && 
       (0 == (aKeyLength % 16)))
    {
        acp_uint32_t sDelta = ((acp_uint32_t)(0x9E3779B9));

        acp_uint8_t* sPlain = aPlain;
        acp_uint8_t* sKey = aKey;
        acp_uint8_t* sCipher = aCipher; 
        
        acp_uint32_t sV[2];
        acp_uint32_t sK[4];

        acp_size_t i;
        acp_size_t j;
        acp_size_t k;
        acp_uint32_t sSum;

        for(i = 0, k = 0; i < aTextLength; i += 8)
        {
            /* Copy plain text and key */
            aclCryptByteCopy(&sV[0], sPlain + i);
            aclCryptByteCopy(&sV[1], sPlain + i + 4);
            aclCryptByteCopy(&sK[0], sKey + k);
            aclCryptByteCopy(&sK[1], sKey + k + 4);
            aclCryptByteCopy(&sK[2], sKey + k + 8);
            aclCryptByteCopy(&sK[3], sKey + k + 12);

            /* TEA Enciphering Algorithm
             * Converting 64bit plaintext to 64bit cipher using 128bit key */
            sSum = 0;
            for(j=0; j<32; j++)
            {
                sSum += sDelta; 
                sV[0] +=
                    ((sV[1]<<4) + sK[0]) ^
                    (sV[1] + sSum) ^
                    ((sV[1]>>5) + sK[1]);
                sV[1] +=
                    ((sV[0]<<4) + sK[2]) ^
                    (sV[0] + sSum) ^
                    ((sV[0]>>5) + sK[3]);
            }

            /* Copy enciphered text back to aCipher */
            aclCryptDWordCopy(sCipher + i    , &sV[0]);
            aclCryptDWordCopy(sCipher + i + 4, &sV[1]);

            k = (k + 16) % aKeyLength;
        }
    }
    else
    {
        sRC = ACP_RC_ENOTSUP;
    }

    return sRC;
}

ACP_EXPORT acp_rc_t aclCryptTEADecipher(void* aCipher,
                             void* aKey,
                             void* aPlain,
                             acp_size_t aTextLength,
                             acp_size_t aKeyLength)
{
    acp_rc_t sRC = ACP_RC_SUCCESS;

    if((aKeyLength != 0) && 
       (0 == (aTextLength % 8)) && 
       (0 == (aKeyLength % 16)))
    {
        acp_uint32_t sDelta = ((acp_uint32_t)(0x9E3779B9));

        acp_uint8_t* sKey = aKey;
        acp_uint8_t* sCipher = aCipher; 
        acp_uint8_t* sPlain = aPlain;
        
        acp_uint32_t sV[2];
        acp_uint32_t sK[4];

        acp_size_t i;
        acp_size_t j;
        acp_size_t k;
        acp_uint32_t sSum;

        for(i = 0, k = 0; i < aTextLength; i += 8)
        {
            /* Copy plain text and key */
            aclCryptByteCopy(&sV[0], sCipher + i);
            aclCryptByteCopy(&sV[1], sCipher + i + 4);
            aclCryptByteCopy(&sK[0], sKey + k);
            aclCryptByteCopy(&sK[1], sKey + k + 4);
            aclCryptByteCopy(&sK[2], sKey + k + 8);
            aclCryptByteCopy(&sK[3], sKey + k + 12);

            /* Decipher */
            sSum = ((acp_uint32_t)(0xC6EF3720));

            /* TEA Enciphering Algorithm
             * Converting 64bit plaintext to 64bit cipher using 128bit key */
            for (j=0; j<32; j++)
            {
                sV[1] -= 
                    ((sV[0]<<4) + sK[2]) ^ 
                    (sV[0] + sSum) ^ 
                    ((sV[0]>>5) + sK[3]);
                sV[0] -= 
                    ((sV[1]<<4) + sK[0]) ^ 
                    (sV[1] + sSum) ^ 
                    ((sV[1]>>5) + sK[1]);
                sSum -= sDelta;
            }

            /* Copy deciphered text back to aPlain */
            aclCryptDWordCopy(sPlain + i    , &sV[0]);
            aclCryptDWordCopy(sPlain + i + 4, &sV[1]);

            k = (k + 16) % aKeyLength;
        }
    }
    else
    {
        sRC = ACP_RC_ENOTSUP;
    }

    return sRC;
}
