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
 * $Id: qdnTrigger.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *
 *     [PROJ-1359] Trigger
 *
 *     Trigger 처리를 위한 자료 구조 및 함수
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QDN_TRIGGER_H_
#define _O_QDN_TRIGGER_H_  1

#include <qtcDef.h>
#include <qsParseTree.h>
#include <qsxProc.h>

//=========================================================
// [PROJ-1359] Trigger를 위한 Parse Tree 정보
//=========================================================

//-----------------------------------------------------
// [Trigger Event 구문]
//-----------------------------------------------------

// [Trigger Event의 정보]
typedef struct qdnTriggerEventTypeList
{
    UInt                       eventType;
    qcmColumn *                updateColumns;
    qdnTriggerEventTypeList *  next;
} qcmTriggerEventTypeList;

typedef struct qdnTriggerEvent {
    qcmTriggerEventTime       eventTime;
    qcmTriggerEventTypeList  *eventTypeList;
} qdnTriggerEvent;

//-----------------------------------------------------
// [Trigger Referencing 구문]
//-----------------------------------------------------

// RERENCING 구문에 기록되는 OLD ROW, NEW ROW등은
// Action Body내에서 [old_row T1%ROWTYPE] 암시적으로 사용되며,
// 하나의 Node로 표현된다.  이를 관리하기 위한 정보이며,
// Trigger Action 수행 시 해당 정보를 이용하여 Procedure의
// Declare Secion의 값을 변화시킨다.
//
// Ex) CREATE TRIGGER trigger_1 AFTER UPDATE ON t1
//     REFERENCING OLD ROW AS old_row
//     AS BEGIN
//         INSERT INTO log_table VALUES ( old_row.i1 );
//     END;
//
//     ===>  Action Body를 위한 Procedure 생성
//
//     CREATE PROCEDURE trigger_procedure
//     AS
//         old_row T1%ROWTYPE;   <========== 바로 여기의 정보를 저장
//     AS BEGIN
//         INSERT INTO log_table VALUES ( old_row.i1 );
//     END;
//-----------------------------------------------------

// [REFERENCING 정보]
typedef struct qdnTriggerRef
{
    qcmTriggerRefType         refType;
    qcNamePosition            aliasNamePos;
    qsVariables             * rowVar;          // referencing row
    qdnTriggerRef           * next;
} qdnTriggerRef;

//-----------------------------------------------------
// [Trigger Action Condition 구문]
//-----------------------------------------------------

// [ACTION CONDITION 정보]
typedef struct qdnTriggerActionCond
{
    qcmTriggerGranularity actionGranularity;

    // BUG-42989 Create trigger with enable/disable option.
    qcmTriggerAble        isEnable;

    qtcNode *             whenCondition;
} qdnTriggerActionCond;

// [ACTION BODY가 참조하는 DML Table]
// Cycle Detection을 위하여 관리한다.

typedef struct qdnActionRelatedTable
{
    UInt                    tableID;
    qsProcStmtType          stmtType;
    qdnActionRelatedTable * next;
} qdnActionRelatedTable;

//-----------------------------------------------------
// [ALTER TRIGGER를 위한 구문]
//-----------------------------------------------------

typedef enum {
    QDN_TRIGGER_ALTER_NONE,
    QDN_TRIGGER_ALTER_ENABLE,
    QDN_TRIGGER_ALTER_DISABLE,
    QDN_TRIGGER_ALTER_COMPILE
} qdnTriggerAlterOption;

//===========================================================
// Trigger를 위한 Parse Tree
//===========================================================

// Example )
//     CREATE TRIGGER triggerUserName.triggerName
//     AFTER UPDATE OF updateColumns ON userName.tableName
//     REFERENCING OLD ROW AS aliasName
//     FOR EACH ROW WHEN tableName.i1 > 30
//     AS
//     BEGIN
//         INSERT INTO log_table VALUES ( aliasName.i1 );
//     END;

