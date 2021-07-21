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
 

#ifndef _O_DKP_BATCH_STATEMENT_MGR_H_
#define _O_DKP_BATCH_STATEMENT_MGR_H_ 1

#include <dknLink.h>

typedef struct dkpBatchStatementMgr dkpBatchStatementMgr;

typedef struct dkpBatchStatement dkpBatchStatement;

IDE_RC dkpBatchStatementMgrCreate( dkpBatchStatementMgr ** aManager );
void dkpBatchStatementMgrDestroy( dkpBatchStatementMgr * aManager );

IDE_RC dkpBatchStatementMgrClear( dkpBatchStatementMgr * aManager );

IDE_RC dkpBatchStatementMgrAddBindVariable( dkpBatchStatementMgr * aManager,
                                            UInt           aBindVarIdx,
                                            UInt           aBindVarType,
                                            UInt           aBindVarLen,
                                            SChar         *aBindVar );
IDE_RC dkpBatchStatementMgrAddBatchStatement( dkpBatchStatementMgr * aManager,
                                              SLong                  aRemoteStmtId );
UInt dkpBatchStatementMgrCountBatchStatement( dkpBatchStatementMgr * aManager );
UInt dkpBatchStatementMgrCountPacket( dkpBatchStatement * aBatchStatement );

IDE_RC dkpBatchStatementMgrGetNextBatchStatement( dkpBatchStatementMgr * aManager,
                                                  dkpBatchStatement   ** aBatchStatement );
IDE_RC dkpBatchStatementMgrGetNextPacket( dkpBatchStatement * aBatchStatement,
                                          dknPacket        ** aPacket );

#endif /* _O_DKP_BATCH_STATEMENT_MGR_H_ */
