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
 * $Id: qmoStatMgr.h 90850 2021-05-17 05:09:54Z donovan.seo $
 *
 * Description :
 *     Statistical Information Manager
 *
 *     다음과 같은 각종 실시간 통계 정보의 추출을 담당한다.
 *          - Table의 Record 개수
 *          - Table의 Disk Page 개수
 *          - Index의 Cardinality
 *          - Column의 Cardinality
 *          - Column의 MIN Value
 *          - Column의 MAX Value
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMO_STAT_MGR_H_
#define _O_QMO_STAT_MGR_H_ 1

#include <mtdTypes.h>
#include <qc.h>
#include <qmsParseTree.h>
#include <qmoDef.h>
#include <qmoHint.h>
#include <smiDef.h>

//-------------------------------------------------------
// default 통계정보
//-------------------------------------------------------
// TASK-6699 TPC-H 성능 개선
# define QMO_STAT_SYSTEM_SREAD_TIME                  (87.70000000)
# define QMO_STAT_SYSTEM_MREAD_TIME                  (17.25000000)
# define QMO_STAT_SYSTEM_MTCALL_TIME                 (0.01130000)
# define QMO_STAT_SYSTEM_HASH_TIME                   (0.01500000)
# define QMO_STAT_SYSTEM_COMPARE_TIME                (0.01130000)
# define QMO_STAT_SYSTEM_STORE_TIME                  (0.05000000)

// BUG-40913 v$table 조인 질의가 성능이 느림
# define QMO_STAT_PFVIEW_RECORD_COUNT                (1024)

# define QMO_STAT_TABLE_RECORD_COUNT                 (10240)
# define QMO_STAT_TABLE_DISK_PAGE_COUNT              (10)
# define QMO_STAT_TABLE_AVG_ROW_LEN                  (100)
# define QMO_STAT_TABLE_MEM_ROW_READ_TIME            (1)
# define QMO_STAT_TABLE_DISK_ROW_READ_TIME           (10)

# define QMO_STAT_INDEX_KEY_NDV                      (100)
# define QMO_STAT_INDEX_AVG_SLOT_COUNT               (100)
# define QMO_STAT_INDEX_HEIGHT                       (1)
# define QMO_STAT_INDEX_DISK_PAGE_COUNT              (1)
# define QMO_STAT_INDEX_CLUSTER_FACTOR               (1)

# define QMO_STAT_COLUMN_NDV                         (100)
# define QMO_STAT_COLUMN_NULL_VALUE_COUNT            (0)
# define QMO_STAT_COLUMN_AVG_LEN                     (20)

// bug-37125
// 측정값이 1보다 작게 측정이 되고
// 0이 나올는 경우도 있기 때문에 최소한의 값으로 지정한다.
# define QMO_STAT_TIME_MIN                           (QMO_COST_EPS)

// TASK-6699 TPC-H 성능 개선
# define QMO_STAT_READROW_TIME_MIN                   (0.05290000)

//-------------------------------------------------------
// qmoIdxCardInfo와 qmoColCardInfo의 flag 초기화
//-------------------------------------------------------
/* qmoColCardInfo와 qmoIdxCardInfo의 flag를 초기화시킴 */
# define QMO_STAT_CLEAR                       (0x00000000)

//-------------------------------------------------------
// qmoIdxCardInfo의 flag 정의
//-------------------------------------------------------

/* qmoIdxCardInfo.flag                                 */
// NO INDEX Hint가 주어진 경우
# define QMO_STAT_CARD_IDX_HINT_MASK          (0x00000007)
# define QMO_STAT_CARD_IDX_HINT_NONE          (0x00000000)
# define QMO_STAT_CARD_IDX_NO_INDEX           (0x00000001)
# define QMO_STAT_CARD_IDX_INDEX              (0x00000002)
# define QMO_STAT_CARD_IDX_INDEX_ASC          (0x00000003)
# define QMO_STAT_CARD_IDX_INDEX_DESC         (0x00000004)

