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
 * $Id$
 **********************************************************************/
#ifndef  _O_QDSD_H_
#define  _O_QDSD_H_ 1

#include <qc.h>
#include <qdParseTree.h>
#include <sdi.h>
#include <sdiZookeeper.h>

typedef enum
{
    QDSD_REPL_QUERY_TYPE_CREATE = 0,
    QDSD_REPL_QUERY_TYPE_DROP,
    QDSD_REPL_QUERY_TYPE_STOP,
    QDSD_REPL_QUERY_TYPE_ADD,
    QDSD_REPL_QUERY_TYPE_START,
    QDSD_REPL_QUERY_TYPE_FLUSH
} qdsdReplQueryType;

class qdsd
{
public:
    static IDE_RC validateShardAdd( qcStatement * aStatement );
    
    static IDE_RC executeShardAdd( qcStatement * aStatement );

    static IDE_RC executeShardAddForClone( qcStatement * aStatement );
    
    static IDE_RC synchronizeKSafetyInfo( qcStatement * aStatement );
    
    static IDE_RC validateShardJoin( qcStatement * aStatement );
    
    static IDE_RC executeShardJoin( qcStatement * aStatement );

    static IDE_RC validateShardDrop( qcStatement * aStatement );
    
    static IDE_RC executeShardDrop( qcStatement * aStatement );

    static IDE_RC validateShardFailover( qcStatement * aStatement );

    static IDE_RC executeShardFailover( qcStatement * aStatement );    

    static IDE_RC validateShardFailback( qcStatement * aStatement );
    
    static IDE_RC executeShardFailback( qcStatement * aStatement );

    static IDE_RC validateShardMove( qcStatement * aStatement );
    
    static IDE_RC executeShardMove( qcStatement * aStatement );
    
    static IDE_RC validateCommon( qcStatement * aStatement );

    static IDE_RC validateCommonForExecute( qcStatement * aStatement );
    
    static IDE_RC executeSetNodes( qcStatement * aStatement );
    static IDE_RC executeUnsetNodes( qcStatement * aStatement );

    static IDE_RC getAllNodeInfoList( qcStatement * aStatement );
    
    static IDE_RC executeTemporaryReplication( qcStatement * aStatement );

    static IDE_RC executeReplicationFunction( qcStatement       * aStatement,
                                              qdsdReplQueryType   aReplQueryType );

    static IDE_RC createReplicationForInternal( qcStatement * aStatement,
                                                SChar       * aNodeName,
                                                SChar       * aReplName,
                                                SChar       * aHostIP,
                                                UInt          aPortNo );

    static IDE_RC createReverseReplication( qcStatement * aStatement,
                                            SChar       * aNodeName,
                                            SChar       * aReplName,
                                            SChar       * aOtherNodeName,
                                            SChar       * aHostIP,
                                            UInt          aPortNo,
                                            sdiReplDirectionalRole aRole );
    
    static IDE_RC stopReplicationForInternal( qcStatement * aStatement,
                                              SChar       * aNodeName,
                                              SChar       * aReplName );

    static IDE_RC dropReplicationForInternal( qcStatement * aStatement,
                                              SChar       * aNodeName,
                                              SChar       * aReplName );

    static IDE_RC searchAndRunReplicaSet( qcStatement       * aStatement,
                                          sdiReplicaSetInfo * aReplicaSetInfo,
                                          UInt                aNth,
                                          qdsdReplQueryType   aReplQueryType);

    static IDE_RC addReplicationForInternal( qcStatement       * aStatement,
                                             sdiReplicaSetInfo * aReplicaSetInfo,
                                             sdiBackupOrder      aBackupOrder );
    
    static IDE_RC startReplicationForInternal( qcStatement * aStatement,
                                               SChar       * aNodeName,
                                               SChar       * aReplName );

    static IDE_RC syncReplicationForInternal( qcStatement * aStatement,
                                              SChar       * aNodeName,
                                              SChar       * aReplName );

    static IDE_RC truncateBackupTable( qcStatement * aStatement );
    
    static IDE_RC truncateBackupTableForInternal( qcStatement       * aStatement,
                                                  SChar             * aNodeName,
                                                  sdiReplicaSetInfo * aReplicaSetInfo );
    
    static IDE_RC findBackupReplicaSetInfo( SChar             * aNodeName,
                                            sdiReplicaSetInfo * aReplicaSetInfo,
                                            sdiReplicaSetInfo * aCurrReplicaSetInfo,
                                            sdiReplicaSetInfo * aPrevReplicaSetInfo,
                                            sdiReplicaSetInfo * aPrevPrevReplicaSetInfo );

