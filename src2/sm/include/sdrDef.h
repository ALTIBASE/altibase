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
 * $Id: sdrDef.h 88414 2020-08-25 04:45:02Z justin.kwon $
 *
 * Description :
 *
 * 본 파일은 recovery layer의 disk 정의 헤더파일이다.
 *
 **********************************************************************/
#ifndef _O_SDR_DEF_H_
#define _O_SDR_DEF_H_ 1

#include <sddDef.h>
#include <sdbDef.h>
#include <smuHash.h>
#include <smuDynArray.h>

// BUG-13972
// sdrMtxStackInfo의 size는 128을 넘을 수 없도록 한다.
// 이 값을 외부에서 정하도록 되어 있었지만
// sdrMtxStack 내부로 가져온다.
#define SDR_MAX_MTX_STACK_SIZE   (128)

// PRJ-1867 sdbDef.h에서 옮겨왔다.
// Corrupt Page를 읽었을 때의 대처 방법
typedef enum
{
    SDR_CORRUPTED_PAGE_READ_FATAL = 0,
    SDR_CORRUPTED_PAGE_READ_ABORT,
    SDR_CORRUPTED_PAGE_READ_PASS
} sdrCorruptPageReadPolicy;

/*-------------------------------------------
 * Description : DRDB 로그 타입
 *-------------------------------------------*/
