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
 * $Id: smiTable.cpp 90302 2021-03-24 01:21:41Z emlee $
 **********************************************************************/
/**************************************************************
 * FILE DESCRIPTION : smiTable.cpp                            *
 * -----------------------------------------------------------*
 *  PROJ-2201 Innovation in sorting and hashing(temp)
 *  위 Project를 통해 DiskTemporary Table이 smiTempTable로 분리되었다.
 *  Lock/Latch/MVCC 관리등을 TempTable은 사용하지 않기 때문에, 이를 분리하여
 *  성능 및 사용성 등을 높인다.
 **************************************************************/
#include <idl.h>
#include <ide.h>

#include <smErrorCode.h>

#include <sma.h>
#include <smc.h>
#include <smm.h>
#include <svm.h>
#include <sml.h>
#include <smn.h>
#include <sdn.h>
#include <smp.h>
#include <sct.h>
#include <smr.h>
#include <smiTable.h>
#include <smiStatement.h>
#include <smiTrans.h>
#include <smDef.h>
#include <smcTableBackupFile.h>
#include <smiMisc.h>
#include <smiMain.h>
#include <smiTableBackup.h>
#include <smiVolTableBackup.h>
#include <smlAPI.h>
#include <sctTableSpaceMgr.h>
#include <svpFreePageList.h>
#include <sdnnModule.h>

// smiTable에서 사용하는 callback함수 list.

typedef IDE_RC (*smiCreateTableFunc)(idvSQL               * aStatistics,
                                     void*                  aTrans,
                                     scSpaceID              aSpaceID,
                                     const smiColumnList  * aColumnList,
                                     UInt                   aColumnSize,
                                     const void           * aInfo,
                                     UInt                   aInfoSize,
                                     const smiValue*        aNullRow,
                                     UInt                   aFlag,
                                     ULong                  aMaxRow,
                                     smiSegAttr             aSegmentAttr,
                                     smiSegStorageAttr      aSegmentStoAttr,
                                     UInt                   aParallelDegree,
                                     const void**           aTable);

typedef  IDE_RC (*smiDropTableFunc)(
                     void              * aTrans,
                     const void        * aTable,
                     sctTBSLockValidOpt  aTBSLvOpt,
                     UInt                aFlag);


typedef IDE_RC  (*smiCreateIndexFunc)(idvSQL              *aStatistics,
                                      smiStatement*        aStatement,
                                      scSpaceID            aTableSpaceID,
                                      const void*          aTable,
                                      SChar *              aName,
                                      UInt                 aId,
                                      UInt                 aType,
                                      const smiColumnList* aKeyColumns,
                                      UInt                 aFlag,
                                      UInt                 aParallelDegree,
                                      UInt                 aBuildFlag,
                                      smiSegAttr         * aSegmentAttr,
                                      smiSegStorageAttr  * aSegmentStoAttr,
                                      ULong                aDirectKeyMaxSize,
                                      const void**         aIndex);

// create table callback functions.
static smiCreateTableFunc smiCreateTableFuncs[(SMI_TABLE_TYPE_MASK >> 12)+1] =
{
    //SMI_TABLE_META
    smcTable::createTable,
    //SMI_TABLE_TEMP_LEGACY
    NULL,
    //SMI_TABLE_MEMORY
    smcTable::createTable,
    //SMI_TABLE_DISK
    smcTable::createTable,
    //SMI_TABLE_FIXED
    smcTable::createTable,
    //SMI_TABLE_VOLATILE
    smcTable::createTable,
    //SMI_TABLE_REMOTE
    NULL
};

// drop table callback functions.
static smiDropTableFunc smiDropTableFuncs[(SMI_TABLE_TYPE_MASK >> 12)+1] =
{
    //SMI_TABLE_META
    smcTable::dropTable,
    //SMI_TABLE_TEMP_LEGACY
    NULL,
    //SMI_TABLE_MEMORY
    smcTable::dropTable,
    //SMI_TABLE_DISK
    smcTable::dropTable,
    //SMI_TABLE_FIXED
    smcTable::dropTable,
    //SMI_TABLE_VOLATILE
    smcTable::dropTable,
    //SMI_TABLE_REMOTE
    NULL
};

// create index  callback functions.
static smiCreateIndexFunc smiCreateIndexFuncs[(SMI_TABLE_TYPE_MASK >> 12)+1] =
{
    //SMI_TABLE_META
    (smiCreateIndexFunc)smiTable::createMemIndex,
    //SMI_TABLE_TEMP_LEGACY
    NULL,
    //SMI_TABLE_MEMORY
    (smiCreateIndexFunc)smiTable::createMemIndex,
    //SMI_TABLE_DISK
    (smiCreateIndexFunc)smiTable::createDiskIndex,
    //SMI_TABLE_FIXED
    (smiCreateIndexFunc)smiTable::createMemIndex,
    //SMI_TABLE_VOLATILE
    (smiCreateIndexFunc)smiTable::createVolIndex,
    //SMI_TABLE_REMOTE
    NULL
};


