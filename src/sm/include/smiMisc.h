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
 * $Id: smiMisc.h 91512 2021-08-21 07:50:50Z emlee $
 **********************************************************************/

#ifndef _O_SMI_MISC_H_
# define _O_SMI_MISC_H_ 1

# include <idTypes.h>
# include <smiDef.h>
# include <smDef.h>
# include <smiLogRec.h>

#define SMI_MISC_TABLE_HEADER( aTable )                         \
        ((const smcTableHeader*)((const smpSlotHeader*)aTable+1))

/* PROJ-2433
 * direct key 최소길이 */
#define SMI_MEM_INDEX_MIN_KEY_SIZE ( 8 )

/* FOR A4 : 원래는 smiMakeDatabaseFileName( )이었던 것을
   Memory DB와 Disk DB를 구별하기 위하여 아래와 같이 변경함
   Disk Table Space 관련 함수들은 smiTableSpace.cpp에서 관리 */

/*---------------------------------------------*/
/* 데이터 베이스 기본 정보                         */
/*---------------------------------------------*/

/* FOR A4 : Memory DB와 Disk DB를 구별하기 위하여 aTSType 인자 추가 */
UInt             smiGetPageSize( smiTableSpaceType aTSType );

/* Disk DB의 현재 총 사이즈를 구한다. */
ULong           smiGetDiskDBFullSize();

/* FOR A4 : sdbBufferMgr에서 사용하는 buffer pool의 페이지 개수 */
UInt             smiGetBufferPoolSize( );

// To Fix BUG-15361
// Database Name을 획득한다.
SChar *          smiGetDBName( );

// PROJ-1579 NCHAR
// 데이터베이스 캐릭터 셋을 얻는다.
SChar *          smiGetDBCharSet( );

// PROJ-1579 NCHAR
// 내셔널 캐릭터 셋을 얻는다.
SChar *          smiGetNationalCharSet( );

/*---------------------------------------------*/
/* 테이블 기본 함수                               */
/*---------------------------------------------*/

const void*      smiGetCatalogTable( void );

/* FOR A4 : Memory Table와 Disk Table을 구별하기 위하여
   aTableType 인자 추가 */
UInt             smiGetVariableColumnSize( UInt aTableType );
UInt             smiGetVariableColumnSize4DiskIndex();

UInt             smiGetVCDescInModeSize();

/* FOR A4 : Memory Table와 Disk Table을 구별하기 위하여
   aTableType 인자 추가 */
UInt             smiGetRowHeaderSize( UInt aTableType );

/* FOR A4 : Memory Table중에서도 TableHandle에 대해서만 사용해야 함 */
smSCN            smiGetRowSCN( const void* aRow );



/*---------------------------------------------*/
/* 인덱스 Type 선택 함수                          */
/*---------------------------------------------*/

IDE_RC           smiFindIndexType( SChar* aName,
                                   UInt*  aType );

// BUG-17449
idBool           smiCanMakeIndex( UInt    aTableType,
                                  UInt    aIndexType );

const SChar*     smiGetIndexName( UInt aType );

idBool           smiGetIndexUnique( const void * aIndex );
    
/* FOR A4 : Memory Table와 Disk Table을 구별하기 위하여
   aTableType 인자 추가 */
UInt             smiGetDefaultIndexType( void );

// 인덱스 타입이 Unique가 가능한지의 여부
idBool           smiCanUseUniqueIndex( UInt aIndexType );

// PROJ-1502 PARTITIONED DISK TABLE
// 대소비교 가능한 인덱스 타입인지 체크
idBool           smiGreaterLessValidIndexType( UInt aIndexType );

// PROJ-1704 MVCC Renewal
idBool           smiIsAgableIndex( const void * aIndex );

// To Fix BUG-16218
// 인덱스 타입이 Composite Index가 가능한지의 여부
idBool           smiCanUseCompositeIndex( UInt aIndexType );

/*---------------------------------------------*/
/* 테이블 정보 추출 함수                          */
/*---------------------------------------------*/

const void *     smiGetTable( smOID aTableId );

