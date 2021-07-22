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

#include <sdiZookeeper.h>
#include <mtc.h>
#include <smi.h>

idBool          sdiZookeeper::mIsConnect = ID_FALSE;
idBool          sdiZookeeper::mIsGetShardLock = ID_FALSE;
idBool          sdiZookeeper::mIsGetZookeeperLock = ID_FALSE;
aclZK_Handler * sdiZookeeper::mZKHandler = NULL ;
SChar           sdiZookeeper::mMyNodeName[SDI_NODE_NAME_MAX_SIZE + 1] = {0,};
iduMutex        sdiZookeeper::mConnectMutex;
SChar           sdiZookeeper::mTargetNodeName[SDI_NODE_NAME_MAX_SIZE + 1] = {0,};
SChar           sdiZookeeper::mFailoverNodeName[SDI_NODE_NAME_MAX_SIZE + 1] = {0,};
ZKJOB           sdiZookeeper::mRunningJobType = ZK_JOB_NONE;
qciUserInfo     sdiZookeeper::mUserInfo;
UInt            sdiZookeeper::mTickTime = 0;
UInt            sdiZookeeper::mSyncLimit = 0;
UInt            sdiZookeeper::mServerCnt = 0;

SChar           sdiZookeeper::mFirstStopNodeName[SDI_NODE_NAME_MAX_SIZE + 1];
SChar           sdiZookeeper::mSecondStopNodeName[SDI_NODE_NAME_MAX_SIZE + 1];
SChar           sdiZookeeper::mFirstStopReplName[SDI_REPLICATION_NAME_MAX_SIZE + 1];
SChar           sdiZookeeper::mSecondStopReplName[SDI_REPLICATION_NAME_MAX_SIZE + 1];

SChar           sdiZookeeper::mDropReplName[SDI_REPLICATION_NAME_MAX_SIZE + 1];

void sdiZookeeper::logAndSetErrMsg( ZKCResult , SChar * )
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return;
}

void sdiZookeeper::initialize( SChar *,
                               qciUserInfo * )
{
    return;
}

void sdiZookeeper::finalize()
{
    return;
}

IDE_RC sdiZookeeper::connect( idBool )
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return IDE_FAILURE;
}

void sdiZookeeper::disconnect()
{ 
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return;
}

IDE_RC sdiZookeeper::getZookeeperMetaLock( UInt )
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return IDE_FAILURE;    
}

IDE_RC sdiZookeeper::getShardMetaLock( UInt )
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::getMetaLock( ZookeeperLockType, UInt )
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return IDE_FAILURE;
}

void sdiZookeeper::releaseZookeeperMetaLock()
{ 
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return;
}

void sdiZookeeper::releaseShardMetaLock()
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return;
}

IDE_RC sdiZookeeper::releaseMetaLock( ZookeeperLockType )
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::validateDB( SChar  *, SChar  *, SChar  * )
{
    return IDE_SUCCESS;
}

IDE_RC sdiZookeeper::validateNode( SChar  *, SChar  *, SChar  *,
                                   SChar  *, SChar  *, SChar  *,
                                   SChar  *, SChar  *, SChar  *,
                                   SChar  * )
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::getKSafetyInfo( SChar  * aKSafety,
                                     SChar  * aReplicationMode,
                                     SChar  * aParallelCount )
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );
    
    return IDE_FAILURE;
}
IDE_RC sdiZookeeper::validateState( ZKState , SChar * )
{ 
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::initClusterMeta( sdiDatabaseInfo,  sdiLocalMetaInfo  )
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::addNode( SChar *, SChar *, SChar *,
                              SChar *, SChar * )
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::addNodeAfterCommit( ULong )
{ 
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::dropNode( SChar * )
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::dropNodeAfterCommit( ULong )
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::shutdown()
{
    return IDE_SUCCESS; 
}

IDE_RC sdiZookeeper::joinNode()
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::joinNodeAfterCommit()
{ 
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return IDE_FAILURE;
}

void sdiZookeeper::reshardingJob()
{
    return;
} 

IDE_RC sdiZookeeper::failoverForWatcher( SChar * )
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::failoverForQuery( SChar *,
                                       SChar * )
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::failoverAfterCommit( ULong, ULong )
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::failoverAfterRollback()
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::failback()
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return IDE_FAILURE;
}


IDE_RC sdiZookeeper::failbackAfterCommit( ULong )
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::updateSMN( ULong )
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::updateState( SChar *, SChar * )
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return IDE_FAILURE;
}


