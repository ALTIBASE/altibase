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

#include <acp.h>
#include <ace.h>
#include <acl.h>
#include <aciTypes.h>
#include <aciVa.h>

#include <alaAPI.h>

#include <oci.h>

#include <oaContext.h>
#include <oaConfig.h>
#include <oaLog.h>
#include <oaMsg.h>

#include <oaLogRecord.h>
#include <oaAlaLogConverter.h>

#include <oaPerformance.h>
#include <oaOciApplier.h>
#include <oaApplierInterface.h>

static acp_bool_t gIsOCIInitialized = ACP_FALSE;

static void writeOciErrorMessageToLogFile(oaOciApplierHandle *aHandle)
{
    int sErrorCode;
    unsigned char sMessage[1024];
    
    (void)OCIErrorGet((dvoid *)aHandle->mError, (ub4)1, (text *)NULL,
                      &sErrorCode, sMessage, ACI_SIZEOF(sMessage),
                      (ub4)OCI_HTYPE_ERROR);
    
    oaLogMessage(OAM_ERR_LIBRARY, "OCI", sErrorCode, sMessage);
}


static int getOciErrorCode( oaOciApplierHandle *aHandle )
{
    int sErrorCode = DEFAULT_ERROR_CODE;
    unsigned char sMessage[1024];
    
    if ( OCIErrorGet( (dvoid *)aHandle->mError, (ub4)1, (text *)NULL,
                      &sErrorCode, sMessage, ACI_SIZEOF(sMessage),
                      (ub4)OCI_HTYPE_ERROR ) != OCI_SUCCESS )
    {
        sErrorCode = -1;
    }

    return sErrorCode;
}

static void oaCheckErrorCode( oaOciApplierHandle *aHandle, int aErrorCode )
{
    int                 sErrorCode       = DEFAULT_ERROR_CODE;
    int                *sSkipError       = NULL;
    acp_list_node_t    *sIterator        = NULL;
    acp_list_t         *sListHead        = &(aHandle->mSkipErrorList);

    if ( aErrorCode == DEFAULT_ERROR_CODE )
    {
        sErrorCode = getOciErrorCode( aHandle );
    }
    else
    {
        sErrorCode = aErrorCode;
    }
    
    ACP_LIST_ITERATE( sListHead, sIterator )
    {
        sSkipError = (int *)( sIterator->mObj );

        if ( *sSkipError == sErrorCode )
        {
            aHandle->mIsSkipErrorList = ACP_TRUE;
            break;
        }
    }
}

/**
 * @breif  Array DML 실행 오류를 출력한다.
 *
 * @param  aContext   Context
 * @param  aHandle    Oracle Applier Handle
 * @param  aLogRecord Log Record Union
 * @param  aStatement Oracle Statement
 */
static void writeArrayDMLOciErrorMessageToLogFile( oaContext          * aContext,
                                                   oaOciApplierHandle * aHandle,
                                                   oaLogRecord        * aLogRecord,
                                                   OCIStmt            * aStatement )
{
    int           sErrorCode = 0;
    unsigned char sMessage[1024] = { '\0', };
    ub4           sErrorCount = 0;
    ub4           sIndex;
    ub4           sRowOffset;   /* To do something to error rows */

    /* Get the number of errors, a different error handler mArrayDMLError is used so that
     * the state of mError is not changed
     */
    ACE_TEST( OCIAttrGet( (CONST dvoid *)aStatement,
                          (ub4)OCI_HTYPE_STMT,
                          (dvoid *)&sErrorCount,
                          (ub4 *)0,
                          (ub4)OCI_ATTR_NUM_DML_ERRORS,
                          aHandle->mArrayDMLError ) );

    if ( sErrorCount > 0 )
    {
        for ( sIndex = 0; sIndex < sErrorCount; sIndex++ )
        {
            ACE_TEST( OCIParamGet( (CONST dvoid *)aHandle->mError,
                                   (ub4)OCI_HTYPE_ERROR,
                                   aHandle->mArrayDMLError,
                                   (dvoid **)&aHandle->mRowError,
                                   sIndex ) );

            /* The row offset into the DML array at which the error occurred */
            ACE_TEST( OCIAttrGet( (CONST dvoid *)aHandle->mRowError,
                                  (ub4)OCI_HTYPE_ERROR,
                                  (dvoid *)&sRowOffset,
                                  (ub4 *)0,
                                  (ub4)OCI_ATTR_DML_ROW_OFFSET,
                                  aHandle->mArrayDMLError ) );

            oaLogRecordDumpDML( aLogRecord, (acp_uint32_t)sRowOffset );

            if ( OCIErrorGet( (dvoid *)aHandle->mRowError, (ub4)1, (text *)NULL,
                              &sErrorCode, sMessage, ACI_SIZEOF(sMessage),
                              (ub4)OCI_HTYPE_ERROR )
                 == OCI_SUCCESS )
            {
                oaLogMessage( OAM_ERR_LIBRARY, "OCI", sErrorCode, sMessage );
                
                oaCheckErrorCode( aHandle, sErrorCode );
            }
            else
            {
                /* Nothing to do */
            }
        }
    }
    else
    {
        /* Nothing to do */
    }

    return;

    ACE_EXCEPTION_END;

    if ( OCIErrorGet( (dvoid *)aHandle->mArrayDMLError, (ub4)1, (text *)NULL,
                      &sErrorCode, sMessage, ACI_SIZEOF(sMessage),
                      (ub4)OCI_HTYPE_ERROR )
         == OCI_SUCCESS )
    {
        oaLogMessage( OAM_ERR_LIBRARY, "OCI", sErrorCode, sMessage );
    }
    else
    {
        /* Nothing to do */
    }

    return;
}

static void writeOciDCLLog( oaOciApplierHandle * aHandle )
{
    if ( ( aHandle->mConflictLoggingLevel == 1 ) ||
         ( aHandle->mConflictLoggingLevel == 2 ) )
    {
        writeOciErrorMessageToLogFile(aHandle);
    }
    else
    {
        /* Nothing to do */
    }
}

static void writeOciDumpTypeLog( oaOciApplierHandle * aHandle,
                                 oaLogRecord        * aLogRecord )
{
    if ( aHandle->mConflictLoggingLevel == 2 )
    {
        oaLogRecordDumpType( aLogRecord );
    }
    else
    {
        /* Nothing to do */
    }
}

static void writeOciDMLLog( oaContext          * aContext,
                            oaOciApplierHandle * aHandle, 
                            oaLogRecord        * aLogRecord,
                            OCIStmt            * aStatement )
{
    acp_uint32_t sArrayDMLCount = 1;

    switch ( aHandle->mConflictLoggingLevel )
    {
        case 2:
            writeOciErrorMessageToLogFile(aHandle);
            switch ( aLogRecord->mCommon.mType )
            {
                case OA_LOG_RECORD_TYPE_INSERT:
                    sArrayDMLCount = ((oaLogRecordInsert*)aLogRecord)->mArrayDMLCount;
                    break;
                case OA_LOG_RECORD_TYPE_UPDATE:
                    sArrayDMLCount = ((oaLogRecordUpdate*)aLogRecord)->mArrayDMLCount;
                    break;
                case OA_LOG_RECORD_TYPE_DELETE:
                    sArrayDMLCount = ((oaLogRecordDelete*)aLogRecord)->mArrayDMLCount;
                    break;
                default:
                    break;
            }

            if ( sArrayDMLCount == 1 )
            {
                oaLogRecordDumpDML( aLogRecord, 0 );
            }
            else
            {
                writeArrayDMLOciErrorMessageToLogFile( aContext,
                                                       aHandle,
                                                       aLogRecord,
                                                       aStatement );
            }
            break;
        case 1:
            writeOciErrorMessageToLogFile(aHandle);
            break;
        case 0:
            break;
        default:
            break;
    }
}

/* Oracle */
ace_rc_t initializeOciApplier( oaContext           * aContext,
                               oaConfigHandle      * aConfigHandle,
                               acp_sint32_t          aTableCount,
                               oaOciApplierHandle ** aOciApplierHandle )
{
    ace_rc_t sAceRC = ACE_RC_SUCCESS;

    oaConfigOracleConfiguration sOracleConfiguration;
    oaOciApplierHandle *sOciApplierHandle = NULL;

    oaConfigGetOracleConfiguration(aConfigHandle, &sOracleConfiguration);
    sAceRC = oaOciApplierInitialize( aContext, 
                                     &sOracleConfiguration,
                                     aTableCount,
                                     &sOciApplierHandle );
    ACE_TEST(sAceRC != ACE_RC_SUCCESS);

    sAceRC = oaOciApplierLogIn(aContext, sOciApplierHandle);
    ACE_TEST(sAceRC != ACE_RC_SUCCESS);

    sAceRC = oaOciInitializeSkipErrorList(aContext, sOciApplierHandle);
    ACE_TEST(sAceRC != ACE_RC_SUCCESS);

    *aOciApplierHandle = sOciApplierHandle;

    return ACE_RC_SUCCESS;

    ACE_EXCEPTION_END;

    return ACE_RC_FAILURE;
}

void oaFinalizeOCILibrary()
{
    /* static var bool */
    if ( gIsOCIInitialized == ACP_TRUE )
    {
        gIsOCIInitialized = ACP_FALSE;
        (void)OCITerminate( OCI_DEFAULT );
    }
    else
    {
        /* nothing to do */
    }
}

void finalizeOciApplier( oaOciApplierHandle *aOciApplierHandle )
{
    oaOciFinalizeSkipErrorList( aOciApplierHandle );

    oaOciApplierLogOut(aOciApplierHandle);

    oaOciApplierFinalize( aOciApplierHandle );
}

/*
 *
 */
