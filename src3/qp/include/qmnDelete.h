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
 * $Id: qmnDelete.h 55241 2012-08-27 09:13:19Z linkedlist $
 *
 * Description :
 *     DETE(DEleTE) Node
 *
 *     관계형 모델에서 delete를 수행하는 Plan Node 이다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMN_DETE_H_
#define _O_QMN_DETE_H_ 1

#include <mt.h>
#include <qmc.h>
#include <qmnDef.h>
#include <qcmTableInfo.h>
#include <qmmParseTree.h>
#include <qmsIndexTable.h>

//-----------------
// Code Node Flags
//-----------------

// qmncDETE.flag
// Limit을 가지는지에 대한 여부
# define QMNC_DETE_LIMIT_MASK               (0x00000002)
# define QMNC_DETE_LIMIT_FALSE              (0x00000000)
# define QMNC_DETE_LIMIT_TRUE               (0x00000002)

// qmncDETE.flag
// VIEW에 대한 delete인지 여부
# define QMNC_DETE_VIEW_MASK                (0x00000004)
# define QMNC_DETE_VIEW_FALSE               (0x00000000)
# define QMNC_DETE_VIEW_TRUE                (0x00000004)

// qmncDETE.flag
// VIEW에 대한 update key preserved property
# define QMNC_DETE_VIEW_KEY_PRESERVED_MASK  (0x00000008)
# define QMNC_DETE_VIEW_KEY_PRESERVED_FALSE (0x00000000)
# define QMNC_DETE_VIEW_KEY_PRESERVED_TRUE  (0x00000008)

# define QMNC_DETE_PARTITIONED_MASK         (0x00000010)
# define QMNC_DETE_PARTITIONED_FALSE        (0x00000000)
# define QMNC_DETE_PARTITIONED_TRUE         (0x00000010)

# define QMNC_DETE_MULTIPLE_TABLE_MASK      (0x00000020)
# define QMNC_DETE_MULTIPLE_TABLE_FALSE     (0x00000000)
# define QMNC_DETE_MULTIPLE_TABLE_TRUE      (0x00000020)

//-----------------
// Data Node Flags
//-----------------

// qmndDETE.flag
# define QMND_DETE_INIT_DONE_MASK           (0x00000001)
# define QMND_DETE_INIT_DONE_FALSE          (0x00000000)
# define QMND_DETE_INIT_DONE_TRUE           (0x00000001)

// qmndDETE.flag
# define QMND_DETE_CURSOR_MASK              (0x00000002)
# define QMND_DETE_CURSOR_CLOSED            (0x00000000)
# define QMND_DETE_CURSOR_OPEN              (0x00000002)

// qmndDETE.flag
# define QMND_DETE_REMOVE_MASK              (0x00000004)
# define QMND_DETE_REMOVE_FALSE             (0x00000000)
# define QMND_DETE_REMOVE_TRUE              (0x00000004)

// qmndDETE.flag
# define QMND_DETE_INDEX_CURSOR_MASK        (0x00000008)
# define QMND_DETE_INDEX_CURSOR_NONE        (0x00000000)
# define QMND_DETE_INDEX_CURSOR_INITED      (0x00000008)

typedef struct qmncDETE  
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
    // delete 관련 정보
    //---------------------------------
    
    // instead of trigger인 경우
    idBool                insteadOfTrigger;

    // PROJ-2551 simple query 최적화
    idBool                isSimple;    // simple delete
    
    //---------------------------------
    // Limitation 관련 정보
    //---------------------------------
    
    qmsLimit            * limit;

    //---------------------------------
    // child constraint 처리를 위한 정보
    //---------------------------------

    qcmRefChildInfo     * childConstraints;

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
    // PROJ-1784 DML Without Retry
    //---------------------------------
    
    smiColumnList        * whereColumnList;
    smiColumnList       ** wherePartColumnList;
    idBool                 withoutRetry;

    /* PROJ-2714 Multiple Update Delete support */
    UInt                   mMultiTableCount;
    qmmDelMultiTables    * mTableList;
} qmncDETE;

