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

#ifndef _O_ULSD_LOB_H_
#define _O_ULSD_LOB_H_ 1

#include <uln.h>
#include <ulnPrivate.h>

/*
 * PROJ-2739 Client-side Sharding LOB
 */
#define ULSD_IS_SHARD_LIB_SESSION(aDbc) \
    ( ((aDbc)->mShardDbcCxt.mShardSessionType == ULSD_SESSION_TYPE_LIB) ? ACP_TRUE : ACP_FALSE )

SQLRETURN ulsdNodeBindFileToCol( ulnStmt      * aStmt,
                                 acp_sint16_t   aColumnNumber,
                                 acp_char_t   * aFileNameArray,
                                 ulvSLen      * aFileNameLengthArray,
                                 acp_uint32_t * aFileOptionsArray,
                                 ulvSLen        aMaxFileNameLength,
                                 ulvSLen      * aLenOrIndPtr );

SQLRETURN ulsdNodeBindFileToParam( ulnStmt       * aStmt,
                                   acp_uint16_t    aParamNumber,
                                   acp_sint16_t    aSqlType,
                                   acp_char_t    * aFileNameArray,
                                   ulvSLen       * aFileNameLengthArray,
                                   acp_uint32_t  * aFileOptionPtr,
                                   ulvSLen         aMaxFileNameLength,
                                   ulvSLen       * aIndicator );

SQLRETURN ulsdGetLobLength( ulnStmt      * aStmt,
                            acp_uint64_t   aLocator,
                            acp_sint16_t   aLocatorType,
                            acp_uint32_t * aLengthPtr );

SQLRETURN ulsdGetLob( ulnStmt      * aStmt,
                      acp_sint16_t   aLocatorCType,
                      acp_uint64_t   aSrcLocator,
                      acp_uint32_t   aFromPosition,
                      acp_uint32_t   aForLength,
                      acp_sint16_t   aTargetCType,
                      void         * aBuffer,
                      acp_uint32_t   aBufferSize,
                      acp_uint32_t * aLengthWritten );

SQLRETURN ulsdPutLob( ulnStmt      * aStmt,
                      acp_sint16_t   aLocatorCType,
                      acp_uint64_t   aLocator,
                      acp_uint32_t   aFromPosition,
                      acp_uint32_t   aForLength,
                      acp_sint16_t   aSourceCType,
                      void         * aBuffer,
                      acp_uint32_t   aBufferSize);

SQLRETURN ulsdFreeLob( ulnStmt      * aStmt,
                       acp_uint64_t   aLocator);

SQLRETURN ulsdTrimLob( ulnStmt       * aStmt,
                       acp_sint16_t    aLocatorCType,
                       acp_uint64_t    aLocator,
                       acp_uint32_t    aStartOffset );

SQLRETURN ulsdGetLobLengthNodes( ulnFnContext * aFnContext,
                                 ulnStmt      * aStmt,
                                 acp_uint64_t   aLocator,
                                 acp_sint16_t   aLocatorType,
                                 acp_uint32_t * aLengthPtr );

SQLRETURN ulsdGetLobNodes( ulnFnContext * aFnContext,
                           ulnStmt      * aStmt,
                           acp_sint16_t   aLocatorCType,
                           acp_uint64_t   aSrcLocator,
                           acp_uint32_t   aFromPosition,
                           acp_uint32_t   aForLength,
                           acp_sint16_t   aTargetCType,
                           void         * aBuffer,
                           acp_uint32_t   aBufferSize,
                           acp_uint32_t * aLengthWritten );

SQLRETURN ulsdPutLobNodes( ulnFnContext * aFnContext,
                           ulnStmt      * aStmt,
                           acp_sint16_t   aLocatorCType,
                           acp_uint64_t   aLocator,
                           acp_uint32_t   aFromPosition,
                           acp_uint32_t   aForLength,
                           acp_sint16_t   aSourceCType,
                           void         * aBuffer,
                           acp_uint32_t   aBufferSize );

SQLRETURN ulsdFreeLobNodes( ulnFnContext * aFnContext,
                            ulnStmt      * aStmt,
                            acp_uint64_t   aLocator );

SQLRETURN ulsdTrimLobNodes( ulnFnContext * aFnContext,
                            ulnStmt      * aStmt,
                            acp_sint16_t   aLocatorCType,
                            acp_uint64_t   aLocator,
                            acp_uint32_t   aStartOffset );

SQLRETURN ulsdLobCopy( ulnFnContext     * aFnContext,
                       ulsdDbc          * aShard,
                       ulnStmt          * aStmt );

ACI_RC ulsdLobLocatorFreeAll( ulnDbc * aMetaDbc );

ACI_RC ulsdLobLocatorDestroy( ulnDbc         * aMetaDbc,
                              ulsdLobLocator * aLobLocator,
                              acp_bool_t       aNeedLock );

ACI_RC ulsdLobLocatorCreate( ulnDbc          * aMetaDbc,
                             acp_uint16_t      aCount,
                             ulsdLobLocator ** aOutputLocator );

ACI_RC ulsdLobAddFetchLocator( ulnStmt      * aStmt,
                               acp_uint64_t * aUserBufferPtr );

