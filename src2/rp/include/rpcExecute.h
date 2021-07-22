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
 * $Id: rpcExecute.h 53446 2012-06-12 15:59:00Z junyoung $
 **********************************************************************/

#ifndef  _O_RPC_EXECUTE_H_
#define _O_RPC_EXECUTE_H_  1

#include <qci.h>

class rpcExecute
{
public:
    static qciExecuteReplicationCallback mCallback;

    /*------------------- DDL -------------------*/
    static IDE_RC executeCreate(void * aQcStatement);
    static IDE_RC executeAlterAddTbl(void * aQcStatement);
    static IDE_RC executeAlterDropTbl(void * aQcStatement);
    static IDE_RC executeAlterAddHost(void * aQcStatement);
    static IDE_RC executeAlterDropHost(void * aQcStatement);
    static IDE_RC executeAlterSetHost(void * aQcStatement);
    static IDE_RC executeAlterSetMode(void * aQcStatement);
    static IDE_RC executeDrop(void * aQcStatement);
    static IDE_RC executeStart(void * aQcStatement);
    static IDE_RC executeQuickStart(void * aQcStatement);
    static IDE_RC executeSync(void * aQcStatement);
    static IDE_RC executeSyncCondition(void * aQcStatement);
    static IDE_RC executeTempSync(void * aQcStatement);
    static IDE_RC executeReset(void * aQcStatement);
    static IDE_RC executeAlterSetRecovery(void * aQcStatement);

    /* PROJ-1915 */
    static IDE_RC executeAlterSetOfflineEnable(void * aQcStatement);
    static IDE_RC executeAlterSetOfflineDisable(void * aQcStatement);

    /* PROJ-1969 */
    static IDE_RC executeAlterSetGapless(void * aQcStatement);
    static IDE_RC executeAlterSetParallel(void * aQcStatement);
    static IDE_RC executeAlterSetGrouping(void * aQcStatement);

    static IDE_RC executeAlterSetDDLReplicate( void * aQcStatement );

    /* BUG-42851 */
    static IDE_RC executeAlterSplitPartition( void         * aQcStatement,
                                              qcmTableInfo * aTableInfo,
                                              qcmTableInfo * aSrcPartInfo,
                                              qcmTableInfo * aDstPartInfo1,
                                              qcmTableInfo * aDstPartInfo2 );

    static IDE_RC executeAlterMergePartition( void         * aQcStatement,
                                              qcmTableInfo * aTableInfo,
                                              qcmTableInfo * aSrcPartInfo1,
                                              qcmTableInfo * aSrcPartInfo2,
                                              qcmTableInfo * aDstPartInfo );

    static IDE_RC executeAlterDropPartition( void          * aQcStatement,
                                             qcmTableInfo  * aTableInfo,
                                             qcmTableInfo  * aSrcPartInfo );

    static IDE_RC executeFailover( void * aQcStatement );

    /*------------------- DCL -------------------*/
    static IDE_RC executeStop( void         * aQcStatement,
                               SChar        * aReplName );                              
    static IDE_RC executeFlush( smiStatement  * aSmiStmt,
                                SChar         * aReplName,
                                rpFlushOption * aFlushOption,
                                idvSQL        * aStatistics );
    static IDE_RC executeFailback(void * aQcStatement);

    static IDE_RC executeDeleteItemReplaceHistory(void * aQcStatement);

};

#endif  // _O_RPC_EXECUTE_H_
