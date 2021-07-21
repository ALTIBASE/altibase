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
 * $Id: aclLog.c 11057 2010-05-11 07:16:40Z gurugio $
 ******************************************************************************/

#include <acpPrintf.h>
#include <acpProc.h>
#include <acpSys.h>
#include <aclLog.h>
#include <aceAssert.h>
#include <aceMsgTable.h>


#define ACL_LOG_DEFAULT_BACKUP_SIZE (1024 * 1024)

#define ACL_LOG_BUFFER_INITIAL_SIZE 5120
#define ACL_LOG_BUFFER_EXTEND_SIZE  1024
#define ACL_LOG_BUFFER_FLUSH_SIZE   4096

#define ACL_LOG_PERM_RWRWRW (   \
    ACP_S_IRUSR | ACP_S_IWUSR | \
    ACP_S_IRGRP | ACP_S_IWGRP | \
    ACP_S_IROTH | ACP_S_IWOTH)

ACP_INLINE void aclLogBackup(acl_log_t *aLog)
{
    acp_path_pool_t   sPool;
    acp_stat_t        sStat;
    const acp_char_t *sPathBase;
    const acp_char_t *sPathExt;
    acp_rc_t          sRC;
    acp_sint32_t      i;
    acp_sint32_t      j;
    acp_sint32_t      sLimit;
    acp_sint32_t      sIndex;
    acp_sint32_t      sToMake;
    acp_bool_t        sBlank = ACP_FALSE;
    acp_time_t        sTimeOldest = acpTimeNow();

    ACP_STR_DECLARE_DYNAMIC(sPath);

    i = acpAtomicCas32(&(aLog->mBackingUp), 1, 0);

    /*
     * Another thread is backing up.
     * Just continue logging
     */
    ACP_TEST_RAISE(0 != i, NO_BACKUP);

    sLimit = aclLogGetBackupLimit(aLog);
    sLimit = (sLimit > 1)? sLimit : ACP_SINT32_MAX;
    sIndex = acpAtomicGet32(&(aLog->mBackupIndex));
    sToMake = 0;

    ACP_STR_INIT_DYNAMIC(sPath,
                         (acp_sint32_t)acpStrGetLength(&aLog->mPath) + 12,
                         16);

    ACP_TEST_RAISE(NULL == acpStrGetBuffer(&(aLog->mPath)),
                   STRING_FAILED);

    acpPathPoolInit(&sPool);

    sPathBase = acpPathBase(acpStrGetBuffer(&aLog->mPath), &sPool);
    sPathExt  = acpPathExt(acpStrGetBuffer(&aLog->mPath), &sPool);

    /* Can do nothing :( */
    ACP_TEST_RAISE((NULL == sPathBase) || (NULL == sPathExt),
                   PATH_FAILED);

    for (i = 0; i < sLimit; i++)
    {
        j = (i + sIndex) % sLimit;

        sRC = acpStrCpyFormat(&sPath, "%s.%u.%s", sPathBase, j + 1, sPathExt);
        ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), STRCAT_FAILED);
        
        sRC = acpFileStatAtPath(acpStrGetBuffer(&sPath), &sStat, ACP_FALSE);

        if (ACP_RC_IS_ENOENT(sRC))
        {
            sBlank = ACP_TRUE;
            sToMake = i;
            break;
        }
        else
        {
            ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), STAT_FAILED);
            
            if((sStat.mModifyTime < sTimeOldest) && (sToMake <= i))
            {
                sTimeOldest = sStat.mModifyTime;
                sToMake = i;
            }
            else
            {
                /* Do nothing  and continue */
            }
        }
    }

    sToMake = (sToMake + sIndex) % sLimit;

    sRC = acpStrCpyFormat(&sPath, "%s.%u.%s", sPathBase, sToMake + 1, sPathExt);
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), STRCAT_FAILED);

    if(sBlank == ACP_FALSE)
    {
        sRC = acpFileRemove(acpStrGetBuffer(&sPath));
    }
    else
    {
        /* Do nothing */
    }

    sRC = acpFileRename(acpStrGetBuffer(&aLog->mPath),
                        acpStrGetBuffer(&sPath));
    if (ACP_RC_IS_EACCES(sRC))
    {
        sRC = acpFileCopy(acpStrGetBuffer(&aLog->mPath),
                          acpStrGetBuffer(&sPath), ACP_FALSE);

        if (ACP_RC_IS_SUCCESS(sRC))
        {
            sRC = acpFileTruncate(&aLog->mFile, 0);

            if (ACP_RC_NOT_SUCCESS(sRC))
            {
                (void)acpFileRemove(acpStrGetBuffer(&sPath));
            }
            else
            {
                /* log file was truncated successfully */
            }
        }
        else
        {
            /* error, just do nothing */
        }
    }
    else
    {
        /* success or error, just do nothing*/
    }

    (void)acpAtomicSet32(&(aLog->mBackupIndex), sToMake + 1);

    acpPathPoolFinal(&sPool);

    ACP_STR_FINAL(sPath);

    (void)acpAtomicSet32(&(aLog->mBackingUp), 0);

    return;

    ACP_EXCEPTION(NO_BACKUP)
    {
        /* Another process is backing up */
    }

    ACP_EXCEPTION(STRING_FAILED)
    {
        (void)acpAtomicSet32(&(aLog->mBackingUp), 0);
    }
    
    ACP_EXCEPTION(PATH_FAILED)
    {
        /* finalize resources */
        ACP_STR_FINAL(sPath);
        acpPathPoolFinal(&sPool); 
        (void)acpAtomicSet32(&(aLog->mBackingUp), 0);
    }

    ACP_EXCEPTION(STRCAT_FAILED)
    {
        /* finalize resources */
        ACP_STR_FINAL(sPath);
        acpPathPoolFinal(&sPool); 
        (void)acpAtomicSet32(&(aLog->mBackingUp), 0);
    }

    ACP_EXCEPTION(STAT_FAILED)
    {
        ACP_STR_FINAL(sPath);
        acpPathPoolFinal(&sPool); 
        (void)acpAtomicSet32(&(aLog->mBackingUp), 0);
    }

    ACP_EXCEPTION_END;

    return;
}

