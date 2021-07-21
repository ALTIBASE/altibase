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
 * $Id: smcTable.h 90259 2021-03-19 01:22:22Z emlee $
 *
 * Description :
 *
 * 본 파일은 table에 대한 헤더파일입니다.
 *
 * # 개념
 *
 * memory/disk(temporary 포함) table을 관리한다.
 * 현재, table이 포함하는 index 헤더에 대한 관리는 향후
 * smcIndex로 분리시킨다.
 *
 * # 구조
 *
 * - smc layer 접근경로
 *
 *                       smi
 *         _______________|_________________
 *        /               |                 \
 *   ___________          |                  ___________ __________
 *   |smcRecord|  _________________       | |sdcRecord|-|sdcKeyMap|
 *                |smcTable       |       |
 *                |_______________|       | sdcTss.. sdcUndo..
 *                ____/\__________________|
 *               /                        \         [sdc]
 *      _______   ____                    |\ _______
 *      |fixed|  |var|                    |  |entry|
 *                                        |
 *            [smp]                       |     [sdp]
 *
 *
 *  1. 테이블 생성 및 제거
 *
 *     memory 타입이든 disk 타입의 테이블 헤더는 모두 MRDB의 catalog
 *     table에서 관리가 되므로 smcTable의 creatTable/dropTable 함수를
 *     공유하여 사용한다
 *     disk table의 page list entry에서는 tablespace의 segment를 하나
 *     생성하여 필요한 page를 할당받아 tuple을 저장한다.disk table은
 *     memory table과 다르게 fixed column 데이타와 variable column 데이
 *     타를 구분하여 저장하지 않기때문에 page list entry는 하나만 존재한다.
 *
 *     !!] pending 연산 : disk G.C에 의해서만 처리되도록 함
 *     table 제거는 pending operation으로 처리되며, memory table의 경우는
 *     ager에 의해 disk table은 disk G.C에 의해 smcTable::dropTablePending()을
 *     호출하여 처리된다. restart recovery후에 refine 수행과정에서 catalog table
 *     을 검색하면서 disk table 타입의 table header를 판독한 경우에 대하여
 *     refine을 고려하지 않으며, 이후에 disk G.C가 start되면 수행 하도록 맡긴다.
 *
 *  2. disk temporary 테이블
 *
 *     - temproary table for meta table
 *       createdb 수행과정중에 sys_* 등의 catalog table을 생성하는 과정에서
 *       smcTable::createTable()함수를 호출하여 생성한다.
 *
 *     !!] meta table에 대하여 page list entry들의 page 할당에 대해서만
 *         모두 logging을 하며, temporary table은 system 종료시에 할당되었던
 *         모든 fixed page list entry의 page들과 var page list entry의 page들을
 *         모두 truncate 시킨다.
 *
 *     - temporary table
 *
 *     temporary table에 대한 생성 및 제거는 일반적인 table
 *     의 처리과정과 다르므로 createTempTable/dropTempTable을 사용하여
 *     no logging, no lock으로 처리한다.
 *     단, slot 할당시에 page 할당관련 logging은 수행하여야 한다.
 *
 *                                                meta table의 fixed slot을
 *                                                할당하여 smcTableHeader를
 *                                                저장한다.
 *                                                        |
 *    _________________________________          _________v_______
 *   | meta table for temporaroy table |         |smcTableHeader |
 *   | : catalog table로부터 fixed slot|  --->   |               |
 *   |___을 할당받아 생성한다.  _______|         |_______________|
 *            |                 |                         |
 *            |                 ㅁ               _________V_________
 *            o                 |                 | smnIndexHeader  |
 *            |                 ㅁ var            |_________________|
 *            o fixed              page list
 *              page list entry    entry         meta table의 var slot을
 *                                               할당하여 smnIndexHeader를
 *                                               저장한다.
 *
 *     !!] 할당되었던 temporary tablespace에 대하여 reset 처리를 한다.
 *
 *
 *  3. disk table에 대한 alter table add/drop column ...등의 DDL에 대한 처리
 *
 *     메모리 테이블은 memory 증가문제를 해소하기 위해 아래와 같은 방법을
 *     사용하여 처리하던 방식을 쓰지 않지만, disk table은 아래와 같은
 *     방식을 그대로 사용해도 문제가 없다.
 *
 *     새로운 table을 생성한 다음에 기존 table과 새로운 table에 대하여
 *     각각 select cursor와 insert cursor를 open하여 rows를 새로운 테이블에
 *     insert한후 old table에 대하여 drop table을 수행한다.
 *     -> QP단에서 처리 => moveRows() .. 등등
 *
 *
 *
 *  4.인덱스 생성 및 제거
 *
 *     인덱스 생성시 모든 과정은 대부분 동일하며,
 *     단, index header를 초기화하는 코드를 해당 index manager를 호출하여
 *     초기화하도록 한다.
 *
 *     !!] pending 연산 : disk G.C에 의해서만 처리되도록 함
 *     index 제거는 pending operation으로 처리되며, memory index의 경우는
 *     transaction commit 시점에서 처리되면 crash시에는 runtime 정보인 index가
 *     자동으로 소멸되므로 고려하지 않아도 된다. 반면에 disk index의 경우는
 *     transaction commit 시점에 처리되지 않고 disk G.C에 의해만 처리되
 *     도록 하며, crash시에도 persistent하기 때문에 disk G.C에 의해 처리될수
 *     있도록 한다. (실제 제거작업을 할때는 smcTable::dropIndexPending() 호출)
 *
 *
 *  5. DDL에 대한 recovery
 *
 *  !!]위와 같은 DDL 수행에 있어서 memory tablespace에 대한 로깅과
 *     disk tablespace 영역에 대한 로깅이 혼합되는 경우에 있어서
 *     정확한 transaction rollback이나 recovery가 보장되어야 한다.
 *
 *  : 예를 들어, disk table 생성과정중 대략적인 로깅은 다음과 같다.
 *
 *    1) catalog table에서 fixed slot 할당에 대한 로그
 *    2) disk data page를 위한 segment 할당에 대한
 *       operation NTA disk d-로그 (SMR_LT_DRDB_OP_NTA)
 *       segment 할당이 이루어짐
 *    3) SMR_SMC_TABLEHEADER_INIT ..
 *    4) table header 할당 및 초기화에 대한
 *       SMR_OP_CREATE_TABLE NTA m-로그
 *    5) column, index, ...등등에 대한 처리
 *       ...
 *
 *    로깅한 후에 segment할당에 대한 operation NTA disk 로그를 로깅을 하고,
 *    crash가 발생하였다.
 *    그렇다면, restart recovery시에 해당 DDL을 수행한
 *    uncommited transaction이 진행한 로그를 최근 순서부터 판독할것이며,
 *
 *    - SMR_OP_CREATE_TABLE을 판독하면, logical undo로써 dropTable을
 *      처리하며, nta 구간을 판독하지 않고 넘어간다.
 *    - 만약, segment 할당에 대한 operation NTA disk 로그를 판독하게 되면,
 *      해당 로그헤더에 저장된 sdRID에 대한 segment 해제를 수행하면 된다.
 *
 * 6. refine catalog table
 *
 *    disk G.C가 drop table에 대한 pending 연산을 처리하는 방법은
 *    다음과 같다.
 *
 *    tss commit list에서 drop table에 대한 pending 연산을 처리는
 *    smcTable::dropTablePending 함수를 통해서 수행됨
 *    1) 이 과정에서 free된 segment rid를 저장한 sdpPageListEntry의
 *       mDataSegDescRID를 SD_NULL_RID로 설정한다.(로깅)
 *    2) free data segment 한다. (1번과 2번과정은 NTA영역이다)
 *    3) 만약 1번과정만 처리된 후 죽었다면, undoAll과정에서 이전값으로
 *       복구가 가능할것이고, 2번까지 처리했다면 free segment를 undo
 *       하지 않을 것이다.
 *    4) 이러한 상황이라면 다음과 같은 결론을 얻는다.
 *       : refine DB시에 disk table header의 entry->mDataSegDescRID가
 *         SD_NULL_RID라면 --> free table header slot을 수행한다.
 *       : 그렇지 않다면 --> dropTablePending을 처리하고,
 *         다음 refineDB시에 free table header slot을 수행한다.
 *
 **********************************************************************/

