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
 * $Id: smrDef.h 91224 2021-07-14 05:36:12Z minku.kang $
 *
 * Description :
 *
 * Recovery Layer Common Header 파일
 *
 *
 **********************************************************************/

#ifndef _O_SMR_DEF_H_
#define _O_SMR_DEF_H_ 1

#include <smDef.h>
#include <smu.h>
#include <sdrDef.h>
#include <smriDef.h>
#include <sdsDef.h>

/* --------------------------------------------------------------------
 * Memory Redo 함수
 * ----------------------------------------------------------------- */

typedef IDE_RC (*smrRecFunction)(smTID,
                                 scSpaceID,
                                 scPageID,
                                 scOffset,
                                 vULong,
                                 SChar*,
                                 SInt,
                                 UInt aFlag);

// BUG-9640
typedef IDE_RC (*smrTBSUptRecFunction)( idvSQL*            aStatistics,
                                        void*              aTrans,
                                        smLSN              aCurLSN,
                                        scSpaceID          aSpaceID,
                                        UInt               aFileID,
                                        UInt               aValueSize,
                                        SChar*             aValuePtr,
                                        idBool             aIsRestart );

#define SMR_LOG_FULL_SIZE               (500)

#define SMR_LOG_FILE_NAME               "logfile"
#define SMR_TEMP_LOG_FILE_NAME          "logfile.tmp"  /* BUG-48409 */

#define SMR_BACKUP_VER_NAME             "backup_ver"
#define SMR_LOGANCHOR_RESERV_SIZE       (4)

/* ------------------------------------------------
 * for multi-loganchor
 * ----------------------------------------------*/
#define SMR_LOGANCHOR_FILE_COUNT       (3)
#define SMR_LOGANCHOR_NAME              "loganchor"

#define SMR_BACKUP_NONE                (0x00000000)
#define SMR_BACKUP_MEMTBS              (0x00000001)
#define SMR_BACKUP_DISKTBS             (0x00000002)

typedef UInt    smrBackupState;


/*
 PRJ-1548 미디어 오류 매체 종류
 미디어 복구가 필요한 데이타파일이
 메모리인지 디스크인지 모두다 인지를 구분하여
 미디어 복구시 적당한 로그레코드를
 재수행하기 위해서이다
 */
# define SMR_FAILURE_MEDIA_NONE      (0x00000000) // 미디어오류없음
# define SMR_FAILURE_MEDIA_MRDB      (0x00000001) // 메모리DB 오류
# define SMR_FAILURE_MEDIA_DRDB      (0x00000010) // 디스크DB 오류
# define SMR_FAILURE_MEDIA_BOTH      (0x00000011) // 모두 오류

/* ------------------------------------------------
 * drdb의 로그가 여러 로그에 걸쳐 저장이 되었는지
 * 단일로그인지를 판단 할 수 있는 type
 * ----------------------------------------------*/
typedef enum
{
    SMR_CT_END = 0,
    SMR_CT_CONTINUE
} smrContType;

typedef enum
{
    SMR_CHKPT_TYPE_MRDB = 0,
    SMR_CHKPT_TYPE_DRDB,
    SMR_CHKPT_TYPE_BOTH
} smrChkptType;

/* ------------------------------------------------
 * redo 적용시 runtime memory에 대하여 함께 재수행
 * 할 것인가의 여부를 표시
 * ----------------------------------------------*/
typedef enum
{
    SMR_RT_DISKONLY = 0,
    SMR_RT_WITHMEM
} smrRedoType;

typedef enum
{
    SMR_SERVER_SHUTDOWN,
    SMR_SERVER_STARTED
} smrServerStatus;

/*  PROJ-1362   replication for LOB */
/*  LogType  SMR_LT_LOB_FOR_REPL의 sub type */
typedef enum
{
    SMR_MEM_LOB_CURSOR_OPEN = 0,
    SMR_DISK_LOB_CURSOR_OPEN,
    SMR_LOB_CURSOR_CLOSE,
    SMR_PREPARE4WRITE,
    SMR_FINISH2WRITE,
    SMR_LOB_ERASE,
    SMR_LOB_TRIM,
    SMR_LOB_OP_MAX
} smrLobOpType;


/* --------------------------------------
   로그타입 추가, 변경, 삭제시
   smrLogFileDump에서 가지고 있는 LogType
   이름에 대한 수정이 필요합니다.

   물론 logType 개수가 늘어나 UChar에서 그
   이상의 타입으로 늘어날 경우, 이름을 저장
   하는 문자열 배열의 크기도 조정해야 합니
   다.
   -------------------------------------- */
typedef UChar                       smrLogType;

/* 어떤 undo, redo도 수행하지 않는 로그로서
   1. Repliction Sender 시작시 Log에 대해서 기록한다.
   자세한 사항은 smiReadLogByOrder.cpp를 참조하기 바란다.
   2. Checkpoint시 smrLogMgr :: getRestartRedoLSN
   호출시 Log에 기록한다.
*/

#define SMR_LT_NULL                  (0)

#define SMR_LT_DUMMY                 (1)

    /* Checkpoint */
#define SMR_LT_CHKPT_BEGIN          (20)
#define SMR_LT_DIRTY_PAGE           (21)
#define SMR_LT_CHKPT_END            (22)

/* Transaction */
/* BUG-47525 */
#define SMR_LT_MEMTRANS_GROUPCOMMIT (37)
/*
 * BUG-24906 [valgrind] sdcUpdate::redo_SDR_SDC_UNBIND_TSS()에서 
 *           valgrind 오류가 발생합니다.
 * 디스크 트랜잭션와 하이브리드 트랜잭션의 Commit/Abort로그와
 * 메모리 트랜잭션의 Commit/Abort로그를 분류한다.
 */
#define SMR_LT_MEMTRANS_COMMIT      (38)
#define SMR_LT_MEMTRANS_ABORT       (39)
#define SMR_LT_DSKTRANS_COMMIT      (40)
#define SMR_LT_DSKTRANS_ABORT       (41)

#define SMR_LT_SAVEPOINT_SET        (42)
#define SMR_LT_SAVEPOINT_ABORT      (43)
#define SMR_LT_XA_PREPARE           (44)
#define SMR_LT_TRANS_PREABORT       (45)
// PROJ-1442 Replication Online 중 DDL 허용
#define SMR_LT_DDL                  (46)
// PROJ-1705 Disk MVCC 리뉴얼
/* Prepare 트랜잭션이 사용하던 트랜잭션 세그먼트 정보를 복원 */
#define SMR_LT_XA_SEGS              (47)


  /*  PROJ-1362   replication for LOB */
#define SMR_LT_LOB_FOR_REPL         (50)

