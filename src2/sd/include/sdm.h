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

#ifndef _O_SDM_H_
#define _O_SDM_H_ 1

#include <idl.h>
#include <sdi.h>
#include <qci.h>
#include <qcm.h>
#include <sdf.h>

/**************************************************************
                       BASIC DEFINITIONS
 **************************************************************/
#define SDM_MAX_META_INDICES       QCM_MAX_META_INDICES

#define SDM_USER                   ((SChar*) "SYS_SHARD")

#define SDM_SEQUENCE_NODE_NAME        ((SChar*) "NEXT_NODE_ID")
#define SDM_SEQUENCE_SHARD_NAME       ((SChar*) "NEXT_SHARD_ID")

#define SDM_DEFAULT_SHARD_NODE_ID        (0)
#define SDM_DEFAULT_SHARD_NODE_ID_STR    "0"

#define SDM_NA_STR                       ((SChar*)"$$N/A")
#define SDM_NA_STR_SIZE                  (5)
/**************************************************************
                       META TABLE NAMES
 **************************************************************/
#define SDM_VERSION                "VERSION_"
#define SDM_NODES                  "NODES_"
#define SDM_REPLICA_SETS           "REPLICA_SETS_"
#define SDM_FAILOVER_HISTORY_      "FAILOVER_HISTORY_"
#define SDM_OBJECTS                "OBJECTS_"
#define SDM_RANGES                 "RANGES_"
#define SDM_CLONES                 "CLONES_"
#define SDM_SOLOS                  "SOLOS_"
#define SDM_LOCAL_META_INFO        "LOCAL_META_INFO_"
#define SDM_GLOBAL_META_INFO       "GLOBAL_META_INFO_"

/**************************************************************
                       COLUMN ORDER
 **************************************************************/
#define SDM_VERSION_MAJOR_VER_COL_ORDER              (0)
#define SDM_VERSION_MINOR_VER_COL_ORDER              (1)
#define SDM_VERSION_PATCH_VER_COL_ORDER              (2)

#define SDM_NODES_NODE_ID_COL_ORDER                    (0)
#define SDM_NODES_NODE_NAME_COL_ORDER                  (1)
#define SDM_NODES_HOST_IP_COL_ORDER                    (2)
#define SDM_NODES_PORT_NO_COL_ORDER                    (3)
#define SDM_NODES_ALTERNATE_HOST_IP_COL_ORDER          (4)
#define SDM_NODES_ALTERNATE_PORT_NO_COL_ORDER          (5)
#define SDM_NODES_INTERNAL_HOST_IP_COL_ORDER           (6)
#define SDM_NODES_INTERNAL_PORT_NO_COL_ORDER           (7)
#define SDM_NODES_INTERNAL_ALTERNATE_HOST_IP_COL_ORDER (8)
#define SDM_NODES_INTERNAL_ALTERNATE_PORT_NO_COL_ORDER (9)
#define SDM_NODES_INTERNAL_CONNECT_TYPE                (10)
#define SDM_NODES_SMN_COL_ORDER                        (11)

#define SDM_REPLICA_SETS_REPLICA_SET_ID_COL_ORDER                 (0)
#define SDM_REPLICA_SETS_PRIMARY_NODE_NAME_COL_ORDER              (1)
#define SDM_REPLICA_SETS_FIRST_BACKUP_NODE_NAME_COL_ORDER         (2)
#define SDM_REPLICA_SETS_SECOND_BACKUP_NODE_NAME_COL_ORDER        (3)
#define SDM_REPLICA_SETS_STOP_FIRST_BACKUP_NODE_NAME_COL_ORDER    (4)
#define SDM_REPLICA_SETS_STOP_SECOND_BACKUP_NODE_NAME_COL_ORDER   (5)
#define SDM_REPLICA_SETS_FIRST_REPLICATION_NAME_COL_ORDER         (6)
#define SDM_REPLICA_SETS_FIRST_REPL_FROM_NODE_NAME_COL_ORDER      (7)
#define SDM_REPLICA_SETS_FIRST_REPL_TO_NODE_NAME_COL_ORDER        (8)
#define SDM_REPLICA_SETS_SECOND_REPLICATION_NAME_COL_ORDER        (9)
#define SDM_REPLICA_SETS_SECOND_REPL_FROM_NODE_NAME_COL_ORDER     (10)
#define SDM_REPLICA_SETS_SECOND_REPL_TO_NODE_NAME_COL_ORDER       (11)
#define SDM_REPLICA_SETS_SMN_COL_ORDER                            (12)

