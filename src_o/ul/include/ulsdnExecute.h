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
 

/**********************************************************************
 * $Id: ulsdnExecute.h 00000 2017-02-27 10:26:00Z swhors $
 **********************************************************************/

#ifndef _O_ULSDN_EXECUTE_H_
#define _O_ULSDN_EXECUTE_H_ 1

#include <ulsdDef.h>

/* PROJ-2638 shard native linker */
SQLRETURN ulsdExecuteForMtDataRows( ulnStmt      *aStmt,
                                    acp_char_t   *aOutBuf,
                                    acp_uint32_t  aOutBufLen,
                                    acp_uint32_t *aOffSets,
                                    acp_uint32_t *aMaxBytes,
                                    acp_uint16_t  aColumnCount );

/* BUG-45392 */
SQLRETURN ulsdExecuteForMtData( ulnStmt *aStmt );

SQLRETURN ulsdExecuteForMtDataRowsAddCallback( acp_uint32_t       aIndex,
                                               ulnStmt           *aStmt,
                                               acp_char_t        *aOutBuf,
                                               acp_uint32_t       aOutBufLen,
                                               acp_uint32_t      *aOffSets,
                                               acp_uint32_t      *aMaxBytes,
                                               acp_uint16_t       aColumnCount,
                                               ulsdFuncCallback **aCallback );

SQLRETURN ulsdExecuteForMtDataAddCallback( acp_uint32_t       aIndex,
                                           ulnStmt           *aStmt,
                                           ulsdFuncCallback **aCallback );

SQLRETURN ulsdExecDirectAddCallback( acp_uint32_t       aIndex,
                                     ulnStmt           *aStmt,
                                     acp_char_t        *aQuery,
                                     acp_sint32_t       aQueryLen,
                                     ulsdFuncCallback **aCallback );

SQLRETURN ulsdExecuteAddCallback( acp_uint32_t       aIndex,
                                  ulnStmt           *aStmt,
                                  ulsdFuncCallback **aCallback );

SQLRETURN ulsdPrepareAddCallback( acp_uint32_t       aIndex,
                                  ulnStmt           *aStmt,
                                  acp_char_t        *aQuery,
                                  acp_sint32_t       aQueryLen,
                                  ulsdFuncCallback **aCallback );

SQLRETURN ulsdPrepareTranAddCallback( acp_uint32_t       aIndex,
                                      ulnDbc            *aDbc,
                                      acp_uint32_t       aXIDSize,
                                      acp_uint8_t       *aXID,
                                      acp_uint8_t       *aReadOnly,
                                      ulsdFuncCallback **aCallback );

SQLRETURN ulsdEndPendingTranAddCallback( acp_uint32_t       aIndex,
                                         ulnDbc            *aDbc,
                                         acp_uint32_t       aXIDSize,
                                         acp_uint8_t       *aXID,
                                         acp_sint16_t       aCompletionType,
                                         ulsdFuncCallback **aCallback );

SQLRETURN ulsdEndTranAddCallback( acp_uint32_t       aIndex,
                                  ulnDbc            *aDbc,
                                  acp_sint16_t       aCompletionType,
                                  ulsdFuncCallback **aCallback );

SQLRETURN ulsdShardEndTranAddCallback( acp_uint32_t       aIndex,
                                       ulnDbc            *aDbc,
                                       acp_sint16_t       aCompletionType,
                                       ulsdFuncCallback **aCallback );

SQLRETURN ulsdPutLobAddCallback( acp_uint32_t       aIndex,
                                 ulnStmt           *aStmt,
                                 acp_sint16_t       aLocatorCType,
                                 acp_uint64_t       aLocator,
                                 acp_uint32_t       aFromPosition,
                                 acp_uint32_t       aForLength,
                                 acp_sint16_t       aSourceCType,
                                 void              *aBuffer,
                                 acp_uint32_t       aBufferSize,
                                 ulsdFuncCallback **aCallback );

void ulsdDoCallback( ulsdFuncCallback *aCallback );

void ulsdReDoCallback( ulsdFuncCallback *aCallback );

SQLRETURN ulsdGetResultCallback( acp_uint32_t      aIndex,
                                 ulsdFuncCallback *aCallback,
                                 acp_uint8_t       aReCall );

void ulsdRemoveCallback( ulsdFuncCallback *aCallback );

void ulsdStmtCallback( ulnStmt *aStmt );

void ulsdDbcCallback( ulnDbc *aDbc );

/* BUG-46814 */
ACI_RC ulsdVerifyFuncCallback( ulnObject *aObject, ulsdFuncCallback *aFuncCallback );

#endif /* _O_ULSDN_EXECUTE_H_ */
