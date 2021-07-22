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
 * $Id: testAcpAtomic.c 5849 2009-06-02 05:11:22Z jykim $
 ******************************************************************************/

#include <act.h>
#include <acpAtomic.h>

#define TEST_VAL16_0   0
#define TEST_VAL16_1   1
#define TEST_VAL16_100 100
#define TEST_VAL16_150 150
#define TEST_VAL16_200 200

#define TEST_VAL32_0   0
#define TEST_VAL32_1   1
#define TEST_VAL32_100 100
#define TEST_VAL32_150 150
#define TEST_VAL32_200 200

#define TEST_VAL64_0   ACP_SINT64_LITERAL(0)
#define TEST_VAL64_1   ACP_SINT64_LITERAL(1)
#define TEST_VAL64_100 ((ACP_SINT64_LITERAL(100) << 32) + 1000)
#define TEST_VAL64_150 ((ACP_SINT64_LITERAL(150) << 32) + 1000)
#define TEST_VAL64_200 ((ACP_SINT64_LITERAL(200) << 32) + 2000)

typedef struct testObjectNode
{
    acp_sint32_t mNodeVal1;
    acp_uint32_t mNodeVal2;
} testObjectNode;

typedef struct testObject
{
    acp_sint32_t    mVal1;
    acp_uint32_t    mVal2;
    testObjectNode *mNode;
} testObject;

static void testAtomic32(volatile acp_uint32_t *aAtomic)
{
    *aAtomic = TEST_VAL32_0;

    /*
     * set
     */
    (void)acpAtomicSet32(aAtomic, TEST_VAL32_1);
    ACT_CHECK(acpAtomicGet32(aAtomic) == TEST_VAL32_1);

    /*
     * set
     */
    ACT_CHECK(acpAtomicSet32(aAtomic, TEST_VAL32_100) == TEST_VAL32_1);
    ACT_CHECK(acpAtomicGet32(aAtomic) == TEST_VAL32_100);

    /*
     * increase
     */
    ACT_CHECK(acpAtomicInc32(aAtomic) == TEST_VAL32_100 + 1);
    ACT_CHECK(acpAtomicGet32(aAtomic) == TEST_VAL32_100 + 1);

    /*
     * decrease
     */
    ACT_CHECK(acpAtomicDec32(aAtomic) == TEST_VAL32_100);
    ACT_CHECK(acpAtomicGet32(aAtomic) == TEST_VAL32_100);

    /*
     * add
     */
    ACT_CHECK(acpAtomicAdd32(aAtomic, TEST_VAL32_100) == TEST_VAL32_200);
    ACT_CHECK(acpAtomicGet32(aAtomic) == TEST_VAL32_200);

    /*
     * subtract
     */
    ACT_CHECK(acpAtomicSub32(aAtomic, TEST_VAL32_100) == TEST_VAL32_100);
    ACT_CHECK(acpAtomicGet32(aAtomic) == TEST_VAL32_100);

    /*
     * compare and swap
     */
    ACT_CHECK(acpAtomicCas32(aAtomic,
                             TEST_VAL32_150,
                             TEST_VAL32_200) == TEST_VAL32_100);
    ACT_CHECK(acpAtomicGet32(aAtomic) == TEST_VAL32_100);

    ACT_CHECK(acpAtomicCas32(aAtomic,
                             TEST_VAL32_200,
                             TEST_VAL32_100) == TEST_VAL32_100);
    ACT_CHECK(acpAtomicGet32(aAtomic) == TEST_VAL32_200);

    ACT_CHECK(acpAtomicCas32Bool(aAtomic,
                                 TEST_VAL32_200,
                                 TEST_VAL32_100) == ACP_FALSE);
    ACT_CHECK(acpAtomicGet32(aAtomic) == TEST_VAL32_200);

    ACT_CHECK(acpAtomicCas32Bool(aAtomic,
                                 TEST_VAL32_100,
                                 TEST_VAL32_200) == ACP_TRUE);
    ACT_CHECK(acpAtomicGet32(aAtomic) == TEST_VAL32_100);
}

