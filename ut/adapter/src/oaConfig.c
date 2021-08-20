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
 * Copyright 2011, Altibase Corporation or its subsidiaries.
 * All rights reserved.
 *
 * Example
 *
 *     LOG_FILE_NAME = oraAdapter.trc
 *     MESSAGE_PATH = msg
 *
 *     # Configurations for ALA
 *     ALA_SENDER_IP = 192.168.1.83
 *     ALA_RECEIVER_PORT = 49167
 *     ALA_REPLICATION_NAME = ALA
 *
 *     # Configurations for Oracle
 *     ORACLE_USER = scott
 *     ORACLE_PASSWORD = tiger
 *
 *     # Optional configurations for ALA
 *     ALA_XLOG_POOL_SIZE = 10000
 *     ALA_ACK_PER_XLOG_COUNT = 100
 *
 *     # Enable ALA logging
 *     ALA_LOGGING_ACTIVE = 1
 *     ALA_LOG_DIRECTORY = ./log
 *     ALA_LOG_FILE_NAME = ala.log
 *     ALA_LOG_FILE_SIZE = 10485760
 *     ALA_MAX_LOG_FILE_NUMBER = 10
 */
#include <oaContext.h>
#include <oaConfig.h>
#include <aciTypes.h>

#define OA_CONFIG_LOG_BACKUP_LIMIT_DEFAULT (10)
#define OA_CONFIG_LOG_BACKUP_LIMIT_MIN     (0)
#define OA_CONFIG_LOG_BACKUP_LIMIT_MAX     (2147483647)
#define OA_CONFIG_LOG_BACKUP_SIZE_DEFAULT  (10 * 1024 * 1024)
#define OA_CONFIG_LOG_BACKUP_SIZE_MIN      (0)
#define OA_CONFIG_LOG_BACKUP_SIZE_MAX      (2147483647)

#define OA_CONFIG_MESSAGE_PATH_DEFAULT ("msg")

#define OA_CONFIG_PERFORMANCE_TICK_CHECK_DEFAULT (0)
#define OA_CONFIG_PERFORMANCE_TICK_CHECK_MIN     (0)
#define OA_CONFIG_PERFORMANCE_TICK_CHECK_MAX     (1)

#define OA_CONFIG_ALA_SENDER_IP_DEFAULT ("127.0.0.1")

#define OA_CONFIG_ALA_SENDER_REPLICATION_PORT_DEFAULT (0)
#define OA_CONFIG_ALA_SENDER_REPLICATION_PORT_MIN     (0)
#define OA_CONFIG_ALA_SENDER_REPLICATION_PORT_MAX     (65535)


#define OA_CONFIG_ALA_RECEIVER_PORT_DEFAULT (47146)
#define OA_CONFIG_ALA_RECEIVER_PORT_MIN (1024)
#define OA_CONFIG_ALA_RECEIVER_PORT_MAX (65535)

#define OA_CONFIG_ALA_REPLICATION_NAME_DEFAULT ("ala")

#define OA_CONFIG_ALA_SOCKET_TYPE_DEFAULT ("TCP")

#define OA_CONFIG_ALA_XLOG_POOL_SIZE_DEFAULT (100000)
#define OA_CONFIG_ALA_XLOG_POOL_SIZE_MIN (1)
/* TODO: Is there no problem? this is maximum value of sint32 */
#define OA_CONFIG_ALA_XLOG_POOL_SIZE_MAX (2147483647)

#define OA_CONFIG_ALA_ACK_PER_XLOG_COUNT_DEFAULT (100)
#define OA_CONFIG_ALA_ACK_PER_XLOG_COUNT_MIN (1)
/* TODO: Is there no problem? this is maximum value of sint32 */
#define OA_CONFIG_ALA_ACK_PER_XLOG_COUNT_MAX (2147483647)

#define OA_CONFIG_ALA_LOGGING_ACTIVE_DEFAULT (1)
#define OA_CONFIG_ALA_LOGGING_ACTIVE_MIN (0)
#define OA_CONFIG_ALA_LOGGING_ACTIVE_MAX (1)

#define OA_CONFIG_ALA_LOG_DIRECTORY_DEFAULT ("")

#define OA_CONFIG_ALA_LOG_FILE_NAME_DEFAULT ("ala.log")

#define OA_CONFIG_ALA_LOG_FILE_SIZE_DEFAULT (10 * 1024 * 1024)
#define OA_CONFIG_ALA_LOG_FILE_SIZE_MIN (0)
/* TODO: Is there no problem? this is maximum value of uint32 */
#define OA_CONFIG_ALA_LOG_FILE_SIZE_MAX (4294967295)

#define OA_CONFIG_ALA_MAX_LOG_FILE_NUMBER_DEFAULT (10)
#define OA_CONFIG_ALA_MAX_LOG_FILE_NUMBER_MIN (1)
/* TODO: Is there no problem? this is maximum value of uint32 */
#define OA_CONFIG_ALA_MAX_LOG_FILE_NUMBER_MAX (4294967295)

#define OA_CONFIG_ALA_RECEIVE_XLOG_TIMEOUT_DEFAULT (300)
#define OA_CONFIG_ALA_RECEIVE_XLOG_TIMEOUT_MIN (1)
#define OA_CONFIG_ALA_RECEIVE_XLOG_TIMEOUT_MAX (4294967295)

/* BUG-43540 */
#define OA_CONFIG_ERROR_RETRY_COUNT_DEFAULT (0) 
#define OA_CONFIG_ERROR_RETRY_COUNT_MIN (0) 
#define OA_CONFIG_ERROR_RETRY_COUNT_MAX (65535) 

#define OA_CONFIG_ERROR_RETRY_INTERVAL_DEFAULT (0) /* sec */
#define OA_CONFIG_ERROR_RETRY_INTERVAL_MIN (0)
#define OA_CONFIG_ERROR_RETRY_INTERVAL_MAX (65535) 

#define OA_CONFIG_SKIP_ERROR_DEFAULT (1) 
#define OA_CONFIG_SKIP_ERROR_MIN (0) 
#define OA_CONFIG_SKIP_ERROR_MAX (1)

#ifdef ALTIADAPTER
#define OA_CONFIG_OTHER_ALTIBASE_AUTOCOMMIT_MODE_DEFAULT (0)
#define OA_CONFIG_OTHER_ALTIBASE_AUTOCOMMIT_MODE_MIN (0)
#define OA_CONFIG_OTHER_ALTIBASE_AUTOCOMMIT_MODE_MAX (1)

#define OA_CONFIG_OTHER_ALTIBASE_ARRAY_DML_MAX_SIZE_DEFAULT (10)
#define OA_CONFIG_OTHER_ALTIBASE_ARRAY_DML_MAX_SIZE_MIN (1)
#define OA_CONFIG_OTHER_ALTIBASE_ARRAY_DML_MAX_SIZE_MAX (32767)

#define OA_CONFIG_OTHER_ALTIBASE_SKIP_INSERT_DEFAULT (0)
#define OA_CONFIG_OTHER_ALTIBASE_SKIP_INSERT_MIN (0)
#define OA_CONFIG_OTHER_ALTIBASE_SKIP_INSERT_MAX (1)

#define OA_CONFIG_OTHER_ALTIBASE_SKIP_UPDATE_DEFAULT (0)
#define OA_CONFIG_OTHER_ALTIBASE_SKIP_UPDATE_MIN (0)
#define OA_CONFIG_OTHER_ALTIBASE_SKIP_UPDATE_MAX (1)

#define OA_CONFIG_OTHER_ALTIBASE_SKIP_DELETE_DEFAULT (0)
#define OA_CONFIG_OTHER_ALTIBASE_SKIP_DELETE_MIN (0)
#define OA_CONFIG_OTHER_ALTIBASE_SKIP_DELETE_MAX (1)

#define OA_CONFIG_OTHER_ALTIBASE_CONFLICT_LOGGING_LEVEL_DEFAULT (2)
#define OA_CONFIG_OTHER_ALTIBASE_CONFLICT_LOGGING_LEVEL_MIN (0)
#define OA_CONFIG_OTHER_ALTIBASE_CONFLICT_LOGGING_LEVEL_MAX (2)

#define OA_CONFIG_OTHER_ALTIBASE_GROUP_COMMIT_DEFAULT (1)
#define OA_CONFIG_OTHER_ALTIBASE_GROUP_COMMIT_MIN (0)
#define OA_CONFIG_OTHER_ALTIBASE_GROUP_COMMIT_MAX (1)

#define OA_CONFIG_OTHER_ALTIBASE_SET_USER_TO_TABLE_DEFAULT (1) 
#define OA_CONFIG_OTHER_ALTIBASE_SET_USER_TO_TABLE_MIN (0) 
#define OA_CONFIG_OTHER_ALTIBASE_SET_USER_TO_TABLE_MAX (1)

#elif JDBCADAPTER

#define OA_CONFIG_OTHER_DATABASE_AUTOCOMMIT_MODE_DEFAULT (0)
#define OA_CONFIG_OTHER_DATABASE_AUTOCOMMIT_MODE_MIN (0)
#define OA_CONFIG_OTHER_DATABASE_AUTOCOMMIT_MODE_MAX (1)

#define OA_CONFIG_OTHER_DATABASE_BATCH_DML_MAX_SIZE_DEFAULT (10)
#define OA_CONFIG_OTHER_DATABASE_BATCH_DML_MAX_SIZE_MIN (1)
#define OA_CONFIG_OTHER_DATABASE_BATCH_DML_MAX_SIZE_MAX (32767)

#define OA_CONFIG_OTHER_DATABASE_SKIP_INSERT_DEFAULT (0)
#define OA_CONFIG_OTHER_DATABASE_SKIP_INSERT_MIN (0)
#define OA_CONFIG_OTHER_DATABASE_SKIP_INSERT_MAX (1)

#define OA_CONFIG_OTHER_DATABASE_SKIP_UPDATE_DEFAULT (0)
#define OA_CONFIG_OTHER_DATABASE_SKIP_UPDATE_MIN (0)
#define OA_CONFIG_OTHER_DATABASE_SKIP_UPDATE_MAX (1)

#define OA_CONFIG_OTHER_DATABASE_SKIP_DELETE_DEFAULT (0)
#define OA_CONFIG_OTHER_DATABASE_SKIP_DELETE_MIN (0)
#define OA_CONFIG_OTHER_DATABASE_SKIP_DELETE_MAX (1)

#define OA_CONFIG_OTHER_DATABASE_CONFLICT_LOGGING_LEVEL_DEFAULT (2)
#define OA_CONFIG_OTHER_DATABASE_CONFLICT_LOGGING_LEVEL_MIN (0)
#define OA_CONFIG_OTHER_DATABASE_CONFLICT_LOGGING_LEVEL_MAX (2)

#define OA_CONFIG_OTHER_DATABASE_GROUP_COMMIT_DEFAULT (1)
#define OA_CONFIG_OTHER_DATABASE_GROUP_COMMIT_MIN (0)
#define OA_CONFIG_OTHER_DATABASE_GROUP_COMMIT_MAX (1)

#define OA_CONFIG_OTHER_DATABASE_JDBC_MAX_HEAP_SIZE_DEFAULT (2 * 1024)
#define OA_CONFIG_OTHER_DATABASE_JDBC_MAX_HEAP_SIZE_MIN (0)
#define OA_CONFIG_OTHER_DATABASE_JDBC_MAX_HEAP_SIZE_MAX (10 * 1024)

#define OA_CONFIG_OTHER_DATABASE_JDBC_DRIVER_PATH_DEFAULT ("")

#define OA_CONFIG_OTHER_DATABASE_JDBC_DRIVER_CLASS_DEFAULT ("")

#define OA_CONFIG_OTHER_DATABASE_JDBC_CONNECTION_URL_DEFAULT ("")

#define OA_CONFIG_OTHER_DATABASE_SET_USER_TO_TABLE_DEFAULT (0) 
#define OA_CONFIG_OTHER_DATABASE_SET_USER_TO_TABLE_MIN (0) 
#define OA_CONFIG_OTHER_DATABASE_SET_USER_TO_TABLE_MAX (1)

#define OA_CONFIG_OTHER_DATABASE_SET_COLUMN_TO_INSERT_DEFAULT (1) 
#define OA_CONFIG_OTHER_DATABASE_SET_COLUMN_TO_INSERT_MIN (0) 
#define OA_CONFIG_OTHER_DATABASE_SET_COLUMN_TO_INSERT_MAX (1)

#define OA_CONFIG_OTHER_DATABASE_JDBC_JVM_OPTION_DEFAULT ("")

#else

/* BUG-32168 add autocommit mode property */
#define OA_CONFIG_ORACLE_AUTOCOMMIT_MODE_DEFAULT (0)
#define OA_CONFIG_ORACLE_AUTOCOMMIT_MODE_MIN (0)
#define OA_CONFIG_ORACLE_AUTOCOMMIT_MODE_MAX (1)

#define OA_CONFIG_ORACLE_ASYNCHRONOUS_COMMIT_DEFAULT (1)
#define OA_CONFIG_ORACLE_ASYNCHRONOUS_COMMIT_MIN (0)
#define OA_CONFIG_ORACLE_ASYNCHRONOUS_COMMIT_MAX (1)

#define OA_CONFIG_ORACLE_GROUP_COMMIT_DEFAULT (1)
#define OA_CONFIG_ORACLE_GROUP_COMMIT_MIN (0)
#define OA_CONFIG_ORACLE_GROUP_COMMIT_MAX (1)

#define OA_CONFIG_ORACLE_SKIP_INSERT_DEFAULT (0)
#define OA_CONFIG_ORACLE_SKIP_INSERT_MIN (0)
#define OA_CONFIG_ORACLE_SKIP_INSERT_MAX (1)

#define OA_CONFIG_ORACLE_SKIP_UPDATE_DEFAULT (0)
#define OA_CONFIG_ORACLE_SKIP_UPDATE_MIN (0)
#define OA_CONFIG_ORACLE_SKIP_UPDATE_MAX (1)

#define OA_CONFIG_ORACLE_SKIP_DELETE_DEFAULT (0)
#define OA_CONFIG_ORACLE_SKIP_DELETE_MIN (0)
#define OA_CONFIG_ORACLE_SKIP_DELETE_MAX (1)

