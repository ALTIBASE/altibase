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
 * $Id: aclLog.h 11055 2010-05-11 07:07:56Z gurugio $
 ******************************************************************************/

#if !defined(_O_ACL_LOG_H_)
#define _O_ACL_LOG_H_

/**
 * @file
 * @ingroup CoreError
 */

#include <acpThrMutex.h>
#include <aceException.h>


ACP_EXTERN_C_BEGIN


/**
 * writes the exception of current context to the log file
 * @param aLog log file
 */
#define ACL_LOG_CURRENT_EXCEPTION(aLog)                         \
    aclLogException((aLog), ((aclContext *)aContext)->mException)


/*
 * log flags
 *
 * bit range : 31 - 24 | 23 - 22 | 21 - 16 | 15 - 0
 *
 *   31 - 24 : log message header
 *   23 - 22 : synchronization
 *   21      : flush log immediately after every write
 *   20 - 16 : reserved
 *   15 - 0  : log level mask
 */

/**
 * log flag: write timestamp in log header by second
 */
#define ACL_LOG_TIMESTAMP_SEC   0x80000000
/**
 * log flag: write timestamp in log header by minute
 */
#define ACL_LOG_TIMESTAMP_MIN   0x81000000
/**
 * log flag: write timestamp in log header by mili-second
 */
#define ACL_LOG_TIMESTAMP_MIL   0x82000000
/**
 * log flag: write timestamp in log header by micro-second
 */
#define ACL_LOG_TIMESTAMP_MIC   0x83000000
/**
 * log flag: mask timestamp in log header
 */
#define ACL_LOG_TIMESTAMP_MASK  0x03000000

/**
 * log flag: default timestamp in log header is by second
 */
#define ACL_LOG_TIMESTAMP       ACL_LOG_TIMESTAMP_SEC

/**
 * log flag; write user id in log header
 */
#define ACL_LOG_UID             0x40000000
/**
 * log flag; write process id in log header
 */
#define ACL_LOG_PID             0x20000000
/**
 * log flag; write thread id in log header
 */
#define ACL_LOG_TID             0x10000000
/**
 * log flag; write log level in log header
 */
#define ACL_LOG_LEVEL           0x08000000
/**
 * log flag; write application specific message in log header
 */
#define ACL_LOG_APPMSG          0x04000000
/**
 * log flag; process shared log file
 */
#define ACL_LOG_PSHARED         0x00800000
/**
 * log flag; thread shared log file
 */
#define ACL_LOG_TSHARED         0x00400000
/**
 * log flag; immediate flushing
 */
#define ACL_LOG_IMMEDIATEFLUSH  0x00200000

#define ACL_LOG_HEADER_MASK 0xff000000
#define ACL_LOG_LEVEL_MASK  0x0000ffff


/**
 * log level
 */
typedef enum acl_log_level_t
{
    ACL_LOG_LEVEL_0 = 0, /**< log level 0 */
    ACL_LOG_LEVEL_1,     /**< log level 1 */
    ACL_LOG_LEVEL_2,     /**< log level 2 */
    ACL_LOG_LEVEL_3,     /**< log level 3 */
    ACL_LOG_LEVEL_4,     /**< log level 4 */
    ACL_LOG_LEVEL_5,     /**< log level 5 */
    ACL_LOG_LEVEL_6,     /**< log level 6 */
    ACL_LOG_LEVEL_7,     /**< log level 7 */
    ACL_LOG_LEVEL_8,     /**< log level 8 */
    ACL_LOG_LEVEL_9,     /**< log level 9 */
    ACL_LOG_LEVEL_10,    /**< log level 10 */
    ACL_LOG_LEVEL_11,    /**< log level 11 */
    ACL_LOG_LEVEL_12,    /**< log level 12 */
    ACL_LOG_LEVEL_13,    /**< log level 13 */
    ACL_LOG_LEVEL_14,    /**< log level 14 */
    ACL_LOG_LEVEL_15,    /**< log level 15 */
    ACL_LOG_LEVEL_MAX
} acl_log_level_t;


/**
 * message log file
 */
typedef struct acl_log_t
{
    acp_thr_mutex_t    mMutex;
    acp_file_t         mFile;
    acp_str_t          mPath;
    acp_str_t          mBuffer;
    acp_offset_t       mBackupSize;
    acp_uint32_t       mFlag;
    acp_uint32_t       mLockCount;
    acp_bool_t         mIsFileOpened;
    acp_bool_t         mIsHeaderWritten;
    acp_bool_t         mIsImmediateFlush;
    const acp_char_t **mLevelNames;
    const acp_char_t  *mAppMsg;
    acp_sint32_t       mBackingUp;
    acp_sint32_t       mBackupLimit;
    acp_sint32_t       mBackupIndex;
} acl_log_t;

