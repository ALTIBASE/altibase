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

#ifndef _O_ULN_FETCH_OP_H_
#define _O_ULN_FETCH_OP_H_ 1

ACI_RC ulnFetchMoveServerCursor(ulnFnContext *aFnContext,
                                ulnPtContext *aPtContext,
                                acp_sint64_t  aOffset);

ACI_RC ulnFetchRequestFetch(ulnFnContext *aFnContext,
                            ulnPtContext *aPtContext,
                            acp_uint32_t  aNumberOfRecordsToFetch,
                            acp_uint16_t  aColumnNumberToStartFrom,
                            acp_uint16_t  aColumnNumberToFetchUntil);

/* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
void ulnFetchInitForFetchResult(ulnFnContext *aFnContext);

ACI_RC ulnFetchUpdateAfterFetch(ulnFnContext *aFnContext);

ACI_RC ulnFetchMoreFromServer(ulnFnContext *aFnContext,
                              ulnPtContext *aPtContext,
                              acp_uint32_t  aNumberOfRowsToGet,
                              acp_uint32_t  aNumberOfPrefechRows);

ACI_RC ulnFetchAllFromServer(ulnFnContext *aFnContext,
                             ulnPtContext *aPtContext);

/* PROJ-2625 Semi-async Prefetch, Prefetch Auto-tuning */
void ulnFetchCalcPrefetchRows(ulnCache     *aCache,
                              ulnCursor    *aCursor,
                              acp_uint32_t *aNumberOfPrefetchRows,
                              acp_sint32_t *aNumberOfRowsToGet);

ACI_RC ulnFetchFromCache(ulnFnContext *aFnContext,
                         ulnPtContext *aPtContext,
                         ulnStmt      *aStmt,
                         acp_uint32_t *aFetchedRowCount);

/* PROJ-1789 Updatable Scrollable Cursor */

ACI_RC ulnFetchFromCacheForKeyset(ulnFnContext *aFnContext,
                                  ulnPtContext *aPtContext,
                                  ulnStmt      *aStmt,
                                  acp_sint64_t  aTargetPos);

ACI_RC ulnFetchFromServerForSensitive(ulnFnContext     *aFnContext,
                                      ulnPtContext     *aPtContext,
                                      acp_sint64_t      aStartPosition,
                                      acp_uint32_t      aFetchCount,
                                      ulnStmtFetchMode  aCacheMode);

ACI_RC ulnFetchFromCacheForSensitive(ulnFnContext *aFnContext,
                                     ulnPtContext *aPtContext,
                                     ulnStmt      *aStmt,
                                     acp_uint32_t *aFetchedRowCount);

#endif /* _O_ULN_FETCH_OP_H_ */
