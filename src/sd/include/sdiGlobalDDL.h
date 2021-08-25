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

/***********************************************************************
 * PROJ-2757 Advanced Global DDL
 **********************************************************************/

#ifndef _O_SDI_GLOBAL_DDL_H_
#define _O_SDI_GLOBAL_DDL_H_ 1

#include <idl.h>
#include <sdi.h>
#include <qcg.h>
#include <sdiZookeeper.h>

class sdiGlobalDDL
{
public:
    static void clearInfo( qcStatement      * aStatement );

    static void setInfo( qcStatement               * aQcStatement,
                         idBool                      aIsShardObject,
                         sdiDDLType                  aDDLType,
                         qcNamePosition            * aUserNamePos,
                         qcNamePosition            * aObjectNamePos,
                         qdIndexPartitionAttribute * aIndexTBSAttr = NULL );

    static IDE_RC execute( qcStatement * aStatement );

    static IDE_RC executeCreateTable( qcStatement    * aStatement );

    static IDE_RC executeSplitPartition( qcStatement    * aStatement );

    static IDE_RC executeDropIndex( qcStatement    * aStatement );

    static IDE_RC executeReplace( qcStatement    * aStatement );

    static IDE_RC updateShardMetaForSplitPartition( qcStatement * aStatement,
                                                    SChar       * aUserName,
                                                    SChar       * aTableName );

private:
    static IDE_RC checkProperties( qcStatement * aStatement,
                                   UInt          aKSafety );

    static IDE_RC executeDDLToAllNodes( qcStatement * aStatement,
                                        UInt          aKSafety,
                                        SChar       * aUserName,
                                        SChar       * aTableName );

    static IDE_RC initShardEnv( qcStatement * aStatement );

    static void   getTableName( qcStatement * aStatement,
                                SChar       * aUserName,
                                SChar       * aTableName );

    static IDE_RC addPendingJob( qcStatement       * aStatement,
                                 UInt                aKSafety,
                                 sdiReplicaSetInfo * aReplicaSetInfo );

    static IDE_RC lockTable( qcStatement    * aStatement,
                             SChar          * aUserName,
                             SChar          * aTableName );

    static IDE_RC lockTableInAllShardNodes( qcStatement      * aStatement,
                                            qcNamePosition     aUserNamePos,
                                            qcNamePosition     aTableNamePos,
                                            smiTableLockMode   aLockMode = SMI_TABLE_LOCK_X );

    static IDE_RC lockPartitionInAllShardNodes( qcStatement      * aStatement,
                                                qcNamePosition     aUserNamePos,
                                                qcNamePosition     aTableNamePos,
                                                qcNamePosition     aPartNamePos );

    static IDE_RC lockTableInAllShardNodes( qcStatement      * aStatement,
                                            SChar            * aUserName,
                                            SChar            * aTableName,
                                            SChar            * aPartName = NULL,
                                            smiTableLockMode   aLockMode = SMI_TABLE_LOCK_X );

    static IDE_RC executeForBackup( qcStatement    * aStatement );

    static IDE_RC executeBackupDDL( qcStatement * aStatement );

    static IDE_RC makeAndExecuteBackupDDL( qcStatement    * aStatement,
                                           SInt             aNumOffsets,
                                           ... );

    static IDE_RC flushReplication( qcStatement       * aStatement,
                                    UInt                aKSafety,
                                    SChar             * aUserName,
                                    sdiReplicaSetInfo * aReplicaSetInfo );

    static void copyReplicaSetInfo( sdiReplicaSetInfo * aSrc,
                                    UInt                aSrcIdx,
                                    sdiReplicaSetInfo * aDst,
                                    UInt                aDstIdx );

    static IDE_RC getReplicaSetInfo( qcStatement       * aStatement,
                                     SChar             * aUserName,
                                     SChar             * aTableName,
                                     sdiReplicaSetInfo * aReplicaSetInfo );
};

