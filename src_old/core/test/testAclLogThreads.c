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
 * $Id: testAclLog.c 8895 2009-11-30 03:11:04Z gurugio $
 ******************************************************************************/

#include <act.h>
#include <acpDir.h>
#include <acpProc.h>
#include <aclContext.h>
#include <aclLog.h>
#include <aceMsgTable.h>
#include <testMsg.h>


#define TEST_BACKUP_SIZE   50000
#define TEST_THREAD_COUNT  10
#define TEST_LOOP_COUNT    1000

ACE_MSG_DECLARE_GLOBAL_AREA;
ACE_ERROR_CALLBACK_DECLARE_GLOBAL_AREA;

typedef struct testContext
{
    ACL_CONTEXT_PREDEFINED_OBJECTS;
} testContext;

typedef struct testThreadLog
{
    acp_sint32_t mID;
    acl_log_t*   mLog;
} testThreadLog;

static acp_sint32_t gMsgNo = 0;
static acp_str_t gTestThreadLogFilePath =
    ACP_STR_CONST("testAclLogThreads.log");

static void testRemoveThreadLogFiles(void)
{
    acp_path_pool_t sPool;
    acp_str_t       sExt  = ACP_STR_CONST("log");
    acp_dir_t       sDir;
    acp_sint32_t    sIndex;
    acp_rc_t        sRC;
    acp_char_t     *sFile;
    acp_str_t       sFileName;
    
    ACP_STR_INIT_CONST(sFileName);
    
    acpPathPoolInit(&sPool);

    sRC = acpDirOpen(&sDir, ".");
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    while (1)
    {
        sRC = acpDirRead(&sDir, &sFile);

        if (ACP_RC_IS_EOF(sRC))
        {
            break;
        }
        else if (ACP_RC_NOT_SUCCESS(sRC))
        {
            ACT_ERROR(("acpDirRead failed with error %d", sRC));
            break;
        }
        else
        {
            /* do nothing */
        }

        sIndex = ACP_STR_INDEX_INITIALIZER;
        
        acpStrSetConstCString(&sFileName, sFile);
        sRC    = acpStrFindCString(&sFileName, "testAclLogThreads.",
                                   &sIndex,
                                   sIndex, 0);

        if (ACP_RC_IS_SUCCESS(sRC) && (sIndex == 0))
        {
            if (acpStrCmpCString(&sExt, acpPathExt(sFile, &sPool), 0) == 0)
            {
                /* BUGBUG: sometimes file removing failes, do not know why */
                (void)acpFileRemove(sFile);
                ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                               ("FILE NAME:%s\n", sFile));
                
            }
            else
            {
                /* do nothing */
            }
        }
        else
        {
            /* do nothing */
        }
    }

    sRC = acpDirClose(&sDir);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    acpPathPoolFinal(&sPool);
}


void testBackupFilesSize(acp_offset_t aCmpSize)
{
    acp_path_pool_t sPool;
    acp_str_t       sExt  = ACP_STR_CONST("log");
    acp_dir_t       sDir;
    acp_sint32_t    sIndex;
    acp_rc_t        sRC;
    acp_char_t     *sFile;
    acp_str_t       sFileName;
    acp_stat_t      sStat;
    
    ACP_STR_INIT_CONST(sFileName);
    
    acpPathPoolInit(&sPool);

    sRC = acpDirOpen(&sDir, ".");
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    while (1)
    {
        sRC = acpDirRead(&sDir, &sFile);

        if (ACP_RC_IS_EOF(sRC))
        {
            break;
        }
        else if (ACP_RC_NOT_SUCCESS(sRC))
        {
            ACT_ERROR(("acpDirRead failed with error %d", sRC));
            break;
        }
        else
        {
            /* do nothing */
        }

        sIndex = ACP_STR_INDEX_INITIALIZER;
        
        acpStrSetConstCString(&sFileName, sFile);
        sRC    = acpStrFindCString(&sFileName, "testAclLogThreads.",
                                   &sIndex,
                                   sIndex, 0);

        if (ACP_RC_IS_SUCCESS(sRC) && (sIndex == 0))
        {
            if (acpCStrCmp(sFile, "testAclLogThreads.log", 0) == 0)
            {
                continue;
            }
            else
            {
                /* do nothing */
            }
            
            if (acpStrCmpCString(&sExt, acpPathExt(sFile, &sPool), 0) == 0)
            {
                sRC = acpFileStatAtPath(sFile, &sStat, ACP_FALSE);
                ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

                ACT_CHECK_DESC(sStat.mSize >= aCmpSize,
                          ("Backup file %s must be bigger than %d",
                           sFile, (acp_sint32_t)aCmpSize));
            }
            else
            {
                /* do nothing */
            }
        }
        else
        {
            /* do nothing */
        }
    }

    sRC = acpDirClose(&sDir);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    acpPathPoolFinal(&sPool);
}

