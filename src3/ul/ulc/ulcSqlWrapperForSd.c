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

#include <uln.h>
#include <ulnPrivate.h>
#include <ulsdnDbc.h>
#include <ulsdnStmt.h>
#include <ulsdnExecute.h>
#include <ulsdnDescribeCol.h>
#include <ulsdnTrans.h>
#include <ulnConfigFile.h>
#include <ulsdnFailover.h>
#include <ulsdnLob.h>
#include <ulsdnDistTxInfo.h>

#include <sqlcli.h>

#ifndef SQL_API
#define SQL_API
#endif

/*
 * =============================
 * Env Handle
 * =============================
 */
SQLHENV SQLGetEnvHandle(SQLHDBC ConnectionHandle)
{
    ulnDbc *sDbc = NULL;

    ULN_TRACE( SQLGetEnvHandle );

    ACI_TEST( ConnectionHandle == NULL );

    sDbc = (ulnDbc *)ConnectionHandle;

    return (SQLHENV)sDbc->mParentEnv;

    ACI_EXCEPTION_END;

    return NULL;
}

SQLRETURN SQL_API SQLGetParameterCount(SQLHSTMT      aStatementHandle,
                                       SQLUSMALLINT *aParamCount )
{
    acp_uint16_t sParamCount = 0;

    ULN_TRACE( SQLGetParameterCount );

    ACI_TEST( ( aStatementHandle == NULL ) ||
              ( aParamCount      == NULL ) );

    sParamCount = ulnStmtGetParamCount( (ulnStmt*)aStatementHandle );

    *aParamCount = sParamCount;

    return SQL_SUCCESS;

    ACI_EXCEPTION_END;

    return SQL_ERROR;
}

SQLRETURN  SQL_API SQLExecuteForMtDataRows(SQLHSTMT      aStatementHandle,
                                           SQLCHAR      *aOutBuffer,
                                           SQLUINTEGER   aOutBufLength,
                                           SQLPOINTER   *aOffSets,
                                           SQLPOINTER   *aMaxBytes,
                                           SQLUSMALLINT  aColumnCount)
{
    ULN_TRACE( SQLExecuteForMtDataRows );
    ACI_TEST( (aOutBuffer    == NULL) ||
              (aOutBufLength <= 0   ) ||
              (aMaxBytes     == NULL) ||
              (aOffSets      == NULL) );
    return ulsdExecuteForMtDataRows( (ulnStmt     *) aStatementHandle,
                                     (acp_char_t  *) aOutBuffer,
                                     (acp_uint32_t ) aOutBufLength,
                                     (acp_uint32_t*) aOffSets,
                                     (acp_uint32_t*) aMaxBytes,
                                     (acp_uint16_t ) aColumnCount );
    ACI_EXCEPTION_END;
    return SQL_ERROR;
}
SQLRETURN  SQL_API SQLExecDirectAddCallback( SQLUINTEGER   aIndex,
                                             SQLHSTMT      aStatementHandle,
                                             SQLCHAR      *aStatementText,
                                             SQLINTEGER    aTextLength,
                                             SQLPOINTER  **aCallback )
{
    return ulsdExecDirectAddCallback( (acp_uint32_t )      aIndex,
                                      (ulnStmt *)          aStatementHandle,
                                      (acp_char_t *)        aStatementText,
                                      (acp_sint32_t)        aTextLength,
                                      (ulsdFuncCallback**)  aCallback );
}
SQLRETURN  SQL_API SQLPrepareAddCallback( SQLUINTEGER   aIndex,
                                          SQLHSTMT      aStatementHandle,
                                          SQLCHAR      *aStatementText,
                                          SQLINTEGER    aTextLength,
                                          SQLPOINTER  **aCallback )
{
    return ulsdPrepareAddCallback( (acp_uint32_t )      aIndex,
                                   (ulnStmt *)          aStatementHandle,
                                   (acp_char_t *)       aStatementText,
                                   (acp_sint32_t)       aTextLength,
                                   (ulsdFuncCallback**) aCallback );
}

