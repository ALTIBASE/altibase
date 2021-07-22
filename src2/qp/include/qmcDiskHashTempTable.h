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
 * $Id: qmcDiskHashTempTable.h 85333 2019-04-26 02:34:41Z et16 $
 *
 * Description :
 *     Disk Hash Temp Table을 위한 정의
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMC_DISK_HASH_TMEP_TABLE_H_
#define _O_QMC_DISK_HASH_TMEP_TABLE_H_ 1

#include <qc.h>
#include <qmc.h>
#include <qmcTempTableMgr.h>
#include <smiDef.h>

/* qmcdDiskHashTemp.flag                                    */
#define QMCD_DISK_HASH_TEMP_INITIALIZE            (0x00000000)

/* qmcdDiskHashTemp.flag                                    */
// Distinct Insertion 여부
#define QMCD_DISK_HASH_INSERT_DISTINCT_MASK       (0x00000001)
#define QMCD_DISK_HASH_INSERT_DISTINCT_FALSE      (0x00000000)
#define QMCD_DISK_HASH_INSERT_DISTINCT_TRUE       (0x00000001)

//---------------------------------------------------
// [Disk Hash Temp Table 객체]
//    초기화 시점에 대한 설명
//    (I) : 최초 한번만 초기화됨
//    (R) : 최초 삽입 및 검색시에 초기화됨
//    (X) : 가변적임
//---------------------------------------------------

typedef struct qmcdDiskHashTemp
{
    UInt             flag;            // (X)
    qcTemplate     * mTemplate;       // (I)
    void           * tableHandle;     // (I)
    ULong            mTempTableSize;

    // Record 구성을 위한 자료 구조
    qmdMtrNode     * recordNode;      // (I) Record 구성 정보
    UInt             recordColumnCnt; // (I) Column 개수 + 1(Hit Flag)
    smiColumnList  * insertColumn;    // (I) Insert Column 정보
    smiValue       * insertValue;     // (I) Insert Value 정보

    // Hashing 을 위한 자료 구조
    qmdMtrNode     * hashNode;          // (I) Hash할 컬럼 정보
    UInt             hashColumnCnt;     // (I) 인덱스 컬럼의 개수
    smiColumnList  * hashKeyColumnList; // (I) Key내의 인덱스 컬럼 정보
    mtcColumn      * hashKeyColumn;     // (I) 인덱스 컬럼 정보

    // 검색을 위한 자료 구조
    smiHashTempCursor  * searchCursor;     // (R) 검색 및 Update를 위한 Cursor
    smiHashTempCursor  * groupCursor;      // (R) Group검색 및 Aggregation Update
    smiHashTempCursor  * groupFiniCursor;  // (R) Group의 Aggregation Finalization
    smiHashTempCursor  * hitFlagCursor;    // (R) Hit Flag 검사를 위한 Cursor
    qtcNode        * hashFilter;       // (X) Hash Filter

    // Filter 생성을 위한 자료 구조
    smiCallBack         filterCallBack;     // (R) CallBack
    qtcSmiCallBackData  filterCallBackData; // (R) CallBack Data

    // Aggregation Update를 위한 자료 구조
    qmdMtrNode     * aggrNode;        // (I) Aggregation Column 정보
    UInt             aggrColumnCnt;   // (I) Aggregation Column의 개수
    smiColumnList  * aggrColumnList;  // (I) Aggregation Column 정보
    smiValue       * aggrValue;       // (R) Aggregation Value 정보
} qmcdDiskHashTemp;

class qmcDiskHash
{
public:
    //------------------------------------------------
    // Disk Hash Temp Table의 관리
    //------------------------------------------------

    // Temp Table을 초기화
    static IDE_RC    init( qmcdDiskHashTemp * aTempTable,
                           qcTemplate       * aTemplate,   // Template
                           qmdMtrNode       * aRecordNode, // Record 구성
                           qmdMtrNode       * aHashNode,   // Hashing Column
                           qmdMtrNode       * aAggrNode,   // Aggregation
                           UInt               aBucketCnt,  // Bucket Count
                           UInt               aMtrRowSize, // Mtr Record Size
                           idBool             aDistinct ); // Distinction여부

    // Temp Table내의 모든 Record의 제거
    static IDE_RC    clear( qmcdDiskHashTemp * aTempTable );

    // Temp Table내의 모든 Record의 Flag을 Clear
    static IDE_RC    clearHitFlag( qmcdDiskHashTemp * aTempTable );

    //------------------------------------------------
    // Disk Hash Temp Table의 구성
    //------------------------------------------------

    // Record의 삽입
    static IDE_RC    insert( qmcdDiskHashTemp * aTempTable,
                             UInt               aHashKey,
                             idBool           * aResult );

    // Aggregation Column의 Update
    static IDE_RC    updateAggr( qmcdDiskHashTemp * aTempTable );

    // Aggregation Column의 Final Update
    static IDE_RC    updateFiniAggr( qmcdDiskHashTemp * aTempTable );

    // 현재 Cursor 위치의 Record의 Hit Flag을 Setting
    static IDE_RC    setHitFlag( qmcdDiskHashTemp * aTempTable );

    // 현재 Cursor 위치의 Record에 Hit Flag가 있는지 판단
    static idBool    isHitFlagged( qmcdDiskHashTemp * aTempTable );