#ifndef _O_SMC_TABLE_H_
#define _O_SMC_TABLE_H_ 1

#include <smDef.h>
#include <sdb.h>
#include <smu.h>
#include <smr.h>
#include <smm.h>
#include <smcDef.h>
#include <smcCatalogTable.h>
#include <smnDef.h>

class smcTable
{
public:
    static IDE_RC initialize();

    static IDE_RC destroy();

    // Table의 Lock Item을 초기화하고 Runtime Item들을 NULL로 세팅한다.
    static IDE_RC initLockAndSetRuntimeNull( smcTableHeader* aHeader );

    // Table Header의 Runtime Item들을 NULL로 초기화한다.
    static IDE_RC setRuntimeNull( smcTableHeader* aHeader );

    /* FOR A4 : runtime정보를 초기화 */
    static IDE_RC initLockAndRuntimeItem( smcTableHeader * aHeader );

    /* table header의 lock item및 mSync초기화 */
    static IDE_RC initLockItem( smcTableHeader * aHeader );

    /* table header의 runtime item 초기화 */
    static IDE_RC initRuntimeItem( smcTableHeader * aHeader );

    /* FOR A4 : table header의 lock item과 runtime정보를 해제 */
    static IDE_RC finLockAndRuntimeItem( smcTableHeader * aHeader );

    /* table header의 lock item 해제 */
    static IDE_RC finLockItem( smcTableHeader * aHeader );


    /* table header의 runtime item 해제 */
    static IDE_RC finRuntimeItem( smcTableHeader * aHeader );

    // table
    static IDE_RC initTableHeader( void                * aTrans,
                                   scSpaceID             aSpaceID,
                                   vULong                aMax,
                                   UInt                  aColumnSize,
                                   UInt                  aColumnCount,
                                   smOID                 aFixOID,
                                   UInt                  aFlag,
                                   smcSequenceInfo*      aSequence,
                                   smcTableType          aType,
                                   smiObjectType         aObjectType,
                                   ULong                 aMaxRow,
                                   smiSegAttr            aSegAttr,
                                   smiSegStorageAttr     aSegStorageAttr,
                                   UInt                  aParallelDegree,
                                   smcTableHeader     *  aHeader );

    /* Column List정보로 부터 Column갯수와 Fixed Row의 크기를 구한다.*/
    static IDE_RC validateAndGetColCntAndFixedRowSize(
                                   const UInt           aTableType,
                                   const smiColumnList *aColumnList,
                                   UInt                *aColumnCnt,
                                   UInt                *aFixedRowSize);

