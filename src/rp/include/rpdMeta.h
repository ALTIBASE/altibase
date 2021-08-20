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
 * $Id: rpdMeta.h 91356 2021-07-27 06:07:52Z donghyun1 $
 **********************************************************************/

#ifndef _O_RPD_META_H_
#define _O_RPD_META_H_ 1

#include <idl.h>
#include <rp.h>
#include <cm.h>
#include <qci.h>
#include <qcmTableInfo.h>
#include <smDef.h>
//* BUG-10344 E *//
#include <qcm.h>
//#include <rpnComm.h>

class smiStatement;


/* Table Meta Log Record에 저장 되는 BODY
 * Column 정보의 경우 LOG Record 의 BODY 에 저장할때
 * 맨 마지막 문자열 포인터는 제외 하고 복사하고
 * 맨 마지막 문자열 포인터의 경우 LOG Record의 BODY 에 문자열을
 * 직접 복사해 준다. */
#define RP_OLD_COLUMN_META_SIZE                 (ID_SIZEOF(smiColumnMeta) - ID_SIZEOF(SChar *))

/* rpdReplication->mFlags의 상태 비트 */
/* 1번 비트 : Endian bit : 0 - Big Endian, 1 - Little Endian */
#define RP_LITTLE_ENDIAN                        (0x00000001)
#define RP_BIG_ENDIAN                           (0x00000000)
#define RP_ENDIAN_MASK                          (0x00000001)

/* 2번 비트 : Start Sync Apply */
#define RP_START_SYNC_APPLY_FLAG_SET            (0x00000002)
#define RP_START_SYNC_APPLY_FLAG_UNSET          (0x00000000)
#define RP_START_SYNC_APPLY_MASK                (0x00000002)

/* 3번 비트 : Wakeup Peer Sender */
#define RP_WAKEUP_PEER_SENDER_FLAG_SET          (0x00000004)
#define RP_WAKEUP_PEER_SENDER_FLAG_UNSET        (0x00000000)
#define RP_WAKEUP_PEER_SENDER_MASK              (0x00000004)

/* 4번 비트 : Recovery Request proj-1608 */
#define RP_RECOVERY_REQUEST_FLAG_SET            (0x00000008)
#define RP_RECOVERY_REQUEST_FLAG_UNSET          (0x00000000)
#define RP_RECOVERY_REQUEST_MASK                (0x00000008)

/* 5번 비트 : Sender for Recovery proj-1608 */
#define RP_RECOVERY_SENDER_FLAG_SET             (0x00000010)
#define RP_RECOVERY_SENDER_FLAG_UNSET           (0x00000000)
#define RP_RECOVERY_SENDER_MASK                 (0x00000010)

/* 6번 비트 : Sender for Offline PROJ-1915 */
#define RP_OFFLINE_SENDER_FLAG_SET              (0x00000020)
#define RP_OFFLINE_SENDER_FLAG_UNSET            (0x00000000)
#define RP_OFFLINE_SENDER_MASK                  (0x00000020)

/* 7번 비트 : Sender for Eager PROJ-2067 */
#define RP_PARALLEL_SENDER_FLAG_SET             (0x00000040)
#define RP_PARALLEL_SENDER_FLAG_UNSET           (0x00000000)
#define RP_PARALLEL_SENDER_MASK                 (0x00000040)

/* 8번 비트 : Incremental Sync Failback for Eager PROJ-2066 */
#define RP_FAILBACK_INCREMENTAL_SYNC_FLAG_SET   (0x00000080)
#define RP_FAILBACK_INCREMENTAL_SYNC_FLAG_UNSET (0x00000000)
#define RP_FAILBACK_INCREMENTAL_SYNC_MASK       (0x00000080)

/* 9번 비트 : Server Startup Failback for Eager PROJ-2066 */
#define RP_FAILBACK_SERVER_STARTUP_FLAG_SET     (0x00000100)
#define RP_FAILBACK_SERVER_STARTUP_FLAG_UNSET   (0x00000000)
#define RP_FAILBACK_SERVER_STARTUP_MASK         (0x00000100)

/* 10번 비트 : Conditional Start PROJ-2737 */
#define RP_START_CONDITIONAL_FLAG_SET           (0x00000200)
#define RP_START_CONDITIONAL_FLAG_UNSET         (0x00000000)
#define RP_START_CONDITIONAL_MASK               (0x00000200)

/* 11번 비트 : Conditional Sync PROJ-2737 */
#define RP_SYNC_CONDITIONAL_FLAG_SET            (0x00000400)
#define RP_SYNC_CONDITIONAL_FLAG_UNSET          (0x00000000)
#define RP_SYNC_CONDITIONAL_MASK                (0x00000400)

/* 12 비트 : Consistent Incremental sync master PROJ-2742 */
#define RP_XLOGFILE_FAILBACK_INCREMENTAL_SYNC_MASTER_FLAG_SET   (0x00000800)
#define RP_XLOGFILE_FAILBACK_INCREMENTAL_SYNC_MASTER_FLAG_UNSET (0x00000000)
#define RP_XLOGFILE_FAILBACK_INCREMENTAL_SYNC_MASTER_MASK       (0x00000800)

/* 13 비트 : Consistent Incremental sync slave PROJ-2742 */
#define RP_XLOGFILE_FAILBACK_INCREMENTAL_SYNC_SLAVE_FLAG_SET   (0x00001000)
#define RP_XLOGFILE_FAILBACK_INCREMENTAL_SYNC_SLAVE_FLAG_UNSET (0x00000000)
#define RP_XLOGFILE_FAILBACK_INCREMENTAL_SYNC_SLAVE_MASK       (0x00001000)



typedef enum
{
    RP_META_DICTTABLECOUNT  = 0,
    RP_META_PARTITIONCOUNT  = 1,
    RP_META_XSN             = 2,
    RP_META_SRID            = 3,
    RP_META_COMPRESSTYPE    = 4,
    RP_META_MAX
} RP_PROTOCOL_OP_CODE;

