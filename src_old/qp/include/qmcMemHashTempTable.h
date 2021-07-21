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
 * $Id: qmcMemHashTempTable.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *     Memory Hash Temp Table을 위한 정의
 *   - Chained Hash Algorithm을 사용한다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QMC_MEM_HASH_TMEP_TABLE_H_
#define _O_QMC_MEM_HASH_TMEP_TABLE_H_ 1

#include <smi.h>
#include <qtcDef.h>
#include <qmc.h>

// BUG-38961 hint 의 max 값과 실제 사용할수 있는 max 값을 동일하게 한다.
// 최대 Bucket 개수
#define QMC_MEM_HASH_MAX_BUCKET_CNT  QMS_MAX_BUCKET_CNT

// Bucket의 자동 확장 조건 ( Record 개수 > BucketCnt * 2 )
#define QMC_MEM_HASH_AUTO_EXTEND_CONDITION        ( 2 )

// Bucket의 자동 확장 정도 (BucketCnt = Record 개수 * 4 )
#define QMC_MEM_HASH_AUTO_EXTEND_RATIO            ( 4 )

/* qmcdMemHashTemp.flag                              */
#define QMC_MEM_HASH_INITIALIZE            (0x00000000)

/* qmcdMemHashTemp.flag                              */
// Distinct Insertion 여부
#define QMC_MEM_HASH_DISTINCT_MASK         (0x00000001)
#define QMC_MEM_HASH_DISTINCT_FALSE        (0x00000000)
#define QMC_MEM_HASH_DISTINCT_TRUE         (0x00000001)

/* qmcdMemHashTemp.flag                              */
// Bucket의 자동 확장 가능 여부
#define QMC_MEM_HASH_AUTO_EXTEND_MASK      (0x00000002)
#define QMC_MEM_HASH_AUTO_EXTEND_ENABLE    (0x00000000)
#define QMC_MEM_HASH_AUTO_EXTEND_DISABLE   (0x00000002)

struct qmcdMemHashTemp;

//---------------------------------------
// Insert를 위한 함수 포인터
//---------------------------------------

// Distinction인지 아닌지에 따라 상이한 함수를 호출한다.
typedef IDE_RC (* qmcHashInsertFunc) ( qmcdMemHashTemp * aTempTable,
                                       UInt              aHash, 
                                       void            * aElement,
                                       void           ** aOutElement );

//---------------------------------------
// Bucket 자료 구조
//---------------------------------------

typedef struct qmcBucket
{
    qmcMemHashElement * element;   // Bucket 내의 최초 Element
    qmcBucket         * next;      // Record가 있는 다음 Bucket
} qmcBucket;

//--------------------------------------------------------------
// [Memory Hash Temp Table의 자료 구조]
//
//               mBucket
//               --------
//               | elem |
//               | next |
//               --------
//         mLast | elem |---->[ ]---->[ ]
//        ------>| next |
//        |      --------
//        |      | elem |
//        |      | next |
//        |      --------
//        |  mTop| elem |---->[ ]
//        |   -- | next |
//        |   |   --------
//        |   -->| elem |---->[ ]---->[ ]---->[ ]
//        -------| next |             mNext
//               --------
//
// 위와 같이 Bucket간의 연결 관계를 유지하여 순차 검색의
// 성능을 향상시킨다.
// 즉, element가 없는 Bucket을 접근하지 않도록 한다.
//
//--------------------------------------------------------------

typedef struct qmcdMemHashTemp
{
    //----------------------------------------------------
    // Memory Hash Temp Table의 기본 정보
    //----------------------------------------------------

    UInt                 flag;         // Distinction여부
    qcTemplate         * mTemplate;    // Template
    iduMemory          * mMemory;
    
    UInt                 mBucketCnt;   // Bucket의 개수
    qmcBucket          * mBucket;      // 할당 받은 Bucket
    qmdMtrNode         * mRecordNode;  // Record 구성 정보
    qmdMtrNode         * mHashNode;    // Hashing 할 Column 정보

    //----------------------------------------------------
    // 삽입을 위한 정보
    //----------------------------------------------------
    
    qmcHashInsertFunc    insert;       // 삽입 함수 포인터
    qmcBucket          * mTop;         // 최초 Record가 있는 Bucket
    qmcBucket          * mLast;        // 마지막에 할당받은 Bucket
    SLong                mRecordCnt;   // 총 저장 Record 개수

    //----------------------------------------------------
    // 검색을 위한 정보
    //----------------------------------------------------
    
    qmcBucket          * mCurrent;     // 현재 검색중인 Bucket
    qmcMemHashElement  * mNext;        // 현재 검색중인 Element
    UInt                 mKey;         // 검색할 Hash Key 값
    qtcNode            * mFilter;      // Range 검색을 위한 Filter

    //----------------------------------------------------
    // 자동 Bucket 확장을 위한 정보
    //----------------------------------------------------

    UInt                 mExtBucketCnt; // 확장된 Bucket의 개수
    qmcBucket          * mExtBucket;    // 확장 Bucket
    qmcBucket          * mExtTop;       // 확장 Bucket의 최초 Bucket
    qmcBucket          * mExtLast;      // 확장 Bucket의 마지막 Bucket

} qmcdMemHashTemp;

