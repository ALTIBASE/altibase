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
 * $Id: qmgSorting.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *     Sorting Graph를 위한 정의
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMG_SORTING_H_
#define _O_QMG_SORTING_H_ 1

#include <qc.h>
#include <qmgDef.h>

//---------------------------------------------------
// Sorting Graph의 Define 상수
//---------------------------------------------------

// qmgSORT.graph.flag
#define QMG_SORT_OPT_TIP_MASK                   (0x0F000000)
#define QMG_SORT_OPT_TIP_NONE                   (0x00000000)
#define QMG_SORT_OPT_TIP_INDEXABLE_ORDERBY      (0x01000000)
#define QMG_SORT_OPT_TIP_LMST                   (0x02000000)


//---------------------------------------------------
// Sorting Graph 를 관리하기 위한 자료 구조
//---------------------------------------------------

typedef struct qmgSORT
{
    qmgGraph         graph;      // 공통 Graph 정보

    qmsSortColumns * orderBy;    // orderBy 정보

    //-----------------------------------------------
    // Limit Sort 최적화가 선택된 경우, 다음과 같이 설정됨
    //   
    //   - limitCnt = qmsLimit::start + qmsLimit::count
    // 
    //   이때 limitCnt는 최대 Limit 개수 이하이어야 함
    //   limitCnt가 최대 Limit 개수를 초과하면,
    //   Limit Sort 최적화는 선택되지 않기 때문이다.
    //   - 최대 limitCnt : QMN_LMST_MAXIMUM_LIMIT_CNT 
    //-----------------------------------------------
    
    ULong            limitCnt;   // 저장할 Row의 수
    
} qmgSORT;

//---------------------------------------------------
// Sorting Graph 를 관리하기 위한 함수
//---------------------------------------------------

class qmgSorting
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

private:
    static IDE_RC makeChildPlan( qcStatement * aStatement,
                                 qmgSORT     * aMyGraph );

    static IDE_RC makeSort( qcStatement * aStatement,
                            qmgSORT     * aMyGraph );

    static IDE_RC makeLimitSort( qcStatement * aStatement,
                                 qmgSORT     * aMyGraph );

    /* PROJ-1071 Parallel Query */
    static IDE_RC makeSort4Parallel(qcStatement* aStatement,
                                    qmgSORT    * aMyGraph);

    static IDE_RC getCostByPrevOrder( qcStatement       * aStatement,
                                      qmgSORT           * aSortGraph,
                                      qmgPreservedOrder * aWantOrder,
                                      SDouble           * aAccessCost,
                                      SDouble           * aDiskCost,
                                      SDouble           * aTotalCost );

};

#endif /* _O_QMG_SORTING_H_ */

