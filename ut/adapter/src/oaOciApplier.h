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
 */

#ifndef __OA_OCI_APPLIER_H__
#define __OA_OCI_APPLIER_H__

#include <acp.h>
#include <oci.h>
#include <oaLogRecord.h>

#define DEFAULT_ERROR_CODE (0)
#define OCI_ERROR_STR_LEN 5 + 1

typedef struct preparedStatement
{

    OCIStmt *mInsert;
    OCIStmt *mUpdate;   /* Used, if ORACLE_UPDATE_STATEMENT_CACHE_SIZE = 0 */
    OCIStmt *mDelete;

} preparedStatement;

struct oaOciApplierHandle
{
    OCIEnv *mEnv;

    OCISvcCtx *mSvcCtx;

    OCIError * mError;
    OCIError * mArrayDMLError;
    OCIError * mRowError;

    OCIServer *mServer;

    OCISession *mSession;

    acp_str_t *mServerAlias;

    acp_str_t *mUser;

    acp_str_t *mPassword;

    acp_uint32_t mCommitMode;
    acp_bool_t   mIsAsynchronousCommit;
    acp_bool_t   mIsGroupCommit;
    acp_bool_t   mDMLExecutedFlag;

    acp_bool_t mSkipInsert;
    acp_bool_t mSkipUpdate;
    acp_bool_t mSkipDelete;
    acp_uint32_t mSkipError;
    acp_list_t   mSkipErrorList;

    acp_bool_t mIsDirectPathInsert;

    acp_uint32_t mArrayDMLMaxSize;
    acp_uint32_t mUpdateStatementCacheSize;

    acp_uint32_t mConflictLoggingLevel;

    acp_uint32_t mErrorRetryCount;
    acp_uint32_t mErrorRetryInterval;

    acp_bool_t mSetUserToTable;
    
    acp_bool_t mSetColumnToInsert;
    
    acp_bool_t mIsSkipErrorList;

    /*
     * prepared statements for each table
     */
    acp_sint32_t mTableCount;
    preparedStatement *mPreparedStatement;
};

typedef struct oaOciApplierHandle oaOciApplierHandle;

ace_rc_t initializeOciApplier( oaContext           * aContext,
                               oaConfigHandle      * aConfigHandle,
                               acp_sint32_t          aTableCount,
                               oaOciApplierHandle ** aOciApplierHandle );

void oaFinalizeOCILibrary( void );

void finalizeOciApplier( oaOciApplierHandle *aOciApplierHandle );

extern ace_rc_t oaOciApplierInitialize( oaContext                   * aContext,
                                        oaConfigOracleConfiguration * aConfig,
                                        acp_sint32_t                  aTableCount,
                                        oaOciApplierHandle         ** aHandle );
extern void oaOciApplierFinalize( oaOciApplierHandle *aHandle );

extern ace_rc_t oaOciApplierLogIn(oaContext *aContext,
                                  oaOciApplierHandle *aHandle);
extern void oaOciApplierLogOut(oaOciApplierHandle *aHandle);

extern ace_rc_t oaOciApplierApplyLogRecordList( oaContext          * aContext,
                                                oaOciApplierHandle * aHandle,
                                                acp_list_t         * aLogRecordList,
                                                oaLogSN              aPrevLastProcessedSN,
                                                oaLogSN            * aLastProcessedSN );

ace_rc_t oaOciApplierApplyLogRecord( oaContext          * aContext,
                                     oaOciApplierHandle * aHandle,
                                     oaLogRecord        * aLogRecord );

ace_rc_t oaOciInitializeSkipErrorList( oaContext          *aContext,
                                       oaOciApplierHandle *aHandle );

void oaOciFinalizeSkipErrorList( oaOciApplierHandle *aOciApplierHandle );

#endif /* __OA_OCI_APPLIER_H__ */
