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
 * $Id: qcmTableSpace.cpp 88963 2020-10-19 03:33:18Z jake.jang $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <idu.h>
#include <qcm.h>
#include <qcg.h>
#include <qcmTableSpace.h>
#include <qcmUser.h>
#include <qdpPrivilege.h>
#include <smiTableSpace.h>
#include <qdpRole.h>

const void * gQcmTBSUsers;
const void * gQcmTBSUsersIndex[ QCM_MAX_META_INDICES ];

/*
  Alias => Tablespace 이름 으로의 Mapping Table을 위한 Entry
*/
struct qdbTBSNameAlias
{
    SChar * mAliasName; /* Tablespace Alias */
    SChar * mTBSName;   /* 실제 Tablespace 이름 */
};

/*
  Alias => Tablespace 이름 으로의 Mapping 을 실시

  To Fix BUG-16839 기존의 시스템 Tablespace이름을
  그대로 사용할 수 있도록 지원

  시노님을 통해 1차 구현하였으나,
  QP의 컨셉에 맞지 않아서 코드 원복하고
  Tablespace이름 매핑 테이블을 간단히 구현함

  [IN] aAliasNamePtr - Alias 문자열 시작
  [OUT] aTBSNamePtr - Tablespace 이름 문자열 시작
*/
IDE_RC qcmTablespace::lookupTBSNameAlias(
    SChar         * aAliasName,
    SChar        ** aFoundTBSName )
{
    struct qdbTBSNameAlias sAliasLookup[] =
        {
            { (SChar*)"SYS_TBS_DATA",   (SChar*)"SYS_TBS_DISK_DATA" },
            { (SChar*)"SYS_TBS_MEMORY", (SChar*)"SYS_TBS_MEM_DATA" },
            { (SChar*)"SYS_TBS_UNDO",   (SChar*)"SYS_TBS_DISK_UNDO" },
            { (SChar*)"SYS_TBS_TEMP",   (SChar*)"SYS_TBS_DISK_TEMP" },
            { NULL, NULL }
        };

    UInt    i;
    SChar * sAlias ;
    SChar * sTBSName ;

    IDE_DASSERT( aAliasName    != NULL );
    IDE_DASSERT( aFoundTBSName != NULL );

    // 아무것도 못 찾을때에 대비하여
    // 기본적으로 Input을 그대로 Output에 기록
    *aFoundTBSName = aAliasName;

    for ( i=0; sAliasLookup[i].mAliasName != NULL; i++ )
    {
        sAlias   = sAliasLookup[i].mAliasName;
        sTBSName = sAliasLookup[i].mTBSName;
        if (idlOS::strCaselessMatch(
                sAlias,
                idlOS::strlen( sAlias ),
                aAliasName,
                idlOS::strlen( aAliasName ) ) == 0)
        {
            *aFoundTBSName = sTBSName;
            break;
        }
    }

    return IDE_SUCCESS ;
}


