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
 
/*
 * Copyright 2015, Altibase Corporation or its subsidiaries.
 * All rights reserved.
 */
#ifndef __OA_CONFIG_INTERFACE_H__
#define __OA_CONFIG_INTERFACE_H__

#ifdef ALTIADAPTER
#define OA_ADAPTER_HOME_ENVIRONMENT_VARIABLE "ALTIBASE_ADAPTER_HOME"
#define OA_ADAPTER_CONFIG_FILE_NAME "altiAdapter.conf"

#define OA_CONFIG_LOG_FILE_NAME_DEFAULT ("altiAdapter.trc")

#define OA_CONFIG_OTHER_ALTIBASE_HOST_DEFAULT ("127.0.0.1")
#define OA_CONFIG_OTHER_ALTIBASE_PORT_DEFAULT (20090)
#define OA_CONFIG_OTHER_ALTIBASE_PORT_MIN (1024)
#define OA_CONFIG_OTHER_ALTIBASE_PORT_MAX (65535)

#define OA_CONFIG_OTHER_ALTIBASE_USER_DEFAULT ("guest")
#define OA_CONFIG_OTHER_ALTIBASE_PASSWORD_DEFAULT ("guest")

typedef struct oaConfigAltibaseConfiguration oaConfigConfiguration;
typedef struct oaConfigHandleForAlti oaConfigHandle;

typedef struct oaConfigAltibaseConfiguration
{
    acp_str_t   *mHost;
    acp_sint32_t mPort;

    acp_str_t *mUser;

    acp_str_t *mPassword;

    acp_sint32_t mAutocommitMode;
    acp_uint32_t mArrayDMLMaxSize;

    acp_sint32_t mSkipInsert;
    acp_sint32_t mSkipUpdate;
    acp_sint32_t mSkipDelete;

    acp_uint32_t mConflictLoggingLevel;
    acp_uint32_t mGroupCommit;

    acp_uint32_t mSkipError;
    acp_uint32_t mErrorRetryCount;
    acp_uint32_t mErrorRetryInterval;
    
    acp_uint32_t mSetUserToTable;
    
    acp_uint32_t mAdapterErrorRestartCount;
    acp_uint32_t mAdapterErrorRestartInterval;
} oaConfigAltibaseConfiguration;

typedef struct oaConfigHandleForAlti
{

        ACP_STR_DECLARE_DYNAMIC(mLogFileName);

        acp_uint32_t mLogBackupLimit;

        acp_uint32_t mLogBackupSize;

        ACP_STR_DECLARE_DYNAMIC(mMessagePath);

        acp_sint32_t mPerformanceTickCheck;

        ACP_STR_DECLARE_DYNAMIC(mAlaSenderIP);

        acp_sint32_t mAlaSenderReplicationPort;

        acp_sint32_t mAlaReceiverPort;

        ACP_STR_DECLARE_DYNAMIC(mAlaReplicationName);

        ACP_STR_DECLARE_DYNAMIC(mAlaSocketType);

        ACP_STR_DECLARE_DYNAMIC(mAltiHost);

        acp_sint32_t mAltiPort;

        ACP_STR_DECLARE_DYNAMIC(mAltiUser);

        ACP_STR_DECLARE_DYNAMIC(mAltiPassword);

        acp_sint32_t mAlaXLogPoolSize;

        acp_uint32_t mAlaAckPerXLogCount;

        acp_sint32_t mAlaLoggingActive;

        ACP_STR_DECLARE_DYNAMIC(mAlaLogDirectory);

        ACP_STR_DECLARE_DYNAMIC(mAlaLogFileName);

        acp_uint32_t mAlaLogFileSize;

        acp_uint32_t mAlaMaxLogFileNumber;

        acp_uint32_t mAlaReceiveXlogTimeout;

        acp_sint32_t mAltiAutocommitMode;

        acp_uint32_t mAltiArrayDMLMaxSize;

        acp_sint32_t mAltiSkipInsert;
        acp_sint32_t mAltiSkipUpdate;
        acp_sint32_t mAltiSkipDelete;

        acp_uint32_t mAltiConflictLoggingLevel;
        acp_uint32_t mAltiGroupCommit;

        acp_uint32_t mSkipError;
        acp_uint32_t mErrorRetryCount;
        acp_uint32_t mErrorRetryInterval;

        acp_uint32_t mSetUserToTable;
        
        acp_uint32_t mAdapterErrorRestartCount;
        acp_uint32_t mAdapterErrorRestartInterval;

        ACP_STR_DECLARE_DYNAMIC( mConstraintAltibaseUser );
        ACP_STR_DECLARE_DYNAMIC( mConstraintAltibasePassword );
        ACP_STR_DECLARE_DYNAMIC( mConstraintAltibaseIP );
        acp_sint32_t mConstraintAltibasePort;
} oaConfigHandleForAlti;