#define SDM_FAILOVER_HISTORY_REPLICA_SET_ID_COL_ORDER                (0)
#define SDM_FAILOVER_HISTORY_PRIMARY_NODE_NAME_COL_ORDER             (1)
#define SDM_FAILOVER_HISTORY_FIRST_BACKUP_NODE_NAME_COL_ORDER        (2)
#define SDM_FAILOVER_HISTORY_SECOND_BACKUP_NODE_NAME_COL_ORDER       (3)
#define SDM_FAILOVER_HISTORY_STOP_FIRST_BACKUP_NODE_NAME_COL_ORDER   (4)
#define SDM_FAILOVER_HISTORY_STOP_SECOND_BACKUP_NODE_NAME_COL_ORDER  (5)
#define SDM_FAILOVER_HISTORY_FIRST_REPLICATION_NAME_COL_ORDER        (6)
#define SDM_FAILOVER_HISTORY_FIRST_REPL_FROM_NODE_NAME_COL_ORDER     (7)
#define SDM_FAILOVER_HISTORY_FIRST_REPL_TO_NODE_NAME_COL_ORDER       (8)
#define SDM_FAILOVER_HISTORY_SECOND_REPLICATION_NAME_COL_ORDER       (9)
#define SDM_FAILOVER_HISTORY_SECOND_REPL_FROM_NODE_NAME_COL_ORDER    (10)
#define SDM_FAILOVER_HISTORY_SECOND_REPL_TO_NODE_NAME_COL_ORDER      (11)
#define SDM_FAILOVER_HISTORY_SMN_COL_ORDER                           (12)

#define SDM_OBJECTS_SHARD_ID_COL_ORDER                         (0)
#define SDM_OBJECTS_USER_NAME_COL_ORDER                        (1)
#define SDM_OBJECTS_OBJECT_NAME_COL_ORDER                      (2)
#define SDM_OBJECTS_OBJECT_TYPE_COL_ORDER                      (3)
#define SDM_OBJECTS_SPLIT_METHOD_COL_ORDER                     (4)
#define SDM_OBJECTS_KEY_COLUMN_NAME_COL_ORDER                  (5)
#define SDM_OBJECTS_SUB_SPLIT_METHOD_COL_ORDER                 (6)
#define SDM_OBJECTS_SUB_KEY_COLUMN_NAME_COL_ORDER              (7)
#define SDM_OBJECTS_DEFAULT_NODE_ID_COL_ORDER                  (8)
#define SDM_OBJECTS_DEFAULT_PARTITION_ID_COL_ORDER             (9)
#define SDM_OBJECTS_DEFAULT_PARTITION_REPLICA_SET_ID_COL_ORDER (10)
#define SDM_OBJECTS_SMN_COL_ORDER                              (11)

#define SDM_RANGES_SHARD_ID_COL_ORDER                (0)
#define SDM_RANGES_PARTITION_NAME_COL_ORDER          (1)
#define SDM_RANGES_VALUE_ID_COL_ORDER                (2)
#define SDM_RANGES_SUB_VALUE_ID_COL_ORDER            (3)
#define SDM_RANGES_NODE_ID_COL_ORDER                 (4)
#define SDM_RANGES_SMN_COL_ORDER                     (5)
#define SDM_RANGES_REPLICA_SET_ID_COL_ORDER          (6)

#define SDM_CLONES_SHARD_ID_COL_ORDER                (0)
#define SDM_CLONES_NODE_ID_COL_ORDER                 (1)
#define SDM_CLONES_SMN_COL_ORDER                     (2)
#define SDM_CLONES_REPLICA_SET_ID_COL_ORDER          (3)

