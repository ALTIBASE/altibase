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
 * $Id: qmgSet.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *     SET Graph를 위한 정의
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMG_SET_H_
#define _O_QMG_SET_H_ 1

#include <qc.h>
#include <qmgDef.h>
#include <qmsParseTree.h>

//---------------------------------------------------
// Set Graph의 Define 상수
//---------------------------------------------------

//---------------------------------------------------
// 상위 Graph가 qmgSet인지 아닌지에 대한 정보
// Plan Node 생성 시 사용
//    - 상위 Graph가 qmgSet이면 PROJ Node를 생성
//    - 상위 Graph가 qmgSet이 아니면 PROJ Node를 생성하지 않음
//---------------------------------------------------

// qmgSET.graph.flag
#define QMG_SET_PARENT_TYPE_SET_MASK    (0x10000000)
#define QMG_SET_PARENT_TYPE_SET_FALSE   (0x00000000)
#define QMG_SET_PARENT_TYPE_SET_TRUE    (0x10000000)

//---------------------------------------------------
// SET Graph 를 관리하기 위한 자료 구조
//---------------------------------------------------

typedef struct qmgSET
{
    qmgGraph      graph;          // 공통 Graph 정보

    qmsSetOpType  setOp;
    UInt          hashBucketCnt;    
    
} qmgSET;

//---------------------------------------------------
// SET Graph 를 관리하기 위한 함수
//---------------------------------------------------

class qmgSet
{
public:
    // Graph 의 초기화
    static IDE_RC  init( qcStatement * aStatement,
                         qmsQuerySet * aQuerySet,
                         qmgGraph    * aLeftGraph,
                         qmgGraph    * aRightGraph, 
                         qmgGraph   ** aGraph );

    // Graph의 최적화 수행
    static IDE_RC  optimize( qcStatement * aStatement, qmgGraph * aGraph );

    // Graph의 Plan Tree 생성
    static IDE_RC  makePlan( qcStatement * aStatement, const qmgGraph * aParent, qmgGraph * aGraph );

    static IDE_RC  makeUnion( qcStatement * aStatement,
                              qmgSET      * aMyGraph,
                              idBool        aIsUnionAll );

    static IDE_RC  makeIntersect( qcStatement * aStatement,
                                  qmgSET      * aMyGraph );

    static IDE_RC  makeMinus( qcStatement * aStatement,
                              qmgSET      * aMyGraph );

    // Graph의 공통 정보를 출력함.
    static IDE_RC  printGraph( qcStatement  * aStatement,
                               qmgGraph     * aGraph,
                               ULong          aDepth,
                               iduVarString * aString );

private:
    static IDE_RC  makeChild( qcStatement * aStatement,
                              qmgSET      * aMyGraph );

    //--------------------------------------------------
    // 최적화를 위한 함수
    //--------------------------------------------------

    // PROJ-1486 Multiple Bag Union

    // Multiple Bag Union 최적화 적용
    static IDE_RC optMultiBagUnion( qcStatement * aStatement,
                                    qmgSET      * aSETGraph );

    // Multiple Children 를 구성
    static IDE_RC linkChildGraph( qcStatement  * aStatement,
                                  qmgGraph     * aChildGraph,
                                  qmgChildren ** aChildren );
};

#endif /* _O_QMG_SET_H_ */