#elif JDBCADAPTER
#define OA_ADAPTER_HOME_ENVIRONMENT_VARIABLE "JDBC_ADAPTER_HOME"
#define OA_ADAPTER_CONFIG_FILE_NAME "jdbcAdapter.conf"
#define OA_ADAPTER_SKIP_ERROR_INCLUDE_FILE_NAME "dbms_skip_error_include.list"
#define OA_ADAPTER_SKIP_ERROR_EXCLUDE_FILE_NAME "dbms_skip_error_exclude.list"
#define OA_ADAPTER_LOGIN_SQL_FILE_NAME "oalogin.sql"
#define OA_ADAPTER_STD_ERR_FILE_NAME "stderr.log"
#define OA_ADAPTER_SQL_READ_BUFF_SIZE   (512 * 1024)
#define OA_ADAPTER_SKIP_ERROR_BUFF_SIZE (32 * 1024)

#define OA_CONFIG_LOG_FILE_NAME_DEFAULT ("jdbcAdapter.trc")

#define OA_CONFIG_OTHER_DATABASE_USER_DEFAULT ("guest")
#define OA_CONFIG_OTHER_DATABASE_PASSWORD_DEFAULT ("guest")

typedef struct oaConfigJDBCConfiguration oaConfigConfiguration;
typedef struct oaConfigHandleForJDBC oaConfigHandle;

typedef struct oaConfigJDBCConfiguration
{
    acp_str_t *mUser;
    acp_str_t *mPassword;

    acp_sint32_t mAutocommitMode;
    acp_uint32_t mBatchDMLMaxSize;
    acp_uint32_t mGroupCommit;
    acp_uint32_t mXmxOpt;
    
    acp_sint32_t mSkipInsert;
    acp_sint32_t mSkipUpdate;
    acp_sint32_t mSkipDelete;

    acp_uint32_t mConflictLoggingLevel;
    acp_str_t   *mDriverPath;
    acp_str_t   *mDriverClass;
    acp_str_t   *mConnectionUrl;
    
    acp_uint32_t mSkipError;
    acp_uint32_t mErrorRetryCount;
    acp_uint32_t mErrorRetryInterval;
    
    acp_uint32_t mSetUserToTable;
    
    acp_uint32_t mSetColumnToInsert;
    
    acp_uint32_t mAdapterErrorRestartCount;
    acp_uint32_t mAdapterErrorRestartInterval;
    
    acp_str_t   *mJVMOpt;
} oaConfigJDBCConfiguration;