// To Fix PR-11412
# define QMO_STAT_INDEX_HAS_ACCESS_HINT( aFlag )                                \
    (((aFlag & QMO_STAT_CARD_IDX_HINT_MASK) == QMO_STAT_CARD_IDX_INDEX)     ||  \
     ((aFlag & QMO_STAT_CARD_IDX_HINT_MASK) == QMO_STAT_CARD_IDX_INDEX_ASC) ||  \
     ((aFlag & QMO_STAT_CARD_IDX_HINT_MASK) == QMO_STAT_CARD_IDX_INDEX_DESC))

/* qmoIdxCardInfo.flag                                 */
// 해당 index를 포함하는 predicate의 존재 유무
# define QMO_STAT_CARD_IDX_EXIST_PRED_MASK    (0x00000010)
# define QMO_STAT_CARD_IDX_EXIST_PRED_FALSE   (0x00000000)
# define QMO_STAT_CARD_IDX_EXIST_PRED_TRUE    (0x00000010)

// To Fix PR-9181
/* qmoIdxCardInfo.flag                                 */
// IN SUBQUERY KEYRANGE가 사용되는 지의 여부
# define QMO_STAT_CARD_IDX_IN_SUBQUERY_MASK   (0x00000020)
# define QMO_STAT_CARD_IDX_IN_SUBQUERY_FALSE  (0x00000000)
# define QMO_STAT_CARD_IDX_IN_SUBQUERY_TRUE   (0x00000020)


/* qmoIdxCardInfo.flag                                 */
// PRIMARY KEY의 여부
# define QMO_STAT_CARD_IDX_PRIMARY_MASK       (0x00000040)
# define QMO_STAT_CARD_IDX_PRIMARY_FALSE      (0x00000000)
# define QMO_STAT_CARD_IDX_PRIMARY_TRUE       (0x00000040)

/* qmoIdxCardInfo.flag                                 */
// indexable 프리디킷으로만 index key column이 구성 여부
# define QMO_STAT_CARD_ALL_EQUAL_IDX_MASK     (0x00000080)
# define QMO_STAT_CARD_ALL_EQUAL_IDX_FALSE    (0x00000000)
# define QMO_STAT_CARD_ALL_EQUAL_IDX_TRUE     (0x00000080)

//-------------------------------------------------------
// qmoColCardInfo의 flag 정의
//-------------------------------------------------------

/* qmoColCardInfo.flag                                 */

/* qmoColCardInfo.flag                                 */
// column의 MIN, MAX 값이 설정되었는지의 여부
# define QMO_STAT_MINMAX_COLUMN_SET_MASK     (0x00000002)
# define QMO_STAT_MINMAX_COLUMN_SET_FALSE    (0x00000000)
# define QMO_STAT_MINMAX_COLUMN_SET_TRUE     (0x00000002)

/* qmoColCardInfo.flag                                 */
// fix BUG-11214 default cardinality가 설정되었는지의 여부
# define QMO_STAT_DEFAULT_CARD_COLUMN_SET_MASK   (0x00000004)
# define QMO_STAT_DEFAULT_CARD_COLUMN_SET_FALSE  (0x00000000)
# define QMO_STAT_DEFAULT_CARD_COLUMN_SET_TRUE   (0x00000004)


//-------------------------------------------------------
// 통계 정보를 관리하기 위한 자료 구조
//-------------------------------------------------------

//--------------------------------------
// SYSTEM Statistics
//--------------------------------------

typedef struct qmoSystemStatistics
{
    idBool       isValidStat;
    SDouble      singleIoScanTime;
    SDouble      multiIoScanTime;
    SDouble      mMTCallTime;
    SDouble      mHashTime;
    SDouble      mCompareTime;
    SDouble      mStoreTime;
    SDouble      mIndexNlJoinPenalty;
} qmoSystemStatistics;