ace_rc_t oaOciApplierInitialize( oaContext                   * aContext,
                                 oaConfigOracleConfiguration * aConfig,
                                 acp_sint32_t                  aTableCount,
                                 oaOciApplierHandle         ** aHandle )
{
    acp_rc_t sAcpRC = ACP_RC_SUCCESS;
    oaOciApplierHandle *sHandle = NULL;
    acp_sint32_t sStage = 0;

    sAcpRC = acpMemCalloc((void **)&sHandle, 1, ACI_SIZEOF(*sHandle));
    ACE_TEST_RAISE(ACP_RC_NOT_SUCCESS(sAcpRC), ERROR_MEM_CALLOC);
    sStage = 1;

    /* Initialize the OCI Process & Environment */
    ACE_TEST_RAISE( OCIEnvCreate( (OCIEnv **)&(sHandle->mEnv),
                                  (ub4)(OCI_DEFAULT | OCI_ENV_NO_MUTEX),
                                  (dvoid *)0,
                                  (dvoid *(*)(dvoid *, size_t))0,
                                  (dvoid *(*)(dvoid *, dvoid *, size_t))0,
                                  (void (*)(dvoid *, dvoid *))0,
                                  (size_t)0,
                                  (dvoid **)0 ),
                    ERROR_CREATE_ENV );
    sStage = 2;
    gIsOCIInitialized = ACP_TRUE;

    /* Allocate a service handle */
    ACE_TEST_RAISE(OCIHandleAlloc((dvoid *)sHandle->mEnv,
                                  (dvoid **)&(sHandle->mSvcCtx),
                                  (ub4)OCI_HTYPE_SVCCTX, (size_t)0,
                                  (dvoid **)0),
                   ERROR_ALLOC_SERVICE_HANDLE);
    sStage = 3;

    /* Allocate an error handle */
    ACE_TEST_RAISE(OCIHandleAlloc((dvoid *)sHandle->mEnv,
                                  (dvoid **)&(sHandle->mError),
                                  (ub4)OCI_HTYPE_ERROR, (size_t)0,
                                  (dvoid **)0),
                   ERROR_ALLOC_ERROR_HANDLE);
    sStage = 4;

    ACE_TEST_RAISE( OCIHandleAlloc( (dvoid *)sHandle->mEnv,
                                    (dvoid **)&(sHandle->mArrayDMLError),
                                    (ub4)OCI_HTYPE_ERROR, (size_t)0,
                                    (dvoid **)0 ),
                    ERROR_ALLOC_ERROR_HANDLE );
    sStage = 5;

    ACE_TEST_RAISE( OCIHandleAlloc( (dvoid *)sHandle->mEnv,
                                    (dvoid **)&(sHandle->mRowError),
                                    (ub4)OCI_HTYPE_ERROR, (size_t)0,
                                    (dvoid **)0 ),
                    ERROR_ALLOC_ERROR_HANDLE );
    sStage = 6;

    /* Allocate a server handle */
    ACE_TEST_RAISE(OCIHandleAlloc((dvoid *)sHandle->mEnv,
                                  (dvoid **)&(sHandle->mServer),
                                  (ub4)OCI_HTYPE_SERVER, (size_t)0,
                                  (dvoid **)0),
                   ERROR_GET_SERVER_HANDLE);
    sStage = 7;

    /* Allocate a authentication handle */
    ACE_TEST_RAISE(OCIHandleAlloc((dvoid *)sHandle->mEnv,
                                  (dvoid **)&(sHandle->mSession),
                                  (ub4)OCI_HTYPE_SESSION, (size_t)0,
                                  (dvoid **)0),
                   ERROR_ALLOC_AUTH_HANDLE);
    sStage = 8;

    sAcpRC = acpMemCalloc((void **)&(sHandle->mPreparedStatement),
                          aTableCount, ACI_SIZEOF(*(sHandle->mPreparedStatement)));
    ACE_TEST_RAISE(ACP_RC_NOT_SUCCESS(sAcpRC), ERROR_MEM_CALLOC);
    sHandle->mTableCount = aTableCount;

    sHandle->mServerAlias = aConfig->mServerAlias;
    sHandle->mUser = aConfig->mUser;
    sHandle->mPassword = aConfig->mPassword;

    sHandle->mCommitMode = aConfig->mAutocommitMode ? OCI_COMMIT_ON_SUCCESS : OCI_DEFAULT;
    sHandle->mIsAsynchronousCommit = ( aConfig->mAsynchronousCommit == 1 )
                                  ? ACP_TRUE : ACP_FALSE;
    sHandle->mIsGroupCommit = ( aConfig->mGroupCommit == 1 )
                            ? ACP_TRUE : ACP_FALSE;

    sHandle->mSkipInsert = ( aConfig->mSkipInsert == 1 ) ? ACP_TRUE : ACP_FALSE;
    sHandle->mSkipUpdate = ( aConfig->mSkipUpdate == 1 ) ? ACP_TRUE : ACP_FALSE;
    sHandle->mSkipDelete = ( aConfig->mSkipDelete == 1 ) ? ACP_TRUE : ACP_FALSE;

    sHandle->mSetUserToTable = ( aConfig->mSetUserToTable == 1 ) ? ACP_TRUE : ACP_FALSE;
    
    sHandle->mSetColumnToInsert = ( aConfig->mSetColumnToInsert == 1 ) ? ACP_TRUE : ACP_FALSE;
    
    sHandle->mIsDirectPathInsert = ( aConfig->mDirectPathInsert == 1 )
                                 ? ACP_TRUE : ACP_FALSE;

    sHandle->mArrayDMLMaxSize = aConfig->mArrayDMLMaxSize;
    sHandle->mUpdateStatementCacheSize = aConfig->mUpdateStatementCacheSize;
    
    sHandle->mConflictLoggingLevel = aConfig->mConflictLoggingLevel;

    sHandle->mErrorRetryCount = aConfig->mErrorRetryCount;
    sHandle->mErrorRetryInterval = aConfig->mErrorRetryInterval;
    sHandle->mSkipError = aConfig->mSkipError;
    sHandle->mIsSkipErrorList = ACP_FALSE;

    *aHandle = sHandle;

    return ACE_RC_SUCCESS;

    ACE_EXCEPTION(ERROR_MEM_CALLOC)
    {
        oaLogMessage(OAM_ERR_MEM_CALLOC);
    }
    ACE_EXCEPTION( ERROR_CREATE_ENV )
    {
        oaLogMessage( OAM_ERR_CREATE_ENV, "OCI" );
    }
    ACE_EXCEPTION(ERROR_ALLOC_SERVICE_HANDLE)
    {
        oaLogMessage(OAM_ERR_ALLOC_SERVICE_HANDLE, "OCI");
    }
    ACE_EXCEPTION(ERROR_ALLOC_ERROR_HANDLE)
    {
        oaLogMessage(OAM_ERR_ALLOC_ERROR_HANDLE, "OCI");
    }
    ACE_EXCEPTION(ERROR_GET_SERVER_HANDLE)
    {
        writeOciErrorMessageToLogFile(sHandle);
    }
    ACE_EXCEPTION(ERROR_ALLOC_AUTH_HANDLE)
    {
        writeOciErrorMessageToLogFile(sHandle);
    }
    ACE_EXCEPTION_END;

    switch (sStage)
    {
        case 8 :
            (void)OCIHandleFree((dvoid *)sHandle->mSession,
                                (ub4)OCI_HTYPE_SESSION);
        case 7 :
            (void)OCIHandleFree((dvoid *)sHandle->mServer,
                                (ub4)OCI_HTYPE_SERVER);
        case 6 :
            (void)OCIHandleFree( (dvoid *)sHandle->mRowError,
                                 (ub4)OCI_HTYPE_ERROR );
        case 5 :
            (void)OCIHandleFree( (dvoid *)sHandle->mArrayDMLError,
                                 (ub4)OCI_HTYPE_ERROR );
        case 4 :
            (void)OCIHandleFree((dvoid *)sHandle->mError,
                                (ub4)OCI_HTYPE_ERROR);
        case 3 :
            (void)OCIHandleFree((dvoid *)sHandle->mSvcCtx,
                                (ub4)OCI_HTYPE_SVCCTX);
        case 2 :
            (void)OCIHandleFree((dvoid *)sHandle->mEnv,
                                (ub4)OCI_HTYPE_ENV);
        case 1 :
            acpMemFree(sHandle);
        default :
            break;
    }

    return ACE_RC_FAILURE;
}

static void freePreparedStatement(preparedStatement *aPreparedStatement,
                                  acp_sint32_t aPreparedStatementCount)
{
    acp_sint32_t i = 0;

    for (i = 0; i < aPreparedStatementCount; i++)
    {
        if (aPreparedStatement[i].mInsert != NULL)
        {
            (void)OCIHandleFree((dvoid *)aPreparedStatement[i].mInsert,
                                (ub4)OCI_HTYPE_STMT);
        }
        else
        {
            /* Nothing to do */
        }

        if ( aPreparedStatement[i].mUpdate != NULL )
        {
            (void)OCIHandleFree( (dvoid *)aPreparedStatement[i].mUpdate,
                                 (ub4)OCI_HTYPE_STMT );
        }
        else
        {
            /* Nothing to do */
        }

        if (aPreparedStatement[i].mDelete != NULL)
        {
            (void)OCIHandleFree((dvoid *)aPreparedStatement[i].mDelete,
                                (ub4)OCI_HTYPE_STMT);
        }
        else
        {
            /* Nothing to do */
        }
    }
}

/*
 *
 */
void oaOciApplierFinalize( oaOciApplierHandle *aHandle )
{
    freePreparedStatement(aHandle->mPreparedStatement,
                          aHandle->mTableCount);
    acpMemFree(aHandle->mPreparedStatement);

    (void)OCIHandleFree((dvoid *)aHandle->mServer, (ub4)OCI_HTYPE_SERVER);

    (void)OCIHandleFree((dvoid *)aHandle->mSvcCtx, (ub4)OCI_HTYPE_SVCCTX);

    (void)OCIHandleFree( (dvoid *)aHandle->mRowError, (ub4)OCI_HTYPE_ERROR );
    (void)OCIHandleFree( (dvoid *)aHandle->mArrayDMLError, (ub4)OCI_HTYPE_ERROR );
    (void)OCIHandleFree( (dvoid *)aHandle->mError, (ub4)OCI_HTYPE_ERROR );

    (void)OCIHandleFree((dvoid *)aHandle->mSession, (ub4)OCI_HTYPE_SESSION);

    (void)OCIHandleFree((dvoid *)aHandle->mEnv, (ub4)OCI_HTYPE_ENV);

    acpMemFree(aHandle);
}

/*
 *
 */