/*********************************************************
  function description: createTable

    FOR A4 :
     -  aTableSpaceID인자가 추가됩
     -  disk temp table을 create할때,table헤더를 SYS_TEMP_TABLE이란
        별도의 catalog table의 fixed slot를 할당받는다.
        loggin최소화 , no-lock을 위하여 smcTable의 createTempTable
        을 호출한다.

     ___________________________________          _________________
    | catalog table for disk temp table |         |smcTableHeader |
    | (SYS_TEMP_TABLE)                  |         -----------------
    -------------------------------------          ^
             |                 |                   |
             |                 ㅁ                  SYS_TEMP_TABLE의 fixed slot.
             o                 |
             |                 ㅁ var
             o fixed              page list
               page list entry    entry

***********************************************************/
IDE_RC smiTable::createTable( idvSQL*               aStatistics,
                              scSpaceID             aTableSpaceID,
                              UInt                  /*aTableID*/,
                              smiStatement*         aStatement,
                              const smiColumnList*  aColumns,
                              UInt                  aColumnSize,
                              const void*           aInfo,
                              UInt                  aInfoSize,
                              const smiValue*       aNullRow,
                              UInt                  aFlag,
                              ULong                 aMaxRow,
                              smiSegAttr            aSegmentAttr,
                              smiSegStorageAttr     aSegmentStoAttr,
                              UInt                  aParallelDegree,
                              const void**          aTable )
{


    const smiColumn * sColumn;
    smiColumnList   * sColumnList;
    scSpaceID         sLobTBSID;
    scSpaceID         sLockedTBSID;
    UInt              sTableTypeFlag;

    sTableTypeFlag = aFlag & SMI_TABLE_TYPE_MASK;

    switch( sctTableSpaceMgr::getTBSLocation( aTableSpaceID ) )
    {
        case SMI_TBS_MEMORY:
            // TableSpace가 메모리 테이블스페이스 라면
            // aFlag의 table type이 disk가 아님을 체크
            IDE_DASSERT( sTableTypeFlag != SMI_TABLE_DISK );
            break;
        case SMI_TBS_VOLATILE:
            IDE_DASSERT( sTableTypeFlag == SMI_TABLE_VOLATILE );
            break;
        case SMI_TBS_DISK:
            // aFlag의 table type이 meta, 혹은 memory가 아님을 체크
            IDE_DASSERT( (sTableTypeFlag != SMI_TABLE_META) &&
                         (sTableTypeFlag != SMI_TABLE_MEMORY) );
            break;
        default:
            IDE_ASSERT(0);
            break;
    }
 

    IDE_TEST( aStatement->prepareDDL((smiTrans*)aStatement->mTrans)
              != IDE_SUCCESS );

    // PRJ-1548 User Memory Tablespace
    // 트랜잭션이 완료될때(commit or abort) TableSpace 잠금을 해제한다.
    // 모든 DML과 DDL시에는 QP Validation Code에서 잠금을 요청하고
    // 실제 실행시에도 잠금을 요청하도록 되어 있지만, 예외적으로
    // CREATE TABLE 시에는 그렇지 않으므로 본 함수에서 Table의 TableSpace를
    // IX 획득한다.

    // fix BUG-17121
    // 또한, 다른 DDL과는 다르게 Table을 LockValidate할 필요없이
    // Index,lob 관련 TableSpace에 IX를 획득한다.

    // -------------- TBS Node (IX) ------------------ //
    IDE_TEST( sctTableSpaceMgr::lockTBSNodeByID(
                                    (void*)aStatement->mTrans->mTrans,
                                    aTableSpaceID,
                                    ID_TRUE,    /* intent */
                                    ID_TRUE,    /* exclusive */
                                    ID_ULONG_MAX,
                                    SCT_VAL_DDL_DML,
                                    NULL,
                                    NULL )
              != IDE_SUCCESS );

    if ( sTableTypeFlag == SMI_TABLE_DISK )
    {
        // 또한, CREATE Disk TABLE 수행시 LOB Column이 포함되어 있다면
        // 관련 TableSpace도 IX를 획득해야 한다.
        // 함수내에서 획득된다.

        sLockedTBSID = aTableSpaceID;
        sColumnList = (smiColumnList *)aColumns;

        while ( sColumnList != NULL )
        {
            sColumn = sColumnList->column;

            // PROJ-1583 large geometry
            IDE_ASSERT( sColumn->value == NULL );

            if ( ( sColumn->flag & SMI_COLUMN_TYPE_MASK )
                   == SMI_COLUMN_TYPE_LOB )
            {
                // LOB Column인 경우
                sLobTBSID = sColumn->colSpace;

                // 일단, 연속적으로 중복된 경우에 대해서 잠금을 회피하도록
                // 간단히 처리하기한다. (성능이슈)

                if ( sLockedTBSID != sLobTBSID )
                {
                    IDE_TEST( sctTableSpaceMgr::lockTBSNodeByID(
                                (void*)aStatement->mTrans->mTrans,
                                sLobTBSID,
                                ID_TRUE, /* intent */
                                ID_TRUE, /* exclusive */
                                ID_ULONG_MAX, /* wait lock timeout */
                                SCT_VAL_DDL_DML,
                                NULL,
                                NULL ) != IDE_SUCCESS );

                    sLockedTBSID = sLobTBSID;
                }
            }
            else
            {
                // 그외 Column Type
            }

            sColumnList = sColumnList->next;
        }
    }
    else
    {
        // 그외 Table Type은 LOB Column을 고려하지 않는다.
    }

    //BUGBUG smiDef.h에서 SMI_TABLE_TYPE_MASK가 변경되면
    // 이코드도 변경되어야함.
    sTableTypeFlag  = sTableTypeFlag >> SMI_SHIFT_TO_TABLE_TYPE;

    IDE_TEST(smiCreateTableFuncs[sTableTypeFlag](
                        aStatistics,
                        (smxTrans*)aStatement->mTrans->mTrans,
                        aTableSpaceID,
                        aColumns,
                        aColumnSize,
                        aInfo,
                        aInfoSize,
                        aNullRow,
                        aFlag,
                        aMaxRow,
                        aSegmentAttr,
                        aSegmentStoAttr,
                        aParallelDegree,
                        aTable )
             != IDE_SUCCESS);

    *aTable = (const void*)((const UChar*)*aTable-SMP_SLOT_HEADER_SIZE);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/*
 * META/MEMORY/DISK/TEMP/FIXED/VOLATILE TABLE을 Drop 한다.
 *
 * disk temp table을 drop할때 no-logging을
 * 하기 위하여, smcTable::dropTempTable을 부른다.
 *
 * [ 인자 ]
 * [IN] aStatement - Statement
 * [IN] aTable     - Drop 할 Table Handle
 * [IN] aTBSLvType - Table 관련된 TBS(table/index/lob)들에 대한
 *                   Lock Validation 타입
 */
IDE_RC smiTable::dropTable( smiStatement       * aStatement,
                            const  void        * aTable,
                            smiTBSLockValidType  aTBSLvType )
{
    UInt  sTableTypeFlag;
    const smcTableHeader* sTableHeader;

    sTableHeader = (smcTableHeader*)((UChar*)aTable+SMP_SLOT_HEADER_SIZE);

    sTableTypeFlag = SMI_GET_TABLE_TYPE( sTableHeader );

    IDU_FIT_POINT( "2.BUG-42411@smiTable::dropTable" );
    IDE_TEST( aStatement->prepareDDL((smiTrans*)aStatement->mTrans)
              != IDE_SUCCESS );

    //BUGBUG smiDef.h에서 SMI_TABLE_TYPE_MASK가 변경되면
    // 이코드도 변경되어야함.
    //sTableTypeFlag  = sTableTypeFlag >> 12;
    // table type flag를 얻기위해 12 bit shift right함
    sTableTypeFlag  = sTableTypeFlag >> SMI_SHIFT_TO_TABLE_TYPE;

    // PRJ-1548 User Memory TableSpace 개념도입
    // Validate Table 에서 다음과 같이 Lock을 획득한다.
    // [1] Table의 TableSpace에 대해 IX
    // [2] Table에 대해 X
    // [3] Table의 Index, Lob Column TableSpace에 대해 IX
    // Table의 상위는 Table의 TableSpace이며 그에 대해 IX를
    // 획득한다.

    IDE_TEST( smiDropTableFuncs[sTableTypeFlag](
                          (smxTrans*)aStatement->mTrans->mTrans,
                          ((UChar*)aTable+SMP_SLOT_HEADER_SIZE),
                          sctTableSpaceMgr::getTBSLvType2Opt( aTBSLvType ),
                          SMC_LOCK_TABLE ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}
/*********************************************************
 * Description : table info 변경하는 함수
 * Implementation :
 *    - Input
 *      aStatistics : statistics
 *      aTable      : table header
 *      aColumns    : column list
 *      aColumnSize : column size
 *      aInfo       : 새로운 table info
 *      aInfoSize   : info size
 *      aFlag       : 새로운 table flag
 *                    ( 또는 table_flag 변경이 없음을 알리는 정보 )
 *      aTBSLvType  : tablespace validation option
 *    - Output
***********************************************************/

IDE_RC smiTable::modifyTableInfo( smiStatement*        aStatement,
                                  const void*          aTable,
                                  const smiColumnList* aColumns,
                                  UInt                 aColumnSize,
                                  const void*          aInfo,
                                  UInt                 aInfoSize,
                                  UInt                 aFlag,
                                  smiTBSLockValidType  aTBSLvType )
{
    IDE_TEST( modifyTableInfo( aStatement,
                               aTable,
                               aColumns,
                               aColumnSize,
                               aInfo,
                               aInfoSize,
                               aFlag,
                               aTBSLvType,
                               0,  // max row
                               0,  // parallel degree 설정안됨을 의미
                               ID_TRUE ) // aIsInitRowTemplate
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smiTable::modifyTableInfo( smiStatement*        aStatement,
                                  const void*          aTable,
                                  const smiColumnList* aColumns,
                                  UInt                 aColumnSize,
                                  const void*          aInfo,
                                  UInt                 aInfoSize,
                                  UInt                 aFlag,
                                  smiTBSLockValidType  aTBSLvType,
                                  ULong                aMaxRow,
                                  UInt                 aParallelDegree,
                                  idBool               aIsInitRowTemplate )
{
    IDE_TEST( aStatement->prepareDDL((smiTrans*)aStatement->mTrans)
              != IDE_SUCCESS );

    // PRJ-1548 User Memory TableSpace 개념도입
    // Validate Table 에서 다음과 같이 Lock을 획득한다.
    // [1] Table의 TableSpace에 대해 IX
    // [2] Table에 대해 X
    // [3] Table의 Index, Lob Column TableSpace에 대해 IX
    // Table의 상위는 Table의 TableSpace이며 그에 대해 IX를
    // 획득한다.
    IDE_TEST( smcTable::modifyTableInfo(
                 (smxTrans*)aStatement->mTrans->mTrans,
                 (smcTableHeader*)((UChar*)aTable+SMP_SLOT_HEADER_SIZE),
                 aColumns,
                 aColumnSize,
                 aInfo,
                 aInfoSize,
                 aFlag,
                 aMaxRow,
                 aParallelDegree,
                 sctTableSpaceMgr::getTBSLvType2Opt( aTBSLvType ),
                 aIsInitRowTemplate )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/*********************************************************
  function description: createIndex
    FOR A4 :
    - disk temporary table의 인덱스 헤더는
    disk temporary table의 catalog 테이블인
    SYS_TEMP_TABLE에서 variable slot할당을 받고,
    no logging을 위하여 smcTable::createTempIndex을 부른다.
    인덱스 헤더를 할당받은후 snap shot은 다음과 같다.
     ___________________________________          _________  ______
    | catalog table for disk temp table |       - |smcTableHeader |
    | (SYS_TEMP_TABLE)                  |       |  -----------------
    -------------------------------------       |   ^
             |                 |                |   |
             |                 ㅁ               |   SYS_TEMP_TABLE의 fixed slot.
             o                 |                |
             |                 ㅁ var           |
             o fixed              page list     |_ ----------------
               page list entry    entry            |smnIndexHeader |
                                                   ----------------
                                             SYS_TEMP_TABLE의 variable slot.

   새롭게 할당된 인덱스 헤더의
   smnInsertFunc,smnDeleteFunc, smnClusterFunc는 인덱스 초기화과정에서
   해당 index모듈의 create함수가 불리게되어  assign된다.

   % memory table index에서는 aKeyColumns가 row에서
    인덱스 key column offset정보를 담고,
    disk table index에서는 aKeyColumns가 key slot 에서
    인덱스 key column offset정보를 담는다.

***********************************************************/
IDE_RC smiTable::createIndex(idvSQL*              aStatistics,
                             smiStatement*        aStatement,
                             scSpaceID            aTableSpaceID,
                             const void*          aTable,
                             SChar*               aName,
                             UInt                 aId,
                             UInt                 aType,
                             const smiColumnList* aKeyColumns,
                             UInt                 aFlag,
                             UInt                 aParallelDegree,
                             UInt                 aBuildFlag,
                             smiSegAttr           aSegAttr,
                             smiSegStorageAttr    aSegmentStoAttr,
                             ULong                aDirectKeyMaxSize,
                             const void**         aIndex)
{
    UInt sTableTypeFlag;
    UInt sState = 0;

    smcTableHeader * sTable = (smcTableHeader*)((SChar*)aTable + SMP_SLOT_HEADER_SIZE);

    sTableTypeFlag = SMI_GET_TABLE_TYPE( sTable );

    // PRJ-1548 User Memory Tablespace
    // CREATE INDEX 관련 TableSpace에 대하여 별도로 IX를 획득한다.
    // -------------- TBS Node (IX) ------------------ //
    IDE_TEST( sctTableSpaceMgr::lockTBSNodeByID(
                      (void*)aStatement->mTrans->mTrans,
                      aTableSpaceID,
                      ID_TRUE,    /* intent */
                      ID_TRUE,    /* exclusive */
                      ID_ULONG_MAX, /* wait lock timeout */
                      SCT_VAL_DDL_DML,
                      NULL,
                      NULL ) != IDE_SUCCESS );

    // BUGBUG smiDef.h에서 SMI_TABLE_TYPE_MASK가 변경되면
    // 이코드도 변경되어야함.
    sTableTypeFlag  = sTableTypeFlag >> SMI_SHIFT_TO_TABLE_TYPE;

    // PRJ-1548 User Memory TableSpace 개념도입
    // Validate Table 에서 다음과 같이 Lock을 획득한다.
    // [1] Table의 TableSpace에 대해 IX
    // [2] Table에 대해 X
    // [3] 이미 생성되어 있는 Table의 Index, Lob Column TableSpace에 대해 IX
    // Table의 상위는 Table의 TableSpace이며 그에 대해 IX를
    // 획득한다.

    /* BUG-24025: [SC] Index Create시 Statement View SCN이 유지되어 Index Build가
     * 종료될때까지 Ager가 동작하지 못함.
     *
     * Index Build전에 Transaction의 모든 Statement의 ViewSCN을 Clear하고 Create
     * Index후에 다시 새로운 ViewSCN을 따준다.
     * */
    /* BUG-31993 [sm_interface] The server does not reset Iterator ViewSCN 
     * after building index for Temp Table
     * TempTable일 경우에는 내부에 이미 열어버린 커서 및 Iterator가 존재하기
     * 때문에, SCN을 변경하면 안된다. */

    /* TempTable Type은 이제 없다. */
    IDE_ERROR( sTableTypeFlag != 
               ( SMI_TABLE_TEMP_LEGACY >> SMI_SHIFT_TO_TABLE_TYPE ) );

    IDE_TEST( smiStatement::initViewSCNOfAllStmt( aStatement ) 
              != IDE_SUCCESS );
    sState = 1;

    IDE_TEST(smiCreateIndexFuncs[sTableTypeFlag](aStatistics,
                                                 aStatement,
                                                 aTableSpaceID,
                                                 sTable,
                                                 aName,
                                                 aId,
                                                 aType,
                                                 aKeyColumns,
                                                 aFlag,
                                                 aParallelDegree,
                                                 aBuildFlag,
                                                 &aSegAttr,
                                                 &aSegmentStoAttr,
                                                 aDirectKeyMaxSize,
                                                 aIndex)
             != IDE_SUCCESS);

    IDE_DASSERT( sState == 1 );

    sState = 0;
    IDE_TEST( smiStatement::setViewSCNOfAllStmt( aStatement ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_DASSERT( sTableTypeFlag 
                     != ( SMI_TABLE_TEMP_LEGACY >> SMI_SHIFT_TO_TABLE_TYPE ) );
        (void)smiStatement::setViewSCNOfAllStmt( aStatement );
    }

    return IDE_FAILURE;
}

/*********************************************************
  function description: createMemIndex
  memory table의 index를 생성한다.
  -added for A4
***********************************************************/
IDE_RC smiTable::createMemIndex(idvSQL              *aStatistics,
                                smiStatement*        aStatement,
                                scSpaceID            aTableSpaceID,
                                smcTableHeader*      aTable,
                                SChar *              aName,
                                UInt                 aId,
                                UInt                 aType,
                                const smiColumnList* aKeyColumns,
                                UInt                 aFlag,
                                UInt                 aParallelDegree,
                                UInt                 /*aBuildFlag*/,
                                smiSegAttr         * aSegmentAttr,
                                smiSegStorageAttr  * aSegmentStoAttr,
                                ULong                aDirectKeyMaxSize,
                                const void**         aIndex)
{
    smxTrans* sTrans;

    // index type에 대한 validation
    IDE_ASSERT( aType != SMI_BUILTIN_SEQUENTIAL_INDEXTYPE_ID );
    IDE_TEST( aStatement->prepareDDL((smiTrans*)aStatement->mTrans)
              != IDE_SUCCESS);

    sTrans = (smxTrans*)aStatement->mTrans->mTrans;

    aFlag &= ~SMI_INDEX_USE_MASK;

    // fix BUG-10518
    if( (aTable->mFlag & SMI_TABLE_DISABLE_ALL_INDEX_MASK)
        == SMI_TABLE_ENABLE_ALL_INDEX )
    {
        aFlag &= ~(SMI_INDEX_USE_MASK);
        aFlag |= SMI_INDEX_USE_ENABLE;
    }
    else
    {
        aFlag &= ~(SMI_INDEX_USE_MASK);
        aFlag |= SMI_INDEX_USE_DISABLE;
    }

    /* BUG-32655 [sm-mem-index] The MMDB Ager must not ignore the failure 
     * of index aging. 
     * IndexBuilding 이전에 Aging할 Job이 있을 경우, IndexBuilding 시점에는
     * 자기 시점에서 유효한 Row만 읽어서 Build하기 때문에 그 이전 Aging
     * Job에서 등록된 Row들이 Index에는 없게 된다. 이것이 일관성에
     * 큰 문제를 줄 수 있으므로, IndexBuilding 전에 미리 Aging한다.
     * 
     * 이때 Table에 XLock을 잡았기 때문에 수행할 수 있는 것*/
    IDE_TEST(smaLogicalAger::doInstantAgingWithTable( aTable->mSelfOID )
             != IDE_SUCCESS);

    /* Index의 Header를 생성한다. */
    IDE_TEST( smcTable::createIndex(aStatistics,
                                    sTrans,
                                    aTableSpaceID,
                                    aTable,
                                    aName,
                                    aId,
                                    aType,
                                    aKeyColumns,
                                    aFlag,
                                    SMI_INDEX_BUILD_DEFAULT,
                                    aSegmentAttr,
                                    aSegmentStoAttr,
                                    aDirectKeyMaxSize,
                                    (void**)aIndex )
              != IDE_SUCCESS );

    if( (aFlag & SMI_INDEX_USE_MASK) == SMI_INDEX_USE_ENABLE )
    {
        // 실제 Index를 생성한다.
        IDE_TEST( smnManager::buildIndex(aStatistics,
                                         sTrans,
                                         aTable,
                                         (smnIndexHeader*)*aIndex,
                                         ID_TRUE,
                                         ID_TRUE,
                                         aParallelDegree,
                                         0,      // aBuildFlag   (for Disk only )
                                         0 )     // aTotalRecCnt (for Disk only )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************
  function description: createVolIndex
  volatile table의 index를 생성한다.
  -added for A4
***********************************************************/
IDE_RC smiTable::createVolIndex(idvSQL              *aStatistics,
                                smiStatement*        aStatement,
                                scSpaceID            aTableSpaceID,
                                smcTableHeader*      aTable,
                                SChar *              aName,
                                UInt                 aId,
                                UInt                 aType,
                                const smiColumnList* aKeyColumns,
                                UInt                 aFlag,
                                UInt                 aParallelDegree,
                                UInt                 /*aBuildFlag*/,
                                smiSegAttr         * aSegmentAttr,
                                smiSegStorageAttr  * aSegmentStoAttr,
                                ULong                /*aDirectKeyMaxSize*/,
                                const void**         aIndex)
{
    smxTrans* sTrans;

    // index type에 대한 validation
    IDE_ASSERT( aType != SMI_BUILTIN_SEQUENTIAL_INDEXTYPE_ID );
    IDE_TEST( aStatement->prepareDDL((smiTrans*)aStatement->mTrans)
              != IDE_SUCCESS);

    sTrans = (smxTrans*)aStatement->mTrans->mTrans;

    aFlag &= ~SMI_INDEX_USE_MASK;

    // fix BUG-10518
    if( (aTable->mFlag & SMI_TABLE_DISABLE_ALL_INDEX_MASK)
        == SMI_TABLE_ENABLE_ALL_INDEX )
    {
        aFlag &= ~(SMI_INDEX_USE_MASK);
        aFlag |= SMI_INDEX_USE_ENABLE;
    }
    else
    {
        aFlag &= ~(SMI_INDEX_USE_MASK);
        aFlag |= SMI_INDEX_USE_DISABLE;
    }

    /* BUG-32655 [sm-mem-index] The MMDB Ager must not ignore the failure 
     * of index aging. 
     * IndexBuilding 이전에 Aging할 Job이 있을 경우, IndexBuilding 시점에는
     * 자기 시점에서 유효한 Row만 읽어서 Build하기 때문에 그 이전 Aging
     * Job에서 등록된 Row들이 Index에는 없게 된다. 이것이 일관성에
     * 큰 문제를 줄 수 있으므로, IndexBuilding 전에 미리 Aging한다.
     * 
     * 이때 Table에 XLock을 잡았기 때문에 수행할 수 있는 것*/
    IDE_TEST(smaLogicalAger::doInstantAgingWithTable( aTable->mSelfOID )
             != IDE_SUCCESS);

    /* Index의 Header를 생성한다. */
    IDE_TEST( smcTable::createIndex(aStatistics,
                                    sTrans,
                                    aTableSpaceID,
                                    aTable,
                                    aName,
                                    aId,
                                    aType,
                                    aKeyColumns,
                                    aFlag,
                                    SMI_INDEX_BUILD_DEFAULT,
                                    aSegmentAttr,
                                    aSegmentStoAttr,
                                    0, /* aDirectKeyMaxSize */
                                    (void**)aIndex )
              != IDE_SUCCESS );

    if( (aFlag & SMI_INDEX_USE_MASK) == SMI_INDEX_USE_ENABLE )
    {
        // 실제 Index를 생성한다.
        IDE_TEST( smnManager::buildIndex(aStatistics,
                                         sTrans,
                                         aTable,
                                         (smnIndexHeader*)*aIndex,
                                         ID_TRUE,
                                         ID_TRUE,
                                         aParallelDegree,
                                         0,      // aBuildFlag   (for Disk only )
                                         0 )     // aTotalRecCnt (for Disk only )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************
  function description: createDiskIndex
  memory table의 index를 생성한다.
  -added for A4
***********************************************************/
IDE_RC smiTable::createDiskIndex(idvSQL               * aStatistics,
                                 smiStatement         * aStatement,
                                 scSpaceID              aTableSpaceID,
                                 smcTableHeader       * aTable,
                                 SChar                * aName,
                                 UInt                   aId,
                                 UInt                   aType,
                                 const smiColumnList  * aKeyColumns,
                                 UInt                   aFlag,
                                 UInt                   aParallelDegree,
                                 UInt                   aBuildFlag,
                                 smiSegAttr           * aSegmentAttr,
                                 smiSegStorageAttr    * aSegmentStoAttr,
                                 ULong                  /*aDirectKeyMaxSize*/,
                                 const void          ** aIndex)
{

    smxTrans*       sTrans;
    ULong           sTotalRecCnt = 0;
    UInt            sBUBuildThreshold;

    // index type에 대한 validation
    IDE_ASSERT( (aType == SMI_BUILTIN_B_TREE_INDEXTYPE_ID) ||
                (aType == SMI_ADDITIONAL_RTREE_INDEXTYPE_ID) );

    /* BUG-33803 Table이 ALL INDEX ENABLE 상태일 경우 index도 ENALBE 상태로
     * 생성하고, ALL INDEX DISABLE 상태일 경우 index도 DISABLE 상태로 생성함. */
    if( (aTable->mFlag & SMI_TABLE_DISABLE_ALL_INDEX_MASK)
        == SMI_TABLE_ENABLE_ALL_INDEX )
    {
        aFlag &= ~(SMI_INDEX_USE_MASK);
        aFlag |= SMI_INDEX_USE_ENABLE;
    }
    else
    {
        aFlag &= ~(SMI_INDEX_USE_MASK);
        aFlag |= SMI_INDEX_USE_DISABLE;
    }

    if( (aBuildFlag & SMI_INDEX_BUILD_LOGGING_MASK)
        == SMI_INDEX_BUILD_DEFAULT )
    {
        aBuildFlag |= SMI_INDEX_BUILD_LOGGING;
    }

    if( (aBuildFlag & SMI_INDEX_BUILD_FORCE_MASK)
        == SMI_INDEX_BUILD_DEFAULT )
    {
        aBuildFlag |= SMI_INDEX_BUILD_NOFORCE;
    }

    IDE_TEST( smcTable::getRecordCount( aTable, &sTotalRecCnt )
              != IDE_SUCCESS );
    sBUBuildThreshold = smuProperty::getIndexBuildBottomUpThreshold();

    // PROJ-1502 PARTITIONED DISK TABLE
    // 커밋되지 않은 ROW(SMI_INDEX_BUILD_UNCOMMITTED_ROW_ENABLE)에 대해서
    // 인덱스 삽입을 유도하기 위해서는 __DISK_INDEX_BOTTOM_UP_BUILD_THRESHOLD 
    // 프로퍼티를 무시해야만 한다.
    if ( (aBuildFlag & SMI_INDEX_BUILD_UNCOMMITTED_ROW_MASK) ==
         SMI_INDEX_BUILD_UNCOMMITTED_ROW_ENABLE )
    {
        sTotalRecCnt = ID_ULONG_MAX;
    }

    // To fix BUG-17994 : Top-down으로 index build할 경우
    //                    무조건 LOGGING & NOFORCE option
    if( sTotalRecCnt < sBUBuildThreshold )
    {
        aBuildFlag &= ~(SMI_INDEX_BUILD_LOGGING_MASK);
        aBuildFlag |= SMI_INDEX_BUILD_LOGGING;
        aBuildFlag &= ~(SMI_INDEX_BUILD_FORCE_MASK);
        aBuildFlag |= SMI_INDEX_BUILD_NOFORCE;
        aBuildFlag |= SMI_INDEX_BUILD_TOPDOWN;
    }


    /* DDL 준비 */
    IDE_TEST( aStatement->prepareDDL((smiTrans*)aStatement->mTrans)
              != IDE_SUCCESS );

    sTrans = (smxTrans*)aStatement->mTrans->mTrans;

    /* Index의 Header를 생성한다. */
    IDE_TEST( smcTable::createIndex(aStatistics,
                                    sTrans,
                                    aTableSpaceID,
                                    aTable,
                                    aName,
                                    aId,
                                    aType,
                                    aKeyColumns,
                                    aFlag,
                                    aBuildFlag,
                                    aSegmentAttr,
                                    aSegmentStoAttr,
                                    0, /* aDirecKeyMaxSize */
                                    (void**)aIndex )
                  != IDE_SUCCESS );

    if( (aFlag & SMI_INDEX_USE_MASK) == SMI_INDEX_USE_ENABLE )
    {
        // 실제 Index를 생성한다.
        IDE_TEST( smnManager::buildIndex(aStatistics,
                                         sTrans,
                                         aTable,
                                         (smnIndexHeader*)*aIndex,
                                         ID_TRUE,  // isNeedValidation
                                         ID_TRUE,  // persistent
                                         aParallelDegree,
                                         aBuildFlag,
                                         sTotalRecCnt )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * MEMORY/DISK TABLE의 Index를  Drop 한다.
 *
 * [ 예외 ]
 *
 * disk temp table의 대한 index drop는 반드시
 * drop table을 통해서 불려야 한다.
 *
 * [ 인자 ]
 * [IN] aStatement - Statement
 * [IN] aTable     - Drop 할 Table Handle
 * [IN] aIndex     - Drop 할 Index Handle
 * [IN] aTBSLvType - Table 관련된 TBS(table/index/lob)들에 대한
 *                   Lock Validation 타입
 */
IDE_RC smiTable::dropIndex( smiStatement      * aStatement,
                            const void        * aTable,
                            const void        * aIndex,
                            smiTBSLockValidType aTBSLvType )
{

    smcTableHeader* sTable;
    smxTrans*       sTrans;

    sTable = (smcTableHeader*)((UChar*)aTable+SMP_SLOT_HEADER_SIZE);

    IDE_TEST( aStatement->prepareDDL((smiTrans*)aStatement->mTrans)
              != IDE_SUCCESS );

    sTrans = (smxTrans*)aStatement->mTrans->mTrans;

    // PRJ-1548 User Memory TableSpace 개념도입
    // Validate Table 에서 다음과 같이 Lock을 획득한다.
    // [1] Table의 TableSpace에 대해 IX
    // [2] Table에 대해 X
    // [3] Table의 Index, Lob Column TableSpace에 대해 IX
    // Table의 상위는 Table의 TableSpace이며 그에 대해 IX를
    // 획득한다.
    IDE_TEST( smcTable::dropIndex( sTrans,
                                   sTable,
                                   (void*)aIndex,
                                   sctTableSpaceMgr::getTBSLvType2Opt( aTBSLvType ) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC smiTable::alterIndexInfo(smiStatement* aStatement,
                                const void*   aTable,
                                const void*   aIndex,
                                const UInt    aFlag)
{
    smcTableHeader* sTable;
    smxTrans*       sTrans;

    IDE_TEST( aStatement->prepareDDL((smiTrans*)aStatement->mTrans) != IDE_SUCCESS );

    sTable = (smcTableHeader*)((UChar*)aTable+SMP_SLOT_HEADER_SIZE);
    sTrans = (smxTrans*)aStatement->mTrans->mTrans;

    // PRJ-1548 User Memory TableSpace 개념도입
    // Validate Table 에서 다음과 같이 Lock을 획득한다.
    // [1] Table의 TableSpace에 대해 IX
    // [2] Table에 대해 X
    // [3] Table의 Index, Lob Column TableSpace에 대해 IX
    // Table의 상위는 Table의 TableSpace이며 그에 대해 IX를
    // 획득한다.
    IDE_TEST( smcTable::alterIndexInfo( sTrans,
                                        sTable,
                                        (smnIndexHeader*)aIndex,
                                        aFlag)
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

UInt smiTable::getIndexInfo(const void* aIndex)
{
    return ((smnIndexHeader*)aIndex)->mFlag;
}

smiSegAttr smiTable::getIndexSegAttr(const void* aIndex)
{
    return ((smnIndexHeader*)aIndex)->mSegAttr;
}

smiSegStorageAttr smiTable::getIndexSegStoAttr(const void* aIndex)
{
    return ((smnIndexHeader*)aIndex)->mSegStorageAttr;
}

/* PROJ-2433 */
UInt smiTable::getIndexMaxKeySize( const void * aIndex )
{
    return ((smnIndexHeader *)aIndex)->mMaxKeySize;
}
void smiTable::setIndexInfo( void * aIndex,
                             UInt   aFlag )
{
    ((smnIndexHeader *)aIndex)->mFlag = aFlag;
}
void smiTable::setIndexMaxKeySize( void * aIndex,
                                   UInt   aMaxKeySize )
{
    ((smnIndexHeader *)aIndex)->mMaxKeySize = aMaxKeySize;
}
/* PROJ-2433 end */

/* ------------------------------------------------
 * for unpin and PR-4360 ALTER TABLE
 * aFlag : ID_FALSE - unpin
 * aFlag : ID_TRUE  - altertable (add/drop column, compact)
 * ----------------------------------------------*/
IDE_RC smiTable::backupMemTable(smiStatement * aStatement,
                                const void   * aTable,
                                SChar        * aBackupFileName,
                                idvSQL       * aStatistics)
{
    smiTableBackup   sTableBackup;
    SInt             sState = 0;
    smcTableHeader * sTable;
    smmTBSNode     * sTBSNode;

    IDE_TEST(sTableBackup.initialize(aTable, aBackupFileName) != IDE_SUCCESS);
    sState = 1;

    IDE_TEST(sTableBackup.backup(aStatement,
                                 aTable,
                                 aStatistics)
             != IDE_SUCCESS);

    sState = 0;
    IDE_TEST(sTableBackup.destroy() != IDE_SUCCESS);

    /* BUG-31881 이후 Restore를 대비해 PageReservation 시작 */
    sTable = (smcTableHeader*)((UChar*)aTable+SMP_SLOT_HEADER_SIZE);
    IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID( sTable->mSpaceID,
                                                        (void**)&sTBSNode )
              != IDE_SUCCESS );
    IDE_TEST( smmFPLManager::beginPageReservation( (smmTBSNode*)sTBSNode,
                                                   aStatement->mTrans->mTrans)
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if(sState != 0)
    {
        IDE_PUSH();

        IDE_ASSERT(sTableBackup.destroy() == IDE_SUCCESS);

        IDE_POP();
    }
    return IDE_FAILURE;
}

/*******************************************************************
 * Description : Volatile Table을 백업한다.
 *******************************************************************/
IDE_RC smiTable::backupVolatileTable(smiStatement * aStatement,
                                     const void   * aTable,
                                     SChar        * aBackupFileName,
                                     idvSQL       * aStatistics)
{
    smiVolTableBackup   sTableBackup;
    SInt                sState = 0;
    smcTableHeader    * sTable;
    svmTBSNode        * sTBSNode;

    IDE_TEST(sTableBackup.initialize(aTable, aBackupFileName) != IDE_SUCCESS);
    sState = 1;

    IDE_TEST(sTableBackup.backup(aStatement,
                                 aTable,
                                 aStatistics)
             != IDE_SUCCESS);

    sState = 0;
    IDE_TEST(sTableBackup.destroy() != IDE_SUCCESS);

    /* BUG-31881 이후 Restore를 대비해 PageReservation 시작 */
    sTable = (smcTableHeader*)((UChar*)aTable+SMP_SLOT_HEADER_SIZE);
    IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID( sTable->mSpaceID,
                                                        (void**)&sTBSNode )
              != IDE_SUCCESS );
    IDE_TEST( svmFPLManager::beginPageReservation( (svmTBSNode*)sTBSNode,
                                                   aStatement->mTrans->mTrans)
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if(sState != 0)
    {
        IDE_PUSH();

        IDE_ASSERT(sTableBackup.destroy() == IDE_SUCCESS);

        IDE_POP();
    }
    return IDE_FAILURE;
}


/* ------------------------------------------------
 * for PR-4360 ALTER TABLE
 * ----------------------------------------------*/
IDE_RC smiTable::backupTableForAlterTable( smiStatement* aStatement,
                                           const void*   aSrcTable,
                                           const void*   aDstTable,
                                           idvSQL       *aStatistics )
{
    smxTrans               * sTrans;
    void                   * sTBSNode;
    SChar                    sBackupFileName[SM_MAX_FILE_NAME];
    idBool                   sRemoveBackupFile = ID_FALSE;
    UInt                     sState = 0;
    smOID                    sSrcOID;
    smOID                    sDstOID;
    smcTableHeader         * sSrcTable;
    smcTableHeader         * sDstTable;

    sTrans = (smxTrans*)aStatement->mTrans->mTrans;

    sSrcTable = (smcTableHeader*)((UChar*)aSrcTable+SMP_SLOT_HEADER_SIZE);
    sDstTable = (smcTableHeader*)((UChar*)aDstTable+SMP_SLOT_HEADER_SIZE);

    IDE_TEST( aStatement->prepareDDL((smiTrans*)aStatement->mTrans) 
              != IDE_SUCCESS );
    sTrans->mDoSkipCheckSCN = ID_TRUE;

    idlOS::memset( sBackupFileName, '\0', SM_MAX_FILE_NAME );

    IDL_MEM_BARRIER;

    //BUG-15627 Alter Add Column시 Abort할 경우 Table의 SCN을 변경해야 함.
    //smaLogicalAger::doInstantAgingWithTable 의 abort로 인해 발생할 수 있는
    //잘못된 view를 보게되는 현상을 방지 하기 위해 abort시에 table의 scn을 바꾸어
    //이 table을 보려고 시도하고 있던 트랜잭션을 rebuild시킨다.
    IDE_TEST(
        smrLogMgr::writeNTALogRec(NULL, /* idvSQL* */
                                  sTrans,
                                  smxTrans::getTransLstUndoNxtLSNPtr((void*)sTrans),
                                  sSrcTable->mSpaceID,
                                  SMR_OP_INSTANT_AGING_AT_ALTER_TABLE,
                                  sSrcTable->mSelfOID )
        != IDE_SUCCESS );

    IDE_TEST(smaLogicalAger::doInstantAgingWithTable( sSrcTable->mSelfOID )
             != IDE_SUCCESS);

    sRemoveBackupFile = ID_TRUE;

    /* PROJ-1594 Volatile TBS */
    /* Table type에 따라 backupTable 함수가 다르다.*/
    if( SMI_TABLE_TYPE_IS_VOLATILE( sSrcTable ) == ID_TRUE )
    {
        IDE_TEST( backupVolatileTable( aStatement, 
                                       aSrcTable, 
                                       sBackupFileName,
                                       aStatistics )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Volatile Table이 아니면 반드시 Memory 테이블이다. */
        IDE_DASSERT( SMI_TABLE_TYPE_IS_MEMORY( sSrcTable ) == ID_TRUE );

        IDE_TEST( backupMemTable( aStatement, 
                                  aSrcTable, 
                                  sBackupFileName,
                                  aStatistics )
                  != IDE_SUCCESS );
    }

    /* BUG-31881 backupMem/VolTable을 하면서 pageReservation 시작 */
    sState = 1;

    IDU_FIT_POINT( "1.BUG-34262@smiTable::backupTableForAlterTable" );

    /* BUG-17954 : Add Column 구문 수행시, Unable to open a backup file로 서버
     * 비정상 종료
     *
     * 파일이 생성된 후 Transaction OID List에 Transaction이 끝난후 File을
     * 지우도록 OID 정보를 Add한다. 만약 backupTable 전에 하게 되고 backup에서
     * 에러가 발생한다면 backupTable은 내부적으로 에러발생시 생성한 backup
     * file을
     * 삭제한다. 때문에 없는 File에 대해서 삭제 요청을 하게 된다.
     * 그런데 같은 Table에 Alter Add Column을 뒤이어서 다른 Transaction이 
     * 수행할때 Alter를 위해서 backupTable에서 backup file을 만들었는데 이전
     * Transaction이 잘못 요청한 Delete Backup File을 LFThread가 수행하여 
     * 현재 Alter의 수행을 위해 필요한 파일을 지우는 경우가 발생한다. */
    IDE_TEST(sTrans->addOID(sSrcTable->mSelfOID,
                            ID_UINT_MAX,
                            sSrcTable->mSpaceID,
                            SM_OID_DELETE_ALTER_TABLE_BACKUP) != IDE_SUCCESS);

    sRemoveBackupFile = ID_FALSE;

    /* BUG-34438  if server is killed when alter table(memory table) before
     * logging SMR_OP_ALTER_TABLE log, all rows are deleted 
     * 예외 발생가 발생 하면 dest Table Restore수행 시 사용한
     * Page들을 DB로 돌려줄 수 있도록, 새 Table(aDstHeader)를 Logging한다. 
     * smcTable::dropTablePageListPending()함수에서 하던 작업을 이동시킨 것*/
    sSrcOID = sSrcTable->mSelfOID;
    sDstOID = sDstTable->mSelfOID;

    IDE_TEST(smrLogMgr::writeNTALogRec(NULL, /* idvSQL* */
                                   sTrans,
                                   smxTrans::getTransLstUndoNxtLSNPtr((void*)sTrans),
                                   sSrcTable->mSpaceID,
                                   SMR_OP_ALTER_TABLE,
                                   sSrcOID,
                                   sDstOID)
             != IDE_SUCCESS);

    sState = 0;


    IDE_TEST( smcTable::dropTablePageListPending(
                  sTrans,
                  sSrcTable,
                  ID_FALSE) // aUndo
              != IDE_SUCCESS );

    IDU_FIT_POINT( "1.BUG-34384@smiTable::backupTableForAlterTable" );

    IDU_FIT_POINT( "1.BUG-16841@smiTable::backupTableForAlterTable" );

    IDU_FIT_POINT( "1.BUG-32780@smiTable::backupTableForAlterTable" );

    sTrans->mDoSkipCheckSCN = ID_FALSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    if( sState != 0 )
    {
        (void)sctTableSpaceMgr::findSpaceNodeBySpaceID( sSrcTable->mSpaceID,
                                                        (void**)&sTBSNode );
        if( SMI_TABLE_TYPE_IS_VOLATILE( sSrcTable ) == ID_TRUE )
        {
            (void)svmFPLManager::endPageReservation( (svmTBSNode*)sTBSNode,
                                                     sTrans );
        }
        else
        {
            (void)smmFPLManager::endPageReservation( (smmTBSNode*)sTBSNode,
                                                     sTrans );
        }
    }

    /* BUG-34262 
     * When alter table add/modify/drop column, backup file cannot be
     * deleted. 
     */
    /* BUG-38254 alter table xxx 에서 hang이 걸릴수 있습니다.
     * 기존의 동일한 이름의 tbk 파일이 있는 exception 상황이면 
     * 파일을 지우지 않습니다. 
     */
    if( (sRemoveBackupFile == ID_TRUE) &&
        (idlOS::access( sBackupFileName, F_OK ) == 0) &&
        ( ideGetErrorCode() != smERR_ABORT_UnableToExecuteAlterTable) )
    { 
        if( idf::unlink( sBackupFileName ) != 0 )
        {
            ideLog::log( SM_TRC_LOG_LEVEL_INTERFACE,
                         SM_TRC_INTERFACE_FILE_UNLINK_ERROR,
                         sBackupFileName, errno );
        }
        else 
        {
            /* nothing to do */
        }
    }
    else
    {
        /* nothing to do */
    }

    sTrans->mDoSkipCheckSCN = ID_FALSE;

    return IDE_FAILURE;

}

IDE_RC smiTable::restoreMemTable(void                  * aTrans,
                                 const void            * aSrcTable,
                                 const void            * aDstTable,
                                 smOID                   aTableOID,
                                 smiColumnList         * aSrcColumnList,
                                 smiColumnList         * aBothColumnList,
                                 smiValue              * aNewRow,
                                 idBool                  aUndo,
                                 smiAlterTableCallBack * aCallBack )
{
    smiTableBackup   sTableBackup;
    SInt             sState = 0;
    smcTableHeader * sTable;
    smmTBSNode     * sTBSNode;
    
    // 원본 테이블이 저장된 파일을 찾아서
    IDE_TEST(sTableBackup.initialize(aSrcTable, NULL) != IDE_SUCCESS);
    sState = 1;

    // 대상 테이블에 복구시킨다.
    IDU_FIT_POINT( "1.BUG-17955@smi:table:restoretable" );
    IDE_TEST(sTableBackup.restore(aTrans,
                                  aDstTable,
                                  aTableOID,
                                  aSrcColumnList,
                                  aBothColumnList,
                                  aNewRow,
                                  aUndo,
                                  aCallBack)
             != IDE_SUCCESS);


    sState = 0;
    IDE_TEST(sTableBackup.destroy() != IDE_SUCCESS);

    /* BUG-31881 이후 PageReservation 종료 */
    sTable = (smcTableHeader*)((UChar*)aSrcTable+SMP_SLOT_HEADER_SIZE);
    IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID( sTable->mSpaceID,
                                                        (void**)&sTBSNode )
              != IDE_SUCCESS );
    IDE_TEST( smmFPLManager::endPageReservation( (smmTBSNode*)sTBSNode,
                                                 aTrans )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if(sState != 0)
    {
        IDE_PUSH();
        IDE_ASSERT(sTableBackup.destroy() == IDE_SUCCESS);
        IDE_POP();
    }
    return IDE_FAILURE;
}

IDE_RC smiTable::restoreVolTable(void                  * aTrans,
                                 const void            * aSrcTable,
                                 const void            * aDstTable,
                                 smOID                   aTableOID,
                                 smiColumnList         * aSrcColumnList,
                                 smiColumnList         * aBothColumnList,
                                 smiValue              * aNewRow,
                                 idBool                  aUndo,
                                 smiAlterTableCallBack * aCallBack )
{
    smiVolTableBackup   sTableBackup;
    SInt                sState = 0;
    smcTableHeader    * sTable;
    svmTBSNode        * sTBSNode;

    // 원본 테이블이 저장된 파일을 찾아서
    IDE_TEST(sTableBackup.initialize(aSrcTable, NULL) != IDE_SUCCESS);
    sState = 1;

    // 대상 테이블에 복구시킨다.
    IDE_TEST(sTableBackup.restore(aTrans,
                                  aDstTable,
                                  aTableOID,
                                  aSrcColumnList,
                                  aBothColumnList,
                                  aNewRow,
                                  aUndo,
                                  aCallBack)
             != IDE_SUCCESS);

    sState = 0;
    IDE_TEST(sTableBackup.destroy() != IDE_SUCCESS);

    /* BUG-31881 이후 PageReservation 종료 */
    sTable = (smcTableHeader*)((UChar*)aSrcTable+SMP_SLOT_HEADER_SIZE);
    IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID( sTable->mSpaceID,
                                                        (void**)&sTBSNode )
              != IDE_SUCCESS );
    IDE_TEST( svmFPLManager::endPageReservation( (svmTBSNode*)sTBSNode,
                                                 aTrans )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if(sState != 0)
    {
        IDE_PUSH();
        IDE_ASSERT(sTableBackup.destroy() == IDE_SUCCESS);
        IDE_POP();
    }
    return IDE_FAILURE;
}

IDE_RC smiTable::restoreTableForAlterTable( 
    smiStatement          * aStatement,
    const void            * aSrcTable,
    const void            * aDstTable,
    smOID                   aTableOID,
    smiColumnList         * aSrcColumnList,
    smiColumnList         * aBothColumnList,
    smiValue              * aNewRow,
    smiAlterTableCallBack * aCallBack )
{
    smcTableHeader *sDstTableHeader =
        (smcTableHeader*)( (UChar*)aDstTable + SMP_SLOT_HEADER_SIZE );

    IDU_FIT_POINT( "1.BUG-42411@smiTable::restoreTableForAlterTable" ); 
    /* PROJ-1594 Volatile TBS */
    /* Volatile Table인 경우 smiVolTableBackup을 사용해야 한다. */
    if( SMI_TABLE_TYPE_IS_VOLATILE( sDstTableHeader ) == ID_TRUE )
    {
        IDE_TEST(restoreVolTable(aStatement->mTrans->mTrans,
                                 aSrcTable,
                                 aDstTable,
                                 aTableOID,
                                 aSrcColumnList,
                                 aBothColumnList,
                                 aNewRow,
                                 ID_FALSE, /* Undo */
                                 aCallBack)
                 != IDE_SUCCESS);
    }
    else
    {
        /* Volatile Table이 아니면 Memory Table이어야 한다.*/
        IDE_DASSERT( SMI_TABLE_TYPE_IS_MEMORY( sDstTableHeader ) == ID_TRUE );

        IDE_TEST(restoreMemTable(aStatement->mTrans->mTrans,  
                                 aSrcTable,
                                 aDstTable,
                                 aTableOID,
                                 aSrcColumnList,
                                 aBothColumnList,
                                 aNewRow,
                                 ID_FALSE, /* Undo */
                                 aCallBack)
                 != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : alter table 과정에서 exception발생으로 인한 undo
 * 하는 루틴이다. disk table에서는 고려할 필요없다.
 ***********************************************************************/
IDE_RC  smiTable::restoreTableByAbort( void         * aTrans,
                                       smOID          aSrcOID,
                                       smOID          aDstOID,
                                       idBool         aDoRestart )
{

    smcTableHeader * sSrcHeader = NULL;
    smcTableHeader * sDstHeader = NULL;
    void           * sSrcSlot;
    smiValue        *sArrValue;
    UInt             sColumnCnt;
    smiColumnList   *sColumnList;
    UInt             sState     = 0;
    UInt             i;

    /* BUG-30457
     * AlterTable이 실패했으면, 이전 테이블 복구를 위해
     * 새 테이블 생성을 위해 사용한 페이지를 반환합니다. 
     * 
     * 그런데 현재는 Old/new TableOid를 모두 Logging하지만 
     * 과거에는 NewTable의 OID를 Logging하지 않았기에, NewTable의 OID(aDstOID)
     * 를 모르기에, skip합니다. (페이지 사용량을 줄이는 연산이기에 해주지 않
     * 아도 치명저적인 문제는 없습니다. */
    if( aDstOID != 0 )
    {
        IDE_ASSERT( smmManager::getOIDPtr( 
                        SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                        aDstOID + SMP_SLOT_HEADER_SIZE,
                        (void**)&sDstHeader )
                    == IDE_SUCCESS );

        /* RestartRecovery시에는 Runtime구조체가 없습니다.
         * 생성 후 처리 끝나면 제거해줘야 합니다. */
        if( aDoRestart == ID_TRUE )
        {
            IDE_TEST( smcTable::prepare4LogicalUndo( sDstHeader )
                      != IDE_SUCCESS );
        }

        IDE_TEST( smcTable::dropTablePageListPending(aTrans,
                                                     sDstHeader,
                                                     ID_TRUE)   // aUndo
                  != IDE_SUCCESS);

        if( aDoRestart == ID_TRUE )
        {
            IDE_TEST( smcTable::finalizeTable4LogicalUndo( sDstHeader,
                                                           aTrans )
                      != IDE_SUCCESS);
        }
    }

    IDE_ASSERT( smmManager::getOIDPtr( 
                    SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                    aSrcOID,
                    (void**)&sSrcSlot )
                == IDE_SUCCESS );

    sSrcHeader = (smcTableHeader*)((UChar*)sSrcSlot + SMP_SLOT_HEADER_SIZE);
    sColumnCnt = smcTable::getColumnCount(sSrcHeader);

    IDE_ASSERT( sColumnCnt > 0 );


    /* RestartRecovery시에는 Runtime구조체가 없습니다.
     * 생성 후 처리 끝나면 제거해줘야 합니다. */
    if( aDoRestart == ID_TRUE)
    {
        IDE_TEST( smcTable::prepare4LogicalUndo( sSrcHeader )
                  != IDE_SUCCESS );
        sState = 1;
    }

    /* BUG-34438  if server is killed when alter 
     * table(memory table) before logging SMR_OP_ALTER_TABLE log, 
     * all rows are deleted
     * 원본테이블에 page들이 매달려 있는 상태에서 table을 restore하면
     * 기존에 있던 row뿐만 아니라 테이블 백업 파일로부터 복원된 row가
     * 테이블에 삽입된다. 확실하게 원본테이블에 page가 존재하지
     * 않는것을 보장하기위해 추가된 코드이다. */
    IDE_TEST( smcTable::dropTablePageListPending(aTrans,
                                                 sSrcHeader,
                                                 ID_TRUE)   // aUndo
              != IDE_SUCCESS);

    IDU_FIT_POINT( "smiTable::restoreTableByAbort::malloc" );
    /* Abort시에는 Column 정보 및 Value를 저장할 Array가 없기에
     * 할당해줍니다. */
    sArrValue = NULL;
    IDE_TEST( iduMemMgr::malloc(IDU_MEM_SM_SMI,
                                (ULong)ID_SIZEOF(smiValue) * sColumnCnt,
                                (void**)&sArrValue,
                                IDU_MEM_FORCE)
              != IDE_SUCCESS );
    sState = 2;


    sColumnList = NULL;
    IDE_TEST( iduMemMgr::malloc(IDU_MEM_SM_SMI,
                                (ULong)ID_SIZEOF(smiColumnList) * sColumnCnt,
                                (void**)&sColumnList,
                                IDU_MEM_FORCE)
              != IDE_SUCCESS );
    sState = 3;

    for(i = 0 ; i < sColumnCnt; i++)
    {
        sColumnList[i].next = sColumnList + i + 1;
        sColumnList[i].column = smcTable::getColumn(sSrcHeader, i);
    }
    sColumnList[i - 1].next = NULL;

    /* Abort이기 때문에 OldTable의 내용을 OldTable로 Restore한다.
     * 그래서 Src/Dst 모두 동일하다. */
    if ( SMI_TABLE_TYPE_IS_MEMORY( sSrcHeader ) == ID_TRUE )
    {
        IDE_TEST( restoreMemTable( aTrans,
                                   sSrcSlot,
                                   sSrcSlot,
                                   aSrcOID,
                                   sColumnList,
                                   sColumnList,
                                   sArrValue,
                                   ID_TRUE, /* Undo */
                                   NULL )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_DASSERT( SMI_TABLE_TYPE_IS_VOLATILE( sSrcHeader ) == ID_TRUE );


        /* BUG-34438  if server is killed when alter table(memory table) before
         * logging SMR_OP_ALTER_TABLE log, all rows are deleted
         * volatile table은 예외 처리시에만 restore를 수행한다.
         * restart recovery시에는 서버가 종료되었다 다시 시작함으로 volatile
         * table의 레코드들을 유지할 필요가 없기때문이다. 
         * server startup시 smaRefineDB::initAllVolatileTables()에서 초기화됨*/
        if( aDoRestart == ID_FALSE )
        {
            IDE_TEST( restoreVolTable( aTrans,
                                       sSrcSlot,
                                       sSrcSlot,
                                       aSrcOID,
                                       sColumnList,
                                       sColumnList,
                                       sArrValue,
                                       ID_TRUE, /* Undo */
                                       NULL )
                      != IDE_SUCCESS );
        }
    }

    IDU_FIT_POINT( "3.BUG-42411@smiTable::restoreTableByAbort" ); 
    sState = 2;
    IDE_TEST (iduMemMgr::free(sColumnList) != IDE_SUCCESS);
    sState = 1;
    IDE_TEST (iduMemMgr::free(sArrValue) != IDE_SUCCESS);

    if( aDoRestart == ID_TRUE)
    {
        IDE_TEST( smcTable::finalizeTable4LogicalUndo( sSrcHeader,
                                                       aTrans )
                 != IDE_SUCCESS);
        sState = 1;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    switch( sState )
    {
        case 3:
            (void) iduMemMgr::free(sColumnList) ;
            sColumnList = NULL;
        case 2:
            (void) iduMemMgr::free(sArrValue) ;
            sArrValue = NULL;
        case 1:
            if(smrRecoveryMgr::isRestart() == ID_TRUE)
            {
                (void) smcTable::finalizeTable4LogicalUndo( sSrcHeader,
                                                            aTrans );
            }
            break;
        default:
            break;
    }
    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC smiTable::createSequence( smiStatement    * aStatement,
                                 SLong             aStartSequence,
                                 SLong             aIncSequence,
                                 SLong             aSyncInterval,
                                 SLong             aMaxSequence,
                                 SLong             aMinSequence,
                                 UInt              aFlag,
                                 const void     ** aTable )
{

    IDE_TEST( aStatement->prepareDDL((smiTrans*)aStatement->mTrans) != IDE_SUCCESS );

    IDE_TEST(smcSequence::createSequence((smxTrans*)aStatement->mTrans->mTrans,
                                         aStartSequence,
                                         aIncSequence,
                                         aSyncInterval,
                                         aMaxSequence,
                                         aMinSequence,
                                         aFlag,
                                         (smcTableHeader**)aTable)
             != IDE_SUCCESS);

    *aTable = (const void*)((const UChar*)*aTable-SMP_SLOT_HEADER_SIZE);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC smiTable::alterSequence(smiStatement    * aStatement,
                               const void      * aTable,
                               SLong             aIncSequence,
                               SLong             aSyncInterval,
                               SLong             aMaxSequence,
                               SLong             aMinSequence,
                               UInt              aFlag,
                               idBool            aIsRestart,
                               SLong             aStartSequence,
                               SLong           * aLastSyncSeq)
{

    smcTableHeader *sTableHeader;

    IDE_TEST( aStatement->prepareDDL((smiTrans*)aStatement->mTrans) != IDE_SUCCESS );

    IDE_TEST_RAISE( SMP_SLOT_IS_DROP( (smpSlotHeader*)aTable ),
                    already_droped_sequence );

    sTableHeader = (smcTableHeader*)((UChar*)aTable+SMP_SLOT_HEADER_SIZE);

    IDE_TEST(smcSequence::alterSequence((smxTrans*)aStatement->mTrans->mTrans,
                                        sTableHeader,
                                        aIncSequence,
                                        aSyncInterval,
                                        aMaxSequence,
                                        aMinSequence,
                                        aFlag,
                                        aIsRestart,
                                        aStartSequence,
                                        aLastSyncSeq)
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(already_droped_sequence)
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_droped_Sequence));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC smiTable::resetSequence( smiStatement    * aStatement,
                                const void      * aTable )
{

    smcTableHeader *sTableHeader;

    IDE_TEST( aStatement->prepareDDL( (smiTrans*)aStatement->mTrans ) != IDE_SUCCESS );

    IDE_TEST_RAISE( SMP_SLOT_IS_DROP( (smpSlotHeader*)aTable ),
                    already_droped_sequence );

    sTableHeader = (smcTableHeader*)((UChar*)aTable+SMP_SLOT_HEADER_SIZE);

    IDE_TEST( smcSequence::resetSequence( (smxTrans*)aStatement->mTrans->mTrans,
                                          sTableHeader )
              != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION( already_droped_sequence )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_droped_Sequence ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC smiTable::readSequence(smiStatement        * aStatement,
                              const void          * aTable,
                              SInt                  aFlag,
                              SLong               * aSequence,
                              smiSeqTableCallBack * aCallBack )
{

    smxTrans       *sTrans;
    UInt            sState = 0;
    smcTableHeader *sTableHeader = NULL;
    void           *sLockItem    = NULL;

    sTrans = (smxTrans*)aStatement->getTrans()->getTrans();

    sTableHeader = (smcTableHeader*)((UChar*)aTable+SMP_SLOT_HEADER_SIZE);
    sLockItem = SMC_TABLE_LOCK( sTableHeader );

    IDE_TEST_RAISE( SMP_SLOT_IS_DROP( (smpSlotHeader*)aTable ),
                    already_droped_sequence );

    IDE_TEST_RAISE( smlLockMgr::getMutexOfLockItem( sLockItem )
                        ->lock( NULL ) != IDE_SUCCESS,
                    mutex_lock_error );
    sState = 1;

    if ( ( aFlag & SMI_SEQUENCE_MASK ) == SMI_SEQUENCE_CURR )
    {
        IDE_TEST( smcSequence::readSequenceCurr( sTableHeader,
                                                 aSequence )
                  != IDE_SUCCESS );
    }
    else
    {
        if ( ( aFlag & SMI_SEQUENCE_MASK ) == SMI_SEQUENCE_NEXT )
        {
            IDE_TEST(smcSequence::readSequenceNext(sTrans,
                                                   sTableHeader,
                                                   aSequence,
                                                   aCallBack)
                     != IDE_SUCCESS);
        }
        else
        {
            IDE_ASSERT(0);
        }
    }

    sState = 0;
    IDE_TEST_RAISE( smlLockMgr::getMutexOfLockItem( sLockItem )
                        ->unlock( ) != IDE_SUCCESS,
                    mutex_unlock_error );

    return IDE_SUCCESS;

    IDE_EXCEPTION( mutex_lock_error );
    {
        IDE_SET(ideSetErrorCode (smERR_FATAL_ThrMutexLock));
    }
    IDE_EXCEPTION( mutex_unlock_error );
    {
        IDE_SET(ideSetErrorCode (smERR_FATAL_ThrMutexUnlock));
    }
    IDE_EXCEPTION(already_droped_sequence)
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_droped_Sequence));
    }
    IDE_EXCEPTION_END;

    if(sState != 0)
    {
        (void)smlLockMgr::getMutexOfLockItem( sLockItem )->unlock();
    }

    return IDE_FAILURE;


}
/*
 * for internal (queue replicaiton) use
 */
IDE_RC smiTable::setSequence( smiStatement        * aStatement,
                              const void          * aTable,
                              SLong                 aSeqValue )
{
    smxTrans       *sTrans;
    UInt            sState = 0;
    smcTableHeader *sTableHeader = NULL;
    void           *sLockItem    = NULL;

    sTrans = (smxTrans*)aStatement->getTrans()->getTrans();

    sTableHeader = (smcTableHeader*)((UChar*)aTable+SMP_SLOT_HEADER_SIZE);
    sLockItem = SMC_TABLE_LOCK( sTableHeader );

    IDE_TEST_RAISE( SMP_SLOT_IS_DROP( (smpSlotHeader*)aTable ),
                    already_droped_sequence );

    IDE_TEST_RAISE( smlLockMgr::getMutexOfLockItem( sLockItem )
                        ->lock( NULL ) != IDE_SUCCESS,
                    mutex_lock_error );
    sState = 1;

    IDE_TEST( smcSequence::setSequenceCurr( sTrans,
                                            sTableHeader,
                                            aSeqValue )
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST_RAISE( smlLockMgr::getMutexOfLockItem( sLockItem )
                        ->unlock( ) != IDE_SUCCESS,
                    mutex_unlock_error );

    return IDE_SUCCESS;

    IDE_EXCEPTION( mutex_lock_error );
    {
        IDE_SET( ideSetErrorCode( smERR_FATAL_ThrMutexLock ) );
    }
    IDE_EXCEPTION( mutex_unlock_error );
    {
        IDE_SET( ideSetErrorCode( smERR_FATAL_ThrMutexUnlock ) );
    }
    IDE_EXCEPTION( already_droped_sequence )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_droped_Sequence ) );
    }
    IDE_EXCEPTION_END;

    if(sState != 0)
    {
        (void)smlLockMgr::getMutexOfLockItem( sLockItem )->unlock();
    }
    else
    {
        /*do nothing*/
    }

    return IDE_FAILURE;
}

IDE_RC smiTable::dropSequence( smiStatement    * aStatement,
                               const void      * aTable )
{
    IDE_TEST( aStatement->prepareDDL((smiTrans*)aStatement->mTrans)
              != IDE_SUCCESS );

    IDE_TEST_RAISE( SMP_SLOT_IS_DROP( (smpSlotHeader*)aTable ),
                    already_droped_sequence );

    IDE_TEST( smcTable::dropTable(
                 (smxTrans*)aStatement->mTrans->mTrans,
                 (smcTableHeader*)((UChar*)aTable+SMP_SLOT_HEADER_SIZE),
                 sctTableSpaceMgr::getTBSLvType2Opt( SMI_TBSLV_DDL_DML ),
                 SMC_LOCK_MUTEX) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(already_droped_sequence)
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_droped_Sequence));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}


IDE_RC smiTable::getSequence( const void      * aTable,
                              SLong           * aStartSequence,
                              SLong           * aIncSequence,
                              SLong           * aSyncInterval,
                              SLong           * aMaxSequence,
                              SLong           * aMinSequence,
                              UInt            * aFlag )
{
    UInt            sState = 0;
    smcTableHeader *sTableHeader = NULL;
    void           *sLockItem    = NULL;

    sTableHeader = (smcTableHeader*)((UChar*)aTable+SMP_SLOT_HEADER_SIZE);
    sLockItem = SMC_TABLE_LOCK( sTableHeader );

    IDE_TEST_RAISE( SMP_SLOT_IS_DROP( (smpSlotHeader*)aTable ),
                    already_droped_sequence );

    IDE_TEST_RAISE( smlLockMgr::getMutexOfLockItem( sLockItem )
                        ->lock( NULL ) != IDE_SUCCESS,
                    mutex_lock_error );
    sState = 1;

    *aStartSequence = sTableHeader->mSequence.mStartSequence;
    *aIncSequence   = sTableHeader->mSequence.mIncSequence;
    *aSyncInterval  = sTableHeader->mSequence.mSyncInterval;
    *aMaxSequence   = sTableHeader->mSequence.mMaxSequence;
    *aMinSequence   = sTableHeader->mSequence.mMinSequence;
    *aFlag          = sTableHeader->mSequence.mFlag;

    sState = 0;
    IDE_TEST_RAISE( smlLockMgr::getMutexOfLockItem( sLockItem )
                        ->unlock( ) != IDE_SUCCESS,
                    mutex_unlock_error );

    return IDE_SUCCESS;

    IDE_EXCEPTION( mutex_lock_error );
    {
        IDE_SET(ideSetErrorCode (smERR_FATAL_ThrMutexLock));
    }
    IDE_EXCEPTION( mutex_unlock_error );
    {
        IDE_SET(ideSetErrorCode (smERR_FATAL_ThrMutexUnlock));
    }
    IDE_EXCEPTION(already_droped_sequence)
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_droped_Sequence));
    }

    IDE_EXCEPTION_END;

    if(sState != 0)
    {
        (void)smlLockMgr::getMutexOfLockItem( sLockItem )->unlock();
    }

    return IDE_FAILURE;

}

IDE_RC smiTable::refineSequence(smiStatement  * aStatement,
                                const void    * aTable)
{
    smcTableHeader *sTableHeader;

    IDE_TEST( aStatement->prepareDDL((smiTrans*)aStatement->mTrans) != IDE_SUCCESS );

    IDE_TEST_RAISE( SMP_SLOT_IS_DROP( (smpSlotHeader*)aTable ),
                    already_droped_sequence );

    sTableHeader = (smcTableHeader*)((UChar*)aTable+SMP_SLOT_HEADER_SIZE);

    IDE_TEST( smcSequence::refineSequence( sTableHeader )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(already_droped_sequence)
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_droped_Sequence));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC smiTable::disableIndex( smiStatement* aStatement,
                               const void*   aTable,
                               const void*   aIndex )
{

    UInt sFlag;
    SInt sState = 0;
    smcTableHeader *sTableHeader;
    smnIndexHeader *sIndexHeader;


    sTableHeader = (smcTableHeader*)((UChar*)aTable+SMP_SLOT_HEADER_SIZE);
    sIndexHeader = (smnIndexHeader*)aIndex;

    IDE_TEST(aStatement->prepareDDL((smiTrans*)aStatement->mTrans)
             != IDE_SUCCESS);

    // fix BUG-8672
    // 자신이 등록한 oid가 처리될때까지 wait하기 때문에
    // statement begin만 하고, cursor를 open하지 않기 때문에
    // memory gc가 skip하도록 한다.
   ((smxTrans*)(aStatement->mTrans->mTrans))->mDoSkipCheckSCN = ID_TRUE;
    IDL_MEM_BARRIER;

    // Instant Aging실시
    IDE_TEST(smaLogicalAger::doInstantAgingWithTable( sTableHeader->mSelfOID )
             != IDE_SUCCESS);

    ((smxTrans*)(aStatement->mTrans->mTrans))->mDoSkipCheckSCN = ID_FALSE;

    IDE_TEST( smcTable::latchExclusive( sTableHeader ) != IDE_SUCCESS );
    sState = 1;

    sFlag = sIndexHeader->mFlag & ~SMI_INDEX_PERSISTENT_MASK;

    // PRJ-1548 User Memory TableSpace 개념도입
    // Validate Table 에서 다음과 같이 Lock을 획득한다.
    // [1] Table의 TableSpace에 대해 IX
    // [2] Table에 대해 X
    // [3] Table의 Index, Lob Column TableSpace에 대해 IX
    // Table의 상위는 Table의 TableSpace이며 그에 대해 IX를 획득한다.
    // 단, 메모리 Table은 [3] 사항에 대해서 수행하지 않는다.
    IDE_TEST(smcTable::alterIndexInfo((smxTrans*)aStatement->mTrans->mTrans,
                                      sTableHeader,
                                      sIndexHeader,
                                      sFlag | SMI_INDEX_USE_DISABLE)
             != IDE_SUCCESS);

    /* Drop Index */
    IDE_TEST( smnManager::dropIndex( sTableHeader,
                                     sIndexHeader )
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( smcTable::unlatch( sTableHeader ) != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_PUSH();

        IDE_ASSERT( smcTable::unlatch( sTableHeader ) == IDE_SUCCESS );

        IDE_POP();
    }

    return IDE_FAILURE;

}

/*******************************************************************************
 * Description: 대상 table의 모든 index를 disable 한다.
 *
 * Related Issues:
 *      PROJ-2184 RP Sync 성능 향상
 *
 * aStatement       - [IN] smiStatement
 * aTable           - [IN] 대상 table의 smcTableHeader
 ******************************************************************************/
IDE_RC smiTable::disableAllIndex( smiStatement  * aStatement,
                                  const void    * aTable )
{
    smxTrans        * sTrans;
    smcTableHeader  * sTableHeader;
    UInt              sTableTypeFlag;
    UInt              sState = 0;


    sTrans       = (smxTrans*)aStatement->mTrans->mTrans;
    sTableHeader = (smcTableHeader*)((UChar*)aTable+SMP_SLOT_HEADER_SIZE);

    IDE_TEST_CONT( (sTableHeader->mFlag & SMI_TABLE_DISABLE_ALL_INDEX_MASK)
                    == SMI_TABLE_DISABLE_ALL_INDEX, already_disabled );

    // disk temp table에 대해서는 disable all index허용하지 않음.
    // fix BUG-21965
    sTableTypeFlag = sTableHeader->mFlag & SMI_TABLE_TYPE_MASK;
    IDE_ASSERT( (sTableTypeFlag == SMI_TABLE_MEMORY) ||
                (sTableTypeFlag == SMI_TABLE_DISK)   ||
                (sTableTypeFlag == SMI_TABLE_VOLATILE) );

    IDE_TEST(aStatement->prepareDDL((smiTrans*)aStatement->mTrans)
             != IDE_SUCCESS);

    // PRJ-1548 User Memory TableSpace 개념도입
    // Validate Table 에서 다음과 같이 Lock을 획득한다.
    // [1] Table의 TableSpace에 대해 IX
    // [2] Table에 대해 X
    // [3] Table의 Index, Lob Column TableSpace에 대해 IX
    // Table의 상위는 Table의 TableSpace이며 그에 대해 IX를 획득한다.
    // 단, 메모리 Table은 [3] 사항에 대해서 수행하지 않는다.
    IDE_TEST( smcTable::validateTable( sTrans,
                                       sTableHeader,
                                       SCT_VAL_DDL_DML )
              != IDE_SUCCESS );

    IDE_TEST( smcTable::latchExclusive( sTableHeader ) != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( smcTable::setUseFlagOfAllIndex( sTrans,
                                              sTableHeader,
                                              SMI_INDEX_USE_DISABLE )
              != IDE_SUCCESS );

    IDE_TEST( sTrans->addOID( sTableHeader->mSelfOID,
                              ID_UINT_MAX,
                              SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                              SM_OID_ALL_INDEX_DISABLE )
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( smcTable::unlatch( sTableHeader ) != IDE_SUCCESS );

    IDE_EXCEPTION_CONT( already_disabled );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_PUSH();

        IDE_ASSERT( smcTable::unlatch( sTableHeader ) == IDE_SUCCESS );

        IDE_POP();
    }

    return IDE_FAILURE;

}

/*******************************************************************************
 * Description: 대상 table의 모든 index를 enable 한다.
 *
 * Related Issues:
 *      PROJ-2184 RP Sync 성능 향상
 *
 * aStatement       - [IN] smiStatement
 * aTable           - [IN] 대상 table의 smcTableHeader
 ******************************************************************************/
IDE_RC smiTable::enableAllIndex( smiStatement  * aStatement,
                                 const void    * aTable )
{
    UInt                 sState = 0;
    smcTableHeader     * sTableHeader;
    smxTrans           * sTrans;
    UInt                 sTableTypeFlag;


    sTrans       = (smxTrans*)aStatement->mTrans->mTrans;
    sTableHeader = (smcTableHeader*)((UChar*)aTable+SMP_SLOT_HEADER_SIZE);

    IDE_TEST_CONT( (sTableHeader->mFlag & SMI_TABLE_DISABLE_ALL_INDEX_MASK)
                    == SMI_TABLE_ENABLE_ALL_INDEX, already_enabled );

    // disk temp table에 대해서는 enable  all index허용하지 않음.
    sTableTypeFlag = sTableHeader->mFlag & SMI_TABLE_TYPE_MASK;
    IDE_ASSERT( (sTableTypeFlag == SMI_TABLE_MEMORY) ||
                (sTableTypeFlag == SMI_TABLE_DISK)   ||
                (sTableTypeFlag == SMI_TABLE_VOLATILE) );

    IDE_TEST( aStatement->prepareDDL((smiTrans*)aStatement->mTrans)
              != IDE_SUCCESS );

    if( sTableTypeFlag != SMI_TABLE_DISK )
    {
        // fix BUG-8672
        // 자신이 등록한 oid가 처리될때까지 wait하기 때문에
        // statement begin만 하고, cursor를 open하지 않기 때문에
        // memory gc가 skip하도록 한다.
        sTrans->mDoSkipCheckSCN = ID_TRUE;
        IDL_MEM_BARRIER;

        // Instant Aging실시
        IDE_TEST( smaLogicalAger::doInstantAgingWithTable(
                                                sTableHeader->mSelfOID )
                  != IDE_SUCCESS);

        sTrans->mDoSkipCheckSCN = ID_FALSE;
    }
    else
    {
        /* Disk table은 aging 처리를 하지 않아도 된다. */
    }

    // PRJ-1548 User Memory TableSpace 개념도입
    // Validate Table 에서 다음과 같이 Lock을 획득한다.
    // [1] Table의 TableSpace에 대해 IX
    // [2] Table에 대해 X
    // [3] Table의 Index, Lob Column TableSpace에 대해 IX
    // Table의 상위는 Table의 TableSpace이며 그에 대해 IX를 획득한다.
    // 단, 메모리 Table은 [3] 사항에 대해서 수행하지 않는다.
    IDE_TEST( smcTable::validateTable( sTrans,
                                       sTableHeader,
                                       SCT_VAL_DDL_DML )
              != IDE_SUCCESS );

    IDE_TEST( smcTable::latchExclusive( sTableHeader ) != IDE_SUCCESS );
    sState = 1;

    // PROJ-1629
    IDE_TEST( smnManager::enableAllIndex( sTrans->mStatistics,
                                          sTrans,
                                          sTableHeader )
              != IDE_SUCCESS );

    IDE_TEST( smcTable::setUseFlagOfAllIndex( sTrans,
                                              sTableHeader,
                                              SMI_INDEX_USE_ENABLE )
              != IDE_SUCCESS);

    sState = 0;
    IDE_TEST( smcTable::unlatch( sTableHeader ) != IDE_SUCCESS );

    IDE_EXCEPTION_CONT( already_enabled );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0)
    {
        IDE_PUSH();
        IDE_ASSERT( smcTable::unlatch( sTableHeader ) == IDE_SUCCESS );
        IDE_POP();
    }

    return IDE_FAILURE;

}

/******************************************************************************
 * Description: 대상 table의 모든 index를 rebuild 한다.
 *
 * Related Issues:
 *      PROJ-2184 RP Sync 성능 향상
 * 
 * aStatement   - [IN] smiStatement
 * aTable       - [IN] 대상 table의 slot pointer
 *****************************************************************************/
IDE_RC smiTable::rebuildAllIndex( smiStatement  * aStatement,
                                  const void    * aTable )
{
    smxTrans        * sTrans;
    smcTableHeader  * sTableHeader;
    UInt              sTableTypeFlag;
    smnIndexHeader  * sIndexHeader;
    smnIndexHeader  * sIndexHeaderList;
    UInt              sIndexHeaderSize;
    smiColumnList     sColumnList[SMI_MAX_IDX_COLUMNS];
    smiColumn         sColumns[SMI_MAX_IDX_COLUMNS];
    UInt              sIndexIdx;
    UInt              sIndexCnt;
    const void      * sDummyIndexHandle;
    UInt              sState = 0;


    sTrans           = (smxTrans*)aStatement->mTrans->mTrans;
    sTableHeader     = (smcTableHeader*)((UChar*)aTable+SMP_SLOT_HEADER_SIZE);
    sIndexHeaderSize = smnManager::getSizeOfIndexHeader();

    IDE_TEST_RAISE( (sTableHeader->mFlag & SMI_TABLE_DISABLE_ALL_INDEX_MASK)
                    == SMI_TABLE_DISABLE_ALL_INDEX, all_index_disabled );

    // disk temp table에 대해서는 disable all index허용하지 않음.
    // fix BUG-21965
    sTableTypeFlag = sTableHeader->mFlag & SMI_TABLE_TYPE_MASK;
    IDE_ASSERT( (sTableTypeFlag == SMI_TABLE_MEMORY) ||
                (sTableTypeFlag == SMI_TABLE_DISK)   ||
                (sTableTypeFlag == SMI_TABLE_VOLATILE) );

    sIndexCnt = smcTable::getIndexCount( sTableHeader );

    IDU_FIT_POINT( "smiTable::rebuildAllIndex::malloc" );

    IDE_TEST( iduMemMgr::malloc( IDU_MEM_SM_SMI,
                                 (ULong)ID_SIZEOF(smnIndexHeader) * sIndexCnt,
                                 (void**)&sIndexHeaderList )
              != IDE_SUCCESS );
    sState = 1;

    /* Index 정보 보관 */
    for( sIndexIdx = 0; sIndexIdx < sIndexCnt; sIndexIdx++ )
    {
        sIndexHeader = (smnIndexHeader*)smcTable::getTableIndex( sTableHeader,
                                                                 sIndexIdx );

        IDE_ERROR( (sIndexHeader->mFlag & SMI_INDEX_USE_MASK)
                   == SMI_INDEX_USE_ENABLE );

        idlOS::memcpy( &sIndexHeaderList[sIndexIdx],
                       sIndexHeader,
                       sIndexHeaderSize );
    }

    /* 모든 index를 drop */
    for( sIndexIdx = 0; sIndexIdx < sIndexCnt; sIndexIdx++ )
    {
        /* index를 drop 할 때 마다 index가 하나씩 제거되기 때문에 매번 aIdx가
         * 0인 index, 즉 첫 번째 발견하는 index를 가져와서 drop 해야 한다. */
        sIndexHeader = (smnIndexHeader*)smcTable::getTableIndex( sTableHeader,
                                                                 0 ); /*aIdx*/

        IDE_TEST( dropIndex( aStatement,
                             aTable,
                             sIndexHeader,
                             SMI_TBSLV_DDL_DML )
                  != IDE_SUCCESS );
    }

    IDE_ERROR( sIndexIdx == sIndexCnt );

    /* 모든 index를 create */
    for( sIndexIdx = 0; sIndexIdx < sIndexCnt; sIndexIdx++ )
    {
        sIndexHeader = &sIndexHeaderList[sIndexIdx];

        IDE_TEST( makeKeyColumnList( sTableHeader,
                                     sIndexHeader,
                                     sColumnList,
                                     sColumns )
                  != IDE_SUCCESS );

        IDE_TEST( createIndex( sTrans->mStatistics,
                               aStatement,
                               sTableHeader->mSpaceID,
                               aTable,
                               sIndexHeader->mName,
                               sIndexHeader->mId,
                               sIndexHeader->mType,
                               sColumnList,
                               sIndexHeader->mFlag,
                               0, /* aParallelDegree */
                               SMI_INDEX_BUILD_DEFAULT,
                               sIndexHeader->mSegAttr,
                               sIndexHeader->mSegStorageAttr,
                               sIndexHeader->mMaxKeySize,
                               &sDummyIndexHandle )
                  != IDE_SUCCESS );
    }

    IDE_ERROR( sIndexCnt == smcTable::getIndexCount(sTableHeader) );

    sState = 0;
    IDE_TEST( iduMemMgr::free( sIndexHeaderList ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( all_index_disabled );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_ALL_INDEX_DISABLED));
    }
    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            IDE_ASSERT( iduMemMgr::free( sIndexHeaderList ) == IDE_SUCCESS );
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}


/*******************************************************************************
 * Description: index rebuild 과정에서 create index를 호출할 때 인자로 넘겨줄
 *      key column list를 구성하는 함수이다.
 *
 * Related Issues:
 *      PROJ-2184 RP Sync 성능 향상
 *
 * Parameters:
 *  aTableHeader    - [IN] smcTableHeader
 *  aIndexHeader    - [IN] smnIndexHeader
 *  aColumnList     - [IN] 새로 구성한 key column 정보를 기록할 column list 공간
 *  aColumns        - [IN] smiColumn 정보를 임시로 기록할 공간
 ******************************************************************************/
IDE_RC smiTable::makeKeyColumnList( void            * aTableHeader,
                                    void            * aIndexHeader,
                                    smiColumnList   * aColumnList,
                                    smiColumn       * aColumns  )
{
    smnIndexHeader    * sIndexHeader    = (smnIndexHeader*)aIndexHeader;
    UInt                sColumnCnt      = sIndexHeader->mColumnCount;
    UInt                sColumnIdx;
    const smiColumn   * sColumn;

    for( sColumnIdx = 0; sColumnIdx < sColumnCnt; sColumnIdx++ )
    {
        sColumn = smcTable::getColumn( aTableHeader,
                                       sIndexHeader->mColumns[sColumnIdx]
                                            & SMI_COLUMN_ID_MASK );

        idlOS::memcpy( &aColumns[sColumnIdx],
                       sColumn,
                       ID_SIZEOF(smiColumn) );

        aColumns[sColumnIdx].flag = sIndexHeader->mColumnFlags[sColumnIdx];
        aColumnList[sColumnIdx].column = &aColumns[sColumnIdx];

        if( sColumnIdx == (sColumnCnt - 1) )
        {
            aColumnList[sColumnIdx].next = NULL;
        }
        else
        {
            aColumnList[sColumnIdx].next = &aColumnList[sColumnIdx+1];
        }
    }

    return IDE_SUCCESS;
}


/*
 * MEMORY/DISK TABLE의 변경에 대한 DDL 수행시 SCN 변경을
 * 등록하려고 호출한다.
 *
 * [ 인자 ]
 * [IN] aStatement - Statement
 * [IN] aTable     - 변경할 Table Handle
 * [IN] aTBSLvType - Table 관련된 TBS(table/index/lob)들에 대한
 *                   Lock Validation 타입
 */
IDE_RC smiTable::touchTable( smiStatement        * aStatement,
                             const void          * aTable,
                             smiTBSLockValidType   aTBSLvType )
{

    return smiTable::modifyTableInfo(
                     aStatement,
                     aTable,
                     NULL,   // *aColumns
                     0,      // aColumnSize
                     NULL,   // *aInfo
                     0,      // aInfoSize
                     SMI_TABLE_FLAG_UNCHANGE,
                     aTBSLvType,
                     0,      // aMaxRow
                     0,      // aParallelDegree
                     ID_FALSE ); //aIsInitRowTemplate

}

void smiTable::getTableInfo( const void * aTable,
                             void      ** aTableInfo )
{
    smcTableHeader    *sTable;
    smVCDesc          *sColumnVCDesc;

    /* Table info를 저장하기 위한 aTableInfo에 대한 메모리 공간은
     * qp 단에서 할당받아야 함 */
    IDE_ASSERT(*aTableInfo != NULL);

    sTable = (smcTableHeader*)((UChar*)aTable+SMP_SLOT_HEADER_SIZE);
    sColumnVCDesc = &(sTable->mInfo);

    if ( sColumnVCDesc->length != 0 )
    {
        smcObject::getObjectInfo(sTable, aTableInfo);
    }
    else
    {
        *aTableInfo = NULL;
    }
}

void smiTable::getTableInfoSize( const void *aTable,
                                 UInt       *aInfoSize )
{
    const smVCDesc       *sInfoVCDesc;
    const smcTableHeader *sTable;

    sTable = (smcTableHeader*)((UChar*)aTable+SMP_SLOT_HEADER_SIZE);
    sInfoVCDesc = &(sTable->mInfo);

    *aInfoSize = sInfoVCDesc->length;
}

/***********************************************************************
 * Description : MEMORY TABLE에 FREE PAGE들을 DB에 반납한다.
 *
 * aStatement [IN] 작업 STATEMENT 객체
 * aTable     [IN] COMPACT할 TABLE의 헤더
 * aPages	  [IN] COMPACT할 Page 개수 (0, UINT_MAX : all)
 **********************************************************************/
IDE_RC smiTable::compactTable( smiStatement * aStatement,
                               const void   * aTable,
                               ULong          aPages )
{
    smcTableHeader * sTable;
    UInt             sPages;

    IDE_DASSERT(aStatement != NULL);
    IDE_DASSERT(aTable != NULL);

    sTable = (smcTableHeader*)((UChar*)aTable + SMP_SLOT_HEADER_SIZE);

    /* BUG-43464 UINT 보다 크면 ID_UINT_MAX 만큼만 compact 수행 */
    if ( aPages > (ULong)SC_MAX_PAGE_COUNT )
    {  
        sPages = SC_MAX_PAGE_COUNT; 
    }
    else
    {        
        sPages = aPages; 
    }

    IDE_TEST( smcTable::compact( aStatement->mTrans->mTrans,
                                 sTable,
                                 sPages )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
   tsm에서만 호출한다.
*/
IDE_RC smiTable::lockTable(smiTrans*        aTrans,
                           const void*      aTable,
                           smiTableLockMode aLockMode,
                           ULong            aLockWaitMicroSec)
{
    smcTableHeader*       sTable;
    idBool                sLocked = ID_FALSE;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aTable != NULL );

    sTable = (smcTableHeader*)((UChar*)aTable+SMP_SLOT_HEADER_SIZE);

    // 테이블의 테이블스페이스들에 대하여 INTENTION 잠금을 획득한다.
    IDE_TEST( sctTableSpaceMgr::lockAndValidateTBS(
                  (void*)((smxTrans*)aTrans->getTrans()), /* smxTrans* */
                  smcTable::getTableSpaceID((void*)sTable),/* smcTableHeader */
                  SCT_VAL_DDL_DML,
                  ID_TRUE,                     /* intent lock  여부 */
                  smlLockMgr::isNotISorS((smlLockMode)aLockMode),  /* exclusive lock 여부 */
                  aLockWaitMicroSec ) != IDE_SUCCESS );

    // 테이블 대하여 잠금을 획득한다.
    IDE_TEST( smlLockMgr::lockTable(((smxTrans*)aTrans->getTrans())->mSlotN,
                                     (smlLockItem *)(SMC_TABLE_LOCK( sTable )),
                                     (smlLockMode)aLockMode,
                                     aLockWaitMicroSec,
                                     NULL,
                                     &sLocked)
              != IDE_SUCCESS);

    IDE_TEST( sLocked != ID_TRUE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/*
    tsm에서만 호출한다.
*/
IDE_RC smiTable::lockTable( smiTrans*   aTrans,
                            const void* aTable )
{
    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aTable != NULL );

    // PROJ-1548
    // smiTable::lockTable을 호출하도록 수정
    IDE_TEST( lockTable( aTrans,
                         aTable,
                         (smiTableLockMode)SML_ISLOCK,
                         ID_ULONG_MAX ) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

// BUG-17477 : rp에서 필요한 함수
IDE_RC smiTable::lockTable( SInt          aSlot,
                            smlLockItem  *aLockItem,
                            smlLockMode   aLockMode,
                            ULong         aLockWaitMicroSec,
                            smlLockMode  *aCurLockMode,
                            idBool       *aLocked,
                            smlLockNode **aLockNode,
                            smlLockSlot **aLockSlot )
{

    IDE_TEST( smlLockMgr::lockTable( aSlot,
                                     aLockItem,
                                     aLockMode,
                                     aLockWaitMicroSec,
                                     aCurLockMode,
                                     aLocked,
                                     aLockNode,
                                     aLockSlot ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Table의 Flag를 변경한다.
 *
 * aStatement [IN] 작업 STATEMENT 객체
 * aTable     [IN] Flag를 변경할 Table
 * aFlagMask  [IN] 변경할 Table Flag의 Mask
 * aFlagValue [IN] 변경할 Table Flag의 Value
 **********************************************************************/
IDE_RC smiTable::alterTableFlag( smiStatement *aStatement,
                                 const void   *aTable,
                                 UInt          aFlagMask,
                                 UInt          aFlagValue )
{
    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aTable != NULL );
    IDE_DASSERT( aFlagMask != 0);
    // Mask외의 Bit를 Value가 사용해서는 안된다.
    IDE_DASSERT( (~aFlagMask & aFlagValue) == 0 );

    smxTrans * sSmxTrans = (smxTrans*)aStatement->mTrans->mTrans;

    smcTableHeader * sTableHeader =
        (smcTableHeader*)((UChar*)aTable+SMP_SLOT_HEADER_SIZE);

    IDE_TEST( smcTable::alterTableFlag( sSmxTrans,
                                        sTableHeader,
                                        aFlagMask,
                                        aFlagValue )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*******************************************************************************
 * Description : Replication을 위해서 Table Meta를 기록한다.
 *
 * [IN] aTrans      - Log Buffer를 포함한 Transaction
 * [IN] aTableMeta  - 기록할 Table Meta의 헤더
 * [IN] aLogBody    - 기록할 Log Body
 * [IN] aLogBodyLen - 기록할 Log Body의 길이
 ******************************************************************************/
IDE_RC smiTable::writeTableMetaLog(smiTrans     * aTrans,
                                   smiTableMeta * aTableMeta,
                                   const void   * aLogBody,
                                   UInt           aLogBodyLen)
{
    return smcTable::writeTableMetaLog(aTrans->getTrans(),
                                       (smrTableMeta *)aTableMeta,
                                       aLogBody,
                                       aLogBodyLen);
}


/***********************************************************************
 * Description : Table의 Insert Limit을 변경한다.
 *
 * aStatement [IN] 작업 STATEMENT 객체
 * aTable     [IN] Flag를 변경할 Table
 * aSegAttr   [IN] 변경할 Table Insert Limit
 *
 **********************************************************************/
IDE_RC smiTable::alterTableSegAttr( smiStatement *aStatement,
                                    const void   *aTable,
                                    smiSegAttr    aSegAttr )
{
    smxTrans            * sSmxTrans;
    smcTableHeader      * sTableHeader;

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aTable != NULL );

    sSmxTrans    = (smxTrans*)aStatement->mTrans->mTrans;
    sTableHeader = (smcTableHeader*)((UChar*)aTable+SMP_SLOT_HEADER_SIZE);

    IDE_TEST( aStatement->prepareDDL((smiTrans*)aStatement->mTrans)
              != IDE_SUCCESS );

    IDE_TEST( smcTable::alterTableSegAttr(
                 sSmxTrans,
                 sTableHeader,
                 aSegAttr,
                 sctTableSpaceMgr::getTBSLvType2Opt( SMI_TBSLV_DDL_DML))
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Table의 Storage Attr을 변경한다.
 *
 * aStatement [IN] 작업 STATEMENT 객체
 * aTable     [IN] Flag를 변경할 Table
 * aSegStoAttr   [IN] 변경할 Table Storage Attr
 *
 **********************************************************************/
IDE_RC smiTable::alterTableSegStoAttr( smiStatement        *aStatement,
                                       const void          *aTable,
                                       smiSegStorageAttr    aSegStoAttr )
{
    smxTrans *            sSmxTrans;
    smcTableHeader      * sTableHeader;

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aTable != NULL );

    sSmxTrans    = (smxTrans*)aStatement->mTrans->mTrans;
    sTableHeader = (smcTableHeader*)((UChar*)aTable+SMP_SLOT_HEADER_SIZE);

    IDE_TEST( aStatement->prepareDDL((smiTrans*)aStatement->mTrans)
              != IDE_SUCCESS );

    IDE_TEST( smcTable::alterTableSegStoAttr(
                 sSmxTrans,
                 sTableHeader,
                 aSegStoAttr,
                 sctTableSpaceMgr::getTBSLvType2Opt( SMI_TBSLV_DDL_DML))
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Index의 INIT/MAX TRANS를 변경한다.
 *
 * aStatement [IN] 작업 STATEMENT 객체
 * aTable     [IN] Flag를 변경할 Table
 * aSegAttr   [IN] 변경할 INDEX INIT/MAX TRANS
 *
 **********************************************************************/
IDE_RC smiTable::alterIndexSegAttr( smiStatement *aStatement,
                                    const void   *aTable,
                                    const void   *aIndex,
                                    smiSegAttr    aSegAttr )
{
    smxTrans            * sSmxTrans;
    smcTableHeader      * sTableHeader;

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aTable != NULL );

    sSmxTrans    = (smxTrans*)aStatement->mTrans->mTrans;
    sTableHeader = (smcTableHeader*)((UChar*)aTable+SMP_SLOT_HEADER_SIZE);

    IDE_TEST( aStatement->prepareDDL((smiTrans*)aStatement->mTrans)
              != IDE_SUCCESS );

    IDE_TEST( smcTable::alterIndexSegAttr(
                 sSmxTrans,
                 sTableHeader,
                 (void*)aIndex,
                 aSegAttr,
                 sctTableSpaceMgr::getTBSLvType2Opt( SMI_TBSLV_DDL_DML))
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Index 의 Storage Attr을 변경한다.
 *
 * aStatement    [IN] 작업 STATEMENT 객체
 * aTable        [IN] Flag를 변경할 Table
 * aIndex        [IN] Flag를 변경할 Index
 * aSegStoAttr   [IN] 변경할 Table Storage Attr
 *
 **********************************************************************/
IDE_RC smiTable::alterIndexSegStoAttr( smiStatement        *aStatement,
                                       const void       *   aTable,
                                       const void       *   aIndex,
                                       smiSegStorageAttr    aSegStoAttr )
{
    smxTrans *            sSmxTrans;
    smcTableHeader      * sTableHeader;

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aTable != NULL );

    sSmxTrans    = (smxTrans*)aStatement->mTrans->mTrans;
    sTableHeader = (smcTableHeader*)((UChar*)aTable+SMP_SLOT_HEADER_SIZE);

    IDE_TEST( aStatement->prepareDDL((smiTrans*)aStatement->mTrans)
              != IDE_SUCCESS );

    IDE_TEST( smcTable::alterIndexSegStoAttr(
                 sSmxTrans,
                 sTableHeader,
                 (void*)aIndex,
                 aSegStoAttr,
                 sctTableSpaceMgr::getTBSLvType2Opt( SMI_TBSLV_DDL_DML))
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smiTable::alterTableAllocExts( smiStatement* aStatement,
                                      const void*   aTable,
                                      ULong         aExtendPageSize )
{
    smxTrans *            sSmxTrans;
    smcTableHeader      * sTableHeader;

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aTable != NULL );

    sSmxTrans    = (smxTrans*)aStatement->mTrans->mTrans;
    sTableHeader = (smcTableHeader*)((UChar*)aTable+SMP_SLOT_HEADER_SIZE);

    IDE_TEST( aStatement->prepareDDL((smiTrans*)aStatement->mTrans)
              != IDE_SUCCESS );

    IDE_TEST( smcTable::alterTableAllocExts(
                 sSmxTrans->mStatistics,
                 sSmxTrans,
                 sTableHeader,
                 aExtendPageSize,
                 sctTableSpaceMgr::getTBSLvType2Opt( SMI_TBSLV_DDL_DML))
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smiTable::alterIndexAllocExts( smiStatement     *   aStatement,
                                      const void       *   aTable,
                                      const void       *   aIndex,
                                      ULong                aExtendPageSize )
{
    smxTrans            * sSmxTrans;
    smcTableHeader      * sTableHeader;

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aTable     != NULL );
    IDE_DASSERT( aIndex     != NULL );

    sSmxTrans    = (smxTrans*)aStatement->mTrans->mTrans;
    sTableHeader = (smcTableHeader*)((UChar*)aTable+SMP_SLOT_HEADER_SIZE);

    IDE_TEST( aStatement->prepareDDL((smiTrans*)aStatement->mTrans)
              != IDE_SUCCESS );

    IDE_TEST( smcTable::alterIndexAllocExts(
                 sSmxTrans->mStatistics,
                 sSmxTrans,
                 sTableHeader,
                 (void*)aIndex,
                 aExtendPageSize,
                 sctTableSpaceMgr::getTBSLvType2Opt( SMI_TBSLV_DDL_DML))
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// fix BUG-22395
IDE_RC smiTable::alterIndexName( idvSQL              *aStatistics,
                                 smiStatement     *   aStatement,
                                 const void       *   aTable,
                                 const void       *   aIndex,
                                 SChar            *   aName)
{
    smxTrans            * sSmxTrans;
    smcTableHeader      * sTableHeader;

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aTable     != NULL );
    IDE_DASSERT( aIndex     != NULL );

    sSmxTrans    = (smxTrans*)aStatement->mTrans->mTrans;
    sTableHeader = (smcTableHeader*)((UChar*)aTable+SMP_SLOT_HEADER_SIZE);

    IDE_TEST( aStatement->prepareDDL((smiTrans*)aStatement->mTrans)
              != IDE_SUCCESS );

    IDE_TEST( smcTable::alterIndexName (
                 aStatistics,
                 sSmxTrans,
                 sTableHeader,
                 (void*)aIndex,
                 aName)
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 지정된 Table를 Aging한다.
 *
 * aStatement    [IN] 작업 STATEMENT 객체
 * aTable        [IN] Aging될 Table
 *
 **********************************************************************/
IDE_RC smiTable::agingTable( smiStatement  * aStatement,
                             const void    * aTable )
{
    smxTrans         * sSmxTrans;
    smcTableHeader   * sTableHeader;
    smnIndexModule   * sIndexModule;

    IDE_ASSERT( aStatement != NULL );
    IDE_ASSERT( aTable     != NULL );

    sSmxTrans    = (smxTrans*)aStatement->mTrans->getTrans();
    sTableHeader = (smcTableHeader*)((UChar*)aTable+SMP_SLOT_HEADER_SIZE);

    IDE_TEST( aStatement->prepareDDL((smiTrans*)aStatement->mTrans)
              != IDE_SUCCESS );

    sIndexModule = &SDN_SEQUENTIAL_ITERATOR;

    IDE_TEST( sIndexModule->mAging( sSmxTrans->mStatistics,
                                    (void*)sSmxTrans,
                                    sTableHeader,
                                    NULL )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : 지정된 Index를 Aging한다.
 *
 * aStatement    [IN] 작업 STATEMENT 객체
 * aIndex        [IN] Aging될 Index
 *
 **********************************************************************/
IDE_RC smiTable::agingIndex( smiStatement  * aStatement,
                             const void    * aIndex )
{
    smxTrans         * sSmxTrans;
    smnIndexModule   * sIndexModule;

    IDE_DASSERT( aStatement != NULL );

    sSmxTrans    = (smxTrans*)aStatement->mTrans->mTrans;

    IDE_TEST( aStatement->prepareDDL((smiTrans*)aStatement->mTrans)
              != IDE_SUCCESS );

    sIndexModule = (smnIndexModule*)((smnIndexHeader*)aIndex)->mModule;

    IDE_TEST( sIndexModule->mAging( sSmxTrans->mStatistics,
                                    (void*)sSmxTrans,
                                    NULL,
                                    (smnIndexHeader*)aIndex )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : PROJ-2399 row template
 *               alter table시 변경된 컬럼 정보를 바탕으로 row template를
 *               다시 만든다.
 ***********************************************************************/
IDE_RC smiTable::modifyRowTemplate( smiStatement * aStatement,
                                    const void   * aTable )
{
    smcTableHeader * sTableHeader;
    smxTrans       * sSmxTrans;

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aTable     != NULL );

    /* 이미 DDL로 수행중인 Tx에 의해서만 불릴수 있다. */
    sSmxTrans = (smxTrans*)aStatement->mTrans->mTrans;
    IDE_ASSERT( sSmxTrans->mIsDDL == ID_TRUE );

    sTableHeader = (smcTableHeader*)SMI_MISC_TABLE_HEADER( aTable );

    IDE_TEST( smcTable::destroyRowTemplate( sTableHeader ) != IDE_SUCCESS );

    IDE_TEST( smcTable::initRowTemplate( NULL, /* aStatistics */
                                         sTableHeader,
                                         NULL ) /* aActionArg */
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : PROJ-2614 Memory Index Reorganization
 *               특정 인덱스에 대해 reorganization을 수행한다.
 ***********************************************************************/
IDE_RC smiTable::indexReorganization( void    * aHeader )
{
    IDE_TEST( smnManager::doKeyReorganization( (smnIndexHeader*)aHeader ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* BUG-45853 : partiton table swap 지원(BUG-45745) 위해서 추가.
               index header의 index id를 변경할수 있는 인터페이스 */
IDE_RC smiTable::swapIndexID( smiStatement * aStatement,
                              void         * aTable1,
                              void         * aIndex1,
                              void         * aTable2,
                              void         * aIndex2 )
{
    volatile UInt sIndex1ID;
    volatile UInt sIndex2ID;

    sIndex1ID = ((smnIndexHeader *)aIndex1)->mId;
    sIndex2ID = ((smnIndexHeader *)aIndex2)->mId;

    IDE_TEST( modifyIndexID( aStatement,
                             aTable1,
                             aIndex1,
                             sIndex2ID )
              != IDE_SUCCESS );

    IDE_TEST( modifyIndexID( aStatement,
                             aTable2,
                             aIndex2,
                             sIndex1ID )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smiTable::modifyIndexID( smiStatement * aStatement,
                                const void   * aTable,
                                void         * aIndex,
                                UInt           aIndexID )
{
    smxTrans       * sTrans;
    smcTableHeader * sTable;
    smnIndexHeader * sIndex;
    scPageID         sPageID;
    scOffset         sOffset;

    sTrans = (smxTrans *)aStatement->mTrans->mTrans;
    sTable = (smcTableHeader *)((SChar *)aTable + SMP_SLOT_HEADER_SIZE);
    sIndex = (smnIndexHeader *)aIndex;

    sPageID = SM_MAKE_PID( sIndex->mSelfOID );
    sOffset = SM_MAKE_OFFSET( sIndex->mSelfOID );
    sOffset += IDU_FT_OFFSETOF( smnIndexHeader, mId );

    IDE_TEST( smrUpdate::physicalUpdate( NULL, /* idvSQL* */
                                         (void *)sTrans,
                                         SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                         sPageID,
                                         sOffset,
                                         (SChar *)&sIndex->mId, /* OLD */
                                         ID_SIZEOF( UInt ),
                                         (SChar *)&aIndexID, /* NEW */
                                         ID_SIZEOF( UInt ),
                                         NULL,
                                         0 )
              != IDE_SUCCESS );

    ideLog::log( IDE_SM_0, "[INDEX OID %ul] Index ID is changed : %u => %u",
                 sIndex->mSelfOID, sIndex->mId, aIndexID );

    if ( ( sTable->mFlag & SMI_TABLE_TYPE_MASK ) == SMI_TABLE_DISK )
    {
        sIndex->mId = aIndexID;
        /* disk경우 runtime header의 index id도 수정 */
        ((sdnbHeader *)(sIndex->mHeader))->mIndexID = aIndexID;
    }
    else
    {
        sIndex->mId = aIndexID;
    }

    IDE_TEST( smmDirtyPageMgr::insDirtyPage( SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, sPageID )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smiTable::swapIndexColumns( smiStatement * aStatement,
                                   void         * aIndex1,
                                   void         * aIndex2 )
{
    UInt * sColumns1;
    UInt * sColumns2;
    UInt   sColumns[SMI_MAX_IDX_COLUMNS];
    UInt   i;

    sColumns1 = ((smnIndexHeader *)aIndex1)->mColumns;
    sColumns2 = ((smnIndexHeader *)aIndex2)->mColumns;

    for ( i = 0; i < ((smnIndexHeader *)aIndex1)->mColumnCount; i++ )
    {
        sColumns[i] = *( sColumns1 + i );
    }

    IDE_TEST( modifyIndexColumns( aStatement,
                                  aIndex1,
                                  sColumns2 )
              != IDE_SUCCESS );

    IDE_TEST( modifyIndexColumns( aStatement,
                                  aIndex2,
                                  sColumns )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smiTable::modifyIndexColumns( smiStatement  * aStatement,
                                     void          * aIndex,
                                     UInt          * aColumns )
{
    smxTrans       * sTrans;
    smnIndexHeader * sIndex;
    scPageID         sPageID;
    scOffset         sOffset;
    UInt             i;

    sTrans = (smxTrans *)aStatement->mTrans->mTrans;
    sIndex = (smnIndexHeader *)aIndex;

    sPageID = SM_MAKE_PID( sIndex->mSelfOID );
    sOffset = SM_MAKE_OFFSET( sIndex->mSelfOID );
    sOffset += IDU_FT_OFFSETOF( smnIndexHeader, mColumns );

    IDE_TEST( smrUpdate::physicalUpdate( NULL, /* idvSQL* */
                                         (void *)sTrans,
                                         SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                         sPageID,
                                         sOffset,
                                         (SChar *)&sIndex->mColumns, /* OLD */
                                         ID_SIZEOF( UInt ) * sIndex->mColumnCount,
                                         (SChar *)aColumns, /* NEW */
                                         ID_SIZEOF( UInt ) * sIndex->mColumnCount,
                                         NULL,
                                         0 )
              != IDE_SUCCESS );

    ideLog::log( IDE_SM_0, "[INDEX OID %ul] Index Columns is changed : %u => %u",
                 sIndex->mSelfOID, sIndex->mColumns[0], *aColumns );

    for ( i = 0; i < sIndex->mColumnCount; i++ )
    {
        sIndex->mColumns[i] = *( aColumns + i );
    }

    IDE_TEST( smmDirtyPageMgr::insDirtyPage( SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, sPageID )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

