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
 * $Id: qmnUpdate.h 55241 2012-08-27 09:13:19Z linkedlist $
 *
 * Description :
 *     UPTE(UPdaTE) Node
 *
 *     관계형 모델에서 delete를 수행하는 Plan Node 이다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMN_UPTE_H_
#define _O_QMN_UPTE_H_ 1

#include <mt.h>
#include <qc.h>
#include <qmc.h>
#include <qmoDef.h>
#include <qmnDef.h>
#include <qcmTableInfo.h>
#include <qmmParseTree.h>
#include <qmcInsertCursor.h>
#include <qmsIndexTable.h>

//-----------------
// Code Node Flags
//-----------------

// qmncUPTE.flag
// Limit을 가지는지에 대한 여부
# define QMNC_UPTE_LIMIT_MASK               (0x00000002)
# define QMNC_UPTE_LIMIT_FALSE              (0x00000000)
# define QMNC_UPTE_LIMIT_TRUE               (0x00000002)

// VIEW에 대한 update인지 여부
# define QMNC_UPTE_VIEW_MASK                (0x00000004)
# define QMNC_UPTE_VIEW_FALSE               (0x00000000)
# define QMNC_UPTE_VIEW_TRUE                (0x00000004)

// VIEW에 대한 update key preserved property
# define QMNC_UPTE_VIEW_KEY_PRESERVED_MASK  (0x00000008)
# define QMNC_UPTE_VIEW_KEY_PRESERVED_FALSE (0x00000000)
# define QMNC_UPTE_VIEW_KEY_PRESERVED_TRUE  (0x00000008)

# define QMNC_UPTE_PARTITIONED_MASK         (0x00000010)
# define QMNC_UPTE_PARTITIONED_FALSE        (0x00000000)
# define QMNC_UPTE_PARTITIONED_TRUE         (0x00000010)

# define QMNC_UPTE_MULTIPLE_TABLE_MASK      (0x00000020)
# define QMNC_UPTE_MULTIPLE_TABLE_FALSE     (0x00000000)
# define QMNC_UPTE_MULTIPLE_TABLE_TRUE      (0x00000020)

//-----------------
// Data Node Flags
//-----------------

// qmndUPTE.flag
# define QMND_UPTE_INIT_DONE_MASK           (0x00000001)
# define QMND_UPTE_INIT_DONE_FALSE          (0x00000000)
# define QMND_UPTE_INIT_DONE_TRUE           (0x00000001)

// qmndUPTE.flag
# define QMND_UPTE_CURSOR_MASK              (0x00000002)
# define QMND_UPTE_CURSOR_CLOSED            (0x00000000)
# define QMND_UPTE_CURSOR_OPEN              (0x00000002)

// qmndUPTE.flag
# define QMND_UPTE_UPDATE_MASK              (0x00000004)
# define QMND_UPTE_UPDATE_FALSE             (0x00000000)
# define QMND_UPTE_UPDATE_TRUE              (0x00000004)

// qmndUPTE.flag
# define QMND_UPTE_INDEX_CURSOR_MASK        (0x00000008)
# define QMND_UPTE_INDEX_CURSOR_NONE        (0x00000000)
# define QMND_UPTE_INDEX_CURSOR_INITED      (0x00000008)

// qmndUPTE.flag
// selected index table cursor를 update해야하는지 여부
# define QMND_UPTE_SELECTED_INDEX_CURSOR_MASK  (0x00000010)
# define QMND_UPTE_SELECTED_INDEX_CURSOR_NONE  (0x00000000)
# define QMND_UPTE_SELECTED_INDEX_CURSOR_TRUE  (0x00000010)