    /* memory/disk 타입 table을 생성한다. */
    static IDE_RC createTable( idvSQL                *aStatistics,
                               void*                  aTrans,
                               scSpaceID              aSpaceID,
                               const smiColumnList*   aColumnList,
                               UInt                   aColumnSize,
                               const void*            aInfo,
                               UInt                   aInfoSize,
                               const smiValue*        aNullRow,
                               UInt                   aFlag,
                               ULong                  aMaxRow,
                               smiSegAttr             aSegAttr,
                               smiSegStorageAttr      aSegStorageAttr,
                               UInt                   aParallelDegree,
                               const void**           aHeader );

    /* Table Header의 Info를 갱시한다. */
    static IDE_RC setInfoAtTableHeader( void*            aTrans,
                                        smcTableHeader*  aTable,
                                        const void*      aInfo,
                                        UInt             aInfoSize );

    /* Table의 Header에 새로운 Column 리스트로 갱신한다. */
    static IDE_RC setColumnAtTableHeader( void*                aTrans,
                                          smcTableHeader*      aTableHeader,
                                          const smiColumnList* aColumnsList,
                                          UInt                 aColumnCnt,
                                          UInt                 aColumnSize );

    /* FOR A4 : memory/disk 타입 table을 제거한다. */
    static IDE_RC dropTable  ( void                 * aTrans,
                               const void           * aHeader,
                               sctTBSLockValidOpt     aTBSLvOpt,
                               UInt                   aFlag = (SMC_LOCK_TABLE));

    static IDE_RC alterIndexInfo(void    *       aTrans,
                                 smcTableHeader* aHeader,
                                 void          * aIndex,
                                 UInt            aFlag);


    /* PROJ-2162 RestartRiskReduction
     * InconsistentFlag를 설정한다. */
    static IDE_RC setIndexInconsistency( smOID            aTableOID,
                                         smOID            aIndexID );

    static IDE_RC setAllIndexInconsistency( smcTableHeader  * aTableHeader );
                                        

    // PROJ-1671
    static IDE_RC alterTableSegAttr( void                  * aTrans,
                                     smcTableHeader        * aHeader,
                                     smiSegAttr              aSegAttr,
                                     sctTBSLockValidOpt      aTBSLvOpt );

    static IDE_RC alterTableSegStoAttr( void               * aTrans,
                                        smcTableHeader     * aHeader,
                                        smiSegStorageAttr    aSegStoAttr,
                                        sctTBSLockValidOpt   aTBSLvOpt );

    static IDE_RC alterTableAllocExts( idvSQL              * aStatistics,
                                       void                * aTrans,
                                       smcTableHeader      * aHeader,
                                       ULong                 aExtendSize,
                                       sctTBSLockValidOpt    aTBSLvOpt );

    static IDE_RC alterIndexSegAttr( void                * aTrans,
                                     smcTableHeader      * aHeader,
                                     void                * aIndex,
                                     smiSegAttr            aSegAttr,
                                     sctTBSLockValidOpt    aTBSLvOpt );
    
    static IDE_RC alterIndexSegStoAttr( void                * aTrans,
                                        smcTableHeader      * aHeader,
                                        void                * aIndex,
                                        smiSegStorageAttr     aSegStoAttr,
                                        sctTBSLockValidOpt    aTBSLvOpt );

    static IDE_RC alterIndexAllocExts( idvSQL              * aStatistics,
                                       void                * aTrans,
                                       smcTableHeader      * aHeader,
                                       void                * aIndex,
                                       ULong                 aExtendSize,
                                       sctTBSLockValidOpt    aTBSLvOpt );

    // fix BUG-22395
    static IDE_RC alterIndexName ( idvSQL              * aStatistics,
                                   void                * aTrans,
                                   smcTableHeader      * aHeader,
                                   void                * aIndex,
                                   SChar               * aName );

    static IDE_RC setUseFlagOfAllIndex(void            * aTrans,
                                       smcTableHeader  * aHeader,
                                       UInt              aFlag);
    // table에 Lock획득
    static IDE_RC lockTable( void*               aTrans,
                             smcTableHeader*     aHeader,
                             sctTBSLockValidOpt  aTBSLvOpt,
                             UInt                aFlag );

    static IDE_RC compact( void           * aTrans,
                           smcTableHeader * aTable,
                           UInt             aPages );
    
    static IDE_RC validateTable( void              * aTrans,
                                 smcTableHeader    * aHeader,
                                 sctTBSLockValidOpt  aTBSLvOpt,
                                 UInt                aFlag = (SMC_LOCK_TABLE));

    static IDE_RC modifyTableInfo( void                 * aTrans,
                                   smcTableHeader       * aHeader,
                                   const smiColumnList  * aColumns,
                                   UInt                   aColumnSize,
                                   const void           * aInfo,
                                   UInt                   aInfoSize,
                                   UInt                   aFlag,
                                   ULong                  aMaxRow,
                                   UInt                   aParallelDegree,
                                   sctTBSLockValidOpt     aTBSLvOpt,
                                   idBool                 aIsInitRowTemplate );

    /* FOR A4 : disk/memory index를 생성한다. */
    static IDE_RC createIndex( idvSQL                 *aStatistics,
                               void                  * aTrans,
                               scSpaceID               aSpaceID,
                               smcTableHeader        * aHeader,
                               SChar                 * aName,
                               UInt                    aID,
                               UInt                    aType,
                               const smiColumnList   * aColumns,
                               UInt                    aFlag,
                               UInt                    aBuildFlag,
                               smiSegAttr            * aSegAttr,
                               smiSegStorageAttr     * aSegStorageAttr,
                               ULong                   aDirectKeyMaxSize,
                               void                 ** aIndex );

