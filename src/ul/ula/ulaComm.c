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
 

/***********************************************************************
 * $Id: ulaComm.c 91310 2021-07-22 05:10:54Z donghyun1 $
 **********************************************************************/

#include <acp.h>
#include <acl.h>
#include <ace.h>

#include <aciTypes.h>

#include <cmAllClient.h>
#include <cmpDefClient.h>
#include <cmpModuleClient.h>

#include <ulaXLogCollector.h>
#include <ulaComm.h>

#include <ulaConvFmMT.h>
#include <ulaConvNumeric.h>

static acp_time_t gTV1Sec;

void ulaCommInitialize(void)
{
    gTV1Sec = ACP_UINT64_LITERAL(1000000);  /* 1 second */
}

void ulaCommDestroy(void)
{
}

ACI_RC ulaCommSendVersion( cmiProtocolContext * aProtocolContext,
                           ulaErrorMgr        * aOutErrorMgr ) 
{
    acp_char_t sOpCode;
    ulaVersion sVersion;

    ACI_TEST_RAISE( cmiCheckAndFlush( aProtocolContext, 
                                      1 + 8, 
                                      ACP_TRUE )
                    != ACI_SUCCESS, ERR_SEND );

    sOpCode =  CMI_PROTOCOL_OPERATION( RP, Version );
    sVersion.mVersion = RP_MAKE_VERSION( REPLICATION_MAJOR_VERSION, 
                                         REPLICATION_MINOR_VERSION, 
                                         REPLICATION_FIX_VERSION, 
                                         REPLICATION_ENDIAN_64BIT );

    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR8( aProtocolContext, &( sVersion.mVersion ) );

    ACI_TEST_RAISE( cmiSend( aProtocolContext, ACP_TRUE )
                    != ACI_SUCCESS, ERR_SEND );

    return ACI_SUCCESS;
    
    ACI_EXCEPTION( ERR_SEND )
    {
        ulaSetErrorCode( aOutErrorMgr, ulaERR_ABORT_NET_FLUSH,
                         "SendVersion", aciGetErrorCode() );
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulaCommRecvVersion( cmiProtocolContext * aProtocolContext,
                           ulaVersion         * aOutReplVersion,
                           acp_bool_t         * aExitFlag,
                           acp_uint32_t         aTimeoutSec,
                           ulaErrorMgr        * aOutErrorMgr )
{
    acp_char_t sOpCode;

    ACI_TEST( ulaCommReadCmBlock( aProtocolContext,
                                  aExitFlag,
                                  NULL /* TimeoutFlag */,
                                  aTimeoutSec ,
                                  aOutErrorMgr )
              != ACI_SUCCESS );

    /* Check Operation Type */
    CMI_RD1( aProtocolContext, sOpCode );
    ACI_TEST_RAISE( sOpCode != CMI_PROTOCOL_OPERATION( RP, Version ),
                    ERR_CHECK_OPERATION_TYPE );

    /* Version Number Set */
    CMI_RD8( aProtocolContext, &(aOutReplVersion->mVersion) );

    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_CHECK_OPERATION_TYPE );
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_ABORT_NET_UNEXPECTED_PROTOCOL,
                        "Protocol Version");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulaCommSendMetaRepl(  cmiProtocolContext * aProtocolContext,
                             acp_char_t         * aRepName,
                             acp_uint32_t         aFlag,
                             acp_uint64_t         sRemoteVersion,
                             ulaErrorMgr        * aOutErrorMgr)
{
    acp_char_t   sOpCode;
    acp_uint32_t sRepNameLen = 0;
    acp_uint32_t sDummyLen = 1;
    acp_char_t   sDummyChar[32] = { 0, };
    acp_uint32_t sDummy = 0;

    sRepNameLen = (acp_uint32_t)acpCStrLen( aRepName, ULA_NAME_LEN );

    sOpCode = CMI_PROTOCOL_OPERATION( RP, MetaRepl );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, &sRepNameLen );
    CMI_WCP( aProtocolContext, (acp_char_t *)aRepName, sRepNameLen );

    CMI_WCP( aProtocolContext, (acp_char_t*)sDummyChar, 16 ); 
    /*
       CMI_WR4( aProtocolContext, (UInt*)&(aRepl->mItemCount) );
       CMI_WR4( aProtocolContext, (UInt*)&(aRepl->mRole) );
       CMI_WR4( aProtocolContext, (UInt*)&(aRepl->mConflictResolution) );
       CMI_WR4( aProtocolContext, &(aRepl->mTransTblSize) );
    */
   
    CMI_WR4( aProtocolContext, &( aFlag ) );

    CMI_WCP( aProtocolContext, (acp_char_t*)sDummyChar, 24 ); 
    /*
       CMI_WR4( aProtocolContext, (UInt*)&(aRepl->mOptions) );
       CMI_WR8( aProtocolContext, &(aRepl->mRPRecoverySN) );
       CMI_WR4( aProtocolContext, &(aRepl->mReplMode) );
       CMI_WR4( aProtocolContext, &(aRepl->mParallelID) );
       CMI_WR4( aProtocolContext, (UInt*)&(aRepl->mIsStarted) );
    */

    CMI_WR4( aProtocolContext, &( sDummyLen ) );
    CMI_WCP( aProtocolContext, (acp_char_t *)sDummyChar, sDummyLen );
    /* CMI_WCP( aProtocolContext, (UChar *)aRepl->mOSInfo, sOSInfoLen ); */
    
    CMI_WCP( aProtocolContext, (acp_char_t*)sDummyChar, 28 ); 
    /*
       CMI_WR4( aProtocolContext, &(aRepl->mCompileBit) );
       CMI_WR4( aProtocolContext, &(aRepl->mSmVersionID) );
       CMI_WR4( aProtocolContext, &(aRepl->mLFGCount) );
       CMI_WR8( aProtocolContext, &(aRepl->mLogFileSize) );
       CMI_WR8( aProtocolContext, &(aRepl->mOffRpRestartSN) );
    */

    CMI_WR4( aProtocolContext, &( sDummyLen ) );
    CMI_WCP( aProtocolContext, (acp_char_t *)sDummyChar, sDummyLen );
    /* CMI_WCP( aProtocolContext, (UChar *)aRepl->mDBCharSet, sCharSetLen ); */
    
    CMI_WR4( aProtocolContext, &( sDummyLen ) );
    CMI_WCP( aProtocolContext, (acp_char_t *)sDummyChar, sDummyLen );
    /* CMI_WCP( aProtocolContext, (UChar *)aRepl->mNationalCharSet, sNationalCharSetLen ); */
    
    CMI_WR4( aProtocolContext, &( sDummyLen ) );
    CMI_WCP( aProtocolContext, (acp_char_t *)sDummyChar, sDummyLen );
    /* CMI_WCP( aProtocolContext, (UChar *)aRepl->mServerID, sServerIDLen ); */
    
    CMI_WR4( aProtocolContext, &( sDummyLen ) );
    CMI_WCP( aProtocolContext, (acp_char_t *)sDummyChar, sDummyLen );
    /* CMI_WCP( aProtocolContext, (UChar *)aRepl->mRemoteFaultDetectTime, sRemoteFaultDetectTimeLen ); */

    if ( needToProcessProtocolOperation( ULA_META_COMPRESSTYPE,
                                         sRemoteVersion )
         == ACP_TRUE )
    {
        CMI_WR4( aProtocolContext, &( sDummy ) );
    }
    
     ACI_TEST_RAISE( cmiSend( aProtocolContext, ACP_TRUE )
                     != ACI_SUCCESS, ERR_SEND );

     return ACI_SUCCESS;

     ACI_EXCEPTION( ERR_SEND )
     {
         ulaSetErrorCode( aOutErrorMgr, ulaERR_ABORT_NET_FLUSH,
                          "sendMeta", aciGetErrorCode() );
     }

     ACI_EXCEPTION_END;

     return ACI_FAILURE;
}