//--------------------------------------
// Index Cardinality 정보
//--------------------------------------

typedef struct qmoKeyRangeColumn
{
    UInt    mColumnCount;
    UShort* mColumnArray;
} qmoKeyRangeColummn;

typedef struct qmoIdxCardInfo
{
    // index ordering과정에서 플랫폼간 diff 방지를 위해
    // qsort 내의 compare함수내에서 사용됨.
    idBool              isValidStat;

    UInt                indexId;
    UInt                flag;
    qcmIndex          * index;

    SDouble             KeyNDV;
    SDouble             avgSlotCount;
    SDouble             pageCnt;
    SDouble             indexLevel;
    SDouble             clusteringFactor;

    qmoKeyRangeColumn   mKeyRangeColumn;
} qmoIdxCardInfo;

//--------------------------------------
// Column Cardinality 정보
//--------------------------------------

typedef struct qmoColCardInfo
{
    //---------------------------------------------------
    // flag : cardinality와 MIN,MAX정보가 저장되었는지의 정보
    //---------------------------------------------------
    idBool              isValidStat;

    UInt                flag;
    mtcColumn         * column;        // column pointer

    SDouble             columnNDV;     // 해당 column의 cardinality 값
    SDouble             nullValueCount;
    SDouble             avgColumnLen;

    //---------------------------------------------------
    // MIN, MAX 값
    //    - MIN, MAX 값을 갖는 Data Type은 숫자형과
    //      날짜형이다.
    //    - 이 중 가장 큰 크기를 갖는 Numeric의 Maximum
    //      만큼 공간을 할당하여, Data Type에 따른
    //      구현의 복잡도를 줄인다.
    //---------------------------------------------------
    ULong               minValue[SMI_MAX_MINMAX_VALUE_SIZE/ID_SIZEOF(ULong)];
    ULong               maxValue[SMI_MAX_MINMAX_VALUE_SIZE/ID_SIZEOF(ULong)];
} qmoColCardInfo;

//-------------------------------------------------------
// 통계 정보
//-------------------------------------------------------

typedef struct qmoStatistics
{
    qmoOptGoalType      optGoleType;
    idBool              isValidStat;

    SDouble             totalRecordCnt;
    SDouble             pageCnt;
    SDouble             avgRowLen;
    SDouble             readRowTime;

    SDouble             firstRowsFactor;
    UInt                firstRowsN;

    UInt                indexCnt;          // Index의 개수
    qmoIdxCardInfo    * idxCardInfo;       // 각 index의 cardinality 정보

    UInt                columnCnt;         // Column의 개수
    qmoColCardInfo    * colCardInfo;       // 각 column의 cardinality 정보
} qmoStatistics;

/***********************************************************************
 * [Access Method를 결정하기 위한 자료 구조]
 *     method : Index에 대한 통계 정보
 *            : Full Scan의 경우 NULL 값임.
 ***********************************************************************/

typedef struct qmoAccessMethod
{
    qmoIdxCardInfo        * method;          // index와 cardinality 정보
    SDouble                 keyRangeSelectivity;
    SDouble                 keyFilterSelectivity;
    SDouble                 filterSelectivity;
    SDouble                 methodSelectivity;
    SDouble                 keyRangeCost;
    SDouble                 keyFilterCost;
    SDouble                 filterCost;
    SDouble                 accessCost;
    SDouble                 diskCost;
    SDouble                 totalCost;
} qmoAccessMethod;

//---------------------------------------------------
// 통계 정보를 관리하기 위한 함수
//---------------------------------------------------

class qmoStat
{
public:

    //--------------------------------------------
    // View에 대한 통계 정보 구축을 위한 Interface
    //--------------------------------------------

    static IDE_RC    getStatInfo4View( qcStatement     * aStatement,
                                       qmgGraph        * aGraph,
                                       qmoStatistics  ** aStatInfo);

