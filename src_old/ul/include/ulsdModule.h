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

#ifndef _O_ULSD_MODULE_H_
#define _O_ULSD_MODULE_H_ 1

#include <ulnPrivate.h>

/* Interface */
typedef ACI_RC (*ulsdModuleHandshakeFunc)(ulnFnContext *aFnContext);
typedef SQLRETURN (*ulsdModuleNodeDriverConnectFunc)(ulnDbc       *aDbc,
                                                     ulnFnContext *aFnContext,
                                                     acp_char_t   *aConnString,
                                                     acp_sint16_t  aConnStringLength);

/* BUG-47327 */
typedef SQLRETURN (*ulsdModuleNodeConnectFunc)( ulnDbc       * aDbc,
                                                ulnFnContext * aFnContext,
                                                acp_char_t   * aServerName,
                                                acp_sint16_t   aServerNameLength,
                                                acp_char_t   * aUserName,
                                                acp_sint16_t   aUserNameLength,
                                                acp_char_t   * aPassword,
                                                acp_sint16_t   aPasswordLength );

typedef ACI_RC (*ulsdModuleEnvRemoveDbcFunc)(ulnEnv *aEnv, ulnDbc *aDbc);
typedef void (*ulsdModuleStmtDestroyFunc)(ulnStmt *aStmt);
typedef SQLRETURN (*ulsdModulePrepareFunc)(ulnFnContext *aFnContext,
                                           ulnStmt      *aStmt,
                                           acp_char_t   *aStatementText,
                                           acp_sint32_t  aTextLength,
                                           acp_char_t   *aAnalyzeText);
typedef SQLRETURN (*ulsdModuleExecuteFunc)(ulnFnContext *aFnContext,
                                           ulnStmt      *aStmt);
typedef SQLRETURN (*ulsdModuleFetchFunc)(ulnFnContext *aFnContext,
                                         ulnStmt      *aStmt);
typedef SQLRETURN (*ulsdModuleCloseCursorFunc)(ulnStmt *aStmt);
typedef SQLRETURN (*ulsdModuleRowCountFunc)(ulnFnContext *aFnContext,
                                            ulnStmt      *aStmt,
                                            ulvSLen      *aRowCount);
typedef SQLRETURN (*ulsdModuleMoreResultsFunc)(ulnFnContext *aFnContext,
                                               ulnStmt      *aStmt);
typedef ulnStmt* (*ulsdModuleGetPreparedStmtFunc)(ulnStmt *aStmt);
typedef void (*ulsdModuleOnCmErrorFunc)(ulnFnContext     *aFnContext,
                                        ulnDbc           *aDbc,
                                        ulnErrorMgr      *aErrorMgr);
typedef ACI_RC (*ulsdModuleUpdateNodeListFunc)(ulnFnContext  *aFnContext,
                                               ulnDbc        *aDbc);
typedef acp_bool_t (*ulsdModuleHasNoDataFunc)( ulnStmt * aStmt );

typedef ACI_RC (*ulsdModuleNofityFailOverFunc)( ulnDbc       *aNodeDbc );

typedef void (*ulsdModuleAlignDataNodeConnectionFunc)( ulnFnContext * aFnContext,
                                                       ulnDbc       * aNodeDbc );

typedef void (*ulsdModuleErrorCheckAndAlignDataNodeFunc)( ulnFnContext * aFnContext );

/*
 * PROJ-2739 Client-side Sharding LOB
 */
typedef SQLRETURN (*ulsdModuleGetLobLengthFunc)( ulnFnContext *aFnContext,
                                                 ulnStmt      *aStmt,
                                                 acp_uint64_t  aLocator,
                                                 acp_sint16_t  aLocatorType,
                                                 acp_uint32_t *aLengthPtr );

typedef SQLRETURN (*ulsdModuleGetLobFunc)( ulnFnContext *aFnContext,
                                           ulnStmt      *aStmt,
                                           acp_sint16_t  aLocatorCType,
                                           acp_uint64_t  aSrcLocator,
                                           acp_uint32_t  aFromPosition,
                                           acp_uint32_t  aForLength,
                                           acp_sint16_t  aTargetCType,
                                           void         *aBuffer,
                                           acp_uint32_t  aBufferSize,
                                           acp_uint32_t *aLengthWritten );