ace_rc_t executeSQL( oaContext          * aContext, 
                     oaOciApplierHandle * aHandle,
                     acp_str_t          * aQueryStr )
{
    OCIStmt    * sStatement = NULL;
    acp_bool_t   sStatementAllocFlag = ACP_FALSE;
    
    ACE_TEST( OCIHandleAlloc( (dvoid *)aHandle->mEnv,
                              (dvoid **)&sStatement,
                              (ub4)OCI_HTYPE_STMT, 0, (dvoid **)0 ) );
    sStatementAllocFlag = ACP_TRUE;

    ACE_TEST( OCIStmtPrepare( sStatement,
                              aHandle->mError,
                              (unsigned char *)acpStrGetBuffer( aQueryStr ),
                              (ub4)acpStrGetLength( aQueryStr ),
                              (ub4)OCI_NTV_SYNTAX,
                              (ub4)OCI_DEFAULT ) );
    
    ACE_TEST( OCIStmtExecute( aHandle->mSvcCtx,
                              sStatement,
                              aHandle->mError,
                              (ub4)1, (ub4)0,
                              (CONST OCISnapshot *)0, (OCISnapshot *)0,
                              (ub4)OCI_COMMIT_ON_SUCCESS ) );

    sStatementAllocFlag = ACP_FALSE;

    return ACE_RC_SUCCESS;

    ACE_EXCEPTION_END;

    writeOciErrorMessageToLogFile( aHandle );
    
    oaLogMessage( OAM_ERR_OCI_ERR_SQL, (unsigned char *)acpStrGetBuffer( aQueryStr ) );

    if ( sStatementAllocFlag == ACP_TRUE )
    {
        (void)OCIHandleFree( (dvoid *)sStatement, (ub4)OCI_HTYPE_STMT );
    }

    return ACE_RC_FAILURE;
}

/*
 *
 */
acp_bool_t isSQLEndLineAndTrim( acp_char_t* aBuffer )
{
    acp_uint32_t        i = 0;
    acp_bool_t          sIsEndSQL = ACP_FALSE;
    
    for ( i = 0 ; aBuffer[i] != '\0' ; i++ )
    {
        if ( (aBuffer[i] == '\r') || (aBuffer[i] == '\n') )
        {
            aBuffer[i] = ' ';
            break;
        }
        else if ( aBuffer[i] == ';' )
        {
            aBuffer[i] = '\0';
            sIsEndSQL = ACP_TRUE;            
        }
        else
        {
            /* Nothing to do */
        }
    }
    
    return sIsEndSQL;
}

/*
 *
 */
ace_rc_t readSQLFile( oaContext          * aContext, 
                      acp_std_file_t     * aFile,
                      acp_str_t          * aQueryStr,
                      acp_bool_t         * aIsSQLEnd,
                      acp_bool_t         * aIsEOF )
{
    acp_char_t      sLineBuffer[OA_ADAPTER_SQL_READ_BUFF_SIZE + 1];
    acp_char_t      sErrMsg[1024];
    acp_sint32_t    sNewSize = 0;
    acp_sint32_t    sBufferSize = 0;
    acp_bool_t      sIsEOF = ACP_FALSE;
    acp_bool_t      sIsSQLEnd = ACP_FALSE;
    ace_rc_t        sAcpRC =  ACE_RC_SUCCESS;
        
    while ( (sIsEOF == ACP_FALSE) && (sIsSQLEnd == ACP_FALSE) )
    {
        sLineBuffer[0] = '\0';
        
        sAcpRC = acpStdGetCString( aFile,
                                   sLineBuffer,
                                   OA_ADAPTER_SQL_READ_BUFF_SIZE + 1 );
        ACE_TEST_RAISE( ACP_RC_NOT_SUCCESS(sAcpRC), ERROR_LOGIN_FILE_READ );
        
        sBufferSize = (acp_sint32_t)acpCStrLen( sLineBuffer, OA_ADAPTER_SQL_READ_BUFF_SIZE + 1 );
        sNewSize += sBufferSize;
        
        ACE_TEST_RAISE( OA_ADAPTER_SQL_READ_BUFF_SIZE < sNewSize, ERROR_LOGIN_SQL_MAX_SIZE );
        
        sIsSQLEnd = isSQLEndLineAndTrim( sLineBuffer );
        
        acpStrCatCString( aQueryStr, sLineBuffer );
        
        (void)acpStdIsEOF( aFile, &sIsEOF );
    }
        
    *aIsSQLEnd = sIsSQLEnd;
    *aIsEOF = sIsEOF;
    
    return ACE_RC_SUCCESS;

    ACE_EXCEPTION( ERROR_LOGIN_FILE_READ )
    {        
        acpErrorString( sAcpRC, sErrMsg, 1024 );
        oaLogMessage( OAM_ERR_FILE_READ, OA_ADAPTER_LOGIN_SQL_FILE_NAME, sErrMsg );
    }
    ACE_EXCEPTION( ERROR_LOGIN_SQL_MAX_SIZE )
    {
        oaLogMessage( OAM_ERR_LOGIN_SQL_MAX_SIZE );
    }
    ACE_EXCEPTION_END;

    return ACE_RC_FAILURE;
}

/*
 *
 */
ace_rc_t executeLogInSqlFile( oaContext          * aContext, 
                              oaOciApplierHandle * aHandle,
                              acp_std_file_t     * aFile )
{
    acp_bool_t      sIsSQLEnd = ACP_FALSE;
    acp_bool_t      sIsEOF = ACP_FALSE;
    
    ACP_STR_DECLARE_DYNAMIC( sQueryStr );
    ACP_STR_INIT_DYNAMIC( sQueryStr, 
                          OA_ADAPTER_SQL_READ_BUFF_SIZE + 1, 
                          OA_ADAPTER_SQL_READ_BUFF_SIZE + 1 );
    
    while ( sIsEOF == ACP_FALSE )
    {
        acpStrClear( &sQueryStr );
        
        ACE_TEST( readSQLFile( aContext,
                               aFile,
                               &sQueryStr,
                               &sIsSQLEnd, 
                               &sIsEOF ) != ACE_RC_SUCCESS );
            
        if ( sIsSQLEnd == ACP_TRUE )
        {
            ACE_TEST( executeSQL( aContext,
                                  aHandle,
                                  &sQueryStr ) != ACE_RC_SUCCESS );
        }
        else
        {   
            /* Nothing to do */
        }
    }
    
    ACP_STR_FINAL( sQueryStr );
    
    return ACE_RC_SUCCESS;

    ACE_EXCEPTION_END;

    ACP_STR_FINAL( sQueryStr );
    
    return ACE_RC_FAILURE;
}

/*
 *
 */

ace_rc_t openFile( oaContext          * aContext,
                   acp_char_t         * aFileName,
                   acp_std_file_t     * aFile,
                   acp_bool_t         * aIsExistFile )
{
    acp_rc_t            sAcpRC = ACP_RC_EEXIST;
    acp_char_t          sErrMsg[1024];
    acp_char_t        * sHome = NULL;
    acp_stat_t          sStat;
    acp_bool_t          sIsExistFile = ACP_FALSE;
    acp_std_file_t      sFile = {NULL};

    ACP_STR_DECLARE_DYNAMIC( sFilePath );
    ACP_STR_INIT_DYNAMIC( sFilePath, 128, 128 );

    sAcpRC = acpEnvGet( OA_ADAPTER_HOME_ENVIRONMENT_VARIABLE, &sHome );
    if ( ACP_RC_NOT_ENOENT(sAcpRC) && (sAcpRC == ACP_RC_SUCCESS) )
    {
        (void)acpStrCpyFormat( &sFilePath, "%s/conf/", sHome );
    }
    else
    {
        (void)acpStrCpyCString( &sFilePath, "./conf/" );
    }

    (void)acpStrCatCString( &sFilePath, aFileName );

    sAcpRC = acpFileStatAtPath( (char *)acpStrGetBuffer( &sFilePath ),
                                &sStat,
                                ACP_TRUE);
    if ( ACP_RC_NOT_ENOENT( sAcpRC ) )
    {
        sAcpRC = acpStdOpen( &sFile,
                             (char *)acpStrGetBuffer( &sFilePath ),
                             ACP_STD_OPEN_READ_TEXT );
        ACE_TEST_RAISE( ACP_RC_NOT_SUCCESS( sAcpRC ), ERROR_FILE_OPEN );
        sIsExistFile = ACP_TRUE;
    }
    else
    {
        /* Nothing to do */
    }
    ACP_STR_FINAL( sFilePath );

    *aIsExistFile = sIsExistFile;
    *aFile = sFile;

    return ACE_RC_SUCCESS;

    ACE_EXCEPTION( ERROR_FILE_OPEN )
    {
        acpErrorString( sAcpRC, sErrMsg, 1024 );
        oaLogMessage( OAM_ERR_FILE_OPEN, aFileName, sErrMsg );
    }
    ACE_EXCEPTION_END;

    ACP_STR_FINAL( sFilePath );

    return ACE_RC_FAILURE;
}

void closeFile( acp_std_file_t * aFile )
{
    (void)acpStdClose( aFile );
}

/*
 *
 */
ace_rc_t executeLogInSql( oaContext          * aContext, 
                          oaOciApplierHandle * aHandle )
{
    acp_std_file_t      sFile = {NULL};
    acp_bool_t          sIsExistFile = ACP_FALSE;
    acp_bool_t          sIsOpenFile = ACP_FALSE;
    
    ACE_TEST( openFile( aContext,
                        OA_ADAPTER_LOGIN_SQL_FILE_NAME,
                        &sFile,
                        &sIsExistFile ) != ACE_RC_SUCCESS );
    if ( sIsExistFile == ACP_TRUE )
    {
        sIsOpenFile = ACP_TRUE;
        
        ACE_TEST( executeLogInSqlFile( aContext, 
                                       aHandle, 
                                       &sFile ) != ACE_RC_SUCCESS );
        sIsOpenFile = ACP_FALSE;
        closeFile( &sFile );
    }
    else
    {
        /* Nothing to do */
    }
    
    return ACE_RC_SUCCESS;

    ACE_EXCEPTION_END;
    
    if ( sIsOpenFile == ACP_TRUE )
    {
        sIsOpenFile = ACP_FALSE;
        closeFile( &sFile );
    }
    else
    {
        /* Nothing to do */
    }
    
    return ACE_RC_FAILURE;
}

/*
 *
 */
