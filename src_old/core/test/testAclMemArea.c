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
 * $Id: testAclMemArea.c 9884 2010-02-03 06:50:04Z gurugio $
 ******************************************************************************/

#include <act.h>
#include <aclMemArea.h>


#define TEST_ALLOC_COUNT 10

#if defined(ACP_CFG_MEMORY_CHECK)

#define TEST_MEM_AREA_CHECK_STAT(aMemArea)

#else

#if 1
#define TEST_MEM_AREA_CHECK_STAT(aMemArea)                                     \
    do                                                                         \
    {                                                                          \
        acl_mem_area_stat_t sStat_MACRO_LOCAL_VAR;                          \
                                                                            \
        aclMemAreaStat((aMemArea), &sStat_MACRO_LOCAL_VAR);                 \
                                                                            \
        ACT_CHECK_DESC((gTestMemAreaStat[gTestCheckStatIndex].mChunkCount == \
                        sStat_MACRO_LOCAL_VAR.mChunkCount) &&               \
                       (gTestMemAreaStat[gTestCheckStatIndex].mTotalSize == \
                        sStat_MACRO_LOCAL_VAR.mTotalSize) &&                \
                       (gTestMemAreaStat[gTestCheckStatIndex].mAllocSize == \
                        sStat_MACRO_LOCAL_VAR.mAllocSize),                  \
                       ("mem area stat should be "                          \
                        "{ %u, %zd, %zd } but { %u, %zd, %zd } at %d",      \
                        gTestMemAreaStat[gTestCheckStatIndex].mChunkCount,  \
                        gTestMemAreaStat[gTestCheckStatIndex].mTotalSize,   \
                        gTestMemAreaStat[gTestCheckStatIndex].mAllocSize,   \
                        sStat_MACRO_LOCAL_VAR.mChunkCount,                  \
                        sStat_MACRO_LOCAL_VAR.mTotalSize,                   \
                        sStat_MACRO_LOCAL_VAR.mAllocSize,                   \
                        gTestCheckStatIndex));                              \
                                                                               \
        gTestCheckStatIndex++;                                                 \
    } while (0)
#else
#define TEST_MEM_AREA_CHECK_STAT(aMemArea)                      \
    do                                                          \
    {                                                           \
        acl_mem_area_stat_t sStat_MACRO_LOCAL_VAR;              \
                                                                \
        aclMemAreaStat((aMemArea), &sStat_MACRO_LOCAL_VAR);     \
                                                                \
        (void)acpPrintf("{\n    %u,\n    %zd,\n    %zd\n},\n",  \
                        sStat_MACRO_LOCAL_VAR.mChunkCount,      \
                        sStat_MACRO_LOCAL_VAR.mTotalSize,       \
                        sStat_MACRO_LOCAL_VAR.mAllocSize);      \
    } while (0)
#endif


