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
 * $Id: sctDef.h 17773 2006-08-28 01:18:54Z raysiasd $
 *
 * Description :
 *
 * TableSpace Manager 자료 구조
 *
 *
 **********************************************************************/

#ifndef _O_SCT_DEF_H_
#define _O_SCT_DEF_H_ 1

#include <smDef.h>
#include <smriDef.h>


// LOGANCHOR에 저장되지 않은 TBS/DBF Attribute는
// 에 대한 값을
# define  SCT_UNSAVED_ATTRIBUTE_OFFSET (0)


#define SCT_INIT_LOCKHIER( aLockHier ) \
{                                      \
    ((sctLockHier*)(aLockHier))->mTBSListSlot = NULL;    \
    ((sctLockHier*)(aLockHier))->mTBSNodeSlot = NULL;    \
}

/*
  Memory/ Disk Tablespace가 공통으로 가지는 정보

[ PROJ-1548 User Memory Tablespace ]

- mStatus : TBS의 현재 상태를 지님
  - Disk/Memory Tablespace모두 같은 방식으로 처리함ㅌ
  - Values
    - Creating :
      - TBS를 CREATE하는 TX가 아직 COMMIT하지 않은 상태
    - Online :
      - TBS를 CREATE하는 TX가 COMMIT한 상태
    - Offline :
    - Dropping :
      - TBS를 DROP하는 TX가 아직 COMMIT하지 않은 상태
    - Dropped :
      - TBS를 DROP하는 TX가 아직 COMMIT한 상태
  - 상태전이
      - 하나의 Tablespace의 상태는 다음과 같이 전이된다.
      (  ==>는 Commit Pending 작업에 의해 전이됨을 도식 )

      Creating ==> Online <-> Offline
                     \         |
                      \        +----> Offline | Dropping ==> Dropped
                       \
                        \-> Online | Dropping ==> Dropped

- SyncMutex
  - 해당 Tablespace안의 Dirty Page를 Disk로 Sync(Flush)할 때
    잡는 Mutex이다.
  - Checkpoint와 Drop Tablespace간의 동시성제어를 담당한다.
    - Checkpoint는 이 Mutex를 잡고 Tablespace의 Status를 검사한다.
    - Drop Tablespace를 수행하는 Tx는 이 Mutex를 잡고
      Tablespace의 Status를 변경한다.
      - 실제 Tablespace관련 Memory해제및 파일삭제는 이 Mutex를
        잡지 않은 채로 수행한다.
        ( Checkpoint가 Mutex잡고 Tablespace의 상태를 검사하기 때문)
    - 목적
      - Checkpoint중인 Tablespace에 대해 Drop Tablespace Tx가
        기다리도록 한다.
      - Drop중인 Tablespace에 대해 Checkpoint가 기다리도록 한다.
*/

typedef struct sctTableSpaceNode
{
    // 테이블스페이스 아이디
    scSpaceID            mID;
    // 테이블스페이스 종류
    // User || System, Disk || Memory, Data || Temp || Undo
    smiTableSpaceType    mType;

    // 테이블스페이스 이름
    SChar               *mName;
    // 테이블스페이스 상태(Creating, Droppping, Online, Offline...)
    UInt                 mState;
    // 테이블스페이스의 Page를 Disk로 Sync하기 전에 잡아야 하는 Mutex
    iduMutex             mSyncMutex;
    // 테이블스페이스를 위한 Commit-Duration lock item
    void                *mLockItem4TBS;

    /* BUG-18279: Drop Table Space시에 생성된 Table을 빠뜨리고
     *            Drop이 수행됩니다.
     *
     * 테이블을 생성한 Transaction의 CommitSCN이 설정된다.
     * 이값은 Server 시작시에 0으로 초기화된다.
     * (이 Tablespace에 DDL을 하는 Transaction의 ViewSCN은
     * 항상 이 SCN보다 작지 않아야 한다. 작으면 Rebuild에러를
     * 올린다. */
    smSCN                mMaxTblDDLCommitSCN;

    iduMutex             mMutex;    // Table Space의 상태변경, FileNode의 상태변경 보호
    iduCond              mBackupCV; // Backup 대기  CV
} sctTableSpaceNode;