SQLRETURN  SQL_API SQLExecuteForMtDataRowsAddCallback( SQLUINTEGER   aIndex,
                                                       SQLHSTMT      aStatementHandle,
                                                       SQLCHAR      *aOutBuffer,
                                                       SQLUINTEGER   aOutBufLength,
                                                       SQLPOINTER   *aOffSets,
                                                       SQLPOINTER   *aMaxBytes,
                                                       SQLUSMALLINT  aColumnCount,
                                                       SQLPOINTER  **aCallback )
{
    return ulsdExecuteForMtDataRowsAddCallback( (acp_uint32_t )      aIndex,
                                                (ulnStmt     *)      aStatementHandle,
                                                (acp_char_t  *)      aOutBuffer,
                                                (acp_uint32_t )      aOutBufLength,
                                                (acp_uint32_t*)      aOffSets,
                                                (acp_uint32_t*)      aMaxBytes,
                                                (acp_uint16_t )      aColumnCount,
                                                (ulsdFuncCallback**) aCallback );
}

SQLRETURN  SQL_API SQLExecuteForMtDataAddCallback( SQLUINTEGER   aIndex,
                                                   SQLHSTMT      aStatementHandle,
                                                   SQLPOINTER  **aCallback )
{
    return ulsdExecuteForMtDataAddCallback( (acp_uint32_t )      aIndex,
                                            (ulnStmt     *)      aStatementHandle,
                                            (ulsdFuncCallback**) aCallback );
}

SQLRETURN  SQL_API SQLPrepareTranAddCallback( SQLUINTEGER   aIndex,
                                              SQLHDBC       aConnectionHandle,
                                              SQLUINTEGER   aXIDSize,
                                              SQLPOINTER   *aXID,
                                              SQLCHAR      *aReadOnly,
                                              SQLPOINTER  **aCallback )
{
    return ulsdPrepareTranAddCallback( (acp_uint32_t)       aIndex,
                                       (ulnDbc*)            aConnectionHandle,
                                       (acp_uint32_t)       aXIDSize,
                                       (acp_uint8_t*)       aXID,
                                       (acp_uint8_t*)       aReadOnly,
                                       (ulsdFuncCallback**) aCallback );
}

SQLRETURN  SQL_API SQLEndPendingTranAddCallback( SQLUINTEGER   aIndex,
                                                 SQLHDBC       aConnectionHandle,
                                                 SQLUINTEGER   aXIDSize,
                                                 SQLPOINTER   *aXID,
                                                 SQLSMALLINT   aCompletionType,
                                                 SQLPOINTER  **aCallback )
{
    return ulsdEndPendingTranAddCallback( (acp_uint32_t)       aIndex,
                                          (ulnDbc*)            aConnectionHandle,
                                          (acp_uint32_t)       aXIDSize,
                                          (acp_uint8_t*)       aXID,
                                          (acp_sint16_t)       aCompletionType,
                                          (ulsdFuncCallback**) aCallback );
}

SQLRETURN  SQL_API SQLEndTranAddCallback( SQLUINTEGER   aIndex,
                                          SQLHDBC       aConnectionHandle,
                                          SQLSMALLINT   aCompletionType,
                                          SQLPOINTER  **aCallback)
{
    return ulsdEndTranAddCallback( (acp_uint32_t)       aIndex,
                                   (ulnDbc*)            aConnectionHandle,
                                   (acp_sint16_t)       aCompletionType,
                                   (ulsdFuncCallback**) aCallback );
}

SQLRETURN SQL_API SQLSetSavepoint( SQLHDBC          aConnectionHandle,
                                   const SQLCHAR  * aSavepointName,
                                   SQLINTEGER       aSavepointNameLength )
{
    return ulnSetSavepoint( (ulnDbc*)           aConnectionHandle,
                            (const acp_char_t*) aSavepointName,
                            (acp_uint32_t)      aSavepointNameLength );
}

