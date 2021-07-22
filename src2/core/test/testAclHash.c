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
 * $Id: testAclHash.c 7482 2009-09-08 06:20:11Z djin $
 ******************************************************************************/

#include <act.h>
#include <aceAssert.h>
#include <aclHash.h>


typedef struct testObject
{
    acp_sint32_t        mInt32;
    acp_sint64_t        mInt64;
    const acp_char_t*   mStr;
    const acp_uint32_t  mPos;
} testObject;


static testObject gTestObj[] =
{
    {
        1,
        1,
        "alpha", 0
    },
    {
        2,
        2,
        "bravo", 1
    },
    {
        3,
        3,
        "charlie", 0
    },
    {
        4,
        4,
        "delta", 2
    },
    {
        5,
        5,
        "echo", 0
    },
    {
        6,
        6,
        "foxtrot", 3
    },
    {
        7,
        7,
        "golf", 1
    },
    {
        8,
        8,
        "hotel", 0
    },
    {
        9,
        9,
        "india", 1
    },
    {
        10,
        10,
        "juliet", 1
    },
    {
        0,
        0,
        NULL,
        0
    }
};


static void testPrintHashStat(acl_hash_table_t *aHashTable)
{
#if 0
    acl_hash_table_stat_t sStat;

    aclHashStat(aHashTable, &sStat);

    (void)acpPrintf("{ %u, %u, %u }\n",
                    sStat.mRecordCount,
                    sStat.mDistribution,
                    sStat.mDeviation);
#else
    ACP_UNUSED(aHashTable);
#endif
}

static void testHashBasic(void)
{
    acl_hash_table_t sHashTable;
    acp_rc_t         sRC;

    sRC = aclHashCreate(&sHashTable,
                        11,
                        (acp_size_t)ACP_SINT32_MAX + 1,
                        aclHashHashInt32,
                        aclHashCompInt32,
                        ACP_TRUE);
    ACT_CHECK(ACP_RC_IS_EINVAL(sRC));
}

static void testHashTraverse(acl_hash_table_t *aHashTable,
                             acp_sint32_t      aCount,
                             acp_bool_t        aIsCut)
{
    acp_bool_t           sVisit[100];
    acl_hash_traverse_t  sHashTraverse;
    void                *sValue;
    acp_rc_t             sRC;
    acp_sint32_t         sCount = 0;
    acp_sint32_t         i;

    ACE_ASSERT(aCount <= (acp_sint32_t)(sizeof(sVisit) / sizeof(acp_bool_t)));

    acpMemSet(sVisit, 0, sizeof(sVisit));

    sRC = aclHashTraverseOpen(&sHashTraverse, aHashTable, aIsCut);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    while (1)
    {
        sRC = aclHashTraverseNext(&sHashTraverse, (void **)&sValue);

        if (ACP_RC_IS_EOF(sRC))
        {
            break;
        }
        else
        {
            /* do nothing */
        }

        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        sCount++;

        for (i = 0; i < aCount; i++)
        {
            if (sValue == &gTestObj[i])
            {
                ACT_CHECK_DESC(sVisit[i] == 0,
                               ("already traversed node returned at %d/%d",
                                sCount,
                                aCount));
                sVisit[i] = 1;
            }
            else
            {
                /* continue */
            }
        }
    }

    aclHashTraverseClose(&sHashTraverse);

    ACT_CHECK_DESC(sCount == aCount,
                   ("number of records in hash table should be %d but %d",
                    aCount,
                    sCount));
}

