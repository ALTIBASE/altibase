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

#ifndef _O_ULN_FETCH_CORE_H_
#define _O_ULN_FETCH_CORE_H_ 1

ACI_RC ulnCheckFetchOrientation(ulnFnContext *aFnContext,
                                acp_sint16_t  aFetchOrientation);

ACI_RC ulnFetchCore(ulnFnContext *aFnContext,
                    ulnPtContext *aPtContext,
                    acp_sint16_t  aFetchOrientation,
                    ulvSLen       aFetchOffset,
                    acp_uint32_t *aNumberOfRowsFetched);

/* PROJ-2616 */
ACI_RC ulnFetchCoreForIPCDASimpleQuery(ulnFnContext *aFnContext,
                                       ulnStmt      *aStmt,
                                       acp_uint32_t *aFetchedRowCount);

#endif /* _O_ULN_FETCH_CORE_H_ */
