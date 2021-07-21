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
 * $Id: qmcMemPartHashTempTable.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *     Memory Partitioned Hash Temp Table을 위한 함수
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qtc.h>
#include <qmcMemPartHashTempTable.h>

IDE_RC qmcMemPartHash::init( qmcdMemPartHashTemp * aTempTable,
                             qcTemplate          * aTemplate,
                             iduMemory           * aMemory,
                             qmdMtrNode          * aRecordNode,
                             qmdMtrNode          * aHashNode,
                             UInt                  aBucketCnt )
{
/***********************************************************************
 *
 * Description :
 *    Memory Partitioned Hash Temp Table을 초기화한다.
 *
 * Implementation :
 *    - 기본 정보 설정
 *    - 삽입 정보 설정
 *    - 검색 정보 설정
 *
 ***********************************************************************/

    // 적합성 검사
    IDE_DASSERT( aTempTable  != NULL );
    IDE_DASSERT( aRecordNode != NULL && aHashNode != NULL );

    //----------------------------------------------------
    // Memory Partitioned Hash Temp Table의 기본 정보 설정
    //----------------------------------------------------

    aTempTable->mFlag         = QMC_MEM_PART_HASH_INITIALIZE;
    aTempTable->mTemplate     = aTemplate;
    aTempTable->mMemory       = aMemory;
    aTempTable->mRecordNode   = aRecordNode;
    aTempTable->mHashNode     = aHashNode;
    aTempTable->mBucketCnt    = aBucketCnt;

    //----------------------------------------------------
    // Partitioned Hashing 에 쓰일 정보
    // - readyForSearch() 에서 정한다.
    // - Partition 개수의 초기 값은 Bucket 개수가 된다.
    //----------------------------------------------------

    aTempTable->mRadixBit     = 0;
    aTempTable->mPartitionCnt = aBucketCnt;
    aTempTable->mHistogram    = NULL;
    aTempTable->mSearchArray  = NULL;

    //----------------------------------------------------
    // 삽입을 위한 정보
    //----------------------------------------------------

    aTempTable->mHead         = NULL;
    aTempTable->mTail         = NULL;
    aTempTable->mRecordCnt    = 0;

    //----------------------------------------------------
    // 검색을 위한 정보
    //----------------------------------------------------

    aTempTable->mNextElem     = NULL;  
    aTempTable->mKey          = 0;
    aTempTable->mFilter       = NULL;
    aTempTable->mNextIdx      = 0;
    aTempTable->mBoundaryIdx  = 0;

    return IDE_SUCCESS;
}