smOID            smiGetTableId( const void* aTable );

UInt             smiGetTableColumnCount( const void* aTable );

UInt             smiGetTableColumnSize( const void* aTable );

const void *     smiGetTableIndexes( const void * aTable,
                                     const UInt   aIdx );

const void *     smiGetTableIndexByID( const void * aTable,
                                       const UInt   aIndexId );

const void *     smiGetTablePrimaryKeyIndex( const void * aTable );

UInt             smiGetTableIndexCount( const void* aTable );

UInt             smiGetTableIndexSize( void );

const void *     smiGetTableInfo( const void * aTable );

UInt             smiGetTableInfoSize( const void* aTable );

IDE_RC           smiGetTableTempInfo( const void    * aTable,
                                      void         ** aRuntimeInfo );

void             smiSetTableTempInfo( const void * aTable,
                                      void       * aTempInfo );

/* BUG-42928 No Partition Lock */
void             smiSetTableReplacementLock( const void * aDstTable,
                                             const void * aSrcTable );
/* BUG-42928 No Partition Lock */
void             smiInitTableReplacementLock( const void * aTable );

/* For A4 : modified for Disk Table */
IDE_RC           smiGetTableNullRow( const void   * aTable,
                                     void        ** aRow,
                                     scGRID       * aGRID );



UInt             smiGetTableFlag(const void* aTable);

// PROJ-1665
UInt             smiGetTableLoggingMode(const void * aTable);
UInt             smiGetTableParallelDegree(const void * aTable);
UInt             smiGetTableMaxRow(const void * aTable);

// FOR A4
idBool           smiIsDiskTable(const void* aTable);
idBool           smiIsMemTable(const void* aTable);
idBool           smiIsUserMemTable( const void* aTable );
idBool           smiIsVolTable(const void* aTable);

IDE_RC           smiGetTableBlockCount( const void * aTable,
                                        ULong      * aBlockCnt );

IDE_RC           smiGetTableSpaceID(const void * aTable );

/* BUG-28752 lock table ... in row share mode 구문이 먹히지 않습니다.
 * implicit/explicit lock을 구분하여 겁니다. 
 * explicit Lock을 거는 곳은 QP의 qmx::executeLockTable뿐입니다. */ 
IDE_RC           smiValidateAndLockObjects( smiTrans          * aTrans,
                                            const void*         aTable,
                                            smSCN               aSCN,
                                            smiTBSLockValidType aTBSLvType,
                                            smiTableLockMode    aLockMode,
                                            ULong               aLockWaitMicroSec,
                                            idBool              aIsExplcitLock );

IDE_RC           smiValidateObjects( const void        * aTable,
                                     smSCN               aSCN);

/* PROJ-2268 Reuse Catalog Table Slot 
 * Table Slot이 재활용되었는지 확인한다. */
IDE_RC           smiValidatePlanHandle( const void * aHandle,
                                        smSCN        aCachedSCN );

// 테이블 스페이스에 대해 Lock을 획득하고 Validation을 수행한다.
IDE_RC          smiValidateAndLockTBS( smiStatement      * aStmt,
                                      scSpaceID             aSpaceID,
                                      smiTBSLockMode        aLockMode,
                                      smiTBSLockValidType   aTBSLvType,
                                      ULong                 aLockWaitMicroSec );


IDE_RC           smiFetchRowFromGRID(
                                     idvSQL                      * aStatistics,
                                     smiStatement                * aStatement,
                                     UInt                          aTableType,
                                     scGRID                        aRowGRID,
                                     const smiFetchColumnList    * aFetchColumnList,
                                     void                        * aTableHdr,
                                     void                        * aDestRowBuf );

IDE_RC           smiGetGRIDFromRowPtr( const void   * aRowPtr,
                                       scGRID       * aRowGRID );


/*---------------------------------------------*/
/* 인덱스 정보 추출 함수                           */
/*---------------------------------------------*/


UInt             smiGetIndexId( const void* aIndex );

UInt             smiGetIndexType( const void* aIndex );


idBool           smiGetIndexRange( const void* aIndex );

