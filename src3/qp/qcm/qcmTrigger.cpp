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
 * $Id: qcmTrigger.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *
 *     [PROJ-1359] Trigger
 *
 *     Trigger를 위한 Meta 및 Cache 관리를 위한 함수
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <idl.h>
#include <smiDef.h>
#include <qdParseTree.h>
#include <qsvEnv.h>
#include <qcmProc.h>
#include <qcmTrigger.h>
#include <qdnTrigger.h>
#include <qcg.h>

// PROJ-1359 Table Handles for Trigger
const void * gQcmTriggers;
const void * gQcmTriggerStrings;
const void * gQcmTriggerUpdateColumns;
const void * gQcmTriggerDmlTables;


// PROJ-1359 Index Handles for Trigger
const void * gQcmTriggersIndex              [QCM_MAX_META_INDICES];
const void * gQcmTriggerStringsIndex        [QCM_MAX_META_INDICES];
const void * gQcmTriggerUpdateColumnsIndex  [QCM_MAX_META_INDICES];
const void * gQcmTriggerDmlTablesIndex      [QCM_MAX_META_INDICES];

IDE_RC
qcmTrigger::getTriggerOID( qcStatement    * aStatement,
                           UInt             aUserID,
                           qcNamePosition   aTriggerName,
                           smOID          * aTriggerOID,
                           UInt           * aTableID,
                           idBool         * aIsExist )
{
/***********************************************************************
 *
 * Description :
 *    해당 Trigger의 ID를 얻음.
 *
 * Implementation :
 *
 *    다음의 질의를 통해 TRIGGER_ID를 획득함.
 *
 *    SELECT TRIGGER_ID FROM SYSTEM_.SYS_TRIGGERS_
 *    WHERE USER_ID = aUserID AND TRIGGER_NAME = aTriggerName;
 *
 ***********************************************************************/

    smiRange              sRange;
    qtcMetaRangeColumn    sFirstMetaRange;
    qtcMetaRangeColumn    sSecondMetaRange;
    mtdIntegerType        sUserID;
    qcNameCharBuffer      sTriggerNameBuffer;
    mtdCharType         * sTriggerName =
        ( mtdCharType * ) & sTriggerNameBuffer;
    smiTableCursor        sCursor;
    idBool                sIsCursorOpen = ID_FALSE;

    scGRID                sRID;
    smiCursorProperties   sCursorProperty;
    void                * sRow;
    mtcColumn           * sFirstMtcColumn;
    mtcColumn           * sSceondMtcColumn;
    mtcColumn           * sTriggerOIDMtcColumn;
    mtcColumn           * sTableIDMtcColumn;

    //-------------------------------------------
    // 적합성 검사
    //-------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( QC_IS_NULL_NAME( aTriggerName ) != ID_TRUE );
    IDE_DASSERT( aTriggerOID != NULL );
    IDE_DASSERT( aTableID != NULL );
    IDE_DASSERT( aIsExist != NULL );

    //-------------------------------------------
    // WHERE USER_ID = aUserID AND TRIGGER_NAME = aTriggerName;
    // 검색을 위한 Key Range 생성
    //-------------------------------------------

    // SYS_TRIGGERS_ meta_table의 column정보를 획득

    // C Type의 값을 DB Type으로 변경
    sUserID = (mtdIntegerType) aUserID;
    qtc::setVarcharValue( sTriggerName,
                          NULL,
                          aTriggerName.stmtText + aTriggerName.offset,
                          aTriggerName.size );

    // 두 개의 Column을 이용한 Key Range 생성
    IDE_TEST( smiGetTableColumns( gQcmTriggers,
                                  QCM_TRIGGERS_USER_ID,
                                  (const smiColumn**)&sFirstMtcColumn )
              != IDE_SUCCESS );      // USER_ID

    IDE_TEST( smiGetTableColumns( gQcmTriggers,
                                  QCM_TRIGGERS_TRIGGER_NAME,
                                  (const smiColumn**)&sSceondMtcColumn )
              != IDE_SUCCESS ); // TRIGGER_NAME

    qcm::makeMetaRangeDoubleColumn(
        & sFirstMetaRange,
        & sSecondMetaRange,
        sFirstMtcColumn,
        (const void *) & sUserID,
        sSceondMtcColumn,
        (const void *) sTriggerName,
        & sRange );

    //-------------------------------------------
    // Data 검색
    //-------------------------------------------

    // Cursor의 초기화
    sCursor.initialize();

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN(&sCursorProperty, NULL);

    // INDEX (USER_ID, TRIGGER_NAME)을 이용한 Cursor Open
    IDE_TEST(
        sCursor.open( QC_SMI_STMT( aStatement ),     // smiStatement
                      gQcmTriggers,                  // Table Handle
                      gQcmTriggersIndex[QCM_TRIGGERS_USERID_TRIGGERNAME_INDEX],
                      smiGetRowSCN( gQcmTriggers ),  // Table SCN
                      NULL,                          // Update Column정보
                      & sRange,                      // Key Range
                      smiGetDefaultKeyRange(),       // Key Filter
                      smiGetDefaultFilter(),         // Filter
                      QCM_META_CURSOR_FLAG,
                      SMI_SELECT_CURSOR,
                      &sCursorProperty)            // Cursor Property
        != IDE_SUCCESS );

    sIsCursorOpen = ID_TRUE;

    //-------------------------------------------
    // Trigger ID의 획득
    //-------------------------------------------

    // Data를 Read함.
    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );
    IDE_TEST( sCursor.readRow( (const void **) & sRow, & sRID, SMI_FIND_NEXT )
              != IDE_SUCCESS );

    if ( sRow != NULL )
    {
        // Trigger ID를 얻음
        // To Fix PR-10648
        // smOID 의 획득 방법이 잘못됨.
        // *aTriggerOID = *(smOID *)
        //    ( (UChar*)sRow+sColumn[QCM_TRIGGERS_TRIGGER_OID].column.offset );
        IDE_TEST( smiGetTableColumns( gQcmTriggers,
                                      QCM_TRIGGERS_TRIGGER_OID,
                                      (const smiColumn**)&sTriggerOIDMtcColumn )
                  != IDE_SUCCESS );
        *aTriggerOID = (smOID) *(ULong *)
            ( (UChar*)sRow + sTriggerOIDMtcColumn->column.offset );

        IDE_TEST( smiGetTableColumns( gQcmTriggers,
                                      QCM_TRIGGERS_TABLE_ID,
                                      (const smiColumn**)&sTableIDMtcColumn )
                  != IDE_SUCCESS );
        *aTableID = *(UInt*)
            ( (UChar*)sRow + sTableIDMtcColumn->column.offset );
        *aIsExist = ID_TRUE;
    }
    else
    {
        // 동일한 Trigger가 없음
        *aIsExist = ID_FALSE;
    }

    sIsCursorOpen = ID_FALSE;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sIsCursorOpen == ID_TRUE )
    {
        (void) sCursor.close();
    }

    return IDE_FAILURE;
}