/*  PROJ-1723 [MDW/INTEGRATOR] Altibase Plugin 개발
   Table/Index/Sequence의
   Create/Alter/Drop DDL에 대해 Query String을 로깅한다.
*/
#define SMR_LT_DDL_QUERY_STRING     (51)


    /* Update */
#define SMR_LT_UPDATE               (60)

    /* Rollback And NTA(Nested Top Action)  And File And */
#define SMR_LT_NTA                  (61)
#define SMR_LT_COMPENSATION         (62)
#define SMR_LT_DUMMY_COMPENSATION   (63)
#define SMR_LT_FILE_BEGIN           (64)

    /* PRJ-1548 Tablespace Update */
#define SMR_LT_TBS_UPDATE           (65)

#define SMR_LT_FILE_END             (66)

/*-------------------------------------------
* DRDB 로그 타입 추가
*
* - 특정 page에 대한 physical or logical redo 로그타입
*   : SMR_DLT_REDOONLY
* - undo record 기록에 대한 redo-undo 로그 타입
*   : SMR_DLT_UNDOABLE
* - 연산에 대한 NTA로깅
*   : SMR_DLT_NTA
*-------------------------------------------*/

/*
 * PRJ-1548 User Memory Tablespace
 * 디스크로그타입이 추가되면 다음함수에도 추가해주어야 한다.
 * smrRecoveryMgr::isDiskLogType()
 */
#define SMR_DLT_REDOONLY            (80)
#define SMR_DLT_UNDOABLE            (81)
#define SMR_DLT_NTA                 (82)
#define SMR_DLT_COMPENSATION        (83)
#define SMR_DLT_REF_NTA             (85)

/* PROJ-2569 DK XA 2PC */
#define SMR_LT_XA_PREPARE_REQ       (86)
#define SMR_LT_XA_END               (87)

/* PROJ-2747 GlobalTx Consistency */
#define SMR_LT_XA_START_REQ         (88)

/*-------------------------------------------
 * Meta 로그 타입 추가
 *------------------------------------------*/
// PROJ-1442 Replication Online 중 DDL 허용
#define SMR_LT_TABLE_META           (100)

/*-------------------------------------------*/
// Log Type Name 구성방법
// SMR + Manager Name + Update 위치 + Action
//
// 예) SMR + SMM + MEMBASE + "Update Link"
//     = SMR_SMM_MEMBASE_UPDATE_LINK
/*-------------------------------------------*/

//========================================================
// smrUpdateLog 의 세부 분류 타입
typedef UShort smrUpdateType;

//========================================================
// smrUpdateType 의 변수에 기록될 값
#define SMR_PHYSICAL                            (0)

//SMM
#define SMR_SMM_MEMBASE_SET_SYSTEM_SCN          (1)
#define SMR_SMM_MEMBASE_ALLOC_PERS_LIST         (2)
#define SMR_SMM_MEMBASE_ALLOC_EXPAND_CHUNK      (3)
#define SMR_SMM_PERS_UPDATE_LINK                (4)
#define SMR_SMM_PERS_UPDATE_NEXT_FREE_PAGE_LINK (5)
#define SMR_SMM_MEMBASE_INFO                    (6)

//SMC
// 1) Table Header
#define SMR_SMC_TABLEHEADER_INIT                       (20)
#define SMR_SMC_TABLEHEADER_UPDATE_INDEX               (21)
#define SMR_SMC_TABLEHEADER_UPDATE_COLUMNS             (22)
#define SMR_SMC_TABLEHEADER_UPDATE_INFO                (23)
#define SMR_SMC_TABLEHEADER_SET_NULLROW                (24)
#define SMR_SMC_TABLEHEADER_UPDATE_ALL                 (25)
#define SMR_SMC_TABLEHEADER_UPDATE_ALLOCINFO           (26)
#define SMR_SMC_TABLEHEADER_UPDATE_FLAG                (27)
#define SMR_SMC_TABLEHEADER_SET_SEQUENCE               (28)
#define SMR_SMC_TABLEHEADER_UPDATE_COLUMN_COUNT        (29)
#define SMR_SMC_TABLEHEADER_UPDATE_TABLE_SEGMENT       (30)
#define SMR_SMC_TABLEHEADER_SET_SEGSTOATTR             (31)
#define SMR_SMC_TABLEHEADER_SET_INSERTLIMIT            (32)
#define SMR_SMC_TABLEHEADER_SET_INCONSISTENCY          (33) /* PROJ-2162 */

// 2) Index Header
#define SMR_SMC_INDEX_SET_FLAG                   (40)
#define SMR_SMC_INDEX_SET_SEGATTR                (41)
#define SMR_SMC_INDEX_SET_SEGSTOATTR             (42)
#define SMR_SMC_INDEX_SET_DROP_FLAG              (43)

// 3) Pers Page
#define SMR_SMC_PERS_INIT_FIXED_PAGE             (60)
#define SMR_SMC_PERS_INIT_FIXED_ROW              (61)
#define SMR_SMC_PERS_UPDATE_FIXED_ROW            (62)
#define SMR_SMC_PERS_UPDATE_FIXED_ROW_NEXT_VERSION  (64)
#define SMR_SMC_PERS_SET_FIX_ROW_DROP_FLAG       (66)
#define SMR_SMC_PERS_SET_FIX_ROW_DELETE_BIT      (67)
#define SMR_SMC_PERS_INIT_VAR_PAGE               (68)
#define SMR_SMC_PERS_UPDATE_VAR_ROW_HEAD         (69)
#define SMR_SMC_PERS_UPDATE_VAR_ROW              (70)
#define SMR_SMC_PERS_SET_VAR_ROW_FLAG            (71)
#define SMR_SMC_PERS_SET_VAR_ROW_NXT_OID         (72)
#define SMR_SMC_PERS_WRITE_LOB_PIECE             (73)
#define SMR_SMC_PERS_SET_INCONSISTENCY           (74) /* PROJ-2162 */

// 4) Replication
#define SMR_SMC_PERS_INSERT_ROW                  (80)
#define SMR_SMC_PERS_UPDATE_INPLACE_ROW          (81)
#define SMR_SMC_PERS_UPDATE_VERSION_ROW          (82)
#define SMR_SMC_PERS_DELETE_VERSION_ROW          (83)
/* PROJ-2429 Dictionary based data compress for on-disk DB */
#define SMR_SMC_SET_CREATE_SCN                   (84)

/* ------------------------------------------------
 * LOG HEADER
 * BUG-35392 적용을 위해 UChar -> UInt로 변경 됨
 * ----------------------------------------------*/
#define SMR_LOG_TYPE_MASK               (0x00000003)
#define SMR_LOG_TYPE_NORMAL             (0x00000000)
#define SMR_LOG_TYPE_REPLICATED         (0x00000001)
#define SMR_LOG_TYPE_REPL_RECOVERY      (0x00000002)

