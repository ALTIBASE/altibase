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
 * $Id: qmcHashTempTable.h 85333 2019-04-26 02:34:41Z et16 $
 *
 * Description :
 *   [Hash Temp Table]
 *
 *   A4에서는 Memory 및 Disk Table을 모두 지원한다.
 *   중간 결과를 저장하는 Node들 중 Hash를 이용한 중간 결과를 만드는
 *   Materialized Node들은 Memory Hash Temp Table 또는 Disk Hash Temp
 *   Table을 사용하게 된다.
 *
 *   이 때, 각 노드가 Memory/Disk Hash Temp Table에 따라 서로 다른
 *   동작을 하지 않도록 하기 위하여 다음과 같은 Hash Temp Table을
 *   이용하여 Transparent한 구현 및 동작이 가능하도록 한다.
 *
 *   이러한 개념을 나타낸 그림은 아래와 같다.
 *
 *                                                     -------------
 *                                                  -->[Memory Hash]
 *     ---------------     -------------------      |  -------------
 *     | Plan Node   |---->| Hash Temp Table |------|  -----------
 *     ---------------     -------------------      -->[Disk Hash]
 *                                                     -----------
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMC_HASH_TMEP_TABLE_H_
#define _O_QMC_HASH_TMEP_TABLE_H_ 1

#include <qmcMemHashTempTable.h>
#include <qmcMemPartHashTempTable.h>
#include <qmcDiskHashTempTable.h>

/* qmcdHashTempTable.flag                                  */
#define QMCD_HASH_TMP_INITIALIZE                 (0x00000000)

/* qmcdHashTempTable.flag                                  */
// Hash Temp Table의 저장 매체
#define QMCD_HASH_TMP_STORAGE_TYPE               (0x00000001)
#define QMCD_HASH_TMP_STORAGE_MEMORY             (0x00000000)
#define QMCD_HASH_TMP_STORAGE_DISK               (0x00000001)

/* qmcdHashTempTable.flag                                  */
// Distinct의 여부
#define QMCD_HASH_TMP_DISTINCT_MASK              (0x00000002)
#define QMCD_HASH_TMP_DISTINCT_FALSE             (0x00000000)
#define QMCD_HASH_TMP_DISTINCT_TRUE              (0x00000002)

/* qmcdHashTempTable.flag                                  */
// Primary Temp Table의 여부
#define QMCD_HASH_TMP_PRIMARY_MASK               (0x00000004)
#define QMCD_HASH_TMP_PRIMARY_TRUE               (0x00000000)
#define QMCD_HASH_TMP_PRIMARY_FALSE              (0x00000004)

/* qmcdHashTempTable.flag                                  */
// Temp Table의 rid fix 여부
#define QMCD_HASH_TMP_FIXED_RID_MASK             (0x00000008)
#define QMCD_HASH_TMP_FIXED_RID_FALSE            (0x00000000)
#define QMCD_HASH_TMP_FIXED_RID_TRUE             (0x00000008)

/* qmcdHashTempTable.flag                                  */
// PROJ-2553 Cache-aware Memory Hash Temp Table
// Hash Temp Table의 저장/탐색 방법 (Bucket List / Partitioned Array)
#define QMCD_HASH_TMP_HASHING_TYPE               (0x00000010)
#define QMCD_HASH_TMP_HASHING_BUCKET             (0x00000000)
#define QMCD_HASH_TMP_HASHING_PARTITIONED        (0x00000010)

// Hash Temp Table 자료 구조
typedef struct qmcdHashTemp
{
    UInt                  flag;

    qcTemplate          * mTemplate;
    qmdMtrNode          * recordNode;       // Record 구성 정보
    qmdMtrNode          * hashNode;         // Hash할 Column 정보
    qmdMtrNode          * aggrNode;         // Aggregation Node 정보

    UInt                  bucketCnt;        // Bucket개수
    UInt                  mtrRowSize;       // 저장 Record의 크기
    UInt                  nullRowSize;      // 저장 null Record의 크기
    qmcMemory           * memoryMgr;        // 저장 Record를 위한 메모리 관리자
    iduMemory           * memory;           // Hash Temp에 사용되는 Memory
    UInt                  memoryIdx;

    SChar               * nullRow;          // Memory Temp Table을 위한 Null Row

    UInt                  hashKey;          // Range검색을 위한 Hash Key

    qmcdMemHashTemp     * memoryTemp;       // Memory Hash Temp Table
    qmcdMemPartHashTemp * memoryPartTemp;   // PROJ-2553 Memory Partitioned Hash Temp Table
    qmcdDiskHashTemp    * diskTemp;         // Disk Hash Temp Table

    idBool                existTempType;    // TEMP_TYPE 컬럼이 존재하는지
    void                * insertRow;        // insert를 위한 임시 버퍼

} qmcdHashTemp;