static void testHashInt32(void)
{
    acl_hash_table_t  sHashTable;
    testObject       *sValue;
    acp_bool_t        sIsEmpty;
    acp_rc_t          sRC;
    acp_uint32_t      sTotalRecord = 0;
    acp_uint32_t      i;

    sRC = aclHashCreate(&sHashTable,
                        11,
                        sizeof(acp_sint32_t),
                        aclHashHashInt32,
                        aclHashCompInt32,
                        ACP_TRUE);

    for (i = 0; gTestObj[i].mStr != NULL; i++)
    {
        sRC = aclHashFind(&sHashTable, &gTestObj[i].mInt32, (void **)&sValue);
        ACT_CHECK(ACP_RC_IS_ENOENT(sRC));
    }

    sIsEmpty = aclHashIsEmpty(&sHashTable);
    ACT_CHECK(sIsEmpty == ACP_TRUE);

    sTotalRecord = aclHashGetTotalRecordCount(&sHashTable);
    ACT_CHECK(sTotalRecord == 0);

    for (i = 0; gTestObj[i].mStr != NULL; i++)
    {
        sRC = aclHashAdd(&sHashTable, &gTestObj[i].mInt32, &gTestObj[i]);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        testHashTraverse(&sHashTable, i + 1, ACP_FALSE);
    }

    sIsEmpty = aclHashIsEmpty(&sHashTable);
    ACT_CHECK(sIsEmpty == ACP_FALSE);

    sTotalRecord = aclHashGetTotalRecordCount(&sHashTable);
    ACT_CHECK(sTotalRecord == i);

    testPrintHashStat(&sHashTable);

    for (i = 0; gTestObj[i].mStr != NULL; i++)
    {
        sRC = aclHashFind(&sHashTable, &gTestObj[i].mInt32, (void **)&sValue);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        ACT_CHECK(sValue == &gTestObj[i]);
    }

    for (i = 0; gTestObj[i].mStr != NULL; i++)
    {
        sRC = aclHashAdd(&sHashTable, &gTestObj[i].mInt32, &gTestObj[i]);
        ACT_CHECK(ACP_RC_IS_EEXIST(sRC));
    }

    for (i = 0; gTestObj[i].mStr != NULL; i++)
    {
        sRC = aclHashFind(&sHashTable, &gTestObj[i].mInt32, (void **)&sValue);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        ACT_CHECK(sValue == &gTestObj[i]);
    }

    for (i = 0; gTestObj[i].mStr != NULL; i++)
    {
        sRC = aclHashRemove(&sHashTable, &gTestObj[i].mInt32, (void **)&sValue);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        ACT_CHECK(sValue == &gTestObj[i]);
    }

    for (i = 0; gTestObj[i].mStr != NULL; i++)
    {
        sRC = aclHashRemove(&sHashTable, &gTestObj[i].mInt32, (void **)&sValue);
        ACT_CHECK(ACP_RC_IS_ENOENT(sRC));
    }

    for (i = 0; gTestObj[i].mStr != NULL; i++)
    {
        sRC = aclHashAdd(&sHashTable, &gTestObj[i].mInt32, &gTestObj[i]);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    testHashTraverse(&sHashTable, i, ACP_TRUE);

    aclHashDestroy(&sHashTable);
}

static void testHashInt64(void)
{
    acl_hash_table_t  sHashTable;
    testObject       *sValue;
    acp_bool_t        sIsEmpty;
    acp_rc_t          sRC;
    acp_uint32_t      sTotalRecord = 0;
    acp_uint32_t      i;

    sRC = aclHashCreate(&sHashTable,
                        11,
                        sizeof(acp_sint64_t),
                        aclHashHashInt64,
                        aclHashCompInt64,
                        ACP_TRUE);

    for (i = 0; gTestObj[i].mStr != NULL; i++)
    {
        sRC = aclHashFind(&sHashTable, &gTestObj[i].mInt64, (void **)&sValue);
        ACT_CHECK(ACP_RC_IS_ENOENT(sRC));
    }

    sIsEmpty = aclHashIsEmpty(&sHashTable);
    ACT_CHECK(sIsEmpty == ACP_TRUE);

    sTotalRecord = aclHashGetTotalRecordCount(&sHashTable);
    ACT_CHECK(sTotalRecord == 0);

    for (i = 0; gTestObj[i].mStr != NULL; i++)
    {
        sRC = aclHashAdd(&sHashTable, &gTestObj[i].mInt64, &gTestObj[i]);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        testHashTraverse(&sHashTable, i + 1, ACP_FALSE);
    }

    sIsEmpty = aclHashIsEmpty(&sHashTable);
    ACT_CHECK(sIsEmpty == ACP_FALSE);

    sTotalRecord = aclHashGetTotalRecordCount(&sHashTable);
    ACT_CHECK(sTotalRecord == i);

    testPrintHashStat(&sHashTable);

    for (i = 0; gTestObj[i].mStr != NULL; i++)
    {
        sRC = aclHashFind(&sHashTable, &gTestObj[i].mInt64, (void **)&sValue);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        ACT_CHECK(sValue == &gTestObj[i]);
    }

    for (i = 0; gTestObj[i].mStr != NULL; i++)
    {
        sRC = aclHashAdd(&sHashTable, &gTestObj[i].mInt64, &gTestObj[i]);
        ACT_CHECK(ACP_RC_IS_EEXIST(sRC));
    }

    for (i = 0; gTestObj[i].mStr != NULL; i++)
    {
        sRC = aclHashFind(&sHashTable, &gTestObj[i].mInt64, (void **)&sValue);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        ACT_CHECK(sValue == &gTestObj[i]);
    }

    for (i = 0; gTestObj[i].mStr != NULL; i++)
    {
        sRC = aclHashRemove(&sHashTable, &gTestObj[i].mInt64, (void **)&sValue);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        ACT_CHECK(sValue == &gTestObj[i]);
    }

    for (i = 0; gTestObj[i].mStr != NULL; i++)
    {
        sRC = aclHashRemove(&sHashTable, &gTestObj[i].mInt64, (void **)&sValue);
        ACT_CHECK(ACP_RC_IS_ENOENT(sRC));
    }

    for (i = 0; gTestObj[i].mStr != NULL; i++)
    {
        sRC = aclHashAdd(&sHashTable, &gTestObj[i].mInt64, &gTestObj[i]);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    testHashTraverse(&sHashTable, i, ACP_TRUE);

    aclHashDestroy(&sHashTable);
}

static void testHashStr(void)
{
    acl_hash_table_t  sHashTable;
    testObject       *sValue;
    acp_rc_t          sRC;
    acp_sint32_t      i;

    sRC = aclHashCreate(&sHashTable,
                        11,
                        10,
                        aclHashHashCString,
                        aclHashCompCString,
                        ACP_TRUE);

    for (i = 0; gTestObj[i].mStr != NULL; i++)
    {
        sRC = aclHashFind(&sHashTable,
                          (void *)(gTestObj[i].mStr + gTestObj[i].mPos),
                          (void **)&sValue);
        ACT_CHECK(ACP_RC_IS_ENOENT(sRC));
    }

    for (i = 0; gTestObj[i].mStr != NULL; i++)
    {
        sRC = aclHashAdd(&sHashTable,
                         (void *)(gTestObj[i].mStr + gTestObj[i].mPos),
                         &gTestObj[i]);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        testHashTraverse(&sHashTable, i + 1, ACP_FALSE);
    }

    testPrintHashStat(&sHashTable);

    for (i = 0; gTestObj[i].mStr != NULL; i++)
    {
        sRC = aclHashFind(&sHashTable,
                          (void *)(gTestObj[i].mStr + gTestObj[i].mPos),
                          (void **)&sValue);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        ACT_CHECK(sValue == &gTestObj[i]);
    }

    for (i = 0; gTestObj[i].mStr != NULL; i++)
    {
        sRC = aclHashAdd(&sHashTable,
                          (void *)(gTestObj[i].mStr + gTestObj[i].mPos),
                         &gTestObj[i]);
        ACT_CHECK(ACP_RC_IS_EEXIST(sRC));
    }

    for (i = 0; gTestObj[i].mStr != NULL; i++)
    {
        sRC = aclHashFind(&sHashTable,
                          (void *)(gTestObj[i].mStr + gTestObj[i].mPos),
                          (void **)&sValue);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        ACT_CHECK(sValue == &gTestObj[i]);
    }

    for (i = 0; gTestObj[i].mStr != NULL; i++)
    {
        sRC = aclHashRemove(&sHashTable,
                          (void *)(gTestObj[i].mStr + gTestObj[i].mPos),
                            (void **)&sValue);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        ACT_CHECK(sValue == &gTestObj[i]);
    }

    for (i = 0; gTestObj[i].mStr != NULL; i++)
    {
        sRC = aclHashRemove(&sHashTable,
                          (void *)(gTestObj[i].mStr + gTestObj[i].mPos),
                            (void **)&sValue);
        ACT_CHECK(ACP_RC_IS_ENOENT(sRC));
    }

    for (i = 0; gTestObj[i].mStr != NULL; i++)
    {
        sRC = aclHashAdd(&sHashTable,
                          (void *)(gTestObj[i].mStr + gTestObj[i].mPos),
                         &gTestObj[i]);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    testHashTraverse(&sHashTable, i, ACP_TRUE);

    aclHashDestroy(&sHashTable);
}

/* BUG-22405, unexpeced operation of 
 * aclHashIsEmpty and aclHashGetTotalRecordCount */
static void testHashIsEmpty(void)
{
    acl_hash_table_t     sHashTable;
    acl_hash_traverse_t  sHashTraverse;
    testObject          *sValue;
    acp_rc_t             sRC;
    acp_sint32_t         i;

    sRC = aclHashCreate(&sHashTable,
                        11,
                        2,
                        aclHashHashCString,
                        aclHashCompCString,
                        ACP_TRUE);

    for (i = 0; gTestObj[i].mStr != NULL; i++)
    {
        sRC = aclHashFind(&sHashTable,
                          (void *)(gTestObj[i].mStr + gTestObj[i].mPos),
                          (void **)&sValue);
        ACT_CHECK(ACP_RC_IS_ENOENT(sRC));
    }

    for (i = 0; gTestObj[i].mStr != NULL; i++)
    {
        sRC = aclHashAdd(&sHashTable,
                         (void *)(gTestObj[i].mStr + gTestObj[i].mPos),
                         &gTestObj[i]);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    sRC = aclHashTraverseOpen(&sHashTraverse, &sHashTable, ACP_TRUE);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    while(1)
    {
        sRC = aclHashTraverseNext(&sHashTraverse, (void **)&sValue);

        if (ACP_RC_IS_EOF(sRC))
        {
            break;
        }
        else
        {
            /* do nothing */
        }
    }

    aclHashTraverseClose(&sHashTraverse);

    ACT_CHECK_DESC(aclHashIsEmpty(&sHashTable) == ACP_TRUE,
                   ("final count should 0 but %u",
                    aclHashGetTotalRecordCount(&sHashTable)));
}

static void testHashFuncs(void)
{
    acp_char_t*         sTestString =
        "Aveverumcorpusnatumdemariavirgine"
        "Verepassumimmolarumincruceprohomine"
        "Cuiuslatusperforatumfuxitaquaetsanguine"
        "Estonobispraegustatummortiinsexamine"
        "OIesudulcisoIesupieoIesufilimariae";
    const acp_uint32_t  sLen = (acp_uint32_t)acpCStrLen(sTestString, 1000);

    acp_uint32_t        i;
    acp_uint32_t        j;

    for(i = 0; i < sLen; i++)
    {
        for(j = 1; j < sLen - i; j++)
        {
            /* To check there is no SIGBUS */
            (void)aclHashHashCString(sTestString + i, j);
        }
    }
}

acp_sint32_t main(void)
{
    ACT_TEST_BEGIN();

    testHashBasic();
    testHashInt32();
    testHashInt64();
    testHashStr();
    testHashIsEmpty();

    testHashFuncs();

    ACT_TEST_END();

    return 0;
}