#define SMR_LOG_SAVEPOINT_MASK          (0x00000004)
#define SMR_LOG_SAVEPOINT_NO            (0x00000000)
#define SMR_LOG_SAVEPOINT_OK            (0x00000004)

#define SMR_LOG_BEGINTRANS_MASK         (0x00000008)
#define SMR_LOG_BEGINTRANS_NO           (0x00000000)
#define SMR_LOG_BEGINTRANS_OK           (0x00000008)

/* BUG-14513 : Insert, update, delete시 allot slot시 alloc Slot log제거.
 * DML Log(insert, update, delete로그가 Alloc Slot에 대한
 * 로그도 포함하고 있는지 표시 */
#define SMR_LOG_ALLOC_FIXEDSLOT_MASK    (0x00000010)
#define SMR_LOG_ALLOC_FIXEDSLOT_NO      (0x00000000)
#define SMR_LOG_ALLOC_FIXEDSLOT_OK      (0x00000010)

/* TASK-2398 Log Compress
 * Log 압축을 원하지 않는 경우,
 * smrLogHead(비압축 Log Head)의 Flag에 이 플래그를 설정한다
 *
 * Disk Log의 경우 Mini Transaction이 접근한 여러 Tablespace 중
 * Log 압축을 하지 않도록 설정된 Tablespace가 하나라도 있으면
 * 이 Flag를 설정하여 로그를 압축하지 않도록 한다. */
#define SMR_LOG_FORBID_COMPRESS_MASK    (0x00000020)
#define SMR_LOG_FORBID_COMPRESS_NO      (0x00000000)
#define SMR_LOG_FORBID_COMPRESS_OK      (0x00000020)

/* TASK-5030 Full XLog */
#define SMR_LOG_FULL_XLOG_MASK          (0x00000040)
#define SMR_LOG_FULL_XLOG_NO            (0x00000000)
#define SMR_LOG_FULL_XLOG_OK            (0x00000040)

/* 해당 로그 레코드가 압축되었는지 여부
 * 이 Flag는 압축로그, 비압축로그의 첫번째 바이트에 기록된다.
 * 이 Flag를 통해 로그를 압축로그로 해석할지,
 * 비압축 로그로 해석할지 여부를 판다름한다. */
#define SMR_LOG_COMPRESSED_MASK         (0x00000080)
#define SMR_LOG_COMPRESSED_NO           (0x00000000) // 압축안한 로그
#define SMR_LOG_COMPRESSED_OK           (0x00000080) // 압축한 로그

/* BUG-35392
 * dummy log 인지 판단하기 위해 사용 */
#define SMR_LOG_DUMMY_LOG_MASK          (0x00000100)
#define SMR_LOG_DUMMY_LOG_NO            (0x00000000) // 정상 로그
#define SMR_LOG_DUMMY_LOG_OK            (0x00000100) // 더미 로그

/* BUG-46854: RP 정보(프라이머리 키) 로그 작성 여부  */
#define SMR_LOG_RP_INFO_LOG_MASK        (0x00000200)
#define SMR_LOG_RP_INFO_LOG_NO          (0x00000000) // 로그 안 씀
#define SMR_LOG_RP_INFO_LOG_OK          (0x00000200) // 로그 씀 

/* BUG-46854: CMPS 로그 여부
 * delete를 롤백했을 때, redo 과정에서 CMPS로그(CLR)보고 undo함.
 * 이 경우 before flag만 로그에 있기 때문에 delete version의 undo
 * 함수에서 CLR에 의한 delete undo 인지 아닌지 알아야 한다. */
#define SMR_LOG_CMPS_LOG_MASK           (0x00000400)
#define SMR_LOG_CMPS_LOG_NO             (0x00000000) // CMPS 로그 아님 
#define SMR_LOG_CMPS_LOG_OK             (0x00000400) // CMPS 로그


/* BUG-48059 주의주의 
 * smrLogHead.mFlag 에 저장하는 flag 아님
 * GCTX Group CommitSCN 을 저장했는지 구분하기 위한 용도 
 * SMR_LOG_TYPE_XXX 과 구별되면 됨 */
#define SMR_LOG_COMMITSCN_MASK          (0x00000004)
#define SMR_LOG_COMMITSCN_NO            (0x00000000) // CommitSCN 이 없거나 INIT 인 경우 
#define SMR_LOG_COMMITSCN_OK            (0x00000004) // GCTX 면서 CommitSCN 을 기록한경우

/* PROJ-1527              */
/* smrLogHead 구조체 축소 */
/*
   Tail 을 Log Record 구조체 안에 가지는 로그 레코드의 크기 계산
   ======================================================================
   PROJ-1527에 의해 log는 8바이트 align될 필요가 없게되었으며
   따라서 smrXXXLog를 정의할 때 align을 고려할 필요가 없다.
   하지만 64비트 환경에서 8바이트 멤버를 가진 구조체의 크기는
   8의 배수로 구해지기 때문에 log를 기록할 때 ID_SIZEOF를
   사용하면 안된다. (낭비를 초래함)
   따라서 로그를 기록할 때 smrXXXLog의 마지막 멤버까지만 기록하면
   되는데 이 마지막 멤버까지의 길이를 구하기 위해 모든 smrXXXLog에는
   mLogRecFence라는 더미 멤버가 존재한다.
   SMR_LOGREC_SIZE는 smrXXXLog 구조체에서 mLogRecFence 앞까지의 크기를 구한다.

   +----------+--------------------+----------+-------+-------+
   | Log Head | Log Body                      | Fence | Dummy |
   +----------+--------------------+----------+-------+-------+
                                              ^
                                              |
                                             size
   따라서 head에 setSize를 하기위해서는 ID_SIZEOF대신 SMR_LOGREC_SIZE를
   써야한다.
   그리고 로그양을 줄이기 위해서는 8바이트 멤버를 앞쪽에, 작은 사이즈의 멤버를
   뒤쪽에 두는게 현명하다.
*/
#define SMR_LOGREC_SIZE(STRUCT_NAME) (offsetof(STRUCT_NAME,mLogRecFence))

/* BUG-35392
 * mFlag 에 추가 정보를 넣기 위해 사이즈를 UChar -> UInt로
 * 변경하고, 멤버의 순서를 바꾼다. */