IDE_RC
qcmTablespace::getTBSAttrByName( qcStatement       * aStatement,
                                 SChar             * aName,
                                 UInt                aNameLength,
                                 smiTableSpaceAttr * aTBSAttr )
{
/***********************************************************************
 *
 * Description :
 *    주어진 Tablespace 이름을 이용하여 Tablespace 정보를 얻는다.
 *
 * Implementation :
 *    To Fix PR-10780
 *    Table Space 관련 Meta Table을 제거하여
 *    SM 으로부터 해당 정보를 획득한다.
 *
 ***********************************************************************/

    SChar         sTBSNameBuffer[SMI_MAX_TABLESPACE_NAME_LEN + 1];

    SChar       * sTBSName;

    //----------------------------------------
    // 적합성 검사
    //----------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aName != NULL );
    IDE_DASSERT( ( aNameLength > 0 ) &&
                 ( aNameLength <= SMI_MAX_TABLESPACE_NAME_LEN ) );
    IDE_DASSERT( aTBSAttr != NULL );

    //----------------------------------------
    // Storage Manager로부터 정보 획득
    //----------------------------------------

    idlOS::memcpy( sTBSNameBuffer, aName, aNameLength );
    sTBSNameBuffer[aNameLength] = '\0';

    // Tablespace Alias인 경우 이를 실제 Tablespace이름으로 변환
    IDE_TEST( lookupTBSNameAlias( sTBSNameBuffer,
                                  & sTBSName ) != IDE_SUCCESS );

    IDE_TEST( smiTableSpace::getAttrByName( sTBSName, aTBSAttr )
              != IDE_SUCCESS );

    //----------------------------------------
    // Cursor 관리를 위한 정보 설정
    // Memory 또는 Disk Tablespace 영역의 접근 여부 설정
    //----------------------------------------

    if ( QC_SHARED_TMPLATE(aStatement) != NULL )
    {
        if ( (smiTableSpace::isMemTableSpaceType( aTBSAttr->mType )  == ID_TRUE) ||
             (smiTableSpace::isVolatileTableSpaceType( aTBSAttr->mType ) == ID_TRUE) )
        {
            // Memory Table Space를 사용하는 DDL인 경우
            // Cursor Flag의 누적
            QC_SHARED_TMPLATE(aStatement)->smiStatementFlag |= SMI_STATEMENT_MEMORY_CURSOR;
        }
        else
        {
            // Disk Table Space를 사용하는 DDL인 경우
            // Cursor Flag의 누적
            QC_SHARED_TMPLATE(aStatement)->smiStatementFlag |= SMI_STATEMENT_DISK_CURSOR;
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmTablespace::getTBSAttrByID( scSpaceID           aTBSID,
                                      smiTableSpaceAttr * aTBSAttr )
{
/***********************************************************************
 *
 * Description :
 *     TBS ID를 이용하여 TBS 정보를 획득함
 *
 * Implementation :
 *     Storage Manager로부터 TBS 정보를 획득함.
 *
 ***********************************************************************/

    //-------------------------------------
    // 적합성 검사
    //-------------------------------------

    IDE_DASSERT( aTBSAttr != NULL );

    //-------------------------------------
    // Storage Manager로부터 정보를 획득함
    //-------------------------------------

    IDE_TEST( smiTableSpace::getAttrByID( aTBSID, aTBSAttr )
              != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qcmTablespace::existDataFileInTBS( scSpaceID   aTBSID,
                                   SChar     * aName,
                                   UInt        aNameLength,
                                   idBool    * aExist )
{
/***********************************************************************
 *
 * Description :
 *    지정된 TBS 내에 동일한 Data File이 존재하는 지 검사
 *
 * Implementation :
 *    Storage Manager로부터 존재 여부를 확인한다.
 *
 ***********************************************************************/

    SChar sDataFileName[SMI_MAX_DATAFILE_NAME_LEN + 1];

    //---------------------------------------
    // 적합성 검사
    //---------------------------------------

    IDE_DASSERT( aName != NULL );
    IDE_DASSERT( ( aNameLength > 0 ) &&
                 ( aNameLength <= SMI_MAX_DATAFILE_NAME_LEN ) );
    IDE_DASSERT( aExist != NULL );

    //---------------------------------------
    // Storage Manager로부터 존재 여부 확인
    //---------------------------------------

    idlOS::memcpy( sDataFileName, aName, aNameLength );
    sDataFileName[aNameLength] = '\0';

    IDE_TEST( smiTableSpace::existDataFile( aTBSID, sDataFileName, aExist )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qcmTablespace::existDataFileInDB( SChar     * aName,
                                  UInt        aNameLength,
                                  idBool    * aExist )
{
/***********************************************************************
 *
 * Description :
 *    전체 DB 내에 동일한 Data File이 존재하는 지 검사
 *
 * Implementation :
 *    Storage Manager로부터 존재 여부를 확인한다.
 *
 ***********************************************************************/

    SChar sDataFileName[SMI_MAX_DATAFILE_NAME_LEN + 1];

    //---------------------------------------
    // 적합성 검사
    //---------------------------------------

    IDE_DASSERT( aName != NULL );
    IDE_DASSERT( ( aNameLength > 0 ) &&
                 ( aNameLength <= SMI_MAX_DATAFILE_NAME_LEN ) );
    IDE_DASSERT( aExist != NULL );

    //---------------------------------------
    // Storage Manager로부터 존재 여부 확인
    //---------------------------------------

    idlOS::memcpy( sDataFileName, aName, aNameLength );
    sDataFileName[aNameLength] = '\0';

    IDE_TEST( smiTableSpace::existDataFile( sDataFileName, aExist )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmTablespace::existObject(qcStatement      *aStatement,
                                  scSpaceID         aTBSID,
                                  idBool           *aExist)
{
/***********************************************************************
 *
 * Description :
 *    명시한 테이블스페이스에 속해 있는 객체가 있는지 체크한다
 *
 * Implementation :
 *    1. SYS_TABLES_ 체크
 *    2. SYS_TABLE_PARTITIONS_ 체크
 *    3. SYS_LOBS_ 체크
 *    4. SYS_INDICES_ 체크
 *    5. SYS_INDEX_PARTITIONS_ 체크
 *
 ***********************************************************************/

    SInt                    sStage = 0;
    smiRange                sRange;
    mtcColumn             * sColumn;
    qtcMetaRangeColumn      sRangeColumn;
    const void            * sRow;
    smiTableCursor          sCursor;
    smiCursorProperties     sCursorProperty;
    scGRID                  sRid; // Disk Table을 위한 Record IDentifier
    mtdIntegerType          sIntDataOfTBSID = (mtdIntegerType) aTBSID;

    *aExist = ID_FALSE;

    sCursor.initialize();

    //-----------------------------------
    // search SYS_TABLES_
    //-----------------------------------
    IDE_TEST( smiGetTableColumns( gQcmTables,
                                  QCM_TABLES_TBS_ID_COL_ORDER,
                                  (const smiColumn**)&sColumn )
              != IDE_SUCCESS );

    qcm::makeMetaRangeSingleColumn( &sRangeColumn,
                                    (const mtcColumn*)sColumn,
                                    (const void *) &(sIntDataOfTBSID),
                                    &sRange );

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN( &sCursorProperty, aStatement->mStatistics );
    
    IDE_TEST(sCursor.open(
                 QC_SMI_STMT( aStatement ),
                 gQcmTables,
                 gQcmTablesIndex[QCM_TABLES_TBSID_IDX_ORDER],
                 smiGetRowSCN(gQcmTables),
                 NULL,
                 &sRange,
                 smiGetDefaultKeyRange(),
                 smiGetDefaultFilter(),
                 QCM_META_CURSOR_FLAG,
                 SMI_SELECT_CURSOR,
                 & sCursorProperty) != IDE_SUCCESS);

    sStage = 1;
    IDE_TEST(sCursor.beforeFirst() != IDE_SUCCESS);

    IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);
    if (sRow != NULL)
    {
        *aExist = ID_TRUE;
    }
    sStage = 0;

    IDE_TEST(sCursor.close() != IDE_SUCCESS);

    //-----------------------------------
    // PROJ-1502 PARTITIONED DISK TABLE
    // search SYS_TABLE_PARTITIONS_
    //-----------------------------------
    if (*aExist != ID_TRUE)
    {
        IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                      QCM_TABLE_PARTITIONS_TBS_ID_COL_ORDER,
                                      (const smiColumn**)&sColumn )
                  != IDE_SUCCESS );

        qcm::makeMetaRangeSingleColumn( &sRangeColumn,
                                        (const mtcColumn*)sColumn,
                                        (const void *) &(sIntDataOfTBSID),
                                        &sRange );

        IDE_TEST(sCursor.open(
                     QC_SMI_STMT(aStatement),
                     gQcmTablePartitions,
                     gQcmTablePartitionsIndex[QCM_TABLE_PARTITIONS_IDX4_ORDER],
                     smiGetRowSCN(gQcmTablePartitions),
                     NULL,
                     &sRange,
                     smiGetDefaultKeyRange(),
                     smiGetDefaultFilter(),
                     QCM_META_CURSOR_FLAG,
                     SMI_SELECT_CURSOR,
                     & sCursorProperty) != IDE_SUCCESS);

        sStage = 1;
        IDE_TEST(sCursor.beforeFirst() != IDE_SUCCESS);

        IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);
        if (sRow != NULL)
        {
            *aExist = ID_TRUE;
        }
        sStage = 0;

        IDE_TEST(sCursor.close() != IDE_SUCCESS);
    }


    //-----------------------------------
    // PROJ-1362
    // search SYS_LOBS_
    //-----------------------------------
    if (*aExist != ID_TRUE)
    {
        IDE_TEST( smiGetTableColumns( gQcmLobs,
                                      QCM_LOBS_TBS_ID_COL_ORDER,
                                      (const smiColumn**)&sColumn )
                  != IDE_SUCCESS );

        qcm::makeMetaRangeSingleColumn( &sRangeColumn,
                                        (const mtcColumn*)sColumn,
                                        (const void *) &(sIntDataOfTBSID),
                                        &sRange );

        IDE_TEST(sCursor.open(
                     QC_SMI_STMT(aStatement),
                     gQcmLobs,
                     gQcmLobsIndex[QCM_LOBS_TBSID_IDX_ORDER],
                     smiGetRowSCN(gQcmLobs),
                     NULL,
                     &sRange,
                     smiGetDefaultKeyRange(),
                     smiGetDefaultFilter(),
                     QCM_META_CURSOR_FLAG,
                     SMI_SELECT_CURSOR,
                     & sCursorProperty) != IDE_SUCCESS);

        sStage = 1;
        IDE_TEST(sCursor.beforeFirst() != IDE_SUCCESS);

        IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);
        if (sRow != NULL)
        {
            *aExist = ID_TRUE;
        }
        sStage = 0;

        IDE_TEST(sCursor.close() != IDE_SUCCESS);
    }

    //-----------------------------------
    // search SYS_INDICES_
    //-----------------------------------
    if (*aExist != ID_TRUE)
    {
        IDE_TEST( smiGetTableColumns( gQcmIndices,
                                      QCM_INDICES_TBSID_COL_ORDER,
                                      (const smiColumn**)&sColumn )
                  != IDE_SUCCESS );

        qcm::makeMetaRangeSingleColumn( &sRangeColumn,
                                        (const mtcColumn*)sColumn,
                                        (const void *) &(sIntDataOfTBSID),
                                        &sRange );

        IDE_TEST(sCursor.open(
                     QC_SMI_STMT( aStatement ),
                     gQcmIndices,
                     gQcmIndicesIndex[QCM_INDICES_TBSID_IDX_ORDER],
                     smiGetRowSCN(gQcmIndices),
                     NULL,
                     &sRange,
                     smiGetDefaultKeyRange(),
                     smiGetDefaultFilter(),
                     QCM_META_CURSOR_FLAG,
                     SMI_SELECT_CURSOR,
                     & sCursorProperty) != IDE_SUCCESS);

        sStage = 1;
        IDE_TEST(sCursor.beforeFirst() != IDE_SUCCESS);

        IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);

        if (sRow != NULL)
        {
            *aExist = ID_TRUE;
        }
        sStage = 0;

        IDE_TEST(sCursor.close() != IDE_SUCCESS);
    }

    //-----------------------------------
    // PROJ-1502 PARTITIONED DISK TABLE
    // search SYS_INDEX_PARTITIONS_
    //-----------------------------------
    if (*aExist != ID_TRUE)
    {
        IDE_TEST( smiGetTableColumns( gQcmIndexPartitions,
                                      QCM_INDEX_PARTITIONS_TBS_ID_COL_ORDER,
                                      (const smiColumn**)&sColumn )
                  != IDE_SUCCESS );

        qcm::makeMetaRangeSingleColumn( &sRangeColumn,
                                        (const mtcColumn*)sColumn,
                                        (const void *) &(sIntDataOfTBSID),
                                        &sRange );

        IDE_TEST(sCursor.open(
                     QC_SMI_STMT( aStatement ),
                     gQcmIndexPartitions,
                     gQcmIndexPartitionsIndex[QCM_INDEX_PARTITIONS_IDX3_ORDER],
                     smiGetRowSCN(gQcmIndexPartitions),
                     NULL,
                     &sRange,
                     smiGetDefaultKeyRange(),
                     smiGetDefaultFilter(),
                     QCM_META_CURSOR_FLAG,
                     SMI_SELECT_CURSOR,
                     & sCursorProperty) != IDE_SUCCESS);

        sStage = 1;
        IDE_TEST(sCursor.beforeFirst() != IDE_SUCCESS);

        IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);

        if (sRow != NULL)
        {
            *aExist = ID_TRUE;
        }
        sStage = 0;

        IDE_TEST(sCursor.close() != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch(sStage)
    {
        case 1:
            sCursor.close();
            break;
    }

    return IDE_FAILURE;
}

IDE_RC qcmTablespace::findTableInfoListInTBS(
    qcStatement      * aStatement,
    scSpaceID          aTBSID,
    idBool             aIsForValidation,
    qcmTableInfoList **aTableInfoList)
{
/***********************************************************************
 *
 * Description :
 *    qdtDrop::validate, qdtDrop::execute 로부터 호출, drop 가능한
 *    테이블의 리스트를 돌려준다.
 *
 *    smiValidateAndLockObjects 에서 TBS에 대해서는
 *    SMI_TBSLV_DROP_TBS옵션으로 처리하기 때문에
 *    Drop 된 TBS이외에 ONLINE/OFFLINE/DISCARDED 상태의 TBS에
 *    대해서도 잠금을 획득할 수 있다.
 *
 * Implementation :
 *
 ***********************************************************************/

    smiRange                sRange;
    mtcColumn             * sColumn;
    mtcColumn             * sTableOidCol;
    mtcColumn             * sTableIdCol;
    mtcColumn             * sTableTypeCol;
    mtcColumn             * sHiddenCol;
    qtcMetaRangeColumn      sRangeColumn;
    SInt                    sStage = 0;
    const void            * sRow;
    smiTableCursor          sCursor;
    UInt                    sTableID;
    qcmTableInfo          * sTableInfo;
    smSCN                   sSCN;
    smiTableLockMode        sLockMode          = SMI_TABLE_LOCK_IS;
    void                  * sTableHandle;
    qcmTableInfoList      * sTableInfoList = NULL;
    qcmTableInfoList      * sTempTableInfoList = NULL;
    qcmTableInfoList      * sCurrTableInfoList;
    smiCursorProperties     sCursorProperty;
    scGRID                  sRid; // Disk Table을 위한 Record IDentifier
    idBool                  sFound;
    mtdIntegerType          sIntDataOfTBSID = (mtdIntegerType) aTBSID;

    UChar                 * sTableType;
    mtdCharType           * sTableTypeStr;
    UChar                 * sHidden;
    mtdCharType           * sHiddenStr;

    sCursor.initialize();

    *aTableInfoList = NULL;

    if( aIsForValidation == ID_TRUE )
    {
        /* Nothing to do */
    }
    else
    {
        sLockMode = SMI_TABLE_LOCK_X;
    }

    //-----------------------------------
    // search SYS_TABLES_
    //-----------------------------------
    IDE_TEST( smiGetTableColumns( gQcmTables,
                                  QCM_TABLES_TBS_ID_COL_ORDER,
                                  (const smiColumn**)&sColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( gQcmTables,
                                  QCM_TABLES_TABLE_OID_COL_ORDER,
                                  (const smiColumn**)&sTableOidCol )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( gQcmTables,
                                  QCM_TABLES_TABLE_ID_COL_ORDER,
                                  (const smiColumn**)&sTableIdCol )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( gQcmTables,
                                  QCM_TABLES_TABLE_TYPE_COL_ORDER,
                                  (const smiColumn**)&sTableTypeCol )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( gQcmTables,
                                  QCM_TABLES_HIDDEN_COL_ORDER,
                                  (const smiColumn**)&sHiddenCol )
              != IDE_SUCCESS );

    qcm::makeMetaRangeSingleColumn( &sRangeColumn,
                                    (const mtcColumn*)sColumn,
                                    (const void *) &(sIntDataOfTBSID),
                                    &sRange );

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN( &sCursorProperty, aStatement->mStatistics );

    IDE_TEST(sCursor.open(
                 QC_SMI_STMT( aStatement ),
                 gQcmTables,
                 gQcmTablesIndex[QCM_TABLES_TBSID_IDX_ORDER],
                 smiGetRowSCN(gQcmTables),
                 NULL,
                 &sRange,
                 smiGetDefaultKeyRange(),
                 smiGetDefaultFilter(),
                 QCM_META_CURSOR_FLAG,
                 SMI_SELECT_CURSOR,
                 & sCursorProperty) != IDE_SUCCESS);

    sStage = 1;
    IDE_TEST(sCursor.beforeFirst() != IDE_SUCCESS);

    IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);

    while (sRow != NULL)
    {
        sTableID = *(UInt *) ((UChar*)sRow +
                              sTableIdCol->column.offset);

        sTableTypeStr = (mtdCharType*) ((UChar*) sRow +
                                        sTableTypeCol->column.offset);
        sTableType = sTableTypeStr->value;

        sHiddenStr = (mtdCharType*)
            ((UChar*) sRow + sHiddenCol->column.offset);
        sHidden = sHiddenStr->value;

        // PROJ-1624 Global Non-partitioned Index
        // hidden table은 hidden table을 소유한 객체가 직접처리한다.
        // if we want to get the table is sequence or not
        /* BUG-35460 Add TABLE_TYPE G in SYS_TABLES_ */
        if ( ( ( idlOS::strMatch( (SChar*)sTableType, sTableTypeStr->length, "T", 1 ) == 0 ) ||
               ( idlOS::strMatch( (SChar*)sTableType, sTableTypeStr->length, "V", 1 ) == 0 ) ||
               ( idlOS::strMatch( (SChar*)sTableType, sTableTypeStr->length, "M", 1 ) == 0 ) ||
               ( idlOS::strMatch( (SChar*)sTableType, sTableTypeStr->length, "A", 1 ) == 0 ) ||
               ( idlOS::strMatch( (SChar*)sTableType, sTableTypeStr->length, "Q", 1 ) == 0 ) ||
               ( idlOS::strMatch( (SChar*)sTableType, sTableTypeStr->length, "G", 1 ) == 0 ) ||
               ( idlOS::strMatch( (SChar*)sTableType, sTableTypeStr->length, "D", 1 ) == 0 ) ||
               ( idlOS::strMatch( (SChar*)sTableType, sTableTypeStr->length, "R", 1 ) == 0 ) ) /* PROJ-2441 flashback */
             &&
             ( idlOS::strMatch( (SChar*)sHidden, sHiddenStr->length, "N", 1 ) == 0 ) )
        {
            IDE_TEST(qcm::getTableInfoByID(aStatement,
                                           sTableID,
                                           &sTableInfo,
                                           &sSCN,
                                           &sTableHandle) != IDE_SUCCESS);

            // fix BUG-17285  Disk Tablespace 를 OFFLINE/DISCARD 후 DROP시
            // 에러발생
            // 테이블의 테이블스페이스가 OFFLINE/ONLINE/DISCARDED 상태인
            // 경우에도 Lock을 획득할 수 있도록 한다.
            IDE_TEST(smiValidateAndLockObjects( (QC_SMI_STMT( aStatement ))->getTrans(),
                                                sTableHandle,
                                                sSCN,
                                                SMI_TBSLV_DROP_TBS, // TBS Validation 옵션
                                                sLockMode,
                                                smiGetDDLLockTimeOut((QC_SMI_STMT(aStatement))->getTrans()),
                                                ID_FALSE ) // BUG-28752 명시적 Lock과 내재적 Lock을 구분합니다.
                     != IDE_SUCCESS);

            IDE_TEST_RAISE(sTableInfo->replicationCount > 0,
                           ERR_DDL_WITH_REPLICATED_TABLE);
            //proj-1608:replicationCount가 0일 때 recovery count는 항상 0이어야 함
            IDE_DASSERT(sTableInfo->replicationRecoveryCount == 0);

            if (sTableInfoList == NULL)
            {
                if( aIsForValidation == ID_TRUE )
                {
                    IDU_FIT_POINT( "qcmTablespace::findTableInfoListInTBS::malloc::sTableInfoList1",
                                    idERR_ABORT_InsufficientMemory );

                    IDE_TEST(QC_QMP_MEM(aStatement)->alloc(ID_SIZEOF(qcmTableInfoList),
                                                           (void**)&(sTableInfoList))
                             != IDE_SUCCESS);
                }
                else
                {
                    IDU_FIT_POINT( "qcmTablespace::findTableInfoListInTBS::malloc::sTableInfoList2",
                                    idERR_ABORT_InsufficientMemory );

                    IDE_TEST(QC_QMX_MEM(aStatement)->alloc(ID_SIZEOF(qcmTableInfoList),
                                                           (void**)&(sTableInfoList))
                             != IDE_SUCCESS);
                }

                sTableInfoList->tableInfo = sTableInfo;
                sTableInfoList->childInfo = NULL;
                sTableInfoList->next = NULL;
                sTempTableInfoList = sTableInfoList;
                *aTableInfoList = sTableInfoList;
            }
            else
            {
                if( aIsForValidation == ID_TRUE )
                {
                    IDU_FIT_POINT( "qcmTablespace::findTableInfoListInTBS::malloc::sTableInfoList3",
                                    idERR_ABORT_InsufficientMemory );

                    IDE_TEST(QC_QMP_MEM(aStatement)->alloc(ID_SIZEOF(qcmTableInfoList),
                                                           (void**)&(sTableInfoList))
                             != IDE_SUCCESS);
                }
                else
                {
                    IDU_FIT_POINT( "qcmTablespace::findTableInfoListInTBS::malloc::sTableInfoList4",
                                    idERR_ABORT_InsufficientMemory );

                    IDE_TEST(QC_QMX_MEM(aStatement)->alloc(ID_SIZEOF(qcmTableInfoList),
                                                           (void**)&(sTableInfoList))
                             != IDE_SUCCESS);
                }

                sTableInfoList->tableInfo = sTableInfo;
                sTableInfoList->childInfo = NULL;
                sTableInfoList->next = NULL;
                sTempTableInfoList->next = sTableInfoList;
                sTempTableInfoList = sTableInfoList;
            }
        }
        else
        {
            // Nothing to do.
        }

        IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);
    }

    sStage = 0;
    IDE_TEST(sCursor.close() != IDE_SUCCESS);

    //-----------------------------------
    // PROJ-1362
    // search SYS_LOBS_
    //-----------------------------------
    IDE_TEST( smiGetTableColumns( gQcmLobs,
                                  QCM_LOBS_TBS_ID_COL_ORDER,
                                  (const smiColumn**)&sColumn )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( gQcmLobs,
                                  QCM_LOBS_TABLE_ID_COL_ORDER,
                                  (const smiColumn**)&sTableIdCol )
              != IDE_SUCCESS );

    qcm::makeMetaRangeSingleColumn( &sRangeColumn,
                                    (const mtcColumn*)sColumn,
                                    (const void *) &(sIntDataOfTBSID),
                                    &sRange );

    IDE_TEST(sCursor.open(
                 QC_SMI_STMT(aStatement),
                 gQcmLobs,
                 gQcmLobsIndex[QCM_LOBS_TBSID_IDX_ORDER],
                 smiGetRowSCN(gQcmLobs),
                 NULL,
                 &sRange,
                 smiGetDefaultKeyRange(),
                 smiGetDefaultFilter(),
                 QCM_META_CURSOR_FLAG,
                 SMI_SELECT_CURSOR,
                 & sCursorProperty) != IDE_SUCCESS);

    sStage = 1;
    IDE_TEST(sCursor.beforeFirst() != IDE_SUCCESS);

    IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);

    while (sRow != NULL)
    {
        sTableID = *(UInt *) ((UChar*)sRow +
                              sTableIdCol->column.offset);

        IDE_TEST(qcm::getTableInfoByID(aStatement,
                                       sTableID,
                                       &sTableInfo,
                                       &sSCN,
                                       &sTableHandle) != IDE_SUCCESS);

        // 중복을 제거한다.
        sFound = ID_FALSE;

        if (*aTableInfoList != NULL)
        {
            sCurrTableInfoList = *aTableInfoList;

            while (sCurrTableInfoList != NULL)
            {
                if (sTableInfo == sCurrTableInfoList->tableInfo)
                {
                    sFound = ID_TRUE;
                    break;
                }
                else
                {
                    // Nothing to do.
                }

                sCurrTableInfoList = sCurrTableInfoList->next;
            }
        }
        else
        {
            // Nothing to do.
        }

        if (sFound == ID_FALSE)
        {
            IDE_TEST(smiValidateAndLockObjects( (QC_SMI_STMT( aStatement ))->getTrans(),
                                                sTableHandle,
                                                sSCN,
                                                SMI_TBSLV_DROP_TBS, // TBS Validation 옵션
                                                sLockMode,
                                                smiGetDDLLockTimeOut((QC_SMI_STMT(aStatement))->getTrans()),
                                                ID_FALSE ) // BUG-28752 명시적 Lock과 내재적 Lock을 구분합니다.
                     != IDE_SUCCESS);

            IDE_TEST_RAISE(sTableInfo->replicationCount > 0,
                           ERR_DDL_WITH_REPLICATED_TABLE);
            //proj-1608:replicationCount가 0일 때 recovery count는 항상 0이어야 함
            IDE_DASSERT(sTableInfo->replicationRecoveryCount == 0);

            if (sTableInfoList == NULL)
            {
                if( aIsForValidation == ID_TRUE )
                {
                    IDU_FIT_POINT( "qcmTablespace::findTableInfoListInTBS::malloc::sTableInfoList5",
                                    idERR_ABORT_InsufficientMemory );

                    IDE_TEST(QC_QMP_MEM(aStatement)->alloc(ID_SIZEOF(qcmTableInfoList),
                                                           (void**)&(sTableInfoList))
                             != IDE_SUCCESS);
                }
                else
                {
                    IDU_FIT_POINT( "qcmTablespace::findTableInfoListInTBS::malloc::sTableInfoList6",
                                    idERR_ABORT_InsufficientMemory );

                    IDE_TEST(QC_QMX_MEM(aStatement)->alloc(ID_SIZEOF(qcmTableInfoList),
                                                           (void**)&(sTableInfoList))
                             != IDE_SUCCESS);
                }

                sTableInfoList->tableInfo = sTableInfo;
                sTableInfoList->childInfo = NULL;
                sTableInfoList->next = NULL;
                sTempTableInfoList = sTableInfoList;
                *aTableInfoList = sTableInfoList;
            }
            else
            {
                if( aIsForValidation == ID_TRUE )
                {
                    IDU_FIT_POINT( "qcmTablespace::findTableInfoListInTBS::malloc::sTableInfoList7",
                                    idERR_ABORT_InsufficientMemory );

                    IDE_TEST(QC_QMP_MEM(aStatement)->alloc(ID_SIZEOF(qcmTableInfoList),
                                                           (void**)&(sTableInfoList))
                             != IDE_SUCCESS);
                }
                else
                {
                    IDU_FIT_POINT( "qcmTablespace::findTableInfoListInTBS::malloc::sTableInfoList8", 
                                    idERR_ABORT_InsufficientMemory );

                    IDE_TEST(QC_QMX_MEM(aStatement)->alloc(ID_SIZEOF(qcmTableInfoList),
                                                           (void**)&(sTableInfoList))
                             != IDE_SUCCESS);
                }

                sTableInfoList->tableInfo = sTableInfo;
                sTableInfoList->childInfo = NULL;
                sTableInfoList->next = NULL;
                sTempTableInfoList->next = sTableInfoList;
                sTempTableInfoList = sTableInfoList;
            }
        }
        else
        {
            // Nothing to do.
        }

        IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);
    }

    sStage = 0;
    IDE_TEST(sCursor.close() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_DDL_WITH_REPLICATED_TABLE)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDD_DDL_WITH_REPLICATED_TBL));
    }
    IDE_EXCEPTION_END;

    switch(sStage)
    {
        case 1:
            sCursor.close();
            break;
    }

    return IDE_FAILURE;
}

