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
#include <ace.h>
#include <acl.h>
#include <aciTypes.h>
#include <acpCStr.h>

#include <alaAPI.h>

#include <oaContext.h>
#include <oaConfig.h>
#include <oaLog.h>
#include <oaMsg.h>

#include <oaLogRecord.h>
#include <oaAlaLogConverter.h>

#include <oaPerformance.h>
#include <oaApplierInterface.h>
#include <oaJDBCApplier.h>

oaAltibaseToJDBCCharSet gAltibaseToJDBCCharSet[] =
{
    { 
        (acp_char_t *)"UTF8",         
        (acp_char_t *)"UTF-8"
    },
    { 
        (acp_char_t *)"UTF16",
        (acp_char_t *)"UTF-16"
    },
    {
        (acp_char_t *)"ASCII",
        (acp_char_t *)"US-ASCII"
    },
    {
        (acp_char_t *)"US7ASCII",
        (acp_char_t *)"US-ASCII"
    },
    {
        (acp_char_t *)"KO16KSC5601",
        (acp_char_t *)"KS_C_5601-1987"
    },
    {
        (acp_char_t *)"KOREAN",
        (acp_char_t *)"KS_C_5601-1987"
    },
    {
        (acp_char_t *)"KSC5601",
        (acp_char_t *)"KS_C_5601-1987"
    },
    {
        (acp_char_t *)"MS949",
        (acp_char_t *)"MS949"
    },
    {
        (acp_char_t *)"SHIFTJIS",
        (acp_char_t *)"Shift_JIS"
    },
    {
        (acp_char_t *)"EUCJP",
        (acp_char_t *)"EUC_JP"
    },
    {
        (acp_char_t *)"EUC-JP",
        (acp_char_t *)"EUC_JP"
    },
    {
        (acp_char_t *)"GB231280",
        (acp_char_t *)"GB2312"
    },
    {
        (acp_char_t *)"BIG5",
        (acp_char_t *)"Big5"
    },
    {
        (acp_char_t *)"MS936",
        (acp_char_t *)"GBK"
    },
    {
        (acp_char_t *)"MS932",
        (acp_char_t *)"MS932"
    },
    {
        NULL,
        NULL
    },
};        

static void getAndWriteBatchDMLErrorMessage( oaJDBCApplierHandle * aHandle,
                                             oaLogRecord         * aLogRecord,
                                             jint                * aParamStatusArray )
{
    acp_uint32_t           i                 = 0;
    acp_uint32_t           sArrayLength      = 0;
 
    if ( aParamStatusArray != NULL )
    {
        sArrayLength = oaJNIGetDMLStatusArrayLength( &(aHandle->mJNIInterfaceHandle));
        
        for ( i = 0 ; i < sArrayLength ; i++ )
        {
            switch ( aParamStatusArray[i] )
            {
                case STATEMENT_EXECUTE_FAILED :
                    oaLogMessageInfo("Batch DML Error occurred.\n"
                                     "  RETURN : EXECUTE_FAILED\n" );
                    oaLogRecordDumpDML( aLogRecord, i );
                    break;
     
                case STATEMENT_SUCCESS_NO_INFO :
                    oaLogMessageInfo("Batch DML Error occurred.\n"
                                     "  RETURN : SUCCESS_NO_INFO\n" );
                    oaLogRecordDumpDML( aLogRecord, i );
                    break;
     
                default:
                    /* Success - Update row count */
                    break;
            }
        }
    }
    else
    {
        oaLogMessageInfo("Batch DML Error occurred.\n"
                         "  RETURN : NULL\n" );
        for ( i = 0 ; i < sArrayLength ; i++ )
        {
            oaLogRecordDumpDML( aLogRecord, i );
        }
    }
}

static void writeJDBCDMLLog( oaJDBCApplierHandle * aHandle, 
                             oaLogRecord         * aLogRecord,
                             jint                * aParamStatusArray )
{
    acp_uint32_t sBatchDMLCount = 0;
    
    if ( aHandle->mJNIInterfaceHandle.mConflictLoggingLevel == 2 )
    {
        switch (  aLogRecord->mCommon.mType )
        {
            case OA_LOG_RECORD_TYPE_INSERT:
                sBatchDMLCount = ((oaLogRecordInsert*)aLogRecord)->mArrayDMLCount;
                break;
            case OA_LOG_RECORD_TYPE_UPDATE:
                sBatchDMLCount = ((oaLogRecordUpdate*)aLogRecord)->mArrayDMLCount;
                break;
            case OA_LOG_RECORD_TYPE_DELETE:
                sBatchDMLCount = ((oaLogRecordDelete*)aLogRecord)->mArrayDMLCount;
                break;
            default:
                break;
        }
         
        if ( sBatchDMLCount == 1 )
        {
            oaLogRecordDumpDML( aLogRecord, 0 /* aArrayDMLIndex */ );
        }
        else
        {
            getAndWriteBatchDMLErrorMessage( aHandle, 
                                             aLogRecord,
                                             aParamStatusArray );
        }
    }
    else
    {
        /* Nothing to do */
    }
}

static void writeJDBCDumpTypeLog( oaJDBCApplierHandle * aHandle,
                                  oaLogRecord         * aLogRecord )
{
    if ( aHandle->mJNIInterfaceHandle.mConflictLoggingLevel == 2 )
    {
        oaLogRecordDumpType( aLogRecord );
    }
    else
    {
        /* Nothing to do */
    }
}

static ace_rc_t altibaseToJDBCCharSet( oaContext            * aContext,
                                       oaJDBCApplierHandle  * aHandle, 
                                       acp_char_t           * aAltibaseCharSet,
                                       acp_char_t          ** aJDBCCharSet)
{
    acp_sint32_t    i       = 0;
    acp_char_t   *  sCharSet = NULL;
    
    for ( i = 0 ; gAltibaseToJDBCCharSet[i].mAltibaseCharSet != NULL ; i++ )
    {
        if ( acpCStrCmp( aAltibaseCharSet, 
                         gAltibaseToJDBCCharSet[i].mAltibaseCharSet,
                         acpCStrLen( gAltibaseToJDBCCharSet[i].mAltibaseCharSet, CHARSET_MAX_LEN ) )
             == 0 )
        {
            sCharSet = gAltibaseToJDBCCharSet[i].mJDBCCharSet;
            break;
        }
        else
        {
            /* Nothing to do */
        }
    }
    
    ACE_TEST_RAISE( sCharSet == NULL, ERROR_NOT_SUPPORT_CHARSET );

    ACE_TEST_RAISE( oaJNICheckSupportedCharSet( aContext, 
                                                &(aHandle->mJNIInterfaceHandle),
                                                sCharSet )
                    != ACP_RC_SUCCESS, ERROR_NOT_SUPPORT_CHARSET );
    
    *aJDBCCharSet = sCharSet;
    
    return ACE_RC_SUCCESS;
    
    ACE_EXCEPTION( ERROR_NOT_SUPPORT_CHARSET )
    {
        oaLogMessage( OAM_MSG_NOT_SUPPORT_CHARSET, aAltibaseCharSet );
    }
    ACE_EXCEPTION_END;
    
    *aJDBCCharSet = NULL;
     
    return ACE_RC_FAILURE;
}