typedef struct qmndDelMultiTables
{
    UInt                   mFlag;
    qmsIndexTableCursors   mIndexTableCursorInfo;
    qcmColumn            * mColumnsForRow;
    idBool                 mNeedTriggerRow;
    idBool                 mExistTrigger;
    smiTableCursor       * mIndexDeleteCursor;
    void                 * mOldRow;    // OLD ROW Reference를 위한 공간
} qmndDelMultiTables;

typedef struct qmndDETE
{
    //---------------------------------
    // Data 영역 공통 정보
    //---------------------------------
    qmndPlan              plan;
    doItFunc              doIt;
    UInt                * flag;        

    //---------------------------------
    // DETE 고유 정보
    //---------------------------------

    mtcTuple            * deleteTuple;
    UShort                deleteTupleID;
    smiTableCursor      * deleteCursor;

    /* PROJ-2464 hybrid partitioned table 지원 */
    qcmTableInfo        * deletePartInfo;

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

    // PROJ-1705
    // trigger row가 필요여부 정보
    idBool                needTriggerRow;
    idBool                existTrigger;
    
    //---------------------------------
    // return into 처리를 위한 정보
    //---------------------------------

    // instead of trigger의 oldRow는 smiValue이므로
    // return into를 위한 tableRow가 필요하다.
    void                * returnRow;
    
    //---------------------------------
    // index table 처리를 위한 정보
    //---------------------------------

    qmsIndexTableCursors  indexTableCursorInfo;

    // selection에 사용된 index table cursor의 인자
    smiTableCursor      * indexDeleteCursor;
    mtcTuple            * indexDeleteTuple;

    //---------------------------------
    // PROJ-1784 DML Without Retry
    //---------------------------------

    smiDMLRetryInfo       retryInfo;

    //---------------------------------
    // PROJ-2359 Table/Partition Access Option
    //---------------------------------

    qcmAccessOption       accessOption;

    qmndDelMultiTables  * mTableArray;
} qmndDETE;

#define QMND_DETE_MULTI_TABLES( _dst_ )         \
{                                               \
    (_dst_)->mFlag                  = 0;        \
    (_dst_)->mColumnsForRow         = NULL;     \
    (_dst_)->mNeedTriggerRow        = ID_FALSE; \
    (_dst_)->mExistTrigger          = ID_FALSE; \
    (_dst_)->mIndexDeleteCursor     = NULL;     \
    (_dst_)->mOldRow                = NULL;     \
}

class qmnDETE
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
    
    // check delete reference
    static IDE_RC checkDeleteRef( qcTemplate * aTemplate,
                                  qmnPlan    * aPlan );
    
    // Cursor의 Close
    static IDE_RC closeCursor( qcTemplate * aTemplate,
                               qmnPlan    * aPlan );

    /* BUG-39399 remove search key preserved table */
    static IDE_RC checkDuplicateDelete( qmncDETE   * aCodePlan,
                                        qmndDETE   * aDataPlan );

    // Cursor의 Close
    static IDE_RC closeCursorMultiTable( qcTemplate * aTemplate,
                                         qmnPlan    * aPlan );

    static IDE_RC checkDeleteRefMultiTable( qcTemplate        * aTemplate,
                                            qmnPlan           * aPlan,
                                            qmmDelMultiTables * aTable,
                                            UInt                aIndex );