    //--------------------------------------------
    // 일반 테이블에 대한 통계 정보를 구축한다.
    // qmsSFWGH->from에 달려있는 모든 Base Table에 대한 통계정보를 구축한다.
    // Validation 과정중에 호출됨.
    //--------------------------------------------
    static IDE_RC  getStatInfo4AllBaseTables( qcStatement    * aStatement,
                                              qmsSFWGH       * aSFWGH );

    // 통계 정보를 출력함.
    static IDE_RC  printStat( qmsFrom       * aFrom,
                              ULong           aDepth,
                              iduVarString  * aString );

    // PROJ-1502 PARTITIONED DISK TABLE
    // partition에 대한 통계 정보를 출력함.
    static IDE_RC  printStat4Partition( qmsTableRef     * aTableRef,
                                        qmsPartitionRef * aPartitionRef,
                                        SChar           * aPartitionName,
                                        ULong             aDepth,
                                        iduVarString    * aString );

    // qmoAccessMethod로 부터 index를 얻기 위해서
    // 항상 method가 NULL인지 검사를 해야 하는데,
    // 이 불편을 줄이기 위해서 한단계 추상화한다.
    inline static qcmIndex* getMethodIndex( qmoAccessMethod * aMethod )
    {
        IDE_DASSERT( aMethod != NULL );

        if( aMethod->method == NULL )
        {
            return NULL;
        }
        else
        {
            return aMethod->method->index;
        }
    }

    // PROJ-1502 PARTITIONED DISK TABLE
    // private->public
    // 하나의 Base Table에 대한 통계정보를 구축한다.
    static IDE_RC    getStatInfo4BaseTable( qcStatement    * aStatement,
                                            qmoOptGoalType   aOptimizerMode,
                                            qcmTableInfo   * aTableInfo,
                                            qmoStatistics ** aStatInfo );

    /* PROJ-1832 New database link */
    static IDE_RC getStatInfo4RemoteTable( qcStatement      * aStatement,
                                           qcmTableInfo     * aTableInfo,
                                           qmoStatistics   ** aStatInfo );

    // PROJ-1502 PARTITIONED DISK TABLE
    // tableRef에 달려있는 partitionRef의 통계정보를 이용하여
    // partitioned table에 대한 통계정보를 구축한다.
    // aStatInfo는 이미 할당되어 있으므로 pointer만을 넘긴다.
    static IDE_RC    getStatInfo4PartitionedTable(
        qcStatement    * aStatement,
        qmoOptGoalType   aOptimizerMode,
        qmsTableRef    * aTableRef,
        qmoStatistics  * aStatInfo );

    static IDE_RC    getSystemStatistics( qmoSystemStatistics * aStatistics );
    static void      getSystemStatistics4Rule( qmoSystemStatistics * aStatistics );

private:

    // 현재 테이블의 left, right 테이블을 순회하면서,
    // base table을 찾아서 통계정보를 구축한다.
    static IDE_RC    findBaseTableNGetStatInfo( qcStatement * aStatement,
                                                qmsSFWGH    * aSFWGH,
                                                qmsFrom     * aFrom );

    static IDE_RC    getTableStatistics( qcStatement   * aStatement,
                                         qcmTableInfo  * aTableInfo,
                                         qmoStatistics * aStatInfo,
                                         smiTableStat  * aData );

    static void      getTableStatistics4Rule( qmoStatistics * aStatistics,
                                              qcmTableInfo  * aTableInfo,
                                              idBool          aIsDiskTable );

    // 테이블이 차지하고 있는 총 disk page 수를 얻음.
    static IDE_RC    setTablePageCount( qcStatement    * aStatement,
                                        qcmTableInfo   * aTableInfo,
                                        qmoStatistics  * aStatInfo,
                                        smiTableStat   * aData );