SQLRETURN SQL_API SQLRollbackToSavepoint( SQLHDBC             aConnectionHandle,
                                          const SQLCHAR     * aSavepointName,
                                          const SQLINTEGER    aSavepointNameLength )
{
    return ulnRollbackToSavepoint( (ulnDbc*)           aConnectionHandle,
                                   (const acp_char_t*) aSavepointName,
                                   (acp_uint32_t)      aSavepointNameLength );
}

SQLRETURN SQL_API SQLShardStmtPartialRollback( SQLHDBC          aConnectionHandle )
{
    return ulsdnStmtShardStmtPartialRollback( aConnectionHandle );
}

void SQL_API SQLDoCallback( SQLPOINTER *aCallback )
{
    ulsdDoCallback( (ulsdFuncCallback*)aCallback );
}

SQLRETURN SQL_API SQLGetResultCallback( SQLUINTEGER  aIndex,
                                        SQLPOINTER  *aCallback,
                                        SQLCHAR      aReCall )
{
    return ulsdGetResultCallback( (acp_uint32_t)      aIndex,
                                  (ulsdFuncCallback*) aCallback,
                                  (acp_uint8_t)       aReCall );
}

void SQL_API SQLRemoveCallback( SQLPOINTER *aCallback )
{
    ulsdRemoveCallback( (ulsdFuncCallback*)aCallback );
}

void SQL_API SQLGetDbcShardTargetDataNodeName(SQLHDBC     aConnectionHandle,
                                              SQLCHAR    *aOutBuff,
                                              SQLINTEGER  aOutBufLength)
{
    acpSnprintf( (acp_char_t *)aOutBuff,
                 aOutBufLength,
                 "%s",
                 ulsdDbcGetShardTargetDataNodeName( (ulnDbc*)aConnectionHandle ) );
}

void SQL_API SQLGetStmtShardTargetDataNodeName(SQLHSTMT    aStatementHandle,
                                               SQLCHAR    *aOutBuff,
                                               SQLINTEGER  aOutBufLength)
{
    acpSnprintf( (acp_char_t *)aOutBuff,
                 aOutBufLength,
                 "%s",
                 ulsdStmtGetShardTargetDataNodeName( (ulnStmt*)aStatementHandle) );
}

void SQL_API SQLSetDbcShardTargetDataNodeName(SQLHDBC  aConnectionHandle,
                                              SQLCHAR *aNodeName)
{
    ulsdDbcSetShardTargetDataNodeName( (ulnDbc*)aConnectionHandle, aNodeName );
}

void SQL_API SQLSetStmtShardTargetDataNodeName(SQLHSTMT  aStatementHandle,
                                               SQLCHAR  *aNodeName)
{
    ulsdStmtSetShardTargetDataNodeName( (ulnStmt*)aStatementHandle, aNodeName );
}

void SQL_API SQLGetDbcLinkInfo(SQLHDBC     aConnectionHandle,
                               SQLCHAR    *aOutBuff,
                               SQLINTEGER  aOutBufLength,
                               SQLINTEGER  aKey)
{
    ulsdDbcGetLinkInfo( (ulnDbc      *)aConnectionHandle,
                        (acp_char_t  *)aOutBuff,
                        (acp_uint32_t )aOutBufLength,
                        (acp_sint32_t )aKey );
}

