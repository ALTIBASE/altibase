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
 * $Id$
 **********************************************************************/
#include <sdm.h>
#include <sdiZookeeper.h>

extern mtfModule sdfCreateMetaModule;
extern mtfModule sdfResetMetaNodeIdModule;
extern mtfModule sdfSetLocalNodeModule;
extern mtfModule sdfSetReplicationModule;
extern mtfModule sdfSetNodeModule;
extern mtfModule sdfUnsetNodeModule;
extern mtfModule sdfSetShardTableModule;
extern mtfModule sdfSetShardTableGlobalModule;
extern mtfModule sdfSetShardProcedureModule;
extern mtfModule sdfSetShardProcedureGlobalModule;
extern mtfModule sdfSetShardRangeModule;
extern mtfModule sdfSetShardRangeGlobalModule;
extern mtfModule sdfSetShardPartitionNodeModule;
extern mtfModule sdfSetShardPartitionNodeGlobalModule;
extern mtfModule sdfSetShardCloneModule;
extern mtfModule sdfSetShardCloneGlobalModule;
extern mtfModule sdfSetShardSoloModule;
extern mtfModule sdfSetShardSoloGlobalModule;
extern mtfModule sdfResetShardResidentNodeModule;
extern mtfModule sdfResetShardPartitionNodeModule;
extern mtfModule sdfUnsetShardTableModule;
extern mtfModule sdfUnsetShardTableGlobalModule;
extern mtfModule sdfUnsetShardTableByIDModule;
extern mtfModule sdfUnsetShardTableByIDGlobalModule;
extern mtfModule sdfUnsetOldSmnModule;
extern mtfModule sdfExecImmediateModule;
// PROJ-2638
extern mtfModule sdfShardNodeName;
extern mtfModule sdfShardKey;
extern mtfModule sdfShardConditionModule;
extern mtfModule sdfResetNodeInternalModule;
extern mtfModule sdfResetNodeExternalModule;
extern mtfModule sdfShardSetSMNModule;

extern mtfModule sdfBackupReplicaSetsModule;
extern mtfModule sdfResetReplicaSetsModule;

extern mtfModule sdfSetShardTableShardKeyModule;
extern mtfModule sdfSetShardTableSoloModule;
extern mtfModule sdfSetShardTableCloneModule;
extern mtfModule sdfSetShardProcedureShardKeyModule;
extern mtfModule sdfSetShardProcedureSoloModule;
extern mtfModule sdfSetShardProcedureCloneModule;

extern mtfModule sdfSetReplicaSCNModule;

extern mtfModule sdfTouchMetaModule;

const mtfModule* sdf::extendedFunctionModules[] =
{
    &sdfCreateMetaModule,
    &sdfResetMetaNodeIdModule,
    &sdfSetLocalNodeModule,
    &sdfSetReplicationModule,
    &sdfSetNodeModule,
    &sdfUnsetNodeModule,
    &sdfSetShardTableModule,
    &sdfSetShardTableGlobalModule,
    &sdfSetShardProcedureModule,
    &sdfSetShardProcedureGlobalModule,
    &sdfSetShardRangeModule,
    &sdfSetShardRangeGlobalModule,
    &sdfSetShardPartitionNodeModule,
    &sdfSetShardPartitionNodeGlobalModule,
    &sdfSetShardCloneModule,
    &sdfSetShardCloneGlobalModule,
    &sdfSetShardSoloModule,
    &sdfSetShardSoloGlobalModule,
    &sdfResetShardResidentNodeModule,
    &sdfResetShardPartitionNodeModule,
    &sdfUnsetShardTableModule,
    &sdfUnsetShardTableGlobalModule,
    &sdfUnsetShardTableByIDModule,
    &sdfUnsetShardTableByIDGlobalModule,
    &sdfUnsetOldSmnModule,
    &sdfExecImmediateModule,
    &sdfResetNodeInternalModule,
    &sdfResetNodeExternalModule,
    &sdfShardSetSMNModule,
    &sdfBackupReplicaSetsModule,
    &sdfResetReplicaSetsModule,
    &sdfSetShardTableShardKeyModule,
    &sdfSetShardTableSoloModule,
    &sdfSetShardTableCloneModule,
    &sdfSetShardProcedureShardKeyModule,
    &sdfSetShardProcedureSoloModule,
    &sdfSetShardProcedureCloneModule,
    &sdfSetReplicaSCNModule,
    &sdfTouchMetaModule,
    NULL
};

const mtfModule* sdf::extendedFunctionModules2[] =
{
    &sdfShardNodeName,
    &sdfShardKey,
    &sdfShardConditionModule,
    NULL
};

