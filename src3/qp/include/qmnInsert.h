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
 *     INST(INSerT) Node
 *
 *     관계형 모델에서 insert를 수행하는 Plan Node 이다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMN_INST_H_
#define _O_QMN_INST_H_ 1

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
#define QMNC_INST_PARTITIONED_MASK  (0x00000001)
#define QMNC_INST_PARTITIONED_FALSE (0x00000000)
#define QMNC_INST_PARTITIONED_TRUE  (0x00000001)

/* BUG-47625 Insert execution 성능향상 */
#define QMNC_INST_COMPLEX_MASK  (0x00000002)
#define QMNC_INST_COMPLEX_FALSE (0x00000000)
#define QMNC_INST_COMPLEX_TRUE  (0x00000002)

#define QMNC_INST_EXIST_NOTNULL_COLUMN_MASK  (0x00000004)
#define QMNC_INST_EXIST_NOTNULL_COLUMN_FALSE (0x00000000)
#define QMNC_INST_EXIST_NOTNULL_COLUMN_TRUE  (0x00000004)

#define QMNC_INST_EXIST_ENCRYPT_COLUMN_MASK  (0x00000008)
#define QMNC_INST_EXIST_ENCRYPT_COLUMN_FALSE (0x00000000)
#define QMNC_INST_EXIST_ENCRYPT_COLUMN_TRUE  (0x00000008)

#define QMNC_INST_EXIST_TIMESTAMP_MASK  (0x00000010)
#define QMNC_INST_EXIST_TIMESTAMP_FALSE (0x00000000)
#define QMNC_INST_EXIST_TIMESTAMP_TRUE  (0x00000010)

#define QMNC_INST_EXIST_QUEUE_MASK  (0x00000020)
#define QMNC_INST_EXIST_QUEUE_FALSE (0x00000000)
#define QMNC_INST_EXIST_QUEUE_TRUE  (0x00000020)

//-----------------
// Data Node Flags
//-----------------

// qmndINST.flag
# define QMND_INST_INIT_DONE_MASK           (0x00000001)
# define QMND_INST_INIT_DONE_FALSE          (0x00000000)
# define QMND_INST_INIT_DONE_TRUE           (0x00000001)

// qmndINST.flag
# define QMND_INST_CURSOR_MASK              (0x00000002)
# define QMND_INST_CURSOR_CLOSED            (0x00000000)
# define QMND_INST_CURSOR_OPEN              (0x00000002)

// qmndINST.flag
# define QMND_INST_INSERT_MASK              (0x00000004)
# define QMND_INST_INSERT_FALSE             (0x00000000)
# define QMND_INST_INSERT_TRUE              (0x00000004)

// qmndINST.flag
# define QMND_INST_INDEX_CURSOR_MASK        (0x00000008)
# define QMND_INST_INDEX_CURSOR_NONE        (0x00000000)
# define QMND_INST_INDEX_CURSOR_INITED      (0x00000008)

// qmndINST.flag
# define QMND_INST_ATOMIC_MASK              (0x00000010)
# define QMND_INST_ATOMIC_FALSE             (0x00000000)
# define QMND_INST_ATOMIC_TRUE              (0x00000010)

typedef struct qmncINST  
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
    // insert 관련 정보
    //---------------------------------

    idBool                isInsertSelect;
    idBool                isMultiInsertSelect;

    // update columns
    qcmColumn           * columns;
    qcmColumn           * columnsForValues;

    // update values
    qmmMultiRows        * rows;       /* BUG-42764 Multi Row */
    UInt                  valueIdx;   // update smiValues index
    UInt                  canonizedTuple;
    // PROJ-2264 Dictionary table
    UInt                  compressedTuple;
    void                * queueMsgIDSeq;

    // direct-path insert
    qmsHints            * hints;
    idBool                isAppend;
    idBool                disableTrigger;

    // sequence 정보
    qcParseSeqCaches    * nextValSeqs;
    
    // instead of trigger인 경우
    idBool                insteadOfTrigger;

    // PROJ-2551 simple query 최적화
    idBool                isSimple;    // simple insert
    UInt                  simpleValueCount;
    qmnValueInfo        * simpleValues;
    UInt                  simpleValueBufSize;  // c-type value를 mt-type value로 변환

    // BUG-43063 insert nowait
    ULong                 lockWaitMicroSec;
    
    //---------------------------------
    // constraint 처리를 위한 정보
    //---------------------------------

    qcmParentInfo       * parentConstraints;
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
    
} qmncINST;

