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
 * $Id: sduProperty.h 91426 2021-08-04 04:40:23Z jake.jang $
 **********************************************************************/

#ifndef _O_SDU_PROPERTY_H_
#define _O_SDU_PROPERTY_H_ 1

#include <idl.h>
#include <idp.h>
#include <smi.h>

#define SDU_PROPERTY( aProperty ) ( sduProperty::mStaticProperty.aProperty )

#define SDU_TRCLOG_DETAIL_SHARD                     ( SDU_PROPERTY( mTrclogDetailShard ) )
#define SDU_SHARD_ENABLE                            ( SDU_PROPERTY( mShardEnable ) )
#define SDU_SHARD_TEST_ENABLE                       ( SDU_PROPERTY( mShardTestEnable ) )
#define SDU_SHARD_ALLOW_AUTO_COMMIT                 ( SDU_PROPERTY( mShardAllowAutoCommit ) )
#define SDU_SHARD_LOCAL_FORCE                       ( SDU_PROPERTY( mShardLocalForce ) )
#define SDU_SHARD_AGGREGATION_TRANSFORM_ENABLE      ( SDU_PROPERTY( mShardAggregationTransformEnable ) )
/* TASK-7219 */
#define SDU_SHARD_TRANSFORM_MODE                    ( SDU_PROPERTY( mShardTransformMode ) )
#define SDU_SHARD_INTERNAL_CONN_ATTR_RETRY_COUNT    ( SDU_PROPERTY( mShardInternalConnAttrRetryCount ) )
#define SDU_SHARD_INTERNAL_CONN_ATTR_RETRY_DELAY    ( SDU_PROPERTY( mShardInternalConnAttrRetryDelay ) )
#define SDU_SHARD_INTERNAL_CONN_ATTR_CONN_TIMEOUT   ( SDU_PROPERTY( mShardInternalConnAttrConnTimeout ) )
#define SDU_SHARD_INTERNAL_CONN_ATTR_LOGIN_TIMEOUT  ( SDU_PROPERTY( mShardInternalConnAttrLoginTimeout ) )
#define SDU_SHARD_REBUILD_DATA_STEP                 ( SDU_PROPERTY( mShardRebuildDataStep ) )

/* PROJ-2701 Sharding online data rebuild */
#define SDU_SHARD_REBUILD_PLAN_DETAIL_FORCE_ENABLE ( SDU_PROPERTY(mShardRebuildPlanDetailForceEnable) )
#define SDU_SHARD_REBUILD_LOCK_TABLE_WITH_DML_ENABLE ( SDU_PROPERTY(mShardRebuildLockTableWithDmlEnable) )
#define SDU_SHARD_META_PROPAGATION_TIMEOUT ( SDU_PROPERTY(mShardMetaPropagationTimeout) )
#define SDU_SHARD_TRANSFORM_STRING_LENGTH_MAX ( SDU_PROPERTY(mShardTransformStringLengthMax) )
#define SDU_SHARD_STATEMENT_RETRY ( SDU_PROPERTY(mShardStatementRetry) )

#define SDU_SHARD_REPLICATION_MAX_PARALLEL_COUNT ( SDU_PROPERTY(mShardReplicationMaxParallelCount) )

// BUG-47817
#define SDU_SHARD_ADMIN_MODE                     ( SDU_PROPERTY( mShardAdminMode ) )

/* PROJ-2726 Sharding Node Manager */
#define ZOOKEEPER_LOCK_WAIT_TIMEOUT ( SDU_PROPERTY(mZookeeperLockWaitTimeout ) )
#define SDU_SHARD_ZOOKEEPER_TEST       ( SDU_PROPERTY( mShardZookeeperTest ) )

/* TASK-7219 */
#define SDU_SHARD_TRANSFORM_PUSH_LIMIT_MASK           (0x00000001)
#define SDU_SHARD_TRANSFORM_PUSH_LIMIT_DISABLE        (0x00000000)
#define SDU_SHARD_TRANSFORM_PUSH_LIMIT_ENABLE         (0x00000001)
#define SDU_SHARD_TRANSFORM_PUSH_SELECT_MASK          (0x00000002)
#define SDU_SHARD_TRANSFORM_PUSH_SELECT_DISABLE       (0x00000000)
#define SDU_SHARD_TRANSFORM_PUSH_SELECT_ENABLE        (0x00000002)
#define SDU_SHARD_TRANSFORM_PUSH_PROJECT_MASK         (0x00000004)
#define SDU_SHARD_TRANSFORM_PUSH_PROJECT_DISABLE      (0x00000000)
#define SDU_SHARD_TRANSFORM_PUSH_PROJECT_ENABLE       (0x00000004)
#define SDU_SHARD_TRANSFORM_PUSH_OUT_REF_PRED_MASK    (0x00000008) /* Non-shard DML */
#define SDU_SHARD_TRANSFORM_PUSH_OUT_REF_PRED_DISABLE (0x00000000)
#define SDU_SHARD_TRANSFORM_PUSH_OUT_REF_PRED_ENABLE  (0x00000008)