// [CREATE TRIGGER를 위한 Parse Tree]
typedef struct qdnCreateTriggerParseTree
{
    //---------------------------------------------
    // Parsing 정보
    //---------------------------------------------

    qcParseTree                common;

    // Trigger Name 정보
    qcNamePosition             triggerUserPos;
    qcNamePosition             triggerNamePos;

    // Trigger Table 정보
    qcNamePosition             userNamePos;
    qcNamePosition             tableNamePos;

    // Trigger Event 정보
    qdnTriggerEvent            triggerEvent;

    // Referencing 정보
    qdnTriggerRef            * triggerReference;

    // Trigger Action 정보
    qdnTriggerActionCond       actionCond;
    qsProcParseTree            actionBody;

    smOID                      triggerOID;
    smSCN                      triggerSCN;

    //---------------------------------------------
    // Validation 정보
    //---------------------------------------------

    // recompile 정보
    idBool                     isRecompile;

    // Trigger Name 정보
    UInt                       triggerUserID;
    SChar                      triggerUserName[QC_MAX_OBJECT_NAME_LEN + 1];
    SChar                      triggerName[QC_MAX_OBJECT_NAME_LEN + 1];

    // Trigger Table 정보
    UInt                       tableUserID;
    UInt                       tableID;
    smOID                      tableOID;
    qcmTableInfo             * tableInfo;

    // TASK-2176
    void                     * tableHandle;
    smSCN                      tableSCN;

    // BUG-20948
    // Trigger의 Target Table이 바뀌었을 경우 Original Table의 정보를 변경하기 위해 저장
    UInt                       orgTableUserID;
    UInt                       orgTableID;
    smOID                      orgTableOID;
    qcmTableInfo             * orgTableInfo;

    void                     * orgTableHandle; 
    smSCN                      orgTableSCN;       

    // 트리거도 프로시저와 동일하게 부분 재빌드가 가능하다.
    // 부분 재빌드 관련 정보는 qsxProcInfo에 존재하기 때문에 가상의
    // qsxProcInfo를 구축해 주어야 한다.
    qsxProcInfo                procInfo;

    // BUG-21761
    // N타입을 U타입으로 변형시킬 때 사용
    qcNamePosList            * ncharList;

    // PROJ-2219 Row-level before update trigger
    // count and list of referenced columns by a trigger.
    UInt                       refColumnCount;
    UChar                    * refColumnList;

} qdnCreateTriggerParseTree;

// PROJ-2219 Row-level before update trigger
#define QDN_REF_COLUMN_FALSE ((UChar)0X00)
#define QDN_REF_COLUMN_TRUE  ((UChar)0X01)

#define QDN_CREATE_TRIGGER_PARSE_TREE_INIT( _dst_ ) \
    {                                               \
        SET_EMPTY_POSITION(_dst_->triggerUserPos);  \
        SET_EMPTY_POSITION(_dst_->triggerNamePos);  \
        SET_EMPTY_POSITION(_dst_->userNamePos);     \
        SET_EMPTY_POSITION(_dst_->tableNamePos);    \
        _dst_->ncharList = NULL;                    \
    }


// [ALTER TRIGGER를 위한 Parse Tree]
typedef struct qdnAlterTriggerParseTree
{
    //---------------------------------------------
    // Parsing 정보
    //---------------------------------------------

    qcParseTree                common;

    // Trigger Name 정보
    qcNamePosition             triggerUserPos;
    qcNamePosition             triggerNamePos;

    qdnTriggerAlterOption      option;

    //---------------------------------------------
    // Validation 정보
    //---------------------------------------------

    // Trigger Name 정보
    UInt                       triggerUserID;
    smOID                      triggerOID;

    // Trigger가 접근하는 Table 정보
    UInt                       tableID;

} qdnAlterTriggerParseTree;

#define QDN_ALTER_TRIGGER_PARSE_TREE_INIT( _dst_ )  \
    {                                               \
        SET_EMPTY_POSITION(_dst_->triggerUserPos);  \
        SET_EMPTY_POSITION(_dst_->triggerNamePos);  \
    }


