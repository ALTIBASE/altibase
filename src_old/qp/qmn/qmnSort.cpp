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
 * $Id: qmnSort.cpp 83667 2018-08-09 06:50:14Z ahra.cho $
 *
 * Description :
 *     SORT(SORT) Node
 *
 *     관계형 모델에서 sorting 연산을 수행하는 Plan Node 이다.
 *
 *     다음과 같은 기능을 위해 사용된다.
 *         - ORDER BY
 *         - Sort-based Grouping
 *         - Sort-based Distinction
 *         - Sort Join
 *         - Merge Join
 *         - Sort-based Left Outer Join
 *         - Sort-based Full Outer Join
 *         - Store And Search
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qmoUtil.h>
#include <qmnSort.h>
#include <qcg.h>
#include <qmxResultCache.h>

IDE_RC
qmnSORT::init( qcTemplate * aTemplate,
               qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    SORT 노드의 초기화
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnSORT::init"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncSORT * sCodePlan = (qmncSORT *) aPlan;
    qmndSORT * sDataPlan =
        (qmndSORT *) (aTemplate->tmplate.data + aPlan->offset);
    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];

    idBool sDependency;

    sDataPlan->doIt        = qmnSORT::doItDefault;
    sDataPlan->searchFirst = qmnSORT::searchDefault;
    sDataPlan->searchNext  = qmnSORT::searchDefault;

    //----------------------------------------
    // 최초 초기화 수행
    //----------------------------------------

    if ( (*sDataPlan->flag & QMND_SORT_INIT_DONE_MASK)
         == QMND_SORT_INIT_DONE_FALSE )
    {
        IDE_TEST( firstInit(aTemplate, sCodePlan, sDataPlan) != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    IDE_TEST( checkDependency( aTemplate,
                               sCodePlan,
                               sDataPlan,
                               & sDependency ) != IDE_SUCCESS );

    if ( sDependency == ID_TRUE )
    {
        //----------------------------------------
        // Temp Table 구축 전 초기화
        //----------------------------------------

        IDE_TEST( qmcSortTemp::clear( sDataPlan->sortMgr ) != IDE_SUCCESS );

        //----------------------------------------
        // Child를 반복 수행하여 Temp Table을 구축
        //----------------------------------------

        IDE_TEST( aPlan->left->init( aTemplate,
                                     aPlan->left ) != IDE_SUCCESS);

        IDE_TEST( storeAndSort( aTemplate, sCodePlan, sDataPlan )
                  != IDE_SUCCESS);

        //----------------------------------------
        // Temp Table 구축 후 초기화
        //----------------------------------------

        sDataPlan->depValue = sDataPlan->depTuple->modify;
    }
    else
    {
        // nothing to do
    }

    //----------------------------------------
    // 검색 함수 및 수행 함수 결정
    //----------------------------------------

    IDE_TEST( setSearchFunction( sCodePlan, sDataPlan )
              != IDE_SUCCESS );

    sDataPlan->doIt = qmnSORT::doItFirst;

    // TODO - A4 Grouping Set Integration
    /*
      sDataPlan->needUpdateCountOfSiblings = ID_TRUE;
    */

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC
qmnSORT::doIt( qcTemplate * aTemplate,
               qmnPlan    * aPlan,
               qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    SORT 의 고유 기능을 수행한다.
 *
 * Implementation :
 *    지정된 함수 포인터를 수행한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnSORT::doIt"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // qmncSORT * sCodePlan = (qmncSORT *) aPlan;
    qmndSORT * sDataPlan =
        (qmndSORT *) (aTemplate->tmplate.data + aPlan->offset);

    IDE_TEST( sDataPlan->doIt( aTemplate, aPlan, aFlag ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnSORT::padNull( qcTemplate * aTemplate,
                  qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    SORT 노드의 Tuple에 Null Row를 설정한다.
 *
 * Implementation :
 *    Child Plan의 Null Padding을 수행하고,
 *    자신의 Null Row를 Temp Table로부터 획득한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnSORT::padNull"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncSORT * sCodePlan = (qmncSORT *) aPlan;
    qmndSORT * sDataPlan =
        (qmndSORT *) (aTemplate->tmplate.data + aPlan->offset);

    // first initialization
    if ( (aTemplate->planFlag[sCodePlan->planID] & QMND_SORT_INIT_DONE_MASK)
         == QMND_SORT_INIT_DONE_FALSE )
    {
        IDE_TEST( aPlan->init( aTemplate, aPlan ) != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    IDE_TEST( aPlan->left->padNull( aTemplate, aPlan->left )
              != IDE_SUCCESS );

    IDE_TEST( qmcSortTemp::getNullRow( sDataPlan->sortMgr,
                                       & sDataPlan->plan.myTuple->row )
              != IDE_SUCCESS );

    IDE_DASSERT( sDataPlan->plan.myTuple->row != NULL );

    sDataPlan->plan.myTuple->modify++;

    // To Fix PR-9822
    // padNull() 함수는 Child 의 modify 값을 변경시키게 된다.
    // 이는 재구축 여부와 관계가 없으므로 그 값을 저장하여
    // 재구축이 되지 않도록 한다.
    sDataPlan->depValue = sDataPlan->depTuple->modify;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnSORT::printPlan( qcTemplate   * aTemplate,
                    qmnPlan      * aPlan,
                    ULong          aDepth,
                    iduVarString * aString,
                    qmnDisplay     aMode )
{
/***********************************************************************
 *
 * Description :
 *    SORT 노드의 수행 정보를 출력한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnSORT::printPlan"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncSORT * sCodePlan = (qmncSORT*) aPlan;
    qmndSORT * sDataPlan =
        (qmndSORT*) (aTemplate->tmplate.data + aPlan->offset);
    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];
    qmndSORT * sCacheDataPlan = NULL;

    ULong  i;
    ULong  sDiskPageCnt;
    SLong  sRecordCnt;
    idBool sIsInit      = ID_FALSE;
    //----------------------------
    // Display 위치 결정
    //----------------------------

    for ( i = 0; i < aDepth; i++ )
    {
        iduVarStringAppend( aString,
                            " " );
    }

    //----------------------------
    // 수행 정보 출력
    //----------------------------

    if ( aMode == QMN_DISPLAY_ALL )
    {
        //----------------------------
        // explain plan = on; 인 경우
        //----------------------------

        if ( (*sDataPlan->flag & QMND_SORT_INIT_DONE_MASK)
             == QMND_SORT_INIT_DONE_TRUE )
        {
            sIsInit = ID_TRUE;
            IDE_TEST( qmcSortTemp::getDisplayInfo( sDataPlan->sortMgr,
                                                   & sDiskPageCnt,
                                                   & sRecordCnt )
                      != IDE_SUCCESS );

            if ( (sCodePlan->plan.flag & QMN_PLAN_STORAGE_MASK)
                 == QMN_PLAN_STORAGE_MEMORY )
            {
                // Memory Temp Table인 경우
                // To Fix BUG-9034
                if ( ( sCodePlan->flag & QMNC_SORT_STORE_MASK )
                     == QMNC_SORT_STORE_ONLY )
                {
                    if ( QCU_DISPLAY_PLAN_FOR_NATC == 0 )
                    {
                        iduVarStringAppendFormat(
                            aString,
                            "STORE ( ITEM_SIZE: %"ID_UINT32_FMT", "
                            "ITEM_COUNT: %"ID_INT64_FMT", "
                            "ACCESS: %"ID_UINT32_FMT,
                            sDataPlan->mtrRowSize,
                            sRecordCnt,
                            sDataPlan->plan.myTuple->modify );
                    }
                    else
                    {
                        // BUG-29209
                        // ITEM_SIZE 정보 보여주지 않음
                        iduVarStringAppendFormat(
                            aString,
                            "STORE ( ITEM_SIZE: BLOCKED, "
                            "ITEM_COUNT: %"ID_INT64_FMT", "
                            "ACCESS: %"ID_UINT32_FMT,
                            sRecordCnt,
                            sDataPlan->plan.myTuple->modify );
                    }
                }
                else
                {
                    if ( QCU_DISPLAY_PLAN_FOR_NATC == 0 )
                    {
                        iduVarStringAppendFormat(
                            aString,
                            "SORT ( ITEM_SIZE: %"ID_UINT32_FMT", "
                            "ITEM_COUNT: %"ID_INT64_FMT", "
                            "ACCESS: %"ID_UINT32_FMT,
                            sDataPlan->mtrRowSize,
                            sRecordCnt,
                            sDataPlan->plan.myTuple->modify );
                    }
                    else
                    {
                        // BUG-29209
                        // ITEM_SIZE 정보 보여주지 않음
                        iduVarStringAppendFormat(
                            aString,
                            "SORT ( ITEM_SIZE: BLOCKED, "
                            "ITEM_COUNT: %"ID_INT64_FMT", "
                            "ACCESS: %"ID_UINT32_FMT,
                            sRecordCnt,
                            sDataPlan->plan.myTuple->modify );
                    }
                }
            }
            else
            {
                // Disk Temp Table인 경우
                // To Fix BUG-9034
                if ( ( sCodePlan->flag & QMNC_SORT_STORE_MASK )
                     == QMNC_SORT_STORE_ONLY )
                {
                    if ( QCU_DISPLAY_PLAN_FOR_NATC == 0 )
                    {
                        iduVarStringAppendFormat(
                            aString,
                            "STORE ( ITEM_SIZE: %"ID_UINT32_FMT", "
                            "ITEM_COUNT: %"ID_INT64_FMT", "
                            "DISK_PAGE_COUNT: %"ID_UINT64_FMT", "
                            "ACCESS: %"ID_UINT32_FMT,
                            sDataPlan->mtrRowSize,
                            sRecordCnt,
                            sDiskPageCnt,
                            sDataPlan->plan.myTuple->modify );
                    }
                    else
                    {
                        // BUG-29209
                        // ITEM_SIZE 정보 보여주지 않음
                        iduVarStringAppendFormat(
                            aString,
                            "STORE ( ITEM_SIZE: BLOCKED, "
                            "ITEM_COUNT: %"ID_INT64_FMT", "
                            "DISK_PAGE_COUNT: BLOCKED, "
                            "ACCESS: %"ID_UINT32_FMT,
                            sRecordCnt,
                            sDataPlan->plan.myTuple->modify );
                    }
                }
                else
                {
                    if ( QCU_DISPLAY_PLAN_FOR_NATC == 0 )
                    {
                        iduVarStringAppendFormat(
                            aString,
                            "SORT ( ITEM_SIZE: %"ID_UINT32_FMT", "
                            "ITEM_COUNT: %"ID_INT64_FMT", "
                            "DISK_PAGE_COUNT: %"ID_UINT64_FMT", "
                            "ACCESS: %"ID_UINT32_FMT,
                            sDataPlan->mtrRowSize,
                            sRecordCnt,
                            sDiskPageCnt,
                            sDataPlan->plan.myTuple->modify );
                    }
                    else
                    {
                        // BUG-29209
                        // ITEM_SIZE 정보 보여주지 않음
                        iduVarStringAppendFormat(
                            aString,
                            "SORT ( ITEM_SIZE: BLOCKED, "
                            "ITEM_COUNT: %"ID_INT64_FMT", "
                            "DISK_PAGE_COUNT: BLOCKED, "
                            "ACCESS: %"ID_UINT32_FMT,
                            sRecordCnt,
                            sDataPlan->plan.myTuple->modify );
                    }
                }
            }
        }
        else
        {
            // 초기화되지 않은 경우
            // To Fix BUG-9034
            if ( ( sCodePlan->flag & QMNC_SORT_STORE_MASK )
                 == QMNC_SORT_STORE_ONLY )
            {
                iduVarStringAppendFormat( aString,
                                          "STORE ( ITEM_SIZE: 0, ITEM_COUNT: 0, "
                                          "ACCESS: 0" );
            }
            else
            {
                iduVarStringAppendFormat( aString,
                                          "SORT ( ITEM_SIZE: 0, ITEM_COUNT: 0, "
                                          "ACCESS: 0" );
            }
        }
    }
    else
    {
        //----------------------------
        // explain plan = only; 인 경우
        //----------------------------

        // To Fix BUG-9034
        if ( ( sCodePlan->flag & QMNC_SORT_STORE_MASK )
             == QMNC_SORT_STORE_ONLY )
        {
            iduVarStringAppendFormat( aString,
                                      "STORE ( ITEM_SIZE: ??, ITEM_COUNT: ??, "
                                      "ACCESS: ??" );
        }
        else
        {
            iduVarStringAppendFormat( aString,
                                      "SORT ( ITEM_SIZE: ??, ITEM_COUNT: ??, "
                                      "ACCESS: ??" );
        }
    }

    //----------------------------
    // Cost 출력
    //----------------------------
    qmn::printCost( aString,
                    sCodePlan->plan.qmgAllCost );

    /* PROJ-2462 Result Cache */
    if ( QCU_TRCLOG_DETAIL_RESULTCACHE == 1 )
    {
        if ( ( sCodePlan->componentInfo != NULL ) &&
             ( ( aTemplate->resultCache.flag & QC_RESULT_CACHE_MAX_EXCEED_MASK )
               == QC_RESULT_CACHE_MAX_EXCEED_FALSE ) &&
             ( ( aTemplate->resultCache.flag & QC_RESULT_CACHE_DATA_ALLOC_MASK )
               == QC_RESULT_CACHE_DATA_ALLOC_TRUE ) )
        {
            qmn::printResultCacheRef( aString,
                                      aDepth,
                                      sCodePlan->componentInfo );
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    //----------------------------
    // PROJ-1473 mtrNode info 
    //----------------------------

    if ( QCU_TRCLOG_DETAIL_MTRNODE == 1 )
    {
        if ( ( sCodePlan->componentInfo != NULL ) &&
             ( ( aTemplate->resultCache.flag & QC_RESULT_CACHE_MAX_EXCEED_MASK )
               == QC_RESULT_CACHE_MAX_EXCEED_FALSE ) &&
             ( ( aTemplate->resultCache.flag & QC_RESULT_CACHE_DATA_ALLOC_MASK )
               == QC_RESULT_CACHE_DATA_ALLOC_TRUE ) )
        {
            sCacheDataPlan = (qmndSORT *) (aTemplate->resultCache.data + sCodePlan->plan.offset);
            qmn::printResultCacheInfo( aString,
                                       aDepth,
                                       aMode,
                                       sIsInit,
                                       &sCacheDataPlan->resultData );
        }
        else
        {
            /* Nothing to do */
        }
        qmn::printMTRinfo( aString,
                           aDepth,
                           sCodePlan->myNode,
                           "myNode",
                           sCodePlan->myNode->dstNode->node.table,
                           sCodePlan->depTupleRowID,
                           ID_USHORT_MAX );
    }

    if (QCG_GET_SESSION_TRCLOG_DETAIL_PREDICATE(aTemplate->stmt) == 1)
    {
        if (sCodePlan->range != NULL)
        {
            for ( i = 0; i < aDepth; i++ )
            {
                iduVarStringAppend( aString,
                                    " " );
            }
            iduVarStringAppend( aString,
                                " [ FILTER ]\n" );
            IDE_TEST(qmoUtil::printPredInPlan(aTemplate,
                                              aString,
                                              aDepth + 1,
                                              sCodePlan->range)
                     != IDE_SUCCESS);
        }
    }

    //----------------------------
    // Operator별 결과 정보 출력
    //----------------------------
    if ( QCU_TRCLOG_RESULT_DESC == 1 )
    {
        IDE_TEST( qmn::printResult( aTemplate,
                                    aDepth,
                                    aString,
                                    sCodePlan->plan.resultDesc )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    //----------------------------
    // Child Plan 정보 출력
    //----------------------------

    IDE_TEST( aPlan->left->printPlan( aTemplate,
                                      aPlan->left,
                                      aDepth + 1,
                                      aString,
                                      aMode ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnSORT::doItDefault( qcTemplate * /* aTemplate */,
                      qmnPlan    * /* aPlan */,
                      qmcRowFlag * /* aFlag */)
{
/***********************************************************************
 *
 * Description :
 *    이 함수가 수행되면 안됨.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnSORT::doItDefault"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_DASSERT(0);

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnSORT::doItFirst( qcTemplate * aTemplate,
                    qmnPlan    * aPlan,
                    qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    최초 수행 함수
 *
 * Implementation :
 *    검색 후 Row가 존재하면, Tuple Set에 복원한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnSORT::doItFirst"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncSORT * sCodePlan = (qmncSORT *) aPlan;
    qmndSORT * sDataPlan =
        (qmndSORT *) (aTemplate->tmplate.data + aPlan->offset);

    IDE_TEST( iduCheckSessionEvent( aTemplate->stmt->mStatistics )
              != IDE_SUCCESS );

    // Option에 따른 검색 수행
    IDE_TEST( sDataPlan->searchFirst( aTemplate,
                                      sCodePlan,
                                      sDataPlan,
                                      aFlag ) != IDE_SUCCESS );

    // Data가 존재할 경우 Tuple Set 복원
    if ( (*aFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
    {
        IDE_TEST( setTupleSet( aTemplate,
                               sDataPlan ) != IDE_SUCCESS );

        sDataPlan->doIt = qmnSORT::doItNext;
    }
    else
    {
        // nothing to do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnSORT::doItNext( qcTemplate * aTemplate,
                   qmnPlan    * aPlan,
                   qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *     다음 수행 함수
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnSORT::doItNext"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncSORT * sCodePlan = (qmncSORT *) aPlan;
    qmndSORT * sDataPlan =
        (qmndSORT *) (aTemplate->tmplate.data + aPlan->offset);

    // 옵션에 맞는 검색 수행
    IDE_TEST( sDataPlan->searchNext( aTemplate,
                                     sCodePlan,
                                     sDataPlan,
                                     aFlag ) != IDE_SUCCESS );

    // Data가 존재할 경우 Tuple Set 복원
    if ( (*aFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
    {
        IDE_TEST( setTupleSet( aTemplate,
                               sDataPlan ) != IDE_SUCCESS );
    }
    else
    {
        sDataPlan->doIt = qmnSORT::doItFirst;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnSORT::doItLast( qcTemplate * aTemplate,
                   qmnPlan    * aPlan,
                   qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *     TODO - A4 Grouping Set Integration
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnSORT::doItLast"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // qmncSORT * sCodePlan = (qmncSORT *) aPlan;
    qmndSORT * sDataPlan =
        (qmndSORT *) (aTemplate->tmplate.data + aPlan->offset);

    IDE_TEST( iduCheckSessionEvent( aTemplate->stmt->mStatistics )
              != IDE_SUCCESS );

    // set that no data found
    *aFlag = QMC_ROW_DATA_NONE;

    sDataPlan->doIt = qmnSORT::doItFirst;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


void qmnSORT::setHitSearch( qcTemplate * aTemplate,
                            qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    Probing (LEFT) Table에 대한 처리가 끝나고, Driving (RIGHT) Table에 대한
 *    처리를 수행하기 위해 사용된다.
 *
 * Implementation :
 *    검색 함수를 강제적으로 Hit 검색으로 전환한다.
 *
 ***********************************************************************/
    qmndSORT * sDataPlan =
        (qmndSORT *) (aTemplate->tmplate.data + aPlan->offset);

    sDataPlan->searchFirst = qmnSORT::searchFirstHit;
    sDataPlan->searchNext = qmnSORT::searchNextHit;
    sDataPlan->doIt = qmnSORT::doItFirst;
}


void qmnSORT::setNonHitSearch( qcTemplate * aTemplate,
                               qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    Probing (LEFT) Table에 대한 처리가 끝나고, Driving (RIGHT) Table에 대한
 *    처리를 수행하기 위해 사용된다.
 *
 * Implementation :
 *    검색 함수를 강제적으로 Non-Hit 검색으로 전환한다.
 *
 ***********************************************************************/

    qmndSORT * sDataPlan =
        (qmndSORT *) (aTemplate->tmplate.data + aPlan->offset);

    sDataPlan->searchFirst = qmnSORT::searchFirstNonHit;
    sDataPlan->searchNext = qmnSORT::searchNextNonHit;
    sDataPlan->doIt = qmnSORT::doItFirst;
}


IDE_RC
qmnSORT::setHitFlag( qcTemplate * aTemplate,
                     qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *     Full Outer Join의 처리시,
 *     Left Outer Join의 처리 과정 중 조건에 만족하는 Record를
 *     검색하는 경우 Hit Flag을 설정한다.
 *
 * Implementation :
 *     Temp Table을 이용하여 현재 Row의 Hit Flag을 셋팅한다.
 *     이미 Temp Table은 현재 Row의 존재를 알고 있어 별도의 인자가
 *     필요 없다.
 *
 ***********************************************************************/

#define IDE_FN "qmnSORT::setHitFlag"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // qmncSORT * sCodePlan = (qmncSORT *) aPlan;
    qmndSORT * sDataPlan =
        (qmndSORT *) (aTemplate->tmplate.data + aPlan->offset);

    // 현재 저장 Row의 Hit Flag을 셋팅
    IDE_TEST( qmcSortTemp::setHitFlag( sDataPlan->sortMgr )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

idBool qmnSORT::isHitFlagged( qcTemplate * aTemplate,
                              qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description : 
 *     현재 Row에 Hit Flag가 있는지 검증한다.
 *
 * Implementation :
 *
 *     Temp Table을 이용하여 현재 Row에 Hit Flag가 있는지 검증한다.
 *
 ***********************************************************************/

    qmndSORT * sDataPlan =
        (qmndSORT *) (aTemplate->tmplate.data + aPlan->offset);

    return qmcSortTemp::isHitFlagged( sDataPlan->sortMgr );
}

IDE_RC
qmnSORT::storeCursor( qcTemplate * aTemplate,
                      qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *     Merge Join등에서 현재 위치의 Cursor를 저장하기 위하여 사용한다.
 *
 * Implementation :
 *     Temp Table의 커서 저장 기능을 사용한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnSORT::storeCursor"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // qmncSORT * sCodePlan = (qmncSORT *) aPlan;
    qmndSORT * sDataPlan =
        (qmndSORT *) (aTemplate->tmplate.data + aPlan->offset);

    // 현재 위치의 커서를 저장함
    IDE_TEST( qmcSortTemp::storeCursor( sDataPlan->sortMgr,
                                        & sDataPlan->cursorInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnSORT::restoreCursor( qcTemplate * aTemplate,
                        qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *     Merge Join에서 사용하며,
 *     저장된 Cursor 위치로 커서를 복원시킨다.
 *
 * Implementation :
 *     Temp Table의 커서 복원 기능을 사용한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnSORT::restoreCursor"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmndSORT * sDataPlan =
        (qmndSORT *) (aTemplate->tmplate.data + aPlan->offset);

    // 현재 위치의 커서를 지정된 위치로 복원시킴
    IDE_TEST( qmcSortTemp::restoreCursor( sDataPlan->sortMgr,
                                          & sDataPlan->cursorInfo )
              != IDE_SUCCESS );

    // 검색된 Row를 이용한 Tuple Set 복원
    IDE_TEST( setTupleSet( aTemplate, sDataPlan )
              != IDE_SUCCESS );

    // 커서의 복원 후의 수행 함수
    sDataPlan->doIt = qmnSORT::doItNext;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnSORT::firstInit( qcTemplate * aTemplate,
                    qmncSORT   * aCodePlan,
                    qmndSORT   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    SORT node의 Data 영역의 멤버에 대한 초기화를 수행
 *
 * Implementation :
 *
 ***********************************************************************/
    qmndSORT * sCacheDataPlan = NULL;

    //---------------------------------
    // 적합성 검사
    //---------------------------------

    //---------------------------------
    // SORT 고유 정보의 초기화
    //---------------------------------
    //
    /* PROJ-2462 Result Cache */
    if ( ( *aDataPlan->flag & QMN_PLAN_RESULT_CACHE_EXIST_MASK )
         == QMN_PLAN_RESULT_CACHE_EXIST_TRUE )
    {
        sCacheDataPlan = (qmndSORT *) (aTemplate->resultCache.data + aCodePlan->plan.offset);
        sCacheDataPlan->resultData.flag = &aTemplate->resultCache.dataFlag[aCodePlan->planID];
        aDataPlan->resultData.flag     = &aTemplate->resultCache.dataFlag[aCodePlan->planID];
        if ( qmxResultCache::initResultCache( aTemplate,
                                              aCodePlan->componentInfo,
                                              &sCacheDataPlan->resultData )
             != IDE_SUCCESS )
        {
            *aDataPlan->flag &= ~QMN_PLAN_RESULT_CACHE_EXIST_MASK;
            *aDataPlan->flag |= QMN_PLAN_RESULT_CACHE_EXIST_FALSE;
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    IDE_TEST( initMtrNode( aTemplate, aCodePlan, aDataPlan ) != IDE_SUCCESS );

    IDE_TEST( initSortNode( aCodePlan, aDataPlan ) != IDE_SUCCESS );

    aDataPlan->mtrRowSize = qmc::getMtrRowSize( aDataPlan->mtrNode );

    aDataPlan->plan.myTuple = aDataPlan->mtrNode->dstTuple;

    aDataPlan->depTuple = & aTemplate->tmplate.rows[aCodePlan->depTupleRowID];
    aDataPlan->depValue = QMN_PLAN_DEFAULT_DEPENDENCY_VALUE;

    //---------------------------------
    // Temp Table의 초기화
    //---------------------------------

    IDE_TEST( initTempTable( aTemplate, aCodePlan, aDataPlan )
              != IDE_SUCCESS );

    //---------------------------------
    // 초기화 완료를 표기
    //---------------------------------

    *aDataPlan->flag &= ~QMND_SORT_INIT_DONE_MASK;
    *aDataPlan->flag |= QMND_SORT_INIT_DONE_TRUE;

    /* PROJ-2462 Result Cache */
    if ( ( *aDataPlan->flag & QMN_PLAN_RESULT_CACHE_EXIST_MASK )
         == QMN_PLAN_RESULT_CACHE_EXIST_TRUE )
    {
        *aDataPlan->resultData.flag &= ~QMX_RESULT_CACHE_INIT_DONE_MASK;
        *aDataPlan->resultData.flag |= QMX_RESULT_CACHE_INIT_DONE_TRUE;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnSORT::initMtrNode( qcTemplate * aTemplate,
                      qmncSORT   * aCodePlan,
                      qmndSORT   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    저장 Column의 관리를 위한 노드를 초기화한다.
 *
 * Implementation :
 *
 ***********************************************************************/
    UInt         sHeaderSize;
    //---------------------------------
    // 적합성 검사
    //---------------------------------

    IDE_DASSERT( aCodePlan->mtrNodeOffset > 0 );

    aDataPlan->mtrNode =
        (qmdMtrNode*) (aTemplate->tmplate.data + aCodePlan->mtrNodeOffset);
    //---------------------------------
    // 저장 관리를 위한 정보의 초기화
    //---------------------------------

    if ( (aCodePlan->plan.flag & QMN_PLAN_STORAGE_MASK)
         == QMN_PLAN_STORAGE_MEMORY )
    {
        sHeaderSize = QMC_MEMSORT_TEMPHEADER_SIZE;

        /* PROJ-2462 Result Cache */
        if ( ( *aDataPlan->flag & QMN_PLAN_RESULT_CACHE_EXIST_MASK )
             == QMN_PLAN_RESULT_CACHE_EXIST_TRUE )
        {
            aDataPlan->mtrNode = ( qmdMtrNode * )( aTemplate->resultCache.data +
                                                   aCodePlan->mtrNodeOffset );
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        sHeaderSize = QMC_DISKSORT_TEMPHEADER_SIZE;
    }

    //---------------------------------
    // 저장 Column의 초기화
    //---------------------------------

    // 1.  저장 Column의 연결 정보 생성
    // 2.  저장 Column의 초기화
    // 3.  저장 Column의 offset을 재조정
    // 4.  Row Size의 계산
    //     - Disk Temp Table의 경우 Row를 위한 Memory도 할당받음.

    IDE_TEST( qmc::linkMtrNode( aCodePlan->myNode,
                                aDataPlan->mtrNode ) != IDE_SUCCESS );

    IDE_TEST( qmc::initMtrNode( aTemplate,
                                aDataPlan->mtrNode,
                                aCodePlan->baseTableCount )
              != IDE_SUCCESS );

    IDE_TEST( qmc::refineOffsets( aDataPlan->mtrNode,
                                  sHeaderSize )
              != IDE_SUCCESS );

    IDE_TEST( qmc::setRowSize( aTemplate->stmt->qmxMem,
                               & aTemplate->tmplate,
                               aDataPlan->mtrNode->dstNode->node.table )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnSORT::initSortNode( qmncSORT   * aCodePlan,
                       qmndSORT   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *     저장 Column의 정보 중 정렬 Column의 시작 위치를 찾는다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnSORT::initSortNode"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmdMtrNode * sNode;

    // 최초 정렬 Column의 위치 검색
    for ( sNode = aDataPlan->mtrNode; sNode != NULL; sNode = sNode->next )
    {
        if ( ( sNode->myNode->flag & QMC_MTR_SORT_NEED_MASK )
             == QMC_MTR_SORT_NEED_TRUE )
        {
            break;
        }
    }

    aDataPlan->sortNode = sNode;

    //-----------------------------
    // 적합성 검사
    //-----------------------------

    switch ( aCodePlan->flag & QMNC_SORT_STORE_MASK )
    {
        case QMNC_SORT_STORE_NONE:
            IDE_DASSERT(0);
            break;
        case QMNC_SORT_STORE_ONLY:
            IDE_DASSERT( aDataPlan->sortNode == NULL );
            break;
        case QMNC_SORT_STORE_PRESERVE:
        case QMNC_SORT_STORE_SORTING:
            IDE_DASSERT( aDataPlan->sortNode != NULL );
            break;
        default:
            IDE_DASSERT(0);
            break;
    }

    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC
qmnSORT::initTempTable( qcTemplate * aTemplate,
                        qmncSORT   * aCodePlan,
                        qmndSORT   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *     Sort Temp Table을 초기화한다.
 *
 * Implementation :
 *
 ***********************************************************************/
    UInt       sFlag          = 0;
    qmndSORT * sCacheDataPlan = NULL;

    //-----------------------------
    // 적합성 검사
    //-----------------------------

    //-----------------------------
    // Flag 정보 초기화
    //-----------------------------

    sFlag = QMCD_SORT_TMP_INITIALIZE;

    if ( (aCodePlan->plan.flag & QMN_PLAN_STORAGE_MASK)
         == QMN_PLAN_STORAGE_MEMORY )
    {
        sFlag &= ~QMCD_SORT_TMP_STORAGE_TYPE;
        sFlag |= QMCD_SORT_TMP_STORAGE_MEMORY;

        IDE_DASSERT( (aDataPlan->plan.myTuple->lflag & MTC_TUPLE_STORAGE_MASK)
                     == MTC_TUPLE_STORAGE_MEMORY );
    }
    else
    {
        sFlag &= ~QMCD_SORT_TMP_STORAGE_TYPE;
        sFlag |= QMCD_SORT_TMP_STORAGE_DISK;

        IDE_DASSERT( (aDataPlan->plan.myTuple->lflag & MTC_TUPLE_STORAGE_MASK)
                     == MTC_TUPLE_STORAGE_DISK );
    }

    /* PROJ-2201 Innovation in sorting and hashing(temp)
     * QMNC의 Flag를 QMND로 이어받음 */
    if( ( aCodePlan->flag & QMNC_SORT_SEARCH_MASK )
        == QMNC_SORT_SEARCH_SEQUENTIAL )
    {
        sFlag &= ~QMCD_SORT_TMP_SEARCH_MASK;
        sFlag |= QMCD_SORT_TMP_SEARCH_SEQUENTIAL;
    }
    else
    {
        sFlag &= ~QMCD_SORT_TMP_SEARCH_MASK;
        sFlag |= QMCD_SORT_TMP_SEARCH_RANGE;

        IDE_DASSERT( (aCodePlan->flag & QMNC_SORT_SEARCH_MASK )
                     == QMNC_SORT_SEARCH_RANGE );
    }

    //-----------------------------
    // Temp Table 초기화
    //-----------------------------

    if ( ( *aDataPlan->flag & QMN_PLAN_RESULT_CACHE_EXIST_MASK )
         == QMN_PLAN_RESULT_CACHE_EXIST_FALSE )
    {
        IDU_FIT_POINT( "qmnSORT::initTempTable::qmxAlloc:sortMgr",
                       idERR_ABORT_InsufficientMemory );
        IDE_TEST( aTemplate->stmt->qmxMem->alloc( ID_SIZEOF( qmcdSortTemp ),
                                                  (void **)&aDataPlan->sortMgr )
                  != IDE_SUCCESS );
        IDE_TEST( qmcSortTemp::init( aDataPlan->sortMgr,
                                     aTemplate,
                                     ID_UINT_MAX,
                                     aDataPlan->mtrNode,
                                     aDataPlan->sortNode,
                                     aCodePlan->storeRowCount,
                                     sFlag )
                  != IDE_SUCCESS );
    }
    else
    {
        /* PROJ-2462 Result Cache */
        sCacheDataPlan = (qmndSORT *) (aTemplate->resultCache.data +
                                      aCodePlan->plan.offset);

        if ( ( *aDataPlan->resultData.flag & QMX_RESULT_CACHE_INIT_DONE_MASK )
             == QMX_RESULT_CACHE_INIT_DONE_FALSE )
        {
            IDU_FIT_POINT( "qmnSORT::initTempTable::qrcAlloc:sortMgr",
                           idERR_ABORT_InsufficientMemory );
            IDE_TEST( sCacheDataPlan->resultData.memory->alloc( ID_SIZEOF( qmcdSortTemp ),
                                                               (void **)&aDataPlan->sortMgr )
                      != IDE_SUCCESS );

            IDE_TEST( qmcSortTemp::init( aDataPlan->sortMgr,
                                         aTemplate,
                                         sCacheDataPlan->resultData.memoryIdx,
                                         aDataPlan->mtrNode,
                                         aDataPlan->sortNode,
                                         aCodePlan->storeRowCount,
                                         sFlag )
                      != IDE_SUCCESS );
            sCacheDataPlan->sortMgr = aDataPlan->sortMgr;
        }
        else
        {
            aDataPlan->sortMgr = sCacheDataPlan->sortMgr;
            IDE_TEST( qmcSortTemp::clearHitFlag( aDataPlan->sortMgr )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnSORT::checkDependency( qcTemplate * aTemplate,
                                 qmncSORT   * aCodePlan,
                                 qmndSORT   * aDataPlan,
                                 idBool     * aDependent )
{
/***********************************************************************
 *
 * Description :
 *    Dependent Tuple에 변화가 있는 지를 검사
 *
 * Implementation :
 *
 ***********************************************************************/
    idBool sDep = ID_FALSE;

    if ( aDataPlan->depValue != aDataPlan->depTuple->modify )
    {
        if ( ( *aDataPlan->flag & QMN_PLAN_RESULT_CACHE_EXIST_MASK )
             == QMN_PLAN_RESULT_CACHE_EXIST_FALSE )
        {
            sDep = ID_TRUE;
        }
        else
        {
            aDataPlan->resultData.flag = &aTemplate->resultCache.dataFlag[aCodePlan->planID];
            if ( ( ( *aDataPlan->resultData.flag & QMX_RESULT_CACHE_STORED_MASK )
                   == QMX_RESULT_CACHE_STORED_TRUE ) &&
                ( aDataPlan->depValue == QMN_PLAN_DEFAULT_DEPENDENCY_VALUE ) )
            {
                sDep = ID_FALSE;
            }
            else
            {
                sDep = ID_TRUE;
            }
        }
    }
    else
    {
        sDep = ID_FALSE;
    }

    *aDependent = sDep;

    return IDE_SUCCESS;
}

IDE_RC
qmnSORT::storeAndSort( qcTemplate * aTemplate,
                       qmncSORT   * aCodePlan,
                       qmndSORT   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    저장 및 정렬 수행
 *
 * Implementation :
 *    Child를 반복적으로 수행하여 이를 저장하고,
 *    마지막으로 Sorting을 수행한다.
 *
 ***********************************************************************/

    qmcRowFlag sFlag = QMC_ROW_INITIALIZE;

    //------------------------------
    // Child Record의 저장
    //------------------------------

    IDE_TEST( aCodePlan->plan.left->doIt( aTemplate,
                                          aCodePlan->plan.left,
                                          & sFlag ) != IDE_SUCCESS );

    while ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
    {
        IDE_TEST( addOneRow( aTemplate, aDataPlan )
                  != IDE_SUCCESS );

        IDE_TEST( aCodePlan->plan.left->doIt( aTemplate,
                                              aCodePlan->plan.left,
                                              & sFlag ) != IDE_SUCCESS );

    }

    //------------------------------
    // 정렬 수행
    //------------------------------

    //  지정한 저장 옵션에 따라 정렬 여부 결정
    if ( (aCodePlan->plan.flag & QMN_PLAN_STORAGE_MASK)
         == QMN_PLAN_STORAGE_DISK )
    {
        // disk일 경우 SORTING or PRESEVED_ORDER 일 경우 정렬을 함
        if( ( aCodePlan->flag & QMNC_SORT_STORE_MASK )
            == QMNC_SORT_STORE_SORTING ||
            ( aCodePlan->flag & QMNC_SORT_STORE_MASK )
            == QMNC_SORT_STORE_PRESERVE )
        {
            IDE_TEST( qmcSortTemp::sort( aDataPlan->sortMgr )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // memory일 경우 SORTING 일때만 정렬을 함
        if( ( aCodePlan->flag & QMNC_SORT_STORE_MASK )
            == QMNC_SORT_STORE_SORTING )
        {
            IDE_TEST( qmcSortTemp::sort( aDataPlan->sortMgr )
                      != IDE_SUCCESS );
        }
    }

    // PROJ-2462 Result Cache
    if ( ( *aDataPlan->flag & QMN_PLAN_RESULT_CACHE_EXIST_MASK )
         == QMN_PLAN_RESULT_CACHE_EXIST_TRUE )
    {
        *aDataPlan->resultData.flag &= ~QMX_RESULT_CACHE_STORED_MASK;
        *aDataPlan->resultData.flag |= QMX_RESULT_CACHE_STORED_TRUE;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmnSORT::addOneRow( qcTemplate * aTemplate,
                    qmndSORT   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    Temp Table에 하나의 Record를 구성하여 삽입한다.
 *
 * Implementation :
 *    1. 공간 할당 : Temp Table을 이용하여 공간 할당을 받으며,
 *                   Memory Temp Table의 경우에만 별도의 공간을
 *                   할당해 준다.
 *    2. 저장 Row의 구성
 *    3. Row의 삽입
 *
 ***********************************************************************/

#define IDE_FN "qmnSORT::addOneRow"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDU_FIT_POINT( "qmnSORT::addOneRow::alloc::myTupleRow",
                    idERR_ABORT_InsufficientMemory );

    // 공간의 할당
    IDE_TEST( qmcSortTemp::alloc( aDataPlan->sortMgr,
                                  & aDataPlan->plan.myTuple->row )
              != IDE_SUCCESS);

    // 저장 Row의 구성
    IDE_TEST( setMtrRow( aTemplate,
                         aDataPlan ) != IDE_SUCCESS );

    // Row의 삽입
    IDE_TEST( qmcSortTemp::addRow( aDataPlan->sortMgr,
                                   aDataPlan->plan.myTuple->row )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC
qmnSORT::setMtrRow( qcTemplate * aTemplate,
                    qmndSORT   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *     저장 Row를 구성한다.
 *
 * Implementation :
 *     저장 Column을 순회하며, 저장 Row를 구성한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnSORT::setMtrRow"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmdMtrNode * sNode;

    for ( sNode = aDataPlan->mtrNode; sNode != NULL; sNode = sNode->next )
    {
        if ( sNode->func.setMtr == qmc::setMtrByCopy )
        {
            IDE_TEST( sNode->func.setMtr( aTemplate,
                                          sNode,
                                          aDataPlan->plan.myTuple->row )
                      != IDE_SUCCESS );
        }
    }

    // BUG-46279 Disk temp 사용 시 grouping 데이터의 정렬이 subquery를 참조하는 경우
    //           결과 값 오류가 발생합니다.  
    for ( sNode = aDataPlan->mtrNode; sNode != NULL; sNode = sNode->next )
    {
        if ( sNode->func.setMtr != qmc::setMtrByCopy )
        {
            IDE_TEST( sNode->func.setMtr( aTemplate,
                                          sNode,
                                          aDataPlan->plan.myTuple->row )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC
qmnSORT::setTupleSet( qcTemplate * aTemplate,
                      qmndSORT   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *     검색된 저장 Row를 기준으로 Tuple Set을 복원한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnSORT::setTupleSet"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmdMtrNode * sNode;

    for ( sNode = aDataPlan->mtrNode; sNode != NULL; sNode = sNode->next )
    {
        IDE_TEST( sNode->func.setTuple( aTemplate, sNode, aDataPlan->plan.myTuple->row )
                  != IDE_SUCCESS );
    }

    aDataPlan->plan.myTuple->modify++;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnSORT::setSearchFunction( qmncSORT   * aCodePlan,
                            qmndSORT   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *     SORT 노드의 검색 옵션에 부합하는 검색 함수를 결정한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnSORT::setSearchFunction"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    switch ( aCodePlan->flag & QMNC_SORT_SEARCH_MASK )
    {
        case QMNC_SORT_SEARCH_SEQUENTIAL:
            aDataPlan->searchFirst = qmnSORT::searchFirstSequence;
            aDataPlan->searchNext = qmnSORT::searchNextSequence;
            break;
        case QMNC_SORT_SEARCH_RANGE:
            aDataPlan->searchFirst = qmnSORT::searchFirstRangeSearch;
            aDataPlan->searchNext = qmnSORT::searchNextRangeSearch;

            // 적합성 검사
            IDE_DASSERT( aCodePlan->range != NULL );
            IDE_DASSERT( aDataPlan->sortNode != NULL );

            break;
        default:
            IDE_DASSERT( 0 );
            break;
    }

    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC
qmnSORT::searchDefault( qcTemplate * /* aTemplate */,
                        qmncSORT   * /* aCodePlan */,
                        qmndSORT   * /* aDataPlan */,
                        qmcRowFlag * /* aFlag */ )
{
/***********************************************************************
 *
 * Description :
 *    호출되어서는 안됨
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnSORT::searchDefault"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_DASSERT(0);

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnSORT::searchFirstSequence( qcTemplate * /* aTemplate */,
                              qmncSORT   * /* aCodePlan */,
                              qmndSORT   * aDataPlan,
                              qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *     첫번째 순차 검색을 수행
 *
 * Implementation :
 *     Temp Table을 이용하여 검색한다.
 *
 *     - Memory 공간의 관리
 *         Disk Temp Table을 사용하는 경우 해당 Tuple의 메모리 공간을
 *         상실할 수 있다.  이를 위해 두 벌의 BackUp 이 필요하다.
 *
 ***********************************************************************/

#define IDE_FN "qmnSORT::searchFirstSequence"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    void * sOrgRow;
    void * sSearchRow;

    // 첫번째 순차 검색
    sOrgRow = sSearchRow = aDataPlan->plan.myTuple->row;
    IDE_TEST( qmcSortTemp::getFirstSequence( aDataPlan->sortMgr,
                                             & sSearchRow )
              != IDE_SUCCESS );
    aDataPlan->plan.myTuple->row = (sSearchRow == NULL) ? sOrgRow : sSearchRow;

    // Row 존재 유무 설정
    if ( sSearchRow == NULL )
    {
        *aFlag = QMC_ROW_DATA_NONE;
    }
    else
    {
        *aFlag = QMC_ROW_DATA_EXIST;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnSORT::searchNextSequence( qcTemplate * /* aTemplate */,
                             qmncSORT   * /* aCodePlan */,
                             qmndSORT   * aDataPlan,
                             qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *     다음 순차 검색을 수행
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnSORT::searchNextSequence"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    void * sOrgRow;
    void * sSearchRow;

    // 다음 순차 검색
    sOrgRow = sSearchRow = aDataPlan->plan.myTuple->row;
    IDE_TEST( qmcSortTemp::getNextSequence( aDataPlan->sortMgr,
                                            & sSearchRow )
              != IDE_SUCCESS );
    aDataPlan->plan.myTuple->row = (sSearchRow == NULL) ? sOrgRow : sSearchRow;

    // Row 존재 유무 설정
    if ( sSearchRow == NULL )
    {
        *aFlag = QMC_ROW_DATA_NONE;
    }
    else
    {
        *aFlag = QMC_ROW_DATA_EXIST;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnSORT::searchFirstRangeSearch( qcTemplate * /* aTemplate */,
                                 qmncSORT   * aCodePlan,
                                 qmndSORT   * aDataPlan,
                                 qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *     첫 번째 Range 검색
 *
 * Implementation :
 *     Range Predicate(DNF로 구성)을 Temp Table로 넘기면,
 *     이를 이용하여 Temp Table이 Range 검색을 수행한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnSORT::searchFirstRangeSearch"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    void * sOrgRow;
    void * sSearchRow;

    // 첫번째 Range 검색
    sOrgRow = sSearchRow = aDataPlan->plan.myTuple->row;
    IDE_TEST( qmcSortTemp::getFirstRange( aDataPlan->sortMgr,
                                          aCodePlan->range,
                                          & sSearchRow )
              != IDE_SUCCESS );
    aDataPlan->plan.myTuple->row = (sSearchRow == NULL) ? sOrgRow : sSearchRow;


    // Row 존재 유무 설정
    if ( sSearchRow == NULL )
    {
        *aFlag = QMC_ROW_DATA_NONE;
    }
    else
    {
        *aFlag = QMC_ROW_DATA_EXIST;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnSORT::searchNextRangeSearch( qcTemplate * /* aTemplate */,
                                qmncSORT   * /* aCodePlan */,
                                qmndSORT   * aDataPlan,
                                qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    다음 Range 검색
 *
 * Implementation :
 *    첫번째 Range검색 시 사용된 Range Predicate을 이용하여
 *    다음 Row를 검색한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnSORT::searchNextRangeSearch"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    void * sOrgRow;
    void * sSearchRow;

    // 다음 Range 검색
    sOrgRow = sSearchRow = aDataPlan->plan.myTuple->row;
    IDE_TEST( qmcSortTemp::getNextRange( aDataPlan->sortMgr,
                                         & sSearchRow )
              != IDE_SUCCESS );
    aDataPlan->plan.myTuple->row = (sSearchRow == NULL) ? sOrgRow : sSearchRow;

    // Row 존재 유무 설정
    if ( sSearchRow == NULL )
    {
        *aFlag = QMC_ROW_DATA_NONE;
    }
    else
    {
        *aFlag = QMC_ROW_DATA_EXIST;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qmnSORT::searchFirstHit( qcTemplate * /* aTemplate */,
                                qmncSORT   * /* aCodePlan */,
                                qmndSORT   * aDataPlan,
                                qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    첫번째 Hit 검색
 *
 * Implementation :
 *    Inverse Sort Join의 처리에 참여할 때 사용되는 함수로
 *    Hit Record가 더 이상 없을 경우, 모든 저장 Row의 Hit Flag을
 *    초기화한다.  이는 Subquery등에서 Inverse Sort Join이 사용될 때,
 *    최초 수행 결과로 인해 다음 Subquery의 수행이 영향을 받지 않도록
 *    하기 위함이다.
 *
 ***********************************************************************/

    void * sOrgRow;
    void * sSearchRow;

    // 첫번째 Hit 검색
    sOrgRow = sSearchRow = aDataPlan->plan.myTuple->row;
    IDE_TEST( qmcSortTemp::getFirstHit( aDataPlan->sortMgr,
                                        & sSearchRow )
              != IDE_SUCCESS );
    aDataPlan->plan.myTuple->row = (sSearchRow == NULL) ? sOrgRow : sSearchRow;

    // Row 존재 유무 설정
    if ( sSearchRow == NULL )
    {
        IDE_TEST( qmcSortTemp::clearHitFlag( aDataPlan->sortMgr )
                  != IDE_SUCCESS );

        *aFlag = QMC_ROW_DATA_NONE;
    }
    else
    {
        *aFlag = QMC_ROW_DATA_EXIST;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qmnSORT::searchNextHit( qcTemplate * /* aTemplate */,
                               qmncSORT   * /* aCodePlan */,
                               qmndSORT   * aDataPlan,
                               qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *     다음 Hit 검색
 *
 * Implementation :
 *
 ***********************************************************************/

    void * sOrgRow;
    void * sSearchRow;

    // 다음 Hit 검색
    sOrgRow = sSearchRow = aDataPlan->plan.myTuple->row;
    IDE_TEST( qmcSortTemp::getNextHit( aDataPlan->sortMgr,
                                       & sSearchRow )
              != IDE_SUCCESS );
    aDataPlan->plan.myTuple->row = (sSearchRow == NULL) ? sOrgRow : sSearchRow;

    // Row 존재 유무 설정
    if ( sSearchRow == NULL )
    {
        IDE_TEST( qmcSortTemp::clearHitFlag( aDataPlan->sortMgr )
                  != IDE_SUCCESS );

        *aFlag = QMC_ROW_DATA_NONE;
    }
    else
    {
        *aFlag = QMC_ROW_DATA_EXIST;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnSORT::searchFirstNonHit( qcTemplate * /* aTemplate */,
                            qmncSORT   * /* aCodePlan */,
                            qmndSORT   * aDataPlan,
                            qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    첫번째 Non-Hit 검색
 *
 * Implementation :
 *    Full Outer Join의 처리에 참여할 때 사용되는 함수로
 *    Non-Hit Record가 더 이상 없을 경우, 모든 저장 Row의 Hit Flag을
 *    초기화한다.  이는 Subquery등에서 Full Outer Join이 사용될 때,
 *    최초 수행 결과로 인해 다음 Subquery의 수행이 영향을 받지 않도록
 *    하기 위함이다.
 *
 ***********************************************************************/

#define IDE_FN "qmnSORT::searchFirstNonHit"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    void * sOrgRow;
    void * sSearchRow;

    // 첫번째 Non-Hit 검색
    sOrgRow = sSearchRow = aDataPlan->plan.myTuple->row;
    IDE_TEST( qmcSortTemp::getFirstNonHit( aDataPlan->sortMgr,
                                           & sSearchRow )
              != IDE_SUCCESS );
    aDataPlan->plan.myTuple->row = (sSearchRow == NULL) ? sOrgRow : sSearchRow;

    // Row 존재 유무 설정
    if ( sSearchRow == NULL )
    {
        IDE_TEST( qmcSortTemp::clearHitFlag( aDataPlan->sortMgr )
                  != IDE_SUCCESS );

        *aFlag = QMC_ROW_DATA_NONE;
    }
    else
    {
        *aFlag = QMC_ROW_DATA_EXIST;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnSORT::searchNextNonHit( qcTemplate * /* aTemplate */,
                           qmncSORT   * /* aCodePlan */,
                           qmndSORT   * aDataPlan,
                           qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *     다음 Non-Hit 검색
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnSORT::searchNextNonHit"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    void * sOrgRow;
    void * sSearchRow;

    // 다음 Non-Hit 검색
    sOrgRow = sSearchRow = aDataPlan->plan.myTuple->row;
    IDE_TEST( qmcSortTemp::getNextNonHit( aDataPlan->sortMgr,
                                          & sSearchRow )
              != IDE_SUCCESS );
    aDataPlan->plan.myTuple->row = (sSearchRow == NULL) ? sOrgRow : sSearchRow;

    // Row 존재 유무 설정
    if ( sSearchRow == NULL )
    {
        IDE_TEST( qmcSortTemp::clearHitFlag( aDataPlan->sortMgr )
                  != IDE_SUCCESS );

        *aFlag = QMC_ROW_DATA_NONE;
    }
    else
    {
        *aFlag = QMC_ROW_DATA_EXIST;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}
