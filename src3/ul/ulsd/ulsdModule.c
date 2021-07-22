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
 * $Id$
 **********************************************************************/

#include <uln.h>
#include <ulnPrivate.h>
#include <mtcc.h>

ACI_RC ulsdModuleHandshake(ulnFnContext *aFnContext)
{
    return aFnContext->mHandle.mDbc->mShardModule->ulsdModuleHandshake(aFnContext);
}

SQLRETURN ulsdModuleNodeDriverConnect(ulnDbc       *aDbc,
                                      ulnFnContext *aFnContext,
                                      acp_char_t   *aConnString,
                                      acp_sint16_t  aConnStringLength)
{
    return aDbc->mShardModule->ulsdModuleNodeDriverConnect(aDbc,
                                                           aFnContext,
                                                           aConnString,
                                                           aConnStringLength);
}

SQLRETURN ulsdModuleNodeConnect( ulnDbc       * aDbc,
                                 ulnFnContext * aFnContext,
                                 acp_char_t   * aServerName,
                                 acp_sint16_t   aServerNameLength,
                                 acp_char_t   * aUserName,
                                 acp_sint16_t   aUserNameLength,
                                 acp_char_t   * aPassword,
                                 acp_sint16_t   aPasswordLength )
{
    return aDbc->mShardModule->ulsdModuleNodeConnect( aDbc,
                                                      aFnContext,
                                                      aServerName,
                                                      aServerNameLength,
                                                      aUserName,
                                                      aUserNameLength,
                                                      aPassword,
                                                      aPasswordLength );
}                                                     

ACI_RC ulsdModuleEnvRemoveDbc(ulnEnv *aEnv, ulnDbc *aDbc)
{
    return aDbc->mShardModule->ulsdModuleEnvRemoveDbc(aEnv, aDbc);
}

void ulsdModuleStmtDestroy(ulnStmt *aStmt)
{
    aStmt->mShardModule->ulsdModuleStmtDestroy(aStmt);
}

SQLRETURN ulsdModulePrepare(ulnFnContext *aFnContext,
                            ulnStmt      *aStmt,
                            acp_char_t   *aStatementText,
                            acp_sint32_t  aTextLength,
                            acp_char_t   *aAnalyzeText)
{
    return aStmt->mShardModule->ulsdModulePrepare(aFnContext,
                                                  aStmt,
                                                  aStatementText,
                                                  aTextLength,
                                                  aAnalyzeText);
}

SQLRETURN ulsdModuleExecute(ulnFnContext *aFnContext,
                            ulnStmt      *aStmt)
{
    return aStmt->mShardModule->ulsdModuleExecute(aFnContext, aStmt);
}

SQLRETURN ulsdModuleFetch(ulnFnContext *aFnContext,
                          ulnStmt      *aStmt)
{
    return aStmt->mShardModule->ulsdModuleFetch(aFnContext, aStmt);
}

SQLRETURN ulsdModuleCloseCursor(ulnStmt *aStmt)
{
    return aStmt->mShardModule->ulsdModuleCloseCursor(aStmt);
}

SQLRETURN ulsdModuleRowCount(ulnFnContext *aFnContext,
                             ulnStmt      *aStmt,
                             ulvSLen      *aRowCount)
{
    return aStmt->mShardModule->ulsdModuleRowCount(aFnContext, aStmt, aRowCount);
}

SQLRETURN ulsdModuleMoreResults(ulnFnContext *aFnContext,
                                ulnStmt      *aStmt)
{
    return aStmt->mShardModule->ulsdModuleMoreResults(aFnContext, aStmt);
}

ulnStmt* ulsdModuleGetPreparedStmt(ulnStmt *aStmt)
{
    return aStmt->mShardModule->ulsdModuleGetPreparedStmt(aStmt);
}

void ulsdModuleOnCmError(ulnFnContext     *aFnContext,
                         ulnDbc           *aDbc,
                         ulnErrorMgr      *aErrorMgr)
{
    aDbc->mShardModule->ulsdModuleOnCmError(aFnContext, aDbc, aErrorMgr);
}

ACI_RC ulsdModuleUpdateNodeList(ulnFnContext  *aFnContext,
                                ulnDbc        *aDbc)
{
    return aDbc->mShardModule->ulsdModuleUpdateNodeList(aFnContext, aDbc);
}