typedef struct qmncUPTE  
{
    //---------------------------------
    // Code 영역 공통 정보
    //---------------------------------

    qmnPlan               plan;
    UInt                  flag;
    UInt                  planID;

    //---------------------------------
    // querySet 관련 정보
    //---------------------------------
    
    qmsTableRef         * tableRef;

    //---------------------------------
    // update 관련 정보
    //---------------------------------
    
    // update columns
    qcmColumn           * columns;
    smiColumnList       * updateColumnList;
    UInt                * updateColumnIDs;
    UInt                  updateColumnCount;
    
    // update values
    qmmValueNode        * values;
    qmmSubqueries       * subqueries; // subqueries in set clause
    UInt                  valueIdx;   // update smiValues index
    UInt                  canonizedTuple;
    // PROJ-2264 Dictionary table
    UInt                  compressedTuple;
    mtdIsNullFunc       * isNull;
    
    // sequence 정보
    qcParseSeqCaches    * nextValSeqs;
    
    // instead of trigger인 경우
    idBool                insteadOfTrigger;

    // PROJ-2551 simple query 최적화
    idBool                isSimple;    // simple update
    qmnValueInfo        * simpleValues;
    UInt                  simpleValueBufSize;  // c-type value를 mt-type value로 변환
    
    //---------------------------------
    // partition 관련 정보
    //---------------------------------

    // row movement update인 경우
    qmsTableRef         * insertTableRef;
    smiColumnList      ** updatePartColumnList;
    idBool                isRowMovementUpdate;
    qmoUpdateType         updateType;

    //---------------------------------
    // cursor 관련 정보
    //---------------------------------
    
    // update only and insert/update/delete
    smiCursorType         cursorType;
    idBool                inplaceUpdate;
    
    //---------------------------------
    // Limitation 관련 정보
    //---------------------------------
    
    qmsLimit            * limit;

    //---------------------------------
    // constraint 처리를 위한 정보
    //---------------------------------
    
    qcmParentInfo       * parentConstraints;
    qcmRefChildInfo     * childConstraints;
    qdConstraintSpec    * checkConstrList;

    //---------------------------------
    // return into 처리를 위한 정보
    //---------------------------------
    
    /* PROJ-1584 DML Return Clause */
    qmmReturnInto       * returnInto;
    
    //---------------------------------
    // Display 관련 정보
    //---------------------------------

    qmsNamePosition       tableOwnerName;     // Table Owner Name
    qmsNamePosition       tableName;          // Table Name
    qmsNamePosition       aliasName;          // Alias Name

    //---------------------------------
    // PROJ-1090 Function-based Index
    //---------------------------------
    
    qmsTableRef         * defaultExprTableRef;
    qcmColumn           * defaultExprColumns;
    qcmColumn           * defaultExprBaseColumns;

    //---------------------------------
    // PROJ-1784 DML Without Retry
    //---------------------------------
    
    smiColumnList       * whereColumnList;
    smiColumnList      ** wherePartColumnList;
    smiColumnList       * setColumnList;
    smiColumnList      ** setPartColumnList;
    idBool                withoutRetry;

    /* PROJ-2714 Multiple Update Delete support */
    UInt                  mMultiTableCount;
    qmmMultiTables      * mTableList;
} qmncUPTE;

typedef struct qmndMultiTables
{
    UInt                   mFlag;
    UInt                   mIndexUpdateCount;
    qmsIndexTableCursors   mIndexTableCursorInfo;
    smiColumnList          mIndexUpdateColumnList[QC_MAX_KEY_COLUMN_COUNT + 2];
    struct qmxLobInfo    * mLobInfo;
    struct qmxLobInfo    * mInsertLobInfo;
    qmcInsertCursor        mInsertCursorMgr;   // row movement용 insert cursor
    smiValue             * mInsertValues;      // row movement용 insert smiValues
    smiValue             * mCheckValues;       // check row movement용 check smiValues
    void                 * mDefaultExprRowBuffer;
    qcmColumn            * mColumnsForRow;
    void                 * mOldRow;    // OLD ROW Reference를 위한 공간
    void                 * mNewRow;    // NEW ROW Reference를 위한 공간
    idBool                 mNeedTriggerRow;
    idBool                 mExistTrigger;
} qmndMultiTables;

