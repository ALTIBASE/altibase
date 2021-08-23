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
 * $Id: qdParseTree.h 91512 2021-08-21 07:50:50Z emlee $
 **********************************************************************/

#ifndef _O_QD_PARSE_TREE_H_
#define _O_QD_PARSE_TREE_H_ 1

#include <smiMisc.h>
#include <qc.h>
#include <qmsParseTree.h>
#include <qcmPriv.h>
#include <qcuProperty.h>
#include <sdiTypes.h>

//PROJ-1583 large geometry
#define QD_MAX_SQL_LENGTH                               (32767)

/* Memory Table Segement Attribute */
#define QD_MEMORY_TABLE_DEFAULT_PCTFREE                 (0)
#define QD_MEMORY_TABLE_DEFAULT_PCTUSED                 (0)
#define QD_MEMORY_TABLE_DEFAULT_INITRANS                (0)
#define QD_MEMORY_TABLE_DEFAULT_MAXTRANS                (0)
/* Memory Index Segement Attribute */
#define QD_MEMORY_INDEX_DEFAULT_INITRANS                (0)
#define QD_MEMORY_INDEX_DEFAULT_MAXTRANS                (0)
/* Disk Table Segement Attribute */
#define QD_DISK_TABLE_DEFAULT_PCTFREE                   (QCU_PROPERTY(mPctFree))
#define QD_DISK_TABLE_DEFAULT_PCTUSED                   (QCU_PROPERTY(mPctUsed))
#define QD_DISK_TABLE_DEFAULT_INITRANS                  (QCU_PROPERTY(mTableInitTrans))
#define QD_DISK_TABLE_DEFAULT_MAXTRANS                  (QCU_PROPERTY(mTableMaxTrans))
/* Disk Index Segement Attribute */
#define QD_DISK_INDEX_DEFAULT_INITRANS                  (QCU_PROPERTY(mIndexInitTrans))
#define QD_DISK_INDEX_DEFAULT_MAXTRANS                  (QCU_PROPERTY(mIndexMaxTrans))
/* Invalid Segement Attribute */
#define QD_INVALID_TRANS_VALUE                          (0xFFFF)
#define QD_INVALID_PCT_VALUE                            (0xFFFF)

//PROJ-1671 
#define QD_MEMORY_SEGMENT_DEFAULT_STORAGE_INITEXTENTS   (0)
#define QD_MEMORY_SEGMENT_DEFAULT_STORAGE_NEXTEXTENTS   (0)
#define QD_MEMORY_SEGMENT_DEFAULT_STORAGE_MINEXTENTS    (0)
#define QD_MEMORY_SEGMENT_DEFAULT_STORAGE_MAXEXTENTS    (0)
#define QD_DISK_TABLESPACE_DEFAULT_SEGMENT_MGMT_TYPE    ((smiSegMgmtType)QCU_PROPERTY(mSegMgmtType))
#define QD_DISK_TABLESPACE_DEFAULT_EXTENT_MGMT_TYPE     (SMI_EXTENT_MGMT_BITMAP_TYPE)
#define QD_DISK_SEGMENT_DEFAULT_STORAGE_INITEXTENTS     (QCU_PROPERTY(mSegStoInitExtCnt))
#define QD_DISK_SEGMENT_DEFAULT_STORAGE_NEXTEXTENTS     (QCU_PROPERTY(mSegStoNextExtCnt))
#define QD_DISK_SEGMENT_DEFAULT_STORAGE_MINEXTENTS      (QCU_PROPERTY(mSegStoMinExtCnt))
#define QD_DISK_SEGMENT_DEFAULT_STORAGE_MAXEXTENTS      (QCU_PROPERTY(mSegStoMaxExtCnt))
#define QD_INVALID_SEGMENT_STORAGE_VALUE                (0)

#define QD_INDEX_DEFAULT_PARALLEL_DEGREE 0


// BUG-29728 referenced options flags
// flag의 앞은 세자리는 DML 구분용으로 중복 체크 해주고,
// 뒤의 세자리는 옵션 구분을 위해서 사용된다
// 기존 Meta 정보를 수정하지 않고 버그를 수정하고,,
// 중복 체크를 위해(이미 사용된 DML의 option 구분)을 위해
// 두가지 정보를 한 flag에 넣어줌.

#define QD_FOREIGN_INIT                  (0x00000000)
#define QD_FOREIGN_DML_MASK              (0x0000F000)
#define QD_FOREIGN_OPTION_MASK           (0x00000FFF)

#define QD_FOREIGN_DELETE_MASK           (0x0000100F)
#define QD_FOREIGN_DELETE_NO_ACTION      (0x00001000)
#define QD_FOREIGN_DELETE_CASCADE        (0x00001001)
#define QD_FOREIGN_DELETE_SET_NULL       (0x00001002)
//#define QD_FOREIGN_DELETE_RESTIRCT     (0x00001003)
//#define QD_FOREIGN_DELETE_SET_DEFAULT  (0x00001004)
 
 
#define QD_FOREIGN_INSERT_MASK           (0x000020F0)
#define QD_FOREIGN_INSERT_NO_ACTION      (0x00002000)
//#define QD_FOREIGN_INSERT_CASCADE      (0x00002010)
//#define QD_FOREIGN_INSERT_RESTIRCT     (0x00002020)
//#define QD_FOREIGN_INSERT_SET_NULL     (0x00002030)
//#define QD_FOREIGN_INSERT_SET_DEFAULT  (0x00002040)
 
#define QD_FOREIGN_UPDATE_MASK           (0x00004F00)
#define QD_FOREIGN_UPDATE_NO_ACTION      (0x00004000)
//#define QD_FOREIGN_UPDATE_CASCADE      (0x00004100)
//#define QD_FOREIGN_UPDATE_RESTIRCT     (0x00004200)
//#define QD_FOREIGN_UPDATE_SET_NULL     (0x00004300)
//#define QD_FOREIGN_UPDATE_SET_DEFAULT  (0x00004400)

// BUG-26361
// sequence min/max value
// QDS_SEQUENCE_MIN_VALUE = -((SLong)(ID_ULONG_MAX/2 -1))
// QDS_SEQUENCE_MAX_VALUE = ((SLong)(ID_ULONG_MAX/2 -1))
#define QDS_SEQUENCE_MIN_VALUE          ((SLong)ID_LONG(-9223372036854775806))
#define QDS_SEQUENCE_MAX_VALUE          ((SLong)ID_LONG(9223372036854775806))
#define QDS_SEQUENCE_MIN_VALUE_STR      (UChar*)"-9223372036854775806"
#define QDS_SEQUENCE_MAX_VALUE_STR      (UChar*)"9223372036854775806"

/* TASK-7217 Sharded sequence */
#define QDS_SHARD_SEQUENCE_MIN_VALUE     ((SLong)ID_LONG(-999999999999999))
#define QDS_SHARD_SEQUENCE_MAX_VALUE     ((SLong)ID_LONG(999999999999999))
#define QDS_SHARD_SEQUENCE_MIN_VALUE_STR (UChar*)"-999999999999999"
#define QDS_SHARD_SEQUENCE_MAX_VALUE_STR (UChar*)"999999999999999"

// BUG-38852
// qdSynonymParseTree->flag   FOR CREATE SYNONYM
#define QDS_SYN_OPT_REPLACE_MASK        (0x00000001)
#define QDS_SYN_OPT_REPLACE_FALSE       (0x00000000)
#define QDS_SYN_OPT_REPLACE_TRUE        (0x00000001)

// qdTableParseTree->flag   FOR CREATE VIEW
#define QDV_OPT_REPLACE_MASK            (0x00000001)
#define QDV_OPT_REPLACE_FALSE           (0x00000000)
#define QDV_OPT_REPLACE_TRUE            (0x00000001)

// qdTableParseTree->flag   FOR CREATE VIEW
#define QDV_OPT_FORCE_MASK              (0x00000002)
#define QDV_OPT_FORCE_FALSE             (0x00000000)
#define QDV_OPT_FORCE_TRUE              (0x00000002)

// qdTableParseTree->flag   FOR CREATE VIEW
#define QDV_OPT_STATUS_MASK             (0x00000004)
#define QDV_OPT_STATUS_VALID            (0x00000000)
#define QDV_OPT_STATUS_INVALID          (0x00000004)

// qdTableParseTree->flag   FOR CREATE VIEW
#define QDV_VIEW_DDL_MASK               (0x00000008)
#define QDV_VIEW_DDL_FALSE              (0x00000000)
#define QDV_VIEW_DDL_TRUE               (0x00000008)

// Proj-1360 queue
// qdTableParseTree->flag   FOR CREATE QUEUE / ALTER QUEUE / DROP QUEUE
#define QDQ_QUEUE_MASK                  (0x00000010)
#define QDQ_QUEUE_FALSE                 (0x00000000)
#define QDQ_QUEUE_TRUE                  (0x00000010)

// PROJ-1407 Temporary Table
// qdTableParseTree->flag   FOR CREATE TEMPORARY TABLE
#define QDT_CREATE_TEMPORARY_MASK       (0x00000020)
#define QDT_CREATE_TEMPORARY_FALSE      (0x00000000)
#define QDT_CREATE_TEMPORARY_TRUE       (0x00000020)

/* qdTableParseTree->flag   FOR CREATE MATERIALIZED VIEW */
#define QDV_MVIEW_TABLE_DDL_MASK        (0x00000040)
#define QDV_MVIEW_TABLE_DDL_FALSE       (0x00000000)
#define QDV_MVIEW_TABLE_DDL_TRUE        (0x00000040)

#define QDV_MVIEW_VIEW_DDL_MASK         (0x00000080)
#define QDV_MVIEW_VIEW_DDL_FALSE        (0x00000000)
#define QDV_MVIEW_VIEW_DDL_TRUE         (0x00000080)

// PROJ-1624 global non-partitioned index
/* BUG-35460 Add TABLE_TYPE G in SYS_TABLES_ */
#define QDV_HIDDEN_INDEX_TABLE_MASK     (0x00000100)
#define QDV_HIDDEN_INDEX_TABLE_FALSE    (0x00000000)
#define QDV_HIDDEN_INDEX_TABLE_TRUE     (0x00000100)

/* BUG-36350 Updatable Join DML WITH READ ONLY
 * qdTableParseTree->flag   WITH READ ONLY */
#define QDV_OPT_WITH_READ_ONLY_MASK      (0x00000200)
#define QDV_OPT_WITH_READ_ONLY_FALSE     (0x00000000)
#define QDV_OPT_WITH_READ_ONLY_TRUE      (0x00000200)

// PROJ-2264 Dictionary Table
// BUG-37144 QDV_HIDDEN_DICTIONARY_TABLE_TRUE add
// qdTableParseTree->flag
#define QDV_HIDDEN_DICTIONARY_TABLE_MASK     (0x00000400)
#define QDV_HIDDEN_DICTIONARY_TABLE_FALSE    (0x00000000)
#define QDV_HIDDEN_DICTIONARY_TABLE_TRUE     (0x00000400)

// PROJ-2365 sequence table
#define QDV_HIDDEN_SEQUENCE_TABLE_MASK   (0x00000800)
#define QDV_HIDDEN_SEQUENCE_TABLE_FALSE  (0x00000000)
#define QDV_HIDDEN_SEQUENCE_TABLE_TRUE   (0x00000800)

// qdSequenceOptions->flag
#define QDS_SEQ_OPT_NOMIN_MASK           (0x00000001)
#define QDS_SEQ_OPT_NOMIN_FALSE          (0x00000000)
#define QDS_SEQ_OPT_NOMIN_TRUE           (0x00000001)