ACI_RC ulaCommRecvMetaRepl( cmiProtocolContext * aProtocolContext,
                            acp_bool_t         * aExitFlag,
                            ulaReplication     * aOutRepl,
                            ulaReplTempInfo    * aOutReplTempInfo,
                            acp_uint32_t         aTimeoutSec,
                            ulaErrorMgr        * aOutErrorMgr )
{
    acp_char_t   sOpCode;
    acp_uint32_t sRepNameLen;
    acp_uint32_t sCharsetLen;
    acp_uint32_t sNCharsetLen;
    acp_uint32_t sTempLen;

    ACI_TEST( ulaCommReadCmBlock( aProtocolContext,
                                  aExitFlag,
                                  NULL /* TimeoutFlag */,
                                  aTimeoutSec,
                                  aOutErrorMgr )
              != ACI_SUCCESS );

    /* Check Operation Type */
    CMI_RD1( aProtocolContext, sOpCode );

    ACI_TEST_RAISE( sOpCode != CMI_PROTOCOL_OPERATION( RP, MetaRepl ),
                    ERR_CHECK_OPERATION_TYPE );

    /* Get Replication Information */
    CMI_RD4( aProtocolContext, &(sRepNameLen) );
    CMI_RCP( aProtocolContext, (acp_uint8_t *)aOutRepl->mXLogSenderName, sRepNameLen );
    aOutRepl->mXLogSenderName[sRepNameLen] = '\0';
    CMI_RD4( aProtocolContext, &(aOutRepl->mTableCount) );
    CMI_RD4( aProtocolContext, (acp_uint32_t*)&(aOutReplTempInfo->mRole) );

    CMI_SKIP_READ_BLOCK( aProtocolContext, 4 );
    /* CMI_RD4( aProtocolContext, &(aRepl->mConflictResolution) ); */
    CMI_RD4( aProtocolContext, &(aOutReplTempInfo->mTransTableSize) );
    CMI_RD4( aProtocolContext, &(aOutReplTempInfo->mFlags) );

    CMI_SKIP_READ_BLOCK( aProtocolContext, 24 );
    /* CMI_RD4( aProtocolContext, &(aRepl->mOptions) );
       CMI_RD8( aProtocolContext, &(aRepl->mRPRecoverySN) );
       CMI_RD4( aProtocolContext, &(aRepl->mReplMode) );
       CMI_RD4( aProtocolContext, &(aRepl->mParallelID) );
       CMI_RD4( aProtocolContext, &(aRepl->mIsStarted) ); */
    CMI_RD4( aProtocolContext, &(sTempLen) );
    CMI_SKIP_READ_BLOCK( aProtocolContext, ( sTempLen + 28 ) );
    /* CMI_RCP( aProtocolContext, (UChar *)aRepl->mOSInfo, sOSInfoLen );
       CMI_RD4( aProtocolContext, &(aRepl->mCompileBit) );
       CMI_RD4( aProtocolContext, &(aRepl->mSmVersionID) );
       CMI_RD4( aProtocolContext, &(aRepl->mLFGCount) );
       CMI_RD8( aProtocolContext, &(aRepl->mLogFileSize) );
       CMI_RD8( aProtocolContext, &(aRepl->mOffRpRestartSN) ); */

    CMI_RD4( aProtocolContext, &(sCharsetLen) );
    CMI_RCP( aProtocolContext, (acp_uint8_t *)aOutRepl->mDBCharSet, sCharsetLen );
    aOutRepl->mDBCharSet[sCharsetLen] = '\0';
    
    CMI_RD4( aProtocolContext, &(sNCharsetLen) );
    CMI_RCP( aProtocolContext, (acp_uint8_t *)aOutRepl->mDBNCharSet, sNCharsetLen );
    aOutRepl->mDBNCharSet[sNCharsetLen] = '\0';
    
    CMI_RD4( aProtocolContext, &(sTempLen) );
    CMI_SKIP_READ_BLOCK( aProtocolContext, sTempLen );
    /* CMI_RCP( aProtocolContext, (UChar *)aRepl->mServerID, sServerIDLen ); */
    CMI_RD4( aProtocolContext, &(sTempLen) );
    CMI_SKIP_READ_BLOCK( aProtocolContext, sTempLen );
    /* CMI_RCP( aProtocolContext, (UChar *)aRepl->mRemoteFaultDetectTime, sRemoteFaultDetectTimeLen ); */

    if ( needToProcessProtocolOperation( ULA_META_COMPRESSTYPE,
                                         aOutRepl->mSenderVersion )
         == ACP_TRUE )
    {
        CMI_RD4( aProtocolContext, &( aOutRepl->mCompressType ) );
    }    
    
    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_CHECK_OPERATION_TYPE );
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_ABORT_NET_UNEXPECTED_PROTOCOL,
                        "Meta Replication");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulaCommRecvMetaReplTbl( cmiProtocolContext * aProtocolContext,
                               acp_bool_t         * aExitFlag,
                               ulaTable           * aOutTable,
                               acp_uint32_t         aTimeoutSec,
                               ulaErrorMgr        * aOutErrorMgr )
{
    acp_char_t   sOpCode;
    acp_uint32_t sLocalUserNameLen;
    acp_uint32_t sLocalTableNameLen;
    acp_uint32_t sRemoteUserNameLen;
    acp_uint32_t sRemoteTableNameLen;
    acp_uint32_t sTempLen;

    ACI_TEST( ulaCommReadCmBlock( aProtocolContext,
                                  aExitFlag,
                                  NULL /* TimeoutFlag */,
                                  aTimeoutSec,
                                  aOutErrorMgr )
              != ACI_SUCCESS );

    /* Check Operation Type */
    CMI_RD1( aProtocolContext, sOpCode );

    ACI_TEST_RAISE( sOpCode != CMI_PROTOCOL_OPERATION( RP, MetaReplTbl ),
                    ERR_CHECK_OPERATION_TYPE );

    /* Get Replication Item Information */
    CMI_RD4( aProtocolContext, &(sTempLen) );
    CMI_SKIP_READ_BLOCK( aProtocolContext, sTempLen );
    /* CMI_RCP( aProtocolContext, (UChar *)aItem->mItem.mRepName, sRepNameLen ); */
    CMI_RD4( aProtocolContext, &(sLocalUserNameLen) );
    CMI_RCP( aProtocolContext, (acp_uint8_t *)aOutTable->mFromUserName, sLocalUserNameLen );
    aOutTable->mFromUserName[sLocalUserNameLen] = '\0';
    CMI_RD4( aProtocolContext, &(sLocalTableNameLen) );
    CMI_RCP( aProtocolContext, (acp_uint8_t *)aOutTable->mFromTableName, sLocalTableNameLen );
    aOutTable->mFromTableName[sLocalTableNameLen] = '\0';

    CMI_RD4( aProtocolContext, &(sTempLen) );
    CMI_SKIP_READ_BLOCK( aProtocolContext, sTempLen );
    /* CMI_RCP( aProtocolContext, (UChar *)aItem->mItem.mLocalPartname, sLocalPartNameLen ); */
    CMI_RD4( aProtocolContext, &(sRemoteUserNameLen) );
    CMI_RCP( aProtocolContext, (acp_uint8_t *)aOutTable->mToUserName, sRemoteUserNameLen );
    aOutTable->mToUserName[sRemoteUserNameLen] = '\0';
    CMI_RD4( aProtocolContext, &(sRemoteTableNameLen) );
    CMI_RCP( aProtocolContext, (acp_uint8_t *)aOutTable->mToTableName, sRemoteTableNameLen );
    aOutTable->mToTableName[sRemoteTableNameLen] = '\0';

    CMI_RD4( aProtocolContext, &(sTempLen) );
    CMI_SKIP_READ_BLOCK( aProtocolContext, sTempLen );
    /* CMI_RCP( aProtocolContext, (UChar *)aItem->mItem.mRemotePartname, sRemotePartNameLen ); */
    CMI_RD4( aProtocolContext, &(sTempLen) );
    CMI_SKIP_READ_BLOCK( aProtocolContext, sTempLen );
    /* CMI_RCP( aProtocolContext, (UChar *)aItem->mPartCondMinValues, sPartCondMinValuesLen ); */
    CMI_RD4( aProtocolContext, &(sTempLen) );
    CMI_SKIP_READ_BLOCK( aProtocolContext, ( sTempLen + 8 ) );
    /* CMI_RCP( aProtocolContext, (UChar *)aItem->mPartCondMaxValues, sPartCondMaxValuesLen );
       CMI_RD4( aProtocolContext, &(aItem->mPartitionMethod) );
       CMI_RD4( aProtocolContext, &(aItem->mPartitionOrder) ); */
    CMI_RD8( aProtocolContext, &(aOutTable->mTableOID) );
    CMI_RD4( aProtocolContext, &(aOutTable->mPKIndexID) );
    CMI_RD4( aProtocolContext, &(aOutTable->mPKColumnCount) );
    aOutTable->mPKColumnCount = 0; /* 위에서 받았으나 mPKIndexID를 조사해서 설정 */
    CMI_RD4( aProtocolContext, &(aOutTable->mColumnCount) );
    CMI_RD4( aProtocolContext, &(aOutTable->mIndexCount) );
    
    /* PROJ-1915 Invalid Max SN을 전송 한다. */
    CMI_SKIP_READ_BLOCK( aProtocolContext, 8 );
    /* CMI_RD8( aProtocolContext, &(aItem->mItem.mInvalidMaxSN) ); */

    CMI_RD4( aProtocolContext, &(aOutTable->mCheckCount) );

    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_CHECK_OPERATION_TYPE );
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_ABORT_NET_UNEXPECTED_PROTOCOL,
                        "Meta Table");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulaCommRecvMetaReplCol( cmiProtocolContext * aProtocolContext,
                               acp_bool_t         * aExitFlag,
                               ulaColumn          * aOutColumn,
                               acp_uint32_t         aTimeoutSec,
                               ulaErrorMgr        * aOutErrorMgr )
{
    acp_char_t   sOpCode;
    acp_uint32_t sColumnNameLen;
    acp_uint32_t sColumnFlag;
    acp_uint32_t sTempLen;

    ACI_TEST( ulaCommReadCmBlock( aProtocolContext,
                                  aExitFlag,
                                  NULL /* TimeoutFlag */,
                                  aTimeoutSec,
                                  aOutErrorMgr )
              != ACI_SUCCESS );

    /* Check Operation Type */
    CMI_RD1( aProtocolContext, sOpCode );

    ACI_TEST_RAISE( sOpCode != CMI_PROTOCOL_OPERATION( RP, MetaReplCol ),
                    ERR_CHECK_OPERATION_TYPE );
    /* Get Replication Item Column Information */
    CMI_RD4( aProtocolContext, &(sColumnNameLen) );
    CMI_RCP( aProtocolContext, (acp_uint8_t *)aOutColumn->mColumnName, sColumnNameLen );
    aOutColumn->mColumnName[sColumnNameLen] = '\0';
    CMI_RD4( aProtocolContext, &(aOutColumn->mColumnID) );
    aOutColumn->mColumnID = aOutColumn->mColumnID & ULA_META_COLUMN_ID_MASK;
    CMI_SKIP_READ_BLOCK( aProtocolContext, 8 );
    /* CMI_RD4( aProtocolContext, &(aColumn->mColumn.column.flag) );
       CMI_RD4( aProtocolContext, &(aColumn->mColumn.column.offset) ); */
    CMI_RD4( aProtocolContext, &(aOutColumn->mSize) );
    CMI_RD4( aProtocolContext, &(aOutColumn->mDataType) );
    if ( aOutColumn->mDataType == MTD_ECHAR_ID )
    {
        aOutColumn->mDataType = MTD_CHAR_ID;
        aOutColumn->mEncrypt  = ACP_TRUE;
    }
    else if ( aOutColumn->mDataType == MTD_EVARCHAR_ID )
    {
        aOutColumn->mDataType = MTD_VARCHAR_ID;
        aOutColumn->mEncrypt  = ACP_TRUE;
    }
    else
    {
        aOutColumn->mEncrypt  = ACP_FALSE;
    }
    CMI_RD4( aProtocolContext, &(aOutColumn->mLanguageID) );
    CMI_RD4( aProtocolContext, &(sColumnFlag) );
    if ( ( sColumnFlag & ULA_META_COLUMN_NOTNULL_MASK )
         == ULA_META_COLUMN_NOTNULL_TRUE )
    {
        aOutColumn->mNotNull = ACP_TRUE;
    }
    else
    {
        aOutColumn->mNotNull = ACP_FALSE;
    }
    CMI_RD4( aProtocolContext, (acp_uint32_t*)&(aOutColumn->mPrecision) );
    CMI_RD4( aProtocolContext, (acp_uint32_t*)&(aOutColumn->mScale) );

    CMI_SKIP_READ_BLOCK( aProtocolContext, 4 );
    /* CMI_RD4( aProtocolContext, &(aColumn->mColumn.encPrecision) ); */
    CMI_RD4( aProtocolContext, &(sTempLen) );
    CMI_SKIP_READ_BLOCK( aProtocolContext, sTempLen );
    /* CMI_RCP( aProtocolContext, (UChar *)aColumn->mColumn.policy, sPolicyNameLen ); */
    CMI_RD4( aProtocolContext, &(sTempLen) );
    CMI_SKIP_READ_BLOCK( aProtocolContext, sTempLen );
    /* CMI_RCP( aProtocolContext, (UChar *)aColumn->mPolicyCode, sPolicyCodeLen ); */
    CMI_RD4( aProtocolContext, &(sTempLen) );
    CMI_SKIP_READ_BLOCK( aProtocolContext, sTempLen );
    /* CMI_RCP( aProtocolContext, (UChar *)aColumn->mECCPolicyName, sECCPolicyNameLen ); */
    CMI_RD4( aProtocolContext, &(sTempLen) );
    CMI_SKIP_READ_BLOCK( aProtocolContext, sTempLen );
    /* CMI_RCP( aProtocolContext, (UChar *)aColumn->mECCPolicyCode, sECCPolicyCodeLen ); */

    /* mQcmColumn Flag */
    CMI_RD4( aProtocolContext, &(aOutColumn->mQPFlag) );
    /* DefaultExpression */
    CMI_RD4( aProtocolContext, &(sTempLen) );
    if ( sTempLen != 0 )
    {
        CMI_SKIP_READ_BLOCK( aProtocolContext, sTempLen );
    }
    else
    {
        /* do nothing */
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_CHECK_OPERATION_TYPE );
    {
        ulaSetErrorCode( aOutErrorMgr, ulaERR_ABORT_NET_UNEXPECTED_PROTOCOL,
                         "Meta Column" );
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulaCommRecvMetaReplIdx( cmiProtocolContext * aProtocolContext,
                               acp_bool_t          * aExitFlag,
                               ulaIndex            * aOutIndex,
                               acp_uint32_t          aTimeoutSec,
                               ulaErrorMgr         * aOutErrorMgr)
{
    acp_char_t   sOpCode;
    acp_uint32_t sNameLen;
    acp_uint32_t sIsUnique;

    ACI_TEST( ulaCommReadCmBlock( aProtocolContext,
                                  aExitFlag,
                                  NULL /* TimeoutFlag */,
                                  aTimeoutSec,
                                  aOutErrorMgr )
              != ACI_SUCCESS );

    /* Check Operation Type */
    CMI_RD1( aProtocolContext, sOpCode );

    ACI_TEST_RAISE( sOpCode != CMI_PROTOCOL_OPERATION( RP, MetaReplIdx ),
                    ERR_CHECK_OPERATION_TYPE );

    CMI_RD4( aProtocolContext, &(sNameLen) );
    CMI_RCP( aProtocolContext, (acp_uint8_t *)aOutIndex->mIndexName, sNameLen );
    aOutIndex->mIndexName[sNameLen] = '\0';
    CMI_RD4( aProtocolContext, &(aOutIndex->mIndexID) );
    CMI_SKIP_READ_BLOCK( aProtocolContext, 4 );
    /* CMI_RD4( aProtocolContext, &(aIndex->indexTypeId) ); */
    CMI_RD4( aProtocolContext, &(aOutIndex->mColumnCount) );
    CMI_RD4( aProtocolContext, &(sIsUnique) );
    aOutIndex->mUnique = ( sIsUnique == 0 ) ? ACP_FALSE : ACP_TRUE;
    CMI_SKIP_READ_BLOCK( aProtocolContext, 4 );
    /* CMI_RD4( aProtocolContext, &(sIsRange) ); */

    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_CHECK_OPERATION_TYPE );
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_ABORT_NET_UNEXPECTED_PROTOCOL,
                        "Meta Index");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulaCommRecvMetaReplIdxCol( cmiProtocolContext * aProtocolContext,
                                  acp_bool_t         * aExitFlag,
                                  acp_uint32_t       * aOutColumnID,
                                  acp_uint32_t         aTimeoutSec,
                                  ulaErrorMgr        * aOutErrorMgr )
{
    acp_char_t sOpCode;

    ACI_TEST( ulaCommReadCmBlock( aProtocolContext,
                                  aExitFlag,
                                  NULL /* TimeoutFlag */,
                                  aTimeoutSec,
                                  aOutErrorMgr )
              != ACI_SUCCESS );

    /* Check Operation Type */
    CMI_RD1( aProtocolContext, sOpCode );

    ACI_TEST_RAISE( sOpCode != CMI_PROTOCOL_OPERATION( RP, MetaReplIdxCol ),
                    ERR_CHECK_OPERATION_TYPE );

    /* Get Replication Item Index Information */
    CMI_RD4( aProtocolContext, aOutColumnID );
    *aOutColumnID = *aOutColumnID & ULA_META_COLUMN_ID_MASK;
    CMI_SKIP_READ_BLOCK( aProtocolContext, 4 );
    /* CMI_RD4( aProtocolContext, aKeyColumnFlag ); */

    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_CHECK_OPERATION_TYPE );
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_ABORT_NET_UNEXPECTED_PROTOCOL,
                        "Meta Index Column");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulaCommRecvMetaReplCheck( cmiProtocolContext * aProtocolContext,
                                 acp_bool_t         * aExitFlag,
                                 ulaCheck           * aCheck,
                                 acp_uint32_t         aTimeoutSec,
                                 ulaErrorMgr        * aOutErrorMgr )
{
    acp_char_t      sOpCode;
    acp_uint32_t    i = 0;
    acp_uint32_t    sCheckNameLength = 0;
    acp_uint32_t    sCheckConditionLength = 0;
    acp_rc_t        sRc;

    ACI_TEST( ulaCommReadCmBlock( aProtocolContext,
                                  aExitFlag,
                                  NULL /* TimeoutFlag */,
                                  aTimeoutSec,
                                  aOutErrorMgr )
              != ACI_SUCCESS ); 

    /* Check Operation Type */
    CMI_RD1( aProtocolContext, sOpCode );

    ACI_TEST_RAISE( sOpCode != CMI_PROTOCOL_OPERATION( RP, MetaReplCheck ),
                    ERR_CHECK_OPERATION_TYPE );

    /* Get Replication Item check Information */
    CMI_RD4( aProtocolContext, &sCheckNameLength );
    CMI_RCP( aProtocolContext, aCheck->mName, sCheckNameLength);
    aCheck->mName[sCheckNameLength] = '\0';

    CMI_RD4( aProtocolContext, &(aCheck->mConstraintID) );
    CMI_RD4( aProtocolContext, &(aCheck->mConstraintColumnCount) );

    for ( i = 0; i < aCheck->mConstraintColumnCount; i++)
    {
        CMI_RD4( aProtocolContext, &(aCheck->mConstraintColumn[i]) ); 
    }

    CMI_RD4( aProtocolContext, &sCheckConditionLength );

    sRc = acpMemAlloc( (void **)&(aCheck->mCheckCondition),
                       (sCheckConditionLength + 1) * ACI_SIZEOF(acp_char_t) );
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ULA_ERR_MEM_ALLOC);

    CMI_RCP( aProtocolContext, aCheck->mCheckCondition, sCheckConditionLength );
    aCheck->mCheckCondition[sCheckConditionLength] = '\0';

    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_CHECK_OPERATION_TYPE );
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_ABORT_NET_UNEXPECTED_PROTOCOL,
                        "Meta Check");
    }

    ACI_EXCEPTION(ULA_ERR_MEM_ALLOC)
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_ABORT_MEMORY_ALLOC);
    }

    ACI_EXCEPTION_END;

    if ( aCheck->mCheckCondition != NULL )
    {
        acpMemFree( aCheck->mCheckCondition );
        aCheck->mCheckCondition = NULL;
    }
    else
    {
        /* do nothing */
    }

    return ACI_FAILURE;
}