typedef struct qmndUPTE
{
    //---------------------------------
    // Data 영역 공통 정보
    //---------------------------------

    qmndPlan              plan;
    doItFunc              doIt;
    UInt                * flag;        

    //---------------------------------
    // UPTE 고유 정보
    //---------------------------------

    mtcTuple            * updateTuple;
    UShort                updateTupleID;
    smiTableCursor      * updateCursor;
    
    /* PROJ-2464 hybrid partitioned table 지원 */
    qcmTableInfo        * updatePartInfo;

    // BUG-38129
    scGRID                rowGRID;
   
    //---------------------------------
    // partition 관련 정보
    //---------------------------------
    
    qmcInsertCursor       insertCursorMgr;   // row movement용 insert cursor
    smiValue            * insertValues;      // row movement용 insert smiValues
    smiValue            * checkValues;       // check row movement용 check smiValues

    //---------------------------------
    // lob 처리를 위한 정보
    //---------------------------------
    
    struct qmxLobInfo   * lobInfo;
    struct qmxLobInfo   * insertLobInfo;
    
    //---------------------------------
    // Limitation 관련 정보
    //---------------------------------
    
    ULong                 limitCurrent;    // 현재 Limit 값
    ULong                 limitStart;      // 시작 Limit 값
    ULong                 limitEnd;        // 최종 Limit 값

    //---------------------------------
    // Trigger 처리를 위한 정보
    //---------------------------------

    qcmColumn           * columnsForRow;
    void                * oldRow;    // OLD ROW Reference를 위한 공간
    void                * newRow;    // NEW ROW Reference를 위한 공간

    // PROJ-1705
    // trigger row가 필요여부 정보
    idBool                needTriggerRow;
    idBool                existTrigger;
    
    //---------------------------------
    // return into 처리를 위한 정보
    //---------------------------------

    // instead of trigger의 newRow는 smiValue이므로
    // return into를 위한 tableRow가 필요하다.
    void                * returnRow;
    
    //---------------------------------
    // index table 처리를 위한 정보
    //---------------------------------

    qmsIndexTableCursors  indexTableCursorInfo;

    // selection에 사용된 index table cursor의 인자
    // rowMovement로 oid,rid까지 update될 수 있다.
    smiColumnList         indexUpdateColumnList[QC_MAX_KEY_COLUMN_COUNT + 2];
    smiValue              indexUpdateValue[QC_MAX_KEY_COLUMN_COUNT + 2];
    smiTableCursor      * indexUpdateCursor;
    mtcTuple            * indexUpdateTuple;
    
    //---------------------------------
    // PROJ-1090 Function-based Index
    //---------------------------------
    
     void               * defaultExprRowBuffer;

    //---------------------------------
    // PROJ-1784 DML Without Retry
    //---------------------------------

    smiDMLRetryInfo       retryInfo;

    //---------------------------------
    // PROJ-2359 Table/Partition Access Option
    //---------------------------------

    qcmAccessOption       accessOption;

    qmndMultiTables     * mTableArray;
} qmndUPTE;

#define QMND_UPDATE_MULTI_TABLES( _dst_ )       \
{                                               \
    (_dst_)->mFlag                  = 0;        \
    (_dst_)->mIndexUpdateCount      = 0;        \
    (_dst_)->mLobInfo               = NULL;     \
    (_dst_)->mInsertLobInfo         = NULL;     \
    (_dst_)->mInsertValues          = NULL;     \
    (_dst_)->mCheckValues           = NULL;     \
    (_dst_)->mDefaultExprRowBuffer  = NULL;     \
    (_dst_)->mColumnsForRow         = NULL;     \
    (_dst_)->mOldRow                = NULL;     \
    (_dst_)->mNewRow                = NULL;     \
    (_dst_)->mNeedTriggerRow        = ID_FALSE; \
    (_dst_)->mExistTrigger          = ID_FALSE; \
}

class qmnUPTE
{
public:

    //------------------------
    // Base Function Pointer
    //------------------------

    // 초기화
    static IDE_RC init( qcTemplate * aTemplate,
                        qmnPlan    * aPlan );

    // 수행 함수
    static IDE_RC doIt( qcTemplate * aTemplate,
                        qmnPlan    * aPlan,
                        qmcRowFlag * aFlag );

    // Null Padding
    static IDE_RC padNull( qcTemplate * aTemplate,
                           qmnPlan    * aPlan );
    
    // Plan 정보 출력
    static IDE_RC printPlan( qcTemplate   * aTemplate,
                             qmnPlan      * aPlan,
                             ULong          aDepth,
                             iduVarString * aString,
                             qmnDisplay     aMode );
    
    // check update parent reference
    static IDE_RC checkUpdateParentRef( qcTemplate * aTemplate,
                                        qmnPlan    * aPlan );

    // check update child reference
    static IDE_RC checkUpdateChildRef( qcTemplate * aTemplate,
                                       qmnPlan    * aPlan );

    // Cursor의 Close
    static IDE_RC closeCursor( qcTemplate * aTemplate,
                               qmnPlan    * aPlan );    
    // BUG-38129
    static IDE_RC getLastUpdatedRowGRID( qcTemplate * aTemplate,
                                         qmnPlan    * aPlan,
                                         scGRID     * aRowGRID );

    /* BUG-39399 remove search key preserved table */
    static IDE_RC checkDuplicateUpdate( qmncUPTE   * aCodePlan,
                                        qmndUPTE   * aDataPlan );
    