SQLRETURN SQL_API SQLDescribeColEx(SQLHSTMT      StatementHandle,
                                   SQLUSMALLINT  ColumnNumber,
                                   SQLCHAR      *ColumnName,
                                   SQLINTEGER    ColumnNameSize,
                                   SQLUINTEGER  *NameLength,
                                   SQLUINTEGER  *DataMTType,
                                   SQLINTEGER   *Precision,
                                   SQLSMALLINT  *Scale,
                                   SQLSMALLINT  *Nullable)
{
    ULN_TRACE( SQLDescribeCol );

    return ulsdDescribeCol( (ulnStmt      *)StatementHandle,
                            (acp_uint16_t  )ColumnNumber,
                            (acp_char_t   *)ColumnName,
                            (acp_sint32_t  )ColumnNameSize,
                            (acp_uint32_t *)NameLength,
                            (acp_uint32_t *)DataMTType,
                            (acp_sint32_t *)Precision,
                            (acp_sint16_t *)Scale,
                            (acp_sint16_t *)Nullable );
}

SQLRETURN  SQL_API SQLEndPendingTran(SQLHDBC      aConnectionHandle,
                                     SQLUINTEGER  aXIDSize,
                                     SQLPOINTER  *aXID,
                                     SQLSMALLINT  aCompletionType)
{
    return ulsdShardEndPendingTran( (ulnDbc *)     aConnectionHandle,
                                    (acp_uint32_t) aXIDSize,
                                    (acp_uint8_t*) aXID,
                                    (acp_sint16_t) aCompletionType );
}

SQLRETURN SQL_API SQLDisconnectLocal(SQLHDBC ConnectionHandle)
{
    /* 서버가 사망한 경우 disconnect protocol은 항상 실패하게 되므로
     * 서버와 상관없이 disconnect를 수행할 필요가 있다.
     */
    return ulnDisconnectLocal((ulnDbc *)ConnectionHandle);
}

void SQL_API SQLSetShardPin(SQLHDBC aConnectionHandle, ULONG aShardPin)
{
    ulnDbcSetShardPin((ulnDbc*)aConnectionHandle, aShardPin);
}

void SQL_API SQLSetShardMetaNumber( SQLHDBC aConnectionHandle,
                                    ULONG   aShardMetaNumber )
{
    ulnDbcSetShardMetaNumber( (ulnDbc *)aConnectionHandle, aShardMetaNumber );
}

SQLRETURN SQL_API SQLReconnect( SQLSMALLINT  HandleType,
                                SQLHANDLE    InputHandle )
{
    ULN_TRACE( SQLReconnect );

    return ulsdnReconnect( HandleType, InputHandle );
}

SQLRETURN SQL_API SQLGetNeedFailover( SQLSMALLINT  HandleType,
                                      SQLHANDLE    InputHandle,
                                      SQLINTEGER  *IsNeed)
{
    ULN_TRACE( SQLGetNeedFailover );

    return ulsdnGetFailoverIsNeeded( HandleType, InputHandle, IsNeed );
}


void SQL_API SQLSetFailoverSuspend( SQLHDBC    aConnectionHandle,
                                    SQLINTEGER aSuspendOnOff )
{
    if ( aSuspendOnOff == 1 )
    {
        ulnDbcSetFailoverSuspendState( aConnectionHandle, ULN_FAILOVER_SUSPEND_ON_STATE );
    }
    else
    {
        ulnDbcSetFailoverSuspendState( aConnectionHandle, ULN_FAILOVER_SUSPEND_OFF_STATE );
    }
}

/* PROJ-2728 Sharding LOB */
SQLRETURN SQL_API SQLGetLobLengthForSd( SQLSMALLINT aHandleType,
                                        SQLHANDLE   aHandle,
                                        SQLUBIGINT    aLocator,
                                        SQLSMALLINT   aLocatorCType,
                                        SQLUINTEGER  *aLobLengthPtr,
                                        SQLUSMALLINT *aIsNull)
{
    return ulnGetLobLength((acp_sint16_t  )aHandleType,
                           (ulnObject    *)aHandle,
                           (acp_uint64_t  )aLocator,
                           (acp_sint16_t  )aLocatorCType,
                           (acp_uint32_t *)aLobLengthPtr,
                           (acp_uint16_t *)aIsNull);
}