// [DROP TRIGGER를 위한 Parse Tree]
typedef struct qdnDropTriggerParseTree
{
    //---------------------------------------------
    // Parsing 정보
    //---------------------------------------------

    qcParseTree                common;

    // Trigger Name 정보
    qcNamePosition             triggerUserPos;
    qcNamePosition             triggerNamePos;

    //---------------------------------------------
    // Validation 정보
    //---------------------------------------------

    // Trigger Name 정보
    UInt                       triggerUserID;
    smOID                      triggerOID;

    // Trigger가 접근하는 Table 정보
    UInt                       tableID;
    qcmTableInfo             * tableInfo;

    // TASK-2176
    void                     * tableHandle;
    smSCN                      tableSCN;

} qdnDropTriggerParseTree;

#define QDN_DROP_TRIGGER_PARSE_TREE_INIT( _dst_ )   \
    {                                               \
        SET_EMPTY_POSITION(_dst_->triggerUserPos);  \
        SET_EMPTY_POSITION(_dst_->triggerNamePos);  \
    }

//===========================================================
// [Trigger Object를 위한 Cache 구조]
//
//     [Trigger Handle] -- info ----> CREATE TRIGGER String
//                      |
//                      -- tempInfo ----> Trigger Object Cache
//
// Trigger Object Cache는 CREATE TRIGGER 또는 Server 구동 시
// 생성되며,  Trigger를 수행하기 위한 일부 정보들을 관리한다.
// Trigger 수행의 동시성 제어 및 Trigger Action 정보가 구축된다.
//===========================================================

typedef struct qdnTriggerCache
{
    iduLatch                     latch;      // 동시성 제어를 위한 latch
    idBool                       isValid;    // Cache 정보의 유효성 여부
    qcStatement                  triggerStatement; // PVO가 완료된 Statement 정보
    
    UInt                         stmtTextLen;
    SChar                      * stmtText;
} qdnTriggerCache;

//=========================================================
// [PROJ-1359] Trigger를 위한 함수
//=========================================================

class qdnTrigger
{
public:

    //----------------------------------------------
    // CREATE TRIGGER를 위한 함수
    //----------------------------------------------

    static IDE_RC parseCreate( qcStatement * aStatement );
    static IDE_RC validateCreate( qcStatement * aStatement );
    static IDE_RC validateRecompile( qcStatement * aStatement );
    static IDE_RC optimizeCreate( qcStatement * aStatement );
    static IDE_RC executeCreate( qcStatement * aStatement );
    static IDE_RC validateReplace( qcStatement * aStatement );
    static IDE_RC executeReplace( qcStatement * aStatement );

    //----------------------------------------------
    // ALTER TRIGGER를 위한 함수
    //----------------------------------------------

    static IDE_RC parseAlter( qcStatement * aStatement );
    static IDE_RC validateAlter( qcStatement * aStatement );
    static IDE_RC optimizeAlter( qcStatement * aStatement );
    static IDE_RC executeAlter( qcStatement * aStatement );

    //----------------------------------------------
    // DROP TRIGGER를 위한 함수
    //----------------------------------------------

    static IDE_RC parseDrop( qcStatement * aStatement );
    static IDE_RC validateDrop( qcStatement * aStatement );
    static IDE_RC optimizeDrop( qcStatement * aStatement );
    static IDE_RC executeDrop( qcStatement * aStatement );

    //----------------------------------------------
    // TRIGGER 수행을 위한 함수
    //----------------------------------------------

    // Trigger Referencing Row의 Build가 필요한지를 판단
    static IDE_RC needTriggerRow( qcStatement         * aStatement,
                                  qcmTableInfo        * aTableInfo,
                                  qcmTriggerEventTime   aEventTime,
                                  UInt                  aEventType,
                                  smiColumnList       * aUptColumn,
                                  idBool              * aIsNeed );

    // Trigger를 수행한다.
    static IDE_RC fireTrigger( qcStatement         * aStatement,
                               iduMemory           * aNewValueMem,
                               qcmTableInfo        * aTableInfo,
                               qcmTriggerGranularity aGranularity,
                               qcmTriggerEventTime   aEventTime,
                               UInt                  aEventType,
                               smiColumnList       * aUptColumn,
                               smiTableCursor      * aTableCursor,
                               scGRID                aGRID,
                               void                * aOldRow,
                               qcmColumn           * aOldRowColumns,
                               void                * aNewRow,
                               qcmColumn           * aNewRowColumns );

    //----------------------------------------------
    // Trigger의 일괄 처리 작업
    //----------------------------------------------

