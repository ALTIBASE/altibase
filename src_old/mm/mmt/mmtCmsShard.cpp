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

#include <cm.h>
#include <mmtServiceThread.h>
#include <cm.h>
#include <qci.h>
#include <sdi.h>
#include <sduVersion.h>
#include <mmErrorCode.h>
#include <mmcSession.h>
#include <mmcStatement.h>
#include <mmcStatementManager.h>
#include <mmcTask.h>
#include <mmtServiceThread.h>
#include <mmtAuditManager.h>

IDE_RC mmtServiceThread::shardHandshakeProtocol( cmiProtocolContext * aCtx,
                                                 cmiProtocol        * /*aProtocol*/,
                                                 void               * /*aSessionOwner*/,
                                                 void               * aUserContext )
{
    UChar                    sMajorVersion;  /* Shard major ver of client */
    UChar                    sMinorVersion;  /* Shard minor ver of client */
    UChar                    sPatchVersion;  /* Shard patch ver of client */
    UChar                    sFlags;         /* reserved */

    mmtServiceThread*        sThread  = (mmtServiceThread *)aUserContext;

    IDE_CLEAR();

    CMI_RD1(aCtx, sMajorVersion);
    CMI_RD1(aCtx, sMinorVersion);
    CMI_RD1(aCtx, sPatchVersion);
    CMI_RD1(aCtx, sFlags);

    PDL_UNUSED_ARG(sFlags);

    IDU_FIT_POINT_RAISE( "mmtServiceThread::shardHandshakeProtocol::ShardMetaVersionMismatch",
                         ShardMetaVersionMismatch );

    if ( (sMajorVersion == SHARD_MAJOR_VERSION) &&
         (sMinorVersion == SHARD_MINOR_VERSION) )
    {
        CMI_WOP(aCtx, CMP_OP_DB_ShardHandshakeResult);
        CMI_WR1(aCtx, sMajorVersion);
        CMI_WR1(aCtx, sMinorVersion);
        CMI_WR1(aCtx, sPatchVersion);
        CMI_WR1(aCtx, sFlags);

        if ( cmiGetLinkImpl( aCtx ) == CMI_LINK_IMPL_IPCDA )
        {
            /* PROJ-2616 */
            MMT_IPCDA_INCREASE_DATA_COUNT( aCtx );
        }
        else
        {
            IDE_TEST( cmiSend( aCtx, ID_TRUE ) != IDE_SUCCESS );
        }

    }
    else
    {
        IDE_RAISE(ShardMetaVersionMismatch);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ShardMetaVersionMismatch);
    {
        IDE_SET(ideSetErrorCode(cmERR_ABORT_SHARD_VERSION_MISMATCH));
    }
    IDE_EXCEPTION_END;

    return sThread->answerErrorResult(aCtx,
                                      CMI_PROTOCOL_OPERATION(DB, ShardHandshake),
                                      0);
}

/* PROJ-2598 */
static IDE_RC answerShardNodeGetListResult( cmiProtocolContext * aProtocolContext,
                                            sdiNodeInfo        * aNodeInfo,
                                            mmcSession         * aSession )
{
    cmiWriteCheckState sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;
    UInt               sWriteSize = 1+2;
    UChar              sLen;
    UChar              sIsTestEnable = (UChar)0;
    UShort             i;
    sdiShardPin        sShardPin = SDI_SHARD_PIN_INVALID;
    ULong              sSMN      = SDI_NULL_SMN;

    if ( SDU_SHARD_TEST_ENABLE == 1 )
    {
        sIsTestEnable = (UChar)1;
    }
    else
    {
        // Nothing to do.
    }

    for ( i = 0; i < aNodeInfo->mCount; i++ )
    {
        sWriteSize += 4+1+16+2+16+2;
        sWriteSize += idlOS::strlen(aNodeInfo->mNodes[i].mNodeName);
    }

    sWriteSize += 1 + 8 + 8;

    sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
    CMI_WRITE_CHECK( aProtocolContext, sWriteSize );
    sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    CMI_WOP( aProtocolContext, CMP_OP_DB_ShardNodeGetListResult );

    CMI_WR1( aProtocolContext, sIsTestEnable );
    CMI_WR2( aProtocolContext, &(aNodeInfo->mCount) );
    for ( i = 0; i < aNodeInfo->mCount; i++ )
    {
        sLen = (UChar)idlOS::strlen(aNodeInfo->mNodes[i].mNodeName);

        CMI_WR4( aProtocolContext, &(aNodeInfo->mNodes[i].mNodeId) );
        CMI_WR1( aProtocolContext, sLen );
        CMI_WCP( aProtocolContext, &(aNodeInfo->mNodes[i].mNodeName), sLen );
        CMI_WCP( aProtocolContext, &(aNodeInfo->mNodes[i].mServerIP), 16 );
        CMI_WR2( aProtocolContext, &(aNodeInfo->mNodes[i].mPortNo) );
        CMI_WCP( aProtocolContext, &(aNodeInfo->mNodes[i].mAlternateServerIP), 16 );
        CMI_WR2( aProtocolContext, &(aNodeInfo->mNodes[i].mAlternatePortNo) );
    }

    sShardPin = aSession->getShardPIN();
    CMI_WR8( aProtocolContext, &sShardPin );

    sSMN = aSession->getShardMetaNumber();
    CMI_WR8( aProtocolContext, &sSMN );

    /* PROJ-2616 */
    MMT_IPCDA_INCREASE_DATA_COUNT(aProtocolContext);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* BUG-44124 ipcda 모드 사용 중 hang - iloader 컬럼이 많은 테이블 */
    if ( ( sWriteCheckState == CMI_WRITE_CHECK_ACTIVATED ) &&
         ( cmiGetLinkImpl( aProtocolContext ) == CMI_LINK_IMPL_IPCDA ) )
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE));
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
}