ace_rc_t oaJDBCApplierInitialize( oaContext                     * aContext,
                                  oaConfigJDBCConfiguration     * aConfig,
                                  const ALA_Replication         * aAlaReplication,
                                  oaJDBCApplierHandle          ** aHandle )
{
    acp_rc_t              sAcpRC    = ACP_RC_SUCCESS;
    oaJDBCApplierHandle * sHandle   = NULL;
     
    ACE_TEST_RAISE( acpMemCalloc( (void **)&sHandle,
                                  1,
                                  ACI_SIZEOF(*sHandle) )
                    != ACE_RC_SUCCESS, ERROR_MEM_CALLOC );
    
    sAcpRC = acpMemCalloc( (void **)&(sHandle->mPreparedStatement),
                           aAlaReplication->mTableCount,
                           ACI_SIZEOF(*(sHandle->mPreparedStatement)) );    
    ACE_TEST_RAISE( ACP_RC_NOT_SUCCESS( sAcpRC ), ERROR_MEM_CALLOC );
    
    sAcpRC = acpMemCalloc( (void **)&(sHandle->mDMLResultArray),
                           aConfig->mBatchDMLMaxSize,
                           ACI_SIZEOF(*(sHandle->mDMLResultArray)) );
    ACE_TEST_RAISE( ACP_RC_NOT_SUCCESS( sAcpRC ), ERROR_MEM_CALLOC );
    
    sHandle->mTableCount = aAlaReplication->mTableCount;
    sHandle->mIsGroupCommit = ( aConfig->mGroupCommit == 1 ) ? ACP_TRUE : ACP_FALSE;
    sHandle->mBatchDMLMaxSize = aConfig->mBatchDMLMaxSize;
     
    sHandle->mSkipInsert = ( aConfig->mSkipInsert == 1 ) ? ACP_TRUE : ACP_FALSE;
    sHandle->mSkipUpdate = ( aConfig->mSkipUpdate == 1 ) ? ACP_TRUE : ACP_FALSE;
    sHandle->mSkipDelete = ( aConfig->mSkipDelete == 1 ) ? ACP_TRUE : ACP_FALSE;

    sHandle->mSetUserToTable = ( aConfig->mSetUserToTable == 1 ) ? ACP_TRUE : ACP_FALSE;
    
    sHandle->mSetColumnToInsert = ( aConfig->mSetColumnToInsert == 1 ) ? ACP_TRUE : ACP_FALSE;
        
    sHandle->mErrorRetryCount = aConfig->mErrorRetryCount;
    sHandle->mErrorRetryInterval = aConfig->mErrorRetryInterval;
    sHandle->mSkipError = aConfig->mSkipError;
    
    sHandle->mJNIInterfaceHandle.mUser = aConfig->mUser;
    sHandle->mJNIInterfaceHandle.mPassword = aConfig->mPassword;
    sHandle->mJNIInterfaceHandle.mCommitMode = ( aConfig->mAutocommitMode == 1 ) ? ACP_TRUE : ACP_FALSE;
    sHandle->mJNIInterfaceHandle.mConflictLoggingLevel = aConfig->mConflictLoggingLevel;
    sHandle->mJNIInterfaceHandle.mXmxOpt = aConfig->mXmxOpt;
    sHandle->mJNIInterfaceHandle.mDriverPath = aConfig->mDriverPath;
    sHandle->mJNIInterfaceHandle.mDriverClass = aConfig->mDriverClass;
    sHandle->mJNIInterfaceHandle.mConnectionUrl = aConfig->mConnectionUrl;
    sHandle->mJNIInterfaceHandle.mJVMOpt = aConfig->mJVMOpt;
    
    ACE_TEST( oaJNIInitialize( aContext, &(sHandle->mJNIInterfaceHandle) ) != ACE_RC_SUCCESS );
    
    ACE_TEST( altibaseToJDBCCharSet( aContext,
                                     sHandle, 
                                     (acp_char_t *)aAlaReplication->mDBCharSet, 
                                     &(sHandle->mCharset) ) 
              != ACP_RC_SUCCESS );
    
    ACE_TEST( altibaseToJDBCCharSet( aContext,
                                     sHandle, 
                                     (acp_char_t *)aAlaReplication->mDBNCharSet, 
                                     &(sHandle->mNCharset) ) 
              != ACP_RC_SUCCESS );
    
    *aHandle = sHandle;
    
    return ACE_RC_SUCCESS;
  
    ACE_EXCEPTION( ERROR_MEM_CALLOC )
    {
        oaLogMessage(OAM_ERR_MEM_CALLOC );
    }
    ACE_EXCEPTION_END;
     
    if ( sHandle != NULL )
    {
        if ( sHandle->mPreparedStatement != NULL )
        {
            acpMemFree( sHandle->mPreparedStatement );
            sHandle->mPreparedStatement = NULL;
        }
        else
        {
            /*Nothing to do */
        }
        
        acpMemFree( sHandle );
        sHandle = NULL;
    }
    else
    {
        /*Nothing to do */
    }
    
    return ACE_RC_FAILURE;
}

ace_rc_t oaJDBCApplierConnect( oaContext *aContext, oaJDBCApplierHandle *aHandle)
{
    ACE_TEST( oaJNIJDBCConnect( aContext, &(aHandle->mJNIInterfaceHandle) ) );
 
    return ACE_RC_SUCCESS;
 
    ACE_EXCEPTION_END;
 
    return ACE_RC_FAILURE;
}