acp_sint32_t testAclLogThread(void* aArg)
{
    acp_sint32_t i;
    acp_sint32_t j;
    testThreadLog* sLog = (testThreadLog*)aArg;

    for (i = 0; i < TEST_LOOP_COUNT; i++)
    {
        j = acpAtomicInc32(&gMsgNo);
        aclLogMessage(sLog->mLog, ACL_LOG_LEVEL_4, TEST_LOG_THREAD_MESSAGE,
                      i, sLog->mID, j);
    }

    return 0;
}

acp_sint32_t main(acp_sint32_t aArgc, acp_char_t *aArgv[])
{
    acp_path_pool_t   sPool;
    const acp_char_t *sPathDir;
    acl_log_t         sLog;
    acp_rc_t          sRC;
    acp_sint32_t      i;
    acp_sint32_t      j;
    acp_thr_t         sThreads[TEST_THREAD_COUNT];
    testThreadLog     sLogs[TEST_THREAD_COUNT];

    ACP_STR_DECLARE_DYNAMIC(sDir);
    ACP_STR_INIT_DYNAMIC(sDir, 0, 32);

    ACP_UNUSED(aArgc);

    ACE_MSG_SET_GLOBAL_AREA;
    ACE_ERROR_CALLBACK_SET_GLOBAL_AREA;

    ACT_TEST_BEGIN();

    acpPathPoolInit(&sPool);

    sPathDir = acpPathDir(aArgv[0], &sPool);
    ACT_CHECK(NULL != sPathDir);

    testRemoveThreadLogFiles();

    sRC = acpStrCpyFormat(&sDir, "%s/msg", sPathDir);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    sRC = aclLogOpen(&sLog,
                     &gTestThreadLogFilePath,
                     ACL_LOG_TIMESTAMP | ACL_LOG_UID | ACL_LOG_PID |
                     ACL_LOG_TID | ACL_LOG_LEVEL | ACL_LOG_TSHARED,
                     3);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    aclLogSetBackupSize(&sLog, 1000);

    sRC = testMsgLoad(&sDir, NULL, NULL, NULL);
    ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

    aclLogSetBackupSize(&sLog, TEST_BACKUP_SIZE);

    for (i = 0; i < TEST_THREAD_COUNT; i++)
    {
        sLogs[i].mID = i;
        sLogs[i].mLog = &sLog;

        sRC = acpThrCreate(&sThreads[i], NULL, testAclLogThread, &sLogs[i]);
        ACT_CHECK_DESC(ACP_RC_IS_SUCCESS(sRC),
                       ("Cannot create Thread %d", i));
    }

    for (i = 0; i < TEST_LOOP_COUNT; i++)
    {
        j = acpAtomicInc32(&gMsgNo);
        aclLogMessage(&sLog, ACL_LOG_LEVEL_4,
                      TEST_LOG_THREAD_MESSAGE,
                      i, TEST_THREAD_COUNT, j);
    }

    for (i = 0; i < TEST_THREAD_COUNT; i++)
    {
        (void)acpThrJoin(&sThreads[i], NULL);
    }

    aclLogClose(&sLog);

    testBackupFilesSize(TEST_BACKUP_SIZE);

    acpPathPoolFinal(&sPool);

    ACP_STR_FINAL(sDir);

    ACT_TEST_END();

    return 0;
}