typedef struct rpdReplHosts
{
    SInt           mHostNo;
    UInt           mPortNo;
    SChar          mRepName[QC_MAX_OBJECT_NAME_LEN + 1];
    SChar          mHostIp [QC_MAX_IP_LEN + 1];
    RP_SOCKET_TYPE mConnType;
    rpIBLatency    mIBLatency;
} rpdReplHosts;

/* PROJ-1915 SYS_REP_OFFLINE_DIR_ */
typedef struct rpdReplOfflineDirs
{
    SChar mRepName[QC_MAX_OBJECT_NAME_LEN +1];
    UInt  mLFG_ID;
    SChar mLogDirPath[SM_MAX_FILE_NAME];
}rpdReplOfflineDirs;

typedef struct rpdVersion
{
    ULong   mVersion;
}rpdVersion;

typedef enum rpdTableMetaType
{
    RP_META_NONE_ITEM = 0,
    RP_META_INSERT_ITEM,
    RP_META_DELETE_ITEM,
    RP_META_UPDATE_ITEM
} rpdTableMetaType; 

typedef struct rpdOldItem
{
    SChar  mRepName[QC_MAX_OBJECT_NAME_LEN + 1];
    ULong  mTableOID;
    ULong  mOldTableOID;
    SChar  mUserName[QC_MAX_OBJECT_NAME_LEN + 1];  
    SChar  mTableName[QC_MAX_OBJECT_NAME_LEN + 1]; 
    SChar  mPartName[QC_MAX_OBJECT_NAME_LEN + 1];
    SChar  mRemoteUserName[QC_MAX_OBJECT_NAME_LEN + 1];  
    SChar  mRemoteTableName[QC_MAX_OBJECT_NAME_LEN + 1]; 
    SChar  mRemotePartName[QC_MAX_OBJECT_NAME_LEN + 1];
    UInt   mPartitionOrder;
    SChar  mPartCondMinValues[QC_MAX_PARTKEY_COND_VALUE_LEN + 1]; 
    SChar  mPartCondMaxValues[QC_MAX_PARTKEY_COND_VALUE_LEN + 1]; 
    UInt   mPKIndexID;
    ULong  mInvalidMaxSN;
    UInt   mTableID;                     
    UInt   mTablePartitionType;
    SChar  mIsPartition    [2];
    SChar  mReplicationUnit[2]; 
    UInt   mTBSType;
    UInt   mPartitionMethod;
    UInt   mPartitionCount;
} rpdOldItem;

typedef struct rpdReplications
{
    /**************************************************************************
     * Replication Information
     **************************************************************************/
    SChar                 mRepName[QC_MAX_OBJECT_NAME_LEN + 1];
    SChar                 mPeerRepName[QC_MAX_OBJECT_NAME_LEN + 1];

    SInt                  mItemCount;

    SInt                  mHostCount;
    rpdReplHosts         *mReplHosts;
    SInt                  mLastUsedHostNo;

    /*
      For Parallel Logging : XLSN을 로그의 Sequence Number로
      변경한다.(PRJ-1464)
      Repliaction이 종료 후 다시 보낼 경우 다시 보내야할
      첫번째 로그의 SN
    */
    smSN                  mXSN;
    SInt                  mIsStarted;
    SInt                  mConflictResolution;  // using typedef enum RP_CONFLICT_RESOLUTION
    SInt                  mRole;                // Replication Role(Replication, XLog Sender)
    UInt                  mReplMode;            // Replication Mode(Lazy/Eager...)
    UInt                  mFlags;
    SInt                  mOptions;
    SInt                  mInvalidRecovery;
    ULong                 mRPRecoverySN;

    SChar                 mRemoteFaultDetectTime[RP_DEFAULT_DATE_FORMAT_LEN + 1];

    /**************************************************************************
     * Server Information (서버 기동 중에는 미변경)
     **************************************************************************/
    UInt                  mTransTblSize;

    /* PROJ-1915 Off-Line */
    SChar                 mOSInfo[QC_MAX_NAME_LEN + 1];
    UInt                  mCompileBit;
    UInt                  mSmVersionID;
    UInt                  mLFGCount;
    ULong                 mLogFileSize;

    SChar                 mDBCharSet[MTL_NAME_LEN_MAX];         // BUG-23718
    SChar                 mNationalCharSet[MTL_NAME_LEN_MAX];

    SChar                 mServerID[IDU_SYSTEM_INFO_LENGTH + 1];

    UInt                  mParallelID;
    rpdVersion            mRemoteVersion;

    UInt                  mParallelApplierCount;

    ULong                 mApplierInitBufferSize;

    smSN                  mRemoteXSN;
    smSN                  mRemoteLastDDLXSN;

    smLSN                 mCurrentReadLSNFromXLogfile;
    
    cmiCompressType       mCompressType;
    
    UInt                  mSqlApplyTableCount;
} rpdReplications;

typedef struct rpdReplItems
{
    ULong   mTableOID;        /* Table OID */

  //  SInt    pk_all_size;                // not useful anymore
  //  SInt    pk_offset       [QCI_MAX_KEY_COLUMN_COUNT];
  //  SInt    pk_size         [QCI_MAX_KEY_COLUMN_COUNT];

    SChar   mRepName        [QC_MAX_OBJECT_NAME_LEN + 1];
    SChar   mLocalUsername  [QC_MAX_OBJECT_NAME_LEN + 1];
    SChar   mLocalTablename [QC_MAX_OBJECT_NAME_LEN + 1];
    SChar   mLocalPartname  [QC_MAX_OBJECT_NAME_LEN + 1];
    SChar   mRemoteUsername [QC_MAX_OBJECT_NAME_LEN + 1];
    SChar   mRemoteTablename[QC_MAX_OBJECT_NAME_LEN + 1];
    SChar   mRemotePartname [QC_MAX_OBJECT_NAME_LEN + 1];
    SChar   mIsPartition    [2];

    smSN    mInvalidMaxSN;    // PROJ-1602 : for Sender

    UInt    mTBSType; // PROJ-1567
    SChar   mReplicationUnit[2]; // PROJ-2336
    idBool  mIsConditionSynced;
} rpdReplItems;