class qmcMemHash
{
public:

    //------------------------------------------------
    // Memory Hash Temp Table의 관리
    //------------------------------------------------
    
    // 초기화를 한다.
    static IDE_RC  init( qmcdMemHashTemp * aTempTable,
                         qcTemplate      * aTemplate,
                         iduMemory       * aMemory,
                         qmdMtrNode      * aRecordNode,
                         qmdMtrNode      * aHashNode,
                         UInt              aBucketCnt,
                         idBool            aDistinct );
    
    // Temp Table내의 Bucket을 초기화한다.
    static IDE_RC  clear( qmcdMemHashTemp * aTempTable );

    // 모든 Record의 Hit Flag을 Reset함
    static IDE_RC clearHitFlag( qmcdMemHashTemp * aTempTable );

    //------------------------------------------------
    // Memory Sort Hash Table의 구성
    //------------------------------------------------

    // 최초 Record를 삽입한다.
    static IDE_RC  insertFirst( qmcdMemHashTemp * aTempTable,
                                UInt              aHash,
                                void            * aElement,
                                void           ** aOutElement );

    // 무조건 Record를 삽입한다.
    static IDE_RC  insertAny( qmcdMemHashTemp * aTempTable,
                              UInt             aHash, 
                              void           * aElement,
                              void          ** aOutElement );

    // 동일한 Record가 없을 경우 삽입한다.
    static IDE_RC  insertDist( qmcdMemHashTemp * aTempTable,
                               UInt              aHash, 
                               void            * aElement, 
                               void           ** aOutElement );

    //------------------------------------------------
    // Memory Hash Temp Table의 검색
    //------------------------------------------------

    //----------------------------
    // 순차 검색
    //----------------------------

    static IDE_RC  getFirstSequence( qmcdMemHashTemp * aTempTable,
                                     void           ** aElement );
    
    static IDE_RC  getNextSequence( qmcdMemHashTemp * aTempTable,
                                    void           ** aElement);

    //----------------------------
    // Range 검색
    //----------------------------
    
    static IDE_RC  getFirstRange( qmcdMemHashTemp * aTempTable,
                                  UInt              aHash,
                                  qtcNode         * aFilter,
                                  void           ** aElement );

    static IDE_RC  getNextRange( qmcdMemHashTemp * aTempTable,
                                 void           ** aElement );

    //----------------------------
    // Same Row & NonHit 검색
    //----------------------------

    static IDE_RC  getSameRowAndNonHit( qmcdMemHashTemp * aTempTable,
                                        UInt              aHash,
                                        void            * aRow,
                                        void           ** aElement );
    
    //----------------------------
    // Hit 검색
    //----------------------------

    static IDE_RC  getFirstHit( qmcdMemHashTemp * aTempTable,
                                void           ** aElement );

    static IDE_RC  getNextHit( qmcdMemHashTemp * aTempTable,
                               void           ** aElement );
    
    //----------------------------
    // NonHit 검색
    //----------------------------

    static IDE_RC  getFirstNonHit( qmcdMemHashTemp * aTempTable,
                                   void          ** aElement );

    static IDE_RC  getNextNonHit( qmcdMemHashTemp * aTempTable,
                               void           ** aElement );
    
    //----------------------------
    // 기타 함수
    //----------------------------

    // 수행 비용 정보 획득
    static IDE_RC  getDisplayInfo( qmcdMemHashTemp * aTempTable,
                                   SLong           * aRecordCnt,
                                   UInt            * aBucketCnt );
    
private:

    // Bucket ID를 찾는다.
    static UInt   getBucketID( qmcdMemHashTemp * aTempTable,
                               UInt              aHash );

    // 두 record간의 대소 비교
    static SInt   compareRow ( qmcdMemHashTemp * aTempTable, 
                               void            * aElem1, 
                               void            * aElem2 );

    // Range 검색시 조건 검사
    static IDE_RC judgeFilter ( qmcdMemHashTemp * aTempTable,
                                void            * aElem,
                                idBool          * aResult );

    // Bucket의 자동 확장
    static IDE_RC extendBucket (qmcdMemHashTemp * aTempTable );

    // 새로운 Bucket에 최초 Record 삽입
    static IDE_RC insertFirstNewBucket( qmcdMemHashTemp   * aTempTable,
                                        qmcMemHashElement * aElem );

    // 새로운 Bucket에 다음 record 삽입
    static IDE_RC insertNextNewBucket( qmcdMemHashTemp   * aTempTable,
                                       qmcMemHashElement * aElem );
    
};

#endif /* _O_QMC_MEM_HASH_TMEP_TABLE_H_ */