ACI_RC ulaCommRecvHandshakeAck( cmiProtocolContext * aProtocolContext,
                                acp_bool_t         * aExitFlag,
                                acp_uint32_t       * aResult,
                                acp_char_t         * aMsg,
                                acp_uint32_t       * aMsgLen,
                                acp_uint32_t         aTimeOut,
                                ulaErrorMgr        * aOutErrorMgr )
{
    acp_char_t   sOpCode;

    ACI_TEST( ulaCommReadCmBlock( aProtocolContext,
                                  aExitFlag,
                                  NULL /* TimeoutFlag */,
                                  aTimeOut,
                                  aOutErrorMgr )
        != ACI_SUCCESS );

    CMI_RD1( aProtocolContext, sOpCode );
    ACI_TEST_RAISE( sOpCode != CMI_PROTOCOL_OPERATION( RP, HandshakeAck ),
                 ERR_CHECK_OPERATION_TYPE );

    CMI_RD4( aProtocolContext, aResult ); 
    CMI_SKIP_READ_BLOCK( aProtocolContext, 12 );
    /*
        CMI_RD4( aProtocolContext, (acp_uint32_t*)&sFailbackStatus ); 
        CMI_RD8( aProtocolContext, (acp_uint64_t*)&sXSN ); 
    */
    CMI_RD4( aProtocolContext, aMsgLen ); 
    CMI_RCP( aProtocolContext, (void *)aMsg, *aMsgLen ); 
    aMsg[*aMsgLen] = '\0';

    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_CHECK_OPERATION_TYPE );
    {
        ulaSetErrorCode( aOutErrorMgr, ulaERR_ABORT_NET_FLUSH,
                         "recvHandshake ACK", aciGetErrorCode() );
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulaCommSendHandshakeAck( cmiProtocolContext * aProtocolContext,
                                acp_uint32_t         aResult,
                                acp_char_t         * aMsg,
                                ulaErrorMgr        * aOutErrorMgr )
{
    acp_uint32_t sMsgLen;
    acp_sint32_t sFailbackStatus = 0; /* dummy : RP_FAILBACK_NONE = 0 */
    acp_uint64_t sXSN = ULA_SN_NULL;
    acp_uint64_t sVersion = RP_CURRENT_VERSION;

    if ( aResult == ULA_MSG_OK )
    {
        // copy protocol version.
        sMsgLen = ACI_SIZEOF( acp_uint64_t );
    }
    else
    {
        sMsgLen = acpCStrLen( aMsg, ULA_ACK_MSG_LEN );
    }

    cmiCheckAndFlush( aProtocolContext, 1 + 12 + sMsgLen, ACP_TRUE );

    /* Replication Item Information Set */
    CMI_WR1( aProtocolContext, CMI_PROTOCOL_OPERATION( RP, HandshakeAck ) );
    CMI_WR4( aProtocolContext, &aResult );
    CMI_WR4( aProtocolContext, (acp_uint32_t*)&sFailbackStatus ); /* dummy */
    CMI_WR8( aProtocolContext, (acp_uint64_t*)&sXSN ); /* dummy */
    CMI_WR4( aProtocolContext, &sMsgLen );

    if ( aResult == ULA_MSG_OK )
    {
        CMI_WR8( aProtocolContext, (acp_uint64_t*)&sVersion );
    }
    else
    {
        CMI_WCP( aProtocolContext, (acp_uint8_t *)aMsg, sMsgLen );
    }

    ACI_TEST_RAISE( cmiSend( aProtocolContext, ACP_TRUE )
                    != ACI_SUCCESS, ERR_SEND );

    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_SEND );
    {
        ulaSetErrorCode( aOutErrorMgr, ulaERR_ABORT_NET_FLUSH,
                         "Handshake ACK", aciGetErrorCode() );
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulaCommRecvXLog( cmiProtocolContext * aProtocolContext,
                        acp_bool_t         * aExitFlag,
                        ulaXLog            * aOutXLog,
                        acl_mem_alloc_t    * aAllocator,
                        acp_uint32_t         aTimeoutSec,
                        ulaErrorMgr        * aOutErrorMgr )
{
    acp_char_t sOpCode;

    if ( CMI_IS_READ_ALL( aProtocolContext ) == ACP_TRUE )
    {
        ACI_TEST( ulaCommReadCmBlock( aProtocolContext,
                                      aExitFlag,
                                      NULL /* TimeoutFlag */,
                                      aTimeoutSec,
                                      aOutErrorMgr )
                  != ACI_SUCCESS );
    }

    CMI_RD1( aProtocolContext, sOpCode );

    switch ( sOpCode )
    {
        case CMI_PROTOCOL_OPERATION( RP, TrBegin ):
        {
            // PROJ-1663 : BEGIN 패킷 미사용
            ACI_TEST( ulaCommRecvTrBegin( aExitFlag,
                                          aProtocolContext,
                                          aOutXLog,
                                          aTimeoutSec,
                                          aOutErrorMgr )
                      != ACI_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, TrCommit ):
        {
            ACI_TEST( ulaCommRecvTrCommit( aExitFlag,
                                           aProtocolContext,
                                           aOutXLog,
                                           aTimeoutSec,
                                           aOutErrorMgr )
                      != ACI_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, TrAbort ):
        {
            ACI_TEST( ulaCommRecvTrAbort( aExitFlag,
                                          aProtocolContext,
                                          aOutXLog,
                                          aTimeoutSec,
                                          aOutErrorMgr )
                      != ACI_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, SPSet ):
        {
            ACI_TEST( ulaCommRecvSPSet( aExitFlag,
                                        aProtocolContext,
                                        aOutXLog,
                                        aAllocator,
                                        aTimeoutSec,
                                        aOutErrorMgr )
                      != ACI_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, SPAbort ):
        {
            ACI_TEST(ulaCommRecvSPAbort(aExitFlag,
                                        aProtocolContext,
                                        aOutXLog,
                                        aAllocator,
                                        aTimeoutSec,
                                        aOutErrorMgr)
                     != ACI_SUCCESS);
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, Insert ):
        {
            ACI_TEST(ulaCommRecvInsert(aExitFlag,
                                       aProtocolContext,
                                       aOutXLog,
                                       aAllocator,
                                       aTimeoutSec,
                                       aOutErrorMgr)
                     != ACI_SUCCESS);
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, Update ):
        {
            ACI_TEST(ulaCommRecvUpdate(aExitFlag,
                                       aProtocolContext,
                                       aOutXLog,
                                       aAllocator,
                                       aTimeoutSec,
                                       aOutErrorMgr)
                     != ACI_SUCCESS);
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, Delete ):
        {
            ACI_TEST(ulaCommRecvDelete(aExitFlag,
                                       aProtocolContext,
                                       aOutXLog,
                                       aAllocator,
                                       aTimeoutSec,
                                       aOutErrorMgr)
                     != ACI_SUCCESS);
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, Stop ):
        {
            ACI_TEST( ulaCommRecvStop( aExitFlag,
                                       aProtocolContext,
                                       aOutXLog,
                                       aTimeoutSec,
                                       aOutErrorMgr )
                      != ACI_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, KeepAlive ):
        {
            ACI_TEST( ulaCommRecvKeepAlive( aExitFlag,
                                            aProtocolContext,
                                            aOutXLog,
                                            aTimeoutSec,
                                            aOutErrorMgr )
                      != ACI_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, LobCursorOpen ):
        {
            ACI_TEST(ulaCommRecvLobCursorOpen(aExitFlag,
                                              aProtocolContext,
                                              aOutXLog,
                                              aAllocator,
                                              aTimeoutSec,
                                              aOutErrorMgr)
                     != ACI_SUCCESS);
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, LobCursorClose ):
        {
            ACI_TEST(ulaCommRecvLobCursorClose(aExitFlag,
                                               aProtocolContext,
                                               aOutXLog,
                                               aTimeoutSec,
                                               aOutErrorMgr)
                     != ACI_SUCCESS);
            break;
        }
        case CMI_PROTOCOL_OPERATION(RP, LobPrepare4Write):
        {
            ACI_TEST(ulaCommRecvLobPrepare4Write(aExitFlag,
                                                 aProtocolContext,
                                                 aOutXLog,
                                                 aTimeoutSec,
                                                 aOutErrorMgr )
                      != ACI_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, LobPartialWrite ):
        {
            ACI_TEST(ulaCommRecvLobPartialWrite(aExitFlag,
                                                aProtocolContext,
                                                aOutXLog,
                                                aAllocator,
                                                aTimeoutSec,
                                                aOutErrorMgr)
                     != ACI_SUCCESS);
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, LobFinish2Write ):
        {
            ACI_TEST( ulaCommRecvLobFinish2Write( aExitFlag,
                                                  aProtocolContext,
                                                  aOutXLog,
                                                  aTimeoutSec,
                                                  aOutErrorMgr )
                      != ACI_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION(RP, LobTrim):
        {
            ACI_TEST(ulaCommRecvLobTrim(aExitFlag,
                                        aProtocolContext,
                                        aOutXLog,
                                        aTimeoutSec,
                                        aOutErrorMgr )
                     != ACI_SUCCESS );
            break;
        }
        case CMI_PROTOCOL_OPERATION( RP, Handshake ):
        {
            ACI_TEST( ulaCommRecvHandshake( aExitFlag,
                                       aProtocolContext,
                                       aOutXLog,
                                       aTimeoutSec,
                                       aOutErrorMgr )
                      != ACI_SUCCESS );
            break;
        }

        default:
            ACI_RAISE( ERR_PROTOCOL_OPID );
    }
    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_PROTOCOL_OPID )
    {
        ulaSetErrorCode( aOutErrorMgr, ulaERR_ABORT_NET_UNEXPECTED_PROTOCOL,
                         "Receive XLog" );
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulaCommRecvHandshake( acp_bool_t         * aExitFlag,
                             cmiProtocolContext * aProtocolContext,
                             ulaXLog            * aOutXLog,
                             acp_uint32_t         aTimeoutSec,
                             ulaErrorMgr        * aOutErrorMgr )
{
    ACP_UNUSED( aExitFlag );
    ACP_UNUSED( aTimeoutSec );
    ACP_UNUSED( aOutErrorMgr );
    
    /* Get Arguemnt XLog Header */
    CMI_RD4( aProtocolContext, &(aOutXLog->mHeader.mType) );
    CMI_RD4( aProtocolContext, &(aOutXLog->mHeader.mTID) );
    CMI_RD8( aProtocolContext, &(aOutXLog->mHeader.mSN) );
    CMI_RD8( aProtocolContext, &(aOutXLog->mHeader.mSyncSN) );
    
    return ACI_SUCCESS;
}

ACI_RC ulaCommRecvTrBegin( acp_bool_t         * aExitFlag,
                           cmiProtocolContext * aProtocolContext,
                           ulaXLog            * aOutXLog,
                           acp_uint32_t         aTimeoutSec,
                           ulaErrorMgr        * aOutErrorMgr )
{
    ACP_UNUSED( aExitFlag );
    ACP_UNUSED( aTimeoutSec );
    ACP_UNUSED( aOutErrorMgr );

    /* Get Arguemnt XLog Header */
    CMI_RD4( aProtocolContext, &(aOutXLog->mHeader.mType) );
    CMI_RD4( aProtocolContext, &(aOutXLog->mHeader.mTID) );
    CMI_RD8( aProtocolContext, &(aOutXLog->mHeader.mSN) );
    CMI_RD8( aProtocolContext, &(aOutXLog->mHeader.mSyncSN) );

    return ACI_SUCCESS;
}

ACI_RC ulaCommRecvTrCommit( acp_bool_t         *aExitFlag,
                            cmiProtocolContext *aProtocolContext,
                            ulaXLog            *aOutXLog,
                            acp_uint32_t        aTimeoutSec,
                            ulaErrorMgr        *aOutErrorMgr )
{
    ACP_UNUSED( aExitFlag );
    ACP_UNUSED( aTimeoutSec );
    ACP_UNUSED( aOutErrorMgr );

    /* Get Arguemnt XLog Header */
    CMI_RD4( aProtocolContext, &(aOutXLog->mHeader.mType) );
    CMI_RD4( aProtocolContext, &(aOutXLog->mHeader.mTID) );
    CMI_RD8( aProtocolContext, &(aOutXLog->mHeader.mSN) );
    CMI_RD8( aProtocolContext, &(aOutXLog->mHeader.mSyncSN) );

    return ACI_SUCCESS;
}

ACI_RC ulaCommRecvTrAbort( acp_bool_t         *aExitFlag,
                           cmiProtocolContext *aProtocolContext,
                           ulaXLog            *aOutXLog,
                           acp_uint32_t        aTimeoutSec,
                           ulaErrorMgr        *aOutErrorMgr )
{
    ACP_UNUSED( aExitFlag );
    ACP_UNUSED( aTimeoutSec );
    ACP_UNUSED( aOutErrorMgr );

    /* Get Arguemnt XLog Header */
    CMI_RD4( aProtocolContext, &(aOutXLog->mHeader.mType) );
    CMI_RD4( aProtocolContext, &(aOutXLog->mHeader.mTID) );
    CMI_RD8( aProtocolContext, &(aOutXLog->mHeader.mSN) );
    CMI_RD8( aProtocolContext, &(aOutXLog->mHeader.mSyncSN) );

    return ACI_SUCCESS;
}

ACI_RC ulaCommRecvSPSet( acp_bool_t         * aExitFlag,
                         cmiProtocolContext * aProtocolContext,
                         ulaXLog            * aOutXLog,
                         acl_mem_alloc_t    * aAllocator,
                         acp_uint32_t         aTimeoutSec,
                         ulaErrorMgr        * aOutErrorMgr )
{
    acp_rc_t sRc;

    ACP_UNUSED( aExitFlag );
    ACP_UNUSED( aTimeoutSec );

    /* Get Arguemnt XLog Header */
    CMI_RD4( aProtocolContext, &(aOutXLog->mHeader.mType) );
    CMI_RD4( aProtocolContext, &(aOutXLog->mHeader.mTID) );
    CMI_RD8( aProtocolContext, &(aOutXLog->mHeader.mSN) );
    CMI_RD8( aProtocolContext, &(aOutXLog->mHeader.mSyncSN) );

    CMI_RD4( aProtocolContext, &(aOutXLog->mSavepoint.mSPNameLen) );
    aOutXLog->mSavepoint.mSPName = NULL;

    sRc = aclMemAlloc( aAllocator,
                       (void **)&aOutXLog->mSavepoint.mSPName,
                       aOutXLog->mSavepoint.mSPNameLen + 1 );
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ULA_ERR_MEM_ALLOC);
    acpMemSet(aOutXLog->mSavepoint.mSPName,
              0x00,
              aOutXLog->mSavepoint.mSPNameLen + 1);

    CMI_RCP( aProtocolContext,
             (acp_uint8_t *)aOutXLog->mSavepoint.mSPName,
             aOutXLog->mSavepoint.mSPNameLen );
    aOutXLog->mSavepoint.mSPName[aOutXLog->mSavepoint.mSPNameLen] = '\0';

    return ACI_SUCCESS;

    ACI_EXCEPTION( ULA_ERR_MEM_ALLOC )
    {
        ulaSetErrorCode( aOutErrorMgr, ulaERR_ABORT_MEMORY_ALLOC );
    }
    ACI_EXCEPTION_END;

    if ( aOutXLog->mSavepoint.mSPName != NULL )
    {
        (void)aclMemFree( aAllocator, aOutXLog->mSavepoint.mSPName );
        aOutXLog->mSavepoint.mSPName = NULL;
    }

    return ACI_FAILURE;
}

ACI_RC ulaCommRecvSPAbort( acp_bool_t         * aExitFlag,
                           cmiProtocolContext * aProtocolContext,
                           ulaXLog            * aOutXLog,
                           acl_mem_alloc_t    * aAllocator,
                           acp_uint32_t         aTimeoutSec,
                           ulaErrorMgr        * aOutErrorMgr )
{
    acp_rc_t sRc;

    ACP_UNUSED( aExitFlag );
    ACP_UNUSED( aTimeoutSec );

    /* Get Arguemnt XLog Header */
    CMI_RD4( aProtocolContext, &(aOutXLog->mHeader.mType) );
    CMI_RD4( aProtocolContext, &(aOutXLog->mHeader.mTID) );
    CMI_RD8( aProtocolContext, &(aOutXLog->mHeader.mSN) );
    CMI_RD8( aProtocolContext, &(aOutXLog->mHeader.mSyncSN) );

    CMI_RD4( aProtocolContext, &(aOutXLog->mSavepoint.mSPNameLen) );
    aOutXLog->mSavepoint.mSPName = NULL;

    sRc = aclMemAlloc( aAllocator,
                       (void **)&aOutXLog->mSavepoint.mSPName,
                       aOutXLog->mSavepoint.mSPNameLen + 1 );
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ULA_ERR_MEM_ALLOC);
    acpMemSet(aOutXLog->mSavepoint.mSPName,
              0x00,
              aOutXLog->mSavepoint.mSPNameLen + 1);

    CMI_RCP( aProtocolContext,
             (acp_uint8_t *)aOutXLog->mSavepoint.mSPName,
             aOutXLog->mSavepoint.mSPNameLen );
    aOutXLog->mSavepoint.mSPName[aOutXLog->mSavepoint.mSPNameLen] = '\0';

    return ACI_SUCCESS;

    ACI_EXCEPTION( ULA_ERR_MEM_ALLOC )
    {
        ulaSetErrorCode( aOutErrorMgr, ulaERR_ABORT_MEMORY_ALLOC );
    }
    ACI_EXCEPTION_END;

    if ( aOutXLog->mSavepoint.mSPName != NULL )
    {
        (void)aclMemFree( aAllocator, aOutXLog->mSavepoint.mSPName );
        aOutXLog->mSavepoint.mSPName = NULL;
    }

    return ACI_FAILURE;
}

ACI_RC ulaCommRecvInsert( acp_bool_t         * aExitFlag,
                          cmiProtocolContext * aProtocolContext,
                          ulaXLog            * aOutXLog,
                          acl_mem_alloc_t    * aAllocator,
                          acp_uint32_t         aTimeoutSec,
                          ulaErrorMgr        * aOutErrorMgr )
{
    acp_uint32_t i;
    acp_uint32_t sImplSPDepth;
    acp_rc_t sRc;

    /* Get Arguemnt XLog Header */
    CMI_RD4( aProtocolContext, &(aOutXLog->mHeader.mType) );
    CMI_RD4( aProtocolContext, &(aOutXLog->mHeader.mTID) );
    CMI_RD8( aProtocolContext, &(aOutXLog->mHeader.mSN) );
    CMI_RD8( aProtocolContext, &(aOutXLog->mHeader.mSyncSN) );
    CMI_RD4( aProtocolContext, &(sImplSPDepth) );
    CMI_RD8( aProtocolContext, &(aOutXLog->mHeader.mTableOID) );
    CMI_RD4( aProtocolContext, &(aOutXLog->mColumn.mColCnt) );
    aOutXLog->mColumn.mCIDArray  = NULL;
    aOutXLog->mColumn.mBColArray = NULL;
    aOutXLog->mColumn.mAColArray = NULL;

    sRc = aclMemAlloc( aAllocator,
                       (void **)&aOutXLog->mColumn.mCIDArray,
                       aOutXLog->mColumn.mColCnt * ACI_SIZEOF(acp_uint32_t) );
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ULA_ERR_MEM_ALLOC);

    sRc = aclMemAlloc( aAllocator,
                       (void **)&aOutXLog->mColumn.mAColArray,
                       aOutXLog->mColumn.mColCnt * ACI_SIZEOF(ulaValue) );
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ULA_ERR_MEM_ALLOC);

    acpMemSet( aOutXLog->mColumn.mAColArray,
               0x00,
               aOutXLog->mColumn.mColCnt * ACI_SIZEOF( ulaValue ) );

    /* Recv Value repeatedly */
    for ( i = 0; i < aOutXLog->mColumn.mColCnt; i++ )
    {
        aOutXLog->mColumn.mCIDArray[i] = i;

        ACI_TEST(ulaCommRecvValue(aExitFlag,
                                  aProtocolContext,
                                  &(aOutXLog->mColumn.mAColArray[i]),
                                  aAllocator,
                                  aTimeoutSec,
                                  aOutErrorMgr)
                 != ACI_SUCCESS);
    }

    // PROJ-1663 : IMPLICIT SAVEPOINT SET
    if (sImplSPDepth != ULA_STATEMENT_DEPTH_NULL)
    {
        ACE_DASSERT( sImplSPDepth <= ULA_STATEMENT_DEPTH_MAX );

        sRc = aclMemAlloc( aAllocator,
                           (void **)&aOutXLog->mSavepoint.mSPName,
                           ULA_IMPLICIT_SVP_NAME_SIZE +
                           ULA_STATEMENT_DEPTH_MAX_SIZE + 1 );
        ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ULA_ERR_MEM_ALLOC);

        (void)acpSnprintf( aOutXLog->mSavepoint.mSPName,
                           ULA_IMPLICIT_SVP_NAME_SIZE +
                           ULA_STATEMENT_DEPTH_MAX_SIZE + 1,
                           "%s%u",
                           ULA_IMPLICIT_SVP_NAME,
                           sImplSPDepth );

        aOutXLog->mSavepoint.mSPNameLen =
            acpCStrLen( aOutXLog->mSavepoint.mSPName,
                        ULA_IMPLICIT_SVP_NAME_SIZE +
                        ULA_STATEMENT_DEPTH_MAX_SIZE + 1 );
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION( ULA_ERR_MEM_ALLOC )
    {
        ulaSetErrorCode( aOutErrorMgr, ulaERR_ABORT_MEMORY_ALLOC );
    }

    ACI_EXCEPTION_END;
    // 이미 ulaSetErrorCode() 수행

    if ( aOutXLog->mColumn.mCIDArray != NULL )
    {
        (void)aclMemFree( aAllocator, aOutXLog->mColumn.mCIDArray );
        aOutXLog->mColumn.mCIDArray = NULL;
    }

    if ( aOutXLog->mColumn.mAColArray != NULL )
    {
        (void)aclMemFree( aAllocator, aOutXLog->mColumn.mAColArray );
        aOutXLog->mColumn.mAColArray = NULL;
    }

    if ( aOutXLog->mSavepoint.mSPName != NULL )
    {
        (void)aclMemFree( aAllocator, aOutXLog->mSavepoint.mSPName );
        aOutXLog->mSavepoint.mSPName = NULL;
    }

    return ACI_FAILURE;
}

ACI_RC ulaCommRecvUpdate( acp_bool_t         * aExitFlag,
                          cmiProtocolContext * aProtocolContext,
                          ulaXLog            * aOutXLog,
                          acl_mem_alloc_t    * aAllocator,
                          acp_uint32_t         aTimeoutSec,
                          ulaErrorMgr        * aOutErrorMgr )
{
    acp_uint32_t i;
    acp_uint32_t sImplSPDepth;
    acp_rc_t     sRc;

    /* Get Arguemnt XLog Header */
    CMI_RD4( aProtocolContext, &(aOutXLog->mHeader.mType) );
    CMI_RD4( aProtocolContext, &(aOutXLog->mHeader.mTID) );
    CMI_RD8( aProtocolContext, &(aOutXLog->mHeader.mSN) );
    CMI_RD8( aProtocolContext, &(aOutXLog->mHeader.mSyncSN) );
    CMI_RD4( aProtocolContext, &(sImplSPDepth) );
    CMI_RD8( aProtocolContext, &(aOutXLog->mHeader.mTableOID) );

    /* Get Arguemnt PKColumn, Column */
    CMI_RD4( aProtocolContext, &(aOutXLog->mPrimaryKey.mPKColCnt) );
    aOutXLog->mPrimaryKey.mPKColArray = NULL;
    CMI_RD4( aProtocolContext, &(aOutXLog->mColumn.mColCnt) );
    aOutXLog->mColumn.mCIDArray  = NULL;
    aOutXLog->mColumn.mBColArray = NULL;
    aOutXLog->mColumn.mAColArray = NULL;

    sRc = aclMemAlloc( aAllocator,
                       (void **)&aOutXLog->mPrimaryKey.mPKColArray,
                       aOutXLog->mPrimaryKey.mPKColCnt * ACI_SIZEOF(ulaValue) );
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ULA_ERR_MEM_ALLOC);
    acpMemSet(aOutXLog->mPrimaryKey.mPKColArray,
              0x00,
              aOutXLog->mPrimaryKey.mPKColCnt * ACI_SIZEOF(ulaValue));

    sRc = aclMemAlloc( aAllocator,
                       (void **)&aOutXLog->mColumn.mCIDArray,
                       aOutXLog->mColumn.mColCnt * ACI_SIZEOF(acp_uint32_t) );
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ULA_ERR_MEM_ALLOC);

    sRc = aclMemAlloc( aAllocator,
                       (void **)&aOutXLog->mColumn.mBColArray,
                       aOutXLog->mColumn.mColCnt * ACI_SIZEOF(ulaValue) );
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ULA_ERR_MEM_ALLOC);
    acpMemSet(aOutXLog->mColumn.mBColArray,
              0x00,
              aOutXLog->mColumn.mColCnt * ACI_SIZEOF(ulaValue));

    sRc = aclMemAlloc( aAllocator,
                       (void **)&aOutXLog->mColumn.mAColArray,
                       aOutXLog->mColumn.mColCnt * ACI_SIZEOF(ulaValue) );
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ULA_ERR_MEM_ALLOC);
    acpMemSet(aOutXLog->mColumn.mAColArray,
              0x00,
              aOutXLog->mColumn.mColCnt * ACI_SIZEOF(ulaValue));

    /* Recv PK Value repeatedly */
    for ( i = 0; i < aOutXLog->mPrimaryKey.mPKColCnt; i++ )
    {
        ACI_TEST(ulaCommRecvValue(aExitFlag,
                                  aProtocolContext,
                                  &(aOutXLog->mPrimaryKey.mPKColArray[i]),
                                  aAllocator,
                                  aTimeoutSec,
                                  aOutErrorMgr)
                 != ACI_SUCCESS);
    }

    /* Recv Update Value repeatedly */
    for ( i = 0; i < aOutXLog->mColumn.mColCnt; i++ )
    {
        ACI_TEST( ulaCommRecvCID( aExitFlag,
                                  aProtocolContext,
                                  &(aOutXLog->mColumn.mCIDArray[i]),
                                  aTimeoutSec,
                                  aOutErrorMgr)
                 != ACI_SUCCESS);

        ACI_TEST(ulaCommRecvValue(aExitFlag,
                                  aProtocolContext,
                                  &(aOutXLog->mColumn.mBColArray[i]),
                                  aAllocator,
                                  aTimeoutSec,
                                  aOutErrorMgr)
                 != ACI_SUCCESS);

        ACI_TEST(ulaCommRecvValue(aExitFlag,
                                  aProtocolContext,
                                  &(aOutXLog->mColumn.mAColArray[i]),
                                  aAllocator,
                                  aTimeoutSec,
                                  aOutErrorMgr)
                 != ACI_SUCCESS);
    }

    // PROJ-1663 : IMPLICIT SAVEPOINT SET
    if ( sImplSPDepth != ULA_STATEMENT_DEPTH_NULL )
    {
        ACE_DASSERT( sImplSPDepth <= ULA_STATEMENT_DEPTH_MAX );

        sRc = aclMemAlloc( aAllocator,
                           (void **)&aOutXLog->mSavepoint.mSPName,
                           ULA_IMPLICIT_SVP_NAME_SIZE +
                           ULA_STATEMENT_DEPTH_MAX_SIZE + 1 );
        ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ULA_ERR_MEM_ALLOC);
        (void)acpSnprintf(aOutXLog->mSavepoint.mSPName,
                          ULA_IMPLICIT_SVP_NAME_SIZE +
                          ULA_STATEMENT_DEPTH_MAX_SIZE + 1,
                          "%s%u",
                          ULA_IMPLICIT_SVP_NAME,
                          sImplSPDepth);

        aOutXLog->mSavepoint.mSPNameLen =
            acpCStrLen( aOutXLog->mSavepoint.mSPName,
                        ULA_IMPLICIT_SVP_NAME_SIZE +
                        ULA_STATEMENT_DEPTH_MAX_SIZE + 1 );
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION( ULA_ERR_MEM_ALLOC )
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_ABORT_MEMORY_ALLOC);
    }
    ACI_EXCEPTION_END;
    // 이미 ulaSetErrorCode() 수행

    if ( aOutXLog->mPrimaryKey.mPKColArray != NULL )
    {
        (void)aclMemFree( aAllocator, aOutXLog->mPrimaryKey.mPKColArray );
        aOutXLog->mPrimaryKey.mPKColArray = NULL;
    }

    if ( aOutXLog->mColumn.mCIDArray != NULL )
    {
        (void)aclMemFree( aAllocator, aOutXLog->mColumn.mCIDArray );
        aOutXLog->mColumn.mCIDArray = NULL;
    }

    if ( aOutXLog->mColumn.mBColArray != NULL )
    {
        (void)aclMemFree( aAllocator, aOutXLog->mColumn.mBColArray );
        aOutXLog->mColumn.mBColArray = NULL;
    }

    if ( aOutXLog->mColumn.mAColArray != NULL)
    {
        (void)aclMemFree( aAllocator, aOutXLog->mColumn.mAColArray );
        aOutXLog->mColumn.mAColArray = NULL;
    }

    if ( aOutXLog->mSavepoint.mSPName != NULL )
    {
        (void)aclMemFree( aAllocator, aOutXLog->mSavepoint.mSPName );
        aOutXLog->mSavepoint.mSPName = NULL;
    }

    return ACI_FAILURE;
}

ACI_RC ulaCommRecvDelete( acp_bool_t         * aExitFlag,
                          cmiProtocolContext * aProtocolContext,
                          ulaXLog            * aOutXLog,
                          acl_mem_alloc_t    * aAllocator,
                          acp_uint32_t         aTimeoutSec,
                          ulaErrorMgr        * aOutErrorMgr )
{
    acp_uint32_t i;
    acp_uint32_t sImplSPDepth;
    acp_rc_t sRc;

    /* Get Arguemnt XLog Header */
    CMI_RD4( aProtocolContext, &(aOutXLog->mHeader.mType) );
    CMI_RD4( aProtocolContext, &(aOutXLog->mHeader.mTID) );
    CMI_RD8( aProtocolContext, &(aOutXLog->mHeader.mSN) );
    CMI_RD8( aProtocolContext, &(aOutXLog->mHeader.mSyncSN) );
    CMI_RD4( aProtocolContext, &(sImplSPDepth) );
    CMI_RD8( aProtocolContext, &(aOutXLog->mHeader.mTableOID) );
    /* Get Arguemnt PKColumn */
    CMI_RD4( aProtocolContext, &(aOutXLog->mPrimaryKey.mPKColCnt) );
    aOutXLog->mPrimaryKey.mPKColArray = NULL;

    CMI_RD4( aProtocolContext, &(aOutXLog->mColumn.mColCnt) );
    aOutXLog->mColumn.mCIDArray  = NULL;
    aOutXLog->mColumn.mBColArray = NULL;
    aOutXLog->mColumn.mAColArray = NULL;

    sRc = aclMemAlloc( aAllocator,
                       (void **)&aOutXLog->mPrimaryKey.mPKColArray,
                       aOutXLog->mPrimaryKey.mPKColCnt * ACI_SIZEOF(ulaValue) );
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ULA_ERR_MEM_ALLOC);
    acpMemSet(aOutXLog->mPrimaryKey.mPKColArray,
              0x00,
              aOutXLog->mPrimaryKey.mPKColCnt * ACI_SIZEOF(ulaValue));

    sRc = aclMemAlloc( aAllocator,
                       (void **)&aOutXLog->mColumn.mCIDArray,
                       aOutXLog->mColumn.mColCnt * ACI_SIZEOF(acp_uint32_t) );
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ULA_ERR_MEM_ALLOC);

    sRc = aclMemAlloc( aAllocator,
                       (void **)&aOutXLog->mColumn.mBColArray,
                       aOutXLog->mColumn.mColCnt * ACI_SIZEOF(ulaValue) );
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ULA_ERR_MEM_ALLOC);
    acpMemSet(aOutXLog->mColumn.mBColArray,
              0x00,
              aOutXLog->mColumn.mColCnt * ACI_SIZEOF(ulaValue));

    /* Recv PK Value repeatedly */
    for (i = 0; i < aOutXLog->mPrimaryKey.mPKColCnt; i++)
    {
        ACI_TEST(ulaCommRecvValue(aExitFlag,
                                  aProtocolContext,
                                  &(aOutXLog->mPrimaryKey.mPKColArray[i]),
                                  aAllocator,
                                  aTimeoutSec,
                                  aOutErrorMgr)
                 != ACI_SUCCESS);
    }

    /* Receive old values */
    for ( i = 0; i < aOutXLog->mColumn.mColCnt; i++ )
    {
        ACI_TEST(ulaCommRecvCID(aExitFlag,
                                aProtocolContext,
                                &(aOutXLog->mColumn.mCIDArray[i]),
                                aTimeoutSec,
                                aOutErrorMgr)
                 != ACI_SUCCESS);

        ACI_TEST(ulaCommRecvValue(aExitFlag,
                                  aProtocolContext,
                                  &(aOutXLog->mColumn.mBColArray[i]),
                                  aAllocator,
                                  aTimeoutSec,
                                  aOutErrorMgr)
                 != ACI_SUCCESS);
    }

    // PROJ-1663 : IMPLICIT SAVEPOINT SET
    if ( sImplSPDepth != ULA_STATEMENT_DEPTH_NULL )
    {
        ACE_DASSERT(sImplSPDepth <= ULA_STATEMENT_DEPTH_MAX);

        sRc = aclMemAlloc( aAllocator,
                           (void **)&aOutXLog->mSavepoint.mSPName,
                           ULA_IMPLICIT_SVP_NAME_SIZE +
                           ULA_STATEMENT_DEPTH_MAX_SIZE + 1 );
        ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ULA_ERR_MEM_ALLOC);
        (void)acpSnprintf(aOutXLog->mSavepoint.mSPName, 
                          ULA_IMPLICIT_SVP_NAME_SIZE +
                          ULA_STATEMENT_DEPTH_MAX_SIZE + 1,
                          "%s%u",
                          ULA_IMPLICIT_SVP_NAME,
                          sImplSPDepth);
        aOutXLog->mSavepoint.mSPNameLen =
            acpCStrLen(aOutXLog->mSavepoint.mSPName,
                       ULA_IMPLICIT_SVP_NAME_SIZE +
                       ULA_STATEMENT_DEPTH_MAX_SIZE + 1);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION( ULA_ERR_MEM_ALLOC )
    {
        ulaSetErrorCode( aOutErrorMgr, ulaERR_ABORT_MEMORY_ALLOC );
    }
    ACI_EXCEPTION_END;
    // 이미 ulaSetErrorCode() 수행

    if (aOutXLog->mPrimaryKey.mPKColArray != NULL)
    {
        (void)aclMemFree( aAllocator, aOutXLog->mPrimaryKey.mPKColArray );
        aOutXLog->mPrimaryKey.mPKColArray = NULL;
    }

    if (aOutXLog->mSavepoint.mSPName != NULL)
    {
        (void)aclMemFree( aAllocator, aOutXLog->mSavepoint.mSPName );
        aOutXLog->mSavepoint.mSPName = NULL;
    }

    return ACI_FAILURE;
}