inline IDE_RC sdiGlobalDDL::initShardEnv( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *    PROJ-2757 Advanced Global DDL
 *
 *    Global DDL 수행할 때 기본 검사
 *
 * Implementation :
 *      1. ShardMeta Lock
 *
 *      2. shard 테이블이면 모든 노드가 정상인지 검사한다.
 *
 *      3. shard linker 검사 & 초기화
 *
 ***********************************************************************/
    idBool          sIsAlive             = ID_FALSE;
    idBool          sHasShardMetaLock    = ID_FALSE;
    sdiClientInfo * sClientInfo          = NULL;

    /* 1. ShardMeta Lock */
    IDE_TEST( sdiZookeeper::getShardMetaLock(
                QC_SMI_STMT(aStatement)->getTrans()->getTransID() )
              != IDE_SUCCESS );
    sHasShardMetaLock = ID_TRUE;

    /* 2. shard 테이블이면 모든 노드가 정상인지 검사한다. */
    if ( QC_SHARD_GLOBAL_DDL_IS_SHARD_OBJECT( aStatement ) == ID_TRUE )
    {
        IDE_TEST( sdiZookeeper::checkAllNodeAlive( &sIsAlive )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( sIsAlive != ID_TRUE, ERR_CLUSTER_STATE );
    }
    else
    {
        /* Nothing to do */
    }

    /* 3. shard linker 검사 & 초기화 */
    IDE_TEST( sdi::checkShardLinker( aStatement ) != IDE_SUCCESS );
        
    sClientInfo = QC_SHARD_CLIENT_INFO( aStatement->session );

    IDE_TEST_RAISE( sClientInfo == NULL, ERR_NODE_NOT_EXIST );

    sdiZookeeper::globalDDLJob();

    /* 이 함수 내에서 에러가 발생하는 경우에는 callAfterRollback이 호출되지
       않도록 flag를 맨 나중에 설정한다.
       즉, 이 함수 내에서 ShardMetaLock을 해제한다. */
    // for qci::endTransForSession
    aStatement->session->mQPSpecific.mFlag &= ~QC_SESSION_GLOBAL_DDL_MASK;
    aStatement->session->mQPSpecific.mFlag |= QC_SESSION_GLOBAL_DDL_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CLUSTER_STATE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_ZKC_DEADNODE_EXIST ) );
    }
    IDE_EXCEPTION( ERR_NODE_NOT_EXIST )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_NODE_NOT_EXIST ) );
    }
    IDE_EXCEPTION_END;

    if ( sHasShardMetaLock == ID_TRUE )
    {
        sdiZookeeper::releaseShardMetaLock();
    }

    return IDE_FAILURE;
}

inline void sdiGlobalDDL::getTableName( qcStatement * aStatement,
                                        SChar       * aUserName,
                                        SChar       * aTableName )
{
    if ( QC_IS_NULL_NAME(QC_SHARD_GLOBAL_DDL_USER_NAME(aStatement)) == ID_TRUE )
    {
        idlOS::strcpy( aUserName, QCG_GET_SESSION_USER_NAME(aStatement) );
    }
    else
    {
        QC_STR_COPY( aUserName, QC_SHARD_GLOBAL_DDL_USER_NAME(aStatement) );
    }

    QC_STR_COPY( aTableName, QC_SHARD_GLOBAL_DDL_TABLE_NAME(aStatement) );
}

inline void sdiGlobalDDL::copyReplicaSetInfo( sdiReplicaSetInfo * aSrc,
                                              UInt                aSrcIdx,
                                              sdiReplicaSetInfo * aDst,
                                              UInt                aDstIdx )
{
    aDst->mReplicaSets[aDstIdx].mReplicaSetId =
                aSrc->mReplicaSets[aSrcIdx].mReplicaSetId;

    idlOS::strcpy( aDst->mReplicaSets[aDstIdx].mPrimaryNodeName,
                   aSrc->mReplicaSets[aSrcIdx].mPrimaryNodeName );

    idlOS::strcpy( aDst->mReplicaSets[aDstIdx].mFirstBackupNodeName,
                   aSrc->mReplicaSets[aSrcIdx].mFirstBackupNodeName );

    idlOS::strcpy( aDst->mReplicaSets[aDstIdx].mSecondBackupNodeName,
                   aSrc->mReplicaSets[aSrcIdx].mSecondBackupNodeName );

#if 0
    idlOS::strcpy( aDst->mReplicaSets[aDstIdx].mStopFirstBackupNodeName,
                   aSrc->mReplicaSets[aSrcIdx].mStopFirstBackupNodeName );

    idlOS::strcpy( aDst->mReplicaSets[aDstIdx].mStopSecondBackupNodeName,
                   aSrc->mReplicaSets[aSrcIdx].mStopSecondBackupNodeName );
#endif
    
    idlOS::strcpy( aDst->mReplicaSets[aDstIdx].mFirstReplName,
                   aSrc->mReplicaSets[aSrcIdx].mFirstReplName );

#if 0
    idlOS::strcpy( aDst->mReplicaSets[aDstIdx].mFirstReplFromNodeName,
                   aSrc->mReplicaSets[aSrcIdx].mFirstReplFromNodeName );

    idlOS::strcpy( aDst->mReplicaSets[aDstIdx].mFirstReplToNodeName,
                   aSrc->mReplicaSets[aSrcIdx].mFirstReplToNodeName );
#endif

    idlOS::strcpy( aDst->mReplicaSets[aDstIdx].mSecondReplName,
                   aSrc->mReplicaSets[aSrcIdx].mSecondReplName );

#if 0
    idlOS::strcpy( aDst->mReplicaSets[aDstIdx].mSecondReplFromNodeName,
                   aSrc->mReplicaSets[aSrcIdx].mSecondReplFromNodeName );

    idlOS::strcpy( aDst->mReplicaSets[aDstIdx].mSecondReplToNodeName,
                   aSrc->mReplicaSets[aSrcIdx].mSecondReplToNodeName );
#endif
}

#endif