#define OA_CONFIG_ORACLE_DIRECT_PATH_INSERT_DEFAULT (0)
#define OA_CONFIG_ORACLE_DIRECT_PATH_INSERT_MIN (0)
#define OA_CONFIG_ORACLE_DIRECT_PATH_INSERT_MAX (1)

#define OA_CONFIG_ORACLE_ARRAY_DML_MAX_SIZE_DEFAULT (10)
#define OA_CONFIG_ORACLE_ARRAY_DML_MAX_SIZE_MIN (1)
#define OA_CONFIG_ORACLE_ARRAY_DML_MAX_SIZE_MAX (32767)

#define OA_CONFIG_ORACLE_UPDATE_STATEMENT_CACHE_SIZE_DEFAULT (20)
#define OA_CONFIG_ORACLE_UPDATE_STATEMENT_CACHE_SIZE_MIN (0)
#define OA_CONFIG_ORACLE_UPDATE_STATEMENT_CACHE_SIZE_MAX (4294967295)

#define OA_CONFIG_ORACLE_CONFLICT_LOGGING_LEVEL_DEFAULT (2)
#define OA_CONFIG_ORACLE_CONFLICT_LOGGING_LEVEL_MIN (0)
#define OA_CONFIG_ORACLE_CONFLICT_LOGGING_LEVEL_MAX (2)

#define OA_CONFIG_ORACLE_SET_USER_TO_TABLE_DEFAULT (1) 
#define OA_CONFIG_ORACLE_SET_USER_TO_TABLE_MIN (0) 
#define OA_CONFIG_ORACLE_SET_USER_TO_TABLE_MAX (1)

#define OA_CONFIG_ORACLE_SET_COLUMN_TO_INSERT_DEFAULT (1) 
#define OA_CONFIG_ORACLE_SET_COLUMN_TO_INSERT_MIN (0) 
#define OA_CONFIG_ORACLE_SET_COLUMN_TO_INSERT_MAX (1)

#endif

#define OA_CONFIG_CONSTRAINT_ALTIBASE_USER_DEFAULT ("guest")
#define OA_CONFIG_CONSTRAINT_ALTIBASE_PASSWORD_DEFAULT ("guest")
#define OA_CONFIG_CONSTRAINT_ALTIBASE_IP_DEFAULT ("127.0.0.1")

#define OA_CONFIG_CONSTRAINT_ALTIBASE_PORT_DEFAULT (20090)
#define OA_CONFIG_CONSTRAINT_ALTIBASE_PORT_MIN (1024)
#define OA_CONFIG_CONSTRAINT_ALTIBASE_PORT_MAX (65535)

/* ADAPTER */
#define OA_CONFIG_ADAPTER_ERROR_RESTART_COUNT_DEFAULT (0)
#define OA_CONFIG_ADAPTER_ERROR_RESTART_COUNT_MIN (0)
#define OA_CONFIG_ADAPTER_ERROR_RESTART_COUNT_MAX (65535)

#define OA_CONFIG_ADAPTER_ERROR_RESTART_INTERVAL_DEFAULT (0)
#define OA_CONFIG_ADAPTER_ERROR_RESTART_INTERVAL_MIN (0)
#define OA_CONFIG_ADAPTER_ERROR_RESTART_INTERVAL_MAX (65535)

static acp_sint32_t oaConfigCallbackLogFileName(acp_sint32_t aDepth,
                                                acl_conf_key_t *aKey,
                                                acp_sint32_t aLineNumber,
                                                void *aValue,
                                                void *aContext)
{
    oaConfigHandle *sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED(aDepth);
    ACP_UNUSED(aKey);
    ACP_UNUSED(aLineNumber);


    (void)acpStrCpy(&(sHandle->mLogFileName), (acp_str_t *)aValue);

    return 0;
}

static acp_sint32_t oaConfigCallbackLogBackupLimit( acp_sint32_t aDepth,
                                                    acl_conf_key_t *aKey,
                                                    acp_sint32_t aLineNumber,
                                                    void *aValue,
                                                    void *aContext )
{
    oaConfigHandle *sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED(aDepth);
    ACP_UNUSED(aKey);
    ACP_UNUSED(aLineNumber);

    sHandle->mLogBackupLimit = *(acp_uint32_t *)aValue;

    return 0;
}

static acp_sint32_t oaConfigCallbackLogBackupSize( acp_sint32_t aDepth,
                                                   acl_conf_key_t *aKey,
                                                   acp_sint32_t aLineNumber,
                                                   void *aValue,
                                                   void *aContext )
{
    oaConfigHandle *sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED(aDepth);
    ACP_UNUSED(aKey);
    ACP_UNUSED(aLineNumber);

    sHandle->mLogBackupSize = *(acp_uint32_t *)aValue;

    return 0;
}

static acp_sint32_t oaConfigCallbackMessagePath(acp_sint32_t aDepth,
                                                acl_conf_key_t *aKey,
                                                acp_sint32_t aLineNumber,
                                                void *aValue,
                                                void *aContext)
{
    oaConfigHandle *sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED(aDepth);
    ACP_UNUSED(aKey);
    ACP_UNUSED(aLineNumber);

    (void)acpStrCpy(&(sHandle->mMessagePath), (acp_str_t *)aValue);

    return 0;
}

static acp_sint32_t oaConfigCallbackPerformanceTickCheck( acp_sint32_t aDepth,
                                                          acl_conf_key_t *aKey,
                                                          acp_sint32_t aLineNumber,
                                                          void *aValue,
                                                          void *aContext )
{
    oaConfigHandle *sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED(aDepth);
    ACP_UNUSED(aKey);
    ACP_UNUSED(aLineNumber);

    sHandle->mPerformanceTickCheck = *(acp_sint32_t *)aValue;

    return 0;
}

static acp_sint32_t oaConfigCallbackAlaSenderIP(acp_sint32_t aDepth,
                                                acl_conf_key_t *aKey,
                                                acp_sint32_t aLineNumber,
                                                void *aValue,
                                                void *aContext)
{
    oaConfigHandle *sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED(aDepth);
    ACP_UNUSED(aKey);
    ACP_UNUSED(aLineNumber);

    /* TODO: Check value is a IP address */

    (void)acpStrCpy(&(sHandle->mAlaSenderIP), (acp_str_t *)aValue);

    return 0;
}

static acp_sint32_t oaConfigCallbackAlaSenderReplicationPort( acp_sint32_t     aDepth,
                                                              acl_conf_key_t * aKey,
                                                              acp_sint32_t     aLineNumber,
                                                              void           * aValue,
                                                              void           * aContext )
{
    oaConfigHandle *sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED( aDepth );
    ACP_UNUSED( aKey );
    ACP_UNUSED( aLineNumber );

    sHandle->mAlaSenderReplicationPort = *(acp_sint32_t *)aValue;

    return 0;
}

static acp_sint32_t oaConfigCallbackAlaReceiverPort(acp_sint32_t aDepth,
                                                    acl_conf_key_t *aKey,
                                                    acp_sint32_t aLineNumber,
                                                    void *aValue,
                                                    void *aContext)
{
    oaConfigHandle *sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED(aDepth);
    ACP_UNUSED(aKey);
    ACP_UNUSED(aLineNumber);

    sHandle->mAlaReceiverPort = *(acp_sint32_t *)aValue;

    return 0;
}

static acp_sint32_t oaConfigCallbackAlaReplicationName(acp_sint32_t aDepth,
                                                       acl_conf_key_t *aKey,
                                                       acp_sint32_t aLineNumber,
                                                       void *aValue,
                                                       void *aContext)
{
    oaConfigHandle *sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED(aDepth);
    ACP_UNUSED(aKey);
    ACP_UNUSED(aLineNumber);

    (void)acpStrCpy(&(sHandle->mAlaReplicationName), (acp_str_t *)aValue);

    return 0;
}

static acp_sint32_t oaConfigCallbackAlaSocketType(acp_sint32_t aDepth,
                                                  acl_conf_key_t *aKey,
                                                  acp_sint32_t aLineNumber,
                                                  void *aValue,
                                                  void *aContext)
{
    oaConfigHandle *sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED(aDepth);
    ACP_UNUSED(aKey);
    ACP_UNUSED(aLineNumber);

    (void)acpStrCpy(&(sHandle->mAlaSocketType), (acp_str_t *)aValue);

    return 0;
}

/* Altibase */
#ifdef ALTIADAPTER
static acp_sint32_t oaConfigCallbackOtherAltibaseHost( acp_sint32_t aDepth,
                                                  acl_conf_key_t *aKey,
                                                  acp_sint32_t aLineNumber,
                                                  void *aValue,
                                                  void *aContext )
{
    oaConfigHandle *sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED(aDepth);
    ACP_UNUSED(aKey);
    ACP_UNUSED(aLineNumber);

    (void)acpStrCpy(&(sHandle->mAltiHost), (acp_str_t *)aValue);

    return 0;
}

static acp_sint32_t oaConfigCallbackOtherAltibasePort( acp_sint32_t aDepth,
                                                  acl_conf_key_t *aKey,
                                                  acp_sint32_t aLineNumber,
                                                  void *aValue,
                                                  void *aContext )
{
    oaConfigHandle *sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED(aDepth);
    ACP_UNUSED(aKey);
    ACP_UNUSED(aLineNumber);

    sHandle->mAltiPort = *(acp_sint32_t *)aValue;

    return 0;
}

static acp_sint32_t oaConfigCallbackOtherAltibaseUser( acp_sint32_t aDepth,
                                                  acl_conf_key_t *aKey,
                                                  acp_sint32_t aLineNumber,
                                                  void *aValue,
                                                  void *aContext )
{
    oaConfigHandle *sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED(aDepth);
    ACP_UNUSED(aKey);
    ACP_UNUSED(aLineNumber);

    (void)acpStrCpy(&(sHandle->mAltiUser), (acp_str_t *)aValue);

    return 0;
}


static acp_sint32_t oaConfigCallbackOtherAltibasePassword( acp_sint32_t aDepth,
                                                      acl_conf_key_t *aKey,
                                                      acp_sint32_t aLineNumber,
                                                      void *aValue,
                                                      void *aContext)
{
    oaConfigHandle *sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED(aDepth);
    ACP_UNUSED(aKey);
    ACP_UNUSED(aLineNumber);

    (void)acpStrCpy(&(sHandle->mAltiPassword), (acp_str_t *)aValue);

    return 0;
}

static acp_sint32_t oaConfigCallbackOtherAltibaseAutocommitMode( acp_sint32_t aDepth,
                                                        acl_conf_key_t *aKey,
                                                        acp_sint32_t aLineNumber,
                                                        void *aValue,
                                                        void *aContext )
{
    oaConfigHandle *sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED(aDepth);
    ACP_UNUSED(aKey);
    ACP_UNUSED(aLineNumber);

    sHandle->mAltiAutocommitMode = *(acp_sint32_t *)aValue;

    return 0;
}

static acp_sint32_t oaConfigCallbackOtherAltibaseArrayDMLMaxSize( acp_sint32_t     aDepth,
                                                         acl_conf_key_t * aKey,
                                                         acp_sint32_t     aLineNumber,
                                                         void           * aValue,
                                                         void           * aContext )
{
    oaConfigHandle * sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED( aDepth );
    ACP_UNUSED( aKey );
    ACP_UNUSED( aLineNumber );

    sHandle->mAltiArrayDMLMaxSize = *(acp_uint32_t *)aValue;

    return 0;
}


static acp_sint32_t oaConfigCallbackOtherAltibaseSkipInsert( acp_sint32_t     aDepth,
                                                        acl_conf_key_t * aKey,
                                                        acp_sint32_t     aLineNumber,
                                                        void           * aValue,
                                                        void           * aContext )
{
    oaConfigHandle * sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED( aDepth );
    ACP_UNUSED( aKey );
    ACP_UNUSED( aLineNumber );

    sHandle->mAltiSkipInsert = *(acp_sint32_t *)aValue;

    return 0;
}

static acp_sint32_t oaConfigCallbackOtherAltibaseSkipUpdate( acp_sint32_t     aDepth,
                                                        acl_conf_key_t * aKey,
                                                        acp_sint32_t     aLineNumber,
                                                        void           * aValue,
                                                        void           * aContext )
{
    oaConfigHandle * sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED( aDepth );
    ACP_UNUSED( aKey );
    ACP_UNUSED( aLineNumber );

    sHandle->mAltiSkipUpdate = *(acp_sint32_t *)aValue;

    return 0;
}

static acp_sint32_t oaConfigCallbackOtherAltibaseSkipDelete( acp_sint32_t     aDepth,
                                                        acl_conf_key_t * aKey,
                                                        acp_sint32_t     aLineNumber,
                                                        void           * aValue,
                                                        void           * aContext )
{
    oaConfigHandle * sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED( aDepth );
    ACP_UNUSED( aKey );
    ACP_UNUSED( aLineNumber );

    sHandle->mAltiSkipDelete = *(acp_sint32_t *)aValue;

    return 0;
}

static acp_sint32_t oaConfigCallbackOtherAltibaseConflictLoggingLevel( acp_sint32_t     aDepth,
                                                                  acl_conf_key_t * aKey,
                                                                  acp_sint32_t     aLineNumber,
                                                                  void           * aValue,
                                                                  void           * aContext )
{
    oaConfigHandle * sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED( aDepth );
    ACP_UNUSED( aKey );
    ACP_UNUSED( aLineNumber );

    sHandle->mAltiConflictLoggingLevel = *(acp_uint32_t *)aValue;

    return 0;
}

static acp_sint32_t oaConfigCallbackOtherAltibaseGroupCommit( acp_sint32_t aDepth,
                                                              acl_conf_key_t *aKey,
                                                              acp_sint32_t aLineNumber,
                                                              void *aValue,
                                                              void *aContext )
{
    oaConfigHandle *sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED( aDepth );
    ACP_UNUSED( aKey );
    ACP_UNUSED( aLineNumber );

    sHandle->mAltiGroupCommit = *(acp_sint32_t *)aValue;

    return 0;
}

#elif JDBCADAPTER
static acp_sint32_t oaConfigCallbackOtherDatabaseUser( acp_sint32_t     aDepth,
                                                       acl_conf_key_t * aKey,
                                                       acp_sint32_t     aLineNumber,
                                                       void           * aValue,
                                                       void           * aContext )
{
    oaConfigHandle *sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED(aDepth);
    ACP_UNUSED(aKey);
    ACP_UNUSED(aLineNumber);

    (void)acpStrCpy(&(sHandle->mJDBCUser), (acp_str_t *)aValue);

    return 0;
}


