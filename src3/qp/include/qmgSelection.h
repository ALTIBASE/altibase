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
 * $Id: qmgSelection.h 90192 2021-03-12 02:01:03Z jayce.park $
 *
 * Description :
 *     Selection Graph를 위한 정의
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMG_SELECTION_H_
#define _O_QMG_SELECTION_H_ 1

#include <qc.h>
#include <qmgDef.h>


//---------------------------------------------------
// Selection Graph의 Define 상수
//---------------------------------------------------

// PROJ-1446 Host variable을 포함한 질의 최적화
// local definition
// optimize() 함수와 getBestAccessMethod() 함수와의 의사소통에 사용됨.
#define QMG_NOT_USED_SCAN_HINT        (0)
#define QMG_USED_SCAN_HINT            (1)
#define QMG_USED_ONLY_FULL_SCAN_HINT  (2)

// To Fix PR-11937
// qmgSELT.graph.flag
// FULL SCAN 힌트가 적용되었는지의 여부
//    FULL SCAN 힌트가 적용된 경우
//    해당 Table이 right가 되어 Index Nested Loop Join을 사용할 수 없다.
#define QMG_SELT_FULL_SCAN_HINT_MASK            (0x20000000)
#define QMG_SELT_FULL_SCAN_HINT_FALSE           (0x00000000)
#define QMG_SELT_FULL_SCAN_HINT_TRUE            (0x20000000)

// qmgSELT.graph.flag
// Not Null Key Range Flag가 사용되는 경우
// (1) indexable Min Max가 적용된 Selection Graph인 경우
#define QMG_SELT_NOTNULL_KEYRANGE_MASK          (0x40000000)
#define QMG_SELT_NOTNULL_KEYRANGE_FALSE         (0x00000000)
#define QMG_SELT_NOTNULL_KEYRANGE_TRUE          (0x40000000)

// PROJ-1502 PARTITIONED DISK TABLE
// qmgPartition 그래프에서는 다음 위의 flag를 공유한다.
// QMG_SELT_FULL_SCAN_HINT_MASK
// QMG_SELT_NOTNULL_KEYRANGE_MASK

// PROJ-1502 PARTITIONED DISK TABLE
// partition에 대한 selection graph인 경우
#define QMG_SELT_PARTITION_MASK                 (0x80000000)
#define QMG_SELT_PARTITION_FALSE                (0x00000000)
#define QMG_SELT_PARTITION_TRUE                 (0x80000000)

#define QMG_SELT_FLAG_CLEAR                     (0x00000000)

/*
 * PROJ-2402 Parallel Table Scan
 * qmgSELT.mFlag
 */
#define QMG_SELT_PARALLEL_SCAN_MASK             (0x00000001)
#define QMG_SELT_PARALLEL_SCAN_TRUE             (0x00000001)
#define QMG_SELT_PARALLEL_SCAN_FALSE            (0x00000000)

/*
 * PROJ-2641 Hierarchy Query Index
 */
#define QMG_BEST_ACCESS_METHOD_HIERARCHY_MASK   (0x00000001)
#define QMG_BEST_ACCESS_METHOD_HIERARCHY_FALSE  (0x00000000)
#define QMG_BEST_ACCESS_METHOD_HIERARCHY_TRUE   (0x00000001)

#define QMG_HIERARCHY_QUERY_DISK_IO_ADJUST_VALUE (10)

//---------------------------------------------------
// Selection Graph 를 관리하기 위한 자료 구조
//---------------------------------------------------