IDE_RC
qcmTrigger::checkTriggerCycle( qcStatement * aStatement,
                               UInt          aTableID,
                               UInt          aCycleTableID )
{
/***********************************************************************
 *
 * Description :
 *
 *    Meta Cache에의 연속적 접근은 동시성 제어를 필요로 하게 되므로
 *    Action Body가 접근하는 Table 정보를 기록하고 있는
 *    SYS_TRIGGER_DML_TABLES_ 를 이용하여 Cycle을 검사한다.
 *
 * Implementation :
 *
 *    aTableID의 Trigger가 접근하는 Table중 aCycleTableID가
 *    존재할 경우 Cycle이 존재함.
 *
 *    SELECT DML_TABLE_ID FROM SYS_TRIGGER_DML_TABLES_
 *           WHERE TABLE_ID = aTableID;
 *
 ***********************************************************************/

    smiRange              sRange;
    qtcMetaRangeColumn    sFirstMetaRange;
    mtcColumn            *sDmlTableIDCol;
    smiTableCursor        sCursor;
    idBool                sIsCursorOpen;
    mtdIntegerType        sTableID;
    SInt                  sDMLTableID;
    scGRID                sRID;
    smiCursorProperties   sCursorProperty;
    void                * sRow;
    mtcColumn           * sFirstMtcColumn;

    //---------------------------------------
    // 적합성 검사
    //---------------------------------------

    IDE_DASSERT( aStatement != NULL );

    //---------------------------------------
    // 해당 Table이 소유한 Trigger가 접근하는 TABLE중에
    // Subject Table이 존재하는 지를 검사한다.
    //---------------------------------------

    sIsCursorOpen = ID_FALSE;

    //-------------------------------------------
    // WHERE TABLE_ID = aTableID;
    // 검색을 위한 Key Range 생성
    //-------------------------------------------



    // C Type의 값을 DB Type으로 변경
    sTableID = (mtdIntegerType) aTableID;

    // 하나의 Column을 이용한 Key Range 생성
    IDE_TEST( smiGetTableColumns( gQcmTriggerDmlTables,
                                  QCM_TRIGGER_DML_TABLES_TABLE_ID,
                                  (const smiColumn**)&sFirstMtcColumn )
              != IDE_SUCCESS );

    qcm::makeMetaRangeSingleColumn(
        & sFirstMetaRange,
        sFirstMtcColumn,
        (const void *) & sTableID,
        & sRange );

    //-------------------------------------------
    // Data 검색
    //-------------------------------------------

    // Cursor의 초기화
    sCursor.initialize();

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN(&sCursorProperty, NULL);

    // INDEX (USER_ID, TRIGGER_NAME)을 이용한 Cursor Open
    IDE_TEST( sCursor.open(
                  QC_SMI_STMT( aStatement ),  // smiStatement
                  gQcmTriggerDmlTables,          // Table Handle
                  gQcmTriggerDmlTablesIndex[QCM_TRIGGER_DML_TABLES_TABLE_ID_INDEX],
                  smiGetRowSCN( gQcmTriggerDmlTables ),  // Table SCN
                  NULL,                          // Update Column정보
                  & sRange,                      // Key Range
                  smiGetDefaultKeyRange(),       // Key Filter
                  smiGetDefaultFilter(),         // Filter
                  QCM_META_CURSOR_FLAG,
                  SMI_SELECT_CURSOR,
                  &sCursorProperty)            // Cursor Property
              != IDE_SUCCESS );

    sIsCursorOpen = ID_TRUE;

    //-------------------------------------------
    //
    //-------------------------------------------

    // Data를 Read함.
    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );
    IDE_TEST( sCursor.readRow( (const void **) & sRow, & sRID, SMI_FIND_NEXT )
              != IDE_SUCCESS );
    // SYS_TRIGGER_DML_TABLES_ meta_table의 column정보를 획득
    IDE_TEST( smiGetTableColumns( gQcmTriggerDmlTables,
                                  QCM_TRIGGER_DML_TABLES_DML_TABLE_ID,
                                  (const smiColumn**)&sDmlTableIDCol )
              != IDE_SUCCESS );

    while ( sRow != NULL )
    {
        // DML_TABLE_ID 를 얻음
        sDMLTableID = *(SInt *)
            ( (UChar*)sRow +
              sDmlTableIDCol->column.offset );

        // Cycle의 존재 여부 검사
        IDE_TEST_RAISE( (UInt)sDMLTableID == aCycleTableID,
                        err_cycle_detected );

        // 얻은 DML_TABLE_ID로부터 추가적인 Cycle이 존재하는 지를 검사
        //IDE_TEST ( checkTriggerCycle( aStatement, sDMLTableID, aCycleTableID )
        //           != IDE_SUCCESS );

        // 다음 Record Read
        IDE_TEST( sCursor.readRow( (const void **) & sRow,
                                   & sRID,
                                   SMI_FIND_NEXT )
                  != IDE_SUCCESS );
    }

    sIsCursorOpen = ID_FALSE;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_cycle_detected);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_TRIGGER_CYCLE_DETECTED));
    }

    IDE_EXCEPTION_END;

    if ( sIsCursorOpen == ID_TRUE )
    {
        (void) sCursor.close();
    }

    return IDE_FAILURE;
}


