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
 * $Id: $
 ******************************************************************************/

#include <act.h>
#include <aclMem.h>
#include <aclMemTlsf.h>
#include <acp.h>

#define TEST_MAX_THREADS 1024
#define TEST_NO_THREADS  1
#define TEST_NO_LOOPS    (1024*128)
#define TEST_BLOCK_SIZE  512
#define TEST_POOL_SIZE   (1024*1024)

#define TEST_SURE(aRC, aFunc)                           \
    do {                                                \
        aRC = aFunc;                                    \
        ACT_CHECK(ACP_RC_IS_SUCCESS(aRC));              \
        if(ACP_RC_NOT_SUCCESS(aRC)) { acpProcExit(1); } \
        else {}                                         \
    } while(0)
 
typedef struct testThread
{
     acp_size_t         mLoops;
     acl_mem_alloc_t*   mAllocator;
} testThread;

acp_sint64_t    gAllocs    = ACP_SINT64_LITERAL(0);
acp_sint64_t    gFrees     = ACP_SINT64_LITERAL(0);
acp_sint32_t    gCreated   = 0;
acp_sint32_t    gDestroyed = 0;
acp_sint32_t    gBlockSize = TEST_BLOCK_SIZE;
acp_sint32_t    gPoolSize  = TEST_POOL_SIZE;

acp_sint64_t    gElapsed;

enum
{
    TEST_NONE = 0,
    TEST_OPT_THREADS,
    TEST_OPT_LOOPS,
    TEST_OPT_BLOCKSIZE,
    TEST_OPT_POOLSIZE,
    TEST_OPT_TLSF,
    TEST_OPT_PRIVATE,
    TEST_OPT_HELP,
    TEST_OPT_MAX
};

static acp_opt_def_t gOptDef[] =
{
    {
        TEST_OPT_THREADS,
        ACP_OPT_ARG_REQUIRED,
        'n',
        "nothreads",
        NULL,
        "1",
        "Number of threads. 1 to 1024."
    },
    {
        TEST_OPT_LOOPS,
        ACP_OPT_ARG_REQUIRED,
        'l',
        "loops",
        NULL,
        "131072",
        "Size of loops to allocate."
    },
    {
        TEST_OPT_BLOCKSIZE,
        ACP_OPT_ARG_REQUIRED,
        'b',
        "blocksize",
        NULL,
        "512",
        "Size of blocks to be allocated."
    },
    {
        TEST_OPT_TLSF,
        ACP_OPT_ARG_NOTEXIST,
        't',
        "tlsf",
        NULL,
        NULL,
        "Use TLSF instead of LIBC."
    },
    {
        TEST_OPT_POOLSIZE,
        ACP_OPT_ARG_REQUIRED,
        'c',
        "poolsize",
        NULL,
        "1048576",
        "Size of pool of memory allocator. Without -t, this takes no effect"
    },
    {
        TEST_OPT_PRIVATE,
        ACP_OPT_ARG_NOTEXIST,
        'p',
        "private",
        NULL,
        NULL,
        "Use private TLSF instance per thread. Without -t, this takes no effect"
    },
    {
        TEST_OPT_HELP,
        ACP_OPT_ARG_NOTEXIST,
        'h',
        "help",
        NULL,
        NULL,
        "Print this help screen."
    },
    {
        0,
        ACP_OPT_ARG_NOTEXIST,
        0,
        NULL,
        NULL,
        NULL,
        NULL
    }
};

void testPrintHelp(acp_sint32_t aExitCode)
{
    acp_rc_t        sRC;
    acp_char_t      sHelp[1024];

    sRC = acpOptHelp(gOptDef, NULL, sHelp, sizeof(sHelp));
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
 
    (void)acpPrintf(sHelp);
    acpProcExit(aExitCode);
}
 