typedef struct qmgSELT
{
    qmgGraph          graph;    // 공통 Graph 정보

    qmsLimit        * limit;    // SCAN Limit 최적화 적용시, limit 정보 설정

    //------------------------------------------------
    // Access Method를 위한 정보
    //     - selectedIndex : 선택된 AccessMethod가 FULL SCAN이 아닌 경우,
    //                       선택된 AccessMethod Index
    //     - accessMethodCnt : 해당 Table의 index 개수 + 1
    //     - accessMethod : 각 accessMethod 정보와 Cost 정보
    //------------------------------------------------
    qcmIndex        * selectedIndex;
    qmoAccessMethod * selectedMethod; // 선택된 Access Method
    UInt              accessMethodCnt;
    qmoAccessMethod * accessMethod;

    qmoScanDecisionFactor * sdf;

    // PROJ-1502 PARTITIONED DISK TABLE
    // partition에 대한 selection graph인 경우
    qmsPartitionRef * partitionRef;

    /* BUG-44659 미사용 Partition의 통계 정보를 출력하다가,
     *           Graph의 Partition/Column/Index Name 부분에서 비정상 종료할 수 있습니다.
     *  Lock을 잡지 않고 Meta Cache를 사용하면, 비정상 종료할 수 있습니다.
     *  qmgSELT에서 Partition Name을 보관하도록 수정합니다.
     */
    SChar             partitionName[QC_MAX_OBJECT_NAME_LEN + 1];

    idBool            forceIndexScan;
    idBool            forceRidScan;    // index table scan인 경우

    UInt              mFlag;
} qmgSELT;

//---------------------------------------------------
// Selection Graph 를 관리하기 위한 함수
//---------------------------------------------------

class qmgSelection
{
public:
    // Graph 의 초기화
    static IDE_RC  init( qcStatement * aStatement,
                         qmsQuerySet * aQuerySet,
                         qmsFrom     * aFrom,
                         qmgGraph   ** aGraph );

    static IDE_RC  init( qcStatement     * aStatement,
                         qmsQuerySet     * aQuerySet,
                         qmsFrom         * aFrom,
                         qmsPartitionRef * aPartitionRef,
                         qmgGraph       ** aGraph );

    // Graph의 최적화 수행
    static IDE_RC optimize( qcStatement * aStatement,
                            qmgGraph * aGraph );

    // Graph의 Plan Tree 생성
    static IDE_RC makePlan( qcStatement * aStatement,
                            const qmgGraph * aParent, 
                            qmgGraph * aGraph );

    // Graph의 공통 정보를 출력함.
    static IDE_RC printGraph( qcStatement  * aStatement,
                              qmgGraph     * aGraph,
                              ULong          aDepth,
                              iduVarString * aString );

    static IDE_RC printAccessMethod( qcStatement     * aStatement,
                                     qmoAccessMethod * aAccessMethod,
                                     ULong             aDepth,
                                     iduVarString    * aString );

    // PROJ-1502 PARTITIONED DISK TABLE - BEGIN -
    static IDE_RC optimizePartition(
        qcStatement * aStatement,
        qmgGraph    * aGraph );

    static IDE_RC makePlanPartition(
        qcStatement    * aStatement,
        const qmgGraph * aParent, 
        qmgGraph       * aGraph );

    static IDE_RC printGraphPartition(
        qcStatement  * aStatement,
        qmgGraph     * aGraph,
        ULong          aDepth,
        iduVarString * aString );
    // PROJ-1502 PARTITIONED DISK TABLE - END -

    // Preserved Order를 만드는 함수
    static IDE_RC makePreservedOrder( qcStatement        * aStatement,
                                      qmoIdxCardInfo     * aIdxCardInfo,
                                      UShort               aTable,
                                      qmgPreservedOrder ** aPreservedOrder );

    // 가장 좋은 accessMethod를 찾아주는 함수
    static IDE_RC getBestAccessMethod(qcStatement     * aStatement,
                                      qmgGraph        * aGraph,
                                      qmoStatistics   * aStatInfo,
                                      qmoPredicate    * aPredicate,
                                      qmoAccessMethod * aAccessMethod,
                                      qmoAccessMethod ** aSelectedAccessMethod,
                                      UInt             * aAccessMethodCnt,
                                      qcmIndex        ** aSelectedIndex,
                                      UInt             * aSelectedScanHint,
                                      UInt               aParallelDegree,
                                      UInt               aFlag );