IDE_RC sdf::checkShardObjectSchema( qcStatement * aStatement,
                                    SChar       * aSqlStr )
{    
    sdiClientInfo  * sClientInfo  = NULL;
    sdiConnectInfo * sConnectInfo = NULL;
    SChar            sStrResult[SDF_DIGEST_LEN + 1] = {0,};
    idBool           sFetchResult = ID_FALSE;
    UInt             sExecCount = 0;
    SChar            sSchemaHashVal1[SDF_DIGEST_LEN + 1] = {0,};
    SChar            sSchemaHashVal2[SDF_DIGEST_LEN + 1] = {0,};
    UInt             i = 0;
            
    sClientInfo = aStatement->session->mQPSpecific.mClientInfo;
            
    if ( sClientInfo != NULL )
    {
        sConnectInfo = sClientInfo->mConnectInfo;
            
        for ( i = 0; i < sClientInfo->mCount; i++, sConnectInfo++ )
        {                
            IDE_TEST( sdi::shardExecDirect( aStatement,
                                            sConnectInfo->mNodeName,
                                            (SChar*)aSqlStr,
                                            (UInt) idlOS::strlen( aSqlStr ),
                                            SDI_INTERNAL_OP_NORMAL,
                                            &sExecCount,
                                            NULL,
                                            sStrResult,
                                            ID_SIZEOF(sStrResult),
                                            &sFetchResult )
                      != IDE_SUCCESS );

            IDE_TEST_RAISE( sFetchResult == ID_FALSE, ERR_NO_DATA_FOUND );
                
            if ( i == 0 )
            {
                idlOS::snprintf( sSchemaHashVal1, SDF_DIGEST_LEN + 1, "%s", sStrResult  );

            }
            else
            {
                idlOS::snprintf( sSchemaHashVal2, SDF_DIGEST_LEN + 1, "%s", sStrResult  );
                    
                IDE_TEST_RAISE( idlOS::strMatch( sSchemaHashVal1, SDF_DIGEST_LEN + 1,
                                                 sSchemaHashVal2, SDF_DIGEST_LEN + 1 )
                                != 0, ERR_DIFF_TABLE_SCHEMA );
            }
        }
    }
    else
    {
        IDE_RAISE( ERR_NODE_NOT_EXIST );
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_DIFF_TABLE_SCHEMA )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_DIFFERENT_SCHEMA ));
    }
    IDE_EXCEPTION( ERR_NODE_NOT_EXIST )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_NODE_NOT_EXIST ) );
    }
    IDE_EXCEPTION( ERR_NO_DATA_FOUND )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdf::checkShardObjectSchema",
                                  "get row count fetch no data found" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdf::makeArgumentList( qcStatement    * aStatement,
                              SChar          * aArgumentListStr,
                              sdfArgument   ** aArgumentList,
                              UInt           * aArgumentCnt,
                              SChar          * aDefaultPartitionNameStr )
{
    SChar       * sTokenPtr = NULL;
    SChar       * sNextTokenPtr = NULL;
    sdfArgument * sArgument = NULL;
    UInt          sArgumentCnt = 0;
    SChar         sBuffer[QC_MAX_OBJECT_NAME_LEN + 1];
    SChar         sArgStr[32000];
    idBool        sHasDefault = ID_FALSE;
    SChar       * sDefaultPartitionNameStr;

    // BUG-49002
    // 분산범위 수가 SDI_RANGE_MAX_COUNT(1000)을 넘는지 검사한다.
    // Table은 default partition 제외하고 계산한다.
    // Procedure는 default를 별도로 설정하므로 그대로 비교한다.
    if ( aDefaultPartitionNameStr != NULL )
    {
        if ( idlOS::strncmp( aDefaultPartitionNameStr, SDM_NA_STR, (QC_MAX_OBJECT_NAME_LEN + 1)) == 0 )
        {
            sDefaultPartitionNameStr = NULL;
        }
        else
        {
            sDefaultPartitionNameStr = aDefaultPartitionNameStr;
        }
    }
    else
    {
        sDefaultPartitionNameStr = aDefaultPartitionNameStr;
    }

    IDE_TEST( stringRemoveCrlf( sArgStr,
                                aArgumentListStr )
              != IDE_SUCCESS );
            
    sTokenPtr = idlOS::strtok_r( sArgStr, " ", &sNextTokenPtr );

    while( sTokenPtr )
    {
        sArgumentCnt++;
            
        IDE_TEST( QC_QMX_MEM(aStatement)->alloc( ID_SIZEOF( sdfArgument ),
                                                 (void**)&sArgument )
                  != IDE_SUCCESS);
            
        sArgument->mArgument1 = NULL;
        sArgument->mArgument2 = NULL;
        sArgument->mIsCheck = ID_FALSE;
        sArgument->mIsFlush = ID_FALSE;
        sArgument->mNext = NULL;

        // VALUE OR PARTITION NAME
        IDE_TEST( stringTrim( sBuffer,
                              sTokenPtr )
                  != IDE_SUCCESS );

        IDE_TEST( QC_QMX_MEM(aStatement)->alloc( idlOS::strlen(sBuffer) + 1,
                                                 (void**)&(sArgument->mArgument1) )
                  != IDE_SUCCESS);

        idlOS::snprintf( sArgument->mArgument1, idlOS::strlen(sBuffer) + 1,
                         "%s ",
                         sBuffer );

        if ( sDefaultPartitionNameStr != NULL )
        {
            if ( idlOS::strncmp(sDefaultPartitionNameStr, sBuffer, (QC_MAX_OBJECT_NAME_LEN + 1)) == 0 )
            {
                sHasDefault = ID_TRUE;
            }
        }
       
        // NODE NAME
        sTokenPtr = idlOS::strtok_r(NULL, ",", &sNextTokenPtr);

        IDE_TEST_RAISE( sTokenPtr == NULL, ERR_INVALID_FUNCTION_ARGUMENT );
        
        IDE_TEST( stringTrim( sBuffer,
                              sTokenPtr )
                  != IDE_SUCCESS );

        IDE_TEST( QC_QMX_MEM(aStatement)->alloc( idlOS::strlen(sBuffer) + 1,
                                                 (void**)&(sArgument->mArgument2) )
                  != IDE_SUCCESS);
        
        idlOS::snprintf( sArgument->mArgument2, idlOS::strlen(sBuffer) + 1,
                         "%s ",
                         sBuffer );

        sTokenPtr = idlOS::strtok_r(NULL, " ", &sNextTokenPtr);
        
        if ( *aArgumentList == NULL )
        {
            *aArgumentList = sArgument;
        }
        else
        {
            sArgument->mNext = *aArgumentList;
            *aArgumentList = sArgument;
        }
    }
    
    *aArgumentCnt = sArgumentCnt;

    IDE_TEST_RAISE( ( sArgumentCnt > ((sHasDefault == ID_TRUE) ? (SDI_RANGE_MAX_COUNT+1):(SDI_RANGE_MAX_COUNT)) ),
                    RANGE_COUNT_OVERFLOW );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT )
    {
        IDE_SET(ideSetErrorCode( sdERR_ABORT_SDF_INVALID_ARGUMENT,
                                 sArgument->mArgument1 ));
    }
    IDE_EXCEPTION( RANGE_COUNT_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_RANGE_OVERFLOW ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC sdf::stringTrim( SChar * aDestString,
                        SChar * aSrcString )
{    
    SChar * sStart;
    SChar * sChar;
    SInt    sSize;
    SInt    i = 0;
    SInt    j = 0;

    sSize = idlOS::strlen(aSrcString);

    //---------------------------------------
    // ltrim
    //---------------------------------------
    for ( sChar = aSrcString, i = 0;
          ( idlOS::pdl_isspace(*sChar) != 0 ) && ( i < sSize );
          sChar++ )
    {
        i++;
    }

    sSize = sSize - i;
    sStart = sChar;

    //---------------------------------------
    // rtrim
    //---------------------------------------
    IDE_TEST_RAISE( sSize <= 0, ERR_INVALID_SIZE );

    for ( sChar = sStart + sSize - 1, i = 0;
          ( idlOS::pdl_isspace(*sChar) != 0 ) && ( i < sSize );
          sChar-- )
    {
        i++;
    }

    sSize = sSize - i;

    IDE_TEST_RAISE( sSize <= 0, ERR_INVALID_SIZE );
    
    for (i = 0, j = 0; i < sSize; i++, j++)
    {
        aDestString[j] = sStart[i];
    }
    aDestString[i] = '\0';

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_SIZE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "sdf::stringTrim",
                                  "invalid size" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdf::stringRemoveCrlf( SChar * aDestString,
                              SChar * aSrcString )
{    
    SChar   *sTokenPtr = NULL;
    UInt    sTokenLen = 0;
    
    sTokenPtr = aSrcString;

    while (1)
    {
        for( ;
             ( ( *sTokenPtr != '\t' ) &&
               ( *sTokenPtr != '\r' ) &&
               ( *sTokenPtr != '\n' ) &&
               ( *sTokenPtr != '\0' ) );
             sTokenPtr++ )
        {
            aDestString[sTokenLen] = *sTokenPtr;
            sTokenLen++;
        }
        
        if ( *sTokenPtr == '\0' )
        {
            break;
        }
        else
        {
            sTokenPtr++;
        }
    }
    
    aDestString[sTokenLen] = '\0';
    
    return IDE_SUCCESS;
}

IDE_RC sdf::validateNode( qcStatement    * aStatement,
                          sdfArgument    * aArgumentList,
                          SChar          * aNodeName,
                          sdiSplitMethod   aSplitMethod )
{
    sdiClientInfo  * sClientInfo = NULL;
    sdiConnectInfo * sConnectInfo = NULL;
    sdfArgument    * sIterator = NULL;
    idBool           sFound = ID_FALSE;
    UInt             i = 0;
    
    IDE_TEST( sdi::checkShardLinker( aStatement ) != IDE_SUCCESS );
    
    sClientInfo = aStatement->session->mQPSpecific.mClientInfo;
    
    switch ( aSplitMethod )
    {
        case SDI_SPLIT_HASH:
        case SDI_SPLIT_RANGE:
        case SDI_SPLIT_LIST:           
            IDE_DASSERT( aArgumentList != NULL );
            
            for ( sIterator = aArgumentList;
                  sIterator != NULL;
                  sIterator = sIterator->mNext )
            {
                sFound = ID_FALSE;
        
                if ( sClientInfo != NULL )
                {
                    sConnectInfo = sClientInfo->mConnectInfo;
            
                    for ( i = 0; i < sClientInfo->mCount; i++, sConnectInfo++ )
                    {            
                        if ( idlOS::strMatch( sConnectInfo->mNodeName,
                                              idlOS::strlen(sConnectInfo->mNodeName) + 1,
                                              sIterator->mArgument2,
                                              idlOS::strlen(sIterator->mArgument2) + 1 )
                             == 0 )
                        {
                            sFound = ID_TRUE;
                            break;
                        }
                    }
            
                    IDE_TEST_RAISE( sFound != ID_TRUE, ERR_NODE_NOT_FOUND );
                }
                else
                {
                    IDE_RAISE( ERR_NODE_NOT_EXIST );
                }
            }
            break;
        case SDI_SPLIT_CLONE:
        case SDI_SPLIT_SOLO:
            if ( sClientInfo != NULL )
            {
                sConnectInfo = sClientInfo->mConnectInfo;

                sFound = ID_FALSE;
         
                for ( i = 0; i < sClientInfo->mCount; i++, sConnectInfo++ )
                {            
                    if ( idlOS::strMatch( sConnectInfo->mNodeName,
                                          idlOS::strlen(sConnectInfo->mNodeName) + 1,
                                          aNodeName,
                                          idlOS::strlen(aNodeName) + 1 )
                         == 0 )
                    {
                        sFound = ID_TRUE;
                        break;
                    }
                }
            
                IDE_TEST_RAISE( sFound != ID_TRUE, ERR_NODE_NOT_FOUND_CLONE_SOLO );
            }
            else
            {
                IDE_RAISE( ERR_NODE_NOT_EXIST );
            }
            break;
        default:
            IDE_RAISE(ERR_INVALID_SHARD_SPLIT_METHOD_NAME);
    }            

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_SHARD_SPLIT_METHOD_NAME );
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_INVALID_SHARD_SPLIT_METHOD_NAME ) );
    }
    IDE_EXCEPTION( ERR_NODE_NOT_EXIST )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_NODE_NOT_EXIST ) );
    }
    IDE_EXCEPTION( ERR_NODE_NOT_FOUND )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDI_INVALID_NODE_NAME2,
                                  sIterator->mArgument2 ));
    }
    IDE_EXCEPTION( ERR_NODE_NOT_FOUND_CLONE_SOLO )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDI_INVALID_NODE_NAME2,
                                  aNodeName ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdf::makeReplName( SChar * aDestReplName,
                          SChar * aSrcReplName )
{
    IDE_TEST_RAISE(
        ( idlOS::strlen(aSrcReplName) +
          idlOS::strlen(SDI_BACKUP_TABLE_PREFIX) ) > SDI_REPLICATION_NAME_MAX_SIZE,
        ERR_NAME_TOO_LONG );

    idlOS::snprintf( aDestReplName,
                     SDI_REPLICATION_NAME_MAX_SIZE + 1,
                     "%s%s",
                     aSrcReplName,
                     SDI_BACKUP_TABLE_PREFIX );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_NAME_TOO_LONG );
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_TOO_LONG_REPL_NAME ) );
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC sdf::executeRemoteSQLWithNewTrans( qcStatement * aStatement,
                                          SChar       * aNodeName,
                                          SChar       * aSqlStr,
                                          idBool        aIsDDLTimeOut )
{
    sdiLocalMetaInfo   sLocalMetaInfo;
    SChar            * sSqlStr = NULL;
    
    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS );

    IDE_TEST( sdi::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );

    if ( aIsDDLTimeOut == ID_TRUE )
    {
        IDE_TEST( sdi::shardExecTempDDLWithNewTrans( aStatement,
                                                     aNodeName,
                                                     aSqlStr,
                                                     idlOS::strlen(aSqlStr) )
                  != IDE_SUCCESS );
    }
    else
    {
        if ( aNodeName != NULL )
        {
            if ( idlOS::strncmp( sLocalMetaInfo.mNodeName, 
                                 aNodeName,
                                 SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
            {            
                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH, "%s ", aSqlStr );
            }
            else
            {
                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                 "EXEC DBMS_SHARD.EXECUTE_IMMEDIATE( '%s', '"QCM_SQL_STRING_SKIP_FMT"' ) ",
                                 aSqlStr,
                                 aNodeName );
            }
        }
        else
        {
            idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                             "EXEC DBMS_SHARD.EXECUTE_IMMEDIATE( '%s', NULL ) ",
                             aSqlStr );
        }
    
        IDE_TEST( sdi::shardExecTempDMLOrDCLWithNewTrans( aStatement,
                                                          sSqlStr )
                  != IDE_SUCCESS );   
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC sdf::remoteSQLWithNewTrans( qcStatement * aStatement,
                                   SChar       * aSqlStr )
{
    sdiClientInfo  * sClientInfo  = NULL;
    sdiConnectInfo * sConnectInfo = NULL;
    smiStatement   * sOldStmt = NULL;
    smiStatement     sSmiStmt;
    UInt             sSmiStmtFlag = 0;
    SInt             sState = 0;
    UInt             i = 0;
    
    sSmiStmtFlag = SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR;
    sOldStmt                = QC_SMI_STMT(aStatement);
    QC_SMI_STMT(aStatement) = &sSmiStmt;
    sState = 1;

    IDE_TEST( sSmiStmt.begin( aStatement->mStatistics,
                              sOldStmt,
                              sSmiStmtFlag )
              != IDE_SUCCESS );
    sState = 2;

    IDE_TEST( sdi::checkShardLinker( aStatement ) != IDE_SUCCESS );

    sClientInfo = aStatement->session->mQPSpecific.mClientInfo;

    sConnectInfo = sClientInfo->mConnectInfo;
    
    for ( i = 0; i < sClientInfo->mCount; i++, sConnectInfo++ )
    {
        IDE_TEST( executeRemoteSQLWithNewTrans( aStatement,
                                                sConnectInfo->mNodeName,
                                                aSqlStr,
                                                ID_TRUE )
                  != IDE_SUCCESS );
    }
    
    sState = 1;
    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );

    sState = 0;
    QC_SMI_STMT(aStatement) = sOldStmt;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    IDE_PUSH();
        
    switch ( sState )
    {
        case 2:
            if ( sSmiStmt.end(SMI_STATEMENT_RESULT_FAILURE) != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_SD_1);
            }
            else
            {
                /* Nothing to do */
            }
        case 1:
            QC_SMI_STMT(aStatement) = sOldStmt;
        default:
            break;
    }

    IDE_POP();
    
    return IDE_FAILURE;
}