    /* FOR A4 : disk/memory index를 제거한다. */
    static IDE_RC dropIndex( void              * aTrans,
                             smcTableHeader    * aHeader,
                             void              * aIndex,
                             sctTBSLockValidOpt  aTBSLvOpt );

    static IDE_RC dropIndexByAbortHeader( idvSQL         * aStatistics,
                                          void           * aTrans,
                                          smcTableHeader * aHeader,
                                          const  UInt      aIdx,
                                          void           * aIndexHeader,
                                          smOID            aOIDIndex );

    static IDE_RC dropIndexPending( void* aIndexHeader );

    static IDE_RC dropIndexList(smcTableHeader * aHeader);

    static IDE_RC dropTablePending( idvSQL           *aStatistics,
                                    void*             aTrans,
                                    smcTableHeader*   aHeader,
                                    idBool            aFlag = ID_TRUE );

    // for alter table PR-4360
    static IDE_RC dropTablePageListPending( void           * aTrans,
                                            smcTableHeader * aHeader,
                                            idBool           aUndo );

    inline static const smiColumn * getColumn( const void  * aHeader,
                                               const UInt    aIndex );

    /* SegmentGRID를 가지는 Lob 컬럼을 반환한다. */
    static const smiColumn* findLOBColumn( void  * aHeader,
                                           scGRID  aSegGRID );

    static UInt getColumnIndex( UInt aColumnID );
    static UInt getPrimaryKeySize(smcTableHeader*    aHeader,
                                  SChar*             aFixRow);

    static UInt getColumnCount(const void *aTableHeader);
    static UInt getLobColumnCount(const void *aTableHeader);

    static const smiColumn* getLobColumn( const void *aTableHeader,
                                          scSpaceID   aSpaceID,
                                          scPageID    aSegPID );

    static IDE_RC createLOBSegmentDesc( smcTableHeader * sHeader );
    static IDE_RC destroyLOBSegmentDesc( smcTableHeader * sHeader );

    static UInt getColumnSize(void *aTableHeader);

    static void* getDiskPageListEntry( void *aTableHeader );
    static void* getDiskPageListEntry( smOID aTableOID );

    static ULong getMaxRows(void *aTableHeader);
    static smOID getTableOID(const void *aTableHeader);

    /* BUG-32479 [sm-mem-resource] refactoring for handling exceptional case
     * about the SMM_OID_PTR and SMM_PID_PTR macro .
     * SMC_TABLE_HEADER를 예외처리하는 함수로 변환함. */
    inline static IDE_RC getTableHeaderFromOID( smOID    aOID, 
                                                void  ** aHeader );

    static IDE_RC getIndexHeaderFromOID( smOID    aOID, 
                                         void  ** aHeader );

    // disk table에 한하여 호출됨 (NTA undo시)
    static IDE_RC doNTADropDiskTable( idvSQL *aStatistics,
                                      void*   aTrans,
                                      SChar*  aRow );

    static IDE_RC getRecordCount(smcTableHeader * aHeader,
                                 ULong          * aRecordCount);

    static IDE_RC setRecordCount(smcTableHeader * aHeader,
                                 ULong            aRecordCount);

    static inline idBool isDropedTable( smcTableHeader * aHeader );

    /* BUG-47367 Delete Thread 전용 Drop Check 함수 */
    static inline idBool isDropedTable4Ager( smcTableHeader * aHeader, smSCN aSCN );


    //PROJ-1362 LOB에서 index개수 제약풀기 part.
    inline static UInt getIndexCount( void * aHeader );

    inline static const void * getTableIndex( void     * aHeader,
                                              const UInt aIdx );
    static const void * getTableIndexByID( void         * aHeader,
                                           const UInt     aIdx );

    static IDE_RC waitForFileDelete( idvSQL *aStatistics, SChar* aStrFileName);

    inline static SChar* getBackupDir();

    static IDE_RC deleteAllTableBackup();

    static IDE_RC  copyAllTableBackup( SChar      * aSrcDir,
                                       SChar      * aTargetDir );


    static void  getTableHeaderFlag(void*,
                                    scPageID*,
                                    scOffset*,
                                    UInt*);

    static UInt getTableHeaderFlagOnly( void* );

    /* PROJ-2162 RestartRiskReduction */
    static void  getTableIsConsistent( void     * aTable,
                                       scPageID * aPageID,
                                       scOffset * aOffset,
                                       idBool   * aFlag );


    static IDE_RC  dropIndexByAbortOID( idvSQL *aStatistics,
                                        void *  aTrans,
                                        smOID   aOldIndexOID,
                                        smOID   aNewIndexOID );

    static IDE_RC  clearIndexList(smOID aTblHeaderOID);

    //BUG-15627 Alter Add Column시 Abort할 경우 Table의 SCN을 변경해야 함.
    //logical undo, 통계정보갱신시 불리는 함수
    static void    changeTableSCNForRebuild( smOID aTableOID );

    // BUG-25611 
    // memory table에 대한 LogicalUndo시, 아직 Refine돼지 않은 상태이기 때문에
    // 임시로 테이블을 Refine해준다.
    static IDE_RC prepare4LogicalUndo( smcTableHeader * aTable );

    // BUG-25611 
    // memory table에 대한 LogicalUndo시, 임시로 Refine된 것을 다시
    // 정리해준다.
    static IDE_RC finalizeTable4LogicalUndo(smcTableHeader * aTable,
                                            void           * aTrans );