typedef struct qmndINST
{
    //---------------------------------
    // Data 영역 공통 정보
    //---------------------------------

    qmndPlan              plan;
    doItFunc              doIt;
    UInt                * flag;        

    //---------------------------------
    // INST 고유 정보
    //---------------------------------

    qmcInsertCursor       insertCursorMgr;

    // PROJ-1566
    UInt                  parallelDegree;
    
    // BUG-38129
    scGRID                rowGRID;

    /* BUG-42764 Multi Row */
    qmmMultiRows        * rows;   // Current row
    //---------------------------------
    // lob 처리를 위한 정보
    //---------------------------------
    
    struct qmxLobInfo   * lobInfo;

    //---------------------------------
    // Trigger 처리를 위한 정보
    //---------------------------------

    qcmColumn           * columnsForRow;
    
    // PROJ-1705
    // trigger row가 필요여부 정보
    idBool                needTriggerRow;
    idBool                existTrigger;
    
    idBool                isAppend;
    //---------------------------------
    // return into 처리를 위한 정보
    //---------------------------------

    // instead of trigger의 newRow는 smiValue이므로
    // return into를 위한 tableRow가 필요하다.
    mtcTuple            * viewTuple;    
    void                * returnRow;
    
    //---------------------------------
    // index table 처리를 위한 정보
    //---------------------------------

    qmsIndexTableCursors  indexTableCursorInfo;

    //---------------------------------
    // PROJ-1090 Function-based Index
    //---------------------------------
    
    void               * defaultExprRowBuffer;

} qmndINST;

class qmnINST
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
    
    // Cursor의 Close
    static IDE_RC closeCursor( qcTemplate * aTemplate,
                               qmnPlan    * aPlan );
    
    // BUG-38129
    static IDE_RC getLastInsertedRowGRID( qcTemplate * aTemplate,
                                          qmnPlan    * aPlan,
                                          scGRID     * aRowGRID );

private:    

    //------------------------
    // 초기화 관련 함수
    //------------------------

    // 최초 초기화
    static IDE_RC firstInit( qcTemplate * aTemplate,
                             qmncINST   * aCodePlan,
                             qmndINST   * aDataPlan );

    // alloc trigger row
    static IDE_RC allocTriggerRow( qmncINST   * aCodePlan,
                                   qmndINST   * aDataPlan );
    
    // alloc return row
    static IDE_RC allocReturnRow( qcTemplate * aTemplate,
                                  qmncINST   * aCodePlan,
                                  qmndINST   * aDataPlan );
    
    // alloc index table cursor
    static IDE_RC allocIndexTableCursor( qcTemplate * aTemplate,
                                         qmncINST   * aCodePlan,
                                         qmndINST   * aDataPlan );
    
    // 호출되어서는 안됨.
    static IDE_RC doItDefault( qcTemplate * aTemplate,
                               qmnPlan    * aPlan,
                               qmcRowFlag * aFlag );

    // 최초 INST을 수행
    static IDE_RC doItFirst( qcTemplate * aTemplate,
                             qmnPlan    * aPlan,
                             qmcRowFlag * aFlag );

    // 다음 INST을 수행
    static IDE_RC doItNext( qcTemplate * aTemplate,
                            qmnPlan    * aPlan,
                            qmcRowFlag * aFlag );

    // 최초 INST을 수행
    static IDE_RC doItFirstMultiRows( qcTemplate * aTemplate,
                                      qmnPlan    * aPlan,
                                      qmcRowFlag * aFlag );

    // 다음 INST을 수행
    static IDE_RC doItNextMultiRows( qcTemplate * aTemplate,
                                     qmnPlan    * aPlan,
                                     qmcRowFlag * aFlag );

    // 최초 INST을 수행
    /* BUG-47625 Insert execution 성능향상 */
    static IDE_RC doItOneRow( qcTemplate * aTemplate,
                              qmnPlan    * aPlan,
                              qmcRowFlag * aFlag );

    // check trigger 
    static IDE_RC checkTrigger( qcTemplate * aTemplate,
                                qmnPlan    * aPlan );
    
    // Cursor의 Open
    static IDE_RC openCursor( qcTemplate * aTemplate,
                              qmnPlan    * aPlan );

    // Insert One Record
    static IDE_RC insertOneRow( qcTemplate * aTemplate,
                                qmnPlan    * aPlan );
    
    // Insert One Record
    static IDE_RC insertOnce( qcTemplate * aTemplate,
                              qmnPlan    * aPlan );
    
    // instead of trigger
    static IDE_RC fireInsteadOfTrigger( qcTemplate * aTemplate,
                                        qmnPlan    * aPlan );

    // check parent references
    static IDE_RC checkInsertChildRefOnScan( qcTemplate           * aTemplate,
                                             qmncINST             * aCodePlan,
                                             qcmTableInfo         * aTableInfo,
                                             UShort                 aTable,
                                             qmcInsertPartCursor  * aCursorIter,
                                             void                ** aRow );
    
    // insert index table
    static IDE_RC insertIndexTableCursor( qcTemplate     * aTemplate,
                                          qmndINST       * aDataPlan,
                                          smiValue       * aInsertValue,
                                          scGRID           aRowGRID );
};

#endif /* _O_QMN_INST_H_ */
