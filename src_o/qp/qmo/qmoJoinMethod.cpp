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
 * $Id: qmoJoinMethod.cpp 89835 2021-01-22 10:10:02Z andrew.shin $
 *
 * Description :
 *    Join Cost를 구하여 다양한 Join Method들 중에서 가장 cost 가 좋은
 *    Join Method를 선택한다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <ide.h>
#include <idl.h>
#include <qmoJoinMethod.h>
#include <qmoSelectivity.h>
#include <qmgSelection.h>
#include <qmgJoin.h>
#include <qmo.h>
#include <qmoCost.h>
#include <qmgPartition.h>
#include <qcgPlan.h>
#include <qmoCrtPathMgr.h>
#include <qmgShardSelect.h>
#include <qmv.h>

IDE_RC
qmoJoinMethodMgr::init( qcStatement    * aStatement,
                        qmgGraph       * aGraph,
                        SDouble          aFirstRowsFactor,
                        qmoPredicate   * aJoinPredicate,
                        UInt             aJoinMethodType,
                        qmoJoinMethod  * aJoinMethod )
{
/***********************************************************************
 *
 * Description : Join Method의 초기화
 *
 * Implementation :
 *    (1) qmoJoinMethod 초기화
 *    (2) Join Method Type에 따라 joinMethodCnt, joinMethodCost 구성
 *    (3) joinMethodCost 초기화
 *
 ***********************************************************************/

    qmoJoinMethodCost       * sJoinMethodCost;
    qmoJoinLateralDirection   sLateralDirection;
    UInt                      sJoinMethodCnt;
    UInt                      sCount;

    IDU_FIT_POINT_FATAL( "qmoJoinMethodMgr::init::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );

    // Join Method 초기화
    aJoinMethod->flag = QMO_JOIN_METHOD_FLAG_CLEAR;
    aJoinMethod->selectedJoinMethod = NULL;
    aJoinMethod->hintJoinMethod = NULL;
    aJoinMethod->hintJoinMethod2 = NULL;
    aJoinMethod->joinMethodCnt = 0;
    aJoinMethod->joinMethodCost = NULL;

    // PROJ-2418
    // Lateral Position 획득
    IDE_TEST( getJoinLateralDirection( aGraph, & sLateralDirection )
              != IDE_SUCCESS );

    // Join Method Type Flag 설정  및 Join Method Cost의 초기화
    switch ( aJoinMethodType & QMO_JOIN_METHOD_MASK )
    {
        case QMO_JOIN_METHOD_NL :
            // Join Method Type 설정
            aJoinMethod->flag &= ~QMO_JOIN_METHOD_MASK;
            aJoinMethod->flag |= QMO_JOIN_METHOD_NL;

            // Join Method Cost의 초기화
            IDE_TEST( initJoinMethodCost4NL( aStatement,
                                             aGraph,
                                             aJoinPredicate,
                                             sLateralDirection,
                                             & sJoinMethodCost,
                                             & sJoinMethodCnt )
                      != IDE_SUCCESS );
            break;
        case QMO_JOIN_METHOD_HASH :
            aJoinMethod->flag &= ~QMO_JOIN_METHOD_MASK;
            aJoinMethod->flag |= QMO_JOIN_METHOD_HASH;

            // Join Method Cost의 초기화
            IDE_TEST( initJoinMethodCost4Hash( aStatement,
                                               aGraph,
                                               aJoinPredicate,
                                               sLateralDirection,
                                               & sJoinMethodCost,
                                               & sJoinMethodCnt )
                      != IDE_SUCCESS );
            break;

        case QMO_JOIN_METHOD_SORT :
            aJoinMethod->flag &= ~QMO_JOIN_METHOD_MASK;
            aJoinMethod->flag |= QMO_JOIN_METHOD_SORT;

            // Join Method Cost의 초기화
            IDE_TEST( initJoinMethodCost4Sort( aStatement,
                                               aGraph,
                                               aJoinPredicate,
                                               sLateralDirection,
                                               & sJoinMethodCost,
                                               & sJoinMethodCnt)
                      != IDE_SUCCESS );
            break;
        case QMO_JOIN_METHOD_MERGE :
            aJoinMethod->flag &= ~QMO_JOIN_METHOD_MASK;
            aJoinMethod->flag |= QMO_JOIN_METHOD_MERGE;

            // Join Method Cost의 초기화
            IDE_TEST( initJoinMethodCost4Merge( aStatement,
                                                aGraph,
                                                aJoinPredicate,
                                                sLateralDirection,
                                                & sJoinMethodCost,
                                                & sJoinMethodCnt)
                      != IDE_SUCCESS );
            break;
        default :
            IDE_FT_ASSERT( 0 );
            break;
    }

    for( sCount=0; sCount < sJoinMethodCnt; ++sCount)
    {
        sJoinMethodCost[sCount].firstRowsFactor = aFirstRowsFactor;
    }

    aJoinMethod->joinMethodCost  = sJoinMethodCost;
    aJoinMethod->joinMethodCnt   = sJoinMethodCnt;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoJoinMethodMgr::getBestJoinMethodCost( qcStatement   * aStatement,
                                         qmgGraph      * aGraph,
                                         qmoPredicate  * aJoinPredicate,
                                         qmoJoinMethod * aJoinMethod )
{
    /********************************************************************
     *
     * Description : Join Method Cost 중에 가장 cost가 좋은 method 선택
     *
     * Implementation :
     *    (1) Join Ordered Hint 처리
     *        left->right 방향만이 feasibility를 만족하므로
     *        right->left 방향의 feasibility를 FALSE로 설정
     *    (2) 각 Join Method Cost의 cost를 구한다.
     *    (3) Join Method 결정
     *        - Join Method Hint가 존재하는 경우 :
     *          A. Join Method Hint를 만족하는 것 중에서 가장 cost가 좋은 것 선택
     *          B. Join Method Hint를 만족하는 것이 없는 경우
     *             hint가 존재하지 않는 것과 동일하게 처리
     *        - Join Method Hint가 존재하지 않는 경우 :
     *          가장 cost가 작은 JoinMethod를 selectedJoinMethod에 연결
     *          단, Index Nested Loop Join과 Anti Outer Join의 경우
     *          table access hint가 있는 경우 그에 따른 feasibility 검사
     *
     ***********************************************************************/

    qmsHints           * sHints;
    qmsJoinMethodHints * sJoinMethodHints;
    qmoJoinMethodCost  * sJoinMethodCost;
    qmoJoinMethodCost  * sSelected;
    qmoJoinMethodCost  * sSameTableOrder;
    qmoJoinMethodCost  * sDiffTableOrder;
    idBool               sCurrentHint;
    UInt                 sNoUseHintMask = 0;
    UInt                 i;

    IDU_FIT_POINT_FATAL( "qmoJoinMethodMgr::getBestJoinMethodCost::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aGraph != NULL );
    IDE_DASSERT( aJoinMethod != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sHints          = aGraph->myQuerySet->SFWGH->hints;
    sJoinMethodCost = aJoinMethod->joinMethodCost;
    sSelected       = NULL;
    sCurrentHint    = ID_TRUE;
    sSameTableOrder = NULL;
    sDiffTableOrder = NULL;

    //------------------------------------------
    // Join Ordered Hint 처리
    //    direction이 right->left인 access method의 feasibility를
    //    모두 false로 설정 ( left->right가 ordered 방향이므로 )
    //------------------------------------------

    // PROJ-1495
    // ordered hint와 push_pred hint가 적용되는 경우
    // push_pred hint가 ordered hint보다 우선적용되므로
    // ordered hint가 적용될 수 없다.
    if( sHints != NULL )
    {
        if( sHints->pushPredHint == NULL )
        {
            if ( sHints->joinOrderType == QMO_JOIN_ORDER_TYPE_ORDERED )
            {
                for ( i = 0; i < aJoinMethod->joinMethodCnt; i++ )
                {
                    // BUG-38701
                    sJoinMethodCost = & aJoinMethod->joinMethodCost[i];

                    if ( ( sJoinMethodCost->flag & QMO_JOIN_METHOD_DIRECTION_MASK )
                         == QMO_JOIN_METHOD_DIRECTION_RIGHTLEFT )
                    {
                        sJoinMethodCost->flag &= ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
                        sJoinMethodCost->flag |= QMO_JOIN_METHOD_FEASIBILITY_FALSE;
                    }
                    else
                    {
                        // Nothing To Do
                    }
                }
            }
            else
            {
                // Nothing To Do
            }
        }
        else
        {
            IDE_TEST( forcePushPredHint( aGraph,
                                         aJoinMethod )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // Nothing To Do
    }

    //------------------------------------------
    // recursive view를 join의 가장 왼쪽으로 변경
    //------------------------------------------

    // PROJ-2582 recursive with
    // push pred, ordered hint는 무시되었다.
    IDE_TEST( forceJoinOrder4RecursiveView( aGraph,
                                            aJoinMethod )
              != IDE_SUCCESS );

    //------------------------------------------
    // Join Method Cost 계산
    //------------------------------------------

    for ( i = 0; i < aJoinMethod->joinMethodCnt; i++ )
    {
        sJoinMethodCost = &aJoinMethod->joinMethodCost[i];

        if ( ( sJoinMethodCost->flag & QMO_JOIN_METHOD_FEASIBILITY_MASK )
             == QMO_JOIN_METHOD_FEASIBILITY_TRUE )
        {
            if ( ( sJoinMethodCost->flag & QMO_JOIN_METHOD_DIRECTION_MASK ) ==
                 QMO_JOIN_METHOD_DIRECTION_LEFTRIGHT )
            {
                IDE_TEST( getJoinCost( aStatement,
                                       sJoinMethodCost,
                                       aJoinPredicate,
                                       aGraph->left,
                                       aGraph->right )
                          != IDE_SUCCESS );
            }
            else
            {
                IDE_TEST( getJoinCost( aStatement,
                                       sJoinMethodCost,
                                       aJoinPredicate,
                                       aGraph->right,
                                       aGraph->left )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            // feasibility 가 FALSE인 경우
            // nothing to do
        }
    }

    //------------------------------------------
    // (1) Join Method Hint를 만족하는 Join Method를 찾음
    //    Hint를 만족하는 Join Method 중 가장 cost가 좋은 Method 정보를 가짐
    //     - hintJoinMethod : Table Order까지 Join Method Hint와 동일한
    //                        Method를 가리킴
    //     - hintJoinMethod2 : Join Method만 Join Method Hint와 동일한
    //                         Method를 가리킴
    //                        ( hintJoinMethod가 NULL일때만 존재 )
    // (2) Join Method Hint가 없는 경우
    //------------------------------------------

    if( sHints != NULL )
    {
        // BUG-42413 NO_USE hint 지원
        // NO_USE로 막힌 조인 메소드를 모두 찾아낸다.
        for ( sJoinMethodHints = sHints->joinMethod;
              sJoinMethodHints != NULL;
              sJoinMethodHints = sJoinMethodHints->next )
        {
            IDE_TEST( qmo::currentJoinDependencies( aGraph,
                                                    & sJoinMethodHints->depInfo,
                                                    & sCurrentHint )
                      != IDE_SUCCESS );

            if ( (sCurrentHint == ID_TRUE) &&
                 (sJoinMethodHints->isNoUse == ID_TRUE) )
            {
                sNoUseHintMask |= (sJoinMethodHints->flag & QMO_JOIN_METHOD_MASK);
            }
            else
            {
                // nothing to do
            }
        }

        for ( sJoinMethodHints = sHints->joinMethod;
              sJoinMethodHints != NULL;
              sJoinMethodHints = sJoinMethodHints->next )
        {
            IDE_TEST( qmo::currentJoinDependencies( aGraph,
                                                    & sJoinMethodHints->depInfo,
                                                    & sCurrentHint )
                      != IDE_SUCCESS );

            if ( sCurrentHint == ID_TRUE )
            {
                // 현재 적용해야할 Hint인 경우,
                // Hint와 동일한 Join Method를 찾음
                IDE_TEST( setJoinMethodHint( aJoinMethod,
                                             sJoinMethodHints,
                                             aGraph,
                                             sNoUseHintMask,
                                             & sSameTableOrder,
                                             & sDiffTableOrder )
                          != IDE_SUCCESS );

                if ( sSameTableOrder != NULL )
                {
                    if ( aJoinMethod->hintJoinMethod == NULL )
                    {
                        aJoinMethod->hintJoinMethod = sSameTableOrder;
                    }
                    else
                    {
                        /* BUG-40589 floating point calculation */
                        if (QMO_COST_IS_GREATER(aJoinMethod->hintJoinMethod->totalCost,
                                                sSameTableOrder->totalCost)
                            == ID_TRUE)
                        {
                            aJoinMethod->hintJoinMethod = sSameTableOrder;
                        }
                        else
                        {
                            // nothing to do
                        }
                    }
                    aJoinMethod->hintJoinMethod2 = NULL;
                }
                else
                {
                    if ( sDiffTableOrder != NULL )
                    {
                        if ( aJoinMethod->hintJoinMethod == NULL )
                        {
                            if ( aJoinMethod->hintJoinMethod2 == NULL )
                            {
                                aJoinMethod->hintJoinMethod2 = sDiffTableOrder;
                            }
                            else
                            {
                                /* BUG-40589 floating point calculation */
                                if (QMO_COST_IS_GREATER(aJoinMethod->hintJoinMethod2->totalCost,
                                                        sDiffTableOrder->totalCost)
                                    == ID_TRUE)
                                {
                                    aJoinMethod->hintJoinMethod2 = sDiffTableOrder;
                                }
                                else
                                {
                                    // nothing to do
                                }
                            }
                        }
                        else
                        {
                            // nothing to do
                        }
                    }
                }
            }
            else
            {
                // 현재 Join과 관련없는 Hint
                // nothing to do
            }
        }
    } 
    else
    {
        // Nothing to do.
    }

    //------------------------------------------
    // Join Method Hint가 없는 경우,
    // Join Method Cost가 가장 좋은 Join Method 선택
    //------------------------------------------

    if ( aJoinMethod->hintJoinMethod == NULL &&
         aJoinMethod->hintJoinMethod2 == NULL )
    {
        for ( i = 0; i < aJoinMethod->joinMethodCnt; i++ )
        {
            sJoinMethodCost = & aJoinMethod->joinMethodCost[i];

            if ( ( sJoinMethodCost->flag & QMO_JOIN_METHOD_FEASIBILITY_MASK )
                 == QMO_JOIN_METHOD_FEASIBILITY_TRUE )
            {
                // feasibility가 TRUE 인 경우
                if ( sSelected == NULL )
                {
                    sSelected = sJoinMethodCost;
                }
                else
                {
                    /* BUG-40589 floating point calculation */
                    if (QMO_COST_IS_GREATER(sSelected->totalCost,
                                            sJoinMethodCost->totalCost)
                        == ID_TRUE)
                    {
                        sSelected = sJoinMethodCost;
                    }
                    else
                    {
                        // nothing to do
                    }
                }
            }
            else
            {
                // nothing to do
            }
        }
    }
    else
    {
        // nothing to do
    }

    aJoinMethod->selectedJoinMethod = sSelected;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoJoinMethodMgr::printJoinMethod( qmoJoinMethod * aMethod,
                                   ULong           aDepth,
                                   iduVarString  * aString )
{
/***********************************************************************
 *
 * Description :
 *    Join Method 정보를 출력한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    UInt  i;
    UInt  j;

    IDU_FIT_POINT_FATAL( "qmoJoinMethodMgr::printJoinMethod::__FT__" );

    //-----------------------------------
    // 적합성 검사
    //-----------------------------------

    IDE_DASSERT( aMethod != NULL );
    IDE_DASSERT( aString != NULL );

    //-----------------------------------
    // Join Method의 정보 출력
    //-----------------------------------

    for ( j = 0; j < aMethod->joinMethodCnt; j++ )
    {
        if ( ( aMethod->joinMethodCost[j].flag &
               QMO_JOIN_METHOD_FEASIBILITY_MASK )
             == QMO_JOIN_METHOD_FEASIBILITY_TRUE )
        {
            // 수행 가능한 Join Method

            //-----------------------------------
            // Join Method의 종류 출력
            //-----------------------------------

            QMG_PRINT_LINE_FEED( i, aDepth, aString );

            switch ( aMethod->joinMethodCost[j].flag &
                     QMO_JOIN_METHOD_MASK )
            {
                case QMO_JOIN_METHOD_FULL_NL       :
                    iduVarStringAppend( aString,
                                        "FULL_NESTED_LOOP[" );
                    break;
                case QMO_JOIN_METHOD_INDEX         :
                    iduVarStringAppend( aString,
                                        "INDEX_NESTED_LOOP[" );
                    iduVarStringAppendFormat( aString, "%s,",
                                              aMethod->joinMethodCost[j].
                                              rightIdxInfo->index->name );
                    break;
                case QMO_JOIN_METHOD_INVERSE_INDEX :
                    iduVarStringAppend( aString,
                                        "INVERSE_INDEX_NESTED_LOOP[" );
                    iduVarStringAppendFormat( aString, "%s,",
                                              aMethod->joinMethodCost[j].
                                              rightIdxInfo->index->name );
                    break;
                case QMO_JOIN_METHOD_FULL_STORE_NL :
                    iduVarStringAppend( aString,
                                        "FULL_STORE_NESTED_LOOP[" );
                    break;
                case QMO_JOIN_METHOD_ANTI          :
                    iduVarStringAppend( aString,
                                        "ANTI_OUTER_NESTED_LOOP[" );
                    iduVarStringAppendFormat( aString, "%s, %s",
                                              aMethod->joinMethodCost[j]
                                              .leftIdxInfo->index->name,
                                              aMethod->joinMethodCost[j]
                                              .rightIdxInfo->index->name );
                    break;
                case QMO_JOIN_METHOD_ONE_PASS_HASH :
                    iduVarStringAppend( aString,
                                        "ONE_PASS_HASH[" );
                    break;
                case QMO_JOIN_METHOD_TWO_PASS_HASH :
                    iduVarStringAppend( aString,
                                        "MULTI_PASS_HASH[" );
                    break;
                case QMO_JOIN_METHOD_INVERSE_HASH :
                    iduVarStringAppend( aString,
                                        "INVERSE_HASH[" );
                    break;
                case QMO_JOIN_METHOD_ONE_PASS_SORT :
                    iduVarStringAppend( aString,
                                        "ONE_PASS_SORT[" );
                    break;
                case QMO_JOIN_METHOD_TWO_PASS_SORT :
                    iduVarStringAppend( aString,
                                        "MULTI_PASS_SORT[" );
                    break;
                case QMO_JOIN_METHOD_INVERSE_SORT :
                    iduVarStringAppend( aString,
                                        "INVERSE_SORT[" );
                    break;
                case QMO_JOIN_METHOD_MERGE         :
                    iduVarStringAppend( aString,
                                        "MERGE[" );
                    break;
                default                            :
                    IDE_FT_ASSERT( 0 );
                    break;
            }

            //-----------------------------------
            // Join 방향의 종류 출력
            //-----------------------------------

            if ( ( aMethod->joinMethodCost[j].flag &
                   QMO_JOIN_METHOD_DIRECTION_MASK )
                 == QMO_JOIN_METHOD_DIRECTION_LEFTRIGHT )
            {
                iduVarStringAppend( aString,
                                    "LEFT=>RIGHT]" );
            }
            else
            {
                iduVarStringAppend( aString,
                                    "RIGHT=>LEFT]" );
            }

            //-----------------------------------
            // Join 정보의 출력
            //-----------------------------------

            // Selectivity
            QMG_PRINT_LINE_FEED( i, aDepth, aString );
            iduVarStringAppendFormat( aString,
                                      "  SELECTIVITY : %"ID_PRINT_G_FMT,
                                      aMethod->joinMethodCost[j].selectivity );

            QMG_PRINT_LINE_FEED( i, aDepth, aString );
            iduVarStringAppendFormat( aString,
                                      "  ACCESS_COST : %"ID_PRINT_G_FMT,
                                      aMethod->joinMethodCost[j].accessCost );

            QMG_PRINT_LINE_FEED( i, aDepth, aString );
            iduVarStringAppendFormat( aString,
                                      "  DISK_COST   : %"ID_PRINT_G_FMT,
                                      aMethod->joinMethodCost[j].diskCost );

            QMG_PRINT_LINE_FEED( i, aDepth, aString );
            iduVarStringAppendFormat( aString,
                                      "  TOTAL_COST  : %"ID_PRINT_G_FMT,
                                      aMethod->joinMethodCost[j].totalCost );

        }
        else
        {
            // 수행이 불가능한 Join Method임
        }
    }

    return IDE_SUCCESS;
}


IDE_RC
qmoJoinMethodMgr::getJoinCost( qcStatement       * aStatement,
                               qmoJoinMethodCost * aMethod,
                               qmoPredicate      * aJoinPredicate,
                               qmgGraph          * aLeft,
                               qmgGraph          * aRight)
{
/***********************************************************************
 *
 * Description : Join Cost 계산기
 *
 * Implementation :
 *    Join Method Type에 맞는 Join Cost 계산기를 호출한다.
 *
 ***********************************************************************/

    IDU_FIT_POINT_FATAL( "qmoJoinMethodMgr::getJoinCost::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------
    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aMethod != NULL );
    IDE_DASSERT( aLeft != NULL );
    IDE_DASSERT( aRight != NULL );

    //------------------------------------------
    // Cost 계산
    //------------------------------------------
    switch( aMethod->flag & QMO_JOIN_METHOD_MASK )
    {
        case QMO_JOIN_METHOD_FULL_NL :
            IDE_TEST( getFullNestedLoop( aStatement,
                                         aMethod,
                                         aJoinPredicate,
                                         aLeft,
                                         aRight )
                      != IDE_SUCCESS );
            break;
        case QMO_JOIN_METHOD_INDEX :
            IDE_TEST( getIndexNestedLoop( aStatement,
                                          aMethod,
                                          aJoinPredicate,
                                          aLeft,
                                          aRight )
                      != IDE_SUCCESS );
            break;
        case QMO_JOIN_METHOD_INVERSE_INDEX :
            IDE_TEST( getIndexNestedLoop( aStatement,
                                          aMethod,
                                          aJoinPredicate,
                                          aLeft,
                                          aRight )
                      != IDE_SUCCESS );
            break;
        case QMO_JOIN_METHOD_FULL_STORE_NL :
            IDE_TEST( getFullStoreNestedLoop( aStatement,
                                              aMethod,
                                              aJoinPredicate,
                                              aLeft,
                                              aRight )
                      != IDE_SUCCESS );
            break;
        case QMO_JOIN_METHOD_ANTI :
            IDE_TEST( getAntiOuter( aStatement,
                                    aMethod,
                                    aJoinPredicate,
                                    aLeft,
                                    aRight )
                != IDE_SUCCESS );
            break;
        case QMO_JOIN_METHOD_ONE_PASS_HASH :
            IDE_TEST( getOnePassHash( aStatement,
                                      aMethod,
                                      aJoinPredicate,
                                      aLeft,
                                      aRight )
                      != IDE_SUCCESS );
            break;
        case QMO_JOIN_METHOD_TWO_PASS_HASH :
            IDE_TEST( getTwoPassHash( aStatement,
                                      aMethod,
                                      aJoinPredicate,
                                      aLeft,
                                      aRight )
                      != IDE_SUCCESS );
            break;
        case QMO_JOIN_METHOD_INVERSE_HASH :
            IDE_TEST( getInverseHash( aStatement,
                                      aMethod,
                                      aJoinPredicate,
                                      aLeft,
                                      aRight )
                      != IDE_SUCCESS );
            break;
        case QMO_JOIN_METHOD_ONE_PASS_SORT :
            IDE_TEST( getOnePassSort( aStatement,
                                      aMethod,
                                      aJoinPredicate,
                                      aLeft,
                                      aRight )
                      != IDE_SUCCESS );
            break;
        case QMO_JOIN_METHOD_TWO_PASS_SORT :
            IDE_TEST( getTwoPassSort( aStatement,
                                      aMethod,
                                      aJoinPredicate,
                                      aLeft,
                                      aRight )
                      != IDE_SUCCESS );
            break;
        case QMO_JOIN_METHOD_INVERSE_SORT :
            IDE_TEST( getInverseSort( aStatement,
                                      aMethod,
                                      aJoinPredicate,
                                      aLeft,
                                      aRight )
                      != IDE_SUCCESS );
            break;
        case QMO_JOIN_METHOD_MERGE :
            IDE_TEST( getMerge( aStatement,
                                aMethod,
                                aJoinPredicate,
                                aLeft,
                                aRight )
                      != IDE_SUCCESS );
            break;
        default :
            IDE_FT_ASSERT( 0 );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoJoinMethodMgr::getFullNestedLoop( qcStatement       * aStatement,
                                     qmoJoinMethodCost * aJoinMethodCost,
                                     qmoPredicate      * aJoinPredicate,
                                     qmgGraph          * aLeft,
                                     qmgGraph          * aRight)
{
    SDouble         sFilterCost = 1;

    IDU_FIT_POINT_FATAL( "qmoJoinMethodMgr::getFullNestedLoop::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aJoinMethodCost != NULL );
    IDE_DASSERT( aLeft != NULL );
    IDE_DASSERT( aRight != NULL );

    aJoinMethodCost->flag &= ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
    aJoinMethodCost->flag |= QMO_JOIN_METHOD_FEASIBILITY_TRUE;

    // left 노드의 cost 값을 낮춘다.
    QMO_FIRST_ROWS_SET(aLeft, aJoinMethodCost);

    sFilterCost = qmoCost::getFilterCost( aStatement->mSysStat,
                                          aJoinPredicate,
                                          (aLeft->costInfo.outputRecordCnt *
                                           aRight->costInfo.outputRecordCnt) );

    // BUG-37407 semi, anti join cost
    if( (aJoinMethodCost->flag & QMO_JOIN_SEMI_MASK)
         == QMO_JOIN_SEMI_TRUE )
    {
        aJoinMethodCost->totalCost =
                qmoCost::getFullNestedJoin4SemiCost( aLeft,
                                                     aRight,
                                                     aJoinMethodCost->selectivity,
                                                     sFilterCost,
                                                     &(aJoinMethodCost->accessCost),
                                                     &(aJoinMethodCost->diskCost) );
    }
    else if( (aJoinMethodCost->flag & QMO_JOIN_ANTI_MASK)
              == QMO_JOIN_ANTI_TRUE )
    {
        aJoinMethodCost->totalCost =
                qmoCost::getFullNestedJoin4AntiCost( aLeft,
                                                     aRight,
                                                     aJoinMethodCost->selectivity,
                                                     sFilterCost,
                                                     &(aJoinMethodCost->accessCost),
                                                     &(aJoinMethodCost->diskCost) );
    }
    else
    {
        aJoinMethodCost->totalCost =
                qmoCost::getFullNestedJoinCost( aLeft,
                                                aRight,
                                                sFilterCost,
                                                &(aJoinMethodCost->accessCost),
                                                &(aJoinMethodCost->diskCost) );

        /* BUG-48234 Recursive With Join cost adjust */
        if ( ( ( aLeft->myQuerySet->lflag & QMV_QUERYSET_RECURSIVE_VIEW_MASK )
              == QMV_QUERYSET_RECURSIVE_VIEW_LEFT ) ||
             ( ( aLeft->myQuerySet->lflag & QMV_QUERYSET_RECURSIVE_VIEW_MASK )
              == QMV_QUERYSET_RECURSIVE_VIEW_RIGHT ) ||
             ( ( aRight->myQuerySet->lflag & QMV_QUERYSET_RECURSIVE_VIEW_MASK )
              == QMV_QUERYSET_RECURSIVE_VIEW_LEFT ) ||
             ( ( aRight->myQuerySet->lflag & QMV_QUERYSET_RECURSIVE_VIEW_MASK )
              == QMV_QUERYSET_RECURSIVE_VIEW_RIGHT ) )
        {
            aJoinMethodCost->totalCost = aJoinMethodCost->totalCost * 8;
        }
    }

    // left 노드의 cost 값을 복원한다.
    QMO_FIRST_ROWS_UNSET(aLeft, aJoinMethodCost);

    return IDE_SUCCESS;
}

IDE_RC
qmoJoinMethodMgr::getFullStoreNestedLoop( qcStatement       * aStatement,
                                          qmoJoinMethodCost * aJoinMethodCost,
                                          qmoPredicate      * aJoinPredicate,
                                          qmgGraph          * aLeft,
                                          qmgGraph          * aRight)
{
    SDouble sFilterCost = 1;
    idBool  sIsDisk;

    IDU_FIT_POINT_FATAL( "qmoJoinMethodMgr::getFullStoreNestedLoop::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aJoinMethodCost != NULL );
    IDE_DASSERT( aLeft != NULL );
    IDE_DASSERT( aRight != NULL );

    //------------------------------------------
    // 저장 매체의 결정
    //------------------------------------------

    IDE_TEST( qmg::isDiskTempTable( aRight, & sIsDisk )
              != IDE_SUCCESS );

    if ( sIsDisk == ID_TRUE )
    {
        // Child의 저장 매체 결정
        aJoinMethodCost->flag &= ~QMO_JOIN_METHOD_RIGHT_STORAGE_MASK;
        aJoinMethodCost->flag |= QMO_JOIN_METHOD_RIGHT_STORAGE_DISK;
    }
    else
    {
        // Child의 저장 매체 결정
        aJoinMethodCost->flag &= ~QMO_JOIN_METHOD_RIGHT_STORAGE_MASK;
        aJoinMethodCost->flag |= QMO_JOIN_METHOD_RIGHT_STORAGE_MEMORY;
    }

    //------------------------------------------
    // Cost 의 계산
    //------------------------------------------
    QMO_FIRST_ROWS_SET(aLeft, aJoinMethodCost);

    sFilterCost = qmoCost::getFilterCost( aStatement->mSysStat,
                                          aJoinPredicate,
                                          (aLeft->costInfo.outputRecordCnt *
                                           aRight->costInfo.outputRecordCnt) );

    aJoinMethodCost->totalCost =
            qmoCost::getFullStoreJoinCost( aLeft,
                                           aRight,
                                           sFilterCost,
                                           aStatement->mSysStat,
                                           sIsDisk,
                                           &(aJoinMethodCost->accessCost),
                                           &(aJoinMethodCost->diskCost) );

    /* BUG-48234 Recursive With Join cost adjust */
    if ( ( ( aLeft->myQuerySet->lflag & QMV_QUERYSET_RECURSIVE_VIEW_MASK )
          == QMV_QUERYSET_RECURSIVE_VIEW_LEFT ) ||
         ( ( aLeft->myQuerySet->lflag & QMV_QUERYSET_RECURSIVE_VIEW_MASK )
          == QMV_QUERYSET_RECURSIVE_VIEW_RIGHT ) ||
         ( ( aRight->myQuerySet->lflag & QMV_QUERYSET_RECURSIVE_VIEW_MASK )
          == QMV_QUERYSET_RECURSIVE_VIEW_LEFT ) ||
         ( ( aRight->myQuerySet->lflag & QMV_QUERYSET_RECURSIVE_VIEW_MASK )
          == QMV_QUERYSET_RECURSIVE_VIEW_RIGHT ) )
    {
        aJoinMethodCost->totalCost = aJoinMethodCost->totalCost * 4;
    }

    QMO_FIRST_ROWS_UNSET(aLeft, aJoinMethodCost);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoJoinMethodMgr::getIndexNestedLoop( qcStatement       * aStatement,
                                      qmoJoinMethodCost * aJoinMethodCost,
                                      qmoPredicate      * aJoinPredicate,
                                      qmgGraph          * aLeft,
                                      qmgGraph          * aRight)
{
    UInt            i;
    SDouble         sRightIndexMemCost  = 0;
    SDouble         sRightIndexDiskCost = 0;
    SDouble         sTempCost           = 0;
    SDouble         sTempMemCost        = 0;
    SDouble         sTempDiskCost       = 0;

    // BUG-36450
    SDouble         sMinCost            = QMO_COST_INVALID_COST;
    SDouble         sMinMemCost         = 0;
    SDouble         sMinDiskCost        = 0;
    idBool          sIsDisk;
    qmoPredInfo   * sJoinPredicate      = NULL;
    qmoIdxCardInfo* sIdxInfo            = NULL;

    UInt              sAccessMethodCnt;
    qmoAccessMethod * sAccessMethod;
    // BUG-45169
    SDouble           sMinIndexCost     = QMO_COST_INVALID_COST;
    SDouble           sCurrIndexCost    = QMO_COST_INVALID_COST;
    idBool            sIsChange         = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qmoJoinMethodMgr::getIndexNestedLoop::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aJoinMethodCost != NULL );
    IDE_DASSERT( aLeft != NULL );
    IDE_DASSERT( aRight != NULL );

    //------------------------------------------
    // 저장 매체의 결정
    //------------------------------------------

    // 저장 매체 판단
    IDE_TEST( qmg::isDiskTempTable( aLeft, & sIsDisk )
              != IDE_SUCCESS );

    if ( sIsDisk == ID_TRUE )
    {
        // Child의 저장 매체 결정
        aJoinMethodCost->flag &= ~QMO_JOIN_METHOD_LEFT_STORAGE_MASK;
        aJoinMethodCost->flag |= QMO_JOIN_METHOD_LEFT_STORAGE_DISK;
    }
    else
    {
        // Child의 저장 매체 결정
        aJoinMethodCost->flag &= ~QMO_JOIN_METHOD_LEFT_STORAGE_MASK;
        aJoinMethodCost->flag |= QMO_JOIN_METHOD_LEFT_STORAGE_MEMORY;
    }

    if( aRight->type == QMG_SELECTION )
    {
        sAccessMethodCnt = ((qmgSELT*) aRight)->accessMethodCnt;
        sAccessMethod    = ((qmgSELT*) aRight)->accessMethod;
    }
    else if( aRight->type == QMG_PARTITION )
    {
        sAccessMethodCnt = ((qmgPARTITION*) aRight)->accessMethodCnt;
        sAccessMethod    = ((qmgPARTITION*) aRight)->accessMethod;
    }
    else if ( aRight->type == QMG_SHARD_SELECT )
    {
        // PROJ-2638
        sAccessMethodCnt = ((qmgShardSELT*) aRight)->accessMethodCnt;
        sAccessMethod    = ((qmgShardSELT*) aRight)->accessMethod;
    }
    else
    {
        sAccessMethodCnt = 0;
        sAccessMethod    = NULL;
    }

    QMO_FIRST_ROWS_SET(aLeft, aJoinMethodCost);

    // BUG-36454 index hint 를 사용하면 sAccessMethod[0] 은 fullscan 아니다.
    for ( i = 0; i < sAccessMethodCnt; i++ )
    {
        sIsChange       = ID_FALSE;
        sCurrIndexCost  = QMO_COST_INVALID_COST;

        if ( sAccessMethod[i].method != NULL )
        {
            // keyRange 만 추출해낸다.
            IDE_TEST ( setJoinPredInfo4NL( aStatement,
                                           aJoinPredicate,
                                           sAccessMethod[i].method,
                                           aRight,
                                           aJoinMethodCost )
                       != IDE_SUCCESS );

            if ( aJoinMethodCost->joinPredicate != NULL )
            {
                // BUG-42429 index cost 를 다시 계산해야함
                adjustIndexCost( aStatement,
                                 aRight,
                                 sAccessMethod[i].method,
                                 aJoinMethodCost->joinPredicate,
                                 &sRightIndexMemCost,
                                 &sRightIndexDiskCost );

                // BUG-37407 semi, anti join cost
                if ( (aJoinMethodCost->flag & QMO_JOIN_SEMI_MASK)
                    == QMO_JOIN_SEMI_TRUE )
                {
                    if ( ( aJoinMethodCost->flag & QMO_JOIN_METHOD_MASK )
                           == QMO_JOIN_METHOD_INVERSE_INDEX )
                    {
                        sTempCost =
                        qmoCost::getInverseIndexNestedJoinCost( aLeft,
                                                                aRight->myFrom->tableRef->statInfo,
                                                                sAccessMethod[i].method,
                                                                aStatement->mSysStat,
                                                                sIsDisk,
                                                                0, // filter cost
                                                                &sTempMemCost,
                                                                &sTempDiskCost );

                        // BUG-45169 semi inverse index NL join 일 때만, index cost를 고려할 필요가 있습니다.
                        sCurrIndexCost = sRightIndexMemCost + sRightIndexDiskCost;
                        IDE_DASSERT(QMO_COST_IS_LESS(sCurrIndexCost, 0.0) == ID_FALSE);

                    }
                    else
                    {
                        sTempCost =
                        qmoCost::getIndexNestedJoin4SemiCost( aLeft,
                                                              aRight->myFrom->tableRef->statInfo,
                                                              sAccessMethod[i].method,
                                                              aStatement->mSysStat,
                                                              0, // filter cost
                                                              &sTempMemCost,
                                                              &sTempDiskCost );
                        // BUG-48327 semi join wrong index
                        sCurrIndexCost = sRightIndexMemCost + sRightIndexDiskCost;
                    }
                }
                else if ( (aJoinMethodCost->flag & QMO_JOIN_ANTI_MASK)
                          == QMO_JOIN_ANTI_TRUE )
                {
                    sTempCost =
                    qmoCost::getIndexNestedJoin4AntiCost( aLeft,
                                                          aRight->myFrom->tableRef->statInfo,
                                                          sAccessMethod[i].method,
                                                          aStatement->mSysStat,
                                                          0, // filter cost
                                                          &sTempMemCost,
                                                          &sTempDiskCost );
                        // BUG-48327 semi join wrong index
                        sCurrIndexCost = sRightIndexMemCost + sRightIndexDiskCost;
                }
                else
                {
                    sTempCost =
                    qmoCost::getIndexNestedJoinCost( aLeft,
                                                     aRight->myFrom->tableRef->statInfo,
                                                     sAccessMethod[i].method,
                                                     aStatement->mSysStat,
                                                     sRightIndexMemCost,
                                                     sRightIndexDiskCost,
                                                     0, // filter cost
                                                     &sTempMemCost,
                                                     &sTempDiskCost );
                }

                // 최소 Cost 인 index 를 저장한다.
                if (((QMO_COST_IS_EQUAL(sMinCost, QMO_COST_INVALID_COST) == ID_TRUE) ||
                     (QMO_COST_IS_LESS(sTempCost, sMinCost) == ID_TRUE)))
                {
                    sIsChange = ID_TRUE;
                }
                else
                {
                    /* BUG-44850 Index NL , Inverse index NL 조인 최적화 수행시 비용이 동일하면 primary key를 우선적으로 선택.
                         0 ( 7.1.0 default ) : primary key 우선적으로 선택 (BUG-44850) +
                             Inverse index NL 조인일 때 index cost가 작은 인덱스 선택 (BUG-45169)
                         1 ( 6.5.1. default ): 기존 플랜 방식과 동일
                         2 ( trunk defulat ) : primary key 우선적으로 선택 (BUG-44850) +
                             Inverse index NL 조인일 때 index cost가 작은 인덱스 선택 (BUG-45169) +
                             index NL 조인 일때 index cost가 작은 인덱스 선택 + ( BUG-48327 )
                             Anti 조인 일때 index cost가 작은 인덱스 선택
                    */
                    if ( (QCU_OPTIMIZER_INDEX_NL_JOIN_ACCESS_METHOD_POLICY != 1) &&
                         (QMO_COST_IS_EQUAL( sTempCost, sMinCost ) == ID_TRUE) )
                    {
                        if ( QCU_OPTIMIZER_INDEX_NL_JOIN_ACCESS_METHOD_POLICY == 0 )
                        {
                            // BUG-45169 semi inverse index NL join 일 때만, index cost를 고려할 필요가 있습니다.
                            if ( ( aJoinMethodCost->flag & QMO_JOIN_METHOD_MASK ) 
                                 == QMO_JOIN_METHOD_INVERSE_INDEX )
                            {
                                if ( QMO_COST_IS_LESS(sCurrIndexCost, sMinIndexCost) == ID_TRUE )
                                {
                                    // sMinIndexCost는 절대 QMO_COST_INVALID_COST 일 수 없다.
                                    IDE_DASSERT(QMO_COST_IS_LESS(sMinIndexCost, 0.0) == ID_FALSE);
                                    sIsChange = ID_TRUE;
                                }
                                else
                                {
                                    /* BUG-44850 Inverse index NL 조인 최적화 수행시 비용이 동일하면 primary key를 우선적으로 선택. */
                                    if ( ( QMO_COST_IS_EQUAL( sCurrIndexCost, sMinIndexCost) == ID_TRUE ) &&
                                         ( (sAccessMethod[i].method->flag & QMO_STAT_CARD_IDX_PRIMARY_MASK) ==
                                           QMO_STAT_CARD_IDX_PRIMARY_TRUE ) )
                                    {
                                        sIsChange = ID_TRUE; 
                                    }
                                    else
                                    {
                                        // Nothing to do.
                                    }
                                }
                            }
                            else
                            {
                                /* BUG-44850 Index NL 조인 최적화 수행시 비용이 동일하면 primary key를 우선적으로 선택. */
                                if ( (sAccessMethod[i].method->flag & QMO_STAT_CARD_IDX_PRIMARY_MASK) ==
                                     QMO_STAT_CARD_IDX_PRIMARY_TRUE )
                                {
                                    sIsChange = ID_TRUE; 
                                }
                                else
                                {
                                    // Nothing to do.
                                }
                            }
                        }
                        else if ( QCU_OPTIMIZER_INDEX_NL_JOIN_ACCESS_METHOD_POLICY == 2 )
                        {
                            if ( QMO_COST_IS_LESS(sCurrIndexCost, sMinIndexCost) == ID_TRUE )
                            {
                                // sMinIndexCost는 절대 QMO_COST_INVALID_COST 일 수 없다.
                                IDE_DASSERT(QMO_COST_IS_LESS(sMinIndexCost, 0.0) == ID_FALSE);
                                sIsChange = ID_TRUE;
                            }
                            else
                            {
                                if ( ( QMO_COST_IS_EQUAL( sCurrIndexCost, sMinIndexCost) == ID_TRUE ) &&
                                     ( (sAccessMethod[i].method->flag & QMO_STAT_CARD_IDX_PRIMARY_MASK) ==
                                       QMO_STAT_CARD_IDX_PRIMARY_TRUE ) )
                                {
                                    sIsChange = ID_TRUE;
                                }
                            }
                        }
                    }
                    else
                    {
                        // Nothing to do.
                    }

                    qcgPlan::registerPlanProperty( aStatement,
                                                   PLAN_PROPERTY_OPTIMIZER_INDEX_NL_JOIN_ACCESS_METHOD_POLICY );
                }

                if ( sIsChange == ID_TRUE )
                {
                    sMinCost        = sTempCost;
                    sMinMemCost     = sTempMemCost;
                    sMinDiskCost    = sTempDiskCost;
                    sJoinPredicate  = aJoinMethodCost->joinPredicate;
                    sIdxInfo        = sAccessMethod[i].method;
                    sMinIndexCost   = sCurrIndexCost;
                }
            }
            else
            {
                // nothing to do
            }
        }
        else
        {
            // nothing to do
        }
    }
    QMO_FIRST_ROWS_UNSET(aLeft, aJoinMethodCost);

    if (QMO_COST_IS_EQUAL(sMinCost, QMO_COST_INVALID_COST) == ID_FALSE)
    {
        IDE_DASSERT(QMO_COST_IS_LESS(sMinCost, 0.0) == ID_FALSE);

        // BUG-39403
        // INVERSE_INDEX Join Method 에서는,
        // Predicate List의 모든 Join Predicate의
        // 비교 연산자 양 쪽 operand가 모두 Column Node여야 한다.
        if ( ( ( aJoinMethodCost->flag & QMO_JOIN_METHOD_MASK )
               == QMO_JOIN_METHOD_INVERSE_INDEX ) &&
             ( qmoPred::hasOnlyColumnCompInPredList( sJoinPredicate ) == ID_FALSE ) )
        {
            aJoinMethodCost->flag &= ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
            aJoinMethodCost->flag |= QMO_JOIN_METHOD_FEASIBILITY_FALSE;
        }
        else
        {
            // INVERSE_INDEX Join Method가 아니거나,
            // INVERSE_INDEX 이고 Join Predicate이 모두 Column Node를 비교하는 경우
            aJoinMethodCost->flag &= ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
            aJoinMethodCost->flag |= QMO_JOIN_METHOD_FEASIBILITY_TRUE;

            aJoinMethodCost->accessCost   = sMinMemCost;
            aJoinMethodCost->diskCost     = sMinDiskCost;
            aJoinMethodCost->totalCost    = sMinCost;

            /* BUG-48234 Recursive With Join cost adjust */
            if ( ( ( aLeft->myQuerySet->lflag & QMV_QUERYSET_RECURSIVE_VIEW_MASK )
                  == QMV_QUERYSET_RECURSIVE_VIEW_LEFT ) ||
                 ( ( aLeft->myQuerySet->lflag & QMV_QUERYSET_RECURSIVE_VIEW_MASK )
                  == QMV_QUERYSET_RECURSIVE_VIEW_RIGHT ) ||
                 ( ( aRight->myQuerySet->lflag & QMV_QUERYSET_RECURSIVE_VIEW_MASK )
                  == QMV_QUERYSET_RECURSIVE_VIEW_LEFT ) ||
                 ( ( aRight->myQuerySet->lflag & QMV_QUERYSET_RECURSIVE_VIEW_MASK )
                  == QMV_QUERYSET_RECURSIVE_VIEW_RIGHT ) )
            {
                aJoinMethodCost->totalCost = aJoinMethodCost->totalCost * 2;
            }
            IDE_DASSERT( sJoinPredicate != NULL );
            IDE_DASSERT( sIdxInfo != NULL );

            aJoinMethodCost->joinPredicate = sJoinPredicate;
            aJoinMethodCost->rightIdxInfo  = sIdxInfo;
        }
    }
    else
    {
        aJoinMethodCost->flag &= ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
        aJoinMethodCost->flag |= QMO_JOIN_METHOD_FEASIBILITY_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoJoinMethodMgr::getAntiOuter( qcStatement       * aStatement,
                                qmoJoinMethodCost * aJoinMethodCost,
                                qmoPredicate      * aJoinPredicate,
                                qmgGraph          * aLeft,
                                qmgGraph          * aRight )
{
    UInt            i;
    SDouble         sTempCost       = 0;
    SDouble         sTempMemCost    = 0;
    SDouble         sTempDiskCost   = 0;

    // BUG-36450
    SDouble         sMinCost        = QMO_COST_INVALID_COST;
    SDouble         sMinMemCost     = 0;
    SDouble         sMinDiskCost    = 0;
    SDouble         sFilterCost     = 1;
    qmoPredInfo   * sJoinPredicate  = NULL;
    qmoIdxCardInfo* sLeftIdxInfo    = NULL;
    qmoIdxCardInfo* sRightIdxInfo   = NULL;

    UInt              sAccessMethodCnt;
    qmoAccessMethod * sAccessMethod;

    IDU_FIT_POINT_FATAL( "qmoJoinMethodMgr::getAntiOuter::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aJoinMethodCost != NULL );
    IDE_DASSERT( aLeft != NULL );
    IDE_DASSERT( aRight != NULL );

    if( aRight->type == QMG_SELECTION )
    {
        sAccessMethodCnt = ((qmgSELT*) aRight)->accessMethodCnt;
        sAccessMethod    = ((qmgSELT*) aRight)->accessMethod;
    }
    else if( aRight->type == QMG_PARTITION )
    {
        sAccessMethodCnt = ((qmgPARTITION*) aRight)->accessMethodCnt;
        sAccessMethod    = ((qmgPARTITION*) aRight)->accessMethod;
    }
    else if ( aRight->type == QMG_SHARD_SELECT )
    {
        // PROJ-2638
        sAccessMethodCnt = ((qmgShardSELT*) aRight)->accessMethodCnt;
        sAccessMethod    = ((qmgShardSELT*) aRight)->accessMethod;
    }
    else
    {
        sAccessMethodCnt = 0;
        sAccessMethod    = NULL;
    }

    QMO_FIRST_ROWS_SET(aLeft, aJoinMethodCost);

    // BUG-36454 index hint 를 사용하면 sAccessMethod[0] 은 fullscan 아니다.
    for( i = 0; i < sAccessMethodCnt; i++ )
    {
        if ( sAccessMethod[i].method != NULL )
        {
            IDE_TEST( setJoinPredInfo4AntiOuter( aStatement,
                                                 aJoinPredicate,
                                                 sAccessMethod[i].method,
                                                 aRight,
                                                 aLeft,
                                                 aJoinMethodCost )
                         != IDE_SUCCESS );

            if( aJoinMethodCost->joinPredicate != NULL )
            {
                sTempCost =
                    qmoCost::getAntiOuterJoinCost( aLeft,
                                                   sAccessMethod[i].accessCost,
                                                   sAccessMethod[i].diskCost,
                                                   sFilterCost,
                                                   &sTempMemCost,
                                                   &sTempDiskCost );

                // 최소 Cost 인 index 를 저장한다.
                /* BUG-40589 floating point calculation */
                if ((QMO_COST_IS_EQUAL(sMinCost, QMO_COST_INVALID_COST) == ID_TRUE) ||
                    (QMO_COST_IS_LESS(sTempCost, sMinCost) == ID_TRUE))
                {
                    sMinCost        = sTempCost;
                    sMinMemCost     = sTempMemCost;
                    sMinDiskCost    = sTempDiskCost;
                    sJoinPredicate  = aJoinMethodCost->joinPredicate;
                    sLeftIdxInfo    = aJoinMethodCost->leftIdxInfo;
                    sRightIdxInfo   = sAccessMethod[i].method;
                }
                else
                {
                    // nothing to do
                }
            }
            else
            {
                // nothing to do
            }
        }
        else
        {
            // nothing to do
        }
    }
    QMO_FIRST_ROWS_UNSET(aLeft, aJoinMethodCost);

    /* BUG-40589 floating point calculation */
    if (QMO_COST_IS_GREATER(sMinCost, 0) == ID_TRUE)
    {
        aJoinMethodCost->flag &= ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
        aJoinMethodCost->flag |= QMO_JOIN_METHOD_FEASIBILITY_TRUE;

        aJoinMethodCost->accessCost   = sMinMemCost;
        aJoinMethodCost->diskCost     = sMinDiskCost;
        aJoinMethodCost->totalCost    = sMinCost;

        IDE_DASSERT( sJoinPredicate != NULL );
        IDE_DASSERT( sRightIdxInfo  != NULL );
        IDE_DASSERT( sLeftIdxInfo   != NULL );

        aJoinMethodCost->joinPredicate = sJoinPredicate;
        aJoinMethodCost->rightIdxInfo  = sRightIdxInfo;
        aJoinMethodCost->leftIdxInfo   = sLeftIdxInfo;
    }
    else
    {
        aJoinMethodCost->flag &= ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
        aJoinMethodCost->flag |= QMO_JOIN_METHOD_FEASIBILITY_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoJoinMethodMgr::getOnePassSort( qcStatement       * aStatement,
                                  qmoJoinMethodCost * aJoinMethodCost,
                                  qmoPredicate      * aJoinPredicate,
                                  qmgGraph          * aLeft,
                                  qmgGraph          * aRight)
{
    idBool  sIsDisk;
    idBool  sUseOrder;
    SDouble sFilterCost = 1;

    IDU_FIT_POINT_FATAL( "qmoJoinMethodMgr::getOnePassSort::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aJoinMethodCost != NULL );
    IDE_DASSERT( aLeft != NULL );
    IDE_DASSERT( aRight != NULL );

    //------------------------------------------
    // 저장 매체의 결정
    //------------------------------------------

    // 저장 매체 판단
    IDE_TEST( qmg::isDiskTempTable( aRight, & sIsDisk )
              != IDE_SUCCESS );

    if ( sIsDisk == ID_TRUE )
    {
        // Child의 저장 매체 결정
        aJoinMethodCost->flag &= ~QMO_JOIN_METHOD_RIGHT_STORAGE_MASK;
        aJoinMethodCost->flag |= QMO_JOIN_METHOD_RIGHT_STORAGE_DISK;
    }
    else
    {
        // Child의 저장 매체 결정
        aJoinMethodCost->flag &= ~QMO_JOIN_METHOD_RIGHT_STORAGE_MASK;
        aJoinMethodCost->flag |= QMO_JOIN_METHOD_RIGHT_STORAGE_MEMORY;
    }

    //------------------------------------------
    // Preserved Order의 사용 가능 여부 판단
    //------------------------------------------
    IDE_TEST( setJoinPredInfo4Sort( aStatement,
                                    aJoinPredicate,
                                    &(aRight->depInfo),
                                    aJoinMethodCost )
              != IDE_SUCCESS );

    if( aJoinMethodCost->joinPredicate != NULL )
    {
        IDE_TEST( canUsePreservedOrder( aStatement,
                                        aJoinMethodCost,
                                        aRight,
                                        QMO_JOIN_CHILD_RIGHT,
                                        & sUseOrder )
                  != IDE_SUCCESS );

        if ( sUseOrder == ID_TRUE )
        {
            // Child Node의 종류 결정
            aJoinMethodCost->flag &= ~QMO_JOIN_RIGHT_NODE_MASK;
            aJoinMethodCost->flag |= QMO_JOIN_RIGHT_NODE_STORE;
        }
        else
        {
            // Child Node의 종류 결정
            aJoinMethodCost->flag &= ~QMO_JOIN_RIGHT_NODE_MASK;
            aJoinMethodCost->flag |= QMO_JOIN_RIGHT_NODE_SORTING;
        }

        //------------------------------------------
        // Cost 의 계산
        //------------------------------------------

        QMO_FIRST_ROWS_SET(aLeft, aJoinMethodCost);

        sFilterCost = qmoCost::getFilterCost( aStatement->mSysStat,
                                              aJoinMethodCost->joinPredicate->predicate,
                                              (aLeft->costInfo.outputRecordCnt *
                                               aRight->costInfo.outputRecordCnt *
                                               aJoinMethodCost->selectivity) );

        aJoinMethodCost->totalCost =
                qmoCost::getOnePassSortJoinCost( aJoinMethodCost,
                                                 aLeft,
                                                 aRight,
                                                 sFilterCost,
                                                 aStatement->mSysStat,
                                                 &(aJoinMethodCost->accessCost),
                                                 &(aJoinMethodCost->diskCost) );

        QMO_FIRST_ROWS_UNSET(aLeft, aJoinMethodCost);

        aJoinMethodCost->flag &= ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
        aJoinMethodCost->flag |= QMO_JOIN_METHOD_FEASIBILITY_TRUE;
    }
    else
    {
        aJoinMethodCost->flag &= ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
        aJoinMethodCost->flag |= QMO_JOIN_METHOD_FEASIBILITY_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoJoinMethodMgr::getTwoPassSort( qcStatement       * aStatement,
                                  qmoJoinMethodCost * aJoinMethodCost,
                                  qmoPredicate      * aJoinPredicate,
                                  qmgGraph          * aLeft,
                                  qmgGraph          * aRight)
{
    idBool  sLeftIsDisk;
    idBool  sRightIsDisk;
    idBool  sLeftUseOrder;
    idBool  sRightUseOrder;
    SDouble sFilterCost = 1;

    IDU_FIT_POINT_FATAL( "qmoJoinMethodMgr::getTwoPassSort::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------
    IDE_DASSERT( aJoinMethodCost != NULL );
    IDE_DASSERT( aLeft != NULL );
    IDE_DASSERT( aRight != NULL );

    //------------------------------------------
    // 저장 매체의 결정
    //------------------------------------------

    // Left 저장 매체 판단
    IDE_TEST( qmg::isDiskTempTable( aLeft, & sLeftIsDisk )
              != IDE_SUCCESS );

    if ( sLeftIsDisk == ID_TRUE )
    {
        // Left Child의 저장 매체 결정
        aJoinMethodCost->flag &= ~QMO_JOIN_METHOD_LEFT_STORAGE_MASK;
        aJoinMethodCost->flag |= QMO_JOIN_METHOD_LEFT_STORAGE_DISK;
    }
    else
    {
        // Left Child의 저장 매체 결정
        aJoinMethodCost->flag &= ~QMO_JOIN_METHOD_LEFT_STORAGE_MASK;
        aJoinMethodCost->flag |= QMO_JOIN_METHOD_LEFT_STORAGE_MEMORY;
    }

    // Right 저장 매체 판단
    IDE_TEST( qmg::isDiskTempTable( aRight, & sRightIsDisk )
              != IDE_SUCCESS );

    if ( sRightIsDisk == ID_TRUE )
    {
        // Right Child의 저장 매체 결정
        aJoinMethodCost->flag &= ~QMO_JOIN_METHOD_RIGHT_STORAGE_MASK;
        aJoinMethodCost->flag |= QMO_JOIN_METHOD_RIGHT_STORAGE_DISK;
    }
    else
    {
        // Right Child의 저장 매체 결정
        aJoinMethodCost->flag &= ~QMO_JOIN_METHOD_RIGHT_STORAGE_MASK;
        aJoinMethodCost->flag |= QMO_JOIN_METHOD_RIGHT_STORAGE_MEMORY;
    }

    //------------------------------------------
    // Preserved Order의 사용 가능 여부 판단
    //------------------------------------------
    IDE_TEST( setJoinPredInfo4Sort( aStatement,
                                    aJoinPredicate,
                                    &(aRight->depInfo),
                                    aJoinMethodCost )
              != IDE_SUCCESS );

    if( aJoinMethodCost->joinPredicate != NULL )
    {
        // Left의 Order 사용 가능 여부 판단
        IDE_TEST( canUsePreservedOrder( aStatement,
                                        aJoinMethodCost,
                                        aLeft,
                                        QMO_JOIN_CHILD_LEFT,
                                        & sLeftUseOrder )
                  != IDE_SUCCESS );

        if ( sLeftUseOrder == ID_TRUE )
        {
            // BUG-37861 two pass sort 일때는 노드를 무조건 만들어야 한다.
            aJoinMethodCost->flag &= ~QMO_JOIN_LEFT_NODE_MASK;
            aJoinMethodCost->flag |= QMO_JOIN_LEFT_NODE_STORE;
        }
        else
        {
            aJoinMethodCost->flag &= ~QMO_JOIN_LEFT_NODE_MASK;
            aJoinMethodCost->flag |= QMO_JOIN_LEFT_NODE_SORTING;
        }

        // Right의 Order 사용 가능 여부 판단
        IDE_TEST( canUsePreservedOrder( aStatement,
                                        aJoinMethodCost,
                                        aRight,
                                        QMO_JOIN_CHILD_RIGHT,
                                        & sRightUseOrder )
                  != IDE_SUCCESS );

        if ( sRightUseOrder == ID_TRUE )
        {
            // Child Node의 종류 결정
            aJoinMethodCost->flag &= ~QMO_JOIN_RIGHT_NODE_MASK;
            aJoinMethodCost->flag |= QMO_JOIN_RIGHT_NODE_STORE;
        }
        else
        {
            // Child Node의 종류 결정
            aJoinMethodCost->flag &= ~QMO_JOIN_RIGHT_NODE_MASK;
            aJoinMethodCost->flag |= QMO_JOIN_RIGHT_NODE_SORTING;
        }

        //------------------------------------------
        // Cost 의 계산
        //------------------------------------------
        QMO_FIRST_ROWS_SET(aLeft, aJoinMethodCost);

        sFilterCost = qmoCost::getFilterCost( aStatement->mSysStat,
                                              aJoinMethodCost->joinPredicate->predicate,
                                              (aLeft->costInfo.outputRecordCnt *
                                               aRight->costInfo.outputRecordCnt *
                                               aJoinMethodCost->selectivity) );

        aJoinMethodCost->totalCost =
                qmoCost::getTwoPassSortJoinCost( aJoinMethodCost,
                                                 aLeft,
                                                 aRight,
                                                 sFilterCost,
                                                 aStatement->mSysStat,
                                                 &(aJoinMethodCost->accessCost),
                                                 &(aJoinMethodCost->diskCost) );

        QMO_FIRST_ROWS_UNSET(aLeft, aJoinMethodCost);

        aJoinMethodCost->flag &= ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
        aJoinMethodCost->flag |= QMO_JOIN_METHOD_FEASIBILITY_TRUE;
    }
    else
    {
        aJoinMethodCost->flag &= ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
        aJoinMethodCost->flag |= QMO_JOIN_METHOD_FEASIBILITY_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoJoinMethodMgr::getInverseSort( qcStatement       * aStatement,
                                         qmoJoinMethodCost * aMethod,
                                         qmoPredicate      * aJoinPredicate,
                                         qmgGraph          * aLeft,
                                         qmgGraph          * aRight)
{
    idBool  sIsDisk;
    idBool  sUseOrder;
    SDouble sFilterCost = 1;

    IDU_FIT_POINT_FATAL( "qmoJoinMethodMgr::getInverseSort::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aMethod != NULL );
    IDE_DASSERT( aLeft != NULL );
    IDE_DASSERT( aRight != NULL );

    //------------------------------------------
    // 저장 매체의 결정
    //------------------------------------------

    // 저장 매체 판단
    IDE_TEST( qmg::isDiskTempTable( aRight, & sIsDisk )
              != IDE_SUCCESS );

    if ( sIsDisk == ID_TRUE )
    {
        // Child의 저장 매체 결정
        aMethod->flag &= ~QMO_JOIN_METHOD_RIGHT_STORAGE_MASK;
        aMethod->flag |= QMO_JOIN_METHOD_RIGHT_STORAGE_DISK;
    }
    else
    {
        // Child의 저장 매체 결정
        aMethod->flag &= ~QMO_JOIN_METHOD_RIGHT_STORAGE_MASK;
        aMethod->flag |= QMO_JOIN_METHOD_RIGHT_STORAGE_MEMORY;
    }

    //------------------------------------------
    // Preserved Order의 사용 가능 여부 판단
    //------------------------------------------
    IDE_TEST( setJoinPredInfo4Sort( aStatement,
                                    aJoinPredicate,
                                    &(aRight->depInfo),
                                    aMethod )
              != IDE_SUCCESS );

    if( aMethod->joinPredicate != NULL )
    {
        IDE_TEST( canUsePreservedOrder( aStatement,
                                        aMethod,
                                        aRight,
                                        QMO_JOIN_CHILD_RIGHT,
                                        & sUseOrder )
                  != IDE_SUCCESS );

        if ( sUseOrder == ID_TRUE )
        {
            // Child Node의 종류 결정
            aMethod->flag &= ~QMO_JOIN_RIGHT_NODE_MASK;
            aMethod->flag |= QMO_JOIN_RIGHT_NODE_STORE;
        }
        else
        {
            // Child Node의 종류 결정
            aMethod->flag &= ~QMO_JOIN_RIGHT_NODE_MASK;
            aMethod->flag |= QMO_JOIN_RIGHT_NODE_SORTING;
        }

        //------------------------------------------
        // Cost 의 계산
        //------------------------------------------

        QMO_FIRST_ROWS_SET(aLeft, aMethod);

        sFilterCost = qmoCost::getFilterCost( aStatement->mSysStat,
                                              aMethod->joinPredicate->predicate,
                                              (aLeft->costInfo.outputRecordCnt *
                                               aRight->costInfo.outputRecordCnt *
                                               aMethod->selectivity) );

        aMethod->totalCost =
                qmoCost::getInverseSortJoinCost( aMethod,
                                                 aLeft,
                                                 aRight,
                                                 sFilterCost,
                                                 aStatement->mSysStat,
                                                 &(aMethod->accessCost),
                                                 &(aMethod->diskCost) );

        QMO_FIRST_ROWS_UNSET(aLeft, aMethod);

        aMethod->flag &= ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
        aMethod->flag |= QMO_JOIN_METHOD_FEASIBILITY_TRUE;
    }
    else
    {
        aMethod->flag &= ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
        aMethod->flag |= QMO_JOIN_METHOD_FEASIBILITY_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoJoinMethodMgr::getOnePassHash( qcStatement       * aStatement,
                                  qmoJoinMethodCost * aJoinMethodCost,
                                  qmoPredicate      * aJoinPredicate,
                                  qmgGraph          * aLeft,
                                  qmgGraph          * aRight)
{
    idBool  sIsDisk;
    SDouble sFilterCost = 1;

    IDU_FIT_POINT_FATAL( "qmoJoinMethodMgr::getOnePassHash::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aJoinMethodCost != NULL );
    IDE_DASSERT( aLeft != NULL );
    IDE_DASSERT( aRight != NULL );

    //------------------------------------------
    // 저장 매체의 결정
    //------------------------------------------

    // 저장 매체 판단
    IDE_TEST( qmg::isDiskTempTable( aRight, & sIsDisk )
              != IDE_SUCCESS );

    if ( sIsDisk == ID_TRUE )
    {
        // Child의 저장 매체 결정
        aJoinMethodCost->flag &= ~QMO_JOIN_METHOD_RIGHT_STORAGE_MASK;
        aJoinMethodCost->flag |= QMO_JOIN_METHOD_RIGHT_STORAGE_DISK;
    }
    else
    {
        // Child의 저장 매체 결정
        aJoinMethodCost->flag &= ~QMO_JOIN_METHOD_RIGHT_STORAGE_MASK;
        aJoinMethodCost->flag |= QMO_JOIN_METHOD_RIGHT_STORAGE_MEMORY;
    }

    //------------------------------------------
    // Cost 의 계산
    //------------------------------------------
    IDE_TEST( setJoinPredInfo4Hash( aStatement,
                                    aJoinPredicate,
                                    aJoinMethodCost )
              != IDE_SUCCESS );

    if( aJoinMethodCost->joinPredicate != NULL )
    {
        QMO_FIRST_ROWS_SET(aLeft, aJoinMethodCost);

        sFilterCost = qmoCost::getFilterCost( aStatement->mSysStat,
                                              aJoinMethodCost->joinPredicate->predicate,
                                              (aLeft->costInfo.outputRecordCnt *
                                               aRight->costInfo.outputRecordCnt *
                                               aJoinMethodCost->selectivity) );

        aJoinMethodCost->totalCost =
                qmoCost::getOnePassHashJoinCost( aLeft,
                                                 aRight,
                                                 sFilterCost,
                                                 aStatement->mSysStat,
                                                 sIsDisk,
                                                 &(aJoinMethodCost->accessCost),
                                                 &(aJoinMethodCost->diskCost) );

        QMO_FIRST_ROWS_UNSET(aLeft, aJoinMethodCost);

        aJoinMethodCost->flag &= ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
        aJoinMethodCost->flag |= QMO_JOIN_METHOD_FEASIBILITY_TRUE;
        //------------------------------------------
        // 최종 비용의 계산
        //------------------------------------------

        aJoinMethodCost->totalCost *= QCU_OPTIMIZER_HASH_JOIN_COST_ADJ;
        aJoinMethodCost->totalCost /= 100.0;

        qcgPlan::registerPlanProperty(aStatement,
                                      PLAN_PROPERTY_OPTIMIZER_HASH_JOIN_COST_ADJ);
    }
    else
    {
        aJoinMethodCost->flag &= ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
        aJoinMethodCost->flag |= QMO_JOIN_METHOD_FEASIBILITY_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoJoinMethodMgr::getTwoPassHash( qcStatement       * aStatement,
                                  qmoJoinMethodCost * aJoinMethodCost,
                                  qmoPredicate      * aJoinPredicate,
                                  qmgGraph          * aLeft,
                                  qmgGraph          * aRight)
{
    idBool  sLeftIsDisk;
    idBool  sRightIsDisk;
    SDouble sFilterCost = 1;

    IDU_FIT_POINT_FATAL( "qmoJoinMethodMgr::getTwoPassHash::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aJoinMethodCost != NULL );
    IDE_DASSERT( aLeft != NULL );
    IDE_DASSERT( aRight != NULL );

    //------------------------------------------
    // 저장 매체의 결정
    //------------------------------------------

    // Left 저장 매체 판단
    IDE_TEST( qmg::isDiskTempTable( aLeft, & sLeftIsDisk )
              != IDE_SUCCESS );

    if ( sLeftIsDisk == ID_TRUE )
    {
        // Left Child의 저장 매체 결정
        aJoinMethodCost->flag &= ~QMO_JOIN_METHOD_LEFT_STORAGE_MASK;
        aJoinMethodCost->flag |= QMO_JOIN_METHOD_LEFT_STORAGE_DISK;
    }
    else
    {
        // Left Child의 저장 매체 결정
        aJoinMethodCost->flag &= ~QMO_JOIN_METHOD_LEFT_STORAGE_MASK;
        aJoinMethodCost->flag |= QMO_JOIN_METHOD_LEFT_STORAGE_MEMORY;
    }

    // Right 저장 매체 판단
    IDE_TEST( qmg::isDiskTempTable( aRight, & sRightIsDisk )
              != IDE_SUCCESS );

    if ( sRightIsDisk == ID_TRUE )
    {
        // Right Child의 저장 매체 결정
        aJoinMethodCost->flag &= ~QMO_JOIN_METHOD_RIGHT_STORAGE_MASK;
        aJoinMethodCost->flag |= QMO_JOIN_METHOD_RIGHT_STORAGE_DISK;
    }
    else
    {
        // Right Child의 저장 매체 결정
        aJoinMethodCost->flag &= ~QMO_JOIN_METHOD_RIGHT_STORAGE_MASK;
        aJoinMethodCost->flag |= QMO_JOIN_METHOD_RIGHT_STORAGE_MEMORY;
    }

    //------------------------------------------
    // Cost 의 계산
    //------------------------------------------
    IDE_TEST( setJoinPredInfo4Hash( aStatement,
                                    aJoinPredicate,
                                    aJoinMethodCost )
              != IDE_SUCCESS );

    if( aJoinMethodCost->joinPredicate != NULL )
    {
        QMO_FIRST_ROWS_SET(aLeft, aJoinMethodCost);

        sFilterCost = qmoCost::getFilterCost( aStatement->mSysStat,
                                              aJoinMethodCost->joinPredicate->predicate,
                                              (aLeft->costInfo.outputRecordCnt *
                                               aRight->costInfo.outputRecordCnt *
                                               aJoinMethodCost->selectivity) );

        aJoinMethodCost->totalCost =
                qmoCost::getTwoPassHashJoinCost( aLeft,
                                                 aRight,
                                                 sFilterCost,
                                                 aStatement->mSysStat,
                                                 sLeftIsDisk,
                                                 sRightIsDisk,
                                                 &(aJoinMethodCost->accessCost),
                                                 &(aJoinMethodCost->diskCost) );

        QMO_FIRST_ROWS_UNSET(aLeft, aJoinMethodCost);

        aJoinMethodCost->flag &= ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
        aJoinMethodCost->flag |= QMO_JOIN_METHOD_FEASIBILITY_TRUE;

        //------------------------------------------
        // 최종 비용의 계산
        //------------------------------------------
        aJoinMethodCost->totalCost *= QCU_OPTIMIZER_HASH_JOIN_COST_ADJ;
        aJoinMethodCost->totalCost /= 100.0;

        qcgPlan::registerPlanProperty(aStatement,
                                      PLAN_PROPERTY_OPTIMIZER_HASH_JOIN_COST_ADJ);
    }
    else
    {
        aJoinMethodCost->flag &= ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
        aJoinMethodCost->flag |= QMO_JOIN_METHOD_FEASIBILITY_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoJoinMethodMgr::getInverseHash( qcStatement       * aStatement,
                                         qmoJoinMethodCost * aJoinMethodCost,
                                         qmoPredicate      * aJoinPredicate,
                                         qmgGraph          * aLeft,
                                         qmgGraph          * aRight)
{
    idBool  sIsDisk;
    SDouble sFilterCost = 1;

    IDU_FIT_POINT_FATAL( "qmoJoinMethodMgr::getInverseHash::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aJoinMethodCost != NULL );
    IDE_DASSERT( aLeft != NULL );
    IDE_DASSERT( aRight != NULL );

    //------------------------------------------
    // 저장 매체의 결정
    //------------------------------------------

    // 저장 매체 판단
    IDE_TEST( qmg::isDiskTempTable( aRight, & sIsDisk )
              != IDE_SUCCESS );

    if ( sIsDisk == ID_TRUE )
    {
        // Child의 저장 매체 결정
        aJoinMethodCost->flag &= ~QMO_JOIN_METHOD_RIGHT_STORAGE_MASK;
        aJoinMethodCost->flag |= QMO_JOIN_METHOD_RIGHT_STORAGE_DISK;
    }
    else
    {
        // Child의 저장 매체 결정
        aJoinMethodCost->flag &= ~QMO_JOIN_METHOD_RIGHT_STORAGE_MASK;
        aJoinMethodCost->flag |= QMO_JOIN_METHOD_RIGHT_STORAGE_MEMORY;
    }

    //------------------------------------------
    // Cost 의 계산
    //------------------------------------------
    IDE_TEST( setJoinPredInfo4Hash( aStatement,
                                    aJoinPredicate,
                                    aJoinMethodCost )
              != IDE_SUCCESS );

    if ( aJoinMethodCost->joinPredicate != NULL )
    {
        QMO_FIRST_ROWS_SET(aLeft, aJoinMethodCost);

        sFilterCost = qmoCost::getFilterCost( aStatement->mSysStat,
                                              aJoinMethodCost->joinPredicate->predicate,
                                              (aLeft->costInfo.outputRecordCnt *
                                               aRight->costInfo.outputRecordCnt *
                                               aJoinMethodCost->selectivity) );

        aJoinMethodCost->totalCost =
                qmoCost::getInverseHashJoinCost( aLeft,
                                                 aRight,
                                                 sFilterCost,
                                                 aStatement->mSysStat,
                                                 sIsDisk,
                                                 &(aJoinMethodCost->accessCost),
                                                 &(aJoinMethodCost->diskCost) );

        QMO_FIRST_ROWS_UNSET(aLeft, aJoinMethodCost);

        aJoinMethodCost->flag &= ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
        aJoinMethodCost->flag |= QMO_JOIN_METHOD_FEASIBILITY_TRUE;
        //------------------------------------------
        // 최종 비용의 계산
        //------------------------------------------

        aJoinMethodCost->totalCost *= QCU_OPTIMIZER_HASH_JOIN_COST_ADJ;
        aJoinMethodCost->totalCost /= 100.0;

        qcgPlan::registerPlanProperty(aStatement,
                                      PLAN_PROPERTY_OPTIMIZER_HASH_JOIN_COST_ADJ);
    }
    else
    {
        aJoinMethodCost->flag &= ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
        aJoinMethodCost->flag |= QMO_JOIN_METHOD_FEASIBILITY_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoJoinMethodMgr::getMerge( qcStatement       * aStatement,
                            qmoJoinMethodCost * aJoinMethodCost,
                            qmoPredicate      * aJoinPredicate,
                            qmgGraph          * aLeft,
                            qmgGraph          * aRight)
{
    idBool  sLeftIsDisk;
    idBool  sRightIsDisk;
    idBool  sLeftUseOrder;
    idBool  sRightUseOrder;
    SDouble sFilterCost = 1;

    qmoAccessMethod * sLeftAccessMethod;
    qmoAccessMethod * sRightAccessMethod;

    IDU_FIT_POINT_FATAL( "qmoJoinMethodMgr::getMerge::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL);
    IDE_DASSERT( aJoinMethodCost != NULL );
    IDE_DASSERT( aLeft != NULL );
    IDE_DASSERT( aRight != NULL );

    //------------------------------------------
    // 저장 매체의 결정
    //------------------------------------------

    // Left 저장 매체 판단
    IDE_TEST( qmg::isDiskTempTable( aLeft, & sLeftIsDisk )
              != IDE_SUCCESS );

    if ( sLeftIsDisk == ID_TRUE )
    {
        // Left Child의 저장 매체 결정
        aJoinMethodCost->flag &= ~QMO_JOIN_METHOD_LEFT_STORAGE_MASK;
        aJoinMethodCost->flag |= QMO_JOIN_METHOD_LEFT_STORAGE_DISK;
    }
    else
    {
        // Left Child의 저장 매체 결정
        aJoinMethodCost->flag &= ~QMO_JOIN_METHOD_LEFT_STORAGE_MASK;
        aJoinMethodCost->flag |= QMO_JOIN_METHOD_LEFT_STORAGE_MEMORY;
    }

    // Right 저장 매체 판단
    IDE_TEST( qmg::isDiskTempTable( aRight, & sRightIsDisk )
              != IDE_SUCCESS );

    if ( sRightIsDisk == ID_TRUE )
    {
        // Right Child의 저장 매체 결정
        aJoinMethodCost->flag &= ~QMO_JOIN_METHOD_RIGHT_STORAGE_MASK;
        aJoinMethodCost->flag |= QMO_JOIN_METHOD_RIGHT_STORAGE_DISK;
    }
    else
    {
        // Right Child의 저장 매체 결정
        aJoinMethodCost->flag &= ~QMO_JOIN_METHOD_RIGHT_STORAGE_MASK;
        aJoinMethodCost->flag |= QMO_JOIN_METHOD_RIGHT_STORAGE_MEMORY;
    }

    //------------------------------------------
    // Child 의 종류 결정
    //------------------------------------------
    // Left Child의 종류 판단

    IDE_TEST( setJoinPredInfo4Merge( aStatement,
                                     aJoinPredicate,
                                     aJoinMethodCost )
              != IDE_SUCCESS );

    if( aJoinMethodCost->joinPredicate != NULL )
    {
        aJoinMethodCost->leftIdxInfo  = NULL;
        aJoinMethodCost->rightIdxInfo = NULL;

        // preserved order 사용 가능 여부 판단
        IDE_TEST( usablePreservedOrder4Merge( aStatement,
                                              aJoinMethodCost->joinPredicate,
                                              aLeft,
                                              & sLeftAccessMethod,
                                              & sLeftUseOrder )
                  != IDE_SUCCESS );

        if ( sLeftUseOrder == ID_TRUE )
        {
            switch ( aLeft->type )
            {
                case QMG_SELECTION :
                case QMG_PARTITION:
                case QMG_SHARD_SELECT:     // PROJ-2638
                    if ( aLeft->myFrom->tableRef->view == NULL )
                    {
                        // 일반 테이블인 경우
                        // 인덱스를 이용한 정렬 및 커서 저장이 가능하다.
                        aJoinMethodCost->flag &= ~QMO_JOIN_LEFT_NODE_MASK;
                        aJoinMethodCost->flag |= QMO_JOIN_LEFT_NODE_NONE;

                        aJoinMethodCost->leftIdxInfo = sLeftAccessMethod->method;
                    }
                    else
                    {
                        // 뷰인 경우
                        // 정렬은 보장되나 저장하여 처리하여야 함.
                        aJoinMethodCost->flag &= ~QMO_JOIN_LEFT_NODE_MASK;
                        aJoinMethodCost->flag |= QMO_JOIN_LEFT_NODE_STORE;
                    }
                    break;

                case QMG_INNER_JOIN:
                    if ( ( ((qmgJOIN*)aLeft)->selectedJoinMethod->flag
                        & QMO_JOIN_METHOD_MASK ) ==
                        QMO_JOIN_METHOD_MERGE )
                    {
                        // 머지 죠인을 통하여 정렬이 보장되는 경우로
                        // 별도의 저장이 필요없다.
                        aJoinMethodCost->flag &= ~QMO_JOIN_LEFT_NODE_MASK;
                        aJoinMethodCost->flag |= QMO_JOIN_LEFT_NODE_NONE;
                    }
                    else
                    {
                        // 정렬은 보장되나 저장하여 처리하여야 함.
                        aJoinMethodCost->flag &= ~QMO_JOIN_LEFT_NODE_MASK;
                        aJoinMethodCost->flag |= QMO_JOIN_LEFT_NODE_STORE;
                    }

                    break;
                default:
                    aJoinMethodCost->flag &= ~QMO_JOIN_LEFT_NODE_MASK;
                    aJoinMethodCost->flag |= QMO_JOIN_LEFT_NODE_STORE;
                    break;
            }

            aJoinMethodCost->flag &= ~QMO_JOIN_METHOD_USE_LEFT_PRES_ORDER_MASK;
            aJoinMethodCost->flag |= QMO_JOIN_METHOD_USE_LEFT_PRES_ORDER_TRUE;
        }
        else
        {
            aJoinMethodCost->flag &= ~QMO_JOIN_LEFT_NODE_MASK;
            aJoinMethodCost->flag |= QMO_JOIN_LEFT_NODE_SORTING;
        }

        // Right Child의 종류 판단
        // preserved order 사용 가능 여부 판단
        IDE_TEST( usablePreservedOrder4Merge( aStatement,
                                              aJoinMethodCost->joinPredicate,
                                              aRight,
                                              & sRightAccessMethod,
                                              & sRightUseOrder )
                  != IDE_SUCCESS );

        if ( sRightUseOrder == ID_TRUE )
        {
            switch ( aRight->type )
            {
                case QMG_SELECTION:
                case QMG_PARTITION:
                case QMG_SHARD_SELECT:     // PROJ-2638
                    if ( aRight->myFrom->tableRef->view == NULL )
                    {
                        // 일반 테이블인 경우
                        // 인덱스를 이용한 정렬 및 커서 저장이 가능하다.
                        aJoinMethodCost->flag &= ~QMO_JOIN_RIGHT_NODE_MASK;
                        aJoinMethodCost->flag |= QMO_JOIN_RIGHT_NODE_NONE;

                        aJoinMethodCost->rightIdxInfo = sRightAccessMethod->method;
                    }
                    else
                    {
                        // 뷰인 경우
                        // 정렬은 보장되나 저장하여 처리하여야 함.
                        aJoinMethodCost->flag &= ~QMO_JOIN_RIGHT_NODE_MASK;
                        aJoinMethodCost->flag |= QMO_JOIN_RIGHT_NODE_STORE;
                    }

                    break;
                default:
                    // Left 와 달리 Right는 Merge Join으로 인한 정렬이
                    // 보장되더라도 Merge Join Algorithm의 특성상
                    // 저장하여 처리하여야 함.
                    aJoinMethodCost->flag &= ~QMO_JOIN_RIGHT_NODE_MASK;
                    aJoinMethodCost->flag |= QMO_JOIN_RIGHT_NODE_STORE;
                    break;
            }

            aJoinMethodCost->flag &= ~QMO_JOIN_METHOD_USE_RIGHT_PRES_ORDER_MASK;
            aJoinMethodCost->flag |= QMO_JOIN_METHOD_USE_RIGHT_PRES_ORDER_TRUE;
        }
        else
        {
            aJoinMethodCost->flag &= ~QMO_JOIN_RIGHT_NODE_MASK;
            aJoinMethodCost->flag |= QMO_JOIN_RIGHT_NODE_SORTING;
        }

        //------------------------------------------
        // Cost 의 계산
        //------------------------------------------
        QMO_FIRST_ROWS_SET(aLeft, aJoinMethodCost);

        sFilterCost = qmoCost::getFilterCost( aStatement->mSysStat,
                                              aJoinMethodCost->joinPredicate->predicate,
                                              (aLeft->costInfo.outputRecordCnt *
                                               aRight->costInfo.outputRecordCnt *
                                               aJoinMethodCost->selectivity) );

        aJoinMethodCost->totalCost = qmoCost::getMergeJoinCost(
                                                aJoinMethodCost,
                                                aLeft,
                                                aRight,
                                                sLeftAccessMethod,
                                                sRightAccessMethod,
                                                sFilterCost,
                                                aStatement->mSysStat,
                                                &(aJoinMethodCost->accessCost),
                                                &(aJoinMethodCost->diskCost) );

        QMO_FIRST_ROWS_UNSET(aLeft, aJoinMethodCost);

        aJoinMethodCost->flag &= ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
        aJoinMethodCost->flag |= QMO_JOIN_METHOD_FEASIBILITY_TRUE;
    }
    else
    {
        aJoinMethodCost->flag &= ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
        aJoinMethodCost->flag |= QMO_JOIN_METHOD_FEASIBILITY_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qmoJoinMethodMgr::initJoinMethodCost4NL( qcStatement             * aStatement,
                                                qmgGraph                * aGraph,
                                                qmoPredicate            * aJoinPredicate,
                                                qmoJoinLateralDirection   aLateralDirection,
                                                qmoJoinMethodCost      ** aMethod,
                                                UInt                    * aMethodCnt)
{
/***********************************************************************
 *
 * Description : Nested Loop Join의 Join Method Cost 초기 설정
 *
 * Implementation :
 *    (1) Join Method Cost 생성 및 초기화
 *    (2) flag 설정 : join type, join direction, left right DISK/MEMORY
 *    (3) 각 type에 feasibility 검사
 *
 ***********************************************************************/

    qmoJoinMethodCost * sJoinMethodCost;
    UInt                sFirstTypeFlag   = 0;
    UInt                i;
    UInt                sMethodCnt       = 0;
    idBool              sIsDirectedJoin  = ID_FALSE;
    idBool              sIsUsable        = ID_FALSE;
    idBool              sIsInverseUsable = ID_FALSE;
    qmoPredicate      * sJoinPredicate   = NULL;

    IDU_FIT_POINT_FATAL( "qmoJoinMethodMgr::initJoinMethodCost4NL::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL);
    IDE_DASSERT( aGraph != NULL );

    //--------------------------------------------------------
    //     Join Type  |
    //     ---------------------------------------------------
    //     Inner Join | full nested loop ( left->right )
    //       (6)      | full nested loop ( right->left )
    //                | full store nested loop ( left->right )
    //                | full store nested loop ( right->left )
    //                | index nested loop ( left->right )
    //                | index nested loop ( right->left )
    //     ---------------------------------------------
    //     Semi Join  | full nested loop ( left->right )
    //       (4)      | full store nested loop ( left->right )
    //                | index nested loop ( left->right )
    //                | inverse index nested loop ( right->left )
    //     ---------------------------------------------
    //     Anti Join  | full nested loop ( left->right )
    //     Left Outer | full store nested loop ( left->right )
    //       (3)      | index nested loop ( left->right )
    //     ---------------------------------------------
    //     Full Outer | full store nested loop ( left->right )
    //       (4)      | full store nested loop ( right->left )
    //                | anti outer ( left->right )
    //                | anti outer ( right->left )
    //---------------------------------------------------------

    //------------------------------------------
    // 각 Join Type에 맞는 Join Method Cost 생성 및 flag 정보 설정
    //------------------------------------------
    switch ( aGraph->type )
    {
        case QMG_INNER_JOIN :
            sMethodCnt = 6;
            sIsDirectedJoin = ID_FALSE;

            // 첫번째 Join Method Type
            sFirstTypeFlag = QMO_JOIN_METHOD_FLAG_CLEAR;
            sFirstTypeFlag |= QMO_JOIN_METHOD_FULL_NL;

            break;
        case QMG_SEMI_JOIN:
            sMethodCnt = 4;
            sIsDirectedJoin = ID_TRUE;

            sFirstTypeFlag = QMO_JOIN_METHOD_FLAG_CLEAR;
            sFirstTypeFlag |= QMO_JOIN_METHOD_FULL_NL;

            break;
        case QMG_ANTI_JOIN:
        case QMG_LEFT_OUTER_JOIN :
            sMethodCnt = 3;
            sIsDirectedJoin = ID_TRUE;

            sFirstTypeFlag = QMO_JOIN_METHOD_FLAG_CLEAR;
            sFirstTypeFlag |= QMO_JOIN_METHOD_FULL_NL;

            break;
        case QMG_FULL_OUTER_JOIN :
            sMethodCnt = 4;
            sIsDirectedJoin = ID_FALSE;

            sFirstTypeFlag = QMO_JOIN_METHOD_FLAG_CLEAR;
            sFirstTypeFlag |= QMO_JOIN_METHOD_FULL_STORE_NL;
            break;
        default :
            IDE_FT_ASSERT( 0 );
            break;
    }

    // Join Method Cost 배열 생성
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmoJoinMethodCost ) *
                                             sMethodCnt,
                                             (void **)&sJoinMethodCost )
              != IDE_SUCCESS );

    // join method type, direction, left와 right child의 inter result type 설정
    IDE_TEST( setFlagInfo( sJoinMethodCost,
                           sFirstTypeFlag,
                           sMethodCnt,
                           sIsDirectedJoin )
              != IDE_SUCCESS );

    // PROJ-2385 Inverse Index NL
    if( aGraph->type == QMG_SEMI_JOIN )
    {
        IDE_DASSERT( sMethodCnt == 4 );

        // sJoinMethodCost[0] : Full nested loop join       (LEFTRIGHT)
        // sJoinMethodCost[1] : Index nested loop join      (LEFTRIGHT)
        // sJoinMethodCost[2] : Full store nested loop join (LEFTRIGHT)
        // sJoinMethodCost[3] : Anti-Outer                  (LEFTRIGHT) << 여기를 아래와 같이 변경한다.
        sJoinMethodCost[3].flag &= ~QMO_JOIN_METHOD_MASK;
        sJoinMethodCost[3].flag |= QMO_JOIN_METHOD_INVERSE_INDEX;
        sJoinMethodCost[3].flag &= ~QMO_JOIN_METHOD_DIRECTION_MASK;
        sJoinMethodCost[3].flag |= QMO_JOIN_METHOD_DIRECTION_RIGHTLEFT;
    }
    else
    {
        // Nothing to do.
    }

    // full (store) nested loop join 사용 가능 여부 조사
    IDE_TEST( usableJoinMethodFullNL( aGraph,
                                      aJoinPredicate,
                                      & sIsUsable )
              != IDE_SUCCESS );

    // PROJ-2385
    // inverse index nl 사용 가능 여부 조사
    //  > Predicate가 Equal Operator를 가지고 있다면 HASH_JOINABLE
    // >> Predicate가 모두 HASH_JOINABLE 이면 Inverse Index NL 가능
    sIsInverseUsable = ID_TRUE;

    for ( sJoinPredicate = aJoinPredicate;
          sJoinPredicate != NULL;
          sJoinPredicate = sJoinPredicate->next )
    {
        if ( ( sJoinPredicate->flag & QMO_PRED_HASH_JOINABLE_MASK )
                == QMO_PRED_HASH_JOINABLE_FALSE )
        {
            sIsInverseUsable = ID_FALSE;
            break;
        }
        else
        {
            // Nothing to do.
        }
    }

    //------------------------------------------
    // 각 Join Method Type과 direction에 따른 Join Method Cost 정보 설정
    //    - selectivity 설정
    //    - feasibility 설정
    //      feasibility 초기값은 TRUE, feasibility가 없으면 FALSE로 설정함
    //------------------------------------------

    for ( i = 0; i < sMethodCnt; i++ )
    {
        sJoinMethodCost[i].selectivity     = aGraph->costInfo.selectivity;

        switch ( sJoinMethodCost[i].flag & QMO_JOIN_METHOD_MASK )
        {
            case QMO_JOIN_METHOD_FULL_NL :
            case QMO_JOIN_METHOD_FULL_STORE_NL :

                //------------------------------------------
                // fix BUG-12580
                // left 노드쪽의 레코드 카운트 예측이 잘못될 경우
                // right 노드가 join 또는 view와 같이 레코드가 많은 경우에
                // 엄청난 성능저하가 발생하게 된다.
                // 따라서, hash 또는 sort join으로 수행할 수 있도록 하기 위해
                // joinable predicate이 있는 경우,
                // full nested loop, full store nested loop join method를
                // 선택하지 않는다.
                //------------------------------------------

                /**********************************************************
                 *
                 *  PROJ-2418
                 * 
                 *  Lateral View와, Lateral View가 참조하는 Relation간
                 *  Join의 Join Method는 반드시 Full NL만 선택되어야 한다.
                 *  그것도, Lateral View가 Driven(RIGHT)에 위치해야 한다.
                 * 
                 *  - Lateral View가 Driving(LEFT)이 되면,
                 *    Driven Table이 Lateral View의 결과 집합을 결정해야 하는데..
                 *    Lateral View가 탐색을 시작할 수 조차 없다.
                 *
                 *  - Lateral View가 Driven에 있지만 MTR Tuple에 쌓이면,
                 *    Driving Table이 Lateral View의 결과 집합을 결정할 때마다
                 *    새로 MTR Tuple에 쌓게 된다. Full NL보다 더 효율이 떨어진다.
                 * 
                 *  따라서, Full Store NL 역시 여기서는 배제한다.
                 *  Lateral View는 Index NL을 사용할 수 조차 없다.
                 *  아래의 Hash-based / Sort-based / Merge 역시 모두 배제한다.
                 *
                **********************************************************/

                switch ( aLateralDirection )
                {
                    case QMO_JOIN_LATERAL_LEFT:
                        // Left에서, Right를 참조하는 LView가 존재하는 경우

                        // FULL_NL을 강제로 써야 하므로 sIsUsable을 TRUE로 설정한다.
                        sIsUsable = ID_TRUE;

                        // RIGHTLEFT Full NL만 남기고 모두 배제
                        // RIGHTLEFT라면, Left의 Lateral View가 Right 위치로 변경된다.
                        if ( ( ( sJoinMethodCost[i].flag & QMO_JOIN_METHOD_MASK ) 
                             != QMO_JOIN_METHOD_FULL_NL ) ||
                             ( ( sJoinMethodCost[i].flag & QMO_JOIN_METHOD_DIRECTION_MASK ) 
                             != QMO_JOIN_METHOD_DIRECTION_RIGHTLEFT ) )
                        {
                            sJoinMethodCost[i].flag &= ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
                            sJoinMethodCost[i].flag |= QMO_JOIN_METHOD_FEASIBILITY_FALSE;
                        }
                        else
                        {
                            // Nothing to do.
                        }
                        break;
                    case QMO_JOIN_LATERAL_RIGHT:
                        // Right에서, Left를 참조하는 LView가 존재하는 경우

                        // FULL_NL을 강제로 써야 하므로 sIsUsable을 TRUE로 설정한다.
                        sIsUsable = ID_TRUE;

                        // LEFTRIGHT Full NL만 남기고 모두 배제
                        if ( ( ( sJoinMethodCost[i].flag & QMO_JOIN_METHOD_MASK ) 
                             != QMO_JOIN_METHOD_FULL_NL ) ||
                             ( ( sJoinMethodCost[i].flag & QMO_JOIN_METHOD_DIRECTION_MASK ) 
                             != QMO_JOIN_METHOD_DIRECTION_LEFTRIGHT ) )
                        {
                            sJoinMethodCost[i].flag &= ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
                            sJoinMethodCost[i].flag |= QMO_JOIN_METHOD_FEASIBILITY_FALSE;
                        }
                        else
                        {
                            // Nothing to do.
                        }
                        break;
                    case QMO_JOIN_LATERAL_NONE:
                        break;
                    default:
                        IDE_DASSERT(0);
                        break;
                }

                if( sIsUsable == ID_TRUE )
                {
                    // joinable predicate이 없는 경우
                    // 또는 use_nl hint가 적용된 경우
                    // 외부 참조를 필요로 하는 Lateral View가 있는 경우

                    // Nothing To Do
                }
                else
                {
                    if( ( sJoinMethodCost[i].flag &
                          QMO_JOIN_METHOD_DIRECTION_MASK ) ==
                        QMO_JOIN_METHOD_DIRECTION_LEFTRIGHT )
                    {
                        if ( ( aGraph->right->type == QMG_SELECTION ) ||
                             ( aGraph->right->type == QMG_SHARD_SELECT ) || // PROJ-2638
                             ( aGraph->right->type == QMG_PARTITION ) )
                        {
                            // BUG-43424 오른쪽에 view 가 있을때 push_pred 힌트를 사용시에만 NL 조인을 허용합니다.
                            if( (aGraph->right->myFrom->tableRef->view != NULL) &&
                                (aGraph->myQuerySet->SFWGH->hints->pushPredHint == NULL) &&
                                ( ( aGraph->flag & QMG_JOIN_ONLY_NL_MASK )
                                  == QMG_JOIN_ONLY_NL_FALSE ) )
                            {
                                sJoinMethodCost[i].flag &=
                                    ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
                                sJoinMethodCost[i].flag |=
                                    QMO_JOIN_METHOD_FEASIBILITY_FALSE;
                            }
                            else
                            {
                                // Nothing To Do
                            }
                        }
                        else
                        {
                            sJoinMethodCost[i].flag &=
                                ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
                            sJoinMethodCost[i].flag |=
                                QMO_JOIN_METHOD_FEASIBILITY_FALSE;
                        }
                    }
                    else
                    {
                        if ( ( aGraph->left->type == QMG_SELECTION ) ||
                             ( aGraph->left->type == QMG_SHARD_SELECT ) || // PROJ-2638
                             ( aGraph->left->type == QMG_PARTITION ) )
                        {
                            // BUG-43424 오른쪽에 view 가 있을때 push_pred 힌트를 사용시에만 NL 조인을 허용합니다.
                            if( (aGraph->left->myFrom->tableRef->view != NULL) &&
                                (aGraph->myQuerySet->SFWGH->hints->pushPredHint == NULL) &&
                                ( ( aGraph->flag & QMG_JOIN_ONLY_NL_MASK )
                                  == QMG_JOIN_ONLY_NL_FALSE ) )
                            {
                                sJoinMethodCost[i].flag &=
                                    ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
                                sJoinMethodCost[i].flag |=
                                    QMO_JOIN_METHOD_FEASIBILITY_FALSE;
                            }
                            else
                            {
                                // Nothing To Do
                            }
                        }
                        else
                        {
                            sJoinMethodCost[i].flag &=
                                ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
                            sJoinMethodCost[i].flag |=
                                QMO_JOIN_METHOD_FEASIBILITY_FALSE;
                        }
                    }
                }

                break;
            case QMO_JOIN_METHOD_INDEX :

                //------------------------------------------
                // feasibility :
                // (1) predicate이 존재
                // (2) right가 qmgSelection 인 경우
                // (3) - Index Nested Loop Join Hint가 존재하는 경우
                //       Join Method Hint 적용 후, table access hint 무시
                //     - 존재하지 않는 경우
                //       right에 Full Scan Hint가 있으면, FALSE
                //------------------------------------------

                // PROJ-2418
                // Child Graph가 외부 참조하는 상황이라면
                // Index NL은 사용할 수 없다.
                if ( ( aJoinPredicate == NULL ) || 
                     ( aLateralDirection != QMO_JOIN_LATERAL_NONE ) )
                {
                    sJoinMethodCost[i].flag &=
                        ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
                    sJoinMethodCost[i].flag |=
                        QMO_JOIN_METHOD_FEASIBILITY_FALSE;
                }
                else
                {
                    // PROJ-1502 PARTITIONED DISK TABLE
                    // feasibility : right가 qmgSelection 또는 qmgPartition 인 경우에만 가능
                    // To Fix BUG-8804
                    if ( ( sJoinMethodCost[i].flag &
                           QMO_JOIN_METHOD_DIRECTION_MASK ) ==
                         QMO_JOIN_METHOD_DIRECTION_LEFTRIGHT )
                    {
                        if ( ( aGraph->right->type == QMG_SELECTION ) ||
                             ( aGraph->right->type == QMG_SHARD_SELECT ) || // PROJ-2638
                             ( aGraph->right->type == QMG_PARTITION ) )
                        {
                            // nothing to do
                        }
                        else
                        {
                            sJoinMethodCost[i].flag &=
                                ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
                            sJoinMethodCost[i].flag |=
                                QMO_JOIN_METHOD_FEASIBILITY_FALSE;
                        }
                    }
                    else
                    {
                        if ( ( aGraph->left->type == QMG_SELECTION ) ||
                             ( aGraph->left->type == QMG_SHARD_SELECT ) || // PROJ-2638
                             ( aGraph->left->type == QMG_PARTITION ) )
                        {
                            // nothing to do
                        }
                        else
                        {
                            sJoinMethodCost[i].flag &=
                                ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
                            sJoinMethodCost[i].flag |=
                                QMO_JOIN_METHOD_FEASIBILITY_FALSE;
                        }
                    }
                }

                break;
            case QMO_JOIN_METHOD_INVERSE_INDEX :

                //------------------------------------------
                // PROJ-2385
                // index nested loop method와 동일하게 검사하지만
                // right가 아닌 left가 selection이어야 한다.
                //------------------------------------------

                if ( ( aJoinPredicate == NULL ) ||
                     ( sIsInverseUsable == ID_FALSE ) )
                {
                    sJoinMethodCost[i].flag &=
                        ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
                    sJoinMethodCost[i].flag |=
                        QMO_JOIN_METHOD_FEASIBILITY_FALSE;
                }
                else
                {
                    if ( ( aGraph->left->type == QMG_SELECTION ) ||
                         ( aGraph->left->type == QMG_SHARD_SELECT ) || // PROJ-2638
                         ( aGraph->left->type == QMG_PARTITION ) )
                    {
                        // nothing to do
                    }
                    else
                    {
                        sJoinMethodCost[i].flag &=
                            ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
                        sJoinMethodCost[i].flag |=
                            QMO_JOIN_METHOD_FEASIBILITY_FALSE;
                    }
                }

                break;
            case QMO_JOIN_METHOD_ANTI :

                //------------------------------------------
                // feasibility :
                //    (1) left, right 모두 qmgSelection 인 경우
                //    (2) join predicate이 존재하는 경우
                //------------------------------------------
                // TODO1502:
                // anti outer join에 대해 고려해야 함.

                // PROJ-2418
                // Child Graph가 외부 참조하는 상황이라면
                // Anti Outer는 사용할 수 없다.
                if ( aLateralDirection != QMO_JOIN_LATERAL_NONE )
                {
                    sJoinMethodCost[i].flag &= ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
                    sJoinMethodCost[i].flag |= QMO_JOIN_METHOD_FEASIBILITY_FALSE;
                }
                else
                {
                    if ( ( (aGraph->left->type == QMG_SELECTION) ||
                           (aGraph->left->type == QMG_SHARD_SELECT ) || // PROJ-2638
                           (aGraph->left->type == QMG_PARTITION) )
                         &&
                         ( (aGraph->right->type == QMG_SELECTION) ||
                           (aGraph->right->type == QMG_SHARD_SELECT ) || // PROJ-2638
                           (aGraph->right->type == QMG_PARTITION) ) )
                    {
                        if ( aJoinPredicate != NULL )
                        {
                            // Nothing to do.
                        }
                        else
                        {
                            // To Fix BUG-8763
                            // Join Predicate이 없는 경우
                            sJoinMethodCost[i].flag &=
                                ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
                            sJoinMethodCost[i].flag |=
                                QMO_JOIN_METHOD_FEASIBILITY_FALSE;
                        }
                    }
                    else
                    {
                        sJoinMethodCost[i].flag &=
                            ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
                        sJoinMethodCost[i].flag |=
                            QMO_JOIN_METHOD_FEASIBILITY_FALSE;
                    }
                }
                break;
            default :
                IDE_DASSERT( 0 );
                break;
        }
    }

    // BUG-37407 semi, anti join cost
    for ( i = 0; i < sMethodCnt; i++ )
    {
        switch ( aGraph->type )
        {
            case QMG_SEMI_JOIN :
                sJoinMethodCost[i].flag &= ~QMO_JOIN_SEMI_MASK;
                sJoinMethodCost[i].flag |=  QMO_JOIN_SEMI_TRUE;
                break;

            case QMG_ANTI_JOIN :
                sJoinMethodCost[i].flag &= ~QMO_JOIN_ANTI_MASK;
                sJoinMethodCost[i].flag |=  QMO_JOIN_ANTI_TRUE;
                break;

            default :
                // nothing todo
                break;
        }
    }

    // BUG-40022
    if ( (QCU_OPTIMIZER_JOIN_DISABLE & QMO_JOIN_DISABLE_NL ) == QMO_JOIN_DISABLE_NL )
    {
        for ( i = 0; i < sMethodCnt; i++ )
        {
            sJoinMethodCost[i].flag &= ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
            sJoinMethodCost[i].flag |= QMO_JOIN_METHOD_FEASIBILITY_FALSE;
        }
    }
    else
    {
        // Nothing To Do
    }

    if ( ( aGraph->flag & QMG_JOIN_ONLY_NL_MASK )
         == QMG_JOIN_ONLY_NL_TRUE )
    {
        for ( i = 0; i < sMethodCnt; i++ )
        {
            if ( ( sJoinMethodCost[i].flag & QMO_JOIN_METHOD_MASK )
                 == QMO_JOIN_METHOD_FULL_STORE_NL )
            {
                sJoinMethodCost[i].flag &= ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
                sJoinMethodCost[i].flag |= QMO_JOIN_METHOD_FEASIBILITY_FALSE;
            }
            else
            {
                /* Nothing to do */
            }
        }
    }
    else
    {
        /* Nothing to do */
    }

    qcgPlan::registerPlanProperty( aStatement,
                                    PLAN_PROPERTY_OPTIMIZER_JOIN_DISABLE );

    // BUG-46932
    if ( ( QCU_OPTIMIZER_INVERSE_JOIN_ENABLE == 0 ) &&
         ( ( aGraph->type == QMG_SEMI_JOIN ) ) ) 
    {
        // recursive with인 경우 forceJoinOrder4RecursiveView에서 순서를 강제로 설정
        if ( (aGraph->myQuerySet->lflag & QMV_QUERYSET_FROM_RECURSIVE_WITH_MASK) 
             == QMV_QUERYSET_FROM_RECURSIVE_WITH_FALSE )
        {
            // 3 : inverse hash
            sJoinMethodCost[3].flag &= ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
            sJoinMethodCost[3].flag |= QMO_JOIN_METHOD_FEASIBILITY_FALSE;
        }
    }

    qcgPlan::registerPlanProperty( aStatement,
                                   PLAN_PROPERTY_OPTIMIZER_INVERSE_JOIN_ENABLE );

    /* BUG-47786 Unnesting 결과 오류 */
    if ( ( QC_SHARED_TMPLATE(aStatement)->flag & QC_TMP_UNNEST_INVERSE_JOIN_DISABLE_MASK )
         == QC_TMP_UNNEST_INVERSE_JOIN_DISABLE_TRUE )
    {
        if ( aGraph->type == QMG_SEMI_JOIN )
        {
            // 3 : inverse hash
            sJoinMethodCost[3].flag &= ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
            sJoinMethodCost[3].flag |= QMO_JOIN_METHOD_FEASIBILITY_FALSE;
        }
    }

    *aMethodCnt = sMethodCnt;
    *aMethod = sJoinMethodCost;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qmoJoinMethodMgr::initJoinMethodCost4Hash( qcStatement             * aStatement,
                                                  qmgGraph                * aGraph,
                                                  qmoPredicate            * aJoinPredicate,
                                                  qmoJoinLateralDirection   aLateralDirection,
                                                  qmoJoinMethodCost      ** aMethod,
                                                  UInt                    * aMethodCnt )
{
/***********************************************************************
 *
 * Description : Hash Join의 join method cost 정보 설정
 *
 * Implementation :
 *    (1) Join Method Cost 생성 및 초기화
 *    (2) flag 설정 : join type, join direction, left right DISK/MEMORY
 *    (3) 각 type에 feasibility 검사
 *
 ***********************************************************************/

    qmoJoinMethodCost * sJoinMethodCost;
    UInt                sFirstTypeFlag;
    UInt                sMethodCnt      = 0;
    idBool              sIsDirectedJoin = ID_FALSE;
    UInt                i;

    IDU_FIT_POINT_FATAL( "qmoJoinMethodMgr::initJoinMethodCost4NL::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL);
    IDE_DASSERT( aGraph != NULL );

    //--------------------------------------------------------
    //     Join Type  |
    //     ---------------------------------------------------
    //     Inner Join | one pass hash join ( left->right )
    //       (4)      | one pass hash join ( right->left )
    //                | two pass hash join ( left->right )
    //                | two pass hash join ( right->left )
    //     ---------------------------------------------
    //     Semi Join  | one pass hash join ( left->right )
    //     Anti Join  | two pass hash join ( left->right )
    //     Left Outer | inverse  hash join ( right->left )
    //       (3)      |
    //     ---------------------------------------------
    //     Full Outer | one pass hash join ( left->right )
    //       (4)      | one pass hash join ( right->left )
    //                | two pass hash join ( left->right )
    //                | two pass hash join ( right->left )
    //---------------------------------------------------------

    //------------------------------------------
    // 각 Join Type에 맞는 Join Method Cost 생성 및 flag 정보 설정
    //------------------------------------------

    sFirstTypeFlag = QMO_JOIN_METHOD_FLAG_CLEAR;
    sFirstTypeFlag |= QMO_JOIN_METHOD_ONE_PASS_HASH;

    switch ( aGraph->type )
    {
        case QMG_INNER_JOIN :
        case QMG_FULL_OUTER_JOIN :
            sMethodCnt = 4;
            sIsDirectedJoin = ID_FALSE;
            break;
        case QMG_SEMI_JOIN:
        case QMG_ANTI_JOIN:
        case QMG_LEFT_OUTER_JOIN :
            sMethodCnt = 3;
            sIsDirectedJoin = ID_TRUE;
            break;
        default :
            IDE_FT_ASSERT( 0 );
            break;
    }

    // Join Method Cost 배열 생성
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmoJoinMethodCost ) *
                                             sMethodCnt,
                                             (void **)&sJoinMethodCost )
              != IDE_SUCCESS );

    // join method type, direction, left와 right child의 inter result type 설정
    IDE_TEST( setFlagInfo( sJoinMethodCost,
                           sFirstTypeFlag,
                           sMethodCnt,
                           sIsDirectedJoin )
              != IDE_SUCCESS );

    /* PROJ-2339 : Revert the direction of INVERSE HASH */
    if( ( aGraph->type == QMG_SEMI_JOIN ) ||
        ( aGraph->type == QMG_ANTI_JOIN ) ||
        ( aGraph->type == QMG_LEFT_OUTER_JOIN ) )
    {
        IDE_DASSERT( sMethodCnt == 3 );

        // sJoinMethodCost[0] : One-Pass Hash Join (LEFTRIGHT)
        // sJoinMethodCost[1] : Two-Pass Hash Join (LEFTRIGHT)
        // sJoinMethodCost[2] : Inverse Hash Join  (LEFTRIGHT) << 여기를 아래와 같이 변경한다.
        sJoinMethodCost[2].flag &= ~QMO_JOIN_METHOD_DIRECTION_MASK;
        sJoinMethodCost[2].flag |= QMO_JOIN_METHOD_DIRECTION_RIGHTLEFT;
    }
    else
    {
        // Nothing to do.
    }

    //------------------------------------------
    // 각 Join Method Type과 direction에 따른 Join Method Cost 정보 설정
    //    - selectivity 설정
    //    - feasibility 설정
    //------------------------------------------

    for ( i = 0; i < sMethodCnt; i++ )
    {
        sJoinMethodCost[i].selectivity     = aGraph->costInfo.selectivity;

        //------------------------------------------
        // feasibility  : predicate이 존재할 경우에만 가능
        //------------------------------------------

        if ( aJoinPredicate == NULL )
        {
            sJoinMethodCost[i].flag &= ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
            sJoinMethodCost[i].flag |= QMO_JOIN_METHOD_FEASIBILITY_FALSE;
        }
        else
        {
            // PROJ-2418
            // Child Graph가 외부 참조하는 상황이라면
            // Hash-based Join Method는 사용할 수 없다.
            if ( aLateralDirection != QMO_JOIN_LATERAL_NONE )
            {
                sJoinMethodCost[i].flag &= ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
                sJoinMethodCost[i].flag |= QMO_JOIN_METHOD_FEASIBILITY_FALSE;
            }
            else
            {
                // Nothing to do
            }

            /* BUG-41599 */
            if ((aJoinPredicate->node->lflag & QTC_NODE_COLUMN_RID_MASK)
                == QTC_NODE_COLUMN_RID_EXIST)
            {
                sJoinMethodCost[i].flag &= ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
                sJoinMethodCost[i].flag |= QMO_JOIN_METHOD_FEASIBILITY_FALSE;
            }
            else
            {
                // Nothing To Do
            }
        }

        if ( (aStatement->disableLeftStore == ID_TRUE) &&
             ((sJoinMethodCost[i].flag & QMO_JOIN_METHOD_MASK) == QMO_JOIN_METHOD_TWO_PASS_HASH) )
        {
            sJoinMethodCost[i].flag &= ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
            sJoinMethodCost[i].flag |= QMO_JOIN_METHOD_FEASIBILITY_FALSE;
        }
        else
        {
            // Nothing To Do
        }
    }

    // BUG-40022
    if ( (QCU_OPTIMIZER_JOIN_DISABLE & QMO_JOIN_DISABLE_HASH) == QMO_JOIN_DISABLE_HASH )
    {
        for ( i = 0; i < sMethodCnt; i++ )
        {
            sJoinMethodCost[i].flag &= ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
            sJoinMethodCost[i].flag |= QMO_JOIN_METHOD_FEASIBILITY_FALSE;
        }
    }
    else
    {
        // Nothing To Do
    }

    qcgPlan::registerPlanProperty( aStatement,
                                    PLAN_PROPERTY_OPTIMIZER_JOIN_DISABLE );

    // BUG-46932
    if ( ( QCU_OPTIMIZER_INVERSE_JOIN_ENABLE == 0 ) &&
         ( ( aGraph->type == QMG_SEMI_JOIN ) || 
           ( aGraph->type == QMG_ANTI_JOIN ) ||
           ( aGraph->type == QMG_LEFT_OUTER_JOIN ) ) )
    {
        // recursive with인 경우 forceJoinOrder4RecursiveView에서 순서를 강제로 설정
        if ( (aGraph->myQuerySet->lflag & QMV_QUERYSET_FROM_RECURSIVE_WITH_MASK) 
             == QMV_QUERYSET_FROM_RECURSIVE_WITH_FALSE )
        {
            // 2 : inverse hash
            sJoinMethodCost[2].flag &= ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
            sJoinMethodCost[2].flag |= QMO_JOIN_METHOD_FEASIBILITY_FALSE;
        }
    }

    qcgPlan::registerPlanProperty( aStatement,
                                   PLAN_PROPERTY_OPTIMIZER_INVERSE_JOIN_ENABLE );

    /* BUG-47786 Unnesting 결과 오류 */
    if ( ( QC_SHARED_TMPLATE(aStatement)->flag & QC_TMP_UNNEST_INVERSE_JOIN_DISABLE_MASK )
         == QC_TMP_UNNEST_INVERSE_JOIN_DISABLE_TRUE )
    {
        if ( ( aGraph->type == QMG_SEMI_JOIN ) ||
             ( aGraph->type == QMG_ANTI_JOIN ) )
        {
            // 2 : inverse hash
            sJoinMethodCost[2].flag &= ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
            sJoinMethodCost[2].flag |= QMO_JOIN_METHOD_FEASIBILITY_FALSE;
        }
    }

    *aMethodCnt = sMethodCnt;
    *aMethod = sJoinMethodCost;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoJoinMethodMgr::initJoinMethodCost4Sort( qcStatement             * aStatement,
                                                  qmgGraph                * aGraph,
                                                  qmoPredicate            * aJoinPredicate,
                                                  qmoJoinLateralDirection   aLateralDirection,
                                                  qmoJoinMethodCost      ** aMethod,
                                                  UInt                    * aMethodCnt )
{
/***********************************************************************
 *
 * Description : Sort Join의 Join Method Cost 초기 설정
 *
 * Implementation :
 *    (1) Join Method Cost 생성 및 초기화
 *    (2) flag 설정 : join type, join direction, left right DISK/MEMORY
 *    (3) 각 type에 feasibility 검사
 *
 ***********************************************************************/

    qmoJoinMethodCost * sJoinMethodCost;
    UInt                sFirstTypeFlag;
    UInt                sMethodCnt      = 0;
    idBool              sIsDirectedJoin = ID_FALSE;
    UInt                i;

    IDU_FIT_POINT_FATAL( "qmoJoinMethodMgr::initJoinMethodCost4Sort::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL);
    IDE_DASSERT( aGraph != NULL );

    //--------------------------------------------------------
    //     Join Type  |
    //     ---------------------------------------------------
    //     Inner Join | one pass sort join ( left->right )
    //       (4)      | one pass sort join ( right->left )
    //                | two pass sort join ( left->right )
    //                | two pass sort join ( right->left )
    //     ---------------------------------------------
    //     Semi Join  | one pass sort join ( left->right )
    //     Anti Join  | two pass sort join ( left->right )
    //       (3)      | inverse  sort join ( right->left )
    //                |
    //     ---------------------------------------------
    //     Left Outer | one pass sort join ( left->right )
    //       (2)      | two pass sort join ( left->right )
    //     ---------------------------------------------
    //     Full Outer | one pass sort join ( left->right )
    //       (4)      | one pass sort join ( right->left )
    //                | two pass sort join ( left->right )
    //                | two pass sort join ( right->left )
    //---------------------------------------------------------

    //------------------------------------------
    // 각 Join Type에 맞는 Join Method Cost 생성 및 flag 정보 설정
    //    - 설정되는 flag 정보 : joinMethodType,
    //                           direction,
    //                           left right disk/memory
    //------------------------------------------

    //------------------------------------------
    // joinMethodType, direction 정보 설정
    //------------------------------------------

    sFirstTypeFlag = QMO_JOIN_METHOD_FLAG_CLEAR;
    sFirstTypeFlag |= QMO_JOIN_METHOD_ONE_PASS_SORT;

    switch ( aGraph->type )
    {
        case QMG_INNER_JOIN :
        case QMG_FULL_OUTER_JOIN :
            sMethodCnt = 4;
            sIsDirectedJoin = ID_FALSE;
            break;
        case QMG_SEMI_JOIN:
        case QMG_ANTI_JOIN:
            sMethodCnt = 3;
            sIsDirectedJoin = ID_TRUE;
            break;
        case QMG_LEFT_OUTER_JOIN :
            sMethodCnt = 2;
            sIsDirectedJoin = ID_TRUE;
            break;
        default :
            IDE_FT_ASSERT( 0 );
            break;
    }

    // Join Method Cost 배열 생성
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmoJoinMethodCost ) *
                                                sMethodCnt,
                                                (void **)&sJoinMethodCost )
                 != IDE_SUCCESS );

    // join method type, direction, left와 right child의 inter result type 설정
    IDE_TEST( setFlagInfo( sJoinMethodCost,
                              sFirstTypeFlag,
                              sMethodCnt,
                              sIsDirectedJoin )
                 != IDE_SUCCESS );

    /* PROJ-2385 : Revert the direction of INVERSE SORT */
    if ( ( aGraph->type == QMG_SEMI_JOIN ) ||
         ( aGraph->type == QMG_ANTI_JOIN ) )
    {
        IDE_DASSERT( sMethodCnt == 3 );

        // sJoinMethodCost[0] : One-Pass Sort Join (LEFTRIGHT)
        // sJoinMethodCost[1] : Two-Pass Sort Join (LEFTRIGHT)
        // sJoinMethodCost[2] : Inverse  Sort Join (LEFTRIGHT) << 여기를 아래와 같이 변경한다.
        sJoinMethodCost[2].flag &= ~QMO_JOIN_METHOD_DIRECTION_MASK;
        sJoinMethodCost[2].flag |= QMO_JOIN_METHOD_DIRECTION_RIGHTLEFT;
    }
    else
    {
        // Nothing to do.
    }

    //------------------------------------------
    // 각 Join Method Type과 direction에 따른 Join Method Cost 정보 설정
    //    - selectivity 설정
    //    - feasibility 설정
    //------------------------------------------

    for ( i = 0; i < sMethodCnt; i++ )
    {
        sJoinMethodCost[i].selectivity     = aGraph->costInfo.selectivity;

        //------------------------------------------
        // feasibility  : predicate이 존재할 경우에만 가능
        //------------------------------------------

        if ( aJoinPredicate == NULL )
        {
            sJoinMethodCost[i].flag &= ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
            sJoinMethodCost[i].flag |= QMO_JOIN_METHOD_FEASIBILITY_FALSE;
        }
        else
        {
            // PROJ-2418
            // Child Graph가 외부 참조하는 상황이라면
            // Sort-based Join Method는 사용할 수 없다.
            if ( aLateralDirection != QMO_JOIN_LATERAL_NONE )
            {
                sJoinMethodCost[i].flag &= ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
                sJoinMethodCost[i].flag |= QMO_JOIN_METHOD_FEASIBILITY_FALSE;
            }
            else
            {
                // Nothing To Do
            }

            /* BUG-41599 */
            if ((aJoinPredicate->node->lflag & QTC_NODE_COLUMN_RID_MASK)
                == QTC_NODE_COLUMN_RID_EXIST)
            {
                sJoinMethodCost[i].flag &= ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
                sJoinMethodCost[i].flag |= QMO_JOIN_METHOD_FEASIBILITY_FALSE;
            }
            else
            {
                // Nothing To Do
            }
        }

        if ( (aStatement->disableLeftStore == ID_TRUE) &&
             ((sJoinMethodCost[i].flag & QMO_JOIN_METHOD_MASK) == QMO_JOIN_METHOD_TWO_PASS_SORT) )
        {
            sJoinMethodCost[i].flag &= ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
            sJoinMethodCost[i].flag |= QMO_JOIN_METHOD_FEASIBILITY_FALSE;
        }
        else
        {
            // Nothing To Do
        }
    }

    // BUG-40022
    if ( (QCU_OPTIMIZER_JOIN_DISABLE & QMO_JOIN_DISABLE_SORT) == QMO_JOIN_DISABLE_SORT )
    {
        for ( i = 0; i < sMethodCnt; i++ )
        {
            sJoinMethodCost[i].flag &= ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
            sJoinMethodCost[i].flag |= QMO_JOIN_METHOD_FEASIBILITY_FALSE;
        }
    }
    else
    {
        // Nothing To Do
    }

    qcgPlan::registerPlanProperty( aStatement,
                                    PLAN_PROPERTY_OPTIMIZER_JOIN_DISABLE );

    // BUG-46932
    if ( ( QCU_OPTIMIZER_INVERSE_JOIN_ENABLE == 0 ) &&
         ( ( aGraph->type == QMG_SEMI_JOIN ) || 
           ( aGraph->type == QMG_ANTI_JOIN ) ) )
    {
        // recursive with인 경우 forceJoinOrder4RecursiveView에서 순서를 강제로 설정
        if ( (aGraph->myQuerySet->lflag & QMV_QUERYSET_FROM_RECURSIVE_WITH_MASK) 
             == QMV_QUERYSET_FROM_RECURSIVE_WITH_FALSE )
        {
            // 2 : inverse sort
            sJoinMethodCost[2].flag &= ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
            sJoinMethodCost[2].flag |= QMO_JOIN_METHOD_FEASIBILITY_FALSE;
        }
    }

    qcgPlan::registerPlanProperty( aStatement,
                                   PLAN_PROPERTY_OPTIMIZER_INVERSE_JOIN_ENABLE );

    /* BUG-47786 Unnesting 결과 오류 */
    if ( ( QC_SHARED_TMPLATE(aStatement)->flag & QC_TMP_UNNEST_INVERSE_JOIN_DISABLE_MASK )
         == QC_TMP_UNNEST_INVERSE_JOIN_DISABLE_TRUE )
    {
        if ( ( aGraph->type == QMG_SEMI_JOIN ) ||
             ( aGraph->type == QMG_ANTI_JOIN ) )
        {
            // 2 : inverse sort
            sJoinMethodCost[2].flag &= ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
            sJoinMethodCost[2].flag |= QMO_JOIN_METHOD_FEASIBILITY_FALSE;
        }
    }

    *aMethodCnt = sMethodCnt;
    *aMethod = sJoinMethodCost;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoJoinMethodMgr::initJoinMethodCost4Merge(qcStatement             * aStatement,
                                                  qmgGraph                * aGraph,
                                                  qmoPredicate            * aJoinPredicate,
                                                  qmoJoinLateralDirection   aLateralDirection,
                                                  qmoJoinMethodCost      ** aMethod,
                                                  UInt                    * aMethodCnt )
{
/***********************************************************************
 *
 * Description : Merge Join의 Join Method Cost 초기 설정
 *
 * Implementation :
 *    (1) joinMethodType, direction 정보 설정
 *    (2) feasibility 검사
 *
 ***********************************************************************/

    UInt                sFirstTypeFlag;
    qmoJoinMethodCost * sJoinMethodCost;
    UInt                sMethodCnt          = 0;
    UInt                i;
    idBool              sIsDirectedJoin     = ID_FALSE;

    qmgGraph          * sRightChildGraph;
    qmgJOIN           * sRightJoinGraph;

    IDU_FIT_POINT_FATAL( "qmoJoinMethodMgr::initJoinMethodCost4Merge::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL);
    IDE_DASSERT( aGraph != NULL );

    //--------------------------------------------------------
    //     Join Type  |
    //     ---------------------------------------------------
    //     Merge Join | merge join ( left->right )
    //       (2)      | merge join ( right->left )
    //--------------------------------------------------------

    //------------------------------------------
    // Inner Join 의 Merge Join
    //------------------------------------------

    switch ( aGraph->type )
    {
        case QMG_INNER_JOIN :
            sMethodCnt = 2;
            sIsDirectedJoin = ID_FALSE;
            break;
        case QMG_SEMI_JOIN:
        case QMG_ANTI_JOIN:
            sMethodCnt = 1;
            sIsDirectedJoin = ID_TRUE;
            break;
        default :
            IDE_FT_ASSERT( 0 );
            break;
    }

    // To Fix PR-7989
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmoJoinMethodCost ) *
                                             sMethodCnt,
                                             (void **)&sJoinMethodCost )
              != IDE_SUCCESS );

    // Join Method Type과 Join Direction 설정
    sFirstTypeFlag = QMO_JOIN_METHOD_FLAG_CLEAR;
    sFirstTypeFlag |= QMO_JOIN_METHOD_MERGE;

    IDE_TEST( setFlagInfo( sJoinMethodCost,
                           sFirstTypeFlag,
                           sMethodCnt,
                           sIsDirectedJoin ) // is left outer
              != IDE_SUCCESS );

    //------------------------------------------
    // 각 Join Method Type과 direction에 따른 Join Method Cost 정보 설정
    //    - selectivity 설정
    //    - feasibility 설정
    //------------------------------------------

    for ( i = 0; i < sMethodCnt; i++ )
    {
        sJoinMethodCost[i].selectivity     = aGraph->costInfo.selectivity;

        // feasibility 설정
        if ( aJoinPredicate == NULL )
        {
            sJoinMethodCost[i].flag &= ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
            sJoinMethodCost[i].flag |= QMO_JOIN_METHOD_FEASIBILITY_FALSE;
        }
        else
        {
            // PROJ-2418
            if ( aLateralDirection != QMO_JOIN_LATERAL_NONE )
            {
                // Child Graph가 외부 참조하는 상황이라면
                // Merge Join Method는 선택될 수 없다.
                sJoinMethodCost[i].flag &= ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
                sJoinMethodCost[i].flag |= QMO_JOIN_METHOD_FEASIBILITY_FALSE;
            }
            else
            {
                if ( ( sJoinMethodCost[i].flag & QMO_JOIN_METHOD_DIRECTION_MASK )
                     == QMO_JOIN_METHOD_DIRECTION_LEFTRIGHT )
                {
                    sRightChildGraph = aGraph->right;
                }
                else
                {
                    sRightChildGraph = aGraph->left;
                }

                // To Fix PR-11944
                // Right Child가 Merge Join인 경우
                // Store Cursor의 일관성이 보장되지 않으므로 사용할 수 없다.
                if ( sRightChildGraph->type == QMG_INNER_JOIN )
                {
                    sRightJoinGraph = (qmgJOIN *) sRightChildGraph;
                    if ( ( sRightJoinGraph->selectedJoinMethod->flag &
                           QMO_JOIN_METHOD_MASK ) == QMO_JOIN_METHOD_MERGE )
                    {
                        sJoinMethodCost[i].flag &=
                            ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
                        sJoinMethodCost[i].flag |=
                            QMO_JOIN_METHOD_FEASIBILITY_FALSE;
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
            }

            /* BUG-41599 */
            if ((aJoinPredicate->node->lflag & QTC_NODE_COLUMN_RID_MASK)
                == QTC_NODE_COLUMN_RID_EXIST)
            {
                sJoinMethodCost[i].flag &= ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
                sJoinMethodCost[i].flag |= QMO_JOIN_METHOD_FEASIBILITY_FALSE;
            }
            else
            {
                // Nothing To Do
            }
        }

        if ( aStatement->disableLeftStore == ID_TRUE )
        {
            sJoinMethodCost[i].flag &= ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
            sJoinMethodCost[i].flag |= QMO_JOIN_METHOD_FEASIBILITY_FALSE;
        }
        else
        {
            // Nothing To Do
        }
    }

    /* TASK-6744 */
    if ( (QCU_PLAN_RANDOM_SEED != 0) &&
         (aStatement->mRandomPlanInfo != NULL) &&
         (smiGetStartupPhase() == SMI_STARTUP_SERVICE) )
    {
        for ( i = 0; i < sMethodCnt; i++ )
        {
            sJoinMethodCost[i].flag &= ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
            sJoinMethodCost[i].flag |= QMO_JOIN_METHOD_FEASIBILITY_FALSE;
        }

        qcgPlan::registerPlanProperty( aStatement,
                                       PLAN_PROPERTY_OPTIMIZER_PLAN_RANDOM_SEED );
    }
    else
    {
        // BUG-40022
        if ( (QCU_OPTIMIZER_JOIN_DISABLE & QMO_JOIN_DISABLE_MERGE) == QMO_JOIN_DISABLE_MERGE )
        {
            for ( i = 0; i < sMethodCnt; i++ )
            {
                sJoinMethodCost[i].flag &= ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
                sJoinMethodCost[i].flag |= QMO_JOIN_METHOD_FEASIBILITY_FALSE;
            }
        }
        else
        {
            // Nothing To Do
        }

        qcgPlan::registerPlanProperty( aStatement,
                                       PLAN_PROPERTY_OPTIMIZER_JOIN_DISABLE );
    }

    *aMethodCnt = sMethodCnt;
    *aMethod = sJoinMethodCost;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoJoinMethodMgr::setFlagInfo( qmoJoinMethodCost * aJoinMethodCost,
                               UInt                aFirstTypeFlag,
                               UInt                aJoinMethodCnt,
                               idBool              aIsDirected )
{
/***********************************************************************
 *
 * Description : Join Method Type 과 Direction 정보를 설정
 *
 * Implementation :
 *    (1) joinMethodType 정보 설정
 *    (2) direction 정보 설정
 *
 ***********************************************************************/

    UInt                sJoinMethodTypeFlag;
    qmoJoinMethodCost * sCurJoinMethodCost;
    qmoJoinMethodCost * sNextJoinMethodCost;
    UInt                i;

    IDU_FIT_POINT_FATAL( "qmoJoinMethodMgr::setFlagInfo::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aJoinMethodCost != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sJoinMethodTypeFlag = aFirstTypeFlag;

    for ( i = 0; i < aJoinMethodCnt; i++ )
    {
        sCurJoinMethodCost = & aJoinMethodCost[i];

        // Join Method Cost 초기화
        sCurJoinMethodCost->selectivity = 0;
        sCurJoinMethodCost->joinPredicate = NULL;
        sCurJoinMethodCost->accessCost = 0;
        sCurJoinMethodCost->diskCost = 0;
        sCurJoinMethodCost->totalCost = 0;
        sCurJoinMethodCost->rightIdxInfo = NULL;
        sCurJoinMethodCost->leftIdxInfo = NULL;
        sCurJoinMethodCost->hashTmpTblCntHint = QMS_NOT_DEFINED_TEMP_TABLE_CNT;

        // flag 정보 초기화
        if ( aIsDirected == ID_FALSE )
        {
            //------------------------------------------
            // 방향성이 있는 join이 아닌 경우
            // left->right 와 right->left 방향 모두 설정
            //------------------------------------------

            if ( i % 2 == 0 )
            {
                //------------------------------------------
                // left->right
                //------------------------------------------

                // join method type 설정
                sCurJoinMethodCost->flag = QMO_JOIN_METHOD_FLAG_CLEAR;
                sCurJoinMethodCost->flag |= sJoinMethodTypeFlag;

                sNextJoinMethodCost = & aJoinMethodCost[i+1];
                sNextJoinMethodCost->flag = QMO_JOIN_METHOD_FLAG_CLEAR;
                sNextJoinMethodCost->flag |= sJoinMethodTypeFlag;

                // direction 정보 설정
                sCurJoinMethodCost->flag &= ~QMO_JOIN_METHOD_DIRECTION_MASK;
                sCurJoinMethodCost->flag |=
                    QMO_JOIN_METHOD_DIRECTION_LEFTRIGHT;
            }
            else
            {
                //------------------------------------------
                // right->left
                //------------------------------------------

                sJoinMethodTypeFlag = sJoinMethodTypeFlag << 1;

                // direction 정보 설정
                sCurJoinMethodCost->flag &= ~QMO_JOIN_METHOD_DIRECTION_MASK;
                sCurJoinMethodCost->flag |=
                    QMO_JOIN_METHOD_DIRECTION_RIGHTLEFT;
            }
        }
        else
        {
            //------------------------------------------
            // 방향성이 있는 join인 경우
            // left->right 방향만을 설정
            //------------------------------------------

            // join method type 설정
            sCurJoinMethodCost->flag = QMO_JOIN_METHOD_FLAG_CLEAR;
            sCurJoinMethodCost->flag |= sJoinMethodTypeFlag;

            sJoinMethodTypeFlag = sJoinMethodTypeFlag << 1;
        }
    }

    return IDE_SUCCESS;
}

IDE_RC
qmoJoinMethodMgr::usablePreservedOrder4Merge( qcStatement    * aStatement,
                                              qmoPredInfo    * aJoinPred,
                                              qmgGraph       * aGraph,
                                              qmoAccessMethod ** aAccessMethod,
                                              idBool         * aUsable )
{
/***********************************************************************
 *
 * Description :  Preserver Order 사용 가능 여부
 *
 * Implementation :
 *
 ***********************************************************************/

    idBool              sUsable;
    qmgPreservedOrder * sPreservedOrder;
    qtcNode           * sCompareNode;
    qtcNode           * sColumnNode;
    qtcNode           * sJoinColumnNode;
    qcmIndex          * sIndex;
    qmoAccessMethod   * sOrgMethod;

    IDU_FIT_POINT_FATAL( "qmoJoinMethodMgr::usablePreservedOrder4Merge::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aJoinPred != NULL );
    IDE_DASSERT( aGraph != NULL );
    IDE_DASSERT( aAccessMethod != NULL );
    IDE_DASSERT( aUsable != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sUsable = ID_FALSE;

    //------------------------------------------
    // preserved order 사용 가능 여부 검사
    //------------------------------------------
    sJoinColumnNode =  qmoPred::getColumnNodeOfJoinPred(
        aStatement,
        aJoinPred->predicate,
        & aGraph->depInfo );

    if ( aGraph->preservedOrder != NULL )
    {
        // Preserved Order가 있는 경우

        if( sJoinColumnNode != NULL )
        {
            if( ( sJoinColumnNode->node.table ==
                  aGraph->preservedOrder->table ) &&
                ( sJoinColumnNode->node.column ==
                  aGraph->preservedOrder->column ) )
            {
                // BUG-21195
                // direction이 ASC인 경우와 NOT DEFINED인 경우를 따로 분리할 필요가 없다.
                // 이전에는 따로 분리가 되어 있었고,
                // ASC인 경우에 aAccessMethod를 세팅하지 않아 segment fault 에러가 발생했었다.
                if ( ( aGraph->preservedOrder->direction == QMG_DIRECTION_ASC ) ||
                     ( aGraph->preservedOrder->direction == QMG_DIRECTION_NOT_DEFINED ) )
                {
                    IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmgPreservedOrder ),
                                                               (void**) & sPreservedOrder )
                              != IDE_SUCCESS );

                    sPreservedOrder->table = aGraph->preservedOrder->table;
                    sPreservedOrder->column = aGraph->preservedOrder->column;
                    sPreservedOrder->direction = QMG_DIRECTION_ASC;

                    // To Fix PR-7989
                    sPreservedOrder->next = NULL;

                    IDE_TEST( qmg::checkUsableOrder( aStatement,
                                                     sPreservedOrder,
                                                     aGraph,
                                                     & sOrgMethod,
                                                     aAccessMethod,
                                                     & sUsable )
                              != IDE_SUCCESS );
                }
                else
                {
                    // nothing to do
                }
            }
            else
            {
                // nothing to do
            }
        }
        else
        {
            // nothing to do
        }
    }
    else
    {
        // Preserved Order가 없는 경우,
        // Preserved Order 사용 가능한지 검사

        // To Fix PR-8023
        //------------------------------
        // Column Node의 추출
        //------------------------------

        sCompareNode = aJoinPred->predicate->node;

        if( ( sCompareNode->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
            == MTC_NODE_LOGICAL_CONDITION_TRUE )
        {
            // CNF의 경우
            sCompareNode = (qtcNode *) sCompareNode->node.arguments;
        }
        else
        {
            // DNF의 경우로 비교 연산자임.
            // Nothing To Do
        }

        if ( sCompareNode->indexArgument == 0 )
        {
            sColumnNode = (qtcNode*) sCompareNode->node.arguments;
        }
        else
        {
            sColumnNode = (qtcNode*) sCompareNode->node.arguments->next;
        }

        //------------------------------
        // Want Order의 생성
        //------------------------------

        IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF( qmgPreservedOrder ),
                                                 (void**) & sPreservedOrder )
                  != IDE_SUCCESS );

        sPreservedOrder->table = sColumnNode->node.table;
        sPreservedOrder->column = sColumnNode->node.column;
        sPreservedOrder->direction = QMG_DIRECTION_ASC;

        // To Fix PR-7989
        sPreservedOrder->next = NULL;

        IDE_TEST( qmg::checkUsableOrder( aStatement,
                                         sPreservedOrder,
                                         aGraph,
                                         & sOrgMethod,
                                         aAccessMethod,
                                         & sUsable )
                  != IDE_SUCCESS );
    }

    // BUG-38118 merge join 결과 오류
    // merge join 의 경우 무조건 asc 순으로 정렬되어야 한다.
    // 또한 index 를 QMNC_SCAN_TRAVERSE_FORWARD 방식으로 읽어야 한다.
    // index 를 QMNC_SCAN_TRAVERSE_BACKWARD 방식으로 읽으면 null 부터 나와 결과가 틀려진다.
    if ( sUsable == ID_TRUE )
    {
        //----------------------------------
        // 첫번째 Column이 ASC 검사.
        //----------------------------------

        if( *aAccessMethod != NULL )
        {
            sIndex  = (*aAccessMethod)->method->index;

            // index desc 힌트가 있는경우 QMNC_SCAN_TRAVERSE_BACKWARD 방식으로 읽는다.
            // QMNC_SCAN_TRAVERSE_BACKWARD 방식으로는 merge 조인을 해서는 안된다.
            if( ((*aAccessMethod)->method->flag & QMO_STAT_CARD_IDX_HINT_MASK)
                == QMO_STAT_CARD_IDX_INDEX_DESC )
            {
                sUsable = ID_FALSE;
            }
            else
            {
                // nothing to do
            }

            // index 컬럼이 desc 순일때 index를 반대로 읽어서 요구한 asc 방식으로 읽으려 하는것이다.
            // QMNC_SCAN_TRAVERSE_BACKWARD 방식으로는 merge 조인을 해서는 안된다.
            if ( (sIndex->keyColsFlag[0] & SMI_COLUMN_ORDER_MASK)
                == SMI_COLUMN_ORDER_DESCENDING )
            {
                sUsable = ID_FALSE;
            }
            else
            {
                // nothing to do
            }
        }
        else
        {
            // nothing to do
        }
    }
    else
    {
        // nothing to do
    }

    *aUsable = sUsable;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoJoinMethodMgr::setJoinMethodHint( qmoJoinMethod      * aJoinMethod,
                                     qmsJoinMethodHints * aJoinMethodHints,
                                     qmgGraph           * aGraph,
                                     UInt                 aNoUseHintMask,
                                     qmoJoinMethodCost ** aSameTableOrder,
                                     qmoJoinMethodCost ** aDiffTableOrder )
{
/***********************************************************************
 *
 * Description : Join Method Hint를 만족하는 Join Method Cost들 중에서
 *               가장 cost가 좋은 것을 찾아냄
 *
 * Implementation :
 *
 ***********************************************************************/

    qmoJoinMethodCost * sJoinMethodCost = NULL;
    qmoJoinMethodCost * sSameTableOrder = NULL;
    qmoJoinMethodCost * sDiffTableOrder = NULL;
    qcDepInfo           sDependencies;
    idBool              sIsSameTableOrder = ID_FALSE;
    UInt i;

    IDU_FIT_POINT_FATAL( "qmoJoinMethodMgr::setJoinMethodHint::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aJoinMethod != NULL );
    IDE_DASSERT( aJoinMethodHints != NULL );
    IDE_DASSERT( aGraph != NULL );

    //------------------------------------------
    // Hint와 동일한 Join Method Type을 가지는 Join Method Cost들 중에서
    // Cost가 가장 좋은 것을 선택
    //------------------------------------------

    for ( i = 0; i < aJoinMethod->joinMethodCnt; i++ )
    {
        sJoinMethodCost = &(aJoinMethod->joinMethodCost[i]);

        // BUG-42413 NO_USE_ hint 지원
        // NO_USE 힌트 skip
        if ( aJoinMethodHints->isNoUse == ID_TRUE )
        {
            // NO_USE로 막힌 조인 메소드 skip
            if ( ( (sJoinMethodCost->flag & QMO_JOIN_METHOD_MASK) &
                   (aNoUseHintMask & QMO_JOIN_METHOD_MASK) ) != 0 )
            {
                continue;
            }
            else
            {
                // nothing to do
            }
        }
        else
        {
            // USE 힌트와 다른 조인 메소드 skip
            if ( ( (sJoinMethodCost->flag & QMO_JOIN_METHOD_MASK) &
                   (aJoinMethodHints->flag & QMO_JOIN_METHOD_MASK) ) == 0 )
            {
                continue;
            }
            else
            {
                // nothing to do
            }
        }

        // feasibility가 없는 경우
        if ( (sJoinMethodCost->flag & QMO_JOIN_METHOD_FEASIBILITY_MASK) ==
              QMO_JOIN_METHOD_FEASIBILITY_FALSE )
        {
            continue;
        }
        else
        {
            // nothing to do
        }

        //------------------------------------------
        // feasility가 TRUE 경우,
        // Table Order까지 동일한 Join Method 인지 검사
        //------------------------------------------

        // To Fix PR-13145
        // Two Pass Hash Join인 경우 Temp Table의 개수 힌트를
        // 적용시켜야 한다.
        // Temp Table Count는 Two Pass Hash Join에서만 값을 갖고
        // 다른 Join Method는 0값을 가지므로,
        // Two Pass Hash Join과 다른 Join Method를 구별할 필요는 없다.
        sJoinMethodCost->hashTmpTblCntHint = aJoinMethodHints->tempTableCnt;

        if ( ( sJoinMethodCost->flag & QMO_JOIN_METHOD_DIRECTION_MASK )
               == QMO_JOIN_METHOD_DIRECTION_LEFTRIGHT )
        {
            qtc::dependencySetWithDep( & sDependencies,
                                       & aGraph->left->depInfo );
        }
        else
        {
            qtc::dependencySetWithDep( & sDependencies,
                                       & aGraph->right->depInfo );
        }

        /* PROJ-2339 Inverse Hash Join + PROJ-2385
            *
            *  Hint가 존재할 때, Join Method 선택은 다음과 같이 이루어진다.
            *  - Hint가 원하는 (Driven, Driving 순서로 이어지는) Order까지 일치하는 Method
            *  - Hint가 원하는 Method지만, Order는 일치하지 않는 Method
            *  >>> Order까지 일치하는 Method가 있다면, 그 중에서 Method를 선택한다.
            *  >>> Order까지 일치하는 Method가 없을 경우,
            *      Order는 일치하지 않지만 힌트가 원하던 Method를 선택한다.
            *
            *  이 방법은 Inner Join / Outer Join에서는 문제가 없다. 하지만,
            *  Semi/Anti Join에서는 Inverse Join Method 추가로 아래와 같은 문제가 존재한다.
            *
            *  Inverse와 Non-Inverse Join Method 모두를 고려한다고 가정하자.
            *  Hint가 원하는 Order와 일치하는 Method로 Non-Inverse Join 중에서 하나가 선택된다.
            *  Inverse Join Method의 Cost는 Order가 일치하지 않는 Method로 인식된다.
            *  이 때, Inverse Join Method의 Cost가 더 낮더라도 무시되는 문제가 발생한다.
            *
            *  따라서, Inverse와 Non-Inverse Join Method 모두를 고려하는 경우에는
            *  (INVERSE_JOIN이나 NO_INVERSE_JOIN이 사용되지 않은 경우, 즉 isUndirected == TRUE)
            *  Inverse Join Method를 'Order가 일치하는 Method'인 것처럼 편입시킨다.
            *
            */
        if ( aJoinMethodHints->isUndirected == ID_TRUE )
        {
            sIsSameTableOrder = ID_TRUE;
        }
        else
        {
            if ( qtc::dependencyContains( & sDependencies,
                                          & aJoinMethodHints->joinTables->table->depInfo )
                 == ID_TRUE )
            {
                sIsSameTableOrder = ID_TRUE;
            }
            else
            {
                sIsSameTableOrder = ID_FALSE;
            }
        }

        if ( sIsSameTableOrder == ID_TRUE )
        {
            //------------------------------------------
            // table order가 같은 경우,
            // 이미 선택된 Join Method와 비교하여 cost가 좋은 것 선택
            //------------------------------------------

            if ( sSameTableOrder == NULL )
            {
                sSameTableOrder = sJoinMethodCost;
            }
            else
            {
                /* BUG-40589 floating point calculation */
                if (QMO_COST_IS_GREATER(sSameTableOrder->totalCost,
                                        sJoinMethodCost->totalCost)
                    == ID_TRUE)
                {
                    sSameTableOrder = sJoinMethodCost;
                }
                else
                {
                    // nothing to do
                }
            }
            sDiffTableOrder = NULL;
        }
        else
        {
            //------------------------------------------
            // table order가 다른 경우
            // 이전에 선택된 Join Method와 비교하여 cost가 좋은 것 선택
            //------------------------------------------

            if ( sSameTableOrder == NULL )
            {
                // Table Order 까지 동일한 Join Method가 아직까지
                // 선택되지 않은 경우
                // 이전에 선택된 Join Method와 cost 비교 후 좋은것 선택
                if ( sDiffTableOrder == NULL )
                {
                    sDiffTableOrder = sJoinMethodCost;
                }
                else
                {
                    /* BUG-40589 floating point calculation */
                    if (QMO_COST_IS_GREATER(sDiffTableOrder->totalCost,
                                            sJoinMethodCost->totalCost)
                        == ID_TRUE)
                    {
                        sDiffTableOrder = sJoinMethodCost;
                    }
                    else
                    {
                        // nothing to do
                    }
                }
            }
            else
            {
                // nothing to do
            }
        }
    }

    *aSameTableOrder = sSameTableOrder;
    *aDiffTableOrder = sDiffTableOrder;

    return IDE_SUCCESS;
}

IDE_RC
qmoJoinMethodMgr::canUsePreservedOrder( qcStatement       * aStatement,
                                        qmoJoinMethodCost * aJoinMethodCost,
                                        qmgGraph          * aGraph,
                                        qmoJoinChild        aChildPosition,
                                        idBool            * aUsable )
{
/***********************************************************************
 *
 * Description :
 *
 *    Sort Join등을 위하여 Preserved Order를 사용 가능한지 검사
 *
 * Implementation :
 *
 ***********************************************************************/

    idBool sUsable;
    qtcNode* sJoinColumnNode;

    IDU_FIT_POINT_FATAL( "qmoJoinMethodMgr::canUsePreservedOrder::__FT__" );

    if ( aGraph->preservedOrder != NULL )
    {
        sJoinColumnNode = qmoPred::getColumnNodeOfJoinPred(
            aStatement,
            aJoinMethodCost->joinPredicate->predicate,
            & aGraph->depInfo );

        if( sJoinColumnNode != NULL )
        {
            if( ( sJoinColumnNode->node.table ==
                  aGraph->preservedOrder->table ) &&
                ( sJoinColumnNode->node.column ==
                  aGraph->preservedOrder->column ) )
            {
                switch ( aChildPosition )
                {
                    case QMO_JOIN_CHILD_LEFT:

                        aJoinMethodCost->flag &=
                            ~QMO_JOIN_METHOD_USE_LEFT_PRES_ORDER_MASK;
                        aJoinMethodCost->flag |=
                            QMO_JOIN_METHOD_USE_LEFT_PRES_ORDER_TRUE;
                        break;

                    case QMO_JOIN_CHILD_RIGHT:
                        aJoinMethodCost->flag &=
                            ~QMO_JOIN_METHOD_USE_RIGHT_PRES_ORDER_MASK;
                        aJoinMethodCost->flag |=
                            QMO_JOIN_METHOD_USE_RIGHT_PRES_ORDER_TRUE;
                        break;

                    default:
                        IDE_FT_ASSERT( 0 );
                        break;
                }
                sUsable = ID_TRUE;
            }
            else
            {
                sUsable = ID_FALSE;
            }
        }
        else
        {
            sUsable = ID_FALSE;
        }
    }
    else
    {
        sUsable = ID_FALSE;
    }

    *aUsable = sUsable;

    return IDE_SUCCESS;
}

IDE_RC
qmoJoinMethodMgr::usableJoinMethodFullNL( qmgGraph      * aGraph,
                                          qmoPredicate  * aJoinPredicate,
                                          idBool        * aIsUsable )
{
/***********************************************************************
 *
 * Description : full nested loop, full store nested loop join의
 *               사용여부를 판단
 *
 * Implementation :
 *
 *   join 노드의
 *   left 노드쪽의 레코드 카운트 예측이 잘못될 경우,
 *   right 노드가 join 또는 view와 같이 레코드가 많은 경우,
 *   성능저하가 발생하게 되어
 *   full (store) nested loop join의 feasibility를 체크하게 된다.
 *
 *   1. use_nl hint가 쓰인 경우는
 *      hint로 수행하기 위해,
 *      full (store) nested loop join의 feasibility를 true로 만든다.
 *
 *   2. joinable predicate이 존재하는 경우,
 *      적절한 join method가 선택되도록 하기 위해,
 *      full (store) nested loop join의 feasibility를 false로 만든다.
 *     ( right graph가 view 또는 join인 경우 feasibility를 flase로.. )
 *
 ***********************************************************************/

    qmsJoinMethodHints * sJoinMethodHint;
    qmoPredicate       * sJoinPredicate;
    idBool               sIsUsable = ID_FALSE;
    idBool               sIsCurrentHint;

    IDU_FIT_POINT_FATAL( "qmoJoinMethodMgr::usableJoinMethodFullNL::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aGraph != NULL );

    //------------------------------------------
    // USE_NL hint 검사
    //------------------------------------------

    // fix BUG-13232
    // use_nl hint가 있는 경우 (예: use_nl(table, view) )
    // full nested loop, full store nested loop join도 가능해야 함.
    for( sJoinMethodHint = aGraph->myQuerySet->SFWGH->hints->joinMethod;
         sJoinMethodHint != NULL;
         sJoinMethodHint = sJoinMethodHint->next )
    {
        // 잘못된 hint가 아니고
        // USE_NL hint인 경우
        if( ( sJoinMethodHint->depInfo.depCount != 0 ) &&
            ( ( sJoinMethodHint->flag & QMO_JOIN_METHOD_NL )
              != QMO_JOIN_METHOD_NONE ) )
        {
            IDE_TEST( qmo::currentJoinDependencies( aGraph,
                                                    & sJoinMethodHint->depInfo,
                                                    & sIsCurrentHint )
                      != IDE_SUCCESS );

            if( sIsCurrentHint == ID_TRUE )
            {
                sIsUsable = ID_TRUE;
                break;
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
    }

    if( sIsUsable == ID_TRUE )
    {
        // Nothing To Do
    }
    else
    {
        // fix BUG-12580
        // joinable predicate이 존재하는지 검사
        // joinable predicate인 경우, 최악의 경우
        // 사용가능한 join method가 full_nll, full_store_nl 밖에 없는 경우를
        // 대비해서, full_nl, full_store_nl 이외에
        // 사용가능한 join method가 있는지를 검사
        for( sJoinPredicate = aJoinPredicate;
             sJoinPredicate != NULL;
             sJoinPredicate = sJoinPredicate->next )
        {
            if( ( ( sJoinPredicate->flag & QMO_PRED_JOINABLE_PRED_MASK )
                  == QMO_PRED_JOINABLE_PRED_TRUE )
                &&
                ( ( ( sJoinPredicate->flag & QMO_PRED_HASH_JOINABLE_MASK )
                    == QMO_PRED_HASH_JOINABLE_TRUE )  ||
                  ( ( sJoinPredicate->flag & QMO_PRED_SORT_JOINABLE_MASK )
                    ==QMO_PRED_SORT_JOINABLE_TRUE )   ||
                  ( ( sJoinPredicate->flag & QMO_PRED_MERGE_JOINABLE_MASK )
                    == QMO_PRED_MERGE_JOINABLE_TRUE ) )
                )
            {
                break;
            }
            else
            {
                // Nothing To Do
            }
        }

        if( sJoinPredicate == NULL )
        {
            sIsUsable = ID_TRUE;
        }
        else
        {
            // Nothing To Do
        }
    }

    if ( ( aGraph->flag & QMG_JOIN_ONLY_NL_MASK )
         == QMG_JOIN_ONLY_NL_TRUE )
    {
        sIsUsable = ID_TRUE;
    }
    else
    {
        /* Nothing to do */
    }

    *aIsUsable = sIsUsable;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoJoinMethodMgr::forcePushPredHint( qmgGraph      * aGraph,
                                     qmoJoinMethod * aJoinMethod )
{
    qmsHints           * sHints;
    qmsPushPredHints   * sPushPredHint;
    qmoJoinMethodCost  * sJoinMethodCost;
    UInt                 i;
    qmgGraph           * sLeftGraph;
    qmgGraph           * sRightGraph;

    IDU_FIT_POINT_FATAL( "qmoJoinMethodMgr::forcePushPredHint::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aGraph != NULL );
    IDE_DASSERT( aJoinMethod != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sHints          = aGraph->myQuerySet->SFWGH->hints;
    sJoinMethodCost = aJoinMethod->joinMethodCost;

    // fix BUG-16677
    for( sPushPredHint = sHints->pushPredHint;
         sPushPredHint != NULL;
         sPushPredHint = sPushPredHint->next )
    {
        if( ( sPushPredHint->table->table->tableRef->flag &
              QMS_TABLE_REF_PUSH_PRED_HINT_MASK )
            == QMS_TABLE_REF_PUSH_PRED_HINT_TRUE )
        {
            // push_pred hint가 적용되는 뷰 또는 JOIN이 존재하는지 검사
            if( qtc::dependencyContains(
                    &(((qmgGraph*)(aGraph))->right->depInfo),
                    &(sPushPredHint->table->table->depInfo) )
                == ID_TRUE )
            {
                break;
            }
            else
            {
                // BUG-26800
                // inner join on절의 join조건에 push_pred 힌트 적용
                // view에 관계된 조인조건이 내려간 테이블을
                // 조인의 오른쪽에서 처리되도록 순서를 결정한다.

                // push_pred hint 적용시,
                // (1) from v1, t1 where v1.i1=t1.i1 의 경우는
                //     join ordering시 left t1, right v1의 순서로 처리되어있지만,
                // (2) from v1 inner join t1 on v1.i1=t1.i1 의 경우는
                //     join odering과정을 거치지 않기때문에
                //     righ에 v1이 오도록 조정해준다.
                if( ( aGraph->type == QMG_INNER_JOIN ) &&
                    ( ((qmgJOIN*)aGraph)->onConditionCNF != NULL ) )
                {
                    // 왼쪽에 테이블 또는 뷰가 존재하는지 검사
                    if( (aGraph->left->type == QMG_SELECTION) ||
                        (aGraph->left->type == QMG_PARTITION) )
                    {
                        if( ( aGraph->left->myFrom->tableRef->flag &
                              QMS_TABLE_REF_PUSH_PRED_HINT_MASK )
                            == QMS_TABLE_REF_PUSH_PRED_HINT_TRUE )
                        {
                            // push_pred hint가 적용되는 뷰가 존재하는지 검사
                            if( qtc::dependencyContains(
                                    &(((qmgGraph*)(aGraph))->left->depInfo),
                                    &(sPushPredHint->table->table->depInfo) )
                                == ID_TRUE )
                            {
                                // push_pred hint가 적용되는 뷰가
                                // 오른쪽에 가도록 join 그래프의 순서 변경
                                sLeftGraph   = aGraph->left;
                                sRightGraph  = aGraph->right;

                                aGraph->left  = sRightGraph;
                                aGraph->right = sLeftGraph;
                                break;
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
                    }
                    else
                    {
                        // Nothint To Do
                    }
                }
                else
                {
                    // JOIN 왼쪽에 테이블 또는 뷰가 아닌 경우
                    // ex)    JOIN
                    //    ------------
                    //    |          |
                    //   JOIN       ???

                    // Nothing To Do
                }
            }
        }
        else
        {
            // Nothing To Do
        }
    }

    // PUSH_PRED(view) hint가 적용되었고,
    // join graph가 view를 포함하는 경우라면,
    // join 수행방향은 left->right의 full nested loop join만 가능하다.
    // joinOrdering 과정중에 view는 push되는 predicate과 연관된
    // 그래프보다 나중에 수행되도록 순서가 정해짐.
    // inner join, left outer join, full outer join은
    // PUSH_PRED(view) hint가 적용되지 않음.

    // BUG-29572
    // PUSH_PRED HINT가 적용되는 뷰가
    // JOIN의 오른쪽에 온 경우에 대해서만 join method를 선택하도록 한다.
    // 예) 선택 가능한 join method의 join order
    //        JOIN
    //    ------------
    //    |          |
    //   ???        VIEW  <= PUSH_PRED HINT가 적용되는 뷰
    //
    // 예) 선택 불가능한 join method의 join order
    //        JOIN
    //    ------------
    //    |          |
    //   VIEW       ???
    //    ^
    //    |
    //  PUSH_PRED HINT가 적용되는 뷰
    //
    // 단, PUSH_PRED HINT가 적용되는 뷰를 가진 JOIN은
    //     상위 JOIN의 왼쪽에 올 수도 있다.
    //
    // 예) 선택 가능한 join method의 join order
    //        JOIN
    //    ------------
    //    |          |
    //   ???        JOIN  <= PUSH_PRED HINT가 적용되는 뷰가 포함된 JOIN
    //
    //        JOIN
    //    ------------
    //    |          |
    //   JOIN       ???
    //    ^
    //    |
    //  PUSH_PRED HINT가 적용되는 뷰가 포함된 JOIN

    // 아래의 if문은 join graph의 오른쪽에 오는 join 대상이
    // push_pred이 적용된 뷰인지를 구분하는 것임
    //
    // aGraph->right->type == QMG_SELECTION
    //   => TABLE/VIEW와 JOIN 그래프를 구분하기 위해 사용
    // sPushPredHint == NULL 인 경우는 push_pred이 적용되는 뷰가 없는 경우임
    //   => 위의 for문에서 push_pred가 적용될 view가 있는 경우 sPushPredHint가 설정됨
    if( ( (aGraph->right->type == QMG_SELECTION) ||
          (aGraph->right->type == QMG_PARTITION) )
        &&
        ( sPushPredHint != NULL ) )
    {
        for ( i = 0; i < aJoinMethod->joinMethodCnt; i++ )
        {
            sJoinMethodCost = & aJoinMethod->joinMethodCost[i];

            if( ( sJoinMethodCost->flag & QMO_JOIN_METHOD_NL )
                != QMO_JOIN_METHOD_FULL_NL )
            {
                sJoinMethodCost->flag &=
                    ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
                sJoinMethodCost->flag
                    |=  QMO_JOIN_METHOD_FEASIBILITY_FALSE;
            }
            else
            {
                // 위에서 join order가 right->left인 경우를 제거함
                if( i == 1 )
                {
                    sJoinMethodCost->flag &=
                        ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
                    sJoinMethodCost->flag
                        |=  QMO_JOIN_METHOD_FEASIBILITY_FALSE;
                }
                else
                {
                    // Nothing To Do
                }
            }
        }
    }
    else
    {
        // Nothing To Do
    }

    return IDE_SUCCESS;
}

IDE_RC
qmoJoinMethodMgr::forceJoinOrder4RecursiveView( qmgGraph      * aGraph,
                                                qmoJoinMethod * aJoinMethod )
{
/******************************************************************************
 *
 * Description : PROJ-2582 recursive with
 *          recursive view의 right subquery에서 참조되는 bottom recursive
 *          view는 오직 한번의 scan만으로 수행되어야 한다. 만일 join의
 *          오른쪽에 오는 경우, 경우에 따라 restart되어 무한 반복될 수 있다.
 *
 * Implementation :
 *
 *****************************************************************************/

    qmoJoinMethodCost  * sJoinMethodCost;
    UShort               sRecursiveViewID;
    qcDepInfo            sDepInfo;
    UInt                 i;

    IDU_FIT_POINT_FATAL( "qmoJoinMethodMgr::forceJoinOrder4RecursiveView::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aGraph != NULL );
    IDE_DASSERT( aJoinMethod != NULL );

    //------------------------------------------
    // 기본 초기화
    //------------------------------------------

    sJoinMethodCost = aJoinMethod->joinMethodCost;
    sRecursiveViewID = aGraph->myQuerySet->SFWGH->recursiveViewID;

    //------------------------------------------
    // recursive view를 join의 가장 왼쪽으로 변경한다.
    //------------------------------------------

    if ( sRecursiveViewID != ID_USHORT_MAX )
    {
        qtc::dependencySet( sRecursiveViewID, &sDepInfo );

        if ( qtc::dependencyContains( &(aGraph->depInfo),
                                      &sDepInfo )
             == ID_TRUE )
        {
            if ( qtc::dependencyContains( &(aGraph->right->depInfo),
                                          &sDepInfo )
                 == ID_TRUE )
            {
                // left->right join 금지
                for ( i = 0; i < aJoinMethod->joinMethodCnt; i++ )
                {
                    sJoinMethodCost = & aJoinMethod->joinMethodCost[i];

                    if ( ( sJoinMethodCost->flag & QMO_JOIN_METHOD_DIRECTION_MASK )
                         == QMO_JOIN_METHOD_DIRECTION_LEFTRIGHT )
                    {
                        sJoinMethodCost->flag &= ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
                        sJoinMethodCost->flag |= QMO_JOIN_METHOD_FEASIBILITY_FALSE;
                    }
                    else
                    {
                        // Nothing To Do
                    }
                }
            }
            else
            {
                // right->left join 금지
                for ( i = 0; i < aJoinMethod->joinMethodCnt; i++ )
                {
                    sJoinMethodCost = & aJoinMethod->joinMethodCost[i];

                    if ( ( sJoinMethodCost->flag & QMO_JOIN_METHOD_DIRECTION_MASK )
                         == QMO_JOIN_METHOD_DIRECTION_RIGHTLEFT )
                    {
                        sJoinMethodCost->flag &= ~QMO_JOIN_METHOD_FEASIBILITY_MASK;
                        sJoinMethodCost->flag |= QMO_JOIN_METHOD_FEASIBILITY_FALSE;
                    }
                    else
                    {
                        // Nothing To Do
                    }
                }
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

    return IDE_SUCCESS;
}

IDE_RC
qmoJoinMethodMgr::setJoinPredInfo4NL( qcStatement           * aStatement,
                                      qmoPredicate          * aJoinPredicate,
                                      qmoIdxCardInfo        * aRightIndexInfo,
                                      qmgGraph              * aRightGraph,
                                      qmoJoinMethodCost     * aMethodCost )
{
/******************************************************************************
 *
 * Description : Index Nested Loop Join 과 관련된 predicate 을 검출하여
 *               qmoPredicate.totalSelectivity 를 설정하고
 *               qmoJoinMethodCost.joinPredicate (qmoPredInfo list) 에 연결
 *
 * Implementation :
 *
 *****************************************************************************/

    qmoJoinIndexNL    sIndexNLInfo;
    qmoPredInfo     * sJoinPredInfo;

    IDU_FIT_POINT_FATAL( "qmoJoinMethodMgr::setJoinPredInfo4NL::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement      != NULL );
    IDE_DASSERT( aRightIndexInfo != NULL );
    IDE_DASSERT( aRightGraph     != NULL );
    IDE_DASSERT( aMethodCost     != NULL );

    //------------------------------------------
    // Join 관련 qmoPredInfo list 획득
    //------------------------------------------

    if ( aJoinPredicate != NULL )
    {
        // Index Nested Loop Join 계산 시 넘겨줄 정보 설정
        sIndexNLInfo.index = aRightIndexInfo->index;
        sIndexNLInfo.predicate = aJoinPredicate;
        sIndexNLInfo.rightChildPred = aRightGraph->myPredicate;
        sIndexNLInfo.rightDepInfo = & aRightGraph->depInfo;
        sIndexNLInfo.rightStatiscalData =
            aRightGraph->myFrom->tableRef->statInfo;

        if( ( aMethodCost->flag & QMO_JOIN_METHOD_DIRECTION_MASK )
            == QMO_JOIN_METHOD_DIRECTION_LEFTRIGHT )
        {
            sIndexNLInfo.direction = QMO_JOIN_DIRECTION_LEFT_RIGHT;
        }
        else
        {
            sIndexNLInfo.direction = QMO_JOIN_DIRECTION_RIGHT_LEFT;
        }

        // Join 관련 qmoPredInfo list 획득
        IDE_TEST( getIndexNLPredInfo( aStatement,
                                      & sIndexNLInfo,
                                      & sJoinPredInfo )
                  != IDE_SUCCESS );

        aMethodCost->joinPredicate = sJoinPredInfo;
        aMethodCost->rightIdxInfo = aRightIndexInfo;
    }
    else
    {
        // join predicate 이 없는 경우
        aMethodCost->joinPredicate = NULL;
        aMethodCost->rightIdxInfo = aRightIndexInfo;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmoJoinMethodMgr::getIndexNLPredInfo( qcStatement      * aStatement,
                                      qmoJoinIndexNL   * aIndexNLInfo,
                                      qmoPredInfo     ** aJoinPredInfo )
{
/******************************************************************************
 *
 * Description : Index Nested Loop Join 가능한 qmoPredInfo list 를 추출한다.
 *              [ composite index인 경우, join index 활용의 최적화 팁을 적용 ]
 *
 * Implementation :
 *
 *  join predicate은 one table predicate처럼 predicate재배치를 할 수 없으므로,
 *  인덱스컬럼과 관련된 컬럼의 연결리스트를 구성한다.
 *
 *  index 에 참여한 predicate의 연결정보를 graph로 넘기는데, 이 연결정보는
 *  join method가 결정되면, 해당 join predicate을 분리해내는 정보로 이용된다.
 *
 *  right child graph의 predicate은 qmoPredicate에 저장되어 있는 값을 이용한다.
 *
 ******************************************************************************/

    UInt           sCnt;
    UInt           sIdxColumnID;
    UInt           sJoinDirection   = 0;
    idBool         sIsUsableNextKey = ID_TRUE;
    idBool         sExistKeyRange   = ID_FALSE;
    qmoPredicate * sPredicate;
    qmoPredicate * sTempPredicate;
    qmoPredicate * sTempMorePredicate;
    qmoPredInfo  * sPredInfo;
    qmoPredInfo  * sTempInfo = NULL;
    qmoPredInfo  * sMoreInfo;
    qmoPredInfo  * sJoinPredInfo = NULL;
    qmoPredInfo  * sLastJoinPredInfo;

    IDU_FIT_POINT_FATAL( "qmoJoinMethodMgr::getIndexNLPredInfo::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement    != NULL );
    IDE_DASSERT( aIndexNLInfo->index != NULL );
    IDE_DASSERT( aIndexNLInfo->predicate != NULL );
    IDE_DASSERT( aIndexNLInfo->rightDepInfo != NULL );
    IDE_DASSERT( aIndexNLInfo->rightStatiscalData != NULL );
    IDE_DASSERT( aJoinPredInfo != NULL );

    //--------------------------------------
    // index nested loop join qmoPred info 획득
    //--------------------------------------

    if( aIndexNLInfo->index->isOnlineTBS == ID_TRUE )
    {
        // 인덱스 사용 가능

        //--------------------------------------
        // 임시 정보 설정.
        // 1. 인자로 받은 right dependencies 정보로 columnID를 설정.
        //    ( 인덱스 컬럼과 비교하기 위함)
        // 2. 동일 컬럼에 대한 대표 selectivity와
        //    다음인덱스컬럼 사용여부에 대한 정보를 구하기 위해 필요한 정보인
        //    indexArgument 정보 설정.
        //    one table predicate과 동일한 함수 사용 (qmoPred::setTotal)
        //--------------------------------------

        if( aIndexNLInfo->direction == QMO_JOIN_DIRECTION_LEFT_RIGHT )
        {
            sJoinDirection = QMO_PRED_INDEX_LEFT_RIGHT;
        }
        else
        {
            sJoinDirection = QMO_PRED_INDEX_RIGHT_LEFT;
        }

        IDE_TEST( setTempInfo( aStatement,
                               aIndexNLInfo->predicate,
                               aIndexNLInfo->rightDepInfo,
                               sJoinDirection )
                  != IDE_SUCCESS );

        for( sCnt = 0; sCnt < aIndexNLInfo->index->keyColCount; sCnt++ )
        {
            sIdxColumnID = aIndexNLInfo->index->keyColumns[sCnt].column.id;

            // 인덱스 컬럼과 같은 컬럼을 가진 predicate을 찾는다.
            sTempInfo = NULL;
            sTempPredicate = NULL;

            for( sPredicate = aIndexNLInfo->predicate;
                 sPredicate != NULL;
                 sPredicate = sPredicate->next )
            {
                if( ( ( sPredicate->flag & QMO_PRED_INDEX_JOINABLE_MASK )
                      == QMO_PRED_INDEX_JOINABLE_TRUE )
                    &&
                    ( ( sPredicate->flag & QMO_PRED_INDEX_DIRECTION_MASK
                        & sJoinDirection ) == sJoinDirection ) )
                {
                    if( sIdxColumnID == sPredicate->id )
                    {

                        IDE_TEST( QC_QMP_MEM( aStatement )->alloc( ID_SIZEOF( qmoPredInfo ),
                                                                   (void **)& sPredInfo )
                                  != IDE_SUCCESS );

                        sPredInfo->predicate = sPredicate;
                        sPredInfo->next = NULL;
                        sPredInfo->more = NULL;

                        //---------------------------------------
                        // 연결관계구성
                        // 1. sTempInfo :
                        //    동일 컬럼에 대한 predicate 연결관계유지
                        // 2. sTempPredicate :
                        //    one table predicate과 같은 qmoPred::setTotal()
                        //    을 사용하기 위한 임시 predicate연결관계구성
                        //---------------------------------------
                        if( sTempInfo == NULL )
                        {
                            sTempInfo = sPredInfo;
                            sMoreInfo = sTempInfo;

                            sTempPredicate = sPredInfo->predicate;
                            sTempMorePredicate = sTempPredicate;
                        }
                        else
                        {
                            sMoreInfo->more = sPredInfo;
                            sMoreInfo = sMoreInfo->more;

                            sTempMorePredicate->more = sPredInfo->predicate;
                            sTempMorePredicate = sTempMorePredicate->more;
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
            } // 하나의 인덱스 컬럼과 같은 컬럼의 predicate을 찾는다.

            // 인덱스 컬럼과 같은 컬럼의 predicate을 연결한다.

            if( sTempInfo != NULL )
            {
                // 하나의 컬럼리스트에 대한 대표 selectivity
                // (qmoPredicate.totalSelectivity) 를 획득하고
                // 다음 인덱스 사용 가능 정보를 얻는다.
                IDE_TEST( qmoSelectivity::setTotalSelectivity( aStatement,
                                                               aIndexNLInfo->rightStatiscalData,
                                                               sTempPredicate )
                          != IDE_SUCCESS );

                qmoPred::setCompositeKeyUsableFlag( sTempPredicate );

                // 이전 컬럼리스트와 연결관계 구성
                if( sJoinPredInfo == NULL )
                {
                    sJoinPredInfo = sTempInfo;
                    sLastJoinPredInfo = sJoinPredInfo;
                }
                else
                {
                    sLastJoinPredInfo->next = sTempInfo;
                    sLastJoinPredInfo = sLastJoinPredInfo->next;
                }

                // 위에서 임시로 연결한 predicate의 more 연결관계 원상복귀
                for( sPredicate = sTempPredicate;
                     sPredicate != NULL;
                     sPredicate = sTempMorePredicate )
                {
                    sTempMorePredicate = sPredicate->more;
                    sPredicate->more = NULL;
                }

                // 다음 인덱스 컬럼 사용여부
                if( ( sPredInfo->predicate->flag & QMO_PRED_NEXT_KEY_USABLE_MASK )
                      == QMO_PRED_NEXT_KEY_USABLE )
                {
                    // Nothing To Do
                }
                else
                {
                    sIsUsableNextKey = ID_FALSE;
                    break;
                }
            }
            else
            {
                // Nothing To Do
            }
        }

        //-------------------------------
        // join index 활용의 최적화 적용
        // right graph의 predicate에 해당 인덱스 컬럼과 같은
        // predicate이 존재하는지 검사.
        //-------------------------------

        if( ( sJoinPredInfo == NULL ) || ( sIsUsableNextKey == ID_FALSE ) )
        {
            // 인자로 넘어온 인덱스와 관련된
            // joinable predicate이 존재하지 않는 경우이거나,
            // joinable predicate이 다음 인덱스 사용가능하지 않은 경우.

            // Nothing To Do
        }
        else
        {
            // 첫번째 컬럼이 존재하는지 검사한다.
            sIdxColumnID = aIndexNLInfo->index->keyColumns[0].column.id;

            // right 그래프에 존재하는지 검사한다.
            for( sPredicate = aIndexNLInfo->rightChildPred;
                 sPredicate != NULL;
                 sPredicate = sPredicate->next )
            {
                if( sIdxColumnID == sPredicate->id )
                {
                    sExistKeyRange = ID_TRUE;
                    break;
                }
                else
                {
                    // 인덱스 컬럼과 같은 컬럼을 가진 predicate이 없는 경우
                    // Nothing To Do
                }
            }

            // Join Predicate에 존재하는지 검사한다.
            for( sPredInfo = sJoinPredInfo;
                 sPredInfo != NULL;
                 sPredInfo = sPredInfo->next )
            {
                if( sIdxColumnID == sPredInfo->predicate->id )
                {
                    sExistKeyRange = ID_TRUE;
                    break;
                }
                else
                {
                    // Nothing To Do
                }
            }

            if( sExistKeyRange == ID_FALSE )
            {
                sJoinPredInfo = NULL;
            }
            else
            {
                // Nothing To Do
            }
        }
    }
    else // ( index->isOnlineTBS == ID_FALSE )
    {
        sJoinPredInfo = NULL;
    }

    *aJoinPredInfo = sJoinPredInfo;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoJoinMethodMgr::setJoinPredInfo4AntiOuter( qcStatement       * aStatement,
                                             qmoPredicate      * aJoinPredicate,
                                             qmoIdxCardInfo    * aRightIndexInfo,
                                             qmgGraph          * aRightGraph,
                                             qmgGraph          * aLeftGraph,
                                             qmoJoinMethodCost * aMethodCost )
{
/******************************************************************************
 *
 * Description : Full outer join 에 대한 join predicate 을 대상으로
 *               qmoPredicate.totalSelectivity 를 설정하고
 *               qmoJoinMethodCost.joinPredicate (qmoPredInfo list) 에 연결
 *
 * Implementation :
 *
 ******************************************************************************/

    qcDepInfo       * sRightDepInfo;
    qmoJoinAntiOuter  sAntiOuterInfo;
    qmoIdxCardInfo  * sLeftIndexInfo;
    UInt              sLeftIndexCnt;
    qmgSELT         * sLeftMyGraph;
    qmoPredInfo     * sJoinPredInfo;
    qmoIdxCardInfo  * sSelectedLeftIdxInfo;

    IDU_FIT_POINT_FATAL( "qmoJoinMethodMgr::setJoinPredInfo4AntiOuter::__FT__" );

    //------------------------------------------
    // 적합성 검사
    //------------------------------------------

    IDE_DASSERT( aStatement      != NULL );
    IDE_DASSERT( aRightIndexInfo != NULL );
    IDE_DASSERT( aRightGraph     != NULL );
    IDE_DASSERT( aLeftGraph      != NULL );
    IDE_DASSERT( aMethodCost     != NULL );

    //------------------------------------------
    // qmoPredInfo list 획득
    //------------------------------------------

    sJoinPredInfo = NULL;
    sSelectedLeftIdxInfo = NULL;

    if( aJoinPredicate != NULL )
    {
        sRightDepInfo = & aRightGraph->depInfo;
        sLeftIndexInfo = aLeftGraph->myFrom->tableRef->statInfo->idxCardInfo;
        sLeftIndexCnt = aLeftGraph->myFrom->tableRef->statInfo->indexCnt;

        // sJoinPredInfo 획득 넘겨줄 정보 설정
        sAntiOuterInfo.index = aRightIndexInfo->index;
        sAntiOuterInfo.predicate = aJoinPredicate;
        // To Fix BUG-8384
        sAntiOuterInfo.rightDepInfo = sRightDepInfo;
        sAntiOuterInfo.leftIndexCnt = sLeftIndexCnt;
        sAntiOuterInfo.leftIndexInfo = sLeftIndexInfo;

        if( aLeftGraph->type == QMG_SELECTION )
        {
            sLeftMyGraph = (qmgSELT*) aLeftGraph;

            // sJoinPredInfo 획득
            IDE_TEST( getAntiOuterPredInfo( aStatement,
                                            sLeftMyGraph->accessMethodCnt,
                                            sLeftMyGraph->accessMethod,
                                            & sAntiOuterInfo,
                                            & sJoinPredInfo,
                                            & sSelectedLeftIdxInfo )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do
        }

        aMethodCost->joinPredicate = sJoinPredInfo;
        aMethodCost->leftIdxInfo = sSelectedLeftIdxInfo;
        aMethodCost->rightIdxInfo = aRightIndexInfo;
    }
    else
    {
        aMethodCost->joinPredicate = NULL;
        aMethodCost->leftIdxInfo = NULL;
        aMethodCost->rightIdxInfo = aRightIndexInfo;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoJoinMethodMgr::getAntiOuterPredInfo( qcStatement      * aStatement,
                                        UInt               aMethodCount,
                                        qmoAccessMethod  * aAccessMethod,
                                        qmoJoinAntiOuter * aAntiOuterInfo,
                                        qmoPredInfo     ** aJoinPredInfo,
                                        qmoIdxCardInfo  ** aLeftIdxInfo )
{
/******************************************************************************
 *
 * Description : Anti Outer Join 관련 qmoPredInfo list 와
 *               cost 가 가장 좋은 qmoAccessMethod 의 index 를 반환한다.
 *
 *               인자로 넘어온 qmoAccessMethod 관련된 predicate을 찾아서
 *               qmoPredInfo list 를 구한다.
 *
 *   Anti Outer Join predicate은
 *   (1) join predicate 양쪽 모두 인덱스를 가지고 있어야 하며,
 *   (2) index column 마다 하나의 predicate만 선택될 수 있다.
 *
 * Implementation :
 *
 *****************************************************************************/

    UInt              sCnt;
    UInt              sDirection = 0;
    UInt              sIdxColumnID;
    qmoPredicate    * sPredicate = NULL;
    qmoPredicate    * sTempPredicate = NULL;
    qmoPredicate    * sTempMorePredicate = NULL;
    qmoPredicate    * sAntiOuterPred = NULL;
    qmoPredInfo     * sPredInfo = NULL;
    qmoPredInfo     * sJoinPredInfo = NULL;
    qmoPredInfo     * sLastJoinPredInfo = NULL;
    qmoIdxCardInfo  * sSelectedLeftIdxInfo = NULL;

    IDU_FIT_POINT_FATAL( "qmoJoinMethodMgr::getAntiOuterPredInfo::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement     != NULL );
    IDE_DASSERT( aAccessMethod  != NULL );
    IDE_DASSERT( aAntiOuterInfo != NULL );
    IDE_DASSERT( aAntiOuterInfo->index != NULL );
    IDE_DASSERT( aAntiOuterInfo->predicate != NULL );
    IDE_DASSERT( aAntiOuterInfo->rightDepInfo != NULL );
    IDE_DASSERT( aJoinPredInfo  != NULL );
    IDE_DASSERT( aLeftIdxInfo   != NULL );

    //--------------------------------------
    // Anti outer join qmoPredInfo list 획득
    //--------------------------------------

    if( aAntiOuterInfo->index->isOnlineTBS == ID_TRUE )
    {
        // 인덱스 사용 가능

        //--------------------------------------
        // 임시 정보 설정.
        // 1. 인자로 받은 right dependencies 정보로 columnID를 설정.
        //    ( 인덱스 컬럼과 비교하기 위함)
        // 2. left child graph의 column을 찾기 위해.
        //--------------------------------------

        sDirection = QMO_PRED_INDEX_DIRECTION_MASK;

        IDE_TEST( setTempInfo( aStatement,
                               aAntiOuterInfo->predicate,
                               aAntiOuterInfo->rightDepInfo,
                               sDirection )
                  != IDE_SUCCESS );

        //--------------------------------------
        // 인덱스 컬럼순으로 인덱스 컬럼과 동일한 컬럼의 predicate을 찾는다.
        //--------------------------------------

        for( sCnt = 0; sCnt < aAntiOuterInfo->index->keyColCount; sCnt++ )
        {
            sIdxColumnID = aAntiOuterInfo->index->keyColumns[sCnt].column.id;

            // 인덱스 컬럼과 같은 컬럼을 가진 predicate을 찾는다.
            sTempPredicate = NULL;

            for( sPredicate = aAntiOuterInfo->predicate;
                 sPredicate != NULL;
                 sPredicate = sPredicate->next )
            {
                if( ( ( sPredicate->flag & QMO_PRED_INDEX_JOINABLE_MASK )
                      == QMO_PRED_INDEX_JOINABLE_TRUE )
                    &&
                    ( ( sPredicate->flag & QMO_PRED_INDEX_DIRECTION_MASK )
                      == sDirection ) )
                {
                    if( sIdxColumnID == sPredicate->id )
                    {
                        //---------------------------------------
                        // 연결관계구성
                        // sTempPredicate :
                        // 현재 right 인덱스 컬럼의 predicate과 관련된
                        // left 인덱스 컬럼을 찾기 위한
                        // 임시 predicate의 연결관계
                        //---------------------------------------
                        if( sTempPredicate == NULL )
                        {
                            sTempPredicate = sPredicate;
                            sTempMorePredicate = sTempPredicate;
                        }
                        else
                        {
                            sTempMorePredicate->more = sPredicate;
                            sTempMorePredicate = sTempMorePredicate->more;
                        }
                    }
                    else
                    {
                        // Nothing To Do
                    }
                } // 현재 인덱스 컬럼과 관련된 predicate이 존재할때의 처리
                else
                {
                    // Nothing To Do
                }
            } // 하나의 인덱스 컬럼과 같은 컬럼의 predicate을 찾는다.

            if( sTempPredicate == NULL )
            {
                // 현재 인덱스와 관련된 predicate이 존재하지 않는 경우
                break;
            }
            else
            {
                IDE_TEST( getAntiOuterPred( aStatement,
                                            sCnt,          // keyColCount
                                            aMethodCount,  // aAntiOuterInfo->leftIndexCnt,
                                            aAccessMethod, // aAntiOuterInfo->leftIndexInfo,
                                            sTempPredicate,
                                            & sAntiOuterPred,
                                            & sSelectedLeftIdxInfo )
                          != IDE_SUCCESS );
            }

            // 인덱스 컬럼과 같은 컬럼의 predicat을 연결한다.
            if( sAntiOuterPred != NULL )
            {
                IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmoPredInfo),
                                                         (void **)&sPredInfo )
                          != IDE_SUCCESS );

                // qmoPredicate.totalSelectivity 획득
                sAntiOuterPred->totalSelectivity = sAntiOuterPred->mySelectivity;
                sPredInfo->predicate = sAntiOuterPred;
                sPredInfo->next = NULL;
                sPredInfo->more = NULL;

                // 이전 컬럼리스트와 연결관계 구성
                if( sJoinPredInfo == NULL )
                {
                    sJoinPredInfo = sPredInfo;
                    sLastJoinPredInfo = sJoinPredInfo;
                }
                else
                {
                    sLastJoinPredInfo->next = sPredInfo;
                    sLastJoinPredInfo = sLastJoinPredInfo->next;
                }

                // 위에서 임시로 연결한 predicate의 more 연결관계 원상복귀
                for( sPredicate = sTempPredicate;
                     sPredicate != NULL;
                     sPredicate = sTempMorePredicate )
                {
                    sTempMorePredicate = sPredicate->more;
                    sPredicate->more = NULL;
                }
            }
            else
            {
                break;
            }

            // 다음 인덱스 컬럼 사용여부
            if( ( sPredInfo->predicate->flag & QMO_PRED_NEXT_KEY_USABLE_MASK )
                == QMO_PRED_NEXT_KEY_USABLE )
            {
                // Nothing To Do
            }
            else
            {
                break;
            }
        } // end of for()

    } // ( index->isOnlineTBS == ID_FALSE )
    else
    {
        sJoinPredInfo = NULL;
    }

    *aJoinPredInfo = sJoinPredInfo;
    *aLeftIdxInfo = sSelectedLeftIdxInfo;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoJoinMethodMgr::getAntiOuterPred( qcStatement      * aStatement,
                                    UInt               aKeyColCnt,
                                    UInt               aMethodCount,
                                    qmoAccessMethod  * aAccessMethod,
                                    qmoPredicate     * aPredicate,
                                    qmoPredicate    ** aAntiOuterPred,
                                    qmoIdxCardInfo  ** aSelectedLeftIdxInfo )
{
/******************************************************************************
 *
 * Description : anti outer join 관련 qmoPredInfo list 획득시 호출된다.
 *               right graph 의 현재 인덱스 컬럼에 속한 predicate 에 대해
 *               left graph (scan) 의 qmoAccessMethod array 를 순회하며 cost 비교
 *               가장 좋은 qmoAccessMethod 의 index 와 그에 속하는 predicate 반환
 *
 *     aMethodCount : left graph (scan) 의 method count
 *     aKeyColCnt  : 현재 right index 컬럼
 *
 * Implementation :
 *
 *****************************************************************************/

    idBool             sIsAntiOuterPred;
    UInt               sValueColumnID;
    UInt               sCnt;
    UInt               sIdxColumnID;
    qtcNode          * sCompareNode;
    qtcNode          * sValueNode;
    qmoPredicate     * sPredicate;
    qmoPredicate     * sAntiOuterPred;
    qmoAccessMethod  * sAccessMethod;
    qmoIdxCardInfo   * sMethodIndexInfo;
    qmoIdxCardInfo   * sSelLeftIdxInfo;
    SDouble            sTotalCost;

    IDU_FIT_POINT_FATAL( "qmoJoinMethodMgr::getAntiOuterPred::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement           != NULL );
    IDE_DASSERT( aAccessMethod        != NULL );
    IDE_DASSERT( aPredicate           != NULL );
    IDE_DASSERT( aAntiOuterPred       != NULL );
    IDE_DASSERT( aSelectedLeftIdxInfo != NULL );

    //--------------------------------------
    // 현재 컬럼리스트에서 anti outer join predicate 선택
    //--------------------------------------

    sAntiOuterPred = NULL;
    sSelLeftIdxInfo = *aSelectedLeftIdxInfo;
    sAccessMethod = aAccessMethod;

    for( sPredicate = aPredicate;
         sPredicate != NULL;
         sPredicate = sPredicate->more )
    {
        sIsAntiOuterPred = ID_TRUE;

        if( ( sPredicate->node->node.lflag & MTC_NODE_LOGICAL_CONDITION_MASK )
            == MTC_NODE_LOGICAL_CONDITION_TRUE )
        {
            sCompareNode = (qtcNode *)(sPredicate->node->node.arguments);

            if( sCompareNode->node.next == NULL )
            {
                // Nothing To Do
            }
            else
            {
                // OR 노드 하위에 비교연산자가 여러개 있는 경우는
                // anti outer joinable predicate이 아님.
                sIsAntiOuterPred = ID_FALSE;
            }
        }
        else
        {
            sCompareNode = sPredicate->node;
        }

        if( sIsAntiOuterPred == ID_TRUE )
        {
            if( sCompareNode->indexArgument == 0 )
            {
                sValueNode = (qtcNode *)(sCompareNode->node.arguments->next);
            }
            else
            {
                sValueNode = (qtcNode *)(sCompareNode->node.arguments);
            }

            //-------------------------------------------
            // anti outer join predicate을 찾는다.
            // 현재 right index column과 같은 columnID를 가진
            // predicate의 연결리스트의 value node와
            // 같은 columnID를 가진 left index를 찾는다.
            //--------------------------------------------
            sValueColumnID =
                QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sValueNode->node.table].
                columns[sValueNode->node.column].column.id;

            // BUG-36454 index hint 를 사용하면 sAccessMethod[0] 은 fullscan 아니다.
            for( sCnt = 0; sCnt < aMethodCount; sCnt++ )
            {
                sMethodIndexInfo = sAccessMethod[sCnt].method;

                if( sMethodIndexInfo != NULL )
                {
                    if( ( sMethodIndexInfo->index->isOnlineTBS == ID_TRUE ) &&
                        ( ( sMethodIndexInfo->index->keyColCount - 1 )
                          >= aKeyColCnt ) )
                    {
                        // 인덱스 사용 가능
                        sIdxColumnID =
                            sMethodIndexInfo->index->keyColumns[aKeyColCnt].column.id;

                        if( sIdxColumnID == sValueColumnID )
                        {
                            if( sAntiOuterPred == NULL )
                            {
                                sTotalCost = sAccessMethod[sCnt].totalCost;
                                sSelLeftIdxInfo = sMethodIndexInfo;
                                sAntiOuterPred = sPredicate;
                            }
                            else
                            {
                                // cost 가 작은 access method 의 index info 선택
                                /* BUG-40589 floating point calculation */
                                if (QMO_COST_IS_GREATER(sTotalCost,
                                                        sAccessMethod[sCnt].totalCost)
                                    == ID_TRUE)
                                {
                                    sTotalCost = sAccessMethod[sCnt].totalCost;
                                    sSelLeftIdxInfo = sMethodIndexInfo;
                                    sAntiOuterPred = sPredicate;
                                }
                                else
                                {
                                    // Nothing To Do
                                }
                            }
                        }
                        else
                        {
                            // Nothing To Do
                        }
                    }
                    else // ( index->isOnlineTBS == ID_FALSE )
                    {
                        // Nothing To Do
                    }
                }
                else // ( sMethodIndexInfo == NULL )
                {
                    // Nothing To Do
                }
            }
        } // AntiOuterJoin predicate인 경우
        else
        {
            // OR 노드 하위에 비교연산자가 두개이상인 경우로,
            // Anti outer joinable predicate이 아님.

            // Nothing To Do
        }
    }

    *aAntiOuterPred = sAntiOuterPred;
    *aSelectedLeftIdxInfo = sSelLeftIdxInfo;

    return IDE_SUCCESS;
}


IDE_RC
qmoJoinMethodMgr::setJoinPredInfo4Hash( qcStatement       * aStatement,
                                        qmoPredicate      * aJoinPredicate,
                                        qmoJoinMethodCost * aMethodCost )
{
/******************************************************************************
 *
 * Description : Hash Join 과 관련된 predicate 을 검출하여
 *               qmoPredicate.totalSelectivity 를 설정하고
 *               qmoJoinMethodCost.joinPredicate (qmoPredInfo list) 에 연결
 *
 * Implementation : 인자로 넘어온 join predicate list중에서
 *                  hash joinable predicate 을 모두 찾고,
 *                  찾은 predicate의 연결정보를 구성한다.
 *
 *****************************************************************************/

    qmoPredicate * sPredicate;
    qmoPredInfo  * sPredInfo;
    qmoPredInfo  * sJoinPredInfo = NULL;
    qmoPredInfo  * sLastJoinPredInfo;

    IDU_FIT_POINT_FATAL( "qmoJoinMethodMgr::setJoinPredInfo4Hash::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement     != NULL );
    IDE_DASSERT( aJoinPredicate != NULL );
    IDE_DASSERT( aMethodCost    != NULL );

    //--------------------------------------
    // Hash join predicate info 획득
    //--------------------------------------

    sJoinPredInfo = NULL;
    sPredicate = aJoinPredicate;

    while( sPredicate != NULL )
    {
        if( ( sPredicate->flag & QMO_PRED_HASH_JOINABLE_MASK )
            == QMO_PRED_HASH_JOINABLE_TRUE )
        {
            IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmoPredInfo),
                                                     (void **)&sPredInfo )
                      != IDE_SUCCESS );

            sPredInfo->predicate = sPredicate;
            sPredInfo->next = NULL;
            sPredInfo->more = NULL;

            // qmoPredicate.totalSelectivity 획득
            sPredInfo->predicate->totalSelectivity
                    = sPredInfo->predicate->mySelectivity;

            if( sJoinPredInfo == NULL )
            {
                sJoinPredInfo = sPredInfo;
                sLastJoinPredInfo = sJoinPredInfo;
            }
            else
            {
                sLastJoinPredInfo->next = sPredInfo;
                sLastJoinPredInfo = sLastJoinPredInfo->next;
            }
        }
        else
        {
            // Nothing To Do
        }

        sPredicate = sPredicate->next;
    }

    aMethodCost->joinPredicate = sJoinPredInfo;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoJoinMethodMgr::setJoinPredInfo4Sort( qcStatement       * aStatement,
                                        qmoPredicate      * aJoinPredicate,
                                        qcDepInfo         * aRightDepInfo,
                                        qmoJoinMethodCost * aMethodCost )
{
/******************************************************************************
 *
 * Description : Sort Join 과 관련된 predicate 을 검출하여
 *               qmoPredicate.totalSelectivity 를 설정하고
 *               qmoJoinMethodCost.joinPredicate (qmoPredInfo list) 에 연결
 *               (sort join은 한 컬럼에 대해서만 적용할 수 있음)
 *
 * Implementation :
 *
 *****************************************************************************/

    qcDepInfo       sAndDepInfo;
    qtcNode       * sCompareNode;
    qtcNode       * sSortColumnNode;
    qmoPredicate  * sPredicate;
    qmoPredInfo   * sPredInfo;
    qmoPredInfo   * sTempInfo = NULL;

    IDU_FIT_POINT_FATAL( "qmoJoinMethodMgr::setJoinPredInfo4Sort::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement     != NULL );
    IDE_DASSERT( aJoinPredicate != NULL );
    IDE_DASSERT( aRightDepInfo  != NULL );
    IDE_DASSERT( aMethodCost    != NULL );

    //--------------------------------------
    // Sort join qmoPredInfo list 획득
    //--------------------------------------

    // 처리된 predicate에 대한 중복 선택 방지를 위한 flag 초기화
    for( sPredicate = aJoinPredicate;
         sPredicate != NULL;
         sPredicate = sPredicate->next )
    {
        sPredicate->flag &= ~QMO_PRED_SEL_PROCESS_MASK;
    }

    //--------------------------------------
    // sort joinable predicate들을 컬럼별로 분리배치하고,
    // 컬럼별로 total selectivity를 구한 후,
    // total selectivity가 가장 좋은 컬럼 하나만 선택한다.
    //--------------------------------------
    for( sPredicate = aJoinPredicate;
         sPredicate != NULL;
         sPredicate = sPredicate->next )
    {
        if( ( ( sPredicate->flag & QMO_PRED_SORT_JOINABLE_MASK )
              == QMO_PRED_SORT_JOINABLE_TRUE )
            &&
            ( ( sPredicate->flag & QMO_PRED_SEL_PROCESS_MASK )
              == QMO_PRED_SEL_PROCESS_FALSE ) )
        {
            IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmoPredInfo),
                                                     (void **)&sPredInfo )
                      != IDE_SUCCESS );

            sPredicate->flag &= ~QMO_PRED_SEL_PROCESS_MASK;
            sPredicate->flag |= QMO_PRED_SEL_PROCESS_TRUE;

            sPredInfo->predicate = sPredicate;
            sPredInfo->next = NULL;
            sPredInfo->more = NULL;

            // 이 predicate과 동일한 컬럼의 predicate이 존재하는지
            // 검사하기 위해, right dependencies에 해당하는 노드를 찾는다.
            if( ( sPredicate->node->node.lflag
                  & MTC_NODE_LOGICAL_CONDITION_MASK )
                == MTC_NODE_LOGICAL_CONDITION_TRUE )
            {
                sCompareNode = (qtcNode *)(sPredicate->node->node.arguments);
            }
            else
            {
                sCompareNode = sPredicate->node;
            }

            // sort column을 찾는다.
            qtc::dependencyAnd(
                aRightDepInfo,
                & ( (qtcNode *)(sCompareNode->node.arguments) )->depInfo,
                & sAndDepInfo );

            if( qtc::dependencyEqual( & sAndDepInfo,
                                      & qtc::zeroDependencies ) != ID_TRUE )
            {
                sSortColumnNode = (qtcNode *)(sCompareNode->node.arguments);
            }
            else
            {
                sSortColumnNode
                    = (qtcNode *)(sCompareNode->node.arguments->next);
            }

            //--------------------------------------------
            // 현재 sort column과 동일한 컬럼이 존재하는지 찾고
            // qmoPredicate.totalSelectivity 를 획득한다.
            //--------------------------------------------
            if( sPredicate->next != NULL )
            {
                IDE_TEST( findEqualSortColumn( aStatement,
                                               sPredicate->next,
                                               aRightDepInfo,
                                               sSortColumnNode,
                                               sPredInfo )
                          != IDE_SUCCESS );
            }
            else
            {
                sPredInfo->predicate->totalSelectivity
                    = sPredInfo->predicate->mySelectivity;
            }

            if( sTempInfo == NULL )
            {
                sTempInfo = sPredInfo;
            }
            else
            {
                /* BUG-40589 floating point calculation */
                if (QMO_COST_IS_GREATER(sTempInfo->predicate->totalSelectivity,
                                        sPredInfo->predicate->totalSelectivity)
                    == ID_TRUE)
                {
                    sTempInfo = sPredInfo;
                }
                else
                {
                    // Nothing To Do
                }
            }

        } // sort joinable predicate에 대한 처리
        else
        {
            // sort joinable predicate이 아니거나,
            // sort joinable predicate인 경우, 이미 처리된 predicate인 경우

            // Nothing To Do
        }
    }

    aMethodCost->joinPredicate = sTempInfo;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoJoinMethodMgr::setJoinPredInfo4Merge( qcStatement         * aStatement,
                                         qmoPredicate        * aJoinPredicate,
                                         qmoJoinMethodCost   * aMethodCost )
{
/******************************************************************************
 *
 * Description : Merge Join 과 관련된 predicate 중
 *               selectivity 가 가장 좋은 qmoPredicate 을 하나만 검출하여
 *               (merge join은 단 하나의 predicate만 선택가능)
 *               qmoPredicate.totalSelectivity 를 설정하고
 *               qmoJoinMethodCost.joinPredicate (qmoPredInfo) 에 연결
 *
 * Implementation :
 *
 *****************************************************************************/

    UInt            sDirection = 0;
    SDouble         sSelectivity;
    qmoPredInfo   * sPredInfo;
    qmoPredicate  * sPredicate;
    qmoPredicate  * sMergeJoinPred = NULL;

    IDU_FIT_POINT_FATAL( "qmoJoinMethodMgr::setJoinPredInfo4Merge::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement     != NULL );
    IDE_DASSERT( aJoinPredicate != NULL );
    IDE_DASSERT( aMethodCost    != NULL );

    //--------------------------------------
    // Merge join qmoPredInfo list 획득
    //--------------------------------------

    sSelectivity = 1;
    sPredicate = aJoinPredicate;

    if( ( aMethodCost->flag & QMO_JOIN_METHOD_DIRECTION_MASK )
        == QMO_JOIN_METHOD_DIRECTION_LEFTRIGHT )
    {
        sDirection = QMO_PRED_MERGE_LEFT_RIGHT;
    }
    else
    {
        sDirection = QMO_PRED_MERGE_RIGHT_LEFT;
    }

    // merge joinable predicate이고,
    // 인자로 넘어온 join 수행가능방향인 predicate 중
    // selectivity가 좋은 predicate을 찾는다.
    while( sPredicate != NULL )
    {
        if( ( ( sPredicate->flag & QMO_PRED_MERGE_JOINABLE_MASK )
              == QMO_PRED_MERGE_JOINABLE_TRUE )
            &&
            ( ( sPredicate->flag & QMO_PRED_MERGE_DIRECTION_MASK
                & sDirection ) == sDirection ) )
        {
            // To Fix BUG-10978
            if ( sMergeJoinPred == NULL )
            {
                sMergeJoinPred = sPredicate;
            }
            else
            {
                // Nothing to do.
            }

            /* BUG-40589 floating point calculation */
            if (QMO_COST_IS_GREATER(sSelectivity, sPredicate->mySelectivity)
                == ID_TRUE)
            {
                sSelectivity = sPredicate->mySelectivity;
                sMergeJoinPred = sPredicate;
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
        sPredicate = sPredicate->next;
    }

    // merge join 에 관계된 qmoPredInfo 정보 설정.
    if( sMergeJoinPred == NULL )
    {
        aMethodCost->joinPredicate = NULL;
    }
    else
    {
        IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmoPredInfo),
                                                 (void **)&sPredInfo )
                  != IDE_SUCCESS );

        sPredInfo->predicate = sMergeJoinPred;
        sPredInfo->next = NULL;
        sPredInfo->more = NULL;

        // qmoPredicate.totalSelectivity 획득
        sPredInfo->predicate->totalSelectivity =
            sPredInfo->predicate->mySelectivity;

        aMethodCost->joinPredicate = sPredInfo;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmoJoinMethodMgr::setTempInfo( qcStatement  * aStatement,
                               qmoPredicate * aPredicate,
                               qcDepInfo    * aRightDepInfo,
                               UInt           aDirection )
{
/******************************************************************************
 *
 * Description : index nested loop, anti outer join qmoPredInfo list 획득시
 *               필요한 임시정보 설정 ( columnID, indexArgument )
 *
 * Implementation :
 *
 *****************************************************************************/

    qtcNode      * sCompareNode;
    qtcNode      * sColumnNode;
    qmoPredicate * sPredicate;

    IDU_FIT_POINT_FATAL( "qmoJoinMethodMgr::setTempInfo::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement    != NULL );
    IDE_DASSERT( aPredicate    != NULL );
    IDE_DASSERT( aRightDepInfo != NULL );

    //--------------------------------------
    // 임시정보 설정( columnID, indexArguments )
    //--------------------------------------

    //--------------------------------------
    // 임시 정보 설정.
    // 1. 인자로 받은 right dependencies 정보로 columnID를 설정.
    //    ( 인덱스 컬럼과 비교하기 위함)
    // 2. 동일 컬럼에 대한 대표 selectivity와
    //    다음인덱스컬럼 사용여부에 대한 정보를 구하기 위해 필요한 정보인
    //    indexArgument 정보 설정.
    //    (1) index nested loop join
    //        one table predicate과 동일한 함수 사용 (qmoPred::setTotal)
    //    (2) anti outer join
    //        left child graph를 찾기 위해.
    //--------------------------------------

    for( sPredicate = aPredicate;
         sPredicate != NULL;
         sPredicate = sPredicate->next )
    {
        if ( ( ( sPredicate->flag & QMO_PRED_INDEX_DIRECTION_MASK & aDirection )
               == aDirection ) &&
             ( ( sPredicate->node->lflag & QTC_NODE_COLUMN_RID_MASK )
               != QTC_NODE_COLUMN_RID_EXIST ) )
        {
            if( ( sPredicate->node->node.lflag
                  & MTC_NODE_LOGICAL_CONDITION_MASK )
                == MTC_NODE_LOGICAL_CONDITION_TRUE )
            {
                sCompareNode = (qtcNode *)(sPredicate->node->node.arguments);
            }
            else
            {
                sCompareNode = sPredicate->node;
            }

            if( qtc::dependencyEqual(
                    & ((qtcNode *)(sCompareNode->node.arguments))->depInfo,
                    aRightDepInfo ) == ID_TRUE )
            {
                sCompareNode->indexArgument = 0;
                sColumnNode = (qtcNode *)(sCompareNode->node.arguments);
            }
            else
            {
                sCompareNode->indexArgument = 1;
                sColumnNode = (qtcNode *)(sCompareNode->node.arguments->next);
            }

            sPredicate->id
                = QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sColumnNode->node.table].
                columns[sColumnNode->node.column].column.id;
        }
        else
        {
            sPredicate->id = QMO_COLUMNID_NON_INDEXABLE;
        }
    }

    return IDE_SUCCESS;
}


IDE_RC
qmoJoinMethodMgr::findEqualSortColumn( qcStatement  * aStatement,
                                       qmoPredicate * aPredicate,
                                       qcDepInfo    * aRightDepInfo,
                                       qtcNode      * aSortColumn,
                                       qmoPredInfo  * aPredInfo )
{
/***********************************************************************
 *
 * Description : sort join qmoPredInfo list 획득시,
 *               현재 sort column과 동일한 컬럼이 존재하는지 검사.
 *
 * Implementation :
 *
 *   인자로 넘어온 sort column과 동일한 컬럼을 모두 찾아서,
 *   aPredInfo->more의 연결리스트로 구성한다.
 *   컬럼의 total selectivity를 구해서,
 *   첫번째 qmoPredicate->totalSelectivity에 저장한다.
 *
 ***********************************************************************/

    idBool          sIsEqual;
    qcDepInfo       sAndDependencies;
    SDouble         sSelectivity;
    qtcNode       * sCompareNode;
    qtcNode       * sSortColumnNode;
    qmoPredicate  * sPredicate;
    qmoPredInfo   * sPredInfo;
    qmoPredInfo   * sMorePredInfo;

    IDU_FIT_POINT_FATAL( "qmoJoinMethodMgr::findEqualSortColumn::__FT__" );

    //--------------------------------------
    // 적합성 검사
    //--------------------------------------

    IDE_DASSERT( aStatement    != NULL );
    IDE_DASSERT( aPredicate    != NULL );
    IDE_DASSERT( aRightDepInfo != NULL );
    IDE_DASSERT( aSortColumn   != NULL );
    IDE_DASSERT( aPredInfo     != NULL );

    //--------------------------------------
    // 현재 sort column과 동일한 컬럼을 찾는다.
    //--------------------------------------

    sMorePredInfo = aPredInfo;
    sSelectivity  = sMorePredInfo->predicate->mySelectivity;

    for( sPredicate = aPredicate;
         sPredicate != NULL;
         sPredicate = sPredicate->next )
    {
        // PR-13286
        // 동일한 Column이라 하더라도 Conversion이 존재한다면,
        // 하나의 Column만을 저장하는 Sort Join에서는 하나의 값만을
        // 저장하여 처리할 수 없다.
        // Ex) T1.int > T2.double + AND T1.int < T2.int
        if ( aSortColumn->node.conversion != NULL )
        {
            break;
        }
        else
        {
            // Go Go
        }

        if( ( ( sPredicate->flag & QMO_PRED_SORT_JOINABLE_MASK )
              == QMO_PRED_SORT_JOINABLE_TRUE )
            &&
            ( ( sPredicate->flag & QMO_PRED_SEL_PROCESS_MASK )
              == QMO_PRED_SEL_PROCESS_FALSE ) )
        {
            // right dependencies에 해당하는 노드를 찾는다.
            if( ( sPredicate->node->node.lflag
                  & MTC_NODE_LOGICAL_CONDITION_MASK )
                == MTC_NODE_LOGICAL_CONDITION_TRUE )
            {
                sCompareNode = (qtcNode *)(sPredicate->node->node.arguments);
            }
            else
            {
                sCompareNode = sPredicate->node;
            }

            // sort column을 찾는다.
            qtc::dependencyAnd( aRightDepInfo,
                                & ((qtcNode *)(sCompareNode->node.arguments))->\
                                depInfo,
                                & sAndDependencies );

            if( qtc::dependencyEqual( & sAndDependencies,
                                      & qtc::zeroDependencies ) != ID_TRUE )
            {
                sSortColumnNode = (qtcNode *)(sCompareNode->node.arguments);
            }
            else
            {
                sSortColumnNode
                    = (qtcNode *)(sCompareNode->node.arguments->next);
            }

            // PR-13286
            // 동일한 Column이라 하더라도 Conversion이 존재한다면,
            // 하나의 Column만을 저장하는 Sort Join에서는 하나의 값만을
            // 저장하여 처리할 수 없다.
            // Ex) T1.int > T2.int + AND T1.int < T2.double
            if ( sSortColumnNode->node.conversion != NULL )
            {
                sIsEqual = ID_FALSE;
            }
            else
            {
                IDE_TEST( qtc::isEquivalentExpression( aStatement,
                                                       aSortColumn,
                                                       sSortColumnNode,
                                                       &sIsEqual )
                          != IDE_SUCCESS );
            }

            if( sIsEqual == ID_TRUE )
            {
                sSelectivity *= sPredicate->mySelectivity;

                IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(qmoPredInfo),
                                                         (void **)&sPredInfo )
                          != IDE_SUCCESS );

                sPredicate->flag &= ~QMO_PRED_SEL_PROCESS_MASK;
                sPredicate->flag |= QMO_PRED_SEL_PROCESS_TRUE;

                sPredInfo->predicate = sPredicate;
                sPredInfo->next = NULL;
                sPredInfo->more = NULL;

                sMorePredInfo->more = sPredInfo;
                sMorePredInfo = sMorePredInfo->more;
            }
            else
            {
                // Nothint To Do
            }
        }
        else
        {
            // Nothing To Do
        }
    }

    aPredInfo->predicate->totalSelectivity = sSelectivity;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmoJoinMethodMgr::getJoinLateralDirection( qmgGraph                * aJoinGraph,
                                                  qmoJoinLateralDirection * aLateralDirection )
{
/***********************************************************************
 *
 * Description : PROJ-2418 Cross/Outer APPLY & Lateral View
 *
 *   현재 Join Graph의 Child Graph 끼리 서로 참조하는지,
 *   그렇다면 참조하는 Graph는 LEFT인지 RIGHT인지를 나타내는
 *   Lateral Position을 반환한다.
 *
 * Implementation :
 *
 *  - Child Graph의 LateralDepInfo를 가져온다.
 *  - 한 쪽의 depInfo와 다른 쪽의 lateralDepInfo를 AND 한다.
 *  - AND한 결과가 존재하면, lateralDepInfo를 가진 쪽이 참조를 하고 있는 것.
 *
 ***********************************************************************/

    qcDepInfo   sDepInfo;
    qcDepInfo   sLeftLateralDepInfo;
    qcDepInfo   sRightLateralDepInfo;

    IDU_FIT_POINT_FATAL( "qmoJoinMethodMgr::getJoinLateralDirection::__FT__" );

    // Child Graph의 Lateral DepInfo 획득
    IDE_TEST( qmg::getGraphLateralDepInfo( aJoinGraph->left,
                                           & sLeftLateralDepInfo )
              != IDE_SUCCESS );

    IDE_TEST( qmg::getGraphLateralDepInfo( aJoinGraph->right,
                                           & sRightLateralDepInfo )
              != IDE_SUCCESS );


    qtc::dependencyAnd( & sLeftLateralDepInfo,
                        & aJoinGraph->right->depInfo,
                        & sDepInfo );

    if ( qtc::haveDependencies( & sDepInfo ) == ID_TRUE )
    {
        // LEFT
        *aLateralDirection = QMO_JOIN_LATERAL_LEFT;
    }
    else
    {
        qtc::dependencyAnd( & sRightLateralDepInfo,
                            & aJoinGraph->left->depInfo,
                            & sDepInfo );

        if ( qtc::haveDependencies( & sDepInfo ) == ID_TRUE )
        {
            // RIGHT
            *aLateralDirection = QMO_JOIN_LATERAL_RIGHT;
        }
        else
        {
            // 외부 참조가 이루어지지 않고 있음
            *aLateralDirection = QMO_JOIN_LATERAL_NONE;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void qmoJoinMethodMgr::adjustIndexCost( qcStatement      * aStatement,
                                        qmgGraph         * aRight,
                                        qmoIdxCardInfo   * aIndexCardInfo,
                                        qmoPredInfo      * aJoinPredInfo,
                                        SDouble          * aMemCost,
                                        SDouble          * aDiskCost )
{
    UInt             sCount;
    UInt             sColCount;
    UInt             sIdxColumnID;
    qmoPredicate   * sPredicate = NULL;
    qmoPredInfo    * sPredInfo;
    mtcColumn      * sColumns;
    qmoColCardInfo * sColCardInfo;
    qmoStatistics  * sStatistics;
    SDouble          sKeyRangeCost        = 0.0;
    SDouble          sRightFilterCost     = 0.0;
    SDouble          sKeyRangeSelectivity = 1.0;
    SDouble          sNDV = 1.0;
    qmoPredWrapper   sKeyRange;
    idBool           sFind;

    sColumns    = QC_SHARED_TMPLATE(aStatement)->tmplate.rows[aRight->myFrom->tableRef->table].columns;
    sStatistics = aRight->myFrom->tableRef->statInfo;

    //--------------------------------------
    // right predicate 은 일부분이 filter 가 된다.
    //--------------------------------------
    sRightFilterCost = qmoCost::getFilterCost(
                            aStatement->mSysStat,
                            aRight->myPredicate,
                            1 );

    //--------------------------------------
    // join predicate 은 일부분이 filter 가 된다.
    //--------------------------------------
    for ( sPredInfo = aJoinPredInfo;
          sPredInfo != NULL;
          sPredInfo = sPredInfo->next )
    {
        sRightFilterCost += qmoCost::getFilterCost(
                            aStatement->mSysStat,
                            sPredInfo->predicate,
                            1 );
    }

    //--------------------------------------
    // join predicate, right predicate 중 index 를 탈수 있는 것을 찾는다.
    //--------------------------------------

    for ( sCount = 0; sCount < aIndexCardInfo->index->keyColCount; sCount++ )
    {
        sFind        = ID_FALSE;
        sNDV         = QMO_STAT_INDEX_KEY_NDV;
        sIdxColumnID = aIndexCardInfo->index->keyColumns[sCount].column.id;
        sColCardInfo = sStatistics->colCardInfo;

        // BUG-43161 keyColumn의 NDV를 참조하는 위치를 변경함
        for ( sColCount    = 0;
              sColCount < sStatistics->columnCnt;
              sColCount++ )
        {
            if ( sIdxColumnID == sColCardInfo[sColCount].column->column.id )
            {
                sNDV = sColCardInfo[sColCount].columnNDV;
                break;
            }
            else
            {
                // Nothing To Do
            }
        }

        // join predicate 을 우선한다.
        for ( sPredInfo = aJoinPredInfo;
              sPredInfo != NULL;
              sPredInfo = sPredInfo->next )
        {
            if( sIdxColumnID == sPredInfo->predicate->id )
            {
                sPredicate  = sPredInfo->predicate;
                sFind       = ID_TRUE;
                break;
            }
            else
            {
                // Nothing To Do
            }
        }

        if ( sFind == ID_FALSE )
        {
            for ( sPredicate = aRight->myPredicate;
                  sPredicate != NULL;
                  sPredicate = sPredicate->next )
            {
                if( sIdxColumnID == sPredicate->id )
                {
                    sFind = ID_TRUE;
                    break;
                }
                else
                {
                    // Nothing To Do
                }
            }
        }
        else
        {
            // Nothing To Do
        }

        if( sFind == ID_TRUE )
        {
            // BUG-42857
            // Index key column 이 join predicate 과 관련된 경우는
            // 이미 보정된 join predicate 의 totalSelectivity 를 사용하면 안됨
            sKeyRangeSelectivity *= 1 / sNDV;

            sKeyRange.pred = sPredicate;
            sKeyRange.prev = NULL;
            sKeyRange.next = NULL;

            sKeyRangeCost       += qmoCost::getKeyRangeCost(
                                        aStatement->mSysStat,
                                        aRight->myFrom->tableRef->statInfo,
                                        aIndexCardInfo,
                                        &sKeyRange,
                                        1.0 );

            sRightFilterCost    -= qmoCost::getFilterCost(
                                        aStatement->mSysStat,
                                        sPredicate,
                                        1 );

            // 다음 index 컬럼을 사용할수 없는 경우
            if( ( sPredicate->flag & QMO_PRED_NEXT_KEY_USABLE_MASK )
                == QMO_PRED_NEXT_KEY_UNUSABLE )
            {
                break;
            }
            else
            {
                // Nothing To Do
            }
        }
        else
        {
            // 맞는 컬럼이 없으므로 더 검사할 필요가 없다.
            break;
        }
    }

    IDE_DASSERT_MSG( sKeyRangeSelectivity >= 0 && sKeyRangeSelectivity <= 1,
                     "Selectivity : %"ID_DOUBLE_G_FMT"\n",
                     sKeyRangeSelectivity );

    //--------------------------------------
    // index_nl 시 right 그래프의 filter 는 index 의 output 만큼 반복 수행된다.
    //--------------------------------------

    sRightFilterCost  = IDL_MAX( sRightFilterCost, 0.0 );
    sRightFilterCost *= (aRight->costInfo.inputRecordCnt * sKeyRangeSelectivity);

    //--------------------------------------
    // sKeyRangeCost 계산시 Selectivity가 고려안되어 있다.
    //--------------------------------------
    sKeyRangeCost    *= sKeyRangeSelectivity;

    //--------------------------------------
    // index cost 계산
    //--------------------------------------

    (void) qmoCost::getIndexScanCost(
                        aStatement,
                        sColumns,
                        sPredicate,
                        aRight->myFrom->tableRef->statInfo,
                        aIndexCardInfo,
                        sKeyRangeSelectivity,
                        1,
                        sKeyRangeCost,
                        0,
                        sRightFilterCost,
                        aMemCost,
                        aDiskCost );

}