    static void  getTableHeaderInfo( void*,
                                     scPageID*,
                                     scOffset*,
                                     smVCDesc*,
                                     smVCDesc*,
                                     UInt*,
                                     UInt*,
                                     ULong*,
                                     UInt*);


    static IDE_RC getTablePageCount( void *aHeader, ULong *aPageCnt );

    static inline scSpaceID getTableSpaceID( void * aHeader );

    static void   addToTotalIndexCount( UInt aCount );

    static IDE_RC freeColumns(idvSQL          *aStatistics,
                              void* aTrans,
                              smcTableHeader* aTableHeader);

    // PROJ-1362 QP Large Record & Internal LOB 지원.
    // 최대 컬럼갯수를 저장한다고 가정할때 사용되는
    // vairable slot갯수를 구한다.
    static inline UInt  getMaxColumnVarSlotCnt(const UInt aColumnSize);
    static inline UInt  getColumnVarSlotCnt(smcTableHeader* aTableHeader);
    static void  buildOIDStack(smcTableHeader* aTableHeader,
                               smcOIDStack*    aOIDStack);

    static void adjustIndexSelfOID( UChar * aIndexHeaders,
                                    smOID   aStartIndexOID,
                                    UInt    aTotalIndexHeaderSize );
    
    static IDE_RC findIndexVarSlot2Insert(smcTableHeader *aHeader,
                                          UInt           aFlag,
                                          const UInt     aIndexHeaderSize,
                                          UInt           *aTargetIndexVarSlotIdx);
    
    // BUF-23218 : DROP INDEX 외 ALTER INDEX에서 쓸 수 있도록 이름 수정
    static IDE_RC findIndexVarSlot2AlterAndDrop(smcTableHeader *aHeader,
                                                void*                aIndex,
                                                const UInt           aIndexHeaderSize,
                                                UInt*                aTargetIndexVarSlotIdx,
                                                SInt*                aOffset );

    static IDE_RC freeIndexes( idvSQL          * aStatistics,
                               void            * aTrans,
                               smcTableHeader  * aTableHeader,
                               UInt              aTableType );

    /* FOR A4 :
       fixed page list entry와 variable page list entries 초기화 */
    static void   initMRDBTablePageList( vULong            aMax,
                                         smcTableHeader*   aHeader );

    /* volatile table의 page list entry 초기화 */
    static void   initVRDBTablePageList( vULong            aMax,
                                         smcTableHeader*   aHeader );

    /* disk page list entry를 초기화 및 segment 할당 */
    static void   initDRDBTablePageList( vULong            aMax,
                                         smcTableHeader*   aHeader,
                                         smiSegAttr        aSegAttr,
                                         smiSegStorageAttr aSegStorageAttr );

    /* online disk tbs/restart recovery시에 disk table의 runtime index header rebuild */
    static IDE_RC rebuildRuntimeIndexHeaders(
                                         idvSQL          * aStatistics,
                                         smcTableHeader  * aHeader,
                                         ULong             aMaxSmoNo );
    /* For Pararrel Loading */
    static IDE_RC loadParallel( SInt   aDbPingPong,
                                idBool aLoadMetaHeader );

    static inline idBool isReplicationTable(const smcTableHeader* aHeader);
    /* BUG-39143 */
    static inline idBool isTransWaitReplicationTable( const smcTableHeader* aHeader );

    static inline idBool isSupplementalTable(const smcTableHeader* aHeader);

    static idBool needReplicate(const smcTableHeader* aTableHeader,
                                void*  aTrans);

    // BUG-17371  [MMDB] Aging이 밀릴경우 System에 과부하
    //           및 Aging이 밀리는 현상이 더 심화됨.
    static void incStatAtABort( smcTableHeader * aHeader,
                                smcTblStatType   aTblStatType );

    static IDE_RC setNullRow(void*             aTrans,
                             smcTableHeader*   aTable,
                             UInt              aTableType,
                             void*             aData);

    static IDE_RC makeLobColumnList(
                                void             * aTableHeader,
                                smiColumn     ***  aArrLobColumn,
                                UInt             * aLobColumnCnt );

    static IDE_RC destLobColumnList( void *aColumnList) ;

    /* PROJ-1594 Volatile TBS */
    /* initAllVolatileTables()에서 사용되는 함수로,
       callback 함수를 이용해 null row를 얻어온다. */
    static IDE_RC makeNullRowByCallback(smcTableHeader *aTableHeader,
                                        smiColumnList  *aColList,
                                        smiValue       *aNullRow,
                                        SChar          *aValueBuffer);

    static scSpaceID getSpaceID(const void *aTableHeader);
    static scPageID  getSegPID( void *aTable );

    ///////////////////////////////////////////////////////////////////////
    // BUG-17371  [MMDB] Aging이 밀릴경우 System에 과부하
    //           및 Aging이 밀리는 현상이 더 심화됨.
    //
    // => 해결 : AGER를 병렬로 수행
    // AGER Thread가 2개 이상이 되면 iduSync로 동시성 제어 불가능.
    // TableHeader의 iduSync대신 iduLatch를 이용하도록 코드를 변경함

    // Table Header의 Index Latch를 할당하고 초기화
    static IDE_RC allocAndInitIndexLatch(smcTableHeader * aTableHeader );

    // Table Header의 Index Latch를 파괴하고 해제
    static IDE_RC finiAndFreeIndexLatch(smcTableHeader * aTableHeader );

    // Table Header의 Index Latch에 Exclusive Latch를 잡는다.
    static IDE_RC latchExclusive(smcTableHeader * aTableHeader );