typedef enum
{
    /* page 속성 update 관련 */
    SDR_SDP_1BYTE  = 1,                /* 1byte  속성값의 설정 */
    SDR_SDP_2BYTE  = 2,                /* 2bytes 속성값의 설정 */
    SDR_SDP_4BYTE  = 4,                /* 4bytes 속성값의 설정 */
    SDR_SDP_8BYTE  = 8,                /* 8bytes 속성값의 설정 */
    SDR_SDP_BINARY,                    /* Nbytes 속성값의 설정 */

    /* page 관련 타입 */
    // PROJ-1665 : page의 consistent 정보를 설정하는 log
    // media recovery 시에 nologging으로 Direct-Path INSERT가 수행된
    // Page인 경우, in-consistent 함을 알아내기 위함
    SDR_SDP_PAGE_CONSISTENT = 32,

    SDR_SDP_INIT_PHYSICAL_PAGE,         /* extent desc에서 freepage 할당 또는
                                           예약페이지 할당, value 없음 */
    SDR_SDP_INIT_LOGICAL_HDR,           /* logical header 초기화 */
    SDR_SDP_INIT_SLOT_DIRECTORY,        /* slot directory 초기화 */
    SDR_SDP_FREE_SLOT,                  /* page에서 실제로 하나의 slto을 free */
    SDR_SDP_FREE_SLOT_FOR_SID,
    SDR_SDP_RESTORE_FREESPACE_CREDIT,

    // BUG-17615
    SDR_SDP_RESET_PAGE,                 /* index bottom-up build시 page reset */

    // PRJ-1149
    SDR_SDP_WRITE_PAGEIMG,             /* 백업과정중에 write발생시 실제
                                           데이타파일에 write를 하지않고
                                           로그로 남김 */
    // sdb
    // PROJ-1665 : Direct-Path Buffer에서 Direct-Path INSERT가 수행된
    //             Page를 Flush할때 Page 전체에 대하여 Logging
    SDR_SDP_WRITE_DPATH_INS_PAGE,      /* PROJ-1665 */

    // PROJ-1671 Bitmap-based Tablespace And Segment Space Management
    SDR_SDPST_INIT_SEGHDR,              /* Segment Header 초기화 */
    SDR_SDPST_INIT_BMP,                 /* Root/Internal Bitmap Page 초기화 */
    SDR_SDPST_INIT_LFBMP,               /* Leaf Bitmap Page 초기화 */
    SDR_SDPST_INIT_EXTDIR,              /* Extent Map Page 초기화 */
    SDR_SDPST_ADD_RANGESLOT,            /* leaf bmp page의 rangeslot을 기록 */
    SDR_SDPST_ADD_SLOTS,                /* rt/it-bmp page의 slot들을 기록 */
    SDR_SDPST_ADD_EXTDESC,              /* extent map page에 extslot 기록 */
    SDR_SDPST_ADD_EXT_TO_SEGHDR,        /* segment에 extent 연결 */
    SDR_SDPST_UPDATE_WM,                /* HWM 갱신 */
    SDR_SDPST_UPDATE_MFNL,              /* BMP MFNL 갱신 */
    SDR_SDPST_UPDATE_PBS,               /* PBS 갱신 */
    SDR_SDPST_UPDATE_LFBMP_4DPATH,      /* DirectPath완료시 lfbmp bitset변경 */

    // PROJ-1705 Disk MVCC Renewal
    /* XXX: migration issue 때문에 아래 값은 58이어야 한다. */
    SDR_SDPSC_INIT_SEGHDR = 58,         /* Segment Header 초기화 */
    SDR_SDPSC_INIT_EXTDIR,              /* ExtDir Page 초기화 */
    SDR_SDPSC_ADD_EXTDESC_TO_EXTDIR,    /* ExtDir Page에 ExtDesc 기록 */

    SDR_SDPTB_INIT_LGHDR_PAGE,         /* local group header initialize */
    SDR_SDPTB_ALLOC_IN_LG,             /* alloc by bitmap index */
    SDR_SDPTB_FREE_IN_LG,              /* free by bitmap index */

                                       /* DML - insert, update, delete */
    SDR_SDC_INSERT_ROW_PIECE = 64,     /* page에 row piece를 insert한 후에 기록 */
    SDR_SDC_INSERT_ROW_PIECE_FOR_UPDATE,
    SDR_SDC_INSERT_ROW_PIECE_FOR_DELETEUNDO, /* delete rollback을 수행한 후에 기록 */
    SDR_SDC_UPDATE_ROW_PIECE,
    SDR_SDC_OVERWRITE_ROW_PIECE,
    SDR_SDC_CHANGE_ROW_PIECE_LINK,
    SDR_SDC_DELETE_FIRST_COLUMN_PIECE,
    SDR_SDC_ADD_FIRST_COLUMN_PIECE,
    SDR_SDC_DELETE_ROW_PIECE_FOR_UPDATE,
    SDR_SDC_DELETE_ROW_PIECE,          /* row piece에 delete flag를 설정한 후에 기록 */
    SDR_SDC_LOCK_ROW,
    SDR_SDC_PK_LOG,

    // PROJ-1705 Disk MVCC Renewal
    SDR_SDC_INIT_CTL,                   /* CTL 영역 및 헤더 초기화 */
    SDR_SDC_EXTEND_CTL,                 /* CTL 영역 */
    SDR_SDC_BIND_CTS,                   /* Binding CTS */
    SDR_SDC_UNBIND_CTS,                 /* unbinding CTS */
    SDR_SDC_BIND_ROW,                   /* Binding In-Row CTS */
    SDR_SDC_UNBIND_ROW,                 /* unbinding In-Row CTS */
    SDR_SDC_ROW_TIMESTAMPING,           /* Row TimeStamping */
    SDR_SDC_DATA_SELFAGING,             /* SELF AGING */

    SDR_SDC_BIND_TSS,                  /* bind TSS */
    SDR_SDC_UNBIND_TSS,                /* unbind TSS */
    SDR_SDC_SET_INITSCN_TO_TSS,        /* Set INITSCN to CTS */
    SDR_SDC_INIT_TSS_PAGE,             /* TSS 페이지 초기화 */
    SDR_SDC_INIT_UNDO_PAGE,            /* Undo 페이지 초기화 */
    SDR_SDC_INSERT_UNDO_REC,           /* undo segment에 undo log를
                                          생성한 후에 기록 */

    SDR_SDN_INSERT_INDEX_KEY = 96,     /* internal key에 대한 insert */
    SDR_SDN_FREE_INDEX_KEY,            /* internal key에 대한 delete */
    SDR_SDN_INSERT_UNIQUE_KEY,         /* insert index unique key에 대한
                                        * physical redo and logical undo */
    SDR_SDN_INSERT_DUP_KEY,            /* insert index duplicate key에 대한
                                        * physical redo and logical undo */
    SDR_SDN_DELETE_KEY_WITH_NTA,       /* delete index key에 대한
                                        * physical redo and logical undo */
    SDR_SDN_FREE_KEYS,                 /* BUG-19637 SMO에 의해 split될때
                                           index key가 여러 개 free */
    SDR_SDN_COMPACT_INDEX_PAGE,        /* index page에 대한 compact */
    SDR_SDN_KEY_STAMPING,
    SDR_SDN_INIT_CTL,                  /* Index CTL 영역 및 헤더 초기화 */
    SDR_SDN_EXTEND_CTL,                /* Index CTL 확장 */
    SDR_SDN_FREE_CTS,

    /*
     * PROJ-2047 Strengthening LOB
     */
    SDR_SDC_LOB_UPDATE_LOBDESC,
    
    SDR_SDC_LOB_INSERT_INTERNAL_KEY,
    
    SDR_SDC_LOB_INSERT_LEAF_KEY,
    SDR_SDC_LOB_UPDATE_LEAF_KEY,
    SDR_SDC_LOB_OVERWRITE_LEAF_KEY,

    SDR_SDC_LOB_FREE_INTERNAL_KEY,
    SDR_SDC_LOB_FREE_LEAF_KEY,
    
    SDR_SDC_LOB_WRITE_PIECE,           /* PROJ-1362 lob piece write,
                                        * replication때문에 추가
                                        * SM입장에서는 SDR_SDP_BINARY를
                                        * 사용해도 무관. */
    SDR_SDC_LOB_WRITE_PIECE4DML,
    
    SDR_SDC_LOB_WRITE_PIECE_PREV,      /* replication때문에 추가,
                                        * replication은 이 로그를 무시한다. */

    SDR_SDC_LOB_ADD_PAGE_TO_AGINGLIST,

    //external recovery function

    /*
     * PROJ-1591 Disk Spatial Index
     */
    SDR_STNDR_INSERT_INDEX_KEY,
    SDR_STNDR_UPDATE_INDEX_KEY,
    SDR_STNDR_FREE_INDEX_KEY,
    
    SDR_STNDR_INSERT_KEY,
    SDR_STNDR_DELETE_KEY_WITH_NTA,
    SDR_STNDR_FREE_KEYS,

    SDR_STNDR_COMPACT_INDEX_PAGE,
    SDR_STNDR_KEY_STAMPING,
    
    SDR_LOG_TYPE_MAX
} sdrLogType;