typedef struct rpdReplSyncItem
{
    SChar          mUserName[QCI_MAX_OBJECT_NAME_LEN + 1];
    SChar          mTableName[QCI_MAX_OBJECT_NAME_LEN + 1];
    SChar          mPartitionName[QCI_MAX_OBJECT_NAME_LEN + 1];
    ULong          mTableOID;
    SChar          mReplUnit[2];
    rpdReplSyncItem * next;
} rpdReplSyncItem;


//proj-1608
typedef struct rpdRecoveryInfo
{
    smSN mMasterBeginSN;
    smSN mMasterCommitSN;
    smSN mReplicatedBeginSN;
    smSN mReplicatedCommitSN;
} rpdRecoveryInfo;

extern "C" int
rpdRecoveryInfoCompare( const void* aElem1, const void* aElem2 );

typedef struct rpdColumn
{
    SChar       mColumnName[QC_MAX_OBJECT_NAME_LEN + 1];
    UChar       mPolicyCode[QCS_POLICY_CODE_SIZE + 1];
    SChar       mECCPolicyName[QCS_POLICY_NAME_SIZE + 1];
    UChar       mECCPolicyCode[QCS_ECC_POLICY_CODE_SIZE + 1];
    UInt        mQPFlag;
    SChar      *mFuncBasedIdxStr;
    mtcColumn   mColumn;
} rpdColumn;

typedef struct rpdIndexSortInfo rpdIndexSortInfo;

/* 
 * PROJ-2737 Internal Replication 
 * 
 * Conditional Action은 local - remote의 아이템 테이블 데이터의 유무로 동작을 결정한다.
 * rpdConditionItemInfo 에 데이터 유무를 저장, 통신 후 비교한다.
 *
 * mIsEmpty ( T - 데이터 없음, F 데이터 있음 )
 *    Local    Remote    Action
 *     F         F       RP_CONDITION_START        
 *     F         T       RP_CONDITION_SYNC        
 *     T         F       RP_CONDITION_TRUNCATE(remote)        
 *     T         T       RP_CONDITION_START        
 *
 * start conditional 인 경우, 
 *   RP_CONDITION_SYNC, RP_CONDITION_TRUNCATE 일때 error를 반환하고 이중화 시작하지 않는다.
 * 
 * */
typedef enum
{
    RP_CONDITION_START     = 0,
    RP_CONDITION_SYNC      = 1,
    RP_CONDITION_TRUNCATE  = 2,
    RP_CONDITION_MAX
} RP_CONDITION_ACTION;

typedef struct rpdConditionItemInfo
{
    ULong                 mTableOID;
    idBool                mIsEmpty;
}rpdConditionItemInfo;

typedef struct rpdConditionActInfo
{
    ULong                 mTableOID;  
    RP_CONDITION_ACTION   mAction;
}rpdConditionActInfo;

typedef enum
{
    RP_APPLY_XLOG     = 0,
    RP_APPLY_SQL      = 1,
    RP_APPLY_SKIP     = 2
} rpdApplyMode;

class rpdMetaItem
{
// Attribute
public:
    rpdReplItems          mItem;
    UInt                  mTableID;
    ULong                 mRemoteTableOID;

    SInt                  mColCount;
    rpdColumn            *mColumns;

    SInt                  mPKColCount;
    qcmIndex              mPKIndex;

    mtcColumn            *mTsFlag;

    // Temporal Meta : It is only used Handshake
    // Not used normal operation
    SInt                  mIndexCount;
    qcmIndex             *mIndices;

    /* BUG-34360 Check Constraint */
    UInt                  mCheckCount;
    qcmCheck             *mChecks;

    // PROJ-1624 non-partitioned index
    qciIndexTableRef     *mIndexTableRef;   /* Receiver 경우만 유효함 */
    void                 *mQueueMsgIDSeq;
    // PROJ-1502 Partitioned Disk Table
    UInt                  mTablePartitionType;
    UInt                  mPartitionMethod;
    UInt                  mPartitionOrder;
    SChar                 mPartCondMinValues[QC_MAX_PARTKEY_COND_VALUE_LEN + 1];
    SChar                 mPartCondMaxValues[QC_MAX_PARTKEY_COND_VALUE_LEN + 1];
    UInt                  mPartitionCount;

    /* PROJ-1442 Replication Online 중 DDL 허용
     * Column 추가/삭제 지원을 위해 추가한다.
     * Handshake 시 Receiver가 구성하는 정보이며, Receiver만 사용한다.
     */
    UInt                  mMapColID[SMI_COLUMN_ID_MAXIMUM];
    idBool                mIsReplCol[SMI_COLUMN_ID_MAXIMUM];
    idBool                mHasOnlyReplCol;  // Replication 대상 Column만 있는가
    idBool                mNeedCompact;

    UInt                  mCompressColCount;
    UInt                  mCompressCID[SMI_COLUMN_ID_MAXIMUM];

private:
    rpdApplyMode          mApplyMode;
    idBool                mIsDummyItem;
    
// Method
public :
    void   initialize( void );
    void   finalize( void );

    void   freeMemory( void );
    ULong  getTotalColumnSizeExceptGeometryAndLob();
    idBool isLobColumnExist( void );

    /* BUG-34360 Check Constraint */
    void  freeMemoryCheck();  

    /* BUG-34360 Check Constraint */
    IDE_RC buildCheckInfo( const qcmTableInfo    *aTableInfo );

    /* BUG-34360 Check Constraint */
    IDE_RC copyCheckInfo( const qcmCheck     *aSource,
                          const UInt          aCheckCount );

    /* BUG-34360 Check Constraint */
    IDE_RC equalCheckList( const rpdMetaItem * aMetaItem );

    /* BUG-37295 Function based index */
    static void getReplColumnIDList( const mtcColumn * aColumnList,
                                     const UInt        aColumnCount,
                                     UInt            * aReplColumnList );

    /* BUG-34360 Check Constraint */
    static UInt getReplColumnCount( const UInt   * aColumns,
                                    const UInt     aColumnCount,
                                    const idBool * aIsReplColArr );

    IDE_RC lockReplItem( smiTrans            * aTransForLock,
                         smiStatement        * aStatementForSelect,
                         smiTBSLockValidType   aTBSLvType,
                         smiTableLockMode      aLockMode,
                         ULong                 aLockWaitMicroSec );