#define SDM_SOLOS_SHARD_ID_COL_ORDER                 (0)
#define SDM_SOLOS_NODE_ID_COL_ORDER                  (1)
#define SDM_SOLOS_SMN_COL_ORDER                      (2)
#define SDM_SOLOS_REPLICA_SET_ID_COL_ORDER           (3)


#define SDM_LOCAL_META_INFO_SHARD_NODE_ID_COL_ORDER                 (0)
#define SDM_LOCAL_META_INFO_SHARDED_DB_NAME_COL_ORDER               (1)
#define SDM_LOCAL_META_INFO_NODE_NAME_COL_ORDER                     (2)
#define SDM_LOCAL_META_INFO_HOST_IP_COL_ORDER                       (3)
#define SDM_LOCAL_META_INFO_PORT_NO_COL_ORDER                       (4)
#define SDM_LOCAL_META_INFO_INTERNAL_HOST_IP_COL_ORDER              (5)
#define SDM_LOCAL_META_INFO_INTERNAL_PORT_NO_COL_ORDER              (6)
#define SDM_LOCAL_META_INFO_INTERNAL_REPLICATION_HOST_IP_COL_ORDER  (7)
#define SDM_LOCAL_META_INFO_INTERNAL_REPLICATION_PORT_NO_COL_ORDER  (8)
#define SDM_LOCAL_META_INFO_INTERNAL_CONN_TYPE_COL_ORDER            (9)
#define SDM_LOCAL_META_INFO_K_SAFETY_COL_ORDER            (10)
#define SDM_LOCAL_META_INFO_REPLICATION_MODE_COL_ORDER            (11)
#define SDM_LOCAL_META_INFO_PARALLEL_COUNT_COL_ORDER            (12)


#define SDM_GLOBAL_META_INFO_ID_COL_ORDER                   (0)
#define SDM_GLOBAL_META_INFO_SHARD_META_NUMBER_COL_ORDER    (1)

/**************************************************************
                       INDEX ORDER
 **************************************************************/
#define SDM_NODES_IDX1_ORDER               (0)
#define SDM_NODES_IDX2_ORDER               (1)
#define SDM_NODES_IDX3_ORDER               (2)
#define SDM_NODES_IDX4_ORDER               (3)
#define SDM_NODES_IDX5_ORDER               (4)
#define SDM_NODES_IDX6_ORDER               (5)
#define SDM_NODES_IDX7_ORDER               (6)
#define SDM_NODES_IDX8_ORDER               (7)

#define SDM_REPLICA_SETS_IDX1_ORDER        (0) /* replica_set_id */
#define SDM_REPLICA_SETS_IDX2_ORDER        (1) /* primary_node_name */

#define SDM_FAILOVER_HISTORY_IDX1_ORDER    (0) /* Replica_set_id */
#define SDM_FAILOVER_HISTORY_IDX2_ORDER    (1) /* Primary_node_name */
#define SDM_FAILOVER_HISTORY_IDX3_ORDER    (2) /* SMN */

#define SDM_OBJECTS_IDX1_ORDER             (0)
#define SDM_OBJECTS_IDX2_ORDER             (1)

#define SDM_RANGES_IDX1_ORDER              (0)

#define SDM_CLONES_IDX1_ORDER              (0)

#define SDM_SOLOS_IDX1_ORDER               (0)

/* TASK-7307 DML Data Consistency in Shard */
#define SDM_CALLED_BY_SHARD_PKG            (0)
#define SDM_CALLED_BY_SHARD_MOVE           (1)
#define SDM_CALLED_BY_SHARD_FAILBACK       (2)

typedef enum
{
    SDM_SORT_ASC,
    SDM_SORT_DESC
}sdmSortOrder;

typedef enum
{
    SDM_REPL_SENDER,
    SDM_REPL_RECEIVER,
    SDM_REPL_CLONE
}sdmReplDirectionalRole;

/**************************************************************
                  CLASS & STRUCTURE DEFINITION
 **************************************************************/

class sdm
{
private:
    static IDE_RC checkMetaVersion( smiStatement * aSmiStmt );

    // hash, range, list의 모든 distribution value meta handle을 읽는다.
    static IDE_RC getMetaTableAndIndex( smiStatement * aSmiStmt,
                                        const SChar  * aMetaTableName,
                                        const void  ** aTableHandle,
                                        const void  ** aIndexHandle );