static acl_mem_area_stat_t gTestMemAreaStat[] =
{
    {
        0,
        0,
        0
    },
    {
        1,
        8,
        8
    },
    {
        2,
        16,
        16
    },
    {
        3,
        24,
        24
    },
    {
        4,
        32,
        32
    },
    {
        5,
        40,
        40
    },
    {
        6,
        48,
        48
    },
    {
        7,
        56,
        56
    },
    {
        8,
        64,
        64
    },
    {
        9,
        72,
        72
    },
    {
        10,
        80,
        80
    },
    {
        10,
        80,
        40
    },
    {
        10,
        80,
        48
    },
    {
        10,
        80,
        56
    },
    {
        10,
        80,
        64
    },
    {
        10,
        80,
        72
    },
    {
        10,
        80,
        80
    },
    {
        10,
        80,
        40
    },
    {
        5,
        40,
        40
    },
    {
        6,
        48,
        48
    },
    {
        7,
        56,
        56
    },
    {
        8,
        64,
        64
    },
    {
        9,
        72,
        72
    },
    {
        10,
        80,
        80
    },
    {
        10,
        80,
        0
    },
    {
        0,
        0,
        0
    },
    {
        1,
        24,
        8
    },
    {
        1,
        24,
        16
    },
    {
        1,
        24,
        24
    },
    {
        2,
        48,
        32
    },
    {
        2,
        48,
        40
    },
    {
        2,
        48,
        48
    },
    {
        3,
        72,
        56
    },
    {
        3,
        72,
        64
    },
    {
        3,
        72,
        72
    },
    {
        4,
        96,
        80
    },
    {
        4,
        96,
        40
    },
    {
        4,
        96,
        48
    },
    {
        4,
        96,
        56
    },
    {
        4,
        96,
        64
    },
    {
        4,
        96,
        72
    },
    {
        4,
        96,
        80
    },
    {
        4,
        96,
        40
    },
    {
        2,
        48,
        40
    },
    {
        2,
        48,
        48
    },
    {
        3,
        72,
        56
    },
    {
        3,
        72,
        64
    },
    {
        3,
        72,
        72
    },
    {
        4,
        96,
        80
    },
    {
        4,
        96,
        0
    },
    {
        0,
        0,
        0
    },
    {
        1,
        8,
        8
    },
    {
        2,
        16,
        16
    },
    {
        3,
        24,
        24
    },
    {
        4,
        32,
        32
    },
    {
        5,
        48,
        48
    },
    {
        6,
        80,
        80
    },
    {
        7,
        144,
        144
    },
    {
        8,
        272,
        272
    },
    {
        9,
        528,
        528
    },
    {
        10,
        1040,
        1040
    },
    {
        10,
        1040,
        48
    },
    {
        10,
        1040,
        80
    },
    {
        10,
        1040,
        144
    },
    {
        10,
        1040,
        272
    },
    {
        10,
        1040,
        528
    },
    {
        10,
        1040,
        1040
    },
    {
        10,
        1040,
        48
    },
    {
        5,
        48,
        48
    },
    {
        6,
        80,
        80
    },
    {
        7,
        144,
        144
    },
    {
        8,
        272,
        272
    },
    {
        9,
        528,
        528
    },
    {
        10,
        1040,
        1040
    },
    {
        10,
        1040,
        0
    },
    {
        0,
        0,
        0
    },
    {
        1,
        24,
        8
    },
    {
        1,
        24,
        16
    },
    {
        1,
        24,
        24
    },
    {
        2,
        48,
        40
    },
    {
        3,
        80,
        72
    },
    {
        4,
        144,
        136
    },
    {
        5,
        272,
        264
    },
    {
        6,
        528,
        520
    },
    {
        7,
        1040,
        1032
    },
    {
        8,
        2064,
        2056
    },
    {
        8,
        2064,
        72
    },
    {
        8,
        2064,
        136
    },
    {
        8,
        2064,
        264
    },
    {
        8,
        2064,
        520
    },
    {
        8,
        2064,
        1032
    },
    {
        8,
        2064,
        2056
    },
    {
        8,
        2064,
        72
    },
    {
        3,
        80,
        72
    },
    {
        4,
        144,
        136
    },
    {
        5,
        272,
        264
    },
    {
        6,
        528,
        520
    },
    {
        7,
        1040,
        1032
    },
    {
        8,
        2064,
        2056
    },
    {
        8,
        2064,
        0
    }
};

static acp_sint32_t gTestCheckStatIndex = 0;

#endif


static void testMemAreaFixedSize(acp_size_t aChunkSize)
{
    acl_mem_area_t           sMemArea;
    acl_mem_area_snapshot_t  sMemSnapshot;
    acp_char_t              *sAddr[TEST_ALLOC_COUNT];
    acp_size_t               sSize;
    acp_rc_t                 sRC;
    acp_sint32_t             i;

    if (aChunkSize == 0)
    {
        sSize = 1;
    }
    else
    {
        sSize = aChunkSize / 3;
    }

    aclMemAreaCreate(&sMemArea, aChunkSize);

    aclMemAreaGetSnapshot(&sMemArea, &sMemSnapshot);
    aclMemAreaFreeToSnapshot(&sMemArea, &sMemSnapshot);

    aclMemAreaShrink(&sMemArea);
    aclMemAreaFreeAll(&sMemArea);

    TEST_MEM_AREA_CHECK_STAT(&sMemArea);

    for (i = 0; i < TEST_ALLOC_COUNT; i++)
    {
        if (i == (TEST_ALLOC_COUNT / 2))
        {
            aclMemAreaGetSnapshot(&sMemArea, &sMemSnapshot);
        }
        else
        {
            /* do nothing */
        }

        sRC = aclMemAreaCalloc(&sMemArea, (void **)&sAddr[i], sSize);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        ACT_CHECK((sAddr[i] != NULL) && (((acp_ulong_t)sAddr[i] % 8) == 0));

        TEST_MEM_AREA_CHECK_STAT(&sMemArea);
    }

    aclMemAreaFreeToSnapshot(&sMemArea, &sMemSnapshot);
    TEST_MEM_AREA_CHECK_STAT(&sMemArea);

    for (i = (TEST_ALLOC_COUNT / 2); i < TEST_ALLOC_COUNT; i++)
    {
        sRC = aclMemAreaCalloc(&sMemArea, (void **)&sAddr[i], sSize);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        ACT_CHECK((sAddr[i] != NULL) && (((acp_ulong_t)sAddr[i] % 8) == 0));

        TEST_MEM_AREA_CHECK_STAT(&sMemArea);
    }

    aclMemAreaFreeToSnapshot(&sMemArea, &sMemSnapshot);
    TEST_MEM_AREA_CHECK_STAT(&sMemArea);

    aclMemAreaShrink(&sMemArea);
    TEST_MEM_AREA_CHECK_STAT(&sMemArea);

    for (i = (TEST_ALLOC_COUNT / 2); i < TEST_ALLOC_COUNT; i++)
    {
        sRC = aclMemAreaCalloc(&sMemArea, (void **)&sAddr[i], sSize);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        ACT_CHECK((sAddr[i] != NULL) && (((acp_ulong_t)sAddr[i] % 8) == 0));

        TEST_MEM_AREA_CHECK_STAT(&sMemArea);
    }

    aclMemAreaFreeAll(&sMemArea);
    TEST_MEM_AREA_CHECK_STAT(&sMemArea);

    aclMemAreaDestroy(&sMemArea);
}