    IDE_RC lockReplItemForDDL( void                * aQcStatement,
                               idBool                aIsValidate,
                               smiTBSLockValidType   aTBSLvType,
                               smiTableLockMode      aLockMode,
                               ULong                 aLockWaitMicroSec );

    inline rpdColumn * getPKRpdColumn(UInt aPkeyID)
    {
        return getRpdColumn( mPKIndex.keyColumns[aPkeyID].column.id);
    }

    inline rpdColumn * getRpdColumn (UInt aColumnID)
    {
        aColumnID &= SMI_COLUMN_ID_MASK;
        if((0 < mColCount) && (aColumnID < (UInt)mColCount))
        {
            return &mColumns[aColumnID];
        }
        else
        {
            return NULL;
        }
    }

    /* PROJ-2563
     * 강제로 A5프로토콜을 사용할때 테이블에 LOB이 있을 경우,
     * 이중화 구성을 할 수 없도록 한다.
     */
    idBool mHasLOBColumn;

    inline idBool hasLOBColumn( void )
    {
        return mHasLOBColumn;
    }

    rpdApplyMode         getApplyMode( void );
    void                 setApplyMode( rpdApplyMode aApplyMode );

    idBool      isDummyItem( void );
    void        setIsDummyItem( idBool aIsDummyItem );
    
    inline idBool needCompact( void )
    {
        return mNeedCompact;
    }

private:
    /* BUG-34360 Check Constraint */
    IDE_RC equalCheck( const SChar    * aTableName,
                       const qcmCheck * aCheck ) const;

    /* BUG-34360 Check Constraint */
    IDE_RC findCheckIndex( const SChar   *aTableName,
                           const SChar   *aCheckName,
                           UInt          *aCheckIndex ) const;

    /* BUG-34360 Check Constraint */
    IDE_RC compareReplCheckColList( const SChar    * aTableName,
                                    const qcmCheck * aCheck,
                                    idBool         * aIsReplCheck );
    IDE_RC lockPartitionList( void                 * aQcStatement,
                              qcmPartitionInfoList * aPartInfoList,
                              smiTBSLockValidType    aTBSValitationOpt,
                              smiTableLockMode       aLockMode,
                              ULong                  aLockWaitMicroSec );
};

class rpdMeta
{
public:
    rpdMeta();
    void   changeToRemoteMetaForSimulateHandshake();
    static IDE_RC checkLocalNRemoteName( rpdReplItems * aItems );

    static IDE_RC equals( smiStatement      * aSmiStatement,
                          idBool              aIsLocalReplication,
                          UInt                aSqlApplyEnable,
                          UInt                aItemCountDiffEnable,
                          rpdMeta           * aRemoteMeta,
                          rpdMeta           * aLocalMeta );

    static IDE_RC equalRepl( idBool            aIsLocalReplication,
                             UInt              aSqlApplyEnable,
                             UInt              aItemCountDiffEnable,
                             rpdReplications * aRemoteRepl,
                             rpdReplications * aLocalRepl );
    static IDE_RC equalItemsAndMakeDummy( smiStatement      * aSmiStatement,
                                          idBool              aIsLocalReplication,
                                          UInt                aSqlApplyEnable,
                                          UInt                aItemCountDiffEnable,
                                          rpdMeta           * aRemoteMeta,
                                          rpdMeta           * aLocalMeta );
    static IDE_RC equalItems( smiStatement      * aSmiStatement,
                              idBool              aIsLocalReplication,
                              UInt                aSqlApplyEnable,
                              UInt                aItemCountDiffEnable,
                              rpdMeta           * aRemoteMeta,
                              rpdMeta           * aLocalMeta );
    static IDE_RC equalItem( smiStatement   * aSmiStatement,
                             UInt             aReplMode,
                             idBool           aIsLocalReplication,
                             UInt             aSqlApplyEnable,
                             UInt             aItemCountDiffEnable,
                             rpdMetaItem    * aItem1, 
                             rpdMetaItem    * aItem2,
                             rpdVersion       aRemoteVersion );
    static IDE_RC equalPartCondValues( smiStatement     * aSmiStatement,
                                       SChar            * aTableName,
                                       SChar            * aUserName,
                                       SChar            * aPartCondValues1,
                                       SChar            * aPartCondValues2 );
    static IDE_RC equalColumn(SChar      *aTableName1,
                              rpdColumn  *aCol1,
                              SChar      *aTableName2,
                              rpdColumn  *aCol2 );
    static IDE_RC equalIndex(SChar     *aTableName1,
                             qcmIndex  *aIndex1,
                             SChar     *aTableName2,
                             qcmIndex  *aIndex2,
                             UInt      *aMapColIDArr2 );

    /* BUG-37295 Function based index */
    static IDE_RC equalFuncBasedIndex( const rpdReplItems * aItem1,
                                       const qcmIndex     * aFuncBasedIndex1,
                                       const rpdColumn    * aColumns1,
                                       const rpdReplItems * aItem2,
                                       const qcmIndex     * aFuncBasedIndex2,
                                       const rpdColumn    * aColumns2 );

    /* BUG-37295 Function based index */
    static idBool isFuncBasedIndex( const qcmIndex  * aIndex,
                                    const rpdColumn * aColumns );

    /* BUG-37295 Function based index */
    static IDE_RC validateIndex( const SChar     * aTableName,
                                 const qcmIndex  * aIndex,
                                 const rpdColumn * aColumns,
                                 const idBool    * aIsReplColArr,
                                 idBool          * aIsValid );

    /* PROJ-1442 Replication Online 중 DDL 허용
     * Callback for smiLogRec
     */
    static IDE_RC getMetaItem(const void  *  aMeta,
                              ULong          aItemOID,
                              const void  ** aItem);
    static UInt  getColumnCount(const void * aItem);
    static const smiColumn * getSmiColumn(const void * aItem,
                                          UInt         aColumnID);