IDE_RC
qcmTrigger::addMetaInfo( qcStatement * aStatement,
                         void        * aTriggerHandle )
{
/***********************************************************************
 *
 * Description :
 *
 *    Trigger 생성과 관련된 모든 Meta Table에 정보를 삽입한다.
 *
 * Implementation :
 *
 *
 ***********************************************************************/

    // SYS_TRIGGERS_ 를 위한 관리 변수
    smOID           sTriggerOID;
    SInt            sSubStringCnt;
    qdnTriggerRef * sRef;
    SInt            sRefRowCnt;

    // SYS_TRIGGER_UPDATE_COLUMNS_를 위한 관리 변수
    SInt          sUptColumnCnt;
    qcmColumn *   sUptColumn;

    // SYS_TRIGGER_STRINGS_ 를 위한 관리 변수
    SChar         sSubString[QCM_TRIGGER_SUBSTRING_LEN * 2 + 2];
    SInt          sCurrPos;
    SInt          sCurrLen;
    SInt          sSeqNo;

    // SYS_TRIGGER_DML_TABLES_ 를 위한 관리 변수
    qsModifiedTable  * sCheckTable;
    qsModifiedTable  * sSameTable;

    // 공통 정보
    vSLong         sRowCnt;
    SChar        * sBuffer;
    qdnCreateTriggerParseTree * sParseTree;

    // PROJ-1579 NCHAR
    UInt            sAddSize = 0;
    SChar         * sStmtBuffer = NULL;
    UInt            sStmtBufferLen = 0;
    qcNamePosList * sNcharList = NULL;
    qcNamePosList * sTempNamePosList = NULL;
    qcNamePosition  sNamePos;
    UInt            sBufferSize = 0;

    //---------------------------------------
    // 적합성 검사
    //---------------------------------------

    IDE_DASSERT( aStatement != NULL );

    //---------------------------------------
    // String을 위한 공간 획득
    //---------------------------------------

    IDU_LIMITPOINT("qcmTrigger::addMetaInfo::malloc1");
    IDE_TEST( aStatement->qmxMem->alloc( QD_MAX_SQL_LENGTH,
                                         (void**) & sBuffer )
              != IDE_SUCCESS );

    //---------------------------------------
    // 기본 정보의 추출
    //---------------------------------------

    sParseTree = (qdnCreateTriggerParseTree *) aStatement->myPlan->parseTree;

    sTriggerOID = smiGetTableId(aTriggerHandle);

    for ( sUptColumn = sParseTree->triggerEvent.eventTypeList->updateColumns,
              sUptColumnCnt = 0;
          sUptColumn != NULL;
          sUptColumn = sUptColumn->next, sUptColumnCnt++ ) ;

    sSubStringCnt = (aStatement->myPlan->stmtTextLen / QCM_TRIGGER_SUBSTRING_LEN ) +
        (((aStatement->myPlan->stmtTextLen % QCM_TRIGGER_SUBSTRING_LEN) == 0) ? 0 : 1);

    for ( sRef = sParseTree->triggerReference, sRefRowCnt = 0;
          sRef != NULL;
          sRef = sRef->next, sRefRowCnt++ ) ;

    //---------------------------------------
    // SYS_TRIGGERS_ 에 정보 삽입
    //---------------------------------------

    idlOS::snprintf( sBuffer, QD_MAX_SQL_LENGTH,
                     "INSERT INTO SYS_TRIGGERS_ VALUES ("
                     QCM_SQL_UINT32_FMT", "     // 0. USER_ID
                     QCM_SQL_CHAR_FMT", "       // 1. USER_NAME
                     QCM_SQL_BIGINT_FMT", "     // 2. TRIGGER_OID
                     QCM_SQL_CHAR_FMT", "       // 3. TRIGGER_NAME
                     QCM_SQL_UINT32_FMT", "     // 4. TABLE_ID
                     QCM_SQL_INT32_FMT", "      // 5. IS_ENABLE
                     QCM_SQL_INT32_FMT", "      // 6. EVENT_TIME
                     QCM_SQL_INT32_FMT", "      // 7. EVENT_TYPE
                     QCM_SQL_INT32_FMT", "      // 8. UPDATE_COLUMN_CNT
                     QCM_SQL_INT32_FMT", "      // 9. GRANULARITY
                     QCM_SQL_INT32_FMT", "      // 10. REF_ROW_CNT
                     QCM_SQL_INT32_FMT", "      // 11. SUBSTRING_CNT
                     QCM_SQL_INT32_FMT", "      // 12. STRING_LENGTH
                     "SYSDATE, "                // 13. CREATED
                     "SYSDATE ) ",              // 14. LAST_DDL_TIME
                     sParseTree->triggerUserID,                // 0.
                     sParseTree->triggerUserName,              // 1.
                     QCM_OID_TO_BIGINT( sTriggerOID ),         // 2.
                     sParseTree->triggerName,                  // 3.
                     sParseTree->tableID,                      // 4.
                     sParseTree->actionCond.isEnable,          // 5. BUG-42989
                     sParseTree->triggerEvent.eventTime,       // 6.
                     sParseTree->triggerEvent.eventTypeList->eventType,
                     // 7.
                     sUptColumnCnt,                            // 8.
                     sParseTree->actionCond.actionGranularity, // 9.
                     sRefRowCnt,                               // 10.
                     sSubStringCnt,                            // 11.
                     aStatement->myPlan->stmtTextLen );        // 12.

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sBuffer,
                                 & sRowCnt )
              != IDE_SUCCESS );

    IDE_DASSERT( sRowCnt == 1 );

    //---------------------------------------
    // SYS_TRIGGER_STRINGS 에 정보 삽입
    //---------------------------------------

    sNcharList = sParseTree->ncharList;

    // PROJ-1579 NCHAR
    // 메타테이블에 저장하기 위해 스트링을 분할하기 전에
    // N 타입이 있는 경우 U 타입으로 변환한다.
    if( sNcharList != NULL )
    {
        for( sTempNamePosList = sNcharList;
             sTempNamePosList != NULL;
             sTempNamePosList = sTempNamePosList->next )
        {
            sNamePos = sTempNamePosList->namePos;

            // U 타입으로 변환하면서 늘어나는 사이즈 계산
            // N'안' => U'\C548' 으로 변환된다면
            // '안'의 캐릭터 셋이 KSC5601이라고 가정했을 때,
            // single-quote안의 문자는 2 byte -> 5byte로 변경된다.
            // 즉, 1.5배가 늘어나는 것이다.
            //(전체 사이즈가 아니라 증가하는 사이즈만 계산하는 것임)
            // 하지만, 어떤 예외적인 캐릭터 셋이 들어올지 모르므로
            // * 2로 충분히 잡는다.
            sAddSize += (sNamePos.size - 3) * 2;
        }

        sBufferSize = aStatement->myPlan->stmtTextLen + sAddSize;

        IDU_LIMITPOINT("qcmTrigger::addMetaInfo::malloc2");
        IDE_TEST(STRUCT_ALLOC_WITH_SIZE(aStatement->qmxMem,
                                        SChar,
                                        sBufferSize,
                                        & sStmtBuffer)
                 != IDE_SUCCESS);

        IDE_TEST( qcmProc::convertToUTypeString( aStatement,
                                                 sNcharList,
                                                 sStmtBuffer,
                                                 sBufferSize )
                  != IDE_SUCCESS );

        sStmtBufferLen = idlOS::strlen( sStmtBuffer );
    }
    else
    {
        sStmtBufferLen = aStatement->myPlan->stmtTextLen;
        sStmtBuffer    = aStatement->myPlan->stmtText;
    }

    for ( sSeqNo = 0, sCurrLen = 0, sCurrPos = 0;
          sSeqNo < sSubStringCnt;
          sSeqNo++ )
    {
        if ( (sStmtBufferLen - sCurrPos) >=
             QCM_TRIGGER_SUBSTRING_LEN )
        {
            sCurrLen = QCM_TRIGGER_SUBSTRING_LEN;
        }
        else
        {
            sCurrLen = sStmtBufferLen - sCurrPos;
        }

        // ['] 와 같은 문자를 처리하기 위하여 [\'] 와 같은
        // String으로 변환하여 복사한다.
        IDE_TEST(
            qcmProc::prsCopyStrDupAppo ( sSubString,
                                         sStmtBuffer + sCurrPos,
                                         sCurrLen )
            != IDE_SUCCESS );

        sCurrPos += sCurrLen;

        idlOS::snprintf( sBuffer, QD_MAX_SQL_LENGTH,
                         "INSERT INTO SYS_TRIGGER_STRINGS_ VALUES ("
                         QCM_SQL_UINT32_FMT", "      // 0. TABLE_ID
                         QCM_SQL_BIGINT_FMT", "      // 1. TRIGGER_OID
                         QCM_SQL_INT32_FMT", "       // 2. SEQ_NO
                         QCM_SQL_CHAR_FMT") ",       // 3. SUBSTRING
                         sParseTree->tableID,               // 0.
                         QCM_OID_TO_BIGINT( sTriggerOID ),  // 1.
                         sSeqNo,                            // 2.
                         sSubString );                      // 3.

        IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sBuffer,
                                     & sRowCnt )
                  != IDE_SUCCESS );

        IDE_DASSERT( sRowCnt == 1 );
    }

    //---------------------------------------
    // SYS_TRIGGER_UPDATE_COLUMNS 에 정보 삽입
    //---------------------------------------

    for ( sUptColumn = sParseTree->triggerEvent.eventTypeList->updateColumns;
          sUptColumn != NULL;
          sUptColumn = sUptColumn->next )
    {
        idlOS::snprintf( sBuffer, QD_MAX_SQL_LENGTH,
                         "INSERT INTO SYS_TRIGGER_UPDATE_COLUMNS_ VALUES ("
                         QCM_SQL_UINT32_FMT", "      // 0. TABLE_ID
                         QCM_SQL_BIGINT_FMT", "      // 1. TRIGGER_OID
                         QCM_SQL_INT32_FMT") ",      // 2. COLUMN_ID
                         sParseTree->tableID,                       // 0.
                         QCM_OID_TO_BIGINT( sTriggerOID ),          // 1.
                         (SInt) sUptColumn->basicInfo->column.id ); // 2.

        IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sBuffer,
                                     & sRowCnt )
                  != IDE_SUCCESS );

        IDE_DASSERT( sRowCnt == 1 );
    }

    //---------------------------------------
    // SYS_TRIGGER_DML_TABLES 에 정보 삽입
    //---------------------------------------

    for ( sCheckTable = aStatement->spvEnv->modifiedTableList;
          sCheckTable != NULL;
          sCheckTable = sCheckTable->next )
    {
        for ( sSameTable = aStatement->spvEnv->modifiedTableList;
              sSameTable != sCheckTable;
              sSameTable = sSameTable->next )
        {
            if ( sSameTable->tableID == sCheckTable->tableID )
            {
                // 동일한 Table이 이미 삽입되었음.
                break;
            }
            else
            {
                // Nothing To Do
            }
        }

        if ( sSameTable == sCheckTable )
        {
            idlOS::snprintf( sBuffer, QD_MAX_SQL_LENGTH,
                             "INSERT INTO SYS_TRIGGER_DML_TABLES_ VALUES ("
                             QCM_SQL_UINT32_FMT", "      // 0. TABLE_ID
                             QCM_SQL_BIGINT_FMT", "      // 1. TRIGGER_OID
                             QCM_SQL_UINT32_FMT", "      // 2. DML_TABLE_ID
                             QCM_SQL_INT32_FMT") ",      // 3. DML_TYPE
                             sParseTree->tableID,                  // 0.
                             QCM_OID_TO_BIGINT( sTriggerOID ),     // 1.
                             sCheckTable->tableID,                 // 2.
                             sCheckTable->stmtType );              // 3.

            IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                         sBuffer,
                                         & sRowCnt )
                      != IDE_SUCCESS );

            IDE_DASSERT( sRowCnt == 1 );
        }
        else
        {
            // 해당 DML_TABLE_ID가 삽입되었음.
            // Nothing To Do
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qcmTrigger::removeMetaInfo( qcStatement * aStatement,
                            smOID         aTriggerOID )
{
/***********************************************************************
 *
 * Description :
 *
 *    Trigger 생성과 관련된 모든 Meta Table 정보를 삭제한다.
 *
 * Implementation :
 *
 *
 ***********************************************************************/

    vSLong         sRowCnt;
    SChar       * sBuffer;

    //---------------------------------------
    // 적합성 검사
    //---------------------------------------

    IDE_DASSERT( aStatement != NULL );

    //---------------------------------------
    // String을 위한 공간 획득
    //---------------------------------------

    IDU_LIMITPOINT("qcmTrigger::removeMetaInfo::malloc");
    IDE_TEST( aStatement->qmxMem->alloc( QD_MAX_SQL_LENGTH,
                                         (void**) & sBuffer )
              != IDE_SUCCESS );

    //---------------------------------------
    // SYS_TRIGGERS_ 에서 정보 삭제
    // DELETE FROM SYS_TRIGGERS_ WHERE TRIGGER_OID = triggerOID;
    //---------------------------------------

    idlOS::snprintf( sBuffer, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_TRIGGERS_ WHERE TRIGGER_OID = "
                     QCM_SQL_BIGINT_FMT,            // 0. TRIGGER_OID
                     QCM_OID_TO_BIGINT( aTriggerOID ) );  // 0.

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sBuffer,
                                 & sRowCnt )
              != IDE_SUCCESS );

    IDE_DASSERT( sRowCnt == 1 );

    //---------------------------------------
    // SYS_TRIGGER_STRINGS_ 에서 정보 삭제
    // DELETE FROM SYS_TRIGGER_STRINGS_ WHERE TRIGGER_OID = triggerOID;
    //---------------------------------------

    idlOS::snprintf( sBuffer, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_TRIGGER_STRINGS_ WHERE TRIGGER_OID ="
                     QCM_SQL_BIGINT_FMT,                    // 0. TRIGGER_OID
                     QCM_OID_TO_BIGINT( aTriggerOID ) );    // 0.

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sBuffer,
                                 & sRowCnt )
              != IDE_SUCCESS );

    IDE_DASSERT( sRowCnt > 0 );

    //---------------------------------------
    // SYS_TRIGGER_UPDATE_COLUMNS_ 에서 정보 삭제
    //---------------------------------------

    idlOS::snprintf( sBuffer, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_TRIGGER_UPDATE_COLUMNS_ "
                     "WHERE TRIGGER_OID = "
                     QCM_SQL_BIGINT_FMT,                 // 0. TRIGGER_OID
                     QCM_OID_TO_BIGINT( aTriggerOID ) ); // 0.

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sBuffer,
                                 & sRowCnt )
              != IDE_SUCCESS );

    //---------------------------------------
    // SYS_TRIGGER_DML_TABLES 에 정보 삽입
    //---------------------------------------

    idlOS::snprintf( sBuffer, QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_TRIGGER_DML_TABLES_ WHERE TRIGGER_OID = "
                     QCM_SQL_BIGINT_FMT,                     // 0. TRIGGER_OID
                     QCM_OID_TO_BIGINT( aTriggerOID ) );     // 0.

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sBuffer,
                                 & sRowCnt )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qcmTrigger::getTriggerMetaCache( smiStatement * aSmiStmt,
                                 UInt           aTableID,
                                 qcmTableInfo * aTableInfo )
{
/***********************************************************************
 *
 * Description :
 *
 *    Table Meta Cache에
 *    Trigger Meta Table 정보를 읽어 Trigger Cache 정보를 삽입한다.
 *
 * Implementation :
 *
 *
 ***********************************************************************/

    UInt                    i;

    smiRange                sRange;
    qtcMetaRangeColumn      sFirstMetaRange;

    smiTableCursor          sCursor;
    idBool                  sIsCursorOpen;

    mtcColumn             *sTableIDCol;
    mtcColumn             *sUserIDCol;
    mtcColumn             *sUserNameCol;
    mtcColumn             *sTriggerOIDCol;
    mtcColumn             *sTriggerNameCol;
    mtcColumn             *sIsEnableCol;
    mtcColumn             *sEventTimeCol;
    mtcColumn             *sEventTypeCol;
    mtcColumn             *sUpdateColumnCntCol;
    mtcColumn             *sGranularityCol;
    mtcColumn             *sRefRowCntCol;

    mtdIntegerType          sTableID;
    mtdCharType           * sCharValue;

    const void            * sRow;
    scGRID                  sRID;
    smiCursorProperties     sCursorProperty;

    //---------------------------------------
    // 적합성 검사
    //---------------------------------------

    IDE_DASSERT( aSmiStmt != NULL );
    IDE_DASSERT( aTableInfo != NULL );

    sIsCursorOpen = ID_FALSE;

    //---------------------------------------
    // Trigger 개수의 획득
    // SELECT COUNT(*)
    //   FROM SYS_TRIGGERS_
    //  WHERE TABLE_ID = aTableID
    //---------------------------------------

    aTableInfo->triggerCount = 0;

    if (gQcmTriggers == NULL)
    {
        // on createdb : not yet created
        aTableInfo->triggerInfo  = NULL;

        return IDE_SUCCESS;
    }
    else
    {
        // Go Next
    }

    //-------------------------------------------
    // WHERE TABLE_ID = aTableID;
    // 검색을 위한 Key Range 생성
    //-------------------------------------------

    // SYS_TRIGGERS_ meta_table의 column정보를 획득
    IDE_TEST( smiGetTableColumns( gQcmTriggers,
                                  QCM_TRIGGERS_TABLE_ID,
                                  (const smiColumn**)&sTableIDCol )
              != IDE_SUCCESS );

    // C Type의 값을 DB Type으로 변경
    sTableID = (mtdIntegerType) aTableID;

    // 하나의 Column을 이용한 Key Range 생성
    qcm::makeMetaRangeSingleColumn(
        & sFirstMetaRange,
        (const mtcColumn*)sTableIDCol,
        (const void *) & sTableID,
        & sRange );

    //-------------------------------------------
    // COUNT(*) 검색
    //-------------------------------------------

    // Cursor의 초기화
    sCursor.initialize();

    SMI_CURSOR_PROP_INIT_FOR_META_INDEX_SCAN(&sCursorProperty, NULL);

    // INDEX (TABLE_ID)을 이용한 Cursor Open
    IDE_TEST(
        sCursor.open( aSmiStmt,  // smiStatement
                      gQcmTriggers,                  // Table Handle
                      gQcmTriggersIndex[QCM_TRIGGERS_TABLEID_TRIGGEROID_INDEX],
                      smiGetRowSCN( gQcmTriggers ),  // Table SCN
                      NULL,                          // Update Column정보
                      & sRange,                      // Key Range
                      smiGetDefaultKeyRange(),       // Key Filter
                      smiGetDefaultFilter(),         // Filter
                      QCM_META_CURSOR_FLAG,
                      SMI_SELECT_CURSOR,
                      &sCursorProperty)            // Cursor Property
        != IDE_SUCCESS );

    sIsCursorOpen = ID_TRUE;

    // Data를 Read함.
    IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );
    IDE_TEST( sCursor.readRow( & sRow, & sRID, SMI_FIND_NEXT )
              != IDE_SUCCESS );

    while ( sRow != NULL )
    {
        aTableInfo->triggerCount++;
        IDE_TEST( sCursor.readRow( & sRow, & sRID, SMI_FIND_NEXT )
                  != IDE_SUCCESS );
    }

    //-------------------------------------------
    // 해당 개수만큼 Table Trigger Cache 정보를 구축
    //-------------------------------------------

    if ( aTableInfo->triggerCount > 0 )
    {
        // Table Trigger Cache를 위한 공간 할당
        IDU_LIMITPOINT("qcmTrigger::getTriggerMetaCache::malloc");
        IDE_TEST(iduMemMgr::malloc( IDU_MEM_QCM,
                                    ID_SIZEOF(qcmTriggerInfo) *
                                    aTableInfo->triggerCount,
                                    (void**) & aTableInfo->triggerInfo )
                 != IDE_SUCCESS);

        IDE_TEST( sCursor.beforeFirst() != IDE_SUCCESS );

        IDE_TEST( sCursor.readRow( & sRow, & sRID, SMI_FIND_NEXT )
                  != IDE_SUCCESS );

        i = 0;
        IDE_TEST( smiGetTableColumns( gQcmTriggers,
                                      QCM_TRIGGERS_USER_ID,
                                      (const smiColumn**)&sUserIDCol )
                  != IDE_SUCCESS );
        IDE_TEST( smiGetTableColumns( gQcmTriggers,
                                      QCM_TRIGGERS_USER_NAME,
                                      (const smiColumn**)&sUserNameCol )
                  != IDE_SUCCESS );
        IDE_TEST( smiGetTableColumns( gQcmTriggers,
                                      QCM_TRIGGERS_TRIGGER_OID,
                                      (const smiColumn**)&sTriggerOIDCol )
                  != IDE_SUCCESS );
        IDE_TEST( smiGetTableColumns( gQcmTriggers,
                                      QCM_TRIGGERS_TRIGGER_NAME,
                                      (const smiColumn**)&sTriggerNameCol )
                  != IDE_SUCCESS );
        IDE_TEST( smiGetTableColumns( gQcmTriggers,
                                      QCM_TRIGGERS_IS_ENABLE,
                                      (const smiColumn**)&sIsEnableCol )
                  != IDE_SUCCESS );
        IDE_TEST( smiGetTableColumns( gQcmTriggers,
                                      QCM_TRIGGERS_EVENT_TIME,
                                      (const smiColumn**)&sEventTimeCol )
                  != IDE_SUCCESS );
        IDE_TEST( smiGetTableColumns( gQcmTriggers,
                                      QCM_TRIGGERS_EVENT_TYPE,
                                      (const smiColumn**)&sEventTypeCol )
                  != IDE_SUCCESS );
        IDE_TEST( smiGetTableColumns( gQcmTriggers,
                                      QCM_TRIGGERS_UPDATE_COLUMN_CNT,
                                      (const smiColumn**)&sUpdateColumnCntCol )
                  != IDE_SUCCESS );
        IDE_TEST( smiGetTableColumns( gQcmTriggers,
                                      QCM_TRIGGERS_GRANULARITY,
                                      (const smiColumn**)&sGranularityCol )
                  != IDE_SUCCESS );
        IDE_TEST( smiGetTableColumns( gQcmTriggers,
                                      QCM_TRIGGERS_REF_ROW_CNT,
                                      (const smiColumn**)&sRefRowCntCol )
                  != IDE_SUCCESS );

        // set aTableInfo->triggerInfo
        while (sRow != NULL)
        {
            //-------------------------------------------
            // 개별 Trigger Cache 정보를 구축
            //-------------------------------------------

            // SELECT USER_ID, USER_NAME, TRIGGER_OID, TRIGGER_NAME
            //        TABLE_ID, ENABLE, EVENT_TIME, EVENT_TYPE,
            //        UPT_COUNT, GRANULARIY

            // USER_ID
            aTableInfo->triggerInfo[i].userID = *(UInt*)
                ( (UChar*)sRow +
                  sUserIDCol->column.offset );

            // USER_NAME
            sCharValue = (mtdCharType*)
                ( (UChar*)sRow +
                  sUserNameCol->column.offset );

            idlOS::memcpy( aTableInfo->triggerInfo[i].userName,
                           sCharValue->value,
                           sCharValue->length );
            aTableInfo->triggerInfo[i].userName[sCharValue->length] = '\0';

            // TRIGGER_OID
            // To Fix PR-10648
            // (ULong *)을 명시적으로 (smOID*)로 변경해서는 안됨.
            // aTableInfo->triggerInfo[i].triggerOID = *(smOID*)
            aTableInfo->triggerInfo[i].triggerOID = (smOID) * (ULong*)
                ( (UChar*)sRow +
                  sTriggerOIDCol->column.offset );

            // triggerHandle
            aTableInfo->triggerInfo[i].triggerHandle = (void *)
                smiGetTable( aTableInfo->triggerInfo[i].triggerOID );

            // TIGGER_NAME
            sCharValue = (mtdCharType*)
                ( (UChar*)sRow +
                  sTriggerNameCol->column.offset );

            idlOS::memcpy( aTableInfo->triggerInfo[i].triggerName,
                           sCharValue->value,
                           sCharValue->length );

            aTableInfo->triggerInfo[i].triggerName[sCharValue->length] = '\0';

            // TABLE_ID
            aTableInfo->triggerInfo[i].tableID = *(UInt*)
                ( (UChar*)sRow +
                  sTableIDCol->column.offset );
            IDE_DASSERT( aTableInfo->triggerInfo[i].tableID == aTableID );

            // ENABLE
            aTableInfo->triggerInfo[i].enable = *(qcmTriggerAble *)
                ( (UChar*)sRow +
                  sIsEnableCol->column.offset );

            // EVENT_TIME
            aTableInfo->triggerInfo[i].eventTime = *(qcmTriggerEventTime *)
                ( (UChar*)sRow +
                  sEventTimeCol->column.offset );

            // EVENT_TYPE
            aTableInfo->triggerInfo[i].eventType = *(UInt *)
                ( (UChar*)sRow +
                  sEventTypeCol->column.offset );


            // UPDATE_COLUMN_CNT
            aTableInfo->triggerInfo[i].uptCount = *(UInt*)
                ( (UChar*)sRow +
                  sUpdateColumnCntCol->column.offset );

            // GRANULARITY
            aTableInfo->triggerInfo[i].granularity = *(qcmTriggerGranularity *)
                ( (UChar*)sRow +
                  sGranularityCol->column.offset );

            // REF_ROW_CNT
            aTableInfo->triggerInfo[i].refRowCnt = *(SInt *)
                ( (UChar*)sRow +
                  sRefRowCntCol->column.offset );

            IDE_TEST( sCursor.readRow( & sRow, & sRID, SMI_FIND_NEXT )
                      != IDE_SUCCESS );

            i++;
        }
    }
    else
    {
        aTableInfo->triggerInfo  = NULL;
    }


    sIsCursorOpen = ID_FALSE;
    IDE_TEST( sCursor.close() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sIsCursorOpen == ID_TRUE )
    {
        (void) sCursor.close();
    }

    return IDE_FAILURE;
}