typedef struct oaConfigHandleForJDBC
{

        ACP_STR_DECLARE_DYNAMIC(mLogFileName);

        acp_uint32_t mLogBackupLimit;

        acp_uint32_t mLogBackupSize;

        ACP_STR_DECLARE_DYNAMIC(mMessagePath);

        acp_sint32_t mPerformanceTickCheck;

        ACP_STR_DECLARE_DYNAMIC(mAlaSenderIP);

        acp_sint32_t mAlaSenderReplicationPort;
                
        acp_sint32_t mAlaReceiverPort;

        ACP_STR_DECLARE_DYNAMIC(mAlaReplicationName);

        ACP_STR_DECLARE_DYNAMIC(mAlaSocketType);

        ACP_STR_DECLARE_DYNAMIC(mJDBCUser);

        ACP_STR_DECLARE_DYNAMIC(mJDBCPassword);

        acp_sint32_t mAlaXLogPoolSize;

        acp_uint32_t mAlaAckPerXLogCount;

        acp_sint32_t mAlaLoggingActive;

        ACP_STR_DECLARE_DYNAMIC(mAlaLogDirectory);

        ACP_STR_DECLARE_DYNAMIC(mAlaLogFileName);

        acp_uint32_t mAlaLogFileSize;

        acp_uint32_t mAlaMaxLogFileNumber;

        acp_uint32_t mAlaReceiveXlogTimeout;

        acp_sint32_t mJDBCAutocommitMode;

        acp_uint32_t mJDBCBatchDMLMaxSize;
        
        acp_sint32_t mJDBCSkipInsert;
        acp_sint32_t mJDBCSkipUpdate;
        acp_sint32_t mJDBCSkipDelete;

        acp_uint32_t mJDBCConflictLoggingLevel;
        acp_uint32_t mJDBCGroupCommit;
        acp_uint32_t mJDBCXmxOpt;
        
        acp_uint32_t mSkipError;
        acp_uint32_t mErrorRetryCount;
        acp_uint32_t mErrorRetryInterval;

        acp_uint32_t mSetUserToTable;

        acp_uint32_t mSetColumnToInsert;
        
        acp_uint32_t mAdapterErrorRestartCount;
        acp_uint32_t mAdapterErrorRestartInterval;
        
        ACP_STR_DECLARE_DYNAMIC( mJDBCDriverPath );
        ACP_STR_DECLARE_DYNAMIC( mJDBCDriverClass );
        ACP_STR_DECLARE_DYNAMIC( mJDBCConnectionUrl );

        ACP_STR_DECLARE_DYNAMIC( mConstraintAltibaseUser );
        ACP_STR_DECLARE_DYNAMIC( mConstraintAltibasePassword );
        ACP_STR_DECLARE_DYNAMIC( mConstraintAltibaseIP );
        acp_sint32_t mConstraintAltibasePort;
        
        ACP_STR_DECLARE_DYNAMIC( mJDBCJVMOpt );
} oaConfigHandleForJDBC;

#else
#define OA_ADAPTER_HOME_ENVIRONMENT_VARIABLE "ORA_ADAPTER_HOME"
#define OA_ADAPTER_CONFIG_FILE_NAME "oraAdapter.conf"
#define OA_ADAPTER_SKIP_ERROR_INCLUDE_FILE_NAME "dbms_skip_error_include.list"
#define OA_ADAPTER_SKIP_ERROR_EXCLUDE_FILE_NAME "dbms_skip_error_exclude.list"
#define OA_ADAPTER_LOGIN_SQL_FILE_NAME "oalogin.sql"
#define OA_ADAPTER_STD_ERR_FILE_NAME "stderr.log"
#define OA_ADAPTER_SQL_READ_BUFF_SIZE   (512 * 1024)
#define OA_ADAPTER_SKIP_ERROR_BUFF_SIZE (32 * 1024)

#define OA_CONFIG_LOG_FILE_NAME_DEFAULT ("oraAdapter.trc")

#define OA_CONFIG_ORACLE_SERVER_ALIAS_DEFAULT ("")
#define OA_CONFIG_ORACLE_USER_DEFAULT ("scott")
#define OA_CONFIG_ORACLE_PASSWORD_DEFAULT ("tiger")

typedef struct oaConfigOracleConfiguration oaConfigConfiguration;
typedef struct oaConfigHandleForOracle oaConfigHandle;

typedef struct oaConfigOracleConfiguration
{

    acp_str_t *mServerAlias;

    acp_str_t *mUser;

    acp_str_t *mPassword;

    acp_sint32_t mAutocommitMode;
    acp_sint32_t mAsynchronousCommit;
    acp_sint32_t mGroupCommit;

    acp_sint32_t mSkipInsert;
    acp_sint32_t mSkipUpdate;
    acp_sint32_t mSkipDelete;
    acp_uint32_t mSkipError;

    acp_sint32_t mDirectPathInsert;

    acp_uint32_t mArrayDMLMaxSize;
    acp_uint32_t mUpdateStatementCacheSize;

    acp_uint32_t mConflictLoggingLevel;

    acp_uint32_t mErrorRetryCount;
    acp_uint32_t mErrorRetryInterval;

    acp_uint32_t mSetUserToTable;
    
    acp_uint32_t mSetColumnToInsert;
    
    acp_uint32_t mAdapterErrorRestartCount;
    acp_uint32_t mAdapterErrorRestartInterval;

} oaConfigOracleConfiguration;

