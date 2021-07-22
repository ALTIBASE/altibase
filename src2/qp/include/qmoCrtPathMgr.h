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
 * $Id: qmoCrtPathMgr.h 86122 2019-09-04 07:20:21Z donovan.seo $
 *
 * Description :
 *     Critical Path Manager
 *
 *     FROM, WHERE로 구성되는 Critical Path에 대한 최적화를 수행하고
 *     이에 대한 Graph를 생성한다.
 *
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMO_CRT_PATH_MGR_H_
#define _O_QMO_CRT_PATH_MGR_H_ 1

#include <qc.h>
#include <qmgDef.h>
#include <qmoCnfMgr.h>
#include <qmoDnfMgr.h>

//---------------------------------------------------
// Critical Path를 관리하기 위한 자료 구조
//---------------------------------------------------

typedef struct qmoCrtPath
{
    qmoNormalType   normalType;    // CNF 또는 DNF

    qmoDNF        * crtDNF;
    qmoCNF        * crtCNF;

    qmoCNF        * currentCNF;    // 현재 CNF( Plan Tree 생성 시 사용 )

    qmgGraph      * myGraph;       // critical path가 결정된 결과 graph

    //-------------------------------------------------------------------
    // PROJ-1405 ROWNUM 관련 자료 구조
    //
    // currentNormalType   : 현재 critical path가 predicate을 처리하는
    //                       Normalization Type 정보
    // rownumPredicate4CNF : critical path가 CNF,NNF로 predicate을 처리할때의
    //                       rownumPredicate 정보
    // rownumPredicate4CNF : critical path가 DNF로 predicate을 처리할때의
    //                       rownumPredicate 정보
    // rownumPredicate     : critical path가 결정한 정규화형태로
    //                       rownumPredicate 연결
    // myQuerySet          : ROWNUM 노드 정보
    //-------------------------------------------------------------------

    qmoNormalType   currentNormalType;
    
    qmoPredicate  * rownumPredicateForCNF;
    qmoPredicate  * rownumPredicateForDNF;
    qmoPredicate  * rownumPredicate;
    
    qmsQuerySet   * myQuerySet;
    idBool          mIsOnlyNL;
} qmoCrtPath;

//---------------------------------------------------
// Critical Path를 관리 함수
//---------------------------------------------------

class qmoCrtPathMgr
{
public:

    // Critical Path 생성 및 초기화
    static IDE_RC    init( qcStatement * aStatement,
                           qmsQuerySet * aQuerySet,
                           qmoCrtPath ** aCrtPath );

    // Critical Path에 대한 최적화 및 Graph 생성
    static IDE_RC    optimize( qcStatement * aStatement,
                               qmoCrtPath  * aCrtPath );

    // Normalization Type 결정
    static IDE_RC    decideNormalType( qcStatement   * aStatement,
                                       qmsFrom       * aFrom,
                                       qtcNode       * aWhere,
                                       qmsHints      * aHint,
                                       idBool          aCNFOnly,
                                       qmoNormalType * aNormalType);

    // PROJ-1405
    // Rownum Predicate을 rownumPredicate에 연결
    static IDE_RC    addRownumPredicate( qmsQuerySet  * aQuerySet,
                                         qmoPredicate * aPredicate );
    
    // PROJ-1405
    // Rownum Predicate을 rownumPredicate에 연결
    static IDE_RC    addRownumPredicateForNode( qcStatement  * aStatement,
                                                qmsQuerySet  * aQuerySet,
                                                qtcNode      * aNode,
                                                idBool         aNeedCopy );

    // BUG-35155 Partial CNF
    static IDE_RC decideNormalType4Where( qcStatement   * aStatement,
                                           qmsFrom       * aFrom,
                                           qtcNode       * aWhere,
                                           qmsHints      * aHint,
                                           idBool          aIsCNFOnly,
                                           qmoNormalType * aNormalType);

private:

    // from절에 view가 있는지 검사
    static IDE_RC    existsViewinFrom( qmsFrom * aFrom,
                                       idBool  * aIsExistView );
};


#endif /* _O_QMO_CRT_PATH_MGR_H_ */
