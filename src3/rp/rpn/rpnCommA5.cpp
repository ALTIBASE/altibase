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
 * $Id: rpnComm.cpp 16602 2006-06-09 01:39:30Z lswhh $
 **********************************************************************/

#include <idl.h>
#include <ide.h>

#include <rpDef.h>
#include <rpuProperty.h>
#include <rpcHBT.h>
#include <rpnComm.h>

IDE_RC rpnComm::sendVersionA5( void               * /*aHBTResource*/,
                               cmiProtocolContext * aProtocolContext,
                               idBool             * /*aExitFlag*/,
                               rpdVersion         * aReplVersion,
                               UInt                 /*aTimeoutSec*/ )
{
    cmiProtocolContext   sProtocolContext;
    cmiProtocol          sProtocol;
    UInt                 sStage      = 0;
    cmiLink            * sLink       = NULL;
    cmpArgRPVersion    * sArgVersion = NULL;

    IDE_TEST( cmiGetLinkForProtocolContext( aProtocolContext, &sLink )
              != IDE_SUCCESS );

    IDE_TEST( cmiInitializeProtocolContext( &sProtocolContext,
                                            CMI_PROTOCOL_MODULE( RP ),
                                            sLink )
              != IDE_SUCCESS );
    sStage = 1;

    cmiProtocolContextCopyA5Header( &sProtocolContext,
                                    &(aProtocolContext->mReadHeader) );

    /* Initialize Protocol */
    CMI_PROTOCOL_INITIALIZE( sProtocol, sArgVersion, RP, Version );
    sStage = 2;

    /* Version Set */
    sArgVersion->mVersion = aReplVersion->mVersion;

    /* Send Protocol */
    IDE_TEST( writeProtocol( NULL,
                             &sProtocolContext,
                             &sProtocol )
              != IDE_SUCCESS );

    /* Finalize Protocol */
    sStage = 1;
    IDE_TEST( cmiFinalizeProtocol( &sProtocol ) != IDE_SUCCESS );

    sStage = 0;
    IDE_TEST( cmiFinalizeProtocolContext( &sProtocolContext ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 2:
            (void)cmiFinalizeProtocol( &sProtocol );
            /* fall through */
        case 1:
            (void)cmiFinalizeProtocolContext( &sProtocolContext );
            /* fall through */
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC rpnComm::recvVersionA5( cmiProtocolContext * aProtocolContext,
                               idBool             * aExitFlag,
                               rpdVersion         * aReplVersion,
                               ULong                aTimeoutSec )
{
    cmiProtocol       sProtocol;
    cmpArgRPVersion * sVersionArg = NULL;
    SInt              sStage      = 0;
    
    IDE_TEST( readCmProtocol( aProtocolContext,
                              &sProtocol,
                              aExitFlag,
                              NULL,
                              aTimeoutSec )
              != IDE_SUCCESS );
    sStage = 1;
    
    IDE_TEST_RAISE( sProtocol.mOpID != CMI_PROTOCOL_OPERATION( RP, Version ),
                    ERR_CHECK_OPERATION_TYPE );

    sVersionArg = CMI_PROTOCOL_GET_ARG( sProtocol, RP, Version );

    aReplVersion->mVersion = sVersionArg->mVersion;

    sStage = 0;
    IDE_TEST( cmiFinalizeProtocol( &sProtocol ) != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CHECK_OPERATION_TYPE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_WRONG_OPERATION_TYPE,
                                  sProtocol.mOpID ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 1:
            (void)cmiFinalizeProtocol( &sProtocol );
        default:
            break;
    }
    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendMetaReplA5( void               * /*aHBTResource*/,
                                cmiProtocolContext * aProtocolContext,
                                idBool             * /*aExitFlag*/,
                                rpdReplications    * aRepl,
                                UInt                /*aTimeoutSec*/ )
{
    cmiProtocolContext   sProtocolContext;
    cmiProtocol          sProtocol;
    UInt                 sStage       = 0;
    cmiLink            * sLink        = NULL;
    cmpArgRPMetaRepl   * sArgMetaRepl = NULL;

    UInt sRepNameLen               = 0;
    UInt sOSInfoLen                = 0;
    UInt sDBCharSetLen             = 0;
    UInt sNationalCharSetLen       = 0;
    UInt sServerIDLen              = 0;
    UInt sRemoteFaultDetectTimeLen = 0;

    /* Validate Parameters */
    IDE_DASSERT( aRepl != NULL );

    IDE_TEST( cmiGetLinkForProtocolContext( aProtocolContext, &sLink )
              != IDE_SUCCESS );

    IDE_TEST( cmiInitializeProtocolContext( &sProtocolContext,
                                            CMI_PROTOCOL_MODULE( RP ),
                                            sLink )
              != IDE_SUCCESS );
    sStage = 1;

    cmiProtocolContextCopyA5Header( &sProtocolContext,
                                    &(aProtocolContext->mReadHeader) );

    /* Initialize Protocol */
    CMI_PROTOCOL_INITIALIZE( sProtocol, sArgMetaRepl, RP, MetaRepl );
    sStage = 2;

    /* Replication Information Set */
    sRepNameLen = idlOS::strlen( aRepl->mRepName );
    IDE_TEST( cmtVariableSetData( &(sArgMetaRepl->mRepName),
                                  (UChar *)aRepl->mRepName,
                                  sRepNameLen + 1 )
              != IDE_SUCCESS );
    sArgMetaRepl->mItemCnt            = aRepl->mItemCount;
    sArgMetaRepl->mRole               = aRepl->mRole;
    sArgMetaRepl->mConflictResolution = aRepl->mConflictResolution;
    sArgMetaRepl->mTransTblSize       = aRepl->mTransTblSize;
    sArgMetaRepl->mFlags              = aRepl->mFlags;
    sArgMetaRepl->mOptions            = aRepl->mOptions;
    sArgMetaRepl->mRPRecoverySN       = aRepl->mRPRecoverySN;
    sArgMetaRepl->mReplMode           = aRepl->mReplMode;
    sArgMetaRepl->mParallelID         = aRepl->mParallelID;
    sArgMetaRepl->mIsStarted          = aRepl->mIsStarted;

    sRemoteFaultDetectTimeLen = idlOS::strlen( aRepl->mRemoteFaultDetectTime );
    IDE_TEST( cmtVariableSetData( &(sArgMetaRepl->mRemoteFaultDetectTime),
                                  (UChar *)(aRepl->mRemoteFaultDetectTime),
                                  sRemoteFaultDetectTimeLen + 1 )
              != IDE_SUCCESS );

    /* PROJ-1915 OS, Endian, SMVersion, LFGCount, LogFileSize */
    sOSInfoLen = idlOS::strlen(aRepl->mOSInfo);
    IDE_TEST( cmtVariableSetData( &sArgMetaRepl->mOSInfo,
                                  (UChar *)aRepl->mOSInfo,
                                  sOSInfoLen + 1)
              != IDE_SUCCESS );
    sArgMetaRepl->mCompileBit     = aRepl->mCompileBit;
    sArgMetaRepl->mSmVersionID    = aRepl->mSmVersionID;
    sArgMetaRepl->mLFGCount       = aRepl->mLFGCount;
    sArgMetaRepl->mLogFileSize    = aRepl->mLogFileSize;

    /* CharSet Set */
    sDBCharSetLen = idlOS::strlen(aRepl->mDBCharSet);
    IDE_TEST( cmtVariableSetData( &sArgMetaRepl->mDBCharSet,
                                  (UChar *)aRepl->mDBCharSet,
                                  sDBCharSetLen + 1)
              != IDE_SUCCESS );
    sNationalCharSetLen = idlOS::strlen(aRepl->mNationalCharSet);
    IDE_TEST( cmtVariableSetData( &sArgMetaRepl->mNationalCharSet,
                                  (UChar *)aRepl->mNationalCharSet,
                                  sNationalCharSetLen + 1)
              != IDE_SUCCESS );

    /* Server ID */
    sServerIDLen = idlOS::strlen(aRepl->mServerID);
    IDE_TEST( cmtVariableSetData( &sArgMetaRepl->mServerID,
                                  (UChar *)aRepl->mServerID,
                                  sServerIDLen + 1)
              != IDE_SUCCESS );

    /* Send Protocol */
    IDE_TEST( writeProtocol( NULL,
                             &sProtocolContext,
                             &sProtocol )
              != IDE_SUCCESS );

    /* Finalize Protocol */
    sStage = 1;
    IDE_TEST( cmiFinalizeProtocol( &sProtocol ) != IDE_SUCCESS );

    sStage = 0;
    IDE_TEST( cmiFinalizeProtocolContext( &sProtocolContext ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 2:
            (void)cmiFinalizeProtocol( &sProtocol );
            /* fall through */
        case 1:
            (void)cmiFinalizeProtocolContext( &sProtocolContext );
            /* fall through */
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC rpnComm::recvMetaReplA5( cmiProtocolContext * aProtocolContext,
                                idBool             * aExitFlag,
                                rpdReplications    * aRepl,
                                ULong                aTimeoutSec )
{
    cmiProtocol sProtocol;
    cmpArgRPMetaRepl * sMetaRepl = NULL;
    UInt sStrLen = 0;
    SInt sStage = 0;
    
    IDE_TEST( readCmProtocol( aProtocolContext,
                              &sProtocol,
                              aExitFlag,
                              NULL,
                              aTimeoutSec )
              != IDE_SUCCESS );
    sStage = 1;
    
    /* Check Operation Type */
    IDE_TEST_RAISE( sProtocol.mOpID != CMI_PROTOCOL_OPERATION( RP, MetaRepl ),
                    ERR_CHECK_OPERATION_TYPE );

    /* Get Argument */
    sMetaRepl = CMI_PROTOCOL_GET_ARG( sProtocol, RP, MetaRepl );

    /* Get Replication Information */
    sStrLen = cmtVariableGetSize( &sMetaRepl->mRepName );
    IDE_TEST( cmtVariableGetData( &sMetaRepl->mRepName,
                                  (UChar *)aRepl->mRepName,
                                  sStrLen )
              != IDE_SUCCESS );
    aRepl->mItemCount          = sMetaRepl->mItemCnt;
    aRepl->mRole               = sMetaRepl->mRole;
    aRepl->mConflictResolution = sMetaRepl->mConflictResolution;
    aRepl->mTransTblSize       = sMetaRepl->mTransTblSize;
    aRepl->mFlags              = sMetaRepl->mFlags;
    aRepl->mOptions            = sMetaRepl->mOptions;
    aRepl->mRPRecoverySN       = sMetaRepl->mRPRecoverySN;
    aRepl->mReplMode           = sMetaRepl->mReplMode;
    aRepl->mParallelID         = sMetaRepl->mParallelID;
    aRepl->mIsStarted          = sMetaRepl->mIsStarted;

    sStrLen = cmtVariableGetSize( &sMetaRepl->mRemoteFaultDetectTime );
    IDE_TEST_RAISE( sStrLen > RP_DEFAULT_DATE_FORMAT_LEN + 1,
                    ERR_INVALID_CM_VARIABLE_SIZE );
    IDE_TEST( cmtVariableGetData( &sMetaRepl->mRemoteFaultDetectTime,
                                  (UChar *)aRepl->mRemoteFaultDetectTime,
                                  sStrLen )
              != IDE_SUCCESS );

    /* PROJ-1915 OS, Endian, SMVersion, LFGCount, LogFileSize 수신 */
    sStrLen = cmtVariableGetSize( &sMetaRepl->mOSInfo );
    IDE_TEST( cmtVariableGetData( &sMetaRepl->mOSInfo,
                                  (UChar *)aRepl->mOSInfo,
                                  sStrLen )
              != IDE_SUCCESS );
    aRepl->mCompileBit     = sMetaRepl->mCompileBit;
    aRepl->mSmVersionID    = sMetaRepl->mSmVersionID;
    aRepl->mLFGCount       = sMetaRepl->mLFGCount;
    aRepl->mLogFileSize    = sMetaRepl->mLogFileSize;

    /* CharSet Set */
    sStrLen = cmtVariableGetSize( &sMetaRepl->mDBCharSet );
    IDE_TEST( cmtVariableGetData( &sMetaRepl->mDBCharSet,
                                  (UChar *)aRepl->mDBCharSet,
                                  sStrLen )
              != IDE_SUCCESS );

    sStrLen = cmtVariableGetSize( &sMetaRepl->mNationalCharSet );
    IDE_TEST( cmtVariableGetData( &sMetaRepl->mNationalCharSet,
                                  (UChar *)aRepl->mNationalCharSet,
                                  sStrLen )
              != IDE_SUCCESS );

    /* Server ID */
    sStrLen = cmtVariableGetSize( &sMetaRepl->mServerID );
    IDE_TEST_RAISE( sStrLen > IDU_SYSTEM_INFO_LENGTH + 1,
                    ERR_INVALID_CM_VARIABLE_SIZE );
    IDE_TEST( cmtVariableGetData( &sMetaRepl->mServerID,
                                  (UChar *)aRepl->mServerID,
                                  sStrLen )
              != IDE_SUCCESS );

    /* Finalize Protocol */
    sStage = 0;
    IDE_TEST( cmiFinalizeProtocol( &sProtocol ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CHECK_OPERATION_TYPE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_WRONG_OPERATION_TYPE,
                                  sProtocol.mOpID ) );
        IDE_ERRLOG( IDE_RP_0 );
    }
    IDE_EXCEPTION( ERR_INVALID_CM_VARIABLE_SIZE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_INVALID_CM_VARIABLE_SIZE,
                                  "rpnComm::recvMetaReplA5" ) );
        IDE_ERRLOG( IDE_RP_0 );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 1:
            (void)cmiFinalizeProtocol( &sProtocol );
        default:
            break;
    }
    
    IDE_POP();

    return IDE_FAILURE;    
}

IDE_RC rpnComm::sendMetaReplTblA5( void               * /*aHBTResource*/,
                                   cmiProtocolContext * aProtocolContext,
                                   idBool             * /*aExitFlag*/,
                                   rpdMetaItem        * aItem,
                                   UInt                 /*aTimeoutSec*/ )
{
    cmiProtocolContext    sProtocolContext;
    cmiProtocol           sProtocol;
    UInt                  sStage          = 0;
    cmiLink             * sLink           = NULL;
    cmpArgRPMetaReplTbl * sArgMetaReplTbl = NULL;

    UInt sRepNameLen           = 0;
    UInt sLocalUserNameLen     = 0;
    UInt sLocalTableNameLen    = 0;
    UInt sLocalPartNameLen     = 0;
    UInt sRemoteUserNameLen    = 0;
    UInt sRemoteTableNameLen   = 0;
    UInt sRemotePartNameLen    = 0;
    UInt sPartCondMinValuesLen = 0;
    UInt sPartCondMaxValuesLen = 0;

    /* Validate Parameters */
    IDE_DASSERT( aItem != NULL );

    IDE_TEST( cmiGetLinkForProtocolContext( aProtocolContext, &sLink )
              != IDE_SUCCESS );

    IDE_TEST( cmiInitializeProtocolContext( &sProtocolContext,
                                            CMI_PROTOCOL_MODULE( RP ),
                                            sLink )
              != IDE_SUCCESS );
    sStage = 1;

    cmiProtocolContextCopyA5Header( &sProtocolContext,
                                    &(aProtocolContext->mReadHeader) );

    /* Initialize Protocol */
    CMI_PROTOCOL_INITIALIZE( sProtocol, sArgMetaReplTbl, RP, MetaReplTbl );
    sStage = 2;

    /* Replication Item Information Set */
    sRepNameLen = idlOS::strlen( aItem->mItem.mRepName );
    IDE_TEST( cmtVariableSetData( &sArgMetaReplTbl->mRepName,
                                  (UChar *)aItem->mItem.mRepName,
                                  sRepNameLen + 1 )
              != IDE_SUCCESS );

    sLocalUserNameLen = idlOS::strlen( aItem->mItem.mLocalUsername );
    IDE_TEST( cmtVariableSetData( &sArgMetaReplTbl->mLocalUserName,
                                  (UChar *)aItem->mItem.mLocalUsername,
                                  sLocalUserNameLen + 1 )
              != IDE_SUCCESS );

    sLocalTableNameLen = idlOS::strlen(aItem->mItem.mLocalTablename);
    IDE_TEST( cmtVariableSetData( &sArgMetaReplTbl->mLocalTableName,
                                  (UChar *)aItem->mItem.mLocalTablename,
                                  sLocalTableNameLen + 1 )
              != IDE_SUCCESS );

    sLocalPartNameLen = idlOS::strlen( aItem->mItem.mLocalPartname );
    IDE_TEST( cmtVariableSetData( &sArgMetaReplTbl->mLocalPartName,
                                  (UChar *)aItem->mItem.mLocalPartname,
                                  sLocalPartNameLen + 1 )
              != IDE_SUCCESS );

    sRemoteUserNameLen = idlOS::strlen( aItem->mItem.mRemoteUsername );
    IDE_TEST( cmtVariableSetData( &sArgMetaReplTbl->mRemoteUserName,
                                  (UChar *)aItem->mItem.mRemoteUsername,
                                  sRemoteUserNameLen + 1 )
              != IDE_SUCCESS );

    sRemoteTableNameLen = idlOS::strlen( aItem->mItem.mRemoteTablename );
    IDE_TEST( cmtVariableSetData( &sArgMetaReplTbl->mRemoteTableName,
                                  (UChar *)aItem->mItem.mRemoteTablename,
                                  sRemoteTableNameLen + 1 )
              != IDE_SUCCESS );

    sRemotePartNameLen = idlOS::strlen( aItem->mItem.mRemotePartname );
    IDE_TEST( cmtVariableSetData( &sArgMetaReplTbl->mRemotePartName,
                                  (UChar *)aItem->mItem.mRemotePartname,
                                  sRemotePartNameLen + 1 )
              != IDE_SUCCESS );

    sPartCondMinValuesLen = idlOS::strlen(aItem->mPartCondMinValues);
    IDE_TEST( cmtVariableSetData( &sArgMetaReplTbl->mPartCondMinValues,
                                  (UChar *)aItem->mPartCondMinValues,
                                  sPartCondMinValuesLen + 1 )
              != IDE_SUCCESS );

    sPartCondMaxValuesLen = idlOS::strlen(aItem->mPartCondMaxValues);
    IDE_TEST( cmtVariableSetData( &sArgMetaReplTbl->mPartCondMaxValues,
                                  (UChar *)aItem->mPartCondMaxValues,
                                  sPartCondMaxValuesLen + 1 )
              != IDE_SUCCESS );

    sArgMetaReplTbl->mPartitionMethod = aItem->mPartitionMethod;
    sArgMetaReplTbl->mPartitionOrder  = aItem->mPartitionOrder;
    sArgMetaReplTbl->mTableOID        = aItem->mItem.mTableOID;
    sArgMetaReplTbl->mPKIndexID       = aItem->mPKIndex.indexId;
    sArgMetaReplTbl->mPKColCnt        = aItem->mPKColCount;
    sArgMetaReplTbl->mColumnCnt       = aItem->mColCount;
    sArgMetaReplTbl->mIndexCnt        = aItem->mIndexCount;

    /* PROJ-1915 Invalid Max SN, mConditionStr 을 전송 한다. */
    sArgMetaReplTbl->mInvalidMaxSN = aItem->mItem.mInvalidMaxSN;

    IDE_TEST( cmtVariableInitialize( &sArgMetaReplTbl->mConditionStr )
              != IDE_SUCCESS );

    /* Send Protocol */
    IDE_TEST( writeProtocol( NULL,
                             &sProtocolContext,
                             &sProtocol )
              != IDE_SUCCESS );

    /* Finalize Protocol */
    sStage = 1;
    IDE_TEST( cmiFinalizeProtocol( &sProtocol ) != IDE_SUCCESS );

    sStage = 0;
    IDE_TEST( cmiFinalizeProtocolContext( &sProtocolContext ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 2:
            (void)cmiFinalizeProtocol( &sProtocol );
            /* fall through */
        case 1:
            (void)cmiFinalizeProtocolContext( &sProtocolContext );
            /* fall through */
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
 *
 */  
IDE_RC rpnComm::recvMetaReplTblA5( cmiProtocolContext * aProtocolContext,
                                   idBool             * aExitFlag,
                                   rpdMetaItem        * aItem,
                                   ULong                aTimeoutSec )
{
    cmiProtocol sProtocol;
    cmpArgRPMetaReplTbl * sMetaReplTbl = NULL;
    SInt sStage = 0;
    UInt sNameLen = 0;
    UInt sValueLen = 0;
    
    IDE_TEST( readCmProtocol( aProtocolContext,
                              &sProtocol,
                              aExitFlag,
                              NULL,
                              aTimeoutSec )
              != IDE_SUCCESS );
    sStage = 1;
    
    /* Check Operation Type */
    IDE_TEST_RAISE( sProtocol.mOpID != CMI_PROTOCOL_OPERATION( RP,
                                                               MetaReplTbl ),
                    ERR_CHECK_OPERATION_TYPE );

    /* Get Argument */
    sMetaReplTbl = CMI_PROTOCOL_GET_ARG( sProtocol, RP, MetaReplTbl );

    /* Get Replication Item Information */
    sNameLen = cmtVariableGetSize( &sMetaReplTbl->mRepName );
    IDE_TEST( cmtVariableGetData( &sMetaReplTbl->mRepName,
                                  (UChar *)aItem->mItem.mRepName,
                                  sNameLen )
              != IDE_SUCCESS );
    sNameLen = cmtVariableGetSize( &sMetaReplTbl->mLocalUserName );
    IDE_TEST( cmtVariableGetData( &sMetaReplTbl->mLocalUserName,
                                  (UChar *)aItem->mItem.mLocalUsername,
                                  sNameLen )
              != IDE_SUCCESS );
    sNameLen = cmtVariableGetSize( &sMetaReplTbl->mLocalTableName );
    IDE_TEST( cmtVariableGetData( &sMetaReplTbl->mLocalTableName,
                                  (UChar *)aItem->mItem.mLocalTablename,
                                  sNameLen )
              != IDE_SUCCESS );
    sNameLen = cmtVariableGetSize( &sMetaReplTbl->mLocalPartName );
    IDE_TEST( cmtVariableGetData( &sMetaReplTbl->mLocalPartName,
                                  (UChar *)aItem->mItem.mLocalPartname,
                                  sNameLen )
              != IDE_SUCCESS );
    sNameLen = cmtVariableGetSize( &sMetaReplTbl->mRemoteUserName );
    IDE_TEST( cmtVariableGetData( &sMetaReplTbl->mRemoteUserName,
                                  (UChar *)aItem->mItem.mRemoteUsername,
                                  sNameLen )
              != IDE_SUCCESS );
    sNameLen = cmtVariableGetSize(&sMetaReplTbl->mRemoteTableName);
    IDE_TEST( cmtVariableGetData( &sMetaReplTbl->mRemoteTableName,
                                  (UChar *)aItem->mItem.mRemoteTablename,
                                  sNameLen )
              != IDE_SUCCESS );
    sNameLen = cmtVariableGetSize(&sMetaReplTbl->mRemotePartName);
    IDE_TEST( cmtVariableGetData( &sMetaReplTbl->mRemotePartName,
                                  (UChar *)aItem->mItem.mRemotePartname,
                                  sNameLen )
              != IDE_SUCCESS );
    sValueLen = cmtVariableGetSize(&sMetaReplTbl->mPartCondMinValues);
    IDE_TEST( cmtVariableGetData( &sMetaReplTbl->mPartCondMinValues,
                                  (UChar *)aItem->mPartCondMinValues,
                                  sValueLen )
              != IDE_SUCCESS );
    sValueLen = cmtVariableGetSize(&sMetaReplTbl->mPartCondMaxValues);
    IDE_TEST( cmtVariableGetData( &sMetaReplTbl->mPartCondMaxValues,
                                  (UChar *)aItem->mPartCondMaxValues,
                                  sValueLen )
              != IDE_SUCCESS );
    
    aItem->mPartitionMethod     = sMetaReplTbl->mPartitionMethod;
    aItem->mPartitionOrder      = sMetaReplTbl->mPartitionOrder;
    aItem->mItem.mTableOID      = sMetaReplTbl->mTableOID;
    aItem->mPKIndex.indexId     = sMetaReplTbl->mPKIndexID;
    aItem->mPKColCount          = sMetaReplTbl->mPKColCnt;
    aItem->mColCount            = sMetaReplTbl->mColumnCnt;
    aItem->mIndexCount          = sMetaReplTbl->mIndexCnt;

    aItem->mItem.mInvalidMaxSN  = sMetaReplTbl->mInvalidMaxSN;
    
    /* 기능 제거된 mConditionStr은 무시한다. */

    /* PROJ-2563 remove to check constraints */
    aItem->mCheckCount = 0;

    aItem->mPartitionCount = 0;

    /* Finalize Protocol */
    sStage = 0;
    IDE_TEST( cmiFinalizeProtocol( &sProtocol ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CHECK_OPERATION_TYPE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_WRONG_OPERATION_TYPE,
                                  sProtocol.mOpID ) );
        IDE_ERRLOG( IDE_RP_0 );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 1:
            (void)cmiFinalizeProtocol( &sProtocol );
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendMetaReplColA5( void               * /*aHBTResource*/,
                                   cmiProtocolContext * aProtocolContext,
                                   idBool             * aExitFlag,
                                   rpdColumn          * aColumn,
                                   UInt                 aTimeoutSec )
{
    cmiProtocolContext    sProtocolContext;
    cmiProtocol           sProtocol;
    UInt                  sStage          = 0;
    cmiLink             * sLink           = NULL;
    cmpArgRPMetaReplCol * sArgMetaReplCol = NULL;

    UInt sColumnNameLen    = 0;
    UInt sPolicyNameLen    = 0;
    UInt sPolicyCodeLen    = 0;
    UInt sECCPolicyNameLen = 0;
    UInt sECCPolicyCodeLen = 0;

    /* Validate Parameters */
    IDE_DASSERT( aColumn != NULL );

    IDE_TEST( cmiGetLinkForProtocolContext( aProtocolContext, &sLink )
              != IDE_SUCCESS );

    IDE_TEST( cmiInitializeProtocolContext( &sProtocolContext,
                                            CMI_PROTOCOL_MODULE( RP ),
                                            sLink )
              != IDE_SUCCESS );
    sStage = 1;

    cmiProtocolContextCopyA5Header( &sProtocolContext,
                                    &(aProtocolContext->mReadHeader) );

    /* Initialize Protocol */
    CMI_PROTOCOL_INITIALIZE( sProtocol, sArgMetaReplCol, RP, MetaReplCol );
    sStage = 2;


    /* Replication Item Information Set */
    sColumnNameLen = idlOS::strlen( aColumn->mColumnName );
    IDE_TEST( cmtVariableSetData( &sArgMetaReplCol->mColumnName,
                                  (UChar *)aColumn->mColumnName,
                                  sColumnNameLen + 1 )
              != IDE_SUCCESS );

    sArgMetaReplCol->mColumnID      = aColumn->mColumn.column.id;
    sArgMetaReplCol->mColumnFlag    = aColumn->mColumn.column.flag;
    sArgMetaReplCol->mColumnOffset  = aColumn->mColumn.column.offset;
    sArgMetaReplCol->mColumnSize    = aColumn->mColumn.column.size;
    sArgMetaReplCol->mDataTypeID    = aColumn->mColumn.type.dataTypeId;
    sArgMetaReplCol->mLangID        = aColumn->mColumn.type.languageId;
    sArgMetaReplCol->mFlags         = aColumn->mColumn.flag;
    sArgMetaReplCol->mPrecision     = aColumn->mColumn.precision;
    sArgMetaReplCol->mScale         = aColumn->mColumn.scale;
    sArgMetaReplCol->mEncPrecision  = aColumn->mColumn.mColumnAttr.mEncAttr.mEncPrecision;

    sPolicyNameLen = idlOS::strlen( aColumn->mColumn.mColumnAttr.mEncAttr.mPolicy );
    IDE_TEST( cmtVariableSetData( &sArgMetaReplCol->mPolicyName,
                                  (UChar *)aColumn->mColumn.mColumnAttr.mEncAttr.mPolicy,
                                  sPolicyNameLen + 1 )
              != IDE_SUCCESS );

    sPolicyCodeLen = idlOS::strlen( (SChar *)aColumn->mPolicyCode );
    IDE_TEST( cmtVariableSetData( &sArgMetaReplCol->mPolicyCode,
                                  (UChar *)aColumn->mPolicyCode,
                                  sPolicyCodeLen + 1 )
              != IDE_SUCCESS );

    sECCPolicyNameLen = idlOS::strlen( aColumn->mECCPolicyName );
    IDE_TEST( cmtVariableSetData( &sArgMetaReplCol->mECCPolicyName,
                                  (UChar *)aColumn->mECCPolicyName,
                                  sECCPolicyNameLen + 1 )
               != IDE_SUCCESS );

    sECCPolicyCodeLen = idlOS::strlen( (SChar *)aColumn->mECCPolicyCode );
    IDE_TEST( cmtVariableSetData( &sArgMetaReplCol->mECCPolicyCode,
                                  (UChar *)aColumn->mECCPolicyCode,
                                  sECCPolicyCodeLen + 1 )
              != IDE_SUCCESS );

    /* Send Protocol */
    IDE_TEST( writeProtocol( NULL,
                             &sProtocolContext,
                             &sProtocol )
              != IDE_SUCCESS );

    IDE_TEST( flushProtocol( NULL,
                             &sProtocolContext,
                             aExitFlag,
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Finalize Protocol */
    sStage = 1;
    IDE_TEST( cmiFinalizeProtocol( &sProtocol ) != IDE_SUCCESS );

    sStage = 0;
    IDE_TEST( cmiFinalizeProtocolContext( &sProtocolContext ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 2:
            (void)cmiFinalizeProtocol( &sProtocol );
            /* fall through */
        case 1:
            (void)cmiFinalizeProtocolContext( &sProtocolContext );
            /* fall through */
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
 *
 */  
IDE_RC rpnComm::recvMetaReplColA5( cmiProtocolContext * aProtocolContext,
                                   idBool             * aExitFlag,
                                   rpdColumn          * aColumn,
                                   ULong                aTimeoutSec )
{
    cmiProtocol sProtocol;
    SInt sStage = 0;
    cmpArgRPMetaReplCol * sMetaReplCol = NULL;
    UInt sNameLen = 0;
    UInt sCodeLen = 0;
    
    IDE_TEST( readCmProtocol( aProtocolContext,
                              &sProtocol,
                              aExitFlag,
                              NULL,
                              aTimeoutSec )
              != IDE_SUCCESS );
    sStage = 1;
    
    /* Check Operation Type */
    IDE_TEST_RAISE( sProtocol.mOpID != CMI_PROTOCOL_OPERATION( RP,
                                                               MetaReplCol ),
                    ERR_CHECK_OPERATION_TYPE );

    /* Get Argument */
    sMetaReplCol = CMI_PROTOCOL_GET_ARG( sProtocol, RP, MetaReplCol );

    /* Get Replication Item Column Information */
    sNameLen = cmtVariableGetSize( &sMetaReplCol->mColumnName );
    IDE_TEST( cmtVariableGetData( &sMetaReplCol->mColumnName,
                                  (UChar *)aColumn->mColumnName,
                                  sNameLen )
              != IDE_SUCCESS );
    
    aColumn->mColumn.column.id          = sMetaReplCol->mColumnID;
    aColumn->mColumn.column.flag        = sMetaReplCol->mColumnFlag;
    aColumn->mColumn.column.offset      = sMetaReplCol->mColumnOffset;
    aColumn->mColumn.column.size        = sMetaReplCol->mColumnSize;
    aColumn->mColumn.type.dataTypeId    = sMetaReplCol->mDataTypeID;
    aColumn->mColumn.type.languageId    = sMetaReplCol->mLangID;
    aColumn->mColumn.flag               = sMetaReplCol->mFlags;
    aColumn->mColumn.precision          = sMetaReplCol->mPrecision;
    aColumn->mColumn.scale              = sMetaReplCol->mScale;
    aColumn->mColumn.mColumnAttr.mEncAttr.mEncPrecision = sMetaReplCol->mEncPrecision;
    
    sNameLen = cmtVariableGetSize( &sMetaReplCol->mPolicyName );
    IDE_TEST( cmtVariableGetData( &sMetaReplCol->mPolicyName,
                                  (UChar *)aColumn->mColumn.mColumnAttr.mEncAttr.mPolicy,
                                  sNameLen )
              != IDE_SUCCESS );
    sCodeLen = cmtVariableGetSize( &sMetaReplCol->mPolicyCode );
    IDE_TEST( cmtVariableGetData( &sMetaReplCol->mPolicyCode,
                                  (UChar *)aColumn->mPolicyCode,
                                  sCodeLen )
              != IDE_SUCCESS );
    sNameLen = cmtVariableGetSize( &sMetaReplCol->mECCPolicyName );
    IDE_TEST( cmtVariableGetData( &sMetaReplCol->mECCPolicyName,
                                  (UChar *)aColumn->mECCPolicyName,
                                  sNameLen )
              != IDE_SUCCESS );
    sCodeLen = cmtVariableGetSize( &sMetaReplCol->mECCPolicyCode );
    IDE_TEST( cmtVariableGetData( &sMetaReplCol->mECCPolicyCode,
                                  (UChar *)aColumn->mECCPolicyCode,
                                  sCodeLen )
              != IDE_SUCCESS );

    /* Finalize Protocol */
    sStage = 0;
    IDE_TEST( cmiFinalizeProtocol( &sProtocol) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CHECK_OPERATION_TYPE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_WRONG_OPERATION_TYPE,
                                  sProtocol.mOpID ) );
        IDE_ERRLOG(IDE_RP_0);
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 1:
            (void)cmiFinalizeProtocol( &sProtocol );
        default:
            break;
    }
    
    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendMetaReplIdxA5( void               * /*aHBTResource*/,
                                   cmiProtocolContext * aProtocolContext,
                                   idBool             * aExitFlag,
                                   qcmIndex           * aIndex,
                                   UInt                 aTimeoutSec )
{
    cmiProtocolContext    sProtocolContext;
    cmiProtocol           sProtocol;
    UInt                  sStage          = 0;
    cmiLink             * sLink           = NULL;
    cmpArgRPMetaReplIdx * sArgMetaReplIdx = NULL;

    UInt sIndexNameLen   = 0;

    /* Validate Parameters */
    IDE_DASSERT( aIndex != NULL );

    IDE_TEST( cmiGetLinkForProtocolContext( aProtocolContext, &sLink )
              != IDE_SUCCESS );

    IDE_TEST( cmiInitializeProtocolContext( &sProtocolContext,
                                            CMI_PROTOCOL_MODULE( RP ),
                                            sLink )
              != IDE_SUCCESS );
    sStage = 1;

    cmiProtocolContextCopyA5Header( &sProtocolContext,
                                    &(aProtocolContext->mReadHeader) );

    /* Initialize Protocol */
    CMI_PROTOCOL_INITIALIZE( sProtocol, sArgMetaReplIdx, RP, MetaReplIdx );
    sStage = 2;

    /* Replication Item Index Information Set */
    sIndexNameLen = idlOS::strlen( aIndex->name );
    IDE_TEST( cmtVariableSetData( &sArgMetaReplIdx->mIndexName,
                                  (UChar *)aIndex->name,
                                  sIndexNameLen + 1)
              != IDE_SUCCESS );

    sArgMetaReplIdx->mIndexID       = aIndex->indexId;
    sArgMetaReplIdx->mIndexTypeID   = aIndex->indexTypeId;
    sArgMetaReplIdx->mKeyColumnCnt  = aIndex->keyColCount;
    sArgMetaReplIdx->mIsUnique      = aIndex->isUnique;
    sArgMetaReplIdx->mIsRange       = aIndex->isRange;

    /* Send Protocol */
    IDE_TEST( writeProtocol( NULL, /* aHBTResource */
                             &sProtocolContext,
                             &sProtocol )
              != IDE_SUCCESS );

    IDE_TEST( flushProtocol( NULL, /* aHBTResource */
                             &sProtocolContext,
                             aExitFlag,
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Finalize Protocol */
    sStage = 1;
    IDE_TEST( cmiFinalizeProtocol( &sProtocol ) != IDE_SUCCESS );

    sStage = 0;
    IDE_TEST( cmiFinalizeProtocolContext( &sProtocolContext ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 2:
            (void)cmiFinalizeProtocol( &sProtocol );
            /* fall through */
        case 1:
            (void)cmiFinalizeProtocolContext( &sProtocolContext );
            /* fall through */
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}


/*
 *
 */  
IDE_RC rpnComm::recvMetaReplIdxA5( cmiProtocolContext * aProtocolContext,
                                   idBool             * aExitFlag,
                                   qcmIndex           * aIndex,
                                   ULong                aTimeoutSec )
{
    cmiProtocol sProtocol;
    SInt sStage = 0;
    cmpArgRPMetaReplIdx * sMetaReplIdx = NULL;
    UInt sNameLen = 0;
    
    IDE_TEST( readCmProtocol( aProtocolContext,
                              &sProtocol,
                              aExitFlag,
                              NULL,
                              aTimeoutSec )
              != IDE_SUCCESS );
    sStage = 1;

    /* Check Operation Type */
    IDE_TEST_RAISE( sProtocol.mOpID != CMI_PROTOCOL_OPERATION( RP,
                                                               MetaReplIdx ),
                    ERR_CHECK_OPERATION_TYPE );

    /* Get Argument */
    sMetaReplIdx = CMI_PROTOCOL_GET_ARG( sProtocol, RP, MetaReplIdx );

    /* Get Replication Item Index Information */
    sNameLen = cmtVariableGetSize( &sMetaReplIdx->mIndexName );
    IDE_TEST( cmtVariableGetData( &sMetaReplIdx->mIndexName,
                                  (UChar *)aIndex->name,
                                  sNameLen )
              != IDE_SUCCESS );
    
    aIndex->indexId     = sMetaReplIdx->mIndexID;
    aIndex->indexTypeId = sMetaReplIdx->mIndexTypeID;
    aIndex->keyColCount = sMetaReplIdx->mKeyColumnCnt;
    aIndex->isUnique    = (sMetaReplIdx->mIsUnique == 0) ? ID_FALSE : ID_TRUE;
    aIndex->isRange     = (sMetaReplIdx->mIsRange == 0) ? ID_FALSE : ID_TRUE;

    /* Finalize Protocol */
    sStage = 0;
    IDE_TEST( cmiFinalizeProtocol( &sProtocol ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CHECK_OPERATION_TYPE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_WRONG_OPERATION_TYPE,
                                  sProtocol.mOpID ) );
        IDE_ERRLOG( IDE_RP_0 );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 1:
            (void)cmiFinalizeProtocol( &sProtocol );
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendMetaReplIdxColA5( void                * /*aHBTResource*/,
                                      cmiProtocolContext  * aProtocolContext,
                                      idBool              * aExitFlag,
                                      UInt                  aColumnID,
                                      UInt                  aKeyColumnFlag,
                                      UInt                  aTimeoutSec )
{
    cmiProtocolContext       sProtocolContext;
    cmiProtocol              sProtocol;
    UInt                     sStage          = 0;
    cmiLink                * sLink           = NULL;
    cmpArgRPMetaReplIdxCol * sArgMetaReplIdxCol = NULL;

    IDE_TEST( cmiGetLinkForProtocolContext( aProtocolContext, &sLink )
              != IDE_SUCCESS );

    IDE_TEST( cmiInitializeProtocolContext( &sProtocolContext,
                                            CMI_PROTOCOL_MODULE( RP ),
                                            sLink )
              != IDE_SUCCESS );
    sStage = 1;

    cmiProtocolContextCopyA5Header( &sProtocolContext,
                                    &(aProtocolContext->mReadHeader) );

    /* Initialize Protocol */
    CMI_PROTOCOL_INITIALIZE( sProtocol, sArgMetaReplIdxCol, RP, MetaReplIdxCol );
    sStage = 2;

    /* Replication Item Index Column Description Set */
    sArgMetaReplIdxCol->mColumnID      = aColumnID;
    sArgMetaReplIdxCol->mKeyColumnFlag = aKeyColumnFlag;

    /* Send Protocol */
    IDE_TEST( writeProtocol( NULL, /* aHBTResource */
                             &sProtocolContext,
                             &sProtocol )
              != IDE_SUCCESS );

    IDE_TEST( flushProtocol( NULL, /* aHBTResource */
                             &sProtocolContext,
                             aExitFlag,
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Finalize Protocol */
    sStage = 1;
    IDE_TEST( cmiFinalizeProtocol( &sProtocol ) != IDE_SUCCESS );

    sStage = 0;
    IDE_TEST( cmiFinalizeProtocolContext( &sProtocolContext ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 2:
            (void)cmiFinalizeProtocol( &sProtocol );
            /* fall through */
        case 1:
            (void)cmiFinalizeProtocolContext( &sProtocolContext );
            /* fall through */
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
 *
 */  
IDE_RC rpnComm::recvMetaReplIdxColA5( cmiProtocolContext * aProtocolContext,
                                      idBool             * aExitFlag,
                                      UInt               * aColumnID,
                                      UInt               * aKeyColumnFlag,
                                      ULong                aTimeoutSec )
{
    cmiProtocol sProtocol;
    SInt sStage = 0;
    cmpArgRPMetaReplIdxCol * sMetaReplIdxCol = NULL;
    
    IDE_TEST( readCmProtocol( aProtocolContext,
                              &sProtocol,
                              aExitFlag,
                              NULL,
                              aTimeoutSec )
              != IDE_SUCCESS );
    sStage = 1;

    /* Check Operation Type */
    IDE_TEST_RAISE( sProtocol.mOpID !=
                    CMI_PROTOCOL_OPERATION( RP, MetaReplIdxCol ),
                    ERR_CHECK_OPERATION_TYPE );

    /* Get Argument */
    sMetaReplIdxCol = CMI_PROTOCOL_GET_ARG( sProtocol, RP, MetaReplIdxCol );

    /* Get Replication Item Index Information */
    *aColumnID      = sMetaReplIdxCol->mColumnID;
    *aKeyColumnFlag = sMetaReplIdxCol->mKeyColumnFlag;

    /* Finalize Protocol */
    sStage = 0;
    IDE_TEST( cmiFinalizeProtocol( &sProtocol ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CHECK_OPERATION_TYPE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_WRONG_OPERATION_TYPE,
                                  sProtocol.mOpID ) );
        IDE_ERRLOG( IDE_RP_0 );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 1:
            (void)cmiFinalizeProtocol( &sProtocol );
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendMetaReplCheckA5( void                 * /*aHBTResource*/,
                                     cmiProtocolContext   * /*aProtocolContext*/,
                                     idBool               * /*aExitFlag*/,
                                     qcmCheck             * /*aCheck*/,
                                     UInt                   /*aTimeoutSec*/ )
{
    IDE_DASSERT( 0 );

    return IDE_FAILURE;
}

/*
 *
 */  
IDE_RC rpnComm::sendHandshakeAckA5( cmiProtocolContext  * aProtocolContext,
                                    idBool              * aExitFlag,
                                    UInt                  aResult,
                                    SInt                  aFailbackStatus,
                                    ULong                 aXSN,
                                    const SChar         * aMsg,
                                    UInt                  aTimeoutSec )
{
    cmiProtocolContext sProtocolContext;
    cmiLink * sLink = NULL;
    cmiProtocol sProtocol;
    cmpArgRPHandshakeAck * sAck = NULL;
    SInt sStage = 0;
    UInt sMsgLen = 0;

    IDE_TEST( cmiGetLinkForProtocolContext( aProtocolContext, &sLink )
              != IDE_SUCCESS );
    
    IDE_TEST( cmiInitializeProtocolContext( &sProtocolContext,
                                            CMI_PROTOCOL_MODULE( RP ),
                                            sLink )
              != IDE_SUCCESS );
    sStage = 1;

    cmiProtocolContextCopyA5Header( &sProtocolContext,
                                    &(aProtocolContext->mReadHeader) );

    CMI_PROTOCOL_INITIALIZE( sProtocol, sAck, RP, HandshakeAck );
    sStage = 2;

    sAck->mResult = aResult;
    sAck->mFailbackStatus = aFailbackStatus;
    sAck->mXSN = aXSN;
    sMsgLen = idlOS::strlen( aMsg );
    IDE_TEST( cmtVariableSetData( &sAck->mMsg, 
                                  (UChar *)aMsg, 
                                  sMsgLen + 1 )
              != IDE_SUCCESS );
    
    IDE_TEST( writeProtocol( NULL, /* aHBTResource */
                             &sProtocolContext,
                             &sProtocol )
              != IDE_SUCCESS );

    IDE_TEST( flushProtocol( NULL, /* aHBTResource */
                             &sProtocolContext,
                             aExitFlag,
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    sStage = 1;
    IDE_TEST( cmiFinalizeProtocol( &sProtocol ) != IDE_SUCCESS );

    sStage = 0;
    IDE_TEST( cmiFinalizeProtocolContext( &sProtocolContext ) != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 2:
            (void)cmiFinalizeProtocol( &sProtocol );
        case 1:
            (void)cmiFinalizeProtocolContext( &sProtocolContext );
        default:
            break;
    }

    IDE_POP()

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvHandshakeAckA5( cmiProtocolContext * aProtocolContext,
                                    idBool             * aExitFlag,
                                    UInt               * aResult,
                                    SInt               * aFailbackStatus,
                                    ULong              * aXSN,
                                    SChar              * aMsg,
                                    UInt               * aMsgLen,
                                    ULong                aTimeOut )
{
    cmiProtocol              sProtocol;
    SInt                     sStage           = 0;
    cmpArgRPHandshakeAck   * sArgHandshakeAck = NULL;

    CMI_PROTOCOL_INITIALIZE( sProtocol, sArgHandshakeAck, RP, HandshakeAck );
    sStage = 1;

    IDE_TEST( readCmProtocol( aProtocolContext,
                              &sProtocol,
                              aExitFlag,
                              NULL,
                              aTimeOut )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sProtocol.mOpID != CMI_PROTOCOL_OPERATION( RP, HandshakeAck ),
                    ERR_CHECK_OPERATION_TYPE );

    *aResult         = sArgHandshakeAck->mResult;
    *aFailbackStatus = sArgHandshakeAck->mFailbackStatus;

    *aMsgLen = cmtVariableGetSize( &sArgHandshakeAck->mMsg );
    IDE_TEST( cmtVariableGetData( &sArgHandshakeAck->mMsg,
                                  (UChar *)aMsg,
                                  *aMsgLen )
              != IDE_SUCCESS );

    if( aXSN != NULL )
    {
        *aXSN = sArgHandshakeAck->mXSN;
    }
    else
    {
        /*nothing to do*/
    }

    /* Finalize Protocol */
    sStage = 0;
    IDE_TEST( cmiFinalizeProtocol( &sProtocol ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CHECK_OPERATION_TYPE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_WRONG_OPERATION_TYPE,
                                  sProtocol.mOpID ) );
        IDE_ERRLOG( IDE_RP_0 );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 1:
            (void)cmiFinalizeProtocol( &sProtocol );
            break;
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
 *
 */  
IDE_RC rpnComm::recvXLogA5( iduMemAllocator    * aAllocator,
                            cmiProtocolContext * aProtocolContext,
                            idBool             * aExitFlag,
                            rpdMeta            * aMeta,
                            rpdXLog            * aXLog,
                            ULong                aTimeoutSec )
{
    cmiProtocol sProtocol;
    SInt sStage = 0;
    
    IDE_TEST( readCmProtocol( aProtocolContext,
                              &sProtocol,
                              aExitFlag,
                              NULL,
                              aTimeoutSec )
              != IDE_SUCCESS );
    sStage = 1;

    switch ( sProtocol.mOpID )
    {
        case CMI_PROTOCOL_OPERATION( RP, TrBegin ):
        {
            IDE_TEST( recvTrBeginA5( &sProtocol, aXLog ) != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, TrCommit ):
        {
            IDE_TEST( recvTrCommitA5( &sProtocol, aXLog ) != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, TrAbort ):
        {
            IDE_TEST( recvTrAbortA5( &sProtocol, aXLog ) != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, SPSet ):
        {
            IDE_TEST( recvSPSetA5( &sProtocol, aXLog ) != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, SPAbort ):
        {
            IDE_TEST( recvSPAbortA5( &sProtocol, aXLog ) != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, Insert ):
        {
            IDE_TEST( recvInsertA5( aAllocator,
                                    aExitFlag,
                                    aProtocolContext,
                                    &sProtocol,
                                    aMeta,
                                    aXLog,
                                    aTimeoutSec )
                      != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, Update ):
        {
            IDE_TEST( recvUpdateA5( aAllocator,
                                    aExitFlag,
                                    aProtocolContext,
                                    &sProtocol,
                                    aMeta,
                                    aXLog,
                                    aTimeoutSec )
                      != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, Delete ):
        {
            IDE_TEST( recvDeleteA5( aAllocator,
                                    aExitFlag,
                                    aProtocolContext,
                                    &sProtocol,
                                    aXLog,
                                    aTimeoutSec )
                      != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, Stop ):
        {
            IDE_TEST( recvStopA5( &sProtocol, aXLog ) != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, KeepAlive ):
        {
            IDE_TEST( recvKeepAliveA5( &sProtocol, aXLog ) != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, Flush ):
        {
            IDE_TEST( recvFlushA5( &sProtocol, aXLog ) != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, LobCursorOpen ):
        {
            IDE_TEST( recvLobCursorOpenA5( aAllocator,
                                           aExitFlag,
                                           aProtocolContext,
                                           &sProtocol,
                                           aMeta,
                                           aXLog,
                                           aTimeoutSec )
                      != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, LobCursorClose ):
        {
            IDE_TEST( recvLobCursorCloseA5( &sProtocol, aXLog ) != IDE_SUCCESS);
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, LobPrepare4Write ):
        {
            IDE_TEST( recvLobPrepare4WriteA5( &sProtocol, aXLog ) != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, LobPartialWrite ):
        {
            IDE_TEST( recvLobPartialWriteA5( aAllocator,
                                             &sProtocol,
                                             aXLog )
                      != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, LobFinish2Write ):
        {
            IDE_TEST( recvLobFinish2WriteA5( &sProtocol, aXLog ) != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, Handshake ): // BUG-23195
        {
            IDE_TEST( recvHandshakeA5( &sProtocol, aXLog ) != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, SyncPKBegin ):
        {
            IDE_TEST( recvSyncPKBeginA5( &sProtocol, aXLog ) != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, SyncPK ):
        {
            IDE_TEST( recvSyncPKA5( aExitFlag,
                                    aProtocolContext,
                                    &sProtocol,
                                    aXLog,
                                    aTimeoutSec )
                      != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, SyncPKEnd ):
        {
            IDE_TEST( recvSyncPKEndA5( &sProtocol, aXLog ) != IDE_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION(RP, FailbackEnd):
        {
            IDE_TEST( recvFailbackEndA5( &sProtocol, aXLog ) != IDE_SUCCESS);
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, SyncStart ):
        case CMI_PROTOCOL_OPERATION( RP, SyncEnd ):
        case CMI_PROTOCOL_OPERATION( RP, LobTrim ):
            /* Don't support this protocol at HDB V6's replication */

        default:
            // BUG-17215
            IDE_RAISE( ERR_PROTOCOL_OPID );
    }

    sStage = 0;
    IDE_TEST( cmiFinalizeProtocol( &sProtocol ) != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_PROTOCOL_OPID );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RECEIVER_UNEXPECTED_PROTOCOL,
                                  sProtocol.mOpID ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 1:
            (void)cmiFinalizeProtocol( &sProtocol );
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;    
}


IDE_RC rpnComm::sendTrBeginA5( void                * /*aHBTResource*/,
                               cmiProtocolContext  * aProtocolContext,
                               idBool              * aExitFlag,
                               smTID                 aTID,
                               smSN                  aSN,
                               smSN                  aSyncSN,
                               UInt                  aTimeoutSec )
{
    cmiProtocolContext       sProtocolContext;
    cmiProtocol              sProtocol;
    UInt                     sStage          = 0;
    cmiLink                * sLink           = NULL;
    cmpArgRPTrBegin        * sArgTrBegin = NULL;

    IDE_TEST( cmiGetLinkForProtocolContext( aProtocolContext, &sLink )
              != IDE_SUCCESS );

    IDE_TEST( cmiInitializeProtocolContext( &sProtocolContext,
                                            CMI_PROTOCOL_MODULE( RP ),
                                            sLink )
              != IDE_SUCCESS );
    sStage = 1;

    cmiProtocolContextCopyA5Header( &sProtocolContext,
                                    &(aProtocolContext->mReadHeader) );

    /* Initialize Protocol */
    CMI_PROTOCOL_INITIALIZE( sProtocol, sArgTrBegin, RP, TrBegin );
    sStage = 2;

    /* Replication XLog Header Set */
    sArgTrBegin->mXLogType  = RP_X_BEGIN;
    sArgTrBegin->mTransID   = aTID;
    sArgTrBegin->mSN        = aSN;
    sArgTrBegin->mSyncSN    = aSyncSN;

    /* Send Protocol */
    IDE_TEST( writeProtocol( NULL, /* aHBTResource */
                             &sProtocolContext,
                             &sProtocol )
              != IDE_SUCCESS );

    IDE_TEST( flushProtocol( NULL, /* aHBTResource */
                             &sProtocolContext,
                             aExitFlag,
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Finalize Protocol */
    sStage = 1;
    IDE_TEST( cmiFinalizeProtocol( &sProtocol ) != IDE_SUCCESS );

    sStage = 0;
    IDE_TEST( cmiFinalizeProtocolContext( &sProtocolContext ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 2:
            (void)cmiFinalizeProtocol( &sProtocol );
            /* fall through */
        case 1:
            (void)cmiFinalizeProtocolContext( &sProtocolContext );
            /* fall through */
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
 *
 */  
IDE_RC rpnComm::recvTrBeginA5( cmiProtocol         * aProtocol,
                               rpdXLog             * aXLog )
{
    cmpArgRPTrBegin * sArg = CMI_PROTOCOL_GET_ARG( *aProtocol, RP, TrBegin );

    /* Get Argument XLog Hdr */
    aXLog->mType    = (rpXLogType)sArg->mXLogType;
    aXLog->mTID     = sArg->mTransID;
    aXLog->mSN      = sArg->mSN;
    aXLog->mSyncSN  = sArg->mSyncSN;

    return IDE_SUCCESS;    
}

IDE_RC rpnComm::sendTrCommitA5( void               * /*aHBTResource*/,
                                cmiProtocolContext * aProtocolContext,
                                idBool             * aExitFlag,
                                smTID                aTID,
                                smSN                 aSN,
                                smSN                 aSyncSN,
                                idBool               /*aForceFlush*/,
                                UInt                 aTimeoutSec )
{
    cmiProtocolContext       sProtocolContext;
    cmiProtocol              sProtocol;
    UInt                     sStage          = 0;
    cmiLink                * sLink           = NULL;
    cmpArgRPTrCommit       * sArgTrCommit    = NULL;

    IDE_TEST( cmiGetLinkForProtocolContext( aProtocolContext, &sLink )
              != IDE_SUCCESS );

    IDE_TEST( cmiInitializeProtocolContext( &sProtocolContext,
                                            CMI_PROTOCOL_MODULE( RP ),
                                            sLink )
              != IDE_SUCCESS );
    sStage = 1;

    cmiProtocolContextCopyA5Header( &sProtocolContext,
                                    &(aProtocolContext->mReadHeader) );

    /* Initialize Protocol */
    CMI_PROTOCOL_INITIALIZE( sProtocol, sArgTrCommit, RP, TrCommit );
    sStage = 2;

    /* Replication XLog Header Set */
    sArgTrCommit->mXLogType  = RP_X_COMMIT;
    sArgTrCommit->mTransID   = aTID;
    sArgTrCommit->mSN        = aSN;
    sArgTrCommit->mSyncSN    = aSyncSN;

    /* Send Protocol */
    IDE_TEST( writeProtocol( NULL, /* aHBTResource */
                             &sProtocolContext,
                             &sProtocol )
              != IDE_SUCCESS );

    IDE_TEST( flushProtocol( NULL, /* aHBTResource */
                             &sProtocolContext,
                             aExitFlag,
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Finalize Protocol */
    sStage = 1;
    IDE_TEST( cmiFinalizeProtocol( &sProtocol ) != IDE_SUCCESS );

    sStage = 0;
    IDE_TEST( cmiFinalizeProtocolContext( &sProtocolContext ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 2:
            (void)cmiFinalizeProtocol( &sProtocol );
            /* fall through */
        case 1:
            (void)cmiFinalizeProtocolContext( &sProtocolContext );
            /* fall through */
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC rpnComm::recvTrCommitA5( cmiProtocol        * aProtocol,
                                rpdXLog            * aXLog )
{
    cmpArgRPTrCommit * sArg = CMI_PROTOCOL_GET_ARG( *aProtocol, RP, TrCommit );

    /* Get Argument XLog Hdr */
    aXLog->mType    = (rpXLogType)sArg->mXLogType;
    aXLog->mTID     = sArg->mTransID;
    aXLog->mSN      = sArg->mSN;
    aXLog->mSyncSN  = sArg->mSyncSN;

    return IDE_SUCCESS;
}

IDE_RC rpnComm::sendTrAbortA5( void               * /*aHBTResource*/,
                               cmiProtocolContext * aProtocolContext,
                               idBool             * aExitFlag,
                               smTID                aTID,
                               smSN                 aSN,
                               smSN                 aSyncSN,
                               UInt                 aTimeoutSec )
{
    cmiProtocolContext       sProtocolContext;
    cmiProtocol              sProtocol;
    UInt                     sStage          = 0;
    cmiLink                * sLink           = NULL;
    cmpArgRPTrAbort        * sArgTrAbort     = NULL;

    IDE_TEST( cmiGetLinkForProtocolContext( aProtocolContext, &sLink )
              != IDE_SUCCESS );

    IDE_TEST( cmiInitializeProtocolContext( &sProtocolContext,
                                            CMI_PROTOCOL_MODULE( RP ),
                                            sLink )
              != IDE_SUCCESS );
    sStage = 1;

    cmiProtocolContextCopyA5Header( &sProtocolContext,
                                    &(aProtocolContext->mReadHeader) );

    /* Initialize Protocol */
    CMI_PROTOCOL_INITIALIZE( sProtocol, sArgTrAbort, RP, TrAbort );
    sStage = 2;

    /* Replication XLog Header Set */
    sArgTrAbort->mXLogType  = RP_X_ABORT;
    sArgTrAbort->mTransID   = aTID;
    sArgTrAbort->mSN        = aSN;
    sArgTrAbort->mSyncSN    = aSyncSN;

    /* Send Protocol */
    IDE_TEST( writeProtocol( NULL, /* aHBTResource */
                             &sProtocolContext,
                             &sProtocol )
              != IDE_SUCCESS );

    IDE_TEST( flushProtocol( NULL, /* aHBTResource */
                             &sProtocolContext,
                             aExitFlag,
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Finalize Protocol */
    sStage = 1;
    IDE_TEST( cmiFinalizeProtocol( &sProtocol ) != IDE_SUCCESS );

    sStage = 0;
    IDE_TEST( cmiFinalizeProtocolContext( &sProtocolContext ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 2:
            (void)cmiFinalizeProtocol( &sProtocol );
            /* fall through */
        case 1:
            (void)cmiFinalizeProtocolContext( &sProtocolContext );
            /* fall through */
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC rpnComm::recvTrAbortA5( cmiProtocol         * aProtocol,
                               rpdXLog             * aXLog )
{
    cmpArgRPTrAbort * sArg = CMI_PROTOCOL_GET_ARG( *aProtocol, RP, TrAbort );

    /* Get Argument XLog Hdr */
    aXLog->mType    = (rpXLogType)sArg->mXLogType;
    aXLog->mTID     = sArg->mTransID;
    aXLog->mSN      = sArg->mSN;
    aXLog->mSyncSN  = sArg->mSyncSN;

    return IDE_SUCCESS;    
}

IDE_RC rpnComm::sendSPSetA5( void               * /*aHBTResource*/,
                             cmiProtocolContext * aProtocolContext,
                             idBool             * aExitFlag,
                             smTID                aTID,
                             smSN                 aSN,
                             smSN                 aSyncSN,
                             UInt                 aSPNameLen,
                             SChar              * aSPName,
                             UInt                 aTimeoutSec )
{
    cmiProtocolContext       sProtocolContext;
    cmiProtocol              sProtocol;
    UInt                     sStage          = 0;
    cmiLink                * sLink           = NULL;
    cmpArgRPSPSet          * sArgSPSet       = NULL;

    /* Validate Parameters */
    IDE_DASSERT( aSPNameLen != 0 );
    IDE_DASSERT( aSPName    != NULL );

    IDE_TEST( cmiGetLinkForProtocolContext( aProtocolContext, &sLink )
              != IDE_SUCCESS );

    IDE_TEST( cmiInitializeProtocolContext( &sProtocolContext,
                                            CMI_PROTOCOL_MODULE( RP ),
                                            sLink )
              != IDE_SUCCESS );
    sStage = 1;

    cmiProtocolContextCopyA5Header( &sProtocolContext,
                                    &(aProtocolContext->mReadHeader) );

    /* Initialize Protocol */
    CMI_PROTOCOL_INITIALIZE( sProtocol, sArgSPSet, RP, SPSet );
    sStage = 2;

    /* Replication XLog Header Set */
    sArgSPSet->mXLogType  = RP_X_SP_SET;
    sArgSPSet->mTransID   = aTID;
    sArgSPSet->mSN        = aSN;
    sArgSPSet->mSyncSN    = aSyncSN;

    /* Additional Information     */
    sArgSPSet->mSPNameLen = aSPNameLen;
    IDE_TEST( cmtVariableSetData( &sArgSPSet->mSPName,
                                  (UChar *)aSPName,
                                  aSPNameLen + 1 )
              != IDE_SUCCESS );

    /* Send Protocol */
    IDE_TEST( writeProtocol( NULL, /* aHBTResource */
                             &sProtocolContext,
                             &sProtocol )
              != IDE_SUCCESS );

    IDE_TEST( flushProtocol( NULL, /* aHBTResource */
                             &sProtocolContext,
                             aExitFlag,
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Finalize Protocol */
    sStage = 1;
    IDE_TEST( cmiFinalizeProtocol( &sProtocol ) != IDE_SUCCESS );

    sStage = 0;
    IDE_TEST( cmiFinalizeProtocolContext( &sProtocolContext ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 2:
            (void)cmiFinalizeProtocol( &sProtocol );
            /* fall through */
        case 1:
            (void)cmiFinalizeProtocolContext( &sProtocolContext );
            /* fall through */
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC rpnComm::recvSPSetA5( cmiProtocol         * aProtocol,
                             rpdXLog             * aXLog )
{
    cmpArgRPSPSet * sArg = CMI_PROTOCOL_GET_ARG( *aProtocol, RP, SPSet );

    /* Get Argument XLog Hdr */
    aXLog->mType    = (rpXLogType)sArg->mXLogType;
    aXLog->mTID     = sArg->mTransID;
    aXLog->mSN      = sArg->mSN;
    aXLog->mSyncSN  = sArg->mSyncSN;

    /* Get Argument Savepoint Name */
    aXLog->mSPNameLen = sArg->mSPNameLen;
    IDE_TEST_RAISE( aXLog->mMemory.alloc( aXLog->mSPNameLen + 1,
                                          (void **)&( aXLog->mSPName ) )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC );

    IDE_TEST( cmtVariableGetData( &sArg->mSPName,
                                  (UChar *)aXLog->mSPName,
                                  sArg->mSPNameLen )
              != IDE_SUCCESS );
    aXLog->mSPName[aXLog->mSPNameLen] = '\0';

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "rpnCommA5::recvSPSetA5",
                                  "mSPName" ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;    
}

IDE_RC rpnComm::sendSPAbortA5( void               * /*aHBTResource*/,
                               cmiProtocolContext * aProtocolContext,
                               idBool             * aExitFlag,
                               smTID                aTID,
                               smSN                 aSN,
                               smSN                 aSyncSN,
                               UInt                 aSPNameLen,
                               SChar              * aSPName,
                               UInt                 aTimeoutSec )
{
     cmiProtocolContext       sProtocolContext;
     cmiProtocol              sProtocol;
     UInt                     sStage          = 0;
     cmiLink                * sLink           = NULL;
     cmpArgRPSPAbort        * sArgSPAbort     = NULL;

     /* Validate Parameters */
     IDE_DASSERT( aSPNameLen != 0 );
     IDE_DASSERT( aSPName    != NULL );

     IDE_TEST( cmiGetLinkForProtocolContext( aProtocolContext, &sLink )
               != IDE_SUCCESS );

     IDE_TEST( cmiInitializeProtocolContext( &sProtocolContext,
                                             CMI_PROTOCOL_MODULE( RP ),
                                             sLink )
               != IDE_SUCCESS );
     sStage = 1;

     cmiProtocolContextCopyA5Header( &sProtocolContext,
                                     &(aProtocolContext->mReadHeader) );

     /* Initialize Protocol */
     CMI_PROTOCOL_INITIALIZE( sProtocol, sArgSPAbort, RP, SPAbort );
     sStage = 2;

     /* Replication XLog Header Set */
     sArgSPAbort->mXLogType  = RP_X_SP_ABORT;
     sArgSPAbort->mTransID   = aTID;
     sArgSPAbort->mSN        = aSN;
     sArgSPAbort->mSyncSN    = aSyncSN;

     /* Additional Information     */
     sArgSPAbort->mSPNameLen = aSPNameLen;
     IDE_TEST( cmtVariableSetData( &sArgSPAbort->mSPName,
                                   (UChar *)aSPName,
                                   aSPNameLen )
               != IDE_SUCCESS );

    /* Send Protocol */
    IDE_TEST( writeProtocol( NULL, /* aHBTResource */
                             &sProtocolContext,
                             &sProtocol )
              != IDE_SUCCESS );

    IDE_TEST( flushProtocol( NULL, /* aHBTResource */
                             &sProtocolContext,
                             aExitFlag,
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

     /* Finalize Protocol */
     sStage = 1;
     IDE_TEST( cmiFinalizeProtocol( &sProtocol ) != IDE_SUCCESS );

     sStage = 0;
     IDE_TEST( cmiFinalizeProtocolContext( &sProtocolContext ) != IDE_SUCCESS );

     return IDE_SUCCESS;

     IDE_EXCEPTION_END;

     IDE_PUSH();

     switch ( sStage )
     {
         case 2:
             (void)cmiFinalizeProtocol( &sProtocol );
             /* fall through */
         case 1:
             (void)cmiFinalizeProtocolContext( &sProtocolContext );
             /* fall through */
         default:
             break;
     }

     IDE_POP();

     return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC rpnComm::recvSPAbortA5( cmiProtocol         * aProtocol,
                               rpdXLog             * aXLog )
{
    cmpArgRPSPAbort * sArg = CMI_PROTOCOL_GET_ARG( *aProtocol, RP, SPAbort );

    /* Get Argument XLog Hdr */
    aXLog->mType    = (rpXLogType)sArg->mXLogType;
    aXLog->mTID     = sArg->mTransID;
    aXLog->mSN      = sArg->mSN;
    aXLog->mSyncSN  = sArg->mSyncSN;

    /* Get Argument Savepoint Name */
    aXLog->mSPNameLen = sArg->mSPNameLen;
    IDE_TEST_RAISE( aXLog->mMemory.alloc( aXLog->mSPNameLen + 1,
                                          (void **)&( aXLog->mSPName ) )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC );

    IDE_TEST( cmtVariableGetData( &sArg->mSPName,
                                  (UChar *)aXLog->mSPName,
                                  sArg->mSPNameLen )
              != IDE_SUCCESS );
    aXLog->mSPName[aXLog->mSPNameLen] = '\0';

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "rpnCommA7::recvSPAbortA5",
                                  "mSPName" ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendInsertA5( void               * /*aHBTResource*/,
                              cmiProtocolContext * aProtocolContext,
                              idBool             * aExitFlag,
                              smTID                aTID,
                              smSN                 aSN,
                              smSN                 aSyncSN,
                              UInt                 aImplSPDepth,
                              ULong                aTableOID,
                              UInt                 aColCnt,
                              smiValue           * aACols,
                              rpValueLen         * aALen,
                              UInt                 aTimeoutSec )
{
    cmiProtocolContext       sProtocolContext;
    cmiProtocol              sProtocol;
    UInt                     sStage          = 0;
    cmiLink                * sLink           = NULL;
    cmpArgRPInsert         * sArgInsert      = NULL;

    UInt i;

    /* Validate Parameters */
    IDE_DASSERT( aImplSPDepth <= SMI_STATEMENT_DEPTH_MAX );
    IDE_DASSERT( aColCnt <= QCI_MAX_COLUMN_COUNT );
    IDE_DASSERT( aACols != NULL );

    IDE_TEST( cmiGetLinkForProtocolContext( aProtocolContext, &sLink )
              != IDE_SUCCESS );

    IDE_TEST( cmiInitializeProtocolContext( &sProtocolContext,
                                            CMI_PROTOCOL_MODULE( RP ),
                                            sLink )
              != IDE_SUCCESS );
    sStage = 1;

    cmiProtocolContextCopyA5Header( &sProtocolContext,
                                    &(aProtocolContext->mReadHeader) );

    /* Initialize Protocol */
    CMI_PROTOCOL_INITIALIZE( sProtocol, sArgInsert, RP, Insert );
    sStage = 2;

    /* Replication XLog Header Set */
    sArgInsert->mXLogType   = RP_X_INSERT;
    sArgInsert->mTransID    = aTID;
    sArgInsert->mSN         = aSN;
    sArgInsert->mSyncSN     = aSyncSN;

    /* Additional Information     */
    sArgInsert->mImplSPDepth = aImplSPDepth;
    sArgInsert->mTableOID    = aTableOID;
    sArgInsert->mColCnt      = aColCnt;

    /* Send Protocol */
    IDE_TEST( writeProtocol( NULL,
                             &sProtocolContext,
                             &sProtocol )
              != IDE_SUCCESS );

    /* Send Value repeatedly */
    for ( i = 0 ; i < aColCnt ; i++ )
    {
        IDE_TEST( sendValueForA5( NULL,
                                  &sProtocolContext,
                                  aExitFlag,
                                  &aACols[i],
                                  aALen[i],
                                  aTimeoutSec )
                  != IDE_SUCCESS );
    }

    /* Finalize Protocol */
    sStage = 1;
    IDE_TEST( cmiFinalizeProtocol( &sProtocol ) != IDE_SUCCESS );

    sStage = 0;
    IDE_TEST( cmiFinalizeProtocolContext( &sProtocolContext ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 2:
            (void)cmiFinalizeProtocol( &sProtocol );
            /* fall through */
        case 1:
            (void)cmiFinalizeProtocolContext( &sProtocolContext );
            break;
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpnComm::recvInsertA5( iduMemAllocator    * aAllocator,
                              idBool             * aExitFlag,
                              cmiProtocolContext * aProtocolContext,
                              cmiProtocol        * aProtocol,
                              rpdMeta            * aMeta,
                              rpdXLog            * aXLog,
                              ULong                aTimeOutSec )
{
    UInt              i = 0;
    cmpArgRPInsert  * sArg    = CMI_PROTOCOL_GET_ARG( *aProtocol, RP, Insert );
    rpdMetaItem     * sItem   = NULL;
    rpdColumn       * sColumn = NULL;
    const mtdModule * sMtd    = NULL;
    UInt              sCID;
    smiValue          sDummy;
    
    IDE_ASSERT( aMeta != NULL );

    /* Get Argument XLog Hdr */
    aXLog->mType        = (rpXLogType)sArg->mXLogType;
    aXLog->mTID         = sArg->mTransID;
    aXLog->mSN          = sArg->mSN;
    aXLog->mSyncSN      = sArg->mSyncSN;

    /* Get Argument XLog Body */
    aXLog->mImplSPDepth = sArg->mImplSPDepth;
    aXLog->mTableOID    = sArg->mTableOID;

    /* PROJ-1442 Replication Online 중 DDL 허용
     * XLog 내의 Column 정보를 재배열한다.
     *
     * Column : mColCnt, mCIDs[], mACols[]
     */
    (void)aMeta->searchRemoteTable( &sItem, aXLog->mTableOID );
    IDE_TEST_RAISE( sItem == NULL, ERR_NOT_FOUND_TABLE );

    aXLog->mColCnt      = sItem->mColCount;

    IDE_TEST_RAISE( aXLog->mMemory.alloc( sArg->mColCnt * ID_SIZEOF( UInt ),
                                          (void**)&( aXLog->mCIDs ) )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC );

    IDE_TEST_RAISE( aXLog->mMemory.alloc( sArg->mColCnt * ID_SIZEOF( smiValue ),
                                          (void**)&( aXLog->mBCols ) )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC );

    IDE_TEST_RAISE( aXLog->mMemory.alloc( sArg->mColCnt * ID_SIZEOF( smiValue ),
                                          (void**)&( aXLog->mACols ) )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC );

    /* Recv Value repeatedly */
    // Column ID 순서로 Column 값을 재배열한다.
    for ( i = 0; i < sArg->mColCnt; i++ )
    {
        if ( sItem->mHasOnlyReplCol == ID_TRUE )
        {
            aXLog->mCIDs[i] = i;
        }

        sCID = sItem->mMapColID[i];

        if ( sCID != RP_INVALID_COLUMN_ID )
        {
            IDE_TEST( recvValueA5( aAllocator,
                                   aExitFlag,
                                   aProtocolContext,
                                   &aXLog->mMemory,
                                   &aXLog->mACols[sCID],
                                   aTimeOutSec )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( recvValueA5( aAllocator,
                                   aExitFlag,
                                   aProtocolContext,
                                   NULL, /* aMemory */
                                   &sDummy,
                                   aTimeOutSec )
                      != IDE_SUCCESS );
            if ( sDummy.value != NULL )
            {
                (void)iduMemMgr::free( (void *)sDummy.value, aAllocator );
            }
        }
    }

    // Standby Server에서 Replication 대상이 아닌 Column에 NULL을 할당한다.
    if ( sItem->mHasOnlyReplCol != ID_TRUE )
    {
        for ( i = 0; i < (UInt)sItem->mColCount; i++ )
        {
            // 재배열된 Column을 XLog에 복사한다.
            aXLog->mCIDs[i] = i;

            // Replication 대상이 아닌 Column은,
            // DEFAULT 값 대신 mtdModule의 staticNull을 사용한다.
            if ( sItem->mIsReplCol[i] != ID_TRUE )
            {
                sColumn = sItem->getRpdColumn( i );
                IDE_TEST_RAISE( sColumn == NULL, ERR_NOT_FOUND_COLUMN );

                IDE_TEST_RAISE(
                    mtd::moduleById( &sMtd,
                                     sColumn->mColumn.type.dataTypeId )
                    != IDE_SUCCESS, ERR_GET_MODULE );

                IDE_TEST( allocNullValue( aAllocator,
                                          &aXLog->mMemory,
                                          &aXLog->mACols[i],
                                          (const mtcColumn *)&sColumn->mColumn,
                                          sMtd )
                          != IDE_SUCCESS );
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_FOUND_TABLE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RECEIVER_NOT_FOUND_TABLE,
                                  aXLog->mSN,
                                  aXLog->mTableOID ) );
    }
    IDE_EXCEPTION( ERR_NOT_FOUND_COLUMN );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RECEIVER_NOT_FOUND_COLUMN,
                                  aXLog->mSN,
                                  aXLog->mTableOID,
                                  i ) );
    }
    IDE_EXCEPTION( ERR_GET_MODULE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_GET_MODULE ) );
    }
    IDE_EXCEPTION( ERR_MEMORY_ALLOC );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET(ideSetErrorCode(rpERR_ABORT_MEMORY_ALLOC,
                                "rpnComm::recvInsertA5",
                                "Column"));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;    
}

IDE_RC rpnComm::sendUpdateA5( void                * /*aHBTResource*/,
                              cmiProtocolContext  * aProtocolContext,
                              idBool              * aExitFlag,
                              smTID                 aTID,
                              smSN                  aSN,
                              smSN                  aSyncSN,
                              UInt                  aImplSPDepth,
                              ULong                 aTableOID,
                              UInt                  aPKColCnt,
                              UInt                  aUpdateColCnt,
                              smiValue            * aPKCols,
                              UInt                * aCIDs,
                              smiValue            * aBCols,
                              smiChainedValue     * aBChainedCols, // PROJ-1705
                              UInt                * /*aBChainedColsTotalLen*/, /* BUG-33722 */
                              smiValue            * aACols,
                              rpValueLen          * aPKLen,
                              rpValueLen          * aALen,
                              rpValueLen          * aBMtdValueLen,
                              UInt                  aTimeoutSec )
{
    cmiProtocolContext       sProtocolContext;
    cmiProtocol              sProtocol;
    UInt                     sStage          = 0;
    cmiLink                * sLink           = NULL;
    cmpArgRPUpdate         * sArgUpdate      = NULL;

    smiChainedValue  * sChainedValue = NULL;
    UInt               i;

    /* Validate Parameters */
    IDE_DASSERT( aImplSPDepth <= SMI_STATEMENT_DEPTH_MAX );
    IDE_DASSERT( aPKColCnt <= QCI_MAX_KEY_COLUMN_COUNT );
    IDE_DASSERT( aUpdateColCnt <= QCI_MAX_COLUMN_COUNT );
    IDE_DASSERT( aPKCols != NULL );
    IDE_DASSERT( aCIDs != NULL) ;
    IDE_DASSERT( aBCols != NULL );
    IDE_DASSERT( aACols != NULL );

    IDE_TEST( cmiGetLinkForProtocolContext( aProtocolContext, &sLink )
              != IDE_SUCCESS );

    IDE_TEST( cmiInitializeProtocolContext( &sProtocolContext,
                                            CMI_PROTOCOL_MODULE( RP ),
                                            sLink )
              != IDE_SUCCESS );
    sStage = 1;

    cmiProtocolContextCopyA5Header( &sProtocolContext,
                                    &(aProtocolContext->mReadHeader) );

    /* Initialize Protocol */
    CMI_PROTOCOL_INITIALIZE( sProtocol, sArgUpdate, RP, Update );
    sStage = 2;

    /* Replication XLog Header Set */
    sArgUpdate->mXLogType   = RP_X_UPDATE;
    sArgUpdate->mTransID    = aTID;
    sArgUpdate->mSN         = aSN;
    sArgUpdate->mSyncSN     = aSyncSN;

    /* Additional Information     */
    sArgUpdate->mImplSPDepth  = aImplSPDepth;
    sArgUpdate->mTableOID     = aTableOID;
    sArgUpdate->mPKColCnt     = aPKColCnt;
    sArgUpdate->mUpdateColCnt = aUpdateColCnt;

    /* Send Protocol */
    IDE_TEST( writeProtocol( NULL,
                             &sProtocolContext,
                             &sProtocol )
              != IDE_SUCCESS );

    /* Send Primary Key Value */
    for ( i = 0 ; i < aPKColCnt ; i++ )
    {
        IDE_TEST( sendPKValueForA5( NULL,
                                    &sProtocolContext,
                                    aExitFlag,
                                    &aPKCols[i],
                                    aPKLen[i],
                                    aTimeoutSec )
                  != IDE_SUCCESS );
    }

    /* Send Update Column Value : Before/After */
    for ( i = 0 ; i < aUpdateColCnt ; i++ )
    {
        IDE_TEST( sendCIDForA5( NULL,
                                &sProtocolContext,
                                aExitFlag,
                                aCIDs[i],
                                aTimeoutSec )
                  != IDE_SUCCESS );

        // PROJ-1705
        sChainedValue = &aBChainedCols[aCIDs[i]];

        // 메모리 테이블에 발생한 update의 분석 결과는 BCols에 들어있다.
        if ( ( sChainedValue->mAllocMethod == SMI_NON_ALLOCED ) && // mAllocMethod의 초기값은 SMI_NON_ALLOCED
             ( aBMtdValueLen[aCIDs[i]].lengthSize == 0 ) )
        {
            IDE_TEST( sendValueForA5( NULL,
                                      &sProtocolContext,
                                      aExitFlag,
                                      &aBCols[aCIDs[i]],
                                      aBMtdValueLen[aCIDs[i]],
                                      aTimeoutSec )
                      != IDE_SUCCESS );
        }
        // 디스크 테이블에 발생한 update의 분석 결과는 BChainedCols에 들어있다.
        else
        {
            IDE_TEST( sendChainedValueForA5( NULL,
                                             &sProtocolContext,
                                             aExitFlag,
                                             sChainedValue,
                                             aBMtdValueLen[aCIDs[i]],
                                             aTimeoutSec )
                      != IDE_SUCCESS );
        }

        IDE_TEST(sendValueForA5(NULL,
                                &sProtocolContext,
                                aExitFlag,
                                &aACols[aCIDs[i]],
                                aALen[aCIDs[i]],
                                aTimeoutSec )
                 != IDE_SUCCESS );
    }

    /* Finalize Protocol */
    sStage = 1;
    IDE_TEST( cmiFinalizeProtocol( &sProtocol ) != IDE_SUCCESS );

    sStage = 0;
    IDE_TEST( cmiFinalizeProtocolContext( &sProtocolContext ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 2:
            (void)cmiFinalizeProtocol( &sProtocol );
            /* fall through */
        case 1:
            (void)cmiFinalizeProtocolContext( &sProtocolContext );
            break;
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpnComm::recvUpdateA5( iduMemAllocator     * aAllocator,
                              idBool              * aExitFlag,
                              cmiProtocolContext  * aProtocolContext,
                              cmiProtocol         * aProtocol,
                              rpdMeta             * aMeta,
                              rpdXLog             * aXLog,
                              ULong                 aTimeOutSec )
{
    UInt              i = 0;
    cmpArgRPUpdate  * sArg  = CMI_PROTOCOL_GET_ARG( *aProtocol, RP, Update );
    rpdMetaItem     * sItem = NULL;
    smiValue          sDummy;
    
    IDE_ASSERT( aMeta != NULL );

    /* Get Argument XLog Hdr */
    aXLog->mType        = (rpXLogType)sArg->mXLogType;
    aXLog->mTID         = sArg->mTransID;
    aXLog->mSN          = sArg->mSN;
    aXLog->mSyncSN      = sArg->mSyncSN;

    /* Get Argument XLog Body */
    aXLog->mImplSPDepth = sArg->mImplSPDepth;
    aXLog->mTableOID    = sArg->mTableOID;
    aXLog->mPKColCnt    = sArg->mPKColCnt;
    aXLog->mColCnt      = sArg->mUpdateColCnt;

    IDE_TEST_RAISE( aXLog->mMemory.alloc( aXLog->mPKColCnt * ID_SIZEOF( smiValue ),
                                          (void**)&( aXLog->mPKCols ) )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC );

    IDE_TEST_RAISE( aXLog->mMemory.alloc( aXLog->mColCnt * ID_SIZEOF( UInt ),
                                          (void**)&( aXLog->mCIDs ) )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC );

    IDE_TEST_RAISE( aXLog->mMemory.alloc( aXLog->mColCnt * ID_SIZEOF( smiValue ),
                                          (void**)&( aXLog->mBCols ) )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC );

    IDE_TEST_RAISE( aXLog->mMemory.alloc( aXLog->mColCnt * ID_SIZEOF( smiValue ),
                                          (void**)&( aXLog->mACols ) )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC );

    /* Recv PK Value repeatedly */
    for ( i = 0; i < aXLog->mPKColCnt; i++ )
    {
        IDE_TEST( recvValueA5( aAllocator,
                               aExitFlag,
                               aProtocolContext,
                               &aXLog->mMemory,
                               &aXLog->mPKCols[i],
                               aTimeOutSec )
                  != IDE_SUCCESS );
    }

    /* PROJ-1442 Replication Online 중 DDL 허용
     * XLog 내의 Column 정보를 재배열한다.
     *   - mPKColCnt는 변하지 않으므로, 제외한다.
     *   - Primary Key의 순서가 같으므로, mPKCIDs[]와 mPKCols[]는 제외한다.
     *
     * Column : mColCnt, mCIDs[], mBCols[], mACols[]
     */
    (void)aMeta->searchRemoteTable( &sItem, aXLog->mTableOID );
    IDE_TEST_RAISE( sItem == NULL, ERR_NOT_FOUND_TABLE );

    /* Recv Update Value repeatedly */
    for ( i = 0; i < aXLog->mColCnt; i++ )
    {
        IDE_TEST( recvCIDA5( aExitFlag,
                             aProtocolContext,
                             &aXLog->mCIDs[i],
                             aTimeOutSec )
                  != IDE_SUCCESS );

        aXLog->mCIDs[i] = sItem->mMapColID[aXLog->mCIDs[i]];

        if ( aXLog->mCIDs[i] != RP_INVALID_COLUMN_ID )
        {
            IDE_TEST( recvValueA5( aAllocator,
                                   aExitFlag,
                                   aProtocolContext,
                                   &aXLog->mMemory,
                                   &aXLog->mBCols[i],
                                   aTimeOutSec )
                      != IDE_SUCCESS );
            IDE_TEST( recvValueA5( aAllocator,
                                   aExitFlag,
                                   aProtocolContext,
                                   &aXLog->mMemory,
                                   &aXLog->mACols[i],
                                   aTimeOutSec )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( recvValueA5( aAllocator,
                                   aExitFlag,
                                   aProtocolContext,
                                   NULL, /* aMemory */
                                   &sDummy,
                                   aTimeOutSec )
                      != IDE_SUCCESS );
            if ( sDummy.value != NULL )
            {
                (void)iduMemMgr::free( (void *)sDummy.value, aAllocator );
            }
            else
            {
                /* nothing to do */
            }
            
            IDE_TEST( recvValueA5( aAllocator,
                                   aExitFlag,
                                   aProtocolContext,
                                   NULL, /* aMemory */
                                   &sDummy,
                                   aTimeOutSec )
                      != IDE_SUCCESS );
            if ( sDummy.value != NULL )
            {
                (void)iduMemMgr::free( (void *)sDummy.value, aAllocator );
            }
            else
            {
                /* nothing to do */
            }
            
            i--;
            aXLog->mColCnt--;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_FOUND_TABLE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RECEIVER_NOT_FOUND_TABLE,
                                  aXLog->mSN,
                                  aXLog->mTableOID ) );
    }
    IDE_EXCEPTION( ERR_MEMORY_ALLOC );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET(ideSetErrorCode(rpERR_ABORT_MEMORY_ALLOC,
                                "rpnComm::recvUpdateA5",
                                "Column"));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendDeleteA5( void              * /*aHBTResource*/,
                              cmiProtocolContext* aProtocolContext,
                              idBool            * aExitFlag,
                              smTID               aTID,
                              smSN                aSN,
                              smSN                aSyncSN,
                              UInt                aImplSPDepth,
                              ULong               aTableOID,
                              UInt                aPKColCnt,
                              smiValue          * aPKCols,
                              rpValueLen        * aPKLen,
                              UInt                /*aColCnt*/,
                              UInt              * /*aCIDs*/,
                              smiValue          * /*aBCols*/,
                              smiChainedValue   * /*aBChainedCols*/, // PROJ-1705
                              rpValueLen        * /*aBMtdValueLen*/,
                              UInt              * /*aBChainedColsTotalLen*/,
                              UInt                aTimeoutSec )
{
    cmiProtocolContext       sProtocolContext;
    cmiProtocol              sProtocol;
    UInt                     sStage          = 0;
    cmiLink                * sLink           = NULL;
    cmpArgRPDelete         * sArgDelete      = NULL;

    UInt               i;

    /* Validate Parameters */
    IDE_DASSERT( aImplSPDepth <= SMI_STATEMENT_DEPTH_MAX );
    IDE_DASSERT( aPKColCnt    <= QCI_MAX_KEY_COLUMN_COUNT );
    IDE_DASSERT( aPKCols      != NULL );

    IDE_TEST( cmiGetLinkForProtocolContext( aProtocolContext, &sLink )
              != IDE_SUCCESS );

    IDE_TEST( cmiInitializeProtocolContext( &sProtocolContext,
                                            CMI_PROTOCOL_MODULE( RP ),
                                            sLink )
              != IDE_SUCCESS );
    sStage = 1;

    cmiProtocolContextCopyA5Header( &sProtocolContext,
                                    &(aProtocolContext->mReadHeader) );

    /* Initialize Protocol */
    CMI_PROTOCOL_INITIALIZE( sProtocol, sArgDelete, RP, Delete );
    sStage = 2;

    /* Replication XLog Header Set */
    sArgDelete->mXLogType = RP_X_DELETE;
    sArgDelete->mTransID  = aTID;
    sArgDelete->mSN       = aSN;
    sArgDelete->mSyncSN   = aSyncSN;

    /* Additional Information     */
    sArgDelete->mImplSPDepth = aImplSPDepth;
    sArgDelete->mTableOID    = aTableOID;
    sArgDelete->mPKColCnt    = aPKColCnt;

    /* Send Protocol */
    IDE_TEST( writeProtocol( NULL,
                             &sProtocolContext,
                             &sProtocol )
              != IDE_SUCCESS );

    /* Send Primary Key Value */
    for ( i = 0 ; i < aPKColCnt ; i++ )
    {
        IDE_TEST( sendPKValueForA5( NULL,
                                    &sProtocolContext,
                                    aExitFlag,
                                    &aPKCols[i],
                                    aPKLen[i],
                                    aTimeoutSec )
                  != IDE_SUCCESS );
    }

    /* Finalize Protocol */
    sStage = 1;
    IDE_TEST( cmiFinalizeProtocol( &sProtocol ) != IDE_SUCCESS );

    sStage = 0;
    IDE_TEST( cmiFinalizeProtocolContext( &sProtocolContext ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 2:
            (void)cmiFinalizeProtocol( &sProtocol );
            /* fall through */
        case 1:
            (void)cmiFinalizeProtocolContext( &sProtocolContext );
            break;
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}


/*
 *
 */
IDE_RC rpnComm::recvDeleteA5( iduMemAllocator     * aAllocator,
                              idBool              * aExitFlag,
                              cmiProtocolContext  * aProtocolContext,
                              cmiProtocol         * aProtocol,
                              rpdXLog             * aXLog,
                              ULong                 aTimeOutSec )
{
    UInt             i;
    cmpArgRPDelete * sArg = CMI_PROTOCOL_GET_ARG( *aProtocol, RP, Delete );

    /* Get Argument XLog Hdr */
    aXLog->mType        = (rpXLogType)sArg->mXLogType;
    aXLog->mTID         = sArg->mTransID;
    aXLog->mSN          = sArg->mSN;
    aXLog->mSyncSN      = sArg->mSyncSN;

    /* Get Argument XLog Body */
    aXLog->mImplSPDepth = sArg->mImplSPDepth;
    aXLog->mTableOID    = sArg->mTableOID;
    aXLog->mPKColCnt    = sArg->mPKColCnt;

    IDE_TEST_RAISE( aXLog->mMemory.alloc( aXLog->mPKColCnt * ID_SIZEOF( smiValue ),
                                          (void**)&( aXLog->mPKCols ) )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC );

    /* Recv PK Value repeatedly */
    for ( i = 0; i < aXLog->mPKColCnt; i++ )
    {
        IDE_TEST( recvValueA5( aAllocator,
                               aExitFlag,
                               aProtocolContext,
                               &aXLog->mMemory,
                               &aXLog->mPKCols[i],
                               aTimeOutSec )
                  != IDE_SUCCESS );
    }

    /* PROJ-1442 Replication Online 중 DDL 허용
     * XLog 내의 Column 정보를 재배열한다.
     *   - mPKColCnt는 변하지 않으므로, 제외한다.
     *   - Primary Key의 순서가 같으므로, mPKCIDs[]와 mPKCols[]는 제외한다.
     */

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET(ideSetErrorCode(rpERR_ABORT_MEMORY_ALLOC,
                                "rpnComm::recvDeleteA5",
                                "Column"));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;    
}

IDE_RC rpnComm::sendCIDForA5( void               * /*aHBTResource*/,
                              cmiProtocolContext * aProtocolContext,
                              idBool             * /*aExitFlag*/,
                              UInt                 aCID,
                              UInt                 /*aTimeoutSec*/ )
{
    cmiProtocol      sProtocol;
    cmpArgRPUIntID * sArgUIntID  = NULL;
    UInt             sStage      = 0;

    /* Validate Parameters */
    IDE_DASSERT( aCID < QCI_MAX_COLUMN_COUNT );

    /* Initialize Protocol */
    CMI_PROTOCOL_INITIALIZE( sProtocol, sArgUIntID, RP, UIntID );
    sStage = 1;

    /* Set Communication Argument */
    sArgUIntID->mUIntID = aCID;

    /* Send Protocol */
    IDE_TEST( writeProtocol( NULL,
                             aProtocolContext,
                             &sProtocol )
              != IDE_SUCCESS );

    /* Finalize Protocol */
    sStage = 0;
    IDE_TEST( cmiFinalizeProtocol( &sProtocol ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 1:
            (void)cmiFinalizeProtocol( &sProtocol );
            break;
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpnComm::recvCIDA5( idBool             * aExitFlag,
                           cmiProtocolContext * aProtocolContext,
                           UInt               * aCID,
                           ULong                aTimeOutSec )
{
    cmiProtocol sProtocol;
    SInt sStage = 0;
    cmpArgRPUIntID * sArg = NULL;

    IDE_TEST( readCmProtocol( aProtocolContext,
                              &sProtocol,
                              aExitFlag,
                              NULL,
                              aTimeOutSec )
              != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST_RAISE( sProtocol.mOpID != CMI_PROTOCOL_OPERATION( RP, UIntID ),
                    ERR_CHECK_OPERATION_TYPE );

    /* Get Argument */
    sArg = CMI_PROTOCOL_GET_ARG( sProtocol, RP, UIntID );
    *aCID = sArg->mUIntID;

    /* Finalize Protocol */
    sStage = 0;
    IDE_TEST( cmiFinalizeProtocol( &sProtocol ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CHECK_OPERATION_TYPE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_WRONG_OPERATION_TYPE,
                                  sProtocol.mOpID ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 1:
            (void)cmiFinalizeProtocol( &sProtocol );
        default:
            break;
    }
    
    IDE_POP();

    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC rpnComm::sendTxAckA5( cmiProtocolContext * aProtocolContext,
                             idBool             * /*aExitFlag*/,
                             smTID                aTID,
                             smSN                 aSN,
                             UInt                 /*aTimeoutSec*/ )
{
    cmiProtocol sProtocol;
    cmpArgRPTxAck * sArg = NULL;
    SInt sStage = 0;
    
    /* Initialize Protocol */
    CMI_PROTOCOL_INITIALIZE( sProtocol, sArg, RP, TxAck );
    sStage = 1;

    /* Set Communication Argument */
    sArg->mTID = (UInt)aTID;
    sArg->mSN  = (ULong)aSN;

    /* Send Protocol */
    IDE_TEST( writeProtocol( NULL,
                             aProtocolContext,
                             &sProtocol )
              != IDE_SUCCESS );

    /* Finalize Protocol */
    sStage = 0;
    IDE_TEST( cmiFinalizeProtocol( &sProtocol ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 1:
            (void)cmiFinalizeProtocol( &sProtocol );
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC rpnComm::recvTxAckForA5( idBool             * aExitFlag,
                                cmiProtocolContext * aProtocolContext,
                                smTID              * aTID,
                                smSN               * aSN,
                                ULong                aTimeoutSec )
{
    cmiProtocol     sProtocol;
    SInt            sStage    = 0;
    cmpArgRPTxAck * sArgTxAck = NULL;

    IDE_TEST( readCmProtocol( aProtocolContext,
                              &sProtocol,
                              aExitFlag,
                              NULL,
                              aTimeoutSec )
              != IDE_SUCCESS );
    sStage = 1;

    /* Validation Ack Type */
    IDE_TEST_RAISE( sProtocol.mOpID != CMI_PROTOCOL_OPERATION( RP, TxAck ),
                    ERR_CHECK_OPERATION_TYPE );

    /* Get Argument */
    sArgTxAck = CMI_PROTOCOL_GET_ARG( sProtocol, RP, TxAck );
    *aTID     = (smTID)sArgTxAck->mTID;
    *aSN      = (smSN)sArgTxAck->mSN;

    /* Finalize Protocol */
    sStage = 0;
    IDE_TEST( cmiFinalizeProtocol( &sProtocol ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CHECK_OPERATION_TYPE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_WRONG_OPERATION_TYPE,
                                  sProtocol.mOpID ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 1:
            (void)cmiFinalizeProtocol( &sProtocol );
            break;
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendValueForA5( void               * /*aHBTResource*/,
                                cmiProtocolContext * aProtocolContext,
                                idBool             * /*aExitFlag*/,
                                smiValue           * aValue,
                                rpValueLen           aValueLen,
                                UInt                 /*aTimeoutSec*/ )
{
    cmiProtocol     sProtocol;
    cmpArgRPValue * sArgValue = NULL;
    UInt            sStage    = 0;

    UChar           sOneByteLenValue;

    /* Validate Parameters */
    IDE_DASSERT(aValue != NULL);

    /* Initialize Protocol */
    CMI_PROTOCOL_INITIALIZE( sProtocol, sArgValue, RP, Value );
    sStage = 1;
    /* Set Communication Argument */

    /* Send mtdValueLength */
    if ( aValueLen.lengthSize > 0 )
    {
        // UShort를 UChar 포인터로 1바이트만 보내는 경우, endian문제가 발생한다.
        // UChar 변수에 담는 과정을 거치도록한다.
        if ( aValueLen.lengthSize == 1 )
        {
            sOneByteLenValue = (UChar) aValueLen.lengthValue;
            IDE_TEST( cmtVariableSetData( &sArgValue->mValue,
                                          &sOneByteLenValue,
                                          aValueLen.lengthSize )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( cmtVariableSetData( &sArgValue->mValue,
                                          (UChar *)&aValueLen.lengthValue,
                                          aValueLen.lengthSize )
                      != IDE_SUCCESS );
        }

        IDE_TEST( cmtVariableAddPiece( &sArgValue->mValue,
                                       aValueLen.lengthSize,
                                       aValue->length,
                                       (UChar *)aValue->value )
                  != IDE_SUCCESS );
    }
    // memoryTable, non-divisible value 또는 null value인 경우, rpValueLen정보가 초기값이다.
    else
    {
        IDE_TEST( cmtVariableSetData( &sArgValue->mValue,
                                      (UChar *)aValue->value,
                                      aValue->length )
                  != IDE_SUCCESS );
    }

    /* Send Protocol */
    IDE_TEST( writeProtocol( NULL,
                             aProtocolContext,
                             &sProtocol )
              != IDE_SUCCESS );

    /* Finalize Protocol */
    sStage = 0;
    IDE_TEST( cmiFinalizeProtocol( &sProtocol ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 1:
            (void)cmiFinalizeProtocol( &sProtocol );
            break;
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendPKValueForA5( void                * /*aHBTHostResource*/,
                                  cmiProtocolContext  * aProtocolContext,
                                  idBool              * /*aExitFlag*/,
                                  smiValue            * aValue,
                                  rpValueLen            aPKLen,
                                  UInt                  /*aTimeoutSec*/ )
{
    cmiProtocol     sProtocol;
    cmpArgRPValue * sArgValue = NULL;
    UInt            sStage    = 0;

    UChar           sOneByteLenValue;

    /* Validate Parameters */
    IDE_DASSERT( aValue != NULL );

    /*
     * PROJ-1705로 인해,
     * divisible data type인 경우, null value이면, 앞에 mtdLength만큼의 길이를 length로 가지나,
     * value에서 그 길이 만큼의 데이터를 저장하고 있지는 않으므로,
     * (rpLenSize 에서 그 값을 가지고 있다.) 다른 함수에서는 동일한 내용의 ASSERT를 해제하였으나,
     * PK Value는 null이 올 수 없으므로 유지시킴.
     */
    IDE_DASSERT( ( aValue->value != NULL ) || ( aValue->length == 0 ) );

    /* Initialize Protocol */
    CMI_PROTOCOL_INITIALIZE( sProtocol, sArgValue, RP, Value);
    sStage = 1;

    /* Set Communication Argument */

    /* Send mtdValueLength */
    if ( aPKLen.lengthSize > 0 )
    {
        // UShort를 UChar 포인터로 1바이트만 보내는 경우, endian문제가 발생한다.
        // UChar 변수에 담는 과정을 거치도록한다.
        if ( aPKLen.lengthSize == 1 )
        {
            sOneByteLenValue = (UChar) aPKLen.lengthValue;
            IDE_TEST( cmtVariableSetData( &sArgValue->mValue,
                                          &sOneByteLenValue,
                                          aPKLen.lengthSize )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( cmtVariableSetData( &sArgValue->mValue,
                                          (UChar *)&aPKLen.lengthValue,
                                          aPKLen.lengthSize )
                      != IDE_SUCCESS );
        }

        IDE_TEST( cmtVariableAddPiece( &sArgValue->mValue,
                                       aPKLen.lengthSize,
                                       aValue->length,
                                       (UChar *)aValue->value )
                  != IDE_SUCCESS );
    }
    // memoryTable, non-divisible value 또는 null value인 경우, rpValueLen정보가 없다.
    else
    {
        IDE_TEST( cmtVariableSetData( &sArgValue->mValue,
                                      (UChar *)aValue->value,
                                      aValue->length )
                  != IDE_SUCCESS );
    }

    /* Send Protocol */
    IDE_TEST( writeProtocol( NULL,
                             aProtocolContext,
                             &sProtocol )
              != IDE_SUCCESS );

    /* Finalize Protocol */
    sStage = 0;
    IDE_TEST( cmiFinalizeProtocol( &sProtocol ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 1:
            (void) cmiFinalizeProtocol(&sProtocol);
            break;
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendChainedValueForA5( void               * /*aHBTHostResource*/,
                                       cmiProtocolContext * aProtocolContext,
                                       idBool             * /*aExitFlag*/,
                                       smiChainedValue    * aChainedValue,
                                       rpValueLen           aBMtdValueLen,
                                       UInt                 /*aTimeoutSec*/ )
{
    cmiProtocol       sProtocol;
    cmpArgRPValue   * sArgValue  = NULL;
    UInt              sStage     = 0;

    smiChainedValue * sChainedValue = NULL;
    UInt              sSkipSize     = 0;
    UChar             sOneByteLenValue;

    /* Validate Parameters */
    IDE_DASSERT( aChainedValue != NULL );

    sChainedValue = aChainedValue;

    /* Initialize Protocol */
    CMI_PROTOCOL_INITIALIZE( sProtocol, sArgValue, RP, Value );
    sStage = 1;

    /* Set Communication Argument */
    if ( sChainedValue != NULL )
    {
        /* Send mtdValue */
        if ( aBMtdValueLen.lengthSize > 0 )
        {
            /*
             * UShort를 UChar 포인터로 1바이트만 보내는 경우, endian문제가 발생한다.
             * UChar 변수에 담는 과정을 거치도록한다.
             */
            if ( aBMtdValueLen.lengthSize == 1 )
            {
                sOneByteLenValue = (UChar) aBMtdValueLen.lengthValue;
                IDE_TEST( cmtVariableSetData( &sArgValue->mValue,
                                              &sOneByteLenValue,
                                              aBMtdValueLen.lengthSize )
                          != IDE_SUCCESS);
            }
            else
            {
                IDE_TEST( cmtVariableSetData( &sArgValue->mValue,
                                              (UChar *)&aBMtdValueLen.lengthValue,
                                              aBMtdValueLen.lengthSize )
                          != IDE_SUCCESS);
            }
            /* divisible 데이터타입의 컬럼 value가 NULL인 경우,
             * mtdValue가 mtdValueLenth정보만으로 구성된다.
             */
            if(sChainedValue->mColumn.value != NULL)
            {
                sSkipSize = aBMtdValueLen.lengthSize;
                IDE_TEST( cmtVariableAddPiece( &sArgValue->mValue,
                                               sSkipSize,
                                               sChainedValue->mColumn.length,
                                               (UChar *)sChainedValue->mColumn.value )
                          != IDE_SUCCESS);
            }
            else
            {
                /* divisible 데이터타입의 컬럼 value가 NULL인 경우,
                 * mtdValue가 mtdValueLenth정보만으로 구성된다.
                 */
                // Nothing to do.
            }
        }
        else
        {
            IDE_TEST( cmtVariableSetData( &sArgValue->mValue,
                                          (UChar *)sChainedValue->mColumn.value,
                                          sChainedValue->mColumn.length )
                      != IDE_SUCCESS);
        }

        sChainedValue = sChainedValue->mLink;
        while ( sChainedValue != NULL )
        {
            sSkipSize = RPU_REPLICATION_POOL_ELEMENT_SIZE + sSkipSize;
            IDE_TEST( cmtVariableAddPiece( &sArgValue->mValue,
                                           sSkipSize,
                                           sChainedValue->mColumn.length,
                                           (UChar *)sChainedValue->mColumn.value )
                      != IDE_SUCCESS);

            sChainedValue = sChainedValue->mLink;
        }
    }
    else
    {
        IDE_DASSERT( ID_FALSE );
    }

    /* Send Protocol */
    IDE_TEST( writeProtocol( NULL,
                             aProtocolContext,
                             &sProtocol )
              != IDE_SUCCESS);

    /* Finalize Protocol */
    sStage = 0;
    IDE_TEST( cmiFinalizeProtocol( &sProtocol ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 1:
            (void)cmiFinalizeProtocol( &sProtocol );
            break;
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
 *
 */  
IDE_RC rpnComm::recvValueA5( iduMemAllocator    * aAllocator,
                             idBool             * aExitFlag,
                             cmiProtocolContext * aProtocolContext,
                             iduMemory          * aMemory,
                             smiValue           * aValue,
                             ULong                aTimeOutSec )
{
    cmiProtocol sProtocol;
    SInt sStage = 0;
    cmpArgRPValue * sArg = NULL;

    if ( aValue != NULL )
    {
        aValue->value = NULL;        
    }
    else
    {
        /* nothing to do */
    }

    IDE_TEST( readCmProtocol( aProtocolContext,
                              &sProtocol,
                              aExitFlag,
                              NULL,
                              aTimeOutSec )
              != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST_RAISE( sProtocol.mOpID != CMI_PROTOCOL_OPERATION( RP, Value ),
                    ERR_CHECK_OPERATION_TYPE );
    
    /* Get Argument */
    sArg = CMI_PROTOCOL_GET_ARG( sProtocol, RP, Value );

    if ( aValue != NULL )
    {
        aValue->length = cmtVariableGetSize( &sArg->mValue );

        /* NOTICE!!!!!
         * Sync PK의 경우, 직접 메모리를 할당받는다. free에 신경써야 한다.
         */
        if ( aValue->length != 0 )
        {
            /* NULL value가 전송되는 경우에는 length가 0으로 넘어오게
             * 되므로, 이 경우에는 메모리 할당을 하지 않도록 한다.
             */
            if ( aMemory != NULL )
            {
                IDE_TEST_RAISE( aMemory->alloc( aValue->length,
                                                (void **)&aValue->value )
                                != IDE_SUCCESS, ERR_MEMORY_ALLOC_VALUE );
            }
            else
            {
                IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_RP_RPN,
                                                   aValue->length,
                                                   (void **)&aValue->value,
                                                   IDU_MEM_IMMEDIATE,
                                                   aAllocator )
                                != IDE_SUCCESS, ERR_MEMORY_ALLOC_VALUE );
                sStage = 2;
            }

            IDE_TEST( cmtVariableGetData( &sArg->mValue,
                                          (UChar *)aValue->value,
                                          aValue->length )
                      != IDE_SUCCESS );
        }
        else
        {
            /* nothing to do */
        }

    }
    else
    {
        /* nothing to do */
    }
    
    /* Finalize Protocol */
    sStage = 0;
    IDE_TEST( cmiFinalizeProtocol( &sProtocol ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CHECK_OPERATION_TYPE );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_WRONG_OPERATION_TYPE,
                                  sProtocol.mOpID ) );
        IDE_ERRLOG( IDE_RP_0 );
    }
    IDE_EXCEPTION( ERR_MEMORY_ALLOC_VALUE );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "rpnComm::recvValueA5",
                                  "aValue->value" ) );
        IDE_ERRLOG( IDE_RP_0 );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 2:
            (void)iduMemMgr::free( (void *)aValue->value, aAllocator );
            aValue->value = NULL;
            /* fall through */
        case 1:
            (void)cmiFinalizeProtocol( &sProtocol );
            break;
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendStopA5( void               * /*aHBTResource*/,
                            cmiProtocolContext * aProtocolContext,
                            idBool             * aExitFlag,
                            smTID                aTID,
                            smSN                 aSN,
                            smSN                 aSyncSN,
                            smSN                 aRestartSN,
                            UInt                 aTimeoutSec )
{
    cmiProtocolContext       sProtocolContext;
    cmiProtocol              sProtocol;
    UInt                     sStage          = 0;
    cmiLink                * sLink           = NULL;
    cmpArgRPStop           * sArgStop        = NULL;

    IDE_TEST( cmiGetLinkForProtocolContext( aProtocolContext, &sLink )
              != IDE_SUCCESS );

    IDE_TEST( cmiInitializeProtocolContext( &sProtocolContext,
                                            CMI_PROTOCOL_MODULE( RP ),
                                            sLink )
              != IDE_SUCCESS );
    sStage = 1;

    cmiProtocolContextCopyA5Header( &sProtocolContext,
                                    &(aProtocolContext->mReadHeader) );

    /* Initialize Protocol */
    CMI_PROTOCOL_INITIALIZE( sProtocol, sArgStop, RP, Stop);
    sStage = 2;

    /* Replication XLog Header Set */
    sArgStop->mXLogType = RP_X_REPL_STOP;
    sArgStop->mTransID  = aTID;
    sArgStop->mSN       = aSN;
    sArgStop->mSyncSN   = aSyncSN;

    // BUG-17748
    sArgStop->mRestartSN = aRestartSN;

    /* Send Protocol */
    IDE_TEST( writeProtocol( NULL,
                             &sProtocolContext,
                             &sProtocol )
              != IDE_SUCCESS );

    IDE_TEST( flushProtocol( NULL,
                             &sProtocolContext,
                             aExitFlag,
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Finalize Protocol */
    sStage = 1;
    IDE_TEST( cmiFinalizeProtocol( &sProtocol ) != IDE_SUCCESS );

    sStage = 0;
    IDE_TEST( cmiFinalizeProtocolContext( &sProtocolContext ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 2:
            (void)cmiFinalizeProtocol( &sProtocol );
            /* fall through */
        case 1:
            (void)cmiFinalizeProtocolContext( &sProtocolContext );
            break;
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpnComm::recvStopA5( cmiProtocol        * aProtocol,
                            rpdXLog            * aXLog )
{
    cmpArgRPStop * sArg = CMI_PROTOCOL_GET_ARG( *aProtocol, RP, Stop );

    /* Get Argument XLog Hdr */
    aXLog->mType    = (rpXLogType)sArg->mXLogType;
    aXLog->mTID     = sArg->mTransID;
    aXLog->mSN      = sArg->mSN;
    aXLog->mSyncSN  = sArg->mSyncSN;

    // BUG-17748
    aXLog->mRestartSN = sArg->mRestartSN;

    return IDE_SUCCESS;
}

IDE_RC rpnComm::sendKeepAliveA5( void               * /*aHBTResource*/,
                                 cmiProtocolContext * aProtocolContext,
                                 idBool             * aExitFlag,
                                 smTID                aTID,
                                 smSN                 aSN,
                                 smSN                 aSyncSN,
                                 smSN                 aRestartSN,
                                 UInt                 aTimeoutSec )
{
    cmiProtocolContext       sProtocolContext;
    cmiProtocol              sProtocol;
    UInt                     sStage          = 0;
    cmiLink                * sLink           = NULL;
    cmpArgRPKeepAlive      * sArgKeepAlive   = NULL;

    IDE_TEST( cmiGetLinkForProtocolContext( aProtocolContext, &sLink )
              != IDE_SUCCESS );

    IDE_TEST( cmiInitializeProtocolContext( &sProtocolContext,
                                            CMI_PROTOCOL_MODULE( RP ),
                                            sLink )
              != IDE_SUCCESS );
    sStage = 1;

    cmiProtocolContextCopyA5Header( &sProtocolContext,
                                    &(aProtocolContext->mReadHeader) );

    /* Initialize Protocol */
    CMI_PROTOCOL_INITIALIZE( sProtocol, sArgKeepAlive, RP, KeepAlive );
    sStage = 2;

    /* Replication XLog Header Set */
    sArgKeepAlive->mXLogType = RP_X_KEEP_ALIVE;
    sArgKeepAlive->mTransID  = aTID;
    sArgKeepAlive->mSN       = aSN;
    sArgKeepAlive->mSyncSN   = aSyncSN;

    // BUG-17748
    sArgKeepAlive->mRestartSN = aRestartSN;

    // BUG-17748
    sArgKeepAlive->mRestartSN = aRestartSN;

    /* Send Protocol */
    IDE_TEST( writeProtocol( NULL,
                             &sProtocolContext,
                             &sProtocol )
              != IDE_SUCCESS );

    IDE_TEST( flushProtocol( NULL,
                             &sProtocolContext,
                             aExitFlag,
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Finalize Protocol */
    sStage = 1;
    IDE_TEST( cmiFinalizeProtocol( &sProtocol ) != IDE_SUCCESS );

    sStage = 0;
    IDE_TEST( cmiFinalizeProtocolContext( &sProtocolContext ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 2:
            (void)cmiFinalizeProtocol( &sProtocol );
            /* fall through */
        case 1:
            (void)cmiFinalizeProtocolContext( &sProtocolContext );
            break;
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpnComm::recvKeepAliveA5( cmiProtocol        * aProtocol,
                                 rpdXLog            * aXLog )
{
    cmpArgRPKeepAlive * sArg =
        CMI_PROTOCOL_GET_ARG( *aProtocol, RP, KeepAlive );

    /* Get Argument XLog Hdr */
    aXLog->mType    = (rpXLogType)sArg->mXLogType;
    aXLog->mTID     = sArg->mTransID;
    aXLog->mSN      = sArg->mSN;
    aXLog->mSyncSN  = sArg->mSyncSN;

    // BUG-17748
    aXLog->mRestartSN = sArg->mRestartSN;

    return IDE_SUCCESS;
}

IDE_RC rpnComm::sendFlushA5( void               * /*aHBTResource*/,
                             cmiProtocolContext * aProtocolContext,
                             idBool             * aExitFlag,
                             smTID                aTID,
                             smSN                 aSN,
                             smSN                 aSyncSN,
                             UInt                 aFlushOption,
                             UInt                 aTimeoutSec )
{
    cmiProtocolContext       sProtocolContext;
    cmiProtocol              sProtocol;
    UInt                     sStage          = 0;
    cmiLink                * sLink           = NULL;
    cmpArgRPFlush          * sArgFlush       = NULL;

    IDE_TEST( cmiGetLinkForProtocolContext( aProtocolContext, &sLink )
              != IDE_SUCCESS );

    IDE_TEST( cmiInitializeProtocolContext( &sProtocolContext,
                                            CMI_PROTOCOL_MODULE( RP ),
                                            sLink )
              != IDE_SUCCESS );
    sStage = 1;

    cmiProtocolContextCopyA5Header( &sProtocolContext,
                                    &(aProtocolContext->mReadHeader) );

    /* Initialize Protocol */
    CMI_PROTOCOL_INITIALIZE( sProtocol, sArgFlush, RP, Flush );
    sStage = 2;

    /* Replication XLog Header Set */
    sArgFlush->mXLogType = RP_X_FLUSH;
    sArgFlush->mTransID  = aTID;
    sArgFlush->mSN       = aSN;
    sArgFlush->mSyncSN   = aSyncSN;

    /* Additional Information */
    sArgFlush->mOption   = aFlushOption;

    /* Send Protocol */
    IDE_TEST( writeProtocol( NULL,
                             &sProtocolContext,
                             &sProtocol )
              != IDE_SUCCESS );

    IDE_TEST( flushProtocol( NULL,
                             &sProtocolContext,
                             aExitFlag,
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Finalize Protocol */
    sStage = 1;
    IDE_TEST( cmiFinalizeProtocol( &sProtocol ) != IDE_SUCCESS );

    sStage = 0;
    IDE_TEST( cmiFinalizeProtocolContext( &sProtocolContext ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 2:
            (void)cmiFinalizeProtocol( &sProtocol );
            /* fall through */
        case 1:
            (void)cmiFinalizeProtocolContext( &sProtocolContext );
            break;
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpnComm::recvFlushA5( cmiProtocol         * aProtocol,
                             rpdXLog             * aXLog )
{
    cmpArgRPFlush *sArg = CMI_PROTOCOL_GET_ARG( *aProtocol, RP, Flush );

    /* Get Argument XLog Hdr */
    aXLog->mType    = (rpXLogType)sArg->mXLogType;
    aXLog->mTID     = sArg->mTransID;
    aXLog->mSN      = sArg->mSN;
    aXLog->mSyncSN  = sArg->mSyncSN;

    /* Get Flush Option */
    aXLog->mFlushOption = sArg->mOption;

    return IDE_SUCCESS;    
}

/*
 *
 */
IDE_RC rpnComm::sendAckA5( cmiProtocolContext * aProtocolContext,
                           idBool             * aExitFlag,
                           rpXLogAck          * aAck,
                           UInt                 aTimeoutSec )
{
    cmiProtocolContext sProtocolContext;
    cmiLink * sLink = NULL;
    cmiProtocol sProtocol;
    cmpArgRPAck * sAck = NULL;
    UInt i = 0;
    SInt sStage = 0;

    IDE_TEST( cmiGetLinkForProtocolContext( aProtocolContext, &sLink )
              != IDE_SUCCESS );
    
    IDE_TEST( cmiInitializeProtocolContext( &sProtocolContext,
                                            CMI_PROTOCOL_MODULE( RP ),
                                            sLink )
              != IDE_SUCCESS );
    sStage = 1;

    cmiProtocolContextCopyA5Header( &sProtocolContext,
                                    &(aProtocolContext->mReadHeader) );
    
    /* Initialize Protocol */
    CMI_PROTOCOL_INITIALIZE( sProtocol, sAck, RP, Ack );
    sStage = 2;

    /* Replication XLog Header Set */
    sAck->mXLogType         = aAck->mAckType;
    sAck->mAbortTxCount     = aAck->mAbortTxCount;
    sAck->mClearTxCount     = aAck->mClearTxCount;
    sAck->mRestartSN        = aAck->mRestartSN;
    sAck->mLastCommitSN     = aAck->mLastCommitSN;
    sAck->mLastArrivedSN    = aAck->mLastArrivedSN;
    sAck->mLastProcessedSN  = aAck->mLastProcessedSN; 
    sAck->mFlushSN          = aAck->mFlushSN;

    /* Send Protocol */
    IDE_TEST( writeProtocol( NULL,
                             &sProtocolContext, 
                             &sProtocol )
              != IDE_SUCCESS );

    for ( i = 0; i < aAck->mAbortTxCount; i++ )
    {
        IDE_TEST( sendTxAckA5( &sProtocolContext,
                               aExitFlag,
                               aAck->mAbortTxList[i].mTID,
                               aAck->mAbortTxList[i].mSN,
                               aTimeoutSec )
                  != IDE_SUCCESS );
    }

    for ( i = 0; i < aAck->mClearTxCount; i++ )
    {
        IDE_TEST( sendTxAckA5( &sProtocolContext,
                               aExitFlag,
                               aAck->mClearTxList[i].mTID,
                               aAck->mClearTxList[i].mSN,
                               aTimeoutSec )
                  != IDE_SUCCESS );
    }

    IDE_TEST( flushProtocol( NULL,
                             &sProtocolContext, 
                             aExitFlag,
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Finalize Protocol */
    sStage = 1;
    IDE_TEST( cmiFinalizeProtocol( &sProtocol ) != IDE_SUCCESS );

    sStage = 0;
    IDE_TEST( cmiFinalizeProtocolContext( &sProtocolContext ) != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 2:
            (void)cmiFinalizeProtocol( &sProtocol );
        case 1:
            (void)cmiFinalizeProtocolContext( &sProtocolContext );
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;    
}

IDE_RC rpnComm::recvAckA5( iduMemAllocator    * /*aAllocator*/,
                           cmiProtocolContext * aProtocolContext,
                           idBool             * aExitFlag,
                           rpXLogAck          * aAck,
                           ULong                aTimeoutSec,
                           idBool             * aIsTimeoutSec )
{
    cmiProtocol   sProtocol;
    SInt          sStage = 0;
    cmpArgRPAck * sArgAck = NULL;

    ULong         i;

    *aIsTimeoutSec = ID_FALSE;

    IDE_TEST( readCmProtocol( aProtocolContext,
                              &sProtocol,
                              aExitFlag,
                              aIsTimeoutSec,
                              aTimeoutSec )
              != IDE_SUCCESS );
    sStage = 1;

    if ( *aIsTimeoutSec == ID_FALSE )
    {
        /* Validation Ack Type */
        IDE_TEST_RAISE( sProtocol.mOpID != CMI_PROTOCOL_OPERATION( RP, Ack ),
                        ERR_CHECK_OPERATION_TYPE );

        /* Get argument */
        sArgAck = CMI_PROTOCOL_GET_ARG(sProtocol, RP, Ack);
        aAck->mAckType           = sArgAck->mXLogType;
        aAck->mAbortTxCount      = sArgAck->mAbortTxCount;
        aAck->mClearTxCount      = sArgAck->mClearTxCount;
        aAck->mRestartSN         = sArgAck->mRestartSN;
        aAck->mLastCommitSN      = sArgAck->mLastCommitSN;
        aAck->mLastArrivedSN     = sArgAck->mLastArrivedSN;
        aAck->mLastProcessedSN   = sArgAck->mLastProcessedSN;
        aAck->mFlushSN           = sArgAck->mFlushSN;

        /* Get Abort Tx List */
        for ( i = 0 ; i < aAck->mAbortTxCount ; i++ )
        {
            IDE_TEST( recvTxAckForA5( aExitFlag,
                                      aProtocolContext,
                                      &(aAck->mAbortTxList[i].mTID),
                                      &(aAck->mAbortTxList[i].mSN),
                                      aTimeoutSec )
                      != IDE_SUCCESS );
        }

        /* Get Clear Tx List */
        for ( i = 0 ; i < aAck->mClearTxCount ; i++ )
        {
            IDE_TEST( recvTxAckForA5( aExitFlag,
                                      aProtocolContext,
                                      &(aAck->mClearTxList[i].mTID),
                                      &(aAck->mClearTxList[i].mSN),
                                      aTimeoutSec )
                      != IDE_SUCCESS );
        }
    }
    else
    {
       /* do nothing */
    }

    /* Finalize Protocol */
    sStage = 0;
    IDE_TEST( cmiFinalizeProtocol( &sProtocol ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CHECK_OPERATION_TYPE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RP_WRONG_OPERATION_TYPE,
                                  sProtocol.mOpID ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 1:
            (void)cmiFinalizeProtocol( &sProtocol );
            break;
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendLobCursorOpenA5( void               * /*aHBTResource*/,
                                     cmiProtocolContext * aProtocolContext,
                                     idBool             * aExitFlag,
                                     smTID                aTID,
                                     smSN                 aSN,
                                     smSN                 aSyncSN,
                                     ULong                aTableOID,
                                     ULong                aLobLocator,
                                     UInt                 aLobColumnID,
                                     UInt                 aPKColCnt,
                                     smiValue           * aPKCols,
                                     rpValueLen         * aPKLen,
                                     UInt                 aTimeoutSec )
{
    cmiProtocolContext       sProtocolContext;
    cmiProtocol              sProtocol;
    UInt                     sStage            = 0;
    cmiLink                * sLink             = NULL;
    cmpArgRPLobCursorOpen  * sArgLobCursorOpen = NULL;

    UInt        i;

    /* Validate Parameters */
    IDE_DASSERT( aLobColumnID < QCI_MAX_COLUMN_COUNT );
    IDE_DASSERT( aPKColCnt <= QCI_MAX_KEY_COLUMN_COUNT );
    IDE_DASSERT( aPKCols != NULL );

    IDE_TEST( cmiGetLinkForProtocolContext( aProtocolContext, &sLink )
              != IDE_SUCCESS );

    IDE_TEST( cmiInitializeProtocolContext( &sProtocolContext,
                                            CMI_PROTOCOL_MODULE( RP ),
                                            sLink )
              != IDE_SUCCESS );
    sStage = 1;

    cmiProtocolContextCopyA5Header( &sProtocolContext,
                                    &(aProtocolContext->mReadHeader) );

    /* Initialize Protocol */
    CMI_PROTOCOL_INITIALIZE( sProtocol, sArgLobCursorOpen, RP, LobCursorOpen );
    sStage = 2;

    /* Replication XLog Header Set */
    sArgLobCursorOpen->mXLogType = RP_X_LOB_CURSOR_OPEN;
    sArgLobCursorOpen->mTransID  = aTID;
    sArgLobCursorOpen->mSN       = aSN;
    sArgLobCursorOpen->mSyncSN   = aSyncSN;

    /* Additional Information */
    sArgLobCursorOpen->mTableOID   = aTableOID;
    sArgLobCursorOpen->mLobLocator = aLobLocator;
    sArgLobCursorOpen->mColumnID   = aLobColumnID;
    sArgLobCursorOpen->mPKColCnt   = aPKColCnt;

    /* Send Protocol */
    IDE_TEST( writeProtocol( NULL,
                             &sProtocolContext,
                             &sProtocol )
              != IDE_SUCCESS );

    /* Send Primary Key Value */
    for ( i = 0 ; i < aPKColCnt ; i++ )
    {
        IDE_TEST( sendPKValueForA5( NULL,
                                    &sProtocolContext,
                                    aExitFlag,
                                    &aPKCols[i],
                                    aPKLen[i],
                                    aTimeoutSec )
                  != IDE_SUCCESS );
    }

    /* Finalize Protocol */
    sStage = 1;
    IDE_TEST( cmiFinalizeProtocol( &sProtocol ) != IDE_SUCCESS );

    sStage = 0;
    IDE_TEST( cmiFinalizeProtocolContext( &sProtocolContext ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 2:
            (void)cmiFinalizeProtocol( &sProtocol );
            /* fall through */
        case 1:
            (void)cmiFinalizeProtocolContext( &sProtocolContext );
            break;
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpnComm::recvLobCursorOpenA5( iduMemAllocator     * aAllocator,
                                     idBool              * aExitFlag,
                                     cmiProtocolContext  * aProtocolContext,
                                     cmiProtocol         * aProtocol,
                                     rpdMeta             * aMeta,
                                     rpdXLog             * aXLog,
                                     ULong                 aTimeOutSec )
{
    UInt                     i = 0;
    cmpArgRPLobCursorOpen  * sArg =
        CMI_PROTOCOL_GET_ARG( *aProtocol, RP, LobCursorOpen );
    rpdMetaItem            * sItem = NULL;

    IDE_ASSERT( aMeta != NULL );

    /* Get Argument XLog Hdr */
    aXLog->mType        = (rpXLogType)sArg->mXLogType;
    aXLog->mTID         = sArg->mTransID;
    aXLog->mSN          = sArg->mSN;
    aXLog->mSyncSN      = sArg->mSyncSN;

    /* Get Lob Information */
    aXLog->mTableOID            = sArg->mTableOID;
    aXLog->mLobPtr->mLobLocator = sArg->mLobLocator;
    aXLog->mPKColCnt            = sArg->mPKColCnt;

    IDE_TEST_RAISE( aXLog->mMemory.alloc( aXLog->mPKColCnt * ID_SIZEOF( smiValue ),
                                          (void**)&( aXLog->mPKCols ) )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC );

    /* Recv PK Value repeatedly */
    for ( i = 0; i < aXLog->mPKColCnt; i++ )
    {
        IDE_TEST( recvValueA5( aAllocator,
                               aExitFlag,
                               aProtocolContext,
                               &aXLog->mMemory,
                               &aXLog->mPKCols[i],
                               aTimeOutSec )
                  != IDE_SUCCESS );
    }

    /* PROJ-1442 Replication Online 중 DDL 허용
     * XLog 내의 Column 정보를 재배열한다.
     *   - mPKColCnt는 변하지 않으므로, 제외한다.
     *   - Primary Key의 순서가 같으므로, mPKCIDs[]와 mPKCols[]는 제외한다.
     *
     * LOB    : mLobColumnID
     */
    (void)aMeta->searchRemoteTable( &sItem, aXLog->mTableOID );
    IDE_TEST_RAISE( sItem == NULL, ERR_NOT_FOUND_TABLE );

    aXLog->mLobPtr->mLobColumnID = sItem->mMapColID[sArg->mColumnID];

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_FOUND_TABLE );
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_RECEIVER_NOT_FOUND_TABLE,
                                  aXLog->mSN,
                                  aXLog->mTableOID ) );
    }
    IDE_EXCEPTION( ERR_MEMORY_ALLOC );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET(ideSetErrorCode(rpERR_ABORT_MEMORY_ALLOC,
                                "rpnComm::recvLobCursorOpenA5",
                                "Column"));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendLobCursorCloseA5( void                * /*aHBTResource*/,
                                      cmiProtocolContext  * aProtocolContext,
                                      idBool              * /*aExitFlag*/,
                                      smTID                 aTID,
                                      smSN                  aSN,
                                      smSN                  aSyncSN,
                                      ULong                 aLobLocator,
                                      UInt                  /*aTimeoutSec*/ )
{
    cmiProtocolContext       sProtocolContext;
    cmiProtocol              sProtocol;
    UInt                     sStage             = 0;
    cmiLink                * sLink              = NULL;
    cmpArgRPLobCursorClose * sArgLobCursorClose = NULL;

    IDE_TEST( cmiGetLinkForProtocolContext( aProtocolContext, &sLink )
              != IDE_SUCCESS );

    IDE_TEST( cmiInitializeProtocolContext( &sProtocolContext,
                                            CMI_PROTOCOL_MODULE( RP ),
                                            sLink )
              != IDE_SUCCESS );
    sStage = 1;

    cmiProtocolContextCopyA5Header( &sProtocolContext,
                                    &(aProtocolContext->mReadHeader) );

    /* Initialize Protocol */
    CMI_PROTOCOL_INITIALIZE( sProtocol, sArgLobCursorClose, RP, LobCursorClose );
    sStage = 2;

    /* Replication XLog Header Set */
    sArgLobCursorClose->mXLogType = RP_X_LOB_CURSOR_CLOSE;
    sArgLobCursorClose->mTransID  = aTID;
    sArgLobCursorClose->mSN       = aSN;
    sArgLobCursorClose->mSyncSN   = aSyncSN;

    /* Additional Information */
    sArgLobCursorClose->mLobLocator = aLobLocator;

    /* Send Protocol */
    IDE_TEST( writeProtocol( NULL,
                             &sProtocolContext,
                             &sProtocol )
              != IDE_SUCCESS );

    /* Finalize Protocol */
    sStage = 1;
    IDE_TEST( cmiFinalizeProtocol( &sProtocol ) != IDE_SUCCESS );

    sStage = 0;
    IDE_TEST( cmiFinalizeProtocolContext( &sProtocolContext ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 2:
            (void)cmiFinalizeProtocol( &sProtocol );
            /* fall through */
        case 1:
            (void)cmiFinalizeProtocolContext( &sProtocolContext );
            break;
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
 *
 */  
IDE_RC rpnComm::recvLobCursorCloseA5( cmiProtocol         * aProtocol,
                                      rpdXLog             * aXLog )
{
    cmpArgRPLobCursorClose * sArg =
        CMI_PROTOCOL_GET_ARG( *aProtocol, RP, LobCursorClose );

    /* Get Argument XLog Hdr */
    aXLog->mType    = (rpXLogType)sArg->mXLogType;
    aXLog->mTID     = sArg->mTransID;
    aXLog->mSN      = sArg->mSN;
    aXLog->mSyncSN  = sArg->mSyncSN;

    /* Get Lob Information */
    aXLog->mLobPtr->mLobLocator = sArg->mLobLocator;

    return IDE_SUCCESS;
}

IDE_RC rpnComm::sendLobPrepare4WriteA5( void               * /*aHBTResource*/,
                                        cmiProtocolContext * aProtocolContext,
                                        idBool             * /*aExitFlag*/,
                                        smTID                aTID,
                                        smSN                 aSN,
                                        smSN                 aSyncSN,
                                        ULong                aLobLocator,
                                        UInt                 aLobOffset,
                                        UInt                 aLobOldSize,
                                        UInt                 aLobNewSize,
                                        UInt                 /*aTimeoutSec*/)
{
    cmiProtocolContext         sProtocolContext;
    cmiProtocol                sProtocol;
    UInt                       sStage               = 0;
    cmiLink                  * sLink                = NULL;
    cmpArgRPLobPrepare4Write * sArgLobPrepare4Write = NULL;

    IDE_TEST( cmiGetLinkForProtocolContext( aProtocolContext, &sLink )
              != IDE_SUCCESS );

    IDE_TEST( cmiInitializeProtocolContext( &sProtocolContext,
                                            CMI_PROTOCOL_MODULE( RP ),
                                            sLink )
              != IDE_SUCCESS );
    sStage = 1;

    cmiProtocolContextCopyA5Header( &sProtocolContext,
                                    &(aProtocolContext->mReadHeader) );

    /* Initialize Protocol */
    CMI_PROTOCOL_INITIALIZE( sProtocol, sArgLobPrepare4Write, RP, LobPrepare4Write );
    sStage = 2;

    /* Replication XLog Header Set */
    sArgLobPrepare4Write->mXLogType = RP_X_LOB_PREPARE4WRITE;
    sArgLobPrepare4Write->mTransID  = aTID;
    sArgLobPrepare4Write->mSN       = aSN;
    sArgLobPrepare4Write->mSyncSN   = aSyncSN;

    /* Additional Information */
    sArgLobPrepare4Write->mLobLocator = aLobLocator;
    sArgLobPrepare4Write->mOffset     = aLobOffset;
    sArgLobPrepare4Write->mOldSize    = aLobOldSize;
    sArgLobPrepare4Write->mNewSize    = aLobNewSize;

    /* Send Protocol */
    IDE_TEST( writeProtocol( NULL,
                             &sProtocolContext,
                             &sProtocol )
              != IDE_SUCCESS );

    /* Finalize Protocol */
    sStage = 1;
    IDE_TEST( cmiFinalizeProtocol( &sProtocol ) != IDE_SUCCESS );

    sStage = 0;
    IDE_TEST( cmiFinalizeProtocolContext( &sProtocolContext ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 2:
            (void)cmiFinalizeProtocol( &sProtocol );
            /* fall through */
        case 1:
            (void)cmiFinalizeProtocolContext( &sProtocolContext );
            break;
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpnComm::recvLobPrepare4WriteA5( cmiProtocol        * aProtocol,
                                        rpdXLog            * aXLog )
{
    cmpArgRPLobPrepare4Write * sArg =
        CMI_PROTOCOL_GET_ARG( *aProtocol, RP, LobPrepare4Write );
    rpdLob * sLobPtr = aXLog->mLobPtr;

    /* Get Argument XLog Hdr */
    aXLog->mType       = (rpXLogType)sArg->mXLogType;
    aXLog->mTID        = sArg->mTransID;
    aXLog->mSN         = sArg->mSN;
    aXLog->mSyncSN     = sArg->mSyncSN;

    /* Get Lob Information */
    sLobPtr->mLobLocator = sArg->mLobLocator;
    sLobPtr->mLobOffset  = sArg->mOffset;
    sLobPtr->mLobOldSize = sArg->mOldSize;
    sLobPtr->mLobNewSize = sArg->mNewSize;

    return IDE_SUCCESS;    
}

IDE_RC rpnComm::sendLobTrimA5( void               * /*aHBTResource*/,
                               cmiProtocolContext * /*aProtocolContext*/,
                               idBool             * /*aExitFlag*/,
                               smTID                /*aTID*/,
                               smSN                 /*aSN*/,
                               smSN                 /*aSyncSN*/,
                               ULong                /*aLobLocator*/,
                               UInt                 /*aLobOffset*/,
                               UInt                 /*aTimeoutSec*/ )
{
    IDE_ASSERT( 0 );
    return IDE_FAILURE;
}

IDE_RC rpnComm::sendLobPartialWriteA5( void               * /*aHBTResource*/,
                                       cmiProtocolContext * aProtocolContext,
                                       idBool             * /*aExitFlag*/,
                                       smTID                aTID,
                                       smSN                 aSN,
                                       smSN                 aSyncSN,
                                       ULong                aLobLocator,
                                       UInt                 aLobOffset,
                                       UInt                 aLobPieceLen,
                                       SChar              * aLobPiece,
                                       UInt                 /*aTimeoutSec*/ )
{
    cmiProtocolContext         sProtocolContext;
    cmiProtocol                sProtocol;
    UInt                       sStage              = 0;
    cmiLink                  * sLink               = NULL;
    cmpArgRPLobPartialWrite  * sArgLobPartialWrite = NULL;

    IDE_TEST( cmiGetLinkForProtocolContext( aProtocolContext, &sLink )
              != IDE_SUCCESS );

    IDE_TEST( cmiInitializeProtocolContext( &sProtocolContext,
                                            CMI_PROTOCOL_MODULE( RP ),
                                            sLink )
              != IDE_SUCCESS );
    sStage = 1;

    cmiProtocolContextCopyA5Header( &sProtocolContext,
                                    &(aProtocolContext->mReadHeader) );

    /* Initialize Protocol */
    CMI_PROTOCOL_INITIALIZE( sProtocol, sArgLobPartialWrite, RP, LobPartialWrite );
    sStage = 2;

    /* Replication XLog Header Set */
    sArgLobPartialWrite->mXLogType = RP_X_LOB_PARTIAL_WRITE;
    sArgLobPartialWrite->mTransID  = aTID;
    sArgLobPartialWrite->mSN       = aSN;
    sArgLobPartialWrite->mSyncSN   = aSyncSN;

    /* Additional Information */
    sArgLobPartialWrite->mLobLocator = aLobLocator;
    sArgLobPartialWrite->mOffset     = aLobOffset;
    sArgLobPartialWrite->mPieceLen   = aLobPieceLen;
    IDE_TEST( cmtVariableSetData( &sArgLobPartialWrite->mPieceValue,
                                  (UChar *)aLobPiece,
                                  aLobPieceLen )
              != IDE_SUCCESS );

    /* Send Protocol */
    IDE_TEST( writeProtocol( NULL,
                             &sProtocolContext,
                             &sProtocol )
              != IDE_SUCCESS );

    /* Finalize Protocol */
    sStage = 1;
    IDE_TEST( cmiFinalizeProtocol( &sProtocol ) != IDE_SUCCESS );

    sStage = 0;
    IDE_TEST( cmiFinalizeProtocolContext( &sProtocolContext ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 2:
            (void)cmiFinalizeProtocol( &sProtocol );
            /* fall through */
        case 1:
            (void)cmiFinalizeProtocolContext( &sProtocolContext );
            break;
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpnComm::recvLobPartialWriteA5( iduMemAllocator    * aAllocator,
                                       cmiProtocol        * aProtocol,
                                       rpdXLog            * aXLog )
{
    cmpArgRPLobPartialWrite * sArg =
        CMI_PROTOCOL_GET_ARG( *aProtocol, RP, LobPartialWrite );
    rpdLob * sLobPtr = aXLog->mLobPtr;

    /* Get Argument XLog Hdr */
    aXLog->mType        = (rpXLogType)sArg->mXLogType;
    aXLog->mTID         = sArg->mTransID;
    aXLog->mSN          = sArg->mSN;
    aXLog->mSyncSN      = sArg->mSyncSN;

    /* Get Lob Information */
    sLobPtr->mLobLocator  = sArg->mLobLocator;
    sLobPtr->mLobOffset   = sArg->mOffset;
    sLobPtr->mLobPieceLen = sArg->mPieceLen;
    sLobPtr->mLobPiece    = NULL;

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_RP_RPN,
                                       sLobPtr->mLobPieceLen,
                                       (void **)&(sLobPtr->mLobPiece),
                                       IDU_MEM_IMMEDIATE,
                                       aAllocator )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC_LOB_PIECE );
    
    IDE_TEST( cmtVariableGetData( &sArg->mPieceValue,
                                  (UChar *)sLobPtr->mLobPiece,
                                  sLobPtr->mLobPieceLen )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC_LOB_PIECE );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "rpnComm::recvLobPartialWriteA5",
                                  "aXLog->mLobPtr->mLobPiece" ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    
    if ( sLobPtr->mLobPiece != NULL )
    {
        (void)iduMemMgr::free( sLobPtr->mLobPiece, aAllocator );
        sLobPtr->mLobPiece = NULL;
    }
    
    IDE_POP();

    return IDE_FAILURE;    
}

IDE_RC rpnComm::sendLobFinish2WriteA5( void               * /*aHBTResource*/,
                                       cmiProtocolContext * aProtocolContext,
                                       idBool             * /* aExitFlag */,
                                       smTID                aTID,
                                       smSN                 aSN,
                                       smSN                 aSyncSN,
                                       ULong                aLobLocator,
                                       UInt                 /*aTimeoutSec*/ )
{
    cmiProtocolContext         sProtocolContext;
    cmiProtocol                sProtocol;
    UInt                       sStage              = 0;
    cmiLink                  * sLink               = NULL;
    cmpArgRPLobFinish2Write  * sArgLobFinish2Write = NULL;

    IDE_TEST( cmiGetLinkForProtocolContext( aProtocolContext, &sLink )
              != IDE_SUCCESS );

    IDE_TEST( cmiInitializeProtocolContext( &sProtocolContext,
                                            CMI_PROTOCOL_MODULE( RP ),
                                            sLink )
              != IDE_SUCCESS );
    sStage = 1;

    cmiProtocolContextCopyA5Header( &sProtocolContext,
                                    &(aProtocolContext->mReadHeader) );

    /* Initialize Protocol */
    CMI_PROTOCOL_INITIALIZE( sProtocol, sArgLobFinish2Write, RP, LobFinish2Write );
    sStage = 2;

    /* Replication XLog Header Set */
    sArgLobFinish2Write->mXLogType = RP_X_LOB_FINISH2WRITE;
    sArgLobFinish2Write->mTransID  = aTID;
    sArgLobFinish2Write->mSN       = aSN;
    sArgLobFinish2Write->mSyncSN   = aSyncSN;

    /* Additional Information */
    sArgLobFinish2Write->mLobLocator = aLobLocator;

    /* Send Protocol */
    IDE_TEST( writeProtocol( NULL,
                             &sProtocolContext,
                             &sProtocol )
              != IDE_SUCCESS );

    /* Finalize Protocol */
    sStage = 1;
    IDE_TEST( cmiFinalizeProtocol( &sProtocol ) != IDE_SUCCESS );

    sStage = 0;
    IDE_TEST( cmiFinalizeProtocolContext( &sProtocolContext ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 2:
            (void)cmiFinalizeProtocol( &sProtocol );
            /* fall through */
        case 1:
            (void)cmiFinalizeProtocolContext( &sProtocolContext );
            break;
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpnComm::recvLobFinish2WriteA5( cmiProtocol        * aProtocol,
                                       rpdXLog            * aXLog )
{
    cmpArgRPLobFinish2Write * sArg =
        CMI_PROTOCOL_GET_ARG( *aProtocol, RP, LobFinish2Write );

    /* Get Argument XLog Hdr */
    aXLog->mType    = (rpXLogType)sArg->mXLogType;
    aXLog->mTID     = sArg->mTransID;
    aXLog->mSN      = sArg->mSN;
    aXLog->mSyncSN  = sArg->mSyncSN;

    /* Get Lob Information */
    aXLog->mLobPtr->mLobLocator = sArg->mLobLocator;

    return IDE_SUCCESS;    
}

IDE_RC rpnComm::sendHandshakeA5( void               * /*aHBTResource*/,
                                 cmiProtocolContext * aProtocolContext,
                                 idBool             * /* aExitFlag */,
                                 smTID                aTID,
                                 smSN                 aSN,
                                 smSN                 aSyncSN,
                                 UInt                 /*aTimeoutSec*/ )
{
    cmiProtocolContext         sProtocolContext;
    cmiProtocol                sProtocol;
    UInt                       sStage         = 0;
    cmiLink                  * sLink          = NULL;
    cmpArgRPHandshake        * sArgHandshake  = NULL;

    IDE_TEST( cmiGetLinkForProtocolContext( aProtocolContext, &sLink )
              != IDE_SUCCESS );

    IDE_TEST( cmiInitializeProtocolContext( &sProtocolContext,
                                            CMI_PROTOCOL_MODULE( RP ),
                                            sLink )
              != IDE_SUCCESS );
    sStage = 1;

    cmiProtocolContextCopyA5Header( &sProtocolContext,
                                    &(aProtocolContext->mReadHeader) );

    /* Initialize Protocol */
    CMI_PROTOCOL_INITIALIZE( sProtocol, sArgHandshake, RP, Handshake );
    sStage = 2;

    /* Replication XLog Header Set */
    sArgHandshake->mXLogType = RP_X_HANDSHAKE;
    sArgHandshake->mTransID  = aTID;
    sArgHandshake->mSN       = aSN;
    sArgHandshake->mSyncSN   = aSyncSN;

    /* Send Protocol */
    IDE_TEST( writeProtocol( NULL,
                             &sProtocolContext,
                             &sProtocol )
              != IDE_SUCCESS );

    /* Finalize Protocol */
    sStage = 1;
    IDE_TEST( cmiFinalizeProtocol( &sProtocol ) != IDE_SUCCESS );

    sStage = 0;
    IDE_TEST( cmiFinalizeProtocolContext( &sProtocolContext ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 2:
            (void)cmiFinalizeProtocol( &sProtocol );
            /* fall through */
        case 1:
            (void)cmiFinalizeProtocolContext( &sProtocolContext );
            break;
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpnComm::recvHandshakeA5( cmiProtocol        * aProtocol,
                                 rpdXLog            * aXLog )
{
    cmpArgRPHandshake * sArg =
        CMI_PROTOCOL_GET_ARG( *aProtocol, RP, Handshake );

    /* Get Argument XLog Hdr */
    aXLog->mType   = (rpXLogType)sArg->mXLogType;
    aXLog->mTID    = sArg->mTransID;
    aXLog->mSN     = sArg->mSN;
    aXLog->mSyncSN = sArg->mSyncSN;

    return IDE_SUCCESS;
}

IDE_RC rpnComm::sendSyncPKBeginA5( void               * /*aHBTResource*/,
                                   cmiProtocolContext * aProtocolContext,
                                   idBool             * aExitFlag,
                                   smTID                aTID,
                                   smSN                 aSN,
                                   smSN                 aSyncSN,
                                   UInt                 aTimeoutSec )
{
    cmiProtocolContext         sProtocolContext;
    cmiProtocol                sProtocol;
    UInt                       sStage          = 0;
    cmiLink                  * sLink           = NULL;
    cmpArgRPSyncPKBegin      * sArgSyncPKBegin = NULL;

    IDE_TEST( cmiGetLinkForProtocolContext( aProtocolContext, &sLink )
              != IDE_SUCCESS );

    IDE_TEST( cmiInitializeProtocolContext( &sProtocolContext,
                                            CMI_PROTOCOL_MODULE( RP ),
                                            sLink )
              != IDE_SUCCESS );
    sStage = 1;

    cmiProtocolContextCopyA5Header( &sProtocolContext,
                                    &(aProtocolContext->mReadHeader) );

    /* Initialize Protocol */
    CMI_PROTOCOL_INITIALIZE( sProtocol, sArgSyncPKBegin, RP, SyncPKBegin );
    sStage = 2;

    /* Replication XLog Header Set */
    sArgSyncPKBegin->mXLogType = RP_X_SYNC_PK_BEGIN;
    sArgSyncPKBegin->mTransID  = aTID;
    sArgSyncPKBegin->mSN       = aSN;
    sArgSyncPKBegin->mSyncSN   = aSyncSN;

    /* Send Protocol */
    IDE_TEST( writeProtocol( NULL,
                             &sProtocolContext,
                             &sProtocol )
              != IDE_SUCCESS );

    IDE_TEST( flushProtocol( NULL,
                             &sProtocolContext,
                             aExitFlag,
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Finalize Protocol */
    sStage = 1;
    IDE_TEST( cmiFinalizeProtocol( &sProtocol ) != IDE_SUCCESS );

    sStage = 0;
    IDE_TEST( cmiFinalizeProtocolContext( &sProtocolContext ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 2:
            (void)cmiFinalizeProtocol( &sProtocol );
            /* fall through */
        case 1:
            (void)cmiFinalizeProtocolContext( &sProtocolContext );
            break;
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpnComm::recvSyncPKBeginA5( cmiProtocol        * aProtocol,
                                   rpdXLog            * aXLog )
{
    cmpArgRPSyncPKBegin * sArg =
        CMI_PROTOCOL_GET_ARG( *aProtocol, RP, SyncPKBegin );

    /* Get Argument XLog Hdr */
    aXLog->mType   = (rpXLogType)sArg->mXLogType;
    aXLog->mTID    = sArg->mTransID;
    aXLog->mSN     = sArg->mSN;
    aXLog->mSyncSN = sArg->mSyncSN;

    return IDE_SUCCESS;
}

IDE_RC rpnComm::sendSyncPKA5( void               * /*aHBTResource*/,
                              cmiProtocolContext * aProtocolContext,
                              idBool             * aExitFlag,
                              smTID                aTID,
                              smSN                 aSN,
                              smSN                 aSyncSN,
                              ULong                aTableOID,
                              UInt                 aPKColCnt,
                              smiValue           * aPKCols,
                              rpValueLen         * aPKLen,
                              UInt                 aTimeoutSec )
{
    cmiProtocolContext         sProtocolContext;
    cmiProtocol                sProtocol;
    UInt                       sStage          = 0;
    cmiLink                  * sLink           = NULL;
    cmpArgRPSyncPK           * sArgSyncPK      = NULL;

    UInt        i;

    IDE_TEST( cmiGetLinkForProtocolContext( aProtocolContext, &sLink )
              != IDE_SUCCESS );

    IDE_TEST( cmiInitializeProtocolContext( &sProtocolContext,
                                            CMI_PROTOCOL_MODULE( RP ),
                                            sLink )
              != IDE_SUCCESS );
    sStage = 1;

    cmiProtocolContextCopyA5Header( &sProtocolContext,
                                    &(aProtocolContext->mReadHeader) );

    /* Initialize Protocol */
    CMI_PROTOCOL_INITIALIZE( sProtocol, sArgSyncPK, RP, SyncPK );
    sStage = 2;

    /* Replication XLog Header Set */
    sArgSyncPK->mXLogType = RP_X_SYNC_PK;
    sArgSyncPK->mTransID  = aTID;
    sArgSyncPK->mSN       = aSN;
    sArgSyncPK->mSyncSN   = aSyncSN;

    /* Additional Information */
    sArgSyncPK->mTableOID = aTableOID;
    sArgSyncPK->mPKColCnt = aPKColCnt;

    /* Send Protocol */
    IDE_TEST( writeProtocol( NULL,
                             &sProtocolContext,
                             &sProtocol )
              != IDE_SUCCESS );

    /* Send Primary Key Value */
    for ( i = 0 ; i < aPKColCnt ; i++ )
    {
        IDE_TEST( sendPKValueForA5( NULL,
                                    &sProtocolContext,
                                    aExitFlag,
                                    &aPKCols[i],
                                    aPKLen[i],
                                    aTimeoutSec )
                  != IDE_SUCCESS);
    }

    /* Finalize Protocol */
    sStage = 1;
    IDE_TEST( cmiFinalizeProtocol( &sProtocol ) != IDE_SUCCESS );

    sStage = 0;
    IDE_TEST( cmiFinalizeProtocolContext( &sProtocolContext ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 2:
            (void)cmiFinalizeProtocol( &sProtocol );
            /* fall through */
        case 1:
            (void)cmiFinalizeProtocolContext( &sProtocolContext );
            break;
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpnComm::recvSyncPKA5( idBool             * aExitFlag,
                              cmiProtocolContext * aProtocolContext,
                              cmiProtocol        * aProtocol,
                              rpdXLog            * aXLog,
                              ULong                aTimeOutSec )
{
    UInt             i = 0;
    cmpArgRPSyncPK * sArg = CMI_PROTOCOL_GET_ARG( *aProtocol, RP, SyncPK );

    /* Get Argument XLog Hdr */
    aXLog->mType   = (rpXLogType)sArg->mXLogType;
    aXLog->mTID    = sArg->mTransID;
    aXLog->mSN     = sArg->mSN;
    aXLog->mSyncSN = sArg->mSyncSN;

    /* Get Argument XLog Body */
    aXLog->mTableOID = sArg->mTableOID;
    aXLog->mPKColCnt = sArg->mPKColCnt;

    IDE_TEST_RAISE( aXLog->mMemory.alloc( aXLog->mPKColCnt * ID_SIZEOF( smiValue ),
                                          (void**)&( aXLog->mPKCols ) )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC );

    /* Recv PK Value repeatedly */
    for ( i = 0; i < aXLog->mPKColCnt; i++ )
    {
        IDE_TEST( recvValueA5( NULL, /* Sender에 전달하기에 직접 할당 */
                               aExitFlag,
                               aProtocolContext,
                               NULL, /* aMemory: Sender에 전달할 것이므로, 직접할당 */
                               &aXLog->mPKCols[i],
                               aTimeOutSec )
                  != IDE_SUCCESS );
    }

    /* PROJ-1442 Replication Online 중 DDL 허용
     * XLog 내의 Column 정보를 재배열한다.
     *   - mPKColCnt는 변하지 않으므로, 제외한다.
     *   - Primary Key의 순서가 같으므로, mPKCIDs[]와 mPKCols[]는 제외한다.
     */

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET(ideSetErrorCode(rpERR_ABORT_MEMORY_ALLOC,
                                "rpnComm::recvSyncPKA5",
                                "Column"));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpnComm::sendSyncPKEndA5( void               * /*aHBTResource*/,
                                 cmiProtocolContext * aProtocolContext,
                                 idBool             * aExitFlag,
                                 smTID                aTID,
                                 smSN                 aSN,
                                 smSN                 aSyncSN,
                                 UInt                 aTimeoutSec )
{
    cmiProtocolContext         sProtocolContext;
    cmiProtocol                sProtocol;
    UInt                       sStage          = 0;
    cmiLink                  * sLink           = NULL;
    cmpArgRPSyncPKEnd        * sArgSyncPKEnd   = NULL;

    IDE_TEST( cmiGetLinkForProtocolContext( aProtocolContext, &sLink )
              != IDE_SUCCESS );

    IDE_TEST( cmiInitializeProtocolContext( &sProtocolContext,
                                            CMI_PROTOCOL_MODULE( RP ),
                                            sLink )
              != IDE_SUCCESS );
    sStage = 1;

    cmiProtocolContextCopyA5Header( &sProtocolContext,
                                    &(aProtocolContext->mReadHeader) );

    /* Initialize Protocol */
    CMI_PROTOCOL_INITIALIZE( sProtocol, sArgSyncPKEnd, RP, SyncPKEnd );
    sStage = 2;

    /* Replication XLog Header Set */
    sArgSyncPKEnd->mXLogType = RP_X_SYNC_PK_END;
    sArgSyncPKEnd->mTransID  = aTID;
    sArgSyncPKEnd->mSN       = aSN;
    sArgSyncPKEnd->mSyncSN   = aSyncSN;

    /* Send Protocol */
    IDE_TEST( writeProtocol( NULL,
                             &sProtocolContext,
                             &sProtocol )
              != IDE_SUCCESS );

    IDE_TEST( flushProtocol( NULL,
                             &sProtocolContext,
                             aExitFlag,
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Finalize Protocol */
    sStage = 1;
    IDE_TEST( cmiFinalizeProtocol( &sProtocol ) != IDE_SUCCESS );

    sStage = 0;
    IDE_TEST( cmiFinalizeProtocolContext( &sProtocolContext ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 2:
            (void)cmiFinalizeProtocol( &sProtocol );
            /* fall through */
        case 1:
            (void)cmiFinalizeProtocolContext( &sProtocolContext );
            break;
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpnComm::recvSyncPKEndA5( cmiProtocol        * aProtocol,
                                 rpdXLog            * aXLog )
{
    cmpArgRPSyncPKEnd * sArg =
        CMI_PROTOCOL_GET_ARG( *aProtocol, RP, SyncPKEnd );

    /* Get Argument XLog Hdr */
    aXLog->mType   = (rpXLogType)sArg->mXLogType;
    aXLog->mTID    = sArg->mTransID;
    aXLog->mSN     = sArg->mSN;
    aXLog->mSyncSN = sArg->mSyncSN;

    return IDE_SUCCESS;    
}

IDE_RC rpnComm::sendFailbackEndA5( void               * /*aHBTResource*/,
                                   cmiProtocolContext * aProtocolContext,
                                   idBool             * aExitFlag,
                                   smTID                aTID,
                                   smSN                 aSN,
                                   smSN                 aSyncSN,
                                   UInt                 aTimeoutSec )
{
    cmiProtocolContext         sProtocolContext;
    cmiProtocol                sProtocol;
    UInt                       sStage          = 0;
    cmiLink                  * sLink           = NULL;
    cmpArgRPFailbackEnd      * sArgFailbackEnd = NULL;

    IDE_TEST( cmiGetLinkForProtocolContext( aProtocolContext, &sLink )
              != IDE_SUCCESS );

    IDE_TEST( cmiInitializeProtocolContext( &sProtocolContext,
                                            CMI_PROTOCOL_MODULE( RP ),
                                            sLink )
              != IDE_SUCCESS );
    sStage = 1;

    cmiProtocolContextCopyA5Header( &sProtocolContext,
                                    &(aProtocolContext->mReadHeader) );

    /* Initialize Protocol */
    CMI_PROTOCOL_INITIALIZE( sProtocol, sArgFailbackEnd, RP, FailbackEnd );
    sStage = 2;

    /* Replication XLog Header Set */
    sArgFailbackEnd->mXLogType = RP_X_FAILBACK_END;
    sArgFailbackEnd->mTransID  = aTID;
    sArgFailbackEnd->mSN       = aSN;
    sArgFailbackEnd->mSyncSN   = aSyncSN;

    /* Send Protocol */
    IDE_TEST( writeProtocol( NULL,
                             &sProtocolContext,
                             &sProtocol )
              != IDE_SUCCESS );

    IDE_TEST( flushProtocol( NULL,
                             &sProtocolContext,
                             aExitFlag,
                             ID_TRUE,
                             aTimeoutSec )
              != IDE_SUCCESS );

    /* Finalize Protocol */
    sStage = 1;
    IDE_TEST( cmiFinalizeProtocol( &sProtocol ) != IDE_SUCCESS );

    sStage = 0;
    IDE_TEST( cmiFinalizeProtocolContext( &sProtocolContext ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 2:
            (void)cmiFinalizeProtocol( &sProtocol );
            /* fall through */
        case 1:
            (void)cmiFinalizeProtocolContext( &sProtocolContext );
            break;
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

/*
 *
 */
IDE_RC rpnComm::recvFailbackEndA5( cmiProtocol        * aProtocol,
                                   rpdXLog            * aXLog )
{
    cmpArgRPFailbackEnd * sArg =
        CMI_PROTOCOL_GET_ARG( *aProtocol, RP, FailbackEnd );

    /* Get Argument XLog Hdr */
    aXLog->mType   = (rpXLogType)sArg->mXLogType;
    aXLog->mTID    = sArg->mTransID;
    aXLog->mSN     = sArg->mSN;
    aXLog->mSyncSN = sArg->mSyncSN;

    return IDE_SUCCESS;
}