    /* PROJ-1442 Replication Online 중 DDL 허용 */
    static IDE_RC insertOldMetaItem(smiStatement * aSmiStmt,
                                    rpdMetaItem  * aItem);
    static IDE_RC insertOldMetaRepl(smiStatement * aSmiStmt,
                                 rpdMeta      * aMeta);
    static IDE_RC deleteOldMetaItem(smiStatement * aSmiStmt,
                                 SChar        * aRepName,
                                 ULong          aItemOID);
    static IDE_RC deleteOldMetaItems(smiStatement    * aSmiStmt,
                                     SChar           * aRepName,
                                     SChar           * aUserName,
                                     SChar           * aTableName,
                                     SChar           * aPartitionName,
                                     rpReplicationUnit aReplUnit);
    static IDE_RC removeOldMetaRepl(smiStatement * aSmiStmt,
                                    SChar        * aRepName);

    static IDE_RC insertReplOldPartItem( smiStatement * aSmiStmt, rpdMetaItem * aItem );

    void   initialize();
    void   freeMemory();
    void   finalize();

    static void setRpdReplication( rpdReplications    * aReplication ) ;

    IDE_RC build(smiStatement       * aSmiStmt,
                 SChar              * aRepName,
                 idBool               aForUpdateFlag,
                 RP_META_BUILD_TYPE   aMetaBuildType,
                 smiTBSLockValidType  aTBSLvType);
    IDE_RC buildWithNewTransaction( idvSQL             * aStatistics,
                                    SChar              * aRepName,
                                    idBool               aForUpdateFlag,
                                    RP_META_BUILD_TYPE   aMetaBuildType );
    static IDE_RC buildTableInfo(smiStatement *aSmiStmt,
                                 rpdMetaItem  *aMetaItem,
                                 smiTBSLockValidType aTBSLvType);
    static IDE_RC buildColumnInfo(RP_IN     qcmColumn  *aQcmColumn,
                                  RP_OUT    rpdColumn  *aColumn,
                                  RP_OUT    mtcColumn **aTsFlag);
    static IDE_RC buildIndexInfo(RP_IN     qcmTableInfo  *aTableInfo,
                                 RP_OUT    SInt          *aIndexCount,
                                 RP_OUT    qcmIndex     **aIndices);

    static IDE_RC buildPartitonInfo( smiStatement * aSmiStmt,
                                     qcmTableInfo * aItemInfo,
                                     rpdMetaItem  * aMetaItem );

    IDE_RC buildIndexTableRef( smiStatement          * aSmiStmt );

    static IDE_RC buildIndexTableRef( smiStatement          * aSmiStmt,
                                      rpdMetaItem           * aMetaItem );

    static IDE_RC isExistItemInLastMeta( smiStatement * aSmiStmt,
                                         SChar        * aRepName,
                                         smOID          aTableOID,
                                         idBool       * aOutIsExist );

    IDE_RC buildLastItemInfo( smiStatement * aSmiStmt, 
                              idBool         aForUpdateFlag,
                              smiTBSLockValidType aTBSLvType );

    IDE_RC buildOldItemsInfo( smiStatement * aSmiStmt );
    IDE_RC buildOldColumnsInfo(smiStatement * aSmiStmt,
                               rpdMetaItem  * aMetaItem);
    IDE_RC buildOldIndicesInfo(smiStatement * aSmiStmt,
                               rpdMetaItem  * aMetaItem);

    rpdMetaItem * findMatchMetaItem( smiTableMeta * aOldItem );

    IDE_RC buildOldPartitonInfo( smiStatement * aSmiStmt,
                                 rpdMetaItem  * aMetaItem,
                                 smiTBSLockValidType aTBSLvType );

    IDE_RC buildOldCheckInfo( smiStatement * aSmiStmt,
                              rpdMetaItem  * aMetaItem );

    static IDE_RC writeTableMetaLog( void         * aQcStatement,
                                     smOID          aOldTableOID,
                                     smOID          aNewTableOID,
                                     SChar        * aRepName,
                                     rpdReplItems * aReplItem,
                                     idBool         aUpdateMeta );

    IDE_RC insertNewTableInfo( smiStatement * aSmiStmt, 
                               smiTableMeta * aItemMeta,
                               const void   * aLogBody,
                               smSN           aDDLCommitSN,
                               idBool         aIsUpdate );

    IDE_RC updateOldTableInfo( smiStatement   * aSmiStmt,
                               rpdMetaItem    * aItemCache,
                               smiTableMeta   * aItemMeta,
                               const void     * aLogBody,                                    
                               smSN             aDDLCommitSN,
                               idBool           aIsUpdateOldItem );

    IDE_RC deleteOldTableInfo( smiStatement * aSmiStmt, 
                               const void   * aLogBody,                                    
                               smOID          aOldTableOID,
                               idBool         aIsUpdate );

    IDE_RC  allocSortItems( void );
    void    sortItemsAfterBuild();

    IDE_RC sendMeta( void               * aHBTResource,
                     cmiProtocolContext * aProtocolContext,
                     idBool             * aExitFlag,
                     idBool               aMetaInitFlag,
                     UInt                 aTimeoutSec ); // BUGBUG : add return flag
    IDE_RC recvMeta( cmiProtocolContext * aProtocolContext,
                     idBool             * aExitFlag,
                     rpdVersion           aVersion,
                     UInt                 aTimeoutSec,
                     idBool             * aMetaInitFlag );    // BUGBUG : add return flag
    static idBool needToProcessProtocolOperation( RP_PROTOCOL_OP_CODE aOpCode, 
                                                  rpdVersion          aRemoteVersion ); 
    void sort( void );


    IDE_RC searchTable             ( rpdMetaItem** aItem, ULong  aContID );

    IDE_RC searchRemoteTable       ( rpdMetaItem** aItem, ULong  aContID );

    /* PROJ-1915 Meta를 복제한다. */
    IDE_RC copyMeta(rpdMeta * aDestMeta);
 
    idBool isLobColumnExist( void );

    // PROJ-1624 non-partitioned index
    static IDE_RC copyIndex(qcmIndex  * aSrcIndex, qcmIndex ** aDstIndex);

    /* PROJ-2397 Compressed Table Replication */
    void   setCompressColumnOIDArray( );
    void   sortCompColumnArray( );
    idBool isMyDictTable( ULong aOID );