typedef struct smrLogHead
{
    /* mFlag는 압축로그와 비압축로그의 공통 Head이다. */
    UInt            mFlag;

    UInt            mSize;

    /* For Parallel Logging :
     * 로그가 기록될 때 LSN(Log Sequence Number)
     * 값을 기록함 */
    smLSN           mLSN;

    /* 로그파일 생성시 memset하지 않아서 garbage가 올라와도
     * Invalid한 Log를 Valid한 로그로 판별하는 확률을 낮추기 위한
     * Magic Number .
     * 로그파일 번호와 로그레코드가 기록되는 오프셋의 조합으로 만들어진다. */
    smMagic         mMagic;

    smrLogType      mType;

    /* BUG-17033: 최상위 Statement가 아닌 Statment에 대해서도
     * Partial Rollback을 지원해야 합니다.
     *
     * Replication이 Statment의 Savepoint를 설정할때 이용해야할
     * Implicit SVN Name Number. */
    UChar           mReplSvPNumber;

    smTID           mTransID;

    smLSN           mPrevUndoLSN;
} smrLogHead;

/* BUG-35392 */
#define SMR_LOG_FLAG_OFFSET     (offsetof(smrLogHead,mFlag))
#define SMR_LOG_LOGSIZE_OFFSET  (offsetof(smrLogHead,mSize))
#define SMR_LOG_LSN_OFFSET      (offsetof(smrLogHead,mLSN))
#define SMR_LOG_MAGIC_OFFSET    (offsetof(smrLogHead,mMagic))

typedef smrLogType smrLogTail;

#define SMR_DEF_LOG_SIZE (ID_SIZEOF(smrLogHead) + ID_SIZEOF(smrLogTail))

/* ------------------------------------------------
 * Operation Log Type
 * ----------------------------------------------*/
typedef enum
{
    SMR_OP_NULL,
    SMR_OP_SMM_PERS_LIST_ALLOC,
    SMR_OP_SMC_FIXED_SLOT_ALLOC,
    SMR_OP_CREATE_TABLE,
    SMR_OP_CREATE_INDEX,
    SMR_OP_DROP_INDEX,
    SMR_OP_INIT_INDEX,
    SMR_OP_SMC_FIXED_SLOT_FREE, /* BUG-31062 로 인하여 제거되었으나*/
    SMR_OP_SMC_VAR_SLOT_FREE,   /* 이후 Log Type의 값을 유지하기 위해 남겨둠*/
    SMR_OP_ALTER_TABLE,
    SMR_OP_SMM_CREATE_TBS,
    SMR_OP_INSTANT_AGING_AT_ALTER_TABLE,
    SMR_OP_SMC_TABLEHEADER_ALLOC,
    SMR_OP_DIRECT_PATH_INSERT,
    SMR_OP_MAX
} smrOPType;


/* ------------------------------------------------
 * FOR A4 : DRDB의 로그 타입
 *
 * 1) 특정 page에 대한 physical or logical redo 로그타입
 * smrLogType으로 SMR_DLT_REDO 타입을 가지며, redo 전용
 * 타입이다.
 *
 * 2) undo record 기록에 대한 redo-undo 로그 타입
 * smrLogType으로SMR_DLT_UNDOREC타입을 가지며, redo 및
 * undo 가능한 타입이다.
 * ----------------------------------------------*/
typedef struct smrDiskLog
{
    smrLogHead  mHead;        /* 로그 header */
    smOID       mTableOID;
    UInt        mRefOffset;   /* disk log buffer에서 DML관련
                                 redo/undo 로그 위치 */
    UInt        mContType;    /* 로그가 나누어졌는지 아닌지 여부 */
    UInt        mRedoLogSize; /* redo log 크기 */
    UInt        mRedoType;    /* runtime memory 데이타 포함여부 */
    UChar       mLogRecFence; /* log record의 크기를 구하기 위해 사용됨 */
} smrDiskLog;


/* ------------------------------------------------
 * PROJ-1566 : disk NTA 로그
 * < extent 할당받을 때 >
 * - mData1 : segment RID
 * - mData2 : extent RID
 * < page list를 meta의 page list에 추가할 때 >
 * - mData1 : table OID
 * - mData2 : tail Page PID
 * - mData3 : 생성된 page들 중, table type의 page 개수
 * - mData4 : 생성된 모든 page 개수 ( multiple, external 포함 )
 * ----------------------------------------------*/
typedef struct smrDiskNTALog
{
    smrLogHead  mHead;        /* 로그 header */
    ULong       mData[ SM_DISK_NTALOG_DATA_COUNT ] ;
    UInt        mDataCount;   /* Data 개수 */
    UInt        mOPType;      /* operation NTA 타입 */
    UInt        mRedoLogSize; /* redo log 크기 */
    scPageID    mSpaceID;     /* SpaceID */
    UChar       mLogRecFence; /* log record의 크기를 구하기 위해 사용됨 */
} smrDiskNTALog;

/* ------------------------------------------------
 * PROJ-1704 : MVCC Renewal
 * ----------------------------------------------*/
typedef struct smrDiskRefNTALog
{
    smrLogHead  mHead;        /* 로그 header */
    UInt        mOPType;      /* operation NTA 타입 */
    UInt        mRefOffset;   /* disk log buffer에서 index관련
                                 logical undo 로그 위치 */
    UInt        mRedoLogSize; /* redo log 크기 */
    scPageID    mSpaceID;     /* SpaceID */
    UChar       mLogRecFence; /* log record의 크기를 구하기 위해 사용됨 */
} smrDiskRefNTALog;

/* ------------------------------------------------
 * disk CLR 로그
 * smrLogType으로 SMR_DLT_COMPENSATION
 * ----------------------------------------------*/
typedef struct smrDiskCMPSLog
{
    smrLogHead      mHead;        /* 로그 header */
    UInt            mRedoLogSize; /* redo log 크기 */
    UChar           mLogRecFence; /* log record의 크기를 구하기 위해 사용됨 */
} smrDiskCMPSLog;

/* ------------------------------------------------
 * PROJ-1867 Disk Page Img Log
 * DPath-Insert의 Page Img Log와 기본 구성이 같다.
 * ----------------------------------------------*/
typedef struct smrDiskPILog
{
    smrLogHead  mHead;        /* 로그 header */
    smOID       mTableOID;
    UInt        mRefOffset;   /* disk log buffer에서 DML관련
                                 redo/undo 로그 위치 */
    UInt        mContType;    /* 로그가 나누어졌는지 아닌지 여부 */
    UInt        mRedoLogSize; /* redo log 크기 */
    UInt        mRedoType;    /* runtime memory 데이타 포함여부 */

    sdrLogHdr   mDiskLogHdr;

    UChar       mPage[SD_PAGE_SIZE]; /* page */
    smrLogTail  mTail;
    UChar       mLogRecFence; /* log record의 크기를 구하기 위해 사용됨 */
} smrDiskPILog;

/* ------------------------------------------------
 * PROJ-1864 Page consistent log
 * ----------------------------------------------*/