// qdSequenceOptions->flag
#define QDS_SEQ_OPT_NOMAX_MASK           (0x00000002)
#define QDS_SEQ_OPT_NOMAX_FALSE          (0x00000000)
#define QDS_SEQ_OPT_NOMAX_TRUE           (0x00000002)

/* TASK-7217 Sharded sequence */
// qdSequenceOptions->flag
#define QDS_SEQ_OPT_RESTART_MASK         (0x00000004)
#define QDS_SEQ_OPT_RESTART_FALSE        (0x00000000)
#define QDS_SEQ_OPT_RESTART_TRUE         (0x00000004)

/* TASK-7217 Sharded sequence */
// qdSequenceOptions->flag
#define QDS_SEQ_OPT_LOCALITY_MASK        SMI_SEQUENCE_LOCALITY_MASK     // (0x00000700)
#define QDS_SEQ_OPT_LOCALITY_LOCAL       SMI_SEQUENCE_LOCALITY_LOCAL    // (0x00000100)
#define QDS_SEQ_OPT_LOCALITY_SHARD       SMI_SEQUENCE_LOCALITY_SHARD    // (0x00000200)
#define QDS_SEQ_OPT_LOCALITY_GLOBAL      SMI_SEQUENCE_LOCALITY_GLOBAL   // (0x00000400) UNUSED

/* TASK-7217 Sharded sequence */
#define QDS_SEQ_OPT_SCALE_FIXED_MASK     SMI_SEQUENCE_SCALE_FIXED_MASK  // (0x00001000)
#define QDS_SEQ_OPT_SCALE_FIXED_FALSE    SMI_SEQUENCE_SCALE_FIXED_FALSE // (0x00000000) VARIABLE
#define QDS_SEQ_OPT_SCALE_FIXED_TRUE     SMI_SEQUENCE_SCALE_FIXED_TRUE  // (0x00001000)

// qdIndexParseTree->flag
#define QDX_IDX_OPT_PERSISTENT_MASK      SMI_INDEX_PERSISTENT_MASK
#define QDX_IDX_OPT_PERSISTENT_FALSE     SMI_INDEX_PERSISTENT_DISABLE
#define QDX_IDX_OPT_PERSISTENT_TRUE      SMI_INDEX_PERSISTENT_ENABLE

// BUG-10518 관련하여, 아래 flag는 쓰이지 않음.
// qdIndexParseTree->flag
#define QDX_IDX_OPT_DISABLE_MASK         SMI_INDEX_USE_MASK
#define QDX_IDX_OPT_DISABLE_FALSE        SMI_INDEX_USE_ENABLE
#define QDX_IDX_OPT_DISABLE_TRUE         SMI_INDEX_USE_DISABLE

// qdDropTBSParseTree->flag INCLUDING CONTENTS
#define QDT_DROP_INCLUDING_CONTENTS_MASK   (0x00000001)
#define QDT_DROP_INCLUDING_CONTENTS_FALSE  (0x00000000)
#define QDT_DROP_INCLUDING_CONTENTS_TRUE   (0x00000001)

// qdDropTBSParseTree->flag AND DATAFILES
#define QDT_DROP_AND_DATAFILES_MASK        (0x00000002)
#define QDT_DROP_AND_DATAFILES_FALSE       (0x00000000)
#define QDT_DROP_AND_DATAFILES_TRUE        (0x00000002)

// qdDropTBSParseTree->flag CASCADE CONSTRAINTS
#define QDT_DROP_CASCADE_CONSTRAINTS_MASK  (0x00000004)
#define QDT_DROP_CASCADE_CONSTRAINTS_FALSE (0x00000000)
#define QDT_DROP_CASCADE_CONSTRAINTS_TRUE  (0x00000004)

// PROJ-1509
// qdDropParseTree->flag
// drop table ... cascade constraints 추가
#define QDD_DROP_CASCADE_CONSTRAINTS_MASK  (0x00000001)
#define QDD_DROP_CASCADE_CONSTRAINTS_FALSE (0x00000000)
#define QDD_DROP_CASCADE_CONSTRAINTS_TRUE  (0x00000001)

/* PROJ-1812 ROLE
 * current userid, public 를 제외 한 것이 role 최대 부여 개수 (126) */
#define QDD_USER_TO_ROLES_MAX_COUNT        (128)

/* PROJ-2626 Snapshot Export */
#define QD_ALTER_DATABASE_SNAPSHOT_BEGIN   (1)
#define QD_ALTER_DATABASE_SNAPSHOT_END     (2)

/* BUG-47599 */
#define QD_EMPTY_PARTITION_NAME            ((SChar*)"")
#define QD_EMPTY_PARTITION_NAME_SIZE       (0)

/* PROJ-2207 Password policy support */
enum qdPasswPolicy
{
    QD_PASSWORD_POLICY_DISABLE = 0, //"F"
    QD_PASSWORD_POLICY_ENABLE       //"T"
};

enum qdAccountStatus
{
    QD_ACCOUNT_UNLOCK = 0,        //"N"
    QD_ACCOUNT_LOCK               //"L"
};
    
#define QD_PASSWORD_POLICY_START_DATE        "01-JAN-2001"
#define QD_PASSWORD_POLICY_START_DATE_FORMAT "DD-MON-RRRR"

#define QD_PASSWORD_POLICY_START_DATE_YEAR    (2001)
#define QD_PASSWORD_POLICY_START_DATE_MON     (1)
#define QD_PASSWORD_POLICY_START_DATE_DAY     (1)

enum qdConstraintType
{
    QD_FOREIGN = 0,
    QD_NOT_NULL,
    QD_UNIQUE,
    QD_PRIMARYKEY,
    QD_NULL,
    QD_TIMESTAMP,
    QD_LOCAL_UNIQUE, // PROJ-1502 PARTITIONED DISK TABLE
    QD_CHECK,       /* PROJ-1107 Check Constraint 지원 */
    QD_CONSTR_MAX
};

// PROJ-1502 PARTITIONED DISK TABLE
enum qdUniqueIndexType
{
    QD_GLOBAL_UNIQUE_INDEX = 0,
    QD_LOCAL_UNIQUE_INDEX,
    QD_NONE_UNIQUE_INDEX
};

// PROJ-1362
enum qdLobStorageAttrType
{
    QD_LOB_STORAGE_ATTR_TABLESPACE = 0,
    QD_LOB_STORAGE_ATTR_LOGGING,
    QD_LOB_STORAGE_ATTR_BUFFER
};

// PROJ-1502 PARTITIONED DISK TABLE
enum qdPartValuesType
{
    QD_RANGE_VALUES_TYPE = 0,
    QD_HASH_VALUES_TYPE,
    QD_LIST_VALUES_TYPE,
    QD_DEFAULT_VALUES_TYPE,
    QD_NONE_VALUES_TYPE
};

// PROJ-1502 PARTITIONED DISK TABLE
enum qdAlterPartitionType
{
    QD_ADD_HASH_PARTITION = 0,
    QD_ADD_RANGE_PARTITION,
    QD_COALESCE_PARTITION,
    QD_DROP_PARTITION,
    QD_MERGE_PARTITION,
    QD_RENAME_PARTITION,
    QD_SPLIT_RANGE_PARTITION,
    QD_SPLIT_LIST_PARTITION,
    QD_TRUNCATE_PARTITION,
    QD_ENABLE_ROW_MOVEMENT,
    QD_DISABLE_ROW_MOVEMENT,
    QD_ACCESS_PARTITION,        /* PROJ-2359 Table/Partition Access Option */
    QD_ALTER_PARTITION,         /* PROJ-2464 hybrid partitioned table 지원 */
    QD_USABLE_PARTITION,        /* TASK-7307 DML Data Consistency in Shard */
    QD_NONE_ALTER_PARTITION
};

// PROJ-1502 PARTITIONED DISK TABLE
enum qdSplitMergeType
{
    QD_ALTER_PARTITION_LEFT_INPLACE_TYPE = 0,
    QD_ALTER_PARTITION_RIGHT_INPLACE_TYPE,
    QD_ALTER_PARTITION_OUTPLACE_TYPE
};

/* PROJ-2207 Password policy support */
enum qdPasswOptionName
{
    QD_FAILED_LOGIN_ATTEMPTS = 0,
    QD_PASSWORD_LIFE_TIME,
    QD_PASSWORD_REUSE_TIME,
    QD_PASSWORD_REUSE_MAX,
    QD_PASSWORD_LOCK_TIME,
    QD_PASSWORD_GRACE_TIME,
    QD_PASSWORD_VERIFY_FUNCTION
};

/* PROJ-2474 TLS/SSL Support */
enum qdDisableTCP
{
    QD_DISABLE_TCP_NONE = 0,
    QD_DISABLE_TCP_FALSE,
    QD_DISABLE_TCP_TRUE
};

enum qdExpLockStatus
{
    QD_NONE_LOCK = 0,
    QD_EXPLICITILY_LOCK,
    QD_EXPLICITILY_UNLOCK
};

/* PROJ-2211 Materialized View */
enum qdMViewBuildType
{
    QD_MVIEW_BUILD_NONE = 0,
    QD_MVIEW_BUILD_IMMEDIATE,
    QD_MVIEW_BUILD_DEFERRED
};
enum qdMViewRefreshType
{
    QD_MVIEW_REFRESH_NONE = 0,
    QD_MVIEW_REFRESH_FORCE,
    QD_MVIEW_REFRESH_COMPLETE,
    QD_MVIEW_REFRESH_FAST,
    QD_MVIEW_REFRESH_NEVER
};
enum qdMViewRefreshTime
{
    QD_MVIEW_REFRESH_ON_NONE = 0,
    QD_MVIEW_REFRESH_ON_DEMAND,
    QD_MVIEW_REFRESH_ON_COMMIT
};

typedef struct qdLobStorageAttribute
{
    qdLobStorageAttrType      type;
    qcNamePosition            TBSName;
    smiTableSpaceAttr         TBSAttr;
    idBool                    logging;
    idBool                    buffer;
    qdLobStorageAttribute   * next;
} qdLobStorageAttribute;

typedef struct qdLobAttribute
{
    qcmColumn                * columns;
    qdLobStorageAttribute    * storageAttr;
    qdLobAttribute           * next;
} qdLobAttribute;

// PROJ-1671 Bitmap Segment
typedef struct qdSegStorageAttr  // temporary struct for parser
{
    // "HIGH" 혹은 "LOW" Identifier의 Position
    qcNamePosition      attrPosition;
    
    UInt              * initExtCnt;
    UInt              * nextExtCnt;
    UInt              * minExtCnt;
    UInt              * maxExtCnt;
    qdSegStorageAttr  * next;
    
} qdSegStorageAttr;

// PROJ-2002 Column Security
typedef struct qdExtColumnAttr  // temporary struct for parser
{
    qcNamePosition      mPolicyPosition;
    SLong               mSrid;
} qdExtColumnAttr;

#define QD_SET_INIT_EXT_COLUMN_ATTR(_col_attr_)         \
{                                                       \
    SET_EMPTY_POSITION( _col_attr_->mPolicyPosition );  \
    _col_attr_->mSrid = 0 ;                             \
}

// PROJ-1502 PARTITIONED DISK TABLE
// for alter table split, merge, add partition
// 새로 생성되는 인덱스 파티션에 대한 정보를 설정한다.
// 현재 존재하는 인덱스의 개수에 따라 여러개가 올 수 있다.
typedef struct qdIndexPartitionAttribute
{
    qcNamePosition             partIndexName;
    qcNamePosition             indexPartName;
    qcNamePosition             TBSName;
    smiTableSpaceAttr          TBSAttr;
    qdIndexPartitionAttribute* next;
} qdIndexPartitionAttribute;