typedef SQLRETURN (*ulsdModulePutLobFunc)( ulnFnContext *aFnContext,
                                           ulnStmt      *aStmt,
                                           acp_sint16_t  aLocatorCType,
                                           acp_uint64_t  aLocator,
                                           acp_uint32_t  aFromPosition,
                                           acp_uint32_t  aForLength,
                                           acp_sint16_t  aSourceCType,
                                           void         *aBuffer,
                                           acp_uint32_t  aBufferSize );

typedef SQLRETURN (*ulsdModuleFreeLobFunc)( ulnFnContext *aFnContext,
                                            ulnStmt      *aStmt,
                                            acp_uint64_t  aLocator );

typedef SQLRETURN (*ulsdModuleTrimLobFunc)( ulnFnContext *aFnContext,
                                            ulnStmt       *aStmt,
                                            acp_sint16_t   aLocatorCType,
                                            acp_uint64_t   aLocator,
                                            acp_uint32_t   aStartOffset );

struct ulsdModule
{
    ulsdModuleHandshakeFunc                     ulsdModuleHandshake;
    ulsdModuleNodeDriverConnectFunc             ulsdModuleNodeDriverConnect;
    ulsdModuleNodeConnectFunc                   ulsdModuleNodeConnect;
    ulsdModuleEnvRemoveDbcFunc                  ulsdModuleEnvRemoveDbc;
    ulsdModuleStmtDestroyFunc                   ulsdModuleStmtDestroy;
    ulsdModulePrepareFunc                       ulsdModulePrepare;
    ulsdModuleExecuteFunc                       ulsdModuleExecute;
    ulsdModuleFetchFunc                         ulsdModuleFetch;
    ulsdModuleCloseCursorFunc                   ulsdModuleCloseCursor;
    ulsdModuleRowCountFunc                      ulsdModuleRowCount;
    ulsdModuleMoreResultsFunc                   ulsdModuleMoreResults;
    ulsdModuleGetPreparedStmtFunc               ulsdModuleGetPreparedStmt;
    ulsdModuleOnCmErrorFunc                     ulsdModuleOnCmError;
    ulsdModuleUpdateNodeListFunc                ulsdModuleUpdateNodeList;
    ulsdModuleNofityFailOverFunc                ulsdModuleNotifyFailOver;
    ulsdModuleAlignDataNodeConnectionFunc       ulsdModuleAlignDataNodeConnection;
    ulsdModuleErrorCheckAndAlignDataNodeFunc    ulsdModuleErrorCheckAndAlignDataNode;
    ulsdModuleHasNoDataFunc                     ulsdModuleHasNoData;

    /* PROJ-2739 Client-side Sharding LOB */
    ulsdModuleGetLobLengthFunc                  ulsdModuleGetLobLength;
    ulsdModuleGetLobFunc                        ulsdModuleGetLob;
    ulsdModulePutLobFunc                        ulsdModulePutLob;
    ulsdModuleFreeLobFunc                       ulsdModuleFreeLob;
    ulsdModuleTrimLobFunc                       ulsdModuleTrimLob;
};

/* Module */
extern ulsdModule gShardModuleCOORD; /* Shard COORDINATOR : server-side execution */
extern ulsdModule gShardModuleMETA; /* Shard META : client-side execution */
extern ulsdModule gShardModuleNODE; /* Shard NODE */

/* Module Wrapper */
ACI_RC ulsdModuleHandshake(ulnFnContext *aFnContext);
SQLRETURN ulsdModuleNodeDriverConnect(ulnDbc       *aDbc,
                                      ulnFnContext *aFnContext,
                                      acp_char_t   *aConnString,
                                      acp_sint16_t  aConnStringLength);

/* BUG-47327 */
SQLRETURN ulsdModuleNodeConnect( ulnDbc       * aDbc,
                                 ulnFnContext * aFnContext,
                                 acp_char_t   * aServerName,
                                 acp_sint16_t   aServerNameLength,
                                 acp_char_t   * aUserName,
                                 acp_sint16_t   aUserNameLength,
                                 acp_char_t   * aPassword,
                                 acp_sint16_t   aPasswordLength );