    // Update Cursor를 이용한 순차 검색
    static IDE_RC    getFirstGroup( qmcdDiskHashTemp * aTempTable,
                                    void            ** aRow );
    // Update Cursor를 이용한 순차 검색
    static IDE_RC    getNextGroup( qmcdDiskHashTemp * aTempTable,
                                   void            ** aRow );

    //------------------------------------------------
    // Disk Hash Temp Table의 검색
    //------------------------------------------------

    //----------------------------
    // 순차 검색
    //----------------------------

    static IDE_RC    getFirstSequence( qmcdDiskHashTemp * aTempTable,
                                       void            ** aRow );
    static IDE_RC    getNextSequence( qmcdDiskHashTemp * aTempTable,
                                      void            ** aRow );

    //----------------------------
    // Range 검색
    //----------------------------

    static IDE_RC    getFirstRange( qmcdDiskHashTemp * aTempTable,
                                    UInt               aHash,
                                    qtcNode          * aHashFilter,
                                    void            ** aRow );
    static IDE_RC    getNextRange( qmcdDiskHashTemp * aTempTable,
                                   void            ** aRow );

    //----------------------------
    // Hit 검색
    //----------------------------

    static IDE_RC    getFirstHit( qmcdDiskHashTemp * aTempTable,
                                  void            ** aRow );
    static IDE_RC    getNextHit( qmcdDiskHashTemp * aTempTable,
                                 void            ** aRow );

    //----------------------------
    // NonHit 검색
    //----------------------------

    static IDE_RC    getFirstNonHit( qmcdDiskHashTemp * aTempTable,
                                     void            ** aRow );
    static IDE_RC    getNextNonHit( qmcdDiskHashTemp * aTempTable,
                                    void            ** aRow );

    //----------------------------
    // Same Row & NonHit 검색
    //----------------------------

    static IDE_RC    getSameRowAndNonHit( qmcdDiskHashTemp * aTempTable,
                                          UInt               aHash,
                                          void             * aRow,
                                          void            ** aResultRow );

    static IDE_RC    getNullRow( qmcdDiskHashTemp * aTempTable,
                                 void            ** aRow );

    //-------------------------
    // To Fix PR-8213
    // Same Group 검색 (Group Aggregation)에서만 사용
    //-------------------------

    static IDE_RC    getSameGroup( qmcdDiskHashTemp * aTempTable,
                                   UInt               aHash,
                                   void             * aRow,
                                   void            ** aResultRow );

    //------------------------------------------------
    // 기타 함수
    //------------------------------------------------

    // 수행 비용 정보 획득
    static IDE_RC  getDisplayInfo( qmcdDiskHashTemp * aTempTable,
                                   ULong            * aPageCount,
                                   SLong            * aRecordCount );

private:

    //------------------------------------------------
    // 초기화를 위한 함수
    //------------------------------------------------
    // Temp Table Size의 예측
    static void    tempSizeEstimate( qmcdDiskHashTemp * aTempTable,
                                     UInt               aBucketCnt,
                                     UInt               aMtrRowSize );

    // Temp Table의 생성
    static IDE_RC  createTempTable( qmcdDiskHashTemp * aTempTable );

    // Insert Column List를 위한 정보 구성
    static IDE_RC  setInsertColumnList( qmcdDiskHashTemp * aTempTable );

    // Aggregation Column List를 위한 정보 구성
    static IDE_RC  setAggrColumnList( qmcdDiskHashTemp * aTempTable );

    // Index Key내에서의 Index Column 정보의 구축
    static IDE_RC  makeIndexColumnInfoInKEY( qmcdDiskHashTemp * aTempTable);

    //------------------------------------------------
    // 삽입 및 검색을 위한 함수
    //------------------------------------------------

    // Insert를 위한 Value 정보 구성
    static IDE_RC  makeInsertSmiValue ( qmcdDiskHashTemp * aTempTable );

    // Aggregation의 Update를 위한 Value 정보 구성
    static IDE_RC  makeAggrSmiValue ( qmcdDiskHashTemp * aTempTable );

    // 주어진 Filter를 만족하는지를 검사하는 Filter 구성
    static IDE_RC  makeHashFilterCallBack( qmcdDiskHashTemp * aTempTable );

    // 두 Row가 동일 Hash Value인지를 하는 Filter CallBack의 생성
    static IDE_RC  makeTwoRowHashFilterCallBack( qmcdDiskHashTemp * aTempTable,
                                                 void             * aRow );

    // 두 Row간의 Hashing Column이 다른지를 검사
    static IDE_RC  hashTwoRowRangeFilter( idBool     * aResult,
                                          const void * aRow,
                                          void       *, /* aDirectKey */
                                          UInt        , /* aDirectKeyPartialSize */
                                          const scGRID aRid,
                                          void       * aData );

    // 두 Row가 동일한 Hashing Column이고 Hit Flag이 없는 지를 검사.
    static IDE_RC  hashTwoRowAndNonHitFilter( idBool     * aResult,
                                              const void * aRow,
                                              const scGRID  aRid,
                                              void       * aData );
};

#endif /* _O_QMC_DISK_HASH_TMEP_TABLE_H_ */