typedef struct smrPageConsistentLog
{
    smrLogHead  mHead;        /* 로그 header */
    smOID       mTableOID;
    UInt        mRefOffset;   /* disk log buffer에서 DML관련
                                 redo/undo 로그 위치 */
    UInt        mContType;    /* 로그가 나누어졌는지 아닌지 여부 */
    UInt        mRedoLogSize; /* redo log 크기 */
    UInt        mRedoType;    /* runtime memory 데이타 포함여부 */

    sdrLogHdr   mDiskLogHdr;

    UChar       mPageConsistent; /* page consistent */
    smrLogTail  mTail;
    UChar       mLogRecFence; /* log record의 크기를 구하기 위해 사용됨 */
} smrPageCinsistentLog;

/* ------------------------------------------------
 * 테이블스페이스 UPDATE에 대한 로그 BUG-9640
 * ----------------------------------------------*/
typedef struct smrTBSUptLog
{
    smrLogHead  mHead;        /* 로그 header */
    scSpaceID   mSpaceID;     /* 해당 tablespace ID */
    UInt        mFileID;
    UInt        mTBSUptType;  /* file 연산 타입 */
    SInt        mAImgSize;    /* after image 크기 */
    SInt        mBImgSize;    /* before image 크기 */
    UChar       mLogRecFence; /* log record의 크기를 구하기 위해 사용됨 */
} smrTBSUptLog;

/* ------------------------------------------------
 *  NTA log
 * ----------------------------------------------*/
typedef struct smrNTALog
{
    smrLogHead mHead;
    scSpaceID  mSpaceID;
    ULong      mData1;
    ULong      mData2;
    smrOPType  mOPType;
    smrLogTail mTail;
    UChar      mLogRecFence; /* log record의 크기를 구하기 위해 사용됨 */
} smrNTALog;

/*---------------------------------------------------------------
 * FOR A4 :  checkpoint log
 * DRDB의 recovery LSN을 기록해야함
 *---------------------------------------------------------------*/
typedef struct smrBeginChkptLog
{
    smrLogHead     mHead;
    /* redoall과정에서 MRDB의 recovery lsn */
    smLSN          mEndLSN;
    /* redoall과정에서 DRDB의 recovery lsn */
    smLSN          mDiskRedoLSN;
    /* PROJ-2569 미완료분산트랜잭션 로그와 mDiskRedoLSN 중에서 작은 값으로 redo시작 */
    smLSN          mDtxMinLSN;
    smrLogTail     mTail;
    UChar          mLogRecFence; /* log record의 크기를 구하기 위해 사용됨 */
} smrBeginChkptLog;

typedef struct smrEndChkptLog
{
    smrLogHead     mHead;
    smrLogTail     mTail;
    UChar          mLogRecFence; /* log record의 크기를 구하기 위해 사용됨 */
} smrEndChkptLog;

/*---------------------------------------------------------------*/
//  transaction log
//
/*---------------------------------------------------------------*/

/* PROJ-1553 Replication self-deadlock */
/* undo log과 abort log를 찍기 전에 pre-abort log를 찍는다. */
typedef struct smrTransPreAbortLog
{
    smrLogHead     mHead;
    smrLogTail     mTail;
    UChar          mLogRecFence; /* log record의 크기를 구하기 위해 사용됨 */
} smrTransPreAbortLog;

typedef struct smrTransAbortLog
{
    smrLogHead     mHead;
    UInt           mDskRedoSize; /* Abort Log는 Disk 의 경우 TSS 변경함 */
    UInt           mGlobalTxId;
    UChar          mLogRecFence; /* log record의 크기를 구하기 위해 사용됨 */
} smrTransAbortLog;

typedef struct smrTransCommitLog // 디스크/메모리 공통
{
    smrLogHead     mHead;
    UInt           mTxCommitTV;
    UInt           mDskRedoSize; /* Commit Log는 Disk 의 경우 TSS 변경함 */
    UInt           mGlobalTxId;
    smSCN          mCommitSCN;   /* Global Trans의 commitSCN */
    UChar          mLogRecFence; /* log record의 크기를 구하기 위해 사용됨 */
} smrTransCommitLog;

typedef struct smrTransMemCommitLog // 메모리용  // smrTransCommitLog 랑 구조는 같다. 단 고정길이.
{
    smrLogHead     mHead;
    UInt           mTxCommitTV;
    UInt           mDskRedoSize; /* Commit Log는 Disk 의 경우 TSS 변경함 */
    UInt           mGlobalTxId;
    smSCN          mCommitSCN;   /* Global Trans의 commitSCN */
    smrLogTail     mTail;
    UChar          mLogRecFence; /* log record의 크기를 구하기 위해 사용됨 */
} smrTransMemCommitLog;

typedef struct smrTransGroupCommitLog
{
    smrLogHead     mHead;
    UInt           mTxCommitTV;
    UInt           mGroupCnt;
    UChar          mLogRecFence; /* log record의 크기를 구하기 위해 사용됨 */
} smrTransGroupCommitLog;

#define SMR_LOG_GROUP_COMMIT_GROUPCNT_OFFSET   (offsetof(smrTransGroupCommitLog,mGroupCnt))

/*--------------------------
    For Global Transaction
  --------------------------*/
typedef struct smrXaPrepareLog
{
    smrLogHead     mHead;
    /* BUG-18981 */
    ID_XID         mXaTransID;
    UInt           mLockCount;
    idBool         mIsGCTx;        /* PROJ-2733 */
    timeval        mPreparedTime;
    smSCN          mFstDskViewSCN; /* XA Trans의 FstDskViewSCN */
    UChar          mLogRecFence;   /* log record의 크기를 구하기 위해 사용됨 */
} smrXaPrepareLog;

/* BUG-2569 */
#define SM_INIT_GTX_ID ( ID_UINT_MAX )

typedef struct smrXaStartReqLog
{
    smrLogHead     mHead;
    ID_XID         mGlobalXID;   /* PROJ-2747 Global Tx Consistent */
    smrLogTail     mTail;
    UChar          mLogRecFence; /* log record의 크기를 구하기 위해 사용됨 */
} smrXaStartReqLog;

typedef struct smrXaPrepareReqLog
{
    smrLogHead     mHead;
    ID_XID         mGlobalXID;   /* PROJ-2747 Global Tx Consistent */
    UInt           mGlobalTxId;
    UInt           mBranchTxSize;
    UChar          mLogRecFence; /* log record의 크기를 구하기 위해 사용됨 */
} smrXaPrepareReqLog;

typedef struct smrXaEndLog
{
    smrLogHead     mHead;
    UInt           mGlobalTxId;
    smrLogTail     mTail;
    UChar          mLogRecFence; /* log record의 크기를 구하기 위해 사용됨 */
} smrXaEndLog;

