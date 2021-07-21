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
 * $Id: qmcMemSortTempTable.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *     Memory Sort Temp Table을 위한 함수
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 *
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qmc.h>
#include <qcg.h>
#include <qmcMemSortTempTable.h>

#define QMC_MEM_SORT_SWAP( elem1, elem2 )               \
    { temp = elem1; elem1 = elem2; elem2 = temp; }

IDE_RC
qmcMemSort::init( qmcdMemSortTemp * aTempTable,
                  qcTemplate      * aTemplate,
                  iduMemory       * aMemory,
                  qmdMtrNode      * aSortNode )
{
/***********************************************************************
 *
 * Description :
 *
 *    해당 Memory Sort Temp Table객체를 초기화한다.
 *
 * Implementation :
 *
 *    BUG-38290
 *    Temp table 은 내부에 qcTemplate 과 qmcMemory 를 가지고
 *    temp table 생성에 사용한다.
 *    이 두가지는 temp table 을 init 할 때의 template 과 그 template 에
 *    연결된 QMX memory 이다.
 *    만약 temp table init 시점과 temp table build 시점에 서로 다른
 *    template 을 사용해야 한다면 이 구조가 변경되어야 한다.
 *    Parallel query 대상이 증가하면서 temp table build 가 parallel 로
 *    진행될 경우 이 내용을 고려해야 한다.
 *
 *    Temp table build 는 temp table 이 존재하는 plan node 의
 *    init 시점에 실행된다.
 *    하지만 현재 parallel query 는 partitioned table 이나 HASH, SORT,
 *    GRAG  노드에만 PRLQ 노드를 생성, parallel 실행하므로 
 *    temp table 이 존재하는 plan node 를 직접 init  하지 않는다.
 *
 *    다만 subqeury filter 내에서 temp table 을 사용할 경우는 예외가 있을 수
 *    있으나, subquery filter 는 plan node 가 실행될 때 초기화 되므로
 *    동시에 temp table  이 생성되는 일은 없다.
 *    (qmcTempTableMgr::addTempTable 의 관련 주석 참조)
 *
 ***********************************************************************/

    IDE_DASSERT( aTempTable != NULL );

    //------------------------------------------------------------
    // Sort Index Table 의 관리를 위한 Memory 관리자의 생성 및 초기화
    //------------------------------------------------------------
    aTempTable->mMemory = aMemory;

    IDU_FIT_POINT( "qmcMemSort::init::alloc::mMemory",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST(aTempTable->mMemory->alloc( ID_SIZEOF(qmcMemory),
                                      (void**)&aTempTable->mMemoryMgr)
             != IDE_SUCCESS);

    aTempTable->mMemoryMgr = new (aTempTable->mMemoryMgr)qmcMemory();

    /* BUG-38290 */
    aTempTable->mMemoryMgr->init( ID_SIZEOF(qmcSortArray) );

    IDU_FIT_POINT( "qmcMemSort::init::cralloc::mArray",
                    idERR_ABORT_InsufficientMemory );

    IDE_TEST(aTempTable->mMemoryMgr->cralloc( aTempTable->mMemory,
                                           ID_SIZEOF(qmcSortArray),
                                           (void**)&aTempTable->mArray )
             != IDE_SUCCESS);

    //------------------------------------------------------------
    // Memory Sort Temp Table 객체의 초기화
    //------------------------------------------------------------

    aTempTable->mArray->parent      = NULL;
    aTempTable->mArray->depth       = 0;
    aTempTable->mArray->numIndex    = QMC_MEM_SORT_MAX_ELEMENTS;
    aTempTable->mArray->shift       = 0;
    aTempTable->mArray->mask        = 0;
    aTempTable->mArray->index       = 0;
    aTempTable->mArray->elements    = aTempTable->mArray->internalElem;

    aTempTable->mTemplate           = aTemplate;
    aTempTable->mSortNode           = aSortNode;

    // TASK-6445
    aTempTable->mUseOldSort         = QCG_GET_SESSION_USE_OLD_SORT( aTemplate->stmt );
    aTempTable->mRunStack           = NULL;
    aTempTable->mRunStackSize       = 0;
    aTempTable->mTempArray          = NULL;

    aTempTable->mStatistics   = aTemplate->stmt->mStatistics;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmcMemSort::clear( qmcdMemSortTemp * aTempTable )
{
/***********************************************************************
 *
 * Description :
 *    저장된 Sorting Data 정보를 제거한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    if ( aTempTable->mMemoryMgr != NULL )
    {
        aTempTable->mMemoryMgr->clearForReuse();

        aTempTable->mArray = NULL;

        IDU_FIT_POINT( "qmcMemSort::clear::cralloc::mArray",
                        idERR_ABORT_InsufficientMemory );

        /* BUG-38290 */
        IDE_TEST(aTempTable->mMemoryMgr->cralloc(
                aTempTable->mMemory,
                ID_SIZEOF(qmcSortArray),
                (void**)&aTempTable->mArray)
            != IDE_SUCCESS);

        aTempTable->mArray->parent      = NULL;
        aTempTable->mArray->depth       = 0;
        aTempTable->mArray->numIndex    = QMC_MEM_SORT_MAX_ELEMENTS;
        aTempTable->mArray->shift       = 0;
        aTempTable->mArray->mask        = 0;
        aTempTable->mArray->index       = 0;
        aTempTable->mArray->elements    = aTempTable->mArray->internalElem;
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmcMemSort::clearHitFlag( qmcdMemSortTemp * aTempTable )
{
/***********************************************************************
 *
 * Description :
 *    저장된 모든 Record의 Hit Flag을 reset한다.
 *
 * Implementation :
 *    최초 위치로 이동하여 모든 Record를 검색하고,
 *    이에 대한 Hit Flag을 reset한다.
 *
 ***********************************************************************/

    qmcMemSortElement * sRow;

    IDE_TEST( beforeFirstElement( aTempTable ) != IDE_SUCCESS );

    IDE_TEST( getNextElement( aTempTable, (void**) & sRow ) != IDE_SUCCESS );

    while ( sRow != NULL )
    {
        sRow->flag &= ~QMC_ROW_HIT_MASK;
        sRow->flag |= QMC_ROW_HIT_FALSE;

        IDE_TEST( getNextElement( aTempTable, (void**) & sRow )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmcMemSort::attach( qmcdMemSortTemp * aTempTable,
                    void            * aRow )
{
/***********************************************************************
 *
 * Description :
 *    Memory Sort Temp Table에 Record를 추가한다.
 *
 * Implementation :
 *    저장될 위치를 찾아, Pointer를 연결한다.
 *    이후 Sort Array를 증가시킨다.
 *
 ***********************************************************************/

    void** sDestination;

    get( aTempTable,
         aTempTable->mArray,
         aTempTable->mArray->index,
         & sDestination );

    *sDestination = aRow;

    IDE_TEST( increase( aTempTable,
                        aTempTable->mArray ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmcMemSort::sort( qmcdMemSortTemp * aTempTable )
{
/***********************************************************************
 *
 * Description :
 *     Sorting을 수행한다.
 *
 * Implementation :
 *     저장된 Record 수만큼 quick sort를 수행한다.
 *
 ***********************************************************************/

    if( aTempTable->mArray->index > 0 )
    {
        // TASK-6445
        if ( aTempTable->mUseOldSort == 0 )
        {
            IDE_TEST( timsort( aTempTable,
                               0,                             
                               aTempTable->mArray->index - 1 )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( quicksort( aTempTable,
                                 0,                             
                                 aTempTable->mArray->index - 1 )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmcMemSort::shiftAndAppend( qmcdMemSortTemp * aTempTable,
                            void            * aRow,
                            void           ** aReturnRow )
{
/***********************************************************************
 *
 * Description :
 *     Limit Sorting을 수행한다.
 *
 * Implementation :
 *     입력할 Row가 입력 가능한지를 검사
 *     입력 가능할 경우 마지막 Row를 밀어내고 Row 삽입
 *
 *     Ex)   3을 입력
 *                  +-- [3]이 삽입될 위치
 *                  V
 *           [1, 2, 3, 4, 5]
 *                 ||
 *                \  /
 *                 \/
 *           [1, 2, 3, 3, 4]     [5] : return row
 *
 ***********************************************************************/

    SLong  sMaxIndex;
    SLong  sInsertPos;
    SLong  i;

    void  ** sSrcIndex;
    void  ** sDstIndex;

    void  ** sDiscardRow;

    //-------------------------------------------
    // 삽입 가능 여부 결정
    // - 마지막에 저장된 Data와 비교하여
    //   왼쪽에 해당하면 삽입할 Data이며,
    //   그 외의 경우라면 버려야 할 Data이다.
    //-------------------------------------------

    IDE_TEST( findInsertPosition ( aTempTable, aRow, & sInsertPos )
              != IDE_SUCCESS );

    if ( sInsertPos >= 0 )
    {
        // Discard Row결정
        // 마지막 위치의 Row를 discard row로 결정

        sMaxIndex = aTempTable->mArray->index - 1;

        get( aTempTable, aTempTable->mArray, sMaxIndex, & sDiscardRow );
        *aReturnRow = *sDiscardRow;

        // Shifting 수행
        // 끝에서부터 해당 삽입 위치까지 Shifting

        if ( sInsertPos < sMaxIndex )
        {
            for ( i = sMaxIndex; i > sInsertPos; i-- )
            {
                get( aTempTable, aTempTable->mArray,   i, & sDstIndex );
                get( aTempTable, aTempTable->mArray, i-1, & sSrcIndex );
                *sDstIndex = *sSrcIndex;
            }
            // 삽입 수행
            *sSrcIndex = aRow;
        }
        else
        {
            // To Fix PR-8201
            // 끝에 삽입될 경우
            IDE_DASSERT( sInsertPos == sMaxIndex );

            get( aTempTable, aTempTable->mArray, sInsertPos, & sSrcIndex );
            *sSrcIndex = aRow;
        }
    }
    else
    {
        // 삽입할 Row가 아님
        *aReturnRow = aRow;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmcMemSort::changeMinMax( qmcdMemSortTemp * aTempTable,
                          void            * aRow,
                          void           ** aReturnRow )
{
/***********************************************************************
 *
 * Description :
 *     Min-Max 저장을 위한 Limit Sorting을 수행한다.
 *
 * Implementation :
 *     Right-Most와 비교하여 보다 Right이면 Right-Most 교체
 *     Left-Most와 비교하여 보다 Left이면 Left-Most 교체
 *     이 외의 경우는 삽입하지 않음.
 *
 ***********************************************************************/

    void  ** sIndex;
    SInt     sOrder;

    //-------------------------------------------
    // 적합성 검사
    //-------------------------------------------

    // Min-Max 두 건만이 저장되어 있어야 함.
    IDE_DASSERT( aTempTable->mArray->index == 2 );

    //-------------------------------------------
    // Right-Most 값 교체 여부 결정
    //-------------------------------------------

    get( aTempTable, aTempTable->mArray, 1, & sIndex );

    sOrder = compare( aTempTable, aRow, *sIndex );

    if ( sOrder > 0 )
    {
        // Right-Most 보다 오른 쪽에 있는 경우
        // Right-Most를 교체한다.
        *aReturnRow = *sIndex;
        *sIndex = aRow;
    }
    else
    {
        // Right-Most 보다 같거나 왼쪽에 있는 경우

        get( aTempTable, aTempTable->mArray, 0, & sIndex );

        sOrder = compare( aTempTable, aRow, *sIndex );

        if ( sOrder < 0 )
        {
            // Left-Most보다 왼쪽에 있는 경우
            // Left-Most를 교체한다.
            *aReturnRow = *sIndex;
            *sIndex = aRow;
        }
        else
        {
            // Left-Most와 Right-Most의 중간에 있는 경우로
            // 삽입 대상이 아니다.
            *aReturnRow = aRow;
        }
    }

    return IDE_SUCCESS;
}

IDE_RC
qmcMemSort::getElement( qmcdMemSortTemp * aTempTable,
                        SLong aIndex,
                        void** aElement )
{
/***********************************************************************
 *
 * Description :
 *    지정된 위치의 Record를 가져온다.
 *
 * Implementation :
 *
 ***********************************************************************/

    void** sBuffer;

    // fix BUG-10441
    IDE_DASSERT ( aIndex >= 0 );

    if( aTempTable->mArray->index > 0 )
    {
        // sort temp table에 저장된 레코드가 하나 이상인 경우로
        // 저장된 위치의 record를 가져온다.
        IDE_DASSERT ( aIndex < aTempTable->mArray->index );

        get( aTempTable, aTempTable->mArray, aIndex, &sBuffer );
        *aElement = *sBuffer;
    }
    else
    {
        // sort temp table에 저장된 레코드가 하나도 없는 경우
        IDE_DASSERT( aIndex == 0 );

        *aElement = NULL;
    }

    return IDE_SUCCESS;
}

IDE_RC
qmcMemSort::getFirstSequence( qmcdMemSortTemp * aTempTable,
                              void           ** aRow )
{
/***********************************************************************
 *
 * Description :
 *    첫 번째 순차 검색
 *
 * Implementation :
 *    최초 위치로 이동하여 해당 Record를 획득한다.
 *
 ***********************************************************************/

    IDE_TEST( beforeFirstElement( aTempTable ) != IDE_SUCCESS );

    IDE_TEST( getNextElement( aTempTable,
                              aRow ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmcMemSort::getNextSequence( qmcdMemSortTemp * aTempTable,
                             void           ** aRow )
{
/***********************************************************************
 *
 * Description :
 *    다음 순차 검색
 *
 * Implementation :
 *    해당 Record를 획득한다.
 *
 ***********************************************************************/

    IDE_TEST( getNextElement( aTempTable,
                              aRow ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmcMemSort::getFirstRange( qmcdMemSortTemp * aTempTable,
                           smiRange        * aRange,
                           void           ** aRow )
{
/***********************************************************************
 *
 * Description :
 *    첫 번째 Range 검색
 *
 * Implementation :
 *    주어진 Range를 설정하고, 이에 부합하는 Record를 검색함.
 *
 ***********************************************************************/

    setKeyRange( aTempTable, aRange );

    IDE_TEST( getFirstKey( aTempTable, aRow ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmcMemSort::getNextRange( qmcdMemSortTemp * aTempTable,
                          void           ** aRow )
{
/***********************************************************************
 *
 * Description :
 *    다음 Range 검색
 *
 * Implementation :
 *    Range에 부합하는 Record를 검색함.
 *
 ***********************************************************************/

    IDE_TEST( getNextKey( aTempTable, aRow ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmcMemSort::getFirstHit( qmcdMemSortTemp * aTempTable,
                                void           ** aRow )
{
/***********************************************************************
 *
 * Description : PROJ-2385
 *    Hit된 최초 Record 검색
 *
 * Implementation :
 *    최초 위치로 이동하여, Hit된 Record를 찾을 때까지
 *    반복 검색함.
 *
 ***********************************************************************/

    qmcMemSortElement * sRow;

    IDE_TEST( beforeFirstElement( aTempTable ) != IDE_SUCCESS );

    IDE_TEST( getNextElement( aTempTable, (void**) & sRow ) != IDE_SUCCESS );

    while ( sRow != NULL )
    {
        if ( (sRow->flag & QMC_ROW_HIT_MASK) == QMC_ROW_HIT_TRUE )
        {
            // Hit된 Record일 경우
            break;
        }
        else
        {
            // Hit되지 않은 Reocrd일 경우 다음 Record획득
            IDE_TEST( getNextElement( aTempTable,
                                      (void**) & sRow ) != IDE_SUCCESS );
        }
    }

    *aRow = (void*) sRow;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmcMemSort::getNextHit( qmcdMemSortTemp * aTempTable,
                               void           ** aRow )
{
/***********************************************************************
 *
 * Description : PROJ-2385
 *    Hit된 다음 Record 검색
 *
 * Implementation :
 *    Hit된 Record를 찾을 때까지 반복 검색함.
 *
 ***********************************************************************/

    qmcMemSortElement * sRow;

    IDE_TEST( getNextElement( aTempTable, (void**) & sRow ) != IDE_SUCCESS );

    while ( sRow != NULL )
    {
        if ( (sRow->flag & QMC_ROW_HIT_MASK) == QMC_ROW_HIT_TRUE )
        {
            // Hit된 Record일 경우
            break;
        }
        else
        {
            // Hit되지 않은 Reocrd일 경우 다음 Record획득
            IDE_TEST( getNextElement( aTempTable,
                                      (void**) & sRow ) != IDE_SUCCESS );
        }
    }

    *aRow = (void*) sRow;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmcMemSort::getFirstNonHit( qmcdMemSortTemp * aTempTable,
                            void           ** aRow )
{
/***********************************************************************
 *
 * Description :
 *    Hit되지 않은 최초 Record 검색
 *
 * Implementation :
 *    최초 위치로 이동하여, Hit되지 않은 Record를 찾을 때까지
 *    반복 검색함.
 *
 ***********************************************************************/

    qmcMemSortElement * sRow;

    IDE_TEST( beforeFirstElement( aTempTable ) != IDE_SUCCESS );

    IDE_TEST( getNextElement( aTempTable, (void**) & sRow ) != IDE_SUCCESS );

    while ( sRow != NULL )
    {
        if ( (sRow->flag & QMC_ROW_HIT_MASK) == QMC_ROW_HIT_FALSE )
        {
            // Hit 되지 않은 Record일 경우
            break;
        }
        else
        {
            // Hit 된 Reocrd일 경우 다음 Record획득
            IDE_TEST( getNextElement( aTempTable,
                                      (void**) & sRow ) != IDE_SUCCESS );
        }
    }

    *aRow = (void*) sRow;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmcMemSort::getNextNonHit( qmcdMemSortTemp * aTempTable,
                           void           ** aRow )
{
/***********************************************************************
 *
 * Description :
 *    Hit되지 않은 다음 Record 검색
 *
 * Implementation :
 *    Hit되지 않은 Record를 찾을 때까지 반복 검색함.
 *
 ***********************************************************************/

    qmcMemSortElement * sRow;

    IDE_TEST( getNextElement( aTempTable, (void**) & sRow ) != IDE_SUCCESS );

    while ( sRow != NULL )
    {
        if ( (sRow->flag & QMC_ROW_HIT_MASK) == QMC_ROW_HIT_FALSE )
        {
            // Hit 되지 않은 Record일 경우
            break;
        }
        else
        {
            // Hit 된 Reocrd일 경우 다음 Record획득
            IDE_TEST( getNextElement( aTempTable,
                                      (void**) & sRow ) != IDE_SUCCESS );
        }
    }

    *aRow = (void*) sRow;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmcMemSort::getNumOfElements( qmcdMemSortTemp * aTempTable,
                              SLong           * aNumElements )
{
/***********************************************************************
 *
 * Description :
 *    저장되어 있는 Record의 개수를 획득한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    IDE_TEST_RAISE( aTempTable == NULL,
                    ERR_INVALID_ARGUMENT );
    
    *aNumElements = aTempTable->mArray->index;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_ARGUMENT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmcMemSort::getNumOfElements",
                                  "aTempTable is NULL" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmcMemSort::getCurrPosition( qmcdMemSortTemp * aTempTable,
                             SLong           * aPosition )
{
/***********************************************************************
 *
 * Description :
 *    현재 검색중인 Record의 위치값을 획득한다.
 *    Merge Join에서 Cursor 위치를 저장하기 위하여 사용함.
 *
 * Implementation :
 *    현재 검색 위치를 리턴함.
 *
 ***********************************************************************/

    // 유효성 검사
    IDE_DASSERT( aTempTable->mIndex >= 0 &&
                 aTempTable->mIndex < aTempTable->mArray->index );

    *aPosition = aTempTable->mIndex;

    return IDE_SUCCESS;
}

IDE_RC
qmcMemSort::setCurrPosition( qmcdMemSortTemp * aTempTable,
                             SLong             aPosition )
{
/***********************************************************************
 *
 * Description :
 *    지정된 위치로 Cursor를 옮김
 *    Merge Join에서 Cursor 위치를 재지정 하기 위하여 사용함.
 *
 * Implementation :
 *    지정된 위치로 index값을 변경시킴.
 *
 ***********************************************************************/

    // 유효성 검사
    IDE_DASSERT( aPosition >= 0 &&
                 aPosition < aTempTable->mArray->index );

    aTempTable->mIndex = aPosition;

    return IDE_SUCCESS;
}

IDE_RC
qmcMemSort::setSortNode( qmcdMemSortTemp  * aTempTable,
                         const qmdMtrNode * aSortNode )
{
/***********************************************************************
 *
 * Description 
 *    현재 정렬키(sortNode)를 변경
 *    Window Sort(WNST)에서 정렬을 반복하기 위해 사용함.
 *
 * Implementation :
 *
 ***********************************************************************/

    aTempTable->mSortNode = (qmdMtrNode*) aSortNode;

    return IDE_SUCCESS;
}

void
qmcMemSort::get( qmcdMemSortTemp * aTempTable,
                 qmcSortArray* aArray,
                 SLong aIndex,
                 void*** aElement )
{
/***********************************************************************
 *
 * Description :
 *    지정된 위치의 Reocrd 위치를 획득한다.
 *
 * Implementation :
 *    Leaf에 해당하는 Sort Array를 찾고,
 *    Sort Array내에서 지정된 위치의 Record 위치를 획득한다.
 *
 ***********************************************************************/

    if( aArray->depth != 0 )
    {
        get( aTempTable,
             aArray->elements[aIndex>>(aArray->shift)],
             aIndex & (aArray->mask),
             aElement );
    }
    else
    {
        *aElement = (void**) (&(aArray->elements[aIndex]));
    }

}

IDE_RC qmcMemSort::increase( qmcdMemSortTemp * aTempTable,
                             qmcSortArray    * aArray )
{
/***********************************************************************
 *
 * Description :
 *    Sort Array의 저장 공간을 증가시킨다.
 *
 * Implementation :
 *    Sort Array의 저장 공간을 증가시키고,
 *    저장 공간이 없을 경우, Sort Array를 추가한다.
 *
 ***********************************************************************/

    qmcSortArray* sParent;
    SLong          sDepth;
    SLong          sReachEnd;

    IDE_TEST( increase( aTempTable,
                        aArray,
                        & sReachEnd ) != IDE_SUCCESS );

    if( sReachEnd )
    {
        IDU_LIMITPOINT("qmcMemSort::increase::malloc1");
        /* BUG-38290 */
        IDE_TEST(aTempTable->mMemoryMgr->cralloc(
                aTempTable->mMemory,
                ID_SIZEOF(qmcSortArray),
                (void**)&aTempTable->mArray)
            != IDE_SUCCESS);

        aTempTable->mArray->parent      = NULL;
        aTempTable->mArray->depth       = aArray->depth + 1;
        aTempTable->mArray->numIndex    =
            (aArray->numIndex) << QMC_MEM_SORT_SHIFT;
        aTempTable->mArray->shift       = aArray->shift + QMC_MEM_SORT_SHIFT;
        aTempTable->mArray->mask        =
            ( (aArray->mask) << QMC_MEM_SORT_SHIFT ) | QMC_MEM_SORT_MASK;
        aTempTable->mArray->index       = aArray->numIndex;
        aTempTable->mArray->elements    = aTempTable->mArray->internalElem;
        aTempTable->mArray->elements[0] = aArray;

        IDU_LIMITPOINT("qmcMemSort::increase::malloc2");
        IDE_TEST(aTempTable->mMemoryMgr->cralloc(
                aTempTable->mMemory,
                ID_SIZEOF(qmcSortArray),
                (void**)&(aTempTable->mArray->elements[1]))
            != IDE_SUCCESS);

        sParent              = aTempTable->mArray;

        aArray->parent       = sParent;
        aArray               = aTempTable->mArray->elements[1];
        aArray->parent       = sParent;
        aArray->depth        = sParent->depth - 1;
        aArray->numIndex     = (sParent->numIndex) >> QMC_MEM_SORT_SHIFT;
        aArray->shift        = sParent->shift - QMC_MEM_SORT_SHIFT;
        aArray->mask         = (sParent->mask) >> QMC_MEM_SORT_SHIFT;
        aArray->index        = 0;
        aArray->elements     = aArray->internalElem;

        for( sDepth = aArray->depth; sDepth > 0; sDepth-- )
        {
            IDU_LIMITPOINT("qmcMemSort::increase::malloc3");
            IDE_TEST(aTempTable->mMemoryMgr->cralloc(
                    aTempTable->mMemory,
                    ID_SIZEOF(qmcSortArray),
                    (void**)&(aArray->elements[0]))
                != IDE_SUCCESS);

            sParent = aArray;
            aArray  = aArray->elements[0];
            aArray->parent      = sParent;
            aArray->depth       = sParent->depth - 1;
            aArray->numIndex    = (sParent->numIndex) >> QMC_MEM_SORT_SHIFT;
            aArray->shift       = sParent->shift - QMC_MEM_SORT_SHIFT;
            aArray->mask        = (sParent->mask) >> QMC_MEM_SORT_SHIFT;
            aArray->index       = 0;
            aArray->elements    = aArray->internalElem;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmcMemSort::increase( qmcdMemSortTemp * aTempTable,
                             qmcSortArray    * aArray,
                             SLong           * aReachEnd )
{
/***********************************************************************
 *
 * Description :
 *    Sort Array의 저장 공간을 증가시킨다.
 *
 * Implementation :
 *    Sort Array의 저장 공간을 증가시키고,
 *    저장 공간이 없을 경우, Sort Array를 추가한다.
 *
 ***********************************************************************/

    qmcSortArray* sParent;
    SLong          sDepth;

    if( aArray->depth == 0 )
    {
        aArray->index++;
        if( aArray->index >= aArray->numIndex )
        {
            *aReachEnd = 1;
        }else
        {
            *aReachEnd = 0;
        }
    }
    else
    {
        IDE_TEST( increase( aTempTable,
                            aArray->elements[(aArray->index)>>(aArray->shift)],
                            aReachEnd ) != IDE_SUCCESS );
        aArray->index++;
        if( aArray->index >= aArray->numIndex )
        {
            *aReachEnd = 1;
        }
        else
        {
            if( *aReachEnd )
            {
                IDU_LIMITPOINT("qmcMemSort::increase::malloc4");
                /* BUG-38290 */
                IDE_TEST( aTempTable->mMemoryMgr->cralloc(
                        aTempTable->mMemory,
                        ID_SIZEOF(qmcSortArray),
                        (void**)&(aArray->elements[(aArray->index) >>
                                  (aArray->shift)]))
                    != IDE_SUCCESS);

                sParent = aArray;
                aArray  = aArray->elements[(aArray->index)>>(aArray->shift)];

                aArray->parent    = sParent;
                aArray->depth     = sParent->depth - 1;
                aArray->numIndex  = (sParent->numIndex) >> QMC_MEM_SORT_SHIFT;
                aArray->shift     = sParent->shift - QMC_MEM_SORT_SHIFT;
                aArray->mask      = (sParent->mask) >> QMC_MEM_SORT_SHIFT;
                aArray->index     = 0;
                aArray->elements  = aArray->internalElem;

                for( sDepth = aArray->depth; sDepth > 0; sDepth-- )
                {
                    IDU_LIMITPOINT("qmcMemSort::increase::malloc5");
                    IDE_TEST(aTempTable->
                             mMemoryMgr->cralloc( aTempTable->mMemory,
                                               ID_SIZEOF(qmcSortArray),
                                               (void**)&(aArray->elements[0]))
                             != IDE_SUCCESS);

                    sParent = aArray;
                    aArray  =
                        aArray->elements[(aArray->index)>>(aArray->shift)];

                    aArray->parent   = sParent;
                    aArray->depth    = sParent->depth - 1;
                    aArray->numIndex =
                        (sParent->numIndex) >> QMC_MEM_SORT_SHIFT;
                    aArray->shift    = sParent->shift - QMC_MEM_SORT_SHIFT;
                    aArray->mask     = (sParent->mask) >> QMC_MEM_SORT_SHIFT;
                    aArray->index    = 0;
                    aArray->elements = aArray->internalElem;
                }
                *aReachEnd = 0;
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmcMemSort::quicksort( qmcdMemSortTemp * aTempTable,
                       SLong aLow,
                       SLong aHigh )
{
/***********************************************************************
 *
 * Description :
 *    지정된 low와 high간의 Data들을 Quick Sorting한다.
 *
 * Implementation :
 *
 *    Stack Overflow가 발생하지 않도록 Stack공간을 마련한다.
 *    Partition을 분할하여 해당 Partition 단위로 Recursive하게
 *    Quick Sorting한다.
 *    자세한 내용은 일반적인 quick sorting algorithm을 참고.
 *
 ***********************************************************************/

    SLong           sLow;
    SLong           sHigh;
    SLong           sPartition;
    qmcSortStack * sSortStack = NULL;
    qmcSortStack * sStack = NULL;
    idBool          sAllEqual;
    SLong           sRecCount = aHigh - aLow + 1;

    IDU_LIMITPOINT("qmcMemSort::quicksort::malloc");
    IDE_TEST(iduMemMgr::malloc(IDU_MEM_QMC,
                               sRecCount * ID_SIZEOF(qmcSortStack),
                               (void**)&sSortStack)
             != IDE_SUCCESS);

    sStack = sSortStack;
    sStack->low = aLow;
    sStack->high = aHigh;
    sStack++;

    sLow = aLow;
    sHigh = aHigh;

    while( sStack != sSortStack )
    {
        while ( sLow < sHigh )
        {
            IDE_TEST( iduCheckSessionEvent( aTempTable->mStatistics )
                      != IDE_SUCCESS );

            sAllEqual = ID_FALSE;
            IDE_TEST( partition( aTempTable,
                                 sLow,
                                 sHigh,
                                 & sPartition,
                                 & sAllEqual ) != IDE_SUCCESS );

            if ( sAllEqual == ID_TRUE )
            {
                break;
            }
            else
            {
                sStack->low = sPartition;
                sStack->high = sHigh;
                sStack++;
                sHigh = sPartition - 1;
            }
        }

        sStack--;
        sLow = sStack->low + 1;
        sHigh = sStack->high;
    }

    IDE_TEST(iduMemMgr::free(sSortStack)
             != IDE_SUCCESS);
    // To Fix PR-12539
    // sStack = NULL;
    sSortStack = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sSortStack != NULL )
    {
        (void)iduMemMgr::free(sSortStack);
        sSortStack = NULL;
    }
    else
    {
        // nothing to do
    }

    return IDE_FAILURE;
}


IDE_RC
qmcMemSort::partition( qmcdMemSortTemp * aTempTable,
                       SLong    aLow,
                       SLong    aHigh,
                       SLong  * aPartition,
                       idBool * aAllEqual )
{
/***********************************************************************
 *
 * Description :
 *     Partition내에서의 정렬을 수행한다.
 *
 * Implementation :
 *     Partition내에서의 정렬을 수행하고,
 *     기준이 되는 Partition의 위치를 결정한다.
 *     자세한 내용은 일반적인 quick sorting algorithm 참조
 *
 ***********************************************************************/

    void ** sPivotRow = NULL;
    void ** sLowRow   = NULL;
    void ** sHighRow  = NULL;
    void  * temp      = NULL;
    SInt    sOrder    = 0;
    SLong   sLow      = aLow;
    SLong   sHigh     = aHigh + 1;
    idBool  sAllEqual = ID_TRUE;

    if ( aLow + QMC_MEM_SORT_SMALL_SIZE < aHigh )
    {
        get( aTempTable, aTempTable->mArray, (aLow + aHigh) / 2,
             &sLowRow );

        get( aTempTable, aTempTable->mArray, aLow,
             &sPivotRow );

        QMC_MEM_SORT_SWAP( *sPivotRow, *sLowRow );

        do
        {
            do
            {
                sLow++;
                if( sLow < sHigh )
                {
                    get( aTempTable, aTempTable->mArray, sLow,
                         &sLowRow );

                    sOrder = compare( aTempTable,
                                      *sPivotRow,
                                      *sLowRow);

                    if( sOrder )
                    {
                        sAllEqual = ID_FALSE;
                    }
                    else
                    {
                        // nothing to do
                    }
                }
            } while( sLow < sHigh && sOrder > 0 );

            do
            {
                sHigh--;
                get( aTempTable, aTempTable->mArray, sHigh,
                     &sHighRow );

                sOrder = compare( aTempTable,
                                  *sPivotRow,
                                  *sHighRow);
                if( sOrder )
                {
                    sAllEqual = ID_FALSE;
                }
                else
                {
                    // nothing to do
                }
            } while( sOrder < 0  );

            if( sLow < sHigh )
            {
                QMC_MEM_SORT_SWAP( *sLowRow, *sHighRow );
            }
            else
            {
                // nothing to do
            }
        } while( sLow < sHigh );

        QMC_MEM_SORT_SWAP( *sPivotRow, *sHighRow );
    }
    else
    {
        // BUG-41048 Improve orderby sort algorithm.
        // 정렬해야하는 데이터의 갯수가 QMC_MEM_SORT_SMALL_SIZE 보다 적을때는
        // insertion sort 를 이용한다.
        IDE_TEST( insertionSort( aTempTable,
                                 aLow,
                                 aHigh )
                  != IDE_SUCCESS );
    }

    *aPartition = sHigh;
    *aAllEqual = sAllEqual;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


SInt
qmcMemSort::compare( qmcdMemSortTemp * aTempTable,
                     const void      * aElem1,
                     const void      * aElem2 )
{
/***********************************************************************
 *
 * Description :
 *     Sorting시에 두 Record간의 대소 비교를 한다.
 *     ORDER BY등을 위한 정렬을 위해서는 여러 개의 Column에 대한
 *     대소 비교가 가능하지만, Range검색을 위한 정렬에서는 하나의
 *     Column에 대한 정렬만이 가능하다.
 *
 * Implementation :
 *     해당 Sort Node의 정보를 이용하여, 대소 비교를 수행한다.
 *
 ***********************************************************************/

    qmdMtrNode   * sNode;
    SInt           sResult = -1;
    mtdValueInfo   sValueInfo1;
    mtdValueInfo   sValueInfo2;
    idBool         sIsNull1;
    idBool         sIsNull2;

    for( sNode = aTempTable->mSortNode;
         sNode != NULL;
         sNode = sNode->next )
    {
        sValueInfo1.value  = sNode->func.getRow(sNode, aElem1);
        sValueInfo1.column = (const mtcColumn *)sNode->func.compareColumn;
        sValueInfo1.flag   = MTD_OFFSET_USE;

        sValueInfo2.value  = sNode->func.getRow(sNode, aElem2);
        sValueInfo2.column = (const mtcColumn *)sNode->func.compareColumn;
        sValueInfo2.flag   = MTD_OFFSET_USE;
       
        /* PROJ-2435 order by nulls first/last */
        if ( ( sNode->flag & QMC_MTR_SORT_NULLS_ORDER_MASK )
             == QMC_MTR_SORT_NULLS_ORDER_NONE )
        {
            /* NULLS first/last 가 없을경우 기존동작 수행*/
            sResult = sNode->func.compare( &sValueInfo1,
                                           &sValueInfo2 );
        }
        else
        {
            /* 1. 2개의 value의 Null 여부를 조사한다. */
            sIsNull1 = sNode->func.isNull( sNode,
                                           (void *)aElem1 );
            sIsNull2 = sNode->func.isNull( sNode,
                                           (void *)aElem2 );

            if ( sIsNull1 == sIsNull2 )
            {
                if ( sIsNull1 == ID_FALSE )
                {
                    /* 2. NULL이없을경우 기존동작 수행*/
                    sResult = sNode->func.compare( &sValueInfo1,
                                                   &sValueInfo2 );
                }
                else
                {
                    /* 3. 둘다 NULL 일 경우 0 */
                    sResult = 0;
                }
            }
            else
            {
                if ( ( sNode->flag & QMC_MTR_SORT_NULLS_ORDER_MASK )
                     == QMC_MTR_SORT_NULLS_ORDER_FIRST )
                {
                    /* 4. NULLS FIRST 일경우 Null을 최소로 한다. */
                    if ( sIsNull1 == ID_TRUE )
                    {
                        sResult = -1;
                    }
                    else
                    {
                        sResult = 1;
                    }
                }
                else
                {
                    /* 5. NULLS LAST 일경우 Null을 최대로 한다. */
                    if ( sIsNull1 == ID_TRUE )
                    {
                        sResult = 1;
                    }
                    else
                    {
                        sResult = -1;
                    }
                }
            }
        }

        if( sResult )
        {
            break;
        }
        else
        {
            continue;
        }
    }

    return sResult;
}

IDE_RC
qmcMemSort::beforeFirstElement( qmcdMemSortTemp * aTempTable )
{
/***********************************************************************
 *
 * Description :
 *    순차 검색을 위해 최초의 Record 이전의 위치로 이동한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    aTempTable->mIndex = -1;

    return IDE_SUCCESS;
}

IDE_RC
qmcMemSort::getNextElement( qmcdMemSortTemp * aTempTable,
                            void           ** aElement )
{
/***********************************************************************
 *
 * Description :
 *    다음 위치의 Record를 검색한다.
 *
 * Implementation :
 *
 *    다음 위치로 이동하여, 그 위치의 Record를 검색한다.
 *    Record가 없을 경우, NULL을 리턴한다.
 *
 ***********************************************************************/

    void** sBuffer;

    aTempTable->mIndex++;

    if( aTempTable->mIndex >= 0 &&
        aTempTable->mIndex < aTempTable->mArray->index )
    {
        get( aTempTable,
             aTempTable->mArray,
             aTempTable->mIndex,
             & sBuffer );
        *aElement = *sBuffer;
    }
    else
    {
        *aElement = NULL;
    }

    return IDE_SUCCESS;
}

void
qmcMemSort::setKeyRange( qmcdMemSortTemp * aTempTable,
                         smiRange * aRange )
{
/***********************************************************************
 *
 * Description :
 *    Key Range 검색을 위해 Range를 설정한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    aTempTable->mRange = aRange;

}

IDE_RC
qmcMemSort::getFirstKey( qmcdMemSortTemp * aTempTable,
                         void ** aElement )
{
/***********************************************************************
 *
 * Description :
 *    Range를 만족하는 최초의 Record를 검색한다.
 *
 * Implementation :
 *
 *    Range를 만족하는 Minimum Key의 위치를 찾고,
 *    있을 경우, Maximum Key의 위치를 찾는다.
 *    Minimum Key의 Record를 리턴한다.
 *
 ***********************************************************************/

    idBool sResult = ID_FALSE;

    aTempTable->mFirstKey = 0;
    aTempTable->mLastKey = aTempTable->mArray->index - 1;
    aTempTable->mCurRange = aTempTable->mRange;

    IDE_TEST( setFirstKey( aTempTable,
                           & sResult ) != IDE_SUCCESS );
    if ( sResult == ID_TRUE )
    {
        IDE_TEST( setLastKey( aTempTable ) != IDE_SUCCESS );

        aTempTable->mIndex = aTempTable->mFirstKey;
        IDE_TEST( getElement( aTempTable,
                              aTempTable->mIndex,
                              aElement ) != IDE_SUCCESS );
    }
    else
    {
        *aElement = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmcMemSort::getNextKey( qmcdMemSortTemp * aTempTable,
                        void ** aElement )
{
/***********************************************************************
 *
 * Description :
 *    Range를 만족하는 다음 Record를 검색한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    aTempTable->mIndex++;

    if ( aTempTable->mIndex > aTempTable->mLastKey )
    {
        *aElement = NULL;
    }
    else
    {
        IDE_TEST( getElement( aTempTable,
                              aTempTable->mIndex,
                              aElement ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmcMemSort::setFirstKey( qmcdMemSortTemp * aTempTable,
                         idBool          * aResult )
{
/***********************************************************************
 *
 * Description :
 *    Range를 만족하는 Minimum Key의 위치를 설정한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    SLong sMin;
    SLong sMax;
    SLong sMed;
    idBool sResult;
    idBool sFind;
    void * sElement;
    const void * sRow;
    qmdMtrNode * sNode;

    sMin = aTempTable->mFirstKey;
    sMax = aTempTable->mLastKey;

    sResult = ID_FALSE;
    sFind = ID_FALSE;

    // find range
    while ( aTempTable->mCurRange != NULL )
    {
        IDE_TEST( getElement( aTempTable,
                              sMin,
                              & sElement ) != IDE_SUCCESS );

        if( sElement == NULL )
        {
            break;
            // fix BUG-10503
            // sResult = ID_FALSE;
        }

        // PROJ-2362 memory temp 저장 효율성 개선
        for( sNode = aTempTable->mSortNode;
             sNode != NULL;
             sNode = sNode->next )
        {
            IDE_TEST( aTempTable->mSortNode->func.setTuple( aTempTable->mTemplate,
                                                            sNode,
                                                            sElement )
                      != IDE_SUCCESS );
        }
        
        sRow = aTempTable->mSortNode->func.getRow( aTempTable->mSortNode,
                                                   sElement );

        /* PROJ-2334 PMT memory variable column */
        changePartitionColumn( aTempTable->mSortNode,
                               aTempTable->mCurRange->maximum.data );
                
        IDE_TEST( aTempTable->mCurRange->maximum.callback(
                      & sResult,
                      sRow,
                      NULL,
                      0,
                      SC_NULL_GRID,
                      aTempTable->mCurRange->maximum.data )
                  != IDE_SUCCESS );
        
        if ( sResult == ID_TRUE )
        {
            break;
        }
        else
        {
            aTempTable->mCurRange = aTempTable->mCurRange->next;
        }
    }

    if ( sResult == ID_TRUE )
    {
        do
        {
            sMed = (sMin + sMax) >> 1;
            IDE_TEST( getElement ( aTempTable,
                                   sMed,
                                   & sElement ) != IDE_SUCCESS );

            // PROJ-2362 memory temp 저장 효율성 개선
            for( sNode = aTempTable->mSortNode;
                 sNode != NULL;
                 sNode = sNode->next )
            {
                IDE_TEST( aTempTable->mSortNode->func.setTuple( aTempTable->mTemplate,
                                                                sNode,
                                                                sElement )
                          != IDE_SUCCESS );
            }
        
            sRow = aTempTable->mSortNode->func.getRow( aTempTable->mSortNode,
                                                       sElement );

            /* PROJ-2334 PMT memory variable column */
            changePartitionColumn( aTempTable->mSortNode,
                                   aTempTable->mCurRange->minimum.data );
                    
            IDE_TEST( aTempTable->mCurRange->minimum.callback(
                          &sResult,
                          sRow,
                          NULL,
                          0,
                          SC_NULL_GRID,
                          aTempTable->mCurRange->minimum.data )
                      != IDE_SUCCESS );

            if ( sResult == ID_TRUE )
            {
                sMax = sMed - 1;

                /* PROJ-2334 PMT memory variable column */
                changePartitionColumn( aTempTable->mSortNode,
                                       aTempTable->mCurRange->maximum.data );

                IDE_TEST( aTempTable->mCurRange->maximum.callback(
                              &sResult,
                              sRow, 
                              NULL,
                              0,
                              SC_NULL_GRID,
                              aTempTable->mCurRange->maximum.data )
                          != IDE_SUCCESS );

                if ( sResult == ID_TRUE )
                {
                    aTempTable->mFirstKey = sMed;
                    sFind = ID_TRUE;
                }
                else
                {
                    // nothing to do
                }
            }
            else
            {
                sMin = sMed + 1;
            }
        } while ( sMin <= sMax );

        if ( sFind == ID_TRUE )
        {
            *aResult = ID_TRUE;
        }
        else
        {
            *aResult = ID_FALSE;
        }
    }
    else
    {
        *aResult = ID_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmcMemSort::setLastKey(qmcdMemSortTemp * aTempTable)
{
/***********************************************************************
 *
 * Description :
 *     Range를 만족하는 Maximum Key의 위치를 설정한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    SLong sMin;
    SLong sMax;
    SLong sMed;
    idBool sResult;
    void * sElement;
    const void * sRow;
    qmdMtrNode * sNode;

    sMin = aTempTable->mFirstKey;
    sMax = aTempTable->mLastKey;

    do
    {
        sMed = (sMin + sMax) >> 1;
        IDE_TEST( getElement ( aTempTable,
                               sMed,
                               & sElement ) != IDE_SUCCESS );

        // PROJ-2362 memory temp 저장 효율성 개선
        for( sNode = aTempTable->mSortNode;
             sNode != NULL;
             sNode = sNode->next )
        {
            IDE_TEST( aTempTable->mSortNode->func.setTuple( aTempTable->mTemplate,
                                                            sNode,
                                                            sElement )
                      != IDE_SUCCESS );
        }
        
        sRow = aTempTable->mSortNode->func.getRow( aTempTable->mSortNode,
                                                   sElement );

        /* PROJ-2334 PMT memory variable column */
        changePartitionColumn( aTempTable->mSortNode,
                               aTempTable->mCurRange->maximum.data );
        
        IDE_TEST( aTempTable->mCurRange->maximum.callback(
                      & sResult,
                      sRow, 
                      NULL,
                      0,
                      SC_NULL_GRID,
                      aTempTable->mCurRange->maximum.data )
                  != IDE_SUCCESS );

        if ( sResult == ID_TRUE )
        {
            sMin = sMed + 1;
            aTempTable->mLastKey = sMed;
        }
        else
        {
            sMax = sMed - 1;
            aTempTable->mLastKey = sMax;
        }
    } while ( sMin <= sMax );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmcMemSort::findInsertPosition( qmcdMemSortTemp * aTempTable,
                                void            * aRow,
                                SLong           * aInsertPos )
{
/***********************************************************************
 *
 * Description :
 *    Limit Sorting에서 Shift And Append를 하기 위한 위치를
 *    검색한다.
 *
 * Implementation :
 *    마지막 Row보다 왼쪽에 위치할 경우, 그 위치를 검색한다.
 *    삽입 대상이 아닐 경우 -1 값을 리턴한다.
 *
 ***********************************************************************/

    SInt sOrder;

    SLong sMin;
    SLong sMed;
    SLong sMax;
    SLong sInsertPos;

    void  ** sMedRow;
    void  ** sMaxRow;

    //------------------------------------------
    // 가장 우측 Row와의 비교
    //------------------------------------------

    sMax = aTempTable->mArray->index - 1;
    get( aTempTable, aTempTable->mArray, sMax, & sMaxRow );

    sOrder = compare( aTempTable, aRow, *sMaxRow );

    //------------------------------------------
    // 삽입할 위치 결정
    //------------------------------------------

    sInsertPos = -1;

    if ( sOrder < 0 )
    {
        // 삽입할 위치를 검색함.
        sMin = 0;
        do
        {
            sMed = (sMin + sMax) >> 1;
            get( aTempTable, aTempTable->mArray, sMed, & sMedRow );

            sOrder = compare( aTempTable, aRow, *sMedRow );
            if ( sOrder <= 0 )
            {
                sInsertPos = sMed;
                sMax = sMed - 1;
            }
            else
            {
                sMin = sMed + 1;
            }
        } while ( sMin <= sMax );

        IDE_DASSERT( sInsertPos != -1 );
    }
    else
    {
        // Right-Most Row보다 우측에 있는 Row임
        // 삽입 대상이 아님을 표현하게 됨
    }

    *aInsertPos = sInsertPos;

    return IDE_SUCCESS;
}

/***********************************************************************
 *
 * Description : Proj-2334 PMT memory variable column
 *
 * Implementation :
 *     1. MEMORY PARTITIONED TABLE 쳬크, MEMORY VARIABLE 쳬크.
 *     2. columnDesc는 partition의 column이기 때문에 srcTuple column과 같은
 *        colum id를 찾는다.
 *        (sortNode의 srcTuple의 column은 partition의 column이다.
 *         PCRD에서 srcTuple의 columns, row정보가 partition 정보로 변경 되기 때문)
 *     3. columnDesc column id == srcTuple column id 동일 하면 복사.
 *     
 ***********************************************************************/
void qmcMemSort::changePartitionColumn( qmdMtrNode  * aNode,
                                        void        * aData )
{
    mtkRangeCallBack  * sData;
    mtcColumn         * sColumn;
    qmdMtrNode        * sNode;

    if ( ( ( aNode->srcTuple->lflag & MTC_TUPLE_PARTITIONED_TABLE_MASK )
           == MTC_TUPLE_PARTITIONED_TABLE_TRUE ) ||
         ( ( aNode->srcTuple->lflag & MTC_TUPLE_PARTITION_MASK )
           == MTC_TUPLE_PARTITION_TRUE ) )
    {
        for ( sData  = (mtkRangeCallBack*)aData;
              sData != NULL;
              sData  = sData->next )
        {
            /* BUG-39957 null range는 columnDesc가 설정되어있지 않다. */
            if ( sData->compare != mtk::compareMinimumLimit )
            {
                for ( sNode = aNode;
                      sNode != NULL;
                      sNode = sNode->next )
                {
                    sColumn = sNode->func.compareColumn;

                    if ( ( ( (sColumn->column.flag & SMI_COLUMN_TYPE_MASK)
                             == SMI_COLUMN_TYPE_VARIABLE ) ||
                           ( (sColumn->column.flag & SMI_COLUMN_TYPE_MASK)
                             == SMI_COLUMN_TYPE_VARIABLE_LARGE ) )&&
                         ( (sNode->flag & QMC_MTR_TYPE_MASK)
                           == QMC_MTR_TYPE_MEMORY_PARTITION_KEY_COLUMN ) )
                    {
                        IDE_DASSERT( sData->columnDesc.column.id == sColumn->column.id );

                        if ( sData->columnDesc.column.colSpace != sColumn->column.colSpace )
                        {
                            /*
                              idlOS::memcpy( &sData->columnDesc.column,
                              &sColumn->column,
                              ID_SIZEOF(smiColumn) );
                            */

                            /* memcpy 대신 colSpace만 바꾼다. */
                            sData->columnDesc.column.colSpace = sColumn->column.colSpace;
                        }
                        else
                        {
                            /* Nothing To Do */
                        }
                    }
                    else
                    {
                        /* Nothing To Do */
                    }
                }
            }
            else
            {
                /* Nothing To Do */
            }
        }
    }
    else
    {
        /* Nothing To Do */
    }

}

IDE_RC qmcMemSort::timsort( qmcdMemSortTemp * aTempTable,
                            SLong             aLow,
                            SLong             aHigh )
{
/****************************************************************************
 *
 * Description : TASK-6445
 *
 *  Timsort (Insertion Sort + Mergesort) 를 수행한다.
 *
 *  1. 배열에서 Run을 찾아, Run Stack에 삽입
 *     1-a. 배열에서 Natural Run이 있다면, 그대로 Run으로 설정
 *     1-b. 배열에서 Natural Run이 없다면, MIN(minrun, 남은 Record 개수) 만큼의
 *          부분 배열을 Insertion Sort로 강제 정렬해 Run 생성
 *
 *  2. Run Stack 유지 조건을 만족하는지 확인
 *     2-a. 유지조건을 만족하면, (1)로 돌아가서 계속 Run을 탐색
 *     2-b. 유지조건을 만족하지 않으면, 만족할 때 까지 Stack의 Run들을 Mergesort
 *
 *  자세한 내용은 NoK Page (http://nok.altibase.com/x/qaMRAg) 를 참고한다.
 *
 ****************************************************************************/

    qmcSortRun   * sCurrRun       = NULL;
    qmcSortArray * sTempArray     = NULL;
    SLong          sMinRun        = 0;
    SLong          sRecCount      = aHigh - aLow + 1;
    SLong          sStartIdx      = 0;
    SLong          sLength        = 0;
    idBool         sIsReverseRun  = ID_FALSE;
    UShort         sState         = 0;

    //--------------------------------------------------------------
    // minrun (Run이 될 최소 길이) 계산
    //--------------------------------------------------------------
    sMinRun = calcMinrun( sRecCount );

    //--------------------------------------------------------------
    // Run Stack 영역 할당
    // Run은 최대 (N/minrun) + 1 까지 생성될 수 있다.
    //--------------------------------------------------------------
    IDU_FIT_POINT( "qmcMemSort::timsort::malloc",
                   idERR_ABORT_InsufficientMemory );
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_QMC,
                                 ( ( sRecCount / sMinRun ) + 1 ) * ID_SIZEOF(qmcSortRun),
                                 (void**) & aTempTable->mRunStack )
              != IDE_SUCCESS );
    sState = 1;

    //-------------------------------------------------
    // 임시 영역을 위한 Temp Array를 만든다.
    // 사용하는 임시 영역은 (N/2) 를 넘을 수 없다.
    //-------------------------------------------------
    IDU_FIT_POINT( "qmcMemSort::timsort::malloc2",
                   idERR_ABORT_InsufficientMemory );
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_QMC,
                                 ID_SIZEOF(qmcSortArray),
                                 (void**) & sTempArray )
              != IDE_SUCCESS );
    sState = 2;

    sTempArray->parent   = NULL;
    sTempArray->depth    = 0;
    sTempArray->numIndex = sRecCount / 2;
    sTempArray->shift    = 0;
    sTempArray->mask     = 0;
    sTempArray->index    = 0;

    IDU_FIT_POINT( "qmcMemSort::timsort::malloc3",
                   idERR_ABORT_InsufficientMemory );
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_QMC,
                                 sTempArray->numIndex * ID_SIZEOF(void*),
                                 (void**) & sTempArray->elements )
              != IDE_SUCCESS );
    sState = 3;

    // Temp Table에 설정
    aTempTable->mTempArray = sTempArray;

    //--------------------------------------------------------------
    // Run 탐색 & Merging
    //--------------------------------------------------------------
    sStartIdx = aLow;

    while ( sRecCount > 0 ) 
    {
        // Run 탐색
        searchRun( aTempTable,        // Array
                   sStartIdx,         // Start Index
                   aHigh + 1,         // Fence
                   & sLength,         // Returned Length
                   & sIsReverseRun ); // Returned Order

        if ( sLength < sMinRun )
        {
            // 탐색한 Run이 minrun보다 작은 경우
            // 남아있는 레코드 개수 vs. minrun 값 중 작은 값을 Length로 선택
            sLength = IDL_MIN( sRecCount, sMinRun );

            // Run 생성
            IDE_TEST( insertionSort( aTempTable,
                                     sStartIdx,
                                     sStartIdx + sLength - 1 )
                      != IDE_SUCCESS );
        }
        else
        {
            // 탐색한 Run이 충분히 긴 경우
            if ( sIsReverseRun == ID_TRUE )
            {
                // 원치 않는 순서로 된 Run인 경우 뒤집는다.
                reverseOrder( aTempTable, 
                              sStartIdx,                 // aLow
                              sStartIdx + sLength - 1 ); // aHigh (not fence)
            }
            else
            {
                // 원하는 순서대로 Run가 형성된 경우
                // Noting to do.
            }
        }

        // [ sStartIdx, sStartIdx+sLength ) 인 Run을 Stack에 넣기
        sCurrRun = & aTempTable->mRunStack[aTempTable->mRunStackSize];
        sCurrRun->mStart  = sStartIdx;
        sCurrRun->mEnd    = sStartIdx + sLength - 1;
        sCurrRun->mLength = sLength;

        // Run Stack Size 증가
        aTempTable->mRunStackSize++;

        // 다음 Run 탐색 준비
        sStartIdx += sLength;
        sRecCount -= sLength;

        //----------------------------------------------------------
        // Stack 유지 조건을 검사하고, 조건을 만족할 때 까지 Merge
        //----------------------------------------------------------
        IDE_TEST( collapseStack( aTempTable ) != IDE_SUCCESS );
    }

    //--------------------------------------------------------------
    // Stack에 남아있는 Run들을 강제로 Merging
    //--------------------------------------------------------------
    IDE_TEST( collapseStackForce( aTempTable ) != IDE_SUCCESS );

    // 이후에는 Stack에 Run이 하나만 들어 있어야 한다.
    IDE_DASSERT( aTempTable->mRunStackSize == 1 );

    //--------------------------------------------------------------
    // 임시 영역 초기화
    //--------------------------------------------------------------
    sState = 2;
    IDE_TEST( iduMemMgr::free( sTempArray->elements ) != IDE_SUCCESS );
    sTempArray->elements = NULL;

    sState = 1;
    IDE_TEST( iduMemMgr::free( sTempArray ) != IDE_SUCCESS );
    sTempArray = NULL;

    // Temp Table 설정
    aTempTable->mTempArray = NULL;

    //--------------------------------------------------------------
    // RunStack 해제 / RunStack Size 초기화
    //--------------------------------------------------------------
    sState = 0;
    IDE_TEST( iduMemMgr::free( aTempTable->mRunStack ) != IDE_SUCCESS );
    aTempTable->mRunStack     = NULL;
    aTempTable->mRunStackSize = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch ( sState )
    {
        case 3:
            (void)iduMemMgr::free( sTempArray->elements );
            sTempArray->elements = NULL;
            /* fall through */
        case 2:
            (void)iduMemMgr::free( sTempArray );
            sTempArray = NULL;
            /* fall through */
        case 1:
            (void)iduMemMgr::free( aTempTable->mRunStack );
            /* fall through */
        default:
            // free() 여부에 관계없이 Temp Table 변수는 초기화해야 한다.
            aTempTable->mRunStack     = NULL;
            aTempTable->mRunStackSize = 0;
            aTempTable->mTempArray    = NULL;
            break;
    }

    return IDE_FAILURE;
}

SLong qmcMemSort::calcMinrun( SLong aLength )
{
/****************************************************************************
 *
 * Description : TASK-6445
 *
 *  Timsort에 필요한 Minimum Run Length (minrun)를 계산한다.
 *  범위는 MINRUN_FENCE/2 <= sMinRun <= MINRUN_FENCE 이다.
 *
 *  기본적으로, 전체 배열을 minrun으로 나눌 때
 *  Run 개수가 2의 거듭제곱수가 되는 minrun을 계산한다.
 *  불가능한 경우, Run 개수가 2의 거듭제곱수에 가깝지만
 *  거듭제곱수보다는 모자라게 만드는 minrun을 찾는다.
 *
 *  이렇게 해야, 마지막 Run의 크기가 minrun에 가까워져서
 *  최종 Merge 단계에서 동일하거나 비슷한 크기의 Run 끼리 진행되기 때문이다.
 *  Mergesort 에서는 두 Run의 길이 차이가 많이 날 수록 성능 저하가 생긴다.
 *
 *  자세한 계산 기준과 공식 성립 과정에 대한 자세한 내용은
 *  NoK Page (http://nok.altibase.com/x/qaMRAg) 를 참고한다.
 *
 * Implementation : 
 *
 *  MINRUN_FENCE가 2^N인 경우, sQuotient와 sRemain을 아래와 같이 계산해
 *  두 값을 더하면 minrun이 된다.
 *
 *  - sQuotient = 가장 왼쪽 N Bit
 *  - sRemain   = 나머지 Bit가 존재하면 1, 아니면 0
 *
 *  (e.g.) 2174 = 100001111110(2)
 *                | Q  || Re |
 *                  33 + 1    = 34
 *
 *  2174/34 = 63 2174%34 = 33 
 *  (즉, 길이 34인 Run이 63개, 32인 Run 1개 생성)
 *
 ****************************************************************************/

    SLong sQuotient = aLength;
    SLong sRemain   = 0;

    IDE_DASSERT( sQuotient > 0 );

    while ( sQuotient >= QMC_MEM_SORT_MINRUN_FENCE ) 
    {
        sRemain |= ( sQuotient & 1 );
        sQuotient >>= 1;
    }

    return sQuotient + sRemain;
}

void qmcMemSort::searchRun( qmcdMemSortTemp * aTempTable,
                            SLong             aStart,
                            SLong             aFence,
                            SLong           * aRunLength,
                            idBool          * aReverseOrder )
{
/****************************************************************************
 *
 * Description : TASK-6445
 *
 *  원하는 방향으로 미리 정렬된 Natural Run을 찾는다.
 *  하지만 원하는 순서와 반대 방향인 Run도 Natural Run으로 간주한다.
 *
 *  Run을 찾는 과정에서 추가 비용이 들지만, 사실은 그렇지 않다.
 *  의미있는 Natural Run을 찾아 낸다면 Insertion Sort 비용이 없어지기 때문이다.
 *
 ****************************************************************************/

    void  ** sCurrRow   = NULL;
    void  ** sNextRow   = NULL;
    SLong    sNext      = aStart + 1;

    IDE_DASSERT( aStart < aFence );

    if ( sNext == aFence )
    {
        // Run Length는 1
        *aRunLength = 1;
    }
    else
    {
        get( aTempTable, aTempTable->mArray, sNext - 1, & sCurrRow );
        get( aTempTable, aTempTable->mArray, sNext, & sNextRow );

        // 아래 if-else 구문을 합치게 되면 불필요한 코드가 줄어들 수 있지만,
        // for문 안에서 Reverse 여부를 항상 체크하는 것은 실행 측면에서 비효율적이다.
        if ( compare( aTempTable, *sNextRow, *sCurrRow ) >= 0 )
        {
            // 원했던 순서
            *aReverseOrder = ID_FALSE;
            
            // 계속 이 순서로 탐색한다.
            for ( sNext = sNext + 1; sNext < aFence; sNext++ )
            {
                get( aTempTable, aTempTable->mArray, sNext - 1, & sCurrRow );
                get( aTempTable, aTempTable->mArray, sNext, & sNextRow );

                if ( compare( aTempTable, *sNextRow, *sCurrRow ) >= 0 )
                {
                    // Nothing to do.
                }
                else
                {
                    // 현재 순서와 반대 방향이 되므로 빠져 나온다.
                    break;
                }
            }
        }
        else
        {
            // 반대 순서
            *aReverseOrder = ID_TRUE;

            // 계속 이 순서로 탐색한다.
            for ( sNext = sNext + 1; sNext < aFence; sNext++ )
            {
                get( aTempTable, aTempTable->mArray, sNext - 1, & sCurrRow );
                get( aTempTable, aTempTable->mArray, sNext, & sNextRow );

                if ( compare( aTempTable, *sNextRow, *sCurrRow ) > 0 )
                {
                    // 현재 순서와 반대 방향이 되므로 빠져 나온다.
                    break;
                }
                else
                {
                    // Nothing to do.
                }
            }
        }

        // Run Length 지정
        *aRunLength = sNext - aStart;
    }

}

IDE_RC qmcMemSort::insertionSort( qmcdMemSortTemp * aTempTable,
                                  SLong             aLow, 
                                  SLong             aHigh )
{
/****************************************************************************
 *
 * Description : 
 *
 *  [aLow, aHigh] 까지 Insertion Sort로 정렬한다.
 *  2개의 알고리즘 모두, 다음의 경우에 한해 이 함수를 필요로 한다.
 *
 *  - Quicksort에서는 Partition의 길이가 QMC_MEM_SORT_SMALL_SIZE 보다 작은 경우,
 *    Partitioning을 하는 것보다 직접 정렬하는 것이 더 낫다.
 *
 *  - Timsort에서는 Natural Run 길이가 minrun보다 적거나 Natural Run이 없을 경우,
 *    minrun 길이만큼 정렬시켜 Run을 생성해야 한다.
 *
 ****************************************************************************/

    void  ** sLowRow   = NULL;
    void  ** sHighRow  = NULL;
    void  ** sPivotRow = NULL;
    void   * sTempRow  = NULL;
    SLong    sStart    = 0;
    SLong    sTemp     = 0;
    SInt     sOrder    = 0;

    for ( sStart = aLow + 1; sStart <= aHigh; sStart++ )
    {
        get( aTempTable, aTempTable->mArray, sStart, &sPivotRow );

        sTempRow = *sPivotRow;

        for ( sTemp = sStart - 1; sTemp >= aLow; --sTemp )
        {
            get( aTempTable, aTempTable->mArray, sTemp, &sLowRow );
            get( aTempTable, aTempTable->mArray, sTemp + 1, &sHighRow );
            sOrder = compare( aTempTable, *sLowRow, sTempRow ); 

            if ( sOrder > 0 )
            {
                *sHighRow = *sLowRow;
            }
            else
            {
                break;
            }
        }
        get( aTempTable, aTempTable->mArray, sTemp + 1, &sHighRow );
        
        *sHighRow = sTempRow;
    }
    
    return IDE_SUCCESS;
}

void qmcMemSort::reverseOrder( qmcdMemSortTemp * aTempTable,
                               SLong             aLow, 
                               SLong             aHigh )
{
/****************************************************************************
 *
 * Description : TASK-6445
 *
 *  [aLow, aHigh] 의 정렬 순서를 바꾼다.
 *  원하는 방향과 역방향인 Run을 대상으로 호출한다.
 *
 * Implementation :
 *
 *  양 끝단부터 시작해 중심으로 오면서, 서로의 값을 바꿔나간다.
 *
 ****************************************************************************/

    void  ** sLowRow  = NULL;
    void  ** sHighRow = NULL;
    void   * temp     = NULL;
    SLong    sLow     = aLow;
    SLong    sHigh    = aHigh;

    while ( sLow < sHigh )
    {
        get( aTempTable, aTempTable->mArray, sLow, & sLowRow );
        get( aTempTable, aTempTable->mArray, sHigh, & sHighRow );
        QMC_MEM_SORT_SWAP( *sLowRow, *sHighRow );

        sLow++;
        sHigh--;
    }

}

IDE_RC qmcMemSort::collapseStack( qmcdMemSortTemp * aTempTable )
{
/****************************************************************************
 *
 * Description : TASK-6445
 *
 *  Run Stack에 있는 Run들이 Merge 가능한지 검사한다.
 *
 *  Stack의 마지막 3개 ( 삽입 시점 순서대로 A, B, C )가 A <= B + C 를 만족할 때
 *   - A <  C --> A와 B를 Merge
 *   - A >= C --> B와 C를 Merge
 *
 *  이 외의 경우 ( Stack에 Run이 2개, 또는 A > B + C )에
 *   - B <= C --> B와 C를 Merge
 *
 * Implementation :  
 *
 *  기준점을 B로 정한다.
 *  mergeRuns() 은 '기준점'과 '기준점 다음 Run'을 Merge 시킨다.
 *
 *  마지막 3개가 존재하는 상황에서 A <= B + C 라면,
 *  A, C의 대소비교를 한 다음 작은 쪽으로 Merge한다.
 *
 *   - A >= C : B와 C를 한다. ( 기준점 B 그대로 mergeRuns() )
 *   - A <  C : A와 B를 한다. ( 기준점 B에서 A로 옮긴 뒤 mergeRuns() )
 *
 *  2개가 남았거나 위 조건을 만족하지 않으면,
 *  B <= C인 상황에서만 기준점을 B 그대로 두고 mergeRuns() 을 호출한다.
 *
 * Note :
 *  
 *  Merge 결과 Run과 아직 남아있는 인접한 Run의 크기 차이가
 *  가장 작게 일어나는 순서를 선택하는 것이다. ( 예 : 30, 20, 10 => 30, 30 )
 *  
 *  Merge 조건에 대한 자세한 설명은
 *  NoK Page (http://nok.altibase.com/x/qaMRAg) 를 참고한다.
 *
 ****************************************************************************/

    qmcSortRun * sRunStack;
    SInt         sRunStackIdx = 0;

    sRunStack    = aTempTable->mRunStack;

    while ( aTempTable->mRunStackSize > 1 )
    {
        // 최근 3개의 Run을 R[n-1], R[n], R[n+1] 로 본다
        sRunStackIdx = aTempTable->mRunStackSize - 2;

        IDE_DASSERT( sRunStackIdx >= 0 );

        // 3개 모두 존재하는 경우
        if ( sRunStackIdx > 0 )
        {
            // R[n-1].length <= R[n].length + R[n+1].length을 만족하면
            if ( sRunStack[sRunStackIdx - 1].mLength <=
                 ( sRunStack[sRunStackIdx].mLength + sRunStack[sRunStackIdx + 1].mLength ) )
            {
                // R[n-1].length < R[n+1].length
                if ( sRunStack[sRunStackIdx - 1].mLength < sRunStack[sRunStackIdx + 1].mLength )
                {
                    // Merge 기준 인덱스를 옮겨서 R[n-1]과 R[n]을 Merge
                    sRunStackIdx--;
                }
                else
                {
                    // Merge 기준 인덱스를 옮기지 않고 R[n]과 R[n+1]을 Merge
                }

                // merge
                IDE_TEST( mergeRuns( aTempTable, sRunStackIdx ) != IDE_SUCCESS );

                // while문 조건을 다시 검사하게 한다.
                continue;
            }
            else
            {
                // 조건 성립이 안 되는 경우, 아래 조건을 검사한다.
                // Nothing to do.
            }
        }
        else
        {
            // Run이 2개 이하인 경우, 아래 조건을 검사한다.
            // Nothing to do.
        }

        // R[n].length <= R[n+1].length
        if ( sRunStack[sRunStackIdx].mLength <= sRunStack[sRunStackIdx + 1].mLength )
        {
            // merge
            IDE_TEST( mergeRuns( aTempTable, sRunStackIdx ) != IDE_SUCCESS );
        }
        else
        {
            // Merge를 종료하고, Stack을 유지한다.
            break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmcMemSort::collapseStackForce( qmcdMemSortTemp * aTempTable )
{
/****************************************************************************
 *
 * Description : TASK-6445
 *
 *  Timsort Run Stack에 있는 Run들을 강제로 Merge한다.
 *  규칙은 동일하나, 조금 간소화 된 형태이다.
 *
 * Implementation :
 *
 *  while() 문 안에서 1개가 남을 때 가지 아래 mergeRuns()가 호출된다.
 *  3개가 남은 상황에서는 A, C의 대소비교를 한 다음 작은 쪽으로 Merge한다.
 *   - A >= C : B와 C를 한다. ( 기준점 B 그대로 mergeRuns() )
 *   - A <  C : A와 B를 한다. ( 기준점 B에서 A로 옮긴 뒤 mergeRuns() )
 *
 *  2개가 남은 상황에서는 2개를 곧바로 Merge한다.
 *
 ****************************************************************************/

    qmcSortRun * sRunStack;
    SInt         sRunStackIdx = 0;

    sRunStack    = aTempTable->mRunStack;

    while ( aTempTable->mRunStackSize > 1 )
    {
        // 직전 3개의 Run을 R[n-1], R[n], R[n+1] 로 본다
        sRunStackIdx = aTempTable->mRunStackSize - 2;

        IDE_DASSERT( sRunStackIdx >= 0 );

        if ( ( sRunStackIdx > 0 ) &&
             ( sRunStack[sRunStackIdx - 1].mLength < sRunStack[sRunStackIdx + 1].mLength ) )
        {
                sRunStackIdx--;
        }
        else
        {
            // Nothing to do.
        }

        // merge
        IDE_TEST( mergeRuns( aTempTable, sRunStackIdx ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmcMemSort::mergeRuns( qmcdMemSortTemp * aTempTable,
                              UInt              aRunStackIdx )
{
/****************************************************************************
 *
 * Description : TASK-6445
 *
 *  두 개의 Run (A)와 Run (B) 간 Mergesort를 수행한다.
 *
 * Implementation : 
 *
 *  Mergesort 전에, 두 Run에서 정렬할 필요가 없는 부분을 건너뛰도록 한다.
 *
 *  Mergesort에 참여하지 않고 그 자리에 계속 있을 수 있는 원소는,
 *  (오름차순 기준으로)
 *
 *  - A의 가장 큰 수 (= 가장 오른쪽의 수)보다 더 크거나 같은 B의 원소들
 *  - B의 가장 작은 수 (=가장 왼쪽의 수)보다 더 작거나 같은 A의 원소들
 *
 *    [A]                       [B]
 *                     (max)    (min)
 *    ----------------------    -----------------------
 *    | 1 | 3 | 5 | 7 | 10 |    | 4 | 6 | 8 | 10 | 12 |
 *    ----------------------    -----------------------
 *          ^                                 ^
 *          여기까지 4보다 작음               여기부터 10보다 크거나 같음
 *
 *     (gallopRight->) [A']           [B']           (<-gallopLeft)
 *     ---------       -------------- -------------  -----------
 *  => | 1 | 3 |       | 5 | 7 | 10 | | 4 | 6 | 8 |  | 10 | 12 |
 *     ---------       -------------- -------------  -----------
 *
 *  => (A'와 B'만 Mergesort에 참여)
 *
 ****************************************************************************/

    void       ** sGallopRow = NULL;
    qmcSortRun  * sRun1;
    qmcSortRun  * sRun2;
    qmcSortRun  * sRun3      = NULL;
    SLong         sBase1;
    SLong         sLen1;
    SLong         sBase2;
    SLong         sLen2;
    SLong         sGallopCnt = 0;

    sRun1  = & aTempTable->mRunStack[aRunStackIdx];
    sRun2  = & aTempTable->mRunStack[aRunStackIdx + 1];
    sBase1 = sRun1->mStart;
    sLen1  = sRun1->mLength;
    sBase2 = sRun2->mStart;
    sLen2  = sRun2->mLength;

    // Stack 정보 갱신
    sRun1->mLength = sLen1 + sLen2;

    // A가 Stack의 (끝에서) 3번째 Run이었다면
    // 중간에서 Merge가 일어나는 것이므로 마지막 Run (C) 정보를 보존해야 한다.
    // B의 정보에 C의 정보를 덮어 쓴다. (B는 사용 완료됨)
    if ( aRunStackIdx == aTempTable->mRunStackSize - 3 )
    {
        sRun3 = & aTempTable->mRunStack[aRunStackIdx + 2];
        sRun2->mStart  = sRun3->mStart;
        sRun2->mEnd    = sRun3->mEnd;
        sRun2->mLength = sRun3->mLength;
    }
    else
    {
        // Nothing to do.
    }

    // RunStack Size 감소
    aTempTable->mRunStackSize--;
    
    //-------------------------------------------------------
    // Trimming
    //-------------------------------------------------------

    // Right Galloping
    get( aTempTable, 
         aTempTable->mArray, 
         sBase2, 
         & sGallopRow );

    sGallopCnt = gallopRight( aTempTable, 
                              aTempTable->mArray,
                              sGallopRow, 
                              sBase1, 
                              sLen1, 
                              0 );

    sBase1 += sGallopCnt;
    sLen1  -= sGallopCnt;
    IDE_TEST_CONT( sLen1 == 0, NO_MORE_MERGE );

    // Left Galloping
    get( aTempTable, 
         aTempTable->mArray, 
         ( sBase1 + ( sLen1 - 1 ) ), 
         & sGallopRow );

    sLen2 = gallopLeft( aTempTable, 
                        aTempTable->mArray,
                        sGallopRow, 
                        sBase2, 
                        sLen2, 
                        sLen2 - 1 );

    IDE_TEST_CONT( sLen2 == 0, NO_MORE_MERGE );

    //-------------------------------------------------------
    // Merging
    //-------------------------------------------------------

    if ( sLen1 <= sLen2 )
    {
        IDE_TEST( mergeLowerRun( aTempTable,
                                 sBase1,
                                 sLen1,
                                 sBase2,
                                 sLen2 ) 
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( mergeHigherRun( aTempTable,
                                  sBase1,
                                  sLen1,
                                  sBase2,
                                  sLen2 ) 
                  != IDE_SUCCESS );
    }

    IDE_EXCEPTION_CONT( NO_MORE_MERGE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

SLong qmcMemSort::gallopLeft( qmcdMemSortTemp  * aTempTable,
                              qmcSortArray     * aArray,
                              void            ** aKey,
                              SLong              aBase,
                              SLong              aLength,
                              SLong              aHint )
{
/****************************************************************************
 *
 * Desciption : TASK-6445
 *
 *  Run의 [aBase, aBase+aLength) 범위에서,
 *  정렬 순서 상 aKey가 위치해야 하는 인덱스를 반환한다.
 *  이 인덱스는 aBase에서의 상대적 위치로 반환된다.
 *  
 *  만약 오름차순이라면
 *  a[aBase+Ret-1] < key <= a[aBase+Ret] 을 만족하는 Ret을 반환한다.
 *
 *  여기서 Left는 '왼쪽 방향' 이란 의미도 있지만,
 *  같은 값이 존재하는 경우 가장 '왼쪽'에 위치하는 인덱스를 구하라는 의미이다.
 *
 * Implementation :
 *
 *  힌트(Hint) 위치부터 시작한다.
 *  Galloping 할 때는, 1칸씩 건너뛰지않고, 2^n-1칸씩 건너 뛴다.
 *
 *   - a[Hint] -> Key 순서가 순방향이면,
 *     다음을 만족할 때 까지 오른쪽으로 Galloping
 *     > ASC  : if ( a[Hint] < Key ) searchCond = ( Key <= a[Hint + Offset] )
 *     > DESC : if ( a[Hint] > Key ) searchCond = ( Key >= a[Hint + Offset] )
 *
 *   - a[Hint] -> Key 순서가 '같은 값이거나' 역방향인 경우,
 *     다음을 만족할 때 까지 왼쪽으로 Galloping
 *     > ASC  : if ( a[Hint] >= Key ) searchCond = ( a[Hint - Offset] < Key )
 *     > DESC : if ( a[Hint] <= Key ) searchCond = ( a[Hint - Offset] > Key )
 *
 *  Galloping이 완료되면, (직전 Offset - 현재 Offset) 범위 안에서
 *  Binary Search를 해서 정확한 위치를 찾는다.
 *
 *  자세한 내용은 NoK 문서를 참고 (http://nok.altibase.com/x/qaMRAg)
 *
 ****************************************************************************/

    void ** sGallopRow = NULL;
    SLong   sStartIdx  = aBase + aHint;
    SLong   sIdx       = 0;
    SLong   sLastIdx   = 0;
    SLong   sMaxIdx    = 0;
    SLong   sTempIdx   = 0;

    get( aTempTable, aArray, sStartIdx, & sGallopRow );

    if ( compare( aTempTable, *aKey, *sGallopRow ) > 0 )
    {
        // a[Hint] -> Key 순서가 순방향 => 오른쪽으로 Galloping
        // sMaxIdx / sIdx / sLastIdx는 sStartIdx에서의 상대 인덱스이다.
        sMaxIdx  = aLength - aHint;
        sLastIdx = 0;
        sIdx     = 1;

        while ( sIdx < sMaxIdx )
        {
            get( aTempTable, aArray, ( sStartIdx + sIdx ), & sGallopRow );

            if ( compare( aTempTable, *aKey, *sGallopRow ) > 0 )
            {
                // 계속 순서가 순방향
                sLastIdx = sIdx;

                // 2^n-1 만큼 Index 증가
                sIdx = ( sIdx << 1 ) + 1;

                // overflow 처리
                if ( sIdx <= 0 )
                {
                    sIdx = sMaxIdx;
                }
                else
                {
                    // Nothing to do.
                }
            }
            else
            {
                // 처음으로 같은 값이거나 순서가 역방향인 원소 출현
                break;
            }
        }

        // sIdx를 sMaxIdx로 보정
        if ( sIdx > sMaxIdx )
        {
            sIdx = sMaxIdx;
        }
        else
        {
            // Nothing to do.
        }

        // sLastIdx / sIdx의 위치를 절대위치로 조정
        sLastIdx += sStartIdx;
        sIdx     += sStartIdx;
    }
    else
    {
        // a[Hint] -> Key 순서가 역방향이거나, 같은 값 => 왼쪽으로 Galloping
        // sMaxIdx / sIdx / sLastIdx는 sStartIdx에서의 상대 인덱스이다.
        sMaxIdx  = aHint + 1;
        sLastIdx = 0;
        sIdx     = 1;

        while ( sIdx < sMaxIdx )
        {
            get( aTempTable, aArray, ( sStartIdx - sIdx ), & sGallopRow );

            if ( compare( aTempTable, *aKey, *sGallopRow ) > 0 )
            {
                // 처음으로 순서가 순방향인 원소 출현
                break;
            }
            else
            {
                // 계속 값이 같거나 역방향
                sLastIdx = sIdx;

                // 2^n-1 만큼 Index 증가
                sIdx = ( sIdx << 1 ) + 1;

                // overflow 처리
                if ( sIdx <= 0 )
                {
                    sIdx = sMaxIdx;
                }
                else
                {
                    // Nothing to do.
                }
            }
        }

        // sIdx를 sMaxIdx로 보정
        if ( sIdx > sMaxIdx )
        {
            sIdx = sMaxIdx;
        }
        else
        {
            // Nothing to do.
        }

        // sLastIdx / sIdx의 위치를 절대위치로 조정
        // 이 때, sLastIdx < sIdx가 되도록 바꾼다.
        sTempIdx = sLastIdx;
        sLastIdx = sStartIdx - sIdx;
        sIdx     = sStartIdx - sTempIdx;
    }

    // 왼쪽으로 갔건 오른쪽으로 갔건,
    // 정확한 범위를 찾기 위해 sLastIdx -> sIdx 로 Binary Search
    sLastIdx++;
    while ( sLastIdx < sIdx )
    {
        sTempIdx = ( sIdx + sLastIdx ) / 2;
        get( aTempTable, aArray, sTempIdx, & sGallopRow );

        if ( compare( aTempTable, *aKey, *sGallopRow ) > 0 )
        {
            // 원소가 Key보다 역방향인 경우
            // 해당 위치가 End
            sLastIdx = sTempIdx + 1;
        }
        else
        {
            // 원소가 Key와 같거나 순방향인 경우
            // 해당 위치 다음이 Start
            sIdx = sTempIdx;
        }
    }

    IDE_DASSERT( sLastIdx == sIdx );
    IDE_DASSERT( sIdx - aBase <= aLength );

    // 인덱스에서 aBase를 빼야한다.
    return ( sIdx - aBase );
}

SLong qmcMemSort::gallopRight( qmcdMemSortTemp  * aTempTable,
                               qmcSortArray     * aArray,
                               void            ** aKey,
                               SLong              aBase,
                               SLong              aLength,
                               SLong              aHint )
{
/****************************************************************************
 *
 * Desciption : TASK-6445
 * 
 *  Run의 [aBase, aBase+aLength) 범위에서,
 *  정렬 순서 상 key가 위치해야 하는 인덱스를 반환한다.
 *  이 인덱스는 aBase에서의 상대적 위치로 반환된다.
 *
 *  만약 오름차순이라면
 *  a[aBase+Ret-1] <= key < a[aBase+Ret] 을 만족하는 Ret을 반환한다.
 *
 *  여기서 Right는 '오른쪽 방향' 이란 의미도 있지만,
 *  같은 값이 존재하는 경우 가장 '오른쪽'에 위치하는 인덱스를 구하라는 의미이다.
 *
 * Implementation :
 *
 *  gallopLeft()와 비교 조건만 다르다.
 *
 *   - a[Hint] -> Key 순서가 '같은 값이거나' 순방향인 경우,
 *     다음을 만족할 때 까지 오른쪽으로 Galloping
 *     > ASC  : if ( a[Hint] <= Key ) searchCond = ( Key < a[Hint + Offset] )
 *     > DESC : if ( a[Hint] >= Key ) searchCond = ( Key > a[Hint + Offset] )
 *
 *   - a[Hint] -> Key 순서가 역방향이면,
 *     다음을 만족할 때 까지 왼쪽으로 Galloping
 *     > ASC  : if ( a[Hint] > Key ) searchCond = ( a[Hint - Offset] <= Key )
 *     > DESC : if ( a[Hint] < Key ) searchCond = ( a[Hint - Offset] >= Key )
 *
 *  Galloping이 완료되면, (직전 Offset - 현재 Offset) 범위 안에서
 *  Binary Search를 해서 정확한 위치를 찾는다.
 *
 *  자세한 내용은 NoK 문서를 참고 (http://nok.altibase.com/x/qaMRAg)
 *
 * Note :
 *
 *  gallopLeft() 와 매우 유사하지만, 코드를 통합할 경우 유지보수도 어려울 뿐더러
 *  Loop 내부에 Branch가 많아지므로 성능 저하를 가져온다.
 *
 *  따라서, 별도 함수로 작성했다.
 *
 ****************************************************************************/

    void ** sGallopRow = NULL;
    SLong   sStartIdx  = aBase + aHint;
    SLong   sIdx       = 0;
    SLong   sLastIdx   = 0;
    SLong   sMaxIdx    = 0;
    SLong   sTempIdx   = 0;

    get( aTempTable, aArray, sStartIdx, & sGallopRow );

    if ( compare( aTempTable, *aKey, *sGallopRow ) >= 0 )
    {
        // a[Hint] -> Key 순서가 순방향이거나, 같은 값 => 오른쪽으로 Galloping
        // sMaxIdx / sIdx / sLastIdx는 sStartIdx에서의 상대 인덱스이다.
        sMaxIdx  = aLength - aHint;
        sLastIdx = 0;
        sIdx     = 1;

        while ( sIdx < sMaxIdx )
        {
            get( aTempTable, aArray, ( sStartIdx + sIdx ), & sGallopRow );

            if ( compare( aTempTable, *sGallopRow, *aKey ) > 0 )
            {
                // 처음으로 순서가 역방향인 원소 출현
                break;
            }
            else
            {
                // 계속 순서가 순방향이거나, 같은 값
                sLastIdx = sIdx;

                // 2^n-1 만큼 Index 증가
                sIdx = ( sIdx << 1 ) + 1;

                // overflow 처리
                if ( sIdx <= 0 )
                {
                    sIdx = sMaxIdx;
                }
                else
                {
                    // Nothing to do.
                }
            }
        }

        // sIdx를 sMaxIdx로 보정
        if ( sIdx > sMaxIdx )
        {
            sIdx = sMaxIdx;
        }
        else
        {
            // Nothing to do.
        }

        // sLastIdx / sIdx의 위치를 절대위치로 조정
        sLastIdx += sStartIdx;
        sIdx     += sStartIdx;
    }
    else
    {
        // a[Hint] -> Key 순서가 역방향 => 왼쪽으로 Galloping
        // sMaxIdx / sIdx / sLastIdx는 sStartIdx에서의 상대 인덱스이다.
        sMaxIdx  = aHint + 1;
        sLastIdx = 0;
        sIdx     = 1;

        while ( sIdx < sMaxIdx )
        {
            get( aTempTable, aArray, ( sStartIdx - sIdx ), & sGallopRow );

            if ( compare( aTempTable, *sGallopRow, *aKey ) > 0 )
            {
                // 계속 역방향
                sLastIdx = sIdx;

                // Gallop(2^n-1)
                sIdx = ( sIdx << 1 ) + 1;

                // overflow 처리
                if ( sIdx <= 0 )
                {
                    sIdx = sMaxIdx;
                }
                else
                {
                    // Nothing to do.
                }
            }
            else
            {
                // 처음으로 값이 같거나 순서가 순방향인 원소 출현
                break;
            }
        }

        // sIdx를 sMaxIdx로 보정
        if ( sIdx > sMaxIdx )
        {
            sIdx = sMaxIdx;
        }
        else
        {
            // Nothing to do.
        }

        // sLastIdx / sIdx의 위치를 절대위치로 조정
        // 이 때, sLastIdx < sIdx가 되도록 바꾼다.
        sTempIdx = sLastIdx;
        sLastIdx = sStartIdx - sIdx;
        sIdx     = sStartIdx - sTempIdx;
    }

    // 왼쪽으로 갔건 오른쪽으로 갔건,
    // 정확한 범위를 찾기 위해 sLastIdx -> sIdx 로 Binary Search
    sLastIdx++;
    while ( sLastIdx < sIdx )
    {
        sTempIdx = ( sIdx + sLastIdx ) / 2;
        get( aTempTable, aArray, sTempIdx, & sGallopRow );

        if ( compare( aTempTable, *sGallopRow, *aKey ) > 0 )
        {
            // 원소가 Key보다 순방향인 경우
            // 해당 위치가 End
            sIdx = sTempIdx;
        }
        else
        {
            // 원소가 Key와 같거나 역방향인 경우
            // 해당 위치 다음이 Start
            sLastIdx = sTempIdx + 1;
        }
    }

    IDE_DASSERT( sLastIdx == sIdx );
    IDE_DASSERT( sIdx - aBase <= aLength );

    // 인덱스에서 aBase를 빼야한다.
    return ( sIdx - aBase );
}

IDE_RC qmcMemSort::mergeLowerRun( qmcdMemSortTemp * aTempTable,
                                  SLong             aBase1,
                                  SLong             aLen1,
                                  SLong             aBase2,
                                  SLong             aLen2 )
{
/*******************************************************************
 *
 * Description : TASK-6445
 *
 *  첫 번째 Run을 Left, 두 번째 Run을 Right라고 할 때
 *  Left가 Right보다 작은 경우에 수행되는 함수이다.
 *
 *  Left L을 임시 영역 L'에 복사하고, 원래 영역에 L'와 Right R을 Mergesort한다.
 *  이 알고리즘은 In-place Sorting 이므로 원래 영역에 정렬시켜야 한다.
 *
 *  --------
 *  |  L'  | (Temp)
 *  --------                           ---------------------
 *     ^ Copy             => Merge =>  |  Merged Array(M)  |
 *  --------------------               ---------------------
 *  |  L   |     R     |
 *  --------------------
 *
 * Implementation : 
 *
 *  Merging 전 Galloping으로 인해, 아래 특징을 활용할 수 있다.
 *
 *  - R의 첫 번째 원소는 항상 M의 첫 번째 원소가 된다.
 *  - L의 마지막 원소는 항상 M의 마지막 원소가 된다.
 *
 *  진행 방향은 왼쪽이다.
 *  아래의 세 인덱스가 왼쪽 끝에서 오른쪽으로 이동한다.
 *
 *  - sTargetIdx  : 현재 정렬 결과가 들어가야 하는 위치
 *  - sLeftIdx    : L에서 정렬되지 않은 현재 위치
 *  - sRightIdx   : R에서 정렬되지 않은 현재 위치
 *
 *  각 인덱스의 Fence는 오른쪽 끝에 경계를 만든다.
 *
 *  - TargetFence는 Right의 Fence와 동일하다.
 *
 *  Galloping Mode 유지 조건은 GALLOPING_WINCNT로 고정된다.
 *  Galloping Mode 진입 조건은 sMinToGallopMode로 조정한다.
 *
 *******************************************************************/

    qmcSortArray  * sLeftArray    = NULL;
    void         ** sLeftRows     = NULL;
    void         ** sTargetRow    = NULL;
    void          * sLeftRow      = NULL;
    void         ** sRightRow     = NULL;
    SLong           sTargetIdx    = aBase1;
    SLong           sLeftIdx      = 0;
    SLong           sLeftFence    = aLen1;
    SLong           sRightIdx     = aBase2;
    SLong           sRightFence   = aBase2 + aLen2;
    SLong           sLeftWinCnt   = 0;
    SLong           sRightWinCnt  = 0;
    SInt            sMinGalloping = QMC_MEM_SORT_GALLOPING_WINCNT;
    
    //-------------------------------------------------
    // L' 설정
    //-------------------------------------------------

    sLeftArray = aTempTable->mTempArray;
    sLeftRows  = (void **)sLeftArray->elements;

    //-------------------------------------------------
    // L를 L'로 복사
    //-------------------------------------------------

    IDE_TEST( moveArrayToArray( aTempTable,
                                aTempTable->mArray, // SrcArr
                                aBase1,             // SrcBase
                                sLeftArray,         // DestArr
                                0,                  // DestBase
                                aLen1,              // MoveLength
                                ID_FALSE )          // IsOverlapped
              != IDE_SUCCESS );

    //--------------------------------------------------
    // Row 준비, 첫 원소 보정
    //--------------------------------------------------

    // L / R의 첫 Row 준비
    sLeftRow = sLeftRows[sLeftIdx];
    get( aTempTable, aTempTable->mArray, sRightIdx, & sRightRow );

    // Target Row 준비
    get( aTempTable, aTempTable->mArray, sTargetIdx, & sTargetRow );
    sTargetIdx++;

    // L'의 원소가 하나만 있는 경우, 바로 건너뛴다.
    IDE_TEST_CONT( aLen1 == 1, FINISH_LINE );

    // R의 첫 번째 원소는 무조건 첫 원소가 되므로, 미리 집어넣는다.
    IDE_DASSERT( compare( aTempTable, *sTargetRow, *sRightRow ) > 0 );
    *sTargetRow = *sRightRow;
    sRightIdx++;

    // 다음 R의 Row 준비
    get( aTempTable, aTempTable->mArray, sRightIdx, & sRightRow );

    //--------------------------------------------------
    // 탐색 시작
    //--------------------------------------------------

    while ( ID_TRUE )
    {
        //----------------------------------------
        // Normal Mode
        // 한 건씩 비교해가면서 전진한다.
        //----------------------------------------
        while ( ID_TRUE )
        {
            get( aTempTable, aTempTable->mArray, sTargetIdx, & sTargetRow );
            sTargetIdx++;

            if ( compare( aTempTable, *sRightRow, sLeftRow ) >= 0 )
            {
                // L'가 먼저 정렬되어야 하는 경우
                *sTargetRow = sLeftRow;
                sLeftIdx++;
                sLeftWinCnt++;
                sRightWinCnt = 0;

                if ( ( sLeftIdx + 1 ) < sLeftFence )
                {
                    // 다음 L'의 Row 준비
                    sLeftRow = sLeftRows[sLeftIdx];

                    if ( sLeftWinCnt >= sMinGalloping )
                    {
                        // Galloping Mode로 전환
                        break;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
                else
                {
                    // L'의 마지막만 남은 경우이거나 모두 소진된 경우 종료
                    IDE_CONT( FINISH_LINE );
                }
            }
            else
            {
                // R이 먼저 정렬되어야 하는 경우
                *sTargetRow = *sRightRow;
                sRightIdx++;
                sRightWinCnt++;
                sLeftWinCnt = 0;

                if ( sRightIdx < sRightFence )
                {
                    // 다음 R의 Row 준비
                    get( aTempTable, aTempTable->mArray, sRightIdx, & sRightRow );

                    if ( sRightWinCnt >= sMinGalloping )
                    {
                        // Galloping Mode로 전환
                        break;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
                else
                {
                    // R이 모두 소진된 경우 종료
                    IDE_CONT( FINISH_LINE );
                }
            }

        } // End of while : Normal Mode 종료
        
        //--------------------------------------------------------------
        // Galloping Mode
        // Normal Mode에서 나왔다면, 어느 한 쪽이 계속 정렬되어야 하는 경우
        // 두 gallop 함수의 결과가 GALLOPING_WINCNT보다 모두 작을 때 까지
        // 아래 과정을 반복한다.
        //
        // (1) R  Row를 기준으로 L'에 대한 gallopRight() 이후
        //     (GallopCnt) 만큼 배열로 이동
        // (2) R Row를 무조건 배열로 이동
        // (3) L' Row를 기준으로 R 에 대한 gallopLeft()  이후
        //     (GallopCnt) 만큼 배열로 이동
        // (4) L' Row를 무조건 배열로 이동
        //--------------------------------------------------------------

        // 다음 Galloping Mode 진입 조건 강화
        sMinGalloping++;

        while ( ( sLeftWinCnt  >= QMC_MEM_SORT_GALLOPING_WINCNT ) || 
                ( sRightWinCnt >= QMC_MEM_SORT_GALLOPING_WINCNT ) )
        {
            // 1회 수행할 때 마다 Galloping 기준값을 감소시킨다.
            // 단, GALLOPING_WINCNT_MIN 보다 낮아져서는 안된다.
            if ( sMinGalloping > QMC_MEM_SORT_GALLOPING_WINCNT_MIN )
            {
                sMinGalloping--;
            }
            else
            {
                // Nothing to do.
            }

            // R의 현재 Row를 기준으로, L'에 대한 Right Galloping
            sLeftWinCnt = gallopRight( aTempTable, 
                                       sLeftArray,                // TargetArray
                                       sRightRow,                 // Key  
                                       sLeftIdx,                  // Start
                                       ( sLeftFence - sLeftIdx ), // Length
                                       0 );                       // Hint

            if ( sLeftWinCnt > 0 )
            {
                // 해당 구간만큼 L'를 그대로 내림
                IDE_TEST( moveArrayToArray( aTempTable,
                                            sLeftArray,         // SrcArr
                                            sLeftIdx,           // SrcBase
                                            aTempTable->mArray, // DestArr
                                            sTargetIdx,         // DestBase
                                            sLeftWinCnt,        // MoveLength
                                            ID_FALSE )          // IsOverlapped
                          != IDE_SUCCESS );

                // Index 증가
                sLeftIdx   += sLeftWinCnt;
                sTargetIdx += sLeftWinCnt;

                if ( ( sLeftIdx + 1 ) < sLeftFence )
                {
                    // L'의 위치를 옮긴다.
                    sLeftRow = sLeftRows[sLeftIdx];
                }
                else
                {
                    // L'의 마지막만 남은 경우이거나 모두 소진된 경우 종료
                    IDE_CONT( FINISH_LINE );
                }
            }
            else
            {
                // Nothing to do.
            }

            // R의 현재 Row를 옮겨 담는다. (다음 차례에선 무조건 정렬되어야 하기 때문)
            get( aTempTable, aTempTable->mArray, sTargetIdx, & sTargetRow );
            *sTargetRow = *sRightRow;
            sTargetIdx++;
            sRightIdx++;

            if ( sRightIdx < sRightFence )
            {
                // R의 위치를 옮긴다.
                get( aTempTable, aTempTable->mArray, sRightIdx, & sRightRow );
            }
            else
            {
                // R이 모두 소진된 경우 종료
                IDE_CONT( FINISH_LINE );
            }

            // L'의 현재 Row를 기준으로, R에 대한 Left Galloping
            sRightWinCnt = gallopLeft( aTempTable, 
                                       aTempTable->mArray,
                                       & sLeftRow,
                                       sRightIdx,
                                       ( sRightFence - sRightIdx ),
                                       ( sRightFence - sRightIdx - 1 ) );
            
            if ( sRightWinCnt > 0 )
            {
                // 해당 구간만큼 R을 옮김
                IDE_TEST( moveArrayToArray( aTempTable,
                                            aTempTable->mArray, // SrcArr
                                            sRightIdx,          // SrcBase
                                            aTempTable->mArray, // DestArr
                                            sTargetIdx,         // DestBase
                                            sRightWinCnt,       // MoveLength
                                            ID_TRUE )           // IsOverlapped
                          != IDE_SUCCESS );

                // Index 증가
                sRightIdx += sRightWinCnt;
                sTargetIdx  += sRightWinCnt;

                if ( sRightIdx < sRightFence )
                {
                    // R의 위치를 옮긴다.
                    get( aTempTable, aTempTable->mArray, sRightIdx, & sRightRow );
                }
                else
                {
                    // R이 모두 소진된 경우 종료
                    IDE_CONT( FINISH_LINE );
                }
            }
            else
            {
                // Nothing to do.
            }

            // L'의 현재 Row를 옮겨 담는다. (다음 차례에선 무조건 정렬되어야 하기 때문)
            get( aTempTable, aTempTable->mArray, sTargetIdx, & sTargetRow );
            *sTargetRow = sLeftRow;
            sTargetIdx++;
            sLeftIdx++;

            if ( ( sLeftIdx + 1 ) < sLeftFence )
            {
                // L'의 위치를 옮긴다.
                sLeftRow = sLeftRows[sLeftIdx];
            }
            else
            {
                // L'의 마지막만 남은 경우이거나 모두 소진된 경우 종료
                IDE_CONT( FINISH_LINE );
            }

            // sTargetIdx는 sRightFence보다 작아야 한다.
            IDE_DASSERT( sRightFence > sTargetIdx );

        } // End of while : Galloping Mode 종료

        // 다음 Galloping Mode 진입 조건 강화
        sMinGalloping++;
    }

    //-------------------------------------------
    // 마무리 구간
    //-------------------------------------------

    IDE_EXCEPTION_CONT( FINISH_LINE ); 

    // L'가 남아있는 경우
    if ( sLeftIdx < sLeftFence )
    {
        // R도 남아있다면, R을 모두 옮긴 다음에 L'를 옮긴다.
        if ( sRightIdx < sRightFence )
        {
            // 이 경우라면, L'엔 딱 하나만 있어야 한다.
            // 두 개 이상인 경우였다면 위에서 처리되어야 하기 때문이다.
            IDE_DASSERT( sLeftIdx + 1 == sLeftFence );

            // R을 한 칸 앞으로 옮긴다.
            IDE_TEST( moveArrayToArray( aTempTable,
                                        aTempTable->mArray,          // SrcArr
                                        sRightIdx,                   // SrcBase
                                        aTempTable->mArray,          // DestArr
                                        ( sRightIdx - 1 ),           // DestBase
                                        ( sRightFence - sRightIdx ), // MoveLength
                                        ID_TRUE )                    // IsOverlapped
                      != IDE_SUCCESS );

            // L'의 마지막 원소를 R의 마지막에 넣기
            sLeftRow = sLeftRows[sLeftIdx];
            get( aTempTable, aTempTable->mArray, ( sRightFence - 1 ), & sRightRow );

            *sRightRow = sLeftRow;
        }
        else
        {
            // R은 이미 정렬이 되어 있고, L'가 일부 남은 경우엔,
            // L'를 모두 옮긴다.
            IDE_TEST( moveArrayToArray( aTempTable,
                                        sLeftArray,                // SrcArr
                                        sLeftIdx,                  // SrcBase
                                        aTempTable->mArray,        // DestArr
                                        sTargetIdx,                // DestBase
                                        ( sLeftFence - sLeftIdx ), // MoveLength
                                        ID_FALSE )                 // IsOverlapped
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // Galloping Mode를 통해 L'가 남아있지 않은 경우
        // 이미 정렬이 완료되었다.

        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmcMemSort::mergeHigherRun( qmcdMemSortTemp * aTempTable,
                                   SLong             aBase1,
                                   SLong             aLen1,
                                   SLong             aBase2,
                                   SLong             aLen2 )
{
/*******************************************************************
 *
 * Description : TASK-6445
 *
 *  첫 번째 Run을 Left, 두 번째 Run을 Right라고 할 때
 *  Rightt가 Left보다 작은 경우에 수행되는 함수이다.
 *
 *  Right R을 임시 영역 R'에 복사하고, 원래 영역에 Left L과 R을 Mergesort한다.
 *  이 알고리즘은 In-place Sorting 이므로 원래 영역에 정렬시켜야 한다.
 *
 *              -------
 *       (Temp) |  R' |
 *              -------              ---------------------
 *               ^ Copy => Merge =>  |  Merged Array(M)  |
 *  -------------------              ---------------------
 *  |     L     |  R  |
 *  -------------------
 *
 * Implementation : 
 * 
 *  mergeLowerRun() 와 유사하지만, 왼쪽 방향으로 진행된다.
 *  오른쪽이 아니므로 조금 더 복잡하며,
 *  모든 인덱스가 오른쪽 끝에서 왼쪽으로 이동한다.
 *
 *  - sTargetIdx  : 현재 정렬 결과가 들어가야 하는 위치
 *  - sLeftIdx    : L에서 정렬되지 않은 현재 위치
 *  - sRightIdx   : R에서 정렬되지 않은 현재 위치
 *
 *  각 인덱스의 Fence는 왼쪽 끝에 경계를 만든다.
 *
 *  - LeftFence는 Base1과 동일하므로 그대로 사용한다.
 *  - RightFence는 무조건 -1이므로 변수로 선언하지 않는다.
 *  - TargetFence는 Left의 Fence와 동일하다.
 *
 *  Galloping Mode 유지 조건은 GALLOPING_WINCNT로 고정된다.
 *  Galloping Mode 진입 조건은 sMinToGallopMode로 조정한다.
 *
 *******************************************************************/

    qmcSortArray  * sRightArray   = NULL;
    void         ** sRightRows    = NULL;
    void         ** sTargetRow    = NULL;
    void         ** sLeftRow      = NULL;
    void          * sRightRow     = NULL;
    SLong           sTargetIdx    = aBase2 + aLen2 - 1;
    SLong           sLeftIdx      = aBase1 + aLen1 - 1;
    SLong           sRightIdx     = aLen2 - 1;
    SLong           sGallopIndex  = 0;
    SLong           sLeftWinCnt   = 0;
    SLong           sRightWinCnt  = 0;
    SInt            sMinToGallopMode = QMC_MEM_SORT_GALLOPING_WINCNT;

    //-------------------------------------------------
    // R' 설정
    //-------------------------------------------------

    sRightArray = aTempTable->mTempArray;
    sRightRows  = (void **)sRightArray->elements;

    //-------------------------------------------------
    // R을 R'로 복사
    //-------------------------------------------------

    IDE_TEST( moveArrayToArray( aTempTable,
                                aTempTable->mArray, // SrcArr
                                aBase2,             // SrcBase
                                sRightArray,        // DestArr
                                0,                  // DestBase
                                aLen2,              // MoveLength
                                ID_FALSE )          // IsOverlapped
              != IDE_SUCCESS );

    //--------------------------------------------------
    // Row 준비, 첫 원소 보정
    //--------------------------------------------------

    // L / R의 마지막 Row 준비
    get( aTempTable, aTempTable->mArray, sLeftIdx, & sLeftRow );
    sRightRow = sRightRows[sRightIdx];

    // Target Row (마지막 Row) 준비
    get( aTempTable, aTempTable->mArray, sTargetIdx, & sTargetRow );
    sTargetIdx--;

    // R'의 원소가 하나만 있는 경우, 바로 건너뛴다.
    IDE_TEST_CONT( aLen2 == 1, FINISH_LINE );

    // L의 마지막 원소는 항상 마지막에 정렬되어야 하니 미리 집어넣는다.
    // Left Row가 Target Row보다 앞에 위치해야 한다.
    IDE_DASSERT( compare( aTempTable, *sLeftRow, *sTargetRow ) > 0 );
    *sTargetRow = *sLeftRow;
    sLeftIdx--;

    // L의 이전 Row 준비
    get( aTempTable, aTempTable->mArray, sLeftIdx, & sLeftRow );

    //--------------------------------------------------
    // 탐색 시작
    //--------------------------------------------------

    while ( ID_TRUE )
    {
        //----------------------------------------
        // Normal Mode
        // 한 건씩 비교해가면서 전진한다.
        //----------------------------------------
        while ( ID_TRUE )
        {
            get( aTempTable, aTempTable->mArray, sTargetIdx, & sTargetRow );
            sTargetIdx--;

            if ( compare( aTempTable, *sLeftRow, sRightRow ) >= 0 )
            {
                // L이 먼저 정렬되어야 하는 경우
                *sTargetRow = *sLeftRow;
                sLeftIdx--;
                sLeftWinCnt++;
                sRightWinCnt = 0;

                if ( sLeftIdx >= aBase1 )
                {
                    // L의 이전 Row 준비
                    get( aTempTable, aTempTable->mArray, sLeftIdx, & sLeftRow );

                    if ( sLeftWinCnt >= sMinToGallopMode )
                    {
                        // Galloping Mode로 전환
                        break;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
                else
                {
                    // L이 모두 소진될 경우 종료
                    IDE_CONT( FINISH_LINE );
                }
            }
            else
            {
                // R'이 먼저 정렬되어야 하는 경우
                *sTargetRow = sRightRow;
                sRightIdx--;
                sRightWinCnt++;
                sLeftWinCnt = 0;

                if ( sRightIdx > 0 )
                {
                    // R의 이전 Row 준비
                    sRightRow = sRightRows[sRightIdx];

                    if ( sRightWinCnt >= sMinToGallopMode )
                    {
                        // Galloping Mode로 전환
                        break;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
                else
                {
                    // R'의 마지막만 남은 경우이거나 모두 소진된 경우 종료
                    IDE_CONT( FINISH_LINE );
                }
            }

        } // End of while : Normal Mode 종료
        
        //--------------------------------------------------------------
        // Galloping Mode
        // Normal Mode에서 나왔다면, 어느 한 쪽이 계속 먼저 정렬되는 경우.
        // 두 gallop 함수의 결과가 GALLOPING_WINCNT보다 모두 작을 때 까지
        // 아래 과정을 반복한다.
        //
        // (1) R' Row를 기준으로 L 에 대한 gallopRight() 이후
        //     (현재 Length) - (GallopCnt) 만큼 뺀 나머지를 배열로 이동
        // (2) R' Row를 무조건 배열로 이동
        // (3) L  Row를 기준으로 R'에 대한 gallopLeft()  이후
        //     (현재 Length) - (GallopCnt) 만큼 뺀 나머지를 배열로 이동
        // (4) L  Row를 무조건 배열로 이동
        //--------------------------------------------------------------

        // 다음 Galloping Mode 진입 조건 강화
        sMinToGallopMode++;

        while ( ( sLeftWinCnt  >= QMC_MEM_SORT_GALLOPING_WINCNT ) || 
                ( sRightWinCnt >= QMC_MEM_SORT_GALLOPING_WINCNT ) )
        {
            // 1회 수행할 때 마다 Galloping 기준값을 감소시킨다.
            // 단, GALLOPING_WINCNT_MIN 보다 낮아져서는 안된다.
            if ( sMinToGallopMode > QMC_MEM_SORT_GALLOPING_WINCNT_MIN )
            {
                sMinToGallopMode--;
            }
            else
            {
                // Nothing to do.
            }

            // R'의 현재 Row를 기준으로, L에 대한 Right Galloping
            sGallopIndex = gallopRight( aTempTable,                
                                        aTempTable->mArray,        // TargetArray
                                        & sRightRow,               // Key  
                                        aBase1,                    // Start
                                        ( sLeftIdx - aBase1 + 1 ), // Length
                                        0 );                       // Hint

            // GallopIndex가 가리키는 곳 '부터' 끝부분까지 옮겨야 하므로
            // Winning Count는 아래와 같이 계산한다.
            // (Length) - (GallopCnt)
            sLeftWinCnt = ( sLeftIdx - aBase1 + 1 ) - sGallopIndex;

            if ( sLeftWinCnt > 0 )
            {
                // 해당 구간만큼 인덱스 조정
                // Winning Count는 길이이으로, 1을 뺀 값으로 인덱스를 조정한다.
                sTargetIdx -= ( sLeftWinCnt - 1 );
                sLeftIdx   -= ( sLeftWinCnt - 1 );

                // 해당 구간만큼 L을 그대로 옮김
                IDE_TEST( moveArrayToArray( aTempTable,
                                            aTempTable->mArray, // SrcArr
                                            sLeftIdx,           // SrcBase
                                            aTempTable->mArray, // DestArr
                                            sTargetIdx,         // DestBase
                                            sLeftWinCnt,        // MoveLength
                                            ID_TRUE )           // IsOverlapped
                          != IDE_SUCCESS );

                // 미처 못 내린 1칸을 더 내린다.
                sTargetIdx--;
                sLeftIdx--;

                if ( sLeftIdx >= aBase1 )
                {
                    // L의 이전 Row 준비
                    get( aTempTable, aTempTable->mArray, sLeftIdx, & sLeftRow );
                }
                else
                {
                    // L이 모두 소진된 경우 종료
                    IDE_CONT( FINISH_LINE );
                }
            }
            else
            {
                // Nothing to do.
            }

            // R'의 현재 Row를 옮겨 담는다. (다음 차례에선 무조건 정렬되어야 하기 때문)
            get( aTempTable, aTempTable->mArray, sTargetIdx, & sTargetRow );
            *sTargetRow = sRightRow;
            sTargetIdx--;
            sRightIdx--;

            if ( sRightIdx > 0 )
            {
                // R'의 이전 Row 준비
                sRightRow = sRightRows[sRightIdx];
            }
            else
            {
                // R'의 마지막만 남은 경우이거나 모두 소진된 경우 종료
                IDE_CONT( FINISH_LINE );
            }

            // L의 현재 Row를 기준으로, R'에 대한 Left Galloping
            sGallopIndex = gallopLeft( aTempTable, 
                                       sRightArray,
                                       sLeftRow,
                                       0,
                                       ( sRightIdx + 1 ),
                                       sRightIdx );
            
            // GallopIndex가 가리키는 곳 부터 이후의 부분을 옮겨야 하므로
            // Winning Count는 아래와 같이 계산한다.
            // (Length) - (GallopIndex)
            sRightWinCnt = ( sRightIdx + 1 ) - sGallopIndex;

            if ( sRightWinCnt > 0 )
            {
                // 해당 구간만큼 인덱스 조정
                // Winning Count는 길이이으로, 1을 뺀 값으로 인덱스를 조정한다.
                sTargetIdx -= ( sRightWinCnt - 1 );
                sRightIdx  -= ( sRightWinCnt - 1 );

                // 해당 구간만큼 R을 옮김
                IDE_TEST( moveArrayToArray( aTempTable,
                                            sRightArray,        // SrcArr
                                            sRightIdx,          // SrcBase
                                            aTempTable->mArray, // DestArr
                                            sTargetIdx,         // DestBase
                                            sRightWinCnt,       // MoveLength
                                            ID_FALSE )          // IsOverlapped
                          != IDE_SUCCESS );

                // 미처 못 내린 1칸을 더 내린다.
                sTargetIdx--;
                sRightIdx--;

                if ( sRightIdx > 0 )
                {
                    // R'의 이전 Row 준비
                    sRightRow = sRightRows[sRightIdx];
                }
                else
                {
                    // R'의 마지막만 남은 경우이거나 모두 소진된 경우 종료
                    IDE_CONT( FINISH_LINE );
                }
            }
            else
            {
                // Nothing to do.
            }

            // L의 현재 Row를 옮겨 담는다. (다음 차례에선 무조건 정렬되어야 하기 때문)
            get( aTempTable, aTempTable->mArray, sTargetIdx, & sTargetRow );
            *sTargetRow = *sLeftRow;
            sTargetIdx--;
            sLeftIdx--;

            if ( sLeftIdx >= aBase1 )
            {
                // L의 이전 Row 준비
                get( aTempTable, aTempTable->mArray, sLeftIdx, & sLeftRow );
            }
            else
            {
                // L이 모두 소진된 경우 종료
                IDE_CONT( FINISH_LINE );
            }

            // sTargetIdx는 aBase1보다 크거나 같아야 한다.
            IDE_DASSERT( sTargetIdx >= aBase1 );

        } // End of while : Galloping Mode 종료

        // 다음 Galloping Mode 진입 조건 강화
        sMinToGallopMode++;
    }

    //-------------------------------------------
    // 마무리 구간
    //-------------------------------------------

    IDE_EXCEPTION_CONT( FINISH_LINE ); 

    // R'가 남아있는 경우
    if ( sRightIdx >= 0 )
    {
        // L도 남아있다면, L을 모두 옮긴 다음에 R'를 옮긴다.
        if ( sLeftIdx >= aBase1 )
        {
            // 이 경우라면, R'엔 딱 하나만 있어야 한다.
            // 두 개 이상인 경우였다면 위에서 처리되어야 하기 때문이다.
            IDE_DASSERT( sRightIdx == 0 );

            // L의 나머지를 한 칸 뒤로 옮긴다.
            IDE_TEST( moveArrayToArray( aTempTable,
                                        aTempTable->mArray,        // SrcArr
                                        aBase1,                    // SrcBase
                                        aTempTable->mArray,        // DestArr
                                        ( aBase1 + 1 ),            // DestBase = SrcBase+1
                                        ( sLeftIdx - aBase1 + 1 ), // MoveLength
                                        ID_TRUE )                  // IsOverlapped
                      != IDE_SUCCESS );

            // R'의 마지막 원소를 L의 처음에 넣기
            sRightRow = sRightRows[sRightIdx];
            get( aTempTable, aTempTable->mArray, aBase1, & sLeftRow );

            *sLeftRow = sRightRow;
        }
        else
        {
            // L은 이미 정렬이 되어 있고, R'가 일부 남은 경우엔,
            // R'를 모두 옮긴다.
            // 여기서 DestBase가 aBase1 인 이유는
            // R' 내용이 모두 '앞쪽'으로 들어가야 하기 때문이다.
            IDE_TEST( moveArrayToArray( aTempTable,
                                        sRightArray,        // SrcArr
                                        0,                  // SrcBase
                                        aTempTable->mArray, // DestArr
                                        aBase1,             // DestBase
                                        ( sRightIdx + 1 ),  // MoveLength
                                        ID_FALSE )          // IsOverlapped
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // Galloping Mode를 통해 R'가 남아있지 않은 경우
        // 이미 정렬이 완료되었다.

        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

//---------------------------------------------------------------
// TASK-6445 Utilities
// 알고리즘과는 별개로, SortArray를 활용하기 위해 만든 함수들
//---------------------------------------------------------------

IDE_RC qmcMemSort::moveArrayToArray( qmcdMemSortTemp * aTempTable,
                                     qmcSortArray    * aSrcArray,
                                     SLong             aSrcBase,
                                     qmcSortArray    * aDestArray,
                                     SLong             aDestBase,
                                     SLong             aMoveLength,
                                     idBool            aIsOverlapped )
{
/*******************************************************************
 *
 * Description :
 *
 *  aSrcBase 에서 aMoveLength 만큼의 elements 영역을 옮긴다.
 *
 *  두 영역이 겹치게 될 경우, Src를 위한 임시 영역을 만들게 된다.
 *  이 영역에 Src 내용을 넣은 뒤 Dest로 이동한다.
 *
 ********************************************************************/

    qmcSortArray    sTempArray;
    qmcSortArray  * sBaseArray    = NULL;
    qmcSortArray  * sSrcArray     = NULL;
    qmcSortArray  * sDestArray    = NULL;
    SLong           sSrcBase      = aSrcBase;
    SLong           sDestBase     = aDestBase;
    SLong           sMoveLength   = aMoveLength;
    SLong           sSrcIdx       = 0;
    SLong           sDestIdx      = 0;
    SLong           sSrcLength    = 0;
    SLong           sDestLength   = 0;
    SLong           sLength       = 0;
    idBool          sIsSrcRemain  = ID_FALSE;
    idBool          sIsDestRemain = ID_FALSE;

    //-------------------------------------------------
    // Src의 기준이 되는 Base Array 생성/지정
    //-------------------------------------------------

    if ( aIsOverlapped == ID_TRUE )
    {
        // 겹칠 수 있다면, 임시 Array를 생성
        sTempArray.parent   = NULL;
        sTempArray.depth    = 0;
        sTempArray.numIndex = aMoveLength;
        sTempArray.shift    = 0;
        sTempArray.mask     = 0;
        sTempArray.index    = 0;

        IDU_FIT_POINT( "qmcMemSort::moveArrayToArray::malloc",
                       idERR_ABORT_InsufficientMemory );
        IDE_TEST( iduMemMgr::malloc( IDU_MEM_QMC,
                                     aMoveLength * ID_SIZEOF(void*),
                                     (void**) & sTempArray.elements )
                  != IDE_SUCCESS );
        
        // 생성한 Array에 Src 복사
        IDE_TEST( moveArrayToArray( aTempTable,
                                    aSrcArray,     // SrcArr
                                    aSrcBase,      // SrcBase
                                    & sTempArray,  // DestArr
                                    0,             // DestBase
                                    aMoveLength,   // MoveLength
                                    ID_FALSE )     // IsOverlapped
                  != IDE_SUCCESS );

        sBaseArray = & sTempArray;
        sSrcBase   = 0; 
    }
    else
    {
        sTempArray.elements = NULL;
        sBaseArray          = aSrcArray;
        sSrcBase            = aSrcBase; 
    }

    //---------------------------------------------------
    // 이동
    //---------------------------------------------------
    
    while ( sMoveLength > 0 )
    {
        // Array 선정

        if ( sIsSrcRemain == ID_FALSE )
        {
            getLeafArray( sBaseArray,
                          sSrcBase,
                          & sSrcArray,
                          & sSrcIdx );
        }
        else
        {
            // SrcArray가 아직 다 이전하지 않은 경우
            // sSrcBase / sSrcIdx 그대로 사용
        }

        if ( sIsDestRemain == ID_FALSE )
        {
            getLeafArray( aDestArray,
                          sDestBase,
                          & sDestArray,
                          & sDestIdx );
        }
        else
        {
            // DestArray가 아직 다 차지 않은 경우
            // sDestBase / sDestIdx 그대로 사용
        }

        //---------------------------------------------------
        // Remain?
        //---------------------------------------------------

        sSrcLength    = sSrcArray->numIndex - sSrcIdx;
        sDestLength   = sDestArray->numIndex - sDestIdx;
        sIsSrcRemain  = ID_FALSE;
        sIsDestRemain = ID_FALSE;

        if ( sSrcLength > sDestLength )
        {
            sLength      = sDestLength;
            sIsSrcRemain = ID_TRUE;
        }
        else if ( sSrcLength < sDestLength )
        {
            sLength       = sSrcLength;
            sIsDestRemain = ID_TRUE;
        }
        else
        {
            // sSrcLength == sDestLength
            sLength = sDestLength;
        }

        // sMoveLength가 더 작으면 sMoveLength로 설정
        sLength = IDL_MIN( sLength, sMoveLength );

        // memcpy()로 이동
        // 덮어쓰이는 영역은 이미 정렬 영역에 이동했거나 임시 영역에 존재한다.
        // 그 외의 경우가 일어나지 않도록 한다.
        idlOS::memcpy( (void **)sDestArray->elements + sDestIdx,
                       (void **)sSrcArray->elements + sSrcIdx,
                       ID_SIZEOF(void*) * sLength );

        // Index 증가
        sSrcBase    += sLength;
        sDestBase   += sLength;
        sSrcIdx     += sLength;
        sDestIdx    += sLength;
        sMoveLength -= sLength;
    }

    // 할당했던 임시 영역을 해제
    if ( aIsOverlapped == ID_TRUE )
    {
        IDE_DASSERT( sTempArray.elements != NULL );
        (void)iduMemMgr::free( sTempArray.elements );
        sTempArray.elements = NULL;
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    // 할당했던 임시 영역을 해제
    if ( sTempArray.elements != NULL )
    {
        (void)iduMemMgr::free( sTempArray.elements );
        sTempArray.elements = NULL;
    }
    else
    {
        // Nothing to do.
    }

    return IDE_FAILURE;
}

void qmcMemSort::getLeafArray( qmcSortArray  * aArray,
                               SLong           aIndex,
                               qmcSortArray ** aRetArray,
                               SLong         * aRetIdx )
{
/*******************************************************************
 *
 * Description :
 *   
 *   SortArray에서 aIndex 위치에 해당하는 Leaf Array를 반환하고,
 *   Leaf Array에서의 상대적인 인덱스 값도 반환한다.
 *
 *******************************************************************/

    if ( aArray->depth != 0 )
    {
        getLeafArray( aArray->elements[aIndex >> (aArray->shift)],
                      aIndex & ( aArray->mask ),
                      aRetArray,
                      aRetIdx );
    }
    else
    {
        if ( aRetArray != NULL )
        {
            *aRetArray = aArray;
        }
        else
        {
            // Nothing to do.
        }

        if ( aRetIdx != NULL )
        {
            *aRetIdx = aIndex;
        }
        else
        {
            // Nothing to do.
        }
    }

}