// 테이블스페이스리스트, 테이블스페이스노드에
// 대한 잠금슬롯 저장 구조체
typedef struct sctLockHier
{
    void *mTBSListSlot;
    void *mTBSNodeSlot;
} sctLockHier;

// X$TABLESPACES_HEADER에서 사용자에게 보여질 정보
typedef struct sctTbsHeaderInfo
{
    scSpaceID          mSpaceID;
    SChar             *mName;
    smiTableSpaceType  mType;
    // Tablespace 상태정보의 bitset
    UInt               mStateBitset;
    // 상태정보 출력 문자열
    UChar              mStateName[SM_DUMP_VALUE_LENGTH + 1];
    // Tablespace안의 데이터에 대한 Log의 압축 여부
    UInt               mAttrLogCompress;
} sctTbsHeaderInfo;

typedef struct sctTbsInfo
{
    scSpaceID          mSpaceID;
    sdFileID           mNewFileID;
    UChar              mExtMgmtName[20];
    UChar              mSegMgmtName[20];
    UInt               mDataFileCount;
    ULong              mTotalPageCount;
    UInt               mExtPageCount;
    // BUG-15564
    // mUsedPageLimit에서 mAllocPageCount로 이름을 바꾼다.
    // 사용중인 페이지가 아니라 할당된 페이지 수를 나타내기
    // 때문이다.
    ULong              mAllocPageCount;

    /* BUG-18115 v$tablespaces의 page_size가 메모리 경우도 8192로 나옵니다.
     *
     * TableSpace별로 PageSize가 나오도록 수정하였습니다.*/
    UInt               mPageSize;

    /* Free된 Extent중 Free Extent Pool에 존재하는 Extent갯수 */
    ULong              mCachedFreeExtCount;
} sctTbsInfo;

// To implement TASK-1842.
typedef enum sctPendingOpType
{
    SCT_POP_CREATE_TBS = 0,
    SCT_POP_DROP_TBS,
    SCT_POP_ALTER_TBS_ONLINE,
    SCT_POP_ALTER_TBS_OFFLINE,
    SCT_POP_CREATE_DBF,
    SCT_POP_DROP_DBF,
    SCT_POP_ALTER_DBF_RESIZE,
    SCT_POP_ALTER_DBF_ONLINE,
    SCT_POP_ALTER_DBF_OFFLINE,
    SCT_POP_UPDATE_SPACECACHE
} sctPendingOpType;

struct sctPendingOp;

// Disk 및 Memory Tablespace특성에 따라 다르게 처리해야 하는
// 작업을 수행할 Pening Function Pointer
// 참고 : sctTableSpaceMgr::executePendingOperation - 주석
typedef IDE_RC (*sctPendingOpFunc) ( idvSQL            * aStatistics,
                                     sctTableSpaceNode * aTBSNode,
                                     sctPendingOp      * aPendingOp );


/*
   To implement TASK-1842.

   PROJ-1548 User Memory Tablespace ---------------------------------------
   Disk/Memory Tablespace및 Disk DBF에 관련된 Pending작업에 대한
   정보를 지닌다.
   Transaction은 이 구조체를 여러 개 Linked List로 가지고 있다가
   Commit이나 Rollback시에
   sctTableSpaceMgr::executePendingOperation을 호출하여
   Pending된 작업을 수행한다.

   Ex> DROP TABLESPACE TBS1 수행시
       Tablespace에 속한 파일을 삭제하는 Operation은 UNDO가 불가하므로
       Commit시에 Pending으로 처리하여야 한다.
       "Tablespace에 속한 파일을 삭제"하는 Pending Operation을
       sctPendingOp구조체에 설정하여 Transaction에 달아놓고 있다가
       Transaction Commit시에 sctPendingOp구조체를 보고
       Tablespace에 속한 파일을 삭제하는 작업을 수행한다.
*/
typedef struct sctPendingOp
{
    scSpaceID        mSpaceID;
    sdFileID         mFileID;
    idBool           mIsCommit;        /* 동작 시기(commit or abort)를 설정 */
    smiTouchMode     mTouchMode;       /* 파일의 물리적 삭제 여부 결정 */
    SLong            mResizePageSize;  /* ALTER RESIZE 연산시 변경량   */
    ULong            mResizeSizeWanted;/* ALTER RESIZE 연산시
                                        * 변경할 파일 절대크기   */
    UInt             mNewTBSState;     /* pending시 변경할 TBS 상태값  */
    UInt             mNewDBFState;     /* pending시 변경할 DBF 상태값  */
    smLSN            mOnlineTBSLSN;    /* online TBS 로그의 Begin LSN */
    sctPendingOpType mPendingOpType;
    sctPendingOpFunc mPendingOpFunc;
    void*            mPendingOpParam;  /* Pending Parameter */
} sctPendingOp;