// PROJ-1502 PARTITIONED DISK TABLE
#define QD_SET_INIT_INDEX_PART_ATTR(_index_part_attr_)                                          \
    {                                                                                           \
        SET_EMPTY_POSITION( _index_part_attr_->partIndexName );                                 \
        SET_EMPTY_POSITION( _index_part_attr_->indexPartName );                                 \
        SET_EMPTY_POSITION( _index_part_attr_->TBSName);                                        \
        idlOS::memset( & _index_part_attr_->TBSAttr, 0x00, ID_SIZEOF(smiTableSpaceAttr) );      \
        _index_part_attr_->next = NULL;                                                         \
    }

// PROJ-1502 PARTITIONED DISK TABLE
typedef struct qdAlterPartition
{
    qdAlterPartitionType       alterType;
    qmsPartCondValList       * splitCondVal;
    qdSplitMergeType           splitMergeType;
    idBool                     isLeftPartIsLess; // for merge partition

    mtdCharType              * partKeyCondMinValStr;
    struct qmmValueNode      * partKeyCondMin;
    SChar                    * partKeyCondMinStmtText;
    struct qmsPartCondValList* partCondMinVal;

    mtdCharType              * partKeyCondMaxValStr;
    struct qmmValueNode      * partKeyCondMax;
    SChar                    * partKeyCondMaxStmtText;
    struct qmsPartCondValList* partCondMaxVal;
    qdIndexPartitionAttribute* indexPartAttr;
} qdAlterPartition;

// PROJ-1502 PARTITIONED DISK TABLE
#define QD_SET_INIT_ALTER_PART(_alter_part_)                                    \
    {                                                                           \
        _alter_part_->alterType = QD_NONE_ALTER_PARTITION;                      \
        _alter_part_->splitCondVal = NULL;                                      \
        _alter_part_->splitMergeType = QD_ALTER_PARTITION_OUTPLACE_TYPE;        \
        _alter_part_->isLeftPartIsLess = ID_TRUE;                               \
        _alter_part_->partKeyCondMinValStr = NULL;                              \
        _alter_part_->partKeyCondMin = NULL;                                    \
        _alter_part_->partKeyCondMinStmtText = NULL;                            \
        _alter_part_->partCondMinVal = NULL;                                    \
        _alter_part_->partKeyCondMaxValStr = NULL;                              \
        _alter_part_->partKeyCondMax = NULL;                                    \
        _alter_part_->partKeyCondMaxStmtText = NULL;                            \
        _alter_part_->partCondMaxVal = NULL;                                    \
        _alter_part_->indexPartAttr = NULL;                                     \
    }

// PROJ-1502 PARTITIONED DISK TABLE
typedef struct qdPartitionAttribute
{
    qcNamePosition             tablePartName;
    SChar                    * tablePartNameStr;
    qcNamePosition             indexPartName;
    SChar                    * indexPartNameStr;
    qcNamePosition             TBSName;
    smiTableSpaceAttr          TBSAttr;
    qdPartitionAttribute     * next;
    qcmColumn                * columns;
    struct qmmValueNode      * partKeyCond;
    struct qmsPartCondValList  partCondVal;
    UInt                       partOrder;
    qdPartValuesType           partValuesType;
    qdLobAttribute           * lobAttr;
    qdAlterPartition         * alterPart;
    UInt                       indexPartID;

    // PROJ-1579 NCHAR
    struct qcNamePosList     * ncharLiteralPos;

    /* PROJ-2359 Table/Partition Access Option */
    qcmAccessOption            accessOption;

    /* PROJ-1810 Partition Exchange */
    qcNamePosition             oldTableName;    //JOIN TABLE 대상 테이블(원래 있던)의 이름
    SChar                    * oldTableNameStr;

    /* TASK-7307 DML Data Consistency in Shard */
    idBool                     mIsUsable;
} qdPartitionAttribute;

// PROJ-1502 PARTITIONED DISK TABLE
#define QD_SET_INIT_PARTITION_ATTR(_part_attr_)                                                 \
    {                                                                                           \
        SET_EMPTY_POSITION( _part_attr_->tablePartName );                                       \
        _part_attr_->tablePartNameStr = NULL;                                                   \
        SET_EMPTY_POSITION( _part_attr_->indexPartName );                                       \
        _part_attr_->indexPartNameStr = NULL;                                                   \
        SET_EMPTY_POSITION( _part_attr_->TBSName );                                             \
        idlOS::memset( & _part_attr_->TBSAttr, 0x00, ID_SIZEOF(smiTableSpaceAttr) );            \
        _part_attr_->next = NULL;                                                               \
        _part_attr_->columns = NULL;                                                            \
        _part_attr_->partKeyCond = NULL;                                                        \
        idlOS::memset( & _part_attr_->partCondVal, 0x00, ID_SIZEOF(qmsPartCondValList) );       \
        _part_attr_->partOrder = QDB_NO_PARTITION_ORDER;                                        \
        _part_attr_->partValuesType = QD_NONE_VALUES_TYPE;                                      \
        _part_attr_->lobAttr = NULL;                                                            \
        _part_attr_->alterPart = NULL;                                                          \
        _part_attr_->ncharLiteralPos = NULL;                                                    \
        _part_attr_->accessOption = QCM_ACCESS_OPTION_NONE;                                     \
        SET_EMPTY_POSITION( _part_attr_->oldTableName );                                        \
        _part_attr_->oldTableNameStr = NULL;                                                    \
        _part_attr_->mIsUsable = ID_TRUE;                                                       \
    }

// PROJ-1502 PARTITIONED DISK TABLE
typedef struct qdPartitionedTable
{
    qcmColumn                   * partKeyColumns;
    qcmPartitionMethod            partMethod;
    UInt                          partCount;
    qdPartitionAttribute        * partAttr;
    struct qcmPartitionInfoList * partInfoList;
} qdPartitionedTable;

// PROJ-1502 PARTITIONED DISK TABLE
#define QD_SET_INIT_PART_TABLE(_part_table_)                    \
    {                                                           \
        _part_table_->partKeyColumns = NULL;                    \
        _part_table_->partMethod = QCM_PARTITION_METHOD_NONE;   \
        _part_table_->partCount = 0;                            \
        _part_table_->partAttr = NULL;                          \
        _part_table_->partInfoList = NULL;                      \
    }

/* Create/Alter Table구문에서 Table의 Flag를 설정하는데 사용 */
typedef struct qdTableAttrFlagList
{
    UInt    attrMask;
    UInt    attrValue;

    // SQL TEXT내에서의 이 Attiribute에 해당되는 Offset과 Size
    // Validation중 특정 Attribute를 찍어서 에러를 내고자 할 때 사용
    qcNamePosition attrPosition;
    
    // Linked List의 Next포인터
    qdTableAttrFlagList * next;
} qdTableAttrFlagList;

// PROJ-1407 Temporary Table
typedef struct qdTemporaryOption
{
    qcNamePosition             temporaryPos;
    qcmTemporaryType           temporaryType;
} qdTemporaryOption;

/* PROJ-2211 Materialized View */
typedef struct qdMViewBuildRefresh
{
    qdMViewBuildType   buildType;
    qdMViewRefreshType refreshType;
    qdMViewRefreshTime refreshTime;
} qdMViewBuildRefresh;

// PROJ-1624 global non-partitioned index
typedef struct qdIndexTableList
{
    UInt               tableID;
    smOID              tableOID;
    qcmTableInfo     * tableInfo;
    void             * tableHandle;
    smSCN              tableSCN;
    qdIndexTableList * next;
} qdIndexTableList;

/* PROJ-2600 Online DDL for Tablespace Alteration */
typedef struct qdPartitionedTableList
{
    qcmTableInfo                  * mTableInfo;
    void                          * mTableHandle;
    smSCN                           mTableSCN;
    qcmPartitionInfoList          * mPartInfoList;
    qdIndexTableList              * mIndexTableList;
    struct qdPartitionedTableList * mNext;
} qdPartitionedTableList;

/* PROJ-2600 Online DDL for Tablespace Alteration */
#define QD_SET_INIT_PART_TABLE_LIST( _dst_ )                    \
    {                                                           \
        (_dst_)->mTableInfo      = NULL;                        \
        (_dst_)->mTableHandle    = NULL;                        \
        SMI_INIT_SCN( & (_dst_)->mTableSCN );                   \
        (_dst_)->mPartInfoList   = NULL;                        \
        (_dst_)->mIndexTableList = NULL;                        \
        (_dst_)->mNext           = NULL;                        \
    }

/* BUG-35445 Check Constraint, Function-Based Index에서 사용 중인 Function을 변경/제거 방지 */
typedef struct qdFunctionNameList
{
    UInt                 userID;
    qcNamePosition       functionName;

    qdFunctionNameList * next;
} qdFunctionNameList;

/* PROJ-1810 Partition Exchange */
typedef struct qdDisjoinTable
{
    /* set in parser */
    /* 기존 파티션의 이름, ID와 이에 대응되는 새 이름, ID를 저장 */
    qcNamePosition         oldPartName;     // old; 구문에서 지정한 파티션 이름
    qcNamePosition         newTableName;    // new; 새로 변환되는 테이블 이름

    /* set in validation */
    UInt                   oldPartID;       // 파티션 ID
    smOID                  oldPartOID;      // 테이블 OID(원래 파티션 OID)
    qcmTableInfo         * oldPartInfo;     // 파티션 info

    /* set in execution */
    /* 메타에서 새롭게 저장하는 부분 */
    UInt                   newTableID;      // new 테이블 ID

    /* next ptr */
    qdDisjoinTable       * next;
} qdDisjoinTable;

#define QD_SET_INIT_DISJOIN_TABLE( _dst_ )              \
{                                                       \
    SET_EMPTY_POSITION( (_dst_)->oldPartName );         \
    SET_EMPTY_POSITION( (_dst_)->newTableName );        \
    (_dst_)->oldPartInfo = NULL;                        \
    (_dst_)->next = NULL;                               \
}

/* PROJ-2464 hybrid partitioned table 지원 */
typedef struct qdSegStoAttrExist
{
    idBool mInitExt;
    idBool mNextExt;
    idBool mMinExt;
    idBool mMaxExt;
} qdSegStoAttrExist;

