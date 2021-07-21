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


#define TEST_BACKUP_SIZE   10000
#define TEST_PROCESS_COUNT 20
#define TEST_LOOP_COUNT    100

ACE_MSG_DECLARE_GLOBAL_AREA;
ACE_ERROR_CALLBACK_DECLARE_GLOBAL_AREA;

typedef struct testContext
{
    ACL_CONTEXT_PREDEFINED_OBJECTS;
} testContext;


static acp_str_t gTestLogFilePath = ACP_STR_CONST("testAclLog.log");

static acp_str_t gTestBackupFilePath = ACP_STR_CONST("testAclLog.1.log");

static acp_str_t gTestChildLogFilePath = ACP_STR_CONST("testAclLogChilds.log");


static void testExceptionFunc(testContext *aContext)
{
    ACE_TEST_RAISE(1, LABEL_EXCEPTION1);

    return;

    ACE_EXCEPTION(LABEL_EXCEPTION1)
    {
        ACE_SET(ACE_SET_ERROR(ACE_LEVEL_2, TEST_ERR_ERROR4, 200));
    }
    ACE_EXCEPTION_END;
}

static void testException(acl_log_t *aLog)
{
    ACL_CONTEXT_DECLARE(testContext, sContext);

    ACL_CONTEXT_INIT();

    ACE_CLEAR();

    testExceptionFunc(&sContext);

    aclLogLock(aLog);

    ACL_LOG_CURRENT_EXCEPTION(aLog);

    aclLogDebugWrite(aLog, "aclLog test exception finished");

    aclLogUnlock(aLog);
}

static void testSimple(acl_log_t *aLog)
{
    aclLogDebugWrite(aLog, "aclLog test started");

    aclLogCallstack(aLog);

    aclLogMessage(aLog, ACL_LOG_LEVEL_4, TEST_LOG_MESSAGE1);
    aclLogMessage(aLog, ACL_LOG_LEVEL_5, TEST_LOG_MESSAGE2, 400);
}

