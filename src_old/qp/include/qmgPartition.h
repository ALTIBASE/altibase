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
 * $Id: qmgPartition.h 20233 2007-02-01 01:58:21Z shsuh $
 *
 * Description :
 *     Partition Graph를 위한 정의
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMG_PARTITION_H_
#define _O_QMG_PARTITION_H_ 1

#include <qc.h>
#include <qmgDef.h>

//---------------------------------------------------
// Partition Graph의 Define 상수
//---------------------------------------------------

// qmgSELT.graph.flag를 공유하여 쓴다.
// QMG_SELT_FULL_SCAN_HINT_MASK
// QMG_SELT_NOTNULL_KEYRANGE_MASK

//---------------------------------------------------
// Partition Graph 를 관리하기 위한 자료 구조
//---------------------------------------------------

typedef struct qmgPARTITION
{
    qmgGraph          graph;    // 공통 Graph 정보

    qmsLimit        * limit;    // SCAN Limit 최적화 적용시, limit 정보 설정

    //----------------------------------------------
    // Partition Filter Predicate 정보:
    //     - partKeyRange        : partition key range 출력용
    //     - partFilterPredicate : partition filter 추출용
    //----------------------------------------------

    qtcNode         * partKeyRange;
    qmoPredicate    * partFilterPredicate;

    //------------------------------------------------
    // Access Method를 위한 정보
    //     - selectedIndex : 선택된 AccessMethod가 FULL SCAN이 아닌 경우,
    //                       선택된 AccessMethod Index
    //     - accessMethodCnt : 해당 Table의 index 개수 + 1
    //     - accessMethod : 각 accessMethod 정보와 Cost 정보
    //------------------------------------------------
    qcmIndex        * selectedIndex;
    qmoAccessMethod * selectedMethod;  // 선택된 Access Method
    UInt              accessMethodCnt; // join을 위한 accessMethodCnt
    qmoAccessMethod * accessMethod;    // join을 위한 accessMethod

    idBool            forceIndexScan;

    qmsPartitionRef * mPrePruningPartRef; // BUG-48800 prepruning tableRef
} qmgPARTITION;

//---------------------------------------------------
// Partition Graph 를 관리하기 위한 함수
//---------------------------------------------------

class qmgPartition
{
public:
    // Graph 의 초기화
    static IDE_RC  init( qcStatement * aStatement,
                         qmsQuerySet * aQuerySet,
                         qmsFrom     * aFrom,
                         qmgGraph   ** aGraph);

    // Graph의 최적화 수행
    static IDE_RC  optimize( qcStatement * aStatement, qmgGraph * aGraph );

    // Graph의 Plan Tree 생성
    static IDE_RC  makePlan( qcStatement * aStatement, const qmgGraph * aParent, qmgGraph * aGraph );

    // Graph의 공통 정보를 출력함.
    static IDE_RC  printGraph( qcStatement  * aStatement,
                               qmgGraph     * aGraph,
                               ULong          aDepth,
                               iduVarString * aString );

    // 상위 graph에 의해 access method가 바뀐 경우
    static IDE_RC alterSelectedIndex( qcStatement  * aStatement,
                                      qmgPARTITION * aGraph,
                                      qcmIndex     * aNewIndex );

    // PROJ-1446 Host variable을 포함한 질의 최적화
    // 상위 JOIN graph에서 ANTI로 처리할 때
    // 하위 SELT graph를 복사하는데 이때 이 함수를
    // 통해서 복사하도록 해야 안전하다.
    static IDE_RC copyPARTITIONAndAlterSelectedIndex( qcStatement   * aStatement,
                                                      qmgPARTITION  * aSource,
                                                      qmgPARTITION ** aTarget,
                                                      qcmIndex      * aNewSelectedIndex,
                                                      UInt            aWhichOne );

    // push-down join predicate를 받아서 자신의 그래프에 연결.
    static IDE_RC setJoinPushDownPredicate( qcStatement   * aStatement,
                                            qmgPARTITION  * aGraph,
                                            qmoPredicate ** aPredicate );

    // push-down non-join predicate를 받아서 자신의 그래프에 연결.
    static IDE_RC setNonJoinPushDownPredicate( qcStatement   * aStatement,
                                               qmgPARTITION  * aGraph,
                                               qmoPredicate ** aPredicate );

    // Preserved Order의 direction을 결정한다.
    static IDE_RC finalizePreservedOrder( qmgGraph * aGraph );
    
private:

    // partition keyrange를 추출
    static IDE_RC extractPartKeyRange(
        qcStatement          * aStatement,
        qmsQuerySet          * aQuerySet,
        qmoPredicate        ** aPartKeyPred,
        UInt                   aPartKeyCount,
        mtcColumn            * aPartKeyColumns,
        UInt                 * aPartKeyColsFlag,
        qtcNode             ** aPartKeyRangeOrg,
        smiRange            ** aPartKeyRange );

    // PROJ-2242 자식 파티션의 AccessMethodsCost 를 이용하여 cost 를 수정함
    static IDE_RC reviseAccessMethodsCost( qmgPARTITION   * aPartitionGraph,
                                           UInt             aPartitionCount );

    // children을 강제로 rid scan으로 변경
    static IDE_RC alterForceRidScan( qcStatement  * aStatement,
                                     qmgPARTITION * aGraph );
};

#endif /* _O_QMG_PARTITION_H_ */