// CREATE TABLE
// ALTER TABLE ADD COL, DROP COL(drop & create), RENAME COL
// ALTER TABLE RENAME TABLE
// ALTER TABLE ADD CONSTRAINT
// ALTER TABLE DROP CONSTRAINT
// CREATE VIEW
// ALTER VIEW
// FLASHBACK TABLE table TO BEFORE DROP (RENAME TO ntable)
// JOIN TABLE table PARTITION BY RANGE/LIST(...) (TABLE ... TO PARTITION ..., ...)
typedef struct qdTableParseTree
{
    qcParseTree                common;

    qcNamePosition             userName;
    qcNamePosition             tableName;
    UInt                       userID;

    qcNamePosition             newTableName;

    struct qdConstraintSpec  * constraints;
    qcmColumn                * columns;
    qcmColumn                * modifyColumns;  // PROJ-1877 alter table modify column
    qmsFrom                  * from;           /* PROJ-1107 Check Constraint 지원 */

    qcStatement              * select;  // CREATE TABLE ... AS SELECT ...
                                        // CREATE VIEW AS SELECT

    ULong                      maxrows; // default value = 0
    UInt                       flag;    // for CREATE VIEW

    // for A4
    qcNamePosition             TBSName;
    smiTableSpaceAttr          TBSAttr;

    // set in processing tablespace validation

    // PRJ-1671 Bitmap TableSpace And Segment Space Management
    // smiSegAttr 자료구조    ( insert high/low limit, init/max trans )
    smiSegAttr                 segAttr;

    // smiSegStoAttr 자료구조 ( initextents, nextextents,
    //                          minextents, maxextents )
    smiSegStorageAttr          segStoAttr;

    // alter table allocate extent ( size .. );
    ULong                      altAllocExtSize;

    /* PROJ-2465 Tablespace Alteration for Table */
    qdIndexPartitionAttribute* indexTBSAttr;

    // PROJ-1362
    qdLobAttribute           * lobAttr;

    // for ALTER TABLE
    qcmTableInfo             * tableInfo; // set in validation

    // TASK-2176
    void                     * tableHandle;
    smSCN                      tableSCN;

    // PROJ-1502 PARTITIONED DISK TABLE
    qdPartitionedTable       * partTable;
    idBool                     isRowmovement;

    // TASK-2398 Log Compression
    // Create/Alter Table구문을 통해 Table의 Flag설정/변경
    // BUG-49063 Create/Alter Queue구문을 통해 Table의 Flag설정/변경
    qdTableAttrFlagList      * tableAttrFlagList;

    // 하나 이상의 Attribute Flag들의 Mask와 Value를 OR한 값
    // Create Table의 경우 사용되며, Validation단계에서 설정한다.
    UInt                       tableAttrMask;
    UInt                       tableAttrValue;

    // PROJ-1665
    UInt                     * loggingMode;
    UInt                       parallelDegree;

    // PROJ-1407 Temporary Table
    qdTemporaryOption        * temporaryOption;
    
    // PROJ-1723 [MDW/INTEGRATOR] Altibase Plugin 개발
    // ID_TRUE  : ADD SUPPLEMENTAL LOG
    // ID_FALSE : DROP SUPPLEMENTAL LOG
    idBool                     isSuppLogging;

    // BUG-21761
    // N타입을 U타입으로 변형시킬 때 사용
    qcNamePosList            * ncharList;

    /* PROJ-2211 Materialized View */
    SChar                      mviewViewName[QC_MAX_OBJECT_NAME_LEN + 1];
    qcmColumn                * mviewViewColumns;
    qdMViewBuildRefresh        mviewBuildRefresh;

    // PROJ-1624 global non-partitioned index
    qdIndexTableList         * oldIndexTables;
    qdIndexTableList         * newIndexTables;
    
    /* PROJ-1090 Function-based Index */
    struct qdIndexParseTree  * createIndexParseTree;
    idBool                     addHiddenColumn;  /* create index의 의미를 포함한다. */
    idBool                     dropHiddenColumn; /* drop index의 의미를 포함한다. */
    qcmColumn                * defaultExprColumns;

    // PROJ-2264 Dictionary table
    qcmCompressionColumn     * compressionColumn;

    /* BUG-35445 Check Constraint, Function-Based Index에서 사용 중인 Function을 변경/제거 방지 */
    qdFunctionNameList       * relatedFunctionNames;

    /* PROJ-2359 Table/Partition Access Option */
    qcmAccessOption            accessOption;

    /* PROJ-2441 flashback  */
    idBool                     useOriginalName;

    /* PROJ-2464 hybrid partitioned table 지원 */
    qdSegStoAttrExist          existSegStoAttr;

    // BUG-45745
    qdPartitionAttribute     * mPartAttr;
    
    /* PROJ-2600 Online DDL for Tablespace Alteration */
    qcNamePosition             mSourceUserName;
    qcNamePosition             mSourceTableName;
    qcNamePosition             mNamesPrefix;
    idBool                     mIgnoreForeignKeyChild;
    idBool                     mIsRenameForce;

    qdPartitionedTableList   * mSourcePartTable;        // Source Partitioned Table Info
    qcmRefChildInfo          * mSourceRefChildInfoList; // Child Info(having Parent Index) List of Source
    qcmRefChildInfo          * mTargetRefChildInfoList; // Child Info(having Parent Index) List of Target
    qdPartitionedTableList   * mRefChildPartTableList;  // Child Partitioned Table Info List for Lock and Meta Cache

    /* TASK-7307 DML Data Consistency in Shard */
    idBool                     mIsUsable;
    qcmShardFlag               mShardFlag;
} qdTableParseTree;

#define QD_SEGMENT_OPTION_INIT( _dst_segAttr_, _dst_segStoAttr )                  \
{                                                                                 \
    (_dst_segAttr_)->mPctFree      = QD_INVALID_PCT_VALUE;                        \
    (_dst_segAttr_)->mPctUsed      = QD_INVALID_PCT_VALUE;                        \
    (_dst_segAttr_)->mInitTrans    = QD_INVALID_TRANS_VALUE;                      \
    (_dst_segAttr_)->mMaxTrans     = QD_INVALID_TRANS_VALUE;                      \
    (_dst_segStoAttr)->mInitExtCnt = QD_DISK_SEGMENT_DEFAULT_STORAGE_INITEXTENTS; \
    (_dst_segStoAttr)->mNextExtCnt = QD_DISK_SEGMENT_DEFAULT_STORAGE_INITEXTENTS; \
    (_dst_segStoAttr)->mMinExtCnt  = QD_DISK_SEGMENT_DEFAULT_STORAGE_MINEXTENTS;  \
    (_dst_segStoAttr)->mMaxExtCnt  = QD_DISK_SEGMENT_DEFAULT_STORAGE_MAXEXTENTS;  \
}

#define QD_SEGMENT_STORAGE_EXIST_INIT( _dst_ ) \
{                                              \
    (_dst_)->mInitExt = ID_FALSE;              \
    (_dst_)->mNextExt = ID_FALSE;              \
    (_dst_)->mMinExt  = ID_FALSE;              \
    (_dst_)->mMaxExt  = ID_FALSE;              \
}

// To Fix PR-8532
#define QD_TABLE_PARSE_TREE_INIT( _dst_ )                                   \
{                                                                           \
    SET_EMPTY_POSITION( (_dst_)->userName );                                \
    SET_EMPTY_POSITION( (_dst_)->tableName );                               \
    (_dst_)->userID = QC_EMPTY_USER_ID;                                     \
                                                                            \
    SET_EMPTY_POSITION( (_dst_)->newTableName );                            \
                                                                            \
    (_dst_)->constraints = NULL;                                            \
    (_dst_)->columns = NULL;                                                \
    (_dst_)->modifyColumns = NULL;                                          \
    (_dst_)->from = NULL;                                                   \
                                                                            \
    (_dst_)->select = NULL;                                                 \
                                                                            \
    (_dst_)->maxrows = ID_ULONG_MAX;                                        \
    (_dst_)->flag = 0;                                                      \
                                                                            \
    SET_EMPTY_POSITION( (_dst_)->TBSName );                                 \
    idlOS::memset( & (_dst_)->TBSAttr, 0x00, ID_SIZEOF(smiTableSpaceAttr) );\
    (_dst_)->TBSAttr.mDiskAttr.mSegMgmtType                                 \
               = QD_DISK_TABLESPACE_DEFAULT_SEGMENT_MGMT_TYPE;              \
    (_dst_)->TBSAttr.mDiskAttr.mExtMgmtType                                 \
               = QD_DISK_TABLESPACE_DEFAULT_EXTENT_MGMT_TYPE;               \
    QD_SEGMENT_OPTION_INIT( &((_dst_)->segAttr), &((_dst_)->segStoAttr) );  \
    (_dst_)->altAllocExtSize = 0;                                           \
    (_dst_)->indexTBSAttr = NULL;                                           \
    (_dst_)->lobAttr = NULL;                                                \
                                                                            \
    (_dst_)->tableInfo = NULL;                                              \
    (_dst_)->partTable = NULL;                                              \
    (_dst_)->isRowmovement = ID_FALSE;                                      \
    (_dst_)->tableAttrFlagList = NULL;                                      \
    (_dst_)->tableAttrMask = 0;                                             \
    (_dst_)->tableAttrValue = 0;                                            \
    (_dst_)->loggingMode = NULL;                                            \
    (_dst_)->parallelDegree = 0;                                            \
    (_dst_)->temporaryOption = NULL;                                        \
    (_dst_)->ncharList = NULL;                                              \
                                                                            \
    idlOS::memset( (_dst_)->mviewViewName, 0x00, QC_MAX_OBJECT_NAME_LEN + 1 );     \
    (_dst_)->mviewViewColumns = NULL;                                       \
    (_dst_)->mviewBuildRefresh.buildType   = QD_MVIEW_BUILD_NONE;           \
    (_dst_)->mviewBuildRefresh.refreshType = QD_MVIEW_REFRESH_NONE;         \
    (_dst_)->mviewBuildRefresh.refreshTime = QD_MVIEW_REFRESH_ON_NONE;      \
                                                                            \
    (_dst_)->oldIndexTables = NULL;                                         \
    (_dst_)->newIndexTables = NULL;                                         \
                                                                            \
    (_dst_)->createIndexParseTree = NULL;                                   \
    (_dst_)->addHiddenColumn = ID_FALSE;                                    \
    (_dst_)->dropHiddenColumn = ID_FALSE;                                   \
    (_dst_)->defaultExprColumns = NULL;                                     \
    (_dst_)->compressionColumn = NULL;                                      \
    (_dst_)->relatedFunctionNames = NULL;                                   \
                                                                            \
    (_dst_)->accessOption = QCM_ACCESS_OPTION_NONE;                         \
    (_dst_)->mIsUsable = ID_TRUE;                                           \
    (_dst_)->mShardFlag = QCM_SHARD_FLAG_TABLE_NONE;                        \
    QD_SEGMENT_STORAGE_EXIST_INIT( &((_dst_)->existSegStoAttr ) );          \
    (_dst_)->mPartAttr = NULL;                                              \
                                                                            \
    SET_EMPTY_POSITION( (_dst_)->mSourceUserName );                         \
    SET_EMPTY_POSITION( (_dst_)->mSourceTableName );                        \
    SET_EMPTY_POSITION( (_dst_)->mNamesPrefix );                            \
    (_dst_)->mIgnoreForeignKeyChild = ID_FALSE;                             \
    (_dst_)->mIsRenameForce = ID_FALSE;                                     \
    (_dst_)->mSourcePartTable = NULL;                                       \
    (_dst_)->mSourceRefChildInfoList = NULL;                                \
    (_dst_)->mTargetRefChildInfoList = NULL;                                \
    (_dst_)->mRefChildPartTableList = NULL;                                 \
}

typedef struct qdTableOptions // temporary struct for parser
{
    qdTemporaryOption        * temporaryOption;
    ULong                    * maxrows;
    qcNamePosition           * TBSName;
    UInt                     * pctFree;
    UInt                     * pctUsed;
    UInt                     * initTrans;
    UInt                     * maxTrans;
    qdSegStorageAttr         * segStorageAttr;
    qdPartitionedTable       * partTable;
    idBool                     isRowmovement;
    qdTableAttrFlagList      * tableAttrFlagList;
    UInt                     * loggingMode;
    UInt                       parallelDegree;
    qcmCompressionColumn     * compressionColumn;
    qcmAccessOption            accessOption;    /* PROJ-2359 Table/Partition Access Option */
    qcmShardFlag               mShardFlag;      /* TASK-7307 DML Data Consistency in Shard */
} qdTableOptions;

