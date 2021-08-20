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
 * $Id: sduProperty.cpp 91426 2021-08-04 04:40:23Z jake.jang $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <sduProperty.h>

sduProperties sduProperty::mStaticProperty;

IDE_RC sduProperty::initProperty( idvSQL * /* aStatistics */ )
{
    initUpperModuleProperty();
    IDE_TEST( load() != IDE_SUCCESS );
    IDE_TEST( setupUpdateCallback() != IDE_SUCCESS );

    // BUG-47817
    if (( SDU_SHARD_ENABLE == 1 ) && ( SDU_SHARD_ADMIN_MODE == 0 ))
    {
        IDE_TEST( idp::update( NULL, "SHARD_ADMIN_MODE", (ULong)1, 0, NULL )
                  != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sduProperty::finalProperty( idvSQL * /* aStatistics */ )
{
    return IDE_SUCCESS;
}

void sduProperty::setReplicationMaxParallelCount(UInt aMaxParallelCount)
{
    SDU_PROPERTY( mShardReplicationMaxParallelCount ) = aMaxParallelCount;
}

void sduProperty::initUpperModuleProperty()
{
    SDU_PROPERTY( mShardReplicationMaxParallelCount ) = 0;
}

IDE_RC sduProperty::load()
{
/***********************************************************************
 *
 * Description :
 *    Server 구동 시 System Property들을 Loading한다.
 *
 * Implementation :
 *    Writable Property의 경우 CallBack을 등록해야 함.
 *
 ***********************************************************************/

    /* BUG-45899 */ 
    IDE_ASSERT( idp::read( "TRCLOG_DETAIL_SHARD",
                           &SDU_PROPERTY( mTrclogDetailShard ) ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "SHARD_ENABLE",
                           &SDU_PROPERTY( mShardEnable ) ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "__SHARD_TEST_ENABLE",
                           &SDU_PROPERTY( mShardTestEnable ) ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "__SHARD_ALLOW_AUTO_COMMIT",
                           &SDU_PROPERTY( mShardAllowAutoCommit) ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "__SHARD_LOCAL_FORCE",
                           &SDU_PROPERTY( mShardLocalForce ) ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "SHARD_AGGREGATION_TRANSFORM_ENABLE",
                           &SDU_PROPERTY( mShardAggregationTransformEnable ) ) == IDE_SUCCESS );

    /* TASK-7219 */
    IDE_ASSERT( idp::read( "SHARD_TRANSFORM_MODE",
                           & SDU_PROPERTY( mShardTransformMode ) ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "SHARD_INTERNAL_CONN_ATTR_RETRY_COUNT",
                           &SDU_PROPERTY( mShardInternalConnAttrRetryCount ) ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "SHARD_INTERNAL_CONN_ATTR_RETRY_DELAY",
                           &SDU_PROPERTY( mShardInternalConnAttrRetryDelay ) ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "SHARD_INTERNAL_CONN_ATTR_CONNECTION_TIMEOUT",
                           &SDU_PROPERTY( mShardInternalConnAttrConnTimeout ) ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "SHARD_INTERNAL_CONN_ATTR_LOGIN_TIMEOUT",
                           &SDU_PROPERTY( mShardInternalConnAttrLoginTimeout ) ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "SHARD_REBUILD_DATA_STEP",
                           & SDU_PROPERTY( mShardRebuildDataStep ) )
                == IDE_SUCCESS );

    /* PROJ-2701 Sharding online data rebuild */
    IDE_ASSERT( idp::read( "SHARD_REBUILD_PLAN_DETAIL_FORCE_ENABLE",
                           & SDU_PROPERTY(mShardRebuildPlanDetailForceEnable) ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "SHARD_REBUILD_LOCK_TABLE_WITH_DML_ENABLE",
                           & SDU_PROPERTY(mShardRebuildLockTableWithDmlEnable) ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "SHARD_META_PROPAGATION_TIMEOUT",
                           & SDU_PROPERTY(mShardMetaPropagationTimeout) ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "SHARD_TRANSFORM_STRING_LENGTH_MAX",
                           & SDU_PROPERTY(mShardTransformStringLengthMax) ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "SHARD_ADMIN_MODE",
                           & SDU_PROPERTY(mShardAdminMode) ) == IDE_SUCCESS );    

    /* PROJ-2726 Sharding Node Manager */
    IDE_ASSERT( idp::read( "ZOOKEEPER_LOCK_WAIT_TIMEOUT",
                           & SDU_PROPERTY(mZookeeperLockWaitTimeout) ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "SHARD_STATEMENT_RETRY",
                           & SDU_PROPERTY(mShardStatementRetry) ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "__SHARD_ZOOKEEPER_TEST",
                           &SDU_PROPERTY( mShardZookeeperTest ) ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "SHARD_DDL_LOCK_TIMEOUT",
                           & SDU_PROPERTY(mShardDDLLockTimeout) ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "SHARD_DDL_LOCK_TRY_COUNT",
                           & SDU_PROPERTY(mShardDDLLockTryCount) ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "__DISABLE_FAILOVER_FOR_WATCHER",
                           & SDU_PROPERTY(mDisableFailoverForWatcher) ) == IDE_SUCCESS );

    IDE_ASSERT( idp::read( "SHARD_INTERNAL_REPLICATION_PARALLEL_COUNT",
                           & SDU_PROPERTY(mShardInternalReplicationParallelCount) ) == IDE_SUCCESS );
    
    return IDE_SUCCESS;
}

IDE_RC sduProperty::setupUpdateCallback()
{
/***********************************************************************
 *
 * Description :
 *    Writable Property의 경우 CallBack을 등록해야 함.
 *
 * Implementation :
 *
 ***********************************************************************/

    /* BUG-45899 */ 
    IDE_TEST( idp::setupAfterUpdateCallback( "TRCLOG_DETAIL_SHARD",
                                             sduProperty::changeTRCLOG_DETAIL_SHARD )
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback( "__SHARD_LOCAL_FORCE",
                                             sduProperty::changeSHARD_LOCAL_FORCE )
              != IDE_SUCCESS );

    /* PROJ-2687 Shard aggregation transform */ 
    IDE_TEST( idp::setupAfterUpdateCallback( "SHARD_AGGREGATION_TRANSFORM_ENABLE",
                                             sduProperty::changeSHARD_AGGREGATION_TRANSFORM_ENABLE )
              != IDE_SUCCESS );

    /* TASK-7219 */
    IDE_TEST( idp::setupAfterUpdateCallback( "SHARD_TRANSFORM_MODE",
                                             sduProperty::changeSHARD_TRANSFORM_MODE )
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback( "SHARD_INTERNAL_CONN_ATTR_RETRY_COUNT",
                                             sduProperty::changeSHARD_INTERNAL_CONN_ATTR_RETRY_COUNT )
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback( "SHARD_INTERNAL_CONN_ATTR_RETRY_DELAY",
                                             sduProperty::changeSHARD_INTERNAL_CONN_ATTR_RETRY_DELAY )
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback( "SHARD_INTERNAL_CONN_ATTR_CONNECTION_TIMEOUT",
                                             sduProperty::changeSHARD_INTERNAL_CONN_ATTR_CONN_TIMEOUT )
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback( "SHARD_INTERNAL_CONN_ATTR_LOGIN_TIMEOUT",
                                             sduProperty::changeSHARD_INTERNAL_CONN_ATTR_LOGIN_TIMEOUT )
              != IDE_SUCCESS );

    /* BUG-45967 Rebuild Data 완료 대기 */
    IDE_TEST( idp::setupAfterUpdateCallback( "SHARD_REBUILD_DATA_STEP",
                                             sduProperty::changeSHARD_REBUILD_DATA_STEP )
              != IDE_SUCCESS );

    /* PROJ-2701 Online data rebuild */
    IDE_TEST( idp::setupAfterUpdateCallback("SHARD_REBUILD_PLAN_DETAIL_FORCE_ENABLE",
                                            sduProperty::changeSHARD_REBUILD_PLAN_DETAIL_FORCE_ENABLE)
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback("SHARD_REBUILD_LOCK_TABLE_WITH_DML_ENABLE",
                                            sduProperty::changeSHARD_REBUILD_LOCK_TABLE_WITH_DML_ENABLE)
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback("SHARD_META_PROPAGATION_TIMEOUT",
                                            sduProperty::changeSHARD_META_PROPAGATION_TIMEOUT)
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback("SHARD_TRANSFORM_STRING_LENGTH_MAX",
                                            sduProperty::changeSHARD_TRANSFORM_STRING_LENGTH_MAX)
              != IDE_SUCCESS );

    // BUG-47817
    IDE_TEST( idp::setupAfterUpdateCallback("SHARD_ADMIN_MODE",
                                            sduProperty::changeSHARD_ADMIN_MODE)
              != IDE_SUCCESS );

    /* PROJ-2726 Sharding Node Manager */
    IDE_TEST( idp::setupAfterUpdateCallback("ZOOKEEPER_LOCK_WAIT_TIMEOUT",
                                            sduProperty::changeZOOKEEPER_LOCK_WAIT_TIMEOUT)
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback("SHARD_STATEMENT_RETRY",
                                            sduProperty::changeSHARD_STATEMENT_RETRY)
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback("SHARD_DDL_LOCK_TIMEOUT",
                                            sduProperty::changeSHARD_DDL_LOCK_TIMEOUT)
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback("SHARD_DDL_LOCK_TRY_COUNT",
                                            sduProperty::changeSHARD_DDL_LOCK_TRY_COUNT)
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback("__DISABLE_FAILOVER_FOR_WATCHER",
                                            sduProperty::changeDISABLE_FAILOVER_FOR_WATCHER)
              != IDE_SUCCESS );

    IDE_TEST( idp::setupAfterUpdateCallback("SHARD_INTERNAL_REPLICATION_PARALLEL_COUNT",
                                            sduProperty::changeSHARD_INTERNAL_REPLICATION_PARALLEL_COUNT)
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* BUG-45899 */
IDE_RC sduProperty::changeTRCLOG_DETAIL_SHARD( idvSQL * /* aStatistics */,
                                               SChar  * /* aName */,
                                               void   * /* aOldValue */,
                                               void   * aNewValue,
                                               void   * /* aArg */ )
{
    idlOS::memcpy( &SDU_PROPERTY( mTrclogDetailShard ),
                   aNewValue,
                   ID_SIZEOF( UInt ) );

    return IDE_SUCCESS;
}

/* PROJ-2687 Shard aggregation transform */
IDE_RC sduProperty::changeSHARD_LOCAL_FORCE( idvSQL * /* aStatistics */,
                                             SChar  * /* aName */,
                                             void   * /* aOldValue */,
                                             void   * aNewValue,
                                             void   * /* aArg */ )
{
    idlOS::memcpy( &SDU_PROPERTY( mShardLocalForce ),
                   aNewValue,
                   ID_SIZEOF( UInt ) );

    return IDE_SUCCESS;
}

/* PROJ-2687 Shard aggregation transform */
IDE_RC sduProperty::changeSHARD_AGGREGATION_TRANSFORM_ENABLE( idvSQL * /* aStatistics */,
                                                              SChar  * /* aName */,
                                                              void   * /* aOldValue */,
                                                              void   * aNewValue,
                                                              void   * /* aArg */ )
{
    idlOS::memcpy( &SDU_PROPERTY( mShardAggregationTransformEnable ),
                   aNewValue,
                   ID_SIZEOF( UInt ) );

    return IDE_SUCCESS;
}

/* TASK-7219 */
IDE_RC sduProperty::changeSHARD_TRANSFORM_MODE( idvSQL * /* aStatistics */,
                                                SChar  * /* aName */,
                                                void   * /* aOldValue */,
                                                void   * aNewValue,
                                                void   * /* aArg */ )
{
    idlOS::memcpy( &SDU_PROPERTY( mShardTransformMode ),
                   aNewValue,
                   ID_SIZEOF( UInt ) );

    return IDE_SUCCESS;
}

IDE_RC sduProperty::changeSHARD_INTERNAL_CONN_ATTR_RETRY_COUNT( idvSQL * /* aStatistics */,
                                                                SChar  * /* aName */,
                                                                void   * /* aOldValue */,
                                                                void   * aNewValue,
                                                                void   * /* aArg */ )
{
    idlOS::memcpy( &SDU_PROPERTY( mShardInternalConnAttrRetryCount ),
                   aNewValue,
                   ID_SIZEOF( UInt ) );

    return IDE_SUCCESS;
}

IDE_RC sduProperty::changeSHARD_INTERNAL_CONN_ATTR_RETRY_DELAY( idvSQL * /* aStatistics */,
                                                                SChar  * /* aName */,
                                                                void   * /* aOldValue */,
                                                                void   * aNewValue,
                                                                void   * /* aArg */ )
{
    idlOS::memcpy( &SDU_PROPERTY( mShardInternalConnAttrRetryDelay ),
                   aNewValue,
                   ID_SIZEOF( UInt ) );

    return IDE_SUCCESS;
}

IDE_RC sduProperty::changeSHARD_INTERNAL_CONN_ATTR_CONN_TIMEOUT( idvSQL* /* aStatistics */,
                                                                 SChar * /* aName */,
                                                                 void  * /* aOldValue */,
                                                                 void  * aNewValue,
                                                                 void  * /* aArg */)
{
    idlOS::memcpy( &SDU_PROPERTY( mShardInternalConnAttrConnTimeout ),
                   aNewValue,
                   ID_SIZEOF( UInt ) );

    return IDE_SUCCESS;
}

IDE_RC sduProperty::changeSHARD_INTERNAL_CONN_ATTR_LOGIN_TIMEOUT( idvSQL* /* aStatistics */,
                                                                  SChar * /* aName */,
                                                                  void  * /* aOldValue */,
                                                                  void  * aNewValue,
                                                                  void  * /* aArg */)
{
    idlOS::memcpy( &SDU_PROPERTY( mShardInternalConnAttrLoginTimeout ),
                   aNewValue,
                   ID_SIZEOF( UInt ) );

    return IDE_SUCCESS;
}

/* BUG-45967 Rebuild Data 완료 대기 */
IDE_RC sduProperty::changeSHARD_REBUILD_DATA_STEP( idvSQL * /* aStatistics */,
                                                   SChar  * /* aName */,
                                                   void   * /* aOldValue */,
                                                   void   * aNewValue,
                                                   void   * /* aArg */ )
{
    idlOS::memcpy( & SDU_PROPERTY( mShardRebuildDataStep ),
                   aNewValue,
                   ID_SIZEOF( SInt ) );

    return IDE_SUCCESS;
}

IDE_RC sduProperty::changeSHARD_REBUILD_PLAN_DETAIL_FORCE_ENABLE( idvSQL* /* aStatistics */,
                                                                  SChar * /* aName */,
                                                                  void  * /* aOldValue */,
                                                                  void  * aNewValue,
                                                                  void  * /* aArg */)
{
    idlOS::memcpy( &SDU_PROPERTY( mShardRebuildPlanDetailForceEnable ),
                   aNewValue,
                   ID_SIZEOF( UInt ) );

    return IDE_SUCCESS;
}

IDE_RC sduProperty::changeSHARD_REBUILD_LOCK_TABLE_WITH_DML_ENABLE( idvSQL* /* aStatistics */,
                                                                    SChar * /* aName */,
                                                                    void  * /* aOldValue */,
                                                                    void  * aNewValue,
                                                                    void  * /* aArg */)
{
    idlOS::memcpy( &SDU_PROPERTY( mShardRebuildLockTableWithDmlEnable ),
                   aNewValue,
                   ID_SIZEOF( UInt ) );

    return IDE_SUCCESS;
}

IDE_RC sduProperty::changeSHARD_META_PROPAGATION_TIMEOUT( idvSQL* /* aStatistics */,
                                                          SChar * /* aName */,
                                                          void  * /* aOldValue */,
                                                          void  * aNewValue,
                                                          void  * /* aArg */)
{
    idlOS::memcpy( &SDU_PROPERTY( mShardMetaPropagationTimeout ),
                   aNewValue,
                   ID_SIZEOF( ULong ) );

    return IDE_SUCCESS;
}

IDE_RC sduProperty::changeSHARD_TRANSFORM_STRING_LENGTH_MAX( idvSQL* /* aStatistics */,
                                                             SChar * /* aName */,
                                                             void  * /* aOldValue */,
                                                             void  * aNewValue,
                                                             void  * /* aArg */)
{
    idlOS::memcpy( &SDU_PROPERTY( mShardTransformStringLengthMax ),
                   aNewValue,
                   ID_SIZEOF( UInt ) );

    return IDE_SUCCESS;
}

// BUG-47817
IDE_RC sduProperty::changeSHARD_ADMIN_MODE( idvSQL* /* aStatistics */,
                                            SChar * /* aName */,
                                            void  * /* aOldValue */,
                                            void  * aNewValue,
                                            void  * /* aArg */)
{
    idlOS::memcpy( &SDU_PROPERTY( mShardAdminMode ),
                   aNewValue,
                   ID_SIZEOF( UInt ) );

    return IDE_SUCCESS;
}

/* PROJ-2726 Sharding Node Manager */
IDE_RC sduProperty::changeZOOKEEPER_LOCK_WAIT_TIMEOUT( idvSQL* /* aStatistics */,
                                                       SChar * /* aName */,
                                                       void  * /* aOldValue */,
                                                       void  * aNewValue,
                                                       void  * /* aArg */)
{
    idlOS::memcpy( &SDU_PROPERTY( mZookeeperLockWaitTimeout ),
                   aNewValue,
                   ID_SIZEOF( UInt ) );

    return IDE_SUCCESS;
}

IDE_RC sduProperty::changeSHARD_STATEMENT_RETRY( idvSQL* /* aStatistics */,
                                                 SChar * /* aName */,
                                                 void  * /* aOldValue */,
                                                 void  * aNewValue,
                                                 void  * /* aArg */)
{
    idlOS::memcpy( &SDU_PROPERTY( mShardStatementRetry ),
                   aNewValue,
                   ID_SIZEOF( UInt ) );

    return IDE_SUCCESS;
}

IDE_RC sduProperty::changeSHARD_DDL_LOCK_TIMEOUT( idvSQL* /* aStatistics */,
    SChar * /* aName */,
    void  * /* aOldValue */,
    void  * aNewValue,
    void  * /* aArg */)
{
    idlOS::memcpy( &SDU_PROPERTY( mShardDDLLockTimeout ),
                   aNewValue,
                   ID_SIZEOF( UInt ) );

    return IDE_SUCCESS;
}

IDE_RC sduProperty::changeSHARD_DDL_LOCK_TRY_COUNT( idvSQL* /* aStatistics */,
                                                    SChar * /* aName */,
                                                    void  * /* aOldValue */,
                                                    void  * aNewValue,
                                                    void  * /* aArg */)
{
    idlOS::memcpy( &SDU_PROPERTY( mShardDDLLockTryCount ),
                   aNewValue,
                   ID_SIZEOF( UInt ) );
    
    return IDE_SUCCESS;
}

IDE_RC sduProperty::changeDISABLE_FAILOVER_FOR_WATCHER( idvSQL* /* aStatistics */,
                                                        SChar * /* aName */,
                                                        void  * /* aOldValue */,
                                                        void  * aNewValue,
                                                        void  * /* aArg */)
{
    idlOS::memcpy( &SDU_PROPERTY( mDisableFailoverForWatcher ),
                   aNewValue,
                   ID_SIZEOF( UInt ) );

    return IDE_SUCCESS;
}

IDE_RC sduProperty::changeSHARD_INTERNAL_REPLICATION_PARALLEL_COUNT( idvSQL* /* aStatistics */,
                                                                     SChar * /* aName */,
                                                                     void  * /* aOldValue */,
                                                                     void  * aNewValue,
                                                                     void  * /* aArg */)
{
    idlOS::memcpy( &SDU_PROPERTY( mShardInternalReplicationParallelCount ),
                   aNewValue,
                   ID_SIZEOF( UInt ) );

    return IDE_SUCCESS;
}