idBool           smiGetIndexDimension( const void* aIndex );

const UInt*      smiGetIndexColumns( const void* aIndex );

const UInt*      smiGetIndexColumnFlags( const void* aIndex );

UInt             smiGetIndexColumnCount( const void* aIndex );

idBool           smiGetIndexBuiltIn( const void * aIndex );

UInt             smiEstimateMaxKeySize( UInt        aColumnCount,
                                        smiColumn * aColumns,
                                        UInt *      aMaxLengths );

UInt             smiGetIndexKeySizeLimit( UInt        aTableType,
                                          UInt        aIndexType );

/*---------------------------------------------*/
// For A4 : Variable Column Data retrieval
/*---------------------------------------------*/

const void* smiGetVarColumn( const void      * aRow,
                             const smiColumn * aColumn,
                             UInt            * aLength );

UInt smiGetVarColumnBufHeadSize( const smiColumn * aColumn );

// Variable Column Data Length retrieval
UInt smiGetVarColumnLength( const void*       aRow,
                            const smiColumn * aColumn );


/*---------------------------------------------*/
/* FOR A4 : Cursor 관련 함수들                   */
/*---------------------------------------------*/

smiRange * smiGetDefaultKeyRange( );
    
smiCallBack * smiGetDefaultFilter( );


/* check point 수행함수 */
IDE_RC smiCheckPoint( idvSQL  * aStatistics,
                      idBool    aStart );

/* In-Doubt 트랜잭션의 정보 얻기 */
IDE_RC smiXaRecover(SInt           *a_slotID, 
                    /* BUG-18981 */
                    ID_XID            *a_xid, 
                    timeval        *a_preparedTime,
                    smiCommitState *a_state); 

/*
 *  현재 Database의 selective loading 모드 얻기
 *  QP에서 pin, unpin 지원할 경우 검사해야 함.
 */
idBool isRuntimeSelectiveLoadingSupport();

/* ------------------------------------------------
 *  Status 정보를 출력하기 위함.
 * ----------------------------------------------*/

void smiGetTxLockInfo(smiTrans *aTrans, smTID *aOwnerList, UInt *aOwnerCount);


//  For A4 : TableSpace type별로 Maximum fixed row size를 반환한다.
//  slot header 포함.
UInt smiGetMaxFixedRowSize(smiTableSpaceType aTblSpaceType );


// PROJ-1362
// LOB Column Piece의 크기를 반환한다.
UInt smiGetLobColumnSize(UInt aTableType);

// LOB Column이 NULL(length == 0)인지 확인한다.
idBool smiIsNullLobColumn(const void*       aRow,
                          const smiColumn * aColumn);


/*
  For A4 :
      TableSpace별로 Database Size를 반환한다.
*/
#if 0 //not used 
ULong    smiTBSSize( scSpaceID aTableSpaceID );
#endif

// Disk Segment를 생성하기 위한 최소 Extent 개수
UInt smiGetMinExtPageCnt();

void smiSetEmergency(idBool aFlag);

void smiSwitchDDL(UInt aFlag);

UInt smiGetCurrTime();


/* ------------------------------------------------
 * interface for replication 
 * ----------------------------------------------*/
/* Callback Function을 Setting해준다.*/
/* BUG-26482 대기 함수를 CommitLog 기록 전후로 분리하여 호출합니다. */
void smiSetCallbackFunction(
        smGetMinSN                   aGetMinSNFunc,
        smIsReplCompleteBeforeCommit aIsReplCompleteBeforeCommitFunc = NULL,
        smIsReplCompleteAfterCommit  aIsReplCompleteAfterCommitFunc  = NULL,
        smCopyToRPLogBuf             aCopyToRPLogBufFunc             = NULL,
        smSendXLog                   aSendXLogFunc                   = NULL,
        smIsReplWaitGlobalTxAfterPrepare aIsReplWaitGlobalTxAfterPrepare = NULL );

IDE_RC smiGetLstLSNByNoLatch(smLSN &aLSN);