static void testAtomic64(volatile acp_uint64_t *aAtomic)
{
    /*
     * set
     */
    *aAtomic = TEST_VAL64_0;

    (void)acpAtomicSet64(aAtomic, TEST_VAL64_1);
    ACT_CHECK(acpAtomicGet64(aAtomic) == TEST_VAL64_1);

    /*
     * set
     */
    ACT_CHECK(acpAtomicSet64(aAtomic, TEST_VAL64_100) == TEST_VAL64_1);
    ACT_CHECK(acpAtomicGet64(aAtomic) == TEST_VAL64_100);

    /*
     * increase
     */
    ACT_CHECK(acpAtomicInc64(aAtomic) == TEST_VAL64_100 + 1);
    ACT_CHECK(acpAtomicGet64(aAtomic) == TEST_VAL64_100 + 1);

    /*
     * decrease
     */
    ACT_CHECK(acpAtomicDec64(aAtomic) == TEST_VAL64_100);
    ACT_CHECK(acpAtomicGet64(aAtomic) == TEST_VAL64_100);

    /*
     * add
     */
    ACT_CHECK(acpAtomicAdd64(aAtomic, TEST_VAL64_100) == TEST_VAL64_200);
    ACT_CHECK(acpAtomicGet64(aAtomic) == TEST_VAL64_200);

    /*
     * subtract
     */
    ACT_CHECK(acpAtomicSub64(aAtomic, TEST_VAL64_100) == TEST_VAL64_100);
    ACT_CHECK(acpAtomicGet64(aAtomic) == TEST_VAL64_100);

    /*
     * compare and swap
     */
    ACT_CHECK(acpAtomicCas64(aAtomic,
                             TEST_VAL64_150,
                             TEST_VAL64_200) == TEST_VAL64_100);
    ACT_CHECK(acpAtomicGet64(aAtomic) == TEST_VAL64_100);

    ACT_CHECK(acpAtomicCas64(aAtomic,
                             TEST_VAL64_200,
                             TEST_VAL64_100) == TEST_VAL64_100);
    ACT_CHECK(acpAtomicGet64(aAtomic) == TEST_VAL64_200);

    ACT_CHECK(acpAtomicCas64Bool(aAtomic,
                                    TEST_VAL64_200,
                                    TEST_VAL64_100) == ACP_FALSE);
    ACT_CHECK(acpAtomicGet64(aAtomic) == TEST_VAL64_200);

    ACT_CHECK(acpAtomicCas64Bool(aAtomic,
                                 TEST_VAL64_100,
                                 TEST_VAL64_200) == ACP_TRUE);
    ACT_CHECK(acpAtomicGet64(aAtomic) == TEST_VAL64_100);
}

static void testAtomic(volatile acp_ulong_t *aAtomic)
{
    /*
     * set
     */
    (void)acpAtomicSet(aAtomic, 1);
    ACT_CHECK(acpAtomicGet(aAtomic) == 1);

    /*
     * set
     */
    ACT_CHECK(acpAtomicSet(aAtomic, 100) == 1);
    ACT_CHECK(acpAtomicGet(aAtomic) == 100);

    /*
     * increase
     */
    ACT_CHECK(acpAtomicInc(aAtomic) == 101);
    ACT_CHECK(acpAtomicGet(aAtomic) == 101);

    /*
     * decrease
     */
    ACT_CHECK(acpAtomicDec(aAtomic) == 100);
    ACT_CHECK(acpAtomicGet(aAtomic) == 100);

    /*
     * add
     */
    ACT_CHECK(acpAtomicAdd(aAtomic, 100) == 200);
    ACT_CHECK(acpAtomicGet(aAtomic) == 200);

    /*
     * subtract
     */
    ACT_CHECK(acpAtomicSub(aAtomic, 100) == 100);
    ACT_CHECK(acpAtomicGet(aAtomic) == 100);

    /*
     * compare and swap
     */
    ACT_CHECK(acpAtomicCas(aAtomic, 100, 200) == 100);
    ACT_CHECK(acpAtomicGet(aAtomic) == 100);

    ACT_CHECK(acpAtomicCas(aAtomic, 200, 100) == 100);
    ACT_CHECK(acpAtomicGet(aAtomic) == 200);

    ACT_CHECK(acpAtomicCasBool(aAtomic,
                               200,
                               100) == ACP_FALSE);
    ACT_CHECK(acpAtomicGet(aAtomic) == 200);

    ACT_CHECK(acpAtomicCasBool(aAtomic,
                               100,
                               200) == ACP_TRUE);
    ACT_CHECK(acpAtomicGet(aAtomic) == 100);
}