ace_rc_t initializeJDBCApplier( oaContext                 * aContext,
                                oaConfigHandle            * aConfigHandle,
                                const ALA_Replication     * aAlaReplication,
                                oaJDBCApplierHandle      ** aJDBCApplierHandle )
{
    oaConfigJDBCConfiguration   sConfiguration;
    oaJDBCApplierHandle       * sApplierHandle = NULL;
    acp_sint32_t                sStage = 0;
 
    oaConfigGetJDBCConfiguration( aConfigHandle, &sConfiguration );
    ACE_TEST( oaJDBCApplierInitialize( aContext,
                                       &sConfiguration,
                                       aAlaReplication,
                                       &sApplierHandle )
              != ACE_RC_SUCCESS ) ;
    sStage = 1;
    
    ACE_TEST( oaJDBCApplierConnect( aContext, sApplierHandle ) != ACE_RC_SUCCESS );
    sStage = 2;

    ACE_TEST( oaJDBCInitializeSkipErrorList(aContext, sApplierHandle) != ACE_RC_SUCCESS );

    ACE_TEST( oaJDBCExecuteLogInSql(aContext, sApplierHandle) != ACE_RC_SUCCESS );

    *aJDBCApplierHandle = sApplierHandle;
 
    return ACE_RC_SUCCESS;
    
    ACE_EXCEPTION_END;
    
    switch ( sStage )
    {
        case 2 :
            oaJDBCApplierDisconnect( sApplierHandle );
        case 1 :
            oaJDBCApplierFinalize( sApplierHandle );
        default :
            break;
    }
    
    return ACE_RC_FAILURE;
}

void oaJDBCClosePrepareStmt( oaJDBCApplierHandle * aHandle )
{
    acp_sint32_t i = 0;
    
    for ( i = 0; i < aHandle->mTableCount; i++ )
    {
        if ( aHandle->mPreparedStatement[i].mInsert != NULL )
        {
            oaJNIPreparedStmtClose( &(aHandle->mJNIInterfaceHandle),
                                    aHandle->mPreparedStatement[i].mInsert );

            aHandle->mPreparedStatement[i].mInsert = NULL;
        }
        else
        {
            /* Nothing to do */
        }

        if ( aHandle->mPreparedStatement[i].mDelete != NULL )
        {
            oaJNIPreparedStmtClose( &(aHandle->mJNIInterfaceHandle),
                                    aHandle->mPreparedStatement[i].mDelete );

            aHandle->mPreparedStatement[i].mDelete = NULL;
        }
        else
        {
            /* Nothing to do */
        }
    }
}

void oaJDBCApplierDisconnect( oaJDBCApplierHandle * aHandle )
{
    oaJNIConnectionClose( &(aHandle->mJNIInterfaceHandle));
}

void oaJDBCApplierFinalize( oaJDBCApplierHandle * aHandle )
{   
    if ( aHandle != NULL )
    {
        oaJNIFinalize( &(aHandle->mJNIInterfaceHandle) ); 
        
        if ( aHandle->mPreparedStatement != NULL )
        {
            acpMemFree( aHandle->mPreparedStatement );
        }
        else
        {
            /* Nothing to do */
        }
        
        if ( aHandle->mDMLResultArray != NULL )
        {
            acpMemFree( aHandle->mDMLResultArray );
        }
        else
        {
            /* Nothing to do */
        }
        
        acpMemFree( aHandle );
    }
    else
    {
        /* Nothing to do */
    }
}

void finalizeJDBCApplier( oaJDBCApplierHandle * aApplierHandle )
{
    oaJDBCFinalizeSkipErrorList( aApplierHandle );

    oaJDBCClosePrepareStmt( aApplierHandle );
    
    oaJDBCApplierDisconnect( aApplierHandle );
     
    oaJDBCApplierFinalize( aApplierHandle );
}

void oaFinalizeJAVAVM()
{
    oaJNIDestroyJAVAVM();
}

static ace_rc_t bindLogRecordColumn( oaContext           * aContext,
                                     oaJDBCApplierHandle * aHandle,
                                     acp_uint16_t          aPosition,
                                     acp_sint32_t          aArrayDMLIndex,
                                     oaLogRecordColumn   * aLogRecordColumn,
                                     jobject               aPrepareStmtObject )
{
    acp_char_t   * sCharSet = NULL;

    switch ( aLogRecordColumn->mType )
    {   
        case OA_LOG_RECORD_VALUE_TYPE_NUMERIC:
        case OA_LOG_RECORD_VALUE_TYPE_FLOAT:
        case OA_LOG_RECORD_VALUE_TYPE_DOUBLE:
        case OA_LOG_RECORD_VALUE_TYPE_REAL:
        case OA_LOG_RECORD_VALUE_TYPE_BIGINT:
        case OA_LOG_RECORD_VALUE_TYPE_INTEGER:
        case OA_LOG_RECORD_VALUE_TYPE_SMALLINT:
        case OA_LOG_RECORD_VALUE_TYPE_DATE:
            sCharSet = NULL;
            break;
            
        case OA_LOG_RECORD_VALUE_TYPE_CHAR:
        case OA_LOG_RECORD_VALUE_TYPE_VARCHAR:
            sCharSet = aHandle->mCharset;
            break;
            
        case OA_LOG_RECORD_VALUE_TYPE_NCHAR:
        case OA_LOG_RECORD_VALUE_TYPE_NVARCHAR:
            sCharSet = aHandle->mNCharset;
            break;
            
        default :
            sCharSet = NULL;
            break;
    }

    if ( aLogRecordColumn->mLength[aArrayDMLIndex] != 0 )
    {
        if ( aLogRecordColumn->mType != OA_LOG_RECORD_VALUE_TYPE_DATE )
        {
            ACE_TEST( oaJNIPreparedStmtSetString( aContext,
                                                  &(aHandle->mJNIInterfaceHandle),
                                                  aPrepareStmtObject, 
                                                  aPosition,
                                                  aLogRecordColumn->mLength[aArrayDMLIndex],
                                                  &(aLogRecordColumn->mValue[aLogRecordColumn->mMaxLength * aArrayDMLIndex]),
                                                  sCharSet )
                      != ACE_RC_SUCCESS );
        }
        else
        {
            ACE_TEST( oaJNIPreparedStmtSetTimeStamp( aContext,
                                                     &(aHandle->mJNIInterfaceHandle),
                                                     aPrepareStmtObject, 
                                                     aPosition,
                                                     &(aLogRecordColumn->mValue[aLogRecordColumn->mMaxLength * aArrayDMLIndex]) )
                      != ACE_RC_SUCCESS );        
        }
    }
    else
    {
        ACE_TEST( oaJNIPreparedStmtSetString( aContext,
                                              &(aHandle->mJNIInterfaceHandle),
                                              aPrepareStmtObject, 
                                              aPosition,
                                              0,
                                              NULL,
                                              sCharSet )
                  != ACE_RC_SUCCESS );
    }

    return ACE_RC_SUCCESS;
    
    ACE_EXCEPTION_END;
    
    return ACE_RC_FAILURE;
}

