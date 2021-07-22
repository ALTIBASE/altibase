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
 * $Id: qmgInsert.h 53265 2012-05-18 00:05:06Z seo0jun $
 *
 * Description :
 *     Insert Graph를 위한 정의
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMG_INSERT_H_
#define _O_QMG_INSERT_H_ 1

#include <qc.h>
#include <qmgDef.h>

//---------------------------------------------------
// Insert Graph의 Define 상수
//---------------------------------------------------


//---------------------------------------------------
// Insert Graph 를 관리하기 위한 자료 구조
//---------------------------------------------------

typedef struct qmgINST
{
    qmgGraph             graph;    // 공통 Graph 정보

    //---------------------------------
    // insert 관련 정보
    //---------------------------------
    
    struct qmsTableRef * tableRef;
    
    // insert columns
    qcmColumn          * columns;    // for INSERT ... SELECT ...
    qcmColumn          * columnsForValues;
    
    // insert values
    qmmMultiRows       * rows;       /* BUG-42764 Multi Row */
    UInt                 valueIdx;   // insert smiValues index
    UInt                 canonizedTuple;
    // PROJ-2264 Dictionary table
    UInt                 compressedTuple;
    // Proj - 1360 Queue
    void               * queueMsgIDSeq;

    // direct-path insert
    qmsHints           * hints;

    // sequence 정보
    qcParseSeqCaches   * nextValSeqs;
    
    // multi-table insert
    idBool               multiInsertSelect;
    
    // instead of trigger
    idBool               insteadOfTrigger;

    // BUG-43063 insert nowait
    ULong                lockWaitMicroSec;
    
    //---------------------------------
    // constraint 처리를 위한 정보
    //---------------------------------
    
    qcmParentInfo      * parentConstraints;
    qdConstraintSpec   * checkConstrList;

    //---------------------------------
    // return into 처리를 위한 정보
    //---------------------------------
    
    /* PROJ-1584 DML Return Clause */
    qmmReturnInto      * returnInto;
    
    //---------------------------------
    // PROJ-1090 Function-based Index
    //---------------------------------
    
    qmsTableRef        * defaultExprTableRef;
    qcmColumn          * defaultExprColumns;
    
} qmgINST;

//---------------------------------------------------
// Insert Graph 를 관리하기 위한 함수
//---------------------------------------------------

class qmgInsert
{
public:
    // Graph 의 초기화
    static IDE_RC  init( qcStatement      * aStatement,
                         qmmInsParseTree  * aParseTree,
                         qmgGraph         * aChildGraph,
                         qmgGraph        ** aGraph );

    // Graph의 최적화 수행
    static IDE_RC  optimize( qcStatement * aStatement, qmgGraph * aGraph );

    // Graph의 Plan Tree 생성
    static IDE_RC  makePlan( qcStatement    * aStatement,
                             const qmgGraph * aParent,
                             qmgGraph       * aGraph );

    // Graph의 공통 정보를 출력함.
    static IDE_RC  printGraph( qcStatement  * aStatement,
                               qmgGraph     * aGraph,
                               ULong          aDepth,
                               iduVarString * aString );

private:
    /* PROJ-2464 hybrid partitioned table 지원 */
    static void   fixHint( qmnPlan * aPlan );
};

#endif /* _O_QMG_INSERT_H_ */

