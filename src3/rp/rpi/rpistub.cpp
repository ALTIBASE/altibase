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
 * $Id: rpi.cpp 34865 2009-08-18 01:54:08Z sparrow $
 **********************************************************************/

#include <idl.h>
#include <smi.h>
#include <rpi.h>

IDE_RC getMinimumSN(const UInt * /*aRestartRedoFileNo*/,
                    const UInt * /*aLastArchiveFileNo*/,
                    smSN       * aSN)
{
    *aSN = SM_SN_NULL;

    return IDE_SUCCESS;
}

IDE_RC
rpi::initREPLICATION()
{
    (void)smiSetCallbackFunction(getMinimumSN, NULL, NULL, NULL, NULL, NULL);

    return IDE_SUCCESS;
}

IDE_RC
rpi::finalREPLICATION()
{
    return IDE_SUCCESS;
}

IDE_RC
rpi::createReplication( void        * /*aStatement*/ )
{
    IDE_SET(ideSetErrorCode(idERR_ABORT_MODULE_IS_NOT_LOADED, "REPLICATION"));

    return IDE_FAILURE;
}

IDE_RC
rpi::alterReplicationFlush( smiStatement  * /* aSmiStmt */,
                            SChar         * /* aReplName */,
                            rpFlushOption * /* aFlushOption */,
                            idvSQL        * /* aStatistics */ )
{
    IDE_SET(ideSetErrorCode(idERR_ABORT_MODULE_IS_NOT_LOADED, "REPLICATION"));

    return IDE_FAILURE;
}

IDE_RC
rpi::alterReplicationAddTable( void        * /*aStatement*/ )
{
    IDE_SET(ideSetErrorCode(idERR_ABORT_MODULE_IS_NOT_LOADED, "REPLICATION"));

    return IDE_FAILURE;
}

IDE_RC
rpi::alterReplicationDropTable( void        * /*aStatement*/ )
{
    IDE_SET(ideSetErrorCode(idERR_ABORT_MODULE_IS_NOT_LOADED, "REPLICATION"));

    return IDE_FAILURE;
}

IDE_RC
rpi::alterReplicationAddHost( void        * /*aStatement*/ )
{
    IDE_SET(ideSetErrorCode(idERR_ABORT_MODULE_IS_NOT_LOADED, "REPLICATION"));

    return IDE_FAILURE;
}

IDE_RC
rpi::alterReplicationDropHost( void        * /*aStatement*/ )
{
    IDE_SET(ideSetErrorCode(idERR_ABORT_MODULE_IS_NOT_LOADED, "REPLICATION"));

    return IDE_FAILURE;
}

IDE_RC
rpi::alterReplicationSetHost( void        * /*aStatement*/ )
{
    IDE_SET(ideSetErrorCode(idERR_ABORT_MODULE_IS_NOT_LOADED, "REPLICATION"));

    return IDE_FAILURE;
}

IDE_RC 
rpi::alterReplicationSetRecovery( void        * /*aStatement*/ )
{
    IDE_SET(ideSetErrorCode(idERR_ABORT_MODULE_IS_NOT_LOADED, "REPLICATION"));

    return IDE_FAILURE;
}

/* PROJ-1915 */
IDE_RC 
rpi::alterReplicationSetOfflineEnable( void        * /*aStatement*/ )
{
    IDE_SET(ideSetErrorCode(idERR_ABORT_MODULE_IS_NOT_LOADED, "REPLICATION"));

    return IDE_FAILURE;
}

IDE_RC 
rpi::alterReplicationSetOfflineDisable( void        * /*aStatement*/ )
{
    IDE_SET(ideSetErrorCode(idERR_ABORT_MODULE_IS_NOT_LOADED, "REPLICATION"));

    return IDE_FAILURE;
}


IDE_RC
rpi::dropReplication( void        * /*aStatement*/ )
{
    IDE_SET(ideSetErrorCode(idERR_ABORT_MODULE_IS_NOT_LOADED, "REPLICATION"));

    return IDE_FAILURE;
}

IDE_RC
rpi::startSenderThread( idvSQL               * /* aStatistics */,
                        iduVarMemList        * /* aMemory */,
                        smiStatement         * /* aSmiStmt */,
                        SChar                * /* aReplName */,
                        RP_SENDER_TYPE         /* astartType */,
                        idBool                 /* aTryHandshakeOnce */,
                        smSN                   /* aStartSN */,
                        qciSyncItems         * /* aSyncItemList */,
                        SInt                   /* aParallelFactor */,
                        void                 * /* aLockTable */ )
{
    IDE_SET(ideSetErrorCode(idERR_ABORT_MODULE_IS_NOT_LOADED, "REPLICATION"));

    return IDE_FAILURE;
}

IDE_RC
rpi::stopSenderThread( smiStatement * /*aSmiStmt*/,
                       SChar        * /*aReplName*/,
                       idvSQL       * /*aStatistics*/,
                       idBool         /*aIsImmediate*/ )
{
    IDE_SET(ideSetErrorCode(idERR_ABORT_MODULE_IS_NOT_LOADED, "REPLICATION"));

    return IDE_FAILURE;
}

IDE_RC rpi::resetReplication(smiStatement * /*aSmiStmt*/,
                             SChar        * /*aReplName*/,
                             idvSQL       * /*aStatistics*/)
{
    IDE_SET(ideSetErrorCode(idERR_ABORT_MODULE_IS_NOT_LOADED, "REPLICATION"));

    return IDE_FAILURE;
}

IDE_RC rpi::stopReceiverThreads(smiStatement * /*aSmiStmt*/,
                                smOID          /*aTableOID*/,
                                idvSQL       * /*aStatistics*/)
{
    IDE_SET(ideSetErrorCode(idERR_ABORT_MODULE_IS_NOT_LOADED, "REPLICATION"));

    return IDE_FAILURE;
}

IDE_RC rpi::writeTableMetaLog(void        * /*aStatement*/,
                              smOID         /*aOldTableOID*/,
                              smOID         /*aNewTableOID*/)
{
    IDE_SET(ideSetErrorCode(idERR_ABORT_MODULE_IS_NOT_LOADED, "REPLICATION"));

    return IDE_FAILURE;
}

IDE_RC rpi::initSystemTables()
{
    return IDE_SUCCESS;
}