ace_rc_t oaOciApplierLogIn(oaContext *aContext,
                           oaOciApplierHandle *aHandle)
{
    acp_sint32_t sStage = 0;
    ub4          sStatementCacheSize = aHandle->mUpdateStatementCacheSize;

    if (acpStrCmpCString(aHandle->mServerAlias, 
                         "", ACP_STR_CASE_SENSITIVE) == 0)
    {
        ACE_TEST_RAISE(OCIServerAttach(aHandle->mServer, aHandle->mError,
                                       (text *)0,
                                       (sb4)0,
                                       (ub4)OCI_DEFAULT),
                       ERROR_ATTACH_SERVER);
    }
    else
    {
        ACE_TEST_RAISE(OCIServerAttach(aHandle->mServer, aHandle->mError,
                                       (text *)acpStrGetBuffer(aHandle->mServerAlias), 
                                       (sb4)acpStrGetLength(aHandle->mServerAlias), 
                                       (ub4)OCI_DEFAULT),
                       ERROR_ATTACH_SERVER);
    }
    sStage = 1;
    
    /* Set the server handle in the service handle */
    ACE_TEST_RAISE(OCIAttrSet((dvoid *)aHandle->mSvcCtx,
                              (ub4)OCI_HTYPE_SVCCTX,
                              (dvoid *)aHandle->mServer, (ub4)0,
                              (ub4)OCI_ATTR_SERVER, 
                              aHandle->mError),
                   ERROR_SET_ATTR_SERVICE_HANDLE);

    /* Set attributes in the authentication handle */
    ACE_TEST_RAISE(OCIAttrSet((dvoid *)aHandle->mSession,
                              (ub4)OCI_HTYPE_SESSION,
                              (dvoid *)acpStrGetBuffer(aHandle->mUser),
                              (ub4)acpStrGetLength(aHandle->mUser),
                              (ub4)OCI_ATTR_USERNAME,
                              aHandle->mError),
                   ERROR_SET_ATTR_USER_NAME);

    ACE_TEST_RAISE(OCIAttrSet((dvoid *)aHandle->mSession,
                              (ub4)OCI_HTYPE_SESSION,
                              (dvoid *)acpStrGetBuffer(aHandle->mPassword),
                              (ub4)acpStrGetLength(aHandle->mPassword),
                              (ub4)OCI_ATTR_PASSWORD,
                              aHandle->mError),
                   ERROR_SET_ATTR_PASSWORD);

    ACE_TEST_RAISE(OCISessionBegin(aHandle->mSvcCtx,
                                   aHandle->mError,
                                   aHandle->mSession,
                                   (ub4)OCI_CRED_RDBMS,
                                   (ub4)OCI_STMT_CACHE),
                   ERROR_BEGIN_SESSION);
    sStage = 2;

    /* Set the authentication handle in the Service handle */
    ACE_TEST_RAISE(OCIAttrSet((dvoid *)aHandle->mSvcCtx,
                              (ub4)OCI_HTYPE_SVCCTX,
                              (dvoid *)aHandle->mSession, (ub4)0,
                              (ub4)OCI_ATTR_SESSION,
                              aHandle->mError),
                   ERROR_SET_ATTR_SESSION);

    /* If statement cache size is zero, statement cache does not be applied. */
    ACE_TEST_RAISE( OCIAttrSet( (dvoid *)aHandle->mSvcCtx,
                                (ub4)OCI_HTYPE_SVCCTX,
                                (dvoid *)&sStatementCacheSize, (ub4)0,
                                (ub4)OCI_ATTR_STMTCACHESIZE,
                                aHandle->mError ),
                    ERROR_SET_ATTR_STMT_CACHE_SIZE );
    
    ACE_TEST( executeLogInSql( aContext, aHandle ) != ACE_RC_SUCCESS );
    
    /* Initialize group commits */
    aHandle->mDMLExecutedFlag = ACP_FALSE;

    return ACE_RC_SUCCESS;

    ACE_EXCEPTION(ERROR_ATTACH_SERVER)
    {
        writeOciErrorMessageToLogFile(aHandle);
    }
    ACE_EXCEPTION(ERROR_SET_ATTR_SERVICE_HANDLE)
    {
        writeOciErrorMessageToLogFile(aHandle);
    }
    ACE_EXCEPTION(ERROR_SET_ATTR_USER_NAME)
    {
        writeOciErrorMessageToLogFile(aHandle);
    }
    ACE_EXCEPTION(ERROR_SET_ATTR_PASSWORD)
    {
        writeOciErrorMessageToLogFile(aHandle);
    }
    ACE_EXCEPTION(ERROR_BEGIN_SESSION)
    {
        writeOciErrorMessageToLogFile(aHandle);
    }
    ACE_EXCEPTION(ERROR_SET_ATTR_SESSION)
    {
        writeOciErrorMessageToLogFile(aHandle);
    }
    ACE_EXCEPTION( ERROR_SET_ATTR_STMT_CACHE_SIZE )
    {
        writeOciErrorMessageToLogFile( aHandle );
    }
    ACE_EXCEPTION_END;

    switch (sStage)
    {
        case 2:
            (void)OCISessionEnd(aHandle->mSvcCtx, aHandle->mError,
                                aHandle->mSession, (ub4)0);
        case 1:
            (void)OCIServerDetach(aHandle->mServer,
                                  aHandle->mError, (ub4)OCI_DEFAULT);
        default:
            break;
    }

    return ACE_RC_FAILURE;
}

/*
 *
 */
void oaOciApplierLogOut(oaOciApplierHandle *aHandle)
{
    (void)OCISessionEnd(aHandle->mSvcCtx, aHandle->mError,
                        aHandle->mSession, (ub4)0);

    (void)OCIServerDetach(aHandle->mServer, aHandle->mError, (ub4)OCI_DEFAULT);
}

static ace_rc_t applyCommitLogRecord(oaContext *aContext,
                                     oaOciApplierHandle *aHandle)
{
    if ( ( aHandle->mDMLExecutedFlag == ACP_TRUE ) &&
         ( aHandle->mCommitMode == OCI_DEFAULT ) )
    {
        if ( aHandle->mIsAsynchronousCommit == ACP_TRUE )
        {
            ACE_TEST_RAISE( OCITransCommit( aHandle->mSvcCtx,
                                            aHandle->mError,
                                            (ub4)(OCI_TRANS_WRITENOWAIT |
                                                  OCI_TRANS_WRITEBATCH) ),
                            ERROR_COMMIT_TRANS );
        }
        else
        {
            ACE_TEST_RAISE( OCITransCommit( aHandle->mSvcCtx,
                                            aHandle->mError,
                                            (ub4)OCI_DEFAULT ),
                            ERROR_COMMIT_TRANS );
        }

        aHandle->mDMLExecutedFlag = ACP_FALSE;
    }
    else
    {
        /* Nothing to do */
    }

    return ACE_RC_SUCCESS;

    ACE_EXCEPTION(ERROR_COMMIT_TRANS)
    {
        writeOciDCLLog( aHandle ); 
    }
    ACE_EXCEPTION_END;

    return ACE_RC_FAILURE;
}

static ace_rc_t applyAbortLogRecord(oaContext *aContext,
                                    oaOciApplierHandle *aHandle)
{
    ACE_TEST_RAISE(OCITransRollback(aHandle->mSvcCtx, aHandle->mError, (ub4)OCI_DEFAULT),
                   ERROR_ROLLBACK_TRANS);
    aHandle->mDMLExecutedFlag = ACP_FALSE;

    return ACE_RC_SUCCESS;

    ACE_EXCEPTION(ERROR_ROLLBACK_TRANS)
    {
        writeOciDCLLog( aHandle ); 
    }
    ACE_EXCEPTION_END;

    return ACE_RC_FAILURE;
}


static ace_rc_t prepareInsertStatement(oaContext *aContext,
                                       oaOciApplierHandle *aHandle,
                                       oaLogRecordInsert *aLogRecord,
                                       OCIStmt **aStatement)
{
    OCIStmt *sStatement = NULL;
    acp_sint32_t sStage = 0;

    ACP_STR_DECLARE_DYNAMIC(sSqlQuery);

    ACP_STR_INIT_DYNAMIC(sSqlQuery, 4096, 4096);

    ACE_TEST_RAISE(OCIHandleAlloc((dvoid *)aHandle->mEnv,
                                  (dvoid **)&sStatement,
                                  (ub4)OCI_HTYPE_STMT, 0, (dvoid **)0),
                   ERROR_ALLOC_STATEMENT_HANDLE);
    sStage = 1;

    prepareInsertQuery( aLogRecord, 
                        &sSqlQuery, 
                        aHandle->mIsDirectPathInsert,
                        aHandle->mSetUserToTable,
                        aHandle->mSetColumnToInsert );

    ACE_TEST_RAISE(OCIStmtPrepare(sStatement,
                                  aHandle->mError, 
                                  (unsigned char *)acpStrGetBuffer(&sSqlQuery),
                                  (ub4)acpStrGetLength(&sSqlQuery),
                                  (ub4)OCI_NTV_SYNTAX,
                                  (ub4)OCI_DEFAULT),
                   ERROR_PREPARE_STATEMENT);

    ACP_STR_FINAL(sSqlQuery);

    *aStatement = sStatement;

    return ACE_RC_SUCCESS;

    ACE_EXCEPTION(ERROR_ALLOC_STATEMENT_HANDLE)
    {
        writeOciErrorMessageToLogFile(aHandle);
    }
    ACE_EXCEPTION(ERROR_PREPARE_STATEMENT)
    {
        writeOciErrorMessageToLogFile(aHandle);
    }
    ACE_EXCEPTION_END;

    switch (sStage)
    {
        case 1:
            (void)OCIHandleFree((dvoid *)sStatement, (ub4)OCI_HTYPE_STMT);
        default:
            break;
    }
    ACP_STR_FINAL(sSqlQuery);

    return ACE_RC_FAILURE;
}

static ace_rc_t getPreparedInsertStatement(oaContext *aContext,
                                           oaOciApplierHandle *aHandle,
                                           oaLogRecordInsert *aLogRecord,
                                           OCIStmt **aStatement)
{
    ace_rc_t sAceRC = ACE_RC_SUCCESS;
    OCIStmt *sStatement = NULL;

    if (aHandle->mPreparedStatement[aLogRecord->mTableId].mInsert == NULL)
    {
        sAceRC = prepareInsertStatement(aContext, aHandle, 
                                        aLogRecord, 
                                        &sStatement);
        ACE_TEST(sAceRC != ACE_RC_SUCCESS);

        aHandle->mPreparedStatement[aLogRecord->mTableId].mInsert = sStatement;
    }
    else
    {
        sStatement = aHandle->mPreparedStatement[aLogRecord->mTableId].mInsert;
    }

    *aStatement = sStatement;

    return ACE_RC_SUCCESS;

    ACE_EXCEPTION_END;

    return ACE_RC_FAILURE;
}

