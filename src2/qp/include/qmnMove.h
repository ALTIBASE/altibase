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
 * $Id: qmnInsert.h 55241 2012-08-27 09:13:19Z linkedlist $
 *
 * Description :
 *     MOVE Node
 *
 *     관계형 모델에서 insert를 수행하는 Plan Node 이다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMN_MOVE_H_
#define _O_QMN_MOVE_H_ 1

#include <mt.h>
#include <qmc.h>
#include <qmnDef.h>
#include <qcmTableInfo.h>
#include <qmmParseTree.h>
#include <qmcInsertCursor.h>
#include <qmsIndexTable.h>

//-----------------
// Code Node Flags
//-----------------

// qmncMOVE.flag
// Limit을 가지는지에 대한 여부
# define QMNC_MOVE_LIMIT_MASK               (0x00000001)
# define QMNC_MOVE_LIMIT_FALSE              (0x00000000)
# define QMNC_MOVE_LIMIT_TRUE               (0x00000001)

//-----------------
// Data Node Flags
//-----------------

// qmndMOVE.flag
# define QMND_MOVE_INIT_DONE_MASK           (0x00000001)
# define QMND_MOVE_INIT_DONE_FALSE          (0x00000000)
# define QMND_MOVE_INIT_DONE_TRUE           (0x00000001)

// qmndMOVE.flag
# define QMND_MOVE_CURSOR_MASK              (0x00000002)
# define QMND_MOVE_CURSOR_CLOSED            (0x00000000)
# define QMND_MOVE_CURSOR_OPEN              (0x00000002)

// qmndMOVE.flag
# define QMND_MOVE_INSERT_MASK              (0x00000004)
# define QMND_MOVE_INSERT_FALSE             (0x00000000)
# define QMND_MOVE_INSERT_TRUE              (0x00000004)

// qmndMOVE.flag
# define QMND_MOVE_REMOVE_MASK              (0x00000008)
# define QMND_MOVE_REMOVE_FALSE             (0x00000000)
# define QMND_MOVE_REMOVE_TRUE              (0x00000008)

// qmndMOVE.flag
# define QMND_MOVE_INSERT_INDEX_CURSOR_MASK     (0x00000010)
# define QMND_MOVE_INSERT_INDEX_CURSOR_NONE     (0x00000000)
# define QMND_MOVE_INSERT_INDEX_CURSOR_INITED   (0x00000010)

// qmndMOVE.flag
# define QMND_MOVE_DELETE_INDEX_CURSOR_MASK     (0x00000020)
# define QMND_MOVE_DELETE_INDEX_CURSOR_NONE     (0x00000000)
# define QMND_MOVE_DELETE_INDEX_CURSOR_INITED   (0x00000020)

typedef struct qmncMOVE  
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
    
    qmsTableRef         * targetTableRef;  // target table
    qmsTableRef         * tableRef;        // source table

    //---------------------------------
    // insert 관련 정보
    //---------------------------------

    // insert columns
    qcmColumn           * columns;

    // insert values
    qmmValueNode        * values;
    UInt                  valueIdx;   // insert smiValues index
    UInt                  canonizedTuple;
    // PROJ-2264 Dictionary table
    UInt                  compressedTuple;
    // sequence 정보
    qcParseSeqCaches    * nextValSeqs;
    
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

    //---------------------------------
    // PROJ-1784 DML Without Retry
    //---------------------------------
    
    smiColumnList       * whereColumnList;
    smiColumnList      ** wherePartColumnList;
    idBool                withoutRetry;
    
} qmncMOVE;

typedef struct qmndMOVE
{
    //---------------------------------
    // Data 영역 공통 정보
    //---------------------------------

    qmndPlan              plan;
    doItFunc              doIt;
    UInt                * flag;        

    //---------------------------------
    // MOVE 고유 정보
    //---------------------------------

    qmcInsertCursor       insertCursorMgr;

    mtcTuple            * deleteTuple;
    UShort                deleteTupleID;
    smiTableCursor      * deleteCursor;

    /* PROJ-2464 hybrid partitioned table 지원 */
    qcmTableInfo        * deletePartInfo;

    //---------------------------------
    // lob 처리를 위한 정보
    //---------------------------------
    
    struct qmxLobInfo   * lobInfo;

    //---------------------------------
    // Limitation 관련 정보
    //---------------------------------
    
    ULong                 limitCurrent;    // 현재 Limit 값
    ULong                 limitStart;      // 시작 Limit 값
    ULong                 limitEnd;        // 최종 Limit 값

    //---------------------------------
    // Trigger 처리를 위한 정보
    //---------------------------------

    // delete를 위한 old row
    qcmColumn           * columnsForRow;
    void                * oldRow;
    
    // trigger row가 필요여부 정보
    idBool                needTriggerRowForInsert;
    idBool                existTriggerForInsert;
    
    idBool                needTriggerRowForDelete;
    idBool                existTriggerForDelete;
   
    //---------------------------------
    // index table 처리를 위한 정보
    //---------------------------------

    qmsIndexTableCursors  insertIndexTableCursorInfo;
    qmsIndexTableCursors  deleteIndexTableCursorInfo;
    
    // selection에 사용된 index table cursor의 인자
    smiTableCursor      * indexDeleteCursor;
    
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

} qmndMOVE;