static acp_sint32_t oaConfigCallbackOtherDatabasePassword( acp_sint32_t     aDepth,
                                                           acl_conf_key_t * aKey,
                                                           acp_sint32_t     aLineNumber,
                                                           void           * aValue,
                                                           void           * aContext)
{
    oaConfigHandle *sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED(aDepth);
    ACP_UNUSED(aKey);
    ACP_UNUSED(aLineNumber);

    (void)acpStrCpy(&(sHandle->mJDBCPassword), (acp_str_t *)aValue);

    return 0;
}

static acp_sint32_t oaConfigCallbackOtherDatabaseAutocommitMode( acp_sint32_t     aDepth,
                                                                 acl_conf_key_t * aKey,
                                                                 acp_sint32_t     aLineNumber,
                                                                 void           * aValue,
                                                                 void           * aContext )
{
    oaConfigHandle *sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED(aDepth);
    ACP_UNUSED(aKey);
    ACP_UNUSED(aLineNumber);

    sHandle->mJDBCAutocommitMode = *(acp_sint32_t *)aValue;

    return 0;
}

static acp_sint32_t oaConfigCallbackOtherDatabaseBatchDMLMaxSize( acp_sint32_t     aDepth,
                                                                  acl_conf_key_t * aKey,
                                                                  acp_sint32_t     aLineNumber,
                                                                  void           * aValue,
                                                                  void           * aContext )
{
    oaConfigHandle * sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED( aDepth );
    ACP_UNUSED( aKey );
    ACP_UNUSED( aLineNumber );

    sHandle->mJDBCBatchDMLMaxSize = *(acp_uint32_t *)aValue;

    return 0;
}


static acp_sint32_t oaConfigCallbackOtherDatabaseSkipInsert( acp_sint32_t     aDepth,
                                                             acl_conf_key_t * aKey,
                                                             acp_sint32_t     aLineNumber,
                                                             void           * aValue,
                                                             void           * aContext )
{
    oaConfigHandle * sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED( aDepth );
    ACP_UNUSED( aKey );
    ACP_UNUSED( aLineNumber );

    sHandle->mJDBCSkipInsert = *(acp_sint32_t *)aValue;

    return 0;
}

static acp_sint32_t oaConfigCallbackOtherDatabaseSkipUpdate( acp_sint32_t     aDepth,
                                                             acl_conf_key_t * aKey,
                                                             acp_sint32_t     aLineNumber,
                                                             void           * aValue,
                                                             void           * aContext )
{
    oaConfigHandle * sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED( aDepth );
    ACP_UNUSED( aKey );
    ACP_UNUSED( aLineNumber );

    sHandle->mJDBCSkipUpdate = *(acp_sint32_t *)aValue;

    return 0;
}

static acp_sint32_t oaConfigCallbackOtherDatabaseSkipDelete( acp_sint32_t     aDepth,
                                                             acl_conf_key_t * aKey,
                                                             acp_sint32_t     aLineNumber,
                                                             void           * aValue,
                                                             void           * aContext )
{
    oaConfigHandle * sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED( aDepth );
    ACP_UNUSED( aKey );
    ACP_UNUSED( aLineNumber );

    sHandle->mJDBCSkipDelete = *(acp_sint32_t *)aValue;

    return 0;
}

static acp_sint32_t oaConfigCallbackOtherDatabaseConflictLoggingLevel( acp_sint32_t     aDepth,
                                                                       acl_conf_key_t * aKey,
                                                                       acp_sint32_t     aLineNumber,
                                                                       void           * aValue,
                                                                       void           * aContext )
{
    oaConfigHandle * sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED( aDepth );
    ACP_UNUSED( aKey );
    ACP_UNUSED( aLineNumber );

    sHandle->mJDBCConflictLoggingLevel = *(acp_uint32_t *)aValue;

    return 0;
}

static acp_sint32_t oaConfigCallbackOtherDatabaseGroupCommit( acp_sint32_t aDepth,
                                                              acl_conf_key_t *aKey,
                                                              acp_sint32_t aLineNumber,
                                                              void *aValue,
                                                              void *aContext )
{
    oaConfigHandle *sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED( aDepth );
    ACP_UNUSED( aKey );
    ACP_UNUSED( aLineNumber );

    sHandle->mJDBCGroupCommit = *(acp_sint32_t *)aValue;

    return 0;
}

static acp_sint32_t oaConfigCallbackOtherDatabaseJDBCMaxHeapSize( acp_sint32_t     aDepth,
                                                                     acl_conf_key_t * aKey,
                                                                     acp_sint32_t     aLineNumber,
                                                                     void           * aValue,
                                                                     void           * aContext )
{
    oaConfigHandle * sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED( aDepth );
    ACP_UNUSED( aKey );
    ACP_UNUSED( aLineNumber );

    sHandle->mJDBCXmxOpt = *(acp_uint32_t *)aValue;

    return 0;
}

static acp_sint32_t oaConfigCallbackOtherDatabaseJDBCDriverPath( acp_sint32_t     aDepth,
                                                                 acl_conf_key_t * aKey,
                                                                 acp_sint32_t     aLineNumber,
                                                                 void           * aValue,
                                                                 void           * aContext )
{
    oaConfigHandle *sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED(aDepth);
    ACP_UNUSED(aKey);
    ACP_UNUSED(aLineNumber);


    (void)acpStrCpy(&( sHandle->mJDBCDriverPath), (acp_str_t *)aValue );

    return 0;
}

static acp_sint32_t oaConfigCallbackOtherDatabaseJDBCDriverClass( acp_sint32_t     aDepth,
                                                                  acl_conf_key_t * aKey,
                                                                  acp_sint32_t     aLineNumber,
                                                                  void           * aValue,
                                                                  void           * aContext )
{
    oaConfigHandle *sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED(aDepth);
    ACP_UNUSED(aKey);
    ACP_UNUSED(aLineNumber);


    (void)acpStrCpy( &(sHandle->mJDBCDriverClass), (acp_str_t *)aValue );

    return 0;
}

static acp_sint32_t oaConfigCallbackOtherDatabaseJDBCConnectionUrl( acp_sint32_t     aDepth,
                                                                    acl_conf_key_t * aKey,
                                                                    acp_sint32_t     aLineNumber,
                                                                    void           * aValue,
                                                                    void           * aContext )
{
    oaConfigHandle *sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED(aDepth);
    ACP_UNUSED(aKey);
    ACP_UNUSED(aLineNumber);


    (void)acpStrCpy( &(sHandle->mJDBCConnectionUrl), (acp_str_t *)aValue );

    return 0;
}

static acp_sint32_t oaConfigCallbackOtherDatabaseJDBCJVMOption( acp_sint32_t     aDepth,
                                                                acl_conf_key_t * aKey,
                                                                acp_sint32_t     aLineNumber,
                                                                void           * aValue,
                                                                void           * aContext )
{
    oaConfigHandle *sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED(aDepth);
    ACP_UNUSED(aKey);
    ACP_UNUSED(aLineNumber);


    (void)acpStrCpy( &(sHandle->mJDBCJVMOpt), (acp_str_t *)aValue );

    return 0;
}

#else


/* Oracle */
static acp_sint32_t oaConfigCallbackOracleServerAlias(acp_sint32_t aDepth,
                                                      acl_conf_key_t *aKey,
                                                      acp_sint32_t aLineNumber,
                                                      void *aValue,
                                                      void *aContext)
{
    oaConfigHandle *sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED(aDepth);
    ACP_UNUSED(aKey);
    ACP_UNUSED(aLineNumber);

    (void)acpStrCpy(&(sHandle->mOracleServerAlias), (acp_str_t *)aValue);

    return 0;
}

static acp_sint32_t oaConfigCallbackOracleUser(acp_sint32_t aDepth,
                                               acl_conf_key_t *aKey,
                                               acp_sint32_t aLineNumber,
                                               void *aValue,
                                               void *aContext)
{
    oaConfigHandle *sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED(aDepth);
    ACP_UNUSED(aKey);
    ACP_UNUSED(aLineNumber);

    (void)acpStrCpy(&(sHandle->mOracleUser), (acp_str_t *)aValue);

    return 0;
}


static acp_sint32_t oaConfigCallbackOraclePassword(acp_sint32_t aDepth,
                                                   acl_conf_key_t *aKey,
                                                   acp_sint32_t aLineNumber,
                                                   void *aValue,
                                                   void *aContext)
{
    oaConfigHandle *sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED(aDepth);
    ACP_UNUSED(aKey);
    ACP_UNUSED(aLineNumber);

    (void)acpStrCpy(&(sHandle->mOraclePassword), (acp_str_t *)aValue);

    return 0;
}

static acp_sint32_t oaConfigCallbackOracleAutocommitMode(acp_sint32_t aDepth,
                                                         acl_conf_key_t *aKey,
                                                         acp_sint32_t aLineNumber,
                                                         void *aValue,
                                                         void *aContext)
{
    oaConfigHandle *sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED(aDepth);
    ACP_UNUSED(aKey);
    ACP_UNUSED(aLineNumber);

    sHandle->mOracleAutocommitMode = *(acp_sint32_t *)aValue;

    return 0;
}

static acp_sint32_t oaConfigCallbackOracleAsynchronousCommit( acp_sint32_t     aDepth,
                                                             acl_conf_key_t * aKey,
                                                             acp_sint32_t     aLineNumber,
                                                             void           * aValue,
                                                             void           * aContext )
{
    oaConfigHandle * sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED( aDepth );
    ACP_UNUSED( aKey );
    ACP_UNUSED( aLineNumber );

    sHandle->mOracleAsynchronousCommit = *(acp_sint32_t *)aValue;

    return 0;
}

static acp_sint32_t oaConfigCallbackOracleGroupCommit( acp_sint32_t     aDepth,
                                                       acl_conf_key_t * aKey,
                                                       acp_sint32_t     aLineNumber,
                                                       void           * aValue,
                                                       void           * aContext )
{
    oaConfigHandle * sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED( aDepth );
    ACP_UNUSED( aKey );
    ACP_UNUSED( aLineNumber );

    sHandle->mOracleGroupCommit = *(acp_sint32_t *)aValue;

    return 0;
}

static acp_sint32_t oaConfigCallbackOracleSkipInsert( acp_sint32_t     aDepth,
                                                      acl_conf_key_t * aKey,
                                                      acp_sint32_t     aLineNumber,
                                                      void           * aValue,
                                                      void           * aContext )
{
    oaConfigHandle * sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED( aDepth );
    ACP_UNUSED( aKey );
    ACP_UNUSED( aLineNumber );

    sHandle->mOracleSkipInsert = *(acp_sint32_t *)aValue;

    return 0;
}

static acp_sint32_t oaConfigCallbackOracleSkipUpdate( acp_sint32_t     aDepth,
                                                      acl_conf_key_t * aKey,
                                                      acp_sint32_t     aLineNumber,
                                                      void           * aValue,
                                                      void           * aContext )
{
    oaConfigHandle * sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED( aDepth );
    ACP_UNUSED( aKey );
    ACP_UNUSED( aLineNumber );

    sHandle->mOracleSkipUpdate = *(acp_sint32_t *)aValue;

    return 0;
}

static acp_sint32_t oaConfigCallbackOracleSkipDelete( acp_sint32_t     aDepth,
                                                      acl_conf_key_t * aKey,
                                                      acp_sint32_t     aLineNumber,
                                                      void           * aValue,
                                                      void           * aContext )
{
    oaConfigHandle * sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED( aDepth );
    ACP_UNUSED( aKey );
    ACP_UNUSED( aLineNumber );

    sHandle->mOracleSkipDelete = *(acp_sint32_t *)aValue;

    return 0;
}

static acp_sint32_t oaConfigCallbackOracleDirectPathInsert( acp_sint32_t     aDepth,
                                                            acl_conf_key_t * aKey,
                                                            acp_sint32_t     aLineNumber,
                                                            void           * aValue,
                                                            void           * aContext )
{
    oaConfigHandle * sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED( aDepth );
    ACP_UNUSED( aKey );
    ACP_UNUSED( aLineNumber );

    sHandle->mOracleDirectPathInsert = *(acp_sint32_t *)aValue;

    return 0;
}

static acp_sint32_t oaConfigCallbackOracleArrayDMLMaxSize( acp_sint32_t     aDepth,
                                                           acl_conf_key_t * aKey,
                                                           acp_sint32_t     aLineNumber,
                                                           void           * aValue,
                                                           void           * aContext )
{
    oaConfigHandle * sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED( aDepth );
    ACP_UNUSED( aKey );
    ACP_UNUSED( aLineNumber );

    sHandle->mOracleArrayDMLMaxSize = *(acp_uint32_t *)aValue;

    return 0;
}

static acp_sint32_t oaConfigCallbackOracleUpdateStatementCacheSize( acp_sint32_t     aDepth,
                                                                    acl_conf_key_t * aKey,
                                                                    acp_sint32_t     aLineNumber,
                                                                    void           * aValue,
                                                                    void           * aContext )
{
    oaConfigHandle * sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED( aDepth );
    ACP_UNUSED( aKey );
    ACP_UNUSED( aLineNumber );

    sHandle->mOracleUpdateStatementCacheSize = *(acp_uint32_t *)aValue;

    return 0;
}

static acp_sint32_t oaConfigCallbackOracleConflictLoggingLevel( acp_sint32_t     aDepth,
                                                                acl_conf_key_t * aKey,
                                                                acp_sint32_t     aLineNumber,
                                                                void           * aValue,
                                                                void           * aContext )
{
    oaConfigHandle * sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED( aDepth );
    ACP_UNUSED( aKey );
    ACP_UNUSED( aLineNumber );

    sHandle->mOracleConflictLoggingLevel = *(acp_uint32_t *)aValue;

    return 0;
}

#endif

static acp_sint32_t oaConfigCallbackAlaXLogPoolSize(acp_sint32_t aDepth,
                                                    acl_conf_key_t *aKey,
                                                    acp_sint32_t aLineNumber,
                                                    void *aValue,
                                                    void *aContext)
{
    oaConfigHandle *sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED(aDepth);
    ACP_UNUSED(aKey);
    ACP_UNUSED(aLineNumber);

    sHandle->mAlaXLogPoolSize = *(acp_sint32_t *)aValue;

    return 0;
}