    static IDE_RC runForNodeReplicationQuery( qcStatement       * aStatement,
                                              sdiReplicaSetInfo * aReplicaSetInfo,
                                              sdiReplicaSetInfo * aCurrReplicaSetInfo,
                                              sdiReplicaSetInfo * aPrevReplicaSetInfo,
                                              sdiReplicaSetInfo * aPrevPrevReplicaSetInfo,
                                              qdsdReplQueryType   aReplQueryType,
                                              idBool              aIsOld );

    static IDE_RC validateIsExistDeadNode( qcStatement * aStatement );

    static IDE_RC resetShardMeta( qcStatement * aStatement );
    
    static IDE_RC resetShardSequence( smiStatement * aSmiStmt,
                                      SChar        * aSeqName );

    static IDE_RC executeRemoteSQLWithNewTrans( qcStatement * aStatement,
                                                SChar       * aNodeName,
                                                SChar       * aSqlStr,
                                                idBool        aIsDDLTimeOut );
    
    static IDE_RC executeRemoteSQLWithNewTransArg( qcStatement         * aStatement,
                                                   SChar               * aNodeName,
                                                   sdiInternalOperation  /*aInternalOP*/,
                                                   qciStmtType           /*aSQLBufStmtType*/,
                                                   SChar               * aSQLBuf,
                                                   UInt                  aSQLBufSize,
                                                   idBool                aIsDDLTimeOut,
                                                   const SChar         * aSQLFormat,
                                                   ... );
    static IDE_RC changeBuckupNodeNameForFailOver( qcStatement       * aStatement,
                                                   sdiReplicaSetInfo * aReplicaSetInfo,
                                                   UInt                aNth);

    static IDE_RC flushReplicationForInternal( qcStatement * aStatement,
                                               SChar       * aNodeName,
                                               SChar       * aReplName );

    static IDE_RC executeReplicationForJoin( qcStatement * aStatement,
                                             idBool        aIsException );
    
    static IDE_RC checkZookeeperSMNAndDataSMN();
    static IDE_RC getProcedureKeyValue( qdReShardAttribute * aReshardAttr,
                                        SChar              * aProcKeyVal );

    static IDE_RC getShardMetaFromOtherNode( qcStatement * aStatement,
                                             SChar       * aNodeName );
    
private:
    /* zookeeper 메타 변경 함수 */
    static IDE_RC executeZookeeperAdd();

    static IDE_RC executeZookeeperJoin();

    static IDE_RC executeZookeeperDrop( qcStatement * aStatement );

    static IDE_RC executeZookeeperFailover( SChar   * aTargetNodeName,
                                            SChar   * aFailoverNodeName,
                                            idBool  * aIsTargetAlive );
    static IDE_RC executeZookeeperFailback();

    static IDE_RC getTableNames( qdReShardAttribute * aReShardAttr, 
                                 SChar * aSessionUserName,
                                 UInt    aNameBufSize, 
                                 SChar * aPrimaryTableName, 
                                 SChar * aBackupTableName );

    static IDE_RC cleanupReplWithNewTransCommit( qcStatement * aStatement, 
                                                 SChar * aNodeName, 
                                                 SChar * aReplName );
    static IDE_RC createReplWithNewTransCommit( qcStatement * aStatement, 
                                                SChar * aNodeName, 
                                                SChar * aReplName, 
                                                SChar * aIP, 
                                                UInt    aPort);
    static IDE_RC alterReplAddTableWithNewTrans( qcStatement * aStatement, 
                                                 SChar * aNodeName, 
                                                 SChar * aReplName, 
                                                 SChar * aFromTableName, 
                                                 SChar * aToTableName);

    static IDE_RC createReplicationsWithNewTrans( qcStatement * aStatement,
                                                  iduList * aNodeInfoList,
                                                  SChar * aFromNodeName,
                                                  SChar * aToNodeName,
                                                  SChar * aReplicationName );
    static IDE_RC prepareReplicationsForSync( qcStatement * aStatement,
                                              iduList * aNodeInfoList,
                                              SChar * aFromNodeName,
                                              SChar * aToNodeName,
                                              SChar * aReplicationName,
                                              qdReShardAttribute * aReShardAttr );
    static IDE_RC executeRemoteSQL( qcStatement          * aStatement, 
                                    SChar                * aNodeName,
                                    sdiInternalOperation   aInternalOP,
                                    qciStmtType            aSQLBufStmtType,
                                    SChar                * aSQLBuf,
                                    UInt                   aSQLBufSize,
                                    const SChar          * aSQLFormat,
                                    ... );
    static IDE_RC executeLocalSQL(qcStatement         * aStatement,
                                  qciStmtType           aSQLBufStmtType,
                                  SChar               * aSQLBuf,
                                  UInt                  aSQLBufSize,
                                  const SChar         * aSQLFormat,
                                  ... );