static void testRemoveLogFiles(void)
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
        sRC    = acpStrFindCString(&sFileName, "testAclLog.",
                                   &sIndex,
                                   sIndex, 0);

        if (ACP_RC_IS_SUCCESS(sRC) && (sIndex == 0))
        {
            if (acpStrCmpCString(&sExt, acpPathExt(sFile, &sPool), 0) == 0)
            {
                sRC = acpFileRemove(sFile);
                ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
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

static void testRemoveChildLogFiles(void)
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
        sRC    = acpStrFindCString(&sFileName, "testAclLogChilds.",
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
        sRC    = acpStrFindCString(&sFileName, "testAclLogChilds.",
                                   &sIndex,
                                   sIndex, 0);

        if (ACP_RC_IS_SUCCESS(sRC) && (sIndex == 0))
        {
            if (acpCStrCmp(sFile, "testAclLogChilds.log", 0) == 0)
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

acp_sint32_t main(acp_sint32_t aArgc, acp_char_t *aArgv[])
{
    acp_path_pool_t   sPool;
    const acp_char_t *sPathDir;
    acl_log_t         sLog;
    acp_proc_t        sProc[TEST_PROCESS_COUNT];
    acp_char_t       *sArgs[3];
    acp_rc_t          sRC;
    acp_sint32_t      i;
    acp_stat_t        sStat;
    acp_offset_t      sLogFileSize = 0;

    ACP_STR_DECLARE_DYNAMIC(sDir);
    ACP_STR_INIT_DYNAMIC(sDir, 0, 32);

    ACE_MSG_SET_GLOBAL_AREA;
    ACE_ERROR_CALLBACK_SET_GLOBAL_AREA;

    ACT_TEST_BEGIN();

    acpPathPoolInit(&sPool);

    sPathDir = acpPathDir(aArgv[0], &sPool);
    ACT_CHECK(NULL != sPathDir);

    if (aArgc > 1)
    {
        acp_uint32_t sFlag;

        testRemoveChildLogFiles();

        sFlag =  ACL_LOG_UID | ACL_LOG_PID |
                 ACL_LOG_TID | ACL_LOG_LEVEL | ACL_LOG_PSHARED |
                 ACL_LOG_TSHARED;

        switch (((acp_uint32_t)acpProcGetSelfID()) % 8)
        {
            case 0:
                sFlag |= ACL_LOG_TIMESTAMP_MIC;
                break;
            case 1:
                sFlag |= ACL_LOG_TIMESTAMP_MIC;
                break;
            case 2:
                sFlag |= ACL_LOG_TIMESTAMP_MIC;
                break;
            case 3:
                sFlag |= ACL_LOG_TIMESTAMP_MIC;
                break;
            case 4:
                sFlag |= ACL_LOG_TIMESTAMP_MIC | ACL_LOG_IMMEDIATEFLUSH;
                break;
            case 5:
                sFlag |= ACL_LOG_TIMESTAMP_MIC | ACL_LOG_IMMEDIATEFLUSH;
                break;
            case 6:
                sFlag |= ACL_LOG_TIMESTAMP_MIC | ACL_LOG_IMMEDIATEFLUSH;
                break;
            case 7:
                sFlag |= ACL_LOG_TIMESTAMP_MIC | ACL_LOG_IMMEDIATEFLUSH;
                break;
        }
        
        sRC = acpStrCpyFormat(&sDir, "%s/msg", sPathDir);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        sRC = testMsgLoad(&sDir, NULL, NULL, NULL);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        sRC = aclLogOpen(&sLog,
                         &gTestChildLogFilePath,
                         sFlag,
                         5);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        aclLogSetBackupSize(&sLog, TEST_BACKUP_SIZE);

        for (i = 0; i < TEST_LOOP_COUNT; i++)
        {
            /*
             * BUG-31783
             * To confirm the consistency of the order of backup.
             * acpSleepSec(1);
             */
            aclLogMessage(&sLog, ACL_LOG_LEVEL_4, TEST_LOG_MESSAGE2, i);
        }

        testBackupFilesSize(TEST_BACKUP_SIZE);

        aclLogClose(&sLog);
    }
    else
    {
        testRemoveLogFiles();

        sRC = acpStrCpyFormat(&sDir, "%s/msg", sPathDir);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        sRC = aclLogOpen(&sLog,
                         &gTestLogFilePath,
                         ACL_LOG_TIMESTAMP | ACL_LOG_UID | ACL_LOG_PID |
                         ACL_LOG_TID | ACL_LOG_LEVEL | ACL_LOG_PSHARED |
                         ACL_LOG_TSHARED,
                         ACL_LOG_LIMIT_DEFAULT);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        aclLogSetBackupSize(&sLog, 1000);

        testSimple(&sLog);
        testException(&sLog);

        sRC = testMsgLoad(&sDir, NULL, NULL, NULL);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        testSimple(&sLog);
        testException(&sLog);

        aclLogSetBackupSize(&sLog, TEST_BACKUP_SIZE);

        sArgs[0] = "child";
        sArgs[1] = NULL;

        for (i = 0; i < TEST_PROCESS_COUNT; i++)
        {
            sRC = acpProcForkExec(&sProc[i], aArgv[0], sArgs, ACP_FALSE);
            ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        }

        for (i = 0; i < TEST_LOOP_COUNT; i++)
        {
            aclLogMessage(&sLog, ACL_LOG_LEVEL_4, TEST_LOG_MESSAGE2, 111);
        }

        for (i = 0; i < TEST_PROCESS_COUNT; i++)
        {
            sRC = acpProcWait(NULL, NULL, ACP_TRUE);
            ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));
        }

        aclLogClose(&sLog);

        /* Test scenario for BUG-25143.                        *
         * Checks that there is no stuck when we open log file *
         * with size more or equal to current backup size      */

        testRemoveLogFiles();

        /* Prepare log file */
        sRC = aclLogOpen(&sLog,
                         &gTestLogFilePath,
                         ACL_LOG_PSHARED,
                         ACL_LOG_LIMIT_DEFAULT);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        for (i = 0; i < TEST_LOOP_COUNT; i++)
        {
            aclLogMessage(&sLog, ACL_LOG_LEVEL_4, TEST_LOG_MESSAGE2, 222);
        }

        aclLogClose(&sLog);

        /* Get log file size */
        sRC = acpFileStatAtPath(acpStrGetBuffer(&gTestLogFilePath), 
                                &sStat, ACP_FALSE);

        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        sLogFileSize = sStat.mSize;

        /* Open log file again */
        sRC = aclLogOpen(&sLog,
                         &gTestLogFilePath,
                         ACL_LOG_PSHARED,
                         ACL_LOG_LIMIT_DEFAULT);
        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        /* Set backup size equal to log file size to force backup */
        aclLogSetBackupSize(&sLog, sLogFileSize);

        /* Write log, backup should be made at this point */
        aclLogMessage(&sLog, ACL_LOG_LEVEL_4, TEST_LOG_MESSAGE2, 333);

        aclLogClose(&sLog);

        /* Check that backup has been created */
        sRC = acpFileStatAtPath(acpStrGetBuffer(&gTestBackupFilePath), 
                                &sStat, ACP_FALSE);

        ACT_CHECK(ACP_RC_IS_SUCCESS(sRC));

        ACT_CHECK(sStat.mSize == sLogFileSize);
    }

    acpPathPoolFinal(&sPool);

    ACP_STR_FINAL(sDir);

    ACT_TEST_END();

    return 0;
}