typedef enum sdrOPType
{
    SDR_OP_INVALID = 0,
    SDR_OP_NULL,
    SDR_OP_SDP_CREATE_TABLE_SEGMENT,
    SDR_OP_SDP_CREATE_LOB_SEGMENT,
    SDR_OP_SDP_CREATE_INDEX_SEGMENT,

    /* LOB Segment */
    SDR_OP_SDC_LOB_ADD_PAGE_TO_AGINGLIST,
    SDR_OP_SDC_LOB_APPEND_LEAFNODE,

    /* Undo Segment */
    SDR_OP_SDC_ALLOC_UNDO_PAGE,

    /* Treelist Segment : PROJ-1671 */
    SDR_OP_SDPTB_ALLOCATE_AN_EXTENT_FROM_TBS,

    /* Extent Dir.Free List for CMS */
    SDR_OP_SDPTB_ALLOCATE_AN_EXTDIR_FROM_LIST,

    SDR_OP_SDPTB_RESIZE_GG,
    SDR_OP_SDPST_ALLOC_PAGE,
    SDR_OP_SDPST_UPDATE_WMINFO_4DPATH,
    SDR_OP_SDPST_UPDATE_MFNL_4DPATH,
    SDR_OP_SDPST_UPDATE_BMP_4DPATH,

    /* Free List TableSpace: PROJ-1671 */
    SDR_OP_SDPSF_ALLOC_PAGE,
    SDR_OP_SDPSF_ADD_PIDLIST_PVTFREEPIDLIST_4DPATH,
    SDR_OP_SDPSF_MERGE_SEG_4DPATH,
    SDR_OP_SDPSF_UPDATE_HWMINFO_4DPATH,

    /* MVCC Renewal: PROJ-1704 */
    SDR_OP_SDN_INSERT_KEY_WITH_NTA,
    SDR_OP_SDN_DELETE_KEY_WITH_NTA,

    /* Direct Path Insert Rollback: PROJ-2068 */
    SDR_OP_SDP_DPATH_ADD_SEGINFO,

    /* Disk Spatial Index: PROJ-1591 */
    SDR_OP_STNDR_INSERT_KEY_WITH_NTA,
    SDR_OP_STNDR_DELETE_KEY_WITH_NTA,

    SDR_OP_MAX
} sdrOPType;

/* ------------------------------------------------
 * Description : sdrHashNode 처리 상태 정의
 *
 * hash 노드 내의 hashed log가 모두 페이지에
 * 반영 여부를 나타내는 상태값을 정의한다.
 * ----------------------------------------------*/
typedef enum
{
    SDR_RECV_NOT_START = 0,   /* 적용되지않은 노드 상태 */
    SDR_RECV_START,           /* 적용중인 노드 상태 */
    SDR_RECV_FINISHED,        /* 적용완료된 노드 상태 */
    SDR_RECV_INVALID          /* 유효하지 않은 hash 노드 */
} sdrHashNodeState;

typedef struct sdrInitPageInfo
{
    // Page ID
    scPageID     mPageID;
    // Parent Page ID
    scPageID     mParentPID;
    // TableOID   BUG-32091
    smOID        mTableOID;
    // IndexID  PROJ-2281
    UInt         mIndexID;
    // Page Type
    UShort       mPageType;
    // Page State
    UShort       mPageState;
    // Parent에서 현재 페이지가 몇번째 페이지인가
    SShort       mIdxInParent;
    UShort       mAlign;
} sdrInitPageInfo;

/* ------------------------------------------------
 * Description : media recovery 대상 데이타파일들의 자료구조
 * ----------------------------------------------*/