/* PROJ-2598 */
IDE_RC mmtServiceThread::shardNodeGetListProtocol( cmiProtocolContext *aProtocolContext,
                                                   cmiProtocol        * /* aProtocol */,
                                                   void               * aSessionOwner,
                                                   void               * aUserContext )
{
    qciShardNodeInfo   sNodeInfo;
    mmcTask          * sTask             = (mmcTask *)aSessionOwner;
    mmtServiceThread * sThread           = (mmtServiceThread *)aUserContext;
    mmcSession       * sSession          = NULL;
    ULong              sSMNForSession    = SDI_NULL_SMN;

    IDE_TEST_RAISE( sTask == NULL, NoTask );

    sSession          = sTask->getSession();
    sSMNForSession    = sSession->getShardMetaNumber();

    IDE_TEST( sdi::getExternalNodeInfo( &sNodeInfo,
                                        sSMNForSession )
              != IDE_SUCCESS );

    IDE_TEST( answerShardNodeGetListResult( aProtocolContext,
                                            &sNodeInfo,
                                            sSession )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( NoTask );
    {
        IDE_SET( ideSetErrorCode( mmERR_ABORT_SESSION_NOT_SPECIFIED ) );
    }
    IDE_EXCEPTION_END;

    return sThread->answerErrorResult( aProtocolContext,
                                       CMI_PROTOCOL_OPERATION(DB, ShardNodeGetList),
                                       0 );
}

/* PROJ-2622 Shard Retry Execution */
static IDE_RC answerShardNodeUpdateListResult( cmiProtocolContext * aProtocolContext,
                                               ULong                aSMN,
                                               sdiNodeInfo        * aNodeInfo )
{
    cmiWriteCheckState sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;
    UInt               sWriteSize = 1+2;
    UChar              sLen;
    UChar              sIsTestEnable = (UChar)0;
    UShort             i;

    if ( SDU_SHARD_TEST_ENABLE == 1 )
    {
        sIsTestEnable = (UChar)1;
    }
    else
    {
        // Nothing to do.
    }

    for ( i = 0; i < aNodeInfo->mCount; i++ )
    {
        sWriteSize += 4+1+16+2+16+2;
        sWriteSize += idlOS::strlen(aNodeInfo->mNodes[i].mNodeName);
    }

    sWriteSize += 1 + 8;

    sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
    CMI_WRITE_CHECK( aProtocolContext, sWriteSize );
    sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    CMI_WOP( aProtocolContext, CMP_OP_DB_ShardNodeUpdateListResult );

    CMI_WR1( aProtocolContext, sIsTestEnable );
    CMI_WR2( aProtocolContext, &(aNodeInfo->mCount) );
    for ( i = 0; i < aNodeInfo->mCount; i++ )
    {
        sLen = (UChar)idlOS::strlen(aNodeInfo->mNodes[i].mNodeName);

        CMI_WR4( aProtocolContext, &(aNodeInfo->mNodes[i].mNodeId) );
        CMI_WR1( aProtocolContext, sLen );
        CMI_WCP( aProtocolContext, &(aNodeInfo->mNodes[i].mNodeName), sLen );
        CMI_WCP( aProtocolContext, &(aNodeInfo->mNodes[i].mServerIP), 16 );
        CMI_WR2( aProtocolContext, &(aNodeInfo->mNodes[i].mPortNo) );
        CMI_WCP( aProtocolContext, &(aNodeInfo->mNodes[i].mAlternateServerIP), 16 );
        CMI_WR2( aProtocolContext, &(aNodeInfo->mNodes[i].mAlternatePortNo) );
    }

    CMI_WR8( aProtocolContext, &aSMN );

    /* PROJ-2616 */
    MMT_IPCDA_INCREASE_DATA_COUNT(aProtocolContext);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* BUG-44124 ipcda 모드 사용 중 hang - iloader 컬럼이 많은 테이블 */
    if ( ( sWriteCheckState == CMI_WRITE_CHECK_ACTIVATED ) &&
         ( cmiGetLinkImpl( aProtocolContext ) == CMI_LINK_IMPL_IPCDA ) )
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE));
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
}

