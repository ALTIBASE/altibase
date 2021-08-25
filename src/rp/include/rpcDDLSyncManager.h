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

#ifndef _O_RPC_DDL_SYNC_MANAGER_H_
#define _O_RPC_DDL_SYNC_MANAGER_H_ 1

#include <idl.h>
#include <rpuProperty.h>
#include <rpdSenderInfo.h>
#include <qci.h>
#include <rpdMeta.h>
#include <rpnComm.h>
#include <rpxDDLExecutor.h>

#define RP_DDL_SYNC_SAVEPOINT_NAME "$$DDL_SYNC_SAVE_POINT"
#define RP_DDL_SQL_BUFFER_MAX      ( 24 * 1024 )
#define RP_DDL_INFO_PART_OID_COUNT QC_DDL_INFO_PART_OID_COUNT

typedef enum
{
    RP_DDL_SYNC_NONE = 0,
    RP_DDL_SYNC_SUCCESS,
    RP_DDL_SYNC_FAILURE,
} rpcDDLResult;

typedef enum
{
    RP_NONE_DDL = 0,
    RP_MASTER_DDL,
    RP_REPLICATED_DDL 
} rpcDDLSyncInfoType;

typedef enum
{
    RP_DDL_SYNC_NONE_MSG = 0,
    RP_DDL_SYNC_INFO_MSG,
    RP_DDL_SYNC_SQL_MSG,
    RP_DDL_SYNC_BEGIN_MSG,
    RP_DDL_SYNC_WAIT_DML_FOR_DDL_MSG,    
    RP_DDL_SYNC_READY_EXECUTE_MSG,
    RP_DDL_SYNC_EXECUTE_MSG,
    RP_DDL_SYNC_EXECUTE_RESULT_MSG,
    RP_DDL_SYNC_COMMIT_MSG,
    RP_DDL_SYNC_CANCEL_MSG,
} RP_DDL_SYNC_MSG_TYPE;

typedef struct rpcDDLTableInfo
{
    UInt                   mTableID;
    smOID                  mTableOID;
    SChar                  mTableName[QC_MAX_OBJECT_NAME_LEN + 1];
    SChar                  mUserName[QC_MAX_OBJECT_NAME_LEN + 1];
    qciAccessOption        mAccessOption;
    UInt                   mPartInfoCount;
    iduList                mPartInfoList;
    idBool                 mIsDDLSyncTable;
    iduListNode            mNode;
} rpcDDLTableInfo;

typedef struct rpcDDLSyncInfo
{
    rpcDDLTableInfo        mDDLTableInfo;
    iduList                mDDLReplInfoList;
    idBool                 mDDLSyncExitFlag;
    rpcDDLSyncInfoType     mDDLSyncInfoType;
    idBool                 mIsBuildNewMeta;
    idBool                 mIsSetSavePoint;
    iduListNode            mNode;
} rpcDDLSyncInfo;

typedef struct rpcDDLReplInfo
{
    SChar                mRepName[QC_MAX_NAME_LEN + 1];
    SChar                mHostIp [QC_MAX_IP_LEN + 1];
    UInt                 mPortNo;
    RP_SOCKET_TYPE       mConnType;
    rpIBLatency          mIBLatency;
    cmiProtocolContext * mProtocolContext;
    idBool             * mDDLSyncExitFlag;
    idvSQL             * mStatistics;
    void               * mHBT;        /* for HBT */
    iduListNode          mNode;
} rpcDDLReplInfo;

class rpcResourceManager;

class rpcDDLSyncManager
{
/* Variable */
public:
    static IDTHREAD rpcResourceManager   * mResourceMgr;

private:

    iduMutex  mDDLSyncInfoListLock;
    iduMutex  mDDLExecutorListLock;

    iduList   mDDLSyncInfoList;
    iduList   mDDLExecutorList;

/* Function */
public:
    IDE_RC initialize( void );
    void   finalize( void );

    IDE_RC ddlSyncBegin( qciStatement       * aQciStatement, 
                         rpcResourceManager * aResourceMgr );

    IDE_RC ddlSyncEnd( smiTrans * aDDLTrans );

    IDE_RC ddlSyncBeginInternal( idvSQL              * aStatistics,
                                 cmiProtocolContext  * aProtocolContext,
                                 smiTrans            * aDDLTrans,
                                 idBool              * aExitFlag,
                                 rpdVersion          * aVersion,
                                 SChar               * aRepName,
                                 rpcResourceManager  * aResourceMgr,
                                 SChar              ** aUserName,
                                 SChar              ** aSql );

    IDE_RC ddlSyncEndInternal( smiTrans * aDDLTrans );