    // Cursor의 Close
    static IDE_RC closeCursorMultiTable( qcTemplate * aTemplate,
                                         qmnPlan    * aPlan );

    static IDE_RC checkUpdateParentRefMultiTable( qcTemplate     * aTemplate,
                                                  qmnPlan        * aPlan,
                                                  qmmMultiTables * aTable,
                                                  UInt             aIndex );

    static IDE_RC checkUpdateChildRefMultiTable( qcTemplate     * aTemplate,
                                                 qmnPlan        * aPlan,
                                                 qmmMultiTables * aTable );
private:    

    //------------------------
    // 초기화 관련 함수
    //------------------------

    // 최초 초기화
    static IDE_RC firstInit( qcTemplate * aTemplate,
                             qmncUPTE   * aCodePlan,
                             qmndUPTE   * aDataPlan );

    // alloc cursor info
    static IDE_RC allocCursorInfo( qcTemplate * aTemplate,
                                   qmncUPTE   * aCodePlan,
                                   qmndUPTE   * aDataPlan );
    
    // alloc trigger row
    static IDE_RC allocTriggerRow( qcTemplate * aTemplate,
                                   qmncUPTE   * aCodePlan,
                                   qmndUPTE   * aDataPlan );
    
    // alloc return row
    static IDE_RC allocReturnRow( qcTemplate * aTemplate,
                                  qmncUPTE   * aCodePlan,
                                  qmndUPTE   * aDataPlan );
    
    // alloc index table cursor
    static IDE_RC allocIndexTableCursor( qcTemplate * aTemplate,
                                         qmncUPTE   * aCodePlan,
                                         qmndUPTE   * aDataPlan );
    
    // 호출되어서는 안됨.
    static IDE_RC doItDefault( qcTemplate * aTemplate,
                               qmnPlan    * aPlan,
                               qmcRowFlag * aFlag );

    // 최초 UPTE을 수행
    static IDE_RC doItFirst( qcTemplate * aTemplate,
                             qmnPlan    * aPlan,
                             qmcRowFlag * aFlag );

    // 다음 UPTE을 수행
    static IDE_RC doItNext( qcTemplate * aTemplate,
                            qmnPlan    * aPlan,
                            qmcRowFlag * aFlag );

    // check trigger 
    static IDE_RC checkTrigger( qcTemplate * aTemplate,
                                qmnPlan    * aPlan );
    
    // Cursor의 Get
    static IDE_RC getCursor( qcTemplate * aTemplate,
                             qmnPlan    * aPlan,
                             idBool     * aIsTableCursorChanged );

    // Cursor의 Open
    static IDE_RC openInsertCursor( qcTemplate * aTemplate,
                                    qmnPlan    * aPlan );

    // Update One Record
    static IDE_RC updateOneRow( qcTemplate * aTemplate,
                                qmnPlan    * aPlan );
    
    // Update One Record
    static IDE_RC updateOneRowForRowmovement( qcTemplate * aTemplate,
                                              qmnPlan    * aPlan );
    
    // Update One Record
    static IDE_RC updateOneRowForCheckRowmovement( qcTemplate * aTemplate,
                                                   qmnPlan    * aPlan );

    // instead of trigger
    static IDE_RC fireInsteadOfTrigger( qcTemplate * aTemplate,
                                        qmnPlan    * aPlan );

    // check parent reference
    static IDE_RC checkUpdateParentRefOnScan( qcTemplate   * aTemplate,
                                              qmncUPTE     * aCodePlan,
                                              mtcTuple     * aUpdateTuple );
    
    // check child reference
    static IDE_RC checkUpdateChildRefOnScan( qcTemplate     * aTemplate,
                                             qmncUPTE       * aCodePlan,
                                             qcmTableInfo   * aTableInfo,
                                             mtcTuple       * aUpdateTuple );

    // update index table
    static IDE_RC updateIndexTableCursor( qcTemplate     * aTemplate,
                                          qmncUPTE       * aCodePlan,
                                          qmndUPTE       * aDataPlan,
                                          smiValue       * aUpdateValue );
    
    // update index table
    static IDE_RC updateIndexTableCursorForRowMovement( qcTemplate     * aTemplate,
                                                        qmncUPTE       * aCodePlan,
                                                        qmndUPTE       * aDataPlan,
                                                        smOID            aPartOID,
                                                        scGRID           aRowGRID,
                                                        smiValue       * aUpdateValue );
    // 최초 초기화
    static IDE_RC firstInitMultiTable( qcTemplate * aTemplate,
                                       qmncUPTE   * aCodePlan,
                                       qmndUPTE   * aDataPlan );