static IDE_RC allocQcmTableInfoList(qcStatement       *aStatement,
                                    qcmTableInfo      *aTablePartInfo,
                                    qcmTableInfoList **aTablePartInfoListElem)
{
    qcmTableInfoList *sTablePartInfoList = NULL;

    IDU_LIMITPOINT("qcmTablespace::allocQcmTableInfoList::malloc");

    IDE_TEST(QC_QMP_MEM(aStatement)->alloc(ID_SIZEOF(qcmTableInfoList),
                                           (void **)&sTablePartInfoList)
             != IDE_SUCCESS);

    sTablePartInfoList->tableInfo = aTablePartInfo;
    sTablePartInfoList->childInfo = NULL;
    sTablePartInfoList->next      = NULL;

    *aTablePartInfoListElem = sTablePartInfoList;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC findTablePartInfoListInTBSInternal(qcStatement       *aStatement,
                                                 scSpaceID          aTBSID,
                                                 const void        *aPartition,
                                                 const void        *aPartitionIndex,
                                                 mtcColumn         *aTbsIdColumn,
                                                 mtcColumn         *aTablePartIdCol,
                                                 qcmTableInfoList **aTablePartInfoList)
{
    smiRange                sRange;
    qtcMetaRangeColumn      sRangeColumn;
    SInt                    sStage = 0;
    const void            * sRow;
    smiTableCursor          sCursor;
    UInt                    sTablePartID;
    qcmTableInfo          * sTablePartInfo;
    smSCN                   sSCN;
    void                  * sTablePartHandle;

    smiCursorProperties     sCursorProperty;
    scGRID                  sRid; // Disk Table을 위한 Record IDentifier
    mtdIntegerType          sIntDataOfTBSID = (mtdIntegerType) aTBSID;

    qcmTableInfoList      * sTablePartInfoList     = NULL;
    qcmTableInfoList      * sLastNode              = NULL;
    qcmTableInfoList      * sFirstNode             = NULL;

    sCursor.initialize();

    qcm::makeMetaRangeSingleColumn( &sRangeColumn,
                                    (const mtcColumn*)aTbsIdColumn,
                                    (const void *) &(sIntDataOfTBSID),
                                    &sRange );

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN( &sCursorProperty, aStatement->mStatistics );
    
    IDE_TEST(sCursor.open(
                 QC_SMI_STMT( aStatement ),
                 aPartition,
                 aPartitionIndex,
                 smiGetRowSCN(aPartition),
                 NULL,
                 &sRange,
                 smiGetDefaultKeyRange(),
                 smiGetDefaultFilter(),
                 QCM_META_CURSOR_FLAG,
                 SMI_SELECT_CURSOR,
                 & sCursorProperty) != IDE_SUCCESS);

    sStage = 1;
    IDE_TEST(sCursor.beforeFirst() != IDE_SUCCESS);

    IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);

    /*
     * 전달되어져 온 list 의 뒤에서부터 추가하기 위해서, current position 을
     * 리스트의 맨 뒤로 옮긴다.
     */
    sLastNode = *aTablePartInfoList;
    if (sLastNode != NULL)
    {
        sFirstNode = sLastNode;

        while (sLastNode->next != NULL)
        {
            sLastNode = sLastNode->next;
        }
    }
    else
    {
        sFirstNode = NULL;
    }

    while (sRow != NULL)
    {
        sTablePartID = *(UInt *)((UChar*)sRow + aTablePartIdCol->column.offset);

        IDE_TEST(qcmPartition::getPartitionInfoByID(
                     aStatement,
                     sTablePartID,
                     &sTablePartInfo,
                     &sSCN,
                     &sTablePartHandle) != IDE_SUCCESS);

        IDE_TEST( qcmPartition::validateAndLockOnePartition( aStatement,
                                                             sTablePartHandle,
                                                             sSCN,
                                                             SMI_TBSLV_DROP_TBS, // TBS Validation 옵션
                                                             SMI_TABLE_LOCK_IS,
                                                             smiGetDDLLockTimeOut((QC_SMI_STMT(aStatement))->getTrans()))
                  != IDE_SUCCESS );

        IDE_TEST(allocQcmTableInfoList(aStatement,
                                       sTablePartInfo,
                                       &sTablePartInfoList) != IDE_SUCCESS);

        /*
         * aTablePartInfoList 리스트에 추가한다.
         */
        if (sFirstNode == NULL)
        {
            sFirstNode = sTablePartInfoList;
            sLastNode  = sFirstNode;
        }
        else
        {
            sLastNode->next = sTablePartInfoList;
            sLastNode       = sTablePartInfoList;
        }

        IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);
    }

    sStage = 0;
    IDE_TEST(sCursor.close() != IDE_SUCCESS);

    *aTablePartInfoList = sFirstNode;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch (sStage)
    {
        case 1:
            sCursor.close();
            break;
    }

    return IDE_FAILURE;
}