IDE_RC sdiZookeeper::getAllNodeNameList( iduList ** )
{ 
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::getAllNodeInfoList( iduList ** , UInt * )
{ 
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return IDE_FAILURE;
}

sdiLocalMetaInfo * sdiZookeeper::getNodeInfo(iduList * aNodeList, SChar * aNodeName)
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return (sdiLocalMetaInfo *)NULL;
}
IDE_RC sdiZookeeper::getAliveNodeNameList( iduListNode ** )
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::getNextNode( SChar *, SChar *, ZKState * )
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::getPrevNode( SChar *, SChar *, ZKState * )
{  
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::getNextAliveNode( SChar *, SChar * )
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::getPrevAliveNode( SChar *, SChar * )
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::settingAddNodeWatch()
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::settingAllDeleteNodeWatch()
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::settingDeleteNodeWatch( SChar * )
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return IDE_FAILURE;
}

void sdiZookeeper::watch_addNewNode( aclZK_Handler *,
                                     SInt           ,
                                     SInt           ,
                                     const SChar   *,
                                     void          * )
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return;
}

void sdiZookeeper::watch_deleteNode( aclZK_Handler *,
                                     SInt           ,
                                     SInt           ,
                                     const SChar   *,
                                     void          * )
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return;
}

IDE_RC sdiZookeeper::getNodeInfo( SChar *, SChar *, SChar *, SInt  * )
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::getNodeState( SChar   *, ZKState * )
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::insertInfo( SChar *, SChar * )
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::readInfo( SChar *, SChar *, SInt  * )
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::removeInfo( SChar * )
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::createPath( SChar *, SChar * )
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::removePath( SChar * )
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::checkPath( SChar *, idBool * )
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::addConnectionInfo()
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::checkAllNodeAlive( idBool * )
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::checkRecentDeadNode( SChar *,
                                          ULong *,
                                          ULong * )
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::removeNode( SChar * )
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::initializeStatic()
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::finalizeStatic()
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return IDE_FAILURE;
}

void sdiZookeeper::setReplInfo4FailoverAfterCommit( SChar * aFirstNodeName,
                                                    SChar * aSecondNodeName,
                                                    SChar * aFirstReplName,
                                                    SChar * aSecondReplName )
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );
    return;
}

IDE_RC sdiZookeeper::killNode( SChar * aNodeName )
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );

    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::addPendingJob( SChar * aSQL,
                                    SChar * aNodeName,
                                    ZKPendingJobType aPendingType,
                                    qciStmtType      aSQLStmtType )
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );
    return IDE_FAILURE;
}

idBool sdiZookeeper::isExistPendingJob()
{
    return ID_FALSE;
}

void sdiZookeeper::executePendingJob( ZKPendingJobType aPendingType )
{
    return;
}

void sdiZookeeper::removePendingJob()
{
    return;
}

void sdiZookeeper::callAfterCommitFunc( ULong   aNewSMN, ULong aBeforeSMN )
{
    return;
}

void sdiZookeeper::callAfterRollbackFunc( ULong   aNewSMN, ULong aBeforeSMN )
{
    return;
}

IDE_RC sdiZookeeper::addRevertJob( SChar * aSQL,
                     SChar * aNodeName,
                     ZKRevertJobType aRevertType )
{
  IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                           "sdiZookeeperstub.cpp",
                           "Zookeeper C client only support Linux." ) );
  return IDE_FAILURE;
}

void sdiZookeeper::executeRevertJob( ZKRevertJobType aRevertType )
{
  return;
}

idBool sdiZookeeper::isExistRevertJob()
{
  return ID_FALSE;
}

void  sdiZookeeper::removeRevertJob()
{
  return;
}

IDE_RC sdiZookeeper::getAliveNodeNameListIncludeNodeName( iduList ** ,
                                                          SChar    * )
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );
    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::checkExistNotFailedoverDeadNode( idBool * )
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );
    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::getZookeeperSMN( ULong * )
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );
    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::getZookeeperInfo4PV( iduList ** aList )
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );
    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::insertList4PV( iduList * aList,
                                    SChar   * aZookeeperPath )
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );
    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::finalizeClusterMeta()
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );
    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::prepareForUpdateSMN( ULong )
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );
    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::getAllNodeInfoList( iduList ** , UInt * , iduMemory * )
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );
    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::removeConnectionInfo( SChar * )
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );
    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::removeRecursivePathAndInfo( SChar * )
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );
    return IDE_FAILURE;
}

IDE_RC sdiZookeeper::checkNodeAlive( SChar * ,idBool * )
{
    IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                              "sdiZookeeperstub.cpp",
                              "Zookeeper C client only support Linux." ) );
    return IDE_FAILURE;
}