    static IDE_RC executeLocalSQLWithSession( qcStatement         * aStatement,
                                              qciStmtType           aSQLBufStmtType,
                                              SChar               * aSQLBuf,
                                              UInt                  aSQLBufSize,
                                              const SChar         * aSQLFormat,
                                              ... );

    static IDE_RC lockAllRemoteReshardTables( qcStatement * aStatement,
                                              SChar * aSQLBuf,
                                              UInt    aSQLBufSize,
                                              SChar * aFromNodeName,
                                              SChar * aToNodeName,
                                              qdReShardAttribute * aReShardAttr );

    static IDE_RC resetShardMetaAndSwapPartition( qcStatement * aStatement,
                                                  SChar * aSQLBuf,
                                                  UInt    aSQLBufSize,
                                                  SChar * aFromNodeName,
                                                  SChar * aToNodeName,
                                                  iduList * aNodeInfoList,
                                                  qdReShardAttribute * aReShardAttr );

    static IDE_RC moveReplication( qcStatement * aStatement,
                                   SChar       * aSQLBuf,
                                   UInt          aSQLBufSize,
                                   UInt          aKSafety,
                                   sdiReplicaSet  * aFromReplicaSet,
                                   sdiReplicaSet  * aToReplicaSet,
                                   qdReShardAttribute * aReShardAttr );

    static IDE_RC truncateAllBackupData( qcStatement * aStatement,
                                         SChar * aSQLBuf,
                                         UInt    aSQLBufSize,
                                         SChar * aNodeName,
                                         qdReShardAttribute * aReShardAttr,
                                         ZKPendingJobType  aPendingJobType );

    static IDE_RC truncateAllPrimaryData( qcStatement * aStatement,
                                          SChar * aSQLBuf,
                                          UInt    aSQLBufSize,
                                          SChar * aNodeName,
                                          qdReShardAttribute * aReShardAttr,
                                          ZKPendingJobType  aPendingJobType );

    static IDE_RC isExistBackupDataForAllReshardAttr( qcStatement * aStatement,
                                                      SChar * aNodeName,
                                                      qdReShardAttribute * aReShardAttr,
                                                      idBool  * aIsExist );
    static IDE_RC clearBackupDataWithNewTrans( void  * aOriMmSession,
                                               SChar * aNodeName,
                                               qdReShardAttribute * aReShardAttr );
    static IDE_RC getIncreasedSMN( qcStatement * aStatement, ULong * aOutNewSMN );
    static IDE_RC executeSQLNodeList( qcStatement        * aStatement,
                                      sdiInternalOperation aInternalOp,
                                      qciStmtType          aSQLStmtType,
                                      SChar              * aSQLBuf,
                                      SChar              * aExceptNodeName,
                                      iduList            * aList );

    static IDE_RC lockTables4Failover( qcStatement        * aStatement,
                                       SChar              * aSQLBuf,
                                       UInt                 aSQLBufSize,
                                       SChar              * aFailoverNodeName,
                                       sdiTableInfoList   * aTableInfoList,
                                       sdiReplicaSetInfo  * aReplicaSetInfo,
                                       sdiNode            * aTargetNodeInfo,
                                       ULong                aSMN );
    static IDE_RC unsetShardTableForDrop( qcStatement       * aStatement );

    static IDE_RC failoverTablePartitionSwap( qcStatement        * aStatement,
                                              SChar              * aFailoverNodeName,
                                              sdiTableInfoList   * aTableInfoList,
                                              sdiReplicaSetInfo  * aReplicaSetInfo,
                                              sdiNode            * aTargetNodeInfo,
                                              ULong                aSMN );

    static IDE_RC failoverResetShardMeta( qcStatement        * aStatement,
                                          SChar              * aTargetNodeName,
                                          SChar              * aFailoverNodeName,
                                          sdiTableInfoList   * aTableInfoList,
                                          sdiReplicaSetInfo  * aReplicaSetInfo,
                                          sdiNode            * aTargetNodeInfo,
                                          iduList            * aAliveNodeList,
                                          ULong                aSMN,
                                          idBool             * aIsUnsetNode );

    static IDE_RC failoverAddReplicationItems( qcStatement        * aStatement,
                                               SChar              * aTargetNodeName,
                                               SChar              * aFailoverNodeName,
                                               SChar              * aFailoverNextNodeName,
                                               sdiTableInfoList   * aTableInfoList,
                                               sdiReplicaSetInfo  * aReplicaSetInfo,
                                               sdiNode            * aTargetNodeInfo,
                                               ULong                aSMN );

    static IDE_RC failoverAddReverseReplicationItems( qcStatement        * aStatement,
                                                      SChar              * aTargetNodeName,
                                                      SChar              * aFailoverNodeName,
                                                      sdiTableInfoList   * aTableInfoList,
                                                      sdiReplicaSetInfo  * aReplicaSetInfo,
                                                      sdiNode            * aTargetNodeInfo,
                                                      ULong                aSMN );

