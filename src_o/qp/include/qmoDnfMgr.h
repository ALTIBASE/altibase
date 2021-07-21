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
 * $Id: qmoDnfMgr.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *     DNF Critical Path Manager
 *
 *     DNF Normalized Form에 대한 최적화를 수행하고
 *     해당 Graph를 생성한다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMO_DNF_MGR_H_
#define _O_QMO_DNF_MGR_H_ 1

#include <qc.h>
#include <qmgDef.h>
#include <qmoCnfMgr.h>
#include <qmgDnf.h>

//---------------------------------------------------
// DNF Critical Path를 관리하기 위한 자료 구조
//---------------------------------------------------

typedef struct qmoDNF
{   
    qtcNode       * normalDNF;    // where절을 DNF로 normalize한 결과
    qmgGraph      * myGraph;      // DNF Form 결과 graph의 top
    qmsQuerySet   * myQuerySet;
    SDouble         cost;        // DNF Total Cost

    //------------------------------------------------------
    // CNF Graph 관련 자료 구조
    //
    //   - cnfCnt : CNF의 개수 ( = normalDNF의 AND의 개수 )
    //   - myCNF  : CNF 배열
    //------------------------------------------------------
    
    UInt            cnfCnt;      // CNF 의 개수

    // PROJ-1446 Host variable을 포함한 질의 최적화
    UInt            madeCnfCnt;  // optmization때 생성된 cnf의 개수
                                 // removeOptimizationInfo에서 사용됨

    qmoCNF        * myCNF;       // CNF들의 배열

    //------------------------------------------------------
    // DNF Graph 관련 자료 구조
    //
    //   - dnfGraphCnt : 생성해야할 DNF Graph 개수 ( = CnfCount - 1 )
    //   - dnfGraph    : Dnf Graph 배열
    //------------------------------------------------------
    UInt            dnfGraphCnt;
    qmgDNF        * dnfGraph;

    //------------------------------------------------
    // Not Normal Form : (~(이전 DNF의 Predicate))들의 List
    //                   중복 data가 없도록 하기 위함
    //    - notNormalForm의 개수 : dnfGraphCnt
    //------------------------------------------------
    
    qtcNode      ** notNormal;
    
} qmoDNF;

//---------------------------------------------------
// DNF Critical Path를 관리하기 위한 함수
//---------------------------------------------------

class qmoDnfMgr 
{
public:

    // DNF Critical Path 생성 및 초기화
    static IDE_RC    init( qcStatement * aStatement,
                           qmoDNF      * aDNF,
                           qmsQuerySet * aQuerySet,
                           qtcNode     * aNormalDNF);
    
    // DNF Critical Path에 대한 최적화 및 Graph 생성
    static IDE_RC    optimize( qcStatement * aStatement,
                               qmoDNF      * aDNF,
                               SDouble       aCnfCost );
   
    // PROJ-1446 Host variable을 포함한 질의 최적화
    // optimization때 만든 정보를 지울 필요가 있을 때
    // 이 함수에 추가하면 된다. 
    static IDE_RC    removeOptimizationInfo( qcStatement * aStatement,
                                             qmoDNF      * aDNF );

private:
    // Dnf Not Normal Form들의 배열을 만드는 함수
    static IDE_RC    makeNotNormal( qcStatement * aStatement,
                                    qmoDNF      * aDNF );
    
    // Dnf Not Normal Form 만드는 함수
    static IDE_RC    makeDnfNotNormal( qcStatement * aStatement,
                                       qtcNode     * aNormalForm,
                                       qtcNode    ** aDnfNotNormal );

    // PROJ-1405 DNF normal form에서 rownum predicate을 제거한다.
    static IDE_RC    removeRownumPredicate( qcStatement * aStatement,
                                            qmoDNF      * aDNF );
};


#endif /* _O_QMO_DNF_MGR_H_ */