typedef struct qdTableElement // temporary struct for parser
{
    qdConstraintSpec  * constraints;
    qcmColumn         * columns;
    qcmColumn         * modifyColumns;
} qdTableElement;

typedef struct qdUserNObjName // temporary struct for parser
{
    qcNamePosition      userName;
    qcNamePosition      objectName;
} qdUserNObjName;

typedef struct qdOuterJoinOper // temporary struct for parser
{
    qcNamePosition      endPos;
} qdOuterJoinOper;

// To Fix PR-10909
/*
// ALTER TABLE ADD CONSTRAINT
// ALTER TABLE DROP CONSTRAINT
typedef struct qdTblConstrParseTree
{
qcParseTree         common;

qcNamePosition      userName;
qcNamePosition      tableName;

qdConstraintSpec  * constraints;

// for ALTER TABLE
scSpaceID           TBSID; // default data TBS of table owner
qcmTableInfo      * tableInfo; // set in validation
} qdTblConsrParseTree;
*/

// PROJ-1502 PARTITIONED DISK TABLE
typedef struct qdPartitionedIndex
{
    qcmColumn                   * partKeyColumns; // for Global Index
    qcmPartitionMethod            partMethod; // for Global Index
    qdPartitionAttribute        * partAttr;
    qcmIndexPartitionType         partIndexType;
    struct qcmPartitionInfoList * partInfoList;
} qdPartitionedIndex;

#define QD_SET_INIT_PARTITIONED_INDEX(_part_index_)                 \
    {                                                               \
        _part_index_->partKeyColumns = NULL;                        \
        _part_index_->partMethod = QCM_PARTITION_METHOD_NONE;       \
        _part_index_->partAttr = NULL;                              \
        _part_index_->partIndexType = QCM_NONE_PARTITIONED_INDEX;   \
        _part_index_->partInfoList = NULL;                          \
    }

// CREATE INDEX
typedef struct qdIndexParseTree
{
    qcParseTree            common;

    qcNamePosition         userNameOfIndex;
    qcNamePosition         indexName;
    UInt                   userIDOfIndex;

    qcNamePosition         userNameOfTable;
    qcNamePosition         tableName;
    UInt                   userIDOfTable;

    qcmColumn            * keyColumns;
    UInt                   flag;        // UNIQUE, NON-UNIQUE

    // BUG-17848 : 영속적인 속성과 휘발성 속성을 분리
    UInt                   buildFlag;

    qcNamePosition         indexType;
    qcNamePosition         TBSName;
    UInt                   parallelDegree;

    // validation information
    UInt                   keyColCount; // set in validation
    scSpaceID              TBSID;
    qcmTableInfo         * tableInfo; // The member of qcmTableInfo is READ-ONLY.

    // PROJ-1502 PARTITIONED DISK TABLE
    qdPartitionedIndex   * partIndex;
    qcNamePosition         rebuildPartName;

    // PROJ-1704 MVCC Renewal
    smiSegAttr             segAttr;
    // PROJ-1671 BITMAP TABLESPACE AND SEGMENT SPACE MANAGEMENT
    smiSegStorageAttr      segStoAttr;
    // alter index allocate extent ( size .. );
    ULong                  altAllocExtSize;

    // TASK-2176
    void                 * tableHandle;
    smSCN                  tableSCN;

    // BUG-15235 RENAME INDEX
    qcNamePosition         newIndexName;

    // PROJ-1624 global non-partitioned index
    SChar                  indexTableName[ QC_MAX_OBJECT_NAME_LEN + 1 ];
    SChar                  keyIndexName[ QC_MAX_OBJECT_NAME_LEN + 1 ];
    SChar                  ridIndexName[ QC_MAX_OBJECT_NAME_LEN + 1 ];
    
    // PROJ-1624 global non-partitioned index
    qdIndexTableList     * oldIndexTables;
    qdIndexTableList     * newIndexTables;

    /* PROJ-1090 Function-based Index */
    qcmColumn            * addColumns;
    qmsFrom              * defaultExprFrom;
    qcNamePosList        * ncharList;

    /* BUG-35445 Check Constraint, Function-Based Index에서 사용 중인 Function을 변경/제거 방지 */
    qdFunctionNameList   * relatedFunctionNames;

    /* PROJ-2433 Direct Key Index */
    ULong                  mDirectKeyMaxSize;

    /* PROJ-2464 hybrid partitioned table 지원 */
    qdSegStoAttrExist     existSegStoAttr;
} qdIndexParseTree;

#define QD_INDEX_PARSE_TREE_INIT(_dst_)                                         \
    {                                                                           \
        SET_EMPTY_POSITION(_dst_->userNameOfIndex);                             \
        SET_EMPTY_POSITION(_dst_->indexName);                                   \
        SET_EMPTY_POSITION(_dst_->userNameOfTable);                             \
        SET_EMPTY_POSITION(_dst_->tableName);                                   \
        QD_SEGMENT_OPTION_INIT( &(_dst_->segAttr), &(_dst_->segStoAttr) );      \
        _dst_->altAllocExtSize = 0;                                             \
        _dst_->keyColumns = NULL;                                               \
        _dst_->tableInfo = NULL;                                                \
        _dst_->partIndex = NULL;                                                \
        _dst_->tableHandle = NULL;                                              \
        _dst_->flag = 0;                                                        \
        SET_EMPTY_POSITION(_dst_->newIndexName);                                \
        _dst_->oldIndexTables = NULL;                                           \
        _dst_->newIndexTables = NULL;                                           \
        _dst_->addColumns = NULL;                                               \
        _dst_->defaultExprFrom = NULL;                                          \
        _dst_->ncharList = NULL;                                                \
        _dst_->relatedFunctionNames = NULL;                                     \
        QD_SEGMENT_STORAGE_EXIST_INIT( &((_dst_)->existSegStoAttr ) );          \
    }


// CREATE SEQUENCE
// ALTER  SEQUENCE
typedef struct qdSequenceOptions
{
    SLong   * startValue;
    SLong   * incrementValue;
    SLong   * minValue;
    SLong   * maxValue;
    SLong   * cacheValue;
    UInt    * cycleOption;
    UInt      flag; // for NOMINVALUE, NOMAXVALUE, RESTART, LOCALITY(SHARD/LOCAL), SCALE(FIXED/VARIABLE for SHARD) option of ALTER SEQUENCE
} qdSequenceOptions;

typedef struct qdSequenceParseTree
{
    qcParseTree           common;

    qcNamePosition        userName;
    qcNamePosition        sequenceName;

    qdSequenceOptions   * sequenceOptions;
    idBool                enableSeqTable;
    idBool                flushCache;

    UInt                  userID;

    // TASK-2176
    void                * sequenceHandle;
    UInt                  sequenceID;    // fix BUG-14394
                                         // validateAlter 시 세팅됨.
} qdSequenceParseTree;

#define QD_SEQUENCE_PARSE_TREE_INIT(_dst_)              \
    {                                                   \
        SET_EMPTY_POSITION(_dst_->userName);            \
        SET_EMPTY_POSITION(_dst_->sequenceName);        \
        _dst_->sequenceOptions = NULL;                  \
        _dst_->enableSeqTable = ID_FALSE;               \
        _dst_->flushCache = ID_FALSE;                   \
        _dst_->sequenceHandle = NULL;                   \
        _dst_->userID = 0;                              \
    }


// User TABLESPACE
typedef struct qdUserTBSAccess
{
    qcNamePosition           TBSName;

    idBool                   isAccess;
    struct qdUserTBSAccess * next;

    // validation information
    scSpaceID                dataTBSID;
    
} qdUserTBSAccess;

/* PROJ-2207 Password policy support */
typedef struct qdUserPasswOptions
{
    qdPasswOptionName    passwOptNum;    
    SInt                 passwOptValue;
    qcNamePosition       passwVerifyFuncName;
    qdUserPasswOptions * next;    
} qdUserPasswOptions;

typedef struct qdUserOptions // temporary struct for parser
{
    qcNamePosition      * password;
    qdUserPasswOptions  * passwOptions;
    qcNamePosition      * dataTBSName;
    qcNamePosition      * tempTBSName;
    qdUserTBSAccess     * access;
    qdExpLockStatus       expLock; 
    qdDisableTCP          disableTCP; /* PROJ-2474 SSL/TLS Support */
} qdUserOptions;

// CREATE USER, ALTER USER
// CONNECT, DISCONNECT
typedef struct qdUserParseTree
{
    qcParseTree        common;

    qcNamePosition     userName;
    qcNamePosition     password;

    idBool             isSysdba;

    // A4
    qcNamePosition     dataTBSName;
    qcNamePosition     tempTBSName;
    qdUserTBSAccess  * access;

    // validation information
    UInt               userID;

    // A4
    scSpaceID          dataTBSID;
    scSpaceID          tempTBSID;
    
    /* PROJ-2207 Password policy support */
    UInt               accountLock;       /* 명시적 Lock 설정 */
    UInt               passwLimitFlag;    /* PASSWORD POLICY 설정 */
    qdExpLockStatus    expLock;           /* 명시적 lock (ALTER LOCK) */
    SInt               failedCount;
    SInt               reuseCount;

    SChar            * lockDate;          /* LOCK 된 날짜 */
    SChar            * expiryDate;        /* 패스워드 만료 날짜 */
    UInt               failLoginAttempts; /* 시도 가능 횟수 */
    UInt               passwLifeTime;     /* 만료 되는 기간 */
    UInt               passwReuseTime;    /* 재사용 가능 기간 */
    UInt               passwReuseMax;     /* 재사용 횟수 */
    UInt               passwLockTime;     /* unlock으로 되는 기간 */
    UInt               passwGraceTime;    /* 만료에 유예기간 */
    qcNamePosition     passwVerifyFunc;   /* 검증 function */
    qdDisableTCP       disableTCP;        /* PROJ-2474 SSL/TLS Support */
} qdUserParseTree;

// DROP object(TABLE, INDEX, USER, SEQUENCE)
// TRUNCATE TABLE
// PURGE TABLE
typedef struct qdDropParseTree
{
    qcParseTree       common;

    qcNamePosition    userName;
    qcNamePosition    objectName;

    UInt              userID;

    // Proj-1360 Queue
    UInt              flag;

    // TASK-2176
    qcmTableInfo    * tableInfo;
    void            * tableHandle;
    smSCN             tableSCN;

    // PROJ-1407 Temporary Table - for truncate
    UInt              tableID;
    qcmTemporaryType  temporaryType;

    // PROJ-1502 PARTITIONED DISK TABLE
    struct qcmPartitionInfoList * partInfoList;

    /* PROJ-2211 Materialized View */
    SChar             mviewViewName[QC_MAX_OBJECT_NAME_LEN + 1];
    qcmTableInfo    * mviewViewInfo;
    void            * mviewViewHandle;
    smSCN             mviewViewSCN;
    
    // PROJ-1624 global non-partitioned index
    qdIndexTableList * oldIndexTables;
    qdIndexTableList * newIndexTables;

    /* PROJ-2441 flashback */
    idBool            useRecycleBin;
    
} qdDropParseTree;