    static IDE_RC getNextNodeID( qcStatement * aStatement,
                                 UInt        * aNodeID,
                                 ULong         aSMN );

    static IDE_RC searchNodeID( smiStatement * aSmiStmt,
                                SInt           aNodeID,
                                ULong          aSMN,
                                idBool       * aExist );

    static IDE_RC getNextShardID( qcStatement * aStatement,
                                  UInt        * aShardID,
                                  ULong         aSMN );

    static IDE_RC searchShardID( smiStatement * aSmiStmt,
                                 SInt           aShardID,
                                 ULong          aSMN,
                                 idBool       * aExist );
    
    static IDE_RC getRange( qcStatement  * aStatement,
                            smiStatement * aSmiStmt,
                            ULong          aSMN,
                            sdiTableInfo * aTableInfo,
                            sdiRangeInfo * aRangeInfo,
                            idBool         aNeedMerge );

    static IDE_RC getClone( qcStatement  * aStatement,
                            smiStatement * aSmiStmt,
                            ULong          aSMN,
                            sdiTableInfo * aTableInfo,
                            sdiRangeInfo * aRangeInfo );

    static IDE_RC getSolo( qcStatement  * aStatement,
                           smiStatement * aSmiStmt,
                           ULong          aSMN,
                           sdiTableInfo * aTableInfo,
                           sdiRangeInfo * aRangeInfo );
public:
    static IDE_RC checkIsInCluster( qcStatement  * aStatement,
                                    idBool       * aIsInCluster );

    static IDE_RC getNodeByName( smiStatement * aSmiStmt,
                                 SChar        * aNodeName,
                                 ULong          aSMN,
                                 sdiNode      * aNode );

    /* PROJ-2701 Online data rebuild */
    static IDE_RC makeShardMeta4NewSMN( qcStatement * aStatement );

    // shard meta create
    static IDE_RC createMeta( qcStatement * aStatement );
    static IDE_RC resetMetaNodeId( qcStatement * aStatement, sdiLocalMetaInfo * aLocalMetaInfo );

    // shard node info
    static IDE_RC insertNode( qcStatement * aStatement,
                              UInt        * aNodeID,
                              SChar       * aNodeName,
                              UInt          aPortNo,
                              SChar       * aHostIP,
                              UInt          aAlternatePortNo,
                              SChar       * aAlternateHostIP,
                              UInt          aConnType,
                              UInt        * aRowCnt );

    static IDE_RC deleteNode( qcStatement * aStatement,
                              SChar       * aNodeName,
                              UInt        * aRowCnt );

    static IDE_RC updateNodeInternal( qcStatement * aStatement,
                                      SChar       * aNodeName,
                                      SChar       * aHostIP,
                                      UInt          aPortNo,
                                      SChar       * aAlternateHostIP,
                                      UInt          aAlternatePortNo,
                                      UInt          aConnType,
                                      UInt        * aRowCnt );

    static IDE_RC updateNodeExternal( qcStatement * aStatement,
                                      SChar       * aNodeName,
                                      SChar       * aHostIP,
                                      UInt          aPortNo,
                                      SChar       * aAlternateHostIP,
                                      UInt          aAlternatePortNo,
                                      UInt        * aRowCnt );

    static IDE_RC checkReferenceObj( qcStatement * aStatement,
                                     SChar       * aNodeName,
                                     ULong         aSMN );

    static IDE_RC insertReplicaSet( qcStatement * aStatement,
                                    UInt          aNodeID,
                                    SChar       * aPrimaryNodeName,
                                    UInt        * aRowCnt );

    static IDE_RC reorganizeReplicaSet( qcStatement * aStatement,
                                        SChar       * aPrimaryNodeName,
                                        UInt        * aRowCnt );
    static IDE_RC checkAndUpdateBackupInfo( qcStatement * aStatement,
                                            SChar * aNewPrimaryNodeName,
                                            SChar * aNew1stBackupNodeName,
                                            SChar * aNew2ndBackupNodeName,
                                            sdiReplicaSet * aOldReplicaSet,
                                            idBool         * aOutIsUpdated );
    static IDE_RC deleteReplicaSet( qcStatement * aStatement,
                                    SChar       * aPrimaryNodeName,
                                    idBool        aIsForce,
                                    UInt        * aRowCnt );