    IDE_RC realizeDDLExecutor( idvSQL * aStatistics );

    IDE_RC recvDDLInfoAndCreateDDLExecutor( cmiProtocolContext * aProtocolContext, rpdVersion * aVersion );

    rpcDDLSyncInfo * findDDLSyncInfoByTableOIDWithoutLock( smOID aTableOID );

    rpcDDLReplInfo * findDDLReplInfoByName( SChar * aRepName );

    void   setIsBuildNewMeta( smOID aTableOID, idBool aIsBuild );

    void   setDDLSyncCancelEvent( SChar * aRepName );

    void   setDDLSyncInfoCancelEvent( SChar * aRepName );

    void   setDDLExecutorCancelEvent( SChar * aRepName );

    IDE_RC recvAndSetDDLSyncCancel( cmiProtocolContext * aProtocolContext, idBool * aExitFlag );

    static IDE_RC runDDL( idvSQL   * aStatistics,
                          SChar    * aSql,
                          SChar    * aUserName,
                          smiStatement * aSmiStmt );

    static IDE_RC checkDDLSyncRemoteVersion( rpdVersion * aVersion );

    void   ddlSyncException( qciStatement * aQciStatement,
                             smiTrans     * aDDLTrans );

    void   sendDDLSyncCancel( rpcDDLSyncInfo * aDDLSyncInfo );
    void   restoreTableInfo( smiTrans * aTrans, rpcDDLTableInfo * aDDLTableInfo );
    void   removeReceiverNewMeta( rpcDDLSyncInfo * aDDLSyncInfo );
    void   destroyDDLSyncInfo( rpcDDLSyncInfo * aDDLSyncInfo );
    void   removeFromDDLSyncInfoList( rpcDDLSyncInfo * aDDLSyncInfo );
    IDE_RC setSkipUpdateXSN( rpcDDLSyncInfo * aDDLSyncInfo, idBool aIsSkip );
    IDE_RC recoverTableAccessOption( idvSQL * aStatistics, rpcDDLSyncInfo * aDDLSyncInfo );
    IDE_RC ddlSyncRollback( smiTrans * aDDLTrans, rpcDDLSyncInfo * aDDLSyncInfo );

    IDE_RC processDDLSyncMsgAck( rpcDDLSyncInfo       * aDDLSyncInfo, 
                                 RP_DDL_SYNC_MSG_TYPE   aMsgType,
                                 rpcDDLResult           aResult,
                                 SChar                * aErrMsg );

private:
    IDE_RC ddlSyncEndCommon( smiTrans * aDDLTrans, rpcDDLSyncInfo * aDDLSyncInfo );

    IDE_RC allocAndInitDDLSyncInfo( rpcDDLSyncInfoType    aDDLSyncInfoType, 
                                    rpcDDLSyncInfo     ** aDDLSyncInfo );

    IDE_RC setDDLSyncInfoAndTableAccessReadOnly( qciStatement * aQciStatement, rpcDDLSyncInfo * aDDLSyncInfo );
    IDE_RC setDDLSyncInfoAndTableAccessReadOnlyInternal( idvSQL              * aStatistics,
                                                         cmiProtocolContext  * aProtocolContext,
                                                         idBool              * aExitFlag,
                                                         rpdVersion          * aVersion,
                                                         SChar               * aRepName,
                                                         rpcDDLSyncInfo      * aDDLSyncInfo,
                                                         SChar              ** aSql );

    IDE_RC waitLastProcessedSN( rpcDDLSyncInfo * aDDLSyncInfo );

    IDE_RC setDDLSyncRollbackInfoAndTableAccessRecoverCommon( smiTrans * aDDLTrans, rpcDDLSyncInfo * aDDLSyncInfo );
    IDE_RC setDDLSyncRollbackInfoAndTableAccessRecover( smiTrans * aDDLTrans, rpcDDLSyncInfo * aDDLSyncInfo );
    IDE_RC setDDLSyncRollbackInfoAndTableAccessRecoverInternal( idvSQL         * aStatistics,
                                                                smiTrans       * aDDLTrans, 
                                                                rpcDDLSyncInfo * aDDLSyncInfo );