IDE_RC qcmTablespace::findTablePartInfoListInTBS(
    qcStatement      * aStatement,
    scSpaceID          aTBSID,
    qcmTableInfoList **aTablePartInfoList)
{
/***********************************************************************
 *
 * Description :
 *    qdtDrop::validate 로부터 호출, drop 가능한 파티션의
 *    리스트를 돌려준다.
 *
 *    smiValidateAndLockObjects 에서 TBS에 대해서는
 *    SMI_TBSLV_DROP_TBS옵션으로 처리하기 때문에
 *    Drop 된 TBS이외에 ONLINE/OFFLINE/DISCARDED 상태의 TBS에
 *    대해서도 잠금을 획득할 수 있다.
 *
 * Implementation :
 *
 ***********************************************************************/
    mtcColumn        *sTbsIdCol;
    mtcColumn        *sTablePartIdCol;
    qcmTableInfoList *sTablePartInfoList = NULL;

    //-----------------------------------
    // search SYS_TABLE_PARTITIONS_
    //-----------------------------------
    IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                  QCM_TABLE_PARTITIONS_TBS_ID_COL_ORDER,
                                  (const smiColumn**)&sTbsIdCol )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( gQcmTablePartitions,
                                  QCM_TABLE_PARTITIONS_PARTITION_ID_COL_ORDER,
                                  (const smiColumn**)&sTablePartIdCol )
              != IDE_SUCCESS );
    IDE_TEST(findTablePartInfoListInTBSInternal(aStatement,
                                                aTBSID,
                                                gQcmTablePartitions,
                                                gQcmTablePartitionsIndex[QCM_TABLE_PARTITIONS_IDX4_ORDER],
                                                sTbsIdCol,
                                                sTablePartIdCol,
                                                &sTablePartInfoList) != IDE_SUCCESS);

    //-----------------------------------
    // search SYS_PART_LOBS_
    //-----------------------------------
    IDE_TEST( smiGetTableColumns( gQcmPartLobs,
                                  QCM_PART_LOBS_TBS_ID_COL_ORDER,
                                  (const smiColumn**)&sTbsIdCol )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( gQcmPartLobs,
                                  QCM_PART_LOBS_PARTITION_ID_COL_ORDER,
                                  (const smiColumn**)&sTablePartIdCol )
              != IDE_SUCCESS );
    IDE_TEST(findTablePartInfoListInTBSInternal(aStatement,
                                                aTBSID,
                                                gQcmPartLobs,
                                                gQcmPartLobsIndex[QCM_PART_LOBS_IDX2_ORDER],
                                                sTbsIdCol,
                                                sTablePartIdCol,
                                                &sTablePartInfoList) != IDE_SUCCESS);


    *aTablePartInfoList = sTablePartInfoList;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmTablespace::findIndexInfoListInTBS(
    qcStatement       *aStatement,
    scSpaceID          aTBSID,
    idBool             aIsForValidation,
    qcmIndexInfoList **aIndexInfoList)
{
/***********************************************************************
 *
 * Description :
 *    명시한 테이블스페이스에 속해 있는 인덱스를 찾아서 그 ID 와 테이블의
 *    ID 를 리스트로  반환한다.
 *    이 때 인덱스가 걸려있는 테이블은 다른 테이블스페이스에 존재해야 한다.
 *
 *    smiValidateAndLockObjects 에서 TBS에 대해서는
 *    SMI_TBSLV_DROP_TBS옵션으로 처리하기 때문에
 *    Drop 된 TBS이외에 ONLINE/OFFLINE/DISCARDED 상태의 TBS에
 *    대해서도 잠금을 획득할 수 있다.
 *
 * Implementation :
 *
 ***********************************************************************/

    SInt                    sStage = 0;
    UInt                    sTableID;
    qcmTableInfo          * sTableInfo;
    smSCN                   sSCN;
    smiTableLockMode        sLockMode          = SMI_TABLE_LOCK_IS;
    smiRange                sRange;
    mtcColumn             * sTBSIDCol;
    mtcColumn             * sIndexIDCol;
    mtcColumn             * sTableIDCol;
    mtcColumn             * sIsPartitionedCol;
    mtcColumn             * sIndexTableIDCol;
    mtdCharType           * sIsPartitionedStr;
    UInt                    sIndexTableID;
    qtcMetaRangeColumn      sRangeColumn;
    const void            * sRow;
    smiTableCursor          sCursor;
    smiCursorProperties     sCursorProperty;
    scGRID                  sRid; // Disk Table을 위한 Record IDentifier
    qcmIndexInfoList      * sFirstIndexInfoList = NULL;
    qcmIndexInfoList      * sIndexInfoList;
    void                  * sTableHandle;
    mtdIntegerType          sIntDataOfTBSID = (mtdIntegerType) aTBSID;
    qdIndexTableList      * sIndexTable;

    sCursor.initialize();

    if( aIsForValidation == ID_TRUE )
    {
        /* Nothing to do */
    }
    else
    {
        sLockMode = SMI_TABLE_LOCK_X;
    }

    //-----------------------------------
    // search SYS_INDEX_PARTITIONS_
    //-----------------------------------
    IDE_TEST( smiGetTableColumns( gQcmIndices,
                                  QCM_INDICES_TBSID_COL_ORDER,
                                  (const smiColumn**)&sTBSIDCol )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( gQcmIndices,
                                  QCM_INDICES_INDEX_ID_COL_ORDER,
                                  (const smiColumn**)&sIndexIDCol )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( gQcmIndices,
                                  QCM_INDICES_TABLE_ID_COL_ORDER,
                                  (const smiColumn**)&sTableIDCol )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( gQcmIndices,
                                  QCM_INDICES_IS_PARTITIONED_COL_ORDER,
                                  (const smiColumn**)&sIsPartitionedCol )
              != IDE_SUCCESS );
    IDE_TEST( smiGetTableColumns( gQcmIndices,
                                  QCM_INDICES_INDEX_TABLE_ID_COL_ORDER,
                                  (const smiColumn**)&sIndexTableIDCol )
              != IDE_SUCCESS );
    
    qcm::makeMetaRangeSingleColumn( &sRangeColumn,
                                    (const mtcColumn*)sTBSIDCol,
                                    (const void *) &(sIntDataOfTBSID),
                                    &sRange );

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN( &sCursorProperty, aStatement->mStatistics );
    
    IDE_TEST(sCursor.open(
                 QC_SMI_STMT( aStatement ),
                 gQcmIndices,
                 gQcmIndicesIndex[QCM_INDICES_TBSID_IDX_ORDER],
                 smiGetRowSCN(gQcmIndices),
                 NULL,
                 &sRange,
                 smiGetDefaultKeyRange(),
                 smiGetDefaultFilter(),
                 QCM_META_CURSOR_FLAG,
                 SMI_SELECT_CURSOR,
                 & sCursorProperty) != IDE_SUCCESS);

    sStage = 1;
    IDE_TEST(sCursor.beforeFirst() != IDE_SUCCESS);

    IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);

    while (sRow != NULL)
    {
        sTableID = *(UInt *) ((UChar*)sRow +
                              sTableIDCol->column.offset);
        sIsPartitionedStr = (mtdCharType *) ((UChar *)sRow +
                                             sIsPartitionedCol->column.offset);
        sIndexTableID = *(UInt*) ((UChar*)sRow +
                                  sIndexTableIDCol->column.offset);

        IDE_TEST(qcm::getTableInfoByID(aStatement,
                                       sTableID,
                                       &sTableInfo,
                                       &sSCN,
                                       &sTableHandle) != IDE_SUCCESS);

        IDE_TEST(smiValidateAndLockObjects( (QC_SMI_STMT( aStatement ))->getTrans(),
                                            sTableHandle,
                                            sSCN,
                                            SMI_TBSLV_DROP_TBS, // TBS Validation 옵션
                                            sLockMode,
                                            smiGetDDLLockTimeOut((QC_SMI_STMT(aStatement))->getTrans()),
                                            ID_FALSE ) // BUG-28752 명시적 Lock과 내재적 Lock을 구분합니다.
                 != IDE_SUCCESS);

        if (sTableInfo->TBSID != aTBSID)
        {
            // BUG-16565
            IDE_TEST_RAISE(sTableInfo->replicationCount > 0,
                           ERR_DDL_WITH_REPLICATED_TABLE);

            //proj-1608:replicationCount가 0일 때 recovery count는 항상 0이어야 함
            IDE_DASSERT(sTableInfo->replicationRecoveryCount == 0);

            if( aIsForValidation == ID_TRUE )
            {
                IDU_FIT_POINT( "qcmTablespace::findIndexInfoListInTBS::cralloc::sIndexInfoList1",
                                idERR_ABORT_InsufficientMemory );

                IDE_TEST(QC_QMP_MEM(aStatement)->cralloc(ID_SIZEOF(qcmIndexInfoList),
                                                         (void**)&(sIndexInfoList))
                         != IDE_SUCCESS);
            }
            else
            {
                IDU_FIT_POINT( "qcmTablespace::findIndexInfoListInTBS::cralloc::sIndexIndexList2",
                                idERR_ABORT_InsufficientMemory );

                IDE_TEST(QC_QMX_MEM(aStatement)->cralloc(ID_SIZEOF(qcmIndexInfoList),
                                                         (void**)&(sIndexInfoList))
                         != IDE_SUCCESS);
            }

            sIndexInfoList->indexID = *(UInt *) ((UChar*)sRow +
                                                 sIndexIDCol->column.offset);
            sIndexInfoList->tableInfo = sTableInfo;
            sIndexInfoList->childInfo = NULL;
            sIndexInfoList->indexTable.tableID = sIndexTableID;
            
            if ( idlOS::strMatch( (SChar *)&(sIsPartitionedStr->value),
                                  sIsPartitionedStr->length,
                                  "T",
                                  1 ) == 0 )
            {
                sIndexInfoList->isPartitionedIndex = ID_TRUE;
            }
            else
            {
                sIndexInfoList->isPartitionedIndex = ID_FALSE;
            }

            if ( sTableInfo->tablePartitionType == QCM_PARTITIONED_TABLE )
            {
                if ( sIndexInfoList->isPartitionedIndex == ID_TRUE )
                {
                    // partitioned index는 index table을 가질 수 없다.
                    IDE_TEST_RAISE( sIndexTableID != 0, ERR_META_CRASH );
                }
                else
                {
                    // non-partitioned index라면 index table이 있어야한다.
                    IDE_TEST_RAISE( sIndexTableID == 0, ERR_META_CRASH );

                    sIndexTable = & sIndexInfoList->indexTable;

                    sIndexTable->tableID = sIndexTableID;
                    
                    IDE_TEST( qcm::getTableInfoByID( aStatement,
                                                     sIndexTableID,
                                                     &sIndexTable->tableInfo,
                                                     &sIndexTable->tableSCN,
                                                     &sIndexTable->tableHandle )
                              != IDE_SUCCESS );

                    // validation lock
                    IDE_TEST( smiValidateAndLockObjects( (QC_SMI_STMT( aStatement ))->getTrans(),
                                                         sIndexTable->tableHandle,
                                                         sIndexTable->tableSCN,
                                                         SMI_TBSLV_DROP_TBS, // TBS Validation 옵션
                                                         sLockMode,
                                                         smiGetDDLLockTimeOut((QC_SMI_STMT(aStatement))->getTrans()),
                                                         ID_FALSE ) // BUG-28752 명시적 Lock과 내재적 Lock을 구분합니다.
                              != IDE_SUCCESS );

                    sIndexTable->tableOID = smiGetTableId(sIndexTable->tableHandle);
                    sIndexTable->next     = NULL;
                }
            }
            else
            {
                // non-partitioned table은 partitioned index를 가질 수 없다.
                IDE_TEST_RAISE( sIndexInfoList->isPartitionedIndex == ID_TRUE,
                                ERR_META_CRASH );

                // non-partitioned table은 index table을 가질 수 없다.
                IDE_TEST_RAISE( sIndexTableID != 0, ERR_META_CRASH );
            }
            
            if (sFirstIndexInfoList == NULL)
            {
                sIndexInfoList->next = NULL;
                sFirstIndexInfoList  = sIndexInfoList;
            }
            else
            {
                sIndexInfoList->next = sFirstIndexInfoList;
                sFirstIndexInfoList  = sIndexInfoList;
            }
        }

        IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);
    }

    sStage = 0;
    IDE_TEST(sCursor.close() != IDE_SUCCESS);

    *aIndexInfoList = sFirstIndexInfoList;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_DDL_WITH_REPLICATED_TABLE)
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDD_DDL_WITH_REPLICATED_TBL));
    }
    IDE_EXCEPTION(ERR_META_CRASH);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCM_META_CRASH));
    }
    IDE_EXCEPTION_END;

    switch(sStage)
    {
        case 1:
            sCursor.close();
            break;
    }

    return IDE_FAILURE;
}

