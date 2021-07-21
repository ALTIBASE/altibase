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
 * $Id: qmcMemHashTempTable.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *     Memory Hash Temp Table을 위한 함수
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qtc.h>
#include <qmcMemHashTempTable.h>

IDE_RC 
qmcMemHash::init( qmcdMemHashTemp * aTempTable,
                  qcTemplate      * aTemplate,
                  iduMemory       * aMemory,
                  qmdMtrNode      * aRecordNode,
                  qmdMtrNode      * aHashNode,
                  UInt              aBucketCnt,
                  idBool            aDistinct )
{
/***********************************************************************
 *
 * Description :
 *    Memory Hash Temp Table을 초기화한다.
 *
 * Implementation :
 *    - 기본 정보 설정
 *    - 삽입 정보 설정
 *    - 검색 정보 설정
 *
 ***********************************************************************/

    // 적합성 검사
    IDE_DASSERT( aTempTable != NULL );
    IDE_DASSERT( aBucketCnt > 0 && aBucketCnt <= QMC_MEM_HASH_MAX_BUCKET_CNT );
    IDE_DASSERT( aRecordNode != NULL && aHashNode != NULL );

    //----------------------------------------------------
    // Memory Hash Temp Table의 기본 정보 설정
    //----------------------------------------------------

    aTempTable->flag = QMC_MEM_HASH_INITIALIZE;
    
    aTempTable->mTemplate  = aTemplate;
    aTempTable->mMemory    = aMemory;
    aTempTable->mBucketCnt = aBucketCnt;

    // Bucket을 위한 Memory 할당
    // Bucket내의 연결 정보를 초기화를 위해 cralloc을 사용한다.
    IDU_FIT_POINT( "qmcMemHash::init::alloc::mBucket",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST( aTempTable->mMemory->cralloc( aTempTable->mBucketCnt * ID_SIZEOF(qmcBucket),
                                            (void**)&aTempTable->mBucket )
              != IDE_SUCCESS);

    aTempTable->mRecordNode = aRecordNode;
    aTempTable->mHashNode   = aHashNode;

    //----------------------------------------------------
    // 삽입을 위한 정보
    //----------------------------------------------------

    if ( aDistinct == ID_TRUE )
    {
        // Distinct Insertion의 경우
        aTempTable->flag &= ~QMC_MEM_HASH_DISTINCT_MASK;
        aTempTable->flag |= QMC_MEM_HASH_DISTINCT_TRUE;
    }
    else
    {
        aTempTable->flag &= ~QMC_MEM_HASH_DISTINCT_MASK;
        aTempTable->flag |= QMC_MEM_HASH_DISTINCT_FALSE;
    }
        
    aTempTable->mTop = NULL;
    aTempTable->mLast = NULL;
    
    aTempTable->mRecordCnt = 0;

    // Distinction의 여부에 관계 없이 처음 삽입은 항상 성공한다.
    // 이 외의 초기화 처리를 위해 다음 함수 포인터를 연결한다.
    
    aTempTable->insert = qmcMemHash::insertFirst;

    //----------------------------------------------------
    // 검색을 위한 정보
    //----------------------------------------------------

    aTempTable->mKey = 0;
    aTempTable->mCurrent = NULL;
    aTempTable->mNext = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC 
qmcMemHash::clear( qmcdMemHashTemp * aTempTable )
{
/***********************************************************************
 *
 * Description :
 *     Memory Hash Temp Table을 초기화한다.
 *
 * Implementation :
 *     Record에 대한 메모리는 상위 Hash Temp Table에서 관리하며,
 *     Bucket에 대한 연결 정보만 초기화한다.
 *
 ***********************************************************************/

    // Bucket의 초기화
    idlOS::memset( aTempTable->mBucket,
                   0x00,
                   aTempTable->mBucketCnt * ID_SIZEOF(qmcBucket) );

    // 삽입 정보의 초기화
    aTempTable->mTop = NULL;
    aTempTable->mLast = NULL;
    aTempTable->mRecordCnt = 0;

    aTempTable->insert = qmcMemHash::insertFirst;

    // 검색 정보의 초기화
    aTempTable->mKey = 0;
    aTempTable->mCurrent = NULL;
    aTempTable->mNext = NULL;

    return IDE_SUCCESS;
}

IDE_RC 
qmcMemHash::clearHitFlag( qmcdMemHashTemp * aTempTable )
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

    qmcMemHashElement * sElement;

    // 최초 Record를 읽는다.
    IDE_TEST( qmcMemHash::getFirstSequence( aTempTable,
                                            (void**) & sElement )
              != IDE_SUCCESS );

    while ( sElement != NULL )
    {
        // Hit Flag을 초기화함.
        sElement->flag &= ~QMC_ROW_HIT_MASK;
        sElement->flag |= QMC_ROW_HIT_FALSE;

        IDE_TEST( qmcMemHash::getNextSequence( aTempTable,
                                               (void**) & sElement )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC 
qmcMemHash::insertFirst( qmcdMemHashTemp * aTempTable,
                         UInt              aHash,
                         void            * aElement,
                         void           ** aOutElement )
{
/***********************************************************************
 *
 * Description :
 *    최초 Record를 삽입한다.
 *    Distinction 여부에 관계 없이 최초 record삽입은 성공하며,
 *    최초 Bucket에 대한 초기화가 이루어진다.
 * 
 * Implementation :
 *
 ***********************************************************************/

    UInt sKey;

    // 주어진 Hash Key값으로 Bucket의 위치를 얻는다.
    sKey = qmcMemHash::getBucketID(aTempTable, aHash);

    // 삽입 관리를 위한 자료 구조를 초기화한다.
    // Bucket에 대한 연결관계를 만든다.
    aTempTable->mTop = & aTempTable->mBucket[sKey];
    aTempTable->mLast = & aTempTable->mBucket[sKey];

    // 해당 Bucket에 Element를 연결한다.
    aTempTable->mLast->element = (qmcMemHashElement*) aElement;
    aTempTable->mLast->element->key = aHash;

    // 삽입이 성공하였음을 표시
    *aOutElement = NULL;

    //-------------------------------------------------
    // Distinction 여부에 따라 Insert할 함수를 결정함
    //-------------------------------------------------

    if ( (aTempTable->flag & QMC_MEM_HASH_DISTINCT_MASK)
         == QMC_MEM_HASH_DISTINCT_TRUE )
    {
        // Distinction일 경우
        aTempTable->insert = qmcMemHash::insertDist;

        // Distinct Insert일 경우만 자동 확장한다.
        aTempTable->flag &= ~QMC_MEM_HASH_AUTO_EXTEND_MASK;
        aTempTable->flag |= QMC_MEM_HASH_AUTO_EXTEND_ENABLE;
    }
    else
    {
        // Non-distinction인 경우
        aTempTable->insert = qmcMemHash::insertAny;

        // Non-Distinct Insert일 경우, 자동 확장하지 않는다.
        // 이는 Bucket의 확장이 even-distribution을 보장할 수
        // 없기 때문이다.
        aTempTable->flag &= ~QMC_MEM_HASH_AUTO_EXTEND_MASK;
        aTempTable->flag |= QMC_MEM_HASH_AUTO_EXTEND_DISABLE;
    }

    aTempTable->mRecordCnt++;

    return IDE_SUCCESS;
}

IDE_RC 
qmcMemHash::insertAny( qmcdMemHashTemp * aTempTable,
                       UInt              aHash, 
                       void            * aElement, 
                       void           ** aOutElement )
{
/***********************************************************************
 *
 * Description :
 *    Non-Distinct Insertion을 수행한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    // Bucket의 위치 획득
    UInt sKey;
    qmcMemHashElement * sElement;

    sKey     = qmcMemHash::getBucketID(aTempTable, aHash);
    sElement = aTempTable->mBucket[sKey].element;

    if ( sElement == NULL )
    {
        //----------------------------------
        // 새로운 Bucket에 할당받은 경우
        //----------------------------------

        // Bucket간의 연결 정보 구축
        aTempTable->mLast->next = & aTempTable->mBucket[sKey];
        aTempTable->mLast = & aTempTable->mBucket[sKey];

        // 새로운 Bucket에 element 연결
        aTempTable->mLast->element = (qmcMemHashElement *) aElement;
        aTempTable->mLast->element->key = aHash;
    }
    else
    {
        //----------------------------------
        // 이미 사용하고 있는 Bucket인 경우
        //----------------------------------

        // Bucket내의 element들과 연결한다.
        ((qmcMemHashElement *)aElement)->next =
            aTempTable->mBucket[sKey].element;
        aTempTable->mBucket[sKey].element = (qmcMemHashElement *)aElement;
        aTempTable->mBucket[sKey].element->key = aHash;
    }

    // 삽입이 성공했음을 표기
    *aOutElement = NULL;

    aTempTable->mRecordCnt++;

    return IDE_SUCCESS;
}

IDE_RC 
qmcMemHash::insertDist( qmcdMemHashTemp * aTempTable,
                        UInt              aHash, 
                        void            * aElement, 
                        void           ** aOutElement )
{
/***********************************************************************
 *
 * Description :
 *    Distinction Insert를 수행한다.
 *  
 * Implementation :
 *    동일한 Hash Key를 가진 Record를 우선 검사하고,
 *    Key가 동일하다면, Data 자체도 비교한다.
 *    다른 record라면 삽입하고,  같은 Record라면 삽입 실패의 원인이 된
 *    Record를 돌려준다.
 *
 ***********************************************************************/

    // Bucket 위치 획득
    UInt sKey;
    qmcMemHashElement * sElement;
    qmcMemHashElement * sLastElement = NULL;
    SInt sResult;

    sKey     = qmcMemHash::getBucketID( aTempTable, aHash);
    sElement = aTempTable->mBucket[sKey].element;

    if ( sElement == NULL )
    {
        //--------------------------------------------
        // Bucket이 비어 있는 경우, Record를 삽입한다.
        //--------------------------------------------

        aTempTable->mLast->next = & aTempTable->mBucket[sKey];
        aTempTable->mLast = & aTempTable->mBucket[sKey];
        aTempTable->mLast->element = (qmcMemHashElement *) aElement;
        aTempTable->mLast->element->key = aHash;
        *aOutElement = NULL;

        // Record 개수 증가
        aTempTable->mRecordCnt++;
    }
    else
    {
        //--------------------------------------------
        // Bucket이 비어 있지 않은 경우
        // 1. Hash Key 가 같은지 비교
        // 2. Hashing Column이 동일한 지를 비교
        //--------------------------------------------

        sResult = 1;

        // Bucket내의 모든 Record 에 대하여 검사
        while ( sElement != NULL )
        {
            if ( sElement->key == aHash )
            {
                // Hash Key가 같은 경우, Hashing Column을 비교
                sResult = qmcMemHash::compareRow( aTempTable, 
                                                  sElement, 
                                                  aElement );
                if ( sResult == 0 )
                {
                    //-----------------------
                    // Column의 Data가 같다면
                    //-----------------------
                    break;
                }
            }
            else
            {
                // nothing to do
            }
            
            sLastElement = sElement;
            sElement = sElement->next;
        }

        if ( sResult == 0 )
        {
            // 동일한 Record가 존재하는 경우,
            // 삽입 실패 원인이 된 record를 리턴
            *aOutElement = sElement;
        }
        else
        {
            // 동일한 Record가 없는 경우, record를 삽입
            sLastElement->next = (qmcMemHashElement *) aElement;
            sLastElement->next->key = aHash;
            *aOutElement = NULL;

            // Record 개수 증가
            aTempTable->mRecordCnt++;
        }
    }

    // [자동 확장 여부 검사.]
    // 다음 조건을 검사.
    // 1. Record가 지정된 Bucket보다 특정 비율 이상보다 많은 경우
    // 2. Bucket을 증가시킬 수 있는 경우
    if ( ( aTempTable->mRecordCnt >
           (aTempTable->mBucketCnt * QMC_MEM_HASH_AUTO_EXTEND_CONDITION) )
         &&
         ( ( aTempTable->flag & QMC_MEM_HASH_AUTO_EXTEND_MASK )
           == QMC_MEM_HASH_AUTO_EXTEND_ENABLE ) )
    {
        // Bucket의 자동 확장 수행
        IDE_TEST( extendBucket( aTempTable ) != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC 
qmcMemHash::getFirstSequence( qmcdMemHashTemp * aTempTable,
                              void           ** aElement )
{
/***********************************************************************
 *
 * Description :
 *    순차 검색을 수행
 *
 * Implementation :
 *    최초 Bucket의 위치로 이동하여 Record를 리턴한다.
 *
 ***********************************************************************/

    if ( aTempTable->mTop == NULL )
    {
        //--------------------------
        // 최초 Bucket이 없는 경우
        //--------------------------
        
        *aElement = NULL;
        aTempTable->mCurrent = NULL;
    }
    else
    {
        //--------------------------
        // 최초 Bucket이 있는 경우
        //--------------------------

        // Bucket내의 최초 Record를 리턴
        *aElement = aTempTable->mTop->element;

        // 현재 Bucket을 설정
        aTempTable->mCurrent = aTempTable->mTop;

        // 현재 Bucket내의 다음 record를 결정
        aTempTable->mNext = aTempTable->mTop->element->next;
    }

    return IDE_SUCCESS;
}

IDE_RC 
qmcMemHash::getNextSequence( qmcdMemHashTemp * aTempTable,
                             void           ** aElement)
{
/***********************************************************************
 *
 * Description :
 *    다음 순차 검색을 수행
 *
 * Implementation :
 *    해당 Record가 없다면, 다음 Bucket에서 Record를 추출한다.
 *
 ***********************************************************************/

    qmcMemHashElement * sElement;

    // 다음 Record 정보 획득
    sElement = aTempTable->mNext;
    
    if ( sElement != NULL )
    {
        //-----------------------------------
        // 다음 Record가 존재하는 경우
        //-----------------------------------
        
        *aElement = sElement;
        aTempTable->mNext = sElement->next;
    }
    else
    {
        //-----------------------------------
        // 다음 Record가 없는 경우
        //-----------------------------------

        // 다음 Bucket을 획득
        aTempTable->mCurrent = aTempTable->mCurrent->next;
        if ( aTempTable->mCurrent != NULL )
        {
            // 다음 Bucket이 존재하는 경우

            // 해당 Bucket내의 record를 리턴
            sElement = aTempTable->mCurrent->element;
            *aElement = sElement;

            // 다음 Record를 지정
            aTempTable->mNext = sElement->next;
        }
        else
        {
            // 다음 Bucket이 없는 경우
            // 더 이상 데이터가 존재하지 않는다.
            *aElement = NULL;

            // 다음 Record 없음을 설정
            aTempTable->mNext = NULL;
        }
    }

    return IDE_SUCCESS;
}

IDE_RC 
qmcMemHash::getFirstRange( qmcdMemHashTemp * aTempTable,
                           UInt              aHash,
                           qtcNode         * aFilter,
                           void           ** aElement )
{
/***********************************************************************
 *
 * Description :
 *     주어진 Key 값과 Filter를 이용하여 Range 검색을 한다.
 *
 * Implementation :
 *     Key값으로 Bucket을 획득하고,
 *     Key값이 같고 주어진 Filter를 만족하는 record를 획득한다.
 *     
 ***********************************************************************/

    // Bucket의 위치 획득
    UInt sKey;
    
    qmcMemHashElement * sElement;
    idBool sJudge = ID_FALSE;

    sKey = getBucketID(aTempTable, aHash);

    // 다음 Range 검색을 위해 Key값과 Filter를 저장
    aTempTable->mKey = aHash;
    aTempTable->mFilter = aFilter;

    sElement = aTempTable->mBucket[sKey].element;

    // Bucket내의 record들을 검사
    while ( sElement != NULL )
    {
        if ( sElement->key == aHash )
        {
            // 동일한 Key값을 갖는 경우, Filter조건도 검사.
            IDE_TEST( qmcMemHash::judgeFilter( aTempTable,
                                               sElement,
                                               & sJudge)
                      != IDE_SUCCESS );
            
            if ( sJudge == ID_TRUE )
            {
                // 원하는 record를 찾은 경우
                
                // To Fix PR-8645
                // Hash Key 값을 검사하기 위한 Filter에서 Access값을
                // 증가하였고, 이 후 Execution Node에서 이를 다시 증가시킴.
                // 따라서, 마지막 한번의 검사에 대해서는 access회수를 감소시킴
                aTempTable->mRecordNode->dstTuple->modify--;
                break;
            }
        }
        else
        {
            // nothing to do
        }
        
        sElement = sElement->next;
    }

    if ( sJudge == ID_TRUE )
    {
        // 원하는 Record를 찾은 경우
        *aElement = (void*)sElement;

        // 다음 검색을 위하여 다음 위치를 저장
        aTempTable->mNext = sElement->next;
    }
    else
    {
        // 원하는 Record를 찾지 못한 경우
        *aElement = NULL;

        // 다음 Range 검색을 할 수 없음을 지정
        aTempTable->mNext = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC 
qmcMemHash::getNextRange( qmcdMemHashTemp * aTempTable,
                          void           ** aElement )
{
/***********************************************************************
 *
 * Description :
 *    다음 Range 검색을 수행한다.
 *
 * Implementation :
 *    이미 저장된 정보를 이용하여, Bucket내의 다음 record부터 filter
 *    조건을 검사하여 record를 검색한다.
 *
 ***********************************************************************/

    qmcMemHashElement * sElement;
    idBool sJudge = ID_FALSE;

    // 동일 Bucket내에서 이전의 검색된 record의 다음 record를 획득
    sElement = aTempTable->mNext;

    // Bucket내에 record가 없을 때까지 반복
    while ( sElement != NULL )
    {
        if ( sElement->key == aTempTable->mKey )
        {
            IDE_TEST( qmcMemHash::judgeFilter( aTempTable,
                                               sElement,
                                               & sJudge )
                      != IDE_SUCCESS );
            
            if ( sJudge == ID_TRUE )
            {
                // key값이 동일하고 조건을 만족할 경우
                
                // To Fix PR-8645
                // Hash Key 값을 검사하기 위한 Filter에서 Access값을
                // 증가하였고, 이 후 Execution Node에서 이를 다시 증가시킴.
                // 따라서, 마지막 한번의 검사에 대해서는 access회수를 감소시킴
                aTempTable->mRecordNode->dstTuple->modify--;
                
                break;
            }
        }
        else
        {
            // nothing to do
        }
        
        sElement = sElement->next;
    }

    if ( sJudge == ID_TRUE )
    {
        // 조건을 만족하는 record를 검색한 경우
        *aElement = (void*)sElement;

        // 다음 검색을 위하여 다음 record 위치를 저장
        aTempTable->mNext = sElement->next;
    }
    else
    {
        // 조건을 만족하는 record가 없는 경우
        *aElement = NULL;
        aTempTable->mNext = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC 
qmcMemHash::getSameRowAndNonHit( qmcdMemHashTemp * aTempTable,
                               UInt              aHash,
                               void            * aRow,
                               void           ** aElement )
{
/***********************************************************************
 *
 * Description :
 *    해당 Row와 동일한 Hashing Column의 값을 가지면서,
 *    Hit되지 않은 record를 검색한다.
 *    Set Intersectoin에서 사용하며, 한 건만이 검색된다.
 *
 * Implementation :
 *    주어진 Key값으로부터 Bucket을 선택한다.
 *    주어진 모든 record에 대하여 다음과 같은 검사를 수행하여
 *    모든 조건을 만족하는 record를 찾는다.
 *        - Key 가 동일함.
 *        - Non-Hit
 *        - Hash Column의 값이 동일함.
 *
 ***********************************************************************/

    qmcMemHashElement * sElement;
    UInt                sKey;
    SInt                sResult;

    sKey = getBucketID( aTempTable, aHash );

    sElement = aTempTable->mBucket[sKey].element;
    
    while ( sElement != NULL )
    {
        //------------------------------------
        // Key가 같고, Hit되지 않고
        // Data가 동일한 Record를 찾는다.
        //------------------------------------
        
        // Hash Key값 및 Hit Flag 검사.
        if ( (sElement->key == aHash) &&
             ( (sElement->flag & QMC_ROW_HIT_MASK) == QMC_ROW_HIT_FALSE ) )
        {  
            sResult = compareRow( aTempTable,
                                  (void*) aRow,
                                  (void*) sElement );
            if ( sResult == 0 )
            {
                // 동일한 Record인 경우
                break;
            }
        }
        sElement = sElement->next;
    }

    *aElement = sElement;

    return IDE_SUCCESS;
}


IDE_RC 
qmcMemHash::getFirstHit( qmcdMemHashTemp * aTempTable,
                         void           ** aElement )
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

    qmcMemHashElement * sElement;

    IDE_TEST( getFirstSequence( aTempTable, aElement ) != IDE_SUCCESS );

    while ( *aElement != NULL )
    {
        sElement = (qmcMemHashElement*) *aElement;
        if ( (sElement->flag & QMC_ROW_HIT_MASK) == QMC_ROW_HIT_TRUE )
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

IDE_RC 
qmcMemHash::getNextHit( qmcdMemHashTemp * aTempTable,
                        void           ** aElement )
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

    qmcMemHashElement * sElement;

    IDE_TEST( getNextSequence( aTempTable, aElement ) != IDE_SUCCESS );

    while ( *aElement != NULL )
    {
        sElement = (qmcMemHashElement*) *aElement;
        if ( (sElement->flag & QMC_ROW_HIT_MASK) == QMC_ROW_HIT_TRUE )
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

IDE_RC 
qmcMemHash::getFirstNonHit( qmcdMemHashTemp * aTempTable,
                            void           ** aElement )
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

    qmcMemHashElement * sElement;

    IDE_TEST( getFirstSequence( aTempTable, aElement ) != IDE_SUCCESS );

    while ( *aElement != NULL )
    {
        sElement = (qmcMemHashElement*) *aElement;
        if ( (sElement->flag & QMC_ROW_HIT_MASK) == QMC_ROW_HIT_FALSE )
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

IDE_RC 
qmcMemHash::getNextNonHit( qmcdMemHashTemp * aTempTable,
                            void           ** aElement )
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

    qmcMemHashElement * sElement;

    IDE_TEST( getNextSequence( aTempTable, aElement ) != IDE_SUCCESS );

    while ( *aElement != NULL )
    {
        sElement = (qmcMemHashElement*) *aElement;
        if ( (sElement->flag & QMC_ROW_HIT_MASK) == QMC_ROW_HIT_FALSE )
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

IDE_RC 
qmcMemHash::getDisplayInfo( qmcdMemHashTemp * aTempTable,
                            SLong           * aRecordCnt,
                            UInt            * aBucketCnt )
{
/***********************************************************************
 *
 * Description :
 *    삽입되어 있는 Record 개수를 획득한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    *aRecordCnt = aTempTable->mRecordCnt;
    *aBucketCnt = aTempTable->mBucketCnt;

    return IDE_SUCCESS;
}

UInt        
qmcMemHash::getBucketID( qmcdMemHashTemp * aTempTable,
                         UInt              aHash )
{
/***********************************************************************
 *
 * Description :
 *    주어진 Hash Key로부터 Bucket ID를 얻는다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmcMemHash::getBucketID"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qmcMemHash::getBucketID"));

    return aHash % aTempTable->mBucketCnt;

#undef IDE_FN
}

SInt          
qmcMemHash::compareRow ( qmcdMemHashTemp * aTempTable,
                         void            * aElem1,
                         void            * aElem2 )

{
/***********************************************************************
 *
 * Description :
 *    저장된 Record의 Hashing Column의 값을 비교하여
 *    값이 동일한 지 다른 지를 Return함.
 *
 * Implementation :
 *
 ***********************************************************************/

    qmdMtrNode * sNode;
    mtdValueInfo sValueInfo1;
    mtdValueInfo sValueInfo2;
    
    SInt sResult = -1;

    for( sNode = aTempTable->mHashNode; 
         sNode != NULL; 
         sNode = sNode->next )
    {
        // 두 Record의 Hashing Column의 값을 비교
        sValueInfo1.value  = sNode->func.getRow(sNode, aElem1);
        sValueInfo1.column = (const mtcColumn *)sNode->func.compareColumn;
        sValueInfo1.flag   = MTD_OFFSET_USE;
        
        sValueInfo2.value  = sNode->func.getRow(sNode, aElem2);
        sValueInfo2.column = (const mtcColumn *)sNode->func.compareColumn;
        sValueInfo2.flag   = MTD_OFFSET_USE;
        
        sResult = sNode->func.compare( &sValueInfo1,
                                       &sValueInfo2 );
        if( sResult != 0)
        {
            // 서로 다른 경우, 더 이상 비교를 하지 않음.
            break;
        }
    }

    return sResult;
}

IDE_RC          
qmcMemHash::judgeFilter ( qmcdMemHashTemp * aTempTable,
                          void *            aElem,
                          idBool          * aResult )

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

    qmdMtrNode * sNode;

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

IDE_RC          
qmcMemHash::extendBucket ( qmcdMemHashTemp * aTempTable )

{
/***********************************************************************
 *
 * Description :
 *    특정 조건(Bucket보다 Record가 매우 많은 경우)일 경우
 *    Bucket의 개수를 자동 확장한다.
 *
 * Implementation :
 *    다음과 같은 절차로 수행된다.
 *        - Bucket의 확장 가능성 검사.
 *        - 새로운 Bucket을 할당받음.
 *        - 기존 Bucket으로부터 record를 얻어와 새로운 Bucket에
 *          삽입한다.
 *        - 이 때, Distinction 여부를 검사할 필요는 없다.
 *
 ***********************************************************************/

    qmcMemHashElement * sElement;

    aTempTable->mExtBucket = NULL;
    aTempTable->mExtTop = NULL;
    aTempTable->mExtLast = NULL;
    

    // Bucket 확장이 가능한지를 검사.
    if ( (aTempTable->mBucketCnt * QMC_MEM_HASH_AUTO_EXTEND_RATIO) >
         QMC_MEM_HASH_MAX_BUCKET_CNT )
    {
        // Bucket Count를 증가시켰을 때 최대 Bucket Size를 넘는 경우라면,
        // 증가시킬 수 없다.

        // 더 이상 증가할 수 없음을 표시
        aTempTable->flag &= ~QMC_MEM_HASH_AUTO_EXTEND_MASK;
        aTempTable->flag |= QMC_MEM_HASH_AUTO_EXTEND_DISABLE;
    }
    else
    {
        // Bucket Count를 증가시킬 수 있는 경우

        //-------------------------------------------------
        // 새롭게 확장할 Bucket의 개수 계산 및 Memory 할당
        //-------------------------------------------------
        
        aTempTable->mExtBucketCnt = aTempTable->mBucketCnt * QMC_MEM_HASH_AUTO_EXTEND_RATIO;
        
        IDU_FIT_POINT( "qmcMemHash::extendBucket::cralloc::mExtBucket",
                        idERR_ABORT_InsufficientMemory );

        IDE_TEST( aTempTable->mMemory->cralloc( aTempTable->mExtBucketCnt * ID_SIZEOF(qmcBucket),
                                                (void**)&aTempTable->mExtBucket )
                  != IDE_SUCCESS);

        //-------------------------------------------------
        // 기존의 Bucket으로부터 Record를 얻어
        // 새로운 Bucket에 삽입
        //-------------------------------------------------

        // 최초 Record 획득
        IDE_TEST( qmcMemHash::getFirstSequence( aTempTable,
                                                (void**) & sElement )
                  != IDE_SUCCESS );
        IDE_ASSERT( sElement != NULL );

        // 최초 Record의 삽입
        IDE_TEST( qmcMemHash::insertFirstNewBucket( aTempTable, sElement )
                  != IDE_SUCCESS );

        //-------------------------------------------------
        // Record가 존재하지 않을 때까지 반복함
        //-------------------------------------------------
        
        IDE_TEST( qmcMemHash::getNextSequence( aTempTable,
                                               (void**) & sElement )
                  != IDE_SUCCESS );

        while ( sElement != NULL )
        {
            // 다음 Record의 삽입
            IDE_TEST( qmcMemHash::insertNextNewBucket( aTempTable, sElement )
                      != IDE_SUCCESS );

            // 다음 Record 획득
            IDE_TEST( qmcMemHash::getNextSequence( aTempTable,
                                                   (void**) & sElement )
                      != IDE_SUCCESS );
        }

        //-------------------------------------------------
        // 새로운 Bucket으로 전환시킴
        // 기존의 Bucket Memory가 dangling되나,
        // 최종적으로 qmxMemory 관리자에 의하여 제거된다.
        // Bucket의 확장은 insert과정 중에 발생하며,
        // Insert과정 주에 검색 과정은 존재하지 않으므로,
        // 다른 정보에 대한 초기화는 필요없다.
        //-------------------------------------------------

        aTempTable->mBucketCnt = aTempTable->mExtBucketCnt;
        aTempTable->mBucket = aTempTable->mExtBucket;
        aTempTable->mTop = aTempTable->mExtTop;
        aTempTable->mLast = aTempTable->mExtLast;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC
qmcMemHash::insertFirstNewBucket( qmcdMemHashTemp   * aTempTable,
                                  qmcMemHashElement * aElem )
{
/***********************************************************************
 *
 * Description :
 *    Bucket 확장 시 최초 record를 삽입함.
 *
 * Implementation :
 *    새로운 Bucket에 대한 초기화 및 record를 삽입한다.
 *    기존의 삽입 방법과 달리,
 *    Hash Element에 이미 Key값이 존재하므로 이에 대한
 *    별도의 저장도 필요 없다.
 *    
 ***********************************************************************/

    UInt    sBucketID;

    // 주어진 Hash Key값으로 새로운 Bucket의 위치를 얻는다.
    sBucketID = aElem->key % aTempTable->mExtBucketCnt;

    // 새로운 연결 정보를 위하여 clear한다.
    aElem->next = NULL;
    
    // 삽입 관리를 위한 자료 구조를 초기화한다.
    // Bucket에 대한 연결관계를 만든다.
    aTempTable->mExtTop = & aTempTable->mExtBucket[sBucketID];
    aTempTable->mExtLast = & aTempTable->mExtBucket[sBucketID];

    // 해당 Bucket에 Element를 연결한다.
    aTempTable->mExtLast->element = (qmcMemHashElement*) aElem;

    return IDE_SUCCESS;
}

IDE_RC
qmcMemHash::insertNextNewBucket( qmcdMemHashTemp   * aTempTable,
                                 qmcMemHashElement * aElem )
{
/***********************************************************************
 *
 * Description :
 *    Bucket 확장 시 다음 record를 삽입함.
 *
 * Implementation :
 *    기존의 삽입 방법과 달리,
 *    Hash Element에 이미 Key값이 존재하므로 이에 대한
 *    별도의 저장도 필요 없다.
 *    이미 Distinction이 보장되어 있기 때문에,
 *    Hash Key 검사 및 Column의 값 검사 등이 필요 없다.
 *
 ***********************************************************************/

    UInt sBucketID;
    qmcMemHashElement * sElement;

    // To Fix PR-8382
    // Extend Bucket을 사용하여 연결 관계를 생성해야 함.
    // 주어진 Hash Key값으로 새로운 Bucket의 위치를 얻는다.
    sBucketID = aElem->key % aTempTable->mExtBucketCnt;
    sElement = aTempTable->mExtBucket[sBucketID].element;

    // 새로운 연결 정보를 위하여 clear한다.
    aElem->next = NULL;

    if ( sElement == NULL )
    {
        //----------------------------------
        // 새로운 Bucket에 할당받은 경우
        //----------------------------------

        // Bucket간의 연결 정보 구축
        aTempTable->mExtLast->next = & aTempTable->mExtBucket[sBucketID];
        aTempTable->mExtLast = & aTempTable->mExtBucket[sBucketID];

        // 새로운 Bucket에 element 연결
        aTempTable->mExtLast->element = (qmcMemHashElement *) aElem;
    }
    else
    {
        //----------------------------------
        // 이미 사용하고 있는 Bucket인 경우
        //----------------------------------

        // Bucket내의 element들과 연결한다.
        ((qmcMemHashElement *)aElem)->next =
            aTempTable->mExtBucket[sBucketID].element;
        aTempTable->mExtBucket[sBucketID].element = (qmcMemHashElement *)aElem;
    }
    
    return IDE_SUCCESS;
}