// PRJ-1548 User Memory Tablespace
// 테이블스페이스 DDL관련 테이블스페이스 로그의 Update 타입
// SDD 모듈에 정의되어 있었지만, 메모리 테이블스페이스와
// 공유되어야 하는 타입이기 때문에 SCT 모듈에 재정의 한다
typedef enum sctUpdateType
{
    SCT_UPDATE_MRDB_CREATE_TBS = 0,      // DDL
    SCT_UPDATE_MRDB_CREATE_CIMAGE_FILE,  // DDL
    SCT_UPDATE_MRDB_DROP_TBS,            // DDL
    SCT_UPDATE_MRDB_ALTER_AUTOEXTEND,    // DDL
    SCT_UPDATE_MRDB_ALTER_TBS_ONLINE,    // DDL
    SCT_UPDATE_MRDB_ALTER_TBS_OFFLINE,   // DDL
    SCT_UPDATE_DRDB_CREATE_TBS,          // DDL
    SCT_UPDATE_DRDB_DROP_TBS,            // DDL
    SCT_UPDATE_DRDB_ALTER_TBS_ONLINE,    // DDL
    SCT_UPDATE_DRDB_ALTER_TBS_OFFLINE,   // DDL
    SCT_UPDATE_DRDB_CREATE_DBF,          // DDL
    SCT_UPDATE_DRDB_DROP_DBF,            // DDL
    SCT_UPDATE_DRDB_EXTEND_DBF,          // DDL
    SCT_UPDATE_DRDB_SHRINK_DBF,          // DDL
    SCT_UPDATE_DRDB_AUTOEXTEND_DBF,      // DDL
    SCT_UPDATE_DRDB_ALTER_DBF_ONLINE,    // DDL
    SCT_UPDATE_DRDB_ALTER_DBF_OFFLINE,   // DDL
    SCT_UPDATE_VRDB_CREATE_TBS,          // DDL
    SCT_UPDATE_VRDB_DROP_TBS,            // DDL
    SCT_UPDATE_VRDB_ALTER_AUTOEXTEND,    // DDL
    SCT_UPDATE_COMMON_ALTER_ATTR_FLAG,   // DDL
    SCT_UPDATE_MAXMAX_TYPE
} sctUpdateType;

// PRJ-1548 User Memory Tablespace
// BACKUP 수행과정에서 테이블스페이스 doAction 함수에
// 전달할 인자들을 정의한다. [IN] 인자
typedef struct sctActBackupArgs
{
    void          * mTrans;
    SChar         * mBackupDir;
    //PROJ-2133 incremental Backup
    smriBISlot    * mCommonBackupInfo;
    idBool          mIsIncrementalBackup;
} sctActBackupArgs;

// PRJ-1548 User Memory Tablespace
// 서버구동과정에서의 데이타베이스 검증과 체크포인트
// 과정에서의 데이타베이스 검증시 doAction 함수에
// 전달할 인자들을 정의한다. [OUT] 인자
typedef struct sctActIdentifyDBArgs
{
    idBool    mIsValidDBF;    // 미디어 복구 필요여부
    idBool    mIsFileExist;     // 데이타파일이 존재함
} sctActIdentifyDBArgs;

// PRJ-1548 User Memory Tablespace
// 메모리 테이블스페이스 미디어 복구완료과정에서
// 파일헤더 복구할때 전달하는 인자
typedef struct sctActRepairArgs
{
    smLSN*     mResetLogsLSN;
} sctActRepairArgs;

/*
   각각의 TBS에 대해 Action을 수행하는 lockAndRun4EachTBS 에서 사용하는
   Function Type들
*/

// Action Function
typedef IDE_RC (*sctAction4TBS)( idvSQL            * aStatistics,
                                 sctTableSpaceNode * aTBSNode,                                                             void              * aActionArg);

