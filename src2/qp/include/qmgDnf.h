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
 * $Id: qmgDnf.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *     DNF Graph를 위한 정의
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMG_DNF_H_
#define _O_QMG_DNF_H_ 1

#include <qc.h>
#include <qmgDef.h>

//---------------------------------------------------
// DNF Graph 를 관리하기 위한 자료 구조
//    - DNF Not Filter는 qmgGraph.myPredicate을 사용
//---------------------------------------------------

typedef struct qmgDNF
{
    qmgGraph graph;  // 공통 Graph 정보
    
} qmgDNF;

//---------------------------------------------------
// DNF Graph 를 관리하기 위한 함수
//---------------------------------------------------

class qmgDnf
{
public:
    // Graph 의 초기화
    static IDE_RC  init( qcStatement * aStatement,
                         qtcNode     * aDnfNotFilter,
                         qmgGraph    * aLeftGraph,
                         qmgGraph    * aRightGraph,
                         qmgGraph    * aGraph );

    // Graph의 최적화 수행
    static IDE_RC  optimize( qcStatement * aStatement, qmgGraph * aGraph );

    // Graph의 Plan Tree 생성
    static IDE_RC  makePlan( qcStatement * aStatement, const qmgGraph * aParent, qmgGraph * aGraph );

    // Graph의 공통 정보를 출력함.
    static IDE_RC  printGraph( qcStatement  * aStatement,
                               qmgGraph     * aGraph,
                               ULong          aDepth,
                               iduVarString * aString );

private:
    
};

#endif /* _O_QMG_DNF_H_ */