class qmcHashTemp
{
public:

    //------------------------------------------------
    // Hash Temp Table의 관리
    //------------------------------------------------

    // Temp Table의 초기화
    static IDE_RC      init( qmcdHashTemp * aTempTable,
                             qcTemplate   * aTemplate,
                             UInt           aMemoryIdx,
                             qmdMtrNode   * aRecordNode,
                             qmdMtrNode   * aHashNode,
                             qmdMtrNode   * aAggrNode,
                             UInt           aBucketCnt,
                             UInt           aFlag );

    // Temp Table의 Data를 제거한다.
    static IDE_RC      clear( qmcdHashTemp * aTempTable );

    // Temp Table내의 모든 Data의 flag을 초기화한다.
    static IDE_RC      clearHitFlag( qmcdHashTemp * aTempTable );

    //------------------------------------------------
    // Hash Temp Table의 구성
    //------------------------------------------------

    // Data를 위한 Memory 할당
    static IDE_RC      alloc( qmcdHashTemp * aTempTable,
                              void        ** aRow );

    // Non-Distinction Data 삽입
    static IDE_RC      addRow( qmcdHashTemp * aTempTable,
                               void         * aRow );

    // Distinction Data 삽입
    static IDE_RC      addDistRow( qmcdHashTemp  * aTempTable,
                                   void         ** aRow,
                                   idBool        * aResult );

    // insert row 생성
    static IDE_RC      makeTempTypeRow( qmcdHashTemp  * aTempTable,
                                        void          * aRow,
                                        void         ** aExtRow );

    // Aggregation Column에 대한 Update를 수행
    static IDE_RC      updateAggr( qmcdHashTemp * aTempTable );

    // To Fix PR-8415
    // Aggregation Column의 Final Update
    static IDE_RC      updateFiniAggr( qmcdHashTemp * aTempTable );

    // To Fix PR-8213
    // Group Aggregation에서만 사용되며, 새로운 Group을 등록한다.
    static IDE_RC      addNewGroup( qmcdHashTemp * aTempTable,
                                    void         * aRow );

    // To Fix PR-8415
    // Disk Temp Table을 사용할 경우 Aggregation의 최종 결과를
    // 저장하여야 한다.  이를 위해 순차적으로 Group을 읽어
    // 처리할 수 있는 인터페이스를 제공한다.
    // 일반 순차 검색을 사용하지 않는 이유는
    // Aggregation 결과를 Update해야 하기 때문이다.
    // 첫번째 group 순차 검색
    static IDE_RC      getFirstGroup( qmcdHashTemp * aTempTable,
                                      void        ** aRow );

    // 다음 group 순차 검색
    inline static IDE_RC getNextGroup( qmcdHashTemp * aTempTable,
                                       void        ** aRow );

    //------------------------------------------------
    // Hash Temp Table의 검색
    //------------------------------------------------

    //-------------------------
    // 순차 검색
    //-------------------------

    static IDE_RC      getFirstSequence( qmcdHashTemp * aTempTable,
                                         void        ** aRow );
    inline static IDE_RC getNextSequence( qmcdHashTemp * aTempTable,
                                          void        ** aRow );

    //-------------------------
    // Range 검색
    //-------------------------

    inline static IDE_RC  getFirstRange( qmcdHashTemp * aTempTable,
                                         UInt           aHashKey,
                                         qtcNode      * aHashFilter,
                                         void        ** aRow );
    inline static IDE_RC getNextRange( qmcdHashTemp * aTempTable,
                                       void        ** aRow );

    //-------------------------
    // Hit 검색
    //-------------------------

