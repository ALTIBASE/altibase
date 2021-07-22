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


/***********************************************************************
* $Id:
***********************************************************************/

#ifndef _0_RPC_DDL_ASYNC_MANAGER_H_
#define _0_RPC_DDL_ASYNC_MANAGER_H_ 1

#include <rpxSender.h>
#include <rpxReceiver.h>

class rpcDDLASyncManager
{
/* Variables */
public:
private:
    /* Functions */
public:
    static IDE_RC ddlASynchronization( rpxSender * aSender,
                                       smTID       aTID,
                                       smSN        aDDLCommitSN );

    static IDE_RC ddlASynchronizationInternal( rpxReceiver * aReceiver );

private:
    static IDE_RC ddlASyncStart( rpxSender * aSender );

    static IDE_RC executeDDLASync( rpxSender * aSender,
                                   smTID       aTID,
                                   smSN        aDDLCommitSN );

    static IDE_RC getDDLASyncInfo( smTID       aTID,
                                   rpxSender * aSender,
                                   SInt        aMaxDDLStmtLen,
                                   UInt      * aTargetCount,
                                   SChar     * aTargetTableName,
                                   SChar    ** aTargetPartNames,
                                   SChar     * aUserName,
                                   SChar     * aDDLStmt );

    static IDE_RC ddlASyncStartInternal( rpxReceiver * aReceiver );

    static IDE_RC executeDDLASyncInternal( rpxReceiver * aReceiver );

    static IDE_RC checkDDLASyncAvailable( rpxReceiver * aReceiver,
                                          SChar       * aUserName,
                                          UInt          aDDLEnableLevel,
                                          UInt          aTargetCount,
                                          SChar       * aTargetTableName,
                                          SChar       * aTargetPartNames,
                                          SChar       * aDDLStmt,
                                          rpdVersion  * aVersion );


    static idBool isAlreadyAppliedDDL( smSN aRemoteDDLCommitSN );

    static IDE_RC checkLocalAndRemoteNames( SChar  * aUserName,
                                            UInt     aTargetCount,
                                            SChar  * aTargetTableName,
                                            UInt     aTargetMaxPartCount,
                                            SChar  * aTargetPartNames );

    static IDE_RC runDDLNMetaRebuild( rpxReceiver * aReceiver,
                                      SChar       * aDDLStmt,
                                      SChar       * aUserName,
                                      smSN          aLastRemoteDDLXSN,
                                      idBool      * aDoSendAck );

    static IDE_RC updateRemoteLastDDLXSN( smiTrans * aTrans, 
                                          SChar    * aRepName,
                                          smSN       aSN );

    static idBool isRetryDDLError( UInt aErrCode );

    static void   printTargetInfo( UInt    aTargetCount,
                                   SChar * aUserName,
                                   SChar * aTargetTableName,
                                   SChar * aTargetPartNames );

};

#endif