    // PROJ-1446 Host variable을 포함한 질의 최적화
    static IDE_RC getBestAccessMethodInExecutionTime(
        qcStatement     * aStatement,
        qmgGraph        * aGraph,
        qmoStatistics   * aStatInfo,
        qmoPredicate    * aPredicate,
        qmoAccessMethod * aAccessMethod,
        qmoAccessMethod ** aSelectedAccessMethod );

    // PROJ-1446 Host variable을 포함한 질의 최적화
    // 상위 graph에 의해 access method가 바뀐 경우
    // selection graph의 sdf를 disable 시킨다.
    static IDE_RC alterSelectedIndex( qcStatement * aStatement,
                                      qmgSELT     * aGraph,
                                      qcmIndex    * aNewIndex );

    // PROJ-1446 Host variable을 포함한 질의 최적화
    // 상위 JOIN graph에서 ANTI로 처리할 때
    // 하위 SELT graph를 복사하는데 이때 이 함수를
    // 통해서 복사하도록 해야 안전하다.
    static IDE_RC copySELTAndAlterSelectedIndex(
        qcStatement * aStatement,
        qmgSELT     * aSource,
        qmgSELT    ** aTarget,
        qcmIndex    * aNewSelectedIndex,
        UInt          aWhichOne );

    // PROJ-1502 PARTITIONED DISK TABLE
    // push-down join predicate를 받아서 자신의 그래프에 연결.
    static IDE_RC setJoinPushDownPredicate( qmgSELT       * aGraph,
                                            qmoPredicate ** aPredicate );

    // PROJ-1502 PARTITIONED DISK TABLE
    // push-down non-join predicate를 받아서 자신의 그래프에 연결.
    static IDE_RC setNonJoinPushDownPredicate( qmgSELT       * aGraph,
                                               qmoPredicate ** aPredicate );

    static IDE_RC finalizePreservedOrder( qmgGraph * aGraph );

    // View Graph의 생성하여 aGraph의 left에 연결
    static IDE_RC makeViewGraph( qcStatement * aStatement, qmgGraph * aGraph );

    // PROJ-1624 global non-partitioned index
    // index table scan용 graph로 변경한다.
    static IDE_RC alterForceRidScan( qcStatement * aStatement,
                                     qmgGraph    * aGraph );

    // PROJ-2242
    static void setFullScanMethod( qcStatement      * aStatement,
                                   qmoStatistics    * aStatInfo,
                                   qmoPredicate     * aPredicate,
                                   qmoAccessMethod  * aAccessMethod,
                                   UInt               aParallelDegree,
                                   idBool             aInExecutionTime );

    /* TASK-7219 Non-shard DML */
    static void isForcePushedPredForShardView( mtcNode * aNode,
                                               idBool  * aIsFound );
private:
    // SCAN을 생성
    static IDE_RC makeTableScan( qcStatement * aStatement,
                                 qmgSELT     * aMyGraph );

    // VSCN을 생성
    static IDE_RC makeViewScan( qcStatement * aStatement,
                                qmgSELT     * aMyGraph );

    // PROJ-2528 recursive with
    // recursive VSCN
    static IDE_RC makeRecursiveViewScan( qcStatement * aStatement,
                                         qmgSELT     * aMyGraph );
    
    /* PROJ-2402 Parallel Table Scan */
    static IDE_RC makeParallelScan(qcStatement* aStatement, qmgSELT* aMyGraph);
    static void setParallelScanFlag(qcStatement* aStatement, qmgGraph* aGraph);

    // VIEW index hint를 view 하위 base table에 적용
    static IDE_RC setViewIndexHints( qcStatement         * aStatement,
                                     qmsTableRef         * aTableRef );

