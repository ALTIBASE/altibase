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
 
/*
 * Copyright 2016, Altibase Corporation or its subsidiaries.
 * All rights reserved.
 */

#ifndef __OA_APPLIER_INTERFACE_H__
#define __OA_APPLIER_INTERFACE_H__

#ifdef ALTIADAPTER
#include <oaAltiApplier.h>
typedef struct oaAltiApplierHandle oaApplierHandle;
#elif JDBCADAPTER
#include <oaJDBCApplier.h>
typedef struct oaJDBCApplierHandle oaApplierHandle;
#else
#include <oaOciApplier.h>
typedef struct oaOciApplierHandle oaApplierHandle;
#endif

#include <alaAPI.h>

ace_rc_t initializeApplier( oaContext                * aContext,
                            oaConfigHandle           * aConfigHandle,
                            const ALA_Replication    * aAlaReplication,
                            oaApplierHandle         ** aApplierHandle );

void finalizeImportedLibrary( void );

void finalizeApplier( oaApplierHandle * aApplierHandle );

void addPlaceHolderToQuery(acp_str_t          * aQuery,
                           acp_sint32_t         aPlaceHolderIndex,
                           oaLogRecordValueType aLogRecordValueType );

void prepareInsertQuery( oaLogRecordInsert * aLogRecord,
                         acp_str_t         * aQuery,
                         acp_bool_t          aIsDirectPathMode,
                         acp_bool_t          aSetUserToTable,
                         acp_bool_t          aSetColumnToInsert );

void prepareUpdateQuery( oaLogRecordUpdate * aLogRecord,
                         acp_str_t         * aQuery,
                         acp_bool_t          aSetUserToTable );

void prepareDeleteQuery( oaLogRecordDelete * aLogRecord,
                         acp_str_t         * aQuery,
                         acp_bool_t          aSetUserToTable  );

acp_uint32_t oaApplierGetArrayDMLMaxSize( oaApplierHandle * aHandle );

acp_bool_t oaApplierIsGroupCommit( oaApplierHandle * aHandle );

ace_rc_t oaApplierApplyLogRecordList( oaContext       * aContext,
                                      oaApplierHandle * aHandle,
                                      acp_list_t      * aLogRecordList,
                                      oaLogSN           aPrevLastProcessedSN,
                                      oaLogSN         * aLastProcessedSN );

acp_bool_t oaApplierIsAckNeeded( oaApplierHandle * aHandle,
                                 oaLogRecord        * aLogRecord );

#endif /* __OA_APPLIER_INTERFACE_H__ */