/**
 * flush the log file
 * @param aLog pointer to the log object to be flushed
 */
ACP_EXPORT void aclLogFlush(acl_log_t *aLog)
{
    acp_size_t sSize = 0;
    acp_size_t sLen;
    acp_size_t sOffset;
    acp_rc_t   sRC;
    acp_char_t *sMsg = acpStrGetBuffer(&aLog->mBuffer);
    sLen    = acpStrGetLength(&aLog->mBuffer);
    sOffset = 0;

    ACE_ASSERT(aLog->mBuffer.mString != NULL);
    
    if (sMsg != NULL)
    {
        while (sLen > 0)
        {
            
            sRC = acpFileWrite(&aLog->mFile,
                               sMsg + sOffset,
                               sLen - sOffset,
                               &sSize);
            
            if (ACP_RC_NOT_SUCCESS(sRC))
            {
                break;
            }
            else
            {
                sLen    -= sSize;
                sOffset += sSize;
            }

            if(ACP_TRUE == aLog->mIsImmediateFlush)
            {
                sRC = acpFileSync(&aLog->mFile);
                if(ACP_RC_NOT_SUCCESS(sRC))
                {
                    break;
                }
                else
                {
                    /* Loop */
                }
            }
            else
            {
                /* Loop */
            }
        }
        
        acpStrClear(&aLog->mBuffer);
    }
    else
    {
        /* Nothing to do on the NULL of sMsg : Just Skip */
    }
}

