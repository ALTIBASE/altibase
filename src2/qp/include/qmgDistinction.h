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
 * $Id: qmgDistinction.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *     Distinction Graph를 위한 정의
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMG_DISTINCTION_H_
#define _O_QMG_DISTINCTION_H_ 1

#include <qc.h>
#include <qmgDef.h>

//---------------------------------------------------
// Distinction Graph의 Define 상수
//---------------------------------------------------

// qmgDIST.graph.flag
#define QMG_DIST_OPT_TIP_MASK                   (0x0F000000)
#define QMG_DIST_OPT_TIP_NONE                   (0x00000000)
#define QMG_DIST_OPT_TIP_INDEXABLE_DISINCT      (0x01000000)


//---------------------------------------------------
// Distinction Graph 를 관리하기 위한 자료 구조
//---------------------------------------------------

typedef struct qmgDIST
{
    qmgGraph    graph;         // 공통 Graph 정보
    
    // hash based Distinction으로 결정된 경우에 설정
    UInt        hashBucketCnt; 
    
} qmgDIST;

//---------------------------------------------------
// Distinction Graph 를 관리하기 위한 함수
//---------------------------------------------------

class qmgDistinction
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
    static IDE_RC  makePlan( qcStatement * aStatement, const qmgGraph * aParent, qmgGraph * aGraph );

    // Graph의 공통 정보를 출력함.
    static IDE_RC  printGraph( qcStatement  * aStatement,
                               qmgGraph     * aGraph,
                               ULong          aDepth,
                               iduVarString * aString );

    static IDE_RC finalizePreservedOrder( qmgGraph * aGraph );

private:
    static IDE_RC  makeSortDistinction( qcStatement * aStatement,
                                        qmgDIST     * aMyGraph );

    static IDE_RC  makeHashDistinction( qcStatement * aStatement,
                                        qmgDIST     * aMyGraph );

    // DISTINCT Target  컬럼 정보를 이용하여Order 자료 구조를 구축한다.
    static IDE_RC makeTargetOrder( qcStatement        * aStatement,
                                   qmsTarget          * aDistTarget,
                                   qmgPreservedOrder ** aNewTargetOrder );
                                  
    // indexable group by 최적화
    static IDE_RC indexableDistinct( qcStatement      * aStatement,
                                     qmgGraph         * aGraph,
                                     qmsTarget        * aDistTarget,
                                     idBool           * aIndexableDistinct );

    // Preserved Order 방식을 사용하였을 경우의 비용 계산
    static IDE_RC getCostByPrevOrder( qcStatement      * aStatement,
                                      qmgDIST          * aDistGraph,
                                      SDouble          * aAccessCost,
                                      SDouble          * aDiskCost,
                                      SDouble          * aTotalCost );
};

#endif /* _O_QMG_DISTINCTION_H_ */