/* PROJ-2622 Shard Retry Execution */
IDE_RC mmtServiceThread::shardNodeUpdateListProtocol( cmiProtocolContext *aProtocolContext,
                                                      cmiProtocol        * /* aProtocol */,
                                                      void               * aSessionOwner,
                                                      void               * aUserContext )
{
    qciShardNodeInfo   sNodeInfo;
    mmcTask          * sTask    = (mmcTask *)aSessionOwner;
    mmtServiceThread * sThread  = (mmtServiceThread *)aUserContext;
    mmcSession       * sSession = NULL;
    ULong              sSessSMN = SDI_NULL_SMN;
    ULong              sRecvSMN = SDI_NULL_SMN;
    ULong              sDataSMN = SDI_NULL_SMN;
    UInt               sSleep = 0;

    IDE_TEST_RAISE( sTask == NULL, NoTask );

    sSession          = sTask->getSession();
    sSessSMN          = sSession->getShardMetaNumber();
    sRecvSMN          = sSession->getReceivedShardMetaNumber();
    sDataSMN          = sdi::getSMNForDataNode();

    IDU_FIT_POINT( "mmtServiceThread::shardNodeUpdateListProtocol" );

    IDU_FIT_POINT_RAISE( "mmtServiceThread::shardNodeUpdateListProtocol::NO_SHARD_META_CHANGE_TO_REBUILD",
                         ERROR_NO_SHARD_META_CHANGE_TO_REBUILD );

    /* sDataSMN 값이
     * sRecvSMN(마지막으로 수신한 SHARD_META_NUMBER, REBUILD_SHARD_META_NUMBER) 값과
     * 비교하여 동일하거나 작은 경우
     * 1. Rebuild loop 가 발생 했거나
     * 2. Library session 에서 rebuild 를 감지했으나
     *    User session 이 접속한 노드에는 아직 SMN 이 변경되지 않았을수 있다.
     */
    while ( sDataSMN <= sRecvSMN )
    {
        IDE_TEST_RAISE( sSleep > SDU_SHARD_META_PROPAGATION_TIMEOUT,
                        ERROR_NO_SHARD_META_CHANGE_TO_REBUILD );

        acpSleepUsec(1000);
        sSleep += 1000;

        sDataSMN = sdi::getSMNForDataNode();
    }

    IDE_TEST( sdi::getExternalNodeInfo( &sNodeInfo,
                                        sDataSMN )
              != IDE_SUCCESS );

    IDE_TEST( answerShardNodeUpdateListResult( aProtocolContext,
                                               sDataSMN,
                                               &sNodeInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( NoTask );
    {
        IDE_SET( ideSetErrorCode( mmERR_ABORT_SESSION_NOT_SPECIFIED ) );
    }
    IDE_EXCEPTION( ERROR_NO_SHARD_META_CHANGE_TO_REBUILD )
    {
        SChar sMsgBuf[SMI_MAX_ERR_MSG_LEN];

        idlOS::snprintf( sMsgBuf,
                         SMI_MAX_ERR_MSG_LEN,
                         "SESSION-SMN:%"ID_UINT64_FMT" RECV-SMN:%"ID_UINT64_FMT" DATA-SMN:%"ID_UINT64_FMT"",
                         sSessSMN, sRecvSMN, sDataSMN );

        IDE_SET( ideSetErrorCode( mmERR_ABORT_NO_SHARD_META_CHANGE_TO_REBUILD,
                                  sMsgBuf ) );
    }

    IDE_EXCEPTION_END;

    return sThread->answerErrorResult( aProtocolContext,
                                       CMI_PROTOCOL_OPERATION(DB, ShardNodeUpdateList),
                                       0 );
}

/* PROJ-2598 Shard pilot (shard analyze) */
static void answerShardWriteMtValue( cmiProtocolContext * aProtocolContext,
                                     UInt                 aMtDataType,
                                     sdiValue           * aValue )
{
    if ( aMtDataType == MTD_SMALLINT_ID )
    {
        CMI_WR2( aProtocolContext, (UShort*)&(aValue->mSmallintMax) );
    }
    else if ( aMtDataType == MTD_INTEGER_ID )
    {
        CMI_WR4( aProtocolContext, (UInt*) &(aValue->mIntegerMax) );
    }
    else if ( aMtDataType == MTD_BIGINT_ID )
    {
        CMI_WR8( aProtocolContext, (ULong*) &(aValue->mBigintMax) );
    }
    else if ( ( aMtDataType == MTD_CHAR_ID ) ||
              ( aMtDataType == MTD_VARCHAR_ID ) )
    {
        CMI_WR2( aProtocolContext, &(aValue->mCharMax.length) );
        CMI_WCP( aProtocolContext,
                 aValue->mCharMax.value,
                 aValue->mCharMax.length );
    }
    else
    {
        IDE_DASSERT( 0 );
    }

    return;

    IDE_EXCEPTION_END;

    IDE_DASSERT( 0 );

    return;
}

static UInt getShardMtValueSize( UInt       aMtDataType,
                                 sdiValue * aValue )
{
    if ( aMtDataType == MTD_SMALLINT_ID )
    {
        return 2;
    }
    else if ( aMtDataType == MTD_INTEGER_ID )
    {
        return 4;
    }
    else if ( aMtDataType == MTD_BIGINT_ID )
    {
        return 8;
    }
    else if ( ( aMtDataType == MTD_CHAR_ID ) ||
              ( aMtDataType == MTD_VARCHAR_ID ) )
    {
        return 2 + aValue->mCharMax.length;
    }
    else
    {
        IDE_DASSERT( 0 );
    }

    return 0;
}

static IDE_RC answerShardWriteValue( cmiProtocolContext     * aProtocolContext,
                                     UInt                     aValueType,
                                     sdiValue               * aValue )
{
    UInt        sLength = 0;

    CMI_WRITE_CHECK( aProtocolContext, 4 +  // Value Type
                                       4 +  // Length 
                                       8 ); // Value  

    CMI_WR4( aProtocolContext, &(aValueType) );

    switch( aValueType )
    {
        case MTD_SMALLINT_ID:
            sLength = 2;
            CMI_WR4( aProtocolContext, &sLength );
            CMI_WR2( aProtocolContext, (UShort*)&(aValue->mSmallintMax) );
            break;
        case MTD_INTEGER_ID:
            sLength = 4;
            CMI_WR4( aProtocolContext, &sLength );
            CMI_WR4( aProtocolContext, (UInt*)&(aValue->mIntegerMax) );
            break;

        case MTD_BIGINT_ID:
            sLength = 8;
            CMI_WR4( aProtocolContext, &sLength );
            CMI_WR8( aProtocolContext, (ULong*)&(aValue->mBigintMax) );
            break;

        case MTD_CHAR_ID:
        case MTD_VARCHAR_ID:
            sLength = aValue->mCharMax.length;
            CMI_WR4( aProtocolContext, &sLength );
            IDE_TEST( cmiSplitWrite( aProtocolContext,
                                     sLength,
                                     aValue->mCharMax.value )
                      != IDE_SUCCESS );
            break;

        default:
            IDE_DASSERT(0);
            break;

    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



static IDE_RC answerShardWriteValueInfo( cmiProtocolContext     * aProtocolContext,
                                         UChar                    aSplitMethod,
                                         sdiValueInfo           * aValueInfo )
{
    sdiValue                * sValue = NULL;
    cmiWriteCheckState        sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    sValue = &(aValueInfo->mValue);

    sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
    CMI_WRITE_CHECK( aProtocolContext, 1 );  // type
    sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    CMI_WR1( aProtocolContext, aValueInfo->mType );

    switch( aValueInfo->mType )
    {
        case SDI_VALUE_INFO_HOST_VAR:
            IDE_TEST( answerShardWriteValue( aProtocolContext,
                                             aValueInfo->mDataValueType,
                                             sValue )
                      != IDE_SUCCESS );
            break;

        case SDI_VALUE_INFO_CONST_VAL:
            switch( aSplitMethod )
            {
                case SDI_SPLIT_HASH:
                case SDI_SPLIT_RANGE:
                case SDI_SPLIT_LIST:
                    IDE_TEST( answerShardWriteValue( aProtocolContext,
                                                     aValueInfo->mDataValueType,
                                                     sValue )
                              != IDE_SUCCESS );
                    break;

                case SDI_SPLIT_CLONE:
                case SDI_SPLIT_SOLO:
                    /* Nothing to do */
                    break;

                default:
                    IDE_DASSERT( 0 );
                    break;
            }
            break;

        default:
            IDE_DASSERT( 0 );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* BUG-44124 ipcda 모드 사용 중 hang - iloader 컬럼이 많은 테이블 */
    if ( ( sWriteCheckState == CMI_WRITE_CHECK_ACTIVATED ) &&
         ( cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA ) )
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE));
    }
    else
    {
        /* Nothing to do */
    }
    
    return IDE_FAILURE;
}

/* PROJ-2598 Shard pilot (shard analyze) */
static IDE_RC answerShardAnalyzeResult( cmiProtocolContext *aProtocolContext,
                                        mmcStatement       *aStatement )
{
    cmiWriteCheckState sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;
    qciStatement*      sQciStmt        = aStatement->getQciStmt();
    UInt               sStatementID    = aStatement->getStmtID();
    UInt               sStatementType  = aStatement->getStmtType();
    UShort             sParamCount     = qci::getParameterCount(sQciStmt);
    UShort             sResultSetCount = 1;

    /* PROJ-2598 Shard pilot(shard analyze) */
    qciShardAnalyzeInfo * sAnalyzeInfo;
    sdiRange            * sRange;
    UChar                 sSplitMethod;
    UChar                 sSubSplitMethod = 0;
    UShort                sRangeIdx  = 0;
    UShort                sValueCnt = 0;
    UShort                sSubValueCnt = 0;
    UInt                  sWriteSize = 0;

    UChar                 sSubKeyExists;
    UChar                 sIsShardQuery;

    /* TASK-7219 Non-shard DML */
    UChar                 sIsPartialExecNeeded = 0;

    IDE_TEST( qci::getShardAnalyzeInfo( sQciStmt, &sAnalyzeInfo )
              != IDE_SUCCESS );

    sSplitMethod = (UChar)(sAnalyzeInfo->mSplitMethod);

    if ( ( sAnalyzeInfo->mSubKeyExists == ID_TRUE ) &&
         ( sAnalyzeInfo->mIsShardQuery == ID_TRUE ) )
    {
        sSubKeyExists = 1;
        sSubSplitMethod = (UChar)(sAnalyzeInfo->mSubSplitMethod);
    }
    else
    {
        sSubKeyExists = 0;
    }

    if ( sAnalyzeInfo->mIsShardQuery == ID_TRUE )
    {
        sIsShardQuery = 1;
    }
    else
    {
        sIsShardQuery = 0;
    }

    /* TASK-7219 Non-shard DML */
    if ( sAnalyzeInfo->mTopQueryFlag[SDI_TQ_PARTIAL_COORD_EXEC_NEEDED] == ID_TRUE )
    {
        sIsPartialExecNeeded = 1;

        /*
         * Partial shard execution이 필요한 non-shard DML인 경우,
         * Client-side로 수행 시키기 위해
         * Shard query로 설정한다.
         */
        sIsShardQuery = 1;
    }
    else
    {
        /* Nothing to do. */
    }

    /************************************************
     * Get protocol static context size
     ************************************************/

    sWriteSize = 1 // CMP_OP_DB_ShardAnalyzeResult
               + 4 // statementID
               + 4 // statementType
               + 2 // paramCount
               + 2 // resultSetCount
               + 1 // splitMethod
               + 1 // isPartialExecNeeded
               + 4 // keyDataType
               + 4 // defaultNodeId
               + 1 // subKeyExists
               + 1 // isShardQuery
               + 2; // valueCount

    if ( sSubKeyExists == ID_TRUE )
    {
        sWriteSize += 1;    // subSplitMethod
        sWriteSize += 4;    // subKeyDataType
        sWriteSize += 2;    // subValueCount

    }

    /************************************************
     * Write protocol context
     ************************************************/

    sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
    CMI_WRITE_CHECK(aProtocolContext, sWriteSize);
    sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    CMI_WOP(aProtocolContext, CMP_OP_DB_ShardAnalyzeResult);
    CMI_WR4(aProtocolContext, &sStatementID);
    CMI_WR4(aProtocolContext, &sStatementType);
    CMI_WR2(aProtocolContext, &sParamCount);
    CMI_WR2(aProtocolContext, &sResultSetCount);

    /* PROJ-2598 Shard pilot(shard analyze) */
    CMI_WR1( aProtocolContext, sSplitMethod );

    /* TASK-7219 Non-shard DML */
    CMI_WR1( aProtocolContext, sIsPartialExecNeeded );
    
    CMI_WR4( aProtocolContext, &(sAnalyzeInfo->mKeyDataType) );
    CMI_WR1( aProtocolContext, sSubKeyExists );
    if ( sSubKeyExists == ID_TRUE )
    {
        CMI_WR1( aProtocolContext, sSubSplitMethod );
        CMI_WR4( aProtocolContext, &(sAnalyzeInfo->mSubKeyDataType) );
    }
    CMI_WR4( aProtocolContext, &(sAnalyzeInfo->mDefaultNodeId) );

    CMI_WR1( aProtocolContext, sIsShardQuery );

    CMI_WR2( aProtocolContext, &(sAnalyzeInfo->mValuePtrCount) );
    if ( sSubKeyExists == ID_TRUE )
    {
        CMI_WR2( aProtocolContext, &(sAnalyzeInfo->mSubValuePtrCount) );
    }

    for ( sValueCnt = 0; sValueCnt < sAnalyzeInfo->mValuePtrCount; sValueCnt++ )
    {
        IDE_TEST( answerShardWriteValueInfo( aProtocolContext,
                                             sSplitMethod,
                                             sAnalyzeInfo->mValuePtrArray[sValueCnt] )
                  != IDE_SUCCESS );
    }

    /* PROJ-2655 Composite shard key */
    if ( sSubKeyExists == 1 )
    {
        for ( sSubValueCnt = 0; sSubValueCnt < sAnalyzeInfo->mSubValuePtrCount; sSubValueCnt++ )
        {
            IDE_TEST( answerShardWriteValueInfo( aProtocolContext,
                                                 sSubSplitMethod,
                                                 sAnalyzeInfo->mSubValuePtrArray[sSubValueCnt] )
                      != IDE_SUCCESS );

        }
    }
    else
    {
        /* Nothing to do. */
    }

    /************************************************
     * Get protocol static context size
     ************************************************/
    sWriteSize = 2;     // rangeInfoCount

    for ( sRangeIdx = 0; sRangeIdx < sAnalyzeInfo->mRangeInfo.mCount; sRangeIdx++ )
    {
        sRange = &(sAnalyzeInfo->mRangeInfo.mRanges[sRangeIdx]);

        if ( sSplitMethod == 1 ) // hash
        {
            sWriteSize += 4;
        }
        else if ( (sSplitMethod == 2) || (sSplitMethod == 3) ) // range or list
        {
            sWriteSize += getShardMtValueSize( sAnalyzeInfo->mKeyDataType,
                                               &sRange->mValue );
        }
        else if ( (sSplitMethod == 4) || (sSplitMethod == 5) ) // clone or solo
        {
            /* Nothing to do. */
        }
        else
        {
            IDE_DASSERT( 0 );
        }

        if ( sSubKeyExists == 1 )
        {
            if ( sSubSplitMethod == 1 ) // hash
            {
                sWriteSize += 4;
            }
            else if ( (sSubSplitMethod == 2) || (sSubSplitMethod == 3) ) // range or list
            {
                sWriteSize += getShardMtValueSize( sAnalyzeInfo->mSubKeyDataType,
                                                   &sRange->mSubValue );
            }
            else if ( (sSubSplitMethod == 4) || (sSubSplitMethod == 5) ) // clone or solo
            {
                /* Nothing to do. */
            }
            else
            {
                IDE_DASSERT( 0 );
            }
        }
        else
        {
            /* Nothing to do. */
        }

        sWriteSize += 4;
    }

    /************************************************
     * Write protocol context
     ************************************************/

    sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
    CMI_WRITE_CHECK(aProtocolContext, sWriteSize);
    sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    CMI_WR2( aProtocolContext, &(sAnalyzeInfo->mRangeInfo.mCount) );

    for ( sRangeIdx = 0; sRangeIdx < sAnalyzeInfo->mRangeInfo.mCount; sRangeIdx++ )
    {
        sRange = &(sAnalyzeInfo->mRangeInfo.mRanges[sRangeIdx]);

        if ( sSplitMethod == 1 ) // hash
        {
            CMI_WR4( aProtocolContext, &(sRange->mValue.mHashMax) );
        }
        else if ( (sSplitMethod == 2) || (sSplitMethod == 3) ) // range or list
        {
            answerShardWriteMtValue( aProtocolContext,
                                     sAnalyzeInfo->mKeyDataType,
                                     &sRange->mValue );
        }
        else if ( (sSplitMethod == 4) || (sSplitMethod == 5) ) // clone or solo
        {
            /* Nothing to do. */
        }
        else
        {
            IDE_DASSERT( 0 );
        }

        if ( sSubKeyExists == 1 )
        {
            if ( sSubSplitMethod == 1 ) // hash
            {
                CMI_WR4( aProtocolContext, &(sRange->mSubValue.mHashMax) );
            }
            else if ( (sSubSplitMethod == 2) || (sSubSplitMethod == 3) ) // range or list
            {
                answerShardWriteMtValue( aProtocolContext,
                                         sAnalyzeInfo->mSubKeyDataType,
                                         &sRange->mSubValue );
            }
            else if ( (sSubSplitMethod == 4) || (sSubSplitMethod == 5) ) // clone or solo
            {
                /* Nothing to do. */
            }
            else
            {
                IDE_DASSERT( 0 );
            }
        }
        else
        {
            /* Nothing to do. */
        }

        CMI_WR4( aProtocolContext, &(sRange->mNodeId) );
    }

    /* PROJ-2616 */
    MMT_IPCDA_INCREASE_DATA_COUNT(aProtocolContext);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* BUG-44124 ipcda 모드 사용 중 hang - iloader 컬럼이 많은 테이블 */
    if ( ( sWriteCheckState == CMI_WRITE_CHECK_ACTIVATED ) &&
         ( cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA ) )
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE));
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
}