typedef struct sdrRecvFileHashNode
{
    sdrHashNodeState   mState;       /* 포함된 redo 로그들이 반영된 상태 */

    scSpaceID          mSpaceID;     /* tablespace id */
    sdFileID           mFileID;      /* file id */
    scPageID           mFstPageID;   /* 데이타파일의 시작 PAGE ID*/
    scPageID           mLstPageID;   /* 데이타파일의 마지막 PAGE ID*/
    smLSN              mFromLSN;     /* 적용할 최초 redo 로그의 LSN */
    smLSN              mToLSN;       /* 적용할 마지막 redo 로그의 LSN */
    time_t             mToTime;      /* 적용할 마지막 redo 로그의
                                        commit 로그의 LSN (time) */

    UInt               mApplyRedoLogCnt; /* 적용할 로그 개수 */
    sddDataFileNode*   mFileNode;        /* 복구 완료직후 파일헤더와
                                            로그앵커 sync를 위해 */
    SChar*             mSpaceName;    /* tablespace 명*/
} sdrRecvFileHashNode;

/* ------------------------------------------------
 * Description : hash 버켓이 관리하는 hash 노드 구조체
 * ----------------------------------------------*/
typedef struct sdrRedoHashNode
{
    sdrHashNodeState     mState;       /* 포함된 redo 로그들이 반영된 상태를
                                      나타내는 플래그 */
    scSpaceID            mSpaceID;     /* tablespace id */
    scPageID             mPageID;      /* page id */
    UInt                 mRedoLogCount; /* 포함하는 redo 로그 노드 count */
    smuList              mRedoLogList; /* 포함하는 redo 로그 list의 base node */

    sdrRecvFileHashNode* mRecvItem;    /* media recovery시 사용되는 2차 hash 노드
                                          포인터 */
} sdrRedoHashNode;

/* ------------------------------------------------
 * PROJ-1665 : corrupted pages hash node 구조체
 * ----------------------------------------------*/
typedef struct sdrCorruptPageHashNode
{
    scSpaceID            mSpaceID;     /* tablespace id */
    scPageID             mPageID;      /* page id */

} sdrCorruptPageHashNode;

/* ------------------------------------------------
 * Description : 실제 redo log가 저장된 구조체
 * ----------------------------------------------*/
typedef struct sdrRedoLogData
{
    sdrLogType    mType;         /* 로그 타입 */
    scSpaceID     mSpaceID;      /* SpaceID  - for PROJ-2162 */
    scOffset      mOffset;       /* offset */
    scSlotNum     mSlotNum;      /* slot num */
    scPageID      mPageID;       /* PID      - for PROJ-2162 */
    void        * mSourceLog;    /* 원본로그 - for PROJ-2162 */
    SChar       * mValue;        /* value */
    UInt          mValueLength;  /* value 길이 */
    smTID         mTransID;      /* tx id */
    smLSN         mBeginLSN;     /* 로그의 start lsn */
    smLSN         mEndLSN;       /* 로그의 end lsn */
    smuList       mNode4RedoLog; /* redo 로그의 연결 리스트 */
} sdrRedoLogData;


/* --------------------------------------------------------------------
 * 여기부터 mini transaction 관련
 * ----------------------------------------------------------------- */
/* --------------------------------------------------------------------
 * mtx의 로깅 모드
 * 일반적으로는 로깅 모드이나
 * 노 로깅 모드로 진행하는 오퍼레이션이 존재한다.
 * ----------------------------------------------------------------- */

typedef enum
{
    // read 용으로만 혹은 log 필요없이 작업한다.
    SDR_MTX_NOLOGGING = 0,

    // redo 로그를 write한다. 일반적으로 사용된다.
    SDR_MTX_LOGGING
} sdrMtxLogMode;


/* --------------------------------------------------------------------
 * 로깅을 redo만 할 것이냐 redo에 대한 before이미지를 기록할 것이냐.
 * before image는 나중에 이 연산을 undo할 때 사용된다.
 * 이는 mtx begin - commit 사이에 abort가 발생했을 때,
 * abort를 처리하기 위하여 이전의 페이지 연산을 되돌리기 위해 사용된다.
 * mtx logging 모드일때 다음의 2개의 type으로 구분된다.
 * ----------------------------------------------------------------- */
typedef enum
{
    // redo 로그를 write한다. 일반적으로 사용된다.
    SDR_MTX_LOGGING_REDO_ONLY = 0 ,

    // n byte redo로그에 대한 undo log를 write한다.
    SDR_MTX_LOGGING_WITH_UNDO

} sdrMtxLoggingType;


/* --------------------------------------------------------------------
 * mtx에서 사용하는 latch의 종류.
 * commit시에 이 종류따라 release를 해주게 된다.
 * ----------------------------------------------------------------- */
