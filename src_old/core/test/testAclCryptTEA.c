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
 * $Id: testAclCryptTEA.c 4486 2009-02-10 07:37:20Z jykim $
 ******************************************************************************/

#include <act.h>
#include <acpStr.h>
#include <acpMem.h>
#include <aclCrypt.h>

acp_sint32_t main(void)
{
    acp_uint32_t sKey1[8] = 
    {
        0x414C5449, 0x42415345, 0x54484542, 0x45535421,
        0xC54CD2F0, 0xBCA0C774, 0xC2A4CD5C, 0xACE0C57C
    };
    acp_uint32_t sKey2[8] = 
    {
        0xC54CD2F0, 0xBCA0C774, 0xC2A4CD5C, 0xACE0C57C,
        0x414C5449, 0x42415345, 0x54484542, 0x45535421
    };

    acp_uint8_t sPlain[] =
        "THEBEST!" "ALTIBASE" "Altibase" "Oracle^^"
        "SybaseTT" "MSSQL-_-" "MySQL_-_" "Informix";
    acp_uint8_t sCipher1[65];
    acp_uint8_t sCipher2[65];
    acp_uint8_t sPlain2[65];
    acp_uint8_t sPlain1[65];

    ACT_TEST_BEGIN();

    ACT_CHECK(ACP_RC_IS_SUCCESS(
            aclCryptTEAEncipher(sPlain, sKey1, sCipher1, 64, 32)
            ));
    ACT_CHECK(ACP_RC_IS_SUCCESS(
            aclCryptTEAEncipher(sCipher1, sKey2, sCipher2, 64, 32)
            ));
    ACT_CHECK(ACP_RC_IS_SUCCESS(
            aclCryptTEADecipher(sCipher2, sKey2, sPlain2, 64, 32)
            ));
    ACT_CHECK(ACP_RC_IS_SUCCESS(
            aclCryptTEADecipher(sCipher1, sKey1, sPlain1, 64, 32)
            ));
    ACT_CHECK(0 == acpMemCmp(sPlain2, sCipher1, 64));
    ACT_CHECK(0 == acpMemCmp(sPlain1, sPlain, 64));

    ACT_CHECK(ACP_RC_NOT_SUCCESS(
            aclCryptTEADecipher(sCipher1, sKey1, sPlain1, 63, 32)
            ));
    ACT_CHECK(ACP_RC_NOT_SUCCESS(
            aclCryptTEADecipher(sCipher1, sKey1, sPlain1, 64, 31)
            ));
    ACT_CHECK(ACP_RC_NOT_SUCCESS(
            aclCryptTEADecipher(sCipher1, sKey1, sPlain1, 63, 31)
            ));
    ACT_CHECK(ACP_RC_NOT_SUCCESS(
            aclCryptTEADecipher(sCipher1, sKey1, sPlain1, 63, 0)
            ));

    ACT_TEST_END();

    return 0;
}
