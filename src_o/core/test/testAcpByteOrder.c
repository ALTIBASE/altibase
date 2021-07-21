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
 * $Id: testAcpByteOrder.c 9029 2009-12-04 05:47:52Z djin $
 ******************************************************************************/

#include <act.h>
#include <acpByteOrder.h>
#include <acpMem.h>

acp_sint32_t main(void)
{
    acp_uint16_t sInt16;
    acp_uint32_t sInt32;
    acp_uint64_t sInt64;

#if defined(ALTI_CFG_OS_WINDOWS)
#pragma pack(push, 1)
    struct
    {
        acp_uint64_t mAlign;
        acp_uint8_t  mByte;
        acp_uint16_t mWord;
        acp_uint32_t mDWord;
        acp_uint64_t mQWord;
        acp_uint8_t  mMangle;
    } sAlign;
#pragma pack(pop)
#else
    struct
    {
        acp_uint64_t mAlign;
        acp_uint8_t  mByte;
        acp_uint16_t mWord;
        acp_uint32_t mDWord;
        acp_uint64_t mQWord;
        acp_uint8_t  mMangle;
    } __attribute__((packed)) sAlign;
#endif

    ACT_TEST_BEGIN();

    sInt16 = 1;

#if defined(ACP_CFG_BIG_ENDIAN)
    ACT_CHECK(((acp_uint8_t *)&sInt16)[0] == 0);
    ACT_CHECK(((acp_uint8_t *)&sInt16)[1] == 1);
#endif

#if defined(ACP_CFG_LITTLE_ENDIAN)
    ACT_CHECK(((acp_uint8_t *)&sInt16)[0] == 1);
    ACT_CHECK(((acp_uint8_t *)&sInt16)[1] == 0);
#endif

    sInt16 = 0x1234;
    sInt32 = 0x12345678;
    sInt64 = ACP_SINT64_LITERAL(0x1234567890ABCDEF);

    ACP_TON_BYTE2(sInt16, sInt16);
    ACP_TON_BYTE4(sInt32, sInt32);
    ACP_TON_BYTE8(sInt64, sInt64);

    ACT_CHECK(((acp_uint8_t *)&sInt16)[0] == 0x12);
    ACT_CHECK(((acp_uint8_t *)&sInt16)[1] == 0x34);

    ACT_CHECK(((acp_uint8_t *)&sInt32)[0] == 0x12);
    ACT_CHECK(((acp_uint8_t *)&sInt32)[1] == 0x34);
    ACT_CHECK(((acp_uint8_t *)&sInt32)[2] == 0x56);
    ACT_CHECK(((acp_uint8_t *)&sInt32)[3] == 0x78);

    ACT_CHECK(((acp_uint8_t *)&sInt64)[0] == 0x12);
    ACT_CHECK(((acp_uint8_t *)&sInt64)[1] == 0x34);
    ACT_CHECK(((acp_uint8_t *)&sInt64)[2] == 0x56);
    ACT_CHECK(((acp_uint8_t *)&sInt64)[3] == 0x78);
    ACT_CHECK(((acp_uint8_t *)&sInt64)[4] == 0x90);
    ACT_CHECK(((acp_uint8_t *)&sInt64)[5] == 0xAB);
    ACT_CHECK(((acp_uint8_t *)&sInt64)[6] == 0xCD);
    ACT_CHECK(((acp_uint8_t *)&sInt64)[7] == 0xEF);

    sInt16 = 0x1234;
    sInt32 = 0x12345678;
    sInt64 = ACP_SINT64_LITERAL(0x1234567890ABCDEF);

    ACP_TON_BYTE2_PTR(&sInt16, &sInt16);
    ACP_TON_BYTE4_PTR(&sInt32, &sInt32);
    ACP_TON_BYTE8_PTR(&sInt64, &sInt64);

    ACT_CHECK(((acp_uint8_t *)&sInt16)[0] == 0x12);
    ACT_CHECK(((acp_uint8_t *)&sInt16)[1] == 0x34);

    ACT_CHECK(((acp_uint8_t *)&sInt32)[0] == 0x12);
    ACT_CHECK(((acp_uint8_t *)&sInt32)[1] == 0x34);
    ACT_CHECK(((acp_uint8_t *)&sInt32)[2] == 0x56);
    ACT_CHECK(((acp_uint8_t *)&sInt32)[3] == 0x78);

    ACT_CHECK(((acp_uint8_t *)&sInt64)[0] == 0x12);
    ACT_CHECK(((acp_uint8_t *)&sInt64)[1] == 0x34);
    ACT_CHECK(((acp_uint8_t *)&sInt64)[2] == 0x56);
    ACT_CHECK(((acp_uint8_t *)&sInt64)[3] == 0x78);
    ACT_CHECK(((acp_uint8_t *)&sInt64)[4] == 0x90);
    ACT_CHECK(((acp_uint8_t *)&sInt64)[5] == 0xAB);
    ACT_CHECK(((acp_uint8_t *)&sInt64)[6] == 0xCD);
    ACT_CHECK(((acp_uint8_t *)&sInt64)[7] == 0xEF);

    /* ------------------------------------------------
     * Checking SIGBUS
     * ----------------------------------------------*/
    ACT_CHECK(0 != ((acp_ulong_t)(&(sAlign.mWord)) %
              (acp_ulong_t)(sizeof(acp_sint16_t))));
    ACT_CHECK(0 != ((acp_ulong_t)(&(sAlign.mDWord)) %
              (acp_ulong_t)(sizeof(acp_sint32_t))));
    ACT_CHECK(0 != ((acp_ulong_t)(&(sAlign.mQWord)) %
              (acp_ulong_t)(sizeof(acp_sint64_t))));

    sAlign.mByte  = 0x01;
    sAlign.mWord  = 0x0102;
    sAlign.mDWord = 0x01020304;
    sAlign.mQWord = ACP_SINT64_LITERAL(0x0102030405060708);
    ACP_TON_BYTE2_PTR(&sAlign.mWord, &sAlign.mWord);
    ACP_TON_BYTE4_PTR(&sAlign.mDWord, &sAlign.mDWord);
    ACP_TON_BYTE8_PTR(&sAlign.mQWord, &sAlign.mQWord);

    ACT_CHECK(((acp_uint8_t *)&(sAlign.mWord))[0] == 0x01);
    ACT_CHECK(((acp_uint8_t *)&(sAlign.mWord))[1] == 0x02);

    ACT_CHECK(((acp_uint8_t *)&(sAlign.mDWord))[0] == 0x01);
    ACT_CHECK(((acp_uint8_t *)&(sAlign.mDWord))[1] == 0x02);
    ACT_CHECK(((acp_uint8_t *)&(sAlign.mDWord))[2] == 0x03);
    ACT_CHECK(((acp_uint8_t *)&(sAlign.mDWord))[3] == 0x04);

    ACT_CHECK(((acp_uint8_t *)&(sAlign.mQWord))[0] == 0x01);
    ACT_CHECK(((acp_uint8_t *)&(sAlign.mQWord))[1] == 0x02);
    ACT_CHECK(((acp_uint8_t *)&(sAlign.mQWord))[2] == 0x03);
    ACT_CHECK(((acp_uint8_t *)&(sAlign.mQWord))[3] == 0x04);
    ACT_CHECK(((acp_uint8_t *)&(sAlign.mQWord))[4] == 0x05);
    ACT_CHECK(((acp_uint8_t *)&(sAlign.mQWord))[5] == 0x06);
    ACT_CHECK(((acp_uint8_t *)&(sAlign.mQWord))[6] == 0x07);
    ACT_CHECK(((acp_uint8_t *)&(sAlign.mQWord))[7] == 0x08);

    /* ------------------------------------------------
     * Single Argument API test
     * ----------------------------------------------*/
    
    sInt16 = 0xFEDC;
    sInt32 = 0xFEDCBA09;
    sInt64 = ACP_SINT64_LITERAL(0xFEDCBA9876543210);

    ACT_CHECK(ACP_TOH_BYTE2_ARG(ACP_TON_BYTE2_ARG(sInt16)) == sInt16);
    ACT_CHECK(ACP_TOH_BYTE4_ARG(ACP_TON_BYTE4_ARG(sInt32)) == sInt32);
    ACT_CHECK(ACP_TOH_BYTE8_ARG(ACP_TON_BYTE8_ARG(sInt64)) == sInt64);
    
    
    ACT_TEST_END();

    return 0;
}