    // Table Header의 Index Latch에 Shared Latch를 잡는다.
    static IDE_RC latchShared(smcTableHeader * aTableHeader );

    //Table Header의 Index Latch를 풀어준다.
    static IDE_RC unlatch(smcTableHeader * aTableHeader );


    /* BUG-30378 비정상적으로 Drop되었지만 refine돼지 않는 
     * 테이블이 존재합니다.
     * dumpTableHeaderByBuffer를 이용하는 래핑 함수 */
    static void dumpTableHeader( smcTableHeader * aTable );

    /* BUG-31206    improve usability of DUMPCI and DUMPDDF 
     * Util에서도 쓸 수 있는 버퍼용 함수*/
    static void dumpTableHeaderByBuffer( smcTableHeader * aTable,
                                         idBool           aDumpBinary,
                                         idBool           aDisplayTable,
                                         SChar          * aOutBuf,
                                         UInt             aOutSize );

    // PROJ-1665 : Table의 Parallel Degree를 넘겨줌
    static UInt getParallelDegree( void * aTableHeader );

    // PROJ-1665
    static idBool isLoggingMode( void * aTableHeader );

    // PROJ-1665
    static idBool isTableConsistent( void * aTableHeader );

    // Table의 Flag를 변경한다.
    static IDE_RC alterTableFlag( void           * aTrans,
                                  smcTableHeader * aTableHeader,
                                  UInt             aFlagMask,
                                  UInt             aFlagValue );

    // Table의 Insert Limit를 변경한다.
    static IDE_RC alterTableSegAttr( void           * aTrans,
                                     smcTableHeader * aTableHeader,
                                     smiSegAttr       aSegAttr );

    /* PROJ-2162 RestartRiskReduction
     * InconsistentFlag를 설정한다. */
    static IDE_RC setTableHeaderInconsistency(smOID            aTableOID );

    static IDE_RC setTableHeaderDropFlag( scSpaceID    aSpaceID,
                                          scPageID     aPID,
                                          scOffset     aOffset,
                                          idBool       aIsDrop );

    // Table Meta Log Record를 기록한다.
    static IDE_RC writeTableMetaLog(void         * aTrans,
                                    smrTableMeta * aTableMeta,
                                    const void   * aLogBody,
                                    UInt           aLogBodyLen);

    static inline smcTableType getTableType( void* aTable );

    static idBool isSegPIDOfTbl( void* aTable, scSpaceID aSpaceID, scPageID aSegPID );
    static idBool isIdxSegPIDOfTbl( void* aTable, scSpaceID aSpaceID, scPageID aSegPID );
    static idBool isLOBSegPIDOfTbl( void* aTable, scSpaceID aSpaceID, scPageID aSegPID );

    static IDE_RC initRuntimeInfos(
                               idvSQL         * aStatistics,
                               smcTableHeader * aHeader,
                               void           * aActionArg );

    static IDE_RC verifyIndexIntegrity(
                               idvSQL         * aStatistics,
                               smcTableHeader * aHeader,
                               void           * aActionArg );

    static smiColumn* getColumnAndOID(const void* aTableHeader,
                                      const UInt  aIndex,
                                      smOID*      aOID);

    static IDE_RC initRowTemplate( idvSQL         * aStatistics,
                                   smcTableHeader * aHeader,
                                   void           * aActionArg );

    static IDE_RC destroyRowTemplate( smcTableHeader * aHeader );

    static void dumpRowTemplate( smcRowTemplate * aRowTemplate );

    /* PROJ-2462 Result Cache */
    static void inline addModifyCount( smcTableHeader * aTableHeader );

    static idBool isNullSegPID4DiskTable( void * aTableHeader );
        
    static IDE_RC setTableCreateSCN( smcTableHeader * aHeader, smSCN  aSCN );
private:

    // Create Table을 위한 에러처리
    static IDE_RC checkError4CreateTable( scSpaceID aSpaceID,
                                          UInt      aTableFlag );


    // Table의 Flag변경에 대한 에러처리
    static IDE_RC checkError4AlterTableFlag( smcTableHeader * aTableHeader,
                                             UInt             aFlagMask,
                                             UInt             aFlagValue );

    // PROJ-1362 QP Large Record & Internal LOB 지원.
    // column , index 개수 제약 풀기.
    static IDE_RC storeTableColumns(void*   aTrans,
                                    UInt    aColumnSize,
                                    const smiColumnList* aColumns,
                                    UInt                aTableType,
                                    smOID*  aHeadVarOID);

    static IDE_RC  freeLobSegFunc(idvSQL*           aStatistics,
                                  void*             aTrans,
                                  scSpaceID         aColSlotSpaceID,
                                  smOID             aColSlotOID,
                                  UInt              aColumnSize,
                                  sdrMtxLogMode     aLoggingMode);

    static IDE_RC  freeLobSegNullFunc(idvSQL*           aStatistics,
                                      void*             aTrans,
                                      scSpaceID         aColSlotSpaceID,
                                      smOID             aColSlotOID,
                                      UInt              aColumnSize,
                                      sdrMtxLogMode     aLoggingMode);


    // Table에 할당된 모든 FreePageHeader를 초기화
    static void   initAllFreePageHeader( smcTableHeader* aTableHeader );

    static IDE_RC removeIdxHdrAndCopyIndexHdrArr(
                                        void*     aTrans,
                                        void*     aDropIndexHeader,
                                        void*     aIndexHeaderArr,
                                        UInt      aIndexHeaderArrLen,
                                        smOID     aOIDIndexHeaderArr );

//For Member
public:
    // mDropIdxPagePool은 이런 Drop된 IndexHeader를 저장할 Memory Pool이다.
    static iduMemPool mDropIdxPagePool;