static ub2 getOciDataType(oaLogRecordValueType aLogRecordValueType)
{
    ub2 sOciDataType = SQLT_STR;

    switch (aLogRecordValueType)
    {
        case OA_LOG_RECORD_VALUE_TYPE_CHAR:
            sOciDataType = SQLT_AFC;
            break;

        case OA_LOG_RECORD_VALUE_TYPE_VARCHAR:
            sOciDataType = SQLT_CHR;
            break;
            
        default:
            sOciDataType = SQLT_STR;
            break;
    }

    return sOciDataType;
}

static ace_rc_t bindLogRecordColumn( oaContext          * aContext,
                                     oaOciApplierHandle * aHandle,
                                     acp_uint32_t         aPosition,
                                     oaLogRecordColumn  * aLogRecordColumn,
                                     OCIStmt            * aStatement )
{
    OCIBind *sBindHandle = NULL;

    ub2 sOciDataType = 0;
    ub1 sCharsetForm = SQLCS_NCHAR;
    ub2 sCharsetId = OCI_UTF16ID;

    sOciDataType = getOciDataType(aLogRecordColumn->mType);

    ACE_TEST_RAISE(OCIBindByPos(aStatement,
                                &sBindHandle,
                                aHandle->mError,
                                (ub4)aPosition,
                                (dvoid *)aLogRecordColumn->mValue,
                                (sb4)aLogRecordColumn->mMaxLength,  /* value_sz */
                                sOciDataType,
                                (dvoid *)0,
                                (ub2 *)aLogRecordColumn->mLength,   /* alenp */
                                (ub2 *)0,
                                (ub4)0, (ub4 *)0, OCI_DEFAULT),
                   ERROR_BIND);

    switch (aLogRecordColumn->mType)
    {
        case OA_LOG_RECORD_VALUE_TYPE_NCHAR:
        case OA_LOG_RECORD_VALUE_TYPE_NVARCHAR:
            ACE_TEST_RAISE(OCIAttrSet((void *)sBindHandle,
                                      (ub4)OCI_HTYPE_BIND,
                                      (void *)&sCharsetForm,
                                      (ub4)0,
                                      (ub4)OCI_ATTR_CHARSET_FORM,
                                      aHandle->mError),
                           ERROR_SET_ATTR_CHARSET);

            ACE_TEST_RAISE(OCIAttrSet((void *)sBindHandle,
                                      (ub4)OCI_HTYPE_BIND,
                                      (void *)&sCharsetId,
                                      (ub4)0,
                                      (ub4)OCI_ATTR_CHARSET_ID,
                                      aHandle->mError),
                           ERROR_SET_ATTR_CHARSET_ID);
            break;

        default:
            break;
    }
    
    return ACE_RC_SUCCESS;

    ACE_EXCEPTION(ERROR_BIND)
    {
        writeOciErrorMessageToLogFile(aHandle);
    }
    ACE_EXCEPTION(ERROR_SET_ATTR_CHARSET)
    {
        writeOciErrorMessageToLogFile(aHandle);
    }
    ACE_EXCEPTION(ERROR_SET_ATTR_CHARSET_ID)
    {
        writeOciErrorMessageToLogFile(aHandle);
    }
    ACE_EXCEPTION_END;

    return ACE_RC_FAILURE;
}

static ace_rc_t bindInsertStatement(oaContext *aContext,
                                    oaOciApplierHandle *aHandle,
                                    oaLogRecordInsert *aLogRecord,
                                    OCIStmt *aStatement)
{
    ace_rc_t     sAceRC          = ACE_RC_SUCCESS;
    acp_sint32_t i               = 0;
    acp_bool_t   sIsHiddenColumn = ACP_FALSE;
    acp_uint32_t sBindIndex      = 0;

    for (i = 0; i < aLogRecord->mColumnCount; i++)
    {
        sIsHiddenColumn = oaLogRecordColumnIsHiddenColumn(
                                   &(aLogRecord->mColumn[i]) );
        if ( sIsHiddenColumn == ACP_FALSE )
        {
            sBindIndex++;
            sAceRC = bindLogRecordColumn(aContext, aHandle, sBindIndex,
                                         &(aLogRecord->mColumn[i]),
                                         aStatement);
            ACE_TEST(sAceRC != ACE_RC_SUCCESS);
        }
        else
        {
            /* do nothing */
        }
    }

    return ACE_RC_SUCCESS;

    ACE_EXCEPTION_END;

    return ACE_RC_FAILURE;
}

static ace_rc_t applyInsertLogRecord(oaContext *aContext,
                                     oaOciApplierHandle *aHandle,
                                     oaLogRecordInsert *aLogRecord)
{
    ace_rc_t       sAceRC = ACE_RC_SUCCESS;
    OCIStmt      * sStatement = NULL;
    acp_uint32_t   sExecuteMode;

    sAceRC = getPreparedInsertStatement(aContext, aHandle, 
                                        aLogRecord, &sStatement);
    ACE_TEST(sAceRC != ACE_RC_SUCCESS);

    sAceRC = bindInsertStatement(aContext, aHandle, aLogRecord, sStatement);
    ACE_TEST(sAceRC != ACE_RC_SUCCESS);

    if ( aLogRecord->mArrayDMLCount == 1 )
    {
        sExecuteMode = aHandle->mCommitMode;
    }
    else
    {
        sExecuteMode = aHandle->mCommitMode | OCI_BATCH_ERRORS;
    }

    aHandle->mDMLExecutedFlag = ACP_TRUE;
    ACE_TEST_RAISE(OCIStmtExecute(aHandle->mSvcCtx,
                                  sStatement,
                                  aHandle->mError,
                                  (ub4)aLogRecord->mArrayDMLCount,  /* iters */
                                  (ub4)0,                           /* rowoff */
                                  (CONST OCISnapshot *)0, (OCISnapshot *)0,
                                  (ub4)sExecuteMode),
                   ERROR_EXECUTE_STATEMENT);

    return ACE_RC_SUCCESS;

    ACE_EXCEPTION(ERROR_EXECUTE_STATEMENT)
    {
        writeOciDMLLog( aContext,
                        aHandle,
                        (oaLogRecord *)aLogRecord,
                        sStatement );
    }
    ACE_EXCEPTION_END;

    return ACE_RC_FAILURE;
}

static ace_rc_t prepareUpdateStatement( oaContext          * aContext,
                                        oaOciApplierHandle * aHandle,
                                        oaLogRecordUpdate  * aLogRecord,
                                        OCIStmt            * aStatement )
{
    ACP_STR_DECLARE_DYNAMIC(sSqlQuery);

    ACP_STR_INIT_DYNAMIC(sSqlQuery, 4096, 4096);

    prepareUpdateQuery( aLogRecord,
                        &sSqlQuery,
                        aHandle->mSetUserToTable );

    ACE_TEST_RAISE(OCIStmtPrepare(aStatement,
                                  aHandle->mError, 
                                  (unsigned char *)acpStrGetBuffer(&sSqlQuery),
                                  (ub4)acpStrGetLength(&sSqlQuery),
                                  (ub4)OCI_NTV_SYNTAX,
                                  (ub4)OCI_DEFAULT),
                   ERROR_PREPARE_STATEMENT);

    ACP_STR_FINAL(sSqlQuery);

    return ACE_RC_SUCCESS;

    ACE_EXCEPTION(ERROR_PREPARE_STATEMENT)
    {
        writeOciErrorMessageToLogFile(aHandle);
    }
    ACE_EXCEPTION_END;

    ACP_STR_FINAL(sSqlQuery);

    return ACE_RC_FAILURE;
}

/**
 * @breif  Cache에서 Prepared Update Statement를 얻는다.
 *
 *         이후에 반드시 putPreparedUpdateStatementToCache()를 호출해야 한다.
 *
 * @param  aContext   Context
 * @param  aHandle    Oracle Applier Handle
 * @param  aLogRecord Update Log Record
 * @param  aStatement OCI Statement
 *
 * @return 성공/실패
 */
static ace_rc_t getPreparedUpdateStatementFromCache( oaContext           * aContext,
                                                     oaOciApplierHandle  * aHandle,
                                                     oaLogRecordUpdate   * aLogRecord,
                                                     OCIStmt            ** aStatement )
{
    OCIStmt * sStatement = NULL;

    ACP_STR_DECLARE_DYNAMIC( sSqlQuery );

    ACP_STR_INIT_DYNAMIC( sSqlQuery, 4096, 4096 );

    prepareUpdateQuery( aLogRecord,
                        &sSqlQuery,
                        aHandle->mSetUserToTable );

    ACE_TEST_RAISE( OCIStmtPrepare2( aHandle->mSvcCtx, /* Service Context */
                                     &sStatement,
                                     aHandle->mError,
                                     (CONST OraText *)acpStrGetBuffer( &sSqlQuery ),
                                     (ub4)acpStrGetLength( &sSqlQuery ),
                                     (CONST OraText *)NULL,
                                     (ub4)0,
                                     (ub4)OCI_NTV_SYNTAX,
                                     (ub4)OCI_DEFAULT ),
                    ERROR_PREPARE_STATEMENT );

    ACP_STR_FINAL( sSqlQuery );

    *aStatement = sStatement;

    return ACE_RC_SUCCESS;

    ACE_EXCEPTION( ERROR_PREPARE_STATEMENT )
    {
        writeOciErrorMessageToLogFile( aHandle );
    }
    ACE_EXCEPTION_END;

    ACP_STR_FINAL( sSqlQuery );

    return ACE_RC_FAILURE;
}

/**
 * @breif  Prepare한 Update Statement를 얻는다.
 *
 *         이후에 반드시 putPreparedUpdateStatement()를 호출해야 한다.
 *
 * @param  aContext                Context
 * @param  aHandle                 Oracle Applier Handle
 * @param  aLogRecord              Update Log Record
 * @param  aStatement              OCI Statement
 * @param  aStatementFromCacheFlag OCI Statement From Cache?
 *
 * @return 성공/실패
 */