ACI_RC ulaCommRecvCID( acp_bool_t         * aExitFlag,
                       cmiProtocolContext * aProtocolContext,
                       acp_uint32_t       * aOutCID,
                       acp_uint32_t         aTimeoutSec,
                       ulaErrorMgr        * aOutErrorMgr )
{
    acp_char_t sOpCode;

    if ( CMI_IS_READ_ALL( aProtocolContext ) == ACP_TRUE )
    {
        ACI_TEST( ulaCommReadCmBlock( aProtocolContext,
                                      aExitFlag,
                                      NULL /* TimeoutFlag */,
                                      aTimeoutSec,
                                      aOutErrorMgr )
                  != ACI_SUCCESS );
    }

    CMI_RD1( aProtocolContext, sOpCode );

    ACI_TEST_RAISE( sOpCode != CMI_PROTOCOL_OPERATION( RP, UIntID ),
                    ERR_PROTOCOL_OPID );
    /* Get Argument */
    CMI_RD4( aProtocolContext, aOutCID );
    *aOutCID = *aOutCID & ULA_META_COLUMN_ID_MASK;

    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_PROTOCOL_OPID )
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_ABORT_NET_UNEXPECTED_PROTOCOL,
                        "XLog Column ID");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulaCommRecvValue( acp_bool_t         * aExitFlag,
                         cmiProtocolContext * aProtocolContext,
                         ulaValue           * aOutValue,
                         acl_mem_alloc_t    * aAllocator,
                         acp_uint32_t         aTimeoutSec,
                         ulaErrorMgr        * aOutErrorMgr)
{
    acp_char_t sOpCode;
    acp_uint32_t sRemainDataInCmBlock;
    acp_uint32_t sRemainDataOfValue;
    acp_rc_t sRc;

    ACP_UNUSED( aTimeoutSec );

    aOutValue->value = NULL;

    if ( CMI_IS_READ_ALL( aProtocolContext ) == ACP_TRUE )
    {
        ACI_TEST( ulaCommReadCmBlock( aProtocolContext,
                                      aExitFlag,
                                      NULL, /* TimeoutFlag */
                                      aTimeoutSec,
                                      aOutErrorMgr )
                  != ACI_SUCCESS );
    }

    CMI_RD1( aProtocolContext, sOpCode );
    ACI_TEST_RAISE( sOpCode != CMI_PROTOCOL_OPERATION( RP, Value ),
                    ERR_CHECK_OPERATION_TYPE );

    /* Get Argument */
    CMI_RD4( aProtocolContext, &(aOutValue->length) );

    /* NOTICE!!!!!
     * 실제 Value에 대한 값을 저장하는 메모리 공간을 여기서 할당받는다.
     * 따라서, 이 XLog를 사용한 쪽에서 여기서 할당해준 메모리 공간을
     * 해제해 주어야 한다.
     */
    if ( aOutValue->length > 0 )
    {
        sRc = aclMemAlloc( aAllocator,
                           (void **)&aOutValue->value,
                           aOutValue->length );
        ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ULA_ERR_MEM_ALLOC);

        sRemainDataOfValue = aOutValue->length;
        sRemainDataInCmBlock = CMI_REMAIN_DATA_IN_READ_BLOCK( aProtocolContext );
        while ( sRemainDataOfValue > 0 )
        {
            if ( sRemainDataInCmBlock == 0 )
            {
                ACI_TEST( ulaCommReadCmBlock( aProtocolContext,
                                              aExitFlag,
                                              NULL /* TimeoutFlag */,
                                              aTimeoutSec,
                                              aOutErrorMgr )
                          != ACI_SUCCESS );
                sRemainDataInCmBlock = aProtocolContext->mReadBlock->mDataSize;
            }
            else
            {
                if ( sRemainDataInCmBlock >= sRemainDataOfValue )
                {
                    CMI_RCP( aProtocolContext,
                             (acp_uint8_t *)aOutValue->value + ( aOutValue->length - sRemainDataOfValue ),
                             sRemainDataOfValue );
                    sRemainDataOfValue = 0;
                }
                else
                {
                    CMI_RCP( aProtocolContext,
                             (acp_uint8_t *)aOutValue->value + ( aOutValue->length - sRemainDataOfValue ),
                             sRemainDataInCmBlock );
                    sRemainDataOfValue = sRemainDataOfValue - sRemainDataInCmBlock;
                }
                sRemainDataInCmBlock = CMI_REMAIN_DATA_IN_READ_BLOCK( aProtocolContext );
            }
        }
    }
    else
    {
        /* NULL value가 전송되는 경우에는 length가 0으로 넘어오게
         * 되므로, 이 경우에는 메모리 할당을 하지 않도록 한다. */
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION( ULA_ERR_MEM_ALLOC )
    {
        ulaSetErrorCode( aOutErrorMgr, ulaERR_ABORT_MEMORY_ALLOC );
    }
    ACI_EXCEPTION( ERR_CHECK_OPERATION_TYPE )
    {
        ulaSetErrorCode( aOutErrorMgr, ulaERR_ABORT_NET_UNEXPECTED_PROTOCOL,
                         "XLog Column Value" );
    }
    ACI_EXCEPTION_END;

    if ( aOutValue->value != NULL )
    {
        (void)aclMemFree( aAllocator, (void *)aOutValue->value );
        aOutValue->value = NULL;
    }

    return ACI_FAILURE;
}

