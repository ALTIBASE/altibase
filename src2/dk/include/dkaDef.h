/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 

/***********************************************************************
 * $id$
 **********************************************************************/

#ifndef _O_DKA_DEF_H_
#define _O_DKA_DEF_H_ 1

#include <idTypes.h>
#include <dkDef.h>


/* Altilinker process enadle/disable */
#define DKA_ALTILINKER_ENABLE           (1)
#define DKA_ALTILINKER_DISABLE          (0)

/* AltiLinker process shutdown flags */
#define DKA_FLAG_SHUTDOWN_FORCE         (0x00000001)
#define DKA_FLAG_SHUTDOWN_NORMAL        (0x00000000)

#define DKA_ALTILINKER_PORT_NO          (20500)

/* AltiLinker process startup command string */
#define DKA_CMD_STR_LEN                 DK_PATH_LEN
#define DKA_CMD_STR_JAVA_HOME           (SChar *)"JAVA_HOME"
#define DKA_CMD_STR_JAVA                "java"
#define DKA_CMD_STR_XMS                 "-Xms"
#define DKA_CMD_STR_XMX                 "-Xmx"
#define DKA_CMD_STR_MB                  "m"
#define DKA_CMD_STR_JAR                 "-jar"
#define DKA_CMD_STR_SPACE               " "
#define DKA_CMD_STR_FILE_NAME           "altilinker.jar"
#define DKA_CMD_STR_LISTEN_PORT         "-listen_port"
#define DKA_CMD_STR_EQUAL               "="
#define DKA_CMD_STR_TRC_DIR             "-trc_dir"
#define DKA_CMD_STR_32BIT               "-d32"
#define DKA_CMD_STR_64BIT               "-d64"

#ifdef ALTIBASE_PRODUCT_XDB
#define DKA_CMD_STR_TRC_FILE_NAME       "-trc_file_name=xdbaltibase_lk.log"
#else
#define DKA_CMD_STR_TRC_FILE_NAME       "-trc_file_name=altibase_lk.log"
#endif /* ALTIBASE_PRODUCT_XDB */


/* AltiLinker property default values */
#define DKA_LINKER_LOG_FILE_PATH_LEN    DK_PATH_LEN

/* ------------------------------------------------
 * AltiLinker process status
 * ----------------------------------------------*/
typedef enum
{
    DKA_LINKER_STATUS_NON = 0,
    DKA_LINKER_STATUS_ACTIVATED,    /* AltiLinker 프로세스를 띄운 상태 */
    DKA_LINKER_STATUS_READY,        /* Linker control session 을 맺은 상태 */
    DKA_LINKER_STATUS_PROCESSING,   /* 프로토콜을 수행중인 상태 */
    DKA_LINKER_STATUS_CLOSING       /* AltiLinker 가 shutdown 중인 상태 */
} dkaLinkerStatus;

/* ------------------------------------------------
 * AltiLinker process trace logging level
 * ----------------------------------------------*/
typedef enum
{
    DKA_LINKER_LOGGING_LEVEL_OFF = 0,
    DKA_LINKER_LOGGING_LEVEL_FATAL,
    DKA_LINKER_LOGGING_LEVEL_ERROR,
    DKA_LINKER_LOGGING_LEVEL_WARNING,   /* default */
    DKA_LINKER_LOGGING_LEVEL_INFO,
    DKA_LINKER_LOGGING_LEVEL_DEBUG,
    DKA_LINKER_LOGGING_LEVEL_TRACE,
    DKA_LINKER_LOGGING_LEVEL_MAX
} dkaLinkerLoggingLevel;

/* Define AltiLinker process information for V$DBLINK_ALTILINKER_STATUS */
typedef struct dkaLinkerProcInfo
{
    dkaLinkerStatus  mStatus;                /* AltiLinker 의 현재 상태 */
    UInt             mLinkerSessionCnt;      /* All linker session count */
    UInt             mRemoteNodeSessionCnt;  /* All remote node session count */
    UInt             mJvmMemMaxSize;         /* JVM memory 의 최대 가용크기 (MB) */
    SLong            mJvmMemUsage;           /* JVM memory 현재 사용량 (byte) */
    SChar            mStartTime[DK_TIME_STR_LEN]; /* AltiLinker 의 start time */
} dkaLinkerProcInfo;

/* Define target remote server information */
typedef struct dkaTargetInfo
{
    SChar   mTargetName[DK_NAME_LEN + 1];
    SChar   mJdbcDriverPath[DK_PATH_LEN + 1];
    SChar   mJdbcDriverClassName[DK_DRIVER_CLASS_NAME_LEN + 1];
    SChar   mRemoteServerUrl[DK_URL_LEN + 1];
    SChar   mRemoteServerUserID[DK_USER_ID_LEN + 1];
    SChar   mRemoteServerPasswd[DK_USER_PW_LEN + 1];
    SChar   mXADataSourceClassName[DK_PATH_LEN + 1];
    SChar   mXADataSourceUrlSetterName[DK_NAME_LEN + 1];
    struct dkaTargetInfo  *mNext;
} dkaTargetInfo;


#endif /* _O_DKA_DEF_H_ */