    // 인덱스 통계정보
    static IDE_RC    getIndexStatistics( qcStatement   * aStatement,
                                         qmoStatistics * aStatistics,
                                         idBool          aIsAutoDBMStat,
                                         smiIndexStat  * aData );

    static void      getIndexStatistics4Rule( qmoStatistics * aStatistics,
                                              idBool          aIsDiskTable );

    static IDE_RC    setIndexPageCnt( qcStatement    * aStatement,
                                      qcmIndex       * aIndex,
                                      qmoIdxCardInfo * aIdxInfo,
                                      smiIndexStat   * aData );

    // 각 column의 cardinality와 MIX,MAX 값을 구해서,
    // 해당 통계정보 자료구조에 저장한다.
    static IDE_RC    getColumnStatistics( qmoStatistics * aStatistics,
                                          smiColumnStat * aData );

    static void      getColumnStatistics4Rule( qmoStatistics * aStatistics );

    static IDE_RC    setColumnNDV( qcmTableInfo   * aTableInfo,
                                   qmoColCardInfo * aColInfo );

    static IDE_RC    setColumnNullCount( qcmTableInfo   * aTableInfo,
                                         qmoColCardInfo * aColInfo );

    static IDE_RC    setColumnAvgLen( qcmTableInfo   * aTableInfo,
                                      qmoColCardInfo * aColInfo );

    // 통계정보의 idxCardInfo를 정렬한다.
    static IDE_RC    sortIndexInfo( qmoStatistics   * aStatInfo,
                                    idBool            aIsAscending,
                                    qmoIdxCardInfo ** aSortedIdxInfoArry );

    // PROJ-2492 Dynamic sample selection
    static IDE_RC    calculateSamplePercentage( qcmTableInfo   * aTableInfo,
                                                UInt             aAutoStatsLevel,
                                                SFloat         * aPercentage );

    //-----------------------------------------------------
    // TPC-H Data 없이 가상의 통계 정보 구축을 위한 Interface
    //-----------------------------------------------------

    // TPC-H 를 위한 가상의 통계 정보 구축
    static IDE_RC    getFakeStatInfo( qcStatement    * aStatement,
                                      qcmTableInfo   * aTableInfo,
                                      qmoStatistics  * aStatInfo );

    // REGION 테이블을 위한 가상 통계 정보 구축
    static IDE_RC    getFakeStat4Region( idBool          aIsDisk,
                                         qmoStatistics * aStatInfo );

    // NATION 테이블을 위한 가상 통계 정보 구축
    static IDE_RC    getFakeStat4Nation( idBool          aIsDisk,
                                         qmoStatistics * aStatInfo );

    // SUPPLIER 테이블을 위한 가상 통계 정보 구축
    static IDE_RC    getFakeStat4Supplier( idBool          aIsDisk,
                                           qmoStatistics * aStatInfo );

    // CUSTOMER 테이블을 위한 가상 통계 정보 구축
    static IDE_RC    getFakeStat4Customer( idBool          aIsDisk,
                                           qmoStatistics * aStatInfo );

    // PART 테이블을 위한 가상 통계 정보 구축
    static IDE_RC    getFakeStat4Part( idBool          aIsDisk,
                                       qmoStatistics * aStatInfo );

    // PARTSUPP 테이블을 위한 가상 통계 정보 구축
    static IDE_RC    getFakeStat4PartSupp( idBool          aIsDisk,
                                           qmoStatistics * aStatInfo );

    // ORDERS 테이블을 위한 가상 통계 정보 구축
    static IDE_RC    getFakeStat4Orders( qcStatement   * aStatement,
                                         idBool          aIsDisk,
                                         qmoStatistics * aStatInfo );

    // LINEITEM 테이블을 위한 가상 통계 정보 구축
    static IDE_RC    getFakeStat4LineItem( qcStatement   * aStatement,
                                           idBool          aIsDisk,
                                           qmoStatistics * aStatInfo );

};

#endif /* _O_QMO_STAT_MGR_H_ */