ACI_RC ulaCommRecvStop( acp_bool_t         * aExitFlag,
                        cmiProtocolContext * aProtocolContext,
                        ulaXLog            * aOutXLog,
                        acp_uint32_t         aTimeoutSec,
                        ulaErrorMgr        * aOutErrorMgr )
{
    ACP_UNUSED( aExitFlag );
    ACP_UNUSED( aTimeoutSec );
    ACP_UNUSED( aOutErrorMgr );

    /* Get Arguemnt XLog Header */
    CMI_RD4( aProtocolContext, &(aOutXLog->mHeader.mType) );
    CMI_RD4( aProtocolContext, &(aOutXLog->mHeader.mTID) );
    CMI_RD8( aProtocolContext, &(aOutXLog->mHeader.mSN) );
    CMI_RD8( aProtocolContext, &(aOutXLog->mHeader.mSyncSN) );
    // BUG-17789
    CMI_RD8( aProtocolContext, &(aOutXLog->mHeader.mRestartSN) );

    return ACI_SUCCESS;
}

ACI_RC ulaCommRecvKeepAlive( acp_bool_t         * aExitFlag,
                             cmiProtocolContext * aProtocolContext,
                             ulaXLog            * aOutXLog,
                             acp_uint32_t         aTimeoutSec,
                             ulaErrorMgr        * aOutErrorMgr )
{
    ACP_UNUSED( aExitFlag );
    ACP_UNUSED( aTimeoutSec );
    ACP_UNUSED( aOutErrorMgr );

    /* Get Arguemnt XLog Header */
    CMI_RD4( aProtocolContext, &(aOutXLog->mHeader.mType) );
    CMI_RD4( aProtocolContext, &(aOutXLog->mHeader.mTID) );
    CMI_RD8( aProtocolContext, &(aOutXLog->mHeader.mSN) );
    CMI_RD8( aProtocolContext, &(aOutXLog->mHeader.mSyncSN) );
    // BUG-17789
    CMI_RD8( aProtocolContext, &(aOutXLog->mHeader.mRestartSN) );

    return ACI_SUCCESS;
}