    // Server 구동시 Trigger Object Cache정보를 구축한다.
    static IDE_RC loadAllTrigger( smiStatement * aSmiStmt );

    // Table 이 소유한 Trigger를 모두 제거한다.
    static IDE_RC dropTrigger4DropTable( qcStatement  * aStatement,
                                         qcmTableInfo * aTableInfo );

    // To Fix BUG-12034
    // qdd::executeDropTable 에서
    // SM이 더이상 실패할 여지가 없을 때에
    // 이 함수를 호출하여 TriggerChache를 free한다.
    static IDE_RC freeTriggerCaches4DropTable( qcmTableInfo * aTableInfo );

    // Trigger Cache 를 삭제한다.
    static IDE_RC freeTriggerCache( qdnTriggerCache * aCache );

    /* PROJ-2600 Online DDL for Tablespace Alteration */
    static void freeTriggerCacheArray( qdnTriggerCache ** aTriggerCaches,
                                       UInt               aTriggerCount );

    /* PROJ-2600 Online DDL for Tablespace Alteration */
    static void restoreTempInfo( qdnTriggerCache ** aTriggerCaches,
                                 qcmTableInfo     * aTableInfo );

    // BUG-16543
    static IDE_RC setInvalidTriggerCache4Table( qcmTableInfo * aTableInfo );

    // BUG-31406
    static IDE_RC executeRenameTable( qcStatement  * aStatement,
                                      qcmTableInfo * aTableInfo );

    /* PROJ-2600 Online DDL for Tablespace Alteration */
    static IDE_RC executeCopyTable( qcStatement       * aStatement,
                                    qcmTableInfo      * aSourceTableInfo,
                                    qcmTableInfo      * aTargetTableInfo,
                                    qcNamePosition      aNamesPrefix,
                                    qdnTriggerCache *** aTargetTriggerCache );

    /* PROJ-2600 Online DDL for Tablespace Alteration */
    static IDE_RC executeSwapTable( qcStatement       * aStatement,
                                    qcmTableInfo      * aSourceTableInfo,
                                    qcmTableInfo      * aTargetTableInfo,
                                    qcNamePosition      aNamesPrefix,
                                    qdnTriggerCache *** aOldSourceTriggerCache,
                                    qdnTriggerCache *** aOldTargetTriggerCache,
                                    qdnTriggerCache *** aNewSourceTriggerCache,
                                    qdnTriggerCache *** aNewTargetTriggerCache );

    // PROJ-2219 Row-level before update trigger
    static IDE_RC verifyTriggers( qcStatement   * aQcStmt,
                                  qcmTableInfo  * aTableInfo,
                                  smiColumnList * aUptColumn,
                                  idBool        * aIsNeedRebuild );

    //----------------------------------------------
    // CREATE TRIGGER의 Execution을 위한 함수
    //----------------------------------------------
    
    // Trigger Handle을 위한 Cache 공간을 할당한다.
    static IDE_RC allocTriggerCache( void             * aTriggerHandle,
                                     smOID              aTriggerOID,
                                     qdnTriggerCache ** aCache );
private:

    //----------------------------------------------
    // CREATE TRIGGER의 Parsing을 위한 함수
    //----------------------------------------------

    // FOR EACH ROW의 Validation을 위한 부가 정보를 추가함
    static IDE_RC addGranularityInfo( qdnCreateTriggerParseTree * aParseTree );

    // FOR EACH ROW의 Action Body를 위한 부가 정보를 추가함.
    static IDE_RC addActionBodyInfo( qcStatement               * aStatement,
                                     qdnCreateTriggerParseTree * aParseTree );

    //----------------------------------------------
    // CREATE TRIGGER의 Validation을 위한 함수
    //----------------------------------------------

    // BUG-24570
    // Trigger User에 대한 정보 설정
    static IDE_RC setTriggerUser( qcStatement               * aStatement,
                                  qdnCreateTriggerParseTree * aParseTree );

    // Trigger 생성에 대한 권한 검사
    static IDE_RC valPrivilege( qcStatement               * aStatement,
                                qdnCreateTriggerParseTree * aParseTree );

