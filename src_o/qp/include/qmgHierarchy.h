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
 * $Id: qmgHierarchy.h 82330 2018-02-23 00:32:59Z donovan.seo $
 *
 * Description :
 *     Hierarchy Graph를 위한 정의
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMG_HIERARCHY_H_
#define _O_QMG_HIERARCHY_H_ 1

#include <qc.h>
#include <qmgDef.h>
#include <qmsParseTree.h>
#include <qmoCnfMgr.h>

//---------------------------------------------------
// Hierarchy Graph 를 관리하기 위한 자료 구조
//---------------------------------------------------

typedef struct qmgHIER
{
    qmgGraph graph;  // 공통 Graph 정보
    
    // Hierarchy Graph를 위한 정보
    qmsHierarchy     * myHierarchy;    // qmsSFWGH::hierarchy를 가리킴

    // startWith 정보
    qmoCNF           * startWithCNF;
    UInt               mStartWithMethodCnt;
    qmoAccessMethod  * mStartWithAccessMethod;
    qmoAccessMethod  * mSelectedStartWithMethod;
    qcmIndex         * mSelectedStartWithIdx;

    // connectBy 정보
    qmoCNF           * connectByCNF;
    UInt               mConnectByMethodCnt;
    qmoAccessMethod  * mConnectByAccessMethod;
    qmoAccessMethod  * mSelectedConnectByMethod;
    qcmIndex         * mSelectedConnectByIdx;
} qmgHIER;

//---------------------------------------------------
// Hierarchy Graph 를 관리하기 위한 함수
//---------------------------------------------------

class qmgHierarchy
{
public:
    // Graph 의 초기화
    static IDE_RC  init( qcStatement  * aStatement,
                         qmsQuerySet  * aQuerySet,
                         qmgGraph     * aChildGraph,
                         qmsFrom      * aFrom,
                         qmsHierarchy * aHierarchy,
                         qmgGraph    ** aGraph );

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
    static IDE_RC optimizeStartWith( qcStatement * aStatement,
                                     qmgHIER     * aMyGraph,
                                     qmsTableRef * aTableRef,
                                     UInt          aIndexCnt );

    static IDE_RC searchTableID( qtcNode * aNode,
                                 UShort  * aTableID,
                                 idBool  * aFind );

    static IDE_RC searchFrom( UShort     aTableID,
                              qmsFrom  * aFrom,
                              qmsFrom ** aFindFrom,
                              idBool   * aFind );
};

#endif /* _O_QMG_HIERARCHY_H_ */