class qmnMOVE
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
    
    // check insert reference
    static IDE_RC checkInsertRef( qcTemplate * aTemplate,
                                  qmnPlan    * aPlan );
    
    // check delete reference
    static IDE_RC checkDeleteRef( qcTemplate * aTemplate,
                                  qmnPlan    * aPlan );
    
    // Cursor의 Close
    static IDE_RC closeCursor( qcTemplate * aTemplate,
                               qmnPlan    * aPlan );
    
private:    

    //------------------------
    // 초기화 관련 함수
    //------------------------

    // 최초 초기화
    static IDE_RC firstInit( qcTemplate * aTemplate,
                             qmncMOVE   * aCodePlan,
                             qmndMOVE   * aDataPlan );

    // cursor info 생성
    static IDE_RC allocCursorInfo( qcTemplate * aTemplate,
                                   qmncMOVE   * aCodePlan,
                                   qmndMOVE   * aDataPlan );
    
    // alloc trigger row
    static IDE_RC allocTriggerRow( qcTemplate * aTemplate,
                                   qmncMOVE   * aCodePlan,
                                   qmndMOVE   * aDataPlan );
    
    // alloc index table cursor
    static IDE_RC allocIndexTableCursor( qcTemplate * aTemplate,
                                         qmncMOVE   * aCodePlan,
                                         qmndMOVE   * aDataPlan );
    
    // 호출되어서는 안됨.
    static IDE_RC doItDefault( qcTemplate * aTemplate,
                               qmnPlan    * aPlan,
                               qmcRowFlag * aFlag );

    // 최초 MOVE을 수행
    static IDE_RC doItFirst( qcTemplate * aTemplate,
                             qmnPlan    * aPlan,
                             qmcRowFlag * aFlag );

    // 다음 MOVE을 수행
    static IDE_RC doItNext( qcTemplate * aTemplate,
                            qmnPlan    * aPlan,
                            qmcRowFlag * aFlag );

    // check trigger 
    static IDE_RC checkTrigger( qcTemplate * aTemplate,
                                qmnPlan    * aPlan );
    
    // Cursor의 Open
    static IDE_RC openInsertCursor( qcTemplate * aTemplate,
                                    qmnPlan    * aPlan );

    // Insert One Record
    static IDE_RC insertOneRow( qcTemplate * aTemplate,
                                qmnPlan    * aPlan );
    
    // Cursor의 Get
    static IDE_RC getCursor( qcTemplate * aTemplate,
                             qmnPlan    * aPlan,
                             idBool     * aIsTableCursorChanged );

    // Delete One Record
    static IDE_RC deleteOneRow( qcTemplate * aTemplate,
                                qmnPlan    * aPlan );
    
    // instead of trigger
    static IDE_RC fireInsteadOfTrigger( qcTemplate * aTemplate,
                                        qmnPlan    * aPlan );

    // check parent references
    static IDE_RC checkInsertChildRefOnScan( qcTemplate           * aTemplate,
                                             qmncMOVE             * aCodePlan,
                                             qcmTableInfo         * aTableInfo,
                                             UShort                 aTable,
                                             qmcInsertPartCursor  * aCursorIter,
                                             void                ** aRow );
    
    // check child references
    static IDE_RC checkDeleteChildRefOnScan( qcTemplate     * aTemplate,
                                             qmncMOVE       * aCodePlan,
                                             qcmTableInfo   * aTableInfo,
                                             mtcTuple       * aDeleteTuple );
    
    // insert index table
    static IDE_RC insertIndexTableCursor( qcTemplate     * aTemplate,
                                          qmndMOVE       * aDataPlan,
                                          smiValue       * aInsertValue,
                                          scGRID           aRowGRID );
    
    // update index table
    static IDE_RC deleteIndexTableCursor( qcTemplate     * aTemplate,
                                          qmncMOVE       * aCodePlan,
                                          qmndMOVE       * aDataPlan );
};

#endif /* _O_QMN_MOVE_H_ */