IDE_RC qcmTrigger::copyTriggerMetaInfo( qcStatement * aStatement,
                                        UInt          aToTableID,
                                        smOID         aToTriggerOID,
                                        SChar       * aToTriggerName,
                                        UInt          aFromTableID,
                                        smOID         aFromTriggerOID,
                                        SChar       * aDDLText,
                                        SInt          aDDLTextLength )
{
/***********************************************************************
 *
 * Description :
 *      SYS_TRIGGERS_,
 *      SYS_TRIGGER_STRINGS_,
 *      SYS_TRIGGER_UPDATE_COLUMNS_,
 *      SYS_TRIGGER_DML_TABLES_를
 *      Trigger 단위로 복사한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    SChar  * sSqlStr         = NULL;
    vSLong   sRowCnt         = ID_vLONG( 0 );
    SInt     sSubStringCount = 0;

    sSubStringCount = aDDLTextLength / QCM_TRIGGER_SUBSTRING_LEN;

    if ( ( aDDLTextLength % QCM_TRIGGER_SUBSTRING_LEN ) != 0 )
    {
        sSubStringCount += 1;
    }
    else
    {
        /* Nothing to do */
    }

    IDU_FIT_POINT( "qcmTrigger::copyTriggerMetaInfo::STRUCT_ALLOC_WITH_SIZE::sSqlStr",
                   idERR_ABORT_InsufficientMemory );

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMX_MEM( aStatement ),
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      & sSqlStr )
              != IDE_SUCCESS );

    idlOS::snprintf( sSqlStr,
                     QD_MAX_SQL_LENGTH,
                     "INSERT INTO SYS_TRIGGERS_ SELECT "
                     "USER_ID, USER_NAME, "
                     "BIGINT'%"ID_INT64_FMT"', "                    // TRIGGER_OID
                     "VARCHAR'%s', "                                // TRIGGER_NAME
                     "INTEGER'%"ID_INT32_FMT"', "                   // TABLE_ID
                     "IS_ENABLE, EVENT_TIME, EVENT_TYPE, UPDATE_COLUMN_CNT, GRANULARITY, REF_ROW_CNT, "
                     "INTEGER'%"ID_INT32_FMT"', "                   // SUBSTRING_CNT
                     "INTEGER'%"ID_INT32_FMT"', "                   // STRING_LENGTH
                     "SYSDATE, SYSDATE "
                     "FROM SYS_TRIGGERS_ WHERE "
                     "TABLE_ID = INTEGER'%"ID_INT32_FMT"' AND "     // TABLE_ID
                     "TRIGGER_OID = BIGINT'%"ID_INT64_FMT"' ",      // TRIGGER_OID
                     QCM_OID_TO_BIGINT( aToTriggerOID ),
                     aToTriggerName,
                     aToTableID,
                     sSubStringCount,
                     aDDLTextLength,
                     aFromTableID,
                     QCM_OID_TO_BIGINT( aFromTriggerOID ) );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 & sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( (ULong)sRowCnt != ID_ULONG(1), ERR_META_CRASH );

    IDE_TEST( insertTriggerStringsMetaInfo( aStatement,
                                            aToTableID,
                                            aToTriggerOID,
                                            aDDLText,
                                            aDDLTextLength )
              != IDE_SUCCESS );

    idlOS::snprintf( sSqlStr,
                     QD_MAX_SQL_LENGTH,
                     "INSERT INTO SYS_TRIGGER_UPDATE_COLUMNS_ SELECT "
                     "INTEGER'%"ID_INT32_FMT"', "                                       // TABLE_ID
                     "BIGINT'%"ID_INT64_FMT"', "                                        // TRIGGER_OID
                     "CAST(%"ID_INT32_FMT" * 1024 + MOD(COLUMN_ID, 1024) AS INTEGER) "  // COLUMN_ID
                     "FROM SYS_TRIGGER_UPDATE_COLUMNS_ WHERE "
                     "TRIGGER_OID = BIGINT'%"ID_INT64_FMT"' ",                          // TRIGGER_OID
                     aToTableID,
                     QCM_OID_TO_BIGINT( aToTriggerOID ),
                     aToTableID,
                     QCM_OID_TO_BIGINT( aFromTriggerOID ) );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 & sRowCnt )
              != IDE_SUCCESS );

    idlOS::snprintf( sSqlStr,
                     QD_MAX_SQL_LENGTH,
                     "INSERT INTO SYS_TRIGGER_DML_TABLES_ SELECT "
                     "INTEGER'%"ID_INT32_FMT"', "               // TABLE_ID
                     "BIGINT'%"ID_INT64_FMT"', "                // TRIGGER_OID
                     "DML_TABLE_ID ,STMT_TYPE "
                     "FROM SYS_TRIGGER_DML_TABLES_ WHERE "
                     "TRIGGER_OID = BIGINT'%"ID_INT64_FMT"' ",  // TRIGGER_OID
                     aToTableID,
                     QCM_OID_TO_BIGINT( aToTriggerOID ),
                     QCM_OID_TO_BIGINT( aFromTriggerOID ) );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 & sRowCnt )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_CRASH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_META_CRASH ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmTrigger::updateTriggerStringsMetaInfo( qcStatement * aStatement,
                                                 UInt          aTableID,
                                                 smOID         aTriggerOID,
                                                 SInt          aDDLTextLength )
{
/***********************************************************************
 *
 * Description :
 *      SYS_TRIGGERS_의 Trigger Strings 정보를 갱신한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    SChar  * sSqlStr         = NULL;
    vSLong   sRowCnt         = ID_vLONG( 0 );
    SInt     sSubStringCount = 0;

    sSubStringCount = aDDLTextLength / QCM_TRIGGER_SUBSTRING_LEN;

    if ( ( aDDLTextLength % QCM_TRIGGER_SUBSTRING_LEN ) != 0 )
    {
        sSubStringCount += 1;
    }
    else
    {
        /* Nothing to do */
    }

    IDU_FIT_POINT( "qcmTrigger::updateTriggerStringsMetaInfo::STRUCT_ALLOC_WITH_SIZE::sSqlStr",
                   idERR_ABORT_InsufficientMemory );

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMX_MEM( aStatement ),
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      & sSqlStr )
              != IDE_SUCCESS );

    idlOS::snprintf( sSqlStr,
                     QD_MAX_SQL_LENGTH,
                     "UPDATE SYS_TRIGGERS_ "
                     "   SET SUBSTRING_CNT = INTEGER'%"ID_INT32_FMT"', "
                     "       STRING_LENGTH = INTEGER'%"ID_INT32_FMT"', "
                     "       LAST_DDL_TIME = SYSDATE "
                     " WHERE TABLE_ID = INTEGER'%"ID_INT32_FMT"' AND "
                     "       TRIGGER_OID = BIGINT'%"ID_INT64_FMT"' ",
                     sSubStringCount,
                     aDDLTextLength,
                     aTableID,
                     QCM_OID_TO_BIGINT( aTriggerOID ) );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 & sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( (ULong)sRowCnt != ID_ULONG(1), ERR_META_CRASH );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_CRASH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_META_CRASH ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmTrigger::insertTriggerStringsMetaInfo( qcStatement * aStatement,
                                                 UInt          aTableID,
                                                 smOID         aTriggerOID,
                                                 SChar       * aDDLText,
                                                 SInt          aDDLTextLength )
{
/***********************************************************************
 *
 * Description :
 *      SYS_TRIGGER_STRINGS_를 입력한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    SChar  * sSqlStr         = NULL;
    vSLong   sRowCnt         = ID_vLONG( 0 );

    SChar    sSubString[QCM_TRIGGER_SUBSTRING_LEN * 2 + 1];
    SInt     sSubStringCount = 0;
    SInt     sSeqNo          = 0;
    SInt     sCurrLen        = 0;
    SInt     sCurrPos        = 0;

    sSubStringCount = aDDLTextLength / QCM_TRIGGER_SUBSTRING_LEN;

    if ( ( aDDLTextLength % QCM_TRIGGER_SUBSTRING_LEN ) != 0 )
    {
        sSubStringCount += 1;
    }
    else
    {
        /* Nothing to do */
    }

    IDU_FIT_POINT( "qcmTrigger::insertTriggerStringsMetaInfo::STRUCT_ALLOC_WITH_SIZE::sSqlStr",
                   idERR_ABORT_InsufficientMemory );

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMX_MEM( aStatement ),
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      & sSqlStr )
              != IDE_SUCCESS );

    // qcmTrigger::addMetaInfo()에서 일부 코드 복사
    for ( sSeqNo = 0, sCurrPos = 0;
          sSeqNo < sSubStringCount;
          sSeqNo++ )
    {
        if ( ( aDDLTextLength - sCurrPos ) >= QCM_TRIGGER_SUBSTRING_LEN )
        {
            sCurrLen = QCM_TRIGGER_SUBSTRING_LEN;
        }
        else
        {
            sCurrLen = aDDLTextLength - sCurrPos;
        }

        // ['] 와 같은 문자를 처리하기 위하여 [\'] 와 같은 String으로 변환하여 복사한다.
        IDE_TEST( qcmProc::prsCopyStrDupAppo( sSubString,
                                              aDDLText + sCurrPos,
                                              sCurrLen )
                  != IDE_SUCCESS );

        sCurrPos += sCurrLen;

        idlOS::snprintf( sSqlStr,
                         QD_MAX_SQL_LENGTH,
                         "INSERT INTO SYS_TRIGGER_STRINGS_ VALUES ("
                         "INTEGER'%"ID_INT32_FMT"', "   // TABLE_ID
                         "BIGINT'%"ID_INT64_FMT"', "    // TRIGGER_OID
                         "INTEGER'%"ID_INT32_FMT"', "   // SEQNO
                         "VARCHAR'%s') ",               // SUBSTRING
                         aTableID,
                         QCM_OID_TO_BIGINT( aTriggerOID ),
                         sSeqNo,
                         sSubString );

        IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( (ULong)sRowCnt != ID_ULONG(1), ERR_META_CRASH );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_CRASH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_META_CRASH ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmTrigger::removeTriggerStringsMetaInfo( qcStatement * aStatement,
                                                 smOID         aTriggerOID )
{
/***********************************************************************
 *
 * Description :
 *      SYS_TRIGGER_STRINGS_를 제거한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    SChar  * sSqlStr = NULL;
    vSLong   sRowCnt = ID_vLONG(0);

    IDU_FIT_POINT( "qcmTrigger::removeTriggerStringsMetaInfo::STRUCT_ALLOC_WITH_SIZE::sSqlStr",
                   idERR_ABORT_InsufficientMemory );

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMX_MEM( aStatement ),
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      & sSqlStr )
              != IDE_SUCCESS );

    idlOS::snprintf( sSqlStr,
                     QD_MAX_SQL_LENGTH,
                     "DELETE FROM SYS_TRIGGER_STRINGS_ "
                     "      WHERE TRIGGER_OID = BIGINT'%"ID_INT64_FMT"' ",
                     (SLong)aTriggerOID );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 & sRowCnt )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( (ULong)sRowCnt == ID_ULONG(0), ERR_META_CRASH );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_CRASH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_META_CRASH ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmTrigger::renameTriggerMetaInfoForSwap( qcStatement    * aStatement,
                                                 qcmTableInfo   * aSourceTableInfo,
                                                 qcmTableInfo   * aTargetTableInfo,
                                                 qcNamePosition   aNamesPrefix )
{
/***********************************************************************
 *
 * Description :
 *      Prefix를 지정한 경우, Source의 TRIGGER_NAME에 Prefix를 붙이고,
 *      Target의 TRIGGER_NAME에서 Prefix를 제거한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    SChar    sNamesPrefix[QC_MAX_OBJECT_NAME_LEN + 1];
    SChar  * sSqlStr = NULL;
    vSLong   sRowCnt = ID_vLONG(0);

    if ( QC_IS_NULL_NAME( aNamesPrefix ) != ID_TRUE )
    {
        QC_STR_COPY( sNamesPrefix, aNamesPrefix );

        IDU_FIT_POINT( "qcmTrigger::renameTriggerMetaInfoForSwap::STRUCT_ALLOC_WITH_SIZE::sSqlStr",
                       idERR_ABORT_InsufficientMemory );

        IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMX_MEM( aStatement ),
                                          SChar,
                                          QD_MAX_SQL_LENGTH,
                                          & sSqlStr )
                  != IDE_SUCCESS );

        // Source의 TRIGGER_NAME에 Prefix를 붙이고, Target의 TRIGGER_NAME에서 Prefix를 제거한다.
        idlOS::snprintf( sSqlStr,
                         QD_MAX_SQL_LENGTH,
                         "UPDATE SYS_TRIGGERS_ "
                         "   SET TRIGGER_NAME = CASE2( TABLE_ID = INTEGER'%"ID_INT32_FMT"', "
                         "                             VARCHAR'%s' || TRIGGER_NAME, "
                         "                             SUBSTR( TRIGGER_NAME, INTEGER'%"ID_INT32_FMT"' ) ) "
                         " WHERE TABLE_ID IN ( INTEGER'%"ID_INT32_FMT"', "
                         "                     INTEGER'%"ID_INT32_FMT"' ) ",
                         aSourceTableInfo->tableID,
                         sNamesPrefix,
                         aNamesPrefix.size + 1,
                         aSourceTableInfo->tableID,
                         aTargetTableInfo->tableID );

        IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                     sSqlStr,
                                     & sRowCnt )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( (UInt)sRowCnt != ( aSourceTableInfo->triggerCount + aTargetTableInfo->triggerCount ),
                        ERR_META_CRASH );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_META_CRASH )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QCM_META_CRASH ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qcmTrigger::swapTriggerDMLTablesMetaInfo( qcStatement * aStatement,
                                                 UInt          aTargetTableID,
                                                 UInt          aSourceTableID )
{
/***********************************************************************
 *
 * Description :
 *      다른 Trigger가 Cycle Check에 사용하는 정보를 갱신한다. (Meta Table)
 *          - SYS_TRIGGER_DML_TABLES_
 *              - DML_TABLE_ID = Table ID 이면, (TABLE_ID와 무관하게) DML_TABLE_ID를 Peer의 Table ID로 교체한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    SChar  * sSqlStr = NULL;
    vSLong   sRowCnt = ID_vLONG(0);

    IDU_FIT_POINT( "qcmTrigger::swapTriggerDMLTablesMetaInfo::STRUCT_ALLOC_WITH_SIZE::sSqlStr",
                   idERR_ABORT_InsufficientMemory );

    IDE_TEST( STRUCT_ALLOC_WITH_SIZE( QC_QMX_MEM( aStatement ),
                                      SChar,
                                      QD_MAX_SQL_LENGTH,
                                      & sSqlStr )
              != IDE_SUCCESS );

    idlOS::snprintf( sSqlStr,
                     QD_MAX_SQL_LENGTH,
                     "UPDATE SYS_TRIGGER_DML_TABLES_ "
                     "   SET DML_TABLE_ID = CASE2( DML_TABLE_ID = INTEGER'%"ID_INT32_FMT"', "
                     "                             INTEGER'%"ID_INT32_FMT"', "
                     "                             INTEGER'%"ID_INT32_FMT"' ) "
                     " WHERE DML_TABLE_ID IN ( INTEGER'%"ID_INT32_FMT"', INTEGER'%"ID_INT32_FMT"' ) ",
                     aTargetTableID,
                     aSourceTableID,
                     aTargetTableID,
                     aTargetTableID,
                     aSourceTableID );

    IDE_TEST( qcg::runDMLforDDL( QC_SMI_STMT( aStatement ),
                                 sSqlStr,
                                 & sRowCnt )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

