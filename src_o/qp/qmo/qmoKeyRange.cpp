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
 * $Id: qmoKeyRange.cpp 86786 2020-02-27 08:04:12Z donovan.seo $
 *
 * Description : Key Range 생성기
 *
 *     < keyRange 생성절차 >
 *     1. DNF 형태로 노드 변환
 *     2. keyRange의 크기 측정
 *     3. keyRange 생성을 위한 메모리 준비
 *     4. keyRange 생성
 *
 *     < keyRange 생성절차가 수행되는 시점 >
 *    --------------------------------------------------------------
 *    |           | fixed keyRange         |   variable keyRange   |
 *    --------------------------------------------------------------
 *    | prepare   | 1.DNF 형태로 노드변환  | 1.DNF 형태로 노드변환 |
 *    | 단계      | 2.keyRange 크기 측정   |                       |
 *    |           | 3.메모리준비           |                       |
 *    |           | 4.keyRange생성         |                       |
 *    |-------------------------------------------------------------
 *    | execution |                        | 2.keyRange 크기 측정  |
 *    | 단계      |                        | 3.메모리준비          |
 *    |           |                        | 4.keyRange 생성       |
 *    --------------------------------------------------------------
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <qtc.h>
#include <qmgProjection.h>
#include <qmoPredicate.h>
#include <qmoKeyRange.h>

extern mtfModule mtfBetween;
extern mtfModule mtfNotBetween;

extern mtfModule mtfInlist;