static acp_sint32_t oaConfigCallbackAlaAckPerXLogCount(acp_sint32_t aDepth,
                                                       acl_conf_key_t *aKey,
                                                       acp_sint32_t aLineNumber,
                                                       void *aValue,
                                                       void *aContext)
{
    oaConfigHandle *sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED(aDepth);
    ACP_UNUSED(aKey);
    ACP_UNUSED(aLineNumber);

    sHandle->mAlaAckPerXLogCount = *(acp_uint32_t *)aValue;

    return 0;
}

static acp_sint32_t oaConfigCallbackAlaLoggingActive(acp_sint32_t aDepth,
                                                     acl_conf_key_t *aKey,
                                                     acp_sint32_t aLineNumber,
                                                     void *aValue,
                                                     void *aContext)
{
    oaConfigHandle *sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED(aDepth);
    ACP_UNUSED(aKey);
    ACP_UNUSED(aLineNumber);

    sHandle->mAlaLoggingActive = *(acp_sint32_t *)aValue;

    return 0;
}

static acp_sint32_t oaConfigCallbackAlaLogDirectory(acp_sint32_t aDepth,
                                                    acl_conf_key_t *aKey,
                                                    acp_sint32_t aLineNumber,
                                                    void *aValue,
                                                    void *aContext)
{
    oaConfigHandle *sHandle = (oaConfigHandle *)aContext;

    acp_char_t *sHome = NULL;
    acp_rc_t sAcpRC = ACP_RC_SUCCESS;

    ACP_STR_DECLARE_DYNAMIC(sAlaLogDirPath);
    ACP_STR_INIT_DYNAMIC(sAlaLogDirPath, 128, 128);

    ACP_UNUSED(aDepth);
    ACP_UNUSED(aKey);
    ACP_UNUSED(aLineNumber);

    if (acpStrCmpCString((acp_str_t *)aValue,
                         "", ACP_STR_CASE_SENSITIVE) == 0)
    {
        sAcpRC = acpEnvGet(OA_ADAPTER_HOME_ENVIRONMENT_VARIABLE, &sHome);

        if (ACP_RC_NOT_ENOENT(sAcpRC) && (sAcpRC == ACP_RC_SUCCESS) )
        {
            (void)acpStrCpyFormat(&sAlaLogDirPath, "%s/trc/", sHome);
        }
        else
        {
            (void)acpStrCpyCString(&sAlaLogDirPath, "./trc/");
        }
    }
    else
    {
        (void)acpStrCpy(&sAlaLogDirPath, (acp_str_t *)aValue);
    }

    (void)acpStrCpy(&(sHandle->mAlaLogDirectory), &(sAlaLogDirPath));

    ACP_STR_FINAL(sAlaLogDirPath);

    return 0;
}

static acp_sint32_t oaConfigCallbackAlaLogFileName(acp_sint32_t aDepth,
                                                   acl_conf_key_t *aKey,
                                                   acp_sint32_t aLineNumber,
                                                   void *aValue,
                                                   void *aContext)
{
    oaConfigHandle *sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED(aDepth);
    ACP_UNUSED(aKey);
    ACP_UNUSED(aLineNumber);

    (void)acpStrCpy(&(sHandle->mAlaLogFileName), (acp_str_t *)aValue);

    return 0;
}

static acp_sint32_t oaConfigCallbackAlaLogFileSize(acp_sint32_t aDepth,
                                                   acl_conf_key_t *aKey,
                                                   acp_sint32_t aLineNumber,
                                                   void *aValue,
                                                   void *aContext)
{
    oaConfigHandle *sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED(aDepth);
    ACP_UNUSED(aKey);
    ACP_UNUSED(aLineNumber);

    sHandle->mAlaLogFileSize = *(acp_uint32_t *)aValue;

    return 0;
}

static acp_sint32_t oaConfigCallbackAlaMaxLogFileNumber(acp_sint32_t aDepth,
                                                        acl_conf_key_t *aKey,
                                                        acp_sint32_t aLineNumber,
                                                        void *aValue,
                                                        void *aContext)
{
    oaConfigHandle *sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED(aDepth);
    ACP_UNUSED(aKey);
    ACP_UNUSED(aLineNumber);

    sHandle->mAlaMaxLogFileNumber = *(acp_uint32_t *)aValue;

    return 0;
}

static acp_sint32_t oaConfigCallbackAlaReceiveXLogTimeout(acp_sint32_t aDepth,
                                                          acl_conf_key_t *aKey,
                                                          acp_sint32_t aLineNumber,
                                                          void *aValue,
                                                          void *aContext)
{
    oaConfigHandle *sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED(aDepth);
    ACP_UNUSED(aKey);
    ACP_UNUSED(aLineNumber);

    sHandle->mAlaReceiveXlogTimeout = *(acp_uint32_t *)aValue;

    return 0;
}

static acp_sint32_t oaConfigCallbackErrorRetryCount( acp_sint32_t aDepth,
                                                     acl_conf_key_t *aKey,
                                                     acp_sint32_t aLineNumber,
                                                     void *aValue,
                                                     void *aContext )
{
    oaConfigHandle *sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED( aDepth );
    ACP_UNUSED( aKey );
    ACP_UNUSED( aLineNumber );

    sHandle->mErrorRetryCount = *(acp_uint32_t *)aValue;

    return 0;
}

static acp_sint32_t oaConfigCallbackErrorRetryInterval( acp_sint32_t aDepth,
                                                        acl_conf_key_t *aKey,
                                                        acp_sint32_t aLineNumber,
                                                        void *aValue,
                                                        void *aContext )
{
    oaConfigHandle *sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED( aDepth );
    ACP_UNUSED( aKey );
    ACP_UNUSED( aLineNumber );

    sHandle->mErrorRetryInterval = *(acp_uint32_t *)aValue;

    return 0;

}

static acp_sint32_t oaConfigCallbackSkipError( acp_sint32_t aDepth,
                                               acl_conf_key_t *aKey,
                                               acp_sint32_t aLineNumber,
                                               void *aValue,
                                               void *aContext )
{
    oaConfigHandle *sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED( aDepth);
    ACP_UNUSED( aKey );
    ACP_UNUSED( aLineNumber );

    sHandle->mSkipError = *(acp_uint32_t *)aValue;

    return 0;

}

static acp_sint32_t oaConfigCallbackConstraintAltibaseUser( acp_sint32_t     aDepth,
                                                            acl_conf_key_t * aKey,
                                                            acp_sint32_t     aLineNumber,
                                                            void           * aValue,
                                                            void           * aContext )
{
    oaConfigHandle *sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED( aDepth );
    ACP_UNUSED( aKey );
    ACP_UNUSED( aLineNumber );

    /* TODO: Check value is a User ID */

    (void)acpStrCpy( &(sHandle->mConstraintAltibaseUser), (acp_str_t *)aValue );

    return 0;
}

static acp_sint32_t oaConfigCallbackConstraintAltibasePassword( acp_sint32_t     aDepth,
                                                                acl_conf_key_t * aKey,
                                                                acp_sint32_t     aLineNumber,
                                                                void           * aValue,
                                                                void           * aContext )
{
    oaConfigHandle *sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED( aDepth );
    ACP_UNUSED( aKey );
    ACP_UNUSED( aLineNumber );

    /* TODO: Check value is a Password */

    (void)acpStrCpy( &(sHandle->mConstraintAltibasePassword), (acp_str_t *)aValue );

    return 0;
}

static acp_sint32_t oaConfigCallbackConstraintAltibaseIP( acp_sint32_t     aDepth,
                                                          acl_conf_key_t * aKey,
                                                          acp_sint32_t     aLineNumber,
                                                          void           * aValue,
                                                          void           * aContext )
{
    oaConfigHandle *sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED( aDepth );
    ACP_UNUSED( aKey );
    ACP_UNUSED( aLineNumber );

    /* TODO: Check value is a IP address */

    (void)acpStrCpy( &(sHandle->mConstraintAltibaseIP), (acp_str_t *)aValue );

    return 0;
}

static acp_sint32_t oaConfigCallbackConstraintAltibasePort( acp_sint32_t     aDepth,
                                                            acl_conf_key_t * aKey,
                                                            acp_sint32_t     aLineNumber,
                                                            void           * aValue,
                                                            void           * aContext )
{
    oaConfigHandle *sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED( aDepth );
    ACP_UNUSED( aKey );
    ACP_UNUSED( aLineNumber );

    sHandle->mConstraintAltibasePort = *(acp_sint32_t *)aValue;

    return 0;
}

static acp_sint32_t oaConfigCallbackAdapterErrorRestartCount( acp_sint32_t     aDepth,
                                                              acl_conf_key_t * aKey,
                                                              acp_sint32_t     aLineNumber,
                                                              void           * aValue,
                                                              void           * aContext )
{
    oaConfigHandle *sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED( aDepth );
    ACP_UNUSED( aKey );
    ACP_UNUSED( aLineNumber );

    sHandle->mAdapterErrorRestartCount = *(acp_sint32_t *)aValue;

    return 0;
}

static acp_sint32_t oaConfigCallbackAdapterErrorRestartInterval( acp_sint32_t     aDepth,
                                                                 acl_conf_key_t * aKey,
                                                                 acp_sint32_t     aLineNumber,
                                                                 void           * aValue,
                                                                 void           * aContext )
{
    oaConfigHandle *sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED( aDepth );
    ACP_UNUSED( aKey );
    ACP_UNUSED( aLineNumber );

    sHandle->mAdapterErrorRestartInterval = *(acp_sint32_t *)aValue;

    return 0;
}

static acp_sint32_t oaConfigCallbackSetUserToTable( acp_sint32_t     aDepth,
                                                    acl_conf_key_t * aKey,
                                                    acp_sint32_t     aLineNumber,
                                                    void           * aValue,
                                                    void           * aContext )
{
    oaConfigHandle *sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED( aDepth );
    ACP_UNUSED( aKey );
    ACP_UNUSED( aLineNumber );

    sHandle->mSetUserToTable = *(acp_uint32_t *)aValue;

    return 0;
}

static acp_sint32_t oaConfigCallbackSetColumnToInsert( acp_sint32_t     aDepth,
                                                       acl_conf_key_t * aKey,
                                                       acp_sint32_t     aLineNumber,
                                                       void           * aValue,
                                                       void           * aContext )
{
    oaConfigHandle *sHandle = (oaConfigHandle *)aContext;

    ACP_UNUSED( aDepth );
    ACP_UNUSED( aKey );
    ACP_UNUSED( aLineNumber );

    sHandle->mSetColumnToInsert = *(acp_uint32_t *)aValue;

    return 0;
}

static acp_sint32_t oaConfigCallbackDefault(acp_sint32_t aDepth,
                                            acl_conf_key_t *aKey,
                                            acp_sint32_t aLineNumber,
                                            void *aValue,
                                            void *aContext)
{
    ACP_UNUSED(aDepth);
    ACP_UNUSED(aKey);
    ACP_UNUSED(aLineNumber);
    ACP_UNUSED(aValue);
    ACP_UNUSED(aContext);
    /* nothing to do */

    return 0;
}


