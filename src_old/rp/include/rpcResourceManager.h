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

#ifndef _O_RPC_RESOURCE_MANAGER_H_
#define _O_RPC_RESOURCE_MANAGER_H_ 1

#include <idl.h>
#include <qci.h>
#include <rpcDDLSyncManager.h>

typedef struct rpcSmiStmt
{
    smiStatement    mStmt;
    iduListNode     mNode;
} rpcSmiStmt;

typedef struct rpcQciStmt
{
    qciStatement    mStmt;
    iduListNode     mNode;
} rpcQciStmt;

typedef struct rpcSmiTrans
{
    smiTrans        mTrans;
    iduListNode     mNode;
} rpcSmiTrans;

class rpcResourceManager
{
    /* Variable */
    public:
        rpcDDLSyncInfo           * mDDLSyncInfo;
        RP_DDL_SYNC_MSG_TYPE       mMsgType;

    private:
        iduList          mSmiStmtList;
        iduList          mQciStmtList;
        iduList          mSmiTransList;
        iduVarMemList    mMemory;

        /* Function */
    public:
        IDE_RC initialize( iduMemoryClientIndex    aIndex );

        void   finalize( rpcDDLSyncManager    * aDDLSyncMgr,
                         smiTrans             * aDDLTrans );

        void   setDDLSyncInfo( rpcDDLSyncInfo * aDDLSyncInfo )
        {
            mDDLSyncInfo = aDDLSyncInfo;
        }
        rpcDDLSyncInfo*  getDDLSyncInfo()
        {
            return mDDLSyncInfo;
        }
        void setMsgType( RP_DDL_SYNC_MSG_TYPE aMsgType )
        {
            mMsgType = aMsgType;
        }

        IDE_RC smiStmtBegin( smiStatement ** aStmt,
                             idvSQL        * aStatistics,
                             smiStatement  * aParent,
                             UInt            aFlag );
        IDE_RC smiStmtEnd( smiStatement * aStmt, UInt aFlag );
        void   smiStmtEndAll(); 

        IDE_RC qciInitializeStatement( qciStatement ** aStmt,
                                       idvSQL        * aStatistics );
        IDE_RC qciFinalizeStatement( qciStatement    * aStmt );
        void   qciFinalizeStatementAll( );

        IDE_RC smiTransBegin( smiTrans     ** aTrans,
                              smiStatement ** aStatement,
                              idvSQL        * aStatistics,
                              UInt            aFlag );

        IDE_RC smiTransCommit( smiTrans * aTrans );
        void smiTransRollback( smiTrans * aTrans );
        void smiTransRollbackAll();

        IDE_RC rpMalloc( ULong aSize, void **aBuffer );
        IDE_RC rpCalloc( ULong aSize, void **aBuffer );
        IDE_RC rpFreeAll();

        void ddlSyncException( rpcDDLSyncManager   * aDDLSyncMgr,
                               smiTrans            * aDDLTrans );

};
#endif