typedef struct oaConfigHandleForOracle
{

    ACP_STR_DECLARE_DYNAMIC(mLogFileName);

    acp_uint32_t mLogBackupLimit;

    acp_uint32_t mLogBackupSize;

    ACP_STR_DECLARE_DYNAMIC(mMessagePath);

    acp_sint32_t mPerformanceTickCheck;

    ACP_STR_DECLARE_DYNAMIC(mAlaSenderIP);

    acp_sint32_t mAlaSenderReplicationPort;

    acp_sint32_t mAlaReceiverPort;

    ACP_STR_DECLARE_DYNAMIC(mAlaReplicationName);

    ACP_STR_DECLARE_DYNAMIC(mAlaSocketType);

    ACP_STR_DECLARE_DYNAMIC(mOracleServerAlias);

    ACP_STR_DECLARE_DYNAMIC(mOracleUser);

    ACP_STR_DECLARE_DYNAMIC(mOraclePassword);

    acp_sint32_t mAlaXLogPoolSize;

    acp_uint32_t mAlaAckPerXLogCount;

    acp_sint32_t mAlaLoggingActive;

    ACP_STR_DECLARE_DYNAMIC(mAlaLogDirectory);

    ACP_STR_DECLARE_DYNAMIC(mAlaLogFileName);

    acp_uint32_t mAlaLogFileSize;

    acp_uint32_t mAlaMaxLogFileNumber;

    acp_uint32_t mAlaReceiveXlogTimeout;

    acp_sint32_t mOracleAutocommitMode;
    acp_sint32_t mOracleAsynchronousCommit;
    acp_sint32_t mOracleGroupCommit;

    acp_sint32_t mOracleSkipInsert;
    acp_sint32_t mOracleSkipUpdate;
    acp_sint32_t mOracleSkipDelete;

    acp_sint32_t mOracleDirectPathInsert;

    acp_uint32_t mOracleArrayDMLMaxSize;
    acp_uint32_t mOracleUpdateStatementCacheSize;

    acp_uint32_t mOracleConflictLoggingLevel;

    acp_uint32_t mSkipError;
    acp_uint32_t mErrorRetryCount;
    acp_uint32_t mErrorRetryInterval;

    acp_uint32_t mSetUserToTable;
    
    acp_uint32_t mSetColumnToInsert;
    
    acp_uint32_t mAdapterErrorRestartCount;
    acp_uint32_t mAdapterErrorRestartInterval;
    
    ACP_STR_DECLARE_DYNAMIC( mConstraintAltibaseUser );
    ACP_STR_DECLARE_DYNAMIC( mConstraintAltibasePassword );
    ACP_STR_DECLARE_DYNAMIC( mConstraintAltibaseIP );
    acp_sint32_t mConstraintAltibasePort;
} oaConfigHandleForOracle;

#endif

enum {

    OA_CONFIG_START = 0,

    OA_CONFIG_LOG_FILE_NAME,

    OA_CONFIG_LOG_BACKUP_LIMIT,

    OA_CONFIG_LOG_BACKUP_SIZE,

    OA_CONFIG_MESSAGE_PATH,

    OA_CONFIG_PERFORMANCE_TICK_CHECK,

    OA_CONFIG_ALA_SENDER_IP,

    OA_CONFIG_ALA_SENDER_REPLICATION_PORT,

    OA_CONFIG_ALA_RECEIVER_PORT,

    OA_CONFIG_ALA_REPLICATION_NAME,

    OA_CONFIG_ALA_SOCKET_TYPE,

    OA_CONFIG_ALA_XLOG_POOL_SIZE,

    OA_CONFIG_ALA_ACK_PER_XLOG_COUNT,

    OA_CONFIG_ALA_LOGGING_ACTIVE,

    OA_CONFIG_ALA_LOG_DIRECTORY,

    OA_CONFIG_ALA_LOG_FILE_NAME,

    OA_CONFIG_ALA_LOG_FILE_SIZE,

    OA_CONFIG_ALA_MAX_LOG_FILE_NUMBER,

    OA_CONFIG_ALA_RECEIVE_XLOG_TIMEOUT,

    OA_CONFIG_ERROR_RETRY_COUNT,

    OA_CONFIG_ERROR_RETRY_INTERVAL,

    OA_CONFIG_SKIP_ERROR,
    
    /* ALTI */
    OA_CONFIG_OTHER_ALTIBASE_HOST,