    static IDE_RC      getFirstHit( qmcdHashTemp * aTempTable,
                                    void        ** aRow );
    inline static IDE_RC getNextHit( qmcdHashTemp * aTempTable,
                                   void        ** aRow );

    //-------------------------
    // Non-Hit 검색
    //-------------------------

    static IDE_RC      getFirstNonHit( qmcdHashTemp * aTempTable,
                                       void        ** aRow );
    inline static IDE_RC getNextNonHit( qmcdHashTemp * aTempTable,
                                        void        ** aRow );

    //-------------------------
    // Same Row And Non-Hit 검색
    //-------------------------

    static IDE_RC      getSameRowAndNonHit( qmcdHashTemp * aTempTable,
                                            void         * aRow,
                                            void        ** aResultRow );

    //-------------------------
    // Null Row 검색
    //-------------------------

    static IDE_RC      getNullRow( qmcdHashTemp * aTempTable,
                                   void        ** aRow );
    //-------------------------
    // To Fix PR-8213
    // Same Group 검색 (Group Aggregation)에서만 사용
    //-------------------------

    static IDE_RC      getSameGroup( qmcdHashTemp  * aTempTable,
                                     void         ** aRow,
                                     void         ** aResultRow );

    //------------------------------------------------
    // Hash Temp Table의 기타 함수
    //------------------------------------------------

    // 검색된 Record에 Hit Flag 설정
    inline static IDE_RC setHitFlag( qmcdHashTemp * aTempTable );

    // 검색된 Record의 Hit Flag가 설정되어 있는지 확인
    inline static idBool isHitFlagged( qmcdHashTemp * aTempTable );

    static IDE_RC      getDisplayInfo( qmcdHashTemp * aTempTable,
                                       ULong        * aDiskPage,
                                       SLong        * aRecordCnt,
                                       UInt         * aBucketCnt );

private:

    //------------------------------------------------
    // 초기화를 위한 함수
    //------------------------------------------------

    // Memory Temp Table을 위한 NULL ROW 생성
    static IDE_RC makeMemNullRow( qmcdHashTemp * aTempTable );

    //------------------------------------------------
    // 초기화를 위한 함수
    //------------------------------------------------

    static IDE_RC getHashKey( qmcdHashTemp * aTempTable,
                              void         * aRow,
                              UInt         * aHashKey );
};

