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
 * $Id: qmoNonCrtPathMgr.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *     Non Critical Path Manager
 *
 *     Critical Path 이외의 부분에 대한 최적화 및 그래프를 생성한다.
 *     즉, 다음과 같은 부분에 대한 그래프 최적화를 수행한다.
 *         - Projection Graph
 *         - Set Graph
 *         - Sorting Graph
 *         - Distinction Graph
 *         - Grouping Graph
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMO_NON_CRT_PATH_MGR_H_
#define _O_QMO_NON_CRT_PATH_MGR_H_ 1

#include <qc.h>
#include <qmgDef.h>
#include <qmsParseTree.h>
#include <qmoCrtPathMgr.h>

//---------------------------------------------------
// Non Critical Path를 관리하기 위한 자료 구조
//---------------------------------------------------

// qmoNonCrtPath.flag
#define QMO_NONCRT_PATH_INITIALIZE      (0x00000000)

// TOP일 경우 Sorting, Projection Graph등을 생성
#define QMO_NONCRT_PATH_TOP_MASK        (0x00000001)
#define QMO_NONCRT_PATH_TOP_FALSE       (0x00000000)
#define QMO_NONCRT_PATH_TOP_TRUE        (0x00000001)

typedef struct qmoNonCrtPath
{
    UInt                flag;
    
    qmgGraph          * myGraph;    // non critical path까지 처리된 결과graph
    qmsQuerySet       * myQuerySet; // 자신의 qmsParseTree::querySet을 가리킴

    qmoCrtPath        * crtPath;    // critical path 정보
    
    qmoNonCrtPath     * left;
    qmoNonCrtPath     * right;
    
} qmoNonCrtPath;

//---------------------------------------------------
// Non Critical Path를 관리 함수
//---------------------------------------------------

class qmoNonCrtPathMgr 
{
public:

    // Non Critical Path 생성 및 초기화
    static IDE_RC    init( qcStatement    * aStatement,
                           qmsQuerySet    * aQuerySet,
                           idBool           aIsTop,
                           qmoNonCrtPath ** aNonCrtPath );
    
    // Non Critical Path에 대한 최적화 및 Graph 생성
    static IDE_RC    optimize( qcStatement    * aStatement,
                               qmoNonCrtPath  * aNonCrtPath);
    
private:
    // Leaf Non Critical Path의 최적화
    static IDE_RC    optimizeLeaf( qcStatement   * aStatement,
                                   qmoNonCrtPath * aNonCrtPath );
    

    // Non-Leaf Non Critical Path의 최적화
    static IDE_RC    optimizeNonLeaf( qcStatement   * aStatement,
                                      qmoNonCrtPath * aNonCrtPath );

    // PROJ-2582 recursive with
    static IDE_RC optimizeSet( qcStatement   * aStatement,
                               qmoNonCrtPath * aNonCrtPath );

    static IDE_RC  optimizeSetRecursive( qcStatement   * aStatement,
                                         qmoNonCrtPath * aNonCrtPath );
};


#endif /* _O_QMO_NON_CRT_PATH_MGR_H_ */

