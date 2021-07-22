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
 * $Id: testAcpShm.c 7181 2009-08-25 01:11:53Z djin $
 ******************************************************************************/

#include <act.h>
#include <acpProc.h>
#include <acpShm.h>
#include <acpCStr.h>
#include <acpFile.h>
#include <acpTest.h>

#define TEST_PERM_RWRWRW (      \
    ACP_S_IRUSR | ACP_S_IWUSR | \
    ACP_S_IRGRP | ACP_S_IWGRP | \
    ACP_S_IROTH | ACP_S_IWOTH)

#define TEST_SHM_SIZE 8192

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

acp_sint32_t main(acp_sint32_t aArgc, acp_char_t** aArgv)
{
    acp_shm_t   sShm;
    acp_rc_t    sRC;
    acp_char_t  sKey[10];
    acp_char_t* sArgs[3] = {sKey, NULL};
    acp_key_t   sShmKey  = (acp_key_t)acpProcGetSelfID();

    ACT_TEST_BEGIN();

    if (aArgc ==  1)
    {
        /* Main Process */
        acp_proc_t      sProc;
        acp_proc_wait_t sWR;

        while (1)
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

        acpMemCpy(sShm.mAddr, TEST_SHM_DATA1, TEST_SHM_DATALENGTH1);

        /* Fork */
        (void)acpSnprintf(sKey, 10, "%d", (acp_sint32_t)sShmKey);

        sRC = acpProcForkExec(&sProc, aArgv[0], sArgs, ACP_FALSE);

        if (ACP_RC_IS_SUCCESS(sRC))
        {
            acp_sint32_t sCmp;

            /* Wait */
            (void)acpProcWait(&sProc, &sWR, ACP_TRUE);

            sCmp = acpMemCmp(sShm.mAddr, TEST_SHM_DATA2, TEST_SHM_DATALENGTH2);
            ACT_CHECK(0 == sCmp);
        }
        else
        {
            ACT_CHECK_DESC(0, ("Cannot Execute Child Process"));
        }

        ACT_CHECK(acpShmGetAddress(&sShm) != NULL);
        ACT_CHECK(acpShmGetSize(&sShm) == TEST_SHM_SIZE);

        sRC = acpShmDetach(&sShm);
        ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                       ("acpShmDetach returns %d", sRC));

        sRC = acpShmDestroy(sShmKey);
        ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                       ("acpShmDetach returns %d", sRC));
    }
    else
    {
        /* Child Process */
        acp_sint32_t sShmID;
        acp_sint32_t sSign;
        acp_sint32_t sCmp;

        (void)acpCStrToInt32(aArgv[1], 10, &sSign,
                             (acp_uint32_t*)&sShmID, 10, NULL);

        sRC = acpShmAttach(&sShm, sShmID);
        ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                       ("acpShmAttach returns %d", sRC));

        sCmp = acpMemCmp(sShm.mAddr, TEST_SHM_DATA1, TEST_SHM_DATALENGTH1);
        ACT_CHECK(0 == sCmp);

        acpMemCpy(sShm.mAddr, TEST_SHM_DATA2, TEST_SHM_DATALENGTH2);

        sRC = acpShmDetach(&sShm);
        ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                       ("acpShmDetach returns %d", sRC));
    }

    ACT_TEST_END();

    return 0;
}