private:

    //------------------------
    // 초기화 관련 함수
    //------------------------

    // 최초 초기화
    static IDE_RC firstInit( qcTemplate * aTemplate,
                             qmncDETE   * aCodePlan,
                             qmndDETE   * aDataPlan );

    // cursor info 생성
    static IDE_RC allocCursorInfo( qcTemplate * aTemplate,
                                   qmncDETE   * aCodePlan,
                                   qmndDETE   * aDataPlan );
    
    // alloc trigger row
    static IDE_RC allocTriggerRow( qcTemplate * aTemplate,
                                   qmncDETE   * aCodePlan,
                                   qmndDETE   * aDataPlan );
    
    // alloc return row
    static IDE_RC allocReturnRow( qcTemplate * aTemplate,
                                  qmncDETE   * aCodePlan,
                                  qmndDETE   * aDataPlan );
    
    // alloc index table cursor
    static IDE_RC allocIndexTableCursor( qcTemplate * aTemplate,
                                         qmncDETE   * aCodePlan,
                                         qmndDETE   * aDataPlan );
    
    // 호출되어서는 안됨.
    static IDE_RC doItDefault( qcTemplate * aTemplate,
                               qmnPlan    * aPlan,
                               qmcRowFlag * aFlag );

    // 최초 DETE을 수행
    static IDE_RC doItFirst( qcTemplate * aTemplate,
                             qmnPlan    * aPlan,
                             qmcRowFlag * aFlag );

    // 다음 DETE을 수행
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

    // Delete One Record
    static IDE_RC deleteOneRow( qcTemplate * aTemplate,
                                qmnPlan    * aPlan );
    
    // instead of trigger
    static IDE_RC fireInsteadOfTrigger( qcTemplate * aTemplate,
                                        qmnPlan    * aPlan );

    // check child references
    static IDE_RC checkDeleteChildRefOnScan( qcTemplate     * aTemplate,
                                             qmncDETE       * aCodePlan,
                                             qcmTableInfo   * aTableInfo,
                                             mtcTuple       * aDeleteTuple );
    
    // update index table
    static IDE_RC deleteIndexTableCursor( qcTemplate     * aTemplate,
                                          qmncDETE       * aCodePlan,
                                          qmndDETE       * aDataPlan );

    static IDE_RC firstInitMultiTable( qcTemplate * aTemplate,
                                       qmncDETE   * aCodePlan,
                                       qmndDETE   * aDataPlan );

    static IDE_RC doItFirstMultiTable( qcTemplate * aTemplate,
                                       qmnPlan    * aPlan,
                                       qmcRowFlag * aFlag );

    // 다음 DETE을 수행
    static IDE_RC doItNextMultiTable( qcTemplate * aTemplate,
                                      qmnPlan    * aPlan,
                                      qmcRowFlag * aFlag );

    static IDE_RC getCursorMultiTable( qcTemplate        * aTemplate,
                                       qmncDETE          * aCodePlan,
                                       qmndDETE          * aDataPlan,
                                       qmmDelMultiTables * aTable,
                                       idBool            * aIsSkip );

    static IDE_RC deleteOneRowMultiTable( qcTemplate        * aTemplate,
                                          qmndDETE          * aDataPlan,
                                          qmmDelMultiTables * aTable,
                                          UInt                aIndex );

    static IDE_RC checkSkipMultiTable( qcTemplate        * aTemplate,
                                       qmndDETE          * aDataPlan,
                                       qmmDelMultiTables * aTable,
                                       idBool            * aIsSkip );

    static IDE_RC deleteIndexTableCursorMultiTable( qcTemplate        * aTemplate,
                                                    qmndDETE          * aDataPlan,
                                                    qmmDelMultiTables * aTable,
                                                    UInt                aIndex );

    static IDE_RC allocIndexTableCursorMultiTable( qcTemplate * aTemplate,
                                                   qmncDETE   * aCodePlan,
                                                   qmndDETE   * aDataPlan );

    static IDE_RC allocTriggerRowMultiTable( qcTemplate * aTemplate,
                                             qmncDETE   * aCodePlan,
                                             qmndDETE   * aDataPlan );

    static IDE_RC checkTriggerMultiTable( qcTemplate        * aTemplate,
                                          qmndDETE          * aDataPlan,
                                          qmmDelMultiTables * aTable,
                                          UInt                aIndex );

    static IDE_RC fireInsteadOfTriggerMultiTable( qcTemplate        * aTemplate,
                                                  qmndDETE          * aDataPlan,
                                                  qmmDelMultiTables * aTable,
                                                  UInt                aIndex );

    static IDE_RC checkDeleteChildRefOnScanMultiTable( qcTemplate        * aTemplate,
                                                       qmmDelMultiTables * aTable,
                                                       qcmTableInfo      * aTableInfo,
                                                       mtcTuple          * aDeleteTuple );
};

#endif /* _O_QMN_DETE_H_ */