static acl_conf_def_t gConfigDefinition[] =
{ 
    ACL_CONF_DEF_STRING("LOG_FILE_NAME",
                        OA_CONFIG_LOG_FILE_NAME,
                        OA_CONFIG_LOG_FILE_NAME_DEFAULT,
                        1, oaConfigCallbackLogFileName),

    ACL_CONF_DEF_UINT32( "LOG_BACKUP_LIMIT",
                         OA_CONFIG_LOG_BACKUP_LIMIT,
                         OA_CONFIG_LOG_BACKUP_LIMIT_DEFAULT,
                         OA_CONFIG_LOG_BACKUP_LIMIT_MIN,
                         OA_CONFIG_LOG_BACKUP_LIMIT_MAX,
                         1, oaConfigCallbackLogBackupLimit ),

    ACL_CONF_DEF_UINT32( "LOG_BACKUP_SIZE",
                         OA_CONFIG_LOG_BACKUP_SIZE,
                         OA_CONFIG_LOG_BACKUP_SIZE_DEFAULT,
                         OA_CONFIG_LOG_BACKUP_SIZE_MIN,
                         OA_CONFIG_LOG_BACKUP_SIZE_MAX,
                         1, oaConfigCallbackLogBackupSize ),

    ACL_CONF_DEF_STRING("MESSAGE_PATH",
                        OA_CONFIG_MESSAGE_PATH,
                        OA_CONFIG_MESSAGE_PATH_DEFAULT,
                        1, oaConfigCallbackMessagePath),

    ACL_CONF_DEF_SINT32( "PERFORMANCE_TICK_CHECK",
                         OA_CONFIG_PERFORMANCE_TICK_CHECK,
                         OA_CONFIG_PERFORMANCE_TICK_CHECK_DEFAULT,
                         OA_CONFIG_PERFORMANCE_TICK_CHECK_MIN,
                         OA_CONFIG_PERFORMANCE_TICK_CHECK_MAX,
                         1, oaConfigCallbackPerformanceTickCheck ),

    ACL_CONF_DEF_STRING("ALA_SENDER_IP",
                        OA_CONFIG_ALA_SENDER_IP,
                        OA_CONFIG_ALA_SENDER_IP_DEFAULT,
                        1, oaConfigCallbackAlaSenderIP),

    ACL_CONF_DEF_SINT32( "ALA_SENDER_REPLICATION_PORT",
                         OA_CONFIG_ALA_SENDER_REPLICATION_PORT,
                         OA_CONFIG_ALA_SENDER_REPLICATION_PORT_DEFAULT,
                         OA_CONFIG_ALA_SENDER_REPLICATION_PORT_MIN,
                         OA_CONFIG_ALA_SENDER_REPLICATION_PORT_MAX,
                         1, oaConfigCallbackAlaSenderReplicationPort ),


    ACL_CONF_DEF_SINT32("ALA_RECEIVER_PORT",
                        OA_CONFIG_ALA_RECEIVER_PORT,
                        OA_CONFIG_ALA_RECEIVER_PORT_DEFAULT,
                        OA_CONFIG_ALA_RECEIVER_PORT_MIN,
                        OA_CONFIG_ALA_RECEIVER_PORT_MAX,
                        1, oaConfigCallbackAlaReceiverPort),

    ACL_CONF_DEF_STRING("ALA_REPLICATION_NAME",
                        OA_CONFIG_ALA_REPLICATION_NAME,
                        OA_CONFIG_ALA_REPLICATION_NAME_DEFAULT,
                        1, oaConfigCallbackAlaReplicationName),

    ACL_CONF_DEF_STRING("ALA_SOCKET_TYPE",
                        OA_CONFIG_ALA_SOCKET_TYPE,
                        OA_CONFIG_ALA_SOCKET_TYPE_DEFAULT,
                        1, oaConfigCallbackAlaSocketType),

    /* Altibase */
#ifdef ALTIADAPTER
    ACL_CONF_DEF_STRING("OTHER_ALTIBASE_HOST",
                        OA_CONFIG_OTHER_ALTIBASE_HOST,
                        OA_CONFIG_OTHER_ALTIBASE_HOST_DEFAULT,
                        1, oaConfigCallbackOtherAltibaseHost),

    ACL_CONF_DEF_SINT32("OTHER_ALTIBASE_PORT",
                        OA_CONFIG_OTHER_ALTIBASE_PORT,
                        OA_CONFIG_OTHER_ALTIBASE_PORT_DEFAULT,
                        OA_CONFIG_OTHER_ALTIBASE_PORT_MIN,
                        OA_CONFIG_OTHER_ALTIBASE_PORT_MAX,
                        1, oaConfigCallbackOtherAltibasePort),

    ACL_CONF_DEF_STRING("OTHER_ALTIBASE_USER",
                        OA_CONFIG_OTHER_ALTIBASE_USER,
                        OA_CONFIG_OTHER_ALTIBASE_USER_DEFAULT,
                        1, oaConfigCallbackOtherAltibaseUser),

    ACL_CONF_DEF_STRING("OTHER_ALTIBASE_PASSWORD",
                        OA_CONFIG_OTHER_ALTIBASE_PASSWORD,
                        OA_CONFIG_OTHER_ALTIBASE_PASSWORD_DEFAULT,
                        1, oaConfigCallbackOtherAltibasePassword),

    ACL_CONF_DEF_SINT32("OTHER_ALTIBASE_AUTOCOMMIT_MODE",
                        OA_CONFIG_OTHER_ALTIBASE_AUTOCOMMIT_MODE,
                        OA_CONFIG_OTHER_ALTIBASE_AUTOCOMMIT_MODE_DEFAULT,
                        OA_CONFIG_OTHER_ALTIBASE_AUTOCOMMIT_MODE_MIN,
                        OA_CONFIG_OTHER_ALTIBASE_AUTOCOMMIT_MODE_MAX,
                        1, oaConfigCallbackOtherAltibaseAutocommitMode),

    ACL_CONF_DEF_UINT32( "OTHER_ALTIBASE_ARRAY_DML_MAX_SIZE",
                         OA_CONFIG_OTHER_ALTIBASE_ARRAY_DML_MAX_SIZE,
                         OA_CONFIG_OTHER_ALTIBASE_ARRAY_DML_MAX_SIZE_DEFAULT,
                         OA_CONFIG_OTHER_ALTIBASE_ARRAY_DML_MAX_SIZE_MIN,
                         OA_CONFIG_OTHER_ALTIBASE_ARRAY_DML_MAX_SIZE_MAX,
                         1, oaConfigCallbackOtherAltibaseArrayDMLMaxSize ),

    ACL_CONF_DEF_SINT32( "OTHER_ALTIBASE_SKIP_INSERT",
                         OA_CONFIG_OTHER_ALTIBASE_SKIP_INSERT,
                         OA_CONFIG_OTHER_ALTIBASE_SKIP_INSERT_DEFAULT,
                         OA_CONFIG_OTHER_ALTIBASE_SKIP_INSERT_MIN,
                         OA_CONFIG_OTHER_ALTIBASE_SKIP_INSERT_MAX,
                         1, oaConfigCallbackOtherAltibaseSkipInsert ),

    ACL_CONF_DEF_SINT32( "OTHER_ALTIBASE_SKIP_UPDATE",
                         OA_CONFIG_OTHER_ALTIBASE_SKIP_UPDATE,
                         OA_CONFIG_OTHER_ALTIBASE_SKIP_UPDATE_DEFAULT,
                         OA_CONFIG_OTHER_ALTIBASE_SKIP_UPDATE_MIN,
                         OA_CONFIG_OTHER_ALTIBASE_SKIP_UPDATE_MAX,
                         1, oaConfigCallbackOtherAltibaseSkipUpdate ),

    ACL_CONF_DEF_SINT32( "OTHER_ALTIBASE_SKIP_DELETE",
                         OA_CONFIG_OTHER_ALTIBASE_SKIP_DELETE,
                         OA_CONFIG_OTHER_ALTIBASE_SKIP_DELETE_DEFAULT,
                         OA_CONFIG_OTHER_ALTIBASE_SKIP_DELETE_MIN,
                         OA_CONFIG_OTHER_ALTIBASE_SKIP_DELETE_MAX,
                         1, oaConfigCallbackOtherAltibaseSkipDelete ),

    ACL_CONF_DEF_UINT32( "OTHER_ALTIBASE_CONFLICT_LOGGING_LEVEL",
                         OA_CONFIG_OTHER_ALTIBASE_CONFLICT_LOGGING_LEVEL,
                         OA_CONFIG_OTHER_ALTIBASE_CONFLICT_LOGGING_LEVEL_DEFAULT,
                         OA_CONFIG_OTHER_ALTIBASE_CONFLICT_LOGGING_LEVEL_MIN,
                         OA_CONFIG_OTHER_ALTIBASE_CONFLICT_LOGGING_LEVEL_MAX,
                         1, oaConfigCallbackOtherAltibaseConflictLoggingLevel ),

     ACL_CONF_DEF_SINT32( "OTHER_ALTIBASE_GROUP_COMMIT",
                          OA_CONFIG_OTHER_ALTIBASE_GROUP_COMMIT,
                          OA_CONFIG_OTHER_ALTIBASE_GROUP_COMMIT_DEFAULT,
                          OA_CONFIG_OTHER_ALTIBASE_GROUP_COMMIT_MIN,
                          OA_CONFIG_OTHER_ALTIBASE_GROUP_COMMIT_MAX,
                          1, oaConfigCallbackOtherAltibaseGroupCommit ),

    ACL_CONF_DEF_UINT32( "OTHER_ALTIBASE_ERROR_RETRY_COUNT",
                         OA_CONFIG_ERROR_RETRY_COUNT,
                         OA_CONFIG_ERROR_RETRY_COUNT_DEFAULT,
                         OA_CONFIG_ERROR_RETRY_COUNT_MIN,
                         OA_CONFIG_ERROR_RETRY_COUNT_MAX,
                         1, oaConfigCallbackErrorRetryCount ),

    ACL_CONF_DEF_UINT32( "OTHER_ALTIBASE_ERROR_RETRY_INTERVAL",
                         OA_CONFIG_ERROR_RETRY_INTERVAL,
                         OA_CONFIG_ERROR_RETRY_INTERVAL_DEFAULT,
                         OA_CONFIG_ERROR_RETRY_INTERVAL_MIN,
                         OA_CONFIG_ERROR_RETRY_INTERVAL_MAX,
                         1, oaConfigCallbackErrorRetryInterval ),

    ACL_CONF_DEF_UINT32( "OTHER_ALTIBASE_SKIP_ERROR",
                         OA_CONFIG_SKIP_ERROR,
                         OA_CONFIG_SKIP_ERROR_DEFAULT,
                         OA_CONFIG_SKIP_ERROR_MIN,
                         OA_CONFIG_SKIP_ERROR_MAX,
                         1, oaConfigCallbackSkipError ),

    ACL_CONF_DEF_SINT32( "OTHER_ALTIBASE_SET_USER_TO_TABLE",
                         OA_CONFIG_OTHER_ALTIBASE_SET_USER_TO_TABLE,
                         OA_CONFIG_OTHER_ALTIBASE_SET_USER_TO_TABLE_DEFAULT,
                         OA_CONFIG_OTHER_ALTIBASE_SET_USER_TO_TABLE_MIN,
                         OA_CONFIG_OTHER_ALTIBASE_SET_USER_TO_TABLE_MAX,
                         1, oaConfigCallbackSetUserToTable ), 
/* JDBC */
#elif JDBCADAPTER
    ACL_CONF_DEF_STRING( "OTHER_DATABASE_USER",
                         OA_CONFIG_OTHER_DATABASE_USER,
                         OA_CONFIG_OTHER_DATABASE_USER_DEFAULT,
                         1, oaConfigCallbackOtherDatabaseUser ),
                          
    ACL_CONF_DEF_STRING( "OTHER_DATABASE_PASSWORD",
                         OA_CONFIG_OTHER_DATABASE_PASSWORD,
                         OA_CONFIG_OTHER_DATABASE_PASSWORD_DEFAULT,
                         1, oaConfigCallbackOtherDatabasePassword ),
    
    ACL_CONF_DEF_SINT32( "OTHER_DATABASE_AUTOCOMMIT_MODE",
                         OA_CONFIG_OTHER_DATABASE_AUTOCOMMIT_MODE,
                         OA_CONFIG_OTHER_DATABASE_AUTOCOMMIT_MODE_DEFAULT,
                         OA_CONFIG_OTHER_DATABASE_AUTOCOMMIT_MODE_MIN,
                         OA_CONFIG_OTHER_DATABASE_AUTOCOMMIT_MODE_MAX,
                         1, oaConfigCallbackOtherDatabaseAutocommitMode ),
    
    ACL_CONF_DEF_UINT32( "OTHER_DATABASE_BATCH_DML_MAX_SIZE",
                         OA_CONFIG_OTHER_DATABASE_BATCH_DML_MAX_SIZE,
                         OA_CONFIG_OTHER_DATABASE_BATCH_DML_MAX_SIZE_DEFAULT,
                         OA_CONFIG_OTHER_DATABASE_BATCH_DML_MAX_SIZE_MIN,
                         OA_CONFIG_OTHER_DATABASE_BATCH_DML_MAX_SIZE_MAX,
                         1, oaConfigCallbackOtherDatabaseBatchDMLMaxSize ),
    
    ACL_CONF_DEF_SINT32( "OTHER_DATABASE_SKIP_INSERT",
                         OA_CONFIG_OTHER_DATABASE_SKIP_INSERT,
                         OA_CONFIG_OTHER_DATABASE_SKIP_INSERT_DEFAULT,
                         OA_CONFIG_OTHER_DATABASE_SKIP_INSERT_MIN,
                         OA_CONFIG_OTHER_DATABASE_SKIP_INSERT_MAX,
                         1, oaConfigCallbackOtherDatabaseSkipInsert ),
    
    ACL_CONF_DEF_SINT32( "OTHER_DATABASE_SKIP_UPDATE",
                         OA_CONFIG_OTHER_DATABASE_SKIP_UPDATE,
                         OA_CONFIG_OTHER_DATABASE_SKIP_UPDATE_DEFAULT,
                         OA_CONFIG_OTHER_DATABASE_SKIP_UPDATE_MIN,
                         OA_CONFIG_OTHER_DATABASE_SKIP_UPDATE_MAX,
                         1, oaConfigCallbackOtherDatabaseSkipUpdate ),
    
    ACL_CONF_DEF_SINT32( "OTHER_DATABASE_SKIP_DELETE",
                         OA_CONFIG_OTHER_DATABASE_SKIP_DELETE,
                         OA_CONFIG_OTHER_DATABASE_SKIP_DELETE_DEFAULT,
                         OA_CONFIG_OTHER_DATABASE_SKIP_DELETE_MIN,
                         OA_CONFIG_OTHER_DATABASE_SKIP_DELETE_MAX,
                         1, oaConfigCallbackOtherDatabaseSkipDelete ),
    
    ACL_CONF_DEF_UINT32( "OTHER_DATABASE_CONFLICT_LOGGING_LEVEL",
                         OA_CONFIG_OTHER_DATABASE_CONFLICT_LOGGING_LEVEL,
                         OA_CONFIG_OTHER_DATABASE_CONFLICT_LOGGING_LEVEL_DEFAULT,
                         OA_CONFIG_OTHER_DATABASE_CONFLICT_LOGGING_LEVEL_MIN,
                         OA_CONFIG_OTHER_DATABASE_CONFLICT_LOGGING_LEVEL_MAX,
                         1, oaConfigCallbackOtherDatabaseConflictLoggingLevel ),

    ACL_CONF_DEF_SINT32( "OTHER_DATABASE_GROUP_COMMIT",
                         OA_CONFIG_OTHER_DATABASE_GROUP_COMMIT,
                         OA_CONFIG_OTHER_DATABASE_GROUP_COMMIT_DEFAULT,
                         OA_CONFIG_OTHER_DATABASE_GROUP_COMMIT_MIN,
                         OA_CONFIG_OTHER_DATABASE_GROUP_COMMIT_MAX,
                         1, oaConfigCallbackOtherDatabaseGroupCommit ),
    
    ACL_CONF_DEF_SINT32( "OTHER_DATABASE_JDBC_MAX_HEAP_SIZE",
                         OA_CONFIG_OTHER_DATABASE_JDBC_MAX_HEAP_SIZE,
                         OA_CONFIG_OTHER_DATABASE_JDBC_MAX_HEAP_SIZE_DEFAULT,
                         OA_CONFIG_OTHER_DATABASE_JDBC_MAX_HEAP_SIZE_MIN,
                         OA_CONFIG_OTHER_DATABASE_JDBC_MAX_HEAP_SIZE_MAX,
                         1, oaConfigCallbackOtherDatabaseJDBCMaxHeapSize ),
                         
    ACL_CONF_DEF_STRING( "OTHER_DATABASE_JDBC_DRIVER_PATH",
                         OA_CONFIG_OTHER_DATABASE_JDBC_DRIVER_PATH,
                         OA_CONFIG_OTHER_DATABASE_JDBC_DRIVER_PATH_DEFAULT,
                         1, oaConfigCallbackOtherDatabaseJDBCDriverPath ),
                         
    ACL_CONF_DEF_STRING( "OTHER_DATABASE_JDBC_DRIVER_CLASS",
                         OA_CONFIG_OTHER_DATABASE_JDBC_DRIVER_CLASS,
                         OA_CONFIG_OTHER_DATABASE_JDBC_DRIVER_CLASS_DEFAULT,
                         1, oaConfigCallbackOtherDatabaseJDBCDriverClass ),         
                         
    ACL_CONF_DEF_STRING( "OTHER_DATABASE_JDBC_CONNECTION_URL",
                         OA_CONFIG_OTHER_DATABASE_JDBC_CONNECTION_URL,
                         OA_CONFIG_OTHER_DATABASE_JDBC_CONNECTION_URL_DEFAULT,
                         1, oaConfigCallbackOtherDatabaseJDBCConnectionUrl ),

    ACL_CONF_DEF_UINT32( "OTHER_DATABASE_ERROR_RETRY_COUNT",
                         OA_CONFIG_ERROR_RETRY_COUNT,
                         OA_CONFIG_ERROR_RETRY_COUNT_DEFAULT,
                         OA_CONFIG_ERROR_RETRY_COUNT_MIN,
                         OA_CONFIG_ERROR_RETRY_COUNT_MAX,
                         1, oaConfigCallbackErrorRetryCount ),
    
    ACL_CONF_DEF_UINT32( "OTHER_DATABASE_ERROR_RETRY_INTERVAL",
                         OA_CONFIG_ERROR_RETRY_INTERVAL,
                         OA_CONFIG_ERROR_RETRY_INTERVAL_DEFAULT,
                         OA_CONFIG_ERROR_RETRY_INTERVAL_MIN,
                         OA_CONFIG_ERROR_RETRY_INTERVAL_MAX,
                         1, oaConfigCallbackErrorRetryInterval ),
    
    ACL_CONF_DEF_UINT32( "OTHER_DATABASE_SKIP_ERROR",
                         OA_CONFIG_SKIP_ERROR,
                         OA_CONFIG_SKIP_ERROR_DEFAULT,
                         OA_CONFIG_SKIP_ERROR_MIN,
                         OA_CONFIG_SKIP_ERROR_MAX,
                         1, oaConfigCallbackSkipError ),

    ACL_CONF_DEF_SINT32( "OTHER_DATABASE_SET_USER_TO_TABLE",
                         OA_CONFIG_OTHER_DATABASE_SET_USER_TO_TABLE,
                         OA_CONFIG_OTHER_DATABASE_SET_USER_TO_TABLE_DEFAULT,
                         OA_CONFIG_OTHER_DATABASE_SET_USER_TO_TABLE_MIN,
                         OA_CONFIG_OTHER_DATABASE_SET_USER_TO_TABLE_MAX,
                         1, oaConfigCallbackSetUserToTable ), 

    ACL_CONF_DEF_SINT32( "OTHER_DATABASE_SET_COLUMN_TO_INSERT",
                         OA_CONFIG_OTHER_DATABASE_SET_COLUMN_TO_INSERT,
                         OA_CONFIG_OTHER_DATABASE_SET_COLUMN_TO_INSERT_DEFAULT,
                         OA_CONFIG_OTHER_DATABASE_SET_COLUMN_TO_INSERT_MIN,
                         OA_CONFIG_OTHER_DATABASE_SET_COLUMN_TO_INSERT_MAX,
                         1, oaConfigCallbackSetColumnToInsert ), 
                      
    ACL_CONF_DEF_STRING( "OTHER_DATABASE_JDBC_JVM_OPTION",
                          OA_CONFIG_OTHER_DATABASE_JDBC_JVM_OPTION,
                          OA_CONFIG_OTHER_DATABASE_JDBC_JVM_OPTION_DEFAULT,
                          1, oaConfigCallbackOtherDatabaseJDBCJVMOption ),     
#else
    /* Oracle */
    ACL_CONF_DEF_STRING("ORACLE_SERVER_ALIAS",
                        OA_CONFIG_ORACLE_SERVER_ALIAS,
                        OA_CONFIG_ORACLE_SERVER_ALIAS_DEFAULT,
                        1, oaConfigCallbackOracleServerAlias),

    ACL_CONF_DEF_STRING("ORACLE_USER",
                        OA_CONFIG_ORACLE_USER,
                        OA_CONFIG_ORACLE_USER_DEFAULT,
                        1, oaConfigCallbackOracleUser),

    ACL_CONF_DEF_STRING("ORACLE_PASSWORD",
                        OA_CONFIG_ORACLE_PASSWORD,
                        OA_CONFIG_ORACLE_PASSWORD_DEFAULT,
                        1, oaConfigCallbackOraclePassword),

    ACL_CONF_DEF_SINT32("ORACLE_AUTOCOMMIT_MODE",
                        OA_CONFIG_ORACLE_AUTOCOMMIT_MODE,
                        OA_CONFIG_ORACLE_AUTOCOMMIT_MODE_DEFAULT,
                        OA_CONFIG_ORACLE_AUTOCOMMIT_MODE_MIN,
                        OA_CONFIG_ORACLE_AUTOCOMMIT_MODE_MAX,
                        1, oaConfigCallbackOracleAutocommitMode),

    ACL_CONF_DEF_SINT32( "ORACLE_ASYNCHRONOUS_COMMIT",  
                         OA_CONFIG_ORACLE_ASYNCHRONOUS_COMMIT,
                         OA_CONFIG_ORACLE_ASYNCHRONOUS_COMMIT_DEFAULT,
                         OA_CONFIG_ORACLE_ASYNCHRONOUS_COMMIT_MIN,
                         OA_CONFIG_ORACLE_ASYNCHRONOUS_COMMIT_MAX,
                         1, oaConfigCallbackOracleAsynchronousCommit ),

    ACL_CONF_DEF_SINT32( "ORACLE_GROUP_COMMIT",
                         OA_CONFIG_ORACLE_GROUP_COMMIT,
                         OA_CONFIG_ORACLE_GROUP_COMMIT_DEFAULT,
                         OA_CONFIG_ORACLE_GROUP_COMMIT_MIN,
                         OA_CONFIG_ORACLE_GROUP_COMMIT_MAX,
                         1, oaConfigCallbackOracleGroupCommit ),

    ACL_CONF_DEF_SINT32( "ORACLE_SKIP_INSERT",
                         OA_CONFIG_ORACLE_SKIP_INSERT,
                         OA_CONFIG_ORACLE_SKIP_INSERT_DEFAULT,
                         OA_CONFIG_ORACLE_SKIP_INSERT_MIN,
                         OA_CONFIG_ORACLE_SKIP_INSERT_MAX,
                         1, oaConfigCallbackOracleSkipInsert ),

    ACL_CONF_DEF_SINT32( "ORACLE_SKIP_UPDATE",
                         OA_CONFIG_ORACLE_SKIP_UPDATE,
                         OA_CONFIG_ORACLE_SKIP_UPDATE_DEFAULT,
                         OA_CONFIG_ORACLE_SKIP_UPDATE_MIN,
                         OA_CONFIG_ORACLE_SKIP_UPDATE_MAX,
                         1, oaConfigCallbackOracleSkipUpdate ),

    ACL_CONF_DEF_SINT32( "ORACLE_SKIP_DELETE",
                         OA_CONFIG_ORACLE_SKIP_DELETE,
                         OA_CONFIG_ORACLE_SKIP_DELETE_DEFAULT,
                         OA_CONFIG_ORACLE_SKIP_DELETE_MIN,
                         OA_CONFIG_ORACLE_SKIP_DELETE_MAX,
                         1, oaConfigCallbackOracleSkipDelete ),

    ACL_CONF_DEF_SINT32( "ORACLE_DIRECT_PATH_INSERT",
                         OA_CONFIG_ORACLE_DIRECT_PATH_INSERT,
                         OA_CONFIG_ORACLE_DIRECT_PATH_INSERT_DEFAULT,
                         OA_CONFIG_ORACLE_DIRECT_PATH_INSERT_MIN,
                         OA_CONFIG_ORACLE_DIRECT_PATH_INSERT_MAX,
                         1, oaConfigCallbackOracleDirectPathInsert ),

    ACL_CONF_DEF_UINT32( "ORACLE_ARRAY_DML_MAX_SIZE",
                         OA_CONFIG_ORACLE_ARRAY_DML_MAX_SIZE,
                         OA_CONFIG_ORACLE_ARRAY_DML_MAX_SIZE_DEFAULT,
                         OA_CONFIG_ORACLE_ARRAY_DML_MAX_SIZE_MIN,
                         OA_CONFIG_ORACLE_ARRAY_DML_MAX_SIZE_MAX,
                         1, oaConfigCallbackOracleArrayDMLMaxSize ),

    ACL_CONF_DEF_UINT32( "ORACLE_UPDATE_STATEMENT_CACHE_SIZE",
                         OA_CONFIG_ORACLE_UPDATE_STATEMENT_CACHE_SIZE,
                         OA_CONFIG_ORACLE_UPDATE_STATEMENT_CACHE_SIZE_DEFAULT,
                         OA_CONFIG_ORACLE_UPDATE_STATEMENT_CACHE_SIZE_MIN,
                         OA_CONFIG_ORACLE_UPDATE_STATEMENT_CACHE_SIZE_MAX,
                         1, oaConfigCallbackOracleUpdateStatementCacheSize ),

    ACL_CONF_DEF_UINT32( "ORACLE_CONFLICT_LOGGING_LEVEL",
                         OA_CONFIG_ORACLE_CONFLICT_LOGGING_LEVEL,
                         OA_CONFIG_ORACLE_CONFLICT_LOGGING_LEVEL_DEFAULT,
                         OA_CONFIG_ORACLE_CONFLICT_LOGGING_LEVEL_MIN,
                         OA_CONFIG_ORACLE_CONFLICT_LOGGING_LEVEL_MAX,
                         1, oaConfigCallbackOracleConflictLoggingLevel ),

    ACL_CONF_DEF_UINT32( "ORACLE_ERROR_RETRY_COUNT",
                         OA_CONFIG_ERROR_RETRY_COUNT,
                         OA_CONFIG_ERROR_RETRY_COUNT_DEFAULT,
                         OA_CONFIG_ERROR_RETRY_COUNT_MIN,
                         OA_CONFIG_ERROR_RETRY_COUNT_MAX,
                         1, oaConfigCallbackErrorRetryCount ),

    ACL_CONF_DEF_UINT32( "ORACLE_ERROR_RETRY_INTERVAL",
                         OA_CONFIG_ERROR_RETRY_INTERVAL,
                         OA_CONFIG_ERROR_RETRY_INTERVAL_DEFAULT,
                         OA_CONFIG_ERROR_RETRY_INTERVAL_MIN,
                         OA_CONFIG_ERROR_RETRY_INTERVAL_MAX,
                         1, oaConfigCallbackErrorRetryInterval ),

    ACL_CONF_DEF_UINT32( "ORACLE_SKIP_ERROR",
                         OA_CONFIG_SKIP_ERROR,
                         OA_CONFIG_SKIP_ERROR_DEFAULT,
                         OA_CONFIG_SKIP_ERROR_MIN,
                         OA_CONFIG_SKIP_ERROR_MAX,
                         1, oaConfigCallbackSkipError ),
    
    ACL_CONF_DEF_SINT32( "ORACLE_SET_USER_TO_TABLE",
                         OA_CONFIG_ORACLE_SET_USER_TO_TABLE,
                         OA_CONFIG_ORACLE_SET_USER_TO_TABLE_DEFAULT,
                         OA_CONFIG_ORACLE_SET_USER_TO_TABLE_MIN,
                         OA_CONFIG_ORACLE_SET_USER_TO_TABLE_MAX,
                         1, oaConfigCallbackSetUserToTable ), 
    
    ACL_CONF_DEF_SINT32( "ORACLE_SET_COLUMN_TO_INSERT",
                         OA_CONFIG_ORACLE_SET_COLUMN_TO_INSERT,
                         OA_CONFIG_ORACLE_SET_COLUMN_TO_INSERT_DEFAULT,
                         OA_CONFIG_ORACLE_SET_COLUMN_TO_INSERT_MIN,
                         OA_CONFIG_ORACLE_SET_COLUMN_TO_INSERT_MAX,
                         1, oaConfigCallbackSetColumnToInsert ),                          
#endif

    ACL_CONF_DEF_SINT32("ALA_XLOG_POOL_SIZE",
                        OA_CONFIG_ALA_XLOG_POOL_SIZE,
                        OA_CONFIG_ALA_XLOG_POOL_SIZE_DEFAULT,
                        OA_CONFIG_ALA_XLOG_POOL_SIZE_MIN,
                        OA_CONFIG_ALA_XLOG_POOL_SIZE_MAX,
                        1, oaConfigCallbackAlaXLogPoolSize),

    ACL_CONF_DEF_UINT32("ALA_ACK_PER_XLOG_COUNT",
                        OA_CONFIG_ALA_ACK_PER_XLOG_COUNT,
                        OA_CONFIG_ALA_ACK_PER_XLOG_COUNT_DEFAULT,
                        OA_CONFIG_ALA_ACK_PER_XLOG_COUNT_MIN,
                        OA_CONFIG_ALA_ACK_PER_XLOG_COUNT_MAX,
                        1, oaConfigCallbackAlaAckPerXLogCount),

    ACL_CONF_DEF_SINT32("ALA_LOGGING_ACTIVE",
                        OA_CONFIG_ALA_LOGGING_ACTIVE,
                        OA_CONFIG_ALA_LOGGING_ACTIVE_DEFAULT,
                        OA_CONFIG_ALA_LOGGING_ACTIVE_MIN,
                        OA_CONFIG_ALA_LOGGING_ACTIVE_MAX,
                        1, oaConfigCallbackAlaLoggingActive),

    ACL_CONF_DEF_STRING("ALA_LOG_DIRECTORY",
                        OA_CONFIG_ALA_LOG_DIRECTORY,
                        OA_CONFIG_ALA_LOG_DIRECTORY_DEFAULT,
                        1, oaConfigCallbackAlaLogDirectory),

    ACL_CONF_DEF_STRING("ALA_LOG_FILE_NAME",
                        OA_CONFIG_ALA_LOG_FILE_NAME,
                        OA_CONFIG_ALA_LOG_FILE_NAME_DEFAULT,
                        1, oaConfigCallbackAlaLogFileName),

    ACL_CONF_DEF_UINT32("ALA_LOG_FILE_SIZE",
                        OA_CONFIG_ALA_LOG_FILE_SIZE,
                        OA_CONFIG_ALA_LOG_FILE_SIZE_DEFAULT,
                        OA_CONFIG_ALA_LOG_FILE_SIZE_MIN,
                        OA_CONFIG_ALA_LOG_FILE_SIZE_MAX,
                        1, oaConfigCallbackAlaLogFileSize),

    ACL_CONF_DEF_UINT32("ALA_MAX_LOG_FILE_NUMBER",
                        OA_CONFIG_ALA_MAX_LOG_FILE_NUMBER,
                        OA_CONFIG_ALA_MAX_LOG_FILE_NUMBER_DEFAULT,
                        OA_CONFIG_ALA_MAX_LOG_FILE_NUMBER_MIN,
                        OA_CONFIG_ALA_MAX_LOG_FILE_NUMBER_MAX,
                        1, oaConfigCallbackAlaMaxLogFileNumber),

    ACL_CONF_DEF_UINT32("ALA_RECEIVE_XLOG_TIMEOUT",
                        OA_CONFIG_ALA_RECEIVE_XLOG_TIMEOUT,
                        OA_CONFIG_ALA_RECEIVE_XLOG_TIMEOUT_DEFAULT,
                        OA_CONFIG_ALA_RECEIVE_XLOG_TIMEOUT_MIN,
                        OA_CONFIG_ALA_RECEIVE_XLOG_TIMEOUT_MAX,
                        1, oaConfigCallbackAlaReceiveXLogTimeout),

    ACL_CONF_DEF_STRING( "ALTIBASE_USER",
                         OA_CONFIG_CONSTRAINT_ALTIBASE_USER,
                         OA_CONFIG_CONSTRAINT_ALTIBASE_USER_DEFAULT,
                         1, oaConfigCallbackConstraintAltibaseUser ),

    ACL_CONF_DEF_STRING( "ALTIBASE_PASSWORD",
                         OA_CONFIG_CONSTRAINT_ALTIBASE_PASSWORD,
                         OA_CONFIG_CONSTRAINT_ALTIBASE_PASSWORD_DEFAULT,
                         1, oaConfigCallbackConstraintAltibasePassword ),

    ACL_CONF_DEF_STRING( "ALTIBASE_IP",
                         OA_CONFIG_CONSTRAINT_ALTIBASE_IP,
                         OA_CONFIG_CONSTRAINT_ALTIBASE_IP_DEFAULT,
                         1, oaConfigCallbackConstraintAltibaseIP ),

    ACL_CONF_DEF_SINT32( "ALTIBASE_PORT",
                         OA_CONFIG_CONSTRAINT_ALTIBASE_PORT,
                         OA_CONFIG_CONSTRAINT_ALTIBASE_PORT_DEFAULT,
                         OA_CONFIG_CONSTRAINT_ALTIBASE_PORT_MIN,
                         OA_CONFIG_CONSTRAINT_ALTIBASE_PORT_MAX,
                         1, oaConfigCallbackConstraintAltibasePort ),

    ACL_CONF_DEF_SINT32( "ADAPTER_ERROR_RESTART_COUNT",
                         OA_CONFIG_ADAPTER_ERROR_RESTART_COUNT,
                         OA_CONFIG_ADAPTER_ERROR_RESTART_COUNT_DEFAULT,
                         OA_CONFIG_ADAPTER_ERROR_RESTART_COUNT_MIN,
                         OA_CONFIG_ADAPTER_ERROR_RESTART_COUNT_MAX,
                         1, oaConfigCallbackAdapterErrorRestartCount ),
 
    ACL_CONF_DEF_SINT32( "ADAPTER_ERROR_RESTART_INTERVAL",
                         OA_CONFIG_ADAPTER_ERROR_RESTART_INTERVAL,
                         OA_CONFIG_ADAPTER_ERROR_RESTART_INTERVAL_DEFAULT,
                         OA_CONFIG_ADAPTER_ERROR_RESTART_INTERVAL_MIN,
                         OA_CONFIG_ADAPTER_ERROR_RESTART_INTERVAL_MAX,
                         1, oaConfigCallbackAdapterErrorRestartInterval ),

    ACL_CONF_DEF_END()
};