/* PROJ-2598 Shard pilot (shard analyze) */
IDE_RC mmtServiceThread::shardAnalyzeProtocol(cmiProtocolContext *aProtocolContext,
                                              cmiProtocol        *,
                                              void               *aSessionOwner,
                                              void               *aUserContext)
{
    mmcTask          *sTask   = (mmcTask *)aSessionOwner;
    mmtServiceThread *sThread = (mmtServiceThread *)aUserContext;
    mmcSession       *sSession;
    mmcStatement     *sStatement = NULL;
    SChar            *sQuery;
    IDE_RC            sRet;

    UInt              sStatementID;
    UInt              sStatementStringLen;
    UInt              sRowSize;

    CMI_RD4(aProtocolContext, &sStatementID);
    CMI_SKIP_READ_BLOCK(aProtocolContext, 1);
    CMI_RD4(aProtocolContext, &sStatementStringLen);

    sRowSize   = sStatementStringLen;

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    if (sStatementID == 0)
    {
        IDE_TEST(mmcStatementManager::allocStatement(&sStatement, sSession, NULL) != IDE_SUCCESS);

        sThread->setStatement(sStatement);

        /* PROJ-2177 User Interface - Cancel */
        sSession->getInfo()->mCurrStmtID = sStatement->getStmtID();
        IDU_SESSION_CLR_CANCELED(*sSession->getEventFlag());

        /* BUG-38472 Query timeout applies to one statement. */
        IDU_SESSION_CLR_TIMEOUT( *sSession->getEventFlag() );
    }
    else
    {
        IDE_TEST(findStatement(&sStatement,
                               sSession,
                               &sStatementID,
                               sThread) != IDE_SUCCESS);

        /* PROJ-2223 Altibase Auditing */
        mmtAuditManager::auditBySession( sStatement );

        IDE_TEST(sStatement->clearPlanTreeText() != IDE_SUCCESS);

        IDE_TEST_RAISE(sStatement->getStmtState() >= MMC_STMT_STATE_EXECUTED,
                       InvalidStatementState);


        IDE_TEST(qci::clearStatement(sStatement->getQciStmt(),
                                     sStatement->getSmiStmt(),
                                     QCI_STMT_STATE_INITIALIZED) != IDE_SUCCESS);

        sStatement->clearGCTxStmtInfo();
        sStatement->setStmtState(MMC_STMT_STATE_ALLOC);
    }

    IDE_TEST_RAISE(sStatementStringLen == 0, NullQuery);
    IDU_FIT_POINT( "mmtServiceThread::shardAnalyzeProtocol::malloc::Query" );

    IDE_TEST(iduMemMgr::malloc(IDU_MEM_MMC,
                               sStatementStringLen + 1,
                               (void **)&sQuery,
                               IDU_MEM_IMMEDIATE) != IDE_SUCCESS);

    if (cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA)
    {
        IDE_TEST_RAISE( cmiSplitReadIPCDA( aProtocolContext,
                                           sRowSize,
                                           (UChar**)&sQuery,
                                           (UChar*)sQuery)
                        != IDE_SUCCESS, cm_error );
    }
    else
    {
        IDE_TEST_RAISE( cmiSplitRead( aProtocolContext,
                                      sRowSize,
                                      (UChar*)sQuery,
                                      NULL )
                        != IDE_SUCCESS, cm_error );
    }
    sRowSize = 0;

    sQuery[sStatementStringLen] = 0;

    IDE_TEST(sStatement->shardAnalyze(sQuery, sStatementStringLen) != IDE_SUCCESS);

    IDE_TEST(answerShardAnalyzeResult(aProtocolContext, sStatement) != IDE_SUCCESS);

    if ( sSession->isNeedRebuildNoti() == ID_TRUE )
    {
        IDE_TEST( sendShardRebuildNoti( aProtocolContext )
                  != IDE_SUCCESS );
    }

    /* 더이상 사용되지 않으므로 즉시 해제한다. */
    IDE_TEST(qci::clearStatement(sStatement->getQciStmt(),
                                 sStatement->getSmiStmt(),
                                 QCI_STMT_STATE_INITIALIZED)
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(InvalidStatementState);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_STATEMENT_STATE_ERROR));
    }
    IDE_EXCEPTION(NullQuery);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INSUFFICIENT_QUERY_ERROR));
    }
    IDE_EXCEPTION(cm_error);
    {
        return IDE_FAILURE;
    }
    IDE_EXCEPTION_END;

    if( sRowSize != 0 )
    {
        IDE_TEST_RAISE( cmiSplitSkipRead( aProtocolContext,
                                          sRowSize,
                                          NULL )
                        != IDE_SUCCESS, cm_error );
    }

    sRet = sThread->answerErrorResult(aProtocolContext,
                                      CMI_PROTOCOL_OPERATION(DB, ShardAnalyze),
                                      0);

    if (sRet == IDE_SUCCESS)
    {
        sThread->mErrorFlag = ID_TRUE;
    }

    //fix BUG-18284.
    // do exactly same as CMP_DB_FREE_DROP
    if (sStatementID == 0)
    {
        if(sStatement != NULL)
        {
            sThread->setStatement(NULL);
            IDE_ASSERT( sStatement->closeCursor(ID_TRUE) == IDE_SUCCESS );
            IDE_ASSERT( mmcStatementManager::freeStatement(sStatement) == IDE_SUCCESS );
        }
    }

    return sRet;
}