IDE_RC qmcMemPartHash::clear( qmcdMemPartHashTemp * aTempTable )
{
/***********************************************************************
 *
 * Description :
 *     Memory Partitioned Hash Temp Table을 초기화한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    //----------------------------------------------------
    // 기본 정보 초기화
    //----------------------------------------------------

    aTempTable->mHead         = NULL;
    aTempTable->mTail         = NULL;

    // 최초 검색 시, 준비를 하도록 한다.
    aTempTable->mFlag &= ~QMC_MEM_PART_HASH_SEARCH_READY_MASK;
    aTempTable->mFlag |= QMC_MEM_PART_HASH_SEARCH_READY_FALSE;

    // mTemplate, mRecordNode, mHashNode, mBucketCnt는 초기화하지 않는다.

    //----------------------------------------------------
    // Partitioned Hashing 에 쓰일 정보 초기화
    //----------------------------------------------------

    aTempTable->mRadixBit     = 0;
    aTempTable->mPartitionCnt = aTempTable->mBucketCnt;
    aTempTable->mHistogram    = NULL;
    aTempTable->mSearchArray  = NULL;

    //----------------------------------------------------
    // 삽입 정보의 초기화
    //----------------------------------------------------

    aTempTable->mHead         = NULL;
    aTempTable->mTail         = NULL;
    aTempTable->mRecordCnt    = 0;

    //----------------------------------------------------
    // 검색 정보의 초기화
    //----------------------------------------------------
    
    aTempTable->mNextElem     = NULL;  
    aTempTable->mKey          = 0;
    aTempTable->mFilter       = NULL;
    aTempTable->mNextIdx      = 0;
    aTempTable->mBoundaryIdx  = 0;

    return IDE_SUCCESS;
}

IDE_RC qmcMemPartHash::clearHitFlag( qmcdMemPartHashTemp * aTempTable )
{
/***********************************************************************
 *
 * Description :
 *    Temp Table내의 모든 Record들의 Hit Flag을 초기화한다.
 *
 * Implementation :
 *    레코드가 존재할 때까지 순차 검색하여 Record들의 Hit Flag을 초기화
 *
 ***********************************************************************/

    qmcMemHashElement * sElement = NULL;

    // 최초 Record를 읽는다.
    IDE_TEST( qmcMemPartHash::getFirstSequence( aTempTable,
                                                (void**) & sElement )
              != IDE_SUCCESS );

    while ( sElement != NULL )
    {
        // Hit Flag을 초기화함.
        sElement->flag &= ~QMC_ROW_HIT_MASK;
        sElement->flag |= QMC_ROW_HIT_FALSE;

        IDE_TEST( qmcMemPartHash::getNextSequence( aTempTable,
                                                   (void**) & sElement )
                  != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qmcMemPartHash::insertAny( qmcdMemPartHashTemp  * aTempTable,
                                  UInt                   aHash,
                                  void                 * aElement,
                                  void                ** aOutElement )
{
/***********************************************************************
 *
 * Description :
 *    Record를 삽입한다.
 * 
 * Implementation :
 *    단일 List 가장 마지막에 새 Element를 삽입한다.
 *
 ***********************************************************************/

    qmcMemHashElement * sElement = aTempTable->mTail;

    if ( sElement == NULL )
    {
        // mTail이 NULL = 첫 Element를 삽입하는 경우
        // mHead = mTail = aElement
        aTempTable->mHead = (qmcMemHashElement*) aElement;
        aTempTable->mTail = (qmcMemHashElement*) aElement;
    }
    else
    {
        // 첫 Element 삽입이 아닌 경우
        // mTail->next = aElement; mTail = aElement
        aTempTable->mTail = (qmcMemHashElement *) aElement;
        sElement->next    = (qmcMemHashElement *) aElement;
    }

    // Hash Key 저장
    ((qmcMemHashElement*) aElement)->key = aHash;

    // 삽입이 성공했음을 표기
    *aOutElement = NULL;

    // Record 개수 증가
    aTempTable->mRecordCnt++;
    
    return IDE_SUCCESS;
}

IDE_RC qmcMemPartHash::getFirstSequence( qmcdMemPartHashTemp  * aTempTable,
                                         void                ** aElement )
{
/***********************************************************************
 *
 * Description :
 *    첫 순차 검색을 수행
 *
 * Implementation :
 *    단일 List에서 첫 Element를 반환한다.
 *
 ***********************************************************************/

    qmcMemHashElement * sElement = aTempTable->mHead;

    if ( sElement != NULL )
    {   
        //-----------------------------------
        // 다음 Record가 존재하는 경우
        //-----------------------------------
        *aElement = sElement;
        aTempTable->mNextElem = sElement->next;
    }   
    else
    {   
        *aElement = NULL;
        aTempTable->mNextElem = NULL;
    }   

    return IDE_SUCCESS;
}

IDE_RC qmcMemPartHash::getNextSequence( qmcdMemPartHashTemp  * aTempTable,
                                        void                ** aElement )
{
/***********************************************************************
 *
 * Description :
 *    다음 순차 검색을 수행
 *
 * Implementation :
 *    단일 List에서 다음 Element를 반환한다.
 *
 ***********************************************************************/


    qmcMemHashElement * sElement = aTempTable->mNextElem;

    if ( sElement != NULL )
    {   
        //-----------------------------------
        // 다음 Record가 존재하는 경우
        //-----------------------------------
        *aElement = sElement;
        aTempTable->mNextElem = sElement->next;
    }   
    else
    {   
        *aElement = NULL;
        aTempTable->mNextElem = NULL;
    }   

    return IDE_SUCCESS;
}

IDE_RC qmcMemPartHash::getFirstRange( qmcdMemPartHashTemp  * aTempTable,
                                      UInt                   aHash,
                                      qtcNode              * aFilter,
                                      void                ** aElement )
{
/***********************************************************************
 *
 * Description :
 *     주어진 Key 값과 Filter를 이용하여 Range 검색을 한다.
 *
 * Implementation :
 *    Key 값으로 Histogram에 접근해 Search Array의 시작 Item부터 탐색한다.
 *    일치하는 Key를 가진 Item이 가리키는 Element와 조건을 검사한다.
 *    조건이 참이라면, 해당 Element를 반환한다.
 *     
 ***********************************************************************/

    qmcMemHashItem * sSearchArray = NULL;
    UInt             sPartIdx     = 0;
    ULong            sTargetIdx   = 0;
    ULong            sBoundaryIdx = 0;
    idBool           sJudge       = ID_FALSE;

    //-------------------------------------------------
    // 최초 검색 시, Histogram / Search Array 준비
    //-------------------------------------------------

    // Record가 존재하지 않으면 readyForSearch()를 호출할 필요가 없다.
    IDE_TEST_CONT( aTempTable->mRecordCnt == 0, NO_ELEMENT );

    if ( ( aTempTable->mFlag & QMC_MEM_PART_HASH_SEARCH_READY_MASK )
            == QMC_MEM_PART_HASH_SEARCH_READY_FALSE )
    {
        IDE_TEST( readyForSearch( aTempTable ) != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    //-------------------------------------------------
    // 검색 준비
    //-------------------------------------------------

    // Histogram / SearchArray는 존재해야 한다.
    IDE_DASSERT( aTempTable->mHistogram   != NULL );
    IDE_DASSERT( aTempTable->mSearchArray != NULL );

    // 다음 Range 검색을 위해 Key값과 Filter를 저장
    aTempTable->mKey    = aHash;
    aTempTable->mFilter = aFilter;

    // Hash Key를 통해, Histogram에 접근할 Index 획득
    sPartIdx     = ( aHash & ( ( 1 << aTempTable->mRadixBit ) - 1 ) );

    // Histogram이 가리키는 현재 Partition / 다음 Partition의 시작 Item Index 획득
    sTargetIdx   = aTempTable->mHistogram[sPartIdx];
    sBoundaryIdx = aTempTable->mHistogram[sPartIdx+1];

    // Search Array 가져오기
    sSearchArray = aTempTable->mSearchArray;

    //-------------------------------------------------
    // 검색 시작
    //-------------------------------------------------

    while ( sTargetIdx < sBoundaryIdx ) 
    {
        if ( sSearchArray[sTargetIdx].mKey == aHash )
        {
            // 동일한 Key값을 갖는 경우, Filter조건도 검사.
            IDE_TEST( qmcMemPartHash::judgeFilter( aTempTable,
                                                   sSearchArray[sTargetIdx].mPtr,
                                                   & sJudge )
                      != IDE_SUCCESS );

            if ( sJudge == ID_TRUE )
            {
                // To Fix PR-8645
                // Hash Key 값을 검사하기 위한 Filter에서 Access값을
                // 증가하였고, 이 후 Execution Node에서 이를 다시 증가시킴.
                // 따라서, 마지막 한번의 검사에 대해서는 access회수를 감소시킴
                aTempTable->mRecordNode->dstTuple->modify--;
                break;
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            // Nothing to do.
        }
        
        sTargetIdx++;
    }

    IDE_EXCEPTION_CONT( NO_ELEMENT );

    if ( sJudge == ID_TRUE )
    {
        // 검색 성공
        *aElement = sSearchArray[sTargetIdx].mPtr;

        // 다음 검색을 위한 정보 저장
        aTempTable->mNextIdx     = sTargetIdx + 1;
        aTempTable->mBoundaryIdx = sBoundaryIdx;
    }
    else
    {
        // 검색 실패
        *aElement = NULL;

        // 다음 검색을 하지 못하도록 정보 초기화
        aTempTable->mNextIdx     = 0;
        aTempTable->mBoundaryIdx = 0;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmcMemPartHash::getNextRange( qmcdMemPartHashTemp  * aTempTable,
                                     void                ** aElement )
{
/***********************************************************************
 *
 * Description :
 *    다음 Range 검색을 수행한다.
 *
 * Implementation :
 *    이미 저장된 정보를 이용하여, Search Array의 다음 Item부터 탐색한다.
 *    일치하는 Key를 가진 Item이 가리키는 Element와 조건을 검사한다.
 *    조건이 참이라면, 해당 Element를 반환한다.
 *
 ***********************************************************************/

    qmcMemHashItem * sSearchArray = NULL;
    UInt             sKey         = 0;
    ULong            sTargetIdx   = 0;
    ULong            sBoundaryIdx = 0;
    idBool           sJudge       = ID_FALSE;

    //-------------------------------------------------
    // 검색 준비
    //-------------------------------------------------

    // Search Array 가져오기
    sSearchArray = aTempTable->mSearchArray;

    // Key값 가져오기
    sKey         = aTempTable->mKey;

    // 현재 Item Index / 다음 Partition 시작 Item Index 가져오기
    sTargetIdx   = aTempTable->mNextIdx;
    sBoundaryIdx = aTempTable->mBoundaryIdx;

    //-------------------------------------------------
    // 검색 시작
    //-------------------------------------------------

    while ( sTargetIdx < sBoundaryIdx ) 
    {
        if ( sSearchArray[sTargetIdx].mKey == sKey )
        {
            // 동일한 Key값을 갖는 경우, Filter조건도 검사.
            IDE_TEST( qmcMemPartHash::judgeFilter( aTempTable,
                                                   sSearchArray[sTargetIdx].mPtr,
                                                   & sJudge )
                      != IDE_SUCCESS );

            if ( sJudge == ID_TRUE )
            {
                // To Fix PR-8645
                // Hash Key 값을 검사하기 위한 Filter에서 Access값을
                // 증가하였고, 이 후 Execution Node에서 이를 다시 증가시킴.
                // 따라서, 마지막 한번의 검사에 대해서는 access회수를 감소시킴
                aTempTable->mRecordNode->dstTuple->modify--;
                break;
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            // Nothing to do.
        }
        
        sTargetIdx++;
    }

    if ( sJudge == ID_TRUE )
    {
        // 검색 성공
        *aElement = sSearchArray[sTargetIdx].mPtr;

        // 다음 검색을 위한 정보 저장
        aTempTable->mNextIdx     = sTargetIdx + 1;
        aTempTable->mBoundaryIdx = sBoundaryIdx;
    }
    else
    {
        // 검색 실패
        *aElement = NULL;

        // 다음 검색을 하지 못하도록 정보 초기화
        aTempTable->mNextIdx     = 0;
        aTempTable->mBoundaryIdx = 0;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmcMemPartHash::getFirstHit( qmcdMemPartHashTemp  * aTempTable,
                                    void                ** aElement )
{
/***********************************************************************
 *
 * Description :
 *    순차 검색하여 Hit된 Record를 리턴한다.
 *
 * Implementation :
 *    Hit 된 record를 찾을 때까지 반복적으로 수행한다.
 *
 ***********************************************************************/

    qmcMemHashElement * sElement = NULL;
    
    IDE_TEST( getFirstSequence( aTempTable, aElement ) != IDE_SUCCESS );

    while ( *aElement != NULL )
    {
        sElement = (qmcMemHashElement*) *aElement;

        if ( ( sElement->flag & QMC_ROW_HIT_MASK ) == QMC_ROW_HIT_TRUE )
        {
            // 원하는 Record를 찾음
            break;
        }
        else
        {
            // 다음 record를 검색
            IDE_TEST( getNextSequence( aTempTable, aElement ) != IDE_SUCCESS );
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmcMemPartHash::getNextHit( qmcdMemPartHashTemp  * aTempTable,
                                   void                ** aElement )
{
/***********************************************************************
 *
 * Description :
 *    순차 검색하여 Hit된 Record를 리턴한다.
 *
 * Implementation :
 *    Hit 된 record를 찾을 때까지 반복적으로 수행한다.
 *
 ***********************************************************************/

    qmcMemHashElement * sElement = NULL;
    
    IDE_TEST( getNextSequence( aTempTable, aElement ) != IDE_SUCCESS );

    while ( *aElement != NULL )
    {
        sElement = (qmcMemHashElement*) *aElement;
        if ( ( sElement->flag & QMC_ROW_HIT_MASK ) == QMC_ROW_HIT_TRUE )
        {
            // 원하는 Record를 찾음
            break;
        }
        else
        {
            // 다음 record를 검색
            IDE_TEST( getNextSequence( aTempTable, aElement ) != IDE_SUCCESS );
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmcMemPartHash::getFirstNonHit( qmcdMemPartHashTemp  * aTempTable,
                                       void                ** aElement )
{
/***********************************************************************
 *
 * Description :
 *    순차 검색하여 NonHit된 Record를 리턴한다.
 *
 * Implementation :
 *    NonHit 된 record를 찾을 때까지 반복적으로 수행한다.
 *
 ***********************************************************************/

    qmcMemHashElement * sElement = NULL;
    
    IDE_TEST( getFirstSequence( aTempTable, aElement ) != IDE_SUCCESS );

    while ( *aElement != NULL )
    {
        sElement = (qmcMemHashElement*) *aElement;
        if ( ( sElement->flag & QMC_ROW_HIT_MASK ) == QMC_ROW_HIT_FALSE )
        {
            // 원하는 Record를 찾음
            break;
        }
        else
        {
            // 다음 record를 검색
            IDE_TEST( getNextSequence( aTempTable, aElement ) != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmcMemPartHash::getNextNonHit( qmcdMemPartHashTemp  * aTempTable,
                                      void                ** aElement )
{
/***********************************************************************
 *
 * Description :
 *    순차 검색하여 NonHit된 Record를 리턴한다.
 *
 * Implementation :
 *    NonHit 된 record를 찾을 때까지 반복적으로 수행한다.
 *
 ***********************************************************************/

    qmcMemHashElement * sElement = NULL;
    
    IDE_TEST( getNextSequence( aTempTable, aElement ) != IDE_SUCCESS );

    while ( *aElement != NULL )
    {
        sElement = (qmcMemHashElement*) *aElement;
        if ( ( sElement->flag & QMC_ROW_HIT_MASK ) == QMC_ROW_HIT_FALSE )
        {
            // 원하는 Record를 찾음
            break;
        }
        else
        {
            // 다음 record를 검색
            IDE_TEST( getNextSequence( aTempTable, aElement ) != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmcMemPartHash::getDisplayInfo( qmcdMemPartHashTemp * aTempTable,
                                       SLong               * aRecordCnt,
                                       UInt                * aBucketCnt )
{
/***********************************************************************
 *
 * Description :
 *    삽입되어 있는 Record 개수, Partition 개수를 반환한다.
 *
 * Implementation :
 *    외부에서는 DISTINCT Hash Temp Table과 구별할 수 없으므로
 *    'Bucket 개수' 라는 개념에 'Partition 개수'를 반환한다.
 *
 ***********************************************************************/

    *aRecordCnt = aTempTable->mRecordCnt;
    *aBucketCnt = aTempTable->mPartitionCnt;

    return IDE_SUCCESS;
}

IDE_RC qmcMemPartHash::judgeFilter( qmcdMemPartHashTemp * aTempTable,
                                    void                * aElem,
                                    idBool              * aResult )
{
/***********************************************************************
 *
 * Description :
 *     주어진 Record가 Filter 조건을 만족하는 지 검사.
 *
 * Implementation :
 *     Filter에 적용할 수 있도록 해당 Hash Column들을 복원시키고
 *     Filter를 적용한다.  예를 들어 memory column의 경우 해당 record의
 *     pointer가 저장되어 있어 이 pointer를 tuple set에 복원시켜야 한다.
 *
 ***********************************************************************/

    qmdMtrNode * sNode = NULL;
    
    //------------------------------------
    // Hashing 대상 column들을 복원시킴
    //------------------------------------

    // To Fix PR-8024
    // 현재 선택된 Row를 Tuple Set에 등록시켜두고 처리해야 함
    aTempTable->mHashNode->dstTuple->row = aElem;
    
    for ( sNode = aTempTable->mHashNode; 
          sNode != NULL; 
          sNode = sNode->next )
    {
        IDE_TEST( sNode->func.setTuple( aTempTable->mTemplate, sNode, aElem )
                  != IDE_SUCCESS );
    }

    //------------------------------------
    // Filter 수행 결과를 획득
    //------------------------------------
    
    IDE_TEST( qtc::judge( aResult, 
                          aTempTable->mFilter, 
                          aTempTable->mTemplate ) != IDE_SUCCESS );

    // To Fix PR-8645
    // Hash Key값에 대한 검사는 실제 Record를 접근하는 검사이며,
    // Access 회수를 증가시켜야 Hash 효과를 측정할 수 있다.
    aTempTable->mRecordNode->dstTuple->modify++;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

UInt qmcMemPartHash::calcRadixBit( qmcdMemPartHashTemp * aTempTable )
{
/***********************************************************************
 *
 * Description :
 *     Radix Bit Partitioning을 위한 Radix Bit를 계산한다.
 *     Radix Bit를 2의 지수로 둔 값이 Partition의 개수가 된다.
 *
 * Implementation :
 *     다음 중 한 가지 방법으로 최적의 Partition 개수를 먼저 구한다.
 *
 *     - AUTO_BUCKETCNT_DISABLE 가 0 로 설정되어 있는 경우 :
 *       ( 실제 Record 개수 ) / AVG_RECORD_COUNT
 *     - AUTO_BUCKETCNT_DISABLE 가 1 로 설정되어 있는 경우 :
 *       ( 초기에 입력된 Bucket 개수 )
 *
 *     그 다음, 최소/최대값을 보정한 뒤 아래 식으로 Radix Bit를 구한다.
 *
 *       RadixBit = ceil( Log(Optimal Partition Count) )
 *       ( Log의 밑은 2 )
 *
 ***********************************************************************/

    ULong sOptPartCnt = 0;
    SInt  sRBit       = 0;

    IDE_DASSERT( aTempTable->mRecordCnt > 0 );

    // 1. Optimal Partition Count를 구한다.
    if ( aTempTable->mTemplate->memHashTempManualBucketCnt == ID_TRUE )
    {
        // [ Estimation Mode ]
        // 입력된 Bucket Count를 기준으로 한다.
        sOptPartCnt = aTempTable->mBucketCnt;
    }
    else
    {
        // [ Calculation Mode ]
        // 기존 Bucket Count 계산 방식을 따른다. (qmgJoin::getBucketCntNTmpTblCnt)
        // 단, 예측 Record 개수가 아닌 실제 Record 개수에 기반하며
        // Partition 당 평균 Element 개수로 지정된 상수로 나눠 계산한다.
        sOptPartCnt = 
            aTempTable->mRecordCnt / QMC_MEM_PART_HASH_AVG_RECORD_COUNT; 
    }

    // 2. Optimal Partition Count의 최소/최대값 보정
    if ( sOptPartCnt < QMC_MEM_PART_HASH_MIN_PART_COUNT )
    {
        sOptPartCnt = QMC_MEM_PART_HASH_MIN_PART_COUNT;
    }
    else if ( sOptPartCnt > QMC_MEM_PART_HASH_MAX_PART_COUNT )
    {
        sOptPartCnt = QMC_MEM_PART_HASH_MAX_PART_COUNT;
    }
    else
    {
        // Nothing to do.
    }

    // 3. Optimal Partition Count보다 크면서 가장 가까운
    //    Partition Count를 구할 수 있는 Radix Bit를 계산한다.
    sRBit = ceilLog2( sOptPartCnt );

    return sRBit;
}

IDE_RC qmcMemPartHash::readyForSearch( qmcdMemPartHashTemp * aTempTable )
{
/***********************************************************************
 *
 * Description :
 *      최초 범위 검색 전에, 히스토그램과 검색 배열을 준비한다.
 *
 * Implementation :
 *
 *  (1) 계산을 하거나, 입력된 Bucket 개수를 통해 Partition 개수를 정한다.
 *
 *  (2) Partition 개수, Record 개수에 맞는
 *      히스토그램, 검색 배열을 할당한다.
 *
 *  (3) 히스토그램에, 각 Partition에 속한 Element 개수를 센다.
 *    - 각 Element Hash Key에 Radix Bit Masking을 해서 Partition 인덱스를 구한다.
 *    - Partition 인덱스가 가리키는 히스토그램 값을 1 증가시킨다.
 *
 *  (4) 히스토그램에서, i번째 값은 0 ~ (i-1)번째 값의 합계로 설정한다.
 *      최종 결과는, 검색 배열에서 각 Partition의 시작 인덱스가 된다.
 *
 *  (5) Partition 개수에 따라 한 번에 검색 배열로 삽입할 것인지,
 *      두 번에 나눠 삽입할 것인지 결정한 뒤, 해당 함수를 수행한다.
 *      (함수는 fanoutSingle / fanoutDouble 로 구별된다.)
 *
 ***********************************************************************/

    qmcMemHashElement * sElement        = NULL;
    ULong             * sHistogram      = NULL;
    ULong               sPartitionCnt   = 0;
    UInt                sPartIdx        = 0;
    UInt                sPartMask       = 0;
    UInt                sSingleRadixBit = 0;
    ULong               sHistoTemp      = 0;
    ULong               sHistoSum       = 0;
    UInt                i               = 0;


    //---------------------------------------------------------------------
    // Radix Bit / Partition Count 계산
    //---------------------------------------------------------------------

    IDE_DASSERT( aTempTable->mRecordCnt > 0 );

    aTempTable->mRadixBit     = calcRadixBit( aTempTable );
    aTempTable->mPartitionCnt = (UInt)( 1 << aTempTable->mRadixBit );

    //---------------------------------------------------------------------
    // Histogram 할당
    //---------------------------------------------------------------------

    IDU_FIT_POINT( "qmcMemPartHash::readyForSearch::malloc1",
                   idERR_ABORT_InsufficientMemory );
    IDE_TEST( aTempTable->mMemory->cralloc( ( aTempTable->mPartitionCnt + 1 ) * ID_SIZEOF(ULong),
                                            (void**) & sHistogram )
              != IDE_SUCCESS );
 
    // Histogram의 마지막은 Record Count여야 한다.
    sHistogram[aTempTable->mPartitionCnt] = aTempTable->mRecordCnt;

    //---------------------------------------------------------------------
    // Histogram에, 각 Partition의 Element 개수 수집
    //
    //  단일 List를 순회하면서 Hash Key에 Radix Bit Masking을 한다.
    //  Masking 결과가 가리키는 Histogram 값을 1 증가시킨다.
    //---------------------------------------------------------------------

    sElement  = aTempTable->mHead;
    sPartMask = ( 1 << aTempTable->mRadixBit ) - 1;

    while ( sElement != NULL )
    {
        sPartIdx = sElement->key & sPartMask;
        sHistogram[sPartIdx]++;
        sElement = sElement->next;
    }

    //---------------------------------------------------------------------
    // Histogram에, 누적 합계 수행
    //
    //  처음에는, 각 Partition에 속한 Element의 개수를 세어 저장했지만
    //  직전 Partition의 Element 개수 합을 통해 시작 위치를 저장하도록 한다.
    //
    //           Histogram             Histogram 
    //           ----------            ----------
    //        P1 |    3   |         P1 |    0   | = 0
    //           ----------            ----------
    //        P2 |    1   |   =>    P2 |    3   | = 0+3
    //           ----------            ----------
    //        P3 |    5   |         P3 |    4   | = 0+3+1
    //           ----------            ----------
    //           |  ...   |            |  ...   |
    //           ----------            ----------
    //           | RecCnt |            | RecCnt |
    //           ----------            ----------
    //
    //---------------------------------------------------------------------

    sPartitionCnt = aTempTable->mPartitionCnt;

    for ( i = 0; i < sPartitionCnt; i++ )
    {
        sHistoTemp = sHistogram[i];
        sHistogram[i] = sHistoSum;
        sHistoSum += sHistoTemp;
    }

    // 완성된 Histogram을 Temp Table에 설정
    aTempTable->mHistogram = sHistogram;

    //---------------------------------------------------------------------
    // Search Array 할당
    //---------------------------------------------------------------------

    IDU_FIT_POINT( "qmcMemPartHash::readyForSearch::malloc2",
                   idERR_ABORT_InsufficientMemory );
    IDE_TEST( aTempTable->mMemory->alloc( aTempTable->mRecordCnt * ID_SIZEOF(qmcMemHashItem),
                                          (void**) & aTempTable->mSearchArray )
              != IDE_SUCCESS );
    
    //---------------------------------------------------------------------
    // Single Fanout 기준 Radix Bit 계산
    //
    // (1) TLB Entry Count * Page Size * 확장 상수 = TLB에 맞는 메모리 영역
    // (2) 여기에 Histogram Item (ULong) 크기만큼 나눈 값 
    //     = Single Fanout의 이상적인 Partition 개수
    // (3) 여기에 ceil(log2())를 하면 Single Fanout 기준 Radix Bit가 계산된다.
    //---------------------------------------------------------------------

    sSingleRadixBit = ceilLog2( ( QMC_MEM_PART_HASH_TLB_ENTRY_MULTIPLIER * 
                                  aTempTable->mTemplate->memHashTempTlbCount * 
                                  QMC_MEM_PART_HASH_PAGE_SIZE ) / ID_SIZEOF(ULong) );

    //---------------------------------------------------------------------
    // Partitioning 수행
    //---------------------------------------------------------------------

    if ( QCU_FORCE_HSJN_MEM_TEMP_FANOUT_MODE == 0 )
    {
        // 0 : Single / Double Fanout 중 선택
        if ( aTempTable->mRadixBit <= sSingleRadixBit )
        {
            IDE_TEST( fanoutSingle( aTempTable ) != IDE_SUCCESS );  
        }
        else
        {
            IDE_TEST( fanoutDouble( aTempTable ) != IDE_SUCCESS );  
        }
    }
    else if ( QCU_FORCE_HSJN_MEM_TEMP_FANOUT_MODE == 1 )
    {
        // 1 : Single Fanout 만 선택
        IDE_TEST( fanoutSingle( aTempTable ) != IDE_SUCCESS );  
    }
    else
    {
        // 2 : Double Fanout 만 선택
        IDE_DASSERT( QCU_FORCE_HSJN_MEM_TEMP_FANOUT_MODE == 2 );
        IDE_TEST( fanoutDouble( aTempTable ) != IDE_SUCCESS );  
    }

    //---------------------------------------------------------------------
    // 마무리 : Flag 전환
    //---------------------------------------------------------------------

    aTempTable->mFlag &= ~QMC_MEM_PART_HASH_SEARCH_READY_MASK;
    aTempTable->mFlag |= QMC_MEM_PART_HASH_SEARCH_READY_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmcMemPartHash::fanoutSingle( qmcdMemPartHashTemp * aTempTable )
{
/***********************************************************************
 *
 * Description :
 *     한 번에 Search Array에 Item을 삽입한다.
 *
 *                         TempHist.            SearchArr. 
 *      LIST                 -----                -----
 *     ------                |---|                |---|
 *     |    |                |---|                |---|
 *     |    |                |---|                |---|
 *     |    | -> PartIdx ->  |---| -> ArrayIdx -> |---|
 *     |    |                |---|                |---|
 *     |    |                |---|                |---|
 *     ------                |---|                |---|
 *                           -----                -----
 *
 *     각 Partition에서 삽입 가능한 위치를 저장할 '임시 Histogram'이 필요하다.
 *     삽입이 이루어질 때 마다 해당 값이 +1 증가한다.
 *     기존 Histogram은 시작 위치 정보를 저장해야 하므로 변경할 수 없기 때문에
 *     임시 공간을 사용하는 것이다.
 *
 * Implementation :
 *
 *   (1) '임시 Histogram'을 할당한다.
 *   (2) '임시 Histogram'에, Histogram 내용을 복사한다.
 *   (3) List에 있는 각 Element마다, 다음을 수행한다.
 *     - Hash Key를 Radix Bit Masking 해서 Partition 인덱스를 구한다.
 *     - Partition 인덱스로 '임시 Histogram'에 접근해 Array 인덱스를 구한다.
 *       이 때, 해당하는 '임시 Histogram 값'을 +1 증가시킨다.
 *     - Array 인덱스로 Search Array에 Element 정보를 저장한다.  
 *
 ***********************************************************************/

    qmcMemHashElement  * sElement      = NULL;
    qmcMemHashItem     * sSearchArray  = NULL;
    ULong              * sHistogram    = NULL;
    ULong              * sInsertHist   = NULL;
    ULong                sArrayIdx     = 0;
    UInt                 sPartitionCnt = 0; 
    UInt                 sMask         = 0; 
    UInt                 sPartitionIdx = 0;
    UInt                 sHashKey      = 0;
    UInt                 sState        = 0;

    //--------------------------------------------------------------------
    // 기본 정보 설정
    //--------------------------------------------------------------------

    sPartitionCnt = aTempTable->mPartitionCnt; 
    sHistogram    = aTempTable->mHistogram;
    sSearchArray  = aTempTable->mSearchArray;

    //--------------------------------------------------------------------
    // 임시 Histogram 할당
    //--------------------------------------------------------------------

    // 원래 Histogram과는 달리, 마지막에 Record 개수를 담을 필요가 없다.
    IDU_FIT_POINT( "qmcMemPartHash::fanoutSingle::malloc",
                   idERR_ABORT_InsufficientMemory );
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_QMC,
                                 sPartitionCnt * ID_SIZEOF(ULong),
                                 (void**) & sInsertHist )
              != IDE_SUCCESS );
    sState = 1;

    //--------------------------------------------------------------------
    // 임시 Histogram에 Histogram 값 복사
    //--------------------------------------------------------------------

    idlOS::memcpy( sInsertHist,
                   sHistogram,
                   sPartitionCnt * ID_SIZEOF(ULong) ); 

    //--------------------------------------------------------------------
    // 순차 검색하면서, Search Array에 Item 삽입
    //--------------------------------------------------------------------

    // 첫 Element와 Mask 준비
    sElement = aTempTable->mHead;
    sMask    = ( 1 << aTempTable->mRadixBit ) - 1;

    while ( sElement != NULL )
    {
        // Radix-bit Masking으로 Partition Index 계산
        sHashKey      = sElement->key;
        sPartitionIdx = sHashKey & sMask;

        IDE_DASSERT( sPartitionIdx < sPartitionCnt );

        // 임시 Histogram 값으로 Search Array Index 계산
        sArrayIdx = sInsertHist[sPartitionIdx];
        sInsertHist[sPartitionIdx]++; // 다음 지점을 가리키도록 미리 증가

        // Search Array Index는 Record 개수보다 작아야 하며,
        // 다음 Partition의 경계를 넘는 경우도 없어야 한다.
        IDE_DASSERT( sArrayIdx < (UInt)aTempTable->mRecordCnt );
        IDE_DASSERT( sArrayIdx < sHistogram[sPartitionIdx+1] );

        // Search Array에 정보 삽입
        sSearchArray[sArrayIdx].mKey = sHashKey;
        sSearchArray[sArrayIdx].mPtr = sElement;

        // 다음 Element 탐색
        sElement = sElement->next;
    }

    // 해제
    sState = 0;
    IDE_TEST( iduMemMgr::free( sInsertHist ) != IDE_SUCCESS );
    sInsertHist = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    // BUG-41824
    switch ( sState )
    {
        case 1:
            (void)iduMemMgr::free( sInsertHist );
            sInsertHist = NULL;
            /* fall through */
        case 0:
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}

IDE_RC qmcMemPartHash::fanoutDouble( qmcdMemPartHashTemp * aTempTable )
{
/***********************************************************************
 *
 * Description :
 *     두 번에 걸쳐 Search Array에 Item을 삽입한다.
 *
 *                          TempHist1            TempArr. 
 *      LIST                  -----                -----
 *     ------                 |   |                |   |
 *     |    |                 |---|                |---|
 *     |    |                 |   |                |   |
 *     |    | -> Part1Idx ->  |---| -> ArrayIdx -> |---|
 *     |    |     (RBit1)     |   |                |   |
 *     |    |                 |---|                |---|
 *     ------                 |   |                |   |
 *                            -----                -----
 *
 *     Temp Array의 각 Partition마다,
 *
 *                        TempHist2            SearchArr. 
 *     |---|                -----                |---|
 *     |   | -> Part2Idx -> |---| -> ArrayIdx -> |---|
 *     |---|    (RBit2)     -----                |---|
 *
 *
 *     Fanout 결과는,
 *
 *               SearchArr. 
 *      LIST       -----
 *     ------      |---|
 *     |    |      |---|
 *     |    |      |---|
 *     |    |  ->  |---|
 *     |    |      |---|
 *     |    |      |---|
 *     ------      |---|
 *                 -----
 *
 *     첫 번째로 나눌 Radix Bit (RBit1)는, 전체 Radix Bit의 1/2로 정한다.
 *     두 번째로 나눌 Radix Bit (RBit2)는, 전체 Radix Bit에서 rB1을 뺀 값이다.
 *
 *     RBit1은 전체 Radix Bit에서 상위 Bit를 나타내며,
 *     RBit2는 전체 Radix Bit에서 하위 Bit를 나타낸다.
 *
 *     (e.g.) 01010100110101 = 0101010 0110101
 *            |  RadixBit  |   | RB1 | | RB2 |
 *
 *     즉, rB1 Masking으로 나타내는 Partition 1개 안에서
 *         rB2 Masking으로 나타내는 Partition 여러 개로 다시 쪼개질 수 있다.
 *
 *     두 번에 걸쳐 삽입되므로, 같은 크기의 임시 Search Array가 더 필요하다. (TempArr.)
 *     하지만 임시 Search Array는 검색 과정에서 쓰지 않고, 최종 Search Array만 사용한다.
 *
 *     메모리 낭비가 있지만, 다수의 Partition으로 나눠야 할 때엔
 *     Single Fanout 대비 속도가 빠르다. (TLB Miss 최소화)
 *
 * Implementation :
 *
 *    RBit1, RBit2를 구한 다음, 각각에 대해 Single Fanout을 2회 수행한다.
 *    
 *    두 번째 Fanout 과정은, 첫 번째 Fanout에서 나눠진 각 Partition마다 반복수행 한다.
 *    그래서, 두 번째 임시 Histogram은 재사용한다.
 *
 ***********************************************************************/

    qmcMemHashElement  * sElement           = NULL;
    qmcMemHashItem     * sSearchArray       = NULL;
    qmcMemHashItem     * sTempSearchArray   = NULL; 
    ULong              * sHistogram         = NULL;
    ULong              * sFirstInsertHist   = NULL;
    ULong              * sLastInsertHist    = NULL;
    ULong                sRecCount          = 0;
    ULong                sArrayIdx          = 0;
    ULong                sStartArrIdx       = 0;
    ULong                sEndArrIdx         = 0;
    UInt                 sFirstRBit         = 0;
    UInt                 sFirstPartCnt      = 0;
    UInt                 sLastRBit          = 0;
    UInt                 sLastPartCnt       = 0;
    UInt                 sMask              = 0; 
    UInt                 sPartitionIdx      = 0;
    UInt                 sHashKey           = 0; 
    UInt                 sState             = 0; 
    UInt                 i                  = 0;
    UInt                 j                  = 0;

    //--------------------------------------------------------------------
    // 기본 정보 설정
    //--------------------------------------------------------------------

    sRecCount    = aTempTable->mRecordCnt; 
    sHistogram   = aTempTable->mHistogram;
    sSearchArray = aTempTable->mSearchArray;

    //--------------------------------------------------------------------
    // 첫 Partition / 마지막 Partition 개수 계산
    //--------------------------------------------------------------------

    // 첫 Radix Bit : 1/2
    sFirstRBit    = aTempTable->mRadixBit / 2;
    sFirstPartCnt = (UInt)( 1 << sFirstRBit );

    // 마지막 Radix Bit : (전체 rBit) - (첫 rBit)
    sLastRBit     = aTempTable->mRadixBit - sFirstRBit;
    sLastPartCnt  = (UInt)( 1 << sLastRBit );

    //--------------------------------------------------------------------
    // 첫 / 마지막 Histogram + 임시 Search Array 할당
    //--------------------------------------------------------------------

    // 첫 Histogram
    IDU_FIT_POINT( "qmcMemPartHash::fanoutDouble::malloc1",
                   idERR_ABORT_InsufficientMemory );
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_QMC,
                                 ( sFirstPartCnt + 1 ) * ID_SIZEOF(ULong),
                                 (void**) & sFirstInsertHist )
              != IDE_SUCCESS );
    sState = 1;

    // 처음 Histogram은 마지막 Record 개수 값을 가지고 있어야 한다.
    sFirstInsertHist[sFirstPartCnt] = sRecCount;

    // 임시 Search Array
    IDU_FIT_POINT( "qmcMemPartHash::fanoutDouble::malloc2",
                   idERR_ABORT_InsufficientMemory );
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_QMC,
                                 sRecCount * ID_SIZEOF(qmcMemHashItem),
                                 (void**) & sTempSearchArray )
              != IDE_SUCCESS );
    sState = 2;
 
    // 마지막 Histogram
    IDU_FIT_POINT( "qmcMemPartHash::fanoutDouble::malloc3",
                   idERR_ABORT_InsufficientMemory );
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_QMC,
                                 sLastPartCnt * ID_SIZEOF(ULong),
                                 (void**) & sLastInsertHist )
              != IDE_SUCCESS );
    sState = 3;

    //--------------------------------------------------------------------
    // 첫 Histogram에 Histogram 값 복사
    //--------------------------------------------------------------------

    // 기존 Histogram에서 Last Partition 개수만큼 건너뛰면서 가져온다
    for ( i = 0; i < sFirstPartCnt; i++ )
    {
        sFirstInsertHist[i] = sHistogram[i*sLastPartCnt];
    }

    //--------------------------------------------------------------------
    // 임시 Search Array에 삽입
    //--------------------------------------------------------------------

    // 첫 Element와 Mask 준비
    sElement = aTempTable->mHead;
    sMask    = ( 1 << aTempTable->mRadixBit ) - 1;

    while ( sElement != NULL )
    {
        // 상위 Bit에 대한 Radix-bit Masking으로 Partition Index 계산
        sHashKey = sElement->key;
        sPartitionIdx = ( sHashKey & sMask ) >> sLastRBit;

        IDE_DASSERT( sPartitionIdx < sFirstPartCnt );

        // 첫 Histogram 값으로 임시 Search Array Index 계산
        sArrayIdx = sFirstInsertHist[sPartitionIdx];
        sFirstInsertHist[sPartitionIdx]++; // 다음 지점을 가리키도록 미리 증가

        // 임시 Search Array Index는 Record 개수보다 작아야 하며,
        // 다음 Partition의 경계를 넘는 경우도 없어야 한다.
        IDE_DASSERT( sArrayIdx < sRecCount );
        IDE_DASSERT( sArrayIdx < sHistogram[(sPartitionIdx+1)*sLastPartCnt] );

        // Search Array에 정보 삽입
        sTempSearchArray[sArrayIdx].mKey = sHashKey;
        sTempSearchArray[sArrayIdx].mPtr = sElement;

        // 다음 Element 검색
        sElement = sElement->next;
    }


    // 첫 Histogram으로 쪼갠 각 Partition 마다 진행
    // 하위 RBit에 대해서만 Masking을 한다.
    sMask = ( 1 << sLastRBit ) - 1;

    for ( i = 0 ; i < sFirstPartCnt; i++ )
    {
        //--------------------------------------------
        // 첫 Search Array의 i번째 Partition 정보 설정
        // - i번째 Partition의 Search Array 시작 위치
        // - i번째 Partition의 Search Array 끝 위치
        //--------------------------------------------
        // 첫 Histogram의 값으로 시작/끝 위치를 정하는데
        // i / i+1이 아닌 i-1 / i를 사용한다.
        // 첫 Fanout 과정에서 이미 첫 Histogram 값들이 
        // 다음 Partition 시작 위치까지 증가되었기 때문이다.
        // Link : http://nok.altibase.com/x/u9ACAg
        //--------------------------------------------

        if ( i == 0 )
        {
            sStartArrIdx = 0;
        }
        else
        {
            sStartArrIdx = sFirstInsertHist[i-1];
        }

        sEndArrIdx = sFirstInsertHist[i];

        //--------------------------------------------------------------------
        // i번째 Partition에 해당하는 Histogram 값들을 마지막 Histogram에 복사
        //--------------------------------------------------------------------

        idlOS::memcpy( sLastInsertHist,
                       & sHistogram[sLastPartCnt * i],
                       ID_SIZEOF(ULong) * sLastPartCnt );

        //--------------------------------------------------------------------
        // i번째 Partition의 내용을 최종 Search Array에 삽입
        //--------------------------------------------------------------------

        for ( j = sStartArrIdx; j < sEndArrIdx; j++ )
        {
            // Radix-bit Masking으로 Partition Index 계산
            sPartitionIdx = sTempSearchArray[j].mKey & sMask;

            IDE_DASSERT( sPartitionIdx < sLastPartCnt );

            // 마지막 Histogram 값으로 최종 Search Array Index 계산
            sArrayIdx = sLastInsertHist[sPartitionIdx];
            sLastInsertHist[sPartitionIdx]++; // 다음 지점을 가리키도록 미리 증가

            // 최종 Search Array Index는 Record 개수보다 작아야 하며,
            // 다음 Partition의 경계를 넘는 경우도 없어야 한다.
            IDE_DASSERT( sArrayIdx < sRecCount );
            IDE_DASSERT( sArrayIdx < sHistogram[(sLastPartCnt*i)+sPartitionIdx+1] );

            // 최종 Search Array에 정보 삽입
            sSearchArray[sArrayIdx].mKey = sTempSearchArray[j].mKey;
            sSearchArray[sArrayIdx].mPtr = sTempSearchArray[j].mPtr;
        }
    }

    // 해제
    sState = 2;
    IDE_TEST( iduMemMgr::free( sLastInsertHist  ) != IDE_SUCCESS );
    sLastInsertHist  = NULL;

    sState = 1;
    IDE_TEST( iduMemMgr::free( sTempSearchArray ) != IDE_SUCCESS );
    sTempSearchArray = NULL;

    sState = 0;
    IDE_TEST( iduMemMgr::free( sFirstInsertHist ) != IDE_SUCCESS );
    sFirstInsertHist = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    // BUG-41824
    switch ( sState )
    {
        case 3:
            (void)iduMemMgr::free( sLastInsertHist  );
            sLastInsertHist = NULL;
            /* fall through */
        case 2:
            (void)iduMemMgr::free( sTempSearchArray );
            sTempSearchArray = NULL;
            /* fall through */
        case 1:
            (void)iduMemMgr::free( sFirstInsertHist  );
            sFirstInsertHist = NULL;
            /* fall through */
        case 0:
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}

/*
 * 2^n >= aNumber 를 만족하는 가장 작은 n 을 구한다
 */
UInt qmcMemPartHash::ceilLog2( ULong aNumber )
{
    ULong sDummyValue;
    UInt  sRet;

    for ( sDummyValue = 1, sRet = 0; sDummyValue < aNumber; sRet++ )
    {
        sDummyValue <<= 1;
    }

    return sRet;
}