static acp_sint32_t oaConfigCallbackError(acp_str_t    *aFilePath,
                                          acp_sint32_t  aLine,
                                          acp_str_t    *aError,
                                          void         *aContext)
{
    ACP_UNUSED(aContext);
    ACP_UNUSED(aFilePath);
    ACP_UNUSED(aLine);
    ACP_UNUSED(aContext);

    (void)acpFprintf(ACP_STD_ERR, "Error: %s\n",
                     acpStrGetBuffer(aError));

    return 0;
}

static void oaConfigInitializeHandle(oaConfigHandle *aHandle)
{
    ACP_STR_INIT_DYNAMIC(aHandle->mLogFileName, 128, 128);
    (void)acpStrCpyCString(&(aHandle->mLogFileName),
                           OA_CONFIG_LOG_FILE_NAME_DEFAULT);

    aHandle->mLogBackupLimit = OA_CONFIG_LOG_BACKUP_LIMIT_DEFAULT;
    aHandle->mLogBackupSize  = OA_CONFIG_LOG_BACKUP_SIZE_DEFAULT;

    ACP_STR_INIT_DYNAMIC(aHandle->mMessagePath, 128, 128);
    (void)acpStrCpyCString(&(aHandle->mMessagePath),
                           OA_CONFIG_MESSAGE_PATH_DEFAULT);

    aHandle->mPerformanceTickCheck = OA_CONFIG_PERFORMANCE_TICK_CHECK_DEFAULT;

    ACP_STR_INIT_DYNAMIC(aHandle->mAlaSenderIP, 128, 128);
    (void)acpStrCpyCString(&(aHandle->mAlaSenderIP),
                           OA_CONFIG_ALA_SENDER_IP_DEFAULT);

    aHandle->mAlaSenderReplicationPort = OA_CONFIG_ALA_SENDER_REPLICATION_PORT_DEFAULT;
    
    aHandle->mAlaReceiverPort = OA_CONFIG_ALA_RECEIVER_PORT_DEFAULT;

    ACP_STR_INIT_DYNAMIC(aHandle->mAlaReplicationName, 128, 128);
    (void)acpStrCpyCString(&(aHandle->mAlaReplicationName),
                           OA_CONFIG_ALA_REPLICATION_NAME_DEFAULT);

    ACP_STR_INIT_DYNAMIC(aHandle->mAlaSocketType, 128, 128);
    (void)acpStrCpyCString(&(aHandle->mAlaSocketType),
                           OA_CONFIG_ALA_SOCKET_TYPE_DEFAULT);

#ifdef ALTIADAPTER
    ACP_STR_INIT_DYNAMIC(aHandle->mAltiHost, 128, 128);
    (void)acpStrCpyCString(&(aHandle->mAltiHost),
                           OA_CONFIG_OTHER_ALTIBASE_HOST_DEFAULT);

    aHandle->mAltiPort = OA_CONFIG_OTHER_ALTIBASE_PORT_DEFAULT;

    ACP_STR_INIT_DYNAMIC(aHandle->mAltiUser, 128, 128);
    (void)acpStrCpyCString(&(aHandle->mAltiUser),
                           OA_CONFIG_OTHER_ALTIBASE_USER_DEFAULT);

    ACP_STR_INIT_DYNAMIC(aHandle->mAltiPassword, 128, 128);
    (void)acpStrCpyCString(&(aHandle->mAltiPassword),
                           OA_CONFIG_OTHER_ALTIBASE_PASSWORD_DEFAULT);
#elif JDBCADAPTER
    ACP_STR_INIT_DYNAMIC(aHandle->mJDBCUser, 128, 128);
    (void)acpStrCpyCString(&(aHandle->mJDBCUser),
                           OA_CONFIG_OTHER_DATABASE_USER_DEFAULT);

    ACP_STR_INIT_DYNAMIC(aHandle->mJDBCPassword, 128, 128);
    (void)acpStrCpyCString(&(aHandle->mJDBCPassword),
                           OA_CONFIG_OTHER_DATABASE_PASSWORD_DEFAULT);   
 
#else
    ACP_STR_INIT_DYNAMIC(aHandle->mOracleServerAlias, 128, 128);
    (void)acpStrCpyCString(&(aHandle->mOracleServerAlias),
                           OA_CONFIG_ORACLE_SERVER_ALIAS_DEFAULT);

    ACP_STR_INIT_DYNAMIC(aHandle->mOracleUser, 128, 128);
    (void)acpStrCpyCString(&(aHandle->mOracleUser),
                           OA_CONFIG_ORACLE_USER_DEFAULT);

    ACP_STR_INIT_DYNAMIC(aHandle->mOraclePassword, 128, 128);
    (void)acpStrCpyCString(&(aHandle->mOraclePassword),
                           OA_CONFIG_ORACLE_PASSWORD_DEFAULT);
#endif
    aHandle->mAlaXLogPoolSize = OA_CONFIG_ALA_XLOG_POOL_SIZE_DEFAULT;

    aHandle->mAlaAckPerXLogCount = OA_CONFIG_ALA_ACK_PER_XLOG_COUNT_DEFAULT;

    aHandle->mAlaLoggingActive = OA_CONFIG_ALA_LOGGING_ACTIVE_DEFAULT;

    ACP_STR_INIT_DYNAMIC(aHandle->mAlaLogDirectory, 128, 128);
    (void)acpStrCpyCString(&(aHandle->mAlaLogDirectory),
                           OA_CONFIG_ALA_LOG_DIRECTORY_DEFAULT);

    ACP_STR_INIT_DYNAMIC(aHandle->mAlaLogFileName, 128, 128);
    (void)acpStrCpyCString(&(aHandle->mAlaLogFileName),
                           OA_CONFIG_ALA_LOG_FILE_NAME_DEFAULT);

    aHandle->mAlaLogFileSize = OA_CONFIG_ALA_LOG_FILE_SIZE_DEFAULT;

    aHandle->mAlaMaxLogFileNumber = OA_CONFIG_ALA_MAX_LOG_FILE_NUMBER_DEFAULT;

    aHandle->mAlaReceiveXlogTimeout = OA_CONFIG_ALA_RECEIVE_XLOG_TIMEOUT_DEFAULT;

    aHandle->mErrorRetryCount = OA_CONFIG_ERROR_RETRY_COUNT_DEFAULT;

    aHandle->mErrorRetryInterval = OA_CONFIG_ERROR_RETRY_INTERVAL_DEFAULT;

    aHandle->mSkipError = OA_CONFIG_SKIP_ERROR_DEFAULT;
    
    aHandle->mAdapterErrorRestartCount = OA_CONFIG_ADAPTER_ERROR_RESTART_COUNT_DEFAULT;

    aHandle->mAdapterErrorRestartInterval = OA_CONFIG_ADAPTER_ERROR_RESTART_INTERVAL_DEFAULT;
            
#ifdef ALTIADAPTER
    aHandle->mAltiAutocommitMode = OA_CONFIG_OTHER_ALTIBASE_AUTOCOMMIT_MODE_DEFAULT;
    aHandle->mAltiArrayDMLMaxSize = OA_CONFIG_OTHER_ALTIBASE_ARRAY_DML_MAX_SIZE_DEFAULT;

    aHandle->mAltiSkipInsert = OA_CONFIG_OTHER_ALTIBASE_SKIP_INSERT_DEFAULT;
    aHandle->mAltiSkipUpdate = OA_CONFIG_OTHER_ALTIBASE_SKIP_UPDATE_DEFAULT;
    aHandle->mAltiSkipDelete = OA_CONFIG_OTHER_ALTIBASE_SKIP_DELETE_DEFAULT;

    aHandle->mAltiConflictLoggingLevel = OA_CONFIG_OTHER_ALTIBASE_CONFLICT_LOGGING_LEVEL_DEFAULT;
    aHandle->mAltiGroupCommit = OA_CONFIG_OTHER_ALTIBASE_GROUP_COMMIT_DEFAULT;

    aHandle->mSetUserToTable = OA_CONFIG_OTHER_ALTIBASE_SET_USER_TO_TABLE_DEFAULT;
#elif JDBCADAPTER
    aHandle->mJDBCAutocommitMode = OA_CONFIG_OTHER_DATABASE_AUTOCOMMIT_MODE_DEFAULT;
    aHandle->mJDBCBatchDMLMaxSize = OA_CONFIG_OTHER_DATABASE_BATCH_DML_MAX_SIZE_DEFAULT;

    aHandle->mJDBCSkipInsert = OA_CONFIG_OTHER_DATABASE_SKIP_INSERT_DEFAULT;
    aHandle->mJDBCSkipUpdate = OA_CONFIG_OTHER_DATABASE_SKIP_UPDATE_DEFAULT;
    aHandle->mJDBCSkipDelete = OA_CONFIG_OTHER_DATABASE_SKIP_DELETE_DEFAULT;

    aHandle->mJDBCConflictLoggingLevel = OA_CONFIG_OTHER_DATABASE_CONFLICT_LOGGING_LEVEL_DEFAULT;
    aHandle->mJDBCGroupCommit = OA_CONFIG_OTHER_DATABASE_GROUP_COMMIT_DEFAULT;
    aHandle->mJDBCXmxOpt = OA_CONFIG_OTHER_DATABASE_JDBC_MAX_HEAP_SIZE_DEFAULT;

    aHandle->mSetUserToTable = OA_CONFIG_OTHER_DATABASE_SET_USER_TO_TABLE_DEFAULT;
    
    aHandle->mSetColumnToInsert = OA_CONFIG_OTHER_DATABASE_SET_COLUMN_TO_INSERT_DEFAULT;
    
    ACP_STR_INIT_DYNAMIC( aHandle->mJDBCDriverPath, 256, 256 );
    (void)acpStrCpyCString( &(aHandle->mJDBCDriverPath),
                            OA_CONFIG_OTHER_DATABASE_JDBC_DRIVER_PATH_DEFAULT );
    ACP_STR_INIT_DYNAMIC( aHandle->mJDBCDriverClass, 128, 128 );
    (void)acpStrCpyCString( &(aHandle->mJDBCDriverClass),
                            OA_CONFIG_OTHER_DATABASE_JDBC_DRIVER_CLASS_DEFAULT );
    ACP_STR_INIT_DYNAMIC( aHandle->mJDBCConnectionUrl, 1024, 1024 );
    (void)acpStrCpyCString( &(aHandle->mJDBCConnectionUrl),
                            OA_CONFIG_OTHER_DATABASE_JDBC_CONNECTION_URL_DEFAULT );
    
    ACP_STR_INIT_DYNAMIC( aHandle->mJDBCJVMOpt, 128, 128 );
    (void)acpStrCpyCString( &(aHandle->mJDBCJVMOpt),
                            OA_CONFIG_OTHER_DATABASE_JDBC_JVM_OPTION_DEFAULT );
#else
    aHandle->mOracleAutocommitMode = OA_CONFIG_ORACLE_AUTOCOMMIT_MODE_DEFAULT;
    aHandle->mOracleAsynchronousCommit = OA_CONFIG_ORACLE_ASYNCHRONOUS_COMMIT_DEFAULT;
    aHandle->mOracleGroupCommit = OA_CONFIG_ORACLE_GROUP_COMMIT_DEFAULT;

    aHandle->mOracleSkipInsert = OA_CONFIG_ORACLE_SKIP_INSERT_DEFAULT;
    aHandle->mOracleSkipUpdate = OA_CONFIG_ORACLE_SKIP_UPDATE_DEFAULT;
    aHandle->mOracleSkipDelete = OA_CONFIG_ORACLE_SKIP_DELETE_DEFAULT;

    aHandle->mOracleDirectPathInsert = OA_CONFIG_ORACLE_DIRECT_PATH_INSERT_DEFAULT;

    aHandle->mOracleArrayDMLMaxSize = OA_CONFIG_ORACLE_ARRAY_DML_MAX_SIZE_DEFAULT;
    aHandle->mOracleUpdateStatementCacheSize = OA_CONFIG_ORACLE_UPDATE_STATEMENT_CACHE_SIZE_DEFAULT;
    
    aHandle->mOracleConflictLoggingLevel = OA_CONFIG_ORACLE_CONFLICT_LOGGING_LEVEL_DEFAULT;

    aHandle->mSetUserToTable = OA_CONFIG_ORACLE_SET_USER_TO_TABLE_DEFAULT;
    
    aHandle->mSetColumnToInsert = OA_CONFIG_ORACLE_SET_COLUMN_TO_INSERT_DEFAULT;
#endif

    ACP_STR_INIT_DYNAMIC( aHandle->mConstraintAltibaseUser, 128, 128 );
    (void)acpStrCpyCString( &(aHandle->mConstraintAltibaseUser),
                            OA_CONFIG_CONSTRAINT_ALTIBASE_USER_DEFAULT );

    ACP_STR_INIT_DYNAMIC( aHandle->mConstraintAltibasePassword, 128, 128 );
    (void)acpStrCpyCString( &(aHandle->mConstraintAltibasePassword),
                            OA_CONFIG_CONSTRAINT_ALTIBASE_PASSWORD_DEFAULT );

    ACP_STR_INIT_DYNAMIC( aHandle->mConstraintAltibaseIP, 128, 128 );
    (void)acpStrCpyCString( &(aHandle->mConstraintAltibaseIP),
                            OA_CONFIG_CONSTRAINT_ALTIBASE_IP_DEFAULT );

    aHandle->mConstraintAltibasePort = OA_CONFIG_CONSTRAINT_ALTIBASE_PORT_DEFAULT;
}