IDE_RC
qmcHashTemp::getFirstRange( qmcdHashTemp * aTempTable,
                            UInt           aHashKey,
                            qtcNode      * aHashFilter,
                            void        ** aRow )
{
/***********************************************************************
 *
 * Description :
 *    첫번째 Range 검색
 *
 * Implementation :
 *    검색을 위한 Hash Key는 각 Plan Node에서 생성하여 전달한다.
 *
 ***********************************************************************/

    if ( (aTempTable->flag & QMCD_HASH_TMP_STORAGE_TYPE)
         == QMCD_HASH_TMP_STORAGE_MEMORY )
    {
        //-----------------------------------------
        // Memory Hash Temp Table을 사용하는 경우
        //-----------------------------------------

        if ( ( aTempTable->flag & QMCD_HASH_TMP_HASHING_TYPE )
             == QMCD_HASH_TMP_HASHING_PARTITIONED )
        {
            IDE_TEST( qmcMemPartHash::getFirstRange( aTempTable->memoryPartTemp,
                                                     aHashKey,
                                                     aHashFilter,
                                                     aRow )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( qmcMemHash::getFirstRange( aTempTable->memoryTemp,
                                                 aHashKey,
                                                 aHashFilter,
                                                 aRow )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        //-----------------------------------------
        // Disk Hash Temp Table을 사용하는 경우
        //-----------------------------------------

        IDE_TEST( qmcDiskHash::getFirstRange( aTempTable->diskTemp,
                                              aHashKey,
                                              aHashFilter,
                                              aRow )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmcHashTemp::getNextRange( qmcdHashTemp * aTempTable,
                           void        ** aRow )
{
/***********************************************************************
 *
 * Description :
 *    다음 Range 검색
 *
 * Implementation :
 *
 ***********************************************************************/

    if ( (aTempTable->flag & QMCD_HASH_TMP_STORAGE_TYPE)
         == QMCD_HASH_TMP_STORAGE_MEMORY )
    {
        //-----------------------------------------
        // Memory Hash Temp Table을 사용하는 경우
        //-----------------------------------------

        if ( ( aTempTable->flag & QMCD_HASH_TMP_HASHING_TYPE )
             == QMCD_HASH_TMP_HASHING_PARTITIONED )
        {
            IDE_TEST( qmcMemPartHash::getNextRange( aTempTable->memoryPartTemp,
                                                    aRow )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( qmcMemHash::getNextRange( aTempTable->memoryTemp,
                                                aRow )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        //-----------------------------------------
        // Disk Hash Temp Table을 사용하는 경우
        //-----------------------------------------

        IDE_TEST( qmcDiskHash::getNextRange( aTempTable->diskTemp,
                                             aRow )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmcHashTemp::getNextSequence( qmcdHashTemp * aTempTable,
                              void        ** aRow )
{
/***********************************************************************
 *
 * Description :
 *    다음 순차 검색
 *
 * Implementation :
 *
 ***********************************************************************/

    if ( (aTempTable->flag & QMCD_HASH_TMP_STORAGE_TYPE)
         == QMCD_HASH_TMP_STORAGE_MEMORY )
    {
        //-----------------------------------------
        // Memory Hash Temp Table을 사용하는 경우
        //-----------------------------------------

        if ( ( aTempTable->flag & QMCD_HASH_TMP_HASHING_TYPE )
             == QMCD_HASH_TMP_HASHING_PARTITIONED )
        {
            IDE_TEST( qmcMemPartHash::getNextSequence( aTempTable->memoryPartTemp,
                                                       aRow )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( qmcMemHash::getNextSequence( aTempTable->memoryTemp,
                                                   aRow )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        //-----------------------------------------
        // Disk Hash Temp Table을 사용하는 경우
        //-----------------------------------------

        IDE_TEST( qmcDiskHash::getNextSequence( aTempTable->diskTemp,
                                                aRow )
                  != IDE_SUCCESS );

    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmcHashTemp::getNextGroup( qmcdHashTemp * aTempTable,
                           void        ** aRow )
{
/***********************************************************************
 *
 * Description :
 *    다음 Group 순차 검색
 *
 * Implementation :
 *
 ***********************************************************************/

    if ( (aTempTable->flag & QMCD_HASH_TMP_STORAGE_TYPE)
         == QMCD_HASH_TMP_STORAGE_MEMORY )
    {
        //-----------------------------------------
        // Memory Hash Temp Table을 사용하는 경우
        //-----------------------------------------

        // PROJ-2553 Grouping Hashing에서는 
        // Partitioned Array Hashing을 사용해서는 안 된다.
        IDE_DASSERT( ( aTempTable->flag & QMCD_HASH_TMP_HASHING_TYPE )
                         != QMCD_HASH_TMP_HASHING_PARTITIONED );

        IDE_TEST( qmcMemHash::getNextSequence( aTempTable->memoryTemp,
                                               aRow )
                  != IDE_SUCCESS );
    }
    else
    {
        //-----------------------------------------
        // Disk Hash Temp Table을 사용하는 경우
        //-----------------------------------------

        IDE_TEST( qmcDiskHash::getNextGroup( aTempTable->diskTemp,
                                             aRow )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmcHashTemp::getNextHit( qmcdHashTemp * aTempTable,
                         void        ** aRow )
{
/***********************************************************************
 *
 * Description :
 *     다음 Hit 된 Row검색
 *
 * Implementation :
 *
 ***********************************************************************/

    if ( (aTempTable->flag & QMCD_HASH_TMP_STORAGE_TYPE)
         == QMCD_HASH_TMP_STORAGE_MEMORY )
    {
        //-----------------------------------------
        // Memory Hash Temp Table을 사용하는 경우
        //-----------------------------------------

        if ( ( aTempTable->flag & QMCD_HASH_TMP_HASHING_TYPE )
             == QMCD_HASH_TMP_HASHING_PARTITIONED )
        {
            IDE_TEST( qmcMemPartHash::getNextHit( aTempTable->memoryPartTemp, aRow )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( qmcMemHash::getNextHit( aTempTable->memoryTemp, aRow )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        //-----------------------------------------
        // Disk Hash Temp Table을 사용하는 경우
        //-----------------------------------------

        IDE_TEST( qmcDiskHash::getNextHit( aTempTable->diskTemp, aRow )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}




IDE_RC
qmcHashTemp::getNextNonHit( qmcdHashTemp * aTempTable,
                            void        ** aRow )
{
/***********************************************************************
 *
 * Description :
 *    다음 Non-Hit Row검색
 *
 * Implementation :
 *
 ***********************************************************************/

    if ( (aTempTable->flag & QMCD_HASH_TMP_STORAGE_TYPE)
         == QMCD_HASH_TMP_STORAGE_MEMORY )
    {
        //-----------------------------------------
        // Memory Hash Temp Table을 사용하는 경우
        //-----------------------------------------

        if ( ( aTempTable->flag & QMCD_HASH_TMP_HASHING_TYPE )
             == QMCD_HASH_TMP_HASHING_PARTITIONED )
        {
            IDE_TEST( qmcMemPartHash::getNextNonHit( aTempTable->memoryPartTemp, aRow )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( qmcMemHash::getNextNonHit( aTempTable->memoryTemp, aRow )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        //-----------------------------------------
        // Disk Hash Temp Table을 사용하는 경우
        //-----------------------------------------

        IDE_TEST( qmcDiskHash::getNextNonHit( aTempTable->diskTemp,
                                              aRow )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmcHashTemp::setHitFlag( qmcdHashTemp * aTempTable )
{
/***********************************************************************
 *
 * Description :
 *    현재 읽어간 Record에 Hit Flag을 설정한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmcMemHashElement * sElement;

    if ( (aTempTable->flag & QMCD_HASH_TMP_STORAGE_TYPE)
         == QMCD_HASH_TMP_STORAGE_MEMORY )
    {
        //-----------------------------------------
        // Memory Hash Temp Table을 사용하는 경우
        //    1. 현재 읽고 있는 Record를 찾는다.
        //    2. 해당 Record에 Hit Flag을 setting한다.
        //-----------------------------------------

        // 현재 읽고 있는 Record 검색
        sElement = (qmcMemHashElement*) aTempTable->recordNode->dstTuple->row;

        // 해당 Record에 Hit Flag Setting
        sElement->flag &= ~QMC_ROW_HIT_MASK;
        sElement->flag |= QMC_ROW_HIT_TRUE;
    }
    else
    {
        //-----------------------------------------
        // Disk Hash Temp Table을 사용하는 경우
        //-----------------------------------------

        IDE_TEST( qmcDiskHash::setHitFlag( aTempTable->diskTemp )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool qmcHashTemp::isHitFlagged( qmcdHashTemp * aTempTable )
{
/***********************************************************************
 *
 * Description  
 *    현재 읽어간 Record에 Hit Flag가 있는지 판단한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmcMemHashElement * sElement;
    idBool              sIsHitFlagged = ID_FALSE;

    if ( (aTempTable->flag & QMCD_HASH_TMP_STORAGE_TYPE)
         == QMCD_HASH_TMP_STORAGE_MEMORY )
    {
        //----------------------------------------------
        // Memory Hash Temp Table을 사용하는 경우
        //    1. 현재 읽고 있는 Record를 찾는다.
        //    2. 해당 Record에 Hit Flag가 있는지 판단한다.
        //----------------------------------------------

        // 현재 읽고 있는 Record 검색
        sElement = (qmcMemHashElement*) aTempTable->recordNode->dstTuple->row;

        // 해당 Record에 Hit Flag가 있는지 판단
        if ( ( sElement->flag & QMC_ROW_HIT_MASK ) == QMC_ROW_HIT_TRUE )
        {
            sIsHitFlagged = ID_TRUE;
        }
        else
        {
            sIsHitFlagged = ID_FALSE;
        }
    }
    else
    {
        //-----------------------------------------
        // Disk Hash Temp Table을 사용하는 경우
        //-----------------------------------------

        sIsHitFlagged = qmcDiskHash::isHitFlagged( aTempTable->diskTemp );
    }

    return sIsHitFlagged;
}

#endif /* _O_QMC_HASH_TMEP_TABLE_H_ */