    inline idBool existMetaItems()
    {
        return (mItems != NULL) ? ID_TRUE : ID_FALSE;
    }

    /* rpdReplication->mFlags의 상태 bit를 set/clear, mask하는 함수들 */
    /* 1번 비트 : Endian bit : 0 - Big Endian, 1 - Little Endian */
    inline static void setReplFlagBigEndian(rpdReplications *aRepl)
    {
        aRepl->mFlags &= ~RP_LITTLE_ENDIAN;
    }
    inline static void setReplFlagLittleEndian(rpdReplications *aRepl)
    {
        aRepl->mFlags |= RP_LITTLE_ENDIAN;
    }
    inline static UInt getReplFlagEndian(rpdReplications *aRepl)
    {
        return(aRepl->mFlags & RP_ENDIAN_MASK);
    }

    /* 2번 비트 : Start Sync Apply */
    inline static idBool isRpStartSyncApply(rpdReplications *aRepl)
    {
        if((aRepl->mFlags & RP_START_SYNC_APPLY_MASK)
           == RP_START_SYNC_APPLY_FLAG_SET)
        {
            return ID_TRUE;
        }

        return ID_FALSE;
    }
    inline static void setReplFlagStartSyncApply(rpdReplications *aRepl)
    {
        aRepl->mFlags |= RP_START_SYNC_APPLY_FLAG_SET;
    }
    inline static void clearReplFlagStartSyncApply(rpdReplications *aRepl)
    {
        aRepl->mFlags &= ~RP_START_SYNC_APPLY_MASK;
    }

    /* 3번 비트 : Wakeup Peer Sender */
    inline static idBool isRpWakeupPeerSender(rpdReplications *aRepl)
    {
        if((aRepl->mFlags & RP_WAKEUP_PEER_SENDER_MASK)
           == RP_WAKEUP_PEER_SENDER_FLAG_SET)
        {
            return ID_TRUE;
        }

        return ID_FALSE;
    }
    inline static void setReplFlagWakeupPeerSender(rpdReplications *aRepl)
    {
        aRepl->mFlags |= RP_WAKEUP_PEER_SENDER_FLAG_SET;
    }
    inline static void clearReplFlagWakeupPeerSender(rpdReplications *aRepl)
    {
        aRepl->mFlags &= ~RP_WAKEUP_PEER_SENDER_MASK;
    }

    /* 4번 비트 : recovery request*/
    inline static idBool isRpRecoveryRequest(rpdReplications *aRepl)
    {
        if((aRepl->mFlags & RP_RECOVERY_REQUEST_MASK)
           == RP_RECOVERY_REQUEST_FLAG_SET)
        {
            return ID_TRUE;
        }

        return ID_FALSE;
    }
    inline static void setReplFlagRecoveryRequest(rpdReplications *aRepl)
    {
        aRepl->mFlags |= RP_RECOVERY_REQUEST_FLAG_SET;
    }
    inline static void clearReplFlagRecoveryRequest(rpdReplications *aRepl)
    {
        aRepl->mFlags &= ~RP_RECOVERY_REQUEST_MASK;
    }

    /* 5번 비트 : Sender for Recovery proj-1608 */
    inline static idBool isRpRecoverySender(rpdReplications *aRepl)
    {
        if((aRepl->mFlags & RP_RECOVERY_SENDER_MASK)
           == RP_RECOVERY_SENDER_FLAG_SET)
        {
            return ID_TRUE;
        }

        return ID_FALSE;
    }
    inline static void setReplFlagRecoverySender(rpdReplications *aRepl)
    {
        aRepl->mFlags |= RP_RECOVERY_SENDER_FLAG_SET;
    }
    inline static void clearReplFlagRecoverySender(rpdReplications *aRepl)
    {
        aRepl->mFlags &= ~RP_RECOVERY_SENDER_MASK;
    }

    inline static void setReplFlagParallelSender(rpdReplications *aRepl)
    {
        aRepl->mFlags |= RP_PARALLEL_SENDER_FLAG_SET;
    }
    inline static void clearReplFlagParallelSender(rpdReplications *aRepl)
    {
        aRepl->mFlags &= ~RP_PARALLEL_SENDER_MASK;
    }
    inline static idBool isRpParallelSender(rpdReplications *aRepl)
    {
        if((aRepl->mFlags & RP_PARALLEL_SENDER_MASK)
           == RP_PARALLEL_SENDER_FLAG_SET)
        {
            return ID_TRUE;
        }

        return ID_FALSE;
    }

    inline static void setReplFlagFailbackIncrementalSync(rpdReplications *aRepl)
    {
        aRepl->mFlags |= RP_FAILBACK_INCREMENTAL_SYNC_FLAG_SET;
    }
    inline static void clearReplFlagFailbackIncrementalSync(rpdReplications *aRepl)
    {
        aRepl->mFlags &= ~RP_FAILBACK_INCREMENTAL_SYNC_MASK;
    }
    inline static idBool isRpFailbackIncrementalSync(rpdReplications *aRepl)
    {
        if((aRepl->mFlags & RP_FAILBACK_INCREMENTAL_SYNC_MASK)
           == RP_FAILBACK_INCREMENTAL_SYNC_FLAG_SET)
        {
            return ID_TRUE;
        }

        return ID_FALSE;
    }

    inline static void setReplFlagFailbackServerStartup( rpdReplications * aRepl )
    {
        aRepl->mFlags |= RP_FAILBACK_SERVER_STARTUP_FLAG_SET;
    }
    inline static void clearReplFlagFailbackServerStartup( rpdReplications * aRepl )
    {
        aRepl->mFlags &= ~RP_FAILBACK_SERVER_STARTUP_MASK;
    }
    inline static idBool isRpFailbackServerStartup( rpdReplications * aRepl )
    {
        if ( ( aRepl->mFlags & RP_FAILBACK_SERVER_STARTUP_MASK )
             == RP_FAILBACK_SERVER_STARTUP_FLAG_SET )
        {
            return ID_TRUE;
        }

        return ID_FALSE;
    }

