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
 * $Id: qmnHash.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *     HASH(HASH) Node
 *
 *     관계형 모델에서 hashing 연산을 수행하는 Plan Node 이다.
 *
 *     다음과 같은 기능을 위해 사용된다.
 *         - Hash Join
 *         - Hash-based Left Outer Join
 *         - Hash-based Full Outer Join
 *         - Store And Search
 *
 *     HASH 노드는 Two Pass Hash Join등에 사용될 때,
 *     여러개의 Temp Table을 관리할 수 있다.
 *     따라서, 모든 삽입 및 검색 시 이에 대한 고려가 충분하여야 한다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qcuProperty.h>
#include <qmoUtil.h>
#include <qmnHash.h>
#include <qcg.h>
#include <qmxResultCache.h>

IDE_RC
qmnHASH::init( qcTemplate * aTemplate,
               qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    HASH 노드의 초기화
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnHASH::init"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncHASH * sCodePlan = (qmncHASH *) aPlan;
    qmndHASH * sDataPlan =
        (qmndHASH *) (aTemplate->tmplate.data + aPlan->offset);
    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];

    UInt   i;
    idBool sDependency;

    sDataPlan->doIt = qmnHASH::doItDefault;

    sDataPlan->searchFirst = qmnHASH::searchDefault;
    sDataPlan->searchNext = qmnHASH::searchDefault;

    //----------------------------------------
    // 최초 초기화 수행
    //----------------------------------------

    if ( (*sDataPlan->flag & QMND_HASH_INIT_DONE_MASK)
         == QMND_HASH_INIT_DONE_FALSE )
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

        for ( i = 0; i < sCodePlan->tempTableCnt; i++ )
        {
            IDE_TEST( qmcHashTemp::clear( & sDataPlan->hashMgr[i] )
                      != IDE_SUCCESS );
        }

        // To Fix PR-8024
        // Mask FALSE 설정 오류
        // STORE AND SEARCH를 위한 초기화
        sDataPlan->isNullStore = ID_FALSE;
        sDataPlan->mtrTotalCnt = 0;

        //----------------------------------------
        // Child를 반복 수행하여 Temp Table을 구축
        //----------------------------------------

        IDE_TEST( aPlan->left->init( aTemplate,
                                     aPlan->left ) != IDE_SUCCESS);

        IDE_TEST( storeAndHashing( aTemplate,
                                   sCodePlan,
                                   sDataPlan ) != IDE_SUCCESS);

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

    switch ( sCodePlan->flag & QMNC_HASH_SEARCH_MASK )
    {
        case QMNC_HASH_SEARCH_SEQUENTIAL:
            sDataPlan->doIt = qmnHASH::doItFirst;
            break;
        case QMNC_HASH_SEARCH_RANGE:
            sDataPlan->doIt = qmnHASH::doItFirst;
            break;
        case QMNC_HASH_SEARCH_STORE_SEARCH:
            sDataPlan->doIt = qmnHASH::doItFirstStoreSearch;

            // 적합성 검사
            IDE_DASSERT( (sCodePlan->flag & QMNC_HASH_STORE_MASK)
                         == QMNC_HASH_STORE_DISTINCT );
            break;
        default:
            IDE_DASSERT( 0 );
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC
qmnHASH::doIt( qcTemplate * aTemplate,
               qmnPlan    * aPlan,
               qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    HASH 의 고유 기능을 수행한다.
 *
 * Implementation :
 *    지정된 함수 포인터를 수행한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnHASH::doIt"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // qmncHASH * sCodePlan = (qmncHASH *) aPlan;
    qmndHASH * sDataPlan =
        (qmndHASH *) (aTemplate->tmplate.data + aPlan->offset);

    IDE_TEST( sDataPlan->doIt( aTemplate, aPlan, aFlag ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC
qmnHASH::padNull( qcTemplate * aTemplate,
                  qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    HASH 노드의 Tuple에 Null Row를 설정한다.
 *
 * Implementation :
 *    Child Plan의 Null Padding을 수행하고,
 *    자신의 Null Row를 Temp Table로부터 획득한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnHASH::padNull"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncHASH * sCodePlan = (qmncHASH *) aPlan;
    qmndHASH * sDataPlan =
        (qmndHASH *) (aTemplate->tmplate.data + aPlan->offset);

    // first initialization
    if ( (aTemplate->planFlag[sCodePlan->planID] & QMND_HASH_INIT_DONE_MASK)
         == QMND_HASH_INIT_DONE_FALSE )
    {
        IDE_TEST( aPlan->init( aTemplate, aPlan ) != IDE_SUCCESS );
    }

    IDE_TEST( aPlan->left->padNull( aTemplate, aPlan->left )
              != IDE_SUCCESS );

    IDE_TEST(
        qmcHashTemp::getNullRow( & sDataPlan->hashMgr[QMN_HASH_PRIMARY_ID],
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
qmnHASH::printPlan( qcTemplate   * aTemplate,
                    qmnPlan      * aPlan,
                    ULong          aDepth,
                    iduVarString * aString,
                    qmnDisplay     aMode )
{
/***********************************************************************
 *
 * Description :
 *    HASH 노드의 수행 정보를 출력한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnHASH::printPlan"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncHASH * sCodePlan = (qmncHASH*) aPlan;
    qmndHASH * sDataPlan =
        (qmndHASH*) (aTemplate->tmplate.data + aPlan->offset);
    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];
    qmndHASH * sCacheDataPlan = NULL;
    idBool     sIsInit        = ID_FALSE;
    SLong sRecordCnt;
    SLong sTotalRecordCnt;
    ULong sPageCnt;
    ULong sTotalPageCnt;
    UInt  sBucketCnt;

    ULong  i;

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

        if ( (*sDataPlan->flag & QMND_HASH_INIT_DONE_MASK)
             == QMND_HASH_INIT_DONE_TRUE )
        {
            // 수행 정보 획득

            sTotalRecordCnt = 0;
            sTotalPageCnt = 0;
            sBucketCnt = 0;
            sIsInit = ID_TRUE;
            for ( i = 0; i < sCodePlan->tempTableCnt; i++ )
            {
                IDE_TEST( qmcHashTemp::getDisplayInfo( & sDataPlan->hashMgr[i],
                                                       & sPageCnt,
                                                       & sRecordCnt,
                                                       & sBucketCnt )
                          != IDE_SUCCESS );
                sTotalRecordCnt += sRecordCnt;
                sTotalPageCnt += sPageCnt;
            }

            if ( (sCodePlan->plan.flag & QMN_PLAN_STORAGE_MASK)
                 == QMN_PLAN_STORAGE_MEMORY )
            {
                // To Fix PR-13136
                // Temp Table의 개수를 알 수 있어야 함.
                if ( sCodePlan->tempTableCnt == 1 )
                {
                    if ( QCU_DISPLAY_PLAN_FOR_NATC == 0 )
                    {
                        iduVarStringAppendFormat(
                            aString,
                            "HASH ( ITEM_SIZE: %"ID_UINT32_FMT", "
                            "ITEM_COUNT: %"ID_INT64_FMT", "
                            "BUCKET_COUNT: %"ID_UINT32_FMT", "
                            "ACCESS: %"ID_UINT32_FMT,
                            sDataPlan->mtrRowSize,
                            sTotalRecordCnt,
                            sBucketCnt,
                            sDataPlan->plan.myTuple->modify );
                    }
                    else
                    {
                        // BUG-29209
                        // ITEM_SIZE 정보 보여주지 않음
                        iduVarStringAppendFormat(
                            aString,
                            "HASH ( ITEM_SIZE: BLOCKED, "
                            "ITEM_COUNT: %"ID_INT64_FMT", "
                            "BUCKET_COUNT: %"ID_UINT32_FMT", "
                            "ACCESS: %"ID_UINT32_FMT,
                            sTotalRecordCnt,
                            sBucketCnt,
                            sDataPlan->plan.myTuple->modify );
                    }
                }
                else
                {
                    if ( QCU_DISPLAY_PLAN_FOR_NATC == 0 )
                    {
                        iduVarStringAppendFormat(
                            aString,
                            "HASH ( ITEM_SIZE: %"ID_UINT32_FMT", "
                            "ITEM_COUNT: %"ID_INT64_FMT", "
                            "BUCKET_COUNT: %"ID_UINT32_FMT
                            "[%"ID_UINT32_FMT"], "
                            "ACCESS: %"ID_UINT32_FMT,
                            sDataPlan->mtrRowSize,
                            sTotalRecordCnt,
                            sBucketCnt,
                            sCodePlan->tempTableCnt,
                            sDataPlan->plan.myTuple->modify );
                    }
                    else
                    {
                        // BUG-29209
                        // ITEM_SIZE 정보 보여주지 않음
                        iduVarStringAppendFormat(
                            aString,
                            "HASH ( ITEM_SIZE: BLOCKED, "
                            "ITEM_COUNT: %"ID_INT64_FMT", "
                            "BUCKET_COUNT: %"ID_UINT32_FMT
                            "[%"ID_UINT32_FMT"], "
                            "ACCESS: %"ID_UINT32_FMT,
                            sTotalRecordCnt,
                            sBucketCnt,
                            sCodePlan->tempTableCnt,
                            sDataPlan->plan.myTuple->modify );
                    }
                }
            }
            else
            {
                // To Fix PR-13136
                // Temp Table의 개수를 알 수 있어야 함.
                if ( sCodePlan->tempTableCnt == 1 )
                {
                    if ( QCU_DISPLAY_PLAN_FOR_NATC == 0 )
                    {
                        iduVarStringAppendFormat(
                            aString,
                            "HASH ( ITEM_SIZE: %"ID_UINT32_FMT", "
                            "ITEM_COUNT: %"ID_INT64_FMT", "
                            "DISK_PAGE_COUNT: %"ID_UINT64_FMT", "
                            "ACCESS: %"ID_UINT32_FMT,
                            sDataPlan->mtrRowSize,
                            sTotalRecordCnt,
                            sTotalPageCnt,
                            sDataPlan->plan.myTuple->modify );
                    }
                    else
                    {
                        // BUG-29209
                        // ITEM_SIZE, DISK_PAGE_COUNT 정보 보여주지 않음
                        iduVarStringAppendFormat(
                            aString,
                            "HASH ( ITEM_SIZE: BLOCKED, "
                            "ITEM_COUNT: %"ID_INT64_FMT", "
                            "DISK_PAGE_COUNT: BLOCKED, "
                            "ACCESS: %"ID_UINT32_FMT,
                            sTotalRecordCnt,
                            sDataPlan->plan.myTuple->modify );
                    }
                }
                else
                {
                    if ( QCU_DISPLAY_PLAN_FOR_NATC == 0 )
                    {
                        iduVarStringAppendFormat(
                            aString,
                            "HASH ( ITEM_SIZE: %"ID_INT32_FMT", "
                            "ITEM_COUNT: %"ID_INT64_FMT", "
                            "DISK_PAGE_COUNT: %"ID_UINT64_FMT
                            "[%"ID_UINT32_FMT"], "
                            "ACCESS: %"ID_UINT32_FMT,
                            sDataPlan->mtrRowSize,
                            sTotalRecordCnt,
                            sTotalPageCnt,
                            sCodePlan->tempTableCnt,
                            sDataPlan->plan.myTuple->modify );
                    }
                    else
                    {
                        // BUG-29209
                        // ITEM_SIZE, DISK_PAGE_COUNT 정보 보여주지 않음
                        iduVarStringAppendFormat(
                            aString,
                            "HASH ( ITEM_SIZE: BLOCKED, "
                            "ITEM_COUNT: %"ID_INT64_FMT", "
                            "DISK_PAGE_COUNT: BLOCKED, "
                            "ACCESS: %"ID_UINT32_FMT,
                            sTotalRecordCnt,
                            sDataPlan->plan.myTuple->modify );
                    }
                }
            }
        }
        else
        {
            // To Fix PR-13136
            // Temp Table의 개수를 알 수 있어야 함.
            if ( sCodePlan->tempTableCnt == 1 )
            {
                iduVarStringAppendFormat( aString,
                                          "HASH ( ITEM_SIZE: 0, ITEM_COUNT: 0, "
                                          "BUCKET_COUNT: %"ID_UINT32_FMT", "
                                          "ACCESS: 0",
                                          sCodePlan->bucketCnt );
            }
            else
            {
                iduVarStringAppendFormat( aString,
                                          "HASH ( ITEM_SIZE: 0, ITEM_COUNT: 0, "
                                          "BUCKET_COUNT: %"ID_UINT32_FMT
                                          "[%"ID_UINT32_FMT"], "
                                          "ACCESS: 0",
                                          sCodePlan->bucketCnt,
                                          sCodePlan->tempTableCnt );
            }
        }
    }
    else
    {
        //----------------------------
        // explain plan = only; 인 경우
        //----------------------------

        if ( sCodePlan->tempTableCnt == 1 )
        {
            iduVarStringAppendFormat( aString,
                                      "HASH ( ITEM_SIZE: ??, ITEM_COUNT: ??, "
                                      "BUCKET_COUNT: %"ID_UINT32_FMT", "
                                      "ACCESS: ??",
                                      sCodePlan->bucketCnt );
        }
        else
        {
            iduVarStringAppendFormat( aString,
                                      "HASH ( ITEM_SIZE: ??, ITEM_COUNT: ??, "
                                      "BUCKET_COUNT: %"ID_UINT32_FMT
                                      "[%"ID_UINT32_FMT"], "
                                      "ACCESS: ??",
                                      sCodePlan->bucketCnt,
                                      sCodePlan->tempTableCnt );
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
        // PROJ-2462 ResultCache
        if ( ( sCodePlan->componentInfo != NULL ) &&
             ( ( aTemplate->resultCache.flag & QC_RESULT_CACHE_MAX_EXCEED_MASK )
               == QC_RESULT_CACHE_MAX_EXCEED_FALSE ) &&
             ( ( aTemplate->resultCache.flag & QC_RESULT_CACHE_DATA_ALLOC_MASK )
               == QC_RESULT_CACHE_DATA_ALLOC_TRUE ) )
        {
            sCacheDataPlan = (qmndHASH *) (aTemplate->resultCache.data + sCodePlan->plan.offset);
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
        if (sCodePlan->filter != NULL)
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
                                              sCodePlan->filter )
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
qmnHASH::doItDefault( qcTemplate * /* aTemplate */,
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

#define IDE_FN "qmnHASH::doItDefault"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_DASSERT( 0 );

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnHASH::doItFirst( qcTemplate * aTemplate,
                    qmnPlan    * aPlan,
                    qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    첫번째 수행 함수
 *
 * Implementation :
 *    해당 검색 함수를 수행하고, 해당 결과를 Tuple Set에 복원한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnHASH::doItFirst"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncHASH * sCodePlan = (qmncHASH *) aPlan;
    qmndHASH * sDataPlan =
        (qmndHASH *) (aTemplate->tmplate.data + aPlan->offset);

    IDE_TEST( iduCheckSessionEvent( aTemplate->stmt->mStatistics )
              != IDE_SUCCESS );

    IDE_TEST( sDataPlan->searchFirst( aTemplate,
                                      sCodePlan,
                                      sDataPlan,
                                      aFlag ) != IDE_SUCCESS );

    if ( (*aFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
    {
        IDE_TEST( setTupleSet( aTemplate,
                               sDataPlan ) != IDE_SUCCESS );

        sDataPlan->doIt = qmnHASH::doItNext;
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
qmnHASH::doItNext( qcTemplate * aTemplate,
                   qmnPlan    * aPlan,
                   qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    다음 검색 함수
 *
 * Implementation :
 *    해당 검색 함수를 수행하고, 해당 결과를 Tuple Set에 복원한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnHASH::doItNext"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncHASH * sCodePlan = (qmncHASH *) aPlan;
    qmndHASH * sDataPlan =
        (qmndHASH *) (aTemplate->tmplate.data + aPlan->offset);

    IDE_TEST( sDataPlan->searchNext( aTemplate,
                                     sCodePlan,
                                     sDataPlan,
                                     aFlag ) != IDE_SUCCESS );

    if ( (*aFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
    {
        IDE_TEST( setTupleSet( aTemplate,
                               sDataPlan ) != IDE_SUCCESS );
    }
    else
    {
        sDataPlan->doIt = qmnHASH::doItFirst;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnHASH::doItFirstStoreSearch( qcTemplate * aTemplate,
                               qmnPlan    * aPlan,
                               qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *     Store And Search의 첫번째 수행 함수
 *
 * Implementation :
 *     0. 조건 검색을 통해 원하는 Row를 검색한다.
 *         - Row의 존재 유무, 상수값의 NULL 여부가 판단된다.
 *     1. Row가 존재한다면 그대로 리턴
 *     2. Row가 존재하지 않는다면 다음과 같은 상황에 따라 대처한다.
 *         - 2.1 저장 Row가 없다면, 데이터 없음을 리턴
 *         - 2.2 저장 Row가 있다면,
 *             - 2.2.1 상수가 NULL이라면, NULL Padding한 후 데이터 있음
 *             - 2.2.2 저장 Row에 NULL이 존재한다면,
 *                     NULL Padding한 후 데이터 있음
 *             - 2.2.3 그 외의 경우, 데이터 없음
 *
 *     이와 같은 처리는 다음과 같은 사항에 대한 처리를 위함이다.
 *
 *     Ex     1)   3 IN (1, 2, 3)      => TRUE
 *     Ex   2.1)   3 IN {}, NULL IN {} => FALSE
 *     Ex 2.2.1)   NULL IN {1,2}       => UNKNOWN
 *     Ex 2.2.2)   3 IN {1,2,NULL}     => UNKNOWN
 *     Ex 2.2.3)   3 IN {1,2}          => FALSE
 *
 ***********************************************************************/

#define IDE_FN "qmnHASH::doItFirstStoreSearch"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncHASH * sCodePlan = (qmncHASH *) aPlan;
    qmndHASH * sDataPlan =
        (qmndHASH *) (aTemplate->tmplate.data + aPlan->offset);

    IDE_TEST( iduCheckSessionEvent( aTemplate->stmt->mStatistics )
              != IDE_SUCCESS );

    // 0. 조건 검색
    IDE_TEST( sDataPlan->searchFirst( aTemplate,
                                      sCodePlan,
                                      sDataPlan,
                                      aFlag ) != IDE_SUCCESS );
    if ( (*aFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
    {
        // 1. Row가 존재하는 경우

        IDE_TEST( setTupleSet( aTemplate, sDataPlan )
                  != IDE_SUCCESS );

        sDataPlan->doIt = qmnHASH::doItNextStoreSearch;
    }
    else
    {
        if ( sDataPlan->mtrTotalCnt == 0 )
        {
            // 2.1 저장 Data가 없는 경우
            // Nothing To Do
        }
        else
        {
            // 2.2 저장 Data가 있는 경우

            if ( ( (*sDataPlan->flag & QMND_HASH_CONST_NULL_MASK)
                   == QMND_HASH_CONST_NULL_TRUE )
                 ||
                 ( sDataPlan->isNullStore == ID_TRUE ) )
            {
                // 2.2.1, 2.2.2 : 상수가 NULL이거나 저장 Row에 NULL이 존재

                IDE_TEST( qmnHASH::padNull( aTemplate, aPlan )
                          != IDE_SUCCESS );

                *aFlag &= ~QMC_ROW_DATA_MASK;
                *aFlag |= QMC_ROW_DATA_EXIST;

                sDataPlan->doIt = qmnHASH::doItNextStoreSearch;
            }
            else
            {
                // 2.2.3
                // Nothing To Do
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnHASH::doItNextStoreSearch( qcTemplate * aTemplate,
                              qmnPlan    * aPlan,
                              qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *     Store And Search를 위한 다음 검색
 *
 * Implementation :
 *     데이터 없음을 리턴
 *     Store And Search의 경우 한 번의 검색으로 그 결과를 결정할 수
 *     있으며, 두 번째 수행에서 그 결과를 확정할 수 있다.
 *
 ***********************************************************************/

#define IDE_FN "qmnHASH::doItNextStoreSearch"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // qmncHASH * sCodePlan = (qmncHASH *) aPlan;
    qmndHASH * sDataPlan =
        (qmndHASH *) (aTemplate->tmplate.data + aPlan->offset);

    // set that no data found
    *aFlag = QMC_ROW_DATA_NONE;

    sDataPlan->doIt = qmnHASH::doItFirstStoreSearch;

    return IDE_SUCCESS;

#undef IDE_FN
}

void qmnHASH::setHitSearch( qcTemplate * aTemplate,
                            qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    Inverse Hash Join에서 사용되며,
 *    Probing Left Table에 대한 처리가 끝나고, Driving Right Table에 대한
 *    처리를 수행하기 위해 사용된다.
 *
 * Implementation :
 *    검색 함수를 강제적으로 Hit 검색으로 전환한다.
 *
 ***********************************************************************/
    qmndHASH * sDataPlan =
        (qmndHASH *) (aTemplate->tmplate.data + aPlan->offset);

    sDataPlan->searchFirst = qmnHASH::searchFirstHit;
    sDataPlan->searchNext = qmnHASH::searchNextHit;
    sDataPlan->doIt = qmnHASH::doItFirst;
}

void qmnHASH::setNonHitSearch( qcTemplate * aTemplate,
                               qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    Full Outer Join에서 사용되며,
 *    Left Outer Join에 대한 처리가 끝나고, Right Outer Join에 대한
 *    처리를 수행하기 위해 사용된다.
 *
 * Implementation :
 *    검색 함수를 강제적으로 Non-Hit 검색으로 전환한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnHASH::setNonHitSearch"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // qmncHASH * sCodePlan = (qmncHASH *) aPlan;
    qmndHASH * sDataPlan =
        (qmndHASH *) (aTemplate->tmplate.data + aPlan->offset);

    sDataPlan->searchFirst = qmnHASH::searchFirstNonHit;
    sDataPlan->searchNext = qmnHASH::searchNextNonHit;
    sDataPlan->doIt = qmnHASH::doItFirst;

#undef IDE_FN
}

IDE_RC
qmnHASH::setHitFlag( qcTemplate * aTemplate,
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
 *
 *     Temp Table을 이용하여 현재 Row의 Hit Flag을 셋팅한다.
 *     이미 Temp Table은 현재 Row의 존재를 알고 있어 별도의 인자가
 *     필요 없다.
 *
 ***********************************************************************/

#define IDE_FN "qmnHASH::setHitFlag"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // qmncHASH * sCodePlan = (qmncHASH *) aPlan;
    qmndHASH * sDataPlan =
        (qmndHASH *) (aTemplate->tmplate.data + aPlan->offset);

    IDE_TEST( qmcHashTemp::setHitFlag( & sDataPlan->
                                       hashMgr[sDataPlan->currTempID] )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

idBool qmnHASH::isHitFlagged( qcTemplate * aTemplate,
                              qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description : PROJ-2339
 *     현재 Row에 Hit Flag가 있는지 검증한다.
 *
 * Implementation :
 *
 *     Temp Table을 이용하여 현재 Row에 Hit Flag가 있는지 검증한다.
 *
 ***********************************************************************/

    qmndHASH * sDataPlan =
        (qmndHASH *) (aTemplate->tmplate.data + aPlan->offset);

    return qmcHashTemp::isHitFlagged( & sDataPlan->
                                      hashMgr[sDataPlan->currTempID] );
}

IDE_RC
qmnHASH::firstInit( qcTemplate * aTemplate,
                    qmncHASH   * aCodePlan,
                    qmndHASH   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    HASH node의 Data 영역의 멤버에 대한 초기화를 수행
 *
 * Implementation :
 *
 ***********************************************************************/
    qmndHASH * sCacheDataPlan = NULL;

    /* PROJ-2462 Result Cache */
    if ( ( *aDataPlan->flag & QMN_PLAN_RESULT_CACHE_EXIST_MASK )
         == QMN_PLAN_RESULT_CACHE_EXIST_TRUE )
    {
        sCacheDataPlan = (qmndHASH *) (aTemplate->resultCache.data + aCodePlan->plan.offset);
        sCacheDataPlan->resultData.flag = &aTemplate->resultCache.dataFlag[aCodePlan->planID];
        aDataPlan->resultData.flag      = &aTemplate->resultCache.dataFlag[aCodePlan->planID];

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

    //---------------------------------
    // 적합성 검사
    //---------------------------------

    if ( (aCodePlan->flag & QMNC_HASH_NULL_CHECK_MASK)
         == QMNC_HASH_NULL_CHECK_TRUE )
    {
        IDE_DASSERT( (aCodePlan->flag & QMNC_HASH_SEARCH_MASK)
                     == QMNC_HASH_SEARCH_STORE_SEARCH );
        IDE_DASSERT( (aCodePlan->flag & QMNC_HASH_STORE_MASK)
                     == QMNC_HASH_STORE_DISTINCT );
    }

    //---------------------------------
    // HASH 고유 정보의 초기화
    //---------------------------------

    IDE_TEST( initMtrNode( aTemplate, aCodePlan, aDataPlan ) != IDE_SUCCESS );

    IDE_TEST( initHashNode( aCodePlan, aDataPlan ) != IDE_SUCCESS );

    aDataPlan->mtrRowSize = qmc::getMtrRowSize( aDataPlan->mtrNode );

    aDataPlan->plan.myTuple = aDataPlan->mtrNode->dstTuple;

    aDataPlan->depTuple =
        & aTemplate->tmplate.rows[aCodePlan->depTupleRowID];
    aDataPlan->depValue = QMN_PLAN_DEFAULT_DEPENDENCY_VALUE;

    aDataPlan->mtrTotalCnt = 0;
    //---------------------------------
    // Temp Table의 초기화
    //---------------------------------

    IDE_TEST( initTempTable( aTemplate, aCodePlan, aDataPlan )
              != IDE_SUCCESS );


    //---------------------------------
    // 초기화 완료를 표기
    //---------------------------------

    *aDataPlan->flag &= ~QMND_HASH_INIT_DONE_MASK;
    *aDataPlan->flag |= QMND_HASH_INIT_DONE_TRUE;

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

#undef IDE_FN
}

IDE_RC
qmnHASH::initMtrNode( qcTemplate * aTemplate,
                      qmncHASH   * aCodePlan,
                      qmndHASH   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    저장 Column의 관리를 위한 노드를 초기화한다.
 *
 * Implementation :
 *
 ***********************************************************************/
    UInt        sHeaderSize = 0;

    //---------------------------------
    // 적합성 검사
    //---------------------------------

    IDE_DASSERT( aCodePlan->mtrNodeOffset > 0 );

    //---------------------------------
    // 저장 관리를 위한 정보의 초기화
    //---------------------------------

    aDataPlan->mtrNode =
        (qmdMtrNode*) (aTemplate->tmplate.data + aCodePlan->mtrNodeOffset);

    if ( (aCodePlan->plan.flag & QMN_PLAN_STORAGE_MASK)
         == QMN_PLAN_STORAGE_MEMORY )
    {
        sHeaderSize = QMC_MEMHASH_TEMPHEADER_SIZE;

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
        sHeaderSize = QMC_DISKHASH_TEMPHEADER_SIZE;
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
                                aCodePlan->baseTableCount ) != IDE_SUCCESS );

    IDE_TEST( qmc::refineOffsets( aDataPlan->mtrNode,
                                  sHeaderSize ) != IDE_SUCCESS );

    IDE_TEST( qmc::setRowSize( aTemplate->stmt->qmxMem,
                               & aTemplate->tmplate,
                               aDataPlan->mtrNode->dstNode->node.table )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnHASH::initHashNode( qmncHASH    * aCodePlan,
                       qmndHASH    * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *     Hashing Column의 시작 위치를 찾음
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnHASH::initHashNode"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmdMtrNode * sNode;

    for ( sNode = aDataPlan->mtrNode; sNode != NULL; sNode = sNode->next )
    {
        if ( ( sNode->myNode->flag & QMC_MTR_HASH_NEED_MASK )
             == QMC_MTR_HASH_NEED_TRUE )
        {
            break;
        }
    }

    aDataPlan->hashNode = sNode;

    //---------------------------------
    // 적합성 검사
    //---------------------------------

    IDE_DASSERT( aDataPlan->hashNode != NULL );

    if ( (aCodePlan->flag & QMNC_HASH_NULL_CHECK_MASK)
         == QMNC_HASH_NULL_CHECK_TRUE )
    {
        // STORE AND SEARCH의 경우 NULL CHECK가 필요한 경우는
        // 한 컬럼에 대한 Hashing일 경우만 가능하다.
        IDE_DASSERT( aDataPlan->hashNode->next == NULL );
    }
    else
    {
        // Nothing To Do
    }

    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC
qmnHASH::initTempTable( qcTemplate * aTemplate,
                        qmncHASH   * aCodePlan,
                        qmndHASH   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *     Hash Temp Table을 초기화한다.
 *
 * Implementation :
 *     Hash Temp Table의 개수만큼 Hash Temp Table을 초기화한다.
 *
 ***********************************************************************/
    UInt        i;
    UInt        sFlag;
    qmndHASH  * sCacheDataPlan = NULL;

    //-----------------------------
    // 적합성 검사
    //-----------------------------

    IDE_DASSERT( aCodePlan->tempTableCnt > 0 );
    IDE_DASSERT( QMN_HASH_PRIMARY_ID == 0 );

    //-----------------------------
    // Flag 정보 초기화
    //-----------------------------

    sFlag = QMCD_HASH_TMP_INITIALIZE;

    switch ( aCodePlan->flag & QMNC_HASH_STORE_MASK )
    {
        case QMNC_HASH_STORE_NONE:
            IDE_DASSERT(0);
            break;
        case QMNC_HASH_STORE_HASHING:
            sFlag &= ~QMCD_HASH_TMP_DISTINCT_MASK;
            sFlag |= QMCD_HASH_TMP_DISTINCT_FALSE;
            break;
        case QMNC_HASH_STORE_DISTINCT:
            sFlag &= ~QMCD_HASH_TMP_DISTINCT_MASK;
            sFlag |= QMCD_HASH_TMP_DISTINCT_TRUE;
            break;
        default:
            IDE_DASSERT(0);
            break;
    }

    if ( (aCodePlan->plan.flag & QMN_PLAN_STORAGE_MASK)
         == QMN_PLAN_STORAGE_MEMORY )
    {
        sFlag &= ~QMCD_HASH_TMP_STORAGE_TYPE;
        sFlag |= QMCD_HASH_TMP_STORAGE_MEMORY;

        IDE_DASSERT( (aDataPlan->plan.myTuple->lflag & MTC_TUPLE_STORAGE_MASK)
                     == MTC_TUPLE_STORAGE_MEMORY );
    }
    else
    {
        sFlag &= ~QMCD_HASH_TMP_STORAGE_TYPE;
        sFlag |= QMCD_HASH_TMP_STORAGE_DISK;

        IDE_DASSERT( (aDataPlan->plan.myTuple->lflag & MTC_TUPLE_STORAGE_MASK)
                     == MTC_TUPLE_STORAGE_DISK );
    }

    // PROJ-2553
    // Bucket-based Hash Temp Table을 사용하지 않고 Non-DISTINCT Hashing인 경우,
    // Partitioned Hashing을 하도록 한다.
    if ( ( aTemplate->memHashTempPartDisable == ID_FALSE ) &&
         ( ( sFlag & QMCD_HASH_TMP_DISTINCT_MASK ) == QMCD_HASH_TMP_DISTINCT_FALSE ) )
    {
        sFlag &= ~QMCD_HASH_TMP_HASHING_TYPE;
        sFlag |= QMCD_HASH_TMP_HASHING_PARTITIONED;
    }
    else
    {
        sFlag &= ~QMCD_HASH_TMP_HASHING_TYPE;
        sFlag |= QMCD_HASH_TMP_HASHING_BUCKET;
    }

    //-----------------------------
    // Temp Table 초기화
    //-----------------------------
    if ( ( *aDataPlan->flag & QMN_PLAN_RESULT_CACHE_EXIST_MASK )
         == QMN_PLAN_RESULT_CACHE_EXIST_FALSE )
    {
        IDU_FIT_POINT( "qmnHASH::initTempTable::qmxAlloc:hashMgr",
                       idERR_ABORT_InsufficientMemory );
        IDE_TEST( aTemplate->stmt->qmxMem->alloc( ID_SIZEOF(qmcdHashTemp) *
                                                  aCodePlan->tempTableCnt,
                                                  (void**) & aDataPlan->hashMgr )
                  != IDE_SUCCESS );
        IDE_TEST_RAISE( aDataPlan->hashMgr == NULL, err_mem_alloc );

        // Primary Temp Table의 초기화

        sFlag &= ~QMCD_HASH_TMP_PRIMARY_MASK;
        sFlag |= QMCD_HASH_TMP_PRIMARY_TRUE;

        IDE_TEST( qmcHashTemp::init( & aDataPlan->hashMgr[QMN_HASH_PRIMARY_ID],
                                     aTemplate,
                                     ID_UINT_MAX,
                                     aDataPlan->mtrNode,
                                     aDataPlan->hashNode,
                                     NULL,  // Aggregation Column없음
                                     aCodePlan->bucketCnt,
                                     sFlag )
                  != IDE_SUCCESS );

        // Secondary Temp Table의 초기화

        sFlag &= ~QMCD_HASH_TMP_PRIMARY_MASK;
        sFlag |= QMCD_HASH_TMP_PRIMARY_FALSE;

        for ( i = 1; i < aCodePlan->tempTableCnt; i++ )
        {
            IDE_TEST( qmcHashTemp::init( & aDataPlan->hashMgr[i],
                                         aTemplate,
                                         ID_UINT_MAX,
                                         aDataPlan->mtrNode,
                                         aDataPlan->hashNode,
                                         NULL,  // Aggregation Column없음
                                         aCodePlan->bucketCnt,
                                         sFlag )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        /* PROJ-2462 Result Cache */
        sCacheDataPlan = (qmndHASH *) (aTemplate->resultCache.data +
                                      aCodePlan->plan.offset);

        if ( ( *aDataPlan->resultData.flag & QMX_RESULT_CACHE_INIT_DONE_MASK )
             == QMX_RESULT_CACHE_INIT_DONE_FALSE )
        {
            IDU_FIT_POINT( "qmnHASH::initTempTable::qrcAlloc:hashMgr",
                           idERR_ABORT_InsufficientMemory );
            IDE_TEST( sCacheDataPlan->resultData.memory->alloc( ID_SIZEOF(qmcdHashTemp) *
                                                               aCodePlan->tempTableCnt,
                                                               (void**) & aDataPlan->hashMgr )
                      != IDE_SUCCESS );
            IDE_TEST_RAISE( aDataPlan->hashMgr == NULL, err_mem_alloc );

            // Primary Temp Table의 초기화
            sFlag &= ~QMCD_HASH_TMP_PRIMARY_MASK;
            sFlag |= QMCD_HASH_TMP_PRIMARY_TRUE;

            IDE_TEST( qmcHashTemp::init( & aDataPlan->hashMgr[QMN_HASH_PRIMARY_ID],
                                         aTemplate,
                                         sCacheDataPlan->resultData.memoryIdx,
                                         aDataPlan->mtrNode,
                                         aDataPlan->hashNode,
                                         NULL,  // Aggregation Column없음
                                         aCodePlan->bucketCnt,
                                         sFlag )
                      != IDE_SUCCESS );

            // Secondary Temp Table의 초기화
            sFlag &= ~QMCD_HASH_TMP_PRIMARY_MASK;
            sFlag |= QMCD_HASH_TMP_PRIMARY_FALSE;

            for ( i = 1; i < aCodePlan->tempTableCnt; i++ )
            {
                IDE_TEST( qmcHashTemp::init( & aDataPlan->hashMgr[i],
                                             aTemplate,
                                             sCacheDataPlan->resultData.memoryIdx,
                                             aDataPlan->mtrNode,
                                             aDataPlan->hashNode,
                                             NULL,  // Aggregation Column없음
                                             aCodePlan->bucketCnt,
                                             sFlag )
                          != IDE_SUCCESS );
            }
            sCacheDataPlan->hashMgr = aDataPlan->hashMgr;
        }
        else
        {
            aDataPlan->hashMgr = sCacheDataPlan->hashMgr;
            aDataPlan->mtrTotalCnt = sCacheDataPlan->mtrTotalCnt;
            aDataPlan->isNullStore = sCacheDataPlan->isNullStore;
            for ( i = 0; i < aCodePlan->tempTableCnt; i++ )
            {
                IDE_TEST( qmcHashTemp::clearHitFlag( & aDataPlan->hashMgr[i] )
                          != IDE_SUCCESS );
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_mem_alloc );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_MEMORY_ALLOCATION));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnHASH::checkDependency( qcTemplate * aTemplate,
                                 qmncHASH   * aCodePlan,
                                 qmndHASH   * aDataPlan,
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
qmnHASH::storeAndHashing( qcTemplate * aTemplate,
                          qmncHASH   * aCodePlan,
                          qmndHASH   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    Child를 반복적으로 수행하여 Hash Temp Table을 구축한다.
 *
 * Implementation :
 *    다음과 같은 사항을 고려하여 구성하여야 한다.
 *        - 삽입할 Temp Table을 결정한다.
 *        - Distinct Option일 경우 삽입 성공 여부를 판단하여
 *          Memory 할당 여부를 결정하여야 한다.
 *        - NULL 여부를 판단하여야 하는 경우 NULL여부를 결정하여야 한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnHASH::storeAndHashing"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmcRowFlag    sFlag = QMC_ROW_INITIALIZE;
    UInt          sHashKey;
    idBool        sInserted;
    UInt          sTempID;
    qmndHASH    * sCacheDataPlan = NULL;

    sInserted = ID_TRUE;

    //------------------------------
    // Child Record의 저장
    //------------------------------

    IDE_TEST( aCodePlan->plan.left->doIt( aTemplate,
                                          aCodePlan->plan.left,
                                          & sFlag ) != IDE_SUCCESS );

    while ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
    {
        //---------------------------------------
        // 1.  저장 Row를 위한 공간 할당
        // 2.  저장 Row의 구성
        // 3.  NULL 여부의 판단
        // 4.  Temp Table 선택
        // 5.  저장 Row의 삽입
        // 6.  Child 수행
        //---------------------------------------

        // 1. 저장 Row를 위한 공간 할당
        if ( sInserted == ID_TRUE )
        {
            IDE_TEST(
                qmcHashTemp::alloc( & aDataPlan->hashMgr[QMN_HASH_PRIMARY_ID],
                                    & aDataPlan->plan.myTuple->row )
                != IDE_SUCCESS );
        }
        else
        {
            // 삽입이 실패한 경우로 이미 할당받은 공간을 사용한다.
            // 따라서, 별도로 공간을 할당 받을 필요가 없다.
        }

        // 2.  저장 Row의 구성
        IDE_TEST( setMtrRow( aTemplate, aDataPlan )
                  != IDE_SUCCESS );

        // 3.  NULL 여부의 판단
        IDE_TEST( checkNullExist( aCodePlan, aDataPlan )
                  != IDE_SUCCESS );

        // 4.  Temp Table 선택
        if ( aCodePlan->tempTableCnt > 1 )
        {
            IDE_TEST( getMtrHashKey( aDataPlan,
                                     & sHashKey )
                      != IDE_SUCCESS );
            sTempID = sHashKey % aCodePlan->tempTableCnt;
        }
        else
        {
            sTempID = QMN_HASH_PRIMARY_ID;
        }

        // 5.  저장 Row의 삽입
        if ( (aCodePlan->flag & QMNC_HASH_STORE_MASK)
             == QMNC_HASH_STORE_DISTINCT )
        {
            IDE_TEST( qmcHashTemp::addDistRow( & aDataPlan->hashMgr[sTempID],
                                               & aDataPlan->plan.myTuple->row,
                                               & sInserted )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( qmcHashTemp::addRow( & aDataPlan->hashMgr[sTempID],
                                           aDataPlan->plan.myTuple->row )
                      != IDE_SUCCESS );
            sInserted = ID_TRUE;
        }

        if( sInserted == ID_TRUE )
        {
            aDataPlan->mtrTotalCnt++;
        }
        else
        {
            // Nothing To Do
        }

        // 6.  Child 수행
        IDE_TEST( aCodePlan->plan.left->doIt( aTemplate,
                                              aCodePlan->plan.left,
                                              & sFlag ) != IDE_SUCCESS );
    }

    /* PROJ-2462 Result Cache */
    if ( ( *aDataPlan->flag & QMN_PLAN_RESULT_CACHE_EXIST_MASK )
         == QMN_PLAN_RESULT_CACHE_EXIST_TRUE )
    {
        *aDataPlan->resultData.flag &= ~QMX_RESULT_CACHE_STORED_MASK;
        *aDataPlan->resultData.flag |= QMX_RESULT_CACHE_STORED_TRUE;

        sCacheDataPlan = (qmndHASH *) (aTemplate->resultCache.data + aCodePlan->plan.offset);
        sCacheDataPlan->mtrTotalCnt = aDataPlan->mtrTotalCnt;
        sCacheDataPlan->isNullStore = aDataPlan->isNullStore;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnHASH::setMtrRow( qcTemplate * aTemplate,
                    qmndHASH   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *     저장 Row를 구성한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnHASH::setMtrRow"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmdMtrNode * sNode;

    for ( sNode = aDataPlan->mtrNode;
          sNode != NULL;
          sNode = sNode->next )
    {
        IDE_TEST( sNode->func.setMtr( aTemplate,
                                      sNode,
                                      aDataPlan->plan.myTuple->row )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnHASH::checkNullExist( qmncHASH   * aCodePlan,
                         qmndHASH   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *     저장 Row의 Hashing 대상 Column이 NULL인지를 판단한다.
 *     One Column에 대한 Store And Search시 사용된다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnHASH::checkNullExist"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    idBool sIsNull;

    if ( (aCodePlan->flag & QMNC_HASH_NULL_CHECK_MASK)
         == QMNC_HASH_NULL_CHECK_TRUE )
    {
        // 하나의 Column만이 Hashing 대상임이 보장된다.
        // 이는 이미 적합성 검사를 통해 보장됨
        sIsNull =
            aDataPlan->hashNode->func.isNull( aDataPlan->hashNode,
                                              aDataPlan->plan.myTuple->row );
        if ( sIsNull == ID_TRUE )
        {
            aDataPlan->isNullStore = ID_TRUE;
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

#undef IDE_FN
}

IDE_RC
qmnHASH::getMtrHashKey( qmndHASH   * aDataPlan,
                        UInt       * aHashKey )
{
/***********************************************************************
 *
 * Description :
 *    저장 Row의 Hash Key값 획득
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnHASH::getMtrHashKey"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UInt sValue = mtc::hashInitialValue;
    qmdMtrNode * sNode;

    for ( sNode = aDataPlan->hashNode; sNode != NULL; sNode = sNode->next )
    {
        sValue = sNode->func.getHash( sValue,
                                      sNode,
                                      aDataPlan->plan.myTuple->row );
    }

    *aHashKey = sValue;

    return IDE_SUCCESS;

#undef IDE_FN
}


IDE_RC
qmnHASH::getConstHashKey( qcTemplate * aTemplate,
                          qmncHASH   * aCodePlan,
                          qmndHASH   * aDataPlan,
                          UInt       * aHashKey )
{
/***********************************************************************
 *
 * Description :
 *    상수 영역의 Hash Key값 획득
 *
 * Implementation :
 *    상수 영역을 수행하고, Stack에 쌓인 정보를 이용하여
 *    Hash Key값을 얻는다.
 *    부가적으로 Store And Search를 위해 상수 영역의 값이
 *    NULL인지에 대한 판단도 수행한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnHASH::getConstHashKey"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    idBool     sIsNull;
    qtcNode  * sNode;
    mtcStack * sStack;

    UInt sValue = mtc::hashInitialValue;

    sStack = aTemplate->tmplate.stack;
    for ( sNode = aCodePlan->filterConst;
          sNode != NULL;
          sNode = (qtcNode*) sNode->node.next )
    {
        IDE_TEST( qtc::calculate( sNode, aTemplate )
                  != IDE_SUCCESS );
        sValue = sStack->column->module->hash( sValue,
                                               sStack->column,
                                               sStack->value );

        // NULL 값 여부의 판단
        sIsNull = sStack->column->module->isNull( sStack->column,
                                                  sStack->value );
        if ( sIsNull == ID_TRUE )
        {
            *aDataPlan->flag &= ~QMND_HASH_CONST_NULL_MASK;
            *aDataPlan->flag |= QMND_HASH_CONST_NULL_TRUE;
        }
        else
        {
            *aDataPlan->flag &= ~QMND_HASH_CONST_NULL_MASK;
            *aDataPlan->flag |= QMND_HASH_CONST_NULL_FALSE;
        }
    }

    *aHashKey = sValue;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnHASH::setTupleSet( qcTemplate * aTemplate,
                      qmndHASH   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *     검색된 저장 Row를 이용하여 Tuple Set을 복원한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnHASH::setTupleSet"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmdMtrNode * sNode;

    for ( sNode = aDataPlan->mtrNode;
          sNode != NULL;
          sNode = sNode->next )
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
qmnHASH::setSearchFunction( qmncHASH   * aCodePlan,
                            qmndHASH   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *     검색 옵션에 따라 검색 함수를 선택한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnHASH::setSearchFunction"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    switch ( aCodePlan->flag & QMNC_HASH_SEARCH_MASK )
    {
        case QMNC_HASH_SEARCH_SEQUENTIAL:
            aDataPlan->searchFirst = qmnHASH::searchFirstSequence;
            aDataPlan->searchNext = qmnHASH::searchNextSequence;

            IDE_DASSERT( aCodePlan->filter == NULL );
            break;
        case QMNC_HASH_SEARCH_RANGE:
            aDataPlan->searchFirst = qmnHASH::searchFirstFilter;
            aDataPlan->searchNext = qmnHASH::searchNextFilter;

            IDE_DASSERT( aCodePlan->filter != NULL );
            IDE_DASSERT( aCodePlan->filterConst != NULL );
            break;
        case QMNC_HASH_SEARCH_STORE_SEARCH:
            aDataPlan->searchFirst = qmnHASH::searchFirstFilter;
            aDataPlan->searchNext = qmnHASH::searchNextFilter;

            IDE_DASSERT( aCodePlan->filter != NULL );
            IDE_DASSERT( aCodePlan->filterConst != NULL );
            break;
        default:
            IDE_DASSERT(0);
            break;
    }

    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC
qmnHASH::searchDefault( qcTemplate * /* aTemplate */,
                        qmncHASH   * /* aCodePlan */,
                        qmndHASH   * /* aDataPlan */,
                        qmcRowFlag * /* aFlag */ )
{
/***********************************************************************
 *
 * Description :
 *     이 함수가 호출되어서는 안됨
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnHASH::searchDefault"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_DASSERT( 0 );

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnHASH::searchFirstSequence( qcTemplate * /* aTemplate */,
                              qmncHASH   * aCodePlan,
                              qmndHASH   * aDataPlan,
                              qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    첫번째 순차 검색
 *
 * Implementation :
 *    Temp Table이 여러 개인 경우를 고려하여 수행하며,
 *    Row Pointer를 잃지 않도록 하여야 한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnHASH::searchFirstSequence"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    void * sOrgRow;
    void * sSearchRow;
    qmcRowFlag sFlag;

    sFlag = QMC_ROW_INITIALIZE;
    aDataPlan->currTempID = 0;

    // To Fix PR-8142
    // Data 없음으로 설정하여야 반복 수행 및 올바른 종료가
    // 가능하다.
    sFlag &= ~QMC_ROW_DATA_MASK;
    sFlag |= QMC_ROW_DATA_NONE;


    // Data가 없고, 더 이상의 Temp Table이 없을 때까지 진행
    while ( (aDataPlan->currTempID < aCodePlan->tempTableCnt) &&
            ((sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_NONE) )
    {
        sOrgRow = sSearchRow = aDataPlan->plan.myTuple->row;

        IDE_TEST(
            qmcHashTemp::getFirstSequence( & aDataPlan->
                                           hashMgr[aDataPlan->currTempID],
                                           & sSearchRow )
            != IDE_SUCCESS );
        aDataPlan->plan.myTuple->row = (sSearchRow == NULL ) ? sOrgRow : sSearchRow;

        if ( sSearchRow == NULL )
        {
            // 다음 Temp Table에 Row가 존재할 수 있다.
            sFlag = QMC_ROW_DATA_NONE;
            aDataPlan->currTempID++;
        }
        else
        {
            sFlag = QMC_ROW_DATA_EXIST;
        }
    }

    *aFlag = sFlag;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnHASH::searchNextSequence( qcTemplate * /* aTemplate */,
                             qmncHASH   * aCodePlan,
                             qmndHASH   * aDataPlan,
                             qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    다음 순차 검색을 수행한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnHASH::searchNextSequence"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    void * sOrgRow;
    void * sSearchRow;

    qmcRowFlag sFlag;
    idBool     sIsNextTemp;

    sFlag = QMC_ROW_INITIALIZE;
    sIsNextTemp = ID_FALSE;

    // To Fix PR-8142
    // Data 없음으로 설정하여야 반복 수행 및 올바른 종료가
    // 가능하다.
    sFlag &= ~QMC_ROW_DATA_MASK;
    sFlag |= QMC_ROW_DATA_NONE;

    // Data가 없고, 더 이상의 Temp Table이 없을 때까지 진행
    while ( (aDataPlan->currTempID < aCodePlan->tempTableCnt) &&
            ((sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_NONE) )
    {
        sOrgRow = sSearchRow = aDataPlan->plan.myTuple->row;

        // To Fix PR-12346
        // Temp Table이 다음 Temp Table이라면
        // Cursor를 Open해서 처리할 수 있도록 해야 함.
        if ( sIsNextTemp == ID_TRUE )
        {
            IDE_TEST(
                qmcHashTemp::getFirstSequence( & aDataPlan->
                                               hashMgr[aDataPlan->currTempID],
                                               & sSearchRow )
                != IDE_SUCCESS );

            sIsNextTemp = ID_FALSE;
        }
        else
        {
            IDE_TEST(
                qmcHashTemp::getNextSequence( & aDataPlan->
                                              hashMgr[aDataPlan->currTempID],
                                              & sSearchRow )
                != IDE_SUCCESS );
        }

        aDataPlan->plan.myTuple->row = (sSearchRow == NULL ) ? sOrgRow : sSearchRow;

        if ( sSearchRow == NULL )
        {
            // 다음 Temp Table에 Row가 존재할 수 있다.
            aDataPlan->currTempID++;
            sFlag = QMC_ROW_DATA_NONE;

            sIsNextTemp = ID_TRUE;
        }
        else
        {
            sFlag = QMC_ROW_DATA_EXIST;
        }
    }

    *aFlag = sFlag;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnHASH::searchFirstFilter( qcTemplate * aTemplate,
                            qmncHASH   * aCodePlan,
                            qmndHASH   * aDataPlan,
                            qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    첫번째 범위 검색
 *
 * Implementation :
 *    다음과 같은 순서로 검색한다.
 *        - 상수의 Hash Key 값 및 검색할 Temp Table 선택
 *        - 지정된 Temp Table로부터 검색
 *
 ***********************************************************************/

#define IDE_FN "qmnHASH::searchFirstFilter"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    void    * sOrgRow;
    void    * sSearchRow;

    UInt      sHashKey;

    IDE_TEST( getConstHashKey( aTemplate, aCodePlan, aDataPlan, & sHashKey )
              != IDE_SUCCESS );

    aDataPlan->currTempID = sHashKey % aCodePlan->tempTableCnt;

    sOrgRow = sSearchRow = aDataPlan->plan.myTuple->row;
    IDE_TEST( qmcHashTemp::getFirstRange( & aDataPlan->
                                          hashMgr[aDataPlan->currTempID],
                                          sHashKey,
                                          aCodePlan->filter,
                                          & sSearchRow )
              != IDE_SUCCESS );
    aDataPlan->plan.myTuple->row = (sSearchRow == NULL) ? sOrgRow : sSearchRow;

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
qmnHASH::searchNextFilter( qcTemplate * /* aTemplate */,
                           qmncHASH   * /* aCodePlan */,
                           qmndHASH   * aDataPlan,
                           qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    다음 범위 검색
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnHASH::searchNextFilter"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    void    * sOrgRow;
    void    * sSearchRow;

    sOrgRow = sSearchRow = aDataPlan->plan.myTuple->row;
    IDE_TEST( qmcHashTemp::getNextRange( & aDataPlan->
                                         hashMgr[aDataPlan->currTempID],
                                         & sSearchRow )
              != IDE_SUCCESS );
    aDataPlan->plan.myTuple->row = (sSearchRow == NULL) ? sOrgRow : sSearchRow;

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

IDE_RC qmnHASH::searchFirstHit( qcTemplate * /* aTemplate */,
                                qmncHASH   * aCodePlan,
                                qmndHASH   * aDataPlan,
                                qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    첫번째 Hit 검색
 *
 * Implementation :
 *    Temp Table이 여러 개인 경우를 고려하여 수행하며,
 *    Row Pointer를 잃지 않도록 하여야 한다.
 *
 *    Inverse Hash Join의 처리에 참여할 때 사용되는 함수로
 *    Hit Record가 더 이상 없을 경우, 모든 저장 Row의 Hit Flag을
 *    초기화한다.  이는 Subquery등에서 Inverse Hash Join이 사용될 때,
 *    최초 수행 결과로 인해 다음 Subquery의 수행이 영향을 받지 않도록
 *    하기 위함이다.
 ***********************************************************************/

    UInt        i;
    void      * sOrgRow;
    void      * sSearchRow;
    qmcRowFlag  sFlag;

    sFlag = QMC_ROW_INITIALIZE;
    aDataPlan->currTempID = 0;

    sFlag &= ~QMC_ROW_DATA_MASK;
    sFlag |= QMC_ROW_DATA_NONE;

    // Data가 없고, 더 이상의 Temp Table이 없을 때까지 진행
    while ( (aDataPlan->currTempID < aCodePlan->tempTableCnt) &&
            ((sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_NONE) )
    {
        sOrgRow = sSearchRow = aDataPlan->plan.myTuple->row;
        IDE_TEST(
            qmcHashTemp::getFirstHit( & aDataPlan->
                                      hashMgr[aDataPlan->currTempID],
                                      & sSearchRow )
            != IDE_SUCCESS );
        aDataPlan->plan.myTuple->row = (sSearchRow == NULL ) ? sOrgRow : sSearchRow;

        if ( sSearchRow == NULL )
        {
            // 다음 Temp Table에 Row가 존재할 수 있다.
            sFlag = QMC_ROW_DATA_NONE;
            aDataPlan->currTempID++;
        }
        else
        {
            sFlag = QMC_ROW_DATA_EXIST;
        }
    }

    if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_NONE )
    {
        for ( i = 0; i < aCodePlan->tempTableCnt; i++ )
        {
            IDE_TEST( qmcHashTemp::clearHitFlag( & aDataPlan->hashMgr[i] )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // Nothing To Do
    }

    *aFlag = sFlag;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnHASH::searchNextHit( qcTemplate * /* aTemplate */,
                               qmncHASH   * aCodePlan,
                               qmndHASH   * aDataPlan,
                               qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *     다음 Hit 검색
 *
 * Implementation :
 *    Inverse Hash Join의 처리에 참여할 때 사용되는 함수로
 *    Hit Record가 더 이상 없을 경우, 모든 저장 Row의 Hit Flag을
 *    초기화한다.  이는 Subquery등에서 Inverse Hash Join이 사용될 때,
 *    최초 수행 결과로 인해 다음 Subquery의 수행이 영향을 받지 않도록
 *    하기 위함이다.
 *
 ***********************************************************************/

    UInt       i;
    void     * sOrgRow;
    void     * sSearchRow;
    qmcRowFlag sFlag;
    idBool     sIsNextTemp;

    sFlag = QMC_ROW_INITIALIZE;
    sIsNextTemp = ID_FALSE;

    sFlag &= ~QMC_ROW_DATA_MASK;
    sFlag |= QMC_ROW_DATA_NONE;

    // Data가 없고, 더 이상의 Temp Table이 없을 때까지 진행
    while ( (aDataPlan->currTempID < aCodePlan->tempTableCnt) &&
            ((sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_NONE) )
    {
        sOrgRow = sSearchRow = aDataPlan->plan.myTuple->row;

        if ( sIsNextTemp == ID_TRUE )
        {
            IDE_TEST(
                qmcHashTemp::getFirstHit( & aDataPlan->
                                          hashMgr[aDataPlan->currTempID],
                                          & sSearchRow )
                != IDE_SUCCESS );

            sIsNextTemp = ID_FALSE;
        }
        else
        {
            IDE_TEST(
                qmcHashTemp::getNextHit( & aDataPlan->
                                         hashMgr[aDataPlan->currTempID],
                                         & sSearchRow )
                != IDE_SUCCESS );
        }

        aDataPlan->plan.myTuple->row = (sSearchRow == NULL ) ? sOrgRow : sSearchRow;

        if ( sSearchRow == NULL )
        {
            // 다음 Temp Table에 Row가 존재할 수 있다.
            sFlag = QMC_ROW_DATA_NONE;
            aDataPlan->currTempID++;

            sIsNextTemp = ID_TRUE;
        }
        else
        {
            sFlag = QMC_ROW_DATA_EXIST;
        }
    }

    if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_NONE )
    {
        for ( i = 0; i < aCodePlan->tempTableCnt; i++ )
        {
            // To Fix BUG-8725
            // qmcHashTemp::clear() --> qmcHashTemp::clearHitFlag() 로 변경
            IDE_TEST( qmcHashTemp::clearHitFlag( & aDataPlan->hashMgr[i] )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // Nothing To Do
    }

    *aFlag = sFlag;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnHASH::searchFirstNonHit( qcTemplate * /* aTemplate */,
                            qmncHASH   * aCodePlan,
                            qmndHASH   * aDataPlan,
                            qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    첫번째 Non-Hit 검색
 *
 * Implementation :
 *    Temp Table이 여러 개인 경우를 고려하여 수행하며,
 *    Row Pointer를 잃지 않도록 하여야 한다.
 *
 *    Full Outer Join의 처리에 참여할 때 사용되는 함수로
 *    Non-Hit Record가 더 이상 없을 경우, 모든 저장 Row의 Hit Flag을
 *    초기화한다.  이는 Subquery등에서 Full Outer Join이 사용될 때,
 *    최초 수행 결과로 인해 다음 Subquery의 수행이 영향을 받지 않도록
 *    하기 위함이다.
 ***********************************************************************/

    UInt        i;
    void      * sOrgRow;
    void      * sSearchRow;
    qmcRowFlag  sFlag;

    sFlag = QMC_ROW_INITIALIZE;
    aDataPlan->currTempID = 0;

    // To Fix PR-8142
    // Data 없음으로 설정하여야 반복 수행 및 올바른 종료가
    // 가능하다.
    sFlag &= ~QMC_ROW_DATA_MASK;
    sFlag |= QMC_ROW_DATA_NONE;

    // Data가 없고, 더 이상의 Temp Table이 없을 때까지 진행
    while ( (aDataPlan->currTempID < aCodePlan->tempTableCnt) &&
            ((sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_NONE) )
    {
        sOrgRow = sSearchRow = aDataPlan->plan.myTuple->row;
        IDE_TEST(
            qmcHashTemp::getFirstNonHit( & aDataPlan->
                                         hashMgr[aDataPlan->currTempID],
                                         & sSearchRow )
            != IDE_SUCCESS );
        aDataPlan->plan.myTuple->row = (sSearchRow == NULL ) ? sOrgRow : sSearchRow;

        if ( sSearchRow == NULL )
        {
            // 다음 Temp Table에 Row가 존재할 수 있다.
            sFlag = QMC_ROW_DATA_NONE;
            aDataPlan->currTempID++;
        }
        else
        {
            sFlag = QMC_ROW_DATA_EXIST;
        }
    }

    if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_NONE )
    {
        for ( i = 0; i < aCodePlan->tempTableCnt; i++ )
        {
            // To Fix BUG-8725
            // qmcHashTemp::clear() --> qmcHashTemp::clearHitFlag() 로 변경
            IDE_TEST( qmcHashTemp::clearHitFlag( & aDataPlan->hashMgr[i] )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // Nothing To Do
    }

    *aFlag = sFlag;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnHASH::searchNextNonHit( qcTemplate * /* aTemplate */,
                           qmncHASH   * aCodePlan,
                           qmndHASH   * aDataPlan,
                           qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *     다음 Non-Hit 검색
 *
 * Implementation :
 *    Full Outer Join의 처리에 참여할 때 사용되는 함수로
 *    Non-Hit Record가 더 이상 없을 경우, 모든 저장 Row의 Hit Flag을
 *    초기화한다.  이는 Subquery등에서 Full Outer Join이 사용될 때,
 *    최초 수행 결과로 인해 다음 Subquery의 수행이 영향을 받지 않도록
 *    하기 위함이다.
 *
 ***********************************************************************/
    
    UInt       i;
    void     * sOrgRow;
    void     * sSearchRow;
    qmcRowFlag sFlag;
    idBool     sIsNextTemp;

    sFlag = QMC_ROW_INITIALIZE;
    sIsNextTemp = ID_FALSE;

    // To Fix PR-8142
    // Data 없음으로 설정하여야 반복 수행 및 올바른 종료가
    // 가능하다.
    sFlag &= ~QMC_ROW_DATA_MASK;
    sFlag |= QMC_ROW_DATA_NONE;

    // Data가 없고, 더 이상의 Temp Table이 없을 때까지 진행
    while ( (aDataPlan->currTempID < aCodePlan->tempTableCnt) &&
            ((sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_NONE) )
    {
        sOrgRow = sSearchRow = aDataPlan->plan.myTuple->row;

        // BUG-34831
        // Full outer join 에서 multi-pass hash join 에 대한 고려를 해야 함
        // 다음 temp table 을 읽기 전에 cursor 를 열어야 한다.
        if ( sIsNextTemp == ID_TRUE )
        {
            IDE_TEST(
                qmcHashTemp::getFirstNonHit( & aDataPlan->
                                             hashMgr[aDataPlan->currTempID],
                                             & sSearchRow )
                != IDE_SUCCESS );

            sIsNextTemp = ID_FALSE;
        }
        else
        {
            IDE_TEST(
                qmcHashTemp::getNextNonHit( & aDataPlan->
                                            hashMgr[aDataPlan->currTempID],
                                            & sSearchRow )
                != IDE_SUCCESS );
        }

        aDataPlan->plan.myTuple->row = (sSearchRow == NULL ) ? sOrgRow : sSearchRow;

        if ( sSearchRow == NULL )
        {
            // 다음 Temp Table에 Row가 존재할 수 있다.
            sFlag = QMC_ROW_DATA_NONE;
            aDataPlan->currTempID++;

            sIsNextTemp = ID_TRUE;
        }
        else
        {
            sFlag = QMC_ROW_DATA_EXIST;
        }
    }

    if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_NONE )
    {
        for ( i = 0; i < aCodePlan->tempTableCnt; i++ )
        {
            // To Fix BUG-8725
            // qmcHashTemp::clear() --> qmcHashTemp::clearHitFlag() 로 변경
            IDE_TEST( qmcHashTemp::clearHitFlag( & aDataPlan->hashMgr[i] )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // Nothing To Do
    }

    *aFlag = sFlag;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