    static SChar      mBackupDir[SM_MAX_FILE_NAME];

    // 현재 Table에 생성된 Index의 갯수
    static UInt       mTotalIndex;

private:
};

SChar* smcTable::getBackupDir()
{
    return mBackupDir;
}

/***********************************************************************
 * Description : aHeader가 가리키는 Table이 Replication이 걸려있으면 ID_TRUE,
 *               아니면 ID_FALSE.
 *
 * aHeader   - [IN] Table Header
 ***********************************************************************/
inline idBool smcTable::isReplicationTable(const smcTableHeader* aHeader)
{
    IDE_DASSERT(aHeader != NULL);

    return  ((aHeader->mFlag & SMI_TABLE_REPLICATION_MASK) == SMI_TABLE_REPLICATION_ENABLE ?
             ID_TRUE : ID_FALSE);
}
inline idBool smcTable::isTransWaitReplicationTable( const smcTableHeader* aHeader )
{
    IDE_DASSERT(aHeader != NULL);

    return  ( (aHeader->mFlag & SMI_TABLE_REPLICATION_TRANS_WAIT_MASK) == 
              SMI_TABLE_REPLICATION_TRANS_WAIT_ENABLE ? ID_TRUE : ID_FALSE );
}
inline idBool smcTable::isSupplementalTable(const smcTableHeader* aHeader)
{
    IDE_DASSERT(aHeader != NULL);

    return ((aHeader->mFlag & SMI_TABLE_SUPPLEMENTAL_LOGGING_MASK) == SMI_TABLE_SUPPLEMENTAL_LOGGING_TRUE ?
             ID_TRUE : ID_FALSE);
}

inline  UInt  smcTable::getMaxColumnVarSlotCnt(const UInt aColumnSize)
{
    UInt sColumnLen = aColumnSize * SMI_COLUMN_ID_MAXIMUM;

    return ( sColumnLen / SMP_VC_PIECE_MAX_SIZE)  + ((sColumnLen % SMP_VC_PIECE_MAX_SIZE) == 0 ? 0:1) ;
}

inline  UInt  smcTable::getColumnVarSlotCnt(smcTableHeader* aTableHeader)
{
    UInt sColumnLen = (aTableHeader->mColumnSize) * (aTableHeader->mColumnCount);

    return ( sColumnLen / SMP_VC_PIECE_MAX_SIZE)  + ((sColumnLen % SMP_VC_PIECE_MAX_SIZE) == 0 ? 0:1) ;
}

inline  UInt  smcTable::getParallelDegree(void * aTableHeader)
{
    IDE_ASSERT( aTableHeader != NULL );

    return ((smcTableHeader*)aTableHeader)->mParallelDegree;
}

inline idBool smcTable::isLoggingMode(void * aTableHeader)
{
    idBool sIsLogging;

    IDE_ASSERT( aTableHeader != NULL );

    if( (((smcTableHeader*)aTableHeader)->mFlag &
         SMI_TABLE_LOGGING_MASK)
        == SMI_TABLE_LOGGING )
    {
        sIsLogging = ID_TRUE;
    }
    else
    {
        sIsLogging = ID_FALSE;
    }

    return sIsLogging;
}

inline idBool smcTable::isTableConsistent(void * aTableHeader)
{
    IDE_ASSERT( aTableHeader != NULL );

    return  ((smcTableHeader*)aTableHeader)->mIsConsistent;
}

inline smcTableType smcTable::getTableType( void* aTable )
{
    smcTableHeader *sTblHdr = (smcTableHeader*)((smpSlotHeader*)aTable + 1);
    return sTblHdr->mType;
}

inline idBool smcTable::isNullSegPID4DiskTable( void * aTableHeader )
{
    idBool isNullSegPID = ID_FALSE;

    if ( sdpSegDescMgr::getSegPID( &(((smcTableHeader*)aTableHeader)->mFixed.mDRDB) ) 
                        == SD_NULL_RID )
    {
        isNullSegPID = ID_TRUE;
    }
    else
    {
        isNullSegPID = ID_FALSE;
    }

    return isNullSegPID;
}

/***********************************************************************
 * Description : get space id of a table
 ***********************************************************************/
inline scSpaceID smcTable::getTableSpaceID( void * aHeader )
{
    IDE_DASSERT( aHeader != NULL );

    return ((smcTableHeader *)aHeader)->mSpaceID;
}

/***********************************************************************
 * Description : table이 drop 되었는지 검사한다.
 ***********************************************************************/
inline idBool smcTable::isDropedTable( smcTableHeader * aHeader )
{
    smpSlotHeader * sSlotHeader = NULL;

    sSlotHeader = (smpSlotHeader *)((smpSlotHeader *)aHeader - 1);
    if ( SM_SCN_IS_DELETED( sSlotHeader->mCreateSCN ) == ID_TRUE )
    {
        return ID_TRUE;
    }

    return ID_FALSE;
}

/***********************************************************************
 * BUG-47367 Ager List 다중화
 * Description : table이 drop 되었는지 검사한다.
 * OID의 SCN 보다 TableHeader.CreateSCN이 크다면 (sSlotHeader->mCreateSCN 아님!)
 * 재활용된 TableSlot이다.
 ***********************************************************************/