/**
 * Maximum value of the number of the log files to keep
 */
#define ACL_LOG_LIMIT_MAX ACP_SINT32_MAX
/**
 * Default value of the number of the log files to keep
 */
#define ACL_LOG_LIMIT_DEFAULT ACL_LOG_LIMIT_MAX


ACP_EXPORT acp_rc_t aclLogOpen(acl_log_t    *aLog,
                               acp_str_t    *aPath,
                               acp_uint32_t  aFlag,
                               acp_sint32_t  aBackupLimit);
ACP_EXPORT void     aclLogClose(acl_log_t *aLog);

/**
 * sets the backup size for the log file
 * @param aLog log object
 * @param aBackupSize backup size
 *
 * if @a aBackupSize is zero, the log file backup will not be performed
 */
ACP_INLINE void aclLogSetBackupSize(acl_log_t *aLog, acp_offset_t aBackupSize)
{
    aLog->mBackupSize = aBackupSize;
}

/**
 * sets the limit of the number of the log files
 * @param aLog log object
 * @param aLimit limit of log files
 *
 * if @a aLimit is zero, the log files will be backed up without limit
 */
ACP_INLINE void aclLogSetBackupLimit(acl_log_t *aLog, acp_sint32_t aLimit)
{
    (void)acpAtomicSet32(&(aLog->mBackupLimit), aLimit);
}

/**
 * gets the limit of the number of the log files
 * @param aLog log object
 */
ACP_INLINE acp_sint32_t aclLogGetBackupLimit(acl_log_t *aLog)
{
    return acpAtomicGet32(&(aLog->mBackupLimit));
}

/**
 * sets the level mask for the log file
 * @param aLog log object
 * @param aLevelMask level mask
 *
 * @a aLevelMask is the bit mask of active log level.
 * for example, to enable only level 0 and level 11, the mask should be
 * (1 << ACL_LOG_LEVEL_0) | (1 << ACL_LOG_LEVEL_11)
 *
 * the default is all log level.
 */
ACP_INLINE void aclLogSetLevelMask(acl_log_t *aLog, acp_uint32_t aLevelMask)
{
    aLog->mFlag =
        (aLog->mFlag & ~ACL_LOG_LEVEL_MASK) | (aLevelMask & ACL_LOG_LEVEL_MASK);
}

/**
 * sets the level names for the log file
 * @param aLog log object
 * @param aLevelNames array of 16 log level names
 * as C style null terminated string
 */
ACP_INLINE void aclLogSetLevelNames(acl_log_t         *aLog,
                                    const acp_char_t **aLevelNames)
{
    aLog->mLevelNames = aLevelNames;
}

/**
 * sets the application specific message for the log file
 * @param aLog log object
 * @param aAppMsg C style null terminated string of the message
 */
ACP_INLINE void aclLogSetAppMsg(acl_log_t *aLog, const acp_char_t *aAppMsg)
{
    aLog->mAppMsg = aAppMsg;
}

ACP_EXPORT void aclLogLock(acl_log_t *aLog);
ACP_EXPORT void aclLogUnlock(acl_log_t *aLog);

ACP_EXPORT void aclLogCallstack(acl_log_t *aLog);
ACP_EXPORT void aclLogException(acl_log_t *aLog, ace_exception_t *aException);

ACP_EXPORT void aclLogMessage(acl_log_t       *aLog,
                              acl_log_level_t  aLevel,
                              ace_msgid_t      aMsgID, ...);
ACP_EXPORT void aclLogMessageWithArgs(acl_log_t       *aLog,
                                      acl_log_level_t  aLevel,
                                      ace_msgid_t      aMsgID,
                                      va_list          aArgs);

ACP_EXPORT void aclLogFlush(acl_log_t *aLog);

#if defined(ACP_CFG_DEBUG) || defined(ACP_CFG_DOXYGEN) || defined(ALINT)

ACP_EXPORT void aclLogDebugWrite(acl_log_t        *aLog,
                                 const acp_char_t *aFormat, ...);
ACP_EXPORT void aclLogDebugWriteWithArgs(acl_log_t        *aLog,
                                         const acp_char_t *aFormat,
                                         va_list           aArgs);

#else

ACP_EXPORT void aclLogDebugWriteBlank(acl_log_t        *aLog,
                                      const acp_char_t *aFormat, ...);
ACP_EXPORT void aclLogDebugWriteWithArgsBlank(acl_log_t        *aLog,
                                              const acp_char_t *aFormat,
                                              va_list           aArgs);

#define aclLogDebugWrite         while (0) aclLogDebugWriteBlank
#define aclLogDebugWriteWithArgs while (0) aclLogDebugWriteWithArgsBlank

#endif


ACP_EXTERN_C_END


#endif
