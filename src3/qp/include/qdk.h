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
 

#ifndef _O_QDK_H_
#define _O_QDK_H_ 1

#include <qdkParseTree.h>
#include <sdi.h>
#include <smiDef.h>

IDE_RC qdkSetDatabaseLinkCallback( qcDatabaseLinkCallback * aCallback );

IDE_RC qdkControlDatabaseLinker( qcStatement                   * aStatement,
                                 qdkDatabaseLinkAlterParseTree * aParseTree );
IDE_RC qdkCloseDatabaseLink( qcStatement                   * aStatement,
                             void                          * aDatabaseLinkSession,
                             qdkDatabaseLinkCloseParseTree * aParseTree );

IDE_RC qdkValidateCreateDatabaseLink( qcStatement * aStatement );
IDE_RC qdkExecuteCreateDatabaseLink( qcStatement * aStatement );

IDE_RC qdkValidateDropDatabaseLink( qcStatement * aStatement );
IDE_RC qdkExecuteDropDatabaseLink( qcStatement * aStatement );

IDE_RC qdkDropDatabaseLinkByUserId( qcStatement * aStatement,
                                    UInt          aUserId );

IDE_RC qdkOpenShardConnection( sdiConnectInfo * aDataNode );

void   qdkCloseShardConnection( sdiConnectInfo * aDataNode );

IDE_RC qdkAddShardTransaction( idvSQL         * aStatistics,
                               smTID            aTransID,
                               sdiClientInfo  * aClientInfo,
                               sdiConnectInfo * aDataNode );

void   qdkDelShardTransaction( sdiConnectInfo * aDataNode );

IDE_RC qdkSetTransactionBrokenOnGlobalCoordinator( void * aDkiSession,
                                                   smTID  aTransID );

IDE_RC qdkAddDtxBranchTx( void   * aDtxInfo,
                          UChar    aCoordinatorType,
                          SChar  * aNodeName,
                          SChar  * aUserName,
                          SChar  * aUserPassword,
                          SChar  * aDataServerIP,
                          UShort   aDataPortNo,
                          UShort   aConnectType );

IDE_RC qdkCheckGlobalTransactionStatus( void * aDataNode );
#endif /* _O_QDK_H_ */
