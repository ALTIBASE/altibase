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
 * $Id: qmgDelete.h 53265 2012-05-18 00:05:06Z seo0jun $
 *
 * Description :
 *     Delete Graph를 위한 정의
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMG_DELETE_H_
#define _O_QMG_DELETE_H_ 1

#include <qc.h>
#include <qmgDef.h>

//---------------------------------------------------
// Delete Graph의 Define 상수
//---------------------------------------------------


//---------------------------------------------------
// Delete Graph 를 관리하기 위한 자료 구조
//---------------------------------------------------

typedef struct qmgDETE
{
    qmgGraph             graph;    // 공통 Graph 정보

    //---------------------------------
    // delete 관련 정보
    //---------------------------------
    
    qmsTableRef        * deleteTableRef; // delete target table
    
    idBool               insteadOfTrigger;  // instead of trigger

    //---------------------------------
    // Limitation 관련 정보
    //---------------------------------
    
    qmsLimit           * limit;   // limit 정보

    //---------------------------------
    // constraint 처리를 위한 정보
    //---------------------------------
    
    qcmRefChildInfo    * childConstraints;

    //---------------------------------
    // return into 처리를 위한 정보
    //---------------------------------
    
    /* PROJ-1584 DML Return Clause */
    qmmReturnInto      * returnInto;

    /* PROJ-2714 Multiple Update Delete support */
    qmmDelMultiTables  * mTableList;
} qmgDETE;

//---------------------------------------------------
// Delete Graph 를 관리하기 위한 함수
//---------------------------------------------------

class qmgDelete
{
public:
    // Graph 의 초기화
    static IDE_RC  init( qcStatement * aStatement,
                         qmsQuerySet * aQuerySet,
                         qmgGraph    * aChildGraph,
                         qmgGraph   ** aGraph );

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
};

#endif /* _O_QMG_DELETE_H_ */