static IDE_RC answerShardTransactionResult( cmiProtocolContext *aProtocolContext, mmcSession *aSession )
{
    cmiWriteCheckState sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;
    smSCN sSCN;

    SM_INIT_SCN(&sSCN);
    
    /* PROJ-2733-DistTxInfo */
    if ( aSession->isGCTx() == ID_TRUE )
    {
        aSession->getLastSystemSCN( aProtocolContext->mProtocol.mOpID, &sSCN );
    }

    #if defined(DEBUG)
    ideLog::log(IDE_SD_18, "= [%s] answerShardTransactionResult, SCN : %"ID_UINT64_FMT,
                aSession->getSessionTypeString(),
                sSCN);
    #endif

    /* PROJ-2733-Protocol */
    switch (aProtocolContext->mProtocol.mOpID)
    {
        case CMP_OP_DB_ShardTransactionV3:
            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK(aProtocolContext, 9);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP(aProtocolContext, CMP_OP_DB_ShardTransactionV3Result);
            CMI_WR8(aProtocolContext, &sSCN);
            break;

        case CMP_OP_DB_ShardTransaction:
            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK(aProtocolContext, 1);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP(aProtocolContext, CMP_OP_DB_ShardTransactionResult);
            break;

        default:
            IDE_DASSERT(0);
            break;
    }

    /* PROJ-2616 */
    MMT_IPCDA_INCREASE_DATA_COUNT(aProtocolContext);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* BUG-44124 ipcda 모드 사용 중 hang - iloader 컬럼이 많은 테이블 */
    if ( ( sWriteCheckState == CMI_WRITE_CHECK_ACTIVATED ) &&
         ( cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA ) )
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE));
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
}

