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
 * $Id: qmnGroupAgg.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * Description :
 *     GRAG(GRoup AGgregation) Node
 *
 *     관계형 모델에서 Hash-based Grouping 연산을 수행하는 Plan Node 이다.
 *
 *     Aggregation과 Group By의 형태에 따라 다음과 같은 수행을 한다.
 *         - Grouping Only
 *         - Aggregation Only
 *         - Group Aggregation
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qcuProperty.h>
#include <qmnGroupAgg.h>
#include <qmxResultCache.h>

IDE_RC
qmnGRAG::init( qcTemplate * aTemplate,
               qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    GRAG 노드의 초기화
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnGRAG::init"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncGRAG * sCodePlan = (qmncGRAG *) aPlan;
    qmndGRAG * sDataPlan =
        (qmndGRAG *) (aTemplate->tmplate.data + aPlan->offset);
    qmndGRAG * sCacheDataPlan = NULL;
    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];

    // PROJ-2444 Parallel Aggreagtion
    sDataPlan->plan.mTID = QMN_PLAN_INIT_THREAD_ID;

    //----------------------------------------
    // 최초 초기화 수행
    //----------------------------------------

    if ( (*sDataPlan->flag & QMND_GRAG_INIT_DONE_MASK)
         == QMND_GRAG_INIT_DONE_FALSE )
    {
        if ( ( *sDataPlan->flag & QMN_PLAN_RESULT_CACHE_EXIST_MASK )
             == QMN_PLAN_RESULT_CACHE_EXIST_TRUE )
        {
            sCacheDataPlan = (qmndGRAG *) (aTemplate->resultCache.data + sCodePlan->plan.offset);
            sCacheDataPlan->resultData.flag = &aTemplate->resultCache.dataFlag[sCodePlan->planID];
            sDataPlan->resultData.flag     = &aTemplate->resultCache.dataFlag[sCodePlan->planID];
            if ( qmxResultCache::initResultCache( aTemplate,
                                                  sCodePlan->componentInfo,
                                                  &sCacheDataPlan->resultData )
                 != IDE_SUCCESS )
            {
                *sDataPlan->flag &= ~QMN_PLAN_RESULT_CACHE_EXIST_MASK;
                *sDataPlan->flag |= QMN_PLAN_RESULT_CACHE_EXIST_FALSE;
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
        IDE_TEST( firstInit(aTemplate, sCodePlan, sDataPlan) != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    if ( ( *sDataPlan->flag & QMN_PLAN_RESULT_CACHE_EXIST_MASK )
         == QMN_PLAN_RESULT_CACHE_EXIST_FALSE )
    {
        IDE_TEST( aPlan->left->init( aTemplate,
                                     aPlan->left ) != IDE_SUCCESS);
    }
    else
    {
        if ( ( *sDataPlan->resultData.flag & QMX_RESULT_CACHE_STORED_MASK )
             == QMX_RESULT_CACHE_STORED_FALSE )
        {
            IDE_TEST( aPlan->left->init( aTemplate,
                                         aPlan->left ) != IDE_SUCCESS);
        }
        else
        {
            /* Nothing to do */
        }
        *sDataPlan->resultData.flag &= ~QMX_RESULT_CACHE_INIT_DONE_MASK;
        *sDataPlan->resultData.flag |= QMX_RESULT_CACHE_INIT_DONE_TRUE;
    }

    sDataPlan->doIt = qmnGRAG::doItFirst;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC
qmnGRAG::doIt( qcTemplate * aTemplate,
               qmnPlan    * aPlan,
               qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    GRAG 의 고유 기능을 수행한다.
 *
 * Implementation :
 *    지정된 함수 포인터를 수행한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnGRAG::doIt"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // qmncGRAG * sCodePlan = (qmncGRAG *) aPlan;
    qmndGRAG * sDataPlan =
        (qmndGRAG *) (aTemplate->tmplate.data + aPlan->offset);

    IDE_TEST( sDataPlan->doIt( aTemplate, aPlan, aFlag ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnGRAG::padNull( qcTemplate * aTemplate,
                  qmnPlan    * aPlan )
{
/***********************************************************************
 *
 * Description :
 *    GRAG 노드의 Tuple에 Null Row를 설정한다.
 *
 * Implementation :
 *    Child Plan의 Null Padding을 수행하고,
 *    자신의 Null Row를 Temp Table로부터 획득한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnGRAG::padNull"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncGRAG * sCodePlan = (qmncGRAG *) aPlan;
    qmndGRAG * sDataPlan =
        (qmndGRAG *) (aTemplate->tmplate.data + aPlan->offset);

    // first initialization
    if ( (aTemplate->planFlag[sCodePlan->planID] & QMND_GRAG_INIT_DONE_MASK)
         == QMND_GRAG_INIT_DONE_FALSE )
    {
        IDE_TEST( aPlan->init( aTemplate, aPlan ) != IDE_SUCCESS );
    }

    IDE_TEST( aPlan->left->padNull( aTemplate, aPlan->left )
              != IDE_SUCCESS );

    IDE_TEST(
        qmcHashTemp::getNullRow( sDataPlan->hashMgr,
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
qmnGRAG::printPlan( qcTemplate   * aTemplate,
                    qmnPlan      * aPlan,
                    ULong          aDepth,
                    iduVarString * aString,
                    qmnDisplay     aMode )
{
/***********************************************************************
 *
 * Description :
 *    GRAG 노드의 수행 정보를 출력한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnGRAG::printPlan"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncGRAG * sCodePlan = (qmncGRAG*) aPlan;
    qmndGRAG * sDataPlan =
        (qmndGRAG*) (aTemplate->tmplate.data + aPlan->offset);
    sDataPlan->flag = & aTemplate->planFlag[sCodePlan->planID];
    qmndGRAG * sCacheDataPlan = NULL;
    idBool     sIsInit        = ID_FALSE;
    ULong  i;
    ULong  sPageCnt;
    SLong  sRecordCnt;
    UInt   sBucketCnt;

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

        if ( (*sDataPlan->flag & QMND_GRAG_INIT_DONE_MASK)
             == QMND_GRAG_INIT_DONE_TRUE )
        {
            // 수행 정보 획득
            sIsInit = ID_TRUE;
            if ( sDataPlan->groupNode != NULL )
            {
                IDE_TEST( qmcHashTemp::getDisplayInfo( sDataPlan->hashMgr,
                                                       & sPageCnt,
                                                       & sRecordCnt,
                                                       & sBucketCnt )
                          != IDE_SUCCESS );
            }
            else
            {
                sPageCnt = 0;
                sRecordCnt = 1;
                sBucketCnt = 1;
            }

            if ( (sCodePlan->plan.flag & QMN_PLAN_STORAGE_MASK)
                 == QMN_PLAN_STORAGE_MEMORY )
            {
                if ( QCU_DISPLAY_PLAN_FOR_NATC == 0 )
                {
                    iduVarStringAppendFormat(
                        aString,
                        "GROUP-AGGREGATION ( "
                        "ITEM_SIZE: %"ID_UINT32_FMT", "
                        "GROUP_COUNT: %"ID_INT64_FMT", "
                        "BUCKET_COUNT: %"ID_UINT32_FMT", "
                        "ACCESS: %"ID_UINT32_FMT,
                        sDataPlan->mtrRowSize,
                        sRecordCnt,
                        sBucketCnt,
                        sDataPlan->plan.myTuple->modify );
                }
                else
                {
                    // BUG-29209
                    // ITEM_SIZE 정보 보여주지 않음
                    iduVarStringAppendFormat(
                        aString,
                        "GROUP-AGGREGATION ( "
                        "ITEM_SIZE: BLOCKED, "
                        "GROUP_COUNT: %"ID_INT64_FMT", "
                        "BUCKET_COUNT: %"ID_UINT32_FMT", "
                        "ACCESS: %"ID_UINT32_FMT,
                        sRecordCnt,
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
                        "GROUP-AGGREGATION ( "
                        "ITEM_SIZE: %"ID_UINT32_FMT", "
                        "GROUP_COUNT: %"ID_INT64_FMT", "
                        "DISK_PAGE_COUNT: %"ID_UINT64_FMT", "
                        "ACCESS: %"ID_UINT32_FMT,
                        sDataPlan->mtrRowSize,
                        sRecordCnt,
                        sPageCnt,
                        sDataPlan->plan.myTuple->modify );
                }
                else
                {
                    // BUG-29209
                    // ITEM_SIZE, DISK_PAGE_COUNT 정보 보여주지 않음
                    iduVarStringAppendFormat(
                        aString,
                        "GROUP-AGGREGATION ( "
                        "ITEM_SIZE: BLOCKED, "
                        "GROUP_COUNT: %"ID_INT64_FMT", "
                        "DISK_PAGE_COUNT: BLOCKED, "
                        "ACCESS: %"ID_UINT32_FMT,
                        sRecordCnt,
                        sDataPlan->plan.myTuple->modify );
                }
            }
        }
        else
        {
            iduVarStringAppendFormat(
                aString,
                "GROUP-AGGREGATION ( ITEM_SIZE: 0, "
                "GROUP_COUNT: 0, "
                "BUCKET_COUNT: %"ID_UINT32_FMT", "
                "ACCESS: 0",
                sCodePlan->bucketCnt );
        }
    }
    else
    {
        //----------------------------
        // explain plan = only; 인 경우
        //----------------------------

        iduVarStringAppendFormat( aString,
                                  "GROUP-AGGREGATION ( ITEM_SIZE: ??, "
                                  "GROUP_COUNT: ??, "
                                  "BUCKET_COUNT: %"ID_UINT32_FMT", "
                                  "ACCESS: ??",
                                  sCodePlan->bucketCnt );
    }

    //----------------------------
    // Thread ID
    //----------------------------
    if ( aMode == QMN_DISPLAY_ALL )
    {
        if ( ( (*sDataPlan->flag & QMND_GRAG_INIT_DONE_MASK)
               == QMND_GRAG_INIT_DONE_TRUE ) &&
             ( sDataPlan->plan.mTID != QMN_PLAN_INIT_THREAD_ID ) )
        {
            if ( QCU_DISPLAY_PLAN_FOR_NATC == 0 )
            {
                iduVarStringAppendFormat( aString,
                                          ", TID: %"ID_UINT32_FMT"",
                                          sDataPlan->plan.mTID );
            }
            else
            {
                iduVarStringAppend( aString, ", TID: BLOCKED" );
            }
        }
        else
        {
            // Parallel execution 이 아닌 경우 출력을 생략한다.
        }
    }
    else
    {
        // Planonly 인 경우 출력을 생략한다.
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
    //-----------------------------

    if ( QCU_TRCLOG_DETAIL_MTRNODE == 1 )
    {
        if ( ( sCodePlan->componentInfo != NULL ) &&
             ( ( aTemplate->resultCache.flag & QC_RESULT_CACHE_MAX_EXCEED_MASK )
               == QC_RESULT_CACHE_MAX_EXCEED_FALSE ) &&
             ( ( aTemplate->resultCache.flag & QC_RESULT_CACHE_DATA_ALLOC_MASK )
               == QC_RESULT_CACHE_DATA_ALLOC_TRUE ) )
        {
            sCacheDataPlan = (qmndGRAG *) (aTemplate->resultCache.data + sCodePlan->plan.offset);
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

IDE_RC qmnGRAG::readyIt( qcTemplate * aTemplate,
                         qmnPlan    * aPlan,
                         UInt         aTID )
{
    qmncGRAG * sCodePlan = (qmncGRAG *) aPlan;
    qmndGRAG * sDataPlan =
        (qmndGRAG *) (aTemplate->tmplate.data + aPlan->offset);

    UInt       sModifyCnt;

    // ----------------
    // TID 설정
    // ----------------
    sDataPlan->plan.mTID = aTID;

    if ( sDataPlan->plan.mTID != QMN_PLAN_INIT_THREAD_ID )
    {
        // PROJ-2444
        // 새로운 템플릿에 맞게 mtrNode 초기화, temp table 초기화등의 작업을 다시 해주어야 한다.
        IDE_TEST( firstInit(aTemplate, sCodePlan, sDataPlan) != IDE_SUCCESS );

        // 자식 플랜의 readyIt 함수를 호출한다.
        IDE_TEST( aPlan->left->readyIt( aTemplate,
                                        aPlan->left,
                                        aTID ) != IDE_SUCCESS);
    }
    else
    {
        // Nothing To Do
    }

    // 기존 ACCESS count
    sModifyCnt = sDataPlan->plan.myTuple->modify;

    // ----------------
    // Tuple 위치의 결정
    // ----------------
    sDataPlan->plan.myTuple = &aTemplate->tmplate.rows[sCodePlan->myNode->dstNode->node.table];

    // ACCESS count 원복
    sDataPlan->plan.myTuple->modify = sModifyCnt;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnGRAG::storeTempTable( qcTemplate * aTemplate,
                                qmncGRAG   * aCodePlan,
                                qmndGRAG   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    GARG 노드에서 temp table 에 이용하여 grouping, aggr 작업을 수행한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    idBool sDependency;
    qmndGRAG * sCacheDataPlan = NULL;

    IDE_TEST( checkDependency( aTemplate,
                               aCodePlan,
                               aDataPlan,
                               &sDependency ) != IDE_SUCCESS );

    if ( sDependency == ID_TRUE )
    {
        //----------------------------------------
        // Temp Table 구축 전 초기화
        //----------------------------------------

        IDE_TEST( qmcHashTemp::clear( aDataPlan->hashMgr ) != IDE_SUCCESS );

        //----------------------------------------
        // Child를 반복 수행하여 Temp Table을 구축
        //----------------------------------------

        // PROJ-2444
        // parallel plan 일때 MERGE 단계는 별도의 함수로 구현되어 있다.
        if ( (aCodePlan->flag & QMNC_GRAG_PARALLEL_STEP_MASK) == QMNC_GRAG_PARALLEL_STEP_MERGE )
        {
            if ( aDataPlan->groupNode == NULL )
            {
                IDE_TEST( aggrOnlyMerge( aTemplate,
                                         aCodePlan,
                                         aDataPlan )
                          != IDE_SUCCESS );
            }
            else
            {
                if ( aDataPlan->aggrNode == NULL )
                {
                    IDE_TEST( groupOnlyMerge( aTemplate,
                                              aCodePlan,
                                              aDataPlan )
                              != IDE_SUCCESS );
                }
                else
                {
                    IDE_TEST( groupAggrMerge( aTemplate,
                                              aCodePlan,
                                              aDataPlan )
                              != IDE_SUCCESS );
                }
            }
        }
        else
        {
            if ( aDataPlan->groupNode == NULL )
            {
                //----------------------------------
                // GROUP BY가 없는 경우
                //----------------------------------

                IDE_DASSERT( aDataPlan->aggrNode != NULL );

                IDE_TEST( aggregationOnly( aTemplate,
                                           aCodePlan,
                                           aDataPlan )
                          != IDE_SUCCESS );
            }
            else
            {
                if ( aDataPlan->aggrNode == NULL )
                {
                    //----------------------------------
                    // Aggregation이 없는 경우
                    //----------------------------------

                    IDE_TEST( groupingOnly( aTemplate,
                                            aCodePlan,
                                            aDataPlan ) != IDE_SUCCESS );
                }
                else
                {
                    //----------------------------------
                    // GROUP BY와 Aggregation이 모두 있는 경우
                    //----------------------------------

                    IDE_TEST( groupAggregation( aTemplate,
                                                aCodePlan,
                                                aDataPlan ) != IDE_SUCCESS);
                }
            }
        }

        /* PROJ-2462 Result Cache */
        if ( ( *aDataPlan->flag & QMN_PLAN_RESULT_CACHE_EXIST_MASK )
             == QMN_PLAN_RESULT_CACHE_EXIST_TRUE )
        {
            sCacheDataPlan = (qmndGRAG *) (aTemplate->resultCache.data +
                                          aCodePlan->plan.offset);
            sCacheDataPlan->isNoData = aDataPlan->isNoData;
            *aDataPlan->resultData.flag &= ~QMX_RESULT_CACHE_STORED_MASK;
            *aDataPlan->resultData.flag |= QMX_RESULT_CACHE_STORED_TRUE;
        }
        else
        {
            /* Nothing to do */
        }

        //----------------------------------------
        // Temp Table 구축 후 초기화
        //----------------------------------------

        aDataPlan->depValue = aDataPlan->depTuple->modify;
    }
    else
    {
        // Nothing To Do
    }

    //----------------------------------------
    // 수행 함수 결정
    //----------------------------------------

    if ( aDataPlan->isNoData == ID_FALSE )
    {
        aDataPlan->doIt = qmnGRAG::doItFirst;
    }
    else
    {
        // GROUP BY가 있고 Record가 없을 경우
        aDataPlan->doIt = qmnGRAG::doItNoData;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnGRAG::doItDefault( qcTemplate * /* aTemplate */,
                      qmnPlan    * /* aPlan */,
                      qmcRowFlag * /* aFlag */ )
{
/***********************************************************************
 *
 * Description :
 *    이 함수가 수행되면 안됨.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnGRAG::doItDefault"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_DASSERT( 0 );

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnGRAG::doItFirst( qcTemplate * aTemplate,
                    qmnPlan    * aPlan,
                    qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    첫번째 수행 함수
 *
 * Implementation :
 *    Temp Table로부터 검색을 수행하고, 해당 결과를 Tuple Set에 복원한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnGRAG::doItFirst"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncGRAG * sCodePlan = (qmncGRAG *) aPlan;
    qmndGRAG * sDataPlan =
        (qmndGRAG *) (aTemplate->tmplate.data + aPlan->offset);

    void * sOrgRow;
    void * sSearchRow;

    IDE_TEST( iduCheckSessionEvent( aTemplate->stmt->mStatistics )
              != IDE_SUCCESS );

    // PROJ-2444
    // parallel 플랜이 아닐때 수행해야 한다.
    if ( sDataPlan->plan.mTID == QMN_PLAN_INIT_THREAD_ID )
    {
        IDE_TEST(readyIt(aTemplate, aPlan, QMN_PLAN_INIT_THREAD_ID) != IDE_SUCCESS);
    }
    else
    {
        // Nothing To Do
    }

    // PROJ-2444
    // readyIt 함수에서 temp table 작업을 수행하면 안된다.
    // 이유는 readyIt 함수는 parallel 하게 수행되지 않는다.
    // 별도의 함수로 분리하여 수행한다.
    IDE_TEST( storeTempTable( aTemplate, sCodePlan, sDataPlan ) != IDE_SUCCESS );

    if ( sDataPlan->groupNode != NULL )
    {
        sOrgRow = sSearchRow = sDataPlan->plan.myTuple->row;
        IDE_TEST( qmcHashTemp::getFirstSequence( sDataPlan->hashMgr,
                                                 & sSearchRow )
                  != IDE_SUCCESS );
        sDataPlan->plan.myTuple->row = (sSearchRow == NULL) ? sOrgRow : sSearchRow;

        if ( sSearchRow == NULL )
        {
            *aFlag = QMC_ROW_DATA_NONE;
        }
        else
        {
            *aFlag = QMC_ROW_DATA_EXIST;
            IDE_TEST( setTupleSet( aTemplate, sCodePlan, sDataPlan )
                      != IDE_SUCCESS );
            sDataPlan->doIt = qmnGRAG::doItNext;
        }
    }
    else
    {
        if ( sDataPlan->isNoData == ID_FALSE )
        {
            // Group By가 없는 경우로 하나의 Record만이 존재한다.
            // 현재의 저장 Row가 그 결과가 된다.
            // 따라서, 다음 수행 함수를 Data 없음을 리턴할 수 있는
            // 함수로 설정한다.
            *aFlag = QMC_ROW_DATA_EXIST;
            IDE_TEST( setTupleSet( aTemplate, sCodePlan, sDataPlan )
                    != IDE_SUCCESS );

            sDataPlan->doIt = qmnGRAG::doItNoData;
        }
        else
        {
            // set that no data found
            *aFlag = QMC_ROW_DATA_NONE;

            sDataPlan->doIt = qmnGRAG::doItFirst;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC
qmnGRAG::doItNext( qcTemplate * aTemplate,
                   qmnPlan    * aPlan,
                   qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    다음 수행 함수
 *
 * Implementation :
 *    Temp Table로부터 검색을 수행하고, 해당 결과를 Tuple Set에 복원한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnGRAG::doItNext"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmncGRAG * sCodePlan = (qmncGRAG *) aPlan;
    qmndGRAG * sDataPlan =
        (qmndGRAG *) (aTemplate->tmplate.data + aPlan->offset);

    void * sOrgRow;
    void * sSearchRow;

    sOrgRow = sSearchRow = sDataPlan->plan.myTuple->row;
    IDE_TEST( qmcHashTemp::getNextSequence( sDataPlan->hashMgr,
                                            & sSearchRow )
              != IDE_SUCCESS );
    sDataPlan->plan.myTuple->row = (sSearchRow == NULL) ? sOrgRow : sSearchRow;

    if ( sSearchRow == NULL )
    {
        *aFlag = QMC_ROW_DATA_NONE;
        sDataPlan->doIt = qmnGRAG::doItFirst;
    }
    else
    {
        *aFlag = QMC_ROW_DATA_EXIST;
        IDE_TEST( setTupleSet( aTemplate, sCodePlan, sDataPlan )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnGRAG::doItNoData( qcTemplate * aTemplate,
                     qmnPlan    * aPlan,
                     qmcRowFlag * aFlag )
{
/***********************************************************************
 *
 * Description :
 *    Aggregation Only에서의 다음 수행 함수
 *
 * Implementation :
 *    결과 없음을 리턴한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnGRAG::doItNoData"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // qmncGRAG * sCodePlan = (qmncGRAG *) aPlan;
    qmndGRAG * sDataPlan =
        (qmndGRAG *) (aTemplate->tmplate.data + aPlan->offset);

    // set that no data found
    *aFlag = QMC_ROW_DATA_NONE;

    sDataPlan->doIt = qmnGRAG::doItFirst;

    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC
qmnGRAG::firstInit( qcTemplate * aTemplate,
                    qmncGRAG   * aCodePlan,
                    qmndGRAG   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    GRAG node의 Data 영역의 멤버에 대한 초기화를 수행
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnGRAG::firstInit"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    //---------------------------------
    // 적합성 검사
    //---------------------------------

    //---------------------------------
    // GRAG 고유 정보의 초기화
    //---------------------------------
    // 1. 저장 Column의 초기화
    // 2. Aggregation Column의 초기화
    // 3. Grouping Column 정보의 위치 지정
    // 4. Tuple 위치 지정

    IDE_TEST( initMtrNode( aTemplate, aCodePlan, aDataPlan ) != IDE_SUCCESS );

    if ( aDataPlan->aggrNodeCnt > 0 )
    {
        IDE_TEST( initAggrNode( aTemplate, aCodePlan, aDataPlan )
                  != IDE_SUCCESS );
    }
    else
    {
        aDataPlan->aggrNode = NULL;
    }

    IDE_TEST( initGroupNode( aCodePlan, aDataPlan )
              != IDE_SUCCESS );

    aDataPlan->mtrRowSize = qmc::getMtrRowSize( aDataPlan->mtrNode );

    aDataPlan->plan.myTuple = aDataPlan->mtrNode->dstTuple;

    aDataPlan->depTuple =
        & aTemplate->tmplate.rows[aCodePlan->depTupleRowID];
    aDataPlan->depValue = QMN_PLAN_DEFAULT_DEPENDENCY_VALUE;
    aDataPlan->isNoData = ID_FALSE;

    //---------------------------------
    // Temp Table의 초기화
    //---------------------------------

    IDE_TEST( initTempTable( aTemplate, aCodePlan, aDataPlan )
              != IDE_SUCCESS );

    //---------------------------------
    // 초기화 완료를 표기
    //---------------------------------

    *aDataPlan->flag &= ~QMND_GRAG_INIT_DONE_MASK;
    *aDataPlan->flag |= QMND_GRAG_INIT_DONE_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnGRAG::initMtrNode( qcTemplate * aTemplate,
                      qmncGRAG   * aCodePlan,
                      qmndGRAG   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    저장 Column의 정보를 초기화한다.
 *
 * Implementation :
 *
 ***********************************************************************/
    qmcMtrNode * sNode;
    UInt         sHeaderSize;
    UShort       i;

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
        sHeaderSize = QMC_MEMHASH_TEMPHEADER_SIZE;
    }
    else
    {
        sHeaderSize = QMC_DISKHASH_TEMPHEADER_SIZE;
    }

    //----------------------------------
    // Aggregation 영역의 구분
    //----------------------------------

    // PROJ-1473
    for( sNode = aCodePlan->myNode, i = 0;
         i < aCodePlan->baseTableCount;
         sNode = sNode->next, i++ )
    {
        // Nothing To Do 
    }  

    for ( sNode = sNode,
              aDataPlan->aggrNodeCnt  = 0;
          sNode != NULL;
          sNode = sNode->next )
    {
        if ( ( sNode->flag & QMC_MTR_GROUPING_MASK )
             == QMC_MTR_GROUPING_FALSE )
        {
            // aggregation node should not use converted source node
            //   e.g) SUM( MIN(I1) )
            //        MIN(I1)'s converted node is not aggregation node
            aDataPlan->aggrNodeCnt++;
        }
        else
        {
            break;
        }
    }

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

    //---------------------------------
    // 저장 Column의 초기화
    //---------------------------------

    // 저장 Column의 연결 정보 생성
    IDE_TEST( qmc::linkMtrNode( aCodePlan->myNode,
                                aDataPlan->mtrNode ) != IDE_SUCCESS );

    // 저장 Column의 초기화
    // Aggregation의 저장 시 Conversion값을 저장해서는 안됨
    IDE_TEST( qmc::initMtrNode( aTemplate,
                                aDataPlan->mtrNode,
                                (UShort)aCodePlan->baseTableCount +
                                aDataPlan->aggrNodeCnt )
              != IDE_SUCCESS );

    // 저장 Column의 offset을 재조정.
    IDE_TEST( qmc::refineOffsets( aDataPlan->mtrNode,
                                  sHeaderSize )
              != IDE_SUCCESS );

    // Row Size의 계산
    IDE_TEST( qmc::setRowSize( aTemplate->stmt->qmxMem,
                               & aTemplate->tmplate,
                               aDataPlan->mtrNode->dstNode->node.table )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnGRAG::initAggrNode( qcTemplate * aTemplate,
                       qmncGRAG   * aCodePlan,
                       qmndGRAG   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    Aggregation Column의 초기화
 *
 * Implementation :
 *    Aggregation Column을 초기화하고,
 *    Distinct Aggregation인 경우 해당 Distinct Node를 찾아 연결한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnAGGR::initAggrNode"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UInt          i;
    UShort        sBaseCnt;    
    qmcMtrNode  * sNode;
    qmdMtrNode  * sAggrNode;
    qmdMtrNode  * sPrevNode;
    UInt          sHeaderSize = 0;

    //-----------------------------------------------
    // 적합성 검사
    //-----------------------------------------------

    IDE_DASSERT( aCodePlan->aggrNodeOffset > 0 );

    //-----------------------------------------------
    // Aggregation Node의 연결 정보를 설정하고 초기화
    //-----------------------------------------------

    /* PROJ-2462 Result Cache */
    if ( ( *aDataPlan->flag & QMN_PLAN_RESULT_CACHE_EXIST_MASK )
         == QMN_PLAN_RESULT_CACHE_EXIST_TRUE )
    {
        aDataPlan->aggrNode = ( qmdMtrNode * )( aTemplate->resultCache.data +
                                                aCodePlan->aggrNodeOffset );
    }
    else
    {
        aDataPlan->aggrNode =
            (qmdMtrNode*) (aTemplate->tmplate.data + aCodePlan->aggrNodeOffset);
    }

    for( sBaseCnt = 0,
             sNode = aCodePlan->myNode;
         sBaseCnt < aCodePlan->baseTableCount;
         sNode = sNode->next,
             sBaseCnt++ )
    {
        // Nothing To Do
    }

    // Aggregation Column의 연결 정보 생성
    for ( i = 0,
              sNode = sNode,
              sAggrNode = aDataPlan->aggrNode,
              sPrevNode = NULL;
          i < aDataPlan->aggrNodeCnt;
          i++, sNode = sNode->next, sAggrNode++ )
    {
        sAggrNode->myNode = sNode;
        sAggrNode->srcNode = NULL;
        sAggrNode->next = NULL;

        if ( sPrevNode != NULL )
        {
            sPrevNode->next = sAggrNode;
        }

        sPrevNode = sAggrNode;
    }

    // aggregation node should not use converted source node
    //   e.g) SUM( MIN(I1) )
    //        MIN(I1)'s converted node is not aggregation node
    IDE_TEST( qmc::initMtrNode( aTemplate,
                                (qmdMtrNode*) aDataPlan->aggrNode,
                                (UShort)aDataPlan->aggrNodeCnt )
              != IDE_SUCCESS );

    // Aggregation Column의 offset을 재조정.
    if ( (aCodePlan->plan.flag & QMN_PLAN_STORAGE_MASK)
         == QMN_PLAN_STORAGE_MEMORY )
    {
        sHeaderSize = QMC_MEMHASH_TEMPHEADER_SIZE;
    }
    else
    {
        sHeaderSize = QMC_DISKHASH_TEMPHEADER_SIZE;
    }

    IDE_TEST( qmc::refineOffsets( aDataPlan->mtrNode,
                                  sHeaderSize )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnGRAG::initGroupNode( qmncGRAG   * aCodePlan,
                        qmndGRAG   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *     Grouping Column의 위치를 찾는다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnGRAG::initGroupNode"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UInt i;
    qmdMtrNode * sMtrNode;

    for ( i = 0, sMtrNode = aDataPlan->mtrNode;
          i < ( aCodePlan->baseTableCount + aDataPlan->aggrNodeCnt );
          i++, sMtrNode = sMtrNode->next ) ;

    aDataPlan->groupNode = sMtrNode;

    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC
qmnGRAG::initTempTable( qcTemplate * aTemplate,
                        qmncGRAG   * aCodePlan,
                        qmndGRAG   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *     Hash Temp Table을 초기화한다.
 *
 * Implementation :
 *
 ***********************************************************************/
    UInt        sFlag;
    qmndGRAG  * sCacheDataPlan = NULL;

    //-----------------------------
    // 적합성 검사
    //-----------------------------

    //-----------------------------
    // Flag 정보 초기화
    //-----------------------------

    sFlag = QMCD_HASH_TMP_PRIMARY_TRUE | QMCD_HASH_TMP_DISTINCT_TRUE;

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
    // DISTINCT Hashing은 Bucket List Hashing 방법을 써야 한다.
    sFlag &= ~QMCD_HASH_TMP_HASHING_TYPE;
    sFlag |= QMCD_HASH_TMP_HASHING_BUCKET;

    //-----------------------------
    // Temp Table 초기화
    //-----------------------------
    if ( ( *aDataPlan->flag & QMN_PLAN_RESULT_CACHE_EXIST_MASK )
         == QMN_PLAN_RESULT_CACHE_EXIST_FALSE )
    {
        IDU_FIT_POINT( "qmnGRAG::initTempTable::qmxAlloc:hashMgr",
                       idERR_ABORT_InsufficientMemory );
        IDE_TEST( aTemplate->stmt->qmxMem->alloc( ID_SIZEOF( qmcdHashTemp ),
                                                  (void **)&aDataPlan->hashMgr )
                  != IDE_SUCCESS );
        IDE_TEST( qmcHashTemp::init( aDataPlan->hashMgr,
                                     aTemplate,
                                     ID_UINT_MAX,
                                     aDataPlan->mtrNode,
                                     aDataPlan->groupNode,
                                     aDataPlan->aggrNode,
                                     aCodePlan->bucketCnt,
                                     sFlag )
                  != IDE_SUCCESS );

        if ( aDataPlan->groupNode == NULL )
        {
            IDU_FIT_POINT( "qmnGRAG::initTempTable::qmxAlloc:myTuple->row",
                           idERR_ABORT_InsufficientMemory );
            IDE_TEST( qmcHashTemp::alloc( aDataPlan->hashMgr,
                                          &aDataPlan->plan.myTuple->row )
                      != IDE_SUCCESS );
        }
        else
        {

            /* Nothing to do */
        }
    }
    else
    {
        /* PROJ-2462 Result Cache */
        sCacheDataPlan = (qmndGRAG *) (aTemplate->resultCache.data +
                                      aCodePlan->plan.offset);

        if ( ( *aDataPlan->resultData.flag & QMX_RESULT_CACHE_INIT_DONE_MASK )
             == QMX_RESULT_CACHE_INIT_DONE_FALSE )
        {
            IDU_FIT_POINT( "qmnGRAG::initTempTable::qrcAlloc:hashMgr",
                           idERR_ABORT_InsufficientMemory );
            IDE_TEST( sCacheDataPlan->resultData.memory->alloc( ID_SIZEOF( qmcdHashTemp ),
                                                               (void **)&aDataPlan->hashMgr )
                      != IDE_SUCCESS );

            IDE_TEST( qmcHashTemp::init( aDataPlan->hashMgr,
                                         aTemplate,
                                         sCacheDataPlan->resultData.memoryIdx,
                                         aDataPlan->mtrNode,
                                         aDataPlan->groupNode,
                                         aDataPlan->aggrNode,
                                         aCodePlan->bucketCnt,
                                         sFlag )
                      != IDE_SUCCESS );
            sCacheDataPlan->hashMgr = aDataPlan->hashMgr;

            if ( aDataPlan->groupNode == NULL )
            {
                IDU_FIT_POINT( "qmnGRAG::initTempTable::qrcAlloc:myTuple->row",
                               idERR_ABORT_InsufficientMemory );
                IDE_TEST( qmcHashTemp::alloc( aDataPlan->hashMgr,
                                              &aDataPlan->plan.myTuple->row )
                          != IDE_SUCCESS );
                sCacheDataPlan->resultRow = aDataPlan->plan.myTuple->row;
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            aDataPlan->hashMgr = sCacheDataPlan->hashMgr;
            aDataPlan->plan.myTuple->row = sCacheDataPlan->resultRow;
            aDataPlan->isNoData = sCacheDataPlan->isNoData;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnGRAG::checkDependency( qcTemplate * aTemplate,
                                 qmncGRAG   * aCodePlan,
                                 qmndGRAG   * aDataPlan,
                                 idBool     * aDependent )
{
/***********************************************************************
 *
 * Description :
 *    Dependent Tuple의 변경 여부 검사
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
qmnGRAG::aggregationOnly( qcTemplate * aTemplate,
                          qmncGRAG   * aCodePlan,
                          qmndGRAG   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *     GROUP BY가 없는 경우로 하나의 Row만을 구성하고,
 *     Aggregation을 수행한다.
 *
 * Implementation :
 *     한 번만 메모리를 할당 받고, Temp Table은 사용하지 않는다.
 *
 ***********************************************************************/

#define IDE_FN "qmnGRAG::aggregationOnly"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmcRowFlag sFlag = QMC_ROW_INITIALIZE;

    // Aggregation Column의 초기화
    IDE_TEST( aggrInit( aTemplate, aDataPlan ) != IDE_SUCCESS );

    //-------------------------------------
    // Child의 반복 수행과 Aggregation
    //-------------------------------------

    IDE_TEST( aCodePlan->plan.left->doIt( aTemplate,
                                          aCodePlan->plan.left,
                                          & sFlag ) != IDE_SUCCESS );

    while ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
    {
        //-----------------------------------
        // PROJ-1473
        // 상위노드에서 참조할 컬럼 저장.
        //-----------------------------------       
        IDE_TEST( setBaseColumnRow( aTemplate, aCodePlan, aDataPlan )
                  != IDE_SUCCESS );        
        
        IDE_TEST( aggrDoIt( aTemplate, aDataPlan ) != IDE_SUCCESS );

        IDE_TEST( aCodePlan->plan.left->doIt( aTemplate,
                                              aCodePlan->plan.left,
                                              & sFlag ) != IDE_SUCCESS );
    }

    //----------------------------------------
    // Aggregation Column의 마무리 수행
    //----------------------------------------

    // PROJ-2444
    // AGGR 단계에서는 aggrFini 을 수행하면 안된다.
    if ( (aCodePlan->flag & QMNC_GRAG_PARALLEL_STEP_MASK) != QMNC_GRAG_PARALLEL_STEP_AGGR )
    {
        IDE_TEST( aggrFini( aTemplate, aDataPlan ) != IDE_SUCCESS );
    }
    else
    {
        // Nothing To Do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnGRAG::groupingOnly( qcTemplate * aTemplate,
                       qmncGRAG   * aCodePlan,
                       qmndGRAG   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *     GROUP BY만 존재하고 Aggregation이 없는 경우에 수행
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnGRAG::groupingOnly"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    idBool sInserted;

    qmcRowFlag sFlag = QMC_ROW_INITIALIZE;

    // doIt left child
    IDE_TEST( aCodePlan->plan.left->doIt( aTemplate,
                                          aCodePlan->plan.left,
                                          & sFlag ) != IDE_SUCCESS );

    //-----------------------------------
    // Result Set의 존재 유무 설정
    //-----------------------------------

    if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
    {
        aDataPlan->isNoData = ID_FALSE;
    }
    else
    {
        // GRAG의 수행 결과가 하나도 없는 경우이다.
        aDataPlan->isNoData = ID_TRUE;
    }

    sInserted = ID_TRUE;

    //-----------------------------------
    // Child의 반복 수행을 통한 Grouping 수행
    //-----------------------------------

    while ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
    {
        //-----------------------------------
        // Memory 공간의 할당
        //-----------------------------------

        if ( sInserted == ID_TRUE )
        {
            IDU_FIT_POINT( "qmnGRAG::groupingOnly::alloc::DataPlan",
                            idERR_ABORT_InsufficientMemory );

            IDE_TEST( qmcHashTemp::alloc( aDataPlan->hashMgr,
                                          & aDataPlan->plan.myTuple->row )
                      != IDE_SUCCESS );
        }
        else
        {
            // 삽입이 실패한 경우라면,
            // Memory 공간이 그대로 남아 있다.
            // Nothing To Do
        }

        //-----------------------------------
        // PROJ-1473
        // 상위노드에서 참조할 컬럼 저장.
        //-----------------------------------
        
        IDE_TEST( setBaseColumnRow( aTemplate, aCodePlan, aDataPlan )
                  != IDE_SUCCESS );        

        //-----------------------------------
        // 저장 Row의 구성 및 삽입
        //-----------------------------------

        IDE_TEST( setGroupRow( aTemplate,
                               aDataPlan ) != IDE_SUCCESS );

        IDE_TEST( qmcHashTemp::addDistRow( aDataPlan->hashMgr,
                                           & aDataPlan->plan.myTuple->row,
                                           & sInserted )
                  != IDE_SUCCESS );

        //-----------------------------------
        // Child Plan 수행
        //-----------------------------------

        IDE_TEST( aCodePlan->plan.left->doIt( aTemplate,
                                              aCodePlan->plan.left,
                                              & sFlag ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnGRAG::groupAggregation( qcTemplate * aTemplate,
                           qmncGRAG   * aCodePlan,
                           qmndGRAG   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnGRAG::groupAggregation"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    void   * sOrgRow;
    void   * sSearchRow;
    void   * sAllocRow;

    qmcRowFlag sFlag = QMC_ROW_INITIALIZE;

    IDE_TEST( aCodePlan->plan.left->doIt( aTemplate,
                                          aCodePlan->plan.left,
                                          & sFlag ) != IDE_SUCCESS );

    //-----------------------------------
    // Result Set의 존재 유무 설정
    //-----------------------------------

    if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
    {
        aDataPlan->isNoData = ID_FALSE;
    }
    else
    {
        // GRAG의 수행 결과가 하나도 없는 경우이다.
        aDataPlan->isNoData = ID_TRUE;
    }

    sSearchRow = NULL;
    sAllocRow = NULL;

    while ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
    {
        //-----------------------------------
        // Memory 공간의 할당
        //-----------------------------------

        if ( sSearchRow == NULL )
        {
            IDU_FIT_POINT( "qmnGRAG::groupAggregation::alloc::DataPlan",
                            idERR_ABORT_InsufficientMemory );

            IDE_TEST( qmcHashTemp::alloc( aDataPlan->hashMgr,
                                          & aDataPlan->plan.myTuple->row )
                      != IDE_SUCCESS );
            sAllocRow = aDataPlan->plan.myTuple->row;
        }
        else
        {
            // 삽입이 실패한 경우라면,
            // Memory 공간이 그대로 남아 있다.
            aDataPlan->plan.myTuple->row = sAllocRow;
        }

        //-----------------------------------
        // PROJ-1473
        // 상위노드에서 참조할 컬럼 저장.
        //-----------------------------------
        
        IDE_TEST( setBaseColumnRow( aTemplate, aCodePlan, aDataPlan )
                  != IDE_SUCCESS );        

        //-----------------------------------
        // To Fix PR-8213
        // 동일 Group의 검색
        //-----------------------------------

        IDE_TEST( setGroupRow( aTemplate, aDataPlan )
                  != IDE_SUCCESS );

        sSearchRow = aDataPlan->plan.myTuple->row;
        IDE_TEST( qmcHashTemp::getSameGroup( aDataPlan->hashMgr,
                                             & aDataPlan->plan.myTuple->row,
                                             & sSearchRow )
                  != IDE_SUCCESS );
        
        //-----------------------------------
        // Aggregation 처리
        //-----------------------------------

        if ( sSearchRow == NULL )
        {
            // 동일 Group이 없는 경우로 저장 Row를 구성하여
            // 새로운 Group을 삽입한다.
            IDE_TEST( aggrInit( aTemplate, aDataPlan )
                      != IDE_SUCCESS );

            IDE_TEST( aggrDoIt( aTemplate, aDataPlan )
                      != IDE_SUCCESS );
            
            IDE_TEST( qmcHashTemp::addNewGroup( aDataPlan->hashMgr,
                                                aDataPlan->plan.myTuple->row )
                      != IDE_SUCCESS );
        }
        else
        {
            aDataPlan->plan.myTuple->row = sSearchRow;
            
            // 이미 기존 Group이 존재하는 경우로 Aggregation 수행
            IDE_TEST( aggrDoIt( aTemplate, aDataPlan )
                      != IDE_SUCCESS );

            sOrgRow = aDataPlan->plan.myTuple->row;
            IDE_TEST( qmcHashTemp::updateAggr( aDataPlan->hashMgr )
                      != IDE_SUCCESS );
            aDataPlan->plan.myTuple->row = sOrgRow;
        }

        //-----------------------------------
        // Child Plan 수행
        //-----------------------------------

        IDE_TEST( aCodePlan->plan.left->doIt( aTemplate,
                                              aCodePlan->plan.left,
                                              & sFlag ) != IDE_SUCCESS );
    }

    // PROJ-2444
    // AGGR 단계에서는 aggrFini 을 수행하면 안된다.
    if ( (aCodePlan->flag & QMNC_GRAG_PARALLEL_STEP_MASK) != QMNC_GRAG_PARALLEL_STEP_AGGR )
    {
        //--------------------------------------
        // To-Fix PR-8415
        // 모든 Group에 대하여 Aggregation 완료를 수행하여야 한다.
        // Disk Temp Table의 경우 Memory 상의 Aggregation만을 수행할 경우
        // 상위에 ORDER BY등이 존재하면 Disk 상에는 최종 Aggregation결과가
        // 존재하지 않아 정확한 결과를 도출할 수 없다.
        //--------------------------------------

        sOrgRow = sSearchRow = aDataPlan->plan.myTuple->row;
        IDE_TEST( qmcHashTemp::getFirstGroup( aDataPlan->hashMgr,
                                              & sSearchRow )
                != IDE_SUCCESS );
        aDataPlan->plan.myTuple->row =
            ( sSearchRow == NULL ) ? sOrgRow : sSearchRow;

        while ( sSearchRow != NULL )
        {
            //----------------------------------------
            // Aggregation Column의 마무리 수행
            //----------------------------------------

            IDE_TEST( aggrFini( aTemplate, aDataPlan ) != IDE_SUCCESS );

            sOrgRow = aDataPlan->plan.myTuple->row;
            IDE_TEST( qmcHashTemp::updateFiniAggr( aDataPlan->hashMgr )
                    != IDE_SUCCESS );
            aDataPlan->plan.myTuple->row = sOrgRow;

            //----------------------------------------
            // 새로운 Group 획득
            //----------------------------------------

            sOrgRow = sSearchRow = aDataPlan->plan.myTuple->row;
            IDE_TEST( qmcHashTemp::getNextGroup( aDataPlan->hashMgr,
                                                 & sSearchRow )
                    != IDE_SUCCESS );
            aDataPlan->plan.myTuple->row =
                ( sSearchRow == NULL ) ? sOrgRow : sSearchRow;
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}
IDE_RC qmnGRAG::aggrOnlyMerge( qcTemplate * aTemplate,
                               qmncGRAG   * aCodePlan,
                               qmndGRAG   * aDataPlan )
{
/***********************************************************************
 *
 * Description : PROJ-2444 Parallel Aggreagtion
 *               aggr 를 사용할때
 *               parallel plan 의 merge 단계를 수행한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    void        * sChildRow         = NULL;
    qmnPlan     * sChildPlan        = aCodePlan->plan.left;
    qmndPlan    * sChildDataPlan    = (qmndPlan*)(aTemplate->tmplate.data + sChildPlan->offset);
    qmcRowFlag    sFlag             = QMC_ROW_INITIALIZE;

    IDE_TEST( aggrInit( aTemplate, aDataPlan ) != IDE_SUCCESS );

    //-------------------------------------
    // Child의 반복 수행과 Aggregation
    //-------------------------------------

    IDE_TEST( aCodePlan->plan.left->doIt( aTemplate,
                                          aCodePlan->plan.left,
                                          &sFlag ) != IDE_SUCCESS );

    while ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
    {
        sChildRow = sChildDataPlan->myTuple->row;

        IDE_TEST( aggrMerge( aTemplate,
                             aDataPlan,
                             sChildRow )
                  != IDE_SUCCESS );

        IDE_TEST( aCodePlan->plan.left->doIt( aTemplate,
                                              aCodePlan->plan.left,
                                              & sFlag ) != IDE_SUCCESS );
    }

    //----------------------------------------
    // Aggregation Column의 마무리 수행
    //----------------------------------------

    IDE_TEST( aggrFini( aTemplate, aDataPlan ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnGRAG::groupOnlyMerge( qcTemplate * aTemplate,
                                qmncGRAG   * aCodePlan,
                                qmndGRAG   * aDataPlan )
{
/***********************************************************************
 *
 * Description : PROJ-2444 Parallel Aggreagtion
 *               group by 를 사용할때
 *               parallel plan 의 merge 단계를 수행한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    void        * sChildRow         = NULL;
    qmnPlan     * sChildPlan        = aCodePlan->plan.left;
    qmndPlan    * sChildDataPlan    = (qmndPlan*)(aTemplate->tmplate.data + sChildPlan->offset);
    idBool        sInserted         = ID_TRUE;
    qmcRowFlag    sFlag             = QMC_ROW_INITIALIZE;

    // doIt left child
    IDE_TEST( aCodePlan->plan.left->doIt( aTemplate,
                                          aCodePlan->plan.left,
                                          &sFlag ) != IDE_SUCCESS );

    //-----------------------------------
    // Result Set의 존재 유무 설정
    //-----------------------------------

    if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
    {
        aDataPlan->isNoData = ID_FALSE;
    }
    else
    {
        // GRAG의 수행 결과가 하나도 없는 경우이다.
        aDataPlan->isNoData = ID_TRUE;
    }

    //-----------------------------------
    // Child의 반복 수행을 통한 Grouping 수행
    //-----------------------------------

    while ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
    {
        //-----------------------------------
        // 저장 Row의 구성 및 삽입
        //-----------------------------------

        // disk temp 는 언제나 aDataPlan->plan.myTuple->row 로 작업을 한다.
        sChildRow = sChildDataPlan->myTuple->row;
        aDataPlan->plan.myTuple->row = sChildRow;

        IDE_TEST( qmcHashTemp::addDistRow( aDataPlan->hashMgr,
                                           &sChildRow,  // 검색용 row
                                           &sInserted )
                  != IDE_SUCCESS );

        //-----------------------------------
        // Child Plan 수행
        //-----------------------------------

        IDE_TEST( aCodePlan->plan.left->doIt( aTemplate,
                                              aCodePlan->plan.left,
                                              &sFlag ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qmnGRAG::groupAggrMerge( qcTemplate * aTemplate,
                                qmncGRAG   * aCodePlan,
                                qmndGRAG   * aDataPlan )
{
/***********************************************************************
 *
 * Description : PROJ-2444 Parallel Aggreagtion
 *               Aggr 함수와 group by 를 사용할때
 *               parallel plan 의 merge 단계를 수행한다.
 *
 * Implementation :
 *
 ***********************************************************************/

    void        * sBufferRow        = NULL;
    void        * sSearchRow        = NULL;
    void        * sChildRow         = NULL;
    qmnPlan     * sChildPlan        = aCodePlan->plan.left;
    qmndPlan    * sChildDataPlan    = (qmndPlan*)(aTemplate->tmplate.data + sChildPlan->offset);
    qmcRowFlag    sFlag             = QMC_ROW_INITIALIZE;

    IDE_TEST( aCodePlan->plan.left->doIt( aTemplate,
                                          aCodePlan->plan.left,
                                          & sFlag ) != IDE_SUCCESS );

    //-----------------------------------
    // Result Set의 존재 유무 설정
    //-----------------------------------

    if ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
    {
        aDataPlan->isNoData = ID_FALSE;
    }
    else
    {
        // GRAG의 수행 결과가 하나도 없는 경우이다.
        aDataPlan->isNoData = ID_TRUE;
    }

    sBufferRow = aDataPlan->plan.myTuple->row;

    while ( (sFlag & QMC_ROW_DATA_MASK) == QMC_ROW_DATA_EXIST )
    {
        // sChildRow 는 현재 GRAG 노드의 mtr 구조와 동일하다.
        sChildRow  = sChildDataPlan->myTuple->row;
        sSearchRow = sBufferRow;

        // 메모리 temp 는 sSearchRow 에 주소값을 저장하고
        // 디스크 temp 는 *(sSearchRow) 의 위치에 값을 복사한다.
        IDE_TEST( qmcHashTemp::getSameGroup( aDataPlan->hashMgr,
                                             &sChildRow,    // 검색용 row
                                             &sSearchRow )  // 결과값
                  != IDE_SUCCESS );

        //-----------------------------------
        // Aggregation 처리
        //-----------------------------------

        if ( sSearchRow == NULL )
        {
            //-----------------------------------
            // PROJ-2527 WITHIN GROUP AGGR
            // Memory 할당
            // parent의 function data 에 child function data merge한다.
            //-----------------------------------
            
            // 동일 Group이 없는 경우로 저장 Row를 구성하여
            // 새로운 Group을 삽입한다.
            // 메모리 temp 는 이미 insert 가 완료된 상태
            // 디스크 temp 는 aDataPlan->plan.myTuple->row 의 값을 insert 한다.
            IDE_TEST( qmcHashTemp::alloc( aDataPlan->hashMgr,
                                          &aDataPlan->plan.myTuple->row )
                      != IDE_SUCCESS );

            // copy group keys
            idlOS::memcpy( aDataPlan->plan.myTuple->row,
                           sChildRow,
                           aDataPlan->plan.myTuple->rowOffset );
            
            IDE_TEST( aggrInit( aTemplate, aDataPlan )
                      != IDE_SUCCESS );
            
            IDE_TEST( aggrMerge( aTemplate,
                                 aDataPlan,
                                 sChildRow )
                      != IDE_SUCCESS );

            IDE_TEST( qmcHashTemp::addNewGroup( aDataPlan->hashMgr,
                                                aDataPlan->plan.myTuple->row )
                      != IDE_SUCCESS );
        }
        else
        {
            // 아래 작업들은 aDataPlan->plan.myTuple->row 을 이용한다.
            aDataPlan->plan.myTuple->row = sSearchRow;
            
            IDE_TEST( aggrMerge( aTemplate,
                                 aDataPlan,
                                 sChildRow )
                      != IDE_SUCCESS );

            IDE_TEST( qmcHashTemp::updateAggr( aDataPlan->hashMgr )
                      != IDE_SUCCESS );
        }

        //-----------------------------------
        // Child Plan 수행
        //-----------------------------------

        IDE_TEST( aCodePlan->plan.left->doIt( aTemplate,
                                              aCodePlan->plan.left,
                                              &sFlag ) != IDE_SUCCESS );
    }

    //----------------------------------------
    // Aggregation Column의 마무리 수행
    //----------------------------------------

    sSearchRow = aDataPlan->plan.myTuple->row;

    IDE_TEST( qmcHashTemp::getFirstGroup( aDataPlan->hashMgr,
                                          &sSearchRow )
              != IDE_SUCCESS );

    while ( sSearchRow != NULL )
    {
        aDataPlan->plan.myTuple->row = sSearchRow;

        IDE_TEST( aggrFini( aTemplate, aDataPlan ) != IDE_SUCCESS );

        IDE_TEST( qmcHashTemp::updateFiniAggr( aDataPlan->hashMgr )
                  != IDE_SUCCESS );

        //----------------------------------------
        // 새로운 Group 획득
        //----------------------------------------

        IDE_TEST( qmcHashTemp::getNextGroup( aDataPlan->hashMgr,
                                             & sSearchRow )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnGRAG::setBaseColumnRow( qcTemplate * aTemplate,
                           qmncGRAG   * aCodePlan,
                           qmndGRAG   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *     Grouping Column의 값을 설정
 *
 * Implementation :
 *
 ***********************************************************************/

    UShort       i;    
    qmdMtrNode * sNode;

    for ( sNode = aDataPlan->mtrNode, i = 0;
          ( sNode != NULL ) && 
              ( i < aCodePlan->baseTableCount );
          sNode = sNode->next, i++ )
    {
        IDE_TEST( sNode->func.setMtr( aTemplate,
                                      sNode,
                                      aDataPlan->plan.myTuple->row )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC
qmnGRAG::setGroupRow( qcTemplate * aTemplate,
                      qmndGRAG   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *     Grouping Column의 값을 설정
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnGRAG::setGroupRow"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    // idBool       sExist;
    qmdMtrNode * sNode;

    for ( sNode = aDataPlan->groupNode;
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
qmnGRAG::aggrInit( qcTemplate * aTemplate,
                   qmndGRAG   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    Aggregation Column의 값을 초기화한다.
 *
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnGRAG::aggrInit"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmdMtrNode * sNode;

    for ( sNode = aDataPlan->aggrNode;
          sNode != NULL;
          sNode = sNode->next )
    {
        IDE_TEST( qtc::initialize( sNode->myNode->dstNode, aTemplate )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC
qmnGRAG::aggrDoIt( qcTemplate * aTemplate,
                   qmndGRAG   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    Aggregation Column에 대한 연산 수행
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnGRAG::aggrDoIt"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmdMtrNode * sNode;

    for ( sNode = aDataPlan->aggrNode;
          sNode != NULL;
          sNode = sNode->next )
    {
        IDE_TEST( qtc::aggregate( sNode->dstNode, aTemplate )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC
qmnGRAG::aggrMerge( qcTemplate * aTemplate,
                    qmndGRAG   * aDataPlan,
                    void       * aSrcRow )
{
/***********************************************************************
 *
 * Description :
 *    Aggregation Column에 대한 연산 수행
 * Implementation :
 *
 ***********************************************************************/

    qmdMtrNode * sNode;

    for ( sNode = aDataPlan->aggrNode;
          sNode != NULL;
          sNode = sNode->next )
    {
        IDE_TEST( aTemplate->tmplate.
                             rows[sNode->dstNode->node.table].
                             execute[sNode->dstNode->node.column].
             merge( (mtcNode*)sNode->dstNode,
                    aTemplate->tmplate.stack,
                    aTemplate->tmplate.stackRemain,
                    aSrcRow,
                    &aTemplate->tmplate ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qmnGRAG::aggrFini( qcTemplate * aTemplate,
                   qmndGRAG   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *    Aggregation Column에 대한 마무리
 * Implementation :
 *
 ***********************************************************************/

#define IDE_FN "qmnGRAG::aggrFini"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qmdMtrNode * sNode;

    for ( sNode = aDataPlan->aggrNode;
          sNode != NULL;
          sNode = sNode->next )
    {
        IDE_TEST( qtc::finalize( sNode->dstNode, aTemplate )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC
qmnGRAG::setTupleSet( qcTemplate * aTemplate,
                      qmncGRAG   * aCodePlan,
                      qmndGRAG   * aDataPlan )
{
/***********************************************************************
 *
 * Description :
 *     저장 Row를 이용하여 Tuple Set을 복원
 *
 * Implementation :
 *     Grouping Column을 Tuple Set에 복원하고
 *     Aggregation Column에 대한 마무리 연산을 수행한다.
 *
 ***********************************************************************/

#define IDE_FN "qmnGRAG::setTupleSet"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UShort       i;    
    qmdMtrNode * sNode;

    //----------------------------------------
    // PROJ-1473
    // 상위노드에서 참조해야 될 저장컬럼의 복원
    //----------------------------------------

    for ( sNode = aDataPlan->mtrNode, i = 0;
          ( sNode != NULL ) &&
              ( i < aCodePlan->baseTableCount );
          sNode = sNode->next, i++ )
    {
        IDE_TEST( sNode->func.setTuple( aTemplate, sNode, aDataPlan->plan.myTuple->row )
                  != IDE_SUCCESS );
    }

    //----------------------------------------
    // Grouping Column의 Tuple Set 복원
    //----------------------------------------

    for ( sNode = aDataPlan->groupNode;
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