    // Trigger Name에 대한 Validation
    static IDE_RC valTriggerName( qcStatement               * aStatement,
                                  qdnCreateTriggerParseTree * aParseTree );

    // Recompile을 위한 Trigger Name에 대한 Validation
    static IDE_RC reValTriggerName( qcStatement               * aStatement,
                                    qdnCreateTriggerParseTree * aParseTree );

    // Trigger Table에 대한 Validation
    static IDE_RC valTableName( qcStatement               * aStatement,
                                qdnCreateTriggerParseTree * aParseTree );

    // To Fix BUG-20948
    static IDE_RC valOrgTableName( qcStatement               * aStatement,
                                   qdnCreateTriggerParseTree * aParseTree );

    // Trigger Event와 Referencing 에 대한 Validation
    static IDE_RC valEventReference( qcStatement               * aStatement,
                                     qdnCreateTriggerParseTree * aParseTree );

    // Action Condition에 대한 Validation
    static IDE_RC valActionCondition( qcStatement               * aStatement,
                                      qdnCreateTriggerParseTree * aParseTree );

    // Action Body에 대한 Validation
    static IDE_RC valActionBody( qcStatement               * aStatement,
                                 qdnCreateTriggerParseTree * aParseTree );
    
    //PROJ-1888 INSTEAD OF TRIGGER 제약사항 검사
    static IDE_RC valInsteadOfTrigger( qdnCreateTriggerParseTree * aParseTree );
    
    // Action Body에 존재하는 Cycle의 검사
    static IDE_RC checkCycle( qcStatement               * aStatement,
                              qdnCreateTriggerParseTree * aParseTree );

    // DML로 참조되는 Table이 소유한 Trigger로부터 Cycle의 검사
    static IDE_RC checkCycleOtherTable( qcStatement               * aStatement,
                                        qdnCreateTriggerParseTree * aParseTree,
                                        UInt                        aTableID );

    //----------------------------------------------
    // CREATE TRIGGER의 Execution을 위한 함수
    //----------------------------------------------

    // Trigger Object를 생성한다.
    static IDE_RC createTriggerObject( qcStatement     * aStatement,
                                       void           ** aTriggerHandle );

    //----------------------------------------------
    // DROP TRIGGER의 Execution을 위한 함수
    //----------------------------------------------

    // Trigger Object를 생성한다.
    static IDE_RC dropTriggerObject( qcStatement     * aStatement,
                                     void            * aTriggerHandle );

    //----------------------------------------------
    // Trigger 수행을 위한 함수
    //----------------------------------------------

    // 조건에 부합하는 Trigger인지 검사
    static IDE_RC checkCondition( qcStatement         * aStatement,
                                  qcmTriggerInfo      * aTriggerInfo,
                                  qcmTriggerGranularity aGranularity,
                                  qcmTriggerEventTime   aEventTime,
                                  UInt                  aEventType,
                                  smiColumnList       * aUptColumn,
                                  idBool              * aNeedAction,
                                  idBool              * aIsRecompile );

    // Validation 여부를 고려하여 Trigger Action을 수행한다.
    static IDE_RC fireTriggerAction( qcStatement    * aStatement,
                                     iduMemory      * aNewValueMem,
                                     qcmTableInfo   * aTableInfo,
                                     qcmTriggerInfo * aTriggerInfo,
                                     smiTableCursor * aTableCursor,
                                     scGRID           aGRID,
                                     void           * aOldRow,
                                     qcmColumn      * aOldRowColumns,
                                     void           * aNewRow,
                                     qcmColumn      * aNewRowColumns );

    // Validate한 경우 Trigger Action을 수행한다.
    static IDE_RC runTriggerAction( qcStatement           * aStatement,
                                    iduMemory             * aNewValueMem,
                                    qcmTableInfo          * aTableInfo,
                                    qcmTriggerInfo        * aTriggerInfo,
                                    smiTableCursor        * aTableCursor,
                                    scGRID                  aGRID,
                                    void                  * aOldRow,
                                    qcmColumn             * aOldRowColumns,
                                    void                  * aNewRow,
                                    qcmColumn             * aNewRowColumns );