typedef enum
{
    SDR_MTX_PAGE_X = SDB_X_LATCH,  // page X-latch

    SDR_MTX_PAGE_S = SDB_S_LATCH,  // page S-latch

    SDR_MTX_PAGE_NO = SDB_NO_LATCH,// no latch

    SDR_MTX_LATCH_X,               // latch만 X로 잡는다.

    SDR_MTX_LATCH_S,               // latch만 S로 잡는다.

    SDR_MTX_MUTEX_X                // mutex를 잡는다.

} sdrMtxLatchMode;

/* --------------------------------------------------------------------
 * mtx에서 사용하는 latch의 종류에 따른 relase function개수
 * sdrMtxLatchMode의 개수와 같다.
 * ----------------------------------------------------------------- */
#define SDR_MTX_RELEASE_FUNC_NUM  (SDR_MTX_MUTEX_X+1)


/* ----------------------------------------------------------------------------
 *   PROJ-1867
 *   CorruptPageErrPolicy - Corrupt Page를 발견하였을 때 대처방식
 *   SDR_CORRUPT_PAGE_ERR_POLICY_SERVERFATAL - CorruptPage 발견시 서버 종료
 *   SDR_CORRUPT_PAGE_ERR_POLICY_OVERWRITE
 *                              - CorruptPage 발견시 ImgLog로 Overwrite 시도
 *   CORRUPT_PAGE_ERR_POLICY  0(00)  1(01)  2(10)  3(11)
 *   SERVERFATAL  (01)          X      O      X      O
 *   OVERWRITE    (10)          X      X      O      O
 * --------------------------------------------------------------------------*/
#define SDR_CORRUPT_PAGE_ERR_POLICY_SERVERFATAL  (0x00000001)
#define SDR_CORRUPT_PAGE_ERR_POLICY_OVERWRITE    (0x00000002)


/* --------------------------------------------------------------------
 *
 * latch item의 타입을 나타낸다.
 * mtx stack에 특정 아이템이 있나 확인하고자 할때
 * 사용된다.
 *
 * ----------------------------------------------------------------- */
typedef enum
{
    SDR_MTX_BCB = 0,
    SDR_MTX_LATCH,
    SDR_MTX_MUTEX
} sdrMtxItemSourceType;

/* ---------------------------------------------
 * latch release를 위한 function prototype
 * --------------------------------------------- */
typedef IDE_RC (*smrMtxReleaseFunc)( void *aObject,
                                     UInt  aLatchMode,
                                     void *aMtx );

/* ---------------------------------------------
 * stack에 적재된 Object의 비교를 위한 함수
 * --------------------------------------------- */
typedef idBool (*smrMtxCompareFunc)( void *aLhs,
                                     void *aRhs );

/* ---------------------------------------------
 * stack에 적재된 Object를 dump 하기 위한 함수
 * --------------------------------------------- */
typedef IDE_RC (*smrMtxDumpFunc)( void *aObject );


/* --------------------------------------------------------------------
 * enum type의 latch mode이름의 최대 길이
 * dump를 위한 것이므로 큰 의미는 없다.
 * ----------------------------------------------------------------- */
#define LATCH_MODE_NAME_MAX_LEN (50)

/* --------------------------------------------------------------------
 * mtx stack의 각 latch item에 적용되는 함수들의 집합과
 * latch 대상에 대한 정보
 * ----------------------------------------------------------------- */
typedef struct sdrMtxLatchItemApplyInfo
{
    smrMtxReleaseFunc mReleaseFunc;
    smrMtxReleaseFunc mReleaseFunc4Rollback;
    smrMtxCompareFunc mCompareFunc;
    smrMtxDumpFunc    mDumpFunc;
    sdrMtxItemSourceType mSourceType;

    // dump를 위한 latch mode의 문자열
    SChar             mLatchModeName[LATCH_MODE_NAME_MAX_LEN];

} sdrMtxLatchItemApplyInfo;

/* --------------------------------------------------------------------
 * Description mtx 스택에 저장되는 자료구조
 *
 * latch 스택에 push하거나 pop하는 단위자료이다.
 * ----------------------------------------------------------------- */
typedef struct sdrMtxLatchItem
{
    void*             mObject;
    sdrMtxLatchMode   mLatchMode;
    smuAlignBuf       mPageCopy;  /*PROJ-2162 RedoValidation을 위해 존재*/
} sdrMtxLatchItem;

/* --------------------------------------------------------------------
 * Description : DRDB 로그의 Header
 *
 * 로그의 타입이 SDR_1/2/4/8_BYTES인 경우 body에 length를
 * 기록하지 않으며, 그 외의 경우는 length를 사용한다.
 * ----------------------------------------------------------------- */