    OA_CONFIG_OTHER_ALTIBASE_PORT,

    OA_CONFIG_OTHER_ALTIBASE_USER,

    OA_CONFIG_OTHER_ALTIBASE_PASSWORD,

    OA_CONFIG_OTHER_ALTIBASE_AUTOCOMMIT_MODE,

    OA_CONFIG_OTHER_ALTIBASE_ARRAY_DML_MAX_SIZE,

    OA_CONFIG_OTHER_ALTIBASE_SKIP_INSERT,

    OA_CONFIG_OTHER_ALTIBASE_SKIP_UPDATE,

    OA_CONFIG_OTHER_ALTIBASE_SKIP_DELETE,

    OA_CONFIG_OTHER_ALTIBASE_CONFLICT_LOGGING_LEVEL,

    OA_CONFIG_OTHER_ALTIBASE_GROUP_COMMIT,
    
    OA_CONFIG_OTHER_ALTIBASE_SET_USER_TO_TABLE,    
    
    /* JDBC */
    OA_CONFIG_OTHER_DATABASE_USER,

    OA_CONFIG_OTHER_DATABASE_PASSWORD,

    OA_CONFIG_OTHER_DATABASE_AUTOCOMMIT_MODE,

    OA_CONFIG_OTHER_DATABASE_BATCH_DML_MAX_SIZE,

    OA_CONFIG_OTHER_DATABASE_SKIP_INSERT,

    OA_CONFIG_OTHER_DATABASE_SKIP_UPDATE,

    OA_CONFIG_OTHER_DATABASE_SKIP_DELETE,

    OA_CONFIG_OTHER_DATABASE_CONFLICT_LOGGING_LEVEL,
    
    OA_CONFIG_OTHER_DATABASE_GROUP_COMMIT,
    
    OA_CONFIG_OTHER_DATABASE_JDBC_DRIVER_PATH,
    
    OA_CONFIG_OTHER_DATABASE_JDBC_DRIVER_CLASS,
    
    OA_CONFIG_OTHER_DATABASE_JDBC_CONNECTION_URL,

    OA_CONFIG_OTHER_DATABASE_JDBC_MAX_HEAP_SIZE,
    
    OA_CONFIG_OTHER_DATABASE_SET_USER_TO_TABLE,
    
    OA_CONFIG_OTHER_DATABASE_SET_COLUMN_TO_INSERT,
    
    OA_CONFIG_OTHER_DATABASE_JDBC_JVM_OPTION,
    
    /* ORACLE */
    OA_CONFIG_ORACLE_SERVER_ALIAS,

    OA_CONFIG_ORACLE_USER,

    OA_CONFIG_ORACLE_PASSWORD,

    OA_CONFIG_ORACLE_AUTOCOMMIT_MODE,

    OA_CONFIG_ORACLE_ASYNCHRONOUS_COMMIT,

    OA_CONFIG_ORACLE_GROUP_COMMIT,

    OA_CONFIG_ORACLE_SKIP_INSERT,

    OA_CONFIG_ORACLE_SKIP_UPDATE,

    OA_CONFIG_ORACLE_SKIP_DELETE,

    OA_CONFIG_ORACLE_DIRECT_PATH_INSERT,

    OA_CONFIG_ORACLE_ARRAY_DML_MAX_SIZE,

    OA_CONFIG_ORACLE_UPDATE_STATEMENT_CACHE_SIZE,

    OA_CONFIG_ORACLE_CONFLICT_LOGGING_LEVEL,
    
    OA_CONFIG_ORACLE_SET_USER_TO_TABLE,
    
    OA_CONFIG_ORACLE_SET_COLUMN_TO_INSERT,
    
    OA_CONFIG_CONSTRAINT_ALTIBASE_USER,
    OA_CONFIG_CONSTRAINT_ALTIBASE_PASSWORD,
    OA_CONFIG_CONSTRAINT_ALTIBASE_IP,
    OA_CONFIG_CONSTRAINT_ALTIBASE_PORT,

    /* ADAPTER */
    OA_CONFIG_ADAPTER_ERROR_RESTART_COUNT,
    OA_CONFIG_ADAPTER_ERROR_RESTART_INTERVAL,
};

#endif /* __OA_CONFIG_INTERFACE_H__ */