/*
 * PROJ-1704 Disk MVCC 리뉴얼
 * Prepare 트랜잭션이 사용하던 트랜잭션 세그먼트에 대한 로깅
 */
typedef struct smrXaSegsLog
{
    smrLogHead     mHead;
    ID_XID         mXaTransID;
    UInt           mTxSegEntryIdx;  /* 사용했던 트랜잭션 세그먼트 엔트리순번 */
    sdRID          mExtRID4TSS;     /* TSS를 포함한 ExtRID */
    scPageID       mFstPIDOfLstExt4TSS; /* TSS를 할당한 Ext의 첫번째 페이지ID */
    sdRID          mFstExtRID4UDS;  /* 처음 사용했던 Undo ExtRID */
    sdRID          mLstExtRID4UDS;  /* 마지막 사용했던 Undo ExtRID */
    scPageID       mFstPIDOfLstExt4UDS; /* 마지막 Undo Ext의 첫번째 페이지ID */
    scPageID       mFstUndoPID;     /* 마지막 사용했던 TSS PID */
    scPageID       mLstUndoPID;     /* 마지막 사용했던 Undo PID */
    smrLogTail     mTail;
    UChar          mLogRecFence;    /* log record의 크기를 구하기 위해 사용 */
} smrXaSegsLog;

/*---------------------------------------------------------------*/
//  Log File Begin & End Log
//
/*---------------------------------------------------------------*/

/*
 * To Fix BUG-11450  LOG_DIR, ARCHIVE_DIR 의 프로퍼티 내용이 변경되면
 *                   DB가 깨짐
 *
 * 로그타입 : SMR_LT_FILE_BEGIN
 */

typedef struct smrFileBeginLog
{
    smrLogHead    mHead;
    UInt          mFileNo;   // 로그파일의 번호
    smrLogTail    mTail;
    UChar         mLogRecFence; /* log record의 크기를 구하기 위해 사용됨 */
} smrFileBeginLog;

typedef struct smrFileEndLog
{
    smrLogHead    mHead;
    smrLogTail    mTail;
    UChar         mLogRecFence; /* log record의 크기를 구하기 위해 사용됨 */
} smrFileEndLog;

/*---------------------------------------------------------------*/
//  Dummy Log
//  어떤 Undo Redo연산시 하지 않는다. 자세한 사항은
//  1. smiReadLogByOrder::initialize
//  2. smrLogMgr :: getRestartRedoLSN
//  를 참조하기 바란다.
/*---------------------------------------------------------------*/
typedef struct smrDummyLog
{
    smrLogHead    mHead;
    smrLogTail    mTail;
    UChar         mLogRecFence; /* log record의 크기를 구하기 위해 사용됨 */
} smrDummyLog;

/*---------------------------------------------------------------*/
//  update log
//
/*---------------------------------------------------------------*/
typedef struct smrUpdateLog
{
    smrLogHead         mHead;
    vULong             mData;
    scGRID             mGRID;
    SInt               mAImgSize;
    SInt               mBImgSize;
    smrUpdateType      mType;
    UChar              mLogRecFence; /* log record의 크기를 구하기 위해 사용됨 */
} smrUpdateLog;

/* ------------------------------------------------
 * compensation log BUG-12399 CASE-3893
 * ----------------------------------------------*/
typedef struct smrCMPSLog
{
    smrLogHead    mHead;
    scGRID        mGRID;
    UInt          mFileID;
    UInt          mTBSUptType;  /* file 연산 타입 */
    vULong        mData;
    smrUpdateType mUpdateType;
    SInt          mBImgSize;
    UChar         mLogRecFence; /* log record의 크기를 구하기 위해 사용됨 */
} smrCMPSLog;

// PROJ-1362.
typedef struct smrLobLog
{
    smrLogHead    mHead;
    smLobLocator  mLocator;
    UChar         mOpType;     // lob operation code
    UChar         mLogRecFence; /* log record의 크기를 구하기 위해 사용됨 */
}smrLobLog;

/* PROJ-1442 Replication Online 중 DDL 허용 */
typedef struct smrDDLLog
{
    smrLogHead    mHead;
    UChar         mLogRecFence; /* log record의 크기를 구하기 위해 사용됨 */
} smrDDLLog;

typedef struct smrTableMeta
{
    /* Table Identifier */
    UInt          mTableID;
    vULong        mTableOID;
    vULong        mOldTableOID;
    UInt          mTBSType; 
    SChar         mRepName[SM_MAX_NAME_LEN + 1 + 7];    // 7 Byte는 Dummy
    SChar         mUserName[SM_MAX_NAME_LEN + 1 + 7];   // 7 Byte는 Dummy
    SChar         mTableName[SM_MAX_NAME_LEN + 1 + 7];  // 7 Byte는 Dummy
    SChar         mPartName[SM_MAX_NAME_LEN + 1 + 7];   // 7 Byte는 Dummy
    SChar         mRemoteUserName[SM_MAX_NAME_LEN + 1 + 7];   // 7 Byte는 Dummy
    SChar         mRemoteTableName[SM_MAX_NAME_LEN + 1 + 7];  // 7 Byte는 Dummy
    SChar         mRemotePartName[SM_MAX_NAME_LEN + 1 + 7];   // 7 Byte는 Dummy

    /* Primary Key Index ID */
    UInt          mPKIndexID;
} smrTableMeta;

typedef struct smrTableMetaLog
{
    smrLogHead    mHead;
    smrTableMeta  mTableMeta;
    UChar         mLogRecFence; /* log record의 크기를 구하기 위해 사용됨 */
} smrTableMetaLog;

/* ------------------------------------------------
 * PROJ-1723
 * ----------------------------------------------*/
typedef struct smrDDLStmtMeta
{
    SChar          mUserName[SM_MAX_NAME_LEN + 1 + 7];
    SChar          mTableName[SM_MAX_NAME_LEN + 1 + 7];
} smrDDLStmtMeta;

typedef struct smrXaLog
{
    ID_XID mXID;
} smrXaLog;

/*---------------------------------------------------------------
 * - loganchor 구조체
 * 다중 Loganchor를 지원하도록 설계
 * 추가된 부분은 checksum과 DRDB의 tablespace 정보를 유지
 *---------------------------------------------------------------*/