    IDE_RC readyDDLSyncCommon( cmiProtocolContext * aProtocolContext,
                               smiTrans           * aTrans,
                               qciTableInfo       * aTableInfo,
                               UInt                 aDDLSyncPartInfoCount,
                               SChar                aDDLSyncPartInfoNames[][QC_MAX_OBJECT_NAME_LEN + 1],
                               rpcDDLSyncInfo     * aDDLSyncInfo,
                               SChar              * aRepName,
                               UInt                 aRepCount );
    IDE_RC readyDDLSync( qciStatement   * aQciStatement,
                         smiTrans       * aTrans,
                         qciTableInfo   * aTableInfo,
                         rpcDDLSyncInfo * aDDLSyncInfo );
    IDE_RC readyDDLSyncInternal( cmiProtocolContext  * aProtocolContext,
                                 smiTrans            * aTrans,
                                 idBool              * aExitFlag,
                                 rpdVersion          * aVersion,
                                 SChar               * aRepName,
                                 rpcDDLSyncInfo      * aDDLSyncInfo,
                                 SChar              ** aSql );

    IDE_RC buildReceiverNewMeta( smiTrans * aTrans, rpcDDLSyncInfo * aDDLSyncInfo );

    void   cancelAndWaitAllDDLSync();
    void   finalizeDDLExcutorList();

    IDE_RC ddlSyncTableLock( smiTrans       * aTrans, 
                             rpcDDLSyncInfo * aDDLSyncInfo,
                             ULong            aTimeout );



    void   addToDDLExecutorList( void * aDDLExecutor );

    void   destroyDDLReplInfoList( rpcDDLSyncInfo * aDDLSyncInfo );

    IDE_RC addToDDLSyncInfoList( cmiProtocolContext * aProtocolContext,
                                 smiTrans           * aTrans,
                                 rpcDDLSyncInfo     * aDDLSyncInfo, 
                                 SChar              * aReplName,
                                 SInt                 aMaxReplCount );
    
    IDE_RC checkAlreadyDDLSync( smiTrans * aTrans, rpcDDLSyncInfo * aDDLSyncInfo );

    IDE_RC processForWaitingGap( qciStatement   * aQciStatement,
                                 smiTrans       * aDDLTrans,
                                 rpcDDLSyncInfo * aDDLSyncInfo );

    void   setDDLTableInfo( rpcDDLTableInfo * aDDLTableInfo,
                            qciTableInfo    * aTableInfo,
                            UInt              aTablePartInfoCount,
                            iduList         * aDDLSyncPartInfoList );

    IDE_RC getPartitionNamesByOID( smiTrans * aTrans,
                                   smOID    * aPartTableOIDs,
                                   UInt       aDDLSyncPartInfoCount,
                                   SChar      aDDLSyncPartInfoNames[][QC_MAX_OBJECT_NAME_LEN + 1] );

    idBool isDDLSyncPartition( qciTableInfo * aTableInfo, 
                               UInt           aDDLSyncPartInfoCount,
                               SChar          aDDLSyncPartInfoNames[][QC_MAX_OBJECT_NAME_LEN + 1] );

    IDE_RC getDDLSyncPartInfoList( smiTrans * aTrans,
                                   UInt       aTableID,
                                   UInt     * aTablePartInfoCount,
                                   UInt       aDDLSyncPartInfoCount,
                                   SChar      aDDLSyncPartInfoNames[][QC_MAX_OBJECT_NAME_LEN + 1],
                                   iduList  * aDDLSyncPartInfoList );
    void   freePartInfoList( iduList * aPartInfoList );

    IDE_RC addDDLTableInfo( SChar           * aPartName,
                            UInt              aTableID,
                            smOID             aTableOID,
                            qciAccessOption   aAccessOption,
                            idBool            aIsDDLSyncTable,
                            iduList         * aPartInfoList );


    IDE_RC setTableAccessOption( smiTrans        * aTrans,
                                 rpcDDLTableInfo * aDDLTableInfo,
                                 qciAccessOption   aAccessOption );
    IDE_RC runAccessTable( smiTrans        * aTrans,
                           UInt              aUserID,
                           SChar           * aTableName,
                           SChar           * aPartName,
                           qciAccessOption   aAccessOption );

    IDE_RC createDDLSyncInfo( smiTrans            * aTrans,
                              rpcDDLSyncInfoType    aDDLSyncInfoType,
                              qciTableInfo        * aTableInfo,
                              UInt                  aDDLSyncPartInfoCount,
                              SChar                 aDDLSyncPartInfoNames[][QC_MAX_OBJECT_NAME_LEN + 1],
                              rpcDDLSyncInfo     ** aDDLSyncInfo );

    IDE_RC setDDLSyncInfo( smiTrans       * aTrans,
                           rpcDDLSyncInfo * aDDLSyncInfo,
                           qciTableInfo   * aTableInfo,
                           UInt             aDDLSyncPartInfoCount,
                           SChar            aDDLSyncPartInfoNames[][QC_MAX_OBJECT_NAME_LEN + 1] );