    inline static void setReplFlagSyncCondition( rpdReplications * aRepl )
    {
        aRepl->mFlags |= RP_SYNC_CONDITIONAL_FLAG_SET;
    }
    inline static void clearReplFlagSyncCondition( rpdReplications * aRepl )
    {
        aRepl->mFlags &= ~RP_SYNC_CONDITIONAL_MASK;
    }
    inline static idBool isRpSyncCondition( rpdReplications * aRepl )
    {
        if ( ( aRepl->mFlags & RP_SYNC_CONDITIONAL_MASK )
             == RP_SYNC_CONDITIONAL_FLAG_SET )
        {
            return ID_TRUE;
        }

        return ID_FALSE;
    }

    inline static void setReplFlagStartCondition( rpdReplications * aRepl )
    {
        aRepl->mFlags |= RP_START_CONDITIONAL_FLAG_SET;
    }
    inline static void clearReplFlagStartCondition( rpdReplications * aRepl )
    {
        aRepl->mFlags &= ~RP_START_CONDITIONAL_MASK;
    }
    inline static idBool isRpStartCondition( rpdReplications * aRepl )
    {
        if ( ( aRepl->mFlags & RP_START_CONDITIONAL_MASK )
             == RP_START_CONDITIONAL_FLAG_SET )
        {
            return ID_TRUE;
        }

        return ID_FALSE;
    }

    /* PROJ-1915 : OFF-LINE 리시버 세팅 */
    inline static idBool isRpOfflineSender(rpdReplications *aRepl)
    {
        if((aRepl->mFlags & RP_OFFLINE_SENDER_MASK)
            == RP_OFFLINE_SENDER_FLAG_SET)
        {
            return ID_TRUE;
        }

        return ID_FALSE;
    }
    inline static void setReplFlagOfflineSender(rpdReplications *aRepl)
    {
        aRepl->mFlags |= RP_OFFLINE_SENDER_FLAG_SET;
    }
    inline static void clearReplFlagOfflineSender(rpdReplications *aRepl)
    {
        aRepl->mFlags &= ~RP_OFFLINE_SENDER_MASK;
    }

    /* PROJ-2742 : Consistent replication incremental sync */
    inline static idBool isRpXLogfileFailbackIncrementalSyncMaster(rpdReplications *aRepl)
    {
        if((aRepl->mFlags & RP_XLOGFILE_FAILBACK_INCREMENTAL_SYNC_MASTER_MASK)
            == RP_XLOGFILE_FAILBACK_INCREMENTAL_SYNC_MASTER_FLAG_SET)
        {
            return ID_TRUE;
        }

        return ID_FALSE;
    }
    inline static void setReplFlagXLogfileFailbackIncrementalSyncMaster(rpdReplications *aRepl)
    {
        aRepl->mFlags |= RP_XLOGFILE_FAILBACK_INCREMENTAL_SYNC_MASTER_FLAG_SET;
    }
    inline static void clearReplFlagXLogfileFailbackIncrementalSyncMaster(rpdReplications *aRepl)
    {
        aRepl->mFlags &= ~RP_XLOGFILE_FAILBACK_INCREMENTAL_SYNC_MASTER_MASK;
    }

    inline static idBool isRpXLogfileFailbackIncrementalSyncSlave(rpdReplications *aRepl)
    {
        if((aRepl->mFlags & RP_XLOGFILE_FAILBACK_INCREMENTAL_SYNC_SLAVE_MASK)
            == RP_XLOGFILE_FAILBACK_INCREMENTAL_SYNC_SLAVE_FLAG_SET)
        {
            return ID_TRUE;
        }

        return ID_FALSE;
    }
    inline static void setReplFlagXLogfileFailbackIncrementalSyncSlave(rpdReplications *aRepl)
    {
        aRepl->mFlags |= RP_XLOGFILE_FAILBACK_INCREMENTAL_SYNC_SLAVE_FLAG_SET;
    }
    inline static void clearReplFlagXLogfileFailbackIncrementalSyncSlave(rpdReplications *aRepl)
    {
        aRepl->mFlags &= ~RP_XLOGFILE_FAILBACK_INCREMENTAL_SYNC_SLAVE_MASK;
    }


    static idBool isUseV6Protocol( rpdReplications * aReplication );

    smOID getTableOID( UInt aIndex );

    idBool  hasLOBColumn( void );

    UInt  getParallelApplierCount( void );
    ULong getApplierInitBufferSize( void );
    UInt  getMaxPkColCountInAllItem( void );

    smSN  getRemoteXSN( void );

    smSN  getRemoteLastDDLXSN( void );

    /* BUG-42851 */
    static idBool isTransWait( rpdReplications * aRepl );

    /* BUG-45236 Local Replication 지원 */
    static idBool isLocalReplication( rpdMeta * aPeerMeta );

    /* BUG-45236 Local Replication 지원 */
    static IDE_RC getPeerReplNameWithNewTransaction( SChar * aRepName,
                                                     SChar * aPeerRepName );

    /* PROJ-2642 */
    void             setSqlApplyTableCount( void );
    UInt             getSqlApplyTableCount( void );
    void             printItemActionInfo( void );

    idBool           isTargetItem( SChar * aUserName,
                                   SChar * aTargetTableName,
                                   SChar * aTargetPartName );

    rpdMetaItem    * findTableMetaItem( SChar * aLocalUserName, SChar * aLocalTableName );
    rpdMetaItem    * findTablePartitionMetaItem( SChar * aLocalUserName,
                                                 SChar * aLocalTableName,
                                                 SChar * aLocalPartname );

    static rpdTableMetaType getTableMetaType( smOID aOldTableOID, smOID aNewTableOID );

    /* PROJ-2737 Internal replicaion */

    static IDE_RC isEmptyTable( smiStatement * aSmiStmt,
                                ULong          aTableOID,
                                idBool       * aIsEmpty );

    static IDE_RC makeConditionInfoWithItems(  smiStatement           * aSmiStmt,
                                               SInt                     aItemCount,
                                               rpdMetaItem           ** aItemList,
                                               rpdConditionItemInfo   * aConditionInfo );