#define QD_DROP_PARSE_TREE_INIT(_dst_)                                      \
    {                                                                       \
        SET_EMPTY_POSITION(_dst_->userName);                                \
        SET_EMPTY_POSITION(_dst_->objectName);                              \
        _dst_->tableInfo = NULL;                                            \
        _dst_->tableHandle = NULL;                                          \
        _dst_->userID = 0;                                                  \
        _dst_->flag = 0;                                                    \
                                                                            \
        idlOS::memset( _dst_->mviewViewName, 0x00, QC_MAX_OBJECT_NAME_LEN + 1 );   \
        _dst_->mviewViewInfo = NULL;                                        \
        _dst_->mviewViewHandle = NULL;                                      \
                                                                            \
        _dst_->partInfoList = NULL;                                         \
        _dst_->oldIndexTables = NULL;                                       \
        _dst_->newIndexTables = NULL;                                       \
        _dst_->useRecycleBin = ID_FALSE;                                    \
    }


typedef struct qdConstraintSpec
{
    UInt                     flag;

    // BUG-17848 : 영속적인 속성과 휘발성 속성을 분리
    UInt                     buildFlag;

    qcNamePosition           constrName;
    qdConstraintType         constrType;
    qcmColumn              * constraintColumns;
    struct qdReferenceSpec * referentialConstraintSpec;
    idBool                   isPers;
    qcNamePosition           indexTBSName;
    UInt                     parallelDegree;

    /* PROJ-1107 Check Constraint 지원 */
    qtcNode                * checkCondition;
    qcNamePosList          * ncharList;

    qdConstraintSpec       * next;

    // information after validation
    UInt                     constrColumnCount;
    scSpaceID                indexTBSID;   // Index 가 저장될 TBS ID
    smiTableSpaceType        indexTBSType; // Index 가 저장될 TBS Type

    // fix BUG-18937
    qdPartitionedIndex     * partIndex;
    smiSegAttr               segAttr;

    // PROJ-1874
    struct qdConstraintState * constrState;

    // PROJ-1624 global non-partitioned index
    UInt                     indexTableID;
    SChar                    indexTableName[ QC_MAX_OBJECT_NAME_LEN + 1 ];
    SChar                    keyIndexName[ QC_MAX_OBJECT_NAME_LEN + 1 ];
    SChar                    ridIndexName[ QC_MAX_OBJECT_NAME_LEN + 1 ];
    
    /* PROJ-2433 Direct Key Index */
    ULong                    mDirectKeyMaxSize;

} qdConstraintSpec;

#define QD_SET_INIT_CONSTRAINT_SPEC(_constr_spec_)                       \
{                                                                        \
    (_constr_spec_)->flag = 0;                                           \
    (_constr_spec_)->buildFlag = SMI_INDEX_BUILD_DEFAULT;                \
    SET_EMPTY_POSITION( (_constr_spec_)->constrName );                   \
    (_constr_spec_)->constrType                = QD_CONSTR_MAX;          \
    (_constr_spec_)->constraintColumns         = NULL;                   \
    (_constr_spec_)->referentialConstraintSpec = NULL;                   \
    (_constr_spec_)->isPers                    = ID_FALSE;               \
    SET_EMPTY_POSITION( (_constr_spec_)->indexTBSName );                 \
    (_constr_spec_)->parallelDegree            = 0;                      \
    (_constr_spec_)->checkCondition            = NULL;                   \
    (_constr_spec_)->ncharList                 = NULL;                   \
    (_constr_spec_)->next                      = NULL;                   \
    (_constr_spec_)->constrColumnCount         = 0;                      \
    (_constr_spec_)->indexTBSID                = ID_USHORT_MAX;          \
    (_constr_spec_)->indexTBSType              = SMI_TABLESPACE_TYPE_MAX;\
    (_constr_spec_)->partIndex                 = NULL;                   \
    (_constr_spec_)->segAttr.mPctFree          = QD_INVALID_PCT_VALUE;   \
    (_constr_spec_)->segAttr.mPctUsed          = QD_INVALID_PCT_VALUE;   \
    (_constr_spec_)->segAttr.mInitTrans        = QD_INVALID_TRANS_VALUE; \
    (_constr_spec_)->segAttr.mMaxTrans         = QD_INVALID_TRANS_VALUE; \
    (_constr_spec_)->constrState               = NULL;                   \
    (_constr_spec_)->indexTableID              = 0;                      \
    (_constr_spec_)->mDirectKeyMaxSize         = (ULong)(ID_ULONG_MAX);  \
}

typedef struct qdReferenceSpec
{
    UInt                    referenceRule;
    qcNamePosition          referencedUserName;
    qcNamePosition          referencedTableName;
    qcmColumn             * referencedColList;
    /* informations after validation */
    UInt                    referencedTableID;
    qcmTableInfo          * referencedTableInfo;    // BUG-17122
    void                  * referencedTableHandle;  // BUG-17122
    smSCN                   referencedTableSCN;     // BUG-17122
    UInt                    referencedIndexID;
    UInt                    referencedUserID;
    UInt                    referencedColCount;
    UInt                    referencedColID[QC_MAX_KEY_COLUMN_COUNT];
} qdReferenceSpec;

// PROJ-1874 FK Novalidate
typedef struct qdConstraintState
{
    idBool                  validate; 
    qcNamePosition          validatePosition;
} qdConstraintState;

#define QD_CONSTRAINT_STATE_INIT(_dst_)                       \
{                                                             \
        _dst_->validate        = ID_TRUE;                     \
        SET_EMPTY_POSITION( _dst_->validatePosition );        \
}

/*
//   Enable, Defferable과 InitialDeferred는 아직 미구현이다.
typedef struct qdConstraintState
{
    idBool                  enable;
    idBool                  validate; 
    idBool                  deferrable;
    idBool                  initialDeferred;
 
    qcNamePosition          enablePosition;
    qcNamePosition          validatePosition;
    qcNamePosition          deferrablePosition;
    qcNamePosition          initialDeferredPosition;
} qdConstraintState;
 
#define QD_CONSTRAINT_STATE_INIT(_dst_)                       \
{                                                             \
        _dst_->enable          = ID_TRUE;                     \
        _dst_->validate        = ID_TRUE;                     \
        _dst_->deferrable      = ID_FALSE;                    \
        _dst_->initialDeferred = ID_FALSE;                    \
        SET_EMPTY_POSITION( _dst_->enablePosition );          \
        SET_EMPTY_POSITION( _dst_->validatePosition );        \
        SET_EMPTY_POSITION( _dst_->deferrablePosition );      \
        SET_EMPTY_POSITION( _dst_->initialDeferredPosition ); \
}
*/

// SET TRANSACTION, SAVEPOINT, COMMIT, ROLLBACK
typedef struct qdTransParseTree
{
    qcParseTree         common;

    qcNamePosition      savepointName;

    UInt    maskType;   // SMI_TRANSACTION_MASK or SMI_ISOLATION_MASK
    UInt    maskValue;  // In case of SMI_TRANSACTION_MASK
                        //      - SMI_TRANSACTION_NORMAL
                        //      - SMI_TRANSACTION_UNTOUCHABLE
                        // In case of SMI_ISOLATION_MASK
                        //      - SMI_ISOLATION_CONSISTENT
                        //      - SMI_ISOLATION_REPEATABLE
                        //      - SMI_ISOLATION_NO_PHANTOM
    idBool  isSession;  // ALTER SESSION SET TRANSACTION
} qdTransParseTree;

// ALTER SESSION SET STACK SIZE = integer;
typedef struct qdStackParseTree
{
    qcParseTree         common;

    UInt                stackSize;
} qdStackParseTree;

// SET variable = value
// CHECK LOG
// CHECK REPLICATION_GAP
// CHECK literal
// CHECK literal integer
typedef struct qdSetParseTree
{
    qcParseTree         common;

    qcNamePosition      variableName;   // variable or LOG or REPLICATION_GAP
    idBool              hasValue;
    qcNamePosition      charValue;
    UInt                numberValue;

    qcNamePosition      userName;       // for CHECK SEQUENCE
} qdSetParseTree;

/* PROJ-1438 JobScheduler */
typedef struct qdJobParseTree
{
    qcParseTree    common;

    qcNamePosition jobName;
    qcNamePosition exec;
    qcNamePosition start;
    qcNamePosition end;
    qcNamePosition comment; // BUG-41713 each job enable disable

    UInt           interval;
    UInt           intervalType;
    idBool         enable; // BUG-41713 each job enable disable
} qdJobParseTree;

enum qdStartOption
{
    QDP_OPTION_START = 1,
    QDP_OPTION_STOP,
    QDP_OPTION_RELOAD
};

// ALTER SYSTEM CHECKPOINT
// ALTER SYSTEM BACKUP
// ALTER SYSTEM COMPACT
// ALTER SYSTEM ARCHIVELOG [START|STOP]
typedef struct qdSystemParseTree
{
    qcParseTree         common;

    SChar             * path;
    idBool              startArchivelog;
    UInt                flusherID;
    qdStartOption       startOption;
} qdSystemParseTree;

// ALTER SYSTEM SET PROPERTY_NAME = integer;
// ALTER SYSTEM SET OPTIMIZER_MODE = COST/RULE;
typedef struct qdSystemSetParseTree
{
    qcParseTree         common;

    qcNamePosition      name;   // System Property 이름
    qcNamePosition      value;  // 변경할 값
} qdSystemSetParseTree;

typedef struct qdDefaultParseTree
{
    qcParseTree           common;

    struct qtcNode      * defaultValue;
    struct qtcNode      * lastNode;
} qdDefaultParseTree;

// PROJ-1502 PARTITIONED DISK TABLE
typedef struct qdPartCondValParseTree
{
    qcParseTree          common;
    struct qmmValueNode * partKeyCond;
} qdPartCondValParseTree;

// GRANT
typedef struct qdGrantParseTree
{
    qcParseTree           common;

    struct qdPrivileges * privileges;
    struct qdGrantees   * grantees;

    // on object
    qcNamePosition        userName;
    qcNamePosition        objectName;

    UInt                  userID;
    qdpObjID              objectID;
    SChar                 objectType[2];
    // T : table or sequence
    // P : procedure

    idBool                grantOption;

    UInt                  grantorID;
} qdGrantParseTree;

#define QD_GRANT_PARSE_TREE_INIT(_dst_)                 \
    {                                                   \
        SET_EMPTY_POSITION(_dst_->userName);            \
        SET_EMPTY_POSITION(_dst_->objectName);          \
        _dst_->privileges = NULL;                       \
        _dst_->grantees = NULL;                         \
    }

enum qdPrivType
{
    QDP_OBJECT_PRIV = 1,        // only OBJECT privilege
    QDP_SYSTEM_PRIV,            // only SYSTEM privilege
    QDP_SYSTEM_OBJECT_PRIV,     // SYSTEM or OBJECT privilege
    QDP_ROLE_PRIV               // ROLE
};

/* PROJ-1812 ROLE */
enum qdUserType
{
    QDP_USER_TYPE = 1,
    QDP_ROLE_TYPE
};

/* role에의해 reference 권한을 부여 받은 userid list */
typedef struct qdReferenceGranteeList
{
    UInt                              userID;
    struct qdReferenceGranteeList   * next;
} qdReferenceGranteeList;

typedef struct qdPrivileges
{
    qdPrivType                privType;

    qcNamePosition            privOrRoleName;
    UInt                      privOrRoleID;

    struct qdPrivileges     * next;
} qdPrivileges;

typedef struct qdGrantees
{
    qdUserType            userType;
    
    UInt                  userOrRoleID;
    qcNamePosition        userOrRoleName;

    // at REVOKE object privileges
    idBool                grantOption;

    struct qdGrantees   * next;
} qdGrantees;