void smiGetLstDeleteLogFileNo( UInt *aFileNo );
IDE_RC smiGetFirstNeedLFN( smSN         aMinLSN,
                           const UInt * aFirstFileNo,
                           const UInt * aEndFileNo,
                           UInt       * aNeedFirstFileNo );


// 마지막으로 Log를 기록하기 위해 "사용된/저장된" LSN값을 리턴한다.
IDE_RC smiGetLastUsedGSN( smSN *aSN );

/* BUG-43426 */
IDE_RC smiWaitAndGetLastValidGSN( smSN *aSN );

// 마지막으로 기록한 로그 레코드의 LSN
IDE_RC smiGetLastValidGSN( smSN *aSN );
IDE_RC smiGetLastValidLSN( smLSN * aLSN );

// Transaction Table Size를 return한다.
UInt smiGetTransTblSize();

// Archive Log 경로를 return 한다.
const SChar * smiGetArchiveDirPath();

// Archive Mode return 한다.
smiArchiveMode smiGetArchiveMode();


// SM에 존재하는 시스템의 통계정보를 반영한다.
// 이 함수에서는 SM 내부의 각 모듈에서 시스템에 반영되어야 하는
// 통계정보를 v$sysstat으로 반영하도록 하면 된다.
void smiApplyStatisticsForSystem();


/***********************************************************************

 Description :

 SM의 한 tablespace의 내용을 검증한다.

 ALTER SYSTEM VERIFY 와 같은 류의 SQL을 사용자가 처리한다면
 QP에서 구문 파싱후  이 함수를 호출한다.

 현재는 SMI_VERIFY_TBS 만 동작하지만, 추후에 기능이 추가될 수 있다.

 호출 예) smiVerifySM(SMI_VERIFY_TBS)

 **********************************************************************/
IDE_RC smiVerifySM( idvSQL*  aStatistics, UInt      aFlag );

/* BUG-35392 FastUnlockLogAllocMutex 기능을 사용하는지 여부 */
idBool smiIsFastUnlockLogAllocMutex();

//PROJ-1362 QP-Large Record & Internal LOB support
// table의 컬럼갯수와 인덱스 개수 제한 풀기.
IDE_RC smiGetTableColumns( const void        * aTable,
                           const UInt          aIndex,
                           const smiColumn  ** aColumn );

/* alter system flush buffer_cache; */
IDE_RC smiFlushBufferPool( idvSQL * aStatistics );

IDE_RC smiFlushSBuffer( idvSQL * aStatistics );

IDE_RC smiFlusherOnOff(idvSQL *aStatistics, UInt aFlusherID, idBool aStart);

IDE_RC smiSBufferFlusherOnOff( idvSQL *aStatistics, 
                               UInt aFlusherID, 
                               idBool aStart );

IDE_RC smiUpdateIndexModule(void *aIndexModule);

/* BUG-49063 */
idBool smiIsAllowDeleteQueue( const void   * aTable );

/* PROJ-1723 [MDW/INTEGRATOR] Altibase Plugin 개발
   DDL Statement Text의 로깅
 */
IDE_RC smiWriteDDLStmtTextLog( idvSQL*          aStatistics,
                               smiStatement   * aStmt,
                               smiDDLStmtMeta * aDDLStmtMeta,
                               SChar *          aStmtText,
                               SInt             aStmtTextLen );

/*PROJ-1608 recovery from replication*/
smSN smiGetReplRecoverySN();
IDE_RC smiSetReplRecoverySN( smSN aReplRecoverySN );
IDE_RC smiGetSyncedMinFirstLogSN( smSN *aSN );

void smiGetRemoveMaxLastLogSN( smSN   *aSN );

/* BUG-20727 */
IDE_RC smiExistPreparedTrans(idBool *aExistFlag);

/* PROJ-1915 */
ULong smiGetLogFileSize();
UInt  smiGetSmVersionID();

/* BUG-21800 */
SChar* smiGetPsmSvpName( void );

IDE_RC smiRebuildMinViewSCN( idvSQL * aStatistics );

/*
 * BUG-27949 [5.3.3] add datafile 에서. DDL_LOCK_TIMEOUT 을 
 *           무시하고 있습니다. (근로복지공단) 
 */