typedef enum sctActionExecMode {
    SCT_ACT_MODE_NONE  = 0x0000,
    // Action수행하는 동안 lock() 과 unlock() 호출
    SCT_ACT_MODE_LATCH = 0x0001
} sctActionExecMode ;


typedef enum sctStateSet {
    SCT_SS_INVALID         = 0x0000, /* 이 값은 절대 사용될 수 없는 값 */
    // Tablespace의 단계별 초기화를 수행해야하는 Tablespace의 상태
    // ( 자세한 내용은 smmTBSMultiState.h를 참고 )
    //
    // ex> DROPPED, DISCARDED, ONLINE, OFFLINE 상태인 Tablespace는
    //     STATE단계까지 Tablespace를 초기화해야함
    SCT_SS_NEED_STATE_PHASE = (SMI_TBS_DROPPED | SMI_TBS_DISCARDED | SMI_TBS_ONLINE | SMI_TBS_OFFLINE ),
    SCT_SS_NEED_MEDIA_PHASE = ( SMI_TBS_DISCARDED | SMI_TBS_OFFLINE | SMI_TBS_ONLINE),
    SCT_SS_NEED_PAGE_PHASE  = ( SMI_TBS_ONLINE ),

    // 각 모듈별 수행을 SKIP할 Tablespace의 상태
    // Tablespace의 생성도중 사망한 경우 SMI_TBS_INCONSISTENT상태가 된다.
    SCT_SS_SKIP_LOAD_FROM_ANCHOR = (SMI_TBS_DROPPED),
    SCT_SS_SKIP_REFINE     = ( SMI_TBS_DROPPED | SMI_TBS_DISCARDED | SMI_TBS_OFFLINE),
    // Drop Pending수행도중 사망한 경우,
    // 어짜피 Drop 될 Tablespace이므로 Redo, Undo Skip함.
    // 단, Tablespace관련 DDL은 Redo,Undo됨.
    SCT_SS_SKIP_MEDIA_REDO = (SMI_TBS_INCONSISTENT | SMI_TBS_DROP_PENDING | SMI_TBS_DROPPED | SMI_TBS_DISCARDED ),
    SCT_SS_SKIP_REDO       = (SMI_TBS_INCONSISTENT | SMI_TBS_DROP_PENDING | SMI_TBS_DROPPED | SMI_TBS_DISCARDED | SMI_TBS_OFFLINE ),
    SCT_SS_SKIP_UNDO       = (SMI_TBS_INCONSISTENT | SMI_TBS_DROP_PENDING | SMI_TBS_DROPPED | SMI_TBS_DISCARDED | SMI_TBS_OFFLINE ),
    // Drop Pending수행도중 사망한 경우,
    // Checkpoint Image일부가 없을 수 있으므로, Prepare,Restore안함
    SCT_SS_SKIP_PREPARE    = (SMI_TBS_INCONSISTENT | SMI_TBS_DROP_PENDING | SMI_TBS_DROPPED | SMI_TBS_DISCARDED | SMI_TBS_OFFLINE ),
    SCT_SS_SKIP_RESTORE    = (SMI_TBS_INCONSISTENT | SMI_TBS_DROP_PENDING | SMI_TBS_DROPPED | SMI_TBS_DISCARDED | SMI_TBS_OFFLINE ),
    // Drop Tablespace Pending을 수행중인 경우에도
    // Checkpoint를 Skip한다.
    SCT_SS_SKIP_CHECKPOINT = (SMI_TBS_DROPPED | SMI_TBS_DROP_PENDING | SMI_TBS_DISCARDED | SMI_TBS_OFFLINE),
    SCT_SS_SKIP_INDEXBUILD = (SMI_TBS_DROPPED | SMI_TBS_DISCARDED | SMI_TBS_OFFLINE),
    SCT_SS_SKIP_DROP_TABLE_CONTENT = ( SMI_TBS_DROPPED | SMI_TBS_DISCARDED | SMI_TBS_OFFLINE),
    /* PAGE단계 destroy시 공유메모리 Page및 Pool을 해제할 Tablespace의 상태
     * BUG-30547 SMI_TBS_DROP_PENDING 상태에서도 공유메모리 제거 */
    SCT_SS_FREE_SHM_PAGE_ON_DESTROY = ( SMI_TBS_DROP_PENDING
                                        | SMI_TBS_DROPPED
                                        | SMI_TBS_OFFLINE ),
    // Drop 가능한 TableSpace 상태
    SCT_SS_HAS_DROP_TABLESPACE = ( SMI_TBS_DISCARDED | SMI_TBS_OFFLINE | SMI_TBS_ONLINE ),
    // Media Recovery 불가능한 TableSpace 상태
    SCT_SS_UNABLE_MEDIA_RECOVERY = ( SMI_TBS_INCONSISTENT |  SMI_TBS_DROPPED | SMI_TBS_DISCARDED | SMI_TBS_DROP_PENDING ),
    // Checkpoint시 DBF 헤더를 갱신하지 않는 경우
    SCT_SS_SKIP_UPDATE_DBFHDR = ( SMI_TBS_DROPPED | SMI_TBS_DROP_PENDING | SMI_TBS_DISCARDED ),
    // Identify Database 과정에서 Check 하지 않은 경우
    SCT_SS_SKIP_IDENTIFY_DB = ( SMI_TBS_INCONSISTENT | SMI_TBS_DROPPED | SMI_TBS_DROP_PENDING | SMI_TBS_DISCARDED ),
    SCT_SS_SKIP_SYNC_DISK_TBS = ( SMI_TBS_DROPPED | SMI_TBS_DISCARDED ),
    SCT_SS_SKIP_SHMUTIL_OPER = ( SMI_TBS_DROPPED | SMI_TBS_DROP_PENDING | SMI_TBS_DISCARDED | SMI_TBS_OFFLINE ),
    SCT_SS_SKIP_COUNTING_TOTAL_PAGES = ( SMI_TBS_DROPPED | SMI_TBS_DROP_PENDING | SMI_TBS_DISCARDED ),
    // 이제는 더이상 Valid 하지 않은 Disk TBS 상태
    SCT_SS_INVALID_DISK_TBS = ( SMI_TBS_DROPPED | SMI_TBS_DISCARDED ),
    // DISCARDED 된 DISK TBS에 대한 Row Aging을 Skip하는 경우
    SCT_SS_SKIP_AGING_DISK_TBS = ( SMI_TBS_DISCARDED ),
    // BUG-48500
    SCT_SS_SKIP_BUILD_FIXED_TABLE = ( SMI_TBS_DROPPED | SMI_TBS_DISCARDED | SMI_TBS_OFFLINE ),

    // fix BUG-8132
    // table/index segment free를 skip 한다.
    SCT_SS_SKIP_FREE_SEG_DISK_TBS = ( SMI_TBS_OFFLINE )
} sctStateSet;


