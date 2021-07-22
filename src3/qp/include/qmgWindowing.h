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
 * $Id: qmgWindowing.h 29304 2008-11-14 08:17:42Z jakim $
 *
 * Description :
 *     Windowing Graph를 위한 정의
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMG_WINDOWING_H_
#define _O_QMG_WINDOWING_H_ 1

#include <qc.h>
#include <qmgDef.h>

//---------------------------------------------------
// Windowing Graph의 Define 상수
//---------------------------------------------------

//---------------------------------------------------
// Windowing Graph 를 관리하기 위한 자료 구조
//---------------------------------------------------

typedef struct qmgWINDOW
{
    qmgGraph              graph;      // 공통 Graph 정보

    qmoDistAggArg       * distAggArg;    
    qmsAnalyticFunc     * analyticFuncList;
    qmsAnalyticFunc    ** analyticFuncListPtrArr;
    UInt                  sortingKeyCount;
    qmgPreservedOrder  ** sortingKeyPtrArr;

} qmgWINDOW;

//---------------------------------------------------
// Windowing Graph 를 관리하기 위한 함수
//---------------------------------------------------

class qmgWindowing
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

    // sorting key들 중에서 want order와 동일한 key 가 존재하는지 검사 
    static
    IDE_RC existWantOrderInSortingKey( qmgGraph          * aGraph,
                                       qmgPreservedOrder * aWantOrder,
                                       idBool            * aFind,
                                       UInt              * aSameKeyPosition );

    // want order로 sorting key 변경 
    static IDE_RC alterSortingOrder( qcStatement       * aStatement,
                                     qmgGraph          * aGraph,
                                     qmgPreservedOrder * aWantOrder,
                                     idBool              aAlterFirstOrder );

    // plan시 변경된 sorting key 정보 재설정 
    static IDE_RC resetSortingKey( qmgPreservedOrder ** aSortingKeyPtrArr,
                                   UInt                 aSortingKeyCount,
                                   qmsAnalyticFunc   ** aAnalyticFuncListPtrArr);

    static IDE_RC finalizePreservedOrder( qmgGraph * aGraph );

private:
    // sorting key와 analytic function 분류 
    static IDE_RC classifySortingKeyNAnalFunc( qcStatement     * aStatement,
                                               qmgWINDOW       * aMyGraph );

    // partition by, order by와 동일한 sorting key, direction이 존재하는지 검사 
    static IDE_RC existSameSortingKeyAndDirection(
        UInt                 aSortingKeyCount,
        qmgPreservedOrder ** aSortingKeyPtrArr,
        qtcOverColumn      * aOverColumn,
        idBool             * aFoundSameSortingKey,
        idBool             * aReplaceSortingKey,
        UInt               * aSortingKeyPosition );
    
    // BUG-33663 Ranking Function
    // sorting key array에서 제거가능한 sorting key를 제거
    static IDE_RC compactSortingKeyArr( UInt                 aFlag,
                                        qmgPreservedOrder ** aSortingKeyPtrArr,
                                        UInt               * aSortingKeyCount,
                                        qmsAnalyticFunc   ** aAnalyticFuncListPtrArr);
    
    static idBool isSameSortingKey( qmgPreservedOrder * aSortingKey1,
                                    qmgPreservedOrder * aSortingKey2 );
    
    static void mergeSortingKey( qmgPreservedOrder * aSortingKey1,
                                 qmgPreservedOrder * aSortingKey2 );

    static IDE_RC getBucketCnt4Windowing( qcStatement  * aStatement,
                                          qmgWINDOW    * aMyGraph,
                                          qtcOverColumn* aGroupBy,
                                          UInt           aHintBucketCnt,
                                          UInt         * aBucketCnt );
};

#endif /* _O_QMG_WINDOWING_H_ */