    static IDE_RC updateLocalMetaInfo(  qcStatement  * aStatement,
                                        sdiShardNodeId aShardNodeID,
                                        SChar        * aNodeName,
                                        SChar        * aHostIP,
                                        UInt           aPortNo,
                                        SChar        * aInternalHostIP,
                                        UInt           aInternalPortNo,
                                        SChar        * aInternalReplHostIP,
                                        UInt           aInternalReplPortNo,
                                        UInt           aInternalConnType,
                                        UInt         * aRowCnt );

    static IDE_RC  updateLocalMetaInfoForReplication( qcStatement  * aStatement,
                                                      UInt           aKSafety,
                                                      SChar        * aReplicationMode,
                                                      UInt           aParallelCount,
                                                      UInt         * aRowCnt );


    static IDE_RC insertProcedure( qcStatement * aStatement,
                                   SChar       * aUserName,
                                   SChar       * aProcName,
                                   SChar       * aSplitMethod,
                                   SChar       * aKeyParamName,
                                   SChar       * aSubSplitMethod,
                                   SChar       * aSubKeyParamName,
                                   SChar       * aDefaultNodeName,
                                   UInt        * aRowCnt,
                                   qsOID         aProcOID,
                                   UInt          aUserID );

    static IDE_RC insertTable( qcStatement * aStatement,
                               SChar       * aUserName,
                               SChar       * aTableName,
                               SChar       * aSplitMethod,
                               SChar       * aKeyColumnName,
                               SChar       * aSubSplitMethod,
                               SChar       * aSubKeyColumnName,
                               SChar       * aDefaultNodeName,
                               SChar       * aDefaultPartitionName,
                               UInt        * aRowCnt );

    static IDE_RC deleteObject( qcStatement * aStatement,
                                SChar       * aUserName,
                                SChar       * aTableName,
                                UInt        * aRowCnt );

    static IDE_RC updateDefaultNodeAndPartititon( qcStatement         * aStatement,
                                                  SChar               * aUserName,
                                                  SChar               * aTableName,
                                                  SChar               * aDefaultNodeName,
                                                  SChar               * aDefaultPartitionName,
                                                  UInt                * aRowCnt,
                                                  sdiInternalOperation  aInternalOP = SDI_INTERNAL_OP_NOT );

    static IDE_RC deleteObjectByID( qcStatement * aStatement,
                                    UInt          aShardID,
                                    UInt        * aRowCnt );

    static IDE_RC deleteOldSMN( qcStatement * aStatement,
                                ULong       * aSMN );
    static IDE_RC updateSMN( qcStatement * aStatement,
                             ULong         aSMN );

    // shard distribution info
    static IDE_RC insertRange( qcStatement  * aStatement,
                               SChar        * aUserName,
                               SChar        * aTableName,
                               SChar        * aPartitionName,
                               SChar        * aValue,
                               SChar        * aSubValue,
                               SChar        * aNodeName,
                               SChar        * aSetFuncType,
                               UInt         * aRowCnt );

    static IDE_RC updateRange( qcStatement         * aStatement,
                               SChar               * aUserName,
                               SChar               * aTableName,
                               SChar               * aOldNodeName,
                               SChar               * aNewNodeName,
                               SChar               * aPartitionName,
                               SChar               * aValue,
                               SChar               * aSubValue,
                               UInt                * aRowCnt,
                               sdiInternalOperation  aInternalOP = SDI_INTERNAL_OP_NOT );

    static IDE_RC insertClone( qcStatement  * aStatement,
                               SChar        * aUserName,
                               SChar        * aTableName,
                               SChar        * aNodeName,
                               UInt           aReplicaSetId,
                               UInt         * aRowCnt,
                               sdiInternalOperation  aInternalOP = SDI_INTERNAL_OP_NOT );


    static IDE_RC insertSolo( qcStatement  * aStatement,
                              SChar        * aUserName,
                              SChar        * aTableName,
                              SChar        * aNodeName,
                              UInt         * aRowCnt );

