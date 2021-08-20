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
 * Copyright 2016, Altibase Corporation or its subsidiaries.
 * All rights reserved.
 */

#ifndef __OA_JDBC_APPLIER_H__
#define __OA_JDBC_APPLIER_H__

#include <acp.h>
#include <jni.h>
#include <oaLogRecord.h>
#include <oaAlaLogConverter.h>
#include <oaApplierInterface.h>
#include <oaJNIInterface.h>

#define STATEMENT_EXECUTE_FAILED  -3
#define STATEMENT_SUCCESS_NO_INFO -2

#define CHARSET_MAX_LEN 15

struct oaAltibaseToJDBCCharSet
{
    acp_char_t    * mAltibaseCharSet;
    acp_char_t    * mJDBCCharSet;
};

typedef struct oaAltibaseToJDBCCharSet oaAltibaseToJDBCCharSet;

typedef struct preparedStatement
{
    jobject mInsert;
    jobject mDelete;
} preparedStatement;

struct oaJDBCApplierHandle
{
    acp_char_t * mCharset;
    acp_char_t * mNCharset;
    acp_bool_t mIsGroupCommit;
 
    acp_bool_t mSkipInsert;
    acp_bool_t mSkipUpdate;
    acp_bool_t mSkipDelete;
 
    acp_uint32_t mBatchDMLMaxSize;
    
    acp_bool_t mSetUserToTable;
 
    acp_bool_t mSetColumnToInsert;
    
    acp_sint32_t mTableCount;
    preparedStatement * mPreparedStatement;
    
    acp_uint32_t mSkipError;
    acp_uint32_t mErrorRetryCount;
    acp_uint32_t mErrorRetryInterval;
    
    acp_bool_t * mDMLResultArray;
    
    oaJNIInterfaceHandle mJNIInterfaceHandle;
};

typedef struct oaJDBCApplierHandle oaJDBCApplierHandle;

ace_rc_t initializeJDBCApplier( oaContext              * aContext,
                                oaConfigHandle         * aConfigHandle,
                                const ALA_Replication  * aAlaReplication,
                                oaJDBCApplierHandle   ** aJDBCApplierHandle );

void finalizeJDBCApplier( oaJDBCApplierHandle *aApplierHandle );

ace_rc_t oaJDBCApplierInitialize( oaContext                     * aContext,
                                  oaConfigJDBCConfiguration     * aConfig,
                                  const ALA_Replication         * aAlaReplication,  
                                  oaJDBCApplierHandle          ** aHandle );

void oaJDBCApplierFinalize( oaJDBCApplierHandle * aHandle );

ace_rc_t oaJDBCApplierConnect( oaContext * aContext, oaJDBCApplierHandle * aHandle );

void oaJDBCApplierDisconnect( oaJDBCApplierHandle * aHandle );

ace_rc_t oaJDBCApplierApplyLogRecordList( oaContext           * aContext,
                                          oaJDBCApplierHandle * aHandle,
                                          acp_list_t          * aLogRecordList,
                                          oaLogSN               aPrevLastProcessedSN,
                                          oaLogSN             * aLastProcessedSN );
                                          
ace_rc_t oaJDBCApplierApplyLogRecord( oaContext           * aContext,
                                      oaJDBCApplierHandle * aHandle,
                                      oaLogRecord         * aLogRecord );

void oaJDBCInitializeDMLResultArray( acp_bool_t * aDMLResultArray, acp_uint32_t aArrayDMLMaxSize );

void oaFinalizeJAVAVM();

ace_rc_t oaJDBCExecuteLogInSql( oaContext * aContext, oaJDBCApplierHandle * aHandle );

ace_rc_t oaJDBCInitializeSkipErrorList( oaContext * aContext, oaJDBCApplierHandle * aHandle );

void oaJDBCFinalizeSkipErrorList( oaJDBCApplierHandle * aHandle );

#endif /* __OA_JDBC_APPLIER_H__ */