// Tablespace에 Lock을 획득한 후 수행할 Validation
typedef enum sctTBSLockValidOpt
{
    SCT_VAL_INVALID         = 0x0000, /* 이 값은 절대 사용될 수 없는 값 */
    SCT_VAL_CHECK_DROPPED   = 0x0001, /* TBS가 DROPPED이면 에러 */
    SCT_VAL_CHECK_DISCARDED = 0x0002, /* TBS가 DROPPED이면 에러 */
    SCT_VAL_CHECK_OFFLINE   = 0x0004, /* TBS가 DROPPED이면 에러 */

    // Tablespace자체에 DDL을 수행하는 경우 (ex> ALTER TABLESPACE)
    // Tablespace안의 Table/Index에 Insert/Update/Delete/Select하는 경우
    // => DROPPED, DISCARDED, OFFLINE이면 에러발생
    SCT_VAL_DDL_DML   =
        (SCT_VAL_CHECK_DROPPED|SCT_VAL_CHECK_DISCARDED|SCT_VAL_CHECK_OFFLINE),
    // DROP TBS의 경우
    // => DROPPED 이면 에러발생
    // => Tablespace가  DISCARDED상태여도 에러를 발생시키지 않는다.
    SCT_VAL_DROP_TBS =
        (SCT_VAL_CHECK_DROPPED),

    // ALTER TBS ONLINE/OFFLINE의 경우
    // => DROPPED, DISCARDED이면 에러발생
    // => Tablespace가 OFFLINE상태여도 에러를 발생시키지 않는다.
    SCT_VAL_ALTER_TBS_ONOFF =
        (SCT_VAL_CHECK_DROPPED|SCT_VAL_CHECK_DISCARDED)
} sctTBSLockValidOpt;

#endif /* _O_SCT_DEF_H_ */