ACP_INLINE void aclLogFileOpen(acl_log_t *aLog)
{
    acp_offset_t sSize = 0;
    acp_rc_t     sRC;
    acp_bool_t   sPrevBackup = ACP_FALSE;

    do
    {
        sRC = acpFileOpen(&aLog->mFile,
                          acpStrGetBuffer(&aLog->mPath),
                          ACP_O_WRONLY | ACP_O_CREAT | ACP_O_APPEND,
                          ACL_LOG_PERM_RWRWRW);

        ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), DO_NOTHING_EXIT);

        if ((aLog->mFlag & ACL_LOG_PSHARED) != 0)
        {
            sRC = acpFileLock(&aLog->mFile);

            if (ACP_RC_NOT_SUCCESS(sRC))
            {
                (void)acpFileClose(&aLog->mFile);

                return;
            }
            else
            {
                /* do nothing */
            }
        }
        else
        {
            /* No need of Lock! */
        }

        if (aLog->mBackupSize > 0)
        {
            sRC = acpFileSeek(&aLog->mFile, 0, ACP_SEEK_END, &sSize);
            ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), CLOSE_EXIT);

            /* Return to prevent endless loop if      *
             * backup was done and we still have file * 
             * size more than backup size             */
            ACP_TEST_RAISE((sPrevBackup == ACP_TRUE) && 
                           (aLog->mBackupSize <= sSize),
                           CLOSE_EXIT);

            if (aLog->mBackupSize <= sSize)
            {
                acp_stat_t sNewStat;

                sRC = acpFileStatAtPath(acpStrGetBuffer(&aLog->mPath),
                                        &sNewStat,
                                        ACP_FALSE);
                if (ACP_RC_IS_SUCCESS(sRC) &&
                    sNewStat.mSize >= aLog->mBackupSize)
                {
                    /* Backup file */
                    aclLogBackup(aLog);
                    sPrevBackup = ACP_TRUE;
                }
                else
                {
                    /* do nothing */
                }

                if ((aLog->mFlag & ACL_LOG_PSHARED) != 0)
                {
                    (void)acpFileUnlock(&aLog->mFile);
                }
                else
                {
                    /* No need of unlock */
                }
                (void)acpFileClose(&aLog->mFile);
            }
            else
            {
                aLog->mIsFileOpened = ACP_TRUE;
            }
        }
        else
        {
            aLog->mIsFileOpened = ACP_TRUE;
        }
    } while (aLog->mIsFileOpened != ACP_TRUE);


    ACP_EXCEPTION(DO_NOTHING_EXIT)
    {
    }

    ACP_EXCEPTION(CLOSE_EXIT)
    {
        (void)acpFileUnlock(&aLog->mFile);
        (void)acpFileClose(&aLog->mFile);
    }
    
    ACP_EXCEPTION_END;
    return;
}

ACP_INLINE void aclLogFileClose(acl_log_t *aLog)
{
    if (aLog->mIsFileOpened == ACP_TRUE)
    {
        aclLogFlush(aLog);

        if ((aLog->mFlag & ACL_LOG_PSHARED) != 0)
        {
            (void)acpFileUnlock(&aLog->mFile);
        }
        else
        {
            /* do nothing */
        }

        (void)acpFileClose(&aLog->mFile);

        aLog->mIsFileOpened = ACP_FALSE;
    }
    else
    {
        /* do nothing */
    }
}

ACP_INLINE void aclLogWriteHeaderTimestamp(acl_log_t  *aLog,
                                           acp_bool_t *aNeedBlank)
{
    acp_time_exp_t sTimeExp;

    if ((aLog->mFlag & ACL_LOG_TIMESTAMP) != 0)
    {
        acpTimeGetLocalTime(acpTimeNow(), &sTimeExp);

        switch( (aLog->mFlag & ACL_LOG_TIMESTAMP_MASK) )
        {
            default: 
            case (ACL_LOG_TIMESTAMP_SEC & ACL_LOG_TIMESTAMP_MASK): /* second */
                (void)acpStrCatFormat(&aLog->mBuffer,
                                      "%04d-%02d-%02d %02d:%02d:%02d",
                                      sTimeExp.mYear,
                                      sTimeExp.mMonth,
                                      sTimeExp.mDay,
                                      sTimeExp.mHour,
                                      sTimeExp.mMin,
                                      sTimeExp.mSec);
                break;
            case (ACL_LOG_TIMESTAMP_MIN & ACL_LOG_TIMESTAMP_MASK): /* minute */
                (void)acpStrCatFormat(&aLog->mBuffer,
                                      "%04d-%02d-%02d %02d:%02d",
                                      sTimeExp.mYear,
                                      sTimeExp.mMonth,
                                      sTimeExp.mDay,
                                      sTimeExp.mHour,
                                      sTimeExp.mMin);
                break;
            case (ACL_LOG_TIMESTAMP_MIL & ACL_LOG_TIMESTAMP_MASK): /* mili */
                (void)acpStrCatFormat(&aLog->mBuffer,
                                      "%04d-%02d-%02d %02d:%02d:%02d.%03d",
                                      sTimeExp.mYear,
                                      sTimeExp.mMonth,
                                      sTimeExp.mDay,
                                      sTimeExp.mHour,
                                      sTimeExp.mMin,
                                      sTimeExp.mSec,
                                      sTimeExp.mUsec / 1000
                                      );
                break;
            case (ACL_LOG_TIMESTAMP_MIC & ACL_LOG_TIMESTAMP_MASK):/* micro */
                (void)acpStrCatFormat(&aLog->mBuffer,
                                      "%04d-%02d-%02d %02d:%02d:%02d.%06d",
                                      sTimeExp.mYear,
                                      sTimeExp.mMonth,
                                      sTimeExp.mDay,
                                      sTimeExp.mHour,
                                      sTimeExp.mMin,
                                      sTimeExp.mSec,
                                      sTimeExp.mUsec);
                break;
        }

        *aNeedBlank = ACP_TRUE;
    }
    else
    {
        /* do nothing */
    }
}