    // WHEN Condition 및 Action Body정보를 처리할 수 있도록
    // REFERENCING ROW의 정보를 셋팅한다.
    static IDE_RC setReferencingRow( qcTemplate                * aClonedTemplate,
                                     qcmTableInfo              * aTableInfo,
                                     qdnCreateTriggerParseTree * aParseTree,
                                     smiTableCursor            * aTableCursor,
                                     scGRID                      aGRID,
                                     void                      * aOldRow,
                                     qcmColumn                 * aOldRowColumns,
                                     void                      * aNewRow,
                                     qcmColumn                 * aNewRowColumns );

    // WHEN Condition 및 Action Body정보를 처리할 수 있도록
    // REFERENCING ROW의 정보를 셋팅한다.
    static IDE_RC makeValueListFromReferencingRow(
        qcTemplate                * aClonedTemplate,
        iduMemory                 * aNewValueMem,
        qcmTableInfo              * aTableInfo,
        qdnCreateTriggerParseTree * aParseTree,
        void                      * aNewRow,
        qcmColumn                 * aNewRowColumn );
    
    // tableRow를 PSM rowtype으로 변환하여 복사
    static IDE_RC copyRowFromTableRow( qcTemplate        * aTemplate,
                                       qcmTableInfo      * aTableInfo,
                                       qsVariables       * aVariable,
                                       smiTableCursor    * aTableCursor,
                                       void              * aTableRow,
                                       scGRID              aGRID,
                                       qcmColumn         * aTableRowColumns,
                                       qcmTriggerRefType   aRefType );

    // value list를 PSM rowtype으로 변환하여 복사
    static IDE_RC copyRowFromValueList( qcTemplate   * aTemplate,
                                        qcmTableInfo * aTableInfo,
                                        qsVariables  * aVariable,
                                        void         * aValueList,
                                        qcmColumn    * aTableRowColumns );

    // row로 부터 value list를 복사
    static IDE_RC copyValueListFromRow( qcTemplate   * aTemplate,
                                        iduMemory    * aNewValueMem,
                                        qcmTableInfo * aTableInfo,
                                        qsVariables  * aVariable,
                                        void         * aValueList,
                                        qcmColumn    * aTableRowColumns );

    // BUG-20797
    // Trigger에서 참조하는 PSM의 변경을 검사한다.
    static IDE_RC checkObjects( qcStatement  * aStatement,
                                idBool       * aInvalidProc );

    // Trigger를 Recompile한다.
    static IDE_RC recompileTrigger( qcStatement     * aStatement,
                                    qcmTriggerInfo  * aTriggerInfo );

    //fix PROJ-1596
    static IDE_RC allocProcInfo( qcStatement * aStatement,
                                 UInt          aTriggerUserID );

    static IDE_RC getTriggerSCN( smOID    aTriggerOID,
                                 smSCN  * aTriggerSCN );

    /* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
    static IDE_RC convertXlobToXlobValue( mtcTemplate        * aTemplate,
                                          smiTableCursor     * aTableCursor,
                                          void               * aTableRow,
                                          scGRID               aGRID,
                                          mtcColumn          * aSrcLobColumn,
                                          void               * aSrcValue,
                                          mtcColumn          * aDestLobColumn,
                                          void               * aDestValue,
                                          qcmTriggerRefType    aRefType );

    /* PROJ-2219 Row-level before update trigger */
    static IDE_RC makeRefColumnList( qcStatement * aQcStmt );

    // BUG-38137 Trigger의 when condition에서 PSM을 호출할 수 없다.
    static IDE_RC checkNoSpFunctionCall( qtcNode * aNode );

    /* PROJ-2600 Online DDL for Tablespace Alteration */
    static IDE_RC makeNewTriggerStringForSwap( qcStatement      * aStatement,
                                               qdnTriggerCache  * aTriggerCache,
                                               SChar            * aTriggerStmtBuffer,
                                               void             * aTriggerHandle,
                                               smOID              aTriggerOID,
                                               UInt               aTableID,
                                               SChar            * aNewTriggerName,
                                               UInt               aNewTriggerNameLen,
                                               SChar            * aPeerTableNameStr,
                                               UInt               aPeerTableNameLen,
                                               qdnTriggerCache ** aNewTriggerCache );

};
#endif // _O_QDN_TRIGGER_H_