    static IDE_RC failbackResetShardMeta( qcStatement        * aStatement,
                                          SChar              * aTargetNodeName,
                                          SChar              * aFailoverNodeName,
                                          sdiTableInfoList   * aTableInfoList,
                                          sdiReplicaSetInfo  * aReplicaSetInfo,
                                          sdiReplicaSetInfo  * aFailoverHistoryInfo,
                                          iduList            * aAliveNodeList,
                                          UInt                 aReplicaSetId,
                                          ULong                aSMN );

    static IDE_RC lockTables4Failback( qcStatement        * aStatement,
                                       SChar              * aSQLBuf,
                                       UInt                 aSQLBufSize,
                                       SChar              * aFailbackFromNodeName,
                                       sdiTableInfoList   * aTableInfoList,
                                       sdiReplicaSetInfo  * aReplicaSetInfo,
                                       sdiReplicaSetInfo  * aFailoverHistoryInfo,
                                       ULong                aSMN );

    static IDE_RC failbackTablePartitionSwap( qcStatement        * aStatement,
                                              SChar              * aFailbackFromNodeName,
                                              sdiTableInfoList   * aTableInfoList,
                                              sdiReplicaSetInfo  * aReplicaSetInfo,
                                              sdiReplicaSetInfo  * aFailoverHistoryInfo,
                                              ULong                aSMN );

    static IDE_RC failbackOrganizeReverseRP( qcStatement        * aStatement,
                                             SChar              * aFailbackFromNodeName,
                                             sdiTableInfoList   * aTableInfoList,
                                             sdiReplicaSetInfo  * aFailoverHistoryInfo,
                                             UInt                 aMainReplicaSetId,
                                             ULong                aSMN );

    static IDE_RC failbackOrganizeReverseRPAfterLock( qcStatement        * aStatement,
                                                      SChar              * aFailbackFromNodeName,
                                                      sdiReplicaSetInfo  * aFailoverHistoryInfo );

    static IDE_RC failbackRecoverRPbeforeLock( qcStatement        * aStatement,
                                               SChar              * aFailbackFromNodeName,
                                               sdiReplicaSetInfo  * aFailoverHistoryInfoWithOtherNode );

    static IDE_RC failbackRecoverRPAfterLock( qcStatement        * aStatement,
                                              SChar              * aFailbackFromNodeName,
                                              sdiReplicaSet      * aMainReplicaSet,
                                              sdiReplicaSetInfo  * aFailoverHistoryInfo,
                                              sdiReplicaSetInfo  * aFailoverHistoryInfoWithOtherNode );
    
    static IDE_RC failbackJoin( qcStatement        * aStatement,
                                SChar              * aFailbackFromNodeName,
                                sdiFailbackState     aFailbackState );

    static IDE_RC failbackFailedover( qcStatement        * aStatement,
                                      SChar              * aFailbackFromNodeName,
                                      ULong                aFailbackSMN );

    static IDE_RC joinRecoverRP( qcStatement        * aStatement,
                                 sdiReplicaSetInfo  * aReplicaSetInfo );
    
    /***********************************************************************
     * TASK-7307 DML Data Consistency in Shard
     ***********************************************************************/
    static IDE_RC executeAlterShardFlag( qcStatement        * aStatement );
    static IDE_RC executeAlterShardNone( qcStatement        * aStatement );
    
    static IDE_RC checkCloneTableDataAndTruncateForShardAdd( qcStatement        * aStatement,
                                                             qdReShardAttribute * aReShardAttr );

    static IDE_RC executeResetCloneMeta( qcStatement * aStatement );

    static IDE_RC checkShardPIN( qcStatement * aStatement );

    static IDE_RC syncFailbackData( qcStatement        * aStatement,
                                    SChar              * aFromNodeName,
                                    SChar              * aToNodeName,
                                    sdiTableInfoList   * aTableInfoList,
                                    sdiReplicaSetInfo  * aReplicaSetInfo,
                                    sdiReplicaSetInfo  * aFailoverHistoryInfo,
                                    ULong                aSMN,
                                    idBool               aIsCloneOnly );

    static IDE_RC syncFailbackDataAfterLock( qcStatement        * aStatement,
                                             SChar              * aFromNodeName,
                                             SChar              * aToNodeName );

    static IDE_RC syncDataWithReplicaSet( qcStatement        * aStatement,
                                          SChar              * aFromNodeName,
                                          sdiTableInfoList   * aTableInfoList,
                                          sdiReplicaSet      * aReplicaSet,
                                          ULong                aSMN );
};
#endif
