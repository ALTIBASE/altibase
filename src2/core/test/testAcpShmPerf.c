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
 * $Id: testAcpShmPerf.c 5910 2009-06-08 08:20:20Z djin $
 ******************************************************************************/

#include <act.h>
#include <acpShm.h>
#include <acpCStr.h>
#include <acpPrintf.h>

#define TEST_PERM_RWRWRW (      \
    ACP_S_IRUSR | ACP_S_IWUSR | \
    ACP_S_IRGRP | ACP_S_IWGRP | \
    ACP_S_IROTH | ACP_S_IWOTH)

#define TEST_SHM_SIZE  8192
#define TEST_SHM_LOOPS 10000

#define TEST_SHM_DATA1                                                  \
    "Good morning. Bon jour. Guten morgen. I love you. Je t'aime. "     \
    "Ich liebe dich. Good bye. Au-revoir. Auf Wiedersehen. "            \
    "Johannes Sebastian Bach. George Frederich Handel. "                \
    "Sergei Vasilyevich Rachmaninov."                                   \
    "Sun Mercury Venus earth Mars Jupiter Saturn Uranus Naptune Pluto"
#define TEST_SHM_DATALENGTH1 acpCStrLen(TEST_SHM_DATA1, TEST_SHM_SIZE)

#define TEST_SHM_DATA2                                                         \
    "How much wood would a woodchuck chuck if a woodchuck could chuck wood? "  \
    "A woodchuck would chuck as much wood as a woodchuck would chuck "         \
    "if a woodchuck could chuck wood. "                                        \
    "Silly sally sells sea shells by the sea shore. "                          \
    "Peter piper picks up a pack of pickled pepper. "
#define TEST_SHM_DATALENGTH2 acpCStrLen(TEST_SHM_DATA2, TEST_SHM_SIZE)

acp_sint32_t main(void)
{
    acp_shm_t    sShm;
    acp_rc_t     sRC;
    acp_sint32_t sCmp;
    acp_key_t    sShmKey = (acp_key_t)acpProcGetSelfID();
    acp_sint32_t i;

    for (i = 0; i < TEST_SHM_LOOPS; i++)
    {
        while (1)
        {
            sRC = acpShmCreate(sShmKey, TEST_SHM_SIZE, TEST_PERM_RWRWRW);

            if (ACP_RC_NOT_SUCCESS(sRC))
            {
                /* if not successed, we need to change a shmkey value */
                sShmKey += 1;
            }
            else
            {
                /* if successed */
                break;
            }
        }

        if(999 == i % 1000)
        {
            (void)acpPrintf("\tPerformance testing for Shared Memory - "
                            "%d loops\n", (acp_sint32_t)(i + 1));
        }
        else
        {
        }

        sRC = acpShmAttach(&sShm, sShmKey);
        ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                       ("acpShmAttach(&sShm, %d) failed.\n",
                        (acp_sint32_t)i));

        acpMemCpy(sShm.mAddr, TEST_SHM_DATA1, TEST_SHM_DATALENGTH1);
        sCmp = acpMemCmp(sShm.mAddr, TEST_SHM_DATA1, TEST_SHM_DATALENGTH1);
        ACT_CHECK_DESC(
            sCmp == 0,
            ("Shared memory not same at loop %d.\n", (acp_sint32_t)i)
            );

        acpMemCpy(sShm.mAddr, TEST_SHM_DATA2, TEST_SHM_DATALENGTH2);
        sCmp = acpMemCmp(sShm.mAddr, TEST_SHM_DATA2, TEST_SHM_DATALENGTH2);
        ACT_CHECK_DESC(
            sCmp == 0,
            ("Shared memory not same at loop %d.\n", (acp_sint32_t)i)
            );

        sRC = acpShmDetach(&sShm);
        ACT_CHECK_DESC(
            ACP_RC_IS_SUCCESS(sRC),
            ("acpShmDetach with key:%d failed.\n", (acp_sint32_t)sShmKey)
            );

        sRC = acpShmDestroy(sShmKey);
        ACT_CHECK_DESC(
            ACP_RC_IS_SUCCESS(sRC),
            ("acpShmDestroy with key:%d is failed.\n", (acp_sint32_t)sShmKey)
            );
    }

    return 0;
}
