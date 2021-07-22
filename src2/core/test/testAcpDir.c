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
 * $Id: testAcpDir.c 6995 2009-08-17 04:40:15Z gurugio $
 ******************************************************************************/

#include <act.h>
#include <acpDir.h>
#include <acpFile.h>

#define TEST_MAX_PATH 1023

static void testDirEntry(void)
{
    acp_char_t*     sPath1 = "testAcpDir.c";
    acp_char_t*     sPath2 = "not_existing_file";
    acp_char_t*     sPath3 = ".";
    acp_char_t*     sPath4 = "./";
    acp_char_t*     sEntryName;
    
    acp_dir_t     sDir;
    acp_stat_t    sStat;
    acp_rc_t      sRC;
    acp_uint32_t  sCount;

    sRC = acpDirOpen(&sDir, sPath1);
    ACT_CHECK(ACP_RC_IS_ENOTDIR(sRC));

    sRC = acpDirOpen(&sDir, sPath2);
    ACT_CHECK(ACP_RC_IS_ENOENT(sRC));

    sRC = acpDirOpen(&sDir, sPath3);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    for (sCount = 0; ACP_RC_IS_SUCCESS(sRC); sCount++)
    {
        sRC = acpDirRead(&sDir, &sEntryName);

        if (ACP_RC_IS_SUCCESS(sRC))
        {
            sRC = acpFileStatAtPath(sEntryName, &sStat, ACP_FALSE);
            ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        }
        else
        {
            /* If do not break here, 
             * for-loop increase sCount one more time,
             * and then sCount is over than the number of files
             */
            break;
        }
    }

    ACT_CHECK(ACP_RC_IS_EOF(sRC));

    sRC = acpDirRewind(&sDir);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    for (sCount = 0; ACP_RC_IS_SUCCESS(sRC); sCount++)
    {
        sRC = acpDirRead(&sDir, &sEntryName);

        if (ACP_RC_IS_SUCCESS(sRC))
        {
            sRC = acpFileStatAtPath(sEntryName, &sStat, ACP_FALSE);
            ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        }
        else
        {
            /* If do not break here, 
             * for-loop increase sCount one more time,
             * and then sCount is over than the number of files
             */
            break;
        }
    }

    ACT_CHECK(ACP_RC_IS_EOF(sRC));

    sRC = acpDirClose(&sDir);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpDirOpen(&sDir, sPath4);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    for (sCount = 0; ACP_RC_IS_SUCCESS(sRC); sCount++)
    {
        sRC = acpDirRead(&sDir, &sEntryName);

        if (ACP_RC_IS_SUCCESS(sRC))
        {
            sRC = acpFileStatAtPath(sEntryName, &sStat, ACP_FALSE);
            ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        }
        else
        {
            /* If do not break here, 
             * for-loop increase sCount one more time,
             * and then sCount is over than the number of files
             */
            break;
        }
    }

    ACT_CHECK(ACP_RC_IS_EOF(sRC));

    sRC = acpDirClose(&sDir);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
}

static void testDirMake(void)
{
    acp_char_t *sPath = "test_dir";
    acp_stat_t  sStat;
    acp_rc_t    sRC;

    sRC = acpDirMake(sPath, 0755);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileStatAtPath(sPath, &sStat, ACP_FALSE);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpDirRemove(sPath);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpFileStatAtPath(sPath, &sStat, ACP_FALSE);
    ACT_CHECK(ACP_RC_IS_ENOENT(sRC));
}

static void testDirGetCwd(void)
{
    acp_char_t  sLong[TEST_MAX_PATH];
    acp_char_t  sAfterChange[TEST_MAX_PATH];
    acp_char_t* sNull = NULL;
    acp_char_t  sShort[1];
    acp_rc_t    sRC;

    sRC = acpDirGetCwd(sLong, sizeof(sLong));
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpDirSetCwd("..", 3);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpDirSetCwd(sLong, sizeof(sLong));
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = acpDirGetCwd(sAfterChange, sizeof(sAfterChange));
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(0 == acpCStrCmp(sLong, sAfterChange, TEST_MAX_PATH));
    
    sRC = acpDirSetCwd("....", 3);
    ACT_CHECK(ACP_RC_IS_EINVAL(sRC));
    sRC = acpDirGetCwd(sNull, 0);
    ACT_CHECK(ACP_RC_IS_EFAULT(sRC));
    sRC = acpDirGetCwd(sShort, 1);
    ACT_CHECK(ACP_RC_IS_ERANGE(sRC));
}

static void testDirGetHome(void)
{
    acp_char_t sDir[TEST_MAX_PATH + 1];
    acp_rc_t sRC;

    sRC = acpDirGetHome(NULL, 0);
    ACT_CHECK(ACP_RC_IS_EFAULT(sRC));
    sRC = acpDirGetHome(sDir, 0);
    ACT_CHECK(ACP_RC_IS_ETRUNC(sRC));
    sRC = acpDirGetHome(sDir, TEST_MAX_PATH);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
    ACT_CHECK(acpCStrLen(sDir, TEST_MAX_PATH) > 0);
}

acp_sint32_t main(void)
{
    ACT_TEST_BEGIN();
    
    testDirEntry();
    testDirMake();
    testDirGetCwd();
    testDirGetHome();

    ACT_TEST_END();

    return 0;
}