typedef struct smrLogAnchor
{
    UInt             mCheckSum;

    smLSN            mBeginChkptLSN;
    smLSN            mEndChkptLSN;

    /* BEGIN CHKPT 로그에도 저장되며, 버퍼 체크포인트(DRDB)시에는
     * 별도의 BEGIN CHKPT 로그를 기록하지 않으므로 바로 Loganchor에만
     * 갱신한다. Restart Recovery 과정에서 BEGIN CHKPT 로그에 저장된 것과
     * Loganchor에 저장된 것을 비교하여 더 큰 것을 선택한다. */
    smLSN            mDiskRedoLSN;

    /*
     * mMemEndLSN            : Restart Redo Point
     * mLstCreatedLogFileNo  : 마지막으로 생성한 LogFile No
     * mFstDeleteFileNo      : 지워진 첫번째 LogFile No
     * mLstDeleteFileNo      : 지워진 마지막 LogFile No
     * mResetLSN             : 비완료 Recovery후 mResetLogs가 가리키는 이후
     *                         log는 버린다.
     */
    smLSN            mMemEndLSN;
    UInt             mLstCreatedLogFileNo;
    UInt             mFstDeleteFileNo;
    UInt             mLstDeleteFileNo;
    smLSN            mMediaRecoveryLSN;

    smLSN            mResetLSN;

    smrServerStatus  mServerStatus;

    smiArchiveMode   mArchiveMode;

    /* PROJ-1704 Disk MVCC 리뉴얼
     * 트랜잭션 세그먼트 엔트리 개수 */
    UInt             mTXSEGEntryCnt;

    UInt             mNewTableSpaceID;

    UInt             mSmVersionID;
    /* proj-1608 recovery from replication */
    smLSN            mReplRecoveryLSN;

    /* change in run-time */
    ULong            mUpdateAndFlushCount; /* Loganchor 파일에 Flush된 횟수 */

    /* PROJ-2133 incremental backup */
    smriCTFileAttr   mCTFileAttr;
    smriBIFileAttr   mBIFileAttr;

    /* PROJ-1497 DB Migration을 방지하기 위해 Reserved 영역을 추가 */
    ULong            mReserveArea[SMR_LOGANCHOR_RESERV_SIZE];
} smrLogAnchor;

/*---------------------------------------------------------------*/
//  For Savepoint Log
//
/*---------------------------------------------------------------*/
#define SMR_IMPLICIT_SVP_NAME           "$$IMPLICIT"
#define SMR_IMPLICIT_SVP_NAME_SIZE      (10)

/* checkpoint reason */
#define SMR_CHECKPOINT_BY_SYSTEM           (0)
#define SMR_CHECKPOINT_BY_LOGFILE_SWITCH   (1)
#define SMR_CHECKPOINT_BY_TIME             (2)
#define SMR_CHECKPOINT_BY_USER             (3)

typedef struct smrArchLogFile
{
    UInt               mFileNo;
    smrArchLogFile    *mArchNxtLogFile;
    smrArchLogFile    *mArchPrvLogFile;
} smrArchLogFile;

/* BUG-14778 Tx의 Log Buffer를 인터페이스로 사용해야 합니다.
 *
 * Code Refactoring을 위해서 추가 되었습니다. After Image와
 * Before Image에 대한 정보를 표현하는데 log기록함수에 여러개의
 * After Image, Before Image에 대한 정보를 넘기기 위해서
 * 사용됩니다.
 *
 * log기록함수에는 image갯수, smrUptLogImgInfo mArr[image갯수]
 * 로 각각의 After , Before Image에 대한 정보가 넘어갑니다.
 *
 * */
typedef struct smrUptLogImgInfo
{
    SChar *mLogImg; /* Log Image에 대한 Pointer */
    UInt   mSize;   /* Image size(byte) */
} smrUptLogImgInfo;

/* Log Flush를 요청하는 경우를 분류 */
typedef enum
{
    /* Transaction */
    SMR_LOG_SYNC_BY_TRX = 0,
    /* Log Flush Thread */
    SMR_LOG_SYNC_BY_LFT,
    /* Buffer Flush Thread */
    SMR_LOG_SYNC_BY_BFT,
    /* Checkpoint Thread */
    SMR_LOG_SYNC_BY_CKP,
    /* Refine */
    SMR_LOG_SYNC_BY_REF,
    /* Other */
    SMR_LOG_SYNC_BY_SYS
} smrSyncByWho;

/* TASK-2398 Log Compress
   Disk Log를 기록하는 함수들에 aWriteOption인자에 넘어갈 Flag
*/
// 로그 압축여부
#define SMR_DISK_LOG_WRITE_OP_COMPRESS_MASK  (0x01)
#define SMR_DISK_LOG_WRITE_OP_COMPRESS_FALSE (0x00)
#define SMR_DISK_LOG_WRITE_OP_COMPRESS_TRUE  (0x01)

#define SMR_DEFAULT_DISK_LOG_WRITE_OPTION SMR_DISK_LOG_WRITE_OP_COMPRESS_TRUE

/* 로그 압축에 필요한 리소스 */
typedef struct smrCompRes
{
    // 마지막으로 사용된 시각
    // 마지막으로 사용된 시각과 현재시각의 차이가 특정 시간을 벗어나면
    // Pool에서 제거하기 위한 용도로 사용된다.
    idvTime                mLastUsedTime;

    // 트랜잭션 rollback때 로그 압축 해제를 위한 버퍼의 핸들
    iduReusedMemoryHandle  mDecompBufferHandle;

    // 로그 압축에 사용할 압축 버퍼
    iduReusedMemoryHandle  mCompBufferHandle;

} smrCompRes;

// smrLSN4Union은 atomic 하게  mSync로 읽은뒤 mLstLSN.mFileNo 및 mLstLSN. mOffset을 가져올때 사용
typedef struct smrLSN4Union
{
    union
    {
        ULong mSync; // For 8 Byte Align
        smLSN mLSN;
    } ;
} smrLSN4Union;

/********** BUG-35392 **********/
// FAST_UNLOCK_LOG_ALLOC_MUTEXT = 1 인 경우 dummyLog 가 발생할수 있는데 
// 경우에 따라 dummy 를 포함하지 않는 로그를 구해야 한다. 
// 로그를 남기기 전에 LstLSN 과 LstWriteLSN 을 저장하고 setFstCheckLSN()  
// 주기적으로 LstLSN 과 LstWriteLSN 의 가장 작은 값을 갱신하면 rebuildMinUCSN()
// 해당값이 쓰기중인 Log가 아닌 (혹은 더미가 아닌) 쓰기가 보장된 LogLSN 임.
//
//  -----------------------------------------------------
//   LSN    | 100 | 101 | 102   | 103 | 104   | 105 | 106 |
//   Status | ok  | ok  | dummy | ok  | dummy | ok  | ok  |
//  ------------- A --- B-------------------------- C --- D --
// 
// smrUncompletedLogInfo.mLstWriteLSN (A) : 더미를 포함하지 않는 마지막 로그 레코드의 LSN
// smrUncompletedLogInfo.mLstLSN (B)      : 더미를 포함하지 않는 마지막 로그 레코드의 Offset
// mLstWriteLSN (C) : 마지막 로그 레코드의 LSN, FAST_UNLOCK_LOG_ALLOC_MUTEXT = 1인 경우 더미 포함 
// mLstLSN (D)      : 마지막 로그 레코드의 Offset, FAST_UNLOCK_LOG_ALLOC_MUTEXT = 1인 경우 더미 포함
// 
// A,B,C,D 는.. 
// A 와 B , C와 D 는 각각 같은 로그의 시작과 끝.
// A <= C , B <= D 가 성립된다. 

