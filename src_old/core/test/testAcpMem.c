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
 * $Id: testAcpMem.c 11316 2010-06-22 23:56:42Z djin $
 ******************************************************************************/

#include <act.h>
#include <acpDl.h>
#include <acpEnv.h>
#include <acpPath.h>
#include <acpMem.h>
#include <acpSys.h>


static void testBasic(void)
{
    acp_char_t *sMem;
    acp_size_t  sSize = 0;
    acp_rc_t    sRC;

    sRC = acpMemAlloc((void **)&sMem, 0);
    ACT_CHECK(ACP_RC_IS_EINVAL(sRC));

    sRC = acpMemCalloc((void **)&sMem, 0, 0);
    ACT_CHECK(ACP_RC_IS_EINVAL(sRC));

    sRC = acpMemRealloc((void **)&sMem, 0);
    ACT_CHECK(ACP_RC_IS_EINVAL(sRC));

    sRC = acpMemAlloc((void **)&sMem, 1024);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC) && (sMem != NULL),
                   ("acpMemAlloc for 1024 bytes failed: error code=%d", sRC));

    sRC = acpMemRealloc((void **)&sMem, 102400);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC) && (sMem != NULL),
                   ("acpMemRealloc to 102400 bytes failed: error code=%d",
                    sRC));

    acpMemFree(sMem);

    sMem = NULL;

    sRC = acpMemRealloc((void **)&sMem, 2048);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC) && (sMem != NULL),
                   ("acpMemRealloc for alloc 2048 bytes failed from NULL: "
                    "error code=%d mem=%p", sRC, sMem));

    acpMemFree(sMem);

    sRC = acpSysGetPageSize(&sSize);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

#if !defined(__INSURE__)
    sRC = acpMemAllocAlign((void**)&sMem, sSize, sSize);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(((acp_ulong_t)sMem % sSize) == 0);

    acpMemFreeAlign(sMem);

    sRC = acpMemAllocAlign((void**)&sMem, sSize / 2, sSize);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(((acp_ulong_t)sMem % sSize) == 0);

    acpMemFreeAlign(sMem);
#endif
}

static void testMove(void)
{
    acp_char_t   *sMem = NULL;
    acp_rc_t      sRC;
    acp_sint32_t  sRet;
    acp_sint32_t  i;

    sRC = acpMemCalloc((void **)&sMem, 8, 128);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC) && (sMem != NULL),
                   ("acpMemCalloc for 1024 bytes failed: error code=%d", sRC));

    for (i = 0; i < 1024; i++)
    {
        ACT_CHECK_DESC(sMem[i] == 0,
                       ("memory alloced via acpMemCalloc is not cleared "
                        "at 0x%x",
                        i));
    }

    acpMemSet(sMem, 10, 1024);

    for (i = 0; i < 1024; i++)
    {
        ACT_CHECK_DESC(sMem[i] == 10,
                       ("acpMemSet to 10, but value at 0x%x is %d",
                        i,
                        sMem[i]));
    }

    sRet = acpMemCmp(sMem + 128, sMem + 512, 256);

    ACT_CHECK_DESC(sRet == 0,
                   ("acpMemCmp should return 0, but returned %d", sRet));

    acpMemSet(sMem + 128, 20, 256);
    acpMemSet(sMem + 512, 30, 256);

    sRet = acpMemCmp(sMem + 128, sMem + 512, 256);

    ACT_CHECK_DESC(sRet < 0,
                   ("acpMemCmp should return positive value, but returned %d",
                    sRet));

    sRet = acpMemCmp(sMem + 512, sMem + 128, 256);

    ACT_CHECK_DESC(sRet > 0,
                   ("acpMemCmp should return negative value, but returned %d",
                    sRet));

    acpMemCpy(sMem, sMem + 128, 128);

    sRet = acpMemCmp(sMem, sMem + 128, 128);

    ACT_CHECK_DESC(sRet == 0,
                   ("copied memory via acpMemCpy is different from source"));

    acpMemSet(sMem + 256, 25, 128);
    acpMemMove(sMem + 256, sMem + 128, 256);

    for (i = 256; i < 384; i++)
    {
        ACT_CHECK_DESC(sMem[i] == 20,
                       ("moved memory via acmMemMove is different "
                        "from source"));
    }
    for (i = 384; i < 512; i++)
    {
        ACT_CHECK_DESC(sMem[i] == 25,
                       ("moved memory via acmMemMove is different "
                        "from source"));
    }

    acpMemSet(sMem + 640, 35, 128);
    acpMemMove(sMem + 384, sMem + 512, 256);

    for (i = 384; i < 512; i++)
    {
        ACT_CHECK_DESC(sMem[i] == 30,
                       ("moved memory via acmMemMove is different "
                        "from source"));
    }
    for (i = 512; i < 640; i++)
    {
        ACT_CHECK_DESC(sMem[i] == 35,
                       ("moved memory via acmMemMove is different "
                        "from source"));
    }

    acpMemFree(sMem);
}