typedef struct sdrLogHdr
{
    scGRID         mGRID;
    sdrLogType     mType;
    UInt           mLength;
} sdrLogHdr;

/* --------------------------------------------------------------------
 * 2개의 stack item을 받아 비교하는 function이다.
 * ----------------------------------------------------------------- */
typedef idBool (*sdrMtxStackItemCompFunc)(void *, void *);

/* --------------------------------------------------------------------
 * stack의 정보를 갖는다. 리스트를 이용하여 구현됨
 * ----------------------------------------------------------------- */
typedef struct sdrMtxStackInfo
{
    UInt          mTotalSize;      // 스택의 총 크기

    UInt          mCurrSize;      // 현재 스택의 크기

    sdrMtxLatchItem mArray[SDR_MAX_MTX_STACK_SIZE];

} sdrMtxStackInfo;

/* -------------------------------------------------------------------------
 * BUG-24730 [SD] Drop된 Temp Segment의 Extent는 빠르게 재사용되어야 합니다. 
 * 
 * Mini-transaction Commit 이후에 재활용 되도록 하기 위해, Mini-transaction
 * 에서 Free Extent 목록을 갖고 있습니다. FreeExtent는 Mini-Transaction의
 * Commit 과정 중 마지막에 일어나야 하는 Pending 연산 입니다.
 * ---------------------------------------------------------------------- */
typedef IDE_RC (*sdrMtxPendingFunc)( void * aData );

/* --------------------------------------------------------------------------
 *  PROJ-2162 RestartRiskReduction
 *  DISK MiniTransaction 관련 Flag등을 정의함.
 *  추가적으로 DiskLogging 관련 Flag등은 smDef.h에 SM_DLOG Prefix로 정리됨
 * ------------------------------------------------------------------------*/

/*  For DISK NOLOGGING Attributes */
# define SDR_MTX_NOLOGGING_ATTR_PERSISTENT_MASK   (0x00000001)
# define SDR_MTX_NOLOGGING_ATTR_PERSISTENT_OFF    (0x00000000)
# define SDR_MTX_NOLOGGING_ATTR_PERSISTENT_ON     (0x00000001)

/* TASK-2398 Log Compress
 *   Disk Log가 Log File에 기록될 때 압축 여부를 결정하는 Flag
 *   => Mini Transaction이 기록하는 Log중
 *      LOG압축을 하지 않겠다고 설정된 Tablespace에 대한 로그가
 *      하나라도 있으면, 로그를 압축하지 않는다. */
# define SDR_MTX_LOG_SHOULD_COMPRESS_MASK         (0x00000002)
# define SDR_MTX_LOG_SHOULD_COMPRESS_ON           (0x00000000)
# define SDR_MTX_LOG_SHOULD_COMPRESS_OFF          (0x00000002)

/* mtx가 init되었으면 true
 * destroy되면 false
 * rollback시에 true일때만 rollback을 수행하도록 하기 위해 추가
 * mtx begin하는 함수에서는 State처리 없이 exception에서
 * mtx rollback만 해주면 된다. */
# define SDR_MTX_INITIALIZED_MASK                 (0x00000004)
# define SDR_MTX_INITIALIZED_ON                   (0x00000000)
# define SDR_MTX_INITIALIZED_OFF                  (0x00000004)

/* Logging 여부 Flag
 * NoLogging - read 용으로만 혹은 log 필요없이 작업한다.
 * Logging   - redo 로그를 write한다. 일반적으로 사용된다.
 * ( 과거 sdrMtxLogMde였음 ) */
# define SDR_MTX_LOGGING_MASK                     (0x00000008)
# define SDR_MTX_LOGGING_ON                       (0x00000000)
# define SDR_MTX_LOGGING_OFF                      (0x00000008)

/* Refrence Offset 설정 여부 ( RefOffset )
 *
 * - DML관련 undo/redo 로그 위치 기록
 * ref offset은 하나의 smrDiskLog에 기록된 여러개의 disk 로그들 중
 * replication sender가 참고하여 로그를 판독하거나 transaction undo시에
 * 판독하는 DML관련 redo/undo 로그가 기록된 위치를 의미한다. */
# define SDR_MTX_REFOFFSET_MASK                   (0x00000010)
# define SDR_MTX_REFOFFSET_OFF                    (0x00000000)
# define SDR_MTX_REFOFFSET_ON                     (0x00000010)

/* MtxUndo가능 여부
 * 
 *  PROJ-2162 RestartRiskReduction
 *  Mtx는 기본적으로 DirtyPage(XLatch잡으면 자동으로 DirtyPage로 인식)에
 * 대해 복구하는 기능은 있지만, 등록되지 않은 Runtime객체의 복구등은 알지
 * 못하기 때문에 복구 불가능하다. 따라서 이러한 경우에 문제 없는지를 검증
 * 해야 한다.
 *  테스르를 통해 검증된 경우만 MtxRollback 가능 여부를 설정한다.
 *
 * 이에 대한 테스트 방법은
 * TC/Server/sm4/Project3/PROJ-2162/function/mtxtest
 * 를 참고한다. */