static ace_rc_t getPreparedUpdateStatement( oaContext           * aContext,
                                            oaOciApplierHandle  * aHandle,
                                            oaLogRecordUpdate   * aLogRecord,
                                            OCIStmt            ** aStatement,
                                            acp_bool_t          * aStatementFromCacheFlag )
{
    OCIStmt    * sStatement = NULL;
    acp_bool_t   sStatementFromCacheFlag = ACP_FALSE;

    if ( aHandle->mUpdateStatementCacheSize != 0 )
    {
        ACE_TEST( getPreparedUpdateStatementFromCache( aContext,
                                                       aHandle,
                                                       aLogRecord,
                                                       &sStatement )
                  != ACE_RC_SUCCESS );
        sStatementFromCacheFlag = ACP_TRUE;
    }
    else
    {
        if ( aHandle->mPreparedStatement[aLogRecord->mTableId].mUpdate == NULL )
        {
            ACE_TEST_RAISE( OCIHandleAlloc( (dvoid *)aHandle->mEnv,
                                            (dvoid **)&sStatement,
                                            (ub4)OCI_HTYPE_STMT, 0, (dvoid **)0 ),
                            ERROR_ALLOC_STATEMENT_HANDLE );

            aHandle->mPreparedStatement[aLogRecord->mTableId].mUpdate = sStatement;
        }
        else
        {
            sStatement = aHandle->mPreparedStatement[aLogRecord->mTableId].mUpdate;
        }

        ACE_TEST( prepareUpdateStatement( aContext,
                                          aHandle,
                                          aLogRecord,
                                          sStatement )
                  != ACE_RC_SUCCESS );
    }

    *aStatement = sStatement;
    *aStatementFromCacheFlag = sStatementFromCacheFlag;

    return ACE_RC_SUCCESS;

    ACE_EXCEPTION( ERROR_ALLOC_STATEMENT_HANDLE )
    {
        writeOciErrorMessageToLogFile( aHandle );
    }
    ACE_EXCEPTION_END;

    return ACE_RC_FAILURE;
}

/**
 * @breif  사용한 Prepared Update Statement를 Cache에 넣는다.
 *
 *         getPreparedUpdateStatementFromCache()와 짝을 이룬다.
 *
 * @param  aContext   Context
 * @param  aHandle    Oracle Applier Handle
 * @param  aStatement OCI Statement
 *
 * @return 성공/실패
 */
static ace_rc_t putPreparedUpdateStatementToCache( oaContext          * aContext,
                                                   oaOciApplierHandle * aHandle,
                                                   OCIStmt            * aStatement )
{
    ACE_TEST_RAISE( OCIStmtRelease( aStatement,
                                    aHandle->mError,
                                    (CONST OraText *)NULL,
                                    (ub4)0,
                                    (ub4)OCI_DEFAULT ),
                    ERROR_RELEASE_STATEMENT );

    return ACE_RC_SUCCESS;

    ACE_EXCEPTION( ERROR_RELEASE_STATEMENT )
    {
        writeOciErrorMessageToLogFile( aHandle );
    }
    ACE_EXCEPTION_END;

    return ACE_RC_FAILURE;
}

/**
 * @breif  사용한 Prepared Update Statement를 처리한다.
 *
 *         getPreparedUpdateStatement()와 짝을 이룬다.
 *
 * @param  aContext                Context
 * @param  aHandle                 Oracle Applier Handle
 * @param  aStatement              OCI Statement
 * @param  aStatementFromCacheFlag OCI Statement From Cache?
 *
 * @return 성공/실패
 */