ACI_RC ulsdModuleNotifyFailOver( ulnDbc *aDbc )
{
    return aDbc->mShardModule->ulsdModuleNotifyFailOver( aDbc );
}

void ulsdModuleAlignDataNodeConnection( ulnFnContext * aFnContext,
                                        ulnDbc       * aNodeDbc )
{
    ulnDbc * sDbc = NULL;

    ULN_FNCONTEXT_GET_DBC( aFnContext, sDbc );

    ACE_DASSERT( sDbc != NULL );

    sDbc->mShardModule->ulsdModuleAlignDataNodeConnection( aFnContext,
                                                           aNodeDbc );
    return;
}

void ulsdModuleErrorCheckAndAlignDataNode( ulnFnContext * aFnContext )
{
    ulnDbc * sDbc = NULL;

    ULN_FNCONTEXT_GET_DBC( aFnContext, sDbc );

    ACE_DASSERT( sDbc != NULL );

    sDbc->mShardModule->ulsdModuleErrorCheckAndAlignDataNode( aFnContext );

    return;
}


acp_bool_t ulsdModuleHasNoData( ulnStmt * aStmt )
{
    return aStmt->mShardModule->ulsdModuleHasNoData( aStmt );
}

/*
 * PROJ-2739 Client-side Sharding LOB
 */
SQLRETURN ulsdModuleGetLobLength( ulnFnContext *aFnContext,
                                  ulnStmt      *aStmt,
                                  acp_uint64_t  aLocator,
                                  acp_sint16_t  aLocatorType,
                                  acp_uint32_t *aLengthPtr )
{
    return aStmt->mShardModule->ulsdModuleGetLobLength( aFnContext,
                                                        aStmt,
                                                        aLocator,
                                                        aLocatorType,
                                                        aLengthPtr );
}

SQLRETURN ulsdModuleGetLob( ulnFnContext *aFnContext,
                            ulnStmt      *aStmt,
                            acp_sint16_t  aLocatorCType,
                            acp_uint64_t  aSrcLocator,
                            acp_uint32_t  aFromPosition,
                            acp_uint32_t  aForLength,
                            acp_sint16_t  aTargetCType,
                            void         *aBuffer,
                            acp_uint32_t  aBufferSize,
                            acp_uint32_t *aLengthWritten )
{
    return aStmt->mShardModule->ulsdModuleGetLob( aFnContext,
                                                  aStmt,
                                                  aLocatorCType,
                                                  aSrcLocator,
                                                  aFromPosition,
                                                  aForLength,
                                                  aTargetCType,
                                                  aBuffer,
                                                  aBufferSize,
                                                  aLengthWritten );
}

SQLRETURN ulsdModulePutLob( ulnFnContext *aFnContext,
                            ulnStmt      *aStmt,
                            acp_sint16_t  aLocatorCType,
                            acp_uint64_t  aLocator,
                            acp_uint32_t  aFromPosition,
                            acp_uint32_t  aForLength,
                            acp_sint16_t  aSourceCType,
                            void         *aBuffer,
                            acp_uint32_t  aBufferSize )
{
    return aStmt->mShardModule->ulsdModulePutLob( aFnContext,
                                                  aStmt,
                                                  aLocatorCType,
                                                  aLocator,
                                                  aFromPosition,
                                                  aForLength,
                                                  aSourceCType,
                                                  aBuffer,
                                                  aBufferSize );
}

SQLRETURN ulsdModuleFreeLob( ulnFnContext *aFnContext,
                             ulnStmt      *aStmt,
                             acp_uint64_t  aLocator )
{
    return aStmt->mShardModule->ulsdModuleFreeLob( aFnContext,
                                                   aStmt,
                                                   aLocator );
}

SQLRETURN ulsdModuleTrimLob( ulnFnContext  *aFnContext,
                             ulnStmt       *aStmt,
                             acp_sint16_t   aLocatorCType,
                             acp_uint64_t   aLocator,
                             acp_uint32_t   aStartOffset )
{
    return aStmt->mShardModule->ulsdModuleTrimLob( aFnContext,
                                                   aStmt,
                                                   aLocatorCType,
                                                   aLocator,
                                                   aStartOffset );
}

