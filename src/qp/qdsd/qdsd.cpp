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

#include <idl.h>
#include <ide.h>
#include <qcg.h>
#include <qcd.h>
#include <qdsd.h>
#include <sdi.h>
#include <sdl.h>
#include <sdm.h>
#include <sdiZookeeper.h>
#include <qcmUser.h>

IDE_RC qdsd::validateShardAdd( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *    ALTER DATABASE SHARD ADD
 *
 * Implementation :
 *    1. SYS, SYSTEM 사용자 체크
 ***********************************************************************/
    idBool              sIsValid = ID_FALSE;
    idBool              sIsConnect = ID_FALSE;
    idBool              sIsAcquireZookeeperMetaLock = ID_FALSE;
    idBool              sIsAcquireShardMetaLock = ID_FALSE;
    SChar               sShardVersion[20] = {0,};
    SChar               sTransTBLSize[10] = {0,};
    sdiDatabaseInfo     sShardDBInfo;
    sdiLocalMetaInfo    sLocalMetaInfo;
    idBool sIsAlreadyExistZKSMeta = ID_FALSE;
    SChar  sPath[SDI_ZKC_PATH_LENGTH] = SDI_ZKC_PATH_ALTIBASE_SHARD;

    qciUserInfo         sUserInfo;
    void *              sMmSession = aStatement->session->mMmSession;
    idBool              sIsKSafetyNULL = ID_FALSE;
    SChar               sKSafetyStr[SDI_KSAFETY_STR_MAX_SIZE + 1] = {0,};
    SChar               sReplicationModeStr[SDI_REPLICATION_MODE_MAX_SIZE + 1] = {0,};
    SChar               sParallelCountStr[SDI_PARALLEL_COUNT_STR_MAX_SIZE + 1] = {0,};

    qci::mSessionCallback.mGetUserInfo( sMmSession, (void *)&sUserInfo );

    //    sParseTree = (qdShardParseTree *)aStatement->myPlan->parseTree;

    /***********************************************************************
     * validateCommon
     ***********************************************************************/

    IDE_TEST( validateCommon( aStatement ) != IDE_SUCCESS );

    /* BUG-48770 다른 session이 있거나 shard cli로 접근한 경우에는 shard DDL을 수행할 수 없다.*/
    if( QCU_DISPLAY_PLAN_FOR_NATC == 0 )
    {
        IDE_TEST_RAISE( qci::mSessionCallback.mCheckSessionCount() > 1, ERR_TOO_MANY_SESSION );
    }

    IDE_TEST_RAISE( qci::mSessionCallback.mIsShardClient( aStatement->session->mMmSession )
                    == ID_TRUE,
                    ERR_SHARD_CLI );

    /***********************************************************************
     * validate SHARD ADD
     ***********************************************************************/

    /***********************************************************************
     * Zookeeper validation
     ***********************************************************************/

    /* valiation에 필요한 정보를 가져온다. */
    sShardDBInfo = sdi::getShardDBInfo();

    idlOS::snprintf( sShardVersion, 20, "%"ID_INT32_FMT".%"ID_INT32_FMT".%"ID_INT32_FMT,
                    sShardDBInfo.mMajorVersion, sShardDBInfo.mMinorVersion, sShardDBInfo.mPatchVersion );
    idlOS::snprintf( sTransTBLSize, 10, "%"ID_UINT32_FMT, sShardDBInfo.mTransTblSize );

    IDE_TEST( sdi::getLocalMetaInfoAndCheckKSafety( &sLocalMetaInfo, &sIsKSafetyNULL ) != IDE_SUCCESS );

    IDE_TEST_RAISE( sLocalMetaInfo.mNodeName[0] == '\0', INVALID_LOCAL_NODE_NAME );
    sdiZookeeper::initialize( sLocalMetaInfo.mNodeName,
                              &sUserInfo );

    IDE_TEST_RAISE( sdiZookeeper::isConnect() == ID_TRUE, ERR_ALREADY_CONNECT );

    /* zookeeper server와 연결한다. */
    IDE_TEST( sdiZookeeper::connect( ID_FALSE ) != IDE_SUCCESS );
    sIsConnect = ID_TRUE;


    /* 클러스터 메타가 생성되어있는지 확인한다. */
    IDE_TEST( sdiZookeeper::checkPath( sPath,
                                       &sIsAlreadyExistZKSMeta ) != IDE_SUCCESS );

    /* zookeeper server에 클러스터 메타가 생성되어있지 않다면 생성한다. */
    if( sIsAlreadyExistZKSMeta == ID_FALSE )
    {
        IDE_TEST_RAISE( sIsKSafetyNULL == ID_TRUE, ERR_INVALID_KSAFETY );
        /* zookeeper server에 클러스터 메타를 생성한다. */
        IDE_TEST( sdiZookeeper::initClusterMeta( sShardDBInfo,
                                                 sLocalMetaInfo ) != IDE_SUCCESS );
    }

    /* 샤드 메타를 변경하므로 zookeeper의 샤드 메타 lock을 잡는다. */
    IDE_TEST( sdiZookeeper::getZookeeperMetaLock(QCG_GET_SESSION_ID(aStatement)) != IDE_SUCCESS );
    sIsAcquireZookeeperMetaLock = ID_TRUE;

    IDE_TEST( sdiZookeeper::getShardMetaLock( QC_SMI_STMT(aStatement)->getTrans()->getTransID() ) != IDE_SUCCESS );
    sIsAcquireShardMetaLock = ID_TRUE;

    /* zookeeper server의 것과 노드 메타(DB이름,바이너리 버전 등)가 일치하는지 체크한다. */

    /* replication 정보가 세팅되지 않았을 경우에는 해당 정보에 대한 체크는 하지 않고 *
     * zookeeper server의 값을 받아와 세팅한다. at execute                    */
    if (sIsAlreadyExistZKSMeta == ID_TRUE)
    {
        if( sIsKSafetyNULL == ID_TRUE )
        {
            /* replication 정보를 제외하고 비교 */
            IDE_TEST( sdiZookeeper::validateNode( sShardDBInfo.mDBName,
                                                sShardDBInfo.mDBCharSet,
                                                sShardDBInfo.mNationalCharSet,
                                                NULL, /* k-safety */
                                                NULL, /* replication mode */
                                                NULL, /* parallel count */
                                                sShardDBInfo.mDBVersion,
                                                sShardVersion,
                                                sTransTBLSize,
                                                sLocalMetaInfo.mNodeName ) != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST_RAISE( idlOS::uIntToStr( sLocalMetaInfo.mKSafety,
                                              sKSafetyStr,
                                              SDI_KSAFETY_STR_MAX_SIZE + 1) != 0, ERR_INVALID_KSAFETY );
            IDE_TEST_RAISE( idlOS::uIntToStr( sLocalMetaInfo.mReplicationMode,
                                              sReplicationModeStr,
                                              SDI_REPLICATION_MODE_MAX_SIZE +1 ) != 0, ERR_INVALID_KSAFETY );
            IDE_TEST_RAISE( idlOS::uIntToStr( sLocalMetaInfo.mParallelCount,
                                              sParallelCountStr,
                                              SDI_PARALLEL_COUNT_STR_MAX_SIZE +1) != 0, ERR_INVALID_KSAFETY );

            IDE_TEST( sdiZookeeper::validateNode( sShardDBInfo.mDBName,
                                                  sShardDBInfo.mDBCharSet,
                                                  sShardDBInfo.mNationalCharSet,
                                                  sKSafetyStr, /* k-safety */
                                                  sReplicationModeStr, /* replication mode */
                                                  sParallelCountStr, /* parallel count */
                                                  sShardDBInfo.mDBVersion,
                                                  sShardVersion,
                                                  sTransTBLSize,
                                                  sLocalMetaInfo.mNodeName ) != IDE_SUCCESS );

        }
    }
    else
    {
        IDE_TEST_RAISE( sIsKSafetyNULL == ID_TRUE, ERR_INVALID_KSAFETY ); /* already checked, for enhance readablility */
    }

    /* zookeeper server에 죽어있는 노드가 있는지 체크한다. */
    IDE_TEST( sdiZookeeper::checkAllNodeAlive( &sIsValid ) != IDE_SUCCESS );
    IDE_TEST_RAISE( sIsValid == ID_FALSE, deadNode_exist );
    
    if ( sIsAcquireShardMetaLock == ID_TRUE )
    {
        sdiZookeeper::releaseShardMetaLock();
        sIsAcquireShardMetaLock = ID_FALSE;
    }

    if ( sIsAcquireZookeeperMetaLock == ID_TRUE )
    {
        sdiZookeeper::releaseZookeeperMetaLock();
        sIsAcquireZookeeperMetaLock = ID_FALSE;
    }

    sdiZookeeper::disconnect();
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TOO_MANY_SESSION )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_TOO_MANY_SESSION, "add" ) );
    }
    IDE_EXCEPTION( ERR_SHARD_CLI )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_SHARD_DDL_TRY_WITH_SHARD_CLI ) );
    }
    IDE_EXCEPTION( ERR_INVALID_KSAFETY )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_INVALID_KSAFETY ) );
    }
    IDE_EXCEPTION( INVALID_LOCAL_NODE_NAME )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_INVALID_LOCAL_NODE_NAME ) );
    }
    IDE_EXCEPTION( deadNode_exist )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_ZKC_DEADNODE_EXIST ) );
    }
    IDE_EXCEPTION( ERR_ALREADY_CONNECT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_ZKC_ALREADY_CONNECTED ) );
    }

    IDE_EXCEPTION_END;

    if ( sIsAcquireShardMetaLock == ID_TRUE )
    {
        sdiZookeeper::releaseShardMetaLock();
        sIsAcquireShardMetaLock = ID_FALSE;
    }

    if ( sIsAcquireZookeeperMetaLock == ID_TRUE )
    {
        sdiZookeeper::releaseZookeeperMetaLock();
        sIsAcquireZookeeperMetaLock = ID_FALSE;
    }

    if( sIsConnect == ID_TRUE )
    {
        sdiZookeeper::disconnect();
    }

    return IDE_FAILURE;
}

IDE_RC qdsd::executeShardAdd( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *    ALTER DATABASE SHARD ADD
 *
 * Implementation :
 *    1. zks meta에 노드 정보 add 및 타 노드 정보 get
 *    2. 샤드 메타 복제 (from other node)
 *       Internal Replication - add Temporary replication
 *    3. set_node 전파
 ***********************************************************************/
    qdShardParseTree  * sParseTree = NULL;

    sParseTree = (qdShardParseTree *)aStatement->myPlan->parseTree;
    
    /***********************************************************************
     * check internal property validate
     ***********************************************************************/
    IDE_TEST( validateCommonForExecute( aStatement ) != IDE_SUCCESS );

    IDE_TEST_RAISE( sdiZookeeper::isConnect() == ID_TRUE, ERR_ALREADY_CONNECT );

    /* zookeeper server와 연결한다. */
    IDE_TEST( sdiZookeeper::connect( ID_FALSE ) != IDE_SUCCESS );

    /* for smn propagation to all nodes at commit */
    sdi::setShardMetaTouched( aStatement->session );

    IDE_TEST( checkShardPIN( aStatement ) != IDE_SUCCESS );

    sdi::setInternalTableSwap( aStatement->session );

    IDE_TEST( sdiZookeeper::getZookeeperMetaLock(QCG_GET_SESSION_ID(aStatement)) != IDE_SUCCESS );
    IDE_TEST( sdiZookeeper::getShardMetaLock( QC_SMI_STMT(aStatement)->getTrans()->getTransID() ) != IDE_SUCCESS );
    
    /***********************************************************************
     * Zookeeper meta 1차 변경
     ***********************************************************************/
    IDE_TEST( executeZookeeperAdd() != IDE_SUCCESS );

    /***********************************************************************
     * SHARD META CHECK RESET
     ***********************************************************************/
    // R2HA SMN은 reset하지 않는다. 추후 정책 혹은 필요 시 추가
    IDE_TEST( resetShardMeta( aStatement ) != IDE_SUCCESS);

    /***********************************************************************
     * synchronize k-safety info with zookeeper meta
     ***********************************************************************/
    IDE_TEST( synchronizeKSafetyInfo(aStatement) != IDE_SUCCESS );

    /***********************************************************************
     * 샤드 메타 복제
     ***********************************************************************/
    
    IDE_TEST( getAllNodeInfoList( aStatement ) != IDE_SUCCESS );
    
    IDE_TEST( executeTemporaryReplication( aStatement ) != IDE_SUCCESS );

    /***********************************************************************
     * check SMN
     ***********************************************************************/
    if ( sParseTree->mNodeCount > 1 )
    {
        IDE_TEST( checkZookeeperSMNAndDataSMN() != IDE_SUCCESS );
    }
    
    /***********************************************************************
     * SET_NODE 전파
     ***********************************************************************/
    IDE_TEST( executeSetNodes( aStatement ) != IDE_SUCCESS );

    /***********************************************************************
     * FAILOVER 처리를 여부 체크를 위한  REPLICA_SETS_ 정보변경
     * replication stop
     ***********************************************************************/
    IDE_TEST( executeReplicationFunction( aStatement,
                                          QDSD_REPL_QUERY_TYPE_STOP )
              != IDE_SUCCESS );

    /***********************************************************************
     * replication drop
     ***********************************************************************/
    IDE_TEST( executeReplicationFunction( aStatement,
                                          QDSD_REPL_QUERY_TYPE_DROP )
              != IDE_SUCCESS );

    /***********************************************************************
     * backup table no item create replication
     ***********************************************************************/
    IDE_TEST( executeReplicationFunction( aStatement,
                                          QDSD_REPL_QUERY_TYPE_CREATE )
              != IDE_SUCCESS );
        
    /***********************************************************************
     * backup table add replication
     ***********************************************************************/
    IDE_TEST( executeReplicationFunction( aStatement,
                                          QDSD_REPL_QUERY_TYPE_ADD )
              != IDE_SUCCESS );

    /***********************************************************************
     * backup table truncate
     ***********************************************************************/
    IDE_TEST( truncateBackupTable( aStatement ) != IDE_SUCCESS );

    /***********************************************************************
     * replication sync start
     ***********************************************************************/
    IDE_TEST( executeReplicationFunction( aStatement,
                                          QDSD_REPL_QUERY_TYPE_START )
              != IDE_SUCCESS );

    /***********************************************************************
     * replication flush
     ***********************************************************************/
    IDE_TEST( executeReplicationFunction( aStatement,
                                          QDSD_REPL_QUERY_TYPE_FLUSH )
              != IDE_SUCCESS );

    /***********************************************************************
     * TASK-7307 DML Data Consistency in Shard
     *   SYSTEM_.SYS_TABLES_.SHARD_FLAG 업데이트
     ***********************************************************************/
    IDE_TEST( executeAlterShardFlag( aStatement ) != IDE_SUCCESS );
    
    /***********************************************************************
     * execute add clone
     ***********************************************************************/
    IDE_TEST( executeShardAddForClone( aStatement ) != IDE_SUCCESS );

    sdi::loadShardPinInfo();

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ALREADY_CONNECT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_ZKC_ALREADY_CONNECTED ) );
    }
    IDE_EXCEPTION_END;
    
    sdi::unsetInternalTableSwap( aStatement->session );

    /* TASK-7217 Sharded sequence */
    qcm::unsetShardNodeID();
    
    return IDE_FAILURE;
}

IDE_RC qdsd::synchronizeKSafetyInfo( qcStatement * aStatement )
{
    sdiLocalMetaInfo    sLocalMetaInfo;
    idBool              sIsKSafetyNULL = ID_FALSE;
    SChar               sKSafetyStr[SDI_KSAFETY_STR_MAX_SIZE + 1] = {0,};
    SChar               sReplicationModeStr[SDI_REPLICATION_MODE_MAX_SIZE + 1] = {0,};
    SChar               sParallelCountStr[SDI_PARALLEL_COUNT_STR_MAX_SIZE + 1] = {0,};
    UInt                sRowCnt = 0;

    IDE_TEST( sdi::getLocalMetaInfoAndCheckKSafety( &sLocalMetaInfo, &sIsKSafetyNULL ) != IDE_SUCCESS );

    if ( sIsKSafetyNULL == ID_TRUE )
    {
        IDE_TEST( sdiZookeeper::getKSafetyInfo( sKSafetyStr,
                                                sReplicationModeStr,
                                                sParallelCountStr ) != IDE_SUCCESS );

        IDE_TEST( sdi::updateLocalMetaInfoForReplication( aStatement,
                                                          idlOS::strToUInt((UChar*)sKSafetyStr,idlOS::strlen(sKSafetyStr)),
                                                          sReplicationModeStr,
                                                          idlOS::strToUInt((UChar*)sParallelCountStr,idlOS::strlen(sParallelCountStr)),
                                                         &sRowCnt ) 
                  != IDE_SUCCESS );
        IDE_TEST_RAISE( sRowCnt != 1, ERR_META_CRASH );
    }
    else
    {
        /* do nothing */
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_CRASH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_META_CRASH ) );
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC qdsd::validateShardJoin( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *    ALTER DATABASE SHARD JOIN
 *
 * Implementation :
 *    1.
 ***********************************************************************/

    idBool              sIsConnect = ID_FALSE;
    sdiLocalMetaInfo    sLocalMetaInfo;
    idBool              sIsAcquireZookeeperMetaLock = ID_FALSE;
    idBool              sIsAcquireShardMetaLock = ID_FALSE;
    qciUserInfo         sUserInfo;
    void *              sMmSession = aStatement->session->mMmSession;
        
    qci::mSessionCallback.mGetUserInfo( sMmSession, &sUserInfo );

    /***********************************************************************
     * validateCommon
     ***********************************************************************/

    IDE_TEST( validateCommon( aStatement ) != IDE_SUCCESS );

    /* BUG-48770 다른 session이 있거나 shard cli로 접근한 경우에는 shard DDL을 수행할 수 없다.*/
    if( QCU_DISPLAY_PLAN_FOR_NATC == 0 )
    {
        IDE_TEST_RAISE( qci::mSessionCallback.mCheckSessionCount() > 1, ERR_TOO_MANY_SESSION );
    }

    IDE_TEST_RAISE( qci::mSessionCallback.mIsShardClient( aStatement->session->mMmSession )
                    == ID_TRUE,
                    ERR_SHARD_CLI );

    /***********************************************************************
     * Zookeeper validation
     ***********************************************************************/

    IDE_TEST_RAISE( sdiZookeeper::isConnect() == ID_TRUE, ERR_ALREADY_CONNECT );

    /* zookeeper server와 연결한다. */
    IDE_TEST( sdi::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );
    IDE_TEST_RAISE( sLocalMetaInfo.mNodeName[0] == '\0', no_meta_exist );

    sdiZookeeper::initialize( sLocalMetaInfo.mNodeName,
                              &sUserInfo );
    IDE_TEST( sdiZookeeper::connect( ID_FALSE ) != IDE_SUCCESS );
    sIsConnect = ID_TRUE;

    /* 샤드 메타를 변경하므로 zookeeper의 샤드 메타 lock을 잡는다. */
    IDE_TEST( sdiZookeeper::getZookeeperMetaLock(QCG_GET_SESSION_ID(aStatement)) != IDE_SUCCESS );
    sIsAcquireZookeeperMetaLock = ID_TRUE;
    
    IDE_TEST( sdiZookeeper::getShardMetaLock( QC_SMI_STMT(aStatement)->getTrans()->getTransID() ) != IDE_SUCCESS );
    sIsAcquireShardMetaLock = ID_TRUE;

    /* 현재 노드의 상태가 JOIN이 가능한 상태인지 체크한다. */
    IDE_TEST( sdiZookeeper::validateState( ZK_JOIN,
                                           sdiZookeeper::mMyNodeName ) != IDE_SUCCESS );
    
    /***********************************************************************
     * check SMN
     ***********************************************************************/
//    IDE_TEST( checkZookeeperSMNAndDataSMN() != IDE_SUCCESS );
    
    if ( sIsAcquireShardMetaLock == ID_TRUE )
    {
        sdiZookeeper::releaseShardMetaLock();
        sIsAcquireShardMetaLock = ID_FALSE;
    }
    
    if ( sIsAcquireZookeeperMetaLock == ID_TRUE )
    {
        sdiZookeeper::releaseZookeeperMetaLock();
        sIsAcquireZookeeperMetaLock = ID_FALSE;
    }
    
    sdiZookeeper::disconnect();

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TOO_MANY_SESSION )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_TOO_MANY_SESSION, "join" ) );
    }
    IDE_EXCEPTION( ERR_SHARD_CLI )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_SHARD_DDL_TRY_WITH_SHARD_CLI ) );
    }
    IDE_EXCEPTION( no_meta_exist )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_SHARD_META_NOT_CREATED ) );
    }
    IDE_EXCEPTION( ERR_ALREADY_CONNECT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_ZKC_ALREADY_CONNECTED ) );
    }
    IDE_EXCEPTION_END;

    if ( sIsAcquireShardMetaLock == ID_TRUE )
    {
        sdiZookeeper::releaseShardMetaLock();
        sIsAcquireShardMetaLock = ID_FALSE;
    }

    if ( sIsAcquireZookeeperMetaLock == ID_TRUE )
    {
        sdiZookeeper::releaseZookeeperMetaLock();
        sIsAcquireZookeeperMetaLock = ID_FALSE;
    }

    if( sIsConnect == ID_TRUE )
    {
        sdiZookeeper::disconnect();
    }

    return IDE_FAILURE;
}

IDE_RC qdsd::executeShardJoin( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *    ALTER DATABASE SHARD JOIN
 *
 * Implementation :
 *    1. 
 ***********************************************************************/

    SChar               sGetShardMetaFromNodeName[SDI_NODE_NAME_MAX_SIZE + 1];
    ULong               sOldSMN = 0;
    ULong               sZKSMN  = 0;

    sdiReplicaSetInfo   sReplicaSetInfo;
    sdiClientInfo     * sClientInfo  = NULL;
    SChar               sSqlStr[QD_MAX_SQL_LENGTH + 1];

    iduList           * sList;
    idBool              sIsAllocList = ID_FALSE;
    
    /***********************************************************************
     * check internal property validate
     ***********************************************************************/
    IDE_TEST( validateCommonForExecute( aStatement ) != IDE_SUCCESS );

    IDE_TEST_RAISE( sdiZookeeper::isConnect() == ID_TRUE, ERR_ALREADY_CONNECT );

    /* zookeeper server와 연결한다. */
    IDE_TEST( sdiZookeeper::connect( ID_FALSE ) != IDE_SUCCESS );

    /* for smn propagation to all nodes at commit */
    sdi::setShardMetaTouched( aStatement->session );

    IDE_TEST( checkShardPIN( aStatement ) != IDE_SUCCESS );

    sdi::setInternalTableSwap( aStatement->session );

    IDE_TEST( sdiZookeeper::getZookeeperMetaLock(QCG_GET_SESSION_ID(aStatement)) != IDE_SUCCESS );
    IDE_TEST( sdiZookeeper::getShardMetaLock( QC_SMI_STMT(aStatement)->getTrans()->getTransID() ) != IDE_SUCCESS );

    IDE_TEST( getAllNodeInfoList( aStatement ) != IDE_SUCCESS );
    
    /***********************************************************************
     * Zookeeper meta 1차 변경
     ***********************************************************************/
    IDE_TEST( executeZookeeperJoin() != IDE_SUCCESS );
    
    IDE_TEST( sdiZookeeper::getZookeeperSMN( & sZKSMN ) != IDE_SUCCESS );
    sOldSMN = sdi::getSMNForDataNode();

    /* Node와 Zookeeper의 SMN이 같으면 Join, 다르면 JOIN_SMN_MODIFIED 이다. */
    if ( sOldSMN != sZKSMN )
    {
        /* SMN이 다르다 Alive Node로 부터 Shard Meta를 가져온다. */
        IDE_TEST( sdiZookeeper::getNextAliveNode( sdiZookeeper::mMyNodeName,
                                                  sGetShardMetaFromNodeName ) != IDE_SUCCESS );

        IDE_TEST( resetShardMeta( aStatement ) != IDE_SUCCESS);
        IDE_TEST( getShardMetaFromOtherNode( aStatement,
                                             sGetShardMetaFromNodeName ) != IDE_SUCCESS );

        sOldSMN = sdi::getSMNForDataNode();
    }

    /* get Alive Node List + LocalNode 필요 */
    IDE_TEST( sdiZookeeper::getAliveNodeNameListIncludeNodeName( &sList,
                                                                 sdiZookeeper::mMyNodeName ) 
              != IDE_SUCCESS );
    sIsAllocList = ID_TRUE;

    /* NodeList 대상으로 ShardLinker 생성해야 한다. */
    IDE_TEST( sdi::checkShardLinkerWithNodeList ( aStatement,
                                                  sOldSMN,
                                                  NULL,
                                                  sList ) != IDE_SUCCESS );

    sClientInfo = aStatement->session->mQPSpecific.mClientInfo;

    IDE_TEST_RAISE( sClientInfo == NULL, ERR_NODE_NOT_EXIST );

    /* get ReplicaSet Info */
    IDE_TEST( sdm::getAllReplicaSetInfoSortedByPName( QC_SMI_STMT( aStatement ),
                                                      &sReplicaSetInfo,
                                                      sOldSMN )
              != IDE_SUCCESS );

    /* JOIN은 항상 SMN Update가 발생해야 한다. */
    idlOS::snprintf( sSqlStr, 
                     QD_MAX_SQL_LENGTH,
                     "EXEC DBMS_SHARD.TOUCH_META(); " );

    IDE_TEST( executeSQLNodeList( aStatement,
                                  SDI_INTERNAL_OP_NORMAL,
                                  QCI_STMT_MASK_SP,
                                  sSqlStr,
                                  NULL,
                                  sList ) != IDE_SUCCESS );

    ideLog::log(IDE_SD_0,"[SHARD_FAILBACK] Touch Meta Complete");

    /* RP Recover */
    IDE_TEST( joinRecoverRP( aStatement,
                             &sReplicaSetInfo )
              != IDE_SUCCESS );

    sIsAllocList = ID_FALSE;
    sdiZookeeper::freeList( sList, SDI_ZKS_LIST_NODENAME );

    sdi::loadShardPinInfo();

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_ALREADY_CONNECT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_ZKC_ALREADY_CONNECTED ) );
    }
    IDE_EXCEPTION( ERR_NODE_NOT_EXIST )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_NODE_NOT_EXIST ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    
    (void)executeReplicationForJoin( aStatement, ID_TRUE );

    IDE_POP();

    if( sIsAllocList == ID_TRUE )
    {
        sdiZookeeper::freeList( sList, SDI_ZKS_LIST_NODENAME );
    }

    sdi::unsetInternalTableSwap( aStatement->session );

    /* TASK-7217 Sharded sequence */
    qcm::unsetShardNodeID();

    return IDE_FAILURE;
}


IDE_RC qdsd::validateShardDrop( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *    ALTER DATABASE SHARD DROP {'node_name'} [ FORCE ] ;
 *
 * Implementation :
 *    1.
 ***********************************************************************/

    qdShardParseTree  * sParseTree = NULL;
    idBool              sIsValid = ID_FALSE;
    SChar               sPath[SDI_ZKC_PATH_LENGTH] = {0,};
    SChar               sNodeName[SDI_NODE_NAME_MAX_SIZE + 1] = {0,};
    idBool              sIsAcquireZookeeperMetaLock = ID_FALSE;
    idBool              sIsAcquireShardMetaLock = ID_FALSE;

    SInt                sDataLength;
    SChar               sBuffer[SDI_ZKC_BUFFER_SIZE] = {0,};
    SChar               sRecentDeadNode[SDI_NODE_NAME_MAX_SIZE + 1] = {0,};
    idBool              sExistNotFailedoverDeadNode = ID_FALSE;
    
    sParseTree = (qdShardParseTree *)aStatement->myPlan->parseTree;

    /***********************************************************************
     * validateCommon
     ***********************************************************************/

    IDE_TEST( validateCommon( aStatement ) != IDE_SUCCESS );
    
    /***********************************************************************
     * Zookeeper validation
     ***********************************************************************/

    /* zookeeper server와 연결이 되어 있는 상태인지 체크한다. */
    IDE_TEST_RAISE( sdiZookeeper::isConnect() != ID_TRUE, connect_Fail );

    IDE_TEST( sdiZookeeper::getZookeeperMetaLock(QCG_GET_SESSION_ID(aStatement)) != IDE_SUCCESS );
    sIsAcquireZookeeperMetaLock = ID_TRUE;

    IDE_TEST( sdiZookeeper::getShardMetaLock( QC_SMI_STMT(aStatement)->getTrans()->getTransID() )
              != IDE_SUCCESS );
    sIsAcquireShardMetaLock = ID_TRUE;

    // DROP 중에 이미 다른 노드에서 노드가 제거된 경우 SMN으로 쳬크 한다.
    IDE_TEST( sdi::validateSMNForShardDDL(aStatement )
              != IDE_SUCCESS );
    
    /***********************************************************************
     * check SMN
     ***********************************************************************/
    IDE_TEST( checkZookeeperSMNAndDataSMN() != IDE_SUCCESS );

    /* force가 아닐 경우 죽어있는 노드가 있는지 체크한다. */
    if( sParseTree->mDDLType != SHARD_DROP_FORCE )
    {
        IDE_TEST( sdiZookeeper::checkAllNodeAlive( &sIsValid ) != IDE_SUCCESS );

        IDE_TEST_RAISE( sIsValid == ID_FALSE, deadNode_exist );

        /* DROP Force 일 경우 Reference Check도 SKIP 하도록 한다. 무조건 지워야 하니까. */
        /* 기본적으로 변경 대상 노드가 존재하는지 확인하는 작업은 validateState 함수에서 처리하나
         * ADD/DROP 구문은 validateState를 수행하지 않으므로 직접 체크해야 한다. */
        if( QC_IS_NULL_NAME( sParseTree->mNodeName ) == ID_FALSE )
        {
            QC_STR_COPY( sNodeName, sParseTree->mNodeName );

            idlOS::strncpy( sPath,
                            SDI_ZKC_PATH_NODE_META,
                            SDI_ZKC_PATH_LENGTH );
            idlOS::strncat( sPath,
                            "/",
                            ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );
            idlOS::strncat( sPath,
                            sNodeName,
                            ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );

            IDE_TEST( sdiZookeeper::checkPath( sPath,
                                               &sIsValid ) != IDE_SUCCESS );

            IDE_TEST_RAISE( sIsValid == ID_FALSE, invalid_nodeName );
        }
        else
        {
            // nothing to do
        }
    }
    else
    {
        /* DROP FORCE */

        /* DROP FORCE는 All node alive check를 하지 않는다. Target은 항상 Failover 되어 있어야 하기 때문 */
        if ( QC_IS_NULL_NAME( sParseTree->mNodeName ) == ID_FALSE )
        {
            QC_STR_COPY( sNodeName, sParseTree->mNodeName );

            idlOS::strncpy( sPath,
                            SDI_ZKC_PATH_NODE_META,
                            SDI_ZKC_PATH_LENGTH );
            idlOS::strncat( sPath,
                            "/",
                            ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );
            idlOS::strncat( sPath,
                            sNodeName,
                            ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );

            IDE_TEST( sdiZookeeper::checkPath( sPath,
                                               &sIsValid ) != IDE_SUCCESS );

            IDE_TEST_RAISE( sIsValid == ID_FALSE, invalid_nodeName );
           
            /* DROP FORCE는 Local 수행하지 않는다. */
            IDE_TEST_RAISE( sdi::isSameNode( sNodeName, 
                                             sdiZookeeper::mMyNodeName )
                            == ID_TRUE, invalid_nodeName );

            sDataLength = SDI_ZKC_BUFFER_SIZE;
            IDE_TEST( sdiZookeeper::getNodeInfo( sNodeName,
                                                 (SChar*)SDI_ZKC_PATH_NODE_FAILOVER_TO,
                                                 sBuffer,
                                                 &sDataLength ) != IDE_SUCCESS );

            IDE_TEST_RAISE( sDataLength <= 0 , Not_failed_over );

            IDE_TEST( sdiZookeeper::checkRecentDeadNode( sRecentDeadNode, NULL, NULL ) 
                      != IDE_SUCCESS );

            IDE_TEST_RAISE( idlOS::strncmp( sNodeName,
                                            sRecentDeadNode,
                                            idlOS::strlen( sRecentDeadNode ) ) != 0, not_my_turn );

            IDE_TEST( sdiZookeeper::checkExistNotFailedoverDeadNode( &sExistNotFailedoverDeadNode ) 
                      != IDE_SUCCESS );

            IDE_TEST_RAISE( sExistNotFailedoverDeadNode != ID_FALSE, EXIST_NOT_FAILEDOVER_DEADNODE );

        }
        else
        {
            /* DROP FORCE는 Local 수행하지 않는다. */
            IDE_RAISE( invalid_nodeName );
        }
    }
    
    if ( sIsAcquireShardMetaLock == ID_TRUE )
    {
        sdiZookeeper::releaseShardMetaLock();
        sIsAcquireShardMetaLock = ID_FALSE;
    }

    if ( sIsAcquireZookeeperMetaLock == ID_TRUE )
    {
        sdiZookeeper::releaseZookeeperMetaLock();
        sIsAcquireZookeeperMetaLock = ID_FALSE;
    }

        ideLog::log(IDE_SD_0,"[SHARD_DROP_FORCE] VALIDATION SUCCESS");

    return IDE_SUCCESS;

    IDE_EXCEPTION( EXIST_NOT_FAILEDOVER_DEADNODE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_EXIST_NOT_FAILEDOVER_DEADNODE ));
    }
    IDE_EXCEPTION( not_my_turn )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_DROPFORCE_NOT_MY_TURN ) ); 
    }
    IDE_EXCEPTION( Not_failed_over )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_TARGET_NODE_NOT_FAILED_OVER ) );
    }
    IDE_EXCEPTION( connect_Fail )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_ZKC_CONNECTION_FAIL ) );
    }
    IDE_EXCEPTION( deadNode_exist )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_ZKC_DEADNODE_EXIST ) );
    }
    IDE_EXCEPTION( invalid_nodeName )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_INVALID_NODE_NAME, sNodeName ) );
    }
    IDE_EXCEPTION_END;

    // the logic below must be move to commit or rollback.
    if ( sIsAcquireShardMetaLock == ID_TRUE )
    {
        sdiZookeeper::releaseShardMetaLock();
        sIsAcquireShardMetaLock = ID_FALSE;
    }

    if ( sIsAcquireZookeeperMetaLock == ID_TRUE )
    {
        sdiZookeeper::releaseZookeeperMetaLock();
        sIsAcquireZookeeperMetaLock = ID_FALSE;
    }

        ideLog::log(IDE_SD_0,"[SHARD_DROP_FORCE] VALIDATION FAILED");


    return IDE_FAILURE;
}

IDE_RC qdsd::executeShardDrop( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *    ALTER DATABASE SHARD DROP {'node_name'} [ FORCE ] ;
 *
 * Implementation :
 *    1. 
 ***********************************************************************/

    qdShardParseTree  * sParseTree = NULL;
    SChar               sSqlStr[QD_MAX_SQL_LENGTH + 1];
    SChar               sNodeName[SDI_NODE_NAME_MAX_SIZE + 1];
    sdiLocalMetaInfo    sLocalMetaInfo;
    idBool              sIsRemoteNode = ID_FALSE;

    SInt                sDataLength;
    SChar               sBuffer[SDI_ZKC_BUFFER_SIZE] = {0,};
    SChar               sRecentDeadNode[SDI_NODE_NAME_MAX_SIZE + 1] = {0,};
    idBool              sExistNotFailedoverDeadNode = ID_FALSE;

    SChar               sFailoverToNodeName[SDI_NODE_NAME_MAX_SIZE + 1];

    ZKState             sZKState = ZK_NONE;

    ULong               sFailbackSMN = 0;
    ULong               sFailoveredSMN = 0;

    ULong               sOldSMN = 0;

    iduList           * sList;
    idBool              sIsAllocList = ID_FALSE;

    sdiClientInfo     * sClientInfo  = NULL;

    sdiReplicaSetInfo   sFailoverHistoryInfo;
    sdiReplicaSetInfo   sFailoverHistoryInfoWithOtherNode;

    sdiReplicaSetInfo   sReplicaSetInfo;

    sdiReplicaSet     * sMainReplicaSet = NULL;
    sdiReplicaSet     * sReplicaSet = NULL;

    SChar               sMyNextNodeName[SDI_NODE_NAME_MAX_SIZE + 1];

    UInt                i;
    UInt                j;
    UInt                sExecCount;
    ULong               sCount = 0;
    idBool              sFetchResult = ID_FALSE;

    SChar               sReverseReplName[SDI_REPLICATION_NAME_MAX_SIZE + 1];
    SChar               sRemoveReplName[SDI_REPLICATION_NAME_MAX_SIZE + 1];

    SChar             * sUserName;

    sdiNodeInfo         sNodeInfo;
    sdiNode           * sTargetNodeInfo = NULL;

    sdiTableInfoList  * sTableInfoList = NULL;

    sdiTableInfoList  * sTmpTableInfoList = NULL;    

    sdiTableInfo      * sTableInfo  = NULL;
    sdiRangeInfo        sRangeInfos;

    sdiShardObjectType  sShardTableType = SDI_NON_SHARD_OBJECT;

    sdiValue            sValueStr;

    SChar               sObjectValue[SDI_RANGE_VARCHAR_MAX_PRECISION + 1];

    sdiLocalMetaInfo  * sNodeMetaInfo = NULL;

    idBool              sIsAlive = ID_FALSE;

    sParseTree = (qdShardParseTree *)aStatement->myPlan->parseTree;

    sUserName = QCG_GET_SESSION_USER_NAME( aStatement );
    
    if( sParseTree->mDDLType == SHARD_DROP )
    {
        /***********************************************************************
         * check internal property validate
         ***********************************************************************/
        IDE_TEST( validateCommonForExecute( aStatement ) != IDE_SUCCESS );

        /* for smn propagation to all nodes at commit */
        sdi::setShardMetaTouched( aStatement->session );
        IDE_TEST( sdiZookeeper::getZookeeperMetaLock(QCG_GET_SESSION_ID(aStatement)) != IDE_SUCCESS );
        IDE_TEST( sdiZookeeper::getShardMetaLock( QC_SMI_STMT(aStatement)->getTrans()->getTransID() ) != IDE_SUCCESS );

        IDE_TEST( getAllNodeInfoList( aStatement ) != IDE_SUCCESS );

        /***********************************************************************
         * check node_name
         ***********************************************************************/
        IDE_TEST( sdi::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );

        if ( QC_IS_NULL_NAME( sParseTree->mNodeName ) != ID_TRUE )
        {
            QC_STR_COPY( sNodeName, sParseTree->mNodeName );

            if ( idlOS::strncmp( sNodeName,
                                 sLocalMetaInfo.mNodeName,
                                 ( SDI_NODE_NAME_MAX_SIZE + 1 ) ) != 0 )
            {
                sIsRemoteNode = ID_TRUE;
            }
        }
        else
        {
            // nothing to do
        }

        if ( sIsRemoteNode == ID_TRUE )
        {
            
            /***********************************************************************
             * SHARD DROP node_name
             ***********************************************************************/
            IDE_RAISE(ERR_NOT_SUPPORTED_SYNTAX);
            sdiZookeeper::releaseShardMetaLock();
            sdiZookeeper::releaseZookeeperMetaLock();

            aStatement->session->mQPSpecific.mFlag &= ~QC_SESSION_HANDOVER_SHARD_DDL_MASK;
            aStatement->session->mQPSpecific.mFlag |= QC_SESSION_HANDOVER_SHARD_DDL_TRUE;

            idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                             "EXEC DBMS_SHARD.EXECUTE_IMMEDIATE( 'ALTER DATABASE SHARD DROP', '"QCM_SQL_STRING_SKIP_FMT"' ) ",
                             sNodeName );

            IDE_TEST( sdi::shardExecTempDMLOrDCLWithNewTrans( aStatement,
                                                              sSqlStr )
                      != IDE_SUCCESS );
        }
        else
        {
            /***********************************************************************
             * SHARD DROP
             ***********************************************************************/

            /***********************************************************************
             * RESET CLONES_ META
             ***********************************************************************/
            IDE_TEST( executeResetCloneMeta( aStatement ) != IDE_SUCCESS );
    
            /***********************************************************************
             * UNSET_NODE 전파
             ***********************************************************************/
            IDE_TEST( executeUnsetNodes( aStatement ) != IDE_SUCCESS );
            
            /***********************************************************************
             * FAILOVER 처리를 여부 체크를 위한  REPLICA_SETS_ 정보변경
             * replication stop
             ***********************************************************************/
            IDE_TEST( executeReplicationFunction( aStatement,
                                                  QDSD_REPL_QUERY_TYPE_STOP )
                      != IDE_SUCCESS );
            
            /***********************************************************************
             * replication drop
             ***********************************************************************/
            IDE_TEST( executeReplicationFunction( aStatement,
                                                  QDSD_REPL_QUERY_TYPE_DROP )
                      != IDE_SUCCESS );

            /***********************************************************************
             * backup table no item create replication
             ***********************************************************************/
            IDE_TEST( executeReplicationFunction( aStatement,
                                                  QDSD_REPL_QUERY_TYPE_CREATE )
                      != IDE_SUCCESS );

            /***********************************************************************
             * backup table add replication
             ***********************************************************************/
            IDE_TEST( executeReplicationFunction( aStatement,
                                                  QDSD_REPL_QUERY_TYPE_ADD )
                      != IDE_SUCCESS );

            /***********************************************************************
             * replication sync start
             ***********************************************************************/
            IDE_TEST( executeReplicationFunction( aStatement,
                                                  QDSD_REPL_QUERY_TYPE_START )
                      != IDE_SUCCESS );
            
            /***********************************************************************
             * replication flush
             ***********************************************************************/
            IDE_TEST( executeReplicationFunction( aStatement,
                                                  QDSD_REPL_QUERY_TYPE_FLUSH )
                      != IDE_SUCCESS );

            /***********************************************************************
             * TASK-7307 DML Data Consistency in Shard
             *   SYSTEM_.SYS_TABLES_.SHARD_FLAG 업데이트
             ***********************************************************************/
            IDE_TEST( executeAlterShardNone( aStatement ) != IDE_SUCCESS );

            /***********************************************************************
             * Zookeeper meta 1차 변경
             ***********************************************************************/
            IDE_TEST( executeZookeeperDrop( aStatement ) != IDE_SUCCESS );
        }
    }
    else
    {
        IDE_DASSERT( sParseTree->mDDLType == SHARD_DROP_FORCE );

        ideLog::log(IDE_SD_0,"[SHARD_DROP_FORCE] EXECUTE START");

        /***********************************************************************
         * check internal property validate
         ***********************************************************************/
        IDE_TEST( validateCommonForExecute( aStatement ) != IDE_SUCCESS );

        ideLog::log(IDE_SD_0,"[SHARD_DROP_FORCE] EXECUTE VALIDATE SUCCESS");


        /* for smn propagation to all nodes at commit */
        sdi::setShardMetaTouched( aStatement->session );
        IDE_TEST( sdiZookeeper::getZookeeperMetaLock(QCG_GET_SESSION_ID(aStatement)) != IDE_SUCCESS );
        IDE_TEST( sdiZookeeper::getShardMetaLock( QC_SMI_STMT(aStatement)->getTrans()->getTransID() ) != IDE_SUCCESS );

        IDE_TEST( executeZookeeperDrop( aStatement ) != IDE_SUCCESS );

        /***********************************************************************
         * check node_name
         ***********************************************************************/
        IDE_TEST( sdi::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );

        QC_STR_COPY( sNodeName, sParseTree->mNodeName );

        IDE_TEST( sdiZookeeper::checkRecentDeadNode( sRecentDeadNode, &sFailbackSMN, &sFailoveredSMN ) 
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( idlOS::strncmp( sNodeName,
                                        sRecentDeadNode,
                                        idlOS::strlen( sRecentDeadNode ) ) != 0, not_my_turn );

        IDE_TEST( sdiZookeeper::checkExistNotFailedoverDeadNode( &sExistNotFailedoverDeadNode ) 
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( sExistNotFailedoverDeadNode != ID_FALSE, EXIST_NOT_FAILEDOVER_DEADNODE );


        sDataLength = SDI_ZKC_BUFFER_SIZE;
        IDE_TEST( sdiZookeeper::getNodeInfo( sNodeName,
                                             (SChar*)SDI_ZKC_PATH_NODE_FAILOVER_TO,
                                             sBuffer,
                                             &sDataLength ) != IDE_SUCCESS );

        IDE_TEST_RAISE( sDataLength <= 0 , Not_failed_over );

        idlOS::strncpy( sFailoverToNodeName,
                        sBuffer,
                        SDI_NODE_NAME_MAX_SIZE + 1 );

        sOldSMN = sdi::getSMNForDataNode();

        /* get Alive Node List */
        IDE_TEST( sdiZookeeper::getAliveNodeNameList( &sList ) != IDE_SUCCESS );
        sIsAllocList = ID_TRUE;

        /* AliveNode들만 대상으로 ShardLinker 생성해야 한다. Node Stop 후에 호출되어야 함. */
        IDE_TEST( sdi::checkShardLinkerWithNodeList ( aStatement,
                                                      sOldSMN,
                                                      NULL,
                                                      sList ) != IDE_SUCCESS );

        sClientInfo = aStatement->session->mQPSpecific.mClientInfo;

        IDE_DASSERT( sClientInfo != NULL );

        IDE_TEST_RAISE( sClientInfo == NULL, ERR_NODE_NOT_EXIST );

        IDE_TEST( sdm::getFailoverHistoryWithPrimaryNodename( QC_SMI_STMT( aStatement),
                                                              &sFailoverHistoryInfo,
                                                              sNodeName,
                                                              sFailbackSMN ) != IDE_SUCCESS );

        IDE_TEST( sdm::getFailoverHistoryWithSMN( QC_SMI_STMT( aStatement),
                                                  &sFailoverHistoryInfoWithOtherNode,
                                                  sFailbackSMN ) != IDE_SUCCESS );

        /* get ReplicaSet Info */
        IDE_TEST( sdm::getAllReplicaSetInfoSortedByPName( QC_SMI_STMT( aStatement ),
                                                          &sReplicaSetInfo,
                                                          sOldSMN )
                  != IDE_SUCCESS );

        /* Find Target's MainReplicaSet ID */
        IDE_TEST( sdiZookeeper::getNextNode( sNodeName, 
                                             sMyNextNodeName, 
                                             &sZKState ) != IDE_SUCCESS );

        IDE_TEST( getAllNodeInfoList( aStatement ) != IDE_SUCCESS );

        sNodeMetaInfo = sdiZookeeper::getNodeInfo( sParseTree->mNodeInfoList,
                                                   sNodeName );

        for ( i = 0; i < sReplicaSetInfo.mCount; i++ )
        {
            if ( sReplicaSetInfo.mReplicaSets[i].mReplicaSetId == sNodeMetaInfo->mShardNodeId )
            {
                sMainReplicaSet = &sReplicaSetInfo.mReplicaSets[i];
                break;
            }
        }
        IDE_DASSERT( sMainReplicaSet != NULL );

        /* ResetShardMeta */
        ideLog::log(IDE_SD_0,"[SHARD_DROP_FORCE] RESET SHARD META START");

        IDE_TEST( sdi::getExternalNodeInfo( &sNodeInfo,
                                            sOldSMN )
                  != IDE_SUCCESS );

        for ( i = 0; i < sNodeInfo.mCount; i++ )
        {
            if ( idlOS::strncmp( sNodeInfo.mNodes[i].mNodeName,
                                 sNodeName,
                                 SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
            {
                sTargetNodeInfo = &sNodeInfo.mNodes[i];
            }
        }

        /* TargetNodeInfo가 Null 이면 이미 Unset 된 상황이다. 더이상 할게 없다. */
        if ( sTargetNodeInfo != NULL )
        {
            ideLog::log(IDE_SD_0,"[SHARD_DROP_FORCE] TARGET NODE EXIST");

            /* get All TableInfo */
            IDE_TEST( sdi::getTableInfoAllObject( aStatement,
                                                  &sTableInfoList,
                                                  sOldSMN )
                      != IDE_SUCCESS );

            for ( sTmpTableInfoList = sTableInfoList;
                  sTmpTableInfoList != NULL;
                  sTmpTableInfoList = sTmpTableInfoList->mNext )

            {
                sTableInfo = sTmpTableInfoList->mTableInfo;

                ideLog::log(IDE_SD_0,"[SHARD_DROP_FORCE] TABLE META CHANGE : %s", sTableInfo->mObjectName );


                /* Default Partition check */
                if ( sTableInfo->mDefaultNodeId == sTargetNodeInfo->mNodeId )
                {
                    /* Node 제거를 위해 Data 제거 해야 하는데 아예 제거 해버리면 
                     * Failback 불가능 해지므로 ReplicaSetId와의 연결을  남길수 있도록 
                     * Dealloc Name으로 셋팅해둔다. */
                    /* Send resetShardPartitioinNode to All Alive Node */
                    if ( sTableInfo->mObjectType == 'T' ) /* Table */
                    {
                        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                         "exec dbms_shard.reset_shard_partition_node( '"
                                         QCM_SQL_STRING_SKIP_FMT"', '"
                                         QCM_SQL_STRING_SKIP_FMT"', '"
                                         QCM_SQL_STRING_SKIP_FMT"', "
                                         QCM_SQL_VARCHAR_FMT", "
                                         QCM_SQL_VARCHAR_FMT" )",
                                         sTableInfo->mUserName,
                                         sTableInfo->mObjectName,
                                         sNodeName,
                                         SDI_NODE_DEALLOC_NAME,
                                         sTableInfo->mDefaultPartitionName );
                    }
                    else /* Procedure */
                    {
                        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                         "exec dbms_shard.reset_shard_resident_node( '"
                                         QCM_SQL_STRING_SKIP_FMT"', '"
                                         QCM_SQL_STRING_SKIP_FMT"', '"
                                         QCM_SQL_STRING_SKIP_FMT"', "
                                         QCM_SQL_VARCHAR_FMT", "
                                         QCM_SQL_VARCHAR_FMT", "
                                         QCM_SQL_VARCHAR_FMT", "
                                         QCM_SQL_UINT32_FMT" )",
                                         sTableInfo->mUserName,
                                         sTableInfo->mObjectName,
                                         sNodeName,
                                         SDI_NODE_DEALLOC_NAME,
                                         "",
                                         "",
                                         1 );
                    }

                    IDE_TEST( executeSQLNodeList( aStatement,
                                                  SDI_INTERNAL_OP_DROPFORCE,
                                                  QCI_STMT_MASK_SP,
                                                  sSqlStr,
                                                  NULL,
                                                  sList ) != IDE_SUCCESS );

                }

                /* Range Info Check */
                IDE_TEST( sdm::getRangeInfo( aStatement,
                                             QC_SMI_STMT( aStatement ),
                                             sOldSMN,
                                             sTableInfo,
                                             &sRangeInfos,
                                             ID_FALSE )
                          != IDE_SUCCESS );

                /* check Table Type */
                sShardTableType = sdi::getShardObjectType( sTableInfo );

                for ( j = 0; j < sRangeInfos.mCount; j++ )
                {

                    if ( sRangeInfos.mRanges[j].mNodeId == sTargetNodeInfo->mNodeId )
                    {
                        switch ( sShardTableType )
                        {
                            case SDI_SINGLE_SHARD_KEY_DIST_OBJECT:
                            case SDI_SOLO_DIST_OBJECT:
                                /* send resetShardPartitionNode to All Alive Node */
                                if ( sTableInfo->mObjectType == 'T' ) /* Table */
                                {
                                    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                                     "exec dbms_shard.reset_shard_partition_node( '"
                                                     QCM_SQL_STRING_SKIP_FMT"', '"
                                                     QCM_SQL_STRING_SKIP_FMT"', '"
                                                     QCM_SQL_STRING_SKIP_FMT"', "
                                                     QCM_SQL_VARCHAR_FMT", "
                                                     QCM_SQL_VARCHAR_FMT" )",
                                                     sTableInfo->mUserName,
                                                     sTableInfo->mObjectName,
                                                     sNodeName,
                                                     SDI_NODE_DEALLOC_NAME,
                                                     sRangeInfos.mRanges[j].mPartitionName );
                                }
                                else /* Procedure */
                                {
                                    if ( sTableInfo->mSplitMethod == SDI_SPLIT_HASH )
                                    {
                                        IDE_TEST( sdi::getValueStr( MTD_INTEGER_ID,
                                                                    &sRangeInfos.mRanges[j].mValue,
                                                                    &sValueStr )
                                                  != IDE_SUCCESS );
                                    }
                                    else
                                    {
                                        IDE_TEST( sdi::getValueStr( sTableInfo->mKeyDataType,
                                                                    &sRangeInfos.mRanges[j].mValue,
                                                                    &sValueStr )
                                                  != IDE_SUCCESS );
                                    }

                                    if ( sValueStr.mCharMax.value[0] == '\'' )
                                    {
                                        // INPUT ARG ('''A''') => 'A' => '''A'''
                                        idlOS::snprintf( sObjectValue,
                                                         SDI_RANGE_VARCHAR_MAX_PRECISION + 1,
                                                         "''%s''",
                                                         sValueStr.mCharMax.value );
                                    }
                                    else
                                    {
                                        // INPUT ARG ('A') => A => 'A'
                                        idlOS::snprintf( sObjectValue,
                                                         SDI_RANGE_VARCHAR_MAX_PRECISION + 1,
                                                         "'%s'",
                                                         sValueStr.mCharMax.value );
                                    }

                                    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                                     "exec dbms_shard.reset_shard_resident_node( '"
                                                     QCM_SQL_STRING_SKIP_FMT"', '"
                                                     QCM_SQL_STRING_SKIP_FMT"', '"
                                                     QCM_SQL_STRING_SKIP_FMT"', "
                                                     QCM_SQL_VARCHAR_FMT", "
                                                     "%s, "
                                                     QCM_SQL_VARCHAR_FMT" )",
                                                     sTableInfo->mUserName,
                                                     sTableInfo->mObjectName,
                                                     sNodeName,
                                                     SDI_NODE_DEALLOC_NAME,
                                                     sObjectValue,
                                                     "" );
                                }

                                IDE_TEST( executeSQLNodeList( aStatement,
                                                              SDI_INTERNAL_OP_DROPFORCE,
                                                              QCI_STMT_MASK_SP,
                                                              sSqlStr,
                                                              NULL,
                                                              sList ) != IDE_SUCCESS );
                                break;
                            case SDI_COMPOSITE_SHARD_KEY_DIST_OBJECT:
                                /* Composite는 지원하지 않는다. */
                                continue;
                            case SDI_CLONE_DIST_OBJECT:
                                /* Clones_ 은 Failover 수행시 이미 제거 했다. */
                                continue;
                            case SDI_NON_SHARD_OBJECT:
                            default:
                                /* Non Shard Table이 오면 안됨. */
                                IDE_DASSERT( 0 );
                                break;
                        }


                    }
                }
            }

            idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                             "EXEC DBMS_SHARD.UNSET_NODE( '"QCM_SQL_STRING_SKIP_FMT"' ); ",
                             sNodeName );

            /* Local Node의 SetNode는 따로 수행했으므로 Exception 으로 제외한다. */
            IDE_TEST( executeSQLNodeList( aStatement,
                                          SDI_INTERNAL_OP_DROPFORCE,
                                          QCI_STMT_MASK_SP,
                                          sSqlStr,
                                          NULL,
                                          sList ) != IDE_SUCCESS );
        }

        ideLog::log(IDE_SD_0,"[SHARD_DROP_FORCE] RESET SHARDMETA COMPLETE");

        ideLog::log(IDE_SD_0,"[SHARD_DROP_FORCE] REORG REPLICATION START");

        /* Organize Replication */
        /* FailoverToNode로 부터 TargetNode로의 ReverseRP를 제거한다. */
        /* ReverseRP를 생성했다는 것은 TargetNode로 부터 Receive 중인 RP가 있다는 뜻이다.
         * 해당 RP 중 제거되는 RP를 판단하여 같이 제거 해야 한다.(RemoveReplName) */
        for ( i = 0 ; i < sFailoverHistoryInfo.mCount; i++ )
        {
            sReplicaSet = &sFailoverHistoryInfo.mReplicaSets[i];

            if ( idlOS::strncmp( sReplicaSet->mFirstBackupNodeName,
                                 sFailoverToNodeName,
                                 SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
            {
                idlOS::snprintf( sReverseReplName, 
                                 SDI_REPLICATION_NAME_MAX_SIZE + 1,
                                 "R_%s", 
                                 sReplicaSet->mFirstReplName );

                /* First가 FailoverToNode이면 지워야 하는 RP 이다. */
                idlOS::snprintf( sRemoveReplName, 
                                 SDI_REPLICATION_NAME_MAX_SIZE + 1,
                                 "%s", 
                                 sReplicaSet->mFirstReplName );

            }
            else if ( idlOS::strncmp( sReplicaSet->mSecondBackupNodeName,
                                      sFailoverToNodeName,
                                      SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )

            {
                idlOS::snprintf( sReverseReplName, 
                                 SDI_REPLICATION_NAME_MAX_SIZE + 1,
                                 "R_%s", 
                                 sReplicaSet->mSecondReplName );

                /* Second가 FailoverTo 인경우
                 * 1. First가 Failover되어 있어서 Second로 Failover 된 경우 : Remove 대상
                 * 2. 한번 Failover되어 Second로 Failover 된 경우 : Remove 대상 아님
                 * 2의 경우가 아니면 Remove 한다. */
                if ( idlOS::strncmp( sReplicaSet->mStopFirstBackupNodeName,
                                     sNodeName,
                                     SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
                {
                    idlOS::snprintf( sRemoveReplName,
                                     SDI_REPLICATION_NAME_MAX_SIZE + 1,
                                     SDM_NA_STR );
                }
                else
                {
                    idlOS::snprintf( sRemoveReplName, 
                                     SDI_REPLICATION_NAME_MAX_SIZE + 1,
                                     "%s", 
                                     sReplicaSet->mSecondReplName );
                }
            }
            else
            {
                idlOS::snprintf( sReverseReplName,
                                 SDI_REPLICATION_NAME_MAX_SIZE + 1,
                                 SDM_NA_STR );

                idlOS::snprintf( sRemoveReplName,
                                 SDI_REPLICATION_NAME_MAX_SIZE + 1,
                                 SDM_NA_STR );
            }

            if ( idlOS::strncmp( sReverseReplName,
                                 SDM_NA_STR,
                                 SDI_REPLICATION_NAME_MAX_SIZE + 1 ) != 0 )
            {
                idlOS::snprintf( sSqlStr, 
                                 QD_MAX_SQL_LENGTH + 1,
                                 "select Count(*) from system_.sys_replications_ where replication_name='%s' ",
                                 sReverseReplName );

                IDE_TEST( sdi::shardExecDirect( aStatement,
                                                sFailoverToNodeName,
                                                (SChar*)sSqlStr,
                                                (UInt) idlOS::strlen(sSqlStr),
                                                SDI_INTERNAL_OP_DROPFORCE,
                                                &sExecCount,
                                                &sCount,
                                                NULL,
                                                ID_SIZEOF(sCount),
                                                &sFetchResult )
                          != IDE_SUCCESS );

                if ( sCount != 0 )
                {
                    /* Reverse RP와 RemoveRP 모두 TxDDL로 처리한다. */
                    idlOS::snprintf(sSqlStr,
                                    QD_MAX_SQL_LENGTH,
                                    "drop replication "QCM_SQL_STRING_SKIP_FMT,
                                    sReverseReplName );

                    IDE_TEST( executeRemoteSQL( aStatement, 
                                                sFailoverToNodeName,
                                                SDI_INTERNAL_OP_DROPFORCE,
                                                QCI_STMT_MASK_DDL,
                                                sSqlStr,
                                                QD_MAX_SQL_LENGTH,
                                                NULL )
                                != IDE_SUCCESS );

                }

                if ( idlOS::strncmp( sRemoveReplName,
                                     SDM_NA_STR,
                                     SDI_REPLICATION_NAME_MAX_SIZE + 1 ) != 0 )
                {
                    idlOS::snprintf(sSqlStr,
                                    QD_MAX_SQL_LENGTH,
                                    "drop replication "QCM_SQL_STRING_SKIP_FMT,
                                    sRemoveReplName );

                    IDE_TEST( executeRemoteSQL( aStatement, 
                                                sFailoverToNodeName,
                                                SDI_INTERNAL_OP_DROPFORCE,
                                                QCI_STMT_MASK_DDL,
                                                sSqlStr,
                                                QD_MAX_SQL_LENGTH,
                                                NULL )
                                != IDE_SUCCESS );
                }
            }
        }

        /* TargetNode가 Receive 중이던 RP들을 찾아서 DROP 해주어야 한다. */
        for ( i = 0 ; i < sFailoverHistoryInfoWithOtherNode.mCount; i++ )
        {
            sReplicaSet = &sFailoverHistoryInfoWithOtherNode.mReplicaSets[i];

            if ( idlOS::strncmp( sReplicaSet->mFirstBackupNodeName,
                                 sNodeName,
                                 SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
            {
                idlOS::snprintf( sRemoveReplName, 
                                 SDI_REPLICATION_NAME_MAX_SIZE + 1,
                                 "%s", 
                                 sReplicaSet->mFirstReplName );
            }
            else if ( idlOS::strncmp( sReplicaSet->mSecondBackupNodeName,
                                 sNodeName,
                                 SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
            {
                idlOS::snprintf( sRemoveReplName, 
                                 SDI_REPLICATION_NAME_MAX_SIZE + 1,
                                 "%s", 
                                 sReplicaSet->mSecondReplName );
            }
            else
            {
                idlOS::snprintf( sRemoveReplName,
                                 SDI_REPLICATION_NAME_MAX_SIZE + 1,
                                 SDM_NA_STR );
            }

            if ( idlOS::strncmp( sRemoveReplName,
                                 SDM_NA_STR,
                                 SDI_REPLICATION_NAME_MAX_SIZE + 1 ) != 0 )
            {

                IDE_TEST( sdiZookeeper::checkNodeAlive( sReplicaSet->mPrimaryNodeName,
                                                        &sIsAlive ) != IDE_SUCCESS );

                if ( sIsAlive == ID_TRUE )
                {
                    idlOS::snprintf( sSqlStr, 
                                     QD_MAX_SQL_LENGTH + 1,
                                     "select Count(*) from system_.sys_replications_ where replication_name='%s' ",
                                     sRemoveReplName );

                    IDE_TEST( sdi::shardExecDirect( aStatement,
                                                    sReplicaSet->mPrimaryNodeName,
                                                    (SChar*)sSqlStr,
                                                    (UInt) idlOS::strlen(sSqlStr),
                                                    SDI_INTERNAL_OP_DROPFORCE,
                                                    &sExecCount,
                                                    &sCount,
                                                    NULL,
                                                    ID_SIZEOF(sCount),
                                                    &sFetchResult )
                              != IDE_SUCCESS );

                    if ( sCount != 0 )
                    {
                        idlOS::snprintf(sSqlStr,
                                        QD_MAX_SQL_LENGTH,
                                        "drop replication "QCM_SQL_STRING_SKIP_FMT,
                                        sRemoveReplName );

                        IDE_TEST( executeRemoteSQL( aStatement, 
                                                    sReplicaSet->mPrimaryNodeName,
                                                    SDI_INTERNAL_OP_DROPFORCE,
                                                    QCI_STMT_MASK_DDL,
                                                    sSqlStr,
                                                    QD_MAX_SQL_LENGTH,
                                                    NULL )
                                    != IDE_SUCCESS );
                    }
                }
            }
        }

        ideLog::log(IDE_SD_0,"[SHARD_DROP_FORCE] REORG REPLICATION COMPLETE");

        /* ResetReplicaSet */
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "exec dbms_shard.reset_ReplicaSets( '"QCM_SQL_STRING_SKIP_FMT"', '"
                                                              QCM_SQL_STRING_SKIP_FMT"', '"
                                                              QCM_SQL_STRING_SKIP_FMT"' )",
                         sUserName,
                         sMyNextNodeName, /* 제거 이기 때문에 Old 값은 사용하지 않는다. */
                         sNodeName );

        IDE_TEST( executeSQLNodeList( aStatement,
                                      SDI_INTERNAL_OP_DROPFORCE,
                                      QCI_STMT_MASK_SP,
                                      sSqlStr,
                                      NULL,
                                      sList ) != IDE_SUCCESS );

        ideLog::log(IDE_SD_0,"[SHARD_DROP_FORCE] RESET REPLICASET COMPLETE");

        /* ResetFailoverHistory */
        /* FailoverHistory에 있는 모든 SMN 에 대해서 처리 해야 하는데 어떻게 해야 할까 */

        ideLog::log(IDE_SD_0,"[SHARD_DROP_FORCE] RESET FAILOVERHISTORY START");

        /* 1차 FailbackSMN 에 일치하는 FailoverHistory는 제거
         * 해당 정보를 이용하여 Failback Sync를 판단할 예정 추후에 제거 해야 함. */
        /* FailbackSMN과 일치하는 FailoverHistory는 일반 Failback과 Failback Sync/Instant를
         * 구분하기 위해 남겨두도록 한다.
         * Drop Force는 가장 최신의 DeadNode를 대상으로만 작동하기 때문에
         * 해당 SMN보다 작은 FailbackSMN을 가진 Failback은 Failback Sync/Instant를 사용해야만 하고
         * DropForce 이후 동작한 Failover 등은 DropForce 상황에 맞추어 Failover 되어 있기 때문에
         * 일반 Failback 을 수행할 수 있다. */
        if ( sFailoverHistoryInfoWithOtherNode.mCount != 0 )
        { 
/*            idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                             "DELETE FROM SYS_SHARD.FAILOVER_HISTORY_ WHERE SMN = "QCM_SQL_BIGINT_FMT, 
                             sFailbackSMN );

            IDE_TEST( executeSQLNodeList( aStatement,
                                          SDI_INTERNAL_OP_DROPFORCE,
                                          QCI_STMT_MASK_SP,
                                          sSqlStr,
                                          NULL,
                                          sList ) != IDE_SUCCESS ); */
        }

        /* 2차 TargetNode를 대상으로 하고 있던 정보 모두 변경 */
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "UPDATE SYS_SHARD.FAILOVER_HISTORY_ "
                         "   SET FIRST_BACKUP_NODE_NAME     = "QCM_SQL_VARCHAR_FMT", "
                         "   STOP_FIRST_BACKUP_NODE_NAME    = "QCM_SQL_VARCHAR_FMT
                         "   WHERE FIRST_BACKUP_NODE_NAME   = "QCM_SQL_VARCHAR_FMT
                         "   OR STOP_FIRST_BACKUP_NODE_NAME = "QCM_SQL_VARCHAR_FMT,
                         SDM_NA_STR,
                         SDM_NA_STR,
                         sNodeName,
                         sNodeName );

        IDE_TEST( executeSQLNodeList( aStatement,
                                      SDI_INTERNAL_OP_DROPFORCE,
                                      QCI_STMT_MASK_SP,
                                      sSqlStr,
                                      NULL,
                                      sList ) != IDE_SUCCESS );

        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "UPDATE SYS_SHARD.FAILOVER_HISTORY_ "
                         "   SET SECOND_BACKUP_NODE_NAME     = "QCM_SQL_VARCHAR_FMT", "
                         "   STOP_SECOND_BACKUP_NODE_NAME    = "QCM_SQL_VARCHAR_FMT
                         "   WHERE SECOND_BACKUP_NODE_NAME   = "QCM_SQL_VARCHAR_FMT
                         "   OR STOP_SECOND_BACKUP_NODE_NAME = "QCM_SQL_VARCHAR_FMT,
                         SDM_NA_STR,
                         SDM_NA_STR,
                         sNodeName,
                         sNodeName );

        IDE_TEST( executeSQLNodeList( aStatement,
                                      SDI_INTERNAL_OP_NORMAL,
                                      QCI_STMT_MASK_SP,
                                      sSqlStr,
                                      NULL,
                                      sList ) != IDE_SUCCESS );

        ideLog::log(IDE_SD_0,"[SHARD_DROP_FORCE] RESET FAILOVERHISTORY COMPLETE");

        sIsAllocList = ID_FALSE;
        sdiZookeeper::freeList( sList, SDI_ZKS_LIST_NODENAME );    
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NODE_NOT_EXIST )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_NODE_NOT_EXIST ) );
    }
    IDE_EXCEPTION( EXIST_NOT_FAILEDOVER_DEADNODE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_EXIST_NOT_FAILEDOVER_DEADNODE ));
    }
    IDE_EXCEPTION( Not_failed_over )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_TARGET_NODE_NOT_FAILED_OVER ) );
    }
    IDE_EXCEPTION( not_my_turn )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_DROPFORCE_NOT_MY_TURN ) ); 
    }
    IDE_EXCEPTION( ERR_NOT_SUPPORTED_SYNTAX );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_SYNTAX_ERROR_SHARD_DROP, sNodeName) );
    }
    IDE_EXCEPTION_END;

    if( sIsAllocList == ID_TRUE )
    {
        sdiZookeeper::freeList( sList, SDI_ZKS_LIST_NODENAME );    
    }
    
    return IDE_FAILURE;
}


IDE_RC qdsd::validateShardFailover( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *    ALTER DATABASE SHARD FAILOVER [ NORMAL | IMMEDIATE | EXIT | FORCE ] 'node_name';
 *
 * Implementation :
 *    1.
 ***********************************************************************/

    qdShardParseTree  * sParseTree = NULL;
    SChar               sNodeName[SDI_NODE_NAME_MAX_SIZE + 1];
    SChar               sNextNodeName[SDI_NODE_NAME_MAX_SIZE + 1];
    SChar               sPath[SDI_ZKC_PATH_LENGTH] = {0,};
    idBool              sIsValid = ID_FALSE;

    idBool              sIsAcquireZookeeperMetaLock = ID_FALSE;
    idBool              sIsAcquireShardMetaLock = ID_FALSE;
    sdiLocalMetaInfo    sLocalMetaInfo;


    sParseTree = (qdShardParseTree *)aStatement->myPlan->parseTree;

    /***********************************************************************
     * validateCommon
     ***********************************************************************/

    IDE_TEST( validateCommon( aStatement ) != IDE_SUCCESS );

    /* 샤드 메타를 변경하므로 zookeeper의 샤드 메타 lock을 잡는다. */
    IDE_TEST( sdiZookeeper::getZookeeperMetaLock(QCG_GET_SESSION_ID(aStatement)) != IDE_SUCCESS );
    sIsAcquireZookeeperMetaLock = ID_TRUE;
    
    IDE_TEST( sdiZookeeper::getShardMetaLock( QC_SMI_STMT(aStatement)->getTrans()->getTransID() ) != IDE_SUCCESS );
    sIsAcquireShardMetaLock = ID_TRUE;

    /***********************************************************************
     * validate SHARD FAILOVER
     ***********************************************************************/

    /***********************************************************************
     * Zookeeper validation
     ***********************************************************************/

    /* zookeeper server와 연결이 되어 있는 상태인지 체크한다. */
    IDE_TEST_RAISE( sdiZookeeper::isConnect() != ID_TRUE, connect_Fail );

    if( QC_IS_NULL_NAME( sParseTree->mNodeName ) == ID_FALSE )
    {
        /* 입력받은 Node 값이 현재 내 Node와 같은지 확인한다.*/
        IDE_TEST( sdi::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );

        QC_STR_COPY( sNodeName, sParseTree->mNodeName );

        if ( idlOS::strncmp( sNodeName,
                             sLocalMetaInfo.mNodeName,
                             ( SDI_NODE_NAME_MAX_SIZE + 1 ) ) == 0 )
        {
            /* 내 노드를 내가 직접 FailOver는 하지 않도록 한다.
             * Node Stop도 해야되고 Commit도 해주어야 하니까. */
            IDE_RAISE( same_nodeName );
        }

        /* 다른 노드의 이름을 입력받을 경우 대상 노드가 존재하는지 확인한다. */
        idlOS::strncpy( sPath,
                        SDI_ZKC_PATH_NODE_META,
                        SDI_ZKC_PATH_LENGTH );
        idlOS::strncat( sPath,
                        "/",
                        ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );
        idlOS::strncat( sPath,
                        sNodeName,
                        ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );

        IDE_TEST( sdiZookeeper::checkPath( sPath,
                                           &sIsValid ) != IDE_SUCCESS );

        IDE_TEST_RAISE( sIsValid == ID_FALSE, invalid_nodeName );
    }
    else
    {
        /* Failover Target NodeName은 항상 들어와야 한다. */
        IDE_RAISE( invalid_nodeName );
    }

    /* Failover를 수행할 Node의 이름을 가져 온다. */
    IDE_TEST( sdiZookeeper::getNextAliveNode( sNodeName,
                                              sNextNodeName ) != IDE_SUCCESS );


    /* Failover 수행할 Node의 상태 확인 */
    IDE_TEST( sdiZookeeper::validateState( ZK_FAILOVER,
                                           sdiZookeeper::mMyNodeName ) != IDE_SUCCESS );

    IDE_TEST( sdiZookeeper::validateState( ZK_FAILOVER,
                                           sNextNodeName ) != IDE_SUCCESS );

    /***********************************************************************
     * check SMN
     ***********************************************************************/
    IDE_TEST( checkZookeeperSMNAndDataSMN() != IDE_SUCCESS );
    
    if ( sIsAcquireShardMetaLock == ID_TRUE )
    {
        sdiZookeeper::releaseShardMetaLock();
        sIsAcquireShardMetaLock = ID_FALSE;
    }
    
    if ( sIsAcquireZookeeperMetaLock == ID_TRUE )
    {
        sdiZookeeper::releaseZookeeperMetaLock();
        sIsAcquireZookeeperMetaLock = ID_FALSE;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( connect_Fail )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_ZKC_CONNECTION_FAIL ) );
    }
    IDE_EXCEPTION( invalid_nodeName )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_INVALID_NODE_NAME, sNodeName ) );
    }
    IDE_EXCEPTION( same_nodeName )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_INVALID_FAILOVER_NODE_NAME, sNodeName ) );
    }

    IDE_EXCEPTION_END;

    if ( sIsAcquireShardMetaLock == ID_TRUE )
    {
        sdiZookeeper::releaseShardMetaLock();
        sIsAcquireShardMetaLock = ID_FALSE;
    }
    
    if ( sIsAcquireZookeeperMetaLock == ID_TRUE )
    {
        sdiZookeeper::releaseZookeeperMetaLock();
        sIsAcquireZookeeperMetaLock = ID_FALSE;
    }

    return IDE_FAILURE;
}

IDE_RC qdsd::executeShardFailover( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *    ALTER DATABASE SHARD FAILOVER [ NORMAL | IMMEDIATE | EXIT | FORCE ] 'node_name';
 *
 * Implementation :
 *    1. 
 ***********************************************************************/

    qdShardParseTree  * sParseTree = NULL;
    idBool              sIsTargetAlive = ID_FALSE;

    SChar             * sUserName;
    SChar               sTargetNodeName[SDI_NODE_NAME_MAX_SIZE + 1];
    SChar               sFailoverNodeName[SDI_NODE_NAME_MAX_SIZE + 1] = {0,};
    SChar               sFailoverNextNodeName[SDI_NODE_NAME_MAX_SIZE + 1];

    SChar               sFirstStopNodeName[SDI_NODE_NAME_MAX_SIZE + 1];
    SChar               sSecondStopNodeName[SDI_NODE_NAME_MAX_SIZE + 1];

    SChar               sFirstStopReplName[SDI_REPLICATION_NAME_MAX_SIZE + 1];
    SChar               sSecondStopReplName[SDI_REPLICATION_NAME_MAX_SIZE + 1];

    SChar               sReverseReplName[SDI_REPLICATION_NAME_MAX_SIZE + 1];
    SChar               sFailoverCreatedReplName[SDI_REPLICATION_NAME_MAX_SIZE + 1];

    sdiClientInfo     * sClientInfo  = NULL;
    SChar               sSqlStr[QD_MAX_SQL_LENGTH + 1];

    ULong               sOldSMN = 0;

    iduList           * sList;
    idBool              sIsAllocList = ID_FALSE;

    UInt                i;

    sdiReplicaSetInfo   sReplicaSetInfo;

    sdiReplicaSet     * sOldNodeReplicaSet = NULL;

    sdiReplicaSet     * sFirstToOldReplicaSet = NULL;
    sdiReplicaSet     * sSecondToOldReplicaSet = NULL;

    sdiNode             sOldNodeInfo;
    sdiNode             sNewNodeInfo;

    sdiTableInfoList  * sTableInfoList = NULL;

    sdiLocalMetaInfo  * sFailoverNextLocalMetaInfo = NULL;
    sdiLocalMetaInfo  * sNodeMetaInfo = NULL;

    SInt                sDataLength;
    SChar               sBuffer[SDI_ZKC_BUFFER_SIZE] = {0,};

    ZKState             sState = ZK_NONE;

    idBool              sIsUnsetNode = ID_TRUE;

    UInt                sExecCount = 0;
    ULong               sCount = 0;
    idBool              sFetchResult = ID_FALSE;

    sParseTree = (qdShardParseTree *)aStatement->myPlan->parseTree;

    sUserName = QCG_GET_SESSION_USER_NAME( aStatement );
    QC_STR_COPY( sTargetNodeName, sParseTree->mNodeName );

    /***********************************************************************
     * check internal property validate
     ***********************************************************************/
    IDE_TEST( validateCommonForExecute( aStatement ) != IDE_SUCCESS );

    /* for smn propagation to all nodes at commit */
    sdi::setShardMetaTouched( aStatement->session );

    IDE_TEST( sdiZookeeper::getZookeeperMetaLock(QCG_GET_SESSION_ID(aStatement)) != IDE_SUCCESS );
    IDE_TEST( sdiZookeeper::getShardMetaLock( QC_SMI_STMT(aStatement)->getTrans()->getTransID() ) != IDE_SUCCESS );

    /* Meta Lock 획득 후 Failover Check */
    /* Meta Lock 획득 하는 동안 다른 Node가 Failover를 수행하였는지 확인 한다. */
    sDataLength = SDI_ZKC_BUFFER_SIZE;
    IDE_TEST( sdiZookeeper::getNodeInfo( sTargetNodeName,
                                         (SChar*)SDI_ZKC_PATH_NODE_FAILOVER_TO,
                                         sBuffer,
                                         &sDataLength ) != IDE_SUCCESS );

    if ( sDataLength > 0 )
    {
        /* 이미 failover가 되어있는 상태이므로 따로 작업할 필요가 없다. */
        ideLog::log(IDE_SD_21,"[SHARD_FAILOVER] Already Failed over To %s\n", sBuffer );

        IDE_CONT( normal_exit );
    }

    /* Failover를 수행할 Node의 이름을 가져 온다. */
    IDE_TEST( sdiZookeeper::getNextAliveNode( sTargetNodeName,
                                              sFailoverNodeName ) != IDE_SUCCESS );

    /* Failover를 수행할 노드의 상태를 확인 */
    IDE_TEST( sdiZookeeper::validateState( ZK_FAILOVER,
                                           sdiZookeeper::mMyNodeName ) != IDE_SUCCESS );

    IDE_TEST( sdiZookeeper::validateState( ZK_FAILOVER,
                                           sFailoverNodeName ) != IDE_SUCCESS );


    /* Failover 수행 */
    /***********************************************************************
     * Zookeeper meta 1차 변경
     ***********************************************************************/
    IDE_TEST( executeZookeeperFailover( sTargetNodeName,
                                        sFailoverNodeName,
                                        &sIsTargetAlive ) != IDE_SUCCESS );

    if ( sIsTargetAlive == ID_TRUE )
    {
        /* 대상 노드가 살아 있으면 KILL 하고 다른 Node로 Failover를 수행한다. */
        // Set Zookeeper TargetNode State = KILL
        // Zookeeper TargetNode Connection Info remove
        IDE_TEST( sdiZookeeper::killNode( sTargetNodeName ) != IDE_SUCCESS );
    }

    /* get SMN */
    sOldSMN = sdi::getSMNForDataNode();

    /* get Alive Node List */
    IDE_TEST( sdiZookeeper::getAliveNodeNameList( &sList ) != IDE_SUCCESS );
    sIsAllocList = ID_TRUE;

    /* AliveNode들만 대상으로 ShardLinker 생성해야 한다. Node Stop 후에 호출되어야 함. */
    IDE_TEST( sdi::checkShardLinkerWithNodeList ( aStatement,
                                                  sOldSMN,
                                                  NULL,
                                                  sList ) != IDE_SUCCESS );

    sClientInfo = aStatement->session->mQPSpecific.mClientInfo;

    IDE_TEST_RAISE( sClientInfo == NULL, ERR_NODE_NOT_EXIST );

    /* get ReplicaSet Info */
    IDE_TEST( sdm::getAllReplicaSetInfoSortedByPName( QC_SMI_STMT( aStatement ),
                                                      &sReplicaSetInfo,
                                                      sOldSMN )
              != IDE_SUCCESS );

    /* get OldNode Info */
    IDE_TEST( sdi::getNodeByName( QC_SMI_STMT( aStatement ),
                                  sTargetNodeName,
                                  sOldSMN,
                                  &sOldNodeInfo )
              != IDE_SUCCESS );

    /* get NewNode Info */
    IDE_TEST( sdi::getNodeByName( QC_SMI_STMT( aStatement ),
                                  sFailoverNodeName,
                                  sOldSMN,
                                  &sNewNodeInfo )
              != IDE_SUCCESS );

    /* get All TableInfo */
    IDE_TEST( sdi::getTableInfoAllObject( aStatement,
                                          &sTableInfoList,
                                          sOldSMN )
              != IDE_SUCCESS );

    /* FailoverNode Receiver Failover */
    /*
    for ( i = 0; i < sReplicaSetInfo.mCount; i++ )
    {
        if ( idlOS::strncmp( sReplicaSetInfo.mReplicaSets[i].mFirstBackupNodeName,
                             sFailoverNodeName,
                             SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
        {
            idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                             "ALTER REPLICATION %s FAILOVER ", 
                             sReplicaSetInfo.mReplicaSets[i].mFirstReplName );

            IDE_TEST ( executeRemoteSQL( aStatement,
                                         sFailoverNodeName,
                                         SDI_INTERNAL_OP_FAILOVER,
                                         QCI_STMT_MASK_DCL,
                                         sSqlStr,
                                         QD_MAX_SQL_LENGTH,
                                         NULL ) 
                       != IDE_SUCCESS );
        }

        if ( idlOS::strncmp( sReplicaSetInfo.mReplicaSets[i].mSecondBackupNodeName,
                             sFailoverNodeName,
                             SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
        {
            idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                             "ALTER REPLICATION %s FAILOVER ", 
                             sReplicaSetInfo.mReplicaSets[i].mSecondReplName );

            IDE_TEST ( executeRemoteSQL( aStatement,
                                         sFailoverNodeName,
                                         SDI_INTERNAL_OP_FAILOVER,
                                         QCI_STMT_MASK_DCL,
                                         sSqlStr,
                                         QD_MAX_SQL_LENGTH,
                                         NULL ) 
                       != IDE_SUCCESS );
        }
    }
    ideLog::log(IDE_SD_21, "[SHARD_FAILOVER] Replication failover success" );
    */

    /* Find ReplicaSets */
    for ( i = 0 ; i < sReplicaSetInfo.mCount; i++ )
    {
        /* TargetNode가 가진 Failover 가능한 ReplicaSet을 찾는다.
         * 2개 이상 가지고 있을수 있으므로 Prev를 먼저 찾는다. */
        if ( idlOS::strncmp( sReplicaSetInfo.mReplicaSets[i].mPrimaryNodeName,
                             sTargetNodeName,
                             SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
        {
            if ( idlOS::strncmp( sReplicaSetInfo.mReplicaSets[i].mFirstBackupNodeName,
                                 sFailoverNodeName,
                                 SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
            {
                sOldNodeReplicaSet = &sReplicaSetInfo.mReplicaSets[i];
            }
        }
        /* First/Second Backup == TargetNode 인경우는 1개 뿐이다. */
        if ( idlOS::strncmp( sReplicaSetInfo.mReplicaSets[i].mFirstBackupNodeName,
                             sTargetNodeName,
                             SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
        {
            sFirstToOldReplicaSet = &sReplicaSetInfo.mReplicaSets[i];
        }
        if ( idlOS::strncmp( sReplicaSetInfo.mReplicaSets[i].mSecondBackupNodeName,
                             sTargetNodeName,
                             SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
        {
            sSecondToOldReplicaSet = &sReplicaSetInfo.mReplicaSets[i];
        }
    }

    if ( sOldNodeReplicaSet == NULL )
    {
        /* Prev ReplicaSet을 못찾았으면 PrevPrev를 찾는다. 
         * 못찾으면 Failover 가능한 ReplicaSet이 없는 것. */
        for ( i = 0 ; i < sReplicaSetInfo.mCount; i++ )
        {
            if ( idlOS::strncmp( sReplicaSetInfo.mReplicaSets[i].mPrimaryNodeName,
                                 sTargetNodeName,
                                 SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
            {
                if ( idlOS::strncmp( sReplicaSetInfo.mReplicaSets[i].mSecondBackupNodeName,
                                     sFailoverNodeName,
                                     SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
                {
                    sOldNodeReplicaSet = &sReplicaSetInfo.mReplicaSets[i];
                }
            }
        }
    }

    IDE_TEST( getAllNodeInfoList( aStatement ) != IDE_SUCCESS );

    ideLog::log(IDE_SD_21,"[SHARD_FAILOVER] Backup ReplicaSet Start");

    /***********************************************************************
     * FAILOVER 수행 및 전파
     ***********************************************************************/
    /* ReplicaSet backup */
    // Node 단위
    // 전 노드 전파
    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "exec dbms_shard.backup_ReplicaSets( '"QCM_SQL_STRING_SKIP_FMT"', '"
                                                           QCM_SQL_STRING_SKIP_FMT"', '"
                                                           QCM_SQL_STRING_SKIP_FMT"' )",
                     sUserName,
                     sTargetNodeName,
                     sFailoverNodeName );

    IDE_TEST( executeSQLNodeList( aStatement,
                                  SDI_INTERNAL_OP_FAILOVER,
                                  QCI_STMT_MASK_SP,
                                  sSqlStr,
                                  sTargetNodeName,
                                  sList ) != IDE_SUCCESS );

    ideLog::log(IDE_SD_21,"[SHARD_FAILOVER] Backup ReplicaSet");


    /* Log 보존을 위한 역이중화 생성 및 Start */
    /* 역 이중화는 FailoverNode to TargetNode로 이루어지기 때문에
     * Sender만 생성가능하다. Receiver는 Failback에서 생성 해야함. */
    for ( i = 0; i < sReplicaSetInfo.mCount; i++ )
    {
        if ( idlOS::strncmp( sReplicaSetInfo.mReplicaSets[i].mPrimaryNodeName,
                             sTargetNodeName,
                             SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
        {
            if ( idlOS::strncmp( sReplicaSetInfo.mReplicaSets[i].mFirstBackupNodeName,
                                 sFailoverNodeName,
                                 SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
            {
                idlOS::snprintf( sReverseReplName, 
                                 SDI_REPLICATION_NAME_MAX_SIZE + 1,
                                 "R_%s", 
                                 sReplicaSetInfo.mReplicaSets[i].mFirstReplName );
            }
            else if ( idlOS::strncmp( sReplicaSetInfo.mReplicaSets[i].mSecondBackupNodeName,
                                 sFailoverNodeName,
                                 SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
            {
                idlOS::snprintf( sReverseReplName, 
                                 SDI_REPLICATION_NAME_MAX_SIZE + 1,
                                 "R_%s", 
                                 sReplicaSetInfo.mReplicaSets[i].mSecondReplName );
            }
            else
            {
                idlOS::snprintf( sReverseReplName,
                                 SDI_REPLICATION_NAME_MAX_SIZE + 1,
                                 SDM_NA_STR );
            }

            if ( idlOS::strncmp( sReverseReplName,
                                 SDM_NA_STR,
                                 SDI_REPLICATION_NAME_MAX_SIZE + 1 ) != 0 )
            {
                sNodeMetaInfo = sdiZookeeper::getNodeInfo( sParseTree->mNodeInfoList,
                                                           sTargetNodeName );

                IDE_TEST( createReverseReplication( aStatement,
                                                    sFailoverNodeName,
                                                    sReverseReplName,
                                                    sNodeMetaInfo->mNodeName,
                                                    sNodeMetaInfo->mInternalRPHostIP,
                                                    sNodeMetaInfo->mInternalRPPortNo,
                                                    SDI_REPL_SENDER )
                          != IDE_SUCCESS );

                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                 "EXEC DBMS_SHARD.EXECUTE_IMMEDIATE( 'ALTER REPLICATION "QCM_SQL_STRING_SKIP_FMT" START RETRY', '"QCM_SQL_STRING_SKIP_FMT"' ) ", 
                                 sReverseReplName,
                                 sFailoverNodeName );

                /* DCL 구문이라 NewTrans로 호출 */
                IDE_TEST( sdi::shardExecTempDMLOrDCLWithNewTrans( aStatement, sSqlStr ) != IDE_SUCCESS );

                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                 "EXEC DBMS_SHARD.EXECUTE_IMMEDIATE( 'ALTER REPLICATION "QCM_SQL_STRING_SKIP_FMT" STOP', '"QCM_SQL_STRING_SKIP_FMT"' ) ", 
                                 sReverseReplName,
                                 sFailoverNodeName );

                /* DCL 구문이라 NewTrans로 호출 */
                IDE_TEST( sdi::shardExecTempDMLOrDCLWithNewTrans( aStatement, sSqlStr ) != IDE_SUCCESS );

                ideLog::log(IDE_SD_21,"[SHARD_FAILOVER] Reverser Replication Created : %s", sReverseReplName );
            }
        }
    }

    IDE_TEST( failoverAddReverseReplicationItems( aStatement,
                                                  sTargetNodeName,
                                                  sFailoverNodeName,
                                                  sTableInfoList,
                                                  &sReplicaSetInfo,
                                                  &sOldNodeInfo,
                                                  sOldSMN ) != IDE_SUCCESS );

    ideLog::log(IDE_SD_21,"[SHARD_FAILOVER] Shard Meta Change Start");


    /* Shard Meta Update */
    // Table Partition 단위
    IDE_TEST( failoverResetShardMeta( aStatement,
                                      sTargetNodeName,
                                      sFailoverNodeName,
                                      sTableInfoList,
                                      &sReplicaSetInfo,
                                      &sOldNodeInfo,
                                      sList,
                                      sOldSMN,
                                      &sIsUnsetNode ) != IDE_SUCCESS );

    ideLog::log(IDE_SD_21,"[SHARD_FAILOVER] RangeInfo Update Complete");

    /* FailoverNode의 Next Node 즉 Failover의 FirstBackupNode를 가져온다. */
    IDE_TEST( sdiZookeeper::getNextNode( sFailoverNodeName, 
                                         sFailoverNextNodeName, 
                                         &sState ) != IDE_SUCCESS );
    /* create RP NewNode to NextNode */
    /* N1 을 N2가 Failover 했다면 N1->N3의 RP를 N2->N3 형태로 유지 하여야 한다.
     * N2 입장에서는 없던 새로운 이중화가 생기는 것 */
    if ( sOldNodeReplicaSet != NULL )
    {
        /* First는 FailoverNode 이거나 PrevNode일 수 밖에 없어서 Check 안함. */
        if ( idlOS::strncmp( sOldNodeReplicaSet->mSecondBackupNodeName,
                             sFailoverNextNodeName,
                             SDI_NODE_NAME_MAX_SIZE + 1) == 0 )
        {
            /* matched */
            idlOS::snprintf( sFailoverCreatedReplName, 
                             SDI_REPLICATION_NAME_MAX_SIZE + 1,
                             "%s", 
                             sOldNodeReplicaSet->mSecondReplName );

            /* FailoverCreatedRP는 Failover 시에 생성되어 Failback 시에 제거된다.
             * Failover 시점에 이미 같은게 생성되어 있으면 이전에 정리되지 않은 찌꺼기이다.
             * Drop 하고 Create 해야 한다. */
            idlOS::snprintf( sSqlStr, 
                             QD_MAX_SQL_LENGTH + 1,
                             "select Count(*) from system_.sys_replications_ where replication_name='%s' ",
                             sFailoverCreatedReplName );

            IDE_TEST( sdi::shardExecDirect( aStatement,
                                            sFailoverNodeName,
                                            (SChar*)sSqlStr,
                                            (UInt) idlOS::strlen(sSqlStr),
                                            SDI_INTERNAL_OP_FAILOVER,
                                            &sExecCount,
                                            &sCount,
                                            NULL,
                                            ID_SIZEOF(sCount),
                                            &sFetchResult )
                      != IDE_SUCCESS );

            if ( sCount != 0 )
            {
                /* RP 생성 시점에 존재하면 안된다. Drop 하고 새로 생성해야 한다. */
                IDE_TEST( dropReplicationForInternal( aStatement,
                                                      sFailoverNodeName,
                                                      sFailoverCreatedReplName )
                          != IDE_SUCCESS );
            }
 
            /* 이중화 생성과 Start는 모두 NewTrans로 이루어 진다. */
            /* Sender를 생성한다. */
            sFailoverNextLocalMetaInfo = sdiZookeeper::getNodeInfo( sParseTree->mNodeInfoList,
                                                                    sOldNodeReplicaSet->mSecondReplToNodeName );

            IDE_TEST( createReplicationForInternal( aStatement,
                                                    sFailoverNodeName,
                                                    sFailoverCreatedReplName,
                                                    sFailoverNextLocalMetaInfo->mInternalRPHostIP,
                                                    sFailoverNextLocalMetaInfo->mInternalRPPortNo )
                      != IDE_SUCCESS );

            /* Create 된 RP는 Rollback시 지워주어야 한다. */
            idlOS::snprintf(sSqlStr,
                            QD_MAX_SQL_LENGTH,
                            "alter replication "QCM_SQL_STRING_SKIP_FMT" stop",
                            sFailoverCreatedReplName );

            (void)sdiZookeeper::addPendingJob( sSqlStr,
                                               sFailoverNodeName,
                                               ZK_PENDING_JOB_AFTER_ROLLBACK,
                                               QCI_STMT_MASK_MAX );

            idlOS::snprintf(sSqlStr,
                            QD_MAX_SQL_LENGTH,
                            "drop replication "QCM_SQL_STRING_SKIP_FMT"",
                            sFailoverCreatedReplName );

            (void)sdiZookeeper::addPendingJob( sSqlStr,
                                               sFailoverNodeName,
                                               ZK_PENDING_JOB_AFTER_ROLLBACK,
                                               QCI_STMT_MASK_MAX );

            ideLog::log(IDE_SD_21,"[SHARD_FAILOVER] FailoverCreatedReplication Sender Created");


            /* Replication Start */
            idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                             "ALTER REPLICATION "QCM_SQL_STRING_SKIP_FMT" START ", 
                             sOldNodeReplicaSet->mSecondReplName );

            if ( executeRemoteSQL( aStatement,
                                   sFailoverNodeName,
                                   SDI_INTERNAL_OP_FAILOVER,
                                   QCI_STMT_MASK_DCL,
                                   sSqlStr,
                                   QD_MAX_SQL_LENGTH,
                                   NULL ) == IDE_SUCCESS )
            {
                ideLog::log(IDE_SD_21,"[SHARD_FAILOVER] Replication Started");

                /* Flush 호출 */
                IDE_TEST( flushReplicationForInternal( aStatement,
                                                       sFailoverNodeName,
                                                       sOldNodeReplicaSet->mSecondReplName )
                          != IDE_SUCCESS );
            }
            else
            {
                /* FailoverNextNode가 죽어있고 아직 Failover 되지 않았다면 Start에 실패할수 있다.
                 * Create RP/Add Item은 수행하고 Start는 실패하였으므로 
                 * Retry로 재시도 해둔다. 추후 FailoverNextNode의 Failover 수행에서 Stop을 요청할 것이다. */
                ideLog::log(IDE_SD_21,"[SHARD_FAILOVER] Replication Failed to Start");

                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                 "ALTER REPLICATION "QCM_SQL_STRING_SKIP_FMT" START RETRY ", 
                                 sOldNodeReplicaSet->mSecondReplName );

                IDE_TEST ( executeRemoteSQL( aStatement,
                                             sFailoverNodeName,
                                             SDI_INTERNAL_OP_FAILOVER,
                                             QCI_STMT_MASK_DCL,
                                             sSqlStr,
                                             QD_MAX_SQL_LENGTH,
                                             NULL ) 
                           != IDE_SUCCESS );

                ideLog::log(IDE_SD_21,"[SHARD_FAILOVER] Replication Start Retry");
            }
        }
    }

    /* Replication Add */
    IDE_TEST( failoverAddReplicationItems( aStatement,
                                           sTargetNodeName,
                                           sFailoverNodeName,
                                           sFailoverNextNodeName,
                                           sTableInfoList,
                                           &sReplicaSetInfo,
                                           &sOldNodeInfo,
                                           sOldSMN ) != IDE_SUCCESS );

    ideLog::log(IDE_SD_21,"[SHARD_FAILOVER] Replication Item Added");

    /* Data Failover 가 수행되었다면 해당 Node를 Unset 한다.
     * Data가 없으면 없는 걸 알수 있어야 한다. */
    if ( ( sOldNodeReplicaSet != NULL ) && ( sIsUnsetNode == ID_TRUE ) )
    {
        /* Nodes_ 에서 제거 */
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "exec dbms_shard.unset_node( '"QCM_SQL_STRING_SKIP_FMT"' )",
                         sTargetNodeName );

        IDE_TEST( executeSQLNodeList( aStatement,
                                      SDI_INTERNAL_OP_FAILOVER,
                                      QCI_STMT_MASK_SP,
                                      sSqlStr,
                                      sTargetNodeName,
                                      sList ) != IDE_SUCCESS );

        ideLog::log(IDE_SD_21,"[SHARD_FAILOVER] Node Meta Update Complete");
    }

    /* ReplicaSet Update & To TargetNode RP Stop */
    /* ReplicaSet Update는 TargetNode->FailoverNode 만 아니라 to TargetNode도 변경해야 하기 때문에
     * checkFailoverAvailable 로는 판단할수 없고 내부에서 판단한다. */
    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "exec dbms_shard.reset_ReplicaSets( '"QCM_SQL_STRING_SKIP_FMT"', '"
                                                          QCM_SQL_STRING_SKIP_FMT"', '"
                                                          QCM_SQL_STRING_SKIP_FMT"' )",
                     sUserName,
                     sTargetNodeName,
                     sFailoverNodeName );

    IDE_TEST( executeSQLNodeList( aStatement,
                                  SDI_INTERNAL_OP_FAILOVER,
                                  QCI_STMT_MASK_SP,
                                  sSqlStr,
                                  sTargetNodeName,
                                  sList ) != IDE_SUCCESS );

    ideLog::log(IDE_SD_21,"[SHARD_FAILOVER] ReplicaSet Update Complete");

    /* Stop할 Repl이 존재한다면 After Commit에 Stop하도록 한다. */
    if ( sFirstToOldReplicaSet != NULL )
    {
        idlOS::strncpy( sFirstStopNodeName,
                        sFirstToOldReplicaSet->mPrimaryNodeName,
                        SDI_NODE_NAME_MAX_SIZE + 1 );
        idlOS::strncpy( sFirstStopReplName,
                        sFirstToOldReplicaSet->mFirstReplName,
                        SDI_REPLICATION_NAME_MAX_SIZE + 1 );
    }
    else
    {
        idlOS::strncpy( sFirstStopNodeName,
                        SDM_NA_STR,
                        SDI_NODE_NAME_MAX_SIZE + 1 );
        idlOS::strncpy( sFirstStopReplName,
                        SDM_NA_STR,
                        SDI_REPLICATION_NAME_MAX_SIZE + 1 );
    }
    if ( sSecondToOldReplicaSet != NULL )
    {
        idlOS::strncpy( sSecondStopNodeName,
                        sSecondToOldReplicaSet->mPrimaryNodeName,
                        SDI_NODE_NAME_MAX_SIZE + 1 );

        idlOS::strncpy( sSecondStopReplName,
                        sSecondToOldReplicaSet->mSecondReplName,
                        SDI_REPLICATION_NAME_MAX_SIZE + 1 );
    }
    else
    {
        idlOS::strncpy( sSecondStopNodeName,
                        SDM_NA_STR,
                        SDI_NODE_NAME_MAX_SIZE + 1 );

        idlOS::strncpy( sSecondStopReplName,
                        SDM_NA_STR,
                        SDI_REPLICATION_NAME_MAX_SIZE + 1 );
    }

    sdiZookeeper::setReplInfo4FailoverAfterCommit( sFirstStopNodeName,
                                                   sSecondStopNodeName,
                                                   sFirstStopReplName,
                                                   sSecondStopReplName );

    /* Partition Swap */
    // Table Partition 단위 Swap
    // 작업이 필요한 Node에만 전파
    /* Swap이 수행되는 Node의 Table에 Lock 획득 및 Swap */

    /* R2HA Flush XLog Files */
    IDE_TEST( lockTables4Failover( aStatement,
                                   sSqlStr,
                                   QD_MAX_SQL_LENGTH,
                                   sFailoverNodeName,
                                   sTableInfoList,
                                   &sReplicaSetInfo,
                                   &sOldNodeInfo,
                                   sOldSMN ) != IDE_SUCCESS );
    
    ideLog::log(IDE_SD_21,"[SHARD_FAILOVER] TargetTable Lock Acquired");

    IDE_TEST( failoverTablePartitionSwap( aStatement,
                                          sFailoverNodeName,
                                          sTableInfoList,
                                          &sReplicaSetInfo,
                                          &sOldNodeInfo,
                                          sOldSMN ) != IDE_SUCCESS );

    ideLog::log(IDE_SD_21,"[SHARD_FAILOVER] Partition Swap Complete");

    /* BUG-48586 */ 
    IDE_TEST( executeRemoteSQL( aStatement,
                                sFailoverNodeName,
                                SDI_INTERNAL_OP_NORMAL,
                                QCI_STMT_MASK_SP,
                                sSqlStr,
                                QD_MAX_SQL_LENGTH,
                                "exec dbms_shard.set_ReplicaSCN()") != IDE_SUCCESS );

    sIsAllocList = ID_FALSE;
    sdiZookeeper::freeList( sList, SDI_ZKS_LIST_NODENAME );    

    ideLog::log(IDE_SD_21,"[SHARD_FAILOVER] Zookeeper Meta Update Complete");

//    sdi::finalizeSession( aStatement->session );

    IDE_EXCEPTION_CONT( normal_exit );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NODE_NOT_EXIST )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_NODE_NOT_EXIST ) );
    }
    IDE_EXCEPTION_END;

    if( sIsAllocList == ID_TRUE )
    {
        sdiZookeeper::freeList( sList, SDI_ZKS_LIST_NODENAME );    
    }

    ideLog::log(IDE_SD_5,"[SHARD_FAILOVER_ERROR] Fail");

//    sdi::finalizeSession( aStatement->session );

    return IDE_FAILURE;
}

IDE_RC qdsd::validateShardFailback( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *    ALTER DATABASE SHARD FAILBACK
 *
 * Implementation :
 *    1.
 ***********************************************************************/

    idBool              sIsConnect  = ID_FALSE;
    SChar               sRecentDeadNode[SDI_NODE_NAME_MAX_SIZE + 1] = {0,};
    sdiLocalMetaInfo    sLocalMetaInfo;
    idBool              sIsAcquireZookeeperMetaLock = ID_FALSE;
    idBool              sIsAcquireShardMetaLock = ID_FALSE;

    qciUserInfo         sUserInfo;
    void *              sMmSession = aStatement->session->mMmSession;

    SInt                sDataLength;
    SChar               sBuffer[SDI_ZKC_BUFFER_SIZE] = {0,};

    qci::mSessionCallback.mGetUserInfo( sMmSession, &sUserInfo );

    /***********************************************************************
     * validateCommon
     ***********************************************************************/

    IDE_TEST( validateCommon( aStatement ) != IDE_SUCCESS );

    /* BUG-48770 다른 session이 있거나 shard cli로 접근한 경우에는 shard DDL을 수행할 수 없다.*/
    if( QCU_DISPLAY_PLAN_FOR_NATC == 0 )
    {
        IDE_TEST_RAISE( qci::mSessionCallback.mCheckSessionCount() > 1, ERR_TOO_MANY_SESSION );
    }

    IDE_TEST_RAISE( qci::mSessionCallback.mIsShardClient( aStatement->session->mMmSession )
                    == ID_TRUE,
                    ERR_SHARD_CLI );

    /***********************************************************************
     * validate SHARD FAILBACK
     ***********************************************************************/

    /***********************************************************************
     * Zookeeper validation
     ***********************************************************************/

    IDE_TEST_RAISE( sdiZookeeper::isConnect() == ID_TRUE, ERR_ALREADY_CONNECT );
    /* zookeeper server와 연결한다. */
    IDE_TEST( sdi::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );
    IDE_TEST_RAISE( sLocalMetaInfo.mNodeName[0] == '\0', no_meta_exist );

    sdiZookeeper::initialize( sLocalMetaInfo.mNodeName,
                              &sUserInfo );
    IDE_TEST_RAISE( sdiZookeeper::connect( ID_FALSE ) != IDE_SUCCESS, connect_Fail );
    sIsConnect = ID_TRUE;

    /* 샤드 메타를 변경하므로 zookeeper의 샤드 메타 lock을 잡는다. */
    IDE_TEST( sdiZookeeper::getZookeeperMetaLock(QCG_GET_SESSION_ID(aStatement)) != IDE_SUCCESS );
    sIsAcquireZookeeperMetaLock = ID_TRUE; 

    IDE_TEST( sdiZookeeper::getShardMetaLock( QC_SMI_STMT(aStatement)->getTrans()->getTransID() ) != IDE_SUCCESS );
    sIsAcquireShardMetaLock = ID_TRUE;

    /* Check Fault_detect_time */
    sDataLength = SDI_ZKC_BUFFER_SIZE;
    IDE_TEST( sdiZookeeper::readInfo( (SChar *)SDI_ZKC_PATH_FAULT_DETECT_TIME,
                                      sBuffer,
                                      &sDataLength ) != IDE_SUCCESS );

    // getProperty SHARD_FAILBACK_GIVEUP_INTERVAL
    

    /* 현재 노드의 상태가 FAILBACK이 가능한 상태인지 체크한다. */
    IDE_TEST( sdiZookeeper::validateState( ZK_FAILBACK,
                                           sdiZookeeper::mMyNodeName ) != IDE_SUCCESS );

    /* Check Failover To */
    sDataLength = SDI_ZKC_BUFFER_SIZE;
    IDE_TEST( sdiZookeeper::getNodeInfo( sdiZookeeper::mMyNodeName,
                                         (SChar*)SDI_ZKC_PATH_NODE_FAILOVER_TO,
                                         sBuffer,
                                         &sDataLength ) != IDE_SUCCESS );

    if ( sDataLength > 0 )
    {
        /* 내 노드가 Failover 되어 있으면 */
        /* 내 노드가 가장 최근에 죽은 노드인지 확인한다. */
        IDE_TEST( sdiZookeeper::checkRecentDeadNode( sRecentDeadNode, NULL, NULL ) != IDE_SUCCESS );

        IDE_TEST_RAISE( idlOS::strncmp( sdiZookeeper::mMyNodeName,
                                        sRecentDeadNode,
                                        idlOS::strlen( sRecentDeadNode ) ) != 0, not_my_turn );
    }
    else
    {
        /* 내 노드가 Failover 수행되지 않았으면 그냥 Failback 대상이다. */
    }

    if ( sIsAcquireShardMetaLock == ID_TRUE )
    {
        sdiZookeeper::releaseShardMetaLock();
        sIsAcquireShardMetaLock = ID_FALSE;
    }
    
    if ( sIsAcquireZookeeperMetaLock == ID_TRUE )
    {
        sdiZookeeper::releaseZookeeperMetaLock();
        sIsAcquireZookeeperMetaLock = ID_FALSE;
    }

    sdiZookeeper::disconnect();
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TOO_MANY_SESSION )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_TOO_MANY_SESSION, "failback" ) );
    }
    IDE_EXCEPTION( ERR_SHARD_CLI )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_SHARD_DDL_TRY_WITH_SHARD_CLI ) );
    }
    IDE_EXCEPTION( connect_Fail )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_ZKC_CONNECTION_FAIL ) );
    }
    IDE_EXCEPTION( no_meta_exist )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_SHARD_META_NOT_CREATED ) );
    }
    IDE_EXCEPTION( not_my_turn )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_ZKC_NOT_MY_TURN ) ); 
    }
    IDE_EXCEPTION( ERR_ALREADY_CONNECT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_ZKC_ALREADY_CONNECTED ) );
    }
    IDE_EXCEPTION_END;
 
    if ( sIsAcquireShardMetaLock == ID_TRUE )
    {
        sdiZookeeper::releaseShardMetaLock();
        sIsAcquireShardMetaLock = ID_FALSE;
    }
   
    if( sIsAcquireZookeeperMetaLock == ID_TRUE )
    {
        sdiZookeeper::releaseZookeeperMetaLock();
        sIsAcquireZookeeperMetaLock = ID_FALSE;
    }

    if( sIsConnect == ID_TRUE )
    {
        sdiZookeeper::disconnect();
    }

    return IDE_FAILURE;
}

IDE_RC qdsd::executeShardFailback( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *    ALTER DATABASE SHARD FAILBACK
 *
 * Implementation :
 *    1. 
 ***********************************************************************/

    SChar               sRecentDeadNode[SDI_NODE_NAME_MAX_SIZE + 1] = {0,};
    SChar               sFailbackFromNodeName[SDI_NODE_NAME_MAX_SIZE + 1];

    ULong               sFailbackSMN = 0;
    ULong               sFailoveredSMN = 0;
    ULong               sOldSMN = 0;
    ULong               sZKSMN = 0;

    SInt                sDataLength;
    SChar               sBuffer[SDI_ZKC_BUFFER_SIZE] = {0,};

    idBool              sExistNotFailedoverDeadNode = ID_FALSE;

    sdiFailbackState    sFailbackState = SDI_FAILBACK_NONE;

    /***********************************************************************
     * check internal property validate
     ***********************************************************************/
    IDE_TEST( validateCommonForExecute( aStatement ) != IDE_SUCCESS );

    IDE_TEST_RAISE( sdiZookeeper::isConnect() == ID_TRUE, ERR_ALREADY_CONNECT );

    /* zookeeper server와 연결한다. */
    IDE_TEST( sdiZookeeper::connect( ID_FALSE ) != IDE_SUCCESS );
    
    /* Meta Lock 획득 */
    sdi::setShardMetaTouched( aStatement->session );

    IDE_TEST( checkShardPIN( aStatement ) != IDE_SUCCESS );

    sdi::setInternalTableSwap( aStatement->session );

    IDE_TEST( sdiZookeeper::getZookeeperMetaLock(QCG_GET_SESSION_ID(aStatement)) != IDE_SUCCESS );
    IDE_TEST( sdiZookeeper::getShardMetaLock( QC_SMI_STMT(aStatement)->getTrans()->getTransID() ) != IDE_SUCCESS );

    ideLog::log(IDE_SD_0,"[SHARD_FAILBACK] Meta Lock Acquired");

    /* Meta Lock 획득 후 Validation */
    /* 현재 노드의 상태가 FAILBACK이 가능한 상태인지 체크한다. */
    IDE_TEST( sdiZookeeper::validateState( ZK_FAILBACK,
                                           sdiZookeeper::mMyNodeName ) != IDE_SUCCESS );

    /***********************************************************************
     * Zookeeper meta 1차 변경
     ***********************************************************************/
    /* zookeeper 메타를 변경한다. */
    IDE_TEST( executeZookeeperFailback() != IDE_SUCCESS );

    /* Check Failover To */
    sDataLength = SDI_ZKC_BUFFER_SIZE;
    IDE_TEST( sdiZookeeper::getNodeInfo( sdiZookeeper::mMyNodeName,
                                         (SChar*)SDI_ZKC_PATH_NODE_FAILOVER_TO,
                                         sBuffer,
                                         &sDataLength ) != IDE_SUCCESS );

    if ( sDataLength > 0 )
    {
        /* 내 노드가 Failover 되어 있으면 */
        /* 내 노드가 가장 최근에 죽은 노드인지 확인한다. */
        IDE_TEST( sdiZookeeper::checkRecentDeadNode( sRecentDeadNode, &sFailbackSMN, &sFailoveredSMN ) != IDE_SUCCESS );

        IDE_TEST_RAISE( idlOS::strncmp( sdiZookeeper::mMyNodeName,
                                        sRecentDeadNode,
                                        idlOS::strlen( sRecentDeadNode ) ) != 0, not_my_turn );

        /* 내 노드가 가장 최근 FAILOVER 노드이면 FAILBACK_FAILEDOVER 이다. */

        /* FailoverHistory에 있는 Node 외에 DeadNode가 존재한다면 Failback 수행하지 않아야 한다. */
        IDE_TEST( sdiZookeeper::checkExistNotFailedoverDeadNode( &sExistNotFailedoverDeadNode ) != IDE_SUCCESS );

        IDE_TEST_RAISE( sExistNotFailedoverDeadNode != ID_FALSE, EXIST_NOT_FAILEDOVER_DEADNODE );

        /* Failover를 해간 Node로 부터 Meta를 복원할 것이다. */
        idlOS::strncpy( sFailbackFromNodeName,
                        sBuffer,
                        SDI_NODE_NAME_MAX_SIZE + 1 );

        sFailbackState = SDI_FAILBACK_FAILEDOVER;

        ideLog::log(IDE_SD_0,"[SHARD_FAILBACK] FAILBACK_FAILEDOVER Start");
    }
    else
    {
        /* 내 노드가 Failover 수행되지 않았으면 Failback 대상이다. */

        /* check Zookeeper SMN */
        /* SMN을 확인하여 Zookeeper와 동일하면 FAILBACK_JOIN 이다. */
        //IDE_TEST( checkZookeeperSMNAndDataSMN( aStatement ) != IDE_SUCCESS );
        IDE_TEST( sdiZookeeper::getZookeeperSMN( &sZKSMN ) != IDE_SUCCESS );
        sOldSMN = sdi::getSMNForDataNode();

        if ( sOldSMN == sZKSMN )
        {
            /* Failover 되지 않았는데 Zookeeper와 Node의 SMN이 일치하면 FAILBACK_JOIN이다.*/
            sFailbackState = SDI_FAILBACK_JOIN;
            ideLog::log(IDE_SD_0,"[SHARD_FAILBACK] FAILBACK_JOIN Start");
        }
        else
        {
            /* Failover 되지 않았는데, SMN도 일치하지 않으면 FAILBACK_JOIN_SMN_MODIFIED 이다 */
            sFailbackState = SDI_FAILBACK_JOIN_SMN_MODIFIED;
            ideLog::log(IDE_SD_0,"[SHARD_FAILBACK] FAILBACK_JOIN_SMN_MODIFIED Start");
        }

        /* NextAliveNode의 이름을 가져온다. */
        /* 아무도 Failover 하지 않았으면 NextAliveNode를 기준으로 Meta를 맞춘다. */
        IDE_TEST( sdiZookeeper::getNextAliveNode( sdiZookeeper::mMyNodeName,
                                                  sFailbackFromNodeName ) != IDE_SUCCESS );
    }

    /***********************************************************************
     * FAILBACK 수행 및 전파
     ***********************************************************************/  

    switch ( sFailbackState )
    {
        case SDI_FAILBACK_JOIN:
        case SDI_FAILBACK_JOIN_SMN_MODIFIED:
            IDE_TEST( failbackJoin( aStatement,
                                    sFailbackFromNodeName,
                                    sFailbackState ) != IDE_SUCCESS );
            break;
        case SDI_FAILBACK_FAILEDOVER:
            IDE_TEST( failbackFailedover( aStatement,
                                          sFailbackFromNodeName,
                                          sFailbackSMN ) != IDE_SUCCESS );
            break;
        case SDI_FAILBACK_NONE:
        default:
            /* 이런건 오면 안됨 */
            IDE_DASSERT( 0 );
            break;
    }

    sdi::loadShardPinInfo();

    return IDE_SUCCESS;

    IDE_EXCEPTION( EXIST_NOT_FAILEDOVER_DEADNODE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_EXIST_NOT_FAILEDOVER_DEADNODE ));
    }
    IDE_EXCEPTION( not_my_turn )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_ZKC_NOT_MY_TURN ) ); 
    }
    IDE_EXCEPTION( ERR_ALREADY_CONNECT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_ZKC_ALREADY_CONNECTED ) );
    }
    IDE_EXCEPTION_END;

    ideLog::log(IDE_SD_0,"[SHARD_FAILBACK] FAILED FAILBACK");

    sdi::unsetInternalTableSwap( aStatement->session );

    /* TASK-7217 Sharded sequence */
    qcm::unsetShardNodeID();

    return IDE_FAILURE;
}

IDE_RC qdsd::getProcedureKeyValue( qdReShardAttribute * aReshardAttr,
                                   SChar              * aProcKeyVal )
{
    qcNamePosition           sValPos;
    qmmValueNode           * sValNode;

    SET_EMPTY_POSITION( sValPos );

    idlOS::memset( aProcKeyVal, 0x00, SDI_RANGE_VARCHAR_MAX_SIZE + 1 );
    IDE_DASSERT(aReshardAttr->mKeyValue != NULL);
    IDE_DASSERT(aReshardAttr->mKeyValue->next == NULL);
    sValNode = aReshardAttr->mKeyValue;
    if ( sValNode->value != NULL )
    {
        sValPos = sValNode->value->position;

        IDE_TEST_RAISE(sValPos.size > (SInt)SDI_RANGE_VARCHAR_MAX_SIZE, ERR_SHARD_KEY_MAX_VALUE_TOO_LONG);
        idlOS::memcpy( aProcKeyVal,
                       sValPos.stmtText + sValPos.offset,
                       sValPos.size );
        aProcKeyVal[sValPos.size] = '\0';
    }
    else
    {
        aProcKeyVal[0] = '\0';
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_SHARD_KEY_MAX_VALUE_TOO_LONG)
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_SHARD_KEY_MAX_VALUE_TOO_LONG ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdsd::validateShardMove( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *    ALTER DATABASE SHARD MOVE [ {TABLE object_name {PARTITION ( partition_name ) }} |
 *                                {PROCEDURE object_name {KEY ( key_value )} } ]+
 *    TO node_name
 *
 ***********************************************************************/
    idBool               sIsValid = ID_FALSE;
    qdShardParseTree   * sParseTree = NULL;
    qdReShardAttribute * sReShardAttr = NULL;
    SChar               sPath[SDI_ZKC_PATH_LENGTH] = SDI_ZKC_PATH_ALTIBASE_SHARD;
    SChar               sToNodeName[SDI_NODE_NAME_MAX_SIZE + 1] = { 0, };
    SChar               sFromNodeName[SDI_NODE_NAME_MAX_SIZE + 1] = { 0, };
    UInt                sUserID;
    SChar               sUserName[QC_MAX_OBJECT_NAME_LEN + 1] = { 0, };
    SChar               sObjectName[QC_MAX_OBJECT_NAME_LEN + 1] = { 0, };
    SChar               sPartitionName[QC_MAX_OBJECT_NAME_LEN + 1] = { 0, };
    idBool              sIsAcquireZookeeperMetaLock = ID_FALSE;
    idBool              sIsAcquireShardMetaLock = ID_FALSE;
    sdiLocalMetaInfo  * sToNodeLocalInfo = NULL;
    SInt                i = 0;
    idBool              sIsMetaInited = ID_FALSE;
    SChar               sProcKeyValue[SDI_RANGE_VARCHAR_MAX_SIZE + 1];
    SChar             * sKeyValue = NULL;
    idBool              sIsCloneExist = ID_FALSE;
    idBool              sIsNotCloneExist = ID_FALSE;
    iduList           * sNodeInfoList;
    UInt                sNodeCount;
    IDE_CLEAR();
    sParseTree = (qdShardParseTree *)aStatement->myPlan->parseTree;

    /***********************************************************************
     * validateCommon
     ***********************************************************************/
    if ( sdi::isShardDDLForAddClone( aStatement ) != ID_TRUE )
    {
        IDE_TEST( validateCommon( aStatement ) != IDE_SUCCESS );

        IDE_TEST_RAISE( sdiZookeeper::isConnect() != ID_TRUE, ERR_NOT_CONNECT );

        /* 클러스터 메타가 생성되어있는지 확인한다. */
        IDE_TEST( sdiZookeeper::checkPath( sPath,
                                           &sIsMetaInited ) != IDE_SUCCESS );
        IDE_TEST_RAISE( sIsMetaInited != ID_TRUE, ERR_META );

        /* property confirmation resharding properties */
        IDE_TEST( sdiZookeeper::getZookeeperMetaLock(QCG_GET_SESSION_ID(aStatement)) != IDE_SUCCESS ); 
        sIsAcquireZookeeperMetaLock = ID_TRUE;

        IDE_TEST( sdiZookeeper::getShardMetaLock( QC_SMI_STMT(aStatement)->getTrans()->getTransID() )
                  != IDE_SUCCESS );
        sIsAcquireShardMetaLock = ID_TRUE;

        IDE_TEST( sdiZookeeper::getAllNodeInfoList( &sNodeInfoList,
                                                    &sNodeCount )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( sNodeInfoList == NULL,
                        ERR_CANNOT_RESHARDING_SHORT_OF_NODE_COUNT );
    }
    
    /***********************************************************************
     * validate SHARD MOVE
     ***********************************************************************/

    switch(sParseTree->mDDLType)
    {
        case SHARD_MOVE_TO:
            IDE_DASSERT( QC_IS_NULL_NAME(sParseTree->mNodeName) != ID_TRUE );
            QC_STR_COPY(sToNodeName, sParseTree->mNodeName);
            IDE_TEST_RAISE( sNodeCount < 2, ERR_CANNOT_RESHARDING_SHORT_OF_NODE_COUNT );
            sToNodeLocalInfo = sdiZookeeper::getNodeInfo(sNodeInfoList, sToNodeName);
            IDE_TEST_RAISE( sToNodeLocalInfo == NULL, ERR_NOT_FOUND_NODE );
            break;
        case SHARD_MOVE_REMOVE:
            IDE_RAISE(ERR_NOT_SUPPORTED_OPTION );
            IDE_DASSERT( QC_IS_NULL_NAME(sParseTree->mNodeName) == ID_TRUE );
            break;
        case SHARD_MOVE_TO_INTERNAL_CLONE_COPY:
        case SHARD_DDL_UNKNOWN:
        case SHARD_ADD:
            if ( sdi::isShardDDLForAddClone( aStatement ) == ID_TRUE )
            {
                // shard add에서 mNodeInfoList, mNodeCount가 set 되어 있다.
                IDE_DASSERT( sParseTree->mNodeInfoList != NULL );
                IDE_DASSERT( QC_IS_NULL_NAME(sParseTree->mNodeName) != ID_TRUE );
                
                QC_STR_COPY(sToNodeName, sParseTree->mNodeName);
                IDE_TEST_RAISE( sParseTree->mNodeCount < 2, ERR_CANNOT_RESHARDING_SHORT_OF_NODE_COUNT );
                sToNodeLocalInfo = sdiZookeeper::getNodeInfo(sParseTree->mNodeInfoList, sToNodeName);
                IDE_TEST_RAISE( sToNodeLocalInfo == NULL, ERR_NOT_FOUND_NODE );
            }
            break;
        case SHARD_JOIN:
        case SHARD_DROP:
        case SHARD_DROP_FORCE:
        case SHARD_FAILOVER_NORMAL:
        case SHARD_FAILOVER_IMMEDIATE:
        case SHARD_FAILOVER_EXIT:
        case SHARD_FAILOVER_FORCE:
        case SHARD_FAILBACK:
        default:
            IDE_RAISE( ERR_INVALID_CONDITION );
            break;
    }
    
    if ( sdi::isShardDDLForAddClone( aStatement ) != ID_TRUE )
    {                    
        /* zookeeper server에 죽어있는 노드가 있는지 체크한다. */
        IDE_TEST( sdiZookeeper::checkAllNodeAlive( &sIsValid )
                  != IDE_SUCCESS );
        IDE_TEST_RAISE( sIsValid == ID_FALSE, deadNode_exist );
    }
                
    /* BUG-48290 */
    IDE_TEST( qcg::setSessionIsInternalLocalOperation( aStatement, SDI_INTERNAL_OP_NORMAL) != IDE_SUCCESS);

    for ( sReShardAttr = sParseTree->mReShardAttr, i = 0;
          sReShardAttr != NULL;
          sReShardAttr = sReShardAttr->next, i++ )
    {
        if (QC_IS_NULL_NAME(sReShardAttr->mUserName) == ID_TRUE)
        {
            sUserID = QCG_GET_SESSION_USER_ID(aStatement);
            IDE_TEST(qciMisc::getUserName(aStatement, sUserID, sUserName) != IDE_SUCCESS);
            sReShardAttr->mUserName.offset = 0;
            sReShardAttr->mUserName.size = idlOS::strlen(sUserName);
            IDE_TEST( QC_QMP_MEM(aStatement)->cralloc( sReShardAttr->mUserName.size + 1,
                                                     (void **) &(sReShardAttr->mUserName.stmtText))
                      != IDE_SUCCESS );
            idlOS::strncpy(sReShardAttr->mUserName.stmtText, sUserName, sReShardAttr->mUserName.size + 1);
        }
        else
        {
            IDE_TEST(qciMisc::getUserID( aStatement, sReShardAttr->mUserName, &sUserID )
                     != IDE_SUCCESS);
            QC_STR_COPY( sUserName, sReShardAttr->mUserName );
        }

        IDE_TEST_RAISE( QC_IS_NULL_NAME(sReShardAttr->mObjectName) == ID_TRUE, ERR_OBJECT_NAME );
        QC_STR_COPY( sObjectName, sReShardAttr->mObjectName );

        switch (sReShardAttr->mObjectType)
        {
            case 'T':
                if ( QC_IS_NULL_NAME(sReShardAttr->mPartitionName) != ID_TRUE )
                {
                    QC_STR_COPY( sPartitionName, sReShardAttr->mPartitionName );
                }
                else
                {
                    sPartitionName[0] = '\0';
                }
                IDE_TEST( sdi::getTableInfoAllForDDL( aStatement,
                                                      sUserID,
                                                      (UChar*)sObjectName,
                                                      (SInt)idlOS::strlen(sObjectName),
                                                      ID_FALSE,
                                                      &(sReShardAttr->mTableInfo),
                                                      &(sReShardAttr->mTableSCN),
                                                      &(sReShardAttr->mTableHandle),
                                                      &(sReShardAttr->mShardObjectInfo) )
                          != IDE_SUCCESS);

                IDE_TEST( sdi::validateOneReshardTable( aStatement,
                                                        sUserName,
                                                        sObjectName,
                                                        sPartitionName,
                                                        sReShardAttr->mShardObjectInfo,
                                                        sReShardAttr->mFromNodeName,   /* out */
                                                        sReShardAttr->mDefaultNodeName /* out */
                                                      ) != IDE_SUCCESS );

                IDE_TEST_RAISE( ( sParseTree->mDDLType == SHARD_MOVE_REMOVE ) &&  /* remove syntax */
                                ( sdi::getShardObjectType( &(sReShardAttr->mShardObjectInfo->mTableInfo) )
                                  == SDI_SINGLE_SHARD_KEY_DIST_OBJECT ) &&
                                ( idlOS::strncmp( sReShardAttr->mDefaultNodeName,
                                                  sReShardAttr->mFromNodeName,
                                                  SDI_NODE_NAME_MAX_SIZE ) != 0 ) && /* not default node partition remove */
                                ( SDI_IS_NULL_NAME(sReShardAttr->mDefaultNodeName) != ID_TRUE ), /* exist default node */
                                ERR_EXIST_DEFAULT_NODE );

                if ( sdi::getShardObjectType( &(sReShardAttr->mShardObjectInfo->mTableInfo) ) == SDI_CLONE_DIST_OBJECT )
                {
                    sIsCloneExist = ID_TRUE;
                }
                else /* not clone */
                {
                    sIsNotCloneExist = ID_TRUE;
                }
                break;
            case 'P':
                if( sReShardAttr->mKeyValue != NULL )
                {
                    IDE_TEST( getProcedureKeyValue( sReShardAttr,
                                                    sProcKeyValue ) != IDE_SUCCESS );

                    if(sProcKeyValue[0] == '\0') /* strlen 0, it is DEFAULT */
                    {
                        sKeyValue = NULL;
                    }
                    else
                    {
                        sKeyValue = sProcKeyValue;
                    }
                }
                else
                {
                    /* clone & solo : do nothing */
                    sKeyValue = NULL;
                }
                IDE_TEST( sdi::getProcedureInfo(aStatement,
                                                sUserID,
                                                sReShardAttr->mUserName,
                                                sReShardAttr->mObjectName,
                                                &(sReShardAttr->mShardObjectInfo)) != IDE_SUCCESS );
                IDE_TEST_RAISE(sReShardAttr->mShardObjectInfo == NULL, ERR_NOT_FOUND_OBJ);

                IDE_TEST( sdi::validateOneReshardProc( aStatement,
                                                       sUserName,
                                                       sObjectName,
                                                       sKeyValue,
                                                       sReShardAttr->mShardObjectInfo,
                                                       sReShardAttr->mFromNodeName,   /* out */
                                                       sReShardAttr->mDefaultNodeName /* out */
                                                     ) != IDE_SUCCESS );
                if ( sdi::getShardObjectType( &(sReShardAttr->mShardObjectInfo->mTableInfo) ) == SDI_CLONE_DIST_OBJECT )
                {
                    sIsCloneExist = ID_TRUE;
                }
                else /* not clone */
                {
                    sIsNotCloneExist = ID_TRUE;
                }
                break;
            default:
                IDE_RAISE(ERR_INVALID_OBJ_TYPE);
                break;
        }

        if ( i == 0 ) /* first reshard attribute */
        {
            (void)idlOS::strncpy( sFromNodeName, sReShardAttr->mFromNodeName, SDI_NODE_NAME_MAX_SIZE + 1 );
        }
        else
        {
            /* the fromNodeName of reshard table must be one node */
            IDE_TEST_RAISE( idlOS::strncmp( sFromNodeName, 
                                            sReShardAttr->mFromNodeName, 
                                            SDI_NODE_NAME_MAX_SIZE ) != 0, ERR_FROM_NODE );
        }
    }
    IDE_TEST_RAISE( ( sIsCloneExist == ID_TRUE ) &&
                    ( QCG_GET_SESSION_IS_SHARD_INTERNAL_LOCAL_OPERATION( aStatement ) != ID_TRUE ),
                    ERR_CLONE_RESHARDING_NOT_SUPPORTED);
    /* clone and not clone resharding is not support, user can not execute clone resharding */
    IDE_TEST_RAISE(( sIsCloneExist == ID_TRUE ) && (sIsNotCloneExist == ID_TRUE), ERR_CLONE_RESHARDING_NOT_SUPPORTED);

    if ( sIsCloneExist == ID_TRUE )
    {
        sParseTree->mDDLType = SHARD_MOVE_TO_INTERNAL_CLONE_COPY;
    }
    else
    {
        IDE_DASSERT(sIsNotCloneExist == ID_TRUE);
        /* do nothing: keep orignal ddl type */
    }
    
    /***********************************************************************
     * check SMN
     ***********************************************************************/
    if ( sdi::isShardDDLForAddClone( aStatement ) != ID_TRUE )
    {                    
        IDE_TEST( checkZookeeperSMNAndDataSMN() != IDE_SUCCESS );

        if ( sIsAcquireShardMetaLock == ID_TRUE )
        {
            sdiZookeeper::releaseShardMetaLock();
            sIsAcquireShardMetaLock = ID_FALSE;
        }
    
        if ( sIsAcquireZookeeperMetaLock == ID_TRUE )
        {
            sdiZookeeper::releaseZookeeperMetaLock();
            sIsAcquireZookeeperMetaLock = ID_FALSE;
        }
    }
                
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_SHARD_META_NOT_CREATED ) );
    }
    IDE_EXCEPTION( ERR_FROM_NODE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_TOO_MANY_SOURCE_NODES_FOR_RESHARDING ) );
    }
    IDE_EXCEPTION( ERR_OBJECT_NAME );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_NOT_EXIST_OBJECT_NAME_FOR_RESHARDING ) );
    }
    IDE_EXCEPTION( ERR_NOT_FOUND_NODE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_INVALID_NODE_NAME, sToNodeName ) );
    }
    IDE_EXCEPTION( ERR_INVALID_CONDITION );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,"qdsd::validateShardMove", "Invalid condition." ) );
    }
    IDE_EXCEPTION( ERR_NOT_SUPPORTED_OPTION );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCP_SYNTAX,": REMOVE option is not supportted." ) );
    }
    IDE_EXCEPTION( ERR_CANNOT_RESHARDING_SHORT_OF_NODE_COUNT );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_TOO_FEW_NODES_FOR_RESHARDING ) );
    }
    IDE_EXCEPTION( ERR_EXIST_DEFAULT_NODE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_NOT_SUPPORT_REMOVE_SHARD_TABLE_WITH_DEFAULT_NODE ) );
    }
    IDE_EXCEPTION( deadNode_exist )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_ZKC_DEADNODE_EXIST ) );
    }
    IDE_EXCEPTION( ERR_NOT_FOUND_OBJ );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_SHARD_OBJECT_NOT_FOUND,
                                  sUserName,
                                  sObjectName) );
    }
    IDE_EXCEPTION( ERR_NOT_CONNECT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_SHARD_NOT_JOIN ) );
    }
    IDE_EXCEPTION( ERR_INVALID_OBJ_TYPE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qdsd::validateShardMove invalid object type",
                                  sReShardAttr->mObjectType ) );
    }
    IDE_EXCEPTION( ERR_CLONE_RESHARDING_NOT_SUPPORTED )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_NOT_SUPP_CLONE_RESHRD ) );
    }
    IDE_EXCEPTION_END;

    if ( sdi::isShardDDLForAddClone( aStatement ) != ID_TRUE )
    {
        // the logic below must be move to rollback.  
        if ( sIsAcquireShardMetaLock == ID_TRUE )
        {
            sdiZookeeper::releaseShardMetaLock();
            sIsAcquireShardMetaLock = ID_FALSE;
        }
        if ( sIsAcquireZookeeperMetaLock == ID_TRUE )
        {
            sdiZookeeper::releaseZookeeperMetaLock();
            sIsAcquireZookeeperMetaLock = ID_FALSE;
        }
    }
    
    return IDE_FAILURE;
}

IDE_RC qdsd::executeShardMove( qcStatement * aStatement )
{
/***********************************************************************
 *
 * Description :
 *    ALTER DATABASE SHARD MOVE [ {TABLE object_name {PARTITION ( partition_name ) }} |
 *                                {PROCEDURE object_name {KEY ( key_value )} } ]+
 *    TO node_name
 *
 * Implementation :
 *    1. parse tree shard object smn validation
 *    2. make name for resharding toNode
 *    3. check node is exist and self resharding
 *    4. get replicaSets fromNode and toNode. and check replication running
 *    5. Determine whether the primary data should be synchronized.
 *    6. Determine whether the first backup data should be synchronized.
 *    7. Determine whether the second backup data should be synchronized.
 *    8. correct truncate flag from k-safey and count of nodes
 *    ex> k-safety : 1
 *    1. alter database shard move table t1 partition p2 to N4
 * <BEFORE>     N1        N2        N3        N4
 * Primary      P1     |--P2--|     P3        P4
 * Backup       P4     |  P1  |     P2        P3
 * <AFTER>      N1     |  N2  |     N3        N4
 * Primary      P1     |      |     P3        P2, P4
 * Backup       P4,P2<-|  P1  |----------------^  P3
 *    2. alter database shard move table t1 partition p2 to N1
 * <BEFORE>     N1        N2        N3        N4
 * Primary      P1     |--P2-----|  P3        P4
 * Backup       P4     |  P1     |  P2        P3
 * <AFTER>      N1     |  N2     |  N3        N4
 * Primary      P1,P2  |         |  P3        P4
 * Backup       P4 ^---|  P1,P2<-|            P3
 ***********************************************************************/
    qdShardParseTree   * sParseTree = NULL;
    qdReShardAttribute * sReShardAttr;
    sdiLocalMetaInfo     sLocalMetaInfo;
    SChar                sFromNodeName[SDI_NODE_NAME_MAX_SIZE + 1] = { 0, };
    SChar                sToNodeName[SDI_NODE_NAME_MAX_SIZE + 1] = { 0, };
    sdiReplicaSetInfo  sFromReplicaSetsInfo;
    sdiReplicaSetInfo  sToReplicaSetsInfo;
    sdiReplicaSet    * sFromReplicaSetPtr = NULL;
    sdiReplicaSet    * sToReplicaSetPtr = NULL;
    sdiReplicaSet      sToReplicaSet;
    sdiReplicaSet      sFromReplicaSet;
    ULong              sSMN = SDI_NULL_SMN;
    idBool             sIsPrimaryDataSync = ID_FALSE;
    idBool             sIsFirstBackupDataSync = ID_FALSE;
    idBool             sIsSecondBackupDataSync = ID_FALSE;
    idBool             sIsDealloc = ID_FALSE;
    SChar            * sSqlStr = NULL;
    SChar              sPrimaryReplicationName[SDI_REPLICATION_NAME_MAX_SIZE + 1] = { 0, };
    SChar              sFirstBackupReplicationName[SDI_REPLICATION_NAME_MAX_SIZE + 1] = { 0, };
    SChar              sSecondBackupReplicationName[SDI_REPLICATION_NAME_MAX_SIZE + 1] = { 0, };
    idBool             sIsFromNodeTruncate = ID_TRUE;
    idBool             sIsFirstBackupOfFromNodeTruncate = ID_TRUE;
    idBool             sIsSecondBackupOfFromNodeTruncate = ID_TRUE;
    sdiLocalMetaInfo * sToNodeLocalInfo = NULL;
    UInt               sShardDDLLockTryCount = QCG_GET_SESSION_SHARD_DDL_LOCK_TRY_COUNT(aStatement);
    UInt               sRemoteLockTimeout = QCG_GET_SESSION_SHARD_DDL_LOCK_TIMEOUT(aStatement);
    sdiGlobalMetaInfo  sMetaNodeInfo = { ID_ULONG(0) };

    sdiNode            sNode; 
    UInt               i;
    
    sParseTree = (qdShardParseTree *)aStatement->myPlan->parseTree;

    /***********************************************************************
     * check internal property validate
     ***********************************************************************/
    if ( sdi::isShardDDLForAddClone( aStatement ) != ID_TRUE )
    {
        IDE_TEST( validateCommonForExecute( aStatement ) != IDE_SUCCESS );

        /* for smn propagation to all nodes at commit */
        sdi::setShardMetaTouched( aStatement->session );

        IDE_TEST( sdiZookeeper::getZookeeperMetaLock(QCG_GET_SESSION_ID(aStatement)) != IDE_SUCCESS );
        IDE_TEST( sdiZookeeper::getShardMetaLock( QC_SMI_STMT(aStatement)->getTrans()->getTransID() )
                  != IDE_SUCCESS );

        IDE_TEST( getAllNodeInfoList( aStatement ) != IDE_SUCCESS );

        sdiZookeeper::reshardingJob();
    }

    /* 1. parse tree shard object smn validation */
    for ( sReShardAttr = sParseTree->mReShardAttr;
          sReShardAttr != NULL;
          sReShardAttr = sReShardAttr->next )
    {
        IDE_TEST( sdi::validateObjectSMN(aStatement, sReShardAttr->mShardObjectInfo) != IDE_SUCCESS );
    }
    /* mFromNodeName of all reshard attr are identical */
    (void)idlOS::strncpy( sFromNodeName, sParseTree->mReShardAttr->mFromNodeName, SDI_NODE_NAME_MAX_SIZE + 1 );

    /* 2. make resharding tonode name */
    switch(sParseTree->mDDLType)
    {
        case SHARD_MOVE_TO:
        case SHARD_MOVE_TO_INTERNAL_CLONE_COPY:
            IDE_DASSERT( QC_IS_NULL_NAME(sParseTree->mNodeName) != ID_TRUE );
            QC_STR_COPY( sToNodeName, sParseTree->mNodeName );
            break;
        case SHARD_MOVE_REMOVE:
            SDI_SET_NULL_NAME( sToNodeName );
            break;
        case SHARD_DDL_UNKNOWN:
        case SHARD_ADD:
        case SHARD_JOIN:
        case SHARD_DROP:
        case SHARD_DROP_FORCE:
        case SHARD_FAILOVER_NORMAL:
        case SHARD_FAILOVER_IMMEDIATE:
        case SHARD_FAILOVER_EXIT:
        case SHARD_FAILOVER_FORCE:
        case SHARD_FAILBACK:
        default:
            IDE_RAISE( ERR_INVALID_CONDITION );
            break;
    }

    /* 3. check node is exist and self resharding */
    if ( SDI_IS_NULL_NAME(sToNodeName) != ID_TRUE )
    {
        sToNodeLocalInfo = sdiZookeeper::getNodeInfo(sParseTree->mNodeInfoList, sToNodeName);
        IDE_TEST_RAISE( sToNodeLocalInfo == NULL, ERR_NOT_FOUND_NODE);
        IDE_TEST_RAISE( sdi::isSameNode(sToNodeName, sFromNodeName) == ID_TRUE, ERR_SAME_NODE );
    }

    /* 4. get replicaSets fromNode and toNode. and check replication running   */
    if ( sdi::isShardDDLForAddClone( aStatement ) == ID_TRUE )
    {
        IDE_TEST( sdi::getGlobalMetaInfoCore( QC_SMI_STMT( aStatement ),
                                              &sMetaNodeInfo )
                  != IDE_SUCCESS );

        sSMN = sMetaNodeInfo.mShardMetaNumber;
    }
    else
    {    
        sSMN = sdi::getSMNForDataNode();
    }
   
    IDE_TEST( sdi::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );

    if ( idlOS::strncmp( sFromNodeName,
                         SDI_NODE_DEALLOC_NAME,
                         SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
    {
        sIsDealloc = ID_TRUE;

        SDI_INIT_REPLICA_SET(&sFromReplicaSet);
        sFromReplicaSetPtr = &sFromReplicaSet; 

        if ( SDI_IS_NULL_NAME(sToNodeName) != ID_TRUE )
        {
            IDE_TEST( sdi::getReplicaSet( QC_SMI_STMT(aStatement)->getTrans(),
                                          sToNodeName,
                                          ID_FALSE,
                                          sSMN,
                                          &sToReplicaSetsInfo ) != IDE_SUCCESS );

            SDI_INIT_NODE(&sNode);
            IDE_TEST( sdm::getNodeByName( QC_SMI_STMT( aStatement ),
                                          sToNodeName,
                                          sSMN,
                                          &sNode )
                      != IDE_SUCCESS );

            /* ToNode에 ReplicaSet이 여러개 연결되어 있는 상태일 수 있다. ToNode의 MainReplicaSet을 찾는다. */
            for ( i = 0 ; i < sToReplicaSetsInfo.mCount; i++ )
            {
                if ( sToReplicaSetsInfo.mReplicaSets[i].mReplicaSetId == sNode.mNodeId )
                {
                    sToReplicaSetPtr = &sToReplicaSetsInfo.mReplicaSets[i];
                    break;
                }
            }
            IDE_TEST_RAISE( sToReplicaSetPtr == NULL, ERR_SHARD_META );

            IDE_TEST( sdi::checkBackupReplicationRunning( sLocalMetaInfo.mKSafety,
                                                          sToReplicaSetPtr,
                                                          sParseTree->mNodeCount ) != IDE_SUCCESS );
        }
        else
        {
            /* DEALLOC을 MOVE 하는 경우 To Null은 지원하지 않음.
             * 위에서 거르고 있으나 이후 REMOVE 제공시에 필요 */
            IDE_RAISE( ERR_NOT_FOUND_NODE );
        }

        /* DEALLOC에서 새로 할당이기 때문에 Data Sync 필요 없음. */
        /* 5. Determine whether the primary data should be synchronized. */
        SDI_SET_NULL_NAME(sPrimaryReplicationName);
        sIsPrimaryDataSync = ID_FALSE;

        /* 6. Determine whether the first backup data should be synchronized. */
        SDI_SET_NULL_NAME(sFirstBackupReplicationName);
        sIsFirstBackupDataSync = ID_FALSE;

        /* 7. Determine whether the second backup data should be synchronized. */
        SDI_SET_NULL_NAME(sSecondBackupReplicationName);
        sIsSecondBackupDataSync = ID_FALSE;

        /* 8. correct truncate flag: exception situation by k-safey and count of nodes and clone resharding */
        sIsFromNodeTruncate = ID_FALSE;
        sIsFirstBackupOfFromNodeTruncate = ID_FALSE;
        sIsSecondBackupOfFromNodeTruncate = ID_FALSE;
    }
    else
    {
    
        IDE_TEST( sdi::getReplicaSet( QC_SMI_STMT(aStatement)->getTrans(),
                                      sFromNodeName,
                                      ID_FALSE,
                                      sSMN,
                                      &sFromReplicaSetsInfo ) != IDE_SUCCESS );

        IDE_TEST_RAISE( sFromReplicaSetsInfo.mCount != 1, ERR_SHARD_META );
        sFromReplicaSetPtr = &sFromReplicaSetsInfo.mReplicaSets[0];
        
        /* Backup Replications running check, 
         * If backup replications of from node and to node does not running normally 
         * then you cannot run resharding.
         */
        IDE_TEST( sdi::checkBackupReplicationRunning( sLocalMetaInfo.mKSafety,
                                                      sFromReplicaSetPtr,
                                                      sParseTree->mNodeCount )
                  != IDE_SUCCESS );

        if ( SDI_IS_NULL_NAME(sToNodeName) != ID_TRUE )
        {
            IDE_TEST( sdi::getReplicaSet( QC_SMI_STMT(aStatement)->getTrans(),
                                          sToNodeName,
                                          ID_FALSE,
                                          sSMN,
                                          &sToReplicaSetsInfo ) != IDE_SUCCESS );
            IDE_TEST_RAISE( sToReplicaSetsInfo.mCount != 1, ERR_SHARD_META );
            sToReplicaSetPtr = &sToReplicaSetsInfo.mReplicaSets[0];
            IDE_TEST( sdi::checkBackupReplicationRunning( sLocalMetaInfo.mKSafety,
                                                          sToReplicaSetPtr,
                                                          sParseTree->mNodeCount ) != IDE_SUCCESS );
        }
        else
        {
            SDI_INIT_REPLICA_SET(&sToReplicaSet);
            sToReplicaSetPtr = &sToReplicaSet;
        }

        /* 5. Determine whether the primary data should be synchronized. */
        if ( SDI_IS_NULL_NAME(sToNodeName) == ID_TRUE )
        {
            /* REMOVE */
            SDI_SET_NULL_NAME(sPrimaryReplicationName);
            sIsPrimaryDataSync = ID_FALSE;
        }
        else
        {
            if ( sParseTree->mDDLType == SHARD_MOVE_TO_INTERNAL_CLONE_COPY )
            {
                idlOS::snprintf(sPrimaryReplicationName,
                                SDI_REPLICATION_NAME_MAX_SIZE + 1,
                                "%s#%s#PR",                   /* nodename size = 10 *2 , + # + # + PR => 24 */
                                sFromNodeName,
                                sToNodeName);
                sIsPrimaryDataSync = ID_TRUE;
            }
            else if ( sdi::isSameNode(sToNodeName, sFromReplicaSetPtr->mFirstBackupNodeName) == ID_TRUE )
            {
                /* the resharding target(to) node of primary data is first backup node
                 * the primary data is in backup table of to node, therefore, the primary data does not need sync
                 */
                idlOS::strncpy(sPrimaryReplicationName, sFromReplicaSetPtr->mFirstReplName, SDI_REPLICATION_NAME_MAX_SIZE + 1);
                sIsPrimaryDataSync = ID_FALSE;
                sIsFirstBackupOfFromNodeTruncate = ID_FALSE;
            }
            else if ( sdi::isSameNode(sToNodeName, sFromReplicaSetPtr->mSecondBackupNodeName) == ID_TRUE )
            {
                /* the resharding target(to) node of primary data is second backup node
                 * the primary data is in backup table of to node, therefore,  the primary data does not need sync
                 */
                idlOS::strncpy(sPrimaryReplicationName, sFromReplicaSetPtr->mSecondReplName, SDI_REPLICATION_NAME_MAX_SIZE + 1);
                sIsPrimaryDataSync = ID_FALSE;
                sIsSecondBackupOfFromNodeTruncate = ID_FALSE;
            }
            else /* not first or second backup node */
            {
                idlOS::snprintf(sPrimaryReplicationName,
                                SDI_REPLICATION_NAME_MAX_SIZE + 1,
                                "%s#%s#PR",                   /* nodename size = 10 *2 , + # + # + PR => 24 */
                                sFromNodeName,
                                sToNodeName);
                sIsPrimaryDataSync = ID_TRUE;
            }
        }

        /* 6. Determine whether the first backup data should be synchronized. */

        if ( SDI_IS_NULL_NAME(sToReplicaSetPtr->mFirstBackupNodeName) == ID_TRUE )
        {
            SDI_SET_NULL_NAME(sFirstBackupReplicationName);
            sIsFirstBackupDataSync = ID_FALSE;
        }
        else
        {
            if ( sParseTree->mDDLType == SHARD_MOVE_TO_INTERNAL_CLONE_COPY )
            {
                SDI_SET_NULL_NAME(sFirstBackupReplicationName);
                sIsFirstBackupDataSync = ID_FALSE;
            }
            else if ( sdi::isSameNode(sToReplicaSetPtr->mFirstBackupNodeName, sFromReplicaSetPtr->mFirstBackupNodeName) == ID_TRUE )
            {
                idlOS::strncpy(sFirstBackupReplicationName, sFromReplicaSetPtr->mFirstReplName, SDI_REPLICATION_NAME_MAX_SIZE + 1);
                sIsFirstBackupDataSync = ID_FALSE;
                sIsFirstBackupOfFromNodeTruncate = ID_FALSE;
            }
            else if ( sdi::isSameNode(sToReplicaSetPtr->mFirstBackupNodeName, sFromReplicaSetPtr->mSecondBackupNodeName) == ID_TRUE )
            {
                idlOS::strncpy(sFirstBackupReplicationName, sFromReplicaSetPtr->mSecondReplName, SDI_REPLICATION_NAME_MAX_SIZE + 1);
                sIsFirstBackupDataSync = ID_FALSE;
                sIsSecondBackupOfFromNodeTruncate = ID_FALSE;
            }
            else if ( sdi::isSameNode(sToReplicaSetPtr->mFirstBackupNodeName, sFromNodeName) == ID_TRUE )
            {
                SDI_SET_NULL_NAME(sFirstBackupReplicationName);
                sIsFirstBackupDataSync = ID_FALSE;
                sIsFromNodeTruncate = ID_FALSE;
            }
            else
            {
                idlOS::snprintf(sFirstBackupReplicationName,
                                SDI_REPLICATION_NAME_MAX_SIZE + 1,
                                "%s#%s#FB",                   /* nodename size = 10 *2 , + # + # + FB => 24 */
                                sFromNodeName,
                                sToReplicaSetPtr->mFirstBackupNodeName);
                sIsFirstBackupDataSync = ID_TRUE;
            }
        }
        /* 7. Determine whether the second backup data should be synchronized. */
        if ( SDI_IS_NULL_NAME(sToReplicaSetPtr->mSecondBackupNodeName) == ID_TRUE )
        {
            SDI_SET_NULL_NAME(sSecondBackupReplicationName);
            sIsSecondBackupDataSync = ID_FALSE;
        }
        else
        {
            if ( sParseTree->mDDLType == SHARD_MOVE_TO_INTERNAL_CLONE_COPY )
            {
                SDI_SET_NULL_NAME(sSecondBackupReplicationName);
                sIsSecondBackupDataSync = ID_FALSE;
            }
            else if ( sdi::isSameNode(sToReplicaSetPtr->mSecondBackupNodeName, sFromReplicaSetPtr->mFirstBackupNodeName) == ID_TRUE )
            {
                idlOS::strncpy(sSecondBackupReplicationName, sFromReplicaSetPtr->mFirstReplName, SDI_REPLICATION_NAME_MAX_SIZE + 1);
                sIsSecondBackupDataSync = ID_FALSE;
                sIsFirstBackupOfFromNodeTruncate = ID_FALSE;
            }
            else if ( sdi::isSameNode(sToReplicaSetPtr->mSecondBackupNodeName, sFromReplicaSetPtr->mSecondBackupNodeName) == ID_TRUE )
            {
                idlOS::strncpy(sSecondBackupReplicationName, sFromReplicaSetPtr->mSecondReplName, SDI_REPLICATION_NAME_MAX_SIZE + 1);
                sIsSecondBackupDataSync = ID_FALSE;
                sIsSecondBackupOfFromNodeTruncate = ID_FALSE;
            }
            else if ( sdi::isSameNode(sToReplicaSetPtr->mSecondBackupNodeName, sFromNodeName) == ID_TRUE )
            {
                SDI_SET_NULL_NAME(sSecondBackupReplicationName);
                sIsSecondBackupDataSync = ID_FALSE;
                sIsFromNodeTruncate = ID_FALSE;
            }
            else
            {
                idlOS::snprintf(sSecondBackupReplicationName,
                                SDI_REPLICATION_NAME_MAX_SIZE + 1,
                                "%s#%s#SB",                   /* nodename size = 10 *2 , + # + # + SB => 24 */
                                sFromNodeName,
                                sToReplicaSetPtr->mSecondBackupNodeName);
                sIsSecondBackupDataSync = ID_TRUE;
            }
        }

        /* 8. correct truncate flag: exception situation by k-safey and count of nodes and clone resharding */
        if ( sParseTree->mDDLType == SHARD_MOVE_TO_INTERNAL_CLONE_COPY )
        {
            sIsFromNodeTruncate = ID_FALSE;
            sIsFirstBackupOfFromNodeTruncate = ID_FALSE;
            sIsSecondBackupOfFromNodeTruncate = ID_FALSE;
        }
        else
        {
            if ( ( sIsFirstBackupOfFromNodeTruncate == ID_TRUE ) &&
                 ( SDI_IS_NULL_NAME(sFromReplicaSetPtr->mFirstBackupNodeName) == ID_TRUE ) )
            {
                sIsFirstBackupOfFromNodeTruncate = ID_FALSE;
            }

            if ( ( sIsSecondBackupOfFromNodeTruncate == ID_TRUE ) &&
                 ( SDI_IS_NULL_NAME(sFromReplicaSetPtr->mSecondBackupNodeName) == ID_TRUE ) )
            {
                sIsSecondBackupOfFromNodeTruncate = ID_FALSE;
            }
        }

        IDE_DASSERT(SDI_IS_NULL_NAME(sFromNodeName) != ID_TRUE);

    }

    /* start resharding operation */
    IDE_TEST( sdi::checkShardLinker( aStatement ) != IDE_SUCCESS );
    IDE_TEST_RAISE( aStatement->session->mQPSpecific.mClientInfo == NULL, ERR_NOT_EXIST_NODE );

    /* until here, prepare resharding with new transaction
     * from the logic below to commit transaction is one transaction.
     */
    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS );

    if ( sIsPrimaryDataSync == ID_TRUE )
    {
        // BUG-48616
        if ( sdi::isShardDDLForAddClone( aStatement ) == ID_TRUE )
        {
            IDE_TEST( checkCloneTableDataAndTruncateForShardAdd( aStatement,
                                                                 sParseTree->mReShardAttr )
                      != IDE_SUCCESS);
        }
        else
        {
            IDE_TEST(clearBackupDataWithNewTrans( aStatement->session->mMmSession,
                                                  sToNodeName,
                                                  sParseTree->mReShardAttr )
                     != IDE_SUCCESS);
        }
        
        IDE_TEST( prepareReplicationsForSync( aStatement,
                                              sParseTree->mNodeInfoList,
                                              sFromNodeName,
                                              sToNodeName,
                                              sPrimaryReplicationName,
                                              sParseTree->mReShardAttr )
                  != IDE_SUCCESS );

        idlOS::snprintf(sSqlStr,
                        QD_MAX_SQL_LENGTH,
                        "alter replication "QCM_SQL_STRING_SKIP_FMT" stop",
                        sPrimaryReplicationName);
        (void)sdiZookeeper::addPendingJob(sSqlStr,
                                          sFromNodeName,
                                          ZK_PENDING_JOB_AFTER_ROLLBACK,
                                          QCI_STMT_MASK_MAX );
        idlOS::snprintf(sSqlStr,
                        QD_MAX_SQL_LENGTH,
                        "drop replication "QCM_SQL_STRING_SKIP_FMT"",
                        sPrimaryReplicationName);
        (void)sdiZookeeper::addPendingJob(sSqlStr,
                                          sFromNodeName,
                                          ZK_PENDING_JOB_AFTER_ROLLBACK,
                                          QCI_STMT_MASK_MAX );
        (void)sdiZookeeper::addPendingJob( sSqlStr,
                                           sToNodeName,
                                           ZK_PENDING_JOB_AFTER_ROLLBACK,
                                           QCI_STMT_MASK_MAX );
    }

    if ( sIsFirstBackupDataSync == ID_TRUE )
    {
        IDE_TEST(clearBackupDataWithNewTrans( aStatement->session->mMmSession,
                                              sToReplicaSetPtr->mFirstBackupNodeName,
                                              sParseTree->mReShardAttr )
                 != IDE_SUCCESS);
        IDE_TEST( prepareReplicationsForSync( aStatement,
                                              sParseTree->mNodeInfoList,
                                              sFromNodeName,
                                              sToReplicaSetPtr->mFirstBackupNodeName,
                                              sFirstBackupReplicationName,
                                              sParseTree->mReShardAttr )
                  != IDE_SUCCESS );

        idlOS::snprintf(sSqlStr,
                        QD_MAX_SQL_LENGTH,
                        "alter replication "QCM_SQL_STRING_SKIP_FMT" stop",
                        sFirstBackupReplicationName);
        (void)sdiZookeeper::addPendingJob(sSqlStr,
                                          sFromNodeName,
                                          ZK_PENDING_JOB_AFTER_ROLLBACK,
                                          QCI_STMT_MASK_MAX );
        idlOS::snprintf(sSqlStr,
                        QD_MAX_SQL_LENGTH,
                        "drop replication "QCM_SQL_STRING_SKIP_FMT"",
                        sFirstBackupReplicationName);
        (void)sdiZookeeper::addPendingJob(sSqlStr,
                                          sFromNodeName,
                                          ZK_PENDING_JOB_AFTER_ROLLBACK,
                                          QCI_STMT_MASK_MAX );
        (void)sdiZookeeper::addPendingJob( sSqlStr,
                                           sToReplicaSetPtr->mFirstBackupNodeName,
                                           ZK_PENDING_JOB_AFTER_ROLLBACK,
                                           QCI_STMT_MASK_MAX );
    }

    if ( sIsSecondBackupDataSync == ID_TRUE )
    {
        IDE_TEST(clearBackupDataWithNewTrans( aStatement->session->mMmSession,
                                              sToReplicaSetPtr->mSecondBackupNodeName,
                                              sParseTree->mReShardAttr )
                 != IDE_SUCCESS);
        IDE_TEST( prepareReplicationsForSync( aStatement,
                                              sParseTree->mNodeInfoList,
                                              sFromNodeName,
                                              sToReplicaSetPtr->mSecondBackupNodeName,
                                              sSecondBackupReplicationName,
                                              sParseTree->mReShardAttr )
                  != IDE_SUCCESS );

        idlOS::snprintf(sSqlStr,
                        QD_MAX_SQL_LENGTH,
                        "alter replication "QCM_SQL_STRING_SKIP_FMT" stop",
                        sSecondBackupReplicationName);
        (void)sdiZookeeper::addPendingJob(sSqlStr,
                                          sFromNodeName,
                                          ZK_PENDING_JOB_AFTER_END_ALL,
                                          QCI_STMT_MASK_MAX );
        idlOS::snprintf(sSqlStr,
                        QD_MAX_SQL_LENGTH,
                        "drop replication "QCM_SQL_STRING_SKIP_FMT"",
                        sSecondBackupReplicationName);
        (void)sdiZookeeper::addPendingJob(sSqlStr,
                                          sFromNodeName,
                                          ZK_PENDING_JOB_AFTER_END_ALL,
                                          QCI_STMT_MASK_MAX );
        (void)sdiZookeeper::addPendingJob( sSqlStr,
                                           sToReplicaSetPtr->mSecondBackupNodeName,
                                           ZK_PENDING_JOB_AFTER_END_ALL,
                                           QCI_STMT_MASK_MAX );
    }

    /* 1. there is serial sync problem , but it goes to serial this time */
    /* 2. there is a conditional sync performance problem, reviewreview
     *    A user may want to synchronize parallel. 
     *    but currently, the conditional sync do not support parallel sync.
     */
    if ( sIsPrimaryDataSync == ID_TRUE )
    {
        IDE_TEST( executeRemoteSQLWithNewTransArg( aStatement,
                                                   sFromNodeName,
                                                   SDI_INTERNAL_OP_NORMAL,
                                                   QCI_STMT_MASK_DCL,
                                                   sSqlStr,
                                                   QD_MAX_SQL_LENGTH,
                                                   ID_FALSE,
                                                   "alter replication "QCM_SQL_STRING_SKIP_FMT" sync conditional",
                                                   sPrimaryReplicationName) != IDE_SUCCESS );
    }
    if ( sIsFirstBackupDataSync == ID_TRUE )
    {
        IDE_TEST( executeRemoteSQLWithNewTransArg( aStatement,
                                                   sFromNodeName,
                                                   SDI_INTERNAL_OP_NORMAL,
                                                   QCI_STMT_MASK_DCL,
                                                   sSqlStr,
                                                   QD_MAX_SQL_LENGTH,
                                                   ID_FALSE,
                                                   "alter replication "QCM_SQL_STRING_SKIP_FMT" sync conditional",
                                                   sFirstBackupReplicationName) != IDE_SUCCESS );
    }
    if ( sIsSecondBackupDataSync == ID_TRUE )
    {
        IDE_TEST( executeRemoteSQLWithNewTransArg( aStatement,
                                                   sFromNodeName,
                                                   SDI_INTERNAL_OP_NORMAL,
                                                   QCI_STMT_MASK_DCL,
                                                   sSqlStr,
                                                   QD_MAX_SQL_LENGTH,
                                                   ID_FALSE,
                                                   "alter replication "QCM_SQL_STRING_SKIP_FMT" sync conditional",
                                                   sSecondBackupReplicationName) != IDE_SUCCESS );
    }

    if ( SDI_IS_NULL_NAME(sPrimaryReplicationName) != ID_TRUE )
    {
        IDE_TEST( executeRemoteSQLWithNewTransArg( aStatement,
                                                   sFromNodeName,
                                                   SDI_INTERNAL_OP_NORMAL,
                                                   QCI_STMT_MASK_DCL,
                                                   sSqlStr,
                                                   QD_MAX_SQL_LENGTH,
                                                   ID_FALSE,
                                                   "alter replication "QCM_SQL_STRING_SKIP_FMT" flush wait %"ID_UINT32_FMT,
                                                   sPrimaryReplicationName,
                                                   (sShardDDLLockTryCount * sRemoteLockTimeout)
                                                   ) != IDE_SUCCESS );
    }

    if ( SDI_IS_NULL_NAME(sFirstBackupReplicationName) != ID_TRUE )
    {
        IDE_TEST( executeRemoteSQLWithNewTransArg( aStatement,
                                                   sFromNodeName,
                                                   SDI_INTERNAL_OP_NORMAL,
                                                   QCI_STMT_MASK_DCL,
                                                   sSqlStr,
                                                   QD_MAX_SQL_LENGTH,
                                                   ID_FALSE,
                                                   "alter replication "QCM_SQL_STRING_SKIP_FMT" flush wait %"ID_UINT32_FMT,
                                                   sFirstBackupReplicationName,
                                                   (sShardDDLLockTryCount * sRemoteLockTimeout)
                                                   ) != IDE_SUCCESS );
    }

    if ( SDI_IS_NULL_NAME(sSecondBackupReplicationName) != ID_TRUE )
    {
        IDE_TEST( executeRemoteSQLWithNewTransArg( aStatement,
                                                   sFromNodeName,
                                                   SDI_INTERNAL_OP_NORMAL,
                                                   QCI_STMT_MASK_DCL,
                                                   sSqlStr,
                                                   QD_MAX_SQL_LENGTH,
                                                   ID_FALSE,
                                                   "alter replication "QCM_SQL_STRING_SKIP_FMT" flush wait %"ID_UINT32_FMT,
                                                   sSecondBackupReplicationName,
                                                   (sShardDDLLockTryCount * sRemoteLockTimeout)
                                                   ) != IDE_SUCCESS );
    }

    /* until here, data sync end. 
     * table locks to execute resharding main swap operation
     */
    IDE_TEST( lockAllRemoteReshardTables( aStatement,
                                          sSqlStr,
                                          QD_MAX_SQL_LENGTH,
                                          sFromNodeName,
                                          sToNodeName,
                                          sParseTree->mReShardAttr ) != IDE_SUCCESS );

    if ( SDI_IS_NULL_NAME(sPrimaryReplicationName) != ID_TRUE )
    {
        IDE_TEST( executeRemoteSQLWithNewTransArg( aStatement,
                                                   sFromNodeName,
                                                   SDI_INTERNAL_OP_NORMAL,
                                                   QCI_STMT_MASK_DCL,
                                                   sSqlStr,
                                                   QD_MAX_SQL_LENGTH,
                                                   ID_FALSE,
                                                   "alter replication "QCM_SQL_STRING_SKIP_FMT" flush wait %"ID_UINT32_FMT,
                                                   sPrimaryReplicationName,
                                                   (sShardDDLLockTryCount * sRemoteLockTimeout)
                                                   ) != IDE_SUCCESS );
    }

    if ( SDI_IS_NULL_NAME(sFirstBackupReplicationName) != ID_TRUE )
    {
        IDE_TEST( executeRemoteSQLWithNewTransArg( aStatement,
                                                   sFromNodeName,
                                                   SDI_INTERNAL_OP_NORMAL,
                                                   QCI_STMT_MASK_DCL,
                                                   sSqlStr,
                                                   QD_MAX_SQL_LENGTH,
                                                   ID_FALSE,
                                                   "alter replication "QCM_SQL_STRING_SKIP_FMT" flush wait %"ID_UINT32_FMT,
                                                   sFirstBackupReplicationName,
                                                   (sShardDDLLockTryCount * sRemoteLockTimeout)
                                                   ) != IDE_SUCCESS );
    }

    if ( SDI_IS_NULL_NAME(sSecondBackupReplicationName) != ID_TRUE )
    {
        IDE_TEST( executeRemoteSQLWithNewTransArg( aStatement,
                                                   sFromNodeName,
                                                   SDI_INTERNAL_OP_NORMAL,
                                                   QCI_STMT_MASK_DCL,
                                                   sSqlStr,
                                                   QD_MAX_SQL_LENGTH,
                                                   ID_FALSE,
                                                   "alter replication "QCM_SQL_STRING_SKIP_FMT" flush wait %"ID_UINT32_FMT,
                                                   sSecondBackupReplicationName,
                                                   (sShardDDLLockTryCount * sRemoteLockTimeout)
                                                   ) != IDE_SUCCESS );
    }

    if ( sIsPrimaryDataSync == ID_TRUE )
    {
        IDE_TEST( executeRemoteSQLWithNewTransArg( aStatement,
                                                   sFromNodeName,
                                                   SDI_INTERNAL_OP_NORMAL,
                                                   QCI_STMT_MASK_DCL,
                                                   sSqlStr,
                                                   QD_MAX_SQL_LENGTH,
                                                   ID_FALSE,
                                                   "alter replication "QCM_SQL_STRING_SKIP_FMT" stop",
                                                   sPrimaryReplicationName) != IDE_SUCCESS );
    }
    if ( sIsFirstBackupDataSync == ID_TRUE )
    {
        IDE_TEST( executeRemoteSQLWithNewTransArg( aStatement,
                                                   sFromNodeName,
                                                   SDI_INTERNAL_OP_NORMAL,
                                                   QCI_STMT_MASK_DCL,
                                                   sSqlStr,
                                                   QD_MAX_SQL_LENGTH,
                                                   ID_FALSE,
                                                   "alter replication "QCM_SQL_STRING_SKIP_FMT" stop",
                                                   sFirstBackupReplicationName) != IDE_SUCCESS );
    }
    if ( sIsSecondBackupDataSync == ID_TRUE )
    {
        IDE_TEST( executeRemoteSQLWithNewTransArg( aStatement,
                                                   sFromNodeName,
                                                   SDI_INTERNAL_OP_NORMAL,
                                                   QCI_STMT_MASK_DCL,
                                                   sSqlStr,
                                                   QD_MAX_SQL_LENGTH,
                                                   ID_FALSE,
                                                   "alter replication "QCM_SQL_STRING_SKIP_FMT" stop",
                                                   sSecondBackupReplicationName) != IDE_SUCCESS );
    }
    
    /* temp replication drop: the drop replication operation must be executed by the transaction that was acquired lock. */
    if ( sIsPrimaryDataSync == ID_TRUE )
    {
        IDE_TEST( executeRemoteSQL( aStatement,
                                    sFromNodeName,
                                    SDI_INTERNAL_OP_NORMAL,
                                    QCI_STMT_MASK_DDL,
                                    sSqlStr,
                                    QD_MAX_SQL_LENGTH,
                                    "drop replication "QCM_SQL_STRING_SKIP_FMT"",
                                    sPrimaryReplicationName) != IDE_SUCCESS );
        IDE_TEST( executeRemoteSQL( aStatement,
                                    sToNodeName,
                                    SDI_INTERNAL_OP_NORMAL,
                                    QCI_STMT_MASK_DDL,
                                    sSqlStr,
                                    QD_MAX_SQL_LENGTH,
                                    "drop replication "QCM_SQL_STRING_SKIP_FMT"",
                                    sPrimaryReplicationName) != IDE_SUCCESS );
    }
    if ( sIsFirstBackupDataSync == ID_TRUE )
    {
        IDE_TEST( executeRemoteSQL( aStatement,
                                    sFromNodeName,
                                    SDI_INTERNAL_OP_NORMAL,
                                    QCI_STMT_MASK_DDL,
                                    sSqlStr,
                                    QD_MAX_SQL_LENGTH,
                                    "drop replication "QCM_SQL_STRING_SKIP_FMT"",
                                    sFirstBackupReplicationName) != IDE_SUCCESS );
        IDE_TEST( executeRemoteSQL( aStatement,
                                    sToReplicaSetPtr->mFirstBackupNodeName,
                                    SDI_INTERNAL_OP_NORMAL,
                                    QCI_STMT_MASK_DDL,
                                    sSqlStr,
                                    QD_MAX_SQL_LENGTH,
                                    "drop replication "QCM_SQL_STRING_SKIP_FMT"",
                                    sFirstBackupReplicationName) != IDE_SUCCESS );
    }
    if ( sIsSecondBackupDataSync == ID_TRUE )
    {
        IDE_TEST( executeRemoteSQL( aStatement,
                                    sFromNodeName,
                                    SDI_INTERNAL_OP_NORMAL,
                                    QCI_STMT_MASK_DDL,
                                    sSqlStr,
                                    QD_MAX_SQL_LENGTH,
                                    "drop replication "QCM_SQL_STRING_SKIP_FMT"",
                                    sSecondBackupReplicationName) != IDE_SUCCESS );
        IDE_TEST( executeRemoteSQL( aStatement,
                                    sToReplicaSetPtr->mSecondBackupNodeName,
                                    SDI_INTERNAL_OP_NORMAL,
                                    QCI_STMT_MASK_DDL,
                                    sSqlStr,
                                    QD_MAX_SQL_LENGTH,
                                    "drop replication "QCM_SQL_STRING_SKIP_FMT"",
                                    sSecondBackupReplicationName) != IDE_SUCCESS );
    }
    
    IDE_TEST( resetShardMetaAndSwapPartition( aStatement,
                                              sSqlStr,
                                              QD_MAX_SQL_LENGTH,
                                              sFromNodeName,
                                              sToNodeName,
                                              sParseTree->mNodeInfoList,
                                              sParseTree->mReShardAttr) != IDE_SUCCESS );
    /* BUG-48586 */
    IDE_TEST( executeRemoteSQL( aStatement,
                                sToNodeName,
                                SDI_INTERNAL_OP_NORMAL,
                                QCI_STMT_MASK_SP,
                                sSqlStr,
                                QD_MAX_SQL_LENGTH,
                                "exec dbms_shard.set_ReplicaSCN()") != IDE_SUCCESS );

    /* Dealloc에서 Alloc 하는 경우 garbage Table을 Truncate 해야 한다. */
    if ( sIsDealloc == ID_TRUE )
    {
        /* ToNoNode의 LOCAL/FirstBackup/SecondBackup의 Table Data도 Truncate 대상이다. */
        if ( idlOS::strncmp( sToNodeName,
                             SDM_NA_STR,
                             SDI_NODE_NAME_MAX_SIZE + 1 ) != 0 )
        {
            IDE_TEST( truncateAllPrimaryData( aStatement,
                                              sSqlStr,
                                              QD_MAX_SQL_LENGTH,
                                              sToNodeName,
                                              sParseTree->mReShardAttr,
                                              ZK_PENDING_JOB_NONE ) != IDE_SUCCESS );
        }

        if ( idlOS::strncmp( sToReplicaSetPtr->mFirstBackupNodeName,
                             SDM_NA_STR,
                             SDI_NODE_NAME_MAX_SIZE + 1 ) != 0 )
        {
            IDE_TEST( truncateAllPrimaryData( aStatement,
                                              sSqlStr,
                                              QD_MAX_SQL_LENGTH,
                                              sToReplicaSetPtr->mFirstBackupNodeName,
                                              sParseTree->mReShardAttr,
                                              ZK_PENDING_JOB_NONE ) != IDE_SUCCESS );
        }

        if ( idlOS::strncmp( sToReplicaSetPtr->mSecondBackupNodeName,
                             SDM_NA_STR,
                             SDI_NODE_NAME_MAX_SIZE + 1 ) != 0 )
        {
            IDE_TEST( truncateAllPrimaryData( aStatement,
                                              sSqlStr,
                                              QD_MAX_SQL_LENGTH,
                                              sToReplicaSetPtr->mSecondBackupNodeName,
                                              sParseTree->mReShardAttr,
                                              ZK_PENDING_JOB_NONE ) != IDE_SUCCESS );
        }

        /* ToNode의 LOCAL/FirstBackup/SecondBackup의 BAK Table 모두 Truncate 대상이다.
         * LOCAL는 DEALLOC이 아닐때에도 하고 있으므로 여기서 안함. */
        if ( idlOS::strncmp( sToReplicaSetPtr->mFirstBackupNodeName,
                             SDM_NA_STR,
                             SDI_NODE_NAME_MAX_SIZE + 1 ) != 0 )
        {
            IDE_TEST( truncateAllBackupData( aStatement,
                                             sSqlStr,
                                             QD_MAX_SQL_LENGTH,
                                             sToReplicaSetPtr->mFirstBackupNodeName,
                                             sParseTree->mReShardAttr,
                                             ZK_PENDING_JOB_NONE ) != IDE_SUCCESS );
        }

        if ( idlOS::strncmp( sToReplicaSetPtr->mSecondBackupNodeName,
                             SDM_NA_STR,
                             SDI_NODE_NAME_MAX_SIZE + 1 ) != 0 )
        {
            IDE_TEST( truncateAllBackupData( aStatement,
                                             sSqlStr,
                                             QD_MAX_SQL_LENGTH,
                                             sToReplicaSetPtr->mSecondBackupNodeName,
                                             sParseTree->mReShardAttr,
                                             ZK_PENDING_JOB_NONE ) != IDE_SUCCESS );
        }
    }

    /* replicaset move
     * logical move: the replicaset of resharding item move to replicaset of tonode through updating replicaset id: it was updated when reset_shard_partition(resident)_node execute
     * physical move: replication drop and add
     */
    if ( (sLocalMetaInfo.mKSafety > 0) && (sParseTree->mDDLType != SHARD_MOVE_TO_INTERNAL_CLONE_COPY) )
    {
        IDE_TEST( moveReplication( aStatement,
                                   sSqlStr,
                                   QD_MAX_SQL_LENGTH,
                                   sLocalMetaInfo.mKSafety,
                                   sFromReplicaSetPtr,
                                   sToReplicaSetPtr,
                                   sParseTree->mReShardAttr ) != IDE_SUCCESS );
    }

    /* truncate garbage backup table  */
    if ( sIsFromNodeTruncate == ID_TRUE )
    {
        IDE_TEST( truncateAllBackupData( aStatement,
                                         sSqlStr,
                                         QD_MAX_SQL_LENGTH,
                                         sFromNodeName,
                                         sParseTree->mReShardAttr,
                                         ZK_PENDING_JOB_NONE ) != IDE_SUCCESS );
    }

    if ( sIsFirstBackupOfFromNodeTruncate == ID_TRUE )
    {
        IDE_TEST( truncateAllBackupData( aStatement,
                                         sSqlStr,
                                         QD_MAX_SQL_LENGTH,
                                         sFromReplicaSetPtr->mFirstBackupNodeName,
                                         sParseTree->mReShardAttr,
                                         ZK_PENDING_JOB_NONE ) != IDE_SUCCESS );
    }

    if ( sIsSecondBackupOfFromNodeTruncate == ID_TRUE )
    {
        IDE_TEST( truncateAllBackupData( aStatement,
                                         sSqlStr,
                                         QD_MAX_SQL_LENGTH,
                                         sFromReplicaSetPtr->mSecondBackupNodeName,
                                         sParseTree->mReShardAttr,
                                         ZK_PENDING_JOB_NONE ) != IDE_SUCCESS );
    }

    IDE_TEST( truncateAllBackupData( aStatement,
                                     sSqlStr,
                                     QD_MAX_SQL_LENGTH,
                                     sToNodeName,
                                     sParseTree->mReShardAttr,
                                     ZK_PENDING_JOB_NONE ) != IDE_SUCCESS );

    if ( sdi::isShardDDLForAddClone( aStatement ) != ID_TRUE )
    {                
        sdiZookeeper::removePendingJob();
    }
            
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_EXIST_NODE );
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_NODE_NOT_EXIST ) );
    }
    IDE_EXCEPTION( ERR_SAME_NODE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_SAME_NODE_NAME, sToNodeName, sFromNodeName ) );
    }
    IDE_EXCEPTION( ERR_SHARD_META );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_TOO_MANY_REPLICA_SETS ) );
    }
    IDE_EXCEPTION( ERR_NOT_FOUND_NODE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_INVALID_NODE_NAME, sToNodeName ) );
    }
    IDE_EXCEPTION( ERR_INVALID_CONDITION );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,"qdsd::executeShardMove", "Invalid condition." ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    /* clear temporary replication for sync */
    /* job for truncate garbage backup table add to zookeeper pending job*/
    if ( sIsPrimaryDataSync == ID_TRUE )
    {
         (void)truncateAllBackupData( aStatement,
                                      sSqlStr,
                                      QD_MAX_SQL_LENGTH,
                                      sToNodeName,
                                      sParseTree->mReShardAttr,
                                      ZK_PENDING_JOB_AFTER_ROLLBACK );
    }

    if ( sIsFirstBackupDataSync == ID_TRUE )
    {
        (void)truncateAllBackupData( aStatement,
                                     sSqlStr,
                                     QD_MAX_SQL_LENGTH,
                                     sToReplicaSetPtr->mFirstBackupNodeName,
                                     sParseTree->mReShardAttr,
                                     ZK_PENDING_JOB_AFTER_ROLLBACK );
    }

    if ( sIsSecondBackupDataSync == ID_TRUE )
    {
        (void)truncateAllBackupData( aStatement,
                                     sSqlStr,
                                     QD_MAX_SQL_LENGTH,
                                     sToReplicaSetPtr->mSecondBackupNodeName,
                                     sParseTree->mReShardAttr,
                                     ZK_PENDING_JOB_AFTER_ROLLBACK );
    }
    IDE_POP();
    return IDE_FAILURE;
}
/**
 * aSmiStmt(IN): executing smistatement of transaction using for shard meta upate
 * aOutNewSMN(OUT): return updated SMN value
 */
IDE_RC qdsd::getIncreasedSMN( qcStatement * aStatement, ULong * aOutNewSMN )
{
    idBool sIsSmiStmtEnd = ID_FALSE;
    smiStatement * sSmiStmt = QC_SMI_STMT(aStatement);
    smiStatement * spRootStmt = sSmiStmt->getTrans()->getStatement();
    UInt sSmiStmtFlag = sSmiStmt->mFlag;

    IDE_TEST( sSmiStmt->end(SMI_STATEMENT_RESULT_SUCCESS) != IDE_SUCCESS );
    sIsSmiStmtEnd = ID_TRUE;

    IDE_TEST( sdi::getIncreasedSMNForMetaNode( sSmiStmt->getTrans(), aOutNewSMN ) != IDE_SUCCESS );

    IDE_TEST( sSmiStmt->begin( aStatement->mStatistics,
                               spRootStmt,
                               sSmiStmtFlag )
              != IDE_SUCCESS );
    sIsSmiStmtEnd = ID_FALSE;

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;

    if( sIsSmiStmtEnd == ID_TRUE )
    {
        IDE_ASSERT( sSmiStmt->begin( aStatement->mStatistics,
                                    spRootStmt,
                                    SMI_STATEMENT_NORMAL|SMI_STATEMENT_ALL_CURSOR ) 
                    == IDE_SUCCESS);
    }

    return IDE_FAILURE;
}

IDE_RC qdsd::validateCommon( qcStatement * aStatement )
{
    sdiLocalMetaInfo sLocalMetaInfo;
    UInt             sPropertyValue = 0;
    SInt             sPropertyStrIdx = 0;

    const SChar    * sReplPropertyStr[] = { "REPLICATION_DDL_ENABLE",
                                            "REPLICATION_DDL_ENABLE_LEVEL",
                                            "REPLICATION_SQL_APPLY_ENABLE",
                                            "REPLICATION_ALLOW_DUPLICATE_HOSTS",
                                            NULL };
    IDE_CLEAR();

    IDE_TEST( sdi::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );
    
    // check OS
#ifndef ALTI_CFG_OS_LINUX
    IDE_RAISE( ERR_ONLY_ACCEPT_LINUX );
#endif   
    // check SYS
    IDE_TEST_RAISE( QCG_GET_SESSION_USER_ID(aStatement) != QC_SYS_USER_ID,
                    ERR_NO_ALTER_DATABASE_SHARD );

    // check SHARD_ENABLE = 1
    IDE_TEST_RAISE( SDU_SHARD_ENABLE != 1, ERR_SHARD_ENABLE );

    // check SHARD META
    IDE_TEST_RAISE( sdi::checkMetaCreated() != ID_TRUE, ERR_SYS_SHARD_META );
    
    // check AUTOCOMMIT OFF
    // R2HA 추후 default autocommit off로 변경 되면 제거 해라
    IDE_TEST_RAISE( QCG_GET_SESSION_IS_AUTOCOMMIT( aStatement ) == ID_TRUE,
                    ERR_AUTOCOMMIT_MDOE );

    if ( sLocalMetaInfo.mKSafety != 0 )
    {
        for( sPropertyStrIdx = 0; sReplPropertyStr[sPropertyStrIdx] != NULL; sPropertyStrIdx++ )
        {
            /* read 4byte property */
            IDE_TEST(idp::read(sReplPropertyStr[sPropertyStrIdx], &sPropertyValue) != IDE_SUCCESS);
            IDE_TEST_RAISE( sPropertyValue != 1, ERR_REPL_PROPERTY );
        }
    }
    
    // set SHARD_DDL session flag
    aStatement->session->mQPSpecific.mFlag &= ~QC_SESSION_SHARD_DDL_MASK;
    aStatement->session->mQPSpecific.mFlag |= QC_SESSION_SHARD_DDL_TRUE;

    return IDE_SUCCESS;
    
#ifndef ALTI_CFG_OS_LINUX
    IDE_EXCEPTION( ERR_ONLY_ACCEPT_LINUX );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_ZKC_NOT_SUPPORT_OS ) );
    }
#endif
    IDE_EXCEPTION( ERR_NO_ALTER_DATABASE_SHARD );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_ALTER_DATABASE_SHARD ) );
    }    
    IDE_EXCEPTION( ERR_AUTOCOMMIT_MDOE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_INSUFFICIENT_ATTRIBUTE,
                                "AUTOCOMMIT = FALSE" ) );
    }
    IDE_EXCEPTION( ERR_SHARD_ENABLE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_INSUFFICIENT_ATTRIBUTE,
                                "SHARD_ENABLE = 1" ) );
    }
    IDE_EXCEPTION( ERR_SYS_SHARD_META );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_SHARD_META_NOT_CREATED ) );
    }
    IDE_EXCEPTION( ERR_REPL_PROPERTY );
    {
        IDE_SET(ideSetErrorCode( qpERR_ABORT_QDSD_INVALID_PROPERTY_FOR_SHARDING, sReplPropertyStr[sPropertyStrIdx] ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdsd::validateCommonForExecute( qcStatement * aStatement )
{
    // check 되는 프로퍼티는 shard ddl에서 internal로 변경 된다.
    // 변경 시점이 execute 수행 전에 변경 되므로 validate check를 execute에서 함
    
    // check GLOBAL_TRANSACTION_LEVEL = 2
    IDE_TEST_RAISE( QCG_GET_SESSION_GTX_LEVEL( aStatement ) < 2,
                    ERR_GLOBAL_TRANSACTION_LEVEL );

    // check DDL TRANSACTION = 1
    IDE_TEST_RAISE ( QCG_GET_SESSION_TRANSACTIONAL_DDL( aStatement) != ID_TRUE,
                     ERR_DDL_TRANSACTION );

    aStatement->session->mQPSpecific.mFlag &= ~QC_SESSION_SHARD_DDL_MASK;
    aStatement->session->mQPSpecific.mFlag |= QC_SESSION_SHARD_DDL_TRUE;

    /* BUG-48290 */
    IDE_TEST( qcg::setSessionIsInternalLocalOperation( aStatement, SDI_INTERNAL_OP_NORMAL) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_GLOBAL_TRANSACTION_LEVEL );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_INSUFFICIENT_ATTRIBUTE,
                                "GLOBAL_TRANSACTION_LEVEL = 2" ) );
    }
    IDE_EXCEPTION( ERR_DDL_TRANSACTION );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_INSUFFICIENT_ATTRIBUTE,
                                 "TRANSACTIONAL_DDL = 1" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdsd::executeSetNodes( qcStatement * aStatement )
{
    qdShardParseTree  * sParseTree = NULL;
    sdiLocalMetaInfo    sLocalMetaInfo;
    SChar               sSqlStr[QD_MAX_SQL_LENGTH + 1]        = {0,};
    sdiClientInfo     * sClientInfo  = NULL;
    sdiConnectInfo    * sConnectInfo = NULL;
    UShort              i            = 0;

    sParseTree = (qdShardParseTree *)aStatement->myPlan->parseTree;

    // LOCAL_META_INFO
    IDE_TEST( sdi::getLocalMetaInfo( &sLocalMetaInfo ) 
              != IDE_SUCCESS );

    IDE_TEST( executeLocalSQL( aStatement, 
                               QCI_STMT_MASK_SP,
                               sSqlStr,
                               QD_MAX_SQL_LENGTH,
                               "EXEC DBMS_SHARD.SET_NODE( '"QCM_SQL_STRING_SKIP_FMT"', VARCHAR'%s', INTEGER'%"ID_INT32_FMT"',VARCHAR'%s', INTEGER'%"ID_INT32_FMT"', INTEGER'%"ID_INT32_FMT"', INTEGER'%"ID_INT32_FMT"' ); ",
                               sLocalMetaInfo.mNodeName,
                               sLocalMetaInfo.mHostIP,
                               sLocalMetaInfo.mPortNo,
                               sLocalMetaInfo.mInternalHostIP,
                               sLocalMetaInfo.mInternalPortNo,
                               sLocalMetaInfo.mInternalConnType,
                               sLocalMetaInfo.mShardNodeId )
              != IDE_SUCCESS );

    /* it don't need remote execute when first add node */
    if ( sParseTree->mNodeCount > 1 )
    {
        IDE_TEST( sdi::checkShardLinker( aStatement ) != IDE_SUCCESS );

        sClientInfo = aStatement->session->mQPSpecific.mClientInfo;

        sConnectInfo = sClientInfo->mConnectInfo;

        for ( i = 0; i < sClientInfo->mCount; i++, sConnectInfo++ )
        {
            if ( idlOS::strMatch( sLocalMetaInfo.mNodeName,
                                  idlOS::strlen( sLocalMetaInfo.mNodeName ),
                                  sConnectInfo->mNodeName,
                                  idlOS::strlen( sConnectInfo->mNodeName )) != 0 )
            {
                IDE_DASSERT ( QC_IS_NULL_NAME( sParseTree->mNodeName ) == ID_TRUE );
                IDE_TEST( executeRemoteSQL( aStatement, 
                                            sConnectInfo->mNodeName,
                                            SDI_INTERNAL_OP_NORMAL,
                                            QCI_STMT_MASK_SP,
                                            sSqlStr,
                                            QD_MAX_SQL_LENGTH,
                               "EXEC DBMS_SHARD.SET_NODE( '"QCM_SQL_STRING_SKIP_FMT"', VARCHAR'%s', INTEGER'%"ID_INT32_FMT"',VARCHAR'%s', INTEGER'%"ID_INT32_FMT"', INTEGER'%"ID_INT32_FMT"', INTEGER'%"ID_INT32_FMT"' ); ",
                                            sLocalMetaInfo.mNodeName,
                                            sLocalMetaInfo.mHostIP,
                                            sLocalMetaInfo.mPortNo,
                                            sLocalMetaInfo.mInternalHostIP,
                                            sLocalMetaInfo.mInternalPortNo,
                                            sLocalMetaInfo.mInternalConnType,
                                            sLocalMetaInfo.mShardNodeId )
                            != IDE_SUCCESS);
            }
            else
            {
                    // nothing to do
            }
        }
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdsd::executeUnsetNodes( qcStatement * aStatement )
{
    qdShardParseTree  * sParseTree = NULL;
    sdiLocalMetaInfo    sLocalMetaInfo;
    SChar               sSqlStr[QD_MAX_SQL_LENGTH + 1]        = {0,};
    sdiClientInfo     * sClientInfo  = NULL;
    sdiConnectInfo    * sConnectInfo = NULL;
    UShort              i            = 0;
    
    sParseTree = (qdShardParseTree *)aStatement->myPlan->parseTree;
    
    // LOCAL_META_INFO
    IDE_TEST( sdi::getLocalMetaInfo( &sLocalMetaInfo ) 
              != IDE_SUCCESS );

    IDE_TEST_RAISE ( sParseTree->mNodeCount < 1 , ERR_INVALID_CONDITION );
    IDE_TEST( sdi::checkShardLinker( aStatement ) != IDE_SUCCESS );

    IDE_TEST( executeLocalSQL( aStatement, 
                               QCI_STMT_MASK_SP,
                               sSqlStr,
                               QD_MAX_SQL_LENGTH,
                               "EXEC DBMS_SHARD.UNSET_NODE( '"QCM_SQL_STRING_SKIP_FMT"' ); ",
                               sLocalMetaInfo.mNodeName )
              != IDE_SUCCESS );

    sClientInfo = aStatement->session->mQPSpecific.mClientInfo;
    sConnectInfo = sClientInfo->mConnectInfo;

    for ( i = 0; i < sClientInfo->mCount; i++, sConnectInfo++ )
    {
        if ( idlOS::strMatch( sLocalMetaInfo.mNodeName,
                              idlOS::strlen( sLocalMetaInfo.mNodeName ),
                              sConnectInfo->mNodeName,
                              idlOS::strlen( sConnectInfo->mNodeName )) != 0 )
        {
            IDE_TEST( executeRemoteSQL( aStatement, 
                                        sConnectInfo->mNodeName,
                                        SDI_INTERNAL_OP_NORMAL,
                                        QCI_STMT_MASK_SP,
                                        sSqlStr,
                                        QD_MAX_SQL_LENGTH,
                                        "EXEC DBMS_SHARD.UNSET_NODE( '"QCM_SQL_STRING_SKIP_FMT"' ); ",
                                        sLocalMetaInfo.mNodeName )
                        != IDE_SUCCESS);
        }
        else
        {
                    // nothing to do
        }
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_CONDITION );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,"qdsd::executeUnSetNode", "Invalid condition." ) );
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC qdsd::executeZookeeperAdd()
{
    sdiLocalMetaInfo      sLocalMetaInfo;
    idBool                sIsMetaInited = ID_FALSE;
    SInt                  sDataLength = 0;
    /* zookeeper meta 삽입용 */
    SChar                 sBuffer[SDI_ZKC_BUFFER_SIZE] = {0,};
    SChar                 sShardID[SDI_ZKC_SHARD_ID_SIZE] = {0,};
    SChar                 sPath[SDI_ZKC_PATH_LENGTH] = SDI_ZKC_PATH_ALTIBASE_SHARD;
    SChar                 sNodeIPnPort[SDI_SERVER_IP_SIZE + SDI_PORT_NUM_BUFFER_SIZE] = {0,};
    SChar                 sInternalNodeIPnPort[SDI_SERVER_IP_SIZE + SDI_PORT_NUM_BUFFER_SIZE] = {0,};
    SChar                 sInternalReplicationHostIPnPort[SDI_SERVER_IP_SIZE + SDI_PORT_NUM_BUFFER_SIZE] = {0,};
    SChar                 sConnType[10] = {0,};

    IDE_TEST( sdi::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );

    /* 클러스터 메타가 생성되어있는지 확인한다. */
    IDE_TEST( sdiZookeeper::checkPath( sPath,
                                       &sIsMetaInited ) != IDE_SUCCESS );
    /* 1. shardID */
    idlOS::snprintf( sShardID, SDI_ZKC_BUFFER_SIZE, "%"ID_INT32_FMT, sLocalMetaInfo.mShardNodeId );

    /* 2. Host IP / Port */
    idlOS::memset( sBuffer, 0, SDI_ZKC_BUFFER_SIZE );
    idlOS::snprintf( sBuffer, SDI_ZKC_BUFFER_SIZE, "%"ID_UINT32_FMT, sLocalMetaInfo.mPortNo );
    idlOS::strncpy( sNodeIPnPort,
                    sLocalMetaInfo.mHostIP,
                    SDI_SERVER_IP_SIZE + SDI_PORT_NUM_BUFFER_SIZE );
    idlOS::strncat( sNodeIPnPort,
                    ":",
                    ID_SIZEOF( sNodeIPnPort ) - idlOS::strlen( sNodeIPnPort ) - 1 );
    idlOS::strncat( sNodeIPnPort,
                    sBuffer,
                    ID_SIZEOF( sNodeIPnPort ) - idlOS::strlen( sNodeIPnPort ) - 1 );

    /* 3. Internal Host IP / Port */
    idlOS::memset( sBuffer, 0, SDI_ZKC_BUFFER_SIZE );
    idlOS::snprintf( sBuffer, SDI_ZKC_BUFFER_SIZE, "%"ID_UINT32_FMT, sLocalMetaInfo.mInternalPortNo );
    idlOS::strncpy( sInternalNodeIPnPort,
                    sLocalMetaInfo.mInternalHostIP,
                    SDI_SERVER_IP_SIZE + SDI_PORT_NUM_BUFFER_SIZE );
    idlOS::strncat( sInternalNodeIPnPort,
                    ":",
                    ID_SIZEOF( sInternalNodeIPnPort ) - idlOS::strlen( sInternalNodeIPnPort ) - 1 );
    idlOS::strncat( sInternalNodeIPnPort,
                    sBuffer,
                    ID_SIZEOF( sInternalNodeIPnPort ) - idlOS::strlen( sInternalNodeIPnPort ) - 1 );

    /* 4. Internal Replication Host IP / Port */
    idlOS::memset( sBuffer, 0, SDI_PORT_NUM_BUFFER_SIZE );
    idlOS::snprintf( sBuffer, SDI_ZKC_BUFFER_SIZE, "%"ID_UINT32_FMT, sLocalMetaInfo.mInternalRPPortNo );
    idlOS::strncpy( sInternalReplicationHostIPnPort,
                    sLocalMetaInfo.mInternalRPHostIP,
                    SDI_SERVER_IP_SIZE + SDI_PORT_NUM_BUFFER_SIZE );
    idlOS::strncat( sInternalReplicationHostIPnPort,
                    ":",
                    ID_SIZEOF( sInternalReplicationHostIPnPort ) - idlOS::strlen( sInternalReplicationHostIPnPort ) - 1 );
    idlOS::strncat( sInternalReplicationHostIPnPort,
                    sBuffer,
                    ID_SIZEOF( sInternalReplicationHostIPnPort ) - idlOS::strlen( sInternalReplicationHostIPnPort ) - 1 );

    /* 5. Internal Conn Type */
    idlOS::memset( sConnType, 0, 10 );
    idlOS::snprintf( sConnType, 10, "%"ID_UINT64_FMT, sLocalMetaInfo.mInternalConnType );
    /* zookeeper server에 노드 메타 정보를 추가한다. */
    IDE_TEST( sdiZookeeper::addNode( sShardID,
                                     sNodeIPnPort,
                                     sInternalNodeIPnPort,
                                     sInternalReplicationHostIPnPort,
                                     sConnType ) != IDE_SUCCESS );

    /* 기존 메타가 존재하고 내 메타 정보에 replication 관련 정보가 비어있다면 가져온다. */
    /* BUGBUG : 현재는 sdiLocalMetaInfo에 replication 관련 정보가 없으므로 일단 주석처리를 하지만 *
     *          차후 replication 정보가 추가되면 그에 맞춰 수정해야 한다.                         */
    /* BUGBUG : 차후 replication 정보가 비어있는 것을 어떻게 확인 할 것인지에 대한 변경이 필요하다. */
    if( sIsMetaInited == ID_TRUE )
//    if( ( sIsMetaInited == ID_TRUE ) &&
//        ( sLocalMetaInfo.mKSafety == 0 ) &&
//        ( sLocalMetaInfo.mReplicationMode == 0 ) &&
//        ( sLocalMetaInfo.mParallelCount == 0 ) )

    {
        // k-safety 값을 가져온다.
        sDataLength = SDI_ZKC_BUFFER_SIZE;
        IDE_TEST( sdiZookeeper::readInfo( (SChar*)SDI_ZKC_PATH_K_SAFETY,
                                          sBuffer,
                                          &sDataLength ) != IDE_SUCCESS );
//        sLocalMetaInfo.mKSafety = idlOS::atoi( sBuffer );

        // replication mode 값을 가져온다.
        idlOS::memset( sBuffer, 0, SDI_ZKC_BUFFER_SIZE );
        sDataLength = SDI_ZKC_BUFFER_SIZE;
        IDE_TEST( sdiZookeeper::readInfo( (SChar*)SDI_ZKC_PATH_REP_MODE,
                                          sBuffer,
                                          &sDataLength ) != IDE_SUCCESS );
//        sLocalMetaInfo.mReplicationMode = idlOS::atoi( sBuffer );

        // parallel count 값을 가져온다.
        idlOS::memset( sBuffer, 0, SDI_ZKC_BUFFER_SIZE );
        sDataLength = SDI_ZKC_BUFFER_SIZE;
        IDE_TEST( sdiZookeeper::readInfo( (SChar*)SDI_ZKC_PATH_PARALLEL_CNT,
                                          sBuffer,
                                          &sDataLength ) != IDE_SUCCESS );
//        sLocalMetaInfo.mParallelCount = idlOS::atoi( sBuffer );

        //가져온 정보를 메타에 추가해야 한다.
    }

    /* TASK-7217 Sharded sequence */
    qcm::setShardNodeID(sLocalMetaInfo.mShardNodeId);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* TASK-7217 Sharded sequence */
    qcm::unsetShardNodeID();
    
    return IDE_FAILURE;
}

IDE_RC qdsd::executeZookeeperJoin()
{
    sdiLocalMetaInfo      sLocalMetaInfo;

    IDE_TEST( sdiZookeeper::joinNode() != IDE_SUCCESS );

    IDE_TEST( sdi::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );
    /* TASK-7217 Sharded sequence */
    qcm::setShardNodeID(sLocalMetaInfo.mShardNodeId);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* TASK-7217 Sharded sequence */
    qcm::unsetShardNodeID();
    
    return IDE_FAILURE;
}

IDE_RC qdsd::executeZookeeperDrop( qcStatement * aQcStmt )
{
    SChar   sNodeName[SDI_NODE_NAME_MAX_SIZE + 1] = {0,};

    qdShardParseTree * sParseTree = (qdShardParseTree *)aQcStmt->myPlan->parseTree;

    if ( sParseTree->mDDLType == SHARD_DROP )
    {
        if( QC_IS_NULL_NAME( sParseTree->mNodeName ) == ID_TRUE )
        {
            /* 구문에 노드 이름이 없다면 내 노드를 제거한다. */
            IDE_TEST( sdiZookeeper::dropNode( sdiZookeeper::mMyNodeName, ID_FALSE ) != IDE_SUCCESS );
        }
        else
        {
            /* 구문에 노드 이름이 있다면 해당 노드를 제거한다. */
            QC_STR_COPY( sNodeName, sParseTree->mNodeName );

            IDE_TEST( sdiZookeeper::dropNode( sNodeName, ID_FALSE ) != IDE_SUCCESS );
        }

        /* drop하려고 하는 노드에 새 session이 달라붙지 못하게 막아야 한다. */
        idp::update( NULL, "SHARD_ADMIN_MODE", (ULong)1, 0, NULL );
        sdi::setShardStatus(0); // SHARD_STATUS 1 -> 0

        /* 기존에 연결되어있던 session을 정리한다. */
        IDE_TEST_RAISE( sdi::closeSessionForShardDrop( aQcStmt ) != IDE_SUCCESS, errSessionClose );

        /* unsetShardNodeID는 commit된 후에 수행되어야 하므로 sdiZookeeper::callAfterCommitFunc에서 수행한다. */
    }
    else
    {
        IDE_DASSERT( sParseTree->mDDLType == SHARD_DROP_FORCE );

        /* Drop Force 일 경우 NodeName이 지정되지 않으면 Validation에서 튕긴다. */
        QC_STR_COPY( sNodeName, sParseTree->mNodeName );

        IDE_TEST( sdiZookeeper::dropNode( sNodeName, ID_TRUE ) != IDE_SUCCESS );

        /* Drop Force는 이미 Failover가 수행된 Node를 대상으로만 수행하기 때문에
         * Local Session 정리 하면 안된다. */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( errSessionClose )
    {
        sdi::setShardStatus(1); // SHARD_STATUS 0 -> 1 
        idp::update( NULL, "SHARD_ADMIN_MODE", (ULong)0, 0, NULL );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* validate 에서 이미 Z-Meta/S-Meta Lock을 획득하였다. */
IDE_RC qdsd::executeZookeeperFailover( SChar   * aTargetNodeName,
                                       SChar   * aFailoverNodeName,
                                       idBool  * aIsTargetAlive )
{
    SChar     sPath[SDI_ZKC_PATH_LENGTH] = {0,};

    /* failover 구문의 대상이 된 노드가 살아 있다면 해당 노드를 내리고
     * failover를 수행해야 하므로 해당 노드가 살아있는지 확인한다. */
    idlOS::strncpy( sPath,
                    SDI_ZKC_PATH_CONNECTION_INFO,
                    SDI_ZKC_PATH_LENGTH );
    idlOS::strncat( sPath,
                    "/",
                    ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );
    idlOS::strncat( sPath,
                    aTargetNodeName,
                    ID_SIZEOF( sPath ) - idlOS::strlen( sPath ) - 1 );

    /* Target이 살아 있다면 Execute에서 Stop 시켜 주어야 한다. */
    IDE_TEST( sdiZookeeper::checkPath( sPath, aIsTargetAlive ) != IDE_SUCCESS );

    /* zookeeper 메타를 변경한다. */
    IDE_TEST( sdiZookeeper::failoverForQuery( aTargetNodeName,
                                              aFailoverNodeName ) != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdsd::executeZookeeperFailback()
{
    sdiLocalMetaInfo      sLocalMetaInfo;

    /* zookeeper 메타를 변경한다. */
    IDE_TEST( sdiZookeeper::failback() != IDE_SUCCESS );

    IDE_TEST( sdi::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );
    /* TASK-7217 Sharded sequence */
    qcm::setShardNodeID(sLocalMetaInfo.mShardNodeId);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* TASK-7217 Sharded sequence */
    qcm::unsetShardNodeID();

    return IDE_FAILURE;
}

IDE_RC qdsd::getAllNodeInfoList( qcStatement * aStatement )
{
    qdShardParseTree  * sParseTree = NULL;

    sParseTree = (qdShardParseTree *)aStatement->myPlan->parseTree;

    IDE_TEST( sdiZookeeper::getAllNodeInfoList( &sParseTree->mNodeInfoList,
                                                &sParseTree->mNodeCount,
                                                QC_QMX_MEM(aStatement) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC qdsd::executeTemporaryReplication( qcStatement * aStatement )
{
    qdShardParseTree  * sParseTree = NULL;
    sdiLocalMetaInfo  * sRemoteLocalMetaInfo = NULL;
    sdiLocalMetaInfo    sCurrLocalMetaInfo;
    SChar               sSqlStr[QD_MAX_SQL_LENGTH];
    iduListNode       * sNode  = NULL;
    iduListNode       * sDummy = NULL;

    sParseTree = (qdShardParseTree *)aStatement->myPlan->parseTree;
    
    IDE_TEST( sdi::getLocalMetaInfo( &sCurrLocalMetaInfo ) != IDE_SUCCESS );
    
    if ( sParseTree->mNodeCount > 1 )
    {
        //원격 노드의 node info
        if ( IDU_LIST_IS_EMPTY( sParseTree->mNodeInfoList ) != ID_TRUE )
        {
            IDU_LIST_ITERATE_SAFE( sParseTree->mNodeInfoList, sNode, sDummy )
            {
                sRemoteLocalMetaInfo = (sdiLocalMetaInfo *)sNode->mObj;
            
                if( idlOS::strncmp( sRemoteLocalMetaInfo->mNodeName,
                                    sCurrLocalMetaInfo.mNodeName,
                                    SDI_NODE_NAME_MAX_SIZE ) != 0 )
                {
                    break;
                }
            }
        }

        IDE_TEST_RAISE( sRemoteLocalMetaInfo == NULL, ERR_NOT_EXIST_LOCAL_META_INFO );
        
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "DELETE FROM SYS_SHARD.GLOBAL_META_INFO_ " );

        IDE_TEST( sdi::shardExecTempDMLOrDCLWithNewTrans( aStatement,
                                                          sSqlStr )
                  != IDE_SUCCESS );

        if ( sCurrLocalMetaInfo.mInternalConnType == 8 ) // IB
        {
            idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                             "ALTER REPLICATION TEMPORARY GET TABLE "
                             "SYS_SHARD.NODES_, SYS_SHARD.OBJECTS_, SYS_SHARD.REPLICA_SETS_, "
                             "SYS_SHARD.SOLOS_, SYS_SHARD.CLONES_, SYS_SHARD.GLOBAL_META_INFO_, "
                             "SYS_SHARD.RANGES_,SYS_SHARD.FAILOVER_HISTORY_ "
                             "FROM '%s',%"ID_INT32_FMT" USING IB",
                             sRemoteLocalMetaInfo->mInternalRPHostIP, // internal_host_ip
                             sRemoteLocalMetaInfo->mInternalRPPortNo ); // internal_port_no
        }
        else
        {
            idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                             "ALTER REPLICATION TEMPORARY GET TABLE "
                             "SYS_SHARD.NODES_, SYS_SHARD.OBJECTS_, SYS_SHARD.REPLICA_SETS_, "
                             "SYS_SHARD.SOLOS_, SYS_SHARD.CLONES_, SYS_SHARD.GLOBAL_META_INFO_, "
                             "SYS_SHARD.RANGES_,SYS_SHARD.FAILOVER_HISTORY_ "
                             "FROM '%s',%"ID_INT32_FMT" ",
                             sRemoteLocalMetaInfo->mInternalRPHostIP, // internal_host_ip
                             sRemoteLocalMetaInfo->mInternalRPPortNo ); // internal_port_no
        }

        IDE_TEST( sdi::shardExecTempDMLOrDCLWithNewTrans( aStatement,
                                                          sSqlStr )
                  != IDE_SUCCESS );

        // 원격 노드로 부터 얻어온 SNM과 동기화
        IDE_TEST( sdi::reloadSMNForDataNode( QC_SMI_STMT( aStatement ) )
                  != IDE_SUCCESS );
        
        // session SMN
        qci::mSessionCallback.mSetShardMetaNumber( aStatement->session->mMmSession,
                                                   sdi::getSMNForDataNode());
        
        idlOS::snprintf( sSqlStr,
                         QD_MAX_SQL_LENGTH,
                         "TRUNCATE TABLE SYS_SHARD.OBJECTS_" );
        
        (void)sdiZookeeper::addPendingJob( sSqlStr,
                                           sCurrLocalMetaInfo.mNodeName,
                                           ZK_PENDING_JOB_AFTER_ROLLBACK,
                                           QCI_STMT_MASK_DDL );

        idlOS::snprintf( sSqlStr,
                         QD_MAX_SQL_LENGTH,
                         "TRUNCATE TABLE SYS_SHARD.REPLICA_SETS_" );
        
        (void)sdiZookeeper::addPendingJob( sSqlStr,
                                           sCurrLocalMetaInfo.mNodeName,
                                           ZK_PENDING_JOB_AFTER_ROLLBACK,
                                           QCI_STMT_MASK_DDL );

        idlOS::snprintf( sSqlStr,
                         QD_MAX_SQL_LENGTH,
                         "TRUNCATE TABLE SYS_SHARD.SOLOS_" );
        
        (void)sdiZookeeper::addPendingJob( sSqlStr,
                                           sCurrLocalMetaInfo.mNodeName,
                                           ZK_PENDING_JOB_AFTER_ROLLBACK,
                                           QCI_STMT_MASK_DDL );

        idlOS::snprintf( sSqlStr,
                         QD_MAX_SQL_LENGTH,
                         "TRUNCATE TABLE SYS_SHARD.CLONES_" );
        
        (void)sdiZookeeper::addPendingJob( sSqlStr,
                                           sCurrLocalMetaInfo.mNodeName,
                                           ZK_PENDING_JOB_AFTER_ROLLBACK,
                                           QCI_STMT_MASK_DDL );
        
        idlOS::snprintf( sSqlStr,
                         QD_MAX_SQL_LENGTH,
                         "TRUNCATE TABLE SYS_SHARD.RANGES_" );
        
        (void)sdiZookeeper::addPendingJob( sSqlStr,
                                           sCurrLocalMetaInfo.mNodeName,
                                           ZK_PENDING_JOB_AFTER_ROLLBACK,
                                           QCI_STMT_MASK_DDL );

        idlOS::snprintf( sSqlStr,
                         QD_MAX_SQL_LENGTH,
                         "TRUNCATE TABLE SYS_SHARD.FAILOVER_HISTORY_" );
        
        (void)sdiZookeeper::addPendingJob( sSqlStr,
                                           sCurrLocalMetaInfo.mNodeName,
                                           ZK_PENDING_JOB_AFTER_ROLLBACK,
                                           QCI_STMT_MASK_DDL );

        // GLOBAL_META_INFO_ 는 초기 값으로 update.
        idlOS::snprintf( sSqlStr,
                         QD_MAX_SQL_LENGTH,
                         "UPDATE SYS_SHARD.GLOBAL_META_INFO_ SET ID = 0, SHARD_META_NUMBER = 1" );
        
        (void)sdiZookeeper::addPendingJob( sSqlStr,
                                           sCurrLocalMetaInfo.mNodeName,
                                           ZK_PENDING_JOB_AFTER_ROLLBACK,
                                           QCI_STMT_MASK_DML );

        // addpending에서  SYS_SHARD.NODES_ 는 제거 하지 않는다. (pending job에 등록하지 않음)
        // 해당 메타 제거 시 원격 노드에 대한 executependingjob을 수행 할 수 없다.
        // cf> 해당 정보는 shard add시에 제거 된다.
    }
    else
    {
        // nothing to do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_EXIST_LOCAL_META_INFO )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "qdsd::executeTemporaryReplication",
                                  "Meta information of remote node does not exist in cluster meta." ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC qdsd::executeReplicationFunction( qcStatement       * aStatement,
                                         qdsdReplQueryType   aReplQueryType )
{
    qdShardParseTree  * sParseTree = NULL;
    sdiLocalMetaInfo    sLocalMetaInfo;
    sdiReplicaSetInfo   sReplicaSetInfo;
    sdiReplicaSetInfo   sOldReplicaSetInfo;
    sdiReplicaSetInfo   sCurrReplicaSetInfo;
    sdiReplicaSetInfo   sPrevReplicaSetInfo;
    sdiReplicaSetInfo   sPrevPrevReplicaSetInfo;
    sdiReplicaSetInfo   sOldCurrReplicaSetInfo;
    sdiReplicaSetInfo   sOldPrevReplicaSetInfo;
    sdiReplicaSetInfo   sOldPrevPrevReplicaSetInfo;
    sdiGlobalMetaInfo   sMetaNodeInfo = { ID_ULONG(0) };
    ULong               sSMN = ID_ULONG(0);

    sParseTree = (qdShardParseTree *)aStatement->myPlan->parseTree;

    SDI_INIT_REPLICA_SET( sReplicaSetInfo.mReplicaSets );
    SDI_INIT_REPLICA_SET( sOldReplicaSetInfo.mReplicaSets );
    SDI_INIT_REPLICA_SET( sCurrReplicaSetInfo.mReplicaSets );
    SDI_INIT_REPLICA_SET( sPrevReplicaSetInfo.mReplicaSets );
    SDI_INIT_REPLICA_SET( sPrevPrevReplicaSetInfo.mReplicaSets );
    SDI_INIT_REPLICA_SET( sOldCurrReplicaSetInfo.mReplicaSets );
    SDI_INIT_REPLICA_SET( sOldPrevReplicaSetInfo.mReplicaSets );
    SDI_INIT_REPLICA_SET( sOldPrevPrevReplicaSetInfo.mReplicaSets );
    
    IDE_TEST( sdi::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );    

    IDE_TEST( sdi::getGlobalMetaInfoCore( QC_SMI_STMT( aStatement ),
                                          &sMetaNodeInfo )
              != IDE_SUCCESS );
    
    if ( sParseTree->mDDLType == SHARD_ADD )
    {
        // current SMN
        sSMN = sMetaNodeInfo.mShardMetaNumber;
    
        IDE_TEST( sdi::getAllReplicaSetInfoSortedByPName( QC_SMI_STMT( aStatement ),
                                                          &sReplicaSetInfo,
                                                          sSMN )
                  != IDE_SUCCESS );

        IDE_TEST( findBackupReplicaSetInfo( sLocalMetaInfo.mNodeName,
                                            &sReplicaSetInfo,
                                            &sCurrReplicaSetInfo,
                                            &sPrevReplicaSetInfo,
                                            &sPrevPrevReplicaSetInfo )
                  != IDE_SUCCESS );


        switch( aReplQueryType )
        {
            case QDSD_REPL_QUERY_TYPE_STOP:
            case QDSD_REPL_QUERY_TYPE_DROP:
                
                // DROP OLD 기준이 된다.
                // old SMN, REPLICA_SETS_ 갱신되기 전의 DATA SMN으로 GET
                sSMN = sdi::getSMNForDataNode();

                IDE_TEST( sdi::getAllReplicaSetInfoSortedByPName( QC_SMI_STMT( aStatement ),
                                                                  &sOldReplicaSetInfo,
                                                                  sSMN )
                          != IDE_SUCCESS );

                if ( sOldReplicaSetInfo.mCount != 0 )
                {
                    IDE_TEST( findBackupReplicaSetInfo( sPrevReplicaSetInfo.mReplicaSets[0].mPrimaryNodeName,
                                                        &sOldReplicaSetInfo,
                                                        &sOldCurrReplicaSetInfo,
                                                        &sOldPrevReplicaSetInfo,
                                                        &sOldPrevPrevReplicaSetInfo )
                              != IDE_SUCCESS );

                    IDE_TEST( runForNodeReplicationQuery( aStatement,
                                                          &sOldReplicaSetInfo,
                                                          &sOldCurrReplicaSetInfo,
                                                          &sOldPrevReplicaSetInfo,
                                                          &sOldPrevPrevReplicaSetInfo,
                                                          aReplQueryType,
                                                          ID_TRUE )
                              != IDE_SUCCESS );
                }
                break;
            case QDSD_REPL_QUERY_TYPE_CREATE:
            case QDSD_REPL_QUERY_TYPE_ADD:
            case QDSD_REPL_QUERY_TYPE_START:
            case QDSD_REPL_QUERY_TYPE_FLUSH:
                if ( sReplicaSetInfo.mCount != 0 )
                {
                    IDE_TEST( runForNodeReplicationQuery( aStatement,
                                                          &sReplicaSetInfo,
                                                          &sCurrReplicaSetInfo,
                                                          &sPrevReplicaSetInfo,
                                                          &sPrevPrevReplicaSetInfo,
                                                          aReplQueryType,
                                                          ID_FALSE )
                              != IDE_SUCCESS );
                }
                break;
            default:
            IDE_DASSERT(0);
            break;
        }
    }
    else
    {
        // DROP OLD 기준이 된다.
        // old SMN, REPLICA_SETS_ 갱신되기 전의 DATA SMN으로 GET
        sSMN = sdi::getSMNForDataNode();
        
        IDE_TEST( sdi::getAllReplicaSetInfoSortedByPName( QC_SMI_STMT( aStatement ),
                                                          &sOldReplicaSetInfo,
                                                          sSMN )
                  != IDE_SUCCESS );

        IDE_TEST( findBackupReplicaSetInfo( sLocalMetaInfo.mNodeName,
                                            &sOldReplicaSetInfo,
                                            &sOldCurrReplicaSetInfo,
                                            &sOldPrevReplicaSetInfo,
                                            &sOldPrevPrevReplicaSetInfo )
                  != IDE_SUCCESS );

        switch( aReplQueryType )
        {
            case QDSD_REPL_QUERY_TYPE_STOP:
            case QDSD_REPL_QUERY_TYPE_DROP:
                if ( sOldReplicaSetInfo.mCount != 0 )
                {
                    IDE_TEST( runForNodeReplicationQuery( aStatement,
                                                          &sOldReplicaSetInfo,
                                                          &sOldCurrReplicaSetInfo,
                                                          &sOldPrevReplicaSetInfo,
                                                          &sOldPrevPrevReplicaSetInfo,
                                                          aReplQueryType,
                                                          ID_FALSE )
                              != IDE_SUCCESS );
                }
                break;
            case QDSD_REPL_QUERY_TYPE_CREATE:
            case QDSD_REPL_QUERY_TYPE_ADD:
            case QDSD_REPL_QUERY_TYPE_START:
            case QDSD_REPL_QUERY_TYPE_FLUSH:
                // current SMN
                sSMN = sMetaNodeInfo.mShardMetaNumber;
            
                IDE_TEST( sdi::getAllReplicaSetInfoSortedByPName( QC_SMI_STMT( aStatement ),
                                                                  &sReplicaSetInfo,
                                                                  sSMN )
                          != IDE_SUCCESS );

                if ( sReplicaSetInfo.mCount != 0 )
                {
                    IDE_TEST( findBackupReplicaSetInfo( sOldPrevReplicaSetInfo.mReplicaSets[0].mPrimaryNodeName,
                                                        &sReplicaSetInfo,
                                                        &sCurrReplicaSetInfo,
                                                        &sPrevReplicaSetInfo,
                                                        &sPrevPrevReplicaSetInfo )
                              != IDE_SUCCESS );

                    IDE_TEST( runForNodeReplicationQuery( aStatement,
                                                          &sReplicaSetInfo,
                                                          &sCurrReplicaSetInfo,
                                                          &sPrevReplicaSetInfo,
                                                          &sPrevPrevReplicaSetInfo,
                                                          aReplQueryType,
                                                          ID_TRUE )
                              != IDE_SUCCESS );
                }
                break;
            default:
            IDE_DASSERT(0);
            break;
        }    
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdsd::runForNodeReplicationQuery( qcStatement       * aStatement,
                                         sdiReplicaSetInfo * aReplicaSetInfo,
                                         sdiReplicaSetInfo * aCurrReplicaSetInfo,
                                         sdiReplicaSetInfo * aPrevReplicaSetInfo,
                                         sdiReplicaSetInfo * aPrevPrevReplicaSetInfo,
                                         qdsdReplQueryType   aReplQueryType,
                                         idBool              aIsOld )
{
    qdShardParseTree  * sParseTree = NULL;
    sdiLocalMetaInfo    sLocalMetaInfo;
    sdiReplicaSetInfo   sFoundReplicaSetInfo;
    UInt                i = 0;

    sParseTree = (qdShardParseTree *)aStatement->myPlan->parseTree;
    
    IDE_TEST( sdi::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );

    // old노드 경우는 현재 노드가 add되기 전의 replicaSetInfo 기준으로 수행
    // KSafety = 1 이면 해당사항 없음
    // KSafety = 2 이면 전 노드의 second stop/drop, old 노드의 first/second의 stop/drop
    // 현 노드의 경우
    // KSafety = 1 이면 전 노드의 first create/add/start
    // KSafety = 2 이면 전 노드, 전전 노드의 second create/add/start
    for ( i = 0; i < sLocalMetaInfo.mKSafety; i++ )
    {
        if ( aCurrReplicaSetInfo->mCount != 0 ) // 현 노드
        {
            if ( sParseTree->mDDLType == SHARD_ADD )
            {
                // add되는 노드는 SET_SHARD_RANGE를 하지 않았기 때문에 add/start replication하지 않음
                // R2HA add 되는 노드에 대해서는 추후 start replication 하도록 해야함.
                // no item replcation start는 현재 shardExecDirect()을 통해서 수행하는 경우는 지원하지 않음
                // 추후 지원시 QDSD_REPL_QUERY_TYPE_START 조건 삭제 하면됨
                if ( aReplQueryType != QDSD_REPL_QUERY_TYPE_ADD )
                {
                    IDE_TEST( sdi::findSendReplInfoFromReplicaSet(
                                  aReplicaSetInfo,
                                  aCurrReplicaSetInfo->mReplicaSets[0].mPrimaryNodeName,
                                  &sFoundReplicaSetInfo )
                              != IDE_SUCCESS );

                    IDE_TEST( searchAndRunReplicaSet( aStatement,
                                                      &sFoundReplicaSetInfo,
                                                      i,
                                                      aReplQueryType )
                              != IDE_SUCCESS );
                }
            }
            else
            {
                IDE_TEST( sdi::findSendReplInfoFromReplicaSet(
                              aReplicaSetInfo,
                              aCurrReplicaSetInfo->mReplicaSets[0].mPrimaryNodeName,
                              &sFoundReplicaSetInfo )
                          != IDE_SUCCESS );

                IDE_TEST( searchAndRunReplicaSet( aStatement,
                                                  &sFoundReplicaSetInfo,
                                                  i,
                                                  aReplQueryType )
                          != IDE_SUCCESS );
            }
        }
        
        if ( aPrevReplicaSetInfo->mCount != 0 ) // 전 노드
        {
            if ( aIsOld == ID_TRUE )
            {
                if ( i == 1 )
                {
                    IDE_TEST( sdi::findSendReplInfoFromReplicaSet(
                                  aReplicaSetInfo,
                                  aPrevReplicaSetInfo->mReplicaSets[0].mPrimaryNodeName,
                                  &sFoundReplicaSetInfo )
                              != IDE_SUCCESS );

                    IDE_TEST( searchAndRunReplicaSet( aStatement,
                                                      &sFoundReplicaSetInfo,
                                                      i,
                                                      aReplQueryType )
                              != IDE_SUCCESS );
                }
            }
            else
            {
                IDE_TEST( sdi::findSendReplInfoFromReplicaSet(
                              aReplicaSetInfo,
                              aPrevReplicaSetInfo->mReplicaSets[0].mPrimaryNodeName,
                              &sFoundReplicaSetInfo )
                          != IDE_SUCCESS );

                IDE_TEST( searchAndRunReplicaSet( aStatement,
                                                  &sFoundReplicaSetInfo,
                                                  i,
                                                  aReplQueryType )
                          != IDE_SUCCESS );
            }            
        }
        
        if ( aPrevPrevReplicaSetInfo->mCount != 0 ) // 전전 노드
        {
            if (( i == 1 ) && ( aIsOld != ID_TRUE ))
            {
                IDE_TEST( sdi::findSendReplInfoFromReplicaSet(
                              aReplicaSetInfo,
                              aPrevPrevReplicaSetInfo->mReplicaSets[0].mPrimaryNodeName,
                              &sFoundReplicaSetInfo )
                          != IDE_SUCCESS );

                IDE_TEST( searchAndRunReplicaSet( aStatement,
                                                  &sFoundReplicaSetInfo,
                                                  i,
                                                  aReplQueryType )
                          != IDE_SUCCESS );
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdsd::searchAndRunReplicaSet( qcStatement       * aStatement,
                                     sdiReplicaSetInfo * aReplicaSetInfo,
                                     UInt                aNth,
                                     qdsdReplQueryType   aReplQueryType )
{
    qdShardParseTree * sParseTree = NULL;
    sdiLocalMetaInfo * sLocalMetaInfo = NULL;
    SChar            * sTempNodeName = NULL;
    sdiBackupOrder     sBackupOrder;
    sParseTree = (qdShardParseTree *)aStatement->myPlan->parseTree;
    SChar               sSqlStr[QD_MAX_SQL_LENGTH + 1];
    
    IDE_TEST_RAISE( aReplicaSetInfo->mCount > 1, ERR_REPLICA_SET_COUNT );
    IDE_TEST_RAISE( aNth > 1, ERR_INVALID_KSAFETY );
    
    if ( aNth == 0 ) /* First */
    {
        sBackupOrder = SDI_BACKUP_1ST;
        if ( idlOS::strncmp( SDM_NA_STR,
                             aReplicaSetInfo->mReplicaSets->mFirstBackupNodeName,
                             SDI_NODE_NAME_MAX_SIZE + 1 ) != 0 )
        {
            sTempNodeName = aReplicaSetInfo->mReplicaSets->mFirstBackupNodeName;
        }
        else
        {
            sTempNodeName = aReplicaSetInfo->mReplicaSets->mStopFirstBackupNodeName;
        }                                

        /* BackupNodeName이 설정되어 있어야만 한다. */
        if ( idlOS::strncmp( sTempNodeName,
                             SDM_NA_STR,
                             SDI_NODE_NAME_MAX_SIZE + 1) != 0 )
        {
            switch( aReplQueryType )
            {
                case QDSD_REPL_QUERY_TYPE_STOP:
                    IDE_TEST( changeBuckupNodeNameForFailOver( aStatement,
                                                               aReplicaSetInfo,
                                                               aNth )
                              != IDE_SUCCESS );

                    // sender
                    IDE_TEST( stopReplicationForInternal(
                                  aStatement,
                                  aReplicaSetInfo->mReplicaSets->mFirstReplFromNodeName,
                                  aReplicaSetInfo->mReplicaSets->mFirstReplName )
                              != IDE_SUCCESS );                    
                    break;
                case QDSD_REPL_QUERY_TYPE_DROP:
                    // sender
                    IDE_TEST( dropReplicationForInternal(
                                  aStatement,
                                  aReplicaSetInfo->mReplicaSets->mFirstReplFromNodeName,
                                  aReplicaSetInfo->mReplicaSets->mFirstReplName )
                              != IDE_SUCCESS );
                    // receiver
                    IDE_TEST( dropReplicationForInternal(
                                  aStatement,
                                  aReplicaSetInfo->mReplicaSets->mFirstReplToNodeName,
                                  aReplicaSetInfo->mReplicaSets->mFirstReplName )
                              != IDE_SUCCESS );                    
                    break;
                case QDSD_REPL_QUERY_TYPE_CREATE:
                    sLocalMetaInfo =
                        sdiZookeeper::getNodeInfo( sParseTree->mNodeInfoList,
                                                   aReplicaSetInfo->mReplicaSets->mFirstReplToNodeName );
                    // sender
                    IDE_TEST( createReplicationForInternal(
                                  aStatement,
                                  aReplicaSetInfo->mReplicaSets->mFirstReplFromNodeName,
                                  aReplicaSetInfo->mReplicaSets->mFirstReplName,
                                  sLocalMetaInfo->mInternalRPHostIP,
                                  sLocalMetaInfo->mInternalRPPortNo )
                              != IDE_SUCCESS );
                    
                    idlOS::snprintf( sSqlStr,
                                     QD_MAX_SQL_LENGTH,
                                     "ALTER REPLICATION "QCM_SQL_STRING_SKIP_FMT" STOP",
                                      aReplicaSetInfo->mReplicaSets->mFirstReplName );
                    
                    (void)sdiZookeeper::addPendingJob( sSqlStr,
                                                       aReplicaSetInfo->mReplicaSets->mFirstReplFromNodeName,
                                                       ZK_PENDING_JOB_AFTER_ROLLBACK,
                                                       QCI_STMT_MASK_DCL );

                    idlOS::snprintf( sSqlStr,
                                     QD_MAX_SQL_LENGTH,
                                     "DROP REPLICATION "QCM_SQL_STRING_SKIP_FMT"",
                                     aReplicaSetInfo->mReplicaSets->mFirstReplName );

                    (void)sdiZookeeper::addPendingJob( sSqlStr,
                                                       aReplicaSetInfo->mReplicaSets->mFirstReplFromNodeName,
                                                       ZK_PENDING_JOB_AFTER_ROLLBACK,
                                                       QCI_STMT_MASK_DDL );

                    sLocalMetaInfo =
                        sdiZookeeper::getNodeInfo( sParseTree->mNodeInfoList,
                                                   aReplicaSetInfo->mReplicaSets->mFirstReplFromNodeName );
                    // receiver
                    IDE_TEST( createReplicationForInternal(
                                  aStatement,
                                  aReplicaSetInfo->mReplicaSets->mFirstReplToNodeName,
                                  aReplicaSetInfo->mReplicaSets->mFirstReplName,
                                  sLocalMetaInfo->mInternalRPHostIP,
                                  sLocalMetaInfo->mInternalRPPortNo )
                              != IDE_SUCCESS );
                    
                    idlOS::snprintf( sSqlStr,
                                     QD_MAX_SQL_LENGTH,
                                     "DROP REPLICATION "QCM_SQL_STRING_SKIP_FMT"",
                                     aReplicaSetInfo->mReplicaSets->mFirstReplName );

                    (void)sdiZookeeper::addPendingJob( sSqlStr,
                                                       aReplicaSetInfo->mReplicaSets->mFirstReplToNodeName,
                                                       ZK_PENDING_JOB_AFTER_ROLLBACK,
                                                       QCI_STMT_MASK_DDL );
                    break;
                case QDSD_REPL_QUERY_TYPE_ADD:

                    IDE_TEST( addReplicationForInternal( aStatement,
                                                         aReplicaSetInfo,
                                                         sBackupOrder )
                              != IDE_SUCCESS );
                    break;
                case QDSD_REPL_QUERY_TYPE_START:
                    // sender
                    IDE_TEST( syncReplicationForInternal(
                                  aStatement,
                                  aReplicaSetInfo->mReplicaSets->mFirstReplFromNodeName,
                                  aReplicaSetInfo->mReplicaSets->mFirstReplName )
                              != IDE_SUCCESS );
                    
                    break;
                case QDSD_REPL_QUERY_TYPE_FLUSH:
                    IDE_TEST( flushReplicationForInternal(
                                  aStatement,
                                  aReplicaSetInfo->mReplicaSets->mPrimaryNodeName,
                                  aReplicaSetInfo->mReplicaSets->mFirstReplName )
                              != IDE_SUCCESS );
                    break;                    
                default:
                    IDE_ASSERT(0);
                    break;
            }
        }
    }
    else if ( aNth == 1 ) /* Second */
    {
        sBackupOrder = SDI_BACKUP_2ND;
        if ( idlOS::strncmp( SDM_NA_STR,
                             aReplicaSetInfo->mReplicaSets->mSecondBackupNodeName,
                             SDI_NODE_NAME_MAX_SIZE + 1 ) != 0 )
        {
            sTempNodeName = aReplicaSetInfo->mReplicaSets->mSecondBackupNodeName;
        }
        else
        {
            sTempNodeName = aReplicaSetInfo->mReplicaSets->mStopSecondBackupNodeName;
        }

        /* BackupNodeName이 설정되어 있어야만 한다. */
        if ( idlOS::strncmp( sTempNodeName,
                             SDM_NA_STR,
                             SDI_NODE_NAME_MAX_SIZE + 1) != 0 )
        {
            switch( aReplQueryType )
            {
                case QDSD_REPL_QUERY_TYPE_STOP:
                    IDE_TEST( changeBuckupNodeNameForFailOver( aStatement,
                                                               aReplicaSetInfo,
                                                               aNth )
                              != IDE_SUCCESS );
                      
                    // sender
                    IDE_TEST( stopReplicationForInternal(
                                    aStatement,
                                    aReplicaSetInfo->mReplicaSets->mSecondReplFromNodeName,
                                    aReplicaSetInfo->mReplicaSets->mSecondReplName )
                                != IDE_SUCCESS );
                      break;
                case QDSD_REPL_QUERY_TYPE_DROP:
                    // sender
                    IDE_TEST( dropReplicationForInternal(
                                  aStatement,
                                  aReplicaSetInfo->mReplicaSets->mSecondReplFromNodeName,
                                  aReplicaSetInfo->mReplicaSets->mSecondReplName )
                              != IDE_SUCCESS );
                    // receiver
                    IDE_TEST( dropReplicationForInternal(
                                  aStatement,
                                  aReplicaSetInfo->mReplicaSets->mSecondReplToNodeName,
                                  aReplicaSetInfo->mReplicaSets->mSecondReplName )
                              != IDE_SUCCESS );
                    break;
                case QDSD_REPL_QUERY_TYPE_CREATE:
                    // sender
                    sLocalMetaInfo =
                        sdiZookeeper::getNodeInfo( sParseTree->mNodeInfoList,
                                                   aReplicaSetInfo->mReplicaSets->mSecondReplToNodeName );

                    IDE_TEST( createReplicationForInternal(
                                  aStatement,
                                  aReplicaSetInfo->mReplicaSets->mSecondReplFromNodeName,
                                  aReplicaSetInfo->mReplicaSets->mSecondReplName,
                                  sLocalMetaInfo->mInternalRPHostIP,
                                  sLocalMetaInfo->mInternalRPPortNo )
                              != IDE_SUCCESS );
                    
                    idlOS::snprintf( sSqlStr,
                                     QD_MAX_SQL_LENGTH,
                                     "ALTER REPLICATION "QCM_SQL_STRING_SKIP_FMT" STOP",
                                     aReplicaSetInfo->mReplicaSets->mSecondReplName );

                    (void)sdiZookeeper::addPendingJob( sSqlStr,
                                                       aReplicaSetInfo->mReplicaSets->mSecondReplFromNodeName,
                                                       ZK_PENDING_JOB_AFTER_ROLLBACK,
                                                       QCI_STMT_MASK_DCL );

                    idlOS::snprintf( sSqlStr,
                                     QD_MAX_SQL_LENGTH,
                                     "DROP REPLICATION "QCM_SQL_STRING_SKIP_FMT"",
                                     aReplicaSetInfo->mReplicaSets->mSecondReplName );

                    (void)sdiZookeeper::addPendingJob( sSqlStr,
                                                       aReplicaSetInfo->mReplicaSets->mSecondReplFromNodeName,
                                                       ZK_PENDING_JOB_AFTER_ROLLBACK,
                                                       QCI_STMT_MASK_DDL );

                    // receiver
                    sLocalMetaInfo =
                        sdiZookeeper::getNodeInfo( sParseTree->mNodeInfoList,
                                                   aReplicaSetInfo->mReplicaSets->mSecondReplFromNodeName );

                    IDE_TEST( createReplicationForInternal(
                                  aStatement,
                                  aReplicaSetInfo->mReplicaSets->mSecondReplToNodeName,
                                  aReplicaSetInfo->mReplicaSets->mSecondReplName,
                                  sLocalMetaInfo->mInternalRPHostIP,
                                  sLocalMetaInfo->mInternalRPPortNo )
                              != IDE_SUCCESS );

                    idlOS::snprintf( sSqlStr,
                                     QD_MAX_SQL_LENGTH,
                                     "DROP REPLICATION "QCM_SQL_STRING_SKIP_FMT"",
                                     aReplicaSetInfo->mReplicaSets->mSecondReplName );

                    (void)sdiZookeeper::addPendingJob( sSqlStr,
                                                       aReplicaSetInfo->mReplicaSets->mSecondReplToNodeName,
                                                       ZK_PENDING_JOB_AFTER_ROLLBACK,
                                                       QCI_STMT_MASK_DDL );

                    break;
                case QDSD_REPL_QUERY_TYPE_ADD:
                    IDE_TEST( addReplicationForInternal( aStatement,
                                                         aReplicaSetInfo,
                                                         sBackupOrder )
                              != IDE_SUCCESS );
                    break;
                case QDSD_REPL_QUERY_TYPE_START:
                    // sender
                    IDE_TEST( syncReplicationForInternal(
                                  aStatement,
                                  aReplicaSetInfo->mReplicaSets->mSecondReplFromNodeName,
                                  aReplicaSetInfo->mReplicaSets->mSecondReplName )
                              != IDE_SUCCESS );
                    break;
                case QDSD_REPL_QUERY_TYPE_FLUSH:
                    IDE_TEST( flushReplicationForInternal(
                                  aStatement,
                                  aReplicaSetInfo->mReplicaSets->mPrimaryNodeName,
                                  aReplicaSetInfo->mReplicaSets->mSecondReplName )
                              != IDE_SUCCESS );
                    break;
                default:
                    IDE_ASSERT(0);
                    break;
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_REPLICA_SET_COUNT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_TOO_MANY_REPLICA_SETS ) );
    }
    IDE_EXCEPTION( ERR_INVALID_KSAFETY )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_INVALID_KSAFETY ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdsd::createReplicationForInternal( qcStatement * aStatement,
                                           SChar       * aNodeName,
                                           SChar       * aReplName,
                                           SChar       * aHostIP,
                                           UInt          aPortNo )
{
    sdiLocalMetaInfo   sLocalMetaInfo;
    SChar              sSqlStr[QD_MAX_SQL_LENGTH + 1];
    
    SChar              sReplicationModeStr[SDI_REPLICATION_MODE_MAX_SIZE + 1];

    IDE_TEST( sdi::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );

    switch ( sLocalMetaInfo.mReplicationMode )
    {
        case SDI_REP_MODE_CONSISTENT_CODE:
            idlOS::snprintf( sReplicationModeStr,
                             ID_SIZEOF(sReplicationModeStr),
                             SDI_REP_MODE_CONSISTENT_STR );
            break;

            /* unset replication mode */
        case SDI_REP_MODE_NETWAIT_CODE:
        case SDI_REP_MODE_NOWAIT_CODE:
            /* R2HA unsupported mode, not yet */
            /* Currently, these mode temporarily run in LAZY mode instead */
            idlOS::snprintf( sReplicationModeStr,
                             ID_SIZEOF(sReplicationModeStr),
                             "LAZY" );
            break;
        case SDI_REP_MODE_NULL_CODE:
            IDE_RAISE( ERR_DOES_NOT_SET_REPLICATION );
        default :
            IDE_DASSERT( 0 );
            break;
    }

    if ( idlOS::strncmp( sLocalMetaInfo.mNodeName, 
                         aNodeName,
                         SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
    {
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "CREATE %s REPLICATION "QCM_SQL_STRING_SKIP_FMT" OPTIONS PARALLEL %"ID_INT32_FMT" WITH '%s',%"ID_INT32_FMT" ",
                         sReplicationModeStr,
                         aReplName,
                         sLocalMetaInfo.mParallelCount,
                         aHostIP,
                         aPortNo );
    }
    else
    {
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "CREATE %s REPLICATION "QCM_SQL_STRING_SKIP_FMT" OPTIONS PARALLEL %"ID_INT32_FMT" WITH ''%s'',%"ID_INT32_FMT" ",
                         sReplicationModeStr,
                         aReplName,
                         sLocalMetaInfo.mParallelCount,
                         aHostIP,
                         aPortNo );
    }
        
    IDE_TEST( executeRemoteSQLWithNewTrans( aStatement,
                                            aNodeName,
                                            sSqlStr,
                                            ID_TRUE )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_DOES_NOT_SET_REPLICATION )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_NOT_EXIST_REPLICA_SET ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdsd::createReverseReplication( qcStatement * aStatement,
                                       SChar       * aNodeName,
                                       SChar       * aReplName,
                                       SChar       * aOtherNodeName,
                                       SChar       * aHostIP,
                                       UInt          aPortNo,
                                       sdiReplDirectionalRole aRole )
{
    sdiLocalMetaInfo   sLocalMetaInfo;
    SChar              sSqlStr[QD_MAX_SQL_LENGTH + 1];
    
    SChar              sReplicationModeStr[SDI_REPLICATION_MODE_MAX_SIZE + 1];
    SChar              sPropagationOption[30];

    UInt               sExecCount = 0;
    ULong              sCount = 0;
    idBool             sFetchResult = ID_FALSE;

    /* Check RP Exist */
    /* ReverseRP는 Failover/Failback 시에만 생성되며 Failback 완료시 삭제된다.
     * ReverseRP이전에 생성되어 있는 같은 ReverseRP가 존재한다면 이전 시도에서 생성된 찌꺼기 이므로 Drop 해주어야 한다. */
    idlOS::snprintf( sSqlStr, 
                     QD_MAX_SQL_LENGTH + 1,
                     "select Count(*) from system_.sys_replications_ where replication_name='%s' ",
                     aReplName );

    IDE_TEST( sdi::shardExecDirect( aStatement,
                                    aNodeName,
                                    (SChar*)sSqlStr,
                                    (UInt) idlOS::strlen(sSqlStr),
                                    SDI_INTERNAL_OP_FAILBACK,
                                    &sExecCount,
                                    &sCount,
                                    NULL,
                                    ID_SIZEOF(sCount),
                                    &sFetchResult )
              != IDE_SUCCESS );

    if ( sCount != 0 )
    {
        /* ReverseRP 생성 시점에 존재하면 안된다. Drop 하고 새로 생성해야 한다. */
        IDE_TEST( dropReplicationForInternal( aStatement,
                                              aNodeName,
                                              aReplName )
                  != IDE_SUCCESS );
    }

    IDE_TEST( sdi::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );

    idlOS::snprintf( sReplicationModeStr,
                     ID_SIZEOF(sReplicationModeStr),
                     "LAZY" );

    switch ( aRole )
    {
        case SDI_REPL_SENDER:
            idlOS::snprintf( sPropagationOption,
                             ID_SIZEOF( sPropagationOption),
                             "FOR PROPAGATION" );
            break;
        case SDI_REPL_RECEIVER:
            idlOS::snprintf( sPropagationOption,
                             ID_SIZEOF( sPropagationOption),
                             "FOR PROPAGABLE LOGGING" );
            break;
        default:
            IDE_DASSERT( 0 );
            break;
    }

    if ( idlOS::strncmp( sLocalMetaInfo.mNodeName, 
                         aNodeName,
                         SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
    {
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "CREATE %s REPLICATION "QCM_SQL_STRING_SKIP_FMT" %s OPTIONS PARALLEL %"ID_INT32_FMT" WITH '%s',%"ID_INT32_FMT" ",
                         sReplicationModeStr,
                         aReplName,
                         sPropagationOption,
                         sLocalMetaInfo.mParallelCount,
                         aHostIP,
                         aPortNo );
    }
    else
    {
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "CREATE %s REPLICATION "QCM_SQL_STRING_SKIP_FMT" %s OPTIONS PARALLEL %"ID_INT32_FMT" WITH ''%s'',%"ID_INT32_FMT" ",
                         sReplicationModeStr,
                         aReplName,
                         sPropagationOption,
                         sLocalMetaInfo.mParallelCount,
                         aHostIP,
                         aPortNo );
    }
        
    IDE_TEST( executeRemoteSQLWithNewTrans( aStatement,
                                            aNodeName,
                                            sSqlStr,
                                            ID_TRUE )
              != IDE_SUCCESS );

    /* Create 된 RP는 Rollback시 지워주어야 한다. */
    idlOS::snprintf(sSqlStr,
                    QD_MAX_SQL_LENGTH,
                    "alter replication "QCM_SQL_STRING_SKIP_FMT" stop",
                    aReplName );

    switch ( aRole )
    {
        case SDI_REPL_SENDER:
            (void)sdiZookeeper::addPendingJob( sSqlStr,
                                               aNodeName,
                                               ZK_PENDING_JOB_AFTER_ROLLBACK,
                                               QCI_STMT_MASK_MAX );
            break;
        case SDI_REPL_RECEIVER:
            (void)sdiZookeeper::addPendingJob( sSqlStr,
                                               aOtherNodeName,
                                               ZK_PENDING_JOB_AFTER_ROLLBACK,
                                               QCI_STMT_MASK_MAX );
            break;
        default:
            IDE_DASSERT( 0 );
            break;
    }

    idlOS::snprintf(sSqlStr,
                    QD_MAX_SQL_LENGTH,
                    "drop replication "QCM_SQL_STRING_SKIP_FMT"",
                    aReplName );

    (void)sdiZookeeper::addPendingJob( sSqlStr,
                                       aNodeName,
                                       ZK_PENDING_JOB_AFTER_ROLLBACK,
                                       QCI_STMT_MASK_MAX );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qdsd::stopReplicationForInternal( qcStatement * aStatement,
                                         SChar       * aNodeName,
                                         SChar       * aReplName )
{
    SChar sSqlStr[QD_MAX_SQL_LENGTH + 1];

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "ALTER REPLICATION "QCM_SQL_STRING_SKIP_FMT" STOP ",
                     aReplName );
    
    IDE_TEST( executeRemoteSQLWithNewTrans( aStatement,
                                            aNodeName,
                                            sSqlStr,
                                            ID_FALSE )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdsd::dropReplicationForInternal( qcStatement * aStatement,
                                         SChar       * aNodeName,
                                         SChar       * aReplName )
{
    SChar sSqlStr[QD_MAX_SQL_LENGTH + 1];

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "DROP REPLICATION "QCM_SQL_STRING_SKIP_FMT" ",
                     aReplName );
    
    IDE_TEST( executeRemoteSQLWithNewTrans( aStatement,
                                            aNodeName,
                                            sSqlStr,
                                            ID_TRUE)
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdsd::addReplicationForInternal( qcStatement       * aStatement,
                                        sdiReplicaSetInfo * aReplicaSetInfo,
                                        sdiBackupOrder      aBackupOrder )
{
    sdiTableInfoList  * sTableInfoList = NULL;
    sdiTableInfoList  * sTmpTableInfoList = NULL;    
    sdiNode             sNode;
    sdiRangeInfo        sRangeInfo;
    sdiLocalMetaInfo    sLocalMetaInfo;
    sdiTableInfo      * sTableInfo = NULL;
    UInt                i = 0;
    SChar             * sReplicationName = NULL;
    SChar             * sSenderNodeName = NULL;
    SChar             * sReceiverNodeName = NULL;

    IDE_TEST( sdi::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );

    switch (aBackupOrder)
    {
        case SDI_BACKUP_1ST:
            sReplicationName = aReplicaSetInfo->mReplicaSets->mFirstReplName;
            sSenderNodeName = aReplicaSetInfo->mReplicaSets->mFirstReplFromNodeName;
            sReceiverNodeName =  aReplicaSetInfo->mReplicaSets->mFirstReplToNodeName;
            break;
        case SDI_BACKUP_2ND:
            sReplicationName = aReplicaSetInfo->mReplicaSets->mSecondReplName;
            sSenderNodeName = aReplicaSetInfo->mReplicaSets->mSecondReplFromNodeName;
            sReceiverNodeName =  aReplicaSetInfo->mReplicaSets->mSecondReplToNodeName;
            break;
        default:
            IDE_DASSERT(0);
            break;
    }
    // get node id
    IDE_TEST( sdi::getNodeByName( QC_SMI_STMT( aStatement ),
                                  aReplicaSetInfo->mReplicaSets->mPrimaryNodeName,
                                  aReplicaSetInfo->mReplicaSets->mSMN,
                                  &sNode )
              != IDE_SUCCESS );

    IDE_TEST( sdi::getTableInfoAllObject( aStatement,
                                          &sTableInfoList,
                                          aReplicaSetInfo->mReplicaSets->mSMN )
              != IDE_SUCCESS );

    for ( sTmpTableInfoList = sTableInfoList;
          sTmpTableInfoList != NULL;
          sTmpTableInfoList = sTmpTableInfoList->mNext )
    {
        sTableInfo = sTmpTableInfoList->mTableInfo;
        /* Procedure는 RP ADD를 수행하지 않는다. */
        if ( sTableInfo->mObjectType == 'T' )
        {
            switch (sdi::getShardObjectType(sTableInfo))
            {
                case SDI_SOLO_DIST_OBJECT:
                    // get range info
                    IDE_TEST( sdi::getRangeInfo ( aStatement,
                                                  QC_SMI_STMT( aStatement ),
                                                  aReplicaSetInfo->mReplicaSets->mSMN,
                                                  sTableInfo,
                                                  &sRangeInfo,
                                                  ID_FALSE )
                              != IDE_SUCCESS );
                    // solo는 하나에 노드를 기준으로 backup table을 생성하기 때문에
                    // solo가 존재하는 노드기준으로 add replication을 수행한다.
                    if (( sRangeInfo.mCount != 0 ) &&
                        ( sRangeInfo.mRanges->mNodeId == sNode.mNodeId ))
                    {
                        IDE_TEST( sdi::addReplTable( aStatement,
                                                     sSenderNodeName,
                                                     sReplicationName,
                                                     sTableInfo->mUserName,
                                                     sTableInfo->mObjectName,
                                                     SDM_NA_STR,
                                                     SDI_REPL_SENDER,
                                                     ID_TRUE )
                                  != IDE_SUCCESS );

                        IDE_TEST( sdi::addReplTable( aStatement,
                                                     sReceiverNodeName,
                                                     sReplicationName,
                                                     sTableInfo->mUserName,
                                                     sTableInfo->mObjectName,
                                                     SDM_NA_STR,
                                                     SDI_REPL_RECEIVER,
                                                     ID_TRUE )
                                  != IDE_SUCCESS );
                    }
                    break;
                case SDI_SINGLE_SHARD_KEY_DIST_OBJECT:
                    /* Default Partition check */
                    if ( sTableInfo->mDefaultNodeId == sNode.mNodeId )
                    {
                        IDE_TEST( sdi::addReplTable( aStatement,
                                                     sSenderNodeName,
                                                     sReplicationName,
                                                     sTableInfo->mUserName,
                                                     sTableInfo->mObjectName,
                                                     sTableInfo->mDefaultPartitionName,
                                                     SDI_REPL_SENDER,
                                                     ID_TRUE )
                                  != IDE_SUCCESS );

                        IDE_TEST( sdi::addReplTable( aStatement,
                                                     sReceiverNodeName,
                                                     sReplicationName,
                                                     sTableInfo->mUserName,
                                                     sTableInfo->mObjectName,
                                                     sTableInfo->mDefaultPartitionName,
                                                     SDI_REPL_RECEIVER,
                                                     ID_TRUE )
                                  != IDE_SUCCESS );
                    }
                    // get range info
                    IDE_TEST( sdi::getRangeInfo ( aStatement,
                                                  QC_SMI_STMT( aStatement ),
                                                  aReplicaSetInfo->mReplicaSets->mSMN,
                                                  sTableInfo,
                                                  &sRangeInfo,
                                                  ID_FALSE )
                              != IDE_SUCCESS );

                    for ( i = 0; i < sRangeInfo.mCount; i++ )
                    {
                        if (( sRangeInfo.mRanges[i].mNodeId == sNode.mNodeId ) &&
                            ( sRangeInfo.mRanges[i].mReplicaSetId == aReplicaSetInfo->mReplicaSets->mReplicaSetId ))
                        {
                            IDE_TEST( sdi::addReplTable( aStatement,
                                                         sSenderNodeName,
                                                         sReplicationName,
                                                         sTableInfo->mUserName,
                                                         sTableInfo->mObjectName,
                                                         sRangeInfo.mRanges[i].mPartitionName,
                                                         SDI_REPL_SENDER,
                                                         ID_TRUE )
                                      != IDE_SUCCESS );

                            IDE_TEST( sdi::addReplTable( aStatement,
                                                         sReceiverNodeName,
                                                         sReplicationName,
                                                         sTableInfo->mUserName,
                                                         sTableInfo->mObjectName,
                                                         sRangeInfo.mRanges[i].mPartitionName,
                                                         SDI_REPL_RECEIVER,
                                                         ID_TRUE )
                                      != IDE_SUCCESS );
                        }
                    }
                    break;
                case SDI_CLONE_DIST_OBJECT:
                case SDI_COMPOSITE_SHARD_KEY_DIST_OBJECT:
                case SDI_NON_SHARD_OBJECT:
                    break;
                default:
                    IDE_DASSERT(0);
                    break;
            }
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdsd::startReplicationForInternal( qcStatement * aStatement,
                                          SChar       * aNodeName,
                                          SChar       * aReplName )
{
    sdiLocalMetaInfo    sLocalMetaInfo;
    SChar               sSqlStr[QD_MAX_SQL_LENGTH + 1];

    sdiInternalOperation sInternalOP = SDI_INTERNAL_OP_NORMAL;

    if( qci::mSessionCallback.mGetShardInternalLocalOperation( aStatement->session->mMmSession ) 
        == SDI_INTERNAL_OP_FAILBACK )
    {
        sInternalOP = SDI_INTERNAL_OP_FAILBACK;
    }
    
    // LOCAL_META_INFO
    IDE_TEST( sdi::getLocalMetaInfo( &sLocalMetaInfo ) 
              != IDE_SUCCESS );

    if ( idlOS::strncmp( sLocalMetaInfo.mNodeName, 
                         aNodeName,
                         SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
    {
        IDE_TEST( executeLocalSQL( aStatement, 
                                   QCI_STMT_MASK_DCL,
                                   sSqlStr,
                                   QD_MAX_SQL_LENGTH,
                                   "ALTER REPLICATION "QCM_SQL_STRING_SKIP_FMT" START ",
                                   aReplName )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( sdi::checkShardLinker( aStatement ) != IDE_SUCCESS );

        if ( aStatement->session->mQPSpecific.mClientInfo != NULL )
        {
            IDE_TEST( executeRemoteSQL( aStatement, 
                                        aNodeName,
                                        sInternalOP,
                                        QCI_STMT_MASK_DCL,
                                        sSqlStr,
                                        QD_MAX_SQL_LENGTH,
                                        "ALTER REPLICATION "QCM_SQL_STRING_SKIP_FMT" START ",
                                        aReplName )
                      != IDE_SUCCESS);
        }
    }
    
    ideLog::log(IDE_SD_17,"[SHARD INTERNAL SQL]: %s, %s",aNodeName, sSqlStr);
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdsd::syncReplicationForInternal( qcStatement * aStatement,
                                          SChar       * aNodeName,
                                          SChar       * aReplName )
{
    sdiLocalMetaInfo    sLocalMetaInfo;
    SChar               sSqlStr[QD_MAX_SQL_LENGTH + 1];

    sdiInternalOperation sInternalOP = SDI_INTERNAL_OP_NORMAL;

    if( qci::mSessionCallback.mGetShardInternalLocalOperation( aStatement->session->mMmSession ) 
        == SDI_INTERNAL_OP_FAILBACK )
    {
        sInternalOP = SDI_INTERNAL_OP_FAILBACK;
    }
    
    // LOCAL_META_INFO
    IDE_TEST( sdi::getLocalMetaInfo( &sLocalMetaInfo ) 
              != IDE_SUCCESS );

    if ( idlOS::strncmp( sLocalMetaInfo.mNodeName, 
                         aNodeName,
                         SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
    {
        IDE_TEST( executeLocalSQL( aStatement, 
                                   QCI_STMT_MASK_DCL,
                                   sSqlStr,
                                   QD_MAX_SQL_LENGTH,
                                   "ALTER REPLICATION "QCM_SQL_STRING_SKIP_FMT" SYNC PARALLEL %"ID_UINT32_FMT,
                                   aReplName,
                                   sLocalMetaInfo.mParallelCount )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( sdi::checkShardLinker( aStatement ) != IDE_SUCCESS );

        if ( aStatement->session->mQPSpecific.mClientInfo != NULL )
        {
            IDE_TEST( executeRemoteSQL( aStatement, 
                                        aNodeName,
                                        sInternalOP,
                                        QCI_STMT_MASK_DCL,
                                        sSqlStr,
                                        QD_MAX_SQL_LENGTH,
                                        "ALTER REPLICATION "QCM_SQL_STRING_SKIP_FMT" SYNC PARALLEL %"ID_UINT32_FMT,
                                        aReplName,
                                        sLocalMetaInfo.mParallelCount )
                      != IDE_SUCCESS);
        }
    }
    
    ideLog::log(IDE_SD_17,"[SHARD INTERNAL SQL]: %s, %s",aNodeName, sSqlStr);
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



IDE_RC qdsd::truncateBackupTable( qcStatement * aStatement )
{
    qdShardParseTree  * sParseTree = NULL;
    sdiLocalMetaInfo    sLocalMetaInfo;
    sdiReplicaSetInfo   sReplicaSetInfo;
    sdiReplicaSetInfo   sCurrReplicaSetInfo;
    sdiReplicaSetInfo   sPrevReplicaSetInfo;
    sdiReplicaSetInfo   sPrevPrevReplicaSetInfo;
    sdiGlobalMetaInfo   sMetaNodeInfo = { ID_ULONG(0) };
    ULong               sSMN = ID_ULONG(0);

    sParseTree = (qdShardParseTree *)aStatement->myPlan->parseTree;
    
    IDE_TEST( sdi::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );    

    IDE_TEST( sdi::getGlobalMetaInfoCore( QC_SMI_STMT( aStatement ),
                                          &sMetaNodeInfo )
              != IDE_SUCCESS );

    sSMN = sMetaNodeInfo.mShardMetaNumber;
    
    IDE_TEST( sdi::getAllReplicaSetInfoSortedByPName( QC_SMI_STMT( aStatement ),
                                                      &sReplicaSetInfo,
                                                      sSMN )
              != IDE_SUCCESS );

    IDE_TEST( findBackupReplicaSetInfo( sLocalMetaInfo.mNodeName,
                                        &sReplicaSetInfo,
                                        &sCurrReplicaSetInfo,
                                        &sPrevReplicaSetInfo,
                                        &sPrevPrevReplicaSetInfo )
              != IDE_SUCCESS );
    
    if ( sLocalMetaInfo.mKSafety == 1 )
    {
        // node3 부터 backup table 삭제
        // 현재 노드에서 전노드의 first backup partition table을 삭제
        if ( sParseTree->mNodeCount > 2 )
        {
            if ( idlOS::strncmp( SDM_NA_STR,
                                 sCurrReplicaSetInfo.mReplicaSets[0].mFirstBackupNodeName,
                                 SDI_NODE_NAME_MAX_SIZE + 1 ) != 0 )
            {
                IDE_TEST( truncateBackupTableForInternal(
                              aStatement,
                              sCurrReplicaSetInfo.mReplicaSets[0].mFirstBackupNodeName,
                              &sPrevReplicaSetInfo )
                          != IDE_SUCCESS );
            }
            else
            {
                IDE_TEST( truncateBackupTableForInternal(
                              aStatement,
                              sCurrReplicaSetInfo.mReplicaSets[0].mStopFirstBackupNodeName,
                              &sPrevReplicaSetInfo )
                          != IDE_SUCCESS );
            }
        }
    }
    else if ( sLocalMetaInfo.mKSafety == 2 )
    {
        // node4 부터 backup table 삭제
        // 현재 노드에서 전노드의 second backup partition table을 삭제
        // 현재 노드에서 전전 노드의 second backup partition table을 삭제    
        if ( sParseTree->mNodeCount > 3 )
        {
            if ( idlOS::strncmp( SDM_NA_STR,
                                 sCurrReplicaSetInfo.mReplicaSets[0].mSecondBackupNodeName,
                                 SDI_NODE_NAME_MAX_SIZE + 1 ) != 0 )
            {
                IDE_TEST( truncateBackupTableForInternal(
                              aStatement,
                              sCurrReplicaSetInfo.mReplicaSets[0].mSecondBackupNodeName,
                              &sPrevReplicaSetInfo )
                          != IDE_SUCCESS );
            }
            else
            {
                IDE_TEST( truncateBackupTableForInternal(
                              aStatement,
                              sCurrReplicaSetInfo.mReplicaSets[0].mStopSecondBackupNodeName,
                              &sPrevReplicaSetInfo )
                          != IDE_SUCCESS );
            }
            

            if ( idlOS::strncmp( SDM_NA_STR,
                                 sCurrReplicaSetInfo.mReplicaSets[0].mFirstBackupNodeName,
                                 SDI_NODE_NAME_MAX_SIZE + 1 ) != 0 )
            {
                IDE_TEST( truncateBackupTableForInternal(
                              aStatement,
                              sCurrReplicaSetInfo.mReplicaSets[0].mFirstBackupNodeName,
                              &sPrevPrevReplicaSetInfo )
                          != IDE_SUCCESS );
            }
            else
            {
                IDE_TEST( truncateBackupTableForInternal(
                              aStatement,
                              sCurrReplicaSetInfo.mReplicaSets[0].mStopFirstBackupNodeName,
                              &sPrevPrevReplicaSetInfo )
                          != IDE_SUCCESS );
            }
        }
    }    

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdsd::truncateBackupTableForInternal( qcStatement       * aStatement,
                                             SChar             * aNodeName,
                                             sdiReplicaSetInfo * aReplicaSetInfo )
{
    sdiTableInfoList  * sTableInfoList = NULL;
    sdiTableInfoList  * sTmpTableInfoList = NULL;
    sdiTableInfo      * sTableInfo = NULL;
    SChar               sSqlStr[QD_MAX_SQL_LENGTH + 1];
    sdiNode             sNode;
    sdiRangeInfo        sRangeInfo;
    UInt                i = 0;
    SChar               sBufferNameStr[QC_MAX_OBJECT_NAME_LEN + 1];
    
    // get node id
    IDE_TEST( sdi::getNodeByName( QC_SMI_STMT( aStatement ),
                                  aReplicaSetInfo->mReplicaSets->mPrimaryNodeName,
                                  aReplicaSetInfo->mReplicaSets->mSMN,
                                  &sNode )
              != IDE_SUCCESS );

    IDE_TEST( sdi::getTableInfoAllObject( aStatement,
                                          &sTableInfoList,
                                          aReplicaSetInfo->mReplicaSets->mSMN )
              != IDE_SUCCESS );

    for ( sTmpTableInfoList = sTableInfoList;
          sTmpTableInfoList != NULL;
          sTmpTableInfoList = sTmpTableInfoList->mNext )
    {
        sTableInfo = sTmpTableInfoList->mTableInfo;
        /* Procedure는 truncate를 수행하지 않는다. */
        if ( sTableInfo->mObjectType == 'T' )
        {
            idlOS::snprintf( sBufferNameStr,
                             QC_MAX_OBJECT_NAME_LEN + 1,
                             "%s%s",
                             SDI_BACKUP_TABLE_PREFIX,
                             sTmpTableInfoList->mTableInfo->mObjectName );

            switch (sdi::getShardObjectType(sTableInfo))
            {
                case SDI_SOLO_DIST_OBJECT:
                    // get range info
                    IDE_TEST( sdi::getRangeInfo ( aStatement,
                                                  QC_SMI_STMT( aStatement ),
                                                  aReplicaSetInfo->mReplicaSets->mSMN,
                                                  sTableInfo,
                                                  &sRangeInfo,
                                                  ID_FALSE )
                              != IDE_SUCCESS );
                    IDE_DASSERT( sRangeInfo.mCount != 0 );
                    if (( sRangeInfo.mCount != 0 ) &&
                        ( sRangeInfo.mRanges->mNodeId == sNode.mNodeId ))
                    {
                        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                         "TRUNCATE TABLE "QCM_SQL_STRING_SKIP_FMT" ",
                                         sBufferNameStr,
                                         aNodeName );

                        IDE_TEST( sdi::shardExecTempDDLWithNewTrans( aStatement,
                                                                     aNodeName,
                                                                     sSqlStr,
                                                                     idlOS::strlen(sSqlStr) )
                                  != IDE_SUCCESS );
                    }
                    break;
                case SDI_SINGLE_SHARD_KEY_DIST_OBJECT:
                    /* Default Partition check */
                    if ( sTableInfo->mDefaultNodeId == sNode.mNodeId )
                    {
                        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                         "ALTER TABLE "QCM_SQL_STRING_SKIP_FMT" TRUNCATE PARTITION "QCM_SQL_STRING_SKIP_FMT" ",
                                         sBufferNameStr,
                                         sTableInfo->mDefaultPartitionName,
                                         aNodeName );

                        IDE_TEST( sdi::shardExecTempDDLWithNewTrans( aStatement,
                                                                     aNodeName,
                                                                     sSqlStr,
                                                                     idlOS::strlen(sSqlStr) )
                                  != IDE_SUCCESS );
                    }
                    // get range info
                    IDE_TEST( sdi::getRangeInfo ( aStatement,
                                                  QC_SMI_STMT( aStatement ),
                                                  aReplicaSetInfo->mReplicaSets->mSMN,
                                                  sTableInfo,
                                                  &sRangeInfo,
                                                  ID_FALSE )
                              != IDE_SUCCESS );

                    for ( i = 0; i < sRangeInfo.mCount; i++ )
                    {
                        if (( sRangeInfo.mRanges[i].mNodeId == sNode.mNodeId ) &&
                            ( sRangeInfo.mRanges[i].mReplicaSetId == aReplicaSetInfo->mReplicaSets->mReplicaSetId ))
                        {
                            idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                             "ALTER TABLE "QCM_SQL_STRING_SKIP_FMT" TRUNCATE PARTITION "QCM_SQL_STRING_SKIP_FMT" ",
                                             sBufferNameStr,
                                             sRangeInfo.mRanges[i].mPartitionName,
                                             aNodeName );

                            IDE_TEST( sdi::shardExecTempDDLWithNewTrans( aStatement,
                                                                         aNodeName,
                                                                         sSqlStr,
                                                                         idlOS::strlen(sSqlStr) )
                                      != IDE_SUCCESS );
                        }
                    }
                    break;
                case SDI_CLONE_DIST_OBJECT:
                case SDI_COMPOSITE_SHARD_KEY_DIST_OBJECT:
                case SDI_NON_SHARD_OBJECT:
                    break;
                default:
                    IDE_DASSERT(0);
                    break;
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdsd::findBackupReplicaSetInfo( SChar             * aNodeName,
                                       sdiReplicaSetInfo * aReplicaSetInfo,
                                       sdiReplicaSetInfo * aCurrReplicaSetInfo,
                                       sdiReplicaSetInfo * aPrevReplicaSetInfo,
                                       sdiReplicaSetInfo * aPrevPrevReplicaSetInfo )
{
    sdiLocalMetaInfo    sLocalMetaInfo;
    SChar             * sTempNodeName = NULL;
    UInt                sCnt = 0;
    UInt                i = 0;

    aCurrReplicaSetInfo->mCount     = 0;
    aPrevReplicaSetInfo->mCount     = 0;
    aPrevPrevReplicaSetInfo->mCount = 0;

    IDE_TEST( sdi::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );

    for ( sCnt = 0; sCnt < aReplicaSetInfo->mCount; sCnt++ )
    {
        // curr replicaSetInfo
        if ( aCurrReplicaSetInfo->mCount != 1 )
        {   
            if ( idlOS::strncmp( aReplicaSetInfo->mReplicaSets[sCnt].mPrimaryNodeName, 
                                 aNodeName,
                                 SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
            {
                idlOS::memcpy( &aCurrReplicaSetInfo->mReplicaSets[0],
                               &aReplicaSetInfo->mReplicaSets[sCnt],
                               ID_SIZEOF( sdiReplicaSet ) );
            
                aCurrReplicaSetInfo->mCount = 1;
            }
        }        
        
        for ( i = 0; i < sLocalMetaInfo.mKSafety; i++ )
        {
            // prev replicaSetInfo
            if ( aPrevReplicaSetInfo->mCount != 1 )
            {
                if ( idlOS::strncmp( SDM_NA_STR,
                                     aReplicaSetInfo->mReplicaSets[sCnt].mFirstBackupNodeName,
                                     SDI_NODE_NAME_MAX_SIZE + 1 ) != 0 )
                {
                    sTempNodeName = aReplicaSetInfo->mReplicaSets[sCnt].mFirstBackupNodeName;
                }
                else
                {
                    sTempNodeName = aReplicaSetInfo->mReplicaSets[sCnt].mStopFirstBackupNodeName;
                }
                                
                if ( idlOS::strncmp( sTempNodeName, 
                                     aNodeName,
                                     SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
                {
                    idlOS::memcpy( &aPrevReplicaSetInfo->mReplicaSets[0],
                                   &aReplicaSetInfo->mReplicaSets[sCnt],
                                   ID_SIZEOF( sdiReplicaSet ) );
            
                    aPrevReplicaSetInfo->mCount = 1;
                }
            }

            if ( i == 1 )
            {    
                // prev prev replicaSetInfo
                if ( aPrevPrevReplicaSetInfo->mCount != 1 )
                {
                    if ( idlOS::strncmp( SDM_NA_STR,
                                         aReplicaSetInfo->mReplicaSets[sCnt].mSecondBackupNodeName,
                                         SDI_NODE_NAME_MAX_SIZE + 1 ) != 0 )
                    {
                        sTempNodeName = aReplicaSetInfo->mReplicaSets[sCnt].mSecondBackupNodeName;
                    }
                    else
                    {
                        sTempNodeName = aReplicaSetInfo->mReplicaSets[sCnt].mStopSecondBackupNodeName;
                    }
                    
                    if ( idlOS::strncmp( sTempNodeName, 
                                         aNodeName,
                                         SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
                    {
                        idlOS::memcpy( &aPrevPrevReplicaSetInfo->mReplicaSets[0],
                                       &aReplicaSetInfo->mReplicaSets[sCnt],
                                       ID_SIZEOF( sdiReplicaSet ) );
            
                        aPrevPrevReplicaSetInfo->mCount = 1;
                    }
                }
            }
        }       
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * IN: aReShardAttr, aNameBufSize
 * IN/OUT: aPrimaryTableName, aBackupTableName
 */
IDE_RC qdsd::getTableNames( qdReShardAttribute * aReShardAttr, 
                            SChar * aSessionUserName, 
                            UInt    aNameBufSize, 
                            SChar * aPrimaryTableName, 
                            SChar * aBackupTableName )
{
    UInt               sObjectNameBufPosition = 0;
    SChar              sObjectNameBuf[ QC_MAX_OBJECT_FULL_STR_LEN + 1 ] = {0,};
    UInt               sBackupObjectNameBufPosition = 0;
    SChar              sBackupObjectNameBuf[ QC_MAX_OBJECT_FULL_STR_LEN + 1 ] = {0,};

    IDE_TEST_RAISE( aNameBufSize < QC_MAX_OBJECT_FULL_STR_LEN + 1, ERR_BUF_SIZE);

    if ( QC_IS_NULL_NAME(aReShardAttr->mUserName) != ID_TRUE )
    {
        QC_STR_COPY(sObjectNameBuf, aReShardAttr->mUserName); /* "user_name" */
        sObjectNameBufPosition = aReShardAttr->mUserName.size ;
        QC_STR_COPY(sBackupObjectNameBuf, aReShardAttr->mUserName); /* "user_name" */
        sBackupObjectNameBufPosition = aReShardAttr->mUserName.size; 
    }
    else
    {
        idlOS::strncpy( sObjectNameBuf, aSessionUserName, QC_MAX_OBJECT_NAME_LEN ); /* "user_name" */
        sObjectNameBufPosition = idlOS::strlen(aSessionUserName); 
        idlOS::strncpy( sBackupObjectNameBuf, aSessionUserName, QC_MAX_OBJECT_NAME_LEN ); /* "user_name" */
        sBackupObjectNameBufPosition = idlOS::strlen(aSessionUserName);
    }

    sObjectNameBuf[sObjectNameBufPosition] = '\"'; 
    sObjectNameBufPosition++;
    sObjectNameBuf[sObjectNameBufPosition] = '.'; /* "user." */
    sObjectNameBufPosition++;
    sObjectNameBuf[sObjectNameBufPosition] = '\"'; 
    sObjectNameBufPosition++;
    sBackupObjectNameBuf[sBackupObjectNameBufPosition] = '\"';
    sBackupObjectNameBufPosition++;
    sBackupObjectNameBuf[sBackupObjectNameBufPosition] = '.'; /* "user." */
    sBackupObjectNameBufPosition++;
    sBackupObjectNameBuf[sBackupObjectNameBufPosition] = '\"';
    sBackupObjectNameBufPosition++;

    idlOS::strncpy(sBackupObjectNameBuf + sBackupObjectNameBufPosition, 
                SDI_BACKUP_TABLE_PREFIX, 
                ID_SIZEOF(SDI_BACKUP_TABLE_PREFIX)); /* "user_name._BAK_" */
    sBackupObjectNameBufPosition += idlOS::strlen(SDI_BACKUP_TABLE_PREFIX);

    IDE_DASSERT(QC_IS_NULL_NAME(aReShardAttr->mObjectName) != ID_TRUE);

    QC_STR_COPY(sObjectNameBuf + sObjectNameBufPosition, aReShardAttr->mObjectName); /* "user_name.table_name" */
    sObjectNameBufPosition += aReShardAttr->mObjectName.size;

    QC_STR_COPY(sBackupObjectNameBuf + sBackupObjectNameBufPosition, aReShardAttr->mObjectName); /* "user_name._BAk_table_name" */
    sBackupObjectNameBufPosition += aReShardAttr->mObjectName.size;

    if ( QC_IS_NULL_NAME(aReShardAttr->mPartitionName) != ID_TRUE )
    {
        sObjectNameBuf[sObjectNameBufPosition] = '\"'; 
        sObjectNameBufPosition++;

        idlOS::snprintf( sObjectNameBuf + sObjectNameBufPosition, 
                         ID_SIZEOF(sObjectNameBuf) - sObjectNameBufPosition,
                         " partition ",
                         11 ); /* "user_name.table_name partition " */
        sObjectNameBufPosition += 11;

        sBackupObjectNameBuf[sBackupObjectNameBufPosition] = '\"';
        sBackupObjectNameBufPosition++;

        idlOS::snprintf( sBackupObjectNameBuf + sBackupObjectNameBufPosition, 
                         ID_SIZEOF(sBackupObjectNameBuf) - sBackupObjectNameBufPosition, 
                         " partition ",
                         11 ); /* "user_name._BAk_table_name partition " */
        sBackupObjectNameBufPosition += 11;

        sObjectNameBuf[sObjectNameBufPosition] = '\"'; 
        sObjectNameBufPosition++;

        QC_STR_COPY(sObjectNameBuf + sObjectNameBufPosition, aReShardAttr->mPartitionName); /* "user_name.table_name partition partition_name" */
        sObjectNameBufPosition += aReShardAttr->mPartitionName.size;

        sBackupObjectNameBuf[sBackupObjectNameBufPosition] = '\"';
        sBackupObjectNameBufPosition++;
        
        QC_STR_COPY(sBackupObjectNameBuf + sBackupObjectNameBufPosition, aReShardAttr->mPartitionName); /* "user_name.table_name partition partition_name" */
        sBackupObjectNameBufPosition += aReShardAttr->mPartitionName.size;
    }

    sObjectNameBuf[sObjectNameBufPosition] = '\0';
    sBackupObjectNameBuf[sBackupObjectNameBufPosition] = '\0';

    idlOS::strncpy(aPrimaryTableName, sObjectNameBuf, aNameBufSize);
    idlOS::strncpy(aBackupTableName, sBackupObjectNameBuf, aNameBufSize);

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_BUF_SIZE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,"qdsd::getTableNames", "The length of table name is too long." ) );
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/* 
 * cleanup garbage replication
 */ 
IDE_RC qdsd::cleanupReplWithNewTransCommit( qcStatement * aStatement, 
                                            SChar * aNodeName, 
                                            SChar * aReplName )
{
    const SChar  sReplStopSQLFmt[] = "alter replication "QCM_SQL_STRING_SKIP_FMT" stop";
    const SChar  sReplDropSQLFmt[] = "drop replication "QCM_SQL_STRING_SKIP_FMT"";
    SChar sSqlStr[ SDI_REPLICATION_NAME_MAX_SIZE + 
                   IDL_MAX(ID_SIZEOF(sReplStopSQLFmt), ID_SIZEOF(sReplDropSQLFmt)) ] = {0,};

    //stop
    idlOS::snprintf(sSqlStr, ID_SIZEOF(sSqlStr), sReplStopSQLFmt, aReplName);
    (void)sdi::shardExecTempDDLWithNewTrans(aStatement,
                                            aNodeName,
                                            sSqlStr,
                                            idlOS::strlen(sSqlStr));

    //drop
    idlOS::snprintf(sSqlStr, ID_SIZEOF(sSqlStr), sReplDropSQLFmt, aReplName);
    (void)sdi::shardExecTempDDLWithNewTrans(aStatement,
                                            aNodeName,
                                            sSqlStr,
                                            idlOS::strlen(sSqlStr));
    return IDE_SUCCESS;
}

IDE_RC qdsd::createReplWithNewTransCommit( qcStatement * aStatement, 
                                           SChar       * aNodeName, 
                                           SChar       * aReplName, 
                                           SChar       * aIP, 
                                           UInt          aPort)
{
    const SChar      * sReplCreateSQLFmt;
    SChar            * sSqlStr = NULL;
    UInt               sLen = 0;
    sdiLocalMetaInfo   sLocalMetaInfo;
   
    IDE_TEST( sdi::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );

    // BUG-48616
    if ( sdi::isSameNode( sLocalMetaInfo.mNodeName, aNodeName ) == ID_TRUE )
    {
        sReplCreateSQLFmt = "create lazy replication "QCM_SQL_STRING_SKIP_FMT" with '%s', %"ID_UINT32_FMT;
    }
    else
    {
        sReplCreateSQLFmt = "create lazy replication "QCM_SQL_STRING_SKIP_FMT" with ''%s'', %"ID_UINT32_FMT;
    }
    
    sLen = idlOS::strlen(sReplCreateSQLFmt) + 1 + SDI_REPLICATION_NAME_MAX_SIZE +
        IDL_IP_ADDR_MAX_LEN + IDL_IP_PORT_MAX_LEN;

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      sLen,
                                      &sSqlStr )
              != IDE_SUCCESS );

    idlOS::snprintf(sSqlStr,
                    sLen,
                    sReplCreateSQLFmt,
                    aReplName,
                    aIP,
                    aPort);
    
    IDE_TEST( sdi::shardExecTempDDLWithNewTrans( aStatement,
                                                 aNodeName,
                                                 sSqlStr,
                                                 idlOS::strlen(sSqlStr)) != IDE_SUCCESS );
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC qdsd::alterReplAddTableWithNewTrans( qcStatement * aStatement, 
                                            SChar * aNodeName, 
                                            SChar * aReplName, 
                                            SChar * aFromTableName, 
                                            SChar * aToTableName)
{
    UInt sShardDDLLockTryCount = QCG_GET_SESSION_SHARD_DDL_LOCK_TRY_COUNT(aStatement);
    UInt i = 0;
    const SChar sAlterReplAddTblSQLFmt[] = "alter replication "QCM_SQL_STRING_SKIP_FMT" add table from "QCM_SQL_STRING_SKIP_FMT" to "QCM_SQL_STRING_SKIP_FMT"";
    SChar sSqlStr[ ID_SIZEOF(sAlterReplAddTblSQLFmt) + 
                   SDI_REPLICATION_NAME_MAX_SIZE + 
                   QC_MAX_OBJECT_FULL_STR_LEN + 
                   QC_MAX_OBJECT_FULL_STR_LEN ] = {0,};
    
    idlOS::snprintf(sSqlStr, ID_SIZEOF(sSqlStr), sAlterReplAddTblSQLFmt, aReplName, aFromTableName, aToTableName);
    
    for (i = 0; i < sShardDDLLockTryCount; i++ )
    {
        if ( sdi::shardExecTempDDLWithNewTrans(aStatement,
                                               aNodeName,
                                               sSqlStr,
                                               idlOS::strlen(sSqlStr)) != IDE_SUCCESS )
        {
            /* do nothing : retry */
        }
        else
        {
            break;
        }
    }

    IDE_TEST(i == sShardDDLLockTryCount);

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC qdsd::createReplicationsWithNewTrans( qcStatement * aStatement,
                                             iduList * aNodeInfoList,
                                             SChar * aFromNodeName,
                                             SChar * aToNodeName,
                                             SChar * aReplicationName )
{
    sdiLocalMetaInfo * sToNodeLocalInfo = NULL;

    sToNodeLocalInfo = sdiZookeeper::getNodeInfo(aNodeInfoList, aToNodeName);
    IDE_TEST_RAISE( sToNodeLocalInfo == NULL, ERR_TO_NODE );

    IDE_TEST( cleanupReplWithNewTransCommit( aStatement,
                                             aFromNodeName,
                                             aReplicationName ) != IDE_SUCCESS);

    IDE_TEST( cleanupReplWithNewTransCommit( aStatement,
                                             aToNodeName,
                                             aReplicationName ) != IDE_SUCCESS);

    IDE_TEST( createReplWithNewTransCommit( aStatement,
                                            aFromNodeName,
                                            aReplicationName,
                                            sToNodeLocalInfo->mInternalRPHostIP,
                                            sToNodeLocalInfo->mInternalRPPortNo) != IDE_SUCCESS );

    // temporary replication for resharding must be start on FromNode
    // to prevent starting on ToNode, set the replication host of ToNode to self host
    IDE_TEST(createReplWithNewTransCommit( aStatement,
                                           aToNodeName,
                                           aReplicationName,
                                           sToNodeLocalInfo->mInternalRPHostIP,
                                           sToNodeLocalInfo->mInternalRPPortNo ) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TO_NODE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_INVALID_NODE_NAME, aToNodeName ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qdsd::prepareReplicationsForSync( qcStatement * aStatement,
                                         iduList * aNodeInfoList,
                                         SChar * aFromNodeName,
                                         SChar * aToNodeName,
                                         SChar * aReplicationName,
                                         qdReShardAttribute * aReShardAttr)
{
    qdReShardAttribute * sReShardAttr = NULL;
    SChar                sUserName[QC_MAX_OBJECT_NAME_LEN + 1] = { 0, };
    SChar                sObjectNameBuf[ QC_MAX_OBJECT_FULL_STR_LEN + 1 ] = {0,};
    SChar                sBackupObjectNameBuf[ QC_MAX_OBJECT_FULL_STR_LEN + 1 ] = {0,};
    UInt                 sUserID;
    /* the precede operation related to replication must be commit by other transaction */
    IDE_TEST( createReplicationsWithNewTrans( aStatement,
                                              aNodeInfoList,
                                              aFromNodeName,
                                              aToNodeName,
                                              aReplicationName ) != IDE_SUCCESS );

    /* replication add table shard key dist table(range) and solo */
    for ( sReShardAttr = aReShardAttr;
          sReShardAttr != NULL;
          sReShardAttr = sReShardAttr->next )
    {
        if ( sReShardAttr->mObjectType == 'T' )
        {
            if (QC_IS_NULL_NAME(sReShardAttr->mUserName) == ID_TRUE)
            {
                sUserID = QCG_GET_SESSION_USER_ID(aStatement);
                IDE_TEST(qciMisc::getUserName(aStatement, sUserID, sUserName) != IDE_SUCCESS);
            }
            else
            {
                QC_STR_COPY(sUserName, sReShardAttr->mUserName);
            }

            IDE_TEST( getTableNames( sReShardAttr,
                                     sUserName,
                                     QC_MAX_OBJECT_FULL_STR_LEN + 1,
                                     sObjectNameBuf,
                                     sBackupObjectNameBuf ) != IDE_SUCCESS );

                IDE_TEST( alterReplAddTableWithNewTrans( aStatement,
                                                         aFromNodeName,
                                                         aReplicationName,
                                                         sObjectNameBuf,
                                                         sBackupObjectNameBuf )
                          != IDE_SUCCESS );

                IDE_TEST( alterReplAddTableWithNewTrans( aStatement,
                                                         aToNodeName,
                                                         aReplicationName,
                                                         sBackupObjectNameBuf,
                                                         sObjectNameBuf )
                          != IDE_SUCCESS )

        }
        else
        {
            /* 'P' procedure: do noting */
        }
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
IDE_RC qdsd::executeLocalSQL(qcStatement         * aStatement,
                             qciStmtType           aSQLBufStmtType,
                             SChar               * aSQLBuf,
                             UInt                  aSQLBufSize,
                             const SChar         * aSQLFormat,
                             ... )
{
    SInt    sWrittenSize = 0;
    va_list sAp;
    vSLong              sRowCnt;
    sdiLocalMetaInfo    sLocalMetaInfo;
    smiStatement      * sOldStmt = NULL;
    smiStatement        sSmiStmt;
    UInt                sSmiStmtFlag = SMI_STATEMENT_NORMAL|SMI_STATEMENT_ALL_CURSOR;
    SInt                sState = 0;
    smiStatement* spRootStmt;
    
    IDE_TEST( sdi::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );
    IDE_TEST_RAISE( sLocalMetaInfo.mNodeName[0] == '\0', no_meta_exist );
    
    if ( aSQLFormat != NULL )
    {
        va_start(sAp, aSQLFormat);
        
        sWrittenSize = idlOS::vsnprintf( aSQLBuf, 
                                         aSQLBufSize,
                                         aSQLFormat,
                                         sAp );
        va_end(sAp);
        
        IDE_TEST_RAISE(sWrittenSize < 0, BUF_OUTPUT_ERROR);
        IDE_TEST_RAISE(sWrittenSize >= (SInt)aSQLBufSize, ERR_NOT_ENOUGH_BUFSIZE );
    }

    ideLog::log(IDE_SD_17,"[SHARD_META] executeLocalSQL Begin: %s", aSQLBuf);
    qcg::getSmiStmt( aStatement, &sOldStmt );
    spRootStmt = sOldStmt->getTrans()->getStatement();
    IDE_TEST( sOldStmt->end(SMI_STATEMENT_RESULT_SUCCESS) != IDE_SUCCESS );

    qcg::setSmiStmt( aStatement, &sSmiStmt );
    sState = 1;

    IDE_TEST( sSmiStmt.begin( aStatement->mStatistics,
                              spRootStmt,
                              sSmiStmtFlag )
              != IDE_SUCCESS );
    sState = 2;

    switch (qciMisc::getStmtType(aSQLBufStmtType))
    {
        case QCI_STMT_MASK_DDL:
            /* R2HA 2 qcstatement -> 1 smstatement problem , implicit savepoint problem */
            IDE_TEST( qciMisc::runRollbackableInternalDDL( aStatement,
                                                           QC_SMI_STMT( aStatement ),
                                                           QCG_GET_SESSION_USER_ID( aStatement ),
                                                           aSQLBuf )
                      != IDE_SUCCESS );
            break;
        case QCI_STMT_MASK_SP:
        case QCI_STMT_MASK_DML:
            IDE_TEST( qcg::runSQLforShardMeta( QC_SMI_STMT( aStatement ),
                                               aSQLBuf,
                                               & sRowCnt,
                                               NULL ) //aStatement->session ) 
                      != IDE_SUCCESS );
            break;
        case QCI_STMT_MASK_DCL:
            IDE_TEST( qciMisc::runDCLforInternal( aStatement,
                                                  aSQLBuf,
                                                  aStatement->session->mMmSession )
                      != IDE_SUCCESS );
            break;
        case QCI_STMT_MASK_DB:
        default:
            IDE_DASSERT(0);
            break;
    }
    sState = 1;
    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    
    IDE_TEST( sOldStmt->begin( aStatement->mStatistics,
                               spRootStmt,
                               sSmiStmtFlag )
                != IDE_SUCCESS );
    sState = 0;
    qcg::setSmiStmt( aStatement, sOldStmt );

    ideLog::log(IDE_SD_17,"[SHARD_META] executeLocalSQL Success: %s", aSQLBuf);

    return IDE_SUCCESS;

    IDE_EXCEPTION( no_meta_exist )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_SHARD_META_NOT_CREATED ) );
    }
    IDE_EXCEPTION( BUF_OUTPUT_ERROR );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,"qdsd::executeLocalSQL", "Internal SQL does not exist." ) );
    }
    IDE_EXCEPTION( ERR_NOT_ENOUGH_BUFSIZE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,"qdsd::executeLocalSQL", "The length of Internal SQL is too long." ) );
    }

    IDE_EXCEPTION_END;
    ideLog::log(IDE_SD_1,"[SHARD_META_ERROR] executeLocalSQL Failure: %s", aSQLBuf);
    IDE_PUSH();
    switch ( sState )
    {
    case 2:
        (void)sSmiStmt.end(SMI_STATEMENT_RESULT_FAILURE);
    case 1:
        (void) sOldStmt->begin( aStatement->mStatistics,
                                spRootStmt,
                                sSmiStmtFlag );
        
        qcg::setSmiStmt( aStatement, sOldStmt );
    default:
        break;
    }
    IDE_POP();
    return IDE_FAILURE;
}

IDE_RC qdsd::executeLocalSQLWithSession( qcStatement         * aStatement,
                                         qciStmtType           aSQLBufStmtType,
                                         SChar               * aSQLBuf,
                                         UInt                  aSQLBufSize,
                                         const SChar         * aSQLFormat,
                                         ... )
{
    SInt    sWrittenSize = 0;
    va_list sAp;
    vSLong              sRowCnt;
    sdiLocalMetaInfo    sLocalMetaInfo;
    smiStatement      * sOldStmt = NULL;
    smiStatement        sSmiStmt;
    UInt                sSmiStmtFlag = SMI_STATEMENT_NORMAL|SMI_STATEMENT_ALL_CURSOR;
    SInt                sState = 0;
    smiStatement* spRootStmt;
    
    IDE_TEST( sdi::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );
    IDE_TEST_RAISE( sLocalMetaInfo.mNodeName[0] == '\0', no_meta_exist );
    
    if ( aSQLFormat != NULL )
    {
        va_start(sAp, aSQLFormat);
        
        sWrittenSize = idlOS::vsnprintf( aSQLBuf, 
                                         aSQLBufSize,
                                         aSQLFormat,
                                         sAp );
        va_end(sAp);
        
        IDE_TEST_RAISE(sWrittenSize < 0, BUF_OUTPUT_ERROR);
        IDE_TEST_RAISE(sWrittenSize >= (SInt)aSQLBufSize, ERR_NOT_ENOUGH_BUFSIZE );
    }

    ideLog::log(IDE_SD_17,"[SHARD_META] executeLocalSQLWithSession Begin: %s", aSQLBuf);
    qcg::getSmiStmt( aStatement, &sOldStmt );
    spRootStmt = sOldStmt->getTrans()->getStatement();
    IDE_TEST( sOldStmt->end(SMI_STATEMENT_RESULT_SUCCESS) != IDE_SUCCESS );

    qcg::setSmiStmt( aStatement, &sSmiStmt );
    sState = 1;

    IDE_TEST( sSmiStmt.begin( aStatement->mStatistics,
                              spRootStmt,
                              sSmiStmtFlag )
              != IDE_SUCCESS );
    sState = 2;

    switch (qciMisc::getStmtType(aSQLBufStmtType))
    {
        case QCI_STMT_MASK_DDL:
            /* R2HA 2 qcstatement -> 1 smstatement problem , implicit savepoint problem */
            IDE_TEST( qciMisc::runRollbackableInternalDDL( aStatement,
                                                           QC_SMI_STMT( aStatement ),
                                                           QCG_GET_SESSION_USER_ID( aStatement ),
                                                           aSQLBuf )
                      != IDE_SUCCESS );
            break;
        case QCI_STMT_MASK_SP:
        case QCI_STMT_MASK_DML:
            IDE_TEST( qcg::runSQLforShardMeta( QC_SMI_STMT( aStatement ),
                                               aSQLBuf,
                                               & sRowCnt,
                                               aStatement->session ) 
                      != IDE_SUCCESS );
            break;
        case QCI_STMT_MASK_DCL:
            IDE_TEST( qciMisc::runDCLforInternal( aStatement,
                                                  aSQLBuf,
                                                  aStatement->session->mMmSession )
                      != IDE_SUCCESS );
            break;
        case QCI_STMT_MASK_DB:
        default:
            IDE_DASSERT(0);
            break;
    }
    sState = 1;
    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    
    IDE_TEST( sOldStmt->begin( aStatement->mStatistics,
                               spRootStmt,
                               sSmiStmtFlag )
                != IDE_SUCCESS );
    sState = 0;
    qcg::setSmiStmt( aStatement, sOldStmt );

    ideLog::log(IDE_SD_17,"[SHARD_META] executeLocalSQLWithSession Success: %s", aSQLBuf);

    return IDE_SUCCESS;

    IDE_EXCEPTION( no_meta_exist )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_SHARD_META_NOT_CREATED ) );
    }
    IDE_EXCEPTION( BUF_OUTPUT_ERROR );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,"qdsd::executeLocalSQLWithSession", "Internal SQL does not exist." ) );
    }
    IDE_EXCEPTION( ERR_NOT_ENOUGH_BUFSIZE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,"qdsd::executeLocalSQLWithSession", "The length of Internal SQL is too long." ) );
    }

    IDE_EXCEPTION_END;
    ideLog::log(IDE_SD_1,"[SHARD_META_ERROR] executeLocalSQLWithSession Failure: %s", aSQLBuf);
    IDE_PUSH();
    switch ( sState )
    {
    case 2:
        (void)sSmiStmt.end(SMI_STATEMENT_RESULT_FAILURE);
    case 1:
        (void) sOldStmt->begin( aStatement->mStatistics,
                                spRootStmt,
                                sSmiStmtFlag );
        
        qcg::setSmiStmt( aStatement, sOldStmt );
    default:
        break;
    }
    IDE_POP();
    return IDE_FAILURE;
}

IDE_RC qdsd::executeRemoteSQL( qcStatement         * aStatement,
                               SChar               * aNodeName,
                               sdiInternalOperation  aInternalOP,
                               qciStmtType           aSQLBufStmtType,
                               SChar               * aSQLBuf,
                               UInt                  aSQLBufSize,
                               const SChar         * aSQLFormat,
                               ... )
{
    SInt    sWrittenSize = 0;
    va_list sAp;
    UInt    sExecCount;
    vSLong              sRowCnt;
    sdiLocalMetaInfo    sLocalMetaInfo;
    smiStatement      * sOldStmt = NULL;
    smiStatement        sSmiStmt;
    UInt                sSmiStmtFlag = SMI_STATEMENT_NORMAL|SMI_STATEMENT_ALL_CURSOR;
    SInt                sState = 0;
    smiStatement      * spRootStmt = NULL;
    idBool              sIsMyNode = ID_FALSE;

    IDE_TEST( sdi::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );
    IDE_TEST_RAISE( sLocalMetaInfo.mNodeName[0] == '\0', no_meta_exist );
    IDE_TEST_RAISE( SDI_IS_NULL_NAME(sLocalMetaInfo.mNodeName) == ID_TRUE , no_meta_exist );

    if ( aSQLFormat != NULL )
    {
        va_start(sAp, aSQLFormat);

        sWrittenSize = idlOS::vsnprintf( aSQLBuf, 
                                         aSQLBufSize,
                                         aSQLFormat,
                                         sAp );
        va_end(sAp);

        IDE_TEST_RAISE(sWrittenSize < 0, BUF_OUTPUT_ERROR);
        IDE_TEST_RAISE(sWrittenSize >= (SInt)aSQLBufSize, ERR_NOT_ENOUGH_BUFSIZE );
    }

    sIsMyNode = sdi::isSameNode(sLocalMetaInfo.mNodeName, aNodeName);

    if ( sIsMyNode == ID_TRUE )
    {
        ideLog::log(IDE_SD_17,
                    "[SHARD_META] [Node: %s] executeReomteSQL for local: %s",
                    aNodeName,
                    aSQLBuf);
        /* it's me */
        qcg::getSmiStmt( aStatement, &sOldStmt );
        spRootStmt = sOldStmt->getTrans()->getStatement();
        IDE_TEST( sOldStmt->end(SMI_STATEMENT_RESULT_SUCCESS) != IDE_SUCCESS );

        qcg::setSmiStmt( aStatement, &sSmiStmt );
        sState = 1;

        IDE_TEST( sSmiStmt.begin( aStatement->mStatistics,
                                    spRootStmt,
                                    sSmiStmtFlag )
                    != IDE_SUCCESS );
        sState = 2;

        switch (qciMisc::getStmtType(aSQLBufStmtType))
        {
            case QCI_STMT_MASK_DDL:
                /* R2HA 2 qcstatement -> 1 smstatement problem , implicit savepoint problem */
                IDE_TEST( qciMisc::runRollbackableInternalDDL( aStatement,
                                                               QC_SMI_STMT( aStatement ),
                                                               QCG_GET_SESSION_USER_ID( aStatement ),
                                                               aSQLBuf )
                          != IDE_SUCCESS );
                break;
            case QCI_STMT_MASK_SP:                            
                if (( aStatement->session->mQPSpecific.mFlag & QC_SESSION_SAHRD_ADD_CLONE_MASK ) ==
                    QC_SESSION_SAHRD_ADD_CLONE_TRUE )
                {
                    sState = 1;
                    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );

                    IDE_TEST( qcg::runSQLforShardMeta( QC_SMI_STMT( aStatement ),
                                                       aSQLBuf,
                                                       & sRowCnt,
                                                       NULL ) //aStatement->session ) 
                              != IDE_SUCCESS );

                    IDE_TEST( sSmiStmt.begin( aStatement->mStatistics,
                                              spRootStmt,
                                              sSmiStmtFlag )
                              != IDE_SUCCESS );
                    sState = 2;
                }
                else
                {
                    /* Local이라도 InternalOp가 필요해서 ShardExecDirect 사용 */
                    sState = 1;
                    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
                    IDE_TEST( sdi::shardExecDirect( aStatement,
                                                    aNodeName,
                                                    (SChar*)aSQLBuf,
                                                    (UInt) idlOS::strlen( aSQLBuf ),
                                                    aInternalOP,
                                                    &sExecCount,
                                                    NULL,
                                                    NULL,
                                                    0,
                                                    NULL )
                              != IDE_SUCCESS );

                    if ( aInternalOP <= SDI_INTERNAL_OP_NORMAL ) 
                    {
                        IDE_TEST_RAISE( sExecCount != 1, ERR_REMOTE_EXECUTION );
                    }
                    IDE_TEST( sSmiStmt.begin( aStatement->mStatistics,
                                              spRootStmt,
                                              sSmiStmtFlag )
                              != IDE_SUCCESS );
                    sState = 2;
                }
                break;
            case QCI_STMT_MASK_DCL:
                if (( aStatement->session->mQPSpecific.mFlag & QC_SESSION_SAHRD_ADD_CLONE_MASK ) ==
                    QC_SESSION_SAHRD_ADD_CLONE_TRUE )
                {
                    sState = 1;
                    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );

                    IDE_TEST( qciMisc::runDCLforInternal( aStatement,
                                                          aSQLBuf,
                                                          aStatement->session->mMmSession )
                              != IDE_SUCCESS);

                    IDE_TEST( sSmiStmt.begin( aStatement->mStatistics,
                                              spRootStmt,
                                              sSmiStmtFlag )
                              != IDE_SUCCESS );
                    sState = 2;
                }
                else
                {
                    /* Local이라도 InternalOp가 필요해서 ShardExecDirect 사용 */
                    sState = 1;
                    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
                    IDE_TEST( sdi::shardExecDirect( aStatement,
                                                    aNodeName,
                                                    (SChar*)aSQLBuf,
                                                    (UInt) idlOS::strlen( aSQLBuf ),
                                                    aInternalOP,
                                                    &sExecCount,
                                                    NULL,
                                                    NULL,
                                                    0,
                                                    NULL )
                              != IDE_SUCCESS );
                    IDE_TEST_RAISE( sExecCount != 1, ERR_REMOTE_EXECUTION );
                    IDE_TEST( sSmiStmt.begin( aStatement->mStatistics,
                                              spRootStmt,
                                              sSmiStmtFlag )
                              != IDE_SUCCESS );
                    sState = 2;
                }
                break;
            case QCI_STMT_MASK_DML:
                IDE_TEST(qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                            aSQLBuf,
                                            & sRowCnt ) != IDE_SUCCESS);

                break;
            case QCI_STMT_MASK_DB:
            default:
                IDE_DASSERT(0);
                break;
        }
        sState = 1;
        IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );

        IDE_TEST( sOldStmt->begin( aStatement->mStatistics,
                                    spRootStmt,
                                    sSmiStmtFlag )
                    != IDE_SUCCESS );
        sState = 0;
        qcg::setSmiStmt( aStatement, sOldStmt );
    }
    else
    {
        ideLog::log(IDE_SD_17,
                    "[SHARD_META] [Node: %s] executeRemoteSQL: %s",
                    aNodeName,
                    aSQLBuf);
        IDE_TEST( sdi::shardExecDirect( aStatement,
                                        aNodeName,
                                        (SChar*)aSQLBuf,
                                        (UInt) idlOS::strlen( aSQLBuf ),
                                        aInternalOP,
                                        &sExecCount,
                                        NULL,
                                        NULL,
                                        0,
                                        NULL )
                    != IDE_SUCCESS );
        if ( aInternalOP <= SDI_INTERNAL_OP_NORMAL ) 
        {
            IDE_TEST_RAISE( sExecCount != 1 , ERR_REMOTE_EXECUTION );
        }
    }

    ideLog::log(IDE_SD_17,"[SHARD_META] executeRemoteSQL Success: %s", aSQLBuf);

    return IDE_SUCCESS;

    IDE_EXCEPTION( no_meta_exist )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_SHARD_META_NOT_CREATED ) );
    }
    IDE_EXCEPTION( BUF_OUTPUT_ERROR );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,"qdsd::executeRemoteSQL", "Internal SQL does not exist." ) );
    }
    IDE_EXCEPTION( ERR_NOT_ENOUGH_BUFSIZE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,"qdsd::executeRemoteSQL", "The length of Internal SQL is too long." ) );
    }
    IDE_EXCEPTION( ERR_REMOTE_EXECUTION );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_EXECUTE_REMOTE_SQL_FAILED, aSQLBuf ) );
    }
    IDE_EXCEPTION_END;
    ideLog::log(IDE_SD_1,"[SHARD_META_ERROR] executeRemoteSQL Failure: %s", aSQLBuf);
    IDE_PUSH();
    switch ( sState )
    {
        case 2:
            (void)sSmiStmt.end(SMI_STATEMENT_RESULT_FAILURE);
        case 1:
            (void) sOldStmt->begin( aStatement->mStatistics,
                                    spRootStmt,
                                    sSmiStmtFlag );
            
            qcg::setSmiStmt( aStatement, sOldStmt );
        default:
            break;
    }
    IDE_POP();
    return IDE_FAILURE;
}

IDE_RC qdsd::executeRemoteSQLWithNewTransArg( qcStatement         * aStatement,
                                              SChar               * aNodeName,
                                              sdiInternalOperation  /*aInternalOP*/,
                                              qciStmtType           /*aSQLBufStmtType*/,
                                              SChar               * aSQLBuf,
                                              UInt                  aSQLBufSize,
                                              idBool                aIsDDLTimeOut,
                                              const SChar         * aSQLFormat,
                                              ... )
{
    SInt    sWrittenSize = 0;
    va_list sAp;

    if ( aSQLFormat != NULL )
    {
        va_start(sAp, aSQLFormat);

        sWrittenSize = idlOS::vsnprintf( aSQLBuf,
                                         aSQLBufSize,
                                         aSQLFormat,
                                         sAp );
        va_end(sAp);

        IDE_TEST_RAISE(sWrittenSize < 0, BUF_OUTPUT_ERROR);
        IDE_TEST_RAISE(sWrittenSize >= (SInt)aSQLBufSize, ERR_NOT_ENOUGH_BUFSIZE );
    }

    IDE_TEST( executeRemoteSQLWithNewTrans(aStatement,
                                           aNodeName,
                                           aSQLBuf,
                                           aIsDDLTimeOut )
              != IDE_SUCCESS );

    ideLog::log(IDE_SD_17,"[SHARD_META] executeRemoteSQLWithNewTransArg Success: %s", aSQLBuf);

    return IDE_SUCCESS;

    IDE_EXCEPTION( BUF_OUTPUT_ERROR );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,"qdsd::executeRemoteSQLWithNewTransArg", "Internal SQL does not exist." ) );
    }
    IDE_EXCEPTION( ERR_NOT_ENOUGH_BUFSIZE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,"qdsd::executeRemoteSQLWithNewTransArg", "The length of Internal SQL is too long." ) );
    }
    IDE_EXCEPTION_END;

    ideLog::log(IDE_SD_1,"[SHARD_META_ERROR] executeRemoteSQLWithNewTransArg Failure: %s", aSQLBuf);

    return IDE_FAILURE;
}

IDE_RC qdsd::lockAllRemoteReshardTables( qcStatement * aStatement,
                                         SChar * aSQLBuf,
                                         UInt    aSQLBufSize,
                                         SChar * aFromNodeName,
                                         SChar * aToNodeName,
                                         qdReShardAttribute * aReShardAttr )
{
    UInt sShardDDLLockTryCount = QCG_GET_SESSION_SHARD_DDL_LOCK_TRY_COUNT(aStatement);
    UInt sRemoteLockTimeout = QCG_GET_SESSION_SHARD_DDL_LOCK_TIMEOUT(aStatement);
    UInt  sLoopCnt = 0;
    idBool sIsCancel = ID_FALSE;
    idBool sIsLockAcquired = ID_FALSE;
    qdReShardAttribute * sReShardAttr = NULL;
    SChar sUserName[QC_MAX_OBJECT_NAME_LEN] = {0,};
    SChar sTableName[QC_MAX_OBJECT_NAME_LEN] = {0,};
    SChar sPartitionName[QC_MAX_OBJECT_NAME_LEN] = {0,};

    /* the reomte cancel is not running. therefore remote lock timeout property sec(shard_ddl_lock_timeout).
     * if a application lock order is A->B->C then executing reshard table should be in the order A, B and C 
     * if A table is locked then the next table lock may will be acquired.
     */
    sLoopCnt = 0;
    while ( (sLoopCnt++ < sShardDDLLockTryCount) && (sIsLockAcquired != ID_TRUE ) )
    {
        sIsCancel = ID_FALSE;
        sIsLockAcquired = ID_FALSE;

        if ( idlOS::strncmp( aFromNodeName,
                             SDI_NODE_DEALLOC_NAME,
                             SDI_NODE_NAME_MAX_SIZE ) != 0 )
        {
            IDE_TEST( executeRemoteSQL( aStatement, 
                                        aFromNodeName,
                                        SDI_INTERNAL_OP_NORMAL,
                                        QCI_STMT_MASK_DCL,
                                        aSQLBuf,
                                        aSQLBufSize,
                                        "SAVEPOINT __RESHARD_SVP0") != IDE_SUCCESS);
        }

        if ( SDI_IS_NULL_NAME(aToNodeName) != ID_TRUE )
        {
            IDE_TEST( executeRemoteSQL( aStatement, 
                                        aToNodeName,
                                        SDI_INTERNAL_OP_NORMAL,
                                        QCI_STMT_MASK_DCL,
                                        aSQLBuf,
                                        aSQLBufSize,
                                        "SAVEPOINT __RESHARD_SVP0") != IDE_SUCCESS);
        }

        for ( sReShardAttr = aReShardAttr;
              sReShardAttr != NULL;
              sReShardAttr = sReShardAttr->next )
        {
            if ( sReShardAttr->mObjectType == 'T' )
            {
                IDE_DASSERT(QC_IS_NULL_NAME(sReShardAttr->mUserName) != ID_TRUE); // the username was set at validation time.
                QC_STR_COPY(sUserName, sReShardAttr->mUserName);
                QC_STR_COPY(sTableName, sReShardAttr->mObjectName);
                if ( QC_IS_NULL_NAME(sReShardAttr->mPartitionName) != ID_TRUE )
                {
                    QC_STR_COPY(sPartitionName, sReShardAttr->mPartitionName);
                    /* acquire table lock to fromnode */
                    if ( idlOS::strncmp( aFromNodeName,
                                         SDI_NODE_DEALLOC_NAME,
                                         SDI_NODE_NAME_MAX_SIZE ) != 0 )
                    {
                        if( executeRemoteSQL( aStatement,
                                              aFromNodeName,
                                              SDI_INTERNAL_OP_NORMAL,
                                              QCI_STMT_MASK_DML,
                                              aSQLBuf,
                                              aSQLBufSize,
                                              "LOCK TABLE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" IN EXCLUSIVE MODE WAIT %"ID_UINT32_FMT,
                                              //"LOCK TABLE %s.%s PARTITION (%s) IN EXCLUSIVE MODE WAIT %"ID_UINT32_FMT,
                                              sUserName,
                                              sTableName,
                                              //sPartitionName,
                                              sRemoteLockTimeout
                                            ) != IDE_SUCCESS )
                        {
                            /* cancel retry */
                            sIsCancel = ID_TRUE;
                            break;
                        }
                    }
                    if ( SDI_IS_NULL_NAME(aToNodeName) != ID_TRUE )
                    {
                        /* acquire table lock to tonode*/
                        if( executeRemoteSQL( aStatement,
                                              aToNodeName,
                                              SDI_INTERNAL_OP_NORMAL,
                                              QCI_STMT_MASK_DCL,
                                              aSQLBuf,
                                              aSQLBufSize,
                                              "LOCK TABLE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" IN EXCLUSIVE MODE WAIT %"ID_UINT32_FMT,
                                              //"LOCK TABLE %s.%s PARTITION (%s) IN EXCLUSIVE MODE WAIT %"ID_UINT32_FMT,
                                              sUserName,
                                              sTableName,
                                              //sPartitionName,
                                              sRemoteLockTimeout ) != IDE_SUCCESS )
                        {
                            /* cancel retry */
                            sIsCancel = ID_TRUE;
                            break;
                        }
                    }
                }
                else /* PartitionName IS NULL : solo table */
                {
                    if ( idlOS::strncmp( aFromNodeName,
                                         SDI_NODE_DEALLOC_NAME,
                                         SDI_NODE_NAME_MAX_SIZE ) != 0 )
                    {
                        /* acquire table lock to fromnode */
                        if( executeRemoteSQL( aStatement,
                                              aFromNodeName,
                                              SDI_INTERNAL_OP_NORMAL,
                                              QCI_STMT_MASK_DML,
                                              aSQLBuf,
                                              aSQLBufSize,
                                              "LOCK TABLE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" IN EXCLUSIVE MODE WAIT %"ID_UINT32_FMT,
                                              sUserName,
                                              sTableName,
                                              sRemoteLockTimeout
                                            ) != IDE_SUCCESS )
                        {
                            /* cancel retry */
                            sIsCancel = ID_TRUE;
                            break;
                        }
                    }
                    if ( SDI_IS_NULL_NAME(aToNodeName) != ID_TRUE )
                    {
                        /* acquire table lock to tonode*/
                        if( executeRemoteSQL( aStatement,
                                              aToNodeName,
                                              SDI_INTERNAL_OP_NORMAL,
                                              QCI_STMT_MASK_DML,
                                              aSQLBuf,
                                              aSQLBufSize,
                                              "LOCK TABLE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" IN EXCLUSIVE MODE WAIT %"ID_UINT32_FMT,
                                              sUserName,
                                              sTableName,
                                              sRemoteLockTimeout ) != IDE_SUCCESS )
                        {
                            /* cancel retry */

                            sIsCancel = ID_TRUE;
                            break;
                        }
                    }
                }
            }
            else
            {
                /* 'P' procedure: R2HA lock procedure %s.%s IN EXCLUSIVE MODE WAIT %"ID_UINT32_FMT */
            }
        }
        if ( sIsCancel == ID_TRUE )
        {
            if ( idlOS::strncmp( aFromNodeName,
                                 SDI_NODE_DEALLOC_NAME,
                                 SDI_NODE_NAME_MAX_SIZE ) != 0 )
            {
                IDE_TEST( executeRemoteSQL( aStatement, 
                                            aFromNodeName,
                                            SDI_INTERNAL_OP_NORMAL,
                                            QCI_STMT_MASK_DCL,
                                            aSQLBuf,
                                            aSQLBufSize,
                                            "ROLLBACK TO SAVEPOINT __RESHARD_SVP0") != IDE_SUCCESS );
            }
            if ( SDI_IS_NULL_NAME(aToNodeName) != ID_TRUE )
            {
                IDE_TEST( executeRemoteSQL( aStatement, 
                                            aToNodeName,
                                            SDI_INTERNAL_OP_NORMAL,
                                            QCI_STMT_MASK_DCL,
                                            aSQLBuf,
                                            aSQLBufSize,
                                            "ROLLBACK TO SAVEPOINT __RESHARD_SVP0") != IDE_SUCCESS );
            }
            sIsLockAcquired = ID_FALSE;
        }
        else
        {
            sIsLockAcquired = ID_TRUE;
            break;
        }
    }
    IDE_TEST_RAISE(sIsLockAcquired == ID_FALSE, ERR_LOCK);

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_LOCK );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_LOCK_FAIL ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    if ( idlOS::strncmp( aFromNodeName,
                         SDI_NODE_DEALLOC_NAME,
                         SDI_NODE_NAME_MAX_SIZE ) != 0 )
    {
        (void)executeRemoteSQL( aStatement, 
                                aFromNodeName,
                                SDI_INTERNAL_OP_NORMAL,
                                QCI_STMT_MASK_DCL,
                                aSQLBuf,
                                aSQLBufSize,
                                "ROLLBACK TO SAVEPOINT __RESHARD_SVP0" );
    }
    if ( SDI_IS_NULL_NAME(aToNodeName) != ID_TRUE )
    {
        (void)executeRemoteSQL( aStatement, 
                                aToNodeName,
                                SDI_INTERNAL_OP_NORMAL,
                                QCI_STMT_MASK_DCL,
                                aSQLBuf,
                                aSQLBufSize,
                                "ROLLBACK TO SAVEPOINT __RESHARD_SVP0" );
    }
    IDE_POP();
    return IDE_FAILURE;
}


IDE_RC qdsd::resetShardMetaAndSwapPartition( qcStatement * aStatement,
                                             SChar * aSQLBuf,
                                             UInt    aSQLBufSize,
                                             SChar * aFromNodeName,
                                             SChar * aToNodeName,
                                             iduList * aNodeInfoList,
                                             qdReShardAttribute * aReShardAttr )
{
    qdReShardAttribute * sReShardAttr = NULL;
    SChar                sUserName[QC_MAX_OBJECT_NAME_LEN] = {0,};
    SChar                sTableName[QC_MAX_OBJECT_NAME_LEN] = {0,};
    SChar                sPartitionName[QC_MAX_OBJECT_NAME_LEN] = {0,};
    sdiLocalMetaInfo   * sNodeInfo = NULL;
    iduListNode        * sNode = NULL;
    iduListNode        * sDummy = NULL;
    sdiShardObjectType    sShardTableType = SDI_NON_SHARD_OBJECT;
    SChar                sProcKeyValue[SDI_RANGE_VARCHAR_MAX_SIZE + 1];
    UInt                 sIsDefault = 0;
    SChar                sBufferNameStr[QC_MAX_OBJECT_NAME_LEN + 1];
    
    for ( sReShardAttr = aReShardAttr;
          sReShardAttr != NULL;
          sReShardAttr = sReShardAttr->next )
    {
        IDE_DASSERT(QC_IS_NULL_NAME(sReShardAttr->mUserName) != ID_TRUE); // the username was set at validation time.
        IDE_DASSERT(sReShardAttr->mShardObjectInfo != NULL);
        QC_STR_COPY(sUserName, sReShardAttr->mUserName);
        QC_STR_COPY(sTableName, sReShardAttr->mObjectName);

        if ( sReShardAttr->mObjectType == 'T' )
        {
            idlOS::snprintf( sBufferNameStr,
                             QC_MAX_OBJECT_NAME_LEN + 1,
                             "%s%s",
                             SDI_BACKUP_TABLE_PREFIX,
                             sTableName );
            
            sShardTableType = sdi::getShardObjectType(&sReShardAttr->mShardObjectInfo->mTableInfo );
            IDE_DASSERT( IDU_LIST_IS_EMPTY( aNodeInfoList ) != ID_TRUE );
            switch ( sShardTableType )
            {
                case SDI_SINGLE_SHARD_KEY_DIST_OBJECT:
                    IDE_DASSERT( QC_IS_NULL_NAME(sReShardAttr->mPartitionName) != ID_TRUE );
                    QC_STR_COPY(sPartitionName, sReShardAttr->mPartitionName);

                    IDU_LIST_ITERATE_SAFE( aNodeInfoList, sNode, sDummy )
                    {
                        sNodeInfo = (sdiLocalMetaInfo *)sNode->mObj;
                        /* EXEC dbms_shard.reset_shard_resident_node('SYS','L1','NODE3','NODE4','300'); */
                        // reset_shard_resident_node can not modify default node therefore do not use it.

                        if ( SDI_IS_NULL_NAME(aToNodeName) != ID_TRUE )
                        {
                            /* EXEC DBMS_SHARD.RESET_SHARD_PARTITION_NODE('SYS','@{aTable}', 'NODE3', 'NODE1', 'P4'); */
                            IDE_TEST( executeRemoteSQL( aStatement,
                                                        sNodeInfo->mNodeName,
                                                        SDI_INTERNAL_OP_NORMAL,
                                                        QCI_STMT_MASK_SP,
                                                        aSQLBuf,
                                                        aSQLBufSize,
                                                        "EXEC dbms_shard.reset_shard_partition_node('"
                                                        QCM_SQL_STRING_SKIP_FMT"','"
                                                        QCM_SQL_STRING_SKIP_FMT"','"
                                                        QCM_SQL_STRING_SKIP_FMT"','"
                                                        QCM_SQL_STRING_SKIP_FMT"',"
                                                        QCM_SQL_VARCHAR_FMT")",
                                                        sUserName,
                                                        sTableName,
                                                        aFromNodeName,
                                                        aToNodeName,
                                                        sPartitionName ) != IDE_SUCCESS );
                        }
                        else
                        {
                            /* EXEC DBMS_SHARD.RESET_SHARD_PARTITION_NODE('SYS','@{aTable}', 'NODE3', '', 'P4'); */
                            IDE_TEST( executeRemoteSQL( aStatement,
                                                        sNodeInfo->mNodeName,
                                                        SDI_INTERNAL_OP_NORMAL,
                                                        QCI_STMT_MASK_SP,
                                                        aSQLBuf,
                                                        aSQLBufSize,
                                                        "EXEC dbms_shard.reset_shard_partition_node('"
                                                        QCM_SQL_STRING_SKIP_FMT"','"
                                                        QCM_SQL_STRING_SKIP_FMT"','"
                                                        QCM_SQL_STRING_SKIP_FMT"','',"
                                                        QCM_SQL_VARCHAR_FMT")",
                                                        sUserName,
                                                        sTableName,
                                                        aFromNodeName,
                                                        sPartitionName ) != IDE_SUCCESS );
                        }
                    }

                    if ( idlOS::strncmp( aFromNodeName,
                                         SDI_NODE_DEALLOC_NAME,
                                         SDI_NODE_NAME_MAX_SIZE + 1 ) != 0 )
                    {
                        IDE_TEST( executeRemoteSQL( aStatement,
                                                    aFromNodeName,
                                                    SDI_INTERNAL_OP_NORMAL,
                                                    QCI_STMT_MASK_DDL,
                                                    aSQLBuf,
                                                    aSQLBufSize,
                                                    "ALTER TABLE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" REPLACE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" PARTITION "QCM_SQL_STRING_SKIP_FMT"",
                                                    sUserName,
                                                    sBufferNameStr,
                                                    sUserName,
                                                    sTableName,
                                                    sPartitionName ) != IDE_SUCCESS );
                    }
                    if ( SDI_IS_NULL_NAME(aToNodeName) != ID_TRUE )
                    {
                        /* ToNode: ALTER TABLE L1_REBUILD REPLACE L1 PARTITION P41 */
                        IDE_TEST( executeRemoteSQL( aStatement,
                                                    aToNodeName,
                                                    SDI_INTERNAL_OP_NORMAL,
                                                    QCI_STMT_MASK_DDL,
                                                    aSQLBuf,
                                                    aSQLBufSize,
                                                    "ALTER TABLE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" REPLACE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" PARTITION "QCM_SQL_STRING_SKIP_FMT"",
                                                    sUserName,
                                                    sBufferNameStr,
                                                    sUserName,
                                                    sTableName,
                                                    sPartitionName ) != IDE_SUCCESS );
                    }
                    break;

                case SDI_SOLO_DIST_OBJECT:
                    IDU_LIST_ITERATE_SAFE( aNodeInfoList, sNode, sDummy )
                    {
                        sNodeInfo = (sdiLocalMetaInfo *)sNode->mObj;
                        if ( SDI_IS_NULL_NAME(aToNodeName) != ID_TRUE )
                        {
                            /* EXEC DBMS_SHARD.RESET_SHARD_RESIDENT_NODE('SYS','@{aTable}', 'NODE3', 'NODE1'); */
                            IDE_TEST( executeRemoteSQL( aStatement,
                                                        sNodeInfo->mNodeName,
                                                        SDI_INTERNAL_OP_NORMAL,
                                                        QCI_STMT_MASK_SP,
                                                        aSQLBuf,
                                                        aSQLBufSize,
                                                        "EXEC dbms_shard.reset_shard_resident_node('"QCM_SQL_STRING_SKIP_FMT"','"QCM_SQL_STRING_SKIP_FMT"','"QCM_SQL_STRING_SKIP_FMT"','"QCM_SQL_STRING_SKIP_FMT"')",
                                                        sUserName,
                                                        sTableName,
                                                        aFromNodeName,
                                                        aToNodeName) != IDE_SUCCESS );
                        }
                        else
                        {
                            /* EXEC DBMS_SHARD.RESET_SHARD_RESIDENT_NODE('SYS','@{aTable}', 'NODE3', ''); */
                            IDE_TEST( executeRemoteSQL( aStatement,
                                                        sNodeInfo->mNodeName,
                                                        SDI_INTERNAL_OP_NORMAL,
                                                        QCI_STMT_MASK_SP,
                                                        aSQLBuf,
                                                        aSQLBufSize,
                                                        "EXEC dbms_shard.reset_shard_resident_node('"QCM_SQL_STRING_SKIP_FMT"','"QCM_SQL_STRING_SKIP_FMT"','"QCM_SQL_STRING_SKIP_FMT"','')",
                                                        sUserName,
                                                        sTableName,
                                                        aFromNodeName ) != IDE_SUCCESS );
                        }
                    }

                    if ( idlOS::strncmp( aFromNodeName,
                                         SDI_NODE_DEALLOC_NAME,
                                         SDI_NODE_NAME_MAX_SIZE + 1 ) != 0 )
                    {
                        IDE_TEST( executeRemoteSQL( aStatement,
                                                    aFromNodeName,
                                                    SDI_INTERNAL_OP_NORMAL,
                                                    QCI_STMT_MASK_DDL,
                                                    aSQLBuf,
                                                    aSQLBufSize,
                                                    "ALTER TABLE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" REPLACE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT"",
                                                    sUserName,
                                                    sBufferNameStr,
                                                    sUserName,
                                                    sTableName) != IDE_SUCCESS );
                    }
                    if ( SDI_IS_NULL_NAME(aToNodeName) != ID_TRUE )
                    {
                        /* ToNode: ALTER TABLE S1_REBUILD REPLACE S1 */
                        IDE_TEST( executeRemoteSQL( aStatement,
                                                    aToNodeName,
                                                    SDI_INTERNAL_OP_NORMAL,
                                                    QCI_STMT_MASK_DDL,
                                                    aSQLBuf,
                                                    aSQLBufSize,
                                                    "ALTER TABLE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" REPLACE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT"",
                                                    sUserName,
                                                    sBufferNameStr,
                                                    sUserName,
                                                    sTableName) != IDE_SUCCESS );
                    }
                    break;

                case SDI_CLONE_DIST_OBJECT:
                    IDU_LIST_ITERATE_SAFE( aNodeInfoList, sNode, sDummy )
                    {
                        sNodeInfo = (sdiLocalMetaInfo *)sNode->mObj;
                        if ( SDI_IS_NULL_NAME(aToNodeName) != ID_TRUE )
                        {
                            /* TASK-7307 DML Data Consistency in Shard
                             *   dbms_shard.set_shard_clone_local 함수에 4번째 인자 추가해서 DDL에서 호출됨을 표시 */
                            /* EXEC DBMS_SHARD.SET_SHARD_CLONE_LOCAL('SYS','@{aTable}', 'NODE4', 1); */
                            IDE_TEST( executeRemoteSQL( aStatement,
                                                        sNodeInfo->mNodeName,
                                                        SDI_INTERNAL_OP_NORMAL,
                                                        QCI_STMT_MASK_SP,
                                                        aSQLBuf,
                                                        aSQLBufSize,
                                                        "EXEC dbms_shard.set_shard_clone_local('"
                                                        QCM_SQL_STRING_SKIP_FMT"','"
                                                        QCM_SQL_STRING_SKIP_FMT"','"
                                                        QCM_SQL_STRING_SKIP_FMT"', "
                                                        "NULL, "QCM_SQL_UINT32_FMT")", // TASK-7307
                                                        sUserName,
                                                        sTableName,
                                                        aToNodeName,
                                                        SDM_CALLED_BY_SHARD_MOVE ) /* TASK-7307 */
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            /* EXEC DBMS_SHARD.RESET_SHARD_RESIDENT_NODE('SYS','@{aTable}', 'NODE3', ''); */
                            IDE_TEST( executeRemoteSQL( aStatement,
                                                        sNodeInfo->mNodeName,
                                                        SDI_INTERNAL_OP_NORMAL,
                                                        QCI_STMT_MASK_SP,
                                                        aSQLBuf,
                                                        aSQLBufSize,
                                                        "EXEC dbms_shard.reset_shard_resident_node('"QCM_SQL_STRING_SKIP_FMT"','"QCM_SQL_STRING_SKIP_FMT"','"QCM_SQL_STRING_SKIP_FMT"','')",
                                                        sUserName,
                                                        sTableName,
                                                        aFromNodeName ) != IDE_SUCCESS );
                        }
                    }

                    if ( idlOS::strncmp( aFromNodeName,
                                         SDI_NODE_DEALLOC_NAME,
                                         SDI_NODE_NAME_MAX_SIZE + 1 ) != 0 )
                    {
                        IDE_TEST( executeRemoteSQL( aStatement, 
                                                    aFromNodeName,
                                                    SDI_INTERNAL_OP_NORMAL,
                                                    QCI_STMT_MASK_DDL,
                                                    aSQLBuf,
                                                    aSQLBufSize,
                                                    "ALTER TABLE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" TOUCH",
                                                    sUserName, 
                                                    sTableName ) != IDE_SUCCESS );
                    }
                    if ( SDI_IS_NULL_NAME(aToNodeName) != ID_TRUE )
                    {
                        /* ToNode: ALTER TABLE S1_REBUILD REPLACE S1 */
                        IDE_TEST( executeRemoteSQL( aStatement,
                                                    aToNodeName,
                                                    SDI_INTERNAL_OP_NORMAL,
                                                    QCI_STMT_MASK_DDL,
                                                    aSQLBuf,
                                                    aSQLBufSize,
                                                    "ALTER TABLE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" REPLACE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT"",
                                                    sUserName,
                                                    sBufferNameStr,
                                                    sUserName,
                                                    sTableName) != IDE_SUCCESS );
                    }
                    break;
                case SDI_NON_SHARD_OBJECT:
                case SDI_COMPOSITE_SHARD_KEY_DIST_OBJECT:
                default:
                    IDE_DASSERT(0);
                    break;
            }
        }
        else
        {
            /* 'P' procedure */
            sShardTableType = sdi::getShardObjectType(&sReShardAttr->mShardObjectInfo->mTableInfo );
            switch (sShardTableType)
            {
                case SDI_SINGLE_SHARD_KEY_DIST_OBJECT:
                    IDE_DASSERT( sReShardAttr->mKeyValue != NULL );
                    IDE_TEST( getProcedureKeyValue( sReShardAttr,
                                                    sProcKeyValue ) != IDE_SUCCESS );
                    IDE_DASSERT( IDU_LIST_IS_EMPTY( aNodeInfoList ) != ID_TRUE );
                    if(sProcKeyValue[0] == '\0')
                    {
                        sIsDefault = 1;
                    }
                    IDU_LIST_ITERATE_SAFE( aNodeInfoList, sNode, sDummy )
                    {
                        sNodeInfo = (sdiLocalMetaInfo *)sNode->mObj;
                        if ( SDI_IS_NULL_NAME(aToNodeName) != ID_TRUE )
                        {
                            /* EXEC DBMS_SHARD.RESET_SHARD_RESIDENT_NODE('SYS','@{Procedure}', 'NODE3', 'NODE1','300'); */
                            IDE_TEST( executeRemoteSQL( aStatement,
                                                        sNodeInfo->mNodeName,
                                                        SDI_INTERNAL_OP_NORMAL,
                                                        QCI_STMT_MASK_SP,
                                                        aSQLBuf,
                                                        aSQLBufSize,
                                                        "EXEC dbms_shard.reset_shard_resident_node("
                                                        "'"QCM_SQL_STRING_SKIP_FMT"','"QCM_SQL_STRING_SKIP_FMT"','"QCM_SQL_STRING_SKIP_FMT"','"QCM_SQL_STRING_SKIP_FMT"',"QCM_SQL_VARCHAR_FMT",'',INTEGER'%"ID_INT32_FMT"')",
                                                        sUserName,
                                                        sTableName,
                                                        aFromNodeName,
                                                        aToNodeName,
                                                        sProcKeyValue,
                                                        sIsDefault) != IDE_SUCCESS );
                        }
                        else
                        {
                            /* EXEC DBMS_SHARD.RESET_SHARD_RESIDENT_NODE('SYS','@{Procedure}', 'NODE3', '','300'); */
                            IDE_TEST( executeRemoteSQL( aStatement,
                                                        sNodeInfo->mNodeName,
                                                        SDI_INTERNAL_OP_NORMAL,
                                                        QCI_STMT_MASK_SP,
                                                        aSQLBuf,
                                                        aSQLBufSize,
                                                        "EXEC dbms_shard.reset_shard_resident_node('"QCM_SQL_STRING_SKIP_FMT"','"QCM_SQL_STRING_SKIP_FMT"','"QCM_SQL_STRING_SKIP_FMT"','',"QCM_SQL_VARCHAR_FMT" )",
                                                        sUserName,
                                                        sTableName,
                                                        aFromNodeName,
                                                        sProcKeyValue) != IDE_SUCCESS );
                        }
                        /* R2HA: FromNode: ALTER PROCEDURE PROC1 TOUCH
                        IDE_TEST( executeRemoteSQL( aStatement,
                                                    aFromNodeName,
                                                    SDI_INTERNAL_OP_NORMAL,
                                                    QCI_STMT_MASK_DDL,
                                                    aSQLBuf,
                                                    aSQLBufSize,
                                                    "ALTER TABLE %s.%s TOUCH",
                                                    sUserName,
                                                    sTableName ) != IDE_SUCCESS );
                        if ( SDI_IS_NULL_NAME(aToNodeName) != ID_TRUE )
                        {
                            IDE_TEST( executeRemoteSQL( aStatement,
                                                        aToNodeName,
                                                        SDI_INTERNAL_OP_NORMAL,
                                                        QCI_STMT_MASK_DDL,
                                                        aSQLBuf,
                                                        aSQLBufSize,
                                                        "ALTER TABLE %s.%s TOUCH",
                                                        sUserName,
                                                        sTableName ) != IDE_SUCCESS );
                        }
                        */
                    }
                    break;
                case SDI_SOLO_DIST_OBJECT:
                    sProcKeyValue[0] = '\0';
                    sIsDefault = 0;
                    IDU_LIST_ITERATE_SAFE( aNodeInfoList, sNode, sDummy )
                    {
                        sNodeInfo = (sdiLocalMetaInfo *)sNode->mObj;
                        if ( SDI_IS_NULL_NAME(aToNodeName) != ID_TRUE )
                        {
                            /* EXEC DBMS_SHARD.RESET_SHARD_RESIDENT_NODE('SYS','@{Procedure}', 'NODE3', 'NODE1','300'); */
                            IDE_TEST( executeRemoteSQL( aStatement,
                                                        sNodeInfo->mNodeName,
                                                        SDI_INTERNAL_OP_NORMAL,
                                                        QCI_STMT_MASK_SP,
                                                        aSQLBuf,
                                                        aSQLBufSize,
                                                        "EXEC dbms_shard.reset_shard_resident_node("
                                                        "'"QCM_SQL_STRING_SKIP_FMT"','"QCM_SQL_STRING_SKIP_FMT"','"QCM_SQL_STRING_SKIP_FMT"','"QCM_SQL_STRING_SKIP_FMT"','','',INTEGER'%"ID_INT32_FMT"')",
                                                        sUserName,
                                                        sTableName,
                                                        aFromNodeName,
                                                        aToNodeName,
                                                        sIsDefault) != IDE_SUCCESS );
                        }
                        else
                        {
                            /* EXEC DBMS_SHARD.RESET_SHARD_RESIDENT_NODE('SYS','@{Procedure}', 'NODE3', '','300'); */
                            IDE_TEST( executeRemoteSQL( aStatement,
                                                        sNodeInfo->mNodeName,
                                                        SDI_INTERNAL_OP_NORMAL,
                                                        QCI_STMT_MASK_SP,
                                                        aSQLBuf,
                                                        aSQLBufSize,
                                                        "EXEC dbms_shard.reset_shard_resident_node('"QCM_SQL_STRING_SKIP_FMT"','"QCM_SQL_STRING_SKIP_FMT"','"QCM_SQL_STRING_SKIP_FMT"','','')",
                                                        sUserName,
                                                        sTableName,
                                                        aFromNodeName ) != IDE_SUCCESS );
                        }
                        /* R2HA: FromNode: ALTER PROCEDURE PROC1 TOUCH
                        IDE_TEST( executeRemoteSQL( aStatement,
                                                    aFromNodeName,
                                                    SDI_INTERNAL_OP_NORMAL,
                                                    QCI_STMT_MASK_DDL,
                                                    aSQLBuf,
                                                    aSQLBufSize,
                                                    "ALTER TABLE %s.%s TOUCH",
                                                    sUserName,
                                                    sTableName ) != IDE_SUCCESS );
                        if ( SDI_IS_NULL_NAME(aToNodeName) != ID_TRUE )
                        {
                            IDE_TEST( executeRemoteSQL( aStatement,
                                                        aToNodeName,
                                                        SDI_INTERNAL_OP_NORMAL,
                                                        QCI_STMT_MASK_DDL,
                                                        aSQLBuf,
                                                        aSQLBufSize,
                                                        "ALTER TABLE %s.%s TOUCH",
                                                        sUserName,
                                                        sTableName ) != IDE_SUCCESS );
                        }
                        */
                    }
                    break;
                case SDI_CLONE_DIST_OBJECT:
                    sProcKeyValue[0] = '\0';
                    sIsDefault = 0;
                    IDU_LIST_ITERATE_SAFE( aNodeInfoList, sNode, sDummy )
                    {
                        sNodeInfo = (sdiLocalMetaInfo *)sNode->mObj;
                        if ( SDI_IS_NULL_NAME(aToNodeName) != ID_TRUE )
                        {
                            /* EXEC DBMS_SHARD.SET_SHARD_CLONE_LOCAL('SYS','@{Procedure}', 'NODE3'); */
                            IDE_TEST( executeRemoteSQL( aStatement,
                                                        sNodeInfo->mNodeName,
                                                        SDI_INTERNAL_OP_NORMAL,
                                                        QCI_STMT_MASK_SP,
                                                        aSQLBuf,
                                                        aSQLBufSize,
                                                        "EXEC dbms_shard.set_shard_clone_local('"
                                                        QCM_SQL_STRING_SKIP_FMT"','"
                                                        QCM_SQL_STRING_SKIP_FMT"','"
                                                        QCM_SQL_STRING_SKIP_FMT"')",
                                                        sUserName,
                                                        sTableName,
                                                        aToNodeName ) != IDE_SUCCESS );
                        }
                        else
                        {
                            /* EXEC DBMS_SHARD.RESET_SHARD_RESIDENT_NODE('SYS','@{Procedure}', 'NODE3', '','300'); */
                            IDE_TEST( executeRemoteSQL( aStatement,
                                                        sNodeInfo->mNodeName,
                                                        SDI_INTERNAL_OP_NORMAL,
                                                        QCI_STMT_MASK_SP,
                                                        aSQLBuf,
                                                        aSQLBufSize,
                                                        "EXEC dbms_shard.reset_shard_resident_node('"
                                                        QCM_SQL_STRING_SKIP_FMT"','"
                                                        QCM_SQL_STRING_SKIP_FMT"','"
                                                        QCM_SQL_STRING_SKIP_FMT"','','')",
                                                        sUserName,
                                                        sTableName,
                                                        aFromNodeName ) != IDE_SUCCESS );
                        }
                        /* R2HA: FromNode: ALTER PROCEDURE PROC1 TOUCH
                        IDE_TEST( executeRemoteSQL( aStatement,
                                                    aFromNodeName,
                                                    SDI_INTERNAL_OP_NORMAL,
                                                    QCI_STMT_MASK_DDL,
                                                    aSQLBuf,
                                                    aSQLBufSize,
                                                    "ALTER TABLE %s.%s TOUCH",
                                                    sUserName,
                                                    sTableName ) != IDE_SUCCESS );
                        if ( SDI_IS_NULL_NAME(aToNodeName) != ID_TRUE )
                        {
                            IDE_TEST( executeRemoteSQL( aStatement,
                                                        aToNodeName,
                                                        SDI_INTERNAL_OP_NORMAL,
                                                        QCI_STMT_MASK_DDL,
                                                        aSQLBuf,
                                                        aSQLBufSize,
                                                        "ALTER TABLE %s.%s TOUCH",
                                                        sUserName,
                                                        sTableName ) != IDE_SUCCESS );
                        }
                        */
                    }
                    break;
                case SDI_NON_SHARD_OBJECT:
                case SDI_COMPOSITE_SHARD_KEY_DIST_OBJECT:
                default:
                    IDE_DASSERT(0);
                    break;
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdsd::moveReplication( qcStatement * aStatement,
                              SChar       * aSQLBuf,
                              UInt          aSQLBufSize,
                              UInt          aKSafety,
                              sdiReplicaSet  * aFromReplicaSet,
                              sdiReplicaSet  * aToReplicaSet,
                              qdReShardAttribute * aReShardAttr )
{
    qdReShardAttribute * sReShardAttr = NULL;
    SChar                sUserName[QC_MAX_OBJECT_NAME_LEN] = {0,};
    SChar                sTableName[QC_MAX_OBJECT_NAME_LEN] = {0,};
    SChar                sPartitionName[QC_MAX_OBJECT_NAME_LEN] = {0,};
    sdiShardObjectType    sShardTableType = SDI_NON_SHARD_OBJECT;
    SChar              * sBeforeFromNodeName[3];
    SChar              * sBeforeToNodeName[3];
    SChar              * sBeforeRepName[3];
    SChar              * sAfterFromNodeName[3];
    SChar              * sAfterToNodeName[3];
    SChar              * sAfterRepName[3];
    UInt                 k = 1;
    qdShardParseTree  * sParseTree = NULL;
    SChar               sBufferNameStr[QC_MAX_OBJECT_NAME_LEN + 1];
    
    IDE_TEST_RAISE( aKSafety < 1, ERR_KSAFETY );

    sParseTree = (qdShardParseTree *)aStatement->myPlan->parseTree;
    sShardTableType = sdi::getShardObjectType( &aReShardAttr->mShardObjectInfo->mTableInfo );
    IDE_TEST_CONT(sShardTableType == SDI_CLONE_DIST_OBJECT, NORMAL_EXIT);

    sBeforeFromNodeName[0] = SDM_NA_STR;
    sBeforeFromNodeName[1] = aFromReplicaSet->mFirstReplFromNodeName;
    sBeforeFromNodeName[2] = aFromReplicaSet->mSecondReplFromNodeName;

    sBeforeToNodeName[0] = SDM_NA_STR;
    sBeforeToNodeName[1] = aFromReplicaSet->mFirstReplToNodeName;
    sBeforeToNodeName[2] = aFromReplicaSet->mSecondReplToNodeName;

    sBeforeRepName[0] = SDM_NA_STR;
    sBeforeRepName[1] = aFromReplicaSet->mFirstReplName;
    sBeforeRepName[2] = aFromReplicaSet->mSecondReplName;

    sAfterFromNodeName[0] = SDM_NA_STR;
    sAfterFromNodeName[1] = aToReplicaSet->mFirstReplFromNodeName;
    sAfterFromNodeName[2] = aToReplicaSet->mSecondReplFromNodeName;

    sAfterToNodeName[0] = SDM_NA_STR;
    sAfterToNodeName[1] = aToReplicaSet->mFirstReplToNodeName;
    sAfterToNodeName[2] = aToReplicaSet->mSecondReplToNodeName;

    sAfterRepName[0] = SDM_NA_STR;
    sAfterRepName[1] = aToReplicaSet->mFirstReplName;
    sAfterRepName[2] = aToReplicaSet->mSecondReplName;

    for ( sReShardAttr = aReShardAttr;
          sReShardAttr != NULL;
          sReShardAttr = sReShardAttr->next )
    {
        IDE_DASSERT(QC_IS_NULL_NAME(sReShardAttr->mUserName) != ID_TRUE); // the username was set at validation time.
        IDE_DASSERT(sReShardAttr->mShardObjectInfo != NULL); // the shardobjectinfo was set at validation time.

        if ( sReShardAttr->mObjectType == 'T' )
        {
            QC_STR_COPY(sUserName, sReShardAttr->mUserName);
            QC_STR_COPY(sTableName, sReShardAttr->mObjectName);

            sShardTableType = sdi::getShardObjectType(&sReShardAttr->mShardObjectInfo->mTableInfo );

            idlOS::snprintf( sBufferNameStr,
                             QC_MAX_OBJECT_NAME_LEN + 1,
                             "%s%s",
                             SDI_BACKUP_TABLE_PREFIX,
                             sTableName );
                        
            switch (sShardTableType)
            {
                case SDI_SINGLE_SHARD_KEY_DIST_OBJECT:
                    QC_STR_COPY(sPartitionName, sReShardAttr->mPartitionName);
                    IDE_DASSERT( QC_IS_NULL_NAME(sReShardAttr->mPartitionName) != ID_TRUE );
                    for (k = 1; k <= aKSafety; k++ )
                    {
                        // BUG-28625
                        if (( sParseTree->mNodeCount == 2 ) && ( k == 2 ))
                        {
                            break;
                        }

                        /* DEALLOC은 FROM이 없다. */
                        if ( aFromReplicaSet->mReplicaSetId != SDI_REPLICASET_NULL_ID )
                        {
                            /* This function should only be called when there is something to be moved. */
                            IDE_TEST_RAISE( SDI_IS_NULL_NAME(sBeforeFromNodeName[k]) == ID_TRUE, ERR_REPLICA_SET );
                            IDE_TEST( executeRemoteSQL( aStatement,
                                                        sBeforeFromNodeName[k],
                                                        SDI_INTERNAL_OP_NORMAL,
                                                        QCI_STMT_MASK_DDL,
                                                        aSQLBuf,
                                                        aSQLBufSize,
                                                        "alter replication "QCM_SQL_STRING_SKIP_FMT" drop table from "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" partition "QCM_SQL_STRING_SKIP_FMT" "
                                                        "to "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" partition "QCM_SQL_STRING_SKIP_FMT"",
                                                        sBeforeRepName[k],
                                                        sUserName,
                                                        sTableName,
                                                        sPartitionName,
                                                        sUserName,
                                                        sBufferNameStr,
                                                        sPartitionName )
                                        != IDE_SUCCESS );

                            IDE_TEST_RAISE( SDI_IS_NULL_NAME(sBeforeToNodeName[k]) == ID_TRUE, ERR_REPLICA_SET );
                            IDE_TEST( executeRemoteSQL( aStatement,
                                                        sBeforeToNodeName[k],
                                                        SDI_INTERNAL_OP_NORMAL,
                                                        QCI_STMT_MASK_DDL,
                                                        aSQLBuf,
                                                        aSQLBufSize,
                                                        "alter replication "QCM_SQL_STRING_SKIP_FMT" drop table from "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" partition "QCM_SQL_STRING_SKIP_FMT" "
                                                        "to "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" partition "QCM_SQL_STRING_SKIP_FMT"",
                                                        sBeforeRepName[k],
                                                        sUserName,
                                                        sBufferNameStr,
                                                        sPartitionName,
                                                        sUserName,
                                                        sTableName,
                                                        sPartitionName )
                                        != IDE_SUCCESS );

                        }

                        if ( SDI_IS_NULL_NAME(sAfterFromNodeName[k]) != ID_TRUE ) /* toreplicaset has N/A in REMOVE syntax */
                        {
                            IDE_TEST( executeRemoteSQL( aStatement,
                                                        sAfterFromNodeName[k],
                                                        SDI_INTERNAL_OP_NORMAL,
                                                        QCI_STMT_MASK_DDL,
                                                        aSQLBuf,
                                                        aSQLBufSize,
                                                        "alter replication "QCM_SQL_STRING_SKIP_FMT" add table from "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" partition "QCM_SQL_STRING_SKIP_FMT" "
                                                        "to "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" partition "QCM_SQL_STRING_SKIP_FMT"",
                                                        sAfterRepName[k],
                                                        sUserName,
                                                        sTableName,
                                                        sPartitionName,
                                                        sUserName,
                                                        sBufferNameStr,
                                                        sPartitionName )
                                        != IDE_SUCCESS );

                            IDE_TEST_RAISE( SDI_IS_NULL_NAME(sAfterToNodeName[k]) == ID_TRUE, ERR_REPLICA_SET );
                            IDE_TEST( executeRemoteSQL( aStatement,
                                                        sAfterToNodeName[k],
                                                        SDI_INTERNAL_OP_NORMAL,
                                                        QCI_STMT_MASK_DDL,
                                                        aSQLBuf,
                                                        aSQLBufSize,
                                                        "alter replication "QCM_SQL_STRING_SKIP_FMT" add table from "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" partition "QCM_SQL_STRING_SKIP_FMT" "
                                                        "to "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" partition "QCM_SQL_STRING_SKIP_FMT"",
                                                        sAfterRepName[k],
                                                        sUserName,
                                                        sBufferNameStr,
                                                        sPartitionName,
                                                        sUserName,
                                                        sTableName,
                                                        sPartitionName )
                                        != IDE_SUCCESS );
                        }
                    }
                    break;
                case SDI_SOLO_DIST_OBJECT:
                    for (k = 1; k <= aKSafety; k++ )
                    {
                        /* DEALLOC은 FROM이 없다. */
                        if ( aFromReplicaSet->mReplicaSetId != SDI_REPLICASET_NULL_ID )
                        {

                            /* This function should only be called when there is something to be moved. */
                            IDE_TEST_RAISE( SDI_IS_NULL_NAME(sBeforeFromNodeName[k]) == ID_TRUE, ERR_REPLICA_SET );
                            IDE_TEST( executeRemoteSQL( aStatement,
                                                        sBeforeFromNodeName[k],
                                                        SDI_INTERNAL_OP_NORMAL,
                                                        QCI_STMT_MASK_DDL,
                                                        aSQLBuf,
                                                        aSQLBufSize,
                                                        "alter replication "QCM_SQL_STRING_SKIP_FMT" drop table from "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" "
                                                        "to "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT"",
                                                        sBeforeRepName[k],
                                                        sUserName,
                                                        sTableName,
                                                        sUserName,
                                                        sBufferNameStr )
                                        != IDE_SUCCESS );

                            IDE_TEST_RAISE( SDI_IS_NULL_NAME(sBeforeToNodeName[k]) == ID_TRUE, ERR_REPLICA_SET );
                            IDE_TEST( executeRemoteSQL( aStatement,
                                                        sBeforeToNodeName[k],
                                                        SDI_INTERNAL_OP_NORMAL,
                                                        QCI_STMT_MASK_DDL,
                                                        aSQLBuf,
                                                        aSQLBufSize,
                                                        "alter replication "QCM_SQL_STRING_SKIP_FMT" drop table from "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" "
                                                        "to "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT"",
                                                        sBeforeRepName[k],
                                                        sUserName,
                                                        sBufferNameStr,
                                                        sUserName,
                                                        sTableName ) //check
                                        != IDE_SUCCESS );

                        }

                        if( SDI_IS_NULL_NAME(sAfterFromNodeName[k]) != ID_TRUE )
                        {
                            IDE_TEST( executeRemoteSQL( aStatement,
                                                        sAfterFromNodeName[k],
                                                        SDI_INTERNAL_OP_NORMAL,
                                                        QCI_STMT_MASK_DDL,
                                                        aSQLBuf,
                                                        aSQLBufSize,
                                                        "alter replication "QCM_SQL_STRING_SKIP_FMT" add table from "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" "
                                                        "to "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT"",
                                                        sAfterRepName[k],
                                                        sUserName,
                                                        sTableName,
                                                        sUserName,
                                                        sBufferNameStr )
                                        != IDE_SUCCESS );

                            IDE_TEST_RAISE( SDI_IS_NULL_NAME(sAfterToNodeName[k]) == ID_TRUE, ERR_REPLICA_SET );
                            IDE_TEST( executeRemoteSQL( aStatement,
                                                        sAfterToNodeName[k],
                                                        SDI_INTERNAL_OP_NORMAL,
                                                        QCI_STMT_MASK_DDL,
                                                        aSQLBuf,
                                                        aSQLBufSize,
                                                        "alter replication "QCM_SQL_STRING_SKIP_FMT" add table from "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" "
                                                        "to "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT"",
                                                        sAfterRepName[k],
                                                        sUserName,
                                                        sBufferNameStr,
                                                        sUserName,
                                                        sTableName )
                                    != IDE_SUCCESS );
                        }
                    }
                    break;
                case SDI_CLONE_DIST_OBJECT:
                    break;
                case SDI_NON_SHARD_OBJECT:
                case SDI_COMPOSITE_SHARD_KEY_DIST_OBJECT:
                default:
                    IDE_DASSERT(0);
                    break;
            }
        }
        else
        {
            /* 'P' procedure: do noting */
        }
    }
    IDE_EXCEPTION_CONT(NORMAL_EXIT);
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_REPLICA_SET );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_INVALID_REPLICA_SET_INFO ) );
    }
    IDE_EXCEPTION( ERR_KSAFETY );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_INVALID_KSAFETY ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdsd::truncateAllBackupData( qcStatement * aStatement,
                                    SChar * aSQLBuf,
                                    UInt    aSQLBufSize,
                                    SChar * aNodeName,
                                    qdReShardAttribute * aReShardAttr,
                                    ZKPendingJobType  aPendingJobType )
{
    qdReShardAttribute * sReShardAttr = NULL;
    SChar                sUserName[QC_MAX_OBJECT_NAME_LEN] = {0,};
    SChar                sTableName[QC_MAX_OBJECT_NAME_LEN] = {0,};
    SChar                sPartitionName[QC_MAX_OBJECT_NAME_LEN] = {0,};
    sdiShardObjectType   sShardTableType = SDI_NON_SHARD_OBJECT;
    SChar                sBufferNameStr[QC_MAX_OBJECT_NAME_LEN + 1];
    
    for ( sReShardAttr = aReShardAttr;
          sReShardAttr != NULL;
          sReShardAttr = sReShardAttr->next )
    {
        IDE_DASSERT(QC_IS_NULL_NAME(sReShardAttr->mUserName) != ID_TRUE);
        IDE_DASSERT(sReShardAttr->mShardObjectInfo != NULL);
        if(sReShardAttr->mObjectType == 'T')
        {
            QC_STR_COPY(sUserName, sReShardAttr->mUserName);
            QC_STR_COPY(sTableName, sReShardAttr->mObjectName);
            sShardTableType = sdi::getShardObjectType(&sReShardAttr->mShardObjectInfo->mTableInfo );

            idlOS::snprintf( sBufferNameStr,
                             QC_MAX_OBJECT_NAME_LEN + 1,
                             "%s%s",
                             SDI_BACKUP_TABLE_PREFIX,
                             sTableName );

            switch (sShardTableType)
            {
                case SDI_SINGLE_SHARD_KEY_DIST_OBJECT:
                    IDE_DASSERT( QC_IS_NULL_NAME(sReShardAttr->mPartitionName) != ID_TRUE );
                    QC_STR_COPY(sPartitionName, sReShardAttr->mPartitionName);
                    if ( aPendingJobType == ZK_PENDING_JOB_NONE )
                    {
                        /* no pending job, immediately execute to remote */
                        IDE_TEST( executeRemoteSQL( aStatement,
                                                    aNodeName,
                                                    SDI_INTERNAL_OP_NORMAL,
                                                    QCI_STMT_MASK_DDL,
                                                    aSQLBuf,
                                                    aSQLBufSize,
                                                    "ALTER TABLE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" TRUNCATE PARTITION "QCM_SQL_STRING_SKIP_FMT"",
                                                    sUserName,
                                                    sBufferNameStr,
                                                    sPartitionName ) != IDE_SUCCESS );
                    }
                    else
                    {
                        idlOS::snprintf(aSQLBuf,
                                        aSQLBufSize,
                                        "ALTER TABLE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" TRUNCATE PARTITION "QCM_SQL_STRING_SKIP_FMT"",
                                        sUserName,
                                        sBufferNameStr,
                                        sPartitionName);
                        
                        sdiZookeeper::addPendingJob(aSQLBuf,
                                                    aNodeName,
                                                    aPendingJobType,
                                                    QCI_STMT_MASK_MAX );
                    }
                    break;
                case SDI_SOLO_DIST_OBJECT:
                case SDI_CLONE_DIST_OBJECT:
                    sShardTableType = sdi::getShardObjectType(&sReShardAttr->mShardObjectInfo->mTableInfo );

                    if ( aPendingJobType == ZK_PENDING_JOB_NONE )
                    {
                        /* no pending job, immediately execute to remote */
                        IDE_TEST( executeRemoteSQL( aStatement,
                                                    aNodeName,
                                                    SDI_INTERNAL_OP_NORMAL,
                                                    QCI_STMT_MASK_DDL,
                                                    aSQLBuf,
                                                    aSQLBufSize,
                                                    "TRUNCATE TABLE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT"",
                                                    sUserName,
                                                    sBufferNameStr ) != IDE_SUCCESS );
                    }
                    else
                    {
                        idlOS::snprintf(aSQLBuf,
                                        aSQLBufSize,
                                        "TRUNCATE TABLE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT"",
                                        sUserName,
                                        sBufferNameStr);
                        
                        sdiZookeeper::addPendingJob( aSQLBuf,
                                                     aNodeName,
                                                     aPendingJobType,
                                                     QCI_STMT_MASK_MAX );
                    }
                    break;
                case SDI_NON_SHARD_OBJECT:
                case SDI_COMPOSITE_SHARD_KEY_DIST_OBJECT:
                default:
                    IDE_DASSERT(0);
                    break;
            }
        }
        else
        {
            /* 'P' procedure: do noting */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdsd::truncateAllPrimaryData( qcStatement * aStatement,
                                    SChar * aSQLBuf,
                                    UInt    aSQLBufSize,
                                    SChar * aNodeName,
                                    qdReShardAttribute * aReShardAttr,
                                    ZKPendingJobType  aPendingJobType )
{
    qdReShardAttribute * sReShardAttr = NULL;
    SChar                sUserName[QC_MAX_OBJECT_NAME_LEN] = {0,};
    SChar                sTableName[QC_MAX_OBJECT_NAME_LEN] = {0,};
    SChar                sPartitionName[QC_MAX_OBJECT_NAME_LEN] = {0,};
    sdiShardObjectType   sShardTableType = SDI_NON_SHARD_OBJECT;
    SChar                sBufferNameStr[QC_MAX_OBJECT_NAME_LEN + 1];
    
    for ( sReShardAttr = aReShardAttr;
          sReShardAttr != NULL;
          sReShardAttr = sReShardAttr->next )
    {
        IDE_DASSERT(QC_IS_NULL_NAME(sReShardAttr->mUserName) != ID_TRUE);
        IDE_DASSERT(sReShardAttr->mShardObjectInfo != NULL);
        if(sReShardAttr->mObjectType == 'T')
        {
            QC_STR_COPY(sUserName, sReShardAttr->mUserName);
            QC_STR_COPY(sTableName, sReShardAttr->mObjectName);
            sShardTableType = sdi::getShardObjectType(&sReShardAttr->mShardObjectInfo->mTableInfo );

            idlOS::snprintf( sBufferNameStr,
                             QC_MAX_OBJECT_NAME_LEN + 1,
                             "%s",
                             sTableName );

            switch (sShardTableType)
            {
                case SDI_SINGLE_SHARD_KEY_DIST_OBJECT:
                    IDE_DASSERT( QC_IS_NULL_NAME(sReShardAttr->mPartitionName) != ID_TRUE );
                    QC_STR_COPY(sPartitionName, sReShardAttr->mPartitionName);
                    if ( aPendingJobType == ZK_PENDING_JOB_NONE )
                    {
                        /* no pending job, immediately execute to remote */
                        IDE_TEST( executeRemoteSQL( aStatement,
                                                    aNodeName,
                                                    SDI_INTERNAL_OP_NORMAL,
                                                    QCI_STMT_MASK_DDL,
                                                    aSQLBuf,
                                                    aSQLBufSize,
                                                    "ALTER TABLE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" TRUNCATE PARTITION "QCM_SQL_STRING_SKIP_FMT"",
                                                    sUserName,
                                                    sBufferNameStr,
                                                    sPartitionName ) != IDE_SUCCESS );
                    }
                    else
                    {
                        idlOS::snprintf(aSQLBuf,
                                        aSQLBufSize,
                                        "ALTER TABLE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" TRUNCATE PARTITION "QCM_SQL_STRING_SKIP_FMT"",
                                        sUserName,
                                        sBufferNameStr,
                                        sPartitionName);
                        
                        sdiZookeeper::addPendingJob(aSQLBuf,
                                                    aNodeName,
                                                    aPendingJobType,
                                                    QCI_STMT_MASK_MAX );
                    }
                    break;
                case SDI_SOLO_DIST_OBJECT:
                case SDI_CLONE_DIST_OBJECT:
                    sShardTableType = sdi::getShardObjectType(&sReShardAttr->mShardObjectInfo->mTableInfo );

                    if ( aPendingJobType == ZK_PENDING_JOB_NONE )
                    {
                        /* no pending job, immediately execute to remote */
                        IDE_TEST( executeRemoteSQL( aStatement,
                                                    aNodeName,
                                                    SDI_INTERNAL_OP_NORMAL,
                                                    QCI_STMT_MASK_DDL,
                                                    aSQLBuf,
                                                    aSQLBufSize,
                                                    "TRUNCATE TABLE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT"",
                                                    sUserName,
                                                    sBufferNameStr ) != IDE_SUCCESS );
                    }
                    else
                    {
                        idlOS::snprintf(aSQLBuf,
                                        aSQLBufSize,
                                        "TRUNCATE TABLE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT"",
                                        sUserName,
                                        sBufferNameStr);
                        
                        sdiZookeeper::addPendingJob( aSQLBuf,
                                                     aNodeName,
                                                     aPendingJobType,
                                                     QCI_STMT_MASK_MAX );
                    }
                    break;
                case SDI_NON_SHARD_OBJECT:
                case SDI_COMPOSITE_SHARD_KEY_DIST_OBJECT:
                default:
                    IDE_DASSERT(0);
                    break;
            }
        }
        else
        {
            /* 'P' procedure: do noting */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qdsd::isExistBackupDataForAllReshardAttr( qcStatement * aStatement,
                                                 SChar * aNodeName,
                                                 qdReShardAttribute * aReShardAttr,
                                                 idBool  * aIsExist )
{
    qdReShardAttribute * sReShardAttr = NULL;
    SChar                sUserName[QC_MAX_OBJECT_NAME_LEN] = {0,};
    SChar                sTableName[QC_MAX_OBJECT_NAME_LEN] = SDI_BACKUP_TABLE_PREFIX;
    SChar                sPartitionName[QC_MAX_OBJECT_NAME_LEN] = {0,};
    SChar              * sPartitionNamePtr = NULL;
    UInt                 sRowCount = 0;
    ULong                sTotalDataCount = 0;

    for ( sReShardAttr = aReShardAttr;
          sReShardAttr != NULL;
          sReShardAttr = sReShardAttr->next )
    {
        IDE_DASSERT(QC_IS_NULL_NAME(sReShardAttr->mUserName) != ID_TRUE);
        IDE_DASSERT(sReShardAttr->mShardObjectInfo != NULL);
        if(sReShardAttr->mObjectType == 'T')
        {
            QC_STR_COPY(sUserName, sReShardAttr->mUserName);
            QC_STR_COPY(sTableName + idlOS::strlen(SDI_BACKUP_TABLE_PREFIX), sReShardAttr->mObjectName);
            if ( QC_IS_NULL_NAME(sReShardAttr->mPartitionName) != ID_TRUE )
            {
                QC_STR_COPY(sPartitionName, sReShardAttr->mPartitionName);
                sPartitionNamePtr = sPartitionName;
            }
            else
            {
                sPartitionNamePtr = NULL;
            }

            IDE_TEST( sdi::getRemoteRowCount( aStatement,
                                              aNodeName,
                                              sUserName,
                                              sTableName,
                                              sPartitionNamePtr,
                                              SDI_INTERNAL_OP_NOT,
                                              &sRowCount ) != IDE_SUCCESS );
            sTotalDataCount += sRowCount;
        }
        else
        {
            /* 'P' procedure: do nothing */
        }
    }

    if ( sTotalDataCount > 0 )
    {
        *aIsExist = ID_TRUE;
    }
    else
    {
        *aIsExist = ID_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdsd::clearBackupDataWithNewTrans( void  * aOriMmSession,
                                          SChar * aNodeName,
                                          qdReShardAttribute * aReShardAttr )
{
    const UInt   sSqlLen = 1024;
    void        *sMmSession = NULL;
    QCD_HSTMT    sHstmt;
    qcStatement *sStatement = NULL;
    SChar        sSqlStr[sSqlLen] = {0,};
    SInt         sStage = 0;
    smiTrans     * sTrans = NULL;
    qciStmtType sStmtType = QCI_STMT_MASK_MAX;
    idBool        sRecordExist;
    smiStatement sSmiStmt;
    UInt sShardDDLLockTryCount = qci::mSessionCallback.mGetShardDDLLockTryCount(aOriMmSession);
    UInt sRemoteLockTimeout = qci::mSessionCallback.mGetShardDDLLockTimeout(aOriMmSession);
    UInt sSelectLockTimeout = (UInt)IDL_MIN((ULong)(sShardDDLLockTryCount * sRemoteLockTimeout), (ULong)ID_UINT_MAX);

    IDE_TEST( qci::mSessionCallback.mAllocInternalSession( &sMmSession, aOriMmSession ) != IDE_SUCCESS );
    IDE_TEST_RAISE( sMmSession == NULL, ERR_INVALID_CONDITION );
    sStage = 1;

    qci::mSessionCallback.mSetShardMetaNumber( sMmSession,
                                               sdi::getSMNForDataNode());

    sTrans     = qci::mSessionCallback.mGetTransWithBegin( sMmSession );
    IDE_ERROR_RAISE( sTrans != NULL, ERR_INVALID_CONDITION );
    IDE_TEST_RAISE( sTrans->isBegin() != ID_TRUE, ERR_INVALID_CONDITION );
    sStage = 2;

    IDE_TEST( qcd::allocStmtNoParent( sMmSession,
                                      ID_TRUE,
                                      & sHstmt )
              != IDE_SUCCESS );
    sStage = 3;

    IDE_TEST( qcd::getQcStmt(sHstmt, &sStatement) != IDE_SUCCESS );

    idlOS::snprintf(sSqlStr, sSqlLen, "ALTER SESSION SET TRANSACTIONAL_DDL = 1");
    IDE_TEST(qcg::runDCLforInternal(sStatement ,
                                    sSqlStr,
                                    sStatement->session->mMmSession ) != IDE_SUCCESS);

    idlOS::snprintf( sSqlStr, sSqlLen,
                     "ALTER SESSION SET QUERY_TIMEOUT = %"ID_INT32_FMT" ", sSelectLockTimeout);

    IDE_TEST( qciMisc::runDCLforInternal( sStatement,
                                          sSqlStr,
                                          sStatement->session->mMmSession ) != IDE_SUCCESS);

    /* the bellow sql do not use. it exists just for plan structure */
    idlOS::snprintf(sSqlStr, sSqlLen, "select 1 from dual");
    IDE_TEST( qcd::prepare( sHstmt,
                            NULL,
                            NULL,
                            & sStmtType,
                            sSqlStr,
                            idlOS::strlen(sSqlStr),
                            ID_TRUE )  // direct-execute mode
              != IDE_SUCCESS );

    IDE_TEST( sSmiStmt.begin( NULL,
                              sTrans->getStatement(),
                              SMI_STATEMENT_NORMAL |
                              SMI_STATEMENT_MEMORY_CURSOR )
              != IDE_SUCCESS );
    sStage = 4;
    qcg::setSmiStmt( sStatement, &sSmiStmt );
    /* execute statement related logic */
    IDE_TEST( sdi::checkShardLinkerWithoutSQL( sStatement,
                                               QCG_GET_SESSION_SHARD_META_NUMBER( sStatement ),
                                               sTrans )
              != IDE_SUCCESS );

    IDE_TEST( qdsd::isExistBackupDataForAllReshardAttr( sStatement,
                                                        aNodeName,
                                                        aReShardAttr,
                                                        &sRecordExist ) != IDE_SUCCESS );

    if ( sRecordExist == ID_TRUE )
    {
        IDE_TEST( qdsd::truncateAllBackupData( sStatement,
                                            sSqlStr,
                                            QD_MAX_SQL_LENGTH,
                                            aNodeName,
                                            aReShardAttr,
                                            ZK_PENDING_JOB_NONE) != IDE_SUCCESS );
    }
    IDE_TEST(sSmiStmt.end(SMI_STATEMENT_RESULT_SUCCESS) != IDE_SUCCESS);
    sStage = 3;


    sStage = 2;
    IDE_TEST( qcd::freeStmt( sHstmt,
                             ID_TRUE )  // free & drop
              != IDE_SUCCESS );

    sStage = 1;
    IDE_TEST_RAISE ( qci::mSessionCallback.mCommit( sMmSession,
                                                    ID_FALSE /* is stored proc*/ )
                     != IDE_SUCCESS, ERR_COMMIT );

    sStage = 0;
    qci::mSessionCallback.mFreeInternalSession( sMmSession,
                                                ID_TRUE /* aIsSuccess */ );
    return IDE_SUCCESS;
    IDE_EXCEPTION(ERR_INVALID_CONDITION)
    {
        IDE_SET( ideSetErrorCode(qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                "qdsd::clearBackupDataWithNewTrans",
                                "Invalid condition.") );
    }
    IDE_EXCEPTION( ERR_COMMIT );
    {
        ideLog::log( IDE_SD_1, "[FAILURE] error code 0x%05X %s",
                     E_ERROR_CODE(ideGetErrorCode()),
                     ideGetErrorMsg(ideGetErrorCode()));
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_TRANSACTION_COMMIT_ERROR ) ); 
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    switch ( sStage )
    {
        case 4:
            IDE_ASSERT(sSmiStmt.end(SMI_STATEMENT_RESULT_FAILURE) == IDE_SUCCESS);
        case 3:
            (void)qcd::freeStmt( sHstmt, ID_TRUE );
                /* fall through */
        case 2:
            (void)qci::mSessionCallback.mRollback( sMmSession, NULL/* savepoint */, ID_FALSE );
                /* fall through */
        case 1:
            (void)qci::mSessionCallback.mFreeInternalSession( sMmSession, ID_FALSE /* aIsSuccess */ );
                /* fall through */
            break;
        default:
                // Nothing to do.
            break;
    }
    IDE_POP();

    return IDE_FAILURE;
}


IDE_RC qdsd::executeSQLNodeList( qcStatement          * aStatement,
                                 sdiInternalOperation   aInternalOp,
                                 qciStmtType            aSQLStmtType,
                                 SChar                * aSQLBuf,
                                 SChar                * aExceptNodeName,
                                 iduList              * aList )
{
    iduListNode       * sIterator = NULL;
    sdiClientInfo     * sClientInfo  = NULL;

    IDE_TEST( sdi::checkShardLinker( aStatement ) != IDE_SUCCESS );

    sClientInfo = aStatement->session->mQPSpecific.mClientInfo;

    // 전체 Alive Node 대상으로 실행
    if ( sClientInfo != NULL )
    {
        IDU_LIST_ITERATE( aList, sIterator )
        {
            /* 특정 Node 제외 하고 실행 */
            if ( aExceptNodeName != NULL )
            {
                if ( idlOS::strncmp ( (SChar*)sIterator->mObj, 
                                      aExceptNodeName, 
                                      SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
                {
                    /* Failover 수행시 TargetNode에는 보낼 필요가 없다.
                     * 이미 Stop 시키고 넘어온거라 Alive 아니여야 하지만 Z-Meta S-Meta Lock을 모두 잡았기 때문에
                     * Alive로 나타날수 있음. -> 확인필요
                     * Stop 타이밍 조절이 필요한가? */
                    continue;
                }
            }

            IDE_TEST( executeRemoteSQL( aStatement, 
                                        (SChar*) sIterator->mObj,
                                        aInternalOp,
                                        aSQLStmtType,
                                        aSQLBuf,
                                        QD_MAX_SQL_LENGTH,
                                        NULL )
                        != IDE_SUCCESS );
        }
    }
    else
    {
        IDE_RAISE( ERR_NODE_NOT_EXIST );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NODE_NOT_EXIST )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_NODE_NOT_EXIST ) );
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC qdsd::resetShardMeta( qcStatement * aStatement )
{
    
    smiTrans       sTrans;
    smiStatement   sSmiStmt;
    smiStatement * sDummySmiStmt = NULL;
    SChar        * sSqlStr       = NULL;
    vSLong         sRowCnt       = 0;
    UInt           sStage        = 0;
    idBool         sIsSmiStmtEnd = ID_FALSE;
    smiStatement * spRootStmt    = QC_SMI_STMT(aStatement)->getTrans()->getStatement();
    UInt           sSmiStmtFlag  = QC_SMI_STMT(aStatement)->mFlag;

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      &sSqlStr )
              != IDE_SUCCESS );

    IDE_TEST( sTrans.initialize() != IDE_SUCCESS );
    sStage = 1;

    IDE_TEST( sTrans.begin( &sDummySmiStmt,
                            NULL,
                            ( SMI_ISOLATION_NO_PHANTOM     |
                              SMI_TRANSACTION_NORMAL       |
                              SMI_TRANSACTION_REPL_DEFAULT |
                              SMI_COMMIT_WRITE_NOWAIT ) )
              != IDE_SUCCESS );
    sStage = 2;

    IDE_TEST( sSmiStmt.begin( NULL,
                              sDummySmiStmt,
                              ( SMI_STATEMENT_NORMAL |
                                SMI_STATEMENT_MEMORY_CURSOR ) )
              != IDE_SUCCESS );
    sStage = 3;
    
    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_SHARD.NODES_ " );

    IDE_TEST( qciMisc::runDMLforDDL( &sSmiStmt,
                                     sSqlStr,
                                     &sRowCnt )
              != IDE_SUCCESS );

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_SHARD.OBJECTS_ " );

    IDE_TEST( qciMisc::runDMLforDDL( &sSmiStmt,
                                     sSqlStr,
                                     &sRowCnt )
              != IDE_SUCCESS );

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_SHARD.RANGES_ " );

    IDE_TEST( qciMisc::runDMLforDDL( &sSmiStmt,
                                     sSqlStr,
                                     &sRowCnt )
              != IDE_SUCCESS );

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_SHARD.CLONES_ " );

    IDE_TEST( qciMisc::runDMLforDDL( &sSmiStmt,
                                     sSqlStr,
                                     &sRowCnt )
              != IDE_SUCCESS );

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_SHARD.SOLOS_ " );

    IDE_TEST( qciMisc::runDMLforDDL( &sSmiStmt,
                                     sSqlStr,
                                     &sRowCnt )
              != IDE_SUCCESS );

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_SHARD.REPLICA_SETS_ " );

    IDE_TEST( qciMisc::runDMLforDDL( &sSmiStmt,
                                     sSqlStr,
                                     &sRowCnt )
              != IDE_SUCCESS );

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_SHARD.FAILOVER_HISTORY_ " );

    IDE_TEST( qciMisc::runDMLforDDL( &sSmiStmt,
                                     sSqlStr,
                                     &sRowCnt )
              != IDE_SUCCESS );

    // reset shard sequence
    IDE_TEST( resetShardSequence( &sSmiStmt,
                                  (SChar*)"NEXT_NODE_ID" )
              != IDE_SUCCESS );

    IDE_TEST( resetShardSequence( &sSmiStmt,
                                  (SChar*)"NEXT_SHARD_ID" )
              != IDE_SUCCESS );
    
    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS )
              != IDE_SUCCESS );
    sStage = 2;

    IDE_TEST( sTrans.commit() != IDE_SUCCESS );
    sStage = 1;

    sStage = 0;
    IDE_TEST( sTrans.destroy( NULL ) != IDE_SUCCESS );

    // commit된 다음 해당 정보를 보기 위해
    // statement end, begin 수행
    IDE_TEST( QC_SMI_STMT(aStatement)->end(SMI_STATEMENT_RESULT_SUCCESS) != IDE_SUCCESS );
    sIsSmiStmtEnd = ID_TRUE;

    IDE_TEST( QC_SMI_STMT(aStatement)->begin( aStatement->mStatistics,
                                              spRootStmt,
                                              sSmiStmtFlag )
              != IDE_SUCCESS );
    sIsSmiStmtEnd = ID_FALSE;    

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch ( sStage )
    {
        case 3:
            ( void )sSmiStmt.end( SMI_STATEMENT_RESULT_FAILURE );
        case 2:
            ( void )sTrans.rollback();
        case 1:
            ( void )sTrans.destroy( NULL );
        default:
            break;
    }
    
    if( sIsSmiStmtEnd == ID_TRUE )
    {
        IDE_ASSERT( QC_SMI_STMT(aStatement)->begin( aStatement->mStatistics,
                                                    spRootStmt,
                                                    SMI_STATEMENT_NORMAL|SMI_STATEMENT_ALL_CURSOR ) 
                    == IDE_SUCCESS);
    }

    return IDE_FAILURE;
}

IDE_RC qdsd::resetShardSequence( smiStatement * aSmiStmt,
                                 SChar        * aSeqName )
{
    qcmSequenceInfo   sSequenceInfo;
    void            * sSequenceHandle;
    SChar             sSqlStr[QD_MAX_SQL_LENGTH + 1];
    UInt              sUserID;
    vSLong            sRowCnt;
    SChar             sQueueSequenceName[QC_MAX_SEQ_NAME_LEN + 1];
    
    // zookeeper shard meta lock을 잡은 상태이므로 별도의 lock은 잡지 않는다.
    idlOS::snprintf( sQueueSequenceName,
                     ID_SIZEOF( sQueueSequenceName ),
                     "%s",
                     aSeqName );

    IDE_TEST_RAISE( qcmUser::getUserID( NULL,
                                        aSmiStmt,
                                        SDM_USER,
                                        idlOS::strlen(SDM_USER),
                                        & sUserID )
                    != IDE_SUCCESS, ERR_NOT_EXIST_USER );

    // Queue Sequence
    IDE_TEST( qcm::getSequenceInfoByName( aSmiStmt,
                                          sUserID,
                                          (UChar*) sQueueSequenceName,
                                          idlOS::strlen( sQueueSequenceName ),
                                          &sSequenceInfo,
                                          &sSequenceHandle )
              != IDE_SUCCESS );

    // sequence reset
    IDE_TEST( smiTable::resetSequence( aSmiStmt,
                                       sSequenceHandle )
              != IDE_SUCCESS );
    
    // tables_ 에 sequence udpate
    idlOS::snprintf( sSqlStr,
                     QD_MAX_SQL_LENGTH,
                     "UPDATE SYS_TABLES_ "
                     "SET USER_ID = INTEGER'%"ID_INT32_FMT"', "
                     "TABLE_NAME = '%s', "
                     "TABLE_OID = BIGINT'%"ID_INT64_FMT"', "
                     "COLUMN_COUNT = INTEGER'%"ID_INT32_FMT"', "
                     "PARALLEL_DEGREE = INTEGER'%"ID_INT32_FMT"', "
                     "LAST_DDL_TIME = SYSDATE "
                     "WHERE TABLE_ID = INTEGER'%"ID_INT32_FMT"'",
                     sUserID,
                     aSeqName,
                     (ULong)smiGetTableId( sSequenceHandle ),
                     0,
                     1,
                     sSequenceInfo.sequenceID );

    IDE_TEST( qciMisc::runDMLforDDL( aSmiStmt,
                                     sSqlStr,
                                     &sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sRowCnt == 0, ERR_META_CRASH );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_EXIST_USER )
    {
        IDE_SET( ideSetErrorCode(sdERR_ABORT_SDM_SHARD_META_NOT_CREATED,
                                 SDM_USER) );
    }
    IDE_EXCEPTION( ERR_META_CRASH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_META_CRASH ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdsd::lockTables4Failover( qcStatement        * aStatement,
                                  SChar              * aSQLBuf,
                                  UInt                 aSQLBufSize,
                                  SChar              * aFailoverNodeName,
                                  sdiTableInfoList   * aTableInfoList,
                                  sdiReplicaSetInfo  * aReplicaSetInfo,
                                  sdiNode            * aTargetNodeInfo,
                                  ULong                aSMN )
{
    UInt               sShardDDLLockTryCount = QCG_GET_SESSION_SHARD_DDL_LOCK_TRY_COUNT(aStatement);
    UInt               sRemoteLockTimeout = QCG_GET_SESSION_SHARD_DDL_LOCK_TIMEOUT(aStatement);
    UInt               sLoopCnt = 0;

    idBool             sIsCancel = ID_FALSE;
    idBool             sIsLockAcquired = ID_FALSE;
    idBool             sIsLocked = ID_FALSE;

    sdiTableInfoList * sTmpTableInfoList = NULL;
    sdiTableInfo     * sTableInfo = NULL;

    sdiRangeInfo       sRangeInfos;

    sdiShardObjectType  sShardTableType = SDI_NON_SHARD_OBJECT;
    
    UInt               j;
    SChar              sBufferNameStr[QC_MAX_OBJECT_NAME_LEN + 1];
    
    /* the reomte cancel is not running. therefore remote lock timeout property sec(shard_ddl_lock_timeout).
     * if a application lock order is A->B->C then executing reshard table should be in the order A, B and C 
     * if A table is locked then the next table lock may will be acquired.
     */
    sLoopCnt = 0;
    while ( (sLoopCnt++ < sShardDDLLockTryCount) && (sIsLockAcquired != ID_TRUE ) )
    {
        sIsCancel = ID_FALSE;
        sIsLockAcquired = ID_FALSE;

        IDE_TEST( executeRemoteSQL( aStatement, 
                                    aFailoverNodeName,
                                    SDI_INTERNAL_OP_FAILOVER,
                                    QCI_STMT_MASK_SP,
                                    aSQLBuf,
                                    aSQLBufSize,
                                    "SAVEPOINT __FAILOVER_SVP0") != IDE_SUCCESS);

        for ( sTmpTableInfoList = aTableInfoList;
              sTmpTableInfoList != NULL;
              sTmpTableInfoList = sTmpTableInfoList->mNext )
        {
            sIsLocked = ID_FALSE;
            sTableInfo = sTmpTableInfoList->mTableInfo;

            if ( sTableInfo->mObjectType == 'T' )
            {
                idlOS::snprintf( sBufferNameStr,
                                 QC_MAX_OBJECT_NAME_LEN + 1,
                                 "%s%s",
                                 SDI_BACKUP_TABLE_PREFIX,
                                 sTableInfo->mObjectName );

                /* Default Partition check */
                if ( sTableInfo->mDefaultNodeId == aTargetNodeInfo->mNodeId )
                {
                    if ( sdm::checkFailoverAvailable( aReplicaSetInfo,
                                                      sTableInfo->mDefaultPartitionReplicaSetId,
                                                      aFailoverNodeName ) == ID_TRUE )
                    {
                        /* Swap 대상이라면 Lock을 획득해야 한다. */
                        /* Default Partition Lock 획득 */
                        if( executeRemoteSQL( aStatement, 
                                              aFailoverNodeName,
                                              SDI_INTERNAL_OP_FAILOVER,
                                              QCI_STMT_MASK_DML,
                                              aSQLBuf,
                                              aSQLBufSize,
                                              "LOCK TABLE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" IN EXCLUSIVE MODE WAIT %"ID_UINT32_FMT,

//                                              "LOCK TABLE %s.%s PARTITION ( %s ) IN EXCLUSIVE MODE WAIT %"ID_UINT32_FMT,
                                              sTableInfo->mUserName,
                                              sTableInfo->mObjectName,
//                                              sTableInfo->mDefaultPartitionName,
                                              sRemoteLockTimeout ) != IDE_SUCCESS )
                        {
                            /* cancel retry */
                            sIsCancel = ID_TRUE;
                            break;
                        }
                        /* _BAK_Table Default Partition Lock 획득 */
                        if( executeRemoteSQL( aStatement, 
                                              aFailoverNodeName,
                                              SDI_INTERNAL_OP_FAILOVER,
                                              QCI_STMT_MASK_DML,
                                              aSQLBuf,
                                              aSQLBufSize,
                                              "LOCK TABLE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" IN EXCLUSIVE MODE WAIT %"ID_UINT32_FMT,
//                                              "LOCK TABLE %s._BAK_%s PARTITION ( %s ) IN EXCLUSIVE MODE WAIT %"ID_UINT32_FMT,
                                              sTableInfo->mUserName,
                                              sBufferNameStr,
//                                              sTableInfo->mDefaultPartitionName,
                                              sRemoteLockTimeout ) != IDE_SUCCESS )
                        {
                            /* cancel retry */
                            sIsCancel = ID_TRUE;
                            break;
                        }

                        /* Table lock 획득 성공, 다음 Table을 확인한다. */
                        continue;
                    }
                    else
                    {
                        /* Swap 대상이 아니면 Lock 잡을 필요가 없다. */
                    }
                }

                /* Range Info Check */
                IDE_TEST( sdm::getRangeInfo( aStatement,
                                             QC_SMI_STMT( aStatement ),
                                             aSMN,
                                             sTableInfo,
                                             &sRangeInfos,
                                             ID_FALSE )
                          != IDE_SUCCESS );

                /* check Table Type */
                sShardTableType = sdi::getShardObjectType( sTableInfo );

                for ( j = 0; j < sRangeInfos.mCount; j++ )
                {

                    if ( sRangeInfos.mRanges[j].mNodeId == aTargetNodeInfo->mNodeId )
                    {
                        if ( sdm::checkFailoverAvailable( aReplicaSetInfo,
                                                          sRangeInfos.mRanges[j].mReplicaSetId,
                                                          aFailoverNodeName ) == ID_TRUE )
                        {
                            switch ( sShardTableType )
                            {
                                case SDI_SINGLE_SHARD_KEY_DIST_OBJECT:
                                    /* Partition Swap 대상이다. Partition lock 획득한다. */
                                    /* Partition Lock 획득 */
                                    if( executeRemoteSQL( aStatement, 
                                                          aFailoverNodeName,
                                                          SDI_INTERNAL_OP_FAILOVER,
                                                          QCI_STMT_MASK_DML,
                                                          aSQLBuf,
                                                          aSQLBufSize,
                                                          "LOCK TABLE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" IN EXCLUSIVE MODE WAIT %"ID_UINT32_FMT,
//                                                          "LOCK TABLE %s.%s PARTITION ( %s ) IN EXCLUSIVE MODE WAIT %"ID_UINT32_FMT,
                                                          sTableInfo->mUserName,
                                                          sTableInfo->mObjectName,
//                                                          sRangeInfos.mRanges[j].mPartitionName,
                                                          sRemoteLockTimeout ) != IDE_SUCCESS )
                                    {
                                        /* cancel retry */
                                        sIsCancel = ID_TRUE;
                                        break;
                                    }
                                    /* _BAK_ Table Partition Lock 획득 */
                                    if( executeRemoteSQL( aStatement, 
                                                          aFailoverNodeName,
                                                          SDI_INTERNAL_OP_FAILOVER,
                                                          QCI_STMT_MASK_DML,
                                                          aSQLBuf,
                                                          aSQLBufSize,
                                                          "LOCK TABLE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" IN EXCLUSIVE MODE WAIT %"ID_UINT32_FMT,
//                                                          "LOCK TABLE %s._BAK_%s PARTITION ( %s ) IN EXCLUSIVE MODE WAIT %"ID_UINT32_FMT,
                                                          sTableInfo->mUserName,
                                                          sBufferNameStr,
//                                                          sRangeInfos.mRanges[j].mPartitionName,
                                                          sRemoteLockTimeout ) != IDE_SUCCESS )
                                    {
                                        /* cancel retry */
                                        sIsCancel = ID_TRUE;
                                        break;
                                    }
                                    sIsLocked = ID_TRUE;
                                    break;
                                case SDI_SOLO_DIST_OBJECT:
                                    /* Table Swap 대상이다. Table lock 획득한다. */
                                    /* Table Lock 획득 */
                                    if( executeRemoteSQL( aStatement, 
                                                          aFailoverNodeName,
                                                          SDI_INTERNAL_OP_FAILOVER,
                                                          QCI_STMT_MASK_DML,
                                                          aSQLBuf,
                                                          aSQLBufSize,
                                                          "LOCK TABLE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" IN EXCLUSIVE MODE WAIT %"ID_UINT32_FMT,
                                                          sTableInfo->mUserName,
                                                          sTableInfo->mObjectName,
                                                          sRemoteLockTimeout ) != IDE_SUCCESS )
                                    {
                                        /* cancel retry */
                                        sIsCancel = ID_TRUE;
                                        break;
                                    }
                                    /* _BAK_ Table Lock 획득 */
                                    if( executeRemoteSQL( aStatement, 
                                                          aFailoverNodeName,
                                                          SDI_INTERNAL_OP_FAILOVER,
                                                          QCI_STMT_MASK_DML,
                                                          aSQLBuf,
                                                          aSQLBufSize,
                                                          "LOCK TABLE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" IN EXCLUSIVE MODE WAIT %"ID_UINT32_FMT,
                                                          sTableInfo->mUserName,
                                                          sBufferNameStr,
                                                          sRemoteLockTimeout ) != IDE_SUCCESS )
                                    {
                                        /* cancel retry */
                                        sIsCancel = ID_TRUE;
                                        break;
                                    }
                                    sIsLocked = ID_TRUE;
                                    break;
                                case SDI_COMPOSITE_SHARD_KEY_DIST_OBJECT:
                                    /* Composite는 HA를 제공하지 않기 때문에 Partition Swap 안함.
                                     * Lock 잡을 필요 없음. */
                                    continue;
                                case SDI_CLONE_DIST_OBJECT:
                                    /* Clone 은 전 Node에 Primary로 존재하기 때문에 Partition Swap 할 필요 없음.
                                     * Lock 잡을 필요 없음. */
                                    continue;
                                case SDI_NON_SHARD_OBJECT:
                                default:
                                    /* Non Shard Table이 오면 안됨. */
                                    IDE_DASSERT( 0 );
                                    break;
                            }

                        }
                        else
                        {
                            /* Swap 대상이 아니면 Lock 잡을 필요가 없다. */
                        }
                    }

                    /* Table Lock을 잡았으면 다음 Table Lock을 획득하러 간다.
                     * Range Info 탐색 중에 Lock 획득에 실패했으면 Retry 할것이다. */
                    if ( ( sIsCancel == ID_TRUE ) || ( sIsLocked == ID_TRUE ) )
                    {
                        break;
                    }
                }

                if ( sIsCancel == ID_TRUE )
                {
                    /* RangeInfo 탐색중에 Lock 획득에 실패했다. */
                    break;
                }
            }
            else
            {
                /* R2HA PROCEDURE LOCK NEED */
            }
        }
        if ( sIsCancel == ID_TRUE )
        {
            IDE_TEST( executeRemoteSQL( aStatement, 
                                        aFailoverNodeName,
                                        SDI_INTERNAL_OP_FAILOVER,
                                        QCI_STMT_MASK_SP,
                                        aSQLBuf,
                                        aSQLBufSize,
                                        "ROLLBACK TO SAVEPOINT __FAILOVER_SVP0") != IDE_SUCCESS );
            sIsLockAcquired = ID_FALSE;
        }
        else
        {
            sIsLockAcquired = ID_TRUE;
            break;
        }
    }

    IDE_TEST_RAISE( sIsLockAcquired != ID_TRUE, ERR_FAILOVERLOCK );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_FAILOVERLOCK )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_LOCK_FAIL ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    (void)executeRemoteSQL( aStatement, 
                            aFailoverNodeName,
                            SDI_INTERNAL_OP_FAILOVER,
                            QCI_STMT_MASK_SP,
                            aSQLBuf,
                            aSQLBufSize,
                            "ROLLBACK TO SAVEPOINT __FAILOVER_SVP0" );
    IDE_POP();
    return IDE_FAILURE;
}

IDE_RC qdsd::executeRemoteSQLWithNewTrans( qcStatement * aStatement,
                                           SChar       * aNodeName,
                                           SChar       * aSqlStr,
                                           idBool        aIsDDLTimeOut )
{
    sdiLocalMetaInfo   sLocalMetaInfo;
    SChar              sSqlStr[QD_MAX_SQL_LENGTH + 1];

    IDE_TEST( sdi::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );

    if ( aIsDDLTimeOut == ID_TRUE )
    {
        IDE_TEST( sdi::shardExecTempDDLWithNewTrans( aStatement,
                                                     aNodeName,
                                                     aSqlStr,
                                                     idlOS::strlen(aSqlStr) )
                  != IDE_SUCCESS );
    }
    else
    {
        if ( aNodeName != NULL )
        {
            if ( idlOS::strncmp( sLocalMetaInfo.mNodeName, 
                                 aNodeName,
                                 SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
            {            
                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH, "%s ", aSqlStr );
            }
            else
            {
                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                 "EXEC DBMS_SHARD.EXECUTE_IMMEDIATE( '%s', '"QCM_SQL_STRING_SKIP_FMT"' ) ",
                                 aSqlStr,
                                 aNodeName );
            }
        }
        else
        {
            idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                             "EXEC DBMS_SHARD.EXECUTE_IMMEDIATE( '%s', NULL ) ",
                             aSqlStr );
        }

        IDE_TEST( sdi::shardExecTempDMLOrDCLWithNewTrans( aStatement,
                                                          sSqlStr )
                  != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}


IDE_RC qdsd::changeBuckupNodeNameForFailOver( qcStatement       * aStatement,
                                              sdiReplicaSetInfo * aReplicaSetInfo,
                                              UInt                aNth)
{
    sdiLocalMetaInfo sLocalMetaInfo;
    SChar            sSqlStr[QD_MAX_SQL_LENGTH + 1];

    IDE_TEST( sdi::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );
    
    if ( aNth == 0 ) /* First */
    {
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "UPDATE SYS_SHARD.REPLICA_SETS_  "
                         "   SET FIRST_BACKUP_NODE_NAME = '%s', "
                         "       STOP_FIRST_BACKUP_NODE_NAME = '%s' "                         
                         "   WHERE PRIMARY_NODE_NAME    = '%s' "
                         "         AND SMN              = %"ID_INT32_FMT,
                         SDM_NA_STR,
                         aReplicaSetInfo->mReplicaSets[0].mFirstBackupNodeName,                         
                         aReplicaSetInfo->mReplicaSets[0].mPrimaryNodeName,
                         aReplicaSetInfo->mReplicaSets[0].mSMN );

        IDE_TEST( executeRemoteSQLWithNewTrans( aStatement,
                                                sLocalMetaInfo.mNodeName,
                                                sSqlStr,
                                                ID_FALSE )
                  != IDE_SUCCESS );

        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "UPDATE SYS_SHARD.REPLICA_SETS_  "
                         "   SET FIRST_BACKUP_NODE_NAME = ''%s'', "
                         "       STOP_FIRST_BACKUP_NODE_NAME = ''%s'' "                         
                         "   WHERE PRIMARY_NODE_NAME    = ''%s'' "
                         "         AND SMN              = %"ID_INT32_FMT,
                         SDM_NA_STR,
                         aReplicaSetInfo->mReplicaSets[0].mFirstBackupNodeName,                         
                         aReplicaSetInfo->mReplicaSets[0].mPrimaryNodeName,
                         aReplicaSetInfo->mReplicaSets[0].mSMN );

        IDE_TEST( executeRemoteSQLWithNewTrans( aStatement,
                                                NULL,
                                                sSqlStr,
                                                ID_FALSE )
                  != IDE_SUCCESS );
        
    }
    else if ( aNth == 1 ) /* Second */
    {
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "UPDATE SYS_SHARD.REPLICA_SETS_ "
                         "   SET SECOND_BACKUP_NODE_NAME = '%s', "
                         "       STOP_SECOND_BACKUP_NODE_NAME = '%s' "                         
                         "   WHERE PRIMARY_NODE_NAME     = '%s' "
                         "         AND SMN               = %"ID_INT32_FMT,
                         SDM_NA_STR,
                         aReplicaSetInfo->mReplicaSets[0].mSecondBackupNodeName,                         
                         aReplicaSetInfo->mReplicaSets[0].mPrimaryNodeName,
                         aReplicaSetInfo->mReplicaSets[0].mSMN );

        IDE_TEST( executeRemoteSQLWithNewTrans( aStatement,
                                                sLocalMetaInfo.mNodeName,
                                                sSqlStr,
                                                ID_FALSE )
                  != IDE_SUCCESS );
        
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "UPDATE SYS_SHARD.REPLICA_SETS_ "
                         "   SET SECOND_BACKUP_NODE_NAME = ''%s'', "
                         "       STOP_SECOND_BACKUP_NODE_NAME = ''%s'' "
                         "   WHERE PRIMARY_NODE_NAME     =  ''%s'' "
                         "         AND SMN               = %"ID_INT32_FMT,
                         SDM_NA_STR,
                         aReplicaSetInfo->mReplicaSets[0].mSecondBackupNodeName,                         
                         aReplicaSetInfo->mReplicaSets[0].mPrimaryNodeName,
                         aReplicaSetInfo->mReplicaSets[0].mSMN );

        IDE_TEST( executeRemoteSQLWithNewTrans( aStatement,
                                                NULL,
                                                sSqlStr,
                                                ID_FALSE )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC qdsd::flushReplicationForInternal( qcStatement * aStatement,
                                          SChar       * aNodeName,
                                          SChar       * aReplName )
{
    sdiLocalMetaInfo    sLocalMetaInfo;
    SChar               sSqlStr[QD_MAX_SQL_LENGTH + 1];
    UInt                sShardDDLLockTryCount = QCG_GET_SESSION_SHARD_DDL_LOCK_TRY_COUNT(aStatement);
    UInt                sRemoteLockTimeout = QCG_GET_SESSION_SHARD_DDL_LOCK_TIMEOUT(aStatement);

    sdiInternalOperation sInternalOP = SDI_INTERNAL_OP_NORMAL;

    if( qci::mSessionCallback.mGetShardInternalLocalOperation( aStatement->session->mMmSession ) 
        == SDI_INTERNAL_OP_FAILBACK )
    {
        sInternalOP = SDI_INTERNAL_OP_FAILBACK;
    }

    IDE_TEST( sdi::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );

    IDE_TEST( sdi::checkShardLinker( aStatement ) != IDE_SUCCESS );

    if ( aStatement->session->mQPSpecific.mClientInfo != NULL )
    {
        if ( idlOS::strncmp( sLocalMetaInfo.mNodeName, 
                             aNodeName,
                             SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
        {
            IDE_TEST( executeLocalSQL( aStatement, 
                                       QCI_STMT_MASK_DCL,
                                       sSqlStr,
                                       QD_MAX_SQL_LENGTH,
                                       "ALTER REPLICATION "QCM_SQL_STRING_SKIP_FMT" FLUSH WAIT %"ID_UINT32_FMT,
                                       aReplName,
                                       (sShardDDLLockTryCount * sRemoteLockTimeout) )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( executeRemoteSQL( aStatement, 
                                        aNodeName,
                                        sInternalOP,
                                        QCI_STMT_MASK_DCL,
                                        sSqlStr,
                                        QD_MAX_SQL_LENGTH,
                                        "ALTER REPLICATION "QCM_SQL_STRING_SKIP_FMT" FLUSH WAIT %"ID_UINT32_FMT,
                                        aReplName,
                                        (sShardDDLLockTryCount * sRemoteLockTimeout) )
                      != IDE_SUCCESS);
        }
            
        ideLog::log(IDE_SD_17,"[SHARD INTERNAL SQL]: %s, %s",aNodeName, sSqlStr);
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdsd::executeReplicationForJoin( qcStatement * aStatement,
                                        idBool        aIsException )
{
    qdShardParseTree  * sParseTree = NULL;
    sdiLocalMetaInfo    sLocalMetaInfo;
    sdiReplicaSetInfo   sReplicaSetInfo;
    sdiReplicaSetInfo   sCurrReplicaSetInfo;
    sdiReplicaSetInfo   sPrevReplicaSetInfo;
    sdiReplicaSetInfo   sPrevPrevReplicaSetInfo;
    sdiGlobalMetaInfo   sMetaNodeInfo = { ID_ULONG(0) };
    ULong               sSMN = ID_ULONG(0);
    UInt                i = 0;

    sParseTree = (qdShardParseTree *)aStatement->myPlan->parseTree;
    
    IDE_TEST( sdi::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );

    IDE_TEST( sdi::getGlobalMetaInfoCore( QC_SMI_STMT( aStatement ),
                                          &sMetaNodeInfo )
              != IDE_SUCCESS );
    
    sSMN = sMetaNodeInfo.mShardMetaNumber;
    
    IDE_TEST( sdi::getAllReplicaSetInfoSortedByPName( QC_SMI_STMT( aStatement ),
                                                      &sReplicaSetInfo,
                                                      sSMN )
              != IDE_SUCCESS );

    IDE_TEST( findBackupReplicaSetInfo( sLocalMetaInfo.mNodeName,
                                        &sReplicaSetInfo,
                                        &sCurrReplicaSetInfo,
                                        &sPrevReplicaSetInfo,
                                        &sPrevPrevReplicaSetInfo )
              != IDE_SUCCESS );

    for ( i = 0; i < sLocalMetaInfo.mKSafety; i++ )
    {
        if ( i == 0 )
        {
            if ( aIsException == ID_TRUE )
            {
                IDE_TEST( stopReplicationForInternal(
                              aStatement,
                              sCurrReplicaSetInfo.mReplicaSets[0].mPrimaryNodeName,
                              sCurrReplicaSetInfo.mReplicaSets[0].mFirstReplName )
                          != IDE_SUCCESS );            
            }
            else
            {
                // current node replication start
                IDE_TEST( syncReplicationForInternal(
                              aStatement,
                              sCurrReplicaSetInfo.mReplicaSets[0].mPrimaryNodeName,
                              sCurrReplicaSetInfo.mReplicaSets[0].mFirstReplName )
                          != IDE_SUCCESS );
            
                // current node replication flush
                IDE_TEST( flushReplicationForInternal(
                              aStatement,
                              sCurrReplicaSetInfo.mReplicaSets[0].mPrimaryNodeName,
                              sCurrReplicaSetInfo.mReplicaSets[0].mFirstReplName )
                          != IDE_SUCCESS );


                // prev node replication flush
                IDE_TEST( flushReplicationForInternal(
                              aStatement,
                              sPrevReplicaSetInfo.mReplicaSets[0].mPrimaryNodeName,
                              sPrevReplicaSetInfo.mReplicaSets[0].mFirstReplName )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            // 노드2개 경우 second backup이 존재하지 않는다.
            if ( sParseTree->mNodeCount > 2 )
            {
                if ( aIsException == ID_TRUE )
                {
                    IDE_TEST( stopReplicationForInternal(
                                  aStatement,
                                  sCurrReplicaSetInfo.mReplicaSets[0].mPrimaryNodeName,
                                  sCurrReplicaSetInfo.mReplicaSets[0].mSecondReplName )
                              != IDE_SUCCESS );
                }
                else
                {    
                    // current node replication start
                    IDE_TEST( syncReplicationForInternal(
                                  aStatement,
                                  sCurrReplicaSetInfo.mReplicaSets[0].mPrimaryNodeName,
                                  sCurrReplicaSetInfo.mReplicaSets[0].mSecondReplName )
                              != IDE_SUCCESS );
            
                    // current node replication flush
                    IDE_TEST( flushReplicationForInternal(
                                  aStatement,
                                  sCurrReplicaSetInfo.mReplicaSets[0].mPrimaryNodeName,
                                  sCurrReplicaSetInfo.mReplicaSets[0].mSecondReplName )
                              != IDE_SUCCESS );

                    // prev prev node replication flush
                    IDE_TEST( flushReplicationForInternal(
                                  aStatement,
                                  sPrevPrevReplicaSetInfo.mReplicaSets[0].mPrimaryNodeName,
                                  sPrevPrevReplicaSetInfo.mReplicaSets[0].mSecondReplName )
                              != IDE_SUCCESS );
                }
            }
        }
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdsd::checkZookeeperSMNAndDataSMN()
{
    SInt  sDataLength = 0;
    SChar sBuffer[SDI_ZKC_BUFFER_SIZE];
    ULong sZookeeperSMN = SDI_NULL_SMN;
    ULong sMyNodeDataSMN = SDI_NULL_SMN;
    idlOS::memset( sBuffer, 0, SDI_ZKC_BUFFER_SIZE );

    sDataLength = SDI_ZKC_BUFFER_SIZE;
    
    IDE_TEST( sdiZookeeper::readInfo( (SChar*)SDI_ZKC_PATH_SMN,
                                      sBuffer,
                                      &sDataLength ) != IDE_SUCCESS );
    sZookeeperSMN = (ULong)idlOS::strtoul( sBuffer, NULL , 10);
    sMyNodeDataSMN = sdi::getSMNForDataNode();
    IDE_TEST_RAISE( sZookeeperSMN != sMyNodeDataSMN,
                    ERR_DIFFERENT_SMN );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_DIFFERENT_SMN )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_SHARD_NOT_SAME_CLUSTER_META_DATA_SMN, sZookeeperSMN, sMyNodeDataSMN ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdsd::failoverTablePartitionSwap( qcStatement        * aStatement,
                                         SChar              * aFailoverNodeName,
                                         sdiTableInfoList   * aTableInfoList,
                                         sdiReplicaSetInfo  * aReplicaSetInfo,
                                         sdiNode            * aTargetNodeInfo,
                                         ULong                aSMN )
{
    sdiTableInfoList  * sTmpTableInfoList = NULL;    

    sdiTableInfo      * sTableInfo  = NULL;
    sdiRangeInfo        sRangeInfos;

    sdiShardObjectType   sShardTableType = SDI_NON_SHARD_OBJECT;
    
    UInt                j;

    SChar               sSqlStr[QD_MAX_SQL_LENGTH + 1];
    SChar               sBufferNameStr[QC_MAX_OBJECT_NAME_LEN + 1];
    
    for ( sTmpTableInfoList = aTableInfoList;
          sTmpTableInfoList != NULL;
          sTmpTableInfoList = sTmpTableInfoList->mNext )

    {
        sTableInfo = sTmpTableInfoList->mTableInfo;

        /* Procedure는 Swap할 Partition이 없다. */
        if ( sTableInfo->mObjectType == 'T' )
        {
            idlOS::snprintf( sBufferNameStr,
                             QC_MAX_OBJECT_NAME_LEN + 1,
                             "%s%s",
                             SDI_BACKUP_TABLE_PREFIX,
                             sTableInfo->mObjectName );

            /* Default Partition check */
            if ( sTableInfo->mDefaultNodeId == aTargetNodeInfo->mNodeId )
            {
                if ( sdm::checkFailoverAvailable( aReplicaSetInfo,
                                                  sTableInfo->mDefaultPartitionReplicaSetId,
                                                  aFailoverNodeName ) == ID_TRUE )
                {
                    /* Partition Swap Send to TargetNode */
                    idlOS::snprintf( sSqlStr, 
                                     QD_MAX_SQL_LENGTH,
                                     "ALTER TABLE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" REPLACE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" PARTITION "QCM_SQL_STRING_SKIP_FMT"",
                                     sTableInfo->mUserName,
                                     sBufferNameStr,
                                     sTableInfo->mUserName,
                                     sTableInfo->mObjectName,
                                     sTableInfo->mDefaultPartitionName );

                    IDE_TEST( executeRemoteSQL( aStatement, 
                                                aFailoverNodeName,
                                                SDI_INTERNAL_OP_FAILOVER,
                                                QCI_STMT_MASK_DDL,
                                                sSqlStr,
                                                QD_MAX_SQL_LENGTH,
                                                NULL )
                                != IDE_SUCCESS );
                }
                else
                {
                    /* Failover 할수 없다면 Swap 하지 않는다. */
                }
            }

            /* Range Info Check */
            IDE_TEST( sdm::getRangeInfo( aStatement,
                                         QC_SMI_STMT( aStatement ),
                                         aSMN,
                                         sTableInfo,
                                         &sRangeInfos,
                                         ID_FALSE )
                      != IDE_SUCCESS );

            /* check Table Type */
            sShardTableType = sdi::getShardObjectType( sTableInfo );

            for ( j = 0; j < sRangeInfos.mCount; j++ )
            {

                if ( sRangeInfos.mRanges[j].mNodeId == aTargetNodeInfo->mNodeId )
                {
                    if ( sdm::checkFailoverAvailable( aReplicaSetInfo,
                                                      sRangeInfos.mRanges[j].mReplicaSetId,
                                                      aFailoverNodeName ) == ID_TRUE )
                    {
                        switch ( sShardTableType )
                        {
                            case SDI_SINGLE_SHARD_KEY_DIST_OBJECT:
                                /* Partition Swap */
                                idlOS::snprintf( sSqlStr, 
                                                 QD_MAX_SQL_LENGTH,
                                                 "ALTER TABLE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" REPLACE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" PARTITION "QCM_SQL_STRING_SKIP_FMT"",
                                                 sTableInfo->mUserName,
                                                 sBufferNameStr,
                                                 sTableInfo->mUserName,
                                                 sTableInfo->mObjectName,
                                                 sRangeInfos.mRanges[j].mPartitionName );
                                break;
                            case SDI_SOLO_DIST_OBJECT:
                                /* Partition Swap */
                                idlOS::snprintf( sSqlStr, 
                                                 QD_MAX_SQL_LENGTH,
                                                 "ALTER TABLE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" REPLACE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT"",
                                                 sTableInfo->mUserName,
                                                 sBufferNameStr,
                                                 sTableInfo->mUserName,
                                                 sTableInfo->mObjectName );

                                break;
                            case SDI_COMPOSITE_SHARD_KEY_DIST_OBJECT:
                                /* Composite는 HA를 제공하지 않기 때문에 Partition Swap 할 필요 없음. */
                                continue;
                            case SDI_CLONE_DIST_OBJECT:
                                /* Clone 은 전 Node에 Primary로 존재하기 때문에 Partition Swap 할 필요 없음. */
                                continue;
                            case SDI_NON_SHARD_OBJECT:
                            default:
                                /* Non Shard Table이 오면 안됨. */
                                IDE_DASSERT( 0 );
                                break;
                        }
                        IDE_TEST( executeRemoteSQL( aStatement, 
                                                    aFailoverNodeName,
                                                    SDI_INTERNAL_OP_FAILOVER,
                                                    QCI_STMT_MASK_DDL,
                                                    sSqlStr,
                                                    QD_MAX_SQL_LENGTH,
                                                    NULL )
                                    != IDE_SUCCESS );
                    }
                    else
                    {
                        /* Failover 할수 없다면 Swap 하지 않는다. */
                    }
                }
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdsd::failoverResetShardMeta( qcStatement        * aStatement,
                                     SChar              * aTargetNodeName,
                                     SChar              * aFailoverNodeName,
                                     sdiTableInfoList   * aTableInfoList,
                                     sdiReplicaSetInfo  * aReplicaSetInfo,
                                     sdiNode            * aTargetNodeInfo,
                                     iduList            * aAliveNodeList,
                                     ULong                aSMN,
                                     idBool             * aIsUnsetNode )
{
    sdiTableInfoList  * sTmpTableInfoList = NULL;    

    sdiTableInfo      * sTableInfo  = NULL;
    sdiRangeInfo        sRangeInfos;

    sdiShardObjectType   sShardTableType = SDI_NON_SHARD_OBJECT;
    
    UInt                j;

    SChar               sSqlStr[QD_MAX_SQL_LENGTH + 1];

    idBool              sIsUnsetNode = ID_TRUE;

    sdiValue            sValueStr;

    SChar               sObjectValue[SDI_RANGE_VARCHAR_MAX_PRECISION + 1];

    for ( sTmpTableInfoList = aTableInfoList;
          sTmpTableInfoList != NULL;
          sTmpTableInfoList = sTmpTableInfoList->mNext )

    {
        sTableInfo = sTmpTableInfoList->mTableInfo;

        /* Default Partition check */
        if ( sTableInfo->mDefaultNodeId == aTargetNodeInfo->mNodeId )
        {
            if ( sdm::checkFailoverAvailable( aReplicaSetInfo,
                                              sTableInfo->mDefaultPartitionReplicaSetId,
                                              aFailoverNodeName ) == ID_TRUE )
            {
                /* Send resetShardPartitioinNode to All Alive Node */
                if ( sTableInfo->mObjectType == 'T' ) /* Table */
                {
                    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                     "exec dbms_shard.reset_shard_partition_node( '"
                                     QCM_SQL_STRING_SKIP_FMT"', '"
                                     QCM_SQL_STRING_SKIP_FMT"', '"
                                     QCM_SQL_STRING_SKIP_FMT"', '"
                                     QCM_SQL_STRING_SKIP_FMT"', "
                                     QCM_SQL_VARCHAR_FMT" )",
                                     sTableInfo->mUserName,
                                     sTableInfo->mObjectName,
                                     aTargetNodeName,
                                     aFailoverNodeName,
                                     sTableInfo->mDefaultPartitionName );
                }
                else /* Procedure */
                {
                    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                     "exec dbms_shard.reset_shard_resident_node( '"
                                     QCM_SQL_STRING_SKIP_FMT"', '"
                                     QCM_SQL_STRING_SKIP_FMT"', '"
                                     QCM_SQL_STRING_SKIP_FMT"', '"
                                     QCM_SQL_STRING_SKIP_FMT"', "
                                     QCM_SQL_VARCHAR_FMT", "
                                     QCM_SQL_VARCHAR_FMT", "
                                     QCM_SQL_UINT32_FMT" )",
                                     sTableInfo->mUserName,
                                     sTableInfo->mObjectName,
                                     aTargetNodeName,
                                     aFailoverNodeName,
                                     "",
                                     "",
                                     1 );
                }

                IDE_TEST( executeSQLNodeList( aStatement,
                                              SDI_INTERNAL_OP_FAILOVER,
                                              QCI_STMT_MASK_SP,
                                              sSqlStr,
                                              aTargetNodeName,
                                              aAliveNodeList ) != IDE_SUCCESS );

            }
            else
            {
                /* Failover 할수 없다면 Shard Meta 에 그냥 둔다. */
                /* Shard Meta에 남아있다면 UnsetNode 수행 대상이 아니다. */
                sIsUnsetNode = ID_FALSE;
            }
        }

        /* Range Info Check */
        IDE_TEST( sdm::getRangeInfo( aStatement,
                                     QC_SMI_STMT( aStatement ),
                                     aSMN,
                                     sTableInfo,
                                     &sRangeInfos,
                                     ID_FALSE )
                  != IDE_SUCCESS );

        /* check Table Type */
        sShardTableType = sdi::getShardObjectType( sTableInfo );

        for ( j = 0; j < sRangeInfos.mCount; j++ )
        {

            if ( sRangeInfos.mRanges[j].mNodeId == aTargetNodeInfo->mNodeId )
            {
                if ( sdm::checkFailoverAvailable( aReplicaSetInfo,
                                                  sRangeInfos.mRanges[j].mReplicaSetId,
                                                  aFailoverNodeName ) == ID_TRUE )
                {
                    switch ( sShardTableType )
                    {
                        case SDI_SINGLE_SHARD_KEY_DIST_OBJECT:
                            /* send resetShardPartitionNode to All Alive Node */
                            if ( sTableInfo->mObjectType == 'T' ) /* Table */
                            {
                                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                                 "exec dbms_shard.reset_shard_partition_node('"
                                                 QCM_SQL_STRING_SKIP_FMT"', '"
                                                 QCM_SQL_STRING_SKIP_FMT"', '"
                                                 QCM_SQL_STRING_SKIP_FMT"', '"
                                                 QCM_SQL_STRING_SKIP_FMT"', "
                                                 QCM_SQL_VARCHAR_FMT" )",
                                                 sTableInfo->mUserName,
                                                 sTableInfo->mObjectName,
                                                 aTargetNodeName,
                                                 aFailoverNodeName,
                                                 sRangeInfos.mRanges[j].mPartitionName );
                            }
                            else /* Procedure */
                            {
                                if ( sTableInfo->mSplitMethod == SDI_SPLIT_HASH )
                                {
                                    IDE_TEST( sdi::getValueStr( MTD_INTEGER_ID,
                                                                &sRangeInfos.mRanges[j].mValue,
                                                                &sValueStr )
                                              != IDE_SUCCESS );
                                }
                                else
                                {
                                    IDE_TEST( sdi::getValueStr( sTableInfo->mKeyDataType,
                                                                &sRangeInfos.mRanges[j].mValue,
                                                                &sValueStr )
                                              != IDE_SUCCESS );
                                }

                                if ( sValueStr.mCharMax.value[0] == '\'' )
                                {
                                    // INPUT ARG ('''A''') => 'A' => '''A'''
                                    idlOS::snprintf( sObjectValue,
                                                     SDI_RANGE_VARCHAR_MAX_PRECISION + 1,
                                                     "''%s''",
                                                     sValueStr.mCharMax.value );
                                }
                                else
                                {
                                    // INPUT ARG ('A') => A => 'A'
                                    idlOS::snprintf( sObjectValue,
                                                     SDI_RANGE_VARCHAR_MAX_PRECISION + 1,
                                                     "'%s'",
                                                     sValueStr.mCharMax.value );
                                }

                                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                                 "exec dbms_shard.reset_shard_resident_node('"
                                                 QCM_SQL_STRING_SKIP_FMT"', '"
                                                 QCM_SQL_STRING_SKIP_FMT"', '"
                                                 QCM_SQL_STRING_SKIP_FMT"', '"
                                                 QCM_SQL_STRING_SKIP_FMT"', "
                                                 "%s, "
                                                 QCM_SQL_VARCHAR_FMT" )",
                                                 sTableInfo->mUserName,
                                                 sTableInfo->mObjectName,
                                                 aTargetNodeName,
                                                 aFailoverNodeName,
                                                 sObjectValue,
                                                 "" );
                            }

                            IDE_TEST( executeSQLNodeList( aStatement,
                                                          SDI_INTERNAL_OP_FAILOVER,
                                                          QCI_STMT_MASK_SP,
                                                          sSqlStr,
                                                          aTargetNodeName,
                                                          aAliveNodeList ) != IDE_SUCCESS );
                            break;
                        case SDI_SOLO_DIST_OBJECT:
                            /* send resetShardPartitionNode to All Alive Node */
                            if ( sTableInfo->mObjectType == 'T' ) /* Table */
                            {
                                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                                 "exec dbms_shard.reset_shard_partition_node('"
                                                 QCM_SQL_STRING_SKIP_FMT"', '"
                                                 QCM_SQL_STRING_SKIP_FMT"', '"
                                                 QCM_SQL_STRING_SKIP_FMT"', '"
                                                 QCM_SQL_STRING_SKIP_FMT"', "
                                                 QCM_SQL_VARCHAR_FMT" )",
                                                 sTableInfo->mUserName,
                                                 sTableInfo->mObjectName,
                                                 aTargetNodeName,
                                                 aFailoverNodeName,
                                                 sRangeInfos.mRanges[j].mPartitionName );
                            }
                            else /* Procedure */
                            {
                                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                                 "exec dbms_shard.reset_shard_resident_node('"
                                                 QCM_SQL_STRING_SKIP_FMT"', '"
                                                 QCM_SQL_STRING_SKIP_FMT"', '"
                                                 QCM_SQL_STRING_SKIP_FMT"', '"
                                                 QCM_SQL_STRING_SKIP_FMT"' )",
                                                 sTableInfo->mUserName,
                                                 sTableInfo->mObjectName,
                                                 aTargetNodeName,
                                                 aFailoverNodeName );
                            }

                            IDE_TEST( executeSQLNodeList( aStatement,
                                                          SDI_INTERNAL_OP_FAILOVER,
                                                          QCI_STMT_MASK_SP,
                                                          sSqlStr,
                                                          aTargetNodeName,
                                                          aAliveNodeList ) != IDE_SUCCESS );
                            break;
                        case SDI_COMPOSITE_SHARD_KEY_DIST_OBJECT:
                            /* Composite는 HA를 제공하지 않기 때문에 Failover 대상이 아니다. */
                            continue;
                        case SDI_CLONE_DIST_OBJECT:
                            /* Clones_ 에서 제거 해 주어야 한다. */
                            if ( sTableInfo->mObjectType == 'T' ) /* Table */
                            {
                                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                                 "exec dbms_shard.reset_shard_partition_node('"
                                                 QCM_SQL_STRING_SKIP_FMT"', '"
                                                 QCM_SQL_STRING_SKIP_FMT"', '"
                                                 QCM_SQL_STRING_SKIP_FMT"', '', "
                                                 QCM_SQL_VARCHAR_FMT" )",
                                                 sTableInfo->mUserName,
                                                 sTableInfo->mObjectName,
                                                 aTargetNodeName,
                                                 sRangeInfos.mRanges[j].mPartitionName );
                            }
                            else /* Procedure */
                            {
                                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                                 "exec dbms_shard.reset_shard_resident_node('"
                                                 QCM_SQL_STRING_SKIP_FMT"', '"
                                                 QCM_SQL_STRING_SKIP_FMT"', '"
                                                 QCM_SQL_STRING_SKIP_FMT"', '' )",
                                                 sTableInfo->mUserName,
                                                 sTableInfo->mObjectName,
                                                 aTargetNodeName );
                            }

                            IDE_TEST( executeSQLNodeList( aStatement,
                                                          SDI_INTERNAL_OP_FAILOVER,
                                                          QCI_STMT_MASK_SP,
                                                          sSqlStr,
                                                          aTargetNodeName,
                                                          aAliveNodeList ) != IDE_SUCCESS );
                            continue;
                        case SDI_NON_SHARD_OBJECT:
                        default:
                            /* Non Shard Table이 오면 안됨. */
                            IDE_DASSERT( 0 );
                            break;
                    }
                }
                else
                {
                    /* Failover 할수 없다면 CLONE Table 만 Shard Meta 에서 제거한다.
                     * reset_shard_partition_node는 NewNodeName이 null 이면 제거이다. */
                    switch ( sShardTableType )
                    {
                        case SDI_SINGLE_SHARD_KEY_DIST_OBJECT:
                        case SDI_SOLO_DIST_OBJECT:
                            /* Shard Meta에 남아있다면 UnsetNode 수행 대상이 아니다. */
                            sIsUnsetNode = ID_FALSE;
                            continue;
                        case SDI_COMPOSITE_SHARD_KEY_DIST_OBJECT:
                            /* Composite는 HA를 제공하지 않기 때문에 Failover 대상이 아니다. */
                            continue;
                        case SDI_CLONE_DIST_OBJECT:
                            /* Clones_ 에서 제거 해 주어야 한다. */
                            if ( sTableInfo->mObjectType == 'T' ) /* Table */
                            {
                                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                                 "exec dbms_shard.reset_shard_partition_node('"
                                                 QCM_SQL_STRING_SKIP_FMT"', '"
                                                 QCM_SQL_STRING_SKIP_FMT"', '"
                                                 QCM_SQL_STRING_SKIP_FMT"', '', "
                                                 QCM_SQL_VARCHAR_FMT" )",
                                                 sTableInfo->mUserName,
                                                 sTableInfo->mObjectName,
                                                 aTargetNodeName,
                                                 sRangeInfos.mRanges[j].mPartitionName );
                            }
                            else /* Procedure */
                            {
                                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                                 "exec dbms_shard.reset_shard_resident_node('"
                                                 QCM_SQL_STRING_SKIP_FMT"', '"
                                                 QCM_SQL_STRING_SKIP_FMT"', '"
                                                 QCM_SQL_STRING_SKIP_FMT"', '') ",
                                                 sTableInfo->mUserName,
                                                 sTableInfo->mObjectName,
                                                 aTargetNodeName );
                            }

                            IDE_TEST( executeSQLNodeList( aStatement,
                                                          SDI_INTERNAL_OP_FAILOVER,
                                                          QCI_STMT_MASK_SP,
                                                          sSqlStr,
                                                          aTargetNodeName,
                                                          aAliveNodeList ) != IDE_SUCCESS );
                            continue;
                        case SDI_NON_SHARD_OBJECT:
                        default:
                            /* Non Shard Table이 오면 안됨. */
                            IDE_DASSERT( 0 );
                            break;
                    }
                }
            }
        }
    }

    *aIsUnsetNode = sIsUnsetNode;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdsd::failoverAddReverseReplicationItems( qcStatement        * aStatement,
                                                 SChar              * aTargetNodeName,
                                                 SChar              * aFailoverNodeName,
                                                 sdiTableInfoList   * aTableInfoList,
                                                 sdiReplicaSetInfo  * aReplicaSetInfo,
                                                 sdiNode            * aTargetNodeInfo,
                                                 ULong                aSMN )
{
    sdiTableInfoList  * sTmpTableInfoList = NULL;    

    sdiTableInfo      * sTableInfo  = NULL;
    sdiRangeInfo        sRangeInfos;

    sdiReplicaSet     * sPrevNodeReplicaSet = NULL;
    sdiReplicaSet     * sPrevPrevNodeReplicaSet = NULL;

    sdiShardObjectType   sShardTableType = SDI_NON_SHARD_OBJECT;

    UInt                i;
    UInt                j;

    SChar               sPartitionName[QC_MAX_OBJECT_NAME_LEN + 1] = { 0, };

    UInt                sPrevReplicaSetId = SDI_REPLICASET_NULL_ID;
    UInt                sPrevPrevReplicaSetId = SDI_REPLICASET_NULL_ID;

    SChar               sPrevReverseReplName[SDI_REPLICATION_NAME_MAX_SIZE + 1];
    SChar               sPrevPrevReverseReplName[SDI_REPLICATION_NAME_MAX_SIZE + 1];


    /* Find Old/New Node ReplicaSet */
    for ( i = 0 ; i < aReplicaSetInfo->mCount; i++ )
    {
        if ( idlOS::strncmp( aReplicaSetInfo->mReplicaSets[i].mPrimaryNodeName,
                             aTargetNodeName,
                             SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
        {
            /* TargetNode가 가진 Failover 가능한 ReplicaSet을 찾는다.
             * 2개 이상 가지고 있을수 있으므로 Prev를 먼저 찾는다. */
            if ( idlOS::strncmp( aReplicaSetInfo->mReplicaSets[i].mFirstBackupNodeName,
                                 aFailoverNodeName,
                                 SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
            {
                sPrevNodeReplicaSet = &(aReplicaSetInfo->mReplicaSets[i]);
                sPrevReplicaSetId   = sPrevNodeReplicaSet->mReplicaSetId;
                idlOS::snprintf( sPrevReverseReplName, 
                                 SDI_REPLICATION_NAME_MAX_SIZE + 1,
                                 "R_%s", 
                                 sPrevNodeReplicaSet->mFirstReplName );
            }
            else if ( idlOS::strncmp( aReplicaSetInfo->mReplicaSets[i].mSecondBackupNodeName,
                                      aFailoverNodeName,
                                      SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
            {
                sPrevPrevNodeReplicaSet = &(aReplicaSetInfo->mReplicaSets[i]);
                sPrevPrevReplicaSetId   = sPrevPrevNodeReplicaSet->mReplicaSetId;

                idlOS::snprintf( sPrevPrevReverseReplName, 
                                 SDI_REPLICATION_NAME_MAX_SIZE + 1,
                                 "R_%s", 
                                 sPrevPrevNodeReplicaSet->mSecondReplName );
            }
        }
    }

    /* R2HA Clone 처리 필요 */
    /* N1 N2 N3 N4 N5 N6 에서 N1을 N4가 Failover 할 때 ... 
     * N1->N4 간에는 이중화가 없어 ReverseRP를 구축하지 않는다.
     * 이런 Failover 발생시 Failback에서 Fullsync 받던가 뭔가 해야 함. */
    /* Clone은 ReverseRP에서만 처리한다. */

    for ( sTmpTableInfoList = aTableInfoList;
          sTmpTableInfoList != NULL;
          sTmpTableInfoList = sTmpTableInfoList->mNext )

    {
        sTableInfo = sTmpTableInfoList->mTableInfo;

        /* Procedure는 RP ADD를 수행하지 않는다. */
        if ( sTableInfo->mObjectType == 'T' )
        {
            /* Default Partition check */
            if ( sTableInfo->mDefaultNodeId == aTargetNodeInfo->mNodeId )
            {
                if ( sdm::checkFailoverAvailable( aReplicaSetInfo,
                                                  sTableInfo->mDefaultPartitionReplicaSetId,
                                                  aFailoverNodeName ) == ID_TRUE )
                {
                    /* Failover 가능하면 ReverseRP가 생성되어 있다. */
                    if ( sTableInfo->mDefaultPartitionReplicaSetId == sPrevReplicaSetId )
                    {
                        IDE_TEST( sdm::addReplTable( aStatement,
                                                     aFailoverNodeName,
                                                     sPrevReverseReplName,
                                                     sTableInfo->mUserName,
                                                     sTableInfo->mObjectName,
                                                     sTableInfo->mDefaultPartitionName,
                                                     SDM_REPL_CLONE,
                                                     ID_FALSE )
                                  != IDE_SUCCESS );
                    }
                    else if ( sTableInfo->mDefaultPartitionReplicaSetId == sPrevPrevReplicaSetId )
                    {
                        IDE_TEST( sdm::addReplTable( aStatement,
                                                     aFailoverNodeName,
                                                     sPrevPrevReverseReplName,
                                                     sTableInfo->mUserName,
                                                     sTableInfo->mObjectName,
                                                     sTableInfo->mDefaultPartitionName,
                                                     SDM_REPL_CLONE,
                                                     ID_FALSE )
                                  != IDE_SUCCESS );
                    }
                }
            }

            /* Range Info Check */
            IDE_TEST( sdm::getRangeInfo( aStatement,
                                         QC_SMI_STMT( aStatement ),
                                         aSMN,
                                         sTableInfo,
                                         &sRangeInfos,
                                         ID_FALSE )
                      != IDE_SUCCESS );

            /* check Table Type */
            sShardTableType = sdi::getShardObjectType( sTableInfo );

            for ( j = 0; j < sRangeInfos.mCount; j++ )
            {
                if ( sRangeInfos.mRanges[j].mNodeId == aTargetNodeInfo->mNodeId )
                {
                    if ( sdm::checkFailoverAvailable( aReplicaSetInfo,
                                                      sRangeInfos.mRanges[j].mReplicaSetId,
                                                      aFailoverNodeName ) == ID_TRUE )
                    {
                        switch ( sShardTableType )
                        {
                            case SDI_SINGLE_SHARD_KEY_DIST_OBJECT:
                                /* Range/Solo의 경우는 Failover 가능이면 추가하면 됨. */
                                idlOS::strncpy( sPartitionName,
                                                sRangeInfos.mRanges[j].mPartitionName,
                                                QC_MAX_OBJECT_NAME_LEN + 1 );
                                break;
                            case SDI_SOLO_DIST_OBJECT:
                                /* Range/Solo의 경우는 Failover 가능이면 추가하면 됨. */
                                idlOS::strncpy( sPartitionName,
                                                SDM_NA_STR,
                                                QC_MAX_OBJECT_NAME_LEN + 1 );

                                break;
                            case SDI_COMPOSITE_SHARD_KEY_DIST_OBJECT:
                                /* Composite는 HA를 제공하지 않기 때문에 RP add 하지 않음. */
                                continue;
                            case SDI_CLONE_DIST_OBJECT:
                                /* Clone은 ReverseRP에서만 처리한다. */
                                idlOS::strncpy( sPartitionName,
                                                SDM_NA_STR,
                                                QC_MAX_OBJECT_NAME_LEN + 1 );
                                break;
                            case SDI_NON_SHARD_OBJECT:
                            default:
                                /* Non Shard Table이 오면 안됨. */
                                IDE_DASSERT( 0 );
                                break;
                        }

                        if ( sRangeInfos.mRanges[j].mReplicaSetId == sPrevReplicaSetId )
                        {
                            IDE_TEST( sdm::addReplTable( aStatement,
                                                         aFailoverNodeName,
                                                         sPrevReverseReplName,
                                                         sTableInfo->mUserName,
                                                         sTableInfo->mObjectName,
                                                         sPartitionName,
                                                         SDM_REPL_CLONE,
                                                         ID_FALSE )
                                      != IDE_SUCCESS );
                        }
                        else if ( sRangeInfos.mRanges[j].mReplicaSetId == sPrevPrevReplicaSetId )
                        {
                            IDE_TEST( sdm::addReplTable( aStatement,
                                                         aFailoverNodeName,
                                                         sPrevPrevReverseReplName,
                                                         sTableInfo->mUserName,
                                                         sTableInfo->mObjectName,
                                                         sPartitionName,
                                                         SDM_REPL_CLONE,
                                                         ID_FALSE )
                                      != IDE_SUCCESS );
                        }
                    }
                }
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}


IDE_RC qdsd::failoverAddReplicationItems( qcStatement        * aStatement,
                                          SChar              * aTargetNodeName,
                                          SChar              * aFailoverNodeName,
                                          SChar              * aFailoverNextNodeName,
                                          sdiTableInfoList   * aTableInfoList,
                                          sdiReplicaSetInfo  * aReplicaSetInfo,
                                          sdiNode            * aTargetNodeInfo,
                                          ULong                aSMN )
{
    sdiTableInfoList  * sTmpTableInfoList = NULL;    

    sdiTableInfo      * sTableInfo  = NULL;
    sdiRangeInfo        sRangeInfos;

    sdiReplicaSet     * sOldNodeReplicaSet = NULL;

    sdiShardObjectType   sShardTableType = SDI_NON_SHARD_OBJECT;

    UInt                i;
    UInt                j;

    SChar               sReplName[SDI_REPLICATION_NAME_MAX_SIZE + 1];
    SChar               sPartitionName[QC_MAX_OBJECT_NAME_LEN + 1] = { 0, };

    idBool              sIsPrevNode = ID_FALSE;

    /* Find Old/New Node ReplicaSet */
    for ( i = 0 ; i < aReplicaSetInfo->mCount; i++ )
    {
        if ( idlOS::strncmp( aReplicaSetInfo->mReplicaSets[i].mPrimaryNodeName,
                             aTargetNodeName,
                             SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
        {
            /* TargetNode가 가진 Failover 가능한 ReplicaSet을 찾는다.
             * 2개 이상 가지고 있을수 있으므로 Prev를 먼저 찾는다. */
            if ( idlOS::strncmp( aReplicaSetInfo->mReplicaSets[i].mFirstBackupNodeName,
                                 aFailoverNodeName,
                                 SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
            {
                sOldNodeReplicaSet = &(aReplicaSetInfo->mReplicaSets[i]);
                sIsPrevNode = ID_TRUE;
            }
        }
    }

    if ( sOldNodeReplicaSet == NULL )
    {
        /* Prev ReplicaSet을 못찾았으면 PrevPrev를 찾는다. 
         * 못찾으면 Failover 가능한 ReplicaSet이 없는 것. */
        for ( i = 0 ; i < aReplicaSetInfo->mCount; i++ )
        {
            if ( idlOS::strncmp( aReplicaSetInfo->mReplicaSets[i].mPrimaryNodeName,
                                 aTargetNodeName,
                                 SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
            {
                if ( idlOS::strncmp( aReplicaSetInfo->mReplicaSets[i].mSecondBackupNodeName,
                                     aFailoverNodeName,
                                     SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
                {
                    sOldNodeReplicaSet = &(aReplicaSetInfo->mReplicaSets[i]);
                }
            }
        }
    }

    /* R2HA Clone 처리 필요 */
    /* N1 N2 N3 N4 N5 N6 에서 N1을 N4가 Failover 할 때 ... N1->N4 간에는 이중화가 없다.
     * 이런 Failover 발생시 Failback에서 Fullsync 받던가 뭔가 해야 함. */
    /* N1 N2 N3 N4 N5 N6 에서 N2를 N4가 Failover 할 때 ... N2->N5가 없으니까 이중화 add는 안하지만
     * Clone은 N4->N2 해 주어야 한다... */

    /* 조건에 맞는 ReplicaSet이 없으면 Add할 Item도 없다. */
    if ( sOldNodeReplicaSet == NULL )
    {
        IDE_CONT( normal_exit );
    }

    // 작업이 필요한 Node에만 전파
    /* N1->N3로 보내던 Repl을 N2->N3에 추가 해 주어야 한다. */
    /* Clone Table만 역으로 N2->N1에 추가해 주어야 한다. N2->N1은 N1->N2 즉 N1의 First다 */
    for ( sTmpTableInfoList = aTableInfoList;
          sTmpTableInfoList != NULL;
          sTmpTableInfoList = sTmpTableInfoList->mNext )

    {
        sTableInfo = sTmpTableInfoList->mTableInfo;

        /* Procedure는 RP ADD를 수행하지 않는다. */
        if ( sTableInfo->mObjectType == 'T' )
        {

            /* Default Partition check */
            if ( sTableInfo->mDefaultNodeId == aTargetNodeInfo->mNodeId )
            {
                if ( sdm::checkFailoverAvailable( aReplicaSetInfo,
                                                  sTableInfo->mDefaultPartitionReplicaSetId,
                                                  aFailoverNodeName ) == ID_TRUE )
                {
                    /* FailoverNode와 FailoverNextNode에 모두 Failover 가능할 때만 RP Add 해야 한다. */
                    if ( sdm::checkFailoverAvailable( aReplicaSetInfo,
                                                      sTableInfo->mDefaultPartitionReplicaSetId,
                                                      aFailoverNextNodeName ) == ID_TRUE )
                    {
                        /* FailoverNode->FailoverNextNode로 RP Add 하려면
                         * 항상 FirstBackupNode가 FailoverNode 여야 함. */
                        if ( sIsPrevNode == ID_TRUE )
                        {
                            IDE_TEST( sdm::addReplTable( aStatement,
                                                         aFailoverNodeName,
                                                         sOldNodeReplicaSet->mSecondReplName,
                                                         sTableInfo->mUserName,
                                                         sTableInfo->mObjectName,
                                                         sTableInfo->mDefaultPartitionName,
                                                         SDM_REPL_SENDER,
                                                         ID_FALSE )
                                      != IDE_SUCCESS );
                        }
                    }
                }
                                                  
            }

            /* Range Info Check */
            IDE_TEST( sdm::getRangeInfo( aStatement,
                                         QC_SMI_STMT( aStatement ),
                                         aSMN,
                                         sTableInfo,
                                         &sRangeInfos,
                                         ID_FALSE )
                      != IDE_SUCCESS );

            /* check Table Type */
            sShardTableType = sdi::getShardObjectType( sTableInfo );

            for ( j = 0; j < sRangeInfos.mCount; j++ )
            {
                if ( sRangeInfos.mRanges[j].mNodeId == aTargetNodeInfo->mNodeId )
                {
                    if ( sdm::checkFailoverAvailable( aReplicaSetInfo,
                                                      sRangeInfos.mRanges[j].mReplicaSetId,
                                                      aFailoverNodeName ) == ID_TRUE )
                    {
                        switch ( sShardTableType )
                        {
                            case SDI_SINGLE_SHARD_KEY_DIST_OBJECT:
                                /* Range/Solo의 경우는 Failover 가능이면 추가하면 됨. */
                                idlOS::strncpy( sReplName,
                                                sOldNodeReplicaSet->mSecondReplName,
                                                SDI_REPLICATION_NAME_MAX_SIZE + 1 );

                                idlOS::strncpy( sPartitionName,
                                                sRangeInfos.mRanges[j].mPartitionName,
                                                QC_MAX_OBJECT_NAME_LEN + 1 );
                                break;
                            case SDI_SOLO_DIST_OBJECT:
                                /* Range/Solo의 경우는 Failover 가능이면 추가하면 됨. */
                                idlOS::strncpy( sReplName,
                                                sOldNodeReplicaSet->mSecondReplName,
                                                SDI_REPLICATION_NAME_MAX_SIZE + 1 );

                                idlOS::strncpy( sPartitionName,
                                                SDM_NA_STR,
                                                QC_MAX_OBJECT_NAME_LEN + 1 );

                                break;
                            case SDI_COMPOSITE_SHARD_KEY_DIST_OBJECT:
                                /* Composite는 HA를 제공하지 않기 때문에 RP add 하지 않음. */
                                continue;
                            case SDI_CLONE_DIST_OBJECT:
                                /* Clone은 ReverseRP에만 추가한다. */
                                continue;
                            case SDI_NON_SHARD_OBJECT:
                            default:
                                /* Non Shard Table이 오면 안됨. */
                                IDE_DASSERT( 0 );
                                break;
                        }

                        /* FailoverNode와 FailoverNextNode에 모두 Failover 가능할 때만 RP Add 해야 한다. */
                        /* Clone은 역이중화라서 FailoverNode와 이중화 연결만 있으면 된다. */
                        if ( sdm::checkFailoverAvailable( aReplicaSetInfo,
                                                          sRangeInfos.mRanges[j].mReplicaSetId,
                                                          aFailoverNextNodeName ) == ID_TRUE )
                        {
                            if ( sIsPrevNode == ID_TRUE )
                            {
                                IDE_TEST( sdm::addReplTable( aStatement,
                                                             aFailoverNodeName,
                                                             sReplName,
                                                             sTableInfo->mUserName,
                                                             sTableInfo->mObjectName,
                                                             sPartitionName,
                                                             SDM_REPL_SENDER,
                                                             ID_FALSE )
                                          != IDE_SUCCESS );
                            }
                        }
                    }
                }
            }

        }
    }

    IDE_EXCEPTION_CONT( normal_exit );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC qdsd::unsetShardTableForDrop( qcStatement       * aStatement )
{
    sdiTableInfoList  * sTableInfoList = NULL;
    sdiTableInfoList  * sTmpTableInfoList = NULL;
    sdiLocalMetaInfo    sLocalMetaInfo;
    sdiTableInfo      * sTableInfo = NULL;
    ULong               sSMN = SDI_NULL_SMN;
    const UInt          sSqlStrLen = 4096;
    SChar               sSqlStr[sSqlStrLen] = {0,};
    SChar             * sMyNodeName;

    IDE_TEST( sdi::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );
    sMyNodeName = sLocalMetaInfo.mNodeName;
    sSMN = sdi::getSMNForDataNode();
    IDE_TEST( sdi::getTableInfoAllObject( aStatement,
                                          &sTableInfoList,
                                          sSMN )
              != IDE_SUCCESS );

    for ( sTmpTableInfoList = sTableInfoList;
          sTmpTableInfoList != NULL;
          sTmpTableInfoList = sTmpTableInfoList->mNext )
    {
        sTableInfo = sTmpTableInfoList->mTableInfo;

        IDE_TEST( sdi::checkShardLinker( aStatement ) != IDE_SUCCESS );

        //EXEC DBMS_SHARD.UNSET_SHARD_TABLE_LOCAL('SYS','T1_SOLO' );
        IDE_TEST( executeRemoteSQL( aStatement,
                                    sMyNodeName,
                                    SDI_INTERNAL_OP_NORMAL,
                                    QCI_STMT_MASK_SP,
                                    sSqlStr,
                                    sSqlStrLen,
                                    "EXEC DBMS_SHARD.UNSET_SHARD_TABLE_LOCAL( '"
                                    QCM_SQL_STRING_SKIP_FMT"', '"
                                    QCM_SQL_STRING_SKIP_FMT"'); ",
                                    sTableInfo->mUserName,
                                    sTableInfo->mObjectName )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* PROJ-2748 Shard Failback */
/* 지정된 NodeName으로 부터 Shard Meta를 가져온다. */
IDE_RC qdsd::getShardMetaFromOtherNode( qcStatement * aStatement,
                                        SChar       * aNodeName )
{
    qdShardParseTree  * sParseTree = NULL;
    sdiLocalMetaInfo  * sRemoteLocalMetaInfo = NULL;
    SChar               sSqlStr[QD_MAX_SQL_LENGTH];
    iduListNode       * sNode  = NULL;
    iduListNode       * sDummy = NULL;

    sParseTree = (qdShardParseTree *)aStatement->myPlan->parseTree;
    
    if ( sParseTree->mNodeCount > 1 )
    {
        //원격 노드의 node info
        if ( IDU_LIST_IS_EMPTY( sParseTree->mNodeInfoList ) != ID_TRUE )
        {
            IDU_LIST_ITERATE_SAFE( sParseTree->mNodeInfoList, sNode, sDummy )
            {
                sRemoteLocalMetaInfo = (sdiLocalMetaInfo *)sNode->mObj;
            
                if( idlOS::strncmp( sRemoteLocalMetaInfo->mNodeName,
                                    aNodeName,
                                    SDI_NODE_NAME_MAX_SIZE ) == 0 )
                {
                    break;
                }
            }
        }

        IDE_TEST_RAISE( sRemoteLocalMetaInfo == NULL, ERR_NOT_EXIST_LOCAL_META_INFO );
        
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "DELETE FROM SYS_SHARD.GLOBAL_META_INFO_ " );

        IDE_TEST( sdi::shardExecTempDMLOrDCLWithNewTrans( aStatement,
                                                          sSqlStr )
                  != IDE_SUCCESS );

        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "ALTER REPLICATION TEMPORARY GET TABLE "
                         "SYS_SHARD.NODES_, SYS_SHARD.OBJECTS_, SYS_SHARD.REPLICA_SETS_, "
                         "SYS_SHARD.SOLOS_, SYS_SHARD.CLONES_, SYS_SHARD.GLOBAL_META_INFO_, "
                         "SYS_SHARD.RANGES_,SYS_SHARD.FAILOVER_HISTORY_ "
                         "FROM '%s',%"ID_INT32_FMT" ",
                         sRemoteLocalMetaInfo->mInternalRPHostIP, // internal_host_ip
                         sRemoteLocalMetaInfo->mInternalRPPortNo ); // internal_port_no

        IDE_TEST( sdi::shardExecTempDMLOrDCLWithNewTrans( aStatement,
                                                          sSqlStr )
                  != IDE_SUCCESS );

        // 원격 노드로 부터 얻어온 SNM과 동기화
        IDE_TEST( sdi::reloadSMNForDataNode( QC_SMI_STMT( aStatement ) )
                  != IDE_SUCCESS );
        
        // session SMN
        qci::mSessionCallback.mSetShardMetaNumber( aStatement->session->mMmSession,
                                                   sdi::getSMNForDataNode());
    }
    else
    {
        /* Failback 이니까 Zookeeper의 NodeInfo에 존재 해야만 한다. 0인 경우는 있을 수 없다. */
        IDE_RAISE( ERR_NOT_EXIST_LOCAL_META_INFO );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_EXIST_LOCAL_META_INFO )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "qdsd::getShardMetaFromOtherNode",
                                  "Meta information of remote node does not exist in cluster meta." ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdsd::failbackResetShardMeta( qcStatement        * aStatement,
                                     SChar              * aTargetNodeName,
                                     SChar              * aFailbackFromNodeName,
                                     sdiTableInfoList   * aTableInfoList,
                                     sdiReplicaSetInfo  * aReplicaSetInfo,
                                     sdiReplicaSetInfo  * aFailoverHistoryInfo,
                                     iduList            * aAliveNodeList,
                                     UInt                 aReplicaSetId,
                                     ULong                aSMN )
{
    sdiTableInfoList  * sTmpTableInfoList = NULL;    

    sdiTableInfo      * sTableInfo  = NULL;
    sdiRangeInfo        sRangeInfos;
    sdiReplicaSet     * sFailoverHistory = NULL;

    sdiShardObjectType  sShardTableType = SDI_NON_SHARD_OBJECT;

    UInt                i;
    UInt                j;

    SChar               sSqlStr[QD_MAX_SQL_LENGTH + 1];

    sdiValue            sValueStr;

    SChar               sObjectValue[SDI_RANGE_VARCHAR_MAX_PRECISION + 1];

    sdiNode             sNode; 

    if ( idlOS::strncmp( aTargetNodeName,
                         aFailbackFromNodeName,
                         SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
    {
        IDE_DASSERT( 0 );
    }

    for ( sTmpTableInfoList = aTableInfoList;
          sTmpTableInfoList != NULL;
          sTmpTableInfoList = sTmpTableInfoList->mNext )

    {
        sTableInfo = sTmpTableInfoList->mTableInfo;

        /* check Table Type */
        sShardTableType = sdi::getShardObjectType( sTableInfo );

        if ( sShardTableType == SDI_CLONE_DIST_OBJECT )
        {
            /* Clone이면 Failover 수행시에 무조건 지우므로 무조건 다시 Set 해주어야 한다. */
            /* 그냥 Set이므로 Table/Procedure 상관없다. */
            idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                             "exec dbms_shard.set_shard_clone_local('"
                             QCM_SQL_STRING_SKIP_FMT"', '"
                             QCM_SQL_STRING_SKIP_FMT"', '"
                             QCM_SQL_STRING_SKIP_FMT"', "
                             QCM_SQL_UINT32_FMT", "
                             QCM_SQL_UINT32_FMT") ", // TASK-7307
                             sTableInfo->mUserName,
                             sTableInfo->mObjectName,
                             aTargetNodeName,
                             aReplicaSetId,
                             SDM_CALLED_BY_SHARD_FAILBACK );

            IDE_TEST( executeSQLNodeList( aStatement,
                                          SDI_INTERNAL_OP_FAILBACK,
                                          QCI_STMT_MASK_SP,
                                          sSqlStr,
                                          NULL,
                                          aAliveNodeList ) != IDE_SUCCESS );
        }
        else
        {
            for ( i = 0; i < aFailoverHistoryInfo->mCount; i++ )
            {
                sFailoverHistory = &aFailoverHistoryInfo->mReplicaSets[i];

                /* Default Partition check */
                if ( sTableInfo->mDefaultPartitionReplicaSetId == sFailoverHistory->mReplicaSetId )
                {
                    /* FailoverHistory의 Primary랑 비교하면 안됨,
                     * 현재 ReplicaSet의 Primary랑 비교해야됨. */
                    if ( sdm::checkFailbackAvailable( aReplicaSetInfo,
                                                      sTableInfo->mDefaultPartitionReplicaSetId,
                                                      aTargetNodeName ) == ID_TRUE )
                    {
                        SDI_INIT_NODE(&sNode);
                        IDE_TEST( sdm::getNodeByID( QC_SMI_STMT( aStatement ),
                                                    sTableInfo->mDefaultNodeId,
                                                    aSMN,
                                                    &sNode )
                                  != IDE_SUCCESS );

                        /* Send resetShardPartitioinNode to All Alive Node */
                        if ( sTableInfo->mObjectType == 'T' ) /* Table */
                        {

                            idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                             "exec dbms_shard.reset_shard_partition_node( '"
                                             QCM_SQL_STRING_SKIP_FMT"', '"
                                             QCM_SQL_STRING_SKIP_FMT"', '"
                                             QCM_SQL_STRING_SKIP_FMT"', '"
                                             QCM_SQL_STRING_SKIP_FMT"', "
                                             QCM_SQL_VARCHAR_FMT", "
                                             QCM_SQL_UINT32_FMT") ", // TASK-7307
                                             sTableInfo->mUserName,
                                             sTableInfo->mObjectName,
                                             sNode.mNodeName,
                                             aTargetNodeName,
                                             sTableInfo->mDefaultPartitionName,
                                             SDM_CALLED_BY_SHARD_FAILBACK );
                        }
                        else /* Procedure */
                        {
                            idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                             "exec dbms_shard.reset_shard_resident_node( '"
                                             QCM_SQL_STRING_SKIP_FMT"', '"
                                             QCM_SQL_STRING_SKIP_FMT"', '"
                                             QCM_SQL_STRING_SKIP_FMT"', '"
                                             QCM_SQL_STRING_SKIP_FMT"', "
                                             QCM_SQL_VARCHAR_FMT", "
                                             QCM_SQL_VARCHAR_FMT", "
                                             QCM_SQL_UINT32_FMT" )",
                                             sTableInfo->mUserName,
                                             sTableInfo->mObjectName,
                                             sNode.mNodeName,
                                             aTargetNodeName,
                                             "",
                                             "",
                                             1 );
                        }

                        IDE_TEST( executeSQLNodeList( aStatement,
                                                      SDI_INTERNAL_OP_FAILBACK,
                                                      QCI_STMT_MASK_SP,
                                                      sSqlStr,
                                                      NULL,
                                                      aAliveNodeList ) != IDE_SUCCESS );

                    }
                    else
                    {
                        /* ReplicaSet이 FailbackFromNode가 아니면 Failover시 DataFailover가 발생하지 않았다.
                         * 그냥 두면 된다. */
                    }
                }

                /* Range Info Check */
                IDE_TEST( sdm::getRangeInfo( aStatement,
                                             QC_SMI_STMT( aStatement ),
                                             aSMN,
                                             sTableInfo,
                                             &sRangeInfos,
                                             ID_FALSE )
                          != IDE_SUCCESS );

                /* check Table Type */
                sShardTableType = sdi::getShardObjectType( sTableInfo );

                for ( j = 0; j < sRangeInfos.mCount; j++ )
                {

                    if ( sRangeInfos.mRanges[j].mReplicaSetId == sFailoverHistory->mReplicaSetId )
                    {
                        if ( sdm::checkFailbackAvailable( aReplicaSetInfo,
                                                          sRangeInfos.mRanges[j].mReplicaSetId,
                                                          aTargetNodeName ) == ID_TRUE )
                        {
                            SDI_INIT_NODE(&sNode);
                            IDE_TEST( sdm::getNodeByID( QC_SMI_STMT( aStatement ),
                                                        sRangeInfos.mRanges[j].mNodeId,
                                                        aSMN,
                                                        &sNode )
                                      != IDE_SUCCESS );

                            switch ( sShardTableType )
                            {
                                case SDI_SINGLE_SHARD_KEY_DIST_OBJECT:
                                case SDI_SOLO_DIST_OBJECT:
                                    /* send resetShardPartitionNode to All Alive Node */
                                    if ( sTableInfo->mObjectType == 'T' ) /* Table */
                                    {
                                        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                                         "exec dbms_shard.reset_shard_partition_node('"
                                                         QCM_SQL_STRING_SKIP_FMT"', '"
                                                         QCM_SQL_STRING_SKIP_FMT"', '"
                                                         QCM_SQL_STRING_SKIP_FMT"', '"
                                                         QCM_SQL_STRING_SKIP_FMT"', "
                                                         QCM_SQL_VARCHAR_FMT", "
                                                         QCM_SQL_UINT32_FMT") ", // TASK-7307
                                                         sTableInfo->mUserName,
                                                         sTableInfo->mObjectName,
                                                         sNode.mNodeName,
                                                         aTargetNodeName,
                                                         sRangeInfos.mRanges[j].mPartitionName,
                                                         SDM_CALLED_BY_SHARD_FAILBACK );
                                    }
                                    else /* Procedure */
                                    {
                                        if ( sTableInfo->mSplitMethod == SDI_SPLIT_HASH )
                                        {
                                            IDE_TEST( sdi::getValueStr( MTD_INTEGER_ID,
                                                                        &sRangeInfos.mRanges[j].mValue,
                                                                        &sValueStr )
                                                      != IDE_SUCCESS );
                                        }
                                        else
                                        {
                                            IDE_TEST( sdi::getValueStr( sTableInfo->mKeyDataType,
                                                                        &sRangeInfos.mRanges[j].mValue,
                                                                        &sValueStr )
                                                      != IDE_SUCCESS );
                                        }

                                        if ( sValueStr.mCharMax.value[0] == '\'' )
                                        {
                                            // INPUT ARG ('''A''') => 'A' => '''A'''
                                            idlOS::snprintf( sObjectValue,
                                                             SDI_RANGE_VARCHAR_MAX_PRECISION + 1,
                                                             "''%s''",
                                                             sValueStr.mCharMax.value );
                                        }
                                        else
                                        {
                                            // INPUT ARG ('A') => A => 'A'
                                            idlOS::snprintf( sObjectValue,
                                                             SDI_RANGE_VARCHAR_MAX_PRECISION + 1,
                                                             "'%s'",
                                                             sValueStr.mCharMax.value );
                                        }

                                        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                                         "exec dbms_shard.reset_shard_resident_node('"
                                                         QCM_SQL_STRING_SKIP_FMT"', '"
                                                         QCM_SQL_STRING_SKIP_FMT"', '"
                                                         QCM_SQL_STRING_SKIP_FMT"', '"
                                                         QCM_SQL_STRING_SKIP_FMT"', "
                                                         "%s, "
                                                         QCM_SQL_VARCHAR_FMT" )",
                                                         sTableInfo->mUserName,
                                                         sTableInfo->mObjectName,
                                                         sNode.mNodeName,
                                                         aTargetNodeName,
                                                         sObjectValue,
                                                         "" );
                                    }

                                    IDE_TEST( executeSQLNodeList( aStatement,
                                                                  SDI_INTERNAL_OP_FAILBACK,
                                                                  QCI_STMT_MASK_SP,
                                                                  sSqlStr,
                                                                  NULL,
                                                                  aAliveNodeList ) != IDE_SUCCESS );
                                    break;
                                case SDI_COMPOSITE_SHARD_KEY_DIST_OBJECT:
                                    /* Composite는 HA를 제공하지 않기 때문에 Failover 대상이 아니다. */
                                    continue;
                                case SDI_CLONE_DIST_OBJECT:
                                    /* Clone은 따로 처리 한다. */
                                    /* 여기 들어오면 안된다. */
                                    IDE_DASSERT( 0 );
                                    continue;
                                case SDI_NON_SHARD_OBJECT:
                                default:
                                    /* Non Shard Table이 오면 안됨. */
                                    IDE_DASSERT( 0 );
                                    break;
                            }
                        }
                        else
                        {
                            /* Failback 대상이 아니면 아무것도 안한다. */
                        }
                    }
                }
            }

        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdsd::lockTables4Failback( qcStatement        * aStatement,
                                  SChar              * aSQLBuf,
                                  UInt                 aSQLBufSize,
                                  SChar              * aFailbackFromNodeName,
                                  sdiTableInfoList   * aTableInfoList,
                                  sdiReplicaSetInfo  * aReplicaSetInfo,
                                  sdiReplicaSetInfo  * aFailoverHistoryInfo,
                                  ULong                aSMN )
{
    UInt               sShardDDLLockTryCount = QCG_GET_SESSION_SHARD_DDL_LOCK_TRY_COUNT(aStatement);
    UInt               sRemoteLockTimeout = QCG_GET_SESSION_SHARD_DDL_LOCK_TIMEOUT(aStatement);
    UInt               sLoopCnt = 0;

    idBool             sIsCancel = ID_FALSE;
    idBool             sIsLockAcquired = ID_FALSE;
    idBool             sIsLocked = ID_FALSE;

    sdiTableInfoList * sTmpTableInfoList = NULL;
    sdiTableInfo     * sTableInfo = NULL;
    sdiReplicaSet    * sFailoverHistory = NULL;

    sdiRangeInfo       sRangeInfos;

    sdiShardObjectType sShardTableType = SDI_NON_SHARD_OBJECT;
   
    UInt               i;
    UInt               j;
    SChar              sBufferNameStr[QC_MAX_OBJECT_NAME_LEN + 1];
    
    /* the reomte cancel is not running. therefore remote lock timeout property sec(shard_ddl_lock_timeout).
     * if a application lock order is A->B->C then executing reshard table should be in the order A, B and C 
     * if A table is locked then the next table lock may will be acquired.
     */
    sLoopCnt = 0;
    while ( (sLoopCnt++ < sShardDDLLockTryCount) && (sIsLockAcquired != ID_TRUE ) )
    {
        sIsCancel = ID_FALSE;
        sIsLockAcquired = ID_FALSE;

        IDE_TEST( executeRemoteSQL( aStatement, 
                                    aFailbackFromNodeName,
                                    SDI_INTERNAL_OP_FAILBACK,
                                    QCI_STMT_MASK_SP,
                                    aSQLBuf,
                                    aSQLBufSize,
                                    "SAVEPOINT __FAILBACK_SVP0") != IDE_SUCCESS);

        for ( sTmpTableInfoList = aTableInfoList;
              sTmpTableInfoList != NULL;
              sTmpTableInfoList = sTmpTableInfoList->mNext )
        {
            sIsLocked = ID_FALSE;
            sTableInfo = sTmpTableInfoList->mTableInfo;

            if ( sTableInfo->mObjectType == 'T' )
            {
                idlOS::snprintf( sBufferNameStr,
                                 QC_MAX_OBJECT_NAME_LEN + 1,
                                 "%s%s",
                                 SDI_BACKUP_TABLE_PREFIX,
                                 sTableInfo->mObjectName );

                for ( i = 0; i < aFailoverHistoryInfo->mCount; i++ )
                {
                    sFailoverHistory = &aFailoverHistoryInfo->mReplicaSets[i];

                    /* Default Partition check */
                    if ( sTableInfo->mDefaultPartitionReplicaSetId == sFailoverHistory->mReplicaSetId )
                    {
                        if ( sdm::checkFailbackAvailable( aReplicaSetInfo,
                                                          sTableInfo->mDefaultPartitionReplicaSetId,
                                                          sdiZookeeper::mMyNodeName ) == ID_TRUE )
                        {
                            /* R2HA 현재 Remote Lock 만잡고 있다(Service Node) Local Node의 Lock이 필요할수도 있다. */
                            /* Swap 대상이라면 Lock을 획득해야 한다. */
                            /* Default Partition Lock 획득 */
                            if( executeRemoteSQL( aStatement, 
                                                  aFailbackFromNodeName,
                                                  SDI_INTERNAL_OP_FAILBACK,
                                                  QCI_STMT_MASK_DML,
                                                  aSQLBuf,
                                                  aSQLBufSize,
                                                  "LOCK TABLE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" IN EXCLUSIVE MODE WAIT %"ID_UINT32_FMT,

    //                                              "LOCK TABLE %s.%s PARTITION ( %s ) IN EXCLUSIVE MODE WAIT %"ID_UINT32_FMT,
                                                  sTableInfo->mUserName,
                                                  sTableInfo->mObjectName,
    //                                              sTableInfo->mDefaultPartitionName,
                                                  sRemoteLockTimeout ) != IDE_SUCCESS )
                            {
                                /* cancel retry */
                                sIsCancel = ID_TRUE;
                                break;
                            }
                            /* _BAK_Table Default Partition Lock 획득 */
                            if( executeRemoteSQL( aStatement, 
                                                  aFailbackFromNodeName,
                                                  SDI_INTERNAL_OP_FAILBACK,
                                                  QCI_STMT_MASK_DML,
                                                  aSQLBuf,
                                                  aSQLBufSize,
                                                  "LOCK TABLE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" IN EXCLUSIVE MODE WAIT %"ID_UINT32_FMT,
    //                                              "LOCK TABLE %s._BAK_%s PARTITION ( %s ) IN EXCLUSIVE MODE WAIT %"ID_UINT32_FMT,
                                                  sTableInfo->mUserName,
                                                  sBufferNameStr,
    //                                              sTableInfo->mDefaultPartitionName,
                                                  sRemoteLockTimeout ) != IDE_SUCCESS )
                            {
                                /* cancel retry */
                                sIsCancel = ID_TRUE;
                                break;
                            }

                            /* Table lock 획득 성공, 다음 Table을 확인한다. */
                            continue;
                        }
                        else
                        {
                            /* Swap 대상이 아니면 Lock 잡을 필요가 없다. */
                        }
                    }

                    /* Range Info Check */
                    IDE_TEST( sdm::getRangeInfo( aStatement,
                                                 QC_SMI_STMT( aStatement ),
                                                 aSMN,
                                                 sTableInfo,
                                                 &sRangeInfos,
                                                 ID_FALSE )
                              != IDE_SUCCESS );

                    /* check Table Type */
                    sShardTableType = sdi::getShardObjectType( sTableInfo );

                    for ( j = 0; j < sRangeInfos.mCount; j++ )
                    {

                        if ( sRangeInfos.mRanges[j].mReplicaSetId == sFailoverHistory->mReplicaSetId )
                        {
                            if ( sdm::checkFailbackAvailable( aReplicaSetInfo,
                                                              sRangeInfos.mRanges[j].mReplicaSetId,
                                                              sdiZookeeper::mMyNodeName ) == ID_TRUE )
                            {
                                switch ( sShardTableType )
                                {
                                    case SDI_SINGLE_SHARD_KEY_DIST_OBJECT:
                                        /* Partition Swap 대상이다. Partition lock 획득한다. */
                                        /* Partition Lock 획득 */
                                        if( executeRemoteSQL( aStatement, 
                                                              aFailbackFromNodeName,
                                                              SDI_INTERNAL_OP_FAILBACK,
                                                              QCI_STMT_MASK_DML,
                                                              aSQLBuf,
                                                              aSQLBufSize,
                                                              "LOCK TABLE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" IN EXCLUSIVE MODE WAIT %"ID_UINT32_FMT,
    //                                                          "LOCK TABLE %s.%s PARTITION ( %s ) IN EXCLUSIVE MODE WAIT %"ID_UINT32_FMT,
                                                              sTableInfo->mUserName,
                                                              sTableInfo->mObjectName,
    //                                                          sRangeInfos.mRanges[j].mPartitionName,
                                                              sRemoteLockTimeout ) != IDE_SUCCESS )
                                        {
                                            /* cancel retry */
                                            sIsCancel = ID_TRUE;
                                            break;
                                        }
                                        /* _BAK_ Table Partition Lock 획득 */
                                        if( executeRemoteSQL( aStatement, 
                                                              aFailbackFromNodeName,
                                                              SDI_INTERNAL_OP_FAILBACK,
                                                              QCI_STMT_MASK_DML,
                                                              aSQLBuf,
                                                              aSQLBufSize,
                                                              "LOCK TABLE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" IN EXCLUSIVE MODE WAIT %"ID_UINT32_FMT,
    //                                                          "LOCK TABLE %s._BAK_%s PARTITION ( %s ) IN EXCLUSIVE MODE WAIT %"ID_UINT32_FMT,
                                                              sTableInfo->mUserName,
                                                              sBufferNameStr,
    //                                                          sRangeInfos.mRanges[j].mPartitionName,
                                                              sRemoteLockTimeout ) != IDE_SUCCESS )
                                        {
                                            /* cancel retry */
                                            sIsCancel = ID_TRUE;
                                            break;
                                        }
                                        sIsLocked = ID_TRUE;
                                        break;
                                    case SDI_SOLO_DIST_OBJECT:
                                        /* Table Swap 대상이다. Table lock 획득한다. */
                                        /* Table Lock 획득 */
                                        if( executeRemoteSQL( aStatement, 
                                                              aFailbackFromNodeName,
                                                              SDI_INTERNAL_OP_FAILBACK,
                                                              QCI_STMT_MASK_DML,
                                                              aSQLBuf,
                                                              aSQLBufSize,
                                                              "LOCK TABLE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" IN EXCLUSIVE MODE WAIT %"ID_UINT32_FMT,
                                                              sTableInfo->mUserName,
                                                              sTableInfo->mObjectName,
                                                              sRemoteLockTimeout ) != IDE_SUCCESS )
                                        {
                                            /* cancel retry */
                                            sIsCancel = ID_TRUE;
                                            break;
                                        }
                                        /* _BAK_ Table Lock 획득 */
                                        if( executeRemoteSQL( aStatement, 
                                                              aFailbackFromNodeName,
                                                              SDI_INTERNAL_OP_FAILBACK,
                                                              QCI_STMT_MASK_DML,
                                                              aSQLBuf,
                                                              aSQLBufSize,
                                                              "LOCK TABLE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" IN EXCLUSIVE MODE WAIT %"ID_UINT32_FMT,
                                                              sTableInfo->mUserName,
                                                              sBufferNameStr,
                                                              sRemoteLockTimeout ) != IDE_SUCCESS )
                                        {
                                            /* cancel retry */
                                            sIsCancel = ID_TRUE;
                                            break;
                                        }
                                        sIsLocked = ID_TRUE;
                                        break;
                                    case SDI_COMPOSITE_SHARD_KEY_DIST_OBJECT:
                                        /* Composite는 HA를 제공하지 않기 때문에 Partition Swap 안함.
                                         * Lock 잡을 필요 없음. */
                                        continue;
                                    case SDI_CLONE_DIST_OBJECT:
                                        /* Clone 은 전 Node에 Primary로 존재하기 때문에 Partition Swap 할 필요 없음.
                                         * Lock 잡을 필요 없음. */
                                        /* R2HA Lock 잡을 필요가 있을수도 있다. Service 중지를 위해? */
                                        continue;
                                    case SDI_NON_SHARD_OBJECT:
                                    default:
                                        /* Non Shard Table이 오면 안됨. */
                                        IDE_DASSERT( 0 );
                                        break;
                                }

                            }
                            else
                            {
                                /* Swap 대상이 아니면 Lock 잡을 필요가 없다. */
                            }
                        }

                        /* Table Lock을 잡았으면 다음 Table Lock을 획득하러 간다.
                         * Range Info 탐색 중에 Lock 획득에 실패했으면 Retry 할것이다. */
                        if ( ( sIsCancel == ID_TRUE ) || ( sIsLocked == ID_TRUE ) )
                        {
                            break;
                        }
                    }

                    if ( sIsCancel == ID_TRUE )
                    {
                        /* RangeInfo 탐색중에 Lock 획득에 실패했다. */
                        break;
                    }
                }
            }
            else
            {
                /* R2HA PROCEDURE LOCK NEED */
            }
        }
        if ( sIsCancel == ID_TRUE )
        {
            IDE_TEST( executeRemoteSQL( aStatement, 
                                        aFailbackFromNodeName,
                                        SDI_INTERNAL_OP_FAILBACK,
                                        QCI_STMT_MASK_SP,
                                        aSQLBuf,
                                        aSQLBufSize,
                                        "ROLLBACK TO SAVEPOINT __FAILBACK_SVP0") != IDE_SUCCESS );
            sIsLockAcquired = ID_FALSE;
        }
        else
        {
            sIsLockAcquired = ID_TRUE;
            break;
        }
    }

    IDE_TEST_RAISE( sIsLockAcquired != ID_TRUE, ERR_FAILBACKLOCK );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_FAILBACKLOCK )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_LOCK_FAIL ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    (void)executeRemoteSQL( aStatement, 
                            aFailbackFromNodeName,
                            SDI_INTERNAL_OP_FAILBACK,
                            QCI_STMT_MASK_SP,
                            aSQLBuf,
                            aSQLBufSize,
                            "ROLLBACK TO SAVEPOINT __FAILBACK_SVP0" );
    IDE_POP();
    return IDE_FAILURE;
}

IDE_RC qdsd::failbackTablePartitionSwap( qcStatement        * aStatement,
                                         SChar              * aFailbackFromNodeName,
                                         sdiTableInfoList   * aTableInfoList,
                                         sdiReplicaSetInfo  * aReplicaSetInfo,
                                         sdiReplicaSetInfo  * aFailoverHistoryInfo,
                                         ULong                aSMN )
{
    sdiTableInfoList  * sTmpTableInfoList = NULL;    

    sdiTableInfo      * sTableInfo  = NULL;
    sdiRangeInfo        sRangeInfos;
    sdiReplicaSet     * sFailoverHistory = NULL;

    sdiShardObjectType  sShardTableType = SDI_NON_SHARD_OBJECT;
    
    UInt                i;
    UInt                j;

    SChar               sSqlStr[QD_MAX_SQL_LENGTH + 1];
    SChar               sBufferNameStr[QC_MAX_OBJECT_NAME_LEN + 1];
    
    for ( sTmpTableInfoList = aTableInfoList;
          sTmpTableInfoList != NULL;
          sTmpTableInfoList = sTmpTableInfoList->mNext )

    {
        sTableInfo = sTmpTableInfoList->mTableInfo;

        /* Procedure는 Swap할 Partition이 없다. */
        if ( sTableInfo->mObjectType == 'T' )
        {
            idlOS::snprintf( sBufferNameStr,
                             QC_MAX_OBJECT_NAME_LEN + 1,
                             "%s%s",
                             SDI_BACKUP_TABLE_PREFIX,
                             sTableInfo->mObjectName );

            for ( i = 0; i < aFailoverHistoryInfo->mCount; i++ )
            {
                sFailoverHistory = &aFailoverHistoryInfo->mReplicaSets[i];

                /* Default Partition check */
                if ( sTableInfo->mDefaultPartitionReplicaSetId == sFailoverHistory->mReplicaSetId )
                {
                    if ( sdm::checkFailbackAvailable( aReplicaSetInfo,
                                                      sTableInfo->mDefaultPartitionReplicaSetId,
                                                      sdiZookeeper::mMyNodeName ) == ID_TRUE )
                    {
                        /* Partition Swap Send to TargetNode */
                        idlOS::snprintf( sSqlStr, 
                                         QD_MAX_SQL_LENGTH,
                                         "ALTER TABLE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" REPLACE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" PARTITION "QCM_SQL_STRING_SKIP_FMT"",
                                         sTableInfo->mUserName,
                                         sBufferNameStr,
                                         sTableInfo->mUserName,
                                         sTableInfo->mObjectName,
                                         sTableInfo->mDefaultPartitionName );

                        IDE_TEST( executeRemoteSQL( aStatement, 
                                                    aFailbackFromNodeName,
                                                    SDI_INTERNAL_OP_FAILBACK,
                                                    QCI_STMT_MASK_DDL,
                                                    sSqlStr,
                                                    QD_MAX_SQL_LENGTH,
                                                    NULL )
                                    != IDE_SUCCESS );
                    }
                    else
                    {
                        /* Failback 할수 없다면 Swap 하지 않는다. */
                    }
                }

                /* Range Info Check */
                IDE_TEST( sdm::getRangeInfo( aStatement,
                                             QC_SMI_STMT( aStatement ),
                                             aSMN,
                                             sTableInfo,
                                             &sRangeInfos,
                                             ID_FALSE )
                          != IDE_SUCCESS );

                /* check Table Type */
                sShardTableType = sdi::getShardObjectType( sTableInfo );

                for ( j = 0; j < sRangeInfos.mCount; j++ )
                {

                    if ( sRangeInfos.mRanges[j].mReplicaSetId == sFailoverHistory->mReplicaSetId )
                    {
                        if ( sdm::checkFailbackAvailable( aReplicaSetInfo,
                                                          sRangeInfos.mRanges[j].mReplicaSetId,
                                                          sdiZookeeper::mMyNodeName ) == ID_TRUE )
                        {
                            switch ( sShardTableType )
                            {
                                case SDI_SINGLE_SHARD_KEY_DIST_OBJECT:
                                    /* Partition Swap */
                                    idlOS::snprintf( sSqlStr, 
                                                     QD_MAX_SQL_LENGTH,
                                                     "ALTER TABLE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" REPLACE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" PARTITION "QCM_SQL_STRING_SKIP_FMT"",
                                                     sTableInfo->mUserName,
                                                     sBufferNameStr,
                                                     sTableInfo->mUserName,
                                                     sTableInfo->mObjectName,
                                                     sRangeInfos.mRanges[j].mPartitionName );
                                    break;
                                case SDI_SOLO_DIST_OBJECT:
                                    /* Partition Swap */
                                    idlOS::snprintf( sSqlStr, 
                                                     QD_MAX_SQL_LENGTH,
                                                     "ALTER TABLE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" REPLACE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT"",
                                                     sTableInfo->mUserName,
                                                     sBufferNameStr,
                                                     sTableInfo->mUserName,
                                                     sTableInfo->mObjectName );

                                    break;
                                case SDI_COMPOSITE_SHARD_KEY_DIST_OBJECT:
                                    /* Composite는 HA를 제공하지 않기 때문에 Partition Swap 할 필요 없음. */
                                    continue;
                                case SDI_CLONE_DIST_OBJECT:
                                    /* Clone 은 전 Node에 Primary로 존재하기 때문에 Partition Swap 할 필요 없음. */
                                    continue;
                                case SDI_NON_SHARD_OBJECT:
                                default:
                                    /* Non Shard Table이 오면 안됨. */
                                    IDE_DASSERT( 0 );
                                    break;
                            }
                            IDE_TEST( executeRemoteSQL( aStatement, 
                                                        aFailbackFromNodeName,
                                                        SDI_INTERNAL_OP_FAILBACK,
                                                        QCI_STMT_MASK_DDL,
                                                        sSqlStr,
                                                        QD_MAX_SQL_LENGTH,
                                                        NULL )
                                        != IDE_SUCCESS );
                        }
                        else
                        {
                            /* Failover 할수 없다면 Swap 하지 않는다. */
                        }
                    }
                }
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdsd::failbackOrganizeReverseRP( qcStatement        * aStatement,
                                        SChar              * aFailbackFromNodeName,
                                        sdiTableInfoList   * aTableInfoList,
                                        sdiReplicaSetInfo  * aFailoverHistoryInfo,
                                        UInt                 aMainReplicaSetId,
                                        ULong                aSMN )
{
    sdiTableInfoList  * sTmpTableInfoList = NULL;    
    sdiTableInfo      * sTableInfo  = NULL;
    sdiRangeInfo        sRangeInfos;

    sdiShardObjectType  sShardTableType = SDI_NON_SHARD_OBJECT;

    SChar               sReverseReplName[SDI_REPLICATION_NAME_MAX_SIZE + 1];
    SChar               sPartitionName[QC_MAX_OBJECT_NAME_LEN + 1] = { 0, };

    sdiReplicaSet     * sReplicaSet = NULL;

    qdShardParseTree  * sParseTree = NULL;

    UInt                i;
    UInt                j;

    sdiLocalMetaInfo  * sNodeMetaInfo = NULL;

    UInt                sExecCount;
    ULong               sCount = 0;

    SChar               sSqlStr[QD_MAX_SQL_LENGTH + 1];

    idBool              sFetchResult = ID_FALSE;

    sParseTree = (qdShardParseTree *)aStatement->myPlan->parseTree;

    ideLog::log( IDE_SD_0, "[SHARD_FAILBACK] Start REVERSE REP" );

    /* Reverse RP Create + Add Item */
    for ( i = 0; i < aFailoverHistoryInfo->mCount; i++ )
    {
        sReplicaSet = &aFailoverHistoryInfo->mReplicaSets[i];

        if ( idlOS::strncmp( sReplicaSet->mFirstBackupNodeName,
                             aFailbackFromNodeName,
                             SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
        {
            idlOS::snprintf( sReverseReplName, 
                             SDI_REPLICATION_NAME_MAX_SIZE + 1,
                             "R_%s", 
                             sReplicaSet->mFirstReplName );
        }
        else if ( idlOS::strncmp( sReplicaSet->mSecondBackupNodeName,
                                  aFailbackFromNodeName,
                                  SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
        {
            idlOS::snprintf( sReverseReplName, 
                             SDI_REPLICATION_NAME_MAX_SIZE + 1,
                             "R_%s", 
                             sReplicaSet->mSecondReplName );
        }
        else
        {
            idlOS::snprintf( sReverseReplName,
                             SDI_REPLICATION_NAME_MAX_SIZE + 1,
                             SDM_NA_STR );
        }

        if ( idlOS::strncmp( sReverseReplName,
                             SDM_NA_STR,
                             SDI_REPLICATION_NAME_MAX_SIZE + 1 ) != 0 )
        {
            /* DROP FORCE의 추가로 ReverseRP가 존재한다고 확신할수 없다.
             * 확인 해 보고 존재하면 ReverseRP를 처리한다. */
            idlOS::snprintf( sSqlStr, 
                             QD_MAX_SQL_LENGTH + 1,
                             "select Count(*) from system_.sys_replications_ where replication_name='%s' ",
                             sReverseReplName );

            IDE_TEST( sdi::shardExecDirect( aStatement,
                                            aFailbackFromNodeName,
                                            (SChar*)sSqlStr,
                                            (UInt) idlOS::strlen(sSqlStr),
                                            SDI_INTERNAL_OP_FAILBACK,
                                            &sExecCount,
                                            &sCount,
                                            NULL,
                                            ID_SIZEOF(sCount),
                                            &sFetchResult )
                      != IDE_SUCCESS );

            if ( sCount != 0 )
            {
                sNodeMetaInfo = sdiZookeeper::getNodeInfo( sParseTree->mNodeInfoList,
                                                           aFailbackFromNodeName );

                IDE_TEST( createReverseReplication( aStatement,
                                                    sdiZookeeper::mMyNodeName,
                                                    sReverseReplName,
                                                    sNodeMetaInfo->mNodeName,
                                                    sNodeMetaInfo->mInternalRPHostIP,
                                                    sNodeMetaInfo->mInternalRPPortNo,
                                                    SDI_REPL_RECEIVER )
                          != IDE_SUCCESS );

                ideLog::log(IDE_SD_21,"[SHARD_FAILBACK] Reverser Replication Created : %s", sReverseReplName );

                /* RP Add */
                for ( sTmpTableInfoList = aTableInfoList;
                      sTmpTableInfoList != NULL;
                      sTmpTableInfoList = sTmpTableInfoList->mNext )
                {
                    sTableInfo = sTmpTableInfoList->mTableInfo;
                    if ( sTableInfo->mObjectType == 'T' )
                    {
                        if ( sTableInfo->mDefaultPartitionReplicaSetId == sReplicaSet->mReplicaSetId )
                        {
                            IDE_TEST( sdm::addReplTable( aStatement,
                                                         sdiZookeeper::mMyNodeName,
                                                         sReverseReplName,
                                                         sTableInfo->mUserName,
                                                         sTableInfo->mObjectName,
                                                         sTableInfo->mDefaultPartitionName,
                                                         SDM_REPL_CLONE,
                                                         ID_TRUE )
                                      != IDE_SUCCESS );
                        }
                        /* Range Info Check */
                        IDE_TEST( sdm::getRangeInfo( aStatement,
                                                     QC_SMI_STMT( aStatement ),
                                                     aSMN,
                                                     sTableInfo,
                                                     &sRangeInfos,
                                                     ID_FALSE )
                                  != IDE_SUCCESS );

                        sShardTableType = sdi::getShardObjectType( sTableInfo );

                        /* CLONE은 Failover 시에 CLONES_에서 제거 되었기 때문에 따로 처리한다. */
                        if ( sShardTableType == SDI_CLONE_DIST_OBJECT )
                        {
                            /* 원래 연결되어 있던 ReplicaSet일때만 Clone 처리한다. */
                            if ( aMainReplicaSetId == sReplicaSet->mReplicaSetId )
                            {
                                /* CLONE은 PartitionName이 없다. */
                                idlOS::strncpy( sPartitionName,
                                                SDM_NA_STR,
                                                QC_MAX_OBJECT_NAME_LEN + 1 );

                                IDE_TEST( sdm::addReplTable( aStatement,
                                                             sdiZookeeper::mMyNodeName,
                                                             sReverseReplName,
                                                             sTableInfo->mUserName,
                                                             sTableInfo->mObjectName,
                                                             sPartitionName,
                                                             SDM_REPL_CLONE,
                                                             ID_TRUE )
                                          != IDE_SUCCESS );
                            }
                        }
                        else
                        {
                            for ( j = 0; j< sRangeInfos.mCount; j++ )
                            {
                                if ( sRangeInfos.mRanges[j].mReplicaSetId == sReplicaSet->mReplicaSetId )
                                {
                                    switch( sShardTableType )
                                    {
                                        case SDI_SINGLE_SHARD_KEY_DIST_OBJECT:
                                            /* Range는 Partition이 존재한다. */
                                            idlOS::strncpy( sPartitionName,
                                                            sRangeInfos.mRanges[j].mPartitionName,
                                                            QC_MAX_OBJECT_NAME_LEN + 1 );
                                            break;
                                        case SDI_SOLO_DIST_OBJECT:
                                            /* SOLO은 PartitionName이 없다. */
                                            idlOS::strncpy( sPartitionName,
                                                            SDM_NA_STR,
                                                            QC_MAX_OBJECT_NAME_LEN + 1 );

                                            break;
                                        case SDI_COMPOSITE_SHARD_KEY_DIST_OBJECT:
                                            /* Composite는 HA를 제공하지 않기 때문에 RP add 하지 않음. */
                                            continue;
                                        case SDI_CLONE_DIST_OBJECT:
                                            /* Clone은 따로 처리 하였음. */
                                        case SDI_NON_SHARD_OBJECT:
                                        default:
                                            /* Non Shard Table이 오면 안됨. */
                                            IDE_DASSERT( 0 );
                                            break;
                                    }
                                    IDE_TEST( sdm::addReplTable( aStatement,
                                                                 sdiZookeeper::mMyNodeName,
                                                                 sReverseReplName,
                                                                 sTableInfo->mUserName,
                                                                 sTableInfo->mObjectName,
                                                                 sPartitionName,
                                                                 SDM_REPL_CLONE,
                                                                 ID_TRUE )
                                              != IDE_SUCCESS );
                                }
                            }
                        }
                    }
                }

                /* Reverse RP Create와 Item Add 가 끝났으면 Flush 받아야 한다. */
                /* Receiver Create이기 때문에 Sender가 이미 Send 중일수 있다. Flush로 확인 한다.
                 * Reverse RP는 Sender 생성시 Start 해두지 않지만
                 * Failback에서 Start 요청 후 KILL 된 후 다시 시도일수 있다. */
                if( flushReplicationForInternal( aStatement,
                                                 aFailbackFromNodeName,
                                                 sReverseReplName )
                    == IDE_SUCCESS )
                {
                    ideLog::log(IDE_SD_0,"[SHARD_FAILBACK] FLUSH REVERSE REP COMPLETE %s", sReverseReplName);
                }
                else
                {
                    /* Flush가 실패하였다면 start 안되어 있는걸로 보고 Start 시킨다. */
                    IDE_TEST( startReplicationForInternal( aStatement,
                                                           aFailbackFromNodeName,
                                                           sReverseReplName )
                              != IDE_SUCCESS );

                    ideLog::log(IDE_SD_0,"[SHARD_FAILBACK] FLUSH REVERSE REP, %s \n", sReverseReplName );
                    
                    IDE_TEST( flushReplicationForInternal( aStatement,
                                                           aFailbackFromNodeName,
                                                           sReverseReplName )
                              != IDE_SUCCESS );

                    ideLog::log(IDE_SD_0,"[SHARD_FAILBACK] FLUSH REVERSE REP COMPLETE %s", sReverseReplName);
                }

            }
        }
    }


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* After Lock은 Flush 와 정리를 한다. */
IDE_RC qdsd::failbackOrganizeReverseRPAfterLock( qcStatement        * aStatement,
                                                 SChar              * aFailbackFromNodeName,
                                                 sdiReplicaSetInfo  * aFailoverHistoryInfo )
{
    SChar               sSqlStr[QD_MAX_SQL_LENGTH + 1];

    SChar               sReverseReplName[SDI_REPLICATION_NAME_MAX_SIZE + 1];

    sdiReplicaSet     * sReplicaSet = NULL;

    UInt                i;

    UInt                sExecCount;
    ULong               sCount = 0;

    idBool              sFetchResult = ID_FALSE;

    ideLog::log(IDE_SD_0,"[SHARD_FAILBACK] Start REVERSE REP After Lock" );

    for ( i = 0; i < aFailoverHistoryInfo->mCount; i++ )
    {
        sReplicaSet = &aFailoverHistoryInfo->mReplicaSets[i];

        if ( idlOS::strncmp( sReplicaSet->mFirstBackupNodeName,
                             aFailbackFromNodeName,
                             SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
        {
            idlOS::snprintf( sReverseReplName, 
                             SDI_REPLICATION_NAME_MAX_SIZE + 1,
                             "R_%s", 
                             sReplicaSet->mFirstReplName );
        }
        else if ( idlOS::strncmp( sReplicaSet->mSecondBackupNodeName,
                                  aFailbackFromNodeName,
                                  SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
        {
            idlOS::snprintf( sReverseReplName, 
                             SDI_REPLICATION_NAME_MAX_SIZE + 1,
                             "R_%s", 
                             sReplicaSet->mSecondReplName );
        }
        else
        {
            idlOS::snprintf( sReverseReplName,
                             SDI_REPLICATION_NAME_MAX_SIZE + 1,
                             SDM_NA_STR );
        }

        if ( idlOS::strncmp( sReverseReplName,
                             SDM_NA_STR,
                             SDI_REPLICATION_NAME_MAX_SIZE + 1 ) != 0 )
        {

            /* DROP FORCE의 추가로 ReverseRP가 존재한다고 확신할수 없다.
             * 확인 해 보고 존재하면 ReverseRP를 처리한다. */
            idlOS::snprintf( sSqlStr,
                             QD_MAX_SQL_LENGTH + 1,
                             "select Count(*) from system_.sys_replications_ where replication_name='%s' ",
                             sReverseReplName );

            IDE_TEST( sdi::shardExecDirect( aStatement,
                                            aFailbackFromNodeName,
                                            (SChar*)sSqlStr,
                                            (UInt) idlOS::strlen(sSqlStr),
                                            SDI_INTERNAL_OP_FAILBACK,
                                            &sExecCount,
                                            &sCount,
                                            NULL,
                                            ID_SIZEOF(sCount),
                                            &sFetchResult )
                      != IDE_SUCCESS );

            if ( sCount != 0 )
            {
                /* Lock 잡은 후에 그 사이 있었던 내용을 Flush 받아온다. */
                IDE_TEST( flushReplicationForInternal( aStatement,
                                                       aFailbackFromNodeName,
                                                       sReverseReplName )
                          != IDE_SUCCESS );

                ideLog::log(IDE_SD_0,"[SHARD_FAILBACK] AFTER LOCK FLUSH REVERSE REP COMPLETE %s", sReverseReplName);

                /* Flush 끝났으면 Stop 해둔다. */
                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                 "ALTER REPLICATION "QCM_SQL_STRING_SKIP_FMT" STOP ", 
                                 sReverseReplName );

                IDE_TEST( executeRemoteSQL( aStatement, 
                                            aFailbackFromNodeName,
                                            SDI_INTERNAL_OP_FAILBACK,
                                            QCI_STMT_MASK_DCL,
                                            sSqlStr,
                                            QD_MAX_SQL_LENGTH,
                                            NULL )
                            != IDE_SUCCESS );

                ideLog::log(IDE_SD_0,"[SHARD_FAILBACK] STOP REVERSE REP %s", sReverseReplName);

                /* Reverse RP는 Commit 시에 Drop은 TxDDL로 처리한다. */
                /* Sender/Receiver 둘다 제거 해야 한다. */
                idlOS::snprintf(sSqlStr,
                                QD_MAX_SQL_LENGTH,
                                "drop replication "QCM_SQL_STRING_SKIP_FMT"",
                                sReverseReplName );

                IDE_TEST( executeRemoteSQL( aStatement, 
                                            aFailbackFromNodeName,
                                            SDI_INTERNAL_OP_FAILBACK,
                                            QCI_STMT_MASK_DDL,
                                            sSqlStr,
                                            QD_MAX_SQL_LENGTH,
                                            NULL )
                            != IDE_SUCCESS );

                /* Local Receiver는 NewTrans로 지워야 한다. TxDDL로 하면 Local Start 에서 Lock 대기 발생
                 * Local Receiver는 Failback에서 생성한거라 이후 실패해도 다시 생성해서하니까 괜찮다. */
                IDE_TEST( dropReplicationForInternal( aStatement,
                                                      sdiZookeeper::mMyNodeName,
                                                      sReverseReplName )
                          != IDE_SUCCESS );
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



/* Before Lock은 FailoverCreateRP를 제외한
 * Failback Node의 Sender와 Receiver RP를 Start 한다. */
IDE_RC qdsd::failbackRecoverRPbeforeLock( qcStatement        * aStatement,
                                          SChar              * aFailbackFromNodeName,
                                          sdiReplicaSetInfo  * aFailoverHistoryInfoWithOtherNode )
{
    UInt             i = 0;
    sdiReplicaSet  * sReplicaSet = NULL;
    SChar            sSqlStr[QD_MAX_SQL_LENGTH + 1];
    UInt              sShardDDLLockTryCount = QCG_GET_SESSION_SHARD_DDL_LOCK_TRY_COUNT(aStatement);
    UInt              sRemoteLockTimeout = QCG_GET_SESSION_SHARD_DDL_LOCK_TIMEOUT(aStatement);
    idBool            sIsAlive = ID_FALSE;

    /* Failback Start RP */
    for ( i = 0; i < aFailoverHistoryInfoWithOtherNode->mCount; i++ )
    {
        sReplicaSet = &aFailoverHistoryInfoWithOtherNode->mReplicaSets[i];

        /* SendRP Start */
        if ( idlOS::strncmp( sReplicaSet->mPrimaryNodeName,
                             sdiZookeeper::mMyNodeName,
                             SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
        {
            if ( idlOS::strncmp( sReplicaSet->mFirstBackupNodeName,
                                 aFailbackFromNodeName,
                                 SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
            {
                /* Failback Node로의 Tx는 Failback 완료 때까지 없을 거니까 lock 이전에 Start 한다. */
                /* Failback Node에서 Send 하는 RP는 Failback 완료시점 부터 Send 하면 된다. */
                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                 "ALTER REPLICATION "QCM_SQL_STRING_SKIP_FMT" RESET ",
                                 sReplicaSet->mFirstReplName );

                /* R2HA RESET이 DDL이라 DDL로 수행하려 하였으나 문제 발생해서 일단 분석안하고
                 * 정상 동작하는 DMLOrDCL로 수행하도록 해둠 추후 분석 필요.
                IDE_TEST( sdi::shardExecTempDDLWithNewTrans( aStatement,
                                                             sdiZookeeper::mMyNodeName,
                                                             sSqlStr,
                                                             idlOS::strlen( sSqlStr ) )
                          != IDE_SUCCESS );*/
                IDE_TEST( sdi::shardExecTempDMLOrDCLWithNewTrans( aStatement,
                                                                  sSqlStr )
                          != IDE_SUCCESS );


                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                 "ALTER REPLICATION "QCM_SQL_STRING_SKIP_FMT" START ",
                                 sReplicaSet->mFirstReplName );

                IDE_TEST( executeLocalSQL( aStatement, 
                                            QCI_STMT_MASK_DCL,
                                            sSqlStr,
                                            QD_MAX_SQL_LENGTH,
                                            NULL )
                            != IDE_SUCCESS );

                idlOS::snprintf(sSqlStr,
                                QD_MAX_SQL_LENGTH,
                                "alter replication "QCM_SQL_STRING_SKIP_FMT" stop",
                                sReplicaSet->mFirstReplName );

                (void)sdiZookeeper::addPendingJob( sSqlStr,
                                                   sdiZookeeper::mMyNodeName,
                                                   ZK_PENDING_JOB_AFTER_ROLLBACK,
                                                   QCI_STMT_MASK_MAX );

                ideLog::log( IDE_SD_0,
                             "[SHARD_FAILBACK] Failback Send Start RP : %s", 
                             sReplicaSet->mFirstReplName );

                if ( idlOS::strncmp( sReplicaSet->mSecondBackupNodeName,
                                     SDM_NA_STR,
                                     SDI_NODE_NAME_MAX_SIZE + 1 ) != 0 )
                {
                    /* Failover Create RP 이다. beforeLock에서는 Start 하지 않는다. */
                    /* Before Lock에서 Flush 한번 해준다. */
                    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                     "ALTER REPLICATION "QCM_SQL_STRING_SKIP_FMT" FLUSH WAIT %"ID_UINT32_FMT,
                                     sReplicaSet->mSecondReplName,
                                     (sShardDDLLockTryCount * sRemoteLockTimeout) );

                    if( executeRemoteSQL( aStatement, 
                                          aFailbackFromNodeName,
                                          SDI_INTERNAL_OP_FAILBACK,
                                          QCI_STMT_MASK_DCL,
                                          sSqlStr,
                                          QD_MAX_SQL_LENGTH,
                                          NULL )
                        != IDE_SUCCESS )
                    {
                        /* Failback 에서 Stop 후 kill 되는 상황으로 start 안되어 있다면
                         * Start/Flush를 수행해 준다. */
                        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                         "ALTER REPLICATION "QCM_SQL_STRING_SKIP_FMT" START ",
                                         sReplicaSet->mSecondReplName );

                        IDE_TEST( executeRemoteSQL( aStatement, 
                                                    aFailbackFromNodeName,
                                                    SDI_INTERNAL_OP_FAILBACK,
                                                    QCI_STMT_MASK_DCL,
                                                    sSqlStr,
                                                    QD_MAX_SQL_LENGTH,
                                                    NULL )
                                  != IDE_SUCCESS );

                        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                         "ALTER REPLICATION "QCM_SQL_STRING_SKIP_FMT" FLUSH WAIT %"ID_UINT32_FMT,
                                         sReplicaSet->mSecondReplName,
                                         (sShardDDLLockTryCount * sRemoteLockTimeout));

                        IDE_TEST( executeRemoteSQL( aStatement, 
                                                    aFailbackFromNodeName,
                                                    SDI_INTERNAL_OP_FAILBACK,
                                                    QCI_STMT_MASK_DCL,
                                                    sSqlStr,
                                                    QD_MAX_SQL_LENGTH,
                                                    NULL )
                                  != IDE_SUCCESS );
                    }

                    ideLog::log(IDE_SD_0,"[SHARD_FAILBACK] FailoverCreatedRP Flush Before Lock" );
                }
            }
            if ( idlOS::strncmp( sReplicaSet->mSecondBackupNodeName,
                                 aFailbackFromNodeName,
                                 SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
            {
                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                 "ALTER REPLICATION "QCM_SQL_STRING_SKIP_FMT" RESET ",
                                 sReplicaSet->mSecondReplName );
                /* R2HA RESET이 DDL이라 DDL로 수행하려 하였으나 문제 발생해서 일단 분석안하고
                 * 정상 동작하는 DMLOrDCL로 수행하도록 해둠 추후 분석 필요.
                IDE_TEST( sdi::shardExecTempDDLWithNewTrans( aStatement,
                                                             sdiZookeeper::mMyNodeName,
                                                             sSqlStr,
                                                             idlOS::strlen( sSqlStr ) )
                          != IDE_SUCCESS );*/

                IDE_TEST( sdi::shardExecTempDMLOrDCLWithNewTrans( aStatement,
                                                                  sSqlStr )
                          != IDE_SUCCESS );


                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                 "ALTER REPLICATION "QCM_SQL_STRING_SKIP_FMT" START ",
                                 sReplicaSet->mSecondReplName );

                IDE_TEST( executeLocalSQL( aStatement, 
                                            QCI_STMT_MASK_DCL,
                                            sSqlStr,
                                            QD_MAX_SQL_LENGTH,
                                            NULL )
                            != IDE_SUCCESS );

                idlOS::snprintf(sSqlStr,
                                QD_MAX_SQL_LENGTH,
                                "alter replication "QCM_SQL_STRING_SKIP_FMT" stop",
                                sReplicaSet->mSecondReplName );

                (void)sdiZookeeper::addPendingJob( sSqlStr,
                                                   sdiZookeeper::mMyNodeName,
                                                   ZK_PENDING_JOB_AFTER_ROLLBACK,
                                                   QCI_STMT_MASK_MAX );

                ideLog::log( IDE_SD_0,
                             "[SHARD_FAILBACK] Failback Send Start RP : %s", 
                             sReplicaSet->mSecondReplName );
            }
        }

        /* Receive RP Start */
        /* DROP FORCE로 인해 FailoverHistory에 기록된 PrimaryNode가 Alive일 거라는 보장이 없다.
         * Alive 상태라면 Failback 시에 Start 요청하면 되고
         * Alive 상태가 아니라면 해당 Node의 Failback 수행시 Start 될 것이다. */

        IDE_TEST( sdiZookeeper::checkNodeAlive( sReplicaSet->mPrimaryNodeName,
                                                &sIsAlive ) != IDE_SUCCESS );

        if ( sIsAlive == ID_TRUE )
        {

            if ( idlOS::strncmp( sReplicaSet->mFirstBackupNodeName,
                                 sdiZookeeper::mMyNodeName,
                                 SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
            {
                /* Failback_Failedover에서 Receive RP는 Stop 되어 있어야 한다.
                 * Failback 에서 Start 시킨후 Kill 등으로 Failback 재시도시 Start되어 있을수 있다 */
                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                 "ALTER REPLICATION "QCM_SQL_STRING_SKIP_FMT" STOP ",
                                 sReplicaSet->mFirstReplName );

                (void)executeRemoteSQL( aStatement, 
                                        sReplicaSet->mPrimaryNodeName,
                                        SDI_INTERNAL_OP_FAILBACK,
                                        QCI_STMT_MASK_DCL,
                                        sSqlStr,
                                        QD_MAX_SQL_LENGTH,
                                        NULL );

                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                 "ALTER REPLICATION "QCM_SQL_STRING_SKIP_FMT" START ",
                                 sReplicaSet->mFirstReplName );

                IDE_TEST( executeRemoteSQL( aStatement, 
                                            sReplicaSet->mPrimaryNodeName,
                                            SDI_INTERNAL_OP_FAILBACK,
                                            QCI_STMT_MASK_DCL,
                                            sSqlStr,
                                            QD_MAX_SQL_LENGTH,
                                            NULL )
                            != IDE_SUCCESS );

                idlOS::snprintf(sSqlStr,
                                QD_MAX_SQL_LENGTH,
                                "alter replication "QCM_SQL_STRING_SKIP_FMT" stop",
                                sReplicaSet->mFirstReplName );

                (void)sdiZookeeper::addPendingJob( sSqlStr,
                                                   sReplicaSet->mPrimaryNodeName,
                                                   ZK_PENDING_JOB_AFTER_ROLLBACK,
                                                   QCI_STMT_MASK_MAX );

                ideLog::log( IDE_SD_0,
                             "[SHARD_FAILBACK] Failback Receive Start RP : %s", 
                             sReplicaSet->mFirstReplName );

                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                 "ALTER REPLICATION "QCM_SQL_STRING_SKIP_FMT" FLUSH WAIT %"ID_UINT32_FMT,
                                 sReplicaSet->mFirstReplName,
                                 (sShardDDLLockTryCount * sRemoteLockTimeout) );

                IDE_TEST( executeRemoteSQL( aStatement, 
                                            sReplicaSet->mPrimaryNodeName,
                                            SDI_INTERNAL_OP_FAILBACK,
                                            QCI_STMT_MASK_DCL,
                                            sSqlStr,
                                            QD_MAX_SQL_LENGTH,
                                            NULL )
                            != IDE_SUCCESS );

                ideLog::log( IDE_SD_0,
                             "[SHARD_FAILBACK] Failback Receive FLUSH RP : %s", 
                             sReplicaSet->mFirstReplName );


            }
            if ( idlOS::strncmp( sReplicaSet->mSecondBackupNodeName,
                                 sdiZookeeper::mMyNodeName,
                                 SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
            {
                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                 "ALTER REPLICATION "QCM_SQL_STRING_SKIP_FMT" STOP ",
                                 sReplicaSet->mSecondReplName );

                (void)executeRemoteSQL( aStatement, 
                                        sReplicaSet->mPrimaryNodeName,
                                        SDI_INTERNAL_OP_FAILBACK,
                                        QCI_STMT_MASK_DCL,
                                        sSqlStr,
                                        QD_MAX_SQL_LENGTH,
                                        NULL );

                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                 "ALTER REPLICATION "QCM_SQL_STRING_SKIP_FMT" START ",
                                 sReplicaSet->mSecondReplName );

                IDE_TEST( executeRemoteSQL( aStatement, 
                                            sReplicaSet->mPrimaryNodeName,
                                            SDI_INTERNAL_OP_FAILBACK,
                                            QCI_STMT_MASK_DCL,
                                            sSqlStr,
                                            QD_MAX_SQL_LENGTH,
                                            NULL )
                            != IDE_SUCCESS );

                idlOS::snprintf(sSqlStr,
                                QD_MAX_SQL_LENGTH,
                                "alter replication "QCM_SQL_STRING_SKIP_FMT" stop",
                                sReplicaSet->mSecondReplName );

                (void)sdiZookeeper::addPendingJob( sSqlStr,
                                                   sReplicaSet->mPrimaryNodeName,
                                                   ZK_PENDING_JOB_AFTER_ROLLBACK,
                                                   QCI_STMT_MASK_MAX );

                ideLog::log( IDE_SD_0,
                             "[SHARD_FAILBACK] Failback Receive Start RP : %s", 
                             sReplicaSet->mSecondReplName );

                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                 "ALTER REPLICATION "QCM_SQL_STRING_SKIP_FMT" FLUSH WAIT %"ID_UINT32_FMT,
                                 sReplicaSet->mSecondReplName,
                                 (sShardDDLLockTryCount * sRemoteLockTimeout) );

                IDE_TEST( executeRemoteSQL( aStatement, 
                                            sReplicaSet->mPrimaryNodeName,
                                            SDI_INTERNAL_OP_FAILBACK,
                                            QCI_STMT_MASK_DCL,
                                            sSqlStr,
                                            QD_MAX_SQL_LENGTH,
                                            NULL )
                            != IDE_SUCCESS );

                ideLog::log( IDE_SD_0,
                             "[SHARD_FAILBACK] Failback Receive FLUSH RP : %s", 
                             sReplicaSet->mSecondReplName );


            }
        }

    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/* Lock을 잡았으니까 이제 FailoverCreatedRP를 Stop 하고 Failback Node에서 Start 하도록한다.
 * Failback Node가 Receiver인 RP도 같이 Start 한다. */
IDE_RC qdsd::failbackRecoverRPAfterLock( qcStatement        * aStatement,
                                         SChar              * aFailbackFromNodeName,
                                         sdiReplicaSet      * aMainReplicaSet,
                                         sdiReplicaSetInfo  * aFailoverHistoryInfo,
                                         sdiReplicaSetInfo  * aFailoverHistoryInfoWithOtherNode )
{
    UInt             i = 0;
    sdiReplicaSet  * sReplicaSet = NULL;
    SChar            sSqlStr[QD_MAX_SQL_LENGTH + 1];
    SChar            sFailoverCreatedReplName[SDI_REPLICATION_NAME_MAX_SIZE + 1] = {0,};
    UInt             sShardDDLLockTryCount = QCG_GET_SESSION_SHARD_DDL_LOCK_TRY_COUNT(aStatement);
    UInt             sRemoteLockTimeout = QCG_GET_SESSION_SHARD_DDL_LOCK_TIMEOUT(aStatement);

    UInt             sExecCount;
    ULong            sCount = 0;
    idBool           sFetchResult = ID_FALSE;

    qdShardParseTree  * sParseTree = NULL;

    sdiLocalMetaInfo  * sNodeMetaInfo = NULL;

    idBool           sIsAlive = ID_FALSE; 



    /* Failover Created RP Stop */
    /* N1의 Failover에서 N1->N3를 N2->N3로 추가한 RP 제거 Stop/Drop 해야함. */
    if ( idlOS::strncmp( aMainReplicaSet->mPrimaryNodeName,
                         aFailbackFromNodeName,
                         SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
    {
        if ( idlOS::strncmp( aMainReplicaSet->mStopFirstBackupNodeName,
                             aFailbackFromNodeName,
                             SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
        {
            if ( idlOS::strncmp( aMainReplicaSet->mSecondBackupNodeName,
                                 SDM_NA_STR,
                                 SDI_NODE_NAME_MAX_SIZE + 1 ) != 0 )
            {
                idlOS::strncpy( sFailoverCreatedReplName,
                                aMainReplicaSet->mSecondReplName,
                                SDI_REPLICATION_NAME_MAX_SIZE + 1 );

                /* Lock 획득 후 Flush 한다. */
                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                 "ALTER REPLICATION "QCM_SQL_STRING_SKIP_FMT" FLUSH WAIT %"ID_UINT32_FMT,
                                 sFailoverCreatedReplName,
                                 (sShardDDLLockTryCount * sRemoteLockTimeout) );

                IDE_TEST( executeRemoteSQL( aStatement, 
                                            aFailbackFromNodeName,
                                            SDI_INTERNAL_OP_FAILBACK,
                                            QCI_STMT_MASK_DCL,
                                            sSqlStr,
                                            QD_MAX_SQL_LENGTH,
                                            NULL )
                            != IDE_SUCCESS );

                /* FailoverCreatedRP는 lock 획득 후 Stop 해야 한다. */
                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                 "ALTER REPLICATION "QCM_SQL_STRING_SKIP_FMT" STOP ",
                                 sFailoverCreatedReplName,
                                 QCI_STMT_MASK_MAX );

                IDE_TEST( executeRemoteSQL( aStatement, 
                                            aFailbackFromNodeName,
                                            SDI_INTERNAL_OP_FAILBACK,
                                            QCI_STMT_MASK_DCL,
                                            sSqlStr,
                                            QD_MAX_SQL_LENGTH,
                                            NULL )
                            != IDE_SUCCESS );

                /* Rollback시 Local Start가 되어 있을수 있기 때문에 local Stop 먼저 하고
                 * Remote start retry를 걸어둔다. */
                idlOS::snprintf(sSqlStr,
                                QD_MAX_SQL_LENGTH,
                                "alter replication "QCM_SQL_STRING_SKIP_FMT" stop ",
                                sFailoverCreatedReplName );

                (void)sdiZookeeper::addPendingJob( sSqlStr,
                                                   sdiZookeeper::mMyNodeName,
                                                   ZK_PENDING_JOB_AFTER_ROLLBACK,
                                                   QCI_STMT_MASK_MAX );

                idlOS::snprintf(sSqlStr,
                                QD_MAX_SQL_LENGTH,
                                "alter replication "QCM_SQL_STRING_SKIP_FMT" start retry",
                                sFailoverCreatedReplName );

                (void)sdiZookeeper::addPendingJob( sSqlStr,
                                                   aFailbackFromNodeName,
                                                   ZK_PENDING_JOB_AFTER_ROLLBACK,
                                                   QCI_STMT_MASK_DDL );

                ideLog::log( IDE_SD_0,
                             "[SHARD_FAILBACK] STOP FailoverCreatedRP : %s", 
                             sFailoverCreatedReplName );

                /* RP Drop은 TxDDL로 처리한다. Sender 만 지워 줘야 한다.
                 * AfterRollback의 경우 Start이다. */
                idlOS::snprintf( sSqlStr,
                                 QD_MAX_SQL_LENGTH,
                                 "drop replication "QCM_SQL_STRING_SKIP_FMT"",
                                 sFailoverCreatedReplName );

                IDE_TEST( executeRemoteSQL( aStatement, 
                                            aFailbackFromNodeName,
                                            SDI_INTERNAL_OP_FAILBACK,
                                            QCI_STMT_MASK_DDL,
                                            sSqlStr,
                                            QD_MAX_SQL_LENGTH,
                                            NULL )
                            != IDE_SUCCESS );

            }
        }
    }

    /* Failback Start RP */
    for ( i = 0; i < aFailoverHistoryInfoWithOtherNode->mCount; i++ )
    {
        sReplicaSet = &aFailoverHistoryInfoWithOtherNode->mReplicaSets[i];

        /* SendRP Start */
        if ( idlOS::strncmp( sReplicaSet->mPrimaryNodeName,
                             sdiZookeeper::mMyNodeName,
                             SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
        {
            if ( idlOS::strncmp( sReplicaSet->mFirstBackupNodeName,
                                 aFailbackFromNodeName,
                                 SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
            {
                if ( idlOS::strncmp( sReplicaSet->mSecondBackupNodeName,
                                     SDM_NA_STR,
                                     SDI_NODE_NAME_MAX_SIZE + 1 ) != 0 )
                {
                    /* FailoverCreatedRP에서 이미 다 보내놨을건데 또 보내면 안된다.
                     * Failback Node에서 Send 하는 것은 앞으로 발생할 Log에 대해서만 보내면 된다. */
                    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                     "ALTER REPLICATION "QCM_SQL_STRING_SKIP_FMT" RESET ",
                                     sReplicaSet->mSecondReplName );

                    IDE_TEST( sdi::shardExecTempDMLOrDCLWithNewTrans( aStatement,
                                                                      sSqlStr )
                              != IDE_SUCCESS );

                    /* Failover Created RP의 Stop 이후에 Start 되어야 한다. */
                    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                     "ALTER REPLICATION "QCM_SQL_STRING_SKIP_FMT" START ",
                                     sReplicaSet->mSecondReplName );

                    IDE_TEST( executeLocalSQL( aStatement, 
                                               QCI_STMT_MASK_DCL,
                                               sSqlStr,
                                               QD_MAX_SQL_LENGTH,
                                               NULL )
                              != IDE_SUCCESS );

                    idlOS::snprintf(sSqlStr,
                                    QD_MAX_SQL_LENGTH,
                                    "alter replication "QCM_SQL_STRING_SKIP_FMT" stop",
                                    sReplicaSet->mSecondReplName );

                    (void)sdiZookeeper::addPendingJob( sSqlStr,
                                                       sdiZookeeper::mMyNodeName,
                                                       ZK_PENDING_JOB_AFTER_ROLLBACK,
                                                       QCI_STMT_MASK_DDL );

                    ideLog::log( IDE_SD_0,
                                 "[SHARD_FAILBACK] Failback Send Start RP : %s", 
                                 sReplicaSet->mSecondReplName );
                }
            }
        }
    }

    /* DROP Force 에 의해 제거된 Node가 존재한다면 해당 Node로의 Send RP를 제거 해야 한다. */
    /* K-Safety 0 등에 의해 존재할수 있는 상황이기 때문에 Select 먼저 해보고 있으면 제거 한다. */
    for ( i = 0; i < aFailoverHistoryInfo->mCount; i++ )
    {
        sReplicaSet = &aFailoverHistoryInfo->mReplicaSets[i];

        if ( ( idlOS::strncmp( sReplicaSet->mFirstBackupNodeName,
                               SDM_NA_STR,
                               SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 ) &&
             ( idlOS::strncmp( sReplicaSet->mStopFirstBackupNodeName,
                               SDM_NA_STR,                                       
                               SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 ) )
        {
            idlOS::snprintf( sSqlStr, 
                             QD_MAX_SQL_LENGTH + 1,
                             "select Count(*) from system_.sys_replications_ where replication_name='%s' ",
                             sReplicaSet->mFirstReplName );

            IDE_TEST( sdi::shardExecDirect( aStatement,
                                            sReplicaSet->mPrimaryNodeName,
                                            (SChar*)sSqlStr,
                                            (UInt) idlOS::strlen(sSqlStr),
                                            SDI_INTERNAL_OP_FAILBACK,
                                            &sExecCount,
                                            &sCount,
                                            NULL,
                                            ID_SIZEOF(sCount),
                                            &sFetchResult )
                      != IDE_SUCCESS );

            if ( sCount != 0 )
            {
                idlOS::snprintf(sSqlStr,
                                QD_MAX_SQL_LENGTH,
                                "drop replication "QCM_SQL_STRING_SKIP_FMT,
                                sReplicaSet->mFirstReplName );

                IDE_TEST( executeRemoteSQL( aStatement, 
                                            sReplicaSet->mPrimaryNodeName,
                                            SDI_INTERNAL_OP_FAILBACK,
                                            QCI_STMT_MASK_DDL,
                                            sSqlStr,
                                            QD_MAX_SQL_LENGTH,
                                            NULL )
                            != IDE_SUCCESS );
            }
        }

        if ( ( idlOS::strncmp( sReplicaSet->mSecondBackupNodeName,
                               SDM_NA_STR,
                               SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 ) &&
             ( idlOS::strncmp( sReplicaSet->mStopSecondBackupNodeName,
                               SDM_NA_STR,
                               SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 ) )
        {
            idlOS::snprintf( sSqlStr, 
                             QD_MAX_SQL_LENGTH + 1,
                             "select Count(*) from system_.sys_replications_ where replication_name='%s' ",
                             sReplicaSet->mSecondReplName );

            IDE_TEST( sdi::shardExecDirect( aStatement,
                                            sReplicaSet->mPrimaryNodeName,
                                            (SChar*)sSqlStr,
                                            (UInt) idlOS::strlen(sSqlStr),
                                            SDI_INTERNAL_OP_FAILBACK,
                                            &sExecCount,
                                            &sCount,
                                            NULL,
                                            ID_SIZEOF(sCount),
                                            &sFetchResult )
                      != IDE_SUCCESS );

            if ( sCount != 0 )
            {
                idlOS::snprintf(sSqlStr,
                                QD_MAX_SQL_LENGTH,
                                "drop replication "QCM_SQL_STRING_SKIP_FMT,
                                sReplicaSet->mSecondReplName );

                IDE_TEST( executeRemoteSQL( aStatement, 
                                            sReplicaSet->mPrimaryNodeName,
                                            SDI_INTERNAL_OP_FAILBACK,
                                            QCI_STMT_MASK_DDL,
                                            sSqlStr,
                                            QD_MAX_SQL_LENGTH,
                                            NULL )
                            != IDE_SUCCESS );
            }
        }
    }

    /* DropForce 된 Node로 부터의 Receive이면 Drop 해야 함.
     * ReplicaSetID == NodeID 이기 때문에 해당 ReplicaSetId와 같은 NodeID가 존재하는지 확인. */
    /* failbackFailedover 함수에서 getAllNodeInfo를 수행하여 ParseTree에 NodeInfo가 저장되어 있다. */
    for ( i = 0; i < aFailoverHistoryInfoWithOtherNode->mCount; i++ )
    {
        sReplicaSet = &aFailoverHistoryInfoWithOtherNode->mReplicaSets[i];

        IDE_TEST( sdiZookeeper::checkNodeAlive( sReplicaSet->mPrimaryNodeName,
                                                &sIsAlive ) != IDE_SUCCESS );

        if ( sIsAlive == ID_FALSE )
        {
            sParseTree = (qdShardParseTree *)aStatement->myPlan->parseTree;

            sNodeMetaInfo = sdiZookeeper::getNodeInfoByID( sParseTree->mNodeInfoList,
                                                           sReplicaSet->mReplicaSetId );

            if ( sNodeMetaInfo == NULL )
            {
                if ( idlOS::strncmp( sReplicaSet->mFirstBackupNodeName,
                                     sdiZookeeper::mMyNodeName,
                                     SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
                {
                    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                     "DROP REPLICATION "QCM_SQL_STRING_SKIP_FMT,
                                     sReplicaSet->mFirstReplName );

                    (void)executeRemoteSQL( aStatement, 
                                            sdiZookeeper::mMyNodeName,
                                            SDI_INTERNAL_OP_FAILBACK,
                                            QCI_STMT_MASK_DDL,
                                            sSqlStr,
                                            QD_MAX_SQL_LENGTH,
                                            NULL );

                    ideLog::log( IDE_SD_0,
                                 "[SHARD_FAILBACK] Failback Receive DROP RP : %s", 
                                 sReplicaSet->mFirstReplName );

                }
                if ( idlOS::strncmp( sReplicaSet->mSecondBackupNodeName,
                                     sdiZookeeper::mMyNodeName,
                                     SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
                {
                    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                     "DROP REPLICATION "QCM_SQL_STRING_SKIP_FMT,
                                     sReplicaSet->mSecondReplName );

                    (void)executeRemoteSQL( aStatement, 
                                            sdiZookeeper::mMyNodeName,
                                            SDI_INTERNAL_OP_FAILBACK,
                                            QCI_STMT_MASK_DDL,
                                            sSqlStr,
                                            QD_MAX_SQL_LENGTH,
                                            NULL );

                    ideLog::log( IDE_SD_0,
                                 "[SHARD_FAILBACK] Failback Receive DROP RP : %s", 
                                 sReplicaSet->mSecondReplName );
                }
            }

        }

    }



    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdsd::failbackJoin( qcStatement        * aStatement,
                           SChar              * aFailbackFromNodeName,
                           sdiFailbackState     aFailbackState )
{
    sdiReplicaSetInfo   sReplicaSetInfo;

    ULong               sOldSMN = 0;

    sdiClientInfo     * sClientInfo  = NULL;
    SChar               sSqlStr[QD_MAX_SQL_LENGTH + 1];

    iduList           * sList;
    idBool              sIsAllocList = ID_FALSE;

    switch ( aFailbackState )
    {
        case SDI_FAILBACK_JOIN:
            break;
        case SDI_FAILBACK_JOIN_SMN_MODIFIED:
            /* Shard Meta Recovery From aFailbackFromNodeName
             * SMN Recovery */
            /***********************************************************************
             * 샤드 메타 복제
             ***********************************************************************/
            IDE_TEST( resetShardMeta( aStatement ) != IDE_SUCCESS);
            IDE_TEST( getAllNodeInfoList( aStatement ) != IDE_SUCCESS );
            IDE_TEST( getShardMetaFromOtherNode( aStatement,
                                                 aFailbackFromNodeName ) != IDE_SUCCESS );

            ideLog::log(IDE_SD_0,"[SHARD_FAILBACK] SMN Update Complete");
            break;
        case SDI_FAILBACK_FAILEDOVER:
        case SDI_FAILBACK_NONE:
        default:
            /* 이런건 오면 안됨 */
            IDE_DASSERT( 0 );
            break;
    }

    sOldSMN = sdi::getSMNForDataNode();

    /* get Alive Node List + LocalNode 필요 */
    IDE_TEST( sdiZookeeper::getAliveNodeNameListIncludeNodeName( &sList,
                                                                 sdiZookeeper::mMyNodeName ) 
              != IDE_SUCCESS );
    sIsAllocList = ID_TRUE;

    /* NodeList 대상으로 ShardLinker 생성해야 한다. */
    IDE_TEST( sdi::checkShardLinkerWithNodeList ( aStatement,
                                                  sOldSMN,
                                                  NULL,
                                                  sList ) != IDE_SUCCESS );

    sClientInfo = aStatement->session->mQPSpecific.mClientInfo;

    IDE_TEST_RAISE( sClientInfo == NULL, ERR_NODE_NOT_EXIST );

    /* get ReplicaSet Info */
    IDE_TEST( sdm::getAllReplicaSetInfoSortedByPName( QC_SMI_STMT( aStatement ),
                                                      &sReplicaSetInfo,
                                                      sOldSMN )
              != IDE_SUCCESS );

    /* Failback은 항상 SMN Update가 발생해야 한다. */
    idlOS::snprintf( sSqlStr, 
                     QD_MAX_SQL_LENGTH,
                     "EXEC DBMS_SHARD.TOUCH_META(); " );

    IDE_TEST( executeSQLNodeList( aStatement,
                                  SDI_INTERNAL_OP_FAILBACK,
                                  QCI_STMT_MASK_SP,
                                  sSqlStr,
                                  NULL,
                                  sList ) != IDE_SUCCESS );

    ideLog::log(IDE_SD_0,"[SHARD_FAILBACK] Touch Meta Complete");

    /* RP Recover */
    IDE_TEST( joinRecoverRP( aStatement,
                             &sReplicaSetInfo )
              != IDE_SUCCESS );

    sIsAllocList = ID_FALSE;
    sdiZookeeper::freeList( sList, SDI_ZKS_LIST_NODENAME );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NODE_NOT_EXIST )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_NODE_NOT_EXIST ) );
    }
    IDE_EXCEPTION_END;

    if ( sIsAllocList == ID_TRUE )
    {
        sdiZookeeper::freeList( sList, SDI_ZKS_LIST_NODENAME );
    }

    return IDE_FAILURE;
}


IDE_RC qdsd::failbackFailedover( qcStatement        * aStatement,
                                 SChar              * aFailbackFromNodeName,
                                 ULong                aFailbackSMN )
{

    SChar             * sUserName;
    SChar               sMyNextNodeName[SDI_NODE_NAME_MAX_SIZE + 1];

    sdiReplicaSetInfo   sFailoverHistoryInfo;
    sdiReplicaSetInfo   sFailoverHistoryInfoWithOtherNode;

    sdiReplicaSetInfo   sReplicaSetInfo;

    sdiReplicaSet     * sMainReplicaSet = NULL;
    sdiReplicaSet     * sReplicaSet = NULL;

    ULong               sOldSMN = 0;
    ULong               sNewSMN = 0;

    sdiClientInfo     * sClientInfo  = NULL;
    SChar               sSqlStr[QD_MAX_SQL_LENGTH + 1];

    iduList           * sList;
    idBool              sIsAllocList = ID_FALSE;

    sdiTableInfoList  * sTableInfoList = NULL;

    sdiNodeInfo         sNodeInfo;
    sdiNode           * sLocalNodeInfo = NULL;

    UInt                i;
    UInt                sReplicaSetId = SDI_REPLICASET_NULL_ID;

    sdiLocalMetaInfo    sLocalMetaInfo;

    ZKState             sState = ZK_NONE;

    idBool              sNeedSync = ID_FALSE;
    qdShardParseTree  * sParseTree = NULL;

    idBool              sSyncTables = ID_FALSE;

    sParseTree = (qdShardParseTree *)aStatement->myPlan->parseTree;

    sUserName = QCG_GET_SESSION_USER_NAME( aStatement );

    /* Shard Meta Recovery From aFailbackFromNodeName
     * SMN Recovery */
    /***********************************************************************
     * 샤드 메타 복제
     ***********************************************************************/
    IDE_TEST( resetShardMeta( aStatement ) != IDE_SUCCESS);
    IDE_TEST( getAllNodeInfoList( aStatement ) != IDE_SUCCESS );
    IDE_TEST( getShardMetaFromOtherNode( aStatement,
                                         aFailbackFromNodeName ) != IDE_SUCCESS );

    ideLog::log(IDE_SD_0,"[SHARD_FAILBACK] SMN Update Complete");


    /* Failback 과정에서 모든 Shard Meta의 조회는 SMN Update가 완료된 이후에 실행되어야 한다.
     * Lock이 잡히면 SMN Update에 실패할수 있기 때문 */

    /* Failover 수행 이후 Drop Force가 수행되었는지 확인. */
    IDE_TEST( sdi::checkFailoverHistoryOverSMN( aFailbackSMN,
                                                &sNeedSync ) != IDE_SUCCESS );

    if ( sNeedSync == ID_TRUE )
    {
        IDE_TEST_RAISE( sParseTree->mDDLType == SHARD_FAILBACK, needFailbackSync );
    }

    /* SetNode 수행필요 한지 확인. */
    /* Shard Meta NodeInfo Check  */
    sOldSMN = sdi::getSMNForDataNode();

    IDE_TEST( sdi::getExternalNodeInfo( &sNodeInfo,
                                        sOldSMN )
              != IDE_SUCCESS );

    for ( i = 0; i < sNodeInfo.mCount; i++ )
    {
        if ( idlOS::strncmp( sNodeInfo.mNodes[i].mNodeName,
                             sdiZookeeper::mMyNodeName,
                             SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
        {
            sLocalNodeInfo = &sNodeInfo.mNodes[i];
        }
    }

    IDE_TEST( qci::mSessionCallback.mSetShardInternalLocalOperation( aStatement->session->mMmSession,
                                                                     SDI_INTERNAL_OP_FAILBACK ) 
              != IDE_SUCCESS );

    /* NodeInfo에 자신이 없으면 SetNode 필요하다. */
    if ( sLocalNodeInfo == NULL )
    {
        ideLog::log(IDE_SD_0,"[SHARD_FAILBACK] Need Node Add");

        // LOCAL_META_INFO
        IDE_TEST( sdi::getLocalMetaInfo( &sLocalMetaInfo ) 
                  != IDE_SUCCESS );

        IDE_TEST( executeLocalSQLWithSession( aStatement, 
                                              QCI_STMT_MASK_SP,
                                              sSqlStr,
                                              QD_MAX_SQL_LENGTH,
                               "EXEC DBMS_SHARD.SET_NODE( '"QCM_SQL_STRING_SKIP_FMT"', VARCHAR'%s', INTEGER'%"ID_INT32_FMT"',VARCHAR'%s', INTEGER'%"ID_INT32_FMT"', INTEGER'%"ID_INT32_FMT"', INTEGER'%"ID_INT32_FMT"' ); ",
                                              sLocalMetaInfo.mNodeName,
                                              sLocalMetaInfo.mHostIP,
                                              sLocalMetaInfo.mPortNo,
                                              sLocalMetaInfo.mInternalHostIP,
                                              sLocalMetaInfo.mInternalPortNo,
                                              sLocalMetaInfo.mInternalConnType,                                              
                                              sLocalMetaInfo.mShardNodeId )
                  != IDE_SUCCESS );

        ideLog::log(IDE_SD_0,"[SHARD_FAILBACK] LOCAL SET NODE COMPLETE");

        /* get SMN SetNode된 Meta를 가지고 작업 해야하므로 IncreasedSMN을 가져온다. */
        IDE_TEST( getIncreasedSMN( aStatement, &sNewSMN ) != IDE_SUCCESS );

        /* get Alive Node List + LocalNode 필요 */
        IDE_TEST( sdiZookeeper::getAliveNodeNameListIncludeNodeName( &sList,
                                                                     sdiZookeeper::mMyNodeName ) 
                  != IDE_SUCCESS );
        sIsAllocList = ID_TRUE;

        /* NodeList 대상으로 ShardLinker 생성해야 한다. */
        IDE_TEST( sdi::checkShardLinkerWithNodeList ( aStatement,
                                                      sNewSMN,
                                                      ( QC_SMI_STMT( aStatement ) )->getTrans(),
                                                      sList ) != IDE_SUCCESS );

        sClientInfo = aStatement->session->mQPSpecific.mClientInfo;

        IDE_TEST_RAISE( sClientInfo == NULL, ERR_NODE_NOT_EXIST );

        /* SetNode 한걸 전노드 전파 하여야 한다. */
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "EXEC DBMS_SHARD.SET_NODE( VARCHAR'"QCM_SQL_STRING_SKIP_FMT"', VARCHAR'%s', INTEGER'%"ID_INT32_FMT"',VARCHAR'%s', INTEGER'%"ID_INT32_FMT"', INTEGER'%"ID_INT32_FMT"', INTEGER'%"ID_INT32_FMT"' ); ",
                         sLocalMetaInfo.mNodeName,
                         sLocalMetaInfo.mHostIP,
                         sLocalMetaInfo.mPortNo,
                         sLocalMetaInfo.mInternalHostIP,
                         sLocalMetaInfo.mInternalPortNo,
                         sLocalMetaInfo.mInternalConnType,
                         sLocalMetaInfo.mShardNodeId );

        /* Local Node의 SetNode는 따로 수행했으므로 Exception 으로 제외한다. */
        IDE_TEST( executeSQLNodeList( aStatement,
                                      SDI_INTERNAL_OP_FAILBACK,
                                      QCI_STMT_MASK_SP,
                                      sSqlStr,
                                      sdiZookeeper::mMyNodeName,
                                      sList ) != IDE_SUCCESS );

        ideLog::log(IDE_SD_0,"[SHARD_FAILBACK] REMOTE SET NODE COMPLETE");

    }
    else
    {
        ideLog::log(IDE_SD_0,"[SHARD_FAILBACK] Already Node Exist");

        /* get Alive Node List + LocalNode 필요 */
        IDE_TEST( sdiZookeeper::getAliveNodeNameListIncludeNodeName( &sList,
                                                                     sdiZookeeper::mMyNodeName ) 
                  != IDE_SUCCESS );
        sIsAllocList = ID_TRUE;

        /* NodeList 대상으로 ShardLinker 생성해야 한다. */
        IDE_TEST( sdi::checkShardLinkerWithNodeList ( aStatement,
                                                      sOldSMN,
                                                      NULL,
                                                      sList ) != IDE_SUCCESS );

        sClientInfo = aStatement->session->mQPSpecific.mClientInfo;

        IDE_TEST_RAISE( sClientInfo == NULL, ERR_NODE_NOT_EXIST );
    }

    /* Failback은 항상 SMN Update가 발생해야 한다. */
    idlOS::snprintf( sSqlStr, 
                     QD_MAX_SQL_LENGTH,
                     "EXEC DBMS_SHARD.TOUCH_META(); " );

    IDE_TEST( executeSQLNodeList( aStatement,
                                  SDI_INTERNAL_OP_FAILBACK,
                                  QCI_STMT_MASK_SP,
                                  sSqlStr,
                                  NULL,
                                  sList ) != IDE_SUCCESS );

    ideLog::log(IDE_SD_0,"[SHARD_FAILBACK] FAILOVER_HISTORY CHECK");

    /* Check Failover_history_ */
    /* Get ReplicaSetInfo From FailoverHistory using sOldSMN  */
    /* get LocalNode's ReplicaSets Using sOldSMN */
    /* 몇개의 ReplicaSet을 관리중이였는지 확인. 최대 3개( Original 1개+ K-Safety 2개) */
    IDE_TEST( sdm::getFailoverHistoryWithPrimaryNodename( QC_SMI_STMT( aStatement),
                                                          &sFailoverHistoryInfo,
                                                          sdiZookeeper::mMyNodeName,
                                                          aFailbackSMN ) != IDE_SUCCESS );

    /* Failover를 수행했어도 SMN Update가 수행되지 않았을수 있다. DataFailover가 수행되지 않았을수도 있다. */

    IDE_TEST( sdm::getFailoverHistoryWithSMN( QC_SMI_STMT( aStatement),
                                              &sFailoverHistoryInfoWithOtherNode,
                                              aFailbackSMN ) != IDE_SUCCESS );

    /* get ReplicaSet Info */
    IDE_TEST( sdm::getAllReplicaSetInfoSortedByPName( QC_SMI_STMT( aStatement ),
                                                      &sReplicaSetInfo,
                                                      sOldSMN )
              != IDE_SUCCESS );

    /* get All TableInfo */
    IDE_TEST( sdi::getTableInfoAllObject( aStatement,
                                          &sTableInfoList,
                                          sOldSMN )
              != IDE_SUCCESS );

    /* Find Failback MainReplicaSet ID */
    IDE_TEST( sdiZookeeper::getNextNode( sdiZookeeper::mMyNodeName, 
                                         sMyNextNodeName, 
                                         &sState ) != IDE_SUCCESS );

    /* ReplicaSet 중에 Failback Node의 Main ReplicaSet을 찾는다.
     * 다른 객체는 삭제되지 않아 ReplicaSet 단위 복원으로 충분하지만, 
     * Clone Table은 삭제되어 있기 때문에 기존에 연결되어 있던 ReplicaSet을 찾아서 입력해주어야 한다.
     * FailbackNode에 여러개의 ReplicaSet의 연결되어 있을수 있기 때문에 그 중 연결할 Main을 찾아야한다. */
    for ( i = 0; i < sReplicaSetInfo.mCount; i++ )
    {
        if ( sLocalMetaInfo.mShardNodeId == sReplicaSetInfo.mReplicaSets[i].mReplicaSetId )
        {
            sMainReplicaSet = &sReplicaSetInfo.mReplicaSets[i];
            break;
        }
    }

    if ( sMainReplicaSet != NULL )
    {
        sReplicaSetId = sMainReplicaSet->mReplicaSetId;
    }
    else
    {
        /* Failback이니까 First에 무조건 Next 찾아야 한다. 못찾으면 Failover/Failback 중 뭔가 문제 있음. */
        /* K-Safety 가 0 인 경우일 수도 있음.
         * 0이라면 DataFailover가 일어나지 않기 때문에 ReplicaSet의 변경/UnsetNode 등이 수행 되지 않았음.
         * 따라서 Failback 하려는 node 정보가 그대로 남아 있음. */
        for( i = 0 ; i< sReplicaSetInfo.mCount; i++ )
        {
            if ( idlOS::strncmp( sReplicaSetInfo.mReplicaSets[i].mPrimaryNodeName,
                                 sdiZookeeper::mMyNodeName,
                                 SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
            {
                sMainReplicaSet = &sReplicaSetInfo.mReplicaSets[i];
                sReplicaSetId = sMainReplicaSet->mReplicaSetId;
                break;
            }
        }
    }

    /* Replication Item Recover */
    /* 역 이중화 정리 */
    /* 역 이중화는 DataFailover가 발생하였을 경우에만 맺어져 있다. */
    /* Clone FullSync는 따로 처리해야 한다. */
    ideLog::log(IDE_SD_0,"[SHARD_FAILBACK] RECOVER REPLICATION START");

    if ( sFailoverHistoryInfo.mCount != 0  )
    {
        ideLog::log(IDE_SD_0,"[SHARD_FAILBACK] RECOVER REVERSE REPLICATION START");

        IDE_TEST( failbackOrganizeReverseRP( aStatement,
                                             aFailbackFromNodeName,
                                             sTableInfoList,
                                             &sFailoverHistoryInfo,
                                             sReplicaSetId,
                                             sOldSMN ) != IDE_SUCCESS );

        ideLog::log(IDE_SD_0,"[SHARD_FAILBACK] RECOVER REVERSE REPLICATION COMPLETE");
    }
    else
    {
        /* Normal Failback일 경우에만 Clone Only Sync를 한다. 
         * Failback Sync에서는 Clone 포함 모든 Table을 Sync 한다. */
        if ( sParseTree->mDDLType == SHARD_FAILBACK )
        {
            /* ReverseRP가 맺어여 있다면 Clone Table을 Sync 받아 왔을 것이지만 없으면 Clone FullSync가 필요하다.
             * ReverseRP 없이 ReplicaSet에 Failback 해야하는 Node의 ReplicaSet정보가 남아 있다면
             * Clone Fullsync 대상이다. lock 잡고 한번만 하면 된다... 근데 너무 느릴듯 */
            for ( i = 0 ; i < sReplicaSetInfo.mCount ; i++ )
            {
                sReplicaSet = &sReplicaSetInfo.mReplicaSets[i];
                if ( idlOS::strncmp( sReplicaSet->mPrimaryNodeName,
                                     sdiZookeeper::mMyNodeName,
                                     SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
                {
                    /* CLONE FULLSYNC*/
                    IDE_TEST( syncFailbackData( aStatement,
                                                aFailbackFromNodeName,
                                                sdiZookeeper::mMyNodeName,
                                                sTableInfoList,
                                                &sReplicaSetInfo,
                                                &sFailoverHistoryInfo,
                                                sOldSMN,
                                                ID_TRUE )
                              != IDE_SUCCESS );

                    sSyncTables = ID_TRUE;
                }
            }
        }
    }

    /* Failback Sync에서는 Clone 포함 모든 Table을 Sync 한다. */
    if ( sParseTree->mDDLType == SHARD_FAILBACK_SYNC )
    {
        /* TABLE FULLSYNC*/
        IDE_TEST( syncFailbackData( aStatement,
                                    aFailbackFromNodeName,
                                    sdiZookeeper::mMyNodeName,
                                    sTableInfoList,
                                    &sReplicaSetInfo,
                                    &sFailoverHistoryInfo,
                                    sOldSMN,
                                    ID_FALSE )
                  != IDE_SUCCESS );

        sSyncTables = ID_TRUE;
    }

    /* RP Stop/Start는 Lock 획득 후 하니까 실패하네 .. 어쩐다 */
    IDE_TEST( failbackRecoverRPbeforeLock( aStatement,
                                           aFailbackFromNodeName,
                                           &sFailoverHistoryInfoWithOtherNode ) != IDE_SUCCESS );

    ideLog::log(IDE_SD_0,"[SHARD_FAILBACK] RESET SHARD META START");

    /* Shard Meta Update Using Old ReplicaSet's ID, All Table, Procedure */
    IDE_TEST( failbackResetShardMeta( aStatement,
                                      sdiZookeeper::mMyNodeName,
                                      aFailbackFromNodeName,
                                      sTableInfoList,
                                      &sReplicaSetInfo,
                                      &sFailoverHistoryInfo,
                                      sList,
                                      sReplicaSetId,
                                      sOldSMN ) != IDE_SUCCESS );

    ideLog::log(IDE_SD_0,"[SHARD_FAILBACK] RESET SHARD META COMPLETE");

    /* Data Failback = Swap Data Partition on aFailbackFromNodeName */
    IDE_TEST( lockTables4Failback( aStatement,
                                   sSqlStr,
                                   QD_MAX_SQL_LENGTH,
                                   aFailbackFromNodeName,
                                   sTableInfoList,
                                   &sReplicaSetInfo,
                                   &sFailoverHistoryInfo,
                                   sOldSMN ) != IDE_SUCCESS );
    
    ideLog::log(IDE_SD_0,"[SHARD_FAILBACK] TargetTable Lock Acquired");

    if ( sSyncTables == ID_TRUE ) 
    {
        IDE_TEST( syncFailbackDataAfterLock( aStatement,
                                             aFailbackFromNodeName,
                                             sdiZookeeper::mMyNodeName )
                  != IDE_SUCCESS );
    }

    IDE_TEST( failbackOrganizeReverseRPAfterLock( aStatement,
                                                  aFailbackFromNodeName,
                                                  &sFailoverHistoryInfo ) 
              != IDE_SUCCESS );

    IDE_TEST( failbackRecoverRPAfterLock( aStatement,
                                          aFailbackFromNodeName,
                                          sMainReplicaSet,
                                          &sFailoverHistoryInfo,
                                          &sFailoverHistoryInfoWithOtherNode ) != IDE_SUCCESS );

    IDE_TEST( failbackTablePartitionSwap( aStatement,
                                          aFailbackFromNodeName,
                                          sTableInfoList,
                                          &sReplicaSetInfo,
                                          &sFailoverHistoryInfo,
                                          sOldSMN ) != IDE_SUCCESS );

    ideLog::log(IDE_SD_0,"[SHARD_FAILBACK] Partition Swap Complete");

    /* Reset ReplicaSet */
    ideLog::log(IDE_SD_0,"[SHARD_FAILBACK] RESET REPLICASET START");

    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                     "exec dbms_shard.reset_ReplicaSets( '"QCM_SQL_STRING_SKIP_FMT"', '"
                     QCM_SQL_STRING_SKIP_FMT"','"
                     QCM_SQL_STRING_SKIP_FMT"' )",
                     sUserName,
                     aFailbackFromNodeName,       /* Failback 이니까 OldNode는 FailbackFromNode이다.    */
                     sdiZookeeper::mMyNodeName ); /* Failback 이니까 NewNode는 Failback 수행 Node 이다. */

    IDE_TEST( executeSQLNodeList( aStatement,
                                  SDI_INTERNAL_OP_FAILBACK,
                                  QCI_STMT_MASK_SP,
                                  sSqlStr,
                                  NULL,
                                  sList ) != IDE_SUCCESS );

    ideLog::log(IDE_SD_0,"[SHARD_FAILBACK] RESET REPLICASET COMPLETE");

    /* deleteFailoverHistoryWithSMN */
    if ( sFailoverHistoryInfoWithOtherNode.mCount != 0 )
    { 
        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                         "DELETE FROM SYS_SHARD.FAILOVER_HISTORY_ WHERE SMN = "QCM_SQL_BIGINT_FMT, aFailbackSMN );

        IDE_TEST( executeSQLNodeList( aStatement,
                                      SDI_INTERNAL_OP_FAILBACK,
                                      QCI_STMT_MASK_SP,
                                      sSqlStr,
                                      NULL,
                                      sList ) != IDE_SUCCESS );
    }

    ideLog::log(IDE_SD_0,"[SHARD_FAILBACK] REMOVE FAILOVERHISTORY COMPLETE");

    sIsAllocList = ID_FALSE;
    sdiZookeeper::freeList( sList, SDI_ZKS_LIST_NODENAME );

    return IDE_SUCCESS;

    IDE_EXCEPTION( needFailbackSync )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDSD_DROP_FORCE_DETECTED ) );
    }
    IDE_EXCEPTION( ERR_NODE_NOT_EXIST )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_NODE_NOT_EXIST ) );
    }
    IDE_EXCEPTION_END;

    if ( sIsAllocList == ID_TRUE )
    {
        sdiZookeeper::freeList( sList, SDI_ZKS_LIST_NODENAME );
    }

    return IDE_FAILURE;
}


IDE_RC qdsd::joinRecoverRP( qcStatement        * aStatement,
                            sdiReplicaSetInfo  * aReplicaSetInfo )
{
    UInt             i = 0;
    sdiReplicaSet  * sReplicaSet = NULL;
    SChar            sSqlStr[QD_MAX_SQL_LENGTH + 1];
    idBool           sIsAlive = ID_FALSE;
    UInt             sShardDDLLockTryCount = QCG_GET_SESSION_SHARD_DDL_LOCK_TRY_COUNT(aStatement);
    UInt             sRemoteLockTimeout = QCG_GET_SESSION_SHARD_DDL_LOCK_TIMEOUT(aStatement);

    ideLog::log(IDE_SD_0,"[SHARD_FAILBACK] Failback Recover RP Start");

    /* FailbackNode Send RP Start */
    for ( i = 0; i < aReplicaSetInfo->mCount; i++ )
    {
        sReplicaSet = &aReplicaSetInfo->mReplicaSets[i];

        /* SendRP Start */
        if ( idlOS::strncmp( sReplicaSet->mPrimaryNodeName,
                             sdiZookeeper::mMyNodeName,
                             SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
        {
            if ( SDI_IS_NULL_NAME( sReplicaSet->mFirstBackupNodeName ) == ID_FALSE )
            {
                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                 "ALTER REPLICATION "QCM_SQL_STRING_SKIP_FMT" START ",
                                 sReplicaSet->mFirstReplName );

                if ( executeLocalSQL( aStatement, 
                                      QCI_STMT_MASK_DCL,
                                      sSqlStr,
                                      QD_MAX_SQL_LENGTH,
                                      NULL )
                     == IDE_SUCCESS )
                {
                    idlOS::snprintf( sSqlStr,
                                     QD_MAX_SQL_LENGTH,
                                     "alter replication "QCM_SQL_STRING_SKIP_FMT" stop",
                                     sReplicaSet->mFirstReplName );

                    (void)sdiZookeeper::addPendingJob( sSqlStr,
                                                       sdiZookeeper::mMyNodeName,
                                                       ZK_PENDING_JOB_AFTER_ROLLBACK,
                                                       QCI_STMT_MASK_DDL );

                    ideLog::log( IDE_SD_21,
                                 "[SHARD_FAILBACK] Replication Started : %s", 
                                 sReplicaSet->mFirstReplName );

                    /* Flush 호출 */
                    IDE_TEST( flushReplicationForInternal( aStatement,
                                                           sdiZookeeper::mMyNodeName,
                                                           sReplicaSet->mFirstReplName )
                              != IDE_SUCCESS );
                }
                else
                {

                    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                     "ALTER REPLICATION "QCM_SQL_STRING_SKIP_FMT" START RETRY ",
                                     sReplicaSet->mFirstReplName );

                    IDE_TEST( executeLocalSQL( aStatement, 
                                               QCI_STMT_MASK_DCL,
                                               sSqlStr,
                                               QD_MAX_SQL_LENGTH,
                                               NULL )
                                != IDE_SUCCESS );

                    idlOS::snprintf( sSqlStr,
                                     QD_MAX_SQL_LENGTH,
                                     "alter replication "QCM_SQL_STRING_SKIP_FMT" stop",
                                     sReplicaSet->mFirstReplName );

                    (void)sdiZookeeper::addPendingJob( sSqlStr,
                                                       sdiZookeeper::mMyNodeName,
                                                       ZK_PENDING_JOB_AFTER_ROLLBACK,
                                                       QCI_STMT_MASK_DDL );

                    ideLog::log( IDE_SD_21,
                                 "[SHARD_FAILBACK] Replication Start Retry : %s", 
                                 sReplicaSet->mFirstReplName );
                }
            }
            if ( SDI_IS_NULL_NAME( sReplicaSet->mSecondBackupNodeName ) == ID_FALSE )
            {
                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                 "ALTER REPLICATION "QCM_SQL_STRING_SKIP_FMT" START ",
                                 sReplicaSet->mSecondReplName );

                if ( executeLocalSQL( aStatement, 
                                      QCI_STMT_MASK_DCL,
                                      sSqlStr,
                                      QD_MAX_SQL_LENGTH,
                                      NULL )
                     == IDE_SUCCESS )
                {
                    idlOS::snprintf( sSqlStr,
                                     QD_MAX_SQL_LENGTH,
                                     "alter replication "QCM_SQL_STRING_SKIP_FMT" stop",
                                     sReplicaSet->mSecondReplName );

                    (void)sdiZookeeper::addPendingJob( sSqlStr,
                                                       sdiZookeeper::mMyNodeName,
                                                       ZK_PENDING_JOB_AFTER_ROLLBACK,
                                                       QCI_STMT_MASK_DDL );

                    ideLog::log( IDE_SD_21,
                                 "[SHARD_FAILBACK] Replication Started : %s", 
                                 sReplicaSet->mSecondReplName );

                    /* Flush 호출 */
                    IDE_TEST( flushReplicationForInternal( aStatement,
                                                           sdiZookeeper::mMyNodeName,
                                                           sReplicaSet->mSecondReplName )
                              != IDE_SUCCESS );
                }
                else
                {

                    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                     "ALTER REPLICATION "QCM_SQL_STRING_SKIP_FMT" START RETRY ",
                                     sReplicaSet->mSecondReplName );

                    IDE_TEST( executeLocalSQL( aStatement, 
                                               QCI_STMT_MASK_DCL,
                                               sSqlStr,
                                               QD_MAX_SQL_LENGTH,
                                               NULL )
                                != IDE_SUCCESS );

                    idlOS::snprintf( sSqlStr,
                                     QD_MAX_SQL_LENGTH,
                                     "alter replication "QCM_SQL_STRING_SKIP_FMT" stop",
                                     sReplicaSet->mSecondReplName );

                    (void)sdiZookeeper::addPendingJob( sSqlStr,
                                                       sdiZookeeper::mMyNodeName,
                                                       ZK_PENDING_JOB_AFTER_ROLLBACK,
                                                       QCI_STMT_MASK_DDL );

                    ideLog::log( IDE_SD_21,
                                 "[SHARD_FAILBACK] Replication Start Retry : %s", 
                                 sReplicaSet->mSecondReplName );
                }
            }
        }
    }

    /* Receive RP Start */
    for ( i = 0; i < aReplicaSetInfo->mCount; i++ )
    {
        sReplicaSet = &aReplicaSetInfo->mReplicaSets[i];
        sIsAlive    = ID_FALSE;

        /* Receive RP Start */
        if ( idlOS::strncmp( sReplicaSet->mFirstBackupNodeName,
                             sdiZookeeper::mMyNodeName,
                             SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
        {
            IDE_TEST ( sdiZookeeper::checkNodeAlive( sReplicaSet->mPrimaryNodeName,
                                                     &sIsAlive ) != IDE_SUCCESS );

            if ( sIsAlive == ID_TRUE )
            {
                /* Join 상황에서 Failback Node의 Receive RP는 Sender가 이미 Start retry 중 일수 있다.
                 * Flush 에 성공하면 Start 된걸로 판단한다. */
                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                 "ALTER REPLICATION "QCM_SQL_STRING_SKIP_FMT" FLUSH WAIT %"ID_UINT32_FMT,
                                 sReplicaSet->mFirstReplName,
                                 (sShardDDLLockTryCount * sRemoteLockTimeout) );

                if ( executeRemoteSQL( aStatement, 
                                        sReplicaSet->mPrimaryNodeName,
                                        SDI_INTERNAL_OP_FAILBACK,
                                        QCI_STMT_MASK_DCL,
                                        sSqlStr,
                                        QD_MAX_SQL_LENGTH,
                                        NULL ) == IDE_SUCCESS )
                {
                    ideLog::log( IDE_SD_0,
                                 "[SHARD_FAILBACK] Failback Receive FLUSH RP : %s", 
                                 sReplicaSet->mFirstReplName );
                }
                else
                {
                    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                     "ALTER REPLICATION "QCM_SQL_STRING_SKIP_FMT" START ",
                                     sReplicaSet->mFirstReplName );

                    IDE_TEST( executeRemoteSQL( aStatement, 
                                                sReplicaSet->mPrimaryNodeName,
                                                SDI_INTERNAL_OP_FAILBACK,
                                                QCI_STMT_MASK_DCL,
                                                sSqlStr,
                                                QD_MAX_SQL_LENGTH,
                                                NULL )
                                != IDE_SUCCESS );

                    idlOS::snprintf( sSqlStr,
                                     QD_MAX_SQL_LENGTH,
                                     "alter replication "QCM_SQL_STRING_SKIP_FMT" start retry",
                                     sReplicaSet->mFirstReplName );

                    (void)sdiZookeeper::addPendingJob( sSqlStr,
                                                       sReplicaSet->mPrimaryNodeName,
                                                       ZK_PENDING_JOB_AFTER_ROLLBACK,
                                                       QCI_STMT_MASK_DDL );


                    ideLog::log( IDE_SD_0,
                                 "[SHARD_FAILBACK] Failback Receive Start RP : %s", 
                                 sReplicaSet->mFirstReplName );

                    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                     "ALTER REPLICATION "QCM_SQL_STRING_SKIP_FMT" FLUSH WAIT %"ID_UINT32_FMT,
                                     sReplicaSet->mFirstReplName,
                                     (sShardDDLLockTryCount * sRemoteLockTimeout) );

                    IDE_TEST( executeRemoteSQL( aStatement, 
                                                sReplicaSet->mPrimaryNodeName,
                                                SDI_INTERNAL_OP_FAILBACK,
                                                QCI_STMT_MASK_DCL,
                                                sSqlStr,
                                                QD_MAX_SQL_LENGTH,
                                                NULL )
                                != IDE_SUCCESS );

                    ideLog::log( IDE_SD_0,
                                 "[SHARD_FAILBACK] Failback Receive FLUSH RP : %s", 
                                 sReplicaSet->mFirstReplName );
                }
            }
        }
        if ( idlOS::strncmp( sReplicaSet->mSecondBackupNodeName,
                             sdiZookeeper::mMyNodeName,
                             SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
        {
            IDE_TEST ( sdiZookeeper::checkNodeAlive( sReplicaSet->mPrimaryNodeName,
                                                     &sIsAlive ) != IDE_SUCCESS );

            if ( sIsAlive == ID_TRUE )
            {
                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                 "ALTER REPLICATION "QCM_SQL_STRING_SKIP_FMT" FLUSH WAIT %"ID_UINT32_FMT,
                                 sReplicaSet->mSecondReplName,
                                 (sShardDDLLockTryCount * sRemoteLockTimeout) );

                if ( executeRemoteSQL( aStatement, 
                                        sReplicaSet->mPrimaryNodeName,
                                        SDI_INTERNAL_OP_FAILBACK,
                                        QCI_STMT_MASK_DCL,
                                        sSqlStr,
                                        QD_MAX_SQL_LENGTH,
                                        NULL )
                     == IDE_SUCCESS )
                {
                    ideLog::log( IDE_SD_0,
                                 "[SHARD_FAILBACK] Failback Receive FLUSH RP : %s", 
                                 sReplicaSet->mSecondReplName );
                }
                else
                {

                    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                     "ALTER REPLICATION "QCM_SQL_STRING_SKIP_FMT" START ",
                                     sReplicaSet->mSecondReplName );

                    IDE_TEST( executeRemoteSQL( aStatement, 
                                                sReplicaSet->mPrimaryNodeName,
                                                SDI_INTERNAL_OP_FAILBACK,
                                                QCI_STMT_MASK_DCL,
                                                sSqlStr,
                                                QD_MAX_SQL_LENGTH,
                                                NULL )
                                != IDE_SUCCESS );

                    idlOS::snprintf( sSqlStr,
                                     QD_MAX_SQL_LENGTH,
                                     "alter replication "QCM_SQL_STRING_SKIP_FMT" start retry",
                                     sReplicaSet->mSecondReplName );

                    (void)sdiZookeeper::addPendingJob( sSqlStr,
                                                       sReplicaSet->mPrimaryNodeName,
                                                       ZK_PENDING_JOB_AFTER_ROLLBACK,
                                                       QCI_STMT_MASK_DDL );

                    ideLog::log( IDE_SD_0,
                                 "[SHARD_FAILBACK] Failback Receive Start RP : %s", 
                                 sReplicaSet->mSecondReplName );

                    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                     "ALTER REPLICATION "QCM_SQL_STRING_SKIP_FMT" FLUSH WAIT %"ID_UINT32_FMT,
                                     sReplicaSet->mSecondReplName,
                                     (sShardDDLLockTryCount * sRemoteLockTimeout));

                    IDE_TEST( executeRemoteSQL( aStatement, 
                                                sReplicaSet->mPrimaryNodeName,
                                                SDI_INTERNAL_OP_FAILBACK,
                                                QCI_STMT_MASK_DCL,
                                                sSqlStr,
                                                QD_MAX_SQL_LENGTH,
                                                NULL )
                                != IDE_SUCCESS );

                    ideLog::log( IDE_SD_0,
                                 "[SHARD_FAILBACK] Failback Receive FLUSH RP : %s", 
                                 sReplicaSet->mSecondReplName );
                }
            }
        }
    }

    ideLog::log(IDE_SD_0,"[SHARD_FAILBACK] Failback Recover RP Complete");

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdsd::executeAlterShardFlag( qcStatement        * aStatement )
{
/***********************************************************************
 * TASK-7307 DML Data Consistency in Shard
 *   모든 샤드 테이블에 대해서 usable, shard_flag 값 초기화 수행
 ***********************************************************************/
    sdiTableInfoList  * sTmpTableInfoList = NULL;    
    sdiTableInfoList  * sTableInfoList = NULL;
    sdiTableInfo      * sTableInfo  = NULL;
    sdiShardObjectType  sShardTableType = SDI_NON_SHARD_OBJECT;
    sdiLocalMetaInfo    sLocalMetaInfo;
    SChar               sSqlStr[QD_MAX_SQL_LENGTH + 1];
    ULong               sSMN = SDI_NULL_SMN;

    // LOCAL_META_INFO
    IDE_TEST( sdi::getLocalMetaInfo( &sLocalMetaInfo ) 
              != IDE_SUCCESS );

    sSMN = sdi::getSMNForDataNode();

    /* get All TableInfo */
    IDE_TEST( sdi::getTableInfoAllObject( aStatement,
                                          &sTableInfoList,
                                          sSMN )
              != IDE_SUCCESS );

    for ( sTmpTableInfoList = sTableInfoList;
          sTmpTableInfoList != NULL;
          sTmpTableInfoList = sTmpTableInfoList->mNext )

    {
        sTableInfo = sTmpTableInfoList->mTableInfo;

        /* check Table Type */
        sShardTableType = sdi::getShardObjectType( sTableInfo );

        if ( sTableInfo->mObjectType == 'T' ) /* Table */
        {
            switch ( sShardTableType )
            {
                case SDI_SINGLE_SHARD_KEY_DIST_OBJECT:
                    IDE_TEST( executeLocalSQL( aStatement, 
                                               QCI_STMT_MASK_DDL,
                                               sSqlStr,
                                               QD_MAX_SQL_LENGTH,
                                               "alter table "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" shard split",
                                               sTableInfo->mUserName,
                                               sTableInfo->mObjectName )
                              != IDE_SUCCESS );
                    break;
                case SDI_COMPOSITE_SHARD_KEY_DIST_OBJECT:
                    continue;
                case SDI_SOLO_DIST_OBJECT:
                    IDE_TEST( executeLocalSQL( aStatement, 
                                               QCI_STMT_MASK_DDL,
                                               sSqlStr,
                                               QD_MAX_SQL_LENGTH,
                                               "alter table "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" shard solo",
                                               sTableInfo->mUserName,
                                               sTableInfo->mObjectName )
                              != IDE_SUCCESS );
                    break;
                case SDI_CLONE_DIST_OBJECT:
                    idlOS::snprintf( sSqlStr,
                                     QD_MAX_SQL_LENGTH,
                                     "alter table "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" shard clone",
                                     sTableInfo->mUserName,
                                     sTableInfo->mObjectName );

                    IDE_TEST( sdi::shardExecTempDMLOrDCLWithNewTrans( aStatement,
                                                                      sSqlStr )
                              != IDE_SUCCESS );

                    // 실패 시 revert 처리
                    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                     "alter table "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" shard none",
                                     sTableInfo->mUserName,
                                     sTableInfo->mObjectName );

                    (void)sdiZookeeper::addPendingJob( sSqlStr,
                                                       sLocalMetaInfo.mNodeName,
                                                       ZK_PENDING_JOB_AFTER_ROLLBACK,
                                                       QCI_STMT_MASK_DDL );
                    break;
                case SDI_NON_SHARD_OBJECT:
                default:
                    /* Non Shard Table이 오면 안됨. */
                    IDE_DASSERT( 0 );
                    break;
            }

            /* Default Partition check */
            if ( ( sTableInfo->mDefaultNodeId == sLocalMetaInfo.mShardNodeId ) &&
                 ( sShardTableType == SDI_SINGLE_SHARD_KEY_DIST_OBJECT ) )
            {
                IDE_TEST( executeLocalSQL( aStatement, 
                                           QCI_STMT_MASK_DDL,
                                           sSqlStr,
                                           QD_MAX_SQL_LENGTH,
                                           "alter table "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" access partition "QCM_SQL_STRING_SKIP_FMT" usable",
                                           sTableInfo->mUserName,
                                           sTableInfo->mObjectName,
                                           sTableInfo->mDefaultPartitionName )
                      != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdsd::executeAlterShardNone( qcStatement        * aStatement )
{
/***********************************************************************
 * TASK-7307 DML Data Consistency in Shard
 *   모든 샤드 테이블에 대해서 usable, shard_flag 값 non-shard 초기화 
 ***********************************************************************/
    sdiTableInfoList  * sTmpTableInfoList = NULL;    
    sdiTableInfoList  * sTableInfoList = NULL;
    sdiTableInfo      * sTableInfo  = NULL;
    sdiLocalMetaInfo    sLocalMetaInfo;
    SChar               sSqlStr[QD_MAX_SQL_LENGTH + 1];
    ULong               sSMN = SDI_NULL_SMN;

    // LOCAL_META_INFO
    IDE_TEST( sdi::getLocalMetaInfo( &sLocalMetaInfo ) 
              != IDE_SUCCESS );

    sSMN = sdi::getSMNForDataNode();

    /* get All TableInfo */
    IDE_TEST( sdi::getTableInfoAllObject( aStatement,
                                          &sTableInfoList,
                                          sSMN )
              != IDE_SUCCESS );

    for ( sTmpTableInfoList = sTableInfoList;
          sTmpTableInfoList != NULL;
          sTmpTableInfoList = sTmpTableInfoList->mNext )

    {
        sTableInfo = sTmpTableInfoList->mTableInfo;

        /* check Table Type */
        if ( sTableInfo->mObjectType == 'T' ) /* Table */
        {
            IDE_TEST( executeLocalSQL( aStatement, 
                                       QCI_STMT_MASK_DDL,
                                       sSqlStr,
                                       QD_MAX_SQL_LENGTH,
                                       "alter table "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" shard none",
                                       sTableInfo->mUserName,
                                       sTableInfo->mObjectName )
              != IDE_SUCCESS );

#if 0
            idlOS::snprintf( sBufferNameStr,
                             QC_MAX_OBJECT_NAME_LEN + 1,
                             "%s%s",
                             SDI_BACKUP_TABLE_PREFIX,
                             sTableInfo->mObjectName );

            if ( ( SDU_SHARD_LOCAL_FORCE != 1 ) &&
                 ( sLocalMetaInfo.mKSafety > 1 ) &&
                 ( sdi::getShardObjectType( sTableInfo ) != SDI_COMPOSITE_SHARD_KEY_DIST_OBJECT ) )
            {
                IDE_TEST( executeLocalSQL( aStatement, 
                                           QCI_STMT_MASK_DDL,
                                           sSqlStr,
                                           QD_MAX_SQL_LENGTH,
                                           "alter table "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" shard none",
                                           sTableInfo->mUserName,
                                           sBufferNameStr )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }
#endif
        }
        else
        {
            /* Nothing to do */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdsd::executeShardAddForClone( qcStatement * aStatement )
{
    qdShardParseTree   * sParseTree = NULL;
    sdiTableInfoList   * sTableInfoList = NULL;
    sdiTableInfoList   * sTmpTableInfoList = NULL;
    sdiTableInfo       * sTableInfo  = NULL;
    SChar              * sSqlStr = NULL;
    const SChar        * sBackupTableQuery = NULL;
    qdReShardAttribute * sReshardAttrbute = NULL;
    UInt                 sLen = 0;
    UInt                 sErrCode = 0;
    sdiLocalMetaInfo     sLocalMetaInfo;
    sdiGlobalMetaInfo    sMetaNodeInfo = { ID_ULONG(0) };
    SChar                sBufferNameStr[QC_MAX_OBJECT_NAME_LEN + 1];
        
    sParseTree = (qdShardParseTree *)aStatement->myPlan->parseTree;

    IDE_TEST( sdi::getGlobalMetaInfoCore( QC_SMI_STMT( aStatement ),
                                          &sMetaNodeInfo )
              != IDE_SUCCESS );

    IDE_TEST( sdi::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );
    
    IDE_TEST( sdi::getTableInfoAllObject( aStatement,
                                          &sTableInfoList,
                                          sMetaNodeInfo.mShardMetaNumber )
              != IDE_SUCCESS );
    
    IDE_TEST( qci::mSessionCallback.mSetShardInternalLocalOperation(
                  aStatement->session->mMmSession,
                  SDI_INTERNAL_OP_NORMAL ) 
              != IDE_SUCCESS );
    
    sParseTree->mNodeName.stmtText = sLocalMetaInfo.mNodeName;
    sParseTree->mNodeName.offset   = 0;
    sParseTree->mNodeName.size     = idlOS::strlen( sLocalMetaInfo.mNodeName );

    for ( sTmpTableInfoList = sTableInfoList;
          sTmpTableInfoList != NULL;
          sTmpTableInfoList = sTmpTableInfoList->mNext )
    {
        sTableInfo = sTmpTableInfoList->mTableInfo;
        
        switch ( sdi::getShardObjectType( sTableInfo ))
        {
            case SDI_NON_SHARD_OBJECT:                   
            case SDI_SINGLE_SHARD_KEY_DIST_OBJECT:                   
            case SDI_COMPOSITE_SHARD_KEY_DIST_OBJECT:
            case SDI_SOLO_DIST_OBJECT:
                // nothing to do
                break;                    
            case SDI_CLONE_DIST_OBJECT:
                IDE_TEST( QC_QMX_MEM(aStatement)->alloc( ID_SIZEOF(qdReShardAttribute),
                                                         (void**) &sReshardAttrbute )
                          != IDE_SUCCESS );

                QD_SET_INIT_RESHARD_ATTR( sReshardAttrbute );
        
                sReshardAttrbute->mObjectType = sTableInfo->mObjectType;

                sReshardAttrbute->mUserName.stmtText = sTableInfo->mUserName;
                sReshardAttrbute->mUserName.offset   = 0;
                sReshardAttrbute->mUserName.size     = idlOS::strlen( sTableInfo->mUserName );

                sReshardAttrbute->mObjectName.stmtText = sTableInfo->mObjectName;
                sReshardAttrbute->mObjectName.offset   = 0;
                sReshardAttrbute->mObjectName.size     = idlOS::strlen( sTableInfo->mObjectName );

                if ( sTableInfo->mObjectType == 'T' )
                {
                    sBackupTableQuery  = "CREATE TABLE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" FROM TABLE SCHEMA "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" USING PREFIX "QCM_SQL_STRING_SKIP_FMT"";
                        
                    sLen = ( ID_SIZEOF(sTableInfo->mUserName) * 2 ) +
                        ( ID_SIZEOF(sTableInfo->mObjectName) * 2 ) +
                        ( idlOS::strlen(SDI_BACKUP_TABLE_PREFIX) * 2 ) + 1;

                    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( aStatement->qmxMem,
                                                      SChar,
                                                      sLen,
                                                      &sSqlStr )
                              != IDE_SUCCESS );

                    idlOS::snprintf( sBufferNameStr,
                                     QC_MAX_OBJECT_NAME_LEN + 1,
                                     "%s%s",
                                     SDI_BACKUP_TABLE_PREFIX,
                                     sTableInfo->mObjectName );
                    
                    // bak table create
                    idlOS::snprintf( sSqlStr,
                                     sLen,
                                     "DROP TABLE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" CASCADE",
                                     sTableInfo->mUserName,
                                     sBufferNameStr );
                    
                    if ( sdi::shardExecTempDMLOrDCLWithNewTrans( aStatement, sSqlStr ))
                    {
                        sErrCode = ideGetErrorCode();

                        if ( sErrCode == qpERR_ABORT_QCV_NOT_EXISTS_TABLE )
                        {
                            IDE_CLEAR();
                        }
                        else
                        {
                            IDE_RAISE( ERR_ALREDAY_EXIST_ERROR_MSG );
                        }
                    }

                    // bak table create
                    idlOS::snprintf( sSqlStr,
                                     sLen,
                                     sBackupTableQuery,
                                     sTableInfo->mUserName,
                                     sBufferNameStr,
                                     sTableInfo->mUserName,
                                     sTableInfo->mObjectName,
                                     SDI_BACKUP_TABLE_PREFIX );

                    IDE_TEST( sdi::shardExecTempDMLOrDCLWithNewTrans( aStatement,
                                                                      sSqlStr )
                              != IDE_SUCCESS );
                }
                else
                {
                    // nothing to do
                }

                sReshardAttrbute->next = NULL;            

                if( sParseTree->mReShardAttr == NULL )
                {
                    sParseTree->mReShardAttr = sReshardAttrbute;
                }
                else
                {
                    sReshardAttrbute->next = sParseTree->mReShardAttr;
                    sParseTree->mReShardAttr = sReshardAttrbute;
                }
                break;                                        
            default:
                IDE_DASSERT(0);
                break;
        }
    }
        
    if ( sParseTree->mReShardAttr != NULL )
    {
        aStatement->session->mQPSpecific.mFlag &= ~QC_SESSION_SAHRD_ADD_CLONE_MASK;
        aStatement->session->mQPSpecific.mFlag |= QC_SESSION_SAHRD_ADD_CLONE_TRUE;

        // shard move를 사용하려면 SHARD_STATUS 1 이여야 한다.
        sdi::setShardStatus(1); // 0->1

        IDE_TEST( validateShardMove( aStatement ) != IDE_SUCCESS );
        IDE_TEST( executeShardMove( aStatement ) != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ALREDAY_EXIST_ERROR_MSG )
    {
        // nothing to do
    }
    IDE_EXCEPTION_END;

    sdi::setShardStatus(0);
    
    return IDE_FAILURE;
}

IDE_RC qdsd::checkCloneTableDataAndTruncateForShardAdd( qcStatement        * aStatement,
                                                        qdReShardAttribute * aReShardAttr )
{
    SChar                sUserName[QC_MAX_OBJECT_NAME_LEN] = {0,};
    SChar                sTableName[QC_MAX_OBJECT_NAME_LEN] = SDI_BACKUP_TABLE_PREFIX;
    idBool               sRecordExist = ID_FALSE;
    mtdBigintType        sResult;
    ULong                sTotalDataCount = 0;
    qdReShardAttribute * sReShardAttr = NULL;     
    const SChar          sRowCntSQLFmt[] = "SELECT COUNT(*) FROM "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT"";
    const SChar          sTruncateSQLFmt[] = "TRUNCATE TABLE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT"";
    SChar sSqlStr[ ( QC_MAX_OBJECT_NAME_LEN * 2 ) + 
                   IDL_MAX(ID_SIZEOF(sRowCntSQLFmt), ID_SIZEOF(sTruncateSQLFmt)) ] = {0,};

    for ( sReShardAttr = aReShardAttr;
          sReShardAttr != NULL;
          sReShardAttr = sReShardAttr->next )
    {
        IDE_DASSERT( QC_IS_NULL_NAME( sReShardAttr->mUserName ) != ID_TRUE );
        IDE_DASSERT( sReShardAttr->mShardObjectInfo != NULL );
        
        if ( sReShardAttr->mObjectType == 'T' )
        {
            QC_STR_COPY( sUserName, sReShardAttr->mUserName );
            QC_STR_COPY( sTableName, sReShardAttr->mObjectName );
            
            idlOS::snprintf( sSqlStr,
                             ID_SIZEOF(sSqlStr),
                             sRowCntSQLFmt,
                             sUserName,
                             sTableName );

            IDE_TEST( qciMisc::runSelectOneRowforDDL( QC_SMI_STMT( aStatement ),
                                                      sSqlStr,
                                                      &sResult,
                                                      &sRecordExist,
                                                      ID_FALSE )
                      != IDE_SUCCESS );
    
            IDE_TEST_RAISE( sRecordExist == ID_FALSE,
                            ERR_NO_DATA_FOUND );

            sTotalDataCount = (UInt)sResult;

            if ( sTotalDataCount > 0 )
            {
                idlOS::snprintf( sSqlStr,
                                 ID_SIZEOF(sSqlStr),
                                 sTruncateSQLFmt,
                                 sUserName,
                                 sTableName );
                
                IDE_TEST( qciMisc::runDDLforInternalWithMmSession(
                              aStatement->mStatistics,
                              aStatement->session->mMmSession,
                              QC_SMI_STMT( aStatement ),
                              QC_EMPTY_USER_ID,
                              QCI_SESSION_INTERNAL_DDL_TRUE,
                              sSqlStr )
                          != IDE_SUCCESS );
            }
            else
            {
                // nothing to do
            }
        }
        else
        {
            /* 'P' procedure: do nothing */
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NO_DATA_FOUND )
    {
        IDE_SET(ideSetErrorCode( qpERR_ABORT_QSX_NO_DATA_FOUND ));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC qdsd::executeResetCloneMeta( qcStatement * aStatement )
{
    sdiTableInfoList  * sTableInfoList = NULL;
    sdiTableInfoList  * sTmpTableInfoList = NULL;
    sdiTableInfo      * sTableInfo  = NULL;
    sdiClientInfo     * sClientInfo  = NULL;
    sdiConnectInfo    * sConnectInfo = NULL;
    UInt                i = 0;
    ULong               sSMN = SDI_NULL_SMN;
    sdiShardObjectType  sShardTableType = SDI_NON_SHARD_OBJECT;
    sdiLocalMetaInfo    sLocalMetaInfo;
    SChar               sSqlStr[QD_MAX_SQL_LENGTH + 1];
    
    sSMN = sdi::getSMNForDataNode();

    IDE_TEST( sdi::getTableInfoAllObject( aStatement,
                                          &sTableInfoList,
                                          sSMN )
              != IDE_SUCCESS );

    IDE_TEST( sdi::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );

    IDE_TEST( sdi::checkShardLinker( aStatement ) != IDE_SUCCESS );

    for ( sTmpTableInfoList = sTableInfoList;
          sTmpTableInfoList != NULL;
          sTmpTableInfoList = sTmpTableInfoList->mNext )
    {
        sTableInfo = sTmpTableInfoList->mTableInfo;

        /* check Table Type */
        sShardTableType = sdi::getShardObjectType( sTableInfo );

        switch ( sShardTableType )
        {
            case SDI_SINGLE_SHARD_KEY_DIST_OBJECT:
            case SDI_SOLO_DIST_OBJECT:
            case SDI_COMPOSITE_SHARD_KEY_DIST_OBJECT:
                break;
            case SDI_CLONE_DIST_OBJECT:
                /* Clones_ 에서 제거 해 주어야 한다. */
                if ( sTableInfo->mObjectType == 'T' ) /* Table */
                {
                    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                     "exec dbms_shard.reset_shard_partition_node('"
                                     QCM_SQL_STRING_SKIP_FMT"', '"
                                     QCM_SQL_STRING_SKIP_FMT"', '"
                                     QCM_SQL_STRING_SKIP_FMT"', "
                                     "NULL ) ",
                                     sTableInfo->mUserName,
                                     sTableInfo->mObjectName,
                                     sLocalMetaInfo.mNodeName );
                }
                else /* Procedure */
                {
                    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                     "exec dbms_shard.reset_shard_resident_node('"
                                     QCM_SQL_STRING_SKIP_FMT"', '"
                                     QCM_SQL_STRING_SKIP_FMT"', '"
                                     QCM_SQL_STRING_SKIP_FMT"', "
                                     "NULL ) ",                // clone not exist value
                                     sTableInfo->mUserName,
                                     sTableInfo->mObjectName,
                                     sLocalMetaInfo.mNodeName );
                }

                sClientInfo = aStatement->session->mQPSpecific.mClientInfo;

                IDE_TEST_RAISE( sClientInfo == NULL, ERR_NODE_NOT_EXIST );

                sConnectInfo = sClientInfo->mConnectInfo;

                for ( i = 0; i < sClientInfo->mCount; i++, sConnectInfo++ )
                {
                    IDE_TEST( executeRemoteSQL( aStatement, 
                                                sConnectInfo->mNodeName,
                                                SDI_INTERNAL_OP_NORMAL,
                                                QCI_STMT_MASK_SP,
                                                sSqlStr,
                                                QD_MAX_SQL_LENGTH,
                                                NULL )
                              != IDE_SUCCESS );
                }
                break;
            case SDI_NON_SHARD_OBJECT:
            default:
                /* Non Shard Table이 오면 안됨. */
                IDE_DASSERT( 0 );
                break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NODE_NOT_EXIST )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_NODE_NOT_EXIST ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdsd::checkShardPIN( qcStatement * aStatement )
{
    sdiShardPin sShardPin = qci::mSessionCallback.mGetShardPIN( aStatement->session->mMmSession );

    /* shard PIN을 체크한다. */
    if( sShardPin == SDI_SHARD_PIN_INVALID )
    {
        /* SMN을 reload하고 shard PIN을 세팅한다.  */
        IDE_TEST( qci::mSessionCallback.mReloadShardMetaNumber(
                aStatement->session->mMmSession,
                ID_TRUE )
            != IDE_SUCCESS );

        /* shard PIN을 세팅한다. */
        qci::mSessionCallback.mSetNewShardPIN( aStatement->session->mMmSession );
    }
    else
    {
        /* 이미 shard PIN이 세팅되어 있다면 따로 작업할 필요가 없다. */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdsd::syncFailbackData( qcStatement        * aStatement,
                               SChar              * aFromNodeName,
                               SChar              * aToNodeName,
                               sdiTableInfoList   * aTableInfoList,
                               sdiReplicaSetInfo  * aReplicaSetInfo,
                               sdiReplicaSetInfo  * aFailoverHistoryInfo,
                               ULong                aSMN,
                               idBool               aIsCloneOnly )
{
    sdiTableInfoList  * sTmpTableInfoList = NULL;    

    sdiTableInfo      * sTableInfo  = NULL;
    sdiRangeInfo        sRangeInfos;
    sdiReplicaSet     * sFailoverHistory = NULL;
    sdiReplicaSet     * sReplicaSet = NULL;

    qdShardParseTree   * sParseTree = NULL;

    sdiShardObjectType  sShardTableType = SDI_NON_SHARD_OBJECT;

    UInt                i;
    UInt                j;

    SChar               sSqlStr[QD_MAX_SQL_LENGTH + 1];

    SChar               sSyncReplicationName[SDI_REPLICATION_NAME_MAX_SIZE + 1] = { 0, };

    UInt                sExecCount = 0;

    sdiNode             sNode; 

    SChar               sReverseReplName[SDI_REPLICATION_NAME_MAX_SIZE + 1];

    ULong               sCount = 0;

    idBool              sFetchResult = ID_FALSE;

    idBool              sExistReverseRP = ID_FALSE;

    idBool              sReverseRPInfo[SDI_NODE_MAX_COUNT] = { ID_FALSE, };

    UInt                sAddCnt[SDI_NODE_MAX_COUNT] = {0, };

    sParseTree = (qdShardParseTree *)aStatement->myPlan->parseTree;

    idlOS::snprintf(sSyncReplicationName,
                    SDI_REPLICATION_NAME_MAX_SIZE + 1,
                    "SYNC_%s_%s",                   /* nodename size = 10 *2 , + _ + _ + SYNC => 26 */
                    aFromNodeName,
                    aToNodeName);

    /* Find Reverse RP */
    for ( i = 0; i < aFailoverHistoryInfo->mCount; i++ )
    {
        sReplicaSet = &aFailoverHistoryInfo->mReplicaSets[i];

        if ( idlOS::strncmp( sReplicaSet->mFirstBackupNodeName,
                             aFromNodeName,
                             SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
        {
            idlOS::snprintf( sReverseReplName, 
                             SDI_REPLICATION_NAME_MAX_SIZE + 1,
                             "R_%s", 
                             sReplicaSet->mFirstReplName );
        }
        else if ( idlOS::strncmp( sReplicaSet->mSecondBackupNodeName,
                                  aFromNodeName,
                                  SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
        {
            idlOS::snprintf( sReverseReplName, 
                             SDI_REPLICATION_NAME_MAX_SIZE + 1,
                             "R_%s", 
                             sReplicaSet->mSecondReplName );
        }
        else
        {
            idlOS::snprintf( sReverseReplName,
                             SDI_REPLICATION_NAME_MAX_SIZE + 1,
                             SDM_NA_STR );
        }

        if ( idlOS::strncmp( sReverseReplName,
                             SDM_NA_STR,
                             SDI_REPLICATION_NAME_MAX_SIZE + 1 ) != 0 )
        {
            /* DROP FORCE의 추가로 ReverseRP가 존재한다고 확신할수 없다.
             * 확인 해 보고 존재하면 ReverseRP를 처리한다. */
            idlOS::snprintf( sSqlStr, 
                             QD_MAX_SQL_LENGTH + 1,
                             "select Count(*) from system_.sys_replications_ where replication_name='%s' ",
                             sReverseReplName );

            IDE_TEST( sdi::shardExecDirect( aStatement,
                                            aFromNodeName,
                                            (SChar*)sSqlStr,
                                            (UInt) idlOS::strlen(sSqlStr),
                                            SDI_INTERNAL_OP_FAILBACK,
                                            &sExecCount,
                                            &sCount,
                                            NULL,
                                            ID_SIZEOF(sCount),
                                            &sFetchResult )
                      != IDE_SUCCESS );

            if ( sCount != 0 )
            {
                sExistReverseRP = ID_TRUE;
                sReverseRPInfo[i] = ID_TRUE;
            }
            else
            {
                sReverseRPInfo[i] = ID_FALSE;
            }
        }
    }
   

    /* clear Local Data */
    for ( sTmpTableInfoList = aTableInfoList;
          sTmpTableInfoList != NULL;
          sTmpTableInfoList = sTmpTableInfoList->mNext )

    {
        sTableInfo = sTmpTableInfoList->mTableInfo;

        if ( sTableInfo->mObjectType == 'T' )
        {
            /* check Table Type */
            sShardTableType = sdi::getShardObjectType( sTableInfo );

            if ( sShardTableType == SDI_CLONE_DIST_OBJECT )
            {
                /* ReverseRP가 없을 때에만 Sync를 수행한다.
                 * Clone Fullsync의 경우에는 항상 ReverseRP가 없다. */
                if ( sExistReverseRP == ID_FALSE )
                {
                    /* Failover 수행시 Clone은 Unset 되었기 때문에 기존 Shard Meta랑 비교해볼수가 없다.
                     * 항상 Clone Sync 해야 한다. */
                    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                     "TRUNCATE TABLE "
                                     QCM_SQL_STRING_SKIP_FMT"."
                                     QCM_SQL_STRING_SKIP_FMT"",
                                     sTableInfo->mUserName,
                                     sTableInfo->mObjectName );

                    IDE_TEST( sdi::shardExecTempDDLWithNewTrans( aStatement,
                                                                 aToNodeName,/* LocalNodeName */
                                                                 sSqlStr,
                                                                 idlOS::strlen(sSqlStr)) 
                              != IDE_SUCCESS );
                }
            }
            else
            {
                if ( aIsCloneOnly == ID_TRUE )
                {
                    continue;
                }

                for ( i = 0; i < aFailoverHistoryInfo->mCount; i++ )
                {
                    /* ReverseRP가 없을때만 Sync를 수행한다. */
                    if ( sReverseRPInfo[i] == ID_FALSE )
                    {
                        sFailoverHistory = &aFailoverHistoryInfo->mReplicaSets[i];

                        /* Default Partition check */
                        if ( sTableInfo->mDefaultPartitionReplicaSetId == sFailoverHistory->mReplicaSetId )
                        {
                            /* FailoverHistory의 Primary랑 비교하면 안됨,
                             * 현재 ReplicaSet의 Primary랑 비교해야됨. */
                            if ( sdm::checkFailbackAvailable( aReplicaSetInfo,
                                                              sTableInfo->mDefaultPartitionReplicaSetId,
                                                              sdiZookeeper::mMyNodeName ) == ID_TRUE )
                            {
                                SDI_INIT_NODE(&sNode);
                                IDE_TEST( sdm::getNodeByID( QC_SMI_STMT( aStatement ),
                                                            sTableInfo->mDefaultNodeId,
                                                            aSMN,
                                                            &sNode )
                                          != IDE_SUCCESS );

                                if ( idlOS::strncmp( aFromNodeName,
                                                     sNode.mNodeName,
                                                     SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
                                {
                                    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                                     "ALTER TABLE "
                                                     QCM_SQL_STRING_SKIP_FMT"."
                                                     QCM_SQL_STRING_SKIP_FMT" TRUNCATE PARTITION "
                                                     QCM_SQL_STRING_SKIP_FMT"",
                                                     sTableInfo->mUserName,
                                                     sTableInfo->mObjectName,
                                                     sTableInfo->mDefaultPartitionName);

                                    IDE_TEST( sdi::shardExecTempDDLWithNewTrans( aStatement,
                                                                                 aToNodeName,/* LocalNodeName */
                                                                                 sSqlStr,
                                                                                 idlOS::strlen(sSqlStr)) 
                                              != IDE_SUCCESS );
                                }
                            }
                            else
                            {
                                /* ReplicaSet이 FailbackFromNode가 아니면 Failover시 DataFailover가 발생하지 않았다.
                                 * DataSync 대상이 아니다. */
                            }
                        }

                        /* Range Info Check */
                        IDE_TEST( sdm::getRangeInfo( aStatement,
                                                     QC_SMI_STMT( aStatement ),
                                                     aSMN,
                                                     sTableInfo,
                                                     &sRangeInfos,
                                                     ID_FALSE )
                                  != IDE_SUCCESS );

                        /* check Table Type */
                        sShardTableType = sdi::getShardObjectType( sTableInfo );

                        for ( j = 0; j < sRangeInfos.mCount; j++ )
                        {
                            if ( sRangeInfos.mRanges[j].mReplicaSetId == sFailoverHistory->mReplicaSetId )
                            {
                                if ( sdm::checkFailbackAvailable( aReplicaSetInfo,
                                                                  sRangeInfos.mRanges[j].mReplicaSetId,
                                                                  sdiZookeeper::mMyNodeName ) == ID_TRUE )
                                {
                                    SDI_INIT_NODE(&sNode);
                                    IDE_TEST( sdm::getNodeByID( QC_SMI_STMT( aStatement ),
                                                                sRangeInfos.mRanges[j].mNodeId,
                                                                aSMN,
                                                                &sNode )
                                              != IDE_SUCCESS );

                                    if ( idlOS::strncmp( aFromNodeName,
                                                         sNode.mNodeName,
                                                         SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
                                    {
                                        switch ( sShardTableType )
                                        {
                                            case SDI_SINGLE_SHARD_KEY_DIST_OBJECT:
                                                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                                                 "ALTER TABLE "
                                                                 QCM_SQL_STRING_SKIP_FMT"."
                                                                 QCM_SQL_STRING_SKIP_FMT" TRUNCATE PARTITION "
                                                                 QCM_SQL_STRING_SKIP_FMT"",
                                                                 sTableInfo->mUserName,
                                                                 sTableInfo->mObjectName,
                                                                 sRangeInfos.mRanges[j].mPartitionName );
                                                break;
                                            case SDI_COMPOSITE_SHARD_KEY_DIST_OBJECT:
                                                /* Composite는 HA를 제공하지 않는다 */
                                                continue;

                                            case SDI_SOLO_DIST_OBJECT:
                                                idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                                                 "TRUNCATE TABLE "
                                                                 QCM_SQL_STRING_SKIP_FMT"."
                                                                 QCM_SQL_STRING_SKIP_FMT"",
                                                                 sTableInfo->mUserName,
                                                                 sTableInfo->mObjectName );

                                                break;
                                            case SDI_CLONE_DIST_OBJECT:
                                                /* Clone은 위에서 따로 처리하였다. */
                                            case SDI_NON_SHARD_OBJECT:
                                            default:
                                                /* Non Shard Table이 오면 안됨. */
                                                IDE_DASSERT( 0 );
                                                break;
                                        }

                                        IDE_TEST( sdi::shardExecTempDDLWithNewTrans( aStatement,
                                                                                     aToNodeName,/* LocalNodeName */
                                                                                     sSqlStr,
                                                                                     idlOS::strlen(sSqlStr)) 
                                                  != IDE_SUCCESS );
                                    }
                                }
                                else
                                {
                                    /* Failback 대상이 아니면 아무것도 안한다. */
                                }
                            }
                        }
                    }
                }
            }
        }
        else
        {
            /* Procedure는 Truncate하지 않아도 된다. */
        }
    }

    /* create RP + check RP Already Created */
    IDE_TEST( createReplicationsWithNewTrans( aStatement,
                                              sParseTree->mNodeInfoList,
                                              aFromNodeName,
                                              aToNodeName,
                                              sSyncReplicationName ) != IDE_SUCCESS );

    /* AddItem */
    for ( sTmpTableInfoList = aTableInfoList;
          sTmpTableInfoList != NULL;
          sTmpTableInfoList = sTmpTableInfoList->mNext )

    {
        sTableInfo = sTmpTableInfoList->mTableInfo;

        if ( sTableInfo->mObjectType == 'T' )
        {
            /* check Table Type */
            sShardTableType = sdi::getShardObjectType( sTableInfo );

            if ( sShardTableType == SDI_CLONE_DIST_OBJECT )
            {
                /* ReverseRp가 없을때만 Sync 한다. */
                if ( sExistReverseRP == ID_FALSE )
                {
                    /* Failover 수행시 Clone은 Unset 되었기 때문에 기존 Shard Meta랑 비교해볼수가 없다.
                     * 항상 Clone Sync 해야 한다. */
                    idlOS::snprintf( sSqlStr, 
                                     QD_MAX_SQL_LENGTH,
                                     "alter replication "
                                     QCM_SQL_STRING_SKIP_FMT
                                     " add table from "
                                     QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT
                                     " to "
                                     QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT,
                                     sSyncReplicationName,             
                                     sTableInfo->mUserName,            
                                     sTableInfo->mObjectName,          
                                     sTableInfo->mUserName,            
                                     sTableInfo->mObjectName );

                    IDE_TEST( sdi::shardExecTempDDLWithNewTrans( aStatement,
                                                                 aToNodeName,/* LocalNodeName */
                                                                 sSqlStr,
                                                                 idlOS::strlen(sSqlStr)) 
                              != IDE_SUCCESS );

                    idlOS::snprintf( sSqlStr, 
                                     QD_MAX_SQL_LENGTH,
                                     "alter replication "
                                     QCM_SQL_STRING_SKIP_FMT
                                     " add table from "
                                     QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT
                                     " to "
                                     QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT,
                                     sSyncReplicationName,             
                                     sTableInfo->mUserName,            
                                     sTableInfo->mObjectName,          
                                     sTableInfo->mUserName,            
                                     sTableInfo->mObjectName );

                    IDE_TEST( sdi::shardExecTempDDLWithNewTrans( aStatement,
                                                                 aFromNodeName,
                                                                 sSqlStr,
                                                                 idlOS::strlen(sSqlStr)) 
                              != IDE_SUCCESS );
                }
            }
            else
            {
                if ( aIsCloneOnly == ID_TRUE )
                {

                    continue;
                }

                for ( i = 0; i < aFailoverHistoryInfo->mCount; i++ )
                {
                    /* ReverseRP가 없을때만 Sync 한다. */
                    if ( sReverseRPInfo[i] == ID_FALSE )
                    {
                        sFailoverHistory = &aFailoverHistoryInfo->mReplicaSets[i];

                        /* Default Partition check */
                        if ( sTableInfo->mDefaultPartitionReplicaSetId == sFailoverHistory->mReplicaSetId )
                        {
                            /* FailoverHistory의 Primary랑 비교하면 안됨,
                             * 현재 ReplicaSet의 Primary랑 비교해야됨. */
                            if ( sdm::checkFailbackAvailable( aReplicaSetInfo,
                                                              sTableInfo->mDefaultPartitionReplicaSetId,
                                                              sdiZookeeper::mMyNodeName ) == ID_TRUE )
                            {
                                SDI_INIT_NODE(&sNode);
                                IDE_TEST( sdm::getNodeByID( QC_SMI_STMT( aStatement ),
                                                            sTableInfo->mDefaultNodeId,
                                                            aSMN,
                                                            &sNode )
                                          != IDE_SUCCESS );

                                if ( idlOS::strncmp( aFromNodeName,
                                                     sNode.mNodeName,
                                                     SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
                                {
                                    sAddCnt[i]++;

                                    idlOS::snprintf( sSqlStr, 
                                                     QD_MAX_SQL_LENGTH,
                                                     "alter replication "
                                                     QCM_SQL_STRING_SKIP_FMT
                                                     " add table from "
                                                     QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT
                                                     " partition "QCM_SQL_STRING_SKIP_FMT" to "
                                                     QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT
                                                     " partition "QCM_SQL_STRING_SKIP_FMT"",
                                                     sSyncReplicationName,                   /* REPLICATION_NAME  */
                                                     sTableInfo->mUserName,                  /* Src UserName      */
                                                     sTableInfo->mObjectName,                /* Src TableName     */
                                                     sTableInfo->mDefaultPartitionName,      /* Src PartitionName */
                                                     sTableInfo->mUserName,                  /* Dst UserName      */
                                                     sTableInfo->mObjectName,                /* Dst TableName     */
                                                     sTableInfo->mDefaultPartitionName       /* Dst PartitionName */ );

                                    IDE_TEST( sdi::shardExecTempDDLWithNewTrans( aStatement,
                                                                                 aToNodeName,/* LocalNodeName */
                                                                                 sSqlStr,
                                                                                 idlOS::strlen(sSqlStr)) 
                                              != IDE_SUCCESS );

                                    idlOS::snprintf( sSqlStr, 
                                                     QD_MAX_SQL_LENGTH,
                                                     "alter replication "
                                                     QCM_SQL_STRING_SKIP_FMT
                                                     " add table from "
                                                     QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT
                                                     " partition "QCM_SQL_STRING_SKIP_FMT" to "
                                                     QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT
                                                     " partition "QCM_SQL_STRING_SKIP_FMT"",
                                                     sSyncReplicationName,                   /* REPLICATION_NAME  */
                                                     sTableInfo->mUserName,                  /* Src UserName      */
                                                     sTableInfo->mObjectName,                /* Src TableName     */
                                                     sTableInfo->mDefaultPartitionName,      /* Src PartitionName */
                                                     sTableInfo->mUserName,                  /* Dst UserName      */
                                                     sTableInfo->mObjectName,                /* Dst TableName     */
                                                     sTableInfo->mDefaultPartitionName       /* Dst PartitionName */ );


                                    IDE_TEST( sdi::shardExecTempDDLWithNewTrans( aStatement,
                                                                                 aFromNodeName,
                                                                                 sSqlStr,
                                                                                 idlOS::strlen(sSqlStr)) 
                                              != IDE_SUCCESS );

                                }


                            }
                            else
                            {
                                /* ReplicaSet이 FailbackFromNode가 아니면 Failover시 DataFailover가 발생하지 않았다.
                                 * DataSync 대상이 아니다. */
                            }
                        }

                        /* Range Info Check */
                        IDE_TEST( sdm::getRangeInfo( aStatement,
                                                     QC_SMI_STMT( aStatement ),
                                                     aSMN,
                                                     sTableInfo,
                                                     &sRangeInfos,
                                                     ID_FALSE )
                                  != IDE_SUCCESS );

                        /* check Table Type */
                        sShardTableType = sdi::getShardObjectType( sTableInfo );

                        for ( j = 0; j < sRangeInfos.mCount; j++ )
                        {
                            if ( sRangeInfos.mRanges[j].mReplicaSetId == sFailoverHistory->mReplicaSetId )
                            {
                                if ( sdm::checkFailbackAvailable( aReplicaSetInfo,
                                                                  sRangeInfos.mRanges[j].mReplicaSetId,
                                                                  sdiZookeeper::mMyNodeName ) == ID_TRUE )
                                {
                                    SDI_INIT_NODE(&sNode);
                                    IDE_TEST( sdm::getNodeByID( QC_SMI_STMT( aStatement ),
                                                                sRangeInfos.mRanges[j].mNodeId,
                                                                aSMN,
                                                                &sNode )
                                              != IDE_SUCCESS );

                                    if ( idlOS::strncmp( aFromNodeName,
                                                         sNode.mNodeName,
                                                         SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 )
                                    {

                                        switch ( sShardTableType )
                                        {
                                            case SDI_SINGLE_SHARD_KEY_DIST_OBJECT:
                                                sAddCnt[i]++;

                                                idlOS::snprintf( sSqlStr, 
                                                                 QD_MAX_SQL_LENGTH,
                                                                 "alter replication "
                                                                 QCM_SQL_STRING_SKIP_FMT
                                                                 " add table from "
                                                                 QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT
                                                                 " partition "QCM_SQL_STRING_SKIP_FMT" to "
                                                                 QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT
                                                                 " partition "QCM_SQL_STRING_SKIP_FMT"",
                                                                 sSyncReplicationName,             
                                                                 sTableInfo->mUserName,            
                                                                 sTableInfo->mObjectName,          
                                                                 sRangeInfos.mRanges[j].mPartitionName,
                                                                 sTableInfo->mUserName,            
                                                                 sTableInfo->mObjectName,          
                                                                 sRangeInfos.mRanges[j].mPartitionName );

                                                IDE_TEST( sdi::shardExecTempDDLWithNewTrans( aStatement,
                                                                                             aToNodeName,/* LocalNodeName */
                                                                                             sSqlStr,
                                                                                             idlOS::strlen(sSqlStr)) 
                                                          != IDE_SUCCESS );

                                                idlOS::snprintf( sSqlStr, 
                                                                 QD_MAX_SQL_LENGTH,
                                                                 "alter replication "
                                                                 QCM_SQL_STRING_SKIP_FMT
                                                                 " add table from "
                                                                 QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT
                                                                 " partition "QCM_SQL_STRING_SKIP_FMT" to "
                                                                 QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT
                                                                 " partition "QCM_SQL_STRING_SKIP_FMT"",
                                                                 sSyncReplicationName,             
                                                                 sTableInfo->mUserName,            
                                                                 sTableInfo->mObjectName,          
                                                                 sRangeInfos.mRanges[j].mPartitionName,
                                                                 sTableInfo->mUserName,            
                                                                 sTableInfo->mObjectName,          
                                                                 sRangeInfos.mRanges[j].mPartitionName );

                                                IDE_TEST( sdi::shardExecTempDDLWithNewTrans( aStatement,
                                                                                             aFromNodeName,
                                                                                             sSqlStr,
                                                                                             idlOS::strlen(sSqlStr)) 
                                                          != IDE_SUCCESS );

                                                break;
                                            case SDI_COMPOSITE_SHARD_KEY_DIST_OBJECT:
                                                /* Composite는 HA를 제공하지 않는다 */
                                                continue;

                                            case SDI_SOLO_DIST_OBJECT:
                                                sAddCnt[i]++;

                                                idlOS::snprintf( sSqlStr, 
                                                                 QD_MAX_SQL_LENGTH,
                                                                 "alter replication "
                                                                 QCM_SQL_STRING_SKIP_FMT
                                                                 " add table from "
                                                                 QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT
                                                                 " to "
                                                                 QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT,
                                                                 sSyncReplicationName,             
                                                                 sTableInfo->mUserName,            
                                                                 sTableInfo->mObjectName,          
                                                                 sTableInfo->mUserName,            
                                                                 sTableInfo->mObjectName );

                                                IDE_TEST( sdi::shardExecTempDDLWithNewTrans( aStatement,
                                                                                             aToNodeName,/* LocalNodeName */
                                                                                             sSqlStr,
                                                                                             idlOS::strlen(sSqlStr)) 
                                                          != IDE_SUCCESS );

                                                idlOS::snprintf( sSqlStr, 
                                                                 QD_MAX_SQL_LENGTH,
                                                                 "alter replication "
                                                                 QCM_SQL_STRING_SKIP_FMT
                                                                 " add table from "
                                                                 QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT
                                                                 " to "
                                                                 QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT,
                                                                 sSyncReplicationName,             
                                                                 sTableInfo->mUserName,            
                                                                 sTableInfo->mObjectName,          
                                                                 sTableInfo->mUserName,            
                                                                 sTableInfo->mObjectName );

                                                IDE_TEST( sdi::shardExecTempDDLWithNewTrans( aStatement,
                                                                                             aFromNodeName,
                                                                                             sSqlStr,
                                                                                             idlOS::strlen(sSqlStr)) 
                                                          != IDE_SUCCESS );
                                                break;

                                            case SDI_CLONE_DIST_OBJECT:
                                                /* Clone은 따로 처리 하였다. */
                                            case SDI_NON_SHARD_OBJECT:
                                            default:
                                                /* Non Shard Table이 오면 안됨. */
                                                IDE_DASSERT( 0 );
                                                break;
                                        }

                                    }
                                }
                                else
                                {
                                    /* Failback 대상이 아니면 아무것도 안한다. */
                                }
                            }
                        }
                    }
                }
            }
        }
        else
        {
            /* Procedure */
        }
    }

    /* Pending Add */
    /* For Rollback */
    idlOS::snprintf(sSqlStr,
                    QD_MAX_SQL_LENGTH,
                    "alter replication "QCM_SQL_STRING_SKIP_FMT" stop",
                    sSyncReplicationName);
    (void)sdiZookeeper::addPendingJob(sSqlStr,
                                      aFromNodeName,
                                      ZK_PENDING_JOB_AFTER_ROLLBACK,
                                      QCI_STMT_MASK_MAX );
    idlOS::snprintf(sSqlStr,
                    QD_MAX_SQL_LENGTH,
                    "drop replication "QCM_SQL_STRING_SKIP_FMT"",
                    sSyncReplicationName);
    (void)sdiZookeeper::addPendingJob(sSqlStr,
                                      aFromNodeName,
                                      ZK_PENDING_JOB_AFTER_ROLLBACK,
                                      QCI_STMT_MASK_MAX );
    (void)sdiZookeeper::addPendingJob( sSqlStr,
                                       aToNodeName,
                                       ZK_PENDING_JOB_AFTER_ROLLBACK,
                                       QCI_STMT_MASK_MAX );

    /* For Commit */
    idlOS::snprintf(sSqlStr,
                    QD_MAX_SQL_LENGTH,
                    "alter replication "QCM_SQL_STRING_SKIP_FMT" stop",
                    sSyncReplicationName);
    (void)sdiZookeeper::addPendingJob(sSqlStr,
                                      aFromNodeName,
                                      ZK_PENDING_JOB_AFTER_COMMIT,
                                      QCI_STMT_MASK_MAX );

    idlOS::snprintf(sSqlStr,
                    QD_MAX_SQL_LENGTH,
                    "drop replication "QCM_SQL_STRING_SKIP_FMT"",
                    sSyncReplicationName);

    (void)sdiZookeeper::addPendingJob(sSqlStr,
                                      aFromNodeName,
                                      ZK_PENDING_JOB_AFTER_COMMIT,
                                      QCI_STMT_MASK_MAX );
    (void)sdiZookeeper::addPendingJob( sSqlStr,
                                       aToNodeName,
                                       ZK_PENDING_JOB_AFTER_COMMIT,
                                       QCI_STMT_MASK_MAX );


    /* Sync Data */
    IDE_TEST( executeRemoteSQLWithNewTransArg( aStatement,
                                               aFromNodeName,
                                               SDI_INTERNAL_OP_FAILBACK,
                                               QCI_STMT_MASK_DCL,
                                               sSqlStr,
                                               QD_MAX_SQL_LENGTH,
                                               ID_FALSE,
                                               "alter replication "QCM_SQL_STRING_SKIP_FMT" sync",
                                               sSyncReplicationName) != IDE_SUCCESS );


    /* Backup Sync - ReverseRP가 없고 DataSync가 없었다면
     * 내 Data로 Service 해야 하는데 Backup이 최신일수 있다.
     * 기존 Backup을 Truncate 하고 내 Data를 Backup으로 Sync 해야 한다. */
    if ( aIsCloneOnly == ID_FALSE )
    {
        for ( i = 0; i < aFailoverHistoryInfo->mCount; i++ )
        {
            /* Sync 받아온게 없을때만 Backup Sync 한다. */
            /* ReverseRP가 없을때만 Sync를 수행한다. */
            if ( ( sReverseRPInfo[i] == ID_FALSE ) && ( sAddCnt[i] == 0 ) )
            {
                IDE_TEST( syncDataWithReplicaSet( aStatement,
                                                  sdiZookeeper::mMyNodeName,
                                                  aTableInfoList,
                                                  &aFailoverHistoryInfo->mReplicaSets[i],
                                                  aSMN ) != IDE_SUCCESS );
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdsd::syncFailbackDataAfterLock( qcStatement        * aStatement,
                                        SChar              * aFromNodeName,
                                        SChar              * aToNodeName )
{
    SChar               sSqlStr[QD_MAX_SQL_LENGTH + 1];
    SChar               sSyncReplicationName[SDI_REPLICATION_NAME_MAX_SIZE + 1] = { 0, };

    idlOS::snprintf(sSyncReplicationName,
                    SDI_REPLICATION_NAME_MAX_SIZE + 1,
                    "SYNC_%s_%s",                   /* nodename size = 10 *2 , + _ + _ + SYNC => 26 */
                    aFromNodeName,
                    aToNodeName);

    /* After Remote Lock */
    /* Flush Logs */
    IDE_TEST( executeRemoteSQLWithNewTransArg( aStatement,
                                aFromNodeName,
                                SDI_INTERNAL_OP_FAILBACK,
                                QCI_STMT_MASK_DCL,
                                sSqlStr,
                                QD_MAX_SQL_LENGTH,
                                ID_FALSE,
                                "alter replication "QCM_SQL_STRING_SKIP_FMT" flush ",
                                sSyncReplicationName ) != IDE_SUCCESS );

    /* Remove RP */
    IDE_TEST( executeRemoteSQLWithNewTransArg( aStatement,
                                aFromNodeName,
                                SDI_INTERNAL_OP_FAILBACK,
                                QCI_STMT_MASK_DCL,
                                sSqlStr,
                                QD_MAX_SQL_LENGTH,
                                ID_FALSE,
                                "alter replication "QCM_SQL_STRING_SKIP_FMT" stop",
                                sSyncReplicationName) != IDE_SUCCESS );

/*    IDE_TEST( executeRemoteSQL( aStatement,
                                aFromNodeName,
                                SDI_INTERNAL_OP_FAILBACK,
                                QCI_STMT_MASK_DDL,
                                sSqlStr,
                                QD_MAX_SQL_LENGTH,
                                "drop replication "QCM_SQL_STRING_SKIP_FMT"",
                                sSyncReplicationName) != IDE_SUCCESS );
    IDE_TEST( executeRemoteSQL( aStatement,
                                aToNodeName,
                                SDI_INTERNAL_OP_FAILBACK,
                                QCI_STMT_MASK_DDL,
                                sSqlStr,
                                QD_MAX_SQL_LENGTH,
                                "drop replication "QCM_SQL_STRING_SKIP_FMT"",
                                sSyncReplicationName) != IDE_SUCCESS );*/

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qdsd::syncDataWithReplicaSet( qcStatement        * aStatement,
                                     SChar              * aFromNodeName,
                                     sdiTableInfoList   * aTableInfoList,
                                     sdiReplicaSet      * aReplicaSet,
                                     ULong                aSMN )
{
    sdiTableInfoList  * sTmpTableInfoList = NULL;    

    sdiTableInfo      * sTableInfo  = NULL;
    sdiRangeInfo        sRangeInfos;

    qdShardParseTree  * sParseTree = NULL;

    sdiShardObjectType  sShardTableType = SDI_NON_SHARD_OBJECT;

    UInt                j;
    UInt                k;

    SChar               sSqlStr[QD_MAX_SQL_LENGTH + 1];

    SChar               sSyncReplicationName[SDI_REPLICATION_NAME_MAX_SIZE + 1] = { 0, };

    SChar               sSyncToNodeName[SDI_NODE_NAME_MAX_SIZE + 1 ];

    sParseTree = (qdShardParseTree *)aStatement->myPlan->parseTree;

    for( k = 0; k < SDI_KSAFETY_MAX ; k++ )
    {
        if ( ( k == 0 ) &&
             ( idlOS::strncmp( aReplicaSet->mFirstBackupNodeName,
                               SDM_NA_STR,
                               SDI_NODE_NAME_MAX_SIZE + 1 ) != 0 ) )
        {
            idlOS::snprintf(sSyncReplicationName,
                            SDI_REPLICATION_NAME_MAX_SIZE + 1,
                            "SYNC_%s_%s",  /* nodename size = 10 *2 , + _ + _ + SYNC => 26 */
                            aFromNodeName,
                            aReplicaSet->mFirstBackupNodeName );

            idlOS::strncpy( sSyncToNodeName,
                            aReplicaSet->mFirstBackupNodeName,
                            SDI_NODE_NAME_MAX_SIZE + 1 );
        }
        else if ( ( k == 1 ) &&
             ( idlOS::strncmp( aReplicaSet->mSecondBackupNodeName,
                               SDM_NA_STR,
                               SDI_NODE_NAME_MAX_SIZE + 1 ) != 0 ) )
        {
            idlOS::snprintf(sSyncReplicationName,
                            SDI_REPLICATION_NAME_MAX_SIZE + 1,
                            "SYNC_%s_%s",  /* nodename size = 10 *2 , + _ + _ + SYNC => 26 */
                            aFromNodeName,
                            aReplicaSet->mSecondBackupNodeName );

            idlOS::strncpy( sSyncToNodeName,
                            aReplicaSet->mSecondBackupNodeName,
                            SDI_NODE_NAME_MAX_SIZE + 1 );
        }
        else
        {
            continue;
        }

        IDE_TEST( createReplicationsWithNewTrans( aStatement,
                                                  sParseTree->mNodeInfoList,
                                                  sdiZookeeper::mMyNodeName,
                                                  sSyncToNodeName,
                                                  sSyncReplicationName ) != IDE_SUCCESS );

        for ( sTmpTableInfoList = aTableInfoList;
              sTmpTableInfoList != NULL;
              sTmpTableInfoList = sTmpTableInfoList->mNext )

        {
            sTableInfo = sTmpTableInfoList->mTableInfo;

            if ( sTableInfo->mObjectType == 'T' )
            {
                /* check Table Type */
                sShardTableType = sdi::getShardObjectType( sTableInfo );

                if ( sShardTableType == SDI_CLONE_DIST_OBJECT )
                {
                    /* Clone은 Backup Sync 안해도 된다. */
                }
                else
                {
                    /* Default Partition check */
                    if ( sTableInfo->mDefaultPartitionReplicaSetId == aReplicaSet->mReplicaSetId )
                    {
                        idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                         "ALTER TABLE "
                                         QCM_SQL_STRING_SKIP_FMT"."
                                         QCM_SQL_STRING_SKIP_FMT" TRUNCATE PARTITION "
                                         QCM_SQL_STRING_SKIP_FMT"",
                                         sTableInfo->mUserName,
                                         sTableInfo->mObjectName,
                                         sTableInfo->mDefaultPartitionName);

                        IDE_TEST( sdi::shardExecTempDDLWithNewTrans( aStatement,
                                                                     sSyncToNodeName,
                                                                     sSqlStr,
                                                                     idlOS::strlen(sSqlStr)) 
                                  != IDE_SUCCESS );

                        idlOS::snprintf( sSqlStr, 
                                         QD_MAX_SQL_LENGTH,
                                         "alter replication "
                                         QCM_SQL_STRING_SKIP_FMT
                                         " add table from "
                                         QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT
                                         " partition "QCM_SQL_STRING_SKIP_FMT" to "
                                         QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT
                                         " partition "QCM_SQL_STRING_SKIP_FMT"",
                                         sSyncReplicationName,                   /* REPLICATION_NAME  */
                                         sTableInfo->mUserName,                  /* Src UserName      */
                                         sTableInfo->mObjectName,                /* Src TableName     */
                                         sTableInfo->mDefaultPartitionName,      /* Src PartitionName */
                                         sTableInfo->mUserName,                  /* Dst UserName      */
                                         sTableInfo->mObjectName,                /* Dst TableName     */
                                         sTableInfo->mDefaultPartitionName       /* Dst PartitionName */ );

                        IDE_TEST( sdi::shardExecTempDDLWithNewTrans( aStatement,
                                                                     sdiZookeeper::mMyNodeName,
                                                                     sSqlStr,
                                                                     idlOS::strlen(sSqlStr)) 
                                  != IDE_SUCCESS );

                        IDE_TEST( sdi::shardExecTempDDLWithNewTrans( aStatement,
                                                                     sSyncToNodeName,
                                                                     sSqlStr,
                                                                     idlOS::strlen(sSqlStr)) 
                                  != IDE_SUCCESS );

                    }

                    /* Range Info Check */
                    IDE_TEST( sdm::getRangeInfo( aStatement,
                                                 QC_SMI_STMT( aStatement ),
                                                 aSMN,
                                                 sTableInfo,
                                                 &sRangeInfos,
                                                 ID_FALSE )
                              != IDE_SUCCESS );

                    /* check Table Type */
                    sShardTableType = sdi::getShardObjectType( sTableInfo );

                    for ( j = 0; j < sRangeInfos.mCount; j++ )
                    {
                        if ( sRangeInfos.mRanges[j].mReplicaSetId == aReplicaSet->mReplicaSetId )
                        {
                            switch ( sShardTableType )
                            {
                                case SDI_SINGLE_SHARD_KEY_DIST_OBJECT:
                                    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                                     "ALTER TABLE "
                                                     QCM_SQL_STRING_SKIP_FMT"."
                                                     QCM_SQL_STRING_SKIP_FMT" TRUNCATE PARTITION "
                                                     QCM_SQL_STRING_SKIP_FMT"",
                                                     sTableInfo->mUserName,
                                                     sTableInfo->mObjectName,
                                                     sRangeInfos.mRanges[j].mPartitionName );
                                    IDE_TEST( sdi::shardExecTempDDLWithNewTrans( aStatement,
                                                                                 sSyncToNodeName,
                                                                                 sSqlStr,
                                                                                 idlOS::strlen(sSqlStr)) 
                                              != IDE_SUCCESS );

                                    idlOS::snprintf( sSqlStr, 
                                                     QD_MAX_SQL_LENGTH,
                                                     "alter replication "
                                                     QCM_SQL_STRING_SKIP_FMT
                                                     " add table from "
                                                     QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT
                                                     " partition "QCM_SQL_STRING_SKIP_FMT" to "
                                                     QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT
                                                     " partition "QCM_SQL_STRING_SKIP_FMT"",
                                                     sSyncReplicationName,             
                                                     sTableInfo->mUserName,            
                                                     sTableInfo->mObjectName,          
                                                     sRangeInfos.mRanges[j].mPartitionName,
                                                     sTableInfo->mUserName,            
                                                     sTableInfo->mObjectName,          
                                                     sRangeInfos.mRanges[j].mPartitionName );

                                    IDE_TEST( sdi::shardExecTempDDLWithNewTrans( aStatement,
                                                                                 sdiZookeeper::mMyNodeName,
                                                                                 sSqlStr,
                                                                                 idlOS::strlen(sSqlStr)) 
                                              != IDE_SUCCESS );

                                    IDE_TEST( sdi::shardExecTempDDLWithNewTrans( aStatement,
                                                                                 sSyncToNodeName,
                                                                                 sSqlStr,
                                                                                 idlOS::strlen(sSqlStr)) 
                                              != IDE_SUCCESS );


                                    break;
                                case SDI_COMPOSITE_SHARD_KEY_DIST_OBJECT:
                                    /* Composite는 HA를 제공하지 않는다 */
                                    continue;

                                case SDI_SOLO_DIST_OBJECT:
                                    idlOS::snprintf( sSqlStr, QD_MAX_SQL_LENGTH,
                                                     "TRUNCATE TABLE "
                                                     QCM_SQL_STRING_SKIP_FMT"."
                                                     QCM_SQL_STRING_SKIP_FMT"",
                                                     sTableInfo->mUserName,
                                                     sTableInfo->mObjectName );
                                    IDE_TEST( sdi::shardExecTempDDLWithNewTrans( aStatement,
                                                                                 sSyncToNodeName,
                                                                                 sSqlStr,
                                                                                 idlOS::strlen(sSqlStr)) 
                                              != IDE_SUCCESS );

                                    idlOS::snprintf( sSqlStr, 
                                                     QD_MAX_SQL_LENGTH,
                                                     "alter replication "
                                                     QCM_SQL_STRING_SKIP_FMT
                                                     " add table from "
                                                     QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT
                                                     " to "
                                                     QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT,
                                                     sSyncReplicationName,             
                                                     sTableInfo->mUserName,            
                                                     sTableInfo->mObjectName,          
                                                     sTableInfo->mUserName,            
                                                     sTableInfo->mObjectName );

                                    IDE_TEST( sdi::shardExecTempDDLWithNewTrans( aStatement,
                                                                                 sdiZookeeper::mMyNodeName,
                                                                                 sSqlStr,
                                                                                 idlOS::strlen(sSqlStr)) 
                                              != IDE_SUCCESS );

                                    IDE_TEST( sdi::shardExecTempDDLWithNewTrans( aStatement,
                                                                                 sSyncToNodeName,
                                                                                 sSqlStr,
                                                                                 idlOS::strlen(sSqlStr)) 
                                              != IDE_SUCCESS );


                                    break;
                                case SDI_CLONE_DIST_OBJECT:
                                    /* Clone은 위에서 따로 처리하였다. */
                                case SDI_NON_SHARD_OBJECT:
                                default:
                                    /* Non Shard Table이 오면 안됨. */
                                    IDE_DASSERT( 0 );
                                    break;
                            }

                        }
                    }
                }
            }
            else
            {
                /* Procedure는 하지 않아도 된다. */
            }
        }

        /* Sync Replication */
        IDE_TEST( executeRemoteSQLWithNewTransArg( aStatement,
                                                   sdiZookeeper::mMyNodeName,
                                                   SDI_INTERNAL_OP_FAILBACK,
                                                   QCI_STMT_MASK_DCL,
                                                   sSqlStr,
                                                   QD_MAX_SQL_LENGTH,
                                                   ID_FALSE,
                                                   "alter replication "QCM_SQL_STRING_SKIP_FMT" sync ",
                                                   sSyncReplicationName) != IDE_SUCCESS );

        /* Stop & Drop Replication */
        IDE_TEST( executeRemoteSQLWithNewTransArg( aStatement,
                                    sdiZookeeper::mMyNodeName,
                                    SDI_INTERNAL_OP_FAILBACK,
                                    QCI_STMT_MASK_DCL,
                                    sSqlStr,
                                    QD_MAX_SQL_LENGTH,
                                    ID_FALSE,
                                    "alter replication "QCM_SQL_STRING_SKIP_FMT" stop",
                                    sSyncReplicationName) != IDE_SUCCESS );

        IDE_TEST( executeRemoteSQLWithNewTransArg( aStatement,
                                    sdiZookeeper::mMyNodeName,
                                    SDI_INTERNAL_OP_FAILBACK,
                                    QCI_STMT_MASK_DCL,
                                    sSqlStr,
                                    QD_MAX_SQL_LENGTH,
                                    ID_FALSE,
                                    "drop replication "QCM_SQL_STRING_SKIP_FMT,
                                    sSyncReplicationName) != IDE_SUCCESS );

        IDE_TEST( executeRemoteSQLWithNewTransArg( aStatement,
                                    sSyncToNodeName,
                                    SDI_INTERNAL_OP_FAILBACK,
                                    QCI_STMT_MASK_DCL,
                                    sSqlStr,
                                    QD_MAX_SQL_LENGTH,
                                    ID_FALSE,
                                    "drop replication "QCM_SQL_STRING_SKIP_FMT,
                                    sSyncReplicationName) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