    // VIEW index hint를 적용하기 위해 set이 아닌 querySet을 찾아 hint 적용
    static IDE_RC findQuerySet4ViewIndexHints( qcStatement   * aStatement,
                                               qmsQuerySet       * aQuerySet,
                                               qmsTableAccessHints * aAccessHint );

    // VIEW index hint에 해당하는 base table을 찾아서 hint 적용
    static IDE_RC findBaseTableNSetIndexHint( qcStatement * aStatement,
                                              qmsFrom     * aFrom,
                                              qmsTableAccessHints * aAccessHint );

    // VIEW index hint를 해당 base table에 적용
    static IDE_RC setViewIndexHintInBaseTable( qcStatement * aStatement,
                                               qmsFrom     * aFrom,
                                               qmsTableAccessHints * aAccessHint );

    // PROJ-1473 view에 대한 push projection 수행
    static IDE_RC doViewPushProjection( qcStatement  * aViewStatement,
                                        qmsTableRef  * aViewTableRef,
                                        qmsQuerySet  * aQuerySet );

    // PROJ-1473 view에 대한 push projection 수행
    static IDE_RC doPushProjection( qcStatement  * aViewStatement,
                                    qmsTableRef  * aViewTableRef,
                                    qmsQuerySet  * aQuerySet );    

    // fix BUG-20939
    static void setViewPushProjMask( qcStatement * aStatement,
                                     qtcNode     * aNode );

    // BUG-18367 view에 대해 push selection 수행
    static IDE_RC doViewPushSelection( qcStatement  * aStatement,
                                       qmsTableRef  * aTableRef,
                                       qmoPredicate * aPredicate,
                                       qmgGraph     * aGraph,
                                       idBool       * aIsPushed );

    // PROJ-2242
    static IDE_RC setIndexScanMethod( qcStatement      * aStatement,
                                      qmgGraph         * aGraph,
                                      qmoStatistics    * aStatInfo,
                                      qmoIdxCardInfo   * aIdxCardInfo,
                                      qmoPredicate     * aPredicate,
                                      qmoAccessMethod  * aAccessMethod,
                                      idBool             aIsMemory,
                                      idBool             aInExecutionTime );

    // view의 preserved order를 복사하고, Table ID를 변경하는 함수
    static IDE_RC copyPreservedOrderFromView( qcStatement        * aStatement,
                                              qmgGraph           * aChildGraph,
                                              UShort               aTableId,
                                              qmgPreservedOrder ** aNewOrder );

    // PROJ-1446 Host variable을 포함한 질의 최적화
    // host variable에 대한 최적화를 위한 준비한다.
    static IDE_RC prepareScanDecisionFactor( qcStatement * aStatement,
                                             qmgSELT     * aGraph );

    // PROJ-1446 Host variable을 포함한 질의 최적화
    // predicate와 SDF에 데이터 영역 offset을 지정한다.
    static IDE_RC setSelectivityOffset( qcStatement  * aStatement,
                                        qmoPredicate * aPredicate );

    static IDE_RC setMySelectivityOffset( qcStatement  * aStatement,
                                          qmoPredicate * aPredicate );

    static IDE_RC setTotalSelectivityOffset( qcStatement  * aStatement,
                                             qmoPredicate * aPredicate );

    // PROJ-1446 Host variable을 포함한 질의 최적화
    // getBestAccessMethod()에서 분리된 함수
    // 주어진 access method들 중에서 최상의 method를 선택한다.
    static IDE_RC selectBestMethod( qcStatement      * aStatement,
                                    qcmTableInfo     * aTableInfo,
                                    qmoAccessMethod  * aAccessMethod,
                                    UInt               aAccessMethodCnt,
                                    qmoAccessMethod ** aSelected );

    static IDE_RC mergeViewTargetFlag( qcStatement * aDescView,
                                       qcStatement * aSrcView );
};

#endif /* _O_QMG_SELECTION_H_ */