acp_sint32_t testAllocThread(void* aParam)
{
    acp_size_t        i;
    acp_rc_t            sRC;

    testThread*         sParam = (testThread*)aParam;
    acp_list_t          sList;
    acp_list_node_t*    sNode;
    acp_list_node_t*    sNext;

    acpListInit(&sList);

    for(i = 0; i < sParam->mLoops; i++)
    {
        sRC = aclMemAlloc(sParam->mAllocator, (void**)&sNode, gBlockSize);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        if(ACP_RC_NOT_SUCCESS(sRC))
        {
            continue;
        }
        else
        {
            acpListAppendNode(&sList, sNode);
            (void)acpAtomicInc64(&gAllocs);
        }
    }

    ACP_LIST_ITERATE_SAFE(&sList, sNode, sNext)
    {
        acpListDeleteNode(sNode);
        (void)aclMemFree(sParam->mAllocator, sNode);
        (void)acpAtomicInc64(&gFrees);
    }

    return 0;
}

/* ALINT:C13-DISABLE */
/* ALINT:C14-DISABLE */
acp_sint32_t main(acp_sint32_t aArgc, acp_char_t** aArgv)
{
    acp_sint32_t        i;
    acp_sint32_t        j;

    acp_sint32_t        sNoThreads  = TEST_NO_THREADS;
    acp_sint32_t        sNoLoops    = TEST_NO_LOOPS;
    acp_bool_t          sTLSF       = ACP_FALSE;
    acp_bool_t          sThreadSafe = ACP_TRUE;

    acp_sint32_t        sDummy;
    acl_mem_tlsf_init_t sInit;

    acp_opt_t           sOpt;
    acp_sint32_t        sOptValue;
    acp_char_t*         sArg;

    acl_mem_alloc_t*    sAlloc;
    acp_rc_t            sRC;

    acp_thr_t           sTestThreads[TEST_MAX_THREADS];
    testThread          sThreadParam[TEST_MAX_THREADS];
    acp_char_t          sError[1024];

    ACT_TEST_BEGIN();

    TEST_SURE(sRC, acpOptInit(&sOpt, aArgc, aArgv));

    while(ACP_TRUE)
    {
        sRC = acpOptGet(&sOpt, gOptDef, NULL, &sOptValue,
                        &sArg, sError, sizeof(sError));

        if(ACP_RC_IS_EOF(sRC))
        {
            break;
        }
        else if(ACP_RC_IS_SUCCESS(sRC))
        {
            /* fall through */
        }
        else
        {
            (void)acpPrintf(sError);
            testPrintHelp(1);
            break;
        }

        switch(sOptValue)
        {
        case TEST_OPT_THREADS:
            TEST_SURE(sRC, acpCStrToInt32(sArg, 1024, &sDummy,
                                          (acp_uint32_t*)&sNoThreads,
                                          10, NULL));
            break;
        case TEST_OPT_LOOPS:
            TEST_SURE(sRC, acpCStrToInt32(sArg, 1024, &sDummy,
                                          (acp_uint32_t*)&sNoLoops,
                                          10, NULL));
            break;
        case TEST_OPT_BLOCKSIZE:
            TEST_SURE(sRC, acpCStrToInt32(sArg, 1024, &sDummy,
                                          (acp_uint32_t*)&gBlockSize,
                                          10, NULL));
            break;
        case TEST_OPT_POOLSIZE:
            TEST_SURE(sRC, acpCStrToInt32(sArg, 1024, &sDummy,
                                          (acp_uint32_t*)&gPoolSize,
                                          10, NULL));
            break;
        case TEST_OPT_TLSF:
            sTLSF = ACP_TRUE;
            break;
        case TEST_OPT_PRIVATE:
            sThreadSafe = ACP_FALSE;
            break;
        case TEST_OPT_HELP:
            testPrintHelp(0);
            break;
        default:
            break;
        }
    }

    sInit.mPoolSize = gPoolSize;

    (void)acpPrintf("=================================================\n");
    (void)acpPrintf("Beginning test with %d threads, %d loops\n",
                    sNoThreads, sNoLoops);
    (void)acpPrintf("Allocator is %s.\n",
                    (sTLSF == ACP_TRUE)? "TLSF":"LIBC");
    if(sTLSF == ACP_TRUE)
    {
        (void)acpPrintf("\t%s for each threads.\n",
                        (sThreadSafe == ACP_TRUE)? "Common":"Private");
        (void)acpPrintf("\tPool size for TLSF is %d\n", gPoolSize);
    }
    else
    {
        /* Do not print LIBC info */
    }
    (void)acpPrintf("\t%u bytes for each alloc.\n", gBlockSize);

    gElapsed = acpTimeNow();
    if(sTLSF == ACP_TRUE && sThreadSafe == ACP_TRUE)
    {
        TEST_SURE(sRC,
                  aclMemAllocGetInstance(ACL_MEM_ALLOC_TLSF,
                                         &sInit,
                                         &sAlloc)
                  );
        sRC = aclMemAllocSetAttr(sAlloc,
                                 ACL_MEM_THREAD_SAFE_ATTR, &sThreadSafe);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        (void)acpAtomicInc32(&gCreated);
    }
    else
    {
        sAlloc = NULL;
    }

    for(i = 0; i < sNoThreads; i++)
    {
        if(sTLSF == ACP_TRUE && sThreadSafe == ACP_FALSE)
        {
            sRC = aclMemAllocGetInstance(ACL_MEM_ALLOC_TLSF,
                                         &sInit,
                                         &sAlloc);
            ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
            if(ACP_RC_NOT_SUCCESS(sRC))
            {
                break;
            }
            else
            {
                sRC = aclMemAllocSetAttr(sAlloc,
                                         ACL_MEM_THREAD_SAFE_ATTR,
                                         &sThreadSafe);
                ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
                (void)acpAtomicInc32(&gCreated);
            }
        }
        else
        {
            /* Do nothing */
        }

        sThreadParam[i].mAllocator = sAlloc;
        sThreadParam[i].mLoops = sNoLoops;

        sRC = acpThrCreate(&(sTestThreads[i]), NULL,
                           testAllocThread, &(sThreadParam[i]));
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    }

    for(j = 0; j < i; j++)
    {
        (void)acpThrJoin(&(sTestThreads[j]), NULL);

        if(sTLSF == ACP_TRUE && sThreadSafe == ACP_FALSE)
        {
            sAlloc = sThreadParam[j].mAllocator;
            (void)aclMemAllocFreeInstance(sAlloc);
            (void)acpAtomicInc32(&gDestroyed);
        }
        else
        {
            /* Do nothing */
        }
    }

    if(sTLSF == ACP_TRUE && sThreadSafe == ACP_TRUE)
    {
        (void)aclMemAllocFreeInstance(sAlloc);
        (void)acpAtomicInc32(&gDestroyed);
    }
    else
    {
        /* Do nothing */
    }

    gElapsed = acpTimeNow() - gElapsed;

    (void)acpPrintf("=================================================\n");
    (void)acpPrintf("Elapsed Time : %lldmin(s) %02lld:%03lld:%03lldsec(s)\n",
                    acpTimeToSec(gElapsed) / 60,
                    acpTimeGetSec(gElapsed) % 60,
                    acpTimeGetMsec(gElapsed),
                    acpTimeGetUsec(gElapsed) % 1000);
    (void)acpPrintf("%20d thread(s)\n", sNoThreads);
    (void)acpPrintf("%20d allocator(s) created.\n%20d destroyed.\n",
                    gCreated, gDestroyed);
    (void)acpPrintf("%20lld allocate(s).\n%20lld free(s).\n",
                    gAllocs, gFrees);
    (void)acpPrintf("=================================================\n");

    ACT_TEST_END();

    return 0;
}