ACP_INLINE void aclLogWriteHeaderUID(acl_log_t *aLog, acp_bool_t *aNeedBlank)
{
    acp_char_t sBuffer[33];
    acp_rc_t   sRC;

    if ((aLog->mFlag & ACL_LOG_UID) != 0)
    {
        sRC = acpSysGetUserName(sBuffer, sizeof(sBuffer));

        if (ACP_RC_IS_SUCCESS(sRC))
        {
            (void)acpStrCatFormat(&aLog->mBuffer,
                                  "%s%s",
                                  ((*aNeedBlank == ACP_TRUE) ? " " : ""),
                                  sBuffer);

            *aNeedBlank = ACP_TRUE;
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

ACP_INLINE void aclLogWriteHeaderPID(acl_log_t *aLog, acp_bool_t *aNeedBlank)
{
    if ((aLog->mFlag & ACL_LOG_PID) != 0)
    {
        (void)acpStrCatFormat(&aLog->mBuffer,
                              "%sP-%u",
                              ((*aNeedBlank == ACP_TRUE) ? " " : ""),
                              acpProcGetSelfID());

        *aNeedBlank = ACP_TRUE;
    }
    else
    {
        /* do nothing */
    }
}

ACP_INLINE void aclLogWriteHeaderTID(acl_log_t *aLog, acp_bool_t *aNeedBlank)
{
    if ((aLog->mFlag & ACL_LOG_TID) != 0)
    {
        (void)acpStrCatFormat(&aLog->mBuffer,
                              "%sT-%llu",
                              ((*aNeedBlank == ACP_TRUE) ? " " : ""),
                              acpThrGetSelfID());

        *aNeedBlank = ACP_TRUE;
    }
    else
    {
        /* do nothing */
    }
}

ACP_INLINE void aclLogWriteHeaderLogLevel(acl_log_t       *aLog,
                                          acl_log_level_t  aLevel,
                                          acp_bool_t      *aNeedBlank)
{
    if (((aLog->mFlag & ACL_LOG_LEVEL) != 0) && (aLevel < ACL_LOG_LEVEL_MAX))
    {
        if (aLog->mLevelNames != NULL)
        {
            (void)acpStrCatFormat(&aLog->mBuffer,
                                  "%s%s",
                                  ((*aNeedBlank == ACP_TRUE) ? " " : ""),
                                  aLog->mLevelNames[aLevel]);
        }
        else
        {
            (void)acpStrCatFormat(&aLog->mBuffer,
                                  "%sL-%d",
                                  ((*aNeedBlank == ACP_TRUE) ? " " : ""),
                                  aLevel);
        }

        *aNeedBlank = ACP_TRUE;
    }
    else
    {
        /* do nothing */
    }
}

ACP_INLINE void aclLogWriteHeaderAppMsg(acl_log_t *aLog, acp_bool_t *aNeedBlank)
{
    if (((aLog->mFlag & ACL_LOG_APPMSG) != 0) && (aLog->mAppMsg != NULL))
    {
        (void)acpStrCatFormat(&aLog->mBuffer,
                              "%s%s",
                              ((*aNeedBlank == ACP_TRUE) ? " " : ""),
                              aLog->mAppMsg);

        *aNeedBlank = ACP_TRUE;
    }
    else
    {
        /* do nothing */
    }
}

ACP_INLINE void aclLogWriteHeader(acl_log_t *aLog, acl_log_level_t aLevel)
{
    acp_bool_t sNeedBlank = ACP_FALSE;

    if ((aLog->mFlag & ACL_LOG_HEADER_MASK) != 0)
    {
        (void)acpStrCatCString(&aLog->mBuffer, "[");
    }
    else
    {
        /* do nothing */
    }

    aclLogWriteHeaderTimestamp(aLog, &sNeedBlank);
    aclLogWriteHeaderUID(aLog, &sNeedBlank);
    aclLogWriteHeaderPID(aLog, &sNeedBlank);
    aclLogWriteHeaderTID(aLog, &sNeedBlank);
    aclLogWriteHeaderLogLevel(aLog, aLevel, &sNeedBlank);
    aclLogWriteHeaderAppMsg(aLog, &sNeedBlank);

    if ((aLog->mFlag & ACL_LOG_HEADER_MASK) != 0)
    {
        (void)acpStrCatCString(&aLog->mBuffer, "] ");
    }
    else
    {
        /* do nothing */
    }
}

ACP_INLINE void aclLogWriteMessageWithArgs(acl_log_t        *aLog,
                                           acl_log_level_t   aLevel,
                                           const acp_char_t *aFormat,
                                           va_list           aArgs)
{
    if (aLog->mIsFileOpened == ACP_TRUE)
    {
        if (aLog->mIsHeaderWritten != ACP_TRUE)
        {
            aclLogWriteHeader(aLog, aLevel);

            aLog->mIsHeaderWritten = ACP_TRUE;
        }
        else
        {
            (void)acpStrCatCString(&aLog->mBuffer, "  ");
        }

        (void)acpStrCatFormatArgs(&aLog->mBuffer, aFormat, aArgs);
        (void)acpStrCatCString(&aLog->mBuffer, "\n");

        if ((acpStrGetLength(&aLog->mBuffer) > ACL_LOG_BUFFER_FLUSH_SIZE) ||
            (ACP_TRUE == aLog->mIsImmediateFlush))
        {
            aclLogFlush(aLog);
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

static void aclLogWriteMessage(acl_log_t        *aLog,
                               acl_log_level_t   aLevel,
                               const acp_char_t *aFormat, ...)
{
    va_list sArgs;

    va_start(sArgs, aFormat);

    aclLogWriteMessageWithArgs(aLog, aLevel, aFormat, sArgs);

    va_end(sArgs);
}


/**
 * opens a log file
 * @param aLog pointer to the log object
 * @param aPath log file path
 * @param aFlag flag for the log file
 * @param aBackupLimit the limit of the number of the backup files
 * @return result code
 *
 * @a aFlag is bit flag which composed of log header flags
 * (#ACL_LOG_TIMESTAMP | #ACL_LOG_UID | #ACL_LOG_PID | #ACL_LOG_TID |
 * #ACL_LOG_LEVEL) and log file sharing flags
 * (#ACL_LOG_PSHARED | #ACL_LOG_TSHARED)
 */
ACP_EXPORT acp_rc_t aclLogOpen(acl_log_t    *aLog,
                               acp_str_t    *aPath,
                               acp_uint32_t  aFlag,
                               acp_sint32_t  aBackupLimit)
{
    acp_rc_t sRC;

    ACP_STR_INIT_DYNAMIC(aLog->mPath,
                         (acp_sint32_t)acpStrGetLength(aPath) + 5,
                         16);
    ACP_STR_INIT_DYNAMIC(aLog->mBuffer,
                         ACL_LOG_BUFFER_INITIAL_SIZE,
                         ACL_LOG_BUFFER_EXTEND_SIZE);

    sRC = acpStrCpy(&aLog->mPath, aPath);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        ACP_STR_FINAL(aLog->mPath);
        ACP_STR_FINAL(aLog->mBuffer);

        return sRC;
    }
    else
    {
        /* do nothing */
    }

    if ((aFlag & ACL_LOG_TSHARED) != 0)
    {
        sRC = acpThrMutexCreate(&aLog->mMutex, ACP_THR_MUTEX_RECURSIVE);

        if (ACP_RC_NOT_SUCCESS(sRC))
        {
            ACP_STR_FINAL(aLog->mPath);
            ACP_STR_FINAL(aLog->mBuffer);

            return sRC;
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

    if ((aFlag & ACL_LOG_LEVEL_MASK) == 0)
    {
        aFlag |= ACL_LOG_LEVEL_MASK;
    }
    else
    {
        /* do nothing */
    }

    /* Immediate flushing */
    if(0 == (aFlag & ACL_LOG_IMMEDIATEFLUSH))
    {
        aLog->mIsImmediateFlush = ACP_FALSE;
    }
    else
    {
        aLog->mIsImmediateFlush = ACP_TRUE;
    }

    aLog->mBackupSize      = ACL_LOG_DEFAULT_BACKUP_SIZE;
    aLog->mFlag            = aFlag;
    aLog->mLockCount       = 0;
    aLog->mBackingUp       = 0;
    aLog->mIsHeaderWritten = ACP_FALSE;
    aLog->mIsFileOpened    = ACP_FALSE;
    aLog->mLevelNames      = NULL;
    aLog->mAppMsg          = NULL;

    (void)aclLogSetBackupLimit(aLog, aBackupLimit);
    (void)acpAtomicSet32(&(aLog->mBackupIndex), 0);

    return ACP_RC_SUCCESS;
}

/**
 * closes the log file
 * @param aLog pointer to the log object
 */
ACP_EXPORT void aclLogClose(acl_log_t *aLog)
{
    ACP_STR_FINAL(aLog->mPath);
    ACP_STR_FINAL(aLog->mBuffer);

    if ((aLog->mFlag & ACL_LOG_TSHARED) != 0)
    {
        (void)acpThrMutexDestroy(&aLog->mMutex);
    }
    else
    {
        /* do nothing */
    }
}

/**
 * locks the log file
 * @param aLog pointer to the log object
 */
ACP_EXPORT void aclLogLock(acl_log_t *aLog)
{
    acp_rc_t sRC;

    if ((aLog->mFlag & ACL_LOG_TSHARED) != 0)
    {
        sRC = acpThrMutexLock(&aLog->mMutex);

        ACE_DASSERT(ACP_RC_IS_SUCCESS(sRC));
    }
    else
    {
        /* do nothing */
    }

    if (aLog->mLockCount == 0)
    {
        aclLogFileOpen(aLog);

        aLog->mIsHeaderWritten = ACP_FALSE;
    }
    else
    {
        /* do nothing */
    }

    aLog->mLockCount++;
}

/**
 * unlocks the log file
 * @param aLog pointer to the log object
 */
ACP_EXPORT void aclLogUnlock(acl_log_t *aLog)
{
    acp_rc_t sRC;

    aLog->mLockCount--;

    if (aLog->mLockCount == 0)
    {
        aclLogFileClose(aLog);
    }
    else
    {
        /* do nothing */
    }

    if ((aLog->mFlag & ACL_LOG_TSHARED) != 0)
    {
        sRC = acpThrMutexUnlock(&aLog->mMutex);

        ACE_DASSERT(ACP_RC_IS_SUCCESS(sRC));
    }
    else
    {
        /* do nothing */
    }
}

/**
 * logs the current callstack
 * @param aLog pointer to the log object
 */
ACP_EXPORT void aclLogCallstack(acl_log_t *aLog)
{
    aclLogLock(aLog);

    /*
     * BUGBUG
     */
    aclLogWriteMessage(aLog,
                       ACL_LOG_LEVEL_MAX,
                       "callstack dump not yet implemented");

    aclLogUnlock(aLog);
}

/**
 * logs the exception
 * @param aLog pointer to the log object
 * @param aException pointer to the exception object
 *
 * if there is a context which contains exception object,
 * #ACL_LOG_CURRENT_EXCEPTION is more convenient way to log current exception
 */
ACP_EXPORT void aclLogException(acl_log_t *aLog, ace_exception_t *aException)
{
    ace_msgid_t  sError;
    acp_uint32_t sLevel;
    acp_uint32_t sLevelMask = 1;
#if defined(ACP_CFG_DEBUG)
    acp_sint32_t i;
#endif

    sError = aException->mErrorCode;
    sLevel = ACE_ERROR_LEVEL(sError);

    sLevelMask <<= sLevel;

    if (((sLevelMask & ACL_LOG_LEVEL_MASK) != 0) &&
        ((sLevelMask & aLog->mFlag) != 0))
    {
        aclLogLock(aLog);

#if defined(ACP_CFG_DEBUG)
        aclLogWriteMessage(aLog,
                           sLevel,
                           "Error %s",
                           aException->mErrorMsg);

        aclLogWriteMessage(aLog, sLevel, "==== Exception Stack Dump ====");

        for (i = aException->mExprCount - 1; i >= 0; i--)
        {
            aclLogWriteMessage(aLog,
                               sLevel,
                               "  %3d: %s",
                               aException->mExprCount - i,
                               aException->mExpr[i]);
        }

        aclLogWriteMessage(aLog, sLevel, "==== End of Dump ====");
#else
        if (aException->mExpr[0] != '\0')
        {
            aclLogWriteMessage(aLog,
                               sLevel,
                               "Error in %s %s",
                               aException->mExpr,
                               aException->mErrorMsg);
        }
        else
        {
            aclLogWriteMessage(aLog,
                               sLevel,
                               "Error %s",
                               aException->mErrorMsg);
        }
#endif

        aclLogUnlock(aLog);
    }
    else
    {
        /* do nothing */
    }
}

/**
 * logs a message
 * @param aLog pointer to the log object
 * @param aLevel level of log
 * @param aMsgID message id
 * @param ... variable number of arguments to be converted to log message
 */
ACP_EXPORT void aclLogMessage(acl_log_t       *aLog,
                              acl_log_level_t  aLevel,
                              ace_msgid_t      aMsgID, ...)
{
    va_list sArgs;

    va_start(sArgs, aMsgID);

    aclLogMessageWithArgs(aLog, aLevel, aMsgID, sArgs);

    va_end(sArgs);
}

/**
 * logs a message
 * @param aLog pointer to the log object
 * @param aLevel level of log
 * @param aMsgID message id
 * @param aArgs va_list of arguments to be converted to log message
 */
ACP_EXPORT void aclLogMessageWithArgs(acl_log_t       *aLog,
                                      acl_log_level_t  aLevel,
                                      ace_msgid_t      aMsgID,
                                      va_list          aArgs)
{
    acp_uint32_t sLevelMask = 1;

    sLevelMask <<= aLevel;

    if (((sLevelMask & ACL_LOG_LEVEL_MASK) != 0) &&
        ((sLevelMask & aLog->mFlag) != 0))
    {
        aclLogLock(aLog);

        aclLogWriteMessageWithArgs(aLog,
                                   aLevel,
                                   aceMsgTableGetLogMsgFormat(aMsgID),
                                   aArgs);

        aclLogUnlock(aLog);
    }
    else
    {
        /* do nothing */
    }
}

#if defined(ACP_CFG_DEBUG) || defined(ACP_CFG_DOXYGEN) || defined(ALINT)

/**
 * logs a debug message
 * @param aLog pointer to the log object
 * @param aFormat pointer to the format string
 * @param ... variable number of arguments to be converted to debug message
 *
 * this function is only for @b debug build mode or @b prerelease build mode.
 * in @b release build mode there is no effect
 */
ACP_EXPORT void aclLogDebugWrite(acl_log_t        *aLog,
                                 const acp_char_t *aFormat, ...)
{
    va_list sArgs;

    va_start(sArgs, aFormat);

    aclLogDebugWriteWithArgs(aLog, aFormat, sArgs);

    va_end(sArgs);
}

/**
 * logs a debug message
 * @param aLog pointer to the log object
 * @param aFormat pointer to the format string
 * @param aArgs va_list of arguments to be converted to debug message
 *
 * @see aclLogDebugWrite()
 */
ACP_EXPORT void aclLogDebugWriteWithArgs(acl_log_t        *aLog,
                                         const acp_char_t *aFormat,
                                         va_list           aArgs)
{
    aclLogLock(aLog);

    aclLogWriteMessageWithArgs(aLog, ACL_LOG_LEVEL_MAX, aFormat, aArgs);

    aclLogUnlock(aLog);
}

#else

ACP_EXPORT void aclLogDebugWriteBlank(acl_log_t        *aLog,
                                      const acp_char_t *aFormat, ...)
{
}

ACP_EXPORT void aclLogDebugWriteWithArgsBlank(acl_log_t        *aLog,
                                              const acp_char_t *aFormat,
                                              va_list           aArgs)
{
}

#endif