IDE_RC qcmTablespace::findIndexPartInfoListInTBS(
    qcStatement       *aStatement,
    scSpaceID          aTBSID,
    qcmIndexInfoList **aIndexPartInfoList)
{
/***********************************************************************
 *
 * Description :
 *    명시한 테이블스페이스에 속해 있는 인덱스 파티션을 찾아서 그 ID 와
 *    테이블의 ID 를 리스트로  반환한다.
 *
 *    smiValidateAndLockObjects 에서 TBS에 대해서는
 *    SMI_TBSLV_DROP_TBS옵션으로 처리하기 때문에
 *    Drop 된 TBS이외에 ONLINE/OFFLINE/DISCARDED 상태의 TBS에
 *    대해서도 잠금을 획득할 수 있다.
 *
 * Implementation :
 *
 ***********************************************************************/

    SInt                    sStage = 0;
    UInt                    sTableID;
    qcmTableInfo          * sTableInfo;
    smSCN                   sSCN;
    smiRange                sRange;
    mtcColumn             * sTBSIDCol;
    mtcColumn             * sIndexIDCol;
    mtcColumn             * sTableIDCol;
    qtcMetaRangeColumn      sRangeColumn;
    const void            * sRow;
    smiTableCursor          sCursor;
    smiCursorProperties     sCursorProperty;
    scGRID                  sRid; // Disk Table을 위한 Record IDentifier
    qcmIndexInfoList      * sFirstIndexPartInfoList = NULL;
    qcmIndexInfoList      * sIndexPartInfoList;
    void                  * sTableHandle;
    mtdIntegerType          sIntDataOfTBSID = (mtdIntegerType) aTBSID;

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN( &sCursorProperty, aStatement->mStatistics );
    
    sCursor.initialize();

    //-----------------------------------
    // search SYS_INDICES_
    //-----------------------------------
    IDE_TEST( smiGetTableColumns( gQcmIndexPartitions,
                                  QCM_INDEX_PARTITIONS_TBS_ID_COL_ORDER,
                                  (const smiColumn**)&sTBSIDCol )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmIndexPartitions,
                                  QCM_INDEX_PARTITIONS_INDEX_ID_COL_ORDER,
                                  (const smiColumn**)&sIndexIDCol )
              != IDE_SUCCESS );

    IDE_TEST( smiGetTableColumns( gQcmIndexPartitions,
                                  QCM_INDEX_PARTITIONS_TABLE_ID_COL_ORDER,
                                  (const smiColumn**)&sTableIDCol )
              != IDE_SUCCESS );

    qcm::makeMetaRangeSingleColumn( &sRangeColumn,
                                    (const mtcColumn*)sTBSIDCol,
                                    (const void *) &(sIntDataOfTBSID),
                                    &sRange );

    IDE_TEST(sCursor.open(
                 QC_SMI_STMT( aStatement ),
                 gQcmIndexPartitions,
                 gQcmIndexPartitionsIndex[QCM_INDEX_PARTITIONS_IDX3_ORDER],
                 smiGetRowSCN(gQcmIndexPartitions),
                 NULL,
                 &sRange,
                 smiGetDefaultKeyRange(),
                 smiGetDefaultFilter(),
                 QCM_META_CURSOR_FLAG,
                 SMI_SELECT_CURSOR,
                 & sCursorProperty) != IDE_SUCCESS);

    sStage = 1;
    IDE_TEST(sCursor.beforeFirst() != IDE_SUCCESS);

    IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);

    while (sRow != NULL)
    {
        sTableID = *(UInt *) ((UChar*)sRow +
                              sTableIDCol->column.offset);

        IDE_TEST(qcm::getTableInfoByID(aStatement,
                                       sTableID,
                                       &sTableInfo,
                                       &sSCN,
                                       &sTableHandle) != IDE_SUCCESS);

        IDE_TEST(smiValidateAndLockObjects( (QC_SMI_STMT( aStatement ))->getTrans(),
                                            sTableHandle,
                                            sSCN,
                                            SMI_TBSLV_DROP_TBS, // TBS Validation 옵션
                                            SMI_TABLE_LOCK_IS,
                                            smiGetDDLLockTimeOut((QC_SMI_STMT(aStatement))->getTrans()),
                                            ID_FALSE ) // BUG-28752 명시적 Lock과 내재적 Lock을 구분합니다.
                 != IDE_SUCCESS);

        IDU_FIT_POINT( "qcmTablespace::findIndexPartInfoListInTBS::cralloc::sIndexPartInfoList",
                        idERR_ABORT_InsufficientMemory );

        IDE_TEST(QC_QMP_MEM(aStatement)->cralloc(ID_SIZEOF(qcmIndexInfoList),
                                                 (void**)&(sIndexPartInfoList))
                 != IDE_SUCCESS);

        sIndexPartInfoList->indexID = *(UInt *) ((UChar*)sRow +
                                                 sIndexIDCol->column.offset);
        sIndexPartInfoList->tableInfo = sTableInfo;
        sIndexPartInfoList->childInfo = NULL;
        sIndexPartInfoList->isPartitionedIndex = ID_FALSE;

        if (sFirstIndexPartInfoList == NULL)
        {
            sIndexPartInfoList->next = NULL;
            sFirstIndexPartInfoList  = sIndexPartInfoList;
        }
        else
        {
            sIndexPartInfoList->next = sFirstIndexPartInfoList;
            sFirstIndexPartInfoList  = sIndexPartInfoList;
        }

        IDE_TEST(sCursor.readRow(&sRow, &sRid, SMI_FIND_NEXT) != IDE_SUCCESS);
    }

    sStage = 0;
    IDE_TEST(sCursor.close() != IDE_SUCCESS);

    *aIndexPartInfoList = sFirstIndexPartInfoList;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch(sStage)
    {
        case 1:
            sCursor.close();
            break;
    }

    return IDE_FAILURE;
}