inline idBool smcTable::isDropedTable4Ager( smcTableHeader * aHeader, smSCN aSCN )
{
    smpSlotHeader * sSlotHeader = (smpSlotHeader *)((smpSlotHeader *)aHeader - 1);

    /* 1.drop 안된 테이블 이면
     *   smpSlotHeader.mCreateSCN
     *        => (create table commitSCN) or (touchTable 시점의 systemSCN/commitSCN)
     *    smcTableHeader.mTableCreateSCN은
     *        => create table의 commitSCN */

    /* 2.drop된 테이블 commit 이면 
     *   smpSlotHeader.mCreateSCN 
     *        => | SM_SCN_DELETE_BIT
     *     smcTableHeader.mTableCreateSCN은
     *        => SM_SCN_INFINITE */
    if ( SM_SCN_IS_DELETED( sSlotHeader->mCreateSCN ) == ID_TRUE )
    {
        return ID_TRUE;
    }

    /* 3.delete ager 가 동작해서 drop table 을 aging 했다면 
     *   smpSlotHeader.mCreateSCN 
     *        => SM_SCN_FREE_ROW
     *     smcTableHeader.mTableCreateSCN은
     *        => SM_SCN_INFINITE */

    /* 4.TableSlot 재활용 중.
     *   smpSlotHeader.mCreateSCN 
     *        => SM_SCN_INFINITE
     *     smcTableHeader.mTableCreateSCN은
     *        => SM_SCN_INFINITE */

    /* 5.TableSlot 재활용 끝.
     *   smpSlotHeader.mCreateSCN 
     *        => create table의 commitSCN
     *     smcTableHeader.mTableCreateSCN은
     *        => create table의 commitSCN */
    if ( SM_SCN_IS_GT( &aHeader->mTableCreateSCN, &aSCN ) )
    {
        return ID_TRUE;
    }

    return ID_FALSE;
}


/**
 * PROJ-2462 Result Cache
 *
 * Description : Table Header의mSequenceInfo 의 mCurSequence 값을 증가 시킨다.
 * 기존에 Table Header의 mSequenceInfo는 사용하고 있지 않으며 Reuslt Cache의
 * table의 insert/update/delete 여부를 알기 위해 mCurSequence 값을 활용한다.
 */
inline void smcTable::addModifyCount( smcTableHeader * aTableHeader )
{
    idCore::acpAtomicInc64( &aTableHeader->mSequence.mCurSequence );
}

/***********************************************************************
 * Description : table의 index 개수 반환
 ***********************************************************************/
//PROJ-1362 LOB에서 인덱스 갯수 제약 풀기.
inline UInt smcTable::getIndexCount( void * aHeader )
{
    smcTableHeader * sHeader;
    UInt  sTotalIndexLen ;
    UInt  i;

    IDE_DASSERT( aHeader != NULL );

    sHeader = (smcTableHeader *)aHeader;
    for ( i = 0, sTotalIndexLen = 0 ; i < SMC_MAX_INDEX_OID_CNT ; i++ )
    {
        sTotalIndexLen += sHeader->mIndexes[i].length;
    }

    return ( sTotalIndexLen / ID_SIZEOF(smnIndexHeader) );
}

/* PROJ-1362 LOB에서 인덱스 갯수 제약 풀기. */
inline const void * smcTable::getTableIndex( void      * aHeader,
                                             const UInt  aIdx )
{
    smcTableHeader  * sHeader;
    smVCPieceHeader * sVCPieceHdr = NULL;

    UInt sOffset;
    UInt  i;
    UInt  sCurSize =0;

    IDE_DASSERT( aIdx < getIndexCount(aHeader) );

    sHeader = (smcTableHeader *) aHeader;
    sOffset = ID_SIZEOF(smnIndexHeader) * aIdx;
    for (i = 0 ; i < SMC_MAX_INDEX_OID_CNT; i++ )
    {
        if ( sHeader->mIndexes[i].fstPieceOID == SM_NULL_OID )
        {
            continue;
        }

        sCurSize = sHeader->mIndexes[i].length;

        if (sCurSize  <= sOffset)
        {
            sOffset -= sCurSize;
        }
        else
        {
            IDE_ASSERT( smmManager::getOIDPtr( 
                                    SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, 
                                    sHeader->mIndexes[i].fstPieceOID,
                                    (void **)&sVCPieceHdr )
                        == IDE_SUCCESS );

            break;
        }
    }

    /* fix BUG-27232 [CodeSonar] Null Pointer Dereference */
    IDE_ASSERT( i != SMC_MAX_INDEX_OID_CNT );

    return ( (UChar *) sVCPieceHdr + ID_SIZEOF(smVCPieceHeader) + sOffset );
}

/***********************************************************************
 * Description : table oid로부터 table header를 얻는다.
 * table의 OID는 slot의 header를 가르킨다.
 ***********************************************************************/
inline IDE_RC smcTable::getTableHeaderFromOID( smOID     aOID, 
                                               void   ** aHeader )
{
    return smmManager::getOIDPtr( SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, 
                                  aOID + SMP_SLOT_HEADER_SIZE, 
                                  (void **)aHeader );
}

/* PROJ-1362 QP-Large Record & Internal LOB support
   table의 컬럼갯수와 인덱스 개수 제한 풀기. */
inline const smiColumn * smcTable::getColumn( const void * aTableHeader,
                                              const UInt   aIndex )
{
    smOID   sDummyOID;

    return getColumnAndOID( aTableHeader,
                            aIndex,
                            &sDummyOID );
}

#endif /* _O_SMC_TABLE_H_ */