static void oaConfigFinalizeHandle(oaConfigHandle *aHandle)
{
    ACP_STR_FINAL(aHandle->mLogFileName);
    ACP_STR_FINAL(aHandle->mMessagePath);
    ACP_STR_FINAL(aHandle->mAlaSenderIP);
    ACP_STR_FINAL(aHandle->mAlaReplicationName);
    ACP_STR_FINAL(aHandle->mAlaSocketType);
#ifdef ALTIADAPTER
    ACP_STR_FINAL( aHandle->mAltiHost );
    ACP_STR_FINAL(aHandle->mAltiUser);
    ACP_STR_FINAL(aHandle->mAltiPassword);
#elif JDBCADAPTER
    ACP_STR_FINAL(aHandle->mJDBCUser);
    ACP_STR_FINAL(aHandle->mJDBCPassword);
    ACP_STR_FINAL(aHandle->mJDBCDriverPath);
    ACP_STR_FINAL(aHandle->mJDBCDriverClass);
    ACP_STR_FINAL(aHandle->mJDBCConnectionUrl);
    ACP_STR_FINAL(aHandle->mJDBCJVMOpt);
#else
    ACP_STR_FINAL( aHandle->mOracleServerAlias );
    ACP_STR_FINAL(aHandle->mOracleUser);
    ACP_STR_FINAL(aHandle->mOraclePassword);
#endif
    ACP_STR_FINAL(aHandle->mAlaLogDirectory);
    ACP_STR_FINAL(aHandle->mAlaLogFileName);
    ACP_STR_FINAL( aHandle->mConstraintAltibaseUser );
    ACP_STR_FINAL( aHandle->mConstraintAltibasePassword );
    ACP_STR_FINAL( aHandle->mConstraintAltibaseIP );
}