ULong smiGetDDLLockTimeOut(smiTrans * aTrans);

IDE_RC smiWriteDummyLog();

/*fix BUG-31514 While a dequeue rollback ,
  another dequeue statement which currenlty is waiting for enqueue event
  might lost the  event  */
void   smiGetLastSystemSCN( smSCN *aLastSystemSCN );
void   smiInaccurateGetLastSystemSCN( smSCN *aLastSystemSCN );
IDE_RC smiSetLastSystemSCN( smSCN * aLastSystemSCN, smSCN * aNewLastSystemSCN );

smSN  smiGetValidMinSNOfAllActiveTrans();

void smiInitDMLRetryInfo( smiDMLRetryInfo * aRetryInfo);

// PROJ-2264
const void * smiGetCompressionColumn( const void      * aRow,
                                      const smiColumn * aColumn,
                                      idBool            aUseColumnOffset,
                                      UInt            * aLength);
// PROJ-2397
void * smiGetCompressionColumnFromOID( smOID           * aCompressionRowOID,
                                       const smiColumn * aColumn,
                                       UInt            * aLength );

// PROJ-2375 Global Meta 
const void * smiGetCompressionColumnLight( const void * aRow,
                                           scSpaceID    aColSpace,
                                           UInt         aVcInOutBaseSize,
                                           UInt         aFlag,
                                           UInt       * aLength );
// PROJ-2264
void smiGetSmiColumnFromMemTableOID( smOID        aTableOID,
                                     UInt         aIndex,
                                     smiColumn ** aSmiColumn );

// PROJ-2264
idBool smiIsDictionaryTable( void * aTableHandle );

// PROJ-2264
void * smiGetTableRuntimeInfoFromTableOID( smOID aTableOID );

// PROJ-2402
IDE_RC smiPrepareForParallel( smiTrans * aTrans,
                              UInt     * aParallelReadGroupID );

idBool smiIsConsistentIndices( const void * aTable );

/* PROJ-2433 */
UInt smiGetMemIndexKeyMinSize();
UInt smiGetMemIndexKeyMaxSize();
idBool smiIsForceIndexDirectKey();

UInt smiForceIndexPersistenceMode(); /* BUG-41121 */

// PROJ-1969
IDE_RC smiGetLstLSN( smLSN      * aEndLSN );

/* PROJ-2462 Result Cache */
void smiGetTableModifyCount( const void   * aTable,
                             SLong        * aModifyCount );

IDE_RC smiWriteXaStartReqLog( ID_XID * aGlobalXID,
                              smTID    aTID,
                              smLSN  * aLSN );

IDE_RC smiWriteXaPrepareReqLog( ID_XID * aGlobalXID,
                                smTID    aTID,
                                UInt     aGlobalTxId,
                                UChar  * aBranchTxInfo,
                                UInt     aBranchTxInfoSize,
                                smLSN  * aLSN );

IDE_RC smiWriteXaEndLog( smTID    aTID,
                         UInt     aGlobalTxId );

/* BUG-48501 */
void smiSetGlobalTxId( smTID   aTID,
                       UInt    aGlobalTxId );

IDE_RC smiSet2PCCallbackFunction( smGetDtxMinLSN aGetDtxMinLSN,
                                  smManageDtxInfo aManageDtxInfo );

/* PROJ-2626 Memory DB 와 Disk UNDO 의 현재 총 사이즈를 구한다. */
void smiGetMemUsedSize( ULong * aMemSize );
IDE_RC smiGetDiskUndoUsedSize( ULong * aSize );

/* MM에서 SM에 Callback함수를 넘겨주기 위한 Callback 등록 함수 */
IDE_RC smiSetSessionCallback( smiSessionCallback *aCallback );

/* PROJ-2733 Replica SCN 설정용 인터페이스 */
void smiSetReplicaSCN( void );

void smiSetTransactionalDDLCallback( smiTransactionalDDLCallback * aTransactionalDDLCallback );

SInt smiGetDDLLockTimeOutProperty();

#endif /* _O_SMI_MISC_H_ */