IDE_RC qcmTablespace::checkAccessTBS(
    qcStatement       *aStatement,
    UInt               aUserID,
    scSpaceID          aTBSID)
{
/***********************************************************************
 *
 * Description :
 *    명시한 테이블스페이스에 user 가 접근할 수 있는지 검사한다.
 *    접근할 수 없으면 에러를 셋팅하고 IDE_FAILURE 를 반환한다.
 *
 * Implementation :
 *    1. aUserID 가 system privilege 를 갖고 있으면 IDE_SUCCESS 를 반환.
 *    2. 검사시에 user 의 default TBS 와 temp TBS 도 비교 대상에 포함해야 한다.
 *    3. SYS_TBS_USERS_ 에 레코드가  존재하지 않으면 접근 불가
 *
 ***********************************************************************/

    idBool                  sIsAccess;
    smiRange                sRange;
    scSpaceID               sDefaultTBSID   = 0; // SC_NULL_SPACEID
    scSpaceID               sTempTBSID      = 0; // SC_NULL_SPACEID
    mtcColumn             * sUserIDCol;
    mtcColumn             * sTBSIDCol;
    mtdIntegerType          sIntDataOfUserID;
    mtdIntegerType          sIntDataOfTBSID;
    mtdIntegerType          sIntDataOfIsAccess;
    qtcMetaRangeColumn      sFirstMetaRange;
    qtcMetaRangeColumn      sSecondMetaRange;
    smiTableCursor          sCursor;
    const void            * sRow = NULL;
    SInt                    sStage = 0;
    smiCursorProperties     sCursorProperty;
    scGRID                  sRid; // Disk Table을 위한 Record IDentifier
    mtcColumn             * sIsAccessMtcColumn;

    if ( gQcmTBSUsers == NULL )
    {
        // on createdb
        // nothing to do
    }
    else
    {
        // 1. check sysdba privilege
        IDE_TEST( qdpRole::checkAccessAnyTBSPriv(
                      aStatement,
                      aUserID,
                      &sIsAccess) != IDE_SUCCESS );

        if ( sIsAccess != ID_TRUE )
        {
            // 2. get default_tbs_id, temp_tbs_id
            IDE_TEST( qcmUser::getDefaultTBS(
                          aStatement,
                          aUserID,
                          &sDefaultTBSID,
                          &sTempTBSID) != IDE_SUCCESS );

            if ( aTBSID != sDefaultTBSID &&
                 aTBSID != sTempTBSID )
            {
                // 3. SYS_TBS_USERS_ 에 레코드가  존재하지 않으면 접근 불가
                sCursor.initialize();


                // set data of USER_ID, TBS_ID
                sIntDataOfTBSID = (mtdIntegerType) aTBSID;
                sIntDataOfUserID = (mtdIntegerType) aUserID;

                IDE_TEST( smiGetTableColumns( gQcmTBSUsers,
                                              QCM_TBS_USERS_TBS_ID_COL_ORDER,
                                              (const smiColumn**)&sTBSIDCol )
                          != IDE_SUCCESS );

                // mtdModule 설정
                IDE_TEST( mtd::moduleById( &sTBSIDCol->module,
                                           sTBSIDCol->type.dataTypeId )
                          != IDE_SUCCESS );

                // mtlModule 설정
                IDE_TEST( mtl::moduleById( &sTBSIDCol->language,
                                           sTBSIDCol->type.languageId )
                          != IDE_SUCCESS );

                IDE_TEST( smiGetTableColumns( gQcmTBSUsers,
                                              QCM_TBS_USERS_USER_ID_COL_ORDER,
                                              (const smiColumn**)&sUserIDCol )
                          != IDE_SUCCESS );

                // mtdModule 설정
                IDE_TEST( mtd::moduleById( &sUserIDCol->module,
                                           sUserIDCol->type.dataTypeId )
                          != IDE_SUCCESS );

                // mtlModule 설정
                IDE_TEST( mtl::moduleById( &sUserIDCol->language,
                                           sUserIDCol->type.languageId )
                          != IDE_SUCCESS );


                qcm::makeMetaRangeDoubleColumn( &sFirstMetaRange,
                                                &sSecondMetaRange,
                                                sTBSIDCol,
                                                &sIntDataOfTBSID,
                                                sUserIDCol,
                                                &sIntDataOfUserID,
                                                &sRange );

                SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN( &sCursorProperty, aStatement->mStatistics );

                IDE_TEST( sCursor.open(
                              QC_SMI_STMT( aStatement ),
                              gQcmTBSUsers,
                              gQcmTBSUsersIndex[QCM_TBS_USERS_TBSID_IDX_ORDER],
                              smiGetRowSCN(gQcmTBSUsers),
                              NULL,
                              & sRange,
                              smiGetDefaultKeyRange(),
                              smiGetDefaultFilter(),
                              QCM_META_CURSOR_FLAG,
                              SMI_SELECT_CURSOR,
                              & sCursorProperty )
                          != IDE_SUCCESS );
                sStage = 1;

                IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );
                IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT )
                          != IDE_SUCCESS );
                
                IDE_TEST_RAISE ( sRow == NULL, ERR_NO_ACCESS_TBS );

                IDE_TEST( smiGetTableColumns( gQcmTBSUsers,
                                              QCM_TBS_USERS_IS_ACCESS_COL_ORDER,
                                              (const smiColumn**)&sIsAccessMtcColumn )
                          != IDE_SUCCESS );

                sIntDataOfIsAccess = *(mtdIntegerType *)
                    ( (UChar*)sRow + sIsAccessMtcColumn->column.offset );

                IDE_TEST_RAISE( (UInt)sIntDataOfIsAccess == 0,
                                ERR_NO_ACCESS_TBS );

                sStage = 0;
                IDE_TEST( sCursor.close() != IDE_SUCCESS );
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NO_ACCESS_TBS);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDT_NO_ACCESS_TBS));
    }
    IDE_EXCEPTION_END;

    if ( sStage != 0 )
    {
        sCursor.close();
    }
    
    return IDE_FAILURE;
}