    static IDE_RC getExternalNodeInfo( smiStatement * aSmiStmt,
                                       sdiNodeInfo  * aNodeInfo,
                                       ULong          aSMN );

    static IDE_RC getInternalNodeInfo( smiStatement * aSmiStmt,
                                       sdiNodeInfo  * aNodeInfo,
                                       ULong          aSMN );

    static IDE_RC getInternalNodeInfoSortedByName( smiStatement * aStatement,
                                                   sdiNodeInfo  * aNodeInfo,
                                                   ULong          aSMN,
                                                   sdmSortOrder   aOrder );

    static IDE_RC getAllReplicaSetInfoSortedByPName( smiStatement       * aSmiStmt,
                                                     sdiReplicaSetInfo  * aNodeInfo,
                                                     ULong                aSMN );

    static IDE_RC getReplicaSetsByPName( smiStatement       * aSmiStmt,
                                         SChar              * aPrimaryNodeName,
                                         ULong                aSMN,
                                         sdiReplicaSetInfo  * aReplicaSetsInfo );

    static IDE_RC getReplicaSetsByReplicaSetId( smiStatement       * aSmiStmt,
                                                UInt                 aReplicaSetId,
                                                ULong                aSMN,
                                                sdiReplicaSetInfo  * aReplicaSetsInfo );

    static IDE_RC getNodeByID( smiStatement * aSmiStmt,
                               UInt           aNodeID,
                               ULong          aSMN,
                               sdiNode      * aNode );

    static IDE_RC getTableInfo( smiStatement * aSmiStmt,
                                SChar        * aUserName,
                                SChar        * aTableName,
                                ULong          aSMN,
                                sdiTableInfo * aTableInfo,
                                idBool       * aIsTableFound );

    static IDE_RC getTableInfoByID( smiStatement * aSmiStmt,
                                    UInt           aShardID,
                                    ULong          aSMN,
                                    sdiTableInfo * aTableInfo,
                                    idBool       * aIsTableFound );

    static IDE_RC getRangeInfo( qcStatement  * aStatement,
                                smiStatement * aSmiStmt,
                                ULong          aSMN,
                                sdiTableInfo * aTableInfo,
                                sdiRangeInfo * aRangeInfo,
                                idBool         aNeedMerge );

    /* PROJ-2655 Composite shard key */
    static IDE_RC shardRangeSort( sdiSplitMethod   aSplitMethod,
                                  UInt             aKeyDataType,
                                  idBool           aSubKeyExists,
                                  sdiSplitMethod   aSubSplitMethod,
                                  UInt             aSubKeyDataType,
                                  sdiRangeInfo   * aRangeInfo );

    static IDE_RC shardEliminateDuplication( sdiTableInfo * aTableInfo,
                                             sdiRangeInfo * aRangeInfo );

    static IDE_RC compareKeyData( UInt       aKeyDataType,
                                  sdiValue * aValue1,
                                  sdiValue * aValue2,
                                  SShort   * aResult );

    static IDE_RC convertRangeValue( SChar       * aValue,
                                     UInt          aLength,
                                     UInt          aKeyType,
                                     sdiValue    * aRangeValue );

    static IDE_RC getLocalMetaInfo( sdiLocalMetaInfo * aLocalMetaInfo );
    static IDE_RC getLocalMetaInfoAndCheckKSafety( sdiLocalMetaInfo * aLocalMetaInfo,
                                                   idBool * aOutIsKSafetyNULL );
    static IDE_RC getLocalMetaInfoCore( smiStatement         * aSmiStmt,
                                        sdiLocalMetaInfo * aLocalMetaInfo,
                                        idBool           * aOutIsKSafetyNULL );

    static IDE_RC getGlobalMetaInfo( sdiGlobalMetaInfo * aMetaNodeInfo );

    static IDE_RC getGlobalMetaInfoCore( smiStatement          * aSmiStmt,
                                         sdiGlobalMetaInfo * aMetaNodeInfo );

    static idBool isShardMetaCreated( smiStatement * aSmiStmt );