IDE_RC sdf::createReplicationForInternal( qcStatement * aStatement,
                                          SChar       * aNodeName,
                                          SChar       * aReplName,
                                          SChar       * aHostIP,
                                          UInt          aPortNo )
{
    sdiLocalMetaInfo   sLocalMetaInfo;
    SChar              sSqlStr[SDF_QUERY_LEN];

    IDE_TEST( sdi::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );

    if ( idlOS::strncmp( sLocalMetaInfo.mNodeName, 
                         aNodeName,
                         SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
    {
        idlOS::snprintf( sSqlStr, SDF_QUERY_LEN,
                         "CREATE REPLICATION "QCM_SQL_STRING_SKIP_FMT" WITH '%s',%"ID_INT32_FMT" ",
                         aReplName,
                         aHostIP,
                         aPortNo );
    }
    else
    {
        idlOS::snprintf( sSqlStr, SDF_QUERY_LEN,
                         "CREATE REPLICATION "QCM_SQL_STRING_SKIP_FMT" WITH ''%s'',%"ID_INT32_FMT" ",
                         aReplName,
                         aHostIP,
                         aPortNo );
    }
        
    IDE_TEST( executeRemoteSQLWithNewTrans( aStatement,
                                            aNodeName,
                                            sSqlStr,
                                            ID_TRUE )
              != IDE_SUCCESS );

    idlOS::snprintf( sSqlStr, SDF_QUERY_LEN,
                     "DROP REPLICATION "QCM_SQL_STRING_SKIP_FMT"",
                     aReplName );
    
    (void)sdiZookeeper::addRevertJob( sSqlStr,
                                      aNodeName,
                                      ZK_REVERT_JOB_REPL_DROP );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdf::startReplicationForInternal( qcStatement * aStatement,
                                         SChar       * aNodeName,
                                         SChar       * aReplName,
                                         SInt          aReplParallelCnt )
{
    SChar sSqlStr[SDF_QUERY_LEN];
    
    idlOS::snprintf( sSqlStr, SDF_QUERY_LEN,
                     "ALTER REPLICATION "QCM_SQL_STRING_SKIP_FMT" SYNC ONLY PARALLEL %"ID_INT32_FMT" ",
                     aReplName,
                     aReplParallelCnt );
    
    IDE_TEST( executeRemoteSQLWithNewTrans( aStatement,
                                            aNodeName,
                                            sSqlStr,
                                            ID_FALSE )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdf::stopReplicationForInternal( qcStatement * aStatement,
                                        SChar       * aNodeName,
                                        SChar       * aReplName )
{
    SChar sSqlStr[SDF_QUERY_LEN];

    idlOS::snprintf( sSqlStr, SDF_QUERY_LEN,
                     "ALTER REPLICATION "QCM_SQL_STRING_SKIP_FMT" STOP ",
                     aReplName );
    
    IDE_TEST( executeRemoteSQLWithNewTrans( aStatement,
                                            aNodeName,
                                            sSqlStr,
                                            ID_FALSE )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdf::dropReplicationForInternal( qcStatement * aStatement,
                                        SChar       * aNodeName,
                                        SChar       * aReplName )
{
    SChar sSqlStr[SDF_QUERY_LEN];

    idlOS::snprintf( sSqlStr, SDF_QUERY_LEN,
                     "DROP REPLICATION "QCM_SQL_STRING_SKIP_FMT" ",
                     aReplName );
    
    IDE_TEST( executeRemoteSQLWithNewTrans( aStatement,
                                            aNodeName,
                                            sSqlStr,
                                            ID_TRUE )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdf::searchReplicaSet( qcStatement            * aStatement,
                              iduList                * aNodeInfoList,
                              sdiReplicaSetInfo      * aReplicaSetInfo,
                              SChar                  * aNodeNameStr,
                              SChar                  * aUserNameStr,
                              SChar                  * aTableNameStr,
                              SChar                  * aPartNameStr,
                              SInt                     aReplParallelCnt,
                              sdfReplicationFunction   aReplicationFunction,
                              sdfReplDirectionalRole   aRole,
                              UInt                     aNth )
{
    sdiLocalMetaInfo  * sTargetMetaInfo = NULL;
    SChar               sReplNameStr[SDI_REPLICATION_NAME_MAX_SIZE + 1];
    UInt                sCnt = 0 ;

    for ( sCnt = 0; sCnt < aReplicaSetInfo->mCount; sCnt++ )
    {
        switch (aRole)
        {
            case SDM_REPL_SENDER:
                if ( aNth == 0 ) /* First */
                {
                    if ( idlOS::strncmp( aReplicaSetInfo->mReplicaSets[sCnt].mFirstBackupNodeName,
                                         SDM_NA_STR,
                                         SDI_NODE_NAME_MAX_SIZE + 1)
                         != 0 )
                    {
                        sTargetMetaInfo =
                            sdiZookeeper::getNodeInfo( aNodeInfoList,
                                                       aReplicaSetInfo->mReplicaSets[sCnt].mFirstReplToNodeName );

                        IDE_TEST( sdf::makeReplName( sReplNameStr,
                                                     aReplicaSetInfo->mReplicaSets[sCnt].mFirstReplName )
                                  != IDE_SUCCESS );

                        switch ( aReplicationFunction )
                        {
                            case SDF_REPL_CREATE:                                
                                IDE_TEST( sdf::createReplicationForInternal(
                                              aStatement,
                                              aReplicaSetInfo->mReplicaSets[sCnt].mPrimaryNodeName,
                                              sReplNameStr,
                                              sTargetMetaInfo->mInternalRPHostIP,
                                              sTargetMetaInfo->mInternalRPPortNo )
                                          != IDE_SUCCESS );

                                break;
                            case SDF_REPL_ADD:
                                IDE_TEST( sdm::addReplTable( aStatement,
                                                             aReplicaSetInfo->mReplicaSets[sCnt].mPrimaryNodeName,
                                                             sReplNameStr,
                                                             aUserNameStr,
                                                             aTableNameStr,
                                                             aPartNameStr,
                                                             SDM_REPL_SENDER,
                                                             ID_TRUE )
                                          != IDE_SUCCESS );
                                
                                break;
                            case SDF_REPL_START:
                                IDE_TEST( sdf::startReplicationForInternal(
                                              aStatement,
                                              aReplicaSetInfo->mReplicaSets[sCnt].mPrimaryNodeName,
                                              sReplNameStr,
                                              aReplParallelCnt )
                                          != IDE_SUCCESS );
                                
                                break;
                            case SDF_REPL_STOP:
                                IDE_TEST( sdf::stopReplicationForInternal(
                                              aStatement,
                                              aReplicaSetInfo->mReplicaSets[sCnt].mPrimaryNodeName,
                                              sReplNameStr )
                                          != IDE_SUCCESS );
                                
                                break;
                            case SDF_REPL_DROP:
                                IDE_TEST( sdf::dropReplicationForInternal(
                                              aStatement,
                                              aReplicaSetInfo->mReplicaSets[sCnt].mPrimaryNodeName,
                                              sReplNameStr )
                                          != IDE_SUCCESS );
                                break;
                            default:
                                IDE_DASSERT(0);
                                break;
                        }
                    }
                }
                else if ( aNth == 1 ) /* Second */
                {
                    if ( idlOS::strncmp( aReplicaSetInfo->mReplicaSets[sCnt].mSecondBackupNodeName,
                                         SDM_NA_STR,
                                         SDI_NODE_NAME_MAX_SIZE + 1)
                         != 0 )
                    {
                        sTargetMetaInfo =
                            sdiZookeeper::getNodeInfo( aNodeInfoList,
                                                       aReplicaSetInfo->mReplicaSets[sCnt].mSecondReplToNodeName );

                        IDE_TEST( sdf::makeReplName( sReplNameStr,
                                                     aReplicaSetInfo->mReplicaSets[sCnt].mSecondReplName )
                                  != IDE_SUCCESS );
                        
                        switch ( aReplicationFunction )
                        {
                            case SDF_REPL_CREATE:                                
                                IDE_TEST( sdf::createReplicationForInternal(
                                              aStatement,
                                              aReplicaSetInfo->mReplicaSets[sCnt].mPrimaryNodeName,
                                              sReplNameStr,
                                              sTargetMetaInfo->mInternalRPHostIP,
                                              sTargetMetaInfo->mInternalRPPortNo )
                                          != IDE_SUCCESS );
                                break;
                            case SDF_REPL_ADD:
                                IDE_TEST( sdm::addReplTable( aStatement,
                                                             aReplicaSetInfo->mReplicaSets[sCnt].mPrimaryNodeName,
                                                             sReplNameStr,
                                                             aUserNameStr,
                                                             aTableNameStr,
                                                             aPartNameStr,
                                                             SDM_REPL_SENDER,
                                                             ID_TRUE )
                                          != IDE_SUCCESS );
                                break;
                            case SDF_REPL_START:
                                IDE_TEST( sdf::startReplicationForInternal(
                                              aStatement,
                                              aReplicaSetInfo->mReplicaSets[sCnt].mPrimaryNodeName,
                                              sReplNameStr,
                                              aReplParallelCnt )
                                          != IDE_SUCCESS );                                                                
                                break;
                            case SDF_REPL_STOP:
                                IDE_TEST( sdf::stopReplicationForInternal(
                                              aStatement,
                                              aReplicaSetInfo->mReplicaSets[sCnt].mPrimaryNodeName,
                                              sReplNameStr )
                                          != IDE_SUCCESS );
                                
                                break;
                            case SDF_REPL_DROP:
                                IDE_TEST( sdf::dropReplicationForInternal(
                                              aStatement,
                                              aReplicaSetInfo->mReplicaSets[sCnt].mPrimaryNodeName,
                                              sReplNameStr )
                                          != IDE_SUCCESS );
                                break;                                
                            default:
                                IDE_DASSERT(0);
                                break;
                        }
                    }
                }
                break;
            case SDM_REPL_RECEIVER:
                if ( aNth == 0 ) /* First */
                {
                    if ( idlOS::strncmp( aReplicaSetInfo->mReplicaSets[sCnt].mFirstBackupNodeName,
                                         SDM_NA_STR,
                                         SDI_NODE_NAME_MAX_SIZE + 1)
                         != 0 )
                    {
                        sTargetMetaInfo =
                            sdiZookeeper::getNodeInfo( aNodeInfoList,
                                                       aReplicaSetInfo->mReplicaSets[sCnt].mPrimaryNodeName );

                        IDE_TEST( sdf::makeReplName( sReplNameStr,
                                                     aReplicaSetInfo->mReplicaSets[sCnt].mFirstReplName )
                                  != IDE_SUCCESS );
                        
                        switch ( aReplicationFunction )
                        {
                            case SDF_REPL_CREATE:
                                IDE_TEST( sdf::createReplicationForInternal(
                                              aStatement,
                                              aNodeNameStr,
                                              sReplNameStr,
                                              sTargetMetaInfo->mInternalRPHostIP,
                                              sTargetMetaInfo->mInternalRPPortNo )
                                          != IDE_SUCCESS );
                                break;
                            case SDF_REPL_ADD:
                                IDE_TEST( sdm::addReplTable( aStatement,
                                                             aNodeNameStr,
                                                             sReplNameStr,
                                                             aUserNameStr,
                                                             aTableNameStr,
                                                             aPartNameStr,
                                                             SDM_REPL_RECEIVER,
                                                             ID_TRUE )
                                          != IDE_SUCCESS );
                                break;
                            case SDF_REPL_START:
                            case SDF_REPL_STOP:
                                break;
                            case SDF_REPL_DROP:
                                IDE_TEST( sdf::dropReplicationForInternal(
                                              aStatement,
                                              aNodeNameStr,
                                              sReplNameStr )
                                          != IDE_SUCCESS );
                                break;
                            default:
                                IDE_DASSERT(0);
                                break;
                        }
                    }
                }
                else if ( aNth == 1 ) /* Second */
                {
                    if ( idlOS::strncmp( aReplicaSetInfo->mReplicaSets[sCnt].mSecondBackupNodeName,
                                         SDM_NA_STR,
                                         SDI_NODE_NAME_MAX_SIZE + 1)
                         != 0 )
                    {
                        sTargetMetaInfo =
                            sdiZookeeper::getNodeInfo( aNodeInfoList,
                                                       aReplicaSetInfo->mReplicaSets[sCnt].mPrimaryNodeName );

                        IDE_TEST( sdf::makeReplName( sReplNameStr,
                                                     aReplicaSetInfo->mReplicaSets[sCnt].mSecondReplName )
                                  != IDE_SUCCESS );
                                
                        switch ( aReplicationFunction )
                        {
                            case SDF_REPL_CREATE:
                                IDE_TEST( sdf::createReplicationForInternal(
                                              aStatement,
                                              aNodeNameStr,
                                              sReplNameStr,
                                              sTargetMetaInfo->mInternalRPHostIP,
                                              sTargetMetaInfo->mInternalRPPortNo )
                                          != IDE_SUCCESS );
                                break;
                            case SDF_REPL_ADD:
                                IDE_TEST( sdm::addReplTable( aStatement,
                                                             aNodeNameStr,
                                                             sReplNameStr,
                                                             aUserNameStr,
                                                             aTableNameStr,
                                                             aPartNameStr,
                                                             SDM_REPL_RECEIVER,
                                                             ID_TRUE )
                                          != IDE_SUCCESS );
                                break;
                            case SDF_REPL_START:
                            case SDF_REPL_STOP:
                                break;
                            case SDF_REPL_DROP:
                                IDE_TEST( sdf::dropReplicationForInternal(
                                              aStatement,
                                              aNodeNameStr,
                                              sReplNameStr )
                                          != IDE_SUCCESS );
                                break;                                
                            default:
                                IDE_DASSERT(0);
                                break;
                        }
                    }
                }
                break;
            default:
                IDE_DASSERT(0);
                break;
        }
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC sdf::getCurrentAccessMode( qcStatement     * aStatement,
                                  SChar           * aTableNameStr,
                                  UInt            * aAccessMode )
{
    SChar           sSqlStr[SDF_QUERY_LEN];
    idBool          sRecordExist;
    mtdBigintType   sResult;
    smiStatement  * sOldStmt = NULL;
    smiStatement    sSmiStmt;
    UInt            sSmiStmtFlag = 0;
    SInt            sState = 0;

    sSmiStmtFlag = SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR;
    sOldStmt                = QC_SMI_STMT(aStatement);
    QC_SMI_STMT(aStatement) = &sSmiStmt;
    sState = 1;

    IDE_TEST( sSmiStmt.begin( aStatement->mStatistics,
                              sOldStmt,
                              sSmiStmtFlag )
              != IDE_SUCCESS );
    sState = 2;

    idlOS::snprintf( sSqlStr, SDF_QUERY_LEN,
                     "SELECT CAST( CASE2( ACCESS = 'W', 1, ACCESS = 'R',2, ACCESS = 'A',3) AS INTEGER ) A "
                     "FROM "
                     "SYSTEM_.SYS_TABLES_ WHERE TABLE_NAME = '%s' ",
                     aTableNameStr );
    
    IDE_TEST( qciMisc::runSelectOneRowforDDL( QC_SMI_STMT( aStatement ),
                                              sSqlStr,
                                              &sResult,
                                              &sRecordExist,
                                              ID_FALSE )
              != IDE_SUCCESS );
    
    IDE_TEST_RAISE( sRecordExist == ID_FALSE,
                    ERR_NO_DATA_FOUND );
    
    *aAccessMode = (UInt)sResult;
    
    sState = 1;
    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );

    sState = 0;
    QC_SMI_STMT(aStatement) = sOldStmt;

    return IDE_SUCCESS;
    
    IDE_EXCEPTION(ERR_NO_DATA_FOUND);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSX_NO_DATA_FOUND));
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    
    switch ( sState )
    {
        case 2:
            if ( sSmiStmt.end(SMI_STATEMENT_RESULT_FAILURE) != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_SD_1);
            }
            else
            {
                /* Nothing to do */
            }
        case 1:
            QC_SMI_STMT(aStatement) = sOldStmt;
        default:
            break;
    }

    IDE_POP();
    
    return IDE_FAILURE;
}

// SET_SHARD package subprogram 중 global로 각 node에 subprogram을
// 수행하는 경우 shard execute direct nested로 수행한다.
IDE_RC sdf::runRemoteQuery( qcStatement * aStatement,
                            SChar       * aSqlStr )
{
    sdiClientInfo * sClientInfo  = NULL;
    UInt            sExecCount = 0;

    IDE_TEST( sdi::checkShardLinker( aStatement ) != IDE_SUCCESS );

    sClientInfo = aStatement->session->mQPSpecific.mClientInfo;

    // 전체 Node 대상으로 Meta Update 실행
    if ( sClientInfo != NULL )
    {
        IDE_TEST( sdi::shardExecDirectNested( aStatement,
                                              NULL,
                                              (SChar*)aSqlStr,
                                              (UInt) idlOS::strlen( aSqlStr ),
                                              SDI_INTERNAL_OP_NORMAL,
                                              &sExecCount)
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( sExecCount == 0,
                        ERR_INVALID_SHARD_NODE );
    }
    else
    {
        IDE_RAISE( ERR_NODE_NOT_EXIST );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NODE_NOT_EXIST )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_NODE_NOT_EXIST ) );
    }
    IDE_EXCEPTION( ERR_INVALID_SHARD_NODE );
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_INVALID_SHARD_NODE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