typedef struct smrUncompletedLogInfo
{
    smrLSN4Union mLstLSN;
    smrLSN4Union mLstWriteLSN;  
} smrUncompletedLogInfo;

#define SM_SYNC_LSN_MAX( mSync )     (mSync = ID_ULONG_MAX)

#define SM_IS_SYNC_LSN_MAX( mSync )  (mSync == ID_ULONG_MAX)

/********** END **********/


/* TASK-4007 [SM] PBT를 위한 기능 추가
 * Log를 Dump할때, 어떤 로그를 어떻게 출력할지를 서버가 아닌 Dump하고자 하는 
 * Util에서 결정하도록, 읽어드린 로그에 대한 처리용 Callback함수를 둔다.*/

typedef IDE_RC (*smrLogDump)(UInt         aOffsetInFile,
                             smrLogHead * aLogHead,
                             idBool       aIsCompressed,
                             SChar      * aLogPtr );

/************************************************************************
 * PROJ-2162 RestartRiskReduction
 *
 *  - Recovery Target Object Info (RTOI) 정의
 * Recovery 대상 객체가 Inconsistent한 상황에서 Recovery를 시도할 경우 어떤
 * 문제가 발생할지 모른다. 따라서 미리 Log를 바탕으로 Targetobject의
 * Consistency를 검사한다.
 *  또한 만약 Recovery 연산이 실패했을 경우, Recovery 대상 객체에 Inconsistent
 * Flag를 설정하여 이후 Recovery가 실패하는 일을 막아준다.
 * Recovery 대상 객체는 Table, Index, MemPage, DiskPage, 네가지이다. 
 * 모두 TBSID, PID, OFFSET, 즉 GRID로 정보를 표현한다. 
 *
 * - InconsistentObjectList (IOL)
 *  RTOI가 Inconsistent하다는 것이 결정되었을 경우, RecoveryMgr에 IOL로
 * 묶이게 된다.
 *
 ***********************************************************************/

/* TOI 상태:
 * <INIT> 초기화됨
 *    |
 * [prepareRTOI               Log에 따라 대상 Object를 가져옴 ]
 *    |                       
 * <PREPARED>                      
 *    |                       
 * [checkObjectConsistency    Object검사함]
 *    |                       
 * <CHECKED>                  
 *    |                       
 * [setObjectInconsistency    Object에 옳지 않음을 설정]
 *    |
 * <DONE>
 *
 * 단, prepareRTOI 함수에서 Log에 따라 CHECKED, 또는 DONE 상태로 바로 
 * 보내는 경우가 있음. (해당 함수 참조 ) */

typedef enum
{
    SMR_RTOI_STATE_INIT,
    SMR_RTOI_STATE_PREPARED,/* prepareRTOI에 의해 Log를 분석함*/
    SMR_RTOI_STATE_CHECKED, /* checkObjectConsistency를 통해 객체정보 습득*/
    SMR_RTOI_STATE_DONE     /* 처리 종료*/
} smrRTOIState;

/* TOI를 등록하게 된 원인 */
typedef enum
{
    SMR_RTOI_CAUSE_INIT,
    SMR_RTOI_CAUSE_OBJECT,   /* 객체(Table,index,Page)등이 이상함*/
    SMR_RTOI_CAUSE_REDO,     /* RedoRecovery에서 실패했음 */
    SMR_RTOI_CAUSE_UNDO,     /* UndoRecovery에서 실패했음 */
    SMR_RTOI_CAUSE_REFINE,   /* Refine에서 실패했음 */
    SMR_RTOI_CAUSE_PROPERTY  /* Property에 의해 강제로 제외됨  */
} smrRTOICause;

/* TOI에 등록된 대상 Object의 종류  */
typedef enum
{
    SMR_RTOI_TYPE_INIT,
    SMR_RTOI_TYPE_TABLE,
    SMR_RTOI_TYPE_INDEX,
    SMR_RTOI_TYPE_MEMPAGE,
    SMR_RTOI_TYPE_DISKPAGE
} smrRTOIType;

/* PROJ-2162 RestartRiskReduction
 * Recovery 대상 객체들입니다. Log로부터, 또는 DRDBPage로부터 추출합니다.
 * 이 자료구조를 바탕으로 잘못된 객체들의 List를 모아두어 마지막에 출력해
 * 줍니다.*/
typedef struct smrRTOI /* RecoveryTargetObjectInfo */
{
    /* TOI를 얻어낸 원본 객체 */
    smrRTOICause   mCause;
    smLSN          mCauseLSN;
    UChar        * mCauseDiskPage;

    /* TOI의 상태 */
    smrRTOIState   mState;

    /* TOI에 대한 정보 */
    smrRTOIType    mType;
    scGRID         mGRID;

    /* IndexCommonPersistentHeader등의 경우 Alter문에 따라 위치가 변경
     * 될 수 있기 때문에, TableHeader, IndexID를 가지고 있어야 함
     * checkObjectConsistency에 의해 설정됨*/
    smOID          mTableOID;
    UInt           mIndexID;

    smrRTOI      * mNext;
} smrRTOI;

/*FixedTable출력을 위한 값 */
typedef struct smrRTOI4FT
{
    SChar      mType[9];  // INIT,TABLE,INDEX,MEMPAGE,DISKPAGE
    SChar      mCause[9]; // INIT,OBJECT,REDO,UNDO,REFINE,PROPERTY
    ULong      mData1;
    ULong      mData2;
} smrRTOI4FT;

typedef enum
{
    SMR_RECOVERY_NORMAL    = 0, // 기본. 어떠한 검증도 무시하지 않습니다
    SMR_RECOVERY_EMERGENCY = 1, // 잘못된 객체, 잘못된 Redo에 대한 접근을 
                                // 막으면서 서버를 구동시킵니다.
    SMR_RECOVERY_SKIP      = 2  // Recovery를 하지 않습니다. 하지만 DB가 
                                // Inconsistency해집니다.
} smrEmergencyRecoveryPolicy;

/* iSQL, TRC 로그로 남길 메시지 구성을 위한 버퍼 크기 */
#define SMR_MESSAGE_BUFFER_SIZE (512)

#endif /* _O_SMR_DEF_H_ */