ACI_RC ulsdModuleEnvRemoveDbc(ulnEnv *aEnv, ulnDbc *aDbc);
void ulsdModuleStmtDestroy(ulnStmt *aStmt);
SQLRETURN ulsdModulePrepare(ulnFnContext *aFnContext,
                            ulnStmt      *aStmt,
                            acp_char_t   *aStatementText,
                            acp_sint32_t  aTextLength,
                            acp_char_t   *aAnalyzeText);
SQLRETURN ulsdModuleExecute(ulnFnContext *aFnContext,
                            ulnStmt      *aStmt);
SQLRETURN ulsdModuleFetch(ulnFnContext *aFnContext,
                          ulnStmt      *aStmt);
SQLRETURN ulsdModuleCloseCursor(ulnStmt *aStmt);
SQLRETURN ulsdModuleRowCount(ulnFnContext *aFnContext,
                             ulnStmt      *aStmt,
                             ulvSLen      *aRowCount);
SQLRETURN ulsdModuleMoreResults(ulnFnContext *aFnContext,
                                ulnStmt      *aStmt);
ulnStmt* ulsdModuleGetPreparedStmt(ulnStmt *aStmt);
void ulsdModuleOnCmError(ulnFnContext     *aFnContext,
                         ulnDbc           *aDbc,
                         ulnErrorMgr      *aErrorMgr);
ACI_RC ulsdModuleUpdateNodeList(ulnFnContext  *aFnContext,
                                ulnDbc        *aDbc);

acp_bool_t ulsdModuleHasNoData( ulnStmt * aStmt );

ACI_RC ulsdModuleNotifyFailOver( ulnDbc       *aDbc );

void ulsdModuleAlignDataNodeConnection( ulnFnContext * aFnContext,
                                        ulnDbc       * aNodeDbc );

void ulsdModuleErrorCheckAndAlignDataNode( ulnFnContext * aFnContext );

/*
 * PROJ-2739 Client-side Sharding LOB
 */
SQLRETURN ulsdModuleGetLobLength( ulnFnContext * aFnContext,
                                  ulnStmt      * aStmt,
                                  acp_uint64_t   aLocator,
                                  acp_sint16_t   aLocatorType,
                                  acp_uint32_t * aLengthPtr );

SQLRETURN ulsdModuleGetLob( ulnFnContext * aFnContext,
                            ulnStmt      * aStmt,
                            acp_sint16_t   aLocatorCType,
                            acp_uint64_t   aSrcLocator,
                            acp_uint32_t   aFromPosition,
                            acp_uint32_t   aForLength,
                            acp_sint16_t   aTargetCType,
                            void         * aBuffer,
                            acp_uint32_t   aBufferSize,
                            acp_uint32_t * aLengthWritten );

SQLRETURN ulsdModulePutLob( ulnFnContext * aFnContext,
                            ulnStmt      * aStmt,
                            acp_sint16_t   aLocatorCType,
                            acp_uint64_t   aLocator,
                            acp_uint32_t   aFromPosition,
                            acp_uint32_t   aForLength,
                            acp_sint16_t   aSourceCType,
                            void         * aBuffer,
                            acp_uint32_t   aBufferSize );

SQLRETURN ulsdModuleFreeLob( ulnFnContext * aFnContext,
                             ulnStmt      * aStmt,
                             acp_uint64_t   aLocator );

SQLRETURN ulsdModuleTrimLob( ulnFnContext  * aFnContext,
                             ulnStmt       * aStmt,
                             acp_sint16_t    aLocatorCType,
                             acp_uint64_t    aLocator,
                             acp_uint32_t    aStartOffset );

ACP_INLINE void ulsdSetEnvShardModule(ulnEnv *aEnv, ulsdModule *aModule)
{
    aEnv->mShardModule = aModule;
}

ACP_INLINE void ulsdSetDbcShardModule(ulnDbc *aDbc, ulsdModule *aModule)
{
    aDbc->mShardModule = aModule;
}

ACP_INLINE void ulsdSetStmtShardModule(ulnStmt *aStmt, ulsdModule *aModule)
{
    aStmt->mShardModule = aModule;
}

#endif /* _O_ULSD_MODULE_H_ */