ACI_RC ulaCommRecvLobCursorOpen( acp_bool_t         * aExitFlag,
                                 cmiProtocolContext * aProtocolContext,
                                 ulaXLog            * aOutXLog,
                                 acl_mem_alloc_t    * aAllocator,
                                 acp_uint32_t         aTimeoutSec,
                                 ulaErrorMgr        * aOutErrorMgr )
{
    acp_uint32_t i;
    acp_rc_t sRc;

    /* Get Arguemnt XLog Header */
    CMI_RD4( aProtocolContext, &(aOutXLog->mHeader.mType) );
    CMI_RD4( aProtocolContext, &(aOutXLog->mHeader.mTID) );
    CMI_RD8( aProtocolContext, &(aOutXLog->mHeader.mSN) );
    CMI_RD8( aProtocolContext, &(aOutXLog->mHeader.mSyncSN) );
    CMI_RD8( aProtocolContext, &(aOutXLog->mHeader.mTableOID) );
    /* Get Lob Information */
    CMI_RD8( aProtocolContext, &(aOutXLog->mLOB.mLobLocator) );
    CMI_RD4( aProtocolContext, &(aOutXLog->mLOB.mLobColumnID) );
    aOutXLog->mLOB.mLobColumnID = ( aOutXLog->mLOB.mLobColumnID & ULA_META_COLUMN_ID_MASK );
    /* Get Arguemnt PKColumn */
    CMI_RD4( aProtocolContext, &(aOutXLog->mPrimaryKey.mPKColCnt) );
    aOutXLog->mPrimaryKey.mPKColArray = NULL;

    sRc = aclMemAlloc( aAllocator,
                       (void **)&aOutXLog->mPrimaryKey.mPKColArray,
                       aOutXLog->mPrimaryKey.mPKColCnt * ACI_SIZEOF(ulaValue) );
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ULA_ERR_MEM_ALLOC);
    acpMemSet(aOutXLog->mPrimaryKey.mPKColArray,
              0x00,
              aOutXLog->mPrimaryKey.mPKColCnt * ACI_SIZEOF(ulaValue));

    /* Recv PK Value repeatedly */
    for ( i = 0; i < aOutXLog->mPrimaryKey.mPKColCnt; i++ )
    {
        ACI_TEST(ulaCommRecvValue(aExitFlag,
                                  aProtocolContext,
                                  &(aOutXLog->mPrimaryKey.mPKColArray[i]),
                                  aAllocator,
                                  aTimeoutSec,
                                  aOutErrorMgr)
                 != ACI_SUCCESS);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION( ULA_ERR_MEM_ALLOC )
    {
        ulaSetErrorCode( aOutErrorMgr, ulaERR_ABORT_MEMORY_ALLOC );
    }
    ACI_EXCEPTION_END;
    // 이미 ulaSetErrorCode() 수행

    if (aOutXLog->mPrimaryKey.mPKColArray != NULL)
    {
        (void)aclMemFree( aAllocator, aOutXLog->mPrimaryKey.mPKColArray );
        aOutXLog->mPrimaryKey.mPKColArray = NULL;
    }

    return ACI_FAILURE;
}