#define SDU_SHARD_DDL_LOCK_TIMEOUT   ( SDU_PROPERTY( mShardDDLLockTimeout ) )
#define SDU_SHARD_DDL_LOCK_TRY_COUNT   ( SDU_PROPERTY( mShardDDLLockTryCount ) )

#define SDU_DISABLE_FAILOVER_FOR_WATCHER   ( SDU_PROPERTY( mDisableFailoverForWatcher ) )
#define SDU_SHARD_INTERNAL_REPLICATION_PARALLEL_COUNT   ( SDU_PROPERTY( mShardInternalReplicationParallelCount ) )

// 참조 : mmuPropertyArgument
typedef struct sduPropertyArgument
{
    idpArgument mArg;
    UInt        mUserID;
    smiTrans   *mTrans;
} sduPropertyArgument;

typedef struct sduProperties
{
    //----------------------------------------------
    // Trace Log 관련 Properties
    //    - Writable Property의 경우, MM 에서 관리하여야 하나,
    //    - Trace Log는 System Property라기 보다는
    //    - RunTime Property의 의미이다.
    //----------------------------------------------

    /* BUG-45899 */
    UInt   mTrclogDetailShard;

    UInt   mShardEnable;
    UInt   mShardTestEnable;
    UInt   mShardLocalForce;
    UInt   mShardAllowAutoCommit;
    UInt   mShardAggregationTransformEnable;
    /* TASK-7219 */
    UInt   mShardTransformMode;
    UInt   mShardInternalConnAttrRetryCount;
    UInt   mShardInternalConnAttrRetryDelay;
    UInt   mShardInternalConnAttrConnTimeout;
    UInt   mShardInternalConnAttrLoginTimeout;
    SInt   mShardRebuildDataStep;

    /* PROJ-2701 Sharding online data rebuild */
    UInt   mShardRebuildPlanDetailForceEnable;
    UInt   mShardRebuildLockTableWithDmlEnable;
    ULong  mShardMetaPropagationTimeout;
    UInt   mShardTransformStringLengthMax;
    UInt   mShardStatementRetry;

    /* not property file,the below values were set property by internal upper module */
    UInt   mShardReplicationMaxParallelCount; /* by replication module */

    // BUG-47817
    UInt   mShardAdminMode;

    /* PROJ-2726 Sharding Node Manager */
    UInt   mZookeeperLockWaitTimeout;
    UInt   mShardZookeeperTest;
    UInt   mShardDDLLockTimeout;
    UInt   mShardDDLLockTryCount;
    UInt   mDisableFailoverForWatcher;
    UInt   mShardInternalReplicationParallelCount;
} sduProperties;

class sduProperty
{
public:

    static sduProperties mStaticProperty;

    static void initUpperModuleProperty();

    static IDE_RC initProperty( idvSQL *aStatistics );
    static IDE_RC finalProperty( idvSQL *aStatistics );

    // System 구동 시 관련 Property를 Loading 함.
    static IDE_RC load();

    // System 구동 시 관련 Property의 callback 을 등록함
    static IDE_RC setupUpdateCallback();

    //----------------------------------------------
    // Writable Property를 위한 Call Back 함수들
    //----------------------------------------------

    /* BUG-45899 */
    static IDE_RC changeTRCLOG_DETAIL_SHARD( idvSQL * /* aStatistics */,
                                             SChar  * aName,
                                             void   * aOldValue,
                                             void   * aNewValue,
                                             void   * /* aArg */ );

    static IDE_RC changeSHARD_LOCAL_FORCE( idvSQL * /* aStatistics */,
                                           SChar  * /* aName */,
                                           void   * /* aOldValue */,
                                           void   * aNewValue,
                                           void   * /* aArg */);

    /* PROJ-2687 Shard aggregation transform */
    static IDE_RC changeSHARD_AGGREGATION_TRANSFORM_ENABLE( idvSQL * /* aStatistics */,
                                                            SChar  * /* aName */,
                                                            void   * /* aOldValue */,
                                                            void   * aNewValue,
                                                            void   * /* aArg */);

    /* PROJ-2687 Shard aggregation transform */
    static IDE_RC changeSHARD_TRANSFORM_MODE( idvSQL * /* aStatistics */,
                                              SChar  * /* aName */,
                                              void   * /* aOldValue */,
                                              void   * aNewValue,
                                              void   * /* aArg */);

    static IDE_RC changeSHARD_INTERNAL_CONN_ATTR_RETRY_COUNT( idvSQL * /* aStatistics */,
                                                          SChar  * /* aName */,
                                                          void   * /* aOldValue */,
                                                          void   * aNewValue,
                                                          void   * /* aArg */ );