    static IDE_RC allocIndexTableCursorMultiTable( qcTemplate * aTemplate,
                                                   qmncUPTE   * aCodePlan,
                                                   qmndUPTE   * aDataPlan );
    // 최초 UPTE을 수행
    static IDE_RC doItFirstMultiTable( qcTemplate * aTemplate,
                                       qmnPlan    * aPlan,
                                       qmcRowFlag * aFlag );

    // 다음 UPTE을 수행
    static IDE_RC doItNextMultiTable( qcTemplate * aTemplate,
                                      qmnPlan    * aPlan,
                                      qmcRowFlag * aFlag );
    // Cursor의 Get
    static IDE_RC getCursorMultiTable( qcTemplate     * aTemplate,
                                       qmncUPTE       * aCodePlan,
                                       qmndUPTE       * aDataPlan,
                                       qmmMultiTables * aTable,
                                       idBool         * aIsSkip );
    // Update One Record
    static IDE_RC updateOneRowMultiTable( qcTemplate     * aTemplate,
                                          qmncUPTE       * aCodePlan,
                                          qmndUPTE       * aDataPlan,
                                          qmmMultiTables * aTable,
                                          UInt             aIndex );

    static IDE_RC checkSkipMultiTable( qcTemplate     * aTemplate,
                                       qmndUPTE       * aDataPlan,
                                       qmmMultiTables * aTable,
                                       idBool         * aIsSkip );

    // update index table
    static IDE_RC updateIndexTableCursorMultiTable( qcTemplate      * aTemplate,
                                                    qmndUPTE        * aDataPlan,
                                                    qmmMultiTables  * aTable,
                                                    UInt              aIndex,
                                                    smiValue        * aUpdateValue );

    static IDE_RC allocTriggerRowMultiTable( qcTemplate * aTemplate,
                                             qmncUPTE   * aCodePlan,
                                             qmndUPTE   * aDataPlan );

    static IDE_RC checkTriggerMultiUpdate( qcTemplate     * aTemplate,
                                           qmndUPTE       * aDataPlan,
                                           qmmMultiTables * aTable,
                                           UInt             aIndex );

    static IDE_RC fireInsteadOfTriggerMultiUpdate( qcTemplate     * aTemplate,
                                                   qmncUPTE       * aCodePlan,
                                                   qmndUPTE       * aDataPlan,
                                                   qmmMultiTables * aTable,
                                                   UInt             aIndex );

    static IDE_RC updateOneRowForCheckRowmovementMultiTable( qcTemplate     * aTemplate,
                                                             qmncUPTE       * aCodePlan,
                                                             qmndUPTE       * aDataPlan,
                                                             qmmMultiTables * aTable,
                                                             UInt             aIndex );
    // Cursor의 Open
    static IDE_RC openInsertCursorMultiTable( qcTemplate     * aTemplate,
                                              qmndUPTE       * aDataPlan,
                                              qmmMultiTables * aTable,
                                              UInt             aIndex );
    // Update One Record
    static IDE_RC updateOneRowForRowmovementMultiTable( qcTemplate     * aTemplate,
                                                        qmncUPTE       * aCodePlan,
                                                        qmndUPTE       * aDataPlan,
                                                        qmmMultiTables * aTable,
                                                        UInt             aIndex );

    static IDE_RC updateIndexTableCursorRowMoveMultiTable( qcTemplate      * aTemplate,
                                                           qmndUPTE        * aDataPlan,
                                                           qmmMultiTables  * aTable,
                                                           UInt              aIndex,
                                                           smOID             aPartOID,
                                                           scGRID            aRowGRID,
                                                           smiValue        * aUpdateValue );

    static IDE_RC checkUpdateParentRefOnScanMultiTable( qcTemplate     * aTemplate,
                                                        qmmMultiTables * aTable,
                                                        mtcTuple       * aUpdateTuple );

    static IDE_RC checkUpdateChildRefOnScanMultiTable( qcTemplate     * aTemplate,
                                                       qmmMultiTables * aTable,
                                                       qcmTableInfo   * aTableInfo,
                                                       mtcTuple       * aUpdateTuple );
};

#endif /* _O_QMN_UPTE_H_ */