    static void   compareCondition( rpdConditionItemInfo   * aSrcCondition,
                                    rpdConditionItemInfo   * aDstCondition,
                                    UInt                     aItemCount,
                                    rpdConditionActInfo    * aResultCondition );

    static void   remappingTableOID( rpdMeta * aRemoteMeta,
                                     rpdMeta * aLocalMeta );

    IDE_RC checkItemReplaceHistoryAndSetTableOID( );

    SChar * getRepName() 
    { 
        return mReplication.mRepName; 
    }

    UInt getReplMode()
    {
        return mReplication.mReplMode;
    }

    smLSN getCurrentReadXLogfileLSN()
    {
        return mReplication.mCurrentReadLSNFromXLogfile;
    }

    void setCurrentReadXLogfileLSN( smLSN aLSN )
    {
        mReplication.mCurrentReadLSNFromXLogfile.mFileNo = aLSN.mFileNo;
        mReplication.mCurrentReadLSNFromXLogfile.mOffset = aLSN.mOffset;
    }

    rpReceiverStartMode getReceiverStartMode( void );

    static void         fillOldMetaItem( rpdMetaItem * aItem, rpdOldItem * aOldItem );
    static idBool       needLastMetaItemInfo( RP_META_BUILD_TYPE      aMetaBuildType,
                                              rpdReplications       * aReplication );
    
    static void         getProtocolCompressType( rpdVersion         aRemoteVersion,
                                                 cmiCompressType   *aCompressType );
    
private:

    static IDE_RC       equalPartitionInfo( smiStatement   * aSmiStatement,
                                            rpdMetaItem    * aItem1,
                                            rpdMetaItem    * aItem2 );
    static void         initializeAndMappingColumn( rpdMetaItem   * aItem1,
                                                    rpdMetaItem   * aItem2 );

    static IDE_RC       equalColumns( rpdMetaItem    * aItem1,
                                      rpdMetaItem    * aItem2 );

    static IDE_RC       equalColumnsAttr( rpdMetaItem    * aItem1,
                                          rpdMetaItem    * aItem2,
                                          rpdVersion       aRemoteVersion );
    static IDE_RC       equalColumnSecurity( SChar      * aTableName1,
                                             rpdColumn  * aCol1,
                                             SChar      * aTableName2,
                                             rpdColumn  * aCol2 );
    
    static IDE_RC       equalColumnSRID( SChar      * aTableName1,
                                         rpdColumn  * aCol1,
                                         SChar      * aTableName2,
                                         rpdColumn  * aCol2 );

    static IDE_RC       equalChecks( rpdMetaItem    * aItem1,
                                     rpdMetaItem    * aItem2 );
    static IDE_RC       equalPrimaryKey( rpdMetaItem   * aItem1,
                                         rpdMetaItem   * aItem2 );
    static IDE_RC       equalIndices( rpdMetaItem   * aItem1,
                                      rpdMetaItem   * aItem2 );

    static IDE_RC       checkRemainIndex( rpdMetaItem      * aItem,
                                          rpdIndexSortInfo * aIndexSortInfo,
                                          SInt               aStartIndex,
                                          idBool             aIsLocalIndex,
                                          SChar            * aLocalTablename,
                                          SChar            * aRemoteTablename );

    static IDE_RC       checkSqlApply( rpdMetaItem    * aItem1,
                                       rpdMetaItem    * aItem2 );

    static idBool       equalReplTables( rpdMeta * aMeta1, rpdMeta * aMeta2 );
    static void         printReplTables( SChar *aHeader, rpdMeta * aMeta );
    static idBool       equalReplTablePartition( rpdMeta * aMeta1, rpdMeta * aMeta2 );

    static idBool       isMetaItemMatch( rpdMetaItem * aMetaItem1, rpdMetaItem * aMetaItem2 );

    static void         copyPartCondValue( SChar * aDst, SChar * aSrc );

    static IDE_RC       copyTableInfo( rpdMetaItem * aDestItem, rpdMetaItem * aSrcItem );

    static IDE_RC       copyColumns( rpdMetaItem * aDestItem, rpdMetaItem * aSrcItem );

    static IDE_RC       copyIndice( rpdMetaItem * aDestItem, rpdMetaItem * aSrcItem );

    static IDE_RC       copyIndexTableRef( rpdMetaItem * aDestItem, rpdMetaItem * aSrcItem );

    static IDE_RC       isNeedDummyItems( UInt      aSqlApplyEnable,
                                          UInt      aItemCountDiffEnable,
                                          rpdMeta * aRemoteMeta, 
                                          rpdMeta * aLocalMeta,
                                          idBool  * aIsNeed );

    SInt                getMetaItemMatchCount( rpdMeta * aMeta );

    IDE_RC              makeDummyMetaItem( rpdMetaItem * aRemoteItem, rpdMetaItem * aItem );

    IDE_RC              removeDummyMetaItems( void );

    IDE_RC              insertNewItem( smiStatement * aSmiStmt, 
                                       smiTableMeta * aItemMeta,
                                       const void   * aLogBody,
                                       smSN           aDDLCommitSN,
                                       idBool         aIsUpdate );
    IDE_RC              deleteOldItem( smiStatement * aSmiStmt, 
                                       const void   * aLogBody,
                                       smOID          aOldTableOID,
                                       idBool         aIsUpdate );


    IDE_RC              reallocSortItems( void );

    IDE_RC              buildDictTables( void );

    IDE_RC              rebuildDictTables( void );

    void                freeDictTables( void );

    void                freeItems( void );

    void                freeSortItems( void );

    // Attribute
public:
    rpdReplications       mReplication;

    rpdMetaItem**         mItemsOrderByTableOID;
    rpdMetaItem**         mItemsOrderByRemoteTableOID;
    rpdMetaItem**         mItemsOrderByLocalName;
    rpdMetaItem**         mItemsOrderByRemoteName;

    SChar                 mErrMsg[RP_ACK_MSG_LEN];

    rpdMetaItem          *mItems;
    smSN                  mChildXSN; //Proj-2067 parallel sender

    SInt                  mDictTableCount;
    smOID                *mDictTableOID;
    smOID               **mDictTableOIDSorted;
};

#endif  /* _O_RPD_META_H_ */