    IDE_RC createAndAddDDLReplInfoList( cmiProtocolContext * aProtocolContext,
                                        smiTrans           * aTrans,
                                        rpcDDLSyncInfo     * aDDLSyncInfo, 
                                        SChar              * aRepName,
                                        SInt                 aReplCount );

    IDE_RC createAndAddDDLReplInfo( cmiProtocolContext * aProtocolContext,
                                    smiStatement       * aSmiStmt,
                                    rpcDDLSyncInfo     * aDDLSyncInfo,
                                    rpdReplications    * aReplications ); 

    IDE_RC checkAndAddReplInfo( idvSQL             * aStatistics,
                                cmiProtocolContext * aProtocolContext, 
                                smiStatement       * aSmiStmt,
                                rpcDDLSyncInfo     * aDDLSyncInfo,
                                rpdMetaItem        * aReplMetaItems,
                                rpdReplications    * aReplication );

    IDE_RC addDDLReplInfo( idvSQL             * aStatistics,
                           cmiProtocolContext * aProtocolContext,
                           rpcDDLSyncInfo     * aDDLSyncInfo,
                           rpdReplHosts       * aReplHosts );

    IDE_RC sendDDLSyncInfo( rpcDDLSyncInfo  * aDDLSyncInfo,
                            SChar           * aSql,
                            UInt              aDDLSyncPartInfoCount,
                            SChar             aDDLSyncPartInfoNames[][QC_MAX_OBJECT_NAME_LEN + 1] );
    IDE_RC recvDDLSyncInfo( cmiProtocolContext   * aProtocolContext,
                            idBool               * aExitFlag,
                            rpdVersion           * aVersion,
                            SChar                * aRepName,
                            SChar                * aUserName,
                            SChar                * aTableName,
                            UInt                 * aDDLSyncPartInfoCount,
                            SChar                  aDDLSyncPartInfoNames[][QC_MAX_OBJECT_NAME_LEN + 1],
                            SChar               ** aSql );

    IDE_RC sendDDLSyncInfoAck( cmiProtocolContext   * aProtocolContext,
                               idBool               * aExitFlag,
                               rpcDDLResult           aResult,
                               SChar                * aErrMsg );
    IDE_RC recvDDLSyncInfoAck( rpcDDLSyncInfo * aDDLSyncInfo ); 

    IDE_RC processDDLSyncMsg( rpcDDLSyncInfo * aDDLSyncInfo, RP_DDL_SYNC_MSG_TYPE aMsgType );
    IDE_RC sendDDLSyncMsg( rpcDDLSyncInfo * aDDLSyncInfo, RP_DDL_SYNC_MSG_TYPE aMsgType );
    IDE_RC recvDDLSyncMsg( rpcDDLSyncInfo * aDDLSyncInfo, RP_DDL_SYNC_MSG_TYPE aMsgType );

    IDE_RC sendDDLSyncMsgAck( rpcDDLSyncInfo       * aDDLSyncInfo, 
                              RP_DDL_SYNC_MSG_TYPE   aMsgType,
                              rpcDDLResult           aResult,
                              SChar                * aErrMsg );
    IDE_RC recvDDLSyncMsgAck( rpcDDLSyncInfo * aDDLSyncInfo, RP_DDL_SYNC_MSG_TYPE   aMsgType );

    IDE_RC connect( rpcDDLReplInfo * aInfo, cmiProtocolContext ** aProtocolContext );
    IDE_RC connectForDDLSync( rpcDDLSyncInfo * aDDLSyncInfo );

    IDE_RC createAndStartDDLExecutor( cmiProtocolContext * aProtocolContext, rpdVersion * aVersion ); 

    IDE_RC rebuildStatement( qciStatement * aQciStatement );

    rpxDDLExecutor * findDDLExecutorByNameWithoutLock( SChar * aRepName );

    rpcDDLReplInfo * findDDLReplInfoByNameWithoutLock( SChar * aRepName );

    IDE_RC getTableInfo( idvSQL        * aStatistics,
                         smiTrans      * aTrans,
                         SChar         * aTableName,
                         SChar         * aUserName,
                         ULong           aTimeout,
                         qciTableInfo ** aTableInfo );

    void   addErrorList( SChar * aDest,
                         SChar * aRepName,
                         SChar * aErrMsg );

    void   addErrorRepList( SChar * aDest, SChar * aSrc );
    void   destroyProtocolContext( cmiProtocolContext * aProtocolContext );
    
    idBool duplicateDDLReplInfo( rpcDDLSyncInfo     * aDDLSyncInfo, 
                                 rpdReplHosts       * aReplHosts );
};
#endif