    static IDE_RC validateRangeCountBeforeInsert( qcStatement  * aStatement,
                                                  sdiTableInfo * aTableInfo,
                                                  ULong          aSMN );

    static IDE_RC validateParamBeforeInsert( qsOID   aProcOID,
                                             SChar * aUserName,
                                             SChar * aProcName,
                                             SChar * aSplitMethod,
                                             SChar * aKeyColumnName,
                                             SChar * aSubSplitMethod,
                                             SChar * aSubKeyName );

    static IDE_RC validateColumnBeforeInsert( qcStatement  * aStatement,
                                              UInt           aUserID,
                                              SChar        * aTableName,
                                              SChar        * aSplitMethod,
                                              SChar        * aKeyColumnName,
                                              SChar        * aSubSplitMethod,
                                              SChar        * aSubKeyName );

    static idBool isSupportDataType( UInt aModuleID );

    static IDE_RC getShardUserID( UInt * aShardUserID );

    static IDE_RC getSplitMethodByPartition( smiStatement   * aStatement,
                                             SChar          * aUserName,
                                             SChar          * aTableName,
                                             sdiSplitMethod * aOutSplitMethod );

    static IDE_RC getShardKeyStrByPartition( smiStatement   * aStatement,
                                             SChar          * aUserName,
                                             SChar          * aTableName,
                                             UInt             aSharKeyBufLen,
                                             SChar          * aOutShardKeyBuf );

    static IDE_RC getPartitionNameByValue( smiStatement          * aStatement,
                                           SChar                 * aUserName,
                                           SChar                 * aTableName,
                                           SChar                 * aValue,
                                           qcmTablePartitionType * aOutTablePartitionType,
                                           SChar                 * aOutPartitionName );

    static IDE_RC getPartitionValueByName( smiStatement * aStatement,
                                           SChar        * aUserName,
                                           SChar        * aTableName,
                                           SChar        * aPartitionName,
                                           SInt           aOutValueBufLen,
                                           SChar        * aOutValueBuf,
                                           idBool       * aOutIsDefault );

    static IDE_RC addReplTable( qcStatement * aStatement,
                                SChar       * aNodeName,
                                SChar       * aReplName,
                                SChar       * aUserName,
                                SChar       * aTableName,
                                SChar       * aPartitionName,
                                sdmReplDirectionalRole aRole,
                                idBool        aIsNewTrans );

    static IDE_RC dropReplTable( qcStatement * aStatement,
                                 SChar       * aNodeName,
                                 SChar       * aReplName,
                                 SChar       * aUserName,
                                 SChar       * aTableName,
                                 SChar       * aPartitionName,
                                 sdmReplDirectionalRole aRole,
                                 idBool        aIsNewTrans );

    static IDE_RC getNthBackupNodeName( smiStatement * aSmiStmt,
                                        SChar        * aNodeName,
                                        SChar        * aBakNodeName,
                                        UInt           aNth,
                                        ULong          aSMN );

    static IDE_RC findSendReplInfoFromReplicaSet( sdiReplicaSetInfo   * aReplicaSetInfo,
                                                  SChar               * aNodeName,
                                                  sdiReplicaSetInfo   * aOutReplicaSetInfo );

    static IDE_RC findRecvReplInfoFromReplicaSet( sdiReplicaSetInfo   * aReplicaSetInfo,
                                                  SChar               * aNodeName,
                                                  UInt                  aNth,
                                                  sdiReplicaSetInfo   * aOutReplicaSetInfo );

    static IDE_RC dropReplicationItemsPerRPSet( qcStatement * aStatement,
                                                SChar       * aReplName,
                                                SChar       * aNodeName,
                                                SChar       * aUserName,
                                                SChar       * aTableName,
                                                sdmReplDirectionalRole aRole,
                                                ULong         aSMN,
                                                UInt          aReplicaSetId );

    static IDE_RC addReplicationItemUsingRPSets( qcStatement       * aStatement,
                                                 sdiReplicaSetInfo * aReplicaSetInfo,
                                                 SChar             * aNodeName,
                                                 SChar             * aUserName,
                                                 SChar             * aTableName,
                                                 SChar             * aPartitionName,
                                                 UInt                aNth,
                                                 sdmReplDirectionalRole aRole );