SQLRETURN SQL_API SQLPutLobForSd( SQLSMALLINT aHandleType,
                                  SQLHANDLE   aHandle,
                                  SQLSMALLINT aLocatorCType,
                                  SQLUBIGINT  aLocator,
                                  SQLUINTEGER aStartOffset,
                                  SQLUINTEGER aSizeToBeUpdated,
                                  SQLSMALLINT aSourceCType,
                                  SQLPOINTER  aDataToPut,
                                  SQLUINTEGER aSizeDataToPut )
{
    return ulnPutLob((acp_sint16_t)aHandleType,
                     (ulnObject  *)aHandle,
                     (acp_sint16_t)aLocatorCType,
                     (acp_uint64_t)aLocator,
                     (acp_uint32_t)aStartOffset,
                     (acp_uint32_t)aSizeToBeUpdated,
                     (acp_sint16_t)aSourceCType,
                     (void       *)aDataToPut,
                     (acp_uint32_t)aSizeDataToPut);
}

SQLRETURN SQL_API SQLGetLobForSd( SQLSMALLINT  aHandleType,
                                  SQLHANDLE    aHandle,
                                  SQLSMALLINT  aLocatorCType,
                                  SQLUBIGINT   aLocator,
                                  SQLUINTEGER  aStartOffset,
                                  SQLUINTEGER  aSizeToGet,
                                  SQLSMALLINT  aTargetCType,
                                  SQLPOINTER   aBufferToStoreData,
                                  SQLUINTEGER  aSizeBuffer,
                                  SQLUINTEGER *aSizeReadPtr)
{
    return ulnGetLob((acp_sint16_t  )aHandleType,
                     (ulnObject    *)aHandle,
                     (acp_sint16_t  )aLocatorCType,
                     (acp_uint64_t  )aLocator,
                     (acp_uint32_t  )aStartOffset,
                     (acp_uint32_t  )aSizeToGet,
                     (acp_sint16_t  )aTargetCType,
                     (void         *)aBufferToStoreData,
                     (acp_uint32_t  )aSizeBuffer,
                     (acp_uint32_t *)aSizeReadPtr);
}

SQLRETURN SQL_API SQLFreeLobForSd( SQLSMALLINT  aHandleType,
                                   SQLHSTMT     aHandle,
                                   SQLUBIGINT   aLocator )
{
    return ulnFreeLob((acp_sint16_t)aHandleType,
                      (ulnObject  *)aHandle,
                      aLocator);
}

SQLRETURN SQL_API SQLTrimLobForSd( SQLSMALLINT  aHandleType,
                                   SQLHSTMT     aHandle,
                                   SQLSMALLINT  aLocatorCType,
                                   SQLUBIGINT   aLocator,
                                   SQLUINTEGER  aStartOffset)
{
    return ulnTrimLob((acp_sint16_t)aHandleType,
                      (ulnObject  *)aHandle,
                      (acp_sint16_t)aLocatorCType,
                      (acp_uint64_t)aLocator,
                      (acp_uint32_t)aStartOffset);
}

SQLRETURN SQL_API SQLLobPrepare4Write( SQLSMALLINT aHandleType,
                                       SQLHANDLE   aHandle,
                                       SQLSMALLINT aLocatorCType,
                                       SQLUBIGINT  aLocator,
                                       SQLUINTEGER aSize,
                                       SQLUINTEGER aStartOffset )
{
    return ulsdnLobPrepare4Write( (acp_sint16_t)aHandleType,
                                  (ulnObject  *)aHandle,
                                  (acp_sint16_t)aLocatorCType,
                                  (acp_uint64_t)aLocator,
                                  (acp_uint32_t)aSize,
                                  (acp_uint32_t)aStartOffset );
}