# define SDR_MTX_UNDOABLE_MASK                    (0x00000020)
# define SDR_MTX_UNDOABLE_OFF                     (0x00000000)
# define SDR_MTX_UNDOABLE_ON                      (0x00000020)

/* MtxUndo무시 여부
 * 
 *  PROJ-2162 RestartRiskReduction
 *  Mtx는 Dirty된 페이지를 반드시 복구한다. 이 연산은 오래 걸린다. 하지만
 * UniqueViolation, 공간부족, Update Retry 연산 등 Undo할 필요 없이 
 * 그냥 Log등을 무시해도 되는 경우가 있다.
 *  이런 경우 Mtx사용 측에서 해당 사항을 설정함으로써 쓸대없는 Rollback을
 * 막는다. */
# define SDR_MTX_IGNORE_UNDO_MASK                 (0x00000040)
# define SDR_MTX_IGNORE_UNDO_OFF                  (0x00000000)
# define SDR_MTX_IGNORE_UNDO_ON                   (0x00000040)

/* StartupBugDetector 기능 수행 여부
 *
 *  PROJ-2162 RestartRiskReduction
 * __SM_ENABLE_STARTUP_BUG_DETECTOR 의 값을 저장해둔다. */
# define SDR_MTX_STARTUP_BUG_DETECTOR_MASK        (0x00000080)
# define SDR_MTX_STARTUP_BUG_DETECTOR_OFF         (0x00000000)
# define SDR_MTX_STARTUP_BUG_DETECTOR_ON          (0x00000080)


# define SDR_MTX_DEFAULT_INIT ( SDR_MTX_NOLOGGING_ATTR_PERSISTENT_OFF | \
                                SDR_MTX_LOG_SHOULD_COMPRESS_ON |        \
                                SDR_MTX_INITIALIZED_ON |                \
                                SDR_MTX_LOGGING_ON |                    \
                                SDR_MTX_REFOFFSET_OFF |                 \
                                SDR_MTX_UNDOABLE_OFF |                  \
                                SDR_MTX_IGNORE_UNDO_OFF |               \
                                SDR_MTX_STARTUP_BUG_DETECTOR_OFF )

# define SDR_MTX_DEFAULT_DESTROY ( SDR_MTX_NOLOGGING_ATTR_PERSISTENT_OFF | \
                                   SDR_MTX_LOG_SHOULD_COMPRESS_ON |        \
                                   SDR_MTX_INITIALIZED_OFF |               \
                                   SDR_MTX_LOGGING_ON |                    \
                                   SDR_MTX_REFOFFSET_OFF |                 \
                                   SDR_MTX_UNDOABLE_OFF |                  \
                                   SDR_MTX_IGNORE_UNDO_OFF |               \
                                   SDR_MTX_STARTUP_BUG_DETECTOR_OFF )

#define SDR_MTX_STARTUP_BUG_DETECTOR_IS_ON(f)                              \
                           ( ( ( (f) & SDR_MTX_STARTUP_BUG_DETECTOR_MASK)  \
                                == SDR_MTX_STARTUP_BUG_DETECTOR_ON ) ?     \
                             ID_TRUE : ID_FALSE ) 
 

typedef struct sdrMtxPendingJob
{
    idBool              mIsCommitJob; /* Commit시 수행(T) Rollback시 수행(F)*/
    sdrMtxPendingFunc   mPendingFunc; /* Job을 수행할 함수 */
    void              * mData;        /* Job을 수행하는데 필요한 데이터.*/
} sdrMtxPendingJob;



/* --------------------------------------------------------------------
 * Description : mini-transaction 자료구조
 *
 * mtx의 정보를 유지한다.
 * ----------------------------------------------------------------- */