ACI_RC ulsdLobAddOutBindLocator( ulnStmt           * aStmt,
                                 acp_uint16_t        aParamNumber,
                                 ulnParamInOutType   aParamInOutType,
                                 ulsdLobLocator    * aInBindLocator,
                                 acp_uint64_t      * aUserBufferPtr );

ACI_RC ulsdLobBindInfoBuild4Param(ulnFnContext      *aFnContext,
                                  void              *aUserBufferPtr,
                                  acp_bool_t        *aChangeInOutType);

ACP_INLINE ulnStmt * ulsdLobGetExecuteStmt( ulnStmt        * aStmt,
                                            ulsdLobLocator * aShardLocator )
{
    ulnStmt       *sNodeStmt = NULL;
    acp_sint16_t   sNodeDbcIndex;

    sNodeDbcIndex = aShardLocator->mNodeDbcIndex;

    if ( sNodeDbcIndex >= 0 )
    {
        sNodeStmt = aStmt->mShardStmtCxt.mShardNodeStmt[sNodeDbcIndex];
    }
    else
    {
        sNodeStmt = aStmt;
    }

    return sNodeStmt;
}

ACP_INLINE acp_bool_t ulsdTypeIsLocatorCType( ulnCTypeID aCTYPE )
{
    acp_bool_t sIsLobType = ACP_FALSE;

    if ( aCTYPE == ULN_CTYPE_BLOB_LOCATOR ||
         aCTYPE == ULN_CTYPE_CLOB_LOCATOR )
    {
        sIsLobType = ACP_TRUE;
    }
    return sIsLobType;
}

ACP_INLINE acp_bool_t ulsdTypeIsLocInBoundLob( ulnCTypeID         aCTYPE,
                                               ulnParamInOutType  aInOutType )
{
    if (aCTYPE == ULN_CTYPE_BLOB_LOCATOR || aCTYPE == ULN_CTYPE_CLOB_LOCATOR)
    {
        if (aInOutType == ULN_PARAM_INOUT_TYPE_INPUT)
        {
            return ACP_TRUE;
        }
    }
    return ACP_FALSE;
}

ACP_INLINE acp_bool_t ulsdParamIsLocInBoundLob( acp_sint16_t aValueType,
                                                acp_sint16_t aInOutType )
{
    if (aValueType == SQL_C_BLOB_LOCATOR || aValueType == SQL_C_CLOB_LOCATOR)
    {
        if (aInOutType == SQL_PARAM_INPUT)
        {
            return ACP_TRUE;
        }
    }
    return ACP_FALSE;
}

ACP_INLINE void ulsdDescRecInitShardLobLocator(ulnStmt           *aStmt,
                                    acp_uint32_t       aParamNumber,
                                    ulnParamInOutType  aInOutType,
                                    void              *aUserBufferPtr)
{
    ulnStmt        *sMetaStmt = NULL;

    if ( ULSD_IS_SHARD_LIB_SESSION(aStmt->mParentDbc) == ACP_TRUE )
    {
        sMetaStmt = aStmt->mShardStmtCxt.mParentStmt;
    }
    else // COORD(Server-side) stmt.
    {
        sMetaStmt = aStmt;
    }

    ulnDescRecSetShardLobLocator( ulnStmtGetIpdRec(sMetaStmt, aParamNumber),
                                  NULL );

    if ( aInOutType == ULN_PARAM_INOUT_TYPE_OUTPUT )
    {
        /* 노드로부터의 OUT이 없을 경우를 대비하여 사용자 버퍼를 0으로 초기화 */
        *(acp_uint64_t *)aUserBufferPtr = 0;
    }
}

ACP_INLINE acp_bool_t ulsdLobHasLocatorInBoundParam(ulnStmt *aStmt)
{
    ulnStmt        *sMetaStmt = NULL;

    if ( ULSD_IS_SHARD_LIB_SESSION(aStmt->mParentDbc) == ACP_TRUE )
    {
        sMetaStmt = aStmt->mShardStmtCxt.mParentStmt;
    }
    else // COORD(Server-side) stmt.
    {
        sMetaStmt = aStmt;
    }

    return sMetaStmt->mShardStmtCxt.mHasLocatorInBoundParam;
}

ACP_INLINE void ulsdLobResetLocatorInBoundParam(ulnStmt *aStmt)
{
    ulnStmt        *sMetaStmt = NULL;

    if ( ULSD_IS_SHARD_LIB_SESSION(aStmt->mParentDbc) == ACP_TRUE )
    {
        sMetaStmt = aStmt->mShardStmtCxt.mParentStmt;
    }
    else // COORD(Server-side) stmt.
    {
        sMetaStmt = aStmt;
    }

    sMetaStmt->mShardStmtCxt.mHasLocatorInBoundParam = ACP_FALSE;
}

ACP_INLINE void ulsdLobResetLocatorParamToCopy(ulnStmt *aStmt)
{
    ulnStmt        *sMetaStmt = NULL;

    if ( ULSD_IS_SHARD_LIB_SESSION(aStmt->mParentDbc) == ACP_TRUE )
    {
        sMetaStmt = aStmt->mShardStmtCxt.mParentStmt;
    }
    else // COORD(Server-side) stmt.
    {
        sMetaStmt = aStmt;
    }

    sMetaStmt->mShardStmtCxt.mHasLocatorParamToCopy = ACP_FALSE;
}

#endif  /* _O_ULSD_LOB_H_ */

