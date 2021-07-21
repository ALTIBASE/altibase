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

#include <act.h>
#include <acpAtomic.h>
#include <acpProc.h>
#include <acpShm.h>

#if defined(__INSURE__)
#define TEST_LOOPS      (4096)
#else
#define TEST_LOOPS      (262144)
#endif

#define TEST_PROCS      (16)
#define TEST_ARR_SIZE   (1024)
#define TEST_SHM_SIZE   (sizeof(testAtomicChunk))
#define TEST_PERM_RWRWRW (      \
    ACP_S_IRUSR | ACP_S_IWUSR | \
    ACP_S_IRGRP | ACP_S_IWGRP | \
    ACP_S_IROTH | ACP_S_IWOTH)

typedef struct testAtomicChunk
{
    acp_sint16_t m16[TEST_ARR_SIZE];
    acp_sint32_t m32[TEST_ARR_SIZE];
    acp_sint64_t m64[TEST_ARR_SIZE];
} testAtomicChunk;

void testParent(acp_char_t** aArgv)
{
    acp_rc_t            sRC;
    acp_shm_t           sShm;
    acp_key_t           sShmKey  = (acp_key_t)acpProcGetSelfID();
    testAtomicChunk*    sAtomic;
    acp_uint32_t        i;

    acp_proc_t      sProc[TEST_PROCS];
    acp_proc_wait_t sWR;
    acp_char_t      sID[12];
    acp_char_t      sKey[12];
    acp_char_t*     sArgs[4] = {sID, sKey, NULL};

    while(1)
    {
        sRC = acpShmCreate(sShmKey, TEST_SHM_SIZE, TEST_PERM_RWRWRW);

        if (ACP_RC_IS_EEXIST(sRC))
        {
            sShmKey += 1;
        }
        else
        {
            break;
        }
    }
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("acpShmCreate returns %d", sRC));

    sRC = acpShmAttach(&sShm, sShmKey);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("acpShmAttach returns %d", sRC));

    sAtomic = (testAtomicChunk*)acpShmGetAddress(&sShm);
    acpMemSet(sAtomic, 0, TEST_SHM_SIZE);

    for(i = 0; i < TEST_PROCS; i++)
    {
        /* Fork */
        (void)acpSnprintf(sID,  12, "%d", i);
        (void)acpSnprintf(sKey, 12, "%d", (acp_sint32_t)sShmKey);

        sRC = acpProcForkExec(&(sProc[i]), aArgv[0], sArgs, ACP_FALSE);
        ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                       ("Cannot Execute Child Process"));
    }

    for(i = 0; i < TEST_PROCS; i++)
    {
        /* Wait */
        (void)acpProcWait(&(sProc[i]), &sWR, ACP_TRUE);
    }

    /*
     * Check all values are proper
     */
    for(i = 0; i < TEST_ARR_SIZE; i++)
    {
        /*
         * The final value is 16*262144=4194304=65536*16(0x00400000)
         * This will be 0 for 16bit value
         */
        ACT_CHECK_DESC(0 == sAtomic->m16[i],
                       ("sAtomic->m16[%d] must be %d but %d",
                        i, 0, sAtomic->m16[i])
                      );
        if(0 != sAtomic->m16[i])
        {
            break;
        }
        else
        {
            /* Do nothing */
        }
    }

    for(i = 0; i < TEST_ARR_SIZE; i++)
    {
        ACT_CHECK_DESC((TEST_LOOPS * TEST_PROCS) == (sAtomic->m32[i]),
                       ("sAtomic->m32[%d] must be %d but %d",
                        i, TEST_LOOPS * TEST_PROCS, sAtomic->m32[i])
                      );
        if((TEST_LOOPS * TEST_PROCS) != sAtomic->m32[i])
        {
            break;
        }
        else
        {
            /* Do nothing */
        }
    }

    for(i = 0; i < TEST_ARR_SIZE; i++)
    {
        ACT_CHECK_DESC((TEST_LOOPS * TEST_PROCS) == (sAtomic->m64[i]),
                       ("sAtomic->m64[%d] must be %d but %d",
                        i, TEST_LOOPS * TEST_PROCS, sAtomic->m64[i])
                      );
        if((TEST_LOOPS * TEST_PROCS) != sAtomic->m64[i])
        {
            break;
        }
        else
        {
            /* Do nothing */
        }
    }

    ACT_CHECK(acpShmGetAddress(&sShm) != NULL);
    ACT_CHECK(acpShmGetSize(&sShm) == TEST_SHM_SIZE);

    sAtomic = (testAtomicChunk*)acpShmGetAddress(&sShm);

    sRC = acpShmDetach(&sShm);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("acpShmDetach returns %d", sRC));

    sRC = acpShmDestroy(sShmKey);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("acpShmDetach returns %d", sRC));
}

void testChild(acp_char_t** aArgv)
{
        /* Child Process */
    acp_rc_t            sRC;
    acp_shm_t           sShm;
    testAtomicChunk*    sAtomic;
    acp_uint32_t        i;

    acp_sint32_t        sSerial;
    acp_sint32_t        sShmID;
    acp_sint32_t        sSign;
    acp_sint32_t        j;

    (void)acpCStrToInt32(aArgv[1], 10, &sSign,
                         (acp_uint32_t*)&sSerial, 10, NULL);
    (void)acpCStrToInt32(aArgv[2], 10, &sSign,
                         (acp_uint32_t*)&sShmID, 10, NULL);

    sRC = acpShmAttach(&sShm, sShmID);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("acpShmAttach returns %d", sRC));
    sAtomic = (testAtomicChunk*)acpShmGetAddress(&sShm);

    for(j = 0; j < TEST_LOOPS; j++)
    {
        for(i = 0; i < TEST_ARR_SIZE; i++)
        {
            (void)acpAtomicInc16(&(sAtomic->m16[i]));
        }

        for(i = 0; i < TEST_ARR_SIZE; i++)
        {
            (void)acpAtomicInc32(&(sAtomic->m32[i]));
        }

        for(i = 0; i < TEST_ARR_SIZE; i++)
        {
            (void)acpAtomicInc64(&(sAtomic->m64[i]));
        }
    }


    sRC = acpShmDetach(&sShm);
    ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                   ("acpShmDetach returns %d", sRC));
}

acp_sint32_t main(acp_sint32_t aArgc, acp_char_t** aArgv)
{
    ACT_TEST_BEGIN();

    if(1 == aArgc)
    {
        testParent(aArgv);
    }
    else
    {
        testChild(aArgv);
    }

    ACT_TEST_END();

    return 0;
}