static void testMemAreaDifferentSize(acp_size_t aChunkSize)
{
    acl_mem_area_t           sMemArea;
    acl_mem_area_snapshot_t  sMemSnapshot;
    acp_char_t              *sAddr[TEST_ALLOC_COUNT];
    acp_size_t               sSize[TEST_ALLOC_COUNT];
    acp_size_t               sCurSize;
    acp_rc_t                 sRC;
    acp_sint32_t             i;

    if (aChunkSize == 0)
    {
        sCurSize = 1;
    }
    else
    {
        sCurSize = aChunkSize / 10;
    }

    aclMemAreaCreate(&sMemArea, aChunkSize);

    aclMemAreaGetSnapshot(&sMemArea, &sMemSnapshot);
    aclMemAreaFreeToSnapshot(&sMemArea, &sMemSnapshot);

    aclMemAreaShrink(&sMemArea);
    aclMemAreaFreeAll(&sMemArea);

    TEST_MEM_AREA_CHECK_STAT(&sMemArea);

    for (i = 0; i < TEST_ALLOC_COUNT; i++)
    {
        if (i == (TEST_ALLOC_COUNT / 2))
        {
            aclMemAreaGetSnapshot(&sMemArea, &sMemSnapshot);
        }
        else
        {
            /* do nothing */
        }

        sRC = aclMemAreaCalloc(&sMemArea, (void **)&sAddr[i], sCurSize);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        ACT_CHECK((sAddr[i] != NULL) && (((acp_ulong_t)sAddr[i] % 8) == 0));

        TEST_MEM_AREA_CHECK_STAT(&sMemArea);

        sSize[i] = sCurSize;

        sCurSize *= 2;
    }

    aclMemAreaFreeToSnapshot(&sMemArea, &sMemSnapshot);
    TEST_MEM_AREA_CHECK_STAT(&sMemArea);

    for (i = (TEST_ALLOC_COUNT / 2); i < TEST_ALLOC_COUNT; i++)
    {
        sRC = aclMemAreaCalloc(&sMemArea, (void **)&sAddr[i], sSize[i]);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        ACT_CHECK((sAddr[i] != NULL) && (((acp_ulong_t)sAddr[i] % 8) == 0));

        TEST_MEM_AREA_CHECK_STAT(&sMemArea);
    }

    aclMemAreaFreeToSnapshot(&sMemArea, &sMemSnapshot);
    TEST_MEM_AREA_CHECK_STAT(&sMemArea);

    aclMemAreaShrink(&sMemArea);
    TEST_MEM_AREA_CHECK_STAT(&sMemArea);

    for (i = (TEST_ALLOC_COUNT / 2); i < TEST_ALLOC_COUNT; i++)
    {
        sRC = aclMemAreaCalloc(&sMemArea, (void **)&sAddr[i], sSize[i]);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        ACT_CHECK((sAddr[i] != NULL) && (((acp_ulong_t)sAddr[i] % 8) == 0));

        TEST_MEM_AREA_CHECK_STAT(&sMemArea);
    }

    aclMemAreaFreeAll(&sMemArea);
    TEST_MEM_AREA_CHECK_STAT(&sMemArea);

    aclMemAreaDestroy(&sMemArea);
}

acp_sint32_t main(void)
{
    ACT_TEST_BEGIN();

    testMemAreaFixedSize(0);
    testMemAreaFixedSize(20);

    testMemAreaDifferentSize(0);
    testMemAreaDifferentSize(20);

    ACT_TEST_END();

    return 0;
}