// REVOKE
typedef struct qdRevokeParseTree
{
    qcParseTree           common;

    struct qdPrivileges * privileges;
    struct qdGrantees   * grantees;

    // on object
    qcNamePosition        userName;
    qcNamePosition        objectName;

    UInt                  userID;
    qdpObjID              objectID;
    SChar                 objectType[2];
    // T : table or sequence
    // P : procedure

    idBool                cascadeConstr;
    idBool                force;

    UInt                  grantorID;
} qdRevokeParseTree;

#define QD_REVOKE_PARSE_TREE_INIT(_dst_)                \
    {                                                   \
        SET_EMPTY_POSITION(_dst_->userName);            \
        SET_EMPTY_POSITION(_dst_->objectName);          \
        _dst_->privileges = NULL;                       \
        _dst_->grantees = NULL;                         \
    }

enum qdSessionClose
{
    QDP_SESSION_CLOSE_NONE = 0,
    QDP_SESSION_CLOSE_ID,
    QDP_SESSION_CLOSE_USER,
    QDP_SESSION_CLOSE_ALL
};

// DATABASE
typedef struct qdDatabaseParseTree
{
    qcParseTree           common;

    qcNamePosition        dbName;
    UInt                  intValue1;
    UInt                  intValue2;
    UInt                  flag;
    UInt                  optionflag;
    idBool                archiveLog;

    // PROJ-1579 NCHAR
    qcNamePosition        dbCharSet;
    qcNamePosition        nationalCharSet;

    qcNamePosition        userName;
    qdSessionClose        closeMethod;
    
} qdDatabaseParseTree;

#define QD_DATABASE_PARSE_TREE_INIT(_dst_)              \
    {                                                   \
        SET_EMPTY_POSITION(_dst_->dbName);              \
        _dst_->intValue1 = 0;                           \
        _dst_->intValue2 = 0;                           \
        _dst_->flag = 0;                                \
        _dst_->optionflag = 0;                          \
        SET_EMPTY_POSITION(_dst_->dbCharSet);           \
        SET_EMPTY_POSITION(_dst_->nationalCharSet);     \
        SET_EMPTY_POSITION(_dst_->userName);            \
        _dst_->closeMethod = QDP_SESSION_CLOSE_NONE;    \
    }

// PROJ-1579 NCHAR
typedef struct qdCharacterSet // temporary struct for parser
{
    qcNamePosition        dbCharSet;
    qcNamePosition        nationalCharSet;
} qdCharacterSet;

typedef struct qdCreateTBSOptions // temporary struct for parser
{
    smiSegMgmtType            * segMgmtType;
    ULong                     * extentSize;
    idBool                    * isOnline;
} qdCreateTBSOptions;

typedef struct qdOptionFlags // temporary struct for parser
{
    UInt                  flag;
    UInt                  optionflag;
} qdOptionFlags;

typedef struct qdTBSFilesSpec
{
    struct smiDataFileAttr    * fileAttr;
    UInt                        fileNo;
    struct qdTBSFilesSpec     * next;
} qdTBSFilesSpec;


/* Tablespace Attribute Flag의 List */
typedef struct qdTBSAttrFlagList
{
    UInt    attrMask;
    UInt    attrValue;

    // SQL TEXT내에서의 이 Attiribute에 해당되는 Offset과 Size
    // Validation중 특정 Attribute를 찍어서 에러를 내고자 할 때 사용
    qcNamePosition attrPosition;
    
    // Linked List의 Next포인터
    qdTBSAttrFlagList * next;
} qdTBSAttrFlagList;


/* Table Attribute Flag의 List */

// TABLESPACE
typedef struct qdCreateTBSParseTree
{
    qcParseTree                common;

    struct smiTableSpaceAttr * TBSAttr;
    qdTBSFilesSpec           * diskFilesSpec;
    ULong                      extentSize;
    UInt                       fileCount;
    /* PRJ-1671 Bitmap Tablespace
     * segment management type */
    smiSegMgmtType             segMgmtType;
    smiExtMgmtType             extMgmtType;

    /* Tablespace의 Attribute Flag
       Ex> LOG_COMPRESS ON/OFF
     */
    qdTBSAttrFlagList         * attrFlagList;
    // Validation이후 세팅 :
    // Tablespace의 모든 Attribute Flag들을 Bitwise OR로 묶은 FLAG
    UInt                        attrFlag;
    
    /* fields used by memory tablespace. */
    SChar                         memTBSName[QC_MAX_OBJECT_NAME_LEN+1];
    struct smiChkptPathAttrList * memChkptPathList;
    ULong                         memSplitFileSize;
    ULong                         memInitSize;
    idBool                        memIsAutoExtend;
    ULong                         memNextSize;
    ULong                         memMaxSize;
    idBool                        memIsOnline;
} qdCreateTBSParseTree;

typedef struct qdTBSFileNames
{
    qcNamePosition             fileName;
    struct qdTBSFileNames    * next;
} qdTBSFileNames;

typedef enum
{
    QD_TBS_BACKUP_NONE = 0,
    QD_TBS_BACKUP_BEGIN,
    QD_TBS_BACKUP_END
} qdTBSBackupState;

typedef enum
{
    QD_ADD_CHECKPOINT_PATH = 0,
    QD_RENAME_CHECKPOINT_PATH,
    QD_DROP_CHECKPOINT_PATH
} qdAlterChkptPathOp;

typedef struct qdAlterChkptPath
{
    qdAlterChkptPathOp       alterOp;
    struct smiChkptPathAttr * fromChkptPathAttr;
    struct smiChkptPathAttr * toChkptPathAttr;
} qdAlterChkptPath;

typedef struct qdAlterTBSParseTree
{
    qcParseTree                common;

    struct smiTableSpaceAttr * TBSAttr;
    qdTBSFilesSpec           * diskFilesSpec;
    qdTBSFileNames           * oldFileNames;
    qdTBSFileNames           * newFileNames;
    qdTBSBackupState           backupState;
    qdAlterChkptPath         * memAlterChkptPath;
    UInt                       fileCount;
    // TASK-2398 Log Compress
    // 변경할 Tablespace의 Attribute Flag
    qdTBSAttrFlagList        * attrFlagToAlter;
} qdAlterTBSParseTree;

typedef struct qdDropTBSParseTree
{
    qcParseTree                common;

    struct smiTableSpaceAttr * TBSAttr;
    UInt                       flag;
} qdDropTBSParseTree;

typedef enum
{
    QD_BACKUP_TABLESPACE = 0,
    QD_BACKUP_LOGANCHOR,
    QD_BACKUP_DATABASE
} qdBackupGranularity;

typedef struct qdFullBackupSpec
{
    SChar                     * srcName;
    SChar                     * path;
    qdBackupGranularity         granularity;
} qdFullBackupSpec;

typedef struct qdTablespaceList
{
    qcNamePosition              namePosition;
    scSpaceID                   id;
    qdTablespaceList          * next;
} qdTablespaceList;

typedef struct qdWithTagSpec
{
    SChar                     * tagName;
} qdWithTagSpec;

typedef struct qdBackupTargetSpec
{
    qdTablespaceList          * tablespaces;
    qdBackupGranularity         granularity;
    qdWithTagSpec             * withTagSpec;
} qdBackupTargetSpec;

typedef struct qdIncrementalBackupSpec
{
    qdBackupTargetSpec        * targetSpec;
    UInt                        level;
    idBool                      cumulative;
} qdIncrementalBackupSpec;

typedef struct qdBackupParseTree
{
    qcParseTree                 common;

    qdFullBackupSpec            fullBackupSpec;
    qdIncrementalBackupSpec     incrementalBackupSpec;
} qdBackupParseTree;

typedef struct qdChangeTrackingParseTree
{
    qcParseTree                 common;
    idBool                      enable;
} qdChangeTrackingParseTree;

typedef enum
{
    QD_MEDIA_RECOVERY_CREATE_DATAFILE = 0,
    QD_MEDIA_RECOVERY_CREATE_CHECKPOINT_IMAGE,
    QD_MEDIA_RECOVERY_RENAME_FILE,
    QD_MEDIA_RECOVERY_DATABASE
} qdMediaRecoveryType;

typedef struct qdUntilSpec
{
    SChar                     * timeString;
    idBool                      cancel;
} qdUntilSpec; // temporary struct for PROJ-1149

typedef struct qdFromTagSpec
{
    SChar                     * tagName;
} qdFromTagSpec;

typedef enum
{
    QD_RECOVER_UNSPECIFIED = 0,
    QD_RECOVER_FROM_TAG,
    QD_RECOVER_UNTIL
} qdRecoveryMethod;

typedef struct qdRecoverSpec
{
    qdRecoveryMethod            method;
    qdUntilSpec               * untilSpec;
    qdFromTagSpec             * fromTagSpec;
} qdRecoverSpec;

typedef struct qdRestoreTargetSpec
{
    qdRecoverSpec             * databaseRestoreSpec;
    qdTablespaceList          * tablespaces;
    qdBackupGranularity         granularity;
} qdRestoreTargetSpec;

typedef struct qdRestoreParseTree
{
    qcParseTree                 common;

    qdRestoreTargetSpec       * targetSpec;
} qdRestoreParseTree;

typedef struct qdMediaRecoveryParseTree
{
    qcParseTree                common;

    // 메모리/디스크 데이타파일 SPEC
    SChar                    * oldName;
    SChar                    * newName;

    // 미디어 복구 옵션
    idBool                     useAnchorfile;
    qdMediaRecoveryType        recoveryType;
    qdRecoverSpec            * recoverSpec;
} qdMediaRecoveryParseTree;

typedef struct qdChangeMoveBackupParseTree
{
    qcParseTree                 common;

    SChar                     * path;
    idBool                      withContents;
} qdChangeMoveBackupParseTree;

// for PROJ-1371
typedef struct qdDirectoryParseTree
{
    qcParseTree           common;
    UInt                  userID;
    SChar*                directoryName;
    qcNamePosition        directoryNamePos;
    SChar*                directoryPath;
    idBool                replace;
    qdpObjID              directoryOID;
} qdDirectoryParseTree;


/* For Project-1076 Synonym */
typedef struct qdSynonymParseTree
{
    /* Parsing Information */
    qcParseTree                common;
    idBool                     isPublic;

    qcNamePosition             synonymOwnerName;
    qcNamePosition             synonymName;

    qcNamePosition             objectOwnerName;
    qcNamePosition             objectName;

    /* Validation Information */
    UInt                       synonymOwnerID;
    UInt                       flag;    // for CREATE OR REPLACE SYNONYM
} qdSynonymParseTree;

#define QD_SYNONYM_PARSE_TREE_INIT(_dst_)        \
    {                                                   \
        /* PRIVATE SYNONYM(default) */                  \
        (_dst_)->isPublic = ID_FALSE;                   \
        SET_EMPTY_POSITION(_dst_->synonymOwnerName);    \
        SET_EMPTY_POSITION(_dst_->synonymName);         \
        SET_EMPTY_POSITION(_dst_->objectOwnerName);     \
        SET_EMPTY_POSITION(_dst_->objectName);          \
        (_dst_)->synonymOwnerID = 0;                    \
        (_dst_)->flag = 0;                              \
    }

/* PROJ-1810 Partition Exchange */
typedef struct qdDisjoinTableParseTree
{
    qcParseTree                common;

    qcNamePosition             userName;
    qcNamePosition             tableName;
    UInt                       userID;

    qcmTableInfo             * tableInfo; // set in validation
    void                     * tableHandle;
    smSCN                      tableSCN;

    UInt                       partCount;

    qdDisjoinTable           * disjoinTable;

    qcmPartitionInfoList     * partInfoList;
} qdDisjoinTableParseTree;