IDE_RC qcmTablespace::existAccessTBS(
    smiStatement      * aSmiStmt,
    scSpaceID           aTBSID,
    UInt                aUserID,
    idBool            * aExist)
{
/***********************************************************************
 *
 * Description :
 *    SYS_TBS_USERS_ 에 해당 user_id, tbs_id 가 존재하는지 검사한다.
 *    레코드의 존재여부는 aExist 에 반환한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    smiRange                sRange;
    mtcColumn             * sUserIDCol;
    mtcColumn             * sTBSIDCol;
    mtdIntegerType          sIntDataOfUserID;
    mtdIntegerType          sIntDataOfTBSID;
    qtcMetaRangeColumn      sFirstMetaRange;
    qtcMetaRangeColumn      sSecondMetaRange;
    smiTableCursor          sCursor;
    const void            * sRow = NULL;
    SInt                    sStage = 0;
    smiCursorProperties     sCursorProperty;
    scGRID                  sRid; // Disk Table을 위한 Record IDentifier

    if ( gQcmTBSUsers == NULL )
    {
        // on createdb
        *aExist = ID_FALSE;
    }
    else
    {
        sCursor.initialize();


        // set data of USER_ID, TBS_ID
        sIntDataOfTBSID  = (mtdIntegerType) aTBSID;
        sIntDataOfUserID = (mtdIntegerType) aUserID;
        IDE_TEST( smiGetTableColumns( gQcmTBSUsers,
                                      QCM_TBS_USERS_TBS_ID_COL_ORDER,
                                      (const smiColumn**)&sTBSIDCol )
                  != IDE_SUCCESS );

        // mtdModule 설정
        IDE_TEST( mtd::moduleById( &sTBSIDCol->module,
                                   sTBSIDCol->type.dataTypeId )
                  != IDE_SUCCESS );

        // mtlModule 설정
        IDE_TEST( mtl::moduleById( &sTBSIDCol->language,
                                   sTBSIDCol->type.languageId )
                  != IDE_SUCCESS );

        IDE_TEST( smiGetTableColumns( gQcmTBSUsers,
                                      QCM_TBS_USERS_USER_ID_COL_ORDER,
                                      (const smiColumn**)&sUserIDCol )
                  != IDE_SUCCESS );

        // mtdModule 설정
        IDE_TEST( mtd::moduleById( &sUserIDCol->module,
                                   sUserIDCol->type.dataTypeId )
                  != IDE_SUCCESS );

        // mtlModule 설정
        IDE_TEST( mtl::moduleById( &sUserIDCol->language,
                                   sUserIDCol->type.languageId )
                  != IDE_SUCCESS );

        qcm::makeMetaRangeDoubleColumn( &sFirstMetaRange,
                                        &sSecondMetaRange,
                                        sTBSIDCol,
                                        &sIntDataOfTBSID,
                                        sUserIDCol,
                                        &sIntDataOfUserID,
                                        &sRange );

        SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN( &sCursorProperty, NULL );
    
        IDE_TEST( sCursor.open(
                      aSmiStmt,
                      gQcmTBSUsers,
                      gQcmTBSUsersIndex[QCM_TBS_USERS_TBSID_IDX_ORDER],
                      smiGetRowSCN(gQcmTBSUsers),
                      NULL,
                      & sRange,
                      smiGetDefaultKeyRange(),
                      smiGetDefaultFilter(),
                      QCM_META_CURSOR_FLAG,
                      SMI_SELECT_CURSOR,
                      & sCursorProperty )
                  != IDE_SUCCESS );
        sStage = 1;

        IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );
        IDE_TEST( sCursor.readRow( &sRow, &sRid, SMI_FIND_NEXT ) != IDE_SUCCESS );
        if ( sRow != NULL )
        {
            *aExist = ID_TRUE;
        }
        else
        {
            *aExist = ID_FALSE;
        }

        sStage = 0;
        IDE_TEST( sCursor.close() != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sStage != 0 )
    {
        sCursor.close();
    }
    
    return IDE_FAILURE;
}