static void testAtomicPointer(testObject *aObj)
{
    testObjectNode *sCmp;
    testObjectNode *sWith;
    acp_rc_t        sRC;

    /* value range test1 */
    aObj->mNode = (testObjectNode *)0x40000000;
    sWith       = (testObjectNode *)0xFFFFFFFF;

    sCmp = aObj->mNode;

    (void)acpAtomicCas(&aObj->mNode, (acp_ulong_t)sWith, (acp_ulong_t)sCmp);

    ACT_CHECK_DESC(aObj->mNode == sWith,
                   (("aObj->mNode(%p) and sWith(%p) should be same, "
                     "but different"), aObj->mNode, sWith));

    /* value range test2 */
    aObj->mNode = (testObjectNode *)0x3FFFFFFF;
    sWith       = (testObjectNode *)0xFFFFFFFF;

    sCmp = aObj->mNode;

    (void)acpAtomicCas(&aObj->mNode, (acp_ulong_t)sWith, (acp_ulong_t)sCmp);

    ACT_CHECK_DESC(aObj->mNode == sWith,
                   (("aObj->mNode(%p) and sWith(%p) should be same, "
                     "but different"), aObj->mNode, sWith));

    /* value range test3 */
    aObj->mNode = (testObjectNode *)0xFFFFFFFF;
    sWith       = (testObjectNode *)0x00000000;

    sCmp = aObj->mNode;

    (void)acpAtomicCas(&aObj->mNode, (acp_ulong_t)sWith, (acp_ulong_t)sCmp);

    ACT_CHECK_DESC(aObj->mNode == sWith,
                   (("aObj->mNode(%p) and sWith(%p) should be same, "
                     "but different"), aObj->mNode, sWith));

    /* value range test4 */
    sRC = acpMemAlloc((void**)&aObj->mNode, sizeof(testObjectNode));
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpMemAlloc((void**)&sWith, sizeof(testObjectNode));
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sCmp = aObj->mNode;

    (void)acpAtomicCas(&aObj->mNode, (acp_ulong_t)sWith, (acp_ulong_t)sCmp);

    ACT_CHECK_DESC(aObj->mNode == sWith,
                   (("aObj->mNode(%p) and sWith(%p) should be same, "
                     "but different"), aObj->mNode, sWith));

    if (aObj->mNode == sWith)
    {
        acpMemFree(sCmp);
    }
    else
    {
        acpMemFree(aObj->mNode);
    }

    acpMemFree(sWith);
}

acp_sint32_t main(void)
{
    volatile acp_uint32_t sAtomic32;
    volatile acp_uint64_t sAtomic64;
    volatile acp_ulong_t  sAtomic;
    testObject sObj;

    ACT_TEST_BEGIN();

    /* test integer values */
    testAtomic32(&sAtomic32);
    testAtomic64(&sAtomic64);
    testAtomic(&sAtomic);

    /* BUG-22243: acpAtomicCas32 at release mode cause an aclQueue hang.
     * test pointers */
    testAtomicPointer(&sObj);

    ACT_TEST_END();

    return 0;
}