#define QD_DISJOIN_PARSE_TREE_INIT( _dst_ )            \
    {                                                  \
        SET_EMPTY_POSITION( (_dst_)->userName );       \
        SET_EMPTY_POSITION( (_dst_)->tableName );      \
        (_dst_)->userID = QC_EMPTY_USER_ID;            \
        (_dst_)->tableInfo = NULL;                     \
        (_dst_)->tableHandle = NULL;                   \
        (_dst_)->disjoinTable = NULL;                  \
        (_dst_)->partInfoList = NULL;                  \
    }

//------------------------------------------
// To Fix BUG-13127, 13364
// Index Attribute 정보를 가지는 자료 구조
// - type
//   index attribyte type
// - TBSName
//   type이 tablespace인 경우, tablespace name 정보 설정, 그 외는 NULL
// - parallelDegree
//   type이 parallel인 경우, parallel degree 값 설정, 그 외는 0
//------------------------------------------

// BUG-17848 : LOGGING_MASK, FORCE_MASK 분리
#define QD_INDEX_ATTR_TYPE_MASK 0x000000FC

enum qdIndexAttrType
{
    QD_INDEX_ATTR_TYPE_TABLESPACE_MASK      = 0x00000004,
    QD_INDEX_ATTR_TYPE_PARALLEL_MASK        = 0x00000008,
    QD_INDEX_ATTR_TYPE_LOGGING_FORCE_MASK   = 0x00000010,

    //------------------------
    // TABLE SPACE TYPE
    //------------------------

    QD_INDEX_ATTR_TABLESPACE = QD_INDEX_ATTR_TYPE_TABLESPACE_MASK,
    
    //------------------------
    // PARALLEL TYPE
    //------------------------

    QD_INDEX_ATTR_PARALLEL = QD_INDEX_ATTR_TYPE_PARALLEL_MASK,
    QD_INDEX_ATTR_NOPARALLEL,

    //------------------------
    // LOGGING TYPE
    //------------------------

    QD_INDEX_ATTR_LOGGING = QD_INDEX_ATTR_TYPE_LOGGING_FORCE_MASK, // LOGGING
    QD_INDEX_ATTR_NOLOGGING_FORCE,
    QD_INDEX_ATTR_NOLOGGING_NOFORCE
};

typedef struct qdIndexAttribute // temporary struct for parser
{
    qcNamePosition       TBSName;
    UInt                 parallelDegree;
    qdIndexAttrType      type;
    qdIndexAttribute   * next;
} qdIndexAttribute;

// INITRANS n MAXTRANS n;
typedef struct qdTTL // temporary struct for parser
{
    // "INITRANS" 혹은 "MAXTRANS" Identifier의 Position
    qcNamePosition    * identPosition;
    
    UInt              * initTrans;
    UInt              * maxTrans;
    
} qdTTL;

// fix BUG-18937
typedef struct qdIndexAttrAndLocalIndex // temporary struct for parser
{
    qdPartitionedIndex  * partIndex;
    qdIndexAttribute    * indexAttr;
    qdTTL               * ttl;
} qdIndexAttrAndLocalIndex;

typedef struct qdTablePhysicalAttr
{
    // "FREE" 혹은 "USED" Identifier의 Position
    qcNamePosition    * freeUsedIdentPosition;

    UInt              * pctFree;
    UInt              * pctUsed;
} qdTablePhysicalAttr;

// temporary struct for
// Partitioned Table의 Row Movement
typedef struct qdTableRowMovement
{
    qcNamePosition namePosition;
    idBool         rowMovement;
} qdTableRowMovement;

typedef struct qdTableMaxRows
{
    qcNamePosition namePosition;
    ULong          maxRows;
} qdTableMaxRows;

// BUG-42883 alter index parser 개선
typedef struct qdIndexTypeAndDirectKey // temporary struct for parser
{
    qcNamePosition position;
    ULong          maxSize;
} qdIndexTypeAndDirectKey;

#define QD_INDEX_TYPE_DIRECT_KEY_INIT( _dst_ )     \
{                                                  \
    SET_EMPTY_POSITION( (_dst_)->position );       \
    (_dst_)->maxSize = (ULong)(ID_ULONG_MAX);      \
}

// BUG-21387 COMMENT
typedef struct qdCommentParseTree
{
    qcParseTree         common;

    qcNamePosition      userName;
    qcNamePosition      tableName;
    qcNamePosition      columnName;
    qcNamePosition      comment;
    
    UInt                userID;
    qcmTableInfo      * tableInfo;
    void              * tableHandle;
    smSCN               tableSCN;
} qdCommentParseTree;

// PROJ-1624 global non-partitioned index
#define QD_OID_COLUMN_NAME      ((SChar *)"$GIT_OID")
#define QD_OID_COLUMN_NAME_SIZE (8)

#define QD_RID_COLUMN_NAME      ((SChar *)"$GIT_RID")
#define QD_RID_COLUMN_NAME_SIZE (8)

#define QD_INDEX_TABLE_PREFIX                ((SChar *)"$GIT_")
#define QD_INDEX_TABLE_PREFIX_SIZE           (5)

#define QD_INDEX_TABLE_KEY_INDEX_PREFIX      ((SChar *)"$GIK_")
#define QD_INDEX_TABLE_KEY_INDEX_PREFIX_SIZE (5)

#define QD_INDEX_TABLE_RID_INDEX_PREFIX      ((SChar *)"$GIR_")
#define QD_INDEX_TABLE_RID_INDEX_PREFIX_SIZE (5)

typedef struct qdIndexCursor
{
    // index table cursor
    smiTableCursor        cursor;
    smiFetchColumnList    fetchColumn;
    
    void                * ridIndexHandle;
    UInt                  ridIndexType;
    smiColumnList         updateColumnList[2];  // oid, rid
    mtcColumn           * ridColumn;
    qcmIndex            * index;

    // opened flag
    idBool                cursorOpened;
    
} qdIndexCursor;

typedef struct qdIndexTableCursors
{
    // index table list
    qdIndexTableList    * indexTables;
    UInt                  indexTableCount;
    
    // index table만큼 할당함
    qdIndexCursor       * indexCursors;
    
    // max로 할당하고 공유함
    const void          * row;
    smiValue            * newRow;
    
} qdIndexTableCursors;

/* PROJ-1090 Function-based Index */
typedef struct qdSortMode           // temporary struct for parser
{
    qcNamePosition      position;
    idBool              isDescending;
} qdSortMode;

typedef struct qdColumnWithPosition // temporary struct for parser
{
    qcmColumn             * column;
    qcNamePosition          beforePosition; // '(', ','
    qcNamePosition          afterPosition;  // ',', 'ASC', 'DESC', ')'
    qdColumnWithPosition  * next;
} qdColumnWithPosition;

/* PROJ-1810 Partition Exchange */
typedef struct qdDisjoinConstr
{
    /* 기존 constraint의 ID와 이에 대응되는 새 ID, 이름을 저장 */
    UInt                  oldConstrID;
    UInt                  newConstrID;
    UInt                  columnCount;
    SChar                 newConstrName[ QC_MAX_OBJECT_NAME_LEN + 1 ];
    qdDisjoinConstr     * next;
} qdDisjoinConstr;

/* BUG-45921 */
typedef struct qdQueueSequenceParseTree
{
    qcParseTree           common;

    qcNamePosition        mUserName;
    qcNamePosition        mQueueName;

    UInt                  mUserID;

    qcmTableInfo        * mTableInfo;
    void                * mTableHandle;
    smSCN                 mTableSCN;

    qcmSequenceInfo       mQueueSequenceInfo;
    void                * mQueueSequenceHandle;
} qdQueueSequenceParseTree;

#define QD_QUEUE_SEQUENCE_PARSE_TREE_INIT(_dst_)        \
    {                                                   \
        SET_EMPTY_POSITION(_dst_->mUserName);           \
        SET_EMPTY_POSITION(_dst_->mQueueName);          \
        _dst_->mUserID = QC_EMPTY_USER_ID;              \
        _dst_->mTableInfo = NULL;                       \
        _dst_->mTableHandle = NULL;                     \
        SMI_INIT_SCN( & (_dst_)->mTableSCN );           \
        _dst_->mQueueSequenceHandle = NULL;             \
    }

typedef enum 
{
    SHARD_DDL_UNKNOWN = 0,
    SHARD_ADD,
    SHARD_JOIN,
    SHARD_DROP,
    SHARD_DROP_FORCE,
    SHARD_FAILOVER_NORMAL,
    SHARD_FAILOVER_IMMEDIATE,
    SHARD_FAILOVER_EXIT,
    SHARD_FAILOVER_FORCE,
    SHARD_FAILBACK,
    SHARD_FAILBACK_SYNC,
    SHARD_FAILBACK_INSTANT,
    SHARD_MOVE_TO,
    SHARD_MOVE_TO_INTERNAL_CLONE_COPY,
    SHARD_MOVE_REMOVE,
} shardDDLType;

typedef struct qdReShardAttribute
{
    /* the variable below will be filled at parsing syntax */
    qcNamePosition          mUserName;
    qcNamePosition          mObjectName;
    qcNamePosition          mPartitionName;
    qmmValueNode *          mKeyValue;
    SChar                   mObjectType; /* 'P':procedure, 'T':table */
    qdReShardAttribute    * next;

    /* the variable below will be filled at validating syntax */
    SChar                   mFromNodeName[SDI_NODE_NAME_MAX_SIZE + 1];
    SChar                   mDefaultNodeName[SDI_NODE_NAME_MAX_SIZE + 1];
    qcmTableInfo          * mTableInfo;
    smSCN                   mTableSCN;
    void                  * mTableHandle;
    sdiObjectInfo         * mShardObjectInfo;
} qdReShardAttribute;

#define QD_SET_INIT_RESHARD_ATTR(_reshard_attr_)                   \
    {                                                              \
        SET_EMPTY_POSITION( _reshard_attr_->mUserName );           \
        SET_EMPTY_POSITION( _reshard_attr_->mObjectName );          \
        SET_EMPTY_POSITION( _reshard_attr_->mPartitionName );      \
        _reshard_attr_->mKeyValue = NULL;                          \
        _reshard_attr_->mObjectType = 'T';                         \
        _reshard_attr_->mFromNodeName[0] = '\0';                   \
        _reshard_attr_->mDefaultNodeName[0] = '\0';                \
        _reshard_attr_->mTableInfo = NULL;                         \
        SM_INIT_SCN(&(_reshard_attr_->mTableSCN));                 \
        _reshard_attr_->mTableHandle = NULL;                       \
        _reshard_attr_->mShardObjectInfo = NULL;                   \
        _reshard_attr_->next = NULL;                               \
    }

typedef struct qdShardParseTree
{
    qcParseTree           common;

    /* the variable below will be filled at parsing shard table syntax */
    qcNamePosition        mNodeName;
    qdReShardAttribute    * mReShardAttr;
    shardDDLType          mDDLType;

    /* the variable below will be filled at validating syntax or starting execution*/
    iduList             * mNodeInfoList;
    UInt                  mNodeCount;
} qdShardParseTree;

#define QDSD_SHARD_PARSE_TREE_INIT(_dst_)               \
    {                                                   \
        SET_EMPTY_POSITION(_dst_->mNodeName);           \
        _dst_->mReShardAttr         = NULL;             \
        _dst_->mNodeInfoList        = NULL;             \
        _dst_->mDDLType             = SHARD_DDL_UNKNOWN;  \
        _dst_->mNodeCount           = 0;                  \
        _dst_->mNodeInfoList        = NULL;             \
    }

#endif /* _O_QD_PARSE_TREE_H_ */

