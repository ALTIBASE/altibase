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
 * $Id: testAciTypes.c 11251 2010-06-14 06:44:26Z denisb $
 ******************************************************************************/

#include <act.h>
#include <acl.h>
#include <acpCStr.h>
#include <acpList.h>
#include <aciTypes.h>

#define TEST_SIZE 10
#define TEST_BUFFER_SIZE 256

typedef struct testAciInt32
{
    acp_char_t   *mFormat;
    acp_uint32_t  mInt;
    acp_char_t   *mStrRes; 

} testAciInt32;

typedef struct testAciPtr
{
    acp_char_t   *mFormat;
    void         *mPtr;
    acp_char_t   *mStrRes;

} testAciPtr;

typedef struct testAciInt64
{
    acp_char_t   *mFormat;
    acp_uint64_t mInt;
    acp_char_t   *mStrRes;

} testAciInt64;

testAciInt32 gInt32Array[] = { {"%"ACI_UINT32_FMT, 5, "5"}, 
                                   {"%"ACI_INT32_FMT, -5, "-5"}, 
                                   {"%"ACI_xINT32_FMT, 0xF, "f"},
                                   {"%"ACI_XINT32_FMT, 0xF, "F"}
                               };

testAciPtr gPtrArray[] = { {"%"ACI_POINTER_FMT,  (void *)0xFFFF, "65535"},
                               {"%"ACI_UPOINTER_FMT, (void *)0xFFFF, "65535"},
                               {"%"ACI_xPOINTER_FMT, (void *)0xFFFF, "ffff"},
                               {"%"ACI_XPOINTER_FMT, (void *)0xFFFF, "FFFF"}
                               };

testAciInt64 gInt64Array[] = {
    {"%"ACI_UINT64_FMT, ACP_UINT64_MAX, "18446744073709551615"},
    {"%"ACI_INT64_FMT,  ACP_UINT64_MAX, "-1"},
    {"%"ACI_xINT64_FMT, ACP_UINT64_MAX, "ffffffffffffffff"},
    {"%"ACI_XINT64_FMT, ACP_UINT64_MAX, "FFFFFFFFFFFFFFFF"}
};

#if defined(ACP_CFG_COMPILE_64BIT)

typedef testAciInt64 testVariantType;

testVariantType gVariantArray[] = {
    {"%"ACI_vULONG_FMT, ACP_UINT64_MAX, "18446744073709551615"},
    {"%"ACI_vxULONG_FMT, ACP_UINT64_MAX, "ffffffffffffffff"},
    {"%"ACI_vXULONG_FMT,ACP_UINT64_MAX,  "FFFFFFFFFFFFFFFF"},
    {"%"ACI_vSLONG_FMT,ACP_UINT64_MAX, "-1"},
    {"%"ACI_vxSLONG_FMT,ACP_UINT64_MAX, "ffffffffffffffff"},
    {"%"ACI_vXSLONG_FMT,ACP_UINT64_MAX, "FFFFFFFFFFFFFFFF"}
};

#else

typedef testAciInt32 testVariantType;

testVariantType gVariantArray[] = {
    {"%"ACI_vULONG_FMT,  ACP_UINT32_MAX, "4294967295"},
    {"%"ACI_vxULONG_FMT, ACP_UINT32_MAX, "ffffffff"},
    {"%"ACI_vXULONG_FMT, ACP_UINT32_MAX, "FFFFFFFF"},
    {"%"ACI_vSLONG_FMT,  ACP_UINT32_MAX, "-1"},
    {"%"ACI_vxSLONG_FMT, ACP_UINT32_MAX, "ffffffff"},
    {"%"ACI_vXSLONG_FMT, ACP_UINT32_MAX, "FFFFFFFF"}
};
#endif


void testAciTypes(void)
{
    acp_rc_t sRC;
    acp_char_t sArray[TEST_SIZE];
    acp_char_t    sBuffer[TEST_BUFFER_SIZE] = {0};
    acp_uint32_t sSize = 0;
    acp_uint32_t i = 0;

    /* Test */
    sSize = (acp_uint32_t)(sizeof(gInt32Array) / sizeof(testAciInt32));


    for (i = 0; i < sSize; i++)
    {
        sRC = acpSnprintf(sBuffer,                     
                          sizeof(sBuffer),             
                          gInt32Array[i].mFormat,     
                          gInt32Array[i].mInt);

        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        ACT_CHECK(acpCStrCmp(sBuffer, gInt32Array[i].mStrRes, 256) == 0);
    }

    sSize = (acp_uint32_t)(sizeof(gPtrArray) / sizeof(testAciPtr));

 
    for (i = 0; i < sSize; i++)
    {
        sRC = acpSnprintf(sBuffer,
                          sizeof(sBuffer),
                          gPtrArray[i].mFormat,
                          gPtrArray[i].mPtr);
     
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        ACT_CHECK(acpCStrCmp(sBuffer, gPtrArray[i].mStrRes, 256) == 0);
    }

    sSize = (acp_uint32_t)(sizeof(gInt64Array) / sizeof(testAciInt64));
        
    for (i = 0; i < sSize; i++)
    {
        sRC = acpSnprintf(sBuffer,
                          sizeof(sBuffer),
                          gInt64Array[i].mFormat, 
                          gInt64Array[i].mInt);
                               
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        ACT_CHECK(acpCStrCmp(sBuffer, gInt64Array[i].mStrRes, 256) == 0); 
    }

    sSize = (acp_uint32_t)(sizeof(gVariantArray) / sizeof(testVariantType));
        
    for (i = 0; i < sSize; i++)
    {
        sRC = acpSnprintf(sBuffer,
                          sizeof(sBuffer),
                          gVariantArray[i].mFormat, 
                          gVariantArray[i].mInt);
 
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        ACT_CHECK(acpCStrCmp(sBuffer, gVariantArray[i].mStrRes, 256) == 0); 
    }



    /* Test types */
    ACT_CHECK(ACI_SUCCESS == 0);
    ACT_CHECK(ACI_SUCCESS != ACI_FAILURE);
    ACT_CHECK(ACI_SUCCESS != ACI_CM_STOP);

    /* Test sizeof */
    ACT_CHECK(ACI_SIZEOF(acp_char_t) == (acp_uint32_t)sizeof(acp_char_t));

    sSize = ACI_SIZEOF(sArray) / ACI_SIZEOF(acp_char_t);
    ACT_CHECK(sSize == TEST_SIZE);
}

acp_sint32_t main(acp_sint32_t aArgc, acp_char_t** aArgv)
{

    ACP_UNUSED(aArgc);
    ACP_UNUSED(aArgv);

    ACT_TEST_BEGIN();

    testAciTypes();

    ACT_TEST_END();

    return 0;
}