typedef struct sdrMtx
{
    smuDynArrayBase     mLogBuffer;  /* mtx 로그 버퍼 */
    idvSQL*             mStatSQL;    /* 통계정보 */
    void*               mTrans;      /* mtx를 초기화한 트랜잭션 */

    UInt                mLogCnt;
    scSpaceID           mAccSpaceID; /* mtx에 의해 변경되는 TBS는 하나이다. */
                                     /* 단, undo tbs는 data tbs에
                                        의해 함께 변경될 수 있다 */

    smLSN               mBeginLSN;    /* Begin LSN */
    smLSN               mEndLSN;      /* End LSN   */
    // disk log의 속성 mtx begin시에 정의된다.
    UInt                mDLogAttr;

    UInt                mFlag;
    UInt                mOpType;      /* operation 로깅의 연산 타입 */
    smLSN               mPPrevLSN;    /* NTA 구간 설정에 필요한 LSN
                                         이전의 이전 LSN */
    UInt                mRefOffset;   /* 로그버퍼상의 DML관련
                                         redo/undo 로그 위치 */

    /* -----------------------------------------------------------------------
     * BUG-24730 [SD] Drop된 Temp Segment의 Extent는 빠르게 재사용되어야 
     * 합니다. 
     * 
     * Mini-transaction Commit 이후에 재활용 되도록 하기 위해, MiniTransaction
     * 에서 Free Extent 목록을 갖고 있습니다. FreeExtent는 MiniTransaction의
     * Commit 과정 중 마지막에 일어나야 하는 Pending 연산 입니다.
     * -------------------------------------------------------------------- */
    smuList             mPendingJob;

    UInt                mDataCount;

    // replication을 위한 table oid
    smOID               mTableOID;

    /* PROJ-2162 RestartRiskReduction
     * DRDB Log는 N개의 SubLog로 구성되며 하나의 SubLog는 initLogRec에 의해
     * sdrLogHdr의 형태로 Header가 기록된다. 따라서 initLogRec으로 한
     * SubLog의 크기를 선언했으면, 그 SubLog를 전부 써야만 Mtx가 정상
     * Commit될 수 있다. */
    /* BUG-32579 The MiniTransaction commit should not be used in
     * exception handling area. */
    UInt                mRemainLogRecSize; 

    //-----------------------------------------------------
    // PROJ-1566
    // < Extent 할당받을때 >
    // - mData1 : SpaceID
    // - mData2 : segment RID
    // - mData3 : extent RID
    // < Page list를 meta의 Page list에 추가할 때 >
    // - mData1 : table OID
    // - mData2 : tail page PID
    // - mData3 : 생성된 page들 중, table type인 page 개수
    // - mData4 : 생성된 모든 page 개수 ( multiple, external 포함 )
    //-----------------------------------------------------
    ULong               mData[ SM_DISK_NTALOG_DATA_COUNT ];

    sdrMtxStackInfo     mLatchStack;  /* mtx에서 사용되는 stack */
    sdrMtxStackInfo     mXLatchPageStack; /* XLatch를 잡은 Page들의 Stack */
} sdrMtx;


/* --------------------------------------------------------------------
 * mtx가 begin하는데 필요한 trans, logmode를 전달하기 위한 구조체
 *
 * ----------------------------------------------------------------- */
typedef struct sdrMtxStartInfo
{
    void          *mTrans;
    sdrMtxLogMode  mLogMode;
} sdrMtxStartInfo;


/* --------------------------------------------------------------------
 * stack의 각 item의 내용을 dump할 것인가.
 * ----------------------------------------------------------------- */
typedef enum
{
    // stack만 dump한다.
    SDR_MTX_DUMP_NORMAL,
    // stack의 아이템을 모두 dump한다.
    SDR_MTX_DUMP_DETAILED
} sdrMtxDumpMode ;

/* --------------------------------------------------------------------
 * mtx의 save point
 * 이 시점까지 latch 잡은 모든 object를 해제할 수 있다.
 * save point 이후로 log가 write 되었다면 save point는 무효화된다.
 * ----------------------------------------------------------------- */
typedef struct sdrSavePoint
{
    // stack의 위치
    UInt   mStackIndex;

    // XLatch Stack의 위치
    UInt   mXLatchStackIndex;

    // mtx가 write한 log의 크기
    UInt   mLogSize;
} sdrSavePoint;

/* --------------------------------------------------------------------
 * Redo 시에 필요한 정보
 * ----------------------------------------------------------------- */
typedef struct sdrRedoInfo
{
    sdrLogType     mLogType; // 로그타입
    scSlotNum      mSlotNum; // Table페이지의 Slot Num
} sdrRedoInfo;

typedef IDE_RC (*sdrDiskRedoFunction)( SChar       * aData,
                                       UInt          aLength,
                                       UChar       * aPagePtr,
                                       sdrRedoInfo * aRedoInfo,
                                       sdrMtx      * aMtx );


typedef IDE_RC (*sdrDiskUndoFunction)(idvSQL      * aStatistics,
                                      smTID         aTransID,
                                      smOID         aOID,
                                      scGRID        aRecGRID,
                                      SChar       * aLogPtr,
                                      smLSN       * aPrevLSN );

typedef IDE_RC (*sdrDiskRefNTAUndoFunction)(idvSQL   * aStatistics,
                                            void     * aTrans,
                                            sdrMtx   * aMtx,
                                            scGRID     aGRID,
                                            SChar    * aLogPtr,
                                            UInt       aSize );

#endif // _O_SDR_DEF_H_