IDE_RC mmtServiceThread::shardTransactionProtocol(cmiProtocolContext *aProtocolContext,
                                                  cmiProtocol        *aProtocol,
                                                  void               *aSessionOwner,
                                                  void               *aUserContext)
{
    mmcTask             *sTask = (mmcTask *)aSessionOwner;
    mmtServiceThread    *sThread = (mmtServiceThread *)aUserContext;
    mmcSession          *sSession;

    UChar                sOperation;
    UInt                 sTouchNodeArr[SDI_NODE_MAX_COUNT];
    UShort               sTouchNodeCount;
    UShort               i;

    switch (aProtocol->mOpID)
    {
        case CMP_OP_DB_ShardTransactionV3:
        case CMP_OP_DB_ShardTransaction:
            /* trans op */
            CMI_RD1(aProtocolContext, sOperation);

            /* touch node array */
            CMI_RD2(aProtocolContext, &sTouchNodeCount);
            IDE_ASSERT(sTouchNodeCount <= SDI_NODE_MAX_COUNT);
            for (i = 0; i < sTouchNodeCount; i++)
            {
                CMI_RD4(aProtocolContext, &(sTouchNodeArr[i]));
            }
            break;

        default:
            IDE_DASSERT(0);
            break;
    }

    IDE_CLEAR();

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    /* BUG-20832 */
    IDE_TEST_RAISE(sSession->getXaAssocState() != MMD_XA_ASSOC_STATE_NOTASSOCIATED,
                   DCLNotAllowedError);

    for (i = 0; i < sTouchNodeCount; i++)
    {
        IDE_TEST( sSession->touchShardNode(sTouchNodeArr[i]) != IDE_SUCCESS );
    }

    switch (sOperation)
    {
        case CMP_DB_TRANSACTION_COMMIT:
            // BUG-47773
            IDE_TEST( sSession->setShardSessionProperty() != IDE_SUCCESS );
            
            IDE_TEST(sSession->commit() != IDE_SUCCESS);
            break;

        case CMP_DB_TRANSACTION_ROLLBACK:
            IDE_TEST(sSession->rollback() != IDE_SUCCESS);
            break;

        default:
            IDE_RAISE(InvalidOperation);
            break;
    }

    IDE_TEST( answerShardTransactionResult(aProtocolContext, sSession) != IDE_SUCCESS )

    if ( sSession->isNeedRebuildNoti() == ID_TRUE )
    {
        IDE_TEST( sendShardRebuildNoti( aProtocolContext )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(DCLNotAllowedError);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_NOT_ALLOWED_DCL));
    }
    IDE_EXCEPTION(InvalidOperation);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_ERROR));
    }
    IDE_EXCEPTION_END;

    return sThread->answerErrorResult(aProtocolContext,
                                      aProtocol->mOpID,  /* PROJ-2733-Protocol */
                                      0,
                                      sSession);
}

static IDE_RC answerShardPrepareResult( cmiProtocolContext *aProtocolContext,
                                        idBool              aReadOnly,
                                        mmcSession         *aSession,
                                        smSCN              *aPrepareSCN )
{
    cmiWriteCheckState sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;
    UChar              sReadOnly;
    smSCN              sPrepareSCN;

    SM_INIT_SCN( &sPrepareSCN );

    if ( aReadOnly == ID_TRUE )
    {
        sReadOnly = (UChar)1;
    }
    else
    {
        sReadOnly = (UChar)0;
    }

    /* PROJ-2733-DistTxInfo */
    if ( aSession->isGCTx() == ID_TRUE )
    {
        SM_SET_SCN( &sPrepareSCN, aPrepareSCN );
        if ( aReadOnly == ID_TRUE )
        {
            IDE_DASSERT( SM_SCN_IS_INIT( sPrepareSCN ) );
            SM_INIT_SCN( &sPrepareSCN );
        }
    }

    #if defined(DEBUG)
    ideLog::log( IDE_SD_18, "= [%s] answerShardPrepareResult, PrepareSCN : %"ID_UINT64_FMT,
                 aSession->getSessionTypeString(),
                 sPrepareSCN );
    #endif

    /* PROJ-2733-Protocol */
    switch (aProtocolContext->mProtocol.mOpID)
    {
        case CMP_OP_DB_ShardPrepareV3:
            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK(aProtocolContext, 10);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP(aProtocolContext, CMP_OP_DB_ShardPrepareV3Result);  /* PROJ-2733-Protocol */
            CMI_WR1(aProtocolContext, sReadOnly);
            CMI_WR8(aProtocolContext, &sPrepareSCN);
            break;

        case CMP_OP_DB_ShardPrepare:
            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK(aProtocolContext, 2);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP(aProtocolContext, CMP_OP_DB_ShardPrepareResult);
            CMI_WR1(aProtocolContext, sReadOnly);
            break;

        default:
            IDE_DASSERT(0);
            break;
    }

    /* PROJ-2616 */
    MMT_IPCDA_INCREASE_DATA_COUNT(aProtocolContext);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* BUG-44124 ipcda 모드 사용 중 hang - iloader 컬럼이 많은 테이블 */
    if ( ( sWriteCheckState == CMI_WRITE_CHECK_ACTIVATED) &&
         ( cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA ) )
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE));
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
}