static void setDMLResultArray( oaJDBCApplierHandle * aHandle, jint * aParamStatusArray )
{
    acp_uint32_t    i                 = 0;
    acp_uint32_t    j                 = 0;
    acp_uint32_t    sArrayLength      = 0;
    
    sArrayLength = oaJNIGetDMLStatusArrayLength( &(aHandle->mJNIInterfaceHandle) );
    
    if ( aParamStatusArray != NULL )
    {
        for ( i = 0 ; i < aHandle->mBatchDMLMaxSize ; i++ )
        {
            if ( aHandle->mDMLResultArray[i] == ACP_FALSE )
            {
               if ( aParamStatusArray[j] >= 0 )
               {
                   aHandle->mDMLResultArray[i] = ACP_TRUE;    
               }
               else
               {
                   /* Nothing to do */
               }
               
               j++;
               if ( j >= sArrayLength )
               {
                   break;
               }
               else
               {
                   /* Nothing to do */
               }
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
}


static ace_rc_t bindInsertLogRecordColumns( oaContext           * aContext,
                                            oaJDBCApplierHandle * aHandle,
                                            oaLogRecordInsert   * aLogRecord,
                                            acp_sint32_t          aArrayDMLIndex,
                                            jobject               aPrepareStmtObject )
{
    acp_sint32_t i               = 0;
    acp_bool_t   sIsHiddenColumn = ACP_FALSE;
    acp_uint16_t sBindIndex      = 0;

    ACE_TEST( oaJNIPreparedStmtClearParameters( aContext,
                                                &(aHandle->mJNIInterfaceHandle),
                                                aPrepareStmtObject )
              != ACE_RC_SUCCESS );
     
    for ( i = 0 ; i < aLogRecord->mColumnCount ; i++ )
    {
        sIsHiddenColumn = oaLogRecordColumnIsHiddenColumn( &(aLogRecord->mColumn[i]) );
        if ( sIsHiddenColumn == ACP_FALSE )
        {
            sBindIndex++;
            ACE_TEST( bindLogRecordColumn( aContext,
                                           aHandle,
                                           sBindIndex,
                                           aArrayDMLIndex,
                                           &(aLogRecord->mColumn[i]),
                                           aPrepareStmtObject )
                      != ACE_RC_SUCCESS );
        }
        else
        {
            /* Nothing to do  */
        }
    }
     
    return ACE_RC_SUCCESS;
 
    ACE_EXCEPTION_END;
 
    return ACE_RC_FAILURE;
}

static ace_rc_t bindInsertStatement( oaContext           * aContext,
                                     oaJDBCApplierHandle * aHandle,
                                     oaLogRecordInsert   * aLogRecord,
                                     jobject               aPrepareStmtObject )
{
    acp_uint64_t i = 0;
 
    if ( aLogRecord->mArrayDMLCount == 1 )
    {
        ACE_TEST( bindInsertLogRecordColumns( aContext,
                                              aHandle,
                                              aLogRecord,
                                              0, /* aArrayDMLIndex */
                                              aPrepareStmtObject )
                  != ACE_RC_SUCCESS );
    }
    else
    {
        for ( i = 0 ; i < aLogRecord->mArrayDMLCount ; i++ )
        {
            if ( aHandle->mDMLResultArray[i] == ACP_FALSE )
            {
                ACE_TEST( bindInsertLogRecordColumns( aContext,
                                                      aHandle,
                                                      aLogRecord,
                                                      i,
                                                      aPrepareStmtObject )
                          != ACE_RC_SUCCESS );
                 
                ACE_TEST( oaJNIPreparedStmtAddBatch( aContext, 
                                                     &(aHandle->mJNIInterfaceHandle),
                                                     aPrepareStmtObject )
                          != ACE_RC_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }
        }
    }
    return ACE_RC_SUCCESS;
    
    ACE_EXCEPTION_END;
     
    return ACE_RC_FAILURE;
}

static ace_rc_t prepareInsertStatement( oaContext           * aContext,
                                        oaJDBCApplierHandle * aHandle,
                                        oaLogRecordInsert   * aLogRecord,
                                        jobject             * aPrepareStmtObject )
{
    ACP_STR_DECLARE_DYNAMIC( sSqlQuery );
    ACP_STR_INIT_DYNAMIC( sSqlQuery,
                          4096,
                          4096 );
    
    prepareInsertQuery( aLogRecord,
                        &sSqlQuery,
                        ACP_FALSE, /* DPath Insert */
                        aHandle->mSetUserToTable,
                        aHandle->mSetColumnToInsert );
    
    ACE_TEST( oaJNIpreparedStatment( aContext,
                                     &(aHandle->mJNIInterfaceHandle),
                                     (acp_char_t *)acpStrGetBuffer(&sSqlQuery),
                                     aPrepareStmtObject )
              != ACP_RC_SUCCESS );
     
    ACP_STR_FINAL(sSqlQuery);

    return ACE_RC_SUCCESS;

    ACE_EXCEPTION_END;
     
    ACP_STR_FINAL( sSqlQuery );

    return ACE_RC_FAILURE;
}

static ace_rc_t getPreparedInsertStatement( oaContext           * aContext,
                                            oaJDBCApplierHandle * aHandle,
                                            oaLogRecordInsert   * aLogRecord,
                                            jobject             * aPrepareStmtObject )
{
    jobject  sStatement = NULL;
    
    if ( aHandle->mPreparedStatement[aLogRecord->mTableId].mInsert == NULL )
    {
        ACE_TEST( prepareInsertStatement( aContext,
                                          aHandle,
                                          aLogRecord,
                                          &sStatement )
                  != ACE_RC_SUCCESS );
        
        aHandle->mPreparedStatement[aLogRecord->mTableId].mInsert = sStatement;
    }
    else
    {
        sStatement = aHandle->mPreparedStatement[aLogRecord->mTableId].mInsert;
    }
    
    *aPrepareStmtObject = sStatement;
    
    return ACE_RC_SUCCESS;
    
    ACE_EXCEPTION_END;

    return ACE_RC_FAILURE;
    
}

static ace_rc_t applyInsertLogRecord( oaContext           * aContext,
                                      oaJDBCApplierHandle * aHandle,
                                      oaLogRecordInsert   * aLogRecord )
{
    jobject         sPrepareStmtObject = NULL;
    jint           *sParamStatusArray  = NULL;
    
    ACE_TEST( getPreparedInsertStatement( aContext,
                                          aHandle,
                                          aLogRecord,
                                          &sPrepareStmtObject )
              != ACE_RC_SUCCESS );
    
    ACE_TEST( bindInsertStatement( aContext,
                                   aHandle,
                                   aLogRecord,
                                   sPrepareStmtObject )
              != ACE_RC_SUCCESS );
 
    if ( aLogRecord->mArrayDMLCount == 1 )
    {
        ACE_TEST( oaJNIPreparedStmtExecute( aContext, 
                                            &(aHandle->mJNIInterfaceHandle), 
                                            sPrepareStmtObject )
                  != ACE_RC_SUCCESS );
    }
    else
    {
        ACE_TEST_RAISE( oaJNIPreparedStmtExecuteBatch( aContext, 
                                                       &(aHandle->mJNIInterfaceHandle), 
                                                       sPrepareStmtObject )
                        != ACE_RC_SUCCESS, ERR_EXECUTE_BATCH_EXCEPTION );
        
        ACE_TEST( oaJNIPreparedStmtClearBatch( aContext,
                                               &(aHandle->mJNIInterfaceHandle),
                                               sPrepareStmtObject )
                  != ACE_RC_SUCCESS );
    }
    
    return ACE_RC_SUCCESS;
     
    // BUG-46551 611이하 jdbcDriver에서는 exception시 내부적으로 clearBatch를 해주지 않음
    ACE_EXCEPTION( ERR_EXECUTE_BATCH_EXCEPTION )
    {
        oaJNIPreparedStmtClearBatch( aContext,
                                     &(aHandle->mJNIInterfaceHandle),
                                     sPrepareStmtObject );
    }    
    ACE_EXCEPTION_END;
    
    sParamStatusArray = oaJNIGetDMLStatusArray( &(aHandle->mJNIInterfaceHandle) );
    
    setDMLResultArray( aHandle, sParamStatusArray );
    
    writeJDBCDMLLog( aHandle, 
                     (oaLogRecord *)aLogRecord, 
                     sParamStatusArray );
        
    oaJNIReleaseDMLStatusArray( &(aHandle->mJNIInterfaceHandle), sParamStatusArray );
    
    return ACE_RC_FAILURE;
}

static ace_rc_t bindUpdateLogRecordColumns( oaContext           * aContext,
                                            oaJDBCApplierHandle * aHandle,
                                            oaLogRecordUpdate   * aLogRecord,
                                            acp_sint32_t          aArrayDMLIndex,
                                            jobject               aPrepareStmtObject )
{
    acp_sint32_t i               = 0;
    acp_sint32_t j               = 0;
    acp_uint32_t sColumnID       = 0;    
    acp_bool_t   sIsHiddenColumn = ACP_FALSE;
    acp_uint16_t sBindIndex      = 0;
     
    for ( i = 0 ; i < aLogRecord->mColumnCount ; i++ )
    {
        sColumnID = aLogRecord->mColumnIDMap[i];
        
        sIsHiddenColumn = oaLogRecordColumnIsHiddenColumn( &(aLogRecord->mColumn[sColumnID]) );
        if ( sIsHiddenColumn == ACP_FALSE )
        {
            sBindIndex++;
            ACE_TEST( bindLogRecordColumn( aContext,
                                           aHandle,
                                           sBindIndex,
                                           aArrayDMLIndex,
                                           &(aLogRecord->mColumn[sColumnID]),
                                           aPrepareStmtObject )
                      != ACE_RC_SUCCESS );
        }
        else
        {
            /* Nothing to do  */
        }
    }
     
    for (j = 0; j < aLogRecord->mPrimaryKeyCount; j++)
    {
        sIsHiddenColumn = oaLogRecordColumnIsHiddenColumn( &(aLogRecord->mColumn[j]) );
        ACE_ASSERT( sIsHiddenColumn == ACP_FALSE );

        sBindIndex++;
        ACE_TEST ( bindLogRecordColumn( aContext,
                                        aHandle,
                                        sBindIndex,
                                        aArrayDMLIndex,
                                        &(aLogRecord->mPrimaryKey[j]),
                                        aPrepareStmtObject )
                   != ACE_RC_SUCCESS );
    }
        
    return ACE_RC_SUCCESS;
 
    ACE_EXCEPTION_END;
 
    return ACE_RC_FAILURE;
}

static ace_rc_t bindUpdateStatement( oaContext           * aContext,
                                     oaJDBCApplierHandle * aHandle,
                                     oaLogRecordUpdate   * aLogRecord,
                                     jobject               aPrepareStmtObject )
{
    ACE_TEST( bindUpdateLogRecordColumns( aContext,
                                          aHandle,
                                          aLogRecord,
                                          0, /* aArrayDMLIndex */
                                          aPrepareStmtObject )
              != ACE_RC_SUCCESS );

    return ACE_RC_SUCCESS;
    
    ACE_EXCEPTION_END;
     
    return ACE_RC_FAILURE;
}

static ace_rc_t prepareUpdateStatement( oaContext           * aContext,
                                         oaJDBCApplierHandle * aHandle,
                                         oaLogRecordUpdate   * aLogRecord,
                                         jobject             * aPrepareStmtObject )
{
    ACP_STR_DECLARE_DYNAMIC( sSqlQuery );
    ACP_STR_INIT_DYNAMIC( sSqlQuery,
                          4096,
                          4096 );
   
    prepareUpdateQuery( aLogRecord,
                        &sSqlQuery,
                        aHandle->mSetUserToTable );
    
    ACE_TEST( oaJNIpreparedStatment( aContext,
                                     &(aHandle->mJNIInterfaceHandle),
                                     (acp_char_t *)acpStrGetBuffer(&sSqlQuery),
                                     aPrepareStmtObject )
              != ACP_RC_SUCCESS );
    
    ACP_STR_FINAL(sSqlQuery);

    return ACE_RC_SUCCESS;

    ACE_EXCEPTION_END;
    
    ACP_STR_FINAL( sSqlQuery );

    return ACE_RC_FAILURE;
}

static ace_rc_t getPreparedUpdateStatement( oaContext           * aContext,
                                            oaJDBCApplierHandle * aHandle,
                                            oaLogRecordUpdate   * aLogRecord,
                                            jobject             * aPrepareStmtObject )
{
    jobject  sStatement = NULL;
    
    ACE_TEST( prepareUpdateStatement( aContext,
                                      aHandle,
                                      aLogRecord,
                                      &sStatement )
              != ACE_RC_SUCCESS );
    
    *aPrepareStmtObject = sStatement;
    
    return ACE_RC_SUCCESS;
    
    ACE_EXCEPTION_END;

    return ACE_RC_FAILURE;
}

static ace_rc_t applyUpdateLogRecord( oaContext           * aContext,
                                      oaJDBCApplierHandle * aHandle,
                                      oaLogRecordUpdate   * aLogRecord )
{
    jobject         sPrepareStmtObject = NULL;
    jint           *sParamStatusArray  = NULL;

    ACE_TEST( getPreparedUpdateStatement( aContext,
                                          aHandle,
                                          aLogRecord,
                                          &sPrepareStmtObject )
              != ACE_RC_SUCCESS );
 
    ACE_TEST( bindUpdateStatement( aContext,
                                   aHandle,
                                   aLogRecord,
                                   sPrepareStmtObject )
              != ACE_RC_SUCCESS );
 
    ACE_TEST( oaJNIPreparedStmtExecute( aContext, 
                                        &(aHandle->mJNIInterfaceHandle), 
                                        sPrepareStmtObject )
              != ACE_RC_SUCCESS );
    
    oaJNIPreparedStmtClose( &(aHandle->mJNIInterfaceHandle),
                            sPrepareStmtObject );
     
    return ACE_RC_SUCCESS;
     
    ACE_EXCEPTION_END;
     
    if ( sPrepareStmtObject != NULL )
    {
        oaJNIPreparedStmtClose( &(aHandle->mJNIInterfaceHandle),
                                sPrepareStmtObject );
    }
    else
    {
        /* Nothing to do */
    }
    
    sParamStatusArray = oaJNIGetDMLStatusArray( &(aHandle->mJNIInterfaceHandle) );
    
    setDMLResultArray( aHandle, sParamStatusArray );
    
    writeJDBCDMLLog( aHandle, 
                     (oaLogRecord *)aLogRecord, 
                     sParamStatusArray );
        
    oaJNIReleaseDMLStatusArray( &(aHandle->mJNIInterfaceHandle), sParamStatusArray );
     
    return ACE_RC_FAILURE;
}

static ace_rc_t bindDeleteLogRecordColumns( oaContext           * aContext,
                                            oaJDBCApplierHandle * aHandle,
                                            oaLogRecordDelete   * aLogRecord,
                                            acp_sint32_t          aArrayDMLIndex,
                                            jobject               aPrepareStmtObject )
{
    acp_sint32_t i               = 0;
    acp_bool_t   sIsHiddenColumn = ACP_FALSE;
    acp_uint16_t sBindIndex      = 0;
     
    for ( i = 0 ; i < aLogRecord->mPrimaryKeyCount ; i++ )
    {
        sIsHiddenColumn = oaLogRecordColumnIsHiddenColumn( &(aLogRecord->mPrimaryKey[i]) );
        if ( sIsHiddenColumn == ACP_FALSE )
        {
            sBindIndex++;
            ACE_TEST( bindLogRecordColumn( aContext,
                                           aHandle,
                                           sBindIndex,
                                           aArrayDMLIndex,
                                           &(aLogRecord->mPrimaryKey[i]),
                                           aPrepareStmtObject )
                      != ACE_RC_SUCCESS );
        }
        else
        {
            /* Nothing to do  */
        }
    }
  
    return ACE_RC_SUCCESS;
 
    ACE_EXCEPTION_END;
 
    return ACE_RC_FAILURE;
}

static ace_rc_t bindDeleteStatement( oaContext           * aContext,
                                     oaJDBCApplierHandle * aHandle,
                                     oaLogRecordDelete   * aLogRecord,
                                     jobject               aPrepareStmtObject )
{
    acp_uint64_t i = 0;
 
    if ( aLogRecord->mArrayDMLCount == 1 )
    {
        ACE_TEST( bindDeleteLogRecordColumns( aContext,
                                              aHandle,
                                              aLogRecord,
                                              0, /* aArrayDMLIndex */
                                              aPrepareStmtObject )
                  != ACE_RC_SUCCESS );
    }
    else
    {
        for ( i = 0 ; i < aLogRecord->mArrayDMLCount ; i++ )
        {
            if ( aHandle->mDMLResultArray[i] == ACP_FALSE )
            {
                ACE_TEST( bindDeleteLogRecordColumns( aContext,
                                                      aHandle,
                                                      aLogRecord,
                                                      i,
                                                      aPrepareStmtObject )
                          != ACE_RC_SUCCESS );
                 
                ACE_TEST( oaJNIPreparedStmtAddBatch( aContext, 
                                                     &(aHandle->mJNIInterfaceHandle),
                                                     aPrepareStmtObject )
                          != ACE_RC_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }
        }
    }
    return ACE_RC_SUCCESS;
    
    ACE_EXCEPTION_END;
     
    return ACE_RC_FAILURE;
}

static ace_rc_t prepareDeleteStatement( oaContext           * aContext,
                                         oaJDBCApplierHandle * aHandle,
                                         oaLogRecordDelete   * aLogRecord,
                                         jobject             * aPrepareStmtObject )
{
    ACP_STR_DECLARE_DYNAMIC( sSqlQuery );
    ACP_STR_INIT_DYNAMIC( sSqlQuery,
                          4096,
                          4096 );
   
    prepareDeleteQuery( aLogRecord,
                        &sSqlQuery,
                        aHandle->mSetUserToTable );
    
    ACE_TEST( oaJNIpreparedStatment( aContext,
                                     &(aHandle->mJNIInterfaceHandle),
                                     (acp_char_t *)acpStrGetBuffer(&sSqlQuery),
                                     aPrepareStmtObject )
              != ACP_RC_SUCCESS );
    
    ACP_STR_FINAL(sSqlQuery);

    return ACE_RC_SUCCESS;

    ACE_EXCEPTION_END;
    
    ACP_STR_FINAL( sSqlQuery );

    return ACE_RC_FAILURE;
}

static ace_rc_t getPreparedDeleteStatement( oaContext           * aContext,
                                            oaJDBCApplierHandle * aHandle,
                                            oaLogRecordDelete   * aLogRecord,
                                            jobject             * aPrepareStmtObject )
{
    jobject  sStatement = NULL;
    
    if ( aHandle->mPreparedStatement[aLogRecord->mTableId].mDelete == NULL )
    {
        ACE_TEST( prepareDeleteStatement( aContext,
                                          aHandle,
                                          aLogRecord,
                                          &sStatement )
                  != ACE_RC_SUCCESS );
        
        aHandle->mPreparedStatement[aLogRecord->mTableId].mDelete = sStatement;
    }
    else
    {
        sStatement = aHandle->mPreparedStatement[aLogRecord->mTableId].mDelete;
    }
    
    *aPrepareStmtObject = sStatement;
    
    return ACE_RC_SUCCESS;
    
    ACE_EXCEPTION_END;

    return ACE_RC_FAILURE;
}

static ace_rc_t applyDeleteLogRecord( oaContext           * aContext,
                                      oaJDBCApplierHandle * aHandle,
                                      oaLogRecordDelete   * aLogRecord )
{
    jobject         sPrepareStmtObject = NULL;
    jint           *sParamStatusArray  = NULL;

    ACE_TEST( getPreparedDeleteStatement( aContext,
                                          aHandle,
                                          aLogRecord,
                                          &sPrepareStmtObject )
              != ACE_RC_SUCCESS );
 
    ACE_TEST( bindDeleteStatement( aContext,
                                   aHandle,
                                   aLogRecord,
                                   sPrepareStmtObject )
              != ACE_RC_SUCCESS );
 
    if ( aLogRecord->mArrayDMLCount == 1 )
    {
        ACE_TEST( oaJNIPreparedStmtExecute( aContext, 
                                            &(aHandle->mJNIInterfaceHandle), 
                                            sPrepareStmtObject )
                  != ACE_RC_SUCCESS );
    }
    else
    {
        ACE_TEST( oaJNIPreparedStmtExecuteBatch( aContext, 
                                                 &(aHandle->mJNIInterfaceHandle), 
                                                 sPrepareStmtObject )
                  != ACE_RC_SUCCESS );
    }

    return ACE_RC_SUCCESS;
     
    ACE_EXCEPTION_END;
     
    sParamStatusArray = oaJNIGetDMLStatusArray( &(aHandle->mJNIInterfaceHandle) );
    
    setDMLResultArray( aHandle, sParamStatusArray );
    
    writeJDBCDMLLog( aHandle, 
                     (oaLogRecord *)aLogRecord, 
                     sParamStatusArray );
        
    oaJNIReleaseDMLStatusArray( &(aHandle->mJNIInterfaceHandle), sParamStatusArray );
     
    return ACE_RC_FAILURE;
}

ACP_INLINE ace_rc_t applyInsertLogRecordWithSkipProperty( oaContext           * aContext,
                                                          oaJDBCApplierHandle * aHandle,
                                                          oaLogRecordInsert   * aLogRecord )
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

ACP_INLINE ace_rc_t applyUpdateLogRecordWithSkipProperty( oaContext           * aContext,
                                                          oaJDBCApplierHandle * aHandle,
                                                          oaLogRecordUpdate   * aLogRecord )
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

ACP_INLINE ace_rc_t applyDeleteLogRecordWithSkipProperty( oaContext           * aContext,
                                                          oaJDBCApplierHandle * aHandle,
                                                          oaLogRecordDelete   * aLogRecord )
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

static ace_rc_t applyCommitLogRecord( oaContext           * aContext,
                                      oaJDBCApplierHandle * aHandle )
{
    if ( aHandle->mJNIInterfaceHandle.mCommitMode == ACP_FALSE )
    {
        ACE_TEST( oaJNIConnectionCommit( aContext, &(aHandle->mJNIInterfaceHandle)) );
    }
    else
    {
        /* Nothing to do */
    }
    return ACE_RC_SUCCESS;
    
    ACE_EXCEPTION_END;
    
    return ACE_RC_FAILURE;
}

static ace_rc_t applyAbortLogRecord( oaContext           * aContext,
                                     oaJDBCApplierHandle * aHandle )
{
    ACE_TEST( oaJNIConnectionRollback( aContext, &(aHandle->mJNIInterfaceHandle)) );

    return ACE_RC_SUCCESS;

    ACE_EXCEPTION_END;

    return ACE_RC_FAILURE;
}

/**
 * @breif  Log Record List를 JDBC를 이용하여 반영한다.
 *
 *         실패 시에도 로그만 남기고 계속 진행하므로, 결과를 반환하지 않는다.
 *
 * @param  aContext       Context
 * @param  aHandle        ALtibase Applier Handle
 * @param  aLogRecordList Log Record List
 */
ace_rc_t oaJDBCApplierApplyLogRecordList( oaContext           * aContext,
                                          oaJDBCApplierHandle * aHandle,
                                          acp_list_t          * aLogRecordList,
                                          oaLogSN               aPrevLastProcessedSN,
                                          oaLogSN             * aLastProcessedSN )
{
    acp_list_t  * sIterator   = NULL;
    acp_uint32_t  sRetryCount = 0;
    
    oaLogSN       sProcessedLogSN  = 0;
    oaLogRecord * sLogRecord       = NULL;

    ACP_LIST_ITERATE( aLogRecordList, sIterator )
    {
        sRetryCount = 0;

        sLogRecord = (oaLogRecord *)((acp_list_node_t *)sIterator)->mObj;

        oaJDBCInitializeDMLResultArray( aHandle->mDMLResultArray, aHandle->mBatchDMLMaxSize );
        
        aHandle->mJNIInterfaceHandle.mIsSkipList = ACP_FALSE;

        while ( oaJDBCApplierApplyLogRecord( aContext,
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

                ACE_TEST_RAISE( ( aHandle->mSkipError == 0 ) &&
                                ( aHandle->mJNIInterfaceHandle.mIsSkipList == ACP_FALSE ),
                                ERR_RETRY_END );
                ACE_TEST_RAISE( ( aHandle->mSkipError == 1 ) &&
                                ( aHandle->mJNIInterfaceHandle.mIsSkipList == ACP_TRUE  ),
                                ERR_RETRY_END );
                break;
            }
            
            aHandle->mJNIInterfaceHandle.mIsSkipList = ACP_FALSE;
        }

        sProcessedLogSN = sLogRecord->mCommon.mSN;

        oaAlaLogConverterResetLogRecord( sLogRecord );
    }

    *aLastProcessedSN = sProcessedLogSN;

    return ACE_RC_SUCCESS;

    ACE_EXCEPTION( ERR_RETRY_END )
    {        
        /* applyAbortLogRecord 는 두번 실행 될 수 있는데 두번 실행되어도 문제가 발생하지 않는다. */
        oaLogMessage( OAM_MSG_DUMP_LOG, "LogRecord apply aborted" );
        (void)applyAbortLogRecord( aContext, aHandle );

        oaLogMessage( OAM_ERR_APPLY_ERROR, aHandle->mErrorRetryCount , "Other Database" );
    }

    ACE_EXCEPTION_END;
    
    *aLastProcessedSN = sProcessedLogSN;

    return ACE_RC_FAILURE;
}

ace_rc_t oaJDBCApplierApplyLogRecord( oaContext           * aContext,
                                      oaJDBCApplierHandle * aHandle,
                                      oaLogRecord         * aLogRecord )
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

    writeJDBCDumpTypeLog( aHandle, aLogRecord );

    return ACE_RC_FAILURE;
}

void oaJDBCInitializeDMLResultArray( acp_bool_t * aDMLResultArray, acp_uint32_t aArrayDMLMaxSize )
{
    acp_uint32_t    i       = 0;
    
    for ( i = 0 ; i < aArrayDMLMaxSize ; i++ )
    {
        aDMLResultArray[i] = ACP_FALSE;
    }
}

static ace_rc_t openFile( oaContext          * aContext,
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

static void closeFile( acp_std_file_t * aFile )
{
    (void)acpStdClose( aFile );
}

static acp_bool_t isSQLEndLineAndTrim( acp_char_t* aBuffer )
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

static ace_rc_t readSQLFile( oaContext          * aContext,
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

static ace_rc_t executeLogInSqlFile( oaContext           * aContext, 
                                     oaJDBCApplierHandle * aHandle,
                                     acp_std_file_t      * aFile )
{
    acp_bool_t      sIsSQLEnd = ACP_FALSE;
    acp_bool_t      sIsEOF = ACP_FALSE;
    acp_bool_t      sIsCreateStmt = ACP_FALSE;
    
    ACP_STR_DECLARE_DYNAMIC( sQueryStr );
    ACP_STR_INIT_DYNAMIC( sQueryStr, 
                          OA_ADAPTER_SQL_READ_BUFF_SIZE + 1, 
                          OA_ADAPTER_SQL_READ_BUFF_SIZE + 1 );
    
    ACE_TEST( oaJNICreateStatement( aContext, 
                                    &(aHandle->mJNIInterfaceHandle) ) 
              != ACE_RC_SUCCESS );
    sIsCreateStmt = ACP_TRUE;
    
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
            ACE_TEST( oaJNIStmtExecute( aContext, 
                                        &(aHandle->mJNIInterfaceHandle),
                                        &sQueryStr ) 
                      != ACE_RC_SUCCESS );
        }
        else
        {   
            /* Nothing to do */
        }
    }
    
    sIsCreateStmt = ACP_FALSE;
    ACE_TEST( oaJNIStatementClose( aContext, 
                                   &(aHandle->mJNIInterfaceHandle) ) 
              != ACE_RC_SUCCESS );
    
    ACP_STR_FINAL( sQueryStr );
    
    return ACE_RC_SUCCESS;

    ACE_EXCEPTION_END;

    if ( sIsCreateStmt == ACP_TRUE )
    {
         (void)oaJNIStatementClose( aContext, 
                                    &(aHandle->mJNIInterfaceHandle) );        
    }
    
    ACP_STR_FINAL( sQueryStr );
    
    return ACE_RC_FAILURE;
}

ace_rc_t oaJDBCExecuteLogInSql( oaContext * aContext, oaJDBCApplierHandle * aHandle )
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

static ace_rc_t readSkipErrorList( oaContext           * aContext,
                                   acp_std_file_t      * aFile,
                                   oaJDBCApplierHandle * aHandle,
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

static ace_rc_t trimSkipError( oaContext            * aContext,
                               oaJDBCApplierHandle  * aHandle, 
                               acp_str_t            * aStr )
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
        
        ACE_TEST_RAISE( ( j != 0 ) && ( j != SQLSTATE_STR_LEN ),
                        ERROR_INVALID_SKIP_LIST );
        
        acpStrResetLength( aStr );

    }
    
    return ACE_RC_SUCCESS;

    ACE_EXCEPTION( ERROR_INVALID_SKIP_LIST )
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
    
    return ACE_RC_FAILURE;
}

static ace_rc_t setSkipError( oaContext            * aContext,
                              oaJDBCApplierHandle  * aHandle,
                              acp_list_t           * aLogRecordList,
                              acp_str_t            * aStr )
{
    acp_list_node_t * sSkipErrorNode    = NULL;
    acp_char_t      * sSkipError        = NULL;
    acp_rc_t          sAcpRC            = ACP_RC_SUCCESS;
    ace_rc_t          sAceRC            = ACE_RC_SUCCESS;
    
    sAceRC = trimSkipError( aContext, 
                            aHandle, 
                            aStr );
    ACE_TEST( sAceRC != ACE_RC_SUCCESS );
    
    if ( acpStrGetLength( aStr ) != 0 )
    {
        sAcpRC = acpMemCalloc( (void **)&sSkipErrorNode,
                               1,
                               ACI_SIZEOF(acp_list_node_t) );
        ACE_TEST_RAISE( ACP_RC_NOT_SUCCESS(sAcpRC), ERROR_MEM_CALLOC );
        
        sAcpRC = acpMemCalloc( (void **)&sSkipError,
                               (SQLSTATE_STR_LEN + 1),
                               ACI_SIZEOF(acp_char_t) );
        ACE_TEST_RAISE( ACP_RC_NOT_SUCCESS(sAcpRC), ERROR_MEM_CALLOC );
        
        sAcpRC = acpCStrCpy( sSkipError,
                             ACI_SIZEOF(sSkipError),
                             acpStrGetBuffer( aStr ),
                             acpStrGetLength( aStr ) );
        ACE_TEST_RAISE( ACP_RC_NOT_SUCCESS(sAcpRC), ERROR_INVALID_SKIP_ERR );
        
        acpListInitObj( sSkipErrorNode, (void *)sSkipError );
        acpListAppendNode( aLogRecordList, sSkipErrorNode );
    }
    else
    {
        /* Nothing to do */
    }
    
    return ACE_RC_SUCCESS;

    ACE_EXCEPTION( ERROR_MEM_CALLOC )
    {
        oaLogMessage( OAM_ERR_MEM_CALLOC );
    }
    ACE_EXCEPTION( ERROR_INVALID_SKIP_ERR )
    {
        oaLogMessage( OAM_MSG_DUMP_LOG, "setSkipError ERROR_INVALID_SKIP_ERR ");

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

static ace_rc_t setSkipErrorList( oaContext           * aContext,
                                  oaJDBCApplierHandle * aHandle,
                                  acp_list_t          * aLogRecordList,
                                  acp_std_file_t      * aFile )
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

ace_rc_t oaJDBCInitializeSkipErrorList( oaContext * aContext, oaJDBCApplierHandle * aHandle )
{
    acp_std_file_t      sFile = {NULL};
    acp_bool_t          sIsExistFile = ACP_FALSE;
    acp_bool_t          sIsOpenFile = ACP_FALSE;
    acp_char_t        * sFileName;
    acp_list_t        * sListHead = &(aHandle->mJNIInterfaceHandle.mSkipErrorList);

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

    return ACE_RC_FAILURE;
}

void oaJDBCFinalizeSkipErrorList( oaJDBCApplierHandle * aHandle )
{
    acp_list_node_t   * sNodeNext   = NULL;
    acp_list_node_t   * sIterator   = NULL;
    acp_list_t        * sListHead   =  &(aHandle->mJNIInterfaceHandle.mSkipErrorList);
    
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