static void testError(void)
{
#if !defined(ALTI_CFG_OS_WINDOWS) && !defined(__INSURE__)
    acp_char_t *sMem;
    acp_rc_t    sRC;

    sRC = acpMemAlloc((void **)&sMem, (acp_size_t)ACP_ULONG_MAX);
    ACT_CHECK_DESC(ACP_RC_IS_ENOMEM(sRC),
                   ("error code of memory allocation failure "
                    "should be ENOMEM"));
#endif
}

/*
 * Test alloc/frees across binaries
 */
#define LIB_NAME "testAcpDl"

static void testDl()
{
    typedef acp_rc_t    testAllocFunc(void**);
    typedef void        testFreeFunc(void*);

    acp_rc_t        sRC;
    acp_dl_t        sDl;
    acp_char_t     *sHomeDir;
    acp_char_t      sDir[ACP_PATH_MAX_LENGTH];
    acp_char_t*     sName = LIB_NAME;

    testAllocFunc*  sAlloc;
    testFreeFunc*   sFree;
    void*           sPtr;

    sRC = acpEnvGet( ALTIBASE_ENV_PREFIX"HOME", &sHomeDir );
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpSnprintf(sDir, sizeof(sDir), "%s/lib", sHomeDir);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpDlOpen(&sDl, sDir, sName, ACP_TRUE);

    if (ACP_RC_IS_SUCCESS(sRC))
    {
        ACT_CHECK(acpDlError(&sDl) == NULL);
    }
    else
    {
        ACT_ERROR(("acpDlOpen failed for %s: %s", sName, acpDlError(&sDl)));
    }

    sAlloc = (testAllocFunc*)acpDlSym(&sDl, "libtestAcpDlAlloc");
    ACT_CHECK_DESC(sAlloc != NULL,
                   ("libtestAcpDlAlloc symbol not found"));
    ACT_CHECK(acpDlError(&sDl) == NULL);

    sFree = (testFreeFunc*)acpDlSym(&sDl, "libtestAcpDlFree");
    ACT_CHECK_DESC(sFree != NULL,
                   ("libtestAcpDlFree symbol not found"));
    ACT_CHECK(acpDlError(&sDl) == NULL);

    if(NULL != sAlloc)
    {
        /*
         * Allocate in shared object
         */
        sRC = (*sAlloc)(&sPtr);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        /*
         * Free in executable
         */
        acpMemFree(sPtr);

        /*
         * Allocate in executable
         */
        sRC = acpMemAlloc(&sPtr, 1024);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        /*
         * Free in shared object
         */
        (*sFree)(sPtr);
    }
    else
    {
        /* Do nothing */
    }

    (void)acpDlClose(&sDl);
}

acp_sint32_t main(acp_sint32_t aArgc, acp_char_t** aArgv)
{
    ACP_UNUSED(aArgc);
    ACP_UNUSED(aArgv);
    ACT_TEST_BEGIN();

    testBasic();
    testMove();
    testError();
    testDl();

    ACT_TEST_END();

    return 0;
}