IDE_RC mmtServiceThread::shardPrepareProtocol(cmiProtocolContext *aProtocolContext,
                                              cmiProtocol        *aProtocol,
                                              void               *aSessionOwner,
                                              void               *aUserContext)
{
    mmcTask             *sTask = (mmcTask *)aSessionOwner;
    mmtServiceThread    *sThread = (mmtServiceThread *)aUserContext;
    mmcSession          *sSession;
    smSCN                sPrepareSCN;

    UInt                 sXIDSize;
    ID_XID               sXID;
    idBool               sReadOnly;

    SM_INIT_SCN( &sPrepareSCN );

    switch (aProtocol->mOpID)
    {
        case CMP_OP_DB_ShardPrepareV3:
        case CMP_OP_DB_ShardPrepare:
            /* xid */
            CMI_RD4(aProtocolContext, &sXIDSize);
            if ( sXIDSize == ID_SIZEOF(ID_XID) )
            {
                CMI_RCP(aProtocolContext, &sXID, ID_SIZEOF(ID_XID));
            }
            else
            {
                /* size가 잘못되었다. 일단 읽고 에러 */
                CMI_SKIP_READ_BLOCK(aProtocolContext, (UShort)sXIDSize);
            }
            break;

        default:
            IDE_DASSERT(0);
            break;
    }

    IDE_CLEAR();

    IDE_TEST_RAISE( sXIDSize != ID_SIZEOF(ID_XID), ERR_INVALID_XID );

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    /* BUG-20832 */
    IDE_TEST_RAISE(sSession->getXaAssocState() != MMD_XA_ASSOC_STATE_NOTASSOCIATED,
                   DCLNotAllowedError);

    IDE_TEST(sSession->prepareForShard(&sXID, &sReadOnly, &sPrepareSCN) != IDE_SUCCESS);

    SM_SET_SCN( &sSession->getInfo()->mGCTxCommitInfo.mPrepareSCN, &sPrepareSCN );

    IDE_TEST( answerShardPrepareResult(aProtocolContext,
                                       sReadOnly,
                                       sSession,
                                       &sPrepareSCN )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_XID);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_XID));
    }
    IDE_EXCEPTION(DCLNotAllowedError);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_NOT_ALLOWED_DCL));
    }
    IDE_EXCEPTION_END;

    return sThread->answerErrorResult(aProtocolContext,
                                      aProtocol->mOpID,  /* PROJ-2733-Protocol */
                                      0);
}

static IDE_RC answerShardEndPendingTxResult( cmiProtocolContext *aProtocolContext,
                                             mmcSession         *aSession )
{
    smSCN sSCN;

    SM_INIT_SCN(&sSCN);

    if ( aSession->isGCTx() == ID_TRUE )
    {
        aSession->getLastSystemSCN( aProtocolContext->mProtocol.mOpID, &sSCN );
    }

    #if defined(DEBUG)
    ideLog::log(IDE_SD_18, "= [%s] answerShardEndPendingTxResult, SCN : %"ID_UINT64_FMT,
                aSession->getSessionTypeString(),
                sSCN);
    #endif

    cmiWriteCheckState sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    /* PROJ-2733-Protocol */
    switch (aProtocolContext->mProtocol.mOpID)
    {
        case CMP_OP_DB_ShardEndPendingTxV3:
            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK(aProtocolContext, 9);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP(aProtocolContext, CMP_OP_DB_ShardEndPendingTxV3Result);
            CMI_WR8(aProtocolContext, &sSCN);
            break;

        case CMP_OP_DB_ShardEndPendingTx:
            sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
            CMI_WRITE_CHECK(aProtocolContext, 1);
            sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

            CMI_WOP(aProtocolContext, CMP_OP_DB_ShardEndPendingTxResult);
            break;

        default:
            IDE_DASSERT(0);
            break;
    }

    /* PROJ-2616 */
    MMT_IPCDA_INCREASE_DATA_COUNT(aProtocolContext);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* BUG-44124 ipcda 모드 사용 중 hang - iloader 컬럼이 많은 테이블 */
    if( ( sWriteCheckState == CMI_WRITE_CHECK_ACTIVATED ) &&
        ( cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA ) )
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE));
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
}