ace_rc_t oaConfigLoad( oaContext *aContext,
                       oaConfigHandle **aHandle,
                       acp_str_t *aConfigPath )
{
    acp_rc_t sAcpRC = ACP_RC_SUCCESS;
    acp_sint32_t sConfigResult = 0;
    oaConfigHandle *sHandle = NULL;
    acp_sint32_t sStage = 0;

    sAcpRC = acpMemAlloc((void **)&sHandle, ACI_SIZEOF(*sHandle));
    ACE_TEST(ACP_RC_NOT_SUCCESS(sAcpRC));
    sStage = 1;

    oaConfigInitializeHandle(sHandle);
    sStage = 2;

    /* TODO: initialize oaConfigHandle structure. */

    sAcpRC = aclConfLoad(aConfigPath,
                         gConfigDefinition,
                         oaConfigCallbackDefault,
                         oaConfigCallbackError,
                         &sConfigResult,
                         (void *)sHandle);
    ACE_TEST(ACP_RC_NOT_SUCCESS(sAcpRC));

    *aHandle = sHandle;

    return ACE_RC_SUCCESS;

    ACE_EXCEPTION_END;

    switch (sStage)
    {
        case 2:
            oaConfigFinalizeHandle(sHandle);
        case 1:
            acpMemFree(sHandle);
        default:
            break;
    }

    return ACE_RC_FAILURE;
}

void oaConfigUnload( oaConfigHandle *aHandle )
{
    oaConfigFinalizeHandle(aHandle);

    acpMemFree(aHandle);
}

/*
 *
 */
void oaConfigGetAlaConfiguration(oaConfigHandle *aHandle,
                                 oaConfigAlaConfiguration *aConfig)
{
    aConfig->mSenderIP = &(aHandle->mAlaSenderIP);

    aConfig->mSenderReplicationPort = aHandle->mAlaSenderReplicationPort;

    aConfig->mReceiverPort = aHandle->mAlaReceiverPort;

    aConfig->mReplicationName = &(aHandle->mAlaReplicationName);

    aConfig->mSocketType = &(aHandle->mAlaSocketType);

    aConfig->mXLogPoolSize = aHandle->mAlaXLogPoolSize;

    aConfig->mAckPerXLogCount = aHandle->mAlaAckPerXLogCount;

    aConfig->mLoggingActive = aHandle->mAlaLoggingActive;

    aConfig->mLogDirectory = &(aHandle->mAlaLogDirectory);

    aConfig->mLogFileName = &(aHandle->mAlaLogFileName);

    aConfig->mLogFileSize = aHandle->mAlaLogFileSize;

    aConfig->mMaxLogFileNumber = aHandle->mAlaMaxLogFileNumber;
    
    aConfig->mReceiveXlogTimeout = aHandle->mAlaReceiveXlogTimeout;

    aConfig->mUseCommittedBuffer = ACP_TRUE;
}

#ifdef ALTIADAPTER
void oaConfigGetAltiConfiguration( oaConfigHandleForAlti *aHandle,
                                   oaConfigAltibaseConfiguration *aConfig )
{
    aConfig->mHost = &(aHandle->mAltiHost);
    aConfig->mPort = aHandle->mAltiPort;

    aConfig->mUser = &(aHandle->mAltiUser);
    aConfig->mPassword = &(aHandle->mAltiPassword);

    aConfig->mAutocommitMode = aHandle->mAltiAutocommitMode;
    aConfig->mArrayDMLMaxSize = aHandle->mAltiArrayDMLMaxSize;

    aConfig->mSkipInsert = aHandle->mAltiSkipInsert;
    aConfig->mSkipUpdate = aHandle->mAltiSkipUpdate;
    aConfig->mSkipDelete = aHandle->mAltiSkipDelete;

    aConfig->mConflictLoggingLevel = aHandle->mAltiConflictLoggingLevel;
    aConfig->mGroupCommit = aHandle->mAltiGroupCommit;

    aConfig->mErrorRetryCount = aHandle->mErrorRetryCount;
    aConfig->mErrorRetryInterval = aHandle->mErrorRetryInterval;
    aConfig->mSkipError = aHandle->mSkipError;
    
    aConfig->mSetUserToTable = aHandle->mSetUserToTable;
    
    aConfig->mAdapterErrorRestartCount = aHandle->mAdapterErrorRestartCount;
    aConfig->mAdapterErrorRestartInterval = aHandle->mAdapterErrorRestartInterval;
}

#elif JDBCADAPTER
void oaConfigGetJDBCConfiguration( oaConfigHandleForJDBC *aHandle,
                                   oaConfigJDBCConfiguration *aConfig )
{
    aConfig->mUser = &(aHandle->mJDBCUser);
    aConfig->mPassword = &(aHandle->mJDBCPassword);

    aConfig->mAutocommitMode = aHandle->mJDBCAutocommitMode;
    aConfig->mBatchDMLMaxSize = aHandle->mJDBCBatchDMLMaxSize;

    aConfig->mSkipInsert = aHandle->mJDBCSkipInsert;
    aConfig->mSkipUpdate = aHandle->mJDBCSkipUpdate;
    aConfig->mSkipDelete = aHandle->mJDBCSkipDelete;

    aConfig->mConflictLoggingLevel = aHandle->mJDBCConflictLoggingLevel;
    aConfig->mGroupCommit = aHandle->mJDBCGroupCommit;
    aConfig->mXmxOpt = aHandle->mJDBCXmxOpt;
    
    aConfig->mErrorRetryCount = aHandle->mErrorRetryCount;
    aConfig->mErrorRetryInterval = aHandle->mErrorRetryInterval;
    aConfig->mSkipError = aHandle->mSkipError;
    
    aConfig->mSetUserToTable = aHandle->mSetUserToTable;
    
    aConfig->mSetColumnToInsert = aHandle->mSetColumnToInsert;
    
    aConfig->mDriverPath = &(aHandle->mJDBCDriverPath);
    aConfig->mDriverClass = &(aHandle->mJDBCDriverClass);
    aConfig->mConnectionUrl = &(aHandle->mJDBCConnectionUrl);
    aConfig->mJVMOpt = &(aHandle->mJDBCJVMOpt);
}

#else
/*
 *
 */
void oaConfigGetOracleConfiguration(oaConfigHandleForOracle *aHandle,
                                    oaConfigOracleConfiguration *aConfig)
{
    aConfig->mServerAlias = &(aHandle->mOracleServerAlias);    

    aConfig->mUser = &(aHandle->mOracleUser);

    aConfig->mPassword = &(aHandle->mOraclePassword);

    aConfig->mAutocommitMode = aHandle->mOracleAutocommitMode;
    aConfig->mAsynchronousCommit = aHandle->mOracleAsynchronousCommit;
    aConfig->mGroupCommit = aHandle->mOracleGroupCommit;

    aConfig->mSkipInsert = aHandle->mOracleSkipInsert;
    aConfig->mSkipUpdate = aHandle->mOracleSkipUpdate;
    aConfig->mSkipDelete = aHandle->mOracleSkipDelete;

    aConfig->mDirectPathInsert = aHandle->mOracleDirectPathInsert;

    aConfig->mArrayDMLMaxSize = aHandle->mOracleArrayDMLMaxSize;
    aConfig->mUpdateStatementCacheSize = aHandle->mOracleUpdateStatementCacheSize;

    aConfig->mConflictLoggingLevel = aHandle->mOracleConflictLoggingLevel;

    aConfig->mErrorRetryCount = aHandle->mErrorRetryCount;
    aConfig->mErrorRetryInterval = aHandle->mErrorRetryInterval;
    aConfig->mSkipError = aHandle->mSkipError;

    aConfig->mSetUserToTable = aHandle->mSetUserToTable;
    
    aConfig->mSetColumnToInsert = aHandle->mSetColumnToInsert;

    aConfig->mAdapterErrorRestartCount = aHandle->mAdapterErrorRestartCount;
    aConfig->mAdapterErrorRestartInterval = aHandle->mAdapterErrorRestartInterval;
}
#endif

/*
 *
 */
void oaConfigGetBasicConfiguration(oaConfigHandle *aHandle,
                                   oaConfigBasicConfiguration *aConfig)
{
    aConfig->mLogFileName = &(aHandle->mLogFileName);

    aConfig->mLogBackupLimit = aHandle->mLogBackupLimit;
    aConfig->mLogBackupSize  = aHandle->mLogBackupSize;

    aConfig->mMessagePath = &(aHandle->mMessagePath);

    aConfig->mPerformanceTickCheck = aHandle->mPerformanceTickCheck;
}

/**
 * @breif  Constraint 프라퍼티를 얻는다.
 *
 * @param  aHandle All Properties
 * @param  aConfig Constraint Properties
 */
void oaConfigGetConstraintConfiguration( oaConfigHandle                  * aHandle,
                                         oaConfigConstraintConfiguration * aConfig )
{
    aConfig->mAltibaseUser     = &(aHandle->mConstraintAltibaseUser);
    aConfig->mAltibasePassword = &(aHandle->mConstraintAltibasePassword);
    aConfig->mAltibaseIP       = &(aHandle->mConstraintAltibaseIP);
    aConfig->mAltibasePort     = aHandle->mConstraintAltibasePort;
}