IDE_RC
qmoKeyRange::estimateKeyRange( qcTemplate  * aTemplate,
                               qtcNode     * aNode,
                               UInt        * aRangeSize )
{
/***********************************************************************
 *
 * Description : Key Range의 크기를 측정한다.
 *
 *   keyRange의 크기 =   (1) 비교연산자에 대한 실제 range size
 *                     + (2) and merge에 필요한 최대 range size
 *                     + (3) or merge에 필요한 최대 range size
 *
 * Implementation :
 *
 ***********************************************************************/

    UInt      sPrevRangeCount;
    UInt      sRangeCount = 0;
    UInt      sCount;
    UInt      sRangeSize = 0;
    qtcNode * sNode;

    IDU_FIT_POINT_FATAL( "qmoKeyRange::estimateKeyRange::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------
    IDE_DASSERT( aNode != NULL );
    IDE_DASSERT( aRangeSize != NULL );

    //--------------------------------------
    // estimate size
    //--------------------------------------

    // aNode는 DNF로 변환된 형태의 노드이다.

    if( ( aNode->node.lflag &
          ( MTC_NODE_LOGICAL_CONDITION_MASK | MTC_NODE_OPERATOR_MASK ) )
        == ( MTC_NODE_LOGICAL_CONDITION_TRUE | MTC_NODE_OPERATOR_OR ) )
    {
        // OR 논리 연산자

        // OR 하위 노드에 대한 size 및
        // and merge와 or merge에 필요한 최대 range count를 구한다.

        sNode = (qtcNode *)(aNode->node.arguments);
        sPrevRangeCount = 1;

        while( sNode != NULL )
        {
            sCount = 0;
            IDE_TEST( estimateRange( aTemplate,
                                     sNode,
                                     &sCount,
                                     &sRangeSize )
                      != IDE_SUCCESS );

            // OR merge를 위한 최대 range 갯수를 구한다.
            sPrevRangeCount = sPrevRangeCount + sCount;
            sRangeCount = sRangeCount + sPrevRangeCount;

            sNode = (qtcNode *)(sNode->node.next);
        }
    }
    else
    {
        // Nothing To Do
    }

    // OR 하위 노드에 대한 계산이 끝나면, 전체 range size 계산
    // (1) AND 노드에 대한 size 계산 ( 비교연산자 + and merge ) +
    // (2) OR  노드에 대한 or merge에 대한 size 계산            +
    // (3) OR  노드에 대한 or merge시 range list를 qsort하기 위해
    //     자료구조 배열을 만들기 위한 size 계산
    //     (fix BUG-9378)
    sRangeSize =
        sRangeSize +
        ( sRangeCount * idlOS::align8((UInt) ID_SIZEOF(smiRange) ) ) +
        ( sRangeCount * ID_SIZEOF(smiRange *) ) ;

    *aRangeSize = sRangeSize;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoKeyRange::makeKeyRange( qcTemplate  * aTemplate,
                           qtcNode     * aNode,
                           UInt          aKeyCount,
                           mtcColumn   * aKeyColumn,
                           UInt        * aKeyColsFlag,
                           UInt          aCompareType,
                           smiRange    * aRangeArea,
                           UInt          aRangeAreaSize, 
                           smiRange   ** aRange,
                           qtcNode    ** aFilter )
{
/***********************************************************************
 *
 * Description : Key Range를 생성한다.
 *
 *     Key Range를 생성하기 위한 public interface.
 *
 * Implementation :
 *
 *     Key Range와 Key Filter에 대한 range 생성이 거의 동일하므로,
 *     내부적으로는 makeRange()라는 동일함수를 호출한다.
 *     이때, 입력인자로 Key Range에 대한 range를 생성하라는 정보를 넘긴다.
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmoKeyRange::makeKeyRange::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------
    IDE_DASSERT( aNode != NULL );
    IDE_DASSERT( aKeyCount > 0 );
    IDE_DASSERT( aKeyColumn != NULL );
    IDE_DASSERT( aKeyColsFlag != NULL );
    IDE_DASSERT( aRangeArea != NULL );
    IDE_DASSERT( aRange != NULL );
    IDE_DASSERT( aFilter != NULL );

    //--------------------------------------
    // keyRange 생성
    //--------------------------------------

    IDE_TEST( makeRange( aTemplate,
                         aNode,
                         aKeyCount,
                         aKeyColumn,
                         aKeyColsFlag,
                         ID_TRUE,
                         aCompareType,
                         aRangeArea,
                         aRangeAreaSize,
                         aRange,
                         aFilter ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoKeyRange::makeKeyFilter( qcTemplate  * aTemplate,
                            qtcNode     * aNode,
                            UInt          aKeyCount,
                            mtcColumn   * aKeyColumn,
                            UInt        * aKeyColsFlag,
                            UInt          aCompareType,
                            smiRange    * aRangeArea,
                            UInt          aRangeAreaSize,
                            smiRange   ** aRange,
                            qtcNode    ** aFilter )
{
/***********************************************************************
 *
 * Description : Key Filter를 생성한다.
 *
 *    Key Filter를 생성하기 위한 public interface.
 *
 *    Key Range생성과 달리 Key Column에 연속된 Column이 존재할 필요가
 *    없다.
 *
 * Implementation :
 *
 *     Key Range와 Key Filter에 대한 range 생성이 거의 동일하므로,
 *     내부적으로는 makeRange()라는 동일함수를 호출한다.
 *     이때, 입력인자로 Key Filter에 대한 range를 생성하라는 정보를 넘긴다.
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmoKeyRange::makeKeyFilter::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aNode != NULL );
    IDE_DASSERT( aKeyCount > 0 );
    IDE_DASSERT( aKeyColumn != NULL );
    IDE_DASSERT( aKeyColsFlag != NULL );
    IDE_DASSERT( aRangeArea != NULL );
    IDE_DASSERT( aRange != NULL );
    IDE_DASSERT( aFilter != NULL );

    //--------------------------------------
    // keyFilter 생성
    //--------------------------------------

    IDE_TEST( makeRange( aTemplate,
                         aNode,
                         aKeyCount,
                         aKeyColumn,
                         aKeyColsFlag,
                         ID_FALSE,
                         aCompareType,
                         aRangeArea,
                         aRangeAreaSize,
                         aRange,
                         aFilter )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoKeyRange::makeNotNullRange( void               * aPredicate,
                               mtcColumn          * aKeyColumn,
                               smiRange           * aRange )
{
/***********************************************************************
 *
 * Description : Indexable MIN, MAX 적용을 위한 Not Null Range 생성
 *
 *     < Indexable MIN, MAX >
 *
 *     MIN(), MAX() aggregation의 경우, 해당 column에 인덱스가 존재한다면,
 *     인덱스를 사용하여 한 건만 fetch함으로써 원하는 결과를 얻을 수 있다.
 *     예) select min(i1) from t1; index on T1(i1)
 *
 *     MAX()의 경우, NULL value가 존재하면 NULL value가 가장 큰 값으로
 *     fetch 되므로, not null range를 생성해서,
 *     NULL 값은 index scan대상에서 제외한다.
 *
 * Implementation :
 *
 *     1. keyRange 적용을 위한 size 구하기
 *     2. 메모리 할당받기
 *     3. not null range 구성
 *
 ***********************************************************************/

    mtkRangeInfo   sRangeInfo;

    IDU_FIT_POINT_FATAL( "qmoKeyRange::makeNotNullRange::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aKeyColumn != NULL );
    IDE_DASSERT( aRange     != NULL );

    //--------------------------------------
    // indexable MIN/MAX 적용을 위한 not null range 생성
    //--------------------------------------

    // not null range 생성
    sRangeInfo.column    = aKeyColumn;
    sRangeInfo.argument  =  0; // not used
    sRangeInfo.direction = MTD_COMPARE_ASCENDING; // not used
    sRangeInfo.columnIdx = 0; //NotNull일 경우 첫번째 KeyColumn을 다룸
    sRangeInfo.useOffset = 0;

    if ( ( aKeyColumn->column.flag & SMI_COLUMN_STORAGE_MASK )
             == SMI_COLUMN_STORAGE_DISK )
    {
        sRangeInfo.compValueType = MTD_COMPARE_STOREDVAL_MTDVAL;
    }
    else
    {
        if( (aKeyColumn->column.flag & SMI_COLUMN_TYPE_MASK)
             == SMI_COLUMN_TYPE_FIXED )
        {
            /*
             * PROJ-2433
             * Direct Key Index를 위한 key compare 함수 type 세팅
             */
            if ( ( smiTable::getIndexInfo( ((qmnCursorPredicate *)aPredicate)->index->indexHandle ) &
                 SMI_INDEX_DIRECTKEY_MASK ) == SMI_INDEX_DIRECTKEY_TRUE )
            {
                sRangeInfo.compValueType = MTD_COMPARE_INDEX_KEY_FIXED_MTDVAL;
            }
            else
            {
                sRangeInfo.compValueType = MTD_COMPARE_FIXED_MTDVAL_FIXED_MTDVAL;
            }
        }
        else
        {
            IDE_DASSERT( ( (aKeyColumn->column.flag & SMI_COLUMN_TYPE_MASK)
                           == SMI_COLUMN_TYPE_VARIABLE ) ||
                         ( (aKeyColumn->column.flag & SMI_COLUMN_TYPE_MASK)
                           == SMI_COLUMN_TYPE_VARIABLE_LARGE ) );

            /*
             * PROJ-2433
             * Direct Key Index를 위한 key compare 함수 type 세팅
             */
            if ( ( smiTable::getIndexInfo( ((qmnCursorPredicate *)aPredicate)->index->indexHandle ) &
                 SMI_INDEX_DIRECTKEY_MASK ) == SMI_INDEX_DIRECTKEY_TRUE )
            {
                sRangeInfo.compValueType = MTD_COMPARE_INDEX_KEY_MTDVAL;
            }
            else
            {
                sRangeInfo.compValueType = MTD_COMPARE_MTDVAL_MTDVAL;
            }
        }
    }

    IDE_TEST( mtk::extractNotNullRange( NULL,
                                        NULL,
                                        &sRangeInfo,
                                        aRange )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoKeyRange::estimateRange( qcTemplate  * aTemplate,
                            qtcNode     * aNode,
                            UInt        * aRangeCount,
                            UInt        * aRangeSize )
{
/***********************************************************************
 *
 * Description : Key Range의 크기를 측정한다.
 *
 *   keyRange의 크기 =   (1) 비교연산자에 대한 실제 range size
 *                     + (2) and merge에 필요한 최대 range size
 *                     + (3) or merge에 필요한 최대 range size
 *
 * Implementation :
 *
 ***********************************************************************/

    UInt      sIsNotRangeCnt = 0; // !=, not between 갯수
    UInt      sSize = 0;
    UInt      sPrevRangeCount;
    UInt      sCount;
    UInt      sAndArgCnt = 0;
    qtcNode * sNode;

    IDU_FIT_POINT_FATAL( "qmoKeyRange::estimateRange::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------
    IDE_DASSERT( aNode != NULL );
    IDE_DASSERT( aRangeCount != NULL );
    IDE_DASSERT( aRangeSize != NULL );

    //--------------------------------------
    // estimate size
    //--------------------------------------

    // aNode는 DNF로 변환된 형태의 노드이다.

    if( ( aNode->node.lflag &
               ( MTC_NODE_LOGICAL_CONDITION_MASK | MTC_NODE_OPERATOR_MASK ) )
             == ( MTC_NODE_LOGICAL_CONDITION_TRUE | MTC_NODE_OPERATOR_AND ) )
    {
        // AND 논리 연산자

        sNode = (qtcNode *)(aNode->node.arguments);
        sAndArgCnt++;
        sPrevRangeCount = 0;

        IDE_TEST( estimateRange( aTemplate,
                                 sNode,
                                 &sPrevRangeCount,
                                 aRangeSize )
                  != IDE_SUCCESS );

        if( sNode->node.next == NULL )
        {

            (*aRangeCount) = sPrevRangeCount;
        }
        else
        {
            sNode = (qtcNode *)(sNode->node.next);

            while( sNode != NULL )
            {
                sAndArgCnt ++;
                sCount = 0;
                IDE_TEST( estimateRange( aTemplate,
                                         sNode,
                                         &sCount,
                                         aRangeSize )
                          != IDE_SUCCESS );

                sPrevRangeCount = sPrevRangeCount + sCount - 1;
                (*aRangeCount) = (*aRangeCount) + sPrevRangeCount;

                // fix BUG-12254
                if( sCount > 1 )
                {
                    // !=, not between인 경우, 그 갯수를 계산
                    sIsNotRangeCnt += 1;
                }
                else
                {
                    // Nothing To Do
                }

                sNode = (qtcNode *)(sNode->node.next);
            }

            if( sIsNotRangeCnt > 0 )
            {
                // range 복사를 위한
                // smiRange, mtkRangeCallBack에 대한 size 계산
                // 참조 : qmoKeyRange::makeRange() 함수
                IDE_TEST(  mtk::estimateRangeDefault( NULL,
                                                      NULL,
                                                      0,
                                                      & sSize )
                           != IDE_SUCCESS );

                // Not range에 대한 range 범위는 최대 notRangeCnt + 1 가 됨.
                // 예) i1 != 1 and i1 != 2 : range 갯수 3개
                // --> i1 < 1 and 1 < i1 < 2 and i1 > 2
                // (1) not range가 composite index의 사용가능 마지막컬럼인경우,
                //     이전 컬럼까지의 composite range가 있으므로
                //     계산된 not range 갯수만큼의 크기가 필요
                sSize *= sIsNotRangeCnt;

                // (2) range 복사시, range내의 mtkRangeCallBack 도 복사
                //     not range 이전 컬럼까지의 composite 처리된
                //     callBack을 모두 복사해야 함.
                (*aRangeSize) += sSize;
                (*aRangeSize) +=
                    (sAndArgCnt-sIsNotRangeCnt) * 2 *
                    idlOS::align8((UInt) ID_SIZEOF(mtkRangeCallBack))
                    * sIsNotRangeCnt;
            }
            else
            {
                // Nothing To Do
            }
        }
    }
    else
    {
        // 비교 연산자

        // 비교연산자에 대한 size estimate
        // BUG-42283 host variable predicate 에 대한 estimateRange size 는 0 이다.
        // (ex) WHERE i1 = :a AND i2 = :a OR :b NOT LIKE 'a%'
        //                                   ^^^^^^^^^^^^^^^^
        if ( qtc::haveDependencies( &aNode->depInfo ) == ID_TRUE )
        {
            IDE_TEST( aTemplate->tmplate.rows[aNode->node.table].
                      execute[aNode->node.column].estimateRange(
                          NULL,
                          & aTemplate->tmplate,
                          0,
                          & sSize )
                      != IDE_SUCCESS );
        }
        else
        {
            sSize = 0;
        }

        //--------------------------------------
        // and merge size 계산을 위한 count 계산
        // (1). !=, not between 은 count=2
        // (2). inlist 는 count=1000
        // (3). 1,2를 제외한 나머지 비교연산자는 count=1
        //--------------------------------------
        if( ( ( aNode->node.lflag & MTC_NODE_OPERATOR_MASK )
              == MTC_NODE_OPERATOR_NOT_EQUAL )
            || ( ( aNode->node.lflag & MTC_NODE_OPERATOR_MASK )
                 == MTC_NODE_OPERATOR_NOT_RANGED ) )
        {
            // !=, not between
            *aRangeCount = 2;
        }
        else
        {
            /* BUG-32622 inlist operator */
            if( aNode->node.module == &mtfInlist )
            {
                *aRangeCount = MTC_INLIST_KEYRANGE_COUNT_MAX;
            }
            else
            {
                *aRangeCount = 1;
            }
        }

        (*aRangeSize) = (*aRangeSize) + sSize;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoKeyRange::makeRange( qcTemplate  * aTemplate,
                        qtcNode     * aNode,
                        UInt          aKeyCount,
                        mtcColumn   * aKeyColumn,
                        UInt        * aKeyColsFlag,
                        idBool        aIsKeyRange,
                        UInt          aCompareType,
                        smiRange    * aRangeArea,
                        UInt          aRangeAreaSize,
                        smiRange   ** aRange,
                        qtcNode    ** aFilter )
{
/***********************************************************************
 *
 * Description :  Key Range를 생성한다.
 *
 *     keyRange와 keyFilter에 대한 range 생성은 거의 동일하며,
 *     차이점은,
 *     (1) keyRange  : Key Column에 연속된 Column이 존재
 *     (2) keyFilter : Key Column에 연속된 Column이 존재할 필요가 없다.
 *
 *     예) index on T1(i1,i2,i3)
 *         . i1=1 and i2>1   : keyRange(O), keyFilter(O)
 *         . (i1,i3) = (1,1) : keyRange(X), keyFilter(O)
 *
 * Implementation :
 *
 *
 *
 ***********************************************************************/

    UInt         i;
    UInt         sCnt;
    UInt         sOffset = 0;
    UInt         sSize;
    UInt         sRangeCount;

    UChar      * sRangeStartPtr = (UChar*)aRangeArea;
    smiRange   * sRange = NULL;
    smiRange   * sNextRange = NULL;
    smiRange   * sCurRange = NULL;
    smiRange   * sLastRange = NULL;
    smiRange   * sLastCurRange = NULL;
    smiRange   * sRangeList = NULL;
    smiRange   * sLastRangeList = NULL;
    smiRange  ** sRangeListArray;

    qtcNode    * sAndNode;
    idBool       sIsExistsValue = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmoKeyRange::makeRange::__FT__" );

    //--------------------------------------
    // fix BUG-13939
    // in subquery keyRange or subquery keyRange 에 대해서는
    // 미리 subquery를 수행한다.
    //--------------------------------------

    IDE_TEST( calculateSubqueryInRangeNode( aTemplate,
                                            aNode,
                                            & sIsExistsValue )
              != IDE_SUCCESS );

    //------------------------------------------
    // sIsExistsValues : In subquery KeyRange인 경우,
    //                   keyRange를 구성할 value가 있는지의 정보
    //------------------------------------------
    if( sIsExistsValue == ID_TRUE )
    {
        //--------------------------------------
        // keyRange 생성
        //--------------------------------------

        // 인자로 넘어온 aNode는 최상위가 OR 노드이다.
        for( sAndNode = (qtcNode *)(aNode->node.arguments);
             sAndNode != NULL;
             sAndNode = (qtcNode *)(sAndNode->node.next) )
        {

            //-------------------------------------
            // 각 AND 노드에 대해, 인덱스 컬럼순으로 range 생성
            //-------------------------------------

            sRange = NULL;

            for( sCnt = 0; sCnt < aKeyCount; sCnt++ )
            {
                // To Fix BUG-10260
                IDE_TEST( makeRange4AColumn( aTemplate,
                                             sAndNode,
                                             &aKeyColumn[sCnt],
                                             sCnt,
                                             &aKeyColsFlag[sCnt],
                                             &sOffset,
                                             sRangeStartPtr,
                                             aRangeAreaSize,
                                             aCompareType,
                                             & sCurRange )
                          != IDE_SUCCESS );

                //----------------------------------
                // 현재 인덱스 컬럼으로 만들어진 range가 없으면,
                // keyRange의 경우, 다음 인덱스컬럼에 대한 range 생성 중단,
                // keyFilter의 경우, 다음 인덱스컬럼에 대한 range 생성 시도.
                //----------------------------------

                if( sCurRange == NULL )
                {
                    if( aIsKeyRange == ID_TRUE )
                    {
                        break;
                    }
                    else
                    {
                        continue;
                    }
                }
                else
                {
                    // Nothing To Do
                }

                if( sRange == NULL )
                {
                    sRange = sCurRange;
                }
                else
                {
                    // fix BUG-12254
                    if( sCurRange->next != NULL )
                    {
                        // 예: index on T1(i1, i2)
                        //     i1=1 and i2 not between 0 and 1
                        //     i1=1 and i2 != 1 인 경우에 대한 처리
                        //
                        //   i1 = 1 and i2 != 1 를 예로 들면,
                        //
                        //   (1) i1 = 1 에 대한 range 생성
                        //   (2) i2 != 1 에 대한 range 생성
                        //      ==>  -무한대 < i2 < 1 OR 1 < i2 < +무한대
                        //   (3) (1)과 (2)로 composite range 구성
                        //      ==> ( i1 = 1 and -무한대 < i2 < 1 )
                        //       or ( i1 = 1 and 1 < i2 < +무한대 )
                        //   (3)을 처리하기 위해서
                        //   i1 = 1 에 대한 range가 하나 더 필요하므로,
                        //   copyRange에서 i1=1 range를 복사해서 사용.

                        sRange->prev = NULL;
                        sRange->next = NULL;

                        sLastRange = sRange;
                        sLastCurRange = sCurRange;

                        // Not Range 갯수 - 1 만큼 range 복사
                        while( sLastCurRange->next != NULL )
                        {
                            IDE_TEST( copyRange ( sRange,
                                                  &sOffset,
                                                  sRangeStartPtr,
                                                  aRangeAreaSize,
                                                  &sNextRange )
                                      != IDE_SUCCESS );

                            sNextRange->prev = sLastRange;
                            sNextRange->next = NULL;

                            sLastRange->next = sNextRange;
                            sLastRange = sLastRange->next;

                            sLastCurRange = sLastCurRange->next;
                        }

                        sLastRange = sRange;
                        sLastCurRange = sCurRange;

                        // 각 range에 composite range 구성
                        while( ( sLastRange != NULL )
                               && ( sLastCurRange != NULL ) )
                        {
                            IDE_TEST( mtk::addCompositeRange( sLastRange,
                                                              sLastCurRange )
                                      != IDE_SUCCESS );

                            sLastRange = sLastRange->next;
                            sLastCurRange = sLastCurRange->next;
                        }
                    }
                    else
                    {
                        IDE_TEST( mtk::addCompositeRange( sRange, sCurRange )
                                  != IDE_SUCCESS );
                    }
                }
            }

            sCurRange = sRange;

            if( sCurRange == NULL )
            {
                if( aIsKeyRange == ID_TRUE )
                {
                    // If one of OR keyrange is NULL, then All range is NULL.
                    // Otherwise, a result will be wrong.
                    // example : TC/Server/qp2/SelectStmt/Bugs/PR-4447/select5
                    sRange = NULL;
                    sRangeCount = 0;
                    break;
                }
                else
                {
                    continue;
                }
            }
            else
            {
                // Nothing To Do
            }

            // 생성된 range들의 연결리스트를 구성
            if( sRangeList == NULL )
            {
                sRangeList = sCurRange;

                for( sLastRangeList = sRangeList;
                     sLastRangeList->next != NULL;
                     sLastRangeList = sLastRangeList->next ) ;
            }
            else
            {
                sLastRangeList->next = sCurRange;

                for( sLastRangeList = sLastRangeList->next;
                     sLastRangeList->next != NULL;
                     sLastRangeList = sLastRangeList->next ) ;
            }
        }

        if( sRangeList != NULL )
        {
            // fix BUG-9378
            // KEY RANGE MERGE 성능 개선

            // OR merge를 위한 range count를 구한다.
            for( sLastRangeList = sRangeList, sRangeCount = 0;
                 sLastRangeList != NULL;
                 sLastRangeList = sLastRangeList->next, sRangeCount++ ) ;

            // OR노드 하위에 AND노드가 2개이상이고,
            // merge할 range가 두개 이상인 경우에 or merge를 수행한다.
            if( ( aNode->node.arguments->next != NULL ) &&
                ( sRangeCount > 1 ) )
            {
                // smiRange pointer 배열을 위한 공간 확보
                sSize = ID_SIZEOF(smiRange *) * sRangeCount;

                IDE_TEST_RAISE( sOffset + sSize > aRangeAreaSize, ERR_OUT_OF_BOUND );

                sRangeListArray = (smiRange **)(sRangeStartPtr + sOffset );
                IDE_TEST_RAISE( sRangeListArray == NULL,
                                ERR_INVALID_MEMORY_AREA );

                sOffset += sSize;

                for( i = 0; i < sRangeCount; i++ )
                {
                    sRangeListArray[i] = sRangeList;
                    sRangeList = sRangeList->next;
                }

                // or merge를 위한 공간 확보
                sSize =
                    idlOS::align8((UInt) ID_SIZEOF(smiRange)) * sRangeCount;

                IDE_TEST_RAISE( sOffset + sSize > aRangeAreaSize, ERR_OUT_OF_BOUND );

                sRange = (smiRange *)(sRangeStartPtr + sOffset);
                IDE_TEST_RAISE( sRange == NULL, ERR_INVALID_MEMORY_AREA );

                sOffset += sSize;

                // or merge
                // BUG-28934
                // data type의 특성에 맞게 key range를 생성한 후, 이를 merge할 때도
                // key range의 특성에 맞게 merge하는 방법이 필요하다.
                IDE_TEST( aKeyColumn->module->mergeOrRangeList( sRange,
                                                                sRangeListArray,
                                                                sRangeCount )
                          != IDE_SUCCESS );
            }
            else
            {
                sRange = sRangeList;
            }
        }
        else
        {
            // Nothing To Do
        }
    }
    else
    {
        // Nothing To Do
    }

    if( sRange != NULL )
    {
        *aRange  = sRange;
        *aFilter = NULL;
    }
    else
    {
        *aRange  = NULL;
        *aFilter = aNode;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_MEMORY_AREA)
    {
        IDE_SET(ideSetErrorCode(qpERR_FATAL_QMO_INVALID_MEMORY_AREA));
    }
    IDE_EXCEPTION(ERR_OUT_OF_BOUND)
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmoKeyRange::makeRange",
                                  "Invalid memory area" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoKeyRange::calculateSubqueryInRangeNode( qcTemplate   * aTemplate,
                                           qtcNode      * aNode,
                                           idBool       * aIsExistsValue )
{
/***********************************************************************
 *
 * Description : Range를 구성할 노드중에 subquery node를 먼저 수행한다.
 *
 * Implementation :
 *
 * subquery keyRange를 수행하기 위한 노드 변환이 아래와 같이 이루어진다.
 * keyRange 구성시에는 index column순서로 range를 구성하게 되는데,
 * index 첫번째 컬럼에 대한 range 구성시,
 * 이에 대응되는 subquery target column인 a1은
 * subquery가 수행되기전이어서 실제적인 a1의 값을 얻지 못하게 된다.
 * 이러한 문제점을 해결하기 위해,
 * range를 구성하기 전에 range node를 모두 뒤져서 subquery를 먼저 실행한다.
 *
 * 예) index on( i1, i2 ) 이고,
 *
 * 1) where ( i2, i1 ) = ( select a2, a1 from ... )
 *
 *    [ = ]                          [=] -------------------> [=]
 *      |                             |                        |
 *    [LIST]--->[subquery]           [i2] ----> [ind]         [i1]--> [ind]
 *      |           |          ==>                |                     |
 *    [i2]-->[i1] [a2]-->[a1]                 [subquery]               [a1]
 *                                                |
 *                                               [a2]
 * 2) where ( i2, i1 ) in ( select a2, a1 from ... )
 *
 *     [IN]                          [=] -------------------> [=]
 *      |                             |                        |
 *    [LIST]--->[subquery]           [i2] ----> [Wrapper]     [i1]--> [ind]
 *      |           |          ==>                 |                    |
 *    [i2]-->[i1] [a2]-->[a1]                  [Subquery]               |
 *                                                 |                    |
 *                                                [a2]                 [a1]
 *
 ***********************************************************************/

    qtcNode     * sAndNode;
    qtcNode     * sCompareNode;
    qtcNode     * sValueNode;

    IDU_FIT_POINT_FATAL( "qmoKeyRange::calculateSubqueryInRangeNode::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aNode != NULL );

    //--------------------------------------
    // fix BUG-13939
    // in subquery keyRange or subquery keyRange 에 대해서는
    // 미리 subquery를 수행한다.
    //--------------------------------------

    for( sAndNode = (qtcNode *)(aNode->node.arguments);
         sAndNode != NULL;
         sAndNode = (qtcNode*)(sAndNode->node.next) )
    {
        for( sCompareNode = (qtcNode *)(sAndNode->node.arguments);
             sCompareNode != NULL;
             sCompareNode = (qtcNode *)(sCompareNode->node.next) )
        {
            if( ( sCompareNode->lflag & QTC_NODE_SUBQUERY_RANGE_MASK )
                == QTC_NODE_SUBQUERY_RANGE_TRUE )
            {
                if( aNode->indexArgument == 0 )
                {
                    sValueNode =
                        (qtcNode*)(sCompareNode->node.arguments->next);
                }
                else
                {
                    sValueNode = (qtcNode*)(sCompareNode->node.arguments);
                }

                IDE_TEST( qtc::calculate( sValueNode, aTemplate )
                          != IDE_SUCCESS );

                if( aTemplate->tmplate.stack->value == NULL )
                {
                    *aIsExistsValue = ID_FALSE;
                    break;
                }
                else
                {
                    *aIsExistsValue = ID_TRUE;
                }
            }
            else
            {
                *aIsExistsValue = ID_TRUE;
            }
        }

        if( *aIsExistsValue == ID_TRUE )
        {
            // Nothing To Do
        }
        else
        {
            break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoKeyRange::makeRange4AColumn( qcTemplate   * aTemplate,
                                qtcNode      * aNode,
                                mtcColumn    * aKeyColumn,
                                UInt           aColumnIdx,
                                UInt         * aKeyColsFlag,
                                UInt         * aOffset,
                                UChar        * aRangeStartPtr,
                                UInt           aRangeAreaSize,
                                UInt           aCompareType,
                                smiRange    ** aRange )
{
/***********************************************************************
 *
 * Description : 하나의 인덱스 컬럼에 대한 range를 생성한다.
 *
 * Implementation :
 *
 *   인덱스컬럼을 포함하는 모든 비교연산자에 대한 range를 만든다.
 *
 *   인덱스 컬럼에 대한 range가 여러개인 경우, range 범위를 조정한다.
 *   예) i1>1 and i1<3 인 경우, 1 < i1 < 3 으로 range 범위 조정.
 *
 ***********************************************************************/

    idBool         sIsSameGroupType = ID_FALSE;
    UInt           sSize;
    UInt           sFlag;
    UInt           sMergeCount;
    smiRange     * sRange = NULL;
    smiRange     * sPrevRange = NULL;
    smiRange     * sCurRange = NULL;
    mtcExecute   * sExecute;
    mtkRangeInfo   sInfo;
    qtcNode      * sNode;
    qtcNode      * sColumnNode;
    qtcNode      * sValueNode;
    qtcNode      * sColumnConversion;
    idBool         sFixedValue = ID_TRUE;

    IDU_FIT_POINT_FATAL( "qmoKeyRange::makeRange4AColumn::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aOffset != NULL );
    IDE_DASSERT( aRangeStartPtr != NULL );

    //--------------------------------------
    // range 생성
    //--------------------------------------

    if( ( aNode->node.lflag &
          ( MTC_NODE_LOGICAL_CONDITION_MASK | MTC_NODE_OPERATOR_MASK ) )
        == ( MTC_NODE_LOGICAL_CONDITION_TRUE | MTC_NODE_OPERATOR_AND ) )
    {
        // AND 논리 연산자

        sNode = (qtcNode *)(aNode->node.arguments);

        while( sNode != NULL )
        {
            IDE_TEST( makeRange4AColumn( aTemplate,
                                         sNode,
                                         aKeyColumn,
                                         aColumnIdx,
                                         aKeyColsFlag,
                                         aOffset,
                                         aRangeStartPtr,
                                         aRangeAreaSize,
                                         aCompareType,
                                         & sCurRange ) != IDE_SUCCESS );

            if( sPrevRange == NULL )
            {
                if( sCurRange == NULL )
                {
                    sRange = NULL;
                }
                else
                {
                    sRange = sCurRange;
                    sPrevRange = sRange;
                }
            }
            else
            {
                if( sCurRange == NULL )
                {
                    sRange = sPrevRange;
                }
                else
                {
                    sMergeCount = getAndRangeCount( sPrevRange, sCurRange );

                    sSize =
                        idlOS::align8((UInt)ID_SIZEOF(smiRange)) * sMergeCount;

                    IDE_TEST_RAISE( sSize + *aOffset > aRangeAreaSize, ERR_OUT_OF_BOUND );

                    sRange = (smiRange*)(aRangeStartPtr + *aOffset);
                    IDE_TEST_RAISE( sRange == NULL, ERR_INVALID_MEMORY_AREA );

                    *aOffset += sSize;

                    // BUG-28934
                    // data type의 특성에 맞게 key range를 생성한 후, 이를 merge할 때도
                    // key range의 특성에 맞게 merge하는 방법이 필요하다.
                    IDE_TEST( aKeyColumn->module->mergeAndRange( sRange,
                                                                 sPrevRange,
                                                                 sCurRange )
                              != IDE_SUCCESS );

                    sPrevRange = sRange;
                }
            }

            sNode = (qtcNode *)(sNode->node.next);
        } // end of while()
    } // AND 논리연산자의 처리
    else
    {
        // 비교 연산자

        //------------------------------------------
        // 비교연산자의 indexArgument의 columnID와 index columnID가 같고,
        // 컬럼에 conversion이 발생하지 않았는지 검사.
        //------------------------------------------

        if( aNode->indexArgument == 0 )
        {
            sColumnNode = (qtcNode *)(aNode->node.arguments);
            sValueNode = (qtcNode *)(aNode->node.arguments->next);
        }
        else
        {
            sColumnNode = (qtcNode *)(aNode->node.arguments->next);
            sValueNode = (qtcNode *)(aNode->node.arguments);
        }

        // To Fix PR-8700
        if ( sColumnNode->node.module == &qtc::passModule )
        {
            // To Fix PR-8700
            // Sort Join등을 위한
            // Pass Node의 경우 Indexable 여부의 판단시에는
            // Conversion 여부를 확인하지 않아야 하는 반면,
            // Key Range 생성을 위해서는 Argument를 이용하여야 한다.
            sColumnNode = (qtcNode *)
                mtf::convertedNode( (mtcNode *) sColumnNode,
                                    & aTemplate->tmplate );
            // fix BUG-12005
            // Sort Join 등을 위한 pass node의 경우
            // mtkRangeInfo.isSameGroupType = ID_FALSE가 되어야 함.
        }
        else
        {
            // fix BUG-12005
            // sm에 내려줄 callback을 지정해 주기 위한 정보로
            // mtkRangeInfo.isSameGroupType에 그 정보를 저장
            sColumnConversion = (qtcNode *)
                mtf::convertedNode( (mtcNode *) sColumnNode,
                                    & aTemplate->tmplate );

            if( sColumnNode == sColumnConversion )
            {
                // Nothing To
                //
                // 완전히 동일한 type이라 변환이 필요 하지 않다.
                // 하지만
                // sIsSameGroupType = ID_FALSE;
            }
            else
            {
                sIsSameGroupType = ID_TRUE;
            }
        }

        if ( aTemplate->tmplate.rows[sColumnNode->node.table].
             columns[sColumnNode->node.column].column.id
             == aKeyColumn->column.id )
        {
            // value node를 수행해서, value를 얻는다.

            // (1) IS NULL, IS NOT NULL일 경우,
            //     sValueNode == NULL
            // (2) BETWEEN, NOT BETWEEN일 경우,
            //     sValueNode와 sValueNode->next의 값을 읽어야 하며,
            // (3) 그 밖의 비교연산자는  sValueNode의 값을 읽어야 한다.
            for( sNode = sValueNode;
                 sNode != NULL && sNode != sColumnNode;
                 sNode = (qtcNode *)(sNode->node.next) )
            {
                // Bug-11320 fix
                // Between, Not between 뿐만 아니라,
                // Like 함수도 인자가 3개일 수 있다.
                // 따라서 sNode가 null이 아닐때까지
                // 모두 calculate 해줘야 한다.
                // calculate된 값이 null이면 중단한다.

                // fix BUG-13939
                if( ( aNode->lflag & QTC_NODE_SUBQUERY_RANGE_MASK )
                    == QTC_NODE_SUBQUERY_RANGE_TRUE )
                {
                    // Nothing To Do

                    // 이미 subquery를 수행했으므로,
                    // calculate 하지 않아도 됨.
                    sFixedValue = ID_FALSE;
                }
                else
                {
                    // To Fix PR-8259
                    IDE_TEST( qtc::calculate( sNode, aTemplate )
                              != IDE_SUCCESS );

                    // BUG-43758
                    // sFixedValue가 ID_TRUE이면 MTD_COMPARE_FIXED_MTDVAL_FIXED_MTDVAL을 설정해
                    // compressed column의 value를 잘못 가져옵니다.
                    if ( ( ( aTemplate->tmplate.stack->column->column.flag
                             &SMI_COLUMN_TYPE_MASK )
                           != SMI_COLUMN_TYPE_FIXED ) ||
                         ( ( aTemplate->tmplate.stack->column->column.flag
                             &SMI_COLUMN_COMPRESSION_MASK )
                           == SMI_COLUMN_COMPRESSION_TRUE ) )

                    {
                        sFixedValue = ID_FALSE;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
            }

            // set Flag
            if( ( *aKeyColsFlag & SMI_COLUMN_ORDER_MASK )
                == SMI_COLUMN_ORDER_ASCENDING )
            {
                sFlag = MTD_COMPARE_ASCENDING;
            }
            else
            {
                sFlag = MTD_COMPARE_DESCENDING;
            }

            sExecute = &(aTemplate->tmplate.
                         rows[aNode->node.table].
                         execute[aNode->node.column]);

            // get keyRange size
            IDE_TEST( sExecute->estimateRange( NULL,
                                               &aTemplate->tmplate,
                                               0,
                                               &sSize ) != IDE_SUCCESS );

            IDE_TEST_RAISE( sSize + *aOffset > aRangeAreaSize, ERR_OUT_OF_BOUND );

            // extract keyRange
            sRange = (smiRange *)(aRangeStartPtr + *aOffset );
            IDE_TEST_RAISE( sRange == NULL, ERR_INVALID_MEMORY_AREA);

            *aOffset += sSize;

            sInfo.column          = aKeyColumn;
            sInfo.argument        = aNode->indexArgument;
            sInfo.direction       = sFlag;
            sInfo.isSameGroupType = sIsSameGroupType;
            sInfo.compValueType   = aCompareType;
            sInfo.columnIdx       = aColumnIdx;
            sInfo.useOffset       = 0;

            // fixed fixed인 경우 fixed compare를 호출하여 성능을 개선한다.
            // BUG-43758
            // aKeyColumn이 compressed column인 경우 MTD_COMPARE_FIXED_MTDVAL_FIXED_MTDVAL을 설정하면
            // compressed column의 value를 잘못 가져옵니다.
            if ( ( aCompareType == MTD_COMPARE_MTDVAL_MTDVAL ) &&
                 ( ( aKeyColumn->column.flag & SMI_COLUMN_TYPE_MASK )
                   == SMI_COLUMN_TYPE_FIXED ) &&
                 ( ( aKeyColumn->column.flag & SMI_COLUMN_COMPRESSION_MASK )
                   == SMI_COLUMN_COMPRESSION_FALSE ) &&
                 ( sFixedValue == ID_TRUE ) )
            {
                sInfo.compValueType = MTD_COMPARE_FIXED_MTDVAL_FIXED_MTDVAL;
            }
            else
            {
                // Nothing to do.
            }

            IDE_TEST( sExecute->extractRange( (mtcNode *)aNode,
                                              &( aTemplate->tmplate ),
                                              & sInfo,
                                              sRange )
                      != IDE_SUCCESS );

            if ( aNode->node.module == &mtfInlist )
            {
                *aOffset -= sSize;
                *aOffset += sInfo.useOffset;
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            sRange = NULL;
        }
    }

    *aRange = sRange;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_MEMORY_AREA)
    {
        IDE_SET(ideSetErrorCode(qpERR_FATAL_QMO_INVALID_MEMORY_AREA));
    }
    IDE_EXCEPTION( ERR_OUT_OF_BOUND )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmoKeyRange::makeRange4AColumn",
                                  "Invalid Memory Area" ));
    }
        
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


UInt
qmoKeyRange::getRangeCount( smiRange * aRange )
{
/***********************************************************************
 *
 * Description : range의 count를 얻는다.
 *
 * Implementation :
 *
 ***********************************************************************/

    //--------------------------------------
    // count 계산
    //--------------------------------------

    UInt       sRangeCount = 0;
    smiRange * sRange;

    sRange = aRange;

    while( sRange != NULL )
    {
        sRangeCount++;
        sRange = sRange->next;
    }

    return sRangeCount;
}


UInt
qmoKeyRange::getAndRangeCount( smiRange * aRange1,
                               smiRange * aRange2 )
{
/***********************************************************************
 *
 * Description : AND merge를 위한 range count를 얻는다.
 *
 * Implementation :
 *
 *     ( range1 + range2 - 1 )
 *
 ***********************************************************************/


    //--------------------------------------
    // count 계산
    //--------------------------------------

    UInt   sRange1Count;
    UInt   sRange2Count;

    sRange1Count = getRangeCount( aRange1 );
    sRange2Count = getRangeCount( aRange2 );

    return ( sRange1Count + sRange2Count - 1 );
}

IDE_RC
qmoKeyRange::copyRange( smiRange  * aRangeOrg,
                        UInt      * aOffset,
                        UChar     * aRangeStartPtr,
                        UInt        aRangeAreaSize,
                        smiRange ** aRangeNew )
{
/***********************************************************************
 *
 * Description :
 *
 * Implementation :
 *
 *     ( range1 + range2 )
 *
 ***********************************************************************/

    UInt                sSize = 0;
    smiRange          * sRange;
    mtkRangeCallBack  * sCallBackOrg = NULL;
    mtkRangeCallBack  * sCallBackNew = NULL;
    mtkRangeCallBack  * sCallBackMinimum = NULL;
    mtkRangeCallBack  * sCallBackMaximum = NULL;
    mtkRangeCallBack  * sLastCallBack = NULL;

    IDU_FIT_POINT_FATAL( "qmoKeyRange::copyRange::__FT__" );

    //--------------------------------------
    // range 복사
    //--------------------------------------

    IDE_TEST( mtk::estimateRangeDefault( NULL,
                                         NULL,
                                         0,
                                         &sSize )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( *aOffset + sSize > aRangeAreaSize, ERR_OUT_OF_BOUND );

    sRange = (smiRange *)(aRangeStartPtr + (*aOffset));
    IDE_TEST_RAISE( sRange == NULL,
                    ERR_INVALID_MEMORY_AREA );

    (*aOffset) += sSize;

    idlOS::memcpy( sRange, aRangeOrg, sSize );

    for( sCallBackOrg =
             (mtkRangeCallBack *)aRangeOrg->minimum.data,
             sCallBackMinimum = NULL;
         sCallBackOrg != NULL;
         sCallBackOrg = sCallBackOrg->next )
    {
        sSize = idlOS::align8((UInt) ID_SIZEOF(mtkRangeCallBack));

        IDE_TEST_RAISE( *aOffset + sSize > aRangeAreaSize, ERR_OUT_OF_BOUND );

        sCallBackNew =
            (mtkRangeCallBack *)(aRangeStartPtr + (*aOffset) );

        (*aOffset) += sSize;

        idlOS::memcpy( sCallBackNew, sCallBackOrg, sSize );

        if( sCallBackMinimum == NULL )
        {
            sCallBackMinimum = sCallBackNew;
            sLastCallBack = sCallBackMinimum;
        }
        else
        {
            sLastCallBack->next = sCallBackNew;
            sLastCallBack = sLastCallBack->next;
        }
    }

    for( sCallBackOrg =
             (mtkRangeCallBack *)aRangeOrg->maximum.data,
             sCallBackMaximum = NULL;
         sCallBackOrg != NULL;
         sCallBackOrg = sCallBackOrg->next )
    {
        sSize = idlOS::align8((UInt) ID_SIZEOF(mtkRangeCallBack));

        IDE_TEST_RAISE( *aOffset + sSize > aRangeAreaSize, ERR_OUT_OF_BOUND );

        sCallBackNew =
            (mtkRangeCallBack *)(aRangeStartPtr + (*aOffset) );

        (*aOffset) += sSize;

        idlOS::memcpy( sCallBackNew, sCallBackOrg, sSize );

        if( sCallBackMaximum == NULL )
        {
            sCallBackMaximum = sCallBackNew;
            sLastCallBack = sCallBackMaximum;
        }
        else
        {
            sLastCallBack->next = sCallBackNew;
            sLastCallBack = sLastCallBack->next;
        }
    }

    sRange->minimum.data = (void *)sCallBackMinimum;
    sRange->maximum.data = (void *)sCallBackMaximum;

    *aRangeNew = sRange;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_MEMORY_AREA)
    {
        IDE_SET(ideSetErrorCode(qpERR_FATAL_QMO_INVALID_MEMORY_AREA));
    }
    IDE_EXCEPTION(ERR_OUT_OF_BOUND)
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qmoKeyRange::copyRange",
                                  "Invalid memory area" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoKeyRange::makePartKeyRange(
    qcTemplate          * aTemplate,
    qtcNode             * aNode,
    UInt                  aPartKeyCount,
    mtcColumn           * aPartKeyColumns,
    UInt                * aPartKeyColsFlag,
    UInt                  aCompareType,
    smiRange            * aRangeArea,
    UInt                  aRangeAreaSize,
    smiRange           ** aRange )
{
/***********************************************************************
 *
 *  Description : PROJ-1502 PARTITIONED DISK TABLE
 *                partition keyrange를 생성한다.
 *
 *  Implementation : 기존의 keyrange생성 루틴과 동일하다.
 *                   filter가 필요없으므로, filter는 무시한다.
 *
 ***********************************************************************/

    qtcNode * sFilter;

    IDU_FIT_POINT_FATAL( "qmoKeyRange::makePartKeyRange::__FT__" );

    IDE_TEST( makeRange( aTemplate,
                         aNode,
                         aPartKeyCount,
                         aPartKeyColumns,
                         aPartKeyColsFlag,
                         ID_TRUE,
                         aCompareType,
                         aRangeArea,
                         aRangeAreaSize,
                         aRange,
                         & sFilter )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
