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
 
#ifndef _O_ULSD_EXTENSION_H_
#define _O_ULSD_EXTENSION_H_ 1

typedef struct ulsdShardKeyContext
{
    ulnDbc       * ConnectionHandle;
    ulnStmt      * StatementHandle;
    acp_char_t   * TableName;
    acp_sint32_t   TableNameLen;
    acp_char_t   * KeyColumnName;
    acp_sint32_t   KeyColumnNameLen;
    SQLCHAR      * Query;
    SQLCHAR        ShardDistType;
} ulsdShardKeyContext;

typedef enum {
    ULSD_SHARD_DIST_TYPE_UNKNOWN    = 'X',
    ULSD_SHARD_DIST_TYPE_HASH       = 'H',
    ULSD_SHARD_DIST_TYPE_RANGE      = 'R',
    ULSD_SHARD_DIST_TYPE_LIST       = 'L',
    ULSD_SHARD_DIST_TYPE_CLONE      = 'C',
    ULSD_SHARD_DIST_TYPE_SOLO       = 'S',
} ulsdShardDistType;

SQLRETURN ulsdInitializeShardKeyContext( ulsdShardKeyContext ** aCtx,
                                         ulnDbc               * aDbc,
                                         acp_char_t           * aTableName,
                                         acp_sint32_t           aTableNameLen,
                                         acp_char_t           * aKeyColumnName,
                                         acp_sint32_t           aKeyColumnNameLen,
                                         acp_sint16_t           aKeyColumnValueType,
                                         void                 * aKeyColumnValueBuffer,
                                         ulvSLen                aKeyColumnValueBufferLen,
                                         ulvSLen              * aKeyColumnStrLenOrIndPtr,
                                         acp_char_t             aShardDistType[1] );

SQLRETURN ulsdGetNodeByShardKeyValue( ulsdShardKeyContext   * aCtx,
                                      const acp_char_t     ** aDstNodeName );

SQLRETURN ulsdFinalizeShardKeyContext( ulsdShardKeyContext * aCtx );

#endif /* _O_ULSD_EXTENSION_H_ */