static ace_rc_t putPreparedUpdateStatement( oaContext          * aContext,
                                            oaOciApplierHandle * aHandle,
                                            OCIStmt            * aStatement,
                                            acp_bool_t           aStatementFromCacheFlag )
{
    if ( aStatementFromCacheFlag == ACP_TRUE )
    {
        ACE_TEST( putPreparedUpdateStatementToCache( aContext,
                                                     aHandle,
                                                     aStatement )
                  != ACE_RC_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    return ACE_RC_SUCCESS;

    ACE_EXCEPTION_END;

    return ACE_RC_FAILURE;
}

static ace_rc_t bindUpdateStatement(oaContext *aContext,
                                    oaOciApplierHandle *aHandle,
                                    oaLogRecordUpdate *aLogRecord,
                                    OCIStmt *aStatement)
{
    ace_rc_t sAceRC = ACE_RC_SUCCESS;

    acp_sint32_t i               = 0;
    acp_sint32_t j               = 0;
    acp_uint32_t sColumnID       = 0;
    acp_bool_t   sIsHiddenColumn = ACP_FALSE;
    acp_uint32_t sBindIndex      = 0;

    for (i = 0; i < aLogRecord->mColumnCount; i++)
    {
        sColumnID = aLogRecord->mColumnIDMap[i];

        sIsHiddenColumn = oaLogRecordColumnIsHiddenColumn(
                                &(aLogRecord->mColumn[sColumnID]) );
        if ( sIsHiddenColumn == ACP_FALSE )
        {
            sBindIndex++;
            sAceRC = bindLogRecordColumn(aContext, aHandle, sBindIndex,
                                         &(aLogRecord->mColumn[sColumnID]),
                                         aStatement);
            ACE_TEST(sAceRC != ACE_RC_SUCCESS);
        }
        else
        {
            /* do nothing */
        }
    }

    for (j = 0; j < aLogRecord->mPrimaryKeyCount; j++)
    {
        sIsHiddenColumn = oaLogRecordColumnIsHiddenColumn(
                                &(aLogRecord->mColumn[j]) );
        if ( sIsHiddenColumn == ACP_FALSE )
        {
            sBindIndex++;
            sAceRC = bindLogRecordColumn(aContext, aHandle, sBindIndex,
                                         &(aLogRecord->mPrimaryKey[j]),
                                         aStatement);
            ACE_TEST(sAceRC != ACE_RC_SUCCESS);
        }
        else
        {
            /* do nothing */
        }

    }

    return ACE_RC_SUCCESS;

    ACE_EXCEPTION_END;

    return ACE_RC_FAILURE;
}

static ace_rc_t applyUpdateLogRecord(oaContext *aContext,
                                     oaOciApplierHandle *aHandle,
                                     oaLogRecordUpdate *aLogRecord)
{
    ace_rc_t       sAceRC = ACE_RC_SUCCESS;
    OCIStmt      * sStatement = NULL;
    acp_uint32_t   sExecuteMode;
    acp_bool_t     sStatementFromCacheFlag = ACP_FALSE;
    acp_bool_t     sStatementGetFlag = ACP_FALSE;

    ACE_TEST( getPreparedUpdateStatement( aContext,
                                          aHandle,
                                          aLogRecord,
                                          &sStatement,
                                          &sStatementFromCacheFlag )
              != ACE_RC_SUCCESS );
    sStatementGetFlag = ACP_TRUE;

    sAceRC = bindUpdateStatement(aContext, aHandle, aLogRecord, sStatement);
    ACE_TEST(sAceRC != ACE_RC_SUCCESS);

    if ( aLogRecord->mArrayDMLCount == 1 )
    {
        sExecuteMode = aHandle->mCommitMode;
    }
    else
    {
        sExecuteMode = aHandle->mCommitMode | OCI_BATCH_ERRORS;
    }

    aHandle->mDMLExecutedFlag = ACP_TRUE;
    ACE_TEST_RAISE(OCIStmtExecute(aHandle->mSvcCtx,
                                  sStatement,
                                  aHandle->mError,
                                  (ub4)aLogRecord->mArrayDMLCount,  /* iters */
                                  (ub4)0,                           /* rowoff */
                                  (CONST OCISnapshot *)0, (OCISnapshot *)0,
                                  (ub4)sExecuteMode),
                   ERROR_EXECUTE_STATEMENT);

    sStatementGetFlag = ACP_FALSE;
    ACE_TEST( putPreparedUpdateStatement( aContext,
                                          aHandle,
                                          sStatement,
                                          sStatementFromCacheFlag )
              != ACE_RC_SUCCESS );

    return ACE_RC_SUCCESS;

    ACE_EXCEPTION(ERROR_EXECUTE_STATEMENT)
    {
         writeOciDMLLog( aContext,
                         aHandle, 
                         (oaLogRecord*)aLogRecord,
                         sStatement );
       
    }
    ACE_EXCEPTION_END;

    if ( sStatementGetFlag == ACP_TRUE )
    {
        (void)putPreparedUpdateStatement( aContext,
                                          aHandle,
                                          sStatement,
                                          sStatementFromCacheFlag );
    }
    else
    {
        /* Nothing to do */
    }

    return ACE_RC_FAILURE;
}

static ace_rc_t prepareDeleteStatement(oaContext *aContext,
                                       oaOciApplierHandle *aHandle,
                                       oaLogRecordDelete *aLogRecord,
                                       OCIStmt **aStatement)
{
    OCIStmt *sStatement = NULL;
    acp_sint32_t sStage = 0;

    ACP_STR_DECLARE_DYNAMIC(sSqlQuery);

    ACP_STR_INIT_DYNAMIC(sSqlQuery, 4096, 4096);

    ACE_TEST_RAISE(OCIHandleAlloc((dvoid *)aHandle->mEnv,
                                  (dvoid **)&sStatement,
                                  (ub4)OCI_HTYPE_STMT, 0, (dvoid **)0),
                   ERROR_ALLOC_STATEMENT_HANDLE);
    sStage = 1;

    prepareDeleteQuery( aLogRecord,
                        &sSqlQuery,
                        aHandle->mSetUserToTable );

    ACE_TEST_RAISE(OCIStmtPrepare(sStatement,
                                  aHandle->mError, 
                                  (unsigned char *)acpStrGetBuffer(&sSqlQuery),
                                  (ub4)acpStrGetLength(&sSqlQuery),
                                  (ub4)OCI_NTV_SYNTAX,
                                  (ub4)OCI_DEFAULT),
                   ERROR_PREPARE_STATEMENT);

    ACP_STR_FINAL(sSqlQuery);

    *aStatement = sStatement;

    return ACE_RC_SUCCESS;

    ACE_EXCEPTION(ERROR_ALLOC_STATEMENT_HANDLE)
    {
        writeOciErrorMessageToLogFile(aHandle);
    }
    ACE_EXCEPTION(ERROR_PREPARE_STATEMENT)
    {
        writeOciErrorMessageToLogFile(aHandle);
    }
    ACE_EXCEPTION_END;

    switch (sStage)
    {
        case 1:
            (void)OCIHandleFree((dvoid *)sStatement, (ub4)OCI_HTYPE_STMT);
        default:
            break;
    }
    ACP_STR_FINAL(sSqlQuery);

    return ACE_RC_FAILURE;
}

static ace_rc_t getPreparedDeleteStatement(oaContext *aContext,
                                           oaOciApplierHandle *aHandle,
                                           oaLogRecordDelete *aLogRecord,
                                           OCIStmt **aStatement)
{
    ace_rc_t sAceRC = ACE_RC_SUCCESS;
    OCIStmt *sStatement = NULL;

    if (aHandle->mPreparedStatement[aLogRecord->mTableId].mDelete == NULL)
    {
        sAceRC = prepareDeleteStatement(aContext, aHandle, 
                                        aLogRecord, 
                                        &sStatement);
        ACE_TEST(sAceRC != ACE_RC_SUCCESS);

        aHandle->mPreparedStatement[aLogRecord->mTableId].mDelete = sStatement;
    }
    else
    {
        sStatement = aHandle->mPreparedStatement[aLogRecord->mTableId].mDelete;
    }

    *aStatement = sStatement;

    return ACE_RC_SUCCESS;

    ACE_EXCEPTION_END;

    return ACE_RC_FAILURE;
}

static ace_rc_t bindDeleteStatement(oaContext *aContext,
                                    oaOciApplierHandle *aHandle,
                                    oaLogRecordDelete *aLogRecord,
                                    OCIStmt *aStatement)
{
    ace_rc_t     sAceRC          = ACE_RC_SUCCESS;
    acp_sint32_t i               = 0;
    acp_bool_t   sIsHiddenColumn = ACP_FALSE;
    acp_uint32_t sBindIndex      = 0;

    for (i = 0; i < aLogRecord->mPrimaryKeyCount; i++)
    {
        sIsHiddenColumn = oaLogRecordColumnIsHiddenColumn(
                                   &(aLogRecord->mPrimaryKey[i]) );
        if ( sIsHiddenColumn == ACP_FALSE )
        {
            sBindIndex++;
            sAceRC = bindLogRecordColumn(aContext, aHandle, sBindIndex,
                                         &(aLogRecord->mPrimaryKey[i]),
                                         aStatement);
            ACE_TEST(sAceRC != ACE_RC_SUCCESS);
        }
        else
        {
            /* do nothing */
        }
    }

    return ACE_RC_SUCCESS;

    ACE_EXCEPTION_END;

    return ACE_RC_FAILURE;
}

static ace_rc_t applyDeleteLogRecord(oaContext *aContext,
                                     oaOciApplierHandle *aHandle,
                                     oaLogRecordDelete *aLogRecord)
{
    ace_rc_t       sAceRC = ACE_RC_SUCCESS;
    OCIStmt      * sStatement = NULL;
    acp_uint32_t   sExecuteMode;

    sAceRC = getPreparedDeleteStatement(aContext, aHandle,
                                        aLogRecord, &sStatement);
    ACE_TEST(sAceRC != ACE_RC_SUCCESS);

    sAceRC = bindDeleteStatement(aContext, aHandle, aLogRecord, sStatement);
    ACE_TEST(sAceRC != ACE_RC_SUCCESS);

    if ( aLogRecord->mArrayDMLCount == 1 )
    {
        sExecuteMode = aHandle->mCommitMode;
    }
    else
    {
        sExecuteMode = aHandle->mCommitMode | OCI_BATCH_ERRORS;
    }

    aHandle->mDMLExecutedFlag = ACP_TRUE;
    ACE_TEST_RAISE(OCIStmtExecute(aHandle->mSvcCtx,
                                  sStatement,
                                  aHandle->mError,
                                  (ub4)aLogRecord->mArrayDMLCount,  /* iters */
                                  (ub4)0,                           /* rowoff */
                                  (CONST OCISnapshot *)0, (OCISnapshot *)0,
                                  (ub4)sExecuteMode),
                   ERROR_EXECUTE_STATEMENT);

    return ACE_RC_SUCCESS;

    ACE_EXCEPTION(ERROR_EXECUTE_STATEMENT)
    {
        writeOciDMLLog( aContext,
                        aHandle,
                        (oaLogRecord *)aLogRecord,
                        sStatement );
     
    }
    ACE_EXCEPTION_END;

    return ACE_RC_FAILURE;
}

ACP_INLINE ace_rc_t applyInsertLogRecordWithSkipProperty( oaContext          * aContext,
                                                          oaOciApplierHandle * aHandle,
                                                          oaLogRecordInsert  * aLogRecord )
{
    ace_rc_t sAceRC = ACE_RC_SUCCESS;

    if ( aHandle->mSkipInsert == ACP_FALSE )
    {
        sAceRC = applyInsertLogRecord( aContext,
                                       aHandle,
                                       aLogRecord );
    }
    else
    {
        /* Nothing to do */
    }

    return sAceRC;
}

ACP_INLINE ace_rc_t applyUpdateLogRecordWithSkipProperty( oaContext          * aContext,
                                                          oaOciApplierHandle * aHandle,
                                                          oaLogRecordUpdate  * aLogRecord )
{
    ace_rc_t sAceRC = ACE_RC_SUCCESS;

    if ( aHandle->mSkipUpdate == ACP_FALSE )
    {
        sAceRC = applyUpdateLogRecord( aContext,
                                       aHandle,
                                       aLogRecord );
    }
    else
    {
        /* Nothing to do */
    }

    return sAceRC;
}

ACP_INLINE ace_rc_t applyDeleteLogRecordWithSkipProperty( oaContext          * aContext,
                                                          oaOciApplierHandle * aHandle,
                                                          oaLogRecordDelete  * aLogRecord )
{
    ace_rc_t sAceRC = ACE_RC_SUCCESS;

    if ( aHandle->mSkipDelete == ACP_FALSE )
    {
        sAceRC = applyDeleteLogRecord( aContext,
                                       aHandle,
                                       aLogRecord );
    }
    else
    {
        /* Nothing to do */
    }

    return sAceRC;
}

/**
 * @breif  Log Record List를 OCI로 반영한다.
 *
 *         실패 시에도 로그만 남기고 계속 진행하므로, 결과를 반환하지 않는다.
 *
 * @param  aContext       Context
 * @param  aHandle        Oracle Applier Handle
 * @param  aLogRecordList Log Record List
 */
ace_rc_t oaOciApplierApplyLogRecordList( oaContext          * aContext,
                                         oaOciApplierHandle * aHandle,
                                         acp_list_t         * aLogRecordList,
                                         oaLogSN              aPrevLastProcessedSN, 
                                         oaLogSN            * aLastProcessedSN )
{
    acp_list_t  * sIterator   = NULL;
    acp_uint32_t  sRetryCount = 0;
    
    oaLogSN       sProcessedLogSN = 0;
    oaLogRecord * sLogRecord      = NULL;

    OA_PERFORMANCE_DECLARE_TICK_VARIABLE( OA_PERFORMANCE_TICK_APPLY );

    OA_PERFORMANCE_TICK_CHECK_BEGIN( OA_PERFORMANCE_TICK_APPLY );

    ACP_LIST_ITERATE( aLogRecordList, sIterator )
    {
        sRetryCount = 0;

        sLogRecord = (oaLogRecord *)((acp_list_node_t *)sIterator)->mObj;

        aHandle->mIsSkipErrorList = ACP_FALSE;
        
        while ( oaOciApplierApplyLogRecord( aContext,
                                            aHandle,
                                            sLogRecord )
                != ACE_RC_SUCCESS )
        {
            if ( aPrevLastProcessedSN >= sLogRecord->mCommon.mSN )
            {
                /* 만약 이전 접속때 Apply 했던 Log 라면 Error 에 대해서 재시도 하지 않고 넘어간다. 
                 * 이전에 Insert 가 Apply 되있는 상태에서 에러로 인해 Restart 한 상황일 때
                 * 같은 Log 에 대해 Insert 가 발생하면 Unique key 에러가 지속적으로 발생할 것이고 
                 * 이를 무시하지 않으면 계속 오류로 종료될 것이다.
                 * 따라서 이전에 이미 Apply 가 완료된 로그에 대해서 발생하는 에러는 무시해야 한다. */

                break;
            }
            else
            {
                /* nothing to do */
            }
            
            if ( sRetryCount < aHandle->mErrorRetryCount )
            {
                sRetryCount++;

                acpSleepSec( aHandle->mErrorRetryInterval );
            }
            else
            {
                if ( sLogRecord->mCommon.mType != OA_LOG_RECORD_TYPE_COMMIT )
                {
                    /* nothing to do */
                }
                else
                {
                    oaLogMessage( OAM_MSG_DUMP_LOG, "LogRecord apply aborted" );
                    (void)applyAbortLogRecord( aContext, aHandle );
                }
                
                oaCheckErrorCode( aHandle, DEFAULT_ERROR_CODE );
                ACE_TEST_RAISE( ( aHandle->mSkipError == 0 ) && 
                                ( aHandle->mIsSkipErrorList == ACP_FALSE ),
                                ERR_RETRY_END );
                ACE_TEST_RAISE( ( aHandle->mSkipError == 1 ) &&
                                ( aHandle->mIsSkipErrorList == ACP_TRUE  ), 
                                ERR_RETRY_END );
                break;
            }
        }

        sProcessedLogSN = sLogRecord->mCommon.mSN;

        oaAlaLogConverterResetLogRecord( sLogRecord );
    }

    OA_PERFORMANCE_TICK_CHECK_END( OA_PERFORMANCE_TICK_APPLY );

    *aLastProcessedSN = sProcessedLogSN;

    return ACE_RC_SUCCESS;

    ACE_EXCEPTION( ERR_RETRY_END )
    {
        /* applyAbortLogRecord 는 두번 실행 될 수 있는데 두번 실행되어도 문제가 발생하지 않는다. */
        oaLogMessage( OAM_MSG_DUMP_LOG, "LogRecord apply aborted" );
        (void)applyAbortLogRecord( aContext, aHandle );

        oaLogMessage( OAM_ERR_APPLY_ERROR, aHandle->mErrorRetryCount, "Oracle" );
    }

    ACE_EXCEPTION_END;

    OA_PERFORMANCE_TICK_CHECK_END( OA_PERFORMANCE_TICK_APPLY );

    *aLastProcessedSN = sProcessedLogSN;

    return ACE_RC_FAILURE;
}

ace_rc_t oaOciApplierApplyLogRecord( oaContext          * aContext,
                                     oaOciApplierHandle * aHandle,
                                     oaLogRecord        * aLogRecord )
{
    switch ( aLogRecord->mCommon.mType )
    {
        case OA_LOG_RECORD_TYPE_INSERT :
            ACE_TEST( applyInsertLogRecordWithSkipProperty( aContext,
                                                            aHandle,
                                                            &( aLogRecord->mInsert ) ) 
                      != ACE_RC_SUCCESS );
            break;

        case OA_LOG_RECORD_TYPE_UPDATE :
            ACE_TEST( applyUpdateLogRecordWithSkipProperty( aContext,
                                                            aHandle,
                                                            &( aLogRecord->mUpdate ) ) 
                      != ACE_RC_SUCCESS );
            break;

        case OA_LOG_RECORD_TYPE_DELETE :
            ACE_TEST( applyDeleteLogRecordWithSkipProperty( aContext,
                                                            aHandle,
                                                            &( aLogRecord->mDelete ) ) 
                      != ACE_RC_SUCCESS );
            break;

        case OA_LOG_RECORD_TYPE_COMMIT :
            ACE_TEST( applyCommitLogRecord( aContext, aHandle ) != ACE_RC_SUCCESS );
            break;

        default:
            break;
    }

    return ACE_RC_SUCCESS;

    ACE_EXCEPTION_END;

    writeOciDumpTypeLog( aHandle, aLogRecord );

    return ACE_RC_FAILURE;
}

static ace_rc_t readSkipErrorList( oaContext           * aContext,
                                   acp_std_file_t      * aFile,
                                   oaOciApplierHandle  * aHandle,
                                   acp_str_t           * aStr,
                                   acp_bool_t          * aIsEOF )
{
    acp_char_t      sLineBuffer[OA_ADAPTER_SKIP_ERROR_BUFF_SIZE + 1];
    acp_char_t      sErrMsg[1024];
    acp_bool_t      sIsEOF = ACP_FALSE;
    ace_rc_t        sAcpRC =  ACE_RC_SUCCESS;

    sLineBuffer[0] = '\0';

    sAcpRC = acpStdGetCString( aFile,
                               sLineBuffer,
                               OA_ADAPTER_SKIP_ERROR_BUFF_SIZE + 1 );
    ACE_TEST_RAISE( ACP_RC_NOT_SUCCESS(sAcpRC), ERROR_LOGIN_FILE_READ );

    sAcpRC = acpStrCpyBuffer( aStr,
                              sLineBuffer,
                              acpCStrLen( sLineBuffer, OA_ADAPTER_SKIP_ERROR_BUFF_SIZE + 1 ) );
    ACE_TEST_RAISE( ACP_RC_NOT_SUCCESS(sAcpRC), ERROR_LOGIN_FILE_READ );    

    (void)acpStdIsEOF( aFile, &sIsEOF );

    *aIsEOF = sIsEOF;

    return ACE_RC_SUCCESS;

    ACE_EXCEPTION( ERROR_LOGIN_FILE_READ )
    {
        acpErrorString( sAcpRC, sErrMsg, 1024 );

        if ( aHandle->mSkipError == 0 )
        {
            oaLogMessage( OAM_ERR_FILE_READ, OA_ADAPTER_SKIP_ERROR_INCLUDE_FILE_NAME, sErrMsg );
        }
        else
        {
            oaLogMessage( OAM_ERR_FILE_READ, OA_ADAPTER_SKIP_ERROR_EXCLUDE_FILE_NAME, sErrMsg );
        }
    }
    ACE_EXCEPTION_END;

    return ACE_RC_FAILURE;
}

static void trimSkipError( acp_str_t * aStr )
{
    acp_uint32_t        i       = 0;
    acp_uint32_t        j       = 0;    
    acp_uint32_t        sStrLen = 0;
    acp_char_t        * sBuffer = NULL;

    sStrLen = acpStrGetLength( aStr );
    sBuffer = acpStrGetBuffer( aStr );
    
    if ( sStrLen != 0 )
    {
        for ( i = 0 ; i < sStrLen ; i++ )
        {
            if (sBuffer[i] != ' ' )
            {
                break;
            }
        }

        for ( ; ( i < sStrLen ) && ( sBuffer[i] != '\0' ) ; i++ )
        {
            if ( (sBuffer[i] == '\r') || (sBuffer[i] == '\n') ||
                 (sBuffer[i] == '#' ) || (sBuffer[i] == ' ' ) )
            {
                break;
            }
            else
            {
                if ( i != j )
                {
                    sBuffer[j] = sBuffer[i];
                }
                else
                {
                    /* nothing to do */
                }
                j++;
            }
        }

        sBuffer[j] = '\0';
       
        acpStrResetLength( aStr );
    }
}

static ace_rc_t setSkipError( oaContext            * aContext,
                              oaOciApplierHandle   * aHandle,
                              acp_list_t           * aLogRecordList,
                              acp_str_t            * aStr )
{
    acp_sint32_t      sSign             = 0;
    acp_list_node_t * sSkipErrorNode    = NULL;
    acp_uint64_t    * sSkipError        = NULL;


    trimSkipError( aStr );
    
    if ( acpStrGetLength( aStr ) != 0 )
    {
        ACE_TEST_RAISE( acpMemCalloc( (void **)&sSkipErrorNode,
                                      1,
                                      ACI_SIZEOF(acp_list_node_t) )
                        != ACE_RC_SUCCESS, ERROR_MEM_CALLOC );

        ACE_TEST_RAISE( acpMemCalloc( (void **)&sSkipError,
                                      1,
                                      ACI_SIZEOF(acp_uint64_t) )
                        != ACE_RC_SUCCESS, ERROR_MEM_CALLOC );

        ACE_TEST_RAISE( acpStrToInteger( aStr, &sSign, sSkipError, NULL, 0, 0)
                       != ACE_RC_SUCCESS, ERROR_INVALID_SKIP_OCI_ERROR );

        acpListInitObj( sSkipErrorNode, (void *)sSkipError );
        acpListAppendNode( aLogRecordList, sSkipErrorNode );
    }
        
    return ACE_RC_SUCCESS;

    ACE_EXCEPTION( ERROR_MEM_CALLOC )
    {
        oaLogMessage(OAM_ERR_MEM_CALLOC );
    }
    ACE_EXCEPTION( ERROR_INVALID_SKIP_OCI_ERROR )
    {
        if ( aHandle->mSkipError == 0 )
        {
            oaLogMessage( OAM_ERR_INVALID_SKIP_LIST, OA_ADAPTER_SKIP_ERROR_INCLUDE_FILE_NAME );
        }
        else
        {
            oaLogMessage( OAM_ERR_INVALID_SKIP_LIST, OA_ADAPTER_SKIP_ERROR_EXCLUDE_FILE_NAME );
        }
    }
    ACE_EXCEPTION_END;

    if ( sSkipError != NULL )
    {
        acpMemFree( sSkipError );
        sSkipError = NULL;
    }

    if ( sSkipErrorNode != NULL )
    {
        acpMemFree( sSkipErrorNode );
        sSkipErrorNode = NULL;
    }

    return ACE_RC_FAILURE;
}

static ace_rc_t setSkipErrorList( oaContext           *aContext,
                                  oaOciApplierHandle  *aHandle,
                                  acp_list_t          *aLogRecordList,
                                  acp_std_file_t      *aFile )
{
    acp_bool_t      sIsEOF = ACP_FALSE;

    ACP_STR_DECLARE_DYNAMIC( sErrStr );
    ACP_STR_INIT_DYNAMIC( sErrStr,
                          OA_ADAPTER_SKIP_ERROR_BUFF_SIZE + 1,
                          OA_ADAPTER_SKIP_ERROR_BUFF_SIZE + 1 );

    while ( sIsEOF == ACP_FALSE )
    {
        acpStrClear( &sErrStr );

        ACE_TEST( readSkipErrorList( aContext,
                                     aFile,
                                     aHandle,
                                     &sErrStr,
                                     &sIsEOF ) != ACE_RC_SUCCESS );

        ACE_TEST( setSkipError( aContext,
                                aHandle,
                                aLogRecordList,
                                &sErrStr )
                  != ACE_RC_SUCCESS );
    }

    ACP_STR_FINAL( sErrStr );

    return ACE_RC_SUCCESS;

    ACE_EXCEPTION_END;

    ACP_STR_FINAL( sErrStr );

    return ACE_RC_FAILURE;
}

ace_rc_t oaOciInitializeSkipErrorList( oaContext          *aContext,
                                       oaOciApplierHandle *aHandle )
{
    acp_std_file_t      sFile = {NULL};
    acp_bool_t          sIsExistFile = ACP_FALSE;
    acp_bool_t          sIsOpenFile = ACP_FALSE;
    acp_char_t        * sFileName;
    acp_list_t        * sListHead = &(aHandle->mSkipErrorList);

    acpListInit( sListHead );

    if ( aHandle->mSkipError == 0 )
    {
        sFileName = OA_ADAPTER_SKIP_ERROR_INCLUDE_FILE_NAME;
    }
    else
    {
        sFileName = OA_ADAPTER_SKIP_ERROR_EXCLUDE_FILE_NAME;
    }

    ACE_TEST( openFile( aContext,
                        sFileName,
                        &sFile,
                        &sIsExistFile ) != ACE_RC_SUCCESS );
    if ( sIsExistFile == ACP_TRUE )
    {
        sIsOpenFile = ACP_TRUE;
        ACE_TEST( setSkipErrorList( aContext,
                                    aHandle,
                                    sListHead,
                                    &sFile ) != ACE_RC_SUCCESS );
        sIsOpenFile = ACP_FALSE;
        closeFile( &sFile );
    }
    else
    {
        /* Nothing to do */
    }

    return ACE_RC_SUCCESS;
    
    ACE_EXCEPTION_END;

    if ( sIsOpenFile == ACP_TRUE )
    {
        sIsOpenFile = ACP_FALSE;
        closeFile( &sFile );
    }
    else
    {
        /* Nothing to do */
    }

    oaLogMessage(OAM_MSG_DUMP_LOG, "oaOciInitializeSkipErrorList ACE_RC_FAILURE" );
    return ACE_RC_FAILURE;
}


void oaOciFinalizeSkipErrorList( oaOciApplierHandle * aHandle )
{
    acp_list_node_t   * sNodeNext   = NULL;    
    acp_list_node_t   * sIterator   = NULL;
    acp_list_t        * sListHead   = &(aHandle->mSkipErrorList);

    ACP_LIST_ITERATE_SAFE(sListHead, sIterator, sNodeNext)
    {
        if ( sIterator->mObj != NULL )
        {
            acpListDeleteNode(sIterator);
            acpMemFree( sIterator->mObj );
            acpMemFree( sIterator );
        }
    }
}