    static IDE_RC changeSHARD_INTERNAL_CONN_ATTR_RETRY_DELAY( idvSQL * /* aStatistics */,
                                                          SChar  * /* aName */,
                                                          void   * /* aOldValue */,
                                                          void   * aNewValue,
                                                          void   * /* aArg */ );

    /* BUG-45967 Rebuild Data 완료 대기 */
    static IDE_RC changeSHARD_REBUILD_DATA_STEP( idvSQL * /* aStatistics */,
                                                 SChar  * /* aName */,
                                                 void   * /* aOldValue */,
                                                 void   * aNewValue,
                                                 void   * /* aArg */ );

    static IDE_RC changeSHARD_INTERNAL_CONN_ATTR_CONN_TIMEOUT( idvSQL* /* aStatistics */,
                                                           SChar * /* aName */,
                                                           void  * /* aOldValue */,
                                                           void  * aNewValue,
                                                           void  * /* aArg */);

    static IDE_RC changeSHARD_INTERNAL_CONN_ATTR_LOGIN_TIMEOUT( idvSQL* /* aStatistics */,
                                                            SChar * /* aName */,
                                                            void  * /* aOldValue */,
                                                            void  * aNewValue,
                                                            void  * /* aArg */);

    /* PROJ-2701 Sharding online data rebuild */
    static IDE_RC changeSHARD_REBUILD_PLAN_DETAIL_FORCE_ENABLE( idvSQL* /* aStatistics */,
                                                                SChar * /* aName */,
                                                                void  * /* aOldValue */,
                                                                void  * aNewValue,
                                                                void  * /* aArg */);

    static IDE_RC changeSHARD_REBUILD_LOCK_TABLE_WITH_DML_ENABLE( idvSQL* /* aStatistics */,
                                                                  SChar * /* aName */,
                                                                  void  * /* aOldValue */,
                                                                  void  * aNewValue,
                                                                  void  * /* aArg */);

    static IDE_RC changeSHARD_META_PROPAGATION_TIMEOUT( idvSQL* /* aStatistics */,
                                                        SChar * /* aName */,
                                                        void  * /* aOldValue */,
                                                        void  * aNewValue,
                                                        void  * /* aArg */);

    static IDE_RC changeSHARD_TRANSFORM_STRING_LENGTH_MAX( idvSQL* /* aStatistics */,
                                                           SChar * /* aName */,
                                                           void  * /* aOldValue */,
                                                           void  * aNewValue,
                                                           void  * /* aArg */);

    static IDE_RC changeSHARD_STATEMENT_RETRY( idvSQL* /* aStatistics */,
                                               SChar * /* aName */,
                                               void  * /* aOldValue */,
                                               void  * aNewValue,
                                               void  * /* aArg */);

    /* The replication attribute related to shading was set. */
    static void setReplicationMaxParallelCount(UInt aMaxParallelCount);

    // BUG-47817
    static IDE_RC changeSHARD_ADMIN_MODE( idvSQL* /* aStatistics */,
                                          SChar * /* aName */,
                                          void  * /* aOldValue */,
                                          void  * aNewValue,
                                          void  * /* aArg */);


    static UInt   setShardAdminMode(UInt aValue) { return mStaticProperty.mShardAdminMode = aValue; }

    /* PROJ-2726 Sharding Node Manager */
    static IDE_RC changeZOOKEEPER_LOCK_WAIT_TIMEOUT( idvSQL* /* aStatistics */,
                                                     SChar * /* aName */,
                                                     void  * /* aOldValue */,
                                                     void  * aNewValue,
                                                     void  * /* aArg */);

    static IDE_RC changeSHARD_DDL_LOCK_TIMEOUT( idvSQL* /* aStatistics */,
                                                SChar * /* aName */,
                                                void  * /* aOldValue */,
                                                void  * aNewValue,
                                                void  * /* aArg */);

    static IDE_RC changeSHARD_DDL_LOCK_TRY_COUNT( idvSQL* /* aStatistics */,
                                                  SChar * /* aName */,
                                                  void  * /* aOldValue */,
                                                  void  * aNewValue,
                                                  void  * /* aArg */);

    static IDE_RC changeDISABLE_FAILOVER_FOR_WATCHER( idvSQL* /* aStatistics */,
                                                      SChar * /* aName */,
                                                      void  * /* aOldValue */,
                                                      void  * aNewValue,
                                                      void  * /* aArg */);

    static IDE_RC changeSHARD_INTERNAL_REPLICATION_PARALLEL_COUNT( idvSQL* /* aStatistics */,
                                                                   SChar * /* aName */,
                                                                   void  * /* aOldValue */,
                                                                   void  * aNewValue,
                                                                   void  * /* aArg */);

};

#endif /* _O_SDU_PROPERTY_H_ */