ACI_RC ulaCommRecvLobCursorClose( acp_bool_t         * aExitFlag,
                                  cmiProtocolContext * aProtocolContext,
                                  ulaXLog            * aOutXLog,
                                  acp_uint32_t         aTimeoutSec,
                                  ulaErrorMgr        * aOutErrorMgr )
{
    ACP_UNUSED( aExitFlag );
    ACP_UNUSED( aTimeoutSec );
    ACP_UNUSED( aOutErrorMgr );

    /* Get Arguemnt XLog Header */
    CMI_RD4( aProtocolContext, &(aOutXLog->mHeader.mType) );
    CMI_RD4( aProtocolContext, &(aOutXLog->mHeader.mTID) );
    CMI_RD8( aProtocolContext, &(aOutXLog->mHeader.mSN) );
    CMI_RD8( aProtocolContext, &(aOutXLog->mHeader.mSyncSN) );
    /* Get Lob Information */
    CMI_RD8( aProtocolContext, &(aOutXLog->mLOB.mLobLocator) );

    return ACI_SUCCESS;
}

ACI_RC ulaCommRecvLobPrepare4Write( acp_bool_t         * aExitFlag,
                                    cmiProtocolContext * aProtocolContext,
                                    ulaXLog            * aOutXLog,
                                    acp_uint32_t         aTimeoutSec,
                                    ulaErrorMgr        * aOutErrorMgr )
{
    ACP_UNUSED( aExitFlag );
    ACP_UNUSED( aTimeoutSec );
    ACP_UNUSED( aOutErrorMgr );

    /* Get Arguemnt XLog Header */
    CMI_RD4( aProtocolContext, &(aOutXLog->mHeader.mType) );
    CMI_RD4( aProtocolContext, &(aOutXLog->mHeader.mTID) );
    CMI_RD8( aProtocolContext, &(aOutXLog->mHeader.mSN) );
    CMI_RD8( aProtocolContext, &(aOutXLog->mHeader.mSyncSN) );

    /* Get Lob Information */
    CMI_RD8( aProtocolContext, &(aOutXLog->mLOB.mLobLocator) );
    CMI_RD4( aProtocolContext, &(aOutXLog->mLOB.mLobOffset) );
    CMI_RD4( aProtocolContext, &(aOutXLog->mLOB.mLobOldSize) );
    CMI_RD4( aProtocolContext, &(aOutXLog->mLOB.mLobNewSize) );

    return ACI_SUCCESS;
}

ACI_RC ulaCommRecvLobPartialWrite( acp_bool_t         * aExitFlag,
                                   cmiProtocolContext * aProtocolContext,
                                   ulaXLog            * aOutXLog,
                                   acl_mem_alloc_t    * aAllocator,
                                   acp_uint32_t         aTimeoutSec,
                                   ulaErrorMgr        * aOutErrorMgr )
{
    acp_rc_t sRc;

    ACP_UNUSED( aExitFlag );
    ACP_UNUSED( aTimeoutSec );

    /* Get Arguemnt XLog Header */
    CMI_RD4( aProtocolContext, &(aOutXLog->mHeader.mType) );
    CMI_RD4( aProtocolContext, &(aOutXLog->mHeader.mTID) );
    CMI_RD8( aProtocolContext, &(aOutXLog->mHeader.mSN) );
    CMI_RD8( aProtocolContext, &(aOutXLog->mHeader.mSyncSN) );

    /* Get Lob Information */
    CMI_RD8( aProtocolContext, &(aOutXLog->mLOB.mLobLocator) );
    CMI_RD4( aProtocolContext, &(aOutXLog->mLOB.mLobOffset) );
    CMI_RD4( aProtocolContext, &( aOutXLog->mLOB.mLobPieceLen) );
    aOutXLog->mLOB.mLobPiece    = NULL;

    sRc = aclMemAlloc( aAllocator,
                       (void **)&aOutXLog->mLOB.mLobPiece,
                       aOutXLog->mLOB.mLobPieceLen );
    ACI_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRc), ULA_ERR_MEM_ALLOC);

    CMI_RCP( aProtocolContext,
             (acp_uint8_t *)aOutXLog->mLOB.mLobPiece,
             aOutXLog->mLOB.mLobPieceLen );

    return ACI_SUCCESS;

    ACI_EXCEPTION(ULA_ERR_MEM_ALLOC)
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_ABORT_MEMORY_ALLOC);
    }
    ACI_EXCEPTION_END;

    if (aOutXLog->mLOB.mLobPiece != NULL)
    {
        (void)aclMemFree( aAllocator, aOutXLog->mLOB.mLobPiece );
        aOutXLog->mLOB.mLobPiece = NULL;
    }

    return ACI_FAILURE;
}