    static IDE_RC dropReplicationItemUsingRPSets( qcStatement       * aStatement,
                                                  sdiReplicaSetInfo * aReplicaSetInfo,
                                                  SChar             * aUserName,
                                                  SChar             * aTableName,
                                                  UInt                aNth,
                                                  sdmReplDirectionalRole  aRole,
                                                  UInt                aSMN );

    static IDE_RC flushReplicationItemUsingRPSets( qcStatement       * aStatement,
                                                   sdiReplicaSetInfo * aReplicaSetInfo,
                                                   SChar             * aUserName,
                                                   SChar             * aNodeName,
                                                   UInt                aNth,
                                                   sdmReplDirectionalRole aRole );
    
    static IDE_RC getTableInfoAllObject( qcStatement       * aStatement,
                                         sdiTableInfoList ** aTableInfoList,
                                         ULong               aSMN );

    static IDE_RC setKeyDataType( qcStatement  * aStatement,
                                  sdiTableInfo * aTableInfo );

    static IDE_RC insertFailoverHistory( qcStatement * aStatement,
                                         SChar       * aNodeName,
                                         ULong         aSMN,
                                         UInt        * aRowCnt );

    static IDE_RC deleteFailoverHistory( qcStatement * aStatement,
                                         ULong         aSMN,
                                         UInt          aReplicaSetId,
                                         UInt        * aRowCnt );

    static idBool checkFailoverAvailable( sdiReplicaSetInfo * aReplicaSetInfo,
                                          UInt                aReplicaSetId,
                                          SChar             * aNodeName );

    static idBool checkFailbackAvailable( sdiReplicaSetInfo * aReplicaSetInfo,
                                          UInt                aReplicaSetId,
                                          SChar             * aNodeName );

    static IDE_RC updateReplicaSet4Failover( qcStatement        * aStatement,
                                             SChar              * aOldNodeName,
                                             SChar              * aNewNodeName );

    static IDE_RC backupReplicaSet4Failover( qcStatement        * aStatement,
                                             SChar              * aTargetNodeName,
                                             SChar              * aFailoverNodeName,
                                             ULong                aSMN );

    static IDE_RC validateShardObjectTable( smiStatement   * aStatement,
                                            SChar          * aUserName,
                                            SChar          * aTableName,
                                            sdfArgument    * aArgumentList,
                                            sdiSplitMethod   aSplitMethod,
                                            UInt             aPartitionCnt,
                                            idBool         * aExistIndex );
    
    static IDE_RC flushReplTable( qcStatement * aStatement,
                                  SChar       * aNodeName,
                                  SChar       * aUserName,
                                  SChar       * aReplName,
                                  idBool        aIsNewTrans );

    /* PROJ-2748 Shard Failback */
    static IDE_RC getFailoverHistoryWithPrimaryNodename( smiStatement       * aSmiStmt,
                                                         sdiReplicaSetInfo  * aReplicaSetsInfo,
                                                         SChar              * aPrimaryNodeName,
                                                         ULong                aSMN );

    static IDE_RC getFailoverHistoryWithSMN( smiStatement       * aSmiStmt,
                                             sdiReplicaSetInfo  * aReplicaSetsInfo,
                                             ULong                aSMN );

    static IDE_RC updateReplicaSet4Failback( qcStatement        * aStatement,
                                             SChar              * aOldNodeName,
                                             SChar              * aNewNodeName );

    static IDE_RC touchMeta( qcStatement * aStatement );

    /* TASK-7307 DML Data Consistency in Shard */
    static IDE_RC alterUsable( qcStatement * aStatement,
                               SChar       * aUserName,
                               SChar       * aTableName,
                               SChar       * aPartitionName,
                               idBool        aIsUsable,
                               idBool        aIsNewTrans );

    static IDE_RC alterShardFlag( qcStatement      * aStatement,
                                  SChar            * aUserName,
                                  SChar            * aTableName,
                                  sdiSplitMethod     aSdSplitMethod,
                                  idBool             aIsNewTrans );

};

#endif /* _O_SDM_H_ */