SQLRETURN SQL_API SQLLobWrite( SQLSMALLINT aHandleType,
                               SQLHANDLE   aHandle,
                               SQLSMALLINT aLocatorCType,
                               SQLUBIGINT  aLocator,
                               SQLSMALLINT aSourceCType,
                               SQLPOINTER  aDataToPut,
                               SQLUINTEGER aSizeDataToPut )
{
    return ulsdnLobWrite( (acp_sint16_t)aHandleType,
                          (ulnObject  *)aHandle,
                          (acp_sint16_t)aLocatorCType,
                          (acp_uint64_t)aLocator,
                          (acp_sint16_t)aSourceCType,
                          (void       *)aDataToPut,
                          (acp_uint32_t)aSizeDataToPut );
}

SQLRETURN SQL_API SQLLobFinishWrite( SQLSMALLINT  aHandleType,
                                     SQLHANDLE    aHandle,
                                     SQLSMALLINT  aLocatorCType,
                                     SQLUBIGINT   aLocator )
{
    return ulsdnLobFinishWrite( (acp_sint16_t)aHandleType,
                                (ulnObject  *)aHandle,
                                (acp_sint16_t)aLocatorCType,
                                (acp_uint64_t)aLocator );
}

/* PROJ-2733-DistTxInfo */
SQLRETURN SQL_API SQLGetScn( SQLHDBC     ConnectionHandle,
                             SQLUBIGINT *Scn )
{
    ULN_TRACE( SQLGetScn );

    return ulsdnGetSCN( (ulnDbc *)ConnectionHandle, (acp_uint64_t *)Scn );
}

SQLRETURN SQL_API SQLSetScn( SQLHDBC     ConnectionHandle,
                             SQLUBIGINT *Scn )
{
    ULN_TRACE( SQLSetScn );

    return ulsdnSetSCN( (ulnDbc *)ConnectionHandle, (acp_uint64_t *)Scn );
}

SQLRETURN SQL_API SQLSetTxFirstStmtScn( SQLHDBC     ConnectionHandle,
                                        SQLUBIGINT *TxFirstStmtScn )
{
    ULN_TRACE( SQLSetTxFirstStmtScn );

    return ulsdnSetTxFirstStmtSCN( (ulnDbc *)ConnectionHandle, (acp_uint64_t *)TxFirstStmtScn );
}

SQLRETURN SQL_API SQLSetTxFirstStmtTime( SQLHDBC   ConnectionHandle,
                                         SQLBIGINT TxFirstStmtTime )
{
    ULN_TRACE( SQLSetTxFirstStmtTime );

    return ulsdnSetTxFirstStmtTime( (ulnDbc *)ConnectionHandle, (acp_sint64_t)TxFirstStmtTime );
}

SQLRETURN SQL_API SQLSetDistLevel( SQLHDBC      ConnectionHandle,
                                   SQLUSMALLINT DistLevel )
{
    ULN_TRACE( SQLSetDistLevel );

    return ulsdnSetDistLevel( (ulnDbc *)ConnectionHandle, (ulsdDistLevel)DistLevel );
}

void SQL_API SQLSetTargetShardMetaNumber( SQLHDBC aConnectionHandle,
                                          ULONG   aShardMetaNumber )
{
    ulnDbcSetTargetShardMetaNumber( (ulnDbc *)aConnectionHandle, aShardMetaNumber );
}

/* TASK-7219 Non-shard DML */
SQLRETURN SQL_API SQLSetPartialExecType( SQLHSTMT   aStmt,
                                         SQLINTEGER aPartialExecType )
{
    ULN_TRACE(SQLSetPartialExecType);
    return ulsdnStmtSetPartialExecType( (ulnStmt *)aStmt,
                                        (acp_sint32_t)aPartialExecType );
}

/* TASK-7219 Non-shard DML */
void SQL_API SQLSetStmtExecSeq( SQLHDBC      aConnectionHandle,
                                SQLUINTEGER  aExecSequence )
{
    ULN_TRACE(SQLSetStmtExecSeq);
    ulnDbcSetExecSeqForShardTx( (ulnDbc *)aConnectionHandle,
                                (acp_uint32_t)aExecSequence );
}