ACI_RC ulaCommRecvLobFinish2Write( acp_bool_t         * aExitFlag,
                                   cmiProtocolContext * aProtocolContext,
                                   ulaXLog            * aOutXLog,
                                   acp_uint32_t         aTimeoutSec,
                                   ulaErrorMgr        * aOutErrorMgr )
{
    ACP_UNUSED( aExitFlag );
    ACP_UNUSED( aProtocolContext );
    ACP_UNUSED( aTimeoutSec );
    ACP_UNUSED( aOutErrorMgr );

    /* Get Arguemnt XLog Header */
    CMI_RD4( aProtocolContext, &(aOutXLog->mHeader.mType) );
    CMI_RD4( aProtocolContext, &(aOutXLog->mHeader.mTID) );
    CMI_RD8( aProtocolContext, &(aOutXLog->mHeader.mSN) );
    CMI_RD8( aProtocolContext, &(aOutXLog->mHeader.mSyncSN) );

    /* Get Lob Information */
    CMI_RD8( aProtocolContext, &(aOutXLog->mLOB.mLobLocator) );

    return ACI_SUCCESS;
}

ACI_RC ulaCommRecvLobTrim( acp_bool_t         * aExitFlag,
                           cmiProtocolContext * aProtocolContext,
                           ulaXLog            * aOutXLog,
                           acp_uint32_t         aTimeoutSec,
                           ulaErrorMgr        * aOutErrorMgr )
{
    ACP_UNUSED( aExitFlag );
    ACP_UNUSED( aTimeoutSec );
    ACP_UNUSED( aOutErrorMgr );

    /* Get Arguemnt XLog Header */
    CMI_RD4( aProtocolContext, &(aOutXLog->mHeader.mType) );
    CMI_RD4( aProtocolContext, &(aOutXLog->mHeader.mTID) );
    CMI_RD8( aProtocolContext, &(aOutXLog->mHeader.mSN) );
    CMI_RD8( aProtocolContext, &(aOutXLog->mHeader.mSyncSN) );

    /* Get Lob Information */
    CMI_RD8( aProtocolContext, &(aOutXLog->mLOB.mLobLocator) );
    CMI_RD4( aProtocolContext, &(aOutXLog->mLOB.mLobOffset) );

    return ACI_SUCCESS;
}

ACI_RC ulaCommSendAck( cmiProtocolContext * aProtocolContext,
                       ulaXLogAck           aAck,
                       ulaErrorMgr        * aOutErrorMgr )
{
    acp_uint8_t  sOpID;
    acp_uint64_t sFlushSN = ULA_SN_NULL;  //proj-1608 ala에서는 사용하지 않음

    ACI_TEST( cmiCheckAndFlush( aProtocolContext, 1 + 52,
                                ACP_TRUE )
              != ACI_SUCCESS );

    /* Replication XLog Header Set */
    sOpID = CMI_PROTOCOL_OPERATION( RP, Ack );
    CMI_WR1( aProtocolContext, sOpID );
    CMI_WR4( aProtocolContext, &(aAck.mAckType) );
    CMI_WR4( aProtocolContext, &(aAck.mAbortTxCount) );
    CMI_WR4( aProtocolContext, &(aAck.mClearTxCount) );
    CMI_WR8( aProtocolContext, &(aAck.mRestartSN) );
    CMI_WR8( aProtocolContext, &(aAck.mLastCommitSN) );
    CMI_WR8( aProtocolContext, &(aAck.mLastArrivedSN) );
    CMI_WR8( aProtocolContext, &(aAck.mLastProcessedSN) );
    CMI_WR8( aProtocolContext, &(sFlushSN) );

    ACI_TEST_RAISE( cmiSend( aProtocolContext, ACP_TRUE )
                    != ACI_SUCCESS, ERR_SEND );

    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_SEND )
    {
        ulaSetErrorCode( aOutErrorMgr, ulaERR_ABORT_NET_FLUSH,
                         "ACK or Stop ACK", aciGetErrorCode() );
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulaCommReadCmBlock( cmiProtocolContext * aProtocolContext,
                           acp_bool_t         * aExitFlag,
                           acp_bool_t         * aIsTimeOut,
                           acp_uint32_t         aTimeOut,
                           ulaErrorMgr        * aOutErrorMgr )
{
    acp_uint32_t i;

    ACI_TEST( CMI_IS_READ_ALL( aProtocolContext ) == ACP_FALSE );

    for(i = 0; i < aTimeOut; i ++)
    {
        /* Read CM Block */
        if ( cmiRecv( aProtocolContext,
                      NULL,
                      gTV1Sec ) == ACI_SUCCESS )
        {
            break;
        }

        ACI_TEST_RAISE( aciGetErrorCode() != cmERR_ABORT_TIMED_OUT,
                        ERR_READ );
        ACI_CLEAR();

        if ( aExitFlag != NULL )
        {
            ACI_TEST_RAISE( *aExitFlag == ACP_TRUE, ERR_EXIT );
        }
    }

    if ( ( aIsTimeOut != NULL ) && ( i >= aTimeOut ) )
    {
        *aIsTimeOut = ACP_TRUE;
    }
    else
    {
        ACI_TEST_RAISE( i >= aTimeOut, ERR_TIMEOUT );
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_READ );
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_ABORT_NET_READ,
                        "read communication block", aciGetErrorCode());
    }
    ACI_EXCEPTION( ERR_EXIT );
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_ABORT_NET_EXIT,
                        "read communication block");
    }
    ACI_EXCEPTION( ERR_TIMEOUT );
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_ABORT_NET_TIMEOUT,
                        "read communication block");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulaCommRecvMetaPartitionCount( cmiProtocolContext * aProtocolContext,
                                      acp_bool_t         * aExitFlag,
                                      acp_uint32_t         aTimeoutSec,
                                      ulaErrorMgr        * aOutErrorMgr )
{
    acp_char_t      sOpCode;
    acp_uint32_t    sDummyCount = 0;

    ACI_TEST( ulaCommReadCmBlock( aProtocolContext,
                                  aExitFlag,
                                  NULL /* TimeoutFlag */,
                                  aTimeoutSec,
                                  aOutErrorMgr )
              != ACI_SUCCESS );

    /* Check Operation Type */
    CMI_RD1( aProtocolContext, sOpCode );
    ACI_TEST_RAISE( sOpCode != CMI_PROTOCOL_OPERATION( RP, MetaPartitionCount ),
                    ERR_CHECK_OPERATION_TYPE );

    /* Get Replication Information */
    CMI_RD4( aProtocolContext, &sDummyCount );

    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_CHECK_OPERATION_TYPE );
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_ABORT_NET_UNEXPECTED_PROTOCOL,
                        "Meta Partition Count");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulaCommSendMetaDictTableCount( cmiProtocolContext * aProtocolContext,
                                      ulaErrorMgr        * aOutErrorMgr )
{
    acp_char_t    sOpCode;
    acp_uint32_t  sDummyCount = 0;

    ACI_TEST( cmiCheckAndFlush( aProtocolContext, 1 + 4,
                                ACP_TRUE )
              != ACI_SUCCESS );

    /* Replication Information Set */
    sOpCode = CMI_PROTOCOL_OPERATION( RP, MetaDictTableCount );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, &sDummyCount );

    ACI_TEST_RAISE( cmiSend( aProtocolContext, ACP_TRUE )
                    != ACI_SUCCESS, ERR_SEND );

    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_SEND )
    {
        ulaSetErrorCode( aOutErrorMgr, ulaERR_ABORT_NET_FLUSH,
                         "SendMetaDictTableCount", aciGetErrorCode() );
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulaCommRecvMetaDictTableCount( cmiProtocolContext * aProtocolContext,
                                      acp_bool_t         * aExitFlag,
                                      acp_uint32_t         aTimeoutSec,
                                      ulaErrorMgr        * aOutErrorMgr )
{
    acp_char_t      sOpCode;
    acp_uint32_t    sDummyCount = 0;

    ACI_TEST( ulaCommReadCmBlock( aProtocolContext,
                                  aExitFlag,
                                  NULL /* TimeoutFlag */,
                                  aTimeoutSec,
                                  aOutErrorMgr )
              != ACI_SUCCESS );

    /* Check Operation Type */
    CMI_RD1( aProtocolContext, sOpCode );
    ACI_TEST_RAISE( sOpCode != CMI_PROTOCOL_OPERATION( RP, MetaDictTableCount ),
                    ERR_CHECK_OPERATION_TYPE );

    CMI_RD4( aProtocolContext, &sDummyCount );

    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_CHECK_OPERATION_TYPE );
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_ABORT_NET_UNEXPECTED_PROTOCOL,
                        "Meta Dictionary Table Count");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulaCommSendMetaInitFlag( cmiProtocolContext * aProtocolContext,
                                ulaErrorMgr        * aOutErrorMgr )
{
    acp_char_t    sOpCode;
    acp_uint32_t  sDummyFlag = 0;

    ACI_TEST( cmiCheckAndFlush( aProtocolContext, 1 + 4,
                                ACP_TRUE )
              != ACI_SUCCESS );

    /* Replication Information Set */
    sOpCode = CMI_PROTOCOL_OPERATION( RP, MetaInitialize );
    CMI_WR1( aProtocolContext, sOpCode );
    CMI_WR4( aProtocolContext, &sDummyFlag );

    ACI_TEST_RAISE( cmiSend( aProtocolContext, ACP_TRUE )
                    != ACI_SUCCESS, ERR_SEND );

    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_SEND )
    {
        ulaSetErrorCode( aOutErrorMgr, ulaERR_ABORT_NET_FLUSH,
                         "SendMetaDictTableCount", aciGetErrorCode() );
    }

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC ulaCommRecvMetaInitFlag( cmiProtocolContext * aProtocolContext,
                                acp_bool_t         * aExitFlag,
                                acp_uint32_t         aTimeoutSec,
                                ulaErrorMgr        * aOutErrorMgr )
{
    acp_char_t   sOpCode;
    acp_uint32_t sDummyFlag = 0;

    ACI_TEST( ulaCommReadCmBlock( aProtocolContext,
                                  aExitFlag,
                                  NULL /* TimeoutFlag */,
                                  aTimeoutSec,
                                  aOutErrorMgr )
              != ACI_SUCCESS );

    /* Check Operation Type */
    CMI_RD1( aProtocolContext, sOpCode );
    ACI_TEST_RAISE( sOpCode != CMI_PROTOCOL_OPERATION( RP, MetaInitialize ),
                    ERR_CHECK_OPERATION_TYPE );

    CMI_RD4( aProtocolContext, &sDummyFlag );

    return ACI_SUCCESS;

    ACI_EXCEPTION( ERR_CHECK_OPERATION_TYPE );
    {
        ulaSetErrorCode(aOutErrorMgr, ulaERR_ABORT_NET_UNEXPECTED_PROTOCOL,
                        "Meta Initialize Flag");
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}