/* BUG-46785 Shard statement partial rollback */
static IDE_RC answerShardStmtPartialRollbackResult( cmiProtocolContext  * aProtocolContext )
{
    cmiWriteCheckState sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
    CMI_WRITE_CHECK( aProtocolContext, 1 );
    sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;

    CMI_WOP( aProtocolContext, CMP_OP_DB_ShardStmtPartialRollbackResult );

    /* PROJ-2616 */
    MMT_IPCDA_INCREASE_DATA_COUNT( aProtocolContext );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* BUG-44124 ipcda 모드 사용 중 hang - iloader 컬럼이 많은 테이블 */
    if( ( sWriteCheckState == CMI_WRITE_CHECK_ACTIVATED ) &&
        ( cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA ) )
    {
        IDE_SET( ideSetErrorCode( mmERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE ) );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
}

IDE_RC mmtServiceThread::shardEndPendingTxProtocol(cmiProtocolContext *aProtocolContext,
                                                   cmiProtocol        *aProtocol,
                                                   void               *aSessionOwner,
                                                   void               *aUserContext)
{
    mmcTask             *sTask = (mmcTask *)aSessionOwner;
    mmtServiceThread    *sThread = (mmtServiceThread *)aUserContext;
    mmcSession          *sSession;

    UInt                 sXIDSize;
    ID_XID               sXID;
    UChar                sOperation;
    smSCN                sCommitSCN;

    SM_INIT_SCN( &sCommitSCN );

    /* xid */
    CMI_RD4(aProtocolContext, &sXIDSize);
    if ( sXIDSize == ID_SIZEOF(ID_XID) )
    {
        CMI_RCP(aProtocolContext, &sXID, ID_SIZEOF(ID_XID));
    }
    else
    {
        /* size가 잘못되었다. 일단 읽고 에러 */
        CMI_SKIP_READ_BLOCK(aProtocolContext, (UShort)sXIDSize);
    }

    /* trans op */
    CMI_RD1(aProtocolContext, sOperation);

    /* PROJ-2733-Protocol */
    switch (aProtocol->mOpID)
    {
        case CMP_OP_DB_ShardEndPendingTxV3:
            CMI_RD8(aProtocolContext, &sCommitSCN);
            break;

        case CMP_OP_DB_ShardEndPendingTx:
            break;

        default:
            IDE_DASSERT(0);
            break;
    }

    IDE_CLEAR();

    IDE_TEST_RAISE( sXIDSize != ID_SIZEOF(ID_XID), ERR_INVALID_XID );

    IDE_TEST(findSession(sTask, &sSession, sThread) != IDE_SUCCESS);

    IDE_TEST(checkSessionState(sSession, MMC_SESSION_STATE_SERVICE) != IDE_SUCCESS);

    /* BUG-20832 */
    IDE_TEST_RAISE(sSession->getXaAssocState() != MMD_XA_ASSOC_STATE_NOTASSOCIATED,
                   DCLNotAllowedError);

    ideLog::log(IDE_SD_18, "= [%s] mmtServiceThread::shardEndPendingTxProtocol, CommitSCN : %"ID_UINT64_FMT,
                sSession->getSessionTypeString(),
                sCommitSCN);

    /* PROJ-2733-DistTxInfo Rollback은 NULL을 보내자. */
    switch (sOperation)
    {
        case CMP_DB_TRANSACTION_COMMIT:
            IDE_TEST(sSession->endPendingTrans(&sXID, ID_TRUE, &sCommitSCN) != IDE_SUCCESS);

            SM_SET_SCN( &sSession->getInfo()->mGCTxCommitInfo.mGlobalCommitSCN, &sCommitSCN );
            break;

        case CMP_DB_TRANSACTION_ROLLBACK:
            IDE_TEST(sSession->endPendingTrans(&sXID, ID_FALSE, NULL) != IDE_SUCCESS);
            break;

        default:
            IDE_RAISE(InvalidOperation);
            break;
    }

    return answerShardEndPendingTxResult(aProtocolContext, sSession);

    IDE_EXCEPTION(ERR_INVALID_XID);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_XID));
    }
    IDE_EXCEPTION(DCLNotAllowedError);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_NOT_ALLOWED_DCL));
    }
    IDE_EXCEPTION(InvalidOperation);
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_INVALID_ERROR));
    }
    IDE_EXCEPTION_END;

    return sThread->answerErrorResult(aProtocolContext,
                                      aProtocol->mOpID,  /* PROJ-2733-Protocol */
                                      0,
                                      sSession);
}

/* BUG-46785 Shard statement partial rollback */
IDE_RC mmtServiceThread::shardStmtPartialRollback( cmiProtocolContext *aProtocolContext,
                                                   cmiProtocol        *,
                                                   void               *aSessionOwner,
                                                   void               *aUserContext )
{
    mmcTask             *sTask = (mmcTask *)aSessionOwner;
    mmtServiceThread    *sThread = (mmtServiceThread *)aUserContext;
    mmcSession          *sSession;

    IDE_TEST( findSession( sTask, &sSession, sThread ) != IDE_SUCCESS );

    IDE_TEST( checkSessionState( sSession, MMC_SESSION_STATE_SERVICE ) != IDE_SUCCESS) ;

    IDE_TEST( sSession->shardStmtPartialRollback() != IDE_SUCCESS );

    return answerShardStmtPartialRollbackResult( aProtocolContext );

    IDE_EXCEPTION_END;

    return sThread->answerErrorResult( aProtocolContext,
                                       CMI_PROTOCOL_OPERATION( DB, ShardStmtPartialRollback ),
                                       0 );
}

IDE_RC mmtServiceThread::sendShardRebuildNoti( cmiProtocolContext *aProtocolContext )
{
    cmiWriteCheckState sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;
    UInt               sWriteSize       = 0;
    ULong              sSMNForSession   = sdi::getSMNForDataNode();

    /* PROJ-2733-Protocol */
    switch (aProtocolContext->mProtocol.mOpID)
    {
        case CMP_OP_DB_PrepareV3:
        case CMP_OP_DB_PrepareByCIDV3:
        case CMP_OP_DB_ShardAnalyze:
        case CMP_OP_DB_Transaction:
        case CMP_OP_DB_ShardTransactionV3:
            /* continue */
            break;

        case CMP_OP_DB_ExecuteV3:
        case CMP_OP_DB_ParamDataInListV3:
            /* continue */
            break;

        case CMP_OP_DB_Prepare:
        case CMP_OP_DB_PrepareByCID:
        case CMP_OP_DB_ExecuteV2:
        case CMP_OP_DB_ParamDataInListV2:
        case CMP_OP_DB_Execute:
        case CMP_OP_DB_ParamDataInList:
        case CMP_OP_DB_ShardTransaction:
            /* Not support version */
            IDE_CONT( EndOfFunction );
            break;

        default:
            IDE_DASSERT(0);
            break;
    }

    sWriteSize += 1;    /* OP CODE */
    sWriteSize += 8;    /* SMN */

    sWriteCheckState = CMI_WRITE_CHECK_ACTIVATED;
    CMI_WRITE_CHECK( aProtocolContext, sWriteSize );
    sWriteCheckState = CMI_WRITE_CHECK_DEACTIVATED;
        
    CMI_WOP( aProtocolContext, CMP_OP_DB_ShardRebuildNotiV3 );
    CMI_WR8( aProtocolContext, &sSMNForSession );

    /* PROJ-2616 */
    MMT_IPCDA_INCREASE_DATA_COUNT(aProtocolContext);

    IDE_EXCEPTION_CONT( EndOfFunction );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* BUG-44124 ipcda 모드 사용 중 hang - iloader 컬럼이 많은 테이블 */
    if ( ( sWriteCheckState == CMI_WRITE_CHECK_ACTIVATED) &&
         ( cmiGetLinkImpl(aProtocolContext) == CMI_LINK_IMPL_IPCDA ) )
    {
        IDE_SET(ideSetErrorCode(mmERR_ABORT_IPCDA_MESSAGE_TOO_LONG, CMB_BLOCK_DEFAULT_SIZE));
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
}

